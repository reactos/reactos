@ stub AddPersonalTrustDBPages
@ stub CatalogCompactHashDatabase
@ stdcall CryptCATAdminAcquireContext(long ptr long)
@ stdcall CryptCATAdminAddCatalog(long wstr wstr long)
@ stdcall CryptCATAdminCalcHashFromFileHandle(long ptr ptr long)
@ stdcall CryptCATAdminEnumCatalogFromHash(long ptr long long ptr)
@ stub CryptCATAdminPauseServiceForBackup
@ stdcall CryptCATAdminReleaseCatalogContext(long long long)
@ stdcall CryptCATAdminReleaseContext(long long)
@ stdcall CryptCATAdminRemoveCatalog(ptr wstr long)
@ stub CryptCATAdminResolveCatalogPath
@ stub CryptCATCDFClose
@ stub CryptCATCDFEnumAttributes
@ stub CryptCATCDFEnumAttributesWithCDFTag
@ stub CryptCATCDFEnumCatAttributes
@ stub CryptCATCDFEnumMembers
@ stub CryptCATCDFEnumMembersByCDFTag
@ stub CryptCATCDFEnumMembersByCDFTagEx
@ stub CryptCATCDFOpen
@ stub CryptCATCatalogInfoFromContext
@ stdcall CryptCATClose(long)
@ stub CryptCATEnumerateAttr
@ stub CryptCATEnumerateCatAttr
@ stdcall CryptCATEnumerateMember(long ptr)
@ stub CryptCATGetAttrInfo
@ stub CryptCATGetCatAttrInfo
@ stub CryptCATGetMemberInfo
@ stub CryptCATHandleFromStore
@ stdcall CryptCATOpen(wstr long long long long)
@ stub CryptCATPersistStore
@ stub CryptCATPutAttrInfo
@ stub CryptCATPutCatAttrInfo
@ stub CryptCATPutMemberInfo
@ stub CryptCATStoreFromHandle
@ stub CryptCATVerifyMember
@ stdcall CryptSIPCreateIndirectData(ptr ptr ptr)
@ stub CryptSIPGetInfo
@ stub CryptSIPGetRegWorkingFlags
@ stdcall CryptSIPGetSignedDataMsg(ptr ptr long ptr ptr)
@ stdcall CryptSIPPutSignedDataMsg(ptr long ptr long ptr)
@ stdcall CryptSIPRemoveSignedDataMsg(ptr long)
@ stdcall CryptSIPVerifyIndirectData(ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub DriverCleanupPolicy
@ stub DriverFinalPolicy
@ stub DriverInitializePolicy
@ stub FindCertsByIssuer
@ stdcall GenericChainCertificateTrust(ptr)
@ stdcall GenericChainFinalProv(ptr)
@ stub HTTPSCertificateTrust
@ stub HTTPSFinalProv
@ stub IsCatalogFile
@ stub MsCatConstructHashTag
@ stub MsCatFreeHashTag
@ stub OfficeCleanupPolicy
@ stub OfficeInitializePolicy
@ stdcall OpenPersonalTrustDBDialog(ptr)
@ stdcall SoftpubAuthenticode(ptr)
@ stdcall SoftpubCheckCert(ptr long long long)
@ stdcall SoftpubCleanup(ptr)
@ stdcall SoftpubDefCertInit(ptr)
@ stdcall SoftpubDllRegisterServer()
@ stdcall SoftpubDllUnregisterServer()
@ stub SoftpubDumpStructure
@ stub SoftpubFreeDefUsageCallData
@ stdcall SoftpubInitialize(ptr)
@ stub SoftpubLoadDefUsageCallData
@ stdcall SoftpubLoadMessage(ptr)
@ stdcall SoftpubLoadSignature(ptr)
@ stub TrustDecode
@ stub TrustFindIssuerCertificate
@ stub TrustFreeDecode
@ stdcall TrustIsCertificateSelfSigned(ptr)
@ stub TrustOpenStores
@ stub WTHelperCertFindIssuerCertificate
@ stub WTHelperCertIsSelfSigned
@ stub WTHelperCheckCertUsage
@ stub WTHelperGetAgencyInfo
@ stdcall WTHelperGetFileHandle(ptr)
@ stdcall WTHelperGetFileName(ptr)
@ stdcall WTHelperGetKnownUsages(long ptr)
@ stdcall WTHelperGetProvCertFromChain(ptr long)
@ stdcall WTHelperGetProvPrivateDataFromChain(ptr ptr)
@ stdcall WTHelperGetProvSignerFromChain(ptr long long long)
@ stub WTHelperIsInRootStore
@ stub WTHelperOpenKnownStores
@ stdcall WTHelperProvDataFromStateData(ptr)
@ stdcall WVTAsn1CatMemberInfoDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1CatMemberInfoEncode(long str ptr ptr ptr)
@ stdcall WVTAsn1CatNameValueDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1CatNameValueEncode(long str ptr ptr ptr)
@ stdcall WVTAsn1SpcFinancialCriteriaInfoDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1SpcFinancialCriteriaInfoEncode(long str ptr ptr ptr)
@ stdcall WVTAsn1SpcIndirectDataContentDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1SpcIndirectDataContentEncode(long str ptr ptr ptr)
@ stdcall WVTAsn1SpcLinkDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1SpcLinkEncode(long str ptr ptr ptr)
@ stub WVTAsn1SpcMinimalCriteriaInfoDecode
@ stub WVTAsn1SpcMinimalCriteriaInfoEncode
@ stdcall WVTAsn1SpcPeImageDataDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1SpcPeImageDataEncode(long str ptr ptr ptr)
@ stub WVTAsn1SpcSigInfoDecode
@ stub WVTAsn1SpcSigInfoEncode
@ stub WVTAsn1SpcSpAgencyInfoDecode
@ stub WVTAsn1SpcSpAgencyInfoEncode
@ stdcall WVTAsn1SpcSpOpusInfoDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1SpcSpOpusInfoEncode(long str ptr ptr ptr)
@ stub WVTAsn1SpcStatementTypeDecode
@ stub WVTAsn1SpcStatementTypeEncode
@ stdcall WinVerifyTrust(long ptr ptr)
@ stdcall WinVerifyTrustEx(long ptr ptr)
@ stdcall WintrustAddActionID(ptr long ptr)
@ stdcall WintrustAddDefaultForUsage(ptr ptr)
@ stdcall WintrustCertificateTrust(ptr)
@ stub WintrustGetDefaultForUsage
@ stdcall WintrustGetRegPolicyFlags(ptr)
@ stdcall WintrustLoadFunctionPointers(ptr ptr)
@ stdcall WintrustRemoveActionID(ptr)
@ stdcall WintrustSetRegPolicyFlags(long)
@ stdcall mscat32DllRegisterServer()
@ stdcall mscat32DllUnregisterServer()
@ stdcall mssip32DllRegisterServer()
@ stdcall mssip32DllUnregisterServer()
