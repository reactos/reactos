
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "d.h"
#include "cvtypes.h"
#include "bm.h"
#include "linklist.hmd"
#include "linklist.h"
#include "cvwin32.h"

/*** LLHlliInit
*
* Purpose:
*   Create a new list with specified options
*
* Input:
*   cbUserData :    Number of bytes for user data per node.  Must be > 0.
*   llf        :    List type flags (llfNull, llfAscending, or llfDescending)
*                   (indicates wether or not list is sorted)
*   lpfnKill   :    callback to application for node deletion notification
*                   (may be NULL)
*   lpfnCmp    :    Node comparison callback.  May be NULL for non-sorted
*                   lists.  Otherwise, required.
*
* Output:
*   Either returns valid HLLI for newly created list or NULL for failure.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
HLLI PASCAL
LLHlliInit(
DWORD           cbUserData,
LLF             llf,
LPFNKILLNODE    lpfnKill,
LPFNFCMPNODE    lpfnCmp ) {
    HLLI    hlli;
    LPLLI   lplli;

    assert( cbUserData );
    assert( llf == llfNull || lpfnCmp );

    if ( hlli = HlliAlloc() ) {
        lplli = LockHlli( hlli );
        _fmemset( lplli, 0, sizeof( LLI ) );
        lplli->pcs = PcsAllocInit();
        lplli->cbUserData = cbUserData;
        lplli->lpfnKill = lpfnKill;
        lplli->lpfnCmp = lpfnCmp;
        lplli->llf = llf;
        UnlockHlli( hlli );
    }
    return hlli;
}

/*** LLHlleCreate
*
* Purpose:
*   Allocate and initialize new node for list
*
* Input:
*   hlli     :  List to create node for.
*
* Output:
*   Returns newly created node (zero filled) if successful, otherwise
*   NULL.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
HLLE PASCAL
LLHlleCreate(
HLLI    hlli ) {
    LPLLE   lplle;
    LPLLI   lplli;
    HLLE    hlle;
    WORD    cbNode;

    assert( hlli );
//
// Ensure that the list is OK
//
#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif
//
// Compute size of node with user data area
//
    lplli = LockHlli( hlli );
    cbNode = (WORD)(sizeof( LLE ) + lplli->cbUserData);
    UnlockHlli( hlli );

#ifdef DEBUGVER
//
// Debug version, we're going to stuff a "sentinel" on to the end of the
// user's data for consistency checks.
//
    cbNode += sizeof( WORD );
#endif // DEBUGVER
//
// Allocate the node
//
    if ( hlle = HlleAllocCb( cbNode ) ) {
        lplle = LockHlle( hlle );
        _fmemset( (LPV)lplle, 0, cbNode );
#ifdef DEBUGVER
//
// Stuff in the consistency check stuff
//
        *(WORD FAR *)((BYTE FAR *)lplle + cbNode - sizeof( WORD )) = WCONSIST;
        lplle->wTest = WCONSIST;
#endif // DEBUGVER
        UnlockHlle( hlle );
    }

    return hlle;
}


/*** LLFAddHlleToLl
*
* Purpose:
*   Add a new node to the end of a list.
*
* Input:
*   hlli    :   List to add node to.
*   hlle    :   Node to append to list.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void PASCAL
LLAddHlleToLl(
HLLI    hlli,
HLLE    hlle ) {
    LPLLE   lplle;
    LPLLI   lplli;

    assert( hlli );
    assert( hlle );

    lplle = LockHlle( hlle );
    lplli = LockHlli( hlli );

    AcquireLockPcs(lplli->pcs);

    assert( lplli->llf == llfNull );
//
//  Initalize node: Since this is to be the last item in the list,
//  pNext should be null.  Also, the pPrev should point to the
//  currently last item in the list (includes NULL)
//
    lplle->hlleNext = hlleNull;
#ifdef DBLLINK
    lplle->hllePrev = lplli->hlleTail;
#endif // DBLLINK
//
//  Chalk up one more for the list
//
    lplli->chlleMac++;
//
//  If the pHead is NULL then initialize the pHead and pTail
//  (you know, like this is the only item in the list)
//
    if ( lplli->hlleHead == hlleNull ) {
        lplli->hlleHead = lplli->hlleTail = hlle;
    }
//
//  Otherwise, update the tail pointer and the pNext for the old tail
//
    else {
        HLLE    hlleT = lplli->hlleTail;

        LockHlle( hlleT )->hlleNext = hlle;
        UnlockHlle( hlleT );
        lplli->hlleTail = hlle;
    }
    UnlockHlle( hlle );

//
// Ensure that the list is OK
//
#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif


    ReleaseLockPcs(lplli->pcs);
    UnlockHlli( hlli );
}

/***
*
* Purpose:
*
* Input:
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void PASCAL
LLInsertHlleInLl(
HLLI    hlli,
HLLE    hlleNew,
DWORD   lParam ) {

    USHORT          fNeedPos = fTrue;
    LPLLI           lplli = LockHlli( hlli );
    HLLE            hlle = hlleNull;
    HLLE            hllePrev = hlleNull;
    WORD            wPos = 0;
    LPFNFCMPNODE    lpfnCmp;
    LPV             lpv;

    assert( hlli );
    AcquireLockPcs( lplli->pcs );

    lpfnCmp = lplli->lpfnCmp;
    assert( lplli->llf == llfNull || lpfnCmp );

    lpv = LLLpvFromHlle( hlleNew );

    switch( lplli->llf ) {
        case llfNull:
            hlle = LLHlleFindNext( hlli, hlle );
            while( wPos++ < LOWORD( lParam ) ) {
                hllePrev = hlle;
                hlle = LLHlleFindNext( hlli, hlle );
            }
            break;

        case llfAscending:
            while( fNeedPos && ( hlle = LLHlleFindNext( hlli, hlle ) ) ) {
                fNeedPos = lpfnCmp( LLLpvFromHlle( hlle ), lpv, lParam ) == fCmpLT;
                UnlockHlle( hlle );
                if ( fNeedPos ) {
                    hllePrev = hlle;
                }
            }
            break;

        case llfDescending:
            while( fNeedPos && ( hlle = LLHlleFindNext( hlli, hlle ) ) ) {
                fNeedPos = lpfnCmp( LLLpvFromHlle( hlle ), lpv, lParam ) == fCmpGT;
                UnlockHlle( hlle );
                if ( fNeedPos ) {
                    hllePrev = hlle;
                }
            }
            break;
    }
    LLInsertHlle( hlli, hllePrev, hlleNew, hlle );
    UnlockHlle( hlleNew );

    ReleaseLockPcs( lplli->pcs );

    UnlockHlli( hlli );
}

/*** LLFDeleteHlleIndexed
*
* Purpose:
*   Delete the ith node from a list.
*
* Input:
*   hlli    :   List containing node to delete
*   lPos    :   zero based index of node to delete.
*
* Output:
*   Return fTrue for successful deletion, else fFalse.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
BOOL PASCAL
LLFDeleteHlleIndexed(
HLLI    hlli,
DWORD   lPos ) {

    USHORT  fRet = fFalse;
    LPLLI   lplli = LockHlli( hlli );
    HLLE    hlleKill = lplli->hlleHead;
    HLLE    hllePrev = hlleNull;
    DWORD   lPosCur = 0L;

    assert( hlli );

    AcquireLockPcs( lplli->pcs );

//
//  Make sure that we're not deleting past the end of the list!
//
    if ( lPos < lplli->chlleMac ) {
//
//  Chug through the list until we find the sucker to kill!
//
        while ( lPos != lPosCur ) {
            hllePrev = hlleKill;
            hlleKill = LLHlleFindNext( hlli, hlleKill );
            ++lPosCur;
        }
        LLDeleteHlle( hlli, hllePrev, hlleKill );
        fRet = fTrue;
    }

#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif

    ReleaseLockPcs( lplli->pcs );

    UnlockHlli( hlli );
    return fRet;
}

/*** LLFDeleteLpvFromLl
*
* Purpose:
*   Delete a node from a list containing lpv data.
*
* Input:
*   hlli    :   List containing node to delete
*   hlle    :   Node to begin search for delete node at.
*   lpv     :   far pointer to comparison data.  Passed onto compare callback.
*   lParam  :   Application supplied data.  Just passed onto compare callback.
*
* Output:
*   Returns fTrue if node has been deleted, else fFalse
*
* Exceptions:
*
* Notes:
*   There must be a compare routine assiciated with this list!!!
*   For a doubly linked list, this is simple.  For singly linked
*   list, we have to go through the list to get the previous node
*   to ensure that the pointers are correct.
*
*************************************************************************/
BOOL PASCAL
LLFDeleteLpvFromLl(
HLLI    hlli,
HLLE    hlle,
LPV     lpv,
DWORD   lParam ) {

#ifdef DBLLINK
    LPLLI   lplli = LockHlli ( hlli );

    AcquireLockPcs ( lplli->pcs );

    hlle = LLHlleFindLpv( hlli, hlle, lpv, lParam );
    if ( hlle ) {
        LLDeleteHlle( hlli, LLHlleFindPrev( hlli, hlle ), hlle );
    }

#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif
    ReleaseLockPcs ( lplli->pcs );
    UnlockHlli ( hlli );
    return hlle != hlleNull;

#else // DBLLINK

    LPLLI           lplli = LockHlli( hlli );
    HLLE            hlleNext;
    LPFNFCMPNODE    lpfnCmp;

    AcquireLockPcs ( lplli->pcs );

    lpfnCmp = lplli->lpfnCmp;

    assert( lpfnCmp );
//
//  We're goint to delete the first occurance AFTER the one specified!
//
    hlleNext = LLHlleFindNext( hlli, hlle );
//
// Look up the data in the list
//
    while( hlleNext &&
        lpfnCmp( LLLpvFromHlle( hlleNext ), lpv, lParam ) != fCmpEQ ) {
        UnlockHlle( hlleNext );
        hlle = hlleNext;
        hlleNext = LLHlleFindNext( hlli, hlleNext );
    }
//
// if hlleNext is non-null then we've found something to delete!!!
//
    if ( hlleNext ) {
        LLDeleteHlle( hlli, hlle, hlleNext );
    }
#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif

    ReleaseLockPcs ( lplli->pcs );

    UnlockHlli( hlli );
    return hlleNext != hlleNull;

#endif // DBLLINK
}

