#include "ctlspriv.h"

///////////////////////////////////////////////////////////////////////////////
// SUBCLASS.C -- subclassing helper functions
//
//      SetWindowSubclass
//      GetWindowSubclass
//      RemoveWindowSubclass
//      DefSubclassProc
//
//  This module defines helper functions that make subclassing windows safe(er)
// and easy(er).  The code maintains a single property on the subclassed window
// and dispatches various "subclass callbacks" to its clients a required.  The
// client is provided reference data and a simple "default processing" API.
//
// Semantics:
//  A "subclass callback" is identified by a unique pairing of a callback
// function pointer and an unsigned ID value.  Each callback can also store a
// single DWORD of reference data, which is passed to the callback function
// when it is called to filter messages.  No reference counting is performed
// for the callback, it may repeatedly call the SetWindowSubclass API to alter
// the value of its reference data element as desired.
//
// Warning: You cannot use these to subclass a window across threads since
//          the critical sections have been removed. 05-May-97
//
// History:
//  26-April-96  francish        Created.
//  05-May  -97  davidds         Stopped serializing the world.
///////////////////////////////////////////////////////////////////////////////
//
// NOTE: Although a linked list would have made the code slightly simpler, this
// module uses a packed callback array to avoid unneccessary fragmentation.  fh
//
struct _SUBCLASS_HEADER;

typedef struct
{
    SUBCLASSPROC    pfnSubclass;        // subclass procedure
    WPARAM          uIdSubclass;        // unique subclass identifier
    DWORD_PTR        dwRefData;          // optional ref data

} SUBCLASS_CALL;

typedef struct _SUBCLASS_FRAME
{
    UINT uCallIndex;                    // index of next callback to call
    UINT uDeepestCall;                  // deepest uCallIndex on stack
    struct _SUBCLASS_FRAME *pFramePrev; // previous subclass frame pointer
    struct _SUBCLASS_HEADER *pHeader;   // header associated with this frame

} SUBCLASS_FRAME;

typedef struct _SUBCLASS_HEADER
{
    UINT uRefs;                         // subclass count
    UINT uAlloc;                        // allocated subclass call nodes
    UINT uCleanup;                      // index of call node to clean up
    DWORD dwThreadId;                   // thread id of window we are hooking
    SUBCLASS_FRAME *pFrameCur;          // current subclass frame pointer
    SUBCLASS_CALL CallArray[1];         // base of packed call node array

} SUBCLASS_HEADER;

#define CALLBACK_ALLOC_GRAIN (3)        // 1 defproc, 1 subclass, 1 spare


#ifdef DEBUG
BOOL IsValidPSUBCLASS_CALL(SUBCLASS_CALL * pcall)
{
    return (IS_VALID_WRITE_PTR(pcall, SUBCLASS_CALL) &&
            (NULL == pcall->pfnSubclass || IS_VALID_CODE_PTR(pcall->pfnSubclass, SUBCLASSPROC)));
}   


BOOL IsValidPSUBCLASS_FRAME(SUBCLASS_FRAME * pframe)
{
    return (IS_VALID_WRITE_PTR(pframe, SUBCLASS_FRAME) && 
            IS_VALID_WRITE_PTR(pframe->pHeader, SUBCLASS_HEADER) &&
            (NULL == pframe->pFramePrev || IS_VALID_WRITE_PTR(pframe->pFramePrev, SUBCLASS_FRAME)));
}    
 

BOOL IsValidPSUBCLASS_HEADER(SUBCLASS_HEADER * phdr)
{
    BOOL bRet = (IS_VALID_WRITE_PTR(phdr, SUBCLASS_HEADER) &&
                 (NULL == phdr->pFrameCur || IS_VALID_STRUCT_PTR(phdr->pFrameCur, SUBCLASS_FRAME)) &&
                 IS_VALID_WRITE_BUFFER(phdr->CallArray, SUBCLASS_CALL, phdr->uAlloc));

    if (bRet)
    {
        UINT i;
        SUBCLASS_CALL * pcall = phdr->CallArray;

        for (i = 0; i < phdr->uRefs; i++, pcall++)
        {
            if (!IS_VALID_STRUCT_PTR(pcall, SUBCLASS_CALL))
                return FALSE;
        }
    }
    return bRet;
}    

#endif

///////////////////////////////////////////////////////////////////////////////
// DEBUG CODE TO CHECK IF WINDOW IS ON SAME THREAD AS CALLER
// Since we don't do any serialization, we need this to make sure of this.
///////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
BOOL IsWindowOnCurrentThread(HWND hWnd)
{
    DWORD foo;

    if (!IsWindow(hWnd))
        // bail if the window is dead so we dont bogusly rip
        return(TRUE);
    
    if (GetCurrentThreadId() != GetWindowThreadProcessId(hWnd, &foo))
    {
        DebugMsg(TF_ALWAYS, TEXT("wn: WindowSubclass - Called from wrong thread %08X"), hWnd);
        return(FALSE);
    }
    else
        return(TRUE);
              
}
#endif

