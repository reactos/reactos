#include "precomp.h"
#include <shlwapi.h>
#include <shlwapi_undoc.h>

#define PROXY_DESKTOP_CLASS L"Proxy Desktop"

BOOL g_SeparateFolders = FALSE;
HWND g_hwndProxyDesktop = NULL;

// fields indented more are unknown ;P
struct HNFBlock
{
    UINT                cbSize;
    DWORD                       offset4;
    DWORD                       offset8;
    DWORD                       offsetC;
    DWORD                       offset10;
    DWORD                       offset14;
    DWORD                       offset18;
    DWORD                       offset1C;
    DWORD                       offset20;
    DWORD                       offset24;
    DWORD                       offset28;
    DWORD                       offset2C;
    DWORD                       offset30;
    UINT                directoryPidlLength;
    UINT                pidlSize7C;
    UINT                pidlSize80;
    UINT                pathLength;
};

class CProxyDesktop :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CProxyDesktop, CWindow, CFrameWinTraits >
{

public:
    CProxyDesktop(IEThreadParamBlock * parameters)
    {
    }

    virtual ~CProxyDesktop()
    {
    }

    LRESULT OnMessage1037(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TRACE("Proxy Desktop message 1037.\n");
        bHandled = TRUE;
        return TRUE;
    }

    LRESULT OnOpenNewWindow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TRACE("Proxy Desktop message 1035 received.\n");
        bHandled = TRUE;
        SHOnCWMCommandLine((HANDLE) lParam);
        return 0;
    }

    DECLARE_WND_CLASS_EX(PROXY_DESKTOP_CLASS, CS_SAVEBITS | CS_DROPSHADOW, COLOR_3DFACE)

    BEGIN_MSG_MAP(CProxyDesktop)
        MESSAGE_HANDLER(WM_EXPLORER_1037, OnMessage1037)
        MESSAGE_HANDLER(WM_EXPLORER_OPEN_NEW_WINDOW, OnOpenNewWindow)
    END_MSG_MAP()
};

HWND FindShellProxy(LPITEMIDLIST pidl)
{
    /* If there is a proxy desktop in the current process use it */
    if (g_hwndProxyDesktop)
        return g_hwndProxyDesktop;

    /* Try to find the desktop of the main explorer process */
    if (!g_SeparateFolders)
    {
        HWND shell = GetShellWindow();

        if (shell)
        {
            TRACE("Found main desktop.\n");
            return shell;
        }
    }
    else
    {
        TRACE("Separate folders setting enabled. Ignoring main desktop.\n");
    }

    /* The main desktop can't be find so try to see if another process has a proxy desktop */
    HWND proxy = FindWindow(PROXY_DESKTOP_CLASS, NULL);
    if (proxy)
    {
        TRACE("Found proxy desktop.\n");
        return proxy;
    }

    return NULL;
}

