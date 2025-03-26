#ifndef INCLUDE_REACTOS_CAPTURE_H
#define INCLUDE_REACTOS_CAPTURE_H

#include <suppress.h>

#if ! defined(_NTOSKRNL_) && ! defined(_WIN32K_)
#error Header intended for use by NTOSKRNL/WIN32K only!
#endif

static const UNICODE_STRING __emptyUnicodeString = {0, 0, NULL};
static const LARGE_INTEGER __emptyLargeInteger = {{0, 0}};
static const ULARGE_INTEGER __emptyULargeInteger = {{0, 0}};
static const IO_STATUS_BLOCK __emptyIoStatusBlock = {{0}, 0};

#if defined(_WIN32K_) && !defined(__cplusplus)
static const LARGE_STRING __emptyLargeString = {0, 0, 0, NULL};
#endif

/*
 * NOTE: Alignment of the pointers is not verified!
 */
#define ProbeForWriteGenericType(Ptr, Type)                                    \
    do {                                                                       \
        if ((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||          \
            (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) { \
            ExRaiseAccessViolation();                                          \
        }                                                                      \
        *(volatile Type *)(Ptr) = *(volatile Type *)(Ptr);                     \
    } while (0)

#define ProbeForWriteBoolean(Ptr) ProbeForWriteGenericType(Ptr, BOOLEAN)
#define ProbeForWriteUchar(Ptr) ProbeForWriteGenericType(Ptr, UCHAR)
#define ProbeForWriteChar(Ptr) ProbeForWriteGenericType(Ptr, CHAR)
#define ProbeForWriteUshort(Ptr) ProbeForWriteGenericType(Ptr, USHORT)
#define ProbeForWriteShort(Ptr) ProbeForWriteGenericType(Ptr, SHORT)
#define ProbeForWriteUlong(Ptr) ProbeForWriteGenericType(Ptr, ULONG)
#define ProbeForWriteLong(Ptr) ProbeForWriteGenericType(Ptr, LONG)
#define ProbeForWriteUint(Ptr) ProbeForWriteGenericType(Ptr, UINT)
#define ProbeForWriteInt(Ptr) ProbeForWriteGenericType(Ptr, INT)
#define ProbeForWriteUlonglong(Ptr) ProbeForWriteGenericType(Ptr, ULONGLONG)
#define ProbeForWriteLonglong(Ptr) ProbeForWriteGenericType(Ptr, LONGLONG)
#define ProbeForWritePointer(Ptr) ProbeForWriteGenericType(Ptr, PVOID)
#define ProbeForWriteHandle(Ptr) ProbeForWriteGenericType(Ptr, HANDLE)
#define ProbeForWriteLangId(Ptr) ProbeForWriteGenericType(Ptr, LANGID)
#define ProbeForWriteSize_t(Ptr) ProbeForWriteGenericType(Ptr, SIZE_T)
#define ProbeForWriteLargeInteger(Ptr) ProbeForWriteGenericType(&((PLARGE_INTEGER)Ptr)->QuadPart, LONGLONG)
#define ProbeForWriteUlargeInteger(Ptr) ProbeForWriteGenericType(&((PULARGE_INTEGER)Ptr)->QuadPart, ULONGLONG)
#define ProbeForWriteUnicodeString(Ptr) ProbeForWriteGenericType((PUNICODE_STRING)Ptr, UNICODE_STRING)
#if defined(_WIN32K_)
#define ProbeForWriteLargeString(Ptr) ProbeForWriteGenericType((PLARGE_STRING)Ptr, LARGE_STRING)
#endif
#define ProbeForWriteIoStatusBlock(Ptr) ProbeForWriteGenericType((PIO_STATUS_BLOCK)Ptr, IO_STATUS_BLOCK)

#define ProbeForReadGenericType(Ptr, Type, Default)                            \
    (((ULONG_PTR)(Ptr) + sizeof(Type) - 1 < (ULONG_PTR)(Ptr) ||                \
     (ULONG_PTR)(Ptr) + sizeof(Type) - 1 >= (ULONG_PTR)MmUserProbeAddress) ?   \
         ExRaiseAccessViolation(), Default :                     \
         *(const volatile Type *)(Ptr))

#define ProbeForReadBoolean(Ptr) ProbeForReadGenericType(Ptr, BOOLEAN, FALSE)
#define ProbeForReadUchar(Ptr) ProbeForReadGenericType(Ptr, UCHAR, 0)
#define ProbeForReadChar(Ptr) ProbeForReadGenericType(Ptr, CHAR, 0)
#define ProbeForReadUshort(Ptr) ProbeForReadGenericType(Ptr, USHORT, 0)
#define ProbeForReadShort(Ptr) ProbeForReadGenericType(Ptr, SHORT, 0)
#define ProbeForReadUlong(Ptr) ProbeForReadGenericType(Ptr, ULONG, 0)
#define ProbeForReadLong(Ptr) ProbeForReadGenericType(Ptr, LONG, 0)
#define ProbeForReadUint(Ptr) ProbeForReadGenericType(Ptr, UINT, 0)
#define ProbeForReadInt(Ptr) ProbeForReadGenericType(Ptr, INT, 0)
#define ProbeForReadUlonglong(Ptr) ProbeForReadGenericType(Ptr, ULONGLONG, 0)
#define ProbeForReadLonglong(Ptr) ProbeForReadGenericType(Ptr, LONGLONG, 0)
#define ProbeForReadPointer(Ptr) ProbeForReadGenericType(Ptr, PVOID, NULL)
#define ProbeForReadHandle(Ptr) ProbeForReadGenericType(Ptr, HANDLE, NULL)
#define ProbeForReadLangId(Ptr) ProbeForReadGenericType(Ptr, LANGID, 0)
#define ProbeForReadSize_t(Ptr) ProbeForReadGenericType(Ptr, SIZE_T, 0)
#define ProbeForReadLargeInteger(Ptr) ProbeForReadGenericType((const LARGE_INTEGER *)(Ptr), LARGE_INTEGER, __emptyLargeInteger)
#define ProbeForReadUlargeInteger(Ptr) ProbeForReadGenericType((const ULARGE_INTEGER *)(Ptr), ULARGE_INTEGER, __emptyULargeInteger)
#define ProbeForReadUnicodeString(Ptr) ProbeForReadGenericType((const UNICODE_STRING *)(Ptr), UNICODE_STRING, __emptyUnicodeString)
#if defined(_WIN32K_)
#define ProbeForReadLargeString(Ptr) ProbeForReadGenericType((const LARGE_STRING *)(Ptr), LARGE_STRING, __emptyLargeString)
#endif
#define ProbeForReadIoStatusBlock(Ptr) ProbeForReadGenericType((const IO_STATUS_BLOCK *)(Ptr), IO_STATUS_BLOCK, __emptyIoStatusBlock)

#define ProbeAndZeroHandle(Ptr) \
    do {                                                                       \
        if ((ULONG_PTR)(Ptr) + sizeof(HANDLE) - 1 < (ULONG_PTR)(Ptr) ||        \
            (ULONG_PTR)(Ptr) + sizeof(HANDLE) - 1 >= (ULONG_PTR)MmUserProbeAddress) { \
            ExRaiseAccessViolation();                                          \
        }                                                                      \
        *(volatile HANDLE *)(Ptr) = NULL;                                      \
    } while (0)

/*
 * Inlined Probing Macros
 */

#if defined(_WIN32K_)
static __inline
VOID
ProbeArrayForRead(IN const VOID *ArrayPtr,
                  IN ULONG ItemSize,
                  IN ULONG ItemCount,
                  IN ULONG Alignment)
{
    ULONG ArraySize;

    /* Check for integer overflow */
    ArraySize = ItemSize * ItemCount;
    if (ArraySize / ItemSize != ItemCount)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    /* Probe the array */
    _PRAGMA_WARNING_SUPPRESS(__WARNING_PROBE_NO_TRY) /* Must be inside __try / __except block */
    ProbeForRead(ArrayPtr, ArraySize, Alignment);
}

static __inline
VOID
ProbeArrayForWrite(IN OUT PVOID ArrayPtr,
                   IN ULONG ItemSize,
                   IN ULONG ItemCount,
                   IN ULONG Alignment)
{
    ULONG ArraySize;

    /* Check for integer overflow */
    ArraySize = ItemSize * ItemCount;
    if (ArraySize / ItemSize != ItemCount)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    /* Probe the array */
    _PRAGMA_WARNING_SUPPRESS(__WARNING_PROBE_NO_TRY) /* Must be inside __try / __except block */
    ProbeForWrite(ArrayPtr, ArraySize, Alignment);
}
#endif /* _WIN32K_ */

static __inline
NTSTATUS
ProbeAndCaptureUnicodeString(OUT PUNICODE_STRING Dest,
                             IN KPROCESSOR_MODE CurrentMode,
                             IN const UNICODE_STRING *UnsafeSrc)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR Buffer = NULL;
    ASSERT(Dest != NULL);

    /* Probe the structure and buffer*/
    if(CurrentMode != KernelMode)
    {
        _SEH2_TRY
        {
#ifdef __cplusplus
            ProbeForRead(UnsafeSrc, sizeof(*UnsafeSrc), 1);
            RtlCopyMemory(Dest, UnsafeSrc, sizeof(*UnsafeSrc));
#else
            *Dest = ProbeForReadUnicodeString(UnsafeSrc);
#endif
            if(Dest->Buffer != NULL)
            {
                if (Dest->Length != 0)
                {
                    ProbeForRead(Dest->Buffer, Dest->Length, sizeof(WCHAR));

                    /* Allocate space for the buffer */
                    Buffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool,
                                                   Dest->Length + sizeof(WCHAR),
                                                   'RTSU');
                    if (Buffer == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        _SEH2_LEAVE;
                    }

                    /* Copy it */
                    RtlCopyMemory(Buffer, Dest->Buffer, Dest->Length);
                    Buffer[Dest->Length / sizeof(WCHAR)] = UNICODE_NULL;

                    /* Set it as the buffer */
                    Dest->Buffer = Buffer;
                    if (Dest->Length % sizeof(WCHAR))
                    {
                        Dest->Length--;
                    }
                    if (Dest->Length >= UNICODE_STRING_MAX_BYTES)
                    {
                        Dest->MaximumLength = Dest->Length;
                    }
                    else
                    {
                        Dest->MaximumLength = Dest->Length + sizeof(WCHAR);
                    }
                }
                else
                {
                    /* Sanitize structure */
                    Dest->MaximumLength = 0;
                    Dest->Buffer = NULL;
                }
            }
            else
            {
                /* Sanitize structure */
                Dest->Length = 0;
                Dest->MaximumLength = 0;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Free allocated resources and zero the destination string */
            if (Buffer != NULL)
            {
                ExFreePoolWithTag(Buffer, 'RTSU');
            }
            Dest->Length = 0;
            Dest->MaximumLength = 0;
            Dest->Buffer = NULL;

            /* Return the error code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        /* Just copy the UNICODE_STRING structure, don't allocate new memory!
           We trust the caller to supply valid pointers and data. */
        *Dest = *UnsafeSrc;
    }

    /* Return */
    return Status;
}

static __inline
VOID
ReleaseCapturedUnicodeString(IN PUNICODE_STRING CapturedString,
                             IN KPROCESSOR_MODE CurrentMode)
{
    if(CurrentMode != KernelMode && CapturedString->Buffer != NULL)
    {
        ExFreePoolWithTag(CapturedString->Buffer, 'RTSU');
    }

    CapturedString->Length = 0;
    CapturedString->MaximumLength = 0;
    CapturedString->Buffer = NULL;
}

#endif /* INCLUDE_REACTOS_CAPTURE_H */
