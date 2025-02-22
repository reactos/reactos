/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/notify.c
 * PURPOSE:         IDirectSoundNotify implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "precomp.h"

typedef struct tagNOTIFYEVENT
{
    DWORD NotifyCount;
    PLOOPEDSTREAMING_POSITION_EVENT_DATA Notify;
    struct tagNOTIFYEVENT *lpNext;
}NOTIFYEVENT, *LPNOTIFYEVENT;

typedef struct
{
    IDirectSoundNotifyVtbl * lpVtbl;
    LONG ref;

    LPNOTIFYEVENT EventListHead;
    BOOL bLoop;
    BOOL bMix;
    HANDLE hPin;
    DWORD BufferSize;

}CDirectSoundNotifyImpl, *LPCDirectSoundNotifyImpl;

static
ULONG
WINAPI
IDirectSoundNotify_fnAddRef(
    LPDIRECTSOUNDNOTIFY iface)
{
    ULONG ref;
    LPCDirectSoundNotifyImpl This = (LPCDirectSoundNotifyImpl)CONTAINING_RECORD(iface, CDirectSoundNotifyImpl, lpVtbl);

    /* increment reference count */
    ref = InterlockedIncrement(&This->ref);

    return ref;
}

static
ULONG
WINAPI
IDirectSoundNotify_fnRelease(
    LPDIRECTSOUNDNOTIFY iface)
{
    ULONG ref;
    LPCDirectSoundNotifyImpl This = (LPCDirectSoundNotifyImpl)CONTAINING_RECORD(iface, CDirectSoundNotifyImpl, lpVtbl);

    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT
WINAPI
IDirectSoundNotify_fnQueryInterface(
    LPDIRECTSOUNDNOTIFY iface,
    IN REFIID riid,
    LPVOID* ppobj)
{
    LPCDirectSoundNotifyImpl This = (LPCDirectSoundNotifyImpl)CONTAINING_RECORD(iface, CDirectSoundNotifyImpl, lpVtbl);

    /* check if the interface is supported */
    if (IsEqualIID(riid, &IID_IDirectSoundNotify) || IsEqualIID(riid, &IID_IUnknown))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    return E_NOINTERFACE;
}

HRESULT
WINAPI
IDirectSoundNotify_fnSetNotificationPositions(
    LPDIRECTSOUNDNOTIFY iface,
    DWORD dwPositionNotifies,
    LPCDSBPOSITIONNOTIFY pcPositionNotifies)
{
    DWORD Index;
    LPNOTIFYEVENT Notify;
    DWORD Result;
    KSEVENT Request;

    LPCDirectSoundNotifyImpl This = (LPCDirectSoundNotifyImpl)CONTAINING_RECORD(iface, CDirectSoundNotifyImpl, lpVtbl);

    if (dwPositionNotifies > DSBNOTIFICATIONS_MAX)
    {
        /* invalid param */
        return DSERR_INVALIDPARAM;
    }

    /* verify notification event handles */
    for(Index = 0; Index < dwPositionNotifies; Index++)
    {
        ASSERT(pcPositionNotifies[Index].hEventNotify);
        ASSERT(pcPositionNotifies[Index].dwOffset < This->BufferSize || pcPositionNotifies[Index].dwOffset != DSBPN_OFFSETSTOP);

        if (pcPositionNotifies[Index].hEventNotify == NULL)
            return DSERR_INVALIDPARAM;

        if (pcPositionNotifies[Index].dwOffset > This->BufferSize && pcPositionNotifies[Index].dwOffset != DSBPN_OFFSETSTOP)
            return DSERR_INVALIDPARAM;
    }

    /* allocate new array */
    Notify = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NOTIFYEVENT));
    if (!Notify)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    /* allocate new array */
    Notify->Notify = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwPositionNotifies * sizeof(LOOPEDSTREAMING_POSITION_EVENT_DATA));
    if (!Notify->Notify)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, Notify);
        return DSERR_OUTOFMEMORY;
    }

    /* FIXME support non-looped streaming */
    ASSERT(This->bLoop);

    /* prepare request */
    Request.Set = KSEVENTSETID_LoopedStreaming;
    Request.Id = KSEVENT_LOOPEDSTREAMING_POSITION;
    Request.Flags = KSEVENT_TYPE_ENABLE;

    for(Index = 0; Index < dwPositionNotifies; Index++)
    {
        /* initialize event entries */
        Notify->Notify[Index].Position = pcPositionNotifies[Index].dwOffset;
        Notify->Notify[Index].KsEventData.EventHandle.Event = pcPositionNotifies[Index].hEventNotify;
        Notify->Notify[Index].KsEventData.NotificationType = KSEVENTF_EVENT_HANDLE;

        if (This->bMix == FALSE)
        {
            /* format is supported natively */
            Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_ENABLE_EVENT, (PVOID)&Request, sizeof(KSEVENT), (PVOID)&Notify->Notify[Index], sizeof(LOOPEDSTREAMING_POSITION_EVENT_DATA), NULL);

            if (Result != ERROR_SUCCESS)
            {
                DPRINT1("Failed to enable event %p Position %u\n", pcPositionNotifies[Index].hEventNotify, pcPositionNotifies[Index].dwOffset);
            }
        }
    }

    /* enlarge notify count */
    Notify->NotifyCount = dwPositionNotifies;

    if (This->EventListHead)
    {
        Notify->lpNext = This->EventListHead;
    }

    /* insert at front */
    (void)InterlockedExchangePointer((LPVOID*)&This->EventListHead, Notify);

    return DS_OK;
}

