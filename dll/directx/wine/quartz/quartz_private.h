/*              DirectShow private interfaces (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
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
#include "wine/strmbase.h"
#include "wine/list.h"

/* see IAsyncReader::Request on MSDN for the explanation of this */
#define MEDIATIME_FROM_BYTES(x) ((LONGLONG)(x) * 10000000)
#define SEC_FROM_MEDIATIME(time) ((time) / 10000000)
#define BYTES_FROM_MEDIATIME(time) SEC_FROM_MEDIATIME(time)
#define MSEC_FROM_MEDIATIME(time) ((time) / 10000)

HRESULT FilterGraph_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterGraphNoThread_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterMapper2_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT FilterMapper_create(IUnknown *pUnkOuter, LPVOID *ppObj) DECLSPEC_HIDDEN;
HRESULT AsyncReader_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT StdMemAllocator_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT AVISplitter_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT MPEGSplitter_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT AVIDec_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT DSoundRender_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VideoRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT NullRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VideoRendererDefault_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT QUARTZ_CreateSystemClock(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT ACMWrapper_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT WAVEParser_create(IUnknown * pUnkOuter, LPVOID * ppv) DECLSPEC_HIDDEN;
HRESULT VMR7Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;
HRESULT VMR9Impl_create(IUnknown *pUnkOuter, LPVOID *ppv) DECLSPEC_HIDDEN;

HRESULT EnumMonikerImpl_Create(IMoniker ** ppMoniker, ULONG nMonikerCount, IEnumMoniker ** ppEnum) DECLSPEC_HIDDEN;

HRESULT IEnumRegFiltersImpl_Construct(REGFILTER * pInRegFilters, const ULONG size, IEnumRegFilters ** ppEnum) DECLSPEC_HIDDEN;
HRESULT IEnumFiltersImpl_Construct(IGraphVersion * pVersionSource, IBaseFilter *** pppFilters, ULONG * pNumFilters, IEnumFilters ** ppEnum) DECLSPEC_HIDDEN;

extern const char * qzdebugstr_guid(const GUID * id) DECLSPEC_HIDDEN;
extern void video_unregister_windowclass(void) DECLSPEC_HIDDEN;

BOOL CompareMediaTypes(const AM_MEDIA_TYPE * pmt1, const AM_MEDIA_TYPE * pmt2, BOOL bWildcards);
void dump_AM_MEDIA_TYPE(const AM_MEDIA_TYPE * pmt) DECLSPEC_HIDDEN;

HRESULT GetClassMediaFile(IAsyncReader * pReader, LPCOLESTR pszFileName, GUID * majorType, GUID * minorType, GUID * sourceFilter);

#endif /* __QUARTZ_PRIVATE_INCLUDED__ */
