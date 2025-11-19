1 stub -noname SHCreateReadOnlySharedMemoryStream
@ stdcall CommandLineToArgvW(wstr ptr)
@ stub CreateRandomAccessStreamOnFile
@ stdcall CreateRandomAccessStreamOverStream(ptr long ptr ptr)
@ stub CreateStreamOverRandomAccessStream
@ stdcall -private DllCanUnloadNow()
@ stub DllGetActivationFactory
@ stub DllGetClassObject
@ stdcall GetCurrentProcessExplicitAppUserModelID(ptr)
@ stdcall GetDpiForMonitor(long long ptr ptr)
@ stub GetDpiForShellUIComponent
@ stdcall GetFeatureEnabledState(long long)
# @ stub GetFeatureVariant
@ stdcall GetProcessDpiAwareness(long ptr)
@ stdcall GetProcessReference(ptr)
@ stdcall GetScaleFactorForDevice(long)
@ stdcall GetScaleFactorForMonitor(long ptr)
@ stub IStream_Copy
@ stdcall IStream_Read(ptr ptr long) _IStream_Read
@ stub IStream_ReadStr
@ stdcall IStream_Reset(ptr)
@ stdcall IStream_Size(ptr ptr)
@ stdcall IStream_Write(ptr ptr long) _IStream_Write
@ stub IStream_WriteStr
@ stdcall IUnknown_AtomicRelease(ptr)
@ stdcall IUnknown_GetSite(ptr ptr ptr)
@ stdcall IUnknown_QueryService(ptr ptr ptr ptr)
@ stdcall IUnknown_Set(ptr ptr)
@ stdcall IUnknown_SetSite(ptr ptr)
@ stdcall IsOS(long)
# @ stub IsProcessInIsolatedContainer
# @ stub IsProcessInWDAGContainer
# @ stub RecordFeatureError
# @ stub RecordFeatureUsage
@ stdcall RegisterScaleChangeEvent(ptr ptr)
@ stdcall RegisterScaleChangeNotifications(long ptr long ptr)
@ stub RevokeScaleChangeNotifications
@ stdcall SHAnsiToAnsi(str ptr long)
@ stdcall SHAnsiToUnicode(str ptr long)
@ stdcall SHCopyKeyA(long str long long)
@ stdcall SHCopyKeyW(long wstr long long)
@ stdcall SHCreateMemStream(ptr long)
@ stdcall SHCreateStreamOnFileA(str long ptr)
@ stdcall SHCreateStreamOnFileEx(wstr long long long ptr ptr)
@ stdcall SHCreateStreamOnFileW(wstr long ptr)
@ stdcall SHCreateThread(ptr ptr long ptr)
@ stdcall SHCreateThreadRef(ptr ptr)
@ stub SHCreateThreadWithHandle
@ stdcall SHDeleteEmptyKeyA(long str)
@ stdcall SHDeleteEmptyKeyW(long wstr)
@ stdcall SHDeleteKeyA(long str)
@ stdcall SHDeleteKeyW(long wstr)
@ stdcall SHDeleteValueA(long str str)
@ stdcall SHDeleteValueW(long wstr wstr)
@ stdcall SHEnumKeyExA(long long str ptr)
@ stdcall SHEnumKeyExW(long long wstr ptr)
@ stdcall SHEnumValueA(long long str ptr ptr ptr ptr)
@ stdcall SHEnumValueW(long long wstr ptr ptr ptr ptr)
@ stdcall SHGetThreadRef(ptr)
@ stdcall SHGetValueA(long str str ptr ptr ptr)
@ stdcall SHGetValueW(long wstr wstr ptr ptr ptr)
@ stdcall SHOpenRegStream2A(long str str long)
@ stdcall SHOpenRegStream2W(long wstr wstr long)
@ stdcall SHOpenRegStreamA(long str str long)
@ stdcall SHOpenRegStreamW(long wstr wstr long)
@ stdcall SHQueryInfoKeyA(long ptr ptr ptr ptr)
@ stdcall SHQueryInfoKeyW(long ptr ptr ptr ptr)
@ stdcall SHQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall SHQueryValueExW(long wstr ptr ptr ptr ptr)
@ stdcall SHRegDuplicateHKey(long)
@ stdcall SHRegGetIntW(ptr wstr long)
@ stdcall SHRegGetPathA(long str str ptr long)
@ stdcall SHRegGetPathW(long wstr wstr ptr long)
@ stdcall SHRegGetValueA(long str str long ptr ptr ptr) advapi32.RegGetValueA
@ stdcall SHRegGetValueW(long wstr wstr long ptr ptr ptr) advapi32.RegGetValueW
@ stdcall SHRegSetPathA(long str str str long)
@ stdcall SHRegSetPathW(long wstr wstr wstr long)
@ stdcall SHReleaseThreadRef()
@ stdcall SHSetThreadRef(ptr)
@ stdcall SHSetValueA(long str str long ptr long)
@ stdcall SHSetValueW(long wstr wstr long ptr long)
@ stdcall SHStrDupA(str ptr)
@ stdcall SHStrDupW(wstr ptr)
# @ stub SHTaskPoolAllowThreadReuse
# @ stub SHTaskPoolDoNotWaitForMoreTasks
# @ stub SHTaskPoolGetCurrentThreadLifetime
# @ stub SHTaskPoolGetUniqueContext
# @ stub SHTaskPoolQueueTask
# @ stub SHTaskPoolSetThreadReuseAllowed
@ stdcall SHUnicodeToAnsi(wstr ptr ptr)
@ stdcall SHUnicodeToUnicode(wstr ptr long)
@ stdcall SetCurrentProcessExplicitAppUserModelID(wstr)
@ stdcall SetProcessDpiAwareness(long)
@ stdcall SetProcessReference(ptr)
@ stdcall SubscribeFeatureStateChangeNotification(ptr ptr ptr)
@ stub UnregisterScaleChangeEvent
# @ stub UnsubscribeFeatureStateChangeNotification

