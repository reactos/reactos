/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCARET.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wucaret.c);


/*++
    void CreateCaret(<hwnd>, <hBitmap>, <nWidth>, <nHeight>)
    HWND <hwnd>;
    BITMAP <hBitmap>;
    int <nWidth>;
    int <nHeight>;

    The %CreateCaret% function creates a new shape for the system caret and
    assigns ownership of the caret to the given window. The caret shape can be a
    line, block, or bitmap as defined by the <hBitmap> parameter. If <hBitmap>
    is a bitmap handle, the <nWidth> and <nHeight> parameters are ignored; the
    bitmap defines its own width and height. (The bitmap handle must have been
    previously created by using the %CreateBitmap%, %CreateDIBitmap%, or
    %LoadBitmap% function.) If <hBitmap> is NULL or 1, <nWidth> and <nHeight>
    give the caret's width and height (in logical units); the exact width and
    height (in pixels) depend on the window's mapping mode.

    If <nWidth> or <nHeight> is zero, the caret width or height is set to the
    system's window-border width or height. Using the window-border width or
    height guarantees that the caret will be visible on a high-resolution
    display.

    The %CreateCaret% function automatically destroys the previous caret shape,
    if any, regardless of which window owns the caret. Once created, the caret
    is initially hidden. To show the caret, the %ShowCaret% function must be
    called.

    <hwnd>
        Identifies the window that owns the new caret.

    <hBitmap>
        Identifies the bitmap that defines the caret shape. If
        <hBitmap> is NULL, the caret is solid; if <hBitmap> is 1, the caret is
        gray.

    <nWidth>
        Specifies the width of the caret (in logical units).

    <nHeight>
        Specifies the height of the caret (in logical units).

    This function does not return a value.

    The system caret is a shared resource. A window should create a caret only
    when it has the input focus or is active. It should destroy the caret before
    losing the input focus or becoming inactive.

    The system's window-border width or height can be retrieved by using the
    %GetSystemMetrics% function with the SM_CXBORDER and SM_CYBORDER indexes.
--*/

