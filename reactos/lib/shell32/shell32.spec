# Functions exported by the Win95 shell32.dll
# (these need to have these exact ordinals, for some
#  win95 and winNT dlls import shell32.dll by ordinal)
# This list was updated to dll version 4.72

   2 stdcall SHChangeNotifyRegister(long long long long long ptr)
   4 stdcall SHChangeNotifyDeregister (long)
   5 stdcall SHChangeNotifyUpdateEntryList (long long long long)
   9 stub PifMgr_OpenProperties
  10 stub PifMgr_GetProperties
  11 stub PifMgr_SetProperties
  13 stub PifMgr_CloseProperties
  15 stdcall ILGetDisplayName(ptr ptr)
  16 stdcall ILFindLastID(ptr)
  17 stdcall ILRemoveLastID(ptr)
  18 stdcall ILClone(ptr)
  19 stdcall ILCloneFirst (ptr)
  20 stdcall ILGlobalClone (ptr)
  21 stdcall ILIsEqual (ptr ptr)
  23 stdcall ILIsParent (ptr ptr long)
  24 stdcall ILFindChild (ptr ptr)
  25 stdcall ILCombine(ptr ptr)
  26 stdcall ILLoadFromStream (ptr ptr)
  27 stdcall ILSaveToStream(ptr ptr)
  28 stdcall SHILCreateFromPath(ptr ptr ptr) SHILCreateFromPathAW
  29 stdcall PathIsRoot(ptr) PathIsRootAW
  30 stdcall PathBuildRoot(ptr long) PathBuildRootAW
  31 stdcall PathFindExtension(ptr) PathFindExtensionAW
  32 stdcall PathAddBackslash(ptr) PathAddBackslashAW
  33 stdcall PathRemoveBlanks(ptr) PathRemoveBlanksAW
  34 stdcall PathFindFileName(ptr) PathFindFileNameAW
  35 stdcall PathRemoveFileSpec(ptr) PathRemoveFileSpecAW
  36 stdcall PathAppend(ptr ptr) PathAppendAW
  37 stdcall PathCombine(ptr ptr ptr) PathCombineAW
  38 stdcall PathStripPath(ptr)PathStripPathAW
  39 stdcall PathIsUNC (ptr) PathIsUNCAW
  40 stdcall PathIsRelative (ptr) PathIsRelativeAW
  41 stdcall IsLFNDriveA(str)
  42 stdcall IsLFNDriveW(wstr)
  43 stdcall PathIsExe (ptr) PathIsExeAW
  45 stdcall PathFileExists(ptr) PathFileExistsAW
  46 stdcall PathMatchSpec (ptr ptr) PathMatchSpecAW
  47 stdcall PathMakeUniqueName (ptr long ptr ptr ptr)PathMakeUniqueNameAW
  48 stdcall PathSetDlgItemPath (long long ptr) PathSetDlgItemPathAW
  49 stdcall PathQualify (ptr) PathQualifyAW
  50 stdcall PathStripToRoot (ptr) PathStripToRootAW
  51 stdcall PathResolve(str long long) PathResolveAW
  52 stdcall PathGetArgs(str) PathGetArgsAW
  53 stdcall DoEnvironmentSubst (long long) DoEnvironmentSubstAW
  54 stdcall DragAcceptFiles(long long)
  55 stdcall PathQuoteSpaces (ptr) PathQuoteSpacesAW
  56 stdcall PathUnquoteSpaces(str) PathUnquoteSpacesAW
  57 stdcall PathGetDriveNumber (str) PathGetDriveNumberAW
  58 stdcall ParseField(str long ptr long) ParseFieldAW
  59 stdcall RestartDialog(long long long)
  60 stdcall ExitWindowsDialog(long)
  61 stdcall RunFileDlg(long long long str str long)
  62 stdcall PickIconDlg(long long long long)
  63 stdcall GetFileNameFromBrowse(long long long long str str str)
  64 stdcall DriveType (long)
  65 stub InvalidateDriveType
  66 stdcall IsNetDrive(long)
  67 stdcall Shell_MergeMenus (long long long long long long)
  68 stdcall SHGetSetSettings(ptr long long)
  69 stub SHGetNetResource
  70 stdcall SHCreateDefClassObject(long long long long long)
  71 stdcall Shell_GetImageList(ptr ptr)
  72 stdcall Shell_GetCachedImageIndex(ptr ptr long) Shell_GetCachedImageIndexAW
  73 stdcall SHShellFolderView_Message(long long long)
  74 stdcall SHCreateStdEnumFmtEtc(long ptr ptr)
  75 stdcall PathYetAnotherMakeUniqueName(ptr wstr wstr wstr)
  76 stub DragQueryInfo
  77 stdcall SHMapPIDLToSystemImageListIndex(ptr ptr ptr)
  78 stdcall OleStrToStrN(str long wstr long) OleStrToStrNAW
  79 stdcall StrToOleStrN(wstr long str long) StrToOleStrNAW
  80 stdcall DragFinish(long)
  81 stdcall DragQueryFile(long long ptr long) DragQueryFileA
  82 stdcall DragQueryFileA(long long ptr long)
  83 stdcall CIDLData_CreateFromIDArray(ptr long ptr ptr)
  84 stub SHIsBadInterfacePtr
  85 stdcall OpenRegStream(long str str long) shlwapi.SHOpenRegStreamA
  86 stdcall SHRegisterDragDrop(long ptr)
  87 stdcall SHRevokeDragDrop(long)
  88 stdcall SHDoDragDrop(long ptr ptr long ptr)
  89 stdcall SHCloneSpecialIDList(long long long)
  90 stub SHFindFiles
  91 stub SHFindComputer
  92 stdcall PathGetShortPath (ptr) PathGetShortPathAW
  93 stdcall Win32CreateDirectory(wstr ptr) Win32CreateDirectoryAW
  94 stdcall Win32RemoveDirectory(wstr) Win32RemoveDirectoryAW
  95 stdcall SHLogILFromFSIL (ptr)
  96 stdcall StrRetToStrN (ptr long ptr ptr) StrRetToStrNAW
  97 stdcall SHWaitForFileToOpen (long long long)
  98 stdcall SHGetRealIDL (ptr ptr ptr)
  99 stdcall SetAppStartingCursor (long long)
 100 stdcall SHRestricted(long)

 102 stdcall SHCoCreateInstance(wstr ptr long ptr ptr)
 103 stdcall SignalFileOpen(long)
 104 stdcall FileMenu_DeleteAllItems(long)
 105 stdcall FileMenu_DrawItem(long ptr)
 106 stdcall FileMenu_FindSubMenuByPidl(long ptr)
 107 stdcall FileMenu_GetLastSelectedItemPidls(long ptr ptr)
 108 stdcall FileMenu_HandleMenuChar(long long)
 109 stdcall FileMenu_InitMenuPopup (long)
 110 stdcall FileMenu_InsertUsingPidl (long long ptr long long ptr)
 111 stdcall FileMenu_Invalidate (long)
 112 stdcall FileMenu_MeasureItem(long ptr)
 113 stdcall FileMenu_ReplaceUsingPidl (long long ptr long ptr)
 114 stdcall FileMenu_Create (long long long long long)
 115 stdcall FileMenu_AppendItem (long ptr long long long long) FileMenu_AppendItemAW
 116 stdcall FileMenu_TrackPopupMenuEx (long long long long long long)
 117 stdcall FileMenu_DeleteItemByCmd(long long)
 118 stdcall FileMenu_Destroy (long)
 119 stdcall IsLFNDrive(ptr) IsLFNDriveAW
 120 stdcall FileMenu_AbortInitMenu ()
 121 stdcall SHFlushClipboard ()
 122 stdcall -noname RunDLL_CallEntry16(long long long str long) #name wrong?
 123 stdcall SHFreeUnusedLibraries ()
 124 stdcall FileMenu_AppendFilesForPidl(long ptr long)
 125 stdcall FileMenu_AddFilesForPidl(long long long ptr long long ptr)
 126 stdcall SHOutOfMemoryMessageBox (long long long)
 127 stdcall SHWinHelp (long long long long)
 128 stdcall -private DllGetClassObject(long long ptr) SHELL32_DllGetClassObject
 129 stdcall DAD_AutoScroll(long ptr ptr)
 130 stdcall DAD_DragEnter(long)
 131 stdcall DAD_DragEnterEx(long long long)
 132 stdcall DAD_DragLeave()
 133 stdcall DragQueryFileW(long long ptr long)
 134 stdcall DAD_DragMove(long long)
 135 stdcall DragQueryPoint(long ptr)
 136 stdcall DAD_SetDragImage(long long)
 137 stdcall DAD_ShowDragImage (long)
 139 stub Desktop_UpdateBriefcaseOnEvent
 140 stdcall FileMenu_DeleteItemByIndex(long long)
 141 stdcall FileMenu_DeleteItemByFirstID(long long)
 142 stdcall FileMenu_DeleteSeparator(long)
 143 stdcall FileMenu_EnableItemByCmd(long long long)
 144 stdcall FileMenu_GetItemExtent (long long)
 145 stdcall PathFindOnPath (ptr ptr) PathFindOnPathAW
 146 stdcall RLBuildListOfPaths()
 147 stdcall SHCLSIDFromString(long long) SHCLSIDFromStringAW
 149 stdcall SHFind_InitMenuPopup(long long long long)

 151 stdcall SHLoadOLE (long)
 152 stdcall ILGetSize(ptr)
 153 stdcall ILGetNext(ptr)
 154 stdcall ILAppend (long long long)
 155 stdcall ILFree (ptr)
 156 stdcall ILGlobalFree (ptr)
 157 stdcall ILCreateFromPath (ptr) ILCreateFromPathAW
 158 stdcall PathGetExtension(str long long) PathGetExtensionAW
 159 stdcall PathIsDirectory(ptr)PathIsDirectoryAW
 160 stub SHNetConnectionDialog
 161 stdcall SHRunControlPanel (long long)
 162 stdcall SHSimpleIDListFromPath (ptr) SHSimpleIDListFromPathAW
 163 stdcall StrToOleStr (wstr str) StrToOleStrAW
 164 stdcall Win32DeleteFile(str) Win32DeleteFileAW
 165 stdcall SHCreateDirectory(long ptr)
 166 stdcall CallCPLEntry16(long long long long long long)
 167 stdcall SHAddFromPropSheetExtArray(long long long)
 168 stdcall SHCreatePropSheetExtArray(long str long)
 169 stdcall SHDestroyPropSheetExtArray(long)
 170 stdcall SHReplaceFromPropSheetExtArray(long long long long)
 171 stdcall PathCleanupSpec(ptr ptr) PathCleanupSpecAW
 172 stdcall SHCreateLinks(long str ptr long ptr)
 173 stdcall SHValidateUNC(long long long)
 174 stdcall SHCreateShellFolderViewEx (ptr ptr)
 175 stdcall SHGetSpecialFolderPath(long long long long) SHGetSpecialFolderPathAW
 176 stdcall SHSetInstanceExplorer (long)
 177 stub DAD_SetDragImageFromListView
 178 stub SHObjectProperties
 179 stub SHGetNewLinkInfoA
 180 stub SHGetNewLinkInfoW
 181 stdcall RegisterShellHook(long long)
 182 varargs ShellMessageBoxW(long long long str long)
 183 varargs ShellMessageBoxA(long long long str long)
 184 stdcall ArrangeWindows(long long long long long)
 185 stub SHHandleDiskFull
 186 stdcall ILGetDisplayNameEx(ptr ptr ptr long)
 187 stub ILGetPseudoNameW
 188 stub ShellDDEInit
 189 stdcall ILCreateFromPathA(str)
 190 stdcall ILCreateFromPathW(wstr)
 195 stdcall SHFree(ptr)
 196 stdcall SHAlloc(long)
 197 stub SHGlobalDefect
 198 stdcall SHAbortInvokeCommand ()
 199 stub SHGetFileIcon
 200 stub SHLocalAlloc
 201 stub SHLocalFree
 202 stub SHLocalReAlloc
 203 stub AddCommasW
 204 stub ShortSizeFormatW
 205 stub Printer_LoadIconsW
 206 stub Link_AddExtraDataSection
 207 stub Link_ReadExtraDataSection
 208 stub Link_RemoveExtraDataSection
 209 stub Int64ToString
 210 stub LargeIntegerToString
 211 stub Printers_GetPidl
 212 stub Printers_AddPrinterPropPages
 213 stub Printers_RegisterWindowW
 214 stub Printers_UnregisterWindow
 215 stub SHStartNetConnectionDialog
 243 stdcall @(long long) shell32_243
 244 stdcall SHInitRestricted(ptr ptr)
 247 stdcall SHGetDataFromIDListA (ptr ptr long ptr long)
 248 stdcall SHGetDataFromIDListW (ptr ptr long ptr long)
 249 stdcall PathParseIconLocation (ptr) PathParseIconLocationAW
 250 stdcall PathRemoveExtension (ptr) PathRemoveExtensionAW
 251 stdcall PathRemoveArgs (ptr) PathRemoveArgsAW
 256 stdcall @(ptr ptr) SHELL32_256
 271 stub SheChangeDirA
 272 stub SheChangeDirExA
 273 stub SheChangeDirExW
 274 stdcall SheChangeDirW(wstr)
 275 stub SheConvertPathW
 276 stub SheFullPathA
 277 stub SheFullPathW
 278 stub SheGetCurDrive
 279 stub SheGetDirA
 280 stub SheGetDirExW
 281 stdcall SheGetDirW (long long)
 282 stub SheGetPathOffsetW
 283 stub SheRemoveQuotesA
 284 stub SheRemoveQuotesW
 285 stub SheSetCurDrive
 286 stub SheShortenPathA
 287 stub SheShortenPathW
 288 stdcall ShellAboutA(long str str long)
 289 stdcall ShellAboutW(long wstr wstr long)
 290 stdcall ShellExecuteA(long str str str str long)
 291 stdcall ShellExecuteEx (long) ShellExecuteExAW
 292 stdcall ShellExecuteExA (long)
 293 stdcall ShellExecuteExW (long)
 294 stdcall ShellExecuteW (long wstr wstr wstr wstr long)
 296 stdcall Shell_NotifyIcon(long ptr) Shell_NotifyIconA
 297 stdcall Shell_NotifyIconA(long ptr)
 298 stdcall Shell_NotifyIconW(long ptr)
 299 stub Shl1632_ThunkData32
 300 stub Shl3216_ThunkData32
 301 stdcall StrChrA(str long) shlwapi.StrChrA
 302 stdcall StrChrIA(str long) shlwapi.StrChrIA
 303 stdcall StrChrIW(wstr long) shlwapi.StrChrIW
 304 stdcall StrChrW(wstr long) shlwapi.StrChrW
 305 stdcall StrCmpNA(str str long) shlwapi.StrCmpNA
 306 stdcall StrCmpNIA(str str long) shlwapi.StrCmpNIA
 307 stdcall StrCmpNIW(wstr wstr long) shlwapi.StrCmpNIW
 308 stdcall StrCmpNW(wstr wstr long) shlwapi.StrCmpNW
 309 stdcall StrCpyNA (ptr str long) lstrcpynA
 310 stdcall StrCpyNW(wstr wstr long) shlwapi.StrCpyNW
 311 stdcall StrNCmpA(str str long) shlwapi.StrCmpNA
 312 stdcall StrNCmpIA(str str long) shlwapi.StrCmpNIA
 313 stdcall StrNCmpIW(wstr wstr long) shlwapi.StrCmpNIW
 314 stdcall StrNCmpW(wstr wstr long) shlwapi.StrCmpNW
 315 stdcall StrNCpyA (ptr str long) lstrcpynA
 316 stdcall StrNCpyW(wstr wstr long) shlwapi.StrCpyNW
 317 stdcall StrRChrA(str str long) shlwapi.StrRChrA
 318 stdcall StrRChrIA(str str long) shlwapi.StrRChrIA
 319 stdcall StrRChrIW(str str long) shlwapi.StrRChrIW
 320 stdcall StrRChrW(wstr wstr long) shlwapi.StrRChrW
 321 stub StrRStrA
 322 stdcall StrRStrIA(str str str) shlwapi.StrRStrIA
 323 stdcall StrRStrIW(wstr wstr wstr) shlwapi.StrRStrIW
 324 stub StrRStrW
 325 stdcall StrStrA(str str) shlwapi.StrStrA
 326 stdcall StrStrIA(str str) shlwapi.StrStrIA
 327 stdcall StrStrIW(wstr wstr) shlwapi.StrStrIW
 328 stdcall StrStrW(wstr wstr) shlwapi.StrStrW

 505 stdcall SHRegCloseKey (long)
 506 stdcall SHRegOpenKeyA (long str long)
 507 stdcall SHRegOpenKeyW (long wstr long)
 508 stub SHRegQueryValueA
 509 stdcall SHRegQueryValueExA(long str ptr ptr ptr ptr)
 510 stdcall SHRegQueryValueW (long long long long)
 511 stdcall SHRegQueryValueExW (long wstr ptr ptr ptr ptr)
 512 stdcall SHRegDeleteKeyW (long wstr)

 520 stdcall SHAllocShared (long long long)
 521 stdcall SHLockShared (long long)
 522 stdcall SHUnlockShared (long)
 523 stdcall SHFreeShared (long long)
 524 stub RealDriveType
 525 stub RealDriveTypeFlags

 640 stdcall NTSHChangeNotifyRegister (long long long long long long)
 641 stdcall NTSHChangeNotifyDeregister (long)

 643 stub SHChangeNotifyReceive
 644 stdcall SHChangeNotification_Lock(long long ptr ptr)
 645 stdcall SHChangeNotification_Unlock(long)
 646 stub SHChangeRegistrationReceive
 647 stub ReceiveAddToRecentDocs
 648 stub SHWaitOp_Operate

 650 stdcall PathIsSameRoot(ptr ptr)PathIsSameRootAW

