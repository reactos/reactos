/*
 * Generic Implementation of COutputQueue
 *
 * Copyright 2011 Aric Stewart, CodeWeavers
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

#include <wine/list.h>

enum {SAMPLE_PACKET, EOS_PACKET};

typedef struct tagQueuedEvent {
    int type;
    struct list entry;

    IMediaSample *pSample;
} QueuedEvent;

static DWORD WINAPI OutputQueue_InitialThreadProc(LPVOID data)
{
    OutputQueue *This = (OutputQueue *)data;
    return This->pFuncsTable->pfnThreadProc(This);
}

static void OutputQueue_FreeSamples(OutputQueue *pOutputQueue)
{
    struct list *cursor, *cursor2;
    LIST_FOR_EACH_SAFE(cursor, cursor2, pOutputQueue->SampleList)
    {
        QueuedEvent *qev = LIST_ENTRY(cursor, QueuedEvent, entry);
        list_remove(cursor);
        HeapFree(GetProcessHeap(),0,qev);
    }
}

HRESULT WINAPI OutputQueue_Construct(
    BaseOutputPin *pInputPin,
    BOOL bAuto,
    BOOL bQueue,
    LONG lBatchSize,
    BOOL bBatchExact,
    DWORD dwPriority,
    const OutputQueueFuncTable* pFuncsTable,
    OutputQueue **ppOutputQueue )

{
    BOOL threaded = FALSE;
    DWORD tid;

    OutputQueue *This;

    if (!pInputPin || !pFuncsTable || !ppOutputQueue)
        return E_INVALIDARG;

    *ppOutputQueue = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(OutputQueue));
    if (!*ppOutputQueue)
        return E_OUTOFMEMORY;

    This = *ppOutputQueue;
    This->pFuncsTable = pFuncsTable;
    This->lBatchSize = lBatchSize;
    This->bBatchExact = bBatchExact;
    InitializeCriticalSection(&This->csQueue);
    This->csQueue.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": OutputQueue.csQueue");
    This->SampleList = HeapAlloc(GetProcessHeap(),0,sizeof(struct list));
    if (!This->SampleList)
    {
        OutputQueue_Destroy(This);
        *ppOutputQueue = NULL;
        return E_OUTOFMEMORY;
    }
    list_init(This->SampleList);

    This->pInputPin = pInputPin;
    IPin_AddRef(&pInputPin->pin.IPin_iface);

    EnterCriticalSection(&This->csQueue);
    if (bAuto && pInputPin->pMemInputPin)
        threaded = IMemInputPin_ReceiveCanBlock(pInputPin->pMemInputPin) == S_OK;
    else
        threaded = bQueue;

    if (threaded)
    {
        This->hThread = CreateThread(NULL, 0, OutputQueue_InitialThreadProc, This, 0, &tid);
        if (This->hThread)
        {
            SetThreadPriority(This->hThread, dwPriority);
            This->hProcessQueue = CreateEventW(NULL, 0, 0, NULL);
        }
    }
    LeaveCriticalSection(&This->csQueue);

    return S_OK;
}

HRESULT WINAPI OutputQueue_Destroy(OutputQueue *pOutputQueue)
{
    EnterCriticalSection(&pOutputQueue->csQueue);
    OutputQueue_FreeSamples(pOutputQueue);
    pOutputQueue->bTerminate = TRUE;
    SetEvent(pOutputQueue->hProcessQueue);
    LeaveCriticalSection(&pOutputQueue->csQueue);

    pOutputQueue->csQueue.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&pOutputQueue->csQueue);
    CloseHandle(pOutputQueue->hProcessQueue);

    HeapFree(GetProcessHeap(),0,pOutputQueue->SampleList);

    IPin_Release(&pOutputQueue->pInputPin->pin.IPin_iface);
    HeapFree(GetProcessHeap(),0,pOutputQueue);
    return S_OK;
}

HRESULT WINAPI OutputQueue_ReceiveMultiple(OutputQueue *pOutputQueue, IMediaSample **ppSamples, LONG nSamples, LONG *nSamplesProcessed)
{
    HRESULT hr = S_OK;
    int i;

    if (!pOutputQueue->pInputPin->pin.pConnectedTo || !pOutputQueue->pInputPin->pMemInputPin)
        return VFW_E_NOT_CONNECTED;

    if (!pOutputQueue->hThread)
    {
        IMemInputPin_AddRef(pOutputQueue->pInputPin->pMemInputPin);
        hr = IMemInputPin_ReceiveMultiple(pOutputQueue->pInputPin->pMemInputPin,ppSamples, nSamples, nSamplesProcessed);
        IMemInputPin_Release(pOutputQueue->pInputPin->pMemInputPin);
    }
    else
    {
        EnterCriticalSection(&pOutputQueue->csQueue);
        *nSamplesProcessed = 0;

        for (i = 0; i < nSamples; i++)
        {
            QueuedEvent *qev = HeapAlloc(GetProcessHeap(),0,sizeof(QueuedEvent));
            if (!qev)
            {
                ERR("Out of Memory\n");
                hr = E_OUTOFMEMORY;
                break;
            }
            qev->type = SAMPLE_PACKET;
            qev->pSample = ppSamples[i];
            IMediaSample_AddRef(ppSamples[i]);
            list_add_tail(pOutputQueue->SampleList, &qev->entry);
            (*nSamplesProcessed)++;
        }

        if (!pOutputQueue->bBatchExact || list_count(pOutputQueue->SampleList) >= pOutputQueue->lBatchSize)
            SetEvent(pOutputQueue->hProcessQueue);
        LeaveCriticalSection(&pOutputQueue->csQueue);
    }
    return hr;
}

HRESULT WINAPI OutputQueue_Receive(OutputQueue *pOutputQueue, IMediaSample *pSample)
{
    LONG processed;
    return OutputQueue_ReceiveMultiple(pOutputQueue,&pSample,1,&processed);
}

VOID WINAPI OutputQueue_SendAnyway(OutputQueue *pOutputQueue)
{
    if (pOutputQueue->hThread)
    {
        EnterCriticalSection(&pOutputQueue->csQueue);
        if (!list_empty(pOutputQueue->SampleList))
        {
            pOutputQueue->bSendAnyway = TRUE;
            SetEvent(pOutputQueue->hProcessQueue);
        }
        LeaveCriticalSection(&pOutputQueue->csQueue);
    }
}

VOID WINAPI OutputQueue_EOS(OutputQueue *pOutputQueue)
{
    EnterCriticalSection(&pOutputQueue->csQueue);
    if (pOutputQueue->hThread)
    {
        QueuedEvent *qev = HeapAlloc(GetProcessHeap(),0,sizeof(QueuedEvent));
        if (!qev)
        {
            ERR("Out of Memory\n");
            LeaveCriticalSection(&pOutputQueue->csQueue);
            return;
        }
        qev->type = EOS_PACKET;
        qev->pSample = NULL;
        list_add_tail(pOutputQueue->SampleList, &qev->entry);
    }
    else
    {
        IPin* ppin = NULL;
        IPin_ConnectedTo(&pOutputQueue->pInputPin->pin.IPin_iface, &ppin);
        if (ppin)
        {
            IPin_EndOfStream(ppin);
            IPin_Release(ppin);
        }
    }
    LeaveCriticalSection(&pOutputQueue->csQueue);
    /* Covers sending the Event to the worker Thread */
    OutputQueue_SendAnyway(pOutputQueue);
}

