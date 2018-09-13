/////////////////////////////////////////////////////////////////////////////
// ACTSAVER.CPP
//
// Implementation of WinMain and supporting functions
//
// History:
//
// Author   Date        Description
// ------   ----        -----------
// jaym     08/26/96    Created
// jaym     02/01/97    Added change password
/////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "initguid.h"
#include "if\actsaver.h"
#include "saver.h"
#include "resource.h"

#define IID_DEFINED
#include "actsaver_i.c"

DECLSPEC_SELECTANY const CLSID CLSID_CURLFolder = {0x3DC7A020, 0x0ACD, 0x11CF, {0xA9, 0xBB, 0x00, 0xAA, 0x00, 0x4A, 0xE8, 0x37}};
DECLSPEC_SELECTANY const CLSID CLSID_ShellInetRoot = {0x871C5380, 0x42A0, 0x1069, {0xA2, 0xEA, 0x08, 0x00, 0x2B, 0x30, 0x30,0x9D}};

#define TF_LIFETIME     TF_ALWAYS
#define TF_IME          TF_ALWAYS

/////////////////////////////////////////////////////////////////////////////
// Module variables
/////////////////////////////////////////////////////////////////////////////
#pragma data_seg(DATASEG_READONLY)
static const TCHAR  s_szMutexName[]     = TEXT("ActiveSSMutex");

static const TCHAR  s_szMprDll[]        = TEXT("MPR.DLL");
static const TCHAR  s_szProviderName[]  = TEXT("SCRSAVE");
static const TCHAR  s_szPwdChangePW[]   = TEXT("PwdChangePasswordA");
    // Password

static const TCHAR  s_szImmDLL[]        = TEXT("IMM32.DLL");
static const TCHAR  s_szImmFnc[]        = TEXT("ImmAssociateContext");
    // IME
#pragma data_seg()

/////////////////////////////////////////////////////////////////////////////
// Global variables
/////////////////////////////////////////////////////////////////////////////
#pragma data_seg(DATASEG_READONLY)
TCHAR g_szRegSubKey[] = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\IE4 Screen Saver");
#pragma data_seg()

BOOL            g_bPlatformNT;
    // Are we running on NT?

HANDLE          g_hHeap = NULL;
IMalloc *       g_pMalloc = NULL;
    // Heap for malloc/free and new/delete

HINSTANCE       g_hIMM = NULL;
IMMASSOCPROC    g_pfnIMMProc = NULL;
    // IME

#ifdef _DEBUG
ULONG           g_cHeapAllocsOutstanding = 0;
#endif  // _DEBUG

CExeModule *    _pModule;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CActiveScreenSaver, CActiveScreenSaver)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Helper functions/macros
/////////////////////////////////////////////////////////////////////////////

BOOL IsScreenSaverRunning();
BOOL RunScreenSaver(HWND hwndParent, long lMode, LPTSTR pszSingleURL);
BOOL RunConfiguration(HWND hwndParent);
void RunPasswordChange(HWND hwndParent);
void InitIME();

#define StartScreenSaverRunning()       CreateMutex(NULL, FALSE, s_szMutexName)
#define EndScreenSaverRunning(hMutex)   CloseHandle((hMutex))

/////////////////////////////////////////////////////////////////////////////
// CExeModule::Unlock
/////////////////////////////////////////////////////////////////////////////
LONG CExeModule::Unlock
(
)
{
    LONG l;

    if ((l = CComModule::Unlock()) == 0)
        PostThreadMessage(dwThreadID, WM_QUIT, 0, 0);

    return l;
}

/////////////////////////////////////////////////////////////////////////////
// ModuleEntry
/////////////////////////////////////////////////////////////////////////////
int __cdecl main() { ASSERT(FALSE); return 0; }

