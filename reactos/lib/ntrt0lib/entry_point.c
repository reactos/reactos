/* $Id$
 * 
 * ReactOS Operating System
 */
#include "ntrt0.h"

#define NDEBUG
#include <debug.h>

#ifdef NDEBUG
#define NTRT_DEBUG_FLAG 0
#else
#define NTRT_DEBUG_FLAG 1
#endif

int argc = 0;
char * argv [32] = {NULL};
char * envp = NULL;

/* Native process' entry point */

VOID STDCALL NtProcessStartup (PPEB Peb)
{
  NTSTATUS  Status = STATUS_SUCCESS;

  /*
   *	Parse the command line.
   */
  Status = NtRtParseCommandLine (Peb, & argc, argv);
  if (STATUS_SUCCESS != Status)
  {
    DPRINT1("NT: %s: NtRtParseCommandLine failed (Status=0x%08lx)\n",
	__FUNCTION__, Status);
  }
  /*
   * Call the user main
   */
  (void) _main (argc, argv, & envp, NTRT_DEBUG_FLAG);
}
/* EOF */
