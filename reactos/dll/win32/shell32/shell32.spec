# Functions exported by the WinXP SP3 shell32.dll (6.0.2900.5686)
2   stdcall SHChangeNotifyRegister(long long long long long ptr)
3   stdcall SHDefExtractIconA(str long long ptr ptr long)
4   stdcall SHChangeNotifyDeregister(long)
5   stdcall -noname SHChangeNotifyUpdateEntryList(long long long long)
6   stdcall SHDefExtractIconW(wstr long long ptr ptr long)
7   stub -noname Shell_7
8   stub -noname Shell_8
9   stub PifMgr_OpenProperties
10  stub PifMgr_GetProperties
11  stub PifMgr_SetProperties
12  stub -noname Shell_12
13  stub PifMgr_CloseProperties
14  stub SHStartNetConnectionDialogW
15  stdcall -noname ILGetDisplayName(ptr ptr)
16  stdcall ILFindLastID(ptr)
17  stdcall ILRemoveLastID(ptr)
18  stdcall ILClone(ptr)
19  stdcall ILCloneFirst(ptr)
20  stdcall -noname ILGlobalClone(ptr)
21  stdcall ILIsEqual(ptr ptr)
22  stub DAD_DragEnterEx2
23  stdcall ILIsParent(ptr ptr long)
24  stdcall ILFindChild(ptr ptr)
25  stdcall ILCombine(ptr ptr)
26  stdcall ILLoadFromStream(ptr ptr)
27  stdcall ILSaveToStream(ptr ptr)
28  stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
29  stdcall -noname PathIsRoot(ptr) PathIsRootAW
30  stdcall -noname PathBuildRoot(ptr long) PathBuildRootAW
31  stdcall -noname PathFindExtension(wstr) PathFindExtensionW
32  stdcall -noname PathAddBackslash(wstr) PathAddBackslashW
33  stdcall -noname PathRemoveBlanks(wstr) PathRemoveBlanksW
34  stdcall -noname PathFindFileName(wstr) PathFindFileNameW
35  stdcall -noname PathRemoveFileSpec(ptr) PathRemoveFileSpecAW # Fixme
36  stdcall -noname PathAppend(ptr ptr) PathAppendAW # Fixme
37  stdcall -noname PathCombine(wstr wstr wstr) PathCombineW
38  stdcall -noname PathStripPath(wstr) PathStripPathW
39  stdcall -noname PathIsUNC(wstr) PathIsUNCW
40  stdcall -noname PathIsRelative(wstr) PathIsRelativeW
41  stdcall IsLFNDriveA(str)
42  stdcall IsLFNDriveW(wstr)
43  stdcall PathIsExe(ptr) PathIsExeAW
44  stdcall OpenAs_RunDLL(long long str long) OpenAs_RunDLLA
45  stdcall -noname PathFileExists(ptr) PathFileExistsAW # Fixme
46  stdcall -noname PathMatchSpec(wstr wstr) PathMatchSpecW
47  stdcall PathMakeUniqueName(ptr long ptr ptr ptr) PathMakeUniqueNameAW
48  stdcall -noname PathSetDlgItemPath(long long wstr) PathSetDlgItemPathW
49  stdcall PathQualify(ptr) PathQualifyAW
50  stdcall -noname PathStripToRoot(wstr) PathStripToRootW
51  stdcall PathResolve(str long long) PathResolveAW
52  stdcall -noname PathGetArgs(wstr) PathGetArgsW
53  stub -noname Shell_53
54  stub -noname LogoffWindowsDialog
55  stdcall -noname PathQuoteSpaces(wstr) PathQuoteSpacesW
56  stdcall -noname PathUnquoteSpaces(wstr) PathUnquoteSpacesW
57  stdcall -noname PathGetDriveNumber(wstr) PathGetDriveNumberW
58  stdcall -noname ParseField(str long ptr long) ParseFieldAW # Fixme
59  stdcall RestartDialog(long wstr long)
60  stdcall -noname ExitWindowsDialog(long) # Fixme
61  stdcall -noname RunFileDlg(long long long str str long) RunFileDlg # Fixme
62  stdcall PickIconDlg(long long long long)
63  stdcall GetFileNameFromBrowse(long long long long str str str)
64  stdcall DriveType(long)
65  stdcall -noname InvalidateDriveType(long) # Fixme
66  stdcall IsNetDrive(long)
67  stdcall Shell_MergeMenus(long long long long long long)
68  stdcall SHGetSetSettings(ptr long long)
69  stub -noname SHGetNetResource # Fixme
70  stdcall -noname SHCreateDefClassObject(long long long long long)
71  stdcall -noname Shell_GetImageLists(ptr ptr)
72  stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
73  stdcall SHShellFolderView_Message(long long long)
74  stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
75  stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
76  stub DragQueryInfo # Fixme
77  stdcall SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
78  stdcall -noname OleStrToStrN(str long wstr long) OleStrToStrNAW # Fixme
79  stdcall -noname StrToOleStrN(wstr long str long) StrToOleStrNAW # Fixme
80  stub SHOpenPropSheetW
81  stdcall OpenAs_RunDLLA(long long str long)
82  stub -noname Shell_82
83  stdcall -noname CIDLData_CreateFromIDArray(ptr long ptr ptr)
84  stub SHIsBadInterfacePtr # Fixme
85  stdcall OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
86  stdcall -noname SHRegisterDragDrop(long ptr)
87  stdcall -noname SHRevokeDragDrop(long)
88  stdcall SHDoDragDrop(long ptr ptr long ptr)
89  stdcall SHCloneSpecialIDList(long long long)
90  stdcall SHFindFiles(ptr ptr)
91  stub SHFindComputer # Fixme
92  stdcall PathGetShortPath(ptr) PathGetShortPathAW
93  stdcall -noname Win32CreateDirectory(wstr ptr) Win32CreateDirectoryW
94  stdcall -noname Win32RemoveDirectory(wstr) Win32RemoveDirectoryW
95  stdcall -noname SHLogILFromFSIL(ptr)
96  stdcall -noname StrRetToStrN(ptr long ptr ptr) StrRetToStrNAW # Fixme
97  stdcall -noname SHWaitForFileToOpen(long long long)
98  stdcall SHGetRealIDL(ptr ptr ptr)
99  stdcall -noname SetAppStartingCursor(long long) # Fixme
100 stdcall SHRestricted(long)
101 stdcall OpenAs_RunDLLW(long long wstr long)
102 stdcall SHCoCreateInstance(wstr ptr long ptr ptr)
103 stdcall SignalFileOpen(long)
104 stub Activate_RunDLL
105 stub AppCompat_RunDLLW
106 stdcall CheckEscapesA(str long)
107 stdcall CheckEscapesW(wstr long)
108 stdcall CommandLineToArgvW(wstr ptr)
109 stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
110 stdcall Control_FillCache_RunDLLA(long long long long)
111 stdcall Control_FillCache_RunDLLW(long long long long)
112 stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
113 stdcall Control_RunDLLA(ptr ptr str long)
114 stub Control_RunDLLAsUserW
115 stdcall Control_RunDLLW(ptr ptr wstr long)
116 stdcall DllCanUnloadNow()
117 stdcall DllGetClassObject(ptr ptr ptr)
118 stdcall DllGetVersion(ptr)
119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
120 stdcall DllInstall(long wstr)
121 stdcall SHFlushClipboard()
122 stdcall -noname RunDLL_CallEntry16(long long long str long) # Fixme #name wrong?
123 stdcall -noname SHFreeUnusedLibraries()
124 stdcall DllRegisterServer()
125 stdcall DllUnregisterServer()
126 stdcall -noname SHOutOfMemoryMessageBox(long long long) # Fixme
127 stdcall -noname SHWinHelp(long long long long)
128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
129 stdcall DAD_AutoScroll(long ptr ptr)
130 stdcall -noname DAD_DragEnter(long)
131 stdcall DAD_DragEnterEx(long double)
132 stdcall DAD_DragLeave()
133 stdcall DoEnvironmentSubstA(str str)
134 stdcall DAD_DragMove(double)
135 stdcall DoEnvironmentSubstW(wstr wstr)
136 stdcall DAD_SetDragImage(long long)
137 stdcall DAD_ShowDragImage(long)
138 stdcall DragAcceptFiles(long long)
139 stdcall DragFinish(long)
140 stdcall DragQueryFile(long long ptr long) DragQueryFileA
141 stdcall DragQueryFileA(long long ptr long)
142 stub DragQueryFileAorW
143 stdcall DragQueryFileW(long long ptr long)
144 stdcall DragQueryPoint(long ptr)
145 stdcall -noname PathFindOnPath(wstr wstr) PathFindOnPathW
146 stdcall -noname RLBuildListOfPaths()
147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
148 stdcall SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
149 stdcall SHFind_InitMenuPopup(long long long long)
150 stdcall DuplicateIcon(long long)
151 stdcall SHLoadOLE(long)
152 stdcall ILGetSize(ptr)
153 stdcall ILGetNext(ptr)
154 stdcall ILAppendID(long long long) ILAppend
155 stdcall ILFree(ptr)
156 stdcall -noname ILGlobalFree(ptr)
157 stdcall ILCreateFromPath(ptr) ILCreateFromPathAW
158 stdcall -noname PathGetExtension(wstr long long) SHPathGetExtensionW
159 stdcall -noname PathIsDirectory(wstr) PathIsDirectoryW
160 stub SHNetConnectionDialog # Fixme
161 stdcall SHRunControlPanel(long long)
162 stdcall SHSimpleIDListFromPath(ptr) SHSimpleIDListFromPathAW # Fixme
163 stdcall -noname StrToOleStr(wstr str) StrToOleStrAW # Fixme
164 stdcall Win32DeleteFile(wstr) Win32DeleteFileW
165 stdcall SHCreateDirectory(long ptr)
166 stdcall CallCPLEntry16(long long long long long long)
167 stdcall SHAddFromPropSheetExtArray(long long long)
168 stdcall SHCreatePropSheetExtArray(long wstr long)
169 stdcall SHDestroyPropSheetExtArray(long)
170 stdcall SHReplaceFromPropSheetExtArray(long long long long)
171 stdcall PathCleanupSpec(ptr ptr)
172 stdcall -noname SHCreateLinks(long str ptr long ptr)
173 stdcall SHValidateUNC(long long long)
174 stdcall SHCreateShellFolderViewEx(ptr ptr)
175 stdcall -noname SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathW
176 stdcall SHSetInstanceExplorer(long)
177 stub DAD_SetDragImageFromListView
178 stdcall SHObjectProperties(long long wstr wstr)
179 stdcall SHGetNewLinkInfoA(str str ptr long long)
180 stdcall SHGetNewLinkInfoW(wstr wstr ptr long long)
181 stdcall -noname RegisterShellHook(long long)
182 varargs ShellMessageBoxA(long long str str long)
183 varargs ShellMessageBoxW(long long wstr wstr long)
184 stdcall -noname ArrangeWindows(long long long long long) # Fixme
185 stub -noname SHHandleDiskFull # Fixme
186 stdcall -noname ILGetDisplayNameEx(ptr ptr ptr long)
187 stub -noname ILGetPseudoNameW # Fixme
188 stdcall -noname ShellDDEInit(long)
189 stdcall ILCreateFromPathA(str)
190 stdcall ILCreateFromPathW(wstr)
191 stdcall SHUpdateImageA(str long long long)
192 stdcall SHUpdateImageW(wstr long long long)
193 stdcall SHHandleUpdateImage(ptr)
194 stdcall -noname SHCreatePropSheetExtArrayEx(long wstr long ptr)
195 stdcall SHFree(ptr)
196 stdcall SHAlloc(long)
197 stub -noname SHGlobalDefect
198 stdcall -noname SHAbortInvokeCommand()
199 stdcall ExtractAssociatedIconA(long str ptr)
200 stdcall -noname SHLocalAlloc(long long)
201 stdcall -noname SHLocalFree(ptr)
202 stdcall -noname SHLocalReAlloc(ptr long long)
203 stdcall -noname AddCommasW(long wstr)
204 stdcall -noname ShortSizeFormatW(double)
205 stdcall Printer_LoadIconsW(wstr ptr ptr)
206 stdcall ExtractAssociatedIconExA(long str long long)
207 stdcall ExtractAssociatedIconExW(long wstr long long)
208 stdcall ExtractAssociatedIconW(long wstr ptr)
209 stub -noname Int64ToString # Fixme
210 stub -noname LargeIntegerToString # Fixme
211 stub -noname Printers_GetPidl # Fixme
212 stub -noname Printers_AddPrinterPropPages
213 stdcall -noname Printers_RegisterWindowW(wstr long ptr ptr)
214 stdcall -noname Printers_UnregisterWindow(long long)
215 stdcall -noname SHStartNetConnectionDialog(long str long)
216 stdcall ExtractIconA(long str long)
217 stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExA
218 stdcall ExtractIconExA(str long ptr ptr long)
219 stdcall ExtractIconExW(wstr long ptr ptr long)
220 stub ExtractIconResInfoA
221 stub ExtractIconResInfoW
222 stdcall ExtractIconW(long wstr long)
223 stub ExtractVersionResource16W
224 stub FindExeDlgProc
225 stdcall FindExecutableA(str str ptr)
226 stdcall FindExecutableW(wstr wstr ptr)
227 stdcall FreeIconList(long)
228 stub InternalExtractIconListA
229 stub InternalExtractIconListW
230 stub -noname Shell_230
231 stub -noname Shell_231
232 stub -noname Shell_232
233 stub -noname Shell_233
234 stub -noname Shell_234
235 stub -noname Shell_235
236 stub -noname Shell_236
237 stub -noname Shell_237
238 stdcall Options_RunDLL(ptr ptr str long)
239 stub PathIsSlowW
240 stub PathIsSlowA
241 stub -noname Shell_241
242 stub -noname Shell_242
243 stdcall @(long long) shell32_243 # Fixme !!! Call SetShellWindowEx()
244 stdcall -noname SHInitRestricted(ptr ptr)
245 stub SHTestTokenMembership
246 stub -noname Shell_246
247 stub -noname Shell_247
248 stub -noname Shell_248 # Fixme
249 stdcall -noname PathParseIconLocation(wstr) PathParseIconLocationW
250 stdcall -noname PathRemoveExtension(wstr) PathRemoveExtensionW
251 stdcall -noname PathRemoveArgs(wstr) PathRemoveArgsW
252 stub -noname Shell_252 # Fixme
253 stub -noname Shell_253
254 stub -noname Shell_254
255 stdcall Options_RunDLLA(ptr ptr str long)
256 stdcall SHCreateShellFolderView(ptr ptr) SHELL32_256 # Fixme
257 stub -noname Shell_257 # Fixme
258 stdcall -noname LinkWindow_RegisterClass()
259 stdcall -noname LinkWindow_UnregisterClass()
260 stdcall Options_RunDLLW(ptr ptr wstr long)
261 stub PrintersGetCommand_RunDLL
262 stub PrintersGetCommand_RunDLLA
263 stub PrintersGetCommand_RunDLLW
264 stub RealShellExecuteA
265 stub RealShellExecuteExA
266 stub RealShellExecuteExW
267 stub RealShellExecuteW
268 stub RegenerateUserEnvironment
269 stdcall SHAddToRecentDocs(long ptr)
270 stdcall SHAppBarMessage(long ptr)
271 stdcall SHBindToParent(ptr ptr ptr ptr)
272 stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
273 stdcall SHBrowseForFolderA(ptr)
274 stdcall SHBrowseForFolderW(ptr)
275 stdcall SHChangeNotify(long long ptr ptr)
276 stub SHChangeNotifySuspendResume
277 stdcall SHCreateDirectoryExA(long str ptr)
278 stdcall SHCreateDirectoryExW(long wstr ptr)
279 stub SHCreateLocalServerRunDll
280 stub SHCreateProcessAsUserW
281 stub SHCreateQueryCancelAutoPlayMoniker
282 stdcall SHCreateShellItem(ptr ptr ptr ptr)
283 stdcall SHEmptyRecycleBinA(long str long)
284 stdcall SHEmptyRecycleBinW(long wstr long)
285 stub SHEnableServiceObject
286 stub SHEnumerateUnreadMailAccountsW
287 stub SHExtractIconsW
288 stdcall SHFileOperation(ptr) SHFileOperationA
289 stdcall SHFileOperationA(ptr)
290 stdcall SHFileOperationW(ptr)
291 stdcall SHFormatDrive(long long long long)
292 stdcall SHFreeNameMappings(ptr)
293 stdcall SHGetDataFromIDListA(ptr ptr long ptr long)
294 stdcall SHGetDataFromIDListW(ptr ptr long ptr long)
295 stdcall SHGetDesktopFolder(ptr)
296 stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
297 stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
298 stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
299 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA
300 stdcall SHGetFileInfoA(ptr long ptr long long)
301 stdcall SHGetFileInfoW(ptr long ptr long long)
302 stdcall SHGetFolderLocation(long long long long ptr)
303 stdcall SHGetFolderPathA(long long long long ptr)
304 stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
305 stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
306 stdcall SHGetFolderPathW(long long long long ptr)
307 stdcall SHGetIconOverlayIndexA(str long)
308 stdcall SHGetIconOverlayIndexW(wstr long)
309 stdcall SHGetInstanceExplorer(long)
310 stdcall SHGetMalloc(ptr)
311 stdcall SHGetNewLinkInfo(str str ptr long long) SHGetNewLinkInfoA
312 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA
313 stdcall SHGetPathFromIDListA(ptr ptr)
314 stdcall SHGetPathFromIDListW(ptr ptr)
315 stdcall SHGetSettings(ptr long)
316 stdcall SHGetSpecialFolderLocation(long long ptr)
317 stdcall SHGetSpecialFolderPathA(long ptr long long)
318 stdcall SHGetSpecialFolderPathW(long ptr long long)
319 stub SHGetUnreadMailCountW
320 stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
321 stdcall SHHelpShortcuts_RunDLLA(long long long long)
322 stdcall SHHelpShortcuts_RunDLLW(long long long long)
323 stub SHInvokePrinterCommandA
324 stub SHInvokePrinterCommandW
325 stdcall SHIsFileAvailableOffline(wstr ptr)
326 stdcall SHLoadInProc(long)
327 stdcall SHLoadNonloadedIconOverlayIdentifiers()
328 stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
329 stdcall SHParseDisplayName(wstr ptr ptr long ptr)
330 stdcall SHPathPrepareForWriteA(long ptr str long)
331 stdcall SHPathPrepareForWriteW(long ptr wstr long)
332 stdcall SHQueryRecycleBinA(str ptr)
333 stdcall SHQueryRecycleBinW(wstr ptr)
334 stdcall SHSetLocalizedName(wstr wstr long)
335 stub SHSetUnreadMailCountW
336 stdcall SHUpdateRecycleBinIcon()
337 stdcall SheChangeDirA(str)
338 stub SheChangeDirExA
339 stub SheChangeDirExW
340 stdcall SheChangeDirW(wstr)
341 stub SheConvertPathW
342 stub SheFullPathA
343 stub SheFullPathW
344 stub SheGetCurDrive
345 stdcall SheGetDirA(long long)
346 stub SheGetDirExW
347 stdcall SheGetDirW(long long)
348 stub SheGetPathOffsetW
349 stub SheRemoveQuotesA
350 stub SheRemoveQuotesW
351 stub SheSetCurDrive
352 stub SheShortenPathA
353 stub SheShortenPathW
354 stdcall ShellAboutA(long str str long)
355 stdcall ShellAboutW(long wstr wstr long)
356 stub ShellExec_RunDLL
357 stub ShellExec_RunDLLA
358 stub ShellExec_RunDLLW
359 stdcall ShellExecuteA(long str str str str long)
360 stdcall ShellExecuteEx(long) ShellExecuteExA
361 stdcall ShellExecuteExA (long)
362 stdcall ShellExecuteExW (long)
363 stdcall ShellExecuteW(long wstr wstr wstr wstr long)
364 stub ShellHookProc
365 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
366 stdcall Shell_NotifyIconA(long ptr)
367 stdcall Shell_NotifyIconW(long ptr)
368 stdcall StrChrA(str long) shlwapi.StrChrA
369 stdcall StrChrIA(str long) shlwapi.StrChrIA
370 stdcall StrChrIW(wstr long) shlwapi.StrChrIW
371 stdcall StrChrW(wstr long) shlwapi.StrChrW
372 stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
373 stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
374 stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
375 stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
376 stdcall StrCpyNA (ptr str long) kernel32.lstrcpynA
377 stdcall StrCpyNW(wstr wstr long) shlwapi.StrCpyNW
378 stdcall StrNCmpA(str str long) shlwapi.StrCmpNA
379 stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA
380 stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
381 stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
382 stdcall StrNCpyA (ptr str long) kernel32.lstrcpynA
383 stdcall StrNCpyW(wstr wstr long) shlwapi.StrCpyNW
384 stdcall StrRChrA(str str long) shlwapi.StrRChrA
385 stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
386 stdcall StrRChrIW(str str long) shlwapi.StrRChrIW
387 stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
388 stub StrRStrA
389 stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
390 stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
391 stub StrRStrW
392 stdcall StrStrA(str str) shlwapi.StrStrA
393 stdcall StrStrIA(str str) shlwapi.StrStrIA
394 stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
395 stdcall StrStrW(wstr wstr) shlwapi.StrStrW
396 stub WOWShellExecute
520 stdcall SHAllocShared(ptr long long)
521 stdcall SHLockShared(long long)
522 stdcall SHUnlockShared(ptr)
523 stdcall SHFreeShared(long long)
524 stdcall RealDriveType(long long)
525 stub -noname RealDriveTypeFlags
526 stdcall SHFlushSFCache()
640 stdcall -noname NTSHChangeNotifyRegister(long long long long long long)
641 stdcall -noname NTSHChangeNotifyDeregister(long)
643 stub -noname SHChangeNotifyReceive # Fixme
644 stdcall SHChangeNotification_Lock(long long ptr ptr)
645 stdcall SHChangeNotification_Unlock(long)
646 stub -noname SHChangeRegistrationReceive # Fixme
648 stub -noname SHWaitOp_Operate
650 stdcall -noname PathIsSameRoot(ptr ptr) PathIsSameRootAW # Fixme
651 stdcall -noname SHReadCabinetState(long long) ReadCabinetState # Fixme
652 stdcall WriteCabinetState(long)
653 stdcall PathProcessCommand(long long long long) PathProcessCommandAW
654 stdcall ReadCabinetState(long long)
660 stdcall -noname FileIconInit(long)
680 stdcall IsUserAnAdmin()
681 stub -noname Shell_681
683 stub -noname Shell_683
684 stub -noname Shell_684
685 stub SHPropStgCreate
688 stub SHPropStgReadMultiple
689 stub SHPropStgWriteMultiple
690 stub -noname Shell_690
691 stub -noname Shell_691
700 stub CDefFolderMenu_Create
701 stdcall CDefFolderMenu_Create2(ptr ptr long ptr ptr ptr long ptr ptr)
702 stub -noname Shell_702
703 stub -noname Shell_703 # Fixme
704 stdcall -noname GUIDFromStringW(wstr ptr) # Fixme
707 stub -noname Shell_707 # Fixme
708 stub -noname Shell_708
709 stub SHGetSetFolderCustomSettingsW
711 stub -noname Shell_711
712 stub -noname Shell_712 # Fixme
713 stub -noname Shell_713
714 stdcall @(ptr) SHELL32_714 # PathIsTemporaryW # Fixme
715 stub -noname Shell_715 # Fixme
716 stub SHMultiFileProperties
719 stub -noname Shell_719
720 stub -noname Shell_720
721 stub -noname Shell_721
722 stub -noname Shell_722
723 stub -noname Shell_723
724 stub -noname Shell_724
725 stub -noname Shell_725
726 stub -noname Shell_726
727 stdcall SHGetImageList(long ptr ptr)
730 stdcall RestartDialogEx(long wstr long long)
731 stub -noname Shell_731
732 stub -noname Shell_732
733 stub -noname Shell_733
740 stub -noname Shell_740
743 stub SHCreateFileExtractIconW
744 stub -noname Shell_744
745 stub -noname Shell_745
747 stub SHLimitInputEdit
748 stub -noname Shell_748 # Fixme
749 stub SHGetShellStyleHInstance
750 stub SHGetAttributesFromDataObject
751 stub -noname Shell_751
752 stub -noname Shell_752
753 stub -noname Shell_753
754 stub -noname Shell_754
755 stub -noname Shell_755
756 stub -noname Shell_756

