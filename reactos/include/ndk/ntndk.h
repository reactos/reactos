/* $Id: ntndk.h,v 1.1.2.2 2004/10/25 02:57:20 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ntndk.h
 * PURPOSE:         Main Native Development Kit Header file to include all others.
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _NT_NDK_
#define _NT_NDK_

#ifndef NTOS_MODE_USER
    /* Kernel-Mode NDK */
    #include "kdtypes.h"    /* Kernel Debugger Types */
    #include "kdfuncs.h"    /* Kernel Debugger Functions */
    #include "cctypes.h"    /* Cache Manager Types */
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
    #include "rostypes.h"   /* Reactos-Specific Types */
    #include "rosfuncs.h"   /* Reactos-Specific Functions */
    #include "setypes.h"    /* Security Subsystem Types */
    #include "sefuncs.h"    /* Security Subsystem Functions */
#else
    /* User-Mode NDK */
    #include "umtypes.h"    /* Native Types in DDK/IFS but not in PSDK */
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
