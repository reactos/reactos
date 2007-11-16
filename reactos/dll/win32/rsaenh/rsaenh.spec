@ stdcall CPAcquireContext(ptr str long ptr) RSAENH_CPAcquireContext
@ stdcall CPCreateHash(long long ptr long ptr) RSAENH_CPCreateHash
@ stdcall CPDecrypt(long long long long long ptr ptr) RSAENH_CPDecrypt
@ stdcall CPDeriveKey(long long long long ptr) RSAENH_CPDeriveKey
@ stdcall CPDestroyHash(long long) RSAENH_CPDestroyHash
@ stdcall CPDestroyKey(long long) RSAENH_CPDestroyKey
@ stdcall CPDuplicateHash(long long ptr long ptr) RSAENH_CPDuplicateHash
@ stdcall CPDuplicateKey(long long ptr long ptr) RSAENH_CPDuplicateKey
@ stdcall CPEncrypt(long long long long long ptr ptr long) RSAENH_CPEncrypt
@ stdcall CPExportKey(long long long long long ptr ptr) RSAENH_CPExportKey
@ stdcall CPGenKey(long long long ptr) RSAENH_CPGenKey
@ stdcall CPGenRandom(long long ptr) RSAENH_CPGenRandom
@ stdcall CPGetHashParam(long long long ptr ptr long) RSAENH_CPGetHashParam
@ stdcall CPGetKeyParam(long long long ptr ptr long) RSAENH_CPGetKeyParam
@ stdcall CPGetProvParam(long long ptr ptr long) RSAENH_CPGetProvParam
@ stdcall CPGetUserKey(long long ptr) RSAENH_CPGetUserKey
@ stdcall CPHashData(long long ptr long long) RSAENH_CPHashData
@ stdcall CPHashSessionKey(long long long long) RSAENH_CPHashSessionKey
@ stdcall CPImportKey(long ptr long long long ptr) RSAENH_CPImportKey
@ stdcall CPReleaseContext(long long) RSAENH_CPReleaseContext
@ stdcall CPSetHashParam(long long long ptr long) RSAENH_CPSetHashParam
@ stdcall CPSetKeyParam(long long long ptr long) RSAENH_CPSetKeyParam
@ stdcall CPSetProvParam(long long ptr long) RSAENH_CPSetProvParam
@ stdcall CPSignHash(long long long wstr long ptr ptr) RSAENH_CPSignHash
@ stdcall CPVerifySignature(long long ptr long long wstr long) RSAENH_CPVerifySignature
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
