//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       padmain.cxx
//
//  Contents:   WinMain and associated functions.
//
//-------------------------------------------------------------------------

#include "padhead.hxx"

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_SHLOBJ_H_
#define X_SHLOBJ_H_
#include "shlobj.h"
#endif

void CreatePerfCtl(DWORD dwFlags, void * pvHost);
void DeletePerfCtl();

MtDefine(SAryThreads_pv, Pad, "s_aryThreads::_pv")
MtDefine(PADTHREADSTATE, Pad, "PADTHREADSTATE")
DeclareTag(tagPadNoCycleBreak, "Pad", "Don't call UrlZonesDetach in wininet.dll")

//+-------------------------------------------------------------------------
//
// Definitions required for files linked from other directories.
//
//--------------------------------------------------------------------------

CRITICAL_SECTION    CGlobalLock::s_cs;                  // Critical section to protect globals
#if DBG==1
DWORD               CGlobalLock::s_dwThreadID = 0;      // Thread ID which owns the critical section
LONG                CGlobalLock::s_cNesting = 0;        // Enter/LeaveCriticalSection nesting level (DEBUG only)
#endif

// used for assert to fool the compiler
DWORD g_dwFALSE = 0;

// default to loading MSHTML.DLL from the pad directory
BOOL g_fLoadSystemMSHTML = 0;

HINSTANCE           g_hInstCore;
HINSTANCE           g_hInstResource;
EXTERN_C HANDLE     g_hProcessHeap;
HANDLE              g_hProcessHeap;
DWORD               g_dwTls;
interface           IMultiLanguage * g_pMultiLanguage;

extern void DeinitDynamicLibraries();
void DeinitMultiLanguage() {}

DeclareTag(tagDumpHeapsOnExit, "Pad", "Dump heaps on exit");

//+-------------------------------------------------------------------------
//
// Global variables
//
//--------------------------------------------------------------------------

ATOM               g_atomMain;
HWND               g_hwndMain;
static LONGLONG    s_liPerfFreq;       // perf frequency
static LONGLONG    s_liTimeStart;      // time at start of subtest
static int         s_iInterval;
static long        s_lSecondaryObjCount = 0;

ExternTag(tagScriptLog);

//+-------------------------------------------------------------------------
//
// Timing stuff
//
//--------------------------------------------------------------------------

#define MAXINTERVALS 200
#define MAXNAME      100


struct INTERVAL
{
    int iTime;
    TCHAR achName[MAXNAME];
    BOOL fReset;
}
    intervals[MAXINTERVALS];

class CPadEvent : public CEventCallBack
{
public:

    virtual int Event(LPCTSTR pchEvent, BOOL fReset = FALSE);
};

int CPadEvent::Event(LPCTSTR pchName, BOOL fReset)
{
    LONGLONG    liTimeStop;       // time at stop of subtest

    LOCK_GLOBALS;

    QueryPerformanceCounter((LARGE_INTEGER *)&liTimeStop);
    if (s_iInterval < MAXINTERVALS)
    {
        intervals[s_iInterval].iTime = (int)(((liTimeStop - s_liTimeStart) * 1000) / s_liPerfFreq);
        _tcsncpy(intervals[s_iInterval].achName, pchName, MAXNAME);
        intervals[s_iInterval].fReset = fReset;
        s_iInterval++;
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&s_liTimeStart);

    return 0;
}


static CDataAry<THREAD_HANDLE> s_aryThreads(Mt(SAryThreads_pv));
                                // array of thread handles.
                                // This array is used only by the main
                                // thread.


CPadEvent g_PadEvent;
CPadEvent * g_pEvent = NULL;

void DumpTimes()
{
#if 0
    CHAR    rgch[MAX_PATH];
    UINT    cch = (UINT) GetModuleFileNameA(g_hInstCore, rgch, sizeof(rgch));

    if (s_iInterval)
    {
        Assert(rgch[cch-4] == '.');
        strcpy(&rgch[cch-4], ".tim");
        FILE * f = fopen(rgch, "w");
        int iAbsTime = 0;

        if (f == NULL)
            return;

        for (int i = 0; i < s_iInterval; i++)
        {
            iAbsTime += intervals[i].iTime;
            if (intervals[i].fReset)
            {
                iAbsTime = 0;
            }
            fprintf(f, "%S,\t%d\t%d\n", intervals[i].achName, iAbsTime, intervals[i].iTime);
        }

        fclose(f);
    }
#endif
}

