#ifndef __INCLUDE_NTOSKRNL_H
#define __INCLUDE_NTOSKRNL_H

#define __NO_CTYPE_INLINES

/* include the ntoskrnl config.h file */
#include "config.h"

#include <roscfg.h>
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <roskrnl.h>
#include <ntos/minmax.h>
#include <ntos/synch.h>
#include <ntos/keyboard.h>
#include <ntos/ntdef.h>
#include <rosrtl/minmax.h>
#include <rosrtl/string.h>
#include <ddk/halfuncs.h>
#include <ddk/kefuncs.h>
#include <ddk/pnptypes.h>
#include <ddk/pnpfuncs.h>
#include <ntdll/ldr.h>
#include <internal/ctype.h>
#include <internal/ntoskrnl.h>
#include <internal/id.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/i386/fpu.h>
#include <internal/module.h>
#include <internal/handle.h>
#include <internal/pool.h>
#include <internal/ps.h>
#include <internal/mm.h>
#include <internal/cc.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ob.h>
#include <internal/se.h>
#include <internal/ldr.h>
#include <internal/kd.h>
#include <internal/ex.h>
#include <internal/ob.h>
#include "internal/xhal.h"
#include <internal/v86m.h>
#include <internal/ifs.h>
#include <internal/port.h>
#include <internal/nls.h>
#include <internal/dbg.h>
#include <internal/trap.h>
#include <internal/safe.h>
#include <internal/test.h>
#include <napi/core.h>
#include <napi/dbg.h>
#include <napi/teb.h>
#include <napi/win32.h>

#include <pseh.h>

#endif /* INCLUDE_NTOSKRNL_H */
