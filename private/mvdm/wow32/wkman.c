/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WKMAN.C
 *  WOW32 16-bit Kernel API support (manually-coded thunks)
 *
 *  History:
 *  Created 27-Jan-1991 by Jeff Parsons (jeffpar)
 *  20-Apr-91 Matt Felton (mattfe) Added WK32CheckLoadModuleDrv
 *  28-Jan-92 Matt Felton (mattfe) Added Wk32GetNextVdmCommand + MIPS build
 *  10-Feb-92 Matt Felton (mattfe) Removed WK32CheckLoadModuleDRV
 *  10-Feb-92 Matt Felton (mattfe) cleanup and task creation
 *   4-mar-92 mattfe add killprocess
 *  11-mar-92 mattfe added W32NotifyThread
 *  12-mar-92 mattfe added WowRegisterShellWindowHandle
 *  17-apr-92 daveh changed to use host_CreateThread and host_ExitThread
 *  11-jun-92 mattfe hung app support W32HungAppNotifyThread, W32EndTask
 *
--*/

#include "precomp.h"
#pragma hdrstop
#include <ntexapi.h>
#include <sharewow.h>
#include <vdmdbg.h>
#include <ntseapi.h>
#include <wingdip.h>     // GACF_ app compat flags
#include "wowfax.h"
#include "demexp.h"

extern void UnloadNetworkFonts( UINT id );

MODNAME(wkman.c);

BOOL GetWOWShortCutInfo (PULONG Bufsize, PVOID Buf);
extern void FreeTaskFormFeedHacks(HAND16 h16);

// Global DATA

//
// The 5 variables below are used to hold STARTUPINFO fields between
// WowExec's GetNextVdmComand call and the InitTask call of the new
// app.  We pass them on to user32's InitTask.
//

DWORD   dwLastHotkey;
DWORD   dwLastX = (DWORD) CW_USEDEFAULT;
DWORD   dwLastY = (DWORD) CW_USEDEFAULT;
DWORD   dwLastXSize = (DWORD) CW_USEDEFAULT;
DWORD   dwLastYSize = (DWORD) CW_USEDEFAULT;

HWND    ghwndShell;           // WOWEXEC Window Handle
HANDLE  ghInstanceUser32;

HAND16  ghShellTDB;                 // WOWEXEC TDB
HANDLE  ghevWowExecMsgWait;
HANDLE  ghevWaitHungAppNotifyThread = (HANDLE)-1;  // Syncronize App Termination to Hung App NotifyThread
HANDLE  ghNotifyThread = (HANDLE)-1;        // Notification Thread Handle
HANDLE  ghHungAppNotifyThread = (HANDLE)-1; // HungAppNotification ThreadHandle
PTD gptdTaskHead;                   // Linked List of TDs
CRITICAL_SECTION gcsWOW;            // WOW Critical Section used when updating task linked list
CRITICAL_SECTION gcsHungApp;        // HungApp Critical Section used when VDM_WOWHUNGAPP bit

HMODCACHE ghModCache[CHMODCACHE];   // avoid callbacks to get 16-bit hMods

volatile HANDLE ghTaskCreation;     // hThread from task creation (see WK32Yield)
                                    // touched by parent and child threads during task init

VPVOID  vpnum_tasks;                // Pointer to KDATA variables (KDATA.ASM)
PWORD16 pCurTDB;                    // Pointer to KDATA variables
PWORD16 pCurDirOwner;               // Pointer to KDATA variables
VPVOID  vpDebugWOW = 0;             // Pointer to KDATA variables
VPVOID  vpLockTDB;                  // Pointer to KDATA variables
VPVOID  vptopPDB = 0;               // KRNL PDB
DOSWOWDATA DosWowData;              // structure that keeps linear pointer to
                                    // DOS internal variables.

//
// List of known DLLs used by WK32WowIsKnownDLL, called by 16-bit LoadModule.
// This causes known DLLs to be forced to load from the 32-bit system
// directory, since these are "special" binaries that should not be
// overwritten by unwitting 16-bit setup programs.
//
// This list is initialized from the registry value
// ...\CurrentControlSet\Control\WOW\KnownDLLs REG_SZ (space separated list)
//

#define MAX_KNOWN_DLLS 64
PSZ apszKnownDLL[MAX_KNOWN_DLLS];

//
// Fully-qualified path to %windir%\control.exe for PM5 setup fix.
// Setup by WK32InitWowIsKnownDll, used by WK32WowIsKnownDll.
//
CHAR szBackslashControlExe[] = "\\control.exe";
PSZ pszControlExeWinDirPath;          // "c:\winnt\control.exe"
PSZ pszControlExeSysDirPath;          // "c:\winnt\system32\control.exe"
CHAR szBackslashProgmanExe[] = "\\progman.exe";
PSZ pszProgmanExeWinDirPath;          // "c:\winnt\progman.exe"
PSZ pszProgmanExeSysDirPath;          // "c:\winnt\system32\progman.exe"

char szWOAWOW32[] = "-WoAWoW32";

//
// WOW GDI/CSR batching limit.
//

DWORD  dwWOWBatchLimit = 0;


UINT GetWOWTaskId(void);

#define TOOLONGLIMIT     _MAX_PATH
#define WARNINGMSGLENGTH 255

static char szCaption[TOOLONGLIMIT + WARNINGMSGLENGTH];
static char szMsgBoxText[TOOLONGLIMIT + WARNINGMSGLENGTH];

extern HANDLE hmodWOW32;


/* WK32WaitEvent - First API called by app, courtesy the C runtimes
 *
 * ENTRY
 *
 * EXIT
 *  Returns TRUE to indicate that a reschedule occurred
 *
 *
 */

ULONG FASTCALL WK32WaitEvent(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    return TRUE;
}


/* WK32KernelTrace - Trace 16Bit Kernel API Calls
 *
 * ENTRY
 *
 * EXIT
 *
 *
 */

ULONG FASTCALL WK32WOWKernelTrace(PVDMFRAME pFrame)
{
#ifdef DEBUG
PBYTE pb1;
PBYTE pb2;
register PWOWKERNELTRACE16 parg16;

 // Check Filtering - Trace Correct TaskID and Kernel Tracing Enabled

    if (((WORD)(pFrame->wTDB & fLogTaskFilter) == pFrame->wTDB) &&
        ((fLogFilter & FILTER_KERNEL16) != 0 )) {

        GETARGPTR(pFrame, sizeof(*parg16), parg16);
        GETVDMPTR(parg16->lpRoutineName, 50, pb1);
        GETVDMPTR(parg16->lpUserArgs, parg16->cParms, pb2);
        if ((fLogFilter & FILTER_VERBOSE) == 0 ) {
          LOGDEBUG(12, ("%s(", pb1));
        } else {
          LOGDEBUG(12, ("%04X %08X %04X %s:%s(",pFrame->wTDB, pb2, pFrame->wAppDS, (LPSZ)"Kernel16", pb1));
        }

        pb2 += 2*sizeof(WORD);              // point past callers CS:IP

        pb2 += parg16->cParms;

        while (parg16->cParms > 0) {
        pb2 -= sizeof(WORD);
        parg16->cParms -= sizeof(WORD);
        LOGDEBUG(12,( "%04x", *(PWORD)pb2));
        if (parg16->cParms > 0) {
            LOGDEBUG(12,( ","));
        }
    }

    LOGDEBUG(12,( ")\n"));
    if (fDebugWait != 0) {
        DbgPrint("WOWSingle Step\n");
        DbgBreakPoint();
    }

    FREEVDMPTR(pb1);
    FREEVDMPTR(pb2);
    FREEARGPTR(parg16);
 }
#else
    UNREFERENCED_PARAMETER(pFrame);
#endif
    return TRUE;
}


DWORD ParseHotkeyReserved(
    CHAR *pchReserved)
{
    ULONG dw;
    CHAR *pch;

    if (!pchReserved || !*pchReserved)
        return 0;

    dw = 0;

    if ((pch = WOW32_strstr(pchReserved, "hotkey")) != NULL) {
        pch += strlen("hotkey");
        pch++;
        dw = atoi(pch);
    }

    return dw;
}


/* WK32WowGetNextVdmCommand - Get Next App Name to Exec
 *
 *
 * Entry - lpReturnedString - Pointer to String Buffer
 *     nSize - Size of Buffer
 *
 * Exit
 *     SUCCESS
 *        if (!pWowInfo->CmdLineSize) {
 *            // no apps queued
 *        } else {
 *            Buffer Has Next App Name to Exec
 *            and new environment
 *        }
 *
 *     FAILURE
 *        Buffer Size too Small or Environment is too small
 *         pWowInfo->EnvSize - required size
 *         pWowInfo->CmdLineSize - required size
 *
 *
 */

// these two functions are imported from ntvdm.exe and housed in
// dos\\command\\cmdenv.c
//
extern VOID cmdCheckTempInit(VOID);
extern LPSTR cmdCheckTemp(LPSTR lpszzEnv);

ULONG FASTCALL WK32WowGetNextVdmCommand (PVDMFRAME pFrame)
{

    ULONG ul;
    PSZ pszEnv16, pszEnv, pszCurDir, pszCmd, pszAppName, pszEnv32, pszTemp;
    register PWOWGETNEXTVDMCOMMAND16 parg16;
    PWOWINFO pWowInfo;
    VDMINFO VDMInfo;
    PCHAR   pTemp;
    WORD    w;
    CHAR    szSiReservedBuf[128];

    GETARGPTR(pFrame, sizeof(WOWGETNEXTVDMCOMMAND16), parg16);
    GETVDMPTR(parg16->lpWowInfo, sizeof(WOWINFO), pWowInfo);
    GETVDMPTR(pWowInfo->lpCmdLine, pWowInfo->CmdLineSize, pszCmd);
    GETVDMPTR(pWowInfo->lpAppName, pWowInfo->AppNameSize, pszAppName);
    GETVDMPTR(pWowInfo->lpEnv, pWowInfo->EnvSize, pszEnv);
    GETVDMPTR(pWowInfo->lpCurDir, pWowInfo->CurDirSize, pszCurDir);

    pszEnv16 = pszEnv;

    // if we have a real environment pointer and size then
    // malloc a 32 bit buffer. Note that the 16 bit buffer should
    // be twice the size.

    VDMInfo.Enviornment = pszEnv;
    pszEnv32 = NULL;

    if (pWowInfo->EnvSize != 0) {
       if (pszEnv32 = malloc_w(pWowInfo->EnvSize)) {
            VDMInfo.Enviornment = pszEnv32;
       }
    }


SkipWowExec:

    VDMInfo.CmdLine = pszCmd;
    VDMInfo.CmdSize = pWowInfo->CmdLineSize;
    VDMInfo.AppName = pszAppName;
    VDMInfo.AppLen = pWowInfo->AppNameSize;
    VDMInfo.PifFile = NULL;
    VDMInfo.PifLen = 0;
    VDMInfo.CurDrive = 0;
    VDMInfo.EnviornmentSize = pWowInfo->EnvSize;
    VDMInfo.ErrorCode = TRUE;
    VDMInfo.VDMState =  fSeparateWow ? ASKING_FOR_SEPWOW_BINARY : ASKING_FOR_WOW_BINARY;
    VDMInfo.iTask = 0;
    VDMInfo.StdIn = 0;
    VDMInfo.StdOut = 0;
    VDMInfo.StdErr = 0;
    VDMInfo.CodePage = 0;
    VDMInfo.TitleLen = 0;
    VDMInfo.DesktopLen = 0;
    VDMInfo.CurDirectory = pszCurDir;
    VDMInfo.CurDirectoryLen = pWowInfo->CurDirSize;
    VDMInfo.Reserved = szSiReservedBuf;
    VDMInfo.ReservedLen = sizeof(szSiReservedBuf);

    ul = GetNextVDMCommand (&VDMInfo);

    if (ul) {

        //
        // BaseSrv will return TRUE with CmdSize == 0 if no more commands
        //
        if (VDMInfo.CmdSize == 0) {
            pWowInfo->CmdLineSize = 0;
            goto CleanUp;
        }

        //
        // If wowexec is the appname then we don't want to pass it back to
        // the existing instance of wowexec in a shared VDM since it will
        // basically do nothing but load and exit. Since it is not run we
        // need call ExitVDM to cleanup. Next we go back to look for more
        // commands.
        //
        if ((! fSeparateWow) && WOW32_strstr(VDMInfo.AppName, "wowexec.exe")) {
            ExitVDM(WOWVDM, VDMInfo.iTask);
            goto SkipWowExec;
        }

    }


    //
    // WOWEXEC will initially call with a guess of the correct environment
    // size. If he did not allocate enough then we will return the appropriate
    // size so that he can try again. WOWEXEC knows that we will require a
    // buffer twice the size specified. The environment can be up to 64k since
    // 16 bit LoadModule can only take a selector pointer to the environment.
    //

    if ( VDMInfo.EnviornmentSize > pWowInfo->EnvSize         ||
         VDMInfo.CmdSize > (USHORT)pWowInfo->CmdLineSize     ||
         VDMInfo.AppLen > (USHORT)pWowInfo->AppNameSize      ||
         VDMInfo.CurDirectoryLen > (ULONG)pWowInfo->CurDirSize )
       {

        // We return the size specified, but assume that WOWEXEC will double
        // it when allocating memory to allow for the string conversion/
        // expansion that might happen for international versions of NT.
        // See below where we uppercase and convert to OEM characters.

        w = 2*(WORD)VDMInfo.EnviornmentSize;
        if ( (DWORD)w == 2*(VDMInfo.EnviornmentSize) ) {
            // Fit in a Word!
            pWowInfo->EnvSize = (WORD)VDMInfo.EnviornmentSize;
        } else {
            // Make it the max size (see 16 bit globalrealloc)
            pWowInfo->EnvSize = (65536-17)/2;
        }

        // Pass back other correct sizes required
        pWowInfo->CmdLineSize = VDMInfo.CmdSize;
        pWowInfo->AppNameSize = VDMInfo.AppLen;
        pWowInfo->CurDirSize = (USHORT)VDMInfo.CurDirectoryLen;
        ul = FALSE;
    }

    if ( ul ) {

        //
        // Boost the hour glass
        //

        ShowStartGlass (10000);

        //
        // Save away wShowWindow, hotkey and startup window position from
        // the STARTUPINFO structure.  We'll pass them over to UserSrv during
        // the new app's InitTask call.  The assumption here is that this
        // will be the last GetNextVDMCommand call before the call to InitTask
        // by the newly-created task.
        //

        dwLastHotkey = ParseHotkeyReserved(VDMInfo.Reserved);

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USESHOWWINDOW) {
            pWowInfo->wShowWindow =
              (VDMInfo.StartupInfo.wShowWindow  == SW_SHOWDEFAULT)
              ? SW_SHOW : VDMInfo.StartupInfo.wShowWindow ;
        } else {
            pWowInfo->wShowWindow = SW_SHOW;
        }

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USEPOSITION) {
            dwLastX = VDMInfo.StartupInfo.dwX;
            dwLastY = VDMInfo.StartupInfo.dwY;
        } else {
            dwLastX = dwLastY = (DWORD) CW_USEDEFAULT;
        }

        if (VDMInfo.StartupInfo.dwFlags & STARTF_USESIZE) {
            dwLastXSize = VDMInfo.StartupInfo.dwXSize;
            dwLastYSize = VDMInfo.StartupInfo.dwYSize;
        } else {
            dwLastXSize = dwLastYSize = (DWORD) CW_USEDEFAULT;
        }

        LOGDEBUG(4, ("WK32WowGetNextVdmCommand: HotKey: %u\n"
                     "    Window Pos:  (%u,%u)\n"
                     "    Window Size: (%u,%u)\n",
                     dwLastHotkey, dwLastX, dwLastY, dwLastXSize, dwLastYSize));


        // 20-Jan-1994 sudeepb
        // Following callout is for inheriting the directories for the new
        // task. After this we mark the CDS's to be invalid which will force
        // new directories to be pickedup on need basis. See bug#1995 for
        // details.

        W32RefreshCurrentDirectories (pszEnv32);

        // Save iTask
        // When Server16 does the Exec Call we can put this Id into task
        // Structure.  When the WOW app dies we can notify Win32 using this
        // taskid so if any apps are waiting they will get notified.

        iW32ExecTaskId = VDMInfo.iTask;

        //
        // krnl expects ANSI strings!
        //

        OemToChar(pszCmd, pszCmd);
        OemToChar(pszAppName, pszAppName);

        //
        // So should the current directory be OEM or Ansi?
        //


        pWowInfo->iTask = VDMInfo.iTask;
        pWowInfo->CurDrive = VDMInfo.CurDrive;
        pWowInfo->EnvSize = (USHORT)VDMInfo.EnviornmentSize;


        // Uppercase the Environment KeyNames but leave the environment
        // variables in mixed case - to be compatible with MS-DOS
        // Also convert environment to OEM character set

        // another thing that we do here is a fixup to temp/tmp variables
        // which  occurs via the provided ntvdm functions


        if (pszEnv32) {

            cmdCheckTempInit();

            for (pszTemp = pszEnv32;*pszTemp;pszTemp += (strlen(pszTemp) + 1)) {

                PSZ pEnv;

                // The MS-DOS Environment is OEM

                if (NULL == (pEnv = cmdCheckTemp(pszTemp))) {
                   pEnv = pszTemp;
                }

                CharToOem(pEnv,pszEnv);

                // Ignore the NT specific Environment variables that start ==

                if (*pszEnv != '=') {
                    if (pTemp = WOW32_strchr(pszEnv,'=')) {
                        *pTemp = '\0';

                        // don't uppercase "windir" as it is lowercase for
                        // Win 3.1 and MS-DOS apps.

                       if (pTemp-pszEnv != 6 || WOW32_strncmp(pszEnv, "windir", 6))
                           WOW32_strupr(pszEnv);
                       *pTemp = '=';
                    }
                }
                pszEnv += (strlen(pszEnv) + 1);
            }

            // Environment is Double NULL terminated
            *pszEnv = '\0';
        }
    }

  CleanUp:
    if (pszEnv32) {
        free_w(pszEnv32);
    }

    FLUSHVDMPTR(parg16->lpWowInfo, sizeof(WOWINFO), pWowInfo);
    FLUSHVDMPTR((ULONG)pWowInfo->lpCmdLine, pWowInfo->CmdLineSize, pszCmd);

    FREEVDMPTR(pszCmd);
    FREEVDMPTR(pszEnv);
    FREEVDMPTR(pszCurDir);
    FREEVDMPTR(pWowInfo);
    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++

 WK32WOWInitTask - API Used to Create a New Task + Thread

 Routine Description:

    All the 16 bit initialization is completed, the app is loaded in memory and ready to go
    we come here to create a thread for this task.

    The current thread impersonates the new task, its running on the new tasks stack and it
    has its wTDB, this makes it easy for us to get a pointer to the new tasks stack and for it
    to have the correct 16 bit stack frame.   In order for the creator to continue correctly
    we set RET_TASKSTARTED on the stack.   Kernel16 will then not return to the new task
    but will know to restart the creator and put his thread ID and stack back.

    We ResetEvent so we can wait for the new thread to get going, this is important since
    we want the first YIELD call from the creator to yield to the newly created task.

    Special Case During Boot
    During the boot process the kernel will load the first app into memory on the main thread
    using the regular LoadModule.   We don't want the first app to start running until the kernel
    boot is completed so we can reuse the first thread.

 Arguments:
    pFrame - Points to the New Tasks Stack Frame

 Return Value:
    TRUE   - Successfully Created a Thread
    FALSE  - Failed to Create a New Task

--*/

ULONG FASTCALL WK32WOWInitTask(PVDMFRAME pFrame)
{
    VPVOID  vpStack;
    DWORD  dwThreadId;
    HANDLE hThread;

#if FASTBOPPING
    vpStack = FASTVDMSTACK();
#else
    vpStack = VDMSTACK();
#endif


    pFrame->wRetID = RET_TASKSTARTED;

       /*
        *  Suspend the timer thread on the startup of every task
        *  To allow resyncing of the dos time to the system time.
        *  When wowexec is the only task running the timer thread
        *  will remain suspended. When the new task actually intializes
        *  it will resume the timer thread, provided it is not wowexec.
        */
    if (nWOWTasks != 1)
        SuspendTimerThread();       // turns timer thread off

    if (fBoot) {
        W32Thread((LPVOID)vpStack);    // SHOULD NEVER RETURN

        WOW32ASSERTMSG(FALSE, "\nWOW32: WK32WOWInitTask ERROR - Main Thread Returning - Contact DaveHart\n");
        ExitVDM(WOWVDM, ALL_TASKS);
        ExitProcess(EXIT_FAILURE);
    }

    hThread = host_CreateThread(NULL,
                                8192,
                                W32Thread,
                                (LPVOID)vpStack,
                                CREATE_SUSPENDED,
                                &dwThreadId);

    ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_hThread = (DWORD) hThread;
    ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ThreadID = dwThreadId;

    if ( hThread ) {

        WOW32VERIFY(DuplicateHandle(
                        GetCurrentProcess(),
                        hThread,
                        GetCurrentProcess(),
                        (HANDLE *) &ghTaskCreation,
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS
                        ));

    }

#ifdef DEBUG
    {
        char szModName[9];

        RtlCopyMemory(szModName, ((PTDB)SEGPTR(pFrame->wTDB,0))->TDB_ModName, 8);
        szModName[8] = 0;

        LOGDEBUG( hThread ? LOG_IMPORTANT : LOG_ALWAYS,
            ("\nWK32WOWInitTask: %s task %04x %s\n",
                hThread ? "created" : "ERROR failed to create",
                pFrame->wTDB,
                szModName
            ));
    }
#endif

    return hThread ? TRUE : FALSE;
}


