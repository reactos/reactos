/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     main test unit
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include "precomp.h"

#define MSV1_0_DLL L"msv1_0.dll"

typedef struct _LSA_FUNCS
{
    HANDLE hMsv1_0Dll;

    SpLsaModeInitializeFn SpLsaModeInitialize;
    PLSA_AP_INITIALIZE_PACKAGE LsaApInitializePackage;

    PSECPKG_FUNCTION_TABLE fnTable;
    ULONG fnTableCount;
} LSA_FUNCS, *PLSA_FUNCS;

static LSA_SECPKG_FUNCTION_TABLE LsaFnTable = {0};

static void InitFnTable()
{
    RtlZeroMemory(&LsaFnTable, sizeof(LsaFnTable));
    LsaFnTable.CreateLogonSession = LsaFnCreateLogonSession;
    LsaFnTable.DeleteLogonSession = LsaFnDeleteLogonSession;
    LsaFnTable.AddCredential = LsaFnAddCredential;
    LsaFnTable.GetCredentials = LsaFnGetCredentials;
    LsaFnTable.DeleteCredential = LsaFnDeleteCredential;
    LsaFnTable.AllocateLsaHeap = LsaFnAllocateLsaHeap;
    LsaFnTable.FreeLsaHeap = LsaFnFreeLsaHeap;
    LsaFnTable.AllocateClientBuffer = LsaFnAllocateClientBuffer;
    LsaFnTable.FreeClientBuffer = LsaFnFreeClientBuffer;
    LsaFnTable.CopyToClientBuffer = LsaFnCopyToClientBuffer;
    LsaFnTable.CopyFromClientBuffer = LsaFnCopyFromClientBuffer;
    LsaFnTable.ImpersonateClient = LsaFnImpersonateClient;
    LsaFnTable.UnloadPackage = LsaFnUnloadPackage;
    LsaFnTable.DuplicateHandle = LsaFnDuplicateHandle;
    LsaFnTable.SaveSupplementalCredentials = LsaFnSaveSupplementalCredentials;
    LsaFnTable.CreateThread = LsaFnCreateThread;
    LsaFnTable.GetClientInfo = LsaFnGetClientInfo;
    LsaFnTable.RegisterNotification = LsaFnRegisterNotification;
    LsaFnTable.CancelNotification = LsaFnCancelNotification;
    LsaFnTable.MapBuffer = LsaFnMapBuffer;
    LsaFnTable.CreateToken = LsaFnCreateToken;
    LsaFnTable.AuditLogon = LsaFnAuditLogon;
    LsaFnTable.CallPackage = LsaFnCallPackage;
    LsaFnTable.FreeReturnBuffer = LsaFnFreeReturnBuffer;
    LsaFnTable.GetCallInfo = LsaFnGetCallInfo;
    LsaFnTable.CallPackageEx = LsaFnCallPackageEx;
    LsaFnTable.CreateSharedMemory = LsaFnCreateSharedMemory;
    LsaFnTable.AllocateSharedMemory = LsaFnAllocateSharedMemory;
    LsaFnTable.FreeSharedMemory = LsaFnFreeSharedMemory;
    LsaFnTable.DeleteSharedMemory = LsaFnDeleteSharedMemory;
    LsaFnTable.OpenSamUser = LsaFnOpenSamUser;
    LsaFnTable.GetUserCredentials = LsaFnGetUserCredentials;
    LsaFnTable.GetUserAuthData = LsaFnGetUserAuthData;
    LsaFnTable.CloseSamUser = LsaFnCloseSamUser;
    LsaFnTable.ConvertAuthDataToToken = LsaFnConvertAuthDataToToken;
    LsaFnTable.ClientCallback = LsaFnClientCallback;
    LsaFnTable.UpdateCredentials = LsaFnUpdateCredentials;
    LsaFnTable.GetAuthDataForUser = LsaFnGetAuthDataForUser;
    LsaFnTable.CrackSingleName = LsaFnCrackSingleName;
    LsaFnTable.AuditAccountLogon = LsaFnAuditAccountLogon;
    LsaFnTable.CallPackagePassthrough = LsaFnCallPackagePassthrough;
    LsaFnTable.DummyFunction1 = LsaFnDummyFunctionX;
    LsaFnTable.DummyFunction2 = LsaFnDummyFunctionX;
    LsaFnTable.DummyFunction3 = LsaFnDummyFunctionX;
    LsaFnTable.LsaProtectMemory = LsaFnProtectMemory;
    LsaFnTable.LsaUnprotectMemory = LsaFnUnprotectMemory;
    LsaFnTable.OpenTokenByLogonId = LsaFnOpenTokenByLogonId;
    LsaFnTable.ExpandAuthDataForDomain = LsaFnExpandAuthDataForDomain;
    LsaFnTable.AllocatePrivateHeap = LsaFnAllocatePrivateHeap;
    LsaFnTable.FreePrivateHeap = LsaFnFreePrivateHeap;
}

