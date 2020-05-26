/*
 * Unit tests for video playback
 *
 * Copyright 2008,2010 Jörg Höhle
 * Copyright 2008 Austin English
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

static inline int get_stride(int width, int depth)
{
    return ((depth * width + 31) >> 3) & ~3;
}

static void test_OpenCase(void)
{
    HIC h;
    ICINFO info;
    /* Check if default handler works */
    h = ICOpen(mmioFOURCC('v','i','d','c'),0,ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.0) failed\n");
    if (h) {
        info.dwSize = sizeof(info);
        info.szName[0] = 0;
        ICGetInfo(h, &info, sizeof(info));
        trace("The default decompressor is %s\n", wine_dbgstr_w(info.szName));
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('v','i','d','c'),0,ICMODE_COMPRESS);
    ok(0!=h || broken(h == 0),"ICOpen(vidc.0) failed\n");  /* Not present in Win8 */
    if (h) {
        info.dwSize = sizeof(info);
        info.szName[0] = 0;
        ICGetInfo(h, &info, sizeof(info));
        trace("The default compressor is %s\n", wine_dbgstr_w(info.szName));
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }

    /* Open a compressor with combinations of lowercase
     * and uppercase compressortype and handler.
     */
    h = ICOpen(mmioFOURCC('v','i','d','c'),mmioFOURCC('m','s','v','c'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.msvc) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('v','i','d','c'),mmioFOURCC('M','S','V','C'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.MSVC) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('V','I','D','C'),mmioFOURCC('m','s','v','c'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(VIDC.msvc) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('V','I','D','C'),mmioFOURCC('M','S','V','C'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(VIDC.MSVC) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('v','i','d','c'),mmioFOURCC('m','S','v','C'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vidc.mSvC) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
    h = ICOpen(mmioFOURCC('v','I','d','C'),mmioFOURCC('m','s','v','c'),ICMODE_DECOMPRESS);
    ok(0!=h,"ICOpen(vIdC.msvc) failed\n");
    if (h) {
        ok(ICClose(h)==ICERR_OK,"ICClose failed\n");
    }
}

static void test_Locate(void)
{
    static BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER),32,8, 1,8, BI_RLE8, 0,100000,100000, 0,0};
    static BITMAPINFOHEADER bo = {sizeof(BITMAPINFOHEADER),32,8, 1,8, BI_RGB, 0,100000,100000, 0,0};
    BITMAPINFOHEADER tmp = {sizeof(BITMAPINFOHEADER)};
    HIC h;
    DWORD err;

    /* Oddly, MSDN documents that ICLocate takes BITMAPINFOHEADER
     * pointers, while ICDecompressQuery takes the larger
     * BITMAPINFO.  Probably it's all the same as long as the
     * variable length color quads are present when they are
     * needed. */

    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h != 0, "RLE8->RGB failed\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");

    bo.biHeight = - bo.biHeight;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h == 0, "RLE8->RGB height<0 succeeded\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
    bo.biHeight = - bo.biHeight;

    bi.biCompression = mmioFOURCC('c','v','i','d'); /* Cinepak */
    h = ICOpen(ICTYPE_VIDEO, mmioFOURCC('c','v','i','d'), ICMODE_DECOMPRESS);
    if (h == 0) win_skip("Cinepak/ICCVID codec not found\n");
    else {
        bo.biBitCount = bi.biBitCount = 32;
        err = ICDecompressQuery(h, &bi, &bo);
        ok(err == ICERR_OK, "Query cvid->RGB32: %d\n", err);

        err = ICDecompressQuery(h, &bi, NULL);
        ok(err == ICERR_OK, "Query cvid 32: %d\n", err);

        bo.biHeight = -bo.biHeight;
        err = ICDecompressQuery(h, &bi, &bo);
        ok(err == ICERR_OK, "Query cvid->RGB32 height<0: %d\n", err);
        bo.biHeight = -bo.biHeight;

        bi.biWidth = 17;

        bi.biBitCount = 8;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query cvid output format: %d\n", err);
        ok(tmp.biBitCount == 24, "Expected 24 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biSizeImage == get_stride(17, 24) * 8, "Expected size %d, got %d\n",
           get_stride(17, 24) * 8, tmp.biSizeImage);

        bi.biBitCount = 15;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query cvid output format: %d\n", err);
        ok(tmp.biBitCount == 24, "Expected 24 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biSizeImage == get_stride(17, 24) * 8, "Expected size %d, got %d\n",
           get_stride(17, 24) * 8, tmp.biSizeImage);

        bi.biBitCount = 16;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query cvid output format: %d\n", err);
        ok(tmp.biBitCount == 24, "Expected 24 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biSizeImage == get_stride(17, 24) * 8, "Expected size %d, got %d\n",
           get_stride(17, 24) * 8, tmp.biSizeImage);

        bi.biBitCount = 24;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query cvid output format: %d\n", err);
        ok(tmp.biBitCount == 24, "Expected 24 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biSizeImage == get_stride(17, 24) * 8, "Expected size %d, got %d\n",
           get_stride(17, 24) * 8, tmp.biSizeImage);

        bi.biBitCount = 32;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query cvid output format: %d\n", err);
        ok(tmp.biBitCount == 24, "Expected 24 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biSizeImage == get_stride(17, 24) * 8, "Expected size %d, got %d\n",
           get_stride(17, 24) * 8, tmp.biSizeImage);

        bi.biWidth = 32;

        ok(ICClose(h) == ICERR_OK,"ICClose failed\n");

        bo.biBitCount = bi.biBitCount = 8;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        todo_wine ok(h != 0, "cvid->RGB8 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        todo_wine ok(h != 0, "cvid->RGB8 height<0 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;

        bo.biBitCount = bi.biBitCount = 16;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        ok(h != 0, "cvid->RGB16 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        ok(h != 0, "cvid->RGB16 height<0 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;

        bo.biBitCount = bi.biBitCount = 32;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        ok(h != 0, "cvid->RGB32 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        ok(h != 0, "cvid->RGB32 height<0 failed\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
        bo.biHeight = - bo.biHeight;

        bi.biCompression = mmioFOURCC('C','V','I','D');
        /* Unlike ICOpen, upper case fails with ICLocate. */
        h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
        ok(h == 0, "CVID->RGB32 upper case succeeded\n");
        if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
    }

    bi.biCompression = mmioFOURCC('M','S','V','C'); /* MS Video 1 */

    bo.biBitCount = bi.biBitCount = 16;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h != 0, "MSVC->RGB16 failed\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");

    bo.biHeight = - bo.biHeight;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    todo_wine ok(h != 0, "MSVC->RGB16 height<0 failed\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
    bo.biHeight = - bo.biHeight;

    bo.biHeight--;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h == 0, "MSVC->RGB16 height too small succeeded\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
    bo.biHeight++;

    /* ICLocate wants upper case MSVC */
    bi.biCompression = mmioFOURCC('m','s','v','c');
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h == 0, "msvc->RGB16 succeeded\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");

    bi.biCompression = mmioFOURCC('M','S','V','C');
    h = ICOpen(ICTYPE_VIDEO, mmioFOURCC('M','S','V','C'), ICMODE_DECOMPRESS);
    ok(h != 0, "No MSVC codec installed!?\n");
    if (h != 0) {
        err = ICDecompressQuery(h, &bi, &bo);
        ok(err == ICERR_OK, "Query MSVC->RGB16: %d\n", err);

        err = ICDecompressQuery(h, &bi, NULL);
        ok(err == ICERR_OK, "Query MSVC 16: %d\n", err);

        bo.biHeight = -bo.biHeight;
        err = ICDecompressQuery(h, &bi, &bo);
        todo_wine ok(err == ICERR_OK, "Query MSVC->RGB16 height<0: %d\n", err);
        bo.biHeight = -bo.biHeight;

        bo.biBitCount = 24;
        err = ICDecompressQuery(h, &bi, &bo);
        ok(err == ICERR_OK, "Query MSVC 16->24: %d\n", err);
        bo.biBitCount = 16;

        bi.biWidth = 553;

        bi.biBitCount = 8;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query MSVC output format: %d\n", err);
        ok(tmp.biBitCount == 8, "Expected 8 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biWidth == 552, "Expected width 552, got %d\n", tmp.biWidth);
        ok(tmp.biSizeImage == get_stride(552, 8) * 8, "Expected size %d, got %d\n",
           get_stride(552, 8) * 8, tmp.biSizeImage);

        bi.biBitCount = 15;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_BADFORMAT, "Query MSVC output format: %d\n", err);

        bi.biBitCount = 16;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query MSVC output format: %d\n", err);
        ok(tmp.biBitCount == 16, "Expected 16 bit, got %d bit\n", tmp.biBitCount);
        ok(tmp.biWidth == 552, "Expected width 552, got %d\n", tmp.biWidth);
        ok(tmp.biSizeImage == get_stride(552, 16) * 8, "Expected size %d, got %d\n",
           get_stride(552, 16) * 8, tmp.biSizeImage);

        bi.biBitCount = 24;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_BADFORMAT, "Query MSVC output format: %d\n", err);

        bi.biBitCount = 32;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_BADFORMAT, "Query MSVC output format: %d\n", err);

        bi.biHeight = 17;
        bi.biBitCount = 8;
        err = ICDecompressGetFormat(h, &bi, &tmp);
        ok(err == ICERR_OK, "Query MSVC output format: %d\n", err);
        ok(tmp.biHeight == 16, "Expected height 16, got %d\n", tmp.biHeight);
        bi.biHeight = 8;

        bi.biWidth = 32;

        bi.biCompression = mmioFOURCC('m','s','v','c');
        err = ICDecompressQuery(h, &bi, &bo);
        ok(err == ICERR_BADFORMAT, "Query msvc->RGB16: %d\n", err);

        ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
    }

    bi.biCompression = BI_RGB;
    bo.biBitCount = bi.biBitCount = 8;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h != 0, "RGB8->RGB identity failed\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");

    bi.biCompression = BI_RLE8;
    h = ICLocate(ICTYPE_VIDEO, 0, &bi, &bo, ICMODE_DECOMPRESS);
    ok(h != 0, "RLE8->RGB again failed\n");
    if (h) ok(ICClose(h) == ICERR_OK,"ICClose failed\n");
}

