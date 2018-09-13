//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dssi.cpp
//
//  This file contains the implementation of the CDSSecurityInfo object,
//  which provides the ISecurityInformation interface for invoking
//  the ACL Editor.
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <dssec.h>
#include "exnc.h"

TCHAR const c_szDomainClass[]       = DOMAIN_CLASS_NAME;    // adsnms.h

GENERIC_MAPPING g_DSMap =
{
    DS_GENERIC_READ,
    DS_GENERIC_WRITE,
    DS_GENERIC_EXECUTE,
    DS_GENERIC_ALL
};


//
// CDSSecurityInfo (ISecurityInformation) class definition
//
class CDSSecurityInfo : public ISecurityInformation, CUnknown
{
protected:
    GUID        m_guidObjectType;
    BSTR        m_strServerName;
    BSTR        m_strObjectPath;
    BSTR        m_strObjectClass;
    BSTR        m_strDisplayName;
    BSTR        m_strSchemaRootPath;
    IDirectoryObject *m_pDsObject;
    PSECURITY_DESCRIPTOR m_pSD;
    DWORD       m_dwSIFlags;
    DWORD       m_dwInitFlags;  // DSSI_*
    HANDLE      m_hInitThread;
    volatile BOOL m_bThreadAbort;
    PFNREADOBJECTSECURITY  m_pfnReadSD;
    PFNWRITEOBJECTSECURITY m_pfnWriteSD;
    LPARAM      m_lpReadContext;
    LPARAM      m_lpWriteContext;

public:
    virtual ~CDSSecurityInfo();

    STDMETHODIMP Init(LPCWSTR pszObjectPath,
                      LPCWSTR pszObjectClass,
                      LPCWSTR pszServer,
                      LPCWSTR pszUserName,
                      LPCWSTR pszPassword,
                      DWORD   dwFlags,
                      PFNREADOBJECTSECURITY  pfnReadSD,
                      PFNWRITEOBJECTSECURITY pfnWriteSD,
                      LPARAM lpContext);

    // IUnknown
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();
    
    // ISecurityInformation
    STDMETHODIMP GetObjectInformation(PSI_OBJECT_INFO pObjectInfo);
    STDMETHODIMP GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault);
    STDMETHODIMP SetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR pSD);
    STDMETHODIMP GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccess,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess);
    STDMETHODIMP MapGeneric(const GUID *pguidObjectType,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask);
    STDMETHODIMP GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes);
    STDMETHODIMP PropertySheetPageCallback(HWND hwnd,
                                           UINT uMsg,
                                           SI_PAGE_TYPE uPage);

private:
    HRESULT Init2(LPCWSTR pszUserName, LPCWSTR pszPassword);
    HRESULT Init3();

    DWORD CheckObjectAccess();

    void WaitOnInitThread(void)
        { WaitOnThread(&m_hInitThread); }

    static DWORD WINAPI InitThread(LPVOID pvThreadData);

    static HRESULT WINAPI DSReadObjectSecurity(LPCWSTR pszObjectPath,
                                               SECURITY_INFORMATION si,
                                               PSECURITY_DESCRIPTOR *ppSD,
                                               LPARAM lpContext);

    static HRESULT WINAPI DSWriteObjectSecurity(LPCWSTR pszObjectPath,
                                                SECURITY_INFORMATION si,
                                                PSECURITY_DESCRIPTOR pSD,
                                                LPARAM lpContext);
};


//
// CDSSecurityInfo (ISecurityInformation) implementation
//
CDSSecurityInfo::~CDSSecurityInfo()
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::~CDSSecurityInfo");

    m_bThreadAbort = TRUE;

    if (m_hInitThread != NULL)
    {
        WaitForSingleObject(m_hInitThread, INFINITE);
        CloseHandle(m_hInitThread);
    }

    DoRelease(m_pDsObject);

    SysFreeString(m_strServerName);
    SysFreeString(m_strObjectPath);
    SysFreeString(m_strObjectClass);
    SysFreeString(m_strDisplayName);
    SysFreeString(m_strSchemaRootPath);

    if (m_pSD != NULL)
        LocalFree(m_pSD);

    TraceLeaveVoid();
}


