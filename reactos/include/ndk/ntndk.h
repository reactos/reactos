/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ntndk.h
 * PURPOSE:         Main Native Development Kit Header file to include all others.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _NTNDK_
#define _NTNDK_

#ifndef NTOS_MODE_USER
    /* Kernel-Mode NDK */
    #include "kdtypes.h"    /* Kernel Debugger Types */
    #include "kdfuncs.h"    /* Kernel Debugger Functions */
    #include "cctypes.h"    /* Cache Manager Types */
    #include "dbgktypes.h"  /* User-Mode Kernel Debugging Types */
    #include "extypes.h"    /* Executive Types */
    #include "haltypes.h"   /* Hardware Abstraction Layer Types */
    #include "halfuncs.h"   /* Hardware Abstraction Layer Functions */
    #include "inbvfuncs.h"  /* Initialization Boot Video Functions */
    #include "iotypes.h"    /* Input/Output Manager Types */
    #include "iofuncs.h"    /* Input/Output Manager Functions */
    #include "ketypes.h"    /* Kernel Types */
    #include "kefuncs.h"    /* Kernel Functions */
    #include "ldrfuncs.h"   /* Loader Functions */
    #include "lpctypes.h"   /* Local Procedure Call Types */
    #include "mmtypes.h"    /* Memory Manager Types */
    #include "obtypes.h"    /* Object Manager Types */
    #include "obfuncs.h"    /* Object Manager Functions */
    #include "potypes.h"    /* Power Manager Types */
    #include "psfuncs.h"    /* Process Manager Functions */
    #include "setypes.h"    /* Security Subsystem Types */
    #include "sefuncs.h"    /* Security Subsystem Functions */
#else
    /* User-Mode NDK */
    #include "umtypes.h"    /* Native Types in DDK/IFS but not in PSDK */
    #include "umfuncs.h"    /* User-Mode NT Library Functions */
#endif

/* Shared NDK */
#include "ldrtypes.h"       /* Loader Types */
#include "pstypes.h"        /* Process Manager Types */
#include "rtltypes.h"       /* Runtime Library Types */
#include "rtlfuncs.h"       /* Runtime Library Functions */
#include "zwtypes.h"        /* Native Types */
#include "zwfuncs.h"        /* Native Functions (System Calls) */
#include "i386/floatsave.h" /* Floating Point Save Area Definitions for i386 */
#include "i386/segment.h"   /* Kernel CPU Segment Definitions for i386 */

#endif 
