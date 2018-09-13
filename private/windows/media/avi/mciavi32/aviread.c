/*************************************************************************
 * Copyright (C) Microsoft Corporation 1992. All rights reserved.
 *
 *************************************************************************/

/*
 * aviread.c: read blocks from the avi file (using worker thread).
 *	      Only built in WIN32 case.
 */

//#define AVIREAD
#ifdef AVIREAD

#include <windows.h>
#include <mmsystem.h>
#include <ntavi.h> 	// This must be included, for both versions

#include <string.h>     // needed for memmove in nt
#include <mmddk.h>
#include <memory.h>
#include "common.h"
#include "ntaviprt.h"
#include "aviread.h"
#include "aviffmt.h"
#include "graphic.h"


/*
 * overview of operation:
 *
 * creation of a avird object (via avird_startread) creates an avird_header
 * data structure and a worker thread. The data structure is protected by
 * a critical section, and contains two semaphores. the semEmpty semaphore is
 * initialised to the number of buffers allocated (normally 2), and the
 * semFull semaphore is initialised to 0 (there are initially no full buffers).
 *
 * The worker thread then loops waiting on semEmpty to get empty buffers,
 * and once it finds them, filling them and signalling via semFull that they
 * are ready. It fills them by callbacks to a AVIRD_FUNC function that
 * we were given a pointer to on object creation.
 *
 * Getting a buffer via avird_getnextbuffer waits on semFull until there
 * are full buffers, and returns the first on the list, after moving
 * it to the 'in use' list. The caller
 * will use/play the data in the buffer, and then call avird_emptybuffer:
 *  - this finds the buffer on the 'in use' list, moves it to the
 * 'empty' list and then signals the worker thread via
 * semEmpty.
 *
 * The worker thread checks the object state each time it is woken up. If this
 * state is 'closing' (bOK == FALSE), the thread frees all memory and exits.
 * avird_endread changes the object state and signals semEmpty to wake up the
 * worker thread.
 */


/*
 * each buffer is represented by one of these headers
 */
typedef struct avird_buffer {

    /*
     * size of the buffer in bytes
     */
    long	lSize;

    /* size of read data in bytes */
    long	lDataSize;


    /*
     * pointer to the next buffer in this state
     */
    struct avird_buffer * pNextBuffer;

    /* FALSE if buffer read failed */
    BOOL bOK;

    /* request sequence */
    int nSeq;

    /*
     * pointer to the actual block of buffer data
     */
    PBYTE	pData;
} AVIRD_BUFFER, * PAVIRD_BUFFER;



/* handles to HAVIRD are pointers to this data structure, but the
 * contents of the struct are known only within this module
 */
typedef struct avird_header {
    /*
     * always hold the critical section before checking/changing the
     * object state or any buffer state
     */
    CRITICAL_SECTION	critsec;

    /*
     * the count of this semaphore is the count of empty buffers
     * waiting to be picked up by the worker thread
     */
    HANDLE		semEmpty;

    /*
     * the count of this semaphore is the count of full buffers waiting
     * to be picked up by the caller.
     */
    HANDLE		semFull;

    /* object state - FALSE indicates close-down request. */
    BOOL		bOK;


    /* pointer to list of buffer headers ready to be filled */
    PAVIRD_BUFFER	pEmpty;

    /* pointer to a list of buffer headers in use by the client */
    PAVIRD_BUFFER	pInUse;

    /*
     * pointer to an ordered list of buffer headers ready to be
     * picked up by the client.
     */
    PAVIRD_BUFFER	pFull;

    /*
     * function to call to fill a buffer
     */
    AVIRD_FUNC		pFunc;

    /* instance arg to pass to pFunc() */
    DWORD		dwInstanceData;

    /* size of next buffer to be read */
    long		lNextSize;

    /* request sequence */
    int nNext;
    /* total in sequence */
    int nBlocks;


} AVIRD_HEADER, * PAVIRD_HEADER;


/* number of buffers to queue up */
#define MAX_Q_BUFS	4


/*
 * worker thread function
 */
DWORD avird_worker(LPVOID lpvThreadData);


/*
 * function to delete whole AVIRD_HEADER data structure.
 */
void avird_freeall(PAVIRD_HEADER phdr);

/*
 * start an avird operation and return a handle to use in subsequent
 * calls. This will cause an asynchronous read (achieved using a separate
 * thread) to start reading the next few buffers
 */
