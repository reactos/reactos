/****************************************************************************
 *
 *  disk32.c
 *
 *   routines do to queued, asynchrous disk I/O in Win32
 *   NOTE: these routines exist because Chicago does not yet
 *         support overlapped io.
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
//#include <win32.h>

#define _INC_MMDEBUG_CODE_ TRUE
#include "mmdebug.h"  // AuxDebug & assert macros
#include "disk32.h"

#define LockList(pHead)    EnterCriticalSection (&pHead->csList)
#define UnlockList(pHead)  LeaveCriticalSection (&pHead->csList)

/*+ QueueInitalize
 *
 * Initalize the queue.
 *
 *-========================================================================*/

BOOL WINAPI QueueInitialize
(
    PQHEAD  pHead
)
{
    InitializeCriticalSection (&pHead->csList);
    if ( ! pHead->hEvtElms)
        pHead->hEvtElms = CreateEvent (NULL, TRUE, FALSE, NULL);

    // a queue being initalized SHOULD be empty.  be sure of it.
    //
    assert (pHead->qe.pNext == NULL || pHead->qe.pNext == &pHead->qe);
    assert (pHead->qe.pPrev == NULL || pHead->qe.pNext == &pHead->qe);
    pHead->qe.pPrev = pHead->qe.pNext = &pHead->qe;
    return TRUE;
}

/*+ QueueDelete
 *
 * de-Initalize the queue
 *
 *-========================================================================*/

BOOL WINAPI QueueDelete
(
    PQHEAD  pHead
)
{
    DeleteCriticalSection (&pHead->csList);
    if ( ! pHead->hEvtElms)
    {
        SetEvent (pHead->hEvtElms);  // just in case.
        CloseHandle (pHead->hEvtElms);
        pHead->hEvtElms = NULL;
    }

    // a queue being Deleted SHOULD be empty.  be sure of it.
    //
    assert (pHead->qe.pNext == &pHead->qe);
    assert (pHead->qe.pPrev == &pHead->qe);

    pHead->qe.pPrev = pHead->qe.pNext = &pHead->qe;
    return TRUE;
}

/*+ QueueInsert
 *
 * insert an element in the queue. If a thread is waiting on the queue
 * it will be awakened.
 *
 *-========================================================================*/

VOID WINAPI QueueInsert
(
    PQHEAD pHead,
    PQELM  pqe
)
{
    PQELM  pqeHead = &pHead->qe;

    // this can only happen if the queue has never been initialized
    //
    assert (pqeHead->pNext != NULL);
    assert (pqeHead->pPrev != NULL);

    LockList (pHead);

    // insert the new element into the list at the tail.
    //
    pqe->pNext = pqeHead;
    pqe->pPrev = pqeHead->pPrev;
    pqe->pPrev->pNext = pqe;
    pqeHead->pPrev = pqe;

    // if the element we just inserted at the tail of the list
    // is also at the head of the list.  The list must have been
    // empty before.  In this case we want to signal the Event
    // to wake any threads waiting on the queue.
    //
    if ((pqeHead->pNext == pqe) && pHead->hEvtElms)
       SetEvent (pHead->hEvtElms);

    UnlockList (pHead);

    return;
}

/*+ QueueRemove
 *
 * remove an element from the queue.  if the queue is empty.
 * wait for an element to be inserted.  A timeout of 0 can be
 * used to POLL the queue.
 *
 *-========================================================================*/

PQELM  WINAPI QueueRemove
(
    PQHEAD pHead,
    DWORD  dwTimeout
)
{
    PQELM  pqe;

    LockList (pHead);

    // next & prev can only be null when a queue is un-initialized.
    //
    assert (pHead->qe.pNext != NULL);
    assert (pHead->qe.pPrev != NULL);

    // if the list is empty and the user specified a non-zero
    // timeout and we have a list semaphore available, wait
    // for the semaphore to be signalled.
    //
    pqe = pHead->qe.pNext;
    if ((pqe == &pHead->qe) && (dwTimeout != 0) && (pHead->hEvtElms != NULL))
    {
        // the queue is empty - so make sure that the event has
        // not been signalled.
        //
        ResetEvent (pHead->hEvtElms);

        // unlock the list before waiting so that we dont
        // deadlock the thread that is inserting things into
        // the list.
        //
        UnlockList (pHead);

        AuxDebugEx (3, DEBUGLINE "Waiting (%d) secs on queue %08x\r\n",
                    dwTimeout, pHead);
        WaitForSingleObject (pHead->hEvtElms, dwTimeout);

        LockList (pHead);
        pqe = pHead->qe.pNext;
    }

    // if the queue is still empty, set pqe to NULL so that we will
    // return null. otherwise remove the head of the queue and return
    // it.
    //
    if (pqe == &pHead->qe)
        pqe = NULL;
    else
    {
       // remove the element from the list.
       //
       pHead->qe.pNext = pqe->pNext;
       pqe->pNext->pPrev = pqe->pPrev;

       // just to be careful, blank out the
       //
       pqe->pPrev = pqe->pNext = NULL;

       // if the queue is now empty, reset the event
       //
       if ((pHead->qe.pNext == &pHead->qe) && pHead->hEvtElms)
           ResetEvent (pHead->hEvtElms);
    }

    UnlockList (pHead);

    return pqe;
}