STDMETHODIMP
CDSSecurityInfo::Init(LPCWSTR pszObjectPath,
                      LPCWSTR pszObjectClass,
                      LPCWSTR pszServer,
                      LPCWSTR pszUserName,
                      LPCWSTR pszPassword,
                      DWORD   dwFlags,
                      PFNREADOBJECTSECURITY  pfnReadSD,
                      PFNWRITEOBJECTSECURITY pfnWriteSD,
                      LPARAM lpContext)
{
    HRESULT hr = S_OK;
    DWORD   dwThreadID;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init");
    TraceAssert(pszObjectPath != NULL);
    TraceAssert(m_strObjectPath == NULL);    // only initialize once

    m_dwInitFlags = dwFlags;

    m_pfnReadSD = DSReadObjectSecurity;
    m_pfnWriteSD = DSWriteObjectSecurity;
    m_lpReadContext = (LPARAM)this;
    m_lpWriteContext = (LPARAM)this;

    if (pfnReadSD)
    {
        m_pfnReadSD = pfnReadSD;
        m_lpReadContext = lpContext;
    }

    if (pfnWriteSD)
    {
        m_pfnWriteSD = pfnWriteSD;
        m_lpWriteContext = lpContext;
    }

    m_strObjectPath = SysAllocString(pszObjectPath);
    if (m_strObjectPath == NULL)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to copy the object path");

    if (pszObjectClass && *pszObjectClass)
        m_strObjectClass = SysAllocString(pszObjectClass);

    if (pszServer)
    {
        // Skip any preceding backslashes
        while (L'\\' == *pszServer)
            pszServer++;

        if (*pszServer)
            m_strServerName = SysAllocString(pszServer);
    }

    // Init2 cracks the path, binds to the object, checks access to
    // the object and gets the schema path.  This used to be done on
    // the other thread below, but is faster now than it used to be.
    //
    // It's preferable to do it here where we can fail and prevent the
    // page from being created if, for example, the user has no access
    // to the object's security descriptor.  This is better than always
    // creating the Security page and having it show a dorky message
    // when initialization fails.
    hr = Init2(pszUserName, pszPassword);
    if (SUCCEEDED(hr))
    {
        if (m_strObjectClass && m_strSchemaRootPath)
        {
            // Do the rest of the initialization (schema queries, etc.) on
            // another thread and allow the UI thread to continue.
            m_hInitThread = CreateThread(NULL,
                                         0,
                                         InitThread,
                                         this,
                                         0,
                                         &dwThreadID);
        }
        else
        {
            //
            // We evidently don't have read_property access to the object,
            // so just assume it's not a container, so we don't have to deal
            // with inherit types.
            //
            // We need to struggle on as best as we can. If someone removes
            // all access to an object, this is the only way an admin can
            // restore it.
            //
            m_guidObjectType = GUID_NULL;
        }
    }

exit_gracefully:

    TraceLeaveResult(hr);
}


char const c_szDsGetDcNameProc[]       = "DsGetDcNameW";
char const c_szNetApiBufferFreeProc[]  = "NetApiBufferFree";
typedef DWORD (WINAPI *PFN_DSGETDCNAME)(LPCWSTR, LPCWSTR, GUID*, LPCWSTR, ULONG, PDOMAIN_CONTROLLER_INFOW*);
typedef DWORD (WINAPI *PFN_NETAPIFREE)(LPVOID);

HRESULT
GetDsDcAddress(BSTR *pbstrDcAddress)
{
    HRESULT hr = E_FAIL;
    HMODULE hNetApi32 = LoadLibrary(c_szNetApi32);
    if (hNetApi32)
    {
        PFN_DSGETDCNAME pfnDsGetDcName = (PFN_DSGETDCNAME)GetProcAddress(hNetApi32, c_szDsGetDcNameProc);
        PFN_NETAPIFREE pfnNetApiFree = (PFN_NETAPIFREE)GetProcAddress(hNetApi32, c_szNetApiBufferFreeProc);

        if (pfnDsGetDcName && pfnNetApiFree)
        {
            PDOMAIN_CONTROLLER_INFOW pDCI;
            DWORD dwErr = (*pfnDsGetDcName)(NULL, NULL, NULL, NULL,
                                            DS_DIRECTORY_SERVICE_REQUIRED | DS_IP_REQUIRED,
                                            &pDCI);
            hr = HRESULT_FROM_WIN32(dwErr);
            if (SUCCEEDED(hr))
            {
                LPCWSTR pszAddress = pDCI->DomainControllerAddress;
                // Skip any preceding backslashes
                while (L'\\' == *pszAddress)
                    pszAddress++;
                *pbstrDcAddress = SysAllocString(pszAddress);
                if (NULL == *pbstrDcAddress)
                    hr = E_OUTOFMEMORY;
                (*pfnNetApiFree)(pDCI);
            }
        }
        FreeLibrary(hNetApi32);
    }
    return hr;
}