static IDirectSoundNotifyVtbl vt_DirectSoundNotify =
{
    /* IUnknown methods */
    IDirectSoundNotify_fnQueryInterface,
    IDirectSoundNotify_fnAddRef,
    IDirectSoundNotify_fnRelease,
    /* IDirectSoundNotify */
    IDirectSoundNotify_fnSetNotificationPositions
};


VOID
DoNotifyPositionEvents(
    LPDIRECTSOUNDNOTIFY iface,
    DWORD OldPosition,
    DWORD NewPosition)
{
    DWORD Index;
    LPNOTIFYEVENT CurEventList;

    LPCDirectSoundNotifyImpl This = (LPCDirectSoundNotifyImpl)CONTAINING_RECORD(iface, CDirectSoundNotifyImpl, lpVtbl);

    CurEventList = This->EventListHead;

    while(CurEventList)
    {
        for(Index = 0; Index < CurEventList->NotifyCount; Index++)
        {
            if (NewPosition > OldPosition)
            {
                /* buffer progress no overlap */
                if (OldPosition < CurEventList->Notify[Index].Position && CurEventList->Notify[Index].Position <= NewPosition)
                {
                    /* process event */
                    SetEvent(CurEventList->Notify[Index].KsEventData.EventHandle.Event);
                }
            }
            else
            {
                /* buffer wrap-arround */
                if (OldPosition < CurEventList->Notify[Index].Position || NewPosition > CurEventList->Notify[Index].Position)
                {
                    /* process event */
                    SetEvent(CurEventList->Notify[Index].KsEventData.EventHandle.Event);
                }
            }
        }

        /* iterate to next event list */
        CurEventList = CurEventList->lpNext;
    }
}

HRESULT
NewDirectSoundNotify(
    LPDIRECTSOUNDNOTIFY * Notify,
    BOOL bLoop,
    BOOL bMix,
    HANDLE hPin,
    DWORD BufferSize)
{
    LPCDirectSoundNotifyImpl This = HeapAlloc(GetProcessHeap(), 0, sizeof(CDirectSoundNotifyImpl));

    if (!This)
        return DSERR_OUTOFMEMORY;

    This->lpVtbl = &vt_DirectSoundNotify;
    This->bLoop = bLoop;
    This->bMix = bMix;
    This->hPin = hPin;
    This->ref = 1;
    This->EventListHead = NULL;
    This->BufferSize = BufferSize;

    *Notify = (LPDIRECTSOUNDNOTIFY)&This->lpVtbl;
    return DS_OK;

}