static void test_ICSeqCompress(void)
{
    /* The purpose of this test is to validate sequential frame compressing
     * functions. The MRLE codec will be used because Wine supports it and
     * it is present in any Windows.
     */
    HIC h;
    DWORD err, vidc = mmioFOURCC('v','i','d','c'), mrle = mmioFOURCC('m', 'r', 'l', 'e');
    DWORD i;
    LONG frame_len;
    BOOL key_frame, ret;
    char *frame;
    COMPVARS pc;
    struct { BITMAPINFOHEADER header; RGBQUAD map[256]; }
    input_header = { {sizeof(BITMAPINFOHEADER), 32, 1, 1, 8, 0, 32*8, 0, 0, 256, 256},
                     {{255,0,0}, {0,255,0}, {0,0,255}, {255,255,255}}};
    PBITMAPINFO bitmap = (PBITMAPINFO) &input_header;
    static BYTE input[32] = {1,2,3,3,3,3,2,3,1};
    static const BYTE output_kf[] = {1,1,1,2,4,3,0,3,2,3,1,0,23,0,0,0,0,1}, /* key frame*/
                      output_nkf[] = {0,0,0,1}; /* non key frame */

    h = ICOpen(vidc, mrle, ICMODE_COMPRESS);
    ok(h != NULL, "Expected non-NULL\n");

    pc.cbSize = sizeof(pc);
    pc.dwFlags    = ICMF_COMPVARS_VALID;
    pc.fccType    = vidc;
    pc.fccHandler = mrle;
    pc.hic        = h;
    pc.lpbiIn     = NULL;
    pc.lpbiOut    = NULL;
    pc.lpBitsOut  = pc.lpBitsPrev = pc.lpState = NULL;
    pc.lQ         = ICQUALITY_DEFAULT;
    pc.lKey       = 1;
    pc.lDataRate  = 300;
    pc.lpState    = NULL;
    pc.cbState    = 0;

    ret = ICSeqCompressFrameStart(&pc, bitmap);
    ok(ret == TRUE, "Expected TRUE\n");
    /* Check that reserved pointers were allocated */
    ok(pc.lpbiIn != NULL, "Expected non-NULL\n");
    ok(pc.lpbiOut != NULL, "Expected non-NULL\n");

    for(i = 0; i < 9; i++)
    {
        frame_len = 0;
        frame = ICSeqCompressFrame(&pc, 0, input, &key_frame, &frame_len);
        ok(frame != NULL, "Frame[%d]: Expected non-NULL\n", i);
        if (frame_len == sizeof(output_nkf))
            ok(!memcmp(output_nkf, frame, frame_len), "Frame[%d]: Contents do not match\n", i);
        else if (frame_len == sizeof(output_kf))
            ok(!memcmp(output_kf, frame, frame_len), "Frame[%d]: Contents do not match\n", i);
        else
            ok(0, "Unknown frame size of %d byten\n", frame_len);
    }

    ICSeqCompressFrameEnd(&pc);
    ICCompressorFree(&pc);
    /* ICCompressorFree already closed the HIC */
    err = ICClose(h);
    ok(err == ICERR_BADHANDLE, "Expected -8, got %d\n", err);
}