/*** LLHlleFindNext
*
* Purpose:
*   Get the next node in a list.
*
* Input:
*   hlli   :    List to search in.
*   hlle   : Place to begin.  If NULL, then return hlleHead.
*
* Output:
*   Returns a handle to the next item in the list.  hlleNull is returned
*   if the end of the list has been reached.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
HLLE PASCAL
LLHlleFindNext(
HLLI    hlli,
HLLE    hlle ) {
    HLLE        hlleRet;

//
//  if hlle is non-null, return the next handle
//
    if ( hlle ) {
        hlleRet = LockHlle( hlle )->hlleNext;
        UnlockHlle( hlle );
    }
//
//  otherwise, we want the beginning of the list, so return the "head"
//
    else {
        LPLLI   lplli;

        lplli = LockHlli ( hlli );
        AcquireLockPcs ( lplli->pcs );
        hlleRet = lplli->hlleHead;
        ReleaseLockPcs ( lplli->pcs );
        UnlockHlli( hlli );
        }

    return hlleRet;
}

/*** LLHlleFindPrev
*
* Purpose:
*   Get the previous node in a list.  DOUBLY LINKED LIBRARY ONLY!!!
*
* Input:
*   hlli    :   List to search in.
*   hlle    :   Place to beging search at.  If hlleNull, the return the
*               last node in the list.
*
* Output:
*   Return a handle to the previous node in the list, hlleNull if the
*   beginning of the list has been hit.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
#ifdef DBLLINK
HLLE PASCAL
LLHlleFindPrev(
HLLI    hlli,
HLLE    hlle ) {
    HLLE    hlleRet;

//
//  if hlle is non-null, return the next handle
//
    if ( hlle ) {
        hlleRet = LockHlle( hlle )->hllePrev;
        UnlockHlle( hlle );
    }
//
//  otherwise, we want the beginning of the list, so return the "head"
//
    else {
        LPLLI   lplli;

        lplli = LockHlli ( hlli );
        AcquireLockPcs ( lplli->pcs );
        hlleRet = lplli->hlleTail;
        ReleaseLockPcs ( lplli->pcs );
        UnlockHlli( hlli );
    }

    return hlleRet;
}
#endif // DBLLINK

/***  LLChlleDestroyList
*
* Purpose:
*   Free all memory associated with a specified list.  Completely
*   destroys the list.  Must call HlliInit() to add new items to list.
*
* Input:
*   hlli    :   List to destroy
*
* Output:
*   Returns number of nodes destroyed in list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
DWORD PASCAL
LLChlleDestroyLl(
HLLI    hlli ) {
    DWORD   cRet = 0;
    LPLLI   lplli;

    assert( hlli );

    lplli = LockHlli ( hlli );
    AcquireLockPcs ( lplli->pcs );

    while ( LLChlleInLl( hlli ) != 0 ) {
        LLFDeleteHlleIndexed( hlli, 0 );
        ++cRet;
    }

    ReleaseLockPcs ( lplli->pcs );
    FreePcs ( lplli->pcs );
    FreeHlli( hlli );

    return cRet;
}

/*** LLHlleFindLpv
*
* Purpose:
*   Locate a node in a list containing specific data.
*
* Input:
*   hlli    :   List to search in.
*   hlle    :   Starting place to begin search.  If NULL, start at
*               beginning of list.
*  lpv     :    Data passed on to compare callback for match.
*  lParam  :    Application supplied data.  Just passed on to callback.
*
* Output:
*   Returns handle to node meeting compare criteria or NULL if not found.
*
* Exceptions:
*
* Notes:
*   Requires that list has a callback function.
*
*************************************************************************/
HLLE PASCAL
LLHlleFindLpv(
HLLI    hlli,
HLLE    hlle,
LPV lpvFind,
DWORD   lParam ) {

    HLLE    hlleRet = hlleNull;
    LPLLI   lplli = LockHlli( hlli );
    LPLLE   lplle;

    AcquireLockPcs ( lplli->pcs );

    assert( hlli );
    assert( lplli->lpfnCmp );

    hlle = LLHlleFindNext( hlli, hlle );

    while ( hlle != hlleNull && hlleRet == hlleNull ) {
        lplle = LockHlle( hlle );
        if ( lplli->lpfnCmp( (LPV)lplle->rgw, lpvFind, lParam ) == fCmpEQ ) {
            hlleRet = hlle;
        }
        UnlockHlle( hlle );
        hlle = LLHlleFindNext( hlli, hlle );
    }

    ReleaseLockPcs ( lplli->pcs );
    UnlockHlli( hlli );

    return hlleRet;
}

