/*
 * Regedit ACL Editor for Registry Keys
 *
 * Copyright (C) 2004 - 2006 Thomas Weidenmueller <w3seek@reactos.com>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "regedit.h"

#define INITGUID
#include <guiddef.h>

/* FIXME - shouldn't be defined here... */
DEFINE_GUID(IID_IRegKeySecurity, 0x965fc360, 0x16ff, 0x11d0, 0x0091, 0xcb,0x00,0xaa,0x00,0xbb,0xb7,0x23);
#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
DEFINE_GUID(IID_IRegKeySecurity2, 0xc3ccfdb4, 0x6f88, 0x11d2, 0x00a3, 0xce,0x00,0xc0,0x4f,0xb1,0x78,0x2a);
#endif

/* FIXME: already defined in aclui.h - causing problems when compiling with MSVC/PSDK*/
#ifdef _MSC_VER
#pragma message ("INVESTIGATE ME")
#endif

#if 1 //#ifndef _MSC_VER
DEFINE_GUID(IID_IEffectivePermission, 0x3853dc76, 0x9f35, 0x407c, 0x0088, 0xa1,0xd1,0x93,0x44,0x36,0x5f,0xbc);
DEFINE_GUID(IID_ISecurityObjectTypeInfo, 0xfc3066eb, 0x79ef, 0x444b, 0x0091, 0x11,0xd1,0x8a,0x75,0xeb,0xf2,0xfa);
#endif

/******************************************************************************
   Implementation of the IUnknown methods of CRegKeySecurity
 ******************************************************************************/

static __inline PCRegKeySecurity
impl_from_ISecurityInformation(struct ISecurityInformation *iface)
{
    return (PCRegKeySecurity)((ULONG_PTR)iface - FIELD_OFFSET(CRegKeySecurity,
                                                              lpISecurityInformationVtbl));
}

#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
static __inline PCRegKeySecurity
impl_from_ISecurityInformation2(struct ISecurityInformation2 *iface)
{
    return (PCRegKeySecurity)((ULONG_PTR)iface - FIELD_OFFSET(CRegKeySecurity,
                                                              lpISecurityInformation2Vtbl));
}
#endif

static __inline PCRegKeySecurity
impl_from_ISecurityObjectTypeInfo(struct ISecurityObjectTypeInfo *iface)
{
    return (PCRegKeySecurity)((ULONG_PTR)iface - FIELD_OFFSET(CRegKeySecurity,
                                                              lpISecurityObjectTypeInfoVtbl));
}

static __inline PCRegKeySecurity
impl_from_IEffectivePermission(struct IEffectivePermission *iface)
{
    return (PCRegKeySecurity)((ULONG_PTR)iface - FIELD_OFFSET(CRegKeySecurity,
                                                              lpIEffectivePermissionVtbl));
}

#define impl_to_interface(impl,iface) (struct iface *)(&(impl)->lp##iface##Vtbl)

static __inline ULONG
CRegKeySecurity_fnAddRef(PCRegKeySecurity obj)
{
    return (ULONG)InterlockedIncrement((LONG*)&obj->ref);
}

static __inline ULONG
CRegKeySecurity_fnRelease(PCRegKeySecurity obj)
{
    ULONG Ret;

    Ret = (ULONG)InterlockedDecrement((LONG*)&obj->ref);
    if (Ret == 0)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 obj);
    }

    return Ret;
}

