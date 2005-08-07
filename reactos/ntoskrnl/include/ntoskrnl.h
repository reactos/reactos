/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/include/ntoskrnl.h
 * PURPOSE:         Main Kernel Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* We are the Kernel */
#define NTKERNELAPI

/* DDK/IFS/NDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/wdmguid.h>
#include <ndk/ntndk.h>
#include <ndk/sysguid.h>
#include <ndk/asm.h>

/* FIXME: Temporary until CC Ros is gone */
#include <ccros.h>        

/* ReactOS Headers */
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/rossym.h>

/* Disk Dump Driver Header */
#include <diskdump/diskdump.h>

/* C Headers */
#include <stdio.h>
#include <ctype.h>
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh/pseh.h>

/* Helper Header */
#include <reactos/helper.h>

/* Internal Headers */
#include "internal/ntoskrnl.h"
#include "config.h"
