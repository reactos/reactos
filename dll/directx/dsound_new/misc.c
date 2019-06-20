/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/misc.c
 * PURPOSE:         Misc support routines
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Topology                 = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Audio = {0x45FFAAA0L, 0x6E1B, 0x11D0, {0xBC, 0xF2, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00}};


VOID
PerformChannelConversion(
    PUCHAR Buffer,
    ULONG BufferLength,
    PULONG BytesRead,
    ULONG OldChannels,
    ULONG NewChannels,
    ULONG BitsPerSample,
    PUCHAR Result,
    ULONG ResultLength,
    PULONG BytesWritten)
{
    DWORD NewIndex, OldIndex;
    DWORD NewLength, Skip;

    if (NewChannels > OldChannels)
    {
        UNIMPLEMENTED;
        ASSERT(0);
    }

    /* setup index */
    NewIndex = 0;
    OldIndex = 0;

    /* calculate offsets */
    NewLength = NewChannels * (BitsPerSample/8);
    Skip = OldChannels * (BitsPerSample/8);

    do
    {
        if (NewIndex + NewLength>= ResultLength)
        {
            NewIndex = ResultLength;
            break;
        }

        if (OldIndex + Skip >= BufferLength)
        {
            OldIndex = BufferLength;
            break;
        }

        /* copy first channel */
        RtlMoveMemory(&Result[NewIndex], &Buffer[OldIndex], NewLength);

        /* skip other channels */
        OldIndex += Skip;

        /* increment offset */
        NewIndex += NewLength;

    }while(TRUE);

    *BytesRead = OldIndex;
    *BytesWritten = NewIndex;
}


BOOL
SetPinFormat(
    IN HANDLE hPin,
    IN LPWAVEFORMATEX WaveFormatEx)
{
    DWORD dwResult;
    KSPROPERTY Property;
    KSDATAFORMAT_WAVEFORMATEX DataFormat;

    /* setup connection request */
    Property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;
    Property.Set = KSPROPSETID_Connection;
    Property.Flags = KSPROPERTY_TYPE_SET;

    /* setup data format */
    DataFormat.WaveFormatEx.wFormatTag = WaveFormatEx->wFormatTag;
    DataFormat.WaveFormatEx.nSamplesPerSec = WaveFormatEx->nSamplesPerSec;
    DataFormat.WaveFormatEx.nBlockAlign = WaveFormatEx->nBlockAlign;
    DataFormat.WaveFormatEx.cbSize = 0;
    DataFormat.DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat.DataFormat.Flags = 0;
    DataFormat.DataFormat.Reserved = 0;
    DataFormat.DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    DataFormat.DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat.DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat.DataFormat.SampleSize = 4;
    DataFormat.WaveFormatEx.nChannels = WaveFormatEx->nChannels;
    DataFormat.WaveFormatEx.nAvgBytesPerSec = WaveFormatEx->nAvgBytesPerSec;
    DataFormat.WaveFormatEx.wBitsPerSample = WaveFormatEx->wBitsPerSample;

    dwResult = SyncOverlappedDeviceIoControl(hPin, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSPROPERTY),(LPVOID)&DataFormat, sizeof(KSDATAFORMAT_WAVEFORMATEX), NULL);

    if (dwResult == ERROR_SUCCESS)
        return TRUE;
    else
        return FALSE;
}


