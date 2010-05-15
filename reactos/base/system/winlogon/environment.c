/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Winlogon
 * FILE:            base/system/winlogon/environment.c
 * PURPOSE:         User environment routines
 * PROGRAMMERS:     Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "winlogon.h"
#include <shlobj.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winlogon);

/* GLOBALS ******************************************************************/

typedef HRESULT (WINAPI *PFSHGETFOLDERPATHW)(HWND, int, HANDLE, DWORD, LPWSTR);


/* FUNCTIONS ****************************************************************/

static VOID
BuildVolatileEnvironment(IN PWLSESSION Session,
                         IN HKEY hKey)
{
    HINSTANCE hShell32 = NULL;
    PFSHGETFOLDERPATHW pfSHGetFolderPathW = NULL;
    WCHAR szPath[MAX_PATH + 1];
    WCHAR szExpandedPath[MAX_PATH + 1];
    LPCWSTR wstr;
    SIZE_T size;
    WCHAR szEnvKey[MAX_PATH];
    WCHAR szEnvValue[1024];
    SIZE_T length;
    LPWSTR eqptr, endptr;

    /* Parse the environment variables and add them to the volatile environment key */
    if (Session->Profile->dwType == WLX_PROFILE_TYPE_V2_0 &&
        Session->Profile->pszEnvironment != NULL)
    {
        wstr = Session->Profile->pszEnvironment;
        while (*wstr != UNICODE_NULL)
        {
            size = wcslen(wstr) + 1;

            eqptr = wcschr(wstr, L'=');

            if (eqptr != NULL)
            {
                endptr = eqptr;

                endptr--;
                while (iswspace(*endptr))
                    endptr--;

                length = (SIZE_T)(endptr - wstr + 1);

                wcsncpy(szEnvKey, wstr, length);
                szEnvKey[length] = 0;

                eqptr++;
                while (iswspace(*eqptr))
                    eqptr++;
                wcscpy(szEnvValue, eqptr);

                RegSetValueExW(hKey,
                               szEnvKey,
                               0,
                               REG_SZ,
                               (LPBYTE)szEnvValue,
                               (wcslen(szEnvValue) + 1) * sizeof(WCHAR));
            }

            wstr += size;
        }
    }

    /* Load shell32.dll and call SHGetFolderPathW to get the users appdata folder path */
    hShell32 = LoadLibraryW(L"shell32.dll");
    if (hShell32 != NULL)
    {
        pfSHGetFolderPathW = (PFSHGETFOLDERPATHW)GetProcAddress(hShell32,
                                                                "SHGetFolderPathW");
        if (pfSHGetFolderPathW != NULL)
        {
            if (pfSHGetFolderPathW(NULL,
                                   CSIDL_APPDATA | CSIDL_FLAG_DONT_VERIFY,
                                   Session->UserToken,
                                   0,
                                   szPath) == S_OK)
            {
                /* FIXME: Expand %USERPROFILE% here. SHGetFolderPathW should do it for us. See Bug #5372.*/
                TRACE("APPDATA path: %S\n", szPath);
                ExpandEnvironmentStringsForUserW(Session->UserToken,
                                                 szPath,
                                                 szExpandedPath,
                                                 MAX_PATH);

                /* Add the appdata folder path to the users volatile environment key */
                TRACE("APPDATA expanded path: %S\n", szExpandedPath);
                RegSetValueExW(hKey,
                               L"APPDATA",
                               0,
                               REG_SZ,
                               (LPBYTE)szExpandedPath,
                               (wcslen(szExpandedPath) + 1) * sizeof(WCHAR));
            }
        }

        FreeLibrary(hShell32);
    }
}


BOOL
CreateUserEnvironment(IN PWLSESSION Session)
{
    HKEY hKey;
    DWORD dwDisp;
    LONG lError;
    HKEY hKeyCurrentUser;

    TRACE("WL: CreateUserEnvironment called\n");

    /* Impersonate the new user */
    ImpersonateLoggedOnUser(Session->UserToken);

    /* Open the new users HKCU key */
    lError = RegOpenCurrentUser(KEY_CREATE_SUB_KEY,
                                &hKeyCurrentUser);
    if (lError == ERROR_SUCCESS)
    {
        /* Create the 'Volatile Environment' key */
        lError = RegCreateKeyExW(hKeyCurrentUser,
                                 L"Volatile Environment",
                                 0,
                                 NULL,
                                 REG_OPTION_VOLATILE,
                                 KEY_WRITE,
                                 NULL,
                                 &hKey,
                                 &dwDisp);
        if (lError == ERROR_SUCCESS)
        {
            BuildVolatileEnvironment(Session,
                                     hKey);

            RegCloseKey(hKey);
        }
        else
        {
            WARN("WL: RegCreateKeyExW() failed (Error: %ld)\n", lError);
        }

        RegCloseKey(hKeyCurrentUser);
    }

    /* Revert the impersonation */
    RevertToSelf();

    TRACE("WL: CreateUserEnvironment done\n");

    return TRUE;
}
