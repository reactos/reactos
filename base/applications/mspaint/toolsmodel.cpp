/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/toolsmodel.cpp
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
    m_pToolObject = ToolBase::createToolObject(m_activeTool);
}

ToolsModel::~ToolsModel()
{
    delete m_pToolObject;
}

int ToolsModel::GetLineWidth() const
{
    return m_lineWidth;
}

void ToolsModel::SetLineWidth(int nLineWidth)
{
    m_lineWidth = nLineWidth;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetShapeStyle() const
{
    return m_shapeStyle;
}

void ToolsModel::SetShapeStyle(int nShapeStyle)
{
    m_shapeStyle = nShapeStyle;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetBrushStyle() const
{
    return m_brushStyle;
}

void ToolsModel::SetBrushStyle(int nBrushStyle)
{
    m_brushStyle = nBrushStyle;
    NotifyToolSettingsChanged();
}

TOOLTYPE ToolsModel::GetActiveTool() const
{
    return m_activeTool;
}

void ToolsModel::SetActiveTool(TOOLTYPE nActiveTool)
{
    m_activeTool = nActiveTool;
    delete m_pToolObject;
    m_pToolObject = ToolBase::createToolObject(nActiveTool);
    NotifyToolChanged();
}

int ToolsModel::GetAirBrushWidth() const
{
    return m_airBrushWidth;
}

void ToolsModel::SetAirBrushWidth(int nAirBrushWidth)
{
    m_airBrushWidth = nAirBrushWidth;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetRubberRadius() const
{
    return m_rubberRadius;
}

void ToolsModel::SetRubberRadius(int nRubberRadius)
{
    m_rubberRadius = nRubberRadius;
    NotifyToolSettingsChanged();
}

BOOL ToolsModel::IsBackgroundTransparent() const
{
    return m_transpBg;
}

void ToolsModel::SetBackgroundTransparent(BOOL bTransparent)
{
    m_transpBg = bTransparent;
    NotifyToolSettingsChanged();
}

int ToolsModel::GetZoom() const
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
    selectionWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
}

void ToolsModel::NotifyZoomChanged()
{
    toolSettingsWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
}

void ToolsModel::OnDown(BUTTON_TYPE button, LONG x, LONG y, BOOL bDoubleClick)
{
    m_pToolObject->begin();
    m_pToolObject->OnDown(button, x, y, bDoubleClick);
    m_pToolObject->end();
}

void ToolsModel::OnMove(BUTTON_TYPE button, LONG x, LONG y)
{
    m_pToolObject->begin();
    m_pToolObject->OnMove(button, x, y);
    m_pToolObject->end();
}

void ToolsModel::OnUp(BUTTON_TYPE button, LONG x, LONG y)
{
    m_pToolObject->begin();
    m_pToolObject->OnUp(button, x, y);
    m_pToolObject->end();
}

void ToolsModel::OnCancelDraw()
{
    m_pToolObject->begin();
    m_pToolObject->OnCancelDraw();
    m_pToolObject->end();
}
