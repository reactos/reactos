@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub WinHttpAddRequestHeaders
@ stdcall WinHttpCheckPlatform()
@ stub WinHttpCloseHandle
@ stub WinHttpConnect
@ stub WinHttpCrackUrl
@ stub WinHttpCreateUrl
@ stdcall WinHttpDetectAutoProxyConfigUrl(long ptr)
@ stub WinHttpGetDefaultProxyConfiguration
@ stdcall WinHttpGetIEProxyConfigForCurrentUser(ptr)
@ stub WinHttpGetProxyForUrl
@ stdcall WinHttpOpen(wstr long wstr wstr long)
@ stub WinHttpOpenRequest
@ stub WinHttpQueryAuthSchemes
@ stub WinHttpQueryDataAvailable
@ stub WinHttpQueryHeaders
@ stub WinHttpQueryOption
@ stub WinHttpReadData
@ stub WinHttpReceiveResponse
@ stub WinHttpSendRequest
@ stub WinHttpSetCredentials
@ stub WinHttpSetDefaultProxyConfiguration
@ stub WinHttpSetOption
@ stub WinHttpSetStatusCallback
@ stub WinHttpSetTimeouts
@ stub WinHttpTimeFromSystemTime
@ stub WinHttpTimeToSystemTime
@ stub WinHttpWriteData
