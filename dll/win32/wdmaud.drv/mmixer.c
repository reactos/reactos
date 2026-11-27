/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/mmixer.c
 *
 * PURPOSE:     WDM Audio Mixer API (User-mode part)
 * PROGRAMMERS: Johannes Anderwald
 */

#include "wdmaud.h"

#include <winreg.h>
#include <setupapi.h>
#include <mmixer.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>
#define NDEBUG
#include <debug.h>
#include <mmebuddy_debug.h>

BOOL MMixerLibraryInitialized = FALSE;

PVOID Alloc(ULONG NumBytes);
MIXER_STATUS Close(HANDLE hDevice);
VOID Free(PVOID Block);
VOID Copy(PVOID Src, PVOID Dst, ULONG NumBytes);
MIXER_STATUS Open(IN LPWSTR DevicePath, OUT PHANDLE hDevice);
MIXER_STATUS Control(IN HANDLE hMixer, IN ULONG dwIoControlCode, IN PVOID lpInBuffer, IN ULONG nInBufferSize, OUT PVOID lpOutBuffer, ULONG nOutBufferSize, PULONG lpBytesReturned);
MIXER_STATUS Enum(IN  PVOID EnumContext, IN  ULONG DeviceIndex, OUT LPWSTR * DeviceName, OUT PHANDLE OutHandle, OUT PHANDLE OutKey);
MIXER_STATUS OpenKey(IN HANDLE hKey, IN LPWSTR SubKey, IN ULONG DesiredAccess, OUT PHANDLE OutKey);
MIXER_STATUS CloseKey(IN HANDLE hKey);
MIXER_STATUS QueryKeyValue(IN HANDLE hKey, IN LPWSTR KeyName, OUT PVOID * ResultBuffer, OUT PULONG ResultLength, OUT PULONG KeyType);
PVOID AllocEventData(IN ULONG ExtraSize);
VOID FreeEventData(IN PVOID EventData);

MIXER_CONTEXT MixerContext =
{
    sizeof(MIXER_CONTEXT),
    NULL,
    Alloc,
    Control,
    Free,
    Open,
    Close,
    Copy,
    OpenKey,
    QueryKeyValue,
    CloseKey,
    AllocEventData,
    FreeEventData
};

GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

/*
 * WaveRT streaming
 *
 * The WaveRT buffer is serviced in two halves, a notification is signalled whenever
 * the hardware enters the next half. Playback data passes a ring of the same size:
 * the commit copies a header into the ring, the RT thread moves one half from the
 * ring into the buffer per notification. Recorded data takes the opposite way and
 * is copied into the queued capture requests. Requests are completed in order by
 * the completion thread, which runs the mmebuddy completion routine.
 */

/* the WaveRT notification count is limited to 1 or 2 */
#define RT_STREAMING_NOTIFICATIONS  2

typedef struct _RT_STREAMING_REQUEST
{
    LIST_ENTRY Entry;
    PSOUND_OVERLAPPED Overlap;
    LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    DWORD Status;
    DWORD BytesTransferred;
    PUCHAR Buffer;      /* capture only */
    DWORD Length;       /* capture only */
} RT_STREAMING_REQUEST, *PRT_STREAMING_REQUEST;

static
VOID
WdmAudCloseHandle(
    _Inout_ PHANDLE Handle)
{
    if (*Handle)
    {
        CloseHandle(*Handle);
        *Handle = NULL;
    }
}

static
VOID
WdmAudCloseRTStreamingEvents(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    WdmAudCloseHandle(&Instance->hNotifyRTStreamingEvent);
    WdmAudCloseHandle(&Instance->hNotifyRTStreamingStopEvent);
    WdmAudCloseHandle(&Instance->hNotifyRTStreamingCompletionEvent);
    WdmAudCloseHandle(&Instance->hNotifyRTStreamingCompletionStopEvent);
}

static
BOOL
RTStreamingCanceled(
    _In_ PSOUND_DEVICE_INSTANCE Instance)
{
    return Instance->ResetInProgress ||
           !Instance->RTStreamingEnabled ||
           Instance->bClosed;
}

static
DWORD
RTStreamingRingWrite(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _In_reads_bytes_(Length) const UCHAR *Source,
    _In_ DWORD Length)
{
    ULONG Available;
    ULONG BytesCopied;
    ULONG FirstPart;
    ULONG WriteOffset;
    LONG BytesUsed;

    BytesUsed = InterlockedCompareExchange(&Instance->RTStreamingShadowBufferBytesUsed, 0, 0);
    if (BytesUsed < 0 || (ULONG)BytesUsed > Instance->RTStreamingShadowBufferLength)
        return 0;

    Available = Instance->RTStreamingShadowBufferLength - BytesUsed;
    BytesCopied = min(Length, Available);
    if (!BytesCopied)
        return 0;

    WriteOffset = Instance->RTStreamingShadowBufferWriteOffset;
    FirstPart = min(BytesCopied, Instance->RTStreamingShadowBufferLength - WriteOffset);
    RtlCopyMemory(&Instance->RTStreamingShadowBuffer[WriteOffset], Source, FirstPart);
    if (BytesCopied != FirstPart)
    {
        RtlCopyMemory(Instance->RTStreamingShadowBuffer,
                      Source + FirstPart,
                      BytesCopied - FirstPart);
    }

    Instance->RTStreamingShadowBufferWriteOffset =
        (WriteOffset + BytesCopied) % Instance->RTStreamingShadowBufferLength;

    /* publishes the data to the reader */
    InterlockedExchangeAdd(&Instance->RTStreamingShadowBufferBytesUsed, BytesCopied);
    return BytesCopied;
}

static
DWORD
RTStreamingRingRead(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _Out_writes_bytes_(Length) UCHAR *Destination,
    _In_ DWORD Length)
{
    ULONG BytesCopied;
    ULONG FirstPart;
    ULONG ReadOffset;
    LONG BytesUsed;

    BytesUsed = InterlockedCompareExchange(&Instance->RTStreamingShadowBufferBytesUsed, 0, 0);
    if (BytesUsed <= 0 || (ULONG)BytesUsed > Instance->RTStreamingShadowBufferLength)
        return 0;

    BytesCopied = min(Length, (ULONG)BytesUsed);
    ReadOffset = Instance->RTStreamingShadowBufferReadOffset;
    FirstPart = min(BytesCopied, Instance->RTStreamingShadowBufferLength - ReadOffset);
    RtlCopyMemory(Destination, &Instance->RTStreamingShadowBuffer[ReadOffset], FirstPart);
    if (BytesCopied != FirstPart)
    {
        RtlCopyMemory(Destination + FirstPart,
                      Instance->RTStreamingShadowBuffer,
                      BytesCopied - FirstPart);
    }

    Instance->RTStreamingShadowBufferReadOffset =
        (ReadOffset + BytesCopied) % Instance->RTStreamingShadowBufferLength;

    /* releases the space to the writer */
    InterlockedExchangeAdd(&Instance->RTStreamingShadowBufferBytesUsed, -((LONG)BytesCopied));
    return BytesCopied;
}

static
VOID
RTStreamingQueueCompletion(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _In_ PRT_STREAMING_REQUEST Request)
{
    InterlockedIncrement(&Instance->RTStreamingCompletionsPending);

    EnterCriticalSection(&Instance->RTStreamingLock);
    InsertTailList(&Instance->RTStreamingCompletionList, &Request->Entry);
    LeaveCriticalSection(&Instance->RTStreamingLock);

    SetEvent(Instance->hNotifyRTStreamingCompletionEvent);
}