HRESULT
CDSSecurityInfo::Init2(LPCWSTR pszUserName, LPCWSTR pszPassword)
{
    HRESULT hr = S_OK;
    DWORD dwAccessGranted;
    PADS_OBJECT_INFO pObjectInfo = NULL;
    IADsPathname *pPath = NULL;
    LPWSTR pszTemp;
    DWORD dwPrivs[] = { SE_SECURITY_PRIVILEGE, SE_TAKE_OWNERSHIP_PRIVILEGE };
    HANDLE hToken = INVALID_HANDLE_VALUE;
    PADS_ATTR_INFO pAttributeInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init2");
    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pDsObject == NULL);  // only do this one time

    //
    // Create an ADsPathname object to parse the path and get the
    // leaf name (for display) and server name (if necessary)
    //
    CoCreateInstance(CLSID_Pathname,
                     NULL,
                     CLSCTX_INPROC_SERVER,
                     IID_IADsPathname,
                     (LPVOID*)&pPath);
    if (pPath)
    {
        if (FAILED(pPath->Set(m_strObjectPath, ADS_SETTYPE_FULL)))
            DoRelease(pPath); // sets pPath to NULL
    }

    if (NULL == m_strServerName)
    {
        // The path may or may not specify a server.  If not, call DsGetDcName
        if (pPath)
            hr = pPath->Retrieve(ADS_FORMAT_SERVER, &m_strServerName);
        if (!pPath || FAILED(hr))
            hr = GetDsDcAddress(&m_strServerName);
        FailGracefully(hr, "Unable to get server name");
    }
    Trace((TEXT("Server \"%s\""), m_strServerName));

    // Enable privileges before binding so CheckObjectAccess
    // and DSRead/WriteObjectSecurity work correctly.
    hToken = EnablePrivileges(dwPrivs, ARRAYSIZE(dwPrivs));

    // Bind to the object and get the schema path, etc.
    Trace((TEXT("Calling ADsOpenObject(%s)"), m_strObjectPath));
    hr = ADsOpenObject(m_strObjectPath,
                       (LPWSTR)pszUserName,
                       (LPWSTR)pszPassword,
                       ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                       IID_IDirectoryObject,
                       (LPVOID*)&m_pDsObject);
    FailGracefully(hr, "Failed to get the DS object");

    // Assume certain access by default
    if (m_dwInitFlags & DSSI_READ_ONLY)
        dwAccessGranted = READ_CONTROL;
    else
        dwAccessGranted = READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY;

    if (!(m_dwInitFlags & DSSI_NO_ACCESS_CHECK))
    {
        // Check whether the user has permission to do anything to
        // the security descriptor on this object.
        dwAccessGranted = CheckObjectAccess();
        Trace((TEXT("AccessGranted = 0x%08x"), dwAccessGranted));

        if (!(dwAccessGranted & (READ_CONTROL | WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY)))
            ExitGracefully(hr, E_ACCESSDENIED, "No access");
    }

    // Translate the access into SI_* flags, starting with this:
    m_dwSIFlags = SI_EDIT_ALL | SI_ADVANCED | SI_EDIT_PROPERTIES | SI_SERVER_IS_DC;

    if (!(dwAccessGranted & WRITE_DAC))
        m_dwSIFlags |= SI_READONLY;

    if (!(dwAccessGranted & WRITE_OWNER))
    {
        if (!(dwAccessGranted & READ_CONTROL))
            m_dwSIFlags &= ~SI_EDIT_OWNER;
        else
            m_dwSIFlags |= SI_OWNER_READONLY;
    }

    if (!(dwAccessGranted & ACCESS_SYSTEM_SECURITY) || (m_dwInitFlags & DSSI_NO_EDIT_SACL))
        m_dwSIFlags &= ~SI_EDIT_AUDITS;

    if (m_dwInitFlags & DSSI_NO_EDIT_OWNER)
        m_dwSIFlags &= ~SI_EDIT_OWNER;

    //
    // Get the displayName property
    //
    pszTemp = (LPWSTR)c_szDisplayName;
    m_pDsObject->GetObjectAttributes(&pszTemp,
                                     1,
                                     &pAttributeInfo,
                                     &dwAttributesReturned);
    if (pAttributeInfo)
    {
        m_strDisplayName = SysAllocString(pAttributeInfo->pADsValues->CaseExactString);
        FreeADsMem(pAttributeInfo);
        pAttributeInfo = NULL;
    }

    // If that failed, try the leaf name.
    if (!m_strDisplayName && pPath)
    {
        // Retrieve the display name
        pPath->SetDisplayType(ADS_DISPLAY_VALUE_ONLY);
        pPath->Retrieve(ADS_FORMAT_LEAF, &m_strDisplayName);
        pPath->SetDisplayType(ADS_DISPLAY_FULL);
    }

    // Get the class name and schema path
    m_pDsObject->GetObjectInformation(&pObjectInfo);
    if (pObjectInfo)
    {
        //
        // Note that m_strObjectClass, if provided, can be different
        // than pObjectInfo->pszClassName.  This is true when editing default
        // ACLs on schema class objects, for example, in which case
        // pObjectInfo->pszClassName will be "attributeSchema" but m_strObjectClass
        // will be something else such as "computer" or "user". Be
        // careful to only use pObjectInfo->pszClassName for getting the path of
        // the schema root, and use m_strObjectClass for everything else.
        //
        // If m_strObjectClass is not provided, use pObjectInfo->pszClassName.
        //
        if (m_strObjectClass == NULL)
            m_strObjectClass = SysAllocString(pObjectInfo->pszClassName);

        // If this is a root object (i.e. domain), hide the ACL protect checkbox
        // Note that there is more than one form of "domain", e.g. "domainDNS"
        // so look for anything that starts with "domain".
        // If this is a root object (i.e. domain), hide the ACL protect checkbox
        if ((m_dwInitFlags & DSSI_IS_ROOT)
            || (m_strObjectClass &&
                CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                                            NORM_IGNORECASE,
                                            m_strObjectClass,
                                            ARRAYSIZE(c_szDomainClass) - 1,
                                            c_szDomainClass,
                                            ARRAYSIZE(c_szDomainClass) - 1)))
        {
            m_dwSIFlags |= SI_NO_ACL_PROTECT;
        }

        // Get the the path of the schema root
        int nClassLen;
        nClassLen = lstrlenW(pObjectInfo->pszClassName);
        pszTemp = pObjectInfo->pszSchemaDN + lstrlenW(pObjectInfo->pszSchemaDN) - nClassLen;
        if (CSTR_EQUAL == CompareString(LOCALE_SYSTEM_DEFAULT,
                                        NORM_IGNORECASE,
                                        pszTemp,
                                        nClassLen,
                                        pObjectInfo->pszClassName,
                                        nClassLen))
        {
            *pszTemp = L'\0';
        }

        // Save the schema root path
        m_strSchemaRootPath = SysAllocString(pObjectInfo->pszSchemaDN);

        // If we still don't have a display name, just copy the RDN.
        // Ugly, but better than nothing.
        if (NULL == m_strDisplayName)
            m_strDisplayName = SysAllocString(pObjectInfo->pszRDN);
    }