static __inline HRESULT
CRegKeySecurity_fnQueryInterface(PCRegKeySecurity obj,
                                 REFIID iid,
                                 PVOID *pvObject)
{
    PVOID pvObj = NULL;

    if (IsEqualGUID(iid,
                    &IID_IRegKeySecurity))
    {
        pvObj = (PVOID)impl_to_interface(obj,
                                         ISecurityInformation);
    }
#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
    else if (IsEqualGUID(iid,
                         &IID_IRegKeySecurity2))
    {
        pvObj = (PVOID)impl_to_interface(obj,
                                         ISecurityInformation2);
    }
#endif
    else if (IsEqualGUID(iid,
                         &IID_IEffectivePermission))
    {
        pvObj = (PVOID)impl_to_interface(obj,
                                         IEffectivePermission);
    }
    else if (IsEqualGUID(iid,
                         &IID_ISecurityObjectTypeInfo))
    {
        pvObj = (PVOID)impl_to_interface(obj,
                                         ISecurityObjectTypeInfo);
    }

    if (pvObj == NULL)
    {
        return E_NOINTERFACE;
    }

    *pvObject = pvObj;
    CRegKeySecurity_fnAddRef(obj);

    return S_OK;
}


/******************************************************************************
   Definition of the ISecurityInformation interface
 ******************************************************************************/

/* IUnknown */
static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnQueryInterface(struct ISecurityInformation *this,
                                      REFIID iid,
                                      PVOID *pvObject);

static ULONG STDMETHODCALLTYPE
ISecurityInformation_fnAddRef(struct ISecurityInformation *this);

static ULONG STDMETHODCALLTYPE
ISecurityInformation_fnRelease(struct ISecurityInformation *this);

/* ISecurityInformation */
static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetObjectInformation(struct ISecurityInformation *this,
                                            PSI_OBJECT_INFO pObjectInfo);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetSecurity(struct ISecurityInformation *this,
                                   SECURITY_INFORMATION RequestedInformation,
                                   PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                                   BOOL fDefault);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnSetSecurity(struct ISecurityInformation *this,
                                   SECURITY_INFORMATION RequestedInformation,
                                   PSECURITY_DESCRIPTOR pSecurityDescriptor);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetAccessRights(struct ISecurityInformation *this,
                                       const GUID* pguidObjectType,
                                      DWORD dwFlags,
                                      PSI_ACCESS* ppAccess,
                                      ULONG* pcAccesses,
                                      ULONG* piDefaultAccess);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnMapGeneric(struct ISecurityInformation *this,
                                 const GUID* pguidObjectType,
                                 UCHAR* pAceFlags,
                                 ACCESS_MASK* pMask);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetInheritTypes(struct ISecurityInformation *this,
                                      PSI_INHERIT_TYPE* ppInheritTypes,
                                      ULONG* pcInheritTypes);
static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnPropertySheetPageCallback(struct ISecurityInformation *this,
                                                HWND hwnd,
                                                UINT uMsg,
                                                SI_PAGE_TYPE uPage);

static const struct ifaceISecurityInformationVbtl vtblISecurityInformation =
{
    /* IUnknown methods */
    ISecurityInformation_fnQueryInterface,
    ISecurityInformation_fnAddRef,
    ISecurityInformation_fnRelease,

    /* ISecurityInformation methods */
    ISecurityInformation_fnGetObjectInformation,
    ISecurityInformation_fnGetSecurity,
    ISecurityInformation_fnSetSecurity,
    ISecurityInformation_fnGetAccessRights,
    ISecurityInformation_fnMapGeneric,
    ISecurityInformation_fnGetInheritTypes,
    ISecurityInformation_fnPropertySheetPageCallback,
};

#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
/******************************************************************************
   Definition of the ISecurityInformation2 interface
 ******************************************************************************/

/* IUnknown */
static HRESULT STDMETHODCALLTYPE
ISecurityInformation2_fnQueryInterface(struct ISecurityInformation2 *this,
                                       REFIID iid,
                                       PVOID *pvObject);

static ULONG STDMETHODCALLTYPE
ISecurityInformation2_fnAddRef(struct ISecurityInformation2 *this);

static ULONG STDMETHODCALLTYPE
ISecurityInformation2_fnRelease(struct ISecurityInformation2 *this);

/* ISecurityInformation2 */
static BOOL STDMETHODCALLTYPE
ISecurityInformation2_fnIsDaclCanonical(struct ISecurityInformation2 *this,
                                        PACL pDacl);