/*++
 WK32YIELD - Yield to the Next Task

 Routine Description:

    Normal Case - A 16 bit task is running and wants to give up the CPU to any higher priority
    task that might want to run.   Since we are running with a non-preemptive scheduler apps
    have to cooperate.

 ENTRY
  pFrame - Not used

 EXIT
  Nothing

--*/

ULONG FASTCALL WK32Yield(PVDMFRAME pFrame)
{
    //
    // WARNING: wkgthunk.c's WOWYield16 export (part of the Generic Thunk
    //      interface) calls this thunk with a NULL pFrame.  If you
    //          change this function to use pFrame change WOWYield16 as
    //          well.
    //

    UNREFERENCED_PARAMETER(pFrame);

    if (ghTaskCreation) {
        DWORD dw;
        HANDLE ThreadEvents[2];

        ThreadEvents[0] = ghevWaitCreatorThread;
        ThreadEvents[1] = ghTaskCreation;
        ghTaskCreation = NULL;
        WOW32VERIFY( ResumeThread(ThreadEvents[1]) != (DWORD)-1 );  // ghTaskCreation

        dw = WaitForMultipleObjects(2, ThreadEvents, FALSE, INFINITE);
        if (dw != WAIT_OBJECT_0) {
            WOW32ASSERTMSGF(FALSE,
                ("\nWK32Yield: ERROR WaitInitTask %d gle %d\n\n", dw, GetLastError())
                );
            ResetEvent(ghevWaitCreatorThread);
        }

        CloseHandle(ThreadEvents[1]);  // ghTaskCreation

        LOGDEBUG(2,("WK32Yield: Creator thread %04X now yielding\n", pFrame->wTDB));
    }


    BlockWOWIdle(TRUE);

    (pfnOut.pfnYieldTask)();

    BlockWOWIdle(FALSE);


    RETURN(0);
}




ULONG FASTCALL WK32OldYield(PVDMFRAME pFrame)
{

    UNREFERENCED_PARAMETER(pFrame);

    BlockWOWIdle(TRUE);

    (pfnOut.pfnDirectedYield)(DY_OLDYIELD);

    BlockWOWIdle(FALSE);


    RETURN(0);
}





/*++
 WK32ForegroundIdleHook - Supply WMU_FOREGROUNDIDLE message when system
                          (foreground "task") goes idle; support for int 2f

 Routine Description:

    This is the hook procedure for idle detection.  When the
    foregorund task goes idle, if the int 2f is hooked, then
    we will get control here and we call Wow16 to issue
    the int 2f:1689 to signal the idle condition to the hooker.

 ENTRY
    normal hook parameters: ignored

 EXIT
  Nothing

--*/

LRESULT CALLBACK WK32ForegroundIdleHook(int code, WPARAM wParam, LPARAM lParam)
{
    PARM16  Parm16;

    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    CallBack16(RET_FOREGROUNDIDLE, &Parm16, 0, 0);

    RETURN(0);
}


/*++
 WK32WowSetIdleHook - Set the hook so we will get notified when the
                   (foreground "task") goes idle; support for int 2f

 Routine Description:

    This sets the hook procedure for idle detection.  When the
    foregorund task goes idle, if the int 2f is hooked, then
    we will get control above and send a message to WOW so it can issue
    the int 2f:1689 to signal the idle condition to the hooker.

 ENTRY
    pFrame - not used

 EXIT
    The hook is set and it's handle is placed in to the per thread
    data ptd->hIdleHook.     0 is returned.  On
    failure, the hook is just not set (sorry), but a debug call is
    made.

--*/

ULONG FASTCALL WK32WowSetIdleHook(PVDMFRAME pFrame)
{
    PTD ptd;
    UNREFERENCED_PARAMETER(pFrame);

    ptd = CURRENTPTD();

    if (ptd->hIdleHook == NULL) {

        // If there is no hook already set then set a GlobaHook
        // It is important to set a GlobalHook otherwise we will not
        // Get accurate timing results with a LocalHook.

        ptd->hIdleHook = SetWindowsHookEx(WH_FOREGROUNDIDLE,
                                          WK32ForegroundIdleHook,
                                          hmodWOW32,
                                          0);

        WOW32ASSERTMSG(ptd->hIdleHook, "\nWK32WowSetIdleHook : ERROR failed to Set Idle Hook Proc\n\n");
    }
    RETURN(0);
}



/*++

 W32Thread - New Thread Starts Here

 Routine Description:

    A newly created thread starts here.   We Allocated the Per Task Data from
    the Threads Stack and point NtCurrentTeb()->WOW32Reserved at it, so that
    we can find it quickly when we dispatch an api or recieve a message from
    Win 32.

    NOTE - The Call to Win32 InitTask() does NOT return until we are in sync
    with the other 16 bit tasks in the non-preemptive scheduler.

    Once We have everything initialized we SetEvent to wake our Creator thread
    and then call Win32 to get in sync with the other tasks running in the
    non-preemptive scheduler.

    Special Case - BOOT
    We return (host_simulate) to the caller - kernel16, so he can complete
    his initialization and then reuse the same thread to start the first app
    (usually wowexec the wow shell).

    The second host_simulate call doesn't return until the app exits
    (see tasking.asm - ExitSchedule) at which point we tidy up the task and
    then kill this thread.   Win32 Non-Preemptive Scheduler will detect the
    thread going away and will then schedule another task.

 ENTRY
  16:16 to New Task Stack

 EXIT
  NEVER RETURNS - Thread Exits

--*/

DWORD W32Thread(LPVOID vpInitialSSSP)
{
    TD td;
    UNICODE_STRING  uImageName;
    WCHAR    wcImageName[MAX_VDMFILENAME];
    RTL_PERTHREAD_CURDIR    rptc;
    PVDMFRAME pFrame;
    PWOWINITTASK16 pArg16;
    PTDB     ptdb;
#if FASTBOPPING
#else
    USHORT SaveIp;
#endif

    RtlZeroMemory(&td, sizeof(TD));

    InitializeCriticalSection(&td.csTD);

    if (gptdShell == NULL) {

        //
        // This is the initial thread, free the temporary TD we used during
        // boot.
        //

        DeleteCriticalSection(&CURRENTPTD()->csTD);
        free_w( (PVOID) CURRENTPTD() );
        gptdShell = &td;

    } else if (pptdWOA) {

        //
        // See WK32WOWLoadModule32
        //

        *pptdWOA = &td;
        pptdWOA = NULL;
    }

    CURRENTPTD() = &td;

    if (fBoot) {
        td.htask16 = 0;
        td.hInst16 = 0;
        td.hMod16  = 0;

        {
            VPVOID vpStack;

#if FASTBOPPING
            vpStack = FASTVDMSTACK();
#else
            vpStack = VDMSTACK();
#endif

            GETFRAMEPTR(vpStack, pFrame);

            pFrame->wAX = 1;

        }

#if FASTBOPPING
        CurrentMonitorTeb = NtCurrentTeb();
        FastWOWCallbackCall();
#else
        SaveIp = getIP();
        host_simulate();
        setIP(SaveIp);
#endif

    }

    //
    // Initialize Per Task Data
    //

    GETFRAMEPTR((VPVOID)vpInitialSSSP, pFrame);
    td.htask16 = pFrame->wTDB;
    ptdb = (PTDB)SEGPTR(td.htask16,0);
    td.VDMInfoiTaskID = iW32ExecTaskId;
    iW32ExecTaskId = (UINT)-1;
    td.vpStack = (VPVOID)vpInitialSSSP;
    td.dwThreadID = GetCurrentThreadId();
    if (THREADID32(td.htask16) == 0) {
        ptdb->TDB_ThreadID = td.dwThreadID;
    }

    EnterCriticalSection(&gcsWOW);
    td.ptdNext = gptdTaskHead;
    gptdTaskHead = &td;
    LeaveCriticalSection(&gcsWOW);
    td.hrgnClip = (HRGN)NULL;

    td.ulLastDesktophDC = 0;
    td.pWOAList = NULL;

    //
    //  NOTE - Add YOUR Per Task Init Code HERE
    //

    //
    // Initialize WOW compatibility flags from registry.
    //

    td.dwWOWCompatFlags = W32ReadWOWCompatFlags(td.htask16, &td.dwWOWCompatFlagsEx);

//
// We now inherit the WOW compatibility flags from the parent's TDB. Right
// now We are only interested in inheriting the WOWCF_UNIQUEHDCHWND flag
// in order to really fix a bug with MS Publisher. Each Wizard and Cue Cards
// that ship with mspub is its own task and would require MANY new
// compatibility flag entries in the registry. This mechanism allows anything
// spawned from an app that has WOWCF_UNIQUEHDCHWND to have
// WOWCF_UNIQUEHDCHWND.
    if (ptdb->TDB_WOWCompatFlags & LOWORD(WOWCF_UNIQUEHDCHWND)) {
        td.dwWOWCompatFlags |= LOWORD(WOWCF_UNIQUEHDCHWND);
    }

    ptdb->TDB_WOWCompatFlags = LOWORD(td.dwWOWCompatFlags);
    ptdb->TDB_WOWCompatFlags2 = HIWORD(td.dwWOWCompatFlags);
    ptdb->TDB_WOWCompatFlagsEx = LOWORD(td.dwWOWCompatFlagsEx);
    ptdb->TDB_WOWCompatFlagsEx2 = HIWORD(td.dwWOWCompatFlagsEx);
#ifdef FE_SB
    td.dwWOWCompatFlags2 = W32ReadWOWCompatFlags2(td.htask16);
    ptdb->TDB_WOWCompatFlagsJPN = LOWORD(td.dwWOWCompatFlags2);
    ptdb->TDB_WOWCompatFlagsJPN2 = HIWORD(td.dwWOWCompatFlags2);
#endif // FE_SB

#ifndef i386
    // Enable the special VDMAllocateVirtualMemory strategy in NTVDM.
    if (td.dwWOWCompatFlagsEx & WOWCFEX_FORCEINCDPMI) {
        SetWOWforceIncrAlloc(TRUE);
    }
#endif

    td.hIdleHook = NULL;

    //
    // Set the CSR batching limit to whatever was specified in
    // win.ini [WOW] BatchLimit= line, which we read into
    // dwWOWBatchLimit during WOW startup in W32Init.
    //
    // This code allows the performance people to benchmark
    // WOW on an API for API basis without having to use
    // a private CSRSRV.DLL with a hardcoded batch limit of 1.
    //
    // Note:  This is a per-thread attribute, so we must call
    // ====   GdiSetBatchLimit during the initialization of
    //        each thread that could call GDI on behalf of
    //        16-bit code.
    //

    if (dwWOWBatchLimit) {

        DWORD  dwOldBatchLimit;

        dwOldBatchLimit = GdiSetBatchLimit(dwWOWBatchLimit);

        LOGDEBUG(LOG_ALWAYS,("WOW W32Thread: Changed thread %d GDI batch limit from %u to %u.\n",
                     nWOWTasks+1, dwOldBatchLimit, dwWOWBatchLimit));
    }


    nWOWTasks++;


    //
    //  Inittask: requires ExpWinVer and Modulename
    //

    {
        DWORD    dwExpWinVer;
        DWORD    dwCompatFlags;
        BYTE     szModName[9]; // modname = 8bytes + nullchar
        BYTE     szBaseFileName[9]; // 8.3 filename minus .3
        LPSTR    pszBaseName;
        CHAR     szFilePath[256];
        LPBYTE   lpModule;
        PWOWINITTASK16 pArg16;
        PTDB     ptdb;
        WORD     wPathOffset;
        BYTE     bImageNameLength;
        ULONG    ulLength;
        BOOL     fRet;
        DWORD    dw;
        HANDLE   hThread;

        GETARGPTR(pFrame, sizeof(WOWINITTASK16), pArg16);
        ptdb = (PTDB)SEGPTR(td.htask16,0);
        td.hInst16 = ptdb->TDB_Module;
        td.hMod16 = ptdb->TDB_pModule;
        hThread = (HANDLE)ptdb->TDB_hThread;
        dwExpWinVer = FETCHDWORD(pArg16->dwExpWinVer);
        RtlCopyMemory(szModName, ptdb->TDB_ModName, 8);
        dwCompatFlags = *((DWORD *)&ptdb->TDB_CompatFlags);
        FREEVDMPTR(ptdb);
        szModName[8] = (BYTE)0;

#define NE_PATHOFFSET   10      // Offset to file path stuff

        dw = MAKELONG(0,td.hMod16);
        GETMISCPTR( dw, lpModule );

        wPathOffset = *((LPWORD)(lpModule+NE_PATHOFFSET));

        bImageNameLength = *(lpModule+wPathOffset);

        bImageNameLength -= 8;      // 7 bytes of trash at the start
        wPathOffset += 8;

        RtlCopyMemory(szFilePath, lpModule + wPathOffset, bImageNameLength);
        szFilePath[bImageNameLength] = 0;

        RtlMultiByteToUnicodeN( wcImageName,
                                sizeof(wcImageName),
                                &ulLength,
                                szFilePath,
                                bImageNameLength );

        wcImageName[bImageNameLength] = L'\0';
        RtlInitUnicodeString(&uImageName, wcImageName);

        LOGDEBUG(2,("WOW W32Thread: setting image name to %ws\n",
                    wcImageName));

        RtlAssociatePerThreadCurdir( &rptc, NULL, &uImageName, NULL );

        FREEMISCPTR( lpModule );

        //
        // Add this task to the list of 16-bit tasks
        //

        AddTaskSharedList(td.htask16, td.hMod16, szModName, szFilePath);

        //
        // Get the base part of the filename, no path or extension,
        // for InitTask to use looking for setup program names.
        // Often this is the same as the module name, to shortcut
        // redundant checks we only pass the base filename if it
        // differs from the module name.
        //

        if (!(pszBaseName = WOW32_strrchr(szFilePath, '\\'))) {
            WOW32ASSERTMSG(FALSE, "W32Thread assumed path was fully qualified, no '\\'.\n");
        }
        pszBaseName++; // skip over backslash to point at start of base filename.
        RtlCopyMemory(szBaseFileName, pszBaseName, sizeof(szBaseFileName) - 1);
        szBaseFileName[sizeof(szBaseFileName) - 1] = 0;
        if (pszBaseName = WOW32_strchr(szBaseFileName, '.')) {
            *pszBaseName = 0;
        }
        if (!WOW32_strcmp(szBaseFileName, szModName)) {
            pszBaseName = NULL;
        } else {
            pszBaseName = szBaseFileName;
        }


        // Init task forces us to the active task in USER
        // and does ShowStartGlass, so new app gets focus correctly
        dw = 0;
        do {
            if (dw) {
                Sleep(dw * 50);
            }

            fRet = (pfnOut.pfnInitTask)(dwExpWinVer,
                                        dwCompatFlags,
                                        szModName,
                                        pszBaseName,
                                        td.htask16,
                                        dwLastHotkey,
                                        fSeparateWow ? 0 : td.VDMInfoiTaskID,
                                        dwLastX,
                                        dwLastY,
                                        dwLastXSize,
                                        dwLastYSize
                                        );
        } while (dw++ < 6 && !fRet);


        if (!fRet) {
            LOGDEBUG(LOG_ALWAYS,
                     ("\n%04X task, PTD address %08X InitTaskFailed\n",
                     td.htask16,
                     &td)
                     );

            W32DestroyTask(&td);
            host_ExitThread(EXIT_FAILURE);
        }

        dwLastHotkey = 0;
        dwLastX = dwLastY = dwLastXSize = dwLastYSize = (DWORD) CW_USEDEFAULT;

        if (fBoot) {

            fBoot = FALSE;

            //
            // This call needs to happen after WOWExec's InitTask call so that
            // USER sees us as expecting Windows version 3.10 -- otherwise they
            // will fail some of the LoadCursor calls.
            //

            InitStdCursorIconAlias();

        } else {

            //
            // Syncronize the new thread with the creator thread.
            // Wake our creator thread
            //

            WOW32VERIFY(SetEvent(ghevWaitCreatorThread));
        }

        td.hThread = hThread;
        LOGDEBUG(2,("WOW W32Thread: New thread ready for execution\n"));

        // turn the timer thread on if its not for the first task
        // which we presume to be wowexec
        if (nWOWTasks != 1) {
            ResumeTimerThread();
        }

        FREEARGPTR(pArg16);
    }

    FREEVDMPTR(pFrame);
    GETFRAMEPTR((VPVOID)vpInitialSSSP, pFrame);
    WOW32ASSERT(pFrame->wTDB == td.htask16);

#if FASTBOPPING
    SETFASTVDMSTACK((VPVOID)vpInitialSSSP);
#else
    SETVDMSTACK(vpInitialSSSP);
#endif
    pFrame->wRetID = RET_RETURN;


    //
    //  Let user set breakpoints before Starting App
    //

    if ( IsDebuggerAttached() ) {

        GETARGPTR(pFrame, sizeof(WOWINITTASK16), pArg16);
        DBGNotifyNewTask((LPVOID)pArg16, OFFSETOF(VDMFRAME,bArgs) );
        FREEARGPTR(pArg16);

        if (flOptions & OPT_BREAKONNEWTASK) {

            LOGDEBUG(
                LOG_ALWAYS,
                ("\n%04X %08X task is starting, PTD address %08X, type g to continue\n\n",
                td.htask16,
                pFrame->vpCSIP,
                &td));

            DebugBreak();
        }
    }


    //
    //   Start APP
    //
    BlockWOWIdle(FALSE);

#ifdef DEBUG
    // BUGBUG: HACK ALERT
    // This code has been added to aid in debugging a problem that only
    // seems to occur on MIPS chk
    // What appears to be happening is that the SS:SP is set correctly
    // above, but sometime later, perhaps during the "BlockWOWIdle" call,
    // the emulator's flat stack pointer ends up getting reset to WOWEXEC's
    // stack. The SETVDMSTACK call below will reset the values we want so
    // that the user can continue normally.
    WOW32ASSERTMSG(LOWORD(vpInitialSSSP)==getSP(), "WOW32: W32Thread Error - SP is invalid!\n");
    SETVDMSTACK(vpInitialSSSP);
#endif

#if NO_W32TRYCALL
    {
    extern INT W32FilterException(INT, PEXCEPTION_POINTERS);
    }
    try {
#endif
#if FASTBOPPING
        CurrentMonitorTeb = NtCurrentTeb();
        FastWOWCallbackCall();
#else
        SaveIp = getIP();
        host_simulate();
        setIP(SaveIp);
#endif
#if NO_W32TRYCALL
    } except (W32FilterException(GetExceptionCode(),
                                 GetExceptionInformation())) {
    }
#endif
    //
    //  We should Never Come Here, an app should get terminated via calling wk32killtask thunk
    //  not by doing an unsimulate call.
    //

#ifdef DEBUG
    WOW32ASSERTMSG(FALSE, "WOW32: W32Thread Error - Too many unsimulate calls\n");
#else
    if (IsDebuggerAttached() && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }
#endif

    W32DestroyTask(&td);
    host_ExitThread(EXIT_SUCCESS);
    return 0;
}


/* WK32KillTask - Force the Distruction of the Current Thread
 *
 * Called When App Does an Exit
 * If there is another active Win16 app then USER32 will schedule another
 * task.
 *
 * ENTRY
 *
 * EXIT
 *  Never Returns - We kill the process
 *
 */

ULONG FASTCALL WK32WOWKillTask(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);

    CURRENTPTD()->dwFlags &= ~TDF_FORCETASKEXIT;
    W32DestroyTask(CURRENTPTD());
    RemoveTaskSharedList();
    host_ExitThread(EXIT_SUCCESS);
    return 0;  // to quiet compiler, never executed.
}


