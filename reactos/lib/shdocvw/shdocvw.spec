# ordinal exports
101 stub -noname IEWinMain
102 stub -noname CreateShortcutInDirA
103 stub -noname CreateShortcutInDirW
104 stub -noname WhichPlatformFORWARD
105 stub -noname CreateShortcutInDirEx
106 stub HlinkFindFrame
107 stub SetShellOfflineState
108 stub AddUrlToFavorites
110 stdcall -noname WinList_Init()
111 stub -noname WinList_Terminate
115 stub -noname CreateFromDesktop
116 stub -noname DDECreatePostNotify
117 stub -noname DDEHandleViewFolderNotify
@ stub IEAboutBox
118 stdcall -noname ShellDDEInit(long)
119 stub -noname SHCreateDesktop
120 stub -noname SHDesktopMessageLoop
121 stub -noname StopWatchModeFORWARD
122 stub -noname StopWatchFlushFORWARD
123 stub -noname StopWatchFORWARD
125 stdcall -noname RunInstallUninstallStubs()
130 stub -noname RunInstallUninstallStubs2
131 stub -noname SHCreateSplashScreen
135 stub -noname IsFileUrl
136 stub -noname IsFileUrlW
137 stub -noname PathIsFilePath
138 stub -noname URLSubLoadString
139 stub -noname OpenPidlOrderStream
140 stub -noname DragDrop
141 stub -noname IEInvalidateImageList
142 stub -noname IEMapPIDLToSystemImageListIndex
143 stub -noname ILIsWeb
145 stub -noname IEGetAttributesOf
146 stub -noname IEBindToObject
147 stub -noname IEGetNameAndFlags
148 stub -noname IEGetDisplayName
149 stub -noname IEBindToObjectEx
150 stub -noname _GetStdLocation
151 stub -noname URLSubRegQueryA
152 stub -noname CShellUIHelper_CreateInstance2
153 stub -noname IsURLChild
158 stub -noname SHRestricted2A
159 stub -noname SHRestricted2W
160 stub -noname SHIsRestricted2W
161 stub @ # CSearchAssistantOC::OnDraw
162 stub -noname CDDEAuto_Navigate
163 stub SHAddSubscribeFavorite
164 stub -noname ResetProfileSharing
165 stub -noname URLSubstitution
167 stub -noname IsIEDefaultBrowser
169 stub -noname ParseURLFromOutsideSourceA
170 stub -noname ParseURLFromOutsideSourceW
171 stub -noname _DeletePidlDPA
172 stub -noname IURLQualify
173 stub -noname SHIsRestricted
174 stub -noname SHIsGlobalOffline
175 stub -noname DetectAndFixAssociations
176 stub -noname EnsureWebViewRegSettings
177 stub -noname WinList_NotifyNewLocation
178 stub -noname WinList_FindFolderWindow
179 stub -noname WinList_GetShellWindows
180 stub -noname WinList_RegisterPending
181 stub -noname WinList_Revoke
183 stub -noname SHMapNbspToSp
185 stub -noname FireEvent_Quit
187 stub -noname SHDGetPageLocation
188 stub -noname SHIEErrorMsgBox
189 stub -noname IEGetDisplayName
190 stub -noname SHRunIndirectRegClientCommandForward
191 stub -noname SHIsRegisteredClient
192 stub -noname SHGetHistoryPIDL
194 stub -noname IECleanUpAutomationObject
195 stub -noname IEOnFirstBrowserCreation
196 stub -noname IEDDE_WindowDestroyed
197 stub -noname IEDDE_NewWindow
198 stub -noname IsErrorUrl
199 stub @
200 stub -noname SHGetViewStream
203 stub -noname NavToUrlUsingIEA
204 stub -noname NavToUrlUsingIEW
208 stub -noname SearchForElementInHead
209 stub -noname JITCoCreateInstance
210 stub -noname UrlHitsNetW
211 stub -noname ClearAutoSuggestForForms
212 stub -noname GetLinkInfo
213 stub -noname UseCustomInternetSearch
214 stub -noname GetSearchAssistantUrlW
215 stub -noname GetSearchAssistantUrlA
216 stub -noname GetDefaultInternetSearchUrlW
217 stub -noname GetDefaultInternetSearchUrlA
218 stub -noname IEParseDisplayNameWithBCW
219 stub -noname IEILIsEqual
220 stub @
221 stub -noname IECreateFromPathCPWithBCA
222 stub -noname IECreateFromPathCPWithBCW
223 stub -noname ResetWebSettings
224 stub -noname IsResetWebSettingsRequired
225 stub -noname PrepareURLForDisplayUTF8W
226 stub -noname IEIsLinkSafe
227 stub -noname SHUseClassicToolbarGlyphs
228 stub -noname SafeOpenPromptForShellExec
229 stub -noname SafeOpenPromptForPackager

@ stdcall -private DllCanUnloadNow() SHDOCVW_DllCanUnloadNow
@ stdcall -private DllGetClassObject(long long ptr) SHDOCVW_DllGetClassObject
@ stdcall DllGetVersion(ptr) SHDOCVW_DllGetVersion
@ stdcall DllInstall(long wstr) SHDOCVW_DllInstall
@ stdcall -private DllRegisterServer() SHDOCVW_DllRegisterServer
@ stdcall -private DllUnregisterServer() SHDOCVW_DllUnregisterServer
@ stub DllRegisterWindowClasses
@ stub DoAddToFavDlg
@ stub DoAddToFavDlgW
@ stub DoFileDownload
@ stub DoFileDownloadEx
@ stub DoOrganizeFavDlg
@ stub DoOrganizeFavDlgW
@ stub DoPrivacyDlg
@ stub HlinkFrameNavigate
@ stub HlinkFrameNavigateNHL
@ stub ImportPrivacySettings
@ stub InstallReg_RunDLL
@ stub IEWriteErrorLog
@ stub OpenURL
@ stub SHGetIDispatchForFolder
@ stdcall SetQueryNetSessionCount(long)
@ stub SoftwareUpdateMessageBox
@ stub URLQualifyA
@ stub URLQualifyW
