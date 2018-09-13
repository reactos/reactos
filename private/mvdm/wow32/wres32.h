/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WRES32.H
 *  WOW32 16-bit resource support
 *
 *  History:
 *  Created 11-Mar-1991 by Jeff Parsons (jeffpar)
--*/

/* Function prototypes
 */
HANDLE  APIENTRY W32FindResource(HANDLE hModule, LPCSTR lpType, LPCSTR lpName, WORD wLangId);
HANDLE	APIENTRY W32LoadResource(HANDLE hModule, HANDLE hResInfo);
BOOL	APIENTRY W32FreeResource(HANDLE hResData, HANDLE hModule);
LPSTR	APIENTRY W32LockResource(HANDLE hResData, HANDLE hModule);
BOOL	APIENTRY W32UnlockResource(HANDLE hResData, HANDLE hModule);
DWORD	APIENTRY W32SizeofResource(HANDLE hModule, HANDLE hResInfo);
