/*
 * Implementation of IReferenceClock
 *
 * Copyright 2004 Raphael Junqueira
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

#include "quartz_private.h"

typedef struct SystemClockAdviseEntry SystemClockAdviseEntry;
struct SystemClockAdviseEntry {
  SystemClockAdviseEntry* next;
  SystemClockAdviseEntry* prev;

  HANDLE           hEvent;
  REFERENCE_TIME   rtBaseTime;
  REFERENCE_TIME   rtIntervalTime;
};

typedef struct SystemClockImpl {
  IReferenceClock IReferenceClock_iface;
  LONG ref;

  /** IReferenceClock */
  HANDLE         adviseThread;
  DWORD          adviseThreadId;
  BOOL           adviseThreadActive;
  REFERENCE_TIME lastRefTime;
  DWORD	         lastTimeTickCount;
  CRITICAL_SECTION safe;

  SystemClockAdviseEntry* pSingleShotAdvise;
  SystemClockAdviseEntry* pPeriodicAdvise;
} SystemClockImpl;

static inline SystemClockImpl *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, SystemClockImpl, IReferenceClock_iface);
}


static void QUARTZ_RemoveAviseEntryFromQueue(SystemClockImpl* This, SystemClockAdviseEntry* pEntry) {
  if (pEntry->prev) pEntry->prev->next = pEntry->next;
  if (pEntry->next) pEntry->next->prev = pEntry->prev;
  if (This->pSingleShotAdvise == pEntry) This->pSingleShotAdvise = pEntry->next;
  if (This->pPeriodicAdvise == pEntry)    This->pPeriodicAdvise = pEntry->next;
}

static void QUARTZ_InsertAviseEntryFromQueue(SystemClockImpl* This, SystemClockAdviseEntry* pEntry, SystemClockAdviseEntry** pQueue) {
  SystemClockAdviseEntry* prev_it = NULL;
  SystemClockAdviseEntry* it = NULL;
  REFERENCE_TIME bornTime =  pEntry->rtBaseTime + pEntry->rtIntervalTime;

  for (it = *pQueue; NULL != it && (it->rtBaseTime + it->rtIntervalTime) < bornTime; it = it->next) {
    prev_it = it;
  }
  if (NULL == prev_it) {
    pEntry->prev = NULL;
    if (NULL != (*pQueue)) pEntry->next = (*pQueue)->next;
    /*assert( NULL == pEntry->next->prev );*/
    if (NULL != pEntry->next) pEntry->next->prev = pEntry;
    (*pQueue) = pEntry;
  } else {
    pEntry->prev = prev_it;
    pEntry->next = prev_it->next;
    prev_it->next = pEntry;
    if (NULL != pEntry->next) pEntry->next->prev = pEntry;
  }
}

#define MAX_REFTIME            (REFERENCE_TIME)(0x7FFFFFFFFFFFFFFF)
#define ADVISE_EXIT            (WM_APP + 0)
#define ADVISE_REMOVE          (WM_APP + 2)
#define ADVISE_ADD_SINGLESHOT  (WM_APP + 4)
#define ADVISE_ADD_PERIODIC    (WM_APP + 8)