HAVIRD
avird_startread(AVIRD_FUNC func, DWORD dwInstanceData, long lFirstSize,
		int nFirst, int nBlocks)
{
    PAVIRD_HEADER phdr;
    PAVIRD_BUFFER pbuf;
    int i;
    HANDLE hThread;
    DWORD dwThreadId;
    int nBufferSize;

    /*
     * allocate and init the header
     */
    phdr = (PAVIRD_HEADER) LocalLock(LocalAlloc(LHND, sizeof(AVIRD_HEADER)));

    if (phdr == NULL) {
	return(NULL);
    }

    InitializeCriticalSection(&phdr->critsec);
    phdr->semEmpty = CreateSemaphore(NULL, MAX_Q_BUFS, MAX_Q_BUFS, NULL);
    phdr->semFull = CreateSemaphore(NULL, 0, MAX_Q_BUFS, NULL);
    phdr->bOK = TRUE;

    phdr->pInUse = NULL;
    phdr->pFull = NULL;
    phdr->pEmpty = NULL;

    phdr->pFunc = func;
    phdr->dwInstanceData = dwInstanceData;
    phdr->lNextSize = lFirstSize;
    phdr->nNext = nFirst;
    phdr->nBlocks = nBlocks;

    /*
     * round sizes up to 2k to reduce cost of small increases
     */
    nBufferSize = (lFirstSize + 2047) & ~2047;
    /*
     * allocate and init the buffers
     */
    for (i = 0; i < MAX_Q_BUFS; i++) {
	pbuf = (PAVIRD_BUFFER) LocalLock(LocalAlloc(LHND, sizeof(AVIRD_BUFFER)));

	pbuf->lSize = nBufferSize;
	pbuf->pData = (PBYTE) LocalLock(LocalAlloc(LHND, pbuf->lSize));

	pbuf->pNextBuffer = phdr->pEmpty;
	phdr->pEmpty = pbuf;
    }

    /*
     * create the worker thread
     */
    hThread = CreateThread(NULL, 0, avird_worker, (LPVOID)phdr, 0, &dwThreadId);
    if (hThread) {
	/* thread was created ok */
	CloseHandle(hThread);
	return( phdr);
    } else {
	avird_freeall(phdr);
	return(NULL);
    }
}

/*
 * return the next buffer from an HAVIRD object.
 */
PBYTE
avird_getnextbuffer(HAVIRD havird, long * plSize)
{
    PAVIRD_HEADER phdr = havird;
    PAVIRD_BUFFER pbuf;



    /* wait for a full buffer -report if actual wait needed*/
    if (WaitForSingleObject(phdr->semFull, 0) == WAIT_TIMEOUT) {
	DPF(("..waiting.."));
	WaitForSingleObject(phdr->semFull, INFINITE);
    }


    /* always hold critsec before messing with queues */
    EnterCriticalSection(&phdr->critsec);

    /* de-queue first full buffer and place on InUse queue */
    pbuf = phdr->pFull;
    phdr->pFull = pbuf->pNextBuffer;
    pbuf->pNextBuffer  = phdr->pInUse;
    phdr->pInUse = pbuf;

    /* finished with critical section */
    LeaveCriticalSection(&phdr->critsec);

    if (!pbuf->bOK) {
	/* buffer read failed */
	DPF(("reporting read failure on %d\n", pbuf->nSeq));
	if (plSize) {
	    *plSize = 0;
	}
    	return(NULL);
    }

    /* return size of buffer if requested */
    if (plSize) {
	*plSize = pbuf->lDataSize;
    }


    return(pbuf->pData);
}




/*
 * return to the queue a buffer that has been finished with (is now empty)
 *
 * causes the worker thread to be woken up and to start filling the buffer
 * again.
 */
void
avird_emptybuffer(HAVIRD havird, PBYTE pBuffer)
{
    PAVIRD_HEADER phdr = havird;
    PAVIRD_BUFFER pbuf, pprev;


    /* always get the critsec before messing with queues */
    EnterCriticalSection(&phdr->critsec);

    pprev = NULL;
    for (pbuf = phdr->pInUse; pbuf != NULL; pbuf = pbuf->pNextBuffer) {

	if (pbuf->pData == pBuffer) {

	    /* this is the buffer */
	    break;
	}
	pprev = pbuf;
    }

    if (pbuf != NULL) {
	 /* de-queue from InUse and place on empty q */
	if (pprev) {
	    pprev->pNextBuffer = pbuf->pNextBuffer;
	} else {
	    phdr->pInUse = pbuf->pNextBuffer;
	}
	pbuf->pNextBuffer = phdr->pEmpty;
	phdr->pEmpty = pbuf;

	/* mark as not validly read */
	pbuf->bOK = FALSE;

	/* signal that there is another buffer to fill */
    	ReleaseSemaphore(phdr->semEmpty, 1, NULL);
    } else {
	DPF(("buffer 0x%x not found on InUse list\n", pBuffer));
    }

    LeaveCriticalSection(&phdr->critsec);

}


/*
 * delete an avird object. the worker thread will be stopped and all
 * data allocated will be freed. The HAVIRD handle is no longer valid after
 * this call.
 */
void
avird_endread(HAVIRD havird)
{
    PAVIRD_HEADER phdr = havird;

    DPF(("killing an avird object\n"));

    /* get the critsec before messing with states */
    EnterCriticalSection(&phdr->critsec);

    /* tell the worker thread to do all the work */
    phdr->bOK = FALSE;

    /* wake up the worker thread */
    ReleaseSemaphore(phdr->semEmpty, 1, NULL);

    /*
     * we must hold the critsec past the semaphore signal: if we
     * release the critsec first, the worker thread might see the
     * state change before we have signalled the semaphore. He would
     * then potentially have destroyed the semaphore AND freed the
     * AVIRD_HEADER structure by the time we tried to signal the
     * semaphore. This way, we are sure that until we release the
     * critsec, everything is still valid
     */

    LeaveCriticalSection(&phdr->critsec);

    /* all done - phdr now may not exist */
}

