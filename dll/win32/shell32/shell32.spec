# Statically assigned ordinals:
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
101 stub -version=0x600+ -noname FileMenu_HandleNotify # shunimpl.RETIRED_FileMenu_HandleNotify # ->1 
102 stdcall SHCoCreateInstance(wstr ptr ptr ptr ptr)
103 stdcall SignalFileOpen(ptr)
104 stdcall -version=0x600+ -noname FileMenu_DeleteAllItems(ptr) # shunimpl.RETIRED_FileMenu_DeleteAllItems # ->2
105 stdcall -version=0x600+ -noname FileMenu_DrawItem(ptr ptr) # shunimpl.RETIRED_FileMenu_DrawItem # ->3
106 stdcall -version=0x600+ -noname FileMenu_FindSubMenuByPidl(ptr ptr) # shunimpl.RETIRED_FileMenu_FindSubMenuByPidl # ->4
107 stdcall -version=0x600+ -noname FileMenu_GetLastSelectedItemPidls(long ptr ptr) # shunimpl.RETIRED_FileMenu_GetLastSelectedItemPidls # ->5
108 stdcall -version=0x600+ -noname FileMenu_HandleMenuChar(ptr long) # shunimpl.RETIRED_FileMenu_HandleMenuChar # ->6
109 stdcall -version=0x600+ -noname FileMenu_InitMenuPopup(ptr) # shunimpl.RETIRED_FileMenu_InitMenuPopup # ->7
110 stub -version=0x600+ -noname FileMenu_ComposeA # shunimpl.RETIRED_FileMenu_ComposeA # ->8
111 stdcall -version=0x600+ -noname FileMenu_Invalidate(ptr) # shunimpl.RETIRED_FileMenu_Invalidate # ->9
112 stdcall -version=0x600+ -noname FileMenu_MeasureItem(ptr ptr) # shunimpl.RETIRED_FileMenu_MeasureItem # ->10
113 stub -version=0x600+ -noname FileMenu_ComposeW # shunimpl.RETIRED_FileMenu_ComposeW # ->11
114 stdcall -version=0x600+ -noname FileMenu_Create(long long ptr long long) # shunimpl.RETIRED_FileMenu_Create # ->12
115 stub -version=0x600+ -noname FileMenu_AppendItem # shunimpl.RETIRED_FileMenu_AppendItem # ->13
116 stdcall -version=0x600+ -noname FileMenu_TrackPopupMenuEx(ptr long long long ptr ptr) # shunimpl.RETIRED_FileMenu_TrackPopupMenuEx # ->14
117 stdcall -version=0x600+ -noname FileMenu_DeleteItemByCmd(ptr long) # shunimpl.RETIRED_FileMenu_DeleteItemByCmd # ->15
118 stdcall -version=0x600+ -noname FileMenu_Destroy(ptr) # shunimpl.RETIRED_FileMenu_Destroy # ->16 
119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
120 stdcall -version=0x600+ -noname FileMenu_AbortInitMenu() # shunimpl.RETIRED_FileMenu_AbortInitMenu # ->17
121 stdcall SHFlushClipboard()
122 stdcall -noname RunDll_CallEntry16(long long long str long)
123 stdcall -noname SHFreeUnusedLibraries()
124 stub -version=0x600+ -noname AppendFilesForPidl() # shunimpl.RETIRED_FileMenu_AppendFilesForPidl # ->18
126 stdcall -noname SHOutOfMemoryMessageBox(long long long)
127 stdcall -noname SHWinHelp(long long long long)
128 stdcall -noname SHDllGetClassObject(ptr ptr ptr) DllGetClassObject
129 stdcall DAD_AutoScroll(long ptr ptr)
130 stdcall -noname DAD_DragEnter(long)
131 stdcall DAD_DragEnterEx(long double)
132 stdcall DAD_DragLeave()
134 stdcall DAD_DragMove(double)
136 stdcall DAD_SetDragImage(long long)
137 stdcall DAD_ShowDragImage(long)
140 stdcall -version=0x600+ -noname FileMenu_DeleteItemByIndex(ptr long) # shunimpl.RETIRED_FileMenu_DeleteItemByIndex # ->19
141 stub -version=0x600+ -noname FileMenu_DeleteMenuItemByFirstID # shunimpl.RETIRED_FileMenu_DeleteMenuItemByFirstID # ->20
142 stdcall -version=0x600+ -noname FileMenu_DeleteSeparator(ptr) # shunimpl. RETIRED_FileMenu_DeleteSeparator # ->21
143 stdcall -version=0x600+ -noname FileMenu_EnableItemByCmd(ptr long long) # shunimpl. RETIRED_FileMenu_EnableItemByCmd # ->22
144 stdcall -version=0x600+ -noname FileMenu_GetItemExtent(ptr long) # shunimpl.RETIRED_FileMenu_GetItemExtent # ->23
145 stdcall -noname PathFindOnPath(wstr wstr) PathFindOnPathW
146 stdcall -noname RLBuildListOfPaths()
147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
148 stdcall SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
149 stdcall SHFind_InitMenuPopup(long long long long)
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
200 stdcall -noname SHCreateDesktop(ptr)
201 stdcall -noname SHDesktopMessageLoop(ptr)
202 stub -noname DDEHandleViewFolderNotify
203 stdcall -noname AddCommasW(long wstr)
204 stdcall -noname ShortSizeFormatW(long ptr)
205 stdcall -noname Printer_LoadIconsW(wstr ptr ptr)
209 stdcall -noname Int64ToString(int64 wstr long long ptr long)
210 stdcall -noname LargeIntegerToString(ptr wstr long long ptr long)
211 stdcall -noname Printers_GetPidl(ptr str long long)
212 stdcall -noname Printers_AddPrinterPropPages(ptr ptr)
213 stdcall -noname Printers_RegisterWindowW(wstr long ptr ptr)
214 stdcall -noname Printers_UnregisterWindow(long long)
215 stdcall -noname SHStartNetConnectionDialog(long str long)
216 stub -version=0x600+ -noname FileMenu_IsFileMenu # shunimpl.RETIRED_FileMenu_IsFileMenu # ->24
217 stub -version=0x600+ -noname FileMenu_ProcessCommand # shunimpl.RETIRED_FileMenu_ProcessCommand # ->25
218 stub -version=0x600+ -noname FileMenu_InsertItem # shunimpl.RETIRED_FileMenu_InsertItem # ->26
219 stub -version=0x600+ -noname FileMenu_InsertSeparator # shunimpl.RETIRED_FileMenu_InsertSeparator # ->27
220 stub -version=0x600+ -noname FileMenu_GetPidl # shunimpl.RETIRED_FileMenu_GetPidl # ->28
221 stub -version=0x600+ -noname FileMenu_EditMode # shunimpl.RETIRED_FileMenu_EditMode # ->29
222 stub -version=0x600+ -noname FileMenu_HandleMenuSelect # shunimpl.RETIRED_FileMenu_HandleMenuSelect # ->30
223 stub -version=0x600+ -noname FileMenu_IsUnexpanded # shunimpl.RETIRED_FileMenu_IsUnexpanded # ->31
224 stub -version=0x600+ -noname FileMenu_DelayedInvalidate # shunimpl.RETIRED_FileMenu_DelayedInvalidate # ->32
225 stub -version=0x600+ -noname FileMenu_IsDelayedInvalid # shunimpl.RETIRED_FileMenu_IsDelayedInvalid # ->33
227 stub -version=0x600+ -noname FileMenu_CreateFromMenu # shunimpl.RETIRED_FileMenu_CreateFromMenu # ->34
230 stdcall -noname FirstUserLogon(wstr wstr)
231 stdcall -noname SHSetFolderPathA(long ptr long str)
232 stdcall -noname SHSetFolderPathW(long ptr long wstr)
233 stdcall -noname SHGetUserPicturePathW(wstr long ptr)
234 stdcall -noname SHSetUserPicturePathW(wstr long ptr)
235 stdcall -noname SHOpenEffectiveToken(ptr)
236 stdcall -noname SHTestTokenPrivilegeW(ptr wstr)
237 stdcall -noname SHShouldShowWizards(ptr)
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
256 stdcall SHCreateShellFolderView(ptr ptr)
257 stdcall -noname SHGetShellFolderViewCB(ptr)
258 stdcall -noname LinkWindow_RegisterClass()
259 stdcall -noname LinkWindow_UnregisterClass(long)
260 stub -version=0x600+ -noname CreateInfoTipFromItem
261 stub -version=0x600+ -noname SHGetUserPicturePath
262 stub -version=0x600+ -noname SHSetUserPicturePath
264 stub -version=0x600+ -noname CreateInfoTipFromItem2
265 stdcall -version=0x600+ -noname ShellExecCmdLine(ptr ptr ptr long ptr long)
266 stub -version=0x600+ -noname ShellExecPidl
270 stub -version=0x600+ -noname SHResolveUserNames
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
726 stdcall -noname CopyStreamUI(ptr ptr ptr int64)
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
749 stdcall -noname SHGetShellStyleHInstance() # Vista: shunimpl.RETIRED_SHGetShellStyleHInstance
750 stdcall -noname SHGetAttributesFromDataObject(ptr long ptr ptr)
751 stdcall -noname SHSimulateDropOnClsid(ptr ptr ptr)
752 stdcall -noname SHGetComputerDisplayNameW(wstr long ptr long)
753 stdcall -noname CheckStagingArea()
754 stub -noname SHLimitInputEditWithFlags
755 stdcall -noname PathIsEqualOrSubFolder(wstr wstr)
756 stub -noname DeleteFileThumbnail
757 stdcall -noname -version=0x600+ DisplayNameOfW(ptr ptr long ptr long)
758 stub -version=0x600+ -noname SHCreateThreadUndoManager
759 stub -version=0x600+ -noname SHGetThreadUndoManager
760 stub -version=0x600+ -noname SHConfirmOperation
761 stub -version=0x600+ -noname SHChangeNotifyDeregisterWindow
762 stub -version=0x600+ -noname Create_IUICommandFromDef
763 stub -version=0x600+ -noname Create_IEnumUICommandFromDefArray
764 stub -version=0x600+ -noname AssocCreateElement
766 stub -version=0x600+ -noname SHCopyStreamWithProgress
777 stub -version=0x600+ -noname SHGetAssocKeys
778 stub -version=0x600+ -noname AssocGetPropListForExt
781 stub -version=0x600+ -noname SHApplyPropertiesToItem
786 stub -version=0x600+ -noname SHCreateCategoryEnum
787 stub -version=0x600+ -noname SHMapIDListToSystemImageListIndexAsync
788 stub -version=0x600+ -noname SHCreateRelatedItemFromIDList
789 stub -version=0x600+ -noname SHCreateRelatedItemWithParent
790 stub -version=0x600+ -noname SHMapIDListToSystemImageListIndex
810 stub -version=0x600+ -noname SHGetUserPicturePathEx
811 stub -version=0x600+ -noname SHGetDefaultUserPicture
812 stub -version=0x600+ -noname SHUserGetPasswordHint
813 stub -version=0x600+ -noname SHUserSetPasswordHint
814 stub -version=0x600+ -noname SHCreateLeafConditionEx
815 stub -version=0x600+ -noname SHCreateAndOrConditionEx
816 stub -version=0x600+ -noname SHCreateAndOrCondition
817 stub -version=0x600+ -noname SHCreateLeafCondition
818 stub -version=0x600+ -noname SHCreateFilter
819 stub -version=0x600+ -noname SHLoadFilterFromStream
820 stub -version=0x600+ -noname SHCreateAutoList
821 stub -version=0x600+ -noname SHCreateSearchIDListFromAutoList
822 stub -version=0x600+ -noname SHCreateSearchIDList
823 stub -version=0x600+ -noname SHGetFolderTypeDescription
824 stub -version=0x600+ -noname SHGetFolderTypeFromCanonicalName
825 stub -version=0x600+ -noname SHCombineMultipleConditions
826 stub -version=0x600+ -noname SHCreateAutoListWithID
827 stub -version=0x600+ -noname SHCreateTransientVFolderIDList
828 stub -version=0x600+ -noname DrawMenuItem
829 stub -version=0x600+ -noname MeasureMenuItem
830 stub -version=0x600+ -noname SHCreateNotConditionEx
831 stub -version=0x600+ -noname SHCreateFilterFromFullText
832 stub -version=0x600+ -noname SHKnownFolderToCSIDL
833 stub -version=0x600+ -noname SHKnownFolderFromCSIDL
834 stub -version=0x600+ -noname SHCreateScopeFromIDLists
835 stub -version=0x600+ -noname SHCreateScopeItemFromIDList
836 stub -version=0x600+ -noname SHCreateScopeFromShellItemArray
837 stub -version=0x600+ -noname SHCreateScopeItemFromShellItem
838 stub -version=0x600+ -noname SHCreateScopeItemFromKnownFolder
839 stub -version=0x600+ -noname CreateSingleVisibleInList
840 stub -version=0x600+ -noname PathGetPathDisplayName
841 stub -version=0x600+ -noname SHFilterConditionFromString
842 stub -version=0x600+ -noname SHFilterConditionToString
843 stub -version=0x600+ -noname SHGetIdentityItem
844 stub -version=0x600+ -noname SHCreateNotCondition
845 stub -version=0x600+ -noname CreateConditionRange
846 stub -version=0x600+ ILLoadFromStreamEx
847 stub -version=0x600+ -noname SHCombineMultipleConditionsEx
848 stub -version=0x600+ -noname SHGetNoAssocIconIndex
849 stub -version=0x600+ -noname SHCreateConditionFactory
850 stub -version=0x600+ -noname PathComparePaths
852 stub -version=0x600+ -noname SHInitializeControlPanelRegkeys
854 stub -version=0x600+ -noname IsShellItemInSearchIndex
856 stub -version=0x600+ -noname CPL_ExecuteTask
857 stub -version=0x600+ -noname CPL_CreateCondition
858 stub -version=0x600+ -noname POOBE_CreateIndirectGraphic
859 stub -version=0x600+ -noname WPC_InstallState
860 stub -version=0x600+ -noname SHGetCorrectOwnerSid
861 stub -version=0x600+ -noname SHDisplayNameFromScopeAndSubQueries
862 stub -version=0x600+ -noname SHCompareIDsFull
863 stub -version=0x600+ -noname GetTryHarderIDList
864 stub -version=0x600+ -noname StampIconForElevation
865 stub -version=0x600+ -noname IsElevationRequired
866 stdcall -noname -version=0x600+ SHExtCoCreateInstance(wstr ptr ptr ptr ptr)
867 stub -version=0x600+ -noname CreateVisibleInDescription
868 stub -version=0x600+ -noname CreateVisibleInList
869 stub -version=0x600+ -noname PathGetPathDisplayNameAlloc
870 stub -version=0x600+ -noname DUI_Shell32_StartDeferUninitialization
871 stub -version=0x600+ -noname DUI_Shell32_EndDeferUninitialization
872 stub -version=0x600+ -noname SHCreateKindFilter
873 stub -version=0x600+ -noname SHIconIndexFromPIDL
874 stub -version=0x600+ -noname SHLaunchSearch
887 stub -noname -version=0x601+ SHExtCoCreateInstanceCheckCategory

