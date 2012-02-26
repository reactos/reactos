/* $Id$
 *
 * subsystems/win32/csrss/csrsrv/api/user.c
 *
 * User functions
 *
 * ReactOS Operating System
 *
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static BOOLEAN ServicesProcessIdValid = FALSE;
static ULONG_PTR ServicesProcessId;


/* FUNCTIONS *****************************************************************/

CSR_API(CsrRegisterServicesProcess)
{
  if (ServicesProcessIdValid == TRUE)
    {
      /* Only accept a single call */
      return STATUS_INVALID_PARAMETER;
    }
  else
    {
      ServicesProcessId = (ULONG_PTR)Request->Data.RegisterServicesProcessRequest.ProcessId;
      ServicesProcessIdValid = TRUE;
      return STATUS_SUCCESS;
    }
}

/* EOF */
