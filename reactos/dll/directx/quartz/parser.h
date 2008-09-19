/*
 * Parser declarations
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

typedef struct ParserImpl ParserImpl;

typedef HRESULT (*PFN_PROCESS_SAMPLE) (LPVOID iface, IMediaSample * pSample, DWORD_PTR cookie);
typedef HRESULT (*PFN_QUERY_ACCEPT) (LPVOID iface, const AM_MEDIA_TYPE * pmt);
typedef HRESULT (*PFN_PRE_CONNECT) (IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *prop);
typedef HRESULT (*PFN_CLEANUP) (LPVOID iface);
typedef HRESULT (*PFN_DISCONNECT) (LPVOID iface);

struct ParserImpl
{
    const IBaseFilterVtbl *lpVtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    PFN_CLEANUP fnCleanup;
    PFN_DISCONNECT fnDisconnect;
    FILTER_INFO filterInfo;
    CLSID clsid;

    PullPin * pInputPin;
    IPin ** ppPins;
    ULONG cStreams;
    DWORD lastpinchange;
    MediaSeekingImpl mediaSeeking;
};

typedef struct Parser_OutputPin
{
    OutputPin pin;

    AM_MEDIA_TYPE * pmt;
    LONGLONG dwSamplesProcessed;
} Parser_OutputPin;

extern HRESULT Parser_AddPin(ParserImpl * This, const PIN_INFO * piOutput, ALLOCATOR_PROPERTIES * props, const AM_MEDIA_TYPE * amt);

extern HRESULT Parser_Create(ParserImpl*, const IBaseFilterVtbl *, const CLSID*, PFN_PROCESS_SAMPLE, PFN_QUERY_ACCEPT, PFN_PRE_CONNECT,
                             PFN_CLEANUP, PFN_DISCONNECT, REQUESTPROC, STOPPROCESSPROC, CHANGEPROC stop, CHANGEPROC current, CHANGEPROC rate);

/* Override the _Release function and call this when releasing */
extern void Parser_Destroy(ParserImpl *This);

extern HRESULT WINAPI Parser_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
extern ULONG WINAPI Parser_AddRef(IBaseFilter * iface);
extern ULONG WINAPI Parser_Release(IBaseFilter * iface);
extern HRESULT WINAPI Parser_GetClassID(IBaseFilter * iface, CLSID * pClsid);
extern HRESULT WINAPI Parser_Stop(IBaseFilter * iface);
extern HRESULT WINAPI Parser_Pause(IBaseFilter * iface);
extern HRESULT WINAPI Parser_Run(IBaseFilter * iface, REFERENCE_TIME tStart);
extern HRESULT WINAPI Parser_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState);
extern HRESULT WINAPI Parser_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock);
extern HRESULT WINAPI Parser_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock);
extern HRESULT WINAPI Parser_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum);
extern HRESULT WINAPI Parser_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin);
extern HRESULT WINAPI Parser_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo);
extern HRESULT WINAPI Parser_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName);
extern HRESULT WINAPI Parser_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo);
