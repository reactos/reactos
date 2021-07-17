@ stdcall BCryptAddContextFunction(long wstr long wstr long)
@ stdcall BCryptAddContextFunctionProvider(long wstr long wstr wstr long)
@ stdcall BCryptCloseAlgorithmProvider(ptr long)
@ stub BCryptConfigureContext
@ stub BCryptConfigureContextFunction
@ stub BCryptCreateContext
@ stdcall BCryptCreateHash(ptr ptr ptr long ptr long long)
@ stdcall BCryptDecrypt(ptr ptr long ptr ptr long ptr long ptr long)
@ stub BCryptDeleteContext
@ stdcall BCryptDeriveKey(ptr wstr ptr ptr long ptr long)
@ stdcall BCryptDeriveKeyCapi(ptr ptr ptr long long)
@ stdcall BCryptDeriveKeyPBKDF2(ptr ptr long ptr long int64 ptr long long)
@ stdcall BCryptDestroyHash(ptr)
@ stdcall BCryptDestroyKey(ptr)
@ stdcall BCryptDestroySecret(ptr)
@ stdcall BCryptDuplicateHash(ptr ptr ptr long long)
@ stdcall BCryptDuplicateKey(ptr ptr ptr long long)
@ stdcall BCryptEncrypt(ptr ptr long ptr ptr long ptr long ptr long)
@ stdcall BCryptEnumAlgorithms(long ptr ptr long)
@ stub BCryptEnumContextFunctionProviders
@ stdcall BCryptEnumContextFunctions(long wstr long ptr ptr)
@ stub BCryptEnumContexts
@ stub BCryptEnumProviders
@ stub BCryptEnumRegisteredProviders
@ stdcall BCryptExportKey(ptr ptr wstr ptr long ptr long)
@ stdcall BCryptFinalizeKeyPair(ptr long)
@ stdcall BCryptFinishHash(ptr ptr long long)
@ stdcall BCryptFreeBuffer(ptr)
@ stdcall BCryptGenRandom(ptr ptr long long)
@ stdcall BCryptGenerateKeyPair(ptr ptr long long)
@ stdcall BCryptGenerateSymmetricKey(ptr ptr ptr long ptr long long)
@ stdcall BCryptGetFipsAlgorithmMode(ptr)
@ stdcall BCryptGetProperty(ptr wstr ptr long ptr long)
@ stdcall BCryptHash(ptr ptr long ptr long ptr long)
@ stdcall BCryptHashData(ptr ptr long long)
@ stdcall BCryptImportKey(ptr ptr wstr ptr ptr long ptr long long)
@ stdcall BCryptImportKeyPair(ptr ptr wstr ptr ptr long long)
@ stdcall BCryptOpenAlgorithmProvider(ptr wstr wstr long)
@ stub BCryptQueryContextConfiguration
@ stub BCryptQueryContextFunctionConfiguration
@ stub BCryptQueryContextFunctionProperty
@ stub BCryptQueryProviderRegistration
@ stub BCryptRegisterConfigChangeNotify
@ stdcall BCryptRegisterProvider(wstr long ptr)
@ stdcall BCryptRemoveContextFunction(long wstr long wstr)
@ stdcall BCryptRemoveContextFunctionProvider(long wstr long wstr wstr)
@ stub BCryptResolveProviders
@ stdcall BCryptSecretAgreement(ptr ptr ptr long)
@ stub BCryptSetAuditingInterface
@ stub BCryptSetContextFunctionProperty
@ stdcall BCryptSetProperty(ptr wstr ptr long long)
@ stdcall BCryptSignHash(ptr ptr ptr long ptr long ptr long)
@ stub BCryptUnregisterConfigChangeNotify
@ stdcall BCryptUnregisterProvider(wstr)
@ stdcall BCryptVerifySignature(ptr ptr ptr long ptr long long)
@ stub GetAsymmetricEncryptionInterface
@ stub GetCipherInterface
@ stub GetHashInterface
@ stub GetRngInterface
@ stub GetSecretAgreementInterface
@ stub GetSignatureInterface