static ICINFO enum_info;

static LRESULT CALLBACK enum_driver_proc(DWORD_PTR id, HDRVR driver, UINT msg,
        LPARAM lparam1, LPARAM lparam2)
{
    ICINFO *info = (ICINFO *)lparam1;

    ok(!id, "Got unexpected id %#lx.\n", id);
    ok(msg == ICM_GETINFO, "Got unexpected message %#x.\n", msg);
    ok(info == &enum_info, "Expected lparam1 %p, got %p.\n", &enum_info, info);
    ok(lparam2 == sizeof(ICINFO), "Got lparam2 %ld.\n", lparam2);

    ok(!info->fccType, "Got unexpected type %#x.\n", info->fccType);
    ok(!info->fccHandler, "Got unexpected handler %#x.\n", info->fccHandler);
    ok(!info->dwFlags, "Got unexpected flags %#x.\n", info->dwFlags);
    ok(!info->dwVersion, "Got unexpected version %#x.\n", info->dwVersion);
    ok(info->dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", info->dwVersionICM);
    ok(!info->szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(info->szName));
    ok(!info->szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(info->szDescription));
    ok(!info->szDriver[0], "Got unexpected driver %s.\n", wine_dbgstr_w(info->szDriver));

    info->dwVersion = 0xdeadbeef;
    return sizeof(ICINFO);
}

static void test_ICInfo(void)
{
    static const WCHAR bogusW[] = {'b','o','g','u','s',0};
    static const DWORD test_type = mmioFOURCC('w','i','n','e');
    static const DWORD test_handler = mmioFOURCC('t','e','s','t');
    DWORD i = 0, found = 0;
    char buffer[MAX_PATH];
    ICINFO info, info2;
    unsigned char *fcc;
    DWORD size;
    BOOL ret;
    HKEY key;
    LONG res;

    for (;;)
    {
        memset(&info, 0x55, sizeof(info));
        info.dwSize = sizeof(info);
        if (!ICInfo(0, i++, &info))
            break;
        trace("Codec name: %s, fccHandler: 0x%08x\n", wine_dbgstr_w(info.szName), info.fccHandler);
        ok(info.fccType, "Expected nonzero type.\n");
        ok(info.fccHandler, "Expected nonzero handler.\n");
        ok(!info.dwFlags, "Got unexpected flags %#x.\n", info.dwFlags);
        ok(!info.dwVersion, "Got unexpected version %#x.\n", info.dwVersion);
        ok(info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", info.dwVersionICM);
        ok(!info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szName));
        ok(!info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szDescription));

        ok(ICInfo(info.fccType, info.fccHandler, &info2),
           "ICInfo failed on fcc 0x%08x\n", info.fccHandler);

        fcc = (unsigned char *)&info.fccHandler;
        if (!isalpha(fcc[0])) continue;

        found++;
        /* Test getting info with a different case - bug 41602 */
        fcc[0] ^= 0x20;
        ok(ICInfo(info.fccType, info.fccHandler, &info2),
           "ICInfo failed on fcc 0x%08x\n", info.fccHandler);
    }
    ok(found != 0, "expected at least one codec\n");

    memset(&info, 0x55, sizeof(info));
    info.dwSize = sizeof(info);
    ok(!ICInfo(ICTYPE_VIDEO, mmioFOURCC('f','a','k','e'), &info), "expected failure\n");
    ok(info.fccType == ICTYPE_VIDEO, "got 0x%08x\n", info.fccType);
    ok(info.fccHandler == mmioFOURCC('f','a','k','e'), "got 0x%08x\n", info.fccHandler);
    ok(!info.dwFlags, "Got unexpected flags %#x.\n", info.dwFlags);
    ok(!info.dwVersion, "Got unexpected version %#x.\n", info.dwVersion);
    ok(info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", info.dwVersionICM);
    ok(!info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szName));
    ok(!info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szDescription));
    ok(!info.szDriver[0], "Got unexpected driver %s.\n", wine_dbgstr_w(info.szDriver));

    if (!RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT"
            "\\CurrentVersion\\Drivers32", 0, KEY_ALL_ACCESS, &key))
    {
        ret = ICInstall(test_type, test_handler, (LPARAM)"bogus", NULL, ICINSTALL_DRIVER);
        ok(ret, "Failed to install driver.\n");

        size = sizeof(buffer);
        res = RegQueryValueExA(key, "wine.test", NULL, NULL, (BYTE *)buffer, &size);
        ok(!res, "Failed to query value, error %d.\n", res);
        ok(!strcmp(buffer, "bogus"), "Got unexpected value \"%s\".\n", buffer);

        memset(&info, 0x55, sizeof(info));
        info.dwSize = sizeof(info);
        ok(ICInfo(test_type, test_handler, &info), "Expected success.\n");
        ok(info.fccType == test_type, "Got unexpected type %#x.\n", info.fccType);
        ok(info.fccHandler == test_handler, "Got unexpected handler %#x.\n", info.fccHandler);
        ok(!info.dwFlags, "Got unexpected flags %#x.\n", info.dwFlags);
        ok(!info.dwVersion, "Got unexpected version %#x.\n", info.dwVersion);
        ok(info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", info.dwVersionICM);
        ok(!info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szName));
        ok(!info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szDescription));
        ok(!lstrcmpW(info.szDriver, bogusW), "Got unexpected driver %s.\n", wine_dbgstr_w(info.szDriver));

        /* Drivers installed after msvfw32 is loaded are not enumerated. */