static DWORD WINAPI SystemClockAdviseThread(LPVOID lpParam) {
  SystemClockImpl* This = lpParam;
  DWORD timeOut = INFINITE;
  DWORD tmpTimeOut;
  MSG msg;
  HRESULT hr;
  REFERENCE_TIME curTime;
  SystemClockAdviseEntry* it = NULL;

  TRACE("(%p): Main Loop\n", This);

  while (TRUE) {
    if (timeOut > 0) MsgWaitForMultipleObjects(0, NULL, FALSE, timeOut, QS_POSTMESSAGE|QS_SENDMESSAGE|QS_TIMER);
    
    EnterCriticalSection(&This->safe);
    /*timeOut = IReferenceClock_OnTimerUpdated(This); */
    hr = IReferenceClock_GetTime(&This->IReferenceClock_iface, &curTime);
    if (FAILED(hr)) {
      timeOut = INFINITE;
      goto outrefresh;
    }

    /** First SingleShots Advice: sorted list */
    it = This->pSingleShotAdvise;
    while ((NULL != it) && (it->rtBaseTime + it->rtIntervalTime) <= curTime) {
      SystemClockAdviseEntry* nextit = it->next;
      /** send event ... */
      SetEvent(it->hEvent);
      /** ... and Release it */
      QUARTZ_RemoveAviseEntryFromQueue(This, it);
      CoTaskMemFree(it);
      it = nextit;
    }
    if (NULL != it) timeOut = (DWORD) ((it->rtBaseTime + it->rtIntervalTime) - curTime) / (REFERENCE_TIME)10000;
    else timeOut = INFINITE;

    /** Now Periodics Advice: semi sorted list (sort cannot be used) */
    for (it = This->pPeriodicAdvise; NULL != it; it = it->next) {
      if (it->rtBaseTime <= curTime) {
	DWORD nPeriods = (DWORD) ((curTime - it->rtBaseTime) / it->rtIntervalTime);
	/** Release the semaphore ... */
	ReleaseSemaphore(it->hEvent, nPeriods, NULL);
	/** ... and refresh time */
	it->rtBaseTime += nPeriods * it->rtIntervalTime;
	/*assert( it->rtBaseTime + it->rtIntervalTime < curTime );*/
      }
      tmpTimeOut = (DWORD) ((it->rtBaseTime + it->rtIntervalTime) - curTime) / (REFERENCE_TIME)10000;
      if (timeOut > tmpTimeOut) timeOut = tmpTimeOut; 
    }

outrefresh:
    LeaveCriticalSection(&This->safe);
    
    while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
      /** if hwnd we suppose that is a windows event ... */
      if  (NULL != msg.hwnd) {
	TranslateMessage(&msg);
	DispatchMessageW(&msg);
      } else {
	switch (msg.message) {	    
	case WM_QUIT:
	case ADVISE_EXIT:
	  goto outofthread;
	case ADVISE_ADD_SINGLESHOT:
	case ADVISE_ADD_PERIODIC:
	  /** set timeout to 0 to do a rescan now */
	  timeOut = 0;
	  break;
	case ADVISE_REMOVE:
	  /** hmmmm what we can do here ... */
	  timeOut = INFINITE;
	  break;
	default:
	  ERR("Unhandled message %u. Critical Path\n", msg.message);
	  break;
	}
      }
    }
  }

outofthread:
  TRACE("(%p): Exiting\n", This);
  return 0;
}
/*static DWORD WINAPI SystemClockAdviseThread(LPVOID lpParam) { */

static BOOL SystemClockPostMessageToAdviseThread(SystemClockImpl* This, UINT iMsg) {
  if (FALSE == This->adviseThreadActive) {
    BOOL res;
    This->adviseThread = CreateThread(NULL, 0, SystemClockAdviseThread, This, 0, &This->adviseThreadId);
    if (NULL == This->adviseThread) return FALSE;
    SetThreadPriority(This->adviseThread, THREAD_PRIORITY_TIME_CRITICAL);
    This->adviseThreadActive = TRUE;
    while(1) {
      res = PostThreadMessageW(This->adviseThreadId, iMsg, 0, 0);
      /* Let the thread creates its message queue (with MsgWaitForMultipleObjects call) by yielding and retrying */
      if (!res && (GetLastError() == ERROR_INVALID_THREAD_ID))
	Sleep(0);
      else
	break;
    }
    return res;
  }
  return PostThreadMessageW(This->adviseThreadId, iMsg, 0, 0);
}

static ULONG WINAPI SystemClockImpl_AddRef(IReferenceClock* iface) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  ULONG ref = InterlockedIncrement(&This->ref);

  TRACE("(%p): AddRef from %d\n", This, ref - 1);

  return ref;
}

