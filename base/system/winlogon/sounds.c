
/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <mmsystem.h>
#include <winsvc.h>

/* GLOBALS ******************************************************************/

typedef struct tagWINLOGON_PLAYSOUND_DATA
{
    WINLOGON_SYSTEM_SOUND Sound;
    PWLSESSION Session;

} WINLOGON_PLAYSOUND_DATA, *PWINLOGON_PLAYSOUND_DATA;

/* FUNCTIONS ****************************************************************/

static BOOL PlaySoundRoutine(LPCWSTR FileName, BOOL bLogon, UINT Flags)
{
    typedef BOOL (WINAPI *PFN_PlaySoundW)(LPCWSTR, HMODULE, DWORD);
    typedef UINT (WINAPI *PFN_waveOutGetNumDevs)(void);

    PFN_PlaySoundW WINMM_PlaySoundW;
    PFN_waveOutGetNumDevs WINMM_waveOutGetNumDevs;

    UINT uNumDevs;
    HMODULE hLibrary;
    BOOL ret = FALSE;

    hLibrary = LoadLibraryW(L"winmm.dll");

    if (hLibrary)
    {
        WINMM_waveOutGetNumDevs = (PFN_waveOutGetNumDevs)GetProcAddress(hLibrary, "waveOutGetNumDevs");

        if (WINMM_waveOutGetNumDevs)
        {
            uNumDevs = WINMM_waveOutGetNumDevs();

            if (!uNumDevs)
            {
                if (!bLogon)
                {
                    Beep(500, 500);
                }

                FreeLibrary(hLibrary);

                return FALSE;
            }
        }

        WINMM_PlaySoundW = (PFN_PlaySoundW)GetProcAddress(hLibrary, "PlaySoundW");

        if (WINMM_PlaySoundW)
        {
            ret = WINMM_PlaySoundW(FileName, NULL, Flags);
        }

        FreeLibrary(hLibrary);
    }

    return ret;
}

