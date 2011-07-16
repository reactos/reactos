@ stdcall AcceptSecurityContext(ptr ptr ptr long long ptr ptr ptr ptr) SECUR32.AcceptSecurityContext
@ stdcall AcquireCredentialsHandleA(str str long ptr ptr ptr ptr ptr ptr) SECUR32.AcquireCredentialsHandleA
@ stdcall AcquireCredentialsHandleW(wstr wstr long ptr ptr ptr ptr ptr ptr) SECUR32.AcquireCredentialsHandleW
@ stdcall AddSecurityPackageA(str ptr) SECUR32.AddSecurityPackageA
@ stdcall AddSecurityPackageW(wstr ptr) SECUR32.AddSecurityPackageW
@ stdcall ApplyControlToken(ptr ptr) SECUR32.ApplyControlToken
@ stdcall CompleteAuthToken(ptr ptr) SECUR32.CompleteAuthToken
@ stdcall DecryptMessage(ptr ptr long ptr) SECUR32.DecryptMessage
@ stdcall DeleteSecurityContext(ptr) SECUR32.DeleteSecurityContext
@ stdcall DeleteSecurityPackageA(str) SECUR32.DeleteSecurityPackageA
@ stdcall DeleteSecurityPackageW(wstr) SECUR32.DeleteSecurityPackageW
@ stdcall EncryptMessage(ptr long ptr long) SECUR32.EncryptMessage
@ stdcall EnumerateSecurityPackagesA(ptr ptr) SECUR32.EnumerateSecurityPackagesA
@ stdcall EnumerateSecurityPackagesW(ptr ptr) SECUR32.EnumerateSecurityPackagesW
@ stdcall ExportSecurityContext(ptr long ptr ptr) SECUR32.ExportSecurityContext
@ stdcall FreeContextBuffer(ptr) SECUR32.FreeContextBuffer
@ stdcall FreeCredentialsHandle(ptr) SECUR32.FreeCredentialsHandle
@ stdcall ImpersonateSecurityContext(ptr) SECUR32.ImpersonateSecurityContext
@ stdcall ImportSecurityContextA(str ptr ptr ptr) SECUR32.ImportSecurityContextA
@ stdcall ImportSecurityContextW(wstr ptr ptr ptr) SECUR32.ImportSecurityContextW
@ stdcall InitializeSecurityContextA(ptr ptr str long long long ptr long ptr ptr ptr ptr) SECUR32.InitializeSecurityContextA
@ stdcall InitializeSecurityContextW(ptr ptr wstr long long long ptr long ptr ptr ptr ptr) SECUR32.InitializeSecurityContextW
@ stdcall InitSecurityInterfaceA() SECUR32.InitSecurityInterfaceA
@ stdcall InitSecurityInterfaceW() SECUR32.InitSecurityInterfaceW
@ stdcall MakeSignature(ptr long ptr long) SECUR32.MakeSignature
@ stdcall QueryContextAttributesA(ptr long ptr) SECUR32.QueryContextAttributesA
@ stdcall QueryContextAttributesW(ptr long ptr) SECUR32.QueryContextAttributesW
@ stdcall QueryCredentialsAttributesA(ptr long ptr) SECUR32.QueryCredentialsAttributesA
@ stdcall QueryCredentialsAttributesW(ptr long ptr) SECUR32.QueryCredentialsAttributesW
@ stdcall QuerySecurityContextToken(ptr ptr) SECUR32.QuerySecurityContextToken
@ stdcall QuerySecurityPackageInfoA(str ptr) SECUR32.QuerySecurityPackageInfoA
@ stdcall QuerySecurityPackageInfoW(wstr ptr) SECUR32.QuerySecurityPackageInfoW
@ stdcall RevertSecurityContext(ptr) SECUR32.RevertSecurityContext
@ stdcall SealMessage(ptr long ptr long) SECUR32.EncryptMessage
@ stdcall UnsealMessage(ptr ptr long ptr) SECUR32.DecryptMessage
@ stdcall VerifySignature(ptr ptr long ptr) SECUR32.VerifySignature

