#ifndef __ACLEDITINT_H
#define __ACLEDITINT_H

extern HINSTANCE hDllInstance;

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

#endif /* __ACLEDITINT_H */

/* EOF */
