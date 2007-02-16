/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS System Libraries
 * FILE:            lib/dnsapi/precomp.h
 * PURPOSE:         Win32 DNS API Libary Header
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#define NTOS_MODE_USER

/* PSDK/NDK Headers */
#include <windows.h>
#include <winerror.h>
#include <windns.h>
#include <ndk/ntndk.h>

/* Internal DNSAPI Headers */
#include <internal/windns.h>