HANDLE MakeSharedPacket(IEThreadParamBlock * threadParams, LPCWSTR strPath, int dwProcessId)
{
    HNFBlock* hnfData;
    UINT sharedBlockSize = sizeof(*hnfData);
    UINT directoryPidlLength = 0;
    UINT pidlSize7C = 0;
    UINT pidlSize80 = 0;
    UINT pathLength = 0;
    LPITEMIDLIST pidl80 = threadParams->offset80;

    // Count the total length of the message packet

    // directory PIDL
    if (threadParams->directoryPIDL)
    {
        directoryPidlLength = ILGetSize(threadParams->directoryPIDL);
        sharedBlockSize += directoryPidlLength;
        TRACE("directoryPidlLength=%d\n", directoryPidlLength);
    }

    // another PIDL
    if (threadParams->offset7C)
    {
        pidlSize7C = ILGetSize(threadParams->offset7C);
        sharedBlockSize += pidlSize7C;
        TRACE("pidlSize7C=%d\n", pidlSize7C);
    }

    // This flag indicates the presence of another pidl?
    if (!(threadParams->offset84 & 0x8000))
    {
        if (pidl80)
        {
            pidlSize80 = ILGetSize(pidl80);
            sharedBlockSize += pidlSize80;
            TRACE("pidlSize80=%d\n", pidlSize7C);
        }
    }
    else
    {
        TRACE("pidl80 sent by value = %p\n", pidl80);
        pidlSize80 = 4;
        sharedBlockSize += pidlSize80;
    }

    // The path string
    if (strPath)
    {
        pathLength = 2 * lstrlenW(strPath) + 2;
        sharedBlockSize += pathLength;
        TRACE("pathLength=%d\n", pidlSize7C);
    }

    TRACE("sharedBlockSize=%d\n", sharedBlockSize);

    // Allocate and fill the shared section
    HANDLE hShared = SHAllocShared(0, sharedBlockSize, dwProcessId);
    if (!hShared)
    {
        ERR("Shared section alloc error.\n");
        return 0;
    }

    PBYTE target = (PBYTE) SHLockShared(hShared, dwProcessId);
    if (!target)
    {
        ERR("Shared section lock error. %d\n", GetLastError());
        SHFreeShared(hShared, dwProcessId);
        return 0;
    }

    // Basic information
    hnfData = (HNFBlock*) target;
    hnfData->cbSize = sharedBlockSize;
    hnfData->offset4 = (DWORD) (threadParams->dwFlags);
    hnfData->offset8 = (DWORD) (threadParams->offset8);
    hnfData->offsetC = (DWORD) (threadParams->offset74);
    hnfData->offset10 = (DWORD) (threadParams->offsetD8);
    hnfData->offset14 = (DWORD) (threadParams->offset84);
    hnfData->offset18 = (DWORD) (threadParams->offset88);
    hnfData->offset1C = (DWORD) (threadParams->offset8C);
    hnfData->offset20 = (DWORD) (threadParams->offset90);
    hnfData->offset24 = (DWORD) (threadParams->offset94);
    hnfData->offset28 = (DWORD) (threadParams->offset98);
    hnfData->offset2C = (DWORD) (threadParams->offset9C);
    hnfData->offset30 = (DWORD) (threadParams->offsetA0);
    hnfData->directoryPidlLength = 0;
    hnfData->pidlSize7C = 0;
    hnfData->pidlSize80 = 0;
    hnfData->pathLength = 0;
    target += sizeof(*hnfData);

    // Copy the directory pidl contents
    if (threadParams->directoryPIDL)
    {
        memcpy(target, threadParams->directoryPIDL, directoryPidlLength);
        target += directoryPidlLength;
        hnfData->directoryPidlLength = directoryPidlLength;
    }

    // Copy the other pidl contents
    if (threadParams->offset7C)
    {
        memcpy(target, threadParams->offset7C, pidlSize7C);
        target += pidlSize7C;
        hnfData->pidlSize7C = pidlSize7C;
    }

    // copy the third pidl
    if (threadParams->offset84 & 0x8000)
    {
        *(LPITEMIDLIST*) target = pidl80;
        target += pidlSize80;
        hnfData->pidlSize80 = pidlSize80;
    }
    else if (pidl80)
    {
        memcpy(target, pidl80, pidlSize80);
        target += pidlSize80;
        hnfData->pidlSize80 = pidlSize80;
    }

    // and finally the path string
    if (strPath)
    {
        memcpy(target, strPath, pathLength);
        hnfData->pathLength = pathLength;
    }

    SHUnlockShared(hnfData);

    return hShared;
}

PIE_THREAD_PARAM_BLOCK ParseSharedPacket(HANDLE hData)
{
    HNFBlock * hnfData;
    PBYTE block;
    int pid;
    PIE_THREAD_PARAM_BLOCK params = NULL;

    if (!hData)
        goto cleanup0;

    pid = GetCurrentProcessId();
    block = (PBYTE) SHLockShared(hData, pid);

    hnfData = (HNFBlock *) block;
    if (!block)
        goto cleanup2;

    if (hnfData->cbSize < sizeof(HNFBlock))
        goto cleanup2;

    params = SHCreateIETHREADPARAM(0, hnfData->offset8, 0, 0);
    if (!params)
        goto cleanup2;

    params->dwFlags = hnfData->offset4;
    params->offset8 = hnfData->offset8;
    params->offset74 = hnfData->offsetC;
    params->offsetD8 = hnfData->offset10;
    params->offset84 = hnfData->offset14;
    params->offset88 = hnfData->offset18;
    params->offset8C = hnfData->offset1C;
    params->offset90 = hnfData->offset20;
    params->offset94 = hnfData->offset24;
    params->offset98 = hnfData->offset28;
    params->offset9C = hnfData->offset2C;
    params->offsetA0 = hnfData->offset30;

    block += sizeof(*hnfData);
    if (hnfData->directoryPidlLength)
    {
        LPITEMIDLIST pidl = NULL;
        if (*block)
            pidl = ILClone((LPITEMIDLIST) block);
        params->directoryPIDL = pidl;

        block += hnfData->directoryPidlLength;
    }

    if (hnfData->pidlSize7C)
    {
        LPITEMIDLIST pidl = NULL;
        if (*block)
            pidl = ILClone((LPITEMIDLIST) block);
        params->offset7C = pidl;

        block += hnfData->pidlSize80;
    }

    if (hnfData->pidlSize80)
    {
        if (!(params->offset84 & 0x8000))
        {
            params->offset80 = *(LPITEMIDLIST *) block;
        }
        else
        {
            LPITEMIDLIST pidl = NULL;
            if (*block)
                pidl = ILClone((LPITEMIDLIST) block);
            params->offset80 = pidl;
        }

        block += hnfData->pidlSize80;
    }

    if (hnfData->pathLength)
    {
        CComPtr<IShellFolder> psfDesktop;
        PWSTR strPath = (PWSTR) block;

        if (FAILED(SHGetDesktopFolder(&psfDesktop)))
        {
            params->directoryPIDL = NULL;
            goto cleanup0;
        }

        if (FAILED(psfDesktop->ParseDisplayName(NULL, NULL, strPath, NULL, &params->directoryPIDL, NULL)))
        {
            params->directoryPIDL = NULL;
            goto cleanup0;
        }
    }

cleanup2:
    SHUnlockShared(hnfData);
    SHFreeShared(hData, pid);

cleanup0:
    if (!params->directoryPIDL)
    {
        SHDestroyIETHREADPARAM(params);
        return NULL;
    }

    return params;
}


