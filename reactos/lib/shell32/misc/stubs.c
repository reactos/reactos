/* $Id: stubs.c,v 1.2 2001/07/06 04:01:27 rex Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/shell32/misc/stubs.c
 * PURPOSE:         Stubbed exports
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 */

#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
#include <debug.h>

#undef DragQueryFile
#undef ShellExecute

#define  STUB  \
  do  \
  {   \
    DbgPrint ("%s(%d):%s not implemented\n", __FILE__, __LINE__, __FUNCTION__);  \
  }   \
  while (0)

VOID
STDCALL
SHChangeNotifyRegister(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
SHChangeNotifyDeregister(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHChangeNotifyUpdateEntryList(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
PifMgr_OpenProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
PifMgr_GetProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
PifMgr_SetProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
PifMgr_CloseProperties(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILGetDisplayName(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILFindLastID(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILRemoveLastID(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILClone(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILCloneFirst(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILGlobalClone(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILIsEqual(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILIsParent(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
ILFindChild(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILCombine(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILLoadFromStream(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ILSaveToStream(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHILCreateFromPath(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
PathIsRoot(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathBuildRoot(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
PathFindExtension(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathAddBackslash(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathRemoveBlanks(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathFindFileName(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathRemoveFileSpec(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathAppend(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
PathCombine(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
PathStripPath(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathIsUNC(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathIsRelative(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathIsExe(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathFileExists(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathMatchSpec(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
PathMakeUniqueName(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5) 
{
  STUB;
}

VOID STDCALL
PathSetDlgItemPath(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
PathQualify(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathStripToRoot(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathResolve(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
PathGetArgs(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
DoEnvironmentSubst(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

void WINAPI
DragAcceptFiles (HWND Unknown1, WINBOOL Unknown2)
{
  STUB;
}

VOID STDCALL
PathQuoteSpaces(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathUnquoteSpaces(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathGetDriveNumber(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ParseField(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
RestartDialog(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
ExitWindowsDialog(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
RunFileDlg(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
PickIconDlg(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
GetFileNameFromBrowse(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

VOID STDCALL
DriveType(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
InvalidateDriveType(VOID)
{
  STUB;
}

VOID STDCALL
IsNetDrive(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
Shell_MergeMenus(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
SHGetSetSettings(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHGetNetResource(VOID)
{
  STUB;
}

VOID STDCALL
SHCreateDefClassObject(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
Shell_GetImageList(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Shell_GetCachedImageIndex(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHShellFolderView_Message(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHCreateStdEnumFmtEtc(VOID)
{
  STUB;
}

VOID STDCALL
PathYetAnotherMakeUniqueName(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
DragQueryInfo(VOID)
{
  STUB;
}

VOID STDCALL
SHMapPIDLToSystemImageListIndex(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
OleStrToStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
StrToOleStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

void WINAPI
DragFinish (HDROP Unknown1)
{
  STUB;
}

unsigned int WINAPI
DragQueryFile(HDROP Unknown1, unsigned int Unknown2, char * Unknown3, unsigned int Unknown4)
{
  STUB;
}

unsigned int WINAPI
DragQueryFileA(HDROP Unknown1, unsigned int Unknown2, char * Unknown3, unsigned int Unknown4)
{
  STUB;
}

VOID STDCALL
CIDLData_CreateFromIDArray(VOID)
{
  STUB;
}

VOID STDCALL
SHIsBadInterfacePtr(VOID)
{
  STUB;
}

VOID STDCALL
SHRegisterDragDrop(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHRevokeDragDrop(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHDoDragDrop(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHCloneSpecialIDList(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHFindFiles(VOID)
{
  STUB;
}

VOID STDCALL
SHFindComputer(VOID)
{
  STUB;
}

VOID STDCALL
PathGetShortPath(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
Win32CreateDirectory(VOID)
{
  STUB;
}

VOID STDCALL
Win32RemoveDirectory(VOID)
{
  STUB;
}

VOID STDCALL
SHLogILFromFSIL(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
StrRetToStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHWaitForFileToOpen(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHGetRealIDL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SetAppStartingCursor(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHRestricted(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHCoCreateInstance(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SignalFileOpen(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_DeleteAllItems(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_DrawItem(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_FindSubMenuByPidl(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_GetLastSelectedItemPidls(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
FileMenu_HandleMenuChar(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_InitMenuPopup(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_InsertUsingPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
FileMenu_Invalidate(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_MeasureItem(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_ReplaceUsingPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
FileMenu_Create(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
FileMenu_AppendItem(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
FileMenu_TrackPopupMenuEx(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
FileMenu_DeleteItemByCmd(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_Destroy(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
IsLFNDrive(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_AbortInitMenu(VOID)
{
  STUB;
}

VOID STDCALL
SHFlushClipboard(VOID)
{
  STUB;
}

VOID STDCALL
RunDLL_CallEntry16(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHFreeUnusedLibraries(VOID)
{
  STUB;
}

VOID STDCALL
FileMenu_AppendFilesForPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
FileMenu_AddFilesForPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

VOID STDCALL
SHOutOfMemoryMessageBox(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHWinHelp(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
DllGetClassObject(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
DAD_AutoScroll(VOID)
{
  STUB;
}

VOID STDCALL
DAD_DragEnter(VOID)
{
  STUB;
}

VOID STDCALL
DAD_DragEnterEx(VOID)
{
  STUB;
}

VOID STDCALL
DAD_DragLeave(VOID)
{
  STUB;
}

unsigned int WINAPI
DragQueryFileW(HDROP Unknown1, unsigned int Unknown2, LPCWSTR Unknown3, unsigned int Unknown4)
{
  STUB;
}

VOID STDCALL
DAD_DragMove(VOID)
{
  STUB;
}

WINBOOL WINAPI
DragQueryPoint (HDROP Unknown1, LPPOINT Unknown2)
{
  STUB;
}

VOID STDCALL
DAD_SetDragImage(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
DAD_ShowDragImage(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
Desktop_UpdateBriefcaseOnEvent(VOID)
{
  STUB;
}

VOID STDCALL
FileMenu_DeleteItemByIndex(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_DeleteItemByFirstID(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileMenu_DeleteSeparator(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FileMenu_EnableItemByCmd(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
FileMenu_GetItemExtent(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
PathFindOnPath(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
RLBuildListOfPaths(VOID)
{
  STUB;
}

VOID STDCALL
SHCLSIDFromString(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHFind_InitMenuPopup(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHLoadOLE(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILGetSize(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILGetNext(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILAppend(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
ILFree(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILGlobalFree(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ILCreateFromPath(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathGetExtension(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathIsDirectory(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHNetConnectionDialog(VOID)
{
  STUB;
}

VOID STDCALL
SHRunControlPanel(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHSimpleIDListFromPath(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
StrToOleStr(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Win32DeleteFile(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHCreateDirectory(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
CallCPLEntry16(VOID)
{
  STUB;
}

VOID STDCALL
SHAddFromPropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHCreatePropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHDestroyPropSheetExtArray(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHReplaceFromPropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
PathCleanupSpec(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHCreateLinks(VOID)
{
  STUB;
}

VOID STDCALL
SHValidateUNC(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHCreateShellFolderViewEx(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHGetSpecialFolderPath(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHSetInstanceExplorer(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
DAD_SetDragImageFromListView(VOID)
{
  STUB;
}

VOID STDCALL
SHObjectProperties(VOID)
{
  STUB;
}

VOID STDCALL
SHGetNewLinkInfoA(VOID)
{
  STUB;
}

VOID STDCALL
SHGetNewLinkInfoW(VOID)
{
  STUB;
}

VOID STDCALL
RegisterShellHook(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID 
ShellMessageBoxW (DWORD Unknown1, ...)
{
  STUB;
}

VOID 
ShellMessageBoxA (DWORD Unknown1, ...)
{
  STUB;
}

VOID STDCALL
ArrangeWindows(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHHandleDiskFull(VOID)
{
  STUB;
}

VOID STDCALL
SHFree(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHAlloc(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHGlobalDefect(VOID)
{
  STUB;
}

VOID STDCALL
SHAbortInvokeCommand(VOID)
{
  STUB;
}

VOID STDCALL
SHGetFileIcon(VOID)
{
  STUB;
}

VOID STDCALL
SHLocalAlloc(VOID)
{
  STUB;
}

VOID STDCALL
SHLocalFree(VOID)
{
  STUB;
}

VOID STDCALL
SHLocalReAlloc(VOID)
{
  STUB;
}

VOID STDCALL
AddCommasW(VOID)
{
  STUB;
}

VOID STDCALL
ShortSizeFormatW(VOID)
{
  STUB;
}

VOID STDCALL
Printer_LoadIconsW(VOID)
{
  STUB;
}

VOID STDCALL
Link_AddExtraDataSection(VOID)
{
  STUB;
}

VOID STDCALL
Link_ReadExtraDataSection(VOID)
{
  STUB;
}

VOID STDCALL
Link_RemoveExtraDataSection(VOID)
{
  STUB;
}

VOID STDCALL
Int64ToString(VOID)
{
  STUB;
}

VOID STDCALL
LargeIntegerToString(VOID)
{
  STUB;
}

VOID STDCALL
Printers_GetPidl(VOID)
{
  STUB;
}

VOID STDCALL
Printer_AddPrinterPropPages(VOID)
{
  STUB;
}

VOID STDCALL
Printers_RegisterWindowW(VOID)
{
  STUB;
}

VOID STDCALL
Printers_UnregisterWindow(VOID)
{
  STUB;
}

VOID STDCALL
SHStartNetConnectionDialog(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
shell32_243(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHInitRestricted(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHGetDataFromIDListA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetDataFromIDListW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
PathParseIconLocation(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathRemoveExtension(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathRemoveArgs(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SheChangeDirA(VOID)
{
  STUB;
}

VOID STDCALL
SheChangeDirExA(VOID)
{
  STUB;
}

VOID STDCALL
SheChangeDirExW(VOID)
{
  STUB;
}

VOID STDCALL
SheChangeDirW(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SheConvertPathW(VOID)
{
  STUB;
}

VOID STDCALL
SheFullPathA(VOID)
{
  STUB;
}

VOID STDCALL
SheFullPathW(VOID)
{
  STUB;
}

VOID STDCALL
SheGetCurDrive(VOID)
{
  STUB;
}

VOID STDCALL
SheGetDirA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SheGetDirExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SheGetDirW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SheGetPathOffsetW(VOID)
{
  STUB;
}

VOID STDCALL
SheRemoveQuotesA(VOID)
{
  STUB;
}

VOID STDCALL
SheRemoveQuotesW(VOID)
{
  STUB;
}

VOID STDCALL
SheSetCurDrive(VOID)
{
  STUB;
}

VOID STDCALL
SheShortenPathA(VOID)
{
  STUB;
}

VOID STDCALL
SheShortenPathW(VOID)
{
  STUB;
}

int WINAPI
ShellAboutA (HWND Unknown1, const char * Unknown2, const char * Unknown3, HICON Unknown4)
{
  STUB;
}

int WINAPI
ShellAboutW (HWND Unknown1, const LPCWSTR Unknown2, const LPCWSTR Unknown3, HICON Unknown4)
{
  STUB;
}

HINSTANCE WINAPI
ShellExecuteA (HWND Unknown1, const char * Unknown2, const char * Unknown3, char * Unknown4, const char * Unknown5, int Unknown6)
{
  STUB;
}

VOID STDCALL
ShellExecuteEx(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ShellExecuteExA(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ShellExecuteExW(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
ShellExecute (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

HINSTANCE WINAPI
ShellExecuteW (HWND Unknown1, const LPCWSTR Unknown2, const LPCWSTR Unknown3, LPCWSTR Unknown4, const LPCWSTR Unknown5, int Unknown6)
{
  STUB;
}

VOID STDCALL
Shell_NotifyIcon(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Shell_NotifyIconA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Shell_NotifyIconW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Shl1632_ThunkData32(VOID)
{
  STUB;
}

VOID STDCALL
Shl3216_ThunkData32(VOID)
{
  STUB;
}

VOID STDCALL
StrCpyNA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
StrNCpyA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
StrRStrA(VOID)
{
  STUB;
}

VOID STDCALL
StrRStrW(VOID)
{
  STUB;
}

VOID STDCALL
SHRegCloseKey(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHRegOpenKeyA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHRegOpenKeyW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHRegQueryValueA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHRegQueryValueExA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
SHRegQueryValueW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHRegQueryValueExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
SHRegDeleteKeyW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHAllocShared(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID STDCALL
SHLockShared(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHUnlockShared(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHFreeShared(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
RealDriveType(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
RealDriveTypeFlags(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
NTSHChangeNotifyRegister(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID STDCALL
NTSHChangeNotifyDeregister(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHChangeNotifyReceive(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHChangeNotification_Lock(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHChangeNotification_Unlock(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHChangeRegistrationReceive(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ReceiveAddToRecentDocs(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHWaitOp_Operate(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
PathIsSameRoot(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
ReadCabinetState(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
WriteCabinetState(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
PathProcessCommand(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
shell32_654(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
FileIconInit(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
IsUserAdmin(VOID)
{
  STUB;
}

VOID STDCALL
shell32_714(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
FOOBAR1217(VOID)
{
  STUB;
}

VOID STDCALL
CheckEscapesA(VOID)
{
  STUB;
}

VOID STDCALL
CheckEscapesW(VOID)
{
  STUB;
}

VOID STDCALL
CommandLineToArgvW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
Control_FillCache_RunDLL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
Control_FillCache_RunDLLA(VOID)
{
  STUB;
}

VOID STDCALL
Control_FillCache_RunDLLW(VOID)
{
  STUB;
}

VOID STDCALL
Control_RunDLL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
Control_RunDLLA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
Control_RunDLLW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
DllInstall(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
DoEnvironmentSubstA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
DoEnvironmentSubstW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
DragQueryFileAorW(VOID)
{
  STUB;
}

HICON WINAPI
DuplicateIcon (HINSTANCE Unknown1, HICON Unknown2)
{
  STUB;
}

HICON WINAPI
ExtractAssociatedIconA (HINSTANCE Unknown1, char * Unknown2, WORD * Unknown3)
{
  STUB;
}

VOID STDCALL
ExtractAssociatedIconExA(VOID)
{
  STUB;
}

VOID STDCALL
ExtractAssociatedIconExW(VOID)
{
  STUB;
}

HICON WINAPI
ExtractAssociatedIconW (HINSTANCE Unknown1, LPCWSTR Unknown2, WORD * Unknown3)
{
  STUB;
}

HICON WINAPI
ExtractIconA (HINSTANCE Unknown1, const char * Unknown2, unsigned int Unknown3)
{
  STUB;
}

VOID STDCALL
ExtractIconEx(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
ExtractIconExA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
ExtractIconExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

HICON WINAPI
ExtractIconW (HINSTANCE Unknown1, const LPCWSTR Unknown2, unsigned int Unknown3)
{
  STUB;
}

VOID STDCALL
ExtractIconResInfoA(VOID)
{
  STUB;
}

VOID STDCALL
ExtractIconResInfoW(VOID) 
{
  STUB;
}

VOID STDCALL
ExtractVersionResource16W(VOID)
{
  STUB;
}

VOID STDCALL
FindExeDlgProc(VOID)
{
  STUB;
}

HINSTANCE WINAPI
FindExecutableA (const char * Unknown1, const char * Unknown2, char * Unknown3)
{
  STUB;
}

HINSTANCE WINAPI
FindExecutableW (const LPCWSTR Unknown1, const LPCWSTR Unknown2, LPCWSTR Unknown3)
{
  STUB;
}

VOID STDCALL
FreeIconList(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
InternalExtractIconListA(VOID)
{
  STUB;
}

VOID STDCALL
InternalExtractIconListW(VOID)
{
  STUB;
}

VOID STDCALL
OpenAs_RunDLL(VOID)
{
  STUB;
}

VOID STDCALL
OpenAs_RunDLLA(VOID)
{
  STUB;
}

VOID STDCALL
OpenAs_RunDLLW(VOID)
{
  STUB;
}

VOID STDCALL
PrintersGetCommand_RunDLL(VOID)
{
  STUB;
}

VOID STDCALL
PrintersGetCommand_RunDLLA(VOID)
{
  STUB;
}

VOID STDCALL
PrintersGetCommand_RunDLLW(VOID)
{
  STUB;
}

VOID STDCALL
RealShellExecuteA(VOID)
{
  STUB;
}

VOID STDCALL
RealShellExecuteExA(VOID)
{
  STUB;
}

VOID STDCALL
RealShellExecuteExW(VOID)
{
  STUB;
}

VOID STDCALL
RealShellExecuteW(VOID)
{
  STUB;
}

VOID STDCALL
RegenerateUserEnvironment(VOID)
{
  STUB;
}

void WINAPI
SHAddToRecentDocs (UINT Unknown1, LPCVOID Unknown2)
{
  STUB;
}

VOID STDCALL
SHAppBarMessage(DWORD Unknown1, DWORD Unknown2) 
{
  STUB;
}

LPITEMIDLIST WINAPI
SHBrowseForFolder (LPBROWSEINFO Unknown1)
{
  STUB;
}

VOID STDCALL
SHBrowseForFolderA(DWORD Unknown1) 
{
  STUB;
}

VOID STDCALL
SHBrowseForFolderW(DWORD Unknown1) 
{
  STUB;
}

void WINAPI
SHChangeNotify (LONG Unknown1, UINT Unknown2, LPCVOID Unknown3, LPCVOID Unknown4)
{
  STUB;
}

VOID STDCALL
ShellHookProc(VOID)
{
  STUB;
}

VOID STDCALL
SHEmptyRecycleBinA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3) 
{
  STUB;
}

VOID STDCALL
SHEmptyRecycleBinW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3) 
{
  STUB;
}

int WINAPI
SHFileOperation (LPSHFILEOPSTRUCT Unknown1)
{
  STUB;
}

VOID STDCALL
SHFileOperationA(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHFileOperationW(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHFormatDrive(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

void WINAPI
SHFreeNameMappings (HANDLE Unknown1)
{
  STUB;
}

VOID STDCALL
SHGetDesktopFolder(DWORD Unknown1)
{
  STUB;
}

DWORD WINAPI
SHGetFileInfo (LPCTSTR Unknown1, DWORD Unknown2, SHFILEINFO FAR * Unknown3, UINT Unknown4, UINT Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetFileInfoA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetFileInfoW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetInstanceExplorer(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHGetMalloc(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHGetNewLinkInfo(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

WINBOOL WINAPI
SHGetPathFromIDList (LPCITEMIDLIST Unknown1, LPTSTR Unknown2)
{
  STUB;
}

VOID STDCALL
SHGetPathFromIDListA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHGetPathFromIDListW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID STDCALL
SHGetSettings(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

HRESULT WINAPI
SHGetSpecialFolderLocation (HWND Unknown1, int Unknown2, LPITEMIDLIST * Unknown3)
{
  STUB;
}

VOID STDCALL
SHHelpShortcuts_RunDLL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHHelpShortcuts_RunDLLA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

VOID STDCALL
SHHelpShortcuts_RunDLLW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

VOID STDCALL
SHLoadInProc(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHQueryRecycleBinA(DWORD Unknown1, DWORD Unknown2) 
{
  STUB;
}

VOID STDCALL
SHQueryRecycleBinW(DWORD Unknown1, DWORD Unknown2) 
{
  STUB;
}

VOID STDCALL
SHUpdateRecycleBinIcon(VOID) 
{
  STUB;
}

VOID STDCALL
WOWShellExecute(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

VOID STDCALL
DllCanUnloadNow(VOID)
{
  STUB;
}

VOID STDCALL
DllGetVersion(DWORD Unknown1)
{
  STUB;
}

VOID STDCALL
SHGetFreeDiskSpace(VOID)
{
  STUB;
}

VOID STDCALL
SHGetSpecialFolderPathA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID STDCALL
SHGetFolderPathA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetFolderPathW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID STDCALL
SHGetFolderLocation(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}


