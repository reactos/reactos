
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

HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet);

LPITEMIDLIST SHELL32_CreatePidlFromBindCtx(IBindCtx *pbc, LPCWSTR path);

HRESULT SHELL32_GetFSItemAttributes(IShellFolder * psf, LPCITEMIDLIST pidl, LPDWORD pdwAttributes);

HRESULT SHELL32_CompareDetails(IShellFolder2* isf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

HRESULT SHELL32_CompareChildren(IShellFolder2* psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);

#ifdef __cplusplus
HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, PERSIST_FOLDER_TARGET_INFO* ppfti,
                LPCITEMIDLIST pidlChild, const GUID* clsid, REFIID riid, LPVOID *ppvOut);

HRESULT SHELL32_CoCreateInitSF (LPCITEMIDLIST pidlRoot, const GUID* clsid,
                                int csidl, REFIID riid, LPVOID *ppvOut);
#endif

HRESULT SHELL32_BindToSF (LPCITEMIDLIST pidlRoot, PERSIST_FOLDER_TARGET_INFO* ppfti,
                LPCITEMIDLIST pidl, const GUID* clsid, REFIID riid, LPVOID *ppvOut);

extern "C"
BOOL HCR_RegOpenClassIDKey(REFIID riid, HKEY *hkey);

void AddFSClassKeysToArray(PCUITEMID_CHILD pidl, HKEY* array, UINT* cKeys);

HRESULT CDefViewBckgrndMenu_CreateInstance(IShellFolder* psf, REFIID riid, void **ppv);

HRESULT SH_GetApidlFromDataObject(IDataObject *pDataObject, PIDLIST_ABSOLUTE* ppidlfolder, PUITEMID_CHILD **apidlItems, UINT *pcidl);

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

void AddClassKeyToArray(const WCHAR * szClass, HKEY* array, UINT* cKeys);

#ifdef __cplusplus

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, DWORD resId)
{
    return SHSetStrRet(pStrRet, shell32_hInstance, resId);
}

#endif

#endif /* _SHFLDR_H_ */
