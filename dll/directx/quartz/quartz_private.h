/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#ifndef __QUARTZ_PRIVATE_INCLUDED__
#define __QUARTZ_PRIVATE_INCLUDED__

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"
#include "wine/list.h"

#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)
#define SEC_FROM_MEDIATIME(time) ((time) / 10000000)
#define BYTES_FROM_MEDIATIME(time) SEC_FROM_MEDIATIME(time)
#define MSEC_FROM_MEDIATIME(time) ((time) / 10000)

#define ICOM_THIS_MULTI(impl,field,iface) impl* const This=(impl*)((char*)(iface) - offsetof(impl,field))

HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT FilterGraphNoThread_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT FilterMapper_create(IUnknown *pUnkOuter, LPVOID *ppObj);
HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT StdMemAllocator_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT MPEGSplitter_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT DSoundRender_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT NullRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT QUARTZ_CreateSystemClock(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT WAVEParser_create(IUnknown * pUnkOuter, LPVOID * ppv);
HRESULT SeekingPassThru_create(IUnknown *pUnkOuter, LPVOID *ppObj);

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum);

typedef struct tagENUMEDIADETAILS
{
	ULONG cMediaTypes;
	AM_MEDIA_TYPE * pMediaTypes;
} ENUMMEDIADETAILS;

typedef HRESULT (* FNOBTAINPIN)(IBaseFilter *iface, ULONG pos, IPin **pin, DWORD *lastsynctick);

HRESULT IEnumPinsImpl_Construct(IEnumPins ** ppEnum, FNOBTAINPIN receive_pin, IBaseFilter *base);
HRESULT IEnumMediaTypesImpl_Construct(const ENUMMEDIADETAILS * pDetails, IEnumMediaTypes ** ppEnum);
HRESULT IEnumRegFiltersImpl_Construct(REGFILTER * pInRegFilters, const ULONG size, IEnumRegFilters ** ppEnum);
HRESULT IEnumFiltersImpl_Construct(IBaseFilter ** ppFilters, ULONG nFilters, IEnumFilters ** ppEnum);

extern const char * qzdebugstr_guid(const GUID * id);
extern const char * qzdebugstr_State(FILTER_STATE state);

HRESULT CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void FreeMediaType(AM_MEDIA_TYPE * pmt);
void DeleteMediaType(AM_MEDIA_TYPE * pmt);
BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards);
void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt);
HRESULT updatehres( HRESULT original, HRESULT new );

typedef struct StdMediaSample2
{
    const IMediaSample2Vtbl * lpvtbl;

    LONG ref;
    AM_SAMPLE2_PROPERTIES props;
    IMemAllocator * pParent;
    struct list listentry;
    LONGLONG tMediaStart;
    LONGLONG tMediaEnd;
} StdMediaSample2;

typedef struct BaseMemAllocator
{
    const IMemAllocatorVtbl * lpVtbl;

    LONG ref;
    ALLOCATOR_PROPERTIES props;
    HRESULT (* fnAlloc) (IMemAllocator *);
    HRESULT (* fnFree)(IMemAllocator *);
    HRESULT (* fnVerify)(IMemAllocator *, ALLOCATOR_PROPERTIES *);
    HRESULT (* fnBufferPrepare)(IMemAllocator *, StdMediaSample2 *, DWORD flags);
    HRESULT (* fnBufferReleased)(IMemAllocator *, StdMediaSample2 *);
    void (* fnDestroyed)(IMemAllocator *);
    HANDLE hSemWaiting;
    BOOL bDecommitQueued;
    BOOL bCommitted;
    LONG lWaiting;
    struct list free_list;
    struct list used_list;
    CRITICAL_SECTION *pCritSect;
} BaseMemAllocator;

HRESULT BaseMemAllocator_Init(HRESULT (* fnAlloc)(IMemAllocator *),
                              HRESULT (* fnFree)(IMemAllocator *),
                              HRESULT (* fnVerify)(IMemAllocator *, ALLOCATOR_PROPERTIES *),
                              HRESULT (* fnBufferPrepare)(IMemAllocator *, StdMediaSample2 *, DWORD),
                              HRESULT (* fnBufferReleased)(IMemAllocator *, StdMediaSample2 *),
                              void (* fnDestroyed)(IMemAllocator *),
                              CRITICAL_SECTION *pCritSect,
                              BaseMemAllocator * pMemAlloc);

HRESULT StdMediaSample2_Construct(BYTE * pbBuffer, LONG cbBuffer, IMemAllocator * pParent, StdMediaSample2 ** ppSample);
void StdMediaSample2_Delete(StdMediaSample2 * This);

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
