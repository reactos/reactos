/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header containing information class probing helpers
 * COPYRIGHT:   Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

#include <reactos/probe.h>

/**
 * @brief
 * Probe helper that validates the provided parameters whenever
 * a NtSet*** system call is invoked from user or kernel mode.
 *
 * @param[in] Class
 * The specific class information that the caller explicitly
 * requested information to be set into an object.
 *
 * @param[in] ClassList
 * An internal INFORMATION_CLASS_INFO consisting of a list array
 * of information classes checked against the requested information
 * classes given in Class parameter.
 *
 * @param[in] ClassListEntries
 * Specifies the number of class entries in an array, provided by
 * the ClassList parameter.
 *
 * @param[in] Buffer
 * A pointer to an arbitrary data content in memory to be validated.
 * Such pointer points to the actual arbitrary information class buffer
 * to be set into the object. This buffer is validated only if the
 * calling processor mode is UM (aka user mode).
 *
 * @param[in] BufferLength
 * The length of the buffer pointed by the Buffer parameter, whose size
 * is in bytes.
 *
 * @param[in] PreviousMode
 * The processor calling level mode. This level mode determines the procedure
 * of probing validation in action. If the level calling mode is KM (aka kernel mode)
 * this function will only validate the properties of the information class itself
 * such as the required information length size. If the calling mode is UM, the
 * pointer buffer provided by Buffer parameter is also validated.
 *
 * @return
 * The outcome of the probe validation is based upon the returned NTSTATUS code.
 * STATUS_SUCCESS is returned if the validation succeeded. Otherwise, one of the
 * following failure status codes is returned:
 *
 * STATUS_INVALID_INFO_CLASS -- Indicates the given information class is not a valid
 * valid SET class (ICIF_SET flag is not set to the corresponding information class)
 * or the actual class is not present in the class list array.
 *
 * STATUS_INFO_LENGTH_MISMATCH -- Indicates the information length doesn't match with
 * the one that the information class itself expects. This is the case with classes
 * ICIF_SET_SIZE_VARIABLE is not set, which means that the class requires a fixed
 * length size.
 *
 * STATUS_ACCESS_VIOLATION -- Indicates the buffer is not within the user mode probe
 * address range. The function will raise an exception.
 *
 * STATUS_DATATYPE_MISALIGNMENT -- Indicates the address of the buffer in memory is
 * not aligned to the required alignment set.
 */
static
__inline
NTSTATUS
DefaultSetInfoBufferCheck(
    _In_ ULONG Class,
    _In_ const INFORMATION_CLASS_INFO *ClassList,
    _In_ ULONG ClassListEntries,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_ KPROCESSOR_MODE PreviousMode)
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

