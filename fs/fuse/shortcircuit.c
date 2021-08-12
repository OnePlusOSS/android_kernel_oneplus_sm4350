// SPDX-License-Identifier: GPL-2.0

#include "fuse_i.h"

#include <linux/aio.h>
#include <linux/fs_stack.h>

#ifdef CONFIG_FUSE_DECOUPLING
#include <linux/moduleparam.h>

int __read_mostly sct_mode = 2;
module_param(sct_mode, int, 0644);

static char *__dentry_name(struct dentry *dentry, char *name)
{
	char *p = dentry_path_raw(dentry, name, PATH_MAX);

	if (IS_ERR(p)) {
		__putname(name);
		return NULL;
	}

	/*
	 * This function relies on the fact that dentry_path_raw() will place
	 * the path name at the end of the provided buffer.
	 */
	BUG_ON(p + strlen(p) + 1 != name + PATH_MAX);

	if (p > name)
		strlcpy(name, p, PATH_MAX);

	return name;
}

static char *dentry_name(struct dentry *dentry)
{
	char *name = __getname();

	if (!name)
		return NULL;

	return __dentry_name(dentry, name);
}

char *inode_name(struct inode *ino)
{
	struct dentry *dentry;
	char *name;

	if (sct_mode != 1)
		return NULL;

	dentry = d_find_alias(ino);
	if (!dentry)
		dentry = d_find_any_alias(ino);

	if (!dentry)
		return NULL;

	name = dentry_name(dentry);

	dput(dentry);

	return name;
}
#endif

int fuse_shortcircuit_setup(struct fuse_conn *fc, struct fuse_req *req)
{
	int  fd, flags;
	struct fuse_open_out *open_out;
	struct file *lower_filp;
	unsigned short open_out_index;
#ifdef CONFIG_FUSE_DECOUPLING
	struct fuse_package *fp = current->fpack;
#endif

	req->lower_filp = NULL;

	if (!fc->shortcircuit)
		return 0;

#ifdef CONFIG_FUSE_DECOUPLING
	if (!sct_mode)
		return 0;
#endif

	if (!(req->in.h.opcode == FUSE_OPEN && req->args->out_numargs == 1) &&
	    !(req->in.h.opcode == FUSE_CREATE && req->args->out_numargs == 2))
		return 0;

	open_out_index = req->args->out_numargs - 1;

	if (req->args->out_args[open_out_index].size != sizeof(*open_out))
		return 0;

	open_out = req->args->out_args[open_out_index].value;
	if (!open_out->fh)
		return 0;

	flags = open_out->open_flags;
	if ((flags & FOPEN_DIRECT_IO) || !(flags & FOPEN_KEEP_CACHE)) {
		pr_info("fuse: bypass sct #flags:%d\n", flags);
		return 0;
	}

#ifdef CONFIG_FUSE_DECOUPLING
	if (sct_mode == 1) {
		if (fp) {
			req->lower_filp = fp->filp;
			fp->filp = NULL;
		}
		return 0;
	}

	if (fp && fp->filp) {
		fput(fp->filp);
		fp->filp = NULL;
	}
#endif

	if (get_user(fd, (int __user *)open_out->fh))
		return -EINVAL;

	if (fd <= 1 || fd >= current->signal->rlim[RLIMIT_NOFILE].rlim_max) {
		pr_info("fuse: bypass sct:%d, %d\n", fd, flags);
		return -EINVAL;
	}

	lower_filp = fget(fd);
	if (!lower_filp) {
		pr_err("fuse: invalid file descriptor for sct.\n");
		return -EINVAL;
	}

	if (!lower_filp->f_op->read_iter ||
	    !lower_filp->f_op->write_iter) {
		pr_err("fuse: sct file misses file operations.\n");
		fput(lower_filp);
		return -EINVAL;
	}

	req->lower_filp = lower_filp;
	pr_debug("fuse: setup sct:%d, %d\n", fd, flags);
	return 0;
}


static inline ssize_t fuse_shortcircuit_read_write_iter(struct kiocb *iocb,
						struct iov_iter *iter,
						bool write)
{
	struct file *fuse_filp = iocb->ki_filp;
	struct fuse_file *ff = fuse_filp->private_data;
	struct file *lower_filp = ff->lower_filp;
	struct inode *shortcircuit_inode;
	struct inode *fuse_inode;
	ssize_t ret = -EIO;

	fuse_inode = fuse_filp->f_path.dentry->d_inode;
	shortcircuit_inode = file_inode(lower_filp);

	iocb->ki_filp = lower_filp;

	if (write) {
		if (!lower_filp->f_op->write_iter)
			goto out;

		ret = call_write_iter(lower_filp, iocb, iter);
		if (ret >= 0 || ret == -EIOCBQUEUED) {
			fsstack_copy_inode_size(fuse_inode, shortcircuit_inode);
			fsstack_copy_attr_times(fuse_inode, shortcircuit_inode);
		}
	} else {
		if (!lower_filp->f_op->read_iter)
			goto out;

		ret = call_read_iter(lower_filp, iocb, iter);
		if (ret >= 0 || ret == -EIOCBQUEUED)
			fsstack_copy_attr_atime(fuse_inode, shortcircuit_inode);
	}

out:
	iocb->ki_filp = fuse_filp;

	return ret;
}

ssize_t fuse_shortcircuit_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	return fuse_shortcircuit_read_write_iter(iocb, to, false);
}

ssize_t fuse_shortcircuit_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	return fuse_shortcircuit_read_write_iter(iocb, from, true);
}

void fuse_shortcircuit_release(struct fuse_file *ff)
{
	if (ff->lower_filp) {
		fput(ff->lower_filp);
		ff->lower_filp = NULL;
	}
}