/*** LLFCheckHlli
*
* Purpose:
*   Consistency check for a list.
*
* Input:
&   hlli      : List to check.
*
* Output:
*   Returns non-null if list is OK, otherwise failure is indicated.
*
* Exceptions:
*
* Notes:
*   This is a debug only function.  We will zip through the entire list
*   and check the head and tail words of EACH node for our magic WCONSIST
*   value.  If either one is not correct, someone trashed a node.  We also
*   check to see that the last node in the list is actually hlleTail for the
*   list.
*
*************************************************************************/
#ifdef DEBUGVER
BOOL PASCAL
LLFCheckHlli(
HLLI    hlli ) {
    LPLLI   lplli = LockHlli( hlli );
    LPLLE   lplle;
    HLLE    hlle = hlleNull;
    WORD    cbOffSet = (WORD)(sizeof( LLE ) + lplli->cbUserData);
    ULONG	fRet;
    HLLE    hlleLast = LLHlleFindNext( hlli, hlleNull );

    fRet = lplli->cbUserData; // && lplli->lpfnCmp && lplli->lpfnKill;
    if ( lplli->chlleMac ) {
        fRet = fRet && lplli->hlleHead && lplli->hlleTail;
    }
    else {
        fRet = fRet && !lplli->hlleHead && !lplli->hlleTail;
    }
    while( fRet && ( hlle = LLHlleFindNext( hlli, hlle ) ) ) {
        lplle = LockHlle( hlle );
        fRet = ( lplle->wTest == WCONSIST &&
            *((WORD FAR *)( (BYTE FAR *)lplle + cbOffSet )) == WCONSIST );
        hlleLast = hlle;
        UnlockHlle( hlle );
    }
    if ( fRet ) {
        fRet = hlleLast == lplli->hlleTail;
    }
    UnlockHlli( hlli );
    assert( fRet );
    return fRet;
}
#endif // DEBUGVER

