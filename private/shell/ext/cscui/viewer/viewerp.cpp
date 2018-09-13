#include "pch.h"
#pragma hdrstop

#include <tchar.h>
#include <process.h>
#include "alloc.h"
#include "cachview.h"
#include "sharedlg.h"
#include "viewerp.h"
#include "cnxcache.h"
#include "msgbox.h"
#include "uuid.h"
#include "idlhelp.h"

CnxNameCache g_TheCnxNameCache;

HINSTANCE Viewer::g_hInstance = NULL;
bool      Viewer::m_bShowDebugInfo = false;
//
// Originally, m_bShowSparseFiles was initialized as false then
// was changeable through the registry.  We decided to always show sparse
// files so it is now initialized as true.  I've left the original
// logic in case we change our minds again.  [brianau - 2/23/98]
//
bool      Viewer::m_bShowSparseFiles = true;


void SetDebugParams(
    void
    )
{
    //
    // Change these to alter the debugger output.
    //
    DBGMODULE(TEXT("CSCUI"));  // Name of module displayed with messages.
    DBGTRACEMASK(DM_NONE);     // What components are traced.
    DBGTRACELEVEL(DL_MID);     // How much detail to trace.
    DBGPRINTMASK(DM_NONE);     // What components to print.
    DBGPRINTLEVEL(DL_MID);     // How much detail to print.
    DBGTRACEVERBOSE(false);    // Include file/line info in trace output?
    DBGPRINTVERBOSE(false);    // Include file/line info in print output?
}


void LoadModuleHandle(
    void
    )
{
    if (NULL == Viewer::g_hInstance)
        Viewer::g_hInstance = GetModuleHandle(TEXT("cscui"));
    if (NULL == Viewer::g_hInstance)
        DWORD dwError = GetLastError();
}


bool
ActivateRunningViewer(
    void
    )
{
    HWND hwnd = FindWindowEx(NULL, NULL, WC_NETCACHE_VIEWER, NULL);
    if (NULL != hwnd)
    {
        ShowWindow(hwnd, SW_SHOWNORMAL);
        return boolify(SetForegroundWindow(hwnd));
    }
    return false;
}

//
// This function invokes the new cache viewer shell namespace extension.
// 
INT 
CSCViewCacheInternalNSE(
    void
    )
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST;
    shei.nShow      = SW_SHOWNORMAL;

    if (SUCCEEDED(CreateOfflineFolderIDList((LPITEMIDLIST *)(&shei.lpIDList))))
    {
        ShellExecuteEx(&shei);
        ILFree((LPITEMIDLIST)(shei.lpIDList));
    }
    return 0;
}


