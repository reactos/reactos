/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    nlvars.c

Abstract:

   This module contains variables used within the msv1_0 authentication
   package.

Author:

    Cliff Van Dyke (CliffV) 29-Apr-1991

Environment:

    User mode - msv1_0 authentication package DLL

Revision History:
  Chandana Surlu         21-Jul-96      Stolen from \\kernel\razzle3\src\security\msv1_0\nlvars.c

--*/

#include "msp.h"
#include "nlp.h"



////////////////////////////////////////////////////////////////////////
//                                                                    //
//                   READ ONLY  Variables                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////


//
// Null copies of Lanman and NT OWF password.
//

LM_OWF_PASSWORD NlpNullLmOwfPassword;
NT_OWF_PASSWORD NlpNullNtOwfPassword;



////////////////////////////////////////////////////////////////////////
//                                                                    //
//                   READ/WRITE Variables                             //
//                                                                    //
////////////////////////////////////////////////////////////////////////

//
// Define the list of active interactive logons.
//
// The NlpActiveLogonLock must be locked while referencing the list or
// any of its elements.
//

RTL_CRITICAL_SECTION NlpActiveLogonLock;
PACTIVE_LOGON NlpActiveLogons;

//
// Define the running enumeration handle.
//
// This variable defines the enumeration handle to assign to a logon
//  session.  It will be incremented prior to assigning it value to
//  the next created logon session.  Access is serialize using
//  NlpActiveLogonLocks.

ULONG NlpEnumerationHandle;


//
// Define the number of successful/unsuccessful logons attempts.
//

ULONG NlpLogonAttemptCount;
