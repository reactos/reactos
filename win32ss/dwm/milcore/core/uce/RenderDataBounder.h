// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//     Definition of the content bounder.
//
//
//------------------------------------------------------------------------

MtExtern(CContentBounder);

// Forward declaration types used in GetContentBounds
class CMilSlaveRenderData;
class CMilDrawingDuce;
class CMilVisual;

//+----------------------------------------------------------------------------
//
//  Class:      CContentBounder
//
//  Synopsis:   This class abstracts the implementation needed to retrieve
//              the bounds of drawing content.
//
//              It is responsible for maintaining references to the objects
//              that do the actual bounding (the bounds render target
//              and render context), and for the interactions with those
//              objects that are required to retrieve bounds.
//
//
//-----------------------------------------------------------------------------
class CContentBounder
{
public:

     // Instantiation, construction, & destruction
    
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CContentBounder));
   
    ~CContentBounder();

    static HRESULT Create(
        __in_ecount(1) CComposition *pDevice, 
        __deref_out_ecount(1) CContentBounder **ppContentBounds
        );

    // Public methods

    HRESULT GetContentBounds(
        __in_ecount_opt(1) CMilSlaveResource *pContent, 
        __out_ecount(1) CMilRectF *prcBounds
        );

    HRESULT GetVisualInnerBounds(
        __in_ecount(1) CMilVisual* pNode,
        __out_ecount(1) CMilRectF *prcBounds
        );
    
private:

    // Ctor inaccessible to prevent inheritance.
    // Use the Create method to instantiate a CContentBounder.    
    CContentBounder(__in_ecount(1) CComposition *pComposition);

    HRESULT Initialize(__in_ecount(1) CComposition *pDevice);

private:

    CSwRenderTargetGetBounds    *m_pBoundsRenderTarget;
    CDrawingContext             *m_pDrawingContext;
    CComposition                *m_pComposition;    

// To properly handle recursion due to nested content, a seperate CContentBounder
// instance is needed for each level of nesting.  Otherwise, each level of nesting will
// clear the bounds held onto by the bounds render target as the stack unwinds.  This
// debug-only variable checks for these occurrences.
#ifdef DBG
    BOOL                        m_fInUse;
#endif
};


