//+------------------------------------------------------------------------
//
//  File:       TRDNTOSP.CXX
//
//  Contents:   Object for HTML based OSP
//
//  Classes:    (part of) CDoc
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include "simpdata.h"
#endif

#ifndef X_MSDATSRC_H_
#define X_MSDATSRC_H_
#include "msdatsrc.h"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#include "commctrl.h"
#endif

#ifndef X_DOCGLBS_HXX_
#define X_DOCGLBS_HXX_
#include "docglbs.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

MtDefine(TridentOSP, DataBind, "TridentOSP")
MtDefine(TridentOSP_apaBSTRtagData_pv, TridentOSP, "TridentOSP::_apaBSTRtagData::_pv")
MtDefine(TridentOSPInit_pDataColumn, Locals, "TridentOSP::Init pDataColumn")
MtDefine(TridentOSPInit_pDataColumn_pv, Locals, "TridentOSP::Init pDataColumn::_pv")

BEGIN_TEAROFF_TABLE(CDoc, DataSource)
    TEAROFF_METHOD(CDoc, getDataMember, getdatamember, (DataMember bstrDM, REFIID riid, IUnknown **ppunk))
    TEAROFF_METHOD(CDoc, getDataMemberName, getdatamembername, (long lIndex, DataMember *pbstrDM))
    TEAROFF_METHOD(CDoc, getDataMemberCount, getdatamembercount, (long *plCount))
    TEAROFF_METHOD(CDoc, addDataSourceListener, adddatasourcelistener, (DataSourceListener *pDSL))
    TEAROFF_METHOD(CDoc, removeDataSourceListener, removedatasourcelistener, (DataSourceListener *pDSL))
END_TEAROFF_TABLE()

class TridentOSP:OLEDBSimpleProvider
{ 
public:

    DECLARE_MEMALLOC_NEW_DELETE(Mt(TridentOSP))

// Constructors
    TridentOSP(CDoc *pDoc);

// Destructor
    ~TridentOSP();

// Initializer
    HRESULT Init();
// IOLEDBSimpleProvider
    
    virtual HRESULT STDMETHODCALLTYPE getRowCount(DBROWCOUNT *pcRows);
    virtual HRESULT STDMETHODCALLTYPE getColumnCount(DB_LORDINAL *pcColumns);
    virtual HRESULT STDMETHODCALLTYPE getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus);
    virtual HRESULT STDMETHODCALLTYPE getVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT *pVar);
    virtual HRESULT STDMETHODCALLTYPE setVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT Var);
    virtual HRESULT STDMETHODCALLTYPE getLocale(BSTR *pbstrLocale);
    virtual HRESULT STDMETHODCALLTYPE deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted);
    virtual HRESULT STDMETHODCALLTYPE insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted);
    virtual HRESULT STDMETHODCALLTYPE find(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val, OSPFIND findflags, OSPCOMP compType, DBROWCOUNT *piRowFound);
    virtual HRESULT STDMETHODCALLTYPE addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener);
    virtual HRESULT STDMETHODCALLTYPE removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener);
    virtual HRESULT STDMETHODCALLTYPE isAsync(BOOL *pbAsynch);
    virtual HRESULT STDMETHODCALLTYPE getEstimatedRows(DBROWCOUNT *piRows);
    virtual HRESULT STDMETHODCALLTYPE stopTransfer(void);

//  IUnknown Interfaces
    virtual HRESULT __stdcall QueryInterface(const IID& iid, void **ppv);
    virtual ULONG __stdcall AddRef();
    virtual ULONG __stdcall Release();

private:
    long _refCount;
    long _rowCount;
    CDoc *_pDoc;
    OLEDBSimpleProviderListener *_pospMyListener;
    CDataAry< CDataAry<BSTR> *> _apaBSTRtagData;

#ifdef OBJCNTCHK
    DWORD _dwObjCnt;
#endif

};


//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::TridentOSP(CDoc *pDoc)
//
//  Synopsis:   Class constructor which takes a pointer to CDoc, gets the 
//              IHTML Collection and stores the tags with ID's in a dynamically
//              allocated array of type CDataAry. Called upon "new TridentOSP"
//
//  Returns:    no return value
//
//-------------------------------------------------------------------------

