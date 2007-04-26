@ stdcall CertAddCRLContextToStore(long ptr long ptr)
@ stdcall CertAddCTLContextToStore(long ptr long ptr)
@ stdcall CertAddCertificateContextToStore(long ptr long ptr)
@ stdcall CertAddEncodedCRLToStore(long long ptr long long ptr)
@ stdcall CertAddEncodedCTLToStore(long long ptr long long ptr)
@ stdcall CertAddEncodedCertificateToStore(long long ptr long long ptr)
@ stub CertAddEncodedCertificateToSystemStoreA
@ stub CertAddEncodedCertificateToSystemStoreW
@ stdcall CertAddEnhancedKeyUsageIdentifier(ptr str)
@ stdcall CertAddSerializedElementToStore(ptr ptr long long long long ptr ptr)
@ stdcall CertAddStoreToCollection(ptr ptr long long)
@ stdcall CertAlgIdToOID(long)
@ stdcall CertCloseStore(ptr long)
@ stdcall CertCompareCertificate(long ptr ptr)
@ stdcall CertCompareCertificateName(long ptr ptr)
@ stdcall CertCompareIntegerBlob(ptr ptr)
@ stdcall CertComparePublicKeyInfo(long ptr ptr)
@ stdcall CertControlStore(long long long ptr)
@ stdcall CertCreateCRLContext(long ptr long)
@ stdcall CertCreateCTLContext(long ptr long)
@ stdcall CertCreateCertificateChainEngine(ptr ptr)
@ stdcall CertCreateCertificateContext(long ptr long)
@ stdcall CertCreateSelfSignCertificate(long ptr long ptr ptr ptr ptr ptr)
@ stdcall CertDeleteCRLFromStore(ptr)
@ stdcall CertDeleteCTLFromStore(ptr)
@ stdcall CertDeleteCertificateFromStore(ptr)
@ stdcall CertDuplicateCRLContext(ptr)
@ stdcall CertDuplicateCTLContext(ptr)
@ stdcall CertDuplicateCertificateContext(ptr)
@ stdcall CertDuplicateStore(ptr)
@ stdcall CertEnumCRLContextProperties(ptr long)
@ stdcall CertEnumCRLsInStore(ptr ptr)
@ stdcall CertEnumCTLContextProperties(ptr long)
@ stdcall CertEnumCTLsInStore(ptr ptr)
@ stdcall CertEnumCertificateContextProperties(ptr long)
@ stdcall CertEnumCertificatesInStore(long ptr)
@ stdcall CertFindAttribute(str long ptr)
@ stdcall CertFindCRLInStore(long long long long ptr ptr)
@ stub CertFindCTLInStore
@ stdcall CertFindCertificateInStore(long long long long ptr ptr)
@ stdcall CertFindCertificateInCRL(ptr ptr long ptr ptr)
@ stdcall CertFindExtension(str long ptr)
@ stdcall CertFindRDNAttr(str ptr)
@ stub CertFindSubjectInCTL
@ stdcall CertFreeCRLContext(ptr)
@ stdcall CertFreeCTLContext(ptr)
@ stub CertFreeCertificateChain
@ stdcall CertFreeCertificateChainEngine(ptr)
@ stdcall CertFreeCertificateContext(ptr)
@ stdcall CertGetCRLContextProperty(ptr long ptr ptr)
@ stdcall CertGetCRLFromStore(ptr ptr ptr ptr)
@ stdcall CertGetCTLContextProperty(ptr long ptr ptr)
@ stub CertGetCertificateChain
@ stdcall CertGetCertificateContextProperty(ptr long ptr ptr)
@ stdcall CertGetEnhancedKeyUsage(ptr long ptr ptr)
@ stub CertGetIntendedKeyUsage
@ stdcall CertGetIssuerCertificateFromStore(long ptr ptr ptr)
@ stdcall CertGetNameStringA(ptr long long ptr ptr long)
@ stdcall CertGetNameStringW(ptr long long ptr ptr long)
@ stdcall CertGetPublicKeyLength(long ptr)
@ stdcall CertGetSubjectCertificateFromStore(ptr long ptr)
@ stdcall CertGetValidUsages(long ptr ptr ptr ptr)
@ stub CertIsRDNAttrsInCertificateName
@ stdcall CertIsValidCRLForCertificate(ptr ptr long ptr)
@ stdcall CertNameToStrA(long ptr long ptr long)
@ stdcall CertNameToStrW(long ptr long ptr long)
@ stdcall CertOIDToAlgId(str)
@ stdcall CertOpenStore(str long long long ptr)
@ stdcall CertOpenSystemStoreA(long str)
@ stdcall CertOpenSystemStoreW(long wstr)
@ stdcall CertRDNValueToStrA(long ptr ptr long)
@ stdcall CertRDNValueToStrW(long ptr ptr long)
@ stdcall CertRemoveEnhancedKeyUsageIdentifier(ptr str)
@ stdcall CertRemoveStoreFromCollection(long long)
@ stdcall CertSaveStore(long long long long ptr long)
@ stdcall CertSerializeCRLStoreElement(ptr long ptr ptr)
@ stdcall CertSerializeCTLStoreElement(ptr long ptr ptr)
@ stdcall CertSerializeCertificateStoreElement(ptr long ptr ptr)
@ stdcall CertSetCRLContextProperty(ptr long long ptr)
@ stdcall CertSetCTLContextProperty(ptr long long ptr)
@ stdcall CertSetCertificateContextProperty(ptr long long ptr)
@ stdcall CertSetEnhancedKeyUsage(ptr ptr)
@ stdcall CertStrToNameA(long str long ptr ptr ptr ptr)
@ stdcall CertStrToNameW(long wstr long ptr ptr ptr ptr)
@ stdcall CertVerifyCRLRevocation(long ptr long ptr)
@ stdcall CertVerifyCRLTimeValidity(ptr ptr)
@ stub CertVerifyCTLUsage
@ stub CertVerifyRevocation
@ stdcall CertVerifySubjectCertificateContext(ptr ptr ptr)
@ stdcall CertVerifyTimeValidity(ptr ptr)
@ stdcall CertVerifyValidityNesting(ptr ptr)
@ stdcall CreateFileU(wstr long long ptr long long ptr) kernel32.CreateFileW
@ stdcall CryptBinaryToStringA(ptr long long ptr ptr)
@ stub CryptBinaryToStringW # (ptr long long ptr ptr)
@ stdcall CryptStringToBinaryA(str long long ptr ptr ptr ptr)
@ stub CryptStringToBinaryW # (wstr long long ptr ptr ptr ptr)
@ stdcall CryptAcquireContextU(ptr wstr wstr long long) advapi32.CryptAcquireContextW
@ stdcall CryptAcquireCertificatePrivateKey(ptr long ptr ptr ptr ptr)
@ stub CryptCloseAsyncHandle
@ stub CryptCreateAsyncHandle
@ stub CryptDecodeMessage
@ stdcall CryptDecodeObject(long str ptr long long ptr ptr)
@ stdcall CryptDecodeObjectEx(long str ptr long long ptr ptr ptr)
@ stub CryptDecryptAndVerifyMessageSignature
@ stub CryptDecryptMessage
@ stdcall CryptEncodeObject(long str ptr ptr ptr)
@ stdcall CryptEncodeObjectEx(long str ptr long ptr ptr ptr)
@ stub CryptEncryptMessage
@ stub CryptEnumOIDFunction
@ stdcall CryptEnumOIDInfo(long long ptr ptr)
@ stub CryptEnumProvidersU
@ stub CryptExportPKCS8
@ stdcall CryptExportPublicKeyInfo(long long long ptr ptr)
@ stdcall CryptExportPublicKeyInfoEx(long long long str long ptr ptr ptr)
@ stdcall CryptFindOIDInfo(long ptr long)
@ stdcall CryptFormatObject(long long long ptr str ptr long ptr ptr)
@ stdcall CryptFreeOIDFunctionAddress(long long)
@ stub CryptGetAsyncParam
@ stdcall CryptGetDefaultOIDDllList(long long ptr ptr)
@ stdcall CryptGetDefaultOIDFunctionAddress(long long wstr long ptr ptr)
@ stub CryptGetMessageCertificates
@ stub CryptGetMessageSignerCount
@ stdcall CryptGetOIDFunctionAddress(long long str long ptr ptr)
@ stdcall CryptGetOIDFunctionValue(long str str wstr ptr ptr ptr)
@ stdcall CryptHashCertificate(long long long ptr long ptr ptr)
@ stub CryptHashMessage
@ stdcall CryptHashPublicKeyInfo(long long long long ptr ptr ptr)
@ stub CryptHashToBeSigned
@ stub CryptImportPKCS8
@ stdcall CryptImportPublicKeyInfo(long long ptr ptr)
@ stdcall CryptImportPublicKeyInfoEx(long long ptr long long ptr ptr)
@ stdcall CryptInitOIDFunctionSet(str long)
@ stdcall CryptInstallOIDFunctionAddress(ptr long str long ptr long)
@ stub CryptLoadSip
@ stdcall CryptMemAlloc(long)
@ stdcall CryptMemFree(ptr)
@ stdcall CryptMemRealloc(ptr long)
@ stub CryptMsgCalculateEncodedLength
@ stub CryptMsgClose
@ stub CryptMsgControl
@ stub CryptMsgCountersign
@ stub CryptMsgCountersignEncoded
@ stub CryptMsgEncodeAndSignCTL
@ stub CryptMsgGetAndVerifySigner
@ stub CryptMsgGetParam
@ stub CryptMsgOpenToDecode
@ stub CryptMsgOpenToEncode
@ stub CryptMsgSignCTL
@ stub CryptMsgUpdate
@ stub CryptMsgVerifyCountersignatureEncoded
@ stdcall CryptProtectData(ptr wstr ptr ptr ptr long ptr)
@ stdcall CryptQueryObject(long ptr long long long ptr ptr ptr ptr ptr ptr)
@ stdcall CryptRegisterDefaultOIDFunction(long str long wstr)
@ stdcall CryptRegisterOIDFunction(long str str wstr str)
@ stub CryptRegisterOIDInfo
@ stdcall CryptSIPAddProvider(ptr)
@ stdcall CryptSIPCreateIndirectData(ptr ptr ptr)
@ stdcall CryptSIPGetSignedDataMsg(ptr ptr long ptr ptr)
@ stdcall CryptSIPLoad(ptr long ptr)
@ stdcall CryptSIPPutSignedDataMsg(ptr long ptr long ptr)
@ stdcall CryptSIPRemoveProvider(ptr)
@ stdcall CryptSIPRemoveSignedDataMsg(ptr long)
@ stdcall CryptSIPRetrieveSubjectGuid(wstr long ptr)
@ stdcall CryptSIPVerifyIndirectData(ptr ptr)
@ stub CryptSetAsyncParam
@ stdcall CryptSetOIDFunctionValue(long str str wstr long ptr long)
@ stub CryptSetProviderU
@ stdcall CryptSignAndEncodeCertificate(long long long str ptr ptr ptr ptr ptr)
@ stub CryptSignAndEncryptMessage
@ stdcall CryptSignCertificate(long long long ptr long ptr ptr ptr ptr)
@ stub CryptSignHashU
@ stub CryptSignMessage
@ stub CryptSignMessageWithKey
@ stdcall CryptUnprotectData(ptr ptr ptr ptr ptr long ptr)
@ stdcall CryptUnregisterDefaultOIDFunction(long str wstr)
@ stdcall CryptUnregisterOIDFunction(long str str)
@ stub CryptUnregisterOIDInfo
@ stdcall CryptVerifyCertificateSignature(long long ptr long ptr)
@ stdcall CryptVerifyCertificateSignatureEx(long long long ptr long ptr long ptr)
@ stub CryptVerifyDetachedMessageHash
@ stub CryptVerifyDetachedMessageSignature
@ stub CryptVerifyMessageHash
@ stdcall CryptVerifyMessageSignature(ptr long ptr long ptr ptr ptr)
@ stub CryptVerifyMessageSignatureWithKey
@ stub CryptVerifySignatureU
@ stdcall I_CryptAllocTls()
@ stdcall I_CryptCreateLruCache(ptr ptr)
@ stub I_CryptCreateLruEntry
@ stdcall I_CryptDetachTls(long)
@ stdcall I_CryptFindLruEntryData(long long long)
@ stdcall I_CryptFlushLruCache(ptr long long)
@ stdcall I_CryptFreeLruCache(ptr long long)
@ stdcall I_CryptFreeTls(long long)
@ stdcall I_CryptGetDefaultCryptProv(long)
@ stub I_CryptGetDefaultCryptProvForEncrypt
@ stdcall I_CryptGetOssGlobal(long)
@ stdcall I_CryptGetTls(long)
@ stub I_CryptInsertLruEntry
@ stdcall I_CryptInstallAsn1Module(long long long)
@ stdcall I_CryptInstallOssGlobal(long long long)
@ stdcall I_CryptReadTrustedPublisherDWORDValueFromRegistry(wstr ptr)
@ stub I_CryptReleaseLruEntry
@ stdcall I_CryptSetTls(long ptr)
@ stdcall I_CryptUninstallAsn1Module(ptr)
@ stub I_CryptUninstallOssGlobal
@ stub PFXExportCertStore
@ stub PFXImportCertStore
@ stub RegCreateHKCUKeyExU
@ stub RegCreateKeyExU
@ stub RegDeleteValueU
@ stub RegEnumValueU
@ stub RegOpenHKCUKeyExU
@ stub RegOpenKeyExU
@ stub RegQueryInfoKeyU
@ stub RegQueryValueExU
@ stub RegSetValueExU
