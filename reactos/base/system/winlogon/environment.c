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

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(winlogon);

/* GLOBALS ******************************************************************/


/* FUNCTIONS ****************************************************************/

BOOL
CreateUserEnvironment(IN PWLSESSION Session,
                      IN LPVOID *lpEnvironment,
                      IN LPWSTR *lpFullEnv)
{
    LPCWSTR wstr;
    SIZE_T EnvBlockSize = 0, ProfileSize = 0;
    LPVOID lpEnviron = NULL;
    LPWSTR lpFullEnviron = NULL;
    HKEY hKey;
    DWORD dwDisp;
    LONG lError;
    HKEY hKeyCurrentUser;

    TRACE("WL: CreateUserEnvironment called\n");

    /* Create environment block for the user */
    if (!CreateEnvironmentBlock(&lpEnviron,
                                Session->UserToken,
                                TRUE))
    {
        WARN("WL: CreateEnvironmentBlock() failed\n");
        return FALSE;
    }

    if (Session->Profile->dwType == WLX_PROFILE_TYPE_V2_0 && Session->Profile->pszEnvironment)
    {
        /* Count required size for full environment */
        wstr = (LPCWSTR)lpEnviron;
        while (*wstr != UNICODE_NULL)
        {
            SIZE_T size = wcslen(wstr) + 1;
            wstr += size;
            EnvBlockSize += size;
        }

        wstr = Session->Profile->pszEnvironment;
        while (*wstr != UNICODE_NULL)
        {
            SIZE_T size = wcslen(wstr) + 1;
            wstr += size;
            ProfileSize += size;
        }

        /* Allocate enough memory */
        lpFullEnviron = HeapAlloc(GetProcessHeap(), 0, (EnvBlockSize + ProfileSize + 1) * sizeof(WCHAR));
        if (!lpFullEnviron)
        {
            TRACE("HeapAlloc() failed\n");
            return FALSE;
        }

        /* Fill user environment block */
        CopyMemory(lpFullEnviron,
                   lpEnviron,
                   EnvBlockSize * sizeof(WCHAR));
        CopyMemory(&lpFullEnviron[EnvBlockSize],
                   Session->Profile->pszEnvironment,
                   ProfileSize * sizeof(WCHAR));
        lpFullEnviron[EnvBlockSize + ProfileSize] = UNICODE_NULL;
    }
    else
    {
        lpFullEnviron = (LPWSTR)lpEnviron;
    }

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

    *lpEnvironment = lpEnviron;
    *lpFullEnv = lpFullEnviron;

    TRACE("WL: CreateUserEnvironment done\n");

    return TRUE;
}
