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

struct msg_result
{
    int msg_index;
    UINT msg;
    BOOL output_format;
    int width;
    int height;
    int bits;
    int compression;
    LRESULT result;
    BOOL todo;
};

static struct msg_result expected_msgs[] =
{
    /* Wine bug - shouldn't be called */
    { 0,  DRV_LOAD,                   FALSE,   0,   0,  0,            0,              TRUE, TRUE},
    { 0,  DRV_ENABLE,                 FALSE,   0,   0,  0,            0,                 0, TRUE},

    { 0,  DRV_OPEN,                   FALSE,   0,   0,  0,            0,        0xdeadbeef, FALSE},

    /* test 1 */
    { 1,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    { 2,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    { 3,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480,  8,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    { 4,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 16,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    { 5,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 16, BI_BITFIELDS,   ICERR_BADFORMAT, FALSE},
    { 6,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 24,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    { 7,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    { 8,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 2 */
    { 9,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {10,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {11,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480,  8,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {12,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 16,       BI_RGB,          ICERR_OK, FALSE},

    /* test 3 */
    {13,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {14,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {15,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480,  8,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {16,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 16,       BI_RGB,          ICERR_OK, FALSE},

    /* test 4 */
    {17,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {18,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {19,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 24,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {20,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {21,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 5 */
    {22,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {23,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {24,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 6 */
    {25,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {26,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {27,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 7 */
    {28,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {29,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {30,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480,  9,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {31,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,   ICERR_BADFORMAT, FALSE},
    {32,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 8 */
    {33,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {34,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {35,  ICM_DECOMPRESS_QUERY,       TRUE,  800, 600, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 9 */
    {36,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {37,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {38,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 10 */
    {39,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {40,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {41,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480, 32,       BI_RGB,          ICERR_OK, FALSE},

    /* test 11 */
    {42,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {43,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {44,  ICM_DECOMPRESS_QUERY,       TRUE,  270, 270,  8,       BI_RGB,          ICERR_OK, FALSE},
    {45,  ICM_DECOMPRESS_GET_PALETTE, FALSE,   0,   0,  0,            0, ICERR_UNSUPPORTED, FALSE},

    /* test 12 */
    {46,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {47,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {48,  ICM_DECOMPRESS_QUERY,       TRUE,  270, 270, 16,       BI_RGB,          ICERR_OK, FALSE},

    /* test 13 */
    {49,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {50,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {51,  ICM_DECOMPRESS_QUERY,       TRUE,  270, 270, 24,       BI_RGB,          ICERR_OK, FALSE},

    /* test 14 */
    {52,  ICM_DECOMPRESS_QUERY,       FALSE,   0,   0,  0,            0,          ICERR_OK, FALSE},
    {53,  ICM_DECOMPRESS_GET_FORMAT,  TRUE,  320, 240, 16,       BI_RGB,          ICERR_OK, FALSE},
    {54,  ICM_DECOMPRESS_QUERY,       TRUE,  640, 480,  4,       BI_RGB,          ICERR_OK, FALSE},

    /* Wine bug - shouldn't be called */
    {55,  DRV_DISABLE,                FALSE,   0,   0,  0,            0,          ICERR_OK, TRUE},
    {55,  DRV_FREE,                   FALSE,   0,   0,  0,            0,          ICERR_OK, TRUE},
};

static int msg_index = 0;

static struct msg_result *get_expected_msg(UINT msg)
{
    int i = 0;
    for(; i < sizeof(expected_msgs) / sizeof(expected_msgs[0]); i++)
    {
        if (expected_msgs[i].msg_index == msg_index && expected_msgs[i].msg == msg)
            return &expected_msgs[i];
    }
    return NULL;
}

LRESULT WINAPI driver_proc_test(DWORD_PTR dwDriverId, HDRVR hdrvr, UINT msg,
                                LPARAM lParam1, LPARAM lParam2)
{
    struct msg_result *expected = get_expected_msg(msg);
    LRESULT res = expected ? expected->result : ICERR_UNSUPPORTED;

    if (msg == DRV_CLOSE)
        return ICERR_OK;

    if (!expected)
    {
        ok(0, "unexpected message: %04x %ld %ld at msg index %d\n",
           msg, lParam1, lParam2, msg_index);
        return ICERR_UNSUPPORTED;
    }
    else if (expected->todo)
    {
        todo_wine ok(0, "unexpected message: %04x %ld %ld at msg index %d\n",
                     msg, lParam1, lParam2, msg_index);
        return res;
    }

    switch (msg)
    {
        case ICM_DECOMPRESS_QUERY:
        {
            BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lParam2;

            if (!lParam2)
            {
                trace("query -> without format\n");
                ok(!expected->output_format, "Expected no output format pointer\n");
                break;
            }

            ok(expected->output_format, "Expected output format pointer\n");
            ok(out->biWidth == expected->width,
               "Expected width %d, got %d\n", expected->width, out->biWidth);
            ok(out->biHeight == expected->height,
               "Expected height %d, got %d\n", expected->height, out->biHeight);
            ok(out->biBitCount == expected->bits,
               "Expected biBitCount %d, got %d\n", expected->bits, out->biBitCount);
            ok(out->biCompression == expected->compression,
               "Expected compression %d, got %d\n", expected->compression, out->biCompression);
            ok(out->biSizeImage == get_stride(out->biWidth, out->biBitCount) * out->biHeight,
               "Expected biSizeImage %d, got %d\n", get_stride(out->biWidth, out->biBitCount) * out->biHeight,
                out->biSizeImage);

            trace("query -> width: %d, height: %d, bit: %d, compression: %d, size: %d\n",
                  out->biWidth, out->biHeight, out->biBitCount, out->biCompression, out->biSizeImage);
            break;
        }

        case ICM_DECOMPRESS_GET_FORMAT:
        {
            BITMAPINFOHEADER *out = (BITMAPINFOHEADER *)lParam2;

            if (!lParam2)
            {
                trace("format -> without format\n");
                ok(!expected->output_format, "Expected no output format pointer\n");
                break;
            }

            ok(expected->output_format, "Expected output format pointer\n");
            ok(out->biWidth == expected->width,
               "Expected width %d, got %d\n", expected->width, out->biWidth);
            ok(out->biHeight == expected->height,
               "Expected height %d, got %d\n", expected->height, out->biHeight);
            ok(out->biBitCount == expected->bits,
               "Expected biBitCount %d, got %d\n", expected->bits, out->biBitCount);
            ok(out->biCompression == expected->compression,
               "Expected compression %d, got %d\n", expected->compression, out->biCompression);

            trace("format -> width: %d, height: %d, bit: %d, compression: %d, size: %d\n",
                  out->biWidth, out->biHeight, out->biBitCount, out->biCompression, out->biSizeImage);

            out->biBitCount = 64;
            break;
        }
    }

    msg_index++;
    return res;
}


void test_ICGetDisplayFormat(void)
{
    static const struct
    {
        int bits_wanted;
        int bits_expected;
        int dx;
        int width_expected;
        int dy;
        int height_expected;
        int msg_index;
    }
    tests[] =
    {
        { 8, 64,   0, 640,   0, 480, 9},
        { 8, 16,   0, 640,   0, 480, 13},
        { 8, 16,   0, 640,   0, 480, 17},
        {24, 64,   0, 640,   0, 480, 22},
        {32, 32,   0, 640,   0, 480, 25},
        { 0, 32,   0, 640,   0, 480, 28},
        { 9, 64,   0, 640,   0, 480, 33},
        {32, 32, 800, 800, 600, 600, 36},
        {32, 32,  -1, 640,  -1, 480, 39},
        {32, 32, -90, 640, -60, 480, 42},
        { 8,  8, 270, 270, 270, 270, 46},
        {16, 16, 270, 270, 270, 270, 49},
        {24, 24, 270, 270, 270, 270, 52},
        { 4,  4,   0, 640,   0, 480, 55},
    };

    HIC ic, ic2;
    BITMAPINFOHEADER in;
    BITMAPINFOHEADER out;
    int real_depth;
    int i;

    ic = ICOpenFunction(ICTYPE_VIDEO, 0xdeadbeef, ICMODE_DECOMPRESS, driver_proc_test);
    ok(!!ic, "Opening driver failed\n");

    for (i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        memset(&in, 0, sizeof(in));
        memset(&out, 0, sizeof(out));

        in.biSize = sizeof(in);
        in.biWidth = 640;
        in.biHeight = 480;
        in.biPlanes = 1;
        in.biBitCount = 32;
        in.biCompression = BI_PNG;
        in.biSizeImage = 1024;

        out.biBitCount = 16;
        out.biWidth = 320;
        out.biHeight = 240;

        ic2 = ICGetDisplayFormat(ic, &in, &out, tests[i].bits_wanted, tests[i].dx, tests[i].dy);
        ok(!!ic2, "Expected ICGetDisplayFormat to succeeded\n");

        ok(out.biBitCount == tests[i].bits_expected,
           "Expected biBitCount %d, got %d\n", tests[i].bits_expected, out.biBitCount);
        ok(out.biWidth == tests[i].width_expected,
           "Expected biWidth %d, got %d\n", tests[i].width_expected, out.biWidth);
        ok(out.biHeight == tests[i].height_expected,
           "Expected biHeight %d, got %d\n", tests[i].height_expected, out.biHeight);
        real_depth = (out.biBitCount > 32) ? 32 : out.biBitCount;
        ok(out.biSizeImage == get_stride(out.biWidth, real_depth) * out.biHeight,
           "Expected biSizeImage %d, got %d\n", get_stride(out.biWidth, real_depth) * out.biHeight,
           out.biSizeImage);
        ok(msg_index == tests[i].msg_index,
           "Expected msg_index %d, got %d\n", tests[i].msg_index, msg_index);
    }

    ICClose(ic);
}

START_TEST(msvfw)
{
    test_OpenCase();
    test_Locate();
    test_ICSeqCompress();
    test_ICGetDisplayFormat();
}