# Automatically assigned ordinals:
@ stdcall OpenAs_RunDLLA(long long str long) # 2k3:101, Vista:125
@ stdcall OpenAs_RunDLLW(long long wstr long) # 2k3:104, Vista:133
@ stdcall -version=0x502 Activate_RunDLL(ptr ptr wstr long) # 2k3:105
@ stub -version=0x600+ PrepareDiscForBurnRunDllW # Vista:135
@ stdcall -version=0x600+ SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA # 2k3:see below, Vista:138
@ stdcall -version=0x600+ SHHelpShortcuts_RunDLLA(long long long long) # 2k3:see below, Vista:139
@ stdcall -version=0x600+ SHHelpShortcuts_RunDLLW(long long long long) # 2k3:see below, Vista:150
@ stdcall AppCompat_RunDLLW(ptr ptr wstr long) # 2k3:106, Vista:199
@ stub -version=0x600+ AssocCreateForClasses # Vista:206
@ stub -version=0x600+ AssocGetDetailsOfPropKey # Vista:207
@ stdcall -version=0x502 CheckEscapesA(str long) # 2k3:107
@ stdcall CheckEscapesW(wstr long) # 2k3:108, Vista:208
@ stdcall CommandLineToArgvW(wstr ptr) # 2k3:109, Vista:226
@ stdcall -version=0x502 Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA # 2k3:110
@ stdcall -version=0x502 Control_FillCache_RunDLLA(long long long long) # 2k3:111
@ stdcall -version=0x502 Control_FillCache_RunDLLW(long long long long) # 2k3:112
@ stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA # 2k3:113, Vista:228
@ stdcall Control_RunDLLA(ptr ptr str long) # 2k3:114, Vista:229
@ stdcall Control_RunDLLAsUserW(ptr ptr wstr long) # 2k3:115, Vista:238
@ stdcall Control_RunDLLW(ptr ptr wstr long) # 2k3:116, Vista:255
@ stdcall -private DllCanUnloadNow() # 2k3:117, Vista:263
@ stdcall -private DllGetClassObject(ptr ptr ptr) # 2k3:118, Vista:267
@ stdcall -private DllGetVersion(ptr) # 2k3:120, Vista:268
@ stdcall -private DllInstall(long wstr) # 2k3:124, Vista:269
@ stdcall -private DllRegisterServer() # 2k3:125, Vista:271
@ stdcall -private DllUnregisterServer() # 2k3:133, Vista:272
@ stdcall DoEnvironmentSubstA(str str) # 2k3:135, Vista:273
@ stdcall DoEnvironmentSubstW(wstr wstr) # 2k3:138, Vista:274
@ stdcall DragAcceptFiles(long long) # 2k3:139, Vista:275
@ stdcall DragFinish(long) # 2k3:140, Vista:276
@ stdcall DragQueryFile(long long ptr long) DragQueryFileA # 2k3:141, Vista:277
@ stdcall DragQueryFileA(long long ptr long) # 2k3:142, Vista:278
@ stdcall DragQueryFileAorW(ptr long wstr long long long) # 2k3:143, Vista:279
@ stdcall DragQueryFileW(long long ptr long) # 2k3:144, Vista:280
@ stdcall DragQueryPoint(long ptr) # 2k3:150, Vista:281
@ stdcall DuplicateIcon(long long) # 2k3:199, Vista:282
@ stdcall ExtractAssociatedIconA(long str ptr) # 2k3:206, Vista:283
@ stdcall ExtractAssociatedIconExA(long str long long) # 2k3:207, Vista:284
@ stdcall ExtractAssociatedIconExW(long wstr long long) # 2k3:208, Vista:285
@ stdcall ExtractAssociatedIconW(long wstr ptr) # 2k3:216, Vista:286
@ stdcall ExtractIconA(long str long) # 2k3:217, Vista:287
@ stdcall ExtractIconEx(ptr long ptr ptr long) ExtractIconExA # 2k3:218, Vista:288
@ stdcall ExtractIconExA(str long ptr ptr long) # 2k3:219, Vista:289
@ stdcall ExtractIconExW(wstr long ptr ptr long) # 2k3:220, Vista:290
@ stdcall -version=0x502 ExtractIconResInfoA(ptr str long ptr ptr) # 2k3:221
@ stdcall -version=0x502 ExtractIconResInfoW(ptr wstr long ptr ptr) # 2k3:222
@ stdcall ExtractIconW(long wstr long) # 2k3:223, Vista:291
@ stdcall -version=0x502 ExtractVersionResource16W(wstr ptr) # 2k3:224
@ stdcall -version=0x502 FindExeDlgProc(ptr long ptr ptr) # 2k3:225
@ stdcall FindExecutableA(str str ptr) # 2k3:226, Vista:292
@ stdcall FindExecutableW(wstr wstr ptr) # 2k3:227, Vista:293
@ stdcall FreeIconList(long) # 2k3:228, Vista:294
@ stub -version=0x600+ InitNetworkAddressControl # Vista:295
@ stdcall InternalExtractIconListA(ptr str ptr) # 2k3:229, Vista:296
@ stdcall InternalExtractIconListW(ptr wstr ptr) # 2k3:238, Vista:297
@ stdcall Options_RunDLL(ptr ptr str long) # 2k3:255, Vista:298
@ stdcall Options_RunDLLA(ptr ptr str long) # 2k3:260, Vista:299
@ stdcall Options_RunDLLW(ptr ptr wstr long) # 2k3:261, Vista:300
@ stdcall PrintersGetCommand_RunDLL(ptr ptr wstr long) # 2k3:262, Vista:301
@ stdcall PrintersGetCommand_RunDLLA(ptr ptr str long) # 2k3:263, Vista:302
@ stdcall PrintersGetCommand_RunDLLW(ptr ptr wstr long) # 2k3:264, Vista:303
@ stdcall RealShellExecuteA(ptr str str str str ptr str ptr long ptr) # 2k3:265, Vista:304
@ stdcall RealShellExecuteExA(ptr str str str str ptr str ptr long ptr long) # 2k3:266, Vista:305
@ stdcall RealShellExecuteExW(ptr wstr wstr wstr wstr ptr wstr ptr long ptr long) # 2k3:267, Vista:306
@ stdcall RealShellExecuteW(ptr wstr wstr wstr wstr ptr wstr ptr long ptr) # 2k3:268, Vista:307
@ stdcall RegenerateUserEnvironment(ptr long) # 2k3:269, Vista:308
@ stub -version=0x600+ SHAddDefaultPropertiesByExt # Vista:309
@ stdcall SHAddToRecentDocs(long ptr) # 2k3:270, Vista:310
@ stdcall SHAppBarMessage(long ptr) # 2k3:271, Vista:311
@ stub -version=0x600+ SHAssocEnumHandlers # Vista:312
@ stub -version=0x600+ SHBindToFolderIDListParent # Vista:313
@ stub -version=0x600+ SHBindToFolderIDListParentEx # Vista:314
@ stdcall -version=0x600+ SHBindToObject(ptr ptr ptr ptr ptr) # Vista:315
@ stdcall SHBindToParent(ptr ptr ptr ptr) # 2k3:272, Vista:316
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA # 2k3:273, Vista:317
@ stdcall SHBrowseForFolderA(ptr) # 2k3:274, Vista:318
@ stdcall SHBrowseForFolderW(ptr) # 2k3:275, Vista:319
@ stdcall SHChangeNotify(long long ptr ptr) # 2k3:276, Vista:320
@ stub -version=0x600+ SHChangeNotifyRegisterThread # Vista:321
@ stdcall SHChangeNotifySuspendResume(long ptr long long) # 2k3:277, Vista:322
@ stub -version=0x600+ SHCreateAssociationRegistration # Vista:323
@ stdcall -version=0x600+ SHCreateDataObject(ptr long ptr ptr ptr ptr) # Vista:324
@ stdcall -version=0x600+ SHCreateDefaultContextMenu(ptr ptr ptr) # Vista:325
@ stdcall -version=0x600+ SHCreateDefaultExtractIcon(ptr ptr) # Vista:326
@ stub -version=0x600+ SHCreateDefaultPropertiesOp # Vista:327
@ stdcall SHCreateDirectoryExA(long str ptr) # 2k3:278, Vista:328
@ stdcall SHCreateDirectoryExW(long wstr ptr) # 2k3:279, Vista:329
@ stub -version=0x600+ SHCreateItemFromIDList # Vista:330
@ stub -version=0x600+ SHCreateItemFromParsingName # Vista:331
@ stub -version=0x600+ SHCreateItemFromRelativeName # Vista:332
@ stub -version=0x600+ SHCreateItemInKnownFolder # Vista:333
@ stub -version=0x600+ SHCreateItemWithParent # Vista:334
@ stub SHCreateLocalServerRunDll # 2k3:280, Vista:335
@ stdcall SHCreateProcessAsUserW(ptr) # 2k3:281, Vista:336
@ stdcall SHCreateQueryCancelAutoPlayMoniker(ptr) # 2k3:282, Vista:337
@ stdcall SHCreateShellItem(ptr ptr ptr ptr) # 2k3:283, Vista:338
@ stub -version=0x600+ SHCreateShellItemArray # Vista:339
@ stdcall -version=0x600+ SHCreateShellItemArrayFromDataObject(ptr ptr ptr) # Vista:340
@ stub -version=0x600+ SHCreateShellItemArrayFromIDLists # Vista:341
@ stub -version=0x600+ SHCreateShellItemArrayFromShellItem # Vista:342
@ stdcall SHEmptyRecycleBinA(long str long) # 2k3:284, Vista:343
@ stdcall SHEmptyRecycleBinW(long wstr long) # 2k3:285, Vista:344
@ stub SHEnableServiceObject # 2k3:286, Vista:345
@ stdcall SHEnumerateUnreadMailAccountsW(ptr long ptr long) # 2k3:287, Vista:346
@ stub -version=0x600+ SHEvaluateSystemCommandTemplate # Vista:347
@ stdcall SHExtractIconsW(wstr long long long ptr ptr long long) user32.PrivateExtractIconsW # 2k3:288, Vista:348
@ stdcall SHFileOperation(ptr) SHFileOperationA # 2k3:289, Vista:349
@ stdcall SHFileOperationA(ptr) # 2k3:290, Vista:350
@ stdcall SHFileOperationW(ptr) # 2k3:291, Vista:351
@ stdcall SHFormatDrive(long long long long) # 2k3:292, Vista:352
@ stdcall SHFreeNameMappings(ptr) # 2k3:293, Vista:353
@ stdcall SHGetDataFromIDListA(ptr ptr long ptr long) # 2k3:294, Vista:354
@ stdcall SHGetDataFromIDListW(ptr ptr long ptr long) # 2k3:295, Vista:355
@ stdcall SHGetDesktopFolder(ptr) # 2k3:296, Vista:356
@ stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA # 2k3:297, Vista:357
@ stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA # 2k3:298, Vista:358
@ stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW # 2k3:299, Vista:359
@ stub -version=0x600+ SHGetDriveMedia # Vista:360
@ stdcall SHGetFileInfo(ptr long ptr long long) SHGetFileInfoA # 2k3:300, Vista:361
@ stdcall SHGetFileInfoA(ptr long ptr long long) # 2k3:301, Vista:362
@ stdcall SHGetFileInfoW(ptr long ptr long long) # 2k3:302, Vista:363
@ stdcall SHGetFolderLocation(long long long long ptr) # 2k3:303, Vista:364
@ stdcall SHGetFolderPathA(long long long long ptr) # 2k3:304, Vista:365
@ stdcall SHGetFolderPathAndSubDirA(long long long long str ptr) # 2k3:305, Vista:366
@ stdcall SHGetFolderPathAndSubDirW(long long long long wstr ptr) # 2k3:306, Vista:367
@ stub -version=0x600+ SHGetFolderPathEx # Vista:368
@ stdcall SHGetFolderPathW(long long long long ptr) # 2k3:307, Vista:369
@ stdcall -version=0x600+ SHGetIDListFromObject(ptr ptr) # Vista:370
@ stdcall SHGetIconOverlayIndexA(str long) # 2k3:308, Vista:371
@ stdcall SHGetIconOverlayIndexW(wstr long) # 2k3:309, Vista:372
@ stdcall SHGetInstanceExplorer(long) # 2k3:310, Vista:373
@ stub -version=0x600+ SHGetKnownFolderIDList # Vista:374
@ stub -version=0x600+ SHGetKnownFolderPath # Vista:375
@ stub -version=0x600+ SHGetLocalizedName # Vista:376
@ stdcall SHGetMalloc(ptr) # 2k3:311, Vista:377
@ stdcall -version=0x600+ SHGetNameFromIDList(ptr long ptr) # Vista:378
@ stdcall SHGetNewLinkInfo(str str ptr long long) SHGetNewLinkInfoA # 2k3:312, Vista:379
@ stdcall SHGetPathFromIDList(ptr ptr) SHGetPathFromIDListA # 2k3:313, Vista:380
@ stdcall SHGetPathFromIDListA(ptr ptr) # 2k3:314, Vista:381
@ stub -version=0x600+ SHGetPathFromIDListEx # Vista:382
@ stdcall SHGetPathFromIDListW(ptr ptr) # 2k3:315, Vista:383
@ stub -version=0x600+ SHGetPropertyStoreFromIDList # Vista:384
@ stub -version=0x600+ SHGetPropertyStoreFromParsingName # Vista:385
@ stdcall SHGetSettings(ptr long) # 2k3:316, Vista:386
@ stdcall SHGetSpecialFolderLocation(long long ptr) # 2k3:317, Vista:387
@ stdcall SHGetSpecialFolderPathA(long ptr long long) # 2k3:318, Vista:388
@ stdcall SHGetSpecialFolderPathW(long ptr long long) # 2k3:319, Vista:389
@ stub -version=0x600+ SHGetStockIconInfo # Vista:390
@ stub -version=0x600+ SHGetTemporaryPropertyForItem # Vista:391
@ stdcall SHGetUnreadMailCountW(ptr wstr ptr ptr ptr long) # 2k3:320, Vista:392
@ stdcall -version=0x502 SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA # 2k3:321
@ stdcall -version=0x502 SHHelpShortcuts_RunDLLA(long long long long) # 2k3:322
@ stdcall -version=0x502 SHHelpShortcuts_RunDLLW(long long long long) # 2k3:323
@ stdcall SHInvokePrinterCommandA(ptr long str str long) # 2k3:324, Vista:393
@ stdcall SHInvokePrinterCommandW(ptr long wstr wstr long) # 2k3:325, Vista:394
@ stdcall SHIsFileAvailableOffline(wstr ptr) # 2k3:326, Vista:395
@ stdcall SHLoadInProc(long) # 2k3:327, Vista:396
@ stdcall SHLoadNonloadedIconOverlayIdentifiers() # 2k3:328, Vista:397
@ stdcall SHOpenFolderAndSelectItems(ptr long ptr long) # 2k3:329, Vista:398
@ stdcall -version=0x600+ SHOpenWithDialog(ptr ptr) # Vista:399
@ stdcall SHParseDisplayName(wstr ptr ptr long ptr) # 2k3:330, Vista:400
@ stdcall SHPathPrepareForWriteA(long ptr str long) # 2k3:331, Vista:401
@ stdcall SHPathPrepareForWriteW(long ptr wstr long) # 2k3:332, Vista:402
@ stdcall SHQueryRecycleBinA(str ptr) # 2k3:333, Vista:403
@ stdcall SHQueryRecycleBinW(wstr ptr) # 2k3:334, Vista:404
@ stub -version=0x600+ SHQueryUserNotificationState # Vista:405
@ stub -version=0x600+ SHRemoveLocalizedName # Vista:406
@ stub -version=0x600+ SHSetDefaultProperties # Vista:407
@ stub -version=0x600+ SHSetKnownFolderPath # Vista:408
@ stdcall SHSetLocalizedName(wstr wstr long) # 2k3:335, Vista:409
@ stub -version=0x600+ SHSetTemporaryPropertyForItem # Vista:410
@ stdcall SHSetUnreadMailCountW (wstr long wstr) # 2k3:336, Vista:411
@ stdcall SHUpdateRecycleBinIcon() # 2k3:337, Vista:412
@ stdcall SheChangeDirA(str) # 2k3:338, Vista:413
@ stdcall -version=0x502 SheChangeDirExA(str) # 2k3:339
@ stdcall SheChangeDirExW(wstr) # 2k3:340, Vista:414
@ stdcall -version=0x502 SheChangeDirW(wstr) # 2k3:341
@ stdcall -version=0x502 SheConvertPathW(wstr wstr long) # 2k3:342
@ stdcall -version=0x502 SheFullPathA(str long str) # 2k3:343
@ stdcall -version=0x502 SheFullPathW(wstr long wstr) # 2k3:344
@ stdcall -version=0x502 SheGetCurDrive() # 2k3:345
@ stdcall SheGetDirA(long long) # 2k3:346, Vista:415
@ stdcall -version=0x502 SheGetDirExW(wstr ptr wstr) # 2k3:347
@ stdcall -version=0x502 SheGetDirW(long long) # 2k3:348
@ stdcall -version=0x502 SheGetPathOffsetW(wstr) # 2k3:349
@ stdcall -version=0x502 SheRemoveQuotesA(str) # 2k3:350
@ stdcall -version=0x502 SheRemoveQuotesW(wstr) # 2k3:351
@ stdcall SheSetCurDrive(long) # 2k3:352, Vista:416
@ stdcall -version=0x502 SheShortenPathA(str long) # 2k3:353
@ stdcall -version=0x502 SheShortenPathW(wstr long) # 2k3:354
@ stdcall ShellAboutA(long str str long) # 2k3:355, Vista:417
@ stdcall ShellAboutW(long wstr wstr long) # 2k3:356, Vista:418
@ stdcall ShellExec_RunDLL(ptr ptr str long) ShellExec_RunDLLA # 2k3:357, Vista:419
@ stdcall ShellExec_RunDLLA(ptr ptr str long) # 2k3:358, Vista:420
@ stdcall ShellExec_RunDLLW(ptr ptr wstr long) # 2k3:359, Vista:421
@ stdcall ShellExecuteA(long str str str str long) # 2k3:360, Vista:422
@ stdcall ShellExecuteEx(long) ShellExecuteExA # 2k3:361, Vista:423
@ stdcall ShellExecuteExA (long) # 2k3:362, Vista:424
@ stdcall ShellExecuteExW (long) # 2k3:363, Vista:425
@ stdcall ShellExecuteW(long wstr wstr wstr wstr long) # 2k3:364, Vista:426
@ stdcall ShellHookProc(long ptr ptr) # 2k3:365, Vista:427
@ stdcall -version=0x600+ Shell_GetCachedImageIndexA(str long long) # Vista:428
@ stdcall -version=0x600+ Shell_GetCachedImageIndexW(wstr long long) # Vista:429
@ stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA # 2k3:366, Vista:430
@ stdcall Shell_NotifyIconA(long ptr) # 2k3:367, Vista:431
@ stdcall Shell_NotifyIconW(long ptr) # 2k3:368, Vista:432
@ stdcall StrChrA(str long) shlwapi.StrChrA # 2k3:369, Vista:433
@ stdcall StrChrIA(str long) shlwapi.StrChrIA # 2k3:370, Vista:434
@ stdcall StrChrIW(wstr long) shlwapi.StrChrIW # 2k3:371, Vista:435
@ stdcall StrChrW(wstr long) shlwapi.StrChrW # 2k3:372, Vista:436
@ stdcall StrCmpNA(str str long) shlwapi.StrCmpNA # 2k3:373, Vista:437
@ stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA # 2k3:374, Vista:438
@ stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW # 2k3:375, Vista:439
@ stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW # 2k3:376, Vista:440
@ stdcall -version=0x502 StrCpyNA (ptr str long) kernel32.lstrcpynA # 2k3:377
@ stdcall -version=0x502 StrCpyNW(wstr wstr long) shlwapi.StrCpyNW # 2k3:378
@ stdcall StrNCmpA(str str long) shlwapi.StrCmpNA # 2k3:379, Vista:441
@ stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA # 2k3:380, Vista:442
@ stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW # 2k3:381, Vista:443
@ stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW # 2k3:382, Vista:444
@ stdcall -version=0x502 StrNCpyA (ptr str long) kernel32.lstrcpynA # 2k3:383
@ stdcall -version=0x502 StrNCpyW(wstr wstr long) shlwapi.StrCpyNW # 2k3:384
@ stdcall StrRChrA(str str long) shlwapi.StrRChrA # 2k3:385, Vista:445
@ stdcall StrRChrIA(str str long) shlwapi.StrRChrIA # 2k3:386, Vista:446
@ stdcall StrRChrIW(wstr wstr long) shlwapi.StrRChrIW # 2k3:387, Vista:447
@ stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW # 2k3:388, Vista:448
@ stdcall StrRStrA(str str str) # 2k3:389, Vista:449
@ stdcall StrRStrIA(str str str) shlwapi.StrRStrIA # 2k3:390, Vista:450
@ stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW # 2k3:391, Vista:451
@ stdcall StrRStrW(wstr wstr wstr) # 2k3:392, Vista:452
@ stdcall StrStrA(str str) shlwapi.StrStrA # 2k3:393, Vista:453
@ stdcall StrStrIA(str str) shlwapi.StrStrIA # 2k3:394, Vista:454
@ stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW # 2k3:395, Vista:455
@ stdcall StrStrW(wstr wstr) shlwapi.StrStrW # 2k3:396, Vista:456
@ stdcall WOWShellExecute(ptr str str str str long ptr) # 2k3:397, Vista:457
@ stub -version=0x600+ WaitForExplorerRestartW # Vista:458
