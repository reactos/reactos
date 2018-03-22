/*
 * PatchAPI
 *
 * Copyright 2011 David Hedberg for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "patchapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mspatcha);

/*****************************************************
 *    DllMain (MSPATCHA.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;
    }

    return TRUE;
}

static inline WCHAR *strdupAW( const char *src )
{
    WCHAR *dst = NULL;
    if (src)
    {
        int len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
        if ((dst = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, src, -1, dst, len );
    }
    return dst;
}

/*****************************************************
 *    ApplyPatchToFileA (MSPATCHA.1)
 */
BOOL WINAPI ApplyPatchToFileA(LPCSTR patch_file, LPCSTR old_file, LPCSTR new_file, ULONG apply_flags)
{
    BOOL ret;
    WCHAR *patch_fileW, *new_fileW, *old_fileW = NULL;

    if (!(patch_fileW = strdupAW( patch_file ))) return FALSE;
    if (old_file && !(old_fileW = strdupAW( old_file )))
    {
        HeapFree( GetProcessHeap(), 0, patch_fileW );
        return FALSE;
    }
    if (!(new_fileW = strdupAW( new_file )))
    {
        HeapFree( GetProcessHeap(), 0, patch_fileW );
        HeapFree( GetProcessHeap(), 0, old_fileW );
        return FALSE;
    }
    ret = ApplyPatchToFileW( patch_fileW, old_fileW, new_fileW, apply_flags );
    HeapFree( GetProcessHeap(), 0, patch_fileW );
    HeapFree( GetProcessHeap(), 0, old_fileW );
    HeapFree( GetProcessHeap(), 0, new_fileW );
    return ret;
}

/*****************************************************
 *    ApplyPatchToFileW (MSPATCHA.6)
 */
BOOL WINAPI ApplyPatchToFileW(LPCWSTR patch_file, LPCWSTR old_file, LPCWSTR new_file, ULONG apply_flags)
{
    FIXME("stub - %s, %s, %s, %08x\n", debugstr_w(patch_file), debugstr_w(old_file),
          debugstr_w(new_file), apply_flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    GetFilePatchSignatureA (MSPATCHA.7)
 */
BOOL WINAPI GetFilePatchSignatureA(LPCSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
                                   PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
                                   PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, LPSTR buffer)
{
    FIXME("stub - %s, %x, %p, %u, %p, %u, %p, %u, %p\n", debugstr_a(filename), flags, data,
          ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    GetFilePatchSignatureW (MSPATCHA.9)
 */
BOOL WINAPI GetFilePatchSignatureW(LPCWSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
                                   PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
                                   PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, LPWSTR buffer)
{
    FIXME("stub - %s, %x, %p, %u, %p, %u, %p, %u, %p\n", debugstr_w(filename), flags, data,
          ignore_range_count, ignore_range, retain_range_count, retain_range, bufsize, buffer);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}
