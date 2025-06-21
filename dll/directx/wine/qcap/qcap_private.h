/*
 * DirectShow capture
 *
 * Copyright (C) 2005 Rolf Kalbermatter
 * Copyright 2005 Maarten Lankhorst
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
#ifndef _QCAP_PRIVATE_H_DEFINED
#define _QCAP_PRIVATE_H_DEFINED

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include <stdbool.h>
#include "dshow.h"
#include "winternl.h"
#include "wine/unixlib.h"
#include "wine/debug.h"
#include "wine/strmbase.h"

HRESULT audio_record_create(IUnknown *outer, IUnknown **out);
HRESULT avi_compressor_create(IUnknown *outer, IUnknown **out);
HRESULT avi_mux_create(IUnknown *outer, IUnknown **out);
HRESULT capture_graph_create(IUnknown *outer, IUnknown **out);
HRESULT file_writer_create(IUnknown *outer, IUnknown **out);
HRESULT smart_tee_create(IUnknown *outer, IUnknown **out);
HRESULT vfw_capture_create(IUnknown *outer, IUnknown **out);

typedef UINT64 video_capture_device_t;

struct create_params
{
    unsigned int                  index;
    video_capture_device_t       *device;
};

struct start_params
{
    video_capture_device_t       device;
};

struct destroy_params
{
    video_capture_device_t       device;
};

struct check_format_params
{
    video_capture_device_t       device;
    const AM_MEDIA_TYPE         *mt;
};

struct set_format_params
{
    video_capture_device_t       device;
    const AM_MEDIA_TYPE         *mt;
};

struct get_format_params
{
    video_capture_device_t       device;
    AM_MEDIA_TYPE               *mt;
    VIDEOINFOHEADER             *format;
};

struct get_media_type_params
{
    video_capture_device_t       device;
    unsigned int                 index;
    AM_MEDIA_TYPE               *mt;
    VIDEOINFOHEADER             *format;
};

struct get_caps_params
{
    video_capture_device_t       device;
    unsigned int                 index;
    AM_MEDIA_TYPE               *mt;
    VIDEOINFOHEADER             *format;
    VIDEO_STREAM_CONFIG_CAPS    *caps;
};

struct get_caps_count_params
{
    video_capture_device_t       device;
    int                         *count;
};

struct get_prop_range_params
{
    video_capture_device_t       device;
    VideoProcAmpProperty         property;
    LONG                        *min;
    LONG                        *max;
    LONG                        *step;
    LONG                        *default_value;
    LONG                        *flags;
};

struct get_prop_params
{
    video_capture_device_t       device;
    VideoProcAmpProperty         property;
    LONG                        *value;
    LONG                        *flags;
};

struct set_prop_params
{
    video_capture_device_t       device;
    VideoProcAmpProperty         property;
    LONG                         value;
    LONG                         flags;
};

struct read_frame_params
{
    video_capture_device_t       device;
    void                        *data;
};

enum unix_funcs
{
    unix_create,
    unix_start,
    unix_destroy,
    unix_check_format,
    unix_set_format,
    unix_get_format,
    unix_get_media_type,
    unix_get_caps,
    unix_get_caps_count,
    unix_get_prop_range,
    unix_get_prop,
    unix_set_prop,
    unix_read_frame,
    unix_funcs_count
};

#endif
