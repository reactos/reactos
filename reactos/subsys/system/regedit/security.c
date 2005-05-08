/*
 * Regedit ACL Editor for Registry Keys
 *
 * Copyright (C) 2004 Thomas Weidenmueller <w3seek@reactos.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     /* Exclude rarely-used stuff from Windows headers */
#define INITGUID
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <accctrl.h>
#include <objbase.h>
#include <basetyps.h>
#include <unknwn.h>
#include "security.h"
#include "regproc.h"
#include "resource.h"

/******************************************************************************
   Implementation of the CRegKeySecurity interface
 ******************************************************************************/

SI_ACCESS RegAccess[] = {
  {&GUID_NULL, KEY_ALL_ACCESS,         (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_FULLCONTROL),      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_READ,               (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_READ),             SI_ACCESS_GENERAL},
  {&GUID_NULL, KEY_QUERY_VALUE,        (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_QUERYVALUE),       SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_SET_VALUE,          (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_SETVALUE),         SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_CREATE_SUB_KEY,     (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_CREATESUBKEY),     SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_ENUMERATE_SUB_KEYS, (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_ENUMERATESUBKEYS), SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_NOTIFY,             (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_NOTIFY),           SI_ACCESS_SPECIFIC},
  {&GUID_NULL, KEY_CREATE_LINK,        (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_CREATELINK),       SI_ACCESS_SPECIFIC},
  {&GUID_NULL, DELETE,                 (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_DELETE),           SI_ACCESS_SPECIFIC},
  {&GUID_NULL, WRITE_DAC,              (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_WRITEDAC),         SI_ACCESS_SPECIFIC},
  {&GUID_NULL, WRITE_OWNER,            (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_WRITEOWNER),       SI_ACCESS_SPECIFIC},
  {&GUID_NULL, READ_CONTROL,           (LPWSTR)MAKEINTRESOURCE(IDS_ACCESS_READCONTROL),      SI_ACCESS_SPECIFIC},
};

DWORD RegDefaultAccess = 1; /* KEY_READ */

GENERIC_MAPPING RegAccessMasks = {
  KEY_READ,
  KEY_WRITE,
  KEY_EXECUTE,
  KEY_ALL_ACCESS
};

SI_INHERIT_TYPE RegInheritTypes[] = {
  {&GUID_NULL, 0,                                        (LPWSTR)MAKEINTRESOURCE(IDS_INHERIT_THISKEYONLY)},
  {&GUID_NULL, CONTAINER_INHERIT_ACE,                    (LPWSTR)MAKEINTRESOURCE(IDS_INHERIT_THISKEYANDSUBKEYS)},
  {&GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, (LPWSTR)MAKEINTRESOURCE(IDS_INHERIT_SUBKEYSONLY)},
};


