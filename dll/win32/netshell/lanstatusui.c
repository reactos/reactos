#include <precomp.h>


/// CLSID
/// HKEY_LOCAL_MACHINE\SOFTWARE\Classes\CLSID\{7007ACCF-3202-11D1-AAD2-00805FC1270E}
// IID B722BCCB-4E68-101B-A2BC-00AA00404770
typedef struct
{
    IOleCommandTarget * lpVtbl;
    LONG ref;
}ILanStatusImpl, *LPILanStatusImpl;


INT_PTR
CALLBACK
LANStatusUIDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            return TRUE;
    }
    return FALSE;
}



static
HRESULT
WINAPI
IOleCommandTarget_fnQueryInterface(
    IOleCommandTarget * iface,
    REFIID iid,
    LPVOID * ppvObj)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    *ppvObj = NULL;

    if (IsEqualIID (iid, &IID_IUnknown) ||
        IsEqualIID (iid, &IID_IOleCommandTarget))
    {
        *ppvObj = This;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static
ULONG
WINAPI
IOleCommandTarget_fnAddRef(
    IOleCommandTarget * iface)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    return refCount;
}

static
ULONG
WINAPI
IOleCommandTarget_fnRelease(
    IOleCommandTarget * iface)
{
    ILanStatusImpl * This =  (ILanStatusImpl*)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    if (!refCount) 
    {
        CoTaskMemFree (This);
    }
    return refCount;
}

static
HRESULT
WINAPI
IOleCommandTarget_fnQueryStatus(
    IOleCommandTarget * iface,
    const GUID *pguidCmdGroup,
    ULONG cCmds,
    OLECMD *prgCmds,
    OLECMDTEXT *pCmdText)
{
    MessageBoxW(NULL, L"IOleCommandTarget_fnQueryStatus", NULL, MB_OK);
    return E_NOTIMPL;
}

static
HRESULT
WINAPI
IOleCommandTarget_fnExec(
    IOleCommandTarget * iface,
    const GUID *pguidCmdGroup,
    DWORD nCmdID,
    DWORD nCmdexecopt,
    VARIANT *pvaIn,
    VARIANT *pvaOut)
{
    NOTIFYICONDATA nid;
    HWND hwndDlg;
    //ILanStatusImpl * This =  (ILanStatusImpl*)iface;

    hwndDlg = CreateDialogW(netshell_hInstance, MAKEINTRESOURCEW(IDD_NETSTATUS), NULL, LANStatusUIDlg);
    if (pguidCmdGroup)
    {
        if (IsEqualIID(pguidCmdGroup, &CGID_ShellServiceObject))
        {
            nid.cbSize = sizeof(nid);
            nid.uID = 100;
            nid.uFlags = NIF_ICON;
            nid.u.uVersion = NOTIFYICON_VERSION;
            nid.hWnd = hwndDlg;
            nid.hIcon = LoadIcon(netshell_hInstance, MAKEINTRESOURCE(IDI_SHELL_NETWORK_FOLDER));

            Shell_NotifyIcon(NIM_ADD, &nid);
            return S_OK;
        }
    }
    return S_OK;
}


static const IOleCommandTargetVtbl vt_OleCommandTarget =
{
    IOleCommandTarget_fnQueryInterface,
    IOleCommandTarget_fnAddRef,
    IOleCommandTarget_fnRelease,
    IOleCommandTarget_fnQueryStatus,
    IOleCommandTarget_fnExec,
};


HRESULT WINAPI LanConnectStatusUI_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    ILanStatusImpl * This;

    if (!ppv)
        return E_POINTER;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    This = (ILanStatusImpl *) CoTaskMemAlloc(sizeof (ILanStatusImpl));
    if (!This)
        return E_OUTOFMEMORY;

    This->ref = 1;
    This->lpVtbl = (IOleCommandTarget*)&vt_OleCommandTarget;

    if (!SUCCEEDED (IOleCommandTarget_fnQueryInterface ((IOleCommandTarget*)This, riid, ppv)))
    {
        IOleCommandTarget_Release((IUnknown*)This);
        return E_NOINTERFACE;
    }

    IOleCommandTarget_Release((IUnknown*)This);
    return S_OK;
}



