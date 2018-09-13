/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Walk.c

Abstract:

    This module contains the support for "Walking".


Author:

    Kent Forschmiedt (kentf) January 1, 1996

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


//
//  Externals
//

extern DMTLFUNCTYPE DmTlFunc;
extern char         abEMReplyBuf[];
extern DEBUG_EVENT64  falseBPEvent;


extern CRITICAL_SECTION csWalk;
extern CRITICAL_SECTION csThreadProcList;


#ifdef HAS_DEBUG_REGS
//
// Initializer defined in dm.h
//
DWORD DebugRegDataSizes[] = DEBUG_REG_DATA_SIZES;
#define NDEBUG_REG_DATA_SIZES  (sizeof(DebugRegDataSizes) / sizeof(*DebugRegDataSizes))

#endif


//
//  Walk Structure.
//
//  Contains information to perform a walk on a thread.
//
typedef struct _WALK {
    LIST_ENTRY  AllWalkList;    //  List of all walks
    LIST_ENTRY  WalkList;       //  List of all walks in this thread

    LIST_ENTRY  GroupEntryList; //  Binding to breakpoints

    HTHDX       hthd;           //  thread
    BPTP        BpType;         //  Breakpoint type
    BOOL        Active;         //  Active flag
    DWORD       GlobalCount;    //  All thread ref count
    DWORD       LocalCount;     //  per-thread ref count

    UOFFSET     AddrStart;      //  Range Begin
    UOFFSET     AddrEnd;        //  Range End
    PBREAKPOINT StartBP;        //  BP on range entry
    BOOL        HasAddrEnd;

    UOFFSET     DataAddr;       //  Data Address
    DWORD       DataSize;       //  Data Size
    PVOID       DataContents;   //  for change detection

    BREAKPOINT *SafetyBP;       //  Safety breakpoint for calls
    METHOD      Method;         //  Walk method
#ifdef HAS_DEBUG_REGS
    int         Register;
    BOOL        SingleStep;     //  In single-step mode
#endif
} WALK;
typedef struct _WALK      *PWALK;

//
// This node binds a walk structure to one or more breakpoints.
//
// One of these nodes will be allocated for each binding of
// a WALK to a breakpoint.
//
typedef struct _WALK_GROUP_ENTRY {

    //
    // Here is the list of walks that belongs to a breakpoint.
    // This is a list of WALK_GROUP_ENTRY nodes; all nodes on
    // this list represent the same breakpoint, each represents
    // a different WALK.
    //
    LIST_ENTRY  WalksInGroup;

    //
    // Here is the list of breakpoints which are using this walk.
    // All WALK_GROUP_ENTRY nodes in this list refer to the same
    // WALK, while each refers to a different breakpoint.
    //
    LIST_ENTRY  GroupsUsingWalk;

    //
    // Here is the walk associated with this node:
    //

    PWALK Walk;

} WALK_GROUP_ENTRY, *PWALK_GROUP_ENTRY;


//
//  Local variables
//
LIST_ENTRY  AllWalkListHead;               //  List of all walks


PWALK
SetWalkThread (
    HPRCX   hprc,
    HTHDX   hthd,
    UOFFSET Addr,
    DWORD   Size,
    BPTP    BpType,
    BOOL    Global
    );

BOOL
RemoveWalkThread (
    HPRCX hprc,
    HTHDX hthd,
    UOFFSET Addr,
    DWORD Size,
    BPTP  BpType,
    BOOL
    );

BOOL
StartWalk(
    PWALK Walk,
    BOOL Continuing
    );

PWALK
AllocateWalk(
    HTHDX hthd,
    UOFFSET Addr,
    DWORD Size,
    BPTP BpType
    );

BOOL
DeallocateWalk(
    PWALK
    );

BOOL
RemoveWalkEntry(
    PWALK Walk,
    BOOL Global
    );

PWALK
FindWalk (
    HTHDX hthd,
    UOFFSET Addr,
    DWORD Size,
    BPTP BpType
    );

PWALK
FindWalkForHthd(
    HANDLE hWalk,
    HTHDX hthd
    );

PBREAKPOINT
FindBpForWalk(
    PVOID pWalk
    );

int
MethodWalk(
    DEBUG_EVENT64*,
    HTHDX,
    DWORDLONG,
    DWORDLONG
    );

VOID
AddWalkToGroupList(
    PLIST_ENTRY GroupList,
    PWALK Walk
    );

VOID
RemoveWalkBindings(
    PWALK Walk
    );

VOID
DuplicateWalkBindings(
    PWALK OldWalk,
    PWALK NewWalk
    );


//*******************************************************************
//
//                      Exported Functions
//
//******************************************************************


VOID
ExprBPInitialize(
    VOID
    )
{
    InitializeListHead(&AllWalkListHead);
}


