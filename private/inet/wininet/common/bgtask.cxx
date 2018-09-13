/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    bgtask.cxx

Abstract:

    Contains background task and support functions 

    Contents:
        LoadBackgroundTaskMgr
        UnloadBackgroundTaskMgr
        NotifyBackgroundTaskMgr
        CreateAndQueueBackgroundWorkItem

        BackgroundTaskMgr::QueueBackgroundWorkItem
        BackgroundTaskMgr::DeQueueAndRunBackgroundWorkItem 
        BackgroundTaskMgr::CreateBackgroundFsm
        BackgroundTaskMgr::Release
        BackgroundTaskMgr::HasBandwidth

        CFsm_BackgroundTask::RunSM
        CFsm_BackgroundTask::DoSendReq
        CFsm_BackgroundTask::~CFsm_BackgroundTask
Author:

    Danpo Zhang (danpoz) 06-26-98

Environment:

    Win32 user-mode

Revision History:

    06-26-1998 danpoz 
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

BackgroundTaskMgr* g_BGTaskMgr = NULL;

//
// API: Init global BackgroundTaskManager
//
BOOL
LoadBackgroundTaskMgr()
{
    if( g_BGTaskMgr )
        return TRUE;

    BackgroundTaskMgr* bgMgr = NULL;
    bgMgr = new BackgroundTaskMgr();
    if( !bgMgr)
        return FALSE;

    g_BGTaskMgr = bgMgr;

    return TRUE;
}

//
// API: Unload global BackgroundTaskManager
//
void
UnloadBackgroundTaskMgr()
{
    if( g_BGTaskMgr )
    {
        //BUGBUG
        //what to do with unfinished task?
        delete g_BGTaskMgr;
    }

    g_BGTaskMgr = NULL;
}


//
// API: Select thread notifis now is a good time to do background task 
//
DWORD
NotifyBackgroundTaskMgr()
{
    DWORD error;

    // can we run another background item?
    if( !g_BGTaskMgr->HasBandwidth() )
    {
        error = ERROR_SUCCESS;
        goto quit;
    }

    // get a background FSM if there is any 
    g_BGTaskMgr->DeQueueAndRunBackgroundWorkItem();

    error = ERROR_SUCCESS;
quit:
    return error;
}


//
// create a background task (fsm) and queue it on the background task
// list, the task item will be picked up later by a free async worker
// thread
//
DWORD
CreateAndQueueBackgroundWorkItem(
    IN  LPCSTR  szUrl
    )
{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "CreateAndQueueBackgroundWorkItem",
                 "%q",
                 szUrl 
                 ));

    DWORD   error;
    CFsm*   pFsm = NULL;

    INET_ASSERT( szUrl );

    // get new fsm
    pFsm = g_BGTaskMgr->CreateBackgroundFsm(szUrl);
    if( !pFsm )
    {
        error = ERROR_NOT_ENOUGH_MEMORY; 
        goto quit;
    }

    // queue fsm
    error = g_BGTaskMgr->QueueBackgroundWorkItem(pFsm);
    if( error != ERROR_SUCCESS )
    {
        // delete the fsm to avoid leak
        delete pFsm;
    }

quit:
    DEBUG_LEAVE(error);
    return error;

}



DWORD
BackgroundTaskMgr::QueueBackgroundWorkItem(
    IN CFsm* pFsm
    )
{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "ICAsyncThread::QueueBackgroundWorkItem",
                 "%#x",
                 pFsm
                 ));

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error = ERROR_INTERNET_INTERNAL_ERROR;

    INET_ASSERT(lpThreadInfo != NULL);

    if( lpThreadInfo != NULL )
    {
        _bgTaskQueue.Acquire();
        _bgTaskQueue.Insert((CPriorityListEntry *)pFsm->List());
        lpThreadInfo->Fsm = NULL;
        _bgTaskQueue.Release();

        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);
    return error;
}



DWORD 
BackgroundTaskMgr::DeQueueAndRunBackgroundWorkItem()
{

    // LOCK list
    _bgTaskQueue.Acquire();
    
    // check the list to see if it is empty
    if( _bgTaskQueue.Head() != _bgTaskQueue.Self() )
    {
        PLIST_ENTRY pEntry = NULL;
        PLIST_ENTRY pPrev = NULL;
        
        pPrev = _bgTaskQueue.Self();
        pEntry = ((CPriorityListEntry*)pPrev)->Next();

        CFsm* pFsm = ContainingFsm(pEntry);
        INET_ASSERT(pFsm);

        // deQueued, we can remove the task item from the queue 
        // remove from the blocked list
        _bgTaskQueue.Remove((CPriorityListEntry *)pFsm);

        // increment the active running fsm count
        InterlockedIncrement(&_lActiveFsm);

        // this fsm will be picked up by a waken worker thread
        pFsm->QueueWorkItem();
    }

    // UNLOCK list
    _bgTaskQueue.Release();

    return ERROR_SUCCESS;
}