static DWORD WINAPI PlaySystemSoundThread(LPVOID lpParameter)
{
    PWINLOGON_PLAYSOUND_DATA PSData = (PWINLOGON_PLAYSOUND_DATA)lpParameter;

    LPCWSTR lpRegSubKey;
    HKEY hHKCU, hRegKey, hRegSnd;
    LPWSTR lpRegVal, lpSndPath;
    DWORD dwRegValType, dwRegValSize, dwSndPathLen;
    LONG lRegStatus;
    BOOL bLogon;

    SERVICE_STATUS_PROCESS Info;
    DWORD dwSize;
    ULONG Index = 0;
    SC_HANDLE hSCManager, hService;

    bLogon = (PSData->Sound == SYSTEMSND_LOGON) ? TRUE : FALSE;

    if (bLogon)
    {
        /* Open the service manager */
        hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);

        if (!hSCManager)
        {
            ERR("OpenSCManager failed (%x)\n", GetLastError());
            goto Quit;
        }

        /* Open the wdmaud service */
        hService = OpenServiceW(hSCManager, L"wdmaud", GENERIC_READ);

        if (!hService)
        {
            /* The service is not installed */
            TRACE("Failed to open wdmaud service (%x)\n", GetLastError());

            CloseServiceHandle(hSCManager);

            goto Quit;
        }

        /* Wait for wdmaud to start */
        do
        {
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(SERVICE_STATUS_PROCESS), &dwSize))
            {
                TRACE("QueryServiceStatusEx failed (%x)\n", GetLastError());
                break;
            }

            if (Info.dwCurrentState == SERVICE_RUNNING)
            {
                break;
            }

            Sleep(1000);

        } while (Index++ < 20);

        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);

        /* If wdmaud is not running exit */
        if (Info.dwCurrentState != SERVICE_RUNNING)
        {
            WARN("wdmaud has not started!\n");
            goto Quit;
        }

        /* Sound subsystem is running. Play logon sound. */
    }

    switch (PSData->Sound)
    {
        case SYSTEMSND_DEFAULT:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemDefault";
            break;

        case SYSTEMSND_LOGON:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\WindowsLogon";
            break;

        case SYSTEMSND_LOGOFF:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\WindowsLogoff";
            break;

        case SYSTEMSND_ASTERISK:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemAsterisk";
            break;

        case SYSTEMSND_EXCLAMATION:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemExclamation";
            break;

        case SYSTEMSND_CRITICAL_STOP:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemHand";
            break;

        case SYSTEMSND_QUESTION:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemQuestion";
            break;

        default:
            lpRegSubKey = L"AppEvents\\Schemes\\Apps\\.Default\\SystemDefault";
    }

    /* Impersonate current user */
    if (!ImpersonateLoggedOnUser(PSData->Session->UserToken))
    {
        goto Quit;
    }

    /* Open user's HKCU */
    lRegStatus = RegOpenCurrentUser(KEY_QUERY_VALUE, &hHKCU);

    if (lRegStatus != ERROR_SUCCESS)
    {
        RevertToSelf();
        goto Quit;
    }

    RevertToSelf();

    /* Open registry key */
    lRegStatus = RegOpenKeyExW(hHKCU, lpRegSubKey, 0, KEY_QUERY_VALUE, &hRegKey);

    RegCloseKey(hHKCU);

    if (lRegStatus != ERROR_SUCCESS)
    {
        goto Quit;
    }

    /* Open .Current */
    lRegStatus = RegOpenKeyExW(hRegKey, L".Current", 0, KEY_QUERY_VALUE, &hRegSnd);

    if (lRegStatus != ERROR_SUCCESS)
    {
        /* If fail then open .Default */
        lRegStatus = RegOpenKeyExW(hRegKey, L".Default", 0, KEY_QUERY_VALUE, &hRegSnd);

        if (lRegStatus != ERROR_SUCCESS)
        {
            RegCloseKey(hRegKey);
            goto Quit;
        }
    }

    RegCloseKey(hRegKey);

    /* Get registry value size */
    lRegStatus = RegQueryValueExW(hRegSnd, NULL, NULL, &dwRegValType, NULL, &dwRegValSize);

    /* Check whether the type is valid */
    if (lRegStatus != ERROR_SUCCESS || (dwRegValType != REG_SZ && dwRegValType != REG_EXPAND_SZ))
    {
        RegCloseKey(hRegSnd);
        goto Quit;
    }

    /* Allocate buffer for registry value */
    lpRegVal = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)dwRegValSize);

    if (!lpRegVal)
    {
        RegCloseKey(hRegSnd);
        goto Quit;
    }

    /* Get registry value */
    RegQueryValueExW(hRegSnd, NULL, NULL, &dwRegValType, (LPBYTE)lpRegVal, &dwRegValSize);

    RegCloseKey(hRegSnd);

    /* Get full sound path length (including the terminating null character) */
    dwSndPathLen = ExpandEnvironmentStringsW(lpRegVal, NULL, 0);

    if (dwSndPathLen == 0)
    {
        HeapFree(GetProcessHeap(), 0, lpRegVal);
        goto Quit;
    }

    /* Allocate buffer for sound path */
    lpSndPath = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)(dwSndPathLen * sizeof(WCHAR)));

    if (!lpSndPath)
    {
        HeapFree(GetProcessHeap(), 0, lpRegVal);
        goto Quit;
    }

    ExpandEnvironmentStringsW(lpRegVal, lpSndPath, dwSndPathLen);

    TRACE("Playing system sound: %ls\n", lpSndPath);

    /* Play sound */
    PlaySoundRoutine(lpSndPath, bLogon, SND_FILENAME | SND_NODEFAULT);

    HeapFree(GetProcessHeap(), 0, lpRegVal);
    HeapFree(GetProcessHeap(), 0, lpSndPath);

Quit:

    /* Deallocate play sound data */
    HeapFree(GetProcessHeap(), 0, PSData);

    return 0;
}

BOOL PlaySystemSound(PWLSESSION Session, WINLOGON_SYSTEM_SOUND Sound)
{
    PWINLOGON_PLAYSOUND_DATA PSData;
    HANDLE hThread;

    PSData = HeapAlloc(GetProcessHeap(), 0, sizeof(WINLOGON_PLAYSOUND_DATA));

    if (!PSData)
    {
        return FALSE;
    }

    PSData->Sound = Sound;
    PSData->Session = Session;

    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)PlaySystemSoundThread, (LPVOID)PSData, CREATE_SUSPENDED, NULL);

    if (hThread)
    {
        if (Sound != SYSTEMSND_LOGON && Sound != SYSTEMSND_LOGOFF)
        {
            /* Same as SND_ASYNC */
            SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
        }

        ResumeThread(hThread);

        CloseHandle(hThread);

        return TRUE;
    }

    /* Error */

    HeapFree(GetProcessHeap(), 0, PSData);

    return FALSE;
}
