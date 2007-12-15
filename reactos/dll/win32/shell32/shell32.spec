# Functions exported by the Win95 shell32.dll
# (these need to have these exact ordinals, for some
#  win95 and winNT dlls import shell32.dll by ordinal)
# This list was updated to dll version 4.72

   2 stdcall SHChangeNotifyRegister(long long long long long ptr)
   3 stdcall SHDefExtractIconA(str long long ptr ptr long)
   4 stdcall SHChangeNotifyDeregister(long)
   5 stdcall -noname SHChangeNotifyUpdateEntryList(long long long long)
   6 stdcall SHDefExtractIconW(wstr long long ptr ptr long)
   9 stub PifMgr_OpenProperties
  10 stub PifMgr_GetProperties
  11 stub PifMgr_SetProperties
  13 stub PifMgr_CloseProperties
  15 stdcall -noname ILGetDisplayName(ptr ptr)
  16 stdcall ILFindLastID(ptr)
  17 stdcall ILRemoveLastID(ptr)
  18 stdcall ILClone(ptr)
  19 stdcall ILCloneFirst(ptr)
  20 stdcall -noname ILGlobalClone(ptr)
  21 stdcall ILIsEqual(ptr ptr)
  23 stdcall ILIsParent(ptr ptr long)
  24 stdcall ILFindChild(ptr ptr)
  25 stdcall ILCombine(ptr ptr)
  26 stdcall ILLoadFromStream(ptr ptr)
  27 stdcall ILSaveToStream(ptr ptr)
  28 stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
  29 stdcall -noname PathIsRoot(ptr) PathIsRootAW
  30 stdcall -noname PathBuildRoot(ptr long) PathBuildRootAW
  31 stdcall -noname PathFindExtension(ptr) PathFindExtensionAW
  32 stdcall -noname PathAddBackslash(ptr) PathAddBackslashAW
  33 stdcall -noname PathRemoveBlanks(ptr) PathRemoveBlanksAW
  34 stdcall -noname PathFindFileName(ptr) PathFindFileNameAW
  35 stdcall -noname PathRemoveFileSpec(ptr) PathRemoveFileSpecAW
  36 stdcall -noname PathAppend(ptr ptr) PathAppendAW
  37 stdcall -noname PathCombine(ptr ptr ptr) PathCombineAW
  38 stdcall -noname PathStripPath(ptr)PathStripPathAW
  39 stdcall -noname PathIsUNC(ptr) PathIsUNCAW
  40 stdcall -noname PathIsRelative(ptr) PathIsRelativeAW
  41 stdcall IsLFNDriveA(str)
  42 stdcall IsLFNDriveW(wstr)
  43 stdcall PathIsExe(ptr) PathIsExeAW
  45 stdcall -noname PathFileExists(ptr) PathFileExistsAW
  46 stdcall -noname PathMatchSpec(ptr ptr) PathMatchSpecAW
  47 stdcall PathMakeUniqueName(ptr long ptr ptr ptr)PathMakeUniqueNameAW
  48 stdcall -noname PathSetDlgItemPath(long long ptr) PathSetDlgItemPathAW
  49 stdcall PathQualify(ptr) PathQualifyAW
  50 stdcall -noname PathStripToRoot(ptr) PathStripToRootAW
  51 stdcall PathResolve(str long long) PathResolveAW
  52 stdcall -noname PathGetArgs(str) PathGetArgsAW
  53 stdcall -noname DoEnvironmentSubst(long long) DoEnvironmentSubstAW
  54 stdcall -noname LogoffWindowsDialog(ptr)
  55 stdcall -noname PathQuoteSpaces(ptr) PathQuoteSpacesAW
  56 stdcall -noname PathUnquoteSpaces(str) PathUnquoteSpacesAW
  57 stdcall -noname PathGetDriveNumber(str) PathGetDriveNumberAW
  58 stdcall -noname ParseField(str long ptr long) ParseFieldAW
  59 stdcall RestartDialog(long wstr long)
  60 stdcall -noname ExitWindowsDialog(long)
  61 stdcall -noname RunFileDlg(long long long str str long)
  62 stdcall PickIconDlg(long long long long)
  63 stdcall GetFileNameFromBrowse(long long long long str str str)
  64 stdcall DriveType(long)
  65 stdcall -noname InvalidateDriveType(long)
  66 stdcall IsNetDrive(long)
  67 stdcall Shell_MergeMenus(long long long long long long)
  68 stdcall SHGetSetSettings(ptr long long)
  69 stub -noname SHGetNetResource
  70 stdcall -noname SHCreateDefClassObject(long long long long long)
  71 stdcall Shell_GetImageList(ptr ptr)
  72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
  73 stdcall SHShellFolderView_Message(long long long)
  74 stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
  75 stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
  76 stub -noname DragQueryInfo
  77 stdcall SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
  78 stdcall -noname OleStrToStrN(str long wstr long) OleStrToStrNAW
  79 stdcall -noname StrToOleStrN(wstr long str long) StrToOleStrNAW
  83 stdcall -noname CIDLData_CreateFromIDArray(ptr long ptr ptr)
  84 stub -noname SHIsBadInterfacePtr
  85 stdcall OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
  86 stdcall -noname SHRegisterDragDrop(long ptr)
  87 stdcall -noname SHRevokeDragDrop(long)
  88 stdcall SHDoDragDrop(long ptr ptr long ptr)
  89 stdcall SHCloneSpecialIDList(long long long)
  90 stdcall SHFindFiles(ptr ptr)
  91 stub -noname SHFindComputer
  92 stdcall PathGetShortPath(ptr) PathGetShortPathAW
  93 stdcall -noname Win32CreateDirectory(wstr ptr) Win32CreateDirectoryAW
  94 stdcall -noname Win32RemoveDirectory(wstr) Win32RemoveDirectoryAW
  95 stdcall -noname SHLogILFromFSIL(ptr)
  96 stdcall -noname StrRetToStrN(ptr long ptr ptr) StrRetToStrNAW
  97 stdcall -noname SHWaitForFileToOpen (long long long)
  98 stdcall SHGetRealIDL(ptr ptr ptr)
  99 stdcall -noname SetAppStartingCursor(long long)
 100 stdcall -noname SHRestricted(long)

 102 stdcall SHCoCreateInstance(wstr ptr long ptr ptr)
 103 stdcall SignalFileOpen(long)
 104 stdcall -noname FileMenu_DeleteAllItems(long)
 105 stdcall -noname FileMenu_DrawItem(long ptr)
 106 stdcall -noname FileMenu_FindSubMenuByPidl(long ptr)
 107 stdcall -noname FileMenu_GetLastSelectedItemPidls(long ptr ptr)
 108 stdcall -noname FileMenu_HandleMenuChar(long long)
 109 stdcall -noname FileMenu_InitMenuPopup(long)
 110 stdcall -noname FileMenu_InsertUsingPidl (long long ptr long long ptr)
 111 stdcall -noname FileMenu_Invalidate(long)
 112 stdcall -noname FileMenu_MeasureItem(long ptr)
 113 stdcall -noname FileMenu_ReplaceUsingPidl(long long ptr long ptr)
 114 stdcall -noname FileMenu_Create(long long long long long)
 115 stdcall -noname FileMenu_AppendItem(long ptr long long long long) FileMenu_AppendItemAW
 116 stdcall -noname FileMenu_TrackPopupMenuEx(long long long long long long)
 117 stdcall -noname FileMenu_DeleteItemByCmd(long long)
 118 stdcall -noname FileMenu_Destroy(long)
 119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
 120 stdcall -noname FileMenu_AbortInitMenu()
 121 stdcall SHFlushClipboard()
 122 stdcall -noname RunDLL_CallEntry16(long long long str long) #name wrong?
 123 stdcall -noname SHFreeUnusedLibraries()
 124 stdcall -noname FileMenu_AppendFilesForPidl(long ptr long)
 125 stdcall -noname FileMenu_AddFilesForPidl(long long long ptr long long ptr)
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
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stdcall -noname FileMenu_DeleteItemByIndex(long long)
 141 stdcall -noname FileMenu_DeleteItemByFirstID(long long)
 142 stdcall -noname FileMenu_DeleteSeparator(long)
 143 stdcall -noname FileMenu_EnableItemByCmd(long long long)
 144 stdcall -noname FileMenu_GetItemExtent(long long)
 145 stdcall -noname PathFindOnPath(ptr ptr) PathFindOnPathAW
 146 stdcall -noname RLBuildListOfPaths()
 147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
 148 stdcall SHMapIDListToImageListIndexAsync(ptr ptr ptr long ptr ptr ptr ptr ptr)
 149 stdcall SHFind_InitMenuPopup(long long long long)

 151 stdcall SHLoadOLE(long)
 152 stdcall ILGetSize(ptr)
 153 stdcall ILGetNext(ptr)
 154 stdcall ILAppend(long long long)
 155 stdcall ILFree(ptr)
 156 stdcall -noname ILGlobalFree(ptr)
 157 stdcall ILCreateFromPath(ptr) ILCreateFromPathAW
 158 stdcall -noname PathGetExtension(str long long) PathGetExtensionAW
 159 stdcall -noname PathIsDirectory(ptr) PathIsDirectoryAW
 160 stub -noname SHNetConnectionDialog
 161 stdcall SHRunControlPanel(long long)
 162 stdcall SHSimpleIDListFromPath(ptr) SHSimpleIDListFromPathAW
 163 stdcall -noname StrToOleStr(wstr str) StrToOleStrAW
 164 stdcall Win32DeleteFile(str) Win32DeleteFileAW
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
 175 stdcall -noname SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathAW
 176 stdcall SHSetInstanceExplorer(long)
 177 stub -noname DAD_SetDragImageFromListView
 178 stdcall SHObjectProperties(long long wstr wstr)
 179 stdcall SHGetNewLinkInfoA(str str ptr long long)
 180 stdcall SHGetNewLinkInfoW(wstr wstr ptr long long)
 181 stdcall -noname RegisterShellHook(long long)
 182 varargs ShellMessageBoxW(long long wstr wstr long)
 183 varargs ShellMessageBoxA(long long str str long)
 184 stdcall -noname ArrangeWindows(long long long long long)
 185 stub -noname SHHandleDiskFull
 186 stdcall -noname ILGetDisplayNameEx(ptr ptr ptr long)
 187 stub -noname ILGetPseudoNameW
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
 199 stub SHGetFileIcon
 200 stub SHLocalAlloc
 201 stub SHLocalFree
 202 stub SHLocalReAlloc
 203 stub AddCommasW
 204 stub ShortSizeFormatW
 205 stdcall Printer_LoadIconsW(wstr ptr ptr)
 206 stub Link_AddExtraDataSection
 207 stub Link_ReadExtraDataSection
 208 stub Link_RemoveExtraDataSection
 209 stub Int64ToString
 210 stub LargeIntegerToString
 211 stub Printers_GetPidl
 212 stub Printers_AddPrinterPropPages
 213 stdcall Printers_RegisterWindowW(wstr long ptr ptr)
 214 stdcall Printers_UnregisterWindow(long long)
 215 stdcall -noname SHStartNetConnectionDialog(long str long)
 238 stdcall Options_RunDLL(ptr ptr str long)
 243 stdcall @(long long) shell32_243
 244 stdcall -noname SHInitRestricted(ptr ptr)
 249 stdcall -noname PathParseIconLocation(ptr) PathParseIconLocationAW
 250 stdcall -noname PathRemoveExtension(ptr) PathRemoveExtensionAW
 251 stdcall -noname PathRemoveArgs(ptr) PathRemoveArgsAW
 255 stdcall Options_RunDLLA(ptr ptr str long)
 256 stdcall @(ptr ptr) SHELL32_256
 258 stdcall -noname LinkWindow_RegisterClass()
 259 stdcall -noname LinkWindow_UnregisterClass()
 260 stdcall Options_RunDLLW(ptr ptr wstr long)