static HRESULT WINAPI SystemClockImpl_QueryInterface(IReferenceClock* iface, REFIID riid, void** ppobj) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  TRACE("(%p, %s,%p)\n", This, debugstr_guid(riid), ppobj);
  
  if (IsEqualIID (riid, &IID_IUnknown) || 
      IsEqualIID (riid, &IID_IReferenceClock)) {
    SystemClockImpl_AddRef(iface);
    *ppobj = This;
    return S_OK;
  }
  
  *ppobj = NULL;
  WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppobj);
  return E_NOINTERFACE;
}

static ULONG WINAPI SystemClockImpl_Release(IReferenceClock* iface) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  ULONG ref = InterlockedDecrement(&This->ref);
  TRACE("(%p): ReleaseRef to %d\n", This, ref);
  if (ref == 0) {
    if (SystemClockPostMessageToAdviseThread(This, ADVISE_EXIT)) {
      WaitForSingleObject(This->adviseThread, INFINITE);
      CloseHandle(This->adviseThread);
    }
    This->safe.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->safe);
    CoTaskMemFree(This);
  }
  return ref;
}

static HRESULT WINAPI SystemClockImpl_GetTime(IReferenceClock* iface, REFERENCE_TIME* pTime) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  DWORD curTimeTickCount;
  HRESULT hr = S_OK;

  TRACE("(%p, %p)\n", This, pTime);

  if (NULL == pTime) {
    return E_POINTER;
  }

  curTimeTickCount = GetTickCount();

  EnterCriticalSection(&This->safe);
  if (This->lastTimeTickCount == curTimeTickCount) hr = S_FALSE;
  This->lastRefTime += (REFERENCE_TIME) (DWORD) (curTimeTickCount - This->lastTimeTickCount) * (REFERENCE_TIME) 10000;
  This->lastTimeTickCount = curTimeTickCount;
  *pTime = This->lastRefTime;
  LeaveCriticalSection(&This->safe);
  return hr;
}

static HRESULT WINAPI SystemClockImpl_AdviseTime(IReferenceClock* iface, REFERENCE_TIME rtBaseTime, REFERENCE_TIME rtStreamTime, HEVENT hEvent, DWORD_PTR* pdwAdviseCookie) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  SystemClockAdviseEntry* pEntry = NULL;

  TRACE("(%p, 0x%s, 0x%s, %ld, %p)\n", This, wine_dbgstr_longlong(rtBaseTime),
      wine_dbgstr_longlong(rtStreamTime), hEvent, pdwAdviseCookie);

  if (!hEvent) {
    return E_INVALIDARG;
  }
  if (0 >= rtBaseTime + rtStreamTime) {
    return E_INVALIDARG;
  }
  if (NULL == pdwAdviseCookie) {
    return E_POINTER;
  }
  pEntry = CoTaskMemAlloc(sizeof(SystemClockAdviseEntry));
  if (NULL == pEntry) {
    return E_OUTOFMEMORY;
  }
  ZeroMemory(pEntry, sizeof(SystemClockAdviseEntry));

  pEntry->hEvent = (HANDLE) hEvent;
  pEntry->rtBaseTime = rtBaseTime + rtStreamTime;
  pEntry->rtIntervalTime = 0;

  EnterCriticalSection(&This->safe);
  QUARTZ_InsertAviseEntryFromQueue(This, pEntry, &This->pSingleShotAdvise);
  LeaveCriticalSection(&This->safe);

  SystemClockPostMessageToAdviseThread(This, ADVISE_ADD_SINGLESHOT);

  *pdwAdviseCookie = (DWORD_PTR) (pEntry);
  return S_OK;
}

