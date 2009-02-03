/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         shell32.dll
 * FILE:            dll/win32/shell32/stubs.c
 * PURPOSE:         shell32.dll stubs
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      03/02/2009  Created
 */


#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
    FIXME("SHLocalAlloc() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalFree(HLOCAL hMem)
{
    FIXME("SHLocalFree() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalReAlloc(HLOCAL hMem,
               SIZE_T uBytes,
               UINT uFlags)
{
    FIXME("SHLocalReAlloc() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
AddCommasW(DWORD dwUnknown, LPWSTR lpNumber)
{
    LPWSTR lpRetBuf = L"0";

    FIXME("AddCommasW() stub\n");
    return lpRetBuf;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
ShortSizeFormatW(LONGLONG llNumber)
{
    FIXME("ShortSizeFormatW() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHFindComputer(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    FIXME("SHFindComputer() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHLimitInputEdit(HWND hWnd, LPVOID lpUnknown)
{
    FIXME("SHLimitInputEdit() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHLimitInputCombo(HWND hWnd, LPVOID lpUnknown)
{
    FIXME("SHLimitInputCombo() stub\n");
    return FALSE;
}
