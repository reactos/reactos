/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/include/win32k.h
 * PURPOSE:         Internal Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#ifndef __WIN32K_H
#define __WIN32K_H
#define INTERNAL_CALL APIENTRY

/* Internal Win32k Headers */
#include <include/accelerator.h>
#include <include/clipboard.h>
#include <include/cliprgn.h>
#include <include/bitmaps.h>
#include <include/brush.h>
#include <include/callback.h>
#include <include/caret.h>
#include <include/class.h>
#include <include/cleanup.h>
#include <include/color.h>
#include <include/coord.h>
#include <include/csr.h>
#include <include/dc.h>
#include <include/dce.h>
#include <include/dib.h>
#include <include/driver.h>
#include <include/driverobj.h>
#include <include/error.h>
#include <include/floatobj.h>
#include <include/gdiobj.h>
#include <include/palette.h>
#include <include/rect.h>
#include <include/win32.h>
#include <include/window.h>
#include <include/winsta.h>
#include <include/xformobj.h>

#include <include/region.h>
#include <include/ntuser.h>
#include <include/cursoricon.h>
#include <include/desktop.h>
#include <include/eng.h>
#include <include/focus.h>
#include <include/guicheck.h>
#include <include/hook.h>
#include <include/hotkey.h>
#include <include/input.h>
#include <include/inteng.h>
#include <include/intgdi.h>
#include <include/intddraw.h>
#include <include/menu.h>
#include <include/monitor.h>
#include <include/mouse.h>
#include <include/msgqueue.h>
#include <include/object.h>
#include <include/paint.h>
#include <include/painting.h>
#include <include/path.h>
#include <include/prop.h>
#include <include/scroll.h>
#include <include/surface.h>
#include <include/tags.h>
#include <include/text.h>
#include <include/timer.h>
#include <include/useratom.h>
#include <include/vis.h>
#include <include/userfuncs.h>
#include <include/winpos.h>
#include <include/mmcopy.h>
#include <include/misc.h>
#include <include/gdifloat.h>
#include <eng/objects.h>
#include <eng/misc.h>
#include <dib/dib.h>

#endif /* __WIN32K_H */
