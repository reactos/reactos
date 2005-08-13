/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Access Control List Editor
 * FILE:            lib/acledit/stubs.c
 * PURPOSE:         acledit.dll stubs
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * NOTES:           If you implement a function, remove it from this file
 *
 *                  Based on guess work and on this nice article:
 *                    http://www.sysinternals.com/ntw2k/info/acledit.shtml
 *
 * UPDATE HISTORY:
 *      07/09/2004  Created
 */
#include <windows.h>
#include <acledit.h>
#include "acleditint.h"

ULONG DbgPrint(PCH Format,...);

#define UNIMPLEMENTED \
  DbgPrint("ACLEDIT:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


DWORD
WINAPI
EditAuditInfo(DWORD Unknown)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
EditOwnerInfo(DWORD Unknown)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
EditPermissionInfo(DWORD Unknown)
{
  UNIMPLEMENTED;
  return 0;
}

LONG
WINAPI
FMExtensionProcW(HWND hWnd,
                 WORD wEvent,
		 LONG lParam)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
SedDiscretionaryAclEditor(HWND hWndOwner,
                          HINSTANCE hInstance,
                          LPCWSTR lpMachineName,
                          PACL_DLG_CONTROL pAclDlgControl,
                          PACL_EDIT_CONTROL pAclEditControl,
                          LPCWSTR lpObjectName,
                          PACL_CHANGE pChangeCallback,
                          PVOID pChangeCallbackContext,
                          PSECURITY_DESCRIPTOR pObjectSecurity,
                          BOOL bNoReadPermission,
                          BOOL bReadOnly,
                          LPDWORD lpdwChangeContextStatus,
                          DWORD Reserved)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
SedTakeOwnership(HWND hWndOwner,
                 HINSTANCE hInstance,
                 LPCWSTR lpMachineName,
                 LPCWSTR lpObjectType,
                 LPCWSTR lpObjectName,
                 DWORD dwObjectCount,
                 PACL_CHANGE pChangeCallback,
                 PVOID pChangeCallbackContext,
                 PSECURITY_DESCRIPTOR pObjectSecurity,
                 BOOL NoReadPermission,
                 BOOL NoOwnerChange,
                 LPDWORD lpdwChangeContextStatus,
                 PACL_HELP_CONTROL pHelpInfo,
                 DWORD Reserved)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
SedSystemAclEditor(HWND hWndOwner,
                   HINSTANCE hInstance,
                   LPCWSTR lpMachineName,
                   PACL_DLG_CONTROL pAclDlgControl,
                   PACL_EDIT_CONTROL pAclEditControl,
                   LPCWSTR lpObjectName,
                   PACL_CHANGE pChangeCallback,
                   PVOID pChangeCallbackContext,
                   PSECURITY_DESCRIPTOR pObjectSecurity,
                   BOOL bNoReadPermission,
                   LPDWORD lpdwChangeContextStatus,
                   DWORD Reserved)
{
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
