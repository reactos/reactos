/* $Id: user.c,v 1.2 2002/11/03 20:01:07 chorns Exp $
 *
 * reactos/subsys/csrss/api/user.c
 *
 * User functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>

#include <csrss/csrss.h>
#include "api.h"
#include <ntdll/rtl.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

static BOOLEAN ServicesProcessIdValid = FALSE;
static ULONG ServicesProcessId;


/* FUNCTIONS *****************************************************************/

CSR_API(CsrRegisterServicesProcess)
{
  NTSTATUS Status;

  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
    sizeof(LPC_MESSAGE);

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

  Reply->Status = Status;

  return(Status);
}


CSR_API(CsrExitReactos)
{
  Reply->Header.MessageSize = sizeof(CSRSS_API_REPLY);
  Reply->Header.DataSize = sizeof(CSRSS_API_REPLY) -
    sizeof(LPC_MESSAGE);



  Reply->Status = STATUS_NOT_IMPLEMENTED;

  return(STATUS_NOT_IMPLEMENTED);
}

/* EOF */
