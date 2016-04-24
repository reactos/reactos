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
    virtual HRESULT WINAPI StartProgressDialog(HWND hwndParent, IUnknown *punkEnableModeless, DWORD dwFlags, LPCVOID reserved);
    virtual HRESULT WINAPI StopProgressDialog();
    virtual HRESULT WINAPI SetTitle(LPCWSTR pwzTitle);
    virtual HRESULT WINAPI SetAnimation(HINSTANCE hInstance, UINT uiResourceId);
    virtual BOOL    WINAPI HasUserCancelled();
    virtual HRESULT WINAPI SetProgress64(ULONGLONG ullCompleted, ULONGLONG ullTotal);
    virtual HRESULT WINAPI SetProgress(DWORD dwCompleted, DWORD dwTotal);
    virtual HRESULT WINAPI SetLine(DWORD dwLineNum, LPCWSTR pwzLine, BOOL bPath, LPCVOID reserved);
    virtual HRESULT WINAPI SetCancelMsg(LPCWSTR pwzMsg, LPCVOID reserved);
    virtual HRESULT WINAPI Timer(DWORD dwTimerAction, LPCVOID reserved);

    //////// IOleWindow
    virtual HRESULT WINAPI GetWindow(HWND* phwnd);
    virtual HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode);

DECLARE_REGISTRY_RESOURCEID(IDR_PROGRESSDIALOG)
DECLARE_NOT_AGGREGATABLE(CProgressDialog)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CProgressDialog)
    COM_INTERFACE_ENTRY_IID(IID_IProgressDialog, IProgressDialog)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
END_COM_MAP()
};

#endif /* _PROGRESSDIALOG_H_ */
