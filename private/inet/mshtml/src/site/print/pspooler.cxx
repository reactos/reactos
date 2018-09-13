//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       pspooler.cxx
//
//  Contents:
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PSPOOLER_HXX_
#define X_PSPOOLER_HXX_
#include "pspooler.hxx"
#endif

#ifndef X_FPRINT_HXX_
#define X_FPRINT_HXX_
#include "fprint.hxx"
#endif

#ifndef X_PRINTWRP_HXX_
#define X_PRINTWRP_HXX_
#include "printwrp.hxx"
#endif

#ifndef X_MSRATING_HXX_
#define X_MSRATING_HXX_
#include "msrating.hxx" // areratingsenabled()
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_UWININET_H_
#define X_UWININET_H_
#include "uwininet.h"
#endif

#ifndef X_WINSPOOL_H_
#define X_WINSPOOL_H_
#include "winspool.h"
#endif

MtDefine(CSpooler, Printing, "CSpooler")
MtDefine(CSpoolerCBindStatusCallback, Printing, "CSpooler::CBindStatusCallback")
MtDefine(CSpooler_aryFrameTempFiles_pv, CSpooler, "CSpooler::_aryFrameTempFiles::_pv")

// Timeouts in seconds.
#define IS_HTML_TIMEOUT 60

HRESULT CreateResourceMoniker(HINSTANCE hInst, TCHAR *pchRID, IMoniker **ppmk);



////////////////////////
// Print Spooler Tags //
////////////////////////

DeclareTag(tagSpooler, "Spooler", "Spooler: Trace Spooler");
DeclareTag(tagIPrintBotherUser, "PrintIPrintBotherUser", "IPrint should bother user");
DeclareTag(tagNoDummyJobs, "PrintNoDummyJobs", "Dont add dummy jobs to queue until print");


//////////////
//  Globals //
//////////////

CCriticalSection    g_csSpooler;
#ifndef WIN16
CSpooler *          g_pSpooler = NULL;
#endif

#ifdef WIN16
static DWORD        g_dwDummyJobId = 0;
#endif // WIN16


////////////////////
// CSpooler Class //
////////////////////

CSpooler::CSpooler()
    : CExecFT(&_cs),  // tell CBaseFT about our critical section
      _aryFrameTempFiles(Mt(CSpooler_aryFrameTempFiles_pv))
{
    InitializeCriticalSection(&_cs);
    _cInit = 0;
    _pts = NULL;
    _ulPrintJobId = 0;
    _fLoadJobWaiting = FALSE;
    _fCanLoad = TRUE;
    _fPrintJobWaiting = FALSE;
    _fShutdown = FALSE;
    _pPrintDocLoading = NULL;
    _PrintObjectPrinting.pPrintDoc = NULL;
    _ppiCurrentLoadJob = NULL;
    _ppiCurrentPrintJob = NULL;
    _pBindStatusCallback = NULL;

    // print support.

    _PrintInfoBag.ptPaperSize.x = 8500;
    _PrintInfoBag.ptPaperSize.y = 11000;

    // _PrintInfoBag.rtMargin initialized in PRINTINFOBAG constructor.
    // _PrintInfoBag.rtMargin = registry settings.

    _PrintInfoBag.hDevMode = 0;
    _PrintInfoBag.hDevNames = 0;
    _PrintInfoBag.hdc = 0;          // always 0 on the spooler level
    _PrintInfoBag.dwThreadId = 0;   // not used in the spooler
    _PrintInfoBag.ptd = 0;          // always 0 on the spooler level
    _PrintInfoBag.fAllPages = 1;
    _PrintInfoBag.nFromPage = 1;
    _PrintInfoBag.nToPage = 1;
    _PrintInfoBag.fPagesSelected = 0;
    _PrintInfoBag.fXML = 0;

    // store the current default printer. We need that for compare later

    GetProfileString(_T("Windows"), _T("Device"), _T(""), _achDefaultPrinter, DEFAULTPRINTERSIZE);
}

