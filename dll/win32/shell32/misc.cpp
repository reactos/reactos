/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     C interface dependent functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define CINTERFACE
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <shobjidl.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 *  SHIsBadInterfacePtr [SHELL32.84]
 */
EXTERN_C BOOL WINAPI SHIsBadInterfacePtr(IUnknown *punk, UINT cbVtbl)
{
    return (IsBadReadPtr(punk, sizeof(LPVOID)) ||
            IsBadReadPtr(punk->lpVtbl, cbVtbl) ||
            IsBadCodePtr((FARPROC)punk->lpVtbl->Release));
}
