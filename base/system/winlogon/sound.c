/*
 * PROJECT:     ReactOS Winlogon
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     System sound notifications
 * COPYRIGHT:   Copyright 2011 Rafal Harabien <rafalh@reactos.org>
 *              Copyright 2013 Eric Kohl <eric.kohl@reactos.org>
 *              Copyright 2016 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *              Copyright 2021 Thamatip Chitpong <tangaming123456@outlook.com>
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"

#include <mmsystem.h>
#include <winsvc.h>

/* GLOBALS ******************************************************************/

typedef struct _WL_PLAYSOUND_DATA
{
    PWLSESSION Session;
    LPWSTR pszSound;
} WL_PLAYSOUND_DATA, *PWL_PLAYSOUND_DATA;

/* FUNCTIONS ****************************************************************/

static inline
BOOL
SpeakerBeep(VOID)
{
    return Beep(440, 125);
}

static
BOOL
PlaySoundRoutine(
    _In_ LPCWSTR lpFileName,
    _In_ BOOL bLogon,
    _In_ DWORD dwFlags)
{
    typedef BOOL (WINAPI *PFPLAYSOUNDW)(LPCWSTR, HMODULE, DWORD);
    typedef UINT (WINAPI *PFWAVEOUTGETNUMDEVS)(VOID);

    HMODULE hLibrary;
    PFPLAYSOUNDW _PlaySoundW;
    PFWAVEOUTGETNUMDEVS _waveOutGetNumDevs;
    UINT uNumDevs;
    BOOL bRet = FALSE;

    hLibrary = LoadLibraryW(L"winmm.dll");
    if (hLibrary)
    {
        _waveOutGetNumDevs = (PFWAVEOUTGETNUMDEVS)GetProcAddress(hLibrary, "waveOutGetNumDevs");
        if (_waveOutGetNumDevs)
        {
            uNumDevs = _waveOutGetNumDevs();
            if (uNumDevs == 0)
            {
                if (!bLogon)
                {
                    SpeakerBeep();
                }

                FreeLibrary(hLibrary);

                return FALSE;
            }
        }

        _PlaySoundW = (PFPLAYSOUNDW)GetProcAddress(hLibrary, "PlaySoundW");
        if (_PlaySoundW)
        {
            bRet = _PlaySoundW(lpFileName, NULL, dwFlags);
        }

        FreeLibrary(hLibrary);
    }

    return bRet;
}

static
DWORD
WINAPI
PlaySystemSoundThread(
    _In_ LPVOID lpParameter)
{
    PWL_PLAYSOUND_DATA PSData;
    BOOL bLogon;

    PSData = (PWL_PLAYSOUND_DATA)lpParameter;

    /* If sound name is NULL, play beep sound */
    if (!PSData->pszSound)
    {
        SpeakerBeep();
        goto Cleanup;
    }

    bLogon = (_wcsicmp(PSData->pszSound, L"WindowsLogon") == 0);

    if (bLogon)
    {
        SERVICE_STATUS_PROCESS Info;
        DWORD dwBytesNeeded;
        ULONG Index = 0;
        SC_HANDLE hSCManager, hService;

        /* Open the service manager */
        hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
        if (!hSCManager)
        {
            ERR("OpenSCManagerW failed (%x)\n", GetLastError());
            goto Cleanup;
        }

        /* Open the wdmaud service */
        hService = OpenServiceW(hSCManager, L"wdmaud", GENERIC_READ);
        if (!hService)
        {
            /* The service is not installed */
            TRACE("Failed to open wdmaud service (%x)\n", GetLastError());
            CloseServiceHandle(hSCManager);
            goto Cleanup;
        }

        /* Wait for wdmaud to start */
        do
        {
            if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&Info, sizeof(Info), &dwBytesNeeded))
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
            goto Cleanup;
        }

        /* Sound subsystem is running. Play logon sound. */
    }

    /* Impersonate current user */
    if (!ImpersonateLoggedOnUser(PSData->Session->UserToken))
    {
        goto Cleanup;
    }

    /* Play sound */
    PlaySoundRoutine(PSData->pszSound, bLogon, SND_ALIAS | SND_NODEFAULT);

    /* End impersonation */
    RevertToSelf();

Cleanup:
    /* Free play sound data */
    HeapFree(GetProcessHeap(), 0, PSData->pszSound);
    HeapFree(GetProcessHeap(), 0, PSData);

    return 0;
}

BOOL
PlaySystemSound(
    _In_ PWLSESSION Session,
    _In_ LPCWSTR lpSound)
{
    PWL_PLAYSOUND_DATA PSData;
    SIZE_T NameLength;
    HANDLE hThread;

    if (!(Session && Session->UserToken))
    {
        return FALSE;
    }

    /* Allocate memory for play sound data structure */
    PSData = HeapAlloc(GetProcessHeap(), 0, sizeof(WL_PLAYSOUND_DATA));
    if (!PSData)
    {
        return FALSE;
    }

    PSData->Session = Session;

    if (lpSound)
    {
        /* Get sound name length, including the NULL terminator */
        NameLength = wcslen(lpSound) + 1;

        /* Allocate memory for sound name */
        PSData->pszSound = HeapAlloc(GetProcessHeap(), 0, NameLength * sizeof(WCHAR));
        if (!PSData->pszSound)
        {
            HeapFree(GetProcessHeap(), 0, PSData);
            return FALSE;
        }

        /* Copy sound name */
        StringCchCopyW(PSData->pszSound, NameLength, lpSound);
    }
    else
    {
        PSData->pszSound = NULL;
    }

    /* Create play sound thread */
    hThread = CreateThread(NULL, 0, PlaySystemSoundThread, PSData, 0, NULL);
    if (!hThread)
    {
        /* Error, free play sound data */
        HeapFree(GetProcessHeap(), 0, PSData->pszSound);
        HeapFree(GetProcessHeap(), 0, PSData);

        return FALSE;
    }

    CloseHandle(hThread);
    
    return TRUE;
}