static HRESULT STDMETHODCALLTYPE
ISecurityInformation2_fnLookupSids(struct ISecurityInformation2 *this,
                                   ULONG cSids,
                                   PSID* rgpSids,
                                   LPDATAOBJECT* ppdo);

static const struct ifaceISecurityInformation2Vbtl vtblISecurityInformation2 =
{
    /* IUnknown methods */
    ISecurityInformation2_fnQueryInterface,
    ISecurityInformation2_fnAddRef,
    ISecurityInformation2_fnRelease,

    /* ISecurityInformation2 methods */
    ISecurityInformation2_fnIsDaclCanonical,
    ISecurityInformation2_fnLookupSids
};
#endif

/******************************************************************************
   Definition of the IEffectivePermission interface
 ******************************************************************************/

/* IUnknown */
static HRESULT STDMETHODCALLTYPE
IEffectivePermission_fnQueryInterface(struct IEffectivePermission *this,
                                      REFIID iid,
                                      PVOID *pvObject);

static ULONG STDMETHODCALLTYPE
IEffectivePermission_fnAddRef(struct IEffectivePermission *this);

static ULONG STDMETHODCALLTYPE
IEffectivePermission_fnRelease(struct IEffectivePermission *this);

/* IEffectivePermission */
static HRESULT STDMETHODCALLTYPE
IEffectivePermission_fnGetEffectivePermission(struct IEffectivePermission *this,
                                              const GUID* pguidObjectType,
                                              PSID pUserSid,
                                              LPCWSTR pszServerName,
                                              PSECURITY_DESCRIPTOR pSD,
                                              POBJECT_TYPE_LIST* ppObjectTypeList,
                                              ULONG* pcObjectTypeListLength,
                                              PACCESS_MASK* ppGrantedAccessList,
                                              ULONG* pcGrantedAccessListLength);

static const struct ifaceIEffectivePermissionVbtl vtblIEffectivePermission =
{
    /* IUnknown methods */
    IEffectivePermission_fnQueryInterface,
    IEffectivePermission_fnAddRef,
    IEffectivePermission_fnRelease,

    /* IEffectivePermissions methods */
    IEffectivePermission_fnGetEffectivePermission
};

/******************************************************************************
   Definition of the ISecurityObjectTypeInfo interface
 ******************************************************************************/

/* IUnknown */
static HRESULT STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnQueryInterface(struct ISecurityObjectTypeInfo *this,
                                         REFIID iid,
                                         PVOID *pvObject);

static ULONG STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnAddRef(struct ISecurityObjectTypeInfo *this);

static ULONG STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnRelease(struct ISecurityObjectTypeInfo *this);

/* ISecurityObjectTypeInfo */
static HRESULT STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnGetInheritSource(struct ISecurityObjectTypeInfo *this,
                                           SECURITY_INFORMATION si,
                                           PACL pACL,
                                           PINHERITED_FROM* ppInheritArray);

static const struct ifaceISecurityObjectTypeInfoVbtl vtblISecurityObjectTypeInfo =
{
    /* IUnknown methods */
    ISecurityObjectTypeInfo_fnQueryInterface,
    ISecurityObjectTypeInfo_fnAddRef,
    ISecurityObjectTypeInfo_fnRelease,

    /* ISecurityObjectTypeInfo methods */
    ISecurityObjectTypeInfo_fnGetInheritSource
};


/******************************************************************************
   Implementation of the ISecurityInformation interface
 ******************************************************************************/

