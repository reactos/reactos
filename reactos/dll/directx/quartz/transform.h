/*
 * Transform Filter declarations
 *
 * Copyright 2005 Christian Costa
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

#include "control_private.h"

typedef struct TransformFilterImpl TransformFilterImpl;

typedef struct TransformFuncsTable {
    HRESULT (*pfnProcessBegin) (TransformFilterImpl *This);
    HRESULT (*pfnProcessSampleData) (InputPin *pin, IMediaSample *pSample);
    HRESULT (*pfnProcessEnd) (TransformFilterImpl *This);
    HRESULT (*pfnQueryConnect) (TransformFilterImpl *This, const AM_MEDIA_TYPE * pmt);
    HRESULT (*pfnConnectInput) (InputPin *pin, const AM_MEDIA_TYPE * pmt);
    HRESULT (*pfnCleanup) (InputPin *pin);
} TransformFuncsTable;

struct TransformFilterImpl
{
    const IBaseFilterVtbl * lpVtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;
    CLSID clsid;
    struct MediaSeekingImpl mediaSeeking;

    IPin **ppPins;
    ULONG npins;
    AM_MEDIA_TYPE pmt;

    const TransformFuncsTable * pFuncsTable;
};

HRESULT TransformFilter_Create(TransformFilterImpl*, const CLSID*, const TransformFuncsTable* pFuncsTable, CHANGEPROC stop, CHANGEPROC current, CHANGEPROC rate);
