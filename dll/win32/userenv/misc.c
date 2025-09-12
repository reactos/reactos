/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/userenv/misc.c
 * PURPOSE:         User profile code
 * PROGRAMMER:      Eric Kohl
 */

#include "precomp.h"
#include <ndk/sefuncs.h>

#include "resources.h"

#define NDEBUG
#include <debug.h>

SID_IDENTIFIER_AUTHORITY LocalSystemAuthority = {SECURITY_NT_AUTHORITY};
SID_IDENTIFIER_AUTHORITY WorldAuthority = {SECURITY_WORLD_SID_AUTHORITY};

/* FUNCTIONS ***************************************************************/

LPWSTR
AppendBackslash(LPWSTR String)
{
    ULONG Length;

    Length = lstrlenW(String);
    if (String[Length - 1] != L'\\')
    {
        String[Length] = L'\\';
        Length++;
        String[Length] = (WCHAR)0;
    }

    return &String[Length];
}

PSECURITY_DESCRIPTOR
CreateDefaultSecurityDescriptor(VOID)
{
    PSID LocalSystemSid = NULL;
    PSID AdministratorsSid = NULL;
    PSID EveryoneSid = NULL;
    PACL Dacl;
    DWORD DaclSize;
    PSECURITY_DESCRIPTOR pSD = NULL;

    /* create the SYSTEM, Administrators and Everyone SIDs */
    if (!AllocateAndInitializeSid(&LocalSystemAuthority,
                                  1,
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &LocalSystemSid) ||
        !AllocateAndInitializeSid(&LocalSystemAuthority,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &AdministratorsSid) ||
        !AllocateAndInitializeSid(&WorldAuthority,
                                  1,
                                  SECURITY_WORLD_RID,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &EveryoneSid))
    {
        DPRINT1("Failed initializing the SIDs for the default security descriptor (0x%p, 0x%p, 0x%p)\n",
                LocalSystemSid, AdministratorsSid, EveryoneSid);
        goto Cleanup;
    }

    /* allocate the security descriptor and DACL */
    DaclSize = sizeof(ACL) +
               ((GetLengthSid(LocalSystemSid) +
                 GetLengthSid(AdministratorsSid) +
                 GetLengthSid(EveryoneSid)) +
                (3 * FIELD_OFFSET(ACCESS_ALLOWED_ACE,
                                  SidStart)));

    pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED,
                                           (SIZE_T)DaclSize + sizeof(SECURITY_DESCRIPTOR));
    if (pSD == NULL)
    {
        DPRINT1("Failed to allocate the default security descriptor and ACL\n");
        goto Cleanup;
    }

    if (!InitializeSecurityDescriptor(pSD,
                                      SECURITY_DESCRIPTOR_REVISION))
    {
        DPRINT1("Failed to initialize the default security descriptor\n");
        goto Cleanup;
    }

    /* initialize and build the DACL */
    Dacl = (PACL)((ULONG_PTR)pSD + sizeof(SECURITY_DESCRIPTOR));
    if (!InitializeAcl(Dacl,
                       (DWORD)DaclSize,
                       ACL_REVISION))
    {
        DPRINT1("Failed to initialize the DACL of the default security descriptor\n");
        goto Cleanup;
    }

    /* add the SYSTEM Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             LocalSystemSid))
    {
        DPRINT1("Failed to add the SYSTEM ACE\n");
        goto Cleanup;
    }

    /* add the Administrators Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_ALL,
                             AdministratorsSid))
    {
        DPRINT1("Failed to add the Administrators ACE\n");
        goto Cleanup;
    }

    /* add the Everyone Ace */
    if (!AddAccessAllowedAce(Dacl,
                             ACL_REVISION,
                             GENERIC_EXECUTE,
                             EveryoneSid))
    {
        DPRINT1("Failed to add the Everyone ACE\n");
        goto Cleanup;
    }

    /* set the DACL */
    if (!SetSecurityDescriptorDacl(pSD,
                                   TRUE,
                                   Dacl,
                                   FALSE))
    {
        DPRINT1("Failed to set the DACL of the default security descriptor\n");

Cleanup:
        if (pSD != NULL)
        {
            LocalFree((HLOCAL)pSD);
            pSD = NULL;
        }
    }

    if (LocalSystemSid != NULL)
    {
        FreeSid(LocalSystemSid);
    }
    if (AdministratorsSid != NULL)
    {
        FreeSid(AdministratorsSid);
    }
    if (EveryoneSid != NULL)
    {
        FreeSid(EveryoneSid);
    }

    return pSD;
}