///////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK MasterSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam);
LRESULT CallNextSubclassProc(SUBCLASS_HEADER *pHeader, HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// RETAIL_ZOMBIE_MESSAGE_WNDPROC
//
// this macro controls the generation of diagnostic code for an error condition
// in the subclass code (see the SubclassDeath function below).
//
// commenting out this macro will zombie windows using DefWindowProc instead.
//
//-----------------------------------------------------------------------------
//#define RETAIL_ZOMBIE_MESSAGE_WNDPROC

#if defined(RETAIL_ZOMBIE_MESSAGE_WNDPROC) || defined(DEBUG)
#ifndef DEBUG
#pragma message("\r\nWARNING: disable retail ZombieWndProc before final release\r\n")
#endif
LRESULT ZombieWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
#define ZombieWndProc DefWindowProc
#endif

//-----------------------------------------------------------------------------
// SubclassDeath
//
// this function is called if we ever enter one of our subclassing procedures
// without our reference data (and hence without the previous wndproc).
//
// hitting this represents a catastrophic failure in the subclass code.
//
// the function resets the wndproc of the window to a 'zombie' window
// procedure to avoid faulting.  the RETAIL_ZOMBIE_MESSAGE_WNDPROC macro above
// controls the generation of diagnostic code for this wndproc.
//
//-----------------------------------------------------------------------------
LRESULT SubclassDeath(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // WE SHOULD NEVER EVER GET HERE
    // if we do please find francish to debug it immediately
    //
    DebugMsg(TF_ALWAYS, TEXT("fatal: SubclassDeath in window %08X"), hWnd);

#ifdef DEBUG    
    //
    // if we are in a debugger, stop now regardless of break flags
    //
    __try { DebugBreak(); } __except(EXCEPTION_EXECUTE_HANDLER) {;} __endexcept
#endif
    
    //
    // we call the outside world so prepare to deadlock if we have the critsec
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTNONCRITICAL
#endif

    //
    // in theory we could save the original wndproc in a separate property
    // but that just wastes memory for something that should never happen
    //
    // convert this window to a zombie in hopes that it will get debugged
    //
    InvalidateRect(hWnd, NULL, TRUE);
    SubclassWindow(hWnd, ZombieWndProc);
    return ZombieWndProc(hWnd, uMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// GetWindowProc
//
// this inline function returns the current wndproc for the specified window.
//
//-----------------------------------------------------------------------------
__inline WNDPROC GetWindowProc(HWND hWnd)
{
    return (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
}

//-----------------------------------------------------------------------------
// g_aCC32Subclass
//
// This is the global ATOM we use to store our SUBCLASS_HEADER property on
// random windows that come our way.
//
//  HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
//
//  Win95's property code is BROKEN.  If you SetProp using a text string, USER
// adds and removes atoms for the property symmetrically, including when the
// window is destroyed with properties lying around (good).  Unfortunately, if
// you SetProp using a global atom, USER doesn't do things quite right in the
// window cleanup case.  It uses the atom without adding references in SetProp
// calls and without deleting them in RemoveProp calls (good so far).  However,
// when a window with one of these properties lying around is cleaned up, USER
// will delete the atom on you.  This tends to break apps that do the
// following:
//
//  - MyAtom = GlobalAddAtom("foo");            // at app startup
//  - SetProp(SomeWindow, MyAtom, MyData);
//  - <window gets destroyed, USER deletes atom>
//  - <time passes>
//  - SetProp(SomeOtherWindow, MyAtom, MyData); // fails or uses random atom
//  - GlobalDeleteAtom(MyAtom);                 // fails or deletes random atom
//
//  One might be tempted to ask why this file uses atom properties if they are
// so broken.  Put simply, it is the only way to defend yourself against other
// apps that use atom properties (like the one described above).  Imagine that
// we call SetProp(OurWindow, "bar", OurData) in some other app at about the
// <time passes> point in the sequence above.  USER has just nuked some poor
// app's atom, and we wander into SetProp, which calls GlobalAddAtom, which
// just happens to give us the free slot created by USER's window cleanup code.
// Now we have a real problem because the very same atom is sitting in some
// global variable in the other app, just waiting to be deleted when that app
// exits (Peachtree Accounting tends to be very good at this...)  Of course the
// ultimate outcome of this is that we will call GetProp in some critical
// routine and our data will have vanished (it's actually still in the window's
// property table but GetProp("bar") calls GlobalFindAtom("bar") to get the
// atom to scan the property table for; and that call will fail so the property
// will be missed and we'll get back NULL).
//
//  Basically, we create an atom and aggressively increment its reference count
// so that it can withstand a few GlobalDeleteAtom calls every now and then.
// Since we are using an atom property, we need to worry about USER's cleanup
// code nuking us too.  Thus we just keep incrementing the reference count
// until it pegs.
//
// IEUNIX 
// We doesn't have the above problems, but MainWin SetProp implementation
// doesn't create GlobalAtom when it gets 2nd argument as a string.
// And it doesn't have to - that's non-documented NT/Win95 implementation.
// So, if UNIX, we will use the ATOM in all the cases, marking #ifdef MAINWIN.
//
//  HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
//
//-----------------------------------------------------------------------------
extern ATOM g_aCC32Subclass;

//-----------------------------------------------------------------------------
// FastGetSubclassHeader
//
// this inline function returns the subclass header for the specified window.
// if the window has no subclass header the return value is NULL.
//
//-----------------------------------------------------------------------------
__inline SUBCLASS_HEADER *FastGetSubclassHeader(HWND hWnd)
{
    return  (g_aCC32Subclass ?
            ((SUBCLASS_HEADER *)GetProp(hWnd, MAKEINTATOM(g_aCC32Subclass))) :
            NULL);
}

//-----------------------------------------------------------------------------
// GetSubclassHeader
//
// this function returns the subclass header for the specified window.  it
// fails if the caller is on the wrong process, but will allow the caller to
// get the header from a thread other than the specified window's thread.
//
//-----------------------------------------------------------------------------
SUBCLASS_HEADER *GetSubclassHeader(HWND hWnd)
{
    DWORD dwProcessId;

    //
    // only return the header if we are in the right process
    //
    if (!GetWindowThreadProcessId(hWnd, &dwProcessId))
        dwProcessId = 0;

    if (dwProcessId != GetCurrentProcessId())
    {
        if (dwProcessId)
            DebugMsg(TF_ALWAYS, TEXT("error: XxxWindowSubclass - wrong process for window %08X"), hWnd);

        ASSERT(FALSE);
        return NULL;
    }

    //
    // return the header
    //
    return FastGetSubclassHeader(hWnd);
}

//-----------------------------------------------------------------------------
// SetSubclassHeader
//
// this function sets the subclass header for the specified window.
//
//-----------------------------------------------------------------------------
BOOL SetSubclassHeader(HWND hWnd, SUBCLASS_HEADER *pHeader,
    SUBCLASS_FRAME *pFrameFixup)
{
    ATOM a;
    BOOL fResult = TRUE;    // assume success

    ASSERT(NULL == pHeader || IS_VALID_STRUCT_PTR(pHeader, SUBCLASS_HEADER));
    ASSERT(NULL == pFrameFixup || IS_VALID_STRUCT_PTR(pFrameFixup, SUBCLASS_FRAME));

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;         // we are partying on the header and frame list
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));
#endif


    if (g_aCC32Subclass == 0) {
        //
        // HACK: we are intentionally incrementing the refcount on this atom
        // WE DO NOT WANT IT TO GO BACK DOWN so we will not delete it in process
        // detach (see comments for g_aCC32Subclass in subclass.c for more info)
        //
        if ((a = GlobalAddAtom(c_szCC32Subclass)) != 0)
            g_aCC32Subclass = a;    // in case the old atom got nuked
    }


    //
    // update the frame list if required
    //
    while (pFrameFixup)
    {
        pFrameFixup->pHeader = pHeader;
        pFrameFixup = pFrameFixup->pFramePrev;
    }

    //
    // do we have a window to update?
    //
    if (hWnd)
    {
        //
        // update/remove the property as required
        //
        if (!pHeader)
        {
            //
            // HACK: we remove with an ATOM so the refcount won't drop
            //          (see comments for g_aCC32Subclass above)
            //
            RemoveProp(hWnd, MAKEINTATOM(g_aCC32Subclass));
        }
        else
        {
            LPCTSTR lpPropAtomOrStr;
#ifndef MAINWIN
            //
            // HACK: we add using a STRING so the refcount will go up
            //          (see comments for g_aCC32Subclass above)
            //
            lpPropAtomOrStr = c_szCC32Subclass;
#else
            if (! g_aCC32Subclass) 
                g_aCC32Subclass = GlobalAddAtom(c_szCC32Subclass);
        
            if (! g_aCC32Subclass) {
                DebugMsg(TF_ALWAYS, TEXT("wn: SetWindowSubclass - couldn't subclass window %08X\
                         GlobalAddAtom failed for %s"), hWnd, c_szCC32Subclass);
                return FALSE;
            }
            lpPropAtomOrStr = g_aCC32Subclass;
#endif
            if (!SetProp(hWnd, lpPropAtomOrStr, (HANDLE)pHeader))
            {
                DebugMsg(TF_ALWAYS, TEXT("wn: SetWindowSubclass - couldn't subclass window %08X"), hWnd);
                fResult = FALSE;
            }
        }
    }

    return fResult;
}

//-----------------------------------------------------------------------------
// FreeSubclassHeader
//
// this function frees the subclass header for the specified window.
//
//-----------------------------------------------------------------------------
void FreeSubclassHeader(HWND hWnd, SUBCLASS_HEADER *pHeader)
{
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;                 // we will be removing the subclass header
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // sanity
    //
    if (!pHeader)
    {
        ASSERT(FALSE);
        return;
    }

    //
    // clean up the header
    //
    SetSubclassHeader(hWnd, NULL, pHeader->pFrameCur);
    LocalFree((HANDLE)pHeader);
}

//-----------------------------------------------------------------------------
// ReAllocSubclassHeader
//
// this function allocates/reallocates a subclass header for the specified
// window.
//
//-----------------------------------------------------------------------------
SUBCLASS_HEADER *ReAllocSubclassHeader(HWND hWnd, SUBCLASS_HEADER *pHeader,
    UINT uCallbacks)
{
    UINT uAlloc;

    ASSERT(NULL == pHeader || IS_VALID_STRUCT_PTR(pHeader, SUBCLASS_HEADER));

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;     // we will be replacing the subclass header
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // granularize the allocation
    //
    uAlloc = CALLBACK_ALLOC_GRAIN *
        ((uCallbacks + CALLBACK_ALLOC_GRAIN - 1) / CALLBACK_ALLOC_GRAIN);

    //
    // do we need to change the allocation?
    //
    if (!pHeader || (uAlloc != pHeader->uAlloc))
    {
        //
        // compute bytes required
        //
        uCallbacks = uAlloc * sizeof(SUBCLASS_CALL) + sizeof(SUBCLASS_HEADER);

        //
        // and try to alloc
        //
        pHeader = CCLocalReAlloc(pHeader, uCallbacks);

        //
        // did it work?
        //
        if (pHeader)
        {
            //
            // yup, update info
            //
            pHeader->uAlloc = uAlloc;

            if (!SetSubclassHeader(hWnd, pHeader, pHeader->pFrameCur))
            {
                FreeSubclassHeader(hWnd, pHeader);
                pHeader = NULL;
            }

        }
    }

    ASSERT(pHeader);
    return pHeader;
}

//-----------------------------------------------------------------------------
// CallOriginalWndProc
//
// this procedure is the default SUBCLASSPROC which is always installed when we
// subclass a window.  the original window procedure is installed as the
// reference data for this callback.  it simply calls the original wndproc and
// returns its result.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK CallOriginalWndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    //
    // dwRefData should be the original window procedure
    //
    ASSERT(dwRefData);

    //
    // and call it
    //
    return CallWindowProc((WNDPROC)dwRefData, hWnd, uMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// AttachSubclassHeader
//
// this procedure makes sure that a given window is subclassed by us.  it
// maintains a reference count on the data structures associated with our
// subclass.  if the window is not yet subclassed by us then this procedure
// installs our subclass procedure and associated data structures.
//
//-----------------------------------------------------------------------------
SUBCLASS_HEADER *AttachSubclassHeader(HWND hWnd)
{
    SUBCLASS_HEADER *pHeader;
    DWORD dwThreadId;

    //
    // we party on the subclass call chain here
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // we only call SetWindowLong for the first caller, which would cause this
    // operation to work out of context sometimes and fail others...
    // artifically prevent people from subclassing from the wrong thread
    //  
    if ((dwThreadId = GetWindowThreadProcessId(hWnd, NULL)) !=
        GetCurrentThreadId())
    {
        AssertMsg(FALSE, TEXT("error: SetWindowSubclass - wrong thread for window %08X"), hWnd);
        return NULL;
    }

    //
    // if haven't already subclassed the window then do it now
    //
    if ((pHeader = GetSubclassHeader(hWnd)) == NULL)
    {
        WNDPROC pfnOldWndProc;
        SUBCLASS_CALL *pCall;

        //
        // attach our header data to the window
        // we need space for two callbacks; the subclass and the original proc
        //
        if ((pHeader = ReAllocSubclassHeader(hWnd, NULL, 2)) == NULL)
            return NULL;

        pHeader->dwThreadId = dwThreadId;

        //
        // actually subclass the window
        //
        if ((pfnOldWndProc = SubclassWindow(hWnd, MasterSubclassProc)) == NULL)
        {
            // clean up and get out
            FreeSubclassHeader(hWnd, pHeader);
            return NULL;
        }

        //
        // set up the first node in the array to call the original wndproc
        //
        ASSERT(pHeader->uAlloc);

        pCall = pHeader->CallArray;
        pCall->pfnSubclass = CallOriginalWndProc;
        pCall->uIdSubclass = 0;
        pCall->dwRefData   = (DWORD_PTR)pfnOldWndProc;

        //
        // init our subclass refcount...
        //
        pHeader->uRefs = 1;
    }

    return pHeader;
}

//-----------------------------------------------------------------------------
// DetachSubclassHeader
//
// this procedure attempts to detach the subclass header from the specified
// window
//
//-----------------------------------------------------------------------------
void DetachSubclassHeader(HWND hWnd, SUBCLASS_HEADER *pHeader, BOOL fForce)
{
    WNDPROC pfnOldWndProc;
#ifdef DEBUG
    SUBCLASS_CALL *pCall;
    UINT uCur;
#endif

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;         // we party on the subclass call chain here
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    ASSERT(pHeader);        // fear

    //
    // if we are not being forced to remove and the window is still valid then
    // sniff around a little and decide if it's a good idea to detach now
    //
    if (!fForce && hWnd)
    {
        ASSERT(pHeader == FastGetSubclassHeader(hWnd)); // paranoia

        //
        // do we still have active clients?
        //
        if (pHeader->uRefs > 1)
            return;

        ASSERT(pHeader->uRefs); // should always have the "call original" node

        //
        // are people on our stack?
        //
        if (pHeader->pFrameCur)
            return;

        //
        // if we are out of context then we should try again later
        //
        if (pHeader->dwThreadId != GetCurrentThreadId())
        {
            SendNotifyMessage(hWnd, WM_NULL, 0, 0L);
            return;
        }

        //
        // we keep the original window procedure as refdata for our
        // CallOriginalWndProc subclass callback
        //
        pfnOldWndProc = (WNDPROC)pHeader->CallArray[0].dwRefData;
        ASSERT(pfnOldWndProc);

        //
        // if somebody else is subclassed after us then we can't detach now
        //
        if (GetWindowProc(hWnd) != MasterSubclassProc)
            return;

        //
        // go ahead and try to detach
        //
        if (!SubclassWindow(hWnd, pfnOldWndProc))
        {
            ASSERT(FALSE);      // just plain shouldn't happen
            return;
        }
    }

    //
    // warn about anybody who hasn't unhooked yet
    //
#ifdef DEBUG
    uCur = pHeader->uRefs;
    pCall = pHeader->CallArray + uCur;
    while (--uCur)          // don't complain about our 'call original' node
    {
        pCall--;
        if (pCall->pfnSubclass)
        {
            //
            // always warn about these they could be leaks
            //
            DebugMsg(TF_ALWAYS, TEXT("warning: orphan subclass: fn %08X, id %08X, dw %08X"),
                pCall->pfnSubclass, pCall->uIdSubclass, pCall->dwRefData);
        }
    }
#endif

    //
    // free the header now
    //
    FreeSubclassHeader(hWnd, pHeader);
}

//-----------------------------------------------------------------------------
// PurgeSingleCallNode
//
// this procedure purges a single dead node in the call array
//
//-----------------------------------------------------------------------------
void PurgeSingleCallNode(HWND hWnd, SUBCLASS_HEADER *pHeader)
{
    UINT uRemain;

    ASSERT(IS_VALID_STRUCT_PTR(pHeader, SUBCLASS_HEADER));

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;         // we will try to re-arrange the call array
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    
    if (!pHeader->uCleanup) // a little sanity
    {
        ASSERT(FALSE);      // nothing to do!
        return;
    }

    //
    // and a little paranoia
    //
    ASSERT(!pHeader->pFrameCur ||
        (pHeader->uCleanup < pHeader->pFrameCur->uDeepestCall));

    //
    // are there any call nodes above the one we're about to remove?
    //
    if ((uRemain = (pHeader->uRefs - pHeader->uCleanup)) > 0)
    {
        //
        // yup, need to fix up the array the hard way
        //
        SUBCLASS_CALL *pCall;
        SUBCLASS_FRAME *pFrame;
        UINT uCur, uMax;

        //
        // move the remaining nodes down into the empty space
        //
        pCall = pHeader->CallArray + pHeader->uCleanup;
        MoveMemory(pCall, pCall + 1, uRemain * sizeof(SUBCLASS_CALL));

        ASSERT(IS_VALID_STRUCT_PTR(pCall, SUBCLASS_CALL));

        //
        // update the call indices of any active frames
        //
        uCur = pHeader->uCleanup;
        pFrame = pHeader->pFrameCur;
        while (pFrame)
        {
            if (pFrame->uCallIndex >= uCur)
            {
                pFrame->uCallIndex--;

                if (pFrame->uDeepestCall >= uCur)
                    pFrame->uDeepestCall--;
            }

            pFrame = pFrame->pFramePrev;
        }

        //
        // now search for any other dead call nodes in the reamining area
        //
        uMax = pHeader->uRefs - 1;  // we haven't decremented uRefs yet
        while (uCur < uMax)
        {
            if (!pCall->pfnSubclass)
                break;

            pCall++;
            uCur++;
        }
        pHeader->uCleanup = (uCur < uMax)? uCur : 0;
    }
    else
    {
        //
        // nope, this case is easy
        //
        pHeader->uCleanup = 0;
    }

    //
    // finally, decrement the client count
    //
    pHeader->uRefs--;
}

//-----------------------------------------------------------------------------
// CompactSubclassHeader
//
// this procedure attempts to compact the subclass call array, freeing the
// subclass header if the array is empty
//
//-----------------------------------------------------------------------------
void CompactSubclassHeader(HWND hWnd, SUBCLASS_HEADER *pHeader)
{
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;         // we will try to re-arrange the call array
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    ASSERT(IS_VALID_STRUCT_PTR(pHeader, SUBCLASS_HEADER));

    //
    // we must handle the "window destroyed unexpectedly during callback" case
    //
    if (hWnd)
    {
        //
        // clean out as many dead callbacks as possible
        //
        while (pHeader->uCleanup && (!pHeader->pFrameCur ||
            (pHeader->uCleanup < pHeader->pFrameCur->uDeepestCall)))
        {
            PurgeSingleCallNode(hWnd, pHeader);
        }

        //
        // do we still have clients?
        //
        if (pHeader->uRefs > 1)
        {
            //
            // yes, shrink our allocation, leaving room for at least one client
            //
            ReAllocSubclassHeader(hWnd, pHeader, pHeader->uRefs + 1);
            return;
        }
    }

    //
    // try to detach and free
    //
    DetachSubclassHeader(hWnd, pHeader, FALSE);
}

//-----------------------------------------------------------------------------
// FindCallRecord
//
// this procedure searches for a call record with the specified subclass proc
// and id, and returns its address.  if no such call record is found then NULL
// is returned.
//
//-----------------------------------------------------------------------------
SUBCLASS_CALL *FindCallRecord(SUBCLASS_HEADER *pHeader,
    SUBCLASSPROC pfnSubclass, WPARAM uIdSubclass)
{
    SUBCLASS_CALL *pCall;
    UINT uCallIndex;

    ASSERT(IS_VALID_STRUCT_PTR(pHeader, SUBCLASS_HEADER));

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;         // we'll be scanning the call array
#endif

    //
    // scan the call array.  note that we assume there is always at least
    // one member in the table (our CallOriginalWndProc record)
    //
    pCall = pHeader->CallArray + (uCallIndex = pHeader->uRefs);
    do
    {
        uCallIndex--;
        pCall--;
        if ((pCall->pfnSubclass == pfnSubclass) &&
            (pCall->uIdSubclass == uIdSubclass))
        {
            return pCall;
        }
    }
    while (uCallIndex != (UINT)-1);

    return NULL;
}

//-----------------------------------------------------------------------------
// GetWindowSubclass
//
// this procedure retrieves the reference data for the specified window
// subclass callback
//
//-----------------------------------------------------------------------------
BOOL GetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
    DWORD_PTR *pdwRefData)
{
    SUBCLASS_HEADER *pHeader;
    SUBCLASS_CALL *pCall;
    BOOL fResult = FALSE;
    DWORD_PTR dwRefData = 0;

    //
    // sanity
    //
    if (!IsWindow(hWnd))
    {
        AssertMsg(FALSE, TEXT("error: GetWindowSubclass - %08X not a window"), hWnd);
        goto ReturnResult;
    }

    //
    // more sanity
    //
    if (!pfnSubclass
#ifdef DEBUG
        || IsBadCodePtr((PROC)pfnSubclass)
#endif
        )
    {
        AssertMsg(FALSE, TEXT("error: GetWindowSubclass - invalid callback %08X"), pfnSubclass);
        goto ReturnResult;
    }

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    
    //
    // if we've subclassed it and they are a client then get the refdata
    //
    if (((pHeader = GetSubclassHeader(hWnd)) != NULL) &&
        ((pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass)) != NULL))
    {
        //
        // fetch the refdata and note success
        //
        dwRefData = pCall->dwRefData;
        fResult = TRUE;
    }

#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // we always fill in/zero pdwRefData regradless of result
    //
ReturnResult:
    if (pdwRefData)
        *pdwRefData = dwRefData;

    return fResult;
}

//-----------------------------------------------------------------------------
// SetWindowSubclass
//
// this procedure installs/updates a window subclass callback.  subclass
// callbacks are identified by their callback address and id pair.  if the
// specified callback/id pair is not yet installed then the procedure installs
// the pair.  if the callback/id pair is already installed then this procedure
// changes the refernce data for the pair.
//
//-----------------------------------------------------------------------------
BOOL SetWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass, UINT_PTR uIdSubclass,
    DWORD_PTR dwRefData)
{
    SUBCLASS_HEADER *pHeader;
    SUBCLASS_CALL *pCall;
    BOOL bResult;

    //
    // some sanity
    //
    if (!IsWindow(hWnd))
    {
        AssertMsg(FALSE, TEXT("error: SetWindowSubclass - %08X not a window"), hWnd);
        return FALSE;
    }

    //
    // more sanity
    //
    if (!pfnSubclass
#ifdef DEBUG
        || IsBadCodePtr((PROC)pfnSubclass)
#endif
        )
    {
        AssertMsg(FALSE, TEXT("error: SetWindowSubclass - invalid callback %08X"), pfnSubclass);
        return FALSE;
    }

    bResult = FALSE;    // assume failure


    //
    // we party on the subclass call chain here

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    //
    // actually subclass the window
    //
    if ((pHeader = AttachSubclassHeader(hWnd)) == NULL)
        goto bail;

    //
    // find a call node for this caller
    //
    if ((pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass)) == NULL)
    {
        //
        // not found, alloc a new one
        //
        SUBCLASS_HEADER *pHeaderT =
            ReAllocSubclassHeader(hWnd, pHeader, pHeader->uRefs + 1);

        if (!pHeaderT)
        {
            //
            // re-query in case it is already gone
            //
            if ((pHeader = FastGetSubclassHeader(hWnd)) != NULL)
                CompactSubclassHeader(hWnd, pHeader);

            goto bail;
        }

        pHeader = pHeaderT;
        pCall = pHeader->CallArray + pHeader->uRefs;
        pHeader->uRefs++;
    }

    //
    // fill in the subclass call data
    //
    pCall->pfnSubclass = pfnSubclass;
    pCall->uIdSubclass = uIdSubclass;
    pCall->dwRefData   = dwRefData;

    bResult = TRUE;

