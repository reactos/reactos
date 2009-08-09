@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) secur32.AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) secur32.AcquireCredentialsHandleW
@ stdcall ApplyControlToken(ptr ptr) secur32.ApplyControlToken
@ stub CloseSslPerformanceData
@ stub CollectSslPerformanceData
@ stdcall CompleteAuthToken(ptr ptr) secur32.CompleteAuthToken
@ stdcall DeleteSecurityContext(ptr) secur32.DeleteSecurityContext
@ stdcall EnumerateSecurityPackagesA(ptr ptr) secur32.EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) secur32.EnumerateSecurityPackagesW
@ stdcall FreeContextBuffer(ptr) secur32.FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) secur32.FreeCredentialsHandle
@ stdcall ImpersonateSecurityContext(ptr) secur32.ImpersonateSecurityContext
@ stdcall InitSecurityInterfaceA() secur32.InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() secur32.InitSecurityInterfaceW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) secur32.InitializeSecurityContextW
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