/* Policy values helpers *****************************************************/

LONG
GetPolicyValues(
    _In_ HKEY hRootKey,
    _Inout_ PPOLICY_VALUES QueryTable)
{
    static PCWSTR PolicyKeys[] =
    {
        L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
        L"Software\\Policies\\Microsoft\\Windows\\System"
    };

    /* Retrieve the sought policy values in each of the policy keys */
    DWORD i;
    for (i = 0; i < _countof(PolicyKeys); ++i)
    {
        HKEY hKey;
        PPOLICY_VALUES Value;
        DWORD dwType, cbData;

        if (RegOpenKeyExW(hRootKey, PolicyKeys[i], 0,
                          KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        {
            continue;
        }

        for (Value = QueryTable; Value->ValueName || Value->Data; ++Value)
        {
            /* Retrieve the type and data length for validation checks */
            if (RegQueryValueExW(hKey, Value->ValueName, NULL,
                                 &dwType, NULL, &cbData) != ERROR_SUCCESS)
            {
                continue;
            }

            /* Verify for compatible types */
            if ( ((Value->Type == REG_SZ || Value->Type == REG_EXPAND_SZ) &&
                  (dwType == REG_SZ || dwType == REG_EXPAND_SZ)) ||
                 (Value->Type == REG_DWORD && dwType == REG_DWORD) ||
                 (Value->Type == REG_QWORD && dwType == REG_QWORD) ||
                 (Value->Type == REG_MULTI_SZ && dwType == REG_MULTI_SZ) ||
                 (Value->Type == REG_NONE || Value->Type == REG_BINARY) )
            {
                /* Verify whether the given buffer can hold the data;
                 * if so, retrieve the actual data */
                if (Value->Length >= cbData)
                {
                    RegQueryValueExW(hKey, Value->ValueName, NULL,
                                     &dwType, Value->Data, &cbData);
                }
            }
        }
        RegCloseKey(hKey);
    }

    return ERROR_SUCCESS;
}

LONG
GetPolicyValue(
    _In_ HKEY hRootKey,
    _In_ PCWSTR ValueName,
    _In_ DWORD Type,
    _Out_opt_ PVOID pData,
    _Inout_opt_ PDWORD pcbData)
{
    POLICY_VALUES QueryTable[] =
    {
        {ValueName, Type, pData, pcbData ? *pcbData : 0},
        {NULL, 0, NULL, 0}
    };
    LONG ret = GetPolicyValues(hRootKey, QueryTable);
    if ((ret == ERROR_SUCCESS) && pcbData)
        *pcbData = QueryTable[0].Length;

    return ret;
}

/* Timed dialog helpers ******************************************************/

#define IDT_DLGTIMER 1

typedef struct _TIMERDLG_BUTTON
{
    UINT uID;
    INT_PTR nResult;
} TIMERDLG_BUTTON, *PTIMERDLG_BUTTON;

typedef struct _TIMERDLG_INFO
{
    ULONG ulTimeout;       ///< Dialog timeout.
    WORD wDefBtnOnTimeout; ///< Index in button map for the default action on timeout.
    WORD wDefBtnOnCancel;  ///< Index in button map for the default action on dialog cancel.
    TIMERDLG_BUTTON ButtonMap[2]; ///< Button/result map for the 1st and 2nd buttons.
} TIMERDLG_INFO, *PTIMERDLG_INFO;

static INT_PTR
CALLBACK
TimerDlgProc(
    _Inout_ PTIMERDLG_INFO Info,
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            /* Setup the timeout */
            if (Info->ulTimeout)
            {
                /* Display the countdown */
                WCHAR szTimeout[10];
                StringCchPrintfW(szTimeout, _countof(szTimeout), L"%d", Info->ulTimeout);
                SetDlgItemTextW(hDlg, IDC_TIMEOUT, szTimeout);

                /* Set a 1-second timer */
                SetTimer(hDlg, IDT_DLGTIMER, 1000, NULL);
            }
            break;
        }

        case WM_COMMAND:
        {
            PTIMERDLG_BUTTON ButtonMap = Info->ButtonMap;
            WORD wDefBtnOnTimeout = min(Info->wDefBtnOnTimeout, _countof(Info->ButtonMap)-1);
            WORD i;

            if (LOWORD(wParam) == IDCANCEL)
            {
                /* ESC or CANCEL button pressed: stop the timeout,
                 * close the dialog and return the default result */
                WORD wDefBtnOnCancel = min(Info->wDefBtnOnCancel, _countof(Info->ButtonMap)-1);
                KillTimer(hDlg, IDT_DLGTIMER);
                EndDialog(hDlg, ButtonMap[wDefBtnOnCancel].nResult);
                break;
            }

            for (i = 0; i < _countof(Info->ButtonMap); ++i)
            {
                if (LOWORD(wParam) == ButtonMap[i].uID)
                {
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        /* Button pressed: stop the timeout, close the
                         * dialog and return the corresponding result */
                        KillTimer(hDlg, IDT_DLGTIMER);
                        EndDialog(hDlg, ButtonMap[i].nResult);
                    }
                    else if (HIWORD(wParam) == BN_KILLFOCUS)
                    {
                        /* Button lost the focus: stop the timeout
                         * only if this isn't the default action */
                        if (i != wDefBtnOnTimeout)
                            break;
                        KillTimer(hDlg, IDT_DLGTIMER);
                        ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUT), SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDC_TIMEOUTSTATIC), SW_HIDE);
                    }
                    break;
                }
            }
            break;
        }

        case WM_TIMER:
        {
            if (Info->ulTimeout < 1)
            {
                /* Timeout ended, close the dialog */
                WORD wDefBtnOnTimeout = min(Info->wDefBtnOnTimeout, _countof(Info->ButtonMap)-1);
                PostMessageW(hDlg, WM_COMMAND,
                             MAKEWPARAM(Info->ButtonMap[wDefBtnOnTimeout].uID, BN_CLICKED),
                             0);
            }
            else
            {
                /* Decrement and display the countdown */
                WCHAR szTimeout[10];
                StringCchPrintfW(szTimeout, _countof(szTimeout), L"%d", --(Info->ulTimeout));
                SetDlgItemTextW(hDlg, IDC_TIMEOUT, szTimeout);
            }
            break;
        }
    }

    return FALSE;
}

