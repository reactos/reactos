/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Snapin cache entry class
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

#pragma once

class CSnapinCacheEntry
{
private:
    CAtlString m_GuidString;
    CAtlString m_AboutGuidString;
    CAtlString m_Name;

    CAtlString m_Provider;
    CAtlString m_Description;
    CAtlString m_Version;

    GUID m_Guid;
    GUID m_AboutGuid;

    int m_ImageNormal;
    int m_ImageOpen;

    BOOL m_Loaded;

public:
    CSnapinCacheEntry(const CAtlString& guidString, const CAtlString& aboutGuidString, const CAtlString& name, HIMAGELIST hImageList);
    ~CSnapinCacheEntry();

    const CAtlString& GuidString() const { return m_GuidString; }
    const CAtlString& AboutGuidString() const { return m_AboutGuidString; }
    const CAtlString& Name() const { return m_Name; }
    const CAtlString& Provider() const { return m_Provider; }
    const CAtlString& Description() const { return m_Description; }
    const CAtlString& Version() const { return m_Version; }

    int NormalImageIndex() { return m_ImageNormal; }
    int OpenImageIndex() { return m_ImageOpen; }

    static CSnapinCacheEntry* Create(CRegKey& SnapinsKey, PWSTR guidString, HIMAGELIST hImageList)
    {
        DPRINT1("Create(%S)\n", guidString);
        WCHAR IndirectBuffer[MAX_PATH];
        WCHAR Buffer[MAX_PATH];
        ULONG dwSize;
        LONG ret;

        CRegKey SnapinKey;
        ret = SnapinKey.Open(SnapinsKey, guidString, KEY_READ);
        if (ret != ERROR_SUCCESS)
        {
            return NULL;
        }

        CRegKey NodeTypesKey;
        ret = NodeTypesKey.Open(SnapinKey, L"StandAlone", KEY_READ);
        if (ret != ERROR_SUCCESS)
        {
            SnapinKey.Close();
            return NULL;
        }
        NodeTypesKey.Close();

        CAtlString Name;
        dwSize = MAX_PATH * sizeof(WCHAR);
        if (SnapinKey.QueryStringValue(L"NameStringIndirect", IndirectBuffer, &dwSize) == ERROR_SUCCESS)
        {
            if (SUCCEEDED(SHLoadIndirectString(IndirectBuffer, Buffer, MAX_PATH, NULL)))
            {
                Name = Buffer;
            }
            else
            {
                Name.Empty();
            }
        }

        if (Name.IsEmpty())
        {
            dwSize = MAX_PATH * sizeof(WCHAR);
            if (SnapinKey.QueryStringValue(L"NameString", Buffer, &dwSize) == ERROR_SUCCESS)
            {
                Name = Buffer;
            }
        }

        if (Name.IsEmpty())
        {
            SnapinKey.Close();
            return NULL;
        }

        CAtlString AboutGuidString;
        dwSize = MAX_PATH * sizeof(WCHAR);
        if (SnapinKey.QueryStringValue(L"About", Buffer, &dwSize) == ERROR_SUCCESS)
        {
            AboutGuidString = Buffer;
        }

        SnapinKey.Close();

        if (AboutGuidString.IsEmpty())
        {
            return NULL;
        }

        CAtlString GuidString = guidString;

        return new CSnapinCacheEntry(GuidString, AboutGuidString, Name, hImageList);
    }
};
