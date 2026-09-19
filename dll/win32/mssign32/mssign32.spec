@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub FreeCryptProvFromCert
@ stub GetCryptProvFromCert
@ stdcall PvkFreeCryptProv(ptr wstr long wstr)
@ stdcall PvkGetCryptProv(ptr wstr wstr long wstr wstr ptr ptr ptr)
@ stub PvkPrivateKeyAcquireContext
@ stub PvkPrivateKeyAcquireContextA
@ stdcall PvkPrivateKeyAcquireContextFromMemory(wstr long ptr long ptr wstr ptr ptr ptr)
@ stub PvkPrivateKeyAcquireContextFromMemoryA
@ stub PvkPrivateKeyLoad
@ stub PvkPrivateKeyLoadA
@ stub PvkPrivateKeyLoadFromMemory
@ stub PvkPrivateKeyLoadFromMemoryA
@ stub PvkPrivateKeyReleaseContext
@ stub PvkPrivateKeyReleaseContextA
@ stub PvkPrivateKeySave
@ stub PvkPrivateKeySaveA
@ stub PvkPrivateKeySaveToMemory
@ stub PvkPrivateKeySaveToMemoryA
@ stub SignError
@ stub SignerAddTimeStampResponse
@ stub SignerAddTimeStampResponseEx
@ stub SignerCreateTimeStampRequest
@ stdcall SignerFreeSignerContext(ptr)
@ stdcall SignerSign(ptr ptr ptr ptr wstr ptr ptr)
@ stdcall SignerSignEx(long ptr ptr ptr ptr wstr ptr ptr ptr)
@ stub SignerTimeStamp
@ stub SignerTimeStampEx
@ stub SpcGetCertFromKey
