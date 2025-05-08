// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilSolidColorBrushDuce);

// Class: CMilSolidColorBrushDuce
class CMilSolidColorBrushDuce : public CMilBrushDuce
{
    friend class CResourceFactory;
    friend class CWindowRenderTarget;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilSolidColorBrushDuce));

    CMilSolidColorBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilBrushDuce(pComposition)
    {
        SetDirty(TRUE);
    }

    virtual ~CMilSolidColorBrushDuce();

public:

    __override
    bool IsConstantOpaque()
    {
        if ((m_data.m_pOpacityAnimation == NULL) &&
            (m_data.m_Opacity == 1.0) &&
            (m_data.m_Color.a == 1.0f) &&
            (m_data.m_pColorAnimation == NULL))
        {
            return true;
        }
        else
        {
            return false;
        }
    }

public:

    //
    // Returns a new CMilSolidColorBrushDuce set to the provided color, with 1 ref count.
    //
    static HRESULT CreateFromColor(
        __deref_out_ecount(1) CMilSolidColorBrushDuce **ppSolidColorBrush,
        __in_ecount(1) CComposition* pComposition,
        __in_ecount(1) const MilColorF &color)
    {
        HRESULT hr = S_OK;

        CMilSolidColorBrushDuce* pBrush = NULL;

        IFCOOM(pBrush = new CMilSolidColorBrushDuce(pComposition));

        pBrush->m_data.m_Color = color;
        pBrush->m_data.m_Opacity = 1.0;

        // As this is a Create method - not a constructor - we return 1 reference.
        pBrush->AddRef();

        *ppSolidColorBrush = pBrush;

    Cleanup:
        RRETURN(hr);
    }

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_SOLIDCOLORBRUSH || CMilBrushDuce::IsOfType(type);
    }

    override virtual bool NeedsBounds(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        return false;
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_SOLIDCOLORBRUSH* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    override HRESULT GetBrushRealizationInternal(
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
        );

    virtual BOOL HasRealizationContextChanged(
        IN const BrushContext *pBrushContext
        ) const
    {
        // The context is not used during solid color brush realization.
        return FALSE;
    }


    CMilSolidColorBrushDuce_Data m_data;

    LocalMILObject<CMILBrushSolid> m_solidBrushRealization;
};

