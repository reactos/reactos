/*
 * Register related wintrust functions
 *
 * Copyright 2006 Paul Vriens
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "winreg.h"
#include "winnls.h"
#include "objbase.h"

#include "guiddef.h"
#include "wintrust.h"
#include "softpub.h"
#include "mssip.h"
#include "wintrust_priv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wintrust);

static CRYPT_TRUST_REG_ENTRY SoftpubInitialization;
static CRYPT_TRUST_REG_ENTRY SoftpubMessage;
static CRYPT_TRUST_REG_ENTRY SoftpubSignature;
static CRYPT_TRUST_REG_ENTRY SoftpubCertificate;
static CRYPT_TRUST_REG_ENTRY SoftpubCertCheck;
static CRYPT_TRUST_REG_ENTRY SoftpubFinalPolicy;
static CRYPT_TRUST_REG_ENTRY SoftpubCleanup;

static CRYPT_TRUST_REG_ENTRY SoftpubDefCertInit;

static CRYPT_TRUST_REG_ENTRY SoftpubDumpStructure;

static CRYPT_TRUST_REG_ENTRY HTTPSCertificateTrust;
static CRYPT_TRUST_REG_ENTRY HTTPSFinalProv;

static CRYPT_TRUST_REG_ENTRY OfficeInitializePolicy;
static CRYPT_TRUST_REG_ENTRY OfficeCleanupPolicy;

static CRYPT_TRUST_REG_ENTRY DriverInitializePolicy;
static CRYPT_TRUST_REG_ENTRY DriverFinalPolicy;
static CRYPT_TRUST_REG_ENTRY DriverCleanupPolicy;

static CRYPT_TRUST_REG_ENTRY GenericChainCertificateTrust;
static CRYPT_TRUST_REG_ENTRY GenericChainFinalProv;

static const CRYPT_TRUST_REG_ENTRY NullCTRE = { 0, NULL, NULL };

static const WCHAR Trust[]            = {'S','o','f','t','w','a','r','e','\\',
                                         'M','i','c','r','o','s','o','f','t','\\',
                                         'C','r','y','p','t','o','g','r','a','p','h','y','\\',
                                         'P','r','o','v','i','d','e','r','s','\\',
                                         'T','r','u','s','t','\\', 0 };

static const WCHAR Initialization[]   = {'I','n','i','t','i','a','l','i','z','a','t','i','o','n','\\', 0};
static const WCHAR Message[]          = {'M','e','s','s','a','g','e','\\', 0};
static const WCHAR Signature[]        = {'S','i','g','n','a','t','u','r','e','\\', 0};
static const WCHAR Certificate[]      = {'C','e','r','t','i','f','i','c','a','t','e','\\', 0};
static const WCHAR CertCheck[]        = {'C','e','r','t','C','h','e','c','k','\\', 0};
static const WCHAR FinalPolicy[]      = {'F','i','n','a','l','P','o','l','i','c','y','\\', 0};
static const WCHAR DiagnosticPolicy[] = {'D','i','a','g','n','o','s','t','i','c','P','o','l','i','c','y','\\', 0};
static const WCHAR Cleanup[]          = {'C','l','e','a','n','u','p','\\', 0};

static const WCHAR DefaultId[]        = {'D','e','f','a','u','l','t','I','d', 0};
static const WCHAR Dll[]              = {'$','D','L','L', 0};

/***********************************************************************
 *              WINTRUST_InitRegStructs
 *
 * Helper function to allocate and initialize the members of the
 * CRYPT_TRUST_REG_ENTRY structs.
 */
