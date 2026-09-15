/*
 * Copyright 2014 Akihiro Sagawa
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
#include <wincrypt.h>
#include <stdlib.h>
#include <string.h>

#include "wine/test.h"

#define WIDTH  16
#define HEIGHT 12

static HCRYPTPROV crypt_prov;

static inline DWORD get_stride(const BITMAPINFO *bmi)
{
    return ((bmi->bmiHeader.biBitCount * bmi->bmiHeader.biWidth + 31) >> 3) & ~3;
}

static inline DWORD get_dib_size(const BITMAPINFO *bmi)
{
    return get_stride(bmi) * abs(bmi->bmiHeader.biHeight);
}

static char *hash_dib(const BITMAPINFO *bmi, const void *bits)
{
    DWORD dib_size = get_dib_size(bmi);
    HCRYPTHASH hash;
    char *buf;
    BYTE hash_buf[20];
    DWORD hash_size = sizeof(hash_buf);
    int i;
    static const char *hex = "0123456789abcdef";

    if(!crypt_prov) return NULL;

    if(!CryptCreateHash(crypt_prov, CALG_SHA1, 0, 0, &hash)) return NULL;

    CryptHashData(hash, bits, dib_size, 0);

    CryptGetHashParam(hash, HP_HASHVAL, NULL, &hash_size, 0);
    if(hash_size != sizeof(hash_buf)) return NULL;

    CryptGetHashParam(hash, HP_HASHVAL, hash_buf, &hash_size, 0);
    CryptDestroyHash(hash);

    buf = malloc(hash_size * 2 + 1);

    for(i = 0; i < hash_size; i++)
    {
        buf[i * 2] = hex[hash_buf[i] >> 4];
        buf[i * 2 + 1] = hex[hash_buf[i] & 0xf];
    }
    buf[i * 2] = '\0';

    return buf;
}

static void init_bmi(BITMAPINFO *bmi, LONG width, LONG height, DWORD size)
{
    memset(bmi, 0, sizeof(*bmi));
    bmi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth       = width;
    bmi->bmiHeader.biHeight      = height;
    bmi->bmiHeader.biPlanes      = 1;
    bmi->bmiHeader.biBitCount    = 32;
    bmi->bmiHeader.biCompression = BI_RGB;
    bmi->bmiHeader.biSizeImage   = size;
}

static void test_DrawDib_sizeimage(void)
{
    const struct {
        LONG width, height;
        DWORD size;
        char hash[41];
    } test_data[] = {
        /* [0] correct size */
        {  WIDTH,   HEIGHT, WIDTH * HEIGHT * sizeof(RGBQUAD), "bc943d5ab024b8b0118d0a80aa283055d39942b8" },
        /* [1] zero size */
        {  WIDTH,   HEIGHT, 0, "bc943d5ab024b8b0118d0a80aa283055d39942b8" },
        /* error patterns */
        {  WIDTH,  -HEIGHT, 0, "" },
        { -WIDTH,   HEIGHT, 0, "" },
        { -WIDTH,  -HEIGHT, 0, "" },
        {      0,        0, 0, "" },
        {      0,   HEIGHT, 0, "" },
        {  WIDTH,        0, 0, "" },
        /* [8] zero size (to compare [9], [10] ) */
        {  WIDTH, HEIGHT/2, 0, "8b75bf6d54a8645380114fe77505ee0699ffffaa" },
        /* [9] insufficient size */
        {  WIDTH, HEIGHT/2, sizeof(RGBQUAD), "8b75bf6d54a8645380114fe77505ee0699ffffaa" },
        /* [10] too much size */
        {  WIDTH, HEIGHT/2, WIDTH * HEIGHT * sizeof(RGBQUAD), "8b75bf6d54a8645380114fe77505ee0699ffffaa" },
    };
    HDC hdc;
    DWORD src_dib_size, dst_dib_size;
    BOOL r;
    HBITMAP dib;
    BITMAPINFO src_info, dst_info;
    RGBQUAD *src_bits = NULL, *dst_bits;
    HDRAWDIB hdd;
    unsigned int i;

    hdc = CreateCompatibleDC(NULL);

    init_bmi(&dst_info, WIDTH, HEIGHT, 0);
    dib = CreateDIBSection(NULL, &dst_info, DIB_RGB_COLORS, (void **)&dst_bits, NULL, 0);
    dst_dib_size = get_dib_size(&dst_info);
    ok(dib != NULL, "CreateDIBSection failed\n");
    SelectObject(hdc, dib);

    init_bmi(&src_info, WIDTH, HEIGHT, 0);
    src_dib_size = get_dib_size(&src_info);
    src_bits = malloc(src_dib_size);
    ok(src_bits != NULL, "Can't allocate memory\n");
    memset(src_bits, 0x88, src_dib_size);

    hdd = DrawDibOpen();
    ok(hdd != NULL, "DrawDibOpen failed\n");

    for (i = 0; i < ARRAY_SIZE(test_data); i++) {
        char *hash;
        memset(dst_bits, 0xff, dst_dib_size);
        init_bmi(&src_info, test_data[i].width, test_data[i].height, test_data[i].size);
        r = DrawDibDraw(hdd, hdc,
                        0, 0, -1, -1, &src_info.bmiHeader, src_bits,
                        0, 0, test_data[i].width, test_data[i].height, 0);
        if (test_data[i].hash[0])
            ok(r, "[%u] DrawDibDraw failed, expected success\n", i);
        else
            ok(!r, "[%u] DrawDibDraw succeeded, expected failed\n", i);
        if (!r || !test_data[i].hash[0])
            continue;

        hash = hash_dib(&dst_info, dst_bits);
        if (!hash) {
            win_skip("This platform doesn't support SHA-1 hash\n");
            continue;
        }
        ok(strcmp(hash, test_data[i].hash) == 0,
           "[%u] got %s, expected %s\n",
           i, hash, test_data[i].hash);
        free(hash);
    }

    r = DrawDibClose(hdd);
    ok(r, "DrawDibClose failed\n");

    free(src_bits);

    DeleteDC(hdc);
}

START_TEST(drawdib)
{
    CryptAcquireContextW(&crypt_prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    test_DrawDib_sizeimage();
    CryptReleaseContext(crypt_prov, 0);
}
