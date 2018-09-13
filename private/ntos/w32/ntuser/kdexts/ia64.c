/****************************** Module Header ******************************\
* Module Name: i386.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains processor specific routines for x86.
*
* History:
* 25-Oct-1995 JimA      Created.
\***************************************************************************/

#undef _X86_
#undef _MIPS_
#undef _ALPHA_
#undef _PPC_
#undef _IA64_

#define _IA64_

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_PPC_) || defined(_X86_)
#error More than one architecture defined!
#endif

#define GetEProcessData GetEProcessData_IA64

#include <process.h>

