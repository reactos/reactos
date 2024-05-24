/*
 * PatchAPI
 *
 * Copyright 2011 David Hedberg for CodeWeavers
 * Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
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
#include "lzx.h"
#include "wine/debug.h"
#include <stdlib.h>
#include "pa19.h"

static const char szHexString[] = "0123456789abcdef";
#define SIGNATURE_MIN_SIZE          9

#define UNKNOWN_FLAGS_COMBINATION   0x00c40001


WINE_DEFAULT_DEBUG_CHANNEL(mspatcha);


typedef struct _SAFE_READ
{
    PBYTE Root;
    DWORD Size;
    PBYTE Ptr;
} SAFE_READ, *PSAFE_READ;


/**
 * @name ReadByte
 * Read the next byte available from @param pRead
 *
 * @param pRead
 * The input buffer
 *
 * @return The byte, or 0
 */
BYTE ReadByte(PSAFE_READ pRead)
{
    if (pRead->Ptr + sizeof(BYTE) <= (pRead->Root + pRead->Size))
    {
        BYTE Value = *(PBYTE)pRead->Ptr;
        pRead->Ptr += sizeof(BYTE);
        return Value;
    }
    pRead->Ptr = pRead->Root + pRead->Size;
    return 0;
}

/**
 * @name ReadUShort
 * Read the next unsigned short available from @param pRead
 *
 * @param pRead
 * The input buffer
 *
 * @return The unsigned short, or 0
 */
USHORT ReadUShort(PSAFE_READ pRead)
{
    if (pRead->Ptr + sizeof(USHORT) <= (pRead->Root + pRead->Size))
    {
        USHORT Value = *(PUSHORT)pRead->Ptr;
        pRead->Ptr += sizeof(USHORT);
        return Value;
    }
    pRead->Ptr = pRead->Root + pRead->Size;
    return 0;
}

/**
 * @name ReadDWord
 * Read the next dword available from @param pRead
 *
 * @param pRead
 * The input buffer
 *
 * @return The dword, or 0
 */
DWORD ReadDWord(PSAFE_READ pRead)
{
    if (pRead->Ptr + sizeof(DWORD) <= (pRead->Root + pRead->Size))
    {
        DWORD Value = *(PDWORD)pRead->Ptr;
        pRead->Ptr += sizeof(DWORD);
        return Value;
    }
    pRead->Ptr = pRead->Root + pRead->Size;
    return 0;
}

/**
 * @name DecodeDWord
 * Read the next variable length-encoded dword from @param pRead
 *
 * @param pRead
 * The input buffer
 *
 * @return The dword, or 0
 */
DWORD DecodeDWord(PSAFE_READ pRead)
{
    UINT Result = 0, offset;

    for (offset = 0; offset < 32; offset += 7)
    {
        DWORD val = ReadByte(pRead);
        Result |= ((val & 0x7f) << offset);
        if (val & 0x80)
            break;
    }

    return Result;
}


/**
 * @name DecodeInt
 * Read the next variable length-encoded int from @param pRead
 *
 * @param pRead
 * The input buffer
 *
 * @return The int, or 0
 */
INT DecodeInt(PSAFE_READ pRead)
{
    INT Result = 0, offset;

    for (offset = 0; offset < 32; offset += 6)
    {
        INT val = (INT)(DWORD)ReadByte(pRead);
        Result |= ((val & 0x3f) << offset);
        if (val & 0x80)
        {
            if (val & 0x40)
                Result *= -1;
            break;
        }
    }

    return Result;
}


typedef struct _PATCH_HEADER
{
    DWORD Flags;

    DWORD ImageBase;
    DWORD ImageTimeStamp;

    DWORD OutputSize;
    DWORD OutputCrc;

    DWORD OldSize;
    DWORD OldCrc;
    DWORD DataSize;  // Payload after the patch header

} PATCH_HEADER;


/**
 * @name MapFile
 * Map a view of a file into readonly memory
 *
 * @param hFile
 * The input file handle, readable
 *
 * @param dwSize
 * Mapped file size (out)
 *
 * @return A Pointer to the start of the memory
 */
static PBYTE MapFile(HANDLE hFile, DWORD* dwSize)
{
    HANDLE hMap;
    PVOID pView;

    *dwSize = GetFileSize(hFile, NULL);
    hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap != INVALID_HANDLE_VALUE)
    {
        pView = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(hMap);
        return pView;
    }

    return NULL;
}


/*****************************************************
 *    DllMain (MSPATCHA.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
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


/**
 * @name ParseHeader
 * Parse a Patch file header
 * @note The current implementation is far from complete!
 *
 * @param Patch
 * Buffer pointing to the raw patch data
 *
 * @param Header
 * The result of the parsed header
 *
 * @return STATUS_SUCCESS on success, an error code otherwise
 */
