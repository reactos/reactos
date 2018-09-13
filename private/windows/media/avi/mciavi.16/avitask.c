/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   avitask.c - Background task that actually manipulates AVI files.

*****************************************************************************/
#include "graphic.h"

void FAR PASCAL DebugBreak(void);

BOOL FAR PASCAL mciaviCloseFile(NPMCIGRAPHIC npMCI);
BOOL FAR PASCAL mciaviOpenFile(NPMCIGRAPHIC npMCI);

#if defined(WIN32) || !defined(DEBUG)
#define StackTop()      (void *)0
#define StackMin()      (void *)0
#define StackBot()      (void *)0
#define StackMark()
#define StackTest()     TRUE
#else
#define STACK           _based(_segname("_STACK"))
#define StackTop()      *((UINT STACK *)10)
#define StackMin()      *((UINT STACK *)12)
#define StackBot()      *((UINT STACK *)14)
#define StackMark()     *((UINT STACK*)StackBot()) = 42
#define StackTest()     *((UINT STACK*)StackBot()) == 42
#endif


/***************************************************************************
 ***************************************************************************/

#ifndef WIN32
#pragma optimize("", off)
void FAR SetPSP(UINT psp)
{
    _asm {
        mov bx,psp
        mov ah,50h
        int 21h
    }
}
#pragma optimize("", on)
#endif

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | mciaviTask |  This function is the background task which plays
 *      AVI files. It is called as a result of the call to mmTaskCreate()
 *      in DeviceOpen(). When this function returns, the task is destroyed.
 *
 * @parm DWORD | dwInst | instance data passed to mmCreateTask - contains
 *      a pointer to an instance data block.
 *
 ***************************************************************************/

