/****************************** Module Header ******************************\
* Module Name: instance.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module handles conversion of instance handles (server side handles)
* to instance indecies used by the handle manager for associating a handle
* with a particular instance.
*
* History:
* 11-5-91 Sanfords Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define INST_GROW_COUNT 4

// globals

PHANDLE aInstance = NULL;
int cInstAllocated = 0;
int iFirstFreeInst = 0;


/***************************************************************************\
* AddInstance
*
* Description:
* Adds a server side instance handle to the instance handle array.
* The array index becomes the client-side unique instance index used for
* identifying other client side handles.
*
* Returns:
* client side instance handle or 0 on error.
*
* History:
* 11-1-91 sanfords Created.
\***************************************************************************/
HANDLE AddInstance(
HANDLE hInstServer)
{
    int i, iNextFree;
    PHANDLE ph;

    if (iFirstFreeInst >= cInstAllocated) {
        if (cInstAllocated == 0) {
           aInstance = (PHANDLE)DDEMLAlloc(sizeof(HANDLE) * INST_GROW_COUNT);
        } else {
           aInstance = (PHANDLE)DDEMLReAlloc((PVOID)aInstance,
                 sizeof(HANDLE) * (cInstAllocated + INST_GROW_COUNT));
        }
        if (aInstance == 0) {
            return (0);
        }
        ph = &aInstance[cInstAllocated];
        i = cInstAllocated + 1;
        while (i <= cInstAllocated + INST_GROW_COUNT) {
           *ph++ = (HANDLE)(UINT_PTR)(UINT)i++;
        }
        cInstAllocated += INST_GROW_COUNT;
    }
    iNextFree = HandleToUlong(aInstance[iFirstFreeInst]);
    if (iNextFree > MAX_INST) {
        /*
         * Instance limit for this process exceeded!
         */
        return(0);
    }
    aInstance[iFirstFreeInst] = hInstServer;
    i = iFirstFreeInst;
    iFirstFreeInst = iNextFree;
    return (CreateHandle(0, HTYPE_INSTANCE, i));
}


/***************************************************************************\
* DestroyInstance
*
* Description:
* Removes an instance from the aInstance table. This does nothing for
* the server side instance info.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
HANDLE DestroyInstance(
HANDLE hInstClient)
{
    register HANDLE hInstServerRet = 0;

    DestroyHandle(hInstClient);
    hInstServerRet = aInstance[InstFromHandle(hInstClient)];
    aInstance[InstFromHandle(hInstClient)] = (HANDLE)UIntToPtr( iFirstFreeInst );
    iFirstFreeInst = InstFromHandle(hInstClient);

    return (hInstServerRet);
}


/***************************************************************************\
* ValidateInstance
*
* Description:
* Verifies the current validity of an instance handle - which is a server
* side handle that also references a client side data structure (pcii).
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
PCL_INSTANCE_INFO ValidateInstance(
HANDLE hInstClient)
{
    PCL_INSTANCE_INFO pcii;

    pcii = (PCL_INSTANCE_INFO)ValidateCHandle(hInstClient, HTYPE_INSTANCE, HINST_ANY);

    if (pcii != NULL) {
        if (pcii->tid != GetCurrentThreadId() ||
                pcii->hInstClient != hInstClient) {
            return (NULL);
        }
    }
    return (pcii);
}


/***************************************************************************\
* SetLastDDEMLError
*
* Description:
* Sets last error value and generates monitor events if monitoring.
*
* History:
* 11-19-91 sanfords Created.
\***************************************************************************/
VOID SetLastDDEMLError(
PCL_INSTANCE_INFO pcii,
DWORD error)
{
    PEVENT_PACKET pep;

    if (pcii->MonitorFlags & MF_ERRORS && !(pcii->afCmd & APPCLASS_MONITOR)) {
        pep = (PEVENT_PACKET)DDEMLAlloc(sizeof(EVENT_PACKET) - sizeof(DWORD) +
                sizeof(MONERRSTRUCT));
        if (pep != NULL) {
            pep->EventType =    MF_ERRORS;
            pep->fSense =       TRUE;
            pep->cbEventData =  sizeof(MONERRSTRUCT);
#define perrs ((MONERRSTRUCT *)&pep->Data)
            perrs->cb =      sizeof(MONERRSTRUCT);
            perrs->wLastError = (WORD)error;
            perrs->dwTime =  NtGetTickCount();
            perrs->hTask =   (HANDLE)LongToHandle( pcii->tid );
#undef perrs
            LeaveDDECrit;
            Event(pep);
            EnterDDECrit;
        }
    }
#if DBG
    if (error != 0 && error != DMLERR_NO_CONV_ESTABLISHED) {
        RIPMSG3(RIP_WARNING,
                "DDEML Error set=%x, Client Instance=%x, Process=%x.",
                error, pcii, GetCurrentProcessId());
    }
#endif
    pcii->LastError = error;
}


