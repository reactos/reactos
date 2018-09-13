#include "npstub.h"
#include <netspi.h>
#include <npord.h>
#include <nphook.h>
#include <npstubx.h>    /* message defs, class name */

#define ARRAYSIZE(x) (sizeof(x)/sizeof((x)[0]))

HINSTANCE hInstance = NULL;
HWND hwndMonitor = NULL;
CRITICAL_SECTION critsec;
ATOM aClass = NULL;

#define ENTERCRITICAL EnterCriticalSection(&::critsec);
#define LEAVECRITICAL LeaveCriticalSection(&::critsec);

UINT cCallsInProgress = 0;
BOOL fShouldUnload = FALSE;


/* This chunk of code invokes the entrypoint hooking feature of MPR.  We hook
 * things and immediately unhook ourselves.  We don't really want to hook
 * any functionality, this is just a way to kick MPR so he'll redetermine
 * the capabilities (via NPGetCaps) of all the net providers, including
 * ours.
 */
F_NPSHookMPR HookHookMPR;
F_UnHookMPR HookUnHookMPR;
F_LoadLibrary HookLoadLibrary;
F_FreeLibrary HookFreeLibrary;
F_GetProcAddress HookGetProcAddress;
F_LoadLibrary16 HookWMLoadWinnet16;
F_FreeLibrary16 HookWMFreeWinnet16;
F_GetProcAddressByName16 HookWMGetProcAddressByName;
F_GetProcAddressByOrdinal16 HookWMGetProcAddressByOrdinal;

MPRCALLS        MPRCalls = { HookHookMPR,
                             HookUnHookMPR,
                             HookLoadLibrary,
                             HookFreeLibrary,
                             HookGetProcAddress,
                             HookWMLoadWinnet16,
                             HookWMFreeWinnet16,
                             HookWMGetProcAddressByName,
                             HookWMGetProcAddressByOrdinal };


DWORD NPSERVICE HookHookMPR ( PMPRCALLS pMPRCalls )
{
    return ((PF_NPSHookMPR)(MPRCalls.pfNPSHookMPR))(pMPRCalls);
}

DWORD NPSERVICE HookUnHookMPR ( PF_NPSHookMPR pfReqNPSHookMPR, 
    PMPRCALLS pReqMPRCalls )
{
    if (pfReqNPSHookMPR == HookHookMPR) {
    
        // The unhook request has reached the hooker that issued
        // the NPSUnHookMe call (us).
        // In other words we are now sucessfully unhooked
        // and may do our unhooking cleanup.
        // In particular, we can release our tables that
        // manage LoadLibrary/GetProcAddress.
        // Note that this code may be executing on a different
        // thread to the NPSUnHookMe call which may have returned
        // a while ago.

        return WN_SUCCESS;
    }
    else {

        // Another hooker has requested to unhook by calling
        // NPSUnHookMe which causes us to be called here.
        // Pass the request on to the MPR service NPSUnHookMPR to
        // process the request, giving it our MPRCALLS
        // data structure so that it can figure out if
        // we are the right hooker to update and otherwise
        // MPR will pass the request on to the next hooker.

        return NPSUnHookMPR ( pfReqNPSHookMPR,
                              pReqMPRCalls,
                              (PMPRCALLS)&MPRCalls );
    }    
}

HINSTANCE HookLoadLibrary(
    LPCTSTR  lpszLibFile
    )
{
    return MPRCalls.pfLoadLibrary(lpszLibFile);
}

BOOL HookFreeLibrary(
    HMODULE hLibModule
    )
{
    return MPRCalls.pfFreeLibrary(hLibModule);
}

FARPROC HookGetProcAddress(
    HMODULE hModule,
    LPCSTR  lpszProc
    )
{
    return MPRCalls.pfGetProcAddress(hModule, lpszProc);
}

HANDLE16 HookWMLoadWinnet16(
    LPCTSTR  lpszLibFile
    )
{
    return MPRCalls.pfLoadLibrary16(lpszLibFile);
}

VOID HookWMFreeWinnet16(
    HANDLE16 hLibModule
    )
{
    MPRCalls.pfFreeLibrary16(hLibModule);
}

DWORD WINAPI HookWMGetProcAddressByName(
    LPCSTR   lpszProc,
    HANDLE16 hModule
    )
{
    return MPRCalls.pfGetProcAddressByName16(lpszProc, hModule);
}