/*
 * worker thread function.
 *
 * loop waiting for semEmpty to tell us there are empty buffers. When
 * we see one, fill it with phdr->pFunc and move it to the
 * full queue. Each time we are woken up, check the state. If it
 * changes to false, delete the whole thing and exit.
 *
 * the argument we are passed is the PAVIRD_HEADER.
 */

DWORD
avird_worker(LPVOID lpvThreadData)
{
    PAVIRD_HEADER phdr = (PAVIRD_HEADER) lpvThreadData;
    PAVIRD_BUFFER pbuf, pprev;
    long lNextSize;
    HANDLE hmem;

    DPF(("Worker %d started\n", GetCurrentThreadId()));

    for (; ;) {

	/* wait for an empty buffer (or state change) */
	WaitForSingleObject(phdr->semEmpty, INFINITE);


	/* get the critical section before touching the state, queues */
	EnterCriticalSection(&phdr->critsec);

	if (phdr->bOK == FALSE) {
	    /* all over bar the shouting */
	    DPF(("%d exiting\n", GetCurrentThreadId()));
	    avird_freeall(phdr);
	    ExitThread(0);
	}


	/* dequeue the first empty buffer */
	pbuf = phdr->pEmpty;

	Assert(pbuf != NULL);

	phdr->pEmpty = pbuf->pNextBuffer;

	lNextSize = phdr->lNextSize;

	pbuf->nSeq = phdr->nNext++;

	if (pbuf->nSeq < phdr->nBlocks) {
	    /* we can now release the critsec until we need to re-Q the filled buf*/
	    LeaveCriticalSection(&phdr->critsec);

	    /* resize the buffer if not big enough */
	    if (pbuf->lSize < lNextSize) {

		hmem = LocalHandle(pbuf->pData);
		LocalUnlock(hmem);
		LocalFree(hmem);

		pbuf->lSize = ((lNextSize + 2047) & ~2047);
		pbuf->pData = LocalLock(LocalAlloc(LHND, pbuf->lSize));
	    }

	    /* record the data content of the buffer */
	    pbuf->lDataSize = lNextSize;
	
	    /* call the filler function */
	    if ((*phdr->pFunc)(pbuf->pData, phdr->dwInstanceData, lNextSize,
			    &lNextSize)) {
		pbuf->bOK = TRUE;
	    } else {
		DPF(("filler reported failure on %d\n", pbuf->nSeq));
	    }

	    /* get the critsec before messing with q's or states */
	    EnterCriticalSection(&phdr->critsec);

	    /* size for next read */
	    phdr->lNextSize = lNextSize;
    	}

	/* place buffer at end of Full queue */
	if (phdr->pFull == NULL) {
	    phdr->pFull = pbuf;
	} else {
	    for (pprev = phdr->pFull; pprev->pNextBuffer != NULL; ) {
		pprev = pprev->pNextBuffer;
	    }
	    pprev->pNextBuffer = pbuf;
	}
	pbuf->pNextBuffer = NULL;

	LeaveCriticalSection(&phdr->critsec);

	/* signal calling thread that there's another buffer for him  */
	ReleaseSemaphore(phdr->semFull, 1, NULL);
    }
    /* silence compiler */
    return (0);
}

/*
 * free one buffer and buffer header
 */
void
avird_freebuffer(PAVIRD_BUFFER pbuf)
{
    HANDLE hmem;

    hmem = LocalHandle( (PSTR)pbuf->pData);
    LocalUnlock(hmem);
    LocalFree(hmem);

    hmem = LocalHandle( (PSTR)pbuf);
    LocalUnlock(hmem);
    LocalFree(hmem);
}


/*
 * function to delete whole AVIRD_HEADER data structure.
 *
 * called on calling thread if start-up fails, or on worker thread if
 * asked to shutdown.
 */
void
avird_freeall(PAVIRD_HEADER phdr)
{
    PAVIRD_BUFFER pbuf, pnext;
    HANDLE hmem;

    if (phdr->semEmpty) {
	CloseHandle(phdr->semEmpty);
    }

    if (phdr->semEmpty) {
	CloseHandle(phdr->semFull);
    }

    DeleteCriticalSection(&phdr->critsec);


    for (pbuf = phdr->pInUse; pbuf != NULL; pbuf = pnext) {
	DPF(("In Use buffers at EndRead\n"));

	pnext = pbuf->pNextBuffer;
	avird_freebuffer(pbuf);
    }

    for (pbuf = phdr->pEmpty; pbuf != NULL; pbuf = pnext) {
	pnext = pbuf->pNextBuffer;
	avird_freebuffer(pbuf);
    }

    for (pbuf = phdr->pFull; pbuf != NULL; pbuf = pnext) {
	pnext = pbuf->pNextBuffer;
	avird_freebuffer(pbuf);
    }


    hmem = LocalHandle((PSTR) phdr);
    LocalUnlock(hmem);
    LocalFree(hmem);
}

#endif //AVIREAD




