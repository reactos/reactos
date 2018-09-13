/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    environ.c

Abstract:

    Environment Variable support

Author:

    Steven R. Wood (stevewo) 30-Jan-1991

Revision History:

--*/

#include "ntrtlp.h"
#include "zwapi.h"
#include "nturtl.h"
#include "string.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(INIT,RtlCreateEnvironment          )
#pragma alloc_text(INIT,RtlDestroyEnvironment         )
#pragma alloc_text(INIT,RtlSetCurrentEnvironment      )
#pragma alloc_text(INIT,RtlQueryEnvironmentVariable_U )
#pragma alloc_text(INIT,RtlSetEnvironmentVariable     )
#endif

NTSTATUS
RtlCreateEnvironment(
    IN BOOLEAN CloneCurrentEnvironment OPTIONAL,
    OUT PVOID *Environment
    )
{
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    PVOID pNew, pOld;

    //
    // If not cloning a copy of the current process's environment variable
    // block, just allocate a block of committed memory and return its
    // address.
    //

    pNew = NULL;
    if (!CloneCurrentEnvironment) {
createEmptyEnvironment:
        MemoryInformation.RegionSize = 1;
        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          &pNew,
                                          0,
                                          &MemoryInformation.RegionSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (NT_SUCCESS( Status )) {
            *Environment = pNew;
            }

        return( Status );
        }

    //
    // Acquire the Peb Lock for the duration while we munge the environment
    // variable storage block.
    //

    RtlAcquirePebLock();

    //
    // Capture the pointer to the current process's environment variable
    // block and initialize the new pointer to null for our finally clause.
    //

    pOld = NtCurrentPeb()->ProcessParameters->Environment;
    if (pOld == NULL) {
        RtlReleasePebLock();
        goto createEmptyEnvironment;
        }

    try {
        //
        // Query the current size of the current process's environment
        // variable block.  Return status if failure.
        //

        Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                       pOld,
                                       MemoryBasicInformation,
                                       &MemoryInformation,
                                       sizeof( MemoryInformation ),
                                       NULL
                                     );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        //
        // Allocate memory to contain a copy of the current process's
        // environment variable block.  Return status if failure.
        //

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          &pNew,
                                          0,
                                          &MemoryInformation.RegionSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE
                                        );
        if (!NT_SUCCESS( Status )) {
            return( Status );
            }

        //
        // Copy the current process's environment to the allocated memory
        // and return a pointer to the copy.
        //

        RtlMoveMemory( pNew, pOld, MemoryInformation.RegionSize );
        *Environment = pNew;
        }
    finally {
        if (AbnormalTermination()) {
            Status = STATUS_ACCESS_VIOLATION;
            if (pNew != NULL) {
                ZwFreeVirtualMemory( NtCurrentProcess(),
                                     &pNew,
                                     &MemoryInformation.RegionSize,
                                     MEM_RELEASE
                                   );
                }
            }

        RtlReleasePebLock();
        }

    return( Status );
}


NTSTATUS
RtlDestroyEnvironment(
    IN PVOID Environment
    )
{
    NTSTATUS Status;
    SIZE_T RegionSize;

    //
    // Free the specified environment variable block.
    //

    RegionSize = 0;
    Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                  &Environment,
                                  &RegionSize,
                                  MEM_RELEASE
                                );
    //
    // Return status.
    //

    return( Status );
}


NTSTATUS
RtlSetCurrentEnvironment(
    IN PVOID Environment,
    OUT PVOID *PreviousEnvironment OPTIONAL
    )
{
    NTSTATUS Status;
    PVOID pOld;

    //
    // Acquire the Peb Lock for the duration while we munge the environment
    // variable storage block.
    //

    RtlAcquirePebLock();

    Status = STATUS_SUCCESS;
    try {
        //
        // Capture current process's environment variable block pointer to
        // return to caller or destroy.
        //

        pOld = NtCurrentPeb()->ProcessParameters->Environment;

        //
        // Change current process's environment variable block pointer to
        // point to the passed block.
        //


        NtCurrentPeb()->ProcessParameters->Environment = Environment;

        //
        // If caller requested it, return the pointer to the previous
        // process environment variable block and set the local variable
        // to NULL so we dont destroy it below.
        //

        if (ARGUMENT_PRESENT( PreviousEnvironment )) {
            *PreviousEnvironment = pOld;
            pOld = NULL;
            }
        }
    finally {
        if (AbnormalTermination()) {
            Status = STATUS_ACCESS_VIOLATION;
            pOld = NULL;
            }
        }

    //
    // Release the Peb Lock
    //

    RtlReleasePebLock();


    //
    // If old environment not returned to caller, destroy it.
    //

    if (pOld != NULL) {
        RtlDestroyEnvironment( pOld );
        }

    //
    // Return status
    //

    return( Status );
}

