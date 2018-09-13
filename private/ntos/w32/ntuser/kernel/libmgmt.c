/****************************** Module Header ******************************\
* Module Name: libmgmt.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the code to manage loading and freeing libraries
* in use by USER.
*
* History:
* 02-04-91 DavidPe      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/*
 * Globals local to this file
 *
 *
 * Number of hmodule entries in the module management table.
 */
int catomSysTableEntries;

/*
 * Array of atoms that are the fully qualified path names of each managed
 * module.
 */
ATOM aatomSysLoaded[CLIBS];

/*
 * Count of processes that have LoadModule()'d each module.
 */
int acatomSysUse[CLIBS];

/*
 * Count of hooks set into each module.
 */
int acatomSysDepends[CLIBS];


/****************************************************************************\
* GetHmodTableIndex
*
* This routine is used to return the index of a given atom within the system
* wide hmod atom table.  If the atom is not found, an attempt to allocate a
* new table entry is made.  If the attempt fails, -1 is returned.
*
* History:
* 02-04-91  DavidPe         Ported.
\****************************************************************************/

int GetHmodTableIndex(
    PUNICODE_STRING pstrLibName)
{
    int i;
    ATOM atom;
    UNICODE_STRING strLibName;

    /*
     * Probe string
     */
    try {
        strLibName = ProbeAndReadUnicodeString(pstrLibName);
        ProbeForReadUnicodeStringBuffer(strLibName);
        atom = UserAddAtom(strLibName.Buffer, FALSE);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
        return -1;
    }

    /*
     * If we can't add the atom we're hosed
     * so return an error.
     */
    if (atom == 0) {
        return -1;
    }

    /*
     * Search for atom index
     */
    for (i = 0; i < catomSysTableEntries && aatomSysLoaded[i] != atom; i++)
        ;

    if (i == catomSysTableEntries) {

        /*
         * Find empty entry for atom
         */
        for (i = 0; i < catomSysTableEntries && aatomSysLoaded[i]; i++)
            ;

        /*
         * Check if no empty entry found
         */
        if (i == catomSysTableEntries) {
            if (i == CLIBS) {
                UserDeleteAtom(atom);
                RIPERR0(ERROR_NOT_ENOUGH_MEMORY,
                        RIP_WARNING,
                        "Memory allocation failed in GetHmodTableIndex");

                return -1;
            }

            /*
             * Increase table size
             */
            catomSysTableEntries++;
        }

        /*
         * Set entry
         */
        aatomSysLoaded[i] = atom;
        acatomSysUse[i] = 0;
        acatomSysDepends[i] = 0;
    } else {
        UserDeleteAtom(atom);
    }

    return i;
}


/*****************************************************************************\
* AddHmodDependency
*
* This function merely increments the dependency count of a given hmod
* atom table index.
*
* History:
* 02-04-91  DavidPe         Ported.
\*****************************************************************************/

VOID AddHmodDependency(
    int iatom)
{
    UserAssert(iatom >= 0);
    if (iatom < catomSysTableEntries) {
        acatomSysDepends[iatom]++;
    }
}


/*****************************************************************************\
* RemoveHmodDependency
*
* This function removes a system dependency on a given index into the hmod
* atom table.  If all dependencies on the hmod have been removed (the Depends
* count  reaches zero) then the QS_SYSEXPUNGE bit is set in all message
* queues so the eventually each process will do a free module on it.
*
* History:
* 02-04-91  DavidPe         Ported.
\*****************************************************************************/

VOID RemoveHmodDependency(
    int iatom)
{

    UserAssert(iatom >= 0);
    if (iatom < catomSysTableEntries &&
        --acatomSysDepends[iatom] == 0) {

        if (acatomSysUse[iatom]) {

            /*
             * Cause each thread to check for expunged dlls
             * the next time they awake.
             */
            gcSysExpunge++;
            gdwSysExpungeMask |= (1 << iatom);
        } else {
            aatomSysLoaded[iatom] = 0;
        }
    }
}


/*****************************************************************************\
* xxxLoadHmodIndex
*
* This function attempts to load the hmodule specified by iatom into the
* system hmod table.  Updates the per-process bitmap accordingly.  Returns
* NULL on success.
*
* History:
* 02-04-91  DavidPe         Ported.
\*****************************************************************************/

HANDLE xxxLoadHmodIndex(
    int iatom,
    BOOL bWx86KnownDll)
{
    WCHAR pszLibName[MAX_PATH];
    HANDLE hmod;
    UNICODE_STRING strLibrary;
    PTHREADINFO    ptiCurrent = PtiCurrent();

    UserAssert((!gptiRit || gptiRit->ppi != PtiCurrent()->ppi) &&
                "Shouldn't load global hooks on system process - gptiRit->ppi is the system process");

    if (iatom >= catomSysTableEntries) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Index out of range");
        return NULL;
    }

    UserGetAtomName(aatomSysLoaded[iatom], pszLibName, sizeof(pszLibName)/sizeof(WCHAR));

    /*
     * Call back the client to load the library.
     */
    RtlInitUnicodeString(&strLibrary, pszLibName);
    hmod = ClientLoadLibrary(&strLibrary, bWx86KnownDll);

    if (hmod != NULL) {
        /*
         * Check to make sure another thread hasn't loaded this library
         * while we were outside the critical section.
         */
        if (!TESTHMODLOADED(ptiCurrent, iatom)) {
            /*
             * Go ahead and bump the reference count.
             */
            acatomSysUse[iatom]++;
            SETHMODLOADED(ptiCurrent, iatom, hmod);

        } else {
            /*
             * Another thread loaded it while we were outside the
             * critical section.  Unload it so the system's
             * reference count is correct.
             */
            ClientFreeLibrary(ptiCurrent->ppi->ahmodLibLoaded[iatom]);
        }
    }

    return hmod;
}


/***********************************************************************\
* DoSysExpunge
*
* This function is called when a thread wakes up and finds its
* QS_SYSEXPUNGE wakebit set.
*
* History:
* 02-04-91  DavidPe         Ported.
\***********************************************************************/

VOID xxxDoSysExpunge(
    PTHREADINFO pti)
{
    int i;

    /*
     * Clear this first before we potentially leave the critical section.
     */
    pti->ppi->cSysExpunge = gcSysExpunge;

    /*
     * Scan for libraries that have been freed
     */
    for (i = 0; i < catomSysTableEntries; i++) {
        if ((acatomSysDepends[i] == 0) && (aatomSysLoaded[i] != 0) &&
                TESTHMODLOADED(pti, i)) {

            HANDLE hmodFree = pti->ppi->ahmodLibLoaded[i];

            /*
             * Clear this hmod for this process before we leave the
             * critical section.
             */
            CLEARHMODLOADED(pti, i);

            /*
             * Decrement the count of processes that have loaded this
             * .dll.  If there are no more, then destroy the reference
             * to this .dll.
             */
            if (--acatomSysUse[i] == 0) {
                UserDeleteAtom(aatomSysLoaded[i]);
                aatomSysLoaded[i] = 0;
                gdwSysExpungeMask &= ~(1 << i);
            }

            /*
             * Call back the client to free the library...
             */
            ClientFreeLibrary(hmodFree);
        }
    }
}
