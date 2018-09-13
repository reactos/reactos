/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements the Win32 handle management services.

Author:

    Mark Lucovsky (markl) 21-Sep-1990

Revision History:

--*/

#include "basedll.h"

BOOL
CloseHandle(
    HANDLE hObject
    )

/*++

Routine Description:

    An open handle to any object can be closed using CloseHandle.

    This is a generic function and operates on the following object
    types:

        - Process Object

        - Thread Object

        - Mutex Object

        - Event Object

        - Semaphore Object

        - File Object

    Please note that Module Objects are not in this list.

    Closing an open handle to an object causes the handle to become
    invalid and the HandleCount of the associated object to be
    decremented and object retention checks to be performed.  Once the
    last open handle to an object is closed, the object is removed from
    the system.

Arguments:

    hObject - An open handle to an object.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) ) {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
        }
    if (CONSOLE_HANDLE(hObject)) {
        return CloseConsoleHandle(hObject);
        }

    Status = NtClose(hObject);
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }
}


BOOL
DuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    )

/*++

Routine Description:

    A duplicate handle can be created with the DuplicateHandle function.

    This is a generic function and operates on the following object
    types:

        - Process Object

        - Thread Object

        - Mutex Object

        - Event Object

        - Semaphore Object

        - File Object

    Please note that Module Objects are not in this list.

    This function requires PROCESS_DUP_ACCESS to both the
    SourceProcessHandle and the TargetProcessHandle.  This function is
    used to pass an object handle from one process to another.  Once
    this call is complete, the target process needs to be informed of
    the value of the target handle.  The target process can then operate
    on the object using this handle value.

Arguments:

    hSourceProcessHandle - An open handle to the process that contains the
        handle to be duplicated. The handle must have been created with
        PROCESS_DUP_HANDLE access to the process.

    hSourceHandle - An open handle to any object that is valid in the
        context of the source process.

    hTargetProcessHandle - An open handle to the process that is to
        receive the duplicated handle.  The handle must have been
        created with PROCESS_DUP_HANDLE access to the process.

    lpTargetHandle - A pointer to a variable which receives the new handle
        that points to the same object as SourceHandle does.  This
        handle value is valid in the context of the target process.

    dwDesiredAccess - The access requested to for the new handle.  This
        parameter is ignored if the DUPLICATE_SAME_ACCESS option is
        specified.

    bInheritHandle - Supplies a flag that if TRUE, marks the target
        handle as inheritable.  If this is the case, then the target
        handle will be inherited to new processes each time the target
        process creates a new process using CreateProcess.

    dwOptions - Specifies optional behaviors for the caller.

        Options Flags:

        DUPLICATE_CLOSE_SOURCE - The SourceHandle will be closed by
            this service prior to returning to the caller.  This occurs
            regardless of any error status returned.

        DUPLICATE_SAME_ACCESS - The DesiredAccess parameter is ignored
            and instead the GrantedAccess associated with SourceHandle
            is used as the DesiredAccess when creating the TargetHandle.

Return Value:

    TRUE - The operation was successful.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    NTSTATUS Status;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hSourceHandle) ) {
        case STD_INPUT_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hSourceHandle = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hSourceHandle = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hSourceHandle) &&
	(hSourceHandle != NtCurrentProcess() &&
	 hSourceHandle != NtCurrentThread()) ) {
        HANDLE Target;

        if (hSourceProcessHandle != NtCurrentProcess() ||
            hTargetProcessHandle != NtCurrentProcess()) {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
            }
        Target = DuplicateConsoleHandle(hSourceHandle,
                                        dwDesiredAccess,
                                        bInheritHandle,
                                        dwOptions
                                       );
        if (Target == INVALID_HANDLE_VALUE) {
            return FALSE;
            }
        else {
            try {
                if ( ARGUMENT_PRESENT(lpTargetHandle) ) {
                    *lpTargetHandle = Target;
                    }
                }
            except (EXCEPTION_EXECUTE_HANDLER) {
                return TRUE;
                }
            return TRUE;
            }
        }

    Status = NtDuplicateObject(
                hSourceProcessHandle,
                hSourceHandle,
                hTargetProcessHandle,
                lpTargetHandle,
                (ACCESS_MASK)dwDesiredAccess,
                bInheritHandle ? OBJ_INHERIT : 0,
                dwOptions
                );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
        }
    else {
        BaseSetLastNTError(Status);
        return FALSE;
        }

    return FALSE;
}


BOOL
GetHandleInformation(
    HANDLE hObject,
    LPDWORD lpdwFlags
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    DWORD Result;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) ) {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hObject)) {
        return GetConsoleHandleInformation(hObject,
                                           lpdwFlags
                                          );
        }

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        Result = 0;
        if (HandleInfo.Inherit) {
            Result |= HANDLE_FLAG_INHERIT;
            }

        if (HandleInfo.ProtectFromClose) {
            Result |= HANDLE_FLAG_PROTECT_FROM_CLOSE;
            }

        *lpdwFlags = Result;
        return TRUE;
        }
    else {
        BaseSetLastNTError( Status );
        return FALSE;
        }
}


BOOL
SetHandleInformation(
    HANDLE hObject,
    DWORD dwMask,
    DWORD dwFlags
    )
{
    NTSTATUS Status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    PPEB Peb;

    Peb = NtCurrentPeb();
    switch( HandleToUlong(hObject) ) {
        case STD_INPUT_HANDLE:  hObject = Peb->ProcessParameters->StandardInput;
                                break;
        case STD_OUTPUT_HANDLE: hObject = Peb->ProcessParameters->StandardOutput;
                                break;
        case STD_ERROR_HANDLE:  hObject = Peb->ProcessParameters->StandardError;
                                break;
        }

    if (CONSOLE_HANDLE(hObject)) {
        return SetConsoleHandleInformation(hObject,
                                           dwMask,
                                           dwFlags
                                          );
        }

    Status = NtQueryObject( hObject,
                            ObjectHandleFlagInformation,
                            &HandleInfo,
                            sizeof( HandleInfo ),
                            NULL
                          );
    if (NT_SUCCESS( Status )) {
        if (dwMask & HANDLE_FLAG_INHERIT) {
            HandleInfo.Inherit = (dwFlags & HANDLE_FLAG_INHERIT) ? TRUE : FALSE;
            }

        if (dwMask & HANDLE_FLAG_PROTECT_FROM_CLOSE) {
            HandleInfo.ProtectFromClose = (dwFlags & HANDLE_FLAG_PROTECT_FROM_CLOSE) ? TRUE : FALSE;
            }

        Status = NtSetInformationObject( hObject,
                                         ObjectHandleFlagInformation,
                                         &HandleInfo,
                                         sizeof( HandleInfo )
                                       );
        if (NT_SUCCESS( Status )) {
            return TRUE;
            }
        }

    BaseSetLastNTError( Status );
    return FALSE;
}
