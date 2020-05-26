/*
 * DirectShow capture services (QCAP.DLL)
 *
 * Copyright 2005 Maarten Lankhorst
 *
 * This file contains the part of the vfw capture interface that
 * does the actual Video4Linux(1/2) stuff required for capturing
 * and setting/getting media format..
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
#include "wine/port.h"

#define COBJMACROS

#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif
#ifdef HAVE_LIBV4L1_H
#include <libv4l1.h>
#endif
#ifdef HAVE_LINUX_VIDEODEV_H
#include <linux/videodev.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "wine/debug.h"
#include "wine/library.h"

#include "capture.h"
#include "qcap_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap_v4l);

#ifdef VIDIOCMCAPTURE

static typeof(open) *video_open = open;
static typeof(close) *video_close = close;
static typeof(ioctl) *video_ioctl = ioctl;
static typeof(read) *video_read = read;
static typeof(mmap) *video_mmap = mmap;
static typeof(munmap) *video_munmap = munmap;

static void video_init(void)
{
#ifdef SONAME_LIBV4L1
    static void *video_lib;

    if (video_lib)
        return;
    video_lib = wine_dlopen(SONAME_LIBV4L1, RTLD_NOW, NULL, 0);
    if (!video_lib)
        return;
    video_open = wine_dlsym(video_lib, "v4l1_open", NULL, 0);
    video_close = wine_dlsym(video_lib, "v4l1_close", NULL, 0);
    video_ioctl = wine_dlsym(video_lib, "v4l1_ioctl", NULL, 0);
    video_read = wine_dlsym(video_lib, "v4l1_read", NULL, 0);
    video_mmap = wine_dlsym(video_lib, "v4l1_mmap", NULL, 0);
    video_munmap = wine_dlsym(video_lib, "v4l1_munmap", NULL, 0);
#endif
}

typedef void (* Renderer)(const Capture *, LPBYTE bufferin, const BYTE *stream);

struct _Capture
{
    UINT width, height, bitDepth, fps, outputwidth, outputheight;
    BOOL swresize;

    CRITICAL_SECTION CritSect;

    IPin *pOut;
    int fd, mmap;
    BOOL iscommitted, stopped;
    struct video_picture pict;
    int dbrightness, dhue, dcolour, dcontrast;

    /* mmap (V4l1) */
    struct video_mmap *grab_buf;
    struct video_mbuf gb_buffers;
    unsigned char *pmap;
    int buffers;

    /* read (V4l1) */
    int imagesize;
    char * grab_data;

    int curframe;

    HANDLE thread;
    Renderer renderer;
};

struct renderlist
{
    int depth;
    const char* name;
    Renderer renderer;
};

static void renderer_RGB(const Capture *capBox, LPBYTE bufferin, const BYTE *stream);
static void renderer_YUV(const Capture *capBox, LPBYTE bufferin, const BYTE *stream);

static const struct renderlist renderlist_V4l[] = {
    {  0, "NULL renderer",               NULL },
    {  8, "Gray scales",                 NULL }, /* 1,  Don't support  */
    {  0, "High 240 cube (BT848)",       NULL }, /* 2,  Don't support  */
    { 16, "16 bit RGB (565)",            NULL }, /* 3,  Don't support  */
    { 24, "24 bit RGB values",   renderer_RGB }, /* 4,  Supported,     */
    { 32, "32 bit RGB values",   renderer_RGB }, /* 5,  Supported      */
    { 16, "15 bit RGB (555)",            NULL }, /* 6,  Don't support  */
    { 16, "YUV 422 (Not P)",     renderer_YUV }, /* 7,  Supported */
    { 16, "YUYV (Not P)",        renderer_YUV }, /* 8,  Supported */
    { 16, "UYVY (Not P)",        renderer_YUV }, /* 9,  Supported */
    { 16, "YUV 420 (Not P)", NULL }, /* 10, Not supported, if I had to guess it's YYUYYV */
    { 12, "YUV 411 (Not P)",     renderer_YUV }, /* 11, Supported */
    {  0, "Raw capturing (BT848)",       NULL }, /* 12, Don't support  */
    { 16, "YUV 422 (Planar)",    renderer_YUV }, /* 13, Supported */
    { 12, "YUV 411 (Planar)",    renderer_YUV }, /* 14, Supported */
    { 12, "YUV 420 (Planar)",    renderer_YUV }, /* 15, Supported */
    { 10, "YUV 410 (Planar)",    renderer_YUV }, /* 16, Supported */
    /* FIXME: add YUV420 support */
    {  0, NULL,                          NULL },
};

