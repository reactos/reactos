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
72 stdcall Shell_GetCachedImageIndex(ptr ptr long)
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
102 stdcall SHCoCreateInstance(wstr ptr ptr ptr ptr)
103 stdcall SignalFileOpen(ptr)
119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
125 stdcall OpenAs_RunDLLA(long long str long)
126 stdcall -noname SHOutOfMemoryMessageBox(long long long)
127 stdcall -noname SHWinHelp(long long long long)
128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
129 stdcall DAD_AutoScroll(long ptr ptr)
130 stdcall -noname DAD_DragEnter(long)
131 stdcall DAD_DragEnterEx(long double)
132 stdcall DAD_DragLeave()
133 stdcall OpenAs_RunDLLW(long long wstr long)
134 stdcall DAD_DragMove(double)
135 stdcall -stub PrepareDiscForBurnRunDllW(long wstr long)
136 stdcall DAD_SetDragImage(long long)
137 stdcall DAD_ShowDragImage(long)
138 stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
139 stdcall SHHelpShortcuts_RunDLLA(long long long long)
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
150 stdcall SHHelpShortcuts_RunDLLW(long long long long)
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
162 stdcall SHSimpleIDListFromPath(ptr) SHSimpleIDListFromPathAW
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
182 varargs ShellMessageBoxW() ShellMessageBoxWrapW
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
199 stdcall AppCompat_RunDLLW(ptr ptr wstr long)
200 stdcall -noname SHCreateDesktop(ptr)
201 stdcall -noname SHDesktopMessageLoop(ptr)
202 stub -noname DDEHandleViewFolderNotify
203 stdcall -noname AddCommasW(long wstr)
204 stdcall -noname ShortSizeFormatW(long ptr)
205 stdcall -noname Printer_LoadIconsW(wstr ptr ptr)
206 stdcall -stub AssocCreateForClasses(ptr long ptr ptr)
207 stdcall -stub AssocGetDetailsOfPropKey(ptr ptr ptr)
208 stdcall CheckEscapesW(wstr long)
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
226 stdcall CommandLineToArgvW(wstr ptr)
228 stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
229 stdcall Control_RunDLLA(ptr ptr str long)
230 stdcall -noname FirstUserLogon(wstr wstr)
231 stdcall SHSetFolderPathA(long ptr long str)
232 stdcall SHSetFolderPathW(long ptr long wstr)
236 stdcall -noname SHTestTokenPrivilegeW(ptr wstr)
237 stdcall -noname SHShouldShowWizards(ptr)
238 stdcall Control_RunDLLAsUserW(ptr ptr wstr long)
239 stdcall PathIsSlowW(wstr long)
240 stdcall PathIsSlowA(str long)
241 stdcall -noname SHGetUserDisplayName(wstr ptr)
242 stdcall -noname SHGetProcessDword(long long)
243 stdcall -noname SHSetShellWindowEx(ptr ptr) user32.SetShellWindowEx
244 stdcall -noname SHSettingsChanged(ptr ptr)
245 stdcall SHTestTokenMembership(ptr ptr)
246 stdcall -noname SHInvokePrivilegedFunctionW(wstr ptr ptr)
247 stub -noname SHGetActiveConsoleSessionId
248 stdcall -noname SHGetUserSessionId(ptr)
249 stdcall -noname PathParseIconLocation(wstr) PathParseIconLocationW
250 stdcall -noname PathRemoveExtension(wstr) PathRemoveExtensionW
251 stdcall -noname PathRemoveArgs(wstr) PathRemoveArgsW
252 stdcall -noname PathIsURL(wstr) shlwapi.PathIsURLW
253 stub -noname SHIsCurrentProcessConsoleSession
254 stub -noname DisconnectWindowsDialog
255 stdcall Control_RunDLLW(ptr ptr wstr long)
256 stdcall SHCreateShellFolderView(ptr ptr)
257 stdcall -noname SHGetShellFolderViewCB(ptr)
258 stdcall -noname LinkWindow_RegisterClass()
259 stdcall -noname SHSetFolderPathA(long ptr long str)
260 stdcall -noname SHSetFolderPathW(long ptr long wstr)
261 stdcall -noname SHGetUserPicturePathW(wstr long ptr)
262 stdcall -noname SHSetUserPicturePathW(wstr long ptr)
263 stdcall DllCanUnloadNow()
264 stub -noname CreateInfoTipFromItem2
265 stub -noname ShellExecCmdLine
266 stub -noname ShellExecPidl
267 stdcall DllGetClassObject(ptr ptr ptr)
268 stdcall DllGetVersion(ptr)
269 stdcall DllInstall(long wstr)
270 stub -noname SHResolveUserNames
271 stdcall DllRegisterServer()
272 stdcall DllUnregisterServer()
273 stdcall DoEnvironmentSubstA(str str)
274 stdcall DoEnvironmentSubstW(wstr wstr)
275 stdcall DragAcceptFiles(long long)
276 stdcall DragFinish(long)
277 stdcall DragQueryFile(long long ptr long) DragQueryFileA
278 stdcall DragQueryFileA(long long ptr long)
279 stdcall DragQueryFileAorW(ptr long wstr long long long)
280 stdcall DragQueryFileW(long long ptr long)
281 stdcall DragQueryPoint(long ptr)
282 stdcall DuplicateIcon(long long)
283 stdcall ExtractAssociatedIconA(long str ptr)
284 stdcall ExtractAssociatedIconExA(long str long long)
285 stdcall ExtractAssociatedIconExW(long wstr long long)
286 stdcall ExtractAssociatedIconW(long wstr ptr)
287 stdcall ExtractIconA(long str long)
288 stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExA
289 stdcall ExtractIconExA(str long ptr ptr long)
290 stdcall ExtractIconExW(wstr long ptr ptr long)
291 stdcall ExtractIconW(long wstr long)
292 stdcall FindExecutableA(str str ptr)
293 stdcall FindExecutableW(wstr wstr ptr)
294 stdcall FreeIconList(long)
295 stdcall -stub InitNetworkAddressControl(ptr)
296 stdcall InternalExtractIconListA(ptr str ptr)
297 stdcall InternalExtractIconListW(ptr wstr ptr)
298 stdcall Options_RunDLL(ptr ptr str long)
299 stdcall Options_RunDLLA(ptr ptr str long)
300 stdcall Options_RunDLLW(ptr ptr wstr long)
301 stdcall PrintersGetCommand_RunDLL(ptr ptr wstr long)
302 stdcall PrintersGetCommand_RunDLLA(ptr ptr str long)
303 stdcall PrintersGetCommand_RunDLLW(ptr ptr wstr long)
304 stdcall RealShellExecuteA(ptr str str str str ptr str ptr long ptr)
305 stdcall RealShellExecuteExA(ptr str str str str ptr str ptr long ptr long)
306 stdcall RealShellExecuteExW(ptr wstr wstr wstr wstr ptr wstr ptr long ptr long)
307 stdcall RealShellExecuteW(ptr wstr wstr wstr wstr ptr wstr ptr long ptr)
308 stdcall RegenerateUserEnvironment(ptr long)
309 stdcall -stub SHAddDefaultPropertiesByExt(long ptr)
310 stdcall SHAddToRecentDocs(long ptr)
311 stdcall SHAppBarMessage(long ptr)
312 stdcall -stub SHAssocEnumHandlers(wstr ptr ptr)
313 stdcall -stub SHBindToFolderIDListParent(ptr ptr ptr ptr ptr)
314 stdcall -stub SHBindToFolderIDListParentEx(ptr ptr ptr ptr ptr ptr)
315 stdcall SHBindToObject(ptr ptr long ptr)
316 stdcall SHBindToParent(ptr ptr ptr ptr)
317 stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
318 stdcall SHBrowseForFolderA(ptr)
319 stdcall SHBrowseForFolderW(ptr)
320 stdcall SHChangeNotify(long long ptr ptr)
321 stdcall -stub SHChangeNotifyRegisterThread(ptr)
322 stdcall SHChangeNotifySuspendResume(long ptr long long)
323 stdcall -stub SHCreateAssociationRegistration(ptr ptr)
324 stdcall SHCreateDataObject(ptr long ptr ptr ptr ptr)
325 stdcall SHCreateDefaultContextMenu(ptr ptr ptr)
326 stdcall SHCreateDefaultExtractIcon(ptr ptr)
327 stdcall -stub SHCreateDefaultPropertiesOp(long ptr)
328 stdcall SHCreateDirectoryExA(long str ptr)
329 stdcall SHCreateDirectoryExW(long wstr ptr)
330 stdcall SHCreateItemFromIDList(ptr ptr ptr)
331 stdcall SHCreateItemFromParsingName(wstr ptr ptr ptr)
332 stdcall SHCreateItemFromRelativeName(ptr wstr ptr ptr ptr)
333 stdcall SHCreateItemInKnownFolder(ptr long wstr ptr ptr)
334 stdcall -stub SHCreateItemWithParent(ptr ptr ptr ptr ptr)
335 stdcall -stub SHCreateLocalServerRunDll(long long)
336 stdcall SHCreateProcessAsUserW(ptr)
337 stdcall SHCreateQueryCancelAutoPlayMoniker(ptr)
338 stdcall SHCreateShellItem(ptr ptr ptr ptr)
339 stdcall SHCreateShellItemArray(ptr ptr long ptr ptr)
340 stdcall SHCreateShellItemArrayFromDataObject(ptr ptr ptr)
341 stdcall SHCreateShellItemArrayFromIDLists(long ptr ptr)
342 stdcall SHCreateShellItemArrayFromShellItem(ptr ptr ptr)
343 stdcall SHEmptyRecycleBinA(long str long)
344 stdcall SHEmptyRecycleBinW(long wstr long)
345 stdcall -stub SHEnableServiceObject(long)
346 stdcall SHEnumerateUnreadMailAccountsW(ptr long ptr long)
347 stdcall -stub SHEvaluateSystemCommandTemplate(wstr wstr wstr wstr)
348 stdcall SHExtractIconsW(wstr long long long ptr ptr long long) user32.PrivateExtractIconsW
349 stdcall SHFileOperation(ptr) SHFileOperationA
350 stdcall SHFileOperationA(ptr)
351 stdcall SHFileOperationW(ptr)
352 stdcall SHFormatDrive(long long long long)
353 stdcall SHFreeNameMappings(ptr)
354 stdcall SHGetDataFromIDListA(ptr ptr long ptr long)
355 stdcall SHGetDataFromIDListW(ptr ptr long ptr long)
356 stdcall SHGetDesktopFolder(ptr)
357 stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
358 stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
359 stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceEx
360 stdcall -stub SHGetDriveMedia(long)
361 stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA
362 stdcall SHGetFileInfoA(ptr long ptr long long)
363 stdcall SHGetFileInfoW(ptr long ptr long long)
364 stdcall SHGetFolderLocation(long long long long ptr)
365 stdcall SHGetFolderPathA(long long long long ptr)
366 stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
367 stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
368 stdcall -stub SHGetFolderPathEx(ptr long ptr wstr long)
369 stdcall SHGetFolderPathW(long long long long ptr)
370 stdcall SHGetIDListFromObject(ptr ptr)
371 stdcall SHGetIconOverlayIndexA(str long)
372 stdcall SHGetIconOverlayIndexW(wstr long)
373 stdcall SHGetInstanceExplorer(long)
374 stdcall -stub SHGetKnownFolderIDList(ptr long ptr ptr)
375 stdcall -stub SHGetKnownFolderPath(ptr long ptr ptr)
376 stdcall -stub SHGetLocalizedName(wstr wstr long long)
377 stdcall SHGetMalloc(ptr)
378 stdcall SHGetNameFromIDList(ptr ptr wstr)
379 stdcall SHGetNewLinkInfo(str str ptr long long) SHGetNewLinkInfoA
380 stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA
381 stdcall SHGetPathFromIDListA(ptr ptr)
382 stdcall -stub SHGetPathFromIDListEx(ptr ptr long)
383 stdcall SHGetPathFromIDListW(ptr ptr)
384 stdcall -stub SHGetPropertyStoreFromIDList(ptr long ptr ptr)
385 stdcall SHGetPropertyStoreFromParsingName(wstr ptr ptr ptr ptr)
386 stdcall SHGetSettings(ptr long)
387 stdcall SHGetSpecialFolderLocation(long long ptr)
388 stdcall SHGetSpecialFolderPathA(long ptr long long)
389 stdcall SHGetSpecialFolderPathW(long ptr long long)
390 stdcall -stub SHGetStockIconInfo(long long ptr)
391 stdcall -stub SHGetTemporaryPropertyForItem(ptr ptr ptr)
392 stdcall SHGetUnreadMailCountW(ptr wstr ptr ptr ptr long)
393 stdcall SHInvokePrinterCommandA(ptr long str str long)
394 stdcall SHInvokePrinterCommandW(ptr long wstr wstr long)
395 stdcall SHIsFileAvailableOffline(wstr ptr)
396 stdcall SHLoadInProc(long)
397 stdcall SHLoadNonloadedIconOverlayIdentifiers()
398 stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
399 stdcall SHOpenWithDialog(ptr ptr)
400 stdcall SHParseDisplayName(wstr ptr ptr long ptr)
401 stdcall SHPathPrepareForWriteA(long ptr str long)
402 stdcall SHPathPrepareForWriteW(long ptr wstr long)
403 stdcall SHQueryRecycleBinA(str ptr)
404 stdcall SHQueryRecycleBinW(wstr ptr)
405 stdcall SHQueryUserNotificationState(ptr)
406 stdcall -stub SHRemoveLocalizedName(wstr wstr long)
407 stdcall -stub SHSetDefaultProperties(long ptr)
408 stdcall -stub SHSetKnownFolderPath(ptr long ptr wstr)
409 stdcall SHSetLocalizedName(wstr wstr long)
410 stdcall -stub SHSetTemporaryPropertyForItem(ptr ptr ptr)
411 stdcall SHSetUnreadMailCountW(wstr long wstr)
412 stdcall SHUpdateRecycleBinIcon()
413 stdcall SheChangeDirA(str)
414 stdcall SheChangeDirExW(wstr)
415 stdcall SheGetDirA(long long)
416 stdcall SheSetCurDrive(long)
417 stdcall ShellAboutA(long str str long)
418 stdcall ShellAboutW(long wstr wstr long)
419 stdcall ShellExec_RunDLL(ptr ptr str long) ShellExec_RunDLLA
420 stdcall ShellExec_RunDLLA(ptr ptr str long)
421 stdcall ShellExec_RunDLLW(ptr ptr wstr long)
422 stdcall ShellExecuteA(long str str str str long)
423 stdcall ShellExecuteEx(ptr) ShellExecuteExA
424 stdcall ShellExecuteExA(ptr)
425 stdcall ShellExecuteExW(ptr)
426 stdcall ShellExecuteW(long wstr wstr wstr wstr long)
427 stdcall ShellHookProc(long ptr ptr)
428 stdcall Shell_GetCachedImageIndexA(str long long) Shell_GetCachedImageIndexAW
429 stdcall Shell_GetCachedImageIndexW(wstr long long) Shell_GetCachedImageIndexAW
430 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
431 stdcall Shell_NotifyIconA(long ptr)
432 stdcall Shell_NotifyIconW(long ptr)
433 stdcall StrChrA(str long) shlwapi.StrChrA
434 stdcall StrChrIA(str long) shlwapi.StrChrIA
435 stdcall StrChrIW(wstr long) shlwapi.StrChrIW
436 stdcall StrChrW(wstr long) shlwapi.StrChrW
437 stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
438 stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
439 stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
440 stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
441 stdcall StrNCmpA(str str long) shlwapi.StrCmpNA
442 stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA
443 stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
444 stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
445 stdcall StrRChrA(str str long) shlwapi.StrRChrA
446 stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
447 stdcall StrRChrIW(wstr wstr long) shlwapi.StrRChrIW
448 stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
449 stdcall StrRStrA(str str str)
450 stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
451 stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
452 stdcall StrRStrW(wstr wstr wstr)
453 stdcall StrStrA(str str) shlwapi.StrStrA
454 stdcall StrStrIA(str str) shlwapi.StrStrIA
455 stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
456 stdcall StrStrW(wstr wstr) shlwapi.StrStrW
457 stdcall WOWShellExecute(ptr str str str str long ptr)
458 stdcall -stub WaitForExplorerRestartW(wstr)
520 stdcall SHAllocShared(ptr long long) shlwapi.SHAllocShared
521 stdcall SHLockShared(long long) shlwapi.SHLockShared
522 stdcall SHUnlockShared(ptr) shlwapi.SHUnlockShared
523 stdcall SHFreeShared(long long) shlwapi.SHFreeShared
524 stdcall RealDriveType(long long)
525 stub -noname DriveTypeExtendedFlags
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
724 stub -noname SHIsTempDisplayMode
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
748 stub -noname SHLimitInputCombo
750 stdcall SHGetAttributesFromDataObject(ptr long ptr ptr)
751 stub -noname SHSimulateDropOnClsid
752 stub -noname SHGetComputerDisplayName
753 stub -noname CheckStagingArea
754 stub -noname SHLimitInputEditWithFlags
755 stub -noname PathIsEqualOrSubFolder
757 stdcall -noname -version=0x600+ DisplayNameOfW(ptr ptr long ptr long)
758 stub -noname SHCreateThreadUndoManager
759 stub -noname SHGetThreadUndoManager
760 stub -noname SHConfirmOperation
761 stub -noname SHChangeNotifyDeregisterWindow
762 stub -noname Create_IUICommandFromDef
763 stub -noname Create_IEnumUICommandFromDefArray
764 stub -noname AssocCreateElement
766 stub -noname SHCopyStreamWithProgress
777 stub -noname SHGetAssocKeys
778 stub -noname AssocGetPropListForExt
781 stub -noname SHApplyPropertiesToItem
786 stub -noname SHCreateCategoryEnum
787 stub -noname SHMapIDListToSystemImageListIndexAsync
788 stub -noname SHCreateRelatedItemFromIDList
789 stub -noname SHCreateRelatedItemWithParent
790 stub -noname SHMapIDListToSystemImageListIndex
810 stub -noname SHGetUserPicturePathEx
811 stub -noname SHGetDefaultUserPicture
812 stub -noname SHUserGetPasswordHint
813 stub -noname SHUserSetPasswordHint
814 stub -noname SHCreateLeafConditionEx
815 stub -noname SHCreateAndOrConditionEx
816 stub -noname SHCreateAndOrCondition
817 stub -noname SHCreateLeafCondition
818 stub -noname SHCreateFilter
819 stub -noname SHLoadFilterFromStream
820 stub -noname SHCreateAutoList
821 stub -noname SHCreateSearchIDListFromAutoList
822 stub -noname SHCreateSearchIDList
823 stub -noname SHGetFolderTypeDescription
824 stub -noname SHGetFolderTypeFromCanonicalName
825 stub -noname SHCombineMultipleConditions
826 stub -noname SHCreateAutoListWithID
827 stub -noname SHCreateTransientVFolderIDList
828 stub -noname DrawMenuItem
829 stub -noname MeasureMenuItem
830 stub -noname SHCreateNotConditionEx
831 stub -noname SHCreateFilterFromFullText
832 stub -noname SHKnownFolderToCSIDL
833 stub -noname SHKnownFolderFromCSIDL
834 stub -noname SHCreateScopeFromIDLists
835 stub -noname SHCreateScopeItemFromIDLis
836 stub -noname SHCreateScopeFromShellItemArray
837 stub -noname SHCreateScopeItemFromShellItem
838 stub -noname SHCreateScopeItemFromKnownFolder
839 stub -noname CreateSingleVisibleInList
840 stub -noname PathGetPathDisplayName
841 stub -noname SHFilterConditionFromString
842 stub -noname SHFilterConditionToString
843 stub -noname SHGetIdentityItem
844 stub -noname SHCreateNotCondition
845 stub -noname CreateConditionRange
846 stdcall -stub ILLoadFromStreamEx(ptr ptr)
847 stdcall -stub -noname -version=0x600+ SHCombineMultipleConditionsEx(ptr ptr long ptr ptr ptr)
848 stub -noname SHGetNoAssocIconIndex
849 stub -noname SHCreateConditionFactory
850 stub -noname PathComparePaths
852 stub -noname SHInitializeControlPanelRegkeys
854 stub -noname IsShellItemInSearchIndex
856 stub -noname CPL_ExecuteTask
857 stub -noname CPL_CreateCondition
858 stub -noname POOBE_CreateIndirectGraphic
859 stub -noname WPC_InstallState
860 stdcall -stub -noname SHGetCorrectOwnerSid(ptr long wstr)
861 stdcall -stub -noname SHDisplayNameFromScopeAndSubQueries(long long long long long ptr)
862 stdcall -stub -noname SHCompareIDsFull(long long long long)
863 stdcall -stub -noname GetTryHarderIDList(long ptr ptr long long ptr)
864 stdcall -stub -noname StampIconForElevation(ptr long ptr)
865 stdcall -stub -noname IsElevationRequired(ptr)
866 stdcall -stub -noname SHExtCoCreateInstance(long long long long long long)
867 stdcall -stub -noname CreateVisibleInDescription(ptr ptr ptr)
868 stdcall -stub -noname CreateVisibleInList(ptr ptr ptr)
869 stdcall -stub -noname PathGetPathDisplayNameAlloc(wstr ptr)
870 stdcall -stub -noname DUI_Shell32_StartDeferUninitialization()
871 stdcall -stub -noname DUI_Shell32_EndDeferUninitialization()
872 stdcall -stub -noname SHCreateKindFilter(long ptr ptr)
873 stdcall -stub -noname SHIconIndexFromPIDL(ptr long long ptr)
874 stdcall -noname SHLaunchSearch(ptr ptr ptr)

