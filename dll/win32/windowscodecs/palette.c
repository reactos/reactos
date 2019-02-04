/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012,2016 Dmitry Timoshkov
 * Copyright 2016 Sebastian Lackner
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    IWICPalette IWICPalette_iface;
    LONG ref;
    UINT count;
    WICColor *colors;
    WICBitmapPaletteType type;
    CRITICAL_SECTION lock; /* must be held when count, colors, or type is accessed */
} PaletteImpl;

static inline PaletteImpl *impl_from_IWICPalette(IWICPalette *iface)
{
    return CONTAINING_RECORD(iface, PaletteImpl, IWICPalette_iface);
}

static HRESULT WINAPI PaletteImpl_QueryInterface(IWICPalette *iface, REFIID iid,
    void **ppv)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IWICPalette, iid))
    {
        *ppv = &This->IWICPalette_iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PaletteImpl_AddRef(IWICPalette *iface)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PaletteImpl_Release(IWICPalette *iface)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This->colors);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static WICColor *generate_gray16_palette(UINT *count)
{
    WICColor *entries;
    UINT i;

    *count = 16;
    entries = HeapAlloc(GetProcessHeap(), 0, 16 * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 16; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i<<20) | (i<<16) | (i<<12) | (i<<8) | (i<<4) | i;
    }
    return entries;
}

static WICColor *generate_gray256_palette(UINT *count)
{
    WICColor *entries;
    UINT i;

    *count = 256;
    entries = HeapAlloc(GetProcessHeap(), 0, 256 * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 256; i++)
    {
        entries[i] = 0xff000000;
        entries[i] |= (i<<16) | (i<<8) | i;
    }
    return entries;
}