TridentOSP::TridentOSP(CDoc *pDoc)
    : _apaBSTRtagData(Mt(TridentOSP_apaBSTRtagData_pv))
{
    _pDoc = pDoc;

// reference counting
    _refCount = 1;
    _rowCount = 0;
    _pDoc->SubAddRef();
    IncrementObjectCount(&_dwObjCnt);

    return;
}

HRESULT
TridentOSP::Init() 
{   
    long i, j, dataLength, dataIndex;
    HRESULT hr;
    VARIANT v, vEmpty;
    IDispatch *pout = NULL;
    IHTMLElement *pout2 = NULL;
    
    CDataAry<BSTR> *pDataColumn, *pDataPtr;
    IHTMLElementCollection *pElementCollection; 
    dataIndex = 0;


//initialization of 2D tag data array

    _pDoc->PrimaryMarkup()->GetCollection(CMarkup::ELEMENT_COLLECTION, &pElementCollection);
    hr = pElementCollection->get_length(&dataLength);
    if(hr)
        return hr;

    V_VT(&vEmpty) = VT_EMPTY;
    V_VT(&v) = VT_I4;

    for (i = 0; i < dataLength; i++) 
    {
        BSTR pstrID = NULL, pstrData = NULL;

        V_I4(&v) = i;
        hr = pElementCollection->item(v, vEmpty, &pout);
        if(hr)
            goto Cleanup;

        pout->QueryInterface(IID_IHTMLElement, (void **)&pout2);
        hr = pout2->get_id(&pstrID);
        if (hr)
            goto Cleanup;

        hr = pout2->get_innerHTML(&pstrData);
        // <HTML> tag gives hr error on get_innerHTML.  Must check ht value and make sure tag is not
        // empty
        if(pstrID && !hr)
        {
            pDataColumn = NULL;
            
            for ( j = 0; j < _apaBSTRtagData.Size(); j++)
            { 
                // Search for a duplicate tag ID in existing structure
                pDataPtr = _apaBSTRtagData.Item(j);

                if (! _tcscmp(pstrID, pDataPtr->Item(0)))       
                    pDataColumn = pDataPtr;
            }

            if (!pDataColumn)
                {       // no match.  Insert new tag id to _apaBSTRtagData
                pDataColumn = new(Mt(TridentOSPInit_pDataColumn)) CDataAry<BSTR>(Mt(TridentOSPInit_pDataColumn_pv));
                if (pDataColumn)
                {
                    _apaBSTRtagData.InsertIndirect(dataIndex, &pDataColumn);
                    pDataColumn->InsertIndirect(0, &pstrID);      
                    dataIndex++; 
                }
            }

            if (pDataColumn)
            {
                pDataColumn->InsertIndirect(pDataColumn->Size(), &pstrData);
                    if (pDataColumn->Size()-1 > _rowCount)
                    _rowCount = pDataColumn->Size()-1;          
            }
        }
        else
        {
           if(pstrData)
               SysFreeString(pstrData);
           if(pstrID)
               SysFreeString(pstrID);
        }

        ClearInterface(&pout);
        ClearInterface(&pout2);
    }


Cleanup:
    ReleaseInterface(pElementCollection);
    ReleaseInterface(pout);
    ReleaseInterface(pout2);

return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::~TridentOSP()
//
//  Synopsis:   Class deconstructor which releases the CDoc reference and 
//              decrements the object count
//
//  Returns:    no return value
//
//-------------------------------------------------------------------------

TridentOSP::~TridentOSP()
{
    CDataAry<BSTR> **pElem;

    int    i, j;

    for (i = _apaBSTRtagData.Size(), pElem = _apaBSTRtagData;
         i > 0;
         i--, pElem++)
    {
        for (j = (*pElem)->Size()-1;
             j >= 0;
             j--)  
        {
             SysFreeString((*pElem)->Item(j));
        }

        (*pElem)->DeleteAll();
        delete *pElem;        
    }
    _apaBSTRtagData.DeleteAll();

    _pDoc->SubRelease();
    DecrementObjectCount(&_dwObjCnt);
    ReleaseInterface(_pospMyListener);
    return;
}


//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::getRowCount(long *pcRows)
//
//  Synopsis:   Returns the number of rows of data.   For now, the number of 
//              rows is only 2, but future revisions will allow access of 
//              multiple rows for duplicate tag ID's 
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::getRowCount(DBROWCOUNT *pcRows)
{
    *pcRows = _rowCount;
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::GetColumnCount(long *pcColumns)
//
//  Synopsis:   Returns the number of columns in the _apaBSTRtagData structure
//              which corresponds to the number of non-empty, unique HTML Tag
//              ID's in the data source.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::getColumnCount(DB_LORDINAL *pcColumns)
{
    *pcColumns = _apaBSTRtagData.Size();
    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::getRWStatus(long iRow, long iColumn, OSPRW *prwStatus)
//
//  Synopsis:   Returns the Read/Write Status of the structure.   All elements
//              are read-only
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus)
{
    *prwStatus = OSPRW_READONLY;
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::getVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT *pVar)
//
//  Synopsis:   Gets the desired Variant from the data structure and returns it. 
//              Row 0 is the Tag ID, and any other row is the Inner HTML.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::getVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT *pVar)
{
    CDataAry<BSTR> *pDataColumn;
    BSTR tempBSTR;

    V_VT(pVar) = VT_BSTR;

    pDataColumn = _apaBSTRtagData.Item(iColumn - 1);

    if (iRow == 0)
    {
       tempBSTR = SysAllocString(pDataColumn->Item(0));
       V_BSTR(pVar) = tempBSTR;
    }

    else
    {   
        if (iRow >= pDataColumn->Size())
            tempBSTR = SysAllocString(_T(""));
        else
            tempBSTR = SysAllocString(pDataColumn->Item(iRow));
        
        V_BSTR(pVar) = tempBSTR;
    }       

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::setVariant(long iRow, long iColumn, OSPFORMAT format, VARIANT Var)
//
//  Synopsis:   Unimplemented.   setVariant should never be called since all the 
//              elements are READONLY  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::setVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT Var)
{
    return E_FAIL;
}



//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::getLocale(BSTR *pbstrLocale)
//
//  Synopsis:   Unimplemented.    
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::getLocale(BSTR *pbstrLocale)
{
// BUGBUG (t-trevs) this should be implemented
    *pbstrLocale = NULL;
    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::deleteRows(long iRow, long cRows, long *pcRowsDeleted)
//
//  Synopsis:   Unimplemented.  Data structure is read-only.  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    *pcRowsDeleted = 0;
    return E_FAIL;
}



//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::insertRows(long iRow, long cRows, long *pcRowsInserted)
//
//  Synopsis:   Unimplemented.  Data structure is read-only.  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    *pcRowsInserted = 0;
    return E_FAIL;
}


//+------------------------------------------------------------------------
//
//  Member:     TridentOSP::find(long iRowStart, long iColumn, VARIANT val, 
//                               OSPFIND findflags, OSPCOMP compType, long *piRowFound)
//
//  Synopsis:   Unimplemented.   
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::find(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val, OSPFIND findflags, OSPCOMP compType, DBROWCOUNT *piRowFound)
{
// BUGBUG (t-trevs) we should probably implement TridentOSP::Find()
    return E_FAIL;
}



//+------------------------------------------------------------------------
//
//  Member:    addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
//
//  Synopsis:   Sets current OLEDBSimpleProviderListener to input pointer.   Sets
//              the member variable.  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    _pospMyListener = pospIListener;
    pospIListener->AddRef();
    return S_OK;
}



//+------------------------------------------------------------------------
//
//  Member:     removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
//
//  Synopsis:   removes the current listener and releases the interface.  Clears the 
//              member variable.  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    Assert(_pospMyListener == pospIListener);
    _pospMyListener = NULL;
    ReleaseInterface(pospIListener);

    return S_OK;
}


