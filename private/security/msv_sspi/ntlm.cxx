//+--------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1996
//
// File:       ntlm.cxx
//
// Contents:   main entrypoints for the ntlm security package
//               SpLsaModeInitialize
//               SpInitialize
//               SpShutdown
//               SpGetInfo
//
//             Helper functions:
//               NtLmSetPolicyInfo
//               NtLmPolicyChangeCallback
//               NtLmRegisterForPolicyChange
//               NtLmUnregisterForPolicyChange
//
// History:    ChandanS  26-Jul-1996   Stolen from kerberos\client2\kerberos.cxx
//             ChandanS  16-Apr-1998   No reboot on domain name change
//
//---------------------------------------------------------------------


// Variables with the EXTERN storage class are declared here
#define NTLM_GLOBAL
#define DEBUG_ALLOCATE

#include <global.h>

// BUGBUG Should be in the SDK?
#define MSV1_0_PACKAGE_NAMEW     L"MICROSOFT_AUTHENTICATION_PACKAGE_V1_0"
BOOLEAN NtLmCredentialInitialized;
BOOLEAN NtLmContextInitialized;
BOOLEAN NtLmRNGInitialized;

//+--------------------------------------------------------------------
//
//  Function:   SpLsaModeInitialize
//
//  Synopsis:   This function is called by the LSA when this DLL is loaded.
//              It returns security package function tables for all
//              security packages in the DLL.
//
//  Arguments:  LsaVersion - Version number of the LSA
//              PackageVersion - Returns version number of the package
//              Tables - Returns array of function tables for the package
//              TableCount - Returns number of entries in array of
//                      function tables.
//
//  Returns:    PackageVersion (as above)
//              Tables (as above)
//              TableCount (as above)
//
//  Notes:
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpLsaModeInitialize(
    IN ULONG LsaVersion,
    OUT PULONG PackageVersion,
    OUT PSECPKG_FUNCTION_TABLE * Tables,
    OUT PULONG TableCount
    )
{
#if DBG
//     SspGlobalDbflag = SSP_CRITICAL| SSP_API| SSP_API_MORE |SSP_INIT| SSP_MISC | SSP_NO_LOCAL;
    SspGlobalDbflag = SSP_CRITICAL ;
    InitializeCriticalSection(&SspGlobalLogFileCritSect);
#endif
    SspPrint((SSP_API, "Entering SpLsaModeInitialize\n"));

    SECURITY_STATUS Status = SEC_E_OK;

    if (LsaVersion != SECPKG_INTERFACE_VERSION)
    {
        SspPrint((SSP_CRITICAL, "Invalid LSA version: %d\n", LsaVersion));
        Status = STATUS_INVALID_PARAMETER;
        goto CleanUp;
    }

    NtLmFunctionTable.InitializePackage        = NULL;
    NtLmFunctionTable.LogonUser                = NULL;
    NtLmFunctionTable.CallPackage              = LsaApCallPackage;
    NtLmFunctionTable.LogonTerminated          = LsaApLogonTerminated;
    NtLmFunctionTable.CallPackageUntrusted     = LsaApCallPackageUntrusted;
    NtLmFunctionTable.LogonUserEx              = NULL;
    NtLmFunctionTable.LogonUserEx2             = LsaApLogonUserEx2;
    NtLmFunctionTable.Initialize               = SpInitialize;
    NtLmFunctionTable.Shutdown                 = SpShutdown;
    NtLmFunctionTable.GetInfo                  = SpGetInfo;
    NtLmFunctionTable.AcceptCredentials        = SpAcceptCredentials;
    NtLmFunctionTable.AcquireCredentialsHandle = SpAcquireCredentialsHandle;
    NtLmFunctionTable.FreeCredentialsHandle    = SpFreeCredentialsHandle;
    NtLmFunctionTable.SaveCredentials          = SpSaveCredentials;
    NtLmFunctionTable.GetCredentials           = SpGetCredentials;
    NtLmFunctionTable.DeleteCredentials        = SpDeleteCredentials;
    NtLmFunctionTable.InitLsaModeContext       = SpInitLsaModeContext;
    NtLmFunctionTable.AcceptLsaModeContext     = SpAcceptLsaModeContext;
    NtLmFunctionTable.DeleteContext            = SpDeleteContext;
    NtLmFunctionTable.ApplyControlToken        = SpApplyControlToken;
    NtLmFunctionTable.GetUserInfo              = SpGetUserInfo;
    NtLmFunctionTable.QueryCredentialsAttributes = SpQueryCredentialsAttributes ;
    NtLmFunctionTable.GetExtendedInformation   = SpGetExtendedInformation ;
    NtLmFunctionTable.SetExtendedInformation   = SpSetExtendedInformation ;
    NtLmFunctionTable.CallPackagePassthrough   = LsaApCallPackagePassthrough;


    *PackageVersion = SECPKG_INTERFACE_VERSION;
    *Tables = &NtLmFunctionTable;
    *TableCount = 1;

CleanUp:

    SspPrint((SSP_API, "Leaving SpLsaModeInitialize\n"));

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmSetPolicyInfo
//
//  Synopsis:   Function to be called when policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//        if fInit is TRUE, this is called by the init routine in ntlm
//
//
//+-------------------------------------------------------------------------
NTSTATUS
NtLmSetPolicyInfo(
    IN PUNICODE_STRING DnsComputerName,
    IN PUNICODE_STRING ComputerName,
    IN PUNICODE_STRING DnsDomainName,
    IN PUNICODE_STRING DomainName,
    IN PSID DomainSid,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass,
    IN BOOLEAN fInit
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    // Buffers to delete on cleanup

    CHAR   *ComputerNameAnsi    = NULL;
    CHAR   *DomainNameAnsi    = NULL;
    //
    // Do this only if this is package init
    //

    EnterCriticalSection(&NtLmGlobalCritSect);

    if (fInit)
    {
        if (ComputerName && ComputerName->Buffer != NULL)
        {
            ULONG cLength = ComputerName->Length / sizeof(WCHAR);

            if ((ComputerName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeComputerName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad computer name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeComputerName,
               ComputerName->Buffer,
               cLength);

            NtLmGlobalUnicodeComputerName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeComputerNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeComputerNameString,
                                   NtLmGlobalUnicodeComputerName );

            // Save old buffers for deleting
            ComputerNameAnsi = NtLmGlobalOemComputerNameString.Buffer;

            Status = RtlUpcaseUnicodeStringToOemString(
                        &NtLmGlobalOemComputerNameString,
                        &NtLmGlobalUnicodeComputerNameString,
                        TRUE );

            if ( !NT_SUCCESS(Status) ) {
                Status = STATUS_SUCCESS;
                //SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Error from RtlUpcaseUnicodeStringToOemString is %d\n", Status));
                ComputerNameAnsi = NULL;
                // goto CleanUp;
            }

        }
    }

    //
    // Initialize various forms of the primary domain name of the local system
    // Do this only if this is package init or it's DnsDomain info
    //

    if (fInit || (ChangedInfoClass == PolicyNotifyDnsDomainInformation))
    {
        if (DnsComputerName && DnsComputerName->Buffer != NULL ) {
            ULONG cLength = DnsComputerName->Length / sizeof(WCHAR);

            if((DnsComputerName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeDnsComputerName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad computer name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeDnsComputerName,
               DnsComputerName->Buffer,
               cLength);

            NtLmGlobalUnicodeDnsComputerName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeDnsComputerNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeDnsComputerNameString,
                                   NtLmGlobalUnicodeDnsComputerName );
        }

        if (DnsDomainName && DnsDomainName->Buffer != NULL ) {
            ULONG cLength = DnsDomainName->Length / sizeof(WCHAR);

            if((DnsDomainName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodeDnsDomainName))
            {
                // Bad ComputerName
                Status = STATUS_INVALID_COMPUTER_NAME;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad domain name length is %d\n", cLength));
                goto CleanUp;
            }

            wcsncpy(NtLmGlobalUnicodeDnsDomainName,
               DnsDomainName->Buffer,
               cLength);

            NtLmGlobalUnicodeDnsDomainName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodeDnsDomainNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodeDnsDomainNameString,
                                   NtLmGlobalUnicodeDnsDomainName );
        }

        if (DomainName && DomainName->Buffer != NULL)
        {
            ULONG cLength = DomainName->Length / sizeof(WCHAR);

            if ((DomainName->Length + sizeof(WCHAR)) > sizeof(NtLmGlobalUnicodePrimaryDomainName))
            {
                Status = STATUS_NAME_TOO_LONG;
                SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Bad domain name length is %d\n", cLength));
                goto CleanUp;
            }
            wcsncpy(NtLmGlobalUnicodePrimaryDomainName,
               DomainName->Buffer,
               cLength);
            NtLmGlobalUnicodePrimaryDomainName[cLength] = UNICODE_NULL;

            // make NtlmGlobalUnicodePrimaryDomainNameString a string form

            RtlInitUnicodeString(  &NtLmGlobalUnicodePrimaryDomainNameString,
                                   NtLmGlobalUnicodePrimaryDomainName );

            // Save old buffers for deleting
            DomainNameAnsi = NtLmGlobalOemPrimaryDomainNameString.Buffer;

            Status = RtlUpcaseUnicodeStringToOemString(
                        &NtLmGlobalOemPrimaryDomainNameString,
                        &NtLmGlobalUnicodePrimaryDomainNameString,
                        TRUE );

            if ( !NT_SUCCESS(Status) ) {
                // SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Error from RtlUpcaseUnicodeStringToOemString is %d\n", Status));
                DomainNameAnsi = NULL;
                // goto CleanUp;
                Status = STATUS_SUCCESS;
            }
        }
    }

    //
    // If this is a standalone windows NT workstation,
    // use the computer name as the Target name.
    //

    if ( DomainSid != NULL)
    {
        NtLmGlobalUnicodeTargetName = NtLmGlobalUnicodePrimaryDomainNameString;
        NtLmGlobalOemTargetName = NtLmGlobalOemPrimaryDomainNameString;
        NtLmGlobalTargetFlags = NTLMSSP_TARGET_TYPE_DOMAIN;
    }
    else
    {
        NtLmGlobalUnicodeTargetName = NtLmGlobalUnicodeComputerNameString;
        NtLmGlobalOemTargetName = NtLmGlobalOemComputerNameString;
        NtLmGlobalTargetFlags = NTLMSSP_TARGET_TYPE_SERVER;
    }

    //
    // initialize the GlobalNtlm3 targetinfo.
    //

    {
        PMSV1_0_AV_PAIR pAV;
        PUNICODE_STRING pDnsTargetName;
        PUNICODE_STRING pDnsComputerName;
        ULONG cbAV;

        if( NtLmGlobalNtLm3TargetInfo.Buffer != NULL )
        {
            NtLmFree(NtLmGlobalNtLm3TargetInfo.Buffer);
        }

        if( NtLmGlobalTargetFlags == NTLMSSP_TARGET_TYPE_DOMAIN ) {
            pDnsTargetName = &NtLmGlobalUnicodeDnsDomainNameString;
        } else {
            pDnsTargetName = &NtLmGlobalUnicodeDnsComputerNameString;
        }

        pDnsComputerName = &NtLmGlobalUnicodeDnsComputerNameString;

        cbAV = NtLmGlobalUnicodeTargetName.Length +
               NtLmGlobalUnicodeComputerNameString.Length +
               pDnsComputerName->Length +
               pDnsTargetName->Length +
               64 ;

        NtLmGlobalNtLm3TargetInfo.Buffer = (PWSTR)NtLmAllocate( cbAV );

        if( NtLmGlobalNtLm3TargetInfo.Buffer == NULL )
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            SspPrint((SSP_CRITICAL, "NtLmSetPolicyInfo, Error from NtLmAllocate\n"));
            goto CleanUp;
        }

        pAV = MsvpAvlInit( NtLmGlobalNtLm3TargetInfo.Buffer );
        MsvpAvlAdd( pAV, MsvAvNbDomainName, &NtLmGlobalUnicodeTargetName, cbAV );
        MsvpAvlAdd( pAV, MsvAvNbComputerName, &NtLmGlobalUnicodeComputerNameString, cbAV );

        if( pDnsTargetName->Length != 0 && pDnsTargetName->Buffer != NULL )
            MsvpAvlAdd( pAV, MsvAvDnsDomainName, pDnsTargetName, cbAV );

        if( pDnsComputerName->Length != 0 && pDnsComputerName->Buffer != NULL )
            MsvpAvlAdd( pAV, MsvAvDnsComputerName, &NtLmGlobalUnicodeDnsComputerNameString, cbAV );

        NtLmGlobalNtLm3TargetInfo.Length = (USHORT)MsvpAvlLen( pAV, cbAV );
    }