/*++

 W32RemoteThread - New Remote Thread Starts Here

 Routine Description:

    The debugger needs to be able to call back into 16-bit code to
    execute some toolhelp functions.  This function is provided as a remote
    interface to calling 16-bit functions.

 ENTRY
  16:16 to New Task Stack

 EXIT
  NEVER RETURNS - Thread Exits

--*/

VDMCONTEXT  vcRemote;
VDMCONTEXT  vcSave;
VPVOID      vpRemoteBlock = (DWORD)0;
WORD        wPrevTDB = 0;
DWORD       dwPrevEBP = 0;

DWORD W32RemoteThread(VOID)
{
    TD td;
    PVDMFRAME pFrame;
    HANDLE      hThread;
    NTSTATUS    Status;
    THREAD_BASIC_INFORMATION ThreadInfo;
    OBJECT_ATTRIBUTES   obja;
    VPVOID      vpStack;

    RtlZeroMemory(&td, sizeof(TD));

    // turn the timer thread off to resync dos time
    if (nWOWTasks != 1)
        SuspendTimerThread();

    Status = NtQueryInformationThread(
        NtCurrentThread(),
        ThreadBasicInformation,
        (PVOID)&ThreadInfo,
        sizeof(THREAD_BASIC_INFORMATION),
        NULL
        );
    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not get thread information\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    InitializeObjectAttributes(
            &obja,
            NULL,
            0,
            NULL,
            0 );


    Status = NtOpenThread(
                &hThread,
                THREAD_SET_CONTEXT
                  | THREAD_GET_CONTEXT
                  | THREAD_QUERY_INFORMATION,
                &obja,
                &ThreadInfo.ClientId );

    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not get open thread handle\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    cpu_createthread( hThread );

    Status = NtClose( hThread );
    if ( !NT_SUCCESS(Status) ) {
#if DBG
        DbgPrint("NTVDM: Could not close thread handle\n");
        DbgBreakPoint();
#endif
        return( 0 );
    }

    InitializeCriticalSection(&td.csTD);

    CURRENTPTD() = &td;

    //
    // Save the current state (for future callbacks)
    //
    vcSave.SegSs = getSS();
    vcSave.SegCs = getCS();
    vcSave.SegDs = getDS();
    vcSave.SegEs = getES();
    vcSave.Eax   = getAX();
    vcSave.Ebx   = getBX();
    vcSave.Ecx   = getCX();
    vcSave.Edx   = getDX();
    vcSave.Esi   = getSI();
    vcSave.Edi   = getDI();
    vcSave.Ebp   = getBP();
    vcSave.Eip   = getIP();
    vcSave.Esp   = getSP();
#if FASTBOPPING
    {
        extern DWORD    saveebp32;

        dwPrevEBP = saveebp32;
    }
#endif

    wPrevTDB = *pCurTDB;

    //
    // Now prepare for the callback.  Set the registers such that it looks
    // like we are returning from the WOWKillRemoteTask call.
    //
    setDS( (WORD)vcRemote.SegDs );
    setES( (WORD)vcRemote.SegEs );
    setAX( (WORD)vcRemote.Eax );
    setBX( (WORD)vcRemote.Ebx );
    setCX( (WORD)vcRemote.Ecx );
    setDX( (WORD)vcRemote.Edx );
    setSI( (WORD)vcRemote.Esi );
    setDI( (WORD)vcRemote.Edi );
    setBP( (WORD)vcRemote.Ebp );
#if FASTBOPPING

    vpStack = MAKELONG( LOWORD(vcRemote.Esp), LOWORD(vcRemote.SegSs) );

    SETFASTVDMSTACK( vpStack );

#else
    setIP( (WORD)vcRemote.Eip );
    setSP( (WORD)vcRemote.Esp );
    setSS( (WORD)vcRemote.SegSs );
    setCS( (WORD)vcRemote.SegCs );
    vpStack = VDMSTACK();
#endif

    //
    // Initialize Per Task Data
    //
    GETFRAMEPTR(vpStack, pFrame);

    td.htask16 = pFrame->wTDB;
    td.VDMInfoiTaskID = -1;
    td.vpStack = vpStack;
    td.pWOAList = NULL;

    //
    //  NOTE - Add YOUR Per Task Init Code HERE
    //

    nWOWTasks++;

    // turn the timer thread on
    if (nWOWTasks != 1)
        ResumeTimerThread();


    pFrame->wRetID = RET_RETURN;

    pFrame->wAX = (WORD)TRUE;
    pFrame->wDX = (WORD)0;

    //
    //   Start Callback
    //
#if FASTBOPPING
    CurrentMonitorTeb = NtCurrentTeb();
    FastWOWCallbackCall();
#else
    host_simulate();
    setIP((WORD)vcSave.Eip);
#endif

    //
    //  We should Never Come Here, an app should get terminated via calling wk32wowkilltask thunk
    //  not by doing an unsimulate call.
    //

#ifdef DEBUG
    WOW32ASSERTMSG(FALSE, "WOW32: W32RemoteThread Error - Too many unsimulate calls");
#else
    if (IsDebuggerAttached() && (flOptions & OPT_DEBUG)) {
        DbgBreakPoint();
    }
#endif

    W32DestroyTask(&td);
    host_ExitThread(EXIT_SUCCESS);
    return 0;
}

//
// lives in dos/dem/demlfn.c
//
extern VOID demLFNCleanup(VOID);

/* W32FreeTask - Per Task Cleanup
 *
 *  Put any 16-bit task clean-up code here.  The remote thread for debugging
 *  is a 16-bit task, but has no real 32-bit thread associated with it, until
 *  the debugger creates it.  Then it is created and destroyed in special
 *  ways, see W32RemoteThread and W32KillRemoteThread.
 *
 * ENTRY
 *  Per Task Pointer
 *
 * EXIT
 *  None
 *
 */
VOID W32FreeTask( PTD ptd )
{
    PWOAINST pWOA, pWOANext;

    nWOWTasks--;

    if (nWOWTasks < 2)
        SuspendTimerThread();

#ifndef i386
    // Disable the special VDMAllocateVirtualMemory strategy in NTVDM.
    if (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_FORCEINCDPMI) {
        SetWOWforceIncrAlloc(FALSE);
    }
#endif

    // Free all DCs owned by the current task

    FreeCachedDCs(ptd->htask16);

    // Unload network fonts

    if( CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNLOADNETFONTS )
    {
        UnloadNetworkFonts( (UINT)CURRENTPTD() );
    }

    // Free all timers owned by the current task

    DestroyTimers16(ptd->htask16);

    // clean up comm support

    FreeCommSupportResources(ptd->dwThreadID);

    // remove the hacks for this task from the FormFeedHackList (see wgdi.c)
    FreeTaskFormFeedHacks(ptd->htask16);

    // Cleanup WinSock support.

    if (WWS32IsThreadInitialized) {
        WWS32TaskCleanup();
    }

    // Free all local resource info owned by the current task

    DestroyRes16(ptd->htask16);

    // Unhook all hooks and reset their state.

    W32FreeOwnedHooks(ptd->htask16);

    // Free all the resources of this task

    FreeCursorIconAlias(ptd->htask16,CIALIAS_HTASK | CIALIAS_TASKISGONE);

    // Free accelerator aliases

    DestroyAccelAlias(ptd->htask16);

    // Remove idle hook, if any has been installed.

    if (ptd->hIdleHook != NULL) {
        UnhookWindowsHookEx(ptd->hIdleHook);
        ptd->hIdleHook = NULL;
    }

    // Free Special thunking list for this task (wparam.c)

    FreeParamMap(ptd->htask16);

    // cleanup lfn search handles and other lfn-related stuff
    demLFNCleanup();

    // Free WinOldAp tracking structures for this thread.

    EnterCriticalSection(&ptd->csTD);

    if (pWOA = ptd->pWOAList) {
        ptd->pWOAList = NULL;
        while (pWOA) {
            pWOANext = pWOA->pNext;
            free_w(pWOA);
            pWOA = pWOANext;
        }
    }

    LeaveCriticalSection(&ptd->csTD);
}



/* WK32KillRemoteTask - Force the Distruction of the Current Thread
 *
 * Called When App Does an Exit
 * If there is another active Win16 app then USER32 will schedule another
 * task.
 *
 * ENTRY
 *
 * EXIT
 *  Never Returns - We kill the process
 *
 */

ULONG FASTCALL WK32KillRemoteTask(PVDMFRAME pFrame)
{
    PWOWKILLREMOTETASK16 pArg16;
    WORD        wSavedTDB;
    PTD         ptd = CURRENTPTD();
    LPBYTE      lpNum_Tasks;

    //
    // Save the current state (for future callbacks)
    //
    vcRemote.SegDs = getDS();
    vcRemote.SegEs = getES();
    vcRemote.Eax   = getAX();
    vcRemote.Ebx   = getBX();
    vcRemote.Ecx   = getCX();
    vcRemote.Edx   = getDX();
    vcRemote.Esi   = getSI();
    vcRemote.Edi   = getDI();
    vcRemote.Ebp   = getBP();
#if FASTBOPPING
    {
        extern DWORD saveip16;
        extern DWORD savecs16;
        VPVOID       vpStack;

        vcRemote.Eip   = saveip16;
        vcRemote.SegCs = savecs16;
        vpStack = FASTVDMSTACK();

        vcRemote.SegSs = HIWORD(vpStack);
        vcRemote.Esp   = LOWORD(vpStack);
    }
#else
    vcRemote.Eip   = getIP();
    vcRemote.Esp   = getSP();
    vcRemote.SegSs = getSS();
    vcRemote.SegCs = getCS();
#endif

    W32FreeTask(CURRENTPTD());

    if ( vpRemoteBlock ) {

        wSavedTDB = ptd->htask16;
        ptd->htask16 = wPrevTDB;
        pFrame->wTDB = wPrevTDB;

        // This is a nop callback just to make sure that we switch tasks
        // back for the one we were on originally.
        GlobalUnlockFree16( 0 );

        GETFRAMEPTR(ptd->vpStack, pFrame);

        pFrame->wTDB = ptd->htask16 = wSavedTDB;

        //
        // We must be returning from a callback, restore the previous
        // context info.   Don't worry about flags, they aren't needed.
        //
        setSS( (WORD)vcSave.SegSs );
        setCS( (WORD)vcSave.SegCs );
        setDS( (WORD)vcSave.SegDs );
        setES( (WORD)vcSave.SegEs );
        setAX( (WORD)vcSave.Eax );
        setBX( (WORD)vcSave.Ebx );
        setCX( (WORD)vcSave.Ecx );
        setDX( (WORD)vcSave.Edx );
        setSI( (WORD)vcSave.Esi );
        setDI( (WORD)vcSave.Edi );
        setBP( (WORD)vcSave.Ebp );
        setIP( (WORD)vcSave.Eip );
        setSP( (WORD)vcSave.Esp );
#if FASTBOPPING
        {
            extern DWORD    saveebp32;

            saveebp32 = dwPrevEBP;
        }
#endif
    } else {
        //
        // Decrement the count of 16-bit tasks so that the last one,
        // excluding the remote handler (WOWDEB.EXE) will remember to
        // call ExitKernel when done.
        //
        GETVDMPTR(vpnum_tasks, 1, lpNum_Tasks);

        *lpNum_Tasks -= 1;

        FREEVDMPTR(lpNum_Tasks);

        //
        // Remove this 32-bit thread from the list of tasks as well.
        //
        WK32DeleteTask( CURRENTPTD() );
    }

    GETARGPTR(pFrame, sizeof(WOWKILLREMOTETASK16), pArg16);

    //
    // Save the current state (for future callbacks)
    //
    vpRemoteBlock = FETCHDWORD(pArg16->lpBuffer);

    // Notify DBG that we have a remote thread address
    DBGNotifyRemoteThreadAddress( W32RemoteThread, vpRemoteBlock );

    FREEARGPTR(pArg16);

    host_ExitThread(EXIT_SUCCESS);
    return 0;  // never executed, keep compiler happy.
}


/* W32DestroyTask - Per Task Cleanup
 *
 *  Task destruction code here.  Put any 32-bit task cleanup code here
 *
 * ENTRY
 *  Per Task Pointer
 *
 * EXIT
 *  None
 *
 */

VOID W32DestroyTask( PTD ptd)
{

    LOGDEBUG(LOG_IMPORTANT,("W32DestroyTask: destroying task %04X\n", ptd->htask16));

    // Inform Hung App Support

    SetEvent(ghevWaitHungAppNotifyThread);

    // Free all information pertinant to this 32-bit thread
    W32FreeTask( ptd );

    // delete the cliprgn used by GetClipRgn if it exists

    if (ptd->hrgnClip != NULL)
    {
        DeleteObject(ptd->hrgnClip);
        ptd->hrgnClip = NULL;
    }

    // Report task termination to Win32 - incase someone is waiting for us
    // LATER - fix Win32 so we don't have to report it.


    if (nWOWTasks == 0) {   // If we're the last one out, turn out the lights & tell Win32 WOWVDM is history.
        ptd->VDMInfoiTaskID = -1;
        ExitVDM(WOWVDM, ALL_TASKS);          // Tell Win32 All Tasks are gone.
    }
    else if (ptd->VDMInfoiTaskID != -1 ) {  // If 32 bit app is waiting for us - then signal we are done
        ExitVDM(WOWVDM, ptd->VDMInfoiTaskID);
    }
    ptd->dwFlags &= ~TDF_IGNOREINPUT;

    if (!(ptd->dwFlags & TDF_TASKCLEANUPDONE)) {
        (pfnOut.pfnWOWCleanup)(HINSTRES32(ptd->hInst16), (DWORD) ptd->htask16);
    }


    // Remove this task from the linked list of tasks

    WK32DeleteTask(ptd);

    // Close This Apps Thread Handle

    if (ptd->hThread) {
        CloseHandle( ptd->hThread );
    }

    DeleteCriticalSection(&ptd->csTD);
}

/***************************************************************************\
* WK32DeleteTask
*
* This function removes a task from the task list.
*
* History:
* Borrowed From User32 taskman.c - mattfe aug 5 92
\***************************************************************************/

void WK32DeleteTask(
    PTD ptdDelete)
{
    PTD ptd, ptdPrev;

    EnterCriticalSection(&gcsWOW);
    ptd = gptdTaskHead;
    ptdPrev = NULL;

    /*
     * Find the task to delete
     */
    while ((ptd != NULL) && (ptd != ptdDelete)) {
        ptdPrev = ptd;
        ptd = ptd->ptdNext;
    }

    /*
     * Error if we didn't find it.  If we did find it, remove it
     * from the chain.  If this was the head of the list, set it
     * to point to our next guy.
     */
    if (ptd == NULL) {
        LOGDEBUG(LOG_ALWAYS,("WK32DeleteTask:Task not found.\n"));
    } else if (ptdPrev != NULL) {
        ptdPrev->ptdNext = ptd->ptdNext;
    } else {
        gptdTaskHead = ptd->ptdNext;
    }
    LeaveCriticalSection(&gcsWOW);
}


/*++
 WK32RegisterShellWindowHandle - 16 Bit Shell Registers is Hanle

 Routine Description:
    This routines saves the 32 bit hwnd for the 16 bit shell

    When WOWEXEC (16 bit shell) has sucessfully created its window it calls us to
    register its window handle.   If this is the shared WOW VDM, we register the
    handle with BaseSrv, which posts WM_WOWEXECSTARTAPP messages when Win16 apps
    are started.

 ENTRY
  pFrame -> hwndShell, 16 bit hwnd for shell (WOWEXEC)

 EXIT
  TRUE  - This is the shared WOW VDM
  FALSE - This is a separate WOW VDM

--*/

ULONG FASTCALL WK32RegisterShellWindowHandle(PVDMFRAME pFrame)
{
    register PWOWREGISTERSHELLWINDOWHANDLE16 parg16;
    WNDCLASS wc;
    NTSTATUS Status;

    GETARGPTR(pFrame, sizeof(WOWREGISTERSHELLWINDOWHANDLE16), parg16);

// gwFirstCmdShow is no longer used, and is available.
#if 0
    GETVDMPTR(parg16->lpwCmdShow, sizeof(WORD), pwCmdShow);
#endif

    if (ghwndShell) {

        //
        // The shared WOW is calling to deregister right before it
        // shuts down.
        //

        WOW32ASSERT( !fSeparateWow );
        WOW32ASSERT( !parg16->hwndShell );

        Status = RegisterWowExec(NULL);

        return NT_SUCCESS(Status);
    }

    ghwndShell = HWND32(parg16->hwndShell);
    ghShellTDB = pFrame->wTDB;

    //
    // Save away the hInstance for User32
    //

    GetClassInfo(0, (LPCSTR)0x8000, &wc);
    ghInstanceUser32 = wc.hInstance;

    // Fritz, when you get called about this it means that the GetClassInfo()
    // call above is returning with lpWC->hInstance == 0 instead of hModuser32.
    WOW32ASSERTMSGF((ghInstanceUser32),
                    ("WOW Error ghInstanceUser32 == NULL! Contact user folks\n"));

    //
    // If this is the shared WOW VDM, register the WowExec window handle
    // with BaseSrv so it can post WM_WOWEXECSTARTAPP messages.
    //

    if (!fSeparateWow) {
        RegisterWowExec(ghwndShell);
    }

    WOW32FaxHandler(WM_DDRV_SUBCLASS, (LPSTR)(HWND32(parg16->hwndFax)));

    FREEARGPTR(parg16);


    //
    // Return value is TRUE if this is the shared WOW VDM,
    // FALSE if this is a separate WOW VDM.
    //

    return fSeparateWow ? FALSE : TRUE;
}


//
// Worker routine for WK32WOWLoadModule32
//

VOID FASTCALL CleanupWOAList(HANDLE hProcess)
{
    PTD ptd;
    PWOAINST *ppWOA, pWOAToFree;

    EnterCriticalSection(&gcsWOW);

    ptd = gptdTaskHead;

    while (ptd) {

        EnterCriticalSection(&ptd->csTD);

        ppWOA = &(ptd->pWOAList);
        while (*ppWOA && (*ppWOA)->hChildProcess != hProcess) {
            ppWOA = &( (*ppWOA)->pNext );
        }

        if (*ppWOA) {

            //
            // We found the WOAINST structure to clean up.
            //

            pWOAToFree = *ppWOA;

            //
            // Remove this entry from the list
            //

            *ppWOA = pWOAToFree->pNext;

            free_w(pWOAToFree);

            LeaveCriticalSection(&ptd->csTD);

            break;   // no need to look at other tasks.

        }

        LeaveCriticalSection(&ptd->csTD);

        ptd = ptd->ptdNext;
    }

    LeaveCriticalSection(&gcsWOW);
}


/*++
 WK32WOWLoadModule32

 Routine Description:
    Exec a 32 bit Process
    This routine is called by the 16 bit kernel when it fails to load a 16 bit task
    with error codes 11 - invalid exe, 12 - os2, 13 - DOS 4.0, 14 - Unknown.

 ENTRY
  pFrame -> lpCmdLine        Input\output buffer for winoldapp cmd line
  pFrame -> lpParameterBlock (see win 3.x apis) Parameter Block if NULL
                             winoldap calling
  pFrame -> lpModuleName     (see win 3.x apis) App Name

 EXIT
  32 - Sucess
  Error code

 History:
 rewrote to call CreateProcess() instead of LoadModule   - barryb 29sep92

--*/


