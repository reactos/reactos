/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CCleanupHandler implementation
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "cleanmgr.h"


CCleanupHandler::CCleanupHandler(CRegKey &subKey, const CStringW &keyName, const GUID &guid)
    : hSubKey(subKey)
    , KeyName(keyName)
    , Guid(guid)
    , dwFlags(0)
    , Priority(0)
    , StateFlags(0)
    , SpaceUsed(0)
    , ShowHandler(true)
    , hIcon(NULL)
{
}

CCleanupHandler::~CCleanupHandler()
{
    Deactivate();
    ::DestroyIcon(hIcon);
}

void
CCleanupHandler::Deactivate()
{
    if (Handler)
    {
        DWORD dwFlags = 0;
        Handler->Deactivate(&dwFlags);
        if (dwFlags & EVCF_REMOVEFROMLIST)
            UNIMPLEMENTED_DBGBREAK();
    }
}

bool
CCleanupHandler::Initialize(LPCWSTR pcwszVolume)
{
    if (FAILED_UNEXPECTEDLY(
            ::CoCreateInstance(Guid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IEmptyVolumeCache, &Handler))))
    {
        return false;
    }

    DWORD dwSize = sizeof(Priority);
    if (hSubKey.QueryBinaryValue(L"Priority", &Priority, &dwSize) != ERROR_SUCCESS)
    {
        if (hSubKey.QueryDWORDValue(L"Priority", Priority) != ERROR_SUCCESS)
            Priority = 200;
    }

    dwSize = sizeof(StateFlags);
    if (hSubKey.QueryDWORDValue(L"StateFlags", StateFlags) != ERROR_SUCCESS)
        StateFlags = 0;

    WCHAR PathBuffer[MAX_PATH] = {};
    ULONG nChars = _countof(PathBuffer);
    if (hSubKey.QueryStringValue(L"IconPath", PathBuffer, &nChars) != ERROR_SUCCESS)
    {
        CStringW Tmp;
        WCHAR GuidStr[50] = {};
        if (StringFromGUID2(Guid, GuidStr, _countof(GuidStr)))
        {
            Tmp.Format(L"CLSID\\%s\\DefaultIcon", GuidStr);
            CRegKey clsid;
            nChars = _countof(PathBuffer);
            if (clsid.Open(HKEY_CLASSES_ROOT, Tmp, KEY_READ) != ERROR_SUCCESS ||
                clsid.QueryStringValue(NULL, PathBuffer, &nChars) != ERROR_SUCCESS)
            {
                PathBuffer[0] = UNICODE_NULL;
            }
        }
    }
    if (!PathBuffer[0])
        StringCchCopyW(PathBuffer, _countof(PathBuffer), L"%systemroot%\\system32\\shell32.dll");

    int Index = 0;
    WCHAR *ptr = wcschr(PathBuffer, L',');
    if (ptr)
    {
        *ptr++ = UNICODE_NULL;
        Index = wcstol(ptr, NULL, 10);
    }
    HICON Large, Small;
    UINT Result = ExtractIconExW(PathBuffer, Index, &Large, &Small, 1);
    if (Result < 1)
        Result = ExtractIconExW(L"%systemroot%\\system32\\shell32.dll", 0, &Large, &Small, 1);
    if (Result >= 1)
    {
        hIcon = Small;
        if (!hIcon)
        {
            hIcon = Large;
        }
        else
        {
            ::DestroyIcon(Large);
        }
    }

    // These options should come from the command line
    // dwFlags |= EVCF_SETTINGSMODE;
    // dwFlags |= EVCF_OUTOFDISKSPACE;

    CComPtr<IEmptyVolumeCache2> spHandler2;
    HRESULT hr = Handler->QueryInterface(IID_PPV_ARG(IEmptyVolumeCache2, &spHandler2));
    if (SUCCEEDED(hr))
    {
        hr = spHandler2->InitializeEx(
            hSubKey, pcwszVolume, KeyName, &wszDisplayName, &wszDescription, &wszBtnText, &dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
            return false;

        // No files to clean will return S_FALSE;
        if (hr != S_OK)
            return false;
    }
    else
    {
        // Observed behavior:
        // When Initialize is called, wszDescription is actually pointing to data
        // wszDescription.AllocateBytes(0x400u);
        hr = Handler->Initialize(hSubKey, pcwszVolume, &wszDisplayName, &wszDescription, &dwFlags);
        if (FAILED_UNEXPECTEDLY(hr))
            return false;

        // No files to clean will return S_FALSE;
        if (hr != S_OK)
            return false;

        CComPtr<IPropertyBag> spBag;
        WCHAR GuidStr[50] = {};
        nChars = _countof(GuidStr);
        if (hSubKey.QueryStringValue(L"PropertyBag", GuidStr, &nChars) == ERROR_SUCCESS)
        {
            GUID guid = {};
            if (!FAILED_UNEXPECTEDLY(CLSIDFromString(GuidStr, &guid)))
            {
                FAILED_UNEXPECTEDLY(
                    CoCreateInstance(guid, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPropertyBag, &spBag)));
            }
        }
        ReadProperty(L"Display", spBag, wszDisplayName);
        ReadProperty(L"Description", spBag, wszDescription);

        if (dwFlags & EVCF_HASSETTINGS)
        {
            ReadProperty(L"AdvancedButtonText", spBag, wszBtnText);
        }
    }

    if ((dwFlags & EVCF_ENABLEBYDEFAULT) && !(StateFlags & HANDLER_STATE_SELECTED))
    {
        StateFlags |= HANDLER_STATE_SELECTED;
    }

    // For convenience
    if (!wszDisplayName)
        SHStrDupW(KeyName, &wszDisplayName);

    return true;
}

void
CCleanupHandler::ReadProperty(LPCWSTR Name, IPropertyBag *pBag, CComHeapPtr<WCHAR> &storage)
{
    if (storage)
        return;

    if (pBag)
    {
        CComVariant tmp;
        tmp.vt = VT_BSTR;
        HRESULT hr = pBag->Read(Name, &tmp, NULL);
        if (!FAILED_UNEXPECTEDLY(hr) && tmp.vt == VT_BSTR)
        {
            SHStrDupW(tmp.bstrVal, &storage);
        }
    }

    if (!storage)
    {
        WCHAR TmpStr[0x200] = {};
        DWORD dwSize = _countof(TmpStr);

        if (hSubKey.QueryStringValue(Name, TmpStr, &dwSize) == ERROR_SUCCESS)
        {
            WCHAR ResolvedStr[0x200] = {};
            SHLoadIndirectString(TmpStr, ResolvedStr, _countof(ResolvedStr), NULL);
            SHStrDupW(ResolvedStr, &storage);
        }
    }
}

BOOL
CCleanupHandler::HasSettings() const
{
    return !!(dwFlags & EVCF_HASSETTINGS);
}

BOOL
CCleanupHandler::DontShowIfZero() const
{
    return !!(dwFlags & EVCF_DONTSHOWIFZERO);
}

