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

MtExtern(CEffectList);

class CEffectList :
    public IMILEffectList,
    public CMILCOMBase
{

protected:

    struct ParamBlock
    {
        CLSID clsid;
        UINT cbParamSize;
        UINT cbParamOffset;
        UINT cResources;
        UINT uResourceOffset;
    };
    
    DynArray<ParamBlock> m_rgParamBlock;
    DynArray<BYTE> m_rgDataBlock;
    DynArray<IUnknown*> m_rgResourceBlock;

    // QI Support method
    
    STDMETHODIMP HrFindInterface(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppvObject
        ) override;

public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CEffectList));
    
    CEffectList();
    virtual ~CEffectList();
    
    // IUnknown methods.
    
    DECLARE_COM_BASE;

    STDMETHODIMP Add(
        __in_ecount(1) REFCLSID clsid,
        UINT size,
        __in_bcount_opt(size) const EffectParams *pData
        ) override;
    
    STDMETHODIMP AddWithResources(
        __in_ecount(1) REFCLSID clsid,
        UINT size,
        __in_bcount_opt(size) const EffectParams *pData,
        UINT cResources,
        __in_pcount_opt_inout(cResources) IUnknown * const *rgpIUnknown
        ) override;

    STDMETHODIMP_(void) Clear() override;

    STDMETHODIMP GetCount(
        __out_ecount(1) UINT *pCount
        ) const override;

    STDMETHODIMP GetCLSID(
        UINT idxEffect,
        __out_ecount(1) CLSID *pClsid
        ) const override;

    STDMETHODIMP GetParameterSize(
        UINT idxEffect,
        __out_ecount(1) UINT *pSize
        ) const override;

    STDMETHODIMP GetParameters(
        UINT idxEffect,
        UINT size,
        __out_bcount_full(size) EffectParams *pData
        ) const override;


    STDMETHODIMP GetResourceCount(
        UINT idxEffect,
        __out_ecount(1) UINT *pcResources
        ) const override;

    STDMETHODIMP GetResources(
        UINT idxEffect,
        UINT cResources,
        __out_pcount_full_out(cResources) IUnknown **rgpResources
        ) const override;

    // Get a pointer directly to the stored datablock. This is used to avoid
    // allocations in our internal code when building the transform chain.
    
    STDMETHODIMP_(void) GetParamRef(
        UINT idxEffect,
        __deref_out_xcount_full(m_rgParamBlock[idxEffect].cbParamSize) const void **ppvData
        ) const override;

    // Get a copy of the resources array without calling AddRef on the resources
    STDMETHODIMP_(void) GetResourcesNoAddRef(
        UINT idxEffect,
        UINT cResources,
        __out_pcount_full_out(cResources) IUnknown **rgpResources
        ) const override;

    // Get total number of resources in the effect list
    STDMETHODIMP GetTotalResourceCount(
        __out_ecount(1) UINT *pcResources
        ) const override;

    // Get a specific resource from the effect list
    STDMETHODIMP GetResource(
        UINT idxResource,
        __deref_out_ecount(1) IUnknown **ppIUnknown
        ) const override;

    // Replace a specific resource in the effect list
    STDMETHODIMP ReplaceResource(
        UINT idxResource,
        __inout_ecount(1) IUnknown *pIUnknown
        ) override;
};




