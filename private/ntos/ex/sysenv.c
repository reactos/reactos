/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    sysenv.c

Abstract:

    This module implements the NT query and set system environment
    variable services.

Author:

    David N. Cutler (davec) 10-Nov-1991

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include "arccodes.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtQuerySystemEnvironmentValue)
#pragma alloc_text(PAGE, NtSetSystemEnvironmentValue)
#endif

//
// Define maximum size of environment value.
//

#define MAXIMUM_ENVIRONMENT_VALUE 1024

//
// Define query/set environment variable synchronization fast mutex.
//

FAST_MUTEX ExpEnvironmentLock;

NTSTATUS
NtQuerySystemEnvironmentValue (
    IN PUNICODE_STRING VariableName,
    OUT PWSTR VariableValue,
    IN USHORT ValueLength,
    OUT PUSHORT ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function locates the specified system environment variable and
    return its value.

    N.B. This service requires the system environment privilege.

Arguments:

    Variable - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable.

    Value - Supplies a pointer to a buffer that receives the value of the
        specified system environment variable.

    ValueLength - Supplies the length of the value buffer in bytes.

    ReturnLength - Supplies an optional pointer to a variable that receives
        the length of the system environment variable value.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
        privilege to query a system environment variable.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        system environment value or the return length cannot be written,
        or the descriptor or the name of the system environment variable
        cannot be read.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
        for this request to complete.

    STATUS_UNSUCCESSFUL - The specified environment variable could not
        be located.

--*/

{

    ULONG AnsiLength;
    ANSI_STRING AnsiString;
    ARC_STATUS ArcStatus;
    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING UnicodeString;
    PCHAR ValueBuffer;

    //
    // Clear address of ANSI buffer.
    //

    AnsiString.Buffer = NULL;

    //
    // Establish an exception handler and attempt to probe and read the
    // name of the specified system environment variable, and probe the
    // variable value buffer and return length. If the probe or read
    // attempt fails, then return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForRead((PVOID)VariableName,
                         sizeof(UNICODE_STRING),
                         sizeof(ULONG));

            UnicodeString = *VariableName;

            //
            // Probe the system environment variable name.
            //

            if (UnicodeString.Length == 0) {
                return STATUS_ACCESS_VIOLATION;
            }

            ProbeForRead((PVOID)UnicodeString.Buffer,
                         UnicodeString.Length,
                         sizeof(WCHAR));

            //
            // Probe the system environment value buffer.
            //

            ProbeForWrite((PVOID)VariableValue, ValueLength, sizeof(WCHAR));

            //
            // If argument is present, probe the return length value.
            //

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUshort(ReturnLength);
            }

            //
            // Check if the current thread has the privilege to query a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                              PreviousMode);

            if (HasPrivilege == FALSE) {
                return(STATUS_PRIVILEGE_NOT_HELD);
            }

        } else {
            UnicodeString = *VariableName;
        }


        //
        // Compute the size of the ANSI variable name, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable name to ANSI.
        //

        AnsiLength = RtlUnicodeStringToAnsiSize(&UnicodeString);
        AnsiString.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength, 'rvnE');
        if (AnsiString.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString.MaximumLength = (USHORT)AnsiLength;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,
                                                &UnicodeString,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString.Buffer);
            return NtStatus;
        }

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the probe of the variable value, or
    // the probe of the return length, then always handle the exception,
    // free the ANSI string buffer if necessary, and return the exception
    // code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (AnsiString.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString.Buffer);
        }

        return GetExceptionCode();
    }

    //
    // Allocate nonpaged pool to receive variable value.
    //

    ValueBuffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, MAXIMUM_ENVIRONMENT_VALUE, 'rvnE');
    if (ValueBuffer == NULL) {
        ExFreePool((PVOID)AnsiString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the system environment variable value.
    //

    ExAcquireFastMutex(&ExpEnvironmentLock);
    ArcStatus = HalGetEnvironmentVariable(AnsiString.Buffer,
                                          MAXIMUM_ENVIRONMENT_VALUE,
                                          ValueBuffer);

    ExReleaseFastMutex(&ExpEnvironmentLock);

    //
    // Free the ANSI string buffer used to hold the variable name.
    //

    ExFreePool((PVOID)AnsiString.Buffer);

    //
    // If the specified environment variable was not found, then free
    // the value buffer and return an unsuccessful status.
    //

    if (ArcStatus != ESUCCESS) {
        ExFreePool((PVOID)ValueBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Establish an exception handler and attempt to write the value of the
    // specified system environment variable. If the write attempt fails,
    // then return the exception code as the service status.
    //

    try {

        //
        // Initialize an ANSI string descriptor, set the maximum length and
        // buffer address for a UNICODE string descriptor, and convert the
        // ANSI variable value to UNICODE.
        //

        RtlInitString(&AnsiString, ValueBuffer);
        UnicodeString.Buffer = (PWSTR)VariableValue;
        UnicodeString.MaximumLength = ValueLength;
        NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString,
                                                &AnsiString,
                                                FALSE);

        //
        // If argument is present, then write the length of the UNICODE
        // variable value.
        //

        if (ARGUMENT_PRESENT(ReturnLength)) {
            *ReturnLength = UnicodeString.Length;
        }

        //
        // Free the value buffer used to hold the variable value.
        //

        ExFreePool((PVOID)ValueBuffer);
        return NtStatus;

    //
    // If an exception occurs during the write of the variable value, or
    // the write of the return length, then always handle the exception
    // and return the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool((PVOID)ValueBuffer);
        return GetExceptionCode();
    }
}

NTSTATUS
NtSetSystemEnvironmentValue (
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING VariableValue
    )

/*++

Routine Description:

    This function sets the specified system environment variable to the
    specified value.

    N.B. This service requires the system environment privilege.

Arguments:

    Variable - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable name.

    Value - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable value.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
        privilege to set a system environment variable.

    STATUS_ACCESS_VIOLATION is returned if the input parameter for the
        system environment variable or value cannot be read.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
        for this request to complete.

--*/

{

    ULONG AnsiLength1;
    ULONG AnsiLength2;
    ANSI_STRING AnsiString1;
    ANSI_STRING AnsiString2;
    ARC_STATUS ArcStatus;
    BOOLEAN HasPrivilege;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString1;
    UNICODE_STRING UnicodeString2;

    //
    // Clear address of ANSI buffers.
    //

    AnsiString1.Buffer = NULL;
    AnsiString2.Buffer = NULL;

    //
    // Establish an exception handler and attempt to set the value of the
    // specified system environment variable. If the read attempt for the
    // system environment variable or value fails, then return the exception
    // code as the service status. Otherwise, return either success or access
    // denied as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForRead((PVOID)VariableName,
                         sizeof(UNICODE_STRING),
                         sizeof(ULONG));

            UnicodeString1 = *VariableName;

            //
            // Handle a zero length string explicitly since probing does not,
            // the error code is unusual, but it's what we would have done with
            // the HAL return code too.
            //

            if (UnicodeString1.Length == 0) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Probe the system environment variable name.
            //

            ProbeForRead((PVOID)UnicodeString1.Buffer,
                         UnicodeString1.Length,
                         sizeof(WCHAR));

            //
            // Probe and capture the string descriptor for the system
            // environment variable value.
            //

            ProbeForRead((PVOID)VariableValue,
                         sizeof(UNICODE_STRING),
                         sizeof(ULONG));

            UnicodeString2 = *VariableValue;

            //
            // Handle a zero length string explicitly since probing does not
            // the error code is unusual, but it's what we would have done with
            // the HAL return code too.
            //

            if (UnicodeString2.Length == 0) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Probe the system environment variable value.
            //

            ProbeForRead((PVOID)UnicodeString2.Buffer,
                         UnicodeString2.Length,
                         sizeof(WCHAR));

            //
            // Check if the current thread has the privilege to query a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                              PreviousMode);

            if (HasPrivilege == FALSE) {
                return(STATUS_PRIVILEGE_NOT_HELD);
            }

        } else {
            UnicodeString1 = *VariableName;
            UnicodeString2 = *VariableValue;
        }


        //
        // Compute the size of the ANSI variable name, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable name to ANSI.
        //

        AnsiLength1 = RtlUnicodeStringToAnsiSize(&UnicodeString1);
        AnsiString1.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength1, 'rvnE');
        if (AnsiString1.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString1.MaximumLength = (USHORT)AnsiLength1;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString1,
                                                &UnicodeString1,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            return NtStatus;
        }

        //
        // Compute the size of the ANSI variable value, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable value to ANSI.
        //

        AnsiLength2 = RtlUnicodeStringToAnsiSize(&UnicodeString2);
        AnsiString2.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength2, 'rvnE');
        if (AnsiString2.Buffer == NULL) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString2.MaximumLength = (USHORT)AnsiLength2;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString2,
                                                &UnicodeString2,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            ExFreePool((PVOID)AnsiString2.Buffer);
            return NtStatus;
        }

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the read of the value descriptor, or
    // the read of the value, then always handle the exception, free the
    // ANSI string buffers if necessary, and return the exception code as
    // the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (AnsiString1.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString1.Buffer);
        }

        if (AnsiString2.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString2.Buffer);
        }

        return GetExceptionCode();
    }

    //
    // Set the system environment variable value.
    //

    ExAcquireFastMutex(&ExpEnvironmentLock);
    ArcStatus = HalSetEnvironmentVariable(AnsiString1.Buffer,
                                          AnsiString2.Buffer);

    ExReleaseFastMutex(&ExpEnvironmentLock);

    //
    // Free the ANSI string buffers used to hold the variable name and value.
    //

    ExFreePool((PVOID)AnsiString1.Buffer);
    ExFreePool((PVOID)AnsiString2.Buffer);

    //
    // If the specified value of the specified environment variable was
    // successfully set, then return a success status. Otherwise, return
    // insufficient resources.
    //

    if (ArcStatus == ESUCCESS) {
        return STATUS_SUCCESS;

    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}
