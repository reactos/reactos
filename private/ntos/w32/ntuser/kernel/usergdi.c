/****************************** Module Header ******************************\
* Module Name: timers.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains GDI-callable exports from user.  No user code
* should call any of these routines.
*
* History:
* 3-Jun-1998 AndrewGo   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
*  UserSetTimer()
*
* GDI-callable routine to enable a system timer on the RIT.
*
*  6/2/98 AndrewGo  Created
\***************************************************************************/

UINT_PTR UserSetTimer(UINT dwElapse, PVOID pTimerFunc)
{
    UINT_PTR id = 0;
    PTIMER   ptmr;

    /*
     * GDI may call during ChangeDisplaySettings, in which case the
     * critical section will already be held.  GDI may also call during
     * CreateDC("Device"), in which case the critical section will not
     * already be held.
     */
    BEGIN_REENTERCRIT();

    /*
     * If the RIT hasn't been started yet, let GDI know this by returning
     * failure.  Once we've initialized the RIT, we'll let GDI know
     * that GDI can start its timers by calling GreStartTimers().
     */
    if (gptmrMaster) {
    
        id = InternalSetTimer(NULL, 0, dwElapse, (TIMERPROC_PWND) pTimerFunc, TMRF_RIT);

        /*
         * We don't want cleanup to be done on thread termination.  Rather
         * than creating a new flag and adding more code to InternalSetTimer,
         * we disable cleanup by modifying the timer directly.
         */
        if (id) {

            ptmr = FindTimer(NULL, id, TMRF_RIT, FALSE);

            UserAssert(ptmr);

            ptmr->ptiOptCreator = NULL;
        }
    }

    END_REENTERCRIT();

    return id;
}

/***************************************************************************\
*  UserKillTimer()
*
*  6/2/98 AndrewGo  Created
\***************************************************************************/

VOID UserKillTimer(UINT_PTR nID)
{
    /*
     * GDI may call during ChangeDisplaySettings, in which case the
     * critical section will already be held.  GDI may also call any
     * time its PDEV reference counts go to zero, in which case the 
     * critical section will not already be held.
     */
    BEGIN_REENTERCRIT();

    KILLRITTIMER(NULL, nID);

    END_REENTERCRIT();
}