CleanUp:

    LeaveCriticalSection(&NtLmGlobalCritSect);

    if (ComputerNameAnsi)
    {
        NtLmFree(ComputerNameAnsi);
    }

    if (DomainNameAnsi)
    {
        NtLmFree(DomainNameAnsi);
    }


    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmPolicyChangeCallback
//
//  Synopsis:   Function to be called when domain policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID NTAPI
NtLmPolicyChangeCallback(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLSAPR_POLICY_INFORMATION Policy = NULL;


    switch (ChangedInfoClass)
    {
        case PolicyNotifyDnsDomainInformation:
        {

            WCHAR UnicodeDnsComputerName[DNS_MAX_NAME_LENGTH + 1];
            UNICODE_STRING UnicodeDnsComputerNameString;
            ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);

            //
            // Get the new domain information
            //


            Status = I_LsaIQueryInformationPolicyTrusted(
                        PolicyDnsDomainInformation,
                        &Policy
                        );

            if (!NT_SUCCESS(Status))
            {
                SspPrint((SSP_CRITICAL, "NtLmPolicyChangeCallback, Error from I_LsaIQueryInformationPolicyTrusted is %d\n", Status));
                goto Cleanup;
            }

            //
            // get the new DNS computer name
            //

            if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                                      UnicodeDnsComputerName,
                                      &DnsComputerNameLength ) )
            {
                UnicodeDnsComputerName[ 0 ] = L'\0';
            }

            RtlInitUnicodeString(  &UnicodeDnsComputerNameString,
                               UnicodeDnsComputerName);


            Status = NtLmSetPolicyInfo(
                        &UnicodeDnsComputerNameString,
                        NULL,
                        (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.DnsDomainName,
                        (PUNICODE_STRING) &Policy->PolicyDnsDomainInfo.Name,
                        (PSID) Policy->PolicyDnsDomainInfo.Sid,
                        ChangedInfoClass,
                        FALSE);

            if (!NT_SUCCESS(Status))
            {
                SspPrint((SSP_CRITICAL, "NtLmPolicyChangeCallback, Error from NtLmSetDomainName is %d\n", Status));
                goto Cleanup;
            }
        }
        break;
        default:
        break;
    }