static const int fallback_V4l[] = { 4, 5, 7, 8, 9, 13, 15, 14, 16, 11, -1 };
/* Fallback: First try raw formats (Should try yuv first perhaps?), then yuv */

/* static const Capture defbox; */

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do {
        r = video_ioctl (fd, request, arg);
    } while (-1 == r && EINTR == errno);

    return r;
}

/* Prepare the capture buffers */
static HRESULT V4l_Prepare(Capture *capBox)
{
    TRACE("%p: Preparing for %dx%d resolution\n", capBox, capBox->width, capBox->height);

    /* Try mmap */
    capBox->mmap = 0;
    if (xioctl(capBox->fd, VIDIOCGMBUF, &capBox->gb_buffers) != -1 &&
        capBox->gb_buffers.frames)
    {
        capBox->buffers = capBox->gb_buffers.frames;
        if (capBox->gb_buffers.frames > 1)
            capBox->buffers = 1;
        TRACE("%p: Using %d/%d buffers\n", capBox,
              capBox->buffers, capBox->gb_buffers.frames);

        capBox->pmap = video_mmap( 0, capBox->gb_buffers.size, PROT_READ|PROT_WRITE,
                                   MAP_SHARED, capBox->fd, 0 );
        if (capBox->pmap != MAP_FAILED)
        {
            int i;

            capBox->grab_buf = CoTaskMemAlloc(sizeof(struct video_mmap) * capBox->buffers);
            if (!capBox->grab_buf)
            {
                video_munmap(capBox->pmap, capBox->gb_buffers.size);
                return E_OUTOFMEMORY;
            }

            /* Setup mmap capture buffers. */
            for (i = 0; i < capBox->buffers; i++)
            {
                capBox->grab_buf[i].format = capBox->pict.palette;
                capBox->grab_buf[i].frame = i;
                capBox->grab_buf[i].width = capBox->width;
                capBox->grab_buf[i].height = capBox->height;
            }
            capBox->mmap = 1;
        }
    }
    if (!capBox->mmap)
    {
        capBox->buffers = 1;
        capBox->imagesize = renderlist_V4l[capBox->pict.palette].depth *
                                capBox->height * capBox->width / 8;
        capBox->grab_data = CoTaskMemAlloc(capBox->imagesize);
        if (!capBox->grab_data)
            return E_OUTOFMEMORY;
    }
    TRACE("Using mmap: %d\n", capBox->mmap);
    return S_OK;
}

static void V4l_Unprepare(Capture *capBox)
{
    if (capBox->mmap)
    {
        for (capBox->curframe = 0; capBox->curframe < capBox->buffers; capBox->curframe++) 
            xioctl(capBox->fd, VIDIOCSYNC, &capBox->grab_buf[capBox->curframe]);
        video_munmap(capBox->pmap, capBox->gb_buffers.size);
        CoTaskMemFree(capBox->grab_buf);
    }
    else
        CoTaskMemFree(capBox->grab_data);
}

HRESULT qcap_driver_destroy(Capture *capBox)
{
    TRACE("%p\n", capBox);

    if( capBox->fd != -1 )
        video_close(capBox->fd);
    capBox->CritSect.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&capBox->CritSect);
    CoTaskMemFree(capBox);
    return S_OK;
}

