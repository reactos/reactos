//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       initacl.cpp
//
//  Contents:   Initialize ACLs so that Everyone only has KEY_READ access
//              for the following REGPATHs:
//                  HKLM\Software\Microsoft\Cryptography\OID ..
//                  HKLM\Software\Microsoft\Cryptography\Providers\Trust ..
//                  HKLM\Software\Microsoft\Cryptography\Services ...
//                  HKLM\Software\Microsoft\SystemCertificates ..
//                  HKLM\Software\Policies\Microsoft\SystemCertificates ..
//                  HKLM\Software\Microsoft\EnterpriseCertificates ..
//
//              Initialize ACLS so that Everyone has KEY_READ and KEY_SET_VALUE
//              access for the following REGPATH:
//                  HKLM\Software\Microsoft\Cryptography\IEDirtyFlags
//
//  Functions:  InitializeHKLMAcls
//
//  Note:       By default HKLM\Software ... gives Everyone special access.
//              Special access includes: KEY_READ, KEY_WRITE, DELETE
//
//  History:    08-May-98    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

#ifdef STATIC
#undef STATIC
#endif
#define STATIC


static const LPCWSTR rgpwszHKLMRegPath[] = {
    OID_REGPATH,
    PROVIDERS_REGPATH,
    SERVICES_REGPATH,
    SYSTEM_STORE_REGPATH,
    GROUP_POLICY_STORE_REGPATH,
    ENTERPRISE_STORE_REGPATH,
};

#define HKLM_REGPATH_CNT    (sizeof(rgpwszHKLMRegPath) / \
                                sizeof(rgpwszHKLMRegPath[0]))


//+-------------------------------------------------------------------------
//  Predefined SIDs allocated once by GetPredefinedSids. Freed when
//  InitializeHKLMAcls() returns
//--------------------------------------------------------------------------
static PSID psidLocalSystem = NULL;
static PSID psidAdministrators = NULL;
static PSID psidEveryone = NULL;

//+-------------------------------------------------------------------------
//  ACL definitions used to set security on the HKLM registry keys
//--------------------------------------------------------------------------
#define HKLM_SYSTEM_ACE_MASK        KEY_ALL_ACCESS
#define HKLM_ADMIN_ACE_MASK         KEY_ALL_ACCESS
#define HKLM_EVERYONE_ACE_MASK      KEY_READ
#define HKLM_ACE_FLAGS              CONTAINER_INHERIT_ACE

#define HKLM_ACE_COUNT              3
#define HKLM_SYSTEM_ACE_INDEX       0
#define HKLM_ADMIN_ACE_INDEX        1
#define HKLM_EVERYONE_ACE_INDEX     2

//+-------------------------------------------------------------------------
//  Maximum allowed access rights for Everyone in HKLM
//--------------------------------------------------------------------------
#define MAX_HKLM_EVERYONE_ACE_MASK  (KEY_READ | GENERIC_READ)

//+-------------------------------------------------------------------------
//  Access rights for Everyone on the CERT_IE_DIRTY_FLAGS registry SubKey
//--------------------------------------------------------------------------
#define IE_EVERYONE_ACE_MASK        (KEY_READ | KEY_SET_VALUE)


//+-------------------------------------------------------------------------
//  Allocate/free predefined SIDs
//--------------------------------------------------------------------------
STATIC BOOL GetPredefinedSids()
{
    BOOL fResult;
    SID_IDENTIFIER_AUTHORITY siaNtAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siaWorldSidAuthority =
        SECURITY_WORLD_SID_AUTHORITY;

    if (!AllocateAndInitializeSid(
            &siaNtAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidLocalSystem
            )) 
        goto AllocateAndInitializeSidError;

    if (!AllocateAndInitializeSid(
            &siaNtAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            &psidAdministrators
            ))
        goto AllocateAndInitializeSidError;

    if (!AllocateAndInitializeSid(
            &siaWorldSidAuthority,
            1,
            SECURITY_WORLD_RID,
            0, 0, 0, 0, 0, 0, 0,
            &psidEveryone
            ))
        goto AllocateAndInitializeSidError;

    fResult = TRUE;
CommonReturn:
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(AllocateAndInitializeSidError)
}

STATIC void FreePredefinedSids()
{
    FreeSid(psidLocalSystem);
    FreeSid(psidAdministrators);
    FreeSid(psidEveryone);

}

#define HKLM_SD_LEN         0x1000