void
IncrementObjectCount()
{
    GetThreadState()->lObjCount++;
    Verify(InterlockedIncrement(&s_lSecondaryObjCount) > 0);
}

void
DecrementObjectCount()
{
    if (--GetThreadState()->lObjCount == 0)
    {
        PostQuitMessage(0);
    }
    Verify(InterlockedDecrement(&s_lSecondaryObjCount) >= 0);
}

void
CheckObjCount()
{
    if (GetThreadState()->lObjCount)
    {
        TraceTag((tagError,
                "Thread (TID=0x%08x) terminated with primary object count=%d",
                GetCurrentThreadId(), GetThreadState()->lObjCount));
    }
}

PADTHREADSTATE *
GetThreadState()
{
    PADTHREADSTATE * pts = (PADTHREADSTATE *)TlsGetValue(g_dwTls);
    AssertSz(pts != NULL, "PADTHREADSTATE not initialized on this thread");
    return(pts);
}

//+-------------------------------------------------------------------------
//
//  Function:   CreatePadDocThreadProc
//
//  Synopsis:   Creates a new pad window.
//
//
//--------------------------------------------------------------------------

DWORD WINAPI CALLBACK
CreatePadDocThreadProc(void * pv)
{
    CThreadProcParam *ptpp = (CThreadProcParam *)pv;
    CPadDoc *   pDoc = NULL;
    EVENT_HANDLE      hEvent = ptpp->_hEvent;
    HRESULT     hr;
    VARIANT     varScriptParam;
    PADTHREADSTATE * pts = (PADTHREADSTATE *)MemAllocClear(Mt(PADTHREADSTATE), sizeof(PADTHREADSTATE));

    if (pts == NULL)
        goto MemoryError;

    TlsSetValue(g_dwTls, pts);

    ::StartCAP();

    // Emit the PadDoc thread's thread ID so people who are debugging can always
    // know which thread they are looking at - the primary, OLE-non-apartment-model
    // thread, or the 2ndary paddoc thread:
    TraceTag((tagError, "PadDoc 2ndary Thread ID: 0x%x", GetCurrentThreadId() ));

    hr = THR(OleInitialize(NULL));
    if (!OK(hr))
        goto Cleanup;

    pDoc = new CPadDoc(ptpp->_fUseShdocvw);

    if (!pDoc)
        goto MemoryError;

    if (ptpp->_ppStm)
    {
        hr = THR(CoMarshalInterThreadInterfaceInStream(
                IID_IUnknown,
                (IPad *) pDoc,
                ptpp->_ppStm));
        if (hr)
            goto Cleanup;
    }

    hr = pDoc->Init(ptpp->_uShow, g_pEvent);
    if (hr)
        goto Cleanup;

    switch (ptpp->_action)
    {
    case ACTION_SCRIPT:
        {
            BOOL  fKeepRunning        = ptpp->_fKeepRunning;
            TCHAR achParam[MAX_PATH];

            VariantInit(&varScriptParam);

            Assert(_tcslen(ptpp->_pchParam) <= MAX_PATH);

            _tcscpy(achParam, ptpp->_pchParam);

            //
            // Unblock the calling thread before we start executing script code,
            // otherwise we may deadlock. Because of this we must copy all the
            // parameters we plan on using because they'll be destroyed as soon
            // as the calling thread gets CPU time.
            //
            SetEvent(hEvent);
            hEvent = NULL;

            CheckError(pDoc->_hwnd, pDoc->ExecuteScript(achParam, &varScriptParam, FALSE));
            VariantClear(&varScriptParam);

            if (!fKeepRunning)
            {
                pDoc->ShowWindow(SW_HIDE);
            }
        }
        break;

    case ACTION_NEW:
        CheckError(pDoc->_hwnd, pDoc->Open(CLSID_HTMLDocument));
        break;

    case ACTION_OPEN:
        if (ptpp->_fUseShdocvw)
        {
            CheckError(pDoc->_hwnd, pDoc->OpenFile(ptpp->_pchParam, NULL));
        }
        else
        {
            CheckError(pDoc->_hwnd, pDoc->Open(CLSID_HTMLDocument, ptpp->_pchParam));
        }

        break;

    case ACTION_HELP:
        pDoc->OnCommand(0, IDM_PAD_ABOUT, NULL);
        // Fall Through

    case ACTION_WELCOME:
        pDoc->Welcome();
        break;

    }

    // We are done with paramters, allow calling thread to continue.

    SetEvent(hEvent);
    hEvent = NULL;

    pDoc->Release();
    pDoc = NULL;

    CPadDoc::Run();

#if DBG==1
    CheckObjCount();
#endif

Cleanup:
    if (hEvent)
        SetEvent(hEvent);
    if (pDoc)
        pDoc->Release();

    UnregisterLocalCLSIDs();
    OleUninitialize();

    ::StopCAP();

    MemFree(pts);
    TlsSetValue(g_dwTls, NULL);

    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

HRESULT
CreatePadDoc(CThreadProcParam * ptpp, IUnknown ** ppUnk)
{
    HRESULT         hr = S_OK;
    THREAD_HANDLE   hThread = NULL;
    EVENT_HANDLE    hEvent = NULL;
    LPSTREAM        pStm = NULL;
    DWORD           idThread, dwResult;

    hEvent = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (hEvent == NULL)
        RRETURN(GetLastWin32Error());

    if (ppUnk)
    {
        ptpp->_ppStm = &pStm;
    }

    ptpp->_hEvent = hEvent;
    hThread = CreateThread(NULL, 0, CreatePadDocThreadProc, ptpp, 0, &idThread);
    if (hThread == NULL)
    {
        hr = GetLastWin32Error();
        goto Cleanup;
    }

    s_aryThreads.AppendIndirect(&hThread);

    ::SuspendCAP();

    dwResult = WaitForSingleObject(hEvent, INFINITE);
    Assert(dwResult == WAIT_OBJECT_0);

    ::ResumeCAP();

    if (pStm)
    {
        hr = THR(CoGetInterfaceAndReleaseStream(pStm, IID_IUnknown, (void **) ppUnk));
    }

Cleanup:
    ptpp->_hEvent = NULL;
    CloseEvent(hEvent);
    RRETURN(hr);
}

LRESULT
WndProcMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
   case WM_DESTROY:
       PostQuitMessage(0);
       break;

   default:
       return DefWindowProc(hwnd, msg, wParam, lParam);
   }

   return 0;
}

