// wsnmp_ma.c
//
// WinSNMP Initialization Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 970310 - Free table memory on PROCESS_DETACH
//        - Refine snmpAllocTable() code
// 970417 - GetVersionEx added to check for
//        - NT vs 95 and adjust code accordingly
//
#include "winsnmp.h"
#include "winsnmpn.h"
// Memory descriptors
SNMPTD   SessDescr;
SNMPTD   PDUsDescr;
SNMPTD   VBLsDescr;
SNMPTD   EntsDescr;
SNMPTD   CntxDescr;
SNMPTD   MsgDescr;
SNMPTD   TrapDescr;
SNMPTD   AgentDescr;

TASK     TaskData;

CRITICAL_SECTION cs_TASK;
CRITICAL_SECTION cs_SESSION;
CRITICAL_SECTION cs_PDU;
CRITICAL_SECTION cs_VBL;
CRITICAL_SECTION cs_ENTITY;
CRITICAL_SECTION cs_CONTEXT;
CRITICAL_SECTION cs_MSG;
CRITICAL_SECTION cs_TRAP;
CRITICAL_SECTION cs_AGENT;
CRITICAL_SECTION cs_XMODE;

//-----------------------------------------------------------------
// snmpAllocTable - This function is used to initialize and to increase
// the size of WinSNMP internal tables. The caller must always ensure
// that this function is executed only within a critical section block
// on the target table's CRITICAL_SECTION object.
//-----------------------------------------------------------------
SNMPAPI_STATUS snmpAllocTable (LPSNMPTD pTableDescr)

{
    LPVOID ptr;
    DWORD nLen;
    SNMPAPI_STATUS lResult = SNMPAPI_FAILURE;

    LPSNMPBD pBufDescr;

    // allocate a buffer large enough for the SNMPBD header plus the space
    // needed to hold 'BlockToAdd' blocks of size 'BlockSize' each.
    // the memory is already zero-ed because of the GPTR flag.
    pBufDescr = GlobalAlloc(GPTR, sizeof(SNMPBD) + (pTableDescr->BlockSize * pTableDescr->BlocksToAdd));
    if (pBufDescr == NULL)
        return SNMPAPI_FAILURE;

    // see if other buffers are present in the table
    if (pTableDescr->Allocated == 0)
    {
        // no blocks previously allocated => pTableDescr->Buffer = NULL at this point
        // pNewBufDescr is the first buffer in the table.
        pBufDescr->next = pBufDescr->prev = pBufDescr;
        pTableDescr->HeadBuffer = pBufDescr;
    }
    else
    {
        // there is at least one other block into the table, so insert the
        // new buffer into the circular list, just before the head of the list
        pBufDescr->next = pTableDescr->HeadBuffer;
        pBufDescr->prev = pTableDescr->HeadBuffer->prev;
        pBufDescr->next->prev = pBufDescr;
        pBufDescr->prev->next = pBufDescr;
    }

    // increase 'Allocated' with the additional 'BlocksToAdd' newly allocated entries.
    pTableDescr->Allocated += pTableDescr->BlocksToAdd;
    
    return SNMPAPI_SUCCESS;
}

//-----------------------------------------------------------------
// snmpInitTableDescr - initializes the table descriptor with the 
// parameters given as arguments. Creates a first chunck of table.
//-----------------------------------------------------------------
SNMPAPI_STATUS snmpInitTableDescr(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwBlocksToAdd, /*in*/DWORD dwBlockSize)
{
	ZeroMemory (pTableDescr, sizeof(SNMPTD));
	pTableDescr->BlocksToAdd = dwBlocksToAdd;
	pTableDescr->BlockSize = dwBlockSize;

	return snmpAllocTable (pTableDescr);
}

//-----------------------------------------------------------------
// snmpFreeTableDescr - releases any memory allocated for the table.
//-----------------------------------------------------------------
VOID snmpFreeTableDescr(/*in*/LPSNMPTD pTableDescr)
{
    // do nothing if the table does not contain any entries
	if (pTableDescr->HeadBuffer == NULL)
        return;

    // break the circular list by setting the 'next' of
    // the buffer before the head to NULL
    pTableDescr->HeadBuffer->prev->next = NULL;

    while (pTableDescr->HeadBuffer != NULL)
    {
        LPSNMPBD pBufDescr;

        pBufDescr = pTableDescr->HeadBuffer;
        pTableDescr->HeadBuffer = pBufDescr->next;
        GlobalFree(pBufDescr);
    }
}

