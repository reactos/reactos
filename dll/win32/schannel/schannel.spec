@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) schan_AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) schan_AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) schan_AcquireCredentialsHandleW
@ stdcall ApplyControlToken(ptr ptr) schan_ApplyControlToken
@ stub CloseSslPerformanceData
@ stub CollectSslPerformanceData
@ stdcall CompleteAuthToken(ptr ptr) schan_CompleteAuthToken
@ stdcall DeleteSecurityContext(ptr) schan_DeleteSecurityContext
@ stdcall EnumerateSecurityPackagesA(ptr ptr) schan_EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) schan_EnumerateSecurityPackagesW
@ stdcall FreeContextBuffer(ptr) schan_FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) schan_FreeCredentialsHandle
@ stdcall ImpersonateSecurityContext(ptr) schan_ImpersonateSecurityContext
@ stdcall InitSecurityInterfaceA() schan_InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() schan_InitSecurityInterfaceW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) schan_InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) schan_InitializeSecurityContextW
@ stdcall MakeSignature(ptr long ptr long) secur32.MakeSignature
@ stub OpenSslPerformanceData
@ stdcall QueryContextAttributesA(ptr long ptr) secur32.QueryContextAttributesA
@ stdcall QueryContextAttributesW(ptr long ptr) secur32.QueryContextAttributesW
@ stdcall QuerySecurityPackageInfoA(str ptr) secur32.QuerySecurityPackageInfoA
@ stdcall QuerySecurityPackageInfoW(wstr ptr) secur32.QuerySecurityPackageInfoW
@ stdcall RevertSecurityContext(ptr) secur32.RevertSecurityContext
@ stdcall SealMessage(ptr long ptr long) secur32.SealMessage
@ stdcall SpLsaModeInitialize(long ptr ptr ptr)
@ stdcall SpUserModeInitialize(long ptr ptr ptr)
@ stub SslCrackCertificate
@ stdcall SslEmptyCacheA(str long)
@ stdcall SslEmptyCacheW(wstr long)
@ stub SslFreeCertificate
@ stub SslGenerateKeyPair
@ stub SslGenerateRandomBits
@ stub SslGetMaximumKeySize
@ stub SslLoadCertificate
@ stdcall UnsealMessage(ptr ptr long ptr) secur32.UnsealMessage
@ stdcall VerifySignature(ptr ptr long ptr) secur32.VerifySignature
