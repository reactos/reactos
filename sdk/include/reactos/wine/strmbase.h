/*
 * Header file for Wine's strmbase implementation
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

#include "wine/list.h"

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE * pDest, const AM_MEDIA_TYPE *pSrc);
void WINAPI FreeMediaType(AM_MEDIA_TYPE * pMediaType);
AM_MEDIA_TYPE * WINAPI CreateMediaType(AM_MEDIA_TYPE const * pSrc);
void WINAPI DeleteMediaType(AM_MEDIA_TYPE * pMediaType);

/* Pin functions */

typedef struct BasePin
{
	IPin IPin_iface;
	LONG refCount;
	LPCRITICAL_SECTION pCritSec;
	PIN_INFO pinInfo;
	IPin * pConnectedTo;
	AM_MEDIA_TYPE mtCurrent;
	REFERENCE_TIME tStart;
	REFERENCE_TIME tStop;
	double dRate;

	const struct BasePinFuncTable* pFuncsTable;
} BasePin;

typedef HRESULT (WINAPI *BasePin_CheckMediaType)(BasePin *This, const AM_MEDIA_TYPE *pmt);
typedef HRESULT (WINAPI *BasePin_AttemptConnection)(BasePin *This, IPin *pReceivePin, const AM_MEDIA_TYPE *pmt);
typedef LONG (WINAPI *BasePin_GetMediaTypeVersion)(BasePin *This);
typedef HRESULT (WINAPI *BasePin_GetMediaType)(BasePin *This, int iPosition, AM_MEDIA_TYPE *amt);

typedef struct BasePinFuncTable {
	/* Required for Input Pins*/
	BasePin_CheckMediaType pfnCheckMediaType;
	/* Required for Output Pins*/
	BasePin_AttemptConnection pfnAttemptConnection;
	/* Required for BasePinImpl_EnumMediaTypes */
	BasePin_GetMediaTypeVersion pfnGetMediaTypeVersion;
	BasePin_GetMediaType pfnGetMediaType;
} BasePinFuncTable;

typedef struct BaseOutputPin
{
	/* inheritance C style! */
	BasePin pin;
	IMemInputPin * pMemInputPin;
	IMemAllocator * pAllocator;

	const struct BaseOutputPinFuncTable* pFuncsTable;
} BaseOutputPin;