exit_gracefully:

    if (pObjectInfo)
        FreeADsMem(pObjectInfo);

    DoRelease(pPath);
    ReleasePrivileges(hToken);

    TraceLeaveResult(hr);
}


HRESULT
CDSSecurityInfo::Init3()
{
    HRESULT hr = S_OK;
    IADsClass *pDsClass = NULL;
    VARIANT var = {0};

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::Init3");
    TraceAssert(m_strSchemaRootPath != NULL);
    TraceAssert(m_strObjectClass != NULL);

    if (m_bThreadAbort)
        goto exit_gracefully;

    // Create the schema cache
    hr = SchemaCache_Create(m_strServerName);
    FailGracefully(hr, "Unable to create schema cache");

    // Bind to the schema class object
    hr = Schema_BindToObject(m_strSchemaRootPath,
                             m_strObjectClass,
                             IID_IADsClass,
                             (LPVOID*)&pDsClass);
    FailGracefully(hr, "Failed to get the Schema class object");

    // Get the class GUID
    Schema_GetObjectID(pDsClass, &m_guidObjectType);

    if (m_bThreadAbort)
        goto exit_gracefully;

    // See if this object is a container, by getting the list of possible
    // child classes.  If this fails, treat it as a non-container.
    pDsClass->get_Containment(&var);

    if (V_VT(&var) == (VT_ARRAY | VT_VARIANT))
    {
        LPSAFEARRAY psa = V_ARRAY(&var);

        TraceAssert(psa && psa->cDims == 1);

        if (psa->rgsabound[0].cElements > 0)
        {
            m_dwSIFlags |= SI_CONTAINER;
        }
    }
    else if (V_VT(&var) == VT_BSTR) // single entry
    {
        TraceAssert(V_BSTR(&var));
        m_dwSIFlags |= SI_CONTAINER;
    }

exit_gracefully:

    VariantClear(&var);
    DoRelease(pDsClass);

    TraceLeaveResult(hr);
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

#undef CLASS_NAME
#define CLASS_NAME CDSSecurityInfo
#include "unknown.inc"

STDMETHODIMP
CDSSecurityInfo::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    INTERFACES iface[] =
    {
        &IID_ISecurityInformation, static_cast<LPSECURITYINFO>(this),
    };

    return HandleQueryInterface(riid, ppv, iface, ARRAYSIZE(iface));
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CDSSecurityInfo::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetObjectInformation");
    TraceAssert(pObjectInfo != NULL &&
                !IsBadWritePtr(pObjectInfo, SIZEOF(*pObjectInfo)));

    pObjectInfo->hInstance = GLOBAL_HINSTANCE;
    pObjectInfo->dwFlags = m_dwSIFlags;
    pObjectInfo->pszServerName = m_strServerName;
    pObjectInfo->pszObjectName = m_strDisplayName ? m_strDisplayName : m_strObjectPath;

    if (!IsEqualGUID(m_guidObjectType, GUID_NULL))
    {
        pObjectInfo->dwFlags |= SI_OBJECT_GUID;
        pObjectInfo->guidObjectType = m_guidObjectType;
    }

    TraceLeaveResult(S_OK);
}


STDMETHODIMP
CDSSecurityInfo::GetSecurity(SECURITY_INFORMATION si,
                             PSECURITY_DESCRIPTOR *ppSD,
                             BOOL fDefault)
{
    HRESULT hr = S_OK;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetSecurity");
    TraceAssert(si != 0);
    TraceAssert(ppSD != NULL);

    *ppSD = NULL;

    // BUGBUG: how to implement this?
    if (fDefault)
        TraceLeaveResult(E_NOTIMPL);

    if (!(si & SACL_SECURITY_INFORMATION) && m_pSD != NULL)
    {
        ULONG nLength = GetSecurityDescriptorLength(m_pSD);

        *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, nLength);
        if (*ppSD != NULL)
            CopyMemory(*ppSD, m_pSD, nLength);
        else
            hr = E_OUTOFMEMORY;
    }
    else
    {
        TraceAssert(m_strObjectPath != NULL);
        TraceAssert(m_pfnReadSD != NULL)
        hr = (*m_pfnReadSD)(m_strObjectPath, si, ppSD, m_lpReadContext);
    }

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::SetSecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr;
    DWORD dwErr;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::SetSecurity");
    TraceAssert(si != 0);
    TraceAssert(pSD != NULL);

    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pfnWriteSD != NULL)
    hr = (*m_pfnWriteSD)(m_strObjectPath, si, pSD, m_lpWriteContext);

    if (SUCCEEDED(hr) && m_pSD != NULL && (si != SACL_SECURITY_INFORMATION))
    {
        // The initial security descriptor is no longer valid.
        LocalFree(m_pSD);
        m_pSD = NULL;
    }

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::GetAccessRights(const GUID* pguidObjectType,
                                 DWORD dwFlags,
                                 PSI_ACCESS *ppAccesses,
                                 ULONG *pcAccesses,
                                 ULONG *piDefaultAccess)
{
    HRESULT hr;
    LPCTSTR pszClassName = NULL;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetAccessRights");
    TraceAssert(ppAccesses != NULL);
    TraceAssert(pcAccesses != NULL);
    TraceAssert(piDefaultAccess != NULL);

    *ppAccesses = NULL;
    *pcAccesses = 0;
    *piDefaultAccess = 0;

    WaitOnInitThread();

    if (pguidObjectType == NULL || IsEqualGUID(*pguidObjectType, GUID_NULL))
    {
        pguidObjectType = &m_guidObjectType;
        pszClassName = m_strObjectClass;
    }

    if (m_dwInitFlags & DSSI_NO_FILTER)
        dwFlags |= SCHEMA_NO_FILTER;

    // No schema path means we don't have read_property access to the object.
    // This limits what we can do.
    if (NULL == m_strSchemaRootPath)
        dwFlags |= SCHEMA_COMMON_PERM;

    hr = SchemaCache_GetAccessRights(pguidObjectType,
                                     pszClassName,
                                     m_strSchemaRootPath,
                                     dwFlags,
                                     ppAccesses,
                                     pcAccesses,
                                     piDefaultAccess);
    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::MapGeneric(const GUID* /*pguidObjectType*/,
                            UCHAR *pAceFlags,
                            ACCESS_MASK *pmask)
{
    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::MapGeneric");
    TraceAssert(pAceFlags != NULL);
    TraceAssert(pmask != NULL);

    // Only CONTAINER_INHERIT_ACE has meaning for DS
    *pAceFlags &= ~OBJECT_INHERIT_ACE;

    MapGenericMask(pmask, &g_DSMap);

    // We don't expose SYNCHRONIZE, so don't pass it through
    // to the UI.  192389
    *pmask &= ~SYNCHRONIZE;

    TraceLeaveResult(S_OK);
}


STDMETHODIMP
CDSSecurityInfo::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                                 ULONG *pcInheritTypes)
{
    HRESULT hr;
    DWORD dwFlags = 0;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::GetInheritTypes");

    if (m_dwInitFlags & DSSI_NO_FILTER)
        dwFlags |= SCHEMA_NO_FILTER;

    hr = SchemaCache_GetInheritTypes(&m_guidObjectType, dwFlags, ppInheritTypes, pcInheritTypes);

    TraceLeaveResult(hr);
}


