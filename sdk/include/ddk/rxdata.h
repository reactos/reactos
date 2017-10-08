#ifndef _RDBSSDATA_
#define _RDBSSDATA_

extern RX_DISPATCHER RxDispatcher;
extern RX_WORK_QUEUE_DISPATCHER RxDispatcherWorkQueues;

extern KMUTEX RxSerializationMutex;
#define RxAcquireSerializationMutex() KeWaitForSingleObject(&RxSerializationMutex, Executive, KernelMode, FALSE, NULL)
#define RxReleaseSerializationMutex() KeReleaseMutex(&RxSerializationMutex, FALSE)

extern PRDBSS_DEVICE_OBJECT RxFileSystemDeviceObject;

#if DBG
extern ULONG RxFsdEntryCount;
#endif

extern LIST_ENTRY RxSrvCalldownList;
extern LIST_ENTRY RxActiveContexts;

#endif