ULONG FASTCALL WU32CreateCaret(PVDMFRAME pFrame)
{
    register PCREATECARET16 parg16;
    HANDLE   h32;

    GETARGPTR(pFrame, sizeof(CREATECARET16), parg16);

    h32 = (HANDLE)parg16->f2;

    // 0 -> caret is solid, 1 -> caret is gray, otherwise it's an hBitmap
    if(((DWORD)h32) > 1) {
    	h32 = HBITMAP32(h32);
    }

    CreateCaret(HWND32(parg16->f1), h32, INT32(parg16->f3), INT32(parg16->f4));

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void DestroyCaret(VOID)

    The %DestroyCaret% function destroys the current caret shape, frees the
    caret from the window that currently owns it, and removes the caret from the
    screen if it is visible. The %DestroyCaret% function checks the ownership of
    the caret and destroys the caret only if a window in the current task owns
    it.

    If the caret shape was previously a bitmap, %DestroyCaret% does not free the
    bitmap.

    This function has no parameters.

    This function does not return a value.

    The caret is a shared resource. If a window has created a caret shape, it
    destroys that shape before it loses the input focus or becomes inactive.
--*/

ULONG FASTCALL WU32DestroyCaret(PVDMFRAME pFrame)
{
    UNREFERENCED_PARAMETER(pFrame);

    DestroyCaret();

    RETURN(0);
}


/*++
    WORD GetCaretBlinkTime(VOID)

    The %GetCaretBlinkTime% function retrieves the caret blink rate. The blink
    rate is the elapsed time in milliseconds between flashes of the caret.

    This function has no parameters.

    The return value specifies the blink rate (in milliseconds).
--*/

ULONG FASTCALL WU32GetCaretBlinkTime(PVDMFRAME pFrame)
{
    ULONG ul;

    UNREFERENCED_PARAMETER(pFrame);

    ul = GETWORD16(GetCaretBlinkTime());

    RETURN(ul);
}


/*++
    void GetCaretPos(<lpPoint>)
    LPPOINT <lpPoint>;

    The %GetCaretPos% function retrieves the caret's current position (in screen
    coordinates), and copies them to the %POINT% structure pointed to by the
    <lpPoint> parameter.

    <lpPoint>
        Points to the %POINT% structure that is to receive the screen coordinates
        of the caret.

    This function does not return a value.

    The caret position is always given in the client coordinates of the window
    that contains the caret.
--*/

ULONG FASTCALL WU32GetCaretPos(PVDMFRAME pFrame)
{
    POINT t1;
    register PGETCARETPOS16 parg16;

    GETARGPTR(pFrame, sizeof(GETCARETPOS16), parg16);

    GetCaretPos(
	&t1
    );

    PUTPOINT16(parg16->f1, &t1);
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void HideCaret(<hwnd>)
    HWND <hwnd>;

    The %HideCaret% function hides the caret by removing it from the display
    screen. Although the caret is no longer visible, it can be displayed again
    by using the %ShowCaret% function. Hiding the caret does not destroy its
    current shape.

    The %HideCaret% function hides the caret only if the given window owns the
    caret. If the <hwnd> parameter is NULL, the function hides the caret only if
    a window in the current task owns the caret.

    Hiding is cumulative. If %HideCaret% has been called five times in a row,
    %ShowCaret% must be called five times before the caret will be shown.

    <hwnd>
        Identifies the window that owns the caret, or it is NULL to indirectly
        specify the window in the current task that owns the caret.

    This function does not return a value.
--*/

ULONG FASTCALL WU32HideCaret(PVDMFRAME pFrame)
{
    register PHIDECARET16 parg16;

    GETARGPTR(pFrame, sizeof(HIDECARET16), parg16);

    HideCaret(
	HWND32(parg16->f1)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void SetCaretBlinkTime(<wMSeconds>)
    WORD <wMSeconds>;

    The %SetCaretBlinkTime% function sets the caret blink rate (elapsed time
    between caret flashes) to the number of milliseconds specified by the
    <wMSeconds> parameter. The caret flashes on or off each <wMSeconds>
    milliseconds. This means one complete flash (on-off-on) takes 2 x
    <wMSeconds> milliseconds.

    <wMSeconds>
        Specifies the new blink rate (in milliseconds).

    This function does not return a value.

    The caret is a shared resource. A window should set the caret blink rate
    only if it owns the caret. It should restore the previous rate before it
    loses the input focus or becomes inactive.
--*/

ULONG FASTCALL WU32SetCaretBlinkTime(PVDMFRAME pFrame)
{
    register PSETCARETBLINKTIME16 parg16;

    GETARGPTR(pFrame, sizeof(SETCARETBLINKTIME16), parg16);

    SetCaretBlinkTime(
	WORD32(parg16->f1)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void SetCaretPos(<X>, <Y>)
    int <X>;
    int <Y>;

    The %SetCaretPos% function moves the caret to the position given by logical
    coordinates specified by the <X> and <Y> parameters. Logical coordinates are
    relative to the client area of the window that owns them and are affected by
    the window's mapping mode, so the exact position in pixels depends on this
    mapping mode.

    The %SetCaretPos% function moves the caret only if it is owned by a window
    in the current task. %SetCaretPos% moves the caret whether or not the caret
    is hidden.

    <X>
        Specifies the new x-coordinate (in logical coordinates) of the caret.

    <Y>
        Specifies the new <y>-coordinate (in logical coordinates) of the
        caret.

    This function does not return a value.

    The caret is a shared resource. A window should not move the caret if it
    does not own the caret.
--*/

ULONG FASTCALL WU32SetCaretPos(PVDMFRAME pFrame)
{
    register PSETCARETPOS16 parg16;

    GETARGPTR(pFrame, sizeof(SETCARETPOS16), parg16);

    SetCaretPos(
	INT32(parg16->f1),
	INT32(parg16->f2)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void ShowCaret(<hwnd>)

    The %ShowCaret% function shows the caret on the display at the caret's
    current position. Once shown, the caret begins flashing automatically.

    The %ShowCaret% function shows the caret only if it has a current shape and
    has not been hidden two or more times in a row. If the caret is not owned by
    the given window, the caret is not shown. If the <hwnd> parameter is NULL,
    the %ShowCaret% function shows the caret only if it is owned by a window in
    the current task.

    Hiding the caret is accumulative. If the %HideCaret% function has been
    called five times in a row, %ShowCaret% must be called five times to show
    the caret.

    <hwnd>
        Identifies the window that owns the caret, or is NULL to specify
        indirectly the owner window in the current task.

    This function does not return a value.

    The caret is a shared resource. A window should show the caret only when it
    has the input focus or is active.
--*/

ULONG FASTCALL WU32ShowCaret(PVDMFRAME pFrame)
{
    register PSHOWCARET16 parg16;

    GETARGPTR(pFrame, sizeof(SHOWCARET16), parg16);

    ShowCaret(
	HWND32(parg16->f1)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}
