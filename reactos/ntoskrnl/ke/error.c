/*
 * COPYRIGHT:  See COPYING in the top directory
 * PROJECT:    ReactOS kernel v0.0.2
 * FILE:       kernel/error.cc
 * PURPOSE:    Error reason setting/getting
 * PROGRAMMER: David Welch
 * UPDATE HISTORY:
 *             16/4/98: Created
 */

/* INCLUDE *****************************************************************/

#include <windows.h>

/* GLOBALS *****************************************************************/

/*
 * Last error code (this should be per process)
 */
DWORD error_code = 0;

/* FUNCTIONS ***************************************************************/

DWORD STDCALL GetLastError(VOID)
/*
 * FUNCTION: Get the detailed error (if any) from the last function
 * RECEIVES: Nothing
 * RETURNS:
 *           The error code
 */
{
        return(error_code);
}


VOID STDCALL SetLastError(DWORD dwErrCode)
/*
 * FUNCTION: Set the last error code
 * RECEIVES:
 *           dwErrCode = the error code to set
 * RETURNS: Nothing
 */
{
        error_code=dwErrCode;
}