# nt40/win98
 651 stdcall ReadCabinetState (long long) # OldReadCabinetState
 652 stdcall WriteCabinetState (long)
 653 stdcall PathProcessCommand (long long long long) PathProcessCommandAW

# win98
 654 stdcall @(long long)shell32_654 # ReadCabinetState@8
 660 stdcall FileIconInit(long)
 680 stdcall IsUserAdmin()

# >= NT5
 714 stdcall @(ptr)SHELL32_714 # PathIsTemporaryW

 730 stdcall RestartDialogEx(long long long long)

1217 stub FOOBAR1217   # no joke! This is the real name!!

#
# version 4.0 (win95)
# _WIN32_IE >= 0x0200
#
@ stdcall CheckEscapesA(str long)
@ stdcall CheckEscapesW(wstr long)
@ stdcall CommandLineToArgvW(wstr ptr)
@ stdcall Control_FillCache_RunDLL(long long long long) Control_FillCache_RunDLLA
@ stdcall Control_FillCache_RunDLLA(long long long long)
@ stdcall Control_FillCache_RunDLLW(long long long long)
@ stdcall Control_RunDLL(ptr ptr str long) Control_RunDLLA
@ stdcall Control_RunDLLA(ptr ptr str long)
@ stdcall Control_RunDLLW(ptr ptr wstr long)
@ stdcall -private DllCanUnloadNow() SHELL32_DllCanUnloadNow
@ stdcall DllInstall(long wstr)SHELL32_DllInstall
@ stdcall -private DllRegisterServer() SHELL32_DllRegisterServer
@ stdcall -private DllUnregisterServer() SHELL32_DllUnregisterServer
@ stdcall DoEnvironmentSubstA(str str)
@ stdcall DoEnvironmentSubstW(wstr wstr)
@ stub DragQueryFileAorW
@ stdcall DuplicateIcon(long long)
@ stdcall ExtractAssociatedIconA(long ptr long)
@ stdcall ExtractAssociatedIconExA(long str long long)
@ stdcall ExtractAssociatedIconExW(long wstr long long)
@ stub ExtractAssociatedIconW
@ stdcall ExtractIconA(long str long)
@ stdcall ExtractIconEx(ptr long ptr ptr long)ExtractIconExAW
@ stdcall ExtractIconExA(str long ptr ptr long)
@ stdcall ExtractIconExW(wstr long ptr ptr long)
@ stdcall ExtractIconW(long wstr long)
@ stub ExtractIconResInfoA
@ stub ExtractIconResInfoW
@ stub ExtractVersionResource16W
@ stub FindExeDlgProc
@ stdcall FindExecutableA(ptr ptr ptr)
@ stdcall FindExecutableW(wstr wstr wstr)
@ stdcall FreeIconList(long)
@ stub InternalExtractIconListA
@ stub InternalExtractIconListW
@ stub OpenAs_RunDLL
@ stub OpenAs_RunDLLA
@ stub OpenAs_RunDLLW
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
@ stdcall SHBrowseForFolder(ptr) SHBrowseForFolderA
@ stdcall SHBrowseForFolderA(ptr)
@ stdcall SHBrowseForFolderW(ptr)
@ stdcall SHChangeNotify (long long ptr ptr)
@ stdcall SHCreateDirectoryExA(long str ptr)
@ stdcall SHCreateDirectoryExW(long wstr ptr)
@ stub ShellHookProc
@ stub SHEmptyRecycleBinA
@ stub SHEmptyRecycleBinW
@ stdcall SHFileOperation(ptr)SHFileOperationAW
@ stdcall SHFileOperationA(ptr)
@ stdcall SHFileOperationW(ptr)
@ stub SHFormatDrive
@ stub SHFreeNameMappings
@ stdcall SHGetDesktopFolder(ptr)
@ stdcall SHGetFileInfo(ptr long ptr long long)SHGetFileInfoAW
@ stdcall SHGetFileInfoA(ptr long ptr long long)
@ stdcall SHGetFileInfoW(ptr long ptr long long)
@ stdcall SHGetInstanceExplorer(long)
@ stdcall SHGetMalloc(ptr)
@ stub SHGetNewLinkInfo
@ stdcall SHGetPathFromIDList(ptr ptr)SHGetPathFromIDListAW
@ stdcall SHGetPathFromIDListA(ptr ptr)
@ stdcall SHGetPathFromIDListW(ptr ptr)
@ stdcall SHGetSettings(ptr long)
@ stdcall SHGetSpecialFolderLocation(long long ptr)
@ stdcall SHHelpShortcuts_RunDLL(long long long long)
@ stub SHHelpShortcuts_RunDLLA
@ stub SHHelpShortcuts_RunDLLW
@ stdcall SHLoadInProc(long)
@ stub SHQueryRecycleBinA
@ stub SHQueryRecycleBinW
@ stub SHUpdateRecycleBinIcon
@ stub WOWShellExecute