#299 stub Shl1632_ThunkData32
#300 stub Shl3216_ThunkData32

 505 stdcall SHRegCloseKey (long)
 506 stdcall SHRegOpenKeyA (long str long)
 507 stdcall SHRegOpenKeyW (long wstr long)
 508 stdcall SHRegQueryValueA(long str ptr ptr)
 509 stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr)
 510 stdcall SHRegQueryValueW (long long long long)
 511 stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr)
 512 stdcall SHRegDeleteKeyW (long wstr)

 520 stdcall SHAllocShared(ptr long long)
 521 stdcall SHLockShared(long long)
 522 stdcall SHUnlockShared(ptr)
 523 stdcall SHFreeShared(long long)
 524 stdcall RealDriveType(long long)
 525 stub RealDriveTypeFlags

 640 stdcall -noname NTSHChangeNotifyRegister(long long long long long long)
 641 stdcall -noname NTSHChangeNotifyDeregister(long)

 643 stub SHChangeNotifyReceive
 644 stdcall SHChangeNotification_Lock(long long ptr ptr)
 645 stdcall SHChangeNotification_Unlock(long)
 646 stub SHChangeRegistrationReceive
 647 stub ReceiveAddToRecentDocs
 648 stub SHWaitOp_Operate

 650 stdcall -noname PathIsSameRoot(ptr ptr) PathIsSameRootAW

 651 stdcall -noname ReadCabinetState(long long) # OldReadCabinetState
 652 stdcall WriteCabinetState(long)
 653 stdcall PathProcessCommand(long long long long) PathProcessCommandAW
 654 stdcall @(long long) shell32_654 # ReadCabinetState@8

 660 stdcall -noname FileIconInit(long)
 680 stdcall IsUserAnAdmin()
 701 stdcall CDefFolderMenu_Create2(ptr ptr long ptr ptr ptr long ptr ptr)
 714 stdcall @(ptr) SHELL32_714 # PathIsTemporaryW
 730 stdcall RestartDialogEx(long wstr long long)

