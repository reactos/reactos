//***   uassist.cpp -- User Assist helpers (retail and debug)
//
// DESCRIPTION
//  this file has the shared-source 'master' implementation.  it is
// #included in each DLL that uses it.  NEEDED because of ENTERCRITICAL
// as stocklib.dll does not have a critical section in it.
//
//  clients do something like:
//      #include "priv.h"   // for types, ASSERT, DM_*, DF_*, etc.
//      #include "../lib/uassist.cpp"
//
//  we cache the UAssist object and provide thunks for 'safe' access to it.

#include "uemapp.h"

#define DM_UASSIST             0

IUserAssist *g_uempUa;      // 0:uninit, -1:failed, o.w.:cached obj

//***   GetUserAssist -- get (and create) cached UAssist object
//
IUserAssist *GetUserAssist()
{
    HRESULT hr;
    IUserAssist *pua;

    if (g_uempUa == 0) {
        // re: CLSCTX_NO_CODE_DOWNLOAD
        // an ('impossible') failed CCI of UserAssist is horrendously slow.
        // e.g. click on the start menu, wait 10 seconds before it pops up.
        // we'd rather fail than hose perf like this, plus this class should
        // never be remote.
        // BUGBUG there must be a better way to tell if CLSCTX_NO_CODE_DOWNLOAD
        // is supported, i've sent mail to 'com' to find out...
        DWORD dwFlags = staticIsOS(OS_NT5) ? (CLSCTX_INPROC|CLSCTX_NO_CODE_DOWNLOAD) : CLSCTX_INPROC;
        hr = THR(CoCreateInstance(CLSID_UserAssist, NULL, dwFlags, IID_IUserAssist, (void**)&pua));
        ASSERT(SUCCEEDED(hr) || pua == 0);  // follow COM rules

        if (pua) {
            HINSTANCE hInst;

            hInst = SHPinDllOfCLSID(&CLSID_UserAssist); // cached across threads
            // we're toast if this fails!!! (but happily, that's 'impossible')
            // e.g. during logon when grpconv.exe is ShellExec'ed, we do
            // a GetUserAssist, which caches a ptr to browseui's singleton
            // object.  then when the ShellExec returns, we do CoUninit,
            // which would free up the (non-pinned) browseui.dll.  then
            // a later use of the cache would go off into space.
            ASSERT(hInst);          // 'impossible', since the CCI succeeded
        }

        ENTERCRITICAL;
        if (g_uempUa == 0) {
            g_uempUa = pua;     // xfer refcnt (if any)
            if (!pua) {
                // mark it failed so we won't try any more
                g_uempUa = (IUserAssist *)-1;
            }
            pua = NULL;
        }
        LEAVECRITICAL;
        if (pua)
            pua->Release();
        TraceMsg(DM_UASSIST, "sl.gua: pua=0x%x g_uempUa=%x", pua, g_uempUa);
    }

    return (g_uempUa == (IUserAssist *)-1) ? 0 : g_uempUa;
}

extern "C"
BOOL UEMIsLoaded()
{
    BOOL fRet;

    fRet = GetModuleHandle(TEXT("ole32.dll")) &&
        GetModuleHandle(TEXT("browseui.dll"));
    
    if (!fRet)
        TraceMsg(TF_WARNING, "uemil: UEMIsLoaded ret=0 (.dll not loaded)");
    
    return fRet;
}

//***   UEMFireEvent, QueryEvent, SetEvent -- 'safe' thunks
// DESCRIPTION
//  call these so don't have to worry about cache or whether Uassist object
// even was successfully created.
extern "C"
HRESULT UEMFireEvent(const GUID *pguidGrp, int eCmd, DWORD dwFlags, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = E_FAIL;
    IUserAssist *pua;

    pua = GetUserAssist();
    if (pua) {
        hr = pua->FireEvent(pguidGrp, eCmd, dwFlags, wParam, lParam);
    }
    return hr;
}

extern "C"
HRESULT UEMQueryEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui)
{
    HRESULT hr = E_FAIL;
    IUserAssist *pua;

    pua = GetUserAssist();
    if (pua) {
        hr = pua->QueryEvent(pguidGrp, eCmd, wParam, lParam, pui);
    }
    return hr;
}

extern "C"
HRESULT UEMSetEvent(const GUID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam, LPUEMINFO pui)
{
    HRESULT hr = E_FAIL;
    IUserAssist *pua;

    pua = GetUserAssist();
    if (pua) {
        hr = pua->SetEvent(pguidGrp, eCmd, wParam, lParam, pui);
    }
    return hr;
}
