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

/* C Standard Headers */
#include <stdarg.h>
#include <excpt.h>

/* Helper Header */
#include <helper.h>

/* A version of ntdef.h to be used with PSDK headers. */
#include "umtypes.h"

/* Kernel-Mode NDK */
#ifndef NTOS_MODE_USER
#include "ifssupp.h"        /* IFS Support Header */
#include "kdfuncs.h"        /* Kernel Debugger Functions */
#include "cctypes.h"        /* Cache Manager Types */
#include "potypes.h"        /* Power Manager Types */
#include "dbgktypes.h"      /* User-Mode Kernel Debugging Types */
#include "haltypes.h"       /* Hardware Abstraction Layer Types */
#include "halfuncs.h"       /* Hardware Abstraction Layer Functions */
#include "inbvfuncs.h"      /* Initialization Boot Video Functions */
#include "iofuncs.h"        /* Input/Output Manager Functions */
#include "kefuncs.h"        /* Kernel Functions */
#include "mmfuncs.h"        /* Memory Manager Functions */
#include "obfuncs.h"        /* Object Manager Functions */
#include "psfuncs.h"        /* Process Manager Functions */
#include "setypes.h"        /* Security Subsystem Types */
#include "sefuncs.h"        /* Security Subsystem Functions */
#endif /* !NTOS_MODE_USER */

/* Shared NDK */
#include "extypes.h"        /* Executive Types */
#include "cmtypes.h"        /* Configuration Manager Types */
#include "kdtypes.h"        /* Kernel Debugger Types */
#include "ketypes.h"        /* Kernel Types */
#include "iotypes.h"        /* Input/Output Manager Types */
#include "ldrtypes.h"       /* Loader Types */
#include "ldrfuncs.h"       /* Loader Functions */
#include "mmtypes.h"        /* Memory Manager Types */
#include "obtypes.h"        /* Object Manager Types */
#include "pstypes.h"        /* Process Manager Types */
#include "lpctypes.h"       /* Local Procedure Call Types */
#include "zwtypes.h"        /* Native Types */
#include "zwfuncs.h"        /* Native Functions (System Calls) */
#include "rtltypes.h"       /* Runtime Library Types */
#include "rtlfuncs.h"       /* Runtime Library Functions */
#include "umfuncs.h"        /* User-Mode NT Library Functions */
#include "i386/floatsave.h" /* Floating Point Save Area Definitions for i386 */
#include "i386/segment.h"   /* Kernel CPU Segment Definitions for i386 */

#endif 