void FAR PASCAL _LOADDS mciaviTask(DWORD dwInst)
{
    NPMCIGRAPHIC npMCI;

    npMCI = (NPMCIGRAPHIC) dwInst;

    // Set this task's error mode to the same as the parent's.
    SetErrorMode(npMCI->uErrorMode);

    DPF2(("MCIAVI: Bkgd Task hTask=%04X\n", GetCurrentTask()));
    DPF2(("MCIAVI: Stack: %04X %04X %04X\n", StackTop(), StackMin(), StackBot()));

    /* Task state is TASKBEINGCREATED at fn. entry, then goes to TASKINIT. */

    Assert(npMCI && npMCI->wTaskState == TASKBEINGCREATED);

    npMCI->wTaskState = TASKINIT;

#ifndef WIN32
    //
    // in order to make this task, more like a "thread" we want to use the
    // same PSP as our parent, so we can share file handles and things.
    //
    // when we get created hTask is a PSP
    //
    npMCI->pspTask = GetCurrentPDB();   // save our PSP
#endif

    npMCI->hTask = GetCurrentTask();
    npMCI->dwTaskError = 0;

    /* Open the file  */

    if (!mciaviOpenFile(npMCI)) {
        // NOTE: IsTask() returns FALSE when hTask==0
        // Set hTask to 0 BEFORE setting wTaskState.  Our creator is polling
        // the state of wTaskState...
        // npMCI->wTaskState = TASKABORT;
        // npMCI->hTask = 0; // This stops others using this task thread.
        DPF1(("Failed to open AVI file\n"));
        goto exit;
    }

    while (IsTask(npMCI->hTask)) {

        npMCI->wTaskState = TASKIDLE;
        DPF2(("MCIAVI: Idle\n"));
        DPF2(("MCIAVI: Stack: %04X %04X %04X\n", StackTop(), StackMin(), StackBot()));

        StackMark();

        /* Block until task is needed. The task count could */
        /* be anything at the exit of playfile or recordfile */
        /* so continue to block until the state really changes. */

        while (npMCI->wTaskState == TASKIDLE)
        {
            mmTaskBlock(npMCI->hTask);
        }

        mciaviMessage(npMCI, npMCI->wTaskState);

        AssertSz(StackTest(), "Stack overflow");

        if (npMCI->wTaskState == TASKCLOSE) {
            break;
        }

    }
exit:
    mciaviTaskCleanup(npMCI);
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api WORD | mciaviTaskCleanup |  called when the background task
 *      is being destroyed.  This is where critical cleanup goes.
 *
 ***************************************************************************/

void FAR PASCAL mciaviTaskCleanup(NPMCIGRAPHIC npMCI)
{

#ifndef WIN32
    //
    // restore our PSP back to normal before exit.
    //
    if (npMCI->pspTask)
    {
        SetPSP(npMCI->pspTask);
    }
#endif

#ifdef USEAVIFILE
    //
    // we must do this so COMPOBJ will shut down right.
    //
    FreeAVIFile(npMCI);
#endif

    //
    //  call a MSVideo shutdown routine.
    //

    //
    //  Signal the foreground task that we're all done.
    //  This must be absolutely the last thing we do.
    //
    npMCI->hTask = 0;
    npMCI->wTaskState = TASKCLOSED;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void | mciaviMessage | this function handles a message from the
 *      background task.
 *
 ***************************************************************************/

void NEAR PASCAL mciaviMessage(NPMCIGRAPHIC npMCI, UINT msg)
{
    UINT  wNotification;

    switch (msg) {

    case TASKREADINDEX:
        Assert(0);
        break;

    /* Check to see if we just got closed */

    case TASKCLOSE:
        DPF1(("MCIAVI: Closing\n"));

	// hold critsec during close in case someone comes in to
	// eg DeviceRealize during the close when things are half-deleted.
	EnterCrit(npMCI);
        mciaviCloseFile(npMCI);
	LeaveCrit(npMCI);

        /* The htask must be set to NULL, otherwise CloseDevice() will  */
        /* get stuck. */

        // NOTE: IsTask() returns FALSE when hTask==0
        // npMCI->hTask = 0;
        // npMCI->wTaskState = TASKABORT;
        return;

    case TASKRELOAD:
 	DPF(("MCIAVI: Loading new file....\n"));
 	mciaviCloseFile(npMCI);
 	npMCI->dwTaskError = 0;
	npMCI->wTaskState = TASKINIT;

 	if (!mciaviOpenFile(npMCI)) {
	    // !!! mciaviOpenNew() !!!!!!!!!!!!!!!!!!!!!
	    npMCI->wTaskState = TASKCLOSE;
	    // npMCI->hTask = 0;
	    return;
	}
	break;

	// We've been woken up to play....
    case TASKSTARTING:
        DPF2(("MCIAVI: Now busy\n"));

        /* Reset to no error */
        npMCI->dwTaskError = 0;

        wNotification = mciaviPlayFile(npMCI);

        if ((wNotification != MCI_NOTIFY_FAILURE) ||
                ((npMCI->dwFlags & MCIAVI_WAITING) == 0))
            GraphicDelayedNotify(npMCI, wNotification);

        break;

    default:
        DPF(("MCIAVI: Unknown task state!!!! (%d)\n", msg));
        break;
    }
}

#ifdef WIN32

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api int | GetPrioritySeparation | Find the foreground process priority
 *    boost
 *
 * @rdesc Returns 0, 1 or 2
 *
 ***************************************************************************/

 DWORD GetPrioritySeparation(void)
 {
     static DWORD Win32PrioritySeparation = 0xFFFFFFFF;

     /* If we're not initialized get the current separation */

     if (Win32PrioritySeparation == 0xFFFFFFFF) {
         HKEY hKey;
         Win32PrioritySeparation = 2;  // This is the default

         /* Code copied from shell\control\main\prictl.c */

         if (RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 TEXT("SYSTEM\\CurrentControlSet\\Control\\PriorityControl"),
                 0,
                 KEY_QUERY_VALUE,
                 &hKey) == ERROR_SUCCESS) {

             DWORD Type;
             DWORD Length;

             Length = sizeof(Win32PrioritySeparation);

             /* Read the value which is the priority boost given to
                forground processes */

             if (RegQueryValueEx(
                      hKey,
                      TEXT("Win32PrioritySeparation"),
                      NULL,
                      &Type,
                      (LPBYTE)&Win32PrioritySeparation,
                      &Length
                      ) != ERROR_SUCCESS) {

                  Win32PrioritySeparation = 2;
             }

             RegCloseKey(hKey);
         }
     }

     return Win32PrioritySeparation;
 }
 #endif // WIN32

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api void| aviTaskYield |  This function yields in the picky way windows
 *      wants us to.
 *
 *      basicly we Dispatch any messages in our que that belong to a window.
 *
 *      NOTE we should not remove
 *
 ***************************************************************************/

void NEAR PASCAL aviTaskYield(void)
{
    MSG msg;

#ifdef WIN32
    DWORD PrioritySeparation;

    //
    //  Do our own kind of 'yield'.  The reason for doing the
    //  Peekmessage on Windows 3.1 was that if you didn't call
    //  it Windows would think you were spinning out of control.
    //  For Windows NT if you call PeekMessage 100 times without
    //  getting anything your priority is lowered which would mess
    //  up our tinkering with the priority here.
    //

    PrioritySeparation = GetPrioritySeparation();

    if (PrioritySeparation != 0) {
        SetThreadPriority(GetCurrentThread(),
                          PrioritySeparation == 1 ?
                              THREAD_PRIORITY_BELOW_NORMAL :  // minus 1
                              THREAD_PRIORITY_LOWEST);        // minus 2
        Sleep(0);    // Causes reschedule decision
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    } else {
        Sleep(0);    // Let other threads in
    }

#else

    //
    // if we were MCIWAVE we would do this....
    //
    //if (PeekMessage(&msg, NULL, 0, WM_MM_RESERVED_FIRST-1, PM_REMOVE))

    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        DPF(("aviTaskYield: got message %04X to window %04X\n", msg.message, msg.hwnd));
        DispatchMessage(&msg);
    }
#endif // WIN32
}

