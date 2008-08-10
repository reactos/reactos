@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stub WinHttpAddRequestHeaders
@ stdcall WinHttpCheckPlatform()
@ stdcall WinHttpCloseHandle(ptr)
@ stdcall WinHttpConnect(ptr wstr long long)
@ stub WinHttpCrackUrl
@ stub WinHttpCreateUrl
@ stdcall WinHttpDetectAutoProxyConfigUrl(long ptr)
@ stub WinHttpGetDefaultProxyConfiguration
@ stdcall WinHttpGetIEProxyConfigForCurrentUser(ptr)
@ stub WinHttpGetProxyForUrl
@ stdcall WinHttpOpen(wstr long wstr wstr long)
@ stdcall WinHttpOpenRequest(ptr wstr wstr wstr wstr ptr long)
@ stub WinHttpQueryAuthSchemes
@ stdcall WinHttpQueryDataAvailable(ptr ptr)
@ stub WinHttpQueryHeaders
@ stdcall WinHttpQueryOption(ptr long ptr ptr)
@ stdcall WinHttpReadData(ptr ptr long ptr)
@ stdcall WinHttpReceiveResponse(ptr ptr)
@ stdcall WinHttpSendRequest(ptr wstr long ptr long long ptr)
@ stub WinHttpSetCredentials
@ stub WinHttpSetDefaultProxyConfiguration
@ stdcall WinHttpSetOption(ptr long ptr long)
@ stub WinHttpSetStatusCallback
@ stub WinHttpSetTimeouts
@ stub WinHttpTimeFromSystemTime
@ stub WinHttpTimeToSystemTime
@ stub WinHttpWriteData
