/*
 *  ReactOS shell32 - 
 *
 *  stubs.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: stubs.c,v 1.4 2003/01/07 17:35:56 robd Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/shell32/misc/stubs.c
 * PURPOSE:         Stubbed exports
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <tchar.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "shell32.h"
#include <malloc.h>

#ifdef __GNUC__
void* _alloca(size_t);
#else
#define __FUNCTION__ "unknown"
#endif

#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
//#include <shellapi.h>
//#include <shlobj.h>

#define HDROP HANDLE
#define LPSHELLEXECUTEINFOA int*
#define LPSHELLEXECUTEINFOW int*
#define PNOTIFYICONDATAA    int*
#define PNOTIFYICONDATAW    int*
#define PAPPBARDATA         int*
#define LPSHFILEOPSTRUCTA   int*
#define LPSHFILEOPSTRUCTW   int*
#define LPSHQUERYRBINFO     int*
#define SHFILEINFOA int
#define SHFILEINFOW int


#define NDEBUG
//#include <debug.h>

typedef struct _SHITEMID { 
    USHORT cb; 
    BYTE   abID[1]; 
} SHITEMID, * LPSHITEMID; 
typedef const SHITEMID  * LPCSHITEMID; 

typedef struct _ITEMIDLIST {
    SHITEMID mkid;
} ITEMIDLIST, * LPITEMIDLIST; 
typedef const ITEMIDLIST * LPCITEMIDLIST; 

int CALLBACK BrowseCallbackProc(
    HWND hwnd, 
    UINT uMsg, 
    LPARAM lParam, 
    LPARAM lpData
    );

typedef int (*BFFCALLBACK)(HWND, UINT, LPARAM, LPARAM);

typedef struct _browseinfo { 
    HWND hwndOwner; 
    LPCITEMIDLIST pidlRoot; 
    LPTSTR pszDisplayName; 
    LPCTSTR lpszTitle; 
    UINT ulFlags; 
    BFFCALLBACK lpfn; 
    LPARAM lParam; 
    int iImage; 
} BROWSEINFO, *PBROWSEINFO, *LPBROWSEINFO; 


#define DbgPrint(a,b,c,d)

#undef DragQueryFile
#undef ShellExecute

#define  STUB  \
  do  \
  {   \
    DbgPrint ("%s(%d):%s not implemented\n", __FILE__, __LINE__, __FUNCTION__); \
  }   \
  while (0)



VOID
WINAPI
SHChangeNotifyRegister(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
SHChangeNotifyDeregister(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHChangeNotifyUpdateEntryList(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
PifMgr_OpenProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
PifMgr_GetProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
PifMgr_SetProperties(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
PifMgr_CloseProperties(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILGetDisplayName(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILFindLastID(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILRemoveLastID(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILClone(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILCloneFirst(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILGlobalClone(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILIsEqual(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILIsParent(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
ILFindChild(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILCombine(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILLoadFromStream(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ILSaveToStream(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHILCreateFromPath(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

BOOL WINAPI
PathIsRootA(LPCSTR path)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathIsRootW(LPCWSTR path)
{
  STUB;
  return FALSE;
}

LPSTR WINAPI
PathBuildRootA(LPSTR Unknown1, int Unknown2)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathBuildRootW(LPWSTR Unknown1, int Unknown2)
{
  STUB;
  return 0;
}

BOOL WINAPI
PathAddExtensionA(LPSTR Unknown1, LPCSTR Unknown2)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathAddExtensionW(LPWSTR Unknown1, LPCWSTR Unknown2)
{
  STUB;
  return FALSE;
}


LPSTR WINAPI
PathFindExtensionA(LPCSTR Unknown1)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathFindExtensionW(LPCWSTR Unknown1)
{
  STUB;
  return 0;
}

LPSTR WINAPI
PathAddBackslashA(LPSTR Unknown1)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathAddBackslashW(LPWSTR Unknown1)
{
  STUB;
  return 0;
}

VOID WINAPI
PathRemoveBlanksA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathRemoveBlanksW(LPWSTR Unknown1)
{
  STUB;
}

LPSTR WINAPI
PathFindFileNameA(LPCSTR Unknown1)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathFindFileNameW(LPCWSTR Unknown1)
{
  STUB;
  return 0;
}

BOOL WINAPI
PathRemoveFileSpecA(LPSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathRemoveFileSpecW(LPWSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathAppendA(LPSTR Unknown1, LPCSTR Unknown2)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathAppendW(LPWSTR Unknown1, LPCWSTR Unknown2)
{
  STUB;
  return FALSE;
}

LPSTR WINAPI
PathCombineA(LPSTR Unknown1, LPCSTR Unknown2, LPCSTR Unknown3)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathCombineW(LPWSTR Unknown1, LPCWSTR Unknown2, LPCWSTR Unknown3)
{
  STUB;
  return 0;
}

VOID WINAPI
PathStripPathA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathStripPathW(LPWSTR Unknown1)
{
  STUB;
}

BOOL WINAPI
PathIsUNCA(LPCSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathIsUNCW(LPCWSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathIsRelativeA(LPCSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathIsRelativeW(LPCWSTR Unknown1)
{
  STUB;
  return FALSE;
}

VOID WINAPI
PathIsExeA(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
PathIsExeW(DWORD Unknown1)
{
  STUB;
}

BOOL WINAPI
PathFileExistsA(LPCSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathFileExistsW(LPCWSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathMatchSpecA(LPCSTR Unknown1, LPCSTR Unknown2)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathMatchSpecW(LPCWSTR Unknown1, LPCWSTR Unknown2)
{
  STUB;
  return FALSE;
}

VOID WINAPI
PathMakeUniqueNameA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5) 
{
  STUB;
}

VOID WINAPI
PathMakeUniqueNameW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5) 
{
  STUB;
}

VOID WINAPI
PathSetDlgItemPathA(HWND Unknown1, int Unknown2, LPCSTR Unknown3)
{
  STUB;
}

VOID WINAPI
PathSetDlgItemPathW(HWND Unknown1, int Unknown2, LPCWSTR Unknown3)
{
  STUB;
}

VOID WINAPI
PathQualifyA(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
PathQualifyW(DWORD Unknown1)
{
  STUB;
}

BOOL WINAPI
PathStripToRootA(LPSTR Unknown1)
{
  STUB;
  return FALSE;
}

BOOL WINAPI
PathStripToRootW(LPWSTR Unknown1)
{
  STUB;
  return FALSE;
}

VOID WINAPI
PathResolveA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
PathResolveW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

LPSTR WINAPI
PathGetArgsA(LPCSTR Unknown1)
{
  STUB;
  return 0;
}

LPWSTR WINAPI
PathGetArgsW(LPCWSTR Unknown1)
{
  STUB;
  return 0;
}

DWORD WINAPI
DoEnvironmentSubst(LPTSTR pszString, UINT cbSize)
{
  STUB;
  return 0;
}

//VOID WINAPI
//DragAcceptFiles (HWND Unknown1, WINBOOL Unknown2)
VOID WINAPI
DragAcceptFiles(HWND hWnd, BOOL fAccept)
{
  STUB;
}

VOID WINAPI
PathQuoteSpacesA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathQuoteSpacesW(LPWSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathUnquoteSpacesA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathUnquoteSpacesW(LPWSTR Unknown1)
{
  STUB;
}

int WINAPI
PathGetDriveNumberA(LPCSTR Unknown1)
{
  STUB;
  return 0;
}

int WINAPI
PathGetDriveNumberW(LPCWSTR Unknown1)
{
  STUB;
  return 0;
}

VOID WINAPI
ParseField(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
RestartDialog(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
ExitWindowsDialog(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
RunFileDlg(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
PickIconDlg(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
GetFileNameFromBrowse(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

VOID WINAPI
DriveType(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
InvalidateDriveType(VOID)
{
  STUB;
}

VOID WINAPI
IsNetDrive(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
Shell_MergeMenus(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
SHGetSetSettings(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHGetNetResource(VOID)
{
  STUB;
}

VOID WINAPI
SHCreateDefClassObject(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
Shell_GetImageList(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
Shell_GetCachedImageIndex(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHShellFolderView_Message(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHCreateStdEnumFmtEtc(VOID)
{
  STUB;
}

VOID WINAPI
PathYetAnotherMakeUniqueName(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
DragQueryInfo(VOID)
{
  STUB;
}

VOID WINAPI
SHMapPIDLToSystemImageListIndex(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
OleStrToStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
StrToOleStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
CIDLData_CreateFromIDArray(VOID)
{
  STUB;
}

VOID WINAPI
SHIsBadInterfacePtr(VOID)
{
  STUB;
}

VOID WINAPI
SHRegisterDragDrop(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHRevokeDragDrop(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHDoDragDrop(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SHCloneSpecialIDList(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHFindFiles(VOID)
{
  STUB;
}

VOID WINAPI
SHFindComputer(VOID)
{
  STUB;
}

VOID WINAPI
PathGetShortPath(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
Win32CreateDirectory(VOID)
{
  STUB;
}

VOID WINAPI
Win32RemoveDirectory(VOID)
{
  STUB;
}

VOID WINAPI
SHLogILFromFSIL(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
StrRetToStrN(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHWaitForFileToOpen(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHGetRealIDL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SetAppStartingCursor(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHRestricted(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHCoCreateInstance(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SignalFileOpen(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_DeleteAllItems(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_DrawItem(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_FindSubMenuByPidl(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_GetLastSelectedItemPidls(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
FileMenu_HandleMenuChar(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_InitMenuPopup(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_InsertUsingPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
FileMenu_Invalidate(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_MeasureItem(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_ReplaceUsingPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
FileMenu_Create(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
FileMenu_AppendItem(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
FileMenu_TrackPopupMenuEx(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
FileMenu_DeleteItemByCmd(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_Destroy(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
IsLFNDrive(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_AbortInitMenu(VOID)
{
  STUB;
}

VOID WINAPI
SHFlushClipboard(VOID)
{
  STUB;
}
/*
VOID WINAPI
RunDLL_CallEntry16(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}
 */
