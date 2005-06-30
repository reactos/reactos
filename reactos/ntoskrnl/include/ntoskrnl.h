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

/* include the ntoskrnl config.h file */
#include "config.h"

/* DDK/IFS/NDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/wdmguid.h>
#include <ddk/ntpnp.h>
#include <ndk/ntndk.h>
#undef IO_TYPE_FILE
#define IO_TYPE_FILE                    0x0F5L /* Temp Hack */

/* ReactOS Headers */
#include <reactos/version.h>
#include <reactos/resource.h>
#include <reactos/bugcodes.h>
#include <reactos/rossym.h>

/* C Headers */
#include <malloc.h>
#include <wchar.h>

/* SEH support with PSEH */
#include <pseh/pseh.h>

/* Helper Header */
#include <reactos/helper.h>

/* Internal Headers */
#include "internal/ntoskrnl.h"