todo_wine
        ok(!ICInfo(test_type, 0, &info), "Expected failure.\n");

        ret = ICRemove(test_type, test_handler, 0);
        ok(ret, "Failed to remove driver.\n");

        res = RegDeleteValueA(key, "wine.test");
        ok(res == ERROR_FILE_NOT_FOUND, "Got error %u.\n", res);
        RegCloseKey(key);
    }
    else
        win_skip("Not enough permissions to register codec drivers.\n");

    if (WritePrivateProfileStringA("drivers32", "wine.test", "bogus", "system.ini"))
    {
        memset(&info, 0x55, sizeof(info));
        info.dwSize = sizeof(info);
        ok(ICInfo(test_type, test_handler, &info), "Expected success.\n");
        ok(info.fccType == test_type, "Got unexpected type %#x.\n", info.fccType);
        ok(info.fccHandler == test_handler, "Got unexpected handler %#x.\n", info.fccHandler);
        ok(!info.dwFlags, "Got unexpected flags %#x.\n", info.dwFlags);
        ok(!info.dwVersion, "Got unexpected version %#x.\n", info.dwVersion);
        ok(info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", info.dwVersionICM);
        ok(!info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szName));
        ok(!info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(info.szDescription));
        ok(!lstrcmpW(info.szDriver, bogusW), "Got unexpected driver %s.\n", wine_dbgstr_w(info.szDriver));

        /* Drivers installed after msvfw32 is loaded are not enumerated. */
todo_wine
        ok(!ICInfo(test_type, 0, &info), "Expected failure.\n");

        ret = WritePrivateProfileStringA("drivers32", "wine.test", NULL, "system.ini");
        ok(ret, "Failed to remove INI entry.\n");
    }

    ret = ICInstall(test_type, test_handler, (LPARAM)enum_driver_proc, NULL, ICINSTALL_FUNCTION);
    ok(ret, "Failed to install driver.\n");

    memset(&enum_info, 0x55, sizeof(enum_info));
    enum_info.dwSize = sizeof(enum_info);
    ok(ICInfo(test_type, test_handler, &enum_info), "Expected success.\n");
    ok(!enum_info.fccType, "Got unexpected type %#x.\n", enum_info.fccType);
    ok(!enum_info.fccHandler, "Got unexpected handler %#x.\n", enum_info.fccHandler);
    ok(!enum_info.dwFlags, "Got unexpected flags %#x.\n", enum_info.dwFlags);
    ok(enum_info.dwVersion == 0xdeadbeef, "Got unexpected version %#x.\n", enum_info.dwVersion);
    ok(enum_info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", enum_info.dwVersionICM);
    ok(!enum_info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(enum_info.szName));
    ok(!enum_info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(enum_info.szDescription));
    ok(!enum_info.szDriver[0], "Got unexpected driver %s.\n", wine_dbgstr_w(enum_info.szDriver));

    /* Functions installed after msvfw32 is loaded are enumerated. */
    memset(&enum_info, 0x55, sizeof(enum_info));
    enum_info.dwSize = sizeof(enum_info);
    ok(ICInfo(test_type, 0, &enum_info), "Expected success.\n");
    ok(!enum_info.fccType, "Got unexpected type %#x.\n", enum_info.fccType);
    ok(!enum_info.fccHandler, "Got unexpected handler %#x.\n", enum_info.fccHandler);
    ok(!enum_info.dwFlags, "Got unexpected flags %#x.\n", enum_info.dwFlags);
    ok(enum_info.dwVersion == 0xdeadbeef, "Got unexpected version %#x.\n", enum_info.dwVersion);
    ok(enum_info.dwVersionICM == ICVERSION, "Got unexpected ICM version %#x.\n", enum_info.dwVersionICM);
    ok(!enum_info.szName[0], "Got unexpected name %s.\n", wine_dbgstr_w(enum_info.szName));
    ok(!enum_info.szDescription[0], "Got unexpected name %s.\n", wine_dbgstr_w(enum_info.szDescription));
    ok(!enum_info.szDriver[0], "Got unexpected driver %s.\n", wine_dbgstr_w(enum_info.szDriver));

    ret = ICRemove(test_type, test_handler, 0);
    ok(ret, "Failed to remove driver.\n");
}