VOID WINAPI
SHFreeUnusedLibraries(VOID)
{
  STUB;
}

VOID WINAPI
FileMenu_AppendFilesForPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
FileMenu_AddFilesForPidl(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

VOID WINAPI
SHOutOfMemoryMessageBox(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHWinHelp(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

/*
VOID WINAPI
DllGetClassObject(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

  This is now implemented in the C++ module _stubs.cpp as:

  STDAPI DllGetClassObject(const CLSID & rclsid, const IID & riid, void ** ppv); 

 */

VOID WINAPI
DAD_AutoScroll(VOID)
{
  STUB;
}

VOID WINAPI
DAD_DragEnter(VOID)
{
  STUB;
}

VOID WINAPI
DAD_DragEnterEx(VOID)
{
  STUB;
}

VOID WINAPI
DAD_DragLeave(VOID)
{
  STUB;
}

VOID WINAPI
DragFinish(HDROP hDrop)
{
  STUB;
}
/*
unsigned int WINAPI
DragQueryFile(HDROP Unknown1, unsigned int Unknown2, char * Unknown3, unsigned int Unknown4)
{
  STUB;
  return 0;
}

unsigned int WINAPI
DragQueryFileA(HDROP Unknown1, unsigned int Unknown2, char * Unknown3, unsigned int Unknown4)
{
  STUB;
  return 0;
}

unsigned int WINAPI
DragQueryFileW(HDROP Unknown1, unsigned int Unknown2, LPCWSTR Unknown3, unsigned int Unknown4)
{
  STUB;
}
 */
UINT WINAPI
DragQueryFileA(HDROP hDrop, UINT iFile, LPSTR lpszFile, UINT cch)
{
  STUB;
  return 0;
}

UINT WINAPI
DragQueryFileW(HDROP hDrop, UINT iFile, LPWSTR lpszFile, UINT cch)
{
  STUB;
  return 0;
}


VOID WINAPI
DAD_DragMove(VOID)
{
  STUB;
}

UINT WINAPI
DragQueryPoint(HDROP Unknown1, LPPOINT Unknown2)
{
  STUB;
  return 0;
}

VOID WINAPI
DAD_SetDragImage(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
DAD_ShowDragImage(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
Desktop_UpdateBriefcaseOnEvent(VOID)
{
  STUB;
}

VOID WINAPI
FileMenu_DeleteItemByIndex(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_DeleteItemByFirstID(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileMenu_DeleteSeparator(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FileMenu_EnableItemByCmd(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
FileMenu_GetItemExtent(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

BOOL WINAPI
PathFindOnPathA(LPSTR Unknown1, LPCSTR* Unknown2)
{
  STUB;
  return 0;
}

BOOL WINAPI
PathFindOnPathW(LPWSTR Unknown1, LPCWSTR* Unknown2)
{
  STUB;
  return 0;
}

VOID WINAPI
RLBuildListOfPaths(VOID)
{
  STUB;
}

VOID WINAPI
SHCLSIDFromString(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHFind_InitMenuPopup(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHLoadOLE(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILGetSize(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILGetNext(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILAppend(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
ILFree(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILGlobalFree(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
ILCreateFromPath(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
PathGetExtensionA(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
PathGetExtensionW(DWORD Unknown1)
{
  STUB;
}

BOOL WINAPI
PathIsDirectoryA(LPCSTR Unknown1)
{
  STUB;
  return 0;
}

BOOL WINAPI
PathIsDirectoryW(LPCWSTR Unknown1)
{
  STUB;
  return 0;
}

VOID WINAPI
SHNetConnectionDialog(VOID)
{
  STUB;
}

VOID WINAPI
SHRunControlPanel(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHSimpleIDListFromPath(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
StrToOleStr(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
Win32DeleteFile(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHCreateDirectory(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

DWORD WINAPI
CallCPLEntry16(HMODULE hMod, FARPROC pFunc, DWORD dw3, DWORD dw4, DWORD dw5, DWORD dw6)
//VOID WINAPI
//CallCPLEntry16(VOID)
{
  STUB;
  return 0;
}
 
VOID WINAPI
SHAddFromPropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHCreatePropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHDestroyPropSheetExtArray(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHReplaceFromPropSheetExtArray(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
PathCleanupSpecA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHCreateLinks(VOID)
{
  STUB;
}

VOID WINAPI
SHValidateUNC(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHCreateShellFolderViewEx(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHGetSpecialFolderPath(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHSetInstanceExplorer(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
DAD_SetDragImageFromListView(VOID)
{
  STUB;
}

VOID WINAPI
SHObjectProperties(VOID)
{
  STUB;
}

/*
//VOID WINAPI
//SHGetNewLinkInfo(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
#ifndef _MSC_VER
BOOL WINAPI
SHGetNewLinkInfo(LPCTSTR pszLinkTo, LPCTSTR pszDir, LPTSTR pszName, BOOL* pfMustCopy, UINT uFlags)
{
  STUB;
}
#endif
 */
#ifdef _MSC_VER
BOOL WINAPI
SHGetNewLinkInfoA(LPCTSTR pszLinkTo, LPCTSTR pszDir, LPTSTR pszName, BOOL* pfMustCopy, UINT uFlags)
#else
BOOL WINAPI
SHGetNewLinkInfoA(VOID)
#endif
{
  STUB;
  return 0;
}

#ifdef _MSC_VER
BOOL WINAPI
SHGetNewLinkInfoW(LPCWSTR pszLinkTo, LPCWSTR pszDir, LPWSTR pszName, BOOL* pfMustCopy, UINT uFlags)
#else
BOOL WINAPI
SHGetNewLinkInfoW(VOID)
#endif
{
  STUB;
  return 0;
}

VOID WINAPI
RegisterShellHook(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID 
ShellMessageBoxW(DWORD Unknown1, ...)
{
  STUB;
}

VOID 
ShellMessageBoxA(DWORD Unknown1, ...)
{
  STUB;
}

VOID WINAPI
ArrangeWindows(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SHHandleDiskFull(VOID)
{
  STUB;
}

VOID WINAPI
SHFree(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHAlloc(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHGlobalDefect(VOID)
{
  STUB;
}

VOID WINAPI
SHAbortInvokeCommand(VOID)
{
  STUB;
}

VOID WINAPI
SHGetFileIcon(VOID)
{
  STUB;
}

VOID WINAPI
SHLocalAlloc(VOID)
{
  STUB;
}

VOID WINAPI
SHLocalFree(VOID)
{
  STUB;
}

VOID WINAPI
SHLocalReAlloc(VOID)
{
  STUB;
}

VOID WINAPI
AddCommasW(VOID)
{
  STUB;
}

VOID WINAPI
ShortSizeFormatW(VOID)
{
  STUB;
}

VOID WINAPI
Printer_LoadIconsW(VOID)
{
  STUB;
}

VOID WINAPI
Link_AddExtraDataSection(VOID)
{
  STUB;
}

VOID WINAPI
Link_ReadExtraDataSection(VOID)
{
  STUB;
}

VOID WINAPI
Link_RemoveExtraDataSection(VOID)
{
  STUB;
}

VOID WINAPI
Int64ToString(VOID)
{
  STUB;
}

VOID WINAPI
LargeIntegerToString(VOID)
{
  STUB;
}

VOID WINAPI
Printers_GetPidl(VOID)
{
  STUB;
}

VOID WINAPI
Printer_AddPrinterPropPages(VOID)
{
  STUB;
}

VOID WINAPI
Printers_RegisterWindowW(VOID)
{
  STUB;
}

VOID WINAPI
Printers_UnregisterWindow(VOID)
{
  STUB;
}

VOID WINAPI
SHStartNetConnectionDialog(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
shell32_243(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHInitRestricted(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHGetDataFromIDListA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SHGetDataFromIDListW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

int WINAPI
PathParseIconLocationA(LPSTR Unknown1)
{
  STUB;
  return 0;
}

int WINAPI
PathParseIconLocationW(LPWSTR Unknown1)
{
  STUB;
  return 0;
}

VOID WINAPI
PathRemoveExtensionA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathRemoveExtensionW(LPWSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathRemoveArgsA(LPSTR Unknown1)
{
  STUB;
}

VOID WINAPI
PathRemoveArgsW(LPWSTR Unknown1)
{
  STUB;
}

VOID WINAPI
SheChangeDirA(VOID)
{
  STUB;
}

VOID WINAPI
SheChangeDirExA(VOID)
{
  STUB;
}

VOID WINAPI
SheChangeDirExW(VOID)
{
  STUB;
}

VOID WINAPI
SheChangeDirW(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SheConvertPathW(VOID)
{
  STUB;
}

VOID WINAPI
SheFullPathA(VOID)
{
  STUB;
}

VOID WINAPI
SheFullPathW(VOID)
{
  STUB;
}

VOID WINAPI
SheGetCurDrive(VOID)
{
  STUB;
}

VOID WINAPI
SheGetDirA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SheGetDirExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SheGetDirW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SheGetPathOffsetW(VOID)
{
  STUB;
}

VOID WINAPI
SheRemoveQuotesA(VOID)
{
  STUB;
}

VOID WINAPI
SheRemoveQuotesW(VOID)
{
  STUB;
}

VOID WINAPI
SheSetCurDrive(VOID)
{
  STUB;
}

VOID WINAPI
SheShortenPathA(VOID)
{
  STUB;
}

VOID WINAPI
SheShortenPathW(VOID)
{
  STUB;
}

UINT WINAPI
ShellAboutA (HWND Unknown1, LPCSTR Unknown2, LPCSTR Unknown3, HICON Unknown4)
{
  STUB;
  return 0;
}

UINT WINAPI
//ShellAboutW (HWND Unknown1, const LPCWSTR Unknown2, const LPCWSTR Unknown3, HICON Unknown4)
ShellAboutW (HWND Unknown1, LPCWSTR Unknown2, LPCWSTR Unknown3, HICON Unknown4)
{
  STUB;
  return 0;
}
/*
HINSTANCE WINAPI
ShellExecuteA (HWND Unknown1, const char * Unknown2, const char * Unknown3, char * Unknown4, const char * Unknown5, int Unknown6)
{
  STUB;
}
 */
/*
//VOID WINAPI
//ShellExecuteEx(DWORD Unknown1)
BOOL WINAPI
ShellExecuteEx(LPSHELLEXECUTEINFO lpExecInfo)
{
  STUB;
  return 0;
}
 */
//VOID WINAPI
//ShellExecuteExA(DWORD Unknown1)
BOOL WINAPI
ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
  STUB;
  return 0;
}

//VOID WINAPI
//ShellExecuteExW(DWORD Unknown1)
BOOL WINAPI
ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
  STUB;
  return 0;
}

VOID WINAPI
ShellExecute (DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

HINSTANCE WINAPI
ShellExecuteW (HWND Unknown1, const LPCWSTR Unknown2, const LPCWSTR Unknown3, LPCWSTR Unknown4, const LPCWSTR Unknown5, int Unknown6)
{
  STUB;
  return 0;
}
/*
//VOID WINAPI
//Shell_NotifyIcon(DWORD Unknown1, DWORD Unknown2)
BOOL WINAPI Shell_NotifyIcon(DWORD dwMessage, PNOTIFYICONDATA pnid)
{
  STUB;
  return 0;
}
 */
//VOID WINAPI
//Shell_NotifyIconA(DWORD Unknown1, DWORD Unknown2)
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
  STUB;
  return 0;
}

//VOID WINAPI
//Shell_NotifyIconW(DWORD Unknown1, DWORD Unknown2)
//BOOL WINAPI Shell_NotifyIconW(DWORD,PNOTIFYICONDATAW);
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid)
{
  STUB;
  return 0;
}

VOID WINAPI
Shl1632_ThunkData32(VOID)
{
  STUB;
}

VOID WINAPI
Shl3216_ThunkData32(VOID)
{
  STUB;
}

VOID WINAPI
StrCpyNA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}
 
VOID WINAPI
StrNCpyA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
StrRStrA(VOID)
{
  STUB;
}

VOID WINAPI
StrRStrW(VOID)
{
  STUB;
}

VOID WINAPI
SHRegCloseKey(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHRegOpenKeyA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHRegOpenKeyW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHRegQueryValueA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHRegQueryValueExA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
SHRegQueryValueW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHRegQueryValueExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
SHRegDeleteKeyW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHAllocShared(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
  STUB;
}

VOID WINAPI
SHLockShared(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHUnlockShared(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHFreeShared(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
RealDriveType(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
RealDriveTypeFlags(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
NTSHChangeNotifyRegister(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
  STUB;
}

VOID WINAPI
NTSHChangeNotifyDeregister(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHChangeNotifyReceive(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHChangeNotification_Lock(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHChangeNotification_Unlock(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHChangeRegistrationReceive(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
ReceiveAddToRecentDocs(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHWaitOp_Operate(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

BOOL WINAPI
PathIsSameRootA(LPCSTR Unknown1, LPCSTR Unknown2)
{
  STUB;
  return 0;
}

BOOL WINAPI
PathIsSameRootW(LPCWSTR Unknown1, LPCWSTR Unknown2)
{
  STUB;
  return 0;
}

VOID WINAPI
ReadCabinetState(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
WriteCabinetState(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
PathProcessCommand(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
shell32_654(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
FileIconInit(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
IsUserAdmin(VOID)
{
  STUB;
}

VOID WINAPI
shell32_714(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
FOOBAR1217(VOID)
{
  STUB;
}

VOID WINAPI
CheckEscapesA(VOID)
{
  STUB;
}

VOID WINAPI
CheckEscapesW(VOID)
{
  STUB;
}

LPWSTR WINAPI
CommandLineToArgvW(DWORD Unknown1, DWORD Unknown2)
//CommandLineToArgvW(LPCWSTR lpCmdLine, int* pNumArgs)
{
// lpCmdLine  - pointer to a command-line string
// pNumArgs   - receives the argument count
  STUB;
  return 0;
}
/*
HRESULT WINAPI
Control_FillCache_RunDLL(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
//VOID WINAPI
//Control_FillCache_RunDLL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
  return 0;
}
 */
//VOID WINAPI
//Control_FillCache_RunDLLA(VOID)
HRESULT WINAPI
Control_FillCache_RunDLLA(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
  STUB;
  return 0;
}

//VOID WINAPI
//Control_FillCache_RunDLLW(VOID)
HRESULT WINAPI
Control_FillCache_RunDLLW(HWND hWnd, HANDLE hModule, DWORD w, DWORD x)
{
  STUB;
  return 0;
}

VOID Control_RunDLL(HWND hWnd, HINSTANCE hInst_unused, LPCWSTR lpCmdLine, DWORD nCmdShow);

VOID WINAPI
Control_RunDLLA(HWND hWnd, HINSTANCE hInst_unused, LPCSTR lpCmdLine, DWORD nCmdShow)
{
    int reqSize = MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, 0, 0) * sizeof(TCHAR);
    if (reqSize) {
//        LPWSTR pCmdLine = (LPWSTR)malloc(reqSize + 10);
        LPWSTR pCmdLine = (LPWSTR)_alloca(reqSize);
        if (MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, pCmdLine, reqSize)) {
            Control_RunDLL(hWnd, hInst_unused, pCmdLine, nCmdShow);
        }
//        free(pCmdLine);
    }
}

VOID WINAPI
Control_RunDLLW(HWND hWnd, HINSTANCE hInst_unused, LPCWSTR lpCmdLine, DWORD nCmdShow)
{
    Control_RunDLL(hWnd, hInst_unused, lpCmdLine, nCmdShow);
}

VOID WINAPI
DllInstall(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

/*
//VOID WINAPI
//DoEnvironmentSubstA(DWORD Unknown1, DWORD Unknown2)
DWORD WINAPI DoEnvironmentSubstA(LPCTSTR pszString, UINT cbSize)
{
  STUB;
}
 */
//VOID WINAPI
//DoEnvironmentSubstW(DWORD Unknown1, DWORD Unknown2)
DWORD WINAPI DoEnvironmentSubstW(LPCTSTR pszString, UINT cbSize)
{
  STUB;
  return 0;
}

VOID WINAPI
DragQueryFileAorW(VOID)
{
  STUB;
}

HICON WINAPI
DuplicateIcon (HINSTANCE Unknown1, HICON Unknown2)
{
  STUB;
  return 0;
}

HICON WINAPI
ExtractAssociatedIconA(HINSTANCE Unknown1, LPCSTR Unknown2, PWORD Unknown3)
{
  STUB;
  return 0;
}

VOID WINAPI
ExtractAssociatedIconExA(VOID)
{
  STUB;
}

VOID WINAPI
ExtractAssociatedIconExW(VOID)
{
  STUB;
}

HICON WINAPI
ExtractAssociatedIconW (HINSTANCE Unknown1, LPCWSTR Unknown2, WORD * Unknown3)
{
  STUB;
  return 0;
}

HICON WINAPI
ExtractIconA (HINSTANCE Unknown1, const char * Unknown2, unsigned int Unknown3)
{
  STUB;
  return 0;
}

/*
//VOID WINAPI
//ExtractIconEx(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
UINT WINAPI ExtractIconEx(LPCTSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
{
// lpszFile         - file name
// nIconIndex       - icon index
// phiconLarge      - large icon array
// phiconSmall      - small icon array
// nIcons           - number of icons to extract
  STUB;
  return 0;
}
 */

//VOID WINAPI
//ExtractIconExA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
//HICON WINAPI ExtractIconExA(LPCSTR,int,HICON*,HICON*,UINT);
//HICON WINAPI ExtractIconExA(LPCSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
#ifdef _MSC_VER
UINT
#else
HICON
#endif
WINAPI
ExtractIconExA(LPCSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
{
  STUB;
  return 0;
}

//VOID WINAPI
//ExtractIconExW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
//HICON WINAPI ExtractIconExW(LPCWSTR,int,HICON*,HICON*,UINT); // from ming header
//UINT WINAPI ExtractIconExW(LPCTSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
//HICON WINAPI ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
#ifdef _MSC_VER
UINT
#else
HICON
#endif
WINAPI
ExtractIconExW(LPCWSTR lpszFile, int nIconIndex, HICON* phiconLarge, HICON* phiconSmall, UINT nIcons)
{
  STUB;
  return 0;
}

HICON
//WINAPI
WINAPI
ExtractIconW (HINSTANCE Unknown1, const LPCWSTR Unknown2, unsigned int Unknown3)
{
  STUB;
  return 0;
}

VOID WINAPI
ExtractIconResInfoA(VOID)
{
  STUB;
}

VOID WINAPI
ExtractIconResInfoW(VOID) 
{
  STUB;
}

VOID WINAPI
ExtractVersionResource16W(VOID)
{
  STUB;
}

VOID WINAPI
FindExeDlgProc(VOID)
{
  STUB;
}

HINSTANCE
WINAPI
FindExecutableA(const char * Unknown1, const char * Unknown2, char * Unknown3)
{
  STUB;
  return 0;
}

HINSTANCE
WINAPI
// FindExecutableW(LPCWSTR,LPCWSTR,LPWSTR);
FindExecutableW(const LPCWSTR Unknown1, const LPCWSTR Unknown2, LPWSTR Unknown3)
{
  STUB;
  return 0;
}

VOID WINAPI
FreeIconList(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
InternalExtractIconListA(VOID)
{
  STUB;
}

VOID WINAPI
InternalExtractIconListW(VOID)
{
  STUB;
}

VOID WINAPI
OpenAs_RunDLL(VOID)
{
  STUB;
}

VOID WINAPI
OpenAs_RunDLLA(VOID)
{
  STUB;
}

VOID WINAPI
OpenAs_RunDLLW(VOID)
{
  STUB;
}

VOID WINAPI
PrintersGetCommand_RunDLL(VOID)
{
  STUB;
}

VOID WINAPI
PrintersGetCommand_RunDLLA(VOID)
{
  STUB;
}

VOID WINAPI
PrintersGetCommand_RunDLLW(VOID)
{
  STUB;
}

VOID WINAPI
RealShellExecuteA(VOID)
{
  STUB;
}

VOID WINAPI
RealShellExecuteExA(VOID)
{
  STUB;
}

VOID WINAPI
RealShellExecuteExW(VOID)
{
  STUB;
}

VOID WINAPI
RealShellExecuteW(VOID)
{
  STUB;
}

VOID WINAPI
RegenerateUserEnvironment(VOID)
{
  STUB;
}

//void WINAPI
VOID WINAPI
SHAddToRecentDocs (UINT Unknown1, LPCVOID Unknown2)
{
  STUB;
}

//VOID WINAPI
//SHAppBarMessage(DWORD Unknown1, DWORD Unknown2) 
UINT WINAPI SHAppBarMessage(DWORD dwMessage, PAPPBARDATA pData)
{
  STUB;
  return 0;
}



LPITEMIDLIST
//WINAPI
WINAPI
SHBrowseForFolder(LPBROWSEINFO Unknown1)
{
  STUB;
  return 0;
}

VOID WINAPI
SHBrowseForFolderA(DWORD Unknown1) 
{
  STUB;
}

VOID WINAPI
SHBrowseForFolderW(DWORD Unknown1) 
{
  STUB;
}

//void WINAPI
VOID WINAPI
SHChangeNotify (LONG Unknown1, UINT Unknown2, LPCVOID Unknown3, LPCVOID Unknown4)
{
  STUB;
}

VOID WINAPI
ShellHookProc(VOID)
{
  STUB;
}


//VOID WINAPI
//SHEmptyRecycleBinA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3) 
HRESULT WINAPI SHEmptyRecycleBinA(HWND hwnd, LPCTSTR pszRootPath, DWORD dwFlags)
{
  STUB;
  return 0;
}

//VOID WINAPI
//SHEmptyRecycleBinW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3) 
HRESULT WINAPI SHEmptyRecycleBinW(HWND hwnd, LPCTSTR pszRootPath, DWORD dwFlags)
{
  STUB;
  return 0;
}
/*
int WINAPI
SHFileOperation (LPSHFILEOPSTRUCT Unknown1)
{
  STUB;
  return 0;
}
 */
//VOID WINAPI
//SHFileOperationA(DWORD Unknown1)
//int WINAPI

INT WINAPI
SHFileOperationA(LPSHFILEOPSTRUCTA lpFileOp)
{
  STUB;
  return 0;
}

//VOID WINAPI
//SHFileOperationW(DWORD Unknown1)
//int WINAPI
INT WINAPI
SHFileOperationW(LPSHFILEOPSTRUCTW lpFileOp)
{
  STUB;
  return 0;
}

VOID WINAPI
SHFormatDrive(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

//void WINAPI
VOID WINAPI
SHFreeNameMappings (HANDLE Unknown1)
{
  STUB;
}

VOID WINAPI
SHGetDesktopFolder(DWORD Unknown1)
{
  STUB;
}
/*
//DWORD WINAPI
//SHGetFileInfo (LPCTSTR Unknown1, DWORD Unknown2, SHFILEINFO FAR * Unknown3, UINT Unknown4, UINT Unknown5)
DWORD_PTR WINAPI
SHGetFileInfo(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFO* psfi, UINT cbFileInfo, UINT uFlags)
{
  STUB;
  return 0;
}
 */

//VOID WINAPI
//SHGetFileInfoA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
//DWORD WINAPI SHGetFileInfoA(LPCSTR,DWORD,SHFILEINFOA*,UINT,UINT);
//DWORD_PTR WINAPI
//DWORD WINAPI
DWORD WINAPI
SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA* psfi, UINT cbFileInfo, UINT uFlags)
{
  STUB;
  return 0;
}

//VOID WINAPI
//SHGetFileInfoW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
//DWORD_PTR WINAPI
//DWORD WINAPI
DWORD WINAPI
SHGetFileInfoW(LPCTSTR pszPath, DWORD dwFileAttributes, SHFILEINFOW* psfi, UINT cbFileInfo, UINT uFlags)
{
  STUB;
  return 0;
}

VOID WINAPI
SHGetInstanceExplorer(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHGetMalloc(DWORD Unknown1)
{
  STUB;
}

//WINBOOL WINAPI
BOOL WINAPI
SHGetPathFromIDList (LPCITEMIDLIST Unknown1, LPTSTR Unknown2)
{
  STUB;
  return 0;
}

VOID WINAPI
SHGetPathFromIDListA(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHGetPathFromIDListW(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

VOID WINAPI
SHGetSettings(DWORD Unknown1, DWORD Unknown2)
{
  STUB;
}

HRESULT
//WINAPI
WINAPI
SHGetSpecialFolderLocation (HWND Unknown1, int Unknown2, LPITEMIDLIST * Unknown3)
{
  STUB;
  return 0;
}

VOID WINAPI
SHHelpShortcuts_RunDLL(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHHelpShortcuts_RunDLLA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

VOID WINAPI
SHHelpShortcuts_RunDLLW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4) 
{
  STUB;
}

VOID WINAPI
SHLoadInProc(DWORD Unknown1)
{
  STUB;
}

//VOID WINAPI
//SHQueryRecycleBinA(DWORD Unknown1, DWORD Unknown2) 
HRESULT WINAPI
SHQueryRecycleBinA(LPCTSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
  STUB;
  return 0;
}

//VOID WINAPI
//SHQueryRecycleBinW(DWORD Unknown1, DWORD Unknown2) 
HRESULT WINAPI
SHQueryRecycleBinW(LPCTSTR pszRootPath, LPSHQUERYRBINFO pSHQueryRBInfo)
{
  STUB;
  return 0;
}

VOID WINAPI
SHUpdateRecycleBinIcon(VOID) 
{
  STUB;
}

VOID WINAPI
WOWShellExecute(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7)
{
  STUB;
}

STDAPI
DllCanUnloadNow(VOID)
{
  STUB;
  return 0;
}

VOID WINAPI
DllGetVersion(DWORD Unknown1)
{
  STUB;
}

VOID WINAPI
SHGetFreeDiskSpace(VOID)
{
  STUB;
}

VOID WINAPI
SHGetSpecialFolderPathA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
  STUB;
}

VOID WINAPI
SHGetFolderPathA(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SHGetFolderPathW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}

VOID WINAPI
SHGetFolderLocation(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4, DWORD Unknown5)
{
  STUB;
}


