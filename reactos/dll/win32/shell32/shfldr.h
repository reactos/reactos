
/*
 *  Virtual Folder
 *  common definitions
 *
 *  Copyright 1997 Marcus Meissner
 *  Copyright 1998, 1999, 2002 Juergen Schmied
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _SHFLDR_H_
#define _SHFLDR_H_

#define CHARS_IN_GUID 39

typedef struct {
    int colnameid;
    int pcsFlags;
    int fmt;
    int cxChar;
} shvheader;

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)

LPCWSTR GetNextElementW (LPCWSTR pszNext, LPWSTR pszOut, DWORD dwOut);
HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc, LPITEMIDLIST * pidlInOut,
                  LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes);

HRESULT HCR_GetClassName(REFIID riid, LPSTRRET strRet);

HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet);

HRESULT SHELL32_BindToFS (LPCITEMIDLIST pidlRoot,
                 LPCWSTR pathRoot, LPCITEMIDLIST pidlComplete, REFIID riid, LPVOID * ppvOut);

HRESULT SHELL32_CompareIDs (IShellFolder * iface, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
LPITEMIDLIST SHELL32_CreatePidlFromBindCtx(IBindCtx *pbc, LPCWSTR path);

HRESULT SHELL32_BindToGuidItem(LPCITEMIDLIST pidlRoot,
                               PCUIDLIST_RELATIVE pidl,
                               LPBC pbcReserved,
                               REFIID riid,
                               LPVOID *ppvOut);

HRESULT SHELL32_GetGuidItemAttributes (IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes);

HRESULT SHELL32_GetFSItemAttributes(IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes);

HRESULT SHELL32_GetDisplayNameOfGUIDItem(IShellFolder2* psf, LPCWSTR pszFolderPath, PCUITEMID_CHILD pidl, DWORD dwFlags, LPSTRRET strRet);

HRESULT SHELL32_SetNameOfGuidItem(PCUITEMID_CHILD pidl, LPCOLESTR lpName, DWORD dwFlags, PITEMID_CHILD *pPidlOut);

HRESULT SHELL32_GetDetailsOfGuidItem(IShellFolder2* psf, PCUITEMID_CHILD pidl, UINT iColumn, SHELLDETAILS *psd);

HRESULT SH_ParseGuidDisplayName(IShellFolder * pFolder,
                                HWND hwndOwner,
                                LPBC pbc,
                                LPOLESTR lpszDisplayName,
                                DWORD *pchEaten,
                                PIDLIST_RELATIVE *ppidl,
                                DWORD *pdwAttributes);

extern "C"
BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey);

static __inline int SHELL32_GUIDToStringA (REFGUID guid, LPSTR str)
{
    return sprintf(str, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

static __inline int SHELL32_GUIDToStringW (REFGUID guid, LPWSTR str)
{
    static const WCHAR fmtW[] =
     { '{','%','0','8','l','x','-','%','0','4','x','-','%','0','4','x','-',
     '%','0','2','x','%','0','2','x','-',
     '%','0','2','x','%','0','2','x','%','0','2','x','%','0','2','x',
     '%','0','2','x','%','0','2','x','}',0 };
    return swprintf(str, fmtW,
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void SHELL_FS_ProcessDisplayFilename(LPWSTR szPath, DWORD dwFlags);
BOOL SHELL_FS_HideExtension(LPWSTR pwszPath);

#endif /* _SHFLDR_H_ */