PADTHREADSTATE stateMain = { 0 };

static HRESULT
Initialize()
{
    HRESULT             hr = S_OK;
    WNDCLASS            wc;

    g_dwTls = TlsAlloc();
    if (g_dwTls == 0xFFFFFFFF)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    TlsSetValue(g_dwTls, &stateMain);

    EnableTag(tagScriptLog, TRUE);
    SetDiskFlag(tagScriptLog, TRUE);
    SetBreakFlag(tagScriptLog, FALSE);

    QueryPerformanceFrequency((LARGE_INTEGER *)&s_liPerfFreq);
    QueryPerformanceCounter((LARGE_INTEGER *)&s_liTimeStart);

    CGlobalLock::Init();

    g_hProcessHeap = GetProcessHeap();

    InitUnicodeWrappers();

    // Create "main" window.  This window is used to keep Windows
    // and OLE happy while we are waiting for something to happen
    // when launched to handle an embedding.

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProcMain;   // windows of this class.
    wc.hInstance = g_hInstCore;
    wc.hIcon = LoadIcon(g_hInstResource, MAKEINTRESOURCE(IDR_PADICON));
    wc.lpszClassName = SZ_APPLICATION_NAME TEXT(" Main");
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
    g_atomMain = RegisterClass(&wc);

    if (!g_atomMain)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    g_hwndMain = CreateWindow(
            SZ_APPLICATION_NAME TEXT(" Main"),
            NULL, WS_OVERLAPPEDWINDOW,
            0, 0, 0, 0, NULL, NULL, g_hInstCore, NULL);
    if (!g_hwndMain)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = THR(OleInitialize(NULL));
    if (hr)
        goto Cleanup;

#if 0
    // Force allocation of memory to hide leaks in OLEAUT32.
    DbgMemoryTrackDisable(TRUE);
    SysFreeString(SysAllocString(_T("Force Allocation Of Memory")));
    DbgMemoryTrackDisable(FALSE);
#endif

    CreatePerfCtl(0, NULL);

Cleanup:
    RRETURN(hr);
}

