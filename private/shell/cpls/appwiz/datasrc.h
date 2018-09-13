//+-----------------------------------------------------------------------
//
//  Add/Remove Programs Data Source Object
//
//------------------------------------------------------------------------

#ifndef _DATASRC_H_
#define _DATASRC_H_

#include <simpdata.h>       // for OLEDBSimpleProvider
#include "mtxarray.h"       // for CMtxArray
#include "worker.h"
#include "iface.h"          // for IARPSimpleProvider


// The load state progression can be as follows:
//
//  LS_NOTSTARTED  -->  LS_LOADING_SLOWINFO -->  LS_DONE
//
// or
//
//  LS_NOTSTARTED  -->  LS_DONE
//  
enum LOAD_STATE
{
    LS_NOTSTARTED,                  // OSP is not initialized
    LS_LOADING_SLOWINFO,            // loading slow info (worker thread)
    LS_DONE,                        // completely finished
};


// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM
//------------------------------------------------------------------------
//
//  CDataSrc (ARP Data Source Object)
//
//  This is the OSP (OLEDB Simple Provider).  It organizes
//  the data in matrix form and disseminates the data to the data
//  consumer via the OLEDBSimpleProvider interface.
//
//------------------------------------------------------------------------

class CDataSrc : public CWorkerThread,
                public OLEDBSimpleProvider,
                public IWorkerEvent,
                public IARPSimpleProvider,
                public ISequentialStream
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef   (void) {return CWorkerThread::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release  (void) {return CWorkerThread::Release();};
    virtual STDMETHODIMP QueryInterface   (REFIID riid, LPVOID * ppvObj);

    // *** ISequentialStream ***
    STDMETHOD(Read)             (void * pvData, ULONG cbData, ULONG * pcbRead);
    STDMETHOD(Write)            (void const * pvData, ULONG cbData, ULONG * pcbWritten);
    
    // *** OLEDBSimpleProvider ***
    STDMETHOD(getRowCount)      (DBROWCOUNT *pcRows);
    STDMETHOD(getColumnCount)   (DB_LORDINAL *pcCols);
    STDMETHOD(getRWStatus)      (DBROWCOUNT iRow, DB_LORDINAL iCol, OSPRW *prwStatus);
    STDMETHOD(getVariant)       (DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT *pVar);
    STDMETHOD(setVariant)       (DBROWCOUNT iRow, DB_LORDINAL iCol, OSPFORMAT format, VARIANT Var);
    STDMETHOD(getLocale)        (BSTR *pbstrLocale);
    STDMETHOD(deleteRows)       (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    STDMETHOD(insertRows)       (DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);
    STDMETHOD(find)             (DBROWCOUNT iRowStart, DB_LORDINAL iCol, VARIANT val,
                                 OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound);
    STDMETHOD(addOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(removeOLEDBSimpleProviderListener)(OLEDBSimpleProviderListener *pospIListener);
    STDMETHOD(getEstimatedRows) (DBROWCOUNT *pcRows);
    STDMETHOD(isAsync)          (BOOL *pbAsync);
    STDMETHOD(stopTransfer)     (void);

    // *** IWorkerEvent methods ***
    STDMETHOD(FireOnDataReady)  (DBROWCOUNT iRow);
    STDMETHOD(FireOnFinished)   (void);
    STDMETHOD(FireOnDatasetChanged)   (void);
    
    // *** IARPSimpleProvider methods ***
    STDMETHOD(Initialize)       (IShellAppManager * psam, IARPEvent *, DWORD dwEnum);
    STDMETHOD(EnumerateItemsAsync)   (void);
    STDMETHOD(Recalculate)      (void);
    STDMETHOD(SetSortCriteria)  (BSTR bstrSortExpr);
    STDMETHOD(SetFilter)        (BSTR bstrFilter);
    STDMETHOD(Sort)             (void);
    STDMETHOD(DoCommand)        (HWND hwndParent, APPCMD appcmd, DBROWCOUNT iRow);
    STDMETHOD(TransferData)     (IARPSimpleProvider * parposp);
    CDataSrc();

    // *** IARPWorker *** (overide)
    STDMETHOD(KillWT)           (void);
    
private:

    virtual ~CDataSrc();

    inline BOOL _IsValidDataRow(DBROWCOUNT iRow);
    inline BOOL _IsValidRow(DBROWCOUNT iRow);
    inline BOOL _IsValidCol(DB_LORDINAL iCol);
    inline BOOL _IsValidCell(DBROWCOUNT iRow, DB_LORDINAL iCol);

    DBROWCOUNT _CalcRows(void);
    DB_LORDINAL _CalcCols(void);
    HRESULT _ApplySortCriteria(BOOL bFireDataSetChanged);
    IAppData * _GetAppData(DBROWCOUNT iRow);

    // NOTE: this is not used to kill the apps enumeration thread!!
    // kills only the thread that get slow appinfo. 
    HRESULT _KillMtxWorkerThread(void);
    HRESULT _EnumAppItems(DWORD dwEnum, LPCWSTR pszCategory);
    DWORD   _ThreadStartProc();
            
    ULONG        _cRef;          // interface reference count
    LOAD_STATE  _loadstate;
    DB_LORDINAL _cCols;         // count of columns
    DBROWCOUNT  _cRows;         // count of rows

    DWORD       _dwEnum;        // items to enumerate (ENUM_*)
    BITBOOL     _fSortDirty: 1; // TRUE: the sort criteria is dirty
    BITBOOL     _fFilterDirty: 1; // TRUE: the sort criteria is dirty
    BITBOOL     _fAppsEnumed : 1; // TRUE if we have already finished enumerating apps
    BITBOOL     _fInEnumOp : 1;   // TRUE if we are in a enumeraion opertion
    IShellAppManager * _psam;
    IARPEvent * _parpevt;
    IMtxArray * _pmtxarray;     // data is stored here
    CComBSTR    _cbstrSort;     // sort string.  contains name of column to sort.
    CComBSTR    _cbstrCategory; // category.  used only for published apps.
};


HRESULT CDataSrc_CreateInstance(REFIID riid, LPVOID * ppvObj);

#endif //DOWNLEVEL_PLATFORM

#endif // _DATASRC_H_
