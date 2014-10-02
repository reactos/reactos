/*
 * Copyright 2013 Jacek Caban for CodeWeavers
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
#include <aviriff.h>
#include <stdio.h>

#include <wine/test.h>

static void test_output(const BYTE *output, int out_size, const BYTE *expect, int size)
{
    char buf[512], *ptr;
    int i;

    i = out_size == size && !memcmp(output, expect, size);
    ok(i, "Unexpected output\n");
    if(i)
        return;

    for(i=0, ptr=buf; i<out_size; i++)
        ptr += sprintf(ptr, "%x ", output[i]);
    trace("Got: %s\n", buf);
    for(i=0, ptr=buf; i<size; i++)
        ptr += sprintf(ptr, "%x ", expect[i]);
    trace("Exp: %s\n", buf);
}

static void test_encode(void)
{
    BITMAPINFOHEADER *output_header;
    DWORD output_size, flags, quality;
    BYTE buf[64];
    ICINFO info;
    HIC hic;
    LRESULT res;

    struct { BITMAPINFOHEADER header; RGBQUAD map[256]; }
    input_header = { {sizeof(BITMAPINFOHEADER), 32, 1, 1, 8, 0, 32*8, 0, 0, 256, 256},
                     {{255,0,0}, {0,255,0}, {0,0,255}, {255,255,255}}};

    static BYTE input1[32] = {1,2,3,3,3,3,2,3,1};
    static const BYTE output1[] = {1,1,1,2,4,3,0,3,2,3,1,0,23,0,0,0,0,1};

    hic = ICOpen(FCC('V','I','D','C'), FCC('m','r','l','e'), ICMODE_COMPRESS);
    ok(hic != NULL, "ICOpen failed\n");

    res = ICGetInfo(hic, &info, sizeof(info));
    ok(res == sizeof(info), "res = %ld\n", res);
    ok(info.dwSize == sizeof(info), "dwSize = %d\n", info.dwSize);
    ok(info.fccHandler == FCC('M','R','L','E'), "fccHandler = %x\n", info.fccHandler);
    todo_wine ok(info.dwFlags == (VIDCF_QUALITY|VIDCF_CRUNCH|VIDCF_TEMPORAL), "dwFlags = %x\n", info.dwFlags);
    ok(info.dwVersionICM == ICVERSION, "dwVersionICM = %d\n", info.dwVersionICM);

    quality = 0xdeadbeef;
    res = ICSendMessage(hic, ICM_GETDEFAULTQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_OK, "ICSendMessage(ICM_GETDEFAULTQUALITY) failed: %ld\n", res);
    ok(quality == 8500, "quality = %d\n", quality);

    quality = 0xdeadbeef;
    res = ICSendMessage(hic, ICM_GETQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_UNSUPPORTED, "ICSendMessage(ICM_GETQUALITY) failed: %ld\n", res);
    ok(quality == 0xdeadbeef, "quality = %d\n", quality);

    quality = ICQUALITY_HIGH;
    res = ICSendMessage(hic, ICM_SETQUALITY, (DWORD_PTR)&quality, 0);
    ok(res == ICERR_UNSUPPORTED, "ICSendMessage(ICM_SETQUALITY) failed: %ld\n", res);

    output_size = ICCompressGetFormatSize(hic, &input_header.header);
    ok(output_size == 1064, "output_size = %d\n", output_size);

    output_header = HeapAlloc(GetProcessHeap(), 0, output_size);
    ICCompressGetFormat(hic, &input_header.header, output_header);

    flags = 0;
    res = ICCompress(hic, ICCOMPRESS_KEYFRAME, output_header, buf, &input_header.header, input1, 0, &flags, 0, 0, 0, NULL, NULL);
    ok(res == ICERR_OK, "ICCompress failed: %ld\n", res);
    test_output(buf, output_header->biSizeImage, output1, sizeof(output1));
    ok(flags == (AVIIF_TWOCC|AVIIF_KEYFRAME), "flags = %x\n", flags);

    HeapFree(GetProcessHeap(), 0, output_header);

    ICClose(hic);
}

START_TEST(msrle)
{
    test_encode();
}
