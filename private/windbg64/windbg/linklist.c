
#include "precomp.h"
#pragma hdrstop


#include    "linklist.h"

//
// Linked List Information
//

typedef struct _LLI {
    PLLE        plleHead;
    PLLE        plleTail;
    DWORD       chlleMac;
    DWORD       cbUserData;     // How big the user data is
    LLF         llf;            // LinkList Flags
    LPFNKILLNODE    lpfnKill;   // Callback for deletion of node
    LPFNFCMPNODE    lpfnCmp;    // Callback for node comparison
    CRITICAL_SECTION cs;
} LLI;

//
// Linked List Entry
//

// The data is an array of int64's.  This is to ensure the data is always aligned on a natural boundary.

typedef struct _LLE {
#ifdef DEBUGVER
    DWORD       dwTest;         // consistency check
#endif
    PLLE        plleNext;
#ifdef DBLLINK
    PLLE        pllePrev;
#endif // DBLLINK
    DWORDLONG   rgdw[1];        // Variable length data
} LLE;

#ifdef DEBUGTRACE
char buf[256];
#endif

//
// These help keep the code clean
//
#define lliAlloc()     ((PLLI)MHAlloc(sizeof(LLI)))
#define FreePlli(plli)  MHFree(plli)
#define FreePlle(plle)  MHFree((HDEP) (plle))
//
//      PRIVATE INTERNAL ROUTINES
//
static VOID LLInsertPlle( PLLI, PLLE, PLLE, PLLE );
static VOID LLDeletePlle( PLLI, PLLE, PLLE );
//
// Some debug stuff
//
#ifdef DEBUGVER
#define WCONSIST        (DWORD)0xfeedbeef
#endif // DEBUGVER