//+-------------------------------------------------------------------------
//  Allocate and get the security descriptor information for the specified
//  registry key.
//--------------------------------------------------------------------------
STATIC PSECURITY_DESCRIPTOR AllocAndGetSecurityDescriptor(
    IN HKEY hKey,
    SECURITY_INFORMATION SecInf
    )
{
    LONG err;
    PSECURITY_DESCRIPTOR psd = NULL;
    DWORD cbsd;

    cbsd = HKLM_SD_LEN;
    if (NULL == (psd = (PSECURITY_DESCRIPTOR) PkiNonzeroAlloc(cbsd)))
        goto OutOfMemory;

    err = RegGetKeySecurity(
            hKey,
            SecInf,
            psd,
            &cbsd
            );
    if (ERROR_SUCCESS == err)
        goto CommonReturn;
    if (ERROR_INSUFFICIENT_BUFFER != err)
        goto RegGetKeySecurityError;

    if (0 == cbsd)
        goto NoSecurityDescriptor;

    PkiFree(psd);
    if (NULL == (psd = (PSECURITY_DESCRIPTOR) PkiNonzeroAlloc(cbsd)))
        goto OutOfMemory;

    if (ERROR_SUCCESS != (err = RegGetKeySecurity(
            hKey,
            SecInf,
            psd,
            &cbsd
            ))) goto RegGetKeySecurityError;

CommonReturn:
    return psd;
ErrorReturn:
    PkiFree(psd);
    psd = NULL;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
SET_ERROR_VAR(RegGetKeySecurityError, err)
SET_ERROR(NoSecurityDescriptor, ERROR_INVALID_SECURITY_DESCR)
}


//+-------------------------------------------------------------------------
//  Checks that "Everyone" doesn't have more than KEY_READ or GENERIC_READ
//  access rights. If valid, returns TRUE.
//--------------------------------------------------------------------------
STATIC BOOL IsValidHKLMAccessRights(
    IN HKEY hKey
    )
{
    BOOL fResult;
    PSECURITY_DESCRIPTOR psd = NULL;
    BOOL fDaclPresent;
    PACL pAcl;                      // not allocated
    BOOL fDaclDefaulted;
    DWORD dwAceIndex;

    if (NULL == (psd = AllocAndGetSecurityDescriptor(
            hKey,
            DACL_SECURITY_INFORMATION
            ))) goto GetSecurityDescriptorError;

    if (!GetSecurityDescriptorDacl(psd, &fDaclPresent, &pAcl,
            &fDaclDefaulted))
        goto GetSecurityDescriptorDaclError;
    if (!fDaclPresent || NULL == pAcl || 0 == pAcl->AceCount)
        goto MissingDaclError;

    for (dwAceIndex = 0; dwAceIndex < pAcl->AceCount; dwAceIndex++) {
        PACCESS_ALLOWED_ACE pAce;

        if (!GetAce(pAcl, dwAceIndex, (void **) &pAce))
            goto GetAceError;

        if (ACCESS_ALLOWED_ACE_TYPE != pAce->Header.AceType)
            continue;
        if (!EqualSid(psidEveryone, (PSID) &pAce->SidStart))
            continue;

        if (0 != (pAce->Mask & ~MAX_HKLM_EVERYONE_ACE_MASK))
            goto InvalidEveryoneAccess;
    }

    fResult = TRUE;
CommonReturn:
    PkiFree(psd);
    return fResult;
InvalidEveryoneAccess:
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(GetSecurityDescriptorError)
TRACE_ERROR(GetSecurityDescriptorDaclError)
SET_ERROR(MissingDaclError, ERROR_INVALID_ACL)
TRACE_ERROR(GetAceError)
}


//+-------------------------------------------------------------------------
//  Create the SecurityDescriptor to be used for HKLM SubKeys
//--------------------------------------------------------------------------
STATIC BOOL CreateHKLMSecurityDescriptor(
    IN ACCESS_MASK EveryoneAccessMask,
    OUT PSECURITY_DESCRIPTOR psd,
    OUT PACL *ppDacl
    )
{
    BOOL fResult;
    PACL pDacl = NULL;
    PACCESS_ALLOWED_ACE pAce;
    DWORD dwAclSize;
    DWORD i;

    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION))
        goto InitializeSecurityDescriptorError;


    // Set DACL

    //
    // compute size of ACL
    //
    dwAclSize = sizeof(ACL) +
        HKLM_ACE_COUNT * ( sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) ) +
        GetLengthSid(psidLocalSystem) +
        GetLengthSid(psidAdministrators) +
        GetLengthSid(psidEveryone)
        ;

    //
    // allocate storage for Acl
    //
    if (NULL == (pDacl = (PACL) PkiNonzeroAlloc(dwAclSize)))
        goto OutOfMemory;

    if (!InitializeAcl(pDacl, dwAclSize, ACL_REVISION))
        goto InitializeAclError;

    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            HKLM_SYSTEM_ACE_MASK,
            psidLocalSystem
            ))
        goto AddAceError;
    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            HKLM_ADMIN_ACE_MASK,
            psidAdministrators
            ))
        goto AddAceError;
    if (!AddAccessAllowedAce(
            pDacl,
            ACL_REVISION,
            EveryoneAccessMask,
            psidEveryone
            ))
        goto AddAceError;

    //
    // make containers inherit.
    //
    for (i = 0; i < HKLM_ACE_COUNT; i++) {
        if(!GetAce(pDacl, i, (void **) &pAce))
            goto GetAceError;
        pAce->Header.AceFlags = HKLM_ACE_FLAGS;
    }

    if (!SetSecurityDescriptorDacl(psd, TRUE, pDacl, FALSE))
        goto SetSecurityDescriptorDaclError;

    fResult = TRUE;
