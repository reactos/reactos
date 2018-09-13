/*++

Copyright (c) 1995 Intel Corp

Module Name:

    queue.h

Abstract:

    Implementation of functions supplied by this queue manager module.

Author:

    Michael Grafton

--*/

#include <stdio.h>
#include <malloc.h>
#include "queue.h"




PQUEUE 
QCreate(void)
/*++

Routine Description:

    Creates a new QUEUE, initializes it, and returns it. Returns NULL
    if memory cannot be allocated in the heap.   

Arguments:

    None.

Return Value:

    NULL if memory allocation fails; else a pointer to a new queue.

--*/
{

    PQUEUE Queue; // points to the new queue

    // Allocate space for a new queue.
    Queue = (PQUEUE)malloc(sizeof(QUEUE));
    if (Queue != NULL){
        
        // Initialize the new queue.
        Queue->Head = Q_NULL;
        Queue->Tail = Q_NULL;
        InitializeCriticalSection(&Queue->CrSec);
    }
    return(Queue);
}




void 
QFree(PQUEUE Queue)
/*++

Routine Description:

    This function removes any items left on the queue, frees them, and
    then frees the queue itself.

Arguments:

    Queue -- Pointer to the queue to free.

Return Value:

    None.

--*/
{
  
    LPVOID Object; // points to objects we pull off the queue

    EnterCriticalSection(&Queue->CrSec);

    // Free everything on the queue.
    Object = QRemove(Queue);
    while (Object) {
        free(Object);
        Object = QRemove(Queue);
    }

    LeaveCriticalSection(&Queue->CrSec);

    // Free the queue itself.
    free(Queue);
}




LPVOID 
QRemove(PQUEUE Queue)
/*++

Routine Description:

    This function dequeues one object from Queue and returns a generic
    pointer to the object.  It is up to the calling function to know
    the size and type of the object dequeued.

Arguments:

    Queue -- The queue from which to dequeue the object.

Return Value:

    NULL if the queue is empty; else, a pointer to the dequeued object.

--*/
{

    LPVOID Data;    // points to the object we pull off the queue
        
    EnterCriticalSection(&Queue->CrSec);
    
    if (Queue->Head == Q_NULL){
        
        // If the queue is empty, we will return NULL. 
        Data = NULL;
        
    } else {
        
        // Get the pointer, NULL it in the QueueArray.
        Data = Queue->QueueArray[Queue->Head];
        Queue->QueueArray[Queue->Head] = NULL;

        // Check to see if we've just emptied the queue; if so, set
        // the Head and Tail indices to Q_NULL.  If not, set the Head
        // index to the right value.
        if (Queue->Head == Queue->Tail) {
            Queue->Head = Queue->Tail = Q_NULL;
        } else {
            Queue->Head = (Queue->Head + 1) % MAX_QUEUE_SIZE;
        }
    }

    LeaveCriticalSection(&Queue->CrSec);
    return(Data);
}





BOOL
QInsert(
    PQUEUE Queue,
    LPVOID Object)
/*++

Routine Description:

    This function enqueues one item into Queue, if there is room for
    it. 

Arguments:

    Queue -- The queue onto which to enqueue Object.

    Object -- A void pointer to the object being enqueued.

Return Value:

    TRUE -- The object was successfully enqueued.

    FALSE -- There were too many items already in the queue; one item
    must be removed in order to insert another.

--*/
{
    BOOL ReturnValue; // holds the return value

    EnterCriticalSection(&Queue->CrSec);
    
    // If the queue is full, set the return value to FALSE and do
    // nothing; if not, update the indices appropriately and set the
    // return value to TRUE. 
    if(((Queue->Tail + 1) % MAX_QUEUE_SIZE == Queue->Head)
       && (Queue->Head != Q_NULL)) {

        ReturnValue = FALSE;

    } else {

        Queue->Tail = (Queue->Tail + 1) % MAX_QUEUE_SIZE;
        Queue->QueueArray[Queue->Tail] = Object;
        if (Queue->Head == Q_NULL){
            Queue->Head = 0;
        }
        ReturnValue = TRUE;
    }

    LeaveCriticalSection(&Queue->CrSec);
    return(ReturnValue);
}




BOOL
QInsertAtHead(
    PQUEUE Queue, 
    LPVOID Object)
/*++

Routine Description:

    This function inserts one item at the HEAD of the queue.  Of
    course, this violates queue semantics...but this can be useful at
    times. 

Arguments:

    Queue -- The queue into which to insert Object.

    Object -- A void pointer to the object being inserted.

Return Value:

    TRUE -- The object was successfully inserted at the head of the
    queue. 

    FALSE -- There were too many items already in the queue; one item
    must be removed in order to insert another.

--*/
{
 
    BOOL ReturnValue; // holds the return value

    EnterCriticalSection(&Queue->CrSec);

    // If the queue is full, set the return value to FALSE and do
    // nothing; if not, update the indices appropriately and set the
    // return value to TRUE. 
    if((Queue->Tail == Queue->Head) && (Queue->Head != Q_NULL)){
        
        ReturnValue = FALSE;
        
    } else {
  
        if (Queue->Head == Q_NULL) {

            // The queue was empty, so just use QInsert.
            QInsert(Queue, Object);
        }

        if (Queue->Head == 0) {
            Queue->Head = MAX_QUEUE_SIZE - 1;
        } else {
            Queue->Head = Queue->Head - 1;
        }
    
        Queue->QueueArray[Queue->Head] = Object;
        ReturnValue = TRUE;
    }

    LeaveCriticalSection(&Queue->CrSec);    
    return(ReturnValue);
}




BOOL
QIsEmpty(PQUEUE Queue)
/*++

Routine Description:

    This function tells the caller if the queue in question contains
    any items.

Arguments:

    Queue -- The queue which we want to query.

Return Value:

    TRUE -- The queue is empty.

    FALSE -- The queue contains at least one item.

--*/
{
    return(Queue->Head == Q_NULL ? TRUE : FALSE);
}