/*** LLPlliInit
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
*   Either returns valid PLLI for newly created list or NULL for failure.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PLLI
WINAPI
LLPlliInit(
    DWORD           cbUserData,
    LLF             llf,
    LPFNKILLNODE    lpfnKill,
    LPFNFCMPNODE    lpfnCmp
    )
{
    PLLI   plli;
#ifdef DEBUGTRACE
    char buf[256];
#endif

    Assert( cbUserData );
    Assert( llf == llfNull || lpfnCmp );

#ifdef DEBUGTRACE
    sprintf(buf,
            "LLPlliInit\tcbUserData: %d\tllf: %p\tKill: %p\tCmp: %p\n",
            cbUserData, llf, lpfnKill, lpfnCmp);
    OutputDebugString(buf);
#endif

    if ( plli = lliAlloc() ) {
        memset( plli, 0, sizeof( LLI ) );
        plli->cbUserData = cbUserData;
        plli->lpfnKill = lpfnKill;
        plli->lpfnCmp = lpfnCmp;
        plli->llf = llf;
        InitializeCriticalSection(&plli->cs);
#ifdef DEBUGTRACE
        sprintf(buf, "LLPlliInit\tlplli: %p\t", lplli);
        OutputDebugString(buf);
#endif
    }

#ifdef DEBUGTRACE
    sprintf(buf, "plli: %p\n", plli);
    OutputDebugString(buf);
#endif

    return plli;
}

/*** LLPlleCreate
*
* Purpose:
*   Allocate and initialize new node for list
*
* Input:
*   plli     :  List to create node for.
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
PLLE
WINAPI
LLPlleCreate(
    PLLI    plli
    )
{
    PLLE    plle;
    DWORD   cbNode;

    Assert( plli );

    // Ensure that the list is OK

#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif

    // Compute size of node with user data area

    cbNode = sizeof( LLE ) - sizeof( DWORD ) + plli->cbUserData;

#ifdef DEBUGVER

    // Debug version, we're going to stuff a "sentinel" on to the end of the
    // user's data for consistency checks.

    cbNode += sizeof( DWORD );
#endif

    // Allocate the node

    if ( plle = (PLLE)MHAlloc( cbNode ) ) {
        ZeroMemory( plle, cbNode );

#ifdef DEBUGTRACE
        sprintf(buf,
                "LLPlleCreate\tplli: %p\tplle: %p\tcbNode: %d\n",
                plli, plle, cbNode);
        OutputDebugString(buf);
#endif

#ifdef DEBUGVER
        // Stuff in the consistency check stuff

        *(DWORD *)((BYTE *)plle + cbNode - sizeof( DWORD )) = WCONSIST;
        plle->dwTest = WCONSIST;
#endif
    }
    return plle;
}


/*** LLFAddPlleToLl
*
* Purpose:
*   Add a new node to the end of a list.
*
* Input:
*   plli    :   List to add node to.
*   plle    :   Node to append to list.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
VOID
WINAPI
LLAddPlleToLl(
    PLLI    plli,
    PLLE    plle
    )
{
    Assert( plli );
    Assert( plle );

    EnterCriticalSection(&plli->cs);

    Assert( plli->llf == llfNull );

    // Initalize node: Since this is to be the last item in the list,
    // pNext should be null.  Also, the pPrev should point to the
    // currently last item in the list (includes NULL)

    plle->plleNext = NULL;
#ifdef DBLLINK
    plle->pllePrev = plli->plleTail;
#endif

    //  Chalk up one more for the list

    plli->chlleMac++;

#ifdef DEBUGTRACE
    sprintf(buf, "LLAddPlleToLl\tplle: %p\tplli: %p\n", plle, plli);
    OutputDebugString(buf);
    sprintf(buf, "LLAddPlleToLl\tchlleMac: %d\n", plli->chlleMac);
    OutputDebugString(buf);
#endif

    // If the pHead is NULL then initialize the pHead and pTail
    // (you know, like this is the only item in the list)

    if ( plli->plleHead == NULL ) {
        plli->plleHead = plli->plleTail = plle;
    } else {

        // Otherwise, update the tail pointer and the pNext for the old tail

        plli->plleTail->plleNext = plle;
        plli->plleTail = plle;
    }

    LeaveCriticalSection(&plli->cs);

    // Ensure that the list is OK

#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif
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
VOID
WINAPI
LLInsertPlleInLl(
    PLLI    plli,
    PLLE    plleNew,
    DWORD   lParam
    )
{
    DWORD           fNeedPos = TRUE;
    LPFNFCMPNODE    lpfnCmp = plli->lpfnCmp;
    LPVOID          lpv = LLLpvFromPlle( plleNew );
    PLLE            plle = NULL;
    PLLE            pllePrev = NULL;
    DWORD           dwPos = 0;

    Assert( plli );
    Assert( plli->llf == llfNull || lpfnCmp );

    EnterCriticalSection(&plli->cs);

    switch( plli->llf ) {
        case llfNull:
            while( dwPos++ < LOWORD( lParam ) ) {
                pllePrev = plle;
                plle = LLPlleFindNext( plli, plle );
            }
            break;

        case llfAscending:
            while( fNeedPos && ( plle = LLPlleFindNext( plli, plle ) ) ) {
                fNeedPos = lpfnCmp( LLLpvFromPlle( plle ), lpv, lParam ) == fCmpLT;
                if ( fNeedPos ) {
                    pllePrev = plle;
                }
            }
            break;

        case llfDescending:
            while( fNeedPos && ( plle = LLPlleFindNext( plli, plle ) ) ) {
                fNeedPos = lpfnCmp( LLLpvFromPlle( plle ), lpv, lParam ) == fCmpGT;
                if ( fNeedPos ) {
                    pllePrev = plle;
                }
            }
            break;
    }

    LLInsertPlle( plli, pllePrev, plleNew, plle );
    LeaveCriticalSection(&plli->cs);
}

/*** LLFDeletePlleIndexed
*
* Purpose:
*   Delete the ith node from a list.
*
* Input:
*   plli    :   List containing node to delete
*   lPos    :    zero based index of node to delete.
*
* Output:
*   Return TRUE for successful deletion, else FALSE.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
BOOL
WINAPI
LLFDeletePlleIndexed(
    PLLI    plli,
    DWORD   lPos
    )
{
    PLLE    plleKill;
    PLLE    pllePrev = NULL;
    DWORD   lPosCur = 0L;
    DWORD   fRet = FALSE;

    Assert( plli );

    EnterCriticalSection(&plli->cs);

    plleKill = plli->plleHead;

    //  Make sure that we're not deleting past the end of the list!

    if ( lPos < plli->chlleMac ) {

        //  Chug through the list until we find the sucker to kill!

        while ( lPos != lPosCur ) {
            pllePrev = plleKill;
            plleKill = LLPlleFindNext( plli, plleKill );
            ++lPosCur;
        }
        LLDeletePlle( plli, pllePrev, plleKill );
        fRet = TRUE;
    }

    LeaveCriticalSection(&plli->cs);

#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif

    return fRet;
}

/*** LLFDeleteLpvFromLl
*
* Purpose:
*   Delete a node from a list containing lpv data.
*
* Input:
*   plli    :   List containing node to delete
*   plle    :   Node to begin search for delete node at.
*   lpv     :   far pointer to comparison data.  Passed onto compare callback.
*   lParam  :   Application supplied data.  Just passed onto compare callback.
*
* Output:
*   Returns TRUE if node has been deleted, else FALSE
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
BOOL
WINAPI
LLFDeleteLpvFromLl(
    PLLI    plli,
    PLLE    plle,
    LPVOID     lpv,
    DWORD   lParam
    )
{
#ifdef DBLLINK
    EnterCriticalSection(&plli->cs);

    plle = LLPlleFindLpv( plli, plle, lpv, lParam );
    if ( plle ) {
        LLDeletePlle( plli, LLPlleFindPrev( plli, plle ), plle );
    }

#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif

    LeaveCriticalSection(&plli->cs);

    return plle != NULL;
#else
    PLLE            plleNext;
    LPFNFCMPNODE    lpfnCmp = plli->lpfnCmp;

    Assert( lpfnCmp );

    EnterCriticalSection(&plli->cs);

    //  We're goint to delete the first occurance AFTER the one specified!

    plleNext = LLPlleFindNext( plli, plle );

    // Look up the data in the list

     while( plleNext &&
            lpfnCmp( LLLpvFromPlle( plleNext ), lpv, lParam ) != fCmpEQ ) {
        plle = plleNext;
        plleNext = LLPlleFindNext( plli, plleNext );
    }

    // if plleNext is non-null then we've found something to delete!!!

    if ( plleNext ) {
        LLDeletePlle( plli, plle, plleNext );
    }

    LeaveCriticalSection(&plli->cs);

#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif

    return plleNext != NULL;
#endif
}

/*** LLPlleFindNext
*
* Purpose:
*   Get the next node in a list.
*
* Input:
*   plli   :    List to search in.
*   plle   : Place to begin.  If NULL, then return plleHead.
*
* Output:
*   Returns a handle to the next item in the list.  NULL is returned
*   if the end of the list has been reached.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PLLE
WINAPI
LLPlleFindNext(
    PLLI    plli,
    PLLE    plle
    )
{
    PLLE    plleRet;

    EnterCriticalSection(&plli->cs);

    //  if plle is non-null, return the next handle

    if ( plle ) {
        plleRet = plle->plleNext;
    } else {

        // otherwise, we want the beginning of the list, so return the "head"

        plleRet = plli->plleHead;
    }

    LeaveCriticalSection(&plli->cs);

    return plleRet;
}

#ifdef DBLLINK

/*** LLPlleFindPrev
*
* Purpose:
*   Get the previous node in a list.  DOUBLY LINKED LIBRARY ONLY!!!
*
* Input:
*   plli    :   List to search in.
*   plle    :   Place to beging search at.  If NULL, then return the
*               last node in the list.
*
* Output:
*   Return a handle to the previous node in the list, NULL if the
*   beginning of the list has been hit.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PLLE
WINAPI
LLPlleFindPrev(
    PLLI    plli,
    PLLE    plle
    )
{
    PLLE    plleRet;

    EnterCriticalSection(&plli->cs);

    // if plle is non-null, return the next handle

    if ( plle ) {
        plleRet = plle->pllePrev;
    } else {

        //  otherwise, we want the beginning of the list, so return the "head"

        plleRet = plli->plleTail;
    }

    LeaveCriticalSection(&plli->cs);

    return plleRet;
}
#endif // DBLLINK

/***  LLChlleDestroyList
*
* Purpose:
*   Free all memory associated with a specified list.  Completely
*   destroys the list.  Must call PlliInit() to add new items to list.
*
* Input:
*   plli    :   List to destroy
*
* Output:
*   Returns number of nodes destroyed in list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
DWORD
WINAPI
LLChlleDestroyLl(
    PLLI    plli
    )
{
    DWORD   cRet = 0;

    Assert( plli );

#ifdef DEBUGTRACE
    sprintf(buf, "LLChlleDestroyLl\tplli: %p\n", plli);
    OutputDebugString(buf);
#endif

    EnterCriticalSection(&plli->cs);

    while ( LLChlleInLl( plli ) != 0 ) {
        LLFDeletePlleIndexed( plli, 0 );
        ++cRet;
    }
    LeaveCriticalSection(&plli->cs);
    DeleteCriticalSection(&plli->cs);
    FreePlli( plli );
    return cRet;
}

/*** LLPlleFindLpv
*
* Purpose:
*   Locate a node in a list containing specific data.
*
* Input:
*   plli    :   List to search in.
*   plle    :   Starting place to begin search.  If NULL, start at
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
PLLE
WINAPI
LLPlleFindLpv(
    PLLI    plli,
    PLLE    plle,
    LPVOID  lpvFind,
    DWORD   lParam
    )
{
    PLLE    plleRet = NULL;

    Assert( plli );
    Assert( plli->lpfnCmp );

    EnterCriticalSection(&plli->cs);

    plle = LLPlleFindNext( plli, plle );

    while ( plle != NULL && plleRet == NULL ) {
        if ( plli->lpfnCmp( (LPV)plle->rgdw, lpvFind, lParam ) == fCmpEQ ) {
            plleRet = plle;
        }
        plle = LLPlleFindNext( plli, plle );
    }
    LeaveCriticalSection(&plli->cs);
    return plleRet;
}

/*** LLFCheckPlli
*
* Purpose:
*   Consistency check for a list.
*
* Input:
*   plli    : List to check.
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
*   check to see that the last node in the list is actually plleTail for the
*   list.
*
*************************************************************************/
#ifdef DEBUGVER
BOOL
WINAPI
LLFCheckPlli(
    PLLI    plli
    )
{
    PLLE    plle = NULL;
    DWORD   cbOffSet;
    DWORD   fRet;
    PLLE    plleLast;

    Assert(plli);

    EnterCriticalSection(&plli->cs);

    cbOffSet = sizeof( LLE ) + plli->cbUserData - sizeof( DWORD );
    plleLast = LLPlleFindNext( plli, NULL );

    fRet = plli->cbUserData;
    if ( plli->chlleMac ) {
        fRet = fRet && plli->plleHead && plli->plleTail;
    } else {
        fRet = fRet && !plli->plleHead && !plli->plleTail;
    }
    while( fRet && ( plle = LLPlleFindNext( plli, plle ) ) ) {
        fRet = ((plle->dwTest == WCONSIST &&
                  *((DWORD *)( (BYTE *)plle + cbOffSet )) == WCONSIST ));
        plleLast = plle;
    }
    if ( fRet ) {
        fRet = plleLast == plli->plleTail;
    }

    LeaveCriticalSection(&plli->cs);

    Assert( fRet );
    return fRet;
}
#endif // DEBUGVER