Cleanup:

    if (Policy != NULL)
    {
        switch (ChangedInfoClass)
        {
            case PolicyNotifyDnsDomainInformation:
            {
                I_LsaIFree_LSAPR_POLICY_INFORMATION(
                    PolicyDnsDomainInformation,
                    Policy
                    );
            }
            break;
            default:
            break;
        }
    }
    return;

}


//+-------------------------------------------------------------------------
//
//  Function:   NtLmRegisterForPolicyChange
//
//  Synopsis:   Register with the LSA to be notified of policy changes
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


NTSTATUS
NtLmRegisterForPolicyChange(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    Status = I_LsaIRegisterPolicyChangeNotificationCallback(
                NtLmPolicyChangeCallback,
                ChangedInfoClass
                );
    if (!NT_SUCCESS(Status))
    {
        SspPrint((SSP_CRITICAL, "NtLmRegisterForPolicyChange, Error from I_LsaIRegisterPolicyChangeNotificationCallback is %d\n", Status));
    }
    SspPrint((SSP_MISC, "I_LsaIRegisterPolicyChangeNotificationCallback called with %d\n", ChangedInfoClass));
    return(Status);

}

//+-------------------------------------------------------------------------
//
//  Function:   NtLmUnregisterForPolicyChange
//
//  Synopsis:   Unregister for policy change notification
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//
//--------------------------------------------------------------------------


