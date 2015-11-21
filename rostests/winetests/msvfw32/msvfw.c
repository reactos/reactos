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

START_TEST(msvfw)
{
    test_OpenCase();
    test_Locate();
    test_ICSeqCompress();
}