DWORD WINAPI OutputQueueImpl_ThreadProc(OutputQueue *pOutputQueue)
{
    do
    {
        EnterCriticalSection(&pOutputQueue->csQueue);
        if (!list_empty(pOutputQueue->SampleList) &&
            (!pOutputQueue->bBatchExact ||
            list_count(pOutputQueue->SampleList) >= pOutputQueue->lBatchSize ||
            pOutputQueue->bSendAnyway
            )
           )
        {
            while (!list_empty(pOutputQueue->SampleList))
            {
                IMediaSample **ppSamples;
                LONG nSamples;
                LONG nSamplesProcessed;
                struct list *cursor, *cursor2;
                int i = 0;

                /* First Pass Process Samples */
                i = list_count(pOutputQueue->SampleList);
                ppSamples = HeapAlloc(GetProcessHeap(),0,sizeof(IMediaSample*) * i);
                nSamples = 0;
                LIST_FOR_EACH_SAFE(cursor, cursor2, pOutputQueue->SampleList)
                {
                    QueuedEvent *qev = LIST_ENTRY(cursor, QueuedEvent, entry);
                    if (qev->type == SAMPLE_PACKET)
                        ppSamples[nSamples++] = qev->pSample;
                    else
                        break;
                    list_remove(cursor);
                    HeapFree(GetProcessHeap(),0,qev);
                }

                if (pOutputQueue->pInputPin->pin.pConnectedTo && pOutputQueue->pInputPin->pMemInputPin)
                {
                    IMemInputPin_AddRef(pOutputQueue->pInputPin->pMemInputPin);
                    LeaveCriticalSection(&pOutputQueue->csQueue);
                    IMemInputPin_ReceiveMultiple(pOutputQueue->pInputPin->pMemInputPin, ppSamples, nSamples, &nSamplesProcessed);
                    EnterCriticalSection(&pOutputQueue->csQueue);
                    IMemInputPin_Release(pOutputQueue->pInputPin->pMemInputPin);
                }
                for (i = 0; i < nSamples; i++)
                    IMediaSample_Release(ppSamples[i]);
                HeapFree(GetProcessHeap(),0,ppSamples);

                /* Process Non-Samples */
                LIST_FOR_EACH_SAFE(cursor, cursor2, pOutputQueue->SampleList)
                {
                    QueuedEvent *qev = LIST_ENTRY(cursor, QueuedEvent, entry);
                    if (qev->type == EOS_PACKET)
                    {
                        IPin* ppin = NULL;
                        IPin_ConnectedTo(&pOutputQueue->pInputPin->pin.IPin_iface, &ppin);
                        if (ppin)
                        {
                            IPin_EndOfStream(ppin);
                            IPin_Release(ppin);
                        }
                    }
                    else if (qev->type == SAMPLE_PACKET)
                        break;
                    else
                        FIXME("Unhandled Event type %i\n",qev->type);
                    list_remove(cursor);
                    HeapFree(GetProcessHeap(),0,qev);
                }
            }
            pOutputQueue->bSendAnyway = FALSE;
        }
        LeaveCriticalSection(&pOutputQueue->csQueue);
        WaitForSingleObject(pOutputQueue->hProcessQueue, INFINITE);
    }
    while (!pOutputQueue->bTerminate);
    return S_OK;
}
