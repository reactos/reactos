/*
 * Precompiled headers for win32k.sys
 */

#define __WIN32K__
#define NTOS_MODE_KERNEL

#include <roskrnl.h>

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>

#include <win32k/win32k.h>
#include <csrss/csrss.h>

#include <internal/ob.h>
#include <internal/safe.h>

#include <napi/win32.h>
#include <ntos.h>
#include <math.h>
#include <float.h>
#include <windows.h>

#include <rosrtl/string.h>
#include <user32/callback.h>

#include <include/accelerator.h>
#include <include/callback.h>
#include <include/caret.h>
#include <include/class.h>
#include <include/cleanup.h>
#include <include/clipboard.h>
#include <include/color.h>
#include <include/cursoricon.h>
#include <include/dce.h>
#include <include/desktop.h>
#include <include/dib.h>
#include <include/eng.h>
#include <include/error.h>
#include <include/focus.h>
#include <include/guicheck.h>
#include <include/hook.h>
#include <include/hotkey.h>
#include <include/input.h>
#include <include/inteng.h>
#include <include/intgdi.h>
#include <include/menu.h>
#include <include/mouse.h>
#include <include/msgqueue.h>
#include <include/object.h>
#include <include/paint.h>
#include <include/painting.h>
#include <include/palette.h>
#include <include/path.h>
#include <include/prop.h>
#include <include/rect.h>
#include <include/scroll.h>
#include <include/surface.h>
#include <include/tags.h>
#include <include/text.h>
#include <include/timer.h>
#include <include/timer.h>
#include <include/useratom.h>
#include <include/vis.h>
#include <include/window.h>
#include <include/winpos.h>
#include <include/winsta.h>

#include <eng/objects.h>
#include <eng/misc.h>
#include <eng/handle.h>
#include <eng/clip.h>
#include <eng/brush.h>

#include <dib/dib.h>

#define NDEBUG
#include <win32k/debug1.h>