INT
CSCViewCacheInternalW(
    INT iView,
    LPCWSTR pszShareW,
    bool bWait
    )
{
    INT iResult = 1;

    SetDebugParams();
    LoadModuleHandle();

    try
    {
        if (g_pSettings->CacheViewerMode() < CSettings::eViewerReadOnly )
        {
            DBGERROR((TEXT("Policy prohibits opening cache viewer.")));
            CscMessageBox(NULL, 
                          MB_OK | MB_ICONWARNING,
                          Viewer::g_hInstance,
                          IDS_ERR_POLICY_NOVIEWCACHE);
            return -1;
        }

        const WCHAR szBlankW[] = L"";
        if (NULL == pszShareW)
            pszShareW = szBlankW;

        CacheWindow::ViewType eView = CacheWindow::ViewType(iView);
        CString strShare(pszShareW);

        if (bWait)
        {
            CacheWindow viewer(Viewer::g_hInstance);
            iResult = viewer.Run(eView, strShare);
        }
        else
        {
            if (!ActivateRunningViewer())
            {
                //
                // Run the viewer in a separate process.
                // This will start rundll32.exe and call CSCViewCacheRunDll 
                // to run the viewer modally in that process.
                //
                CString strCmdLine;
                STARTUPINFO si = {0};
                PROCESS_INFORMATION pi ={0};
                BOOL bSuccess = FALSE;

#ifdef UNICODE            
                strCmdLine.Format(TEXT("rundll32.exe cscui.dll,CSCViewCacheRunDll %1!d! %2"), 
                                  int(eView), strShare.Cstr());
#else
                strCmdLine.Format(TEXT("rundll32.exe cscui.dll,CSCViewCacheRunDllA %1!d! %2"), 
                                  int(eView), strShare.Cstr());
#endif
                si.cb = sizeof(si);
                si.lpDesktop = TEXT("WinSta0\\Default");

                bSuccess = CreateProcess(NULL,
                                         strCmdLine,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         0,
                                         NULL,
                                         NULL,
                                         &si,
                                         &pi);

                if (bSuccess)
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CSCViewCacheInternalW"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    return iResult;
}


INT
CSCViewCacheInternalA(
    INT iInitialView,
    LPCSTR pszShareA,
    bool bWait
    )
{
    INT iResult = 1;
    const CHAR szBlankA[] = "";
    if (NULL == pszShareA)
        pszShareA = szBlankA;

    USES_CONVERSION;
    iResult = CSCViewCacheInternalW(iInitialView, A2W(pszShareA), bWait);
    return iResult;
}


//
// Trivial class for passing parameters to share dialog thread proc.
//
class ShareDialogThreadParams
{
    public:
        ShareDialogThreadParams(HWND hwndParent,
                                LPCTSTR pszShare)
                                : m_hwndParent(hwndParent),
                                  m_hModule(NULL),
                                  m_strShare(pszShare) { }
    
        HWND    m_hwndParent;
        HMODULE m_hModule;
        CString m_strShare;

        void SetModuleHandle(HINSTANCE hModule)
            { m_hModule = hModule; }
};


//
// The share dialog's thread proc.
//
UINT WINAPI
ShareDialogThreadProc(
    LPVOID pvParam
    )
{
    ShareDialogThreadParams *ptp = reinterpret_cast<ShareDialogThreadParams *>(pvParam);
    DBGASSERT((NULL != ptp));

    HMODULE hModule = ptp->m_hModule; // Save local copy.

    try
    {
        SharePropSheet dlg(Viewer::g_hInstance,
                           &g_cRefCount,
                           ptp->m_hwndParent,
                           ptp->m_strShare);

        dlg.Run();
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in ShareDialogThreadProc"), e.dwError));
        CscWin32Message(NULL, e.dwError, CSCUI::SEV_ERROR);
    }
    delete ptp;

    if (NULL != hModule)
        FreeLibraryAndExitThread(hModule, 0);

    return 0;
}


//
// HACK:
// If pszShareW points to an empty string, we only show the CSC configuration
// page.  The share summary-options combo property sheet already existed.
// Later on we needed to display only the options in a prop sheet all by itself.
// No time to do a good redesign to take this into account so we just pass 
// in an empty share name string.
//
VOID
CSCViewShareSummaryInternalW(
    LPCWSTR pszShareW,
    HWND hwndParent,
    BOOL bModal
    )
{
    SetDebugParams();
    LoadModuleHandle();
    try
    {
        CString strShare(pszShareW);
        if (bModal)
        {
            SharePropSheet dlg(Viewer::g_hInstance,
                               &g_cRefCount,
                               hwndParent,
                               strShare);
            dlg.Run();
        }
        else
        {
            //
            // This thread param buffer will be deleted by the
            // thread proc.
            //
            ShareDialogThreadParams *ptp = new ShareDialogThreadParams(hwndParent,
                                                                       strShare);
            if (NULL != ptp)
            {
                //
                // LoadLibrary on ourselves so that we stay in memory even
                // if the caller calls FreeLibrary.  We'll call FreeLibrary
                // when the thread proc exits.
                //
                ptp->SetModuleHandle(LoadLibrary(TEXT("cscui.dll")));

                DWORD idThread;
                HANDLE hThread = (HANDLE)_beginthreadex(NULL,
                                           0,          // Default stack size
                                           ShareDialogThreadProc,
                                           (LPVOID)ptp,
                                           0,
                                           (UINT *)&idThread);

                if (INVALID_HANDLE_VALUE != hThread)
                {
                    CloseHandle(hThread);
                }
            }
        }
    }
    catch(CException& e)
    {
        DBGERROR((TEXT("C++ exception %d caught in CSCViewShareSummaryInternalW"), e.dwError));
        CscWin32Message(NULL, e. dwError, CSCUI::SEV_ERROR);
    }
}


VOID
CSCViewShareSummaryInternalA(
    LPCSTR pszShareA,
    HWND hwndParent, 
    BOOL bModal
    )
{
    USES_CONVERSION;
    CSCViewShareSummaryInternalW(A2W(pszShareA), hwndParent, bModal);
}


VOID
CSCViewOptionsInternal(
    HWND hwndParent,
    BOOL bModal
    )
{
    CSCViewShareSummaryInternalW(L"", hwndParent, bModal);
}
