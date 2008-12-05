#ifndef __INCLUDE_INTERNAL_PROBE_H
#define __INCLUDE_INTERNAL_PROBE_H

#include <reactos/probe.h>

static
__inline
NTSTATUS
DefaultSetInfoBufferCheck(ULONG Class,
                          const INFORMATION_CLASS_INFO *ClassList,
                          ULONG ClassListEntries,
                          PVOID Buffer,
                          ULONG BufferLength,
                          KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_SET))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeSET > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeSET)
        {
            if (!(ClassList[Class].Flags & ICIF_SET_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH2_TRY
                {
                    ProbeForRead(Buffer,
                                 BufferLength,
                                 ClassList[Class].AlignmentSET);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

static
__inline
NTSTATUS
DefaultQueryInfoBufferCheck(ULONG Class,
                            const INFORMATION_CLASS_INFO *ClassList,
                            ULONG ClassListEntries,
                            PVOID Buffer,
                            ULONG BufferLength,
                            PULONG ReturnLength,
                            PULONG_PTR ReturnLengthLong,
                            KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Class < ClassListEntries)
    {
        if (!(ClassList[Class].Flags & ICIF_QUERY))
        {
            Status = STATUS_INVALID_INFO_CLASS;
        }
        else if (ClassList[Class].RequiredSizeQUERY > 0 &&
                 BufferLength != ClassList[Class].RequiredSizeQUERY)
        {
            if (!(ClassList[Class].Flags & ICIF_QUERY_SIZE_VARIABLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
        }

        if (NT_SUCCESS(Status))
        {
            if (PreviousMode != KernelMode)
            {
                _SEH2_TRY
                {
                    if (Buffer != NULL)
                    {
                        ProbeForWrite(Buffer,
                                      BufferLength,
                                      ClassList[Class].AlignmentQUERY);
                    }

                    if (ReturnLength != NULL)
                    {
                        ProbeForWriteUlong(ReturnLength);
                    }
                    if (ReturnLengthLong != NULL)
                    {
                        ProbeForWrite(ReturnLengthLong, sizeof(ULONG_PTR), sizeof(ULONG_PTR));
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
        }
    }
    else
        Status = STATUS_INVALID_INFO_CLASS;

    return Status;
}

#endif