/*** LLInsertHlle
*
* Purpose:
*   INTERNAL utility function to insert a node into a specified place
*   in the list.  This will update the pointers and head/tail information
*   for the list.
*
* Input:
*   hlli       :    List to insert node into.
*   hllePrev   :    Node which will appear right before the inserted one.
*   hlle       :    Our newly created node.
*   hlleNext   :   The node which will be immediately after the new one.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void PASCAL
LLInsertHlle(
HLLI    hlli,
HLLE    hllePrev,
HLLE    hlle,
HLLE    hlleNext ) {

    LPLLE   lplle;
    LPLLI   lplli;

//
// Note that we don't need to acquire critical sections here since the
//  higher level routines do that.
//
    lplli = LockHlli( hlli );
    lplle = LockHlle( hlle );
//
//  If there is a previous node, update its hnext
//
    if ( hllePrev ) {
        LockHlle( hllePrev )->hlleNext = hlle;
        UnlockHlle( hllePrev );
    }
//
// Otherwise, update the head
//
    else {
        lplli->hlleHead = hlle;
    }
//
// Set the hNext for the new node
//
    lplle->hlleNext = hlleNext;
//
// We're adding to the end of the list, update the hlleTail
//
    if ( hlleNext == hlleNull ) {
        lplli->hlleTail = hlle;
    }
//
//  If there is a next, update its hPrev
//
#ifdef DBLLINK
    else {
        LockHlle( hlleNext )->hllePrev = hlle;
        UnlockHlle( hlleNext );
    }
//
// Set the hPrev for the new node
//
    lplle->hllePrev = hllePrev;
#endif // DBLLINK
//
// Increment the number of items on the list
//
    ++lplli->chlleMac;
    UnlockHlle( hlle );
    UnlockHlli( hlli );
}

/*** LLFDeleteHlleFromLl
*
* Purpose:
*   Delete a specified hlle.  Update head/tail and node pointers.
*
* Input:
*   hlli      : List containing node to delete.
*   hlle      : The node to destroy.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*   For doubly linked list, we could just use the hllePrev handle,
*  but there's no guarantee that the node is in the list specified.  So
*   We do a look up just to make sure.
*
*************************************************************************/
BOOL PASCAL
LLFDeleteHlleFromLl(
HLLI    hlli,
HLLE    hlle ) {
    HLLE    hllePrev = hlleNull;
    HLLE    hlleCur = hlleNull;
    USHORT  fRet;
    LPLLI   lplli;

    assert( hlli );

    //
    // REVIEW:PERFORMANCE don't need to lock hlli down on non NT versions
    //
    lplli = LockHlli ( hlli );
    AcquireLockPcs ( lplli->pcs );

    assert( hlle );
    while( ( hlleCur = LLHlleFindNext( hlli, hlleCur ) ) && hlleCur != hlle ) {
        hllePrev = hlleCur;
    }
    if ( fRet = ( hlle == hlleCur ) ) {
        LLDeleteHlle( hlli, hllePrev, hlle );
    }

    ReleaseLockPcs ( lplli->pcs );
    UnlockHlli ( hlli );

    return fRet;
}

