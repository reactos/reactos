// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Module Name:
//
//    printtarget.h
//
//     The classes used to support printing are called generic for historical reasons.
//---------------------------------------------------------------------------------

MtExtern(CSlaveGenericRenderTarget);

//------------------------------------------------------------------
// CSlaveGenericRenderTarget
//------------------------------------------------------------------

class CSlaveGenericRenderTarget : public CRenderTarget
{
    friend class CResourceFactory;

private:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSlaveGenericRenderTarget));

    CSlaveGenericRenderTarget(CComposition *pComposition);
    virtual ~CSlaveGenericRenderTarget();
    
public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GENERICRENDERTARGET || CRenderTarget::IsOfType(type);
    }

    override HRESULT GetBaseRenderTargetInternal(
        __deref_out_opt IRenderTargetInternal **ppIRT
        );
    
    virtual HRESULT Render(
        __out_ecount(1) bool *pfPresentNeeded
        );

    virtual HRESULT Present();
    
    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessCreate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GENERICTARGET_CREATE* pCmd
        );

private:
    IMILRenderTarget *m_pRenderTarget;
    UINT m_uiWidth;
    UINT m_uiHeight;
};



