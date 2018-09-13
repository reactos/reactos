//+-----------------------------------------------------------------------
//
//  Add/Remove Programs Data Source Object
//
//------------------------------------------------------------------------


#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "datasrc.h"
#include "dump.h"
#include "util.h"

//---------------------------------------------------------------------------
//   
//---------------------------------------------------------------------------


// constructor
CDataSrc::CDataSrc()
{
    TraceMsg(TF_OBJLIFE, "(Mtx) creating");
    TraceAddRef(CDataSrc, _cRef);
    
    ASSERT(NULL == _parpevt);
    ASSERT(NULL == _pmtxarray);
    ASSERT(NULL == _psam);
    ASSERT(FALSE == _fAppsEnumed);
    ASSERT(FALSE == _fInEnumOp);
    
    _loadstate = LS_NOTSTARTED;
}


// destructor
CDataSrc::~CDataSrc()
{
    TraceMsg(TF_OBJLIFE, "(Mtx) destroying");

    ATOMICRELEASE(_pmtxarray);
    ATOMICRELEASE(_parpevt);
    ATOMICRELEASE(_psam);
}


/*--------------------------------------------------------------------
Purpose: IUnknown::QueryInterface
*/
STDMETHODIMP CDataSrc::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDataSrc, IARPSimpleProvider),
        QITABENT(CDataSrc, OLEDBSimpleProvider),
        QITABENT(CDataSrc, ISequentialStream),
        QITABENT(CDataSrc, IWorkerEvent),
        { 0 },
    };

    HRESULT hres = QISearch(this, (LPCQITAB)qit, riid, ppvObj);
    if (FAILED(hres))
        hres = CWorkerThread::QueryInterface(riid, ppvObj);

    return hres;
}


/*--------------------------------------------------------------------
Purpose: IARPWorker::KillWT
         Kills the worker thread that enumerates apps
*/
STDMETHODIMP CDataSrc::KillWT()
{
    // Primary thread wants to kill us, this means we are about to be released
    // also kill the mtxarray thread here, because that kill has to be on the main thread, too.
    // And we can't depend on CDataSrc descrutor to do it (because that final release could be called on the
    // back groud thread) 
    _KillMtxWorkerThread();

    return CWorkerThread::KillWT();
}