//+------------------------------------------------------------------------
//  Member:     TridentOSP::isAsync(BOOL *pbAsynch)
//
//  Synopsis:   Implementation does not support Asynchronous data transfer 
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::isAsync(BOOL *pbAsynch)
{
    *pbAsynch = 0;
    return S_OK;
}


//+------------------------------------------------------------------------
//  Member:     TridentOSP::getEstimatedRows(long *piRows)
//
//  Synopsis:   Returns unspecified estimation of rows  
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE
TridentOSP::getEstimatedRows(DBROWCOUNT *piRows)
{
    *piRows = -1;
    return S_OK;
}



//+------------------------------------------------------------------------
//  Member:     TridentOSP::stopTransfer(void)
//
//  Synopsis:   Unimplemented.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
TridentOSP::stopTransfer(void)
{
    return S_OK;
}




//+------------------------------------------------------------------------
//  Member:     TridentOSP::AddRef()
//
//  Synopsis:   Increments the Reference Count
//
//  Returns:    ULONG
//
//-------------------------------------------------------------------------


ULONG __stdcall
TridentOSP::AddRef()
{
    return ++_refCount;
}


//+------------------------------------------------------------------------
//  Member:     TridentOSP::AddRef()
//
//  Synopsis:   Decrements the Reference Count and if reference count becomes
//              0, deletes the instance
//
//  Returns:    ULONG
//
//-------------------------------------------------------------------------

