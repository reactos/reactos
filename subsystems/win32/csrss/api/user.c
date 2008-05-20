/* $Id$
 *
 * reactos/subsys/csrss/api/user.c
 *
 * User functions
 *
 * ReactOS Operating System
 *
 * PROGRAMMER: Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <csrss.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static BOOLEAN ServicesProcessIdValid = FALSE;
static ULONG ServicesProcessId;


/* FUNCTIONS *****************************************************************/

CSR_API(CsrRegisterServicesProcess)
{
  NTSTATUS Status;

  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  if (ServicesProcessIdValid == TRUE)
    {
      /* Only accept a single call */
      Status = STATUS_INVALID_PARAMETER;
    }
  else
    {
      ServicesProcessId = (ULONG)Request->Data.RegisterServicesProcessRequest.ProcessId;
      ServicesProcessIdValid = TRUE;
      Status = STATUS_SUCCESS;
    }

  Request->Status = Status;

  return(Status);
}

/* EOF */