/*-------------------------------------------------------------------------
Purpose: IWorkerEvent::FireOnDataReady

         Called by worker thread when some data is ready.
*/
STDMETHODIMP 
CDataSrc::FireOnDataReady(
    DBROWCOUNT iRow
    )
{
    // OSP listener expects row to be 1-based
    _parpevt->RowChanged(iRow + 1);
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IWorkerEvent::FireOnFinished

         Called by worker thread when it is complete.
*/
STDMETHODIMP 
CDataSrc::FireOnFinished(void)
{
    _loadstate = LS_DONE;
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: IWorkerEvent::FireOnDatasetChanged

         Called by worker thread when it is complete.
*/
STDMETHODIMP 
CDataSrc::FireOnDatasetChanged(void)
{
    if (_parpevt)
        _parpevt->DataSetChanged();
    return S_OK;
}

//  CDataSrc::_CalcRows
//      Calculate the number of rows in the OSP

DBROWCOUNT CDataSrc::_CalcRows(void)
{
    DBROWCOUNT lRet = 0;
    
    if (_pmtxarray)
        _pmtxarray->GetItemCount(&lRet);
        
    return lRet;    
}


//  CDataSrc::_CalcCols
//      Calculate the number of columns in the OSP

DB_LORDINAL CDataSrc::_CalcCols(void)
{
    DB_LORDINAL lRet = 0;
    
    if (_pmtxarray)
        _pmtxarray->GetFieldCount(&lRet);
        
    return lRet;
}


inline BOOL CDataSrc::_IsValidDataRow(DBROWCOUNT iRow)
{
    // Rows are 1-based.  The 0th row refers to label information.
    // -1 means wildcard.

    // The 0th row is NOT a valid data row.
    return (iRow > 0 && iRow <= _cRows);
}


inline BOOL CDataSrc::_IsValidRow(DBROWCOUNT iRow)
{
    // Rows are 1-based.  The 0th row refers to label information.
    // -1 means wildcard.
    return (iRow >= 0 && iRow <= _cRows);
}


inline BOOL CDataSrc::_IsValidCol(DB_LORDINAL iCol)
{
    // Columns are 1-based.  The 0th column refers to header information.
    // -1 means wildcard.
    return (iCol >= 1 && iCol <= _cCols);
}


inline BOOL CDataSrc::_IsValidCell(DBROWCOUNT iRow, DB_LORDINAL iCol)
{
    return _IsValidRow(iRow) && _IsValidCol(iCol);
}


/*-------------------------------------------------------------------------
Purpose: Returns the appdata object of the given row (1-based).  Returns
         NULL if there is none.
*/
IAppData * CDataSrc::_GetAppData(DBROWCOUNT iRow)
{
    IAppData * pappdata = NULL;

    if (_pmtxarray)
    {
        ASSERT(0 < iRow && iRow <= _cRows);
        _pmtxarray->GetAppData(iRow-1, &pappdata);
    }
    
    return pappdata;
}


// Structure used to transfer matrix object thru ISequentialStream()

typedef struct tagARPDSODATA
{
    LOAD_STATE  loadstate;
    DB_LORDINAL cCols;         // count of columns
    DBROWCOUNT  cRows;         // count of rows
    DWORD       dwEnum;        // items to enumerate (ENUM_*)
    IMtxArray * pmtxarray;     // data is stored here
    BSTR        bstrSort;      // sort string
} ARPDSODATA;




/*-------------------------------------------------------------------------
Purpose: ISequentialStream::Read

         Return the matrix object of this datasource object.
         IARPSimpleProvider::TransferData uses this method.
*/
STDMETHODIMP CDataSrc::Read(void * pvData, ULONG cbData, ULONG * pcbRead)
{
    HRESULT hres = E_INVALIDARG;
    
    ASSERT(IS_VALID_WRITE_BUFFER(pvData, BYTE, cbData));
    ASSERT(NULL == pcbRead || IS_VALID_WRITE_PTR(pcbRead, ULONG));
    
    if (pvData)
    {
        ARPDSODATA * pdsodata = (ARPDSODATA *)pvData;

        if (pcbRead)
            *pcbRead = 0;

        if (sizeof(*pdsodata) <= cbData)
        {
            pdsodata->loadstate = _loadstate;
            pdsodata->cCols = _cCols;
            pdsodata->cRows = _cRows;
            pdsodata->dwEnum = _dwEnum;

            pdsodata->pmtxarray = _pmtxarray;
            if (_pmtxarray)
                _pmtxarray->AddRef();
            
            pdsodata->bstrSort = _cbstrSort.Copy();

            if (pcbRead)
                *pcbRead = sizeof(*pdsodata);
        }
        hres = S_OK;
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: ISequentialStream::Write

         Set the matrix object of this datasource object.
         IARPSimpleProvider::TransferData uses this method.
*/
STDMETHODIMP CDataSrc::Write(void const * pvData, ULONG cbData, ULONG * pcbWritten)
{
    HRESULT hres = E_INVALIDARG;
    
    ASSERT(IS_VALID_READ_BUFFER(pvData, BYTE, cbData));
    ASSERT(NULL == pcbWritten || IS_VALID_WRITE_PTR(pcbWritten, ULONG));
    
    if (pvData)
    {
        ARPDSODATA * pdsodata = (ARPDSODATA *)pvData;

        if (pcbWritten)
            *pcbWritten = 0;

        if (sizeof(*pdsodata) <= cbData)
        {
            _loadstate = pdsodata->loadstate;
            _cCols = pdsodata->cCols;
            _cRows = pdsodata->cRows;
            _dwEnum = pdsodata->dwEnum;

            // We won't addref this, since the supplier should have done that.
            _pmtxarray = pdsodata->pmtxarray;

            _cbstrSort.Empty();
            _cbstrSort.Attach(pdsodata->bstrSort);

            if (pcbWritten)
                *pcbWritten = sizeof(*pdsodata);
        }
        hres = S_OK;
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::Initialize

         Must be called before enumerating items.
         
*/
STDMETHODIMP CDataSrc::Initialize(IShellAppManager * psam, IARPEvent * parpevt, DWORD dwEnum)
{
    ASSERT(psam);
    ASSERT(IS_VALID_CODE_PTR(parpevt, CEventBroker));

    ATOMICRELEASE(_psam);
    ATOMICRELEASE(_parpevt);

    _psam = psam;
    _psam->AddRef();
    
    _parpevt = parpevt;
    _parpevt->AddRef();

    _dwEnum = dwEnum;
    
    return S_OK;
}


HRESULT CDataSrc::_EnumAppItems(DWORD dwEnum, LPCWSTR pszCategory)
{
    HRESULT hres;
    IInstalledApp* pAppIns;
    CAppData* pcad;

    ASSERT(NULL == pszCategory || IS_VALID_STRING_PTRW(pszCategory, -1));
    
    switch (dwEnum)
    {
    case ENUM_INSTALLED:
        IEnumInstalledApps* pEnumIns;

        // Now that we have the object, start enumerating the items
        hres = THR(_psam->EnumInstalledApps(&pEnumIns));
        if (SUCCEEDED(hres))
        {
            // Loop through all the apps on the machine, building our table
            while (S_OK == pEnumIns->Next(&pAppIns))
            {
                // If we've been asked to bail, do so
                if (IsKilled())
                {
                    pAppIns->Release();
                    break;
                }
                
                APPINFODATA ai = {0};
                
                // Get the 'fast' app info from the app manager object
                ai.cbSize = sizeof(ai);
                ai.dwMask = AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | AIM_REGISTEREDOWNER
                               | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | AIM_SUPPORTTELEPHONE | AIM_HELPLINK
                               | AIM_INSTALLLOCATION | AIM_INSTALLDATE | AIM_COMMENTS | AIM_IMAGE
                               | AIM_READMEURL | AIM_CONTACT | AIM_UPDATEINFOURL;
                if (SUCCEEDED(pAppIns->GetAppInfo(&ai)) &&
                    lstrlen(ai.pszDisplayName) > 0)
                {
                    SLOWAPPINFO sai = {0};
                    pAppIns->GetCachedSlowAppInfo(&sai);
                    
                    // Now save all this information away
                    pcad = new CAppData(pAppIns, &ai, &sai);
                    if (pcad)
                    {
                        _pmtxarray->AddItem(pcad, NULL);
                        pcad->Release();
                    }
                    else
                    {
                        // Something failed
                        pAppIns->Release();
                        ClearAppInfoData(&ai);
                    }
                }
                // NOTE: we do NOT release the pointer (pAppIns) here,
                // its lifetime is passed to the CAppData object
            }
            pEnumIns->Release();
            hres = S_OK;
        }
        break;

    case ENUM_PUBLISHED:
        IEnumPublishedApps * pepa;      // Salt 'n...

        // Convert an empty string to a null string if we need to
        if (pszCategory && 0 == *pszCategory)
            pszCategory = NULL;
            
        // Enumerate published apps
        hres = THR(_psam->EnumPublishedApps(pszCategory, &pepa));
        if (SUCCEEDED(hres))
        {
            IPublishedApp * ppa;

            while (S_OK == pepa->Next(&ppa))
            {
                // If we've been asked to bail, do so
                if (IsKilled())
                {
                    ppa->Release();
                    break;
                }
                
                APPINFODATA ai = {0};
                
                // Get the 'fast' app info from the app manager object
                ai.cbSize = sizeof(ai);
                ai.dwMask = AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | AIM_REGISTEREDOWNER
                               | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | AIM_SUPPORTTELEPHONE | AIM_HELPLINK
                               | AIM_INSTALLLOCATION | AIM_INSTALLDATE | AIM_COMMENTS | AIM_IMAGE;
                if (SUCCEEDED(ppa->GetAppInfo(&ai)) &&
                    lstrlen(ai.pszDisplayName) > 0)
                {
                    PUBAPPINFO pai = {0};
                    pai.cbSize = sizeof(pai);
                    pai.dwMask = PAI_SOURCE | PAI_ASSIGNEDTIME | PAI_PUBLISHEDTIME;
                    ppa->GetPublishedAppInfo(&pai);
                    
                    // Now save all this information away
                    pcad = new CAppData(ppa, &ai, &pai);
                    if (pcad)
                    {
                        _pmtxarray->AddItem(pcad, NULL);
                        pcad->Release();
                    }
                    else
                    {
                        // Something failed
                        ppa->Release();
                        ClearAppInfoData(&ai);
                        ClearPubAppInfo(&pai);
                    }
                }
                // NOTE: we do NOT release the pointer (ppa) here,
                // its lifetime is passed to the CAppData object
            }
            pepa->Release();
            hres = S_OK;
        } 
        
        break;

    case ENUM_OCSETUP:
        // Create an object that enums the OCSetup items
        COCSetupEnum * pocse;
        
        pocse = new COCSetupEnum;
        if ( pocse && pocse->EnumOCSetupItems() )
        {
            COCSetupApp * pocsa;

            while ( pocse->Next(&pocsa) )
            {
                // If we've been asked to bail, do so
                if (IsKilled())
                {
                    delete pocsa;
                    break;
                }
            
                // REVIEW: Is it worth it to use an APPINFODATA structure?  COcSetupApp
                // doesn't need this structure but I think it buys us sorting once inside
                // the CAppData array as well as a free implementation of the get_DisplayName
                // property which can be accessed via script.  The data sorting might be
                // important but it might also be worth it to special case that ability.
                APPINFODATA ai = {0};
                ai.cbSize = sizeof(ai);
                ai.dwMask = AIM_DISPLAYNAME;

                if ( pocsa->GetAppInfo(&ai) && (lstrlen(ai.pszDisplayName) > 0) )
                {
                    // Now save all this information away
                    pcad = new CAppData(pocsa, &ai);
                    if (pcad)
                    {
                        _pmtxarray->AddItem(pcad, NULL);
                        pcad->Release();
                    }
                    else
                    {
                        // Something failed
                        delete pocsa;
                        ClearAppInfoData(&ai);
                    }
                }
                // NOTE: we do NOT release the pointer (pocsa) here,
                // its lifetime is passed to the CAppData object
            }
        }
        hres = S_OK;
        break;

    case ENUM_CATEGORIES:
        SHELLAPPCATEGORYLIST sacl = {0};

        // Get the list of categories
        hres = _psam->GetPublishedAppCategories(&sacl);
        if (SUCCEEDED(hres))
        {
            SHELLAPPCATEGORY * psac = sacl.pCategory;

            // If we've been asked to bail, do so
            if (IsKilled())
            {
                ReleaseShellCategory(psac);
                break;
            }
            
            UINT i;

            for (i = 0; i < sacl.cCategories; i++, psac++)
            {
                // Now save all this information away
                pcad = new CAppData(psac);
                if (pcad)
                {
                    _pmtxarray->AddItem(pcad, NULL);
                    pcad->Release();
                }
                else
                {
                    // Something failed
                    ReleaseShellCategory(psac);
                }
            }

            // NOTE: we do NOT release the pointer (sacl) here,
            // its lifetime is passed to the CAppData object
        }
        break;
    }

    return hres;
}
    

/*-------------------------------------------------------------------------
Purpose: CDataSrc::_ThreadStartProc()
         The thread proc for the background thread that enumerates applications
*/
DWORD CDataSrc::_ThreadStartProc()
{
    TraceMsg(TF_TASKS, "[%x] Starting enumerator thread", _dwThreadId);

    // Enumerate the applications, this function does the real work
    _EnumAppItems(_dwEnum, _cbstrCategory);

    // Claim to the world that we are done
    _fAppsEnumed = TRUE;
    _fInEnumOp = FALSE;

    // Tell Trident that dataset has changed
    PostWorkerMessage(WORKERWIN_FIRE_DATASETCHANGED, 0, 0);

    // Call our base class and do clean up. 
    return CWorkerThread::_ThreadStartProc();
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::Recalculate

         Recalculate the number of rows and columns and apply the sorting criteria
         for installed apps, load it's slowappinfo. 
*/
STDMETHODIMP CDataSrc::Recalculate(void)
{
    HRESULT hres = E_PENDING;
    
    if (_fAppsEnumed)
    {
        // Calculate the columns used and cache that away.
        _cCols = _CalcCols();
        _cRows = _CalcRows();
        
        // Presort the items according to the existing sort criteria
        _ApplySortCriteria(FALSE);
        
        if (0 < _cRows)
            _parpevt->RowsAvailable(0, _cRows);
        _parpevt->LoadCompleted();

        // We only get slow info for the installed apps
        if (ENUM_INSTALLED == _dwEnum)
        {
            _loadstate = LS_LOADING_SLOWINFO;

            // Create and kick off the worker thread
            IWorkerEvent * pwrkevt;
            IARPWorker * pmtxworker;

            QueryInterface(IID_IWorkerEvent, (LPVOID *)&pwrkevt);
            ASSERT(pwrkevt);        // this should never fail

            hres = _pmtxarray->QueryInterface(IID_IARPWorker, (LPVOID *)&pmtxworker);
            if (SUCCEEDED(hres))
            {
                // Tell the worker thread to notify us
                pmtxworker->SetListenerWT(pwrkevt);
                hres = pmtxworker->StartWT(THREAD_PRIORITY_BELOW_NORMAL);

                pmtxworker->Release();
            }
            pwrkevt->Release();
        }
        else
            _loadstate = LS_DONE;

        hres = S_OK;
    }
    else
    {
        // This function should only be called when app enumeration is done. 
        ASSERTMSG(FALSE, "This function should only be called when app enumeration is done");
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::EnumerateItemsAsync

         Enumerate the app items asynchronously.  This  call returns
         when all the items have been enumerated.  The caller should call
         Initialize first.
*/
STDMETHODIMP CDataSrc::EnumerateItemsAsync(void)
{
    HRESULT hres = S_OK;

    ASSERT(_parpevt);      // Caller should have called Initialize() first
    ASSERT(_psam);

    if (!_fInEnumOp)
    {
        _fInEnumOp = TRUE;
        // Make sure the slow info worker thread isn't already running. Stop it if it is.
        _KillMtxWorkerThread();

        // If we already have a list, nuke it
        ATOMICRELEASE(_pmtxarray);

        hres = THR(CMtxArray_CreateInstance(IID_IMtxArray, (LPVOID *)&_pmtxarray));
        if (SUCCEEDED(hres))
        {
            _pmtxarray->Initialize(_dwEnum);

            // Start enumerating items
            SetListenerWT(this);

            // Can't AddRef and worker thread
            hres = THR(StartWT(THREAD_PRIORITY_NORMAL));
        }
        else
            // Let people try again. 
            _fInEnumOp = FALSE;
    }
    else
    {
        // This function should only be called before any enumeration started
        ASSERTMSG(FALSE, "This function should only be called before any enumeration started");
        hres = E_PENDING;
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: Sorts the data
*/
HRESULT CDataSrc::_ApplySortCriteria(BOOL bFireDataSetChanged)
{
    HRESULT hres = E_FAIL;

    if (_pmtxarray)
    {
        _pmtxarray->SetSortCriteria(_cbstrSort);
        
        hres = _pmtxarray->SortItems();
        if (SUCCEEDED(hres))
        {
            // Mark the duplicated name entries for published apps
            if ((ENUM_PUBLISHED == _dwEnum) && !StrCmpW(_cbstrSort, L"displayname"))
                _pmtxarray->MarkDupEntries();
        	
            // Tell the databinding agent that our dataset changed
            if (bFireDataSetChanged)
                _parpevt->DataSetChanged();
        }
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::SetSortCriteria
         Set the sort criterion for the datasource.

         Returns S_OK if the sort criteria is different, S_FALSE if it is
         no different.

         bstrSortExpr       Name of column to sort by ("" = no sorting)
         
*/
STDMETHODIMP CDataSrc::SetSortCriteria(BSTR bstrSortExpr) 
{
    HRESULT hres = S_FALSE;

    // Is this a new sort criteria?
    if (NULL == (LPWSTR)_cbstrSort || 0 != StrCmpIW(bstrSortExpr, _cbstrSort))
    {
        // Yes
        _cbstrSort = bstrSortExpr;
        hres = S_OK;
        _fSortDirty = TRUE;
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::SetFilter
         Set the filter criterion for the datasource.  Right now this only
         works for published apps, via a category.

         Returns S_OK if the filter criteria is different, S_FALSE if it is
         no different.

         bstrSortExpr       Name of column to sort by ("" = no sorting)
         
*/
STDMETHODIMP CDataSrc::SetFilter(BSTR bstrFilter) 
{
    HRESULT hres = S_FALSE;

    // Is this a new filter criteria?
    if (NULL == (LPWSTR)_cbstrCategory || 0 != StrCmpIW(bstrFilter, _cbstrCategory))
    {
        // Yes
        _cbstrCategory = bstrFilter;
        hres = S_OK;
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::Sort

         Initiates a sort operation if any of the changes invalidates the 
         existing criteria.
*/
STDMETHODIMP CDataSrc::Sort(void) 
{
    HRESULT hres = S_OK;

    if (_fSortDirty)
    {
        // Is the datasource started?
        if (LS_NOTSTARTED != _loadstate)
        {
            // Yes; we can apply the sort now
            hres = _ApplySortCriteria(TRUE);
            if (SUCCEEDED(hres))
                _fSortDirty = FALSE;
        }
    }

    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::TransferData

         Transfer the contents of given datasource object to this datasource.  
         This is useful for operations that change the dataset in-place, 
         like sorting.
*/
STDMETHODIMP CDataSrc::TransferData(IARPSimpleProvider * parposp)
{
    HRESULT hres;
    ISequentialStream * pstream;
    
    ASSERT(parposp);

    hres = parposp->QueryInterface(IID_ISequentialStream, (LPVOID *)&pstream);
    if (SUCCEEDED(hres))
    {
        IARPWorker * pmtxworker;
        ARPDSODATA dsodata;
        ULONG cb;

        // Transfer the state and data from that datasource to this
        // datasource.
        pstream->Read(&dsodata, sizeof(dsodata), &cb);
        Write(&dsodata, cb, NULL);

        if (_pmtxarray)
        {
            hres = _pmtxarray->QueryInterface(IID_IARPWorker, (LPVOID *)&pmtxworker);
            if (SUCCEEDED(hres))
            {
                // Tell the worker thread that this is the new datasource 
                // object to receive events
                IWorkerEvent * pwrkevt;
                
                QueryInterface(IID_IWorkerEvent, (LPVOID *)&pwrkevt);
                ASSERT(pwrkevt);        // this should never fail
                
                pmtxworker->SetListenerWT(pwrkevt);
                pmtxworker->Release();

                pwrkevt->Release();
            }

            _fAppsEnumed = TRUE;
        }        
        pstream->Release();
    }
    
    return hres;
}


/*-------------------------------------------------------------------------
Purpose: IARPSimpleProvider::DoCommand

         Commit a specific action on the record.  Unlike standard
         ADO commands that affect a recordset, these commands
         are intended to be specific to managing the apps themselves
         (like installing or uninstalling).

         NOTE: this method is called indirectly via script.
*/
STDMETHODIMP CDataSrc::DoCommand(HWND hwndParent, APPCMD appcmd, DBROWCOUNT iRow)
{
    HRESULT hres = S_OK;
    
    IAppData * pappdata = _GetAppData(iRow);
    if (pappdata)
    {
        if (_IsValidDataRow(iRow))
        {
            hres = pappdata->DoCommand(hwndParent, appcmd);

            // Was the app succesfully uninstalled/changed/whatever?
            if (S_OK == hres)
            {
                // Yes
                DBROWCOUNT lDeleted;
                
                switch (appcmd)
                {
                case APPCMD_UNINSTALL:
                    // Fire the event to the databinding agent
                    deleteRows(iRow, 1, &lDeleted); 
                    break;

                case APPCMD_UPGRADE:
                case APPCMD_REPAIR:
                case APPCMD_MODIFY:
                case APPCMD_INSTALL:
                    // Fire the event
                    _parpevt->RowChanged(iRow);
                    break;
                }
            }
        }
        pappdata->Release();
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getRowCount

         Return the number of rows in the table.
*/
STDMETHODIMP CDataSrc::getRowCount(DBROWCOUNT *pcRows)
{
    ASSERT(IS_VALID_WRITE_PTR(pcRows, DBROWCOUNT));

    *pcRows = _cRows;
        
    TraceMsg(TF_DSO, "(Mtx) getRowCount returning %d", _cRows);
    
    return S_OK;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getColumnCount

         Return the number of columns in the table.
*/
STDMETHODIMP CDataSrc::getColumnCount(DB_LORDINAL *pcCols)
{
    ASSERT(IS_VALID_WRITE_PTR(pcCols, DB_LORDINAL));

    *pcCols = _cCols;

    TraceMsg(TF_DSO, "(Mtx) getColumnCount returning %d", _cCols);

    return S_OK;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getRWStatus

         Gets the read/write status of a cell, row, column or the 
         entire array.  

         This implementation cannot set the read/write status on any
         cell, so all data cells are presumed to have the default
         access and all column heading cells are presumed to be 
         read-only.  Therefore, we don't keep track of this info
         in the individual cells.

         E_INVALIDARG - returned if indices are out of bounds
*/
STDMETHODIMP CDataSrc::getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus)
{
    HRESULT hres = E_INVALIDARG;

    if ((_IsValidRow(iRow) || -1 == iRow) && 
        (_IsValidCol(iCol) || -1 == iCol))
    {
        if (iRow == -1)
        {
            // BUGBUG (scotth): this comment is copied from the TDC
            //  sample app.  Is it still valid?
            //
            //  Should return READONLY if there is only a label row,
            //  but frameworks tend to get confused if they want to
            //  later insert data.
            //
//          *prwStatus = m_iDataRows > 0 ? OSPRW_MIXED : OSPRW_READONLY;
            *prwStatus = OSPRW_MIXED;
        }
        else if (iRow == 0)
            *prwStatus = OSPRW_READONLY;
        else
            *prwStatus = OSPRW_DEFAULT;
        hres = S_OK;
    }

    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) getRWStatus(%d, %d) failed %s", iRow, iCol, Dbg_GetHRESULT(hres));
    else        
        TraceMsg(TF_DSO, "(Mtx) getRWStatus(%d, %d) returning %s", iRow, iCol, Dbg_GetOSPRW(*prwStatus));
    
    return hres;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getVariant

         Retrieves a variant value for a cell.
*/
STDMETHODIMP CDataSrc::getVariant(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT * pvar)
{
    HRESULT hres  = E_INVALIDARG;

    TraceMsg(TF_DSO, "(Mtx) getVariant(%d, %d)", iRow, iCol);
    
    ASSERT(IS_VALID_WRITE_PTR(pvar, VARIANT));
    
    if (_IsValidCell(iRow, iCol))
    {
        VARIANT var;

        // Massage col to be 0-based
        iCol--;
        
        // Are they asking for the field name?
        if (0 == iRow)
        {
            // Yes; get the field name
            if (_pmtxarray)
                hres = _pmtxarray->GetFieldName(iCol, &var);
            else
                hres = E_FAIL;
        }
        else
        {
            // No; get the field value
            IAppData * pappdata = _GetAppData(iRow);
            if (pappdata)
            {
                hres = pappdata->GetVariant(iCol, &var);
                pappdata->Release();
            }
            else
                hres = E_FAIL;
        }
            
        if (SUCCEEDED(hres))
        {
            if (OSPFORMAT_RAW == format)
            {
                // Copy the raw variant value
                *pvar = var;
            }
            else if (OSPFORMAT_FORMATTED == format || OSPFORMAT_HTML == format)
            {
                // Consumer wants it in text format
                if (VT_BSTR == var.vt || VT_EMPTY == var.vt)
                {
                    // Already done
                    *pvar = var;
                }
                else if (VT_UI4 == var.vt)
                {
                    // Coerce
                    VarBstrFromUI4( var.lVal, 0, 0, &(pvar->bstrVal));
                    if (pvar->bstrVal != NULL)
                    {
                        pvar->vt = VT_BSTR;
                    }
                    else
                        hres = E_OUTOFMEMORY;
                }
                else
                    hres = E_NOTIMPL;
            }
            else
                hres = E_INVALIDARG;

            if (FAILED(hres))
            {
                VariantClear(&var);
                pvar->vt = VT_BSTR;
                pvar->bstrVal = SysAllocString(L"#Error");
            }
        }
    }

    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) getVariant failed %s", Dbg_GetHRESULT(hres));
    
    return hres;
}



/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::setVariant

         Set a cell's variant value from a given variant.  The given variant
         type is coerced into the columns underlying type.
*/
STDMETHODIMP CDataSrc::setVariant(DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT var)
{
    HRESULT hres = E_INVALIDARG;

    TraceMsg(TF_DSO, "(Mtx) setVariant(%d, %d)", iRow, iCol);

#ifdef NYI
    if (_IsValidCol(iCol))
    {
        // Massage col to be 0-based
        iCol--;
        
        // Is the data agent trying to change an existing cell?
        if (_IsValidDataRow(iRow))
        {
            // Yes
            IAppData * pappdata = _GetAppData(iRow);
            if (pappdata)
            {
                hres = pappdata->SetVariant(iCol, &var);
                pappdata->Release();
            }
            else
                hres = E_FAIL;
        }
        else
        {
            // No; it wants to add a new row
        }
    }
#else
    hres = E_NOTIMPL;
#endif

    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) setVariant failed %s", Dbg_GetHRESULT(hres));
    
    return hres;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getLocale

         Returns to the consumer the locale of the data we
         are providing.  App management data is in the locale
         of the system, so return an empty bstr.
         
*/
STDMETHODIMP CDataSrc::getLocale(BSTR *pbstrLocale)
{
    TraceMsg(TF_DSO, "(Mtx) getLocale");
    
    *pbstrLocale = SysAllocString(L"");
    return *pbstrLocale ? S_OK : E_OUTOFMEMORY;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::deleteRows

         Used to delete rows from the array.  Bounds are checked
         to make sure that the rows can all be deleted.  Label
         rows cannot be deleted.

         E_INVALIDARG - returned if any rows to be deleted are 
                        out of bounds
*/
STDMETHODIMP CDataSrc::deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    HRESULT hres = E_INVALIDARG;

    TraceMsg(TF_DSO, "(Mtx) deleteRows(%d, %d)", iRow, cRows);
    
    *pcRowsDeleted = 0;

    if (_IsValidDataRow(iRow) && cRows >= 0 &&
        _IsValidDataRow(iRow + cRows - 1))
    {
        _parpevt->AboutToDeleteRows(iRow, cRows);

        *pcRowsDeleted = cRows;
        if (cRows > 0)
        {
            //  Delete the rows from the array
            
            _pmtxarray->DeleteItems(iRow - 1, cRows);
            _cRows = _CalcRows();

            //  Notify the event-handler of the deletion
            _parpevt->DeletedRows(iRow, cRows);
        }
        hres = S_OK;
    }

    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) deleteRows failed %s", Dbg_GetHRESULT(hres));
    
    return hres;
}


//+-----------------------------------------------------------------------
//
//  Member:    InsertRows()
//
//  Synopsis:  Allows for the insertion of new rows.  This can either be
//             used to insert new rows between existing rows, or to
//             append new rows to the end of the table.  Thus, to
//             insert new rows at the end of the table, a user would
//             specify the initial row as 1 greater than the current
//             row dimension.
//             Note that iRow is checked to ensure that it is within the
//             proper bounds (1..<current # of rows>+1).
//             User cannot delete column heading row.
//
//  Arguments: iRow            rows will be inserted *before* row 'iRow'
//             cRows           how many rows to insert
//             pcRowsInserted  actual number of rows inserted (OUT)
//
//  Returns:   S_OK upon success, i.e. all rows could be inserted.
//             E_INVALIDARG if row is out of allowed bounds.
//             It is possible that fewer than the requested rows were
//             inserted.  In this case, E_OUTOFMEMORY would be returned,
//             and the actual number of rows inserted would be set.
//
//------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::insertRows

*/
STDMETHODIMP CDataSrc::insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    HRESULT hres  = E_NOTIMPL;

    TraceMsg(TF_DSO, "(Mtx) insertRows(%d, %d)", iRow, cRows);
    
    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) insertRows failed %s", Dbg_GetHRESULT(hres));
    
    return hres;
}


//+-----------------------------------------------------------------------
//
//  Member:    Find()
//
//  Synopsis:  Searches for a row matching the specified criteria
//
//  Arguments: iRowStart       The starting row for the search
//             iCol            The column being tested
//             vTest           The value against which cells in the
//                               test column are tested
//             findFlags       Flags indicating whether to search up/down
//                               and whether comparisons are case sensitive.
//             compType        The comparison operator for matching (find a
//                             cell =, >=, <=, >, <, <> the test value)
//             piRowFound      The row with a matching cell [OUT]
//
//  Returns:   S_OK upon success, i.e. a row was found (piRowFound set).
//             E_FAIL upon failure, i.e. a row was not found.
//             E_INVALIDARG if starting row 'iRowStart' or test column 'iCol'
//               are out of bounds.
//             DISP_E_TYPEMISMATCH if the test value's type does not match
//               the test column's type.
//
//------------------------------------------------------------------------

/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::find

*/
STDMETHODIMP CDataSrc::find(DBROWCOUNT iRowStart, DB_LORDINAL iCol, VARIANT vTest,
        OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound)
{
    HRESULT hres  = E_NOTIMPL;

    TraceMsg(TF_DSO, "(Mtx) find(%d, %d)", iRowStart, iCol);
    
    if (FAILED(hres))
        TraceMsg(TF_WARNING, "(Mtx) find failed %s", Dbg_GetHRESULT(hres));
    
    return hres;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::addOLEDBSimpleProviderListener

         Sets or clears a reference to the OSP listener.
*/
STDMETHODIMP CDataSrc::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospl)
{
    HRESULT hres;

    TraceMsg(TF_DSO, "(Mtx) addOLEDBSimpleProviderListener  <%s>", Dbg_GetLS(_loadstate));
    
    if (_parpevt == NULL)
        hres = E_FAIL;
    else
    {
        _parpevt->SetOSPListener(pospl);
        
        // If the event sink has been added, and we're already loaded,
        // then fire transferComplete, because we probably couldn't before.
        if (LS_NOTSTARTED < _loadstate)
            _parpevt->LoadCompleted();

        hres = S_OK;
    }

    return hres;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::removeOLEDBSimpleProviderListener

*/
STDMETHODIMP CDataSrc::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener * pospl)
{
    if (_parpevt && S_OK == _parpevt->IsOSPListener(pospl))
    {
        TraceMsg(TF_DSO, "(Mtx) removeOLEDBSimpleProviderListener");
        
        _parpevt->SetOSPListener(NULL);
    }
    return S_OK;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::getEstimatedRows

         Returns an estimated number of rows in the matrix.
         Return -1 if unknown.
*/
STDMETHODIMP CDataSrc::getEstimatedRows(DBROWCOUNT *pcRows)
{
    if (LS_NOTSTARTED == _loadstate)
        *pcRows = -1;
    else
        *pcRows = _cRows;

    TraceMsg(TF_DSO, "(Mtx) getEstimatedRows returning %d  <%s>", *pcRows, Dbg_GetLS(_loadstate));
    
    return S_OK;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::isAsync

*/
STDMETHODIMP CDataSrc::isAsync(BOOL *pbAsync)
{
    // This OSP always behaves as if it is async.  Specifically, we always fire
    // TransferComplete, even if we have to buffer the notification until our
    // addOLEDBSimplerProviderListener is actually called.
    *pbAsync = TRUE;
    return S_OK;
}


/*----------------------------------------------------------
Purpose: OLEDBSimpleProvider::stopTransfer

         The data download has been cancelled.
*/
STDMETHODIMP CDataSrc::stopTransfer()
{
    TraceMsg(TF_DSO, "(Mtx) stopTransfer  <%s>", Dbg_GetLS(_loadstate));
    
    //  Force the load state into UNINITIALISED or LOADED ...
    //
    switch (_loadstate)
    {
    case LS_NOTSTARTED:
    case LS_DONE:
        // No need to do anything, because we either haven't started
        // or are already finished.
        break;

    case LS_LOADING_SLOWINFO:
        // Stop the worker thread.
        _KillMtxWorkerThread();

        // Say we're done
        _loadstate = LS_DONE;
        
        TraceMsg(TF_DSO, "(Mtx) Setting state to <%s>", Dbg_GetLS(_loadstate));
        
        // Fire an abort event
        _parpevt->LoadAborted();
        break;
    }

    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Helper method to kill the worker thread
*/
HRESULT CDataSrc::_KillMtxWorkerThread(void)
{
    HRESULT hres = S_OK;

    if (_pmtxarray)
    {
        IARPWorker * pmtxworker;
        
        hres = _pmtxarray->QueryInterface(IID_IARPWorker, (LPVOID *)&pmtxworker);
        if (SUCCEEDED(hres))
        {
            hres = pmtxworker->KillWT();
            pmtxworker->Release();
        }
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: Create-instance function for CDataSrc

*/
HRESULT CDataSrc_CreateInstance(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres = E_OUTOFMEMORY;

    *ppvObj = NULL;
    
    CDataSrc * pObj = new CDataSrc();
    if (pObj)
    {
        hres = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }

    return hres;
}

#endif //DOWNLEVEL_PLATFORM