static int get_display_format_test;

static DWORD get_size_image(LONG width, LONG height, WORD depth)
{
    DWORD ret = width * depth;
    ret = (ret + 7) / 8;    /* divide by byte size, rounding up */
    ret = (ret + 3) & ~3;   /* align to 4 bytes */
    ret *= abs(height);
    return ret;
}

static const RGBQUAD color_yellow = {0x00, 0xff, 0xff, 0x00};

static BITMAPINFOHEADER gdf_in, *gdf_out;

static LRESULT CALLBACK gdf_driver_proc(DWORD_PTR id, HDRVR driver, UINT msg,
    LPARAM lparam1, LPARAM lparam2)
{
    LRESULT ret = 0;

    if (winetest_debug > 1)
        trace("(%#lx, %p, %#x, %#lx, %#lx)\n", id, driver, msg, lparam1, lparam2);

    switch(msg)
    {
    case DRV_LOAD:
    case DRV_OPEN:
    case DRV_CLOSE:
    case DRV_FREE:
        return 1;
    case ICM_DECOMPRESS_QUERY:
    {
        BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lparam2;
        DWORD expected_size;

        ok(lparam1 == (LPARAM)&gdf_in, "got input %#lx\n", lparam1);

        if (!out)
            return ICERR_OK;

        ok(out == gdf_out, "got output %p\n", out);

        ok(out->biSize == sizeof(*out), "got size %d\n", out->biSize);
        expected_size = get_size_image(out->biWidth, out->biHeight, out->biBitCount);
        ok(out->biSizeImage == expected_size, "expected image size %d, got %d\n",
            expected_size, out->biSizeImage);

        ok(out->biPlanes == 0xcccc, "got planes %d\n", out->biPlanes);
        ok(out->biXPelsPerMeter == 0xcccccccc && out->biYPelsPerMeter == 0xcccccccc,
            "got resolution %dx%d\n", out->biXPelsPerMeter, out->biYPelsPerMeter);
        ok(out->biClrUsed == 0xcccccccc, "got biClrUsed %u\n", out->biClrUsed);
        ok(out->biClrImportant == 0xcccccccc, "got biClrImportant %u\n", out->biClrImportant);

        switch (get_display_format_test)
        {
            case 0:
                return ICERR_OK;
            case 1:
                if (out->biWidth == 30 && out->biHeight == 40 && out->biCompression == BI_RGB && out->biBitCount == 16)
                    return ICERR_OK;
                break;
            case 2:
                if (out->biWidth == 30 && out->biHeight == 40 && out->biCompression == BI_BITFIELDS && out->biBitCount == 16)
                    return ICERR_OK;
                break;
            case 3:
                if (out->biWidth == 30 && out->biHeight == 40 && out->biCompression == BI_RGB && out->biBitCount == 24)
                    return ICERR_OK;
                break;
            case 4:
                if (out->biWidth == 30 && out->biHeight == 40 && out->biCompression == BI_RGB && out->biBitCount == 32)
                    return ICERR_OK;
                break;
            case 5:
                if (out->biWidth == 10 && out->biHeight == 20 && out->biCompression == BI_RGB && out->biBitCount == 32)
                    return ICERR_OK;
                break;
            case 6:
                break;
        }

        return ICERR_BADFORMAT;
    }
    case ICM_DECOMPRESS_GET_FORMAT:
    {
        BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lparam2;

        ok(lparam1 == (LPARAM)&gdf_in, "got input %#lx\n", lparam1);
        if (out)
        {
            ok(out == gdf_out, "got output %p\n", out);

            memset(out, 0x55, sizeof(*out));
            out->biWidth = 50;
            out->biHeight = 60;
            out->biBitCount = 0xdead;
            out->biCompression = 0xbeef;
            out->biSizeImage = 0;

            return ICERR_OK;
        }
    }
    case ICM_DECOMPRESS_GET_PALETTE:
    {
        BITMAPINFO *out = (BITMAPINFO *)lparam2;

        ok(lparam1 == (LPARAM)&gdf_in, "got input %#lx\n", lparam1);
        if (out)
        {
            ok(out == (BITMAPINFO *)gdf_out, "got output %p\n", out);

            out->bmiHeader.biClrUsed = 1;
            out->bmiColors[0] = color_yellow;

            return 0xdeadbeef;
        }
    }
    }

    return ret;
}