/*** LLDeleteHlle
*
* Purpose:
*   INTERNAL utility routine to delete a node.  Update head/tail and
*   node pointers.
*
* Input:
*   hlli      : List containing node to delete.
*   hllePrev  : Node immediately preceding node to be deleted in the list.
*   hlle      : The node to destroy.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void PASCAL
LLDeleteHlle(
HLLI    hlli,
HLLE    hllePrev,
HLLE    hlle ) {

    LPLLI   lplli = LockHlli( hlli );
    HLLE    hlleNext = LLHlleFindNext( hlli, hlle );
//
//  If there is a previous node, update its hnext
//
    if ( hllePrev ) {
        LockHlle( hllePrev )->hlleNext = hlleNext;
        UnlockHlle( hllePrev );
    }
//
// Otherwise, update the head
//
    else {
        lplli->hlleHead = hlleNext;
    }
//
// We're adding to the end of the list, update the hlleTail
//
    if ( hlleNext == hlleNull ) {
        lplli->hlleTail = hllePrev;
    }
//
//  If there is a next, update its hPrev
//
#ifdef DBLLINK
    else {
        LockHlle( hlleNext )->hllePrev = hllePrev;
        UnlockHlle( hlleNext );
    }
#endif // DBLLINK
//
//  Let the app free up its own mess for the data from this node
//
    if ( lplli->lpfnKill ) {
        lplli->lpfnKill( (LPV)LockHlle( hlle )->rgw );
        UnlockHlle( hlle );
    }
    FreeHlle( hlle );
//
// Decrement the number of items on the list
//
    --lplli->chlleMac;
    UnlockHlli( hlli );
}

/*** LLChlleInLl
*
* Purpose:
*   Return the number of nodes in a list
*
* Input:
*   hlli   :    List to get count for
*
* Output:
*   Number of nodes in the list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
DWORD PASCAL
LLChlleInLl(
HLLI    hlli ) {

    DWORD   lRet;
    LPLLI   lplli;

    assert( hlli );

    lplli = LockHlli ( hlli );
    AcquireLockPcs ( lplli->pcs );

    lRet = lplli->chlleMac;

    ReleaseLockPcs ( lplli->pcs );
    UnlockHlli( hlli );
    return lRet;
}

/*** LLLpvFromHlle
*
* Purpose:
*   Get a FAR pointer to the user data of a node.  This locks the node
*   down.  It is the application's responsibility to unlock it!
*
* Input:
*   hlle    :   Node to get data for.
*
* Output:
*   A FAR pointer to the data.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPV PASCAL
LLLpvFromHlle(
HLLE    hlle ) {
    if (hlle == hlleNull) {
        return NULL;
    }
    else {
        return (LPV)( LockHlle( hlle )->rgw );
    }
}

/*** LLHlleGetLast
*
* Purpose:
*   Get the last node in the specified list.
*
* Input:
*   hlli    :   List to look up last item in.
*
* Output:
*   handle to the last item, or NULL if empty list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
HLLE PASCAL
LLHlleGetLast(
HLLI    hlli ) {
    HLLE    hlleRet;
    LPLLI   lplli;

    assert( hlli );

    lplli = LockHlli ( hlli );

    AcquireLockPcs ( lplli->pcs );

    hlleRet = lplli->hlleTail;

    ReleaseLockPcs ( lplli->pcs );
    UnlockHlli( hlli );
    return hlleRet;
}


/*** LLHlleAddToHeadOfLI
*
* Purpose:
*   Add a new node to the head of a list.
*
* Input:
*   hlli    :   List to add node to.
*   hlle    :   Node to append to list.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void PASCAL LLHlleAddToHeadOfLI( HLLI   hlli, HLLE  hlle ) {

    LPLLE   lplle;
    LPLLI   lplli;

    assert( hlli );
    assert( hlle );

    lplli = LockHlli( hlli );
    AcquireLockPcs ( lplli->pcs );

    lplle = LockHlle( hlle );
    assert( lplli->llf == llfNull );

    lplle->hlleNext = lplli->hlleHead;

    //  If the pHead is NULL then initialize the pTail
    //  (you know, like this is the only item in the list)
    if ( lplli->hlleHead == hlleNull ) {
        lplli->hlleTail = hlle;
    }
    lplli->hlleHead = hlle;
#ifdef DBLLINK
    lplle->hllePrev = hlleNull;
#endif // DBLLINK

    //  Chalk up one more for the list
    lplli->chlleMac++;

    // Ensure that the list is OK
    UnlockHlle( hlle );

#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif

    ReleaseLockPcs ( lplli->pcs );

    UnlockHlli( hlli );

}

/*** LLFRemoveHlleFromLl
*
* Purpose:
*   Remove a specified hlle.    Update head/tail and node pointers.
*
* Input:
*   hlli      : List containing node to remove.
*   hlle      : The node to remove.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*   For doubly linked list, we could just use the hllePrev handle,
*  but there's no guarantee that the node is in the list specified.  So
*   We do a look up just to make sure.
*
*************************************************************************/
BOOL PASCAL LLFRemoveHlleFromLl( HLLI hlli, HLLE hlle ) {

    HLLE    hllePrev = hlleNull;
    HLLE    hlleCur = hlleNull;
    USHORT  fRet;
    LPLLI   lplli;

    assert( hlli );
    assert( hlle );

    lplli = LockHlli ( hlli );

    AcquireLockPcs ( lplli->pcs );

    while( ( hlleCur = LLHlleFindNext( hlli, hlleCur ) ) && hlleCur != hlle ) {
        hllePrev = hlleCur;
    }
    if ( fRet = ( hlle == hlleCur ) ) {

        HLLE    hlleNext = LLHlleFindNext( hlli, hlle );

        //  If there is a previous node, update its hnext
        if ( hllePrev ) {
            LockHlle( hllePrev )->hlleNext = hlleNext;
            UnlockHlle( hllePrev );
        }

        // Otherwise, update the head
        else {
            lplli->hlleHead = hlleNext;
        }

        // We're adding to the end of the list, update the hlleTail
        if ( hlleNext == hlleNull ) {
            lplli->hlleTail = hllePrev;
        }

        //  If there is a next, update its hPrev
#ifdef DBLLINK
        else {
            LockHlle( hlleNext )->hllePrev = hllePrev;
            UnlockHlle( hlleNext );
        }
#endif // DBLLINK

        // Decrement the number of items on the list
        --lplli->chlleMac;
    }

#if DEBUG > 1
    LLFCheckHlli( hlli );
#endif

    ReleaseLockPcs ( lplli->pcs );

    UnlockHlli( hlli );

    return fRet;
}
