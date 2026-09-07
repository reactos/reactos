// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//       DrawingGroup Duce resource implementation.
//

#include "precomp.hpp"

MtDefine(CMilDrawing, MILRender, "CMilDrawing");
MtDefine(CMilDrawingGroupDuce, CMilDrawing, "CMilDrawingGroupDuce");

//+------------------------------------------------------------------------
//
//  Member:    CMilDrawingGroupDuce::Draw
//
//  Synopsis:  Draw the content of this DrawingGroup to the drawing context
//
//-------------------------------------------------------------------------
HRESULT 
CMilDrawingGroupDuce::Draw(
    __in_ecount(1) CDrawingContext *pDrawingContext // Drawing context to draw to
    )
{
    HRESULT hr = S_OK;

    BOOL fPushedTransform = FALSE;
    BOOL fPushedEffects = FALSE;
    DOUBLE rOpacity;
    BOOL fPushedGuidelineCollection = FALSE;
    BOOL fPushedRenderOptions = FALSE;
    CRectF<CoordinateSpace::LocalRendering> bounds;
    CRectF<CoordinateSpace::LocalRendering> *pBounds = NULL;
    
    Assert(pDrawingContext);

    if (!(EnterResource()))
    {
        // In case of a loop
        goto Cleanup;
    }

    //
    // Push edgemode, transform, clip, opacity and guideline collection.
    //
    // Do this even when there are no children because content may be generated when 
    // effects are supported.  However, if it turns out that we choose not to support 
    // effects, this could be changed.
    //

    {
        MilRenderOptions renderOptions = { 0 };
        
        //
        // First, we handle the nodes EdgeMode, as this may affect the bounds
        //
        if ((m_data.m_EdgeMode == MilEdgeMode::Aliased))
        {
            renderOptions.Flags |= MilRenderOptionFlags::EdgeMode;
            renderOptions.EdgeMode = m_data.m_EdgeMode;        
        }

        //
        // Check to see if we have any bitmapScalingMode.
        //
        if (m_data.m_bitmapScalingMode != MilBitmapScalingMode::Unspecified)
        {
            renderOptions.Flags |=  MilRenderOptionFlags::BitmapScalingMode;
            renderOptions.BitmapScalingMode = m_data.m_bitmapScalingMode;        
        }


        //
        // Check to see if we have a ClearTypeHint.
        //
        if (m_data.m_ClearTypeHint != MilClearTypeHint::Auto)
        {
            renderOptions.Flags |=  MilRenderOptionFlags::ClearTypeHint;
            renderOptions.ClearTypeHint = m_data.m_ClearTypeHint;        
        }
        

        if (renderOptions.Flags != 0)
        {
            IFC(pDrawingContext->PushRenderOptions(&renderOptions));
            
            fPushedRenderOptions = TRUE;        
        }
    }

    // 
    // Push transform if one exists
    //    
    if (NULL != m_data.m_pTransform)
    {
        IFC(pDrawingContext->PushTransform(m_data.m_pTransform));
        fPushedTransform = TRUE;
    }    

    
    // 
    // Push guideline collection if one exists
    //    
    if (m_data.m_pGuidelineSet)
    {
        IFC(pDrawingContext->PushGuidelineCollection(
            m_data.m_pGuidelineSet
            ));
        
        fPushedGuidelineCollection = TRUE;
    }    
 
    // if have an effect (currently the content is used only by effects)
    if (m_pContent != NULL)
    {
        // Apply clip
        // Get current geometry resource
        CMilGeometryDuce *pGeometryMask = NULL;
        if (NULL != m_data.m_pClipGeometry)
        {
            IFC(GetTypeSpecificResource(
                m_data.m_pClipGeometry,
                TYPE_GEOMETRY,
                &pGeometryMask
                ));
        }
        // Future Consieration: handle bitmap effects for drawings
        IFC(pDrawingContext->PushEffects(1.0f, pGeometryMask, NULL, NULL, NULL));
        fPushedEffects = TRUE;

        IFC(m_pContent->Draw(pDrawingContext));
    }
    else // if we didn't find any drawings, then apply opacity and draw the children.
    {
        //
        // Push opacity & mask effects
        //

        // Get current opacity value
        IFC(GetDoubleCurrentValue(&m_data.m_Opacity, m_data.m_pOpacityAnimation, &rOpacity));

        // Get current geometry resource
        CMilGeometryDuce *pGeometryMask = NULL;
        if (NULL != m_data.m_pClipGeometry)
        {
            IFC(GetTypeSpecificResource(
                m_data.m_pClipGeometry,
                TYPE_GEOMETRY,
                &pGeometryMask
                ));
        }

        CMilBrushDuce *pBrushMask = NULL;
        if (NULL != m_data.m_pOpacityMask)
        {
            IFC(GetTypeSpecificResource(
                m_data.m_pOpacityMask,
                TYPE_BRUSH,
                &pBrushMask
                ));
        }

        // Compute our children's bounds
        if (pBrushMask && !m_fInBoundsCalculation)
        {
            pBounds = &bounds;
            IFC(GetChildrenBounds(
                pDrawingContext->GetContentBounder(),
                pBounds));
        } 
        // Future Consideration: handle bitmap effects for drawings
        IFC(pDrawingContext->PushEffects(rOpacity, pGeometryMask, pBrushMask, NULL, pBounds));
        fPushedEffects = TRUE;

        //
        // 
        // Draw the elements in the children collection
        //
        //
        
        if (m_data.m_cChildren > 0)
        {
            HRESULT hrDrawing = S_OK;
            
            //
            // Obtain children array & array count
            //
                
            // Calculate number of elements in the children array
            UINT uChildrenCount = m_data.m_cChildren;
            
            Assert(m_data.m_rgpChildren);  // Must be non-NULL if m_cChildren > 0
           
            //
            // Iterate over the children collection, calling Draw
            //
            for(UINT i = 0; i < uChildrenCount; i++)
            {
                // Ignore failures in the rendering layer to remain consistent with the RenderData
                // implementation, RenderBuffer.  
                //
                // If we returned failures that occured during rendering, the entire scene would be 
                // aborted, instead of the single Drawing primitive that caused the error.  This is 
                // especially dire because user-specified non-invertible matrices will cause a 
                // FAILED HRESULT in hardware.
                MIL_THRX(hrDrawing, m_data.m_rgpChildren[i]->Draw(pDrawingContext));
            }
        }
    }
Cleanup:

    //
    // Pop properties in the reverse order that they were pushed in
    //

    LeaveResource();

    // Pop effects
    if (fPushedEffects)
    {
        MIL_THR_SECONDARY(pDrawingContext->PopEffects());
    }        

    // Pop guideline collection
    if (fPushedGuidelineCollection)
    {
        pDrawingContext->PopGuidelineCollection();
    }    
    
    // Pop transform
    if (fPushedTransform)
    {
        pDrawingContext->PopTransform();
    }

    if (fPushedRenderOptions)
    {
        pDrawingContext->PopRenderOptions();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CMilDrawingGroupDuce::GetChildrenBounds
//
//  Synopsis:  Get the bounds of the children of this drawing group.
//
//-------------------------------------------------------------------------
HRESULT  
CMilDrawingGroupDuce::GetChildrenBounds(
    __in_ecount(1) CContentBounder *pContentBounder,
    __out_ecount(1) CRectF<CoordinateSpace::LocalRendering> *pBounds
    )
{
    HRESULT hr = S_OK;

    bool fInBoundsCalculation = m_fInBoundsCalculation;
    m_fInBoundsCalculation = true;

    LeaveResource();

    CMilTransformDuce *pTransform = m_data.m_pTransform;
    CMilGeometryDuce *pClipGeometry = m_data.m_pClipGeometry;
    
    m_data.m_pTransform = NULL;
    m_data.m_pClipGeometry = NULL;
            
    IFC(pContentBounder->GetContentBounds(this, pBounds));

    Verify(EnterResource());

    m_fInBoundsCalculation = fInBoundsCalculation;
    
Cleanup:
    m_data.m_pTransform = pTransform;
    m_data.m_pClipGeometry = pClipGeometry;

    RRETURN(hr);
}

            


