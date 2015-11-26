/*
 * Header file for private strmbase implementations
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#ifndef _STRMBASE_PCH_
#define _STRMBASE_PCH_

#include <wine/config.h>

#include <assert.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define WIN32_LEAN_AND_MEAN

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <dshow.h>

#include <wine/debug.h>
#include <wine/strmbase.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(strmbase);

/* Quality Control */
typedef struct QualityControlImpl {
    IQualityControl IQualityControl_iface;
    IPin *input;
    IBaseFilter *self;
    IQualityControl *tonotify;

    /* Render stuff */
    IReferenceClock *clock;
    REFERENCE_TIME last_in_time, last_left, avg_duration, avg_pt, avg_render, start, stop;
    REFERENCE_TIME current_jitter, current_rstart, current_rstop, clockstart;
    double avg_rate;
    LONG64 rendered, dropped;
    BOOL qos_handled, is_dropped;
} QualityControlImpl;

HRESULT QualityControlImpl_Create(IPin *input, IBaseFilter *self, QualityControlImpl **ppv);
void QualityControlImpl_Destroy(QualityControlImpl *This);
HRESULT WINAPI QualityControlImpl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv);
ULONG WINAPI QualityControlImpl_AddRef(IQualityControl *iface);
ULONG WINAPI QualityControlImpl_Release(IQualityControl *iface);
HRESULT WINAPI QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm);
HRESULT WINAPI QualityControlImpl_SetSink(IQualityControl *iface, IQualityControl *tonotify);

void QualityControlRender_Start(QualityControlImpl *This, REFERENCE_TIME tStart);
void QualityControlRender_SetClock(QualityControlImpl *This, IReferenceClock *clock);
HRESULT QualityControlRender_WaitFor(QualityControlImpl *This, IMediaSample *sample, HANDLE ev);
void QualityControlRender_DoQOS(QualityControlImpl *priv);
void QualityControlRender_BeginRender(QualityControlImpl *This);
void QualityControlRender_EndRender(QualityControlImpl *This);

#endif /* _STRMBASE_PCH_ */
