#include "stdarg.h"

#undef i386
#undef _X86_
#undef MIPS
#undef _MIPS_
#undef ALPHA
#undef _ALPHA_
#undef ALPHA64
#undef _ALPHA64_
#undef AXP64
#undef _AXP64_
#undef PPC
#undef _PPC_
#undef _IA64_
#undef IA64


//
// _X86_ causes:
//      nt.h        include "nti386.h"
//      nti386.h    an x86 typedef for _DBGKD_CONTROL_REPORT et al
//      nti386.h    the 386 CONTEXT record and friends
//
// _ALPHA_ causes:
//      ntdef.h     UNALIGNED to be defined as __unaligned
//      nt.h        include "ntalpha.h"
//      ntalpha.h   an alpha typedef for _DBGKD_CONTROL_REPORT et al
//      ntalpha.h   the alpha CONTEXT record and friends
//
// _IA64_ causes:
//      ntdef.h     UNALIGNED to be defined as __unaligned
//      nt.h        include "ntia64.h"
//      ntia64.h    an IA64 typedef for _DBGKD_CONTROL_REPORT et al
//      ntia64.h    the IA64 CONTEXT record and friends//
//




#if defined(TARGET_i386)

#define _X86_ 1

#define _CONTEXT _I386_CONTEXT
#define CONTEXT I386_CONTEXT
#define PCONTEXT PI386_CONTEXT
#define LPCONTEXT LPI386_CONTEXT

#include <windows.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <windbgkd.h>
#include <dbghelp.h>

#if defined(HOST_ALPHA)
#undef _cdecl
#define _cdecl
#endif






#elif defined(TARGET_ALPHA) || defined(TARGET_AXP64)

#define _ALPHA_ 1
#define _AXP64_ 1

#if defined(HOST_i386)
#define __unaligned
#endif

#define _CONTEXT _ALPHA_CONTEXT
#define CONTEXT ALPHA_CONTEXT
#define PCONTEXT PALPHA_CONTEXT
#define LPCONTEXT LPALPHA_CONTEXT

#include <windows.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <windbgkd.h>
#include <dbghelp.h>

#if defined(HOST_i386)
#undef UNALIGNED
#define UNALIGNED
#endif

#if !defined(HOST_ALPHA)
#undef _ALPHA_
#endif

#elif defined(TARGET_IA64)

#define _IA64_ 1

#if defined(HOST_i386)
#define __unaligned
#endif

#include <windows.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <windbgkd.h>
#include <dbghelp.h>
#include "kxia64.h"

#if defined(HOST_i386)
#undef UNALIGNED
#define UNALIGNED
#endif

#if !defined(HOST_ALPHA)
#undef _ALPHA_
#endif

#else

#error "Unsupported target CPU"

#endif
