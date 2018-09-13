/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walk.h

Abstract:

    This header file is used to cause the correct machine/platform specific
    data structures to be used when compiling for a non-hosted platform.

Author:

    Wesley Witt (wesw) 21-Aug-1993

Environment:

    User Mode

--*/


#undef i386
#undef _X86_
#undef ALPHA
#undef _ALPHA_
#undef IA64
#undef _IA64_

//-----------------------------------------------------------------------------------------
//
// intel x86
//
//-----------------------------------------------------------------------------------------
#if defined(TARGET_i386)

#pragma message( "Compiling for target = x86" )

#define _X86_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <windows.h>
#include <imagehlp.h>

#if defined(_M_ALPHA)
#undef _cdecl
#define _cdecl
#endif

#if !defined(_M_IX86)
#undef _X86_
#endif

//-----------------------------------------------------------------------------------------
//
// alpha axp
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_ALPHA)

#pragma message( "Compiling for target = alpha" )

#define _ALPHA_ 1

#if defined(_M_IX86)
#define __unaligned
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <windows.h>
#include <imagehlp.h>

#if defined(_M_IX86)
#undef UNALIGNED
#define UNALIGNED
#endif

#if !defined(_M_ALPHA)
#undef _ALPHA_
#endif
//-----------------------------------------------------------------------------------------
//
// ia64
//
//-----------------------------------------------------------------------------------------
#elif defined(TARGET_IA64)

#pragma message( "Compiling for target = ia64" )

#define _IA64_

#if defined(_M_IA64)
#define IA64
#endif

#if defined(_M_IX86)
#define __unaligned
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <ia64inst.h>
#include <windows.h>
#include <imagehlp.h>

#if !defined(_M_IA64)
#undef IA64
#undef _IA64_
#endif

#if defined(_M_IX86)
#undef _cdecl
#undef UNALIGNED
#define UNALIGNED
#endif
#else

//-----------------------------------------------------------------------------------------
//
// unknown platform
//
//-----------------------------------------------------------------------------------------
#error "Unsupported target CPU"

#endif