100 stub -noname SHManagedCreateStreamOnFile
101 stub -noname SHManagedCreateFile
102 stub -noname SHIsEmptyStream
103 stub -noname MapWin32ErrorToSTG
104 stub -noname ModeToCreateFileFlags
105 stub -noname CreateTempStreamName
106 stub -noname StreamCopyTo
107 stub -noname IStream_ReadStrLong
108 stub -noname IStream_WriteStrLong
109 stub -noname SHCreateStreamOnModuleResourceW
110 stub -noname SHCreateStreamOnDllResourceW
111 stub -noname CreateWritableSharedMemoryStream
115 stub -noname CreateRandomAccessStreamOnFileWithOptions
116 stub -noname RandomAccessStreamCopyAsync
117 stub -noname CreateRandomAccessStreamOnPlaceholderFile
120 stub -noname SHRegGetCLSIDKey
121 stub -noname SHRegSetValue
122 stub SHRegGetValueFromHKCUHKLM
123 stub -noname SHRegGetBoolValueFromHKCUHKLM
124 stub -noname SHGetValueGoodBootA
125 stub -noname SHGetValueGoodBootW
126 stub -noname SHLoadRegUIStringW
127 stub -noname QuerySourceCreateFromKeyEx
130 stub -noname SHGlobalCounterGetValue
131 stub -noname SHGlobalCounterIncrement
132 stub -noname SHGlobalCounterDecrement
133 stub -noname SHGlobalCounterSetValue
140 stub -noname IUnknown_ProfferService
141 stub -noname IUnknown_RemoveBackReferences
142 stub -noname IUnknown_GetClassID
143 stub -noname StrRetToStrW
144 stub -noname StrRetToBSTR
145 stub -noname StrRetToBufW
150 stub -noname SHAnsiToUnicodeCP
151 stub -noname SHUnicodeToAnsiCP
152 stub -noname SHUnicodeToAnsiCPAlloc
153 stub -noname SHAnsiToUnicodeCPAlloc
160 stub -noname SHWaitForSendMessageThread
161 stub -noname SHWaitForThreadWithWakeMask
162 stub -noname SHQueueUserWorkItem
170 stub -noname PathIsNetworkPathW
171 stub -noname PathIsNetworkPathA
172 stub -noname PathBuildRootW
173 stub -noname PathBuildRootA
174 stub -noname DriveType
175 stub -noname IsNetDrive
181 stub -noname SHMapHandle
182 stub -noname SHAllocShared
183 stub -noname SHLockSharedEx
184 stub -noname SHLockShared
185 stub -noname SHGetSizeShared
186 stub -noname SHUnlockShared
187 stub -noname SHFreeShared
188 stub -noname SHCreateWorkerWindowW
189 stub -noname SHCreateOplockProvider
190 stub -noname SHWindowsPolicy
191 stub -noname SHWindowsPolicyGetValue
192 stub -noname IsAppCompatModeEnabled
193 stub -noname SHGetObjectCompatFlags
200 stub -noname GUIDFromStringW
220 stub -noname GetPhysicalDpiForDevice
222 stub -noname ScaleRelativePixelsForDevice
223 stub -noname PhysicalRectFromScaledRect
224 stub -noname ScaleAndMapRelativeRectForDevice
225 stub -noname CanOverrideScaleFactor
226 stub -noname GetScalingOverride
227 stub -noname RegisterScaleChangeSink
228 stub -noname SetDesignModeScaleFactor
229 stub -noname GetProposedScaleFactorForWindow
230 stub -noname SHSetWindowSubclass
231 stub -noname SHGetWindowSubclassData
232 stub -noname SHRemoveWindowSubclass
233 stub -noname SHDefSubclassProc
234 stub -noname SHRegisterClassW
240 stub -noname RegisterScaleChangeSinkForWindow
241 stub -noname ScaleAndMapRelativeRect
242 stub -noname RelativeRectFromPhysicalRectWithScale
244 stub -noname GetScaleFactorForWindow
245 stub -noname RegisterScaleChangeNotificationsForWindow
246 stub -noname RevokeScaleChangeNotificationsForWindow
247 stub -noname GetOverrideScaleFactorForWindow
248 stub -noname GetSystemScaleFactorForWindow
249 stub -noname UpdateScalingInfoCache
250 stub -noname RegisterCurrentWindowChangeListener
251 stub -noname RegisterWindowMonitorChangeListener
252 stub -noname UnregisterCurrentWindowChangeListener
253 stub -noname UnregisterWindowMonitorChangeListener
254 stub -noname AddCurrentWindowCandidate
255 stub -noname GetCurrentWindow
260 stub -noname PhysicalRectFromRelativeRectWithScales
261 stub -noname RelativeRectFromPhysicalRectWithScales
270 stub -noname SHCreateMemoryStreamOnSharedBuffer
280 stub -noname _CreateDirectoryHelper
281 stub -noname Win32CreateDirectory
282 stub -noname SuspendSHNotify
283 stub -noname ResumeSHNotify
284 stub -noname IsNotifySuspended
290 stub -noname SHCreateDirectoryExW
291 stub -noname SHCreateDirectoryExA
292 stub -noname SHCreateDirectory