bail:
    //
    // release the critical section and return the result
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    return bResult;
}

//-----------------------------------------------------------------------------
// RemoveWindowSubclass
//
// this procedure removes a subclass callback from a window.  subclass
// callbacks are identified by their callback address and id pair.
//
//-----------------------------------------------------------------------------
BOOL RemoveWindowSubclass(HWND hWnd, SUBCLASSPROC pfnSubclass,
    UINT_PTR uIdSubclass)
{
    SUBCLASS_HEADER *pHeader;
    SUBCLASS_CALL *pCall;
    BOOL bResult;
    UINT uCall;

    //
    // some sanity
    //
    if (!IsWindow(hWnd))
    {
        AssertMsg(FALSE, TEXT("error: RemoveWindowSubclass - %08X not a window"), hWnd);
        return FALSE;
    }

    //
    // more sanity
    //
    if (!pfnSubclass
#ifdef DEBUG
        || IsBadCodePtr((PROC)pfnSubclass)
#endif
        )
    {
        AssertMsg(FALSE, TEXT("error: RemoveWindowSubclass - invalid callback %08X"), pfnSubclass);
        return FALSE;
    }

    bResult = FALSE;    // assume failure

    //
    // we party on the subclass call chain here

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // obtain our subclass data
    //
    if ((pHeader = GetSubclassHeader(hWnd)) == NULL)
        goto bail;

    //
    // find the callback to remove
    //
    if ((pCall = FindCallRecord(pHeader, pfnSubclass, uIdSubclass)) == NULL)
        goto bail;

    //
    // disable this node and remember that we have something to clean up
    //
    pCall->pfnSubclass = NULL;

    uCall = (UINT) (pCall - pHeader->CallArray);

    if (!pHeader->uCleanup || (uCall < pHeader->uCleanup))
        pHeader->uCleanup = uCall;

    //
    // now try to clean up any unused nodes
    //
    CompactSubclassHeader(hWnd, pHeader);
#ifdef DEBUG
    // the call above can realloc or free the subclass header for this window
    pHeader = NULL;
#endif

    bResult = TRUE;     // it worked

bail:
    //
    // release the critical section and return the result
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    return bResult;
}

