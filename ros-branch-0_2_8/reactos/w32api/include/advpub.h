/*
 * Copyright 2004 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_ADVPUB_H
#define __WINE_ADVPUB_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _CabInfo {
    PSTR  pszCab;
    PSTR  pszInf;
    PSTR  pszSection;
    char  szSrcPath[MAX_PATH];
    DWORD dwFlags;
} CABINFO, *PCABINFO;

typedef struct _StrEntry {
    LPSTR pszName;
    LPSTR pszValue;
} STRENTRY, *LPSTRENTRY;

typedef const STRENTRY CSTRENTRY;
typedef CSTRENTRY *LPCSTRENTRY;

typedef struct _StrTable {
    DWORD cEntries;
    LPSTRENTRY pse;
} STRTABLE, *LPSTRTABLE;

typedef const STRTABLE CSTRTABLE;
typedef CSTRTABLE *LPCSTRTABLE;

/* Flags for RunSetupCommand */
#define RSC_FLAG_INF                0x00000001
#define RSC_FLAG_SKIPDISKSPACECHECK 0x00000002
#define RSC_FLAG_QUIET              0x00000004
#define RSC_FLAG_NGCONV             0x00000008
#define RSC_FLAG_UPDHLPDLLS         0x00000010
#define RSC_FLAG_DELAYREGISTEROCX   0x00000200
#define RSC_FLAG_SETUPAPI           0x00000400

/* Flags for DelNode */
#define ADN_DEL_IF_EMPTY            0x00000001
#define ADN_DONT_DEL_SUBDIRS        0x00000002
#define ADN_DONT_DEL_DIR            0x00000004

HRESULT WINAPI RunSetupCommand(HWND hWnd,
     LPCSTR szCmdName, LPCSTR szInfSection, LPCSTR szDir, LPCSTR lpszTitle,
     HANDLE *phEXE, DWORD dwFlags, LPVOID pvReserved);
HRESULT WINAPI DelNode(LPCSTR pszFileOrDirName, DWORD dwFlags);
DWORD WINAPI NeedRebootInit(VOID);
BOOL WINAPI NeedReboot(DWORD dwRebootCheck);
HRESULT WINAPI RegInstall(HMODULE hm, LPCSTR pszSection, LPCSTRTABLE pstTable);
HRESULT WINAPI GetVersionFromFile(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);
HRESULT WINAPI GetVersionFromFileEx(LPSTR lpszFilename, LPDWORD pdwMSVer, LPDWORD pdwLSVer, BOOL bVersion);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_ADVPUB_H */
