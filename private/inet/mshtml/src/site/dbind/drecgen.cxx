//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       drecgen.cxx
//
//  Contents:   record generator.
//              CRecordGeneratorTask
//              CRecordGenerator
//
//  History:
//
//  Jul-96      AlexA   Creation
//  8/13/96     SamBent Made record generation a LW task, to simulate async
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include <table.hxx>
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>                   // for db_s_endofrowset
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include "detail.hxx"
#endif

#ifndef X_DRECGEN_HXX_
#define X_DRECGEN_HXX_
#include "drecgen.hxx"
#endif

DeclareTag(tagRecGenTask,"Databinding","CRecordGeneratorTask");
DeclareTag(tagRecGen,"DataBinding","CRecordGenerator");
DeclareTag(tagRecGenBatchSize, "Databinding", "Fetch g_BatchSize records at a time");

MtDefine(CRecordGenerator, DataBind, "CRecordGenerator")
MtDefine(CRecordGeneratorTask, DataBind, "CRecordGeneratorTask")

const int INITIAL_BATCH_SIZE = 3;

#if DBG == 1
static int g_BatchSize = 1;     // When tagRecGenBatchSize is enabled;  change via debugger
#endif


//////////////////////////////////////////////////////////////////////////
//
//  CRecordGeneratorTask implementation
//
//////////////////////////////////////////////////////////////////////////

// The lightweight task that generates records uses a buffer divided into
// four sections, containing respectively
//      o records client has released
//      o records client has fetched (but not released yet)
//      o records task has retrieved from the cursor (but client hasn't fetched)
//      o emply slots
// These areas are defined by data members _iEndReleased, _iEndFetched, and
// _iEndGenerated, which give the index just after the corresponding area:
//
//       --------------------------------------------------------------------
//      |   released    |     fetched     |    generated      |   (empty)    |
//       --------------------------------------------------------------------
//       _iEndReleased---^  _iEndFetched---^  _iEndGenerated---^

// typedef char CompileTimeAssert[cInitialRequestSize<=cBufferCapacity];


CRecordGeneratorTask::CRecordGeneratorTask(CDetailGenerator* pDetailGenerator,
                                           CRecordGenerator* pRecordGenerator,
                                           const CDataLayerBookmark& dlbStart,
                                           LONG lSkipCount,
                                           LONG lRecordsDesired,
                                           int cInitialRequestSize):
    CTask(TRUE),                        // this task should be born blocked
    _pDetailGenerator(pDetailGenerator),
    _pRecordGenerator(pRecordGenerator),
    _dlbLastRetrieved(dlbStart),
    _lSkipCount(lSkipCount),
    _hrTerminated(S_FALSE),         // in case I'm cancelled
    _cInitialRequestSize(cInitialRequestSize)
{
    if (lRecordsDesired > 0)
    {
        _cDirection = 1;
        _cDesired = lRecordsDesired;
    }
    else
    {
        _cDirection = -1;
        _cDesired = -lRecordsDesired;
    }
    Assert(_pDetailGenerator && _pRecordGenerator); 
    Assert(_cDesired > 0);
}

#if DBG==1
//+------------------------------------------------------------------------
//
//  Member:     IsValid
//
//  Synopsis:   Check that buffer representation is valid (used in Asserts)
//
//  Arguments:  none
//
//  Returns:    true        buffer is valid
//              false       buffer isn't valid

BOOL
CRecordGeneratorTask::IsValid() const
{
    return  _iEndReleased <= _iEndFetched &&
            _iEndFetched <= _iEndGenerated &&
            _iEndGenerated <= cBufferCapacity &&
            _cDirection * _cDirection == 1;
}
#endif


//+------------------------------------------------------------------------
//
//  Member:     FetchRecords (called by client)
//
//  Synopsis:   Move records from my buffer to client's buffer
//
//  Arguments:  cRecords            number of records to copy
//              ahRecords           client's array of records
//              pcRecordsFetched    number of records copied
//
//  Returns:    S_OK        it worked