VOID
ExprBPCreateThread(
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    If global walking, adds walk to new thread. Called when a
    new thread is created.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

Return Value:

    None
--*/

{
    PWALK       Walk;
    PLIST_ENTRY List;
    HTHDX       hthdT;

    //
    //  If there are global walks, set them in this thread
    //

    //
    //  Get a walk list from any thread in this process and
    //  traverse it, copying any global walks.  Note that we
    //  can use any walk list because global walks are common
    //  to all threads.
    //

    EnterCriticalSection(&csWalk);

    EnterCriticalSection(&csThreadProcList);
    hthdT = hprc->hthdChild;
    while (hthdT && hthdT == hthd) {
        hthdT = hthdT->nextSibling;
    }
    LeaveCriticalSection(&csThreadProcList);

    if (hthdT) {

        List = hthdT->WalkList.Flink;

        while (List != &hthdT->WalkList) {

            Walk = CONTAINING_RECORD(List, WALK, WalkList);
            List = List->Flink;

            if ( Walk->GlobalCount > 0 ) {
                PWALK twalk = SetWalkThread( hprc,
                                             hthd,
                                             Walk->DataAddr,
                                             Walk->DataSize,
                                             Walk->BpType,
                                             TRUE
                                             );
                //
                // bind the new walk record the same as the old:
                //
                //
                DuplicateWalkBindings(Walk, twalk);

            }
        }
    }

    LeaveCriticalSection(&csWalk);
}


VOID
ExprBPExitThread (
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    Removes walk in a thread, called when the thread is gone.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

Return Value:

    None

--*/

{
    PLIST_ENTRY List;
    PWALK       Walk;
    DWORD       GlobalCount;
    DWORD       LocalCount;

    EnterCriticalSection(&csWalk);

    List = hthd->WalkList.Flink;

    while ( List != &hthd->WalkList ) {

        Walk = CONTAINING_RECORD(List, WALK, WalkList);
        List = List->Flink;

        GlobalCount = Walk->GlobalCount;
        LocalCount  = Walk->LocalCount;
        while ( GlobalCount-- ) {
            RemoveWalkEntry( Walk, TRUE );
        }
        while ( LocalCount-- ) {
            RemoveWalkEntry( Walk, FALSE );
        }
    }

    LeaveCriticalSection(&csWalk);
}


VOID
ExprBPContinue (
    HPRCX   hprc,
    HTHDX   hthd
    )
/*++

Routine Description:

    Continues walking. Called as a result of a continue command.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

Return Value:

    None

--*/

{
    PWALK       Walk;
    PLIST_ENTRY List;


    if ( !hthd ) {
        return;
    }

    //
    //  See if we have a walk on the thread
    //

    EnterCriticalSection(&csWalk);

    List = hthd->WalkList.Flink;
    while (List != &hthd->WalkList) {
        Walk = CONTAINING_RECORD(List, WALK, WalkList);
        List = List->Flink;
#ifdef HAS_DEBUG_REGS
        if ( Walk->Register >= 0 && !Walk->SingleStep ) {

            StartWalk( Walk, TRUE );

        } else
#endif
        if ( !Walk->Active ) {

            if (Walk->BpType != bptpRange) {
                //
                //  Get the current address for the thread.
                //
                Walk->AddrStart = PC( hthd );

                //
                //  Get the end of the range
                //

                Walk->AddrEnd = Walk->AddrStart;
                Walk->HasAddrEnd = FALSE;

            }
            //
            //  Start walking
            //

            StartWalk( Walk, TRUE );
        }
    }
    LeaveCriticalSection(&csWalk);
}                               /* ExprBPContinue() */


VOID
ExprBPResetBP(
    HTHDX hthd,
    PBREAKPOINT bp
    )
/*++

Routine Description:

    After stepping off of a hardware BP, reset debug register(s)
    before continuing.

Arguments:

    hthd - Supplies the thread which has been stepped.

    bp - Supplies the BREAKPOINT

Return Value:

    none

--*/
{
#ifndef HAS_DEBUG_REGS
    Unreferenced(hthd);
#else
    PWALK Walk;
    PDEBUGREG Dr;

    assert(bp);
    assert(bp->hWalk);

    Walk = FindWalkForHthd(bp->hWalk, hthd);

    assert(Walk->Register >= 0 && hthd->DebugRegs[Walk->Register].InUse);

    Dr = &hthd->DebugRegs[Walk->Register];

    SetupDebugRegister(
        hthd,
        Walk->Register,
        Dr->DataSize,
        Dr->DataAddr,
        Dr->BpType
        );

#endif  // HAS_DEBUG_REGS
}



VOID
ExprBPClearBPForStep(
    HTHDX hthd
    )
/*++

Routine Description:

    Turn off a hardware breakpoint to allow a single step to occur.
    This is necessary for x86 exec breakpoints, but not for data read/write.

Arguments:

    hthd - Supplies the thread which is going to be stepped.

Return Value:

    none

--*/
{
#ifndef HAS_DEBUG_REGS
    Unreferenced(hthd);
#else
    BREAKPOINT *bp;
    PWALK Walk;

    bp = AtBP(hthd);
    assert(bp);

    assert(bp->hWalk);

    Walk = FindWalkForHthd(bp->hWalk, hthd);

    assert(Walk && Walk->Register >= 0 && hthd->DebugRegs[Walk->Register].InUse);

    ClearDebugRegister(hthd, Walk->Register);
#endif
}


void
ExprBPRestoreDebugRegs(
    HTHDX   hthd
    )
/*++

Routine Description:

    Restore the CPU debug registers to the state that we last put
    them in.  This routine is needed because the system trashes
    the debug registers after initializing the DLLs and before the
    app entry point is executed.

Arguments:

    hthd    - Supplies descriptor for thread whose registers need fixing.

Return Value:

    None

--*/
{
#ifndef HAS_DEBUG_REGS
    Unreferenced(hthd);
#else
    PWALK       Walk;
    PLIST_ENTRY List;
    PDEBUGREG Dr;

    EnterCriticalSection(&csWalk);

    List = hthd->WalkList.Flink;

    while (List != &hthd->WalkList) {

        Walk = CONTAINING_RECORD(List, WALK, WalkList);
        List = List->Flink;

        if ( Walk->Active && Walk->Register >= 0 && hthd->DebugRegs[Walk->Register].InUse) {

            Dr = &hthd->DebugRegs[Walk->Register];
            SetupDebugRegister(
                hthd,
                Walk->Register,
                Dr->DataSize,
                Dr->DataAddr,
                Dr->BpType
                );
        }
    }
    LeaveCriticalSection(&csWalk);
#endif
}



HANDLE
SetWalk (
    HPRCX   hprc,
    HTHDX   hthd,
    UOFFSET   Addr,
    DWORD   Size,
    DWORD   BpType
    )
/*++

Routine Description:

    Sets up a walk.  Returns a handle which may be used to associate this
    walk with a breakpoint structure.

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies size of memory to watch

    BpType  -   Supplies type of breakpoint

Return Value:

    A handle to the new list of walks

--*/

{
    PWALK   Walk;
    PLIST_ENTRY GroupList = NULL;

    if ( hprc ) {

        //
        //  If a thread is specified, we use that specific thread,
        //  otherwise we must set the walk in all existing threads,
        //  plus we must set things up so that we walk all future
        //  threads too (while this walk is active).
        //
        if ( hthd ) {

            Walk = SetWalkThread( hprc, hthd, Addr, Size, BpType, FALSE );
            if (Walk) {
                GroupList = MHAlloc(sizeof(LIST_ENTRY));
                InitializeListHead(GroupList);
                AddWalkToGroupList(GroupList, Walk);
            }

        } else {

            GroupList = MHAlloc(sizeof(LIST_ENTRY));
            InitializeListHead(GroupList);

            EnterCriticalSection(&csThreadProcList);

            for ( hthd = (HTHDX)hprc->hthdChild;
                  hthd;
                  hthd = hthd->nextSibling ) {

                Walk = SetWalkThread( hprc, hthd, Addr, Size, BpType, TRUE );
                if (Walk) {
                    AddWalkToGroupList(GroupList, Walk);
                }
            }

            LeaveCriticalSection(&csThreadProcList);

            if (IsListEmpty(GroupList)) {
                MHFree(GroupList);
                GroupList = NULL;
            }
        }
    }

    return (HANDLE)GroupList;
}



BOOL
RemoveWalk(
    HANDLE hWalk,
    BOOL Global
    )

/*++

Routine Description:

    Remove a group of walks.

Arguments:


Return Value:


--*/

{
    PLIST_ENTRY GroupListHead = (PLIST_ENTRY)hWalk;
    PLIST_ENTRY List;
    PWALK_GROUP_ENTRY Entry;
    PWALK Walk;

    List = GroupListHead->Flink;

    while (List != GroupListHead) {
        Entry = CONTAINING_RECORD(List, WALK_GROUP_ENTRY, WalksInGroup);
        List = List->Flink;
        Walk = Entry->Walk;

        RemoveWalkEntry(Walk, Global);
    }

    //
    // The list head pointed to by GroupList will be freed
    // when the last entry is deleted by DeallocateWalk.
    //

    return TRUE;
}



BOOL
RemoveWalkEntry(
    PWALK Walk,
    BOOL Global
    )
{
    HTHDX hthd = Walk->hthd;

#ifndef KERNEL
    BOOL Froze = FALSE;
    //
    //  freeze the thread
    //
    if ( hthd->tstate & ts_running ) {
        if ( SuspendThread( hthd->rwHand ) != -1L) {
            hthd->tstate |= ts_frozen;
            Froze = TRUE;
        }
    }

#endif
    //
    //  Remove the walk
    //
    Global ? Walk->GlobalCount-- : Walk->LocalCount--;

    if ( Walk->GlobalCount == 0 &&
         Walk->LocalCount  == 0 ) {

#ifdef HAS_DEBUG_REGS
        if ( Walk->Register >= 0 ) {
            Walk->Active = FALSE;
            DeallocateWalk( Walk );
        } else
#endif
        {
            //
            //  If the walk is active, the method will eventually
            //  be called. Otherwise we must call the method
            //  ourselves.
            //
            if ( !Walk->Active ) {
                MethodWalk( NULL, hthd, 0, (DWORDLONG)Walk );
            }
        }
    }
#ifndef KERNEL
    //
    //  Resume the thread if we froze it.
    //
    if ( Froze ) {
        if (ResumeThread(hthd->rwHand) != -1L ) {
            hthd->tstate &= ~ts_frozen;
        }
    }
#endif
    return TRUE;
}


#ifdef HAS_DEBUG_REGS
PBREAKPOINT
GetWalkBPFromBits(
    HTHDX   hthd,
    DWORD   bits
    )
{
    PWALK       Walk;
    PLIST_ENTRY List;
    PBREAKPOINT bp = NULL;

    EnterCriticalSection(&csWalk);

    //
    // This only finds the first match.  If more than one BP was
    // matched by the CPU, we won't notice.
    //

    List = hthd->WalkList.Flink;

    while (List != &hthd->WalkList) {
        Walk = CONTAINING_RECORD(List, WALK, WalkList);
        List = List->Flink;
        if ( Walk->Register >= 0 && hthd->DebugRegs[Walk->Register].InUse ) {
            if (bits & (1 << Walk->Register)) {
                // hit!
                bp = FindBpForWalk(Walk);
                break;
            }
        }
    }

    LeaveCriticalSection(&csWalk);

    return bp;
}
#endif // HAS_DEBUG_REGS



BOOL
CheckWalk(
    PWALK Walk
    )
/*++

Routine Description:

    This decides whether a data or range breakpoint should fire.

    Current implementation handles:

        Data change, emulated or implemented on hardware write BP
        Range BP

    Not handled:

        Emulated data read/write BP

Arguments:

    Walk - Supplies the thread and breakpoint info

Return Value:

    TRUE if the breakpoint should fire, FALSE if it should be ignored.
    This implementation is conservative; unhandled cases always return TRUE.

--*/
{
    PVOID Data;
    DWORD dwSize;
    BOOL ret = TRUE;

    if (Walk->BpType == bptpDataC) {
        if (Walk->DataContents && (Data = MHAlloc(Walk->DataSize))) {
            if (DbgReadMemory(Walk->hthd->hprc,
                              Walk->DataAddr,
                              Data,
                              Walk->DataSize,
                              &dwSize)) {
                ret = (memcmp(Data, Walk->DataContents, Walk->DataSize) != 0);
            }
            MHFree(Data);
        }
    }
    else if (Walk->BpType == bptpRange) {
        ret = Walk->AddrStart <= PC(Walk->hthd) &&
                PC(Walk->hthd) <= Walk->AddrEnd;
    }

    return ret;
}


BOOL
CheckDataBP(
    HTHDX hthd,
    PBREAKPOINT Bp
    )
/*++

Routine Description:

    This decides whether a breakpoint should fire.  If it is not
    a data breakpoint, it should fire.  If it is a data breakpoint,
    decide whether it has been satisfied.

Arguments:

    hthd - Supplies the thread that stopped

    Bp   - Supplies the breakpoint that was hit

Return Value:

    TRUE if the breakpoint has really fired, FALSE if it should be ignored.

--*/
{
    PWALK Walk;

    assert(hthd);
    assert(Bp);

    if (!Bp->hWalk) {
        return TRUE;
    }

    Walk = FindWalkForHthd(Bp->hWalk, hthd);

    assert(Walk);

    if (!Walk) {
        return TRUE;
    }

    return CheckWalk(Walk);
}

//*******************************************************************
//
//                      Local Functions
//
//******************************************************************

PWALK
SetWalkThread (
    HPRCX   hprc,
    HTHDX   hthd,
    UOFFSET Addr,
    DWORD   Size,
    DWORD   BpType,
    BOOL    Global
    )
/*++

Routine Description:

    Sets up a walk in a specific thread

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    BpType  -   Supplies type (read, read/write, change, exec, range)

    Global  -   Supplies global flag

Return Value:

    BOOL    -   TRUE if Walk set

--*/

{
    PWALK       Walk;
    BOOL        AllocatedWalk = FALSE;
    BOOL        Ok            = FALSE;
    BOOL        Froze         = FALSE;

    if ( Walk = FindWalk( hthd, Addr, Size, BpType ) ) {

        //
        //  If the walk is already active, just increment the
        //  reference count and we're done.
        //

        //
        // if it isn't active, fall through and activate it
        //
        if ( Walk->Active ) {
            Global ? Walk->GlobalCount++ : Walk->LocalCount++;
            Ok = TRUE;
            goto Done;
        }

    } else {

        //
        //  Allocate a walk for this thread.
        //

        if ( Walk = AllocateWalk( hthd, Addr, Size, BpType ) ) {
            AllocatedWalk = TRUE;
        } else {
            goto Done;
        }
    }

#ifdef KERNEL
    Ok = TRUE;
#else
    //
    //  We have to freeze the specified thread in order to get
    //  the current address.
    //
    if ( !(hthd->tstate & ts_running) || (hthd->tstate & ts_frozen)) {
        Ok = TRUE;
    } else if ( SuspendThread( hthd->rwHand ) != -1L) {
        Froze = TRUE;
        Ok    = TRUE;
        hthd->context.ContextFlags = CONTEXT_CONTROL;
        DbgGetThreadContext( hthd, &hthd->context );
    }
#endif

    if ( Ok ) {

        //
        //  Increment reference count
        //

        Global ? Walk->GlobalCount++ : Walk->LocalCount++;

        if (Walk->BpType != bptpRange) {
            //
            //  Get the current address for the thread.
            //

            Walk->AddrStart = PC( hthd );

            //
            //  Get the end of the range
            //
            Walk->AddrEnd    = Walk->AddrStart;
            Walk->HasAddrEnd = FALSE;
        }

        Ok = StartWalk( Walk, FALSE );

#ifndef KERNEL
        //
        //  Resume the thread if we froze it.
        //
        if ( Froze ) {
            if (ResumeThread(hthd->rwHand) == (ULONG)-1) {
                assert(!"ResumeThread failed in SetWalkThread");
                hthd->tstate |= ts_frozen;
            }
        }
#endif
    }

Done:
    //
    //  Clean up
    //
    if ( !Ok ) {
        if ( Walk && AllocatedWalk ) {
            DeallocateWalk( Walk );
        }
        Walk = NULL;
    }

    return Walk;
}




BOOL
RemoveWalkThread (
    HPRCX   hprc,
    HTHDX   hthd,
    UOFFSET Addr,
    DWORD   Size,
    BPTP    BpType,
    BOOL    Global
    )
/*++

Routine Description:

    Removes a walk in a specific thread

Arguments:

    hprc    -   Supplies process

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    BpType  -   Supplies breakpoint type

    Global  -   Supplies global flag

Return Value:

    BOOL    -   TRUE if Walk removed

--*/

{
    PWALK       Walk;

    if ( Walk = FindWalk( hthd, Addr, Size, BpType ) ) {
        return RemoveWalkEntry(Walk, Global);
    } else {
        return FALSE;
    }
}



BOOL
StartWalk(
    PWALK   Walk,
    BOOL    Continuing
    )
/*++

Routine Description:

    Starts walking.

Arguments:

    Walk    -   Supplies the walk sructure

    Continuing - Supplies a flag saying that the thread is being continued

Return Value:

    BOOL    -   TRUE if done

--*/

{
    BREAKPOINT* bp;
    ACVECTOR    action  = NO_ACTION;
    HTHDX       hthd = Walk->hthd;
    DWORD       dwSize;

    if (!(hthd->tstate & ts_stopped) || Continuing) {
        if (Walk->BpType == bptpDataC) {
            //
            // remember contents for change detection
            //
            if (Walk->DataContents) {
                MHFree(Walk->DataContents);
            }
            Walk->DataContents = MHAlloc(Walk->DataSize);
            //
            // if we can't read the data, just fire on any write
            //
            if (!DbgReadMemory(hthd->hprc,
                               Walk->DataAddr,
                               Walk->DataContents,
                               Walk->DataSize,
                               &dwSize)) {
                MHFree(Walk->DataContents);
                Walk->DataContents = NULL;
            }
        }

#ifdef HAS_DEBUG_REGS
        if ( Walk->Register >= 0 ) {
            PDEBUGREG Dr = &hthd->DebugRegs[Walk->Register];

            if ( !Dr->InUse ) {

                if (!SetupDebugRegister(
                        hthd,
                        Walk->Register,
                        Dr->DataSize,
                        Dr->DataAddr,
                        Dr->BpType)
                ) {
                    return FALSE;
                }

                Walk->Active        = TRUE;
                Walk->SingleStep    = FALSE;
                Dr->InUse           = TRUE;
            }

        } else
#endif // HAS_DEBUG_REGS
        if (Walk->BpType != bptpRange ||
            ((Walk->AddrStart <= PC(hthd) && PC(hthd) <= Walk->AddrEnd)) ) {

            bp = AtBP( hthd );

            //
            // if the thread is sitting on a BP, the step off of BP code
            // does its thing first, then the walk method gets control.
            //
            if ( !bp ) {
                Walk->Active = TRUE;

                //
                //  Setup a single step
                //
                if (!SetupSingleStep(hthd, FALSE )) {
                    return FALSE;
                }

                //
                //  Place a single step on our list of expected events.
                //
                if (!(hthd->tstate & ts_stopped) || Continuing) {
                    RegisterExpectedEvent(
                            hthd->hprc,
                            hthd,
                            EXCEPTION_DEBUG_EVENT,
                            (DWORD_PTR)EXCEPTION_SINGLE_STEP,
                            &(Walk->Method),
                            action,
                            FALSE,
                            0);
                }
            }

        } else {

            //
            // Range BP, current IP is out of range.
            //

            //
            // This implementation only works reliably for function
            // scoped range BPs.  If a range has an entry point other
            // than its lowest address, it will fail to activate the
            // range stepper on entry.
            //

            ADDR bpAddr;
            BREAKPOINT *pbp;

            AddrFromHthdx(&bpAddr, hthd);
            bpAddr.addr.off = Walk->AddrStart;

            pbp = FindBP(Walk->hthd->hprc,
                         Walk->hthd,
                         bptpExec,
                         bpnsStop,
                         &bpAddr,
                         TRUE);

            if (!pbp) {
                METHOD *method = (METHOD*)MHAlloc(sizeof(METHOD));
                *method = Walk->Method;
                Walk->StartBP = SetBP(Walk->hthd->hprc,
                                      Walk->hthd,
                                      bptpExec,
                                      bpnsStop,
                                      &bpAddr,
                                      (HPID)INVALID);
                method->lparam2 = (LPVOID) Walk->StartBP;
                RegisterExpectedEvent(Walk->hthd->hprc,
                                      Walk->hthd,
                                      BREAKPOINT_DEBUG_EVENT,
                                      (DWORD_PTR) Walk->StartBP,
                                      DONT_NOTIFY,
                                      SSActionRemoveBP,
                                      FALSE,
                                      (UINT_PTR)method);
            }
        }
    }

    return TRUE;
}                                   /* StartWalk() */



//*******************************************************************
//
//                      WALK Stuff
//
//******************************************************************


PWALK
AllocateWalk (
    HTHDX       hthd,
    UOFFSET     Addr,
    DWORD       Size,
    BPTP        BpType
    )
/*++

Routine Description:

    Allocates new Walk structure and adds it to the list

Arguments:

    hthd    -   Supplies thread

    Addr    -   Supplies address

    Size    -   Supplies Size

    BpType  -   Read, write, change, exec


Return Value:

    PWALK   -   Walk created

--*/
{
    PWALK   Walk;
    DWORD   i;

    EnterCriticalSection(&csWalk);

    if ( Walk = (PWALK)MHAlloc( sizeof( WALK ) ) ) {

        Walk->hthd          = hthd;
        Walk->GlobalCount   = 0;
        Walk->LocalCount    = 0;
        Walk->Active        = FALSE;
        Walk->SafetyBP      = NULL;
        Walk->BpType        = BpType;
        Walk->DataContents  = NULL;
        Walk->StartBP       = NULL;

        Walk->Method.notifyFunction = MethodWalk;
        Walk->Method.lparam = (UINT_PTR)Walk;

        InitializeListHead(&Walk->GroupEntryList);
        InsertTailList(&hthd->WalkList, &Walk->WalkList);
        InsertTailList(&AllWalkListHead, &Walk->AllWalkList);

#ifdef HAS_DEBUG_REGS

        //
        //  If we can use (or re-use) a REG_WALK structure, do so.
        //
        if (BpType == bptpRange) {

            Walk->DataAddr      = 0;
            Walk->DataSize      = 0;
            Walk->Register      = -1;
            Walk->AddrStart     = Addr;
            Walk->AddrEnd       = Addr + Size;
            Walk->HasAddrEnd    = TRUE;

        } else if (Addr == 0) {

            Walk->DataAddr      = 0;
            Walk->DataSize      = 0;
            Walk->Register      = -1;
            Walk->AddrStart     = 0;
            Walk->AddrEnd       = 0;
            Walk->HasAddrEnd    = FALSE;

        } else {

            Walk->DataAddr      = Addr;
            Walk->DataSize      = Size;
            Walk->AddrStart     = 0;
            Walk->AddrEnd       = 0;
            Walk->HasAddrEnd    = FALSE;

            for (i = 0; i < NDEBUG_REG_DATA_SIZES; i++) {
                if (Size == DebugRegDataSizes[i]) {
                    break;
                }
            }

            if (i == NDEBUG_REG_DATA_SIZES) {

                Walk->Register = -1;

            } else {

                int   Register = -1;

                for ( i=0; i < NUMBER_OF_DEBUG_REGISTERS; i++ ) {
                    if ( hthd->DebugRegs[i].ReferenceCount == 0 ) {
                        Register = i;
                    } else if ( (hthd->DebugRegs[i].DataAddr == Addr)       &&
                                (hthd->DebugRegs[i].DataSize >= Size)       &&
                                (hthd->DebugRegs[i].BpType   == BpType) ) {
                        Register = i;
                        break;
                    }
                }

                Walk->Register = Register;

                if ( Register >= 0 ) {

                    if ( hthd->DebugRegs[Register].ReferenceCount == 0 ) {

                        hthd->DebugRegs[Register].DataAddr = Addr;
                        hthd->DebugRegs[Register].DataSize = Size;
                        hthd->DebugRegs[Register].BpType   = BpType;
                        hthd->DebugRegs[Register].InUse    = FALSE;
                    }

                    hthd->DebugRegs[Register].ReferenceCount++;

                }

            }
        }
#endif

    }

    LeaveCriticalSection(&csWalk);

    return Walk;
}




BOOL
DeallocateWalk (
    PWALK   Walk
    )
/*++

Routine Description:

    Takes a walk out of the list and frees its memory.

Arguments:

    Walk    -   Supplies Walk to deallocate

Return Value:


    BOOLEAN -   TRUE if deallocated

--*/
{
    EnterCriticalSection(&csWalk);

    RemoveEntryList(&Walk->AllWalkList);
    RemoveEntryList(&Walk->WalkList);

    RemoveWalkBindings(Walk);

#ifdef HAS_DEBUG_REGS
    if ( Walk->Register >= 0 ) {
        PDEBUGREG Dr = &Walk->hthd->DebugRegs[Walk->Register];
        if (--Dr->ReferenceCount <= 0) {
            Dr->InUse = FALSE;
            ClearDebugRegister(Walk->hthd, Walk->Register);
        }
    }
#endif

    if (Walk->DataContents) {
        MHFree(Walk->DataContents);
    }

    MHFree( Walk );

    LeaveCriticalSection(&csWalk);

    return TRUE;
}




PWALK
FindWalk (
    HTHDX       hthd,
    UOFFSET     Addr,
    DWORD       Size,
    BPTP        BpType
    )
/*++

Routine Description:

    Finds a walk

Arguments:

    hthd        -   Supplies thread

    Addr        -   Supplies Address

    Size        -   Supplies Size

    BpType      -   Supplies type of BP

Return Value:

    PWALK       -   Found Walk

--*/

{
    PWALK   Walk;
    PWALK   FoundWalk = NULL;
    PLIST_ENTRY List;

    EnterCriticalSection(&csWalk);

    List = hthd->WalkList.Flink;

    while ( List != &hthd->WalkList ) {

        Walk = CONTAINING_RECORD(List, WALK, WalkList);
        List = List->Flink;

        if ( Walk->BpType == BpType ) {
            if (BpType == bptpRange) {
                if (Walk->DataAddr == Addr && Walk->DataSize == Size) {
                    FoundWalk = Walk;
                    break;
                }
            } else {
                if ((Walk->DataAddr == 0) || (Walk->DataAddr == Addr) ) {
#ifdef HAS_DEBUG_REGS
                    if ( Walk->Register == -1 ) {

                        FoundWalk = Walk;
                        break;

                    } else if ( Size <= hthd->DebugRegs[Walk->Register].DataSize )
#endif
                    {

                        FoundWalk = Walk;
                        break;
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&csWalk);

    return FoundWalk;
}


PWALK
FindWalkForHthd(
    HANDLE hWalk,
    HTHDX hthd
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/

{
    PLIST_ENTRY GroupList = (PLIST_ENTRY)hWalk;
    PWALK_GROUP_ENTRY Entry;
    PLIST_ENTRY List;
    PWALK Walk;

    List = GroupList->Flink;
    while (List != GroupList) {
        Entry = CONTAINING_RECORD(List, WALK_GROUP_ENTRY, WalksInGroup);
        List = List->Flink;
        Walk = Entry->Walk;
        if (Walk->hthd == hthd) {
            return Walk;
        }
    }
    return NULL;
}



BOOL
IsWalkInGroup(
    HANDLE hWalk,
    PVOID  pWalk
    )
{
    PLIST_ENTRY GroupList = (PLIST_ENTRY)hWalk;
    PLIST_ENTRY List;
    PWALK Walk;

    List = GroupList->Flink;
    while (List != GroupList) {
        if ( (PWALK)pWalk == CONTAINING_RECORD(List, WALK_GROUP_ENTRY, WalksInGroup)->Walk ) {
            return TRUE;
        }
        List = List->Flink;
    }
    return FALSE;
}



PBREAKPOINT
FindBpForWalk(
    PVOID pWalk
    )
{
    PBREAKPOINT pbp;

    EnterCriticalSection(&csThreadProcList);

    pbp = bpList;
    while (pbp) {
        if (pbp->hWalk && IsWalkInGroup(pbp->hWalk, pWalk)) {
            break;
        }
        pbp = pbp->next;
    }

    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}



MethodWalk(
    DEBUG_EVENT64* de,
    HTHDX        hthd,
    DWORDLONG    unused,
    DWORDLONG    lparam
    )
/*++

Routine Description:

    Walk method.

Arguments:

    de      -   Supplies debug event

    hthd    -   Supplies thread

    unused  -

    Walk    -   Supplies Walk

Return Value:

    Nothing meaningful.

--*/
{
    PWALK Walk = (PWALK)lparam;
    HPRCX hprc = hthd->hprc;

    Unreferenced( unused );

    //
    // however we got here, we don't need this BP anymore.
    //
    if (Walk->SafetyBP) {
        RemoveBP( Walk->SafetyBP );
        Walk->SafetyBP = NULL;
    }

    if (Walk->GlobalCount == 0 && Walk->LocalCount == 0) {

        //
        // Walk has been removed, now discard it.
        //
        if ( Walk->Active ) {
            ContinueThread(hthd);
        }
        DeallocateWalk( Walk );

    } else {

        //
        // is this useful?  This is for emulating one source line
        // at a time.
        //

        if (Walk->BpType == bptpRange && !Walk->HasAddrEnd ) {
            Walk->AddrEnd = GetEndOfRange( hprc, hthd, Walk->AddrStart );
            Walk->HasAddrEnd = TRUE;
        }

        //
        // First check to see if it should fire; if it should,
        // then ask the EM if it should stop.
        //

        if (CheckWalk(Walk) && CheckBpt(hthd, FindBpForWalk(Walk))) {

            //
            // tell the EM it stopped.
            //
            Walk->Active = FALSE;
            ConsumeAllThreadEvents(hthd, FALSE);
            NotifyEM(&falseBPEvent, hthd, 0,(UINT_PTR)FindBpForWalk(Walk));

        } else if (Walk->BpType == bptpRange) {

            //
            //  We still are in the range, continue stepping.
            //
            ADDR currAddr;
            int lpf;

            AddrFromHthdx(&currAddr, hthd);
            IsCall(hthd, &currAddr, &lpf, FALSE);

            if (lpf == INSTR_IS_CALL) {
                //
                // Set a safety breakpoint on the return site to prevent running
                // free over system calls and other mysterious cases.
                //
                Walk->SafetyBP = SetBP(hprc, hthd, bptpExec, bpnsStop, &currAddr,(HPID)INVALID);
            }

            SingleStep(hthd, &(Walk->Method), TRUE, FALSE);

        } else {

            //
            //  Have the Expression BP manager know that we are continuing
            //
            //ExprBPContinue( hprc, hthd );
            //
            // This just calls StartWalk instead of ExprBPContinue, to shorten
            // the code path at the expense of generality.
            // It could be shortened rather more by simply copying the
            // relevant fragments from StartWalk to here.
            //
            StartWalk( Walk, TRUE );
            ContinueThread(hthd);
        }
    }

    return TRUE;
}


VOID
DuplicateWalkBindings(
    PWALK OldWalk,
    PWALK NewWalk
    )
/*++

Routine Description:

    Copy the breakpoint bindings for a walk into another walk.
    This is used for duplicationg global watchpoints.

Arguments:

    OldWalk - Supplies a WALK which is bound to one or more breakpoints.

    NewWalk - Supplies a WALK which is to be bound to the same breakpoints
            as OldWalk.

Return Value:

    None

--*/
{
    PLIST_ENTRY OldList;
    PWALK_GROUP_ENTRY OldEntry;
    PWALK_GROUP_ENTRY NewEntry;

    //
    // Run down the list of breakpoints that OldWalk is bound to,
    // and create equivalent bindings for NewWalk.
    //

    OldList = OldWalk->GroupEntryList.Flink;

    while (OldList != &OldWalk->GroupEntryList) {

        //
        // Find each breakpoint
        //

        OldEntry = CONTAINING_RECORD(OldList, WALK_GROUP_ENTRY, GroupsUsingWalk);

        NewEntry = MHAlloc(sizeof(WALK_GROUP_ENTRY));

        NewEntry->Walk = NewWalk;

        //
        // add this to the Walk's list of BP bindings:
        //

        InsertTailList(&NewWalk->GroupEntryList, &NewEntry->GroupsUsingWalk);

        //
        // And add it to the BP's list of walks:
        //

        InsertTailList(&OldEntry->WalksInGroup, &NewEntry->WalksInGroup);
        OldList = OldList->Flink;

    }
}


VOID
RemoveWalkBindings(
    PWALK Walk
    )
/*++

Routine Description:

    Delete all of the bindings from a WALK to breakpoints (Group lists).
    The GROUP_LIST_ENTRY binding nodes will be freed.

Arguments:

    Walk - Supplies the WALK which is to be unbound.

Return Value:

    None

--*/
{

    //
    // remove all GROUP_LIST_ENTRY bindings for a walk
    //

    PLIST_ENTRY List;
    PLIST_ENTRY PossibleGroupListHead;
    PWALK_GROUP_ENTRY Entry;

    List = Walk->GroupEntryList.Flink;

    while (List != &Walk->GroupEntryList) {

        Entry = CONTAINING_RECORD(List, WALK_GROUP_ENTRY, GroupsUsingWalk);
        List = List->Flink;

        // clean this entry out of the lists...
        PossibleGroupListHead = Entry->WalksInGroup.Flink;
        RemoveEntryList(&Entry->WalksInGroup);
        if (IsListEmpty(PossibleGroupListHead)) {
            MHFree(PossibleGroupListHead);
        }

        RemoveEntryList(&Entry->GroupsUsingWalk);

        MHFree(Entry);
    }
}


VOID
AddWalkToGroupList(
    PLIST_ENTRY GroupList,
    PWALK Walk
    )
/*++

Routine Description:

    Bind a WALK to a breakpoint's walk group list.

Arguments:

    GroupList - Supplis the list header for a breakpoint's group list

    Walk - Supplies a WALK structure which is to be added to the group list.

Return Value:

    None

--*/
{
    PWALK_GROUP_ENTRY Entry;

    Entry = MHAlloc(sizeof(WALK_GROUP_ENTRY));

    Entry->Walk = Walk;
    InsertTailList(GroupList, &Entry->WalksInGroup);
    InsertTailList(&Walk->GroupEntryList, &Entry->GroupsUsingWalk);
}
