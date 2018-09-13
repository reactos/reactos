/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUCURSOR.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#include "precomp.h"
#pragma hdrstop


MODNAME(wucursor.c);

/*++
    void ClipCursor(<lpRect>)
    LPRECT <lpRect>;

    The %ClipCursor% function confines the cursor to the rectangle on the
    display screen given by the <lpRect> parameter. If a subsequent cursor
    position, given with the %SetCursorPos% function or the mouse, lies outside
    the rectangle, Windows automatically adjusts the position to keep the cursor
    inside. If <lpRect> is NULL, the cursor is free to move anywhere on the
    display screen.

    <lpRect>
        Points to a %RECT% structure that contains the screen coordinates
        of the upper-left and lower-right corners of the confining rectangle.

    This function does not return a value.

    The cursor is a shared resource. An application that has confined the cursor
    to a given rectangle must free it before relinquishing control to another
    application.
--*/

ULONG FASTCALL WU32ClipCursor(PVDMFRAME pFrame)
{
    RECT t1, *p1;
    register PCLIPCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(CLIPCURSOR16), parg16);
    p1 = GETRECT16(parg16->f1, &t1);

    ClipCursor(
        p1
        );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    HCURSOR CreateCursor(<hInstance>, <nXhotspot>, <nYhotspot>, <nWidth>,
        <nHeight>, <lpANDbitPlane>, <lpXORbitPlane>)
    HANDLE <hInstance>;
    int <nXhotspot>;
    int <nYhotspot>;
    int <nWidth>;
    int <nHeight>;
    LPSTR <lpANDbitPlane>;
    LPSTR <lpXORbitPlane>;

    The %CreateCursor% function creates a cursor that has specified width,
    height, and bit patterns.

    <hInstance>
        Identifies an instance of the module creating the cursor.

    <nXhotspot>
        Specifies the horizontal position of the cursor hotspot.

    <nYhotspot>
        Specifies the vertical position of the cursor hotspot.

    <nWidth>
        Specifies the width in pixels of the cursor.

    <nHeight>
        Specifies the height in pixels of the cursor.

    <lpANDbitPlane>
        Points to an array of bytes containing the bit values for the AND mask
        of the cursor. This can be the bits of a device-dependent monochrome
        bitmap.

    <lpXORbitPlane>
        Points to an array of bytes containing the bit values for the XOR mask
        of the cursor. This can be the bits of a device-dependent monochrome
        bitmap.

    The return value identifies the cursor if the function was successful.
    Otherwise, it is NULL.
--*/

ULONG FASTCALL WU32CreateCursor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PCREATECURSOR16 parg16;
    int     nWidth;
    int     nHeight;
    int     nPlanes;
    int     nBitsPixel;
    DWORD   nBytesAND;
    DWORD   nBytesXOR;
    LPBYTE  lpBitsAND;
    LPBYTE  lpBitsXOR;
    int     ScanLen16;

    HANDLE  h32;
    HAND16  h16;
    HAND16  hInst16;

    GETARGPTR(pFrame, sizeof(CREATECURSOR16), parg16);
    hInst16    = parg16->f1;
    nWidth     = INT32(parg16->f4);
    nHeight    = INT32(parg16->f5);

    nPlanes    = 1;     /* MONOCHROME BITMAP */
    nBitsPixel = 1;     /* MONOCHROME BITMAP */

    /*
    ** Convert the AND mask bits
    */
    ScanLen16 = (((nWidth*nBitsPixel)+15)/16) * 2 ;  // bytes/scan in 16 bit world
    nBytesAND = ScanLen16*nHeight*nPlanes;

    GETVDMPTR(parg16->f6, nBytesAND, lpBitsAND);


    /*
    ** Convert the XOR mask bits
    */
    ScanLen16 = (((nWidth*nBitsPixel)+15)/16) * 2 ;  // bytes/scan in 16 bit world
    nBytesXOR = ScanLen16*nHeight*nPlanes;

    GETVDMPTR(parg16->f7, nBytesXOR, lpBitsXOR);


    h32 = (HANDLE)CreateCursor(HMODINST32(hInst16),INT32(parg16->f2),
                              INT32(parg16->f3),
                      nWidth, nHeight, lpBitsAND, lpBitsXOR);

    if (h32) {
        h16 = (HAND16)W32Create16BitCursorIcon(hInst16,
                                       INT32(parg16->f2), INT32(parg16->f3),
                                       nWidth, nHeight, nPlanes, nBitsPixel,
                                       lpBitsAND, lpBitsXOR,
                                       nBytesAND, nBytesXOR);

        ul  = SetupCursorIconAlias(hInst16, h32, h16,
                                   HANDLE_TYPE_CURSOR, NULL, (WORD)NULL);
    } else {
        ul = 0;
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL DestroyCursor(<hCursor>)
    HCURSOR <hCursor>;

    The %DestroyCursor% function destroys a cursor that was previously created
    by the %CreateCursor% function and frees any memory that the cursor
    occupied. It should not be used to destroy any cursor that was not created
    with the %CreateCursor% function.

    <hCursor>
        Identifies the cursor to be destroyed. The cursor must not be in current
        use.

    The return value is TRUE if the function was successful. It is FALSE if
    the function failed.
--*/

ULONG FASTCALL WU32DestroyCursor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDESTROYCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(DESTROYCURSOR16), parg16);

    if (ul = GETBOOL16(DestroyCursor(HCURSOR32(parg16->f1))))
        FREEHCURSOR16(parg16->f1);

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HCURSOR SetCursor(<hCursor>)
    HCURSOR <hCursor>;

    The %SetCursor% function sets the cursor shape to the shape specified by the
    <hCursor> parameter. The cursor is set only if the new shape is different
    from the current shape. Otherwise, the function returns immediately. The
    %SetCursor% function is quite fast if the cursor identified by the <hCursor>
    parameter is the same as the current cursor.

    If <hCursor> is NULL, the cursor is removed from the screen.

    <hCursor>
        Identifes the cursor resource. The resource must have been loaded
        previously by using the %LoadCursor% function.

    The return value identifies the cursor resource that defines the previous
    cursor shape. It is NULL if there is no previous shape.

    The cursor is a shared resource. A window that uses the cursor should set
    the shape only when the cursor is in its client area or when it is capturing
    all mouse input. In systems without a mouse, the window should restore the
    previous cursor shape before the cursor leaves the client area or before the
    window relinquishes control to another window.

    Any application that needs to change the shape of the cursor while it is in
    a window must make sure the class cursor for the given window's class is set
    to NULL. If the class cursor is not NULL, Windows restores the previous
    shape each time the mouse is moved.
