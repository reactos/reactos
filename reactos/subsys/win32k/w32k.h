/*
 * Precompiled headers for win32k.sys
 */

#define __WIN32K__
#define NTOS_MODE_KERNEL

#include <roscfg.h>
#include <roskrnl.h>

#include <ddk/winddi.h>
#include <ddk/ntddk.h>
#include <ddk/ntddmou.h>

#include <internal/ob.h>
#include <internal/safe.h>
#include <internal/kbd.h>
#include <internal/ps.h>

#include <napi/win32.h>
#include <ntos.h>
#include <math.h>
#include <float.h>
#include <windows.h>

#include <win32k/win32k.h>
#include <csrss/csrss.h>

#include <rosrtl/string.h>
#include <user32/callback.h>

#include <eng/objects.h>
#include <eng/misc.h>

#include <include/misc.h>
#include <include/color.h>
#include <include/dib.h>
#include <include/eng.h>
#include <include/error.h>
#include <include/guicheck.h>
#include <include/inteng.h>
#include <include/intgdi.h>
#include <include/object.h>
#include <include/paint.h>
#include <include/palette.h>
#include <include/path.h>
#include <include/rect.h>
#include <include/surface.h>
#include <include/tags.h>
#include <include/text.h>
#include <include/useratom.h>
#include <include/internal.h>
#include <include/callback.h>

#include <dib/dib.h>

#define NDEBUG
#include <win32k/debug1.h>