static WICColor *generate_halftone8_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 17 : 16;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 8; i++)
    {
        entries[i] = 0xff000000;
        if (i & 1) entries[i] |= 0xff;
        if (i & 2) entries[i] |= 0xff00;
        if (i & 4) entries[i] |= 0xff0000;
    }

    for (i = 8; i < 16; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-8];
    }

    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone27_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 29 : 28;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 27; i++)
    {
        static const BYTE halftone_values[4] = { 0x00,0x80,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%3];
        entries[i] |= halftone_values[(i/3)%3] << 8;
        entries[i] |= halftone_values[(i/9)%3] << 16;
    }

    entries[i++] = 0xffc0c0c0;
    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone64_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 73 : 72;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 64; i++)
    {
        static const BYTE halftone_values[4] = { 0x00,0x55,0xaa,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%4];
        entries[i] |= halftone_values[(i/4)%4] << 8;
        entries[i] |= halftone_values[(i/16)%4] << 16;
    }

    for (i = 64; i < 72; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-64];
    }

    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone125_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 127 : 126;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 125; i++)
    {
        static const BYTE halftone_values[5] = { 0x00,0x40,0x80,0xbf,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%5];
        entries[i] |= halftone_values[(i/5)%5] << 8;
        entries[i] |= halftone_values[(i/25)%5] << 16;
    }

    entries[i++] = 0xffc0c0c0;
    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone216_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 225 : 224;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 216; i++)
    {
        static const BYTE halftone_values[6] = { 0x00,0x33,0x66,0x99,0xcc,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values[i%6];
        entries[i] |= halftone_values[(i/6)%6] << 8;
        entries[i] |= halftone_values[(i/36)%6] << 16;
    }

    for (i = 216; i < 224; i++)
    {
        static const DWORD halftone[8] = { 0xc0c0c0, 0x808080, 0x800000, 0x008000,
                                           0x000080, 0x808000, 0x800080, 0x008080 };
        entries[i] = 0xff000000;
        entries[i] |= halftone[i-216];
    }

    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone252_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = add_transparent ? 253 : 252;
    entries = HeapAlloc(GetProcessHeap(), 0, *count * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 252; i++)
    {
        static const BYTE halftone_values_rb[6] = { 0x00,0x33,0x66,0x99,0xcc,0xff };
        static const BYTE halftone_values_g[7] = { 0x00,0x2b,0x55,0x80,0xaa,0xd5,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values_rb[i%6];
        entries[i] |= halftone_values_g[(i/6)%7] << 8;
        entries[i] |= halftone_values_rb[(i/42)%6] << 16;
    }

    if (add_transparent)
        entries[i] = 0;

    return entries;
}

static WICColor *generate_halftone256_palette(UINT *count, BOOL add_transparent)
{
    WICColor *entries;
    UINT i;

    *count = 256;
    entries = HeapAlloc(GetProcessHeap(), 0, 256 * sizeof(WICColor));
    if (!entries) return NULL;

    for (i = 0; i < 256; i++)
    {
        static const BYTE halftone_values_b[4] = { 0x00,0x55,0xaa,0xff };
        static const BYTE halftone_values_gr[8] = { 0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff };
        entries[i] = 0xff000000;
        entries[i] |= halftone_values_b[i%4];
        entries[i] |= halftone_values_gr[(i/4)%8] << 8;
        entries[i] |= halftone_values_gr[(i/32)%8] << 16;
    }

    if (add_transparent)
        entries[255] = 0;

    return entries;
}

static HRESULT WINAPI PaletteImpl_InitializePredefined(IWICPalette *iface,
    WICBitmapPaletteType type, BOOL add_transparent)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    WICColor *colors;
    UINT count;

    TRACE("(%p,%u,%d)\n", iface, type, add_transparent);

    switch (type)
    {
    case WICBitmapPaletteTypeFixedBW:
        count = 2;
        colors = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WICColor));
        if (!colors) return E_OUTOFMEMORY;
        colors[0] = 0xff000000;
        colors[1] = 0xffffffff;
        break;

    case WICBitmapPaletteTypeFixedGray4:
        count = 4;
        colors = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WICColor));
        if (!colors) return E_OUTOFMEMORY;
        colors[0] = 0xff000000;
        colors[1] = 0xff555555;
        colors[2] = 0xffaaaaaa;
        colors[3] = 0xffffffff;
        break;

    case WICBitmapPaletteTypeFixedGray16:
        colors = generate_gray16_palette(&count);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedGray256:
        colors = generate_gray256_palette(&count);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone8:
        colors = generate_halftone8_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone27:
        colors = generate_halftone27_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone64:
        colors = generate_halftone64_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone125:
        colors = generate_halftone125_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone216:
        colors = generate_halftone216_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone252:
        colors = generate_halftone252_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    case WICBitmapPaletteTypeFixedHalftone256:
        colors = generate_halftone256_palette(&count, add_transparent);
        if (!colors) return E_OUTOFMEMORY;
        break;

    default:
        WARN("invalid palette type %u\n", type);
        return E_INVALIDARG;
    }

    EnterCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, This->colors);
    This->colors = colors;
    This->count = count;
    This->type = type;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_InitializeCustom(IWICPalette *iface,
    WICColor *pColors, UINT colorCount)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    WICColor *new_colors;

    TRACE("(%p,%p,%u)\n", iface, pColors, colorCount);

    if (colorCount == 0)
    {
        new_colors = NULL;
    }
    else
    {
        if (!pColors) return E_INVALIDARG;
        new_colors = HeapAlloc(GetProcessHeap(), 0, sizeof(WICColor) * colorCount);
        if (!new_colors) return E_OUTOFMEMORY;
        memcpy(new_colors, pColors, sizeof(WICColor) * colorCount);
    }

    EnterCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, This->colors);
    This->colors = new_colors;
    This->count = colorCount;
    This->type = WICBitmapPaletteTypeCustom;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

#define R_COUNT (1 << 5)
#define R_SHIFT (8 - 5)
#define R_SCALE 2

#define G_COUNT (1 << 6)
#define G_SHIFT (8 - 6)
#define G_SCALE 3

#define B_COUNT (1 << 5)
#define B_SHIFT (8 - 5)
#define B_SCALE 1

struct histogram
{
    unsigned int data[R_COUNT][G_COUNT][B_COUNT];
};

struct box
{
    int r_min, r_max;
    int g_min, g_max;
    int b_min, b_max;
    unsigned int count;
    unsigned int score;
};

/* count nonzero elements in the histogram range [r_min, r_max] x [g_min, g_max] x [b_min, b_max] */
static inline unsigned int histogram_count(struct histogram *h, int r_min, int r_max,
                                           int g_min, int g_max, int b_min, int b_max)
{
    unsigned int count = 0;
    int r, g, b;
    for (r = r_min; r <= r_max; r++)
    for (g = g_min; g <= g_max; g++)
    for (b = b_min; b <= b_max; b++)
        if (h->data[r][g][b] != 0) count++;
    return count;
}

