/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

#ifndef RTL_H
#define RTL_H

/* We're a core NT DLL, we don't import syscalls */
#define WIN32_NO_STATUS
#define _INC_SWPRINTF_INL_
#undef __MSVCRT__

/* C Headers */
#include <stdlib.h>
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#include <ndk/ntndk.h>

/* Internal RTL header */
#include "rtlp.h"

/* PSEH Support */
#include <reactos/helper.h>
#include <pseh/pseh.h>

#ifndef _MSC_VER
#include <intrin.h>
#endif

#endif /* RTL_H */

/* EOF */
