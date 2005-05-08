#ifndef _REGEXP_SECURITY_H
#define _REGEXP_SECURITY_H

/* FIXME - remove the definition */
DWORD STDCALL
GetSecurityInfo(HANDLE handle,
                SE_OBJECT_TYPE ObjectType,
                SECURITY_INFORMATION SecurityInfo,
                PSID* ppsidOwner,
                PSID* ppsidGroup,
                PACL* ppDacl,
                PACL* ppSacl,
                PSECURITY_DESCRIPTOR* ppSecurityDescriptor);

DEFINE_GUID(IID_CRegKeySecurity, 0x965fc360, 0x16ff, 0x11d0, 0x0091, 0xcb,0x00,0xaa,0x00,0xbb,0xb7,0x23);

/******************************************************************************
   CRegKeySecurity
 ******************************************************************************/

typedef struct CRegKeySecurity *LPREGKEYSECURITY;

typedef struct ifaceCRegKeySecurityVbtl ifaceCRegKeySecurityVbtl;
struct ifaceCRegKeySecurityVbtl
{
  /* IUnknown */
  HRESULT (STDMETHODCALLTYPE *QueryInterface)(LPREGKEYSECURITY this,
                                              REFIID iid,
					      PVOID *pvObject);
  ULONG (STDMETHODCALLTYPE *AddRef)(LPREGKEYSECURITY this);
  ULONG (STDMETHODCALLTYPE *Release)(LPREGKEYSECURITY this);

  /* CRegKeySecurity */
  HRESULT (STDMETHODCALLTYPE *GetObjectInformation)(LPREGKEYSECURITY this,
                                                    PSI_OBJECT_INFO pObjectInfo);
  HRESULT (STDMETHODCALLTYPE *GetSecurity)(LPREGKEYSECURITY this,
                                           SECURITY_INFORMATION RequestedInformation,
                                           PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                                           BOOL fDefault);
  HRESULT (STDMETHODCALLTYPE *SetSecurity)(LPREGKEYSECURITY this,
                                           SECURITY_INFORMATION RequestedInformation,
                                           PSECURITY_DESCRIPTOR pSecurityDescriptor);
  HRESULT (STDMETHODCALLTYPE *GetAccessRights)(LPREGKEYSECURITY this,
                                               const GUID* pguidObjectType,
                                               DWORD dwFlags,
                                               PSI_ACCESS* ppAccess,
                                               ULONG* pcAccesses,
                                               ULONG* piDefaultAccess);
  HRESULT (STDMETHODCALLTYPE *MapGeneric)(LPREGKEYSECURITY this,
                                          const GUID* pguidObjectType,
                                          UCHAR* pAceFlags,
                                          ACCESS_MASK* pMask);
  HRESULT (STDMETHODCALLTYPE *GetInheritTypes)(LPREGKEYSECURITY this,
                                               PSI_INHERIT_TYPE* ppInheritTypes,
                                               ULONG* pcInheritTypes);
  HRESULT (STDMETHODCALLTYPE *PropertySheetPageCallback)(LPREGKEYSECURITY this,
                                                         HWND hwnd,
                                                         UINT uMsg,
                                                         SI_PAGE_TYPE uPage);
};

typedef struct CRegKeySecurity
{
  /* IUnknown fields */
  ifaceCRegKeySecurityVbtl* lpVtbl;
  DWORD ref;
  /* CRegKeySecurity fields */
  HANDLE Handle;
  SE_OBJECT_TYPE ObjectType;
  SI_OBJECT_INFO ObjectInfo;
  BOOL *Btn;
} REGKEYSECURITY;

HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnQueryInterface(LPREGKEYSECURITY this,
                                                           REFIID iid,
							   PVOID *pvObject);
ULONG STDMETHODCALLTYPE CRegKeySecurity_fnAddRef(LPREGKEYSECURITY this);
ULONG STDMETHODCALLTYPE CRegKeySecurity_fnRelease(LPREGKEYSECURITY this);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnGetObjectInformation(LPREGKEYSECURITY this,
                                                                 PSI_OBJECT_INFO pObjectInfo);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnGetSecurity(LPREGKEYSECURITY this,
                                                        SECURITY_INFORMATION RequestedInformation,
                                                        PSECURITY_DESCRIPTOR* ppSecurityDescriptor,
                                                        BOOL fDefault);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnSetSecurity(LPREGKEYSECURITY this,
                                                        SECURITY_INFORMATION RequestedInformation,
                                                        PSECURITY_DESCRIPTOR pSecurityDescriptor);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnGetAccessRights(LPREGKEYSECURITY this,
                                                            const GUID* pguidObjectType,
                                                            DWORD dwFlags,
                                                            PSI_ACCESS* ppAccess,
                                                            ULONG* pcAccesses,
                                                            ULONG* piDefaultAccess);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnMapGeneric(LPREGKEYSECURITY this,
                                                       const GUID* pguidObjectType,
                                                       UCHAR* pAceFlags,
                                                       ACCESS_MASK* pMask);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnGetInheritTypes(LPREGKEYSECURITY this,
                                                            PSI_INHERIT_TYPE* ppInheritTypes,
                                                            ULONG* pcInheritTypes);
HRESULT STDMETHODCALLTYPE CRegKeySecurity_fnPropertySheetPageCallback(LPREGKEYSECURITY this,
                                                                      HWND hwnd,
                                                                      UINT uMsg,
                                                                      SI_PAGE_TYPE uPage);

static ifaceCRegKeySecurityVbtl efvt =
{
  /* IUnknown methods */
  CRegKeySecurity_fnQueryInterface,
  CRegKeySecurity_fnAddRef,
  CRegKeySecurity_fnRelease,

  /* CRegKeySecurity methods */
  CRegKeySecurity_fnGetObjectInformation,
  CRegKeySecurity_fnGetSecurity,
  CRegKeySecurity_fnSetSecurity,
  CRegKeySecurity_fnGetAccessRights,
  CRegKeySecurity_fnMapGeneric,
  CRegKeySecurity_fnGetInheritTypes,
  CRegKeySecurity_fnPropertySheetPageCallback
};

#endif /* _REGEXP_SECURITY_H */

/* EOF */