/* compute weighted average color in the range [r_min, r_max] x [g_min, g_max] x [b_min, b_max] */
static unsigned int histogram_color(struct histogram *h, int r_min, int r_max,
                                    int g_min, int g_max, int b_min, int b_max)
{
    unsigned long long r_sum = 0, g_sum = 0, b_sum = 0;
    unsigned int tmp, count = 0;
    int r, g, b;

    for (r = r_min; r <= r_max; r++)
    for (g = g_min; g <= g_max; g++)
    for (b = b_min; b <= b_max; b++)
    {
        if (!(tmp = h->data[r][g][b])) continue;
        r_sum += ((r << R_SHIFT) + ((1 << R_SHIFT) / 2)) * tmp;
        g_sum += ((g << G_SHIFT) + ((1 << G_SHIFT) / 2)) * tmp;
        b_sum += ((b << B_SHIFT) + ((1 << B_SHIFT) / 2)) * tmp;
        count += tmp;
    }

    return ((b_sum + (count / 2)) / count) |
           ((g_sum + (count / 2)) / count) << 8 |
           ((r_sum + (count / 2)) / count) << 16 | 0xff000000;
}

/* same as histogram_count */
static inline unsigned int box_count(struct histogram *h, struct box *b)
{
    return histogram_count(h, b->r_min, b->r_max, b->g_min, b->g_max, b->b_min, b->b_max);
}

/* same as histogram_color */
static inline unsigned int box_color(struct histogram *h, struct box *b)
{
    return histogram_color(h, b->r_min, b->r_max, b->g_min, b->g_max, b->b_min, b->b_max);
}

/* compute score used to determine best split (also called "volume") */
static inline unsigned int box_score(struct box *b)
{
    unsigned int tmp, sum = 0;
    tmp = ((b->r_max - b->r_min) << R_SHIFT) * R_SCALE; sum += tmp * tmp;
    tmp = ((b->g_max - b->g_min) << G_SHIFT) * G_SCALE; sum += tmp * tmp;
    tmp = ((b->b_max - b->b_min) << B_SHIFT) * B_SCALE; sum += tmp * tmp;
    return sum;
}

/* attempt to shrink a box */
static void shrink_box(struct histogram *h, struct box *b)
{
    int i;
    for (i = b->r_min; i <= b->r_max; i++)
        if (histogram_count(h, i, i, b->g_min, b->g_max, b->b_min, b->b_max)) { b->r_min = i; break; }
    for (i = b->r_max; i >= b->r_min; i--)
        if (histogram_count(h, i, i, b->g_min, b->g_max, b->b_min, b->b_max)) { b->r_max = i; break; }
    for (i = b->g_min; i <= b->g_max; i++)
        if (histogram_count(h, b->r_min, b->r_max, i, i, b->b_min, b->b_max)) { b->g_min = i; break; }
    for (i = b->g_max; i >= b->g_min; i--)
        if (histogram_count(h, b->r_min, b->r_max, i, i, b->b_min, b->b_max)) { b->g_max = i; break; }
    for (i = b->b_min; i <= b->b_max; i++)
        if (histogram_count(h, b->r_min, b->r_max, b->g_min, b->g_max, i, i)) { b->b_min = i; break; }
    for (i = b->b_max; i >= b->b_min; i--)
        if (histogram_count(h, b->r_min, b->r_max, b->g_min, b->g_max, i, i)) { b->b_max = i; break; }
    b->count = box_count(h, b);
    b->score = box_score(b);
}

/* helper for split_box */
static inline void set_avg(int *min, int *max)
{
    int avg = (*min + *max) / 2;
    *min = avg + 1;
    *max = avg;
}

/* split a box based on the best axis */
static void split_box(struct histogram *h, struct box *b1, struct box *b2)
{
    int r = ((b1->r_max - b1->r_min) << R_SHIFT) * R_SCALE;
    int g = ((b1->g_max - b1->g_min) << G_SHIFT) * G_SCALE;
    int b = ((b1->b_max - b1->b_min) << B_SHIFT) * B_SCALE;

    *b2 = *b1;

    if (r > g)
    {
        if (b > r) set_avg(&b1->b_min, &b2->b_max);
        else set_avg(&b1->r_min, &b2->r_max);
    }
    else
    {
        if (b > g) set_avg(&b1->b_min, &b2->b_max);
        else set_avg(&b1->g_min, &b2->g_max);
    }

    shrink_box(h, b1);
    shrink_box(h, b2);
}

/* find box suitable for split based on count */
static struct box *find_box_max_count(struct box *b, int count)
{
    struct box *best = NULL;
    for (; count--; b++)
        if (b->score && (!best || b->count > best->count)) best = b;
    return best;
}

/* find box suitable for split based on score */
static struct box *find_box_max_score(struct box *b, int count)
{
    struct box *best = NULL;
    for (; count--; b++)
        if (b->score && (!best || b->score > best->score)) best = b;
    return best;
}

/* compute color map with at most 'desired' colors
 * image must be in 24bpp BGR format and colors are returned in 0xAARRGGBB format */
