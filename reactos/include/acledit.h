/*
 * acledit.h
 *
 * Access Control List Editor definitions
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __ACLEDIT_H
#define __ACLEDIT_H

#include <windows.h>

typedef struct _ACL_HELP_CONTROL
{
  LPWSTR lpHelpFile;
  DWORD dwMainDialogTopic;
  DWORD dwACLEditorDialogTopic;
  DWORD Reserved1;
  DWORD dwAddEntryDialogTopic;
  DWORD Reserved2;
  DWORD Reserved3;
  DWORD dwAccountDialogTopic;
} ACL_HELP_CONTROL, *PACL_HELP_CONTROL;

typedef struct _ACL_DLG_CONTROL
{
  UCHAR ucVersion;
  BOOL bIsContainer;
  BOOL bAllowNewObject;
  BOOL bMapSpecificToGeneric;
  LPDWORD lpdwGenericAccessMap;
  LPDWORD lpdwGenericMappingNewObjects;
  LPWSTR lpDialogTitle;
  PACL_HELP_CONTROL pHelpInfo;
  LPWSTR lpSubReplaceTitle;
  LPWSTR lpSubReplaceObjectsTitle;
  LPWSTR lpSubReplaceConfirmation;
  LPWSTR lpSpecialAccess;
  LPWSTR lpSpecialNewAccess;
} ACL_DLG_CONTROL, *PACL_DLG_CONTROL;

typedef struct _ACL_EDIT_ENTRY{
  DWORD dwType;
  DWORD dwAccessMask;
  DWORD dwAccessMask1;
  LPWSTR lpName;
} ACL_EDIT_ENTRY, *PACL_EDIT_ENTRY;

typedef struct _ACL_EDIT_CONTROL
{
  DWORD dwNumberOfEntries;
  PACL_EDIT_ENTRY pEntries;
  LPWSTR lpDefaultPermissionName;
} ACL_EDIT_CONTROL, *PACL_EDIT_CONTROL;

typedef DWORD (CALLBACK *PACL_CHANGE)(HWND hWnd, 
                                      HINSTANCE hInstance, 
                                      PVOID pCallbackContext, 
                                      PSECURITY_DESCRIPTOR pNewSD,
                                      PSECURITY_DESCRIPTOR pNewObjectSD,
                                      BOOL bApplyToSubContainers,
                                      BOOL bApplyToSubObjects,
                                      LPDWORD lpdwChangeContextStatus);

DWORD WINAPI
EditAuditInfo(DWORD Unknown);

DWORD WINAPI
EditOwnerInfo(DWORD Unknown);

DWORD WINAPI
EditPermissionInfo(DWORD Unknown);

LONG WINAPI
FMExtensionProcW(HWND hWnd,
                 WORD wEvent,
		 LONG lParam);

DWORD WINAPI
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
                          DWORD Reserved);

DWORD WINAPI
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
                 DWORD Reserved);

DWORD WINAPI
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
                   DWORD Reserved);

#endif /* __ACLEDIT_H */

/* EOF */