#ifdef DEBUG

/*+ QueueDump
 *
 *-========================================================================*/

void WINAPI QueueDump
(
    PQHEAD pHead
)
{
    PQELM  pqe;
    UINT   nMax;

    LockList (pHead);

    AuxDebugEx (2, "Dumping Queue %08X\r\n", pHead);
    AuxDebugEx (2, "\telm %08x (next=%08x, prev=%08x)\r\n", &pHead->qe, pHead->qe);

    nMax = 10;
    pqe = pHead->qe.pNext;
    while (nMax && pqe != &pHead->qe)
    {
        pqe = pqe->pNext;
        --nMax;
        AuxDebugEx (2, "\telm %08x (next=%08x, prev=%08x)\r\n", pqe, *pqe);
    }

    UnlockList (pHead);
}
#endif // DEBUG

/*+ SequentialIOThreadProc
 *
 * thread proc dos sequential writes to a file from buffers queued
 * to it.
 *
 *-========================================================================*/

DWORD WINAPI AsyncIOThreadProc
(
    LPQIO lpqio
)
{
    // we loop forever.  exit from this loop is when
    // QueueRemove returns a qiobuf with cb == 0
    //
    for (;;)
    {
        PQIOBUF pqBuf;
        DWORD   dwOff;

        // get the next buffer to be written.  if we are running
        // down, then dont bother to wait for more buffers if
        // the queue is empty.
        //
        pqBuf = (LPVOID)QueueRemove (&lpqio->que, INFINITE);

        assert (!pqBuf || !IsBadWritePtr(pqBuf, sizeof(*pqBuf)));

        // if we got no buffer back from queue remove, this may be reasonable
        // in the case of two threads, just loop back and try again.
        //
        if ( ! pqBuf )
            continue;

        // break out of the loop when a -1 buffer pointer is queued
        //
        if ( pqBuf->lpv == (LPVOID)-1)
        {
            QueueInsert (&lpqio->queDone, (LPVOID)pqBuf);
            break;
        }

        AuxDebugEx (2, DEBUGLINE "tid %08X %s %X bytes at %08X into %08X\r\n",
                    lpqio->tid,
                    pqBuf->bWrite ? "Writing" : "Reading",
                    pqBuf->cb, pqBuf->dwOffset, pqBuf->lpv);

        assert3 (!pqBuf->cb || HIWORD(pqBuf->lpv), "QioThread - invalid buffer %08X", pqBuf->lpv);

        if ( HIWORD(pqBuf->lpv) && pqBuf->cb )
        {
            assert (!IsBadReadPtr(pqBuf->lpv, pqBuf->cb));

            dwOff = SetFilePointer (lpqio->hFile, pqBuf->dwOffset, NULL, FILE_BEGIN);
            if (dwOff != pqBuf->dwOffset)
            {
                pqBuf->dwError = GetLastError();
                AuxDebug2 ("avifile32 seek error %d", pqBuf->dwError);
            }
            else
            {
                if (pqBuf->bWrite)
                {
                    if ( ! WriteFile (lpqio->hFile, pqBuf->lpv, pqBuf->cb,
                                      &pqBuf->cbDone, NULL) ||
                           (pqBuf->cb != pqBuf->cbDone))
                    {
                        pqBuf->dwError = GetLastError();
                        AuxDebug2 ("avifile32 write error %d", pqBuf->dwError);
                    }
                    else
                        pqBuf->dwError = 0;
                }
                else
                {
                    if ( ! ReadFile (lpqio->hFile, pqBuf->lpv, pqBuf->cb,
                                      &pqBuf->cbDone, NULL) ||
                           (pqBuf->cb != pqBuf->cbDone))
                    {
                        pqBuf->dwError = GetLastError();
                        AuxDebug2 ("avifile32 read error %d", pqBuf->dwError);
                    }
                    else
                        pqBuf->dwError = 0;
                }
            }
        }

        // Once the write is done, but the buffer on the done queue
        //
        QueueInsert (&lpqio->queDone, (LPVOID)pqBuf);
    }

    return 0;
}

/*+ QioInitialize
 *
 * Open a file for queued sequential io.
 *
 *-========================================================================*/

BOOL WINAPI QioInitialize
(
    LPQIO  lpqio,
    HANDLE hFile,
    int    nPrio
)
{
    DebugSetOutputLevel (GetProfileInt("debug", "avifil32", 0));

    AuxDebugEx (1, DEBUGLINE "QioInitialize (%08x, %d)\r\n",
                lpqio, nPrio);

    nPrio = max(nPrio, THREAD_PRIORITY_IDLE);
    nPrio = min(nPrio, THREAD_PRIORITY_TIME_CRITICAL);

    QueueInitialize (&lpqio->que);
    QueueInitialize (&lpqio->queDone);

    assert ( ! lpqio->hThread);

    lpqio->hFile = hFile;
    lpqio->nPrio = nPrio;
    lpqio->hThread = CreateThread (NULL, 0,
                                   AsyncIOThreadProc,
                                   lpqio,
                                   0,
                                   &lpqio->tid);

    // if we fail creating the thread, cleanup and
    // return error.
    //
    if ( ! lpqio->hThread)
    {
        AuxDebugEx (1, DEBUGLINE "QioInitialize - CreateThread failed\r\n",
                    lpqio, nPrio);

        QueueDelete (&lpqio->que);
        QueueDelete (&lpqio->queDone);
        ZeroMemory (lpqio, sizeof(*lpqio));
        return FALSE;
    }

    SetThreadPriority (lpqio->hThread, lpqio->nPrio);

    return TRUE;
}