static SI_ACCESS RegAccess[] = {
    {&GUID_NULL, KEY_ALL_ACCESS,         MAKEINTRESOURCEW(IDS_ACCESS_FULLCONTROL),      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_READ,               MAKEINTRESOURCEW(IDS_ACCESS_READ),             SI_ACCESS_GENERAL},
    {&GUID_NULL, KEY_QUERY_VALUE,        MAKEINTRESOURCEW(IDS_ACCESS_QUERYVALUE),       SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_SET_VALUE,          MAKEINTRESOURCEW(IDS_ACCESS_SETVALUE),         SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_CREATE_SUB_KEY,     MAKEINTRESOURCEW(IDS_ACCESS_CREATESUBKEY),     SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_ENUMERATE_SUB_KEYS, MAKEINTRESOURCEW(IDS_ACCESS_ENUMERATESUBKEYS), SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_NOTIFY,             MAKEINTRESOURCEW(IDS_ACCESS_NOTIFY),           SI_ACCESS_SPECIFIC},
    {&GUID_NULL, KEY_CREATE_LINK,        MAKEINTRESOURCEW(IDS_ACCESS_CREATELINK),       SI_ACCESS_SPECIFIC},
    {&GUID_NULL, DELETE,                 MAKEINTRESOURCEW(IDS_ACCESS_DELETE),           SI_ACCESS_SPECIFIC},
    {&GUID_NULL, WRITE_DAC,              MAKEINTRESOURCEW(IDS_ACCESS_WRITEDAC),         SI_ACCESS_SPECIFIC},
    {&GUID_NULL, WRITE_OWNER,            MAKEINTRESOURCEW(IDS_ACCESS_WRITEOWNER),       SI_ACCESS_SPECIFIC},
    {&GUID_NULL, READ_CONTROL,           MAKEINTRESOURCEW(IDS_ACCESS_READCONTROL),      SI_ACCESS_SPECIFIC},
};

static const DWORD RegDefaultAccess = 1; /* KEY_READ */

static GENERIC_MAPPING RegAccessMasks = {
    KEY_READ,
    KEY_WRITE,
    KEY_EXECUTE,
    KEY_ALL_ACCESS
};

static SI_INHERIT_TYPE RegInheritTypes[] = {
    {&GUID_NULL, 0,                                        (LPWSTR)MAKEINTRESOURCEW(IDS_INHERIT_THISKEYONLY)},
    {&GUID_NULL, CONTAINER_INHERIT_ACE,                    (LPWSTR)MAKEINTRESOURCEW(IDS_INHERIT_THISKEYANDSUBKEYS)},
    {&GUID_NULL, INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE, (LPWSTR)MAKEINTRESOURCEW(IDS_INHERIT_SUBKEYSONLY)},
};

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnQueryInterface(struct ISecurityInformation *this,
                                      REFIID iid,
                                      PVOID *pvObject)
{
    if (IsEqualGUID(iid,
                    &IID_IUnknown))
    {
        *pvObject = (PVOID)this;
        ISecurityInformation_fnAddRef(this);
        return S_OK;
    }

    return CRegKeySecurity_fnQueryInterface(impl_from_ISecurityInformation(this),
                                            iid,
                                            pvObject);
}

static ULONG STDMETHODCALLTYPE
ISecurityInformation_fnAddRef(struct ISecurityInformation *this)
{
    return CRegKeySecurity_fnAddRef(impl_from_ISecurityInformation(this));
}