/*** LLInsertPlle
*
* Purpose:
*   INTERNAL utility function to insert a node into a specified place
*   in the list.  This will update the pointers and head/tail information
*   for the list.
*
* Input:
*   plli       :    List to insert node into.
*   pllePrev   :    Node which will appear right before the inserted one.
*   plle       :    Our newly created node.
*   plleNext   :   The node which will be immediately after the new one.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*   The caller must hold the critical section for the list while calling.
*
*************************************************************************/
static
VOID
LLInsertPlle(
    PLLI    plli,
    PLLE    pllePrev,
    PLLE    plle,
    PLLE    plleNext
    )
{
#ifdef DEBUGTRACE
    sprintf(buf,
            "LLInsertPlle\t plli: %p\tpllePrev: %p\tplle: %p\tplleNext: %p\n",
            plli, pllePrev, plle, plleNext);
    OutputDebugString(buf);
#endif

    //  If there is a previous node, update its hnext

    if ( pllePrev ) {
        pllePrev->plleNext = plle;
    } else {

        // Otherwise, update the head

        plli->plleHead = plle;
    }

    // Set the hNext for the new node

    plle->plleNext = plleNext;

    // We're adding to the end of the list, update the plleTail

    if ( plleNext == NULL ) {
        plli->plleTail = plle;
    }

#ifdef DBLLINK
    else {
        //  If there is a next, update its hPrev
        plleNext->pllePrev = plle;
    }

    // Set the hPrev for the new node

    plle->pllePrev = pllePrev;
#endif // DBLLINK

    // Increment the number of items on the list

    ++plli->chlleMac;

#ifdef DEBUGTRACE
    sprintf(buf, "LLInsertPlle\t chlleMac: %d\n", plli->chlleMac);
    OutputDebugString(buf);
#endif

}

