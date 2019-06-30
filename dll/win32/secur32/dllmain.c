/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/secur32/lsa.c
 * PURPOSE:         Client-side LSA functions
 * UPDATE HISTORY:
 *                  Created 05/08/00
 */

/* INCLUDES ******************************************************************/
#include <precomp.h>

void SECUR32_initializeProviders(void);
void SECUR32_freeProviders(void);

/* FUNCTIONS *****************************************************************/

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG Reason, PVOID Reserved)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        SECUR32_initializeProviders();
        break;
    case DLL_PROCESS_DETACH:
        SECUR32_freeProviders();
        break;
    }
   return(TRUE);
}
