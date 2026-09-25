// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/**************************************************************************
*

*
* Module Name:
*
*   CEffectList object.
*
* Abstract:
*
*   This class holds an array of parameter blocks. Each parameter block has a
*   CLSID specifying the transform and the size and pointer to the required 
*   initialization parameters needed to create the transform.
*
*
**************************************************************************/

#include "precomp.hpp"

MtDefine(CEffectList, MILRender, "CEffectList");

/**************************************************************************
*
* Function Description:
*
*   QueryInterface support routines.
*
*
**************************************************************************/

STDMETHODIMP
CEffectList::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    ) override
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILEffectList)
        {
            *ppvObject = static_cast<IMILEffectList*>(this);
            
            hr = S_OK;
        }
        else
        if (riid == IID_IEffectInternal)
        {
            *ppvObject = static_cast<CEffectList*>(this);
            
            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    RRETURN(hr);
}

/**************************************************************************
*
* Function Description:
*
*   Constructor.
*
*
**************************************************************************/

CEffectList::CEffectList()
{
}

/**************************************************************************
*
* Function Description:
*
*   Destructor. Iterate the arrays and free any allocations. Assumes there are no NULL
*   resources because AddWithResources prevents this case from happening.
*
*
**************************************************************************/

CEffectList::~CEffectList()
{
    for (UINT i=0; i<m_rgResourceBlock.GetCount(); i++)
    {
        Assert(m_rgResourceBlock[i]);
        m_rgResourceBlock[i]->Release();
    }
}

/**************************************************************************
*
* Function Description:
*
*   GetCount. 
*
*   Returns the number of parameter blocks stored in the list.
*
*
**************************************************************************/

