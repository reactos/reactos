//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       detail.cxx
//
//  Contents:   HTML Tables Repeating Extensions
//
//  History:
//
//  Jul-96      AlexA   Creation
//  8/14/96     SamBent Support record generation in a lightweight task
//
//----------------------------------------------------------------------------

#ifndef I_DETAIL_HXX_
#define I_DETAIL_HXX_
#pragma INCMSG("--- Beg 'detail.hxx'")

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include "dlcursor.hxx"
#endif

#ifndef X_DBINDING_HXX_
#define X_DBINDING_HXX_
#include "dbinding.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

//  TABLE "Repeat" implementation:
//  -----------------------------
//  
//  1. Overall Design:
//  
//  HTML Tables can specifying repetition by introducing the DataSource attribute in the TABLE tag.
//  HTML Table according to the HTML3.0 spec can have a header and the footer section and can consist of
//  multiple TBODYs. Our first implementation of the HTML tables extension will assume that any elements
//  that is embedded inside the TABLE and is not in the header and not in the footer section is repeatable 
//  if the DataSource is specified on the table. For example, if you have 2 TBODYs inside of the TABLE, 
//  we will repeat 2 TBODYs for each record in the data source.
//
//  The definition of the scrollable region inside the table is yet to be defined by the HTML tables spec.
//  But for now, we assume it will the mechanism to specify that user wants the table to be X units wide and
//  Y units height. The size of the table and the Style Sheets specification for scrolling will determine the
//  scrollable region. Since the table will have the header and a footer and we want to view the repeated 
//  Elements in a scrollable region being clipped, the easiest way to implement that would be to introduce 
//  the object, that will populate the rectangular area of the table and clip all the repeated elements. 
//  We will call that object a RepeaterSite (DetailSection).
//

typedef long  RECORDINDEX;
extern  const RECORDINDEX RECORDINDEX_INVALID;

typedef CPtrAry<CElement *> CAryElement;

class CRecordGenerator;         // forward reference
class CTable;
class CTableRow;
class CTableSection;
class CDGTask;

//+---------------------------------------------------------------------------
//
//  Building of the repeated instances is handled by two classes:
//
//  1, class CDetailGenerator
//      is responsible for a procreation of the instances (cloning)
//  2. class CRecordGenerator
//      is a supplyer of the HS. The object of this class is created for 
//      every unique Repeated Source of data (1 per RowSet).
//  Note: the main objection for having a separate classe for Instance 
//      building (as opposed to implementing it as a part of the Table class)
//      is to be able later in the Form^3 development to add generic 
//      repeatition of any HTML elements (not just repeating elements of 
//      the HTML tables).
//
//  Note: The reason for having the CRecordGenerator as a separate object is 
//      to be able assynchrounosly download data from the Element set.
//
//----------------------------------------------------------------------------

// there is the following convention for public/protected/private member 
// frunctions and member data used in this file.

// 1. public methods
//    are used by the host of the detail generator
// 2. protected methods (not abstract)
//    implmenting the call backs from the 2 friend classes CRecordGenerator, 
//    and CTabularDataBindAgent.
// 3. protected abstract (virtual = 0) methods
//    need to be overriden for any concreate implmentation of the CDetailHost 
//    class

// (sambent, 5/19/97)  I've merged CTableDetailGenerator into CDetailGenerator
// (removed useless derivation), and moved CScrollTableDetailGenerator to dead
// code land (we're not supporting scrolling tables).  This means nothing
// derives from CDetailGenerator any more.  So I've removed all the virtual
// functions, using the macro DB_VIRT to suggest which function might have to
// be made virtual if any derivation becomes necessary in the future.

#define DB_VIRT


//+----------------------------------------------------------------------------
//
//  Class CDetailGenerator
//
//  Purpose:
//      Table repetition manager
//

MtExtern(CDetailGenerator)