--*/

ULONG FASTCALL WU32SetCursor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(SETCURSOR16), parg16);

    ul = GETHCURSOR16(SetCursor(
        HCURSOR32(parg16->f1)
    ));


    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void SetCursorPos(<X>, <Y>)
    int <X>;
    int <Y>;

    The %SetCursorPos% function moves the cursor to the screen coordinates given
    by the <X> and <Y> parameters. If the new coordinates are not within the
    screen rectangle set by the most recent %ClipCursor% function, Windows
    automatically adjusts the coordinates so that the cursor stays within the
    rectangle.

    <X>
        Specifies the new x-coordinate (in screen coordinates) of the cursor.

    <Y>
        Specifies the new <y>-coordinate (in screen coordinates) of the
        cursor.

    This function does not return a value.

    The cursor is a shared resource. A window should move the cursor only when
    the cursor is in its client area.
--*/

ULONG FASTCALL WU32SetCursorPos(PVDMFRAME pFrame)
{
    register PSETCURSORPOS16 parg16;

    GETARGPTR(pFrame, sizeof(SETCURSORPOS16), parg16);

    SetCursorPos(
    INT32(parg16->f1),
    INT32(parg16->f2)
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    int ShowCursor(<fShow>)
    BOOL <fShow>;

    The %ShowCursor% function shows or hides the cursor. When the %ShowCursor%
    function is called, an internal display counter is incremented by one if the
    <fShow> parameter is TRUE, or decremented by one if the <fShow> parameter is
    FALSE. If the internal display counter is greater then or equal to zero, the
    cursor is displayed. If the counter is less then zero, the cursor is
    hidden. Calls to the %ShowCursor% function are accumulative: for each call
    to hide the cursor, a corresponding call must be made to show the cursor.

    <fShow>
        Specifies whether the display count is to be increased or decreased. The
        display count is increased if fShow is TRUE. Otherwise, it is
        decreased.

    The return value specifies the new display count.

    When Windows is first started, the display count is zero if a mouse is
    installed or -1 if no mouse is installed.

    The cursor is a shared resource. A window that hides the cursor should show
    the cursor before the cursor leaves its client area, or before the window
    relinquishes control to another window.
--*/

ULONG FASTCALL WU32ShowCursor(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSHOWCURSOR16 parg16;

    GETARGPTR(pFrame, sizeof(SHOWCURSOR16), parg16);

    ul = GETINT16(ShowCursor(
    BOOL32(parg16->f1)
    ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


//**************************************************************************
// This handles both LoadIcon and LoadCursor
//
//**************************************************************************

ULONG FASTCALL WU32LoadCursor(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ psz2;
    LPBYTE pResData = NULL;
    register PLOADCURSOR16 parg16;
    BOOL fIsCursor;
    HAND16 hInst16;
    HAND16 hRes16;

    LPWSTR lpUniName_CursorIcon;

    GETARGPTR(pFrame, sizeof(LOADCURSOR16), parg16);
    GETPSZIDPTR(parg16->f2, psz2);
    GETMISCPTR (parg16->f3, pResData);

    fIsCursor = ((WORD)parg16->f7  == (WORD)RT_CURSOR);
    hInst16 = FETCHWORD(parg16->f1);
    hRes16 = parg16->f5;

    if (HIWORD(psz2) != (WORD) NULL) {
        if (!(MBToWCS(psz2, -1, &lpUniName_CursorIcon, -1, TRUE))) {
            FREEMISCPTR(pResData);
            FREEPSZIDPTR(psz2);
            FREEARGPTR(parg16);
            RETURN(ul);
        }
    }
    else {
        lpUniName_CursorIcon = (LPWSTR)psz2;
    }

    ul = (ULONG) (pfnOut.pfnServerLoadCreateCursorIcon)(HINSTRES32(hInst16),
                                    (LPTSTR) NULL,  // pszModName unused by user32
                                    parg16->f6,
                                    (LPCTSTR) lpUniName_CursorIcon,
                                    parg16->f4,
                                    pResData,
                                    (LPTSTR) parg16->f7,
                                    0);

    if (ul)
        ul = SetupResCursorIconAlias(hInst16, (HAND32)ul,
                                     psz2, hRes16,
                                     fIsCursor ? HANDLE_TYPE_CURSOR : HANDLE_TYPE_ICON);



    if (HIWORD(psz2) != (WORD) NULL) {
        LocalFree (lpUniName_CursorIcon);
    }

    FREEMISCPTR(pResData);
    FREEPSZIDPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}
