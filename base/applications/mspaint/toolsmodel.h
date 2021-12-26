/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolsmodel.h
 * PURPOSE:     Keep track of tool parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

/* CLASSES **********************************************************/

class ToolsModel
{
private:
    int m_lineWidth;
    int m_shapeStyle;
    int m_brushStyle;
    int m_activeTool;
    int m_airBrushWidth;
    int m_rubberRadius;
    BOOL m_transpBg;
    int m_zoom;

    void NotifyToolChanged();
    void NotifyToolSettingsChanged();
    void NotifyZoomChanged();

public:
    ToolsModel();
    int Zoomed(int xy) const;
    int UnZoomed(int xy) const;
    int GetLineWidth();
    void SetLineWidth(int nLineWidth);
    int GetShapeStyle();
    void SetShapeStyle(int nShapeStyle);
    int GetBrushStyle();
    void SetBrushStyle(int nBrushStyle);
    int GetActiveTool();
    void SetActiveTool(int nActiveTool);
    int GetAirBrushWidth();
    void SetAirBrushWidth(int nAirBrushWidth);
    int GetRubberRadius();
    void SetRubberRadius(int nRubberRadius);
    BOOL IsBackgroundTransparent();
    void SetBackgroundTransparent(BOOL bTransparent);
    int GetZoom() const;
    void SetZoom(int nZoom);
};