static
VOID
RTStreamingProcessCompletions(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    PLIST_ENTRY Entry;
    PRT_STREAMING_REQUEST Request;

    for (;;)
    {
        EnterCriticalSection(&Instance->RTStreamingLock);
        if (IsListEmpty(&Instance->RTStreamingCompletionList))
        {
            LeaveCriticalSection(&Instance->RTStreamingLock);
            break;
        }
        Entry = RemoveHeadList(&Instance->RTStreamingCompletionList);
        LeaveCriticalSection(&Instance->RTStreamingLock);

        Request = CONTAINING_RECORD(Entry, RT_STREAMING_REQUEST, Entry);
        Request->CompletionRoutine(Request->Status, Request->BytesTransferred, (LPOVERLAPPED)Request->Overlap);
        FreeMemory(Request);

        InterlockedDecrement(&Instance->RTStreamingCompletionsPending);
    }
}

static
VOID
RTStreamingWaitForCommits(
    _In_ PSOUND_DEVICE_INSTANCE Instance)
{
    /* the cancel flags are set by the caller, a commit checks them after it registered */
    MemoryBarrier();
    while (InterlockedCompareExchange(&Instance->RTStreamingCommitActive, 0, 0) != 0)
    {
        Sleep(1);
    }
}

/* copies recorded data into the capture requests, completes the full ones */
static
VOID
RTStreamingServiceCapture(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    PRT_STREAMING_REQUEST Request;
    BOOL Completed = FALSE;
    DWORD BytesCopied;

    EnterCriticalSection(&Instance->RTStreamingLock);

    while (!IsListEmpty(&Instance->RTStreamingCaptureList))
    {
        Request = CONTAINING_RECORD(Instance->RTStreamingCaptureList.Flink,
                                    RT_STREAMING_REQUEST,
                                    Entry);

        BytesCopied = RTStreamingRingRead(Instance,
                                          Request->Buffer + Request->BytesTransferred,
                                          Request->Length - Request->BytesTransferred);
        Request->BytesTransferred += BytesCopied;
        InterlockedExchangeAdd(&Instance->RTStreamingBytesPlayed, BytesCopied);

        if (Request->BytesTransferred < Request->Length)
            break;

        RemoveEntryList(&Request->Entry);
        InterlockedIncrement(&Instance->RTStreamingCompletionsPending);
        InsertTailList(&Instance->RTStreamingCompletionList, &Request->Entry);
        Completed = TRUE;
    }

    LeaveCriticalSection(&Instance->RTStreamingLock);

    if (Completed)
        SetEvent(Instance->hNotifyRTStreamingCompletionEvent);
}

/* completes the capture requests with the data they hold */
static
VOID
RTStreamingFlushCapture(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    PRT_STREAMING_REQUEST Request;
    BOOL Completed = FALSE;

    EnterCriticalSection(&Instance->RTStreamingLock);

    while (!IsListEmpty(&Instance->RTStreamingCaptureList))
    {
        Request = CONTAINING_RECORD(RemoveHeadList(&Instance->RTStreamingCaptureList),
                                    RT_STREAMING_REQUEST,
                                    Entry);
        Request->Status = ERROR_OPERATION_ABORTED;
        InterlockedIncrement(&Instance->RTStreamingCompletionsPending);
        InsertTailList(&Instance->RTStreamingCompletionList, &Request->Entry);
        Completed = TRUE;
    }

    LeaveCriticalSection(&Instance->RTStreamingLock);

    if (Completed)
        SetEvent(Instance->hNotifyRTStreamingCompletionEvent);
}

/* number of halves the hardware went through since the last notification */
static
ULONG
RTStreamingElapsedHalves(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    LARGE_INTEGER Now;
    LONGLONG Elapsed;
    LONGLONG Period = Instance->RTStreamingPeriodTicks;
    ULONG Halves = 1;

    QueryPerformanceCounter(&Now);
    Elapsed = Now.QuadPart - Instance->RTStreamingLastWake.QuadPart;
    Instance->RTStreamingLastWake = Now;

    /* the notification event is auto reset, late wakeups merge notifications */
    if (Period > 0 && Elapsed >= Period * 7 / 4)
    {
        Halves = (ULONG)min((Elapsed + Period / 2) / Period, 64);
    }

    return Halves;
}

static
VOID
RTStreamingRenderPeriod(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _In_ ULONG Halves)
{
    ULONG HalfLength = Instance->RTStreamingHalfLength;
    ULONG Offset = Instance->RTStreamingBufferOffset;
    ULONG Half;
    ULONG BytesCopied;
    ULONG Index;

    /* the halves which finished playing; a missed one has been played again with
     * its old content, the most recent one is refilled */
    for (Index = 0; Index < Halves; Index++)
    {
        if (Index)
        {
            Offset = (Offset + HalfLength) % (2 * HalfLength);
            InterlockedIncrement(&Instance->RTStreamingUnderrunCount);
            InterlockedExchangeAdd(&Instance->RTStreamingUnderrunBytes, HalfLength);
        }

        Half = Offset / HalfLength;
        InterlockedExchangeAdd(&Instance->RTStreamingBytesPlayed, Instance->RTStreamingHalfData[Half]);
        Instance->RTStreamingHalfData[Half] = 0;
    }

    BytesCopied = RTStreamingRingRead(Instance, &Instance->RTStreamingBuffer[Offset], HalfLength);
    if (BytesCopied < HalfLength)
    {
        RtlZeroMemory(&Instance->RTStreamingBuffer[Offset + BytesCopied], HalfLength - BytesCopied);
        if (BytesCopied)
        {
            InterlockedIncrement(&Instance->RTStreamingUnderrunCount);
            InterlockedExchangeAdd(&Instance->RTStreamingUnderrunBytes, HalfLength - BytesCopied);
        }
    }

    Instance->RTStreamingHalfData[Half] = BytesCopied;
    InterlockedExchangeAdd(&Instance->RTStreamingPeriodCount, Halves);
    if (BytesCopied)
    {
        InterlockedExchange(&Instance->RTStreamingLastDataPeriod, Instance->RTStreamingPeriodCount);
    }

    InterlockedExchange(&Instance->RTStreamingBufferOffset, (Offset + HalfLength) % (2 * HalfLength));
}

static
VOID
RTStreamingCapturePeriod(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _In_ ULONG Halves)
{
    ULONG HalfLength = Instance->RTStreamingHalfLength;
    ULONG Offset = Instance->RTStreamingBufferOffset;
    ULONG BytesCopied;

    if (Halves > 1)
    {
        /* the hardware has overwritten the older halves, take the most recent one */
        InterlockedExchangeAdd(&Instance->RTStreamingOverrunBytes, (Halves - 1) * HalfLength);
        Offset = (Offset + (Halves - 1) * HalfLength) % (2 * HalfLength);
    }

    BytesCopied = RTStreamingRingWrite(Instance, &Instance->RTStreamingBuffer[Offset], HalfLength);
    if (BytesCopied < HalfLength)
    {
        /* no capture request took the data in time */
        InterlockedExchangeAdd(&Instance->RTStreamingOverrunBytes, HalfLength - BytesCopied);
    }

    InterlockedExchangeAdd(&Instance->RTStreamingPeriodCount, Halves);
    InterlockedExchange(&Instance->RTStreamingBufferOffset, (Offset + HalfLength) % (2 * HalfLength));

    RTStreamingServiceCapture(Instance);
}

