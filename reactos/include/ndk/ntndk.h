/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ntndk.h

Abstract:

    Master include file for the Native Development Kit.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _NTNDK_
#define _NTNDK_

//
// Disable some warnings that we'd get on /W4.
// Only active for compilers which support this feature.
//
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4115)
#pragma warning(disable:4214)
#endif

//
// Headers needed for NDK
//
#include <stdio.h>          // C Standard Header
#include <excpt.h>          // C Standard Header
#include <stdarg.h>         // C Standard Header
#include <umtypes.h>        // General Definitions
#include <intrin.h>         // Use Inlined Intrinsics

//
// Type Headers
//
#include <cctypes.h>        // Cache Manager Types
#include <cmtypes.h>        // Configuration Manager Types
#include <dbgktypes.h>      // User-Mode Kernel Debugging Types
#include <extypes.h>        // Executive Types
#include <kdtypes.h>        // Kernel Debugger Types
#include <ketypes.h>        // Kernel Types
#include <haltypes.h>       // Hardware Abstraction Layer Types
#include <ifssupp.h>        // IFS Support Header
#include <iotypes.h>        // Input/Output Manager Types
#include <ldrtypes.h>       // Loader Types
#include <lpctypes.h>       // Local Procedure Call Types
#include <mmtypes.h>        // Memory Manager Types
#include <obtypes.h>        // Object Manager Types
#include <potypes.h>        // Power Manager Types
#include <pstypes.h>        // Process Manager Types
#include <rtltypes.h>       // Runtime Library Types
#include <setypes.h>        // Security Subsystem Types

//
// Function Headers
//
#include <cmfuncs.h>        // Configuration Manager Functions
#include <dbgkfuncs.h>      // User-Mode Kernel Debugging Functions
#include <kdfuncs.h>        // Kernel Debugger Functions
#include <kefuncs.h>        // Kernel Functions
#include <exfuncs.h>        // Executive Functions
#include <halfuncs.h>       // Hardware Abstraction Layer Functions
#include <iofuncs.h>        // Input/Output Manager Functions
#include <inbvfuncs.h>      // Initialization Boot Video Functions
#include <ldrfuncs.h>       // Loader Functions
#include <lpcfuncs.h>       // Local Procedure Call Functions
#include <mmfuncs.h>        // Memory Manager Functions
#include <obfuncs.h>        // Object Manager Functions
#include <pofuncs.h>        // Power Manager Functions
#include <psfuncs.h>        // Process Manager Functions
#include <rtlfuncs.h>       // Runtime Library Functions
#include <sefuncs.h>        // Security Subsystem Functions
#include <umfuncs.h>        // User-Mode NT Library Functions

//
// Assembly Support
//
#include <asm.h>            // Assembly Offsets

#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif // _NTNDK_
