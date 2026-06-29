/*
 * PROJECT:     browseui
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Common registry based settings editor
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#include "precomp.h"

#define HKEY_GRAYED HKEY_REGTREEOPTION_GRAYED
#define MAXVALUEDATA 100 // Max size for CheckedValue/UncheckedValue/DefaultValue

enum {
    TYPE_CHECK = 0,
    TYPE_RADIO,
    TYPE_GROUP,
    TYPE_INVALID,
    STATEIMAGESPERTYPE = 4,
    STATE_CHECKOFF = TYPE_CHECK * STATEIMAGESPERTYPE,
    STATE_CHECKON,
    STATE_RADIOOFF = TYPE_RADIO * STATEIMAGESPERTYPE,
    STATE_RADIOON,
    STATE_GROUP    = TYPE_GROUP * STATEIMAGESPERTYPE,
    REG_GET_DIRECT = -1,
};

C_ASSERT((STATE_CHECKON & 1) && (STATE_RADIOON & 1) && STATE_RADIOON < STATE_GROUP);

static HBITMAP CreatDibBitmap(HDC hDC, UINT cx, UINT cy, UINT bpp)
{
    BITMAPINFO bi;
    PVOID pvBits;

    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = bpp;
    bi.bmiHeader.biCompression = BI_RGB;
    return CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
}

static HIMAGELIST CreateStateImages(UINT ImageSize)
{
    enum { bpp = 32 };
    UINT Failed = FALSE, State;
    HIMAGELIST hIL = ImageList_Create(ImageSize, ImageSize, ILC_MASK | bpp, 8, 0);
    if (!hIL)
        return hIL;

    INT BorderSize = max(1, min(ImageSize / 16, 4));
    RECT Rect;
    HDC hDC = CreateCompatibleDC(NULL);

    HBITMAP hbmMask = CreateBitmap(ImageSize, ImageSize, 1, 1, NULL);
    Failed |= hbmMask == NULL;

    HBITMAP hbmData = CreatDibBitmap(hDC, ImageSize, ImageSize, bpp);
    Failed |= hbmData == NULL;

    const HGDIOBJ hbmOld = SelectObject(hDC, hbmMask);
    Failed |= hbmOld == NULL;

    // Create the check images
    SetRect(&Rect, 0, 0, ImageSize, ImageSize);
    FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
    InflateRect(&Rect, -BorderSize, -BorderSize);
    FillRect(hDC, &Rect, HBRUSH(GetStockObject(BLACK_BRUSH)));

    SelectObject(hDC, hbmData);
    State = DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_MONO;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_CHECKED);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_INACTIVE);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_CHECKED | DFCS_INACTIVE);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;

    // Create the radio images
    SelectObject(hDC, hbmMask);
    SetRect(&Rect, 0, 0, ImageSize, ImageSize);
    FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
    InflateRect(&Rect, -BorderSize, -BorderSize);
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, DFCS_BUTTONRADIOMASK | DFCS_FLAT | DFCS_MONO);

    SelectObject(hDC, hbmData);
    State = DFCS_BUTTONRADIOIMAGE | DFCS_FLAT | DFCS_MONO;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_CHECKED);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_INACTIVE);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;
    DrawFrameControl(hDC, &Rect, DFC_BUTTON, State | DFCS_CHECKED | DFCS_INACTIVE);
    Failed |= ImageList_Add(hIL, hbmData, hbmMask) < 0;

    SelectObject(hDC, hbmOld);
    DeleteObject(hbmMask);
    DeleteObject(hbmData);
    DeleteDC(hDC);
    if (Failed)
    {
        ImageList_Destroy(hIL);
        hIL = NULL;
    }
    return hIL;
}

static inline DWORD NormalizeRegType(DWORD Type)
{
    switch (Type)
    {
        case REG_EXPAND_SZ: return REG_SZ;
        case REG_DWORD: return REG_BINARY;
        default: return Type;
    }
}

static inline DWORD GetRegDWORD(HKEY hKey, LPCWSTR Name, PDWORD Data)
{
    DWORD Size = sizeof(*Data);
    return SHRegGetValueW(hKey, NULL, Name, SRRF_RT_DWORD, NULL, Data, &Size);
}

static DWORD GetRegDWORD(HKEY hKey, LPCWSTR Name, DWORD Default)
{
    DWORD Data = Default, Size = sizeof(Data);
    DWORD Err = SHRegGetValueW(hKey, NULL, Name, SRRF_RT_DWORD, NULL, &Data, &Size);
    return Err ? Default : Data;
}

static HRESULT GetRegString(HKEY hKey, LPCWSTR Name, LPWSTR Buffer, DWORD cchBuffer)
{
    DWORD Size = cchBuffer * sizeof(*Buffer);
    DWORD Err = SHRegGetValueW(hKey, NULL, Name, SRRF_RT_REG_SZ, NULL, Buffer, &Size);
    return Err ? HRESULT_FROM_WIN32(Err) : Size / sizeof(*Buffer);
}

static int AddIconFromRegistry(HIMAGELIST hIL, HKEY hKey, LPCWSTR Name)
{
    WCHAR Path[MAX_PATH + 42];
    if (GetRegString(hKey, Name, Path, _countof(Path)) > 0) // Will %expand% for us
    {
        int idx = PathParseIconLocationW(Path);
        HICON hIcon = NULL;
        if (ExtractIconExW(Path, idx * -1, NULL, &hIcon, 1)) // Note: Index/Id is opposite of DefaultIcon style
        {
            idx = ImageList_AddIcon(hIL, hIcon);
            DestroyIcon(hIcon);
            if (idx != -1)
                return idx;
        }
    }
    return I_IMAGENONE;
}

static HRESULT GetDefaultValue(HKEY hKey, DWORD &Type, LPVOID Data, DWORD &Size)
{
    DWORD Err = SHRegGetValueW(hKey, NULL, L"DefaultValue", SRRF_RT_ANY, &Type, Data, &Size);
    return HRESULT_FROM_WIN32(Err);
}

HRESULT CRegTreeOptions::GetSetState(HKEY hKey, DWORD &Type, LPBYTE Data, DWORD &Size, BOOL Set)
{
    HRESULT hr;
    UINT SysParam = GetRegDWORD(hKey, Set ? L"SPIActionSet" : L"SPIActionGet", DWORD(0));
    if (SysParam)
    {
        Size = sizeof(BOOL);
        Type = REG_DWORD;
        SHBoolSystemParametersInfo(SysParam, Data);
        return S_OK;
    }

    // learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-iregtreeitem
    WCHAR szTemp[42];
    CLSID clsid;
    if (SUCCEEDED(GetRegString(hKey, L"CLSID", szTemp, _countof(szTemp))) &&
        SUCCEEDED(CLSIDFromString(szTemp, &clsid)))
    {
        IRegTreeItem *pRTI;
        if (SUCCEEDED(hr = IUnknown_QueryService(m_pUnkSite, clsid, IID_PPV_ARG(IRegTreeItem, &pRTI))) ||
            SUCCEEDED(hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IRegTreeItem, &pRTI))))
        {
            BOOL *boolptr = (BOOL*)Data;
            Size = sizeof(BOOL);
            Type = REG_GET_DIRECT;
            hr = Set ? pRTI->SetCheckState(*boolptr) : pRTI->GetCheckState(boolptr);
            pRTI->Release();
        }
        return hr;
    }

    DWORD Mask = ~DWORD(0), Offset = 0;
    BOOL HasMask = GetRegDWORD(hKey, L"Mask", &Mask) == ERROR_SUCCESS;
    BOOL HasOffset = GetRegDWORD(hKey, L"Offset", &Offset) == ERROR_SUCCESS;

    HKEY hValueKey = (HKEY)LongToHandle(GetRegDWORD(hKey, L"HKeyRoot", HandleToLong(HKEY_CURRENT_USER)));
    WCHAR SubKey[MAX_PATH], Name[100];
    hr = GetRegString(hKey, L"RegPath", SubKey, _countof(SubKey));
    if (FAILED(hr))
        return hr;
    hr = GetRegString(hKey, L"ValueName", Name, _countof(Name));
    if (FAILED(hr))
        return hr;
    DWORD Err, cb;
    if (Set)
        Err = RegCreateKeyExW(hValueKey, SubKey, 0, NULL, 0, KEY_READ | KEY_SET_VALUE, NULL, &hValueKey, NULL);
    else
        Err = RegOpenKeyExW(hValueKey, SubKey, 0, KEY_READ, &hValueKey);
    if (Err)
        return HRESULT_FROM_WIN32(Err);

    if (Set && (HasMask || HasOffset))
    {
        Err = SHRegGetValueW(hValueKey, NULL, Name, SRRF_RT_ANY, NULL, NULL, &cb);
        hr = HRESULT_FROM_WIN32(Err);
        if (Err == ERROR_FILE_NOT_FOUND)
        {
            DWORD DefaultData = 0;
            cb = sizeof(DefaultData);
            hr = GetDefaultValue(hKey, Type, &DefaultData, cb);
            if (SUCCEEDED(hr))
            {
                Err = SHSetValueW(hValueKey, NULL, Name, Type, &DefaultData, cb);
                hr = HRESULT_FROM_WIN32(Err);
            }
        }

        if (SUCCEEDED(hr))
        {
            Err = ERROR_OUTOFMEMORY;
            cb = max(cb, (Offset + 1) * sizeof(DWORD));
            if (PDWORD pBigData = (PDWORD)LocalAlloc(LPTR, cb)) // LPTR to zero extended data
            {
                Err = SHRegGetValueW(hValueKey, NULL, Name, SRRF_RT_ANY, &Type, pBigData, &cb);
                Size = cb;
                if (Err == ERROR_SUCCESS)
                {
                    pBigData[Offset] &= ~Mask;
                    pBigData[Offset] |= *((PDWORD)Data);
                    Err = SHSetValueW(hValueKey, NULL, Name, Type, pBigData, cb);
                }
                LocalFree(pBigData);
            }
            hr = HRESULT_FROM_WIN32(Err);
        }
    }
    else if (Set)
    {
        Err = SHSetValueW(hValueKey, NULL, Name, Type, Data, Size);
        hr = HRESULT_FROM_WIN32(Err);
    }
    else
    {
        if (Offset && HasOffset)
        {
            Size = sizeof(DWORD);
            Err = SHRegGetValueW(hValueKey, NULL, Name, SRRF_RT_ANY, NULL, NULL, &cb);
            if (Err == ERROR_SUCCESS)
            {
                Err = ERROR_OUTOFMEMORY;
                if (PDWORD pBigData = (PDWORD)LocalAlloc(LPTR, cb))
                {
                    Err = SHRegGetValueW(hValueKey, NULL, Name, SRRF_RT_ANY, &Type, pBigData, &cb);
                    if (Offset < cb / sizeof(DWORD) && Err == ERROR_SUCCESS)
                        *((PDWORD)Data) = pBigData[Offset];
                    else
                        *((PDWORD)Data) = 0; // Value not present or offset too large
                    LocalFree(pBigData);
                }
            }
        }
        else
        {
            Err = SHRegGetValueW(hValueKey, NULL, Name, SRRF_RT_ANY, &Type, Data, &Size);
        }

        hr = HRESULT_FROM_WIN32(Err);
        if (HasMask && SUCCEEDED(hr))
            *((PDWORD)Data) &= Mask;
    }
    RegCloseKey(hValueKey);
    return hr;
}

HRESULT CRegTreeOptions::GetCheckState(HKEY hKey, BOOL UseDefault)
{
    BYTE CurrData[MAXVALUEDATA], CheckData[MAXVALUEDATA];
    DWORD CurrType, CheckType, CurrSize, CheckSize, Err, Checked;

    if (hKey == HKEY_GRAYED)
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);

    CheckSize = sizeof(CheckData);
    Err = SHRegGetValueW(hKey, NULL, L"CheckedValue", SRRF_RT_ANY, &CheckType, CheckData, &CheckSize);
    if (Err)
        return HRESULT_FROM_WIN32(Err);

    HRESULT hr = E_FAIL;
    if (!UseDefault)
    {
        CurrSize = sizeof(CurrData);
        hr = GetSetState(hKey, CurrType, CurrData, CurrSize, FALSE);
    }
    if (FAILED(hr))
    {
        CurrSize = sizeof(CurrData);
        hr = GetDefaultValue(hKey, CurrType, CurrData, CurrSize);
    }
    if (FAILED(hr))
    {
        return hr;
    }

    CurrType = NormalizeRegType(CurrType);
    CheckType = NormalizeRegType(CheckType);
    if (CurrType == (UINT)REG_GET_DIRECT)
    {
        Checked = *(BOOL*)CurrData;
    }
    else if (CurrType != CheckType)
    {
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    else if (CheckType == REG_SZ)
    {
        *(LPWSTR)(&CheckData[sizeof(CheckData) - sizeof(WCHAR)]) = UNICODE_NULL;
        if (CheckSize <= sizeof(CheckData) - sizeof(WCHAR))
            *(LPWSTR)(&CheckData[CheckSize]) = UNICODE_NULL;
        Checked = !lstrcmpiW((LPWSTR)CheckData, (LPWSTR)CurrData);
    }
    else if (CheckType == REG_BINARY)
    {
        Checked = CheckSize == CurrSize && !memcmp(CheckData, CurrData, CurrSize);
    }
    else
    {
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
    }
    return Checked ? S_OK : S_FALSE;
}

HRESULT CRegTreeOptions::SaveCheckState(HKEY hKey, BOOL Checked)
{
    BYTE Data[MAXVALUEDATA];
    DWORD Type = REG_DWORD, Size = sizeof(Data);
    *((PDWORD)Data) = Checked;
    LPCWSTR Name = Checked ? L"CheckedValue" : L"UncheckedValue";
    DWORD Err = SHRegGetValueW(hKey, NULL, Name, SRRF_RT_ANY, &Type, Data, &Size);
    if (Err == ERROR_FILE_NOT_FOUND)
        Err = ERROR_SUCCESS;
    return Err ? HRESULT_FROM_WIN32(Err) : GetSetState(hKey, Type, Data, Size, TRUE);
}

void CRegTreeOptions::WalkTree(WALK_TREE_CMD Command, HWND hTree, HTREEITEM hTI)
{
    for (HTREEITEM hChildTI = TreeView_GetChild(hTree, hTI); hChildTI;)
    {
        WalkTree(Command, hTree, hChildTI);
        hChildTI = TreeView_GetNextSibling(hTree, hChildTI);
    }

    TVITEM tvi;
    tvi.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem = hTI;
    if (!TreeView_GetItem(hTree, &tvi))
        return;

    HKEY hKey = (HKEY)tvi.lParam;
    UINT State = tvi.iImage;
    switch (Command)
    {
        case WALK_TREE_SAVE:
            if (State == STATE_CHECKON || State == STATE_RADIOON)
                SaveCheckState(hKey, TRUE);
            else if (State == STATE_CHECKOFF)
                SaveCheckState(hKey, FALSE);
            break;

        case WALK_TREE_DESTROY:
            if (hKey && hKey != HKEY_GRAYED)
            {
                RegCloseKey(hKey);
                tvi.lParam = 0;
                tvi.mask = TVIF_PARAM;
                TreeView_SetItem(hTree, &tvi);
            }
            break;

        case WALK_TREE_DEFAULT:
        case WALK_TREE_REFRESH:
            if (State < STATE_GROUP)
            {
                HRESULT hr = GetCheckState(hKey, Command == WALK_TREE_DEFAULT);
                if (SUCCEEDED(hr))
                {
                    tvi.iImage = tvi.iSelectedImage = (tvi.iImage & ~1) | (hr == S_OK);
                    TreeView_SetItem(hTree, &tvi);
                }
            }
            break;
    }
}

CRegTreeOptions::CRegTreeOptions()
{
    m_hTree = NULL;
    m_hIL = NULL;
}

CRegTreeOptions::~CRegTreeOptions()
{
    if (m_hIL && m_hTree)
        ImageList_Destroy(TreeView_SetImageList(m_hTree, NULL, TVSIL_NORMAL));
}

void CRegTreeOptions::AddItemsFromRegistry(HKEY hKey, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    for (DWORD Index = 0, cchName;;)
    {
        WCHAR Name[MAX_PATH], Temp[MAX_PATH];
        cchName = _countof(Name);
        if (RegEnumKeyExW(hKey, Index++, Name, &cchName, NULL, NULL, NULL, NULL) != ERROR_SUCCESS)
            break;

        HKEY hItemKey, hCloseKey = NULL;
        if (RegOpenKeyExW(hKey, Name, 0, KEY_READ, &hItemKey) != ERROR_SUCCESS)
            continue;

        TVINSERTSTRUCT tvis;
        TVITEM &tvi = tvis.item;
        tvi.mask = TVIF_PARAM | TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        tvi.lParam = (LPARAM)hItemKey;
        UINT Type = TYPE_INVALID;
        if (GetRegString(hItemKey, L"Type", Temp, _countof(Temp)) > 0)
        {
            if (!lstrcmpi(Temp, L"checkbox"))
                Type = TYPE_CHECK;
            if (!lstrcmpi(Temp, L"radio"))
                Type = TYPE_RADIO;
            if (!lstrcmpi(Temp, L"group"))
                Type = TYPE_GROUP;
        }
        BOOL Valid = Type != TYPE_INVALID;

        if (Type == TYPE_GROUP)
        {
            tvi.lParam = 0;
            hCloseKey = hItemKey;
            tvi.iImage = AddIconFromRegistry(m_hIL, hItemKey, L"Bitmap");
        }
        else if (Valid)
        {
            HRESULT hr = GetCheckState(hItemKey);
            Valid = SUCCEEDED(hr);
            tvi.iImage = (Type * STATEIMAGESPERTYPE) | (hr == S_OK);
        }

        if (Valid)
        {
            if (SUCCEEDED(SHLoadRegUIStringW(hItemKey, L"Text", Temp, _countof(Temp))))
                tvi.pszText = Temp;
            else
                tvi.pszText = Name;
#ifdef __REACTOS__ // Grayed is a ROS extension
            DWORD grayed = GetRegDWORD(hItemKey, L"Grayed", DWORD(FALSE));
            if (grayed)
            {
                tvi.lParam = (LPARAM)HKEY_GRAYED;
                hCloseKey = hItemKey;
                tvi.iImage += 2; // Use disabled state image
                if (grayed & 4)
                    tvi.iImage = (tvi.iImage & ~1) | ((grayed & 2) != 0); // Force a specific state
            }
#endif
            tvi.iSelectedImage = tvi.iImage;
            tvis.hParent = hParent;
            tvis.hInsertAfter = hInsertAfter;
            Valid = (tvi.hItem = TreeView_InsertItem(m_hTree, &tvis)) != NULL;
        }

        if (Valid && Type == TYPE_GROUP)
        {
            AddItemsFromRegistry(hItemKey, tvi.hItem, TVI_FIRST);
            TreeView_Expand(m_hTree, tvi.hItem, TVE_EXPAND);
        }
        if (!Valid)
            RegCloseKey(hItemKey);
        if (hCloseKey && hItemKey != hCloseKey)
            RegCloseKey(hCloseKey);
    }
    SendMessage(m_hTree, TVM_SORTCHILDREN, 0, (LPARAM)hParent);
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::InitTree(HWND hTV, HKEY hKey, LPCSTR SubKey, char const *pUnknown)
{
    UNREFERENCED_PARAMETER(pUnknown);
    m_hTree = hTV;

    m_hIL = CreateStateImages(GetSystemMetrics(SM_CXSMICON));
    HIMAGELIST hIL = TreeView_SetImageList(hTV, m_hIL, TVSIL_NORMAL);
    if (hIL)
        ImageList_Destroy(hIL);

    WCHAR Path[MAX_PATH];
    SHAnsiToUnicode(SubKey, Path, _countof(Path));
    if (RegOpenKeyExW(hKey, Path, 0, KEY_ENUMERATE_SUB_KEYS, &hKey) == ERROR_SUCCESS)
    {
        AddItemsFromRegistry(hKey, TVI_ROOT, TVI_ROOT);
        RegCloseKey(hKey);
    }

    TreeView_EnsureVisible(hTV, TreeView_GetRoot(hTV));
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::WalkTree(WALK_TREE_CMD Command)
{
    for (HTREEITEM hTI = TreeView_GetRoot(m_hTree); hTI;)
    {
        WalkTree(Command, m_hTree, hTI);
        hTI = TreeView_GetNextSibling(m_hTree, hTI);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::ToggleItem(HTREEITEM hTI)
{
    TVITEM tvi;
    tvi.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem = hTI;
    if (!tvi.hItem || !TreeView_GetItem(m_hTree, &tvi))
        return E_INVALIDARG;

    if (tvi.iImage >= STATE_GROUP || (HKEY)tvi.lParam == HKEY_GRAYED)
        return S_FALSE;

    if (tvi.iImage <= max(STATE_CHECKOFF, STATE_CHECKON))
    {
        tvi.iImage ^= 1;
    }
    else
    {
        HTREEITEM hRadio = TreeView_GetChild(m_hTree, TreeView_GetParent(m_hTree, hTI));
        while (hRadio) // Turn off everyone in this group
        {
            tvi.hItem = hRadio;
            tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            if (TreeView_GetItem(m_hTree, &tvi) && tvi.iImage == STATE_RADIOON && hRadio != hTI)
            {
                tvi.iImage ^= 1;
                tvi.iSelectedImage = tvi.iImage;
                TreeView_SetItem(m_hTree, &tvi);
            }
            hRadio = TreeView_GetNextSibling(m_hTree, hRadio);
        }
        tvi.iImage = STATE_RADIOON;
    }

    tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvi.hItem = hTI;
    tvi.iSelectedImage = tvi.iImage;
    TreeView_SetItem(m_hTree, &tvi);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CRegTreeOptions::ShowHelp(HTREEITEM hTI, unsigned long Unknown)
{
    return E_NOTIMPL;
}