VOID
NtLmUnregisterForPolicyChange(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    )
{
    (VOID) I_LsaIUnregisterPolicyChangeNotificationCallback(
                NtLmPolicyChangeCallback,
                ChangedInfoClass
                );

}


//+--------------------------------------------------------------------
//
//  Function:   SpInitialize
//
//  Synopsis:   Initializes the Security package
//
//  Arguments:  PackageId - Contains ID for this package assigned by LSA
//              Parameters - Contains machine-specific information
//              FunctionTable - Contains table of LSA helper routines
//
//  Returns: None
//
//  Notes: Everything that was done in LsaApInitializePackage
//         should be done here. Lsa assures us that only
//         one thread is executing this at a time. Don't
//         have to worry about concurrency problems.(BUGBUG verify)
//         Most of the stuff was taken from SspCommonInitialize()
//         from svcdlls\ntlmssp\common\initcomn.c
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpInitialize(
    IN ULONG_PTR PackageId,
    IN PSECPKG_PARAMETERS Parameters,
    IN PLSA_SECPKG_FUNCTION_TABLE FunctionTable
    )
{
    SspPrint((SSP_API, "Entering SpInitialize\n"));

    SECURITY_STATUS Status = SEC_E_OK;
    WCHAR UnicodeComputerName[CNLEN + 1];
    UNICODE_STRING UnicodeComputerNameString;
    ULONG ComputerNameLength =
        (sizeof(UnicodeComputerName)/sizeof(WCHAR));

    WCHAR UnicodeDnsComputerName[DNS_MAX_NAME_LENGTH + 1];
    UNICODE_STRING UnicodeDnsComputerNameString;
    ULONG DnsComputerNameLength = sizeof(UnicodeDnsComputerName) / sizeof(WCHAR);

    //
    // Init the global crit section
    //

    InitializeCriticalSection(&NtLmGlobalCritSect);

    //
    // All the following are global
    //

    NtLmState                  = NtLmLsaMode;
    NtLmPackageId              = PackageId;



    // We really need this to be a day less than maxtime so when callers
    // of sspi convert to utc, they won't get time in the past.

    NtLmGlobalForever.HighPart = 0x7FFFFF36;
    NtLmGlobalForever.LowPart  = 0xD5969FFF;

    //
    // Following are local
    //

    NtLmCredentialInitialized = FALSE;
    NtLmContextInitialized    = FALSE;
    NtLmRNGInitialized        = FALSE;

    //
    // Save away the Lsa functions
    //

    LsaFunctions    = FunctionTable;

    //
    // Save the Parameters info
    //

    NtLmSecPkg.MachineState = Parameters->MachineState;
    NtLmSecPkg.SetupMode    = Parameters->SetupMode;


    //
    // allocate a locally unique ID rereferencing the machine logon.
    //

    Status = NtAllocateLocallyUniqueId( &NtLmGlobalLuidMachineLogon );

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtAllocateLocallyUniqueId is %d\n", Status));
        goto CleanUp;
    }

    //
    // create a logon session for the machine logon.
    //

    Status = LsaFunctions->CreateLogonSession( &NtLmGlobalLuidMachineLogon );
    if( !NT_SUCCESS(Status) ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from CreateLogonSession is %d\n", Status));
        goto CleanUp;
    }


    Status = NtLmDuplicateUnicodeString(
                                 &NtLmSecPkg.DomainName,
                                 &Parameters->DomainName);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateUnicodeString is %d\n", Status));
        goto CleanUp;
    }

    Status = NtLmDuplicateUnicodeString(
                                 &NtLmSecPkg.DnsDomainName,
                                 &Parameters->DnsDomainName);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateUnicodeString is %d\n", Status));
        goto CleanUp;
    }

    if (Parameters->DomainSid != NULL) {
        Status = NtLmDuplicateSid( &NtLmSecPkg.DomainSid,
                                   Parameters->DomainSid );


        if (!NT_SUCCESS (Status))
        {
            SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmDuplicateSid is %d\n", Status));
            goto CleanUp;
        }
    }

    //
    // Determine if this machine is running NT Workstation or NT Server
    //

    if (!RtlGetNtProductType (&NtLmGlobalNtProductType))
    {
        SspPrint((SSP_API_MORE, "RtlGetNtProductType defaults to NtProductWinNt\n"));
    }

    if ( !GetComputerNameW( UnicodeComputerName,
                            &ComputerNameLength ) ) {
        Status = STATUS_INVALID_COMPUTER_NAME;
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from GetComputerNameW is %d\n", Status));
        goto CleanUp;
    }

    if ( !GetComputerNameExW( ComputerNameDnsFullyQualified,
                              UnicodeDnsComputerName,
                              &DnsComputerNameLength ) )
    {

        //
        // per CliffV, failure is legal.
        //

        UnicodeDnsComputerName[ 0 ] = L'\0';
    }

    //
    // Set all the globals relating to computer name, domain name, sid etc.
    // This routine is also used by the callback for notifications from the lsa
    //

    RtlInitUnicodeString(  &UnicodeComputerNameString,
                           UnicodeComputerName);

    RtlInitUnicodeString(  &UnicodeDnsComputerNameString,
                           UnicodeDnsComputerName);

    Status = NtLmSetPolicyInfo( &UnicodeDnsComputerNameString,
                                &UnicodeComputerNameString,
                                &NtLmSecPkg.DnsDomainName,
                                &NtLmSecPkg.DomainName,
                                NtLmSecPkg.DomainSid,
                                PolicyNotifyAuditEventsInformation, // Ignored
                                TRUE ); // yes, package init

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmSetDomainInfo %d\n", Status));
        goto CleanUp;
    }

    //
    // pickup a copy of the Local System access token.
    //

    {
        HANDLE hProcessToken;
        NTSTATUS StatusToken;

        StatusToken = NtOpenProcessToken(
                        NtCurrentProcess(),
                        TOKEN_QUERY | TOKEN_DUPLICATE,
                        &hProcessToken
                        );

        if( NT_SUCCESS( StatusToken ) ) {

            TOKEN_STATISTICS LocalTokenStatistics;
            DWORD TokenStatisticsSize = sizeof(LocalTokenStatistics);
            LUID LogonIdSystem = SYSTEM_LUID;

            Status = NtQueryInformationToken(
                            hProcessToken,
                            TokenStatistics,
                            &LocalTokenStatistics,
                            TokenStatisticsSize,
                            &TokenStatisticsSize
                            );

            if( NT_SUCCESS( Status ) ) {

                //
                // see if it's SYSTEM.
                //

                if(RtlEqualLuid(
                                &LogonIdSystem,
                                &(LocalTokenStatistics.AuthenticationId)
                                )) {


                    Status = SspDuplicateToken(
                                    hProcessToken,
                                    SecurityImpersonation,
                                    &NtLmGlobalAccessTokenSystem
                                    );
                }
            }

            NtClose( hProcessToken );
        }
    }

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, could not acquire SYSTEM token %d\n", Status));
        goto CleanUp;
    }


    //
    // Init the Credential stuff
    //

    Status = SspCredentialInitialize();
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from SspCredentialInitializeis %d\n", Status));
        goto CleanUp;
    }
    NtLmCredentialInitialized = TRUE;

    //
    // Init the Context stuff
    //
    Status = SspContextInitialize();
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from SspContextInitializeis %d\n", Status));
        goto CleanUp;
    }
    NtLmContextInitialized = TRUE;

    //
    // Get the locale and check if it is FRANCE, which doesn't allow
    // encryption
    //

    NtLmGlobalEncryptionEnabled = IsEncryptionPermitted();

    //
    // Init the random number generator stuff
    //

    if( !NtLmInitializeRNG() ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmInitializeRNG\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto CleanUp;
    }
    NtLmRNGInitialized = TRUE;

    if( !NtLmInitializeProtectedMemory() ) {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmInitializeProtectedMemory\n"));
        Status = STATUS_UNSUCCESSFUL;
        goto CleanUp;
    }

    NtLmCheckLmCompatibility();

    NtLmQueryMappedDomains();

    // Do the Init stuff for the MSV authentication package
    // Passing FunctionTable as a (PLSA_DISPATCH_TABLE).
    // Well, the first 11 entries are the same. Cheating a
    // bit.

    Status = LsaApInitializePackage(
                      (ULONG) PackageId,
                      (PLSA_DISPATCH_TABLE)FunctionTable,
                      NULL,
                      NULL,
                      NULL);

    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from LsaApInitializePackage is %d\n", Status));
        goto CleanUp;
    }

    Status = NtLmRegisterForPolicyChange(PolicyNotifyDnsDomainInformation);
    if (!NT_SUCCESS (Status))
    {
        SspPrint((SSP_CRITICAL, "SpInitialize, Error from NtLmRegisterForPolicyChange is %d\n", Status));
        goto CleanUp;
    }


