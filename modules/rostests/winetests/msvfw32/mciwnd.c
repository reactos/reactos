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
    ok(ret, "Failed to get a temp path, err %ld\n", GetLastError());
    if (!ret)
        return FALSE;

    ret = GetTempFileNameA(temp_path, "mci", 0, fname);
    ok(ret, "Failed to get a temp name, err %ld\n", GetLastError());
    if (!ret)
        return FALSE;
    DeleteFileA(fname);

    lstrcatA(fname, ".avi");

    hFile = CreateFileA(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Failed to create a file, err %ld\n", GetLastError());
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    buffer_length = padding[1];
    buffer = calloc(1, buffer_length);

    WriteFile(hFile, file_header, sizeof(file_header), &written, NULL);
    WriteFile(hFile, &main_avi_header, sizeof(MainAVIHeader), &written, NULL);
    WriteFile(hFile, stream_list, sizeof(stream_list), &written, NULL);
    WriteFile(hFile, &avi_stream_header, sizeof(AVIStreamHeader), &written, NULL);
    WriteFile(hFile, video_stream_format, sizeof(video_stream_format), &written, NULL);
    WriteFile(hFile, padding, sizeof(padding), &written, NULL);
    WriteFile(hFile, buffer, buffer_length, &written, NULL);
    WriteFile(hFile, data, sizeof(data), &written, NULL);

    free(buffer);

    CloseHandle(hFile);
    return ret;
}

/* expected information for window creation */
static struct {
    /* input */
    const char* expectedA;
    const WCHAR* expectedW;
    /* output */
    unsigned open_msg;
    enum {NO_MATCH, PTR_ANSI_MATCH, PTR_UNICODE_MATCH, ANSI_MATCH, UNICODE_MATCH } match;
} wnd_creation;

static WNDPROC old_MCIWndProc;
static LRESULT WINAPI proxy_MCIWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
    {
    case WM_CREATE:
        {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            if (cs->lpCreateParams)
            {
                if (cs->lpCreateParams == wnd_creation.expectedA)
                    wnd_creation.match = PTR_ANSI_MATCH;
                else if (cs->lpCreateParams == wnd_creation.expectedW)
                    wnd_creation.match = PTR_UNICODE_MATCH;
                else if (!strcmp(cs->lpCreateParams, wnd_creation.expectedA))
                    wnd_creation.match = ANSI_MATCH;
                else if (!wcscmp(cs->lpCreateParams, wnd_creation.expectedW))
                    wnd_creation.match = UNICODE_MATCH;
            }
        }
        break;
    case MCIWNDM_OPENA:
    case MCIWNDM_OPENW:
        wnd_creation.open_msg = wMsg;
        break;
    default:
        break;
    }
    return old_MCIWndProc(hWnd, wMsg, wParam, lParam);
}

static LRESULT CALLBACK hook_proc( int code, WPARAM wp, LPARAM lp )
{
    if (code == HCBT_CREATEWND && lp)
    {
        CREATESTRUCTW* cs = ((CBT_CREATEWNDW *)lp)->lpcs;
        if (cs && ((ULONG_PTR)cs->lpszClass >> 16) && !wcsicmp(cs->lpszClass, L"MCIWndClass"))
        {
            old_MCIWndProc = (WNDPROC)SetWindowLongPtrW((HWND)wp, GWLP_WNDPROC, (LPARAM)proxy_MCIWndProc);
            ok(old_MCIWndProc != NULL, "No wnd proc\n");
        }
    }

    return CallNextHookEx( NULL, code, wp, lp );
}

