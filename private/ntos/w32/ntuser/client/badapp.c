/****************************** Module Header ******************************\
* Module Name: badapp.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL  G_bFirstCount = TRUE;
BADAPP G_UserBadApp;

//#define BADAPPTRACE
///////////////////////////////////////////
//note that the bad app fix is made up of changes to the following files
//windows\user\client - client.c cltxt.h csend.c clinit.c globals.h
//            \inc    - user.h cf.h cf1.h
//            \server - server.c input.c queue.c focusact.c
//       \gdi\inc     - csrgdi.h
//windows\base\client\citrix - compatfl.c compatfl.h
//mvdm\wow32          - wkman.c
//sdk\inc             - ntpsapi.h
//
//the mechanism was moved from the server to client and changed to a sleep
//    see server.c for the registry stuff which is in control\\citrix
//    the pti->cSpins and teb->user32reserved0 fields were changed
//    and tif_spinning and pif_forcebackgroundpriority and cspinbackground
//       were removed to more easily detect a sync problem with new source
//Values can be stored in the registry for each app (under
//    machine\software\citrix\compatibility\applications\xxx (where xxx is the
//    application name).  For win32 apps, when user32.dll is loaded it
//    initializes the values in the G_UserBadApp structure, and for Win16 apps
//    the values are stored in the Teb by wow32 when the app is loaded.  If
//    neither of these values are filled in, this routine will use the default
//    system values.


/**************************************************************
* CtxBadAppDelay
*
* If counttype=0 then call is due to peek.  else was a yield
* we don't use the difference right now
* the delaycount is determined based on G_bFirstCount
* If count > delaycount
* then reset the count
* Then sleep for BadAppTimeDelay milliseconds
*
* G_bFirstCount reset to TRUE if get message from PeekMessage or GetMessage
* or in this routine if pti->cSpins == 0
*
* Returns the amount of milliseconds delayed
*/
VOID CtxBadAppDelay(ULONG counttype)
{
   ULONG delaycount;
   PCLIENTINFO pci;
   PTEB pTeb;
   LARGE_INTEGER BadAppDelay;

   pci = GetClientInfo();
   pTeb = NtCurrentTeb();
   if ( (pci != NULL) && ((pci->CitrixcSpins) == 0) ) {
      G_bFirstCount = TRUE;
   }

   if (G_UserBadApp.BadAppFlags & CITRIX_COMPAT_BADAPPVALID) {
       if (G_bFirstCount) {
          delaycount = G_UserBadApp.BadAppFirstCount;
       } else {
          delaycount = G_UserBadApp.BadAppNthCount;
       }
       BadAppDelay = G_UserBadApp.BadAppTimeDelay;
   } else if (pTeb->CtxCompatFlags & CITRIX_COMPAT_BADAPPVALID) {
       if (G_bFirstCount) {
          delaycount = pTeb->BadAppFirstCount;
       } else {
          delaycount = pTeb->BadAppNthCount;
       }
       BadAppDelay = pTeb->BadAppTimeDelay;
   } else {
       if (G_bFirstCount) {
          delaycount = gpsi->BadAppFirstCount;
       }
       else delaycount = gpsi->BadAppNthCount;
       BadAppDelay = gpsi->BadAppTimeDelay;
   }

   if ( (pTeb->CitrixTEBcSpins) > delaycount ) {
      (pTeb->CitrixTEBcSpins) = 0;

#ifdef BADAPPTRACE
      if (counttype) {
         DbgPrint("j.k. CtxBadAppDelay YIELD, bFirstCount=%u\n",(ULONG) bFirstCount);
      }
      else DbgPrint("j.k. CtxBadAppDelay PEEK, bFirstCount=%u\n",(ULONG) bFirstCount);
#endif

      G_bFirstCount = FALSE;
      NtDelayExecution(
         FALSE,
         &BadAppDelay);
   }
   return;
UNREFERENCED_PARAMETER(counttype);
}

/*
 * This is the Citrix version of NtUserYieldTask() that implements the Bad App processing
 *
 */
BOOL
CtxUserYieldTask( VOID )
{
    ULONG cSpins;
    PCLIENTINFO pci;
    BOOL Result;

    //every yield is bad news
    //server side increments so if not reset should move our count
    pci = GetClientInfo();
    if (pci != NULL) {
          cSpins = pci->CitrixcSpins;
    }

    Result = NtUserYieldTask();

    if ( pci != NULL) {
        if (cSpins >= pci->CitrixcSpins) {
        //in this case the server side reset cspins so we reset our count
            (NtCurrentTeb()->CitrixTEBcSpins) = pci->CitrixcSpins;
        }
        else {
            (NtCurrentTeb()->CitrixTEBcSpins) +=  pci->CitrixcSpins - cSpins;
            CtxBadAppDelay(1);   //show yield.
        }
    }

    return( Result );
}

/*
 * This is the Citrix version of NtUserWaitMessage() that implements the Bad App processing
 *
 */
BOOL
CtxUserWaitMessage( VOID )
{
    //since waiting, we can reset to firstcount
    G_bFirstCount = TRUE;

    return ( NtUserWaitMessage() );
}