//-----------------------------------------------------------------
// snmpAllocTableEntry - finds an empty slot in the table described
// by pTableDescr, and returns its index. If none could be 
// found, table is extended in order to get some new empty slots.
// It is not an API call so it doesn't check its parameters.
//-----------------------------------------------------------------
SNMPAPI_STATUS snmpAllocTableEntry(/*in*/LPSNMPTD pTableDescr, /*out*/LPDWORD pdwIndex)
{
     // check if there are any empty entries into the table ..
    if (pTableDescr->Allocated == pTableDescr->Used)
    {
        // .. if not, enlarge the table ..
        if (!snmpAllocTable (pTableDescr))
            return SNMPAPI_ALLOC_ERROR;
        // .. and return the first empty slot
        *pdwIndex = pTableDescr->Used;

        // don't forget to update the 'Used' fields. The first one markes a new entry in use
        // in the buffer, the second one marks a new entry in use in the table as a whole
        (pTableDescr->HeadBuffer->prev->Used)++;
        pTableDescr->Used++;
    }
    else
    {
        DWORD dwBufferIndex, dwInBufferIndex;
        LPSNMPBD pBufDescr;
        LPBYTE pTblEntry; // cursor on the entries in the table

        // scan the list of buffers searching for the buffer that
        // holds at least one available entry.
        for (pBufDescr = pTableDescr->HeadBuffer, dwBufferIndex=0;
             pBufDescr->Used >= pTableDescr->BlocksToAdd;
             pBufDescr = pBufDescr->next, dwBufferIndex++)
        {
             // just a precaution: make sure we are not looping infinitely here
             // this shouldn't happen as far as 'Allocated' and 'Used' say there
             // are available entries, hence at least a buffer should match
             if (pBufDescr->next == pTableDescr->HeadBuffer)
                 return SNMPAPI_OTHER_ERROR;
        }

        // now that we have the buffer with available entries,
        // search in it for the first one available.
        for ( pTblEntry = (LPBYTE)pBufDescr + sizeof(SNMPBD), dwInBufferIndex = 0;
              dwInBufferIndex < pTableDescr->BlocksToAdd;
              dwInBufferIndex++, pTblEntry += pTableDescr->BlockSize)
        {
              // an empty slot into the table has the first field = (HSNMP_SESSION)0
              if (*(HSNMP_SESSION *)pTblEntry == 0)
                  break;
        }

        // make sure the buffer is not corrupted (it is so if 'Used' shows at
        // least an entry being available, but none seems to be so)
        if (dwInBufferIndex == pTableDescr->BlocksToAdd)
            return SNMPAPI_OTHER_ERROR;

        // don't forget to update the 'Used' fields. The first one markes a new entry in use
        // in the buffer, the second one marks a new entry in use in the table as a whole
        pBufDescr->Used++;
        pTableDescr->Used++;

        // we have the index of the buffer that contains the available entry
        // and the index of that entry inside the buffer. So just compute
        // the overall index and get out.
        (*pdwIndex) = dwBufferIndex * pTableDescr->BlocksToAdd + dwInBufferIndex;
    }

    return SNMPAPI_SUCCESS;
}

//-----------------------------------------------------------------
// snmpFreeTableEntry - releases the entry at index dwIndex from the
// table described by pTableDescr. It checks the validity of the index
// and returns SNMPAPI_INDEX_INVALID if it is not in the range of the
// allocated entries. It does not actually frees the memory, it cleares
// it up and adjusts internal counters.
//-----------------------------------------------------------------
SNMPAPI_STATUS snmpFreeTableEntry(/*in*/LPSNMPTD pTableDescr, /*out*/DWORD dwIndex)
{
    LPSNMPBD pBufDescr;
    LPBYTE pTableEntry;

    if (dwIndex >= pTableDescr->Allocated)
        return SNMPAPI_INDEX_INVALID;

    // scan for the buffer that holds the entry at index dwIndex
    for (pBufDescr = pTableDescr->HeadBuffer;
         dwIndex >= pTableDescr->BlocksToAdd;
         pBufDescr = pBufDescr->next, dwIndex -= pTableDescr->BlocksToAdd);

    // we have the buffer, get the actual pointer to the entry
    pTableEntry = (LPBYTE)pBufDescr + sizeof(SNMPBD);
    pTableEntry += dwIndex * pTableDescr->BlockSize;

    // zero the entry - having the first HSNMP_SESSION field set to 0
    // makes this entry available for further allocations
    ZeroMemory (pTableEntry, pTableDescr->BlockSize);

    // update the 'Used' fields to show that one entry less is in use
    if (pBufDescr->Used > 0)
        (pBufDescr->Used)--;
    if (pTableDescr->Used > 0)
        (pTableDescr->Used)--;

    return SNMPAPI_SUCCESS;
}

//-----------------------------------------------------------------
// snmpGetTableEntry - takes as arguments a table description (pTableDescr)
// and the zero based index (dwIndex) of the entry requested from the table
// and returns in pointer to the entry requested.
//-----------------------------------------------------------------
PVOID snmpGetTableEntry(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwIndex)
{
    LPSNMPBD pBufDescr;
    LPBYTE pTableEntry;

    // bugbug - we make the assumption the index is correct
    // scan for the buffer that holds the entry at index dwIndex
    for (pBufDescr = pTableDescr->HeadBuffer;
         dwIndex >= pTableDescr->BlocksToAdd;
         pBufDescr = pBufDescr->next, dwIndex -= pTableDescr->BlocksToAdd);

    // we have the buffer, get the actual pointer to the entry
    pTableEntry = (LPBYTE)pBufDescr + sizeof(SNMPBD);
    pTableEntry += dwIndex * pTableDescr->BlockSize;

    // this is it, pTableEntry can be returned to the caller
    return pTableEntry;
}