static void test_window_create(unsigned line, const char* fname, HWND parent,
                               BOOL with_mci, BOOL call_ansi, unsigned match)
{
    HMODULE hinst = GetModuleHandleA(NULL);
    WCHAR fnameW[MAX_PATH];
    HWND window;
    char error[200];
    LRESULT ret;
    BOOL expect_ansi = match == PTR_ANSI_MATCH || match == ANSI_MATCH;

    MultiByteToWideChar(CP_ACP, 0, fname, -1, fnameW, ARRAY_SIZE(fnameW));

    wnd_creation.expectedA = fname;
    wnd_creation.expectedW = fnameW;
    wnd_creation.open_msg = 0;
    wnd_creation.match = NO_MATCH;

    if (call_ansi)
    {
        if (with_mci)
            window = MCIWndCreateA(parent, hinst, MCIWNDF_NOERRORDLG, fname);
        else
            window = CreateWindowExA(0, "MCIWndClass", NULL,
                                     WS_CLIPSIBLINGS | WS_CLIPCHILDREN | MCIWNDF_NOERRORDLG,
                                     0, 0, 300, 0,
                                     parent, 0, hinst, expect_ansi ? (LPVOID)fname : (LPVOID)fnameW);
    }
    else
    {
        if (with_mci)
            window = MCIWndCreateW(parent, hinst, MCIWNDF_NOERRORDLG, fnameW);
        else
            window = CreateWindowExW(0, L"MCIWndClass", NULL,
                                     WS_CLIPSIBLINGS | WS_CLIPCHILDREN | MCIWNDF_NOERRORDLG,
                                     0, 0, 300, 0,
                                     parent, 0, hinst, expect_ansi ? (LPVOID)fname : (LPVOID)fnameW);
    }
    ok_(__FILE__, line)(window != NULL, "Failed to create an MCIWnd window\n");
    ok_(__FILE__, line)(wnd_creation.match == match, "unexpected match %u\n", wnd_creation.match);
    ok_(__FILE__, line)((expect_ansi && wnd_creation.open_msg == MCIWNDM_OPENA) ||
                        (!expect_ansi && wnd_creation.open_msg == MCIWNDM_OPENW),
                        "bad open message %u %s%u\n", match,
                        wnd_creation.open_msg >= WM_USER ? "WM_USER+" : "",
                        wnd_creation.open_msg >= WM_USER ? wnd_creation.open_msg - WM_USER : wnd_creation.open_msg);
    ret = SendMessageA(window, MCIWNDM_GETERRORA, sizeof(error), (LPARAM)error);
    ok_(__FILE__, line)(!ret || broken(ret == ERROR_INVALID_HANDLE) /* w2003std, w2008s64 */,
                        "Unexpected error %Id\n", ret);
    DestroyWindow(window);
}

static void test_MCIWndCreate(void)
{
    HWND parent, window;
    HMODULE hinst = GetModuleHandleA(NULL);
    char fname[MAX_PATH];
    char invalid_fname[] = "invalid.avi";
    char error[200];
    HHOOK hook;
    LRESULT ret;

    create_avi_file(fname);

    hook = SetWindowsHookExW( WH_CBT, hook_proc, NULL, GetCurrentThreadId() );

    test_window_create( __LINE__, fname, NULL, TRUE,  TRUE,  UNICODE_MATCH );
    test_window_create( __LINE__, fname, NULL, TRUE,  FALSE, PTR_UNICODE_MATCH );
    test_window_create( __LINE__, fname, NULL, FALSE, TRUE,  PTR_ANSI_MATCH );
    test_window_create( __LINE__, fname, NULL, FALSE, FALSE, PTR_ANSI_MATCH );

    parent = CreateWindowExA(0, "static", "msvfw32 test",
                             WS_POPUP, 0, 0, 100, 100,
                             0, 0, 0, NULL);
    ok(parent != NULL, "Failed to create a window\n");
    ok(!IsWindowUnicode(parent), "Expecting ansi parent window\n");

    test_window_create( __LINE__, fname, parent, TRUE,  TRUE,  UNICODE_MATCH );
    test_window_create( __LINE__, fname, parent, TRUE,  FALSE, PTR_UNICODE_MATCH );
    test_window_create( __LINE__, fname, parent, FALSE, TRUE,  PTR_ANSI_MATCH );
    test_window_create( __LINE__, fname, parent, FALSE, FALSE, PTR_ANSI_MATCH );

    DestroyWindow(parent);

    parent = CreateWindowExW(0, L"static", L"msvfw32 test",
                             WS_POPUP, 0, 0, 100, 100,
                             0, 0, 0, NULL);
    ok(parent != NULL, "Failed to create a window\n");
    ok(IsWindowUnicode(parent), "Expecting unicode parent window\n");

    test_window_create( __LINE__, fname, parent, TRUE,  TRUE,  UNICODE_MATCH );
    test_window_create( __LINE__, fname, parent, TRUE,  FALSE, PTR_UNICODE_MATCH );
    test_window_create( __LINE__, fname, parent, FALSE, TRUE,  PTR_UNICODE_MATCH );
    test_window_create( __LINE__, fname, parent, FALSE, FALSE, PTR_UNICODE_MATCH );

    DestroyWindow(parent);

    UnhookWindowsHookEx( hook );

    window = MCIWndCreateA(NULL, hinst, MCIWNDF_NOERRORDLG, invalid_fname);
    ok(window != NULL, "Failed to create an MCIWnd window\n");

    ret = SendMessageA(window, MCIWNDM_GETERRORA, sizeof(error), (LPARAM)error);
    todo_wine ok(ret == MCIERR_FILE_NOT_FOUND, "Unexpected error %Id\n", ret);

    DestroyWindow(window);

    DeleteFileA(fname);
}

START_TEST(mciwnd)
{
    test_MCIWndCreate();
}