/*** LLFDeletePlleFromLl
*
* Purpose:
*   Delete a specified plle.  Update head/tail and node pointers.
*
* Input:
*   plli      : List containing node to delete.
*   plle      : The node to destroy.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*   For doubly linked list, we could just use the pllePrev handle,
*   but there's no guarantee that the node is in the list specified.
*   So we do a look up just to make sure.
*
*************************************************************************/
BOOL
WINAPI
LLFDeletePlleFromLl(
    PLLI    plli,
    PLLE    plle
    )
{
    PLLE    pllePrev = NULL;
    PLLE    plleCur = NULL;
    DWORD   fRet;

    Assert( plli );
    Assert( plle );

    EnterCriticalSection(&plli->cs);

    while( ( plleCur = LLPlleFindNext( plli, plleCur ) ) && plleCur != plle ) {
        if (plleCur == plli->plleHead && pllePrev) {

            // for some odd reason we have looped back around to the
            // head of the list, and will loop endlessly.

            Assert( !"cannot find entry in list, HELP i'm lost" );
            break;
        }
        pllePrev = plleCur;
    }
    if ( fRet = ( plle == plleCur ) ) {
#ifdef DEBUGTRACE
        OutputDebugString("LLFDeletePlleFromLl calling LLDeletePlle\n");
#endif
        LLDeletePlle( plli, pllePrev, plle );
    }

    LeaveCriticalSection(&plli->cs);

    return fRet;
}

