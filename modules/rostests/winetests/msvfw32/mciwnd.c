/*
 * Unit tests for MCIWnd
 *
 * Copyright 2019 Sven Baars
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

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <vfw.h>

#include "wine/heap.h"
#include "wine/test.h"

static const DWORD file_header[] = /* file_header */
{
    FOURCC_RIFF, 0x8c0 /* file size */,
    formtypeAVI,
    FOURCC_LIST, 0xc0 /* list length */,
    listtypeAVIHEADER, ckidAVIMAINHDR, sizeof(MainAVIHeader),
};

static const MainAVIHeader main_avi_header =
{
    0x0001046b,    /* dwMicroSecPerFrame   */
    0x00000000,    /* dwMaxBytesPerSec     */
    0x00000000,    /* dwPaddingGranularity */
    0x00000810,    /* dwFlags              */
    2,             /* dwTotalFrames        */
    0,             /* dwInitialFrames      */
    1,             /* dwStreams            */
    0x48,          /* dwSuggestedBufferSize*/
    5,             /* dwWidth              */
    5,             /* dwHeight             */
    { 0, 0, 0, 0 } /* dwReserved[4]        */
};

static const DWORD stream_list[] =
{
    FOURCC_LIST, 0x74 /* length */,
    listtypeSTREAMHEADER, ckidSTREAMHEADER, 0x38 /* length */,
};

static const AVIStreamHeader avi_stream_header =
{
    streamtypeVIDEO, /* fccType              */
    0,               /* fccHandler           */
    0,               /* dwFlags              */
    0,               /* wPriority            */
    0,               /* wLanguage            */
    0,               /* dwInitialFrames      */
    1,               /* dwScale              */
    0xf,             /* dwRate               */
    0,               /* dwStart              */
    2,               /* dwLength             */
    0x48,            /* dwSuggestedBufferSize*/
    0,               /* dwQuality            */
    0,               /* dwSampleSize         */
    { 0, 0, 0, 0 }   /* short left right top bottom */
};

static const DWORD video_stream_format[] =
{
    ckidSTREAMFORMAT,
    0x28 /* length */,
    0x28 /* length */,
    5    /* width */,
    5    /* height */,
    0x00180001 ,
    mmioFOURCC('c', 'v', 'i', 'd'),
    0x245a,
    0, 0, 0, 0,
};

static const DWORD padding[] =
{
    ckidAVIPADDING, 0x718 /* length */,
};

static const DWORD data[] =
{
    FOURCC_LIST, 0xa4 /* length */, listtypeAVIMOVIE,
    mmioFOURCC('0', '0', 'd', 'b'), 0x48, 0x48000000, 0x08000800,
    0x00100100, 0x00003e00, 0x08000000, 0x00200800,
    0x00001600, 0x00000000, 0x00003a00, 0x22e306f9,
    0xfc120000, 0x0a000022, 0x00000000, 0x00300000,
    0x00c01200, 0x00000000, 0x02010000, 0x00000000,
    mmioFOURCC('0', '0', 'd', 'b'), 0x48, 0x48000000, 0x08000800,
    0x00100100, 0x00003e00, 0x08000000, 0x00200800,
    0x00001600, 0x00000000, 0x00003a00, 0x22e306f9,
    0xfc120000, 0x0a000022, 0x00000000, 0x00300000,
    0x00c01200, 0x00000000, 0x02010000, 0x00000000,
    mmioFOURCC('i', 'd', 'x', '1'), 0x20, mmioFOURCC('0', '0', 'd', 'b'), 0x10,
    0x04, 0x48, mmioFOURCC('0', '0', 'd', 'b'), 0x10,
    0x54, 0x48,
};

static BOOL create_avi_file(char *fname)
{
    HANDLE hFile;
    DWORD written;
    char temp_path[MAX_PATH];
    BOOL ret;
    BYTE *buffer;
    ULONG buffer_length;

    ret = GetTempPathA(sizeof(temp_path), temp_path);
    ok(ret, "Failed to get a temp path, err %d\n", GetLastError());
    if (!ret)
        return FALSE;

    ret = GetTempFileNameA(temp_path, "mci", 0, fname);
    ok(ret, "Failed to get a temp name, err %d\n", GetLastError());
    if (!ret)
        return FALSE;
    DeleteFileA(fname);

    lstrcatA(fname, ".avi");

    hFile = CreateFileA(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Failed to create a file, err %d\n", GetLastError());
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    buffer_length = padding[1];
    buffer = heap_alloc_zero(buffer_length);

    WriteFile(hFile, file_header, sizeof(file_header), &written, NULL);
    WriteFile(hFile, &main_avi_header, sizeof(MainAVIHeader), &written, NULL);
    WriteFile(hFile, stream_list, sizeof(stream_list), &written, NULL);
    WriteFile(hFile, &avi_stream_header, sizeof(AVIStreamHeader), &written, NULL);
    WriteFile(hFile, video_stream_format, sizeof(video_stream_format), &written, NULL);
    WriteFile(hFile, padding, sizeof(padding), &written, NULL);
    WriteFile(hFile, buffer, buffer_length, &written, NULL);
    WriteFile(hFile, data, sizeof(data), &written, NULL);

    heap_free(buffer);

    CloseHandle(hFile);
    return ret;
}

static void test_MCIWndCreate(void)
{
    HWND parent, window;
    HMODULE hinst = GetModuleHandleA(NULL);
    char fname[MAX_PATH];
    char invalid_fname[] = "invalid.avi";
    char error[200];
    LRESULT ret;

    create_avi_file(fname);

    window = MCIWndCreateA(NULL, hinst, MCIWNDF_NOERRORDLG, fname);
    ok(window != NULL, "Failed to create an MCIWnd window without parent\n");

    ret = SendMessageA(window, MCIWNDM_GETERRORA, sizeof(error), (LPARAM)error);
    ok(!ret || broken(ret == ERROR_INVALID_HANDLE) /* w2003std, w2008s64 */,
       "Unexpected error %ld\n", ret);

    DestroyWindow(window);

    parent = CreateWindowExA(0, "static", "msvfw32 test",
                             WS_POPUP, 0, 0, 100, 100,
                             0, 0, 0, NULL);
    ok(parent != NULL, "Failed to create a window\n");
    window = MCIWndCreateA(parent, hinst, MCIWNDF_NOERRORDLG, fname);
    ok(window != NULL, "Failed to create an MCIWnd window\n");

    ret = SendMessageA(window, MCIWNDM_GETERRORA, sizeof(error), (LPARAM)error);
    ok(!ret || broken(ret == ERROR_INVALID_HANDLE) /* w2003std, w2008s64 */,
       "Unexpected error %ld\n", ret);

    DestroyWindow(parent);

    window = MCIWndCreateA(NULL, hinst, MCIWNDF_NOERRORDLG, invalid_fname);
    ok(window != NULL, "Failed to create an MCIWnd window\n");

    ret = SendMessageA(window, MCIWNDM_GETERRORA, sizeof(error), (LPARAM)error);
    todo_wine ok(ret == MCIERR_FILE_NOT_FOUND, "Unexpected error %ld\n", ret);

    DestroyWindow(window);

    DeleteFileA(fname);
}

START_TEST(mciwnd)
{
    test_MCIWndCreate();
}