BOOLEAN RtlpEnvironCacheValid;
UNICODE_STRING RtlpEnvironCacheName;
UNICODE_STRING RtlpEnvironCacheValue;

NTSTATUS
RtlQueryEnvironmentVariable_U(
    IN PVOID Environment OPTIONAL,
    IN PUNICODE_STRING Name,
    IN OUT PUNICODE_STRING Value
    )
{
    NTSTATUS Status;
    UNICODE_STRING CurrentName;
    UNICODE_STRING CurrentValue;
    PWSTR p;
    PPEB Peb;

    Status = STATUS_VARIABLE_NOT_FOUND;
    Peb = NtCurrentPeb();

    try {
        if (ARGUMENT_PRESENT( Environment )) {
            p = Environment;
            if (*p == UNICODE_NULL) {
                leave;
                }
            }
        else {
            //
            // Acquire the Peb Lock for the duration while we munge the
            // environment variable storage block.
            //

            RtlAcquirePebLock();

            //
            // Capture the pointer to the current process's environment variable
            // block.
            //

            p = Peb->ProcessParameters->Environment;

            }
#if DBG
        if (*p == UNICODE_NULL)
            DbgPrint( "RTL: QEV - Empty Environment being searched: %08x\n", p);
        else if ((UCHAR)((*p) >> 8) != '\0')
            DbgPrint( "RTL: QEV - Possible ANSI Environment being searched: %08x\n", p);
#endif

        if ( RtlpEnvironCacheValid && p == Peb->ProcessParameters->Environment ) {
            if (RtlEqualUnicodeString( Name, &RtlpEnvironCacheName, TRUE )) {

                //
                // Names are equal.  Always return the length of the
                // value string, excluding the terminating null.  If
                // there is room in the caller's buffer, return a copy
                // of the value string and success status.  Otherwise
                // return an error status.  In the latter case, the caller
                // can examine the length field of their value string
                // so they can determine much memory is needed.
                //

                Value->Length = RtlpEnvironCacheValue.Length;
                if (Value->MaximumLength >= RtlpEnvironCacheValue.Length) {
                    RtlMoveMemory( Value->Buffer,
                                   RtlpEnvironCacheValue.Buffer,
                                   RtlpEnvironCacheValue.Length
                                 );
                    //
                    // Null terminate returned string if there is room.
                    //

                    if (Value->MaximumLength > RtlpEnvironCacheValue.Length) {
                        Value->Buffer[ RtlpEnvironCacheValue.Length/sizeof(WCHAR) ] = L'\0';
                        }

                    Status = STATUS_SUCCESS;
                    }
                else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    }
                goto environcachehit;
                }
            }

        //
        // The environment variable block consists of zero or more null
        // terminated UNICODE strings.  Each string is of the form:
        //
        //      name=value
        //
        // where the null termination is after the value.
        //

        if (p != NULL) while (*p) {
            //
            // Determine the size of the name and value portions of
            // the current string of the environment variable block.
            //

            CurrentName.Buffer = p;
            CurrentName.Length = 0;
            CurrentName.MaximumLength = 0;
            while (*p) {
                //
                // If we see an equal sign, then compute the size of
                // the name portion and scan for the end of the value.
                //

                if (*p == L'=' && p != CurrentName.Buffer) {
                    CurrentName.Length = (USHORT)(p - CurrentName.Buffer)*sizeof(WCHAR);
                    CurrentName.MaximumLength = (USHORT)(CurrentName.Length+sizeof(WCHAR));
                    CurrentValue.Buffer = ++p;

                    while(*p) {
                        p++;
                        }
                    CurrentValue.Length = (USHORT)(p - CurrentValue.Buffer)*sizeof(WCHAR);
                    CurrentValue.MaximumLength = (USHORT)(CurrentValue.Length+sizeof(WCHAR));

                    //
                    // At this point we have the length of both the name
                    // and value portions, so exit the loop so we can
                    // do the compare.
                    //
                    break;
                    }
                else {
                    p++;
                    }
                }

            //
            // Skip over the terminating null character for this name=value
            // pair in preparation for the next iteration of the loop.
            //

            p++;

            //
            // Compare the current name with the one requested, ignore
            // case.
            //

            if (RtlEqualUnicodeString( Name, &CurrentName, TRUE )) {
                //
                // Names are equal.  Always return the length of the
                // value string, excluding the terminating null.  If
                // there is room in the caller's buffer, return a copy
                // of the value string and success status.  Otherwise
                // return an error status.  In the latter case, the caller
                // can examine the length field of their value string
                // so they can determine much memory is needed.
                //

                Value->Length = CurrentValue.Length;
                if (Value->MaximumLength >= CurrentValue.Length) {
                    RtlMoveMemory( Value->Buffer,
                                   CurrentValue.Buffer,
                                   CurrentValue.Length
                                 );
                    //
                    // Null terminate returned string if there is room.
                    //

                    if (Value->MaximumLength > CurrentValue.Length) {
                        Value->Buffer[ CurrentValue.Length/sizeof(WCHAR) ] = L'\0';
                        }

                    if ( !Environment || Environment == Peb->ProcessParameters->Environment) {
                        RtlpEnvironCacheValid = TRUE;
                        RtlpEnvironCacheName = CurrentName;
                        RtlpEnvironCacheValue = CurrentValue;
                        }

                    Status = STATUS_SUCCESS;
                    }
                else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                    }
                break;
                }
            }
