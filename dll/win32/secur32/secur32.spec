@ stdcall LsaRegisterLogonProcess(ptr ptr ptr)
@ stdcall LsaLogonUser(ptr ptr long long ptr long ptr ptr ptr ptr ptr ptr ptr ptr)
@ stdcall LsaDeregisterLogonProcess(long)
@ stdcall LsaConnectUntrusted(long)
@ stdcall LsaLookupAuthenticationPackage(ptr ptr ptr)
@ stdcall LsaFreeReturnBuffer(ptr)
@ stdcall LsaCallAuthenticationPackage(long long ptr long ptr ptr ptr)
@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr)
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr)
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr)
@ stdcall AddCredentialsA(ptr str str long ptr ptr ptr ptr)
@ stdcall AddCredentialsW(ptr wstr wstr long ptr ptr ptr ptr)
@ stub AddSecurityPackageA
@ stub AddSecurityPackageW
@ stdcall ApplyControlTokenA(ptr ptr)
@ stdcall ApplyControlToken(ptr ptr) ApplyControlTokenW
@ stdcall CompleteAuthToken(ptr ptr)
@ stub CredMarshalTargetInfo
@ stub CredUnmarshalTargetInfo
@ stdcall DecryptMessage(ptr ptr long ptr)
@ stdcall DeleteSecurityContext(ptr)
@ stub DeleteSecurityPackageA
@ stub DeleteSecurityPackageW
@ stdcall EncryptMessage(ptr long ptr long)
@ stdcall EnumerateSecurityPackagesA(ptr ptr)
@ stdcall EnumerateSecurityPackagesW(ptr ptr)
@ stdcall ExportSecurityContext(ptr long ptr ptr)
@ stdcall FreeContextBuffer(ptr)
@ stdcall FreeCredentialsHandle(ptr)
@ stdcall GetComputerObjectNameA(long ptr ptr)
@ stdcall GetComputerObjectNameW(long ptr ptr)
@ stub GetSecurityUserInfo
@ stdcall GetUserNameExA(long ptr ptr)
@ stdcall GetUserNameExW(long ptr ptr)
@ stdcall ImpersonateSecurityContext(ptr)
@ stdcall ImportSecurityContextA(str ptr ptr ptr)
@ stdcall ImportSecurityContextW(wstr ptr ptr ptr)
@ stdcall InitSecurityInterfaceA()
@ stdcall InitSecurityInterfaceW()
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr)
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr)
@ stdcall LsaEnumerateLogonSessions(ptr ptr)
@ stdcall LsaGetLogonSessionData(ptr ptr)
@ stdcall LsaRegisterPolicyChangeNotification(long ptr)
@ stdcall LsaUnregisterPolicyChangeNotification(long ptr)
@ stdcall MakeSignature(ptr long ptr long)
@ stdcall QueryContextAttributesA(ptr long ptr)
@ stdcall QueryContextAttributesW(ptr long ptr)
@ stdcall QueryCredentialsAttributesA(ptr long ptr)
@ stdcall QueryCredentialsAttributesW(ptr long ptr)
@ stdcall QuerySecurityContextToken(ptr ptr)
@ stdcall QuerySecurityPackageInfoA(str ptr)
@ stdcall QuerySecurityPackageInfoW(wstr ptr)
@ stdcall RevertSecurityContext(ptr)
@ stub SaslAcceptSecurityContext
@ stub SaslEnumerateProfilesA
@ stub SaslEnumerateProfilesW
@ stub SaslGetProfilePackageA
@ stub SaslGetProfilePackageW
@ stub SaslIdentifyPackageA
@ stub SaslIdentifyPackageW
@ stub SaslInitializeSecurityContextA
@ stub SaslInitializeSecurityContextW
@ stdcall SealMessage(ptr long ptr long) EncryptMessage
@ stub SecCacheSspiPackages
@ stub SecDeleteUserModeContext
@ stub SecGetLocaleSpecificEncryptionRules
@ stub SecInitUserModeContext
@ stub SecpFreeMemory
@ stub SecpTranslateName
@ stub SecpTranslateNameEx
@ stub SetContextAttributesA
@ stub SetContextAttributesW
@ stdcall TranslateNameA(str long long ptr ptr)
@ stdcall TranslateNameW(wstr long long ptr ptr)
@ stdcall UnsealMessage(ptr ptr long ptr) DecryptMessage
@ stdcall VerifySignature(ptr ptr long ptr)
