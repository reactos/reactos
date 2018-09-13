/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/

LRESULT CALLBACK QCQPCtrlWndProc (HWND, UINT, WPARAM, LPARAM) ;
     /*
      * call -   QCQPProc (hWnd, iMessage, wParam, lParam)
      * hWnd         parent's window handle
      * iMessage     window's messages
      * wParam       word parameter
      * lParam       long parameter
      */

void EnableQCQPCtrl (HWND, int, BOOL) ;
     /*
      * call -   EnableQCQPCtrl (hWnd, enable) ;
      * hWnd         control's parent handle
      * id           control's id
      * enable       TRUE to enable or FALSE to grayed out the control
      */

HWND CreateQCQPWindow(LPSTR, DWORD, int, int, int, int, HWND, HMENU, HANDLE, WPARAM) ;
     /*
      * call -   CreateQCQPWindow (title, style, x, y, dx, dy, hWnd, id, hInst, message) ;
      * title        window's title
      * style        search for "definitions for user control styles"
      *              window's style in low word
      *              window's bitmap reference ID in high word
      * x            left position
      * y            top position
      * dx           width
      * dy           height
      * hWnd         parent's handle
      * id           control's id
      * hInst        parent's instance handle
      * message      message to send back to parent's wParam under WM_COMMAND
      */

/* definitions for user control styles */
/* it is important that the following definitions must be in pair and the
 * latter one must be an incremental of the first by one unit.
 */

/* definitions for bitmap button */
#define QCQP_CS_PUSHBUTTON      2
#define QCQP_CS_LATCHBUTTON     3

// definitionS for class byte window extra indexes
#ifdef WIN32
#define CBWNDEXTRA_STYLE        0       // index to button style
#define CBWNDEXTRA_STATE        4       // index to button state
#define CBWNDEXTRA_TEXTFORMAT   8       // index to button text format
#define CBWNDEXTRA_BITMAP       12       // index to bitmap reference
#define CBWNDEXTRA_MESSAGE      16       // index to message to send back to parent
#define CBWNDEXTRA_FONT         16       // index to a font handle
#define CBWNDEXTRA_QCQPCTRL (CBWNDEXTRA_MESSAGE + 4*sizeof(PVOID))
#else
#define CBWNDEXTRA_STYLE        0       // index to button style
#define CBWNDEXTRA_STATE        2       // index to button state
#define CBWNDEXTRA_TEXTFORMAT   4       // index to button text format
#define CBWNDEXTRA_BITMAP       4       // index to bitmap reference
#define CBWNDEXTRA_MESSAGE      6       // index to message to send back to parent
#define CBWNDEXTRA_FONT         6       // index to a font handle
#define CBWNDEXTRA_QCQPCTRL 14
#endif

// Bitmaps stored in window
#ifdef WIN32
#define CBWNDEXTRA_BMAP_NORMAL  (CBWNDEXTRA_MESSAGE + sizeof(PVOID))
#define CBWNDEXTRA_BMAP_PUSHED  (CBWNDEXTRA_BMAP_NORMAL  + sizeof(PVOID))
#define CBWNDEXTRA_BMAP_GREYED  (CBWNDEXTRA_BMAP_PUSHED  + sizeof(PVOID))
#else
#define CBWNDEXTRA_BMAP_NORMAL  8
#define CBWNDEXTRA_BMAP_PUSHED  10
#define CBWNDEXTRA_BMAP_GREYED  12
#endif

