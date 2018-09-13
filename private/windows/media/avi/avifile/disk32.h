/****************************************************************************
 *
 *   disk32.h
 *
 *   routines to do async disk writes in Win32 Chicago
 *
 ***************************************************************************/

#if !defined DISK32_H
#define DISK32_H

#ifdef __cplusplus
extern "C" {            // Assume C declarations for C++
#endif  // __cplusplus

// include this struct as a field in any code that is to be enqueued.
//
typedef struct _qelm * PQELM;
typedef struct _qelm {
    PQELM pNext;
    PQELM pPrev;
    } QELM;

// use this for the head/tail of a queue.
//
typedef struct _qhead * PQHEAD;
typedef struct _qhead {
    HANDLE           hEvtElms;     // optional Semaphore that has element count
    CRITICAL_SECTION csList;       // list synchronization lock
    QELM             qe;           // head/tail pointers
    } QHEAD;

// Initalize the queue.
//
BOOL WINAPI QueueInitialize (
    PQHEAD  pHead
    );

// de-Initalize the queue
//
BOOL WINAPI QueueDelete (
    PQHEAD  pHead
    );

// insert an element in the queue. If a thread is waiting on the queue
// it will be awakened.
//
VOID WINAPI QueueInsert (
    PQHEAD pHead,
    PQELM  pqe
    );

// remove an element from the queue.  if the queue is empty.
// wait for an element to be inserted.  A timeout of 0 can be
// used to POLL the queue.
//
PQELM  WINAPI QueueRemove (
    PQHEAD pHead,
    DWORD  dwTimeout
    );

#ifdef DEBUG
void WINAPI QueueDump (
    PQHEAD pHead
    );
#else
 #define QueueDump(a)
#endif

typedef struct _qiobuf * PQIOBUF;
typedef struct _qiobuf {
    QELM   qe;         // queue pointers, used by queue.c MUST be first field!!
    LPVOID lpv;        // pointer to data
    DWORD  cb;         // size of data
    DWORD  dwOffset;   // file seek offset
    DWORD  dwError;    // success/fail of write operation
    DWORD  cbDone;     // actual bytes written/read
    BYTE   bWrite;     // read/write flag
    BYTE   bPending;   // TRUE when io has been removed from done queue
    BYTE   bSpare[2];  // spare flags
    } QIOBUF;

typedef struct _qio * LPQIO;
typedef struct _qio {
    QHEAD   que;      // head for buffers queued to be written
    QHEAD   queDone;  // pointer to head of queue for write completion
    HANDLE  hFile;
    HANDLE  hThread;
    DWORD   tid;
    UINT    uState;
    UINT    nIOCount;
    int     nPrio;
    } QIO;

BOOL WINAPI QioInitialize (
    LPQIO  lpqio,
    HANDLE hFile,
    int    nPrio
    );

// add a buffer to the async io queue
//
BOOL WINAPI QioAdd (
    LPQIO   lpqio,
    PQIOBUF pqBuf
    );

BOOL WINAPI QioWait (
    LPQIO   lpqio,
    PQIOBUF pqBufWait,
    BOOL    bWait
    );

BOOL WINAPI QioCommit (
    LPQIO lpqio
    );

// Shutdown Qio thread
//
BOOL WINAPI QioShutdown (
    LPQIO  lpqio
    );

#ifdef __cplusplus
}            // Assume C declarations for C++
#endif  // __cplusplus

#endif // DISK32_H
