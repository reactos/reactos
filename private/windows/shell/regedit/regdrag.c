/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGDRAG.C
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

#include "pch.h"
#include "regedit.h"

typedef struct _REGDRAGDRATA {
    POINT DragRectPoint;
    POINT HotSpotPoint;
    HWND hLockWnd;
    PRECT pDragRectArray;
    int DragRectCount;
}   REGDRAGDATA;

REGDRAGDATA s_RegDragData;

VOID
PASCAL
DrawDragRects(
    VOID
    );

/*******************************************************************************
*
*  RegEdit_DragObjects
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     hWnd, handle of RegEdit window.
*     hSourceWnd, handle of window initiating the drag.
*     hDragImageList, image used during drag operation, assumed to be at image
*        index 0.  May be NULL if pDragRectArray is valid.
*     pDragRectArray, array of rectangles to draw during drag operation.  May
*        be NULL if hDragImageList is valid.
*     DragRectCount, number of rectangles pointed to be pDragRectArray.
*     HotSpotPoint, offset of the cursor hotpoint relative to the image.
*
*******************************************************************************/

VOID
PASCAL
RegEdit_DragObjects(
    HWND hWnd,
    HIMAGELIST hDragImageList,
    PRECT pDragRectArray,
    int DragRectCount,
    POINT HotSpotPoint
    )
{

    RECT CurrentDropRect;
    HCURSOR hDropCursor;
    HCURSOR hNoDropCursor;
    HCURSOR hDragCursor;
    HCURSOR hNewDragCursor;
    POINT Point;
    BOOL fContinueDrag;
    MSG Msg;
    MSG PeekMsg;

    HTREEITEM hCurrentDropTreeItem = NULL;

    GetWindowRect(g_RegEditData.hKeyTreeWnd, &CurrentDropRect);

    GetCursorPos(&Point);
    Point.x -= CurrentDropRect.left;
    Point.y -= CurrentDropRect.top;

    if (hDragImageList != NULL) {
        if ( ImageList_BeginDrag(hDragImageList, 0, HotSpotPoint.x, HotSpotPoint.y) ) {
            ImageList_DragEnter(g_RegEditData.hKeyTreeWnd, Point.x, Point.y );
        }
    }

    s_RegDragData.hLockWnd = g_RegEditData.hKeyTreeWnd;
    LockWindowUpdate(s_RegDragData.hLockWnd);

    if (hDragImageList != NULL) {

        ShowCursor(FALSE);
        ImageList_DragShowNolock(TRUE);

    }

    else {

        s_RegDragData.HotSpotPoint = HotSpotPoint;
        s_RegDragData.pDragRectArray = pDragRectArray;
        s_RegDragData.DragRectCount = DragRectCount;

        s_RegDragData.DragRectPoint = Point;
        DrawDragRects();

    }

    hDropCursor = LoadCursor(NULL, IDC_ARROW);
    hDragCursor = hDropCursor;
    hNoDropCursor = LoadCursor(NULL, IDC_NO);

    SetCapture(hWnd);

    fContinueDrag = TRUE;

    while (fContinueDrag && GetMessage(&Msg, NULL, 0, 0)) {

        switch (Msg.message) {

            case WM_MOUSEMOVE:
                //
                //  If we have another WM_MOUSEMOVE message in the queue
                //  (before any other mouse message), don't process this
                //  mouse message.
                //

                if (PeekMessage(&PeekMsg, NULL, WM_MOUSEFIRST, WM_MOUSELAST,
                    PM_NOREMOVE) && PeekMsg.message == WM_MOUSEMOVE)
                    break;

                if (!PtInRect(&CurrentDropRect, Msg.pt)) {

                    hNewDragCursor = hNoDropCursor;

                }

                else {

                    hNewDragCursor = hDropCursor;

                }

                if (hNewDragCursor != hDragCursor) {

                    if (hDragImageList != NULL) {

                        if (hNewDragCursor == hDropCursor) {

                            ImageList_DragShowNolock(TRUE);
                            ShowCursor(FALSE);

                        }

                        else {

                            ImageList_DragShowNolock(FALSE);
                            ShowCursor(TRUE);
                            SetCursor(hNewDragCursor);

                        }

                    }

                    else
                        SetCursor(hNewDragCursor);

                    hDragCursor = hNewDragCursor;

                }

                Msg.pt.x -= CurrentDropRect.left;
                Msg.pt.y -= CurrentDropRect.top;

                {

                TV_HITTESTINFO TVHitTestInfo;
                HTREEITEM hTreeItem;

                TVHitTestInfo.pt = Msg.pt;
                hTreeItem = TreeView_HitTest(g_RegEditData.hKeyTreeWnd, &TVHitTestInfo);

                if (hTreeItem != hCurrentDropTreeItem) {

                    ImageList_DragShowNolock(FALSE);

//                    DbgPrintf(("Got a drop target!!!\n"));

//                    SetWindowRedraw(g_RegEditData.hKeyTreeWnd, FALSE);

                    TreeView_SelectDropTarget(g_RegEditData.hKeyTreeWnd, hTreeItem);

//                    SetWindowRedraw(g_RegEditData.hKeyTreeWnd, TRUE);

                    hCurrentDropTreeItem = hTreeItem;

                    ImageList_DragShowNolock(TRUE);

                }

                }

                if (hDragImageList != NULL)
                    ImageList_DragMove(Msg.pt.x, Msg.pt.y);

                else {

                    DrawDragRects();
                    s_RegDragData.DragRectPoint = Msg.pt;
                    DrawDragRects();

                }
                break;

            case WM_KEYDOWN:
                if (Msg.wParam != VK_ESCAPE)
                    break;
                //  FALL THROUGH

            case WM_LBUTTONDOWN:
            case WM_RBUTTONDOWN:
                fContinueDrag = FALSE;
                break;

            case WM_LBUTTONUP:
            case WM_RBUTTONUP:
                fContinueDrag = FALSE;
                break;

            default:
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
                break;

        }

    }

    ReleaseCapture();

    if (hDragImageList != NULL) {

        ImageList_DragShowNolock(FALSE);
        ImageList_EndDrag();

        if (hDragCursor == hDropCursor)
            ShowCursor(TRUE);

    }

    else
        DrawDragRects();

    LockWindowUpdate(NULL);

}

/*******************************************************************************
*
*  DragDragRects
*
*  DESCRIPTION:
*
*  PARAMETERS:
*     (none).
*
*******************************************************************************/

VOID
PASCAL
DrawDragRects(
    VOID
    )
{

    HDC hDC;
    int Index;
    RECT Rect;

    hDC = GetDCEx(s_RegDragData.hLockWnd, NULL, DCX_WINDOW | DCX_CACHE |
        DCX_LOCKWINDOWUPDATE);

    for (Index = s_RegDragData.DragRectCount; Index >= 0; Index--) {

        Rect = s_RegDragData.pDragRectArray[Index];
        OffsetRect(&Rect, s_RegDragData.DragRectPoint.x -
            s_RegDragData.HotSpotPoint.x, s_RegDragData.DragRectPoint.y -
            s_RegDragData.HotSpotPoint.y);
        DrawFocusRect(hDC, &Rect);

    }

    ReleaseDC(s_RegDragData.hLockWnd, hDC);

}