static
DWORD
WINAPI
RTStreamingThreadProc(
    LPVOID Parameter)
{
    PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)Parameter;
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    HANDLE WaitObjects[2];
    DWORD WaitStatus;
    MMRESULT Result;

    Result = GetSoundDeviceFromInstance(Instance, &SoundDevice);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT(Result == MMSYSERR_NOERROR);

    WaitObjects[0] = Instance->hNotifyRTStreamingStopEvent;
    WaitObjects[1] = Instance->hNotifyRTStreamingEvent;

    DPRINT("RTStreamingThreadProc entered %p\n", Instance);

    while (Instance->RTStreamingEnabled)
    {
        WaitStatus = WaitForMultipleObjects(2, WaitObjects, FALSE, INFINITE);
        if (WaitStatus != WAIT_OBJECT_0 + 1)
            break;

        if (DeviceType == WAVE_OUT_DEVICE_TYPE)
            RTStreamingRenderPeriod(Instance, RTStreamingElapsedHalves(Instance));
        else if (DeviceType == WAVE_IN_DEVICE_TYPE)
            RTStreamingCapturePeriod(Instance, RTStreamingElapsedHalves(Instance));
    }

    DPRINT("Exiting thread underruns %ld (%ld bytes) overruns %ld bytes\n",
           Instance->RTStreamingUnderrunCount,
           Instance->RTStreamingUnderrunBytes,
           Instance->RTStreamingOverrunBytes);
    return 0;
}

static
DWORD
WINAPI
RTStreamingCompletionThreadProc(
    IN  PVOID Parameter)
{
    PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)Parameter;
    HANDLE WaitObjects[2];
    DWORD WaitStatus;

    WaitObjects[0] = Instance->hNotifyRTStreamingCompletionStopEvent;
    WaitObjects[1] = Instance->hNotifyRTStreamingCompletionEvent;

    DPRINT("RTStreamingCompletionThreadProc entered %p\n", Instance);

    for (;;)
    {
        WaitStatus = WaitForMultipleObjects(2, WaitObjects, FALSE, INFINITE);

        /* the queued requests are completed before the thread stops */
        RTStreamingProcessCompletions(Instance);

        if (WaitStatus != WAIT_OBJECT_0 + 1)
            break;
    }

    DPRINT("Exiting completion thread\n");
    return 0;
}

static
VOID
WdmAudStopRTStreamingThread(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    if (Instance->hRTStreamingThread)
    {
        SetEvent(Instance->hNotifyRTStreamingStopEvent);
        WaitForSingleObject(Instance->hRTStreamingThread, INFINITE);
        WdmAudCloseHandle(&Instance->hRTStreamingThread);
    }
}

static
VOID
WdmAudStopRTCompletionThread(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    if (Instance->hRTStreamingCompletionThread)
    {
        SetEvent(Instance->hNotifyRTStreamingCompletionStopEvent);
        WaitForSingleObject(Instance->hRTStreamingCompletionThread, INFINITE);
        WdmAudCloseHandle(&Instance->hRTStreamingCompletionThread);
    }
}

static
BOOL
WdmAudStartRTStreamingThreads(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    if (!Instance->hRTStreamingCompletionThread)
    {
        ResetEvent(Instance->hNotifyRTStreamingCompletionStopEvent);
        Instance->hRTStreamingCompletionThread =
            CreateThread(NULL, 0, RTStreamingCompletionThreadProc, Instance, 0, NULL);
    }

    if (!Instance->hRTStreamingThread)
    {
        ResetEvent(Instance->hNotifyRTStreamingStopEvent);
        Instance->hRTStreamingThread =
            CreateThread(NULL, 0, RTStreamingThreadProc, Instance, 0, NULL);

        /* a half must be refilled before the hardware wraps around to it */
        if (Instance->hRTStreamingThread)
            SetThreadPriority(Instance->hRTStreamingThread, THREAD_PRIORITY_TIME_CRITICAL);
    }

    return Instance->hRTStreamingThread && Instance->hRTStreamingCompletionThread;
}

/* fills the whole buffer from the ring before the pin starts at position 0 */
static
VOID
RTStreamingPrime(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    ULONG HalfLength = Instance->RTStreamingHalfLength;
    ULONG BytesCopied;

    /* a notification of the previous run must not consume a half */
    ResetEvent(Instance->hNotifyRTStreamingEvent);

    BytesCopied = RTStreamingRingRead(Instance, Instance->RTStreamingBuffer, 2 * HalfLength);
    RtlZeroMemory(Instance->RTStreamingBuffer + BytesCopied,
                  Instance->RTStreamingBufferLength - BytesCopied);

    Instance->RTStreamingHalfData[0] = min(BytesCopied, HalfLength);
    Instance->RTStreamingHalfData[1] = BytesCopied - Instance->RTStreamingHalfData[0];
    InterlockedExchange(&Instance->RTStreamingBufferOffset, 0);
    InterlockedExchange(&Instance->RTStreamingPeriodCount, 0);
    InterlockedExchange(&Instance->RTStreamingLastDataPeriod, 0);
}

/* runs the pin, a stopped pin is taken through ACQUIRE and PAUSE first */
static
MMRESULT
RTStreamingRun(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    MIXER_STATUS Status = MM_STATUS_SUCCESS;

    if (!Instance->bStarted)
    {
        /* the pin starts at position 0, a notification of the previous run is stale */
        ResetEvent(Instance->hNotifyRTStreamingEvent);

        Status = MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_ACQUIRE);
        if (Status == MM_STATUS_SUCCESS)
            Status = MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_PAUSE);
    }

    if (Status == MM_STATUS_SUCCESS)
    {
        QueryPerformanceCounter(&Instance->RTStreamingLastWake);
        MemoryBarrier();
        Status = MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_RUN);
    }

    if (Status != MM_STATUS_SUCCESS)
        return MMSYSERR_ERROR;

    Instance->bStarted = TRUE;
    Instance->RTStreamingRunning = TRUE;
    return MMSYSERR_NOERROR;
}

/* starts or resumes playback, a new stream begins with the data of the ring */
static
MMRESULT
RTStreamingStartRender(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    MMRESULT Result = MMSYSERR_NOERROR;

    EnterCriticalSection(&Instance->RTStreamingLock);

    if (!Instance->RTStreamingRunning)
    {
        if (!Instance->bStarted)
            RTStreamingPrime(Instance);

        Result = RTStreamingRun(Instance);
    }

    LeaveCriticalSection(&Instance->RTStreamingLock);
    return Result;
}

/* the pin keeps its position in PAUSE, a STOP rewinds it */
static
VOID
RTStreamingStopPin(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_PAUSE);
    MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_ACQUIRE);
    MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_STOP);
    Instance->bStarted = FALSE;
    Instance->RTStreamingRunning = FALSE;
}

/* plays the data of the ring and of the buffer */
static
VOID
RTStreamingDrain(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    DWORD Timeout;
    DWORD Start;

    if (!Instance->RTStreamingRunning ||
        !Instance->hRTStreamingThread ||
        !Instance->RTStreamingBytesPerSec)
    {
        return;
    }

    /* the ring and the buffer, plus some slack */
    Timeout = (DWORD)((ULONGLONG)(Instance->RTStreamingShadowBufferLength +
                                  Instance->RTStreamingBufferLength) * 1000 /
                      Instance->RTStreamingBytesPerSec) + 500;
    Start = GetTickCount();

    while (GetTickCount() - Start < Timeout)
    {
        /* data written on a notification has been played two notifications later */
        if (InterlockedCompareExchange(&Instance->RTStreamingShadowBufferBytesUsed, 0, 0) == 0 &&
            (LONG)(Instance->RTStreamingPeriodCount - Instance->RTStreamingLastDataPeriod) >= 2)
        {
            break;
        }

        Sleep(10);
    }

    DPRINT("RTStreamingDrain waited %lu ms\n", GetTickCount() - Start);
}

