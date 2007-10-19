/* Copyright Krzysztof Kowalczyk 2006-2007
   License: GPLv2 */
#ifndef SUMATRAPDF_H_
#define SUMATRAPDF_H_

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER
#define WINVER 0x0410
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#ifndef _WIN32_WINDOWS                // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                        // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600        // Change this to the appropriate value to target other versions of IE.
#endif

//#define WIN32_LEAN_AND_MEAN                // Exclude rarely-used stuff from Windows headers

#include <windows.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "resource.h"

#include "win_util.h"
#include "DisplayModelSplash.h"
#include "DisplayModelFitz.h"

/* TODO: Currently not used. The idea is to be able to switch between different
   visual styles. Because I can. */
enum AppVisualStyle {
    VS_WINDOWS = 1,
    VS_AMIGA
};

/* Current state of a window:
  - WS_ERROR_LOADING_PDF - showing an error message after failing to open a PDF
  - WS_SHOWING_PDF - showing a PDF file
  - WS_ABOUT - showing "about" screen */
enum WinState {
    WS_ERROR_LOADING_PDF = 1,
    WS_SHOWING_PDF,
    WS_ABOUT
};

/* When doing "about" animation, remembers the current animation state */
typedef struct {
    HWND        hwnd;
    int         frame;
    UINT_PTR    timerId;
} AnimState;

/* Describes actions which can be performed by mouse */
enum MouseAction {
	MA_IDLE = 0,
	MA_DRAGGING,
	MA_SELECTING
};

/* Represents selected area on given page */
typedef struct SelectionOnPage {
	int				pageNo;
	RectD			selectionPage;		/* position of selection rectangle on page */
	RectI			selectionCanvas;	/* position of selection rectangle on canvas */
	SelectionOnPage	*next;				/* pointer to next page with selected area
										 * or NULL if such page not exists */
} SelectionOnPage;

/* Describes information related to one window with (optional) pdf document
   on the screen */
class WindowInfo {
public:
    WindowInfo() {
        memzero(this, sizeof(*this)); // TODO: this might not be valid
    }
    void GetCanvasSize() {
        GetClientRect(hwndCanvas, &m_canvasRc);
    }
    int winDx() { return rect_dx(&m_canvasRc); }
    int winDy() { return rect_dy(&m_canvasRc); }
    SizeI winSize() { return SizeI(rect_dx(&m_canvasRc), rect_dy(&m_canvasRc)); }

    /* points to the next element in the list or the first element if
       this is the first element */
    WindowInfo *    next;
    WinState        state;
    WinState        prevState;

    DisplayModel *  dm;
    HWND            hwndFrame;
    HWND            hwndCanvas;
    HWND            hwndToolbar;
    HWND            hwndReBar;

    HDC             hdc;
    BITMAPINFO *    dibInfo;

    /* bitmap and hdc for (optional) double-buffering */
    HDC             hdcToDraw;
    HDC             hdcDoubleBuffer;
    HBITMAP         bmpDoubleBuffer;

    PdfLink *       linkOnLastButtonDown;
    const char *    url;

	/*if set to TRUE, page rotating, zooming, and scrolling is impossible */
	bool			documentBlocked;

	MouseAction     mouseAction;

    /* when dragging the document around, this is previous position of the
       cursor. A delta between previous and current is by how much we
       moved */
    int             dragPrevPosX, dragPrevPosY;
    AnimState       animState;

	bool showSelection;

	/* selection rectangle in screen coordinates
	 * while selecting, it represents area which is being selected */
	RectI selectionRect;

	/* after selection is done, the selected area is converted
	 * to user coordinates for each page which has not empty intersection with it */
	SelectionOnPage *selectionOnPage;


private:
    RECT m_canvasRc;
};

#endif

#define SUMATRAPDF_API __declspec(dllexport)
extern "C" {
    SUMATRAPDF_API void Sumatra_LoadPDF(WindowInfo* pdfWin, const char *pdfFile);
    SUMATRAPDF_API void Sumatra_Print(WindowInfo* pdfWin);
    SUMATRAPDF_API void Sumatra_PrintPDF(WindowInfo* pdfWin, const char *pdfFile, long showOptionWindow);
    SUMATRAPDF_API void Sumatra_SetDisplayMode(WindowInfo* pdfWin, long displayMode);
    SUMATRAPDF_API long Sumatra_GoToNextPage(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_GoToPreviousPage(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_GoToFirstPage(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_GoToLastPage(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_GoToThisPage(WindowInfo* pdfWin, long pageNumber);
    SUMATRAPDF_API long Sumatra_GetNumberOfPages(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_GetCurrentPage(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_ZoomIn(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_ZoomOut(WindowInfo* pdfWin);
    SUMATRAPDF_API long Sumatra_SetZoom(WindowInfo* pdfWin, long zoomValue);
    SUMATRAPDF_API long Sumatra_GetCurrentZoom(WindowInfo* pdfWin);
    SUMATRAPDF_API void Sumatra_Resize(WindowInfo* pdfWin);
    SUMATRAPDF_API void Sumatra_ClosePdf(WindowInfo* pdfWin);
    SUMATRAPDF_API void Sumatra_ShowPrintDialog(WindowInfo* pdfWin);
    SUMATRAPDF_API WindowInfo* Sumatra_Init(HWND parentHandle);
    SUMATRAPDF_API void Sumatra_Exit();
}