DWORD ParseHeader(SAFE_READ* Patch, PATCH_HEADER* Header)
{
    DWORD Crc, Unknown;
    int Delta;

    ZeroMemory(Header, sizeof(*Header));

    /* Validate the patch */
    Crc = RtlComputeCrc32(0, Patch->Root, Patch->Size);
    if (Crc != ~0)
        return ERROR_PATCH_CORRUPT;

    if (ReadDWord(Patch) != '91AP')
        return ERROR_PATCH_DECODE_FAILURE;

    /* Read the flags, warn about an unknown combination */
    Header->Flags = ReadDWord(Patch);
    if (Header->Flags ^ UNKNOWN_FLAGS_COMBINATION)
        ERR("Unknown flags: 0x%x, patch will most likely fail\n", Header->Flags ^ UNKNOWN_FLAGS_COMBINATION);

    /* 0x5bb3284e, 0x5bb33562, 0x5bb357b1 */
    Unknown = ReadDWord(Patch);
    TRACE("Unknown: 0x%x\n", Unknown);

    Header->OutputSize = DecodeDWord(Patch);
    Header->OutputCrc = ReadDWord(Patch);

    Unknown = ReadByte(Patch);
    if (Unknown != 1)
        ERR("Field after CRC is not 1 but %u\n", Unknown);

    Delta = DecodeInt(Patch);
    Header->OldSize = Header->OutputSize + Delta;
    Header->OldCrc = ReadDWord(Patch);

    Unknown = ReadUShort(Patch);
    if (Unknown != 0)
        ERR("Field1 after OldCrc is not 0 but %u\n", Unknown);

    Unknown = DecodeDWord(Patch);
    if (Unknown != 0)
        ERR("Field2 after OldCrc is not 0 but %u\n", Unknown);

    Header->DataSize = DecodeDWord(Patch);
                            /* Remaining data, minus the CRC appended */
    if (Header->DataSize != (Patch->Size - (Patch->Ptr - Patch->Root) - sizeof(DWORD)))
    {
        ERR("Unable to read header, check previous logging!\n");
        return ERROR_PATCH_DECODE_FAILURE;
    }
    return STATUS_SUCCESS;
}

/**
 * @name CreateNewFileFromPatch
 * Using the input @param Header and @param Patch, create a new file on @param new_file
 *
 * @param Header
 * Parsed / preprocessed patch header
 *
 * @param Patch
 * Memory buffer pointing to the patch payload
 *
 * @param new_file
 * A handle to the output file. This file will be resized
 *
 * @return STATUS_SUCCESS on success, an error code otherwise
 */
DWORD CreateNewFileFromPatch(PATCH_HEADER* Header, SAFE_READ* Patch, HANDLE new_file)
{
    SAFE_READ NewFile;
    HANDLE hMap;
    USHORT BlockSize;
    DWORD dwStatus;
    struct LZXstate* state;
    int lzxResult;

    hMap = CreateFileMappingW(new_file, NULL, PAGE_READWRITE, 0, Header->OutputSize, NULL);
    if (hMap == INVALID_HANDLE_VALUE)
        return ERROR_PATCH_NOT_AVAILABLE;

    NewFile.Root = NewFile.Ptr = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, 0);
    CloseHandle(hMap);
    NewFile.Size = Header->OutputSize;

    if (!NewFile.Root)
        return ERROR_PATCH_NOT_AVAILABLE;

    /* At this point Patch->Ptr should point to the payload */
    BlockSize = ReadUShort(Patch);

    /* This window size does not work on all files (for example, MS SQL Express 2008 setup) */
    state = LZXinit(17);
    if (state)
    {
        lzxResult = LZXdecompress(state, Patch->Ptr, NewFile.Ptr, BlockSize, NewFile.Size);
        LZXteardown(state);

        if (lzxResult == DECR_OK)
            dwStatus = STATUS_SUCCESS;
        else
            dwStatus = ERROR_PATCH_DECODE_FAILURE;
    }
    else
    {
        dwStatus = ERROR_INSUFFICIENT_BUFFER;
    }

    UnmapViewOfFile(NewFile.Root);
    return dwStatus;
}


/*****************************************************
 *    ApplyPatchToFileByHandles (MSPATCHA.2)
 */
