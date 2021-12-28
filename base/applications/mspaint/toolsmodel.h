/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolsmodel.h
 * PURPOSE:     Keep track of tool parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
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
};

enum BUTTON_TYPE
{
    BUTTON_LEFT,
    BUTTON_RIGHT,
};

/* CLASSES **********************************************************/

struct ToolBase
{
    TOOLTYPE m_tool;
    HDC m_hdc;
    COLORREF m_fg, m_bg;

    ToolBase(TOOLTYPE tool) : m_tool(tool), m_hdc(NULL), m_fg(0), m_bg(0)
    {
    }

    virtual ~ToolBase()
    {
    }

    virtual void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick);
    virtual void OnMove(BUTTON_TYPE button, LONG x, LONG y);
    virtual void OnUp(BUTTON_TYPE button, LONG x, LONG y);
    virtual void OnCancelDraw();

    void begin();
    void end();

    static ToolBase* createToolObject(TOOLTYPE type);
};

class ToolsModel
{
private:
    int m_lineWidth;
    int m_shapeStyle;
    int m_brushStyle;
    TOOLTYPE m_activeTool;
    int m_airBrushWidth;
    int m_rubberRadius;
    BOOL m_transpBg;
    int m_zoom;
    ToolBase *m_pToolObject;

    void NotifyToolChanged();
    void NotifyToolSettingsChanged();
    void NotifyZoomChanged();

public:
    ToolsModel();
    ~ToolsModel();
    int GetLineWidth() const;
    void SetLineWidth(int nLineWidth);
    int GetShapeStyle() const;
    void SetShapeStyle(int nShapeStyle);
    int GetBrushStyle() const;
    void SetBrushStyle(int nBrushStyle);
    TOOLTYPE GetActiveTool() const;
    void SetActiveTool(TOOLTYPE nActiveTool);
    int GetAirBrushWidth() const;
    void SetAirBrushWidth(int nAirBrushWidth);
    int GetRubberRadius() const;
    void SetRubberRadius(int nRubberRadius);
    BOOL IsBackgroundTransparent() const;
    void SetBackgroundTransparent(BOOL bTransparent);
    int GetZoom() const;
    void SetZoom(int nZoom);

    void OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick);
    void OnMove(BUTTON_TYPE button, LONG x, LONG y);
    void OnUp(BUTTON_TYPE button, LONG x, LONG y);
    void OnCancelDraw();
};