static void TestLsaModeInitialize(
    IN PLSA_FUNCS lsa)
{
    ULONG PackageVersion;
    NTSTATUS res;

    trace("testing SpLsaModeInitialize.\n");

    res = lsa->SpLsaModeInitialize(SECPKG_INTERFACE_VERSION, &PackageVersion,
                                   &lsa->fnTable, &lsa->fnTableCount);
    ok(res == STATUS_SUCCESS, "SpLsaModeInitialize failed: 0x%x\n", (UINT)res);
    if (res != STATUS_SUCCESS)
        return;
    ok(lsa->fnTable != NULL, "lsa.fnTable is NULL\n");
    ok(lsa->fnTableCount == 1, "lsa.fnTableCount != 1\n");
    ok(PackageVersion == SECPKG_INTERFACE_VERSION, "Unexpected version %lu of package!\n", PackageVersion);

    ok(lsa->fnTable->InitializePackage == NULL, "InitializePackage != NULL\n");
    ok(lsa->fnTable->LsaLogonUser == NULL, "LsaLogonUser != NULL\n");
    ok(lsa->fnTable->CallPackage != NULL, "CallPackage == NULL\n");
    ok(lsa->fnTable->LogonTerminated != NULL, "LogonTerminated == NULL\n");
    ok(lsa->fnTable->CallPackageUntrusted != NULL, "CallPackageUntrusted == NULL\n");
    ok(lsa->fnTable->CallPackagePassthrough != NULL, "CallPackagePassthrough == NULL\n");
    ok(lsa->fnTable->LogonUserEx == NULL, "LogonUserEx != NULL\n");
    ok(lsa->fnTable->LogonUserEx2 != NULL, "LogonUserEx2 == NULL\n");
    ok(lsa->fnTable->Initialize != NULL, "Initialize == NULL\n");
    ok(lsa->fnTable->Shutdown != NULL, "Shutdown == NULL\n");
    ok(lsa->fnTable->GetInfo != NULL, "GetInfo == NULL\n");
    ok(lsa->fnTable->AcceptCredentials != NULL, "AcceptCredentials == NULL\n");
    ok(lsa->fnTable->SpAcquireCredentialsHandle != NULL, "SpAcquireCredentialsHandle == NULL\n");
    ok(lsa->fnTable->SpQueryCredentialsAttributes != NULL, "SpQueryCredentialsAttributes == NULL\n");
    ok(lsa->fnTable->FreeCredentialsHandle != NULL, "FreeCredentialsHandle == NULL\n");
    ok(lsa->fnTable->SaveCredentials != NULL, "SaveCredentials == NULL\n");
    ok(lsa->fnTable->GetCredentials != NULL, "GetCredentials == NULL\n");
    ok(lsa->fnTable->DeleteCredentials != NULL, "DeleteCredentials == NULL\n");
    ok(lsa->fnTable->InitLsaModeContext != NULL, "InitLsaModeContext == NULL\n");
    ok(lsa->fnTable->AcceptLsaModeContext != NULL, "AcceptLsaModeContext == NULL\n");
    ok(lsa->fnTable->DeleteContext != NULL, "DeleteContext == NULL\n");
    ok(lsa->fnTable->ApplyControlToken != NULL, "ApplyControlToken == NULL\n");
    ok(lsa->fnTable->GetUserInfo != NULL, "GetUserInfo == NULL\n");
    ok(lsa->fnTable->GetExtendedInformation != NULL, "GetExtendedInformation == NULL\n");
    ok(lsa->fnTable->SpQueryContextAttributes == NULL, "SpQueryContextAttributes != NULL\n");
    ok(lsa->fnTable->SpAddCredentials == NULL, "SpAddCredentials != NULL\n");
    ok(lsa->fnTable->SetExtendedInformation != NULL, "SetExtendedInformation == NULL\n");
}

// a random id we use where we need a "AuthenticationPackageId"
#define LSA_AUTH_PACKAGE_ID 6

