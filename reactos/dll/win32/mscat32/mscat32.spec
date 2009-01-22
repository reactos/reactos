@ stub CryptCATVerifyMember
@ stdcall CryptCATAdminAcquireContext(long ptr long) wintrust.CryptCATAdminAcquireContext
@ stdcall CryptCATAdminAddCatalog(long wstr wstr long) wintrust.CryptCATAdminAddCatalog
@ stdcall CryptCATAdminCalcHashFromFileHandle(long ptr ptr long) wintrust.CryptCATAdminCalcHashFromFileHandle
@ stdcall CryptCATAdminEnumCatalogFromHash(long ptr long long ptr) wintrust.CryptCATAdminEnumCatalogFromHash
@ stdcall CryptCATAdminReleaseCatalogContext(long long long) wintrust.CryptCATAdminReleaseCatalogContext
@ stdcall CryptCATAdminReleaseContext(long long) wintrust.CryptCATAdminReleaseContext
@ stub CryptCATCDFClose
@ stub CryptCATCDFEnumAttributes
@ stub CryptCATCDFEnumAttributesWithCDFTag
@ stub CryptCATCDFEnumCatAttributes
@ stub CryptCATCDFEnumMembers
@ stub CryptCATCDFEnumMembersByCDFTag
@ stub CryptCATCDFOpen
@ stub CryptCATCatalogInfoFromContext
@ stdcall CryptCATClose(long) wintrust.CryptCATClose
@ stub CryptCATEnumerateAttr
@ stub CryptCATEnumerateCatAttr
@ stdcall CryptCATEnumerateMember(long ptr) wintrust.CryptCATEnumerateMember
@ stub CryptCATGetAttrInfo
@ stub CryptCATGetCatAttrInfo
@ stub CryptCATGetMemberInfo
@ stub CryptCATHandleFromStore
@ stdcall CryptCATOpen(wstr long long long long) wintrust.CryptCATOpen
@ stub CryptCATPersistStore
@ stub CryptCATPutAttrInfo
@ stub CryptCATPutCatAttrInfo
@ stub CryptCATPutMemberInfo
@ stub CryptCATStoreFromHandle
@ stdcall -private DllRegisterServer() wintrust.mscat32DllRegisterServer
@ stdcall -private DllUnregisterServer() wintrust.mscat32DllUnregisterServer
@ stub IsCatalogFile
@ stub MsCatConstructHashTag
@ stub MsCatFreeHashTag
