/*
**  DRAGSIZE.C
**
**  Drag size bar code
**
*/

#include "precomp.h"
#pragma hdrstop


typedef struct tagDRAGSIZEINFO
{
	DWORD lpData;
	BOOL bDragging;
	POINT ptNow;
	HFONT hFont;
} DRAGSIZEINFO, *PDRAGSIZEINFO;

extern HINSTANCE hInstance;

extern TCHAR g_szNULL[];

static TCHAR szDragSizeClass[] = DRAGSIZECLASSNAME;

LRESULT NEAR PASCAL InitDragSizeWnd(HWND hWnd, LPCREATESTRUCT lpCreate)
{
	PDRAGSIZEINFO pDragSizeInfo;

	/* Create the status info struct; abort if it does not exist,
	** otherwise save it in the window structure
	*/
	pDragSizeInfo = ALLOCWINDOWPOINTER(PDRAGSIZEINFO, sizeof(DRAGSIZEINFO));
	if (!pDragSizeInfo)
	{
		return(-1);
	}
	SETWINDOWPOINTER(hWnd, PDRAGSIZEINFO, pDragSizeInfo);
	pDragSizeInfo->lpData = (DWORD)lpCreate->lpCreateParams;
	pDragSizeInfo->bDragging = FALSE;

	return(0);
}


/* Track the mouse and send messages to the parent whenever it moves.
*/
void NEAR PASCAL DragSize(HWND hWnd, PDRAGSIZEINFO pDragSizeInfo, POINT ptStart)
{
	MSG msg;
	HWND hwndParent;
        LONG wID;

	if (!pDragSizeInfo || pDragSizeInfo->bDragging)
	{
		return;
	}

	pDragSizeInfo->bDragging = TRUE;
	pDragSizeInfo->ptNow     = ptStart;

	SetCapture(hWnd);

	hwndParent = GetParent(hWnd);
        wID = GETWINDOWID(hWnd);

        SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(wID, DSN_BEGINDRAG), (LPARAM)hWnd);

	for ( ; ; )
	{
		if (GetCapture() != hWnd)
		{
EndAbort:
			/* Abort the process.
			*/
			pDragSizeInfo->ptNow = ptStart;
                        SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(wID, DSN_DRAGGING), (LPARAM)hWnd);
			goto EndAdjust;
		}

		if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			continue;
		}

		switch (msg.message)
		{
		case WM_KEYDOWN:
			switch (msg.wParam)
			{
			case VK_ESCAPE:
AbortAdjust:
				SetCapture(NULL);
				goto EndAbort;

			default:
				break;
			}
			break;

		case WM_KEYUP:
		case WM_CHAR:
			break;

		case WM_LBUTTONDOWN:
			/* This shouldn't happen.
			*/
			goto AbortAdjust;

		case WM_MOUSEMOVE:
            LPARAM2POINT( msg.lParam, &(pDragSizeInfo->ptNow) );
            
            TraceMsg(TF_GENERAL, "DragSize: ptNow = (%d, %d)\n", pDragSizeInfo->ptNow.x, pDragSizeInfo->ptNow.y);
            
            SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(wID, DSN_DRAGGING), (LPARAM)hWnd);
			break;

		case WM_LBUTTONUP:
			/* All done.
			*/
			SetCapture(NULL);
			goto EndAdjust;

		case WM_RBUTTONDOWN:
			goto AbortAdjust;

		default:
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			break;
		}
	}

EndAdjust:
        SendMessage(hwndParent, WM_COMMAND, MAKEWPARAM(wID, DSN_ENDDRAG), (LPARAM)hWnd);
	pDragSizeInfo->bDragging = FALSE;
}