static int median_cut(unsigned char *image, unsigned int width, unsigned int height,
                      unsigned int stride, int desired, unsigned int *colors)
{
    struct box boxes[256];
    struct histogram *h;
    unsigned int x, y;
    unsigned char *p;
    struct box *b1, *b2;
    int numboxes, i;

    if (!(h = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*h))))
        return 0;

    for (y = 0; y < height; y++)
        for (x = 0, p = image + y * stride; x < width; x++, p += 3)
            h->data[p[2] >> R_SHIFT][p[1] >> G_SHIFT][p[0] >> B_SHIFT]++;

    numboxes = 1;
    boxes[0].r_min = 0; boxes[0].r_max = R_COUNT - 1;
    boxes[0].g_min = 0; boxes[0].g_max = G_COUNT - 1;
    boxes[0].b_min = 0; boxes[0].b_max = B_COUNT - 1;
    shrink_box(h, &boxes[0]);

    while (numboxes <= desired / 2)
    {
        if (!(b1 = find_box_max_count(boxes, numboxes))) break;
        b2 = &boxes[numboxes++];
        split_box(h, b1, b2);
    }
    while (numboxes < desired)
    {
        if (!(b1 = find_box_max_score(boxes, numboxes))) break;
        b2 = &boxes[numboxes++];
        split_box(h, b1, b2);
    }

    for (i = 0; i < numboxes; i++)
        colors[i] = box_color(h, &boxes[i]);

    HeapFree(GetProcessHeap(), 0, h);
    return numboxes;
}


static HRESULT WINAPI PaletteImpl_InitializeFromBitmap(IWICPalette *palette,
    IWICBitmapSource *source, UINT desired, BOOL add_transparent)
{
    IWICImagingFactory *factory = NULL;
    IWICBitmap *rgb24_bitmap = NULL;
    IWICBitmapSource *rgb24_source;
    IWICBitmapLock *lock = NULL;
    WICPixelFormatGUID format;
    HRESULT hr;
    UINT width, height, stride, size, actual_number_of_colors;
    BYTE *src;
    WICColor colors[256];

    TRACE("(%p,%p,%u,%d)\n", palette, source, desired, add_transparent);

    if (!source || desired < 2 || desired > 256)
        return E_INVALIDARG;

    hr = IWICBitmapSource_GetPixelFormat(source, &format);
    if (hr != S_OK) return hr;

    /* For interoperability with gdiplus where PixelFormat24bppRGB actully stored
     * as BGR (and there is no a corresponding RGB format) we have to use 24bppBGR
     * to avoid format conversions.
     */
    if (!IsEqualGUID(&format, &GUID_WICPixelFormat24bppBGR))
    {
        hr = WICConvertBitmapSource(&GUID_WICPixelFormat24bppBGR, source, &rgb24_source);
        if (hr != S_OK) return hr;
    }
    else
        rgb24_source = source;

    hr = ImagingFactory_CreateInstance(&IID_IWICImagingFactory, (void **)&factory);
    if (hr != S_OK) goto fail;

    hr = IWICImagingFactory_CreateBitmapFromSource(factory, rgb24_source, WICBitmapCacheOnLoad, &rgb24_bitmap);
    if (hr != S_OK) goto fail;

    hr = IWICBitmap_Lock(rgb24_bitmap, NULL, WICBitmapLockRead, &lock);
    if (hr != S_OK) goto fail;

    IWICBitmapLock_GetSize(lock, &width, &height);
    IWICBitmapLock_GetStride(lock, &stride);
    IWICBitmapLock_GetDataPointer(lock, &size, &src);

    actual_number_of_colors = median_cut(src, width, height, stride, add_transparent ? desired - 1 : desired, colors);
    TRACE("actual number of colors: %u\n", actual_number_of_colors);

    if (actual_number_of_colors)
    {
        if (add_transparent) colors[actual_number_of_colors++] = 0;

        hr = IWICPalette_InitializeCustom(palette, colors, actual_number_of_colors);
    }
    else
        hr = E_OUTOFMEMORY;

fail:
    if (lock)
        IWICBitmapLock_Release(lock);

    if (rgb24_bitmap)
        IWICBitmap_Release(rgb24_bitmap);

    if (factory)
        IWICImagingFactory_Release(factory);

    if (rgb24_source != source)
        IWICBitmapSource_Release(rgb24_source);

    return hr;
}

