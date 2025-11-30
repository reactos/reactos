/*
 * Implementation of MedaType utility functions
 *
 * Copyright 2003 Robert Shearman
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"
#include "dvdmedia.h"
#include "dxva.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const struct
{
    const GUID *guid;
    const char *name;
}
strmbase_guids[] =
{
#define X(g) {&(g), #g}
    X(GUID_NULL),

#undef OUR_GUID_ENTRY
#define OUR_GUID_ENTRY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) X(name),
#include "uuids.h"

#undef X
};

static const char *strmbase_debugstr_guid(const GUID *guid)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(strmbase_guids); ++i)
    {
        if (IsEqualGUID(strmbase_guids[i].guid, guid))
            return wine_dbg_sprintf("%s", strmbase_guids[i].name);
    }

    return debugstr_guid(guid);
}

void strmbase_dump_media_type(const AM_MEDIA_TYPE *mt)
{
    if (!TRACE_ON(quartz) || !mt) return;

    TRACE("Dumping media type %p: major type %s, subtype %s",
            mt, strmbase_debugstr_guid(&mt->majortype), strmbase_debugstr_guid(&mt->subtype));
    if (mt->bFixedSizeSamples) TRACE(", fixed size samples");
    if (mt->bTemporalCompression) TRACE(", temporal compression");
    if (mt->lSampleSize) TRACE(", sample size %ld", mt->lSampleSize);
    if (mt->pUnk) TRACE(", pUnk %p", mt->pUnk);
    TRACE(", format type %s.\n", strmbase_debugstr_guid(&mt->formattype));

    if (!mt->pbFormat) return;

    TRACE("Dumping format %p: ", mt->pbFormat);

    if (IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx) && mt->cbFormat >= sizeof(WAVEFORMATEX))
    {
        WAVEFORMATEX *wfx = (WAVEFORMATEX *)mt->pbFormat;

        TRACE("tag %#x, %u channels, sample rate %lu, %lu bytes/sec, alignment %u, %u bits/sample.\n",
                wfx->wFormatTag, wfx->nChannels, wfx->nSamplesPerSec,
                wfx->nAvgBytesPerSec, wfx->nBlockAlign, wfx->wBitsPerSample);

        if (wfx->cbSize)
        {
            const unsigned char *extra = (const unsigned char *)(wfx + 1);
            unsigned int i;

            TRACE("  Extra bytes:");
            for (i = 0; i < wfx->cbSize; ++i)
            {
                if (!(i % 16)) TRACE("\n     ");
                TRACE(" %02x", extra[i]);
            }
            TRACE("\n");
        }
    }
    else if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo) && mt->cbFormat >= sizeof(VIDEOINFOHEADER))
    {
        VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)mt->pbFormat;

        if (!IsRectEmpty(&vih->rcSource)) TRACE("source %s, ", wine_dbgstr_rect(&vih->rcSource));
        if (!IsRectEmpty(&vih->rcTarget)) TRACE("target %s, ", wine_dbgstr_rect(&vih->rcTarget));
        if (vih->dwBitRate) TRACE("bitrate %lu, ", vih->dwBitRate);
        if (vih->dwBitErrorRate) TRACE("error rate %lu, ", vih->dwBitErrorRate);
        TRACE("%s sec/frame, ", debugstr_time(vih->AvgTimePerFrame));
        TRACE("size %ldx%ld, %u planes, %u bpp, compression %s, image size %lu",
                vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, vih->bmiHeader.biPlanes,
                vih->bmiHeader.biBitCount, debugstr_fourcc(vih->bmiHeader.biCompression),
                vih->bmiHeader.biSizeImage);
        if (vih->bmiHeader.biXPelsPerMeter || vih->bmiHeader.biYPelsPerMeter)
            TRACE(", resolution %ldx%ld", vih->bmiHeader.biXPelsPerMeter, vih->bmiHeader.biYPelsPerMeter);
        if (vih->bmiHeader.biClrUsed) TRACE(", %lu colours", vih->bmiHeader.biClrUsed);
        if (vih->bmiHeader.biClrImportant) TRACE(", %lu important colours", vih->bmiHeader.biClrImportant);
        TRACE(".\n");
    }
    else if (IsEqualGUID(&mt->formattype, &FORMAT_VideoInfo2) && mt->cbFormat >= sizeof(VIDEOINFOHEADER2))
    {
        VIDEOINFOHEADER2 *vih = (VIDEOINFOHEADER2 *)mt->pbFormat;

        if (!IsRectEmpty(&vih->rcSource)) TRACE("source %s, ", wine_dbgstr_rect(&vih->rcSource));
        if (!IsRectEmpty(&vih->rcTarget)) TRACE("target %s, ", wine_dbgstr_rect(&vih->rcTarget));
        if (vih->dwBitRate) TRACE("bitrate %lu, ", vih->dwBitRate);
        if (vih->dwBitErrorRate) TRACE("error rate %lu, ", vih->dwBitErrorRate);
        TRACE("%s sec/frame, ", debugstr_time(vih->AvgTimePerFrame));
        if (vih->dwInterlaceFlags) TRACE("interlace flags %#lx, ", vih->dwInterlaceFlags);
        if (vih->dwCopyProtectFlags) TRACE("copy-protection flags %#lx, ", vih->dwCopyProtectFlags);
        TRACE("aspect ratio %lu/%lu, ", vih->dwPictAspectRatioX, vih->dwPictAspectRatioY);
        if (vih->dwControlFlags) TRACE("control flags %#lx, ", vih->dwControlFlags);
        if (vih->dwControlFlags & AMCONTROL_COLORINFO_PRESENT)
        {
            const DXVA_ExtendedFormat *colorimetry = (const DXVA_ExtendedFormat *)&vih->dwControlFlags;

            TRACE("chroma site %#x, range %#x, matrix %#x, lighting %#x, primaries %#x, transfer function %#x, ",
                    colorimetry->VideoChromaSubsampling, colorimetry->NominalRange, colorimetry->VideoTransferMatrix,
                    colorimetry->VideoLighting, colorimetry->VideoPrimaries, colorimetry->VideoTransferFunction);
        }
        TRACE("size %ldx%ld, %u planes, %u bpp, compression %s, image size %lu",
                vih->bmiHeader.biWidth, vih->bmiHeader.biHeight, vih->bmiHeader.biPlanes,
                vih->bmiHeader.biBitCount, debugstr_fourcc(vih->bmiHeader.biCompression),
                vih->bmiHeader.biSizeImage);
        if (vih->bmiHeader.biXPelsPerMeter || vih->bmiHeader.biYPelsPerMeter)
            TRACE(", resolution %ldx%ld", vih->bmiHeader.biXPelsPerMeter, vih->bmiHeader.biYPelsPerMeter);
        if (vih->bmiHeader.biClrUsed) TRACE(", %lu colours", vih->bmiHeader.biClrUsed);
        if (vih->bmiHeader.biClrImportant) TRACE(", %lu important colours", vih->bmiHeader.biClrImportant);
        TRACE(".\n");
    }
    else
        TRACE("not implemented for this format type.\n");
}

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE *dest, const AM_MEDIA_TYPE *src)
{
    *dest = *src;
    if (src->pbFormat)
    {
        dest->pbFormat = CoTaskMemAlloc(src->cbFormat);
        if (!dest->pbFormat)
            return E_OUTOFMEMORY;
        memcpy(dest->pbFormat, src->pbFormat, src->cbFormat);
    }
    if (dest->pUnk)
        IUnknown_AddRef(dest->pUnk);
    return S_OK;
}

void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType)
{
    CoTaskMemFree(pMediaType->pbFormat);
    pMediaType->pbFormat = NULL;
    if (pMediaType->pUnk)
    {
        IUnknown_Release(pMediaType->pUnk);
        pMediaType->pUnk = NULL;
    }
}

AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc)
{
    AM_MEDIA_TYPE * pDest;

    pDest = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!pDest)
        return NULL;

    if (FAILED(CopyMediaType(pDest, pSrc)))
    {
        CoTaskMemFree(pDest);
        return NULL;
    }

    return pDest;
}

void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType)
{
    FreeMediaType(pMediaType);
    CoTaskMemFree(pMediaType);
}
