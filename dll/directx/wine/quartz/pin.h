/*
 * IPin function declarations to allow inheritance
 *
 * Copyright 2003 Robert Shearman
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

#pragma once

/* This function will process incoming samples to the pin.
 * Any return value valid in IMemInputPin::Receive is allowed here
 *
 * Cookie is the cookie that was set when requesting the buffer, if you don't
 * implement custom requesting, you can safely ignore this
 */
typedef HRESULT (* SAMPLEPROC_PULL)(LPVOID userdata, IMediaSample * pSample, DWORD_PTR cookie);

/* This function will determine whether a type is supported or not.
 * It is allowed to return any error value (within reason), as opposed
 * to IPin::QueryAccept which is only allowed to return S_OK or S_FALSE.
 */
typedef HRESULT (* QUERYACCEPTPROC)(LPVOID userdata, const AM_MEDIA_TYPE * pmt);

/* This function is called prior to finalizing a connection with
 * another pin and can be used to get things from the other pin
 * like IMemInput interfaces.
 *
 * props contains some defaults, but you can safely override them to your liking
 */
typedef HRESULT (* PRECONNECTPROC)(IPin * iface, IPin * pConnectPin, ALLOCATOR_PROPERTIES *props);

/* This function is called whenever a cleanup operation has to occur,
 * this is usually after a flush, seek, or end of stream notification.
 * This code may even be repeated multiple times, so build your code to
 * tolerate this behavior. Return value is ignored and should be S_OK.
 */
typedef HRESULT (* CLEANUPPROC) (LPVOID userdata);

/* This function is called whenever a request for a new sample is made,
 * If you implement it (it can be NULL for default behavior), you have to
 * call IMemAllocator_GetBuffer and IMemAllocator_RequestBuffer
 * This is useful if you want to request more than 1 buffer at simultaneously
 *
 * This will also cause the Sample Proc to be called with empty buffers to indicate
 * failure in retrieving the sample.
 */
typedef HRESULT (* REQUESTPROC) (LPVOID userdata);

/* This function is called after processing is done (for whatever reason that is caused)
 * This is useful if you create processing threads that need to die
 */
typedef HRESULT (* STOPPROCESSPROC) (LPVOID userdata);

#define ALIGNDOWN(value,boundary) ((value)/(boundary)*(boundary))
#define ALIGNUP(value,boundary) (ALIGNDOWN((value)+(boundary)-1, (boundary)))

typedef struct PullPin
{
	/* inheritance C style! */
	BasePin pin;
	LPVOID pUserData;

	REFERENCE_TIME rtStart, rtCurrent, rtNext, rtStop;
	IAsyncReader * pReader;
	IMemAllocator * prefAlloc;
	IMemAllocator * pAlloc;
	QUERYACCEPTPROC fnQueryAccept;
	SAMPLEPROC_PULL fnSampleProc;
	PRECONNECTPROC fnPreConnect;
	REQUESTPROC fnCustomRequest;
	CLEANUPPROC fnCleanProc;
	STOPPROCESSPROC fnDone;
	double dRate;
	BOOL stop_playback;
	DWORD cbAlign;

	/* Any code that touches the thread must hold the thread lock,
	 * lock order: thread_lock and then the filter critical section
	 * also signal thread_sleepy so the thread knows to wake up
	 */
	CRITICAL_SECTION thread_lock;
	HANDLE hThread;
	DWORD requested_state;
	HANDLE hEventStateChanged, thread_sleepy;
	DWORD state;
} PullPin;

#define Req_Sleepy 0
#define Req_Die    1
#define Req_Run    2
#define Req_Pause  3

/*** Constructors ***/
HRESULT PullPin_Construct(const IPinVtbl *PullPin_Vtbl, const PIN_INFO * pPinInfo,
                          SAMPLEPROC_PULL pSampleProc, LPVOID pUserData, QUERYACCEPTPROC pQueryAccept,
                          CLEANUPPROC pCleanUp, REQUESTPROC pCustomRequest, STOPPROCESSPROC pDone,
                          LPCRITICAL_SECTION pCritSec, IPin ** ppPin);

/**************************/
/*** Pin Implementation ***/

/* Pull Pin */
HRESULT WINAPI PullPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI PullPin_Disconnect(IPin * iface);
HRESULT WINAPI PullPin_QueryInterface(IPin * iface, REFIID riid, LPVOID * ppv);
ULONG   WINAPI PullPin_Release(IPin * iface);
HRESULT WINAPI PullPin_EndOfStream(IPin * iface);
HRESULT WINAPI PullPin_QueryAccept(IPin * iface, const AM_MEDIA_TYPE * pmt);
HRESULT WINAPI PullPin_BeginFlush(IPin * iface);
HRESULT WINAPI PullPin_EndFlush(IPin * iface);
HRESULT WINAPI PullPin_NewSegment(IPin * iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);

/* Thread interaction functions: Hold the thread_lock before calling them */
HRESULT PullPin_StartProcessing(PullPin * This);
HRESULT PullPin_PauseProcessing(PullPin * This);
HRESULT PullPin_WaitForStateChange(PullPin * This, DWORD dwMilliseconds);

/* COM helpers */
static inline PullPin *impl_PullPin_from_IPin( IPin *iface )
{
    return CONTAINING_RECORD(iface, PullPin, pin.IPin_iface);
}