// Deactivated!
// The call to lsa->fnTable->Initialize will make lsass.exe unstable.
// Maybe it is not possible to call it a second time. Initialize was
// already called by lsass.exe
#if 0
SECPKG_PARAMETERS LsaParameters = {0};
static void TestLsaSpInitialize(
    IN PLSA_FUNCS lsa)
{
    NTSTATUS res;
    UCHAR SidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
    PSID pSid;
    SID_IDENTIFIER_AUTHORITY ntAuthority = { SECURITY_NT_AUTHORITY };
    DWORD size;
    WCHAR DomainName[64];
    WCHAR DnsDomainName[64];

    #define PACKAGE_ID 0x2

    trace("testing Initialize.\n");

    if (lsa->fnTable->Initialize == NULL)
    {
        ok(FALSE, "Initialize is NULL!\n");
        return;
    }

    size = ARRAY_SIZE(DomainName);
    if (!GetComputerNameExW(ComputerNameNetBIOS, DomainName, &size))
    {
        ok(FALSE, "GetComputerNameExW failed with 0x%lx.\n", GetLastError());
        DomainName[0] = L'\0';
    }
    size = ARRAY_SIZE(DnsDomainName);
    if (!GetComputerNameExW(ComputerNameDnsFullyQualified, DnsDomainName, &size))
    {
        ok(FALSE, "GetComputerNameExW failed with 0x%lx.\n", GetLastError());
        DnsDomainName[0] = L'\0';
    }

    LsaParameters.Version = 2;
    LsaParameters.MachineState = SECPKG_STATE_STANDALONE |
                                 SECPKG_STATE_ENCRYPTION_PERMITTED |
                                 SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED;

    LsaParameters.SetupMode = 0;
    RtlInitUnicodeString(&LsaParameters.DomainName, DomainName);
    RtlInitUnicodeString(&LsaParameters.DnsDomainName, DnsDomainName);

    pSid = SidBuffer;
    RtlInitializeSid(pSid, &ntAuthority, 2);
    *RtlSubAuthoritySid(pSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(pSid, 1) = DOMAIN_ALIAS_RID_ADMINS;

    ok(RtlValidSid(pSid), "SID is not valid!\n");

    LsaParameters.DomainSid = pSid;

    res = lsa->fnTable->Initialize(PACKAGE_ID, &LsaParameters, &LsaFnTable);
    ok(res == STATUS_SUCCESS, "SpLsaInitialize failed: 0x%x\n", (UINT)res);
    ok(g_LogonId.LowPart != 0, "CreateLogonSession was not called!\n");
}
#endif

static void TestLsaSpGetInfo(
    IN PLSA_FUNCS lsa)
{
    NTSTATUS res;
    SecPkgInfoW info;
    RtlZeroMemory(&info, sizeof(info));

    trace("testing GetInfo.\n");

    res = lsa->fnTable->GetInfo(&info);
    ok(res == STATUS_SUCCESS, "GetInfo failed with NTSTATUS 0x%lx.\n", res);
    ok(info.fCapabilities == 0x2b37, "info.fCapabilities != 0x2b37 (is 0x%lx)\n", info.fCapabilities);
    ok(info.wVersion == 1, "info.wVersion != 1 (is %i)\n", info.wVersion);
    ok(info.wRPCID == 10, "info.wRPCID != 10 (is %i)\n", info.wVersion);
    ok(wcscmp(info.Name, L"NTLM") == 0, "info.Name != NTLM (is %S)\n", info.Name);
}

static void TestLsaSpGetExtendedInformation(
    IN PLSA_FUNCS lsa)
{
    NTSTATUS res;
    int i;
    PSECPKG_EXTENDED_INFORMATION pInfo;
    SECPKG_EXTENDED_INFORMATION_CLASS Class;

    char *ClassName;
    char *CLASSNAMES[] = {"SecpkgGssInfo", "SecpkgContextThunks",
                          "SecpkgMutualAuthLevel", "SecpkgWowClientDll",
                          "SecpkgExtraOids", "SecpkgMaxInfo",
                          "SecpkgNego2Info" };

    trace("testing GetExtendedInformation.\n");

    for (i = 0; i < SecpkgMaxInfo; i++)
    {
        pInfo = NULL;
        Class = (SECPKG_EXTENDED_INFORMATION_CLASS)i+1;
        ClassName = i <= ARRAY_SIZE(CLASSNAMES) ? CLASSNAMES[i] : "unknown class";

        trace("testing GetExtendedInformation(%s, ...)\n", ClassName);

        res = lsa->fnTable->GetExtendedInformation(Class, &pInfo);

        if ((Class == SecpkgContextThunks) ||
            (Class == SecpkgWowClientDll))
            ok(res == STATUS_SUCCESS, "GetExtendedInformation(%s, ...) failed!\n", ClassName);
        else
            ok(res != STATUS_SUCCESS, "GetExtendedInformation(%s, ...) unexpected succeded!\n", ClassName);

        if (res == STATUS_SUCCESS)
        {
            PrintHexDump(sizeof(*pInfo), (PBYTE)pInfo);
            LsaFnFreePrivateHeap(pInfo);
        }
    }
}

static void TestLsaSpSetExtendedInformation(
    IN PLSA_FUNCS lsa)
{
    NTSTATUS res;
    SECPKG_EXTENDED_INFORMATION Info;

    trace("testing SetExtendedInformation.\n");

    RtlZeroMemory(&Info, sizeof(Info));

    Info.Class = SecpkgMutualAuthLevel;

    // it seems SetExtendedInformation always returns OK
    // if class is SecpkgMutualAuthLevel
    // so it's no fun to test
    Info.Info.MutualAuthLevel.MutualAuthLevel = 1;
    res = lsa->fnTable->SetExtendedInformation(SecpkgMutualAuthLevel, &Info);
    ok(res == STATUS_SUCCESS, "SetExtendedInformation(SecpkgMutualAuthLevel, ...) failed!\n");
}

static void TestLsaApInitializePackage(
    _In_ PLSA_FUNCS lsa)
{
    NTSTATUS res;
    PLSA_STRING pAuthenticationPackageName = NULL;

    trace("testing ApInitializePackage.\n");

    // * On WinXp lsa->fnTable->InitializePackage is NULL
    //   so we have to use the pointer returned by GetProcAddress
    // * PLSA_DISPATCH_TABLE is a subset of LSA_SECPKG_FUNCTION_TABLE.
    //   However LsaApInitializePackage (it seems) expects more than
    //   PLSA_DISPATCH_TABLE. Passing only PLSA_DISPATCH_TABLE would fail.
    res = lsa->LsaApInitializePackage(LSA_AUTH_PACKAGE_ID, (PLSA_DISPATCH_TABLE)&LsaFnTable, NULL, NULL,
                                      &pAuthenticationPackageName);
    ok(res == STATUS_SUCCESS, "InitializePackage failed!\n");

    ok(pAuthenticationPackageName != NULL,"pAuthenticationPackageName is NULL\n");
    if (pAuthenticationPackageName)
        ok(strcmp(pAuthenticationPackageName->Buffer, MSV1_0_PACKAGE_NAME) == 0,
           "AuthenticationPackageName is %s, expected %s.",
           pAuthenticationPackageName->Buffer, MSV1_0_PACKAGE_NAME);

    LsaFnFreeLsaHeap(pAuthenticationPackageName->Buffer);
    LsaFnFreeLsaHeap(pAuthenticationPackageName);
}

START_TEST(msv1_0)
{
    LSA_FUNCS lsa;

    LsaFnInit();
    InitFnTable();

    lsa.hMsv1_0Dll = GetModuleHandle(MSV1_0_DLL);
    if (lsa.hMsv1_0Dll == NULL)
    {
        ok(FALSE, "failed to get module handle of %S.\n", MSV1_0_DLL);
        goto done;
    }

    lsa.SpLsaModeInitialize = (SpLsaModeInitializeFn)GetProcAddress(lsa.hMsv1_0Dll, "SpLsaModeInitialize");
    if (lsa.SpLsaModeInitialize == NULL)
    {
        ok(FALSE, "failed to get proc address for SpLsaModeInitialize\n");
        goto done;
    }

    lsa.LsaApInitializePackage = (PLSA_AP_INITIALIZE_PACKAGE)GetProcAddress(lsa.hMsv1_0Dll, "LsaApInitializePackage");
    if (lsa.LsaApInitializePackage == NULL)
    {
        ok(FALSE, "failed to get proc address for LsaApInitializePackage\n");
        return;
    }

    TestLsaModeInitialize(&lsa);

    #if 0
    TestLsaSpInitialize(&lsa);
    #endif

    TestLsaSpGetInfo(&lsa);

    TestLsaSpGetExtendedInformation(&lsa);

    TestLsaSpSetExtendedInformation(&lsa);

    TestLsaApInitializePackage(&lsa);

done:
    LsaFnFini();
}
