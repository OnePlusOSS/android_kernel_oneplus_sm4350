#ifndef _ONEPLUS_DISPLAY_PANEL_CABC_H_
#define _ONEPLUS_DISPLAY_PANEL_CABC_H_

#include <linux/err.h>
#include "dsi_display.h"
#include "dsi_panel.h"
#include "dsi_ctrl.h"
#include "dsi_ctrl_hw.h"
#include "dsi_drm.h"
#include "dsi_clk.h"
#include "dsi_pwr.h"
#include "sde_dbg.h"

int oplus_display_panel_get_cabc(void *data);
int oplus_display_panel_set_cabc(void *data);
int oplus_dsi_update_cabc_mode(void);
int __oplus_display_set_cabc(int mode);
int dsi_panel_cabc_mode(struct dsi_panel *panel, int mode);
int dsi_panel_cabc_mode_unlock(struct dsi_panel *panel, int mode);
int dsi_display_cabc_mode(struct dsi_display *display, int mode);

#endif  /* _ONEPLUS_DISPLAY_PANEL_CABC_H_ */
