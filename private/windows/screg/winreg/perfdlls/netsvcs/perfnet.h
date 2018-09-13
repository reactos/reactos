extern  HANDLE  hEventLog;       // handle to event log
extern  HANDLE  hLibHeap;       // handle to DLL heap            
extern  LPWSTR  wszTotal;

PM_OPEN_PROC    OpenServerObject;
PM_LOCAL_COLLECT_PROC CollectServerObjectData;
PM_CLOSE_PROC   CloseServerObject;

PM_OPEN_PROC    OpenServerQueueObject;
PM_LOCAL_COLLECT_PROC CollectServerQueueObjectData;
PM_CLOSE_PROC   CloseServerQueueObject;

PM_OPEN_PROC    OpenRedirObject;
PM_LOCAL_COLLECT_PROC CollectRedirObjectData;
PM_CLOSE_PROC   CloseRedirObject;

PM_OPEN_PROC    OpenBrowserObject;
PM_LOCAL_COLLECT_PROC CollectBrowserObjectData;
PM_CLOSE_PROC   CloseBrowserObject;

