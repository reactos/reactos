#ifndef __INCLUDE_NTOSKRNL_H
#define __INCLUDE_NTOSKRNL_H

#define __NO_CTYPE_INLINES


#include <reactos/roscfg.h>
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/defines.h>
#include <reactos/string.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <ddk/ntifs.h>
#include <ndk/ntndk.h>

/* Leave Intact */
#include <internal/ctype.h>
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
#include <internal/nt.h>
#include "internal/xhal.h"
#include <internal/v86m.h>
#include <internal/ifs.h>
#include <internal/port.h>
#include <internal/nls.h>
#include <internal/dbg.h>
#include <internal/trap.h>
#include <internal/safe.h>

#endif /* INCLUDE_NTOSKRNL_H */