STDMETHODIMP
CDSSecurityInfo::PropertySheetPageCallback(HWND hwnd,
                                           UINT uMsg,
                                           SI_PAGE_TYPE uPage)
{
    if (PSPCB_SI_INITDIALOG == uMsg && uPage == SI_PAGE_PERM)
    {
        WaitOnInitThread();

        //
        // HACK ALERT!!!
        //
        // Exchange Platinum is required to hide membership of some groups
        // (distribution lists) for legal reasons. The only way they found
        // to do this is with non-canonical ACLs which look roughly like
        //   Allow Admins access
        //   Deny Everyone access
        //   <normal ACL>
        //
        // Since ACLUI always generates ACLs in NT Canonical Order, we can't
        // allow these funky ACLs to be modified. If we did, the DL's would
        // either become visible or Admins would get locked out.
        //
        if (!(SI_READONLY & m_dwSIFlags))
        {
            DWORD dwAclType = IsSpecificNonCanonicalSD(m_pSD);
            if (ENC_RESULT_NOT_PRESENT != dwAclType)
            {
                // It's a funky ACL so don't allow changes
                m_dwSIFlags |= SI_READONLY;

                // Tell the user what's going on
                MsgPopup(hwnd,
                         MAKEINTRESOURCE(IDS_SPECIAL_ACL_WARNING),
                         MAKEINTRESOURCE(IDS_SPECIAL_SECURITY_TITLE),
                         MB_OK | MB_ICONWARNING | MB_SETFOREGROUND,
                         g_hInstance,
                         m_strDisplayName);

                // S_FALSE suppresses further popups from aclui ("The ACL
                // is not ordered correctly, etc.")
                return S_FALSE;
            }
        }
    }
    return S_OK;
}


DWORD WINAPI
CDSSecurityInfo::InitThread(LPVOID pvThreadData)
{
    CDSSecurityInfo *psi;

    HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
    InterlockedIncrement(&GLOBAL_REFCOUNT);

    psi = (CDSSecurityInfo*)pvThreadData;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::InitThread");
    TraceAssert(psi != NULL);

#if DBG
    DWORD dwTime = GetTickCount();
#endif

    ThreadCoInitialize();

    psi->Init3();

    ThreadCoUninitialize();

#if DBG
    Trace((TEXT("InitThread complete, elapsed time: %d ms"), GetTickCount() - dwTime));
#endif

    TraceLeave();

    InterlockedDecrement(&GLOBAL_REFCOUNT);
    FreeLibraryAndExitThread(hInstThisDll, 0);
    return 0;
}


