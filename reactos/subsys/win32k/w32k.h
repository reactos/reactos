/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Graphics Subsystem
 * FILE:            subsys/win32k/w32k.h
 * PURPOSE:         Main Win32K Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* We are Win32K */
#define __WIN32K__

/* DDK/NDK/SDK Headers */
#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/winddi.h>
#include <ddk/ntddmou.h>
#include <windows.h>
#include <ndk/ntndk.h>

/* SEH Support with PSEH */
#include <pseh/pseh.h>

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Header */
#include <reactos/helper.h>

/* External Win32K Header */
#include <win32k/win32k.h>
#include <win32k/win32.h>

/* Internal Win32K Header */
#include "include/win32k.h"