HRESULT qcap_driver_set_format(Capture *capBox, AM_MEDIA_TYPE * mT)
{
    int newheight, newwidth;
    struct video_window window;
    VIDEOINFOHEADER *format;

    TRACE("%p\n", capBox);

    format = (VIDEOINFOHEADER *) mT->pbFormat;
    if (format->bmiHeader.biBitCount != 24 ||
        format->bmiHeader.biCompression != BI_RGB)
    {
        FIXME("unsupported media type %d %d\n", format->bmiHeader.biBitCount,
              format->bmiHeader.biCompression );
        return VFW_E_INVALIDMEDIATYPE;
    }

    newwidth = format->bmiHeader.biWidth;
    newheight = format->bmiHeader.biHeight;

    TRACE("%p -> (%p) - %d %d\n", capBox, mT, newwidth, newheight);

    if (capBox->height == newheight && capBox->width == newwidth)
        return S_OK;

    if(-1 == xioctl(capBox->fd, VIDIOCGWIN, &window))
    {
        ERR("ioctl(VIDIOCGWIN) failed (%d)\n", errno);
        return E_FAIL;
    }
    window.width = newwidth;
    window.height = newheight;
    if (xioctl(capBox->fd, VIDIOCSWIN, &window) == -1)
    {
        TRACE("using software resize: %dx%d -> %dx%d\n",
               window.width, window.height, capBox->width, capBox->height);
        capBox->swresize = TRUE;
    }
    else
    {
        capBox->height = window.height;
        capBox->width = window.width;
        capBox->swresize = FALSE;
    }
    capBox->outputwidth = window.width;
    capBox->outputheight = window.height;
    return S_OK;
}