1217 stub FOOBAR1217   # no joke! This is the real name!!

@ stdcall CheckEscapesA(str long)
@ stdcall CheckEscapesW(wstr long)
@ stdcall CommandLineToArgvW(wstr ptr)
@ stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
@ stdcall Control_FillCache_RunDLLA(long long long long)
@ stdcall Control_FillCache_RunDLLW(long long long long)
@ stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
@ stdcall Control_RunDLLA(ptr ptr str long)
@ stub Control_RunDLLAsUserW
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
@ stub DragQueryFileAorW
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
@ stub ExtractIconResInfoA
@ stub ExtractIconResInfoW
@ stdcall ExtractIconW(long wstr long)
@ stub ExtractVersionResource16W
@ stub FindExeDlgProc
@ stdcall FindExecutableA(str str ptr)
@ stdcall FindExecutableW(wstr wstr ptr)
@ stub FixupOptionalComponents
@ stdcall FreeIconList(long)
@ stub InternalExtractIconListA
@ stub InternalExtractIconListW
@ stub OCInstall
@ stdcall OpenAs_RunDLL(long long str long) OpenAs_RunDLLA
@ stdcall OpenAs_RunDLLA(long long str long)
@ stdcall OpenAs_RunDLLW(long long wstr long)
@ stub PrintersGetCommand_RunDLL
@ stub PrintersGetCommand_RunDLLA
@ stub PrintersGetCommand_RunDLLW
@ stub RealShellExecuteA
@ stub RealShellExecuteExA
@ stub RealShellExecuteExW
@ stub RealShellExecuteW
@ stub RegenerateUserEnvironment
@ stdcall SHAddToRecentDocs (long ptr)
@ stdcall SHAppBarMessage(long ptr)
@ stdcall SHBindToParent(ptr ptr ptr ptr)
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
@ stdcall SHBrowseForFolderA(ptr)
@ stdcall SHBrowseForFolderW(ptr)
@ stdcall SHChangeNotify (long long ptr ptr)
@ stub SHChangeNotifySuspendResume
@ stdcall SHCreateDefaultContextMenu(ptr ptr ptr)
@ stdcall SHCreateDefaultExtractIcon(ptr ptr)
@ stdcall SHCreateDirectoryExA(long str ptr)
@ stdcall SHCreateDirectoryExW(long wstr ptr)
@ stub SHCreateProcessAsUserW
@ stdcall SHEmptyRecycleBinA(long str long)
@ stdcall SHEmptyRecycleBinW(long wstr long)
@ stub SHExtractIconsW
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
@ stdcall SHGetFolderPathW(long long long long ptr)
@ stub SHGetFreeDiskSpace
@ stub SHGetIconOverlayIndexA
@ stub SHGetIconOverlayIndexW
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
@ stdcall SHHelpShortcuts_RunDLL(long long long long) SHHelpShortcuts_RunDLLA
@ stdcall SHHelpShortcuts_RunDLLA(long long long long)
@ stdcall SHHelpShortcuts_RunDLLW(long long long long)
@ stub SHInvokePrinterCommandA
@ stub SHInvokePrinterCommandW
@ stdcall SHIsFileAvailableOffline(wstr ptr)
@ stdcall SHLoadInProc(long)
@ stdcall SHLoadNonloadedIconOverlayIdentifiers()
@ stdcall SHPathPrepareForWriteA(long ptr str long)
@ stdcall SHPathPrepareForWriteW(long ptr wstr long)
@ stdcall SHQueryRecycleBinA(str ptr)
@ stdcall SHQueryRecycleBinW(wstr ptr)
@ stdcall SHSetLocalizedName(wstr wstr long)
@ stub SHUpdateRecycleBinIcon
@ stdcall SheChangeDirA(str)
@ stub SheChangeDirExA
@ stub SheChangeDirExW
@ stdcall SheChangeDirW(wstr)
@ stub SheConvertPathW
@ stub SheFullPathA
@ stub SheFullPathW
@ stub SheGetCurDrive
@ stdcall SheGetDirA(long long)
@ stub SheGetDirExW
@ stdcall SheGetDirW (long long)
@ stub SheGetPathOffsetW
@ stub SheRemoveQuotesA
@ stub SheRemoveQuotesW
@ stub SheSetCurDrive
@ stub SheShortenPathA
@ stub SheShortenPathW
@ stdcall ShellAboutA(long str str long)
@ stdcall ShellAboutW(long wstr wstr long)
@ stub ShellExec_RunDLL
@ stub ShellExec_RunDLLA
@ stub ShellExec_RunDLLW
@ stdcall ShellExecuteA(long str str str str long)
@ stdcall ShellExecuteEx (long) ShellExecuteExA
@ stdcall ShellExecuteExA (long)
@ stdcall ShellExecuteExW (long)
@ stdcall ShellExecuteW (long wstr wstr wstr wstr long)
@ stub ShellHookProc
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
@ stdcall StrRChrIW(str str long) shlwapi.StrRChrIW
@ stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
@ stub StrRStrA
@ stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
@ stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
@ stub StrRStrW
@ stdcall StrStrA(str str) shlwapi.StrStrA
@ stdcall StrStrIA(str str) shlwapi.StrStrIA
@ stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
@ stdcall StrStrW(wstr wstr) shlwapi.StrStrW
@ stub WOWShellExecute