typedef HRESULT (WINAPI *BaseOutputPin_DecideBufferSize)(BaseOutputPin *This, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
typedef HRESULT (WINAPI *BaseOutputPin_DecideAllocator)(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc);
typedef HRESULT (WINAPI *BaseOutputPin_BreakConnect)(BaseOutputPin * This);

typedef struct BaseOutputPinFuncTable {
	BasePinFuncTable base;

	/* Required for BaseOutputPinImpl_DecideAllocator */
	BaseOutputPin_DecideBufferSize pfnDecideBufferSize;
	/* Required for BaseOutputPinImpl_AttemptConnection */
	BaseOutputPin_DecideAllocator pfnDecideAllocator;
	BaseOutputPin_BreakConnect pfnBreakConnect;
} BaseOutputPinFuncTable;

typedef struct BaseInputPin
{
	/* inheritance C style! */
	BasePin pin;

	IMemInputPin IMemInputPin_iface;
	IMemAllocator * pAllocator;
	BOOL flushing, end_of_stream;
	IMemAllocator *preferred_allocator;

	const struct BaseInputPinFuncTable* pFuncsTable;
} BaseInputPin;

typedef HRESULT (WINAPI *BaseInputPin_Receive)(BaseInputPin *This, IMediaSample *pSample);

typedef struct BaseInputPinFuncTable {
	BasePinFuncTable base;
	/* Optional */
	BaseInputPin_Receive pfnReceive;
} BaseInputPinFuncTable;

/* Base Pin */
HRESULT WINAPI BasePinImpl_GetMediaType(BasePin *This, int iPosition, AM_MEDIA_TYPE *pmt);
LONG WINAPI BasePinImpl_GetMediaTypeVersion(BasePin *This);
ULONG WINAPI BasePinImpl_AddRef(IPin * iface);
HRESULT WINAPI BasePinImpl_Disconnect(IPin * iface);
HRESULT WINAPI BasePinImpl_ConnectedTo(IPin * iface, IPin ** ppPin);
HRESULT WINAPI BasePinImpl_ConnectionMediaType(IPin * iface, AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_QueryPinInfo(IPin * iface, PIN_INFO * pInfo);
HRESULT WINAPI BasePinImpl_QueryDirection(IPin * iface, PIN_DIRECTION * pPinDir);
HRESULT WINAPI BasePinImpl_QueryId(IPin * iface, LPWSTR * Id);
HRESULT WINAPI BasePinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BasePinImpl_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum);
HRESULT WINAPI BasePinImpl_QueryInternalConnections(IPin * iface, IPin ** apPin, ULONG * cPin);
HRESULT WINAPI BasePinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

/* Base Output Pin */
HRESULT WINAPI BaseOutputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI BaseOutputPinImpl_Release(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_Connect(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseOutputPinImpl_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseOutputPinImpl_Disconnect(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_EndOfStream(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_BeginFlush(IPin * iface);
HRESULT WINAPI BaseOutputPinImpl_EndFlush(IPin * iface);

HRESULT WINAPI BaseOutputPinImpl_GetDeliveryBuffer(BaseOutputPin * This, IMediaSample ** ppSample, REFERENCE_TIME * tStart, REFERENCE_TIME * tStop, DWORD dwFlags);
HRESULT WINAPI BaseOutputPinImpl_Deliver(BaseOutputPin * This, IMediaSample * pSample);
HRESULT WINAPI BaseOutputPinImpl_BreakConnect(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_Active(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_Inactive(BaseOutputPin * This);
HRESULT WINAPI BaseOutputPinImpl_InitAllocator(BaseOutputPin *This, IMemAllocator **pMemAlloc);
HRESULT WINAPI BaseOutputPinImpl_DecideAllocator(BaseOutputPin *This, IMemInputPin *pPin, IMemAllocator **pAlloc);
HRESULT WINAPI BaseOutputPinImpl_AttemptConnection(BasePin *This, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);

HRESULT WINAPI BaseOutputPin_Construct(const IPinVtbl *OutputPin_Vtbl, LONG outputpin_size, const PIN_INFO * pPinInfo, const BaseOutputPinFuncTable* pBaseOutputFuncsTable, LPCRITICAL_SECTION pCritSec, IPin ** ppPin);
HRESULT WINAPI BaseOutputPin_Destroy(BaseOutputPin *This);

/* Base Input Pin */
HRESULT WINAPI BaseInputPinImpl_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI BaseInputPinImpl_Release(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_Connect(IPin * iface, IPin * pConnector, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI BaseInputPinImpl_EndOfStream(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_BeginFlush(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_EndFlush(IPin * iface);
HRESULT WINAPI BaseInputPinImpl_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

HRESULT BaseInputPin_Construct(const IPinVtbl *InputPin_Vtbl, LONG inputpin_size, const PIN_INFO * pPinInfo,
        const BaseInputPinFuncTable* pBaseInputFuncsTable,
        LPCRITICAL_SECTION pCritSec, IMemAllocator *, IPin ** ppPin);
HRESULT WINAPI BaseInputPin_Destroy(BaseInputPin *This);

typedef struct BaseFilter
{
	IBaseFilter IBaseFilter_iface;
	LONG refCount;
	CRITICAL_SECTION csFilter;

	FILTER_STATE state;
	REFERENCE_TIME rtStreamStart;
	IReferenceClock * pClock;
	FILTER_INFO filterInfo;
	CLSID clsid;
	LONG pinVersion;

	const struct BaseFilterFuncTable* pFuncsTable;
} BaseFilter;

typedef IPin* (WINAPI *BaseFilter_GetPin)(BaseFilter* iface, int iPosition);
typedef LONG (WINAPI *BaseFilter_GetPinCount)(BaseFilter* iface);
typedef LONG (WINAPI *BaseFilter_GetPinVersion)(BaseFilter* iface);

typedef struct BaseFilterFuncTable {
	/* Required */
	BaseFilter_GetPin pfnGetPin;
	BaseFilter_GetPinCount pfnGetPinCount;
} BaseFilterFuncTable;

HRESULT WINAPI BaseFilterImpl_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI BaseFilterImpl_AddRef(IBaseFilter * iface);
ULONG WINAPI BaseFilterImpl_Release(IBaseFilter * iface);
HRESULT WINAPI BaseFilterImpl_GetClassID(IBaseFilter * iface, CLSID * pClsid);
HRESULT WINAPI BaseFilterImpl_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState );
HRESULT WINAPI BaseFilterImpl_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock);
HRESULT WINAPI BaseFilterImpl_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock);
HRESULT WINAPI BaseFilterImpl_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum);
HRESULT WINAPI BaseFilterImpl_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo);
HRESULT WINAPI BaseFilterImpl_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName );
HRESULT WINAPI BaseFilterImpl_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo);

LONG WINAPI BaseFilterImpl_GetPinVersion(BaseFilter* This);
VOID WINAPI BaseFilterImpl_IncrementPinVersion(BaseFilter* This);

HRESULT WINAPI BaseFilter_Init(BaseFilter * This, const IBaseFilterVtbl *Vtbl, const CLSID *pClsid, DWORD_PTR DebugInfo, const BaseFilterFuncTable* pBaseFuncsTable);
HRESULT WINAPI BaseFilter_Destroy(BaseFilter * This);

/* Enums */
HRESULT WINAPI EnumMediaTypes_Construct(BasePin *iface, BasePin_GetMediaType enumFunc, BasePin_GetMediaTypeVersion versionFunc, IEnumMediaTypes ** ppEnum);

HRESULT WINAPI EnumPins_Construct(BaseFilter *base,  BaseFilter_GetPin receive_pin, BaseFilter_GetPinCount receive_pincount, BaseFilter_GetPinVersion receive_version, IEnumPins ** ppEnum);

/* Transform Filter */
typedef struct TransformFilter
{
	BaseFilter filter;

	IPin **ppPins;
	ULONG npins;
	AM_MEDIA_TYPE pmt;
	CRITICAL_SECTION csReceive;

	const struct TransformFilterFuncTable * pFuncsTable;
	struct QualityControlImpl *qcimpl;
        /* IMediaSeeking and IMediaPosition are implemented by ISeekingPassThru */
        IUnknown *seekthru_unk;
} TransformFilter;

typedef HRESULT (WINAPI *TransformFilter_DecideBufferSize) (TransformFilter *iface, IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *ppropInputRequest);
typedef HRESULT (WINAPI *TransformFilter_StartStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_StopStreaming) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_Receive) (TransformFilter* iface, IMediaSample* pIn);
typedef HRESULT (WINAPI *TransformFilter_CompleteConnect) (TransformFilter *iface, PIN_DIRECTION dir, IPin *pPin);
typedef HRESULT (WINAPI *TransformFilter_BreakConnect) (TransformFilter *iface, PIN_DIRECTION dir);
typedef HRESULT (WINAPI *TransformFilter_SetMediaType) (TransformFilter *iface, PIN_DIRECTION dir, const AM_MEDIA_TYPE *pMediaType);
typedef HRESULT (WINAPI *TransformFilter_CheckInputType) (TransformFilter *iface, const AM_MEDIA_TYPE *pMediaType);
typedef HRESULT (WINAPI *TransformFilter_EndOfStream) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_BeginFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_EndFlush) (TransformFilter *iface);
typedef HRESULT (WINAPI *TransformFilter_NewSegment) (TransformFilter *iface,
REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);
typedef HRESULT (WINAPI *TransformFilter_Notify) (TransformFilter *iface, IBaseFilter *sender, Quality qm);

typedef struct TransformFilterFuncTable {
	/* Required */
	TransformFilter_DecideBufferSize pfnDecideBufferSize;
	/* Optional */
	TransformFilter_StartStreaming pfnStartStreaming;
	TransformFilter_Receive pfnReceive;
	TransformFilter_StopStreaming pfnStopStreaming;
	TransformFilter_CheckInputType pfnCheckInputType;
	TransformFilter_SetMediaType pfnSetMediaType;
	TransformFilter_CompleteConnect pfnCompleteConnect;
	TransformFilter_BreakConnect pfnBreakConnect;
	TransformFilter_EndOfStream pfnEndOfStream;
	TransformFilter_BeginFlush pfnBeginFlush;
	TransformFilter_EndFlush pfnEndFlush;
	TransformFilter_NewSegment pfnNewSegment;
	TransformFilter_Notify pfnNotify;
} TransformFilterFuncTable;

HRESULT WINAPI TransformFilterImpl_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
ULONG WINAPI TransformFilterImpl_Release(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Stop(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Pause(IBaseFilter * iface);
HRESULT WINAPI TransformFilterImpl_Run(IBaseFilter * iface, REFERENCE_TIME tStart);
HRESULT WINAPI TransformFilterImpl_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin);
HRESULT WINAPI TransformFilterImpl_Notify(TransformFilter *iface, IBaseFilter *sender, Quality qm);

HRESULT TransformFilter_Construct( const IBaseFilterVtbl *filterVtbl, LONG filter_size, const CLSID* pClsid, const TransformFilterFuncTable* pFuncsTable, IBaseFilter ** ppTransformFilter);

/* Source Seeking */
typedef HRESULT (WINAPI *SourceSeeking_ChangeRate)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStart)(IMediaSeeking *iface);
typedef HRESULT (WINAPI *SourceSeeking_ChangeStop)(IMediaSeeking *iface);

typedef struct SourceSeeking
{
	IMediaSeeking IMediaSeeking_iface;

	ULONG refCount;
	SourceSeeking_ChangeStop fnChangeStop;
	SourceSeeking_ChangeStart fnChangeStart;
	SourceSeeking_ChangeRate fnChangeRate;
	DWORD dwCapabilities;
	double dRate;
	LONGLONG llCurrent, llStop, llDuration;
	GUID timeformat;
	PCRITICAL_SECTION crst;
} SourceSeeking;

HRESULT SourceSeeking_Init(SourceSeeking *pSeeking, const IMediaSeekingVtbl *Vtbl, SourceSeeking_ChangeStop fnChangeStop, SourceSeeking_ChangeStart fnChangeStart, SourceSeeking_ChangeRate fnChangeRate, PCRITICAL_SECTION crit_sect);

HRESULT WINAPI SourceSeekingImpl_GetCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_CheckCapabilities(IMediaSeeking * iface, DWORD * pCapabilities);
HRESULT WINAPI SourceSeekingImpl_IsFormatSupported(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_QueryPreferredFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetTimeFormat(IMediaSeeking * iface, GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_IsUsingTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_SetTimeFormat(IMediaSeeking * iface, const GUID * pFormat);
HRESULT WINAPI SourceSeekingImpl_GetDuration(IMediaSeeking * iface, LONGLONG * pDuration);
HRESULT WINAPI SourceSeekingImpl_GetStopPosition(IMediaSeeking * iface, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetCurrentPosition(IMediaSeeking * iface, LONGLONG * pCurrent);
HRESULT WINAPI SourceSeekingImpl_ConvertTimeFormat(IMediaSeeking * iface, LONGLONG * pTarget, const GUID * pTargetFormat, LONGLONG Source, const GUID * pSourceFormat);
HRESULT WINAPI SourceSeekingImpl_SetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, DWORD dwCurrentFlags, LONGLONG * pStop, DWORD dwStopFlags);
HRESULT WINAPI SourceSeekingImpl_GetPositions(IMediaSeeking * iface, LONGLONG * pCurrent, LONGLONG * pStop);
HRESULT WINAPI SourceSeekingImpl_GetAvailable(IMediaSeeking * iface, LONGLONG * pEarliest, LONGLONG * pLatest);
HRESULT WINAPI SourceSeekingImpl_SetRate(IMediaSeeking * iface, double dRate);
HRESULT WINAPI SourceSeekingImpl_GetRate(IMediaSeeking * iface, double * dRate);
HRESULT WINAPI SourceSeekingImpl_GetPreroll(IMediaSeeking * iface, LONGLONG * pPreroll);

/* PosPassThru */
HRESULT WINAPI RendererPosPassThru_RegisterMediaTime(IUnknown *iface, REFERENCE_TIME start);
HRESULT WINAPI RendererPosPassThru_ResetMediaTime(IUnknown *iface);
HRESULT WINAPI RendererPosPassThru_EOS(IUnknown *iface);

HRESULT WINAPI CreatePosPassThru(IUnknown* pUnkOuter, BOOL bRenderer, IPin *pPin, IUnknown **ppPassThru);
HRESULT WINAPI PosPassThru_Construct(IUnknown* pUnkOuter, LPVOID *ppPassThru);

/* Filter Registration */

typedef REGPINTYPES AMOVIESETUP_MEDIATYPE;
typedef REGFILTERPINS AMOVIESETUP_PIN;

typedef struct AMOVIESETUP_FILTER {
	const CLSID *clsid;
	const WCHAR *name;
	DWORD merit;
	UINT pins;
	const AMOVIESETUP_PIN *pPin;
} AMOVIESETUP_FILTER, *LPAMOVIESETUP_FILTER;

typedef IUnknown *(CALLBACK *LPFNNewCOMObject)(LPUNKNOWN pUnkOuter, HRESULT *phr);
typedef void (CALLBACK *LPFNInitRoutine)(BOOL bLoading, const CLSID *rclsid);

typedef struct tagFactoryTemplate {
	const WCHAR *m_Name;
	const CLSID *m_ClsID;
	LPFNNewCOMObject m_lpfnNew;
	LPFNInitRoutine m_lpfnInit;
	const AMOVIESETUP_FILTER *m_pAMovieSetup_Filter;
} FactoryTemplate;

HRESULT WINAPI AMovieDllRegisterServer2(BOOL bRegister);
HRESULT WINAPI AMovieSetupRegisterFilter2(const AMOVIESETUP_FILTER *pFilter, IFilterMapper2  *pIFM2, BOOL  bRegister);

/* Output Queue */
typedef struct tagOutputQueue {
    CRITICAL_SECTION csQueue;

    BaseOutputPin * pInputPin;

    HANDLE hThread;
    HANDLE hProcessQueue;

    LONG lBatchSize;
    BOOL bBatchExact;
    BOOL bTerminate;
    BOOL bSendAnyway;

    struct list SampleList;

    const struct OutputQueueFuncTable* pFuncsTable;
} OutputQueue;

typedef DWORD (WINAPI *OutputQueue_ThreadProc)(OutputQueue *This);

typedef struct OutputQueueFuncTable
{
    OutputQueue_ThreadProc pfnThreadProc;
} OutputQueueFuncTable;

HRESULT WINAPI OutputQueue_Construct( BaseOutputPin *pInputPin, BOOL bAuto,
    BOOL bQueue, LONG lBatchSize, BOOL bBatchExact, DWORD dwPriority,
    const OutputQueueFuncTable* pFuncsTable, OutputQueue **ppOutputQueue );
HRESULT WINAPI OutputQueue_Destroy(OutputQueue *pOutputQueue);
HRESULT WINAPI OutputQueue_ReceiveMultiple(OutputQueue *pOutputQueue, IMediaSample **ppSamples, LONG nSamples, LONG *nSamplesProcessed);
HRESULT WINAPI OutputQueue_Receive(OutputQueue *pOutputQueue, IMediaSample *pSample);
VOID WINAPI OutputQueue_EOS(OutputQueue *pOutputQueue);
VOID WINAPI OutputQueue_SendAnyway(OutputQueue *pOutputQueue);
DWORD WINAPI OutputQueueImpl_ThreadProc(OutputQueue *pOutputQueue);

typedef struct tagBaseWindow
{
	HWND hWnd;
	LONG Width;
	LONG Height;
	HINSTANCE hInstance;
	LPWSTR pClassName;
	DWORD ClassStyles;
	DWORD WindowStyles;
	DWORD WindowStylesEx;
	HDC hDC;

	const struct BaseWindowFuncTable* pFuncsTable;
} BaseWindow;

typedef LPWSTR (WINAPI *BaseWindow_GetClassWindowStyles)(BaseWindow *This, DWORD *pClassStyles, DWORD *pWindowStyles, DWORD *pWindowStylesEx);
typedef RECT (WINAPI *BaseWindow_GetDefaultRect)(BaseWindow *This);
typedef BOOL (WINAPI *BaseWindow_PossiblyEatMessage)(BaseWindow *This, UINT uMsg, WPARAM wParam, LPARAM lParam);
typedef LRESULT (WINAPI *BaseWindow_OnReceiveMessage)(BaseWindow *This, HWND hwnd, INT uMsg, WPARAM wParam, LPARAM lParam);
typedef BOOL (WINAPI *BaseWindow_OnSize)(BaseWindow *This, LONG Height, LONG Width);

typedef struct BaseWindowFuncTable
{
	/* Required */
	BaseWindow_GetClassWindowStyles pfnGetClassWindowStyles;
	BaseWindow_GetDefaultRect pfnGetDefaultRect;
	/* Optional, WinProc Related */
	BaseWindow_OnReceiveMessage pfnOnReceiveMessage;
	BaseWindow_PossiblyEatMessage pfnPossiblyEatMessage;
	BaseWindow_OnSize pfnOnSize;
} BaseWindowFuncTable;

HRESULT WINAPI BaseWindow_Init(BaseWindow *pBaseWindow, const BaseWindowFuncTable* pFuncsTable);
HRESULT WINAPI BaseWindow_Destroy(BaseWindow *pBaseWindow);

HRESULT WINAPI BaseWindowImpl_PrepareWindow(BaseWindow *This);
HRESULT WINAPI BaseWindowImpl_DoneWithWindow(BaseWindow *This);
RECT WINAPI BaseWindowImpl_GetDefaultRect(BaseWindow *This);
LRESULT WINAPI BaseWindowImpl_OnReceiveMessage(BaseWindow *This, HWND hwnd, INT uMsg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI BaseWindowImpl_OnSize(BaseWindow *This, LONG Height, LONG Width);

typedef struct{
    ITypeInfo *pTypeInfo;
} BaseDispatch;

HRESULT WINAPI BaseDispatch_Init(BaseDispatch *This, REFIID riid);
HRESULT WINAPI BaseDispatch_Destroy(BaseDispatch *This);
HRESULT WINAPI BaseDispatchImpl_GetIDsOfNames(BaseDispatch *This, REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
HRESULT WINAPI BaseDispatchImpl_GetTypeInfo(BaseDispatch *This, REFIID riid, UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
HRESULT WINAPI BaseDispatchImpl_GetTypeInfoCount(BaseDispatch *This, UINT *pctinfo);

#ifdef __IVideoWindow_FWD_DEFINED__
typedef struct tagBaseControlWindow
{
	BaseWindow baseWindow;
	IVideoWindow IVideoWindow_iface;
	BaseDispatch baseDispatch;

	BOOL AutoShow;
	HWND hwndDrain;
	HWND hwndOwner;
	BaseFilter* pFilter;
	CRITICAL_SECTION* pInterfaceLock;
	BasePin*  pPin;
} BaseControlWindow;

HRESULT WINAPI BaseControlWindow_Init(BaseControlWindow *pControlWindow, const IVideoWindowVtbl *lpVtbl, BaseFilter *owner, CRITICAL_SECTION *lock, BasePin* pPin, const BaseWindowFuncTable* pFuncsTable);
HRESULT WINAPI BaseControlWindow_Destroy(BaseControlWindow *pControlWindow);

BOOL WINAPI BaseControlWindowImpl_PossiblyEatMessage(BaseWindow *This, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfoCount(IVideoWindow *iface, UINT*pctinfo);
HRESULT WINAPI BaseControlWindowImpl_GetTypeInfo(IVideoWindow *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo);
HRESULT WINAPI BaseControlWindowImpl_GetIDsOfNames(IVideoWindow *iface, REFIID riid, LPOLESTR*rgszNames, UINT cNames, LCID lcid, DISPID*rgDispId);
HRESULT WINAPI BaseControlWindowImpl_Invoke(IVideoWindow *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS*pDispParams, VARIANT*pVarResult, EXCEPINFO*pExepInfo, UINT*puArgErr);
HRESULT WINAPI BaseControlWindowImpl_put_Caption(IVideoWindow *iface, BSTR strCaption);
HRESULT WINAPI BaseControlWindowImpl_get_Caption(IVideoWindow *iface, BSTR *strCaption);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle);
HRESULT WINAPI BaseControlWindowImpl_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx);
HRESULT WINAPI BaseControlWindowImpl_put_AutoShow(IVideoWindow *iface, LONG AutoShow);
HRESULT WINAPI BaseControlWindowImpl_get_AutoShow(IVideoWindow *iface, LONG *AutoShow);
HRESULT WINAPI BaseControlWindowImpl_put_WindowState(IVideoWindow *iface, LONG WindowState);
HRESULT WINAPI BaseControlWindowImpl_get_WindowState(IVideoWindow *iface, LONG *WindowState);
HRESULT WINAPI BaseControlWindowImpl_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_get_BackgroundPalette(IVideoWindow *iface, LONG *pBackgroundPalette);
HRESULT WINAPI BaseControlWindowImpl_put_Visible(IVideoWindow *iface, LONG Visible);
HRESULT WINAPI BaseControlWindowImpl_get_Visible(IVideoWindow *iface, LONG *pVisible);
HRESULT WINAPI BaseControlWindowImpl_put_Left(IVideoWindow *iface, LONG Left);
HRESULT WINAPI BaseControlWindowImpl_get_Left(IVideoWindow *iface, LONG *pLeft);
HRESULT WINAPI BaseControlWindowImpl_put_Width(IVideoWindow *iface, LONG Width);
HRESULT WINAPI BaseControlWindowImpl_get_Width(IVideoWindow *iface, LONG *pWidth);
HRESULT WINAPI BaseControlWindowImpl_put_Top(IVideoWindow *iface, LONG Top);
HRESULT WINAPI BaseControlWindowImpl_get_Top(IVideoWindow *iface, LONG *pTop);

HRESULT WINAPI BaseControlWindowImpl_put_Height(IVideoWindow *iface, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_get_Height(IVideoWindow *iface, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_put_Owner(IVideoWindow *iface, OAHWND Owner);
HRESULT WINAPI BaseControlWindowImpl_get_Owner(IVideoWindow *iface, OAHWND *Owner);
HRESULT WINAPI BaseControlWindowImpl_put_MessageDrain(IVideoWindow *iface, OAHWND Drain);
HRESULT WINAPI BaseControlWindowImpl_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain);
HRESULT WINAPI BaseControlWindowImpl_get_BorderColor(IVideoWindow *iface, LONG *Color);
HRESULT WINAPI BaseControlWindowImpl_put_BorderColor(IVideoWindow *iface, LONG Color);
HRESULT WINAPI BaseControlWindowImpl_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode);
HRESULT WINAPI BaseControlWindowImpl_SetWindowForeground(IVideoWindow *iface, LONG Focus);
HRESULT WINAPI BaseControlWindowImpl_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top, LONG Width, LONG Height);
HRESULT WINAPI BaseControlWindowImpl_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg, LONG_PTR wParam, LONG_PTR lParam);
HRESULT WINAPI BaseControlWindowImpl_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlWindowImpl_HideCursor(IVideoWindow *iface, LONG HideCursor);
HRESULT WINAPI BaseControlWindowImpl_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden);
#endif

#ifdef __IBasicVideo_FWD_DEFINED__
#ifdef __amvideo_h__
typedef struct tagBaseControlVideo
{
	IBasicVideo IBasicVideo_iface;
	BaseDispatch baseDispatch;

	BaseFilter* pFilter;
	CRITICAL_SECTION* pInterfaceLock;
	BasePin*  pPin;

	const struct BaseControlVideoFuncTable* pFuncsTable;
} BaseControlVideo;

typedef HRESULT (WINAPI *BaseControlVideo_GetSourceRect)(BaseControlVideo* This, RECT *pSourceRect);
typedef HRESULT (WINAPI *BaseControlVideo_GetStaticImage)(BaseControlVideo* This, LONG *pBufferSize, LONG *pDIBImage);
typedef HRESULT (WINAPI *BaseControlVideo_GetTargetRect)(BaseControlVideo* This, RECT *pTargetRect);
typedef VIDEOINFOHEADER* (WINAPI *BaseControlVideo_GetVideoFormat)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_IsDefaultSourceRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_IsDefaultTargetRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetDefaultSourceRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetDefaultTargetRect)(BaseControlVideo* This);
typedef HRESULT (WINAPI *BaseControlVideo_SetSourceRect)(BaseControlVideo* This, RECT *pSourceRect);
typedef HRESULT (WINAPI *BaseControlVideo_SetTargetRect)(BaseControlVideo* This, RECT *pTargetRect);

typedef struct BaseControlVideoFuncTable {
	/* Required */
	BaseControlVideo_GetSourceRect pfnGetSourceRect;
	BaseControlVideo_GetStaticImage pfnGetStaticImage;
	BaseControlVideo_GetTargetRect pfnGetTargetRect;
	BaseControlVideo_GetVideoFormat pfnGetVideoFormat;
	BaseControlVideo_IsDefaultSourceRect pfnIsDefaultSourceRect;
	BaseControlVideo_IsDefaultTargetRect pfnIsDefaultTargetRect;
	BaseControlVideo_SetDefaultSourceRect pfnSetDefaultSourceRect;
	BaseControlVideo_SetDefaultTargetRect pfnSetDefaultTargetRect;
	BaseControlVideo_SetSourceRect pfnSetSourceRect;
	BaseControlVideo_SetTargetRect pfnSetTargetRect;
} BaseControlVideoFuncTable;

HRESULT WINAPI BaseControlVideo_Init(BaseControlVideo *pControlVideo, const IBasicVideoVtbl *lpVtbl, BaseFilter *owner, CRITICAL_SECTION *lock, BasePin* pPin, const BaseControlVideoFuncTable* pFuncsTable);
HRESULT WINAPI BaseControlVideo_Destroy(BaseControlVideo *pControlVideo);

HRESULT WINAPI BaseControlVideoImpl_GetTypeInfoCount(IBasicVideo *iface, UINT*pctinfo);
HRESULT WINAPI BaseControlVideoImpl_GetTypeInfo(IBasicVideo *iface, UINT iTInfo, LCID lcid, ITypeInfo**ppTInfo);
HRESULT WINAPI BaseControlVideoImpl_GetIDsOfNames(IBasicVideo *iface, REFIID riid, LPOLESTR*rgszNames, UINT cNames, LCID lcid, DISPID*rgDispId);
HRESULT WINAPI BaseControlVideoImpl_Invoke(IBasicVideo *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS*pDispParams, VARIANT*pVarResult, EXCEPINFO*pExepInfo, UINT*puArgErr);
HRESULT WINAPI BaseControlVideoImpl_get_AvgTimePerFrame(IBasicVideo *iface, REFTIME *pAvgTimePerFrame);
HRESULT WINAPI BaseControlVideoImpl_get_BitRate(IBasicVideo *iface, LONG *pBitRate);
HRESULT WINAPI BaseControlVideoImpl_get_BitErrorRate(IBasicVideo *iface, LONG *pBitErrorRate);
HRESULT WINAPI BaseControlVideoImpl_get_VideoWidth(IBasicVideo *iface, LONG *pVideoWidth);
HRESULT WINAPI BaseControlVideoImpl_get_VideoHeight(IBasicVideo *iface, LONG *pVideoHeight);
HRESULT WINAPI BaseControlVideoImpl_put_SourceLeft(IBasicVideo *iface, LONG SourceLeft);
HRESULT WINAPI BaseControlVideoImpl_get_SourceLeft(IBasicVideo *iface, LONG *pSourceLeft);
HRESULT WINAPI BaseControlVideoImpl_put_SourceWidth(IBasicVideo *iface, LONG SourceWidth);
HRESULT WINAPI BaseControlVideoImpl_get_SourceWidth(IBasicVideo *iface, LONG *pSourceWidth);
HRESULT WINAPI BaseControlVideoImpl_put_SourceTop(IBasicVideo *iface, LONG SourceTop);
HRESULT WINAPI BaseControlVideoImpl_get_SourceTop(IBasicVideo *iface, LONG *pSourceTop);
HRESULT WINAPI BaseControlVideoImpl_put_SourceHeight(IBasicVideo *iface, LONG SourceHeight);
HRESULT WINAPI BaseControlVideoImpl_get_SourceHeight(IBasicVideo *iface, LONG *pSourceHeight);
HRESULT WINAPI BaseControlVideoImpl_put_DestinationLeft(IBasicVideo *iface, LONG DestinationLeft);
HRESULT WINAPI BaseControlVideoImpl_get_DestinationLeft(IBasicVideo *iface, LONG *pDestinationLeft);
HRESULT WINAPI BaseControlVideoImpl_put_DestinationWidth(IBasicVideo *iface, LONG DestinationWidth);
HRESULT WINAPI BaseControlVideoImpl_get_DestinationWidth(IBasicVideo *iface, LONG *pDestinationWidth);
HRESULT WINAPI BaseControlVideoImpl_put_DestinationTop(IBasicVideo *iface, LONG DestinationTop);
HRESULT WINAPI BaseControlVideoImpl_get_DestinationTop(IBasicVideo *iface, LONG *pDestinationTop);
HRESULT WINAPI BaseControlVideoImpl_put_DestinationHeight(IBasicVideo *iface, LONG DestinationHeight);
HRESULT WINAPI BaseControlVideoImpl_get_DestinationHeight(IBasicVideo *iface, LONG *pDestinationHeight);
HRESULT WINAPI BaseControlVideoImpl_SetSourcePosition(IBasicVideo *iface, LONG Left, LONG Top, LONG Width, LONG Height);
HRESULT WINAPI BaseControlVideoImpl_GetSourcePosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlVideoImpl_SetDefaultSourcePosition(IBasicVideo *iface);
HRESULT WINAPI BaseControlVideoImpl_SetDestinationPosition(IBasicVideo *iface, LONG Left, LONG Top, LONG Width, LONG Height);
HRESULT WINAPI BaseControlVideoImpl_GetDestinationPosition(IBasicVideo *iface, LONG *pLeft, LONG *pTop, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlVideoImpl_SetDefaultDestinationPosition(IBasicVideo *iface);
HRESULT WINAPI BaseControlVideoImpl_GetVideoSize(IBasicVideo *iface, LONG *pWidth, LONG *pHeight);
HRESULT WINAPI BaseControlVideoImpl_GetVideoPaletteEntries(IBasicVideo *iface, LONG StartIndex, LONG Entries, LONG *pRetrieved, LONG *pPalette);
HRESULT WINAPI BaseControlVideoImpl_GetCurrentImage(IBasicVideo *iface, LONG *pBufferSize, LONG *pDIBImage);
HRESULT WINAPI BaseControlVideoImpl_IsUsingDefaultSource(IBasicVideo *iface);
HRESULT WINAPI BaseControlVideoImpl_IsUsingDefaultDestination(IBasicVideo *iface);
#endif
#endif

/* BaseRenderer Filter */
typedef struct BaseRendererTag
{
	BaseFilter filter;

	BaseInputPin *pInputPin;
	IUnknown *pPosition;
	CRITICAL_SECTION csRenderLock;
	HANDLE evComplete;
	HANDLE ThreadSignal;
	HANDLE RenderEvent;
	IMediaSample *pMediaSample;

	IQualityControl *pQSink;
	struct QualityControlImpl *qcimpl;

	const struct BaseRendererFuncTable * pFuncsTable;
} BaseRenderer;

typedef HRESULT (WINAPI *BaseRenderer_CheckMediaType)(BaseRenderer *This, const AM_MEDIA_TYPE *pmt);
typedef HRESULT (WINAPI *BaseRenderer_DoRenderSample)(BaseRenderer *This, IMediaSample *pMediaSample);
typedef VOID (WINAPI *BaseRenderer_OnReceiveFirstSample)(BaseRenderer *This, IMediaSample *pMediaSample);
typedef VOID (WINAPI *BaseRenderer_OnRenderEnd)(BaseRenderer *This, IMediaSample *pMediaSample);
typedef VOID (WINAPI *BaseRenderer_OnRenderStart)(BaseRenderer *This, IMediaSample *pMediaSample);
typedef VOID (WINAPI *BaseRenderer_OnStartStreaming)(BaseRenderer *This);
typedef VOID (WINAPI *BaseRenderer_OnStopStreaming)(BaseRenderer *This);
typedef VOID (WINAPI *BaseRenderer_OnWaitEnd)(BaseRenderer *This);
typedef VOID (WINAPI *BaseRenderer_OnWaitStart)(BaseRenderer *This);
typedef VOID (WINAPI *BaseRenderer_PrepareRender)(BaseRenderer *This);
typedef HRESULT (WINAPI *BaseRenderer_ShouldDrawSampleNow)(BaseRenderer *This, IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime);
typedef HRESULT (WINAPI *BaseRenderer_PrepareReceive)(BaseRenderer *This, IMediaSample *pMediaSample);
typedef HRESULT (WINAPI *BaseRenderer_EndOfStream)(BaseRenderer *This);
typedef HRESULT (WINAPI *BaseRenderer_BeginFlush) (BaseRenderer *This);
typedef HRESULT (WINAPI *BaseRenderer_EndFlush) (BaseRenderer *This);
typedef HRESULT (WINAPI *BaseRenderer_BreakConnect) (BaseRenderer *This);
typedef HRESULT (WINAPI *BaseRenderer_CompleteConnect) (BaseRenderer *This, IPin *pReceivePin);

typedef struct BaseRendererFuncTable {
	/* Required */
	BaseRenderer_CheckMediaType pfnCheckMediaType;
	BaseRenderer_DoRenderSample pfnDoRenderSample;
	/* Optional, Data Handlers */
	BaseRenderer_OnReceiveFirstSample  pfnOnReceiveFirstSample;
	BaseRenderer_OnRenderEnd  pfnOnRenderEnd;
	BaseRenderer_OnRenderStart  pfnOnRenderStart;
	BaseRenderer_OnStartStreaming  pfnOnStartStreaming;
	BaseRenderer_OnStopStreaming  pfnOnStopStreaming;
	BaseRenderer_OnWaitEnd  pfnOnWaitEnd;
	BaseRenderer_OnWaitStart  pfnOnWaitStart;
	BaseRenderer_PrepareRender  pfnPrepareRender;
	BaseRenderer_ShouldDrawSampleNow  pfnShouldDrawSampleNow;
	BaseRenderer_PrepareReceive pfnPrepareReceive;
	/* Optional, Input Pin */
	BaseRenderer_CompleteConnect pfnCompleteConnect;
	BaseRenderer_BreakConnect pfnBreakConnect;
	BaseRenderer_EndOfStream pfnEndOfStream;
	BaseRenderer_BeginFlush pfnBeginFlush;
	BaseRenderer_EndFlush pfnEndFlush;
} BaseRendererFuncTable;

HRESULT WINAPI BaseRendererImpl_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI BaseRendererImpl_Release(IBaseFilter * iface);
HRESULT WINAPI BaseRendererImpl_Receive(BaseRenderer *This, IMediaSample * pSample);
HRESULT WINAPI BaseRendererImpl_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin);
HRESULT WINAPI BaseRendererImpl_Stop(IBaseFilter * iface);
HRESULT WINAPI BaseRendererImpl_Run(IBaseFilter * iface, REFERENCE_TIME tStart);
HRESULT WINAPI BaseRendererImpl_Pause(IBaseFilter * iface);
HRESULT WINAPI BaseRendererImpl_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock);
HRESULT WINAPI BaseRendererImpl_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState);
HRESULT WINAPI BaseRendererImpl_EndOfStream(BaseRenderer* iface);
HRESULT WINAPI BaseRendererImpl_BeginFlush(BaseRenderer* iface);
HRESULT WINAPI BaseRendererImpl_EndFlush(BaseRenderer* iface);
HRESULT WINAPI BaseRendererImpl_ClearPendingSample(BaseRenderer *iface);