HRESULT
CRecordGeneratorTask::FetchRecords(IN  ULONG  cRecords,
                     OUT RECORD ahRecords[],
                     OUT ULONG* pcRecordsFetched)
{
//    TraceTag((tagRecGenTask,
//                "CRecordGeneratorTask[%p]::FetchRecords(%lu, %p, %p) time %lu",
//                this, cRecords, ahRecords, pcRecordsFetched, GetTickCount()));
    Assert(pcRecordsFetched && "NULL pointer");
    Assert(IsValid());

    // don't fetch more records than we have
    if (cRecords > _iEndGenerated-_iEndFetched)
        cRecords = _iEndGenerated-_iEndFetched;

    // copy records to client's memory
    memmove(ahRecords, &_aRecordBuffer[_iEndFetched], cRecords * sizeof(RECORD));
    _iEndFetched += cRecords;
    
    // tell client how many
    *pcRecordsFetched = cRecords;

    RRETURN(S_OK);
}


//+------------------------------------------------------------------------
//
//  Member:     ReleaseRecords (called by client)
//
//  Synopsis:   Release records
//
//  Arguments:  cRecords            number of records to release
//
//  Returns:    S_OK        it worked

HRESULT
CRecordGeneratorTask::ReleaseRecords(ULONG cRecords)
{
//    TraceTag((tagRecGenTask,
//                "CRecordGeneratorTask[%p]::ReleaseRecords(%lu) time %lu",
//                this, cRecords, GetTickCount()));
    Assert(IsValid());
    HRESULT hr;

    CDataLayerCursor* pCursor;
    
    hr = _pRecordGenerator->GetCursor(&pCursor);
    if (hr)
        goto Cleanup;
    
    // don't release more records than we have
    if (cRecords > _iEndGenerated-_iEndReleased)
        cRecords = _iEndGenerated-_iEndReleased;

    // release the records, and mark them released
    pCursor->ReleaseRows(cRecords, &_aRecordBuffer[_iEndReleased]);
    _iEndReleased += cRecords;
    if (_iEndFetched < _iEndReleased)
        _iEndFetched = _iEndReleased;

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnRun (called by task manager)
//
//  Synopsis:   Retrieve more records, and callback to client
//
//  Arguments:  dwTimeout           tick count by which we should finish
//
//  Returns:    nothing

void 
CRecordGeneratorTask::OnRun(DWORD dwTimeout)
{
    TraceTag((tagRecGenTask,
                "CRecordGeneratorTask[%p]::OnRun(%lu) time %lu",
                this, dwTimeout, GetTickCount()));
    Assert(IsValid());

    HRESULT hr;
    DBCOUNTITEM ulRecordsRetrieved = 0;
    ULONG   cThrottleRecords;         // how many we have time for
    ULONG   cRecords;                 // how many we plan to retrieve
    const   ULONG dwStartTime = GetTickCount();
    CDataLayerCursor* pCursor;
#if DBG==1
    ULONG cPrevTime = _cPrevTime;
    ULONG cPrevGenerated = _cPrevGenerated;
#endif
    
    hr = _pRecordGenerator->GetCursor(&pCursor);
    Assert(!hr && "Couldn't get cursor");       // this shouldn't happen

    // discard released records, and shift contents of buffer
    if (_iEndReleased > 0)
    {
        memmove(_aRecordBuffer, &_aRecordBuffer[_iEndReleased],
                (_iEndGenerated-_iEndReleased) * sizeof(RECORD));
        _iEndGenerated -= _iEndReleased;
        _iEndFetched   -= _iEndReleased;
        _iEndReleased   = 0;
    }
    Assert(IsValid());

    // cThrottleRecords is the "ideal" number of records to fetch
    cThrottleRecords = 
                    // first time - throttle to initial request
        (_cPrevTime == 0) ? _cInitialRequestSize :
                    // otherwise estimate how many we have time for
        (2*_cPrevGenerated*(dwTimeout-dwStartTime) + _cPrevTime) / (2*_cPrevTime);
    if (cThrottleRecords < 1)   // but get at least one
        cThrottleRecords = 1;    
    
    // determine how many records to fetch
    cRecords = _cDesired - _cTotalGenerated;     // try to finish request
    if (cRecords > cThrottleRecords)             // but don't take too long
        cRecords = cThrottleRecords;
    if (cRecords > cBufferCapacity-_iEndGenerated) // and don't overflow buffer
        cRecords = cBufferCapacity - _iEndGenerated;

#if DBG==1
    if (IsTagEnabled(tagRecGenBatchSize))
        cRecords = g_BatchSize;
#endif
    AssertSz(cRecords > 0, "Can't make progress fetching 0 records");

    // retrieve the records, mark them generated
    hr = pCursor->GetRowsAt(_dlbLastRetrieved,
                            _lSkipCount,
                            cRecords * _cDirection,
                            &ulRecordsRetrieved,
                            &_aRecordBuffer[_iEndGenerated]);
    
    _cTotalGenerated += (ULONG)ulRecordsRetrieved;
    _iEndGenerated += (ULONG)ulRecordsRetrieved;

    // notify client that records are available
    if (_iEndGenerated - _iEndFetched > 0)
    {
        IGNORE_HR(_pDetailGenerator->OnRecordsAvailable(_iEndGenerated - _iEndFetched));
    }

    // prepare for the next batch
    if (ulRecordsRetrieved > 0)
    {
        // remember where we stopped
        IGNORE_HR(pCursor->CreateBookmark(_aRecordBuffer[_iEndGenerated-1],
                                        &_dlbLastRetrieved));
        _lSkipCount = _cDirection;

        // remember how long it took and how many records we got
        _cPrevTime = GetTickCount() - dwStartTime;
        _cPrevGenerated = ulRecordsRetrieved;
    }

    TraceTag((tagRecGenTask,
        "CRecordGeneratorTask[%p]::OnRun (time %lu) retrieved %lu records in %lu ticks \n"
        " desired = %lu, cThrottleRecords = %lu, cRecords = %lu, cPrevTime = %lu, cPrevGenerated = %lu, ticks = %lu",
        this, GetTickCount(), ulRecordsRetrieved, _cPrevTime,
        _cDesired, cThrottleRecords, cRecords,
        cPrevTime, cPrevGenerated , dwTimeout-dwStartTime
        ));

    // if we're done, terminate
    if (_iEndGenerated - _iEndFetched == 0 &&
        (FAILED(hr) || ulRecordsRetrieved<cRecords || _cTotalGenerated>=_cDesired))
    {
        _hrTerminated = hr;
        Terminate();
    }
}

//+------------------------------------------------------------------------
//
//  Member:     OnTerminate (called by task manager)
//
//  Synopsis:   notify parent that task is done
//
//  Arguments:  none
//
//  Returns:    nothing

void
CRecordGeneratorTask::OnTerminate()
{
    TraceTag((tagRecGenTask,
                "CRecordGeneratorTask[%p]::OnTerminate() time %lu",
                this, GetTickCount()));
    
    // release records
    IGNORE_HR(ReleaseRecords(_iEndGenerated - _iEndReleased));
    _dlbLastRetrieved = CDataLayerBookmark::TheNull;

    // tell parent I'm done
    if (_hrTerminated != S_FALSE)
        _pRecordGenerator->OnTaskDone();
}
    


//////////////////////////////////////////////////////////////////////////
//
//  CRecordGenerator implementation
//
//////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  class       CRecordGenerator
//
//  Member:     constructor
//
//-------------------------------------------------------------------------
CRecordGenerator::CRecordGenerator ():
    _pDLC(0),
    _pDetailGenerator(0),
    _pTask(0)
{
    TraceTag((tagRecGen, "CRecordGenerator::constructor() -> %p", this));
}




//+------------------------------------------------------------------------
//
//  class       CRecordGenerator
//
//  Member:     destructor
//
//-------------------------------------------------------------------------
CRecordGenerator::~CRecordGenerator()
{
    TraceTag((tagRecGen, "CRecordGenerator::destructor() -> %p", this));

    Assert (!_pDLC);                // Detach should be called first
    Assert (!_pDetailGenerator);
    Assert (!_pTask);
}




//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Detaches the object, removing any internal self-
//              references.
//
//-------------------------------------------------------------------------
void 
CRecordGenerator::Detach()
{
    TraceTag((tagRecGen, "CRecordGenerator::Detach() -> %p", this));

    // if my task is running, shut it off
    if (_pTask)
    {
        _pTask->Release();
        _pTask = 0;
    }

    if (_pDLC)
    {
        _pDLC->Release();
        _pDLC = NULL;
    }
    
    _pDetailGenerator = NULL;
}




//+------------------------------------------------------------------------
//
//  Member:     RequestMetaData
//
//  Synopsis:   Before starting generating rows, we need to get the metadata
//              to establish binding specifications for the bound elements.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT 
CRecordGenerator::RequestMetaData()
{
    TraceTag((tagRecGen, "CRecordGenerator::RequestMetaData() -> %p", this));
    Assert(_pDetailGenerator && "call Init first");
    
    HRESULT hr;
    CDataLayerCursor *pCursor;

    hr = GetCursor(&pCursor);
    if (hr)
        goto Cleanup;

    hr = _pDetailGenerator->OnMetaDataAvailable();

Cleanup:
    RRETURN (hr);
}



//+------------------------------------------------------------------------
//
//  Member:     GetCursor
//
//  Synopsis:   Get Cursor 
//
//  Note:       Could fail during the metadata fetching
//
//  Arguments:  [ppCursor]              -- pointer to an a result (pointer to a Cursor).
//                                          
//  Returns:    HRESULT 
//
//-------------------------------------------------------------------------
HRESULT 
CRecordGenerator::GetCursor(CDataLayerCursor **ppDataLayerCursor)
{
    *ppDataLayerCursor = _pDLC;
    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     CancelRequest
//
//  Synopsis:   Call from the Host to stop previous request to generate rows
//
//  Arguments:  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT 
CRecordGenerator::CancelRequest ()
{
//    TraceTag((tagRecGen, "CRecordGenerator::CancelRequest() -> %p", this));

    if (_pTask)
    {
        _pTask->Release();      // this calls Terminate
        _pTask = 0;
    }
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     GetRatio
//
//  Synopsis:   Get the ratio for the record
//
//  Arguments:  record          record to get the ratio for
//              pulNumerator    result
//              pulDenominator  result
//                                          
//-------------------------------------------------------------------------

void 
CRecordGenerator::GetRatio(RECORD record, 
                           DBCOUNTITEM *pulNumerator, 
                           DBCOUNTITEM *pulDenominator)
{
    Assert (pulNumerator);
    Assert (pulDenominator);

    CDataLayerCursor *  pCursor;
    HRESULT             hr;

    hr = GetCursor(&pCursor);
    if (hr)
        goto Cleanup;
    
    hr = pCursor->GetPositionAndSize(record, pulNumerator, pulDenominator);
    Assert (!hr);           // should never fail

Cleanup:
    return;
}




#if DBG==1
static ULONG ulDebugGenerateCount=0;    // set (in debugger) to truncate query
static BOOL fDebugSynchronous=0;        // set to turn off asynchrony
#endif

//+------------------------------------------------------------------------
//
//  Member:     StartTask
//
//  Synopsis:   Start a task to retrive records
//
//  Arguments:  dlbStart        bookmark of starting record
//              lSkipCount      offset from bookmark to starting record
//              ulGenerateCount number of records to generate
//
//  Returns:    HRESULT

HRESULT
CRecordGenerator::StartTask(const CDataLayerBookmark& dlbStart, 
                           LONG lSkipCount,
                           ULONG ulGenerateCount)
{
    TraceTag((tagRecGen, "CRecordGenerator::StartTask[%p](%p, %ld, %lu)",
                            this, &dlbStart, lSkipCount, ulGenerateCount));
    HRESULT hr = S_OK;
    
#if DBG==1
    // debugging - reset desired count
    if (ulDebugGenerateCount>0 && ulDebugGenerateCount<ulGenerateCount)
        ulGenerateCount = ulDebugGenerateCount;
#endif

    Assert(!_pTask);

    // create a task to generate the records (this also schedules the task)
    _pTask = new CRecordGeneratorTask(_pDetailGenerator, this, dlbStart,
                                      lSkipCount, ulGenerateCount, INITIAL_BATCH_SIZE);
    if (_pTask == 0)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    _pTask->SetBlocked(FALSE);          // let task get scheduled now.

#if DBG==1
    // debugging - do task synchronously
    if (fDebugSynchronous)
    {
        while (_pTask)
        {
            _pTask->OnRun(GetTickCount()+100);
        }
    }
#endif

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnTaskDone
//
//  Synopsis:   Notification from task that it's done.  Release the task,
//              and notify my client

void
CRecordGenerator::OnTaskDone()
{
    if (_pTask)
    {
        HRESULT hrReasonTerminated = _pTask->ReasonTerminated();
        
        _pTask->Release();
        _pTask = 0;

        Assert(_pDetailGenerator);
        _pDetailGenerator->OnRequestDone( hrReasonTerminated == DB_S_ENDOFROWSET 
                                    || hrReasonTerminated == DB_E_BADSTARTPOSITION );
    }
}
