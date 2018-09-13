/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDRAG.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Drag and drop routines for the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGDRAG
#define _INC_REGDRAG

VOID
PASCAL
RegEdit_DragObjects(
    HWND hWnd,
    HIMAGELIST hDragImageList,
    PRECT pDragRectArray,
    int DragRectCount,
    POINT HotSpotPoint
    );

#endif // _INC_REGDRAG