ULONG FASTCALL WK32WOWLoadModule32(PVDMFRAME pFrame)
{
    static PSZ pszExplorerFullPathUpper = NULL;         // "C:\WINNT\EXPLORER.EXE"

    ULONG ulRet;
    int i;
    char *pch, *pSrc;
    PSZ pszModuleName;
    PSZ pszWinOldAppCmd;
    PBYTE pbCmdLine;
    BOOL CreateProcessStatus;
    PPARAMETERBLOCK16 pParmBlock16;
    PWORD16 pCmdShow = NULL;
    BOOL fProgman = FALSE;
    PROCESS_INFORMATION ProcessInformation;
    STARTUPINFO StartupInfo;
    char CmdLine[2*MAX_PATH];
    char szOut[2*MAX_PATH];
    char szMsgBoxText[4*MAX_PATH];
    register PWOWLOADMODULE16 parg16;
    PTD ptd;

    GETARGPTR(pFrame, sizeof(WOWLOADMODULE16), parg16);
    GETPSZPTR(parg16->lpWinOldAppCmd, pszWinOldAppCmd);
    if (parg16->lpParameterBlock) {
        GETVDMPTR(parg16->lpParameterBlock,sizeof(PARAMETERBLOCK16), pParmBlock16);
        GETPSZPTR(pParmBlock16->lpCmdLine, pbCmdLine);
    } else {
        pParmBlock16 = NULL;
        pbCmdLine = NULL;
    }

    UpdateDosCurrentDirectory(DIR_DOS_TO_NT); // update current dir


    /*
     *  if ModuleName == NULL, called by winoldap, or LM_NTLOADMODULE
     *     to deal with the process handle.
     *
     *     if lpParameterBlock == NULL
     *        winoldap calling to wait on the process handle
     *     else
     *        LM_NTLoadModule calling to clean up process handle
     *        because an error ocurred loading winoldap.
     */
    if (!parg16->lpModuleName) {
        HANDLE hProcess;
        MSG msg;

        pszModuleName = NULL;

        if (pszWinOldAppCmd &&
            *pszWinOldAppCmd &&
            RtlEqualMemory(pszWinOldAppCmd, szWOAWOW32, sizeof(szWOAWOW32)-1))
          {
            hProcess = (HANDLE)strtoul(pszWinOldAppCmd + sizeof(szWOAWOW32) - 1,
                                       NULL,
                                       16
                                       );
            if (hProcess == (HANDLE)-1)  {         // ULONG_MAX
                hProcess = NULL;
            }

            if (parg16->lpParameterBlock && hProcess) {

                //
                // Error loading winoldap.mod
                //

                pptdWOA = NULL;
                CleanupWOAList(hProcess);
                CloseHandle(hProcess);
                hProcess = NULL;
            }
        } else {
            hProcess = NULL;
        }

        BlockWOWIdle(TRUE);

        if (hProcess) {
            while (MsgWaitForMultipleObjects(1, &hProcess, FALSE, INFINITE, QS_ALLINPUT)
                   == WAIT_OBJECT_0 + 1)
            {
                PeekMessage(&msg, NULL, 0,0, PM_NOREMOVE);
            }

            if (!GetExitCodeProcess(hProcess, &ulRet)) {
                ulRet = 0;
            }

            CleanupWOAList(hProcess);
            CloseHandle(hProcess);
        } else {
            (pfnOut.pfnYieldTask)();
            ulRet = 0;
        }

        BlockWOWIdle(FALSE);

        goto lm32Exit;


     /*
      *  if ModuleName == -1, uses traditional style winoldap cmdline
      *  and is called to spawn a non win16 app.
      *
      *    "<cbWord><CmdLineParameters>CR<ModulePathName>LF"
      *
      *  Extract the ModuleName from the command line
      *
      */
    } else if (parg16->lpModuleName == -1) {
        pszModuleName = NULL;

        pSrc = pbCmdLine + 2;
        pch = WOW32_strchr(pSrc, '\r');
        if (!pch || (i = pch - pSrc) >= MAX_PATH) {
            ulRet = 23;
            goto lm32Exit;
            }

        pSrc = pch + 1;
        pch = WOW32_strchr(pSrc, '\n');
        if (!pch || (i = pch - pSrc) >= MAX_PATH) {
            ulRet = 23;
            goto lm32Exit;
            }

        pch = CmdLine;
        while (*pSrc != '\n' && *pSrc) {
            *pch++ = *pSrc++;
        }
        *pch++ = ' ';


        pSrc = pbCmdLine + 2;
        while (*pSrc != '\r' && *pSrc) {
            *pch++ = *pSrc++;
        }
        *pch = '\0';

     /*
      * lpModuleName contains Application Path Name
      * pbCmdLIne contains Command Tail
      */
    } else {
        GETPSZPTR(parg16->lpModuleName, pszModuleName);
        if (pszModuleName) {
            //
            // 2nd part of control.exe/progman.exe implemented here.  In the
            // first part, in WK32WowIsKnownDll, forced the 16-bit loader to
            // load c:\winnt\system32\control.exe(progman.exe) if the app
            // tries to load c:\winnt\control.exe(progman.exe).  16-bit
            // LoadModule tries and eventually discovers its a PE module
            // and returns LME_PE, which causes this function to get called.
            // Unfortunately, the scope of the WK32WowIsKnownDLL modified
            // path is LMLoadExeFile, so by the time we get here, the path is
            // once again c:\winnt\control.exe(progman.exe).  Fix that.
            //

            if (!WOW32_stricmp(pszModuleName, pszControlExeWinDirPath) ||
                (fProgman = TRUE,
                 !WOW32_stricmp(pszModuleName, pszProgmanExeWinDirPath))) {

                strcpy(CmdLine, fProgman
                                 ? pszProgmanExeSysDirPath
                                 : pszControlExeSysDirPath);
            } else {
                strcpy(CmdLine, pszModuleName);
            }

            FREEPSZPTR(pszModuleName);
            }
        else {
            ulRet = 2; // LME_FNF
            goto lm32Exit;
            }


        pch = CmdLine + strlen(CmdLine);
        *pch++ = ' ';

        //
        // The cmdline is a Pascal-style string: a count byte followed by
        // characters followed by a terminating CR character.  If this string is
        // not well formed we will still try to reconstruct the command line in
        // a similar manner that the c startup code does so using the following
        // assumptions:
        //
        // 1. The command line can be no greater that 128 characters including
        //    the length byte and the terminator.
        //
        // 2. The valid terminators for a command line are CR or 0.
        //
        //

        i = 0;
        pSrc = pbCmdLine+1;
        while (*pSrc != '\r' && *pSrc && i < 0x80 - 2) {
            *pch++ = *pSrc++;
        }
        *pch = '\0';
    }


    RtlZeroMemory((PVOID)&StartupInfo, (DWORD)sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;

    //
    // pCmdShow is documented as a pointer to an array of two WORDs,
    // the first of which must be 2, and the second of which is
    // the nCmdShow to use.  It turns out that Win3.1 ignores
    // the second word (uses SW_NORMAL) if the first word isn't 2.
    // Pixie 2.0 passes an array of 2 zeros, which on Win 3.1 works
    // because the nCmdShow of 0 (== SW_HIDE) is ignored since the
    // first word isn't 2.
    //
    // Our logic, then, is to use SW_NORMAL unless pCmdShow is
    // valid and points to a WORD value 2, in which case we use
    // the next word as nCmdShow.
    //
    // DaveHart 27 June 1993.
    //

    GETVDMPTR(pParmBlock16->lpCmdShow, 4, pCmdShow);
    if (pCmdShow && 2 == pCmdShow[0]) {
        StartupInfo.wShowWindow = pCmdShow[1];
    } else {
        StartupInfo.wShowWindow = SW_NORMAL;
    }

    if (pCmdShow)
        FREEVDMPTR(pCmdShow);


    CreateProcessStatus = CreateProcess(
                            NULL,
                            CmdLine,
                            NULL,               // security
                            NULL,               // security
                            FALSE,              // inherit handles
                            CREATE_NEW_CONSOLE | CREATE_DEFAULT_ERROR_MODE,
                            NULL,               // environment strings
                            NULL,               // current directory
                            &StartupInfo,
                            &ProcessInformation
                            );


    if (CreateProcessStatus) {
        DWORD WaitStatus;

        if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_SYNCHRONOUSDOSAPP) {
            LPBYTE lpT;

            // This is for supporting BeyondMail installation. It uses
            // 40:72 as shared memory when it execs DOS programs. The windows
            // part of installation program loops till the byte at 40:72 is
            // non-zero. The DOS program  ORs in 0x80 into this location which
            // effectively signals the completion of the DOS task. On NT
            // Windows and Dos programs are different processes and thus this
            // 'sharing' business doesn't work. Hence this compatibility stuff.
            //                                                - nanduri

            WaitStatus = WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
            lpT = GetRModeVDMPointer(0x400072);
            *lpT |= 0x80;
        }
        else if (!(CURRENTPTD()->dwWOWCompatFlags & WOWCF_NOWAITFORINPUTIDLE)) {

           DWORD dw;
           int i = 20;

            //
            // Wait for the started process to go idle.
            //
            do {
                dw = WaitForInputIdle(ProcessInformation.hProcess, 5000);
                WaitStatus = WaitForSingleObject(ProcessInformation.hProcess, 0);
            } while (dw == WAIT_TIMEOUT && WaitStatus == WAIT_TIMEOUT && i--);
        }

        CloseHandle(ProcessInformation.hThread);

        if (ProcessInformation.hProcess) {

            PWOAINST pWOAInst;
            DWORD    cb;

            //
            // We're returning a process handle to winoldap, so
            // build up a WOAINST structure add add it to this
            // task's list of child WinOldAp instances.
            //

            if (parg16->lpModuleName && -1 != parg16->lpModuleName) {

                GETPSZPTR(parg16->lpModuleName, pszModuleName);
                cb = strlen(pszModuleName)+1;

            } else {

                cb = 1;  // null terminator
                pszModuleName = NULL;

            }

            //
            // WOAINST includes one byte of szModuleName in its
            // size, allocate enough room for the full string.
            //

            pWOAInst = malloc_w( (sizeof *pWOAInst) + cb - 1 );
            WOW32ASSERT(pWOAInst);

            if (pWOAInst) {

                ptd = CURRENTPTD();

                EnterCriticalSection(&ptd->csTD);

                pWOAInst->pNext = ptd->pWOAList;
                ptd->pWOAList = pWOAInst;

                pWOAInst->dwChildProcessID = ProcessInformation.dwProcessId;
                pWOAInst->hChildProcess = ProcessInformation.hProcess;

                //
                // point pptdWOA at pWOAInst->ptdWOA so that
                // W32Thread can fill in the pointer to the
                // WinOldAp TD.
                //

                pWOAInst->ptdWOA = NULL;
                pptdWOA = &(pWOAInst->ptdWOA);

                if (pszModuleName == NULL) {

                    pWOAInst->szModuleName[0] = 0;

                } else {

                    RtlCopyMemory(
                        pWOAInst->szModuleName,
                        pszModuleName,
                        cb
                        );

                    //
                    // We are storing pszModuleName for comparison
                    // later in WowGetModuleHandle, called by
                    // Win16 GetModuleHandle.  The latter always
                    // uppercases the paths involved, so we do
                    // as well so that we can do a case-insensitive
                    // comparison.
                    //

                    WOW32_strupr(pWOAInst->szModuleName);

                    //
                    // HACK -- PackRat can't run Explorer in one
                    // of its "Application Windows", because the
                    // spawned explorer.exe process goes away
                    // after asking the existing explorer to put
                    // up a window.
                    //
                    // If we're starting Explorer, close the
                    // process handle find the "real" shell
                    // explorer.exe process and put its handle
                    // and ID in this WOAINST structure.  This
                    // fixes PackRat, but means that the
                    // winoldap task never goes away because
                    // the shell never goes away.
                    //

                    if (! pszExplorerFullPathUpper) {

                        int nLenWin = strlen(pszWindowsDirectory);
                        int nLenExpl = strlen(szExplorerDotExe);

                        //
                        // pszExplorerFullPathUpper looks like "C:\WINNT\EXPLORER.EXE"
                        //

                        pszExplorerFullPathUpper =
                            malloc_w(nLenWin +                          // strlen(pszWindowsDirectory)
                                     1 +                                // backslash
                                     nLenExpl +                         // strlen("explorer.exe")
                                     1                                  // null terminator
                                     );

                        if (pszExplorerFullPathUpper) {
                            RtlCopyMemory(pszExplorerFullPathUpper, pszWindowsDirectory, nLenWin);
                            pszExplorerFullPathUpper[nLenWin] = '\\';
                            RtlCopyMemory(&pszExplorerFullPathUpper[nLenWin+1], szExplorerDotExe, nLenExpl+1);
                            WOW32_strupr(pszExplorerFullPathUpper);
                        }

                    }

                    if (pszExplorerFullPathUpper &&
                        ! WOW32_strcmp(pWOAInst->szModuleName, pszExplorerFullPathUpper)) {

                        GetWindowThreadProcessId(
                            GetShellWindow(),
                            &pWOAInst->dwChildProcessID
                            );

                        CloseHandle(pWOAInst->hChildProcess);
                        pWOAInst->hChildProcess = ProcessInformation.hProcess =
                            OpenProcess(
                                PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                                FALSE,
                                pWOAInst->dwChildProcessID
                                );
                    }

                }

                LeaveCriticalSection(&ptd->csTD);
            }

            if (pszModuleName) {
                FREEPSZPTR(pszModuleName);
            }
        }

        ulRet = 33;
        pch = pszWinOldAppCmd + 2;
        sprintf(pch, "%s%x\r", szWOAWOW32, ProcessInformation.hProcess);
        *pszWinOldAppCmd = (char) strlen(pch);
        *(pszWinOldAppCmd+1) = '\0';

    } else {
        //
        // CreateProcess failed, map the most common error codes
        //
        switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND:
            ulRet = 2;
            break;

        case ERROR_PATH_NOT_FOUND:
            ulRet = 3;
            break;

        case ERROR_BAD_EXE_FORMAT:
            ulRet = 11;
            break;

        // put up warning that they're trying to load a binary intended for
        // a different platform
        case ERROR_EXE_MACHINE_TYPE_MISMATCH:

            // attempt to find the end of the module name path
            pch = CmdLine;
            while((*pch != ' ') && (*pch != '//') && (*pch != '\0')) {
               pch++;
            }
            *pch = '\0';
            LoadString(hmodWOW32,
                       iszMisMatchedBinary,
                       szMsgBoxText,
                       sizeof szMsgBoxText);

            sprintf(szOut, szMsgBoxText, CmdLine);

            LoadString(hmodWOW32,
                       iszMisMatchedBinaryTitle,
                       szMsgBoxText,
                       sizeof szMsgBoxText);

            MessageBox(NULL,
                       szOut,
                       szMsgBoxText,
                       MB_OK | MB_ICONEXCLAMATION);

            // fall through to default case

        default:
            ulRet = 0; // no memory
            break;
        }

    }


lm32Exit:
    FREEARGPTR(parg16);
    FREEPSZPTR(pbCmdLine);
    FREEPSZPTR(pszWinOldAppCmd);
    if (pParmBlock16)
        FREEVDMPTR(pParmBlock16);

    RETURN(ulRet);
}


/*++
 WK32WOWQueryPerformanceCounter

 Routine Description:
    Calls NTQueryPerformanceCounter
    Implemented for Performance Group

 ENTRY
  pFrame -> lpPerformanceFrequency points to location for storing Frequency
  pFrame -> lpPerformanceCounter points to location for storing Counter

 EXIT
  NTStatus Code

--*/

ULONG FASTCALL WK32WOWQueryPerformanceCounter(PVDMFRAME pFrame)
{
    PLARGE_INTEGER pPerfCount16;
    PLARGE_INTEGER pPerfFreq16;
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
    register PWOWQUERYPERFORMANCECOUNTER16 parg16;

    GETARGPTR(pFrame, sizeof(WOWQUERYPERFORMANCECOUNTER16), parg16);

    if (parg16->lpPerformanceCounter != 0) {
        GETVDMPTR(parg16->lpPerformanceCounter, 8, pPerfCount16);
    }
    if (parg16->lpPerformanceFrequency != 0) {
        GETVDMPTR(parg16->lpPerformanceFrequency, 8, pPerfFreq16);
    }

    NtQueryPerformanceCounter ( &PerformanceCounter, &PerformanceFrequency );

    if (parg16->lpPerformanceCounter != 0) {
        STOREDWORD(pPerfCount16->LowPart,PerformanceCounter.LowPart);
        STOREDWORD(pPerfCount16->HighPart,PerformanceCounter.HighPart);
    }

    if (parg16->lpPerformanceFrequency != 0) {
        STOREDWORD(pPerfFreq16->LowPart,PerformanceFrequency.LowPart);
        STOREDWORD(pPerfFreq16->HighPart,PerformanceFrequency.HighPart);
    }

    FREEVDMPTR(pPerfCount16);
    FREEVDMPTR(pPerfFreq16);
    FREEARGPTR(parg16);
    RETURN(TRUE);
}

/*++
  WK32WOWOutputDebugString - Write a String to the debugger

  The 16 bit kernel OutputDebugString calls this thunk to actually output the string to the
  debugger.   The 16 bit kernel routine does all the parameter validation etc before calling
  this routine.   Note also that all 16 bit kernel trace output also uses this routine, so
  it not just the app which calls this function.

  If this is a checked build the the output is send via LOGDEBUG so that it gets mingled with
  the WOW trace information, this is useful when running the 16 bit logger tool.


  Entry
    pFrame->vpString Pointer to NULL terminated string to output to the debugger.

  EXIT
    ZERO

--*/

ULONG FASTCALL WK32WOWOutputDebugString(PVDMFRAME pFrame)
{
    PSZ psz1;
    register PWOWOUTPUTDEBUGSTRING16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZPTRNOLOG(parg16->vpString, psz1);

#ifdef DEBUG            // So we can intermingle LOGGER output & WOW Logging
    if ( !(flOptions & OPT_DEBUG) ) {
        OutputDebugString(psz1);
    } else {
        INT  length;
        char text[TMP_LINE_LEN];
        PSZ  pszTemp;

        length = strlen(psz1);
        if ( length > TMP_LINE_LEN-1 ) {
            WOW32_strncpy( text, psz1, TMP_LINE_LEN );
            text[TMP_LINE_LEN-2] = '\n';
            text[TMP_LINE_LEN-1] = '\0';
            pszTemp = text;
        } else {
            pszTemp = psz1;
        }

        LOGDEBUG(LOG_ALWAYS, ("%s", pszTemp));     // in debug version
    }
#else
    OutputDebugString(psz1);
#endif
    FREEPSZPTR(psz1);
    FREEARGPTR(parg16);
    RETURN(0);
}



/* WK32WowFailedExec - WOWExec Failed to Exec Application
 *
 *
 * Entry - Global Variable iW32ExecTaskId
 *
 *
 * Exit
 *     SUCCESS TRUE
 *
 */

ULONG FASTCALL WK32WowFailedExec(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);
    if(iW32ExecTaskId != -1) {
        ExitVDM(WOWVDM,iW32ExecTaskId);
        iW32ExecTaskId = (UINT)-1;
        ShowStartGlass (0);
    }
    FlushMapFileCaches();
    return TRUE;
}


/*++

    Hung App Support
    ================

    There are many levels at which hung app support works.   The User will
    bring up the Task List and hit the End Task Button.    USER32 will post
    a WM_ENDSESSION message to the app.   If the app does not exit after a specified
    timeout them USER will call W32HunAppThread, provided that the task is at the
    client/server boundary.   If the app is looping (ie not at the client/server
    boundary) then it will use the HungAppNotifyThread to alter WOW to kill
    the currently running task.    For the case of W32EndTask we simply
    return back to the 16 bit kernel and force it to perform and Int 21 4C Exit
    call.   For the case of the HungAppNotifyThread we have to somehow grab
    the apps thread - at a point which is "safe".   On non x86 platforms this
    means that the emulator must be at a know safe state - ie not actively emulating
    instructions.    The worst case is if the app is spinning with interrupts
    disabled.

    Notify Thread will
        Force Interrupts to be Enabled SetMSW()
        Set global flag for heartbeatthread so it knows there is work to do
        wait for the app to exit
        timeout - terminate thread() reduce # of tasks

    Alter Global Flag in 16 bit Kernel, that is checked on TimerTick Routines,
    that routine will:-

        Tidy the stack if  on the DOSX stack during h/w interrupt simulation
        Force Int 21 4C exit - might have to patch return address of h/w interrupt
        and then do it at simulated TaskTime.

    Worst Case
    If we don't kill the app in the timeout specified the WOW will put up a dialog
    and then ExitProcess to kill itself.

    Suggestions - if we don't managed to cleanly kill a task we should reduce
    the app count by 2 - (ie the task and WOWExec, so when the last 16 bit app
    goes away we will shutdown WOW).   Also in the case put up a dialog box
    stating you should save your work for 16 bit apps too.

--*/


/*++

 InitializeHungAppSupport - Setup Necessary Threads and Callbacks

 Routine Description
    Create a HungAppNotification Thread
    Register CallBack Handlers With SoftPC Base which are called when
    interrupt simulation is required.

 Entry
    NONE

 EXIT
    TRUE - Success
    FALSE - Faled

--*/
BOOL WK32InitializeHungAppSupport(VOID)
{

    // Register Interrupt Idle Routine with SoftPC
    ghevWowExecMsgWait = RegisterWOWIdle();


    // Create HungAppNotify Thread

    InitializeCriticalSection(&gcsWOW);
    InitializeCriticalSection(&gcsHungApp);  // protects VDM_WOWHUNGAPP bit

    if(!(pfnOut.pfnRegisterUserHungAppHandlers)((PFNW32ET)W32HungAppNotifyThread,
                                     ghevWowExecMsgWait))
       {
        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Error Failed to RegisterUserHungAppHandlers\n"));
        return FALSE;
    }

    if (!(ghevWaitHungAppNotifyThread = CreateEvent(NULL, TRUE, FALSE, NULL))) {
        LOGDEBUG(LOG_ALWAYS,("WK32InitializeHungAppSupport ERROR: event allocation failure\n"));
        return FALSE;
    }


    return TRUE;
}





