/****************************** Module Header ******************************\
* Module Name: handles.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* HANDLES.C - Data handle manager
*
*
* This module allows a 32 bit value to be converted into a handle that
* can be validated with a high probability of correctness.
*
* A handle array is kept which contains the 32 bit data associated with
* it, and a copy of the correect handle value. The handle itself is
* composed of a combination of the index into the array for the associated
* data, the instance value, a type value and a DDEML instance value.
*
* The HIWORD of a handle is guarenteed not to be 0.
*
* History:
* 10-28-91 Sanfords Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

// globals

PCHANDLEENTRY aHandleEntry = NULL;

// statics

int cHandlesAllocated = 0;
int iFirstFree = 0;
DWORD nextId = 1;

#define GROW_COUNT 16
// #define TESTING
#ifdef TESTING
VOID CheckHandleTable()
{
    int i;

    for (i = 0; i < cHandlesAllocated; i++) {
        if (aHandleEntry[i].handle && aHandleEntry[i].dwData) {
            switch (TypeFromHandle(aHandleEntry[i].handle)) {
            case HTYPE_INSTANCE:
                UserAssert(((PCL_INSTANCE_INFO)aHandleEntry[i].dwData)->hInstClient == aHandleEntry[i].handle);
                break;

            case HTYPE_CLIENT_CONVERSATION:
            case HTYPE_SERVER_CONVERSATION:
                UserAssert(((PCONV_INFO)aHandleEntry[i].dwData)->hConv == (HCONV)aHandleEntry[i].handle ||
                        ((PCONV_INFO)aHandleEntry[i].dwData)->hConv == 0);
                break;
            }
        }
    }
}
#else
#define CheckHandleTable()
#endif // TESTING


/***************************************************************************\
* CreateHandle
*
* Description:
* Creates a client side handle.
*
* Returns 0 on error.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HANDLE CreateHandle(
ULONG_PTR dwData,
DWORD type,
DWORD inst)
{
    HANDLE h;
    int i, iNextFree;
    PCHANDLEENTRY phe;

    if (iFirstFree >= cHandlesAllocated) {
        if (cHandlesAllocated == 0) {
           aHandleEntry = (PCHANDLEENTRY)DDEMLAlloc(sizeof(CHANDLEENTRY) * GROW_COUNT);
        } else {
           aHandleEntry = (PCHANDLEENTRY)DDEMLReAlloc(aHandleEntry,
               sizeof(CHANDLEENTRY) * (cHandlesAllocated + GROW_COUNT));
        }
        if (aHandleEntry == NULL) {
            return (0);
        }
        i = cHandlesAllocated;
        cHandlesAllocated += GROW_COUNT;
        phe = &aHandleEntry[i];
        while (i < cHandlesAllocated) {
           // phe->handle = 0; // indicates empty - ZERO init.
           phe->dwData = ++i; // index to next free spot.
           phe++;
        }
    }
    h = aHandleEntry[iFirstFree].handle = (HANDLE)LongToHandle(
         HandleFromId(nextId) |
         HandleFromIndex(iFirstFree) |
         HandleFromType(type) |
         HandleFromInst(inst) );
    iNextFree = (int)aHandleEntry[iFirstFree].dwData;
    aHandleEntry[iFirstFree].dwData = dwData;
    nextId++;
    if (nextId == 0) {     // guarentees HIWORD of handle != 0
       nextId++;
    }
    iFirstFree = iNextFree;

    CheckHandleTable();
    return (h);
}


/***************************************************************************\
* DestroyHandle
*
* Description:
* Frees up a handle.
*
* Assumptions:
* The handle is valid.
* Critical Section is entered.
*
* Returns:
* Data in handle before destruction.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
ULONG_PTR DestroyHandle(
HANDLE h)
{
    register int i;
    register ULONG_PTR dwRet;

    CheckHandleTable();

    i = IndexFromHandle(h);
    UserAssert(aHandleEntry[i].handle == h);
    aHandleEntry[i].handle = 0;
    dwRet = aHandleEntry[i].dwData;
    aHandleEntry[i].dwData = iFirstFree;
    iFirstFree = i;

    return (dwRet);
}


/***************************************************************************\
* GetHandleData
*
* Description:
* A quick way to retrieve a valid handle's data
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
ULONG_PTR GetHandleData(
HANDLE h)
{
    register ULONG_PTR dwRet;

    CheckHandleTable();
    dwRet = aHandleEntry[IndexFromHandle(h)].dwData;
    return (dwRet);
}


/***************************************************************************\
* SetHandleData
*
* Description:
* A quick way to change a valid handle's data.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
VOID SetHandleData(
HANDLE h,
ULONG_PTR dwData)
{
    aHandleEntry[IndexFromHandle(h)].dwData = dwData;
}


/***************************************************************************\
* ValidateCHandle
*
* Description:
* General handle validation routine. ExpectedType or ExpectedInstance
* can be HTYPE_ANY/HINST_ANY. (note Expected Instance is an instance
* index into the aInstance array, NOT a instance handle.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
ULONG_PTR ValidateCHandle(
HANDLE h,
DWORD ExpectedType,
DWORD ExpectedInstance)
{
    register int i;
    register ULONG_PTR dwRet;

    CheckHandleTable();
    dwRet = 0;
    i = IndexFromHandle(h);
    if (i < cHandlesAllocated &&
          aHandleEntry[i].handle == h &&
          (ExpectedType == -1 || ExpectedType == TypeFromHandle(h)) &&
          (ExpectedInstance == -1 || ExpectedInstance == InstFromHandle(h))) {
       dwRet = aHandleEntry[i].dwData;
    }

    return (dwRet);
}


PCL_INSTANCE_INFO PciiFromHandle(
HANDLE h)
{
    PCHANDLEENTRY phe;

    CheckDDECritIn;

    if (!cHandlesAllocated) {
        return(NULL);
    }
    phe = &aHandleEntry[cHandlesAllocated];

    do {
        phe--;
        if (phe->handle != 0 &&
                TypeFromHandle(phe->handle) == HTYPE_INSTANCE &&
                (InstFromHandle(phe->handle) == InstFromHandle(h))) {
            return(((PCL_INSTANCE_INFO)phe->dwData)->tid == GetCurrentThreadId() ?
                (PCL_INSTANCE_INFO)phe->dwData : NULL);
        }
    } while (phe != aHandleEntry);
    return(NULL);
}



/***************************************************************************\
* ApplyFunctionToObjects
*
* Description:
* Used for cleanup, this allows the handle array to be scanned for
* handles meeting the ExpectedType and ExpectedInstance criteria
* and apply the given function to each handle.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
VOID ApplyFunctionToObjects(
DWORD ExpectedType,
DWORD ExpectedInstance,
PFNHANDLEAPPLY pfn)
{
    PCHANDLEENTRY phe;

    CheckDDECritIn;

    if (!cHandlesAllocated) {
        return;
    }
    phe = &aHandleEntry[cHandlesAllocated];

    do {
        phe--;
        if (phe->handle != 0 &&
                (ExpectedType == HTYPE_ANY ||
                    ExpectedType == TypeFromHandle(phe->handle)) &&
                (ExpectedInstance == HTYPE_ANY ||
                    ExpectedInstance == InstFromHandle(phe->handle))) {
            LeaveDDECrit;
            CheckDDECritOut;
            (*pfn)(phe->handle);
            EnterDDECrit;
        }
    } while (phe != aHandleEntry);
}


DWORD GetFullUserHandle(WORD wHandle)
{
    DWORD dwHandle;
    PHE phe;

    dwHandle = HMIndexFromHandle(wHandle);

    if (dwHandle < gpsi->cHandleEntries) {

        phe = &gSharedInfo.aheList[dwHandle];

        if (phe->bType == TYPE_WINDOW)
            return(MAKELONG(dwHandle, phe->wUniq));
    }

    /*
     * object may be gone, but we must pass something.
     * DDE terminates will fail if we don't map this right even after
     * the window is dead!
     *
     * NOTE: This fix will only work for WOW apps, but since the 32bit
     * tracking layer locks dde windows until the last terminate is
     * received, we won't see this problem on the 32bit side.
     *
     * BUG: We WILL see a problem for OLE32 thunked DDE though.
     */
    return(wHandle);
}



/***************************************************************************\
* BestSetLastDDEMLError
*
* Description:
* This sets the LastError field of all instances that belong to the
* current thread. This is used to get error information to applications
* which generated an error where the exact instance could not be
* determined.
*
* History:
* 11-12-91 sanfords Created.
\***************************************************************************/
VOID BestSetLastDDEMLError(
DWORD error)
{
    PCHANDLEENTRY phe;

    CheckDDECritIn;

    if (!cHandlesAllocated) {
        return;
    }
    phe = &aHandleEntry[cHandlesAllocated];
    do {
        phe--;
        if (phe->handle != 0 && TypeFromHandle(phe->handle) == HTYPE_INSTANCE) {
            SetLastDDEMLError((PCL_INSTANCE_INFO)phe->dwData, error);
        }
    } while (phe != aHandleEntry);
}
