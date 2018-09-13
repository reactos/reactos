/****************************** Module Header ******************************\
* Module Name: winprop.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains routines having to do with window properties.
*
* History:
* 11-13-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* InternalSetProp
*
* SetProp searches the linked-list of window property structures for the
* specified key.  If found, the existing property structure is changed to
* hold the new hData handle.  If no property is found with the specified key
* a new property structure is created and initialized.
*
* Since property keys are retained as atoms, we convert the incoming pszKey
* to an atom before lookup or storage.  pszKey might actually be an atom
* already, so we keep a flag, PROPF_STRING, so we know whether the atom was
* created by the system or whether it was passed in.  This way we know
* whether we should destroy it when the property is destroyed.
*
* Several property values are for User's private use.  These properties are
* denoted with the flag PROPF_INTERNAL.  Depending on the fInternal flag,
* either internal (User) or external (application) properties are set/get/
* removed/enumerated, etc.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

BOOL InternalSetProp(
    PWND pwnd,
    LPWSTR pszKey,
    HANDLE hData,
    DWORD dwFlags)
{
    PPROP pprop;

    if (pszKey == NULL) {
        RIPERR0(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"pszKey\" (NULL) to InternalSetProp");

        return FALSE;
    }

    /*
     * If no property list exists for this window, create one.
     */
    pprop = _FindProp(pwnd, pszKey, dwFlags & PROPF_INTERNAL);
    if (pprop == NULL) {

        /*
         * pszKey must be an atom within the server.
         */
        UserAssert(!IS_PTR(pszKey));

        /*
         * CreateProp allocates the property and links it into the window's
         * property list.
         */
        pprop = CreateProp(pwnd);
        if (pprop == NULL)
            return FALSE;

        pprop->atomKey = PTR_TO_ID(pszKey);
        pprop->fs = (WORD)dwFlags;
    }

    pprop->hData = hData;

    return TRUE;
}


/***************************************************************************\
* InternalRemoveProp
*
* Remove the specified property from the specified window's property list.
* The property's hData handle is returned to the caller who can then free
* it or whatever.  NOTE: This also applies to internal properties as well --
* InternalRemoveProp will free the property structure and atom (if created
* by User) but will not free the hData itself.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

HANDLE InternalRemoveProp(
    PWND pwnd,
    LPWSTR pszKey,
    BOOL fInternal)
{
    PPROP pprop;
    PPROP ppropLast;
    HANDLE hT;

    /*
     * Find the property to be removed.
     */
    pprop = _FindProp(pwnd, pszKey, fInternal);
    if (pprop == NULL)
        return NULL;

    /*
     * Remember what it was pointing at.
     */
    hT = pprop->hData;

    /*
     * Move the property at the end of the list into this slot.
     */
    pwnd->ppropList->iFirstFree--;
    ppropLast = &pwnd->ppropList->aprop[pwnd->ppropList->iFirstFree];
    *pprop = *ppropLast;
    RtlZeroMemory(ppropLast, sizeof(*ppropLast));

    return hT;
}


/***************************************************************************\
* _BuildPropList
*
* This is a unique client/server routine - it builds a list of Props and
* returns it to the client.  Unique since the client doesn't know how
* big the list is ahead of time.
*
* 29-Jan-1992 JohnC    Created.
\***************************************************************************/

