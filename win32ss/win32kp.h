/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            win32ss/win32kp.h
 * PURPOSE:         Internal Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#pragma once

/* Enable debugging features */
#define GDI_DEBUG 0
#define DBG_ENABLE_GDIOBJ_BACKTRACES 0
#define DBG_ENABLE_EVENT_LOGGING 0
#define DBG_ENABLE_SERVICE_HOOKS 0

/* Internal NtGdi Headers */
typedef struct _DC *PDC;
#include "gdi/ntgdi/rect.h"
#include "gdi/ntgdi/misc.h"
#include "gdi/ntgdi/gdiobj.h"
#include "gdi/ntgdi/palette.h"
#include "gdi/eng/surface.h"
#include "gdi/eng/mdevobj.h"
#include "gdi/eng/pdevobj.h"
#include "gdi/eng/ldevobj.h"
#include "gdi/eng/device.h"
#include "gdi/eng/driverobj.h"
#include "gdi/eng/engobjects.h"
#include "gdi/eng/eng.h"
#include "gdi/eng/engevent.h"
#include "gdi/eng/inteng.h"
#include "gdi/eng/xlateobj.h"
#include "gdi/eng/floatobj.h"
#include "gdi/eng/mouse.h"
#include "gdi/eng/mapping.h"
#include "gdi/ntgdi/xformobj.h"
#include "gdi/ntgdi/brush.h"
#include "gdi/ntgdi/color.h"
#include "gdi/ntgdi/bitmaps.h"
#include "gdi/ntgdi/region.h"
#include "gdi/ntgdi/dc.h"
#include "gdi/ntgdi/dib.h"
#include "gdi/ntgdi/cliprgn.h"
#include "gdi/ntgdi/intgdi.h"
#include "gdi/ntgdi/paint.h"
#include "gdi/ntgdi/text.h"
#include "gdi/ntgdi/pen.h"
#include "gdi/ntgdi/cliprgn.h"
#include "gdi/ntgdi/coord.h"
#include "gdi/ntgdi/path.h"
#include "gdi/dib/dib.h"
#include "reactx/ntddraw/intddraw.h"

/* Internal NtUser Headers */
#include "user/ntuser/win32kdebug.h"
#include "user/ntuser/win32.h"
#include "user/ntuser/tags.h"
#ifndef __cplusplus
#include "user/ntuser/ntuser.h"
#include "user/ntuser/usrheap.h"
#include "user/ntuser/object.h"
#include "user/ntuser/shutdown.h"
#include "user/ntuser/cursoricon.h"
#include "user/ntuser/accelerator.h"
#include "user/ntuser/hook.h"
#include "user/ntuser/clipboard.h"
#include "user/ntuser/winsta.h"
#include "user/ntuser/msgqueue.h"
#include "user/ntuser/desktop.h"
#include "user/ntuser/dce.h"
#include "user/ntuser/focus.h"
#include "user/ntuser/hotkey.h"
#include "user/ntuser/input.h"
#include "user/ntuser/menu.h"
#include "user/ntuser/monitor.h"
#include "user/ntuser/timer.h"
#include "user/ntuser/caret.h"
#include "user/ntuser/painting.h"
#include "user/ntuser/class.h"
#include "user/ntuser/window.h"
#include "user/ntuser/security.h"
#include "user/ntuser/sysparams.h"
#include "user/ntuser/prop.h"
#include "user/ntuser/guicheck.h"
#include "user/ntuser/useratom.h"
#include "user/ntuser/vis.h"
#include "user/ntuser/userfuncs.h"
#include "user/ntuser/scroll.h"
#include "user/ntuser/winpos.h"
#include "user/ntuser/callback.h"
#include "user/ntuser/mmcopy.h"
#include "user/ntuser/ghost.h"

/* CSRSS Interface */
#include "user/ntuser/csr.h"

#endif // __cplusplus

#include "gdi/ntgdi/gdidebug.h"