//-----------------------------------------------------------------------------
// DefSubclassProc
//
// this procedure calls the next handler in the window's subclass chain.  the
// last handler in the subclass chain is installed by us, and calls the
// original window procedure for the window.
//
//-----------------------------------------------------------------------------
LRESULT DefSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SUBCLASS_HEADER *pHeader;
    LRESULT lResult = 0L;

    //
    // make sure the window is still valid
    //
    if (!IsWindow(hWnd))
    {
        AssertMsg(FALSE, TEXT("warning: DefSubclassProc - %08X not a window"), hWnd);
        goto BailNonCritical;
    }

    //
    // take the critical section while we figure out who to call next
    //

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
        
    //
    // complain if we are being called improperly
    //
    if ((pHeader = FastGetSubclassHeader(hWnd)) == NULL)
    {
        AssertMsg(FALSE, TEXT("error: DefSubclassProc - window %08X not subclassed"), hWnd);
        goto BailCritical;
    }
    else if (GetCurrentThreadId() != pHeader->dwThreadId)
    {
        AssertMsg(FALSE, TEXT("error: DefSubclassProc - wrong thread for window %08X"), hWnd);
        goto BailCritical;
    }
    else if (!pHeader->pFrameCur)
    {
        AssertMsg(FALSE, TEXT("error: DefSubclassProc - window %08X not in callback"), hWnd);
        goto BailCritical;
    }

    //
    // call the next proc in the subclass chain
    //
    // WARNING: this call temporarily releases the critical section
    // WARNING: pHeader is invalid when this call returns
    //
    lResult = CallNextSubclassProc(pHeader, hWnd, uMsg, wParam, lParam);
