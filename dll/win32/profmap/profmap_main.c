/*
 * PROJECT:         ReactOS profmap.dll
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/profmap/profmap_main.c
 * PURPOSE:         ReactOS Userenv
 * PROGRAMMER:      Copyright 2019 Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

/* INCLUDES *******************************************************************/

#include <windef.h>
#include <winbase.h>

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
DllMain(HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/*
 * @unimplemented
 * 
 * NOTES:
 *   Based on the documentation from:
 *   http://sendmail2.blogspot.com/2012/11/windows-small-business-server-2008_7553.html?view=magazine
 */
BOOL
WINAPI
RemapAndMoveUserA(IN LPCSTR pComputer,
                  IN DWORD dwFlags,
                  IN LPCSTR pCurrentUser,
                  IN LPCSTR pNewUser)
{
    DPRINT1("RemapAndMoveUserA is unimplemented\n");
    return FALSE;
}

/*
 * @unimplemented
 * 
 * NOTES:
 *   Based on the documentation from:
 *   http://sendmail2.blogspot.com/2012/11/windows-small-business-server-2008_7553.html?view=magazine
 */
BOOL
WINAPI
RemapAndMoveUserW(IN LPCWSTR pComputer,
                  IN DWORD dwFlags,
                  IN LPCWSTR pCurrentUser,
                  IN LPCWSTR pNewUser)
{
    DPRINT1("RemapAndMoveUserW is unimplemented\n");
    return FALSE;
}

/* EOF */