//-----------------------------------------------------------------
// snmpValidTableEntry - returns TRUE or FALSE as the entry at zero
// based index dwIndex from the table described by pTableDescr has
// valid data (is allocated) or not
//-----------------------------------------------------------------
BOOL snmpValidTableEntry(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwIndex)
{
    return (dwIndex < pTableDescr->Allocated) &&
           (*(HSNMP_SESSION *)snmpGetTableEntry(pTableDescr, dwIndex) != 0);
}

// Save error value as session/task/global error and return 0
SNMPAPI_STATUS SaveError(HSNMP_SESSION hSession, SNMPAPI_STATUS nError)
{
	TaskData.nLastError = nError;
	if (hSession)
	{
		LPSESSION pSession = snmpGetTableEntry(&SessDescr, HandleToUlong(hSession)-1);
		pSession->nLastError = nError;
	}
	return (SNMPAPI_FAILURE);
}

SNMPAPI_STATUS CheckRange (DWORD index, LPSNMPTD block)
{
if ((!index) || (index > block->Allocated))
   return (SNMPAPI_FAILURE);
else
   return (SNMPAPI_SUCCESS);
}

int snmpInit (void)
{
// Initialize Tables
if (snmpInitTableDescr(&SessDescr,  DEFSESSIONS, sizeof(SESSION)) != SNMPAPI_SUCCESS ||
	snmpInitTableDescr(&PDUsDescr,  DEFPDUS, sizeof(PDUS)) != SNMPAPI_SUCCESS        ||
	snmpInitTableDescr(&VBLsDescr,  DEFVBLS, sizeof(VBLS)) != SNMPAPI_SUCCESS        ||
    snmpInitTableDescr(&EntsDescr,  DEFENTITIES, sizeof(ENTITY)) != SNMPAPI_SUCCESS  ||
    snmpInitTableDescr(&CntxDescr,  DEFCONTEXTS, sizeof(CTXT)) != SNMPAPI_SUCCESS    ||
    snmpInitTableDescr(&MsgDescr,   DEFMSGS, sizeof(SNMPMSG)) != SNMPAPI_SUCCESS     ||
    snmpInitTableDescr(&TrapDescr,  DEFTRAPS, sizeof(TRAPNOTICE)) != SNMPAPI_SUCCESS  ||
    snmpInitTableDescr(&AgentDescr, DEFAGENTS, sizeof(AGENT)) != SNMPAPI_SUCCESS)
    return (SNMPAPI_FAILURE);
//
return (SNMPAPI_SUCCESS);
} // end_snmpInit()

void snmpFree (void)
{
snmpFreeTableDescr(&SessDescr);
snmpFreeTableDescr(&PDUsDescr);
snmpFreeTableDescr(&VBLsDescr);
snmpFreeTableDescr(&EntsDescr);
snmpFreeTableDescr(&CntxDescr);
snmpFreeTableDescr(&MsgDescr);
snmpFreeTableDescr(&TrapDescr);
snmpFreeTableDescr(&AgentDescr);
} // end_snmpFree()

BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
    BOOL errCode = FALSE;
    LPCRITICAL_SECTION pCSArray[10]; // ten critical sections to initialize (cs_TASK..cs_XMODE)
    INT nCS;                         // counter in pCSArray

    pCSArray[0] = &cs_TASK;
    pCSArray[1] = &cs_SESSION;
    pCSArray[2] = &cs_PDU;
    pCSArray[3] = &cs_VBL;
    pCSArray[4] = &cs_ENTITY;
    pCSArray[5] = &cs_CONTEXT;
    pCSArray[6] = &cs_MSG;
    pCSArray[7] = &cs_TRAP;
    pCSArray[8] = &cs_AGENT;
    pCSArray[9] = &cs_XMODE;

    switch (dwReason)
    {
       case DLL_PROCESS_ATTACH:
           // Init task-specific data area
           ZeroMemory (&TaskData, sizeof(TASK));
           // Build tables
           __try
           {
               for (nCS = 0; nCS < 10; nCS++)
                    InitializeCriticalSection (pCSArray[nCS]);
           }
           __except(EXCEPTION_EXECUTE_HANDLER)
           {
               // if an exception was raised, rollback the successfully initialized CS
               while (nCS > 0)
                   DeleteCriticalSection(pCSArray[--nCS]);
               break;
           }

           if (snmpInit() == SNMPAPI_SUCCESS)
               errCode = TRUE;
           break;

       case DLL_THREAD_ATTACH:
           // A new thread is being created in the current process.
           break;

       case DLL_THREAD_DETACH:
           // A thread is exiting cleanly.
           break;

       case DLL_PROCESS_DETACH:
           // The calling process is detaching the DLL from its address space.
           for (nCS = 0; nCS < 10; nCS++)
               DeleteCriticalSection(pCSArray[nCS]);

           snmpFree();
           errCode = TRUE;
           break;

       default:
           break;
    }
    return (errCode);
}