/* returns the ring and the counters to the state of a new stream */
static
VOID
RTStreamingResetBuffers(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance)
{
    InterlockedExchange(&Instance->RTStreamingBufferOffset, 0);
    Instance->RTStreamingShadowBufferReadOffset = 0;
    Instance->RTStreamingShadowBufferWriteOffset = 0;
    Instance->RTStreamingHalfData[0] = 0;
    Instance->RTStreamingHalfData[1] = 0;
    InterlockedExchange(&Instance->RTStreamingShadowBufferBytesUsed, 0);
    InterlockedExchange(&Instance->RTStreamingBytesPlayed, 0);
    InterlockedExchange(&Instance->RTStreamingLastPosition, 0);
    InterlockedExchange(&Instance->RTStreamingPeriodCount, 0);
    InterlockedExchange(&Instance->RTStreamingLastDataPeriod, 0);
    InterlockedExchange(&Instance->RTStreamingUnderrunCount, 0);
    InterlockedExchange(&Instance->RTStreamingUnderrunBytes, 0);
    InterlockedExchange(&Instance->RTStreamingOverrunBytes, 0);

    if (Instance->RTStreamingBuffer)
        RtlZeroMemory(Instance->RTStreamingBuffer, Instance->RTStreamingBufferLength);
    if (Instance->RTStreamingShadowBuffer)
        RtlZeroMemory(Instance->RTStreamingShadowBuffer, Instance->RTStreamingShadowBufferLength);

    MemoryBarrier();
}

/* releases everything the WaveRT setup allocated */
static
VOID
RTStreamingCleanup(
    _Inout_ PSOUND_DEVICE_INSTANCE Instance,
    _In_ BOOL EventRegistered)
{
    if (EventRegistered)
    {
        MMixerUnregisterRTStreamingEvent(&MixerContext,
                                         Instance->Handle,
                                         Instance->hNotifyRTStreamingEvent);
    }

    WdmAudCloseRTStreamingEvents(Instance);

    HeapFree(GetProcessHeap(), 0, Instance->RTStreamingShadowBuffer);
    Instance->RTStreamingShadowBuffer = NULL;
    Instance->RTStreamingShadowBufferLength = 0;
    Instance->RTStreamingBuffer = NULL;
    Instance->RTStreamingBufferLength = 0;

    if (Instance->RTStreamingLockInitialized)
    {
        DeleteCriticalSection(&Instance->RTStreamingLock);
        Instance->RTStreamingLockInitialized = FALSE;
    }
}

MIXER_STATUS
QueryKeyValue(
    IN HANDLE hKey,
    IN LPWSTR KeyName,
    OUT PVOID * ResultBuffer,
    OUT PULONG ResultLength,
    OUT PULONG KeyType)
{
    if (RegQueryValueExW((HKEY)hKey, KeyName, NULL, KeyType, NULL, ResultLength) == ERROR_FILE_NOT_FOUND)
        return MM_STATUS_UNSUCCESSFUL;

    *ResultBuffer = HeapAlloc(GetProcessHeap(), 0, *ResultLength);
    if (*ResultBuffer == NULL)
        return MM_STATUS_NO_MEMORY;

    if (RegQueryValueExW((HKEY)hKey, KeyName, NULL, KeyType, *ResultBuffer, ResultLength) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *ResultBuffer);
        return MM_STATUS_UNSUCCESSFUL;
    }
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
OpenKey(
    IN HANDLE hKey,
    IN LPWSTR SubKey,
    IN ULONG DesiredAccess,
    OUT PHANDLE OutKey)
{
    if (RegOpenKeyExW((HKEY)hKey, SubKey, 0, DesiredAccess, (PHKEY)OutKey) == ERROR_SUCCESS)
        return MM_STATUS_SUCCESS;

    return MM_STATUS_UNSUCCESSFUL;
}

MIXER_STATUS
CloseKey(
    IN HANDLE hKey)
{
    RegCloseKey((HKEY)hKey);
    return MM_STATUS_SUCCESS;
}


PVOID Alloc(ULONG NumBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumBytes);
}

MIXER_STATUS
Close(HANDLE hDevice)
{
    if (CloseHandle(hDevice))
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

VOID
Free(PVOID Block)
{
    HeapFree(GetProcessHeap(), 0, Block);
}

VOID
Copy(PVOID Src, PVOID Dst, ULONG NumBytes)
{
    RtlMoveMemory(Src, Dst, NumBytes);
}

MIXER_STATUS
Open(
    IN LPWSTR DevicePath,
    OUT PHANDLE hDevice)
{
     DevicePath[1] = L'\\';
    *hDevice = CreateFileW(DevicePath,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           NULL);
    if (*hDevice == INVALID_HANDLE_VALUE)
    {
        return MM_STATUS_UNSUCCESSFUL;
    }

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
Control(
    IN HANDLE hMixer,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    ULONG nOutBufferSize,
    PULONG lpBytesReturned)
{
    OVERLAPPED Overlapped;
    BOOLEAN IoResult;
    DWORD Transferred = 0;

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return MM_STATUS_NO_MEMORY;

    /* Talk to the device */
    IoResult = DeviceIoControl(hMixer,
                               dwIoControlCode,
                               lpInBuffer,
                               nInBufferSize,
                               lpOutBuffer,
                               nOutBufferSize,
                               &Transferred,
                               &Overlapped);

    /* If failure occurs, make sure it's not just due to the overlapped I/O */
    if ( ! IoResult )
    {
        if ( GetLastError() != ERROR_IO_PENDING )
        {
            CloseHandle(Overlapped.hEvent);

            if (GetLastError() == ERROR_MORE_DATA || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if ( lpBytesReturned )
                    *lpBytesReturned = Transferred;
                return MM_STATUS_MORE_ENTRIES;
            }

            return MM_STATUS_UNSUCCESSFUL;
        }
    }

    /* Wait for the I/O to complete */
    WaitForSingleObjectEx(Overlapped.hEvent, INFINITE, TRUE);

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);

    if (!IoResult)
        return MM_STATUS_UNSUCCESSFUL;
    if ( lpBytesReturned )
        *lpBytesReturned = Transferred;
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
Enum(
    IN  PVOID EnumContext,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * DeviceName,
    OUT PHANDLE OutHandle,
    OUT PHANDLE OutKey)
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DetailData;
    BOOL Result;
    DWORD Length;
    MIXER_STATUS Status;

    //printf("Enum EnumContext %p DeviceIndex %lu OutHandle %p\n", EnumContext, DeviceIndex, OutHandle);

    InterfaceData.cbSize = sizeof(InterfaceData);
    InterfaceData.Reserved = 0;

    Result = SetupDiEnumDeviceInterfaces(EnumContext,
                                NULL,
                                &CategoryGuid,
                                DeviceIndex,
                                &InterfaceData);

    if (!Result)
    {
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            return MM_STATUS_NO_MORE_DEVICES;
        }
        return MM_STATUS_UNSUCCESSFUL;
    }

    Length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR);
    DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(),
                                                             0,
                                                             Length);
    DetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    DeviceData.cbSize = sizeof(DeviceData);
    DeviceData.Reserved = 0;

    Result = SetupDiGetDeviceInterfaceDetailW(EnumContext,
                                    &InterfaceData,
                                    DetailData,
                                    Length,
                                    NULL,
                                    &DeviceData);

    if (!Result)
    {
        DPRINT("SetupDiGetDeviceInterfaceDetailW failed with %lu\n", GetLastError());
        return MM_STATUS_UNSUCCESSFUL;
    }


    *OutKey = SetupDiOpenDeviceInterfaceRegKey(EnumContext, &InterfaceData, 0, KEY_READ);
     if ((HKEY)*OutKey == INVALID_HANDLE_VALUE)
     {
        HeapFree(GetProcessHeap(), 0, DetailData);
        return MM_STATUS_UNSUCCESSFUL;
    }

    Status = Open(DetailData->DevicePath, OutHandle);

    if (Status != MM_STATUS_SUCCESS)
    {
        RegCloseKey((HKEY)*OutKey);
        HeapFree(GetProcessHeap(), 0, DetailData);
        return Status;
    }

    *DeviceName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(DetailData->DevicePath)+1) * sizeof(WCHAR));
    if (*DeviceName == NULL)
    {
        CloseHandle(*OutHandle);
        RegCloseKey((HKEY)*OutKey);
        HeapFree(GetProcessHeap(), 0, DetailData);
        return MM_STATUS_NO_MEMORY;
    }
    wcscpy(*DeviceName, DetailData->DevicePath);
    HeapFree(GetProcessHeap(), 0, DetailData);

    return Status;
}