/**
 * @brief
 * Probe helper that validates the provided parameters whenever
 * a NtQuery*** system call is invoked from user or kernel mode.
 *
 * @param[in] Class
 * The specific class information that the caller explicitly
 * requested information to be queried from an object.
 *
 * @param[in] ClassList
 * An internal INFORMATION_CLASS_INFO consisting of a list array
 * of information classes checked against the requested information
 * classes given in Class parameter.
 *
 * @param[in] ClassListEntries
 * Specifies the number of class entries in an array, provided by
 * the ClassList parameter.
 *
 * @param[in] Flags
 * Specifies a bit mask flag that influences how the query probe
 * validation must be performed against Buffer and ReturnLength
 * parameters. For further information in regard of this parameter,
 * see remarks.
 *
 * @param[in] Buffer
 * A pointer to an arbitrary data content in memory to be validated.
 * Such parameter must be an initialized variable where the queried
 * information is going to be received into this pointer. If the calling
 * processor mode is UM (aka user mode) this parameter is validated.
 * This parameter can be NULL (see remarks for more details).
 *
 * @param[in] BufferLength
 * The length of the buffer pointed by the Buffer parameter, whose size
 * is in bytes. If the Buffer parameter is NULL, this parameter can be 0.
 *
 * @param[in] ReturnLength
 * The returned length of the buffer whose size is in bytes. If Buffer is
 * NULL as well as BufferLength is 0, this parameter receives the actual
 * return length needed to store the buffer in memory space. If the
 * processor level calling mode is UM, this parameter is validated.
 * If ICIF_FORCE_RETURN_LENGTH_PROBE is specified in Flags parameter,
 * ReturnLength mustn't be NULL (see remarks). Otherwise it can be NULL.
 *
 * @param[in] ReturnLengthPtr
 * This parameter is of the same nature as the ReturnLength one, with the
 * difference being that the type parameter can be a ULONG on x86 systems
 * or a ULONGLONG on AMD64 systems. If the processor level calling mode is
 * UM, this parameter is validated. This parameter is currently unused.
 *
 * @param[in] PreviousMode
 * The processor calling level mode. This level mode determines the procedure
 * of probing validation in action. If the level calling mode is KM (aka kernel mode)
 * this function will only validate the properties of the information class itself
 * such as the required information length size. If the calling mode is UM, the
 * pointer buffer provided by Buffer parameter is also validated as well as
 * the return length parameter.
 *
 * @return
 * The outcome of the probe validation is based upon the returned NTSTATUS code.
 * STATUS_SUCCESS is returned if the validation succeeded. Otherwise, one of the
 * following failure status codes is returned:
 *
 * STATUS_INVALID_INFO_CLASS -- Indicates the given information class is not a valid
 * QUERY class (ICIF_QUERY flag is not set to the corresponding information class)
 * or the actual class is not present in the class list array.
 *
 * STATUS_INFO_LENGTH_MISMATCH -- Indicates the information length doesn't match with the
 * one that the information class itself expects. This is the case with classes where
 * ICIF_QUERY_SIZE_VARIABLE is not set, which means that the class requires a fixed length size.
 *
 * STATUS_ACCESS_VIOLATION -- Indicates the buffer is not within the user mode probe address range
 * or the buffer variable is not writable (see remarks). The function will raise an exception.
 *
 * STATUS_DATATYPE_MISALIGNMENT -- Indicates the address of the buffer in memory is not
 * aligned to the required alignment set.
 *
 * @remarks
 * The probing of Buffer and ReturnLength are influenced based on the probe flags
 * pointed by Flags parameter. The following flags are:
 *
 * ICIF_PROBE_READ_WRITE -- This flag explicitly tells the function to do a read and
 * write probe against Buffer parameter. ProbeForWrite is invoked in this case.
 * This is the default mechanism.
 *
 * ICIF_PROBE_READ -- This flag explicitly tells the function to do a read probe against
 * Buffer parameter only, that is, the function does not probe if the parameter is actually
 * writable. ProbeForRead is invoked in this case.
 *
 * ICIF_FORCE_RETURN_LENGTH_PROBE -- If this flag is set, the function will force probe
 * the ReturnLength parameter. In this scenario if ReturnLength is NULL a STATUS_ACCESS_VIOLATION
 * exception is raised. NtQueryInformationToken is the only NT system call where ReturnLength
 * has to be properly initialized and not NULL.
 *
 * Buffer parameter can be NULL if the caller does not want to actually query a certain information
 * from an object. This is typically with query NT syscalls where a caller has to query the actual
 * buffer length needed to store the queried information before doing a "real" query in the first place.
 */
static
__inline
NTSTATUS
DefaultQueryInfoBufferCheck(
    _In_ ULONG Class,
    _In_ const INFORMATION_CLASS_INFO *ClassList,
    _In_ ULONG ClassListEntries,
    _In_ ULONG Flags,
    _In_opt_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_opt_ PULONG ReturnLength,
    _In_opt_ PULONG_PTR ReturnLengthPtr,
    _In_ KPROCESSOR_MODE PreviousMode)
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
                        if (Flags & ICIF_PROBE_READ)
                        {
                            ProbeForRead(Buffer,
                                         BufferLength,
                                         ClassList[Class].AlignmentQUERY);
                        }
                        else
                        {
                            ProbeForWrite(Buffer,
                                          BufferLength,
                                          ClassList[Class].AlignmentQUERY);
                        }
                    }

                    if ((Flags & ICIF_FORCE_RETURN_LENGTH_PROBE) || (ReturnLength != NULL))
                    {
                        ProbeForWriteUlong(ReturnLength);
                    }

                    if (ReturnLengthPtr != NULL)
                    {
                        ProbeForWrite(ReturnLengthPtr, sizeof(ULONG_PTR), sizeof(ULONG_PTR));
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
