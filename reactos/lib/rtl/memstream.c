/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/mem.c
 * PURPOSE:         MemoryStream functions
 * PROGRAMMER:      David Quintana (gigaherz@gmail.com)
 */

/* INCLUDES *******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* VIRTUAL METHOD TABLES ******************************************************/

const struct IStreamVtbl RtlMemoryStreamVtbl =
{
    RtlQueryInterfaceMemoryStream,
    RtlAddRefMemoryStream,
    RtlReleaseMemoryStream,
    RtlReadMemoryStream,
    RtlWriteMemoryStream,
    RtlSeekMemoryStream,
    RtlSetMemoryStreamSize,
    RtlCopyMemoryStreamTo,
    RtlCommitMemoryStream,
    RtlRevertMemoryStream,
    RtlLockMemoryStreamRegion,
    RtlUnlockMemoryStreamRegion,
    RtlStatMemoryStream,
    RtlCloneMemoryStream,
};

const struct IStreamVtbl RtlOutOfProcessMemoryStreamVtbl =
{
    RtlQueryInterfaceMemoryStream,
    RtlAddRefMemoryStream,
    RtlReleaseMemoryStream,
    RtlReadOutOfProcessMemoryStream,
    RtlWriteMemoryStream,
    RtlSeekMemoryStream,
    RtlSetMemoryStreamSize,
    RtlCopyMemoryStreamTo,
    RtlCommitMemoryStream,
    RtlRevertMemoryStream,
    RtlLockMemoryStreamRegion,
    RtlUnlockMemoryStreamRegion,
    RtlStatMemoryStream,
    RtlCloneMemoryStream,
};

/* FUNCTIONS ******************************************************************/

static
PRTL_MEMORY_STREAM
IStream_To_RTL_MEMORY_STREAM(
    _In_ IStream *Interface)
{
    if (Interface == NULL)
        return NULL;

    return CONTAINING_RECORD(Interface, RTL_MEMORY_STREAM, Vtbl);
}

/*
 * @implemented
 */
VOID
NTAPI
RtlInitMemoryStream(
    _Out_ PRTL_MEMORY_STREAM Stream)
{
    RtlZeroMemory(Stream, sizeof(RTL_MEMORY_STREAM));
    Stream->Vtbl = &RtlMemoryStreamVtbl;
}

/*
 * @implemented
 */
VOID
NTAPI
RtlInitOutOfProcessMemoryStream(
    _Out_ PRTL_MEMORY_STREAM Stream)
{
    RtlZeroMemory(Stream, sizeof(RTL_MEMORY_STREAM));
    Stream->Vtbl = &RtlOutOfProcessMemoryStreamVtbl;
    Stream->FinalRelease = RtlFinalReleaseOutOfProcessMemoryStream;
}

/*
 * @unimplemented
 */
VOID
NTAPI
RtlFinalReleaseOutOfProcessMemoryStream(
    _In_ PRTL_MEMORY_STREAM Stream)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlQueryInterfaceMemoryStream(
    _In_ IStream *This,
    _In_ REFIID RequestedIid,
    _Outptr_ PVOID *ResultObject)
{
    if (IsEqualGUID(RequestedIid, &IID_IUnknown) ||
        IsEqualGUID(RequestedIid, &IID_ISequentialStream) ||
        IsEqualGUID(RequestedIid, &IID_IStream))
    {
        IStream_AddRef(This);
        *ResultObject = This;
        return S_OK;
    }

    *ResultObject = NULL;
    return E_NOINTERFACE;
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlAddRefMemoryStream(
    _In_ IStream *This)
{
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);

    return InterlockedIncrement(&Stream->RefCount);
}

/*
 * @implemented
 */
