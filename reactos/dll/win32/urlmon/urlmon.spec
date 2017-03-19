#100 ZoneMappingToRegKey
#101 CoInternetIsExtensionsOff
#102 CoInternetSetExtensionsOff
#103 CoInternetExtensionAllowed
#104 CoInternetCreateExtension
#105 CoInternetExtensionCollectStats
#106 CoInternetExtensionNeedsApproval
#107 CoInternetApproveExtension
108 stdcall -noname IsInternetESCEnabledLocal()
#109 stub AsyncGetClassBits
110 stdcall AsyncInstallDistributionUnit(ptr ptr ptr long long ptr ptr ptr long)
111 stdcall -noname IsProtectedModeURL(ptr)
#112 IsProtectedModeIUri
#113 IsFileInSpecialDirs
#114 SkipBrokerCheckForURL
#115 IEIsProtectedModeURLInternal
#116 CoInternetDomainNeedsApproval
117 stdcall BindAsyncMoniker(ptr long ptr ptr ptr)
#118 stub CDLGetLongPathNameA
#119 stub CDLGetLongPathNameW
120 stdcall CoGetClassObjectFromURL(ptr wstr long long wstr ptr long ptr ptr ptr)
#121 stub CoInstall
#122 CoInternetCanonicalizeIUri
123 stdcall CoInternetCombineIUri(ptr ptr long ptr long)
124 stdcall CoInternetCombineUrl(wstr wstr long ptr long ptr long)
125 stdcall CoInternetCombineUrlEx(ptr wstr long ptr long)
126 stdcall CoInternetCompareUrl(wstr wstr long)
127 stdcall CoInternetCreateSecurityManager(ptr ptr long)
128 stdcall CoInternetCreateZoneManager(ptr ptr long)
#129 CoInternetFeatureSettingsChanged
#130 stub CoInternetGetProtocolFlags
131 stdcall CoInternetGetSecurityUrl(ptr ptr long long)
132 stdcall CoInternetGetSecurityUrlEx(ptr ptr long long)
133 stdcall CoInternetGetSession(long ptr long)
134 stdcall CoInternetIsFeatureEnabled(long long)
#135 CoInternetIsFeatureEnabledForIUri
136 stdcall CoInternetIsFeatureEnabledForUrl(long long wstr ptr)
137 stdcall CoInternetIsFeatureZoneElevationEnabled(wstr wstr ptr long)
138 stdcall CoInternetParseIUri(ptr long long wstr long ptr long)
139 stdcall CoInternetParseUrl(wstr long long wstr long ptr long)
140 stdcall CoInternetQueryInfo(ptr long long ptr long ptr long)
141 stdcall CoInternetSetFeatureEnabled(long long long)
142 stdcall CompareSecurityIds(ptr long ptr long long)
#143 CompatFlagsFromClsid
144 stdcall CopyBindInfo(ptr ptr)
145 stdcall CopyStgMedium(ptr ptr)
146 stdcall CreateAsyncBindCtx(long ptr ptr ptr)
147 stdcall CreateAsyncBindCtxEx(ptr long ptr ptr ptr long)
148 stdcall CreateFormatEnumerator(long ptr ptr)
149 stdcall CreateIUriBuilder(ptr long long ptr)
150 stdcall CreateURLMoniker(ptr wstr ptr)
151 stdcall CreateURLMonikerEx2(ptr ptr ptr long)
152 stdcall CreateURLMonikerEx(ptr wstr ptr long)
153 stdcall CreateUri(wstr long long ptr)
#154 CreateUriFromMultiByteString
#155 CreateUriPriv
156 stdcall CreateUriWithFragment(wstr wstr long long ptr)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllRegisterServerEx()
@ stdcall -private DllUnregisterServer()
163 stdcall Extract(long ptr)
164 stdcall FaultInIEFeature(long ptr ptr long)
#165 stub FindMediaType
#166 stub FindMediaTypeClass
167 stdcall FindMimeFromData(ptr ptr ptr long ptr long ptr long)
#168 GetAddSitesFileUrl
169 stdcall GetClassFileOrMime(ptr wstr ptr long wstr long ptr)
#170 stub GetClassURL
#171 stub GetComponentIDFromCLSSPEC
#172 GetIDNFlagsForUri
#173 GetIUriPriv
#174 GetLabelsFromNamedHost
#175 stub GetMarkOfTheWeb
#176 GetPortFromUrlScheme
#177 GetPropertyFromName
#178 GetPropertyName
179 stdcall GetSoftwareUpdateInfo(ptr ptr)
#180 GetUrlmonThreadNotificationHwnd
181 stdcall -stub HlinkGoBack(ptr)
182 stdcall -stub HlinkGoForward(ptr)
#183 stub HlinkNavigateMoniker
184 stdcall HlinkNavigateString(ptr wstr)
185 stdcall HlinkSimpleNavigateToMoniker(ptr wstr wstr ptr ptr ptr long long)
186 stdcall HlinkSimpleNavigateToString(wstr wstr wstr ptr ptr ptr long long)
187 stdcall -stub IEInstallScope(ptr)
#188 InstallFlash
#189 IntlPercentEncodeNormalize
190 stdcall IsAsyncMoniker(ptr)
#191 IsDWORDProperty
#192 IsIntranetAvailable
#193 stub IsJITInProgress
194 stdcall IsLoggingEnabledA(str)
195 stdcall IsLoggingEnabledW(wstr)
#196 IsStringProperty
197 stdcall IsValidURL(ptr wstr long)
198 stdcall MkParseDisplayNameEx(ptr wstr ptr ptr)
199 stdcall ObtainUserAgentString(long str ptr)
#200 stub PrivateCoInstall
#201 QueryAssociations
#202 QueryClsidAssociation
203 stdcall RegisterBindStatusCallback(ptr ptr ptr long)
204 stdcall RegisterFormatEnumerator(ptr ptr long)
#205 stub RegisterMediaTypeClass
206 stdcall RegisterMediaTypes(long ptr ptr)
207 stdcall ReleaseBindInfo(ptr)
#208 ResetUrlmonLanguageData
209 stdcall RevokeBindStatusCallback(ptr ptr)
210 stdcall RevokeFormatEnumerator(ptr ptr)
#211 stub SetSoftwareUpdateAdvertisementState
#212 ShouldDisplayPunycodeForUri
#213 ShouldShowIntranetWarningSecband
#214 ShowTrustAlertDialog
215 stdcall -stub URLDownloadA(long ptr long ptr long)
216 stdcall URLDownloadToCacheFileA(ptr str str long long ptr)
217 stdcall URLDownloadToCacheFileW(ptr wstr wstr long long ptr)
218 stdcall URLDownloadToFileA(ptr str str long ptr)
219 stdcall URLDownloadToFileW(ptr wstr wstr long ptr)
#220 stub URLDownloadW
221 stdcall URLOpenBlockingStreamA(ptr str ptr long ptr)
222 stdcall URLOpenBlockingStreamW(ptr wstr ptr long ptr)
#223 stub URLOpenPullStreamA
#224 stub URLOpenPullStreamW
225 stdcall URLOpenStreamA(ptr str long ptr)
226 stdcall URLOpenStreamW(ptr wstr long ptr)
#227 stub UrlMkBuildVersion
228 stdcall UrlMkGetSessionOption(long ptr long ptr long)
229 stdcall UrlMkSetSessionOption(long ptr long long)
#230 stub WriteHitLogging
#231 stub ZonesReInit
#304 IECompatLogEventWithUrl
#305 IECompatLogPopupMgr
#306 IECompatLogMkAndViewSource
#307 IECompatLogMimeHandling
#308 IECompatLogControlBlock
#309 IECompatLogObjCache
#310 IECompatLogWindowRestriction
#311 IECompatLogBinaryBhvr
#312 IECompatLogIDNNavigation
#313 IECompatLogSSLNavBlock
#314 IECompatLogRedirectUrl
#315 IECompatLogScriptUrl
#316 IECompatLogAntiphishingUrl
#318 IECompatLogZoneElevation3
#319 IECompatLogZoneElevation4
#320 IECompatLogSubframeNavigate
#321 IECompatLogFileDownloadWithSrcUrl
#322 IECompatLogCSSFix
#323 IECompatLogUIPIBlockedExtension
#324 ResetWarnOnIntranetFlag
#325 PSCreateMemoryPropertyStore
#326 PSCreatePropertyStoreFromObject
#327 PSCreateAdapterFromPropertyStore
#328 VariantCompare
#329 VariantToGUID
#330 VariantToStringWithDefault
#331 InitPropVariantFromBuffer
#332 InitPropVariantFromCLSID
#333 InitPropVariantFromFileTime
#334 InitPropVariantFromString
#335 InitVariantFromBuffer
#336 InitVariantFromStrRet
#337 PropVariantGetElementCount
#338 PropVariantToBoolean
#339 PropVariantToInt32
#340 PropVariantToUInt32
#341 PropVariantToInt64
#342 PropVariantToUInt64
#343 PropVariantToBooleanWithDefault
#344 PropVariantToInt32WithDefault
#345 PropVariantToUInt32WithDefault
#346 PropVariantToInt64WithDefault
#347 PropVariantToUInt64WithDefault
#348 PropVariantToBuffer
#349 PropVariantToFileTime
#350 PropVariantToGUID
#351 PropVariantToStringAlloc
#352 VariantToBoolean
#353 VariantToBooleanWithDefault
#354 VariantToPropVariant
#355 PropVariantToVariant
#360 ClearVariantArray
#361 InitVariantFromFileTime
#362 InitVariantFromGUIDAsString
#363 InitVariantFromResource
#364 PropVariantToStringWithDefault
#365 PropVariantToString
#366 VariantToBuffer
#367 VariantToDouble
#368 VariantToDoubleArray
#369 VariantToDoubleArrayAlloc
#370 VariantToFileTime
#371 VariantToInt16
#372 VariantToUInt16
#373 VariantToInt32
#374 VariantToUInt32
#375 VariantToInt64
#376 VariantToUInt64
#377 VariantToInt16Array
#378 VariantToUInt16Array
#379 VariantToInt32Array
#380 VariantToUInt32Array
#381 VariantToInt64Array
#382 VariantToUInt64Array
#383 VariantToInt16ArrayAlloc
#384 VariantToUInt16ArrayAlloc
#385 VariantToInt32ArrayAlloc
#386 VariantToUInt32ArrayAlloc
#387 VariantToUInt32WithDefault
#388 VariantToInt64ArrayAlloc
#389 VariantToUInt64ArrayAlloc
#390 VariantToString
#391 VariantToStringArray
#392 VariantToStringArrayAlloc
#393 VariantToStringAlloc
#394 VariantToStrRet
#395 PropVariantToBSTR
#396 PropVariantChangeType
#400 InitCustomerFeedback
#401 DeinitCustomerFeedback
#403 SqmAddExtensionClsid
#404 SqmOptedIn
#406 ScheduleSqmTasksInIdleThread
#407 IESqmGetSession
#408 LogSqmDWord
#409 LogSqmBool
#410 LogSqmBits
#411 LogSqmSetString
#412 LogSqmIfMax
#413 LogSqmIfMin
#414 LogSqmIncrement
#415 LogSqmAddToAverage
#416 LogSqmAddToStreamDWord
#417 LogSqmAddToStreamString
#420 FindDomainOrHostFromUri
#421 GetIESqmMutex
#422 ReleaseIESqmMutex
#423 LogSqmUXCommandOffsetInternal
#430 IsSQMOptionEnabled
#431 SetSQMOption
#432 GetSQMUrl
#433 LCIELowerConnLimit
#434 IECompatLogApplicationProtocolDialog
#435 IECompatLogNavigationRestricted
#436 IECompatLogMimeSniffUnsafe
#437 IECompatLogMimeSniffImageNotUpgraded
#438 IECompatLogProxyContentManaged
#439 IECompatLogNoCompression
#440 LCIEGetEffectiveConnLimit
#441 UrlmonCreateInstance
#442 CreateBrowserEmulationFilter
#443 CreateReadOnlyBrowserEmulationFilter
#444 MapUriToBrowserEmulationState
#445 MapBrowserEmulationModeToUserAgent
#446 CoInternetGetBrowserProfile
#447 CoInternetSetBrowserProfile
#448 DeleteBrowserEmulationUserData
#449 CoInternetGetBrowserEmulationMode
#450 CoInternetSetBrowserEmulationMode
#451 CleanBrowserEmulationCache
#452 ClearSessionBasedEmulationData
#453 GetSecMgrCacheSeed
#454 SeedSecMgrCache
#455 FlushUrlmonZonesCache
#456 Urlmon_CleanIETldListCache
#457 CreateIETldListManager
#458 IsClientCertSuppliedInProcess
