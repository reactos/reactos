/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Progress dialog implementation
 * COPYRIGHT:   Copyright 2023-2025 Mark Jansen <mark.jansen@reactos.org>
 */

#pragma once

class CProgressDlg
{
    CComPtr<IProgressDialog> m_spProgress;
    DWORD m_dwTotal = 0;
public:

    ~CProgressDlg()
    {
        Stop();
    }


    void Start(DWORD dwTotalSteps, LPCWSTR Title, LPCWSTR Text)
    {
        HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC, IID_PPV_ARG(IProgressDialog, &m_spProgress));
        if (FAILED_UNEXPECTEDLY(hr))
            return;

        m_dwTotal = dwTotalSteps;

        m_spProgress->SetTitle(Title);
        m_spProgress->SetLine(2, Text, TRUE, NULL);
        m_spProgress->StartProgressDialog(NULL, NULL, PROGDLG_NOMINIMIZE, NULL);
        m_spProgress->SetProgress(0, m_dwTotal);
    }

    void Step(DWORD dwProgress, LPCWSTR Text)
    {
        m_spProgress->SetProgress(dwProgress, m_dwTotal);
        m_spProgress->SetLine(1, Text, TRUE, NULL);
    }

    void Stop()
    {
        if (m_spProgress)
        {
            m_spProgress->StopProgressDialog();
            m_spProgress.Release();
        }
    }
};
