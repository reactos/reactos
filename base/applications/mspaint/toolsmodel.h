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
    int GetLineWidth() const;
    void SetLineWidth(int nLineWidth);
    int GetShapeStyle() const;
    void SetShapeStyle(int nShapeStyle);
    int GetBrushStyle() const;
    void SetBrushStyle(int nBrushStyle);
    int GetActiveTool() const;
    void SetActiveTool(int nActiveTool);
    int GetAirBrushWidth() const;
    void SetAirBrushWidth(int nAirBrushWidth);
    int GetRubberRadius() const;
    void SetRubberRadius(int nRubberRadius);
    BOOL IsBackgroundTransparent() const;
    void SetBackgroundTransparent(BOOL bTransparent);
    int GetZoom() const;
    void SetZoom(int nZoom);
};