static DYNLIB g_dynlibWininet = { NULL, NULL, "wininet.dll" };
static DYNPROC g_dynprocUrlZonesDetach = { NULL, &g_dynlibWininet, "UrlZonesDetach" };

static void
Terminate()
{
    extern void DeinitPalette();
    DeinitPalette();

    if (g_hwndMain)
    {
        Assert(IsWindow(g_hwndMain));
        Verify(DestroyWindow(g_hwndMain));
    }

    if (g_atomMain)
    {
        Verify(UnregisterClass((TCHAR *)(DWORD_PTR)g_atomMain, g_hInstCore));
    }

    // Hack to break LoadLibrary cycle between WININET and URLMON

    if (    !IsTagEnabled(tagPadNoCycleBreak)
        &&  GetModuleHandleA("wininet.dll")
        &&  GetModuleHandleA("urlmon.dll")
        &&  LoadProcedure(&g_dynprocUrlZonesDetach) == S_OK)
    {
        ((void (STDAPICALLTYPE *)())g_dynprocUrlZonesDetach.pfn)();
    }

#if DBG==1
    if (IsTagEnabled(tagDumpHeapsOnExit))
    {
        DbgExDumpProcessHeaps();
        DeleteFileA("c:\\heapdump.x1");
        MoveFileA("c:\\heapdump.txt", "\\heapdump.x1");
    }
#endif

    CPadFactory::Revoke();

    DumpTimes();

    OleUninitialize();

#if DBG==1
    if (IsTagEnabled(tagDumpHeapsOnExit))
    {
        DbgExDumpProcessHeaps();
        DeleteFileA("c:\\heapdump.x2");
        MoveFileA("c:\\heapdump.txt", "\\heapdump.x2");
    }
#endif

#ifdef WHEN_CONTROL_PALETTE_IS_SUPPORTED
    DeinitControlPalette();
#endif // WHEN_CONTROL_PALETTE_IS_SUPPORTED

    DeinitDynamicLibraries();

#if DBG==1
    if (IsTagEnabled(tagDumpHeapsOnExit))
    {
        DbgExDumpProcessHeaps();
        DeleteFileA("c:\\heapdump.x3");
        MoveFileA("c:\\heapdump.txt", "\\heapdump.x3");
    }
#endif

    CGlobalLock::Deinit();

    DeletePerfCtl();

    TlsFree(g_dwTls);
}


HRESULT
CheckError(HWND hwnd, HRESULT hr)
{
    TCHAR achBuf[MAX_PATH];

    if (OK(hr))
        return hr;

    if (!FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        hr,
        LANG_SYSTEM_DEFAULT,
        achBuf,
        ARRAY_SIZE(achBuf),
        NULL))
    {
        _tcscpy(achBuf, TEXT("Unknown error."));
    }

    MessageBox(hwnd,
            achBuf,
            NULL,
            MB_APPLMODAL | MB_ICONERROR | MB_OK);

    return hr;
}


