2   stdcall SHChangeNotifyRegister(long long long long long ptr)
3   stdcall SHDefExtractIconA(str long long ptr ptr long)
4   stdcall SHChangeNotifyDeregister(long)
5   stdcall -noname SHChangeNotifyUpdateEntryList(long long long long)
6   stdcall SHDefExtractIconW(wstr long long ptr ptr long)
7   stdcall -noname SHLookupIconIndexA(str long long)
8   stdcall -noname SHLookupIconIndexW(wstr long long)
9   stdcall PifMgr_OpenProperties(wstr wstr long long)
10  stdcall PifMgr_GetProperties(ptr wstr ptr long long)
11  stdcall PifMgr_SetProperties(ptr wstr ptr long long)
12  stdcall -noname SHStartNetConnectionDialogA(ptr str long)
13  stdcall PifMgr_CloseProperties(ptr long)
14  stdcall SHStartNetConnectionDialogW(ptr wstr long)
15  stdcall -noname ILGetDisplayName(ptr ptr)
16  stdcall ILFindLastID(ptr)
17  stdcall ILRemoveLastID(ptr)
18  stdcall ILClone(ptr)
19  stdcall ILCloneFirst(ptr)
20  stdcall -noname ILGlobalClone(ptr)
21  stdcall ILIsEqual(ptr ptr)
22  stdcall DAD_DragEnterEx2(ptr long long ptr)
23  stdcall ILIsParent(ptr ptr long)
24  stdcall ILFindChild(ptr ptr)
25  stdcall ILCombine(ptr ptr)
26  stdcall ILLoadFromStream(ptr ptr)
27  stdcall ILSaveToStream(ptr ptr)
28  stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
29  stdcall -noname PathIsRoot(ptr) PathIsRootAW
30  stdcall -noname PathBuildRoot(ptr long) PathBuildRootAW # FIXME: should be PathBuildRootW
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
45  stdcall -noname PathFileExists(ptr) PathFileExistsAW # Fixme
46  stdcall -noname PathMatchSpec(wstr wstr) PathMatchSpecW
47  stdcall PathMakeUniqueName(ptr long ptr ptr ptr) PathMakeUniqueNameAW
48  stdcall -noname PathSetDlgItemPath(long long wstr) PathSetDlgItemPathW
49  stdcall PathQualify(ptr) PathQualifyAW
50  stdcall -noname PathStripToRoot(wstr) PathStripToRootW
51  stdcall PathResolve(str long long) PathResolveAW
52  stdcall -noname PathGetArgs(wstr) PathGetArgsW
53  stdcall -noname IsSuspendAllowed() # FIXME: Wine says DoEnvironmentSubst
54  stdcall -noname LogoffWindowsDialog(ptr)
55  stdcall -noname PathQuoteSpaces(wstr) PathQuoteSpacesW
56  stdcall -noname PathUnquoteSpaces(wstr) PathUnquoteSpacesW
57  stdcall -noname PathGetDriveNumber(wstr) PathGetDriveNumberW
58  stdcall -noname ParseField(str long ptr long) ParseFieldAW # Fixme
59  stdcall RestartDialog(long wstr long)
60  stdcall -noname ExitWindowsDialog(long) # Fixme
61  stdcall -noname RunFileDlg(long long long wstr wstr long) RunFileDlg # Fixme?
62  stdcall PickIconDlg(long long long long)
63  stdcall GetFileNameFromBrowse(long long long long wstr wstr wstr)
64  stdcall DriveType(long)
65  stdcall -noname InvalidateDriveType(long)
66  stdcall IsNetDrive(long)
67  stdcall Shell_MergeMenus(long long long long long long)
68  stdcall SHGetSetSettings(ptr long long)
69  stdcall -noname SHGetNetResource(ptr long ptr long)
70  stdcall -noname SHCreateDefClassObject(long long long long long)
71  stdcall Shell_GetImageLists(ptr ptr)
72  stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
73  stdcall SHShellFolderView_Message(long long long)
74  stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
75  stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
76  stdcall -noname DragQueryInfo(ptr ptr)
77  stdcall SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
78  stdcall -noname OleStrToStrN(str long wstr long) OleStrToStrNAW # Fixme
79  stdcall -noname StrToOleStrN(wstr long str long) StrToOleStrNAW # Fixme
80  stdcall SHOpenPropSheetW(wstr ptr long ptr ptr ptr wstr)
82  stdcall -noname DDECreatePostNotify(ptr)
83  stdcall -noname CIDLData_CreateFromIDArray(ptr long ptr ptr)
84  stdcall -noname SHIsBadInterfacePtr(ptr long) # Fixme
85  stdcall OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
86  stdcall -noname SHRegisterDragDrop(long ptr)
87  stdcall -noname SHRevokeDragDrop(long)
88  stdcall SHDoDragDrop(long ptr ptr long ptr)
89  stdcall SHCloneSpecialIDList(long long long)
90  stdcall SHFindFiles(ptr ptr)
91  stdcall -noname SHFindComputer(ptr ptr)
92  stdcall PathGetShortPath(ptr) PathGetShortPathAW
93  stdcall -noname Win32CreateDirectory(wstr ptr) Win32CreateDirectoryW
94  stdcall -noname Win32RemoveDirectory(wstr) Win32RemoveDirectoryW
95  stdcall -noname SHLogILFromFSIL(ptr)
96  stdcall -noname StrRetToStrN(ptr long ptr ptr) StrRetToStrNAW # Fixme
97  stdcall -noname SHWaitForFileToOpen(long long long)
98  stdcall SHGetRealIDL(ptr ptr ptr)
99  stdcall -noname SetAppStartingCursor(long long) # Fixme
100 stdcall SHRestricted(long)
102 stdcall SHCoCreateInstance(wstr ptr long ptr ptr)
103 stdcall SignalFileOpen(ptr)
119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
121 stdcall SHFlushClipboard()
122 stdcall -noname RunDLL_CallEntry16(long long long str long) # Fixme #name wrong?
123 stdcall -noname SHFreeUnusedLibraries()
126 stdcall -noname SHOutOfMemoryMessageBox(long long long) # Fixme
127 stdcall -noname SHWinHelp(long long long long)
128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
129 stdcall DAD_AutoScroll(long ptr ptr)
130 stdcall -noname DAD_DragEnter(long)
131 stdcall DAD_DragEnterEx(long double)
132 stdcall DAD_DragLeave()
134 stdcall DAD_DragMove(double)
136 stdcall DAD_SetDragImage(long long)
137 stdcall DAD_ShowDragImage(long)
145 stdcall -noname PathFindOnPath(wstr wstr) PathFindOnPathW
146 stdcall -noname RLBuildListOfPaths()
147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
148 stdcall SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
149 stdcall SHFind_InitMenuPopup(long long long long)
151 stdcall SHLoadOLE(long)
152 stdcall ILGetSize(ptr)
153 stdcall ILGetNext(ptr)
154 stdcall ILAppendID(long long long) ILAppend
155 stdcall ILFree(ptr)
156 stdcall -noname ILGlobalFree(ptr)
157 stdcall ILCreateFromPath(ptr) ILCreateFromPathAW
158 stdcall -noname PathGetExtension(wstr long long) SHPathGetExtensionW
159 stdcall -noname PathIsDirectory(wstr) PathIsDirectoryW
160 stdcall -noname SHNetConnectionDialog(ptr wstr long) # Fixme
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
177 stdcall DAD_SetDragImageFromListView(ptr long long)
178 stdcall SHObjectProperties(long long wstr wstr)
179 stdcall SHGetNewLinkInfoA(str str ptr long long)
180 stdcall SHGetNewLinkInfoW(wstr wstr ptr long long)
181 stdcall -noname RegisterShellHook(long long)
182 varargs ShellMessageBoxW(long long wstr wstr long)
183 varargs ShellMessageBoxA(long long str str long)
184 stdcall -noname ArrangeWindows(long long long long long)
185 stdcall -noname SHHandleDiskFull(ptr long) # Fixme
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
# FIXME: Wine has SHGetFileIcon/SHLocalAlloc/SHLocalFree/SHLocalReAlloc from 199-202
200 stdcall -noname SHCreateDesktop(ptr)
201 stdcall -noname SHDesktopMessageLoop(ptr)
202 stub -noname DDEHandleViewFolderNotify
203 stdcall -noname AddCommasW(long wstr)
204 stdcall -noname ShortSizeFormatW(double)
205 stdcall -noname Printer_LoadIconsW(wstr ptr ptr)
209 stdcall -noname Int64ToString(double wstr long long ptr long) # Fixme
210 stdcall -noname LargeIntegerToString(ptr wstr long long ptr long) # Fixme
211 stdcall -noname Printers_GetPidl(ptr str) # Fixme
212 stdcall -noname Printers_AddPrinterPropPages(ptr ptr)
213 stdcall -noname Printers_RegisterWindowW(wstr long ptr ptr)
214 stdcall -noname Printers_UnregisterWindow(long long)
215 stdcall -noname SHStartNetConnectionDialog(long str long)
230 stdcall -noname FirstUserLogon(wstr wstr)
231 stdcall -noname SHSetFolderPathA(long ptr long str)
232 stdcall -noname SHSetFolderPathW(long ptr long wstr)
233 stdcall -noname SHGetUserPicturePathW(wstr long ptr)
234 stdcall -noname SHSetUserPicturePathW(wstr long ptr)
235 stdcall -noname SHOpenEffectiveToken(ptr)
236 stdcall -noname SHTestTokenPrivilegeW(ptr ptr)
237 stdcall -noname SHShouldShowWizards(ptr)
239 stdcall PathIsSlowW(wstr long)
240 stdcall PathIsSlowA(str long)
241 stdcall -noname SHGetUserDisplayName(wstr ptr)
242 stdcall -noname SHGetProcessDword(long long)
243 stdcall -noname SHSetShellWindowEx(ptr ptr) user32.SetShellWindowEx
244 stdcall -noname SHInitRestricted(ptr ptr)
245 stdcall SHTestTokenMembership(ptr ptr)
246 stub -noname SHInvokePrivilegedFunctionW
247 stub -noname SHGetActiveConsoleSessionId
248 stdcall -noname SHGetUserSessionId(ptr) # Fixme
249 stdcall -noname PathParseIconLocation(wstr) PathParseIconLocationW
250 stdcall -noname PathRemoveExtension(wstr) PathRemoveExtensionW
251 stdcall -noname PathRemoveArgs(wstr) PathRemoveArgsW
252 stdcall -noname PathIsURL(wstr) shlwapi.PathIsURLW
253 stub -noname SHIsCurrentProcessConsoleSession
254 stub -noname DisconnectWindowsDialog
256 stdcall SHCreateShellFolderView(ptr ptr)
257 stdcall -noname SHGetShellFolderViewCB(ptr)
258 stdcall -noname LinkWindow_RegisterClass()
259 stdcall -noname LinkWindow_UnregisterClass()
520 stdcall SHAllocShared(ptr long long)
521 stdcall SHLockShared(long long)
522 stdcall SHUnlockShared(ptr)
523 stdcall SHFreeShared(long long)
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
650 stdcall -noname PathIsSameRoot(ptr ptr) PathIsSameRootAW # Fixme
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
731 stub -noname SHRegisterDarwinLink
732 stdcall -noname SHReValidateDarwinCache()
733 stdcall -noname CheckDiskSpace()
740 stub -noname SHCreateFileDataObject
743 stdcall SHCreateFileExtractIconW(wstr long ptr ptr)
744 stub -noname Create_IEnumUICommand
745 stub -noname Create_IUIElement
747 stdcall SHLimitInputEdit(ptr ptr)
748 stdcall -noname SHLimitInputCombo(ptr ptr)
749 stub SHGetShellStyleHInstance
750 stub SHGetAttributesFromDataObject
751 stub -noname SHSimulateDropOnClsid
752 stub -noname SHGetComputerDisplayNameW
753 stub -noname CheckStagingArea
754 stub -noname SHLimitInputEditWithFlags
755 stdcall -noname PathIsEqualOrSubFolder(wstr wstr)
756 stub -noname DeleteFileThumbnail

