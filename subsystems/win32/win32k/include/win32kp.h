/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/include/win32k.h
 * PURPOSE:         Internal Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#pragma once
#define INTERNAL_CALL APIENTRY

#ifndef _MSC_VER
#define PLACE_IN_SECTION(s) __attribute__((section(s)))
#define INIT_FUNCTION PLACE_IN_SECTION("INIT")
#else
#define INIT_FUNCTION
#endif

/* Enable debugging features */
#define GDI_DEBUG 0
#define DBG_ENABLE_EVENT_LOGGING 0
#define DBG_ENABLE_SERVICE_HOOKS 0

/* misc headers  */
#include <include/mmcopy.h>
#include <include/tags.h>
#include <include/win32kdebug.h>
#include <include/cleanup.h>

/* Internal NtGdi Headers */
#include <include/gdiobj.h>
#include <include/surface.h>
#include <include/brush.h>
#include <include/color.h>
#include <include/dc.h>
#include <include/dib.h>
#include <include/driver.h>
#include <include/driverobj.h>
#include <include/floatobj.h>
#include <include/palette.h>
#include <include/pdevobj.h>
#include <include/rect.h>
#include <include/xformobj.h>
#include <include/xlateobj.h>
#include <include/cliprgn.h>
#include <include/region.h>
#include <include/eng.h>
#include <include/inteng.h>
#include <include/intgdi.h>
#include <include/intddraw.h>
#include <include/paint.h>
#include <include/path.h>
#include <include/bitmaps.h>
#include <include/text.h>
#include <include/misc.h>
#include <include/engobjects.h>
#include <include/engevent.h>
#include <include/ldevobj.h>
#include <include/device.h>
#include <include/pen.h>
#include <include/cliprgn.h>
#include <include/coord.h>
#include <include/gdifloat.h>
#include <dib/dib.h>
#include <include/gdidebug.h>

/* Internal NtUser Headers */
#include <include/ntuser.h>
#include <include/accelerator.h>
#include <include/clipboard.h>
#include <include/caret.h>
#include <include/class.h>
#include <include/dce.h>
#include <include/sysparams.h>
#include <include/win32.h>
#include <include/window.h>
#include <include/winsta.h>
#include <include/cursoricon.h>
#include <include/desktop.h>
#include <include/focus.h>
#include <include/guicheck.h>
#include <include/hook.h>
#include <include/hotkey.h>
#include <include/input.h>
#include <include/menu.h>
#include <include/monitor.h>
#include <include/mouse.h>
#include <include/msgqueue.h>
#include <include/object.h>
#include <include/painting.h>
#include <include/prop.h>
#include <include/timer.h>
#include <include/useratom.h>
#include <include/vis.h>
#include <include/userfuncs.h>
#include <include/winpos.h>
#include <include/scroll.h>
#include <include/csr.h>
#include <include/callback.h>
