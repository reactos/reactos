#ifndef GDI_SW_WINSYS_H
#define GDI_SW_WINSYS_H

#include <windows.h>

#include "pipe/p_compiler.h"
#include "state_tracker/sw_winsys.h"

void gdi_sw_display( struct sw_winsys *winsys,
                     struct sw_displaytarget *dt,
                     HDC hDC );

struct sw_winsys *
gdi_create_sw_winsys(void);

#endif