DWORD WINAPI HookWMGetProcAddressByOrdinal(
    WORD     wOrdinal,
    HANDLE16 hModule
    )
{
    return MPRCalls.pfGetProcAddressByOrdinal16(wOrdinal, hModule);
}

void KickMPR(void)
{
    if (NPSHookMPR((PMPRCALLS)&MPRCalls) == WN_SUCCESS) {
        NPSUnHookMe(HookHookMPR, (PMPRCALLS)&MPRCalls);
    }
}
/***** End MPR hooking code *****/


/***** Begin code to delay-load the real net provider DLL *****/
HMODULE hmodRealNP = NULL;

PF_NPGetCaps pfnNPGetCaps = NULL;
PF_NPGetUniversalName pfnNPGetUniversalName = NULL;
PF_NPGetUser pfnNPGetUser = NULL;
PF_NPValidLocalDevice pfnNPValidLocalDevice = NULL;
PF_NPAddConnection pfnNPAddConnection = NULL;
PF_NPCancelConnection pfnNPCancelConnection = NULL;
PF_NPGetConnection pfnNPGetConnection = NULL;
PF_NPGetConnectionPerformance pfnNPGetConnectionPerformance = NULL;
PF_NPFormatNetworkName pfnNPFormatNetworkName = NULL;
PF_NPOpenEnum pfnNPOpenEnum = NULL;
PF_NPEnumResource pfnNPEnumResource = NULL;
PF_NPCloseEnum pfnNPCloseEnum = NULL;
PF_NPGetResourceParent pfnNPGetResourceParent = NULL;
PF_NPGetResourceInformation pfnNPGetResourceInformation = NULL;
PF_NPLogon pfnNPLogon = NULL;
PF_NPLogoff pfnNPLogoff = NULL;
PF_NPGetHomeDirectory pfnNPGetHomeDirectory = NULL;
PF_NPGetPolicyPath pfnNPGetPolicyPath = NULL;


struct {
    UINT nOrd;
    FARPROC *ppfn;
} aProcs[] = {
    { ORD_GETCAPS, (FARPROC *)&pfnNPGetCaps },
    { ORD_GETUNIVERSALNAME, (FARPROC *)&pfnNPGetUniversalName },
    { ORD_GETUSER, (FARPROC *)&pfnNPGetUser },
    { ORD_VALIDDEVICE, (FARPROC *)&pfnNPValidLocalDevice },
    { ORD_ADDCONNECTION, (FARPROC *)&pfnNPAddConnection },
    { ORD_CANCELCONNECTION, (FARPROC *)&pfnNPCancelConnection },
    { ORD_GETCONNECTIONS, (FARPROC *)&pfnNPGetConnection },
    { ORD_GETCONNPERFORMANCE, (FARPROC *)&pfnNPGetConnectionPerformance },
    { ORD_FORMATNETWORKNAME, (FARPROC *)&pfnNPFormatNetworkName },
    { ORD_OPENENUM, (FARPROC *)&pfnNPOpenEnum },
    { ORD_ENUMRESOURCE, (FARPROC *)&pfnNPEnumResource },
    { ORD_CLOSEENUM, (FARPROC *)&pfnNPCloseEnum },
    { ORD_GETRESOURCEPARENT, (FARPROC *)&pfnNPGetResourceParent },
    { ORD_GETRESOURCEINFORMATION, (FARPROC *)&pfnNPGetResourceInformation },
    { ORD_LOGON, (FARPROC *)&pfnNPLogon },
    { ORD_LOGOFF, (FARPROC *)&pfnNPLogoff },
    { ORD_GETHOMEDIRECTORY, (FARPROC *)&pfnNPGetHomeDirectory },
    { ORD_GETPOLICYPATH, (FARPROC *)&pfnNPGetPolicyPath },
};


void LoadRealNP(void)
{
    ENTERCRITICAL

    if (::hmodRealNP == NULL) {
        char szDLLName[MAX_PATH];

        szDLLName[0] = '\0';
        HKEY hkeySection;
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\NPSTUB\\NetworkProvider",
                         0, KEY_QUERY_VALUE, &hkeySection) == ERROR_SUCCESS) {
            DWORD dwType;
            DWORD cbData = sizeof(szDLLName);
            RegQueryValueEx(hkeySection, "RealDLL", NULL, &dwType, (LPBYTE)szDLLName, &cbData);
            RegCloseKey(hkeySection);
        }

        if (szDLLName[0] == '\0')
            lstrcpy(szDLLName, "mslocusr.dll");

        ::hmodRealNP = LoadLibrary(szDLLName);

        if (::hmodRealNP != NULL) {
            for (UINT i=0; i<ARRAYSIZE(::aProcs); i++) {
                *(aProcs[i].ppfn) = GetProcAddress(::hmodRealNP, (LPCSTR)aProcs[i].nOrd);
            }
        }
    }
    LEAVECRITICAL
}