/*** LLDeletePlle
*
* Purpose:
*   INTERNAL utility routine to delete a node.  Update head/tail and
*   node pointers.
*
* Input:
*   plli      : List containing node to delete.
*   pllePrev  : Node immediately preceding node to be deleted in the list.
*   plle      : The node to destroy.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*   Caller must hold the critical section while calling
*
*************************************************************************/
static
VOID
LLDeletePlle(
    PLLI    plli,
    PLLE    pllePrev,
    PLLE    plle
    )
{
    PLLE    plleNext = LLPlleFindNext( plli, plle );

#ifdef DEBUGTRACE
    sprintf(buf,
            "LLDeletePlle\t plli: %p\tpllePrev: %p\tplle: %p\tplleNext: %p\n",
            plli, pllePrev, plle, plleNext);
    OutputDebugString(buf);
#endif

    // If there is a previous node, update its hnext

    if ( pllePrev ) {
        pllePrev->plleNext = plleNext;
#ifdef DEBUGTRACE
        OutputDebugString("LLDeletePlle\t Removing from middle\n");
#endif
    } else {

        // Otherwise, update the head

        plli->plleHead = plleNext;
#ifdef DEBUGTRACE
        OutputDebugString("LLDeletePlle\t Removing from head\n");
#endif
    }

    // We're adding to the end of the list, update the plleTail

    if ( plleNext == NULL ) {
#ifdef DEBUGTRACE
        OutputDebugString("LLDeletePlle\t Removing from tail\n");
#endif
        plli->plleTail = pllePrev;
    }

#ifdef DBLLINK
    else {
        //  If there is a next, update its hPrev
        plleNext->pllePrev = pllePrev;
    }
#endif // DBLLINK

    //  Let the app free up its own mess for the data from this node

    if ( plli->lpfnKill ) {
        plli->lpfnKill( (LPVOID)plle->rgdw );
    }

#ifdef DEBUGTRACE
    sprintf(buf, "LLDeletePlle\t Kill Function: %p\n"
                 "LLDeletePlle\t Delete plle\n", plli->lpfnKill);
    OutputDebugString(buf);
#endif

    FreePlle( plle );

    // Decrement the number of items on the list

    --plli->chlleMac;

#ifdef DEBUGTRACE
    sprintf(buf, "LLDeletePlle\t chlleMac: %d\n", plli->chlleMac);
    OutputDebugString(buf);
#endif

}

