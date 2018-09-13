
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    bgtask.hxx

Abstract:

    Contains back ground task class def 

Author:

     Richard L Firth (rfirth) 11-Apr-1997

Revision History:

    22-Jun-1998 rfirth
        Created

--*/

//
// prototypes
//
BOOL    LoadBackgroundTaskMgr(VOID);
VOID    UnloadBackgroundTaskMgr(VOID);
DWORD   NotifyBackgroundTaskMgr(VOID);
DWORD   CreateAndQueueBackgroundWorkItem(LPCSTR);


class BackgroundTaskMgr {
public:
    BackgroundTaskMgr(); 
    DWORD           DeQueueAndRunBackgroundWorkItem();
    DWORD           QueueBackgroundWorkItem(CFsm* pFsm);
    CFsm*           CreateBackgroundFsm(LPCSTR szUrl);
    BOOL            HasBandwidth();
    VOID            NotifyFsmDone();
private:
    LONG            _lActiveFsm;
    CPriorityList   _bgTaskQueue; 

};