static HRESULT WINAPI PaletteImpl_InitializeFromPalette(IWICPalette *iface,
    IWICPalette *source)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    UINT count;
    WICColor *colors = NULL;
    WICBitmapPaletteType type;
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, source);

    if (!source) return E_INVALIDARG;

    hr = IWICPalette_GetType(source, &type);
    if (hr != S_OK) return hr;
    hr = IWICPalette_GetColorCount(source, &count);
    if (hr != S_OK) return hr;
    if (count)
    {
        colors = HeapAlloc(GetProcessHeap(), 0, sizeof(WICColor) * count);
        if (!colors) return E_OUTOFMEMORY;
        hr = IWICPalette_GetColors(source, count, colors, &count);
        if (hr != S_OK)
        {
            HeapFree(GetProcessHeap(), 0, colors);
            return hr;
        }
    }

    EnterCriticalSection(&This->lock);
    HeapFree(GetProcessHeap(), 0, This->colors);
    This->colors = colors;
    This->count = count;
    This->type = type;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_GetType(IWICPalette *iface,
    WICBitmapPaletteType *pePaletteType)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);

    TRACE("(%p,%p)\n", iface, pePaletteType);

    if (!pePaletteType) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pePaletteType = This->type;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_GetColorCount(IWICPalette *iface, UINT *pcCount)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);

    TRACE("(%p,%p)\n", iface, pcCount);

    if (!pcCount) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    *pcCount = This->count;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_GetColors(IWICPalette *iface, UINT colorCount,
    WICColor *pColors, UINT *pcActualColors)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);

    TRACE("(%p,%i,%p,%p)\n", iface, colorCount, pColors, pcActualColors);

    if (!pColors || !pcActualColors) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);

    if (This->count < colorCount) colorCount = This->count;

    memcpy(pColors, This->colors, sizeof(WICColor) * colorCount);

    *pcActualColors = colorCount;

    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_IsBlackWhite(IWICPalette *iface, BOOL *pfIsBlackWhite)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);

    TRACE("(%p,%p)\n", iface, pfIsBlackWhite);

    if (!pfIsBlackWhite) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    if (This->type == WICBitmapPaletteTypeFixedBW)
        *pfIsBlackWhite = TRUE;
    else
        *pfIsBlackWhite = FALSE;
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_IsGrayscale(IWICPalette *iface, BOOL *pfIsGrayscale)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);

    TRACE("(%p,%p)\n", iface, pfIsGrayscale);

    if (!pfIsGrayscale) return E_INVALIDARG;

    EnterCriticalSection(&This->lock);
    switch(This->type)
    {
        case WICBitmapPaletteTypeFixedBW:
        case WICBitmapPaletteTypeFixedGray4:
        case WICBitmapPaletteTypeFixedGray16:
        case WICBitmapPaletteTypeFixedGray256:
            *pfIsGrayscale = TRUE;
            break;
        default:
            *pfIsGrayscale = FALSE;
    }
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static HRESULT WINAPI PaletteImpl_HasAlpha(IWICPalette *iface, BOOL *pfHasAlpha)
{
    PaletteImpl *This = impl_from_IWICPalette(iface);
    UINT i;

    TRACE("(%p,%p)\n", iface, pfHasAlpha);

    if (!pfHasAlpha) return E_INVALIDARG;

    *pfHasAlpha = FALSE;

    EnterCriticalSection(&This->lock);
    for (i=0; i<This->count; i++)
        if ((This->colors[i]&0xff000000) != 0xff000000)
        {
            *pfHasAlpha = TRUE;
            break;
        }
    LeaveCriticalSection(&This->lock);

    return S_OK;
}

static const IWICPaletteVtbl PaletteImpl_Vtbl = {
    PaletteImpl_QueryInterface,
    PaletteImpl_AddRef,
    PaletteImpl_Release,
    PaletteImpl_InitializePredefined,
    PaletteImpl_InitializeCustom,
    PaletteImpl_InitializeFromBitmap,
    PaletteImpl_InitializeFromPalette,
    PaletteImpl_GetType,
    PaletteImpl_GetColorCount,
    PaletteImpl_GetColors,
    PaletteImpl_IsBlackWhite,
    PaletteImpl_IsGrayscale,
    PaletteImpl_HasAlpha
};

HRESULT PaletteImpl_Create(IWICPalette **palette)
{
    PaletteImpl *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PaletteImpl));
    if (!This) return E_OUTOFMEMORY;

    This->IWICPalette_iface.lpVtbl = &PaletteImpl_Vtbl;
    This->ref = 1;
    This->count = 0;
    This->colors = NULL;
    This->type = WICBitmapPaletteTypeCustom;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": PaletteImpl.lock");

    *palette = &This->IWICPalette_iface;

    return S_OK;
}