BOOL
DoDataIntersection(
    HANDLE hFilter,
    DWORD PinId,
    DWORD SampleFrequency,
    LPWAVEFORMATEX WaveFormatEx,
    DWORD MinimumBitsPerSample,
    DWORD MaximumBitsPerSample,
    DWORD MaximumChannels,
    LPWAVEFORMATEX WaveFormatOut)
{
    DWORD nChannels, nBitsPerSample;
    KSDATAFORMAT_WAVEFORMATEX WaveFormat;
    PKSP_PIN Pin;
    PKSMULTIPLE_ITEM Item;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;
    DWORD dwResult;

    /* allocate request */
    Pin = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATAFORMAT_WAVEFORMATEX));
    if (!Pin)
    {
        /* no memory */
        return FALSE;
    }

    Item = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)(Item + 1);

    /* setup request */
    Pin->PinId = PinId;
    Pin->Property.Flags = KSPROPERTY_TYPE_GET;
    Pin->Property.Set = KSPROPSETID_Pin;
    Pin->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;
    Item->Count = 1;
    Item->Size = sizeof(KSDATAFORMAT_WAVEFORMATEX);


    DataFormat->WaveFormatEx.wFormatTag = WaveFormatEx->wFormatTag;
    DataFormat->WaveFormatEx.nSamplesPerSec = SampleFrequency;
    DataFormat->WaveFormatEx.nBlockAlign = WaveFormatEx->nBlockAlign;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;

    for(nChannels = 1; nChannels <= 2; nChannels++)
    {
        for(nBitsPerSample = MinimumBitsPerSample; nBitsPerSample <= MaximumBitsPerSample; nBitsPerSample += 8)
        {
            DataFormat->WaveFormatEx.nChannels = nChannels;
            DataFormat->WaveFormatEx.nAvgBytesPerSec = (nBitsPerSample / 8) * nChannels * SampleFrequency;
            DataFormat->WaveFormatEx.wBitsPerSample = nBitsPerSample;

            DPRINT("CurrentFormat: InFormat nChannels %u wBitsPerSample %u nSamplesPerSec %u\n",
                   nChannels, nBitsPerSample, SampleFrequency);

            dwResult = SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)Pin, sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + sizeof(KSDATAFORMAT_WAVEFORMATEX),
                                                     (LPVOID)&WaveFormat, sizeof(KSDATAFORMAT_WAVEFORMATEX), NULL);

            DPRINT("dwResult %x\n", dwResult);


            if (dwResult == ERROR_SUCCESS)
            {
                /* found a compatible audio range */
                WaveFormatOut->cbSize = 0;
                WaveFormatOut->nBlockAlign = WaveFormatEx->nBlockAlign;
                WaveFormatOut->wFormatTag = WaveFormatEx->wFormatTag;
                WaveFormatOut->nAvgBytesPerSec = (nBitsPerSample / 8) * nChannels * SampleFrequency;
                WaveFormatOut->wBitsPerSample = nBitsPerSample;
                WaveFormatOut->nSamplesPerSec = SampleFrequency;
                WaveFormatOut->nChannels = nChannels;

                /* free buffer */
                HeapFree(GetProcessHeap(), 0, Pin);

                DPRINT("InFormat  nChannels %u wBitsPerSample %u nSamplesPerSec %u\nOutFormat nChannels %u nBitsPerSample %u nSamplesPerSec %u\n",
                       WaveFormatEx->nChannels, WaveFormatEx->wBitsPerSample, WaveFormatEx->nSamplesPerSec,
                       WaveFormatOut->nChannels, WaveFormatOut->wBitsPerSample, WaveFormatOut->nSamplesPerSec);

                return TRUE;
            }
        }
    }

    /* free buffer */
    HeapFree(GetProcessHeap(), 0, Pin);
    ASSERT(0);
    return FALSE;
}

DWORD
OpenPin(
    HANDLE hFilter,
    ULONG PinId,
    LPWAVEFORMATEX WaveFormatEx,
    PHANDLE hPin,
    BOOL bLoop)
{
    DWORD Size, Result;
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;

    /* calculate request size */
    Size = sizeof(KSPIN_CONNECT) + sizeof(KSDATAFORMAT_WAVEFORMATEX);

    PinConnect = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
    if (!PinConnect)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }
    /* build pin request */
    PinConnect->Interface.Set = KSINTERFACESETID_Standard;

    if (bLoop)
        PinConnect->Interface.Id = KSINTERFACE_STANDARD_LOOPED_STREAMING;
    else
        PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;

    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinToHandle = NULL;
    PinConnect->PinId = PinId;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;

    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX) (PinConnect + 1);

    /* initialize data format */
    DataFormat->WaveFormatEx.wFormatTag = WaveFormatEx->wFormatTag;
    DataFormat->WaveFormatEx.nChannels = WaveFormatEx->nChannels;
    DataFormat->WaveFormatEx.nSamplesPerSec = WaveFormatEx->nSamplesPerSec;
    DataFormat->WaveFormatEx.nBlockAlign = WaveFormatEx->nBlockAlign;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = WaveFormatEx->nAvgBytesPerSec;
    DataFormat->WaveFormatEx.wBitsPerSample = WaveFormatEx->wBitsPerSample;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;

    Result = KsCreatePin(hFilter, PinConnect, GENERIC_READ | GENERIC_WRITE, hPin);

    HeapFree(GetProcessHeap(), 0, PinConnect);

    return Result;
}


DWORD
OpenFilter(
    IN LPCWSTR lpFileName,
    IN PHANDLE OutHandle)
{
    HANDLE Handle;

    /* open the filter */
    Handle = CreateFileW(lpFileName, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL);

    /* check for success */
    if (Handle == INVALID_HANDLE_VALUE)
    {
        DPRINT("Failed to open Filter %ws\n", lpFileName);
        return GetLastError();
    }

    *OutHandle = Handle;
    return ERROR_SUCCESS;
}

