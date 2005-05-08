#ifndef __INCLUDE_NTOSKRNL_H
#define __INCLUDE_NTOSKRNL_H

#define __NO_CTYPE_INLINES

/* include the ntoskrnl config.h file */
#include "config.h"

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
#include <pseh.h>
#include <internal/ctype.h>
#include <internal/id.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/i386/fpu.h>
#include <internal/i386/tss.h>
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
#ifdef KDBG
#include <internal/kdb.h>
#endif
#include <internal/dbgk.h>
#include <internal/trap.h>
#include <internal/safe.h>
#include <internal/test.h>

#endif /* INCLUDE_NTOSKRNL_H */


#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(__SOURCE_STRING__) \
{ \
 sizeof(__SOURCE_STRING__) - sizeof((__SOURCE_STRING__)[0]), \
 sizeof(__SOURCE_STRING__), \
 (__SOURCE_STRING__) \
}
#endif

#ifdef DBG
#ifndef PAGED_CODE
#define PAGED_CODE()                                                           \
  do {                                                                         \
    if(KeGetCurrentIrql() > APC_LEVEL) {                                       \
      DbgPrint("%s:%i: Pagable code called at IRQL > APC_LEVEL (%d)\n",        \
               __FILE__, __LINE__, KeGetCurrentIrql());                        \
      KEBUGCHECK(0);                                                           \
    }                                                                          \
  } while(0)
#endif
#define PAGED_CODE_RTL PAGED_CODE
#else
#ifndef PAGED_CODE
#define PAGED_CODE()
#endif
#define PAGED_CODE_RTL()
#endif

#endif /* INCLUDE_NTOSKRNL_H */
