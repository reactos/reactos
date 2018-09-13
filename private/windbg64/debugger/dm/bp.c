#include "precomp.h"
#pragma hdrstop

SetFile()


BREAKPOINT      masterBP = {0L,0L};
PBREAKPOINT     bpList = &masterBP;

extern HTHDX        thdList;
extern CRITICAL_SECTION csThreadProcList;


PBREAKPOINT
GetNewBp(
    HPRCX         hprc,
    HTHDX         hthd,
    BPTP          BpType,
    BPNS          BpNotify,
    ADDR         *AddrBp,
    HPID          id,
    PBREAKPOINT   BpUse
    )
/*++

Routine Description:

    Allocates a BREAKPOINT structure and initializes it. Note that
    this does NOT add the structure to the breakpoint list (bplist).
    If it is not an address bp (i.e. it is a watchpoint), the hwalk
    field must be initialized later.

Arguments:

    hprc    - Supplies process to put BP in

    hthd    - Supplies optional thread

    AddrBp  - Supplies address structure for the breakpoint

    id      - Supplies EM id for BP

    BpUse   - (optional) Supplies other BP on same address (so we can steal
                the original code from it instead of reading).

Return Value:

    PBREAKPOINT      -   Pointer to allocated and initialized structure.

--*/

{
    PBREAKPOINT Bp;
    DWORD       i;

    assert( !BpUse || ( BpUse->hthd != hthd ) || (BpUse->bpNotify != BpNotify) );

    Bp = (PBREAKPOINT)MHAlloc(sizeof(BREAKPOINT));
    assert( Bp );

    if ( Bp ) {

        assert( bpList );

        Bp->next        = NULL;
        Bp->hprc        = hprc;
        Bp->hthd        = hthd;
        Bp->id          = id;
        Bp->instances   = 1;
        Bp->isStep      = FALSE;
        Bp->hBreakPoint = 0;
        Bp->bpType      = BpType;
        Bp->bpNotify    = BpNotify;
        Bp->hWalk       = NULL;
#if defined(TARGET_IA64)
        Bp->flags       = 1;  
#endif
        memset(&Bp->addr, 0, sizeof(Bp->addr));

        //
        // Get the opcode from the indicated address
        //

        if (AddrBp) {

            assert(!ADDR_IS_LI(*AddrBp));
            if (ADDR_IS_LI(*AddrBp)) {
                MHFree(Bp);
                Bp = NULL;
            } else {

                Bp->instr1      = 0;
                Bp->addr        = *AddrBp;

                if ( BpUse ) {

                    Bp->instr1 = BpUse->instr1;

                } else if (!AddrReadMemory(hprc, hthd, AddrBp, &(Bp->instr1), BP_SIZE, &i) ||
                               (i != BP_SIZE)) {

#ifdef KERNEL
                    Bp->instr1 = 0;
#else
                    assert(!"AddrReadMemory failed");
                    MHFree(Bp);
                    Bp = NULL;
#endif
                }
            }
        }
    }

    return Bp;
}



PBREAKPOINT
SetBP(
    HPRCX     hprc,
    HTHDX     hthd,
    BPTP      bptype,
    BPNS      bpnotify,
    LPADDR    paddr,
    HPID      id
    )
