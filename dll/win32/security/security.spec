@ stdcall LsaRegisterLogonProcess(ptr ptr ptr) secur32.LsaRegisterLogonProcess
@ stdcall LsaLogonUser(ptr ptr long long ptr long ptr ptr ptr ptr ptr ptr ptr ptr) secur32.LsaLogonUser
@ stdcall LsaDeregisterLogonProcess(long) secur32.LsaDeregisterLogonProcess
@ stdcall LsaConnectUntrusted(long) secur32.LsaConnectUntrusted
@ stdcall LsaLookupAuthenticationPackage(ptr ptr ptr) SECUR32.LsaLookupAuthenticationPackage
@ stdcall LsaFreeReturnBuffer(ptr) SECUR32.LsaFreeReturnBuffer
@ stdcall LsaCallAuthenticationPackage(long long ptr long ptr ptr ptr) SECUR32.LsaCallAuthenticationPackage
@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) SECUR32.AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) SECUR32.AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) SECUR32.AcquireCredentialsHandleW
@ stdcall AddCredentialsA(ptr str str long ptr ptr ptr ptr) SECUR32.AddCredentialsA
@ stdcall AddCredentialsW(ptr wstr wstr long ptr ptr ptr ptr) SECUR32.AddCredentialsW
@ stub AddSecurityPackageA # SECUR32.AddSecurityPackageA
@ stub AddSecurityPackageW # SECUR32.AddSecurityPackageW
@ stdcall ApplyControlTokenA(ptr ptr) SECUR32.ApplyControlTokenA
@ stdcall ApplyControlTokenW(ptr ptr) SECUR32.ApplyControlTokenW
@ stdcall CompleteAuthToken(ptr ptr) SECUR32.CompleteAuthToken
@ stub CredMarshalTargetInfo # SECUR32.CredMarshalTargetInfo
@ stub CredUnmarshalTargetInfo # SECUR32.CredUnmarshalTargetInfo
@ stdcall DecryptMessage(ptr ptr long ptr) SECUR32.DecryptMessage
@ stdcall DeleteSecurityContext(ptr) SECUR32.DeleteSecurityContext
@ stub DeleteSecurityPackageA # SECUR32.DeleteSecurityPackageA
@ stub DeleteSecurityPackageW # SECUR32.DeleteSecurityPackageW
@ stdcall EncryptMessage(ptr long ptr long) SECUR32.EncryptMessage
@ stdcall EnumerateSecurityPackagesA(ptr ptr) SECUR32.EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) SECUR32.EnumerateSecurityPackagesW
@ stdcall ExportSecurityContext(ptr long ptr ptr) SECUR32.ExportSecurityContext
@ stdcall FreeContextBuffer(ptr) SECUR32.FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) SECUR32.FreeCredentialsHandle
@ stdcall GetComputerObjectNameA(long ptr ptr) SECUR32.GetComputerObjectNameA
@ stdcall GetComputerObjectNameW(long ptr ptr) SECUR32.GetComputerObjectNameW
@ stub GetSecurityUserInfo # SECUR32.GetSecurityUserInfo
@ stdcall GetUserNameExA(long ptr ptr) SECUR32.GetUserNameExA
@ stdcall GetUserNameExW(long ptr ptr) SECUR32.GetUserNameExW
@ stdcall ImpersonateSecurityContext(ptr) SECUR32.ImpersonateSecurityContext
@ stdcall ImportSecurityContextA(str ptr ptr ptr) SECUR32.ImportSecurityContextA
@ stdcall ImportSecurityContextW(wstr ptr ptr ptr) SECUR32.ImportSecurityContextW
@ stdcall InitSecurityInterfaceA() SECUR32.InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() SECUR32.InitSecurityInterfaceW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) SECUR32.InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) SECUR32.InitializeSecurityContextW
@ stdcall LsaEnumerateLogonSessions(ptr ptr) SECUR32.LsaEnumerateLogonSessions
@ stdcall LsaGetLogonSessionData(ptr ptr) SECUR32.LsaGetLogonSessionData
@ stdcall LsaRegisterPolicyChangeNotification(long ptr) SECUR32.LsaRegisterPolicyChangeNotification
@ stdcall LsaUnregisterPolicyChangeNotification(long ptr) SECUR32.LsaUnregisterPolicyChangeNotification
@ stdcall MakeSignature(ptr long ptr long) SECUR32.MakeSignature
@ stdcall QueryContextAttributesA(ptr long ptr) SECUR32.QueryContextAttributesA
@ stdcall QueryContextAttributesW(ptr long ptr) SECUR32.QueryContextAttributesW
@ stdcall QueryCredentialsAttributesA(ptr long ptr) SECUR32.QueryCredentialsAttributesA
@ stdcall QueryCredentialsAttributesW(ptr long ptr) SECUR32.QueryCredentialsAttributesW
@ stdcall QuerySecurityContextToken(ptr ptr) SECUR32.QuerySecurityContextToken
@ stdcall QuerySecurityPackageInfoA(str ptr) SECUR32.QuerySecurityPackageInfoA
@ stdcall QuerySecurityPackageInfoW(wstr ptr) SECUR32.QuerySecurityPackageInfoW
@ stdcall RevertSecurityContext(ptr) SECUR32.RevertSecurityContext
@ stub SaslAcceptSecurityContext # SECUR32.SaslAcceptSecurityContext
@ stub SaslEnumerateProfilesA # SECUR32.SaslEnumerateProfilesA
@ stub SaslEnumerateProfilesW # SECUR32.SaslEnumerateProfilesW
@ stub SaslGetProfilePackageA # SECUR32.SaslGetProfilePackageA
@ stub SaslGetProfilePackageW # SECUR32.SaslGetProfilePackageW
@ stub SaslIdentifyPackageA # SECUR32.SaslIdentifyPackageA
@ stub SaslIdentifyPackageW # SECUR32.SaslIdentifyPackageW
@ stub SaslInitializeSecurityContextA # SECUR32.SaslInitializeSecurityContextA
@ stub SaslInitializeSecurityContextW # SECUR32.SaslInitializeSecurityContextW
@ stdcall SealMessage(ptr long ptr long) SECUR32.EncryptMessage
@ stub SecCacheSspiPackages # SECUR32.SecCacheSspiPackages
@ stub SecDeleteUserModeContext # SECUR32.SecDeleteUserModeContext
@ stub SecGetLocaleSpecificEncryptionRules # SECUR32.SecGetLocaleSpecificEncryptionRules
@ stub SecInitUserModeContext # SECUR32.SecInitUserModeContext
@ stub SecpFreeMemory # SECUR32.SecpFreeMemory
@ stub SecpTranslateName # SECUR32.SecpTranslateName
@ stub SecpTranslateNameEx # SECUR32.SecpTranslateNameEx
@ stub SetContextAttributesA # SECUR32.SetContextAttributesA
@ stub SetContextAttributesW #SECUR32.SetContextAttributesW
@ stdcall TranslateNameA(str long long ptr ptr) SECUR32.TranslateNameA
@ stdcall TranslateNameW(wstr long long ptr ptr) SECUR32.TranslateNameW
@ stdcall UnsealMessage(ptr ptr long ptr) SECUR32.DecryptMessage
@ stdcall VerifySignature(ptr ptr long ptr) SECUR32.VerifySignature