; Win7+ - These ordinals are higher than any of the ones in win vista
881 stub -noname -version=0x601+ SHEnumClassesOfCategories
882 stub -noname -version=0x601+ SHWriteClassesOfCategories
884 stub -noname -version=0x601+ SHDoDragDropWithPreferredEffect
885 stdcall -noname -version=0x601+ RunInstallUninstallStubs()
886 stub -noname -version=0x601+ SHLaunchSearch
887 stub -noname -version=0x601+ SHExtCoCreateInstanceCheckCategory
888 stub -noname -version=0x601+ SHLimitInputEndSubclass
892 stub -noname -version=0x601+ GetSqmableFileName
893 stub -noname -version=0x601+ SetWindowRelaunchProperties
894 stub -noname -version=0x601+ MakeDestinationItem
895 stdcall -noname -version=0x601+ GetAppPathFromLink(ptr wstr long)
896 stub -noname -version=0x601+ ClearDestinationsForAllApps
897 stub -noname -version=0x601+ SaveTopViewSettings
899 stdcall -stub -noname -version=0x601+ SetExplorerServerMode(long)
900 stub -noname -version=0x601+ GetAppIDRoot
902 stub -noname -version=0x601+ IsSearchEnabled
 
@ stdcall -stub -version=0x601+ Shell_NotifyIconGetRect(ptr ptr)
@ stdcall -stub -version=0x601+ SHGetKnownFolderItem(ptr ptr ptr ptr ptr)
@ stdcall -stub -version=0x601+ SHAssocEnumHandlersForProtocolByApplication(wstr ptr ptr)
@ stdcall -version=0x601+ GetCurrentProcessExplicitAppUserModelID(wstr)
@ stdcall -version=0x601+ SetCurrentProcessExplicitAppUserModelID(wstr)
@ stdcall -stub -version=0x601+ StgMakeUniqueName(ptr wstr long ptr ptr)
