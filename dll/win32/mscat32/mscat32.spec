1 stdcall CryptCATVerifyMember(ptr ptr ptr) wintrust.CryptCATVerifyMember
@ stdcall CatalogCompactHashDatabase() wintrust.CatalogCompactHashDatabase
@ stdcall CryptCATAdminAcquireContext(long ptr long) wintrust.CryptCATAdminAcquireContext
@ stdcall CryptCATAdminAddCatalog(long wstr wstr long) wintrust.CryptCATAdminAddCatalog
@ stdcall CryptCATAdminCalcHashFromFileHandle(long ptr ptr long) wintrust.CryptCATAdminCalcHashFromFileHandle
@ stdcall CryptCATAdminEnumCatalogFromHash(long ptr long long ptr) wintrust.CryptCATAdminEnumCatalogFromHash
@ stdcall CryptCATAdminReleaseCatalogContext(long long long) wintrust.CryptCATAdminReleaseCatalogContext
@ stdcall CryptCATAdminReleaseContext(long long) wintrust.CryptCATAdminReleaseContext
@ stdcall CryptCATCDFClose(ptr) wintrust.CryptCATCDFClose
@ stdcall CryptCATCDFEnumAttributes(ptr ptr ptr ptr) wintrust.CryptCATCDFEnumAttributes
@ stdcall CryptCATCDFEnumAttributesWithCDFTag(ptr wstr ptr ptr ptr) wintrust.CryptCATCDFEnumAttributesWithCDFTag
@ stdcall CryptCATCDFEnumCatAttributes(ptr ptr ptr) wintrust.CryptCATCDFEnumCatAttributes
@ stdcall CryptCATCDFEnumMembers(ptr ptr ptr) wintrust.CryptCATCDFEnumMembers
@ stdcall CryptCATCDFEnumMembersByCDFTag(ptr wstr ptr ptr long) wintrust.CryptCATCDFEnumMembersByCDFTag
@ stdcall CryptCATCDFEnumMembersByCDFTagEx(ptr wstr ptr ptr long ptr) wintrust.CryptCATCDFEnumMembersByCDFTagEx
@ stdcall CryptCATCDFOpen(wstr ptr) wintrust.CryptCATCDFOpen
@ stdcall CryptCATCatalogInfoFromContext(ptr ptr long) wintrust.CryptCATCatalogInfoFromContext
@ stdcall CryptCATClose(long) wintrust.CryptCATClose
@ stdcall CryptCATEnumerateAttr(ptr ptr ptr) wintrust.CryptCATEnumerateAttr
@ stdcall CryptCATEnumerateCatAttr(ptr ptr) wintrust.CryptCATEnumerateCatAttr
@ stdcall CryptCATEnumerateMember(long ptr) wintrust.CryptCATEnumerateMember
@ stdcall CryptCATGetAttrInfo(ptr ptr wstr) wintrust.CryptCATGetAttrInfo
@ stdcall CryptCATGetCatAttrInfo(ptr wstr) wintrust.CryptCATGetCatAttrInfo
@ stdcall CryptCATGetMemberInfo(ptr wstr) wintrust.CryptCATGetMemberInfo
@ stdcall CryptCATHandleFromStore(ptr) wintrust.CryptCATHandleFromStore
@ stdcall CryptCATOpen(wstr long long long long) wintrust.CryptCATOpen
@ stdcall CryptCATPersistStore(ptr) wintrust.CryptCATPersistStore
@ stdcall CryptCATPutAttrInfo(ptr ptr wstr long long ptr) wintrust.CryptCATPutAttrInfo
@ stdcall CryptCATPutCatAttrInfo(ptr wstr long long ptr) wintrust.CryptCATPutCatAttrInfo
@ stdcall CryptCATPutMemberInfo(ptr wstr wstr ptr long long ptr) wintrust.CryptCATPutMemberInfo
@ stdcall CryptCATStoreFromHandle(ptr) wintrust.CryptCATStoreFromHandle
@ stdcall -private DllRegisterServer() wintrust.mscat32DllRegisterServer
@ stdcall -private DllUnregisterServer() wintrust.mscat32DllUnregisterServer
@ stdcall IsCatalogFile(ptr wstr) wintrust.IsCatalogFile
@ stdcall MsCatConstructHashTag(long ptr ptr) wintrust.MsCatConstructHashTag
@ stdcall MsCatFreeHashTag(ptr) wintrust.MsCatFreeHashTag
