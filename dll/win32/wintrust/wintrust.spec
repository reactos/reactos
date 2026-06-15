@ stub AddPersonalTrustDBPages
@ stub CatalogCompactHashDatabase
#@ stub ComputeFirstPageHash
#@ stub ConfigCiFinalPolicy
#@ stub ConfigCiPackageFamilyNameCheck
@ stdcall CryptCATAdminAcquireContext(ptr ptr long)
@ stdcall CryptCATAdminAcquireContext2(ptr ptr wstr ptr long)
@ stdcall CryptCATAdminAddCatalog(long wstr wstr long)
@ stdcall CryptCATAdminCalcHashFromFileHandle(long ptr ptr long)
@ stdcall CryptCATAdminCalcHashFromFileHandle2(ptr ptr ptr ptr long)
@ stdcall CryptCATAdminEnumCatalogFromHash(long ptr long long ptr)
@ stub CryptCATAdminPauseServiceForBackup
@ stdcall CryptCATAdminReleaseCatalogContext(long long long)
@ stdcall CryptCATAdminReleaseContext(long long)
@ stdcall CryptCATAdminRemoveCatalog(ptr wstr long)
@ stdcall CryptCATAdminResolveCatalogPath(ptr wstr ptr long)
#@ stub CryptCATAllocSortedMemberInfo
@ stdcall CryptCATCDFClose(ptr)
@ stub CryptCATCDFEnumAttributes
@ stub CryptCATCDFEnumAttributesWithCDFTag
@ stdcall CryptCATCDFEnumCatAttributes(ptr ptr ptr)
@ stub CryptCATCDFEnumMembers
@ stub CryptCATCDFEnumMembersByCDFTag
@ stdcall CryptCATCDFEnumMembersByCDFTagEx(ptr wstr ptr ptr long ptr)
@ stdcall CryptCATCDFOpen(wstr ptr)
@ stdcall CryptCATCatalogInfoFromContext(ptr ptr long)
@ stdcall CryptCATClose(long)
@ stdcall CryptCATEnumerateAttr(ptr ptr ptr)
@ stdcall CryptCATEnumerateCatAttr(ptr ptr)
@ stdcall CryptCATEnumerateMember(long ptr)
#@ stub CryptCATFreeSortedMemberInfo
@ stdcall CryptCATGetAttrInfo(ptr ptr wstr)
@ stdcall CryptCATGetCatAttrInfo(ptr wstr )
@ stdcall CryptCATGetMemberInfo(ptr wstr)
@ stub CryptCATHandleFromStore
@ stdcall CryptCATOpen(wstr long long long long)
@ stdcall CryptCATPersistStore(ptr)
@ stdcall CryptCATPutAttrInfo(ptr ptr wstr long long ptr)
@ stdcall CryptCATPutCatAttrInfo(ptr wstr long long ptr)
@ stdcall CryptCATPutMemberInfo(ptr wstr wstr ptr long long ptr)
@ stub CryptCATStoreFromHandle
@ stub CryptCATVerifyMember
@ stdcall CryptSIPCreateIndirectData(ptr ptr ptr)
#@ stub CryptSIPGetCaps
@ stub CryptSIPGetInfo
@ stub CryptSIPGetRegWorkingFlags
#@ stub CryptSIPGetSealedDigest
@ stdcall CryptSIPGetSignedDataMsg(ptr ptr long ptr ptr)
@ stdcall CryptSIPPutSignedDataMsg(ptr long ptr long ptr)
@ stdcall CryptSIPRemoveSignedDataMsg(ptr long)
@ stdcall CryptSIPVerifyIndirectData(ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DriverCleanupPolicy(ptr)
@ stdcall DriverFinalPolicy(ptr)
@ stdcall DriverInitializePolicy(ptr)
@ stdcall FindCertsByIssuer(ptr ptr ptr ptr long wstr long)
@ stdcall GenericChainCertificateTrust(ptr)
@ stdcall GenericChainFinalProv(ptr)
#@ stub GetAuthenticodeSha256Hash
@ stdcall HTTPSCertificateTrust(ptr)
@ stdcall HTTPSFinalProv(ptr)
@ stdcall IsCatalogFile(ptr wstr)
@ stub MsCatConstructHashTag
@ stub MsCatFreeHashTag
@ stub OfficeCleanupPolicy
@ stub OfficeInitializePolicy
@ stdcall OpenPersonalTrustDBDialog(ptr)
#@ stub OpenPersonalTrustDBDialogEx
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
#@ stub SrpCheckSmartlockerEAandProcessToken
@ stub TrustDecode
@ stub TrustFindIssuerCertificate
@ stub TrustFreeDecode
@ stdcall TrustIsCertificateSelfSigned(ptr)
@ stub TrustOpenStores
#@ stub WTGetBioSignatureInfo
#@ stub WTGetPluginSignatureInfo
#@ stub WTGetSignatureInfo
@ stdcall WTHelperCertCheckValidSignature(ptr)
@ stub WTHelperCertFindIssuerCertificate
@ stub WTHelperCertIsSelfSigned
@ stub WTHelperCheckCertUsage
@ stub WTHelperGetAgencyInfo
@ stdcall WTHelperGetFileHandle(ptr)
#@ stub WTHelperGetFileHash
@ stdcall WTHelperGetFileName(ptr)
@ stdcall WTHelperGetKnownUsages(long ptr)
@ stdcall WTHelperGetProvCertFromChain(ptr long)
@ stdcall WTHelperGetProvPrivateDataFromChain(ptr ptr)
@ stdcall WTHelperGetProvSignerFromChain(ptr long long long)
#@ stub WTHelperIsChainedToMicrosoft
#@ stub WTHelperIsChainedToMicrosoftFromStateData
@ stub WTHelperIsInRootStore
@ stub WTHelperOpenKnownStores
@ stdcall WTHelperProvDataFromStateData(ptr)
#@ stub WTIsFirstConfigCiResultPreferred
#@ stub WTLogConfigCiScriptEvent
#@ stub WTLogConfigCiSignerEvent
#@ stub WTValidateBioSignaturePolicy
#@ stub WVTAsn1CatMemberInfo2Decode
#@ stub WVTAsn1CatMemberInfo2Encode
@ stdcall WVTAsn1CatMemberInfoDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1CatMemberInfoEncode(long str ptr ptr ptr)
@ stdcall WVTAsn1CatNameValueDecode(long str ptr long long ptr ptr)
@ stdcall WVTAsn1CatNameValueEncode(long str ptr ptr ptr)
#@ stub WVTAsn1IntentToSealAttributeDecode
#@ stub WVTAsn1IntentToSealAttributeEncode
#@ stub WVTAsn1SealingSignatureAttributeDecode
#@ stub WVTAsn1SealingSignatureAttributeEncode
#@ stub WVTAsn1SealingTimestampAttributeDecode
#@ stub WVTAsn1SealingTimestampAttributeEncode
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
@ stdcall WintrustAddDefaultForUsage(str ptr)
@ stdcall WintrustCertificateTrust(ptr)
@ stub WintrustGetDefaultForUsage
@ stdcall WintrustGetRegPolicyFlags(ptr)
@ stdcall WintrustLoadFunctionPointers(ptr ptr)
@ stdcall WintrustRemoveActionID(ptr)
#@ stub WintrustSetDefaultIncludePEPageHashes
@ stdcall WintrustSetRegPolicyFlags(long)
@ stdcall mscat32DllRegisterServer()
@ stdcall mscat32DllUnregisterServer()
@ stdcall mssip32DllRegisterServer()
@ stdcall mssip32DllUnregisterServer()