static HRESULT ExplorerMessageLoop(IEThreadParamBlock * parameters)
{
    HRESULT hResult;
    MSG Msg;
    BOOL Ret;

    // Tell the thread ref we are using it.
    if (parameters && parameters->offsetF8)
        parameters->offsetF8->AddRef();

    /* Handle /e parameter */
     UINT wFlags = 0;
     if ((parameters->dwFlags & SH_EXPLORER_CMDLINE_FLAG_E))
        wFlags |= SBSP_EXPLOREMODE;

    /* Handle /select parameter */
    PUITEMID_CHILD pidlSelect = NULL;
    if ((parameters->dwFlags & SH_EXPLORER_CMDLINE_FLAG_SELECT) &&
        (ILGetNext(parameters->directoryPIDL) != NULL))
    {
        pidlSelect = ILClone(ILFindLastID(parameters->directoryPIDL));
        ILRemoveLastID(parameters->directoryPIDL);
    }

    CComPtr<IShellBrowser> psb;
    hResult = CShellBrowser_CreateInstance(IID_PPV_ARG(IShellBrowser, &psb));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = psb->BrowseObject(parameters->directoryPIDL, wFlags);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (pidlSelect != NULL)
    {
        CComPtr<IShellView> shellView;
        hResult = psb->QueryActiveShellView(&shellView);
        if (SUCCEEDED(hResult))
        {
            shellView->SelectItem(pidlSelect, SVSI_SELECT | SVSI_FOCUSED | SVSI_ENSUREVISIBLE);
        }
        ILFree(pidlSelect);
    }

    CComPtr<IBrowserService2> browser;
    hResult = psb->QueryInterface(IID_PPV_ARG(IBrowserService2, &browser));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    psb.Release();

    while ((Ret = GetMessage(&Msg, NULL, 0, 0)) != 0)
    {
        if (Ret == -1)
        {
            // Error: continue or exit?
            break;
        }

        if (Msg.message == WM_QUIT)
            break;

        if (browser->v_MayTranslateAccelerator(&Msg) != S_OK)
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }

    int nrc = browser->Release();
    if (nrc > 0)
    {
        DbgPrint("WARNING: There are %d references to the CShellBrowser active or leaked.\n", nrc);
    }

    browser.Detach();

    // Tell the thread ref we are not using it anymore.
    if (parameters && parameters->offsetF8)
        parameters->offsetF8->Release();

    return hResult;
}

static DWORD WINAPI BrowserThreadProc(LPVOID lpThreadParameter)
{
    IEThreadParamBlock * parameters = (IEThreadParamBlock *) lpThreadParameter;

    OleInitialize(NULL);
    ExplorerMessageLoop(parameters);

    /* Destroying the parameters releases the thread reference */
    SHDestroyIETHREADPARAM(parameters);

    /* Wake up the proxy desktop thread so it can check whether the last browser thread exited */
    /* Use PostMessage in order to force GetMessage to return and check if all browser windows have exited */
    PostMessageW(FindShellProxy(NULL), WM_EXPLORER_1037, 0, 0);

    OleUninitialize();

    return 0;
}