CleanUp:

    if (!NT_SUCCESS (Status))
    {
        SpShutdown();
    }

    SspPrint((SSP_API, "Leaving SpInitialize\n"));

    return(SspNtStatusToSecStatus(Status, SEC_E_INTERNAL_ERROR));
}

//+--------------------------------------------------------------------
//
//  Function:   SpShutdown
//
//  Synopsis:   Exported function to shutdown the Security package.
//
//  Effects:    Forces the freeing of all credentials, contexts
//              and frees all global data
//
//  Arguments:  none
//
//  Returns:
//
//  Notes:      SEC_E_OK in all cases
//         Most of the stuff was taken from SspCommonShutdown()
//         from svcdlls\ntlmssp\common\initcomn.c
//
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpShutdown(
    VOID
    )
{
    SspPrint((SSP_API, "Entering SpShutdown\n"));

    //
    // comment out LSA mode cleanup code, per NTBUG 400026,
    // which can result in access violations during shutdown when
    // calls into package are still occuring during shutdown.
    //

#if 0

    if (NtLmContextInitialized)
    {
        SspContextTerminate();
        NtLmContextInitialized = FALSE;
    }

    if (NtLmCredentialInitialized)
    {
        SspCredentialTerminate();
        NtLmCredentialInitialized = FALSE;
    }

    if (NtLmGlobalOemComputerNameString.Buffer != NULL)
    {
        RtlFreeOemString(&NtLmGlobalOemComputerNameString);
        NtLmGlobalOemComputerNameString.Buffer = NULL;
    }

    if (NtLmGlobalOemPrimaryDomainNameString.Buffer != NULL)
    {
        RtlFreeOemString(&NtLmGlobalOemPrimaryDomainNameString);
        NtLmGlobalOemPrimaryDomainNameString.Buffer = NULL;
    }

    if (NtLmGlobalNtLm3TargetInfo.Buffer != NULL)
    {
        NtLmFree (NtLmGlobalNtLm3TargetInfo.Buffer);
        NtLmGlobalNtLm3TargetInfo.Buffer = NULL;
    }

    if ( NtLmSecPkg.DomainName.Buffer != NULL )
    {
        NtLmFree (NtLmSecPkg.DomainName.Buffer);
    }

    if ( NtLmSecPkg.DnsDomainName.Buffer != NULL )
    {
        NtLmFree (NtLmSecPkg.DnsDomainName.Buffer);
    }

    if ( NtLmSecPkg.DomainSid != NULL )
    {
        NtLmFree (NtLmSecPkg.DomainSid);
    }

    if (NtLmGlobalLocalSystemSid != NULL)
    {
        FreeSid( NtLmGlobalLocalSystemSid);
        NtLmGlobalLocalSystemSid = NULL;
    }

    if (NtLmGlobalAliasAdminsSid != NULL)
    {
        FreeSid( NtLmGlobalAliasAdminsSid);
        NtLmGlobalAliasAdminsSid = NULL;
    }

    if (NtLmRNGInitialized)
    {
        NtLmCleanupRNG();
        NtLmRNGInitialized = FALSE;
    }

    NtLmFreeMappedDomains();

    NtLmUnregisterForPolicyChange(PolicyNotifyDnsDomainInformation);

    if (NtLmGlobalAccessTokenSystem != NULL) {
        NtClose( NtLmGlobalAccessTokenSystem );
        NtLmGlobalAccessTokenSystem = NULL;
    }

    DeleteCriticalSection(&NtLmGlobalCritSect);
    SspPrint((SSP_API, "Leaving SpShutdown\n"));
#if DBG
    DeleteCriticalSection(&SspGlobalLogFileCritSect);
#endif

#endif  // NTBUG 400026

    NtLmCleanupProtectedMemory();

    return(SEC_E_OK);
}

