# Up until Windows 2000 these APIs have hardcoded ordinals.
# Keep it that way for compatibility.
1 stub CDLGetLongPathNameA
2 stub CDLGetLongPathNameW
# IsJITInProgress has a hardcoded ordinal on WinME and Windows 2000
#3 stub IsJITInProgress

@ stub AsyncGetClassBits
@ stub AsyncInstallDistributionUnit
@ stdcall BindAsyncMoniker(ptr long ptr ptr ptr)
@ stdcall CoGetClassObjectFromURL(ptr wstr long long wstr ptr long ptr ptr ptr)
@ stub CoInstall
@ stdcall CoInternetCombineUrl(wstr wstr long wstr long ptr long)
@ stdcall CoInternetCompareUrl(wstr wstr long)
@ stdcall CoInternetCreateSecurityManager(ptr ptr long)
@ stdcall CoInternetCreateZoneManager(ptr ptr long)
@ stub CoInternetGetProtocolFlags
@ stub CoInternetGetSecurityUrl
@ stdcall CoInternetGetSession(long ptr long)
@ stdcall CoInternetParseUrl(wstr long long wstr long ptr long)
@ stdcall CoInternetQueryInfo(ptr long long ptr long ptr long)
@ stub CompareSecurityIds
@ stub CopyBindInfo
@ stub CopyStgMedium
@ stdcall CreateAsyncBindCtx(long ptr ptr ptr)
@ stdcall CreateAsyncBindCtxEx(ptr long ptr ptr ptr long)
@ stdcall CreateFormatEnumerator(long ptr ptr)
@ stdcall CreateURLMoniker(ptr wstr ptr)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllRegisterServerEx()
@ stdcall -private DllUnregisterServer()
@ stdcall Extract(long ptr) cabinet.Extract
@ stdcall FaultInIEFeature(long ptr ptr long)
@ stub FindMediaType
@ stub FindMediaTypeClass
@ stdcall FindMimeFromData(long ptr ptr long ptr long ptr long)
@ stub GetClassFileOrMime
@ stub GetClassURL
@ stub GetComponentIDFromCLSSPEC
@ stub GetMarkOfTheWeb
@ stdcall GetSoftwareUpdateInfo(ptr ptr)
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
@ stdcall RegisterFormatEnumerator(ptr ptr long)
@ stub RegisterMediaTypeClass
@ stub RegisterMediaTypes
@ stdcall ReleaseBindInfo(ptr)
@ stdcall RevokeBindStatusCallback(ptr ptr)
@ stdcall RevokeFormatEnumerator(ptr ptr)
@ stub SetSoftwareUpdateAdvertisementState
@ stub URLDownloadA
@ stdcall URLDownloadToCacheFileA(ptr str str long long ptr)
@ stdcall URLDownloadToCacheFileW(ptr wstr wstr long long ptr)
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
