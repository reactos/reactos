//------------------------------------------------------------------------------
// undo.cpp
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
//
// Undo support routines for TriEdit
//
//------------------------------------------------------------------------------

#include "stdafx.h"

#include <ocidl.h>

#include "undo.h"
#include "triedit.h"
#include "document.h"

///////////////////////////////////////////////////////////////////////////////
//
// AddUndoUnit
//
// Add the given undo unit to the given Trident instance. Return S_OK
// or a Trident error code.
//

HRESULT AddUndoUnit(IUnknown* punkTrident, IOleUndoUnit* pioleUndoUnit)
{
    HRESULT hr = E_FAIL;
    IServiceProvider* piservProv;
    IOleUndoManager* pioleUndoManager;
    
    if (punkTrident && pioleUndoUnit)
    {
        hr = punkTrident->QueryInterface(IID_IServiceProvider, (LPVOID*)&piservProv);

        if (SUCCEEDED(hr))
        {
            _ASSERTE(piservProv);
            hr = piservProv->QueryService(IID_IOleUndoManager,
                    IID_IOleUndoManager, (LPVOID*)&pioleUndoManager);

            if (SUCCEEDED(hr))
            {
                _ASSERTE(pioleUndoManager);
                hr = pioleUndoManager->Add(pioleUndoUnit);
                _ASSERTE(SUCCEEDED(hr));
                pioleUndoManager->Release();
            }
            piservProv->Release();
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// EmptyUndoRedoStack
//
// If fUndo is TRUE, discard all undo items from the given undo manager.
// If fUndo is FALSE, discard all redo items from the given undo manager
// Return S_OK if all goes well, or a Trident error code otherwise.
//

HRESULT EmptyUndoRedoStack(BOOL fUndo, IOleUndoManager *pUndoManager)
{
	CComPtr<IEnumOleUndoUnits> srpEnum;
    CComPtr<IOleUndoUnit> srpcd;
    ULONG cFetched=0, cTotal=0;
	HRESULT hr = E_FAIL;
	
    if (fUndo)
    {
		if (FAILED(hr = pUndoManager->EnumUndoable(&srpEnum)))
			goto Fail;
	}
	else
	{
		if (FAILED(hr = pUndoManager->EnumRedoable(&srpEnum)))
			goto Fail;
	}
	

    while (SUCCEEDED(srpEnum->Next(1, &srpcd, &cFetched))) 
    {
        _ASSERTE(cFetched <=1);
        if (srpcd == NULL)
            break;
            
        cTotal++;
        srpcd.Release();
    }

	// get the one on top of the stack and discard from that
    if (cTotal > 0)
    {
    	if (FAILED(hr = srpEnum->Reset()))
        	goto Fail; 
    	if (FAILED(hr = srpEnum->Skip(cTotal-1)))
        	goto Fail;

    	srpcd.Release();
    	if (FAILED(hr = srpEnum->Next(1, &srpcd, &cFetched)))
        	goto Fail;

		_ASSERTE(cFetched ==1);
		
    	if (FAILED(hr = pUndoManager->DiscardFrom(srpcd)))
        	goto Fail;
	}

Fail:
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// GetUndoManager
//
// Obtain and return (under *ppOleUndoManager) the IOleUndoManager
// associated with the given Trident instance. Return S_OK if a 
// manager was returned; E_FAIL otherwise.
//

HRESULT GetUndoManager(IUnknown* punkTrident, IOleUndoManager **ppOleUndoManager)
{
   HRESULT hr = E_FAIL;
   CComPtr<IServiceProvider> srpiservProv;
   CComPtr<IOleUndoManager> srpioleUndoManager;

   _ASSERTE(ppOleUndoManager != NULL);
   _ASSERTE(punkTrident != NULL);
   if (punkTrident)
    {
        hr = punkTrident->QueryInterface(IID_IServiceProvider, (LPVOID*)&srpiservProv);

        if (SUCCEEDED(hr))
        {
            _ASSERTE(srpiservProv);
            if (SUCCEEDED(hr = srpiservProv->QueryService(IID_IOleUndoManager,
                    IID_IOleUndoManager, (LPVOID*)&srpioleUndoManager)))
            {
                *ppOleUndoManager = srpioleUndoManager;
                (*ppOleUndoManager)->AddRef();
            }
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndo::CUndo
// CUndo::~Undo
//
// Simple constructor and destructor for the CUndo class. 
//

CUndo::CUndo()
{
    m_cRef = 1;
    m_fUndo = TRUE;

}

CUndo::~CUndo()
{
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndo::QueryInterface (IUnknown method)
// CUndo::AddRef (IUnknown method)
// CUndo::Release (IUnknown method)
//
// Implementations of the three IUnknown methods.
//

STDMETHODIMP CUndo::QueryInterface(REFIID riid, LPVOID* ppvObject)
{
    if (!ppvObject) 
        return E_POINTER;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = (IUnknown*)this;
    else
    if (IsEqualGUID(riid, IID_IOleUndoUnit))
        *ppvObject = (IOleUndoUnit*)this;
    else
        return E_NOINTERFACE;
    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) CUndo::AddRef(void)
{
    return InterlockedIncrement((LONG*)&m_cRef);
}

STDMETHODIMP_(ULONG) CUndo::Release(void)
{
    ULONG cRef = InterlockedDecrement((LONG*)&m_cRef);
    if (!cRef)
        delete this;
    return cRef;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoDrag::CUndoDrag
// CUndoDrag::~CUndoDrag
//
// Constructor for an object which can undo the drag of an HTML element.
//

CUndoDrag::CUndoDrag(IHTMLStyle* pihtmlStyle, POINT ptOrig, POINT ptMove)
{
    m_pihtmlStyle = pihtmlStyle;
    if (m_pihtmlStyle)
        m_pihtmlStyle->AddRef();
    m_ptOrig = ptOrig;
    m_ptMove = ptMove;
}

CUndoDrag::~CUndoDrag()
{
    SAFERELEASE(m_pihtmlStyle);
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoDrag::Do (IOleUndoUnit method)
//
// Do or undo dragging of an HTML element from place to place. Set or
// restore the item's position. Return S_OK.
//

STDMETHODIMP CUndoDrag::Do(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK;
    if (pUndoManager)
    {
        hr = pUndoManager->Add(this);        
    }
    if (m_pihtmlStyle)
    {
        // We do a put_pixelLeft(-1) and put_pixelTop(-1) below in order
        // to work around a Trident problem.  Sometimes they don't think
        // that anything has changed - these calls below fool them into
        // thinking that the values have changed.
        if (m_fUndo)
        {
            m_pihtmlStyle->put_pixelLeft(-1);
            m_pihtmlStyle->put_pixelLeft(m_ptOrig.x);
            m_pihtmlStyle->put_pixelTop(-1);
            m_pihtmlStyle->put_pixelTop(m_ptOrig.y);
        }
        else
        {
            m_pihtmlStyle->put_pixelLeft(-1);
            m_pihtmlStyle->put_pixelLeft(m_ptMove.x);
            m_pihtmlStyle->put_pixelTop(-1);
            m_pihtmlStyle->put_pixelTop(m_ptMove.y);
        }
        m_fUndo = !m_fUndo;
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoDrag::GetDescription (IOleUndoUnit method)
//
// Return the description of the undo item. Note that this function
// returns an empty string since this is the only would-be localizable
// content in TriEdit.
//

STDMETHODIMP CUndoDrag::GetDescription(BSTR *pBstr)
{
    if (pBstr)
    {
        *pBstr = SysAllocString(_T(" "));
        return S_OK;
    }
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoDrag::GetUnitType (IOleUndoUnit method)
//
// Return the CLSID and an identifier for the undo item.
//

STDMETHODIMP CUndoDrag::GetUnitType(CLSID *pClsid, LONG *plID)
{
    if (pClsid)
        *pClsid = UID_TRIEDIT_UNDO;
    if (plID)
        *plID = TRIEDIT_UNDO_DRAG;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoDrag::OnNextAdd (IOleUndoUnit method)
//
// Do nothing, but do it extremely well.
//

STDMETHODIMP CUndoDrag::OnNextAdd(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackManager::~CUndoPackManager
//
// Destructor for a CUndoPackManager object. If currently packing undo
// items, end the packing before destroying the object. 
//

CUndoPackManager::~CUndoPackManager(void)
{
    if (m_fPacking)
        End();
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackManager::Start
// 
// Called to tell the pack manager to begin accumulating subsequent
// undo units into a unit that can be undone in one fell swoop. Turn
// on the packing flag. Return S_OK if all goes well or E_FAIL if
// something goes wrong.
//

HRESULT CUndoPackManager::Start(void)
{
    HRESULT hr = E_FAIL;
    CComPtr<IOleUndoManager> srpioleUndoManager;
    CComPtr<IEnumOleUndoUnits> srpEnum;
    CComPtr<IOleUndoUnit> srpcd;
    ULONG cFetched=0;

    _ASSERTE(m_indexStartPacking==0);
    
    if (FAILED(hr = GetUndoManager(m_srpUnkTrident, &srpioleUndoManager)))
        goto Fail;

    if (FAILED(hr = srpioleUndoManager->EnumUndoable(&srpEnum)))
        goto Fail;

    while(SUCCEEDED(srpEnum->Next(1, &srpcd, &cFetched))) 
    {
        _ASSERTE(cFetched <=1);
        if (srpcd == NULL)
            break;
            
        m_indexStartPacking++;
        srpcd.Release();
    }

    m_fPacking = TRUE;

 Fail:
    if (!m_fPacking)
        m_indexStartPacking=0;
        
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackManager::End
//
// Called to tell the pack manager to stop accumulating undo units. Pack
// the accumulated undo units in to the parent undo unit and turn off
// the packing flag. Return S_OK if all goes well or E_FAIL if something
// goes wrong.
//

HRESULT CUndoPackManager::End(void)
{
    HRESULT hr = E_FAIL;
    CUndoPackUnit *pUndoParentUnit;
    
    _ASSERTE(m_srpUnkTrident != NULL);
    pUndoParentUnit = new CUndoPackUnit();
    _ASSERTE(pUndoParentUnit  != NULL);
    
    if (FAILED(hr = pUndoParentUnit->PackUndo(m_indexStartPacking, m_srpUnkTrident)))
        goto Fail;

    m_fPacking = FALSE;
Fail:
    pUndoParentUnit->Release();
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackUnit::Do (IOleUndoUnit method)
//
// Invoke the Do method of each undo unit referenced by the object. Return
// S_OK.
//

STDMETHODIMP CUndoPackUnit::Do(IOleUndoManager *pUndoManager)
{
    HRESULT hr = S_OK; 

    for (INT i=sizeof(m_rgUndo)/sizeof(IOleUndoUnit*)-1; i >= 0; i--)
    {
        if (m_rgUndo[i] == NULL)
            continue;
        
        if (FAILED(hr = m_rgUndo[i]->Do(pUndoManager)))
            goto Fail;
    }

	::EmptyUndoRedoStack(FALSE, pUndoManager);
	
Fail:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackUnit::GetDescription (IOleUndoUnit method)
//
// Return the description of the undo item. Note that this function
// returns an empty string since this string would be one of only
// two localizable strings in TriEdit.
//

STDMETHODIMP CUndoPackUnit::GetDescription(BSTR *pBstr)
{
    if (pBstr)
    {
        // In order to save localization work for the two TriEdit strings, 
        // it was decided that we would return a blank string here
        *pBstr = SysAllocString(_T(" "));
        return S_OK;
    }
    return E_FAIL;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackUnit::GetUnitType (IOleUndoUnit method)
//
// Return the CLSID and an identifier for the undo item.
//

STDMETHODIMP CUndoPackUnit::GetUnitType(CLSID *pClsid, LONG *plID)
{
    if (pClsid)
        *pClsid = UID_TRIEDIT_UNDO;
    if (plID)
        *plID = TRIEDIT_UNDO_PACK;
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackUnit::OnNextAdd (IOleUndoUnit method)
//
// Do nothing, but do it extremely well.
//

STDMETHODIMP CUndoPackUnit::OnNextAdd(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// CUndoPackUnit::PackUndo
//
// Pack all of the undo units starting at the given index in to
// the parent undo manager. Return S_OK if all goes well, or
// E_FAIL if something goes wrong.
//

HRESULT CUndoPackUnit::PackUndo(ULONG indexStartPacking, IUnknown *pUnkTrident)
{
    HRESULT hr = E_FAIL;
    CComPtr<IOleUndoManager> srpioleUndoManager;
    CComPtr<IEnumOleUndoUnits> srpEnumUndo;
    CComPtr<IOleUndoUnit> rgUndo[cUndoPackMax]; // CONSIDER: allocate dynamically
    CComPtr<IOleUndoUnit> srpcd;
    ULONG cFetched=0, cUndo=0, i=0;
    
    if (FAILED(hr = GetUndoManager(pUnkTrident, &srpioleUndoManager)))
        goto Fail;

    if (FAILED(hr = srpioleUndoManager->EnumUndoable(&srpEnumUndo)))
        goto Fail;
        
    _ASSERTE(srpEnumUndo != NULL);
    while(SUCCEEDED(srpEnumUndo->Next(1, &srpcd, &cFetched))) 
    {
        _ASSERTE(cFetched <= 1);
        if (srpcd == NULL)
            break;
            
        cUndo++;
        srpcd.Release();
    }

    // if there's nothing to pack
    if ((cUndo-indexStartPacking) == 0)
        return S_OK;
        
    if ((cUndo-indexStartPacking) > cUndoPackMax)
        return E_OUTOFMEMORY;
        
    // get the undo units that we want to pack
    if (FAILED(hr = srpEnumUndo->Reset()))
        goto Fail; 
    if (FAILED(hr =srpEnumUndo->Skip(indexStartPacking)))
        goto Fail;
    if (FAILED(hr = srpEnumUndo->Next(cUndo-indexStartPacking, (IOleUndoUnit **) &m_rgUndo, &cFetched)))
        goto Fail;
    _ASSERTE(cFetched == (cUndo-indexStartPacking));
    
    // now clear the undo/redo stack and then adds back the undo unit except that one that we just packed
    if (FAILED(hr = srpEnumUndo->Reset()))
        goto Fail;

    if (FAILED(hr = srpEnumUndo->Next(cUndo, (IOleUndoUnit **) &rgUndo, &cFetched)))
        goto Fail;
        
    _ASSERTE(cFetched == cUndo);

    if (FAILED(hr = srpioleUndoManager->DiscardFrom(NULL)))
        goto Fail;

    for (i=0; i < indexStartPacking; i++)
    {
        if (FAILED(hr = srpioleUndoManager->Add(rgUndo[i])))
            goto Fail;
    }

    if (FAILED(hr = ::AddUndoUnit(pUnkTrident, this)))
        goto Fail;
Fail:   
    return hr;
}

