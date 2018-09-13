/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WSHELL.H
 *  WOW32 16-bit SHELL API support
 *
 *  History:
 *  Created 14-April-1992 by Chandan Chauhan (ChandanC)
--*/

ULONG FASTCALL WS32DoEnvironmentSubst(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegOpenKey(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegCreateKey(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegCloseKey(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegDeleteKey(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegSetValue(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegQueryValue(PVDMFRAME pFrame);
ULONG FASTCALL WS32RegEnumKey(PVDMFRAME pFrame);
ULONG FASTCALL WS32DragAcceptFiles(PVDMFRAME pFrame);
ULONG FASTCALL WS32DragQueryFile(PVDMFRAME pFrame);
ULONG FASTCALL WS32DragFinish (PVDMFRAME pFrame);
ULONG FASTCALL WS32DragQueryPoint (PVDMFRAME pFrame);
ULONG FASTCALL WS32ShellAbout (PVDMFRAME pFrame);
ULONG FASTCALL WS32ShellExecute (PVDMFRAME pFrame);
ULONG FASTCALL WS32FindExecutable (PVDMFRAME pFrame);
ULONG FASTCALL WS32ExtractIcon (PVDMFRAME pFrame);

LONG  APIENTRY WOWRegDeleteKey(HKEY hKey, LPCSTR lpszSubKey);

ULONG ConvertToWin31Error(ULONG ul);
LPSZ  Remove_Classes (LPSZ psz);

WORD W32ShellExecuteCallBack (LPSZ lpszCmdLine, WORD fuCmdShow, LPSZ lpszNewDir);

typedef struct _DROPALIAS {
   struct _DROPALIAS FAR *lpNext;
   HAND32 h32;
   HAND16 h16;
   DWORD  dwFlags;
} DROPALIAS, *LPDROPALIAS;

#define HDROP_H16         0x0001
#define HDROP_H32         0x0002
#define HDROP_FREEALIAS   0x0004
#define HDROP_ALLOCALIAS  0x0008
#define HDROP_COPYDATA    0x0010

HAND16 CopyDropFilesFrom32(HANDLE h32);
HANDLE CopyDropFilesFrom16(HAND16 h16);
DWORD  DropFilesHandler(HAND16 h16, HANDLE h32, UINT flInput);
BOOL   FindAndReleaseHDrop16 (HAND16 h16);

