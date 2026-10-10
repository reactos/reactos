@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) secur32.AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleW
@ stdcall AddCredentialsA(ptr str str long ptr ptr ptr ptr) secur32.AddCredentialsA
@ stdcall AddCredentialsW(ptr wstr wstr long ptr ptr ptr ptr) secur32.AddCredentialsW
@ stdcall AddSecurityPackageA(str ptr) secur32.AddSecurityPackageA
@ stdcall AddSecurityPackageW(wstr ptr) secur32.AddSecurityPackageW
@ stdcall ApplyControlToken(ptr ptr) secur32.ApplyControlToken
@ stub ChangeAccountPasswordA
@ stub ChangeAccountPasswordW
@ stdcall CompleteAuthToken(ptr ptr) secur32.CompleteAuthToken
@ stub CredMarshalTargetInfo
@ stub CredUnmarshalTargetInfo
@ stdcall DecryptMessage(ptr ptr long ptr) secur32.DecryptMessage
@ stdcall DeleteSecurityContext(ptr) secur32.DeleteSecurityContext
@ stdcall DeleteSecurityPackageA(str) secur32.DeleteSecurityPackageA
@ stdcall DeleteSecurityPackageW(wstr) secur32.DeleteSecurityPackageW
@ stdcall EncryptMessage(ptr long ptr long) secur32.EncryptMessage
@ stdcall EnumerateSecurityPackagesA(ptr ptr) secur32.EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) secur32.EnumerateSecurityPackagesW
@ stdcall ExportSecurityContext(ptr long ptr ptr) secur32.ExportSecurityContext
@ stdcall FreeContextBuffer(ptr) secur32.FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) secur32.FreeCredentialsHandle
@ stub GetSecurityUserInfo
@ stdcall GetUserNameExA(long ptr ptr) secur32.GetUserNameExA
@ stdcall GetUserNameExW(long ptr ptr) secur32.GetUserNameExW
@ stdcall ImpersonateSecurityContext(ptr) secur32.ImpersonateSecurityContext
@ stdcall ImportSecurityContextA(str ptr ptr ptr) secur32.ImportSecurityContextA
@ stdcall ImportSecurityContextW(wstr ptr ptr ptr) secur32.ImportSecurityContextW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextW
@ stdcall InitSecurityInterfaceA() secur32.InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() secur32.InitSecurityInterfaceW
@ stub LogonUserExExW
@ stdcall LsaCallAuthenticationPackage(long long ptr long ptr ptr ptr) secur32.LsaCallAuthenticationPackage
@ stdcall LsaConnectUntrusted(ptr) secur32.LsaConnectUntrusted
@ stdcall LsaDeregisterLogonProcess(long) secur32.LsaDeregisterLogonProcess
@ stdcall LsaEnumerateLogonSessions(ptr ptr) secur32.LsaEnumerateLogonSessions
@ stdcall LsaFreeReturnBuffer(ptr) secur32.LsaFreeReturnBuffer
@ stdcall LsaGetLogonSessionData(ptr ptr) secur32.LsaGetLogonSessionData
@ stdcall LsaLogonUser(ptr ptr long long ptr long ptr ptr ptr ptr ptr ptr ptr ptr) secur32.LsaLogonUser
@ stdcall LsaLookupAuthenticationPackage(ptr ptr ptr) secur32.LsaLookupAuthenticationPackage
@ stdcall LsaRegisterLogonProcess(ptr ptr ptr) secur32.LsaRegisterLogonProcess
@ stub LsaRegisterPolicyChangeNotification
@ stub LsaUnregisterPolicyChangeNotification
@ stdcall MakeSignature(ptr long ptr long) secur32.MakeSignature
@ stdcall QueryContextAttributesA(ptr long ptr) secur32.QueryContextAttributesA
@ stub QueryContextAttributesExA
@ stub QueryContextAttributesExW
@ stdcall QueryContextAttributesW(ptr long ptr) secur32.QueryContextAttributesW
@ stdcall QueryCredentialsAttributesA(ptr long ptr) secur32.QueryCredentialsAttributesA
@ stub QueryCredentialsAttributesExA
@ stub QueryCredentialsAttributesExW
@ stdcall QueryCredentialsAttributesW(ptr long ptr) secur32.QueryCredentialsAttributesW
@ stdcall QuerySecurityContextToken(ptr ptr) secur32.QuerySecurityContextToken
@ stdcall QuerySecurityPackageInfoA(str ptr) secur32.QuerySecurityPackageInfoA
@ stdcall QuerySecurityPackageInfoW(wstr ptr) secur32.QuerySecurityPackageInfoW
@ stdcall RevertSecurityContext(ptr) secur32.RevertSecurityContext
@ stub SaslAcceptSecurityContext
@ stub SaslEnumerateProfilesA
@ stub SaslEnumerateProfilesW
@ stub SaslGetContextOption
@ stub SaslGetProfilePackageA
@ stub SaslGetProfilePackageW
@ stub SaslIdentifyPackageA
@ stub SaslIdentifyPackageW
@ stub SaslInitializeSecurityContextA
@ stub SaslInitializeSecurityContextW
@ stub SaslSetContextOption
@ stdcall SealMessage(ptr long ptr long) secur32.SealMessage
@ stub SecCacheSspiPackages
@ stub SecDeleteUserModeContext
@ stub SeciAllocateAndSetCallFlags
@ stub SeciAllocateAndSetIPAddress
@ stub SeciFreeCallContext
@ stub SeciIsProtectedUser
@ stub SecInitUserModeContext
@ stdcall SetContextAttributesA(ptr long ptr long) secur32.SetContextAttributesA
@ stdcall SetContextAttributesW(ptr long ptr long) secur32.SetContextAttributesW
@ stub SetCredentialsAttributesA
@ stub SetCredentialsAttributesW
@ stub SspiCompareAuthIdentities
@ stub SspiCopyAuthIdentity
@ stub SspiDecryptAuthIdentity
@ stub SspiDecryptAuthIdentityEx
@ stdcall SspiEncodeAuthIdentityAsStrings(ptr ptr ptr ptr)
@ stdcall SspiEncodeStringsAsAuthIdentity(wstr wstr wstr ptr)
@ stub SspiEncryptAuthIdentity
@ stub SspiEncryptAuthIdentityEx
@ stub SspiExcludePackage
@ stdcall SspiFreeAuthIdentity(ptr)
@ stub SspiGetComputerNameForSPN
@ stub SspiGetTargetHostName
@ stub SspiIsAuthIdentityEncrypted
@ stdcall SspiLocalFree(ptr)
@ stub SspiMarshalAuthIdentity
@ stub SspiPrepareForCredRead
@ stdcall SspiPrepareForCredWrite(ptr wstr ptr ptr ptr ptr ptr)
@ stub SspiUnmarshalAuthIdentity
@ stub SspiUnmarshalAuthIdentityInternal
@ stub SspiValidateAuthIdentity
@ stdcall SspiZeroAuthIdentity(ptr)
@ stdcall UnsealMessage(ptr ptr long ptr) secur32.UnsealMessage
@ stdcall VerifySignature(ptr ptr long ptr) secur32.VerifySignature
