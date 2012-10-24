/* $Id: win32csr.h 55699 2012-02-19 06:44:09Z ion $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/win32csr.h
 * PURPOSE:         Interface to win32csr.dll
 */

#pragma once

/* PSDK/NDK Headers */
#define WIN32_NO_STATUS
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include <psapi.h>

/* External Winlogon Header */
#include <winlogon.h>

/* CSRSS Header */
#include <csr/csrsrv.h>

/* Internal CSRSS Headers */
#include <win/winmsg.h>
#include <desktopbg.h>

/* Public Win32K Headers */
#include <ntuser.h>

#include <commctrl.h>

extern HANDLE Win32CsrApiHeap;
extern HINSTANCE Win32CsrDllHandle;

/* desktopbg.c */
CSR_API(CsrCreateDesktop);
CSR_API(CsrShowDesktop);
CSR_API(CsrHideDesktop);

/* EOF */
