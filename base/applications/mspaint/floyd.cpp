/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Floyd Steinberg's error variance method
 * COPYRIGHT:  Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

static inline INT FindNearestColor(INT r, INT g, INT b, const RGBQUAD* palette, INT nColors)
{
    INT bestIdx  = 0;
    DWORD bestDist = UINT_MAX;
    for (INT i = 0; i < nColors; i++)
    {
        const INT dr = r - (INT)palette[i].rgbRed;
        const INT dg = g - (INT)palette[i].rgbGreen;
        const INT db = b - (INT)palette[i].rgbBlue;
        const DWORD dist = (DWORD)(dr*dr + dg*dg + db*db);
        if (dist < bestDist)
        {
            bestDist = dist;
            bestIdx = i;
        }
        if (dist == 0)
            break;
    }
    return bestIdx;
}

void FloydSteinberg(const BYTE* srcBuf, INT srcStride, INT W, INT H,
                    const RGBQUAD* palette, INT nColors, PBYTE indexImg)
{
    struct ERR_RGB { float r, g, b; };
    const INT nCount = W * H;
    ERR_RGB* err = (ERR_RGB*)LocalAlloc(LPTR, nCount * sizeof(ERR_RGB));
    for (INT i = 0; i < nCount; ++i)
        err[i].r = err[i].g = err[i].b = 0.0f;

    for (INT y = 0; y < H; y++)
    {
        for (INT x = 0; x < W; x++)
        {
            const BYTE* px = srcBuf + y * srcStride + x * 3;
            const float fr = (float)px[2] + err[y*W + x].r; // R
            const float fg = (float)px[1] + err[y*W + x].g; // G
            const float fb = (float)px[0] + err[y*W + x].b; // B

            const INT ir = (INT)max(0.f, min(255.f, fr));
            const INT ig = (INT)max(0.f, min(255.f, fg));
            const INT ib = (INT)max(0.f, min(255.f, fb));

            const INT idx = FindNearestColor(ir, ig, ib, palette, nColors);
            indexImg[y * W + x] = (BYTE)idx;

            const float er = fr - (float)palette[idx].rgbRed;
            const float eg = fg - (float)palette[idx].rgbGreen;
            const float eb = fb - (float)palette[idx].rgbBlue;

#define SPREAD(nx, ny, w) do { \
    if ((unsigned)(nx) < (unsigned)W && (unsigned)(ny) < (unsigned)H) { \
        ERR_RGB& e = err[ny * W + nx]; \
        e.r += er * (w); e.g += eg * (w); e.b += eb * (w); \
    } \
} while (0)
            SPREAD(x + 1, y,     7.f / 16);
            SPREAD(x - 1, y + 1, 3.f / 16);
            SPREAD(x,     y + 1, 5.f / 16);
            SPREAD(x + 1, y + 1, 1.f / 16);
        }
    }

    LocalFree(err);
}
