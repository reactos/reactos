/*
 * Progress dialog
 *
 * Copyright 2014              Huw Campbell
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

#ifndef _PROGRESSDIALOG_H_
#define _PROGRESSDIALOG_H_

class CProgressDialog :
    public CComCoClass<CProgressDialog, &CLSID_ProgressDialog>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IProgressDialog,
    public IOleWindow
{
public:
    CRITICAL_SECTION cs;
    HWND hwnd;
    DWORD dwFlags;
    DWORD dwUpdate;
    LPWSTR lines[3];
    LPWSTR cancelMsg;
    LPWSTR title;
    BOOL isCancelled;
    ULONGLONG ullCompleted;
    ULONGLONG ullTotal;
    HWND hwndDisabledParent;
    void set_progress_marquee();
    void update_dialog(DWORD dwUpdate);
    void end_dialog();

    UINT clockHand;
    struct progressMark {
        ULONGLONG ullMark;
        DWORD     dwTime;
    };
    progressMark progressClock[30];
    DWORD dwStartTime;

    CProgressDialog();
    ~CProgressDialog();

    // IProgressDialog
    STDMETHOD(StartProgressDialog)(HWND hwndParent, IUnknown *punkEnableModeless, DWORD dwFlags, LPCVOID reserved) override;
    STDMETHOD(StopProgressDialog)() override;
    STDMETHOD(SetTitle)(LPCWSTR pwzTitle) override;
    STDMETHOD(SetAnimation)(HINSTANCE hInstance, UINT uiResourceId) override;
    STDMETHOD_(BOOL, HasUserCancelled)() override;
    STDMETHOD(SetProgress64)(ULONGLONG ullCompleted, ULONGLONG ullTotal) override;
    STDMETHOD(SetProgress)(DWORD dwCompleted, DWORD dwTotal) override;
    STDMETHOD(SetLine)(DWORD dwLineNum, LPCWSTR pwzLine, BOOL bPath, LPCVOID reserved) override;
    STDMETHOD(SetCancelMsg)(LPCWSTR pwzMsg, LPCVOID reserved) override;
    STDMETHOD(Timer)(DWORD dwTimerAction, LPCVOID reserved) override;

    // IOleWindow
    STDMETHOD(GetWindow)(HWND* phwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

DECLARE_REGISTRY_RESOURCEID(IDR_PROGRESSDIALOG)
DECLARE_NOT_AGGREGATABLE(CProgressDialog)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProgressDialog)
    COM_INTERFACE_ENTRY_IID(IID_IProgressDialog, IProgressDialog)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
END_COM_MAP()
};

#endif /* _PROGRESSDIALOG_H_ */