CFsm*
BackgroundTaskMgr::CreateBackgroundFsm(LPCSTR szUrl)
{
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    // HACK - HACK for TESTING... 
    // correct solution is when lpThreadInfo->Fsm != NULL
    // we do not create a fsm so that we always make sure
    // there is one fsm on the select thread 
    //
    if( lpThreadInfo != NULL )
    {
        lpThreadInfo->Fsm = NULL;
    }

    return new CFsm_BackgroundTask(this, szUrl);
}
    

BOOL
BackgroundTaskMgr::HasBandwidth()
{
    // only one fsm can be picked at anytime
    return !_lActiveFsm;  
}


BackgroundTaskMgr::BackgroundTaskMgr() 
    : _lActiveFsm(0)
{
}

void 
BackgroundTaskMgr::NotifyFsmDone()
{
    InterlockedDecrement(&_lActiveFsm);
}



DWORD 
CFsm_BackgroundTask::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_ASYNC,
                 Dword,
                 "CFsm_BackgroundTask::RunSM",
                 "%#x",
                 Fsm
                 ));

    
    CFsm_BackgroundTask* fsm = (CFsm_BackgroundTask*) Fsm;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    BOOL fIsAsyncWorkerThread = TRUE;

    if( lpThreadInfo )
    {
        fIsAsyncWorkerThread = lpThreadInfo->IsAsyncWorkerThread;
        lpThreadInfo->IsAsyncWorkerThread = FALSE;
    }

    switch( fsm->GetState() ) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        fsm->DoSendReq();

    default:
        break;
    }

    fsm->SetDone(0);
    if( lpThreadInfo )
    {
        lpThreadInfo->IsAsyncWorkerThread = fIsAsyncWorkerThread;
    }
    DEBUG_LEAVE(0);
    return 0;
}


typedef HRESULT (WINAPI * pfnObtainUA)(DWORD, LPSTR, DWORD*);

DWORD
CFsm_BackgroundTask::DoSendReq()
{
    HINTERNET hInternet = NULL;
    HINTERNET hRequest = NULL;
    CHAR szBuffer[4000];
    DWORD dwBytesRead;
    BOOL  fSuccess;

    DWORD error;
    BOOL  fUAFromUrlmon = FALSE;


    CHAR*  pszUA = NULL;

    HINSTANCE hinst = GetModuleHandle("urlmon.dll");
    if( hinst )
    {
        pfnObtainUA pfnUA = (pfnObtainUA)
            GetProcAddress(hinst, "ObtainUserAgentString");

        if( pfnUA )
        {
            DWORD dwSize = MAX_PATH;
            pszUA = new CHAR[dwSize];
            if( pszUA )
            {
                HRESULT hr = (*pfnUA)(0, pszUA, &dwSize);
            
                if( S_OK == hr )
                {
                    fUAFromUrlmon = TRUE;
                }
                else if( E_OUTOFMEMORY == hr )
                {
                    // the original pszUA is allocated too small
                    // we need bigger buffer size (returned by dwSize)
                    delete [] pszUA;
            
                    pszUA = new CHAR[dwSize];
                    if( pszUA )
                    { 
                        hr = (*pfnUA)(0, pszUA, &dwSize);
                        if( S_OK == hr )
                        {
                            fUAFromUrlmon = TRUE;
                        }
                    } 
                } // original buffer too small, create bigger one
            }  
        } // get the pFN to UA agent
    } // urlmon.dll is loaded

    if( fUAFromUrlmon && pszUA)
    {
        hInternet = InternetOpen(
                pszUA, 
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0 );
    }
    else
    {
        hInternet = InternetOpen(
                "Mozilla/4.0 (compatible; MSIE 5.01; Win32)", 
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL,
                NULL,
                0 );
    }

    if( !hInternet )
    {
        goto quit;
    }

    hRequest = InternetOpenUrl(
                hInternet,
                m_lpszUrl,
                "Accept: */*\r\n", 
                (DWORD) -1,
                INTERNET_FLAG_RESYNCHRONIZE | INTERNET_FLAG_BGUPDATE | INTERNET_FLAG_KEEP_CONNECTION,
                INTERNET_NO_CALLBACK
                );

    if( !hRequest )
    {
        DWORD dwLastErr = 0;
        dwLastErr = GetLastError();
        goto quit;
    }

    
    do {
        dwBytesRead = 0;

        fSuccess = InternetReadFile(
                        hRequest,
                        szBuffer,
                        sizeof(szBuffer)-1,
                        &dwBytesRead
                        );

        if( !fSuccess )
        {
            goto quit;
        }

    } while ( dwBytesRead != 0 );        

    error = ERROR_SUCCESS;

quit:
    if( hRequest )
        InternetCloseHandle(hRequest);
    if( hInternet )
        InternetCloseHandle(hInternet);

    if( fUAFromUrlmon && pszUA )
        delete [] pszUA;

    return error;
}

CFsm_BackgroundTask::~CFsm_BackgroundTask()
{
    DELETE_MANDATORY_PARAM(m_lpszUrl);
    m_pMgr->NotifyFsmDone();
}