/*** LLChlleInLl
*
* Purpose:
*   Return the number of nodes in a list
*
* Input:
*   plli   :    List to get count for
*
* Output:
*   Number of nodes in the list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
DWORD
WINAPI
LLChlleInLl(
    PLLI    plli
    )
{
    DWORD   lRet;

    Assert( plli );

    EnterCriticalSection(&plli->cs);

    lRet = plli->chlleMac;

    LeaveCriticalSection(&plli->cs);

    return lRet;
}

/*** LLLpvFromPlle
*
* Purpose:
*   Get a far pointer to the user data of a node.  This locks the node
*   down.  It is the application's responsibility to unlock it!
*
* Input:
*   plle    :   Node to get data for.
*
* Output:
*   A far pointer to the data.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LPVOID
WINAPI
LLLpvFromPlle(
    PLLE    plle
    )
{
#ifdef DEBUGVER
    Assert(plle->dwTest == WCONSIST);
#endif
    return (LPVOID)( plle->rgdw );
}

/***    LLUnlockPlle
*
* Purpose:
*   Reverse the effects of LLLpvFromPlle
*
* Input:
*   plle    :   Node to be unlocked
*
* Output:
*   void
*
* Exceptions:
*
* Notes:
*
***********************************************************************/
VOID
WINAPI
LLUnlockPlle(
    PLLE    plle
    )
{
    return;
}