@ stdcall Activate_RunDLL(long ptr ptr ptr)
@ stdcall AppCompat_RunDLLW(ptr ptr wstr long)
@ stdcall CheckEscapesA(str long)
@ stdcall CheckEscapesW(wstr long)
@ stdcall CommandLineToArgvW(wstr ptr)
@ stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
@ stdcall Control_FillCache_RunDLLA(long long long long)
@ stdcall Control_FillCache_RunDLLW(long long long long)
@ stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
@ stdcall Control_RunDLLA(ptr ptr str long)
@ stdcall Control_RunDLLAsUserW(ptr ptr wstr long)
@ stdcall Control_RunDLLW(ptr ptr wstr long)
@ stdcall -private DllCanUnloadNow()
@ stdcall -private DllGetClassObject(ptr ptr ptr)
@ stdcall -private DllGetVersion(ptr)
@ stdcall -private DllInstall(long wstr)
@ stdcall -private DllRegisterServer()
@ stdcall -private DllUnregisterServer()
@ stdcall DoEnvironmentSubstA(str str)
@ stdcall DoEnvironmentSubstW(wstr wstr)
@ stdcall DragAcceptFiles(long long)
@ stdcall DragFinish(long)
@ stdcall DragQueryFile(long long ptr long) DragQueryFileA
@ stdcall DragQueryFileA(long long ptr long)
@ stdcall DragQueryFileAorW(ptr long wstr long long long)
@ stdcall DragQueryFileW(long long ptr long)
@ stdcall DragQueryPoint(long ptr)
@ stdcall DuplicateIcon(long long)
@ stdcall ExtractAssociatedIconA(long str ptr)
@ stdcall ExtractAssociatedIconExA(long str long long)
@ stdcall ExtractAssociatedIconExW(long wstr long long)
@ stdcall ExtractAssociatedIconW(long wstr ptr)
@ stdcall ExtractIconA(long str long)
@ stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExA
@ stdcall ExtractIconExA(str long ptr ptr long)
@ stdcall ExtractIconExW(wstr long ptr ptr long)
@ stdcall ExtractIconResInfoA(ptr str long ptr ptr)
@ stdcall ExtractIconResInfoW(ptr wstr long ptr ptr)
@ stdcall ExtractIconW(long wstr long)
@ stdcall ExtractVersionResource16W(wstr ptr)
@ stdcall FindExecutableA(str str ptr)
@ stdcall FindExecutableW(wstr wstr ptr)
@ stdcall FindExeDlgProc(ptr long ptr ptr)
@ stdcall FreeIconList(long)
@ stdcall InternalExtractIconListA(ptr str ptr)
@ stdcall InternalExtractIconListW(ptr wstr ptr)
@ stdcall OpenAs_RunDLL(long long str long) OpenAs_RunDLLA
@ stdcall OpenAs_RunDLLA(long long str long)
@ stdcall OpenAs_RunDLLW(long long wstr long)
@ stdcall Options_RunDLL(ptr ptr str long)
@ stdcall Options_RunDLLA(ptr ptr str long)
@ stdcall Options_RunDLLW(ptr ptr wstr long)
@ stdcall PrintersGetCommand_RunDLL(ptr ptr wstr long)
@ stdcall PrintersGetCommand_RunDLLA(ptr ptr str long)
@ stdcall PrintersGetCommand_RunDLLW(ptr ptr wstr long)
@ stdcall RealShellExecuteA(ptr str str str str str str str long ptr)
@ stdcall RealShellExecuteExA(ptr str str str str str str str long ptr long)
@ stdcall RealShellExecuteExW(ptr str str str str str str str long ptr long)
@ stdcall RealShellExecuteW(ptr wstr wstr wstr wstr wstr wstr wstr long ptr)
@ stdcall RegenerateUserEnvironment(ptr long)
@ stdcall SHAddToRecentDocs(long ptr)
@ stdcall SHAppBarMessage(long ptr)
@ stdcall SHBindToParent(ptr ptr ptr ptr)
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
@ stdcall SHBrowseForFolderA(ptr)
@ stdcall SHBrowseForFolderW(ptr)
@ stdcall SHChangeNotify(long long ptr ptr)
@ stdcall SHChangeNotifySuspendResume(long ptr long long)
@ stdcall SHCreateDirectoryExA(long str ptr)
@ stdcall SHCreateDirectoryExW(long wstr ptr)
@ stub SHCreateLocalServerRunDll
@ stdcall SHCreateProcessAsUserW(ptr)
@ stdcall SHCreateQueryCancelAutoPlayMoniker(ptr)
@ stdcall SHCreateShellItem(ptr ptr ptr ptr)
@ stdcall SHEmptyRecycleBinA(long str long)
@ stdcall SHEmptyRecycleBinW(long wstr long)
@ stub SHEnableServiceObject
@ stub SHEnumerateUnreadMailAccountsW
@ stdcall SHExtractIconsW(wstr long long long ptr ptr long long) user32.PrivateExtractIconsW
@ stdcall SHFileOperation(ptr) SHFileOperationA
@ stdcall SHFileOperationA(ptr)
@ stdcall SHFileOperationW(ptr)
@ stdcall SHFormatDrive(long long long long)
@ stdcall SHFreeNameMappings(ptr)
@ stdcall SHGetDataFromIDListA(ptr ptr long ptr long)
@ stdcall SHGetDataFromIDListW(ptr ptr long ptr long)
@ stdcall SHGetDesktopFolder(ptr)
@ stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA
@ stdcall SHGetFileInfoA(ptr long ptr long long)
@ stdcall SHGetFileInfoW(ptr long ptr long long)
@ stdcall SHGetFolderLocation(long long long long ptr)
@ stdcall SHGetFolderPathA(long long long long ptr)
@ stdcall SHGetFolderPathAndSubDirA(long long long long str ptr)
@ stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr)
@ stdcall SHGetFolderPathW(long long long long ptr)
@ stdcall SHGetIconOverlayIndexA(str long)
@ stdcall SHGetIconOverlayIndexW(wstr long)
@ stdcall SHGetInstanceExplorer(long)
@ stdcall SHGetMalloc(ptr)
@ stdcall SHGetNewLinkInfo(str str ptr long long) SHGetNewLinkInfoA
@ stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA
@ stdcall SHGetPathFromIDListA(ptr ptr)
@ stdcall SHGetPathFromIDListW(ptr ptr)
@ stdcall SHGetSettings(ptr long)
@ stdcall SHGetSpecialFolderLocation(long long ptr)
@ stdcall SHGetSpecialFolderPathA(long ptr long long)
@ stdcall SHGetSpecialFolderPathW(long ptr long long)
@ stdcall SHGetUnreadMailCountW (long wstr long ptr wstr long)
@ stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
@ stdcall SHHelpShortcuts_RunDLLA(long long long long)
@ stdcall SHHelpShortcuts_RunDLLW(long long long long)
@ stdcall SHInvokePrinterCommandA(ptr long str str long)
@ stdcall SHInvokePrinterCommandW(ptr long wstr wstr long)
@ stdcall SHIsFileAvailableOffline(wstr ptr)
@ stdcall SHLoadInProc(long)
@ stdcall SHLoadNonloadedIconOverlayIdentifiers()
@ stdcall SHOpenFolderAndSelectItems(ptr long ptr long)
@ stdcall SHParseDisplayName(wstr ptr ptr long ptr)
@ stdcall SHPathPrepareForWriteA(long ptr str long)
@ stdcall SHPathPrepareForWriteW(long ptr wstr long)
@ stdcall SHQueryRecycleBinA(str ptr)
@ stdcall SHQueryRecycleBinW(wstr ptr)
@ stdcall SHSetLocalizedName(wstr wstr long)
@ stdcall SHSetUnreadMailCountW (wstr long wstr)
@ stdcall SHUpdateRecycleBinIcon()
@ stdcall SheChangeDirA(str)
@ stdcall SheChangeDirExA(str)
@ stdcall SheChangeDirExW(wstr)
@ stdcall SheChangeDirW(wstr)
@ stdcall SheConvertPathW(wstr wstr long)
@ stdcall SheFullPathA(str long str)
@ stdcall SheFullPathW(wstr long wstr)
@ stdcall SheGetCurDrive()
@ stdcall SheGetDirA(long long)
@ stdcall SheGetDirExW(wstr ptr wstr)
@ stdcall SheGetDirW(long long)
@ stdcall SheGetPathOffsetW(wstr)
@ stdcall SheRemoveQuotesA(str)
@ stdcall SheRemoveQuotesW(wstr)
@ stdcall SheSetCurDrive(long)
@ stdcall SheShortenPathA(str long)
@ stdcall SheShortenPathW(wstr long)
@ stdcall ShellAboutA(long str str long)
@ stdcall ShellAboutW(long wstr wstr long)
@ stdcall ShellExec_RunDLL(ptr ptr wstr long)
@ stdcall ShellExec_RunDLLA(ptr ptr str long)
@ stdcall ShellExec_RunDLLW(ptr ptr wstr long)
@ stdcall ShellExecuteA(long str str str str long)
@ stdcall ShellExecuteEx(long) ShellExecuteExA
@ stdcall ShellExecuteExA (long)
@ stdcall ShellExecuteExW (long)
@ stdcall ShellExecuteW(long wstr wstr wstr wstr long)
@ stdcall ShellHookProc(long ptr ptr)
@ stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
@ stdcall Shell_NotifyIconA(long ptr)
@ stdcall Shell_NotifyIconW(long ptr)
@ stdcall StrChrA(str long) shlwapi.StrChrA
@ stdcall StrChrIA(str long) shlwapi.StrChrIA
@ stdcall StrChrIW(wstr long) shlwapi.StrChrIW
@ stdcall StrChrW(wstr long) shlwapi.StrChrW
@ stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
@ stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
@ stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
@ stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
@ stdcall StrCpyNA (ptr str long) kernel32.lstrcpynA
@ stdcall StrCpyNW(wstr wstr long) shlwapi.StrCpyNW
@ stdcall StrNCmpA(str str long) shlwapi.StrCmpNA
@ stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA
@ stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
@ stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
@ stdcall StrNCpyA (ptr str long) kernel32.lstrcpynA
@ stdcall StrNCpyW(wstr wstr long) shlwapi.StrCpyNW
@ stdcall StrRChrA(str str long) shlwapi.StrRChrA
@ stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
@ stdcall StrRChrIW(wstr wstr long) shlwapi.StrRChrIW
@ stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
@ stdcall StrRStrA(str str str)
@ stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
@ stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
@ stdcall StrRStrW(wstr wstr wstr)
@ stdcall StrStrA(str str) shlwapi.StrStrA
@ stdcall StrStrIA(str str) shlwapi.StrStrIA
@ stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
@ stdcall StrStrW(wstr wstr) shlwapi.StrStrW
@ stdcall WOWShellExecute(ptr str str str str long ptr)