static HRESULT WINAPI SystemClockImpl_AdvisePeriodic(IReferenceClock* iface, REFERENCE_TIME rtStartTime, REFERENCE_TIME rtPeriodTime, HSEMAPHORE hSemaphore, DWORD_PTR* pdwAdviseCookie) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  SystemClockAdviseEntry* pEntry = NULL;

  TRACE("(%p, 0x%s, 0x%s, %ld, %p)\n", This, wine_dbgstr_longlong(rtStartTime),
      wine_dbgstr_longlong(rtPeriodTime), hSemaphore, pdwAdviseCookie);

  if (!hSemaphore) {
    return E_INVALIDARG;
  }
  if (0 >= rtStartTime || 0 >= rtPeriodTime) {
    return E_INVALIDARG;
  }
  if (NULL == pdwAdviseCookie) {
    return E_POINTER;
  }
  pEntry = CoTaskMemAlloc(sizeof(SystemClockAdviseEntry));
  if (NULL == pEntry) {
    return E_OUTOFMEMORY;
  }
  ZeroMemory(pEntry, sizeof(SystemClockAdviseEntry));

  pEntry->hEvent = (HANDLE) hSemaphore;
  pEntry->rtBaseTime = rtStartTime;
  pEntry->rtIntervalTime = rtPeriodTime;

  EnterCriticalSection(&This->safe);
  QUARTZ_InsertAviseEntryFromQueue(This, pEntry, &This->pPeriodicAdvise);
  LeaveCriticalSection(&This->safe);

  SystemClockPostMessageToAdviseThread(This, ADVISE_ADD_PERIODIC);

  *pdwAdviseCookie = (DWORD_PTR) (pEntry);
  return S_OK;
}

static HRESULT WINAPI SystemClockImpl_Unadvise(IReferenceClock* iface, DWORD_PTR dwAdviseCookie) {
  SystemClockImpl *This = impl_from_IReferenceClock(iface);
  SystemClockAdviseEntry* pEntry = NULL;
  SystemClockAdviseEntry* it = NULL;
  HRESULT ret = S_OK;
  TRACE("(%p, %lu)\n", This, dwAdviseCookie);

  pEntry = (SystemClockAdviseEntry*) dwAdviseCookie;

  EnterCriticalSection(&This->safe);
  for (it = This->pPeriodicAdvise; NULL != it && it != pEntry; it = it->next) ;
  if (it != pEntry) {
    for (it = This->pSingleShotAdvise; NULL != it && it != pEntry; it = it->next) ;
    if (it != pEntry) {
      ret = S_FALSE;
      goto out;
    }
  }

  QUARTZ_RemoveAviseEntryFromQueue(This, pEntry);
  CoTaskMemFree(pEntry);

  SystemClockPostMessageToAdviseThread(This, ADVISE_REMOVE);

out:
  LeaveCriticalSection(&This->safe);
  return ret;
}

static const IReferenceClockVtbl SystemClock_Vtbl = 
{
    SystemClockImpl_QueryInterface,
    SystemClockImpl_AddRef,
    SystemClockImpl_Release,
    SystemClockImpl_GetTime,
    SystemClockImpl_AdviseTime,
    SystemClockImpl_AdvisePeriodic,
    SystemClockImpl_Unadvise
};

HRESULT QUARTZ_CreateSystemClock(IUnknown * pUnkOuter, LPVOID * ppv) {
  SystemClockImpl* obj = NULL;
  
  TRACE("(%p,%p)\n", ppv, pUnkOuter);
  
  obj = CoTaskMemAlloc(sizeof(SystemClockImpl));
  if (NULL == obj) 	{
    *ppv = NULL;
    return E_OUTOFMEMORY;
  }
  ZeroMemory(obj, sizeof(SystemClockImpl));

  obj->IReferenceClock_iface.lpVtbl = &SystemClock_Vtbl;
  obj->ref = 0;  /* will be inited by QueryInterface */

  obj->lastTimeTickCount = GetTickCount();
  InitializeCriticalSection(&obj->safe);
  obj->safe.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": SystemClockImpl.safe");

  return SystemClockImpl_QueryInterface(&obj->IReferenceClock_iface, &IID_IReferenceClock, ppv);
}
