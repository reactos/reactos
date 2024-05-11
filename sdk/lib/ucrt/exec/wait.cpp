/***
*wait.c - wait for child process to terminate
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wait() - wait for child process to terminate
*
*******************************************************************************/

#include <corecrt_internal.h>
#include <errno.h>
#include <process.h>
#include <stdlib.h>

/***
*int _cwait(stat_loc, process_id, action_code) - wait for specific child
*       process
*
*Purpose:
*       The function _cwait() suspends the calling-process until the specified
*       child-process terminates.  If the specifed child-process terminated
*       prior to the call to _cwait(), return is immediate.
*
*Entry:
*       int *stat_loc - pointer to where status is stored or nullptr
*       process_id - specific process id to be interrogated (0 means any)
*       action_code - specific action to perform on process ID
*                   either _WAIT_CHILD or _WAIT_GRANDCHILD
*
*Exit:
*       process ID of terminated child or -1 on error
*
*       *stat_loc is updated to contain the following:
*       Normal termination: lo-byte = 0, hi-byte = child exit code
*       Abnormal termination: lo-byte = term status, hi-byte = 0
*
*Exceptions:
*
*******************************************************************************/

extern "C" intptr_t __cdecl _cwait(
    int*     const exit_code_result,
    intptr_t const process_id,
    int      const action_code
    )
{
    DBG_UNREFERENCED_PARAMETER(action_code);
    
    if (exit_code_result)
        *exit_code_result = static_cast<DWORD>(-1);

    // Explicitly check for process ids -1 and -2.  In Windows NT, -1 is a handle
    // to the current process and -2 is a handle to the current thread, and the
    // OS will let you wait (forever) on either.
    _VALIDATE_RETURN_NOEXC(process_id != -1 && process_id != -2, ECHILD, -1);

    __crt_unique_handle process_handle(reinterpret_cast<HANDLE>(process_id));

    // Wait for the child process and get its exit code:
    DWORD exit_code;
    if (WaitForSingleObject(process_handle.get(), static_cast<DWORD>(-1)) == 0 &&
        GetExitCodeProcess(process_handle.get(), &exit_code))
    {
        if (exit_code_result)
            *exit_code_result = exit_code;

        return process_id;
    }

    // One of the API calls failed; map the error and return failure.
    DWORD const os_error = GetLastError();
    if (os_error == ERROR_INVALID_HANDLE)
    {
        errno = ECHILD;
        _doserrno = os_error;
    }
    else
    {
        __acrt_errno_map_os_error(os_error);
    }

    if (exit_code_result)
        *exit_code_result = static_cast<DWORD>(-1);

    return -1;
}
