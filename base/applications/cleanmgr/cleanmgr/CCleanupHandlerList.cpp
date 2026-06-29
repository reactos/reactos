/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CCleanupHandlerList implementation
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#include "cleanmgr.h"

void CCleanupHandlerList::LoadHandlers(WCHAR Drive)
{
    m_DriveStr.Format(L"%c:", Drive);

    CRegKey VolumeCaches;
    if (VolumeCaches.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\VolumeCaches", KEY_READ) != ERROR_SUCCESS)
        return;

    LONG ItemIndex = 0;
    WCHAR szKeyName[MAX_PATH];

    WCHAR wszVolume[] = { Drive, L':', L'\\', UNICODE_NULL };

    while (TRUE)
    {
        DWORD dwSize = _countof(szKeyName);
        if (VolumeCaches.EnumKey(ItemIndex++, szKeyName, &dwSize) != ERROR_SUCCESS)
        {
            break;
        }


        CRegKey hSubKey;
        if (hSubKey.Open(VolumeCaches, szKeyName, KEY_READ) == ERROR_SUCCESS)
        {
            WCHAR GuidStr[50] = {};
            dwSize = _countof(GuidStr);
            if (hSubKey.QueryStringValue(NULL, GuidStr, &dwSize) != ERROR_SUCCESS)
            {
                continue;
            }

            GUID guid = {};
            if (FAILED_UNEXPECTEDLY(CLSIDFromString(GuidStr, &guid)))
                continue;

            CCleanupHandler* handler = new CCleanupHandler(hSubKey, szKeyName, guid);

            if (!handler->Initialize(wszVolume))
            {
                delete handler;
                continue;
            }

            m_Handlers.AddTail(handler);
        }
    }

    // Sort handlers
    BOOL fChanged = m_Handlers.GetCount() > 0;
    while (fChanged)
    {
        fChanged = FALSE;

        for (size_t n = 0; n < m_Handlers.GetCount() - 1; n++)
        {
            POSITION leftPos = m_Handlers.FindIndex(n);
            POSITION rightPos = m_Handlers.FindIndex(n+1);
            CCleanupHandler* left = m_Handlers.GetAt(leftPos);
            CCleanupHandler* right = m_Handlers.GetAt(rightPos);

            if (right->Priority < left->Priority)
            {
                m_Handlers.SwapElements(leftPos, rightPos);
                fChanged = TRUE;
            }
            else if (right->Priority == left->Priority)
            {
                CStringW leftStr(left->wszDisplayName);
                if (leftStr.Compare(right->wszDisplayName) > 0)
                {
                    m_Handlers.SwapElements(leftPos, rightPos);
                    fChanged = TRUE;
                }
            }
        }
    }
}


DWORDLONG
CCleanupHandlerList::ScanDrive(IEmptyVolumeCacheCallBack *picb)
{
    CProgressDlg progress;
    CString Caption;
    Caption.Format(IDS_CALCULATING, m_DriveStr.GetString());
    CStringW Title(MAKEINTRESOURCE(IDS_DISK_CLEANUP));
    progress.Start((DWORD)m_Handlers.GetCount(), Title, Caption);
    int ItemIndex = 0;
    DWORDLONG TotalSpaceUsed = 0;
    ForEach(
        [&](CCleanupHandler *current)
        {
            Caption.Format(IDS_SCANNING, current->wszDisplayName.m_pData);
            progress.Step(++ItemIndex, Caption);

            HRESULT hr = current->Handler->GetSpaceUsed(&current->SpaceUsed, picb);

            if (FAILED_UNEXPECTEDLY(hr))
            {
                current->ShowHandler = false;
                current->StateFlags &= ~HANDLER_STATE_SELECTED;
                return;
            }

            if (current->SpaceUsed == 0 && current->DontShowIfZero())
            {
                current->ShowHandler = false;
                current->StateFlags &= ~HANDLER_STATE_SELECTED;
            }
            TotalSpaceUsed += current->SpaceUsed;
        });
    progress.Stop();

    return TotalSpaceUsed;
}

void
CCleanupHandlerList::ExecuteCleanup(IEmptyVolumeCacheCallBack *picb)
{
    CProgressDlg progress;
    CString Caption;
    Caption.Format(IDS_CLEANING_CAPTION, m_DriveStr.GetString());

    DWORD TotalSelected = 0;
    ForEach(
        [&](CCleanupHandler *current)
        {
            if (current->StateFlags & HANDLER_STATE_SELECTED)
                TotalSelected++;
        });

    CStringW Title(MAKEINTRESOURCE(IDS_DISK_CLEANUP));
    progress.Start(TotalSelected, Title, Caption);
    int ItemIndex = 0;
    ForEach(
        [&](CCleanupHandler *current)
        {
            if (!(current->StateFlags & HANDLER_STATE_SELECTED))
                return;

            Caption.Format(IDS_CLEANING, current->wszDisplayName.m_pData);
            progress.Step(++ItemIndex, Caption);

            // If there is nothing to clean, we might get STG_E_NOMOREFILES
            if (current->SpaceUsed > 0)
            {
                HRESULT hr = current->Handler->Purge(-1, picb);
                if (FAILED_UNEXPECTEDLY(hr))
                    return;
            }
        });
    progress.Stop();
}