# Functions exported by the WinVista shell32.dll
@ stdcall SHCreateDefaultContextMenu(ptr ptr ptr)
@ stdcall SHCreateDefaultExtractIcon(ptr ptr)

# Discontinued in version 6.0. Until retired in WinVista and higher
@ stdcall FileMenu_DeleteAllItems(long)
@ stdcall FileMenu_DrawItem(long ptr)
@ stdcall FileMenu_FindSubMenuByPidl(long ptr)
@ stdcall FileMenu_GetLastSelectedItemPidls(long ptr ptr)
@ stdcall FileMenu_HandleMenuChar(long long)
@ stdcall FileMenu_InitMenuPopup(long)
@ stdcall FileMenu_InsertUsingPidl (long long ptr long long ptr)
@ stdcall FileMenu_Invalidate(long)
@ stdcall FileMenu_MeasureItem(long ptr)
@ stdcall FileMenu_ReplaceUsingPidl(long long ptr long ptr)
@ stdcall FileMenu_Create(long long long long long)
@ stdcall FileMenu_AppendItem(long ptr long long long long) FileMenu_AppendItemAW
@ stdcall FileMenu_TrackPopupMenuEx(long long long long long long)
@ stdcall FileMenu_DeleteItemByCmd(long long)
@ stdcall FileMenu_Destroy(long)
@ stdcall FileMenu_AbortInitMenu()
@ stdcall FileMenu_AppendFilesForPidl(long ptr long)
@ stdcall FileMenu_AddFilesForPidl(long long long ptr long long ptr)
@ stdcall FileMenu_DeleteItemByIndex(long long)
@ stdcall FileMenu_DeleteItemByFirstID(long long)
@ stdcall FileMenu_DeleteSeparator(long)
@ stdcall FileMenu_EnableItemByCmd(long long long)
@ stdcall FileMenu_GetItemExtent(long long)

# 4.0 (NT) and higher. Until discontinued in 5.0
# FIXME: ordinals 505-512?
@ stdcall SHRegCloseKey(long)
@ stdcall SHRegOpenKeyA(long str long)
@ stdcall SHRegOpenKeyW(long wstr long)
@ stdcall SHRegQueryValueA(long str ptr ptr)
@ stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr)
@ stdcall SHRegQueryValueW (long long long long)
@ stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr)
@ stdcall SHRegDeleteKeyW(long wstr)
