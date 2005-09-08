/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         User-mode APC support
 * FILE:            lib/rtl/apc.c
 * PROGRAMER:       David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

VOID STDCALL
KiUserApcDispatcher(PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK Iosb,
		    ULONG Reserved,
		    PCONTEXT Context)
{
   /*
    * Call the APC
    */
   //DPRINT1("ITS ME\n");
   ApcRoutine(ApcContext,
	      Iosb,
	      Reserved);
   /*
    * Switch back to the interrupted context
    */
    //DPRINT1("switch back\n");
   NtContinue(Context, 1);
}

