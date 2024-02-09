/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of tool parameters, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

enum TOOLTYPE
{
    TOOL_FREESEL  =  1,
    TOOL_RECTSEL  =  2,
    TOOL_RUBBER   =  3,
    TOOL_FILL     =  4,
    TOOL_COLOR    =  5,
    TOOL_ZOOM     =  6,
    TOOL_PEN      =  7,
    TOOL_BRUSH    =  8,
    TOOL_AIRBRUSH =  9,
    TOOL_TEXT     = 10,
    TOOL_LINE     = 11,
    TOOL_BEZIER   = 12,
    TOOL_RECT     = 13,
    TOOL_SHAPE    = 14,
    TOOL_ELLIPSE  = 15,
    TOOL_RRECT    = 16,
    TOOL_MAX = TOOL_RRECT,
};

enum BrushStyle
{
    BrushStyleRound,
    BrushStyleSquare,
    BrushStyleForeSlash,
    BrushStyleBackSlash,
};

/* CLASSES **********************************************************/

struct ToolBase
{
    HDC m_hdc;
    COLORREF m_fg, m_bg;

    ToolBase() : m_hdc(NULL) { }
    virtual ~ToolBase() { }

    virtual void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick) { }
    virtual BOOL OnMouseMove(BOOL bLeftButton, LONG& x, LONG& y) { return TRUE; }
    virtual BOOL OnButtonUp(BOOL bLeftButton, LONG& x, LONG& y) { return TRUE; }

    virtual void OnDrawOverlayOnImage(HDC hdc) { }
    virtual void OnDrawOverlayOnCanvas(HDC hdc) { }

    virtual void OnSpecialTweak(BOOL bMinus) { }

    virtual void OnEndDraw(BOOL bCancel);

    void beginEvent();
    void endEvent();
    void reset();

    static ToolBase* createToolObject(TOOLTYPE type);
};

class ToolsModel
{
private:
    int m_lineWidth;
    INT m_penWidth;
    INT m_brushWidth;
    int m_shapeStyle;
    BrushStyle m_brushStyle;
    TOOLTYPE m_activeTool;
    TOOLTYPE m_oldActiveTool;
    INT m_airBrushRadius;
    int m_rubberRadius;
    BOOL m_transpBg;
    int m_zoom;
    ToolBase *m_pToolObject;

    ToolBase *GetOrCreateTool(TOOLTYPE nTool);

public:
    ToolsModel();
    ~ToolsModel();

    BOOL IsSelection() const;

    int GetLineWidth() const;
    void SetLineWidth(int nLineWidth);
    void MakeLineThickerOrThinner(BOOL bThinner);

    INT GetPenWidth() const;
    void SetPenWidth(INT nPenWidth);
    void MakePenThickerOrThinner(BOOL bThinner);

    int GetShapeStyle() const;
    void SetShapeStyle(int nShapeStyle);

    INT GetBrushWidth() const;
    void SetBrushWidth(INT nBrushWidth);
    void MakeBrushThickerOrThinner(BOOL bThinner);

    BrushStyle GetBrushStyle() const;
    void SetBrushStyle(BrushStyle nBrushStyle);

    TOOLTYPE GetActiveTool() const;
    TOOLTYPE GetOldActiveTool() const;
    void SetActiveTool(TOOLTYPE nActiveTool);

    INT GetAirBrushRadius() const;
    void SetAirBrushRadius(INT nAirBrushRadius);
    void MakeAirBrushThickerOrThinner(BOOL bThinner);

    int GetRubberRadius() const;
    void SetRubberRadius(int nRubberRadius);
    void MakeRubberThickerOrThinner(BOOL bThinner);

    SIZE GetToolSize() const;

    BOOL IsBackgroundTransparent() const;
    void SetBackgroundTransparent(BOOL bTransparent);

    int GetZoom() const;
    void SetZoom(int nZoom);

    void OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick);
    void OnMouseMove(BOOL bLeftButton, LONG x, LONG y);
    void OnButtonUp(BOOL bLeftButton, LONG x, LONG y);
    void OnEndDraw(BOOL bCancel);
    void OnDrawOverlayOnImage(HDC hdc);
    void OnDrawOverlayOnCanvas(HDC hdc);

    void resetTool();
    void selectAll();

    void NotifyToolChanged();
    void NotifyToolSettingsChanged();
    void NotifyZoomChanged();

    void SpecialTweak(BOOL bMinus);

    void DrawWithMouseTool(POINT pt, WPARAM wParam);
};

extern ToolsModel toolsModel;

static inline int Zoomed(int xy)
{
    return xy * toolsModel.GetZoom() / 1000;
}

static inline int UnZoomed(int xy)
{
    return xy * 1000 / toolsModel.GetZoom();
}

static inline void Zoomed(POINT& pt)
{
    pt = { Zoomed(pt.x), Zoomed(pt.y) };
}

static inline void Zoomed(RECT& rc)
{
    rc = { Zoomed(rc.left), Zoomed(rc.top), Zoomed(rc.right), Zoomed(rc.bottom) };
}

static inline void UnZoomed(POINT& pt)
{
    pt = { UnZoomed(pt.x), UnZoomed(pt.y) };
}

static inline void UnZoomed(RECT& rc)
{
    rc = { UnZoomed(rc.left), UnZoomed(rc.top), UnZoomed(rc.right), UnZoomed(rc.bottom) };
}