void
Run ()
{
    for (;;)
    {
        DWORD   result ;
                int     cObjects = s_aryThreads.Size();
        THREAD_HANDLE * pHandle = s_aryThreads;

        // wait for any message sent or posted to this queue
        // or for one of the passed handles to become signaled

        ::SuspendCAP();

        result = MsgWaitForMultipleObjects(
                        cObjects,
                        pHandle,
                        FALSE,
                        INFINITE,
                        QS_ALLINPUT);

        ::ResumeCAP();

        // result tells us the type of event we have:
        // a message or a signaled handle

        // if there are one or more messages in the queue ...
        if (result == (WAIT_OBJECT_0 + cObjects))
        {
            MSG    msg;
            BOOL   fQuit;

            // read all of the messages in this next loop
            // removing each message as we read it

            for (;;)
            {
                ::SuspendCAP();

#ifndef UNIX
                Assert(!InSendMessage());
#endif
                fQuit = !PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

                ::ResumeCAP();

                if (fQuit)
                    break;

#ifdef UNIX
                if (msg.hwnd != GetDesktopWindow()) {
                    CPadDoc::RunOneMessage(&msg);
                    continue;
                }

                Assert(!InSendMessage());
#endif

                if (msg.message == WM_QUIT)
                    return;

                CPadDoc::RunOneMessage(&msg);
            }
        }
        else
        {
            int i = result - WAIT_OBJECT_0;

            CloseThread(s_aryThreads[i]);
            s_aryThreads.Delete(i);
            if (s_aryThreads.Size() == 0)
                return;
        }
    }
}
#ifdef UNIX
extern "C"
#endif
int WINAPI
PadMain(int argc, char ** argv, IMallocSpy * pSpy)
{
    HRESULT     hr = S_OK;
    char *      pchParam = NULL;
    TCHAR       achParam[MAX_PATH];
    int         nRet = 0;
    int         i;
    BOOL        fKeepRunning = FALSE;
    PAD_ACTION  action = ACTION_WELCOME;
    char *      pLogFileName = NULL;
    BOOL        fOpenLogFile = FALSE;
    BOOL        fDoTrace = FALSE;
    BOOL        fDialogs = TRUE;
    BOOL        fUseShdocvw = FALSE;

    ::StopCAPAll();

    hr = Initialize();
    if (hr)
        goto Cleanup;

    for (i = 1; i < argc; i++)
    {
        if (lstrcmpiA(argv[i], "/embedding") == 0 ||
            lstrcmpiA(argv[i], "-embedding") == 0)
        {
            action = ACTION_SERVER;
            fOpenLogFile = TRUE;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/r") == 0 ||
                 lstrcmpiA(argv[i], "-r") == 0 ||
                 lstrcmpiA(argv[i], "/register") == 0 ||
                 lstrcmpiA(argv[i], "-register") == 0))
        {
            action = ACTION_REGISTER_PAD;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/local") == 0 ||
                 lstrcmpiA(argv[i], "-local") == 0))
        {
            action = ACTION_REGISTER_LOCAL_TRIDENT;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/system") == 0 ||
                 lstrcmpiA(argv[i], "-system") == 0))
        {
            action = ACTION_REGISTER_SYSTEM_TRIDENT;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/m") == 0 ||
                 lstrcmpiA(argv[i], "-m") == 0 ||
                 lstrcmpiA(argv[i], "/mail") == 0 ||
                 lstrcmpiA(argv[i], "-mail") == 0))
        {
            action = ACTION_REGISTER_MAIL;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/nomail") == 0 ||
                 lstrcmpiA(argv[i], "-nomail") == 0))
        {
            action = ACTION_UNREGISTER_MAIL;
        }
        else if (argc >= 2 &&
                (lstrcmpiA(argv[i], "/nuke") == 0 ||
                 lstrcmpiA(argv[i], "-nuke") == 0))
        {
            action = ACTION_NUKE_KNOWNDLLS;
        }
        else if (lstrcmpiA(argv[i], "-n") == 0 ||
                lstrcmpiA(argv[i], "/n") == 0)
        {
            action = ACTION_NEW;
        }
        else if (lstrcmpiA(argv[i], "-k") == 0 ||
                lstrcmpiA(argv[i], "/k") == 0)
        {
            fKeepRunning = TRUE;
        }
        else if (lstrcmpiA(argv[i], "-s") == 0 ||
                lstrcmpiA(argv[i], "/s") == 0)
        {
            fDialogs = FALSE;
        }
        else if (lstrcmpiA(argv[i], "-t") == 0 ||
                lstrcmpiA(argv[i], "/t") == 0)
        {
            g_pEvent = &g_PadEvent;  // to log event times
        }
#ifdef UNIX_LATER
        else if (lstrcmpiA(argv[i], "-d") == 0 ||
                 lstrcmpiA(argv[i], "/d") == 0)
        {
            g_pEvent = new CDispatchEvent(); // to do dhtml via IDispatch
        }