# Functions exported by the WinVista shell32.dll
@ stdcall SHCreateDefaultContextMenu(ptr ptr ptr)
@ stdcall SHCreateDefaultExtractIcon(ptr ptr)

# Unknown functions. They need to be removed
#@ stdcall FileMenu_DeleteAllItems(long)
#@ stdcall FileMenu_DrawItem(long ptr)
#@ stdcall FileMenu_FindSubMenuByPidl(long ptr)
#@ stdcall FileMenu_GetLastSelectedItemPidls(long ptr ptr)
#@ stdcall FileMenu_HandleMenuChar(long long)
#@ stdcall FileMenu_InitMenuPopup(long)
#@ stdcall FileMenu_InsertUsingPidl (long long ptr long long ptr)
#@ stdcall FileMenu_Invalidate(long)
#@ stdcall FileMenu_MeasureItem(long ptr)
#@ stdcall FileMenu_ReplaceUsingPidl(long long ptr long ptr)
#@ stdcall FileMenu_Create(long long long long long)
#@ stdcall FileMenu_AppendItem(long ptr long long long long) FileMenu_AppendItemAW
#@ stdcall FileMenu_TrackPopupMenuEx(long long long long long long)
#@ stdcall FileMenu_DeleteItemByCmd(long long)
#@ stdcall FileMenu_Destroy(long)
#@ stdcall FileMenu_AbortInitMenu()
#@ stdcall FileMenu_AppendFilesForPidl(long ptr long)
#@ stdcall FileMenu_AddFilesForPidl(long long long ptr long long ptr)
#@ stdcall FileMenu_DeleteItemByIndex(long long)
#@ stdcall FileMenu_DeleteItemByFirstID(long long)
#@ stdcall FileMenu_DeleteSeparator(long)
#@ stdcall FileMenu_EnableItemByCmd(long long long)
#@ stdcall FileMenu_GetItemExtent(long long)
#@ stdcall SHRegCloseKey (long)
#@ stdcall SHRegOpenKeyA (long str long)
#@ stdcall SHRegOpenKeyW (long wstr long)
#@ stdcall SHRegQueryValueA(long str ptr ptr)
#@ stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr)
#@ stdcall SHRegQueryValueW (long long long long)
#@ stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr)
#@ stdcall SHRegDeleteKeyW (long wstr)
