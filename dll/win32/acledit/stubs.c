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
#include "acleditint.h"

ULONG DbgPrint(PCH Format,...);

#define UNIMPLEMENTED \
  DbgPrint("ACLEDIT:  %s at %s:%d is UNIMPLEMENTED!\n",__FUNCTION__,__FILE__,__LINE__)


DWORD
WINAPI
EditAuditInfo(DWORD Unknown)
{
  UNREFERENCED_PARAMETER(Unknown);
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
EditOwnerInfo(DWORD Unknown)
{
  UNREFERENCED_PARAMETER(Unknown);
  UNIMPLEMENTED;
  return 0;
}

DWORD
WINAPI
EditPermissionInfo(DWORD Unknown)
{
  UNREFERENCED_PARAMETER(Unknown);
  UNIMPLEMENTED;
  return 0;
}

LONG
WINAPI
FMExtensionProcW(HWND hWnd,
                 WORD wEvent,
                 LONG lParam)
{
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(wEvent);
  UNREFERENCED_PARAMETER(lParam);
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
  UNREFERENCED_PARAMETER(hWndOwner);
  UNREFERENCED_PARAMETER(hInstance);	
  UNREFERENCED_PARAMETER(lpMachineName);
  UNREFERENCED_PARAMETER(pAclDlgControl);
  UNREFERENCED_PARAMETER(pAclEditControl);
  UNREFERENCED_PARAMETER(lpObjectName);
  UNREFERENCED_PARAMETER(pChangeCallback);
  UNREFERENCED_PARAMETER(pChangeCallbackContext);
  UNREFERENCED_PARAMETER(pObjectSecurity);
  UNREFERENCED_PARAMETER(bNoReadPermission);
  UNREFERENCED_PARAMETER(bReadOnly);
  UNREFERENCED_PARAMETER(lpdwChangeContextStatus);
  UNREFERENCED_PARAMETER(Reserved);
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
                 BOOL bNoReadPermission,
                 BOOL bNoOwnerChange,
                 LPDWORD lpdwChangeContextStatus,
                 PACL_HELP_CONTROL pHelpInfo,
                 DWORD Reserved)
{
  UNREFERENCED_PARAMETER(hWndOwner);
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(lpMachineName);
  UNREFERENCED_PARAMETER(lpObjectType);
  UNREFERENCED_PARAMETER(lpObjectName);
  UNREFERENCED_PARAMETER(dwObjectCount);
  UNREFERENCED_PARAMETER(pChangeCallback);
  UNREFERENCED_PARAMETER(pChangeCallbackContext);
  UNREFERENCED_PARAMETER(pObjectSecurity);
  UNREFERENCED_PARAMETER(bNoReadPermission);
  UNREFERENCED_PARAMETER(bNoOwnerChange);
  UNREFERENCED_PARAMETER(lpdwChangeContextStatus);
  UNREFERENCED_PARAMETER(pHelpInfo);
  UNREFERENCED_PARAMETER(Reserved);
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
  UNREFERENCED_PARAMETER(hWndOwner);
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(lpMachineName);
  UNREFERENCED_PARAMETER(pAclDlgControl);
  UNREFERENCED_PARAMETER(pAclEditControl);
  UNREFERENCED_PARAMETER(lpObjectName);
  UNREFERENCED_PARAMETER(pChangeCallback);
  UNREFERENCED_PARAMETER(pChangeCallbackContext);
  UNREFERENCED_PARAMETER(pObjectSecurity);
  UNREFERENCED_PARAMETER(bNoReadPermission);
  UNREFERENCED_PARAMETER(lpdwChangeContextStatus);
  UNREFERENCED_PARAMETER(Reserved);
  UNIMPLEMENTED;
  return 0;
}

/* EOF */