HRESULT qcap_driver_get_format(const Capture *capBox, AM_MEDIA_TYPE ** mT)
{
    VIDEOINFOHEADER *vi;

    mT[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
    if (!mT[0])
        return E_OUTOFMEMORY;
    vi = CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    mT[0]->cbFormat = sizeof(VIDEOINFOHEADER);
    if (!vi)
    {
        CoTaskMemFree(mT[0]);
        mT[0] = NULL;
        return E_OUTOFMEMORY;
    }
    mT[0]->majortype = MEDIATYPE_Video;
    mT[0]->subtype = MEDIASUBTYPE_RGB24;
    mT[0]->formattype = FORMAT_VideoInfo;
    mT[0]->bFixedSizeSamples = TRUE;
    mT[0]->bTemporalCompression = FALSE;
    mT[0]->pUnk = NULL;
    mT[0]->lSampleSize = capBox->outputwidth * capBox->outputheight * capBox->bitDepth / 8;
    TRACE("Output format: %dx%d - %d bits = %u KB\n", capBox->outputwidth,
          capBox->outputheight, capBox->bitDepth, mT[0]->lSampleSize/1024);
    vi->rcSource.left = 0; vi->rcSource.top = 0;
    vi->rcTarget.left = 0; vi->rcTarget.top = 0;
    vi->rcSource.right = capBox->width; vi->rcSource.bottom = capBox->height;
    vi->rcTarget.right = capBox->outputwidth; vi->rcTarget.bottom = capBox->outputheight;
    vi->dwBitRate = capBox->fps * mT[0]->lSampleSize;
    vi->dwBitErrorRate = 0;
    vi->AvgTimePerFrame = (LONGLONG)10000000.0 / (LONGLONG)capBox->fps;
    vi->bmiHeader.biSize = 40;
    vi->bmiHeader.biWidth = capBox->outputwidth;
    vi->bmiHeader.biHeight = capBox->outputheight;
    vi->bmiHeader.biPlanes = 1;
    vi->bmiHeader.biBitCount = 24;
    vi->bmiHeader.biCompression = BI_RGB;
    vi->bmiHeader.biSizeImage = mT[0]->lSampleSize;
    vi->bmiHeader.biClrUsed = vi->bmiHeader.biClrImportant = 0;
    vi->bmiHeader.biXPelsPerMeter = 100;
    vi->bmiHeader.biYPelsPerMeter = 100;
    mT[0]->pbFormat = (void *)vi;
    dump_AM_MEDIA_TYPE(mT[0]);
    return S_OK;
}

HRESULT qcap_driver_get_prop_range( Capture *capBox,
            VideoProcAmpProperty Property, LONG *pMin, LONG *pMax,
            LONG *pSteppingDelta, LONG *pDefault, LONG *pCapsFlags )
{
    TRACE("%p -> %d %p %p %p %p %p\n", capBox, Property,
          pMin, pMax, pSteppingDelta, pDefault, pCapsFlags);

    switch (Property)
    {
    case VideoProcAmp_Brightness:
        *pDefault = capBox->dbrightness;
        break;
    case VideoProcAmp_Contrast:
        *pDefault = capBox->dcontrast;
        break;
    case VideoProcAmp_Hue:
        *pDefault = capBox->dhue;
        break;
    case VideoProcAmp_Saturation:
        *pDefault = capBox->dcolour;
        break;
    default:
        FIXME("Not implemented %d\n", Property);
        return E_NOTIMPL;
    }
    *pMin = 0;
    *pMax = 65535;
    *pSteppingDelta = 65536/256;
    *pCapsFlags = VideoProcAmp_Flags_Manual;
    return S_OK;
}

HRESULT qcap_driver_get_prop( Capture *capBox,
            VideoProcAmpProperty Property, LONG *lValue, LONG *Flags )
{
    TRACE("%p -> %d %p %p\n", capBox, Property, lValue, Flags);

    switch (Property)
    {
    case VideoProcAmp_Brightness:
        *lValue = capBox->pict.brightness;
        break;
    case VideoProcAmp_Contrast:
        *lValue = capBox->pict.contrast;
        break;
    case VideoProcAmp_Hue:
        *lValue = capBox->pict.hue;
        break;
    case VideoProcAmp_Saturation:
        *lValue = capBox->pict.colour;
        break;
    default:
        FIXME("Not implemented %d\n", Property);
        return E_NOTIMPL;
    }
    *Flags = VideoProcAmp_Flags_Manual;
    return S_OK;
}

HRESULT qcap_driver_set_prop(Capture *capBox, VideoProcAmpProperty Property,
            LONG lValue, LONG Flags)
{
    TRACE("%p -> %d %d %d\n", capBox, Property, lValue, Flags);

    switch (Property)
    {
    case VideoProcAmp_Brightness:
        capBox->pict.brightness = lValue;
        break;
    case VideoProcAmp_Contrast:
        capBox->pict.contrast = lValue;
        break;
    case VideoProcAmp_Hue:
        capBox->pict.hue = lValue;
        break;
    case VideoProcAmp_Saturation:
        capBox->pict.colour = lValue;
        break;
    default:
        FIXME("Not implemented %d\n", Property);
        return E_NOTIMPL;
    }

    if (xioctl(capBox->fd, VIDIOCSPICT, &capBox->pict) == -1)
    {
        ERR("ioctl(VIDIOCSPICT) failed (%d)\n",errno);
        return E_FAIL;
    }
    return S_OK;
}

static void renderer_RGB(const Capture *capBox, LPBYTE bufferin, const BYTE *stream)
{
    int depth = renderlist_V4l[capBox->pict.palette].depth;
    int size = capBox->height * capBox->width * depth / 8;
    int pointer, offset;

    switch (depth)
    {
    case 24:
        memcpy(bufferin, stream, size);
        break;
    case 32:
        pointer = 0;
        offset = 1;
        while (pointer + offset <= size)
        {
            bufferin[pointer] = stream[pointer + offset];
            pointer++;
            bufferin[pointer] = stream[pointer + offset];
            pointer++;
            bufferin[pointer] = stream[pointer + offset];
            pointer++;
            offset++;
        }
        break;
    default:
        ERR("Unknown bit depth %d\n", depth);
        return;
    }
}

static void renderer_YUV(const Capture *capBox, LPBYTE bufferin, const BYTE *stream)
{
    enum YUV_Format format;

    switch (capBox->pict.palette)
    {
    case  7: /* YUV422  -  same as YUYV */
    case  8: /* YUYV    */
        format = YUYV;
        break;
    case  9: /* UYVY    */
        format = UYVY;
        break;
    case 11: /* YUV411  */
        format = UYYVYY;
        break;
    case 13: /* YUV422P */
        format = YUVP_421;
        break;
    case 14: /* YUV411P */
        format = YUVP_441;
        break;
    case 15: /* YUV420P */
        format = YUVP_422;
        break;
    case 16: /* YUV410P */
        format = YUVP_444;
        break;
    default:
        ERR("Unknown palette %d\n", capBox->pict.palette);
        return;
    }
    YUV_To_RGB24(format, bufferin, stream, capBox->width, capBox->height);
}

static void Resize(const Capture * capBox, LPBYTE output, const BYTE *input)
{
    /* the whole image needs to be reversed,
       because the dibs are messed up in windows */
    if (!capBox->swresize)
    {
        int depth = capBox->bitDepth / 8;
        int inoffset = 0, outoffset = capBox->height * capBox->width * depth;
        int ow = capBox->width * depth;
        while (outoffset > 0)
        {
            int x;
            outoffset -= ow;
            for (x = 0; x < ow; x++)
                output[outoffset + x] = input[inoffset + x];
            inoffset += ow;
        }
    }
    else
    {
        HDC dc_s, dc_d;
        HBITMAP bmp_s, bmp_d;
        int depth = capBox->bitDepth / 8;
        int inoffset = 0, outoffset = (capBox->outputheight) * capBox->outputwidth * depth;
        int ow = capBox->outputwidth * depth;
        LPBYTE myarray;

        /* FIXME: Improve software resizing: add error checks and optimize */

        myarray = CoTaskMemAlloc(capBox->outputwidth * capBox->outputheight * depth);
        dc_s = CreateCompatibleDC(NULL);
        dc_d = CreateCompatibleDC(NULL);
        bmp_s = CreateBitmap(capBox->width, capBox->height, 1, capBox->bitDepth, input);
        bmp_d = CreateBitmap(capBox->outputwidth, capBox->outputheight, 1, capBox->bitDepth, NULL);
        SelectObject(dc_s, bmp_s);
        SelectObject(dc_d, bmp_d);
        StretchBlt(dc_d, 0, 0, capBox->outputwidth, capBox->outputheight,
                   dc_s, 0, 0, capBox->width, capBox->height, SRCCOPY);
        GetBitmapBits(bmp_d, capBox->outputwidth * capBox->outputheight * depth, myarray);
        while (outoffset > 0)
        {
            int i;

            outoffset -= ow;
            for (i = 0; i < ow; i++)
                output[outoffset + i] = myarray[inoffset + i];
            inoffset += ow;
        }
        CoTaskMemFree(myarray);
        DeleteObject(dc_s);
        DeleteObject(dc_d);
        DeleteObject(bmp_s);
        DeleteObject(bmp_d);
    }
}

static void V4l_GetFrame(Capture * capBox, unsigned char ** pInput)
{
    if (capBox->mmap)
    {
        if (xioctl(capBox->fd, VIDIOCSYNC, &capBox->grab_buf[capBox->curframe]) == -1)
            WARN("Syncing ioctl failed: %d\n", errno);

        *pInput = capBox->pmap + capBox->gb_buffers.offsets[capBox->curframe];
    }
    else
    {
        int retval;
        while ((retval = video_read(capBox->fd, capBox->grab_data, capBox->imagesize)) == -1)
            if (errno != EAGAIN) break;
        if (retval == -1)
            WARN("Error occurred while reading from device: %s\n", strerror(errno));
        *pInput = (unsigned char*) capBox->grab_data;
    }
}

static void V4l_FreeFrame(Capture * capBox)
{
    TRACE("\n");
    if (capBox->mmap)
    {
        if (xioctl(capBox->fd, VIDIOCMCAPTURE, &capBox->grab_buf[capBox->curframe]) == -1)
           ERR("Freeing frame for capture failed: %s\n", strerror(errno));
    }
    if (++capBox->curframe == capBox->buffers)
        capBox->curframe = 0;
}

static DWORD WINAPI ReadThread(LPVOID lParam)
{
    Capture * capBox = lParam;
    HRESULT hr;
    IMediaSample *pSample = NULL;
    ULONG framecount = 0;
    unsigned char *pTarget, *pInput, *pOutput;

    hr = V4l_Prepare(capBox);
    if (FAILED(hr))
    {
        ERR("Stop IFilterGraph: %x\n", hr);
        capBox->thread = 0;
        capBox->stopped = TRUE;
        return 0;
    }

    pOutput = CoTaskMemAlloc(capBox->width * capBox->height * capBox->bitDepth / 8);
    capBox->curframe = 0;
    do {
        V4l_FreeFrame(capBox);
    } while (capBox->curframe != 0);

    while (1)
    {
        EnterCriticalSection(&capBox->CritSect);
        if (capBox->stopped)
            break;
        hr = BaseOutputPinImpl_GetDeliveryBuffer((BaseOutputPin *)capBox->pOut, &pSample, NULL, NULL, 0);
        if (SUCCEEDED(hr))
        {
            int len;
            
            if (!capBox->swresize)
                len = capBox->height * capBox->width * capBox->bitDepth / 8;
            else
                len = capBox->outputheight * capBox->outputwidth * capBox->bitDepth / 8;
            IMediaSample_SetActualDataLength(pSample, len);

            len = IMediaSample_GetActualDataLength(pSample);
            TRACE("Data length: %d KB\n", len / 1024);

            IMediaSample_GetPointer(pSample, &pTarget);
            /* FIXME: Check return values.. */
            V4l_GetFrame(capBox, &pInput);
            capBox->renderer(capBox, pOutput, pInput);
            Resize(capBox, pTarget, pOutput);
            hr = BaseOutputPinImpl_Deliver((BaseOutputPin *)capBox->pOut, pSample);
            TRACE("%p -> Frame %u: %x\n", capBox, ++framecount, hr);
            IMediaSample_Release(pSample);
            V4l_FreeFrame(capBox);
        }
        if (FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
        {
            TRACE("Return %x, stop IFilterGraph\n", hr);
            V4l_Unprepare(capBox);
            capBox->thread = 0;
            capBox->stopped = TRUE;
            break;
        }
        LeaveCriticalSection(&capBox->CritSect);
    }

    LeaveCriticalSection(&capBox->CritSect);
    CoTaskMemFree(pOutput);
    return 0;
}

HRESULT qcap_driver_run(Capture *capBox, FILTER_STATE *state)
{
    HANDLE thread;
    HRESULT hr;

    TRACE("%p -> (%p)\n", capBox, state); 

    if (*state == State_Running) return S_OK;

    EnterCriticalSection(&capBox->CritSect);

    capBox->stopped = FALSE;

    if (*state == State_Stopped)
    {
        *state = State_Running;
        if (!capBox->iscommitted)
        {
            ALLOCATOR_PROPERTIES ap, actual;
            BaseOutputPin *out;

            capBox->iscommitted = TRUE;

            ap.cBuffers = 3;
            if (!capBox->swresize)
                ap.cbBuffer = capBox->width * capBox->height;
            else
                ap.cbBuffer = capBox->outputwidth * capBox->outputheight;
            ap.cbBuffer = (ap.cbBuffer * capBox->bitDepth) / 8;
            ap.cbAlign = 1;
            ap.cbPrefix = 0;

            out = (BaseOutputPin *)capBox->pOut;

            hr = IMemAllocator_SetProperties(out->pAllocator, &ap, &actual);

            if (SUCCEEDED(hr))
                hr = IMemAllocator_Commit(out->pAllocator);

            TRACE("Committing allocator: %x\n", hr);
        }

        thread = CreateThread(NULL, 0, ReadThread, capBox, 0, NULL);
        if (thread)
        {
            capBox->thread = thread;
            SetThreadPriority(thread, THREAD_PRIORITY_LOWEST);
            LeaveCriticalSection(&capBox->CritSect);
            return S_OK;
        }
        ERR("Creating thread failed.. %u\n", GetLastError());
        LeaveCriticalSection(&capBox->CritSect);
        return E_FAIL;
    }

    ResumeThread(capBox->thread);
    *state = State_Running;
    LeaveCriticalSection(&capBox->CritSect);
    return S_OK;
}

HRESULT qcap_driver_pause(Capture *capBox, FILTER_STATE *state)
{
    TRACE("%p -> (%p)\n", capBox, state);     

    if (*state == State_Paused)
        return S_OK;
    if (*state == State_Stopped)
        qcap_driver_run(capBox, state);

    EnterCriticalSection(&capBox->CritSect);
    *state = State_Paused;
    SuspendThread(capBox->thread);
    LeaveCriticalSection(&capBox->CritSect);

    return S_OK;
}

HRESULT qcap_driver_stop(Capture *capBox, FILTER_STATE *state)
{
    TRACE("%p -> (%p)\n", capBox, state);

    if (*state == State_Stopped)
        return S_OK;

    EnterCriticalSection(&capBox->CritSect);

    if (capBox->thread)
    {
        if (*state == State_Paused)
            ResumeThread(capBox->thread);
        capBox->stopped = TRUE;
        capBox->thread = 0;
        if (capBox->iscommitted)
        {
            BaseOutputPin *out;
            HRESULT hr;

            capBox->iscommitted = FALSE;

            out = (BaseOutputPin*)capBox->pOut;

            hr = IMemAllocator_Decommit(out->pAllocator);

            if (hr != S_OK && hr != VFW_E_NOT_COMMITTED)
                WARN("Decommitting allocator: %x\n", hr);
        }
        V4l_Unprepare(capBox);
    }

    *state = State_Stopped;
    LeaveCriticalSection(&capBox->CritSect);
    return S_OK;
}

Capture * qcap_driver_init( IPin *pOut, USHORT card )
{
    Capture * capBox = NULL;
    char device[20];
    struct video_capability capa;
    struct video_picture pict;
    struct video_window window;

    YUV_Init();
    video_init();

    capBox = CoTaskMemAlloc(sizeof(Capture));
    if (!capBox)
        goto error;

    /* capBox->vtbl = &defboxVtbl; */

    InitializeCriticalSection( &capBox->CritSect );
    capBox->CritSect.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": Capture.CritSect");

    sprintf(device, "/dev/video%i", card);
    TRACE("opening %s\n", device);
#ifdef O_CLOEXEC
    if ((capBox->fd = video_open(device, O_RDWR | O_NONBLOCK | O_CLOEXEC)) == -1 && errno == EINVAL)
#endif
        capBox->fd = video_open(device, O_RDWR | O_NONBLOCK);
    if (capBox->fd == -1)
    {
        WARN("open failed (%d)\n", errno);
        goto error;
    }
    fcntl( capBox->fd, F_SETFD, FD_CLOEXEC );  /* in case O_CLOEXEC isn't supported */

    memset(&capa, 0, sizeof(capa));

    if (xioctl(capBox->fd, VIDIOCGCAP, &capa) == -1)
    {
        WARN("ioctl(VIDIOCGCAP) failed (%d)\n", errno);
        goto error;
    }

    if (!(capa.type & VID_TYPE_CAPTURE))
    {
        WARN("not a video capture device\n");
        goto error;
    }

    TRACE("%d inputs on %s\n", capa.channels, capa.name );

    if (xioctl(capBox->fd, VIDIOCGPICT, &pict) == -1)
    {
        ERR("ioctl(VIDIOCGPICT) failed (%d)\n", errno );
        goto error;
    }

    TRACE("depth %d palette %d (%s) hue %d color %d contrast %d\n",
          pict.depth, pict.palette, renderlist_V4l[pict.palette].name,
          pict.hue, pict.colour, pict.contrast );

    capBox->dbrightness = pict.brightness;
    capBox->dcolour = pict.colour;
    capBox->dhue = pict.hue;
    capBox->dcontrast = pict.contrast;

    if (!renderlist_V4l[pict.palette].renderer)
    {
        int palet = pict.palette, i;

        TRACE("No renderer available for %s, falling back to defaults\n",
             renderlist_V4l[pict.palette].name);
        capBox->renderer = NULL;
        for (i = 0; fallback_V4l[i] >=0 ; i++)
        {
            int n = fallback_V4l[i];

            if (renderlist_V4l[n].renderer == NULL)
                continue;

            pict.depth = renderlist_V4l[n].depth;
            pict.palette = n;
            if (xioctl(capBox->fd, VIDIOCSPICT, &pict) == -1)
            {
                TRACE("Could not render with %s (%d)\n",
                      renderlist_V4l[n].name, n);
                continue;
            }
            TRACE("using renderer %s (%d)\n", 
                  renderlist_V4l[n].name, n);
            capBox->renderer = renderlist_V4l[n].renderer;
            break;
        }

        if (!capBox->renderer)
        {
            ERR("video format %s isn't available\n",
                 renderlist_V4l[palet].name);
            goto error;
        }
    }
    else
    {
        TRACE("Using the suggested format\n");
        capBox->renderer = renderlist_V4l[pict.palette].renderer;
    }
    memcpy(&capBox->pict, &pict, sizeof(struct video_picture));

    memset(&window, 0, sizeof(window));
    if (xioctl(capBox->fd, VIDIOCGWIN, &window) == -1)
    {
        WARN("VIDIOCGWIN failed (%d)\n", errno);
        goto error;
    }

    capBox->height = capBox->outputheight = window.height;
    capBox->width = capBox->outputwidth = window.width;
    capBox->swresize = FALSE;
    capBox->bitDepth = 24;
    capBox->pOut = pOut;
    capBox->fps = 3;
    capBox->stopped = FALSE;
    capBox->curframe = 0;
    capBox->iscommitted = FALSE;

    TRACE("format: %d bits - %d x %d\n", capBox->bitDepth, capBox->width, capBox->height);

    return capBox;

error:
    if (capBox)
        qcap_driver_destroy( capBox );

    return NULL;
}

#else

Capture * qcap_driver_init( IPin *pOut, USHORT card )
{
    const char msg[] = 
        "The v4l headers were not available at compile time,\n"
        "so video capture support is not available.\n";
    MESSAGE(msg);
    return NULL;
}

#define FAIL_WITH_ERR \
    ERR("v4l absent: shouldn't be called\n"); \
    return E_NOTIMPL

HRESULT qcap_driver_destroy(Capture *capBox)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_set_format(Capture *capBox, AM_MEDIA_TYPE * mT)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_format(const Capture *capBox, AM_MEDIA_TYPE ** mT)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_prop_range( Capture *capBox,
        VideoProcAmpProperty  Property, LONG *pMin, LONG *pMax,
        LONG *pSteppingDelta, LONG *pDefault, LONG *pCapsFlags )
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_get_prop(Capture *capBox,
        VideoProcAmpProperty Property, LONG *lValue, LONG *Flags)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_set_prop(Capture *capBox, VideoProcAmpProperty Property,
        LONG lValue, LONG Flags)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_run(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_pause(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

HRESULT qcap_driver_stop(Capture *capBox, FILTER_STATE *state)
{
    FAIL_WITH_ERR;
}

#endif /* defined(VIDIOCMCAPTURE) */
