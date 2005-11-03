/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/rtl/rtl.h
 * PURPOSE:         Run-Time Libary Header
 * PROGRAMMER:      Alex Ionescu
 */

/* INCLUDES ******************************************************************/

/* We're a core NT DLL, we don't import syscalls */
#define _NTSYSTEM_
#define _NTDLLBUILD_

/* C Headers */
#include <stdio.h>

/* PSDK/NDK Headers */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* Helper Header */
#include <reactos/helper.h>

/* Internal RTL header */
#include "rtlp.h"

/* EOF */
