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
    ZeroMemory(m_pTools, sizeof(m_pTools));
    m_pToolObject = getToolObject(m_activeTool);
}

ToolBase *ToolsModel::getToolObject(TOOLTYPE nActiveTool)
{
    if (!m_pTools[nActiveTool])
        m_pTools[nActiveTool] = ToolBase::createToolObject(nActiveTool);

    return m_pTools[nActiveTool];
}

ToolsModel::~ToolsModel()
{
    for (size_t i = 0; i < _countof(m_pTools); ++i)
    {
        delete m_pTools[i];
        m_pTools[i] = NULL;
    }
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
    m_pToolObject = getToolObject(m_activeTool);
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

void ToolsModel::OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
{
    m_pToolObject->begin();
    m_pToolObject->OnButtonDown(bLeftButton, x, y, bDoubleClick);
    m_pToolObject->end();
}

void ToolsModel::OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->begin();
    m_pToolObject->OnMouseMove(bLeftButton, x, y);
    m_pToolObject->end();
}

void ToolsModel::OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->begin();
    m_pToolObject->OnButtonUp(bLeftButton, x, y);
    m_pToolObject->end();
}

void ToolsModel::OnCancelDraw()
{
    m_pToolObject->begin();
    m_pToolObject->OnCancelDraw();
    m_pToolObject->end();
}

void ToolsModel::resetTool()
{
    m_pToolObject->reset();
}