ULONG
NTAPI
RtlReleaseMemoryStream(
    _In_ IStream *This)
{
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);
    LONG Result;

    Result = InterlockedDecrement(&Stream->RefCount);

    if (Result == 0)
    {
        if (Stream->FinalRelease)
            Stream->FinalRelease(Stream);
    }

    return Result;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlReadMemoryStream(
    _In_ IStream *This,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesRead)
{
    ULONG CopyLength;
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);
    SIZE_T Available = (PUCHAR)Stream->End - (PUCHAR)Stream->Current;

    if (BytesRead)
        *BytesRead = 0;

    if (!Length)
        return S_OK;

    CopyLength = min(Available, Length);

    RtlMoveMemory(Buffer, Stream->Current, CopyLength);

    Stream->Current = (PUCHAR)Stream->Current + CopyLength;

    *BytesRead = CopyLength;

    return S_OK;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlReadOutOfProcessMemoryStream(
    _In_ IStream *This,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesRead)
{
    NTSTATUS Status;
    ULONG CopyLength;
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);
    SIZE_T Available = (PUCHAR)Stream->End - (PUCHAR)Stream->Current;

    if (BytesRead)
        *BytesRead = 0;

    if (!Length)
        return S_OK;

    CopyLength = min(Available, Length);

    Status = NtReadVirtualMemory(Stream->ProcessHandle,
                                 Stream->Current,
                                 Buffer,
                                 CopyLength,
                                 BytesRead);

    if (NT_SUCCESS(Status))
        Stream->Current = (PUCHAR)Stream->Current + *BytesRead;

    return HRESULT_FROM_WIN32(RtlNtStatusToDosError(Status));
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlSeekMemoryStream(
    _In_ IStream *This,
    _In_ LARGE_INTEGER RelativeOffset,
    _In_ ULONG Origin,
    _Out_opt_ PULARGE_INTEGER ResultOffset)
{
    PVOID NewPosition;
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);

    switch (Origin)
    {
        case STREAM_SEEK_SET:
            NewPosition = (PUCHAR)Stream->Start + RelativeOffset.QuadPart;
            break;

        case STREAM_SEEK_CUR:
            NewPosition = (PUCHAR)Stream->Current + RelativeOffset.QuadPart;
            break;

        case STREAM_SEEK_END:
            NewPosition = (PUCHAR)Stream->End - RelativeOffset.QuadPart;
            break;

        default:
            return E_INVALIDARG;
    }

    if (NewPosition < Stream->Start || NewPosition > Stream->End)
        return STG_E_INVALIDPOINTER;

    Stream->Current = NewPosition;

    if (ResultOffset)
        ResultOffset->QuadPart = (PUCHAR)Stream->Current - (PUCHAR)Stream->Start;

    return S_OK;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlCopyMemoryStreamTo(
    _In_ IStream *This,
    _In_ IStream *Target,
    _In_ ULARGE_INTEGER Length,
    _Out_opt_ PULARGE_INTEGER BytesRead,
    _Out_opt_ PULARGE_INTEGER BytesWritten)
{
    CHAR Buffer[1024];
    ULONGLONG TotalSize;
    ULONG Left, Amount;
    HRESULT Result;

    if (BytesRead)
        BytesRead->QuadPart = 0;
    if (BytesWritten)
        BytesWritten->QuadPart = 0;

    if (!Target)
        return S_OK;

    if (!Length.QuadPart)
        return S_OK;

    /* Copy data */
    TotalSize = Length.QuadPart;
    while (TotalSize)
    {
        Left = (ULONG)min(TotalSize, sizeof(Buffer));

        /* Read */
        Result = IStream_Read(This, Buffer, Left, &Amount);
        if (BytesRead)
            BytesRead->QuadPart += Amount;
        if (FAILED(Result) || Amount == 0)
            break;

        Left = Amount;

        /* Write */
        Result = IStream_Write(Target, Buffer, Left, &Amount);
        if (BytesWritten)
            BytesWritten->QuadPart += Amount;
        if (FAILED(Result) || Amount != Left)
            break;

        TotalSize -= Left;
    }
    return Result;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlStatMemoryStream(
    _In_ IStream *This,
    _Out_ STATSTG *Stats,
    _In_ ULONG Flags)
{
    PRTL_MEMORY_STREAM Stream = IStream_To_RTL_MEMORY_STREAM(This);

    if (!Stats)
        return STG_E_INVALIDPOINTER;

    RtlZeroMemory(Stats, sizeof(STATSTG));
    Stats->type = STGTY_STREAM;
    Stats->cbSize.QuadPart = (PUCHAR)Stream->End - (PUCHAR)Stream->Start;

    return S_OK;
}

/* DUMMY FUNCTIONS ************************************************************/
/*
 * The following functions return E_NOTIMPL in Windows Server 2003.
 */

/*
 * @implemented
 */
HRESULT
NTAPI
RtlWriteMemoryStream(
    _In_ IStream *This,
    _In_reads_bytes_(Length) CONST VOID *Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG BytesWritten)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(BytesWritten);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlSetMemoryStreamSize(
    _In_ IStream *This,
    _In_ ULARGE_INTEGER NewSize)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(NewSize);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlCommitMemoryStream(
    _In_ IStream *This,
    _In_ ULONG CommitFlags)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(CommitFlags);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlRevertMemoryStream(
    _In_ IStream *This)
{
    UNREFERENCED_PARAMETER(This);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlLockMemoryStreamRegion(
    _In_ IStream *This,
    _In_ ULARGE_INTEGER Offset,
    _In_ ULARGE_INTEGER Length,
    _In_ ULONG LockType)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockType);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlUnlockMemoryStreamRegion(
    _In_ IStream *This,
    _In_ ULARGE_INTEGER Offset,
    _In_ ULARGE_INTEGER Length,
    _In_ ULONG LockType)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(Offset);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(LockType);

    return E_NOTIMPL;
}

/*
 * @implemented
 */
HRESULT
NTAPI
RtlCloneMemoryStream(
    _In_ IStream *This,
    _Outptr_ IStream **ResultStream)
{
    UNREFERENCED_PARAMETER(This);
    UNREFERENCED_PARAMETER(ResultStream);

    return E_NOTIMPL;
}