void UnloadRealNP(void)
{
    ENTERCRITICAL
    {
        if (cCallsInProgress > 0) {
            fShouldUnload = TRUE;
        }
        else {
            for (UINT i=0; i<ARRAYSIZE(::aProcs); i++) {
                *(aProcs[i].ppfn) = NULL;
            }

            FreeLibrary(hmodRealNP);
            hmodRealNP = NULL;
            fShouldUnload = FALSE;
            KickMPR();
        }
    }
    LEAVECRITICAL
}


LRESULT MonitorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_NPSTUB_LOADDLL:
        LoadRealNP();
        KickMPR();
        break;

    case WM_NPSTUB_UNLOADDLL:
        UnloadRealNP();
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void _ProcessAttach()
{
    //
    // All the per-instance initialization code should come here.
    //
	::DisableThreadLibraryCalls(::hInstance);

    InitializeCriticalSection(&::critsec);

    WNDCLASS wc;

    wc.style = 0;
    wc.lpfnWndProc = MonitorWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = ::hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szNPSTUBClassName;

    ::aClass = RegisterClass(&wc);

    if (::aClass != NULL) {
        ::hwndMonitor = CreateWindow(szNPSTUBClassName, "",
                                     WS_POPUP | WS_DISABLED,
                                     0, 0, 0, 0,
                                     NULL, NULL,
                                     ::hInstance, NULL);
    }

    LoadRealNP();
}


void _ProcessDetach()
{
    if (::hwndMonitor != NULL)
        DestroyWindow(::hwndMonitor);
    if (::aClass != NULL)
        UnregisterClass((LPSTR)(WORD)::aClass, ::hInstance);

    DeleteCriticalSection(&::critsec);
}


extern "C" STDAPI_(BOOL) DllMain(HINSTANCE hInstDll, DWORD fdwReason, LPVOID reserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        ::hInstance = hInstDll;
        _ProcessAttach();
    }
    else if (fdwReason == DLL_PROCESS_DETACH) 
    {
        _ProcessDetach();
    }

    return TRUE;
}


void EnterSPI(void)
{
    ENTERCRITICAL
    {
        ::cCallsInProgress++;
    }
    LEAVECRITICAL
}


void LeaveSPI(void)
{
    ENTERCRITICAL
    {
        ::cCallsInProgress--;

        if (::fShouldUnload && !::cCallsInProgress)
            PostMessage(::hwndMonitor, WM_NPSTUB_UNLOADDLL, 0, 0);
    }
    LEAVECRITICAL
}


