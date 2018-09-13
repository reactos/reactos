/*++

     Copyright (c) 1996 Intel Corporation
     Copyright (c) 1996 Microsoft Corporation
     All Rights Reserved
     
     Permission is granted to use, copy and distribute this software and 
     its documentation for any purpose and without fee, provided, that 
     the above copyright notice and this statement appear in all copies. 
     Intel makes no representations about the suitability of this 
     software for any purpose.  This software is provided "AS IS."  
     
     Intel specifically disclaims all warranties, express or implied, 
     and all liability, including consequential and other indirect 
     damages, for the use of this software, including liability for 
     infringement of any proprietary rights, and including the 
     warranties of merchantability and fitness for a particular purpose. 
     Intel does not assume any responsibility for any errors which may 
     appear in this software nor any responsibility to update it.


  Module Name:

    dthook.cpp

  Abstract:

    This module contains the hooks that allow specially-compiled
    versions of layered provier DLL call into the debug/trace DLL.

  Author:

    bugs@brandy.jf.intel.com

--*/

#include "precomp.h"

//
// Static Globals
//

// Function pointers to the Debug/Trace DLL entry points
static LPFNWSANOTIFY PreApiNotifyFP = NULL;
static LPFNWSANOTIFY PostApiNotifyFP = NULL;

// Handle to the Debug/Trace DLL module
static HMODULE       DTDll = NULL;

// Static string to pass to Debug/Trace notification functions
static char LibName[] = "msrlsp";

//
// Functions
//


LPFNWSANOTIFY
GetPreApiNotifyFP(void)
/*++

  Function Description:

      Returns a pointer to the WSAPreApiNotify function exported by
      the Debug/Trace DLL.  This variable is global to this file only,
      and is initialized during DT_Initialize().

  Arguments:

      None.

  Return Value:

      Returns whatever is stored in PreApiNotifyFP.

--*/
{
    return(PreApiNotifyFP);
}





LPFNWSANOTIFY
GetPostApiNotifyFP(void)
/*++

  Function Description:

      Returns a pointer to the WSAPreApiNotify function exported by
      the Debug/Trace DLL.  This variable is global to this file only,
      and is initialized during DT_Initialize().

  Arguments:

      None.

  Return Value:

      Returns whatever is stored in PreApiNotifyFP.

--*/
{
    return(PostApiNotifyFP);
}





void
DTHookInitialize(void)
/*++

  Function Description:

      Intializes this hook module.  Loads the Debug/Trace DLL, if
      possible, and sets the global function pointers to point to the
      entry points exported by that DLL.  If the DLL can't be loaded,
      the function just returns and the function pointers are left at
      NULL.

      This function MUST be called before any of the hook functions
      are called, or the hook functions will not work.

  Arguments:

      None.

  Return Value:

      None.

--*/
{
    DTDll = (HMODULE)LoadLibrary("dt_dll");

    if (DTDll == NULL) {
        return;
    }

    PreApiNotifyFP = (LPFNWSANOTIFY)GetProcAddress(
        DTDll,
        "WSAPreApiNotify");
    
    PostApiNotifyFP = (LPFNWSANOTIFY)GetProcAddress(
        DTDll,
        "WSAPostApiNotify");
}





void
DTHookShutdown(void)
/*++

  Function Description:

      Should be called to shutdown Debug/Tracing.  The function
      pointers are set to NULL, and the DLL is unloaded from memory.

  Arguments:

      None.

  Return Value:

      None.

--*/
{
    if (DTDll != NULL) {
        FreeLibrary(DTDll);
    }

    PreApiNotifyFP = NULL;
    PostApiNotifyFP = NULL;
}
