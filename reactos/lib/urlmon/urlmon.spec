1 stub CDLGetLongPathNameA
2 stub CDLGetLongPathNameW
@ stub AsyncGetClassBits
@ stub AsyncInstallDistributionUnit
@ stub BindAsyncMoniker
@ stub CoGetClassObjectFromURL
@ stub CoInstall
@ stdcall CoInternetCombineUrl(wstr wstr long wstr long ptr long)
@ stdcall CoInternetCompareUrl(wstr wstr long)
@ stdcall CoInternetCreateSecurityManager(ptr ptr long)
@ stdcall CoInternetCreateZoneManager(ptr ptr long)
@ stub CoInternetGetProtocolFlags
@ stub CoInternetGetSecurityUrl
@ stdcall CoInternetGetSession(long ptr long)
@ stub CoInternetParseUrl
@ stdcall CoInternetQueryInfo(ptr long long ptr long ptr long)
@ stub CompareSecurityIds
@ stub CopyBindInfo
@ stub CopyStgMedium
@ stdcall CreateAsyncBindCtx(long ptr ptr ptr)
@ stdcall CreateAsyncBindCtxEx(ptr long ptr ptr ptr long)
@ stub CreateFormatEnumerator
@ stdcall CreateURLMoniker(ptr wstr ptr)
@ stdcall -private DllCanUnloadNow() URLMON_DllCanUnloadNow
@ stdcall -private DllGetClassObject(ptr ptr ptr) URLMON_DllGetClassObject
@ stdcall DllInstall(long ptr) URLMON_DllInstall
@ stdcall -private DllRegisterServer() URLMON_DllRegisterServer
@ stdcall -private DllRegisterServerEx() URLMON_DllRegisterServerEx
@ stdcall -private DllUnregisterServer() URLMON_DllUnregisterServer
@ stdcall Extract(long ptr) cabinet.Extract
@ stub FaultInIEFeature
@ stub FindMediaType
@ stub FindMediaTypeClass
@ stdcall FindMimeFromData(long ptr ptr long ptr long ptr long)
@ stub GetClassFileOrMime
@ stub GetClassURL
@ stub GetComponentIDFromCLSSPEC
@ stub GetMarkOfTheWeb
@ stub GetSoftwareUpdateInfo
@ stub HlinkGoBack
@ stub HlinkGoForward
@ stub HlinkNavigateMoniker
@ stdcall HlinkNavigateString(ptr wstr)
@ stub HlinkSimpleNavigateToMoniker
@ stdcall HlinkSimpleNavigateToString(wstr wstr wstr ptr ptr ptr long long)
@ stdcall IsAsyncMoniker(ptr)
@ stub IsLoggingEnabledA
@ stub IsLoggingEnabledW
@ stdcall IsValidURL(ptr wstr long)
@ stdcall MkParseDisplayNameEx(ptr ptr ptr ptr) ole32.MkParseDisplayName
@ stdcall ObtainUserAgentString(long str ptr)
@ stub PrivateCoInstall
@ stdcall RegisterBindStatusCallback(ptr ptr ptr long)
@ stub RegisterFormatEnumerator
@ stub RegisterMediaTypeClass
@ stub RegisterMediaTypes
@ stdcall ReleaseBindInfo(ptr)
@ stdcall RevokeBindStatusCallback(ptr ptr)
@ stub RevokeFormatEnumerator
@ stub SetSoftwareUpdateAdvertisementState
@ stub URLDownloadA
@ stub URLDownloadToCacheFileA
@ stub URLDownloadToCacheFileW
@ stdcall URLDownloadToFileA(ptr str str long ptr)
@ stdcall URLDownloadToFileW(ptr wstr wstr long ptr)
@ stub URLDownloadW
@ stub URLOpenBlockingStreamA
@ stub URLOpenBlockingStreamW
@ stub URLOpenPullStreamA
@ stub URLOpenPullStreamW
@ stub URLOpenStreamA
@ stub URLOpenStreamW
@ stub UrlMkBuildVersion
@ stdcall UrlMkGetSessionOption(long ptr long ptr long)
@ stdcall UrlMkSetSessionOption(long ptr long long)
@ stub WriteHitLogging
@ stub ZonesReInit
