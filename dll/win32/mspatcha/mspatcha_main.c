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

#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "ndk/rtlfuncs.h"
#include "patchapi.h"
#include "wine/debug.h"

static const char szHexString[] = "0123456789abcdef";
#define SIGNATURE_MIN_SIZE          9

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

/*****************************************************
 *    ApplyPatchToFileA (MSPATCHA.1)
 */
BOOL WINAPI ApplyPatchToFileA(LPCSTR patch_file, LPCSTR old_file, LPCSTR new_file, ULONG apply_flags)
{
    BOOL ret = FALSE;
    HANDLE hPatch, hOld, hNew;

    hPatch = CreateFileA(patch_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hPatch != INVALID_HANDLE_VALUE)
    {
        hOld = CreateFileA(old_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (hOld != INVALID_HANDLE_VALUE)
        {
            hNew = CreateFileA(new_file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if (hNew != INVALID_HANDLE_VALUE)
            {
                ret = ApplyPatchToFileByHandles(hPatch, hOld, hNew, apply_flags);
                CloseHandle(hNew);
            }
            CloseHandle(hOld);
        }
        CloseHandle(hPatch);
    }

    return ret;
}

/*****************************************************
 *    ApplyPatchToFileByHandles (MSPATCHA.2)
 */
BOOL WINAPI ApplyPatchToFileByHandles(HANDLE patch_file, HANDLE old_file, HANDLE new_file, ULONG apply_flags)
{
    FIXME("stub - %p, %p, %p, %08x\n", patch_file, old_file, new_file, apply_flags);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*****************************************************
 *    ApplyPatchToFileW (MSPATCHA.6)
 */
BOOL WINAPI ApplyPatchToFileW(LPCWSTR patch_file, LPCWSTR old_file, LPCWSTR new_file, ULONG apply_flags)
{
    BOOL ret = FALSE;
    HANDLE hPatch, hOld, hNew;

    hPatch = CreateFileW(patch_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hPatch != INVALID_HANDLE_VALUE)
    {
        hOld = CreateFileW(old_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (hOld != INVALID_HANDLE_VALUE)
        {
            hNew = CreateFileW(new_file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
            if (hNew != INVALID_HANDLE_VALUE)
            {
                ret = ApplyPatchToFileByHandles(hPatch, hOld, hNew, apply_flags);
                CloseHandle(hNew);
            }
            CloseHandle(hOld);
        }
        CloseHandle(hPatch);
    }

    return ret;
}

/*****************************************************
 *    GetFilePatchSignatureA (MSPATCHA.7)
 */
BOOL WINAPI GetFilePatchSignatureA(LPCSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
                                   PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
                                   PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, PVOID buffer)
{
    BOOL ret = FALSE;
    HANDLE hFile;

    hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ret = GetFilePatchSignatureByHandle(hFile, flags, data, ignore_range_count, ignore_range,
                                            retain_range_count, retain_range, bufsize, buffer);
        CloseHandle(hFile);
    }

    return ret;
}

/*****************************************************
 *    GetFilePatchSignatureA (MSPATCHA.7)
 */
BOOL WINAPI GetFilePatchSignatureByHandle(HANDLE hFile, ULONG flags, PVOID data, ULONG ignore_range_count,
                                   PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
                                   PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, PVOID buffer)
{
    BOOL ret = FALSE;
    HANDLE hMap;
    DWORD dwSize, ulCrc;
    PVOID pView;

    if (flags)
        FIXME("Unhandled flags 0x%x\n", flags);
    if (ignore_range_count)
        FIXME("Unhandled ignore_range_count %u\n", ignore_range_count);
    if (retain_range_count)
        FIXME("Unhandled ignore_range_count %u\n", retain_range_count);

    dwSize = GetFileSize(hFile, NULL);
    hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap != INVALID_HANDLE_VALUE)
    {
        pView = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hMap);

        if (dwSize >= 2 && *(PWORD)pView == IMAGE_DOS_SIGNATURE)
        {
            FIXME("Potentially unimplemented case, normalized signature\n");
        }

        ulCrc = RtlComputeCrc32(0, pView, dwSize);
        if (bufsize >= SIGNATURE_MIN_SIZE)
        {
            char *pBuffer = buffer;
            pBuffer[8] = '\0';
            for (dwSize = 0; dwSize < 8; ++dwSize)
            {
                pBuffer[7 - dwSize] = szHexString[ulCrc & 0xf];
                ulCrc >>= 4;
            }
            ret = TRUE;
        }
        UnmapViewOfFile(pView);

        if (bufsize < SIGNATURE_MIN_SIZE)
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
    }

    return ret;
}

/*****************************************************
 *    GetFilePatchSignatureW (MSPATCHA.9)
 */
BOOL WINAPI GetFilePatchSignatureW(LPCWSTR filename, ULONG flags, PVOID data, ULONG ignore_range_count,
                                   PPATCH_IGNORE_RANGE ignore_range, ULONG retain_range_count,
                                   PPATCH_RETAIN_RANGE retain_range, ULONG bufsize, PVOID buffer)
{
    CHAR LocalBuf[SIGNATURE_MIN_SIZE];
    BOOL ret = FALSE;
    HANDLE hFile;

    hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ret = GetFilePatchSignatureByHandle(hFile, flags, data, ignore_range_count, ignore_range,
                                            retain_range_count, retain_range, sizeof(LocalBuf), LocalBuf);
        CloseHandle(hFile);

        if (bufsize < (SIGNATURE_MIN_SIZE * sizeof(WCHAR)))
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }
        if (ret)
        {
            MultiByteToWideChar(CP_ACP, 0, LocalBuf, -1, buffer, bufsize / sizeof(WCHAR));
        }
    }

    return ret;
}

/*****************************************************
 *    TestApplyPatchToFileA (MSPATCHA.10)
 */
BOOL WINAPI TestApplyPatchToFileA(LPCSTR patch_file, LPCSTR old_file, ULONG apply_flags)
{
    BOOL ret = FALSE;
    HANDLE hPatch, hOld;

    hPatch = CreateFileA(patch_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hPatch != INVALID_HANDLE_VALUE)
    {
        hOld = CreateFileA(old_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (hOld != INVALID_HANDLE_VALUE)
        {
            ret = TestApplyPatchToFileByHandles(hPatch, hOld, apply_flags);
            CloseHandle(hOld);
        }
        CloseHandle(hPatch);
    }

    return ret;
}

/*****************************************************
 *    TestApplyPatchToFileByHandles (MSPATCHA.11)
 */
BOOL WINAPI TestApplyPatchToFileByHandles(HANDLE patch_file, HANDLE old_file, ULONG apply_flags)
{
    return ApplyPatchToFileByHandles(patch_file, old_file, INVALID_HANDLE_VALUE, apply_flags | APPLY_OPTION_TEST_ONLY);
}

/*****************************************************
 *    TestApplyPatchToFileW (MSPATCHA.12)
 */
BOOL WINAPI TestApplyPatchToFileW(LPCWSTR patch_file, LPCWSTR old_file, ULONG apply_flags)
{
    BOOL ret = FALSE;
    HANDLE hPatch, hOld;

    hPatch = CreateFileW(patch_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
    if (hPatch != INVALID_HANDLE_VALUE)
    {
        hOld = CreateFileW(old_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                           OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        if (hOld != INVALID_HANDLE_VALUE)
        {
            ret = TestApplyPatchToFileByHandles(hPatch, hOld, apply_flags);
            CloseHandle(hOld);
        }
        CloseHandle(hPatch);
    }

    return ret;
}
