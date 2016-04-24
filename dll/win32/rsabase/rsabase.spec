@ stdcall CPAcquireContext(ptr str long ptr) rsaenh.CPAcquireContext
@ stdcall CPCreateHash(long long ptr long ptr) rsaenh.CPCreateHash
@ stdcall CPDecrypt(long long long long long ptr ptr) rsaenh.CPDecrypt
@ stdcall CPDeriveKey(long long long long ptr) rsaenh.CPDeriveKey
@ stdcall CPDestroyHash(long long) rsaenh.CPDestroyHash
@ stdcall CPDestroyKey(long long) rsaenh.CPDestroyKey
@ stdcall CPDuplicateHash(long long ptr long ptr) rsaenh.CPDuplicateHash
@ stdcall CPDuplicateKey(long long ptr long ptr) rsaenh.CPDuplicateKey
@ stdcall CPEncrypt(long long long long long ptr ptr long) rsaenh.CPEncrypt
@ stdcall CPExportKey(long long long long long ptr ptr) rsaenh.CPExportKey
@ stdcall CPGenKey(long long long ptr) rsaenh.CPGenKey
@ stdcall CPGenRandom(long long ptr) rsaenh.CPGenRandom
@ stdcall CPGetHashParam(long long long ptr ptr long) rsaenh.CPGetHashParam
@ stdcall CPGetKeyParam(long long long ptr ptr long) rsaenh.CPGetKeyParam
@ stdcall CPGetProvParam(long long ptr ptr long) rsaenh.CPGetProvParam
@ stdcall CPGetUserKey(long long ptr) rsaenh.CPGetUserKey
@ stdcall CPHashData(long long ptr long long) rsaenh.CPHashData
@ stdcall CPHashSessionKey(long long long long) rsaenh.CPHashSessionKey
@ stdcall CPImportKey(long ptr long long long ptr) rsaenh.CPImportKey
@ stdcall CPReleaseContext(long long) rsaenh.CPReleaseContext
@ stdcall CPSetHashParam(long long long ptr long) rsaenh.CPSetHashParam
@ stdcall CPSetKeyParam(long long long ptr long) rsaenh.CPSetKeyParam
@ stdcall CPSetProvParam(long long ptr long) rsaenh.CPSetProvParam
@ stdcall CPSignHash(long long long wstr long ptr ptr) rsaenh.CPSignHash
@ stdcall CPVerifySignature(long long ptr long long wstr long) rsaenh.CPVerifySignature
@ stdcall -private DllRegisterServer() rsaenh.DllRegisterServer
@ stdcall -private DllUnregisterServer() rsaenh.DllUnregisterServer