NTSTATUS _BuildPropList(
    PWND pwnd,
    PROPSET aPropSet[],
    UINT cPropMax,
    PUINT pcPropNeeded)
{
    UINT i;
    PPROPLIST ppropList;
    PPROP pProp;
    DWORD iRetCnt = 0;            // The number of Props returned
    DWORD iProp = 0;
    PPROPSET pPropSetLast = (aPropSet + cPropMax - 1);
    NTSTATUS Status;

    /*
     * If the Window does not have a property list then we're done
     */
    ppropList = pwnd->ppropList;
    if (ppropList == NULL) {
        *pcPropNeeded = 0;
        return STATUS_SUCCESS;
    }

    /*
     * For each element in the property list enumerate it.
     * (only if it is not internal!)
     */
    Status = STATUS_SUCCESS;
    pProp = ppropList->aprop;
    for (i = ppropList->iFirstFree; i > 0; i--) {

        /*
         * if we run out of space in shared memory return
         * STATUS_BUFFER_TOO_SMALL
         */
        if (&aPropSet[iProp] > pPropSetLast) {

            /*
             * Reset to the beginning of the output
             * buffer so we can continue and compute
             * the needed space.
             */
            iProp = 0;
            Status = STATUS_BUFFER_TOO_SMALL;
        }

        if (!(pProp->fs & PROPF_INTERNAL)) {
            aPropSet[iProp].hData = pProp->hData;
            aPropSet[iProp].atom = pProp->atomKey;
            iProp++;
            iRetCnt++;
        }
        pProp++;
    }

    /*
     * Return the number of PROPLISTs given back to the client
     */

    *pcPropNeeded = iRetCnt;

    return Status;
}


/***************************************************************************\
* CreateProp
*
* Create a property structure and link it at the head of the specified
* window's property list.
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

PPROP CreateProp(
    PWND pwnd)
{
    PPROPLIST ppropList;
    PPROP pprop;

    if (pwnd->ppropList == NULL) {
        pwnd->ppropList = (PPROPLIST)DesktopAlloc(pwnd->head.rpdesk,
                                                  sizeof(PROPLIST),
                                                  DTAG_PROPLIST);
        if (pwnd->ppropList == NULL) {
            return NULL;
        }
        pwnd->ppropList->cEntries = 1;
    } else if (pwnd->ppropList->iFirstFree == pwnd->ppropList->cEntries) {
        ppropList = (PPROPLIST)DesktopAlloc(pwnd->head.rpdesk,
                                            sizeof(PROPLIST) + pwnd->ppropList->cEntries * sizeof(PROP),
                                            DTAG_PROPLIST);
        if (ppropList == NULL) {
            return NULL;
        }
        RtlCopyMemory(ppropList, pwnd->ppropList, sizeof(PROPLIST) + (pwnd->ppropList->cEntries - 1) * sizeof(PROP));
        DesktopFree(pwnd->head.rpdesk, pwnd->ppropList);
        pwnd->ppropList = ppropList;
        pwnd->ppropList->cEntries++;
    }
    pprop = &pwnd->ppropList->aprop[pwnd->ppropList->iFirstFree];
    pwnd->ppropList->iFirstFree++;

    return pprop;
}


/***************************************************************************\
* DeleteProperties
*
* When a window is destroyed we want to destroy all its accompanying
* properties.  DestroyProperties does this, including destroying any hData
* that was allocated by User for internal properties.  Any atoms created
* along with the properties are destroyed as well.  hData in application
* properties are not destroyed automatically; we assume the application
* is taking care of that itself (in its WM_DESTROY handler or similar).
*
* History:
* 11-14-90 darrinm      Rewrote from scratch with new data structures and
*                       algorithms.
\***************************************************************************/

void DeleteProperties(
    PWND pwnd)
{
    PPROP pprop;
    UINT i;

    UserAssert(pwnd->ppropList);

    /*
     * Loop through the whole list of properties on this window.
     */
    pprop = pwnd->ppropList->aprop;
    for (i = pwnd->ppropList->iFirstFree; i > 0; i--) {

        /*
         * Is this an internal property?  If so, free any data we allocated
         * for it.
         */
        if ((pprop->fs & PROPF_INTERNAL) && !(pprop->fs & PROPF_NOPOOL)) {
                UserFreePool(pprop->hData);
        }

        /*
         * Advance to the next property in the list.
         */
        pprop++;
    }

    /*
     * All properties gone, free the property list and clear out the
     * window's property list pointer.
     */
    DesktopFree(pwnd->head.rpdesk, pwnd->ppropList);
    pwnd->ppropList = NULL;
}

