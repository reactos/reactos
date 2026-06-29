/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CCleanupHandler definition
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#define HANDLER_STATE_SELECTED 1


struct CCleanupHandler
{
    CCleanupHandler(CRegKey &subKey, const CStringW &keyName, const GUID &guid);
    ~CCleanupHandler();

    void Deactivate();

    bool
    Initialize(LPCWSTR pcwszVolume);

    void
    ReadProperty(LPCWSTR Name, IPropertyBag *pBag, CComHeapPtr<WCHAR> &storage);

    BOOL
    HasSettings() const;

    BOOL
    DontShowIfZero() const;

    CRegKey hSubKey;
    CStringW KeyName;
    GUID Guid;

    CComHeapPtr<WCHAR> wszDisplayName;
    CComHeapPtr<WCHAR> wszDescription;
    CComHeapPtr<WCHAR> wszBtnText;

    CStringW IconPath;
    DWORD dwFlags;
    DWORD Priority;
    DWORD StateFlags;

    CComPtr<IEmptyVolumeCache> Handler;
    DWORDLONG SpaceUsed;
    bool ShowHandler;
    HICON hIcon;
};

