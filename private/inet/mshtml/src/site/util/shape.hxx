//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       shape.hxx
//
//  Contents:   Generic class that can represent rectangle, circle, polygon,
//              etc. Currently used to represent the focus outline for various
//              HTML elements.
//
//----------------------------------------------------------------------------

#ifndef _SHAPE_HXX_
#define _SHAPE_HXX_ 1

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_CSIMUTIL_HXX_
#define X_CSIMUTIL_HXX_
#include "csimutil.hxx"
#endif

class CFormDrawInfo;
class CPointAry;

//+---------------------------------------------------------------------------
//
//  Class:      CShape
//
//  Synopsis:   Abstract shape class
//
//----------------------------------------------------------------------------

class CShape
{
public:
    CShape(): _cThick(0)
    {}

    virtual ~CShape() {}

    virtual void OffsetShape(const CSize & sizeOffset) = 0;
    virtual void GetBoundingRect(CRect * pRect) = 0;
            void DrawShape(CFormDrawInfo * pDI);

    virtual void Draw(HDC hDC, SIZECOORD  cThick) = 0;

    SIZECOORD  _cThick;
};


class CRectShape: public CShape
{
public:
    void OffsetShape(const CSize & sizeOffset);
    void GetBoundingRect(CRect * pRect);
    void Draw(HDC hDC, SIZECOORD  cThick);

    CRect   _rect;
};

class CCircleShape: public CRectShape
{
public:
    void Set(POINTCOORD xCenter, POINTCOORD yCenter, SIZECOORD radius);

    void Draw(HDC hDC, SIZECOORD  cThick);
};

class CPolyShape: public CShape
{
public:
    void OffsetShape(const CSize & sizeOffset);
    void GetBoundingRect(CRect * pRect);
    void Draw(HDC hDC, SIZECOORD  cThick);

    CPointAry   _aryPoint;

};

//+---------------------------------------------------------------------------
//
//  Class:      CWigglyShape
//
//  Synopsis:   Represents focus outline for text range elements like anchors
//              where each character on a line could potentially have its own
//              rectangle. RegionFromElement does the work to identify what
//              rectangles are required for the characters being evaluated.
//              Finally, this class really stores an array of such shapes, 
//              because text range can span multiple lines and each lines has 
//              its own outline that tightly surrounds it.
//
//----------------------------------------------------------------------------
// 

MtExtern(CWigglyAry)
MtExtern(CWigglyAry_pv)
DECLARE_CPtrAry(CWigglyAry, CRectShape *, Mt(Mem), Mt(CWigglyAry_pv))

class CWigglyShape : public CShape
{
public:
    ~CWigglyShape();

    void OffsetShape(const CSize & sizeOffset);
    void GetBoundingRect(CRect * pRect);
    void Draw(HDC hDC, SIZECOORD  cThick);

    CWigglyAry  _aryWiggly;
};


#endif _SHAPE_HXX_
