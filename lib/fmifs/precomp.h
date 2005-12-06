/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/fmifs/precomp.h
 * PURPOSE:         Win32 FMIFS API Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define UNICODE
#define _UNICODE

/* PSDK/NDK Headers */
#include <windows.h>
#include <ndk/ntndk.h>

/* FMIFS Public Header */
#include <fmifs/fmifs.h>

/* VFATLIB Public Header */
#include <fslib/vfatlib.h>

/* EOF */
