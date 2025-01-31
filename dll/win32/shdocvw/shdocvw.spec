101 stdcall -noname IEWinMain(str long) # FIXME: Inspect
102 stub -noname CreateShortcutInDirA # FIXME: Inspect
103 stub -noname CreateShortcutInDirW # FIXME: Inspect
104 stdcall -noname WhichPlatformFORWARD()
105 stub -noname CreateShortcutInDirEx # FIXME: Inspect
106 stub AddUrlToFavorites
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllGetVersion(ptr)
110 stdcall -noname WinList_Init()
111 stdcall -noname WinList_Terminate()
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stub DllRegisterWindowClasses
115 stub -noname CreateFromDesktop
116 stub -noname DDECreatePostNotify
117 stub -noname DDEHandleViewFolderNotify
118 stdcall -noname ShellDDEInit(long)
119 stub -noname SHCreateDesktopDEPRECATED
120 stub -noname SHDesktopMessageLoopDEPRECATED
121 stdcall -noname StopWatchModeFORWARD()
122 stdcall -noname StopWatchFlushFORWARD()
123 stdcall -noname StopWatchWFORWARD(long str long long long)
@ stdcall -private DllUnregisterServer()
125 stdcall -noname RunInstallUninstallStubs()
126 stub DoAddToFavDlg
127 stub DoAddToFavDlgW
128 stdcall DoFileDownload(wstr)
129 stub DoFileDownloadEx
130 stdcall -noname RunInstallUninstallStubs2(long)
131 stub -noname SHCreateSplashScreen
132 stdcall DoOrganizeFavDlg(long str)
133 stdcall DoOrganizeFavDlgW(long wstr)
134 stub DoPrivacyDlg
135 stub -noname IsFileUrl
136 stub -noname IsFileUrlW
137 stub -noname PathIsFilePath
138 stub -noname URLSubLoadString
139 stub -noname OpenPidlOrderStream
140 stub -noname DragDrop
141 stub -noname IEInvalidateImageList # FIXME: Inspect
142 stub -noname IEMapPIDLToSystemImageListIndex # FIXME: Inspect
143 stub -noname ILIsWeb
144 stub HlinkFindFrame
145 stub -noname IEGetAttributesOf
146 stub -noname IEBindToObject
147 stub -noname IEGetNameAndFlags
148 stub -noname IEGetDisplayName
149 stub -noname IEBindToObjectEx
150 stdcall -stub -noname _GetStdLocation(ptr long long)
151 stdcall -noname URLSubRegQueryA(str str long ptr long long) # FIXME: Inspect
152 stub -noname CShellUIHelper_CreateInstance2 # FIXME: Inspect
153 stub -noname IsURLChild
154 stub HlinkFrameNavigate
155 stub HlinkFrameNavigateNHL
156 stub IEWriteErrorLog
157 stdcall ImportPrivacySettings(wstr ptr ptr)
158 stdcall -noname SHRestricted2A(long str long) # FIXME: Inspect
159 stdcall -noname SHRestricted2W(long wstr long) # FIXME: Inspect
160 stub -noname SHIsRestricted2W # FIXME: Inspect
161 stub -noname shdocvw_161 # CSearchAssistantOC::OnDraw # FIXME: Inspect
162 stub -noname CDDEAuto_Navigate # FIXME: Inspect
163 stub SHAddSubscribeFavorite
164 stdcall -noname ResetProfileSharing(long) # FIXME: Inspect
165 stub -noname URLSubstitution # FIXME: Inspect
166 stdcall OpenURL(long long str long) ieframe.OpenURL # FIXME: Inspect
167 stub -noname IsIEDefaultBrowser # FIXME: Inspect
168 stub SHGetIDispatchForFolder
169 stdcall -noname ParseURLFromOutsideSourceA(str ptr ptr ptr)
170 stdcall -noname ParseURLFromOutsideSourceW(wstr ptr ptr ptr)
171 stub -noname _DeletePidlDPA
172 stub -noname IURLQualify
173 stub -noname SHIsRestricted
174 stub -noname SHIsGlobalOffline
175 stub -noname DetectAndFixAssociations
176 stub -noname EnsureWebViewRegSettings
177 stdcall -noname WinList_NotifyNewLocation(ptr long ptr)
178 stdcall -noname WinList_FindFolderWindow(ptr long ptr ptr)
179 stdcall -noname WinList_GetShellWindows(long)
180 stdcall -noname WinList_RegisterPending(long ptr long ptr)
181 stdcall -noname WinList_Revoke(long)
182 stdcall SetQueryNetSessionCount(long)
183 stub -noname SHMapNbspToSp
184 stub SetShellOfflineState
185 stub -noname FireEvent_Quit
186 stub SoftwareUpdateMessageBox
187 stub -noname SHDGetPageLocation
188 stub -noname SHIEErrorMsgBox
189 stub -noname IEGetDisplayName_2 # FIXME: Inspect
190 stub -noname SHRunIndirectRegClientCommandForward
191 stub -noname SHIsRegisteredClient
192 stub -noname SHGetHistoryPIDL
193 stub URLQualifyA
194 stub -noname IECleanUpAutomationObject
195 stub -noname IEOnFirstBrowserCreation
196 stub -noname IEDDE_WindowDestroyed
197 stub -noname IEDDE_NewWindow
198 stub -noname IsErrorUrl
199 stub URLQualifyW
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
218 stdcall -noname IEParseDisplayNameWithBCW(long wstr ptr ptr)
219 stdcall -noname IEILIsEqual(ptr ptr long)
221 stub -noname IECreateFromPathCPWithBCA
222 stub -noname IECreateFromPathCPWithBCW
223 stub -noname ResetWebSettings # FIXME: Inspect
224 stub -noname IsResetWebSettingsRequired # FIXME: Inspect
225 stub -noname PrepareURLForDisplayUTF8W
226 stub -noname IEIsLinkSafe
227 stub -noname SHUseClassicToolbarGlyphs
228 stub -noname SafeOpenPromptForShellExec
229 stub -noname SafeOpenPromptForPackager
230 stub -noname ShowUrlInNewBrowserInstance
231 stub -noname RecordExtensionCreation
232 stub -noname GetExtensionRecords
233 stub -noname GetExtensionRecordBlockReason
234 stub -noname ClearExtensionRecordsBlockReason