STDMETHODIMP 
CEffectList::GetCount(
    __out_ecount(1) UINT *pCount
    )  const override
{
    API_ENTRY_NOFPU(IMILEffectList::Add);
    HRESULT hr = S_OK;
    
    if (pCount)
    {
        *pCount = m_rgParamBlock.GetCount();
    }
    else
    {
        hr = THR(E_INVALIDARG);
    }
    
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   Add. 
*
*   Adds a parameter block to the list.
*
*
**************************************************************************/

STDMETHODIMP 
CEffectList::Add(
    __in_ecount(1) REFCLSID clsid,
    UINT size,
    __in_bcount_opt(size) const EffectParams *pData
    ) override
{
    return AddWithResources(clsid, size, pData, 0, NULL);
}

    
/**************************************************************************
*
* Function Description:
*
*   AddWithResources. 
*
*   Adds a parameter block to the list, along with an array of IUnknowns.
*
*
**************************************************************************/

STDMETHODIMP
CEffectList::AddWithResources(
    __in_ecount(1) REFCLSID clsid,
    UINT size,
    __in_bcount_opt(size) const EffectParams *pData,
    UINT cResources,
    __in_pcount_opt_inout(cResources) IUnknown * const *rgpIUnknown
    ) override
{
    API_ENTRY_NOFPU(IMILEffectList::AddWithResources);
    HRESULT hr = S_OK;
    
    if ((size > 0 && pData == NULL) ||
        (cResources > 0 && rgpIUnknown == NULL))
    {
        IFC(E_INVALIDARG);
    }
    
    for (UINT i = 0; i < cResources; i++)
    {
        if (rgpIUnknown[i] == NULL)
        {
            IFC(E_POINTER);
        }
    }
    
    ParamBlock p;
    p.clsid = clsid;
    p.cbParamOffset = m_rgDataBlock.GetCount();
    p.cbParamSize = size;
    p.cResources = cResources;
    p.uResourceOffset = m_rgResourceBlock.GetCount();
    
    IFC(m_rgParamBlock.Add(p));
    
    if (size > 0)
    {
        hr = THR(m_rgDataBlock.AddMultipleAndSet(reinterpret_cast<const BYTE*>(pData), size));
        
        if (FAILED(hr))
        {
            m_rgParamBlock.SetCount(m_rgParamBlock.GetCount()-1);
            goto Cleanup;
        }
    }
    
    if (cResources > 0)
    {
        hr = THR(m_rgResourceBlock.AddMultipleAndSet(rgpIUnknown, cResources));
 
        if (FAILED(hr))
        {
            m_rgDataBlock.SetCount(p.cbParamOffset);
            m_rgParamBlock.SetCount(m_rgParamBlock.GetCount()-1);
            goto Cleanup;
        }
        
        for (UINT i = 0; i < cResources; i++)
        {
            rgpIUnknown[i]->AddRef();
        }
    }
    
Cleanup:
    API_CHECK(hr);
    return hr;
}

    
/**************************************************************************
*
* Function Description:
*
*   GetCLSID. 
*
*   Get the CLSID associated with a given array index
*
*
**************************************************************************/

STDMETHODIMP 
CEffectList::GetCLSID(
    UINT idxEffect,
    __out_ecount(1) CLSID *pClsid
    ) const override
{
    API_ENTRY_NOFPU(IMILEffectList::GetCLSID);
    
    HRESULT hr = S_OK;
    
    if (pClsid && idxEffect < m_rgParamBlock.GetCount())
    {
        GpMemcpy(pClsid, &m_rgParamBlock[idxEffect].clsid, sizeof(CLSID));
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    API_CHECK(hr);
    return hr;
}
    
/**************************************************************************
*
* Function Description:
*
*   GetSize. 
*
*   Get the size of the parameter block associated with a given array index
*
*
**************************************************************************/

STDMETHODIMP
CEffectList::GetParameterSize(
    UINT idxEffect,
    __out_ecount(1) UINT *pSize
    ) const override
{
    API_ENTRY_NOFPU(IMILEffectList::GetParameterSize);

    HRESULT hr = S_OK;
    
    if (pSize && idxEffect < m_rgParamBlock.GetCount())
    {
        *pSize = m_rgParamBlock[idxEffect].cbParamSize;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   GetParameters. 
*
*   Get the parameter block associated with the given index. 
*
*
**************************************************************************/

STDMETHODIMP 
CEffectList::GetParameters(
    UINT idxEffect,
    UINT size,
    __out_bcount_full(size) EffectParams *pData
    ) const override
{
    API_ENTRY_NOFPU(IMILEffectList::GetParameters);
    HRESULT hr = S_OK;
    
    UINT uiSize = m_rgParamBlock[idxEffect].cbParamSize;
    if (idxEffect >= (UINT)m_rgParamBlock.GetCount() ||
        size < uiSize ||
        pData == NULL)
    {
        IFC(E_INVALIDARG);
    }

    void* pParamData = m_rgDataBlock.GetDataBuffer() + m_rgParamBlock[idxEffect].cbParamOffset;
    GpMemcpy(pData, pParamData, uiSize);

Cleanup:
    API_CHECK(hr);
    return hr;
}


/**************************************************************************
*
* Function Description:
*
*   GetResourceCount. 
*
*   Returns the number of resources associated with an effect
*
*
**************************************************************************/

STDMETHODIMP 
CEffectList::GetResourceCount(
    UINT idxEffect,
    __out_ecount(1) UINT *pcResources
    ) const override
{
    API_ENTRY_NOFPU(IMILEffectList::GetResourceCount);
    HRESULT hr = S_OK;

    if (idxEffect >= (UINT)m_rgParamBlock.GetCount() ||
        pcResources == NULL)
    {
        IFC(E_INVALIDARG);
    }
        
    *pcResources = m_rgParamBlock[idxEffect].cResources;

Cleanup:
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   GetResource. 
*
*   Gets resource from the effect list
*
*
**************************************************************************/
STDMETHODIMP
CEffectList::GetResources(
    UINT idxEffect,
    UINT cResources,
    __out_pcount_full_out(cResources) IUnknown **rgpResources
    ) const override
{
    API_ENTRY_NOFPU(IMILEffectList::GetResources);
    HRESULT hr = S_OK;

    if (idxEffect >= m_rgParamBlock.GetCount() ||
        cResources != m_rgParamBlock[idxEffect].cResources ||
        rgpResources == NULL)
    {
        IFC(E_INVALIDARG);
    }

    void* ppResourceData = m_rgResourceBlock.GetDataBuffer() + m_rgParamBlock[idxEffect].uResourceOffset;
    GpMemcpy(rgpResources, ppResourceData, cResources * sizeof(*rgpResources));

    for (UINT i = 0; i < cResources; i++)
    {
        rgpResources[i]->AddRef();
    }

Cleanup:    
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   Clear. 
*
*   Remove all the effect descriptions from the effect list. 
*
*
**************************************************************************/

STDMETHODIMP_(void)
CEffectList::Clear(void) override
{
    m_rgParamBlock.SetCount(0);
    m_rgDataBlock.SetCount(0);

    for (UINT i = 0; i < m_rgResourceBlock.GetCount(); i++)
    {
        m_rgResourceBlock[i]->Release();
    }
    
    m_rgResourceBlock.SetCount(0);
}

/**************************************************************************
*
* Function Description:
*
*   GetParamRef. 
*
*   Get a reference to the parameter block associated with the given index. 
*
*
**************************************************************************/

STDMETHODIMP_(void)
CEffectList::GetParamRef(
    UINT idxEffect,
    __deref_out_xcount_full(m_rgParamBlock[idxEffect].cbParamSize) const void **ppvData
    ) const override
{
    Assert(idxEffect < m_rgParamBlock.GetCount());
    Assert(*ppvData == NULL);
    
    *ppvData = m_rgDataBlock.GetDataBuffer() + m_rgParamBlock[idxEffect].cbParamOffset;
}

/**************************************************************************
*
* Function Description:
*
*   GetResourcesNoAddRef. 
*
*   Gets a copy of the array without calling AddRef
*
*
**************************************************************************/

STDMETHODIMP_(void)
CEffectList::GetResourcesNoAddRef(
    UINT idxEffect,
    UINT cResources,
    __out_pcount_full_out(cResources) IUnknown **rgpResources
    ) const override
{
    Assert(idxEffect < m_rgParamBlock.GetCount());
    Assert(cResources == m_rgParamBlock[idxEffect].cResources);

    const void* ppResourceData = m_rgResourceBlock.GetDataBuffer() + m_rgParamBlock[idxEffect].uResourceOffset;
    GpMemcpy(rgpResources, ppResourceData, cResources * sizeof(*rgpResources));
}


/**************************************************************************
*
* Function Description:
*
*   CEffectList::GetTotalResourceCount. 
*
*   Get total number of resources in the effect list
*
*
**************************************************************************/
STDMETHODIMP
CEffectList::GetTotalResourceCount(
    __out_ecount(1) UINT *pcResources
    ) const override
{
    HRESULT hr = S_OK;

    if (pcResources == NULL)
    {
        IFC(E_INVALIDARG);
    }

    *pcResources = m_rgResourceBlock.GetCount();

Cleanup:
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   CEffectList::GetResource. 
*
*   Get a specific resource from the effect list
*
*
**************************************************************************/

STDMETHODIMP
CEffectList::GetResource(
    UINT idxResource,
    __deref_out_ecount(1) IUnknown **ppIUnknown
    ) const override
{
    HRESULT hr = S_OK;

    if (ppIUnknown == NULL ||
        idxResource >= m_rgResourceBlock.GetCount()
        )
    {
        IFC(E_INVALIDARG);
    }

    *ppIUnknown = m_rgResourceBlock[idxResource];
    (*ppIUnknown)->AddRef();

Cleanup:
    API_CHECK(hr);
    return hr;
}

/**************************************************************************
*
* Function Description:
*
*   CEffectList::ReplaceResource. 
*
*   Replace a specific resource in the effect list
*
*
**************************************************************************/
STDMETHODIMP
CEffectList::ReplaceResource(
    UINT idxResource,
    __inout_ecount(1) IUnknown *pIUnknown
    ) override
{
    HRESULT hr = S_OK;

    if (idxResource >= m_rgResourceBlock.GetCount())
    {
        IFC(E_INVALIDARG);
    }

    IUnknown * &pICurrent = m_rgResourceBlock[idxResource];
    ReplaceInterface(pICurrent, pIUnknown);

Cleanup:
    API_CHECK(hr);
    return hr;
}

//------------------------------------------------------------------------------
//  MILCreateEffectList (static method)
//------------------------------------------------------------------------------

HRESULT WINAPI MILCreateEffectList(
    __deref_out IMILEffectList **ppiEffectList
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppiEffectList)
    {
        CEffectList *pObj = new CEffectList;

        hr = E_OUTOFMEMORY;

        if (pObj)
        {
            *ppiEffectList = static_cast<IMILEffectList *>(pObj);
            pObj->AddRef();
            hr = S_OK;
        }
    }

    RRETURN(hr);
}