environcachehit:;
        }
    finally {
        //
        // If abnormally terminating, assume access violation.
        //

        if (AbnormalTermination()) {
            Status = STATUS_ACCESS_VIOLATION;
            }

        //
        // Release the Peb lock.
        //

        if (!ARGUMENT_PRESENT( Environment )) {
            RtlReleasePebLock();
            }
        }

    //
    // Return status.
    //

    return( Status );
}


NTSTATUS
RtlSetEnvironmentVariable(
    IN OUT PVOID *Environment OPTIONAL,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING Value OPTIONAL
    )
{
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    UNICODE_STRING CurrentName;
    UNICODE_STRING CurrentValue;
    PVOID pOld, pNew;
    ULONG n, Size;
    SIZE_T NewSize;
    LONG CompareResult;
    PWSTR p, pStart, pEnd;

    //
    // Validate passed in name and reject if zero length or anything but the first
    // character is an equal sign.
    //
    n = Name->Length / sizeof( WCHAR );
    if (n == 0) {
        return STATUS_INVALID_PARAMETER;
        }

    try {
        p = Name->Buffer;
        while (--n) {
            if (*++p == L'=') {
                return STATUS_INVALID_PARAMETER;
                }
            }
        }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
        }

    RtlpEnvironCacheValid = FALSE;
    Status = STATUS_VARIABLE_NOT_FOUND;
    if (ARGUMENT_PRESENT( Environment )) {
        pOld = *Environment;
        }
    else {
        //
        // Acquire the Peb Lock for the duration while we munge the
        // environment variable storage block.
        //

        RtlAcquirePebLock();

        //
        // Capture the pointer to the current process's environment variable
        // block.
        //

        pOld = NtCurrentPeb()->ProcessParameters->Environment;
        }
    pNew = NULL;

    try {
        //
        // The environment variable block consists of zero or more null
        // terminated UNICODE strings.  Each string is of the form:
        //
        //      name=value
        //
        // where the null termination is after the value.
        //

        p = pOld;
        pEnd = NULL;
        if (p != NULL) while (*p) {
            //
            // Determine the size of the name and value portions of
            // the current string of the environment variable block.
            //

            CurrentName.Buffer = p;
            CurrentName.Length = 0;
            CurrentName.MaximumLength = 0;
            while (*p) {
                //
                // If we see an equal sign, then compute the size of
                // the name portion and scan for the end of the value.
                //

                if (*p == L'=' && p != CurrentName.Buffer) {
                    CurrentName.Length = (USHORT)(p - CurrentName.Buffer) * sizeof(WCHAR);
                    CurrentName.MaximumLength = (USHORT)(CurrentName.Length+sizeof(WCHAR));
                    CurrentValue.Buffer = ++p;

                    while(*p) {
                        p++;
                        }
                    CurrentValue.Length = (USHORT)(p - CurrentValue.Buffer) * sizeof(WCHAR);
                    CurrentValue.MaximumLength = (USHORT)(CurrentValue.Length+sizeof(WCHAR));

                    //
                    // At this point we have the length of both the name
                    // and value portions, so exit the loop so we can
                    // do the compare.
                    //
                    break;
                    }
                else {
                    p++;
                    }
                }

            //
            // Skip over the terminating null character for this name=value
            // pair in preparation for the next iteration of the loop.
            //

            p++;

            //
            // Compare the current name with the one requested, ignore
            // case.
            //

            if (!(CompareResult = RtlCompareUnicodeString( Name, &CurrentName, TRUE ))) {
                //
                // Names are equal.  Now find the end of the current
                // environment variable block.
                //

                pEnd = p;
                while (*pEnd) {
                    while (*pEnd++) {
                        }
                    }
                pEnd++;

                if (!ARGUMENT_PRESENT( Value )) {
                    //
                    // If the caller did not specify a new value, then delete
                    // the entire name=value pair by copying up the remainder
                    // of the environment variable block.
                    //

                    RtlMoveMemory( CurrentName.Buffer,
                                   p,
                                   (ULONG) ((pEnd - p)*sizeof(WCHAR))
                                 );
                    Status = STATUS_SUCCESS;
                    }
                else
                if (Value->Length <= CurrentValue.Length) {
                    //
                    // New value is smaller, so copy new value, then null
                    // terminate it, and then move up the remainder of the
                    // variable block so it is immediately after the new
                    // null terminated value.
                    //

                    pStart = CurrentValue.Buffer;
                    RtlMoveMemory( pStart, Value->Buffer, Value->Length );
                    pStart += Value->Length/sizeof(WCHAR);
                    *pStart++ = L'\0';

                    RtlMoveMemory( pStart, p,(ULONG)((pEnd - p)*sizeof(WCHAR)) );
                    Status = STATUS_SUCCESS;
                    }
                else {
                    //
                    // New value is larger, so query the current size of the
                    // environment variable block.  Return status if failure.
                    //

                    Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                                   pOld,
                                                   MemoryBasicInformation,
                                                   &MemoryInformation,
                                                   sizeof( MemoryInformation ),
                                                   NULL
                                                 );
                    if (!NT_SUCCESS( Status )) {
                        return( Status );
                        }

                    //
                    // See if there is room for new, larger value.  If not
                    // allocate a new copy of the environment variable
                    // block.
                    //

                    NewSize = (pEnd - (PWSTR)pOld)*sizeof(WCHAR) +
                                Value->Length - CurrentValue.Length;
                    if (NewSize >= MemoryInformation.RegionSize) {
                        //
                        // Allocate memory to contain a copy of the current
                        // process's environment variable block.  Return
                        // status if failure.
                        //

                        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                                          &pNew,
                                                          0,
                                                          &NewSize,
                                                          MEM_COMMIT,
                                                          PAGE_READWRITE
                                                        );
                        if (!NT_SUCCESS( Status )) {
                            return( Status );
                            }

                        //
                        // Copy the current process's environment to the allocated memory
                        // inserting the new value as we do the copy.
                        //

                        Size = (ULONG) (CurrentValue.Buffer - (PWSTR)pOld);
                        RtlMoveMemory( pNew, pOld, Size*sizeof(WCHAR) );
                        pStart = (PWSTR)pNew + Size;
                        RtlMoveMemory( pStart, Value->Buffer, Value->Length );
                        pStart += Value->Length/sizeof(WCHAR);
                        *pStart++ = L'\0';
                        RtlMoveMemory( pStart, p,(ULONG)((pEnd - p)*sizeof(WCHAR)));
			if (ARGUMENT_PRESENT( Environment ))
			    *Environment = pNew;
                        else {
			    NtCurrentPeb()->ProcessParameters->Environment = pNew;
                            NtCurrentPeb()->EnvironmentUpdateCount += 1;
                            }

			ZwFreeVirtualMemory( NtCurrentProcess(),
                                     &pOld,
                                     &MemoryInformation.RegionSize,
                                     MEM_RELEASE
                                   );
                        pNew = pOld;
                        }
                    else {
                        pStart = CurrentValue.Buffer + Value->Length/sizeof(WCHAR) + 1;
                        RtlMoveMemory( pStart, p,(ULONG)((pEnd - p)*sizeof(WCHAR)));
                        *--pStart = L'\0';

                        RtlMoveMemory( pStart - Value->Length/sizeof(WCHAR),
                                       Value->Buffer,
                                       Value->Length
                                     );
                        }
                    }

                break;
                }
            else
            if (CompareResult < 0) {
                //
                // Request name is less than current name, then look no
                // further as we will not find it in our sorted list.
                // The insertion point for the new variable is before the
                // variable just examined.
                //

                p = CurrentName.Buffer;
                break;
                }
            }

        //
        // If variable name not found and a new value parameter was specified
        // then insert the new variable name and its value at the appropriate
        // place in the environment variable block (i.e. where p points to).
        //

        if (pEnd == NULL && ARGUMENT_PRESENT( Value )) {
            if (p != NULL) {
                //
                // Name not found.  Now find the end of the current
                // environment variable block.
                //

                pEnd = p;
                while (*pEnd) {
                    while (*pEnd++) {
                        }
                    }
                pEnd++;

                //
                // New value is present, so query the current size of the
                // environment variable block.  Return status if failure.
                //

                Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                               pOld,
                                               MemoryBasicInformation,
                                               &MemoryInformation,
                                               sizeof( MemoryInformation ),
                                               NULL
                                             );
                if (!NT_SUCCESS( Status )) {
                    return( Status );
                    }

                //
                // See if there is room for new, larger value.  If not
                // allocate a new copy of the environment variable
                // block.
                //

                NewSize = (pEnd - (PWSTR)pOld) * sizeof(WCHAR) +
                          Name->Length +
                          sizeof(WCHAR) +
                          Value->Length +
                          sizeof(WCHAR);
                }
            else {
                NewSize = Name->Length +
                          sizeof(WCHAR) +
                          Value->Length +
                          sizeof(WCHAR);
                MemoryInformation.RegionSize = 0;
                }

            if (NewSize >= MemoryInformation.RegionSize) {
                //
                // Allocate memory to contain a copy of the current
                // process's environment variable block.  Return
                // status if failure.
                //

                Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                                  &pNew,
                                                  0,
                                                  &NewSize,
                                                  MEM_COMMIT,
                                                  PAGE_READWRITE
                                                );
                if (!NT_SUCCESS( Status )) {
                    return( Status );
                    }

                //
                // Copy the current process's environment to the allocated memory
                // inserting the new value as we do the copy.
                //

                if (p != NULL) {
                    Size = (ULONG)(p - (PWSTR)pOld);
                    RtlMoveMemory( pNew, pOld, Size*sizeof(WCHAR) );
                    }
                else {
                    Size = 0;
                    }
                pStart = (PWSTR)pNew + Size;
                RtlMoveMemory( pStart, Name->Buffer, Name->Length );
                pStart += Name->Length/sizeof(WCHAR);
                *pStart++ = L'=';
                RtlMoveMemory( pStart, Value->Buffer, Value->Length );
                pStart += Value->Length/sizeof(WCHAR);
                *pStart++ = L'\0';
                if (p != NULL) {
                    RtlMoveMemory( pStart, p,(ULONG)((pEnd - p)*sizeof(WCHAR)) );
                    }

		if (ARGUMENT_PRESENT( Environment ))
		    *Environment = pNew;
                else {
		    NtCurrentPeb()->ProcessParameters->Environment = pNew;
                    NtCurrentPeb()->EnvironmentUpdateCount += 1;
                    }
                ZwFreeVirtualMemory( NtCurrentProcess(),
                                     &pOld,
                                     &MemoryInformation.RegionSize,
                                     MEM_RELEASE
                                   );
                }
            else {
                pStart = p + Name->Length/sizeof(WCHAR) + 1 + Value->Length/sizeof(WCHAR) + 1;
                RtlMoveMemory( pStart, p,(ULONG)((pEnd - p)*sizeof(WCHAR)) );
                RtlMoveMemory( p, Name->Buffer, Name->Length );
                p += Name->Length/sizeof(WCHAR);
                *p++ = L'=';
                RtlMoveMemory( p, Value->Buffer, Value->Length );
                p += Value->Length/sizeof(WCHAR);
                *p++ = L'\0';
                }
            }
        }
    finally {
        //
        // Release the Peb lock.
        //

        if (!ARGUMENT_PRESENT( Environment )) {
            RtlReleasePebLock();
            }

        //
        // If abnormally terminating, assume access violation.
        //

        if (AbnormalTermination()) {
            return (STATUS_ACCESS_VIOLATION);
            }
        }

    //
    // Return status.
    //

    return( Status );
}