CSpooler::~CSpooler()
{
    EmptyQueue(&_dblPrintJobs);
    EmptyQueue(&_dblOldPrintJobs);

        // see comment at SetPrintInfo
    if (_PrintInfoBag.hDevNames)
        GlobalFree(_PrintInfoBag.hDevNames);
    if (_PrintInfoBag.hDevMode)
        GlobalFree(_PrintInfoBag.hDevMode);

    DeleteCriticalSection(&_cs);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::Passivate
//
//  Synopsis:   Passivates the spooler by first causing the CSpooler::ThreadExec
//              loop to exit (shut down), and then returns after the thread has
//              ended (which is when CExecFT::Shutdown() invoked by CSpooler::
//              Shutdown() returns).
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

void CSpooler::Passivate()
{
    TraceTag((tagSpooler, "Spooler Passivate (Enter)"));

    _fShutdown = TRUE;

    TraceTag((tagSpooler, "Spooler Passivate: kill Spooler thread"));

    Shutdown();

    super::Passivate();

    TraceTag((tagSpooler, "Spooler Passivate (Leave)"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::Release
//
//  Synopsis:   Special release function that detaches from global on last release.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

ULONG CSpooler::Release()
{
    ULONG ulRefs;
    {
        // Final release needs to be protected from getspooler's initial addref
        LOCK_SECTION(g_csSpooler);

        ulRefs = InterlockedRelease();
        if (ulRefs == 0)
        {
            // detach from global
            Assert(TASK_GLOBAL(g_pSpooler)==this);
            TASK_GLOBAL(g_pSpooler) = NULL;
        }

//
// IEUNIX: BUGBUG Unix used only 1 thread.
// We should take this out after we support multi-threads.
//
#ifdef UNIX
        // Make this thread variable NULL otherwise we endup
        // releasing it twice and hence freeing it up.
        THREADSTATE * pts = GetThreadState();
        if( pts->pSpooler == this )
            pts->pSpooler = NULL;
#endif
        // Leave critical section g_csSpooler
    }

    if (ulRefs == 0)
    {
        Passivate();
        SubRelease();
    }

    return(ulRefs);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::Shutdown
//
//  Synopsis:   Shuts down the spooler.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

void CSpooler::Shutdown()
{
    _fShutdown = TRUE;

    SetEvent(_hevWait);

    // Wait up to five minutes for the spooler thread to actually end.
    super::Shutdown(300000);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::ThreadInit
//
//  Synopsis:   Initializes the spooler thread.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

HRESULT CSpooler::ThreadInit()
{
    HRESULT hr;

#ifndef WIN16
    hr = THR(OleInitialize(NULL));
    if (FAILED(hr))
        goto Error;
#endif // ndef WIN16

    _cInit = 1;

    hr = THR(AddRefThreadState(&_dwObjCnt));
    if (hr)
        goto Error;

    _pts = GetThreadState();

    _cInit = 2;

    _hevWait = CreateEventA(NULL, FALSE, FALSE, NULL);

    if (_hevWait == NULL)
    {
        hr = GetLastWin32Error();
        goto Error;
    }

    _cInit = 3;

Error:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::ThreadTerm
//
//  Synopsis:   Terminates the spooler thread.
//
//  Arguments:  None.
//
//----------------------------------------------------------------------------

void CSpooler::ThreadTerm()
{
    if (_pBindStatusCallback)
    {
        // Block future callbacks from callback object
        _pBindStatusCallback->_pSpooler = NULL;

        _pBindStatusCallback->Release();
    }

    if (_pPrintDocLoading)
    {
        _pPrintDocLoading->Release();
    }

    if (_ppiCurrentPrintJob)
    {
        switch (_ppiCurrentPrintJob->grfPrintObjectType)
        {
        case POT_CPRINTDOC:
            if (_PrintObjectPrinting.pPrintDoc)
                _PrintObjectPrinting.pPrintDoc->Release();
            break;

        case POT_IPRINT:
            ReleaseInterface(_PrintObjectPrinting.pPrint);
            break;

        case POT_IOLECOMMANDTARGET:
            ReleaseInterface(_PrintObjectPrinting.pOleCommandTarget);
            break;
        }
    }

    if (_cInit >= 2)
    {
        ReleaseThreadState(&_dwObjCnt);
    }

#ifndef WIN16
    if (_cInit >= 1)
    {
        OleUninitialize();
    }
#endif // ndef WIN16

    CloseEvent(_hevWait);
}







//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::ThreadExec
//
//  Synopsis:   The heart of the spooler.  A "printing loop" that initiates
//              new downloads, recognizes when load jobs have completed,
//              and initiates the printing of jobs.  This function only exits
//              on shutdown, and the spooler thread lives only within this
//              scope.
//
//  Arguments:  None.
//
//----------------------------------------------------------------------------

void CSpooler::ThreadExec()
{
    THREADSTATE *   pts = GetThreadState();
    DWORD           dw;
    MSG             msg;
    HRESULT         hrLocal;
    EVENT_HANDLE    hEventToRaise=0;
    EVENT_HANDLE    hEventOnError=0;

    TraceTag((tagSpooler, "Spooler ThreadExec (Enter)"));

    for (;;)
    {
        // Wait for the wake-up event or more messages.
        if ( !(_fCanLoad && _fLoadJobWaiting) && !_fPrintJobWaiting )
        {
#ifdef WIN16
            dw = WaitForSingleObject(_hevWait, INFINITE);
#elif defined(UNIX)
            dw = MsgWaitForMultipleObjects(1, &_hevWait, FALSE, 2000, (DWORD)-1);
#else
            dw = MsgWaitForMultipleObjects(1, &_hevWait, FALSE, 2000, QS_ALLINPUT);
#endif
        }

        // Leave if it is time to shutdown.
        if (_fShutdown) // && IsEmpty())
            break;

        // Process all messages in the message queue.
#ifdef WIN16
        // WIN16 threading limitation - we shouldn't dispatch messages from any thread
        // other than the main one
        ThreadYield();
#else        
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#endif

        // Check if the current load job has been cancelled during downloading and
        // if so whether the cancellation resulted in an OnLoadComplete notification.
        if (_ppiCurrentLoadJob && !_fCanLoad)
        {
            EnterCriticalSection();
            if (_ppiCurrentLoadJob && !_fCanLoad)
            {
                // If the load job is not marked cancelled, see if the user cancelled it
                // in the meantime.
                if (!_ppiCurrentLoadJob->fCancelled)
                {
                    // Check Windows print queue.
                    _ppiCurrentLoadJob->fCancelled = QueryDummyJob(_ppiCurrentLoadJob) != S_OK;

                    // If the user has cancelled the job currently loading, abort the
                    // download.
                    if (_ppiCurrentLoadJob->fCancelled)
                    {
                        // In 99% of all cases, this results in a CSpooler::OnLoadComplete()
                        // callback.  For the other 1% of cases, we set up a timeout,
                        // after which we will give ourselves the callback.
                        if (_ppiCurrentLoadJob->grfPrintObjectType == POT_CPRINTDOC
                            && !_ppiCurrentLoadJob->fAbortLoading)
                        {
                            // Mark the load job as download abort.
                            _ppiCurrentLoadJob->fAbortLoading = TRUE;
                            _dwLoadJobAbortTime = GetTickCount();
                        }

                        // Abort download.
                        IGNORE_HR( CancelDownload() );
                    }
                    else if (!_ppiCurrentLoadJob->fAbortLoading &&
                             _pPrintDocLoading && _pPrintDocLoading->WaitingForNothingButControls()
                        )
                    {
                        // Mark the load job as download abort.
                        _ppiCurrentLoadJob->fAbortLoading = TRUE;
                        _dwLoadJobAbortTime = GetTickCount();
                    }
                    else if (_ppiCurrentLoadJob->fAbortLoading
                        && _pPrintDocLoading && !_pPrintDocLoading->WaitingForNothingButControls())
                    {
                        // If we are suddenly no longer only waiting for controls anymore,
                        // invert our decision
                        _ppiCurrentLoadJob->fAbortLoading = FALSE;
                    }
                }

                if (_ppiCurrentLoadJob->fAbortLoading && _pPrintDocLoading)
                {
                    // This means, we haven't received the CSpooler::OnLoadComplete()
                    // callback (remaining 1% of all cases), or were waiting too long
                    // for controls only. After a timeout kicks in, we simply give
                    // ourselves the callback.

                    DWORD dwCurrentTickCount = GetTickCount();

                    // Time out after 30 seconds (30,000 millisecs).
                    if (dwCurrentTickCount - _dwLoadJobAbortTime > 30000)
                    {
                        OnLoadComplete(_pPrintDocLoading, SP_FINISHED_LOADING_HTMLDOCUMENT);
                    }
                }
            }
            LeaveCriticalSection();
        }

        {
            // Check if there are any unremoved dummy jobs to be removed from the dummyjoblist.
            DUMMYJOB *pdjUnremovedDummyJob = _dblUnremovedDummyJobs.First(), *pdjNextUnremovedDummyJob = NULL;
            while (pdjUnremovedDummyJob)
            {
                pdjNextUnremovedDummyJob = _dblUnremovedDummyJobs.Next(pdjUnremovedDummyJob);

                // Add a dummy job on the windows queue.
                IGNORE_HR( RemoveDummyJob(NULL, pdjUnremovedDummyJob, RDJ_REMOVE_DUMMYJOB_IF_POSSIBLE) );

                pdjUnremovedDummyJob = pdjNextUnremovedDummyJob;
            }
        }

        // Check if a new load job can be started.
        if (_fCanLoad && _fLoadJobWaiting)
        {
            hrLocal = S_FALSE;

            Assert(!_dblPrintJobs.IsEmpty() && _dblPrintJobs.First()
                && "Corrupt spooler state - no print job available");

            // Extract print job information.
            EnterCriticalSection();

            _ppiCurrentLoadJob = _dblPrintJobs.First();
            _fLoadJobWaiting = _dblPrintJobs.Next(_ppiCurrentLoadJob) != NULL;
            _fCanLoad = FALSE;

            // Was the job cancelled "manually"?
            _ppiCurrentLoadJob->fCancelled |= QueryDummyJob(_ppiCurrentLoadJob) != S_OK;

            // Initiate loading asynchronously.
            if (!_ppiCurrentLoadJob->fCancelled)
            {
                LeaveCriticalSection();

                hrLocal = InitiateLoading();

                EnterCriticalSection();

                Assert(OK(hrLocal) && "Problem initiating loading of print job");
            }

            if (hrLocal)
            {
                // If problems occurred or job cancelled, give next print job a chance.
                _dblPrintJobs.Remove(_ppiCurrentLoadJob);
                _dblOldPrintJobs.Append(_ppiCurrentLoadJob);
                _fCanLoad = TRUE;

                // Remove dummy job since job failed or was cancelled.
                IGNORE_HR( RemoveDummyJob(_ppiCurrentLoadJob) );

                // If print DC is no longer needed, get rid of it.
                if (!_fPrintJobWaiting || _ppiCurrentLoadJob->fRootDocument)
                {
                    hEventOnError = _ppiCurrentLoadJob->hEvent;
                    IGNORE_HR(WrapUpPrintJob(_ppiCurrentLoadJob));
                }

                // No current load job at this point.
                _ppiCurrentLoadJob = NULL;
            }

            LeaveCriticalSection();
        }

        // Print the next job
        if (_fPrintJobWaiting)
        {
            // ATTENTION: The spooler thread is the one printing which
            // means that while the spooler is printing a job, nothing
            // else is done in the spooler thread.  Therefore when the
            // spooler detects that a job has finished loading, no job
            // is currently busy printing.
            hrLocal = InitiatePrinting(&hEventToRaise);

            _fPrintJobWaiting = FALSE;

            DeleteTempFiles();
        }

        // ATTENTION2: It is possible for some jobs to finish
        // loading synchronously even though we ask them to load
        // asynchronously.  This happens when we bind to an object
        // which we have already loaded to find out if Trident can
        // print it.  In this case the next job finishes loading
        // before the current job has started printing, and when
        // the load job finishes this early (synchronously), we
        // we mark it as such in CSpooler::OnLoadComplete(), so
        // that the print job is not overwritten by the next load
        // job.  We can therefore process such a job here because
        // we have just finished printing the current job if there
        // was one.
        if (_ppiCurrentLoadJob && _ppiCurrentLoadJob->fLoadFinishedEarly)
        {
            // No print job that could erroneously be overwritten.
            Assert(!_ppiCurrentPrintJob);

            // Process the deferred OnLoadComplete notification.
            OnLoadComplete(_pPrintObjectLoadFinishedEarly, _idCallbackIdLoadFinishedEarly);
        }

        // Give each unblocked lightweight task a chance to run as long as
        // there are no messages to process.
        while (pts->task.cUnblocked > 0)
        {
            CTask::TaskmanRun();

            if ( GetQueueStatus(QS_ALLINPUT) )
                break;
        }

        // we can be dead now, but it should be safe to
        // clear this event so that the caller can run again
        if (hEventToRaise)
        {
            SetEvent(hEventToRaise);
            hEventToRaise=0;
        }

        // we can be dead now, but it should be safe to
        // clear this event so that the caller can run again
        if (hEventOnError)
        {
            SetEvent(hEventOnError);
            hEventOnError=0;
        }
    }

    IGNORE_HR( WrapUpPrintSpooler() );

    // If there are still objects around, wait for them to be released
    if (pts->dll.lObjCount > 1)
    {
        DWORD dwStart = GetTickCount();

        TraceTag((tagSpooler, "Spooler Shutdown (waiting for %ld objects to go away)",
            pts->dll.lObjCount - 1));

        while (pts->dll.lObjCount > 1 && (GetTickCount() - dwStart) < 5000)
        {
            // Process incoming messages.  With luck, they will release
            // the outstanding objects.
#ifdef WIN16
            dw = WaitForSingleObject(_hevWait, 250);
#else
            dw = MsgWaitForMultipleObjects(1, &_hevWait, FALSE, 250, QS_ALLINPUT);

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
#endif
        }

        TraceTag((tagSpooler, "Spooler Shutdown (%ld objects remain)",
            pts->dll.lObjCount - 1));
    }

    TraceTag((tagSpooler, "Spooler ThreadExec (Leave)"));
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::OnLoadComplete
//
//  Synopsis:   A callback function for the currently active print doc.
//              Called when download is complete or load error.
//
//              This function is used by CPrintDoc's when they finish loading.
//              In case of a CPrintDoc load failure, we still get a call, but
//              in that case the document contains the actual error which we
//              print.  As a result of this mechanism, CPrintDoc load problems
//              are completely transparent to us.
//
//              This function is also called by the bind-status-callback object
//              in turn used by servers supporting IPrint or IOleCommandTarget.
//              Load failures in that case are reported to us.  If there is no
//              load failure, we simply call the corresponding print function.
//              On the other hand, if there is such an "external" load failure
//              we replace the failed IPrint/IOleCommandTarget job with a
//              CPrintDoc containing the error message which we print instead.
//                  We do this by discarding the failed job, and creating an
//              error document (CPrintDoc) loaded from src\f3\rsrc\prntserv.htm.
//              Immediately after we initiate the creation of such an error
//              document, we return, relying on the callback from the error
//              document to progress the state of the spooler.
//
//  Arguments:  pPrintObject: Print object pointer used for identification
//              id:           Spooler callback id (type of notification)
//
//----------------------------------------------------------------------------

void CSpooler::OnLoadComplete(void *pPrintObject, SPOOLER_CALLBACK_ID id)
{
    EnterCriticalSection();

    if (!_ppiCurrentLoadJob || _fShutdown)
    {
        Assert(_fShutdown && "Unexpected call to CSpooler::OnLoadComplete");
        goto Cleanup;
    }

    // If there is a print job still in progress or if the load job has
    // finished loading (synchronously) before we finished initiating it, ...
    if (_ppiCurrentPrintJob || _ppiCurrentLoadJob->fInitiatingLoading)
    {
        // Remember to revisit the load job later and return immediately.
        _ppiCurrentLoadJob->fLoadFinishedEarly = TRUE;
        _idCallbackIdLoadFinishedEarly = id;
        _pPrintObjectLoadFinishedEarly = pPrintObject;
        goto Cleanup;
    }

    // Clear the abort loading flag in case it was set.
    _ppiCurrentLoadJob->fAbortLoading = FALSE;

    switch (id)
    {
    case SP_FAILED_LOADING_EXTERNALDOCUMENT:
    {
        HRESULT hr;

        Assert(_pPrintDocLoading == NULL);

        // Attempt to create an error document.
        if (!_ppiCurrentLoadJob->fCancelled && !_fShutdown)
        {
            // Load an error document.  If this function succeeds, we are going to receive
            // another callback.
            hr = THR( LoadErrorDocument(&_pPrintDocLoading, _ppiCurrentLoadJob, INET_E_CANNOT_INSTANTIATE_OBJECT) );

            if (hr)
            {
                Assert(_pPrintDocLoading == NULL);

                // If creating the document fails, we will just pretend this job
                // was cancelled.
                _ppiCurrentLoadJob->fCancelled = TRUE;
            }
            else
            {
                // Turn this document into a printdoc.
                _ppiCurrentLoadJob->grfPrintObjectType = POT_CPRINTDOC;

                // Make sure we preserve the original Url.
                _ppiCurrentLoadJob->fOverrideDocUrl = TRUE;

                // Because we created the errordocument, we usually get another callback when
                // the downman finishes loading the error document.

                ClearInterface(&_pBindStatusCallback);

                // Depend on the other callback (result from CreateErrorDocument() which loads the
                // error document as a print doc) to progress the spooler's state.
                goto Cleanup;
            }
        }
        // fall through only if we couldn't create an error document...
    }
    case SP_FINISHED_LOADING_EXTERNALDOCUMENT:
        ClearInterface(&_pBindStatusCallback);
        // fall through

    case SP_FINISHED_LOADING_HTMLDOCUMENT:

        // Sometimes we are getting extraneous notifications, ignore those:
        if ( id == SP_FINISHED_LOADING_HTMLDOCUMENT
          && ((void *) _pPrintDocLoading != pPrintObject || !_ppiCurrentLoadJob) )
        {
            Assert(!"Ignoring extraneous call to OnLoadStatus(LOADSTATUS_DONE)");
            goto Cleanup;
        }

        // If we just finished loading an Outlook Express Header.
        if (_ppiCurrentLoadJob->ppibagRootDocument->fPrintOEHeader
            && !_ppiCurrentLoadJob->ppibagRootDocument->pPrintDocOEHeader
            && !_ppiCurrentLoadJob->fCancelled && !_fShutdown)
        {
            // Reprocess current print info - this time with the real Url.
            _fLoadJobWaiting = TRUE;

            // Avoid infinite print header loop.
            Assert(_pPrintDocLoading);
            if (!_pPrintDocLoading)
                _ppiCurrentLoadJob->ppibagRootDocument->fPrintOEHeader = FALSE;

            // Print info bag takes ownership of OE header print doc.
            _ppiCurrentLoadJob->ppibagRootDocument->pPrintDocOEHeader = _pPrintDocLoading;
        }
        else
        {
            // Dequeue print job.
            _dblPrintJobs.Remove(_ppiCurrentLoadJob);
            _dblOldPrintJobs.Append(_ppiCurrentLoadJob);

            // If job cancelled, don't expand or print job.
            if (_ppiCurrentLoadJob->fCancelled || _fShutdown)
            {
                DWORD dwPrintObjectType = _ppiCurrentLoadJob->grfPrintObjectType;

                // Remove dummy job since job was cancelled or failed.
                IGNORE_HR(RemoveDummyJob(_ppiCurrentLoadJob));

                // Loaded print job cancelled, failed, or spooler shutdown.
                IGNORE_HR(WrapUpPrintJob(_ppiCurrentLoadJob));

                // Discard print doc
                if (dwPrintObjectType == POT_CPRINTDOC && _pPrintDocLoading)
                {
                    _pPrintDocLoading->Release();
                }
            }
            else
            {
                switch (_ppiCurrentLoadJob->grfPrintObjectType)
                {
                case POT_CPRINTDOC:
                {
                    TCHAR   achOverridePrintUrl[pdlUrlLen];

                    if (!_ppiCurrentLoadJob->fAlternateDoc
                      && (S_OK != AreRatingsEnabled())
                      && S_OK == _pPrintDocLoading->GetAlternatePrintDoc(achOverridePrintUrl, pdlUrlLen)
                      && (_tcslen(achOverridePrintUrl) > 0))
                    {
                        // Prepend the alternate doc.
                        if (S_OK != PrependChildURLsOfLoadDoc(TRUE, achOverridePrintUrl))
                        {
                            // Remove dummy job if alternate print job wasn't enqueued.
                            IGNORE_HR(RemoveDummyJob(_ppiCurrentLoadJob));
                        }

                        // Loaded print job not to be printed.
                        IGNORE_HR(WrapUpPrintJob(_ppiCurrentLoadJob));

                        // Don't print current load PrintDoc.
                        _pPrintDocLoading->Release();
                    }
                    else if (_pPrintDocLoading->_fFrameSet && !_pPrintDocLoading->_PrintInfoBag.fPrintAsShown)
                    {
                        // Prepend all frame Urls.
                        IGNORE_HR(PrependChildURLsOfLoadDoc(FALSE));

                        // Remove dummy job since root job with frames is not printed.
                        IGNORE_HR(RemoveDummyJob(_ppiCurrentLoadJob));

                        // Loaded print job not to be printed.
                        IGNORE_HR(WrapUpPrintJob(_ppiCurrentLoadJob));

                        // Don't print current load PrintDoc.
                        _pPrintDocLoading->Release();
                    }
                    else // No frames:
                    {
                        // If we are printing recursively, prepend all nested links.
                        // Afterwards, print current load PrintDoc.
                        if (_ppiCurrentLoadJob->nDepth)
                        {
                            IGNORE_HR(PrependChildURLsOfLoadDoc(TRUE));
                        }

                        // Allow this job to print.
                        _PrintObjectPrinting.pPrintDoc = _pPrintDocLoading;
                        _ppiCurrentPrintJob = _ppiCurrentLoadJob;
                        _fPrintJobWaiting = TRUE;
                    }
                    break;
                }

                case POT_IPRINT:
                    // Allow this job to print.
                    _PrintObjectPrinting.pPrint = (IPrint *) pPrintObject;
                    // fall through

                case POT_IOLECOMMANDTARGET:
                    if (_ppiCurrentLoadJob->grfPrintObjectType == POT_IOLECOMMANDTARGET)
                    {
                        _PrintObjectPrinting.pOleCommandTarget = (IOleCommandTarget *) pPrintObject;
                    }

                    _ppiCurrentPrintJob = _ppiCurrentLoadJob;
                    _fPrintJobWaiting = TRUE;
                    break;
                }
            }
        }

        // Allow next job to load.
        _fCanLoad = TRUE;
        _ppiCurrentLoadJob = NULL;
        _pPrintDocLoading = NULL;

        break;
    }

    // Wake up the spooler thread.
    SetEvent(_hevWait);

Cleanup:

    LeaveCriticalSection();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::EnqueuePrintJob
//
//  Synopsis:   Appends a new print job to the end of the print queue.
//              Returns a positive print job id, or zero in case of an error.
//
//  Arguments:  ppiNewPrintInfo:  Pointer to PRINTINFO structure containing
//                                data pertinent to printing and print job.
//
//----------------------------------------------------------------------------

HRESULT CSpooler::EnqueuePrintJob(PRINTINFO *ppiNewPrintInfo, ULONG *pulJobId)
{
    THREADSTATE *pts = GetThreadState();
    HRESULT     hr;

    Assert(TASK_GLOBAL(g_pSpooler) == this && "Multiple spooler objects exist.");

    Assert(ppiNewPrintInfo && ppiNewPrintInfo->ppibagRootDocument &&
        "Illegal PRINTINFO structure passed to EnqueuePrintJob");

    EnterCriticalSection();

    // Every enqueued job is by definition a "root document".
    ppiNewPrintInfo->fRootDocument = TRUE;
    ppiNewPrintInfo->fCancelled = FALSE;
    ppiNewPrintInfo->fFrameChild = FALSE;
    ppiNewPrintInfo->fLoadFinishedEarly = FALSE;
    ppiNewPrintInfo->ppibagRootDocument->dwThreadId = pts->dll.idThread;

    // Assign the job a new print job id.
    ppiNewPrintInfo->ulPrintJobId = NewPrintJobId();

    // copy data from the current set of printinfo stuff to the
    // printjob...
    *(ppiNewPrintInfo->ppibagRootDocument) = _PrintInfoBag;
    ppiNewPrintInfo->ppibagRootDocument->hDevMode = 0;
    ppiNewPrintInfo->ppibagRootDocument->hDevNames = 0;

    // Allocate targetdevice and IC for the print job.  Also set print mode.
    hr = InitPrintHandles(_PrintInfoBag.hDevMode,
                          _PrintInfoBag.hDevNames,
                          &(ppiNewPrintInfo->ppibagRootDocument->ptd),
                          &(ppiNewPrintInfo->ppibagRootDocument->hic),
                          &(ppiNewPrintInfo->ppibagRootDocument->dwPrintMode),
                          (ppiNewPrintInfo->ppibagRootDocument->fConsiderBitmapFonts
                                ? (&(ppiNewPrintInfo->ppibagRootDocument->hdc)) : NULL) );
    if (hr)
    {
        Assert(!"Failed to initialize handles for printing");
        goto Error;
    }

    _PrintInfoBag.hdc = 0;              // this one is now owned by the printjob

    // Append new job to the end of the print queue.
    _dblPrintJobs.Append(ppiNewPrintInfo);

    // Wake up the spooler thread.
    _fLoadJobWaiting = TRUE;

    if (pulJobId)
    {
        *pulJobId = ppiNewPrintInfo->ulPrintJobId;
    }

    // Add a dummy job on the windows queue.
    IGNORE_HR( AddDummyJob(ppiNewPrintInfo) );

    _fRemoveTempFilesWhenEmpty = TRUE;

Error:
    LeaveCriticalSection();

    SetEvent(_hevWait);

    return hr;

}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CancelPrintJob
//
//  Synopsis:   Deletes a print job from the print queue if it was
//              enqueued by the calling thread.
//              Returns S_OK if successful, S_FALSE if not found.
//
//              If ulPrintJobId is zero Deletes all print jobs from the
//              print queue that were enqueued by the calling thread.
//
//  Arguments:  ulPrintJobId:  Unique print job id to be cancelled or zero.
//              fRecursive:    Optional. Determines cancelling of child jobs.
//
//----------------------------------------------------------------------------

HRESULT CSpooler::CancelPrintJob(ULONG ulPrintJobId, BOOL fRecursive)
{
    THREADSTATE *pts = GetThreadState();
    unsigned int nPrintJobDepth = 0, nPass;
    BOOL fCancelled = FALSE;
    PRINTINFO * ppiPrintJob;

    Assert(TASK_GLOBAL(g_pSpooler) == this && "Multiple spooler objects exist.");

    EnterCriticalSection();

    // Cancel all qualifying jobs
    for (nPass = 0 ; nPass < 2 ; nPass++)
    {
        // Hook up two queues (old jobs, pending jobs) as if they were one queue.
        ppiPrintJob = nPass ? _dblPrintJobs.First() : _dblOldPrintJobs.First();

        // Walk through this combined print queue.
        while (ppiPrintJob)
        {
            // If cancelled and not cancel all (hence cancel recursion)...
            if (fCancelled && ulPrintJobId)
            {
                // If job is a child job of print job to be recursively cancelled,
                // i.e. if job is not a root document and has a smaller depth.
                // Note that framechildren have to be made smaller by subtracting
                // 1 from their depth.
                if (!ppiPrintJob->fRootDocument &&
                    ppiPrintJob->nDepth - (ppiPrintJob->fFrameChild?1:0) < nPrintJobDepth)
                {
                    // If job currently printing, let the CPrintDoc know.
                    if (ppiPrintJob == _ppiCurrentPrintJob &&
                        _ppiCurrentPrintJob->grfPrintObjectType == POT_CPRINTDOC)
                    {
                        _PrintObjectPrinting.pPrintDoc->_fCancelled = TRUE;
                    }

                    // Mark job as cancelled.
                    ppiPrintJob->fCancelled = TRUE;
                }
                else
                {
                    // As soon as we find a job that NO LONGER qualifies for cancelling
                    // we are done due to the depth-first arrangement of print jobs
                    // in the queue.
                    goto NecessaryPrintJobsCancelled;
                }
            }
            else
            {
                // If there is a job id match, or we are cancelling all jobs (also
                // make sure that the thread id is the same).
                if ((ppiPrintJob->ulPrintJobId == ulPrintJobId || !ulPrintJobId)
                    && ppiPrintJob->ppibagRootDocument->dwThreadId == pts->dll.idThread)
                {
                    // If job currently printing, let the CPrintDoc know.
                    if (ppiPrintJob == _ppiCurrentPrintJob &&
                        _ppiCurrentPrintJob->grfPrintObjectType == POT_CPRINTDOC)
                    {
                        _PrintObjectPrinting.pPrintDoc->_fCancelled = TRUE;
                    }

                    // Mark job as cancelled.
                    ppiPrintJob->fCancelled = TRUE;

                    // Remember that we cancelled something.
                    fCancelled = TRUE;

                    // Unless we are cancelling recursively or everything, bail out.
                    if (ulPrintJobId && !fRecursive)
                    {
                        goto NecessaryPrintJobsCancelled;
                    }

                    // Remember the cancel depth of the print job.
                    nPrintJobDepth = ppiPrintJob->nDepth;
                }
            }

            // Next.
            ppiPrintJob = nPass ? _dblPrintJobs.Next(ppiPrintJob) : _dblOldPrintJobs.Next(ppiPrintJob);
        }
    }

NecessaryPrintJobsCancelled:

    LeaveCriticalSection();

    return fCancelled?S_OK:S_FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::InitiateLoading
//
//  Synopsis:   Initiates the loading of a new print job asynchronously.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

HRESULT CSpooler::InitiateLoading()
{
    HRESULT hr;
    IMoniker *pmk = NULL;
    IBindCtx *pBCtx = NULL;
    PRINTINFOBAG *pPIBag;
    BOOL fTrident;
    BOOL fLoadFromStream = FALSE;

    TCHAR   achBuff[pdlUrlLen] = TEXT("");

    Assert(!_pPrintDocLoading && "Corrupt Spooler state: Old print job hanging around.");
    Assert(_ppiCurrentLoadJob && _ppiCurrentLoadJob->ppibagRootDocument
        && "Corrupt Spooler state: Missing load job after load request.");

    pPIBag = _ppiCurrentLoadJob->ppibagRootDocument;
    _ppiCurrentLoadJob->fInitiatingLoading = TRUE;
    _ppiCurrentLoadJob->grfPrintObjectType = POT_CPRINTDOC;

    // Find out whether the passed in name is already a URL

    if (_ppiCurrentLoadJob->fTempFile ? !PathIsURL(_ppiCurrentLoadJob->cstrTempFileName)
                                      : !PathIsURL(_ppiCurrentLoadJob->cstrBaseUrl))
    {
        // so change it...
        _tcscpy(achBuff, TEXT("file://"));
    }
    // concat the original name...

    _tcscat(achBuff, _ppiCurrentLoadJob->fTempFile ?
            _ppiCurrentLoadJob->cstrTempFileName :
            _ppiCurrentLoadJob->cstrBaseUrl);

    fTrident = _ppiCurrentLoadJob->fTempFile || _ppiCurrentLoadJob->fPrintHtmlOnly;

    // If we have a OE header to load, load it first.
    if (pPIBag->fPrintOEHeader && !pPIBag->pPrintDocOEHeader)
    {
        if (pPIBag->pstmOEHeader)
        {
            fLoadFromStream = TRUE;
            fTrident = TRUE;
        }
        else
        {
            pPIBag->fPrintOEHeader = FALSE;
        }
    }

    hr = CreateURLMoniker(NULL, achBuff, &pmk);

    if (hr || !pmk)
        goto Cleanup;

    // If we don't already know that we need Trident as a print server,
    // find out whether we need Trident or another application.
    if (!fTrident)
    {
        // Find out if this doc is HTML compatible (HTML, GIF, JPG, .. anything Trident prints).
        hr = THR( IsDocHTMLCompatible(pmk, &fTrident) );

        // If job was cancelled or shutdown, bail out.
        if (_ppiCurrentLoadJob->fCancelled || _fShutdown)
        {
            hr = S_FALSE;
            goto Cleanup;
        }

        // If this test failed because we couldn't load the doc (timeout), print an error document.
        if (hr)
        {
            // Load an error document:
            hr = THR( LoadErrorDocument(&_pPrintDocLoading, _ppiCurrentLoadJob, hr) );

            if (hr)
            {
                Assert(_pPrintDocLoading == NULL);

                // If creating the document fails, we will just pretend this job
                // was cancelled.
                _ppiCurrentLoadJob->fCancelled = TRUE;

                hr = S_FALSE;
            }
            else
            {
                // Turn this document into an error printdoc.
                _ppiCurrentLoadJob->grfPrintObjectType = POT_CPRINTDOC;

                // Make sure we preserve the original Url.
                _ppiCurrentLoadJob->fOverrideDocUrl = TRUE;

                // ATTENTION:  Here we depend on a OnLoadComplete notification from CreateErrorDocument.
            }

            goto Cleanup;
        }
    }

    // Invoke the appropriate load procedure.

    if (fTrident) // trident can load and print the document
    {
        _pPrintDocLoading = new CPrintDoc;
        if (!_pPrintDocLoading)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        // let the print doc know if we're coming from an xml file
        _pPrintDocLoading->_fXML = pPIBag->fXML;  
        
        // As part of the initialization, let the PrintDoc have a copy of this job's
        // print info bag and a pointer to this spooler.
        hr = _pPrintDocLoading->DoInit(_ppiCurrentLoadJob, this);
        if (hr)
            goto Cleanup;

        if (fLoadFromStream)
        {
            hr = THR( _pPrintDocLoading->Load( pPIBag->pstmOEHeader ) );
        }
        else
        {
            hr = THR(CreateAsyncBindCtxEx(NULL, 0, NULL, NULL, &pBCtx, 0));
            if (hr)
                goto Cleanup;

            hr = THR( _pPrintDocLoading->Load( FALSE, pmk, pBCtx, STGM_READ ) );
        }

        Assert(!hr && "Problem loading Url.");

#ifdef WIN16_NEVER
        // bad hack.
        // For Win16, we assume that Load() does not do a context
        // switch.
       _ppiCurrentLoadJob->fLoadFinishedEarly = TRUE;
       _idCallbackIdLoadFinishedEarly = SP_FINISHED_LOADING_HTMLDOCUMENT;
       _pPrintObjectLoadFinishedEarly = (void *)_pPrintDocLoading;

        // No print job that could erroneously be overwritten.
        Assert(!_ppiCurrentPrintJob);

        // Process the deferred OnLoadComplete notification.
        OnLoadComplete(_pPrintObjectLoadFinishedEarly, _idCallbackIdLoadFinishedEarly);
        if (_fPrintJobWaiting)
        {
            EVENT_HANDLE hEventToRaise = 0;
            HRESULT hrLocal = InitiatePrinting(&hEventToRaise);
            Assert(!hrLocal && "Printing problem");
            _fPrintJobWaiting = FALSE;
        }
#endif // WIN16
    }
    else // non trident load / print
#ifdef WIN16
    {
        Assert(0);
    }
#else
    {
        IPrint *pPrint = NULL;

        Assert(!_pBindStatusCallback);

        _pBindStatusCallback = new CBindStatusCallback(this);
        if (!_pBindStatusCallback)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(CreateAsyncBindCtx(0, (IBindStatusCallback *) _pBindStatusCallback, NULL, &pBCtx));
        if (!OK(hr) || !pBCtx)
            goto Cleanup;

        _ppiCurrentLoadJob->grfPrintObjectType = POT_IPRINT;
        hr = pmk->BindToObject( pBCtx, NULL, IID_IPrint, (void **) &pPrint );

        if (OK(hr))
        {
            if (pPrint)
            {
                // ATTENTION:
                // This case should never happen if servers behave correctly because we received
                // an object synchronously even though we made an asynchronous request.  This
                // tends to happen when the external object is already in memory, for example
                // when a "print selected frame" request is made.
                // If it should happen, defer OnLoadComplete call.

                if (!_ppiCurrentLoadJob->fLoadFinishedEarly && _idCallbackIdLoadFinishedEarly != SP_FAILED_LOADING_EXTERNALDOCUMENT)
                {
                    // Remember to revisit the load job later and return for now.
                    _ppiCurrentLoadJob->fLoadFinishedEarly = TRUE;
                    _idCallbackIdLoadFinishedEarly = SP_FINISHED_LOADING_EXTERNALDOCUMENT;
                    _pPrintObjectLoadFinishedEarly = (void *) pPrint;
                }
                else
                {
                    // Who knows what we got?  UrlMon told us it failed loading, but returned an
                    // object anyway.  Object is unusable for printing, release it.
                    ClearInterface(&pPrint);
                }
            }
        }
        else // Try IOleCommandTarget
        {
            IOleCommandTarget *pOleCommandTarget = NULL;

            // Attempt to bind to IID_IOleCommandTarget.
            _ppiCurrentLoadJob->grfPrintObjectType = POT_IOLECOMMANDTARGET;
            hr = pmk->BindToObject( pBCtx, NULL, IID_IOleCommandTarget, (void **) &pOleCommandTarget );

            if (OK(hr))
            {
                if (pOleCommandTarget)
                {
                    // This case should never happen if servers behave correctly because we received
                    // an object synchronously even though we made an asynchronous request.  This
                    // tends to happen when the external object is already in memory, for example
                    // when a "print selected frame" request is made.
                    // If it should happen, defer OnLoadComplete call.

                    if (!_ppiCurrentLoadJob->fLoadFinishedEarly && _idCallbackIdLoadFinishedEarly != SP_FAILED_LOADING_EXTERNALDOCUMENT)
                    {
                        // Remember to revisit the load job later and return for now.
                        _ppiCurrentLoadJob->fLoadFinishedEarly = TRUE;
                        _idCallbackIdLoadFinishedEarly = SP_FINISHED_LOADING_EXTERNALDOCUMENT;
                        _pPrintObjectLoadFinishedEarly = (void *) pOleCommandTarget;
                    }
                    else
                    {
                        // Who knows what we got?  UrlMon told us it failed loading, but returned an
                        // object anyway.  Object is unusable for printing, release it.
                        ClearInterface(&pOleCommandTarget);
                    }
                }
            }
            else
            {
                Assert(!"Problem binding to non-html print object object");
                goto Cleanup;
            }
        }

        hr = S_OK;
    }
#endif //!WIN16

Cleanup:

    if (hr && _pPrintDocLoading)
    {
        _pPrintDocLoading->Release();
        _pPrintDocLoading = NULL;
    }

    ReleaseInterface(pBCtx);
    ReleaseInterface(pmk);

    // This flag HAS to be cleared in all cases at end of this function.
    _ppiCurrentLoadJob->fInitiatingLoading = FALSE;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::InitiatePrinting
//
//  Synopsis:   Synchronously prints the current print doc.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

HRESULT CSpooler::InitiatePrinting(EVENT_HANDLE *phEventToRaise)
{
    THREADSTATE *   pts = GetThreadState();
    IMoniker *      pmk = NULL;
    IBindCtx *      pBCtx = NULL;
    HRESULT         hr = S_OK;
    DVTARGETDEVICE  *ptd = NULL;

    Assert(_PrintObjectPrinting.pPrintDoc && _ppiCurrentPrintJob && "No print doc available for print job.");

    // Check if our dummy job is still alive and kicking on the Windows spooler.
    if (QueryDummyJob(_ppiCurrentPrintJob) != S_OK)
    {
        // This means the user wants our job to be cancelled.
        _ppiCurrentPrintJob->fCancelled = TRUE;
    }

    if (_ppiCurrentPrintJob->fNeedsDummyJob)
    {
        // Cancel the dummy job (will be removed completely, after we are done printing).
        IGNORE_HR( CancelDummyJob(&(_ppiCurrentPrintJob->djDummyJob)) );
    }

    // Bail out if we were cancelled.
    if (_ppiCurrentPrintJob->fCancelled)
    {
        goto Cleanup;
    }

    if (!_ppiCurrentPrintJob->ppibagRootDocument->hdc)
    {
        hr = THR( FormsPrint(_ppiCurrentPrintJob->ppibagRootDocument,0,_PrintObjectPrinting.pPrintDoc,TRUE) );
        if (hr)
            goto Cleanup;
    }

    ptd = _ppiCurrentPrintJob->ppibagRootDocument->ptd;
    Assert(ptd && "No target device available");

    if (!ptd)
    {
        goto Cleanup;
    }

    // Set the print mode on the thread state.
    if (pts)
    {
        pts->dwPrintMode = _ppiCurrentPrintJob->ppibagRootDocument->dwPrintMode;
    }

    // Actually print the document.
    switch (_ppiCurrentPrintJob->grfPrintObjectType)
    {
    case POT_CPRINTDOC:

        Assert(_PrintObjectPrinting.pPrintDoc);

        // NOTE: InitForPrint has already been called in CPrintDoc::DoInit

        _PrintObjectPrinting.pPrintDoc->_dci.HimetricFromDevice(
            & _PrintObjectPrinting.pPrintDoc->_sizel, _PrintObjectPrinting.pPrintDoc->_dci._sizeDst);
        _PrintObjectPrinting.pPrintDoc->_sizel.cy *= 100;
        _PrintObjectPrinting.pPrintDoc->_paryPrintPage = NULL;

        if (_ppiCurrentPrintJob->fOverrideDocUrl)
        {
            _PrintObjectPrinting.pPrintDoc->_cstrUrl.Set(_ppiCurrentPrintJob->cstrBaseUrl);
        }

        hr = THR(_PrintObjectPrinting.pPrintDoc->Print());
        _ppiCurrentPrintJob->fCancelled = _PrintObjectPrinting.pPrintDoc->_fCancelled;

        break;

    case POT_IPRINT:
        // Only print IPrints if we haven't already printed all IPrint docs from the UI thread (CDoc::DoPrint).
        if (!_ppiCurrentPrintJob->fExtJobsComplete)
        {
            LONG lPages;
            LONG lFirstPage = _ppiCurrentPrintJob->ppibagRootDocument->fAllPages ? 1 : _ppiCurrentPrintJob->ppibagRootDocument->nFromPage ,
                 lLastPage  = _ppiCurrentPrintJob->ppibagRootDocument->fAllPages ? PAGESET_TOLASTPAGE : _ppiCurrentPrintJob->ppibagRootDocument->nToPage;
            PAGESET *pPageSet;
            IPrint *pPrint = _PrintObjectPrinting.pPrint;
            DVTARGETDEVICE  *ptdOld = NULL;

            if (!pPrint)
            {
                Assert(!"BindToObject(IPrint) succeeded, but returned NULL pointer");
                hr = E_FAIL;
                break;
            }

            pPageSet = (PAGESET *)CoTaskMemAlloc(sizeof(PAGESET));

            if (!pPageSet)
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            pPageSet->cbStruct = sizeof(PAGESET);
            pPageSet->fOddPages = TRUE;
            pPageSet->fEvenPages = TRUE;
            pPageSet->cPageRange = 1;
            pPageSet->rgPages[0].nFromPage = lFirstPage;
            pPageSet->rgPages[0].nToPage = lLastPage;

    #ifndef WIN16
            if (!g_fUnicodePlatform)
            {
                // IMPORTANT: We are not internally converting the DEVMODE structure back and forth
                // from ASCII to Unicode on non-Unicode platforms anymore because we are not touching
                // the two strings or any other member.  Converting the DEVMODE structure can
                // be tricky because of potential and common discrepancies between the
                // value of the dmSize member and sizeof(DEVMODE).  (25155)

                // As a result, we pass around DEVMODEW structures on unicode platforms and DEVMODEA
                // structures on non-unicode platforms.  Members of IPrint, however, always
                // expect DEVMODEW pointers.  Here we manually create a new target device structure
                // containing a DEVMODEW copy of the DEVMODEA original currently in ptd.

                ptdOld = ptd;
                ptd = TargetDeviceWFromTargetDeviceA(ptdOld);
            }
    #endif // ndef WIN16

    #if DBG==1
            if (IsTagEnabled(tagIPrintBotherUser))
            {
                hr = pPrint->Print(PRINTFLAG_MAYBOTHERUSER | PRINTFLAG_PROMPTUSER |
                    PRINTFLAG_USERMAYCHANGEPRINTER | PRINTFLAG_RECOMPOSETODEVICE,
                    &ptd, &pPageSet, NULL, NULL, lFirstPage, &lPages, &lLastPage);
            }
            else
    #endif // DBG == 1
            hr = pPrint->Print(_ppiCurrentPrintJob->ppibagRootDocument->fPrintToFile ?
                                  PRINTFLAG_MAYBOTHERUSER | PRINTFLAG_PRINTTOFILE :
                                  0,
                               &ptd,
                               &pPageSet,
                               NULL,
                               NULL,
                               lFirstPage,
                               &lPages,
                               &lLastPage);

    #ifndef WIN16
            if (!g_fUnicodePlatform)
    #endif // ndef WIN16
            {
                // Free the temporary DEVMODEW copy of ptd.
                if (ptd)
                    CoTaskMemFree(ptd);

                ptd = ptdOld;
            }
            else
            {
                _ppiCurrentPrintJob->ppibagRootDocument->ptd = ptd; // cache the new value
            }

            CoTaskMemFree(pPageSet);

            Assert(!hr && "Printing failed");
        }
        break;

    case POT_IOLECOMMANDTARGET:
    {
        IOleCommandTarget * pOleCommandTarget = _PrintObjectPrinting.pOleCommandTarget;

        if (!pOleCommandTarget)
        {
            Assert(!"BindToObject(IOleCommandTarget) succeeded, but returned NULL pointer");
            hr = E_FAIL;
            break;
        }

#if DBG==1
        OLECMD              olecmd;
        OLECMDTEXT          olecmdtext;

        olecmd.cmdID = OLECMDID_PRINT;
        olecmd.cmdf = 0;
        olecmdtext.cmdtextf = OLECMDTEXTF_NONE;
        olecmdtext.cwActual = olecmdtext.cwBuf = 0;

        // Find out whether server supports printing via IOleCommandTarget.
        hr = THR( pOleCommandTarget->QueryStatus(NULL, 1, &olecmd, &olecmdtext) );

        if (!OK(hr) || !(olecmd.cmdf & OLECMDF_SUPPORTED))
        {
            Assert(!"External server does not support printing via IOleCommandTarget");
        }

        if (IsTagEnabled(tagIPrintBotherUser))
        {
            hr = THR( pOleCommandTarget->Exec(NULL, OLECMDID_PRINT, OLECMDEXECOPT_PROMPTUSER, NULL, NULL) );
        }
        else
#endif // DBG == 1
        // !!! don't insert commands here (note else clause within #if DBG==1 !!!
        // Actually print document via IOleCommandTarget.
        hr = THR( pOleCommandTarget->Exec(NULL, OLECMDID_PRINT, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL) );
    }
        break;
    } // end switch

Cleanup:

    // Release the print doc.
    switch (_ppiCurrentPrintJob->grfPrintObjectType)
    {
    case POT_CPRINTDOC:
        _PrintObjectPrinting.pPrintDoc->Release();
        _PrintObjectPrinting.pPrintDoc = NULL;
        break;

    case POT_IPRINT:
        ReleaseInterface(_PrintObjectPrinting.pPrint);
        _PrintObjectPrinting.pPrint = NULL;
        break;

    case POT_IOLECOMMANDTARGET:
        ReleaseInterface(_PrintObjectPrinting.pOleCommandTarget);
        _PrintObjectPrinting.pOleCommandTarget = NULL;
        break;
    }

    *phEventToRaise = _ppiCurrentPrintJob->hEvent;

    // Remove the dummy job completely.
    IGNORE_HR( RemoveDummyJob(_ppiCurrentPrintJob) );

    // If this is the last job or the last job of a recursive request,
    // kill the print device context.
    IGNORE_HR(WrapUpPrintJob(_ppiCurrentPrintJob));

    _ppiCurrentPrintJob = NULL;

    // Wake up the spooler thread.
    SetEvent(_hevWait);

    ReleaseInterface(pmk);
    ReleaseInterface(pBCtx);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::IsDocHTMLCompatible
//
//  Synopsis:   Synchronously determines whether Trident can print this
//              document (HTML, GIF, ..).
//
//              This function only applies to the document currently being
//              loaded.
//
//  Arguments:  pmk [in]: Moniker of document to be identified.
//
//----------------------------------------------------------------------------

HRESULT CSpooler::IsDocHTMLCompatible(IMoniker *pmk, BOOL *pfTrident)
{
    IBindCtx *              pBCtx = NULL;
    IHTMLDocument *         pDoc = NULL;
    CBindStatusCallback *   pBindStatusCallback = NULL;
    EVENT_HANDLE            hEvent = 0;
    ULONG                   ulTimerInSeconds = 0;
    BOOL                    fTimeOut = FALSE;
    HRESULT                 hr = S_OK;

    // If anything goes wrong here, assume we are dealing with an HTML compatible doc.
    *pfTrident = TRUE;

    // Insist on a loading print job.
    if (!_ppiCurrentLoadJob)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Create the wakeup event
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvent)
    {
        Assert(!"Couldn't create an event for print type investigation");
        goto Cleanup;
    }

    // Create a CBindStatusCallback object.  Set it up for print type investigation mode.
    pBindStatusCallback = new CBindStatusCallback(NULL, hEvent, TRUE);
    if (!pBindStatusCallback)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(CreateAsyncBindCtx(0, (IBindStatusCallback *) pBindStatusCallback, NULL, &pBCtx));
    if (!OK(hr) || !pBCtx)
    {
        goto Cleanup;
    }

    // We use the existence of IHTMLDocument as a sign that Trident can
    // print this document.
    hr = pmk->BindToObject( pBCtx, NULL, IID_IHTMLDocument, (void **) &pDoc );

    // Wait for the callback to occur.
    if (hr != E_NOINTERFACE &&
        hr != E_NOTIMPL &&
        hr != INET_E_CANNOT_INSTANTIATE_OBJECT &&
        hr != E_ACCESSDENIED)
    {
        while (!fTimeOut && !_fShutdown && !_ppiCurrentLoadJob->fCancelled)
        {
            MSG msg;
            DWORD dw;

            // Process all messages in the message queue.
#ifdef WIN16
            ThreadYield();
#else               
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }                         
#endif                

            // Wait for the event to be fired.
            dw = WaitForSingleObject(hEvent, 1000);

            // If the event was fired, we are done.
            if (dw != WAIT_TIMEOUT)
            {
                break;
            }

            // Check if the loading print job has been cancelled.
            _ppiCurrentLoadJob->fCancelled |= QueryDummyJob(_ppiCurrentLoadJob) != S_OK;

            // Wait at most 60 seconds.
            fTimeOut = ulTimerInSeconds++ >= IS_HTML_TIMEOUT;
        }

        // Ask the bind status callback what it found out about the object's type.
        *pfTrident = fTimeOut || pBindStatusCallback->IsObjectHTMLCompatible();

        hr = fTimeOut
            ? INET_E_CANNOT_INSTANTIATE_OBJECT
            : pBindStatusCallback->LoadFailed()
                ? INET_E_RESOURCE_NOT_FOUND
                : S_OK;
    }
    else
    {
        // We received a nope synchronously.
        *pfTrident = FALSE;

        hr = S_OK;
    }

    // Revoke our BindStatusCallback object.
    RevokeBindStatusCallback(pBCtx, pBindStatusCallback);

Cleanup:

    if (hEvent)
        CloseEvent(hEvent);

    if (pDoc)
        pDoc->Release();

    if (pBindStatusCallback)
        pBindStatusCallback->Release();

    ReleaseInterface(pBCtx);

    RRETURN2(hr, INET_E_CANNOT_INSTANTIATE_OBJECT, INET_E_RESOURCE_NOT_FOUND);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::WrapUpPrintJob
//
//  Synopsis:   Destroys the device context and the old queue whenever
//              a (single or recursive) print jobs completes or is cancelled.
//
//  Arguments:  None
//
//----------------------------------------------------------------------------

HRESULT CSpooler::WrapUpPrintJob(PRINTINFO *ppiPrintJob)
{
    // always do this, the tempfiles can be deleted now
    if (ppiPrintJob->fTempFile && ppiPrintJob->cstrTempFileName.Length())
    {
        DeleteFile(ppiPrintJob->cstrTempFileName);
    }

    if (_dblPrintJobs.IsEmpty() || _dblPrintJobs.First()->fRootDocument)
    {
        // Close device context.
        if (ppiPrintJob->ppibagRootDocument->hdc)
        {
            DeleteDC(ppiPrintJob->ppibagRootDocument->hdc);
            ppiPrintJob->ppibagRootDocument->hdc = 0;
        }

        // Delete targetdevice and IC.
        DeinitPrintHandles(
            ppiPrintJob->ppibagRootDocument->ptd,
            ppiPrintJob->ppibagRootDocument->hic);

        EmptyQueue(&_dblOldPrintJobs);
    }

    UpdateStatusBar();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::WrapUpPrintSpooler
//
//  Synopsis:   A helper function that cleans up the spooler as best as it
//              can when the spooler is shut down while there are still
//              jobs pending.
//
//----------------------------------------------------------------------------

HRESULT CSpooler::WrapUpPrintSpooler()
{
    EnterCriticalSection();

    // 1. Cancel download
    if (_ppiCurrentLoadJob)
    {
        IGNORE_HR( CancelDownload() );
    }

    // 2a. Remove current dummy jobs.
    PRINTINFO *ppiJobWithDummy = _dblPrintJobs.First();
    while (ppiJobWithDummy)
    {
        // Remove the dummy job from the windows queue.
        IGNORE_HR( RemoveDummyJob(ppiJobWithDummy, NULL, RDJ_FORCE_REMOVAL_FROM_QUEUE) );

        ppiJobWithDummy = _dblPrintJobs.Next(ppiJobWithDummy);
    }

    // 2b. Remove expired dummy jobs remaining on dummy jobs list.
    while (!_dblUnremovedDummyJobs.IsEmpty())
    {
        // Remove the dummy job from the windows queue.
         HRESULT hr = THR(RemoveDummyJob(NULL, _dblUnremovedDummyJobs.First(), RDJ_FORCE_REMOVAL_FROM_QUEUE));

         if (hr)
         {
            Assert(!"Dummy job wasn't really removed.  Bailing out to avoid infinite loop.  Expect memory leaks.");

            // Bail out to avoid infinite loop.
            break;
         }
    }

    // 3. Deallocate print jobs
    EmptyQueue(&_dblPrintJobs);
    EmptyQueue(&_dblOldPrintJobs);

    // 4. Delete frame tempfiles
    DeleteTempFiles(TRUE);

    LeaveCriticalSection();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::EmptyQueue
//
//  Synopsis:   A helper function that empties and deallocates a queue.
//              Attention every root documents property bag is deallocated
//              also.
//
//  Arguments:  pdblQueue:  The print queue to empty.
//
//----------------------------------------------------------------------------

void CSpooler::EmptyQueue(PrintQueue *pdblQueue)
{
    // Empty the queue and deallocate all PRINTINFOs as well as
    // PRINTINFOBAGs owned by their root documents.  Careful!!!

    if (!pdblQueue)
    {
        Assert(!"No print queue passed to EmptyQueue");
        return;
    }

    while (!pdblQueue->IsEmpty())
    {
        PRINTINFO *ppiTemp = pdblQueue->First();

        pdblQueue->Remove(ppiTemp);

        if (ppiTemp->fRootDocument && ppiTemp->ppibagRootDocument)
        {
            // Release OE mail header print doc and IStream owned by PIBag.
            if (ppiTemp->ppibagRootDocument->fPrintOEHeader)
            {
                if (ppiTemp->ppibagRootDocument->pPrintDocOEHeader)
                    ppiTemp->ppibagRootDocument->pPrintDocOEHeader->Release();

                ReleaseInterface(ppiTemp->ppibagRootDocument->pstmOEHeader);
            }

            delete ppiTemp->ppibagRootDocument;
        }

        delete ppiTemp;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::DeleteTempFiles
//
//  Synopsis:   A helper function that deletes all tempfiles created for nested
//              i/frame documents.
//
//  Arguments:  fShutdown - when we are shutting down, force deletion.
//
//----------------------------------------------------------------------------

void CSpooler::DeleteTempFiles(BOOL fShutdown)
{
    int iCount, cSize;

    if (!fShutdown && !_fRemoveTempFilesWhenEmpty)
        return;

    if (!fShutdown)
        EnterCriticalSection();

    if (fShutdown || (_fRemoveTempFilesWhenEmpty && IsEmpty()))
    {
        cSize = _aryFrameTempFiles.Size();

        for (iCount = 0 ; iCount < cSize ; iCount++)
        {
            if (_aryFrameTempFiles[iCount])
            {
                if (_aryFrameTempFiles[iCount]->Length())
                    DeleteFile(*(_aryFrameTempFiles[iCount]));
                
                delete _aryFrameTempFiles[iCount];
                _aryFrameTempFiles[iCount] = NULL;
            }
        }

        _aryFrameTempFiles.DeleteAll();
        _fRemoveTempFilesWhenEmpty = FALSE;
    }

    if (!fShutdown)
        LeaveCriticalSection();
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::DuplicatePrintJob
//
//  Synopsis:   Checks whether (1) a URL has already been printed (within a
//              recursive print operation) or (2) if the URL is already on the
//              print queue (within the same recursive print job).
//
//  Arguments:  achNewUrl:  The URL to be checked.
//
//----------------------------------------------------------------------------

BOOL CSpooler::DuplicatePrintJob(TCHAR *achNewUrl)
{
    BOOL fPrinted = FALSE;
    PRINTINFO *ppiOldJob = _dblOldPrintJobs.First();

    // Scan jobs already printed.
    while (ppiOldJob)
    {
        // IMPORTANT (27739): we are case-sensitive.  Same story in
        // CPrintDoc::AfterLoadComplete().
        if (!StrCmpC(achNewUrl, ppiOldJob->cstrBaseUrl))
        {
            fPrinted = TRUE;
            break;
        }

        ppiOldJob = _dblOldPrintJobs.Next(ppiOldJob);
    }

    // Also scan jobs already queued but not yet printed.
    if (!fPrinted)
    {
        ppiOldJob = _dblPrintJobs.First();
        while (ppiOldJob)
        {
            if (ppiOldJob->fRootDocument)
            {
                // Once we reach a whole new print job, we no longer
                // care about matches.
                break;
            }

            if (!StrCmpC(achNewUrl, ppiOldJob->cstrBaseUrl))
            {
                fPrinted = TRUE;
                break;
            }

            ppiOldJob = _dblPrintJobs.Next(ppiOldJob);
        }
    }

    return fPrinted;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::PrependChildURLsOfLoadDoc
//
//  Synopsis:   Given the print doc currently loading, this function
//              prepends either all contained links or all contained frames (URLs)
//              to the print queue.
//
//  Arguments:  fLinks: Prepend links as opposed to frames.
//              achAlternateUrl: Alternate document to print - overrides everything else
//
//----------------------------------------------------------------------------

HRESULT CSpooler::PrependChildURLsOfLoadDoc(BOOL fLinks, TCHAR *achAlternateUrl)
{
    CURLAry             aryURLs;
    CURLAry             aryPrettyURLs;
    CPrintInfoFlagsAry  aryPrintInfos;
    BOOL                fUsePrintInfoFlagsArray = FALSE;
    BOOL                fUsePrettyURLArray = FALSE;
    LONG                lUrlCount;
    HRESULT             hr = E_FAIL;

    // 1. Obtain an array of embedded Urls.
    if (achAlternateUrl)
    {
        // Treat alternate url like a link.
        CStr *pStrURL;

        Assert(_tcslen(achAlternateUrl) > 0);
        Assert(fLinks && "By convention alternate urls go through the links path");

        hr = aryURLs.AppendIndirect(NULL, &pStrURL);
        if (!hr)
        {
            hr = THR(pStrURL->Set(achAlternateUrl));
        }

        Assert(aryURLs.Size() == 1);
    }
    else if (fLinks)
    {
        // Only print linked documents if ratings are not enabled.
        if (S_OK != AreRatingsEnabled())
        {
            hr = THR(_pPrintDocLoading->EnumContainedURLs(&aryURLs, &aryPrettyURLs));
        }
    }
    else
    {
        hr = THR(_pPrintDocLoading->EnumFrameURLs(&aryURLs, (CURLAry *)&aryPrettyURLs, &aryPrintInfos));

        // Only use the print info and pretty url arrays if the sizes match
        // to avoid using misaligned arrays.
        fUsePrintInfoFlagsArray = !hr && (aryURLs.Size() == aryPrintInfos.Size());
        fUsePrettyURLArray = !hr && (aryURLs.Size() == aryPrettyURLs.Size());

        Assert(fUsePrintInfoFlagsArray && fUsePrettyURLArray && "PrependChildURLsOfLoadDoc:  Misaligned arrays");
    }

    // 2. Add new Urls to queue.
    if (!hr)
    {
        ULONG ulLastPrintJobId = 0;

        for (lUrlCount = aryURLs.Size()-1 ; lUrlCount >= 0 ; lUrlCount--)
        {
            TCHAR achUrl[pdlUrlLen];
            DWORD cchUrl;

            // Obtain absolute Url.
            if (FAILED(CoInternetCombineUrl(_ppiCurrentLoadJob->cstrBaseUrl,
                                aryURLs[lUrlCount],
                                URL_ESCAPE_SPACES_ONLY | URL_BROWSER_MODE,
                                achUrl,
                                ARRAY_SIZE(achUrl),
                                &cchUrl,
                                0)))
            {
                // Skip problematic Urls.
                continue;
            }

            // If we are dealing with links, get rid of bookmarks and skip
            // mailto links.
            if (fLinks)
            {
                LPTSTR pch = (LPTSTR) UrlGetLocation(achUrl);
                UINT uProtocol;

                // If we have a bookmark, null out the # to clip of bookmark.
                Assert(!pch || *pch == _T('#'));
                if ( pch && *pch == _T('#') )
                    *pch = 0;

                // Skip mailto links.
                uProtocol = GetUrlScheme(achUrl);
                if (URL_SCHEME_MAILTO == uProtocol ||
                    URL_SCHEME_TELNET == uProtocol ||
                    URL_SCHEME_VBSCRIPT == uProtocol || URL_SCHEME_JAVASCRIPT == uProtocol ||
                    URL_SCHEME_NEWS == uProtocol || URL_SCHEME_SNEWS == uProtocol)
                {
                    // If not, skip Url.
                    continue;
                }
            }
            else
            {
               // Skip about frames.
               UINT uProtocol = GetUrlScheme(achUrl);
               if (URL_SCHEME_ABOUT == uProtocol)
               {
                   continue;
               }
            }

            // Prepend every new Url to the Print Queue.
            if (!DuplicatePrintJob(achUrl))
            {
                PRINTINFO *ppiNextJob;

                hr = THR(CreatePrintInfo(&ppiNextJob, _ppiCurrentLoadJob->ppibagRootDocument));
                if (hr)
                {
                    goto Cleanup;
                }

                // Copy print info.
                ppiNextJob->cstrBaseUrl.Set(achUrl);
                ppiNextJob->fFrameChild = !fLinks;
                ppiNextJob->nDepth = _ppiCurrentLoadJob->nDepth - ((fLinks && !achAlternateUrl)?1:0);
                ppiNextJob->fAlternateDoc = achAlternateUrl != NULL;
                ppiNextJob->fPrintHtmlOnly = fUsePrintInfoFlagsArray && !!(aryPrintInfos[lUrlCount] & PIF_HTMLDOCUMENT);
                ppiNextJob->fDontRunScripts = ppiNextJob->fPrintHtmlOnly && (_ppiCurrentLoadJob->fTempFile || _ppiCurrentLoadJob->fDontRunScripts);
                ppiNextJob->fUsePrettyUrl = fUsePrettyURLArray && !!(aryPrintInfos[lUrlCount] & PIF_USEPRETTYURL);
                if (ppiNextJob->fUsePrettyUrl) ppiNextJob->cstrPrettyUrl.Set(aryPrettyURLs[lUrlCount]);
                ppiNextJob->fExtJobsComplete = !fLinks && _ppiCurrentLoadJob->fExtJobsComplete;
                ppiNextJob->fNeedsDummyJob = (fLinks && !achAlternateUrl) || _ppiCurrentLoadJob->fNeedsDummyJob;
                ppiNextJob->ulPrintJobId = NewPrintJobId();

                if (!ulLastPrintJobId)
                {
                    // Remember last Job enqueued.
                    ulLastPrintJobId = ppiNextJob->ulPrintJobId;
                }

                if (achAlternateUrl)
                {
                    // Alternate jobs inherit dummy jobs.
                    ppiNextJob->djDummyJob = _ppiCurrentLoadJob->djDummyJob;

                    // There can only be one.
                    Assert(aryURLs.Size() == 1);
                }

                // prepend job to the queue
                _dblPrintJobs.Prepend(ppiNextJob);

                _fLoadJobWaiting = TRUE;
            }
        }

        if (achAlternateUrl)
        {
            // If we were supposed to enqueue an alternate url, but that fell through
            // because of the url type (mailto, telnet, etc.), let the caller know.
            if (!ulLastPrintJobId)
            {
                hr = S_FALSE;
            }
        }
        else if (ulLastPrintJobId)
        {
            // Add dummy jobs for all print jobs just prepended (except for alternate urls).
            PRINTINFO *ppiJobInserted = _dblPrintJobs.First();
            while (ppiJobInserted)
            {
                // Add a dummy job on the windows queue.
                IGNORE_HR( AddDummyJob(ppiJobInserted) );

                // When we reach the last job prepended, we are done.
                if (ulLastPrintJobId == ppiJobInserted->ulPrintJobId)
                    break;

                ppiJobInserted = _dblPrintJobs.Next(ppiJobInserted);
            }
        }
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CreateErrorDocument
//
//  Synopsis:   Returns a CPrintDoc corresponding to the error code passed in.
//
//  Arguments:  ppPrintDoc [out]: A pointer to a pointer to a printdoc created, or
//                                a pointer to NULL if object creation failed.
//              hrError [in]:     The error according to the Printdoc returned.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::LoadErrorDocument(CPrintDoc **ppPrintDoc, PRINTINFO *pPIFailedJob, HRESULT hrError)
{
    HRESULT     hr = S_OK;
    IMoniker *  pmk = NULL;
    TCHAR    *  pchError;

    Assert(pPIFailedJob && pPIFailedJob->ppibagRootDocument);

    if (!ppPrintDoc)
    {
        hr = E_POINTER;
        goto Error;
    }

    *ppPrintDoc = new CPrintDoc;
    if (!*ppPrintDoc)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    }

    // As part of the initialization, let the PrintDoc have a copy of this job's
    // print info bag and a pointer to this spooler.
    hr = (*ppPrintDoc)->DoInit(pPIFailedJob, this);
    if (hr)
        goto Error;

    // Determine which error message to print.
    switch (hrError)
    {
    case INET_E_RESOURCE_NOT_FOUND:
        pchError = _T("printnf.htm");
        break;
    case INET_E_CANNOT_INSTANTIATE_OBJECT:
        pchError = _T("printerr.htm");
        break;
    default:
        pchError = _T("printunk.htm");
    }

    hr = THR(CreateResourceMoniker(
                GetResourceHInst(),
                pchError,
                &pmk));
    if (hr)
        goto Error;

    hr = THR((*ppPrintDoc)->Load(TRUE, pmk, NULL, 0));
    if (hr)
        goto Error;

Cleanup:
    ReleaseInterface(pmk);
    RRETURN(hr);

Error:

    Assert(!"CreateErrorDocument was not able to create a proper print doc");

    if (ppPrintDoc && *ppPrintDoc)
    {
        (*ppPrintDoc)->Release();
        *ppPrintDoc = NULL;
    }

    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//              Global Spooler functions
//
//
//  Member:     IsSpooler
//
//  Synopsis:   checks if there is a thread level spooler...
//              returns S_OK if the spooler is there....
//
//
//----------------------------------------------------------------------------
HRESULT IsSpooler(void)
{
    THREADSTATE *   pts = GetThreadState();
    HRESULT     hr = S_OK;

    if (pts->pSpooler == NULL)
    {
        // both writing and reading g_pDwnMan need to be in cs
        LOCK_SECTION(g_csSpooler);

        if (TASK_GLOBAL(g_pSpooler) == NULL)
        {
            hr = S_FALSE;
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//              Global Spooler functions
//
//
//  Member:     GetSpooler
//
//  Synopsis:   Retrieves a thread-level copy of the global spooler pointer.
//              Creates the spooler, in case it doesn't exist.
//
//  Arguments:  ppSpooler [OUT]:  pointer to the spooler.
//
//----------------------------------------------------------------------------

HRESULT GetSpooler(CSpooler ** ppSpooler)
{
    THREADSTATE *   pts = GetThreadState();
    USE_FAST_TASK_GLOBALS;
    HRESULT         hr;

    if (pts->pSpooler == NULL)
    {
        // both writing and reading g_pDwnMan need to be in cs
        LOCK_SECTION(g_csSpooler);

        if (FAST_TASK_GLOBAL(g_pSpooler) == NULL)
        {
            FAST_TASK_GLOBAL(g_pSpooler) = new CSpooler;

            if (FAST_TASK_GLOBAL(g_pSpooler) == NULL)
                RRETURN(E_OUTOFMEMORY);

            hr = THR(FAST_TASK_GLOBAL(g_pSpooler)->Launch(TRUE));

            if (hr)
            {
                FAST_TASK_GLOBAL(g_pSpooler)->Release();
                FAST_TASK_GLOBAL(g_pSpooler) = NULL;
                RRETURN(hr);
            }

            Assert(FAST_TASK_GLOBAL(g_pSpooler)->GetThreadId() != GetCurrentThreadId());

            TraceTag((tagSpooler, "Spooler created (_ulRefs = 1)"));

            // The first reference (via constructor) is given to the
            // thread which created the spooler manager.  No AddRef
            // required here.

            pts->pSpooler = FAST_TASK_GLOBAL(g_pSpooler);
        }
        else if (FAST_TASK_GLOBAL(g_pSpooler)->GetThreadId() != GetCurrentThreadId())
        {
            TraceTag((tagSpooler, "Spooler attached (_ulRefs = %ld)",
                FAST_TASK_GLOBAL(g_pSpooler)->GetRefs() + 1));

            pts->pSpooler = FAST_TASK_GLOBAL(g_pSpooler);
            FAST_TASK_GLOBAL(g_pSpooler)->AddRef();
        }
    }

    Assert((!pts->pSpooler && FAST_TASK_GLOBAL(g_pSpooler)) || pts->pSpooler == FAST_TASK_GLOBAL(g_pSpooler));

    *ppSpooler = FAST_TASK_GLOBAL(g_pSpooler);

#ifdef UNIX
    if(!(*ppSpooler) )
    {
        RRETURN(E_FAIL);
    }
#endif
    RRETURN(S_OK);
}


//+---------------------------------------------------------------------------
//
//  Member:     DeinitSpooler
//
//  Synopsis:   Deinitialize the spooler.
//
//  Arguments:  pts:  Thread pointer
//
//----------------------------------------------------------------------------

void DeinitSpooler(THREADSTATE * pts)
{
    if (pts->pSpooler)
    {
        TraceTag((tagSpooler, "Spooler detached (_ulRefs = %ld)",
            pts->pSpooler->GetRefs() - 1));

        pts->pSpooler->Release();
        pts->pSpooler = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CreatePrintInfo
//
//  Synopsis:   init's a printinfo structure
//
//  Arguments:  pppiNewPrintJob -> points to new structure pointer
//
//----------------------------------------------------------------------------

HRESULT CreatePrintInfo(PRINTINFO **pppiNewPrintJob, PRINTINFOBAG *ppibagRootDocument)
{
    HRESULT hr = S_OK;

    Assert(pppiNewPrintJob && "Null pointer");

    *pppiNewPrintJob = new PRINTINFO;
    if (!(*pppiNewPrintJob))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    memset(*pppiNewPrintJob, 0, sizeof(PRINTINFO));

    if (ppibagRootDocument)
    {
        (*pppiNewPrintJob)->ppibagRootDocument = ppibagRootDocument;
    }
    else
    {
        (*pppiNewPrintJob)->ppibagRootDocument = new PRINTINFOBAG;
        if (!(*pppiNewPrintJob)->ppibagRootDocument)
        {
            delete *pppiNewPrintJob;
            *pppiNewPrintJob = NULL;

            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        memset((*pppiNewPrintJob)->ppibagRootDocument, 0, sizeof(PRINTINFOBAG));
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//
//              Spooler::CBindStatusCallback::IBindStatusCallback functions
//
//
//  Member:     CSpooler::CBindStatusCallback::QueryInterface
//
//  Synopsis:   We support the following interfaces:
//
//                  IUnknown
//                  IBindStatusCallback
//
//  Arguments:  [iid]
//              [ppv]
//
//  Returns:    HRESULT (STDMETHOD)
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSpooler::CBindStatusCallback::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown ||
        iid == IID_IBindStatusCallback)
    {
        *ppv = (IPropertyPage *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG)
CSpooler::CBindStatusCallback::AddRef( )
{
    return ++_ulRefs;
}


STDMETHODIMP_(ULONG)
CSpooler::CBindStatusCallback::Release( )
{
    ULONG ulRefs = --_ulRefs;

    if (ulRefs == 0)
    {
        delete this;
    }
    return ulRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnStartBinding, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker when binding is starting.
//              We keep a reference to the IBinding interface so that we
//              can abort (cancel) it if necessary.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnStartBinding(DWORD grfBSCOption, IBinding *pbinding)
{
    if (_pSpooler)
    {
        // process _ppiCurrentLoadJob->fCancel
        if (_pSpooler->_ppiCurrentLoadJob && _pSpooler->_ppiCurrentLoadJob->fCancelled)
        {
            IGNORE_HR(pbinding->Abort());
        }
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::GetPriority, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker.  May be called at any
//              time during the binding operation if the moniker needs to
//              make new priority decisions.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::GetPriority(LONG *pnPriority)
{
    *pnPriority = NORMAL_PRIORITY_CLASS;
    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnLowResource, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker when it detects low
//              resources.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnLowResource(DWORD reserved)
{
    Assert(!"IBindStatusCallback::OnLowResource call");

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnProgress, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker as progress is made.
//              Clear POST data in case of redirect (NS/IE3 compat)
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
    ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnStopBinding, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker to indicate the end of
//              the bind operation.  This method is always called, whether
//              the bind operation succeeded, failed, or was aborted by the
//              client.  At this point, the IBinding interface received
//              during the OnStartBinding callback must be released.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnStopBinding(HRESULT hresult, LPCWSTR szError)
{
    if (!OK(hresult))
    {
        if (_fBindModeInvestigateObjectType)
        {
            // The only error codes that we allow as a sign that this is no Trident
            // object are INET_E_CANNOT_INSTANTIATE_OBJECT (0x800c0010),
            //            E_NOINTERFACE (0x80004002), or
            //            E_NOTIMPL (0x80004001). or E_ACCESSDENIED (0x80070005)
            _fObjectIsHTMLCompatible = hresult != INET_E_CANNOT_INSTANTIATE_OBJECT &&
                                       hresult != E_NOINTERFACE &&
                                       hresult != E_NOTIMPL &&
                                       hresult != E_ACCESSDENIED;            

            //  We get back connection error codes here, not on the BindObject call as the original code seemed to assume.
            //  We seem get an INET_E_RESOURCE_NOT_FOUND when the connection utterly failed.
            //    Added INET_E_CONNECTION_TIMEOUT to the load failed conditions for good measure.  <g>
            //  We might (in fact, probably do) want to do this for *any* error code here, but for now we'll just do these.
            _fLoadFailed =      hresult == INET_E_RESOURCE_NOT_FOUND
                            ||  hresult == INET_E_CONNECTION_TIMEOUT;


            if (_hEvent)
            {
                SetEvent(_hEvent);
            }
        }
        else if (_pSpooler)
        {
            // something went wrong - notify spooler
            _pSpooler->OnLoadComplete(NULL, SP_FAILED_LOADING_EXTERNALDOCUMENT);
        }
    }

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::GetBindInfo, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker during the BindToStorage
//              method to obtain the bind information for the bind operation.
//
//              Shamelessly stolen from IE3.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::GetBindInfo(DWORD *pgrfBINDF, BINDINFO *pbindinfo)
{
    DWORD dwBindf = BINDF_ASYNCHRONOUS;

    Assert(pbindinfo);
    Assert(pgrfBINDF);
    if (!pbindinfo || !pgrfBINDF)
        return E_POINTER;

    if (!pbindinfo->cbSize)
        return E_INVALIDARG;

    DWORD cbSize = pbindinfo->cbSize;
    memset(pbindinfo, 0, cbSize );
    pbindinfo->cbSize = cbSize;

    // default method is GET.  Valid ones are _GET, _PUT, _POST, _CUSTOM
    pbindinfo->dwBindVerb = BINDVERB_GET;

    *pgrfBINDF = dwBindf;

    return(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnDataAvailable, IBindStatusCallback
//
//  Synopsis:   Callback from asynchronous moniker to notify the client
//              that data is available on the stream.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnDataAvailable(DWORD grfBSCF, DWORD dwSize,
    FORMATETC * pformatetc, STGMEDIUM * pstgmed)
{
    RRETURN(E_NOTIMPL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CBindStatusCallback::OnObjectAvailable, IBindStatusCallback
//
//  Synopsis:   This function informs us that the object has been fully
//              loaded.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown *punk)
{
    HRESULT hr = S_OK;

    if (!punk)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    if (_fBindModeInvestigateObjectType)
    {
        _fObjectIsHTMLCompatible = TRUE;

        if (_hEvent)
        {
            SetEvent(_hEvent);
        }
    }
    else if (_pSpooler)
    {
        punk->AddRef();
        _pSpooler->OnLoadComplete((void *) punk, SP_FINISHED_LOADING_EXTERNALDOCUMENT);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::GetPrintInfo
//
//  Synopsis:   the method get's the current printsettings....
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::GetPrintInfo(PRINTINFOBAG * pPrintInfoBag) const
{
    Assert(pPrintInfoBag);
    if (pPrintInfoBag)
    {
        *pPrintInfoBag = _PrintInfoBag;
        return S_OK;
    }
    return E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::SetPrintInfo
//
//  Synopsis:   the method sets the current printsettings....
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::SetPrintInfo(const PRINTINFOBAG &printInfoBag)
{


    // on Win95 those hglobals will not be changed
    // when the dialog is reexecuted. Therefore we would
    // be freeing the memory too early.
/*
these lines caused a bug :
Let's suppose hDevNames has and address of A, and hDevMode is B.
The wrapper for PrintDialog first allocate a new address for hDevNames, let's say C
copy the content and release the old address : A
Now the same happening to hDevMode, but when it allocates new memory for hDevMode it gets
back the last released memory A.
                                Old     New
          hDevNames      A       C
          hDevMode       B       A
Here in the next few lines we release the old address of hDevNames (A), but it is the
new address of hDevMode, therefore we loose hDevMode
The two globalfree is moved to the CSpooler destructor.

    if (_PrintInfoBag.hDevNames != printInfoBag.hDevNames)
    {
        GlobalFree(_PrintInfoBag.hDevNames);
    }
    if (_PrintInfoBag.hDevMode != printInfoBag.hDevMode)
    {
        GlobalFree(_PrintInfoBag.hDevMode);
    }
*/

    _PrintInfoBag = printInfoBag;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::IsEmpty
//
//  Synopsis:   Returns whether there are any active or pending print jobs in
//              the spooler's current and old queue.
//
//  Returns:    Whether both the current queue and the old queue are empty.
//
//----------------------------------------------------------------------------

BOOL
CSpooler::IsEmpty()
{
    BOOL fReturn;

    EnterCriticalSection();

    fReturn = _dblPrintJobs.IsEmpty() && _dblOldPrintJobs.IsEmpty();

    LeaveCriticalSection();

    return fReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::AddDummyJob
//
//  Synopsis:   Adds a dummy job to the Windows print spooler, so the user
//              can see and manipulate it instead.
//
//  Arguments:  pPrintInfo  Print info containing the job for which a dummy job
//                          is to be added
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::AddDummyJob(PRINTINFO *pPrintInfo)
{

    // Temporarily remove this function out for unix, 
#ifdef UNIX
    return S_OK;
#endif

    HRESULT hr = S_OK;
#ifndef WIN16
#ifdef UNIX
    DOCINFO    docinfo;
#else
    DOC_INFO_1 docinfo;
#endif // UNIX
#endif // !WIN16

    if ( !pPrintInfo->fNeedsDummyJob
         WHEN_DBG(|| IsTagEnabled(tagNoDummyJobs)) )
        goto Cleanup;

    Assert(pPrintInfo && !pPrintInfo->djDummyJob.hPrinter && !pPrintInfo->djDummyJob.dwDummyJobId);

    // Obtain the printer handle. (Closed in RemoveDummyJob)
    Assert(pPrintInfo->ppibagRootDocument->ptd);
#ifndef WIN16
    OpenPrinter((TCHAR*) (((BYTE*)pPrintInfo->ppibagRootDocument->ptd)+
                pPrintInfo->ppibagRootDocument->ptd->tdDeviceNameOffset), &(pPrintInfo->djDummyJob.hPrinter), NULL);

    if (!pPrintInfo->djDummyJob.hPrinter)
    {
        WHEN_DBG(DWORD dwError = GetLastError(); Assert(!g_Zero.ab[0] || dwError);)
        Assert(!"Could not obtain printer handle for dummy job");
        hr = E_FAIL;
        goto Cleanup;
    }

#ifndef UNIX
    docinfo.pDocName = pPrintInfo->fUsePrettyUrl ? pPrintInfo->cstrPrettyUrl : pPrintInfo->cstrBaseUrl;
    docinfo.pOutputFile = NULL;
    docinfo.pDatatype = NULL;

    pPrintInfo->djDummyJob.dwDummyJobId = StartDocPrinter(pPrintInfo->djDummyJob.hPrinter, 1, (LPBYTE) &docinfo);
#else
    docinfo.lpszDocName = pPrintInfo->cstrBaseUrl;
    docinfo.lpszOutput = NULL;
    docinfo.lpszDatatype = NULL;

        pPrintInfo->djDummyJob.dwDummyJobId = StartDoc(pPrintInfo->ppibagRootDocument->hdc, &docinfo);
#endif // UNIX

#ifndef UNIX
    if (pPrintInfo->djDummyJob.dwDummyJobId)
    {
        // Pause dummy jobs:  This is really ugly, but this seems to be the only way that
        // dummy jobs don't get in the way of real print jobs by blocking the Win-95 print
        // spooler.
        SetJob(pPrintInfo->djDummyJob.hPrinter, pPrintInfo->djDummyJob.dwDummyJobId, 0, NULL, JOB_CONTROL_PAUSE);
    }
#else
    // why this?  dwDummyJobId always > 0 cos its type is UINT?
    if (pPrintInfo->djDummyJob.dwDummyJobId > 0) {
          EndDoc(pPrintInfo->ppibagRootDocument->hdc);
    }
#endif // UNIX
    else
    {
        hr = E_FAIL;
        WHEN_DBG(DWORD dwError = GetLastError(); Assert(!g_Zero.ab[0] || dwError);)

        // Close printer to be done with this dummy job.
        ClosePrinter(pPrintInfo->djDummyJob.hPrinter);
        pPrintInfo->djDummyJob.hPrinter = 0;

        Assert(!"Failed adding dummy job to Windows print spooler.");
    }
#else
    pPrintInfo->djDummyJob.dwDummyJobId = ++g_dwDummyJobId;
#endif // !WIN16

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CancelDummyJob
//
//  Synopsis:   Cancels a dummy job on the Windows print spooler.
//
//  Arguments:  pDummyJob   Dummy job to be cancelled.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::CancelDummyJob(DUMMYJOB *pDummyJob)
{
#if DBG
    if (IsTagEnabled(tagNoDummyJobs))
        return S_OK;
#endif

#if !defined(WIN16) && !defined(UNIX)
    Assert(pDummyJob && pDummyJob->hPrinter && pDummyJob->dwDummyJobId);

    if (pDummyJob->hPrinter && pDummyJob->dwDummyJobId)
    {
        // Cancel dummy job
        SetJob(pDummyJob->hPrinter, pDummyJob->dwDummyJobId, 0, NULL, JOB_CONTROL_CANCEL);
    }
#endif

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::RemoveDummyJob
//
//  Synopsis:   Removes a dummy job from the Windows print spooler.
//
//  Arguments:  pDummyJobIn Dummy job to be removed.
//
//              dwFlags     Flags for removal:
//
//                            RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY - if not removable yet,
//                            enqueue dummy in DummyJobList to be revisited later.  If
//                            removable don't enqueue.
//
//                            RDJ_REMOVE_DUMMYJOB_IF_POSSIBLE - if removable, remove
//                            dummy job.  Otherwise don't enqueue.
//
//                            RDJ_FORCE_REMOVAL_FROM_QUEUE - wraps up dummy regardless
//                            of its state on the Windows print spooler and removes
//                            dummy job from the queue.
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::RemoveDummyJob(PRINTINFO *pPrintInfo, DUMMYJOB *pDummyJobIn, REMOVE_DUMMYJOB_MODE dwFlags)
{

#if defined(WIN16) || defined(UNIX)
    return S_OK;
#else
    BYTE        pBuffer[sizeof(JOB_INFO_1) + sizeof(TCHAR) * 2 * pdlUrlLen];
    DWORD       cbBuf = sizeof(JOB_INFO_1) + sizeof(TCHAR) * 2 * pdlUrlLen, cbNeeded;
    DUMMYJOB *  pDummyJob = pDummyJobIn;
    BOOL        fRemoveJob = FALSE;
    BOOL        fForceRemoveLoadJobOnShutdown = _fShutdown && RDJ_FORCE_REMOVAL_FROM_QUEUE == dwFlags && pPrintInfo;

    if ( (pPrintInfo && !pPrintInfo->fNeedsDummyJob)
         WHEN_DBG(|| IsTagEnabled(tagNoDummyJobs)) )
        goto Cleanup;

    Assert((pPrintInfo && !pDummyJob) || (!pPrintInfo && pDummyJob) || !"Exactly one dummy job info needed");

    Assert(!pPrintInfo || dwFlags == RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY || fForceRemoveLoadJobOnShutdown ||
        !"Violation of RemoveDummyJob calling convention. If a print info is passed in, a copy of the dummy job is made.  Watch for memory leaks!");

    // Copy the dummy job struct if one wasn't passed in.
    if (!pDummyJob && pPrintInfo && pPrintInfo->djDummyJob.hPrinter)
    {
        pDummyJob = new DUMMYJOB;
        
        if (!pDummyJob)
        {
            // E_OUTOFMEMORY
            goto Cleanup;
        }

        *pDummyJob = pPrintInfo->djDummyJob;
        pPrintInfo->djDummyJob.hPrinter = 0;  // pass off ownership of printer handle
    }

    if (!pDummyJob || !pDummyJob->hPrinter)
    {
        Assert(!"Invalid dummy job");
        goto Cleanup;
    }

    // We have the prerequisites.  New default: Remove job.
    fRemoveJob = TRUE;

    // Cancel dummy job
    IGNORE_HR( CancelDummyJob(pDummyJob) );

#ifndef UNIX
    // Read dummy job status only if we are not forcing the removal.  Don't remove dummy job
    // if GetJob is not successful (returning 0) and if the dummy entry is marked in the
    // Windows spooler as "DELETING AND NOT PRINTING".
    if ( dwFlags != RDJ_FORCE_REMOVAL_FROM_QUEUE
        && ( (!g_fUnicodePlatform && dwFlags == RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY) // don't remove on enqueue on Win95
          || !GetJob(pDummyJob->hPrinter, pDummyJob->dwDummyJobId, 1, pBuffer, cbBuf, &cbNeeded)
          || (((JOB_INFO_1 *) pBuffer)->Status & (JOB_STATUS_DELETING | JOB_STATUS_PRINTING)) != JOB_STATUS_DELETING ) )
    {
        // Job not ready to be fully removed.
        fRemoveJob = FALSE;
    }
#endif // !UNIX

    // If the dummy job has to be removed, wrap it up.
    if ( fRemoveJob )
    {
        // If so, remove it.
        ClosePrinter(pDummyJob->hPrinter);
        pDummyJob->hPrinter = 0;

        // If we were asked to enqueue the dummy job and it was not necessary because we can remove
        // it right away, we are done.  Otherwise remove dummy job from dummy queue.
        if (dwFlags != RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY && !fForceRemoveLoadJobOnShutdown)
        {
            _dblUnremovedDummyJobs.Remove(pDummyJob);
        }

        delete pDummyJob;
    }
    else if (dwFlags == RDJ_ENQUEUE_DUMMYJOB_IF_NECESSARY)
    {
        // Append unremoved dummy job to list.
        _dblUnremovedDummyJobs.Append(pDummyJob);
    }

Cleanup:

    return fRemoveJob ? S_OK : S_FALSE;
#endif // ndef WIN16
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::QueryDummyJob
//
//  Synopsis:   Determines the status of a dummy job on the Windows print spooler.
//
//  Arguments:  pPrintInfo  Print info for which the dummy job is to be queried.
//
//  Returns:    S_OK if the job is still on the queue; S_FALSE if it isn't (cancelled?)
//
//----------------------------------------------------------------------------

HRESULT
CSpooler::QueryDummyJob(PRINTINFO *pPrintInfo)
{
    HRESULT hr = S_OK;

    if ( !pPrintInfo->fNeedsDummyJob
        WHEN_DBG(|| IsTagEnabled(tagNoDummyJobs)) )
        return S_OK;

    Assert(pPrintInfo);

#if !defined(WIN16) && !defined(UNIX)
    // Determine whether the dummy job is still on the windows queue.
    if (pPrintInfo->djDummyJob.hPrinter && pPrintInfo->djDummyJob.dwDummyJobId)
    {
        // This buffer size is somewhat arbitrary.  This is plenty of space, but
        // if this is not enough we will just assume that the job is still on the queue.
        BYTE  pBuffer[sizeof(JOB_INFO_1) + sizeof(TCHAR) * 2 * pdlUrlLen];
        DWORD cbBuf = sizeof(JOB_INFO_1) + sizeof(TCHAR) * 2 * pdlUrlLen, cbNeeded;
        BOOL fSuccess = GetJob(pPrintInfo->djDummyJob.hPrinter, pPrintInfo->djDummyJob.dwDummyJobId, 1, pBuffer, cbBuf, &cbNeeded);

        if (fSuccess)
        {
            DWORD dwStatus = ((JOB_INFO_1 *) pBuffer)->Status;
            // We have to be very conservative here.  We HAVE to err on the save side
            // (print although cancelled as opposed to not printing good jobs).
            hr = (dwStatus & JOB_STATUS_DELETING) ? S_FALSE : S_OK;
        }
    }
#endif // !UNIX && !WIN16
    // ELSE we assume adding the dummy job failed and couldn't have been
    // cancelled manually.

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::CancelDownload
//
//  Synopsis:   Cancels the job currently loading.
//
//----------------------------------------------------------------------------

HRESULT CSpooler::CancelDownload()
{
    Assert(_ppiCurrentLoadJob);

    if (!_ppiCurrentLoadJob)
        return S_OK;

    switch (_ppiCurrentLoadJob->grfPrintObjectType)
    {
    case POT_CPRINTDOC:
        if (_pPrintDocLoading)
        {
            IGNORE_HR( _pPrintDocLoading->ExecStop() );

            // IMPORTANT: Here we rely on a call to OnLoadStatus(LOADSTATUS_DONE)
            // to progress the spooler.
        }
        break;

    case POT_IPRINT:
    case POT_IOLECOMMANDTARGET:
        // Do Nothing for now:  Can create a _pBinding member in Callback object
        // that gets set in OnStartBinding.  Here we would call
        // _pBindStatusCallback->_pbinding->Abort()
        break;
    }

    return S_OK;
}




//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::UpdateDefaultPrinter
//
//  Synopsis:   checks if the default printer changed
//              if so goes ahead and deletes the old printinfo
//              so that a new one will be created the next time print
//              is done
//
//----------------------------------------------------------------------------

void
CSpooler::UpdateDefaultPrinter(void)
{
    TCHAR   achDefaultPrinter[DEFAULTPRINTERSIZE];

    GetProfileString(_T("Windows"), _T("Device"), _T(""), achDefaultPrinter, DEFAULTPRINTERSIZE);

    if (_tcsicmp(achDefaultPrinter, _achDefaultPrinter)!=0)
    {
        // they are different, store the new value and delete the printinfos
        _tcscpy(_achDefaultPrinter, achDefaultPrinter);
        GlobalFree(_PrintInfoBag.hDevNames);
        GlobalFree(_PrintInfoBag.hDevMode);

        _PrintInfoBag.hDevNames=0;
        _PrintInfoBag.hDevMode=0;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CSpooler::AddTempFile
//
//  Synopsis:   Adds a tempfile to the array of tempfile that the spooler
//              deletes as soon as it is empty again.
//
//----------------------------------------------------------------------------

void
CSpooler::AddTempFile(TCHAR * pchTempFile)
{
    CStr * pStr;
    int    cSize;

    if (!pchTempFile)
        return;

    EnterCriticalSection();

    _fRemoveTempFilesWhenEmpty = FALSE;
    cSize = _aryFrameTempFiles.Size();

    pStr = new CStr;
    if (pStr)
    {
        pStr->Set(pchTempFile);

        if (S_OK != _aryFrameTempFiles.Insert(cSize, pStr))
            delete pStr;
    }

    LeaveCriticalSection();
}


void
CSpooler::SetNotifyWindow(HWND hwnd)
{
    _hwndNotify = hwnd;
    UpdateStatusBar();
}




HRESULT
CSpooler::UpdateStatusBar(void)
{
    if (IsWindow(_hwndNotify))
    {
        PostMessage(_hwndNotify, WM_PRINTSTATUS, (WPARAM) !IsEmpty(), 0L);
    }
    RRETURN(S_OK);
}



