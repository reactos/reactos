#ifndef __INTERNAL_NTOSKRNL_H
#define __INTERNAL_NTOSKRNL_H

#define __MINGW_IMPORT extern

#ifndef AS_INVOKED
#include <roscfg.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <wchar.h>
#define NTOS_KERNEL_MODE
#include <ntos.h>
#include <ddk/ntpoapi.h>
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#endif /* !AS_INVOKED */

#include <internal/ntoskrnl.h>
#include <internal/dbg.h>
#include <internal/id.h>
#include <internal/kd.h>
#include <internal/arch/ke.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/nt.h>
#include <internal/cc.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ex.h>
#include <internal/se.h>
#include <internal/ldr.h>
#include <internal/ifs.h>
#include <internal/trap.h>
#include <internal/v86m.h>
#include <internal/xhal.h>
#include <internal/port.h>
#include <internal/pool.h>
#include <internal/safe.h>
#include <internal/module.h>
#include <internal/handle.h>
#include <internal/registry.h>
#include <internal/i386/fpu.h>
#include <internal/i386/mm.h>
#include <internal/i386/segment.h>

/* Conflicts with a member name */
#undef DeviceCapabilities

#endif /* __INTERNAL_NTOSKRNL_H */