CommonReturn:
    *ppDacl = pDacl;
    return fResult;
ErrorReturn:
    PkiFree(pDacl);
    pDacl = NULL;
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(InitializeSecurityDescriptorError)
TRACE_ERROR(OutOfMemory)
TRACE_ERROR(InitializeAclError)
TRACE_ERROR(AddAceError)
TRACE_ERROR(GetAceError)
TRACE_ERROR(SetSecurityDescriptorDaclError)
}

//+-------------------------------------------------------------------------
//  Set the DACL for the SubKey
//--------------------------------------------------------------------------
STATIC BOOL SetHKLMDacl(
    IN HKEY hKey,
    IN PSECURITY_DESCRIPTOR psd
    )
{
    BOOL fResult;
    LONG err;

    if (ERROR_SUCCESS != (err = RegSetKeySecurity(
            hKey,
            DACL_SECURITY_INFORMATION,
            psd
            )))
        goto RegSetKeySecurityError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR_VAR(RegSetKeySecurityError, err)
}

STATIC BOOL GetSubKeyInfo(
    IN HKEY hKey,
    OUT OPTIONAL DWORD *pcSubKeys,
    OUT OPTIONAL DWORD *pcchMaxSubKey = NULL
    )
{
    BOOL fResult;
    LONG err;
    if (ERROR_SUCCESS != (err = RegQueryInfoKeyU(
            hKey,
            NULL,       // lpszClass
            NULL,       // lpcchClass
            NULL,       // lpdwReserved
            pcSubKeys,
            pcchMaxSubKey,
            NULL,       // lpcchMaxClass
            NULL,       // lpcValues
            NULL,       // lpcchMaxValuesName
            NULL,       // lpcbMaxValueData
            NULL,       // lpcbSecurityDescriptor
            NULL        // lpftLastWriteTime
            ))) goto RegQueryInfoKeyError;
    fResult = TRUE;

CommonReturn:
    // For Win95 Remote Registry Access:: returns half of the cch
    if (pcchMaxSubKey && *pcchMaxSubKey)
        *pcchMaxSubKey = (*pcchMaxSubKey + 1) * 2 + 2;
    return fResult;
ErrorReturn:
    fResult = FALSE;
    if (pcSubKeys)
        *pcSubKeys = 0;
    if (pcchMaxSubKey)
        *pcchMaxSubKey = 0;
    goto CommonReturn;
SET_ERROR_VAR(RegQueryInfoKeyError, err)
}

