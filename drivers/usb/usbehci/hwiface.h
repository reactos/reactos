#include "hardware.h"
#include <ntddk.h>

PQUEUE_TRANSFER_DESCRIPTOR
CreateDescriptor(PEHCI_HOST_CONTROLLER hcd, UCHAR PIDCode, ULONG TotalBytesToTransfer);

VOID
FreeDescriptor(PEHCI_HOST_CONTROLLER hcd, PQUEUE_TRANSFER_DESCRIPTOR Descriptor);

VOID
DumpQueueHeadList(PEHCI_HOST_CONTROLLER hcd);

PQUEUE_HEAD
CreateQueueHead(PEHCI_HOST_CONTROLLER hcd);

VOID
LinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead);

VOID
UnlinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead);

VOID
DeleteQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead);

VOID
LinkQueueHeadToCompletedList(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead);

VOID
CleanupAsyncList(PEHCI_HOST_CONTROLLER hcd);
