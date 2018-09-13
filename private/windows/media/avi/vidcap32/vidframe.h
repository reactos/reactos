/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   vidframe.h: Frame for capture window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * interface to vidframe window class - this window class creates a child
 * AVICAP window, and is responsible for positioning it within the vidframe
 * window, framing it, and scrolling it if it will not fit. Call
 * vidframeCreate to create the window, and vidframeLayout if the
 * video image size might have changed.
 */



/*
 * create a frame window and child capture window at the
 * given location. Initialise the class if this is the
 * first time through.
 *
 * returns the window handle of the frame window
 * (or NULL if failure). returns the window handle of the AVICAP window
 * via phwndCap.
 */
HWND vidframeCreate(
    HWND hwndParent,
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    int x,
    int y,
    int cx,
    int cy,
    HWND FAR * phwndCap
);


/*
 * layout the window  - decide if we need scrollbars or
 * not, and position the avicap window correctly
 *
 * called if the size of the vidframe window changes or can be
 * called externally whenever the video size might have changed.
 */
void vidframeLayout(HWND hwndFrame, HWND hwndCap);

/*
 * change the background fill brush to be one of-
 *  IDD_PrefsDefBackground  - windows default background colour
 *  IDD_PrefsLtGrey - light grey
 *  IDD_PrefsDkGrey - dark grey
 *  IDD_PrefsBlack - black
 */
void vidframeSetBrush(HWND hwnd, int iPref);