//+-------------------------------------------------------------------------
//  Check the HKEY for valid access rights for Everyone. If not valid, set
//  the HKEY's DACL. Enumerate the HKEY's SubKeys and recursively call.
//--------------------------------------------------------------------------
STATIC BOOL RecursiveInitializeHKLMSubKeyAcls(
    IN HKEY hKey,
    IN PSECURITY_DESCRIPTOR psd
    )
{
    BOOL fResult = TRUE;
    DWORD cSubKeys;
    DWORD cchMaxSubKey;
    LPWSTR pwszSubKey = NULL;

    if (!IsValidHKLMAccessRights(hKey))
        fResult &= SetHKLMDacl(hKey, psd);

    if (!GetSubKeyInfo(
            hKey,
            &cSubKeys,
            &cchMaxSubKey
            ))
        return FALSE;

    if (cSubKeys && cchMaxSubKey) {
        DWORD i;

        cchMaxSubKey++;
        if (NULL == (pwszSubKey = (LPWSTR) PkiNonzeroAlloc(
                cchMaxSubKey * sizeof(WCHAR))))
            goto OutOfMemory;

        for (i = 0; i < cSubKeys; i++) {
            DWORD cchSubKey = cchMaxSubKey;
            LONG err;
            HKEY hSubKey;

            if (ERROR_SUCCESS != (err = RegEnumKeyExU(
                    hKey,
                    i,
                    pwszSubKey,
                    &cchSubKey,
                    NULL,               // lpdwReserved
                    NULL,               // lpszClass
                    NULL,               // lpcchClass
                    NULL                // lpftLastWriteTime
                    )) || 0 == cchSubKey ||
                            L'\0' == *pwszSubKey)
                continue;

            if (ERROR_SUCCESS != (err = RegOpenKeyExU(
                    hKey,
                    pwszSubKey,
                    0,                      // dwReserved
                    KEY_READ | WRITE_DAC,
                    &hSubKey))) {
#if DBG
                DbgPrintf(DBG_SS_CRYPT32,
                    "RegOpenKeyEx(%S) returned error: %d 0x%x\n",
                    pwszSubKey, err, err);
#endif
            } else {
                fResult &= RecursiveInitializeHKLMSubKeyAcls(hSubKey, psd);
                RegCloseKey(hSubKey);
            }
        }
    }

CommonReturn:
    PkiFree(pwszSubKey);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  Initialize the HKLM registry used by crypt32 so that Everyone only has
//  KEY_READ access rights.
//
//  Initialize the IEDirtyFlags registry key so that Everyone has KEY_READ
//  and KEY_SET_VALUE access rights.
//--------------------------------------------------------------------------
BOOL
InitializeHKLMAcls()
{
    BOOL fResult = TRUE;
    SECURITY_DESCRIPTOR sd;
    PACL pDacl = NULL;
    SECURITY_ATTRIBUTES SecAttr;

    HKEY hKey;
    LONG err;
    DWORD dwDisposition;
    DWORD i;

    if (!FIsWinNT())
        return TRUE;

    if (!GetPredefinedSids())
        return FALSE;

    if (!CreateHKLMSecurityDescriptor(
            HKLM_EVERYONE_ACE_MASK,
            &sd,
            &pDacl
            ))
        goto ErrorReturn;

    memset(&SecAttr, 0, sizeof(SecAttr));
    SecAttr.nLength = sizeof(SecAttr);
    SecAttr.lpSecurityDescriptor = (LPVOID) &sd;
    SecAttr.bInheritHandle = FALSE;


    // Iterate through the HKLM registry locations used by crypt32. If
    // the registry key doesn't exist, create it and give Everyone READ_KEY
    // access. Otherwise, recurse through its SubKeys.  For SubKeys having
    // more than KEY_READ access rights for Everyone, set their ACLs giving
    // only KEY_READ access to Everyone.

    for (i = 0; i < HKLM_REGPATH_CNT; i++) {
        if (ERROR_SUCCESS != (err = RegCreateKeyExU(
                HKEY_LOCAL_MACHINE,
                rgpwszHKLMRegPath[i],
                0,                      // dwReserved
                NULL,                   // lpClass
                REG_OPTION_NON_VOLATILE,
                MAXIMUM_ALLOWED,
                &SecAttr,
                &hKey,
                &dwDisposition))) {
#if DBG
            DbgPrintf(DBG_SS_CRYPT32,
                "RegCreateKeyEx(HKLM\\%S) returned error: %d 0x%x\n",
                rgpwszHKLMRegPath[i], err, err);
#endif
            fResult = FALSE;
            continue;
        }

        if (REG_CREATED_NEW_KEY != dwDisposition)
            fResult &= RecursiveInitializeHKLMSubKeyAcls(hKey, &sd);

        RegCloseKey(hKey);
    }
    PkiFree(pDacl);

    // Allow Everyone to have KEY_READ and KEY_SET_VALUE access to
    // the IEDirtyFlags registry key
    if (!CreateHKLMSecurityDescriptor(
            IE_EVERYONE_ACE_MASK,
            &sd,
            &pDacl
            ))
        goto ErrorReturn;

    if (ERROR_SUCCESS != (err = RegCreateKeyExU(
            HKEY_LOCAL_MACHINE,
            CERT_IE_DIRTY_FLAGS_REGPATH,
            0,                      // dwReserved
            NULL,                   // lpClass
            REG_OPTION_NON_VOLATILE,
            MAXIMUM_ALLOWED,
            &SecAttr,
            &hKey,
            &dwDisposition))) {
#if DBG
        DbgPrintf(DBG_SS_CRYPT32,
            "RegCreateKeyEx(HKLM\\%S) returned error: %d 0x%x\n",
            CERT_IE_DIRTY_FLAGS_REGPATH, err, err);
#endif
        fResult = FALSE;
    } else {
        if (REG_CREATED_NEW_KEY != dwDisposition)
            fResult &= SetHKLMDacl(hKey, &sd);
        RegCloseKey(hKey);
    }
    PkiFree(pDacl);

CommonReturn:
    FreePredefinedSids();
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}
