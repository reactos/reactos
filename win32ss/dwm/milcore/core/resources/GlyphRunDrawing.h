// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      GlyphsDrawing Duce resource definition.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGlyphRunDrawingDuce);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CMilGlyphRunDrawingDuce
//
//  Synopsis:
//      CMilDrawingDuce implementation that draws a glyphrun.  This class is the
//      resource that backs the managed GlyphRunDrawing class.
//
//------------------------------------------------------------------------------
class CMilGlyphRunDrawingDuce : public CMilDrawingDuce
{
    friend class CResourceFactory;

protected:
    
    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGlyphRunDrawingDuce));

    CMilGlyphRunDrawingDuce(__in_ecount(1) CComposition* pComposition)
        : CMilDrawingDuce(pComposition)
    {
    }

    virtual ~CMilGlyphRunDrawingDuce() { UnRegisterNotifiers(); }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GLYPHRUNDRAWING || CMilDrawingDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_GLYPHRUNDRAWING* pCmd
        );

    HRESULT RegisterNotifiers(CMilSlaveHandleTable *pHandleTable);
    
    override void UnRegisterNotifiers();

    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDrawingContext
        );    

private:
    
    CMilGlyphRunDrawingDuce_Data m_data;    
};