BOOL WINAPI ApplyPatchToFileByHandles(HANDLE patch_file, HANDLE old_file, HANDLE new_file, ULONG apply_flags)
{
    SAFE_READ Patch, OldFile;
    DWORD dwStatus;
    PATCH_HEADER Header;

    Patch.Root = Patch.Ptr = MapFile(patch_file, &Patch.Size);
    if (!Patch.Root)
    {
        SetLastError(ERROR_PATCH_CORRUPT);
        return FALSE;
    }

    /* Let's decode the header */
    dwStatus = ParseHeader(&Patch, &Header);
    if (dwStatus != STATUS_SUCCESS)
    {
        UnmapViewOfFile(Patch.Root);
        SetLastError(dwStatus);
        return FALSE;
    }

    OldFile.Root = OldFile.Ptr = MapFile(old_file, &OldFile.Size);
    if (OldFile.Root)
    {
        DWORD dwCrc;

        /* Verify the input file */
        dwCrc = RtlComputeCrc32(0, OldFile.Root, OldFile.Size);
        if (OldFile.Size == Header.OldSize && dwCrc == Header.OldCrc)
        {
            if (apply_flags & APPLY_OPTION_TEST_ONLY)
                dwStatus = STATUS_SUCCESS;
            else
                dwStatus = CreateNewFileFromPatch(&Header, &Patch, new_file);
        }
        else
        {
            dwStatus = ERROR_PATCH_WRONG_FILE;
        }
        UnmapViewOfFile(OldFile.Root);
    }
    else
    {
        dwStatus = GetLastError();
        if (dwStatus == STATUS_SUCCESS)
            dwStatus = ERROR_PATCH_NOT_AVAILABLE;
    }

    UnmapViewOfFile(Patch.Root);
    SetLastError(dwStatus);
    return dwStatus == STATUS_SUCCESS;
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
    DWORD dwSize, ulCrc;
    PBYTE pData;

    if (flags)
        FIXME("Unhandled flags 0x%x\n", flags);
    if (ignore_range_count)
        FIXME("Unhandled ignore_range_count %u\n", ignore_range_count);
    if (retain_range_count)
        FIXME("Unhandled ignore_range_count %u\n", retain_range_count);

    if ((pData = MapFile(hFile, &dwSize)))
    {
        if (dwSize >= 2 && *(PWORD)pData == IMAGE_DOS_SIGNATURE)
        {
            FIXME("Potentially unimplemented case, normalized signature\n");
        }

        ulCrc = RtlComputeCrc32(0, pData, dwSize);
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
        UnmapViewOfFile(pData);

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

static inline WCHAR *strdupAW(const char *src)
{
    int len;
    WCHAR *dst;
    if (!src) return NULL;
    len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
    if ((dst = malloc(len * sizeof(*dst)))) MultiByteToWideChar(CP_ACP, 0, src, -1, dst, len);
    return dst;
}

BOOL WINAPI ApplyPatchToFileExA(LPCSTR patch_file, LPCSTR old_file, LPCSTR new_file, ULONG apply_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID progress_ctx)
{
    BOOL ret = FALSE;
    WCHAR *patch_fileW, *new_fileW, *old_fileW = NULL;

    if (!(patch_fileW = strdupAW(patch_file))) return FALSE;

    if (old_file && !(old_fileW = strdupAW(old_file)))
        goto free_wstrs;
    if (!(new_fileW = strdupAW(new_file)))
        goto free_wstrs;

    ret = apply_patch_to_file(patch_fileW, old_fileW, new_fileW, apply_flags, progress_fn, progress_ctx, FALSE);

    HeapFree(GetProcessHeap(), 0, new_fileW);
free_wstrs:
    HeapFree(GetProcessHeap(), 0, patch_fileW);
    HeapFree(GetProcessHeap(), 0, old_fileW);
    return ret;
}

/*****************************************************
 *    ApplyPatchToFileExW (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileExW(LPCWSTR patch_file_name, LPCWSTR old_file_name, LPCWSTR new_file_name,
    ULONG apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID progress_ctx)
{
    return apply_patch_to_file(patch_file_name, old_file_name, new_file_name, apply_option_flags,
        progress_fn, progress_ctx, FALSE);
}

/*****************************************************
 *    ApplyPatchToFileByHandlesEx (MSPATCHA.@)
 */
BOOL WINAPI ApplyPatchToFileByHandlesEx(HANDLE patch_file_hndl, HANDLE old_file_hndl, HANDLE new_file_hndl,
    ULONG apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn,
    PVOID progress_ctx)
{
    return apply_patch_to_file_by_handles(patch_file_hndl, old_file_hndl, new_file_hndl,
        apply_option_flags, progress_fn, progress_ctx, FALSE);
}

BOOL WINAPI ApplyPatchToFileByBuffers(PBYTE patch_file_view, ULONG  patch_file_size,
    PBYTE  old_file_view, ULONG  old_file_size,
    PBYTE* new_file_buf, ULONG  new_file_buf_size, ULONG* new_file_size,
    FILETIME* new_file_time,
    ULONG  apply_option_flags,
    PPATCH_PROGRESS_CALLBACK progress_fn, PVOID  progress_ctx)
 {
    /* NOTE: windows preserves last error on success for this function, but no apps are known to depend on it */

    DWORD err = apply_patch_to_file_by_buffers(patch_file_view, patch_file_size,
        old_file_view, old_file_size,
        new_file_buf, new_file_buf_size, new_file_size, new_file_time,
        apply_option_flags,
        progress_fn, progress_ctx,
        FALSE);

    SetLastError(err);

    return err == ERROR_SUCCESS;
}

BOOL WINAPI TestApplyPatchToFileByBuffers(BYTE *patch_file_buf, ULONG patch_file_size,
    BYTE *old_file_buf, ULONG old_file_size,
    ULONG* new_file_size,
    ULONG  apply_option_flags)
{
    /* NOTE: windows preserves last error on success for this function, but no apps are known to depend on it */

    DWORD err = apply_patch_to_file_by_buffers(patch_file_buf, patch_file_size,
        old_file_buf, old_file_size,
        NULL, 0, new_file_size, NULL,
        apply_option_flags,
        NULL, NULL,
        TRUE);

    SetLastError(err);

    return err == ERROR_SUCCESS;
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