//+--------------------------------------------------------------------
//
//  Function:   SpGetInfo
//
//  Synopsis:   Returns information about the package
//
//  Effects:    returns pointers to global data
//
//  Arguments:  PackageInfo - Receives security package information
//
//  Returns:    SEC_E_OK in all cases
//
//  Notes:      Pointers to constants ok. Lsa will copy the data
//              before sending it to someone else
//
//---------------------------------------------------------------------
NTSTATUS NTAPI
SpGetInfo(
    OUT PSecPkgInfo PackageInfo
    )
{
    SspPrint((SSP_API, "Entering SpGetInfo\n"));

    // BUGBUG Do we remove the PRIVACY flags if Encryption is not enabled?
    PackageInfo->fCapabilities    = NTLMSP_CAPS;
    PackageInfo->wVersion         = SECURITY_SUPPORT_PROVIDER_INTERFACE_VERSION;
    PackageInfo->wRPCID           = RPC_C_AUTHN_WINNT;
    PackageInfo->cbMaxToken       = NTLMSP_MAX_TOKEN_SIZE;
    PackageInfo->Name             = NTLMSP_NAME;
    PackageInfo->Comment          = NTLMSP_COMMENT;

    SspPrint((SSP_API, "Leaving SpGetInfo\n"));

    return(SEC_E_OK);
}

NTSTATUS
NTAPI
SpGetExtendedInformation(
    IN  SECPKG_EXTENDED_INFORMATION_CLASS Class,
    OUT PSECPKG_EXTENDED_INFORMATION * ppInformation
    )
{
    return SEC_E_UNSUPPORTED_FUNCTION ;
}

NTSTATUS
NTAPI
SpSetExtendedInformation(
    IN SECPKG_EXTENDED_INFORMATION_CLASS Class,
    IN PSECPKG_EXTENDED_INFORMATION Info
    )
{
    NTSTATUS Status ;


    switch ( Class )
    {
        case SecpkgMutualAuthLevel:
            NtLmGlobalMutualAuthLevel = Info->Info.MutualAuthLevel.MutualAuthLevel ;
            Status = SEC_E_OK ;
            break;

        default:
            Status = SEC_E_UNSUPPORTED_FUNCTION ;
            break;
    }

    return Status ;
}



VOID
NtLmCheckLmCompatibility(
    )
