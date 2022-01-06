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
    m_oldActiveTool = m_activeTool = TOOL_PEN;
    m_airBrushWidth = 5;
    m_rubberRadius = 4;
    m_transpBg = FALSE;
    m_zoom = 1000;
    ZeroMemory(&m_tools, sizeof(m_tools));
    m_pToolObject = GetOrCreateTool(m_activeTool);
}

ToolsModel::~ToolsModel()
{
    for (size_t i = 0; i < TOOL_MAX + 1; ++i)
        delete m_tools[i];
}

ToolBase *ToolsModel::GetOrCreateTool(TOOLTYPE nTool)
{
    if (!m_tools[nTool])
        m_tools[nTool] = ToolBase::createToolObject(nTool);

    return m_tools[nTool];
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

TOOLTYPE ToolsModel::GetOldActiveTool() const
{
    return m_oldActiveTool;
}

void ToolsModel::SetActiveTool(TOOLTYPE nActiveTool)
{
    switch (m_activeTool)
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_RUBBER:
        case TOOL_COLOR:
        case TOOL_ZOOM:
            break;

        case TOOL_TEXT:
            if (nActiveTool != TOOL_TEXT)
            {
                // Finish the text
                OnButtonDown(TRUE, -1, -1, TRUE);
                OnButtonUp(TRUE, -1, -1);
            }
            break;

        default:
            m_oldActiveTool = m_activeTool;
            break;
    }

    m_activeTool = nActiveTool;
    m_pToolObject = GetOrCreateTool(m_activeTool);
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

    if (fontsDialog.IsWindow())
        fontsDialog.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);

    textEditWindow.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
}

void ToolsModel::NotifyToolSettingsChanged()
{
    toolSettingsWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
    selectionWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
}

void ToolsModel::NotifyZoomChanged()
{
    toolSettingsWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
}

void ToolsModel::OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
{
    m_pToolObject->beginEvent();
    updateStartAndLast(x, y);
    m_pToolObject->OnButtonDown(bLeftButton, x, y, bDoubleClick);
    m_pToolObject->endEvent();
}

void ToolsModel::OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    m_pToolObject->OnMouseMove(bLeftButton, x, y);
    updateLast(x, y);
    m_pToolObject->endEvent();
}

void ToolsModel::OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    m_pToolObject->OnButtonUp(bLeftButton, x, y);
    updateLast(x, y);
    m_pToolObject->endEvent();
}

void ToolsModel::OnCancelDraw()
{
    m_pToolObject->beginEvent();
    m_pToolObject->OnCancelDraw();
    m_pToolObject->endEvent();
}

void ToolsModel::resetTool()
{
    m_pToolObject->reset();
}

void ToolsModel::selectAll()
{
    SetActiveTool(TOOL_RECTSEL);
    OnButtonDown(TRUE, 0, 0, FALSE);
    OnMouseMove(TRUE, imageModel.GetWidth(), imageModel.GetHeight());
    OnButtonUp(TRUE, imageModel.GetWidth(), imageModel.GetHeight());
}