/*** LLPlleGetLast
*
* Purpose:
*   Get the last node in the specified list.
*
* Input:
*   plli    :   List to look up last item in.
*
* Output:
*   handle to the last item, or NULL if empty list.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
PLLE
WINAPI
LLPlleGetLast(
    PLLI    plli
    )
{
    Assert( plli );
    return plli->plleTail;
}


/*** LLPlleAddToHeadOfLI
*
* Purpose:
*   Add a new node to the head of a list.
*
* Input:
*   plli    :   List to add node to.
*   plle    :   Node to append to list.
*
* Output:
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
WINAPI
LLPlleAddToHeadOfLI(
    PLLI    plli,
    PLLE    plle
    )
{
    Assert( plli );
    Assert( plle );

    EnterCriticalSection(&plli->cs);
    Assert( plli->llf == llfNull );

    plle->plleNext = plli->plleHead;

    // If the pHead is NULL then initialize the pTail
    // (you know, like this is the only item in the list)

    if ( plli->plleHead == NULL ) {
        plli->plleTail = plle;
    }
    plli->plleHead = plle;

#ifdef DBLLINK
    plle->pllePrev = NULL;
#endif

    //  Chalk up one more for the list
    plli->chlleMac++;

#ifdef DEBUGTRACE
    sprintf(buf, "LLPlleAddToHead\t plli: %p\tplleHead: %p\tplle: %p\tplleTail: %p\n",
            plli, plle->plleNext, plle, plli->plleTail);
    OutputDebugString(buf);
    sprintf(buf, "LLPlleAddToHead\t chlleMac: %d\n", plli->chlleMac);
    OutputDebugString(buf);
#endif

    LeaveCriticalSection(&plli->cs);

#ifdef DEBUGVER
    // Ensure that the list is OK
    LLFCheckPlli( plli );
#endif

}

/*** LLFRemovePlleFromLl
*
* Purpose:
*   Remove a specified plle.    Update head/tail and node pointers.
*
* Input:
*   plli      : List containing node to remove.
*   plle      : The node to remove.
*
* Output:
*   None.
*
* Exceptions:
*
* Notes:
*   For doubly linked list, we could just use the pllePrev handle,
*   but there's no guarantee that the node is in the list specified.
*   So we do a look up just to make sure.
*
*************************************************************************/
BOOL
WINAPI
LLFRemovePlleFromLl(
    PLLI    plli,
    PLLE    plle
    )
{
    PLLE    pllePrev = NULL;
    PLLE    plleCur = NULL;
    PLLE    plleNext;
    DWORD   fRet;
    Assert( plli );
    Assert( plle );

    EnterCriticalSection(&plli->cs);

    while( ( plleCur = LLPlleFindNext( plli, plleCur ) ) && plleCur != plle ) {
        pllePrev = plleCur;
    }

#ifdef DEBUGTRACE
    sprintf(buf, "LLFRemovePlle\t plli: %p\tplle: %p\tplleCur: %p",
           plli, plle, plleCur);
    OutputDebugString(buf);
#endif

    if ( fRet = ( plle == plleCur ) ) {

        plleNext = LLPlleFindNext( plli, plle );

#ifdef DEBUGTRACE
        sprintf(buf, "\tplleNext: %p\n", plleNext);
        OutputDebugString(buf);
        if (plleNext == plleCur) {
            OutputDebugString("Next == Current!!!!!\n");
        }
#endif

        //  If there is a previous node, update its hnext
        if ( pllePrev ) {
            pllePrev->plleNext = plleNext;
        } else {
            // Otherwise, update the head
            plli->plleHead = plleNext;
        }

        // We're adding to the end of the list, update the plleTail
        if ( plleNext == NULL ) {
            plli->plleTail = pllePrev;
        }

#ifdef DBLLINK
        else {
            //  If there is a next, update its hPrev
            plleNext->pllePrev = pllePrev;
        }
#endif // DBLLINK

        // Decrement the number of items on the list
        --plli->chlleMac;
#ifdef DEBUGTRACE
        sprintf(buf, "LLFRemovePlle\t chlleMac: %d\n", plli->chlleMac);
        OutputDebugString(buf);
#endif
    }

    LeaveCriticalSection(&plli->cs);
#ifdef DEBUGVER
    LLFCheckPlli( plli );
#endif

    return fRet;
}