/* Error reporting helpers ***************************************************/

typedef struct _ERRORDLG_PARAMS
{
    ULONG ulTimeout;
    PCWSTR pszString;
} ERRORDLG_PARAMS, *PERRORDLG_PARAMS;

static TIMERDLG_INFO ErrorDlg =
{
    0, ///< Global error-dialog timeout, used for all displayed error dialogs.
    0, 1,
    {{IDOK, TRUE}, {IDCANCEL, FALSE}}
};

static INT_PTR
CALLBACK
ErrorDlgProc(
    _In_ HWND hDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PERRORDLG_PARAMS Params = (PERRORDLG_PARAMS)lParam;

        /* Position and focus the dialog */
        // CenterWindow(hDlg); // Already done with the DS_CENTER style.
        SetForegroundWindow(hDlg);

        /* Display the message */
        SetDlgItemTextW(hDlg, IDC_ERRORDESC, Params->pszString);

        /* Setup the timeout and display the countdown */
        ErrorDlg.ulTimeout = Params->ulTimeout;
        // SetWindowLongPtrW(hDlg, DWLP_USER, &ErrorDlg);
        TimerDlgProc(&ErrorDlg, hDlg, WM_INITDIALOG, wParam, lParam);
        return TRUE;
    }
    return TimerDlgProc(&ErrorDlg, hDlg, uMsg, wParam, lParam);
}