/*++

Routine Description:

    This routine checks to see if we should support the LM challenge
    response protocol by looking in the registry under
    system\currentcontrolset\Control\Lsa\LmCompatibilityLevel. The level
    indicates whether to send the LM reponse by default and whether to
    ever send the LM response

Arguments:

    none.

Return Value:

    None

--*/
{
    NTSTATUS NtStatus;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;

    //
    // initialize defaults
    // Assume that LM is supported.
    //

    NtLmGlobalLmProtocolSupported = 0;
    NtLmGlobalRequireNtlm2 = FALSE;
    NtLmGlobalDatagramUse128BitEncryption = FALSE;
    NtLmGlobalDatagramUse56BitEncryption = FALSE;


    //
    // Open the Lsa key in the registry
    //

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return;
    }

    //
    // save away registry key so we can use it for notification events.
    //

    NtLmGlobalLsaKey = (HKEY)KeyHandle;



    // now open the MSV1_0 subkey...

    RtlInitUnicodeString(
        &KeyName,
        L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Lsa\\Msv1_0"
        );

    InitializeObjectAttributes(
        &ObjectAttributes,
        &KeyName,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    NtStatus = NtOpenKey(
                &KeyHandle,
                KEY_READ,
                &ObjectAttributes
                );

    if (!NT_SUCCESS(NtStatus)) {
        return;
    }

    //
    // save away registry key so we can use it for notification events.
    //

    NtLmGlobalLsaMsv1_0Key = (HKEY)KeyHandle;



}

ULONG
NtLmValidMinimumSecurityFlagsMask(
    IN  ULONG   MinimumSecurity
    )
/*++

    This routine takes a NtLmMinimumClientSec or NtLmMinimumServerSec registry
    value and masks off the bits that are not relevant for enforcing the
    supported options.

--*/
{

    return (MinimumSecurity & (
                    NTLMSSP_NEGOTIATE_UNICODE |
                    NTLMSSP_NEGOTIATE_SIGN |
                    NTLMSSP_NEGOTIATE_SEAL |
                    NTLMSSP_NEGOTIATE_NTLM2 |
                    NTLMSSP_NEGOTIATE_128 |
                    NTLMSSP_NEGOTIATE_KEY_EXCH |
                    NTLMSSP_NEGOTIATE_56
                    ));

}

VOID
NTAPI
NtLmQueryDynamicGlobals(
    PVOID pvContext,
    BOOLEAN f
    )
{
    SspPrint((SSP_API, "Entering NtLmQueryDynamicGlobals\n"));

    HKEY KeyHandle;     // open registry key to Lsa\MSV1_0
    LONG RegStatus;

    DWORD RegValueType;
    DWORD RegValue;
    DWORD RegValueSize;

    KeyHandle = NtLmGlobalLsaKey;

    if( KeyHandle != NULL )
    {
        //
        // lm compatibility level.
        //

        RegValueSize = sizeof( RegValue );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"LmCompatibilityLevel",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalLmProtocolSupported = (ULONG)RegValue;
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalLmProtocolSupported = 0;
        }

    }



    KeyHandle = NtLmGlobalLsaMsv1_0Key;

    if( KeyHandle != NULL )
    {
        //
        // get minimum client security flag.
        //

        RegValueSize = sizeof( RegValue );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"NtlmMinClientSec",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalMinimumClientSecurity =
                    NtLmValidMinimumSecurityFlagsMask( (ULONG)RegValue );
            }
        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalMinimumClientSecurity = 0 ;
        }

        //
        // get minimum server security flags.
        //

        RegValueSize = sizeof( RegValueSize );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"NtlmMinServerSec",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                NtLmGlobalMinimumServerSecurity =
                    NtLmValidMinimumSecurityFlagsMask( (ULONG)RegValue );
            }

        } else if( RegStatus == ERROR_FILE_NOT_FOUND ) {

            //
            // value was deleted - resort to default.
            //

            NtLmGlobalMinimumServerSecurity = 0;
        }

        //
        // All datagram related flags need to be set.
        //

        if (NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_NTLM2)
        {
            NtLmGlobalRequireNtlm2 = TRUE;
        }

        if ((NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_128) &&
            (NtLmSecPkg.MachineState & SECPKG_STATE_STRONG_ENCRYPTION_PERMITTED))
        {
            NtLmGlobalDatagramUse128BitEncryption = TRUE;
        } else if (NtLmGlobalMinimumClientSecurity & NTLMSSP_NEGOTIATE_56) {
            NtLmGlobalDatagramUse56BitEncryption = TRUE;
        }

#if DBG


        //
        // get the debugging flag
        //


        RegValueSize = sizeof( RegValueSize );

        RegStatus = RegQueryValueExW(
                        KeyHandle,
                        L"DBFlag",
                        NULL,
                        &RegValueType,
                        (PUCHAR)&RegValue,
                        &RegValueSize
                        );


        if ( RegStatus == ERROR_SUCCESS ) {

            //
            // Check that the data is the correct size and type - a ULONG.
            //

            if ((RegValueSize >= sizeof(ULONG)) &&
                (RegValueType == REG_DWORD)) {

                SspGlobalDbflag = (ULONG)RegValue;
            }

        }