/*++
 WK32WowWaitForMsgAndEvent

 Routine Description:
    Calls USER32 WowWaitForMsgAndEvent
    Called by WOWEXEC (interrupt dispatch optimization)

 ENTRY
  pFrame->hwnd must be WOWExec's hwnd

 EXIT
  FALSE - A message has arrived, WOWExec must call GetMessage
  TRUE  - The interrupt event was toggled, no work for WOWExec

--*/

ULONG FASTCALL WK32WowWaitForMsgAndEvent(PVDMFRAME pFrame)
{
    register PWOWWAITFORMSGANDEVENT16 parg16;
    BOOL  RetVal;

    GETARGPTR(pFrame, sizeof(WOWWAITFORMSGANDEVENT16), parg16);

    //
    // This is a private api so lets make sure it is wowexec
    //
    if (ghwndShell != HWND32(parg16->hwnd)) {
        FREEARGPTR(parg16);
        return FALSE;
    }

    //
    // WowExec will set VDM_TIMECHANGE bit in the pntvdmstate
    // when it receives a WM_TIMECHANGE message. It is now safe
    // to Reinit the Virtual Timer Hardware as wowexec is the currently
    // scheduled task, and we expect no one to be polling on
    // timer hardware\Bios tic count.
    //
    if (*pNtVDMState & VDM_TIMECHANGE) {
        SuspendTimerThread();
        ResumeTimerThread();
        }

    BlockWOWIdle(TRUE);

    RetVal = (ULONG) (pfnOut.pfnWowWaitForMsgAndEvent)(ghevWowExecMsgWait);

    BlockWOWIdle(FALSE);

    FREEARGPTR(parg16);
    return RetVal;
}


/*++
 WowMsgBoxThread

 Routine Description:
    Worker Thread routine which does all of the msg box work for
    Wk32WowMsgBox (See below)

 ENTRY

 EXIT
  VOID

--*/
DWORD WowMsgBoxThread(VOID *pv)
{
    PWOWMSGBOX16 pWowMsgBox16 = (PWOWMSGBOX16)pv;
    PSZ   pszMsg, pszTitle;
    char  szMsg[MAX_PATH*2];
    char  szTitle[MAX_PATH];
    UINT  Style;


    if (pWowMsgBox16->pszMsg) {
        GETPSZPTR(pWowMsgBox16->pszMsg, pszMsg);
        szMsg[MAX_PATH*2 - 1] = '\0';
        WOW32_strncpy(szMsg, pszMsg, MAX_PATH*2 - 1);
        FREEPSZPTR(pszMsg);
    } else {
        szMsg[0] = '\0';
    }

    if (pWowMsgBox16->pszTitle) {
        GETPSZPTR(pWowMsgBox16->pszTitle, pszTitle);
        szTitle[MAX_PATH - 1] = '\0';
        WOW32_strncpy(szTitle, pszTitle, MAX_PATH);
        FREEPSZPTR(pszTitle);
    } else {
        szTitle[0] = '\0';
    }

    Style = pWowMsgBox16->dwOptionalStyle | MB_OK | MB_SYSTEMMODAL;

    pWowMsgBox16->dwOptionalStyle = 0xffffffff;

    MessageBox (NULL, szMsg, szTitle, Style);

    return 1;
}



/*++
 WK32WowMsgBox

 Routine Description:
    Creates an asynchronous msg box and returns immediately
    without waiting for the msg box to be dismissed. Provided
    for WowExec as WowExec must use its special WowWaitForMsgAndEvent
    api for hardware interrupt dispatching.

    Called by WOWEXEC (interrupt dispatch optimization)

 ENTRY
     pszMsg          - Message for MessageBox
     pszTitle        - Caption for MessageBox
     dwOptionalStyle - MessageBox style bits additional to
                       MB_OK | MB_SYSTEMMODAL

 EXIT
     VOID - nothing is returned as we do not wait for a reply from
            the user.

--*/

ULONG FASTCALL WK32WowMsgBox(PVDMFRAME pFrame)
{
    PWOWMSGBOX16 pWowMsgBox16;
    DWORD Tid;
    HANDLE hThread;

    GETARGPTR(pFrame, sizeof(WOWMSGBOX16), pWowMsgBox16);
    hThread = CreateThread(NULL, 0, WowMsgBoxThread, (PVOID)pWowMsgBox16, 0, &Tid);
    if (hThread) {
        do {
           if (WaitForSingleObject(hThread, 15) != WAIT_TIMEOUT)
               break;
        } while (pWowMsgBox16->dwOptionalStyle != 0xffffffff);

        CloseHandle(hThread);
        }
    else {
        WowMsgBoxThread((PVOID)pWowMsgBox16);
        }

    FREEARGPTR(pWowMsgBox16);
    return 0;
}



#ifdef debug
UINT  gLasthtaskKill = 0;
#endif

/*++

 W32HungAppNotifyThread

    USER32 Calls this routine:
        1 - if the App Agreed to the End Task (from Task List)
        2 - if the app didn't respond to the End Task
        3 - shutdown

    NTVDM Calls this routine:
        1 - if an app has touched some h/w that it shouldn't and the user
            requiested to terminate the app (passed NULL for current task)

    WOW32 Calls this routine:
        1 - when WowExec receives a WM_WOWEXECKILLTASK message.

 ENTRY
  hKillUniqueID - TASK ID of task to kill or NULL for current Task

 EXIT
  NEVER RETURNS - Goes away when WOW is killed

--*/

DWORD W32HungAppNotifyThread(UINT htaskKill)
{
    PTD ptd;
    LPWORD pLockTDB;
    WORD  hTask16;
    DWORD dwThreadId;
    int nMsgBoxChoice;
    PTDB pTDB;
    char    szModName[9];
    char    szErrorMessage[200];
    DWORD   dwResult;
    BOOL    fSuccess;


    if (!ResetEvent(ghevWaitHungAppNotifyThread)) {
         LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: ERROR failed to ResetEvent\n"));
    }

    ptd = NULL;

    if (htaskKill) {

        EnterCriticalSection(&gcsWOW);

        ptd = gptdTaskHead;

        /*
         * See if the Task is still alive
         */
        while ((ptd != NULL) && (ptd->htask16 != htaskKill)) {
            ptd = ptd->ptdNext;
        }

        LeaveCriticalSection(&gcsWOW);

    }

    // If we are seeing this notification for a second time, these selectors 
    // probably don't match -- which means this 16-bit context is really messed
    // up.  We'd better prevent it from doing any 16-bit callbacks at this point
    // or it will result in a crash dlg for the user & will kill the VDM & any
    // other 16-bit apps (tasks) running in this VDM.
    // This situation will occur if the call to WaitForSingleObject() (following
    // the call to SendMessageTimeout() below) actually times-out rather than
    // complete.  The thread will actually be dead by the time *this* function
    // gets called again with a now-invalid TDB struct.  This situation appears
    // to result from clicking End Task from the task manager or somehow trying
    // to kill a not-hung 16-bit task via external means.  Bug #408188
    if(HIWORD(ptd->vpStack) != HIWORD(ptd->vpCBStack)) {

#ifdef debug
        // sanity check to make sure this only happens for the same task
        WOW32ASSERTMSG((htaskKill == gLasthtaskKill), 
                       ("WOW: Unexpected mis-matched selector case\n"));
        gLasthtaskKill = 0;
#endif
        return 0;
    }

    // point to LockTDB

    GETVDMPTR(vpLockTDB, 2, pLockTDB);

    // If the task is alive then attempt to kill it

    if ( ( ptd != NULL ) || ( htaskKill == 0 ) ) {

        // Set LockTDB == The app we are trying to kill
        // (see \kernel31\TASKING.ASM)
        // and then try to cause a task switch by posting WOWEXEC a message
        // and then posting a message to the app we want to kill

        if ( ptd != NULL) {
            hTask16 = ptd->htask16;

        }
        else {
            // htaskKill == 0
            // Kill the Active Task
            hTask16 = *pCurTDB;
        }

        pTDB = (PTDB)SEGPTR(hTask16, 0);

        WOW32ASSERTMSGF( pTDB && pTDB->TDB_sig == TDB_SIGNATURE,
                ("W32HungAppNotifyThread: TDB sig doesn't match, TDB %x htaskKill %x pTDB %x.\n",
                 hTask16, htaskKill, pTDB));

        dwThreadId = pTDB->TDB_ThreadID;

        //
        // if the task to be killed is this task end immediately
        //
        if (dwThreadId == GetCurrentThreadId()) {
            EnterCriticalSection(&gcsHungApp);
            *pNtVDMState |= VDM_WOWHUNGAPP;
            LeaveCriticalSection(&gcsHungApp);
            call_ica_hw_interrupt( KEYBOARD_ICA, KEYBOARD_LINE, 1 );

            //
            // return to 16 bit to process int 9.
            //

            return 0;
            }

        *pLockTDB = hTask16;
        SendMessageTimeout(ghwndShell, WM_WOWEXECHEARTBEAT, 0, 0, SMTO_BLOCK,1*1000,&dwResult);

        //
        // terminate any pending named pipe operations for this thread (ie app)
        //

        VrCancelPipeIo(dwThreadId);

        PostThreadMessage(dwThreadId, WM_KEYDOWN, VK_ESCAPE, 0x1B000A);
        PostThreadMessage(dwThreadId,   WM_KEYUP, VK_ESCAPE, 0x1B0001);

        if (WaitForSingleObject(ghevWaitHungAppNotifyThread,
                                CMS_WAITTASKEXIT) == 0) {
            LOGDEBUG(2,("W32HungAppNotifyThread: Success with forced task switch\n"));
            ExitThread(EXIT_SUCCESS);
        }

#ifdef debug
        gLasthtaskKill = htaskKill;
#endif

        // Failed
        //
        // Probably means the current App is looping in 16 bit land not
        // responding to input.

        // Warn the User if its a different App than the one he wants to kill
        // Don't do this if WOWEXEC is the hung app, since users don't know
        // what that is.


        if (*pLockTDB != *pCurTDB && gptdShell->htask16 != *pCurTDB && *pCurTDB) {

            pTDB = (PTDB)SEGPTR(*pCurTDB, 0);

            WOW32ASSERTMSGF( pTDB && pTDB->TDB_sig == TDB_SIGNATURE,
                    ("W32HungAppNotifyThread: Current TDB sig doesn't match, TDB %x htaskKill %x pTDB %x.\n",
                     *pCurTDB, htaskKill, pTDB));

            RtlCopyMemory(szModName, pTDB->TDB_ModName, (sizeof szModName)-1);
            szModName[(sizeof szModName) - 1] = 0;

            fSuccess = LoadString(
                           hmodWOW32,
                           iszCantEndTask,
                           szMsgBoxText,
                           WARNINGMSGLENGTH
                           );
            WOW32ASSERT(fSuccess);

            fSuccess = LoadString(
                           hmodWOW32,
                           iszApplicationError,
                           szCaption,
                           WARNINGMSGLENGTH
                           );
            WOW32ASSERT(fSuccess);

            wsprintf(
                szErrorMessage,
                szMsgBoxText,
                szModName,
                szModName
                );

            nMsgBoxChoice =
                MessageBox(
                    NULL,
                    szErrorMessage,
                    szCaption,
                    MB_TOPMOST | MB_SETFOREGROUND | MB_TASKMODAL |
                    MB_ICONSTOP | MB_OKCANCEL
                    );

            if (nMsgBoxChoice == IDCANCEL) {
                 ExitThread(0);
            }
        }

        //
        // See code in \mvdm\wow16\drivers\keyboard\keyboard.asm
        // where keyb_int where it handles this interrupt and forces an
        // int 21 function 4c - Exit.  It only does this if VDM_WOWHUNGAPP
        // is turned on in NtVDMState, and it clears that bit.  We wait for
        // the bit to be clear if it's already set, indicating another instance
        // of this thread has already initiated an INT 9 to kill a task.  By
        // waiting we avoid screwing up the count of threads active on the
        // 16-bit side (bThreadsIn16Bit).
        //
        // LATER shouldn't allow user to kill WOWEXEC
        //
        // LATER should enable h/w interrupt before doing this - use 40: area
        // on x86.   On MIPS we'd need to call CPU interface.
        //

        EnterCriticalSection(&gcsHungApp);

        while (*pNtVDMState & VDM_WOWHUNGAPP) {
            LeaveCriticalSection(&gcsHungApp);
            LOGDEBUG(LOG_ALWAYS, ("WOW32 W32HungAppNotifyThread waiting for previous INT 9 to clear before dispatching another.\n"));
            Sleep(1 * 1000);
            EnterCriticalSection(&gcsHungApp);
        }

        *pNtVDMState |= VDM_WOWHUNGAPP;

        LeaveCriticalSection(&gcsHungApp);

        call_ica_hw_interrupt( KEYBOARD_ICA, KEYBOARD_LINE, 1 );

        if (WaitForSingleObject(ghevWaitHungAppNotifyThread,
                                CMS_WAITTASKEXIT) != 0) {

            LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Error, timeout waiting for task to terminate\n"));

            fSuccess = LoadString(
                           hmodWOW32,
                           iszUnableToEndSelTask,
                           szMsgBoxText,
                           WARNINGMSGLENGTH);
            WOW32ASSERT(fSuccess);

            fSuccess = LoadString(
                           hmodWOW32,
                           iszSystemError,
                           szCaption,
                           WARNINGMSGLENGTH);
            WOW32ASSERT(fSuccess);

            nMsgBoxChoice =
                MessageBox(
                    NULL,
                    szMsgBoxText,
                    szCaption,
                    MB_TOPMOST | MB_SETFOREGROUND | MB_TASKMODAL |
                    MB_ICONSTOP | MB_OKCANCEL | MB_DEFBUTTON1
                    );

            if (nMsgBoxChoice == IDCANCEL) {
                 EnterCriticalSection(&gcsHungApp);
                 *pNtVDMState &= ~VDM_WOWHUNGAPP;
                 LeaveCriticalSection(&gcsHungApp);
                 ExitThread(0);
            }

            LOGDEBUG(LOG_ALWAYS, ("W32HungAppNotifyThread: Destroying WOW Process\n"));

            ExitVDM(WOWVDM, ALL_TASKS);
            ExitProcess(0);
        }

        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Success with Keyboard Interrupt\n"));

    } else { // task not found

        LOGDEBUG(LOG_ALWAYS,("W32HungAppNotifyThread: Task already Terminated \n"));

    }

    ExitThread(EXIT_SUCCESS);
    return 0;   // remove compiler warning
}



/*++

 W32EndTask - Cause Current Task to Exit (HUNG APP SUPPORT)

 Routine Description:
    This routine is called when unthunking WM_ENDSESSION to cause the current
    task to terminate.

 ENTRY
    The apps thread that we want to kill

 EXIT
  DOES NOT RETURN - The task will exit and wind up in WK32WOWKillTask which
  will cause that thread to Exit.

--*/

VOID APIENTRY W32EndTask(VOID)
{
    PARM16 Parm16;
    VPVOID vp = 0;

    LOGDEBUG(LOG_WARNING,("W32EndTask: Forcing Task %04X to Exit\n",CURRENTPTD()->htask16));

    CallBack16(RET_FORCETASKEXIT, &Parm16, 0, &vp);

    //
    //  We should Never Come Here, an app should get terminated via calling wk32wowkilltask thunk
    //  not by doing an unsimulate call
    //

    WOW32ASSERTMSG(FALSE, "W32EndTask: Error - Returned From ForceTaskExit callback - contact DaveHart");
}


ULONG FASTCALL WK32DirectedYield(PVDMFRAME pFrame)
{
    register PDIRECTEDYIELD16 parg16;

    //
    // This code is duplicated in wkgthunk.c by WOWDirectedYield16.
    // The two must be kept synchronized.
    //

    GETARGPTR(pFrame, sizeof(DIRECTEDYIELD16), parg16);


    BlockWOWIdle(TRUE);

    (pfnOut.pfnDirectedYield)(THREADID32(parg16->hTask16));

    BlockWOWIdle(FALSE);


    FREEARGPTR(parg16);
    RETURN(0);
}

/***************************************************************************\
* EnablePrivilege
*
* Enables/disables the specified well-known privilege in the current thread
* token if there is one, otherwise the current process token.
*
* Returns TRUE on success, FALSE on failure
*
* History:
* 12-05-91 Davidc       Created
* 06-15-93 BobDay       Stolen from WinLogon
\***************************************************************************/
BOOL
EnablePrivilege(
    ULONG Privilege,
    BOOL Enable
    )
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(Privilege,
                                (BOOLEAN)Enable,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(Privilege,
                                    (BOOLEAN)Enable,
                                    FALSE,
                                    &WasEnabled);
    }


    if (!NT_SUCCESS(Status)) {
        LOGDEBUG(LOG_ALWAYS,("WOW32: EnablePrivilege Failed to %s privilege : 0x%lx, status = 0x%lx\n", Enable ? "enable" : "disable", Privilege, Status));
        return(FALSE);
    }

    return(TRUE);
}

//*****************************************************************************
// W32GetAppCompatFlags -
//    Returns the Compatibility flags for the Current Task or of the
//    specified Task.
//    These are the 16-bit kernel's compatibility flags, not to be
//    confused with our separate WOW compatibility flags.
//
//*****************************************************************************

ULONG W32GetAppCompatFlags(HTASK16 hTask16)
{

    PTDB ptdb;

    if (hTask16 == (HAND16)NULL) {
        hTask16 = CURRENTPTD()->htask16;
    }

    ptdb = (PTDB)SEGPTR((hTask16),0);

    return (ULONG)MAKELONG(ptdb->TDB_CompatFlags, ptdb->TDB_CompatFlags2);
}


//*****************************************************************************
// W32ReadWOWCompatFlags -
//
//    Returns the WOW-specific compatibility flags for the specified task.
//    Called during thread initialization to set td.dwWOWCompatFlags.
//    These are not to be confused with the 16-bit kernel's compatibility
//    flags.
//
//    Flag values are defined in wow32.h.
//
//*****************************************************************************

ULONG W32ReadWOWCompatFlags(HTASK16 htask16, DWORD *pdwWOWCompatFlagsEx)
{
    LONG lError;
    HKEY hKey = 0;
    char szModName[9];
    char szHexAsciiFlags[22];
    DWORD dwType = REG_SZ;
    DWORD cbData = sizeof(szHexAsciiFlags);
    ULONG ul = 0;
    char *pch;

    *pdwWOWCompatFlagsEx = 0;

    lError = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\Compatibility",
        0,
        KEY_QUERY_VALUE,
        &hKey
        );

    if (ERROR_SUCCESS != lError) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags: RegOpenKeyEx failed, error %ld.\n", lError));
        goto Cleanup;
    }

    //
    // Fetch the EXE's module name into szModName, trimming trailing blanks.
    //

    RtlCopyMemory(
        szModName,
        ((PTDB)SEGPTR(CURRENTPTD()->htask16,0))->TDB_ModName,
        8
        );
    szModName[8] = 0;

    pch = &szModName[8];
    while (*(--pch) == ' ') {
        *pch = 0;
    }

    lError = RegQueryValueEx(
        hKey,
        szModName,
        0,
        &dwType,
        szHexAsciiFlags,
        &cbData
        );

    if (ERROR_SUCCESS != lError) {

        //
        // This module name doesn't have any compatibility flags.
        //

        goto Cleanup;
    }

    WOW32ASSERTMSGF(REG_SZ == dwType, ("W32ReadWOWCompatFlags(%s): RegQueryValueEx returned type %lx, must be REG_SZ.\n", szModName, dwType));
    if (REG_SZ != dwType) {
        goto Cleanup;
    }

    if (!(pch = WOW32_strstr(szHexAsciiFlags, "0x"))) {
        goto BadFormat;
    }
    pch += 2;  // skip "0x"

    if (!NT_SUCCESS(RtlCharToInteger(pch, 16, &ul))) {
        goto BadFormat;
    }

    if (pch = WOW32_strstr(pch, " 0x")) {
        pch += 3;  // skip " 0x"

        if (!NT_SUCCESS(RtlCharToInteger(pch, 16, pdwWOWCompatFlagsEx))) {
            goto BadFormat;
        }
    }

    LOGDEBUG(0,("WOW: Compatibility flags for %s are %08x %08x\n", szModName, ul, *pdwWOWCompatFlagsEx));

    goto Cleanup;

BadFormat:
    LOGDEBUG(0,("W32ReadWOWCompatFlags(%s): Unable to interpret '%s' as hex.\n", szModName, szHexAsciiFlags));

Cleanup:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return ul;
}

#ifdef FE_SB
// We need compatibility flags for DBCS BIT,
//*****************************************************************************
// W32ReadWOWCompatFlags2 -
//
//    Returns the WOW-specific compatibility flags for the specified task.
//    Called during thread initialization to set td.dwWOWCompatFlags2.
//    These are not to be confused with the 16-bit kernel's compatibility
//    flags.
//
//    Flag values are defined in wow32.h.
//
//*****************************************************************************

