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
    m_airBrushRadius = 5;
    m_rubberRadius = 4;
    m_transpBg = FALSE;
    m_zoom = 1000;
    m_pToolObject = GetOrCreateTool(m_activeTool);
}

ToolsModel::~ToolsModel()
{
    delete m_pToolObject;
    m_pToolObject = NULL;
}

ToolBase *ToolsModel::GetOrCreateTool(TOOLTYPE nTool)
{
    delete m_pToolObject;
    m_pToolObject = ToolBase::createToolObject(nTool);
    return m_pToolObject;
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
    INT thickness = GetAirBrushRadius();
    SetAirBrushRadius(bThinner ? max(1, thickness - 1) : (thickness + 1));
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
    OnEndDraw(FALSE);

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

INT ToolsModel::GetAirBrushRadius() const
{
    return m_airBrushRadius;
}

void ToolsModel::SetAirBrushRadius(INT nAirBrushRadius)
{
    m_airBrushRadius = nAirBrushRadius;
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

SIZE ToolsModel::GetToolSize() const
{
    SIZE size;
    switch (m_activeTool)
    {
        case TOOL_FREESEL:
        case TOOL_RECTSEL:
            size.cx = selectionModel.m_rc.Width();
            size.cy = selectionModel.m_rc.Height();
            break;
        case TOOL_COLOR:
        case TOOL_ZOOM:
        case TOOL_TEXT:
        case TOOL_FILL:
            size.cx = size.cy = 1;
            break;
        case TOOL_LINE:
        case TOOL_BEZIER:
        case TOOL_RECT:
        case TOOL_RRECT:
        case TOOL_SHAPE:
        case TOOL_ELLIPSE:
            size.cx = size.cy = GetLineWidth();
            break;
        case TOOL_AIRBRUSH:
            size.cx = size.cy = GetAirBrushRadius() * 2;
            break;
        case TOOL_RUBBER:
            size.cx = size.cy = GetRubberRadius() * 2;
            break;
        case TOOL_BRUSH:
            size.cx = size.cy = GetBrushWidth();
            break;
        case TOOL_PEN:
            size.cx = size.cy = GetPenWidth();
            break;
    }
    if (size.cx < 1)
        size.cx = 1;
    if (size.cy < 1)
        size.cy = 1;
    return size;
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
