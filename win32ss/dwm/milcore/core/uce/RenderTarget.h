// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------
//

//
//  Abstract:
//
//     The classes is used as the base class for other render targets like 
//     HwndTarget, SurfTarget, PrintTarget
//---------------------------------------------------------------------------------

MtExtern(CRenderTarget);

//------------------------------------------------------------------
// CRenderTarget
//------------------------------------------------------------------

class CRenderTarget : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CRenderTarget));

    CRenderTarget(__in_ecount(1) CComposition *pComposition);
    virtual ~CRenderTarget();

    HRESULT Initialize(__in_ecount(1) CComposition *pDevice);

    void ReleaseDrawingContext();

    HRESULT GetDrawingContext(
        __deref_out_ecount(1) CDrawingContext **pDrawingContext,
        bool allowCreation = true
        );

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_RENDERTARGET;
    }

    virtual HRESULT Render(
        __out_ecount(1) bool *pfNeedsPresent
        ) 
    {
        RIP("Unexpected call to CRenderTarget::Render");
        RRETURN(E_NOTIMPL);
    }

    virtual HRESULT Present()
    {
        RIP("Unexpected call to CRenderTarget::Present");
        RRETURN(E_NOTIMPL);
    }

    inline virtual HRESULT NotifyDisplaySetChange(bool, int, int)
    {
        return S_OK;
    }

    inline virtual BOOL PostDisplayAvailabilityMessage(int)
    {
        return TRUE;
    }

    virtual HRESULT UpdateRenderTargetFlags()
    {
        return S_OK;
    }

    virtual HRESULT GetBaseRenderTargetInternal(
        __deref_out_opt IRenderTargetInternal **ppIRT
        ) = 0;
    
     // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    //   implementation of the following commands is shared by render
    //   targets deriving from this class.
    // 
    // ------------------------------------------------------------------------

    virtual HRESULT ProcessSetRoot(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_SETROOT *pCmd
        ); /* overriden in wintarget.h */

    virtual HRESULT ProcessSetClearColor(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_SETCLEARCOLOR *pCmd
        ) /* overriden in surftarget.h, hwndtarget.h, wintarget.h */
    {
        RRETURN(E_UNEXPECTED);
    } 

    virtual HRESULT ProcessInvalidate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_INVALIDATE *pCmd,
        __in_bcount_opt(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        ) /* overriden in surftarget.h, hwndtarget.h */
    {
        RRETURN(E_UNEXPECTED);
    }

    virtual HRESULT ProcessSetFlags(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_SETFLAGS *pCmd
        ) /* overriden in hwndtarget.h */
    {
        RRETURN(E_UNEXPECTED);
    }

    virtual HRESULT ProcessUpdateWindowSettings(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_TARGET_UPDATEWINDOWSETTINGS *pCmd
        )
    {
        RRETURN(E_UNEXPECTED);
    }

protected:

    //
    // This is the composition partition in which this CDrawingContext is used
    // It is used to create the CContentBounder and CPreComputeContext and
    // to get access to the schedule manager
    //

    CComposition *m_pComposition;

    //
    // Root node of the retained graphics tree.
    //

    CMilVisual *m_pRoot;

    // Root drawing context for this target.
    CDrawingContext *m_pDrawingContext;  
};