ULONG W32ReadWOWCompatFlags2(HTASK16 htask16)
#if 0
// if need more than 32 BIT, this use.
// JUST RESERVED
ULONG W32ReadWOWCompatFlags2(HTASK16 htask16, DWORD *pdwWOWCompatFlagsEx)
#endif
{
    LONG lError;
    HKEY hKey = 0;
    char szModName[9];
    char szHexAsciiFlags[12];
    DWORD dwType = REG_SZ;
    DWORD cbData = sizeof(szHexAsciiFlags);
    ULONG ul = 0;
#if 0
// if need more than 32 BIT, this use.
// JUST RESERVED
    ULONG ulRetCode;

    *pdwWOWCompatFlagsEx = 0;
#endif

    lError = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE,
        "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\Compatibility2",
        0,
        KEY_QUERY_VALUE,
        &hKey
        );

    if (ERROR_SUCCESS != lError) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags2: RegOpenKeyEx failed, error %ld.\n", lError));
        goto Cleanup;
    }

    RtlCopyMemory(
        szModName,
        ((PTDB)SEGPTR(CURRENTPTD()->htask16,0))->TDB_ModName,
        8
        );
    szModName[8] = 0;

    lError = RegQueryValueEx(
        hKey,
        szModName,
        0,
        &dwType,
        szHexAsciiFlags,
        &cbData
        );

    if (ERROR_SUCCESS != lError) {

        //
        // This module name doesn't have any compatibility2 flags.
        //

        goto Cleanup;
    }

    if (REG_SZ != dwType) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags2: RegQueryValueEx returned type %lx, must be REG_SZ.\n", dwType));
        WOW32ASSERT(FALSE);
        goto Cleanup;
    }

    //
    // Force the string to lowercase for the convenience of sscanf.
    //

    WOW32_strlwr(szHexAsciiFlags);

    //
    // sscanf() returns the number of fields converted.
    // We should change to the much smaller and faster strtoul(sz, NULL, 16)
    // if/when it becomes available in our runtimes.
    //

    if (1 != sscanf(szHexAsciiFlags, "0x%lx", &ul)) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags2: Unable to interpret '%s' as hex.\n", szHexAsciiFlags));
        goto Cleanup;
    }
#if 0
// if need more than 32 BIT, this use.
// JUST RESERVED
    ulRetCode = sscanf(szHexAsciiFlags, "0x%lx 0x%lx", &ul, pdwWOWCompatFlagsEx);
    if ((1 != ulRetCode) && (2 != ulRetCode)) {
        LOGDEBUG(0,("W32ReadWOWCompatFlags2: Unable to interpret '%s' as hex.\n", szHexAsciiFlags));
        goto Cleanup;
    }


    LOGDEBUG(0,("WOW: Compatibility flags DBCS for %s are %08x %08x\n", szModName, ul,*pdwWOWCompatFlagsEx));
#endif

    LOGDEBUG(0,("WOW: Compatibility flags DBCS for %s are %08x\n", szModName, ul));

Cleanup:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return ul;
}
#endif // FE_SB

//*****************************************************************************
// This is called from COMM.drv via WowCloseComPort in kernel16, whenever
// a com port needs to be released.
//
// PortId 0 is COM1, 1 is COM2 etc.
//                                                                   - Nanduri
//*****************************************************************************

ULONG FASTCALL WK32WowCloseComPort(PVDMFRAME pFrame)
{
    register PWOWCLOSECOMPORT16 parg16;

    GETARGPTR(pFrame, sizeof(WOWCLOSECOMPORT16), parg16);
    host_com_close((INT)parg16->wPortId);
    FREEARGPTR(parg16);
    return 0;  // quiet compiler, not used.
}


//*****************************************************************************
// WK32WowDelFile
// The call to demFileDelete will handle the case where there there is an
// open handle to the file. In case it fails, we try hacking around the case
// where a font file being held by GDI32.
//*****************************************************************************

DWORD FASTCALL WK32WowDelFile(PVDMFRAME pFrame)
{
    PSZ psz1;
    PWOWDELFILE16 parg16;
    DWORD retval;

    GETARGPTR(pFrame, sizeof(WOWFILEDEL16), parg16);
    GETVDMPTR(parg16->lpFile, 1, psz1);

    LOGDEBUG(fileoclevel,("WK32WOWDelFile: %s \n",psz1));

    retval = demFileDelete(psz1);

    switch(retval) {
        case 0:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            break;

        default:
            // Some Windows Install Programs copy a .FON font file to a temp
            // directory use the font during installation and then try to delete
            // the font - without calling RemoveFontResource();   GDI32 Keeps the
            // Font file open and thus the delete fails.

            // What we attempt here is to assume that the file is a FONT file
            // and try to remove it before deleting it, since the above delete
            // has already failed.

            if ( RemoveFontResourceOem(psz1) ) {
                LOGDEBUG(fileoclevel,("WK32WOWDelFile: RemoveFontResource on %s \n",psz1));
                SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
            }

            if(DeleteFileOem(psz1)) {
                retval = 0;
            }
    }

    if ( retval ) {
        retval |= 0xffff0000;
    }

    FREEVDMPTR(psz1);
    FREEARGPTR(parg16);
    return retval;
}


//*****************************************************************************
// This is called as soon as wow is initialized to notify the 32-bit world
// what the addresses are of some key kernel variables.
//
//*****************************************************************************

ULONG FASTCALL WK32WOWNotifyWOW32(PVDMFRAME pFrame)
{
    register PWOWNOTIFYWOW3216 parg16;

    GETARGPTR(pFrame, sizeof(WOWNOTIFYWOW3216), parg16);

    vpDebugWOW  = FETCHDWORD(parg16->lpDebugWOW);
    GETVDMPTR(FETCHDWORD(parg16->lpcurTDB), 2, pCurTDB);
    vpnum_tasks = FETCHDWORD(parg16->lpnum_tasks);
    vpLockTDB   = FETCHDWORD(parg16->lpLockTDB);
    vptopPDB    = FETCHDWORD(parg16->lptopPDB);
    GETVDMPTR(FETCHDWORD(parg16->lpCurDirOwner), 2, pCurDirOwner);

    //
    // IsDebuggerAttached will tell the 16-bit kernel to generate
    // debug events.
    //
    IsDebuggerAttached();

    FREEARGPTR(parg16);

    return 0;
}

//*****************************************************************************
// Currently, this routine is called very very soon after the 16-bit kernel.exe
// has switched to protected mode. The variables set up here are used in the
// file i/o routines.
//*****************************************************************************

extern VOID demWOWLFNInit(PWOWLFNINIT pLFNInit);
extern VOID DosWowUpdateTDBDir(UCHAR Drive, LPSTR pszDir);
extern BOOL DosWowGetTDBDir(UCHAR Drive, LPSTR pCurrentDirectory);
extern BOOL DosWowDoDirectHDPopup(VOID);

//
//  Function returns TRUE if we should do the popup
//  and FALSE if we should not
//

ULONG FASTCALL WK32DosWowInit(PVDMFRAME pFrame)
{
    register PWOWDOSWOWINIT16 parg16;
    PDOSWOWDATA pDosWowData;
    PULONG  pTemp;
    WOWLFNINIT LFNInit;

    GETARGPTR(pFrame, sizeof(WOWDOSWOWINIT16), parg16);

    // covert all fixed DOS address to linear addresses for fast WOW thunks.
    pDosWowData = GetRModeVDMPointer(FETCHDWORD(parg16->lpDosWowData));

    DosWowData.lpCDSCount = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCDSCount));
    pTemp = (PULONG)GetRModeVDMPointer(FETCHDWORD(pDosWowData->lpCDSFixedTable));
    DosWowData.lpCDSFixedTable = (DWORD) GetRModeVDMPointer(FETCHDWORD(*pTemp));

    DosWowData.lpCDSBuffer = (DWORD)GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCDSBuffer));
    DosWowData.lpCurDrv = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCurDrv));
    DosWowData.lpCurPDB = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpCurPDB));
    DosWowData.lpDrvErr = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpDrvErr));
    DosWowData.lpExterrLocus = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpExterrLocus));
    DosWowData.lpSCS_ToSync = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpSCS_ToSync));
    DosWowData.lpSftAddr = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpSftAddr));
    DosWowData.lpExterr = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpExterr));
    DosWowData.lpExterrActionClass = (DWORD) GetRModeVDMPointer(
                                        FETCHDWORD(pDosWowData->lpExterrActionClass));

/*    // right here we shall make a dynamic check to see if wow is running on
    // Winterm Server and if so -- whether we need to thunk GetWindowsDirectory
    {
        PDWORD UNALIGNED pdwWinTermFlags;
        GETVDMPTR(FETCHDWORD(parg16->lpdwWinTermFlags), sizeof(DWORD), pdwWinTermFlags);

        if (IsTerminalServer()) {
           *pdwWinTermFlags |= WINTERM_SERVER;
        }


    }
*/



    // excellent chance to have us let ntvdm know we're lfn aware and alive

    LFNInit.pDosWowUpdateTDBDir    = DosWowUpdateTDBDir;
    LFNInit.pDosWowGetTDBDir       = DosWowGetTDBDir;
    LFNInit.pDosWowDoDirectHDPopup = DosWowDoDirectHDPopup;

    demWOWLFNInit(&LFNInit);

    FREEARGPTR(parg16);
    return (0);
}


//*****************************************************************************
//
// WK32InitWowIsKnownDLL(HANDLE hKeyWow)
//
// Called by W32Init to read list of known DLLs from the registry.
//
// hKeyWow is an open handle to ...\CurrentControlSet\WOW, we use
// the value REG_SZ value KnownDLLs which looks like "commdlg.dll mmsystem.dll
// toolhelp.dll olecli.dll olesvr.dll".
//
//*****************************************************************************

VOID WK32InitWowIsKnownDLL(HANDLE hKeyWow)
{
    CHAR  sz[2048];
    PSZ   pszKnownDLL;
    PCHAR pch;
    ULONG ulSize = sizeof(sz);
    int   nCount;
    DWORD dwRegValueType;
    LONG  lRegError;
    ULONG ulAttrib;

    //
    // Get the list of known DLLs from the registry.
    //

    lRegError = RegQueryValueEx(
                    hKeyWow,
                    "KnownDLLs",
                    NULL,
                    &dwRegValueType,
                    sz,
                    &ulSize
                    );

    if (ERROR_SUCCESS == lRegError && REG_SZ == dwRegValueType) {

        //
        // Allocate memory to hold a copy of this string to be
        // used to hold the strings pointed to by
        // apszKnownDLL[].  This memory won't be freed until
        // WOW goes away.
        //

        pszKnownDLL = malloc_w_or_die(ulSize);

        strcpy(pszKnownDLL, sz);

        //
        // Lowercase the entire value so that we can search these
        // strings case-sensitive in WK32WowIsKnownDLL.
        //

        WOW32_strlwr(pszKnownDLL);

        //
        // Parse the KnownDLL string into apszKnownDLL array.
        // strtok() does this quite handily.
        //

        nCount = 0;

        pch = apszKnownDLL[0] = pszKnownDLL;

        while (apszKnownDLL[nCount]) {
            nCount++;
            if (nCount >= MAX_KNOWN_DLLS) {
                LOGDEBUG(0,("WOW32 Init: Too many known DLLs, must have %d or fewer.\n", MAX_KNOWN_DLLS-1));
                apszKnownDLL[MAX_KNOWN_DLLS-1] = NULL;
                break;
            }
            pch = WOW32_strchr(pch, ' ');
            if (!pch) {
                break;
            }
            *pch = 0;
            pch++;
            if (0 == *pch) {
                break;
            }
            while (' ' == *pch) {
                pch++;
            }
            apszKnownDLL[nCount] = pch;
        }

    } else {
        LOGDEBUG(0,("InitWowIsKnownDLL: RegQueryValueEx error %ld.\n", lRegError));
    }

    //
    // The Known DLL list is ready, now build up a fully-qualified paths
    // to %windir%\control.exe and %windir%\system32\control.exe
    // for WOWCF_CONTROLEXEHACK below.
    //

    //
    // pszControlExeWinDirPath looks like "c:\winnt\control.exe"
    //

    pszControlExeWinDirPath =
        malloc_w_or_die(strlen(pszWindowsDirectory)     +
                        sizeof(szBackslashControlExe)-1 + // strlen("\\control.exe")
                        1                                 // null terminator
                        );

    strcpy(pszControlExeWinDirPath, pszWindowsDirectory);
    strcat(pszControlExeWinDirPath, szBackslashControlExe);


    //
    // pszProgmanExeWinDirPath looks like "c:\winnt\progman.exe"
    //

    pszProgmanExeWinDirPath =
        malloc_w_or_die(strlen(pszWindowsDirectory)     +
                        sizeof(szBackslashProgmanExe)-1 + // strlen("\\progman.exe")
                        1                                 // null terminator
                        );

    strcpy(pszProgmanExeWinDirPath, pszWindowsDirectory);
    strcat(pszProgmanExeWinDirPath, szBackslashProgmanExe);


    //
    // pszControlExeSysDirPath looks like "c:\winnt\system32\control.exe"
    //

    pszControlExeSysDirPath =
        malloc_w_or_die(strlen(pszSystemDirectory)      +
                        sizeof(szBackslashControlExe)-1 + // strlen("\\control.exe")
                        1                                 // null terminator
                        );

    strcpy(pszControlExeSysDirPath, pszSystemDirectory);
    strcat(pszControlExeSysDirPath, szBackslashControlExe);

    //
    // pszProgmanExeSysDirPath looks like "c:\winnt\system32\control.exe"
    //

    pszProgmanExeSysDirPath =
        malloc_w_or_die(strlen(pszSystemDirectory)      +
                        sizeof(szBackslashProgmanExe)-1 + // strlen("\\progman.exe")
                        1                                 // null terminator
                        );

    strcpy(pszProgmanExeSysDirPath, pszSystemDirectory);
    strcat(pszProgmanExeSysDirPath, szBackslashProgmanExe);

    // Make the KnownDLL, CTL3DV2.DLL, file attribute ReadOnly.
    // Later we should do this for all WOW KnownDll's
    strcpy(sz, pszSystemDirectory);
    strcat(sz, "\\CTL3DV2.DLL");
    ulAttrib = GetFileAttributesOem(sz);
    if ((ulAttrib != 0xFFFFFFFF) && !(ulAttrib & FILE_ATTRIBUTE_READONLY)) {
        ulAttrib |= FILE_ATTRIBUTE_READONLY;
        SetFileAttributesOem(sz, ulAttrib);
    }

}


//*****************************************************************************
//
// WK32WowIsKnownDLL -
//
// This routine is called from within LoadModule (actually MyOpenFile),
// when kernel31 has determined that the module is not already loaded,
// and is about to search for the DLL.  If the base name of the passed
// path is a known DLL, we allocate and pass back to the 16-bit side
// a fully-qualified path to the DLL in the system32 directory.
//
//*****************************************************************************

ULONG FASTCALL WK32WowIsKnownDLL(PVDMFRAME pFrame)
{
    register WOWISKNOWNDLL16 *parg16;
    PSZ pszPath;
    VPVOID UNALIGNED *pvpszKnownDLLPath;
    PSZ pszKnownDLLPath;
    size_t cbKnownDLLPath;
    char **ppsz;
    char szLowercasePath[13];
    ULONG ul = 0;
    BOOL fProgman = FALSE;

    GETARGPTR(pFrame, sizeof(WOWISKNOWNDLL16), parg16);

    GETPSZPTRNOLOG(parg16->lpszPath, pszPath);
    GETVDMPTR(parg16->lplpszKnownDLLPath, sizeof(*pvpszKnownDLLPath), pvpszKnownDLLPath);

    if (pszPath) {

        //
        // Special hack for apps which WinExec %windir%\control.exe or
        // %windir%\progman.exe.  This formerly was only done under a
        // compatibility bit, but now is done for all apps.  Both
        // the 3.1[1] control panel and program manager binaries cannot
        // work under WOW because of other shell conflicts, like different
        // .GRP files and conflicting use of the control.ini file for both
        // 16-bit and 32-bit CPLs.
        //
        // Compare the path passed in with the precomputed
        // pszControlExeWinDirPath, which looks like "c:\winnt\control.exe".
        // If it matches, pass back the "Known DLL path" of
        // "c:\winnt\system32\control.exe".  Same for progman.exe.
        //

        if (!WOW32_stricmp(pszPath, pszControlExeWinDirPath) ||
            (fProgman = TRUE,
             !WOW32_stricmp(pszPath, pszProgmanExeWinDirPath))) {

            VPVOID vp;

            cbKnownDLLPath = 1 + strlen(fProgman
                                         ? pszProgmanExeSysDirPath
                                         : pszControlExeSysDirPath);

            vp = malloc16(cbKnownDLLPath);

            // 16-bit memory may have moved - refresh flat pointers now

            FREEVDMPTR(pvpszKnownDLLPath);
            FREEPSZPTR(pszPath);
            FREEARGPTR(parg16);
            FREEVDMPTR(pFrame);
            GETFRAMEPTR(((PTD)CURRENTPTD())->vpStack, pFrame);
            GETARGPTR(pFrame, sizeof(WOWISKNOWNDLL16), parg16);
            GETPSZPTRNOLOG(parg16->lpszPath, pszPath);
            GETVDMPTR(parg16->lplpszKnownDLLPath, sizeof(*pvpszKnownDLLPath), pvpszKnownDLLPath);

            *pvpszKnownDLLPath = vp;

            if (*pvpszKnownDLLPath) {

                GETPSZPTRNOLOG(*pvpszKnownDLLPath, pszKnownDLLPath);

                RtlCopyMemory(
                   pszKnownDLLPath,
                   fProgman
                    ? pszProgmanExeSysDirPath
                    : pszControlExeSysDirPath,
                   cbKnownDLLPath);

                // LOGDEBUG(0,("WowIsKnownDLL: %s known(c) -=> %s\n", pszPath, pszKnownDLLPath));

                FLUSHVDMPTR(*pvpszKnownDLLPath, cbKnownDLLPath, pszKnownDLLPath);
                FREEPSZPTR(pszKnownDLLPath);

                ul = 1;          // return success, meaning is known dll
                goto Cleanup;
            }
        }

        //
        // We don't mess with attempts to open that include a
        // path.
        //

        if (WOW32_strchr(pszPath, '\\') || WOW32_strchr(pszPath, ':') || strlen(pszPath) > 12) {
            // LOGDEBUG(0,("WowIsKnownDLL: %s has a path, not checking.\n", pszPath));
            goto Cleanup;
        }

        //
        // Make a lowercase copy of the path.
        //

        WOW32_strncpy(szLowercasePath, pszPath, sizeof(szLowercasePath));
        szLowercasePath[sizeof(szLowercasePath)-1] = 0;
        WOW32_strlwr(szLowercasePath);


        //
        // Step through apszKnownDLL trying to find this DLL
        // in the list.
        //

        for (ppsz = &apszKnownDLL[0]; *ppsz; ppsz++) {

            //
            // We compare case-sensitive for speed, since we're
            // careful to lowercase the strings in apszKnownDLL
            // and szLowercasePath.
            //

            if (!WOW32_strcmp(szLowercasePath, *ppsz)) {

                //
                // We found the DLL in the list, now build up
                // a buffer for the 16-bit side containing
                // the full path to that DLL in the system32
                // directory.
                //

                cbKnownDLLPath = strlen(pszSystemDirectory) +
                                 1 +                     // "\"
                                 strlen(szLowercasePath) +
                                 1;                      // null

                *pvpszKnownDLLPath = malloc16(cbKnownDLLPath);

                if (*pvpszKnownDLLPath) {
#ifndef _X86_
                    HANDLE hFile;
#endif

                    GETPSZPTRNOLOG(*pvpszKnownDLLPath, pszKnownDLLPath);

#ifndef _X86_
                    // On RISC platforms, wx86 support tells 32-bit apps that
                    // the system dir is SYS32X86 instead of SYSTEM32.  This 
                    // allows us to keep the x86 binaries separate from the 
                    // native RISC binaries in the SYSTEM32 dir.  It also 
                    // prevents x86 setup programs from clobbering the native 
                    // RISC binaries & replacing them with an x86 version in the
                    // SYSTEM32 dir.  Unfortunately several "32-bit" programs 
                    // have 16-bit components (Most notably Outlook forms 
                    // support).  These 16-bit components will also be copied to
                    // the SYS32X86 dir.  This is not a problem unless the 
                    // binary shows up in our KnownDLLs list. This code attempts
                    // to locate KnownDLLs in the SYS32X86 dir on RISC machines 
                    // before looking in the SYSTEM32 dir. See bug #321335.
                    strcpy(pszKnownDLLPath, pszWindowsDirectory);
                    strcat(pszKnownDLLPath, "\\SYS32X86\\");
                    strcat(pszKnownDLLPath, szLowercasePath);

                    // see if this knowndll exists in the sys32x86 dir
                    hFile = CreateFile(pszKnownDLLPath,
                                       GENERIC_READ,
                                       FILE_SHARE_READ,
                                       NULL,
                                       OPEN_EXISTING,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

                    if(hFile != INVALID_HANDLE_VALUE) {

                         CloseHandle(hFile);

                         //Yep, that's what we'll go with
                         LOGDEBUG(0,("WowIsKnownDLL: %s known -=> %s\n", 
                                    pszPath, 
                                    pszKnownDLLPath));
                         FLUSHVDMPTR(*pvpszKnownDLLPath, 
                                     cbKnownDLLPath, 
                                     pszKnownDLLPath);
                         FREEPSZPTR(pszKnownDLLPath);
     
                         ul = 1;  // return success, meaning is known dll
                         goto Cleanup;
                    }
                    // otherwise we just fall through & use system32 dir
#endif  // ifndef _X86_

                    strcpy(pszKnownDLLPath, pszSystemDirectory);
                    strcat(pszKnownDLLPath, "\\");
                    strcat(pszKnownDLLPath, szLowercasePath);

                    // LOGDEBUG(0,("WowIsKnownDLL: %s known -=> %s\n", pszPath, pszKnownDLLPath));

                    FLUSHVDMPTR(*pvpszKnownDLLPath, cbKnownDLLPath, pszKnownDLLPath);
                    FREEPSZPTR(pszKnownDLLPath);

                    ul = 1;          // return success, meaning is known dll
                    goto Cleanup;
                }
            }
        }

        //
        // We've checked the Known DLL list and come up empty, or
        // malloc16 failed.
        //

        // LOGDEBUG(0,("WowIsKnownDLL: %s is not a known DLL.\n", szLowercasePath));

    } else {

        //
        // pszPath is NULL, so free the 16-bit buffer pointed
        // to by *pvpszKnownDLLPath.
        //

        if (*pvpszKnownDLLPath) {
            free16(*pvpszKnownDLLPath);
            ul = 1;
        }
    }

  Cleanup:
    FLUSHVDMPTR(parg16->lplpszKnownDLLPath, sizeof(*pvpszKnownDLLPath), pvpszKnownDLLPath);
    FREEVDMPTR(pvpszKnownDLLPath);
    FREEPSZPTR(pszPath);
    FREEARGPTR(parg16);

    return ul;
}


VOID RemoveHmodFromCache(HAND16 hmod16)
{
    INT i;

    //
    // blow this guy out of the hinst/hmod cache
    // if we find it, slide the other entries up to overwrite it
    // and then zero out the last entry
    //

    for (i = 0; i < CHMODCACHE; i++) {
        if (ghModCache[i].hMod16 == hmod16) {

            // if we're not at the last entry, slide the rest up 1

            if (i != CHMODCACHE-1) {
                RtlMoveMemory((PVOID)(ghModCache+i),
                              (CONST VOID *)(ghModCache+i+1),
                              sizeof(HMODCACHE)*(CHMODCACHE-i-1) );
            }

            // the last entry is now either a dup or the one going away

            ghModCache[CHMODCACHE-1].hMod16 =
            ghModCache[CHMODCACHE-1].hInst16 = 0;
        }
    }
}

//
// Scans the share memory segment for wow processes which might have
// been killed and removes them.
//

VOID
CleanseSharedList(
    VOID
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDMEMOBJECT   lpsmo;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDPROCESS     lpspPrev;
    HANDLE              hProcess;
    DWORD               dwOffset;

    lpstm = LOCKSHAREWOW();
    if ( !lpstm ) {
        LOGDEBUG(0,("WOW32: CleanseSharedList failed to map in shared wow memory\n") );
        return;
    }

    if ( !lpstm->fInitialized ) {
        lpstm->fInitialized = TRUE;
        lpstm->dwFirstProcess = 0;
    }

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpstm + sizeof(SHAREDTASKMEM));

    lpspPrev = NULL;
    dwOffset = lpstm->dwFirstProcess;

    while( dwOffset ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwOffset);

        WOW32ASSERT(lpsp->dwType == SMO_PROCESS);

        // Test this process to see if he is still around.

        hProcess = OpenProcess( SYNCHRONIZE, FALSE, lpsp->dwProcessId );
        if ( hProcess == NULL ) {
           // cleanup tasks for this process first
           LPSHAREDTASK lpst;
           DWORD dwTaskOffset;

           dwTaskOffset = lpsp->dwFirstTask; // this is an offset of the first task
           while (dwTaskOffset) {

              lpst = (LPSHAREDTASK)((CHAR*)lpstm + dwTaskOffset);
              // log this removal
              LOGDEBUG(0, ("CleanseSharedList: Forceful removal of a dead task %s\n", lpst->szFilePath));

              dwTaskOffset = lpst->dwNextTask;
              lpst->dwType = SMO_AVAILABLE;
           }

           if ( lpspPrev ) {
              lpspPrev->dwNextProcess = lpsp->dwNextProcess;
           } else {
               lpstm->dwFirstProcess = lpsp->dwNextProcess;
           }
           lpsp->dwType = SMO_AVAILABLE;

        } else {
           CloseHandle( hProcess );
           lpspPrev = lpsp;        // only update lpspPrev if lpsp is valid
        }
        dwOffset = lpsp->dwNextProcess;
    }

    UNLOCKSHAREWOW();
}