/*+ QioAdd
 *
 *
 *-========================================================================*/

BOOL WINAPI QioAdd
(
    LPQIO   lpqio,
    PQIOBUF pqBuf
)
{
    assert (lpqio);
    assert (pqBuf);

    assert (lpqio->que.qe.pNext != NULL);
    assert (lpqio->hThread);

    if (!lpqio->hThread)
        return FALSE;

    // the queue insert/remove code in this function make the
    // assumption that the queue pointers are the first element
    // of the pqBuf structure.
    //
    assert ((DWORD)&pqBuf->qe - (DWORD)pqBuf == 0);

    // not allowed to queue a buffer with no size or no pointer
    //
    assert (HIWORD(pqBuf->lpv));
    assert (pqBuf->cb);

    pqBuf->bPending = TRUE;
    QueueInsert (&lpqio->que, (LPVOID)pqBuf);
    return TRUE;
}

/*+ QioWait
 *
 *
 *-========================================================================*/

BOOL WINAPI QioWait
(
    LPQIO   lpqio,
    PQIOBUF pqBufWait,
    BOOL    bWait
)
{
    assert (lpqio);
    assert (pqBufWait);

    assert (lpqio->que.qe.pNext != NULL);
    assert (lpqio->hThread);

    // the queue insert/remove code in this function make the
    // assumption that the queue pointers are the first element
    // of the pqBuf structure.
    //
    assert ((DWORD)&pqBufWait->qe - (DWORD)pqBufWait == 0);

    if (pqBufWait->bPending)
    {
        PQIOBUF pqBufT;
        DWORD   dwTimeout = bWait ? INFINITE : 0;

        do
        {
           pqBufT = (LPVOID) QueueRemove (&lpqio->queDone, dwTimeout);
           AuxDebugEx (4, DEBUGLINE "QioWait(%08X) - removed %08X\r\n", lpqio, pqBufT);
           assert (!pqBufT || !IsBadWritePtr(pqBufT, sizeof(*pqBufT)));

           if (!pqBufT)
              return FALSE;

           pqBufT->bPending = FALSE;

        } while (pqBufT != pqBufWait);
    }

    return TRUE;
}

/*+ QioCommit
 *
 * Waits for the Qio thread to complete any i/o that is in it's queue.
 * then causes the qio thread to exit and waits for it to do so.
 *
 *-========================================================================*/

BOOL WINAPI QioCommit
(
    LPQIO lpqio
)
{
    QIOBUF qb;

    assert (lpqio);

    AuxDebugEx (2, DEBUGLINE "QioCommit (%08X)\r\n", lpqio);

    // queue up a zero size buffer as a placeholder
    // and then wait for the placeholder to be moved
    // to the done queue.
    //
    ZeroMemory (&qb, sizeof(qb));
    qb.bPending = TRUE;
    QueueInsert (&lpqio->que, (LPVOID)&qb);

    return QioWait (lpqio, &qb, TRUE);
}

/*+ QioShutdown
 *
 * Waits for the Qio thread to complete any i/o that is in it's queue.
 * then causes the qio thread to exit and waits for it to do so.
 *
 *-========================================================================*/

BOOL WINAPI QioShutdown
(
    LPQIO  lpqio
)
{
    QIOBUF qb;

    assert (lpqio);

    AuxDebugEx (1, DEBUGLINE "QioShutdown (%08X)\r\n", lpqio);

    if ( ! lpqio->hThread)
    {
        AuxDebugEx (1, DEBUGLINE "QioShutdown - nothing to do!\r\n");
        return TRUE;
    }

    assert (lpqio->hThread);

    // queue up a zero size buffer to tell the write thread to quit
    //
    ZeroMemory (&qb, sizeof(qb));
    qb.lpv = (LPVOID)-1;
    qb.bPending = TRUE;
    QueueInsert (&lpqio->que, (LPVOID)&qb);
    QioWait (lpqio, &qb, TRUE);

    // wait for the thread to shut down.
    //
    AuxDebugEx (1, DEBUGLINE "Waiting for QIO thread\r\n");
    WaitForSingleObject (lpqio->hThread, INFINITE);

    AuxDebugEx (1, DEBUGLINE "closeing thread handle\r\n");
    CloseHandle (lpqio->hThread), lpqio->hThread = NULL;

    //INLINE_BREAK;

    // finally, delete the queues
    //
    QueueDelete (&lpqio->que);
    QueueDelete (&lpqio->queDone);

    return TRUE;
}