/*************************************************************************
* SHCreateIETHREADPARAM		[BROWSEUI.123]
*/
extern "C" IEThreadParamBlock *WINAPI SHCreateIETHREADPARAM(
    long param8, long paramC, IUnknown *param10, IUnknown *param14)
{
    IEThreadParamBlock                      *result;

    TRACE("SHCreateIETHREADPARAM\n");

    result = (IEThreadParamBlock *) LocalAlloc(LMEM_ZEROINIT, sizeof(*result));
    if (result == NULL)
        return NULL;
    result->offset0 = param8;
    result->offset8 = paramC;
    result->offsetC = param10;
    if (param10 != NULL)
        param10->AddRef();
    result->offset14 = param14;
    if (param14 != NULL)
        param14->AddRef();
    return result;
}

/*************************************************************************
* SHCloneIETHREADPARAM			[BROWSEUI.124]
*/
extern "C" IEThreadParamBlock *WINAPI SHCloneIETHREADPARAM(IEThreadParamBlock *param)
{
    IEThreadParamBlock                      *result;

    TRACE("SHCloneIETHREADPARAM\n");

    result = (IEThreadParamBlock *) LocalAlloc(LMEM_FIXED, sizeof(*result));
    if (result == NULL)
        return NULL;
    *result = *param;
    if (result->directoryPIDL != NULL)
        result->directoryPIDL = ILClone(result->directoryPIDL);
    if (result->offset7C != NULL)
        result->offset7C = ILClone(result->offset7C);
    if (result->offset80 != NULL)
        result->offset80 = ILClone(result->offset80);
    if (result->offset70 != NULL)
        result->offset70->AddRef();
#if 0
    if (result->offsetC != NULL)
        result->offsetC->Method2C();
#endif
    return result;
}

/*************************************************************************
* SHDestroyIETHREADPARAM		[BROWSEUI.126]
*/
extern "C" void WINAPI SHDestroyIETHREADPARAM(IEThreadParamBlock *param)
{
    TRACE("SHDestroyIETHREADPARAM\n");

    if (param == NULL)
        return;
    if (param->directoryPIDL != NULL)
        ILFree(param->directoryPIDL);
    if (param->offset7C != NULL)
        ILFree(param->offset7C);
    if ((param->dwFlags & 0x80000) == 0 && param->offset80 != NULL)
        ILFree(param->offset80);
    if (param->offset14 != NULL)
        param->offset14->Release();
    if (param->offset70 != NULL)
        param->offset70->Release();
    if (param->offset78 != NULL)
        param->offset78->Release();
    if (param->offsetC != NULL)
        param->offsetC->Release();
    if (param->offsetF8 != NULL)
        param->offsetF8->Release();
    LocalFree(param);
}

/*************************************************************************
* SHOnCWMCommandLine			[BROWSEUI.127]
*/
extern "C" BOOL WINAPI SHOnCWMCommandLine(HANDLE hSharedInfo)
{
    TRACE("SHOnCWMCommandLine\n");

    PIE_THREAD_PARAM_BLOCK params = ParseSharedPacket(hSharedInfo);

    if (params)
        return SHOpenFolderWindow(params);

    SHDestroyIETHREADPARAM(params);

    return FALSE;
}

/*************************************************************************
* SHOpenFolderWindow			[BROWSEUI.102]
* see SHOpenNewFrame below for remarks
*/
extern "C" HRESULT WINAPI SHOpenFolderWindow(PIE_THREAD_PARAM_BLOCK parameters)
{
    HANDLE                                  threadHandle;
    DWORD                                   threadID;

    // Only try to convert the pidl when it is going to be printed
    if (TRACE_ON(browseui))
    {
        WCHAR debugStr[MAX_PATH + 2] = { 0 };
        if (!ILGetDisplayName(parameters->directoryPIDL, debugStr))
        {
            debugStr[0] = UNICODE_NULL;
        }
        TRACE("SHOpenFolderWindow %p(%S)\n", parameters->directoryPIDL, debugStr);
    }

    PIE_THREAD_PARAM_BLOCK paramsCopy = SHCloneIETHREADPARAM(parameters);

    SHGetInstanceExplorer(&(paramsCopy->offsetF8));
    threadHandle = CreateThread(NULL, 0x10000, BrowserThreadProc, paramsCopy, 0, &threadID);
    if (threadHandle != NULL)
    {
        CloseHandle(threadHandle);
        return S_OK;
    }
    SHDestroyIETHREADPARAM(paramsCopy);
    return E_FAIL;
}

