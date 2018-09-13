//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       DRECGEN.hxx
//
//  Contents:   HTML Elements Data Binding Extensions
//
//  History:
//
//  Jul-96      AlexA   Creation
//  8/13/96     SamBent Made record generation a LW task, to simulate async
//
//----------------------------------------------------------------------------

#ifndef I_DRECGEN_HXX_
#define I_DRECGEN_HXX_
#pragma INCMSG("--- Beg 'drecgen.hxx'")

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include <taskman.hxx>
#endif

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include <dlcursor.hxx>
#endif

// forward references
class CDetailGenerator;
class CRecordGenerator;

typedef HROW RECORD;
const int cBufferCapacity = 20;

MtExtern(CRecordGeneratorTask)

//+---------------------------------------------------------------------------
//
//  class CRecordGeneratorTask
//
//----------------------------------------------------------------------------

class CRecordGeneratorTask: public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CRecordGeneratorTask))

    // CTask methods
    void    OnRun(DWORD dwTimeout);
    void    OnTerminate();

private:
    friend class CRecordGenerator;
    CRecordGeneratorTask(CDetailGenerator* pDetailGenerator,
                         CRecordGenerator* pRecordGenerator,
                         const CDataLayerBookmark& dlbStart,
                         LONG lSkipCount,
                         LONG lRecordsDesired,
                         int cInitialRequestSize);
    HRESULT ReasonTerminated() const { return _hrTerminated; }

    // Move records from my buffer to client's
    HRESULT FetchRecords(IN  ULONG  cRecords,
                         OUT RECORD ahRecords[],
                         OUT ULONG* pcRecordsFetched);

    // Release records
    HRESULT ReleaseRecords(ULONG cRecords);

private:
    // collaborators
    CDetailGenerator*   _pDetailGenerator;  // client
    CRecordGenerator*   _pRecordGenerator;  // parent
    
    // request parameters
    ULONG   _cDesired;                      // number of records desired in request
    int     _cDirection;                    // direction (1: -1);
    int     _cInitialRequestSize;           // initial request size (number of records to get)
    
    // current state of request
    CDataLayerBookmark  _dlbLastRetrieved;  // bookmark of last record retrieved
    LONG    _lSkipCount;                    // offset from _dlbLastRetrieved to start of batch
    ULONG   _cTotalGenerated;               // number of records generated for request
    ULONG   _cPrevGenerated;                // records in previous batch
    ULONG   _cPrevTime;                     // time spent in previous batch
    HRESULT _hrTerminated;                  // the reason request terminated

    // data buffer
    RECORD  _aRecordBuffer[cBufferCapacity]; // buffer
    ULONG   _iEndReleased;                  // end of "released" area of buffer
    ULONG   _iEndFetched;                   // end of "fetched" area of buffer
    ULONG   _iEndGenerated;                 // end of "generated" area of buffer

    // helper functions
#if DBG==1
    BOOL    IsValid() const;
#endif
};


//+---------------------------------------------------------------------------
//
//  class CRecordGenerator
//
//----------------------------------------------------------------------------

MtExtern(CRecordGenerator)

class CRecordGenerator
{
public:
    DECLARE_MEMALLOC_NEW_DELETE(Mt(CRecordGenerator))
    CRecordGenerator (); 
    ~CRecordGenerator ();

    // Initialization
    HRESULT Init(CDataLayerCursor *pDLC, CDetailGenerator *pHost)
        {   Assert(pDLC && pHost);
            _pDLC = pDLC;
            _pDLC->AddRef();
            _pDetailGenerator = pHost;
            return S_OK;
        }

    // Detaches the object, removing any internal self-references.                   
    void Detach ();

    HRESULT RequestMetaData();  // request to load MetaData

    HRESULT RequestRecordsAtBookmark(const CDataLayerBookmark& dlb,
                                    LONG lSkipCount, LONG lGenerateCount)
            { return StartTask(dlb, lSkipCount, lGenerateCount); }
                                    
    // Stop previously requested record generation
    HRESULT CancelRequest ();

    // Move records from my buffer to client's
    HRESULT FetchRecords(IN  ULONG  cRecords,
                         OUT RECORD ahRecords[],
                         OUT ULONG* pcRecordsFetched)
        { return _pTask->FetchRecords(cRecords, ahRecords, pcRecordsFetched); }

    // Release records
    HRESULT ReleaseRecords(ULONG cRecords)
        { return _pTask->ReleaseRecords(cRecords); }

    // Get Cursor method is needed for data binding transaction.
    HRESULT GetCursor (CDataLayerCursor **ppCursor);

    // get the ratio for the record
    void GetRatio(HROW hrow, DBCOUNTITEM *pulNumerator, DBCOUNTITEM *pulDenominator);

    // When task is done, forget about it
    void    OnTaskDone();

    // Returns True if there is a current outstanding CRecordGeneratorTask
    BOOL TaskBusy() { return !!_pTask; }

private:
    // helper functions
    HRESULT StartTask(const CDataLayerBookmark& dlb, LONG lSkipCount, 
                      ULONG ulGenerateCount);

    // private state
    CDataLayerCursor            *_pDLC;
    CDetailGenerator            *_pDetailGenerator; // pointer to a host of an UI instances
    CRecordGeneratorTask*       _pTask;     // current task

    NO_COPY(CRecordGenerator);
};

#pragma INCMSG("--- End 'drecgen.hxx'")
#else
#pragma INCMSG("*** Dup 'drecgen.hxx'")
#endif
