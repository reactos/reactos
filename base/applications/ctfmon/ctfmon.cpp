/*
 * PROJECT:     ReactOS CTF Monitor
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Providing Language Bar front-end
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "ctfmon.h"
#include "CRegWatcher.h"
#include "CTipBarWnd.h"
#include "CModulePath.h"

// ntdll!NtQueryInformationProcess
typedef NTSTATUS (WINAPI *FN_NtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
FN_NtQueryInformationProcess g_fnNtQueryInformationProcess = NULL;

// kernel32!SetProcessShutdownParameters
typedef BOOL (WINAPI *FN_SetProcessShutdownParameters)(DWORD, DWORD);
FN_SetProcessShutdownParameters g_fnSetProcessShutdownParameters = NULL;

// kernel32!GetSystemWow64DirectoryW
typedef UINT (WINAPI *FN_GetSystemWow64DirectoryW)(LPWSTR, UINT);
FN_GetSystemWow64DirectoryW g_fnGetSystemWow64DirectoryW = NULL;

HINSTANCE   g_hInst         = NULL;     // The application instance
HINSTANCE   g_hKernel32     = NULL;     // The "kernel32.dll" instance
UINT        g_uACP          = CP_ACP;   // The active codepage
BOOL        g_fWinLogon     = FALSE;    // Is it a log-on process?
HANDLE      g_hCicMutex     = NULL;     // The Cicero mutex
BOOL        g_bOnWow64      = FALSE;    // Is the app running on WoW64?
BOOL        g_fNoRunKey     = FALSE;    // Don't write registry key "Run"?
BOOL        g_fJustRunKey   = FALSE;    // Just write registry key "Run"?
DWORD       g_dwOsInfo      = 0;        // The OS version info. See GetOSInfo below
CTipBarWnd* g_pTipBarWnd    = NULL;     // TIP Bar window

BOOL
CModulePath::Init(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    SIZE_T cchPath;
    if (bSysWinDir)
    {
        // Usually C:\Windows or C:\ReactOS
        cchPath = ::GetSystemWindowsDirectory(m_szPath, _countof(m_szPath));
    }
    else
    {
        // Usually C:\Windows\system32 or C:\ReactOS\system32
        cchPath = ::GetSystemDirectoryW(m_szPath, _countof(m_szPath));
    }

    m_szPath[_countof(m_szPath) - 1] = UNICODE_NULL; // Avoid buffer overrun

    if ((cchPath == 0) || (cchPath > _countof(m_szPath) - 2))
        goto Failure;

    // Add backslash if necessary
    if ((cchPath > 0) && m_szPath[cchPath - 1] != L'\\')
    {
        m_szPath[cchPath + 0] = L'\\';
        m_szPath[cchPath + 1] = UNICODE_NULL;
    }

    // Append pszFileName
    if (FAILED(StringCchCatW(m_szPath, _countof(m_szPath), pszFileName)))
        goto Failure;

    m_cchPath = wcslen(m_szPath);
    return TRUE;

Failure:
    m_szPath[0] = UNICODE_NULL;
    m_cchPath = 0;
    return FALSE;
}

// Get an instance handle that is already loaded
HINSTANCE
GetSystemModuleHandle(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return GetModuleHandleW(ModPath.m_szPath);
}

// Load a system library
HINSTANCE
LoadSystemLibrary(
    _In_ LPCWSTR pszFileName,
    _In_ BOOL bSysWinDir)
{
    CModulePath ModPath;
    if (!ModPath.Init(pszFileName, bSysWinDir))
        return NULL;
    return ::LoadLibraryW(ModPath.m_szPath);
}

// Is the system on WoW64?
static BOOL
IsWow64(VOID)
{
    HMODULE hNTDLL;
    ULONG_PTR Value;
    NTSTATUS Status;

    hNTDLL = GetSystemModuleHandle(L"ntdll.dll", FALSE);
    if (!hNTDLL)
        return FALSE;

    g_fnNtQueryInformationProcess =
        (FN_NtQueryInformationProcess)::GetProcAddress(hNTDLL, "NtQueryInformationProcess");
    if (!g_fnNtQueryInformationProcess)
        return FALSE;

    Status = g_fnNtQueryInformationProcess(GetCurrentProcess(), ProcessWow64Information,
                                           &Value, sizeof(Value), 0);
    if (!NT_SUCCESS(Status))
        return FALSE;

    return !!Value;
}

static VOID
ParseCommandLine(
    _In_ LPCWSTR pszCmdLine)
{
    g_fNoRunKey = g_fJustRunKey = FALSE;

    for (LPCWSTR pch = pszCmdLine; *pch; ++pch)
    {
        // Skip space
        while (*pch == L' ')
            ++pch;

        if (*pch == UNICODE_NULL)
            return;

        if ((*pch == L'-') || (*pch == L'/'))
        {
            ++pch;
            switch (*pch)
            {
                case L'N': case L'n': // Found "/N" option
                    g_fNoRunKey = TRUE;
                    break;

                case L'R': case L'r': // Found "/R" option
                    g_fJustRunKey = TRUE;
                    break;

                case UNICODE_NULL:
                    return;

                default:
                    break;
            }
        }
    }
}

static VOID
WriteRegRun(VOID)
{
    if (g_fNoRunKey) // If "/N" option is specified
        return; // Don't add

    // Open 'Run' key
    HKEY hKey;
    LSTATUS error = ::RegCreateKeyW(HKEY_CURRENT_USER,
                                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                                    &hKey);
    if (error != ERROR_SUCCESS)
        return;

    // Write the module path
    CModulePath ModPath;
    if (ModPath.Init(L"ctfmon.exe", FALSE))
    {
        DWORD cbData = (ModPath.m_cchPath + 1) * sizeof(WCHAR);
        ::RegSetValueExW(hKey, L"ctfmon.exe", 0, REG_SZ, (BYTE*)ModPath.m_szPath, cbData);
    }

    ::RegCloseKey(hKey);
}

static VOID
GetOSInfo(VOID)
{
    g_dwOsInfo = 0; // Clear all flags

    // Check OS version info
    OSVERSIONINFOW VerInfo = { sizeof(VerInfo) };
    ::GetVersionExW(&VerInfo);
    if (VerInfo.dwPlatformId == DLLVER_PLATFORM_NT)
        g_dwOsInfo |= OSINFO_NT;

    // Check codepage
    g_uACP = ::GetACP();
    switch (g_uACP)
    {
        case 932: // Japanese (Japan)
        case 936: // Chinese (PRC, Singapore)
        case 949: // Korean (Korea)
        case 950: // Chinese (Taiwan, Hong Kong)
            g_dwOsInfo |= OSINFO_CJK;
            break;
        default:
            break;
    }

    if (::GetSystemMetrics(SM_IMMENABLED))
        g_dwOsInfo |= OSINFO_IMM;

    if (::GetSystemMetrics(SM_DBCSENABLED))
        g_dwOsInfo |= OSINFO_DBCS;

    // I'm not interested in other flags
}

static HRESULT
GetGlobalCompartment(
    _In_ REFGUID guid,
    _Inout_ ITfCompartment **ppComp)
{
    *ppComp = NULL;

    ITfCompartmentMgr *pCompMgr = NULL;
    HRESULT hr = TF_GetGlobalCompartment(&pCompMgr);
    if (FAILED(hr))
        return hr;

    if (!pCompMgr)
        return E_FAIL;

    hr = pCompMgr->GetCompartment(guid, ppComp);
    pCompMgr->Release();
    return hr;
}

static HRESULT
SetGlobalCompartmentDWORD(
    _In_ REFGUID guid,
    _In_ DWORD dwValue)
{
    HRESULT hr;
    VARIANT vari;
    ITfCompartment *pComp;

    hr = GetGlobalCompartment(guid, &pComp);
    if (FAILED(hr))
        return hr;

    V_VT(&vari) = VT_I4;
    V_I4(&vari) = dwValue;
    hr = pComp->SetValue(0, &vari);

    pComp->Release();
    return hr;
}

static BOOL
CheckX64System(
    _In_ LPWSTR lpCmdLine)
{
    // Is the system x64?
    SYSTEM_INFO SystemInfo;
    ::GetSystemInfo(&SystemInfo);
    if (SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_IA64 ||
        SystemInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64)
    {
        return FALSE;
    }

    // Get GetSystemWow64DirectoryW function
    g_hKernel32 = GetSystemModuleHandle(L"kernel32.dll", FALSE);
    g_fnGetSystemWow64DirectoryW =
        (FN_GetSystemWow64DirectoryW)::GetProcAddress(g_hKernel32, "GetSystemWow64DirectoryW");
    if (!g_fnGetSystemWow64DirectoryW)
        return FALSE;

    // Build WoW64 ctfmon.exe pathname
    WCHAR szPath[MAX_PATH];
    UINT cchPath = g_fnGetSystemWow64DirectoryW(szPath, _countof(szPath));
    if (!cchPath && FAILED(StringCchCatW(szPath, _countof(szPath), L"\\ctfmon.exe")))
        return FALSE;

    // Create a WoW64 ctfmon.exe process
    PROCESS_INFORMATION pi;
    STARTUPINFOW si = { sizeof(si) };
    si.wShowWindow = SW_SHOWMINNOACTIVE;
    if (!::CreateProcessW(szPath, lpCmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return FALSE;

    ::CloseHandle(pi.hThread);
    ::CloseHandle(pi.hProcess);
    return TRUE;
}

static BOOL
InitApp(
    _In_ HINSTANCE hInstance,
    _In_ LPWSTR lpCmdLine)
{
    g_hInst = hInstance;    // Save the instance handle
    g_uACP = GetACP();      // Save the active codepage
    g_bOnWow64 = IsWow64(); // Is the system on WoW64?

    // Create a mutex for Cicero
    g_hCicMutex = TF_CreateCicLoadMutex(&g_fWinLogon);
    if (!g_hCicMutex)
        return FALSE;

    // Write to "Run" registry key for starting up
    WriteRegRun();

    // Get OS info
    GetOSInfo();

    // Call SetProcessShutdownParameters if possible
    if (g_dwOsInfo & OSINFO_NT)
    {
        g_hKernel32 = GetSystemModuleHandle(L"kernel32.dll", FALSE);
        g_fnSetProcessShutdownParameters =
            (FN_SetProcessShutdownParameters)
                ::GetProcAddress(g_hKernel32, "SetProcessShutdownParameters");

        if (g_fnSetProcessShutdownParameters)
            g_fnSetProcessShutdownParameters(0xF0, SHUTDOWN_NORETRY);
    }

    // Start text framework
    TF_InitSystem();

    // Start watching registry if x86/x64 native
    if (!g_bOnWow64)
        CRegWatcher::Init();

    // Create TIP Bar window
    g_pTipBarWnd = new CTipBarWnd();
    if (!g_pTipBarWnd || !g_pTipBarWnd->Init())
        return FALSE;

    if (g_pTipBarWnd->CreateWnd())
    {
        // Go to the bottom of the hell
        ::SetWindowPos(g_pTipBarWnd->m_hWnd, HWND_BOTTOM, 0, 0, 0, 0,
                       SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
    }

    // Display TIP Bar Popup if x86/x64 native
    if (!g_bOnWow64)
        GetPopupTipbar(g_pTipBarWnd->m_hWnd, g_fWinLogon);

    // Do x64 stuffs
    CheckX64System(lpCmdLine);

    return TRUE;
}

VOID
UninitApp(VOID)
{
    // Close TIP Bar Popup
    ClosePopupTipbar();

    // Close the mutex
    CloseHandle(g_hCicMutex);
    g_hCicMutex = NULL;

    // Quit watching registry if x86/x64 native
    if (!g_bOnWow64)
        CRegWatcher::Uninit();
}

static INT
DoMainLoop(VOID)
{
    MSG msg;

    if (g_bOnWow64) // Is the system on WoW64?
    {
        // Just a simple message loop
        while (::GetMessageW(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
        return (INT)msg.wParam;
    }

    // Open the existing event by the name
    HANDLE hSwitchEvent = ::OpenEventW(SYNCHRONIZE, FALSE, L"WinSta0_DesktopSwitch");

    // The target events to watch
    HANDLE ahEvents[WATCHENTRY_MAX + 1];

    // Borrow some handles from CRegWatcher
    CopyMemory(ahEvents, CRegWatcher::s_ahWatchEvents, WATCHENTRY_MAX * sizeof(HANDLE));

    ahEvents[EI_DESKTOP_SWITCH] = hSwitchEvent; // Add it

    // Another message loop
    for (;;)
    {
        // Wait for target signal
        DWORD dwWait = ::MsgWaitForMultipleObjects(_countof(ahEvents), ahEvents, 0, INFINITE,
                                                   QS_ALLINPUT);
        if (dwWait == (WAIT_OBJECT_0 + _countof(ahEvents))) // Is input available?
        {
            // Do the events
            while (::PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    goto Quit;

                ::TranslateMessage(&msg);
                ::DispatchMessageW(&msg);
            }
        }
        else if (dwWait == (WAIT_OBJECT_0 + EI_DESKTOP_SWITCH)) // Desktop switch?
        {
#define GUID_COMPARTMENT_SPEECH_OPENCLOSE GUID_NULL // FIXME
            SetGlobalCompartmentDWORD(GUID_COMPARTMENT_SPEECH_OPENCLOSE, 0);
            ::ResetEvent(hSwitchEvent);
        }
        else // Do the other events
        {
            CRegWatcher::OnEvent(INT(dwWait - WAIT_OBJECT_0));
        }
    }

Quit:
    ::CloseHandle(hSwitchEvent);

    return (INT)msg.wParam;
}

// The main function for Unicode Win32
INT WINAPI
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInst,
    LPWSTR lpCmdLine,
    INT nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInst);
    UNREFERENCED_PARAMETER(nCmdShow);

    /////////////////////////////////////////////////////////////////////////
    // Prologue of wWinMain

    // Parse command line
    ParseCommandLine(lpCmdLine);

    if (g_fJustRunKey) // If '/R' option is specified
    {
        // Just write registry and exit
        WriteRegRun();
        return 1;
    }

    // Initialize the application
    if (!InitApp(hInstance, lpCmdLine))
        return 0;

    /////////////////////////////////////////////////////////////////////////
    // The main loop

    INT ret = DoMainLoop();

    /////////////////////////////////////////////////////////////////////////
    // Epilogue of wWinMain

    if (g_pTipBarWnd)
    {
        delete g_pTipBarWnd;
        g_pTipBarWnd = NULL;
    }

    // Un-initialize app and text framework
    if (!CTipBarWnd::s_bUninitedSystem)
    {
        UninitApp();
        TF_UninitSystem();
    }

    return ret;
}