#ifdef DEBUG
    pHeader = NULL;
#endif

    //
    // return the result
    //
BailCritical:
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

BailNonCritical:
    return lResult;
}

//-----------------------------------------------------------------------------
// UpdateDeepestCall
//
// this procedure updates the deepest call index for the specified frame
//
//-----------------------------------------------------------------------------
void UpdateDeepestCall(SUBCLASS_FRAME *pFrame)
{
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;     // we are partying on the frame list
#endif

    if (pFrame->pFramePrev &&
        (pFrame->pFramePrev->uDeepestCall < pFrame->uCallIndex))
    {
        pFrame->uDeepestCall = pFrame->pFramePrev->uDeepestCall;
    }
    else
        pFrame->uDeepestCall = pFrame->uCallIndex;
}

//-----------------------------------------------------------------------------
// EnterSubclassFrame
//
// this procedure sets up a new subclass frame for the specified header, saving
// away the previous one
//
//-----------------------------------------------------------------------------
__inline void EnterSubclassFrame(SUBCLASS_HEADER *pHeader,
    SUBCLASS_FRAME *pFrame)
{
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;     // we are partying on the header and frame list
#endif

    //
    // fill in the frame and link it into the header
    //
    pFrame->uCallIndex   = pHeader->uRefs;
    pFrame->pFramePrev   = pHeader->pFrameCur;
    pFrame->pHeader      = pHeader;
    pHeader->pFrameCur   = pFrame;

    //
    // initialize the deepest call index for this frame
    //
    UpdateDeepestCall(pFrame);
}