static void check_bitmap_header_(int line, BITMAPINFOHEADER *header, LONG width, LONG height, WORD depth, DWORD compression)
{
    ok_(__FILE__, line)(header->biWidth == width, "expected %d, got %d\n", width, header->biWidth);
    ok_(__FILE__, line)(header->biHeight == height, "expected %d, got %d\n", height, header->biHeight);
    ok_(__FILE__, line)(header->biBitCount == depth, "expected %d, got %d\n", depth, header->biBitCount);
    ok_(__FILE__, line)(header->biCompression == compression, "expected %#x, got %#x\n", compression, header->biCompression);
}
#define check_bitmap_header(a,b,c,d,e) check_bitmap_header_(__LINE__,a,b,c,d,e)

static void test_ICGetDisplayFormat(void)
{
    static const DWORD testcc = mmioFOURCC('t','e','s','t');
#ifdef __REACTOS__
    char outbuf[FIELD_OFFSET(BITMAPINFO, bmiColors) + 256 * sizeof(RGBQUAD)];
#else
    char outbuf[FIELD_OFFSET(BITMAPINFO, bmiColors[256])];
#endif
    BITMAPINFO *out_bmi;
    LRESULT lres;
    BOOL ret;
    HIC hic;

    memset(&gdf_in, 0xcc, sizeof(gdf_in));
    gdf_in.biSize = sizeof(gdf_in);
    gdf_in.biWidth = 10;
    gdf_in.biHeight = 20;
    gdf_in.biBitCount = 1;
    gdf_in.biCompression = testcc;

    ret = ICInstall(ICTYPE_VIDEO, testcc, (LPARAM)gdf_driver_proc, NULL, ICINSTALL_FUNCTION);
    ok(ret, "ICInstall failed\n");

    hic = ICOpen(ICTYPE_VIDEO, testcc, ICMODE_DECOMPRESS);
    ok(ret, "ICOpen failed\n");

    memset(outbuf, 0, sizeof(outbuf));
    gdf_out = (BITMAPINFOHEADER *)outbuf;

    /* ICGetDisplayFormat tries several default formats; make sure those work */
    get_display_format_test = 0;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 1, BI_RGB);

    get_display_format_test = 1;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 16, BI_RGB);

    get_display_format_test = 2;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 16, BI_BITFIELDS);

    get_display_format_test = 3;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 24, BI_RGB);

    get_display_format_test = 4;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 32, BI_RGB);

    get_display_format_test = 5;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 10, 20, 32, BI_RGB);

    /* if every default format is rejected, the output of
     * ICM_DECOMPRESS_GET_FORMAT is returned */
    get_display_format_test = 6;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 50, 60, 0xdead, 0xbeef);

    /* given bpp is treated as a lower bound */
    get_display_format_test = 1;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 24, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 50, 60, 0xdead, 0xbeef);

    get_display_format_test = 3;
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 24, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 24, BI_RGB);

    get_display_format_test = 0;

    /* width or height <= 0 causes the input width and height to be supplied */
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 0, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 10, 20, 1, BI_RGB);

    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, 0);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 10, 20, 1, BI_RGB);

    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, -10, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 10, 20, 1, BI_RGB);

    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 1, 30, -10);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 10, 20, 1, BI_RGB);

    /* zero bpp causes 32 bpp to be supplied */
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 0, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    ok(gdf_out->biBitCount == 32 || gdf_out->biBitCount == 24,
        "got %d\n", gdf_out->biBitCount);
    ok(gdf_out->biCompression == BI_RGB, "got %#x\n", gdf_out->biCompression);

    /* specifying 8 bpp yields a request for palette colours */
    hic = ICGetDisplayFormat(hic, &gdf_in, gdf_out, 8, 30, 40);
    ok(hic != NULL, "ICGetDisplayFormat failed\n");
    check_bitmap_header(gdf_out, 30, 40, 8, BI_RGB);
    ok(gdf_out->biClrUsed == 1, "got biClrUsed %u\n", gdf_out->biClrUsed);
    out_bmi = (BITMAPINFO *)gdf_out;
    ok(!memcmp(&out_bmi->bmiColors[0], &color_yellow, sizeof(color_yellow)),
        "got wrong colour\n");

    lres = ICClose(hic);
    ok(lres == ICERR_OK, "got %ld\n", lres);

    ret = ICRemove(ICTYPE_VIDEO, testcc, 0);
    ok(ret, "ICRemove failed\n");
}

START_TEST(msvfw)
{
    test_OpenCase();
    test_Locate();
    test_ICSeqCompress();
    test_ICInfo();
    test_ICGetDisplayFormat();
}
