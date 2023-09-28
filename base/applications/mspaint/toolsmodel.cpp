/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Keep track of tool parameters, notify listeners
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

ToolsModel toolsModel;

/* FUNCTIONS ********************************************************/

ToolsModel::ToolsModel()
{
    m_lineWidth = m_penWidth = 1;
    m_brushWidth = 4;
    m_shapeStyle = 0;
    m_brushStyle = BrushStyleRound;
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
    for (size_t i = 0; i < _countof(m_tools); ++i)
        delete m_tools[i];
}

ToolBase *ToolsModel::GetOrCreateTool(TOOLTYPE nTool)
{
    if (!m_tools[nTool])
        m_tools[nTool] = ToolBase::createToolObject(nTool);

    return m_tools[nTool];
}

BOOL ToolsModel::IsSelection() const
{
    return (GetActiveTool() == TOOL_RECTSEL || GetActiveTool() == TOOL_FREESEL);
}

int ToolsModel::GetLineWidth() const
{
    return m_lineWidth;
}

void ToolsModel::SetLineWidth(int nLineWidth)
{
    m_lineWidth = nLineWidth;
    NotifyToolSettingsChanged();
    imageModel.NotifyImageChanged();
}

INT ToolsModel::GetPenWidth() const
{
    return m_penWidth;
}

void ToolsModel::SetPenWidth(INT nPenWidth)
{
    m_penWidth = nPenWidth;
    NotifyToolSettingsChanged();
    imageModel.NotifyImageChanged();
}

INT ToolsModel::GetBrushWidth() const
{
    return m_brushWidth;
}

void ToolsModel::SetBrushWidth(INT nBrushWidth)
{
    m_brushWidth = nBrushWidth;
    NotifyToolSettingsChanged();
    imageModel.NotifyImageChanged();
}

void ToolsModel::MakeLineThickerOrThinner(BOOL bThinner)
{
    INT thickness = GetLineWidth();
    SetLineWidth(bThinner ? max(1, thickness - 1) : (thickness + 1));
}

void ToolsModel::MakePenThickerOrThinner(BOOL bThinner)
{
    INT thickness = GetPenWidth();
    SetPenWidth(bThinner ? max(1, thickness - 1) : (thickness + 1));
}

void ToolsModel::MakeBrushThickerOrThinner(BOOL bThinner)
{
    INT thickness = GetBrushWidth();
    SetBrushWidth(bThinner ? max(1, thickness - 1) : (thickness + 1));
}

void ToolsModel::MakeAirBrushThickerOrThinner(BOOL bThinner)
{
    INT thickness = GetAirBrushWidth();
    SetAirBrushWidth(bThinner ? max(1, thickness - 1) : (thickness + 1));
}

void ToolsModel::MakeRubberThickerOrThinner(BOOL bThinner)
{
    INT thickness = GetRubberRadius();
    SetRubberRadius(bThinner ? max(1, thickness - 1) : (thickness + 1));
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

BrushStyle ToolsModel::GetBrushStyle() const
{
    return m_brushStyle;
}

void ToolsModel::SetBrushStyle(BrushStyle nBrushStyle)
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
    OnFinishDraw();

    selectionModel.Landing();

    m_activeTool = nActiveTool;

    switch (m_activeTool)
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_TEXT:
            // The active tool is not an actually drawing tool
            break;

        case TOOL_LINE:
        case TOOL_BEZIER:
        case TOOL_RECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
        case TOOL_FILL:
        case TOOL_AIRBRUSH:
        case TOOL_RRECT:
        case TOOL_RUBBER:
        case TOOL_BRUSH:
        case TOOL_PEN:
            // The active tool is an actually drawing tool. Save it for TOOL_COLOR to restore
            m_oldActiveTool = nActiveTool;
            break;
    }

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
    imageModel.NotifyImageChanged();
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
    if (toolBoxContainer.IsWindow())
        toolBoxContainer.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
    if (toolSettingsWindow.IsWindow())
        toolSettingsWindow.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
    if (fontsDialog.IsWindow())
        fontsDialog.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_TOOLSMODELTOOLCHANGED, m_activeTool);
}

void ToolsModel::NotifyToolSettingsChanged()
{
    if (toolSettingsWindow.IsWindow())
        toolSettingsWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_TOOLSMODELSETTINGSCHANGED);
}

void ToolsModel::NotifyZoomChanged()
{
    if (toolSettingsWindow.IsWindow())
        toolSettingsWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
    if (textEditWindow.IsWindow())
        textEditWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
    if (canvasWindow.IsWindow())
        canvasWindow.SendMessage(WM_TOOLSMODELZOOMCHANGED);
}

void ToolsModel::OnButtonDown(BOOL bLeftButton, LONG x, LONG y, BOOL bDoubleClick)
{
    m_pToolObject->beginEvent();
    g_ptStart.x = g_ptEnd.x = x;
    g_ptStart.y = g_ptEnd.y = y;
    m_pToolObject->OnButtonDown(bLeftButton, x, y, bDoubleClick);
    m_pToolObject->endEvent();
}

void ToolsModel::OnMouseMove(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnMouseMove(bLeftButton, x, y))
    {
        g_ptEnd.x = x;
        g_ptEnd.y = y;
    }
    m_pToolObject->endEvent();
}

void ToolsModel::OnButtonUp(BOOL bLeftButton, LONG x, LONG y)
{
    m_pToolObject->beginEvent();
    if (m_pToolObject->OnButtonUp(bLeftButton, x, y))
    {
        g_ptEnd.x = x;
        g_ptEnd.y = y;
    }
    m_pToolObject->endEvent();
}

void ToolsModel::OnCancelDraw()
{
    ATLTRACE("ToolsModel::OnCancelDraw()\n");
    m_pToolObject->beginEvent();
    m_pToolObject->OnCancelDraw();
    m_pToolObject->endEvent();
}

void ToolsModel::OnFinishDraw()
{
    ATLTRACE("ToolsModel::OnFinishDraw()\n");
    m_pToolObject->beginEvent();
    m_pToolObject->OnFinishDraw();
    m_pToolObject->endEvent();
}

void ToolsModel::OnDrawOverlayOnImage(HDC hdc)
{
    m_pToolObject->OnDrawOverlayOnImage(hdc);
}

void ToolsModel::OnDrawOverlayOnCanvas(HDC hdc)
{
    m_pToolObject->OnDrawOverlayOnCanvas(hdc);
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

void ToolsModel::SpecialTweak(BOOL bMinus)
{
    m_pToolObject->OnSpecialTweak(bMinus);
}
