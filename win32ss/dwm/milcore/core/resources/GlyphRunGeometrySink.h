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
//      Glyph run geometry creator.
//
//  $ENDTAG
//
//  Classes:
//      CGlyphRunGeometrySink
//
//------------------------------------------------------------------------------

MtExtern(CGlyphRunGeometrySink);

class CGlyphRunGeometrySink 
    : public CMILCOMBase
    , public IDWriteGeometrySink
{
   
public:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CGlyphRunGeometrySink));


    static HRESULT Create(
        __deref_out CGlyphRunGeometrySink **ppGeometrySink
        );
    
    HRESULT ProduceGeometry(
        __in MilPoint2F* pBaselineOrigin,
        __deref_out CMilGeometryDuce **ppGeometry
        );

    HRESULT ProduceGeometryData(
        __deref_out MilPathGeometry **ppGeometryData,
        OUT UINT *pSize,
        OUT MilFillMode::Enum *pFillRule
        );
    
//
// CMILCOMBase interface
//
    DECLARE_COM_BASE
    
    override STDMETHOD(HrFindInterface)(
        __in_ecount(1) REFIID riid, __deref_out void **ppvObject
        );
 
//
// ID2D1SimplifiedGeometrySink interface
//
    override STDMETHOD_(void, SetFillMode)(
        D2D1_FILL_MODE fillMode 
        );
     
    override STDMETHOD_(void, SetSegmentFlags)(
        D2D1_PATH_SEGMENT vertexFlags 
        );
    
    override STDMETHOD_(void, BeginFigure)(
        D2D1_POINT_2F startPoint,
        D2D1_FIGURE_BEGIN figureBegin 
        );
    
    override STDMETHOD_(void, AddLines)(
        __in_ecount(pointsCount) CONST D2D1_POINT_2F *points,
        UINT pointsCount 
        );
    
    override STDMETHOD_(void, AddBeziers)(
        __in_ecount(beziersCount) CONST D2D1_BEZIER_SEGMENT *beziers,
        UINT beziersCount 
        );
    
    override STDMETHOD_(void, EndFigure)(
        D2D1_FIGURE_END figureEnd 
        );
    
    override STDMETHOD(Close)(
        );

protected:
    CGlyphRunGeometrySink();

    HRESULT Initialize();

    virtual ~CGlyphRunGeometrySink();


private:
    STDMETHOD_(void, AddLine)(
        D2D1_POINT_2F point 
        );
    
    STDMETHOD_(void, AddBezier)(
        __in CONST D2D1_BEZIER_SEGMENT *bezier 
        );

    void AddGenericPoly(
        MilPoint2D *point1,
        MilPoint2D *point2,
        MilPoint2D *point3,
        bool hasCurves,
        MilSegmentType::Enum segmentType);

    void EndSegment();
    
    // Stores the geometry data we are creating.
    CMilPathGeometryDuce_Data m_pathGeometryData;

    int m_currentOffset;

    MilPathGeometry *m_pGeometry;
    int m_cFigures;

    MilPathFigure *m_pCurrentFigure;
    int m_currentFigureOffset;
    int m_offsetToLastSegment;
    int m_lastFigureSize;

    MilSegment *m_pCurrentSegment;
    int m_currentSegmentOffset;
    int m_lastSegmentSize;
    bool m_isSegSmoothJoin;
    bool m_isSegGap;

    DynArray<void*, TRUE> m_arrGeometryDataStructs;
    UINT m_arrGeometryDataStructsCount;

    HRESULT m_hr;
    bool m_isSinkClosed;
    bool m_hasProducedGeometry;

};

