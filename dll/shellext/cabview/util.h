/*
 * PROJECT:     ReactOS CabView Shell Extension
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Utility header file
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

template<class H> static int ErrorBox(H hWnd, int Error)
{
    SHELL_ErrorBox(hWnd, Error);
    return Error;
}

template<class T> static inline bool IsPathSep(T c)
{
    return c == '\\' || c == '/';
}

inline bool IsEqual(const SHCOLUMNID &scid, REFGUID guid, UINT pid)
{
    return scid.pid == pid && IsEqualGUID(scid.fmtid, guid);
}

inline HRESULT InitVariantFromBuffer(const void *buffer, UINT cb, VARIANT *pv)
{
    SAFEARRAY *pa = SafeArrayCreateVector(VT_UI1, 0, cb);
    if (pa)
    {
        CopyMemory(pa->pvData, buffer, cb);
        V_VT(pv) = VT_UI1 | VT_ARRAY;
        V_UNION(pv, parray) = pa;
        return S_OK;
    }
    V_VT(pv) = VT_EMPTY;
    return E_OUTOFMEMORY;
}

inline void FreeStrRet(STRRET &str)
{
    if (str.uType == STRRET_WSTR)
    {
        SHFree(str.pOleStr);
        str.uType = STRRET_CSTR;
    }
}

inline HRESULT StrTo(LPCWSTR str, UINT len, STRRET &sr)
{
    LPWSTR data = (LPWSTR)SHAlloc(++len * sizeof(WCHAR));
    if (!data)
        return E_OUTOFMEMORY;
    lstrcpynW(data, str, len);
    sr.uType = STRRET_WSTR;
    sr.pOleStr = data;
    return S_OK;
}

inline HRESULT StrTo(LPCWSTR str, STRRET &sr)
{
    return StrTo(str, lstrlenW(str), sr);
}

inline HRESULT StrTo(LPCWSTR str, UINT len, VARIANT &v)
{
    BSTR data = SysAllocStringLen(str, len);
    if (!data)
        return E_OUTOFMEMORY;
    V_VT(&v) = VT_BSTR;
    V_BSTR(&v) = data;
    return S_OK;
}

inline HRESULT StrTo(LPCWSTR str, VARIANT &v)
{
    return StrTo(str, lstrlenW(str), v);
}

inline HRESULT StrRetToVariantBSTR(STRRET *psr, VARIANT &v)
{
    HRESULT hr = StrRetToBSTR(psr, NULL, &V_BSTR(&v));
    if (SUCCEEDED(hr))
        V_VT(&v) = VT_BSTR;
    return hr;
}

inline HRESULT GetDetailsOf(IShellFolder2 &Folder, PCUITEMID_CHILD pidl, UINT Column, PWSTR &String)
{
    SHELLDETAILS details;
    HRESULT hr = Folder.GetDetailsOf(pidl, Column, &details);
    if (SUCCEEDED(hr))
        hr = StrRetToStrW(&details.str, pidl, &String);
    return hr;
}

inline HRESULT InsertMenuItem(QCMINFO &qcmi, UINT &Pos, UINT &TrackId, UINT Id, UINT ResId, int State = 0)
{
    UINT flags = 0;
    WCHAR string[MAX_PATH];
    string[0] = UNICODE_NULL;
    if ((Id += qcmi.idCmdFirst) > qcmi.idCmdLast)
        return E_FAIL;
    else if (ResId == (UINT)-1)
        flags |= MF_SEPARATOR;
    else if (!LoadStringW(_AtlBaseModule.GetResourceInstance(), ResId, string, _countof(string)))
        return E_FAIL;

    MENUITEMINFOW mii;
    mii.cbSize = FIELD_OFFSET(MENUITEMINFOW, hbmpItem); // USER32 version agnostic
    mii.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE;
    mii.fType = flags;
    mii.wID = Id;
    mii.dwTypeData = string;
    mii.cch = 0;
    mii.fState = State;
    if (!InsertMenuItemW(qcmi.hmenu, Pos, TRUE, &mii))
        return E_FAIL;
    Pos++;
    TrackId = max(TrackId, Id);
    return S_OK;
}

inline SFGAOF MapFSToSFAttributes(UINT att)
{
    return ((att & FILE_ATTRIBUTE_READONLY) ? SFGAO_READONLY : 0) |
           ((att & FILE_ATTRIBUTE_HIDDEN) ? SFGAO_HIDDEN : 0) |
           ((att & FILE_ATTRIBUTE_SYSTEM) ? SFGAO_SYSTEM : 0);
}

inline bool IncludeInEnumIDList(SHCONTF contf, SFGAOF att)
{
    const SHCONTF both = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;
    const SFGAOF superbits = SFGAO_HIDDEN | SFGAO_READONLY | SFGAO_SYSTEM;
    const bool isfile = (att & (SFGAO_STREAM | SFGAO_FOLDER)) != SFGAO_FOLDER;
    if ((contf & both) != both && !(contf & SHCONTF_STORAGE))
    {
        if (isfile && (contf & SHCONTF_FOLDERS))
            return false;
        if ((att & SFGAO_FOLDER) && (contf & SHCONTF_NONFOLDERS))
            return false;
    }
    if ((att & SFGAO_HIDDEN) && !(contf & (SHCONTF_INCLUDEHIDDEN | SHCONTF_STORAGE)))
        return false;
    if ((att & superbits) > SFGAO_HIDDEN && !(contf & (SHCONTF_INCLUDESUPERHIDDEN | SHCONTF_STORAGE)))
        return false;
    return true;
}

inline int MapPIDLToSystemImageListIndex(IShellFolder *pSF, PCUITEMID_CHILD pidl, UINT GilFlags = 0)
{
    int normal, open;
    BOOL qopen = GilFlags & GIL_OPENICON;
    normal = SHMapPIDLToSystemImageListIndex(pSF, pidl, qopen ? &open : NULL);
    return qopen && open != -1 ? open : normal;
}