#
# version 4.70 (IE3.0)
# _WIN32_IE >= 0x0300
#

#
# version 4.71 (IE4.0)
# _WIN32_IE >= 0x0400
#
@ stdcall DllGetVersion(ptr)SHELL32_DllGetVersion
@ stub SHGetFreeDiskSpace
@ stdcall SHGetSpecialFolderPathA(long ptr long long)
@ stdcall SHGetSpecialFolderPathW(long ptr long long)
#
# version 4.72 (IE4.01)
# _WIN32_IE >= 0x0401
# no new exports
#

#
# version 5.00 (Win2K)
# _WIN32_IE >= 0x0500
#
@ stdcall SHBindToParent(ptr ptr ptr ptr)
@ stdcall SHGetDiskFreeSpaceA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExA(str ptr ptr ptr) kernel32.GetDiskFreeSpaceExA
@ stdcall SHGetDiskFreeSpaceExW(wstr ptr ptr ptr) kernel32.GetDiskFreeSpaceExW
@ stdcall SHGetFolderPathA(long long long long ptr)
@ stdcall SHGetFolderPathW(long long long long ptr)
@ stdcall SHGetFolderLocation(long long long long ptr)

# version 6.0 (WinXP)
# _WIN32_IE >= 0x600
@ stdcall SHDefExtractIconA(str long long ptr ptr long)
@ stdcall SHDefExtractIconW(wstr long long ptr ptr long)
