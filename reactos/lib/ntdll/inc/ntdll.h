/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/ntdll/inc/ntdll.h
 * PURPOSE:         Native Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* SDK/DDK/NDK Headers. */
#include <windows.h>
#define NTOS_MODE_USER
#define READY_FOR_NEW_NTDLL
#include <ndk/ntndk.h>

/* Internal NTDLL */
#include "ntdllp.h"

/* CSRSS Header */
#include <csrss/csrss.h>

/* Helper Macros */
#include <reactos/helper.h>

/* EOF */
