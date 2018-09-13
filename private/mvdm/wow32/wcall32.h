/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WCALL32.H
 *  WOW32 16-bit resource support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/


/* Function prototypes
 */
HANDLE  APIENTRY W32LocalAlloc(UINT dwFlags, UINT dwBytes, HANDLE hInstance);
HANDLE	APIENTRY W32LocalReAlloc(HANDLE hMem, UINT dwBytes, UINT dwFlags, HANDLE hInstance, PVOID* ppv);
LPSTR	APIENTRY W32LocalLock(HANDLE hMem, HANDLE hInstance);
BOOL	APIENTRY W32LocalUnlock(HANDLE hMem, HANDLE hInstance);
DWORD	APIENTRY W32LocalSize(HANDLE hMem, HANDLE hInstance);
HANDLE	APIENTRY W32LocalFree(HANDLE hMem, HANDLE hInstance);
ULONG   APIENTRY W32GetExpWinVer(HANDLE Inst);
DWORD   APIENTRY W32InitDlg(HWND hDlg, LONG lParam);
WORD    APIENTRY W32GlobalAlloc16(UINT uFlags, DWORD dwBytes);
VOID	APIENTRY W32GlobalFree16(WORD selector);
DWORD	WOWRtlGetExpWinVer(HANDLE hmod);
int     APIENTRY W32EditNextWord (LPSZ lpszEditText, int ichCurrentWord,
                                  int cbEditText, int action, DWORD dwProc16);
