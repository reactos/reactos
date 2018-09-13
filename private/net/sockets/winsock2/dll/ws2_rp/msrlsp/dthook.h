/*++

     Copyright c 1996 Intel Corporation
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

    dthook.h

  Abstract:

    Header file containing definitions, function prototypes, and other
    stuff for the Debug/Trace.

  Author:

    bugs@brandy.jf.intel.com

--*/

#ifndef DTHOOK_H
#define DTHOOK_H

// #include "warnoff.h"
#include <windows.h>
#include "dt_dll.h"


//
// Function Declarations
//

LPFNWSANOTIFY
GetPreApiNotifyFP(void);

LPFNWSANOTIFY
GetPostApiNotifyFP(void);

void
DTHookInitialize(void);

void
DTHookShutdown(void);



#ifdef DEBUG_TRACING

#define PREAPINOTIFY(x) \
    ( GetPreApiNotifyFP()  ? ( (*(GetPreApiNotifyFP())) x ) : FALSE)
#define POSTAPINOTIFY(x) \
    if ( GetPostApiNotifyFP() ) { \
         (VOID) ( (*(GetPostApiNotifyFP())) x ); \
    } else

#else

#define PREAPINOTIFY(x) FALSE
#define POSTAPINOTIFY(x)

#endif  // DEBUG_TRACING
#endif  // DTHOOK_H