//-----------------------------------------------------------------------------
// LeaveSubclassFrame
//
// this procedure cleans up the current subclass frame for the specified
// header, restoring the previous one
//
//-----------------------------------------------------------------------------
__inline SUBCLASS_HEADER *LeaveSubclassFrame(SUBCLASS_FRAME *pFrame)
{
    SUBCLASS_HEADER *pHeader;

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;     // we are partying on the header
#endif

    //
    // unlink the frame from its header (if it still exists)
    //
    if ((pHeader = pFrame->pHeader) != NULL)
        pHeader->pFrameCur = pFrame->pFramePrev;

    return pHeader;
}

//-----------------------------------------------------------------------------
// SubclassFrameException
//
// this procedure cleans up when an exception is thrown from a subclass frame
//
//-----------------------------------------------------------------------------
void SubclassFrameException(SUBCLASS_FRAME *pFrame)
{
    //
    // clean up the current subclass frame
    //

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#endif
    DebugMsg(TF_ALWAYS, TEXT("warning: cleaning up subclass frame after exception"));
    LeaveSubclassFrame(pFrame);
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#endif
}

//-----------------------------------------------------------------------------
// MasterSubclassProc
//
// this is the window procedure we install to dispatch subclass callbacks.
// it maintains a linked list of 'frames' through the stack which allow
// DefSubclassProc to call the right subclass procedure in multiple-message
// scenarios.
//
//-----------------------------------------------------------------------------
LRESULT CALLBACK MasterSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    SUBCLASS_FRAME Frame;
    SUBCLASS_HEADER *pHeader;
    LRESULT lResult = 0;

    //
    // prevent people from partying on the callback chain while we look at it
    //

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif
    //
    // freak out if we got here and we don't have our data
    //
    if ((pHeader = FastGetSubclassHeader(hWnd)) == NULL)
    {
#ifdef FREETHREADEDSUBCLASSGOOP
        LEAVECRITICAL;
#else
        ASSERT(IsWindowOnCurrentThread(hWnd));        
#endif
        return SubclassDeath(hWnd, uMsg, wParam, lParam);
    }

    //
    // set up a new subclass frame and save away the previous one
    //
    EnterSubclassFrame(pHeader, &Frame);

    __try   // protect our state information from exceptions
    {
        //
        // go ahead and call the subclass chain on this frame
        //
        // WARNING: this call temporarily releases the critical section
        // WARNING: pHeader is invalid when this call returns
        //
        lResult =
            CallNextSubclassProc(pHeader, hWnd, uMsg, wParam, lParam);
#ifdef DEBUG
        pHeader = NULL;
#endif
    }
    __except ((SubclassFrameException(&Frame), EXCEPTION_CONTINUE_SEARCH))
    {
        ASSERT(FALSE);
    }
    __endexcept

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;
#else
    ASSERT(IsWindowOnCurrentThread(hWnd));    
