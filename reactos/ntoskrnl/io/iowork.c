/* $Id: iowork.c,v 1.5 2003/07/11 01:23:14 royce Exp $
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               reactos/ntoskrnl/io/iowork.c
 * PURPOSE:            Manage IO system work queues
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 *                     Robert Dickenson (odin@pnc.com.au)
 * REVISION HISTORY:
 *       28/09/2002:   (RDD) Created from copy of ex/work.c
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _IO_WORKITEM
{
  WORK_QUEUE_ITEM Item;
  PDEVICE_OBJECT  DeviceObject;
  PIO_WORKITEM_ROUTINE WorkerRoutine;
  PVOID           Context;
} IO_WORKITEM;

/* GLOBALS ******************************************************************/

#define TAG_IOWI TAG('I', 'O', 'W', 'I')

/* FUNCTIONS ****************************************************************/

VOID STDCALL STATIC
IoWorkItemCallback(PVOID Parameter)
{
  PIO_WORKITEM IoWorkItem = (PIO_WORKITEM)Parameter;
  PDEVICE_OBJECT DeviceObject = IoWorkItem->DeviceObject;
  IoWorkItem->WorkerRoutine(IoWorkItem->DeviceObject, IoWorkItem->Context);
  ObDereferenceObject(DeviceObject);
}

/*
 * @implemented
 */
VOID STDCALL
IoQueueWorkItem(IN PIO_WORKITEM IoWorkItem, 
		IN PIO_WORKITEM_ROUTINE WorkerRoutine,
		IN WORK_QUEUE_TYPE QueueType, 
		IN PVOID Context)
/*
 * FUNCTION: Inserts a work item in a queue for one of the system worker
 * threads to process
 * ARGUMENTS:
 *        IoWorkItem = Item to insert
 *        QueueType = Queue to insert it in
 */
{
  ExInitializeWorkItem(&IoWorkItem->Item, IoWorkItemCallback, 
		       (PVOID)IoWorkItem);
  IoWorkItem->WorkerRoutine = WorkerRoutine;
  IoWorkItem->Context = Context;
  ObReferenceObjectByPointer(IoWorkItem->DeviceObject,
			     FILE_ALL_ACCESS,
			     NULL,
			     KernelMode);
  ExQueueWorkItem(&IoWorkItem->Item, QueueType);
}

/*
 * @implemented
 */
VOID STDCALL
IoFreeWorkItem(PIO_WORKITEM IoWorkItem)
{
  ExFreePool(IoWorkItem);
}

/*
 * @implemented
 */
PIO_WORKITEM STDCALL
IoAllocateWorkItem(PDEVICE_OBJECT DeviceObject)
{
  PIO_WORKITEM IoWorkItem = NULL;
  
  IoWorkItem = 
    ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_WORKITEM), TAG_IOWI);
  if (IoWorkItem == NULL)
    {
      return(NULL);
    }
  RtlZeroMemory(IoWorkItem, sizeof(IO_WORKITEM));
  IoWorkItem->DeviceObject = DeviceObject;
  return(IoWorkItem);
}

/* EOF */