extern "C" int _stdcall ModuleEntry
(
    void
)
{
    // Determine if we're running on NT or Win95
    OSVERSIONINFO vi;
    vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&vi);
    g_bPlatformNT = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT);

    LPTSTR pszCmdLine = GetCommandLine();

    if (*pszCmdLine == TEXT('\"'))
    {
        // Scan, and skip over, subsequent characters until
        // another double-quote or a null is encountered.
        while (*++pszCmdLine && (*pszCmdLine != TEXT('\"')));
        // If we stopped on a double-quote (usual case), skip
        // over it.
        if (*pszCmdLine == TEXT('\"'))
            pszCmdLine++;
    }
    else
    {
        while (*pszCmdLine != TEXT(' '))
            pszCmdLine++;
    }

    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
        pszCmdLine++;

    STARTUPINFO si = { 0 };
    GetStartupInfoA(&si);

    int i = WinMain(GetModuleHandle(NULL),
                    NULL,
                    pszCmdLine,
                    ((si.dwFlags & STARTF_USESHOWWINDOW)
                        ? si.wShowWindow
                        : SW_SHOWDEFAULT));

    ExitProcess(i);

    TraceMsg(TF_LIFETIME, "Main thread exiting without ExitProcess!");

    return i;
}

/////////////////////////////////////////////////////////////////////////////
// VersionCheck
/////////////////////////////////////////////////////////////////////////////
#define FILE_TO_CHECK_VER TEXT("shdocvw.dll")
#define VALID_SHDOCVW_VER 0x0005000007de00d5    // IE5.0 1402.16

BOOL VersionCheck(VOID)
{
    BOOL    fRC = FALSE;
    LPSTR   lpFileInfo; 
    LPSTR   lpstrVffInfo = NULL;
    UINT    uiLen; 
    DWORD   dwVerInfoSize;     // Size of version information block
    DWORD   dwVerHnd;
    TCHAR   szBuf[256];

    if((dwVerInfoSize = GetFileVersionInfoSize(FILE_TO_CHECK_VER, &dwVerHnd)) != 0)
    {
        if((lpstrVffInfo = (LPSTR)LocalAlloc(LPTR, dwVerInfoSize)) != NULL)
        {
            if(GetFileVersionInfo(FILE_TO_CHECK_VER, dwVerHnd, dwVerInfoSize, lpstrVffInfo))
            {
                if(VerQueryValue(lpstrVffInfo, TEXT("\\"), (LPVOID *)&lpFileInfo, &uiLen))
                {
                    LARGE_INTEGER uiFileVersion64;
                    uiFileVersion64.HighPart = ((VS_FIXEDFILEINFO *)lpFileInfo)->dwFileVersionMS;
                    uiFileVersion64.LowPart =  ((VS_FIXEDFILEINFO *)lpFileInfo)->dwFileVersionLS;
                    if(uiFileVersion64.QuadPart >= VALID_SHDOCVW_VER)
                    {
                        fRC = TRUE;
                    }
                    else
                    {
                        wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("Screen Saver version mismatch with %s"), FILE_TO_CHECK_VER);
                        TraceMsg(TF_ERROR, szBuf);
                    }
                }
            }
            
            LocalFree((LPSTR)lpstrVffInfo);
        }
        else
        {
            wnsprintf(szBuf, ARRAYSIZE(szBuf), TEXT("Screen Saver alloc failed in VersionCheck. GLE=%d"), GetLastError());
            TraceMsg(TF_ERROR, szBuf);
        }
    }

    return(fRC);
}


