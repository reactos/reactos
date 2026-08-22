// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Software clipper implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


MtExtern(CRectClipper);

class CRectClipper : public CSpanClipper
{
private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CRectClipper));

public:

    void SetClip(__in_ecount(1) CMILSurfaceRect const &rc);

    // COutputSpan

    virtual void OutputSpan(INT y, INT xMin, INT xMax);

    // CSpanClipper

    virtual void GetClipBounds(__in_ecount(1) CMILSurfaceRect *prc) const;
    virtual void SetOutputSpan(__in_ecount(1) COutputSpan *pOutputSpan);

protected:
    CMILSurfaceRect m_rcClip;
    COutputSpan *m_pOutputSpan;
};