VOID
ErrorDialogEx(
    _In_ ULONG ulTimeout,
    _In_ PCWSTR pszString)
{
    ERRORDLG_PARAMS Params = {ulTimeout, pszString};
    DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_ERRORDLG),
                    NULL, ErrorDlgProc, (LPARAM)&Params);
}

VOID
ReportErrorWorker(
    _In_ DWORD dwFlags,
    _In_ PCWSTR pszStr)
{
    // TODO: Report event to Application log, "Userenv" source.
    DPRINT1("%S", pszStr);

    if (!(dwFlags & PI_NOUI))
    {
        /* Retrieve the "Profile Dialog Time Out" policy value,
         * defaulting to 30 seconds timeout */
        ULONG ulTimeout = 30;
        DWORD cbData = sizeof(ulTimeout);
        GetPolicyValue(HKEY_LOCAL_MACHINE,
                       L"ProfileDlgTimeOut",
                       REG_DWORD,
                       &ulTimeout,
                       &cbData);
        ErrorDialogEx(ulTimeout, pszStr);
    }
}

VOID
ReportErrorV(
    _In_ DWORD dwFlags,
    _In_ PCWSTR pszStr,
    _In_ va_list args)
{
    WCHAR Buffer[4096];
    _vsnwprintf(Buffer, _countof(Buffer), pszStr, args);
    ReportErrorWorker(dwFlags, Buffer);
}

VOID
__cdecl
ReportError(
    _In_ DWORD dwFlags,
    _In_ PCWSTR pszStr,
    ...)
{
    va_list args;

    va_start(args, pszStr);
    ReportErrorV(dwFlags, pszStr, args);
    va_end(args);
}


/* Dynamic DLL loading interface **********************************************/

/* OLE32.DLL import table */
DYN_MODULE DynOle32 =
{
  L"ole32.dll",
  {
    "CoInitialize",
    "CoCreateInstance",
    "CoUninitialize",
    NULL
  }
};

/*
 * Use this function to load functions from other modules. We cannot statically
 * link to e.g. ole32.dll because those dlls would get loaded on startup with
 * winlogon and they may try to register classes etc when not even a window station
 * has been created!
 */
BOOL
LoadDynamicImports(PDYN_MODULE Module,
                   PDYN_FUNCS DynFuncs)
{
    LPSTR *fname;
    PVOID *fn;

    ZeroMemory(DynFuncs, sizeof(DYN_FUNCS));

    DynFuncs->hModule = LoadLibraryW(Module->Library);
    if (!DynFuncs->hModule)
    {
        return FALSE;
    }

    fn = &DynFuncs->fn.foo;

    /* load the imports */
    for (fname = Module->Functions; *fname != NULL; fname++)
    {
        *fn = GetProcAddress(DynFuncs->hModule, *fname);
        if (*fn == NULL)
        {
            FreeLibrary(DynFuncs->hModule);
            DynFuncs->hModule = (HMODULE)0;

            return FALSE;
        }

        fn++;
    }

    return TRUE;
}

VOID
UnloadDynamicImports(PDYN_FUNCS DynFuncs)
{
    if (DynFuncs->hModule)
    {
        FreeLibrary(DynFuncs->hModule);
        DynFuncs->hModule = (HMODULE)0;
    }
}

/* EOF */
