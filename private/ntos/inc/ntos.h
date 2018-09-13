/*++ BUILD Version: 0006    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntos.h

Abstract:

    Top level include file for the NTOS component.

Author:

    Steve Wood (stevewo) 28-Feb-1989


Revision History:

--*/

#ifndef _NTOS_
#define _NTOS_

#include <nt.h>
#include <ntrtl.h>
#include "ntosdef.h"
#include "exlevels.h"
#include "exboosts.h"
#include "bugcodes.h"
#include "init.h"
#include "v86emul.h"

#ifdef _X86_
#include "i386.h"
#endif

#ifdef _MIPS_
#include "mips.h"
#endif

#ifdef _ALPHA_
#include "alpha.h"
#endif

#ifdef _PPC_
#include "ppc.h"
#endif

#ifdef _IA64_
#include "ia64.h"
#endif // _IA64_

#include "arc.h"
#include "ke.h"
#include "kd.h"
#include "ex.h"
#include "ps.h"
#include "se.h"
#include "io.h"
#include "ob.h"
#include "mm.h"
#include "lpc.h"
#include "dbgk.h"
#include "lfs.h"
#include "cache.h"
#include "pnp.h"
#include "hal.h"
#include "cm.h"
#include "po.h"

#define _NTDDK_

//
// Temp. Until we define a header file for types
// Outside of the kernel these are exported by reference
//

#ifdef _NTDRIVER_
extern POBJECT_TYPE *ExEventPairObjectType;
extern POBJECT_TYPE *PsProcessType;
extern POBJECT_TYPE *PsThreadType;
extern POBJECT_TYPE *PsJobType;
extern POBJECT_TYPE *LpcPortObjectType;
extern POBJECT_TYPE *LpcWaitablePortObjectType;
#else
extern POBJECT_TYPE ExEventPairObjectType;
extern POBJECT_TYPE PsProcessType;
extern POBJECT_TYPE PsThreadType;
extern POBJECT_TYPE PsJobType;
extern POBJECT_TYPE LpcPortObjectType;
extern POBJECT_TYPE LpcWaitablePortObjectType;
#endif // _NTDRIVER

#endif // _NTOS_
