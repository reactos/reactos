#ifndef __INCLUDE_NTOSKRNL_H
#define __INCLUDE_NTOSKRNL_H

#define NTKERNELAPI

/* include the ntoskrnl config.h file */
#include "config.h"

#include <roskrnl.h>
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/wdmguid.h>

#undef IO_TYPE_FILE
#define IO_TYPE_FILE                    0x0F5L /* Temp Hack */

#include <roscfg.h>
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/rossym.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <wchar.h>
#include <ntos/minmax.h>
#include <ntos/synch.h>
#include <ntos/keyboard.h>
#include <ntos/ntdef.h>
#include <ntos/ldrtypes.h>
#include <ntos/ntpnp.h>
#include <ddk/ldrfuncs.h>
#include <rosrtl/minmax.h>
#include <rosrtl/string.h>
#include <ntdll/ldr.h>
#include <pseh.h>
#include <internal/ctype.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/i386/fpu.h>
#include <internal/module.h>
#include <internal/handle.h>
#include <internal/pool.h>
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/cc.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/se.h>
#include <internal/ldr.h>
#include <internal/kd.h>
#include <internal/ex.h>
#include "internal/xhal.h"
#include <internal/v86m.h>
#include <internal/ifs.h>
#include <internal/port.h>
#include <internal/nls.h>
#ifdef KDBG
#include <internal/kdb.h>
#endif
#include <internal/dbgk.h>
#include <internal/trap.h>
#include <internal/safe.h>
#include <internal/tag.h>
#include <internal/test.h>
#include <internal/inbv.h>
#include <napi/core.h>
#include <napi/dbg.h>
#include <napi/teb.h>
#include <napi/win32.h>

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#endif /* INCLUDE_NTOSKRNL_H */
