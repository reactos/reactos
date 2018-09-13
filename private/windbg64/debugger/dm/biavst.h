/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    biavst.h

Abstract:

    This header file is used to cause the correct machine/platform specific
    data structures to be used when compiling for a non-hosted platform.

Author:

    Wesley Witt (wesw) 2-Aug-1993

Environment:

    User Mode

--*/

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
#undef IA64
#undef _IA64_

#define _NTSYSTEM_

//-----------------------------------------------------------------------------------------
//
// mips r4000
//
//-----------------------------------------------------------------------------------------
#if defined(TARGET_MIPS)

#pragma message( "Compiling for target = mips" )

#define _MIPS_

#if defined(HOST_MIPS)
#define MIPS
#endif

#if defined(HOST_i386)
#define __unaligned
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <ntos.h>
#include <iop.h>
#include <windows.h>

#if !defined(HOST_MIPS)
#undef MIPS
#undef _MIPS_
#endif

#if defined(HOST_i386)
#undef _cdecl
#undef UNALIGNED
#define UNALIGNED
#endif

//-----------------------------------------------------------------------------------------
//
// PowerPC
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_PPC)

#pragma message( "Compiling for target = ppc" )

#define _PPC_

#if defined(HOST_PPC)
#define PPC
#endif

#if defined(HOST_i386)
#define __unaligned
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <ntos.h>
#include <iop.h>
#include <windows.h>

#if !defined(HOST_PPC)
#undef PPC
#undef _PPC_
#endif

#if defined(HOST_i386)
#undef _cdecl
#undef UNALIGNED
#define UNALIGNED
#endif

//-----------------------------------------------------------------------------------------
//
// intel x86
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_i386)

#pragma message( "Compiling for target = x86" )

#define _X86_

#if defined(HOST_MIPS)
#define MIPS
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <ntos.h>
#include <iop.h>
#include <windows.h>

#if defined(HOST_MIPS)
#undef _cdecl
#define _cdecl
#endif

#if defined(HOST_ALPHA) || defined(HOST_AXP64)
#undef _cdecl
#define _cdecl
#endif

#if !defined(HOST_i386)
#undef _X86_
#endif

//-----------------------------------------------------------------------------------------
//
// alpha axp
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_ALPHA) || defined(TARGET_AXP64)

#pragma message( "Compiling for target = alpha" )

#define _ALPHA_ 1

#if defined(HOST_i386)
#define __unaligned
#endif

#if defined(HOST_MIPS)
#define MIPS
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <ntos.h>
#include <iop.h>
#include <windows.h>

#if defined(HOST_MIPS)
#undef _cdecl
#define _cdecl
#endif

#if defined(HOST_i386)
#undef UNALIGNED
#define UNALIGNED
#endif

#if !defined(HOST_ALPHA) && !defined(HOST_AXP64)
#undef _ALPHA_
#endif


//-----------------------------------------------------------------------------------------
//
// Intel IA64
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_IA64)

#pragma message( "Compiling for target = ia64" )

#define _IA64_ 1

#if defined(HOST_i386)
#define __unaligned
#endif

#if defined(HOST_MIPS)
#define MIPS
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>
#include <ntos.h>
#include <iop.h>
#include <windows.h>

#if defined(HOST_MIPS)
#undef _cdecl
#define _cdecl
#endif

#if defined(HOST_i386)
#undef UNALIGNED
#define UNALIGNED
#endif

#if !defined(HOST_IA64)
#undef _IA64_
#endif


#else

//-----------------------------------------------------------------------------------------
//
// unknown platform
//
//-----------------------------------------------------------------------------------------
#error "Unsupported target CPU"

#endif