PVOID
AllocEventData(
    IN ULONG ExtraSize)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KSEVENTDATA) + ExtraSize);
    if (!Data)
        return NULL;

    Data->EventHandle.Event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!Data->EventHandle.Event)
    {
        HeapFree(GetProcessHeap(), 0, Data);
        return NULL;
    }

    Data->NotificationType = KSEVENTF_EVENT_HANDLE;
    return Data;
}

VOID
FreeEventData(IN PVOID EventData)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)EventData;

    CloseHandle(Data->EventHandle.Event);
    HeapFree(GetProcessHeap(), 0, Data);
}


BOOL
WdmAudInitUserModeMixer()
{
    HDEVINFO DeviceHandle;
    MIXER_STATUS Status;

    if (MMixerLibraryInitialized)
    {
        /* Library is already initialized */
        return TRUE;
    }


    /* Create a device list */
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        /* Failed to create a device list */
        return FALSE;
    }


    /* Initialize the mixer library */
    Status = MMixerInitialize(&MixerContext, Enum, (PVOID)DeviceHandle);

    /* Free device list */
    SetupDiDestroyDeviceInfoList(DeviceHandle);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* Failed to initialize mixer library */
        DPRINT1("Failed to initialize mixer library with %x\n", Status);
        return FALSE;
    }

    /* Library is now initialized */
    MMixerLibraryInitialized = TRUE;

    /* Completed successfully */
    return TRUE;
}