HRESULT
SetSecurityInfoMask(LPUNKNOWN punk, SECURITY_INFORMATION si)
{
    HRESULT hr = E_INVALIDARG;
    if (punk)
    {
        IADsObjectOptions *pOptions;
        hr = punk->QueryInterface(IID_IADsObjectOptions, (void**)&pOptions);
        if (SUCCEEDED(hr))
        {
            VARIANT var;
            V_VT(&var) = VT_I4;
            V_I4(&var) = si;
            hr = pOptions->SetOption(ADS_OPTION_SECURITY_MASK, var);
            pOptions->Release();
        }
    }
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   CDSSecurityInfo::DSReadObjectSecurity
//
//  Synopsis:   Reads the security descriptor from the specied DS object
//
//  Arguments:  [IN  pszObjectPath] --  ADS path of DS object
//              [IN  SeInfo]        --  Security descriptor parts requested
//              [OUT ppSD]          --  Security descriptor returned here
//              [IN  lpContext]     --  CDSSecurityInfo*
//
//  Notes:      The returned security descriptor must be freed with LocalFree
//
//----------------------------------------------------------------------------
HRESULT WINAPI
CDSSecurityInfo::DSReadObjectSecurity(LPCWSTR /*pszObjectPath*/,
                                      SECURITY_INFORMATION SeInfo,
                                      PSECURITY_DESCRIPTOR *ppSD,
                                      LPARAM lpContext)
{
    HRESULT hr;
    LPWSTR pszSDProperty = (LPWSTR)c_szSDProperty;
    PADS_ATTR_INFO pSDAttributeInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::DSReadObjectSecurity");
    TraceAssert(SeInfo != 0);
    TraceAssert(ppSD != NULL);
    TraceAssert(lpContext != 0);

    *ppSD = NULL;

    CDSSecurityInfo *pThis = reinterpret_cast<CDSSecurityInfo*>(lpContext);
    TraceAssert(pThis != NULL);
    TraceAssert(pThis->m_pDsObject != NULL);

    // Set the SECURITY_INFORMATION mask
    hr = SetSecurityInfoMask(pThis->m_pDsObject, SeInfo);
    FailGracefully(hr, "Unable to set ADS_OPTION_SECURITY_MASK");

    // Read the security descriptor
    hr = pThis->m_pDsObject->GetObjectAttributes(&pszSDProperty,
                                                 1,
                                                 &pSDAttributeInfo,
                                                 &dwAttributesReturned);
    if (SUCCEEDED(hr) && !pSDAttributeInfo)
        hr = E_ACCESSDENIED;    // This happens for SACL if no SecurityPrivilege
    FailGracefully(hr, "Unable to read nTSecurityDescriptor attribute");

    TraceAssert(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->dwADsType);
    TraceAssert(ADSTYPE_NT_SECURITY_DESCRIPTOR == pSDAttributeInfo->pADsValues->dwType);

    *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);
    if (!*ppSD)
        ExitGracefully(hr, E_OUTOFMEMORY, "LocalAlloc failed");

    CopyMemory(*ppSD,
               pSDAttributeInfo->pADsValues->SecurityDescriptor.lpValue,
               pSDAttributeInfo->pADsValues->SecurityDescriptor.dwLength);

exit_gracefully:

    if (pSDAttributeInfo)
        FreeADsMem(pSDAttributeInfo);

    TraceLeaveResult(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CDSSecurityInfo::DSWriteObjectSecurity
//
//  Synopsis:   Writes the security descriptor to the specied DS object
//
//  Arguments:  [IN  pszObjectPath] --  ADS path of DS object
//              [IN  SeInfo]        --  Security descriptor parts provided
//              [IN  pSD]           --  The new security descriptor
//              [IN  lpContext]     --  CDSSecurityInfo*
//
//----------------------------------------------------------------------------
HRESULT WINAPI
CDSSecurityInfo::DSWriteObjectSecurity(LPCWSTR /*pszObjectPath*/,
                                       SECURITY_INFORMATION SeInfo,
                                       PSECURITY_DESCRIPTOR pSD,
                                       LPARAM lpContext)
{
    HRESULT hr;
    ADSVALUE attributeValue;
    ADS_ATTR_INFO attributeInfo;
    DWORD dwAttributesModified;
    DWORD dwSDLength;
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdControl = 0;
    DWORD dwRevision;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::DSWriteObjectSecurity");
    TraceAssert(pSD && IsValidSecurityDescriptor(pSD));
    TraceAssert(SeInfo != 0);
    TraceAssert(lpContext != 0);

    CDSSecurityInfo *pThis = reinterpret_cast<CDSSecurityInfo*>(lpContext);
    TraceAssert(pThis != NULL);
    TraceAssert(pThis->m_pDsObject != NULL);

    // Set the SECURITY_INFORMATION mask
    hr = SetSecurityInfoMask(pThis->m_pDsObject, SeInfo);
    FailGracefully(hr, "Unable to set ADS_OPTION_SECURITY_MASK");

    // Need the total size
    dwSDLength = GetSecurityDescriptorLength(pSD);

    //
    // If necessary, make a self-relative copy of the security descriptor
    //
    GetSecurityDescriptorControl(pSD, &sdControl, &dwRevision);
    if (!(sdControl & SE_SELF_RELATIVE))
    {
        psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, dwSDLength);

        if (psd == NULL ||
            !MakeSelfRelativeSD(pSD, psd, &dwSDLength))
        {
            DWORD dwErr = GetLastError();
            ExitGracefully(hr, HRESULT_FROM_WIN32(dwErr), "Unable to make self-relative SD copy");
        }

        // Point to the self-relative copy
        pSD = psd;
    }

    attributeValue.dwType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeValue.SecurityDescriptor.dwLength = dwSDLength;
    attributeValue.SecurityDescriptor.lpValue = (LPBYTE)pSD;

    attributeInfo.pszAttrName = (LPWSTR)c_szSDProperty;
    attributeInfo.dwControlCode = ADS_ATTR_UPDATE;
    attributeInfo.dwADsType = ADSTYPE_NT_SECURITY_DESCRIPTOR;
    attributeInfo.pADsValues = &attributeValue;
    attributeInfo.dwNumValues = 1;

    // Write the security descriptor
    hr = pThis->m_pDsObject->SetObjectAttributes(&attributeInfo,
                                                 1,
                                                 &dwAttributesModified);
    if (HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION) == hr
        && OWNER_SECURITY_INFORMATION == SeInfo)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_OWNER);
    }

