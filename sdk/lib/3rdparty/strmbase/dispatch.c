/*
 * ITypeInfo cache for IDispatch
 *
 * Copyright 2019 Zebediah Figura
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

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static ITypeLib *control_typelib;
static ITypeInfo *control_typeinfo[last_tid];

static REFIID control_tid_id[] =
{
    &IID_IBasicAudio,
    &IID_IBasicVideo,
    &IID_IMediaControl,
    &IID_IMediaEvent,
    &IID_IMediaPosition,
    &IID_IVideoWindow,
};

HRESULT strmbase_get_typeinfo(enum strmbase_type_id tid, ITypeInfo **ret)
{
    HRESULT hr;

    if (!control_typelib)
    {
        ITypeLib *typelib;

        hr = LoadRegTypeLib(&LIBID_QuartzTypeLib, 1, 0, LOCALE_SYSTEM_DEFAULT, &typelib);
        if (FAILED(hr))
        {
            ERR("Failed to load typelib, hr %#lx.\n", hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer((void **)&control_typelib, typelib, NULL))
            ITypeLib_Release(typelib);
    }
    if (!control_typeinfo[tid])
    {
        ITypeInfo *typeinfo;

        hr = ITypeLib_GetTypeInfoOfGuid(control_typelib, control_tid_id[tid], &typeinfo);
        if (FAILED(hr))
        {
            ERR("Failed to get type info for %s, hr %#lx.\n", debugstr_guid(control_tid_id[tid]), hr);
            return hr;
        }
        if (InterlockedCompareExchangePointer((void **)(control_typeinfo + tid), typeinfo, NULL))
            ITypeInfo_Release(typeinfo);
    }
    ITypeInfo_AddRef(*ret = control_typeinfo[tid]);
    return S_OK;
}

void strmbase_release_typelibs(void)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(control_typeinfo); ++i)
    {
        if (control_typeinfo[i])
            ITypeInfo_Release(control_typeinfo[i]);
    }
    if (control_typelib)
        ITypeLib_Release(control_typelib);
}