//
// Add this process to the shared memory list of wow processes
//
VOID
AddProcessSharedList(
    VOID
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDMEMOBJECT   lpsmo;
    LPSHAREDPROCESS     lpsp;
    DWORD               dwResult;
    INT                 count;

    lpstm = LOCKSHAREWOW();
    if ( !lpstm ) {
        LOGDEBUG(0,("WOW32: AddProcessSharedList failed to map in shared wow memory\n") );
        return;
    }

    // Scan for available slot
    count = 0;
    dwResult = 0;

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpstm + sizeof(SHAREDTASKMEM));

    while ( count < MAX_SHARED_OBJECTS ) {
        if ( lpsmo->dwType == SMO_AVAILABLE ) {
            lpsp = (LPSHAREDPROCESS)lpsmo;
            dwResult = (DWORD)((CHAR *)lpsp - (CHAR *)lpstm);
            lpsp->dwType          = SMO_PROCESS;
            lpsp->dwProcessId     = GetCurrentProcessId();
            lpsp->dwAttributes    = fSeparateWow ? 0 : WOW_SYSTEM;
            lpsp->pfnW32HungAppNotifyThread = (LPTHREAD_START_ROUTINE) W32HungAppNotifyThread;
            lpsp->dwNextProcess   = lpstm->dwFirstProcess;
            lpsp->dwFirstTask     = 0;
            lpstm->dwFirstProcess = dwResult;
            break;
        }
        lpsmo++;
        count++;
    }
    if ( count == MAX_SHARED_OBJECTS ) {
        LOGDEBUG(0, ("WOW32: AddProcessSharedList: Not enough room in WOW's Shared Memory\n") );
    }
    UNLOCKSHAREWOW();

    dwSharedProcessOffset = dwResult;
}

//
// Remove this process from the shared memory list of wow tasks
//
VOID
RemoveProcessSharedList(
    VOID
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDPROCESS     lpspPrev;
    DWORD               dwOffset;
    DWORD               dwCurrentId;

    lpstm = LOCKSHAREWOW();
    if ( !lpstm ) {
        LOGDEBUG(0,("WOW32: RemoveProcessSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpspPrev = NULL;
    dwCurrentId = GetCurrentThreadId();
    dwOffset = lpstm->dwFirstProcess;

    while( dwOffset != 0 ) {
        lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwOffset);
        WOW32ASSERT(lpsp->dwType == SMO_PROCESS);

        // Is this the guy to remove?

        if ( lpsp->dwProcessId == dwCurrentId ) {
           // so we're removing this process from the shared list




            if ( lpspPrev ) {
                lpspPrev->dwNextProcess = lpsp->dwNextProcess;
            } else {
                lpstm->dwFirstProcess = lpsp->dwNextProcess;
            }
            lpsp->dwType = SMO_AVAILABLE;
            break;
        }
        lpspPrev = lpsp;
        dwOffset = lpsp->dwNextProcess;
    }

    UNLOCKSHAREWOW();
}

//
// AddTaskSharedList
//
// Add this thread to the shared memory list of wow tasks.
// If hMod16 is zero, this call is to reserve the given
// htask, another call will come later to really add the
// task entry.
//
// When reserving an htask, a return of 0 means the htask
// is in use in another VDM, a nonzero return means either
// the shared memory is full or couldn't be accessed OR
// the htask was reserved.  This way the return is passed
// directly back to krnl386's task.asm where 0 means try
// again and nonzero means go with it.
//

WORD
AddTaskSharedList(
    HTASK16 hTask16,
    HAND16  hMod16,
    PSZ     pszModName,
    PSZ     pszFilePath
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    LPSHAREDMEMOBJECT   lpsmo;
    INT                 count;
    INT                 len;
    WORD                wRet;

    lpstm = LOCKSHAREWOW();
    if ( !lpstm ) {
        LOGDEBUG(0,("WOW32: AddTaskSharedList failed to map in shared wow memory\n") );
        wRet = hTask16;
        goto Exit;
    }

    lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwSharedProcessOffset);

    //
    // Scan to see if this htask is already in use.
    //

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpstm + sizeof(SHAREDTASKMEM));
    count = 0;
    while ( count < MAX_SHARED_OBJECTS ) {
        if ( lpsmo->dwType == SMO_TASK ) {
            lpst = (LPSHAREDTASK)lpsmo;
            if (lpst->hTask16 == hTask16) {

                //
                // This htask is already in the table, if we're calling to fill in the
                // details that's fine, if we are calling to reserve fail the call,
                //

                if (hMod16) {

                    lpst->dwThreadId     = GetCurrentThreadId();
                    lpst->hMod16         = (WORD)hMod16;

                    strcpy(lpst->szModName, pszModName);

                    len = strlen(pszFilePath);
                    WOW32ASSERTMSGF(len <= (sizeof lpst->szFilePath) - 1,
                                    ("WOW32: too-long EXE path truncated in shared memory: '%s'\n", pszFilePath));
                    len = min(len, (sizeof lpst->szFilePath) - 1);
                    RtlCopyMemory(lpst->szFilePath, pszFilePath, len);
                    lpst->szFilePath[len] = 0;

                    wRet = hTask16;
                } else {
                    wRet = 0;
                }
                goto UnlockExit;
            }
        }
        lpsmo++;
        count++;
    }

    //
    // We didn't find this htask, scan for available slot.
    //

    lpsmo = (LPSHAREDMEMOBJECT)((CHAR *)lpstm + sizeof(SHAREDTASKMEM));
    count = 0;
    while ( count < MAX_SHARED_OBJECTS ) {
        if ( lpsmo->dwType == SMO_AVAILABLE ) {
            lpst = (LPSHAREDTASK)lpsmo;
            lpst->dwType         = SMO_TASK;
            lpst->hTask16        = (WORD)hTask16;
            lpst->dwThreadId     = 0;
            lpst->hMod16         = 0;
            lpst->szModName[0]   = 0;
            lpst->szFilePath[0]  = 0;
            lpst->dwNextTask     = lpsp->dwFirstTask;
            lpsp->dwFirstTask    = (DWORD)((CHAR *)lpst - (CHAR *)lpstm);
            break;
        }
        lpsmo++;
        count++;
    }
    if ( count == MAX_SHARED_OBJECTS ) {
        LOGDEBUG(0, ("WOW32: AddTaskSharedList: Not enough room in WOW's Shared Memory\n") );
    }

    wRet = hTask16;

UnlockExit:
    UNLOCKSHAREWOW();
Exit:
    return wRet;
}


//
// Remove this thread from the shared memory list of wow tasks
//
VOID
RemoveTaskSharedList(
    VOID
) {
    LPSHAREDTASKMEM     lpstm;
    LPSHAREDPROCESS     lpsp;
    LPSHAREDTASK        lpst;
    LPSHAREDTASK        lpstPrev;
    DWORD               dwCurrentId;
    DWORD               dwOffset;

    lpstm = LOCKSHAREWOW();
    if ( !lpstm ) {
        LOGDEBUG(0,("WOW32: RemoveTaskSharedList failed to map in shared wow memory\n") );
        return;
    }

    lpsp = (LPSHAREDPROCESS)((CHAR *)lpstm + dwSharedProcessOffset);

    lpstPrev = NULL;
    dwCurrentId = GetCurrentThreadId();
    dwOffset = lpsp->dwFirstTask;

    while( dwOffset != 0 ) {
        lpst = (LPSHAREDTASK)((CHAR *)lpstm + dwOffset);

        WOW32ASSERT(lpst->dwType == SMO_TASK);

        // Is this the guy to remove?

        if ( lpst->dwThreadId == dwCurrentId ) {
            if ( lpstPrev ) {
                lpstPrev->dwNextTask = lpst->dwNextTask;
            } else {
                lpsp->dwFirstTask = lpst->dwNextTask;
            }
            lpst->dwType = SMO_AVAILABLE;
            break;
        }
        lpstPrev = lpst;
        dwOffset = lpst->dwNextTask;
    }

    UNLOCKSHAREWOW();
}


VOID W32RefreshCurrentDirectories (PCHAR lpszzEnv)
{
LPSTR   lpszVal;
CHAR   chDrive, achEnvDrive[] = "=?:";

    if (lpszzEnv) {
        while(*lpszzEnv) {
            if(*lpszzEnv == '=' &&
                    (chDrive = (CHAR)toupper(*(lpszzEnv+1))) >= 'A' &&
                    chDrive <= 'Z' &&
                    (*(PCHAR)((ULONG)lpszzEnv+2) == ':')) {
                lpszVal = (PCHAR)((ULONG)lpszzEnv + 4);
                achEnvDrive[1] = chDrive;
                SetEnvironmentVariable (achEnvDrive,lpszVal);
            }
            lpszzEnv = WOW32_strchr(lpszzEnv,'\0');
            lpszzEnv++;
        }
        *(PUCHAR)DosWowData.lpSCS_ToSync = (UCHAR)0xff;
    }
}


/* WK32CheckUserGdi - hack routine to support Simcity. See the explanation
 *                    in kernel31\3ginterf.asm routine HackCheck.
 *
 *
 * Entry - pszPath  Full Path of the file in the module table
 *
 * Exit
 *     SUCCESS
 *       1
 *
 *     FAILURE
 *       0
 *
 */

ULONG FASTCALL WK32CheckUserGdi(PVDMFRAME pFrame)
{
    PWOWCHECKUSERGDI16 parg16;
    PSTR    psz;
    CHAR    szPath[MAX_PATH+10];
    UINT    cb;
    ULONG   ul;

    //
    // Get arguments.
    //

    GETARGPTR(pFrame, sizeof(WOWCHECKUSERGDI16), parg16);
    psz = SEGPTR(FETCHWORD(parg16->pszPathSegment),
                     FETCHWORD(parg16->pszPathOffset));

    FREEARGPTR(parg16);

    strcpy(szPath, pszSystemDirectory);
    cb = strlen(szPath);

    strcpy(szPath + cb, "\\GDI.EXE");

    if (WOW32_stricmp(szPath, psz) == 0)
        goto Success;

    strcpy(szPath + cb, "\\USER.EXE");

    if (WOW32_stricmp(szPath, psz) == 0)
        goto Success;

    ul = 0;
    goto Done;

Success:
    ul = 1;

Done:
    return ul;
}



/* WK32ExitKernel - Force the Distruction of the WOW Process
 *                  Formerly known as WK32KillProcess.
 *
 * Called when the 16 bit kernel exits and by KillWOW and
 * checked WOWExec when the user wants to nuke the shared WOW.
 *
 * ENTRY
 *
 *
 * EXIT
 *  Never Returns - The Process Goes Away
 *
 */

ULONG FASTCALL WK32ExitKernel(PVDMFRAME pFrame)
{
    PEXITKERNEL16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    WOW32ASSERTMSGF(
        ! parg16->wExitCode,
        ("\n"
         "WOW ERROR:  ExitKernel(0x%x) called on 16-bit side.\n"
         "==========  Please contact DaveHart or another WOW developer.\n"
         "\n\n",
         parg16->wExitCode
        ));

    ExitVDM(WOWVDM, ALL_TASKS);
    ExitProcess(parg16->wExitCode);

    return 0;   // Never executed, here to avoid compiler warning.
}





/* WK32FatalExit - Called as FatalExitThunk by kernel16 FatalExit
 *
 *
 * parg16->f1 is FatalExit code
 *
 */

ULONG FASTCALL WK32FatalExit(PVDMFRAME pFrame)
{
    PFATALEXIT16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    WOW32ASSERTMSGF(
        FALSE,
        ("\n"
         "WOW ERROR:  FatalExit(0x%x) called by 16-bit WOW kernel.\n"
         "==========  Contact the DOSWOW alias.\n"
         "\n\n",
         FETCHWORD(parg16->f1)
        ));

    // Sometimes we get this with no harm done (app bug)

    ExitVDM(WOWVDM, ALL_TASKS);
    ExitProcess(parg16->f1);

    return 0;   // Never executed, here to avoid compiler warning.
}


//
// WowPartyByNumber is present on checked builds only as a convenience
// to WOW developers who need a quick, temporary thunk for debugging.
// The checked wowexec.exe has a menu item, Party By Number, which
// collects a number and string parameter and calls this thunk.
//

#ifdef DEBUG

#pragma warning (4:4723)        // lower to -W4