#define CALLNP(name,err,params)         \
    {                                   \
        if (pfn##name == NULL)          \
            return err;                 \
        DWORD dwRet = err;              \
        EnterSPI();                     \
        if (pfn##name != NULL)          \
            dwRet = (*pfn##name)params; \
        LeaveSPI();                     \
        return dwRet;                   \
    }                                   //last line doesn't need a backslash


SPIENTRY NPGetCaps(
    DWORD nIndex
    )
{
    CALLNP(NPGetCaps,0,(nIndex));
}


SPIENTRY NPGetUniversalName(
	LPTSTR  lpLocalPath,
	DWORD   dwInfoLevel,
	LPVOID  lpBuffer,
	LPDWORD lpBufferSize
    )
{
    CALLNP(NPGetUniversalName,WN_NOT_SUPPORTED,
           (lpLocalPath,dwInfoLevel,lpBuffer,lpBufferSize));
}


SPIENTRY NPGetUser(
    LPTSTR  lpName,
    LPTSTR  lpAuthenticationID,
    LPDWORD lpBufferSize
    )
{
    CALLNP(NPGetUser,WN_NOT_SUPPORTED,
           (lpName,lpAuthenticationID,lpBufferSize));
}


SPIENTRY NPValidLocalDevice(
    DWORD dwType,
    DWORD dwNumber
    )
{
    CALLNP(NPValidLocalDevice,WN_NOT_SUPPORTED,(dwType,dwNumber));
}


SPIENTRY NPAddConnection(
    HWND hwndOwner,
    LPNETRESOURCE lpNetResource,
    LPTSTR lpPassword,
    LPTSTR lpUserID,
    DWORD dwFlags,
	LPTSTR lpAccessName,
	LPDWORD lpBufferSize,
	LPDWORD lpResult
    )
{
    CALLNP(NPAddConnection,WN_NOT_SUPPORTED,
           (hwndOwner,lpNetResource,lpPassword,lpUserID,dwFlags,lpAccessName,lpBufferSize,lpResult));
}


SPIENTRY NPCancelConnection(
    LPTSTR lpName,
    BOOL fForce,
 	DWORD dwFlags
    )
{
    CALLNP(NPCancelConnection,WN_NOT_SUPPORTED,
           (lpName,fForce,dwFlags));
}


SPIENTRY NPGetConnection(
    LPTSTR lpLocalName,
    LPTSTR lpRemoteName,
    LPDWORD lpBufferSize
    )
{
    CALLNP(NPGetConnection,WN_NOT_SUPPORTED,
           (lpLocalName,lpRemoteName,lpBufferSize));
}


SPIENTRY NPGetConnectionPerformance(
    LPTSTR lpRemoteName, 
    LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
    )
{
    CALLNP(NPGetConnectionPerformance,WN_NOT_SUPPORTED,
           (lpRemoteName,lpNetConnectInfoStruct));
}


SPIENTRY NPFormatNetworkName(
    LPTSTR lpRemoteName,
    LPTSTR lpFormattedName,
    LPDWORD lpnLength,
    DWORD dwFlags,
    DWORD dwAveCharPerLine
    )
{
    CALLNP(NPFormatNetworkName,WN_NOT_SUPPORTED,
           (lpRemoteName,lpFormattedName,lpnLength,dwFlags,dwAveCharPerLine));
}


SPIENTRY NPOpenEnum(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage,
    LPNETRESOURCE lpNetResource,
    LPHANDLE lphEnum
    )
{
    CALLNP(NPOpenEnum,WN_NOT_SUPPORTED,
           (dwScope,dwType,dwUsage,lpNetResource,lphEnum));
}


SPIENTRY NPEnumResource(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbFree
    )
{
    CALLNP(NPEnumResource,WN_NOT_SUPPORTED,
           (hEnum,lpcCount,lpBuffer,cbBuffer,lpcbFree));
}


SPIENTRY NPCloseEnum(
    HANDLE hEnum
    )
{
    CALLNP(NPCloseEnum,WN_NOT_SUPPORTED,
           (hEnum));
}


SPIENTRY NPGetResourceParent(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD cbBuffer
    )
{
    CALLNP(NPGetResourceParent,WN_NOT_SUPPORTED,
           (lpNetResource,lpBuffer,cbBuffer));
}


SPIENTRY NPGetResourceInformation(
	LPNETRESOURCE lpNetResource,
	LPVOID lpBuffer,
	LPDWORD cbBuffer,
	LPSTR *lplpSystem
    )
{
    CALLNP(NPGetResourceInformation,WN_NOT_SUPPORTED,
           (lpNetResource,lpBuffer,cbBuffer,lplpSystem));
}


SPIENTRY NPLogon(
    HWND hwndOwner,
    LPLOGONINFO lpAuthentInfo,
    LPLOGONINFO lpPreviousAuthentInfo,
    LPTSTR lpLogonScript,
    DWORD dwBufferSize,
    DWORD dwFlags
    )
{
    CALLNP(NPLogon,WN_NOT_SUPPORTED,
           (hwndOwner,lpAuthentInfo,lpPreviousAuthentInfo,lpLogonScript,dwBufferSize,dwFlags));
}


SPIENTRY NPLogoff(
    HWND hwndOwner,
    LPLOGONINFO lpAuthentInfo,
    DWORD dwReason
    )
{
    CALLNP(NPLogoff,WN_NOT_SUPPORTED,
           (hwndOwner,lpAuthentInfo,dwReason));
}


SPIENTRY NPGetHomeDirectory(
    LPTSTR lpDirectory,
    LPDWORD lpBufferSize
    )
{
    CALLNP(NPGetHomeDirectory,WN_NOT_SUPPORTED,
           (lpDirectory,lpBufferSize));
}


SPIENTRY NPGetPolicyPath(
    LPTSTR lpPath,
    LPDWORD lpBufferSize,
	DWORD dwFlags
    )
{
    CALLNP(NPGetPolicyPath,WN_NOT_SUPPORTED,
           (lpPath,lpBufferSize,dwFlags));
}

