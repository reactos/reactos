
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
    typedef BOOL (WINAPI *PFN_PLAYSOUNDW)(LPCWSTR, HMODULE, DWORD);
    typedef UINT (WINAPI *PFN_WAVEOUTGETNUMDEVS)(void);

    PFN_PLAYSOUNDW WINMM_PlaySoundW;
    PFN_WAVEOUTGETNUMDEVS WINMM_waveOutGetNumDevs;

    UINT uNumDevs;
    HMODULE hLibrary;
    BOOL bRet = FALSE;

    hLibrary = LoadLibraryW(L"winmm.dll");

    if (hLibrary)
    {
        WINMM_waveOutGetNumDevs = (PFN_WAVEOUTGETNUMDEVS)GetProcAddress(hLibrary, "waveOutGetNumDevs");

        if (WINMM_waveOutGetNumDevs)
        {
            uNumDevs = WINMM_waveOutGetNumDevs();

            if (uNumDevs == 0)
            {
                if (!bLogon)
                {
                    Beep(500, 500);
                }

                FreeLibrary(hLibrary);

                return FALSE;
            }
        }

        WINMM_PlaySoundW = (PFN_PLAYSOUNDW)GetProcAddress(hLibrary, "PlaySoundW");

        if (WINMM_PlaySoundW)
        {
            bRet = WINMM_PlaySoundW(FileName, NULL, Flags);
        }

        FreeLibrary(hLibrary);
    }

    return bRet;
}

static DWORD WINAPI PlaySystemSoundThread(LPVOID lpParameter)
{
    PWINLOGON_PLAYSOUND_DATA PSData = (PWINLOGON_PLAYSOUND_DATA)lpParameter;
    BOOL bLogon;
    BOOL bImpersonating = FALSE;
    LPCWSTR lpRegSubKey;
    HKEY hHKCU = NULL;
    HKEY hRegKey = NULL;
    HKEY hRegSnd = NULL;
    LPWSTR lpRegVal = NULL;
    LPWSTR lpSndPath = NULL;
    DWORD dwRegValType, dwRegValSize, dwSndPathLen;
    LONG lError;
  
    bLogon = (PSData->Sound == SYSTEMSND_LOGON);

    if (bLogon)
    {
        SERVICE_STATUS_PROCESS Info;
        DWORD dwSize;
        ULONG Index = 0;
        SC_HANDLE hSCManager, hService;
      
        /* Open the service manager */
        hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);

        if (!hSCManager)
        {
            ERR("OpenSCManagerW failed (%x)\n", GetLastError());
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
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(Info), &dwSize))
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

        /*
         * Sound subsystem is running. Play logon sound.
         */
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
    bImpersonating = ImpersonateLoggedOnUser(PSData->Session->UserToken);
  
    if (!bImpersonating)
    {
        goto Quit;
    }

    /* Open user's HKCU */
    lError = RegOpenCurrentUser(KEY_READ, &hHKCU);

    if (lError != ERROR_SUCCESS)
    {
        hHKCU = NULL;
      
        goto Cleanup;
    }

    /* Open registry key */
    lError = RegOpenKeyExW(hHKCU, lpRegSubKey, 0, KEY_READ, &hRegKey);

    if (lError != ERROR_SUCCESS)
    {
        hRegKey = NULL;
      
        goto Cleanup;
    }

    /* Open .Current */
    lError = RegOpenKeyExW(hRegKey, L".Current", 0, KEY_READ, &hRegSnd);

    if (lError != ERROR_SUCCESS)
    {
        /* If fail then open .Default */
        lError = RegOpenKeyExW(hRegKey, L".Default", 0, KEY_READ, &hRegSnd);

        if (lError != ERROR_SUCCESS)
        {
            hRegSnd = NULL;
        
            goto Cleanup;
        }
    }

    /* Get registry value size */
    lError = RegQueryValueExW(hRegSnd, NULL, NULL, &dwRegValType, NULL, &dwRegValSize);
  
    if (lError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    /* Check whether the type is valid */
    if (dwRegValType != REG_SZ && dwRegValType != REG_EXPAND_SZ)
    {
        goto Cleanup;
    }

    /* Allocate buffer for registry value */
    lpRegVal = HeapAlloc(GetProcessHeap(), 0, dwRegValSize);

    if (!lpRegVal)
    {
        goto Cleanup;
    }

    /* Get registry value */
    lError = RegQueryValueExW(hRegSnd, NULL, NULL, &dwRegValType, (LPBYTE)lpRegVal, &dwRegValSize);
  
    if (lError != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    /* Get full sound path length (including the terminating null character) */
    dwSndPathLen = ExpandEnvironmentStringsW(lpRegVal, NULL, 0);

    if (dwSndPathLen == 0)
    {
        goto Cleanup;
    }

    /* Allocate buffer for sound path */
    lpSndPath = HeapAlloc(GetProcessHeap(), 0, dwSndPathLen * sizeof(WCHAR));

    if (!lpSndPath)
    {
        goto Cleanup;
    }

    if (ExpandEnvironmentStringsW(lpRegVal, lpSndPath, dwSndPathLen) == 0)
    {
        goto Cleanup;
    }

    TRACE("Playing system sound: %ls\n", lpSndPath);

    /* Play sound */
    PlaySoundRoutine(lpSndPath, bLogon, SND_FILENAME | SND_NODEFAULT);

Cleanup:
  
    if (lpSndPath)
    {
        HeapFree(GetProcessHeap(), 0, lpSndPath);
    }
  
    if (lpRegVal)
    {
        HeapFree(GetProcessHeap(), 0, lpRegVal);
    }
  
    if (hRegSnd)
    {
        RegCloseKey(hRegSnd);
    }
  
    if (hRegKey)
    {
        RegCloseKey(hRegKey);
    }
  
    if (hHKCU)
    {
        RegCloseKey(hHKCU);
    }
      
    if (bImpersonating)
    {
        RevertToSelf();
    }

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

    hThread = CreateThread(NULL, 0, PlaySystemSoundThread, (LPVOID)PSData, CREATE_SUSPENDED, NULL);

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

    /*
     * Error
     */

    HeapFree(GetProcessHeap(), 0, PSData);

    return FALSE;
}