ULONG __stdcall
TridentOSP::Release()
{
    ULONG ulRefs = --_refCount;
    if (ulRefs == 0)
        delete this;

    return ulRefs;
}


//+------------------------------------------------------------------------
//  Member:     TridentOSP::QueryInterface(const IID& iid, void **ppv)
//
//  Synopsis:   QueryInterface implementation for TridentOSP class
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT __stdcall
TridentOSP::QueryInterface(const IID& iid, void **ppv)
{
    if (iid == IID_IUnknown || iid == IID_OLEDBSimpleProvider)
    {
        _refCount++;
        *ppv = this;
        return S_OK;
    }
    
    return E_FAIL;
}


//+------------------------------------------------------------------------
//  Member:     CDoc::getDataMember(DataMember bstrDM,REFIID riid, IUnknown **ppunk)
//
//  Synopsis:   sets ppunk to point to a new TridentOSP class.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------



HRESULT STDMETHODCALLTYPE
CDoc::getDataMember(DataMember bstrDM,REFIID riid, IUnknown **ppunk)
{
    HRESULT hr = S_OK;
    *ppunk = NULL;

    if (_readyState == READYSTATE_COMPLETE)
    {
        TridentOSP *pNew = new TridentOSP(this);
        hr = pNew->Init();
        if ( !hr && pNew )
        {
            *ppunk = (IUnknown *) (void *) pNew;
        }
    }
    return hr;
}


//+------------------------------------------------------------------------
//  Member:     CDoc::getDataMemberName(long lIndex, DataMember *pbstrDM)
//
//  Synopsis:   Unimplemented.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


HRESULT STDMETHODCALLTYPE 
CDoc::getDataMemberName(long lIndex, DataMember *pbstrDM)
{
    *pbstrDM = NULL;
    return S_OK;
}


//+------------------------------------------------------------------------
//  Member:     CDoc::getDataMemberCount(long *plCount)
//
//  Synopsis:   Unimplemented.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::getDataMemberCount(long *plCount)
{
    *plCount = NULL;
    return S_OK;
}

//+------------------------------------------------------------------------
//  Member:     CDoc::addDataSourceListener(DataSourceListener *pDSL)
//
//  Synopsis:   Unimplemented.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::addDataSourceListener(DataSourceListener *pDSL)
{
    _pDSL = pDSL;
    pDSL->AddRef();
    return S_OK;
}


//+------------------------------------------------------------------------
//  Member:     CDoc::removeDataSourceListener(DataSourceListener *pDSL)
//  Synopsis:   Unimplemented.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CDoc::removeDataSourceListener(DataSourceListener *pDSL)
{
    Assert(_pDSL == pDSL);
    _pDSL = NULL;
    ReleaseInterface(pDSL);
    return S_OK;
}


