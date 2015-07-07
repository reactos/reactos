/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/toolsmodel.cpp
 * PURPOSE:     Keep track of tool parameters, notify listeners
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

ToolsModel::ToolsModel()
{
    m_lineWidth = 1;
    m_shapeStyle = 0;
    m_brushStyle = 0;
    m_activeTool = TOOL_PEN;
    m_airBrushWidth = 5;
    m_rubberRadius = 4;
    m_transpBg = FALSE;
    m_zoom = 1000;
}

int ToolsModel::GetLineWidth()
{
    return m_lineWidth;
}

void ToolsModel::SetLineWidth(int nLineWidth)
{
    m_lineWidth = nLineWidth;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetShapeStyle()
{
    return m_shapeStyle;
}

void ToolsModel::SetShapeStyle(int nShapeStyle)
{
    m_shapeStyle = nShapeStyle;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetBrushStyle()
{
    return m_brushStyle;
}

void ToolsModel::SetBrushStyle(int nBrushStyle)
{
    m_brushStyle = nBrushStyle;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetActiveTool()
{
    return m_activeTool;
}

void ToolsModel::SetActiveTool(int nActiveTool)
{
    m_activeTool = nActiveTool;
    NotifyToolChanged();
}

int ToolsModel::GetAirBrushWidth()
{
    return m_airBrushWidth;
}

void ToolsModel::SetAirBrushWidth(int nAirBrushWidth)
{
    m_airBrushWidth = nAirBrushWidth;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetRubberRadius()
{
    return m_rubberRadius;
}

void ToolsModel::SetRubberRadius(int nRubberRadius)
{
    m_rubberRadius = nRubberRadius;
    NotifyToolSettingsChanged();
}

BOOL ToolsModel::IsBackgroundTransparent()
{
    return m_transpBg;
}

void ToolsModel::SetBackgroundTransparent(BOOL bTransparent)
{
    m_transpBg = bTransparent;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetZoom()
{
    return m_zoom;
}

void ToolsModel::SetZoom(int nZoom)
{
    m_zoom = nZoom;
    NotifyZoomChanged();
}

void ToolsModel::NotifyToolChanged()
{
    toolBoxContainer.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
    toolSettingsWindow.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
    textEditWindow.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
}

void ToolsModel::NotifyToolSettingsChanged()
{
    toolSettingsWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
}

void ToolsModel::NotifyZoomChanged()
{
    toolSettingsWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
}