#endif
        else if (lstrcmpiA(argv[i], "-l") == 0 ||
                lstrcmpiA(argv[i], "/l") == 0)
        {
            i++;
            fOpenLogFile = TRUE;
            if(i < argc)
            {
                pLogFileName = argv[i];
            }
        }
        else if (lstrcmpiA(argv[i], "-x") == 0 ||
                lstrcmpiA(argv[i], "/x") == 0)
        {
            if (i + 1 < argc)
            {
                pchParam = argv[++i];
                action = ACTION_SCRIPT;
            }
        }
        else if (argc > 2 &&
                (lstrcmpiA(argv[i], "/trace") == 0 ||
                 lstrcmpiA(argv[i], "-trace") == 0))
        {
            fDoTrace = TRUE;
        }
        else if (lstrcmpiA(argv[i], "-?") == 0 ||
                lstrcmpiA(argv[i], "/?") == 0)
        {
            action = ACTION_HELP;
        }
        else if (lstrcmpiA(argv[i], "-shdocvw") == 0 ||
                lstrcmpiA(argv[i], "/shdocvw") == 0)
        {
            fUseShdocvw = TRUE;
        }
        else if (lstrcmpiA(argv[i], "-loadsystem") == 0 ||
                lstrcmpiA(argv[i], "/loadsystem") == 0)
        {
            g_fLoadSystemMSHTML = TRUE;
        }
        else if (lstrcmpiA(argv[i], "-1") == 0 ||
                lstrcmpiA(argv[i], "/1") == 0)
        {
            BOOL (WINAPI *pfn)(HANDLE, DWORD);
            pfn = (BOOL (WINAPI *)(HANDLE, DWORD))GetProcAddress(GetModuleHandleA("KERNEL32.DLL"), "SetProcessAffinityMask");
            if (pfn)
            {
                pfn(GetCurrentProcess(), 1);
            }
        }
        else
        {
            pchParam = argv[i];
            action = ACTION_OPEN;
        }
    }

    if (fDoTrace)
    {
        hr = THR(RegisterLocalCLSIDs());
        if (hr == S_OK)
        {
            DbgExDoTracePointsDialog(FALSE);
        }
    }

    if((pLogFileName != NULL && *pLogFileName != 0) || fOpenLogFile)
    {
        DbgExOpenLogFile(pLogFileName);
    }


    if (pchParam)
    {
        MultiByteToWideChar(CP_ACP, 0, pchParam, -1, achParam, ARRAY_SIZE(achParam));
    }

    if (pSpy)
    {
        CoRegisterMallocSpy(pSpy);
    }

    // Emit the main thread's thread ID so people who are debugging can always
    // know which thread they are looking at - the primary, OLE-non-apartment-model
    // thread, or the 2ndary paddoc thread:
    TraceTag((tagError, "Main Thread ID: 0x%x", GetCurrentThreadId() ));

    switch (action)
    {
    case ACTION_SERVER:
        hr = THR(CPadFactory::Register());
        break;

    case ACTION_REGISTER_PAD:
        hr = THR(RegisterPad());
        goto Cleanup;

    case ACTION_REGISTER_LOCAL_TRIDENT:
        hr = THR(RegisterTrident(NULL, fDialogs, FALSE));
        goto Cleanup;

    case ACTION_REGISTER_SYSTEM_TRIDENT:
        hr = THR(RegisterTrident(NULL, fDialogs, TRUE));
        goto Cleanup;

    case ACTION_NUKE_KNOWNDLLS:
        hr = THR(NukeKnownDLLStuff());
        goto Cleanup;

    default:
        {
            CThreadProcParam tpp(fUseShdocvw, action, fKeepRunning, achParam);
            if (CheckError(NULL, CreatePadDoc(&tpp, NULL)) != S_OK)
                goto Cleanup;
        }
        break;
    }

    Run();

Cleanup:

    // Flush the message queue now

    {
        MSG msg;
        for (int n = 0; n < 1000; ++n)
            PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
    }

    Terminate();
    return nRet;
}

#ifdef UNIX
extern "C"
#endif
BOOL
WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fOk = TRUE;

    AssertThreadDisable(TRUE);

    g_hInstCore     = (HINSTANCE)hDll;
    g_hInstResource = (HINSTANCE)hDll;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls((HINSTANCE)hDll);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }

    AssertThreadDisable(FALSE);

    return fOk;
}
