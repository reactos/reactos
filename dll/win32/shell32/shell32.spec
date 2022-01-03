2 stdcall SHChangeNotifyRegister(long long long long long ptr)
3 stdcall SHDefExtractIconA(str long long ptr ptr long)
4 stdcall SHChangeNotifyDeregister(long)
5 stdcall -noname SHChangeNotifyUpdateEntryList(long long long long)
6 stdcall SHDefExtractIconW(wstr long long ptr ptr long)
7 stdcall -noname SHLookupIconIndexA(str long long)
8 stdcall -noname SHLookupIconIndexW(wstr long long)
9 stdcall PifMgr_OpenProperties(wstr wstr long long)
10 stdcall PifMgr_GetProperties(ptr wstr ptr long long)
11 stdcall PifMgr_SetProperties(ptr wstr ptr long long)
12 stdcall -noname SHStartNetConnectionDialogA(ptr str long)
13 stdcall PifMgr_CloseProperties(ptr long)
14 stdcall SHStartNetConnectionDialogW(ptr wstr long)
15 stdcall -noname ILGetDisplayName(ptr ptr)
16 stdcall ILFindLastID(ptr)
17 stdcall ILRemoveLastID(ptr)
18 stdcall ILClone(ptr)
19 stdcall ILCloneFirst(ptr)
20 stdcall -noname ILGlobalClone(ptr)
21 stdcall ILIsEqual(ptr ptr)
22 stdcall DAD_DragEnterEx2(ptr long long ptr)
23 stdcall ILIsParent(ptr ptr long)
24 stdcall ILFindChild(ptr ptr)
25 stdcall ILCombine(ptr ptr)
26 stdcall ILLoadFromStream(ptr ptr)
27 stdcall ILSaveToStream(ptr ptr)
28 stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
29 stdcall -noname PathIsRoot(ptr) PathIsRootAW
30 stdcall -noname PathBuildRoot(ptr long) PathBuildRootW
31 stdcall -noname PathFindExtension(wstr) PathFindExtensionW
32 stdcall -noname PathAddBackslash(wstr) PathAddBackslashW
33 stdcall -noname PathRemoveBlanks(wstr) PathRemoveBlanksW
34 stdcall -noname PathFindFileName(wstr) PathFindFileNameW
35 stdcall -noname PathRemoveFileSpec(ptr) PathRemoveFileSpecAW
36 stdcall -noname PathAppend(ptr ptr) PathAppendAW
37 stdcall -noname PathCombine(wstr wstr wstr) PathCombineW
38 stdcall -noname PathStripPath(wstr) PathStripPathW
39 stdcall -noname PathIsUNC(wstr) PathIsUNCW
40 stdcall -noname PathIsRelative(wstr) PathIsRelativeW
41 stdcall IsLFNDriveA(str)
42 stdcall IsLFNDriveW(wstr)
43 stdcall PathIsExe(ptr) PathIsExeAW
44 stub -noname Control_RunDLLNoFallback
45 stdcall -noname PathFileExists(ptr) PathFileExistsAW # FIXME
46 stdcall -noname PathMatchSpec(wstr wstr) PathMatchSpecW
47 stdcall PathMakeUniqueName(ptr long ptr ptr ptr) PathMakeUniqueNameAW
48 stdcall -noname PathSetDlgItemPath(long long wstr) PathSetDlgItemPathW
49 stdcall PathQualify(ptr) PathQualifyAW
50 stdcall -noname PathStripToRoot(wstr) PathStripToRootW
51 stdcall PathResolve(str long long) PathResolveAW
52 stdcall -noname PathGetArgs(wstr) PathGetArgsW
53 stdcall -noname IsSuspendAllowed()
54 stdcall -noname LogoffWindowsDialog(ptr)
55 stdcall -noname PathQuoteSpaces(wstr) PathQuoteSpacesW
56 stdcall -noname PathUnquoteSpaces(wstr) PathUnquoteSpacesW
57 stdcall -noname PathGetDriveNumber(wstr) PathGetDriveNumberW
58 stdcall -noname ParseField(str long ptr long) ParseFieldAW # FIXME
59 stdcall RestartDialog(long wstr long)
60 stdcall -noname ExitWindowsDialog(long)
61 stdcall -noname RunFileDlg(long long long wstr wstr long)
62 stdcall PickIconDlg(long long long long)
63 stdcall GetFileNameFromBrowse(long long long long wstr wstr wstr)
64 stdcall DriveType(long)
65 stdcall -noname InvalidateDriveType(long)
66 stdcall IsNetDrive(long)
67 stdcall Shell_MergeMenus(long long long long long long)
68 stdcall SHGetSetSettings(ptr long long)
69 stdcall -noname SHGetNetResource(ptr long ptr long)
70 stdcall -noname SHCreateDefClassObject(long long long long long)
71 stdcall Shell_GetImageLists(ptr ptr)
72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
73 stdcall SHShellFolderView_Message(long long long)
74 stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
75 stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
76 stdcall -noname DragQueryInfo(ptr ptr)
77 stdcall SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
78 stdcall -noname OleStrToStrN(str long wstr long) OleStrToStrNAW # FIXME
79 stdcall -noname StrToOleStrN(wstr long str long) StrToOleStrNAW # FIXME
80 stdcall SHOpenPropSheetW(wstr ptr long ptr ptr ptr wstr)
81 stdcall OpenAs_RunDLL(long long str long) OpenAs_RunDLLA
82 stdcall -noname DDECreatePostNotify(ptr)
83 stdcall -noname CIDLData_CreateFromIDArray(ptr long ptr ptr)
84 stdcall -noname SHIsBadInterfacePtr(ptr long)
85 stdcall OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
86 stdcall -noname SHRegisterDragDrop(long ptr)
87 stdcall -noname SHRevokeDragDrop(long)
88 stdcall SHDoDragDrop(long ptr ptr long ptr)
89 stdcall SHCloneSpecialIDList(long long long)
90 stdcall SHFindFiles(ptr ptr)
91 stdcall -noname SHFindComputer(ptr ptr)
92 stdcall PathGetShortPath(ptr) PathGetShortPathAW
93 stdcall -noname Win32CreateDirectory(wstr ptr) Win32CreateDirectoryW
94 stdcall -noname Win32RemoveDirectory(wstr) Win32RemoveDirectoryW
95 stdcall -noname SHLogILFromFSIL(ptr)
96 stdcall -noname StrRetToStrN(ptr long ptr ptr) StrRetToStrNAW # FIXME
97 stdcall -noname SHWaitForFileToOpen(long long long)
98 stdcall SHGetRealIDL(ptr ptr ptr)
99 stdcall -noname SetAppStartingCursor(long long)
100 stdcall SHRestricted(long)
101 stdcall OpenAs_RunDLLA(long long str long)
102 stdcall SHCoCreateInstance(wstr ptr long ptr ptr)
103 stdcall SignalFileOpen(ptr)
104 stdcall OpenAs_RunDLLW(long long wstr long)
105 stdcall Activate_RunDLL(long ptr ptr ptr)
106 stdcall AppCompat_RunDLLW(ptr ptr wstr long)
107 stdcall CheckEscapesA(str long)
108 stdcall CheckEscapesW(wstr long)
109 stdcall CommandLineToArgvW(wstr ptr)
110 stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
111 stdcall Control_FillCache_RunDLLA(long long long long)
112 stdcall Control_FillCache_RunDLLW(long long long long)
113 stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
114 stdcall Control_RunDLLA(ptr ptr str long)
115 stdcall Control_RunDLLAsUserW(ptr ptr wstr long)
116 stdcall Control_RunDLLW(ptr ptr wstr long)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
@ stdcall -private DllGetVersion(ptr)
121 stdcall SHFlushClipboard()
122 stdcall -noname RunDll_CallEntry16(long long long str long)
123 stdcall -noname SHFreeUnusedLibraries()
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
126 stdcall -noname SHOutOfMemoryMessageBox(long long long)
127 stdcall -noname SHWinHelp(long long long long)
128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
129 stdcall DAD_AutoScroll(long ptr ptr)
130 stdcall -noname DAD_DragEnter(long)
131 stdcall DAD_DragEnterEx(long double)
132 stdcall DAD_DragLeave()
@ stdcall -private DllUnregisterServer()
134 stdcall DAD_DragMove(double)
135 stdcall DoEnvironmentSubstA(str str)
136 stdcall DAD_SetDragImage(long long)
137 stdcall DAD_ShowDragImage(long)
138 stdcall DoEnvironmentSubstW(wstr wstr)
139 stdcall DragAcceptFiles(long long)
140 stdcall DragFinish(long)
141 stdcall DragQueryFile(long long ptr long) DragQueryFileA
142 stdcall DragQueryFileA(long long ptr long)
143 stdcall DragQueryFileAorW(ptr long wstr long long long)
144 stdcall DragQueryFileW(long long ptr long)
145 stdcall -noname PathFindOnPath(wstr wstr) PathFindOnPathW
146 stdcall -noname RLBuildListOfPaths()
147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
148 stdcall SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
149 stdcall SHFind_InitMenuPopup(long long long long)
150 stdcall DragQueryPoint(long ptr)
151 stdcall SHLoadOLE(long)
152 stdcall ILGetSize(ptr)
153 stdcall ILGetNext(ptr)
154 stdcall ILAppendID(long long long)
155 stdcall ILFree(ptr)
156 stdcall -noname ILGlobalFree(ptr)
157 stdcall ILCreateFromPath(ptr) ILCreateFromPathAW
158 stdcall -noname PathGetExtension(wstr long long) SHPathGetExtensionW
159 stdcall -noname PathIsDirectory(wstr) PathIsDirectoryW
160 stdcall -noname SHNetConnectionDialog(ptr wstr long)
161 stdcall SHRunControlPanel(wstr ptr)
162 stdcall SHSimpleIDListFromPath(ptr) SHSimpleIDListFromPathAW # FIXME
163 stdcall -noname StrToOleStr(wstr str) StrToOleStrAW # FIXME
164 stdcall Win32DeleteFile(wstr) Win32DeleteFileW
165 stdcall SHCreateDirectory(long ptr)
166 stdcall CallCPLEntry16(long long long long long long)
167 stdcall SHAddFromPropSheetExtArray(long long long)
168 stdcall SHCreatePropSheetExtArray(long wstr long)
169 stdcall SHDestroyPropSheetExtArray(long)
170 stdcall SHReplaceFromPropSheetExtArray(long long long long)
171 stdcall PathCleanupSpec(ptr ptr)
172 stdcall -noname SHCreateLinks(long str ptr long ptr)
173 stdcall SHValidateUNC(ptr wstr long)
174 stdcall SHCreateShellFolderViewEx(ptr ptr)
175 stdcall -noname SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathW
176 stdcall SHSetInstanceExplorer(long)
177 stdcall -noname DAD_SetDragImageFromListView(ptr long long)
178 stdcall SHObjectProperties(long long wstr wstr)
179 stdcall SHGetNewLinkInfoA(str str ptr long long)
180 stdcall SHGetNewLinkInfoW(wstr wstr ptr long long)
181 stdcall -noname RegisterShellHook(long long)
182 varargs ShellMessageBoxW() ShellMessageBoxWrapW ## This is the no-named 'shlwapi.ShellMessageBoxWrapW' (i.e. 'shlwapi.#388')
183 varargs ShellMessageBoxA(ptr ptr str str long)
184 stdcall -noname ArrangeWindows(long long long long long)
185 stdcall -noname SHHandleDiskFull(ptr long)
186 stdcall -noname ILGetDisplayNameEx(ptr ptr ptr long)
187 stdcall -noname ILGetPseudoNameW(ptr ptr wstr long)
188 stdcall -noname ShellDDEInit(long)
189 stdcall ILCreateFromPathA(str)
190 stdcall ILCreateFromPathW(wstr)
191 stdcall SHUpdateImageA(str long long long)
192 stdcall SHUpdateImageW(wstr long long long)
193 stdcall SHHandleUpdateImage(ptr)
194 stdcall -noname SHCreatePropSheetExtArrayEx(long wstr long ptr)
195 stdcall SHFree(ptr)
196 stdcall SHAlloc(long)
197 stdcall -noname SHGlobalDefect(long)
198 stdcall -noname SHAbortInvokeCommand()
199 stdcall DuplicateIcon(long long)
200 stdcall -noname SHCreateDesktop(ptr)
201 stdcall -noname SHDesktopMessageLoop(ptr)
202 stub -noname DDEHandleViewFolderNotify
203 stdcall -noname AddCommasW(long wstr)
204 stdcall -noname ShortSizeFormatW(double)
205 stdcall -noname Printer_LoadIconsW(wstr ptr ptr)
206 stdcall ExtractAssociatedIconA(long str ptr)
207 stdcall ExtractAssociatedIconExA(long str long long)
208 stdcall ExtractAssociatedIconExW(long wstr long long)
209 stdcall -noname Int64ToString(int64 wstr long long ptr long)
210 stdcall -noname LargeIntegerToString(ptr wstr long long ptr long)
211 stdcall -noname Printers_GetPidl(ptr str long long)
212 stdcall -noname Printers_AddPrinterPropPages(ptr ptr)
213 stdcall -noname Printers_RegisterWindowW(wstr long ptr ptr)
214 stdcall -noname Printers_UnregisterWindow(long long)
215 stdcall -noname SHStartNetConnectionDialog(long str long)
216 stdcall ExtractAssociatedIconW(long wstr ptr)
217 stdcall ExtractIconA(long str long)
218 stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExA
219 stdcall ExtractIconExA(str long ptr ptr long)
220 stdcall ExtractIconExW(wstr long ptr ptr long)
221 stdcall ExtractIconResInfoA(ptr str long ptr ptr)
222 stdcall ExtractIconResInfoW(ptr wstr long ptr ptr)
223 stdcall ExtractIconW(long wstr long)
224 stdcall ExtractVersionResource16W(wstr ptr)
225 stdcall FindExeDlgProc(ptr long ptr ptr)
226 stdcall FindExecutableA(str str ptr)
227 stdcall FindExecutableW(wstr wstr ptr)
228 stdcall FreeIconList(long)
229 stdcall InternalExtractIconListA(ptr str ptr)
230 stdcall -noname FirstUserLogon(wstr wstr)
231 stdcall -noname SHSetFolderPathA(long ptr long str)
232 stdcall -noname SHSetFolderPathW(long ptr long wstr)
233 stdcall -noname SHGetUserPicturePathW(wstr long ptr)
234 stdcall -noname SHSetUserPicturePathW(wstr long ptr)
235 stdcall -noname SHOpenEffectiveToken(ptr)
236 stdcall -noname SHTestTokenPrivilegeW(ptr ptr)
237 stdcall -noname SHShouldShowWizards(ptr)
238 stdcall InternalExtractIconListW(ptr wstr ptr)
239 stdcall PathIsSlowW(wstr long)
240 stdcall PathIsSlowA(str long)
241 stdcall -noname SHGetUserDisplayName(wstr ptr)
242 stdcall -noname SHGetProcessDword(long long)
243 stdcall -noname SHSetShellWindowEx(ptr ptr) user32.SetShellWindowEx
244 stdcall -noname SHSettingsChanged(ptr ptr)
245 stdcall SHTestTokenMembership(ptr ptr)
246 stub -noname SHInvokePrivilegedFunctionW
247 stub -noname SHGetActiveConsoleSessionId
248 stdcall -noname SHGetUserSessionId(ptr)
249 stdcall -noname PathParseIconLocation(wstr) PathParseIconLocationW
250 stdcall -noname PathRemoveExtension(wstr) PathRemoveExtensionW
251 stdcall -noname PathRemoveArgs(wstr) PathRemoveArgsW
252 stdcall -noname PathIsURL(wstr) shlwapi.PathIsURLW
253 stub -noname SHIsCurrentProcessConsoleSession
254 stub -noname DisconnectWindowsDialog
255 stdcall Options_RunDLL(ptr ptr str long)
256 stdcall SHCreateShellFolderView(ptr ptr)
257 stdcall -noname SHGetShellFolderViewCB(ptr)
258 stdcall -noname LinkWindow_RegisterClass()
259 stdcall -noname LinkWindow_UnregisterClass(long)
260 stdcall Options_RunDLLA(ptr ptr str long)
261 stdcall Options_RunDLLW(ptr ptr wstr long)
262 stdcall PrintersGetCommand_RunDLL(ptr ptr wstr long)
263 stdcall PrintersGetCommand_RunDLLA(ptr ptr str long)
264 stdcall PrintersGetCommand_RunDLLW(ptr ptr wstr long)
265 stdcall RealShellExecuteA(ptr str str str str str str str long ptr)
266 stdcall RealShellExecuteExA(ptr str str str str str str str long ptr long)
267 stdcall RealShellExecuteExW(ptr str str str str str str str long ptr long)
268 stdcall RealShellExecuteW(ptr wstr wstr wstr wstr wstr wstr wstr long ptr)
269 stdcall RegenerateUserEnvironment(ptr long)
270 stdcall SHAddToRecentDocs(long ptr)
271 stdcall SHAppBarMessage(long ptr)
272 stdcall SHBindToParent(ptr ptr ptr ptr)
273 stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
274 stdcall SHBrowseForFolderA(ptr)
275 stdcall SHBrowseForFolderW(ptr)
276 stdcall SHChangeNotify(long long ptr ptr)
277 stdcall SHChangeNotifySuspendResume(long ptr long long)
278 stdcall SHCreateDirectoryExA(long str ptr)
279 stdcall SHCreateDirectoryExW(long wstr ptr)
280 stub SHCreateLocalServerRunDll
281 stdcall SHCreateProcessAsUserW(ptr)
282 stdcall SHCreateQueryCancelAutoPlayMoniker(ptr)
283 stdcall SHCreateShellItem(ptr ptr ptr ptr)
284 stdcall SHEmptyRecycleBinA(long str long)
285 stdcall SHEmptyRecycleBinW(long wstr long)
286 stub SHEnableServiceObject
287 stdcall SHEnumerateUnreadMailAccountsW(ptr long ptr long)
288 stdcall SHExtractIconsW(wstr long long long ptr ptr long long) user32.PrivateExtractIconsW
289 stdcall SHFileOperation(ptr) SHFileOperationA
290 stdcall SHFileOperationA(ptr)
291 stdcall SHFileOperationW(ptr)
292 stdcall SHFormatDrive(long long long long)
293 stdcall SHFreeNameMappings(ptr)
294 stdcall SHGetDataFromIDListA(ptr ptr long ptr long)
295 stdcall SHGetDataFromIDListW(ptr ptr long ptr long)
296 stdcall SHGetDesktopFolder(ptr)
297 stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
298 stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
299 stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
300 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA
301 stdcall SHGetFileInfoA(ptr long ptr long long)
302 stdcall SHGetFileInfoW(ptr long ptr long long)
303 stdcall SHGetFolderLocation(long long long long ptr)
304 stdcall SHGetFolderPathA(long long long long ptr)
305 stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
306 stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
307 stdcall SHGetFolderPathW(long long long long ptr)
308 stdcall SHGetIconOverlayIndexA(str long)
309 stdcall SHGetIconOverlayIndexW(wstr long)
310 stdcall SHGetInstanceExplorer(long)
311 stdcall SHGetMalloc(ptr)
312 stdcall SHGetNewLinkInfo(str str ptr long long) SHGetNewLinkInfoA
313 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA
314 stdcall SHGetPathFromIDListA(ptr ptr)
315 stdcall SHGetPathFromIDListW(ptr ptr)
316 stdcall SHGetSettings(ptr long)
317 stdcall SHGetSpecialFolderLocation(long long ptr)
318 stdcall SHGetSpecialFolderPathA(long ptr long long)
319 stdcall SHGetSpecialFolderPathW(long ptr long long)
320 stdcall SHGetUnreadMailCountW (long wstr long ptr wstr long)
321 stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
322 stdcall SHHelpShortcuts_RunDLLA(long long long long)
323 stdcall SHHelpShortcuts_RunDLLW(long long long long)
324 stdcall SHInvokePrinterCommandA(ptr long str str long)
325 stdcall SHInvokePrinterCommandW(ptr long wstr wstr long)
326 stdcall SHIsFileAvailableOffline(wstr ptr)
327 stdcall SHLoadInProc(long)
328 stdcall SHLoadNonloadedIconOverlayIdentifiers()
329 stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
330 stdcall SHParseDisplayName(wstr ptr ptr long ptr)
331 stdcall SHPathPrepareForWriteA(long ptr str long)
332 stdcall SHPathPrepareForWriteW(long ptr wstr long)
333 stdcall SHQueryRecycleBinA(str ptr)
334 stdcall SHQueryRecycleBinW(wstr ptr)
335 stdcall SHSetLocalizedName(wstr wstr long)
336 stdcall SHSetUnreadMailCountW (wstr long wstr)
337 stdcall SHUpdateRecycleBinIcon()
338 stdcall SheChangeDirA(str)
339 stdcall SheChangeDirExA(str)
340 stdcall SheChangeDirExW(wstr)
341 stdcall SheChangeDirW(wstr)
342 stdcall SheConvertPathW(wstr wstr long)
343 stdcall SheFullPathA(str long str)
344 stdcall SheFullPathW(wstr long wstr)
345 stdcall SheGetCurDrive()
346 stdcall SheGetDirA(long long)
347 stdcall SheGetDirExW(wstr ptr wstr)
348 stdcall SheGetDirW(long long)
349 stdcall SheGetPathOffsetW(wstr)
350 stdcall SheRemoveQuotesA(str)
351 stdcall SheRemoveQuotesW(wstr)
352 stdcall SheSetCurDrive(long)
353 stdcall SheShortenPathA(str long)
354 stdcall SheShortenPathW(wstr long)
355 stdcall ShellAboutA(long str str long)
356 stdcall ShellAboutW(long wstr wstr long)
357 stdcall ShellExec_RunDLL(ptr ptr wstr long)
358 stdcall ShellExec_RunDLLA(ptr ptr str long)
359 stdcall ShellExec_RunDLLW(ptr ptr wstr long)
360 stdcall ShellExecuteA(long str str str str long)
361 stdcall ShellExecuteEx(long) ShellExecuteExA
362 stdcall ShellExecuteExA (long)
363 stdcall ShellExecuteExW (long)
364 stdcall ShellExecuteW(long wstr wstr wstr wstr long)
365 stdcall ShellHookProc(long ptr ptr)
366 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
367 stdcall Shell_NotifyIconA(long ptr)
368 stdcall Shell_NotifyIconW(long ptr)
369 stdcall StrChrA(str long) shlwapi.StrChrA
370 stdcall StrChrIA(str long) shlwapi.StrChrIA
371 stdcall StrChrIW(wstr long) shlwapi.StrChrIW
372 stdcall StrChrW(wstr long) shlwapi.StrChrW
373 stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
374 stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
375 stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
376 stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
377 stdcall StrCpyNA (ptr str long) kernel32.lstrcpynA
378 stdcall StrCpyNW(wstr wstr long) shlwapi.StrCpyNW
379 stdcall StrNCmpA(str str long) shlwapi.StrCmpNA
380 stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA
381 stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
382 stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
383 stdcall StrNCpyA (ptr str long) kernel32.lstrcpynA
384 stdcall StrNCpyW(wstr wstr long) shlwapi.StrCpyNW
385 stdcall StrRChrA(str str long) shlwapi.StrRChrA
386 stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
387 stdcall StrRChrIW(wstr wstr long) shlwapi.StrRChrIW
388 stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
389 stdcall StrRStrA(str str str)
390 stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
391 stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
392 stdcall StrRStrW(wstr wstr wstr)
393 stdcall StrStrA(str str) shlwapi.StrStrA
394 stdcall StrStrIA(str str) shlwapi.StrStrIA
395 stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
396 stdcall StrStrW(wstr wstr) shlwapi.StrStrW
397 stdcall WOWShellExecute(ptr str str str str long ptr)
520 stdcall SHAllocShared(ptr long long) shlwapi.SHAllocShared
521 stdcall SHLockShared(long long) shlwapi.SHLockShared
522 stdcall SHUnlockShared(ptr) shlwapi.SHUnlockShared
523 stdcall SHFreeShared(long long) shlwapi.SHFreeShared
524 stdcall RealDriveType(long long)
525 stdcall -noname RealDriveTypeFlags(long long)
526 stdcall SHFlushSFCache()
640 stdcall -noname NTSHChangeNotifyRegister(long long long long long long)
641 stdcall -noname NTSHChangeNotifyDeregister(long)
643 stdcall -noname SHChangeNotifyReceive(long long ptr ptr)
644 stdcall SHChangeNotification_Lock(long long ptr ptr)
645 stdcall SHChangeNotification_Unlock(long)
646 stdcall -noname SHChangeRegistrationReceive(ptr long)
648 stdcall -noname SHWaitOp_Operate(ptr long)
650 stdcall -noname PathIsSameRoot(wstr wstr) PathIsSameRootW
651 stdcall -noname OldReadCabinetState(long long) ReadCabinetState
652 stdcall WriteCabinetState(long)
653 stdcall PathProcessCommand(long long long long) PathProcessCommandAW
654 stdcall ReadCabinetState(long long)
660 stdcall -noname FileIconInit(long)
680 stdcall IsUserAnAdmin()
681 stdcall -noname SHGetAppCompatFlags(long) shlwapi.SHGetAppCompatFlags
683 stub -noname SHStgOpenStorageW
684 stub -noname SHStgOpenStorageA
685 stdcall SHPropStgCreate(ptr ptr ptr long long long ptr ptr)
688 stdcall SHPropStgReadMultiple(ptr long long ptr ptr)
689 stdcall SHPropStgWriteMultiple(ptr ptr long ptr ptr long)
690 stub -noname SHIsLegacyAnsiProperty
691 stub -noname SHFileSysBindToStorage
700 stdcall CDefFolderMenu_Create(ptr ptr long ptr ptr ptr ptr ptr ptr)
701 stdcall CDefFolderMenu_Create2(ptr ptr long ptr ptr ptr long ptr ptr)
702 stdcall -noname CDefFolderMenu_MergeMenu(ptr long long ptr)
703 stdcall -noname GUIDFromStringA(str ptr)
704 stdcall -noname GUIDFromStringW(wstr ptr)
707 stdcall -noname SHOpenPropSheetA(str ptr long ptr ptr ptr str)
708 stdcall -noname SHGetSetFolderCustomSettingsA(ptr str long)
709 stdcall SHGetSetFolderCustomSettingsW(ptr wstr long)
711 stdcall -noname CheckWinIniForAssocs()
712 stdcall -noname SHCopyMonikerToTemp(ptr wstr wstr long)
713 stdcall -noname PathIsTemporaryA(str)
714 stdcall -noname PathIsTemporaryW(wstr)
715 stdcall -noname SHCreatePropertyBag(ptr ptr)
716 stdcall SHMultiFileProperties(ptr long)
719 stdcall -noname SHParseDarwinIDFromCacheW(wstr wstr)
720 stdcall -noname MakeShellURLFromPathA(str str long)
721 stdcall -noname MakeShellURLFromPathW(wstr wstr long)
722 stub -noname SHCreateInstance
723 stdcall -noname SHCreateSessionKey(long ptr)
724 stdcall -noname SHIsTempDisplayMode()
725 stdcall -noname GetFileDescriptor(ptr long long wstr)
726 stdcall -noname CopyStreamUI(ptr ptr ptr)
727 stdcall SHGetImageList(long ptr ptr)
730 stdcall RestartDialogEx(long wstr long long)
731 stdcall -noname -stub SHRegisterDarwinLink(long long long)
732 stdcall -noname SHReValidateDarwinCache()
733 stdcall -noname CheckDiskSpace()
740 stdcall -noname SHCreateFileDataObject(ptr long ptr ptr ptr)
743 stdcall SHCreateFileExtractIconW(wstr long ptr ptr)
744 stub -noname Create_IEnumUICommand
745 stub -noname Create_IUIElement
747 stdcall SHLimitInputEdit(ptr ptr)
748 stdcall -noname SHLimitInputCombo(ptr ptr)
749 stdcall -noname -version=0x501-0x502 SHGetShellStyleHInstance()
750 stdcall -noname SHGetAttributesFromDataObject(ptr long ptr ptr)
751 stub -noname SHSimulateDropOnClsid
752 stdcall -noname SHGetComputerDisplayNameW(long long long long)
753 stdcall -noname CheckStagingArea()
754 stub -noname SHLimitInputEditWithFlags
755 stdcall -noname PathIsEqualOrSubFolder(wstr wstr)
756 stub -noname DeleteFileThumbnail