exit_gracefully:

    if (psd != NULL)
        LocalFree(psd);

    TraceLeaveResult(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CheckObjectAccess
//
//  Synopsis:   Checks access to the security descriptor of a DS object.
//              In particular, determines whether the user has READ_CONTROL,
//              WRITE_DAC, WRITE_OWNER, and/or ACCESS_SYSTEM_SECURITY access.
//
//  Arguments:  none
//
//  Return:     DWORD (Access Mask)
//
//  Notes:      The check for READ_CONTROL involves reading the Owner,
//              Group, and DACL.  This security descriptor is saved
//              in m_pSD.
//
//              The checks for WRITE_DAC, WRITE_OWNER, and
//              ACCESS_SYSTEM_SECURITY involve getting sDRightsEffective
//              from the object.
//
//----------------------------------------------------------------------------
DWORD
CDSSecurityInfo::CheckObjectAccess()
{
    DWORD dwAccessGranted = 0;
    HRESULT hr;
    SECURITY_INFORMATION si = 0;
    LPWSTR pProp = (LPWSTR)c_szSDRightsProp;
    PADS_ATTR_INFO pSDRightsInfo = NULL;
    DWORD dwAttributesReturned;

    TraceEnter(TRACE_DSSI, "CDSSecurityInfo::CheckObjectAccess");

#ifdef DSSEC_PRIVATE_DEBUG
    // FOR DEBUGGING ONLY
    // Turn this on to prevent the dialogs from being read-only. This is
    // useful for testing the object picker against NTDEV (for example).
    TraceMsg("Returning all access for debugging");
    dwAccessGranted = (READ_CONTROL | WRITE_OWNER | WRITE_DAC | ACCESS_SYSTEM_SECURITY);
#endif

    // Test for READ_CONTROL by trying to read the Owner, Group, and DACL
    TraceAssert(NULL == m_pSD); // shouldn't get here twice
    TraceAssert(m_strObjectPath != NULL);
    TraceAssert(m_pfnReadSD != NULL);
    hr = (*m_pfnReadSD)(m_strObjectPath,
                        OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                        &m_pSD,
                        m_lpReadContext);
    if (SUCCEEDED(hr))
    {
        TraceAssert(NULL != m_pSD);
        dwAccessGranted |= READ_CONTROL;
    }

    // If we're in read-only mode, there's no need to check anything else.
    if (m_dwInitFlags & DSSI_READ_ONLY)
        TraceLeaveValue(dwAccessGranted);

    // Read the sDRightsEffective property to determine writability
    m_pDsObject->GetObjectAttributes(&pProp,
                                     1,
                                     &pSDRightsInfo,
                                     &dwAttributesReturned);
    if (pSDRightsInfo)
    {
        TraceAssert(ADSTYPE_INTEGER == pSDRightsInfo->dwADsType);
        si = pSDRightsInfo->pADsValues->Integer;
        FreeADsMem(pSDRightsInfo);
    }
    else
    {
        //
        // Note that GetObjectAttributes commonly returns S_OK even when
        // it fails, so the HRESULT is basically useless here.
        //
        // This can fail if we don't have read_property access, which can
        // happen when an admin is trying to restore access to an object
        // that has had all access removed or denied
        //
        // Assume we can write the Owner and DACL. If not, the worst that
        // happens is the user gets an "Access Denied" message when trying
        // to save changes.
        //
        TraceMsg("GetObjectAttributes failed to read sDRightsEffective");
        si = OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    }

    // The resulting SECURITY_INFORMATION mask indicates the
    // security descriptor parts that may be modified by the user.
    Trace((TEXT("sDRightsEffective = 0x%08x"), si));

    if (OWNER_SECURITY_INFORMATION & si)
        dwAccessGranted |= WRITE_OWNER;

    if (DACL_SECURITY_INFORMATION & si)
        dwAccessGranted |= WRITE_DAC;

    if (SACL_SECURITY_INFORMATION & si)
        dwAccessGranted |= ACCESS_SYSTEM_SECURITY;

    TraceLeaveValue(dwAccessGranted);
}


//+---------------------------------------------------------------------------
//
//  Function:   DSCreateISecurityInfoObjectEx
//
//  Synopsis:   Instantiates an ISecurityInfo interface for a DS object
//
//  Arguments:  [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  pwszServer]        --  Name/address of DS DC (optional)
//              [IN  pwszUserName]      --  User name for validation (optional)
//              [IN  pwszPassword]      --  Password for validation (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [OUT ppSI]              --  Interface pointer returned here
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSCreateISecurityInfoObjectEx(LPCWSTR pwszObjectPath,
                              LPCWSTR pwszObjectClass,
                              LPCWSTR pwszServer,
                              LPCWSTR pwszUserName,
                              LPCWSTR pwszPassword,
                              DWORD   dwFlags,
                              LPSECURITYINFO *ppSI,
                              PFNREADOBJECTSECURITY  pfnReadSD,
                              PFNWRITEOBJECTSECURITY pfnWriteSD,
                              LPARAM lpContext)
{
    HRESULT hr;
    DWORD dwErr;
    CDSSecurityInfo* pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateISecurityInfoObjectEx");

    if (pwszObjectPath == NULL || ppSI == NULL)
        TraceLeaveResult(E_INVALIDARG);

    *ppSI = NULL;

    //
    // Create and initialize the ISecurityInformation object.
    //
    pSI = new CDSSecurityInfo();      // ref == 0
    if (!pSI)
        ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create CDSSecurityInfo object");

    pSI->AddRef();                    // ref == 1

    hr = pSI->Init(pwszObjectPath,
                   pwszObjectClass,
                   pwszServer,
                   pwszUserName,
                   pwszPassword,
                   dwFlags,
                   pfnReadSD,
                   pfnWriteSD,
                   lpContext);
    if (FAILED(hr))
    {
        DoRelease(pSI);
    }

    *ppSI = (LPSECURITYINFO)pSI;

exit_gracefully:

    TraceLeaveResult(hr);
}


STDAPI
DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,
                            LPCWSTR pwszObjectClass,
                            DWORD   dwFlags,
                            LPSECURITYINFO *ppSI,
                            PFNREADOBJECTSECURITY  pfnReadSD,
                            PFNWRITEOBJECTSECURITY pfnWriteSD,
                            LPARAM lpContext)
{
    return DSCreateISecurityInfoObjectEx(pwszObjectPath,
                                         pwszObjectClass,
                                         NULL, //pwszServer,
                                         NULL, //pwszUserName,
                                         NULL, //pwszPassword,
                                         dwFlags,
                                         ppSI,
                                         pfnReadSD,
                                         pfnWriteSD,
                                         lpContext);
}