class CDetailGenerator: public CDataLayerCursorEvents
{
    friend class CRecordInstance;
    friend class CFindDataBindWalker;

public:
    CDetailGenerator();
    DB_VIRT ~CDetailGenerator();
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDetailGenerator))

    // Initialization
    HRESULT Init(CDataLayerCursor         *pdlc,
                 CTable                   *pTable,
                 unsigned int              cInsertNewRowAt,
                 int                       cPageSize);

    // save the Element template's (as HTML text to a stream)
    HRESULT PrepareForCloning();

    // Populate the table with Elements (is called when populating 1st time)
    DB_VIRT HRESULT Generate(); 
    void    StopGeneration() { CancelRequest(); }
    
    // Detaches the repeated element, removing any 
    // internal self-references.                   
    DB_VIRT void Detach ();
    void    ReleaseGeneratedRows();
    void    ReleaseRecords(RECORDINDEX riFirst, RECORDINDEX riLast);

    void    SetTemplateCount(int i) { _iTemplateCount = i; }
    HRESULT AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining, LPCTSTR strField);
    HRESULT HookUpDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining,
                                LPCTSTR strField);

    // Get the number of elements (rows in case of the table) in the template.
    int GetTemplateSize () { return _iTemplateCount; }

    DB_VIRT CElement *GetHost() { return (CElement *) _pTable; }
    LPCTSTR GetDataSrc() { return _bstrDataSrc; }
    LPCTSTR GetDataFld() { return _bstrDataFld; }
    BOOL    IsRepeating() { return TRUE; }
    BOOL    IsFieldKnown(LPCTSTR strField);
    void    SetReadyState();

    HRESULT nextPage();                 // for tables with a pageSize>0
    HRESULT previousPage();
    HRESULT firstPage();
    HRESULT lastPage();
    HRESULT SetPageSize(long cPageSize);

    // CDataLayerCursorEvents methods
    HRESULT AllChanged();
    HRESULT RowsChanged(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT FieldsChanged(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[]);
    HRESULT RowsInserted(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT DeletingRows(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT RowsDeleted(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT DeleteCancelled(DBCOUNTITEM cRows, const HROW *ahRows);
    HRESULT RowsAdded();
    HRESULT PopulationComplete();

    // Support for record number property
    long    RowIndex2RecordNumber(int iRow)
        { return RecordIndex2RecordNumber(RowIndex2RecordIndex(iRow)); }
    
    // callbacks from RecordGenerator:
    HRESULT OnMetaDataAvailable();
    DB_VIRT HRESULT OnRecordsAvailable(ULONG cRecords);
    DB_VIRT HRESULT OnRequestDone(BOOL fLastData);
    DB_VIRT HRESULT NewRecord();

    // put the template back in the table
    HRESULT RestoreTemplate();

    // support for hierarchy
    CRecordInstance*   GetInstanceForRow(int iRow)
    {
        RECORDINDEX ri = RowIndex2RecordIndex(iRow);
        return (ri<0 || ri>=RecordCount()) ? NULL : _aryRecInstance[ri];
    }


protected:

    HROW    GetHrow(RECORDINDEX ri);
    HRESULT FindHrow(HROW hrow, RECORDINDEX *pri);
    HRESULT FindBookmark(const CDataLayerBookmark& dlb, RECORDINDEX *pri);
    RECORDINDEX FirstActiveIndex()
        { return !_reqCurrent.IsActive() ? 0 : _reqCurrent.IsForward() ? 0 :
                 _reqCurrent._riCurrent == RECORDINDEX_INVALID ? RecordCount() :
                        _reqCurrent._riCurrent + 1; }
    RECORDINDEX LastActiveIndex()
        { return (!_reqCurrent.IsActive() ? RecordCount() :
                !_reqCurrent.IsForward() ? RecordCount() :
                _reqCurrent._riCurrent == RECORDINDEX_INVALID ? RecordCount() :
                        _reqCurrent._riCurrent) - 1; }

    //+-------------------------------------------------------------------
    //  The following functions are for Cloning implementation
    //--------------------------------------------------------------------

    HRESULT Clone(UINT nTemplates, RECORDINDEX ri=RECORDINDEX_INVALID);
    HRESULT AddTemplates(UINT nTemplates, RECORDINDEX ri=RECORDINDEX_INVALID);
    HRESULT CreateDetail(HROW hrow, RECORDINDEX *pri = NULL);
    HRESULT CreateRecordInstance (RECORDINDEX *pri);
    HRESULT PasteRows(BSTR bstrTemplates, CTableSection *pSectionInsertBefore);

    //+-----------------------------------------------------------------------
    //  Record interation (really "record index" interation)
    //------------------------------------------------------------------------

    RECORDINDEX GetCurrentIndex() const { return _reqCurrent._riCurrent; }
    void        SetCurrentIndex(RECORDINDEX ri) { _reqCurrent._riCurrent = ri; }
    RECORDINDEX FirstAvailableIndex() const
        { return (GetCurrentIndex() == RECORDINDEX_INVALID) ?
                    _aryRecInstance.Size() : GetCurrentIndex(); }
    DB_VIRT RECORDINDEX AdvanceRecordIndex(RECORDINDEX riPrev);
    DB_VIRT RECORDINDEX RetreatRecordIndex(RECORDINDEX riPrev);

    // Mappings between row index, record index, record number
    int getFirstDetailRowIndex() { return _iFirstGeneratedRow; }
    int RecordIndex2RowIndex(RECORDINDEX ri)
        { return getFirstDetailRowIndex() + ri*GetTemplateSize(); }
    RECORDINDEX RowIndex2RecordIndex(int iR)
        { return iR >= getFirstDetailRowIndex()? 
            (iR - getFirstDetailRowIndex())/GetTemplateSize():
            RECORDINDEX_INVALID; }     
    long    RecordIndex2RecordNumber(RECORDINDEX ri);

    // Request to record generator
    HRESULT MakeRequest(const CDataLayerBookmark& dlbStart, LONG lOffset,
                        LONG cRecords, RECORDINDEX ri);
    HRESULT CancelRequest(BOOL fLastRequest=TRUE);
    HRESULT EndRequest(BOOL fLastRequest=TRUE);
    void    SetBookmarks();
    void    ReleaseProgress();
    BOOL    PinnedToEnd() const { return !_reqCurrent.IsForward() &&
                _reqCurrent._dlbBot == CDataLayerBookmark::TheLast; }

    //+-----------------------------------------------------------------------
    //  Member data (accessable from the derived classes).
    //------------------------------------------------------------------------

    CTable *        _pTable;
    int             _iFirstGeneratedRow;        // index into a table arySites of the first generated row

    // counter of repeated template elements (rows)
    int                     _iTemplateCount;    
    
    CRecordGenerator *      _pRecordGenerator;      // pointer to a record generator

    // number of generated table rows per display page, for paginated tables
    int             _cPageSize;

    CDataLayerCursor *      _pDLC;              // cursor
    CPtrAry<CRecordInstance *>  _aryRecInstance; // one for each repetition of the template
    
    LONG        RecordCount() const { return _aryRecInstance.Size(); }

private:

    //+-----------------------------------------------------------------------
    //  Private member data (not accessable from the derived classes).
    //------------------------------------------------------------------------

    // delete the detail section
    void DeleteDetail (RECORDINDEX ri);
    
    inline void ClearRecordGenerator();
    inline void ClearDataBindAgent();
    void ClearBoundTemplateElements();

    HRESULT     AssignHrowToRecord(HROW hrow, RECORDINDEX ri);
    
    BSTR        _bstrHtmlText;          // pointer to html text of the tenplate
    BSTR        _bstrDataSrc;           // my full dataSrc
    BSTR        _bstrDataFld;           // my full dataFld

    CDataSourceProvider *   _pProvider; // my table's provider (not refcounted)

    DWORD       _dwProgressCookie;      // cookie for progress item
    CDGTask *   _pDGTask;               // my hookup task

    // Request to record generator
    struct CRecRequest
    {
        CDataLayerBookmark  _dlbStart;  // request starts at (bmk, offset)
        LONG                _lOffset;
        LONG                _cRecords;  // number of records desired
        LONG                _cRecordsRetrieved; // number we got so far
        RECORDINDEX         _riCurrent; // index for next record
        CDataLayerBookmark  _dlbTop;    // bookmark for topmost record retrieved
        RECORDINDEX         _riTop;     // index of topmost record retrieved
        CDataLayerBookmark  _dlbBot;    // bookmark for bottommost record retrieved
        RECORDINDEX         _riBot;     // index of bottommost record retrieved
        unsigned            _fActive:1; // true if active
        unsigned            _fGotRecords:1; // true if request retrieved any records
        
        void    Activate(const CDataLayerBookmark& dlb, LONG lOffset, LONG cRecords,
                        RECORDINDEX ri);
        void    Deactivate() { _fActive = FALSE; }
        BOOL    IsActive() const { return _fActive; }
        BOOL    IsForward() const { return _cRecords >= 0; }
        BOOL    GotRecords() const { return _fGotRecords; }
    };

    CRecRequest _reqCurrent;        // current request to record generator
        
    unsigned    _fRestoringTemplate:1;  // true while restoring original template
    
    NO_COPY(CDetailGenerator);
};


//+----------------------------------------------------------------------------
//
//  Class CDGTask
//
//  Purpose:
//      Hook up elements in a repeated table
//
//  The point here is to actually hook up the databound elements at a safe time.
//  See IE5 bug 48246 for background on what can go wrong if we don't.
//
//  Elements can get pasted into the table in a variety of ways:
//      (a) as part of an instance of the repeating template,
//      (b) from script, like "foo.innerHTML = '<DIV>...</DIV>'" where foo is
//          an element inside the table,
//      (c) via MarkupServices, like (b) except using the interfaces directly
//
//  "Safe time" means that we're not in the middle of modifying the tree.  We
//  have to let the operation finish, and then come back and start propagating
//  database values into the new elements.
//
//  To do this, each CDetailGenerator owns a CDGTask.  Elements register their
//  need for values during EnterTree by calling AddDataboundElement.  This
//  simply adds the element to a list, and enables the task.  When the task runs,
//  it marches throught the list hooking up the elements to the data source;
//  it is "safe" to do so, because the tree operations have released control
//  to the task manager.
//
//  We have to be a little careful about the list, because propagating a database
//  value may have a side effect of adding more elements to the list, or
//  removing elements from the tree that are still on the list.

MtExtern(CDGTask);

class CDGTask: public CTask
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CDGTask));

    // construction/destruction
    CDGTask(CDetailGenerator *pDG);
    
    // Databinding service
    HRESULT AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining,
                                LPCTSTR strField);
    BOOL    IsIdle() { return TestFlag(TASKF_BLOCKED); }
    
    // CTask methods
    void    OnRun(DWORD dwTimeout);
    void    OnTerminate();
    
private:
    // Request to bind databound element
    struct CBindRequestRecord
    {
        CElement *  pElement;
        LONG        id;
        CTableRow * pRowContaining;
        LPCTSTR     strField;
    };

    CDetailGenerator *              _pDG;               // my owner
    CDataAry<CBindRequestRecord>    _aryBindRequest;    // list of requests
};

#pragma INCMSG("--- End 'detail.hxx'")
#else
#pragma INCMSG("*** Dup 'detail.hxx'")
#endif