#endif

    }



    //
    // (re)register the wait events.
    //

    if( NtLmGlobalRegChangeNotifyEvent )
    {
        if( NtLmGlobalLsaKey )
        {
            RegNotifyChangeKeyValue(
                            NtLmGlobalLsaKey,
                            FALSE,
                            REG_NOTIFY_CHANGE_LAST_SET,
                            NtLmGlobalRegChangeNotifyEvent,
                            TRUE
                            );
        }

#if DBG
        if( NtLmGlobalLsaMsv1_0Key )
        {
            RegNotifyChangeKeyValue(
                            NtLmGlobalLsaMsv1_0Key,
                            FALSE,
                            REG_NOTIFY_CHANGE_LAST_SET,
                            NtLmGlobalRegChangeNotifyEvent,
                            TRUE
                            );
        }
#endif

    }


    SspPrint((SSP_API, "Leaving NtLmQueryDynamicGlobals\n"));

    return;
}


VOID
NtLmQueryMappedDomains(
    VOID
    )
{
    HKEY KeyHandle;     // open registry key to Lsa\MSV1_0
    LONG RegStatus;
    DWORD RegValueType;
    WCHAR RegDomainName[DNS_MAX_NAME_LENGTH+1];
    DWORD RegDomainSize;


    //
    // register the workitem that waits for the RegChangeNotifyEvent
    // to be signalled.  This supports dynamic refresh of configuration
    // parameters.
    //

    NtLmGlobalRegChangeNotifyEvent = CreateEvent( NULL, FALSE, FALSE, NULL );

    //
    // query the globals once prior to registering the wait
    // if a registry change occurs, the globals will be re-read by the worker
    // thread.
    //

    NtLmQueryDynamicGlobals( NULL, FALSE );

    NtLmGlobalRegWaitObject = RegisterWaitForSingleObjectEx(
                                    NtLmGlobalRegChangeNotifyEvent,
                                    NtLmQueryDynamicGlobals,
                                    NULL,
                                    INFINITE,
                                    0 // dwFlags
                                    );

    KeyHandle = NtLmGlobalLsaMsv1_0Key;

    if( KeyHandle == NULL )
        return;


    //
    // we only support loading the following globals once during initialization;
    // they are not re-read until next reboot.
    //



    //
    // Check the registry for a domain name to map
    //

    RegDomainSize = sizeof( RegDomainName );
    RegStatus = RegQueryValueExW(
                    KeyHandle,
                    L"MappedDomain",
                    NULL,
                    &RegValueType,
                    (PUCHAR) RegDomainName,
                    &RegDomainSize
                    );

    if (RegStatus == ERROR_SUCCESS && RegDomainSize <= 0xFFFF) {

        NtLmLocklessGlobalMappedDomainString.Length = (USHORT)(RegDomainSize - sizeof(WCHAR));
        NtLmLocklessGlobalMappedDomainString.MaximumLength = (USHORT)RegDomainSize;
        NtLmLocklessGlobalMappedDomainString.Buffer = (PWSTR)NtLmAllocate( RegDomainSize );

        if( NtLmLocklessGlobalMappedDomainString.Buffer != NULL )
            CopyMemory( NtLmLocklessGlobalMappedDomainString.Buffer,
                        RegDomainName,
                        RegDomainSize );
    } else {
        RtlInitUnicodeString(
            &NtLmLocklessGlobalMappedDomainString,
            NULL
            );
    }


    //
    // Check the registry for a domain name to use
    //

    RegDomainSize = sizeof( RegDomainName );
    RegStatus = RegQueryValueExW(
                    KeyHandle,
                    L"PreferredDomain",
                    NULL,
                    &RegValueType,
                    (PUCHAR) RegDomainName,
                    &RegDomainSize
                    );

    if (RegStatus == ERROR_SUCCESS && RegDomainSize <= 0xFFFF) {

        NtLmLocklessGlobalPreferredDomainString.Length = (USHORT)(RegDomainSize - sizeof(WCHAR));
        NtLmLocklessGlobalPreferredDomainString.MaximumLength = (USHORT)RegDomainSize;
        NtLmLocklessGlobalPreferredDomainString.Buffer = (PWSTR)NtLmAllocate( RegDomainSize );

        if( NtLmLocklessGlobalPreferredDomainString.Buffer != NULL )
            CopyMemory( NtLmLocklessGlobalPreferredDomainString.Buffer,
                        RegDomainName,
                        RegDomainSize );
    } else {
        RtlInitUnicodeString(
            &NtLmLocklessGlobalPreferredDomainString,
            NULL
            );
    }


    return;
}


VOID
NtLmFreeMappedDomains(
    VOID
    )
{
    if( NtLmGlobalRegWaitObject )
        UnregisterWait( NtLmGlobalRegWaitObject );

    if( NtLmGlobalRegChangeNotifyEvent )
        CloseHandle( NtLmGlobalRegChangeNotifyEvent );

    if( NtLmLocklessGlobalMappedDomainString.Buffer ) {
        NtLmFree( NtLmLocklessGlobalMappedDomainString.Buffer );
        NtLmLocklessGlobalMappedDomainString.Buffer = NULL;
    }

    if( NtLmLocklessGlobalPreferredDomainString.Buffer ) {
        NtLmFree( NtLmLocklessGlobalPreferredDomainString.Buffer );
        NtLmLocklessGlobalPreferredDomainString.Buffer = NULL;
    }
}