/*++

Routine Description:

    Set a breakpoint, or increment instance count on an existing bp.
    if hthd is non-NULL, BP is only for that thread.

Arguments:

    hprc  - Supplies process to put BP in

    hthd  - Supplies optional thread

    bptype - Supplies OSDEBUG BP type

    bpnotify - Supplies OSDEBUG notification code

    paddr - Supplies address structure for the breakpoint

    id    - Supplies EM id for BP

Return Value:

    pointer to bp structure, or NULL for failure

--*/
{
    PBREAKPOINT pbp;
    PBREAKPOINT pbpT;
    ADDR        addr;
    ADDR        addr2;

    if (!hprc) {
        return (PBREAKPOINT)NULL;
    }

    EnterCriticalSection(&csThreadProcList);

    /*
     * First let's try to find a breakpoint that
     * matches this description
     */

    pbpT = FindBP(hprc, hthd, bptype, bpnotify, paddr, FALSE);

    /*
     * If this thread has a breakpoint here,
     * increment reference count.
     */

    if (pbpT &&
        pbpT->hthd == hthd &&
        pbpT->bpNotify == bpnotify
        ) {

        pbp = pbpT;
        pbp->instances++;

    } else if ( pbp = GetNewBp( hprc, hthd, bptype, bpnotify, paddr, id, pbpT )) {

        if ( pbpT ) {
            AddBpToList(pbp);           // if already a bp at that addr, just add to list
        } else {

            //
            //  Now write the cpu-specific breakpoint code.
            //
            if ( WriteBreakPoint(pbp) ) {
                AddBpToList(pbp);
            } else {
                MHFree( pbp );
                pbp = NULL;
            }
        }


        /*
         * Make it a linear address to start with
         */

        addr2 = *paddr;
        TranslateAddress(hprc, hthd, &addr2, TRUE);

        /*
         * Check with the threads to see if we are at this address. If so then
         *  we need to set the BP field so we don't hit the bp imeadiately
         */

        if (hthd) {
            AddrFromHthdx(&addr, hthd);
            if ((hthd->tstate & ts_stopped) &&
                (AtBP(hthd) == NULL) &&
                AreAddrsEqual(hprc, hthd, &addr, &addr2)) {
                SetBPFlag(hthd, pbp);
            }
        } else {
            for (hthd = hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                AddrFromHthdx(&addr, hthd);
                if ((hthd->tstate & ts_stopped) &&
                    (AtBP(hthd) == NULL) &&
                    AreAddrsEqual(hprc, hthd, &addr, &addr2)) {
                    SetBPFlag(hthd, pbp);
                }
            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* SetBP() */


#ifdef KERNEL


BOOL
SetBPEx(
    HPRCX         hprc,
    HTHDX         hthd,
    HPID          id,
    DWORD         Count,
    ADDR         *Addrs,
    PBREAKPOINT  *Bps,
    DWORD         ContinueStatus
    )

/*++

Routine Description:

    Allocates a bunch of breakpoints from a given list of linear offsets.

Arguments:

    hprc    - Supplies process to put BP in

    hthd    - Supplies optional thread

    Count   - Supplies count of breakpoints to set

    Addrs   - Supplies list with Count addresses

    Bps     - Supplies buffer to be filled with Count pointers to
                       BREAKPOINT structures.  Original contents are
                       overwritten.

    ContinueStatus -

Return Value:

    BOOL    -   If TRUE, then ALL breakpoints were set.
                If FALSE, then NONE of the breakpoints were set.


    NOTENOTE - Not sure of what will happen if the list contains duplicated
               addresses!

--*/

{
    PDBGKD_WRITE_BREAKPOINT64   DbgKdBp;
    PDBGKD_RESTORE_BREAKPOINT   DbgKdBpRes;
    DWORD                       SetCount = 0;
    DWORD                       NewCount = 0;
    DWORD                       i;
    DWORD                       j;
    PBREAKPOINT                 BpT;
    BOOL                        Ok;
    ADDR                        Addr;
    ADDR                        Addr2;

    if (!hprc) {
        assert(!"hprc == NULL is SetBPEx");
        return FALSE;
    }

    assert( Count > 0 );
    assert( Addrs );
    assert( Bps );

    if ( Count == 1 ) {
        //
        //  Only one breakpoint, faster to simply call SetBP
        //
        Bps[0] = SetBP( hprc, hthd, bptpExec, bpnsStop, &Addrs[0], id );
        return ( Bps[0] != NULL );
    }

    EnterCriticalSection(&csThreadProcList);

    AddrInit( &Addr, 0, 0, 0, TRUE, TRUE, FALSE, FALSE );

    //
    //  Allocate space for Count breakpoints
    //
    DbgKdBp = (PDBGKD_WRITE_BREAKPOINT64)
                            MHAlloc( sizeof(DBGKD_WRITE_BREAKPOINT64) * Count );
    assert( DbgKdBp );

    if ( !DbgKdBp ) {
        LeaveCriticalSection(&csThreadProcList);
        return FALSE;
    }

    for ( i=0; i<Count; i++ ) {

        //
        //  See if we already have a breakpoint at this address.
        //
        BpT = FindBP( hprc, hthd, bptpExec, bpnsStop, &Addrs[i], FALSE );

        if (BpT &&
            BpT->hthd == hthd &&
            BpT->bpNotify == bpnsStop
            ) {

            //
            // exact match: just bump the instance count
            //

            Bps[i] = BpT;
            Bps[i]->instances++;

        } else if (BpT) {

            //
            // address matched: keep the old BP handle, make a new record
            //

            Bps[i] = GetNewBp( hprc, hthd, bptpExec, bpnsStop, &Addrs[i], id, NULL );
            Bps[i]->hBreakPoint = BpT->hBreakPoint;

        } else {

            //
            // no match: need a new BP
            //

            Bps[i] = GetNewBp( hprc, hthd, bptpExec, bpnsStop, &Addrs[i], id, NULL );
            assert( Bps[i] );

            //
            // set instance to 0 to indicate it is unset
            //
            Bps[i]->instances = 0;

            DbgKdBp[ NewCount ].BreakPointAddress = GetAddrOff(Addrs[i]);
            DbgKdBp[ NewCount ].BreakPointHandle  = 0;

            //
            // keep BP packet index in handle field until BP is set
            //
            Bps[i]->hBreakPoint = NewCount++;
        }
    }

    Ok = TRUE;
    if ( NewCount > 0 ) {

        //
        //  Set all new breakpoints
        //
        assert( NewCount <= Count );
        Ok = WriteBreakPointEx( hthd, NewCount, DbgKdBp, ContinueStatus );
    }

    if ( Ok ) {
        //
        //  Fill in the breakpoint list
        //
        for ( i=0; i<Count; i++ ) {

            if ( Bps[i] && Bps[i]->instances == 0) {

                j = Bps[i]->hBreakPoint;

                assert( GetAddrOff(Addrs[i]) ==
                                         DbgKdBp[j].BreakPointAddress );
                //
                //  Allocate new BP structure and get handle from
                //  the breakpoint packet.
                //

                Bps[i]->hBreakPoint = DbgKdBp[j].BreakPointHandle;
                Bps[i]->instances = 1;
                AddBpToList(Bps[i]);
            }

            SetCount++;

            //
            //  Check with the threads to see if we are at this address.
            //  If so then we need to set the BP field so we don't hit
            //  the bp imeadiately
            //

            Addr2 = Bps[i]->addr;

            if ( hthd ) {
                AddrFromHthdx( &Addr, hthd );
                if ((hthd->tstate & ts_stopped) &&
                    (AtBP(hthd) == NULL) &&
                    AreAddrsEqual(hprc, hthd, &Addr, &Addr2 )) {
                    SetBPFlag(hthd, Bps[i]);
                }
            } else {
                for (hthd=hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                    AddrFromHthdx( &Addr, hthd );
                    if ((hthd->tstate & ts_stopped) &&
                        (AtBP(hthd) == NULL) &&
                        AreAddrsEqual(hprc, hthd, &Addr, &Addr2)) {
                        SetBPFlag(hthd, Bps[i]);
                    }
                }
            }
        }

        assert( j == NewCount );

    } else {

        //
        //  Clean up any breakpoints that were set.
        //
        DbgKdBpRes = (PDBGKD_RESTORE_BREAKPOINT)
                    MHAlloc( sizeof(DBGKD_RESTORE_BREAKPOINT) * NewCount );
        assert( DbgKdBpRes );

        if ( DbgKdBpRes ) {

            //
            //  Put all breakpoints with a valid handle on the list of
            //  breakpoints to be removed.
            //
            j = 0;
            for ( i=0; i<NewCount;i++) {
                if ( DbgKdBp[i].BreakPointHandle != 0 ) {
                    DbgKdBpRes[j++].BreakPointHandle = DbgKdBp[i].BreakPointHandle;
                }
            }

            //
            //  Now remove them
            //
            if ( j > 0 ) {
                assert( j <= NewCount );
                RestoreBreakPointEx( j, DbgKdBpRes );
            }

            MHFree( DbgKdBpRes );

            //
            //  Remove allocated BP structures
            //
            for ( i=0; i<Count; i++ ) {
                if ( Bps[i] && Bps[i]->instances == 0 ) {
                    assert( !Bps[i]->next );
                    MHFree( Bps[i] );
                    Bps[i] = NULL;
                }
           }
        }
    }

    MHFree( DbgKdBp );

    LeaveCriticalSection(&csThreadProcList);

    return (SetCount == Count);
}


#else   // KERNEL

BOOL
SetBPEx(
    HPRCX         hprc,
    HTHDX         hthd,
    HPID          id,
    DWORD         Count,
    ADDR         *Addrs,
    PBREAKPOINT  *Bps,
    DWORD         ContinueStatus
    )

/*++

Routine Description:

    Allocates a bunch of breakpoints from a given list of linear offsets.

Arguments:

    hprc    - Supplies process to put BP in

    hthd    - Supplies optional thread

    Count   - Supplies count of breakpoints to set

    Addrs   - Supplies list with Count addresses

    Bps     - Supplies buffer to be filled with Count pointers to
                       BREAKPOINT structures.  Original contents is
                       overwritten.

    ContinueStatus -

Return Value:

    BOOL    -   If TRUE, then ALL breakpoints were set.
                If FALSE, then NONE of the breakpoints were set.


    NOTENOTE - Not sure of what will happen if the list contains duplicated
               addresses!

--*/

{
    DWORD       SetCount = 0;
    DWORD       NewCount = 0;
    DWORD       i;
    DWORD       j;
    DWORD       cbBytes;
    ADDR        Addr;
    ADDR        Addr2;
    PBREAKPOINT BpT;

    if (!hprc) {
        assert(!"hprc == NULL in SetBPEx");
        return FALSE;
    }

    assert( Count > 0 );
    assert( Addrs );
    assert( Bps );

    if ( Count == 1 ) {
        //
        //  Only one breakpoint, faster to simply call SetBP
        //
        Bps[0] = SetBP( hprc, hthd, bptpExec, bpnsStop, &Addrs[0], id );
        return ( Bps[0] != NULL );
    }

    EnterCriticalSection(&csThreadProcList);

    for ( i=0; i<Count; i++ ) {

        //
        //  See if we already have a breakpoint at this address.
        //
        BpT = FindBP( hprc, hthd, bptpExec, bpnsStop, &Addrs[i], FALSE );

        if (BpT &&
            BpT->hthd == hthd &&
            BpT->bpNotify == bpnsStop
            ) {

            //
            //  Reuse this breakpoint
            //

            Bps[i]->instances++;
            assert( Bps[i]->instances > 1 );

        } else {

            //
            //  Get new breakpoint
            //

            Bps[i] = GetNewBp(hprc, hthd, bptpExec, bpnsStop, &Addrs[i], id, BpT);

            if ( !Bps[i] ) {
                assert(!"GetNewBp failed in SetBPEx");
                break;
            }

            if (!BpT) {
                if (!WriteBreakPoint(Bps[i])) {
                    MHFree( Bps[i] );
                    Bps[i] = NULL;
                    assert(!"WriteBreakPoint failed in SetBPEx");
                    break;
                }
            }
        }
    }

    if ( i < Count ) {
        //
        //  Something went wrong, will backtrack
        //

        assert(!"i < Count in SetBPEx");

        for ( j=0; j<i; j++ ) {

            assert( Bps[j] );
            Bps[j]->instances--;
            if ( Bps[j]->instances == 0 ) {

                if ( !ADDR_IS_LI(Bps[j]->addr) ) {
#if defined(TARGET_IA64)
                    BP_UNIT     Content;
                    ADDR        BundleAddr;
                                                 
                    // Read in memory since adjancent instructions in the same bundle may have 
                    // been modified after we save them. Restore only the content of the slot which has 
                    // the break instruction inserted.
         
                    AddrReadMemory(hprc, hthd, &Bps[j]->addr, (LPBYTE) &Content, BP_SIZE, &cbBytes);
         
                    switch (GetAddrOff(Bps[j]->addr) & 0xf) {
                        case 0:
                            Content = (Content & ~(INST_SLOT0_MASK)) | (Bps[j]->instr1 & INST_SLOT0_MASK);
                            break;
                        case 4:
                            Content = (Content & ~(INST_SLOT1_MASK)) | (Bps[j]->instr1 & INST_SLOT1_MASK);
                            break;
                        case 8:
                            Content = (Content & ~(INST_SLOT2_MASK)) | (Bps[j]->instr1 & INST_SLOT2_MASK);
                            break;
                        default:
                            break;
                     }
                             
                     AddrWriteMemory(hprc, hthd, &Bps[j]->addr, (LPBYTE) &Content, BP_SIZE, &cbBytes);
         
                     // restore template to MLI if displaced instruction was MOVL
                     if (Bps[j]->flags & BREAKPOINT_IA64_MOVL) {
                        GetAddrOff(BundleAddr) = GetAddrOff(Bps[j]->addr) & ~0xf;
                        AddrReadMemory(hprc, hthd, &BundleAddr,(LPBYTE) &Content, BP_SIZE, &cbBytes);
         
                        Content &= ~((INST_TEMPL_MASK >> 1) << 1);  // set template to MLI
                        Content |= 0x4;
         
                        AddrWriteMemory(hprc, hthd, &BundleAddr,(LPBYTE) &Content, BP_SIZE, &cbBytes);
                     }
#else
                    AddrWriteMemory(hprc, hthd, &Bps[j]->addr,
                                      (LPBYTE) &Bps[j]->instr1, BP_SIZE, &cbBytes);
#endif
                }
                MHFree( Bps[j] );
                Bps[j] = NULL;
            }
        }

    } else {

        //
        //  Add all the new breakpoints to the list
        //
        for ( i=0; i<Count; i++ ) {

            if ( Bps[i]->instances == 1 ) {
                AddBpToList(Bps[i]);
            }

            //
            //  Check with the threads to see if we are at this address. If so then
            //  we need to set the BP field so we don't hit the bp imeadiately
            //

            Addr2 = Bps[i]->addr;

            if ( hthd ) {
                AddrFromHthdx( &Addr, hthd );
                if ((hthd->tstate & ts_stopped) &&
                    (AtBP(hthd) == NULL) &&
                    AreAddrsEqual(hprc, hthd, &Addr, &Addr2 )) {
                    SetBPFlag(hthd, Bps[i]);
                }
            } else {
                for (hthd=hprc->hthdChild; hthd; hthd = hthd->nextSibling) {
                    AddrFromHthdx( &Addr, hthd );
                    if ((hthd->tstate & ts_stopped) &&
                        (AtBP(hthd) == NULL) &&
                        AreAddrsEqual(hprc, hthd, &Addr, &Addr2)) {
                        SetBPFlag(hthd, Bps[i]);
                    }
                }
            }
        }

        SetCount = Count;
    }

    LeaveCriticalSection(&csThreadProcList);

    return (SetCount == Count);
}

#endif // KERNEL

BOOL
BPInRange(
    HPRCX         hprc,
    HTHDX         hthd,
    PBREAKPOINT   bp,
    LPADDR        paddrStart,
    DWORD         cb,
    LPDWORD       offset,
    BP_UNIT     * instr
    )
{
    ADDR        addr1;
    ADDR        addr2;

    /*
     * If the breakpoint has a Loader index address then we can not
     *  possibly match it
     */

    assert (!ADDR_IS_LI(*paddrStart) );
    if (ADDR_IS_LI(bp->addr)) {
        return FALSE;
    }

    *offset = 0;

    /*
     * Now check for "equality" of the addresses.
     *
     *     Need to include size of BP in the address range check.  Since
     *  the address may start half way through a breakpoint.
     */

    if ((ADDR_IS_FLAT(*paddrStart) == TRUE) &&
        (ADDR_IS_FLAT(bp->addr) == TRUE)) {
#if defined(TARGET_IA64)
        //
        // This is a bit different for IA64 because intraslot addresses 
        // are "quasi" addresses
        //
        if (
            // we have a match if the BP start within the read memory range
            ((GetAddrOff(bp->addr) >= GetAddrOff(*paddrStart)) && 
             (GetAddrOff(bp->addr) < GetAddrOff(*paddrStart) + cb)) ||

            // or we have a match if the range starts within the BP itself -
            // notice - the length of BP is 4, not sizeof(BP_UNIT) which is 8
            ((GetAddrOff(*paddrStart) >= GetAddrOff(bp->addr)) &&
             (GetAddrOff(*paddrStart) < GetAddrOff(bp->addr) + 4))
            ) {
#else
        if ((GetAddrOff(*paddrStart) - sizeof(BP_UNIT) + 1 <=
                GetAddrOff(bp->addr)) &&
            (GetAddrOff(bp->addr) < GetAddrOff(*paddrStart) + cb)) {
#endif
            *offset = (DWORD)(GetAddrOff(bp->addr) - GetAddrOff(*paddrStart));
            *instr = bp->instr1;
            return TRUE;
        }
        return FALSE;
    }

    /*
     * The two addresses did not start out as flat addresses.  So change
     *  them to linear addresses so that we can see if the addresses are
     *  are really the same
     */

    addr1 = *paddrStart;
    if (!TranslateAddress(hprc, hthd, &addr1, TRUE)) {
        return FALSE;
    }
    addr2 = bp->addr;
    if (!TranslateAddress(hprc, hthd, &addr2, TRUE)) {
        return FALSE;
    }

#if defined(TARGET_IA64) //same as above
        if ( 
            ((GetAddrOff(addr2) >= GetAddrOff(addr1)) &&
             (GetAddrOff(addr2) < GetAddrOff(addr1) + cb)) ||
            ((GetAddrOff(addr1) >= GetAddrOff(addr2)) &&
             (GetAddrOff(addr1) < GetAddrOff(addr2) + 4))
            ) {
#else
    if ((GetAddrOff(addr1) - sizeof(BP_UNIT) + 1 <= GetAddrOff(addr2)) &&
        (GetAddrOff(addr2) < GetAddrOff(addr1) + cb)) {
#endif
        *offset = (DWORD)(GetAddrOff(addr2) - GetAddrOff(addr1));
        *instr = bp->instr1;
        return TRUE;
    }

    return FALSE;
}


BOOL
BPPriorityIsGreater(
    BPNS n1,
    BPNS n2
    )
{
    if (n1 == bpnsStop) {
        return (n2 == bpnsContinue) || (n2 == bpnsCheck);
    } else if (n1 == bpnsCheck) {
        return (n2 == bpnsContinue);
    } else {
        return FALSE;
    }
}


#if 0
PBREAKPOINT
FindBP(
    HPRCX    hprc,
    HTHDX    hthd,
    BPTP     bptype,
    BPNS     bpnotify,
    LPADDR   paddr,
    BOOL     fExact
    )
/*++

Routine Description:

    Find and return a pointer to a BP struct.

    This is called for the following cases:

    1) A breakpoint has been hit, and we want to find a BP record for it.
       In this case, we want to see the highest priority match, with
       preference to the correct thread.

       Call with bpnotify == -1, hthd == xxxx, fExact == FALSE

    2) We want to step off/through a BP.  Any match will do, we just need
       the instruction.

       Call with bpnotify == -1, hthd == 0, fExact == FALSE

    3) We want to delete a BP.  Everything must match exactly.  (It would
       be better to do this directly by the pointer to the BP, but the
       shells do not support this properly.)

       Call with bpnotify == bpnsXXXX, hthd == xxxx, fExact == TRUE
       (N.B. hthd may be NULL, which means "any thread")

    Always returns a BP that matches hthd and bpnotify if one exists; if
    fExact is FALSE and there is no exact match, a BP matching only hprc
    and address will succeed.


Arguments:

    hprc   - Supplies process

    hthd   - Supplies thread

    bptype - Supplies OSDEBUG BP type

    bpnotify -

    paddr  - Supplies address

    fExact - Supplies TRUE if must be for a certain thread

Return Value:

    pointer to BREAKPOINT struct, or NULL if none found.

--*/
{
    PBREAKPOINT  pbp;
    PBREAKPOINT  pbpFound = NULL;
    ADDR         addr;

    EnterCriticalSection(&csThreadProcList);

    /*
     * Pre-translate the address to a linear address
     */

    addr = *paddr;
    TranslateAddress(hprc, hthd, &addr, TRUE);

    //
    // Check for an equivalent breakpoint.  Breakpoints will be equal if
    //
    //  1.  The process must be the same
    //  2.  The BP type must be the same
    //  3.  a) if it is an exec BP the addresses must match
    //      b) if not, MatchWalk is called
    //  4.  The thread and notify types must match if fExact is specified
    //

    for (pbp=bpList->next; pbp; pbp=pbp->next) {
        if ((pbp->hprc == hprc) && (bptype == pbp->bpType)) {
            if (bptype == bptpExec && AreAddrsEqual(hprc, hthd, &pbp->addr, &addr)) {

                //
                // if it matches exactly, take it now.
                // if not, take it if it is better than the
                // previous partial match
                //

                if (hthd == pbp->hthd && bpnotify == pbp->bpNotify) {
                    pbpFound = pbp;
                    break;
                }

                if (!pbpFound) {
                    //
                    // any match is better than none
                    //

                    pbpFound = pbp;

                } else if (pbp->hthd == NULL || pbp->hthd == hthd) {
                    //
                    // this thread matches:
                    //   is it better than the one we had?
                    //
                    if (hthd != pbpFound->hthd) {
                        pbpFound = pbp;
                    } else if (bpnotify == pbp->bpNotify && bpnotify != pbpFound->bpNotify) {
                        pbpFound = pbp;
                    } else if ((bpnotify == (BPNS)-1) &&
                            BPPriorityIsGreater(pbp->bpNotify, pbpFound->bpNotify)) {
                        pbpFound = pbp;
                    }
                }
            }
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    if (!fExact || (
            pbpFound &&
            (pbpFound->hthd == hthd) &&
            (pbpFound->bpNotify == bpnotify) ) )
    {
        return pbpFound;
    } else {
        return NULL;
    }
}                               /* FindBP() */

#endif

PBREAKPOINT
FindBP(
    HPRCX    hprc,
    HTHDX    hthd,
    BPTP     bptype,
    BPNS     bpnotify,
    LPADDR   paddr,
    BOOL     fExact
    )
/*++

Routine Description:

    Find and return a pointer to a BP struct.

    This is called for the following cases:

    1) A breakpoint has been hit, and we want to find a BP record for it.
       In this case, we want to see the highest priority match, with
       preference to the correct thread.

       Call with bpnotify == -1, hthd == xxxx, fExact == FALSE

    2) We want to step off/through a BP.  Any match will do, we just need
       the instruction.

       Call with bpnotify == -1, hthd == 0, fExact == FALSE

    3) We want to delete a BP.  Everything must match exactly.  (It would
       be better to do this directly by the pointer to the BP, but the
       shells do not support this properly.)

       Call with bpnotify == bpnsXXXX, hthd == xxxx, fExact == TRUE
       (N.B. hthd may be NULL, which means "any thread")

    Always returns a BP that matches hthd and bpnotify if one exists; if
    fExact is FALSE and there is no exact match, a BP matching only hprc
    and address will succeed.


Arguments:

    hprc   - Supplies process

    hthd   - Supplies thread

    bptype - Supplies OSDEBUG BP type

    bpnotify -

    paddr  - Supplies address

    fExact - Supplies TRUE if must be for a certain thread

Return Value:

    pointer to BREAKPOINT struct, or NULL if none found.

--*/
{
    PBREAKPOINT  pbp;
    PBREAKPOINT  pbpFound = NULL;
    ADDR         addr;

    EnterCriticalSection(&csThreadProcList);

    /*
     * Pre-translate the address to a linear address
     */

    addr = *paddr;
    TranslateAddress(hprc, hthd, &addr, TRUE);

    //
    // Check for an equivalent breakpoint.  Breakpoints will be equal if
    //
    //  1.  The process must be the same
    //  2.  The BP type must be the same
    //  3.  a) if it is an exec BP the addresses must match
    //      b) if it is a message BP, the addresses must match
    //      c) other BP types are not supported at this time
    //  4.  The thread and notify types must match if fExact is specified
    //

    for (pbp=bpList->next; pbp; pbp=pbp->next)
    {
        if ((pbp->hprc == hprc) && (bptype == pbp->bpType))
        {
            switch (bptype) {

                case bptpMessage:
                case bptpExec:

                    if (AreAddrsEqual(hprc, hthd, &pbp->addr, &addr))
                    {
                        //
                        // if it matches exactly, take it now.
                        // if not, take it if it is better than the
                        // previous partial match
                        //

                        if (hthd == pbp->hthd && bpnotify == pbp->bpNotify) {
                            pbpFound = pbp;
                            goto out_of_loop;
                        }

                        if (!pbpFound) {

                            //
                            // any match is better than none
                            //

                            pbpFound = pbp;

                        } else if (pbp->hthd == NULL || pbp->hthd == hthd) {

                            //
                            // this thread matches:
                            //   is it better than the one we had?
                            //

                            if (hthd != pbpFound->hthd)
                            {
                                pbpFound = pbp;
                            }
                            else if (bpnotify == pbp->bpNotify &&
                                     bpnotify != pbpFound->bpNotify)
                            {
                                pbpFound = pbp;
                            }
                            else if ((bpnotify == (BPNS)-1) &&
                                     BPPriorityIsGreater (pbp->bpNotify,
                                                          pbpFound->bpNotify)
                                    )
                            {
                                pbpFound = pbp;
                            }
                        }
                    }

                break;

                default:

                    // At this time FindBP () only supports types Message and
                    // exec.  Add your matching code this this switch stmt.

                    assert (FALSE);
            }
        }
    }

out_of_loop:

    LeaveCriticalSection(&csThreadProcList);

    if (!fExact || (
            pbpFound &&
            (pbpFound->hthd == hthd) &&
            (pbpFound->bpNotify == bpnotify) ) )
    {
        return pbpFound;
    } else {
        return NULL;
    }
}                               /* FindBP() */


PBREAKPOINT
BPNextHprcPbp(
    HPRCX        hprc,
    PBREAKPOINT  pbp
    )

/*++

Routine Description:

    Find the next breakpoint for the given process after pbp.
    If pbp is NULL start at the front of the list, for a find
    first, find next behaviour.


Arguments:

    hprc    - Supplies the process handle to match breakpoints for

    pbp     - Supplies pointer to breakpoint item to start searching after

Return Value:

    NULL if no matching breakpoint is found else a pointer to the
    matching breakpoint

--*/

{
    EnterCriticalSection(&csThreadProcList);
    if (pbp == NULL) {
        pbp = bpList->next;
    } else {
        pbp = pbp->next;
    }

    for ( ; pbp; pbp = pbp->next ) {
        if (pbp->hprc == hprc) {
            break;
        }
    }
    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* BPNextHprcPbp() */


PBREAKPOINT
BPNextHthdPbp(
    HTHDX        hthd,
    PBREAKPOINT  pbp
    )
/*++

Routine Description:

    Find the next breakpoint for the given thread after pbp.
    If pbp is NULL start at the front of the list for find
    first, find next behaviour.

Arguments:

    hthd    - Supplies the thread handle to match breakpoints for

    pbp     - Supplies pointer to breakpoint item to start searching after

Return Value:

    NULL if no matching breakpoint is found else a pointer to the
    matching breakpoint

--*/

{
    EnterCriticalSection(&csThreadProcList);

    if (pbp == NULL) {
        pbp = bpList->next;
    } else {
        pbp = pbp->next;
    }

    for ( ; pbp; pbp = pbp->next ) {
        if (pbp->hthd == hthd) {
            break;
        }
    }

    LeaveCriticalSection(&csThreadProcList);

    return pbp;
}                               /* BPNextHthdPbp() */




BOOL
RemoveBPHelper(
    PBREAKPOINT pbp,
    BOOL        fRestore
    )
{
    PBREAKPOINT         pbpPrev;
    PBREAKPOINT         pbpCur;
    PBREAKPOINT         pbpT;
    HTHDX               hthd;
    BOOL                rVal = FALSE;


    //
    // first, is it real?
    //
    if (!pbp || pbp == EMBEDDED_BP) {
        return FALSE;
    }

    EnterCriticalSection(&csThreadProcList);

    /* Decrement the instances counter      */
    if (--pbp->instances) {

        /*
         * BUGBUG:  jimsch -- Feb 29 1993
         *    This piece of code is most likely incorrect.  We need to
         *      know if we are the DM freeing a breakpoint or the user
         *      freeing a breakpoint before we clear the step bit.  Otherwise
         *      we may be in the following situation
         *
         *      Set a thread specific breakpoint on an address
         *      Step the thread so that the address is the destination is
         *              where the step ends up (but it takes some time such
         *              as over a function call)
         *      Clear the thread specific breakpoint
         *
         *      This will cause the step breakpoint to be cleared so we will
         *      stop at the address instead of just continuing stepping.
         */

        pbp->isStep = FALSE;
        LeaveCriticalSection(&csThreadProcList);
        return FALSE;
    }

    /* Search the list for the specified breakpoint */


    for (   pbpPrev = bpList, pbpCur = bpList->next;
            pbpCur;
            pbpPrev = pbpCur, pbpCur = pbpCur->next) {

        if (pbpCur == pbp)  {

            /*
             * Remove this bp from the list:
             */

            pbpPrev->next = pbpCur->next;

            //
            // pbpT will be used later to replace atBP
            // in any thread which happens to be sitting
            // on the BP which we are removing.
            //
            if (pbpCur->bpType != bptpExec && pbpCur->bpType != bptpMessage) {

                pbpT = NULL;

            } else {
                //
                // see if there is another bp on the same address:
                //

                pbpT = FindBP(pbpCur->hprc,
                              pbpCur->hthd,
                              pbpCur->bpType,
                              (BPNS)-1,
                              &pbpCur->addr,
                              FALSE);

                if (!pbpT && (pbpCur->bpType == bptpExec ||
                        pbpCur->bpType == bptpMessage))
                {
                    //
                    // if this was the only one, put the
                    // opcode back where it belongs.
                    //

                    if ( fRestore ) {
                        RestoreBreakPoint( pbpCur );
                    }
                }
            }

            if (pbpCur->hWalk) {
                RemoveWalk(pbpCur->hWalk, pbpCur->hthd == NULL);
            }

            //
            // Now we have to go through all the threads to see
            // if any of them are on this breakpoint and clear
            // the breakpoint indicator on these threads
            //

            //
            // Could be on any thread:
            //

            //
            // (We are already in the ThreadProcList critical section)
            //

            for (hthd = thdList->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse;
                 hthd;
                 hthd = hthd->nextGlobalThreadThisProbablyIsntTheOneYouWantedToUse) {
                if (hthd->atBP == pbpCur) {
                    hthd->atBP = pbpT;
                }
            }

            MHFree(pbpCur);
            rVal = TRUE;
            break;
        }

    }

    LeaveCriticalSection(&csThreadProcList);

    return rVal;

}


BOOL
RemoveAllHprcBP(
    HPRCX hprc
    )
{
    PBREAKPOINT pbp, pbpT;

    for (pbp = BPNextHprcPbp(hprc, NULL); pbp; pbp = pbpT) {
        BYTE count = pbp->instances;
        pbpT = BPNextHprcPbp(hprc, pbp);
        while (count--) {
            RemoveBPHelper(pbp, TRUE);
        }
    }

    // All bps for this process should be cleared.
    assert(BPNextHprcPbp(hprc,NULL) == NULL);

    return TRUE;
}


BOOL
RemoveBP(
    PBREAKPOINT pbp
    )
{
    return RemoveBPHelper( pbp, TRUE );
}

#ifdef KERNEL

BOOL
RemoveBPEx(
    DWORD       Count,
    PBREAKPOINT *Bps
    )
{

    PDBGKD_RESTORE_BREAKPOINT   DbgKdBp;
    DWORD                       RestoreCount = 0;
    DWORD                       GoneCount    = 0;
    DWORD                       i;
    PBREAKPOINT                 BpCur;
    PBREAKPOINT                 BpOther;

    assert( Count > 0 );

    if ( Count == 1 ) {
        //
        //  Only one breakpoint, its faster to simply call RemoveBP
        //
        return RemoveBP( Bps[0] );
    }

    EnterCriticalSection(&csThreadProcList);

    DbgKdBp = (PDBGKD_RESTORE_BREAKPOINT)MHAlloc( sizeof(DBGKD_RESTORE_BREAKPOINT) * Count );
    assert( DbgKdBp );

    if ( DbgKdBp ) {

        //
        //  Find out what breakpoints we have to restore and put them in
        //  the list.
        //
        for ( i=0; i<Count;i++ ) {

            assert( Bps[i] != EMBEDDED_BP );

            for (   BpCur = bpList->next; BpCur; BpCur = BpCur->next) {

                if ( BpCur == Bps[i] )  {

                    //
                    // See if there is another bp on the same address.
                    //
                    for ( BpOther = bpList->next; BpOther; BpOther = BpOther->next ) {
                        if ( (BpOther != BpCur) &&
                             AreAddrsEqual( BpCur->hprc, BpCur->hthd, &BpCur->addr, &BpOther->addr ) ) {
                            break;
                        }
                    }

                    if ( !BpOther ) {
                        //
                        // If this was the only one, put it in the list.
                        //
                        DbgKdBp[GoneCount++].BreakPointHandle = Bps[i]->hBreakPoint;
                    }

                    break;
                }
            }
        }

        //
        //  Restore the breakpoints in the list.
        //
        if ( GoneCount > 0 ) {
            assert( GoneCount <= Count );
            RestoreBreakPointEx( GoneCount, DbgKdBp );
        }

        //
        //  All breakpoints that were to be restored have been
        //  restored, now go ahead and do the cleaning up stuff.
        //
        for ( i=0; i<Count;i++ ) {
            RemoveBPHelper( Bps[i], FALSE );
            RestoreCount++;
        }

        MHFree( DbgKdBp );
    }

    LeaveCriticalSection(&csThreadProcList);

    return ( RestoreCount == Count );
}

#else // KERNEL

BOOL
RemoveBPEx(
    DWORD       Count,
    PBREAKPOINT *Bps
    )
{

    DWORD                       i;

    assert( Count > 0 );

        for ( i=0; i<Count;i++ ) {

        RemoveBPHelper( Bps[i], TRUE );
    }

    return TRUE;
}

#endif // KERNEL


void
SetBPFlag(
    HTHDX hthd,
    PBREAKPOINT bp
    )
{
    hthd->atBP = bp;
}



PBREAKPOINT
AtBP(
    HTHDX hthd
    )
{
    return hthd->atBP;
}




void
ClearBPFlag(
    HTHDX hthd
    )
{
    hthd->atBP = NULL;
}



void
RestoreInstrBP(
    HTHDX            hthd,
    PBREAKPOINT      bp
    )
/*++

Routine Description:

    Replace the instruction for a breakpoint.  If it was not
    the debugger's BP, skip the IP past it.

Arguments:

    hthd -  Thread

    bp   -  breakpoint data

Return Value:


--*/
{
    //
    // Check if this is an embedded breakpoint
    //

    if (bp == EMBEDDED_BP) {

        //
        // It was, so there is no instruction to restore,
        // just increment the EIP
        //

        IncrementIP(hthd);
        return;
    }

    if (bp->hWalk) {

        //
        // This is really a hardware breakpoint.  Let the
        // walk manager fix this.
        //

        ExprBPClearBPForStep(hthd);

    } else {

        //
        // Replace the breakpoint current in memory with the correct
        // instruction
        //

        RestoreBreakPoint( bp );
        bp->hBreakPoint = 0;

    }

    return;
}


VOID
DeleteAllBps(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    PBREAKPOINT pbp, bpn;

    EnterCriticalSection(&csThreadProcList);

    pbp = bpList->next;

    while (pbp) {
        bpn = pbp->next;
        if (bpn) {
            MHFree( pbp );
        }
        pbp = bpn;
    }

    bpList->next = NULL;
    bpList->hprc = NULL;

    LeaveCriticalSection(&csThreadProcList);
}

void
AddBpToList(
    PBREAKPOINT pbp
    )
{
    assert(bpList);
    EnterCriticalSection(&csThreadProcList);
    pbp->next    = bpList->next;
    bpList->next = pbp;
    LeaveCriticalSection(&csThreadProcList);
}

PBREAKPOINT
SetWP(
    HPRCX   hprc,
    HTHDX   hthd,
    BPTP    bptype,
    BPNS    bpnotify,
    ADDR    addr
    )
{
    return (PBREAKPOINT)0;
}