// 75FA56C1h
// (pidl, 0, -1, 1)
// this function should handle creating a new process if needed, but I'm leaving that out for now
// this function always opens a new window - it does NOT check for duplicates
/*************************************************************************
* SHOpenNewFrame				[BROWSEUI.103]
*/
extern "C" HRESULT WINAPI SHOpenNewFrame(LPITEMIDLIST pidl, IUnknown *paramC, long param10, DWORD dwFlags)
{
    IEThreadParamBlock                      *parameters;

    TRACE("SHOpenNewFrame\n");

    parameters = SHCreateIETHREADPARAM(0, 1, paramC, NULL);
    if (parameters == NULL)
    {
        ILFree(pidl);
        return E_OUTOFMEMORY;
    }
    if (paramC != NULL)
        parameters->offset10 = param10;
    parameters->directoryPIDL = pidl;
    parameters->dwFlags = dwFlags;

    HRESULT hr = SHOpenFolderWindow(parameters);

    SHDestroyIETHREADPARAM(parameters);

    return hr;
}

/*************************************************************************
* SHCreateFromDesktop			[BROWSEUI.106]
* parameter is a FolderInfo
*/
BOOL WINAPI SHCreateFromDesktop(_In_ PEXPLORER_CMDLINE_PARSE_RESULTS parseResults)
{
    TRACE("SHCreateFromDesktop\n");

    IEThreadParamBlock * parameters = SHCreateIETHREADPARAM(0, 0, 0, 0);
    if (!parameters)
        return FALSE;

    PCWSTR strPath = NULL;
    if (parseResults->dwFlags & SH_EXPLORER_CMDLINE_FLAG_STRING)
    {
        if (parseResults->pidlPath)
        {
            WARN("strPath and pidlPath are both assigned. This shouldn't happen.\n");
        }

        strPath = parseResults->strPath;
    }

    parameters->dwFlags = parseResults->dwFlags;
    parameters->offset8 = parseResults->nCmdShow;

    LPITEMIDLIST pidl = parseResults->pidlPath ? ILClone(parseResults->pidlPath) : NULL;
    if (!pidl && parseResults->dwFlags & SH_EXPLORER_CMDLINE_FLAG_STRING)
    {
        if (parseResults->strPath && parseResults->strPath[0])
        {
            pidl = ILCreateFromPathW(parseResults->strPath);
        }
    }

    // HACK! This shouldn't happen! SHExplorerParseCmdLine needs fixing.
    if (!pidl)
    {
        SHGetFolderLocation(NULL, CSIDL_PERSONAL, NULL, NULL, &pidl);
    }

    parameters->directoryPIDL = pidl;

    // Try to find the owner of the idlist, if we aren't running /SEPARATE
    HWND desktop = NULL;
    if (!(parseResults->dwFlags & SH_EXPLORER_CMDLINE_FLAG_SEPARATE))
        desktop = FindShellProxy(parameters->directoryPIDL);

    // If found, ask it to open the new window
    if (desktop)
    {
        TRACE("Found desktop hwnd=%p\n", desktop);

        DWORD dwProcessId;

        GetWindowThreadProcessId(desktop, &dwProcessId);
        AllowSetForegroundWindow(dwProcessId);

        HANDLE hShared = MakeSharedPacket(parameters, strPath, dwProcessId);
        if (hShared)
        {
            TRACE("Sending open message...\n");

            PostMessageW(desktop, WM_EXPLORER_OPEN_NEW_WINDOW, 0, (LPARAM) hShared);
        }

        SHDestroyIETHREADPARAM(parameters);
        return TRUE;
    }

    TRACE("Desktop not found or separate flag requested.\n");

    // Else, start our own message loop!
    HRESULT hr = CoInitialize(NULL);
    CProxyDesktop * proxy = new CProxyDesktop(parameters);
    if (proxy)
    {
        g_hwndProxyDesktop = proxy->Create(0);

        LONG refCount;
        CComPtr<IUnknown> thread;
        if (SHCreateThreadRef(&refCount, &thread) >= 0)
        {
            SHSetInstanceExplorer(thread);
            if (strPath)
                parameters->directoryPIDL = ILCreateFromPath(strPath);
            SHOpenFolderWindow(parameters);
            parameters = NULL;
            thread.Release();
        }

        MSG Msg;
        while (GetMessageW(&Msg, 0, 0, 0) && refCount)
        {
            TranslateMessage(&Msg);
            DispatchMessageW(&Msg);
        }

        DestroyWindow(g_hwndProxyDesktop);

        delete proxy;
    }

    if (SUCCEEDED(hr))
        CoUninitialize();

    SHDestroyIETHREADPARAM(parameters);

    return TRUE;
}