LPREGKEYSECURITY CRegKeySecurity_fnConstructor(HANDLE Handle, SE_OBJECT_TYPE ObjectType, SI_OBJECT_INFO *ObjectInfo, BOOL *Btn)
{
  LPREGKEYSECURITY obj;

  obj = (LPREGKEYSECURITY)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(REGKEYSECURITY));
  if(obj != NULL)
  {
    obj->ref = 1;
    obj->lpVtbl = &efvt;
    obj->Handle = Handle;
    obj->ObjectType = ObjectType;
    obj->ObjectInfo = *ObjectInfo;
    obj->Btn = Btn;
  }

  return obj;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnQueryInterface(LPREGKEYSECURITY this,
                                 REFIID iid,
				 PVOID *pvObject)
{
  if(IsEqualGUID(iid, &IID_IUnknown) ||
     IsEqualGUID(iid, &IID_CRegKeySecurity))
  {
    *pvObject = this;
    return S_OK;
  }

  *pvObject = NULL;
  return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE
CRegKeySecurity_fnAddRef(LPREGKEYSECURITY this)
{
  return (ULONG)InterlockedIncrement(&this->ref);
}

ULONG STDMETHODCALLTYPE
CRegKeySecurity_fnRelease(LPREGKEYSECURITY this)
{
  ULONG rfc;

  rfc = (ULONG)InterlockedDecrement(&this->ref);
  if(rfc == 0)
  {
    HeapFree(GetProcessHeap(), 0, this);
  }
  return rfc;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnGetObjectInformation(LPREGKEYSECURITY this,
                                       PSI_OBJECT_INFO pObjectInfo)
{
  *pObjectInfo = this->ObjectInfo;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnGetSecurity(LPREGKEYSECURITY this,
                              SECURITY_INFORMATION RequestedInformation,
                              PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                              BOOL fDefault)
{
  /* FIXME */
  if(GetSecurityInfo(this->Handle, this->ObjectType, RequestedInformation, 0, 0,
                     0, 0, ppSecurityDescriptor) == ERROR_SUCCESS)
  {
    return S_OK;
  }
  else
  {
    return E_ACCESSDENIED;
  }
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnSetSecurity(LPREGKEYSECURITY this,
                              SECURITY_INFORMATION RequestedInformation,
                              PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  /* FIXME */
  *this->Btn = TRUE;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnGetAccessRights(LPREGKEYSECURITY this,
                                  const GUID* pguidObjectType,
                                  DWORD dwFlags,
                                  PSI_ACCESS* ppAccess,
                                  ULONG* pcAccesses,
                                  ULONG* piDefaultAccess)
{
  *ppAccess = RegAccess;
  *pcAccesses = sizeof(RegAccess) / sizeof(SI_ACCESS);
  *piDefaultAccess = RegDefaultAccess;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnMapGeneric(LPREGKEYSECURITY this,
                             const GUID* pguidObjectType,
                             UCHAR* pAceFlags,
                             ACCESS_MASK* pMask)
{
  MapGenericMask(pMask, (PGENERIC_MAPPING)&RegAccessMasks);
  *pMask &= ~SYNCHRONIZE;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnGetInheritTypes(LPREGKEYSECURITY this,
                                  PSI_INHERIT_TYPE* ppInheritTypes,
                                  ULONG* pcInheritTypes)
{
  /* FIXME */
  if(this->ObjectInfo.dwFlags & SI_CONTAINER)
  {
    *ppInheritTypes = RegInheritTypes;
    *pcInheritTypes = sizeof(RegInheritTypes) / sizeof(SI_INHERIT_TYPE);
    return S_OK;
  }
  return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
CRegKeySecurity_fnPropertySheetPageCallback(LPREGKEYSECURITY this,
                                            HWND hwnd,
                                            UINT uMsg,
                                            SI_PAGE_TYPE uPage)
{
  /* FIXME */
  return S_OK;
}


/******************************************************************************/

#define ACLUI_DLL	_T("aclui.dll")
#define FN_EDITSECURITY	"EditSecurity" /* no unicode, GetProcAddr doesn't accept unicode! */

typedef struct _CHANGE_CONTEXT
{
  HKEY hKey;
  LPTSTR KeyString;
} CHANGE_CONTEXT, *PCHANGE_CONTEXT;

typedef BOOL (WINAPI *PEDITSECURITY)(HWND hwndOwner,
                                     LPREGKEYSECURITY psi);

static PEDITSECURITY pfnEditSecurity;
static HMODULE hAclUiDll;

BOOL
InitializeAclUiDll(VOID)
{
  if(!(hAclUiDll = LoadLibrary(ACLUI_DLL)))
  {
    return FALSE;
  }
  if(!(pfnEditSecurity = (PEDITSECURITY)GetProcAddress(hAclUiDll, FN_EDITSECURITY)))
  {
    FreeLibrary(hAclUiDll);
    hAclUiDll = NULL;
    return FALSE;
  }

  return TRUE;
}

VOID
UnloadAclUiDll(VOID)
{
  if(hAclUiDll != NULL)
  {
    FreeLibrary(hAclUiDll);
  }
}

BOOL
RegKeyEditPermissions(HWND hWndOwner,
                      HKEY hKey,
                      LPCTSTR lpMachine,
                      LPCTSTR lpKeyName)
{
  BOOL Result;
  HMODULE hAclEditDll;
  LPWSTR Machine, KeyName;
  HKEY hInfoKey;
  LPREGKEYSECURITY RegKeySecurity;
  SI_OBJECT_INFO ObjectInfo;

  if(pfnEditSecurity == NULL)
  {
    return FALSE;
  }

#ifndef UNICODE
  /* aclui.dll only accepts unicode strings, convert them */
  if(lpMachine != NULL)
  {
    int lnMachine = lstrlen(lpMachine);
    if(!(Machine = HeapAlloc(GetProcessHeap(), 0, (lnMachine + 1) * sizeof(WCHAR))))
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    if(lnMachine > 0)
    {
      MultiByteToWideChar(CP_ACP, 0, lpMachine, -1, Machine, lnMachine + 1);
    }
    else
      *Machine = L'\0';
  }
  else
    Machine = NULL;

  if(lpKeyName != NULL)
  {
    int lnKeyName = lstrlen(lpKeyName);
    if(!(KeyName = HeapAlloc(GetProcessHeap(), 0, (lnKeyName + 1) * sizeof(WCHAR))))
    {
      if(Machine != NULL)
      {
        HeapFree(GetProcessHeap(), 0, Machine);
      }
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
    if(lnKeyName > 0)
    {
      MultiByteToWideChar(CP_ACP, 0, lpKeyName, -1, KeyName, lnKeyName + 1);
    }
    else
      *KeyName = L'\0';
  }
  else
    KeyName = NULL;
#else
  Machine = (LPWSTR)lpMachine;
  KeyName = (LPWSTR)lpKeyName;
#endif

    /* try to open the key again with more access rights */
  if(RegOpenKeyEx(hKey, NULL, 0, READ_CONTROL, &hInfoKey) != ERROR_SUCCESS)
  {
    /* FIXME - print error with FormatMessage */
    return FALSE;
  }

  ObjectInfo.dwFlags = SI_EDIT_ALL  | SI_ADVANCED | SI_CONTAINER | SI_OWNER_RECURSE | SI_EDIT_PERMS;
  ObjectInfo.hInstance = hInst;
  ObjectInfo.pszServerName = Machine;
  ObjectInfo.pszObjectName = KeyName;
  ObjectInfo.pszPageTitle = KeyName;

  if(!(RegKeySecurity = CRegKeySecurity_fnConstructor(hInfoKey, SE_REGISTRY_KEY, &ObjectInfo, &Result)))
  {
    /* FIXME - print error with FormatMessage */
    return FALSE;
  }

  /* display the security editor dialog */
  pfnEditSecurity(hWndOwner, RegKeySecurity);

  /* dereference the interface, it should be destroyed here */
  CRegKeySecurity_fnRelease(RegKeySecurity);

  RegCloseKey(hInfoKey);

#ifndef UNICODE
  if(Machine != NULL)
  {
    HeapFree(GetProcessHeap(), 0, Machine);
  }
  if(KeyName != NULL)
  {
    HeapFree(GetProcessHeap(), 0, KeyName);
  }
#endif

  return Result;
}

/* EOF */
