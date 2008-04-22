#ifndef _REGEXP_SECURITY_H
#define _REGEXP_SECURITY_H

BOOL
InitializeAclUiDll(VOID);

VOID
UnloadAclUiDll(VOID);

#define REGEDIT_IMPLEMENT_ISECURITYINFORMATION2    0

/******************************************************************************
   ISecurityInformation
 ******************************************************************************/

typedef struct ISecurityInformation *LPSECURITYINFORMATION;

typedef struct ifaceISecuritInformationVbtl ifaceISecurityInformationVbtl;
struct ifaceISecurityInformationVbtl
{
    /* IUnknown */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct ISecurityInformation *this,
                                                REFIID iid,
                                                PVOID *pvObject);
    ULONG (STDMETHODCALLTYPE *AddRef)(struct ISecurityInformation *this);
    ULONG (STDMETHODCALLTYPE *Release)(struct ISecurityInformation *this);

    /* ISecurityInformation */
    HRESULT (STDMETHODCALLTYPE *GetObjectInformation)(struct ISecurityInformation *this,
                                                      PSI_OBJECT_INFO pObjectInfo);
    HRESULT (STDMETHODCALLTYPE *GetSecurity)(struct ISecurityInformation *this,
                                             SECURITY_INFORMATION RequestedInformation,
                                             PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                                             BOOL fDefault);
    HRESULT (STDMETHODCALLTYPE *SetSecurity)(struct ISecurityInformation *this,
                                             SECURITY_INFORMATION RequestedInformation,
                                             PSECURITY_DESCRIPTOR pSecurityDescriptor);
    HRESULT (STDMETHODCALLTYPE *GetAccessRights)(struct ISecurityInformation *this,
                                                 const GUID* pguidObjectType,
                                                 DWORD dwFlags,
                                                 PSI_ACCESS* ppAccess,
                                                 ULONG* pcAccesses,
                                                 ULONG* piDefaultAccess);
    HRESULT (STDMETHODCALLTYPE *MapGeneric)(struct ISecurityInformation *this,
                                            const GUID* pguidObjectType,
                                            UCHAR* pAceFlags,
                                            ACCESS_MASK* pMask);
    HRESULT (STDMETHODCALLTYPE *GetInheritTypes)(struct ISecurityInformation *this,
                                                 PSI_INHERIT_TYPE* ppInheritTypes,
                                                 ULONG* pcInheritTypes);
    HRESULT (STDMETHODCALLTYPE *PropertySheetPageCallback)(struct ISecurityInformation *this,
                                                           HWND hwnd,
                                                           UINT uMsg,
                                                           SI_PAGE_TYPE uPage);
};

#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
/******************************************************************************
   ISecurityInformation2
 ******************************************************************************/

typedef struct ISecurityInformation2 *LPSECURITYINFORMATION2;

typedef struct ifaceISecurityInformation2Vbtl ifaceISecurityInformation2Vbtl;
struct ifaceISecurityInformation2Vbtl
{
    /* IUnknown */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct ISecurityInformation2 *this,
                                                REFIID iid,
                                                PVOID *pvObject);
    ULONG (STDMETHODCALLTYPE *AddRef)(struct ISecurityInformation2 *this);
    ULONG (STDMETHODCALLTYPE *Release)(struct ISecurityInformation2 *this);

    /* ISecurityInformation2 */
    BOOL (STDMETHODCALLTYPE *IsDaclCanonical)(struct ISecurityInformation2 *this,
                                              PACL pDacl);
    HRESULT (STDMETHODCALLTYPE *LookupSids)(struct ISecurityInformation2 *this,
                                            ULONG cSids,
                                            PSID* rgpSids,
                                            LPDATAOBJECT* ppdo);
};
#endif

/******************************************************************************
   IEffectivePermission
 ******************************************************************************/

typedef struct IEffectivePermission *LPEFFECTIVEPERMISSION;

typedef struct ifaceIEffectivePermissionVbtl ifaceIEffectivePermissionVbtl;
struct ifaceIEffectivePermissionVbtl
{
    /* IUnknown */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct IEffectivePermission *this,
                                                REFIID iid,
                                                PVOID *pvObject);
    ULONG (STDMETHODCALLTYPE *AddRef)(struct IEffectivePermission *this);
    ULONG (STDMETHODCALLTYPE *Release)(struct IEffectivePermission *this);

    /* IEffectivePermission */
    HRESULT (STDMETHODCALLTYPE *GetEffectivePermission)(struct IEffectivePermission *this,
                                                        const GUID* pguidObjectType,
                                                        PSID pUserSid,
                                                        LPCWSTR pszServerName,
                                                        PSECURITY_DESCRIPTOR pSD,
                                                        POBJECT_TYPE_LIST* ppObjectTypeList,
                                                        ULONG* pcObjectTypeListLength,
                                                        PACCESS_MASK* ppGrantedAccessList,
                                                        ULONG* pcGrantedAccessListLength);
};

/******************************************************************************
   ISecurityObjectTypeInfo
 ******************************************************************************/

typedef struct ISecurityObjectTypeInfo *LPSECURITYOBJECTTYPEINFO;

typedef struct ifaceISecurityObjectTypeInfoVbtl ifaceISecurityObjectTypeInfoVbtl;
struct ifaceISecurityObjectTypeInfoVbtl
{
    /* IUnknown */
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(struct ISecurityObjectTypeInfo *this,
                                                REFIID iid,
                                                PVOID *pvObject);
    ULONG (STDMETHODCALLTYPE *AddRef)(struct ISecurityObjectTypeInfo *this);
    ULONG (STDMETHODCALLTYPE *Release)(struct ISecurityObjectTypeInfo *this);

    /* ISecurityObjectTypeInfo */
    HRESULT (STDMETHODCALLTYPE *GetInheritSource)(struct ISecurityObjectTypeInfo *this,
                                                  SECURITY_INFORMATION si,
                                                  PACL pACL,
                                                  PINHERITED_FROM* ppInheritArray);
};

/******************************************************************************
   CRegKeySecurity
 ******************************************************************************/

typedef struct _CRegKeySecurity
{
    /* IUnknown fields and interfaces */
    const struct ifaceISecurityInformationVbtl *lpISecurityInformationVtbl;
#if REGEDIT_IMPLEMENT_ISECURITYINFORMATION2
    const struct ifaceISecurityInformation2Vbtl *lpISecurityInformation2Vtbl;
#endif
    const struct ifaceIEffectivePermissionVbtl *lpIEffectivePermissionVtbl;
    const struct ifaceISecurityObjectTypeInfoVbtl *lpISecurityObjectTypeInfoVtbl;
    DWORD ref;

    /* CRegKeySecurity fields */
    SI_OBJECT_INFO ObjectInfo;
    BOOL *Btn;
    HKEY hRootKey;
    TCHAR szRegKey[1];
} CRegKeySecurity, *PCRegKeySecurity;

#endif /* _REGEXP_SECURITY_H */

/* EOF */