DWORD
SyncOverlappedDeviceIoControl(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesTransferred OPTIONAL)
{
    OVERLAPPED Overlapped;
    BOOLEAN IoResult;
    DWORD Transferred = 0;

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (!Overlapped.hEvent)
        return GetLastError();

    /* Talk to the device */
    IoResult = DeviceIoControl(Handle,
                               IoControlCode,
                               InBuffer,
                               InBufferSize,
                               OutBuffer,
                               OutBufferSize,
                               BytesTransferred,
                               &Overlapped);

    /* If failure occurs, make sure it's not just due to the overlapped I/O */
    if (!IoResult)
    {
        if ( GetLastError() != ERROR_IO_PENDING )
        {
            CloseHandle(Overlapped.hEvent);
            return GetLastError();
        }
    }

    /* Wait for the I/O to complete */
    IoResult = GetOverlappedResult(Handle,
                                   &Overlapped,
                                   &Transferred,
                                   TRUE);

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);

    if (!IoResult)
        return GetLastError();

    if ( BytesTransferred )
        *BytesTransferred = Transferred;

    return ERROR_SUCCESS;
}

DWORD
GetFilterPinCount(
    IN HANDLE hFilter,
    OUT PULONG NumPins)
{
    KSPROPERTY Pin;

    *NumPins = 0;

    /* setup the pin request */
    Pin.Flags = KSPROPERTY_TYPE_GET;
    Pin.Set = KSPROPSETID_Pin;
    Pin.Id = KSPROPERTY_PIN_CTYPES;

    /* query the device */
    return SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Pin, sizeof(KSPROPERTY), (PVOID)NumPins, sizeof(ULONG), NULL);
}

DWORD
GetFilterNodeProperty(
    IN HANDLE hFilter,
    IN ULONG PropertyId,
    OUT PKSMULTIPLE_ITEM *OutMultipleItem)
{
    DWORD Status, BytesReturned;
    PKSMULTIPLE_ITEM MultipleItem;
    KSPROPERTY Property;

    /* setup query request */
    Property.Id = PropertyId;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    /* query the size */
    Status = SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    if (Status != ERROR_MORE_DATA)
    {
        /* failed */
        DPRINT("Failed to query PropertyId %lu ErrorCode %lx\n", PropertyId, Status);
        return Status;
    }

    MultipleItem = HeapAlloc(GetProcessHeap(), 0, BytesReturned);
    if (!MultipleItem)
    {
        /* not enough memory */
        DPRINT("Failed to allocate %u Bytes\n", BytesReturned);
        return ERROR_OUTOFMEMORY;
    }

    /* retrieve data ranges */
    Status = SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSP_PIN), (LPVOID)MultipleItem, BytesReturned, &BytesReturned);


    if (Status != ERROR_SUCCESS)
    {
        /* failed to get data ranges */
        DPRINT("SyncOverlappedDeviceIoControl failed with %lx\n", Status);

        HeapFree(GetProcessHeap(), 0, MultipleItem);
        return Status;
    }

    /* save result */
    *OutMultipleItem = MultipleItem;
    return Status;

}

DWORD
GetFilterPinCommunication(
    IN HANDLE hFilter,
    IN ULONG PinId,
    OUT PKSPIN_COMMUNICATION Communication)
{
    KSP_PIN Property;

    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
    Property.PinId = PinId;
    Property.Reserved = 0;

    return SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSP_PIN), (LPVOID)Communication, sizeof(KSPIN_COMMUNICATION), NULL);
}

DWORD
GetFilterPinDataFlow(
    IN HANDLE hFilter,
    IN ULONG PinId,
    OUT PKSPIN_DATAFLOW DataFlow)
{
    KSP_PIN Property;

    Property.Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_DATAFLOW;
    Property.PinId = PinId;
    Property.Reserved = 0;

    return SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSP_PIN), (LPVOID)DataFlow, sizeof(KSPIN_DATAFLOW), NULL);
}