static ULONG STDMETHODCALLTYPE
ISecurityInformation_fnRelease(struct ISecurityInformation *this)
{
    return CRegKeySecurity_fnRelease(impl_from_ISecurityInformation(this));
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetObjectInformation(struct ISecurityInformation *this,
                                            PSI_OBJECT_INFO pObjectInfo)
{
    PCRegKeySecurity obj = impl_from_ISecurityInformation(this);

    *pObjectInfo = obj->ObjectInfo;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetSecurity(struct ISecurityInformation *this,
                                   SECURITY_INFORMATION RequestedInformation,
                                   PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                                   BOOL fDefault)
{
    PCRegKeySecurity obj = impl_from_ISecurityInformation(this);
    LONG ErrorCode;

    ErrorCode = GetNamedSecurityInfoW(obj->szRegKey,
                                      SE_REGISTRY_KEY,
                                      RequestedInformation,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      ppSecurityDescriptor);

    return HRESULT_FROM_WIN32(ErrorCode);
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnSetSecurity(struct ISecurityInformation *this,
                                   SECURITY_INFORMATION RequestedInformation,
                                   PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
    PCRegKeySecurity obj = impl_from_ISecurityInformation(this);

    /* FIXME */
    *obj->Btn = TRUE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetAccessRights(struct ISecurityInformation *this,
                                       const GUID* pguidObjectType,
                                       DWORD dwFlags,
                                       PSI_ACCESS* ppAccess,
                                       ULONG* pcAccesses,
                                       ULONG* piDefaultAccess)
{
    *ppAccess = RegAccess;
    *pcAccesses = COUNT_OF(RegAccess);
    *piDefaultAccess = RegDefaultAccess;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnMapGeneric(struct ISecurityInformation *this,
                                  const GUID* pguidObjectType,
                                  UCHAR* pAceFlags,
                                  ACCESS_MASK* pMask)
{
    MapGenericMask(pMask,
                   &RegAccessMasks);
    *pMask &= ~SYNCHRONIZE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnGetInheritTypes(struct ISecurityInformation *this,
                                       PSI_INHERIT_TYPE* ppInheritTypes,
                                       ULONG* pcInheritTypes)
{
    PCRegKeySecurity obj = impl_from_ISecurityInformation(this);

    /* FIXME */
    if (obj->ObjectInfo.dwFlags & SI_CONTAINER)
    {
        *ppInheritTypes = RegInheritTypes;
        *pcInheritTypes = COUNT_OF(RegInheritTypes);
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation_fnPropertySheetPageCallback(struct ISecurityInformation *this,
                                            HWND hwnd,
                                            UINT uMsg,
                                            SI_PAGE_TYPE uPage)
{
    return S_OK;
}

#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
/******************************************************************************
   Implementation of the ISecurityInformation2 interface
 ******************************************************************************/

static HRESULT STDMETHODCALLTYPE
ISecurityInformation2_fnQueryInterface(struct ISecurityInformation2 *this,
                                       REFIID iid,
                                       PVOID *pvObject)
{
    if (IsEqualGUID(iid,
                    &IID_IUnknown))
    {
        *pvObject = (PVOID)this;
        ISecurityInformation2_fnAddRef(this);
        return S_OK;
    }

    return CRegKeySecurity_fnQueryInterface(impl_from_ISecurityInformation2(this),
                                            iid,
                                            pvObject);
}

static ULONG STDMETHODCALLTYPE
ISecurityInformation2_fnAddRef(struct ISecurityInformation2 *this)
{
    return CRegKeySecurity_fnAddRef(impl_from_ISecurityInformation2(this));
}

static ULONG STDMETHODCALLTYPE
ISecurityInformation2_fnRelease(struct ISecurityInformation2 *this)
{
    return CRegKeySecurity_fnRelease(impl_from_ISecurityInformation2(this));
}

static BOOL STDMETHODCALLTYPE
ISecurityInformation2_fnIsDaclCanonical(struct ISecurityInformation2 *this,
                                        PACL pDacl)
{
    /* FIXME */
    return TRUE;
}

static HRESULT STDMETHODCALLTYPE
ISecurityInformation2_fnLookupSids(struct ISecurityInformation2 *this,
                                   ULONG cSids,
                                   PSID* rgpSids,
                                   LPDATAOBJECT* ppdo)
{
    /* FIXME */
    return E_NOTIMPL;
}
#endif

/******************************************************************************
   Implementation of the IEffectivePermission interface
 ******************************************************************************/

static HRESULT STDMETHODCALLTYPE
IEffectivePermission_fnQueryInterface(struct IEffectivePermission *this,
                                      REFIID iid,
                                      PVOID *pvObject)
{
    if (IsEqualGUID(iid,
                    &IID_IUnknown))
    {
        *pvObject = (PVOID)this;
        IEffectivePermission_fnAddRef(this);
        return S_OK;
    }

    return CRegKeySecurity_fnQueryInterface(impl_from_IEffectivePermission(this),
                                            iid,
                                            pvObject);
}

static ULONG STDMETHODCALLTYPE
IEffectivePermission_fnAddRef(struct IEffectivePermission *this)
{
    return CRegKeySecurity_fnAddRef(impl_from_IEffectivePermission(this));
}

static ULONG STDMETHODCALLTYPE
IEffectivePermission_fnRelease(struct IEffectivePermission *this)
{
    return CRegKeySecurity_fnRelease(impl_from_IEffectivePermission(this));
}

static HRESULT STDMETHODCALLTYPE
IEffectivePermission_fnGetEffectivePermission(struct IEffectivePermission *this,
                                              const GUID* pguidObjectType,
                                              PSID pUserSid,
                                              LPCWSTR pszServerName,
                                              PSECURITY_DESCRIPTOR pSD,
                                              POBJECT_TYPE_LIST* ppObjectTypeList,
                                              ULONG* pcObjectTypeListLength,
                                              PACCESS_MASK* ppGrantedAccessList,
                                              ULONG* pcGrantedAccessListLength)
{
    PACL Dacl = NULL;
    BOOL DaclPresent, DaclDefaulted;
    PACCESS_MASK GrantedAccessList;
    DWORD ErrorCode = ERROR_SUCCESS;
    TRUSTEE Trustee = {0};
    static OBJECT_TYPE_LIST DefObjTypeList = {0};

    *ppObjectTypeList = &DefObjTypeList;
    *pcObjectTypeListLength = 1;

    BuildTrusteeWithSid(&Trustee,
                        pUserSid);

    if (GetSecurityDescriptorDacl(pSD,
                                  &DaclPresent,
                                  &Dacl,
                                  &DaclDefaulted) && DaclPresent)
    {
        GrantedAccessList = (PACCESS_MASK)LocalAlloc(LMEM_FIXED,
                                                     sizeof(ACCESS_MASK));
        if (GrantedAccessList == NULL)
        {
            goto Fail;
        }

        ErrorCode = GetEffectiveRightsFromAcl(Dacl,
                                              &Trustee,
                                              GrantedAccessList);
        if (ErrorCode == ERROR_SUCCESS)
        {
            *ppGrantedAccessList = GrantedAccessList;
            *pcGrantedAccessListLength = 1;
        }
        else
            LocalFree((HLOCAL)GrantedAccessList);
    }
    else
Fail:
        ErrorCode = GetLastError();

    return HRESULT_FROM_WIN32(ErrorCode);
}

/******************************************************************************
   Implementation of the ISecurityObjectTypeInfo interface
 ******************************************************************************/

static HRESULT STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnQueryInterface(struct ISecurityObjectTypeInfo *this,
                                         REFIID iid,
                                         PVOID *pvObject)
{
    if (IsEqualGUID(iid,
                    &IID_IUnknown))
    {
        *pvObject = (PVOID)this;
        ISecurityObjectTypeInfo_fnAddRef(this);
        return S_OK;
    }

    return CRegKeySecurity_fnQueryInterface(impl_from_ISecurityObjectTypeInfo(this),
                                            iid,
                                            pvObject);
}

static ULONG STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnAddRef(struct ISecurityObjectTypeInfo *this)
{
    return CRegKeySecurity_fnAddRef(impl_from_ISecurityObjectTypeInfo(this));
}

static ULONG STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnRelease(struct ISecurityObjectTypeInfo *this)
{
    return CRegKeySecurity_fnRelease(impl_from_ISecurityObjectTypeInfo(this));
}

static HRESULT STDMETHODCALLTYPE
ISecurityObjectTypeInfo_fnGetInheritSource(struct ISecurityObjectTypeInfo *this,
                                           SECURITY_INFORMATION si,
                                           PACL pACL,
                                           PINHERITED_FROM* ppInheritArray)
{
    PCRegKeySecurity obj = impl_from_ISecurityObjectTypeInfo(this);
    PINHERITED_FROM pif, pif2;
    SIZE_T pifSize;
    DWORD ErrorCode, i;
    LPWSTR lpBuf;

    pifSize = pACL->AceCount * sizeof(INHERITED_FROM);
    pif = (PINHERITED_FROM)HeapAlloc(GetProcessHeap(),
                                     0,
                                     pifSize);
    if (pif == NULL)
    {
        return E_OUTOFMEMORY;
    }

    ErrorCode = GetInheritanceSourceW(obj->szRegKey,
                                      SE_REGISTRY_KEY,
                                      si,
                                      (obj->ObjectInfo.dwFlags & SI_CONTAINER) != 0,
                                      NULL,
                                      0,
                                      pACL,
                                      NULL,
                                      &RegAccessMasks,
                                      pif);

    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Calculate the size of the buffer to return */
        for (i = 0;
             i < pACL->AceCount;
             i++)
        {
            if (pif[i].AncestorName != NULL)
            {
                pifSize += (wcslen(pif[i].AncestorName) + 1) * sizeof(WCHAR);
            }
        }

        /* Allocate enough space for the array and the strings */
        pif2 = (PINHERITED_FROM)LocalAlloc(LMEM_FIXED,
                                           pifSize);
        if (pif2 == NULL)
        {
            ErrorCode = GetLastError();
            goto Cleanup;
        }

        /* copy the array and strings to the buffer */
        lpBuf = (LPWSTR)((ULONG_PTR)pif2 + (pACL->AceCount * sizeof(INHERITED_FROM)));
        for (i = 0;
             i < pACL->AceCount;
             i++)
        {
            pif2[i].GenerationGap = pif[i].GenerationGap;
            if (pif[i].AncestorName != NULL)
            {
                pif2[i].AncestorName = lpBuf;
                wcscpy(lpBuf,
                        pif[i].AncestorName);
                lpBuf += wcslen(pif[i].AncestorName) + 1;
            }
            else
                pif2[i].AncestorName = NULL;
        }

        /* return the newly allocated array */
        *ppInheritArray = pif2;
    }

Cleanup:
    FreeInheritedFromArray(pif,
                           pACL->AceCount,
                           NULL);
    HeapFree(GetProcessHeap(),
             0,
             pif);

    return HRESULT_FROM_WIN32(ErrorCode);
}

/******************************************************************************
   Implementation of the CRegKeySecurity constructor
 ******************************************************************************/

static PCRegKeySecurity
CRegKeySecurity_fnConstructor(LPWSTR lpRegKey,
                              HKEY hRootKey,
                              SI_OBJECT_INFO *ObjectInfo,
                              BOOL *Btn)
{
    PCRegKeySecurity obj;

    obj = (PCRegKeySecurity)HeapAlloc(GetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      FIELD_OFFSET(CRegKeySecurity,
                                                   szRegKey[wcslen(lpRegKey) + 1]));
    if (obj != NULL)
    {
        obj->ref = 1;
        obj->lpISecurityInformationVtbl = &vtblISecurityInformation;
#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
        obj->lpISecurityInformation2Vtbl = &vtblISecurityInformation2;
#endif
        obj->lpIEffectivePermissionVtbl = &vtblIEffectivePermission;
        obj->lpISecurityObjectTypeInfoVtbl =  &vtblISecurityObjectTypeInfo;
        obj->ObjectInfo = *ObjectInfo;
        obj->Btn = Btn;
        obj->hRootKey = hRootKey;
        wcscpy(obj->szRegKey,
                lpRegKey);
    }
    else
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);

    return obj;
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

typedef struct _CHANGE_CONTEXT
{
  HKEY hKey;
  LPWSTR KeyString;
} CHANGE_CONTEXT, *PCHANGE_CONTEXT;

typedef BOOL (WINAPI *PEDITSECURITY)(HWND hwndOwner,
                                     struct ISecurityInformation *psi);

static PEDITSECURITY pfnEditSecurity;
static HMODULE hAclUiDll;

BOOL
InitializeAclUiDll(VOID)
{
    if (!(hAclUiDll = LoadLibraryW(L"aclui.dll")))
    {
        return FALSE;
    }

    if (!(pfnEditSecurity = (PEDITSECURITY)GetProcAddress(hAclUiDll,
                                                         "EditSecurity")))
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
    if (hAclUiDll != NULL)
    {
        FreeLibrary(hAclUiDll);
    }
}

BOOL
RegKeyEditPermissions(HWND hWndOwner,
                      HKEY hKey,
                      LPCWSTR lpMachine,
                      LPCWSTR lpKeyName)
{
    BOOL Result = FALSE;
    LPCWSTR lphKey = NULL;
    LPWSTR lpKeyPath = NULL;
    PCRegKeySecurity RegKeySecurity;
    SI_OBJECT_INFO ObjectInfo;
    size_t lnMachine = 0, lnKeyName = 0;

    if (pfnEditSecurity == NULL)
    {
        return FALSE;
    }

    if (lpMachine != NULL)
        lnMachine = wcslen(lpMachine);
    if (lpKeyName != NULL)
        lnKeyName = wcslen(lpKeyName);

    /* build registry path */
    if (lpMachine != NULL &&
        (lpMachine[0] == L'\0' ||
         (lpMachine[0] == L'.' && lpMachine[1] == L'.')))
    {
        lnMachine = 0;
    }

    if (hKey == HKEY_CLASSES_ROOT)
        lphKey = L"CLASSES_ROOT";
    else if (hKey == HKEY_CURRENT_USER)
        lphKey = L"CURRENT_USER";
    else if (hKey == HKEY_LOCAL_MACHINE)
        lphKey = L"MACHINE";
    else if (hKey == HKEY_USERS)
        lphKey = L"USERS";
    else if (hKey == HKEY_CURRENT_CONFIG)
        lphKey = L"CONFIG";
    else
    goto Cleanup;

    lpKeyPath = HeapAlloc(GetProcessHeap(),
                          0,
                          (2 + lnMachine + 1 + wcslen(lphKey) + 1 + lnKeyName) * sizeof(WCHAR));
    if (lpKeyPath == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Cleanup;
    }
    lpKeyPath[0] = L'\0';

    if (lnMachine != 0)
    {
        wcscat(lpKeyPath,
               L"\\\\");
        wcscat(lpKeyPath,
               lpMachine);
        wcscat(lpKeyPath,
               L"\\");
    }

    wcscat(lpKeyPath,
           lphKey);
    if (lpKeyName != NULL && lpKeyName[0] != L'\0')
    {
        if (lpKeyName[0] != L'\\')
        {
            wcscat(lpKeyPath,
                    L"\\");
        }

        wcscat(lpKeyPath,
               lpKeyName);
    }

    ObjectInfo.dwFlags = SI_EDIT_ALL  | SI_ADVANCED | SI_CONTAINER | SI_EDIT_EFFECTIVE | SI_EDIT_PERMS |
                             SI_OWNER_RECURSE | SI_RESET_DACL_TREE | SI_RESET_SACL_TREE;
    ObjectInfo.hInstance = hInst;
    ObjectInfo.pszServerName = (LPWSTR)lpMachine;
    ObjectInfo.pszObjectName = (LPWSTR)lpKeyName; /* FIXME */
    ObjectInfo.pszPageTitle = (LPWSTR)lpKeyName; /* FIXME */

    if (!(RegKeySecurity = CRegKeySecurity_fnConstructor(lpKeyPath,
                                                         hKey,
                                                         &ObjectInfo,
                                                         &Result)))
    {
        goto Cleanup;
    }

    /* display the security editor dialog */
    pfnEditSecurity(hWndOwner,
                    impl_to_interface(RegKeySecurity,
                                      ISecurityInformation));

    /* dereference the interface, it should be destroyed here */
    CRegKeySecurity_fnRelease(RegKeySecurity);

Cleanup:
    if (lpKeyPath != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpKeyPath);
    }

    return Result;
}

/* EOF */