//+---------------------------------------------------------------------------
//
//  Function:   DSCreateSecurityPage
//
//  Synopsis:   Creates a Security property page for a DS object
//
//  Arguments:  [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [OUT phPage]            --  HPROPSHEETPAGE returned here
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSCreateSecurityPage(LPCWSTR pwszObjectPath,
                     LPCWSTR pwszObjectClass,
                     DWORD   dwFlags,
                     HPROPSHEETPAGE *phPage,
                     PFNREADOBJECTSECURITY  pfnReadSD,
                     PFNWRITEOBJECTSECURITY pfnWriteSD,
                     LPARAM lpContext)
{
    HRESULT hr;
    LPSECURITYINFO pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateSecurityPage");

    if (NULL == phPage || NULL == pwszObjectPath || !*pwszObjectPath)
        TraceLeaveResult(E_INVALIDARG);

    *phPage = NULL;

    hr = DSCreateISecurityInfoObject(pwszObjectPath,
                                     pwszObjectClass,
                                     dwFlags,
                                     &pSI,
                                     pfnReadSD,
                                     pfnWriteSD,
                                     lpContext);
    if (SUCCEEDED(hr))
    {
        hr = _CreateSecurityPage(pSI, phPage);
        DoRelease(pSI);
    }

    TraceLeaveResult(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   DSEditSecurity
//
//  Synopsis:   Displays a modal dialog for editing security on a DS object
//
//  Arguments:  [IN  hwndOwner]         --  Dialog owner window
//              [IN  pwszObjectPath]    --  Full ADS path of DS object
//              [IN  pwszObjectClass]   --  Class of the object (optional)
//              [IN  dwFlags]           --  Combination of DSSI_* flags
//              [IN  pwszCaption        --  Optional dialog caption
//              [IN  pfnReadSD]         --  Optional function for reading SD
//              [IN  pfnWriteSD]        --  Optional function for writing SD
//              [IN  lpContext]         --  Passed to pfnReadSD/pfnWriteSD
//
//  Return:     HRESULT
//
//----------------------------------------------------------------------------
STDAPI
DSEditSecurity(HWND hwndOwner,
               LPCWSTR pwszObjectPath,
               LPCWSTR pwszObjectClass,
               DWORD dwFlags,
               LPCWSTR pwszCaption,
               PFNREADOBJECTSECURITY pfnReadSD,
               PFNWRITEOBJECTSECURITY pfnWriteSD,
               LPARAM lpContext)
{
    HRESULT hr;
    LPSECURITYINFO pSI = NULL;

    TraceEnter(TRACE_SECURITY, "DSCreateSecurityPage");

    if (NULL == pwszObjectPath || !*pwszObjectPath)
        TraceLeaveResult(E_INVALIDARG);

    if (pwszCaption && *pwszCaption)
    {
        // Use the provided caption
        HPROPSHEETPAGE hPage = NULL;

        hr = DSCreateSecurityPage(pwszObjectPath,
                                  pwszObjectClass,
                                  dwFlags,
                                  &hPage,
                                  pfnReadSD,
                                  pfnWriteSD,
                                  lpContext);
        if (SUCCEEDED(hr))
        {
            PROPSHEETHEADERW psh;
            psh.dwSize = SIZEOF(psh);
            psh.dwFlags = PSH_DEFAULT;
            psh.hwndParent = hwndOwner;
            psh.hInstance = GLOBAL_HINSTANCE;
            psh.pszCaption = pwszCaption;
            psh.nPages = 1;
            psh.nStartPage = 0;
            psh.phpage = &hPage;

            PropertySheetW(&psh);
        }
    }
    else
    {
        // This method creates a caption like "Permissions for Foo"
        hr = DSCreateISecurityInfoObject(pwszObjectPath,
                                         pwszObjectClass,
                                         dwFlags,
                                         &pSI,
                                         pfnReadSD,
                                         pfnWriteSD,
                                         lpContext);
        if (SUCCEEDED(hr))
        {
            hr = _EditSecurity(hwndOwner, pSI);
            DoRelease(pSI);
        }
    }

    TraceLeaveResult(hr);
}
