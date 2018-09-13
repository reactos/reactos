/****************************** Module Header ******************************\
* Module Name: job.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the code to implement the job object in NTUSER.
*
* History:
* 29-Jul-1997 CLupu   Created.
\***************************************************************************/

#include "precomp.h"

PW32JOB CreateW32Job(PEJOB Job);
VOID UpdateJob(PW32JOB pW32Job);
void SetProcessFlags(PW32JOB pW32Job, PPROCESSINFO ppi);
BOOL JobCalloutAddProcess(PW32JOB, PPROCESSINFO);
BOOL JobCalloutTerminate(PW32JOB);

/***************************************************************************\
* UserJobCallout
*
* History:
* 29-Jul-1997 CLupu   Created.
\***************************************************************************/
NTSTATUS UserJobCallout(
    PKWIN32_JOBCALLOUT_PARAMETERS Parm)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PW32JOB  pW32Job = NULL;
    PEJOB    Job;
    PSW32JOBCALLOUTTYPE CalloutType;
    PVOID    Data;


    Job = Parm->Job;
    CalloutType = Parm->CalloutType;
    Data = Parm->Data;

    /*
     * The EJOB lock must be acquired at this time.
     */
    UserAssert(ExIsResourceAcquiredExclusiveLite(&Job->JobLock));

    UserAssert(gpresUser != NULL);

    BEGIN_REENTERCRIT();

    BEGINATOMICCHECK();

    /*
     * find the W32JOB in the global list (if any)
     */
    pW32Job = gpJobsList;

    while (pW32Job) {
        if (pW32Job->Job == Job) {
            break;
        }
        pW32Job = pW32Job->pNext;
    }

    switch (CalloutType) {
    case PsW32JobCalloutSetInformation:

        if (pW32Job == NULL) {

            /*
             * The W32Job is not created yet. Assert that this is not
             * a call to remove UI restrictions
             */
            UserAssert(Data != 0);

            if ((pW32Job = CreateW32Job(Job)) == NULL) {
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        } else {

            /*
             * The W32Job structure is already created. Return if
             * the restrictions are the same as before.
             */
            if (PtrToUlong(Data) == pW32Job->restrictions) {
                TAGMSG0(DBGTAG_Job, "UserJobCallout: SetInformation same as before");
                break;
            }
        }

        /*
         * Set the restrictions
         */
        pW32Job->restrictions = PtrToUlong(Data);

        UpdateJob(pW32Job);
        break;

    case PsW32JobCalloutAddProcess:

        /*
         * 'Data' parameter is a pointer to W32PROCESS. So this callout
         * happens only for GUI processes.
         */
        UserAssert(Job->UIRestrictionsClass != 0);

        /*
         * Assert that the W32JOB structure is already created.
         */
        UserAssert(pW32Job != NULL);

        TAGMSG3(DBGTAG_Job, "UserJobCallout: AddProcess Job 0x%x W32Job 0x%x Process 0x%x",
                Job, pW32Job, (ULONG_PTR)Data);

        /*
         * this callout must be only for GUI processes
         */
        UserAssert(Data != NULL);

        JobCalloutAddProcess(pW32Job, (PPROCESSINFO)Data);

        break;

    case PsW32JobCalloutTerminate:

        TAGMSG2(DBGTAG_Job, "UserJobCallout: Terminate Job 0x%x W32Job 0x%x",
                Job, pW32Job);

        if (pW32Job) {
            JobCalloutTerminate(pW32Job);
        }
        break;

    default:
        TAGMSG2(DBGTAG_Job, "UserJobCallout: Invalid callout 0x%x Job 0x%x",
                CalloutType, Job);

        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    ENDATOMICCHECK();

    END_REENTERCRIT();

    return Status;
}

/***************************************************************************\
* CreateW32Job
*
* Creates a W32Job
*
* History:
* 18-Mar-1998 CLupu   Created.
\***************************************************************************/
PW32JOB CreateW32Job(
    PEJOB Job)
{
    PW32JOB pW32Job;

    TAGMSG1(DBGTAG_Job, "CreateW32Job: EJOB 0x%x", Job);

    pW32Job = UserAllocPoolZInit(sizeof(W32JOB), TAG_W32JOB);

    if (pW32Job == NULL) {
        RIPMSG0(RIP_ERROR, "CreateW32Job: memory allocation error");
        return NULL;
    }

    /*
     * Create the global atom table for this job
     */
    CreateGlobalAtomTable(&pW32Job->pAtomTable);

    if (pW32Job->pAtomTable == NULL) {
        RIPMSG1(RIP_ERROR, "CreateW32Job: fail to create the atom table for job 0x%x",
                pW32Job);

        UserFreePool(pW32Job);
        return NULL;
    }

    /*
     * Link it in the W32 job's list
     */
    pW32Job->pNext = gpJobsList;
    gpJobsList = pW32Job;

    pW32Job->Job = Job;

    TAGMSG2(DBGTAG_Job, "CreateW32Job: pW32Job 0x%x created for EJOB 0x%x",
            pW32Job, Job);

    return pW32Job;
}

/***************************************************************************\
* UpdateJob
*
* Walks the processinfo list in userk to update all the processes assigned
* to this job .
*
* History:
* 20-Mar-1998 CLupu   Created.
\***************************************************************************/
VOID UpdateJob(
    PW32JOB pW32Job)
{
    PPROCESSINFO ppi;

    UserAssert(ExIsResourceAcquiredExclusiveLite(&pW32Job->Job->JobLock));
    CheckCritIn();

    TAGMSG1(DBGTAG_Job, "UpdateJob: pW32Job 0x%x", pW32Job);

    /*
     * walk the GUI processes list to see if any new process got
     * assigned to the current job.
     */
    ppi = gppiList;

    while (ppi) {
        if (ppi->Process->Job == pW32Job->Job) {

            /*
             * the process is assigned to this job
             */
            if (ppi->pW32Job == NULL) {

                /*
                 * add the process to the W32 job
                 */
                JobCalloutAddProcess(pW32Job, ppi);
            } else {

                /*
                 * The process is already added to the job. Just
                 * update the restrictions.
                 */
                SetProcessFlags(pW32Job, ppi);
            }
        }
        ppi = ppi->ppiNextRunning;
    }
}

/***************************************************************************\
* RemoveProcessFromJob
*
* This is called during the delete process callout.
*
* History:
* 30-Jul-1997 CLupu   Created.
\***************************************************************************/
BOOL RemoveProcessFromJob(
    PPROCESSINFO ppi)
{
    PW32JOB pW32Job;
    UINT    ip;

    CheckCritIn();

    pW32Job = ppi->pW32Job;

    TAGMSG2(DBGTAG_Job, "RemoveProcessFromJob: ppi 0x%x pW32Job 0x%x",
            ppi, pW32Job);

    /*
     * The job might not have UI restrictions
     */
    if (pW32Job == NULL) {
        return FALSE;
    }

    /*
     * remove the ppi from the job's ppi table
     */
    for (ip = 0; ip < pW32Job->uProcessCount; ip++) {

        UserAssert(pW32Job->ppiTable[ip]->pW32Job == pW32Job);

        if (ppi == pW32Job->ppiTable[ip]) {

            ppi->pW32Job = NULL;

            RtlMoveMemory(pW32Job->ppiTable + ip,
                          pW32Job->ppiTable + ip + 1,
                          (pW32Job->uProcessCount - ip - 1) * sizeof(PPROCESSINFO));

            (pW32Job->uProcessCount)--;

            /*
             * free the process array if this is the last one.
             */
            if (pW32Job->uProcessCount == 0) {
                UserFreePool(pW32Job->ppiTable);
                pW32Job->ppiTable = NULL;
                pW32Job->uMaxProcesses = 0;
            }
            
            TAGMSG2(DBGTAG_Job, "RemoveProcessFromJob: ppi 0x%x removed from pW32Job 0x%x",
                    ppi, pW32Job);

            return TRUE;
        }
    }
    
    TAGMSG2(DBGTAG_Job, "RemoveProcessFromJob: ppi 0x%x not found in pW32Job 0x%x",
            ppi, pW32Job);

    UserAssert(0);

    return FALSE;
}

/***************************************************************************\
* SetProcessFlags
*
* History:
* 29-Jul-1997 CLupu   Created.
\***************************************************************************/
void SetProcessFlags(
    PW32JOB      pW32Job,
    PPROCESSINFO ppi)
{
    PTHREADINFO pti;

    CheckCritIn();

    TAGMSG3(DBGTAG_Job, "SetProcessFlags: pW32Job 0x%x ppi 0x%x restrictions 0x%x",
            pW32Job, ppi, pW32Job->restrictions);

    UserAssert(ppi->pW32Job == pW32Job);

    if (pW32Job->restrictions == 0) {
        ((PW32PROCESS)ppi)->W32PF_Flags &= ~W32PF_RESTRICTED;
    } else {
        ((PW32PROCESS)ppi)->W32PF_Flags |= W32PF_RESTRICTED;
    }

    KeAttachProcess(&ppi->Process->Pcb);

    /*
     * walk the pti list and set the restricted flag as appropriate
     */
    pti = ppi->ptiList;

    if (pW32Job->restrictions == 0) {
        while (pti) {
            pti->TIF_flags &= ~TIF_RESTRICTED;
            pti->pClientInfo->dwTIFlags &= ~TIF_RESTRICTED;
            pti = pti->ptiSibling;
        }
    } else {
        while (pti) {
            pti->TIF_flags |= TIF_RESTRICTED;
            pti->pClientInfo->dwTIFlags |= TIF_RESTRICTED;
            pti = pti->ptiSibling;
        }
    }

    KeDetachProcess();
}

/***************************************************************************\
* JobCalloutAddProcess
*
* History:
* 30-Jul-1997 CLupu   Created.
\***************************************************************************/
BOOL JobCalloutAddProcess(
    PW32JOB      pW32Job,
    PPROCESSINFO ppi)
{
    PPROCESSINFO* ppiTable;

    CheckCritIn();

    UserAssert(pW32Job != NULL);

    /*
     * This process is not yet initialized
     */
    if (ppi->Process == NULL) {
        return FALSE;
    }

    if (!(ppi->W32PF_Flags & W32PF_PROCESSCONNECTED)) {
        TAGMSG2(DBGTAG_Job, "JobCalloutAddProcess: pW32Job 0x%x ppi 0x%x not yet initialized",
                pW32Job, ppi);
        return FALSE;
    }

    TAGMSG2(DBGTAG_Job, "JobCalloutAddProcess: pW32Job 0x%x ppi 0x%x",
            pW32Job, ppi);

#if DBG
    /*
     * Make sure the process is not already in the job's process list
     */
    {
        UINT ip;
        for (ip = 0; ip < pW32Job->uProcessCount; ip++) {

            UserAssert(pW32Job->ppiTable[ip]->pW32Job == pW32Job);
            UserAssert(ppi != pW32Job->ppiTable[ip]);
        }
    }
#endif // DBG

    /*
     * save the pW32Job pointer in the process info
     */
    UserAssert(ppi->pW32Job == NULL);

    ppi->pW32Job = pW32Job;

    if (pW32Job->uProcessCount == pW32Job->uMaxProcesses) {

        /*
         * No more room. Allocate more space for the process table
         */
        if (pW32Job->uMaxProcesses == 0) {

            UserAssert(pW32Job->ppiTable == NULL);

            ppiTable = UserAllocPool(JP_DELTA * sizeof(PPROCESSINFO), TAG_W32JOBEXTRA);

        } else {
            UserAssert(pW32Job->ppiTable != NULL);

            ppiTable = UserReAllocPool(pW32Job->ppiTable,
                                       pW32Job->uMaxProcesses * sizeof(PPROCESSINFO),
                                       (pW32Job->uMaxProcesses + JP_DELTA) * sizeof(PPROCESSINFO),
                                       TAG_W32JOBEXTRA);
        }

        if (ppiTable == NULL) {
            RIPMSG0(RIP_ERROR, "JobCalloutAddProcess: memory allocation error\n");
            return FALSE;
        }

        pW32Job->ppiTable = ppiTable;
        pW32Job->uMaxProcesses += JP_DELTA;
    }

    /*
     * now add the process to the job
     */
    pW32Job->ppiTable[pW32Job->uProcessCount] = ppi;
    (pW32Job->uProcessCount)++;

    SetProcessFlags(pW32Job, ppi);

    return TRUE;
}

/***************************************************************************\
* JobCalloutTerminate
*
* This is called during the job object delete routine.
*
* History:
* 30-Jul-1997 CLupu   Created.
\***************************************************************************/
BOOL JobCalloutTerminate(
    PW32JOB pW32Job)
{
    CheckCritIn();

    UserAssert(pW32Job != NULL);

    TAGMSG1(DBGTAG_Job, "JobCalloutTerminate: pW32Job 0x%x", pW32Job);

    /*
     * No processes should be attached to this job
     */
    UserAssert(pW32Job->ppiTable == NULL);
    UserAssert(pW32Job->uProcessCount == 0);
    UserAssert(pW32Job->uMaxProcesses == 0);

    if (pW32Job->pgh) {
        UserAssert(pW32Job->ughCrt > 0);
        UserAssert(pW32Job->ughMax > 0);

        UserFreePool(pW32Job->pgh);
        pW32Job->pgh    = NULL;
        pW32Job->ughCrt = 0;
        pW32Job->ughMax = 0;
    }

    /*
     * remove the W32 job from the job's list
     */
    REMOVE_FROM_LIST(W32JOB, gpJobsList, pW32Job, pNext);

    RtlDestroyAtomTable(pW32Job->pAtomTable);

    UserFreePool(pW32Job);

    return TRUE;
}