MMRESULT
WdmAudCleanupByMMixer()
{
    /* TODO */
    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetMixerCapabilities(
    IN ULONG DeviceId,
    LPMIXERCAPSW Capabilities)
{
    if (MMixerGetCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudGetLineInfo(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINEW MixLine,
    IN ULONG Flags)
{
    if (MMixerGetLineInfo(&MixerContext, hMixer, MixerId, Flags, MixLine)  == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetLineControls(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINECONTROLSW MixControls,
    IN ULONG Flags)
{
    if (MMixerGetLineControls(&MixerContext, hMixer, MixerId, Flags, MixControls) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudSetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerSetControlDetails(&MixerContext, hMixer, MixerId, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;

}

MMRESULT
WdmAudGetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerGetControlDetails(&MixerContext, hMixer, MixerId, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetWaveOutCapabilities(
    IN ULONG DeviceId,
    LPWAVEOUTCAPSW Capabilities)
{
    if (MMixerWaveOutCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;

}

MMRESULT
WdmAudGetWaveInCapabilities(
    IN ULONG DeviceId,
    LPWAVEINCAPSW Capabilities)
{
    if (MMixerWaveInCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudSetWaveDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;
    BOOL bWaveIn;
    BOOL EventRegistered = FALSE;

    DPRINT("SetWaveDeviceFormatByMMixer\n");
    Result = GetSoundDeviceFromInstance(Instance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    ASSERT( Result == MMSYSERR_NOERROR );

    bWaveIn = (DeviceType == WAVE_IN_DEVICE_TYPE ? TRUE : FALSE);

    if (MMixerOpenWave(&MixerContext, DeviceId, bWaveIn, WaveFormat, NULL, NULL, &Instance->Handle) == MM_STATUS_SUCCESS)
    {
        MIXER_STATUS MixerStatus = MMixerInitializeRTStreamingBuffer(
            &MixerContext,
            Instance->Handle,
            PAGE_SIZE * 8,
            RT_STREAMING_NOTIFICATIONS,
            &Instance->RTStreamingBuffer,
            &Instance->RTStreamingBufferLength);
        if (MixerStatus == MM_STATUS_SUCCESS)
        {
            LARGE_INTEGER Frequency;
            ULONG BlockAlign = max(WaveFormat->nBlockAlign, 1);

            DPRINT("RTStreamingBuffer %p Length %u\n",
                   Instance->RTStreamingBuffer,
                   Instance->RTStreamingBufferLength);

            /* the halves are serviced on the notifications, keep them block aligned */
            Instance->RTStreamingHalfLength = Instance->RTStreamingBufferLength / 2;
            Instance->RTStreamingHalfLength -= Instance->RTStreamingHalfLength % BlockAlign;
            Instance->RTStreamingBytesPerSec = WaveFormat->nAvgBytesPerSec;
            if (!Instance->RTStreamingHalfLength || !Instance->RTStreamingBytesPerSec)
            {
                DPRINT1("Invalid RT buffer length %u or format\n", Instance->RTStreamingBufferLength);
                goto FailedRTStreamingSetup;
            }

            /* duration of a half, to detect merged notifications */
            QueryPerformanceFrequency(&Frequency);
            Instance->RTStreamingPeriodTicks =
                (LONGLONG)Instance->RTStreamingHalfLength * Frequency.QuadPart /
                Instance->RTStreamingBytesPerSec;

            InitializeCriticalSection(&Instance->RTStreamingLock);
            Instance->RTStreamingLockInitialized = TRUE;
            InitializeListHead(&Instance->RTStreamingCompletionList);
            InitializeListHead(&Instance->RTStreamingCaptureList);
            Instance->RTStreamingCompletionsPending = 0;
            Instance->RTStreamingCommitActive = 0;

            Instance->hNotifyRTStreamingEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            Instance->hNotifyRTStreamingStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            Instance->hNotifyRTStreamingCompletionEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            Instance->hNotifyRTStreamingCompletionStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (Instance->hNotifyRTStreamingEvent == NULL ||
                Instance->hNotifyRTStreamingStopEvent == NULL ||
                Instance->hNotifyRTStreamingCompletionEvent == NULL ||
                Instance->hNotifyRTStreamingCompletionStopEvent == NULL)
            {
                DPRINT1("Failed to create event with %x", GetLastError());
                goto FailedRTStreamingSetup;
            }
            MixerStatus = MMixerRegisterRTStreamingEvent(&MixerContext, Instance->Handle, Instance->hNotifyRTStreamingEvent);
            if (MixerStatus != MM_STATUS_SUCCESS)
                goto FailedRTStreamingSetup;

            EventRegistered = TRUE;
            Instance->RTStreamingShadowBuffer = (PUCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Instance->RTStreamingBufferLength);
            if (Instance->RTStreamingShadowBuffer == NULL)
            {
                DPRINT1("Failed to create shadow buffer with %x", GetLastError());
                goto FailedRTStreamingSetup;
            }

            Instance->RTStreamingShadowBufferLength = Instance->RTStreamingBufferLength;
            RTStreamingResetBuffers(Instance);
            Instance->bStarted = FALSE;
            Instance->RTStreamingRunning = FALSE;
            Instance->LegacyStreaming = FALSE;
            Instance->RTStreamingEnabled = TRUE;
        }
        else
        {
            Instance->LegacyStreaming = TRUE;
        }

        if (DeviceType == WAVE_OUT_DEVICE_TYPE && !Instance->RTStreamingEnabled)
        {
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_RUN);
        }
        return MMSYSERR_NOERROR;

FailedRTStreamingSetup:
        RTStreamingCleanup(Instance, EventRegistered);
        return MMSYSERR_ERROR;
    }
    return MMSYSERR_ERROR;
}


MMRESULT
WdmAudGetCapabilitiesByMMixer(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMDEVICE_TYPE DeviceType;
    MMRESULT Result;

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (DeviceType == MIXER_DEVICE_TYPE)
    {
        return WdmAudGetMixerCapabilities(DeviceId, (LPMIXERCAPSW)Capabilities);
    }
    else if (DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        return WdmAudGetWaveOutCapabilities(DeviceId, (LPWAVEOUTCAPSW)Capabilities);
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        return WdmAudGetWaveInCapabilities(DeviceId, (LPWAVEINCAPSW)Capabilities);
    }
    else
    {
        /* Not supported */
        return MMSYSERR_ERROR;
    }
}

MMRESULT
WdmAudOpenSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE* SoundDevice,
    OUT PVOID* Handle)
{
    if (WdmAudInitUserModeMixer())
        return MMSYSERR_NOERROR;
    else
        return MMSYSERR_ERROR;
}

MMRESULT
WdmAudCloseSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    DPRINT("CloseSoundDeviceByMMixer\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (DeviceType == MIXER_DEVICE_TYPE)
    {
        /* No op */
        return MMSYSERR_NOERROR;
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        if (SoundDeviceInstance->RTStreamingEnabled)
        {
            DPRINT("closing device handling\n");

            /* a running commit stops, the threads keep running for the drain */
            SoundDeviceInstance->bClosed = TRUE;
            RTStreamingWaitForCommits(SoundDeviceInstance);

            /* the completed headers are still in the ring and the buffer */
            if (DeviceType == WAVE_OUT_DEVICE_TYPE)
                RTStreamingDrain(SoundDeviceInstance);

            WdmAudStopRTStreamingThread(SoundDeviceInstance);
            RTStreamingStopPin(SoundDeviceInstance);

            /* complete what is left, then stop the completion thread */
            RTStreamingFlushCapture(SoundDeviceInstance);
            WdmAudStopRTCompletionThread(SoundDeviceInstance);

            /* a completion may have started one more streaming pass, it ends at once */
            while (InterlockedCompareExchange(&SoundDeviceInstance->StreamingRequests, 0, 0) != 0)
                Sleep(1);

            SoundDeviceInstance->RTStreamingEnabled = FALSE;
            RTStreamingCleanup(SoundDeviceInstance, TRUE);
        }
        else
        {
            /* Make sure the pin is stopped */
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);
        }
        CloseHandle(Handle);
        return MMSYSERR_NOERROR;
    }

    /* Midi is not supported */
    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetNumWdmDevsByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount)
{
    switch(DeviceType)
    {
        case MIXER_DEVICE_TYPE:
            *DeviceCount = MMixerGetCount(&MixerContext);
            break;
        case WAVE_OUT_DEVICE_TYPE:
            *DeviceCount = MMixerGetWaveOutCount(&MixerContext);
            break;
        case WAVE_IN_DEVICE_TYPE:
            *DeviceCount = MMixerGetWaveInCount(&MixerContext);
            break;
        default:
            *DeviceCount = 0;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudQueryMixerInfoByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN DWORD MixerId,
    IN UINT uMsg,
    IN LPVOID Parameter,
    IN DWORD Flags)
{
    LPMIXERLINEW MixLine;
    LPMIXERLINECONTROLSW MixControls;
    LPMIXERCONTROLDETAILS MixDetails;
    HANDLE hMixer = NULL;

    MixLine = (LPMIXERLINEW)Parameter;
    MixControls = (LPMIXERLINECONTROLSW)Parameter;
    MixDetails = (LPMIXERCONTROLDETAILS)Parameter;

    /* FIXME param checks */

    if (SoundDeviceInstance)
    {
        hMixer = SoundDeviceInstance->Handle;
    }

    switch(uMsg)
    {
        case MXDM_GETLINEINFO:
            return WdmAudGetLineInfo(hMixer, MixerId, MixLine, Flags);
        case MXDM_GETLINECONTROLS:
            return WdmAudGetLineControls(hMixer, MixerId, MixControls, Flags);
        case MXDM_SETCONTROLDETAILS:
            return WdmAudSetControlDetails(hMixer, MixerId, MixDetails, Flags);
        case MXDM_GETCONTROLDETAILS:
            return WdmAudGetControlDetails(hMixer, MixerId, MixDetails, Flags);
        default:
            DPRINT1("MixerId %lu, uMsg %lu, Parameter %p, Flags %lu\n", MixerId, uMsg, Parameter, Flags);
            SND_ASSERT(0);
            return MMSYSERR_NOTSUPPORTED;
    }
}

MMRESULT
WdmAudGetDeviceInterfaceStringByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize)
{
    /* FIXME */
    return MMSYSERR_NOTSUPPORTED;
}

VOID
CALLBACK
MixerEventCallback(
    IN PVOID MixerEventContext,
    IN HANDLE hMixer,
    IN ULONG NotificationType,
    IN ULONG Value)
{
    PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)MixerEventContext;

    DriverCallback(Instance->WinMM.ClientCallback,
                   HIWORD(Instance->WinMM.Flags),
                   Instance->WinMM.Handle,
                   NotificationType,
                   Instance->WinMM.ClientCallbackInstanceData,
                   (DWORD_PTR)Value,
                   0);
}

MMRESULT
WdmAudSetMixerDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    if (MMixerOpen(&MixerContext, DeviceId, (PVOID)Instance, MixerEventCallback, &Instance->Handle) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudSetWaveStateByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN BOOL bStart)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MIXER_STATUS MixerStatus;
    MMRESULT Result;

    DPRINT("WdmAuSetWaveState bStart %x\n", bStart);

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ((DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE) &&
        SoundDeviceInstance->RTStreamingEnabled)
    {
        SoundDeviceInstance->bStarted = bStart;
        if (bStart)
        {
            if (!WdmAudStartRTStreamingThreads(SoundDeviceInstance))
                return MMSYSERR_NOMEM;

            if (DeviceType == WAVE_OUT_DEVICE_TYPE)
            {
                /* a paused stream continues; a new one starts once data is queued,
                 * by the commit when there is none yet */
                if (SoundDeviceInstance->bStarted ||
                    InterlockedCompareExchange(&SoundDeviceInstance->RTStreamingShadowBufferBytesUsed, 0, 0) > 0)
                {
                    return RTStreamingStartRender(SoundDeviceInstance);
                }
                return MMSYSERR_NOERROR;
            }

            EnterCriticalSection(&SoundDeviceInstance->RTStreamingLock);
            Result = SoundDeviceInstance->RTStreamingRunning ?
                     MMSYSERR_NOERROR : RTStreamingRun(SoundDeviceInstance);
            LeaveCriticalSection(&SoundDeviceInstance->RTStreamingLock);
            return Result;
        }

        /* no commit may restart the stream from now on */
        SoundDeviceInstance->bPaused = TRUE;
        MemoryBarrier();

        /* the pin keeps its position and the buffered data while paused */
        EnterCriticalSection(&SoundDeviceInstance->RTStreamingLock);
        if (SoundDeviceInstance->RTStreamingRunning)
        {
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            SoundDeviceInstance->RTStreamingRunning = FALSE;
        }
        LeaveCriticalSection(&SoundDeviceInstance->RTStreamingLock);
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        if (bStart)
        {
            MixerStatus = MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            if (MixerStatus == MM_STATUS_SUCCESS)
                MixerStatus = MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            if (MixerStatus == MM_STATUS_SUCCESS)
                MixerStatus = MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_RUN);
            if (MixerStatus != MM_STATUS_SUCCESS)
                return MMSYSERR_ERROR;
        }
        else
        {
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);
        }
    }
    else if (DeviceType == MIDI_IN_DEVICE_TYPE || DeviceType == MIDI_OUT_DEVICE_TYPE)
    {
        if (bStart)
        {
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_RUN);
        }
        else
        {
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);
        }
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudResetStreamByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMDEVICE_TYPE DeviceType,
    IN  BOOLEAN bStartReset)
{
    MIXER_STATUS Status;

    DPRINT("WdmaudResetStream bStartReset %x\n", bStartReset);
    if (SoundDeviceInstance->RTStreamingEnabled)
    {
        if (bStartReset)
        {
            BOOL AllCompleted;

            /* stop the commits before the ring is touched */
            SoundDeviceInstance->ResetInProgress = TRUE;
            RTStreamingWaitForCommits(SoundDeviceInstance);

            /* headers are completed once they are in the ring; when all were, the
             * client waited for them and resets before closing, so play them */
            EnterCriticalSection(&SoundDeviceInstance->QueueLock);
            AllCompleted = (SoundDeviceInstance->HeadWaveHeader == NULL);
            LeaveCriticalSection(&SoundDeviceInstance->QueueLock);

            if (DeviceType == WAVE_OUT_DEVICE_TYPE && AllCompleted)
                RTStreamingDrain(SoundDeviceInstance);

            /* the completion thread stays, it completes the outstanding requests */
            WdmAudStopRTStreamingThread(SoundDeviceInstance);
            RTStreamingStopPin(SoundDeviceInstance);
            RTStreamingFlushCapture(SoundDeviceInstance);
            RTStreamingResetBuffers(SoundDeviceInstance);
        }
        else
        {
            SoundDeviceInstance->ResetInProgress = FALSE;
        }
        return MMSYSERR_NOERROR;
    }

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Status = MMixerSetWaveResetState(&MixerContext, SoundDeviceInstance->Handle, bStartReset);
        DPRINT("WdmaudResetStream Result %x\n", Status);
        if (Status == MM_STATUS_SUCCESS)
        {
            /* Completed successfully */
            return MMSYSERR_NOERROR;
        }
    }
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WdmAudGetWavePositionByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMTIME* Time)
{
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    MIXER_STATUS Status;
    MMRESULT Result;
    DWORD Position;

    DPRINT("GetWavePosition\n");

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT(Result == MMSYSERR_NOERROR);

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        if (SoundDeviceInstance->RTStreamingEnabled)
        {
            LONG RTPosition = SoundDeviceInstance->RTStreamingBytesPlayed;
            LONG LastPosition;

            /* bytes played, plus the part of the half which is playing now */
            if (DeviceType == WAVE_OUT_DEVICE_TYPE &&
                SoundDeviceInstance->RTStreamingRunning &&
                SoundDeviceInstance->RTStreamingPeriodTicks > 0)
            {
                LARGE_INTEGER Now;
                LONGLONG Elapsed;
                ULONG Playing;

                QueryPerformanceCounter(&Now);
                Elapsed = Now.QuadPart - SoundDeviceInstance->RTStreamingLastWake.QuadPart;
                Playing = SoundDeviceInstance->RTStreamingHalfData[
                    (SoundDeviceInstance->RTStreamingBufferOffset / SoundDeviceInstance->RTStreamingHalfLength) & 1];
                if (Elapsed > 0)
                {
                    RTPosition += (LONG)min(Elapsed * SoundDeviceInstance->RTStreamingHalfLength /
                                            SoundDeviceInstance->RTStreamingPeriodTicks,
                                            (LONGLONG)Playing);
                }
            }

            /* the interpolation must not run ahead of the next update */
            LastPosition = SoundDeviceInstance->RTStreamingLastPosition;
            if (RTPosition - LastPosition < 0)
                RTPosition = LastPosition;
            else
                InterlockedExchange(&SoundDeviceInstance->RTStreamingLastPosition, RTPosition);

            /* Store position */
            Time->wType = TIME_BYTES;
            Time->u.cb = RTPosition;

            /* Completed successfully */
            return MMSYSERR_NOERROR;
        }

        Status = MMixerGetWavePosition(&MixerContext, SoundDeviceInstance->Handle, &Position);
        if (Status == MM_STATUS_SUCCESS)
        {
            /* Store position */
            Time->wType = TIME_BYTES;
            Time->u.cb = Position;

            /* Completed successfully */
            return MMSYSERR_NOERROR;
        }
    }
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WdmAudGetVolumeByMMixer(
    _In_ PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    _In_ DWORD DeviceId,
    _Out_ PDWORD pdwVolume)
{
    MMRESULT Result;
    MIXERLINE MixLine;
    MIXERCONTROL MixControl;
    MIXERLINECONTROLS MixLineControls;
    MIXERCONTROLDETAILS MixControlDetails;
    MIXERCONTROLDETAILS_UNSIGNED MixControlDetailsU[2]; // For 2 (stereo) channels

    MixLine.cbStruct = sizeof(MixLine);
    MixLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    /* Get line info */
    Result = WdmAudGetLineInfo(SoundDeviceInstance->Handle,
                               DeviceId,
                               &MixLine,
                               MIXER_OBJECTF_MIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    MixLineControls.cbStruct = sizeof(MixLineControls);
    MixLineControls.dwLineID = MixLine.dwLineID;
    MixLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    MixLineControls.cControls = 1;
    MixLineControls.cbmxctrl = sizeof(MixControl);
    MixLineControls.pamxctrl = &MixControl;

    /* Get line controls */
    Result = WdmAudGetLineControls(SoundDeviceInstance->Handle,
                                   DeviceId,
                                   &MixLineControls,
                                   MIXER_OBJECTF_MIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    MixControlDetails.cbStruct = sizeof(MixControlDetails);
    MixControlDetails.dwControlID = MixControl.dwControlID;
    MixControlDetails.cChannels = MixLine.cChannels;
    MixControlDetails.cMultipleItems = 0;
    MixControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    MixControlDetails.paDetails = MixControlDetailsU;

    /* Get volume control details */
    Result = WdmAudGetControlDetails(SoundDeviceInstance->Handle,
                                     DeviceId,
                                     &MixControlDetails,
                                     MIXER_OBJECTF_MIXER);
    if (MMSUCCESS(Result))
        *pdwVolume = MAKELONG(LOWORD(MixControlDetailsU[0].dwValue), HIWORD(MixControlDetailsU[1].dwValue));

    return Result;
}

MMRESULT
WdmAudSetVolumeByMMixer(
    _In_ PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    _In_ DWORD DeviceId,
    _In_ DWORD dwVolume)
{
    MMRESULT Result;
    MIXERLINE MixLine;
    MIXERCONTROL MixControl;
    MIXERLINECONTROLS MixLineControls;
    MIXERCONTROLDETAILS MixControlDetails;
    MIXERCONTROLDETAILS_UNSIGNED MixControlDetailsU[2]; // For 2 (stereo) channels

    MixLine.cbStruct = sizeof(MixLine);
    MixLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;

    /* Get line info */
    Result = WdmAudGetLineInfo(SoundDeviceInstance->Handle,
                               DeviceId,
                               &MixLine,
                               MIXER_OBJECTF_MIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    MixLineControls.cbStruct = sizeof(MixLineControls);
    MixLineControls.dwLineID = MixLine.dwLineID;
    MixLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    MixLineControls.cControls = 1;
    MixLineControls.cbmxctrl = sizeof(MixControl);
    MixLineControls.pamxctrl = &MixControl;

    /* Get line controls */
    Result = WdmAudGetLineControls(SoundDeviceInstance->Handle,
                                   DeviceId,
                                   &MixLineControls,
                                   MIXER_OBJECTF_MIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (!MMSUCCESS(Result))
        return TranslateInternalMmResult(Result);

    /* Convert volume level to be set */
    MixControlDetailsU[0].dwValue = LOWORD(dwVolume); // Left channel
    MixControlDetailsU[1].dwValue = HIWORD(dwVolume); // Right channel

    MixControlDetails.cbStruct = sizeof(MixControlDetails);
    MixControlDetails.dwControlID = MixControl.dwControlID;
    MixControlDetails.cChannels = MixLine.cChannels;
    MixControlDetails.cMultipleItems = 0;
    MixControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
    MixControlDetails.paDetails = MixControlDetailsU;

    /* Set volume control details */
    Result = WdmAudSetControlDetails(SoundDeviceInstance->Handle,
                                     DeviceId,
                                     &MixControlDetails,
                                     MIXER_OBJECTF_MIXER);
    return Result;
}

static
VOID WINAPI
CommitWaveBufferApc(PVOID ApcContext,
           PIO_STATUS_BLOCK IoStatusBlock,
           ULONG Reserved)
{
    DWORD dwErrorCode;
    PSOUND_OVERLAPPED Overlap;
    KSSTREAM_HEADER* lpHeader;

    dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
    Overlap = (PSOUND_OVERLAPPED)IoStatusBlock;
    lpHeader = Overlap->CompletionContext;

    /* Call mmebuddy overlap routine */
    Overlap->OriginalCompletionRoutine(dwErrorCode,
        lpHeader->DataUsed, &Overlap->Standard);

    HeapFree(GetProcessHeap(), 0, lpHeader);
}

DWORD
WINAPI
CommitLegacyRoutine(
    IN LPVOID Parameter)
{
    PSOUND_OVERLAPPED Overlap = (PSOUND_OVERLAPPED)Parameter;
    DPRINT("CommitLegacyRoutine Before wait\n");
    /* Wait for the I/O to complete */
    WaitForSingleObjectEx(Overlap->Standard.hEvent, INFINITE, TRUE);
    DPRINT("CommitLegacyRoutine after wait\n");
    CommitWaveBufferApc(NULL, (PIO_STATUS_BLOCK)Overlap, 0);
    ExitThread(0);
}

MMRESULT
WdmAudCommitWaveBufferByMMixer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    MMRESULT Result;
    ULONG IoCtl;
    KSSTREAM_HEADER* lpHeader;
    NTSTATUS Status;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }
    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (SoundDeviceInstance->RTStreamingEnabled)
    {
        PRT_STREAMING_REQUEST Request;
        DWORD Offset = 0;

        if (!WdmAudStartRTStreamingThreads(SoundDeviceInstance))
            return MMSYSERR_NOMEM;

        Request = AllocateMemory(sizeof(*Request));
        if (!Request)
            return MMSYSERR_NOMEM;

        RtlZeroMemory(Request, sizeof(*Request));
        Request->Overlap = Overlap;
        Request->CompletionRoutine = CompletionRoutine;
        Request->Status = ERROR_SUCCESS;

        /* a reset or close waits for this before it touches the ring;
         * the cancel flags are checked only after registering */
        InterlockedIncrement(&SoundDeviceInstance->RTStreamingCommitActive);

        if (DeviceType == WAVE_IN_DEVICE_TYPE)
        {
            /* filled by the RT thread, the caller must not wait for the data */
            Request->Buffer = (PUCHAR)OffsetPtr;
            Request->Length = Length;

            if (RTStreamingCanceled(SoundDeviceInstance))
            {
                Request->Status = ERROR_OPERATION_ABORTED;
                RTStreamingQueueCompletion(SoundDeviceInstance, Request);
            }
            else
            {
                EnterCriticalSection(&SoundDeviceInstance->RTStreamingLock);
                InsertTailList(&SoundDeviceInstance->RTStreamingCaptureList, &Request->Entry);
                LeaveCriticalSection(&SoundDeviceInstance->RTStreamingLock);

                RTStreamingServiceCapture(SoundDeviceInstance);
            }

            InterlockedDecrement(&SoundDeviceInstance->RTStreamingCommitActive);
            return MMSYSERR_NOERROR;
        }

        while (Offset < Length)
        {
            if (RTStreamingCanceled(SoundDeviceInstance))
            {
                Request->Status = ERROR_OPERATION_ABORTED;
                break;
            }

            Offset += RTStreamingRingWrite(SoundDeviceInstance,
                                           &((PUCHAR)OffsetPtr)[Offset],
                                           Length - Offset);

            /* start when the ring is full or nothing else is queued, so that the
             * buffer is primed with as much data as possible */
            if (!SoundDeviceInstance->RTStreamingRunning &&
                !SoundDeviceInstance->bPaused &&
                (Offset < Length || Overlap->Header == SoundDeviceInstance->TailWaveHeader))
            {
                if (RTStreamingStartRender(SoundDeviceInstance) != MMSYSERR_NOERROR)
                {
                    Request->Status = ERROR_GEN_FAILURE;
                    break;
                }
            }

            if (Offset < Length)
            {
                /* the RT thread frees a half per notification */
                Sleep(1);
            }
        }

        /* completed once in the ring, the client can queue the next buffer in time */
        Request->BytesTransferred = Offset;
        RTStreamingQueueCompletion(SoundDeviceInstance, Request);

        InterlockedDecrement(&SoundDeviceInstance->RTStreamingCommitActive);
        return MMSYSERR_NOERROR;
    }
    else if (SoundDeviceInstance->LegacyStreaming)
    {
        lpHeader = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KSSTREAM_HEADER));
        if (!lpHeader)
        {
            /* No memory */
            return MMSYSERR_NOMEM;
        }

        /* Setup stream packet */
        lpHeader->Size = sizeof(KSSTREAM_HEADER);
        lpHeader->PresentationTime.Numerator = 1;
        lpHeader->PresentationTime.Denominator = 1;
        lpHeader->Data = OffsetPtr;
        lpHeader->FrameExtent = Length;
        Overlap->CompletionContext = lpHeader;
        Overlap->OriginalCompletionRoutine = CompletionRoutine;
        IoCtl = (DeviceType == WAVE_OUT_DEVICE_TYPE ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM);

        if (DeviceType == WAVE_OUT_DEVICE_TYPE)
        {
            lpHeader->DataUsed = Length;
        }
        Overlap->Standard.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        Status = NtDeviceIoControlFile(
            SoundDeviceInstance->Handle, Overlap->Standard.hEvent, NULL, NULL, (PIO_STATUS_BLOCK)Overlap, IoCtl, NULL, 0,
            lpHeader, sizeof(KSSTREAM_HEADER));

        if (!NT_SUCCESS(Status) && GetLastError() != ERROR_IO_PENDING )
        {
            DPRINT("Failed %x\n", GetLastError());
            CloseHandle(Overlap->Standard.hEvent);
            return MMSYSERR_ERROR;
        }

        HANDLE hThread = CreateThread(NULL, 0, CommitLegacyRoutine, (LPVOID)Overlap, 0, NULL);
        CloseHandle(hThread);
        return MMSYSERR_NOERROR;
    }
    else
    {
        ASSERT(FALSE);
    }
    return MMSYSERR_NOTSUPPORTED;
}
