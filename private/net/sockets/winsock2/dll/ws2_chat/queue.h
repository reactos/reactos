/*++

Copyright (c) 1995 Intel Corp

Module Name:

    queue.h

Abstract:

    Header file for the queue manager implemented by queue.c.

Author:

    Michael Grafton

--*/
#ifndef QUEUE_H
#define QUEUE_H

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */


#define ABNORMAL 1
#define NORMAL 0

#define MAX_QUEUE_SIZE 1024
#define Q_NULL -1


typedef struct _QUEUE {
  
  LPVOID QueueArray[MAX_QUEUE_SIZE];
  int Head;
  int Tail;
  CRITICAL_SECTION CrSec;
  
} QUEUE, *PQUEUE;


PQUEUE QCreate(void);
void QFree(PQUEUE Queue);
BOOL QInsert(PQUEUE Queue, LPVOID Object);
BOOL QInsertAtHead(PQUEUE Queue, LPVOID Object);
LPVOID QRemove(PQUEUE Queue);
BOOL QIsEmpty(PQUEUE Queue);

#endif