HRESULT WINAPI BaseRenderer_Init(BaseRenderer *This, const IBaseFilterVtbl *Vtbl, IUnknown *pUnkOuter, const CLSID *pClsid, DWORD_PTR DebugInfo, const BaseRendererFuncTable* pBaseFuncsTable);

#ifdef __IBasicAudio_FWD_DEFINED__
typedef struct tagBasicAudio
{
	IBasicAudio IBasicAudio_iface;
	BaseDispatch baseDispatch;
} BasicAudio;

HRESULT WINAPI BasicAudio_Init(BasicAudio *This, const IBasicAudioVtbl *Vtbl);
HRESULT WINAPI BasicAudio_Destroy(BasicAudio *pBasicAudio);

HRESULT WINAPI BasicAudioImpl_GetTypeInfoCount(IBasicAudio *iface, UINT*pctinfo);
HRESULT WINAPI BasicAudioImpl_GetTypeInfo(IBasicAudio *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo);
HRESULT WINAPI BasicAudioImpl_GetIDsOfNames(IBasicAudio *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
HRESULT WINAPI BasicAudioImpl_Invoke(IBasicAudio *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExepInfo, UINT *puArgErr);
#endif

/* Dll Functions */
BOOL WINAPI STRMBASE_DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv);
HRESULT WINAPI STRMBASE_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
HRESULT WINAPI STRMBASE_DllCanUnloadNow(void);