#endif

    //
    // restore the previous subclass frame
    //
    pHeader = LeaveSubclassFrame(&Frame);

    //
    // if the header is gone we have already cleaned up in a nested frame
    //
    if (!pHeader)
        goto BailOut;

    //
    // was the window nuked (somehow) without us seeing the WM_NCDESTROY?
    //
    if (!IsWindow(hWnd))
    {
        //
        // EVIL! somebody subclassed after us and didn't pass on WM_NCDESTROY
        //
        AssertMsg(FALSE, TEXT("unknown subclass proc swallowed a WM_NCDESTROY"));

        // go ahead and clean up now
        hWnd = NULL;
        uMsg = WM_NCDESTROY;
    }

    //
    // if we are returning from a WM_NCDESTROY then we need to clean up
    //
    if (uMsg == WM_NCDESTROY)
    {
        DetachSubclassHeader(hWnd, pHeader, TRUE);
        goto BailOut;
    }

    //
    // is there any pending cleanup, or are all our clients gone?
    //
    if (pHeader->uCleanup || (!pHeader->pFrameCur && (pHeader->uRefs <= 1)))
    {
        CompactSubclassHeader(hWnd, pHeader);
#ifdef DEBUG
        pHeader = NULL;
#endif
    }

    //
    // all done
    //
BailOut:
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#endif
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTNONCRITICAL;
#endif
    return lResult;
}