/////////////////////////////////////////////////////////////////////////////
// WinMain
/////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain
(
    HINSTANCE   hInstance, 
    HINSTANCE   hPrevInstance,
    LPTSTR      lpCmdLine,
    int         nShowCmd
)
{
    BOOL    bDone = FALSE;
    HWND    hwndParent = NULL;

    if(!VersionCheck())
        return 1;
        
    // Initialize OLE COM
    EVAL(SUCCEEDED(::CoInitialize(NULL)));
    EVAL(SUCCEEDED(SHGetMalloc(&g_pMalloc)));

    ASSERT(g_pMalloc);

    if (NULL == g_pMalloc)
        return 1;

    _pModule = new CExeModule;

    if (NULL == _pModule)
    {
        g_pMalloc->Release();
        return 1;
    }

    // Initialize the module.
    _pModule->Init(ObjectMap, hInstance);
    _pModule->dwThreadID = GetCurrentThreadId();

    // Register the server.
//    EVAL(SUCCEEDED(_pModule->RegisterServer(TRUE)));

    InitIME();

    for (;;)
    {
        switch (*lpCmdLine)
        {
            case TEXT('P'): // Preview Mode
            case TEXT('p'):
            {
                // Skip to the empty space
                do lpCmdLine++; while(*lpCmdLine == TEXT(' '));
                
                if  (
#ifdef _DEBUG
                    (
#endif  // _DEBUG
                        ((hwndParent = (HWND)myatoi(lpCmdLine)) != NULL)
#ifdef _DEBUG
                        ||
                        
                        ((hwndParent = GetDesktopWindow()) != NULL)
                    )
#endif  // _DEBUG
                    &&
                    IsWindow(hwndParent)
                    )
                {
                    EVAL(RunScreenSaver(hwndParent, SSMODE_PREVIEW, NULL));
                }

                bDone = TRUE;
                break;
            }

            case TEXT('S'): // Screen Saver Mode
            case TEXT('s'):
            {
                // Make sure we don't run in Screen Saver mode more than once.
                if (!IsScreenSaverRunning())
                    EVAL(RunScreenSaver(NULL, SSMODE_NORMAL, NULL));
                else
                    TraceMsg(TF_ALWAYS, "Screen Saver already running in Screen Saver mode.");

                bDone = TRUE;
                break;
            }

            case TEXT('C'): // Configuration Mode
            case TEXT('c'):
            {
                do
                {
                    lpCmdLine++;
                }
                while((*lpCmdLine == TEXT(' ')) || (*lpCmdLine == TEXT(':')));
                
                HWND hwndParent = (HWND)myatoi(lpCmdLine);

                if (NULL == hwndParent)
                {
                    hwndParent = GetForegroundWindow();
                }
                EVAL(RunConfiguration(hwndParent));

                bDone = TRUE;
                break;
            }

            case TEXT('A'): // Change Password Mode
            case TEXT('a'):
            {
                // Skip to the empty space
                do lpCmdLine++; while (*lpCmdLine == TEXT(' '));

                if  (
                    ((hwndParent = (HWND)myatoi(lpCmdLine)) == NULL)
                    ||
                    !IsWindow(hwndParent)
                    )
                {
                    hwndParent = GetForegroundWindow();
                }

                RunPasswordChange(hwndParent);

                bDone = TRUE;
                break;
            }

            case TEXT('U'): // Single URL Mode
            case TEXT('u'):
            {
                // Skip to the empty space
                do lpCmdLine++; while (*lpCmdLine == TEXT(' '));

                if (!IsScreenSaverRunning())
                    EVAL(RunScreenSaver(NULL, SSMODE_SINGLEURL, lpCmdLine));
                else
                    TraceMsg(TF_ALWAYS, "Screen Saver already running in Screen Saver mode.");

                bDone = TRUE;
                break;
            }

            case TEXT('\0'):
            {
                EVAL(RunConfiguration(NULL));

                bDone = TRUE;
                break;
            }

            case TEXT(' '):
            case TEXT('-'):
            case TEXT('/'):
            {
                lpCmdLine++;
                break;
            }

            default:
            {
                bDone = TRUE;
                break;
            }
        }

        if (bDone)
            break;
    }

//    _pModule->UnregisterServer();

    _pModule->Term();
    delete _pModule;

    TraceMsg(TF_LIFETIME, "Screen saver leaving memory");

    if (g_hHeap != NULL)
    {
#ifdef _DEBUG
        TraceMsg(TF_ALWAYS, "MEM: %d heap alloctions remaining", g_cHeapAllocsOutstanding);
#endif  // _DEBUG

        EVAL(HeapDestroy(g_hHeap));
        g_hHeap = NULL;
    }

    // NOTE! NO TraceMsg AFTER HEAP IS DESTROYED!

    if (g_pMalloc != NULL)
        g_pMalloc->Release();

    ::CoUninitialize();

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// IsScreenSaverRunning
/////////////////////////////////////////////////////////////////////////////
BOOL IsScreenSaverRunning
(
)
{
    HANDLE  hMutex;
    BOOL    bRunning;

    // Create a named mutex to determine if the screen
    // saver is already running in screen saver mode.
    bRunning =  (
                ((hMutex = CreateMutex(NULL, FALSE, s_szMutexName)) != NULL)
                &&
                (GetLastError() == ERROR_ALREADY_EXISTS)
                );
    
    if (hMutex != NULL)
        EVAL(CloseHandle(hMutex));

    return bRunning;
}

/////////////////////////////////////////////////////////////////////////////
// RunScreenSaver
/////////////////////////////////////////////////////////////////////////////
BOOL RunScreenSaver
(
    HWND    hwndParent,
    long    lMode,
    LPTSTR  pszSingleURL
)
{
    HRESULT hrResult;

    ASSERT(_pModule != NULL);

    if (FAILED(hrResult = _pModule->RegisterClassObjects(   CLSCTX_LOCAL_SERVER,
                                                            REGCLS_MULTIPLEUSE)))
    {
        return hrResult;
    }

    for (;;)
    {
        IScreenSaver * pScreenSaver;
        if (FAILED(hrResult = CoCreateInstance( CLSID_CActiveScreenSaver,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IScreenSaver,
                                                (void **)&pScreenSaver)))
        {
            break;
        }

        BOOL    bDummy;
        HANDLE  hRunMutex;

        if (lMode == SSMODE_NORMAL)
        {
            hRunMutex = StartScreenSaverRunning();

            // Make sure the system knows we are running.
            SystemParametersInfo(SPI_SCREENSAVERRUNNING, TRUE, &bDummy, 0);
        }
        else if (lMode == SSMODE_SINGLEURL)
        {
            ASSERT(pszSingleURL != NULL);

            BSTR bstrURL = TCharSysAllocString(pszSingleURL);

            if (bstrURL)
            {
                pScreenSaver->put_CurrentURL(bstrURL);
                SysFreeString(bstrURL);
            }
        }

        pScreenSaver->put_Mode(lMode);
        pScreenSaver->Run(hwndParent);
        EVAL(pScreenSaver->Release() == 0);

        if (lMode == SSMODE_NORMAL)
        {
            ASSERT(hRunMutex != NULL);

            // #61159, important!! keep the sequence of the following calls.
            // Make sure the system knows we are not running.
            SystemParametersInfo(SPI_SCREENSAVERRUNNING, FALSE, &bDummy, 0);

            EndScreenSaverRunning(hRunMutex);
        }

        hrResult = S_OK;
        break;
    }

    _pModule->RevokeClassObjects();

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// RunConfiguration
/////////////////////////////////////////////////////////////////////////////
BOOL RunConfiguration
(
    HWND hwndParent
)
{
    HRESULT hrResult;

    ASSERT(_pModule != NULL);

    if (FAILED(hrResult = _pModule->RegisterClassObjects(   CLSCTX_LOCAL_SERVER,
                                                            REGCLS_MULTIPLEUSE)))
    {
        return hrResult;
    }

    for (;;)
    {
        IScreenSaverConfig * pConfig;
        if (FAILED(hrResult = CoCreateInstance( CLSID_CActiveScreenSaver,
                                                NULL,
                                                CLSCTX_INPROC_SERVER,
                                                IID_IScreenSaverConfig,
                                                (void **)&pConfig)))
        {
            break;
        }

        pConfig->ShowDialog(hwndParent);
        EVAL(pConfig->Release() == 0);

        TraceMsg(TF_LIFETIME, "Leaving configuration mode!");

        break;
    }

    _pModule->RevokeClassObjects();

    return SUCCEEDED(hrResult);
}

/////////////////////////////////////////////////////////////////////////////
// RunPasswordChange
/////////////////////////////////////////////////////////////////////////////
typedef DWORD (FAR PASCAL *PWCHGPROC)(LPCSTR, HWND, DWORD, LPVOID);

void RunPasswordChange
(
    HWND hwndParent
)
{
    HINSTANCE mpr;

    if ((mpr = LoadLibrary(s_szMprDll)) != NULL)
    {
        PWCHGPROC pwd;

        if ((pwd = (PWCHGPROC)GetProcAddress(mpr, s_szPwdChangePW)) != NULL)
            pwd(s_szProviderName, hwndParent, 0, NULL);

        EVAL(FreeLibrary(mpr));
    }
}

/////////////////////////////////////////////////////////////////////////////
// InitIME
/////////////////////////////////////////////////////////////////////////////
void InitIME
(
)
{
    // Load the IME DLL so we can disable, if needed.
    if ((g_hIMM = GetModuleHandle(s_szImmDLL)) != NULL)
    {
        g_pfnIMMProc = (IMMASSOCPROC)GetProcAddress(g_hIMM, s_szImmFnc);

        TraceMsg(TF_IME, "Loaded IMM DLL 0x%.8X, GetProcAddress = 0x%.8X", g_hIMM, g_pfnIMMProc);
    }
}

