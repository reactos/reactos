//------------------------------------------------------------------------
//
//  IShellFolder Data Provider Data Object
//  Copyright (C) Microsoft Corporation, 1996, 1997
//
//  File:       DataPrv.h
//
//  Contents:   Declaration of the CShellFolderData COM object.
//
//------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop

#include "dataprv.h"
#include "commctrl.h"

// TODO: use IShellDetails instead
const LPCWSTR c_awszColumns[] = {
        L"Title",
        L"URL",
    };


HRESULT CShellFolderData::SetListener(OLEDBSimpleProviderListener **ppListener)
{
    _ppListener = ppListener;

    return S_OK;
}

CShellFolderData::~CShellFolderData() 
{
    ATOMICRELEASE(_psf);

    if (_hdpa)
        DPA_Destroy(_hdpa);
}

STDMETHODIMP CShellFolderData::getRowCount(DBROWCOUNT *pcRows)
{
    HRESULT hr = _DoEnum();

    *pcRows = 0;
    if (SUCCEEDED(hr)) {
        *pcRows = DPA_GetPtrCount(_hdpa);
    }

    return S_OK;
}

STDMETHODIMP CShellFolderData::getColumnCount(DB_LORDINAL *pcColumns)
{
    *pcColumns = ARRAYSIZE(c_awszColumns);
    return S_OK;
}

STDMETHODIMP CShellFolderData::getRWStatus(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPRW *prwStatus)
{
    *prwStatus = OSPRW_READONLY; 
    return S_OK;
}

STDMETHODIMP CShellFolderData::getVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT *pVar)
{
    VariantInit(pVar);
    HRESULT hr = _DoEnum();
    if (FAILED(hr)) 
        return hr;

    // TODO: optimize for speed
    hr = E_FAIL;
    if (iColumn > 0 && iColumn <= ARRAYSIZE(c_awszColumns)) 
    {
        if (iRow == 0) 
        {
            pVar->bstrVal = SysAllocString(c_awszColumns[iColumn-1]);
            pVar->vt = VT_BSTR;
            hr = S_OK;        
        } 
        else if (iRow > 0) 
        {
            if (_psf && _hdpa && ((iRow-1) < DPA_GetPtrCount(_hdpa)))
            {
                LPCITEMIDLIST pidl = (LPCITEMIDLIST)DPA_GetPtr(_hdpa, iRow-1);
                STRRET strret;

                switch(iColumn) {
                case 1:
                    hr = _psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret);
                    break;

                case 2:
                    hr = _psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &strret);
                    break;
                }

                if (SUCCEEDED(hr))
                {
                    WCHAR szValue[MAX_PATH];
                    hr = StrRetToBufW(&strret, pidl, szValue, ARRAYSIZE(szValue));
                    if (SUCCEEDED(hr))
                    {
                        pVar->vt = VT_BSTR;
                        pVar->bstrVal = SysAllocString(szValue);
                    }
                }
            }
        }    
    } 

    return hr;
}

STDMETHODIMP CShellFolderData::setVariant(DBROWCOUNT iRow, DB_LORDINAL iColumn, OSPFORMAT format, VARIANT Var)
{
    return E_NOTIMPL; 
}

STDMETHODIMP CShellFolderData::getLocale(BSTR *pbstrLocale)
{
    return E_NOTIMPL;
    
}

STDMETHODIMP CShellFolderData::deleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsDeleted)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderData::insertRows(DBROWCOUNT iRow, DBROWCOUNT cRows, DBROWCOUNT *pcRowsInserted)
{
    return E_NOTIMPL;
}


STDMETHODIMP CShellFolderData::find(DBROWCOUNT iRowStart, DB_LORDINAL iColumn, VARIANT val,
        OSPFIND findFlags, OSPCOMP compType, DBROWCOUNT *piRowFound)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderData::addOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    IUnknown_Set((IUnknown **)_ppListener, pospIListener);
    return S_OK;    
}

STDMETHODIMP CShellFolderData::removeOLEDBSimpleProviderListener(OLEDBSimpleProviderListener *pospIListener)
{
    IUnknown_Set((IUnknown **)_ppListener, NULL);
    return S_OK;
}

STDMETHODIMP CShellFolderData::getEstimatedRows(DBROWCOUNT *pcRows)
{
    *pcRows = -1;
    return S_OK;
}

STDMETHODIMP CShellFolderData::isAsync(BOOL *pbAsync)
{
    *pbAsync = TRUE;
    return S_OK;
}

STDMETHODIMP CShellFolderData::stopTransfer()
{
    return E_NOTIMPL;    
}

static int FreePidlCallBack(void *pvPidl, void *)
{
    LPITEMIDLIST pidl = (LPITEMIDLIST)pvPidl, 
    ILFree(pidl);
    return 1;
}

HRESULT CShellFolderData::_DoEnum(void)
{
    HRESULT hr = S_OK;

    // TODO: optimize this

    if (_hdpa) {
        DPA_DestroyCallback(_hdpa, FreePidlCallBack, 0);
        _hdpa = NULL;
    }

    if (_psf) {
        _hdpa = DPA_Create(4);
        if (_hdpa) {
            IEnumIDList* penum;
            hr = _psf->EnumObjects(NULL, SHCONTF_NONFOLDERS|SHCONTF_FOLDERS,
                                     &penum);
            if (SUCCEEDED(hr)) {
                ULONG celt;
                LPITEMIDLIST pidl;
                while (penum->Next(1, &pidl, &celt)==S_OK && celt==1) {
                    DPA_AppendPtr(_hdpa, pidl);
                }
                penum->Release();
            }
        } else {
            hr = E_OUTOFMEMORY;
        }
    } else {
        hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CShellFolderData::SetShellFolder(IShellFolder *psf)
{
    IUnknown_Set((IUnknown **)&_psf, psf);
    return S_OK;
}