ULONG FASTCALL WK32WowPartyByNumber(PVDMFRAME pFrame)
{
    PWOWPARTYBYNUMBER16 parg16;
    PSZ psz;
    ULONG ulRet = 0;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZPTR(parg16->psz, psz);

    switch (parg16->dw) {

        case 0:  // Access violation
            *(char *)0xa0000000 = 0;
            break;

        case 1:  // Stack overflow
            {
                char EatStack[2048];

                strcpy(EatStack, psz);
                WK32WowPartyByNumber(pFrame);
                strcpy(EatStack, psz);
            }
            break;

        case 2:  // Datatype misalignment
            {
                DWORD adw[2];
                PDWORD pdw = (void *)((char *)adw + 2);

                *pdw = (DWORD)-1;

                //
                // On some platforms the above will just work (hardware or
                // emulation), so we force it with RaiseException.
                //
                RaiseException((DWORD)EXCEPTION_DATATYPE_MISALIGNMENT,
                               0, 0, NULL);
            }
            break;

        case 3:  // Integer divide by zero
            ulRet = 1 / (parg16->dw - 3);
            break;

        case 4:  // Other exception
            RaiseException((DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
                           EXCEPTION_NONCONTINUABLE, 0, NULL);
            break;

        case 5:  // gpm16
            //
            // Quick test that WOWGetProcModule16 is working
            //
            {
                char sz[256];

                wsprintf(sz, "GetProcModule16(%lx) == %x\n", gpfn16GetProcModule, WOWGetProcModule16(gpfn16GetProcModule));
                OutputDebugString(sz);
            }
            break;

        case 6:  // Test lstrcmp callback to user16 used by user32
            {
                char szMsg[256];
                WCHAR wsz1[256];
                WCHAR wsz2[256];
                char *s1;
                char *s2;

                s1 = psz;
                s2 = WOW32_strchr(s1, ' ');

                if (s2) {

                    *s2++ = 0;

                    RtlMultiByteToUnicodeN(
                        wsz1,
                        sizeof wsz1,
                        NULL,
                        s1,
                        strlen(s1) + 1
                        );

                    RtlMultiByteToUnicodeN(
                        wsz2,
                        sizeof wsz2,
                        NULL,
                        s2,
                        strlen(s2) + 1
                        );

                    ulRet = WOWlstrcmp16(wsz1, wsz2);

                    wsprintf(szMsg, "WOWlstrcmp16(%ws, %ws) == %d", wsz1, wsz2, ulRet);
                    MessageBox(NULL, szMsg, "WK32WowPartyByNumber", MB_OK | MB_ICONEXCLAMATION);
                } else {
                    MessageBox(NULL, "use two strings separated by a space", "WK32WowPartyByNumber", MB_OK | MB_ICONEXCLAMATION);
                    ulRet = 0;
                }
            }
            break;

        default:
            {
                char szMsg[255];

                wsprintf(szMsg, "WOW Unhandled Party By Number (%d, '%s')", parg16->dw, psz);

                MessageBeep(0);
                MessageBox(NULL, szMsg, "WK32WowPartyByNumber", MB_OK | MB_ICONEXCLAMATION);
            }
    }

    FREEPSZPTR(psz);
    FREEARGPTR(parg16);
    return ulRet;
}

#endif


//
// MyVerQueryValue checks several popular code page values for the given
// string.  This may need to be extended ala WinFile's wfdlgs2.c to search
// the translation table.  For now we only need a few.
//

BOOL
FASTCALL
MyVerQueryValue(
    const LPVOID pBlock,
    LPSTR lpName,
    LPVOID * lplpBuffer,
    PUINT puLen
    )
{
#define SFILEN 25                // Length of apszSFI strings without null
    static PSZ apszSFI[] = {
        "\\StringFileInfo\\040904E4\\",
        "\\StringFileInfo\\04090000\\"
    };
    char szSubBlock[128];
    BOOL fRet;
    int i;

    strcpy(szSubBlock+SFILEN, lpName);

    for (fRet = FALSE, i = 0;
         i < (sizeof apszSFI / sizeof apszSFI[0]) && !fRet;
         i++) {

        RtlCopyMemory(szSubBlock, apszSFI[i], SFILEN);
        fRet = VerQueryValue(pBlock, szSubBlock, lplpBuffer, puLen);
    }

    return fRet;
}


//
// Utility routine to fetch the Product Name and Product Version strings
// from a given EXE.
//

BOOL
FASTCALL
WowGetProductNameVersion(
    PSZ pszExePath,
    PSZ pszProductName,
    DWORD cbProductName,
    PSZ pszProductVersion,
    DWORD cbProductVersion,
    PSZ pszParamName,
    PSZ pszParam,
    DWORD cbParam
    )
{
    DWORD dwZeroMePlease;
    DWORD cbVerInfo;
    LPVOID lpVerInfo = NULL;
    LPSTR pName;
    DWORD cbName;
    LPSTR pVersion;
    DWORD cbVersion;
    BOOL fRet;
    DWORD cbParamValue;
    LPSTR pParamValue;

    fRet = (
        (cbVerInfo = GetFileVersionInfoSize(pszExePath, &dwZeroMePlease)) &&
        (lpVerInfo = malloc_w(cbVerInfo)) &&
        GetFileVersionInfo(pszExePath, 0, cbVerInfo, lpVerInfo) &&
        MyVerQueryValue(lpVerInfo, "ProductName", &pName, &cbName) &&
        cbName <= cbProductName &&
        MyVerQueryValue(lpVerInfo, "ProductVersion", &pVersion, &cbVersion) &&
        cbVersion <= cbProductVersion
        );
    if (fRet && NULL != pszParamName && NULL != pszParam) {
       fRet = MyVerQueryValue(lpVerInfo, pszParamName, &pParamValue, &cbParamValue) &&
              cbParamValue <= cbParam;
    }


    if (fRet) {
        strcpy(pszProductName, pName);
        strcpy(pszProductVersion, pVersion);
        if (NULL != pszParamName && NULL != pszParam) {
           strcpy(pszParam, pParamValue);
        }
    }

    if (lpVerInfo) {
        free_w(lpVerInfo);
    }

    return fRet;
}


#if 0    // currently unused
//
// This routine is simpler to use if you are doing an exact match
// against a particular name/version pair.
//

BOOL
FASTCALL
WowDoNameVersionMatch(
    PSZ pszExePath,
    PSZ pszProductName,
    PSZ pszProductVersion
    )
{
    DWORD dwJunk;
    DWORD cbVerInfo;
    LPVOID lpVerInfo = NULL;
    LPSTR pName;
    LPSTR pVersion;
    BOOL fRet;

    fRet = (
        (cbVerInfo = GetFileVersionInfoSize(pszExePath, &dwJunk)) &&
        (lpVerInfo = malloc_w(cbVerInfo)) &&
        GetFileVersionInfo(pszExePath, 0, cbVerInfo, lpVerInfo) &&
        MyVerQueryValue(lpVerInfo, "ProductName", &pName, &dwJunk) &&
        ! WOW32_stricmp(pszProductName, pName) &&
        MyVerQueryValue(lpVerInfo, "ProductVersion", &pVersion, &dwJunk) &&
        ! WOW32_stricmp(pszProductVersion, pVersion)
        );

    if (lpVerInfo) {
        free_w(lpVerInfo);
    }

    return fRet;
}
#endif




//
// This thunk is called by kernel31's GetModuleHandle
// when it cannot find a handle for given filename.
//
// We look to see if this task has any child apps
// spawned via WinOldAp, and if so we look to see
// if the module name matches for any of them.
// If it does, we return the hmodule of the
// associated WinOldAp.  Otherwise we return 0
//

ULONG FASTCALL WK32WowGetModuleHandle(PVDMFRAME pFrame)
{
    PWOWGETMODULEHANDLE16 parg16;
    ULONG ul;
    PSZ pszModuleName;
    PTD ptd;
    PWOAINST pWOA;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    GETPSZPTR(parg16->lpszModuleName, pszModuleName);

    ptd = CURRENTPTD();

    EnterCriticalSection(&ptd->csTD);

    pWOA = ptd->pWOAList;
    while (pWOA && WOW32_strcmp(pszModuleName, pWOA->szModuleName)) {
        pWOA = pWOA->pNext;
    }

    if (pWOA && pWOA->ptdWOA) {
        ul = pWOA->ptdWOA->hMod16;
        LOGDEBUG(LOG_ALWAYS, ("WK32WowGetModuleHandle(%s) returning %04x.\n",
                              pszModuleName, ul));
    } else {
        ul = 0;
    }

    LeaveCriticalSection(&ptd->csTD);

    return ul;
}


//
// This function is called by kernel31's CreateTask after it's
// allocated memory for the TDB, the selector of which serves
// as the htask.  We want to enforce uniqueness of these htasks
// across all WOW VDMs in the system, so this function attempts
// to reserve the given htask in the shared memory structure.
// If successful the htask is returned, if it's already in use
// 0 is returned and CreateTask will allocate another selector
// and try again.
//
// -- DaveHart 24-Apr-96
//

ULONG FASTCALL WK32WowReserveHtask(PVDMFRAME pFrame)
{
    PWOWRESERVEHTASK16 parg16;
    ULONG ul;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    ul = AddTaskSharedList(parg16->htask, 0, NULL, NULL);

    FREEARGPTR(parg16);

    return ul;
}

/*
 * This function is called by kernel31 to dispatch a wow lfn api call
 * the responsible party here is in dem code and all we have to do is to
 * - retrieve it's frame pointer
 *
 *
 */

ULONG FASTCALL WK32WOWLFNEntry(PVDMFRAME pFrame)
{
   PWOWLFNFRAMEPTR16 parg16;
   LPVOID lpUserFrame;
   ULONG ul;

   GETARGPTR(pFrame, sizeof(*parg16), parg16);

   // now retrieve a flat pointer

   GETMISCPTR(parg16->lpUserFrame, lpUserFrame);

   ul = demWOWLFNEntry(lpUserFrame);

   FREEMISCPTR(lpUserFrame);
   FREEARGPTR(parg16);

   return(ul);
}


//
// This function is called by kernel31 to start or stop the
// shared WOW shutdown timer.
//

ULONG FASTCALL WK32WowShutdownTimer(PVDMFRAME pFrame)
{
    PWOWSHUTDOWNTIMER16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    if (parg16->fEnable) {

        //
        // When this thunk is called with fEnable == 1, to turn on the shutdown
        // timer, it is initially called on the task that is shutting down.  Since
        // we want to be on WowExec's thread so SetTimer will work right, in this
        // case we post a message to WowExec asking it to call this API again, but
        // on the right thread.
        //

        if (ghShellTDB != pFrame->wTDB) {
            PostMessage(ghwndShell, WM_WOWEXECSTARTTIMER, 0, 0);
        } else {
#ifdef WX86
            TermWx86System();
#endif

            SetTimer(ghwndShell, 1, dwSharedWowTimeout, NULL);
        }

    } else {

        //
        // A task was started before the timer expired, kill it.
        //

        WOW32ASSERTMSG(ghShellTDB == pFrame->wTDB, "WowShutdownTimer(0) called on non-WowExec thread\n");

        KillTimer(ghwndShell, 1);
    }

    FREEARGPTR(parg16);

    return 0;
}


//
// This function is called by kernel31 to shrink the process's
// working set to a minimum.
//

ULONG FASTCALL WK32WowTrimWorkingSet(PVDMFRAME pFrame)
{
    SetProcessWorkingSetSize(ghProcess, 0xffffffff, 0xffffffff);

    return 0;
}

//
// IsQuickBooksVersion2 used by WK32SetAppCompatFlags below.
//

BOOL FASTCALL IsQuickBooksVersion2(WORD pModule)
{
    BOOL fRet;
    PSZ pszModuleFileName;
    HANDLE hEXE;
    HANDLE hSec = 0;
    PVOID pEXE = NULL;
    PIMAGE_DOS_HEADER pMZ;
    PIMAGE_OS2_HEADER pNE;
    PBYTE pNResTab;
    DWORD cbVerInfo;
    DWORD dwJunk;

    fRet = FALSE;

    //
    // see wow16\inc\newexe.inc, NEW_EXE1 struct, ne_pfileinfo
    // is at offset 10, a near pointer within the segment referenced
    // by hmod.  This points to kernel.inc's OPENSTRUC which has
    // the filename buffer at offset 8.
    //

    pszModuleFileName = SEGPTR(pModule, (*(WORD *)SEGPTR(pModule, 10)) + 8);

    hEXE = CreateFile(
        pszModuleFileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,   // security
        OPEN_EXISTING,
        0,      // flags & attributes
        NULL
        );

    if (INVALID_HANDLE_VALUE == hEXE) {
        goto Cleanup;
    }

    hSec = CreateFileMapping(
        hEXE,
        NULL,   // security
        PAGE_READONLY,
        0,      // max size hi
        0,      // max size lo  both zero == file size
        NULL    // name
        );

    if ( ! hSec) {
        goto Cleanup;
    }

    pEXE = MapViewOfFile(
        hSec,
        FILE_MAP_READ,
        0,      // offset hi
        0,      // offset lo
        0       // size to map   zero == entire file
        );

    pMZ = pEXE;

    if (IMAGE_DOS_SIGNATURE != pMZ->e_magic) {
        WOW32ASSERTMSG(IMAGE_DOS_SIGNATURE == pMZ->e_magic, "WOW IsQuickBooks MZ sig.\n");
        goto Cleanup;
    }

    pNE = (PVOID) ((PBYTE)pEXE + pMZ->e_lfanew);

    if (IMAGE_OS2_SIGNATURE != pNE->ne_magic) {
        WOW32ASSERTMSG(IMAGE_OS2_SIGNATURE == pNE->ne_magic, "WOW IsQuickBooks NE sig.\n");
        goto Cleanup;
    }

    pNResTab = (PBYTE)pEXE + pNE->ne_nrestab;

    //
    // The first entry in the non-resident names table is
    // the NE description specified in the .DEF file.
    // We have the culprit if it matches the string below,
    // note the initial 'R' is a length byte, 0x52 bytes follow,
    // so we compare 0x53.
    //
    // Of course Intuit is full of clever programmers, so this
    // description string still appears in QBW.EXE v3.1 and probably
    // later.  Thankfully someone there thought to add version
    // resources between v2 and v3, so if there are version
    // resources we'll say it's not v2.
    //

    fRet = RtlEqualMemory(
        pNResTab,
        "RQuickBooks for Windows Version 2.  Copyright 1993 Intuit Inc. All rights reserved.",
        0x53
        );

    if (fRet) {
        cbVerInfo = GetFileVersionInfoSize(pszModuleFileName, &dwJunk);
        fRet = !cbVerInfo;
    }

  Cleanup:

    if (pEXE) {
        UnmapViewOfFile(pEXE);
    }

    if (hSec) {
        CloseHandle(hSec);
    }

    if (INVALID_HANDLE_VALUE != hEXE) {
        CloseHandle(hEXE);
    }

    return fRet;
}


// the code below is stolen (with some enhancements) from
// then original WOWShouldWeSayWin95
//

BOOL FASTCALL fnInstallShieldOverrideVersionFlag(PTDB pTDB)
{
   CHAR szModName[9];
   PCHAR pch;
   CHAR szName[16];
   CHAR szVersion[16];
   CHAR szVerSubstring[4];
   DWORD dwSubVer;
   PSTR pszFileName;

   RtlCopyMemory(szModName, pTDB->TDB_ModName, 8);
   for (pch = &szModName[7]; ' ' == *pch && pch >= szModName; --pch);
   *++pch = '\0';

   if (WOW32_stricmp(szModName, "ISSET_SE")) {
      return(FALSE);
   }

   // now having the pTDB retrieve module file name from pExe
   // this guy's sitting in TDB_pModule -- and we know it's a real thing
   // not an alias

   //
   // see wow16\inc\newexe.inc, NEW_EXE1 struct, ne_pfileinfo
   // is at offset 10, a near pointer within the segment referenced
   // by hmod.  This points to kernel.inc's OPENSTRUC which has
   // the filename buffer at offset 8.
   //
   pszFileName = SEGPTR(pTDB->TDB_pModule, (*(WORD *)SEGPTR(pTDB->TDB_pModule, 10)) + 8);

   if (!WowGetProductNameVersion(pszFileName,
                                 szName,
                                 sizeof(szName),
                                 szVersion,
                                 sizeof(szVersion),
                                 NULL, NULL, 0) ||
      WOW32_stricmp(szName, "InstallSHIELD")) {
      return(FALSE);
   }

   //
   // now we definitely know it's installshield and it's version is in szVersion
   //


   //
   // InstallShield _Setup SDK_ setup.exe shipped
   // with VC++ 4.0 is stamped 2.20.903.0 but also
   // needs to be lied to about it being Win95.
   // According to samir@installshield.com versions
   // 2.20.903.0 through 2.20.905.0 need this.
   // We'll settle for 2.20.903* - 2.20.905*
   // These are based on the 3.0 codebase but
   // bear the 2.20.x version stamps.
   //

   if (RtlEqualMemory(szVersion, "2.20.90", 7) &&
       ('3' == szVersion[7] ||
        '4' == szVersion[7] ||
        '5' == szVersion[7]) ) {
       return(TRUE);
   }

   //
   // We want to lie in GetVersion if the version stamp on
   // the InstallShield setup.exe is 3.00.xxx.0, where
   // xxx is 000 through 087.  Later versions know how
   // to detect NT.
   //

   if (!RtlEqualMemory(szVersion, "3.00.", 5)) {
      return(FALSE);
   }

   RtlCopyMemory(szVerSubstring, &szVersion[5], 3);
   szVerSubstring[3] = 0;
   RtlCharToInteger(szVerSubstring, 10, &dwSubVer);

   if (dwSubVer >= 88 && dwSubVer != 101) {
      return(FALSE);
   }

   return(TRUE); // version 3.00.000 - 3.00.087
}

BOOL FASTCALL fnInstallTimelineOverrideVersionFlag(PTDB pTDB)
{
   CHAR szModName[9];
   PCHAR pch;
   CHAR szName[64];
   CHAR szVersion[16];
   CHAR szFileVersion[16];
   PSTR pszFileName;

   RtlCopyMemory(szModName, pTDB->TDB_ModName, 8);
   for (pch = &szModName[7]; ' ' == *pch && pch >= szModName; --pch);
   *++pch = '\0';

   if (WOW32_stricmp(szModName, "INSTBIN")) { // instbin is an installer for
      return(FALSE);
   }

   // now having the pTDB retrieve module file name from pExe
   // this guy's sitting in TDB_pModule -- and we know it's a real thing
   // not an alias

   //
   // see wow16\inc\newexe.inc, NEW_EXE1 struct, ne_pfileinfo
   // is at offset 10, a near pointer within the segment referenced
   // by hmod.  This points to kernel.inc's OPENSTRUC which has
   // the filename buffer at offset 8.
   //
   pszFileName = SEGPTR(pTDB->TDB_pModule, (*(WORD *)SEGPTR(pTDB->TDB_pModule, 10)) + 8);

   // now retrieve version resources
   if (!WowGetProductNameVersion(pszFileName,
                                 szName,
                                 sizeof(szName),
                                 szVersion,
                                 sizeof(szVersion),
                                 "FileVersion",
                                 szFileVersion,
                                 sizeof(szFileVersion)) ||
      WOW32_stricmp(szName, "Symantec Install for Windows Applications")) {
      return(FALSE);
   }

   // so it is Symantec install -- check versions

   if (!WOW32_stricmp(szVersion, "3.4") && !WOW32_stricmp(szFileVersion, "3.4.1.1")) {
      return(FALSE);
   }


   return(TRUE); // we can munch on Win95 -- this is not the same install as
                 // used in timeline application
}


typedef BOOL (FASTCALL *PFNOVERRIDEVERSIONFLAG)(PTDB);

PFNOVERRIDEVERSIONFLAG rgOverrideFns[] = {
   fnInstallShieldOverrideVersionFlag,
   fnInstallTimelineOverrideVersionFlag
};


// this function is to be used when we set "3.1" compat flag in the registry
// then we call these functions to override the flag if function returns true
// if returns TRUE then the version flag is set to 95
// if returns FALSE then the version flag is left to what it was in the registry

BOOL IsOverrideVersionFlag(PTDB pTDB)
{
   int i;
   BOOL fOverride = FALSE;

   for (i = 0; i < sizeof(rgOverrideFns)/sizeof(rgOverrideFns[0]) && !fOverride; ++i) {
       fOverride = (*rgOverrideFns[i])(pTDB);
   }

   return(fOverride);

}


//
// This function replaces the original assembler in
// kernel31\miscapi.asm.  It's the same, except it special-cases
// some flags based on more than just the module name (for
// example version stamps).
//
// Note that we're still running on the creator thread,
// so CURRENTPTD() refers to the parent of the app we're
// looking up flags for.
//

ULONG FASTCALL WK32SetAppCompatFlags(PVDMFRAME pFrame)
{
    PSETAPPCOMPATFLAGS16 parg16;
    DWORD dwAppCompatFlags = 0;
    PTDB  pTDB;
    char  szModName[9];
    char  szAppCompatFlags[12];  // 0x00000000

    GETARGPTR(pFrame, sizeof(*parg16), parg16);

    pTDB = (PVOID)SEGPTR(parg16->TDB,0);

    //
    // Hacks don't apply to 4.0 or above
    //

    if (pTDB->TDB_ExpWinVer < 0x400) {

        RtlCopyMemory(szModName, pTDB->TDB_ModName, sizeof(szModName)-1);
        szModName[sizeof(szModName)-1] = 0;


        szAppCompatFlags[0] = 0;

        if (GetProfileString(
                "Compatibility",
                szModName,
                "",
                szAppCompatFlags,
                sizeof(szAppCompatFlags))) {

            dwAppCompatFlags = strtoul(szAppCompatFlags, NULL, 0);
        }

        //
        // SOME hacks don't apply to 3.1 or above
        //

        if (pTDB->TDB_ExpWinVer >= 0x30a) {
            dwAppCompatFlags &= GACF_31VALIDMASK;
        }

        //
        // Intuit QuickBooks 2.0 needs to have GACF_RANDOM3XUI turned on,
        // but later versions don't want it on.  Win9x prompts the user with
        // a warning that leads to a help file that tells the user to turn
        // on this bit using a little tool if they're using QBW v2.  We are
        // going to just do the right thing by looking at the Description
        // field of the EXE header for a string we expect only to find in
        // v2, and if it's there turn on GACF_RANDOM3XUI.
        //

        if (pTDB->TDB_ExpWinVer == 0x30a &&
            RtlEqualMemory(szModName, "QBW", 4)) {

            if (IsQuickBooksVersion2(pTDB->TDB_pModule)) {

                dwAppCompatFlags |= GACF_RANDOM3XUI;
            }

        }


        // this code checks to see that ISSET_SE in adobe premier 4.2
        // is told that it's running on Win95
        if (IsOverrideVersionFlag(pTDB)) {
           dwAppCompatFlags &= ~GACF_WINVER31;
        }

        LOGDEBUG(LOG_ALWAYS, ("WK32SetAppCompatFlags '%s' got %x (%s).\n",
                              szModName, dwAppCompatFlags, szAppCompatFlags));
    }

    FREEARGPTR(parg16);

    return dwAppCompatFlags;
}


//
// This function is called by mciavi32 to facilitate usage of a 16-bit mciavi
// instead
//
//
BOOL WOWUseMciavi16(VOID)
{
   return((BOOL)(CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_USEMCIAVI16));
}

VOID WOWExitVdm(ULONG iWowTask)
{
   if (ALL_TASKS == iWowTask) {
      // meltdown
      RemoveProcessSharedList();
   }

   ExitVDM(WOWVDM, iWowTask);
}