//-----------------------------------------------------------------------------
// EnterSubclassCallback
//
// this procedure finds the next callback in the subclass chain and updates
// pFrame to indicate that we are calling it
//
//-----------------------------------------------------------------------------
UINT EnterSubclassCallback(SUBCLASS_HEADER *pHeader, SUBCLASS_FRAME *pFrame,
    SUBCLASS_CALL *pCallChosen)
{
    SUBCLASS_CALL *pCall;
    UINT uDepth;

    //
    // we will be scanning the subclass chain and updating frame data
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;
#endif

    //
    // scan the subclass chain for the next callable subclass callback
    //
    pCall = pHeader->CallArray + pFrame->uCallIndex;
    uDepth = 0;
    do
    {
        uDepth++;
        pCall--;

    } while (!pCall->pfnSubclass);

    //
    // copy the callback information for the caller
    //
    pCallChosen->pfnSubclass = pCall->pfnSubclass;
    pCallChosen->uIdSubclass = pCall->uIdSubclass;
    pCallChosen->dwRefData   = pCall->dwRefData;

    //
    // adjust the frame's call index by the depth we entered
    //
    pFrame->uCallIndex -= uDepth;

    //
    // keep the deepest call index up to date
    //
    UpdateDeepestCall(pFrame);

    return uDepth;
}

//-----------------------------------------------------------------------------
// LeaveSubclassCallback
//
// this procedure finds the next callback in the cal
//
//-----------------------------------------------------------------------------
__inline void LeaveSubclassCallback(SUBCLASS_FRAME *pFrame, UINT uDepth)
{
    //
    // we will be updating subclass frame data
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;
#endif

    //
    // adjust the frame's call index by the depth we entered and return
    //
    pFrame->uCallIndex += uDepth;

    //
    // keep the deepest call index up to date
    //
    UpdateDeepestCall(pFrame);
}

//-----------------------------------------------------------------------------
// SubclassCallbackException
//
// this procedure cleans up when a subclass callback throws an exception
//
//-----------------------------------------------------------------------------
void SubclassCallbackException(SUBCLASS_FRAME *pFrame, UINT uDepth)
{
    //
    // clean up the current subclass callback
    //

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#endif
    DebugMsg(TF_ALWAYS, TEXT("warning: cleaning up subclass callback after exception"));
    LeaveSubclassCallback(pFrame, uDepth);
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#endif
}

//-----------------------------------------------------------------------------
// CallNextSubclassProc
//
// this procedure calls the next subclass callback in the subclass chain
//
// WARNING: this call temporarily releases the critical section
// WARNING: pHeader is invalid when this call returns
//
//-----------------------------------------------------------------------------
LRESULT CallNextSubclassProc(SUBCLASS_HEADER *pHeader, HWND hWnd, UINT uMsg,
    WPARAM wParam, LPARAM lParam)
{
    SUBCLASS_CALL Call;
    SUBCLASS_FRAME *pFrame;
    LRESULT lResult;
    UINT uDepth;

#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTCRITICAL;     // sanity
#endif
    ASSERT(pHeader);    // paranoia

    //
    // get the current subclass frame
    //
    pFrame = pHeader->pFrameCur;
    ASSERT(pFrame);

    //
    // get the next subclass call we need to make
    //
    uDepth = EnterSubclassCallback(pHeader, pFrame, &Call);

    //
    // leave the critical section so we don't deadlock in our callback
    //
    // WARNING: pHeader is invalid when this call returns
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    LEAVECRITICAL;
#endif
#ifdef DEBUG
    pHeader = NULL;
#endif

    //
    // we call the outside world so prepare to deadlock if we have the critsec
    //
#ifdef FREETHREADEDSUBCLASSGOOP
    ASSERTNONCRITICAL;
#endif

    __try   // protect our state information from exceptions
    {
        //
        // call the chosen subclass proc
        //
        ASSERT(Call.pfnSubclass);

        lResult = Call.pfnSubclass(hWnd, uMsg, wParam, lParam,
            Call.uIdSubclass, Call.dwRefData);
    }
    __except ((SubclassCallbackException(pFrame, uDepth),
        EXCEPTION_CONTINUE_SEARCH))
    {
        ASSERT(FALSE);
    }
    __endexcept

    //
    // we left the critical section before calling out so re-enter it
    //

#ifdef FREETHREADEDSUBCLASSGOOP
    ENTERCRITICAL;
#endif
    
    //
    // finally, clean up and return
    //
    LeaveSubclassCallback(pFrame, uDepth);
    return lResult;
}

///////////////////////////////////////////////////////////////////////////////

#if defined(RETAIL_ZOMBIE_MESSAGE_WNDPROC) || defined(DEBUG)
#ifdef DEBUG
static const TCHAR c_szZombieMessage[] =                                     \
    TEXT("This window has encountered an internal error which is preventing ")    \
    TEXT("it from operating normally.\r\n\nPlease report this problem to ")       \
    TEXT("FrancisH immediately.");
#else
static const TCHAR c_szZombieMessage[] =                                     \
    TEXT("This window has encountered an internal error which is preventing ")    \
    TEXT("it from operating normally.\r\n\nPlease report this as a bug in the ")  \
    TEXT("COMCTL32 library.");
#endif

LRESULT ZombieWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        {
            HDC hDC = (HDC)wParam;
            HBRUSH hBrush = CreateSolidBrush(RGB(255,255,0));

            if (hBrush)
            {
                RECT rcErase;

                switch (GetClipBox(hDC, &rcErase))
                {
                default:
                    FillRect(hDC, &rcErase, hBrush);
                    break;
                case NULLREGION:
                case ERROR:
                    break;
                }

                DeleteBrush(hBrush);
            }
        }
        return 1;

    case WM_PAINT:
        {
            RECT rcClient;
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);

            if (hDC && GetClientRect(hWnd, &rcClient))
            {
                COLORREF clrBkSave = SetBkColor(hDC, RGB(255,255,0));
                COLORREF clrFgSave = SetTextColor(hDC, RGB(255,0,0));

                DrawText(hDC, c_szZombieMessage, -1, &rcClient,
                    DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK |
                    DT_WORD_ELLIPSIS);

                SetTextColor(hDC, clrFgSave);
                SetBkColor(hDC, clrBkSave);
            }

            EndPaint(hWnd, &ps);
        }
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
#endif

///////////////////////////////////////////////////////////////////////////////