DWORD
GetFilterPinDataRanges(
    IN HANDLE hFilter,
    IN ULONG PinId,
    IN OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSP_PIN Property;
    ULONG BytesReturned = 0;
    DWORD Status;
    PKSMULTIPLE_ITEM MultipleItem;

    /* prepare request */
    Property.Reserved = 0;
    Property.PinId = PinId;
    Property.Property.Set = KSPROPSETID_Pin;
    Property.Property.Id = KSPROPERTY_PIN_DATARANGES;
    Property.Property.Flags = KSPROPERTY_TYPE_GET;

    /* retrieve size of data ranges buffer */
    Status = SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

#if 0
    if (Status != ERROR_MORE_DATA)
    {
        DPRINT("SyncOverlappedDeviceIoControl failed with %lx\n", Status);
        return Status;
    }
#endif

    ASSERT(BytesReturned);
    MultipleItem = HeapAlloc(GetProcessHeap(), 0, BytesReturned);
    if (!MultipleItem)
    {
        /* not enough memory */
        DPRINT("Failed to allocate %u Bytes\n", BytesReturned);
        return ERROR_OUTOFMEMORY;
    }

    /* retrieve data ranges */
    Status = SyncOverlappedDeviceIoControl(hFilter, IOCTL_KS_PROPERTY, (LPVOID)&Property, sizeof(KSP_PIN), (LPVOID)MultipleItem, BytesReturned, &BytesReturned);


    if (Status != ERROR_SUCCESS)
    {
        /* failed to get data ranges */
        DPRINT("SyncOverlappedDeviceIoControl failed with %lx\n", Status);

        HeapFree(GetProcessHeap(), 0, MultipleItem);
        return Status;
    }

    /* save result */
    *OutMultipleItem = MultipleItem;
    return Status;
}

BOOL
CreateCompatiblePin(
    IN HANDLE hFilter,
    IN DWORD PinId,
    IN BOOL bLoop,
    IN LPWAVEFORMATEX WaveFormatEx,
    OUT LPWAVEFORMATEX WaveFormatOut,
    OUT PHANDLE hPin)
{
    PKSMULTIPLE_ITEM Item = NULL;
    PKSDATARANGE_AUDIO AudioRange;
    DWORD dwResult;
    DWORD dwIndex, nChannels;

    dwResult = GetFilterPinDataRanges(hFilter, PinId, &Item);

    if (dwResult != ERROR_SUCCESS)
    {
        /* failed to get data ranges */
         return FALSE;
    }

    CopyMemory(WaveFormatOut, WaveFormatEx, sizeof(WAVEFORMATEX));

    /* iterate through all dataranges */
    AudioRange = (PKSDATARANGE_AUDIO)(Item + 1);
    for(dwIndex = 0; dwIndex < Item->Count; dwIndex++)
    {
        if (AudioRange->DataRange.FormatSize != sizeof(KSDATARANGE_AUDIO))
        {
            UNIMPLEMENTED;
            AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
            continue;
        }

        if (WaveFormatOut->nSamplesPerSec < AudioRange->MinimumSampleFrequency)
            WaveFormatOut->nSamplesPerSec = AudioRange->MinimumSampleFrequency;
        else if (WaveFormatOut->nSamplesPerSec > AudioRange->MaximumSampleFrequency)
            WaveFormatOut->nSamplesPerSec = AudioRange->MaximumSampleFrequency;

        if (WaveFormatOut->wBitsPerSample < AudioRange->MinimumBitsPerSample)
            WaveFormatOut->wBitsPerSample = AudioRange->MinimumBitsPerSample;
        else if (WaveFormatOut->wBitsPerSample > AudioRange->MaximumBitsPerSample)
            WaveFormatOut->wBitsPerSample = AudioRange->MaximumBitsPerSample;

        DPRINT("MinimumBitsPerSample %u MaximumBitsPerSample %u MinimumSampleFrequency %u MaximumSampleFrequency %u\n",
            AudioRange->MinimumBitsPerSample, AudioRange->MaximumBitsPerSample, AudioRange->MinimumSampleFrequency, AudioRange->MaximumSampleFrequency);

        for(nChannels = 1; nChannels <= AudioRange->MaximumChannels; nChannels++)
        {
            WaveFormatOut->nChannels = nChannels;

            dwResult = OpenPin(hFilter, PinId, WaveFormatOut, hPin, TRUE);
            if (dwResult == ERROR_SUCCESS)
            {
                DPRINT("InFormat  nChannels %u wBitsPerSample %u nSamplesPerSec %u\nOutFormat nChannels %u nBitsPerSample %u nSamplesPerSec %u\n",
                       WaveFormatEx->nChannels, WaveFormatEx->wBitsPerSample, WaveFormatEx->nSamplesPerSec,
                       WaveFormatOut->nChannels, WaveFormatOut->wBitsPerSample, WaveFormatOut->nSamplesPerSec);


                /* free buffer */
                HeapFree(GetProcessHeap(), 0, Item);
                return TRUE;
            }
        }
        AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
    }

    /* free buffer */
    HeapFree(GetProcessHeap(), 0, Item);
    return FALSE;
}

