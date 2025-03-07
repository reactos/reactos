
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
    WORD colnameid; // Column title text resource id passed to LoadString
    WORD colstate;  // SHCOLSTATEF returned by IShellFolder2::GetDefaultColumnState
                    // HACK: SHCOLSTATEF truncated to WORD to reduce .rdata section size
    WORD fmt;       // LVCFMT_*
    WORD cxChar;    // Column width hint
} shvheader;
/* 
 * CFSFolder column indices. CDesktopFolder MUST use the same indices!
 * According to the documentation for IShellFolder2::GetDetailsOf,
 * the first 4 columns for SFGAO_FILESYSTEM items must be Name, Size, Type, Modified date
For Details See:
https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellfolder2-getdetailsof
 */
#define SHFSF_COL_NAME          0
#define SHFSF_COL_SIZE          1
#define SHFSF_COL_TYPE          2 // SHGFI_TYPENAME
#define SHFSF_COL_MDATE         3 // Modified date
#define SHFSF_COL_FATTS         4 // File attributes
#define SHFSF_COL_COMMENT       5

typedef struct _REQUIREDREGITEM
{
    REFCLSID clsid;
    LPCSTR pszCpl;
    BYTE Order; // According to Geoff Chappell, required items have a fixed sort order
} REQUIREDREGITEM;

typedef struct _REGFOLDERINFO
{
    PIDLTYPE PidlType;
    BYTE Count; // Count of required items
    const REQUIREDREGITEM *Items;
    REFCLSID clsid;
    LPCWSTR pszParsingPath;
    LPCWSTR pszEnumKeyName;
} REGFOLDERINFO;

typedef struct _REGFOLDERINITDATA
{
    IShellFolder *psfOuter;
    const REGFOLDERINFO *pInfo;
} REGFOLDERINITDATA, *PREGFOLDERINITDATA;

HRESULT CRegFolder_CreateInstance(PREGFOLDERINITDATA pInit, LPCITEMIDLIST pidlRoot, REFIID riid, void **ppv);

#define GET_SHGDN_FOR(dwFlags)         ((DWORD)dwFlags & (DWORD)0x0000FF00)
#define GET_SHGDN_RELATION(dwFlags)    ((DWORD)dwFlags & (DWORD)0x000000FF)
#define IS_SHGDN_FOR_PARSING(flags) ( ((flags) & (SHGDN_FORADDRESSBAR | SHGDN_FORPARSING)) == SHGDN_FORPARSING)
#define IS_SHGDN_DESKTOPABSOLUTEPARSING(flags) ( ((flags) & (SHGDN_FORADDRESSBAR | SHGDN_FORPARSING | 0xFF)) == SHGDN_FORPARSING)

static inline SFGAOF 
SHELL_CreateFolderEnumItemAttributeQuery(SHCONTF Flags, BOOL ForRegItem)
{
    SFGAOF query = SFGAO_FOLDER | (ForRegItem ? SFGAO_NONENUMERATED : 0);
    if (!(Flags & SHCONTF_INCLUDEHIDDEN))
        query |= SFGAO_HIDDEN;
    if (!(Flags & SHCONTF_INCLUDESUPERHIDDEN))
        query |= SFGAO_HIDDEN | SFGAO_SYSTEM;
    return query;
}

SHCONTF
SHELL_GetDefaultFolderEnumSHCONTF();

BOOL
SHELL_IncludeItemInFolderEnum(IShellFolder *pSF, PCUITEMID_CHILD pidl, SFGAOF Query, SHCONTF Flags);

HRESULT
Shell_NextElement(
    _Inout_ LPWSTR *ppch,
    _Out_ LPWSTR pszOut,
    _In_ INT cchOut,
    _In_ BOOL bValidate);

HRESULT SHELL32_ParseNextElement (IShellFolder2 * psf, HWND hwndOwner, LPBC pbc, LPITEMIDLIST * pidlInOut,
                  LPOLESTR szNext, DWORD * pEaten, DWORD * pdwAttributes);

HRESULT SHELL32_GetDisplayNameOfChild (IShellFolder2 * psf, LPCITEMIDLIST pidl, DWORD dwFlags, LPSTRRET strRet);

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

HRESULT CDefViewBckgrndMenu_CreateInstance(IShellFolder* psf, REFIID riid, void **ppv);

HRESULT SH_GetApidlFromDataObject(IDataObject *pDataObject, PIDLIST_ABSOLUTE* ppidlfolder, PUITEMID_CHILD **apidlItems, UINT *pcidl);

static __inline int SHELL32_GUIDToStringA (REFGUID guid, LPSTR str)
{
    return sprintf(str, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

static __inline int SHELL32_GUIDToStringW (REFGUID guid, LPWSTR str)
{
    static const WCHAR fmtW[] =
     { '{','%','0','8','l','X','-','%','0','4','X','-','%','0','4','X','-',
     '%','0','2','X','%','0','2','X','-',
     '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
     '%','0','2','X','%','0','2','X','}',0 };
    return swprintf(str, fmtW,
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

void SHELL_FS_ProcessDisplayFilename(LPWSTR szPath, DWORD dwFlags);
BOOL SHELL_FS_HideExtension(LPCWSTR pwszPath);

void CloseRegKeyArray(HKEY* array, UINT cKeys);
LSTATUS AddClassKeyToArray(const WCHAR* szClass, HKEY* array, UINT* cKeys);
LSTATUS AddClsidKeyToArray(REFCLSID clsid, HKEY* array, UINT* cKeys);
void AddFSClassKeysToArray(UINT cidl, PCUITEMID_CHILD_ARRAY apidl, HKEY* array, UINT* cKeys);

#ifdef __cplusplus

struct CRegKeyHandleArray
{
    HKEY hKeys[16];
    UINT cKeys;

    CRegKeyHandleArray() : cKeys(0) {}
    ~CRegKeyHandleArray() { CloseRegKeyArray(hKeys, cKeys); }
    operator HKEY*() { return hKeys; }
    operator UINT*() { return &cKeys; }
    operator UINT() { return cKeys; }
    HKEY& operator [](SIZE_T i) { return hKeys[i]; }
};

HRESULT inline SHSetStrRet(LPSTRRET pStrRet, DWORD resId)
{
    return SHSetStrRet(pStrRet, shell32_hInstance, resId);
}

#endif

#endif /* _SHFLDR_H_ */