static void WINTRUST_InitRegStructs(void)
{
#define WINTRUST_INITREGENTRY( action, dllname, functionname ) \
    action.cbStruct = sizeof(CRYPT_TRUST_REG_ENTRY); \
    action.pwszDLLName = wcsdup(dllname); \
    action.pwszFunctionName = wcsdup(functionname);

    WINTRUST_INITREGENTRY(SoftpubInitialization, SP_POLICY_PROVIDER_DLL_NAME, SP_INIT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubMessage, SP_POLICY_PROVIDER_DLL_NAME, SP_OBJTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubSignature, SP_POLICY_PROVIDER_DLL_NAME, SP_SIGTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCertificate, SP_POLICY_PROVIDER_DLL_NAME, WT_PROVIDER_CERTTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCertCheck, SP_POLICY_PROVIDER_DLL_NAME, SP_CHKCERT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubFinalPolicy, SP_POLICY_PROVIDER_DLL_NAME, SP_FINALPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubCleanup, SP_POLICY_PROVIDER_DLL_NAME, SP_CLEANUPPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubDefCertInit, SP_POLICY_PROVIDER_DLL_NAME, SP_GENERIC_CERT_INIT_FUNCTION)
    WINTRUST_INITREGENTRY(SoftpubDumpStructure, SP_POLICY_PROVIDER_DLL_NAME, SP_TESTDUMPPOLICY_FUNCTION_TEST)
    WINTRUST_INITREGENTRY(HTTPSCertificateTrust, SP_POLICY_PROVIDER_DLL_NAME, HTTPS_CERTTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(HTTPSFinalProv, SP_POLICY_PROVIDER_DLL_NAME, HTTPS_FINALPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(OfficeInitializePolicy, OFFICE_POLICY_PROVIDER_DLL_NAME, OFFICE_INITPROV_FUNCTION)
    WINTRUST_INITREGENTRY(OfficeCleanupPolicy, OFFICE_POLICY_PROVIDER_DLL_NAME, OFFICE_CLEANUPPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(DriverInitializePolicy, SP_POLICY_PROVIDER_DLL_NAME, DRIVER_INITPROV_FUNCTION)
    WINTRUST_INITREGENTRY(DriverFinalPolicy, SP_POLICY_PROVIDER_DLL_NAME, DRIVER_FINALPOLPROV_FUNCTION)
    WINTRUST_INITREGENTRY(DriverCleanupPolicy, SP_POLICY_PROVIDER_DLL_NAME, DRIVER_CLEANUPPOLICY_FUNCTION)
    WINTRUST_INITREGENTRY(GenericChainCertificateTrust, SP_POLICY_PROVIDER_DLL_NAME, GENERIC_CHAIN_CERTTRUST_FUNCTION)
    WINTRUST_INITREGENTRY(GenericChainFinalProv, SP_POLICY_PROVIDER_DLL_NAME, GENERIC_CHAIN_FINALPOLICY_FUNCTION)

#undef WINTRUST_INITREGENTRY
}

/***********************************************************************
 *              WINTRUST_FreeRegStructs
 *
 * Helper function to free 2 members of the CRYPT_TRUST_REG_ENTRY
 * structs.
 */
static void WINTRUST_FreeRegStructs(void)
{
#define WINTRUST_FREEREGENTRY( action ) \
    free(action.pwszDLLName); \
    free(action.pwszFunctionName);

    WINTRUST_FREEREGENTRY(SoftpubInitialization);
    WINTRUST_FREEREGENTRY(SoftpubMessage);
    WINTRUST_FREEREGENTRY(SoftpubSignature);
    WINTRUST_FREEREGENTRY(SoftpubCertificate);
    WINTRUST_FREEREGENTRY(SoftpubCertCheck);
    WINTRUST_FREEREGENTRY(SoftpubFinalPolicy);
    WINTRUST_FREEREGENTRY(SoftpubCleanup);
    WINTRUST_FREEREGENTRY(SoftpubDefCertInit);
    WINTRUST_FREEREGENTRY(SoftpubDumpStructure);
    WINTRUST_FREEREGENTRY(HTTPSCertificateTrust);
    WINTRUST_FREEREGENTRY(HTTPSFinalProv);
    WINTRUST_FREEREGENTRY(OfficeInitializePolicy);
    WINTRUST_FREEREGENTRY(OfficeCleanupPolicy);
    WINTRUST_FREEREGENTRY(DriverInitializePolicy);
    WINTRUST_FREEREGENTRY(DriverFinalPolicy);
    WINTRUST_FREEREGENTRY(DriverCleanupPolicy);
    WINTRUST_FREEREGENTRY(GenericChainCertificateTrust);
    WINTRUST_FREEREGENTRY(GenericChainFinalProv);

#undef WINTRUST_FREEREGENTRY
}

/***********************************************************************
 *              WINTRUST_guid2wstr
 *
 * Create a wide-string from a GUID
 *
 */
static void WINTRUST_Guid2Wstr(const GUID* pgActionID, WCHAR* GuidString)
{ 
    static const WCHAR wszFormat[] = {'{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
                                      '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','%','0','2','X','%','0','2',
                                      'X','%','0','2','X','%','0','2','X','}', 0};

    wsprintfW(GuidString, wszFormat, pgActionID->Data1, pgActionID->Data2, pgActionID->Data3,
        pgActionID->Data4[0], pgActionID->Data4[1], pgActionID->Data4[2], pgActionID->Data4[3],
        pgActionID->Data4[4], pgActionID->Data4[5], pgActionID->Data4[6], pgActionID->Data4[7]);
}

/***********************************************************************
 *              WINTRUST_WriteProviderToReg
 *
 * Helper function for WintrustAddActionID
 *
 */
static LONG WINTRUST_WriteProviderToReg(WCHAR* GuidString,
                                        const WCHAR* FunctionType,
                                        CRYPT_TRUST_REG_ENTRY RegEntry)
{
    static const WCHAR Function[] = {'$','F','u','n','c','t','i','o','n', 0};
    WCHAR ProvKey[MAX_PATH];
    HKEY Key;
    LONG Res = ERROR_SUCCESS;

    /* Create the needed key string */
    ProvKey[0]='\0';
    lstrcatW(ProvKey, Trust);
    lstrcatW(ProvKey, FunctionType);
    lstrcatW(ProvKey, GuidString);

    if (!RegEntry.pwszDLLName || !RegEntry.pwszFunctionName)
        return ERROR_INVALID_PARAMETER;

    Res = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ProvKey, 0, NULL, 0, KEY_WRITE, NULL, &Key, NULL);
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Create the $DLL entry */
    Res = RegSetValueExW(Key, Dll, 0, REG_SZ, (BYTE*)RegEntry.pwszDLLName,
        (lstrlenW(RegEntry.pwszDLLName) + 1)*sizeof(WCHAR));
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Create the $Function entry */
    Res = RegSetValueExW(Key, Function, 0, REG_SZ, (BYTE*)RegEntry.pwszFunctionName,
        (lstrlenW(RegEntry.pwszFunctionName) + 1)*sizeof(WCHAR));

error_close_key:
    RegCloseKey(Key);

    return Res;
}

/***********************************************************************
 *		WintrustAddActionID (WINTRUST.@)
 *
 * Add the definitions of the actions a Trust provider can perform to
 * the registry.
 *
 * PARAMS
 *   pgActionID [I] Pointer to a GUID for the Trust provider.
 *   fdwFlags   [I] Flag to indicate whether registry errors are passed on.
 *   psProvInfo [I] Pointer to a structure with information about DLL
 *                  name and functions.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE. (Use GetLastError() for more information)
 *
 * NOTES
 *   Adding definitions is basically only adding relevant information
 *   to the registry. No verification takes place whether a DLL or its
 *   entrypoints exist.
 *   Information in the registry will always be overwritten.
 *
 */
BOOL WINAPI WintrustAddActionID( GUID* pgActionID, DWORD fdwFlags,
                                 CRYPT_REGISTER_ACTIONID* psProvInfo)
{
    WCHAR GuidString[39];
    LONG Res;
    LONG WriteActionError = ERROR_SUCCESS;

    TRACE("%s %lx %p\n", debugstr_guid(pgActionID), fdwFlags, psProvInfo);

    /* Some sanity checks.
     * We use the W2K3 last error as it makes more sense (W2K leaves the last error
     * as is).
     */
    if (!pgActionID ||
        !psProvInfo ||
        (psProvInfo->cbStruct != sizeof(CRYPT_REGISTER_ACTIONID)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Create this string only once, instead of in the helper function */
    WINTRUST_Guid2Wstr( pgActionID, GuidString);

    /* Write the information to the registry */
    Res = WINTRUST_WriteProviderToReg(GuidString, Initialization  , psProvInfo->sInitProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Message         , psProvInfo->sObjectProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Signature       , psProvInfo->sSignatureProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Certificate     , psProvInfo->sCertificateProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, CertCheck       , psProvInfo->sCertificatePolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, FinalPolicy     , psProvInfo->sFinalPolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, DiagnosticPolicy, psProvInfo->sTestPolicyProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;
    Res = WINTRUST_WriteProviderToReg(GuidString, Cleanup         , psProvInfo->sCleanupProvider);
    if (Res != ERROR_SUCCESS) WriteActionError = Res;

    /* Testing (by restricting access to the registry for some keys) shows that the last failing function
     * will be used for last error.
     * If the flag WT_ADD_ACTION_ID_RET_RESULT_FLAG is set and there are errors when adding the action
     * we have to return FALSE. Errors includes both invalid entries as well as registry errors.
     * Testing also showed that one error doesn't stop the registry writes. Every action will be dealt with.
     */

    if (WriteActionError != ERROR_SUCCESS)
    {
        SetLastError(WriteActionError);

        if (fdwFlags == WT_ADD_ACTION_ID_RET_RESULT_FLAG)
            return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *              WINTRUST_RemoveProviderFromReg
 *
 * Helper function for WintrustRemoveActionID
 *
 */
static void WINTRUST_RemoveProviderFromReg(WCHAR* GuidString,
                                           const WCHAR* FunctionType)
{
    WCHAR ProvKey[MAX_PATH];

    /* Create the needed key string */
    ProvKey[0]='\0';
    lstrcatW(ProvKey, Trust);
    lstrcatW(ProvKey, FunctionType);
    lstrcatW(ProvKey, GuidString);

    /* We don't care about success or failure */
    RegDeleteKeyW(HKEY_LOCAL_MACHINE, ProvKey);
}

/***********************************************************************
 *              WintrustRemoveActionID (WINTRUST.@)
 *
 * Remove the definitions of the actions a Trust provider can perform
 * from the registry.
 *
 * PARAMS
 *   pgActionID [I] Pointer to a GUID for the Trust provider.
 *
 * RETURNS
 *   Success: TRUE. (Use GetLastError() for more information)
 *   Failure: FALSE. (Use GetLastError() for more information)
 *
 * NOTES
 *   Testing shows that WintrustRemoveActionID always returns TRUE and
 *   that a possible error should be retrieved via GetLastError().
 *   There are no checks if the definitions are in the registry.
 */
BOOL WINAPI WintrustRemoveActionID( GUID* pgActionID )
{
    WCHAR GuidString[39];

    TRACE("(%s)\n", debugstr_guid(pgActionID));
 
    if (!pgActionID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return TRUE;
    }

    /* Create this string only once, instead of in the helper function */
    WINTRUST_Guid2Wstr( pgActionID, GuidString);

    /* We don't care about success or failure */
    WINTRUST_RemoveProviderFromReg(GuidString, Initialization);
    WINTRUST_RemoveProviderFromReg(GuidString, Message);
    WINTRUST_RemoveProviderFromReg(GuidString, Signature);
    WINTRUST_RemoveProviderFromReg(GuidString, Certificate);
    WINTRUST_RemoveProviderFromReg(GuidString, CertCheck);
    WINTRUST_RemoveProviderFromReg(GuidString, FinalPolicy);
    WINTRUST_RemoveProviderFromReg(GuidString, DiagnosticPolicy);
    WINTRUST_RemoveProviderFromReg(GuidString, Cleanup);

    return TRUE;
}

/***********************************************************************
 *              WINTRUST_WriteSingleUsageEntry
 *
 * Helper for WintrustAddDefaultForUsage, writes a single value and its
 * data to:
 *
 * HKLM\Software\Microsoft\Cryptography\Trust\Usages\<OID>
 */
static LONG WINTRUST_WriteSingleUsageEntry(LPCSTR OID,
                                           const WCHAR* Value,
                                           WCHAR* Data)
{
    static const WCHAR Usages[] = {'U','s','a','g','e','s','\\', 0};
    WCHAR* UsageKey;
    HKEY Key;
    LONG Res = ERROR_SUCCESS;
    WCHAR* OIDW;
    DWORD Len;

    /* Turn OID into a wide-character string */
    Len = MultiByteToWideChar( CP_ACP, 0, OID, -1, NULL, 0 );
    OIDW = malloc( Len * sizeof(WCHAR) );
    MultiByteToWideChar( CP_ACP, 0, OID, -1, OIDW, Len );

    /* Allocate the needed space for UsageKey */
    UsageKey = malloc((wcslen(Trust) + wcslen(Usages) + Len) * sizeof(WCHAR));
    /* Create the key string */
    lstrcpyW(UsageKey, Trust);
    lstrcatW(UsageKey, Usages);
    lstrcatW(UsageKey, OIDW);

    Res = RegCreateKeyExW(HKEY_LOCAL_MACHINE, UsageKey, 0, NULL, 0, KEY_WRITE, NULL, &Key, NULL);
    if (Res == ERROR_SUCCESS)
    {
        /* Create the Value entry */
        Res = RegSetValueExW(Key, Value, 0, REG_SZ, (BYTE*)Data,
                             (lstrlenW(Data) + 1)*sizeof(WCHAR));
    }
    RegCloseKey(Key);

    free(OIDW);
    free(UsageKey);

    return Res;
}

/***************************************************************************
 *              WINTRUST_RegisterGenVerifyV2
 *
 * Register WINTRUST_ACTION_GENERIC_VERIFY_V2 actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_GENERIC_VERIFY_V2 ({00AAC56B-CD44-11D0-8CC2-00C04FC295EE}
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterGenVerifyV2(void)
{
    BOOL RegisteredOK = TRUE;
    static GUID ProvGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    CRYPT_REGISTER_ACTIONID ProvInfo;
    CRYPT_PROVIDER_REGDEFUSAGE DefUsage = { sizeof(CRYPT_PROVIDER_REGDEFUSAGE),
                                            &ProvGUID,
                                            NULL,   /* No Dll provided */
                                            NULL,   /* No load callback function */
                                            NULL }; /* No free callback function */

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddDefaultForUsage(szOID_PKIX_KP_CODE_SIGNING, &DefUsage))
        RegisteredOK = FALSE;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        RegisteredOK = FALSE;

    return RegisteredOK;
}

/***************************************************************************
 *              WINTRUST_RegisterPublishedSoftware
 *
 * Register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE actions and usages.
 *
 * NOTES
 *   WIN_SPUB_ACTION_PUBLISHED_SOFTWARE ({64B9D180-8DA2-11CF-8736-00AA00A485EB})
 *   is defined in wintrust.h
 */
static BOOL WINTRUST_RegisterPublishedSoftware(void)
{
    static GUID ProvGUID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

#define WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI { 0xc6b2e8d0, 0xe005, 0x11cf, { 0xa1,0x34,0x00,0xc0,0x4f,0xd7,0xbf,0x43 }}

/***************************************************************************
 *              WINTRUST_RegisterPublishedSoftwareNoBadUi
 *
 * Register WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI actions and usages.
 *
 * NOTES
 *   WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI ({C6B2E8D0-E005-11CF-A134-00C04FD7BF43})
 *   is not defined in any include file. (FIXME: Find out if the name is correct).
 */
static BOOL WINTRUST_RegisterPublishedSoftwareNoBadUi(void)
{
    static GUID ProvGUID = WIN_SPUB_ACTION_PUBLISHED_SOFTWARE_NOBADUI;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***************************************************************************
 *              WINTRUST_RegisterGenCertVerify
 *
 * Register WINTRUST_ACTION_GENERIC_CERT_VERIFY actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_GENERIC_CERT_VERIFY ({189A3842-3041-11D1-85E1-00C04FC295EE})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterGenCertVerify(void)
{
    static GUID ProvGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubDefCertInit;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***************************************************************************
 *              WINTRUST_RegisterTrustProviderTest
 *
 * Register WINTRUST_ACTION_TRUSTPROVIDER_TEST actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_TRUSTPROVIDER_TEST ({573E31F8-DDBA-11D0-8CCB-00C04FC295EE})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterTrustProviderTest(void)
{
    static GUID ProvGUID = WINTRUST_ACTION_TRUSTPROVIDER_TEST;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = SoftpubDumpStructure;
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***************************************************************************
 *              WINTRUST_RegisterHttpsProv
 *
 * Register HTTPSPROV_ACTION actions and usages.
 *
 * NOTES
 *   HTTPSPROV_ACTION ({573E31F8-AABA-11D0-8CCB-00C04FC295EE})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterHttpsProv(void)
{
    BOOL RegisteredOK = TRUE;
    static CHAR SoftpubLoadUsage[] = "SoftpubLoadDefUsageCallData";
    static CHAR SoftpubFreeUsage[] = "SoftpubFreeDefUsageCallData";
    static GUID ProvGUID = HTTPSPROV_ACTION;
    CRYPT_REGISTER_ACTIONID ProvInfo;
    CRYPT_PROVIDER_REGDEFUSAGE DefUsage = { sizeof(CRYPT_PROVIDER_REGDEFUSAGE),
                                            &ProvGUID,
                                            NULL, /* Will be filled later */
                                            SoftpubLoadUsage,
                                            SoftpubFreeUsage };

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = HTTPSCertificateTrust;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = HTTPSFinalProv;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    DefUsage.pwszDllName = wcsdup(SP_POLICY_PROVIDER_DLL_NAME);

    if (!WintrustAddDefaultForUsage(szOID_PKIX_KP_SERVER_AUTH, &DefUsage))
        RegisteredOK = FALSE;
    if (!WintrustAddDefaultForUsage(szOID_PKIX_KP_CLIENT_AUTH, &DefUsage))
        RegisteredOK = FALSE;
    if (!WintrustAddDefaultForUsage(szOID_SERVER_GATED_CRYPTO, &DefUsage))
        RegisteredOK = FALSE;
    if (!WintrustAddDefaultForUsage(szOID_SGC_NETSCAPE, &DefUsage))
        RegisteredOK = FALSE;

    free(DefUsage.pwszDllName);

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        RegisteredOK = FALSE;

    return RegisteredOK;
}

/***************************************************************************
 *              WINTRUST_RegisterOfficeSignVerify
 *
 * Register OFFICESIGN_ACTION_VERIFY actions and usages.
 *
 * NOTES
 *   OFFICESIGN_ACTION_VERIFY ({5555C2CD-17FB-11D1-85C4-00C04FC295EE})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterOfficeSignVerify(void)
{
    static GUID ProvGUID = OFFICESIGN_ACTION_VERIFY;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = OfficeInitializePolicy;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = SoftpubFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = OfficeCleanupPolicy;


    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***************************************************************************
 *              WINTRUST_RegisterDriverVerify
 *
 * Register DRIVER_ACTION_VERIFY actions and usages.
 *
 * NOTES
 *   DRIVER_ACTION_VERIFY ({F750E6C3-38EE-11D1-85E5-00C04FC295EE})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterDriverVerify(void)
{
    static GUID ProvGUID = DRIVER_ACTION_VERIFY;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = DriverInitializePolicy;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = SoftpubCertificate;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = DriverFinalPolicy;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = DriverCleanupPolicy;


    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***************************************************************************
 *              WINTRUST_RegisterGenChainVerify
 *
 * Register WINTRUST_ACTION_GENERIC_CHAIN_VERIFY actions and usages.
 *
 * NOTES
 *   WINTRUST_ACTION_GENERIC_CHAIN_VERIFY ({FC451C16-AC75-11D1-B4B8-00C04FB66EA0})
 *   is defined in softpub.h
 */
static BOOL WINTRUST_RegisterGenChainVerify(void)
{
    static GUID ProvGUID = WINTRUST_ACTION_GENERIC_CHAIN_VERIFY;
    CRYPT_REGISTER_ACTIONID ProvInfo;

    ProvInfo.cbStruct                   = sizeof(CRYPT_REGISTER_ACTIONID);
    ProvInfo.sInitProvider              = SoftpubInitialization;
    ProvInfo.sObjectProvider            = SoftpubMessage;
    ProvInfo.sSignatureProvider         = SoftpubSignature;
    ProvInfo.sCertificateProvider       = GenericChainCertificateTrust;
    ProvInfo.sCertificatePolicyProvider = SoftpubCertCheck;
    ProvInfo.sFinalPolicyProvider       = GenericChainFinalProv;
    ProvInfo.sTestPolicyProvider        = NullCTRE; /* No diagnostic policy */
    ProvInfo.sCleanupProvider           = SoftpubCleanup;

    if (!WintrustAddActionID(&ProvGUID, 0, &ProvInfo))
        return FALSE;

    return TRUE;
}

/***********************************************************************
 *              WintrustAddDefaultForUsage (WINTRUST.@)
 *
 * Write OID and callback functions to the registry.
 *
 * PARAMS
 *   pszUsageOID [I] Pointer to a GUID.
 *   psDefUsage  [I] Pointer to a structure that specifies the callback functions.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 *
 * NOTES
 *   WintrustAddDefaultForUsage will only return TRUE or FALSE, no last 
 *   error is set, not even when the registry cannot be written to.
 */
BOOL WINAPI WintrustAddDefaultForUsage(const char *pszUsageOID,
                                       CRYPT_PROVIDER_REGDEFUSAGE *psDefUsage)
{
    static const WCHAR CBAlloc[]    = {'C','a','l','l','b','a','c','k','A','l','l','o','c','F','u','n','c','t','i','o','n', 0};
    static const WCHAR CBFree[]     = {'C','a','l','l','b','a','c','k','F','r','e','e','F','u','n','c','t','i','o','n', 0};
    LONG Res = ERROR_SUCCESS;
    LONG WriteUsageError = ERROR_SUCCESS;
    DWORD Len;
    WCHAR GuidString[39];

    TRACE("(%s %p)\n", debugstr_a(pszUsageOID), psDefUsage);

    /* Some sanity checks. */
    if (!pszUsageOID ||
        !psDefUsage ||
        !psDefUsage->pgActionID ||
        (psDefUsage->cbStruct != sizeof(CRYPT_PROVIDER_REGDEFUSAGE)))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (psDefUsage->pwszDllName)
    {
        Res = WINTRUST_WriteSingleUsageEntry(pszUsageOID, Dll, psDefUsage->pwszDllName);
        if (Res != ERROR_SUCCESS) WriteUsageError = Res;
    }
    if (psDefUsage->pwszLoadCallbackDataFunctionName)
    {
        WCHAR* CallbackW;

        Len = MultiByteToWideChar( CP_ACP, 0, psDefUsage->pwszLoadCallbackDataFunctionName, -1, NULL, 0 );
        CallbackW = malloc( Len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, psDefUsage->pwszLoadCallbackDataFunctionName, -1, CallbackW, Len );

        Res = WINTRUST_WriteSingleUsageEntry(pszUsageOID, CBAlloc, CallbackW);
        if (Res != ERROR_SUCCESS) WriteUsageError = Res;

        free(CallbackW);
    }
    if (psDefUsage->pwszFreeCallbackDataFunctionName)
    {
        WCHAR* CallbackW;

        Len = MultiByteToWideChar( CP_ACP, 0, psDefUsage->pwszFreeCallbackDataFunctionName, -1, NULL, 0 );
        CallbackW = malloc( Len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, psDefUsage->pwszFreeCallbackDataFunctionName, -1, CallbackW, Len );

        Res = WINTRUST_WriteSingleUsageEntry(pszUsageOID, CBFree, CallbackW);
        if (Res != ERROR_SUCCESS) WriteUsageError = Res;

        free(CallbackW);
    }

    WINTRUST_Guid2Wstr(psDefUsage->pgActionID, GuidString);
    Res = WINTRUST_WriteSingleUsageEntry(pszUsageOID, DefaultId, GuidString);
    if (Res != ERROR_SUCCESS) WriteUsageError = Res;

    if (WriteUsageError != ERROR_SUCCESS)
        return FALSE;

    return TRUE;
}

static FARPROC WINTRUST_ReadProviderFromReg(WCHAR *GuidString, const WCHAR *FunctionType)
{
    WCHAR ProvKey[MAX_PATH], DllName[MAX_PATH];
    char FunctionName[MAX_PATH];
    HKEY Key;
    LONG Res = ERROR_SUCCESS;
    DWORD Size;
    HMODULE Lib;
    FARPROC Func = NULL;

    /* Create the needed key string */
    ProvKey[0]='\0';
    lstrcatW(ProvKey, Trust);
    lstrcatW(ProvKey, FunctionType);
    lstrcatW(ProvKey, GuidString);

    Res = RegOpenKeyExW(HKEY_LOCAL_MACHINE, ProvKey, 0, KEY_READ, &Key);
    if (Res != ERROR_SUCCESS) return NULL;

    /* Read the $DLL entry */
    Size = sizeof(DllName);
    Res = RegQueryValueExW(Key, Dll, NULL, NULL, (LPBYTE)DllName, &Size);
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Read the $Function entry */
    Size = sizeof(FunctionName);
    Res = RegQueryValueExA(Key, "$Function", NULL, NULL, (LPBYTE)FunctionName, &Size);
    if (Res != ERROR_SUCCESS) goto error_close_key;

    /* Load the library - there appears to be no way to close a provider, so
     * just leak the module handle.
     */
    Lib = LoadLibraryW(DllName);
    Func = GetProcAddress(Lib, FunctionName);

error_close_key:
    RegCloseKey(Key);

    return Func;
}

static CRITICAL_SECTION cache_cs;
static CRITICAL_SECTION_DEBUG cache_cs_debug =
{
    0, 0, &cache_cs,
    { &cache_cs_debug.ProcessLocksList, &cache_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cache_cs") }
};
static CRITICAL_SECTION cache_cs = { &cache_cs_debug, -1, 0, 0, 0, 0 };

static struct provider_cache_entry
{
    GUID guid;
    CRYPT_PROVIDER_FUNCTIONS provider_functions;
}
*provider_cache;
static unsigned int provider_cache_size;

static void * WINAPI WINTRUST_Alloc(DWORD cb)
{
    return calloc(1, cb);
}

static void WINAPI WINTRUST_Free(void *p)
{
    free(p);
}

/***********************************************************************
 *              WintrustLoadFunctionPointers (WINTRUST.@)
 */
BOOL WINAPI WintrustLoadFunctionPointers( GUID* pgActionID,
                                          CRYPT_PROVIDER_FUNCTIONS* pPfns )
{
    WCHAR GuidString[39];
    BOOL cached = FALSE;
    unsigned int i;

    TRACE("(%s %p)\n", debugstr_guid(pgActionID), pPfns);

    if (!pPfns) return FALSE;
    if (!pgActionID)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (pPfns->cbStruct != sizeof(CRYPT_PROVIDER_FUNCTIONS)) return FALSE;

    EnterCriticalSection( &cache_cs );
    for (i = 0; i < provider_cache_size; ++i)
    {
        if (IsEqualGUID( &provider_cache[i].guid, pgActionID ))
        {
            TRACE( "Using cached data.\n" );
            *pPfns = provider_cache[i].provider_functions;
            cached = TRUE;
            break;
        }
    }
    LeaveCriticalSection( &cache_cs );
    if (cached) return TRUE;

    /* Create this string only once, instead of in the helper function */
    WINTRUST_Guid2Wstr( pgActionID, GuidString);

    /* Get the function pointers from the registry, where applicable */
    pPfns->pfnAlloc = WINTRUST_Alloc;
    pPfns->pfnFree = WINTRUST_Free;
    pPfns->pfnAddStore2Chain = WINTRUST_AddStore;
    pPfns->pfnAddSgnr2Chain = WINTRUST_AddSgnr;
    pPfns->pfnAddCert2Chain = WINTRUST_AddCert;
    pPfns->pfnAddPrivData2Chain = WINTRUST_AddPrivData;
    pPfns->psUIpfns = NULL;
    pPfns->pfnInitialize = (PFN_PROVIDER_INIT_CALL)WINTRUST_ReadProviderFromReg(GuidString, Initialization);
    pPfns->pfnObjectTrust = (PFN_PROVIDER_OBJTRUST_CALL)WINTRUST_ReadProviderFromReg(GuidString, Message);
    pPfns->pfnSignatureTrust = (PFN_PROVIDER_SIGTRUST_CALL)WINTRUST_ReadProviderFromReg(GuidString, Signature);
    pPfns->pfnCertificateTrust = (PFN_PROVIDER_CERTTRUST_CALL)WINTRUST_ReadProviderFromReg(GuidString, Certificate);
    pPfns->pfnCertCheckPolicy = (PFN_PROVIDER_CERTCHKPOLICY_CALL)WINTRUST_ReadProviderFromReg(GuidString, CertCheck);
    pPfns->pfnFinalPolicy = (PFN_PROVIDER_FINALPOLICY_CALL)WINTRUST_ReadProviderFromReg(GuidString, FinalPolicy);
    pPfns->pfnTestFinalPolicy = (PFN_PROVIDER_TESTFINALPOLICY_CALL)WINTRUST_ReadProviderFromReg(GuidString, DiagnosticPolicy);
    pPfns->pfnCleanupPolicy = (PFN_PROVIDER_CLEANUP_CALL)WINTRUST_ReadProviderFromReg(GuidString, Cleanup);

    EnterCriticalSection( &cache_cs );
    for (i = 0; i < provider_cache_size; ++i)
        if (IsEqualGUID( &provider_cache[i].guid, pgActionID )) break;

    if (i == provider_cache_size)
    {
        struct provider_cache_entry *new;

        new = realloc( provider_cache, (provider_cache_size + 1) * sizeof(*new) );
        if (new)
        {
            provider_cache = new;
            provider_cache[provider_cache_size].guid = *pgActionID;
            provider_cache[provider_cache_size].provider_functions = *pPfns;
            ++provider_cache_size;
        }
    }
    LeaveCriticalSection( &cache_cs );

    return TRUE;
}

/***********************************************************************
 *              WINTRUST_SIPPAddProvider
 *
 * Helper for DllRegisterServer.
 */
static BOOL WINTRUST_SIPPAddProvider(GUID* Subject, WCHAR* MagicNumber)
{
    static WCHAR CryptSIPGetSignedDataMsg[] =
        {'C','r','y','p','t','S','I','P','G','e','t','S','i','g','n','e','d','D','a','t','a','M','s','g', 0};
    static WCHAR CryptSIPPutSignedDataMsg[] =
        {'C','r','y','p','t','S','I','P','P','u','t','S','i','g','n','e','d','D','a','t','a','M','s','g', 0};
    static WCHAR CryptSIPCreateIndirectData[] =
        {'C','r','y','p','t','S','I','P','C','r','e','a','t','e','I','n','d','i','r','e','c','t','D','a','t','a', 0};
    static WCHAR CryptSIPVerifyIndirectData[] =
        {'C','r','y','p','t','S','I','P','V','e','r','i','f','y','I','n','d','i','r','e','c','t','D','a','t','a', 0};
    static WCHAR CryptSIPRemoveSignedDataMsg[] =
        {'C','r','y','p','t','S','I','P','R','e','m','o','v','e','S','i','g','n','e','d','D','a','t','a','M','s','g', 0};
    SIP_ADD_NEWPROVIDER NewProv;
    BOOL Ret;

    /* Clear and initialize the structure */
    memset(&NewProv, 0, sizeof(SIP_ADD_NEWPROVIDER));
    NewProv.cbStruct = sizeof(SIP_ADD_NEWPROVIDER);
    NewProv.pwszDLLFileName = wcsdup(SP_POLICY_PROVIDER_DLL_NAME);
    /* Fill the structure */
    NewProv.pgSubject              = Subject;
    NewProv.pwszMagicNumber        = MagicNumber;
    NewProv.pwszIsFunctionName     = NULL;
    NewProv.pwszGetFuncName        = CryptSIPGetSignedDataMsg;
    NewProv.pwszPutFuncName        = CryptSIPPutSignedDataMsg;
    NewProv.pwszCreateFuncName     = CryptSIPCreateIndirectData;
    NewProv.pwszVerifyFuncName     = CryptSIPVerifyIndirectData;
    NewProv.pwszRemoveFuncName     = CryptSIPRemoveSignedDataMsg;
    NewProv.pwszIsFunctionNameFmt2 = NULL;
    NewProv.pwszGetCapFuncName     = NULL;

    Ret = CryptSIPAddProvider(&NewProv);

    free(NewProv.pwszDLLFileName);

    return Ret;
}

/***********************************************************************
 *              DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    static const CHAR SpcPeImageDataEncode[]           = "WVTAsn1SpcPeImageDataEncode";
    static const CHAR SpcPeImageDataDecode[]           = "WVTAsn1SpcPeImageDataDecode";
    static const CHAR SpcLinkEncode[]                  = "WVTAsn1SpcLinkEncode";
    static const CHAR SpcLinkDecode[]                  = "WVTAsn1SpcLinkDecode";
    static const CHAR SpcSigInfoEncode[]               = "WVTAsn1SpcSigInfoEncode";
    static const CHAR SpcSigInfoDecode[]               = "WVTAsn1SpcSigInfoDecode";
    static const CHAR SpcIndirectDataContentEncode[]   = "WVTAsn1SpcIndirectDataContentEncode";
    static const CHAR SpcIndirectDataContentDecode[]   = "WVTAsn1SpcIndirectDataContentDecode";
    static const CHAR SpcSpAgencyInfoEncode[]          = "WVTAsn1SpcSpAgencyInfoEncode";
    static const CHAR SpcSpAgencyInfoDecode[]          = "WVTAsn1SpcSpAgencyInfoDecode";
    static const CHAR SpcMinimalCriteriaInfoEncode[]   = "WVTAsn1SpcMinimalCriteriaInfoEncode";
    static const CHAR SpcMinimalCriteriaInfoDecode[]   = "WVTAsn1SpcMinimalCriteriaInfoDecode";
    static const CHAR SpcFinancialCriteriaInfoEncode[] = "WVTAsn1SpcFinancialCriteriaInfoEncode";
    static const CHAR SpcFinancialCriteriaInfoDecode[] = "WVTAsn1SpcFinancialCriteriaInfoDecode";
    static const CHAR SpcStatementTypeEncode[]         = "WVTAsn1SpcStatementTypeEncode";
    static const CHAR SpcStatementTypeDecode[]         = "WVTAsn1SpcStatementTypeDecode";
    static const CHAR CatNameValueEncode[]             = "WVTAsn1CatNameValueEncode";
    static const CHAR CatNameValueDecode[]             = "WVTAsn1CatNameValueDecode";
    static const CHAR CatMemberInfoEncode[]            = "WVTAsn1CatMemberInfoEncode";
    static const CHAR CatMemberInfoDecode[]            = "WVTAsn1CatMemberInfoDecode";
    static const CHAR SpcSpOpusInfoEncode[]            = "WVTAsn1SpcSpOpusInfoEncode";
    static const CHAR SpcSpOpusInfoDecode[]            = "WVTAsn1SpcSpOpusInfoDecode";
    static GUID Unknown1 = { 0xDE351A42, 0x8E59, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static GUID Unknown2 = { 0xC689AABA, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static GUID Unknown3 = { 0xC689AAB8, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static GUID Unknown4 = { 0xC689AAB9, 0x8E78, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static GUID Unknown5 = { 0xDE351A43, 0x8E59, 0x11D0, { 0x8C,0x47,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static GUID Unknown6 = { 0x9BA61D3F, 0xE73A, 0x11D0, { 0x8C,0xD2,0x00,0xC0,0x4F,0xC2,0x95,0xEE }};
    static WCHAR MagicNumber2[] = {'M','S','C','F', 0};
    static WCHAR MagicNumber3[] = {'0','x','0','0','0','0','4','5','5','0', 0};
    static WCHAR CafeBabe[] = {'0','x','c','a','f','e','b','a','b','e', 0};

    HRESULT CryptRegisterRes = S_OK;
    HRESULT TrustProviderRes = S_OK;
    HRESULT SIPAddProviderRes = S_OK;
    HCRYPTPROV crypt_provider;
    BOOL ret;

    TRACE("\n");

    /* Testing on native shows that when an error is encountered in one of the CryptRegisterOIDFunction calls
     * the rest of these calls are skipped. Registering is however continued for the trust providers.
     *
     * We are not totally in line with native as all decoding functions are registered after all encoding
     * functions there.
     */
#define WINTRUST_REGISTEROID( oid, encode_funcname, decode_funcname ) \
    do { \
        if (!CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_ENCODE_OBJECT_FUNC, oid, SP_POLICY_PROVIDER_DLL_NAME, encode_funcname)) \
        {                                                               \
            CryptRegisterRes = HRESULT_FROM_WIN32(GetLastError());      \
            goto add_trust_providers;                                   \
        }                                                               \
        if (!CryptRegisterOIDFunction(X509_ASN_ENCODING, CRYPT_OID_DECODE_OBJECT_FUNC, oid, SP_POLICY_PROVIDER_DLL_NAME, decode_funcname)) \
        {                                                               \
            CryptRegisterRes = HRESULT_FROM_WIN32(GetLastError());      \
            goto add_trust_providers;                                   \
        }                                                               \
    } while (0)

    WINTRUST_REGISTEROID(SPC_PE_IMAGE_DATA_OBJID, SpcPeImageDataEncode, SpcPeImageDataDecode);
    WINTRUST_REGISTEROID(SPC_PE_IMAGE_DATA_STRUCT, SpcPeImageDataEncode, SpcPeImageDataDecode);
    WINTRUST_REGISTEROID(SPC_CAB_DATA_OBJID, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_CAB_DATA_STRUCT, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_JAVA_CLASS_DATA_OBJID, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_JAVA_CLASS_DATA_STRUCT, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_LINK_OBJID, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_LINK_STRUCT, SpcLinkEncode, SpcLinkDecode);
    WINTRUST_REGISTEROID(SPC_SIGINFO_OBJID, SpcSigInfoEncode, SpcSigInfoDecode);
    WINTRUST_REGISTEROID(SPC_SIGINFO_STRUCT, SpcSigInfoEncode, SpcSigInfoDecode);
    WINTRUST_REGISTEROID(SPC_INDIRECT_DATA_OBJID, SpcIndirectDataContentEncode, SpcIndirectDataContentDecode);
    WINTRUST_REGISTEROID(SPC_INDIRECT_DATA_CONTENT_STRUCT, SpcIndirectDataContentEncode, SpcIndirectDataContentDecode);
    WINTRUST_REGISTEROID(SPC_SP_AGENCY_INFO_OBJID, SpcSpAgencyInfoEncode, SpcSpAgencyInfoDecode);
    WINTRUST_REGISTEROID(SPC_SP_AGENCY_INFO_STRUCT, SpcSpAgencyInfoEncode, SpcSpAgencyInfoDecode);
    WINTRUST_REGISTEROID(SPC_MINIMAL_CRITERIA_OBJID, SpcMinimalCriteriaInfoEncode, SpcMinimalCriteriaInfoDecode);
    WINTRUST_REGISTEROID(SPC_MINIMAL_CRITERIA_STRUCT, SpcMinimalCriteriaInfoEncode, SpcMinimalCriteriaInfoDecode);
    WINTRUST_REGISTEROID(SPC_FINANCIAL_CRITERIA_OBJID, SpcFinancialCriteriaInfoEncode, SpcFinancialCriteriaInfoDecode);
    WINTRUST_REGISTEROID(SPC_FINANCIAL_CRITERIA_STRUCT, SpcFinancialCriteriaInfoEncode, SpcFinancialCriteriaInfoDecode);
    WINTRUST_REGISTEROID(SPC_STATEMENT_TYPE_OBJID, SpcStatementTypeEncode, SpcStatementTypeDecode);
    WINTRUST_REGISTEROID(SPC_STATEMENT_TYPE_STRUCT, SpcStatementTypeEncode, SpcStatementTypeDecode);
    WINTRUST_REGISTEROID(CAT_NAMEVALUE_OBJID, CatNameValueEncode, CatNameValueDecode);
    WINTRUST_REGISTEROID(CAT_NAMEVALUE_STRUCT, CatNameValueEncode, CatNameValueDecode);
    WINTRUST_REGISTEROID(CAT_MEMBERINFO_OBJID, CatMemberInfoEncode, CatMemberInfoDecode);
    WINTRUST_REGISTEROID(CAT_MEMBERINFO_STRUCT, CatMemberInfoEncode, CatMemberInfoDecode);
    WINTRUST_REGISTEROID(SPC_SP_OPUS_INFO_OBJID, SpcSpOpusInfoEncode, SpcSpOpusInfoDecode);
    WINTRUST_REGISTEROID(SPC_SP_OPUS_INFO_STRUCT, SpcSpOpusInfoEncode,  SpcSpOpusInfoDecode);

#undef WINTRUST_REGISTEROID

add_trust_providers:

    /* Testing on W2K3 shows:
     * All registry writes are tried. If one fails this part will return S_FALSE.
     *
     * Last error is set to the last error encountered, regardless if the first
     * part failed or not.
     */

    /* Create the necessary action registry structures */
    WINTRUST_InitRegStructs();

    /* Register several Trust Provider actions */
    if (!WINTRUST_RegisterGenVerifyV2())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterPublishedSoftware())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterPublishedSoftwareNoBadUi())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterGenCertVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterTrustProviderTest())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterHttpsProv())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterOfficeSignVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterDriverVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterGenChainVerify())
        TrustProviderRes = S_FALSE;

    /* Free the registry structures */
    WINTRUST_FreeRegStructs();

    /* Testing on W2K3 shows:
     * All registry writes are tried. If one fails this part will return S_FALSE.
     *
     * Last error is set to the last error encountered, regardless if the previous
     * parts failed or not.
     */

    if (!WINTRUST_SIPPAddProvider(&Unknown1, NULL))
        SIPAddProviderRes = S_FALSE;
    if (!WINTRUST_SIPPAddProvider(&Unknown2, MagicNumber2))
        SIPAddProviderRes = S_FALSE;
    if (!WINTRUST_SIPPAddProvider(&Unknown3, MagicNumber3))
        SIPAddProviderRes = S_FALSE;
    if (!WINTRUST_SIPPAddProvider(&Unknown4, CafeBabe))
        SIPAddProviderRes = S_FALSE;
    if (!WINTRUST_SIPPAddProvider(&Unknown5, CafeBabe))
        SIPAddProviderRes = S_FALSE;
    if (!WINTRUST_SIPPAddProvider(&Unknown6, CafeBabe))
        SIPAddProviderRes = S_FALSE;

    /* Native does a CryptSIPRemoveProvider here for {941C2937-1292-11D1-85BE-00C04FC295EE}.
     * This SIP Provider is however not found on up-to-date window install and native will
     * set the last error to ERROR_FILE_NOT_FOUND.
     * Wine has the last error set to ERROR_INVALID_PARAMETER. There shouldn't be an app
     * depending on this last error though so there is no need to imitate native to the full extent.
     *
     * (The ERROR_INVALID_PARAMETER for Wine it totally valid as we (and native) do register
     * a trust provider without a diagnostic policy).
     */

    /* Create a dummy context to force creation of the MachineGuid registry key. */
    ret = CryptAcquireContextW(&crypt_provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    if (ret) CryptReleaseContext(crypt_provider, 0);
    else ERR("Failed to acquire cryptographic context: %lu\n", GetLastError());

    /* If CryptRegisterRes is not S_OK it will always overrule the return value. */
    if (CryptRegisterRes != S_OK)
        return CryptRegisterRes;
    else if (SIPAddProviderRes == S_OK)
        return TrustProviderRes;
    else 
        return SIPAddProviderRes;
}

/***********************************************************************
 *              DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              SoftpubDllRegisterServer (WINTRUST.@)
 *
 * Registers softpub.dll
 *
 * PARAMS
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: S_FALSE. (See also GetLastError()).
 *
 * NOTES
 *  DllRegisterServer in softpub.dll will call this function.
 *  See comments in DllRegisterServer.
 */
HRESULT WINAPI SoftpubDllRegisterServer(void)
{
    HRESULT TrustProviderRes = S_OK;

    TRACE("\n");

    /* Create the necessary action registry structures */
    WINTRUST_InitRegStructs();

    /* Register several Trust Provider actions */
    if (!WINTRUST_RegisterGenVerifyV2())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterPublishedSoftware())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterPublishedSoftwareNoBadUi())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterGenCertVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterTrustProviderTest())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterHttpsProv())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterOfficeSignVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterDriverVerify())
        TrustProviderRes = S_FALSE;
    if (!WINTRUST_RegisterGenChainVerify())
        TrustProviderRes = S_FALSE;

    /* Free the registry structures */
    WINTRUST_FreeRegStructs();

    return TrustProviderRes;
}

/***********************************************************************
 *              SoftpubDllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI SoftpubDllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mscat32DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI mscat32DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mscat32DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI mscat32DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mssip32DllRegisterServer (WINTRUST.@)
 */
HRESULT WINAPI mssip32DllRegisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}

/***********************************************************************
 *              mssip32DllUnregisterServer (WINTRUST.@)
 */
HRESULT WINAPI mssip32DllUnregisterServer(void)
{
     FIXME("stub\n");
     return S_OK;
}
