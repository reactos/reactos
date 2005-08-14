/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/kernel32/k32.h
 * PURPOSE:         Win32 Kernel Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* C Headers */
#include <wchar.h>

/* DDK Driver Headers */
#include <ddk/ntddbeep.h>
#include <ddk/ntddser.h>
#include <ddk/ntddtape.h>

/* Internal Kernel32 Header */
#include "include/kernel32.h"

/* PSEH for SEH Support */
#include <pseh/pseh.h>

/* Helper Header */
#include <reactos/helper.h>

/* EOF */