/* Ask the parent to paint the window.
*/
void NEAR PASCAL PaintDragSizeWnd(HWND hWnd, PDRAGSIZEINFO pDragSizeInfo)
{
	PAINTSTRUCT ps;
	DRAWITEMSTRUCT dis;
	HDC hDC;
	HFONT hOldFont = NULL;

	if (!pDragSizeInfo)
	{
		return;
	}

	hDC = BeginPaint(hWnd, &ps);

	if (pDragSizeInfo->hFont)
	{
		hOldFont = SelectObject(hDC, pDragSizeInfo->hFont);
	}

	/* Fill in the DRAWITEMSTRUCT.  Note that some of the fields are
	** undefined.
	*/
	dis.CtlID    = GetDlgCtrlID(hWnd);
	dis.hwndItem = hWnd;
	dis.hDC      = hDC;
	GetClientRect(hWnd, &dis.rcItem);
	dis.itemData = pDragSizeInfo->lpData;

	SendMessage(GetParent(hWnd), WM_DRAWITEM, GetDlgCtrlID(hWnd),
		(LPARAM)(LPDRAWITEMSTRUCT)&dis);

	if (hOldFont)
	{
		SelectObject(hDC, hOldFont);
	}

	EndPaint(hWnd, &ps);
}


LRESULT CALLBACK DragSizeWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
#define lpCreate ((LPCREATESTRUCT)lParam)

	PDRAGSIZEINFO pDragSizeInfo;

	pDragSizeInfo = GETWINDOWPOINTER(hWnd, PDRAGSIZEINFO);

	switch (uMessage)
	{
	case WM_NCCREATE:
                SendMessage(lpCreate->hwndParent, WM_COMMAND,
                        MAKEWPARAM((SHORT)(lpCreate->hMenu), DSN_NCCREATE), (LPARAM)hWnd);
		break;

	case WM_CREATE:
		InitDragSizeWnd(hWnd, (LPCREATESTRUCT)lParam);
		break;

	case WM_DESTROY:
		if (pDragSizeInfo)
		{
			FREEWINDOWPOINTER(pDragSizeInfo);
			SETWINDOWPOINTER(hWnd, PDRAGSIZEINFO, NULL);
		}
		break;

        case WM_LBUTTONDOWN: {
                    POINT pt;
                    LPARAM2POINT( lParam, &pt );
                    DragSize(hWnd, pDragSizeInfo, pt);
                }
                break;

	case WM_PAINT:
		PaintDragSizeWnd(hWnd, pDragSizeInfo);
		return(0);

	case WM_SETFONT:
		if (!pDragSizeInfo)
		{
			return(1L);
		}
		pDragSizeInfo->hFont = (HFONT)wParam;

		if (LOWORD(lParam))
		{
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		return(0L);

	case WM_GETFONT:
		if (!pDragSizeInfo)
		{
                        return(LONG)(NULL);
		}
		return(MAKELRESULT(pDragSizeInfo->hFont, 0));

        case DSM_DRAGPOS: {
                    LPPOINT lppt = (LPPOINT)lParam;

                    if (!pDragSizeInfo || !pDragSizeInfo->bDragging || lppt == NULL)
                    {
                            return(-1L);
                    }

                    *lppt = pDragSizeInfo->ptNow;

                    return(0);
                }
		break;

	default:
		break;
	}

	return(DefWindowProc(hWnd, uMessage, wParam, lParam));
}


BOOL FAR PASCAL InitDragSizeClass(void)
{
	WNDCLASS rClass;

	if (GetClassInfo(hInstance, szDragSizeClass, &rClass))
	{
		return(TRUE);
	}

	rClass.style         = 0;
	rClass.lpfnWndProc   = DragSizeWndProc;
	rClass.cbClsExtra    = 0;
	rClass.cbWndExtra    = sizeof(PDRAGSIZEINFO);
	rClass.hInstance     = hInstance;
	rClass.hIcon         = NULL;
	rClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	rClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
	rClass.lpszMenuName  = NULL;
	rClass.lpszClassName = szDragSizeClass;

	return(RegisterClass(&rClass));
}


HWND WINAPI CreateDragSizeWindow(LONG style, int x, int y, int wid, int hgt, HWND hwndParent, LONG wID)
{
	/* Create a default window and return
	*/
	return(CreateWindow(szDragSizeClass, g_szNULL, style,
		x, y, wid, hgt, hwndParent, (HMENU)wID, hInstance, NULL));
}
