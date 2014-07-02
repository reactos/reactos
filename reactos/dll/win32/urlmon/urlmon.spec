#100 ZoneMappingToRegKey
#101 stub AsyncGetClassBits
102 stdcall AsyncInstallDistributionUnit(ptr ptr ptr long long ptr ptr ptr long)
103 stdcall BindAsyncMoniker(ptr long ptr ptr ptr)
#104 stub CDLGetLongPathNameA
#105 stub CDLGetLongPathNameW
106 stdcall CoGetClassObjectFromURL(ptr wstr long long wstr ptr long ptr ptr ptr)
#107 stub CoInstall
108 stdcall CoInternetCombineUrl(wstr wstr long ptr long ptr long)
109 stdcall CoInternetCompareUrl(wstr wstr long)
110 stdcall CoInternetCreateSecurityManager(ptr ptr long)
111 stdcall CoInternetCreateZoneManager(ptr ptr long)
#112 CoInternetFeatureSettingsChanged
#113 stub CoInternetGetProtocolFlags
114 stdcall CoInternetGetSecurityUrl(ptr ptr long long)
115 stdcall CoInternetGetSession(long ptr long)
116 stdcall CoInternetIsFeatureEnabled(long long)
117 stdcall CoInternetIsFeatureEnabledForUrl(long long wstr ptr)
118 stdcall CoInternetIsFeatureZoneElevationEnabled(wstr wstr ptr long)
119 stdcall CoInternetParseUrl(wstr long long wstr long ptr long)
120 stdcall CoInternetQueryInfo(ptr long long ptr long ptr long)
121 stdcall CoInternetSetFeatureEnabled(long long long)
122 stdcall CompareSecurityIds(ptr long ptr long long)
#123 CompatFlagsFromClsid
124 stdcall CopyBindInfo(ptr ptr)
125 stdcall CopyStgMedium(ptr ptr)
126 stdcall CreateAsyncBindCtx(long ptr ptr ptr)
127 stdcall CreateAsyncBindCtxEx(ptr long ptr ptr ptr long)
128 stdcall CreateFormatEnumerator(long ptr ptr)
129 stdcall CreateURLMoniker(ptr wstr ptr)
130 stdcall CreateURLMonikerEx(ptr wstr ptr long)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllRegisterServerEx()
@ stdcall -private DllUnregisterServer()
137 stdcall Extract(long ptr)
138 stdcall FaultInIEFeature(long ptr ptr long)
#139 stub FindMediaType
#140 stub FindMediaTypeClass
141 stdcall FindMimeFromData(long ptr ptr long ptr long ptr long)
#142 GetAddSitesFileUrl
143 stdcall GetClassFileOrMime(ptr wstr ptr long wstr long ptr)
#144 stub GetClassURL
#145 stub GetComponentIDFromCLSSPEC
#146 stub GetMarkOfTheWeb
147 stdcall GetSoftwareUpdateInfo(ptr ptr)
148 stdcall -stub HlinkGoBack(ptr)
#149 stub HlinkGoForward
#150 stub HlinkNavigateMoniker
151 stdcall HlinkNavigateString(ptr wstr)
152 stdcall HlinkSimpleNavigateToMoniker(ptr wstr wstr ptr ptr ptr long long)
153 stdcall HlinkSimpleNavigateToString(wstr wstr wstr ptr ptr ptr long long)
#154 InstallFlash
155 stdcall IsAsyncMoniker(ptr)
#156 stub IsJITInProgress
157 stdcall IsLoggingEnabledA(str)
158 stdcall IsLoggingEnabledW(wstr)
159 stdcall IsValidURL(ptr wstr long)
160 stdcall MkParseDisplayNameEx(ptr wstr ptr ptr)
161 stdcall ObtainUserAgentString(long str ptr)
#162 stub PrivateCoInstall
163 stdcall RegisterBindStatusCallback(ptr ptr ptr long)
164 stdcall RegisterFormatEnumerator(ptr ptr long)
#165 stub RegisterMediaTypeClass
166 stdcall RegisterMediaTypes(long ptr ptr)
167 stdcall ReleaseBindInfo(ptr)
168 stdcall RevokeBindStatusCallback(ptr ptr)
169 stdcall RevokeFormatEnumerator(ptr ptr)
#170 stub SetSoftwareUpdateAdvertisementState
#171 ShowTrustAlertDialog
#172 stub URLDownloadA
173 stdcall URLDownloadToCacheFileA(ptr str str long long ptr)
174 stdcall URLDownloadToCacheFileW(ptr wstr wstr long long ptr)
175 stdcall URLDownloadToFileA(ptr str str long ptr)
176 stdcall URLDownloadToFileW(ptr wstr wstr long ptr)
#177 stub URLDownloadW
178 stdcall URLOpenBlockingStreamA(ptr str ptr long ptr)
179 stdcall URLOpenBlockingStreamW(ptr wstr ptr long ptr)
#180 stub URLOpenPullStreamA
#181 stub URLOpenPullStreamW
182 stdcall URLOpenStreamA(ptr str long ptr)
183 stdcall URLOpenStreamW(ptr wstr long ptr)
#184 stub UrlMkBuildVersion
185 stdcall UrlMkGetSessionOption(long ptr long ptr long)
186 stdcall UrlMkSetSessionOption(long ptr long long)
#187 stub WriteHitLogging
#188 stub ZonesReInit

#FIXME: Needed by Wine
@ stdcall CoInternetCombineUrlEx(ptr wstr long ptr long)
@ stdcall CoInternetParseIUri(ptr long long wstr long ptr long)
@ stdcall CreateIUriBuilder(ptr long long ptr)
@ stdcall CreateUri(wstr long long ptr)
@ stdcall CreateURLMonikerEx2(ptr ptr ptr long)
