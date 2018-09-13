/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WUSER.C
 *  WOW32 16-bit User API support
 *
 *  History:
 *  Created 07-Mar-1991 by Jeff Parsons (jeffpar)
--*/


#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop


MODNAME(wuser.c);

extern HANDLE hmodWOW32;


/*++
    HDC BeginPaint(<hwnd>, <lpPaint>)
    HWND <hwnd>;
    LPPAINTSTRUCT <lpPaint>;

    The %BeginPaint% function prepares the given window for painting and fills
    the paint structure pointed to by the <lpPaint> parameter with information
    about the painting.

    The paint structure contains a handle to the device context for the window,
    a %RECT% structure that contains the smallest rectangle that completely
    encloses the update region, and a flag that specifies whether or not the
    background has been erased.

    The %BeginPaint% function automatically sets the clipping region of the
    device context to exclude any area outside the update region. The update
    region is set by the %InvalidateRect% or %InvalidateRgn% functions and by
    the system after sizing, moving, creating, scrolling, or any other operation
    that affects the client area. If the update region is marked for erasing,
    %BeginPaint% sends a WM_ERASEBKGND message to the window.

    An application should not call the %BeginPaint% function except in response
    to a WM_PAINT message. Each %BeginPaint% call must have a matching call to
    the %EndPaint% function.

    <hwnd>
        Identifies the window to be repainted.

    <lpPaint>
        Points to the %PAINTSTRUCT% structure that is to receive painting
        information, such as the device context for the window and the update
        rectangle.

    The return value identifies the device context for the specified window.

    If the caret is in the area to be painted, the %BeginPaint% function
    automatically hides the caret to prevent it from being erased.
--*/

ULONG FASTCALL WU32BeginPaint(PVDMFRAME pFrame)
{
    ULONG ul;
    PAINTSTRUCT t2;
    register PBEGINPAINT16 parg16;
    VPVOID  vpPaint;

    GETARGPTR(pFrame, sizeof(BEGINPAINT16), parg16);
    vpPaint = parg16->vpPaint;

    ul = GETHDC16(BeginPaint(
    HWND32(parg16->hwnd),
    &t2
    ));

    putpaintstruct16(vpPaint, &t2);
    W32FixPaintRect (vpPaint, &t2);

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HICON CreateIcon(<hInstance>, <nWidth>, <nHeight>, <nPlanes>,
        <nBitsPixel>, <lpANDbits>, <lpXORbits>)
    HANDLE <hInstance>;
    int <nWidth>;
    int <nHeight>;
    BYTE <nPlanes>;
    BYTE <nBitsPixel>;
    LPSTR <lpANDbits>;
    LPSTR <lpXORbits>;

    This function creates an icon that has specified width, height, colors, and
    bit patterns.

    <hInstance>
        Identifies an instance of the module creating the icon.

    <nWidth>
        Specifies the width in pixels of the icon.

    <nHeight>
        Specifies the height in pixels of the icon.

    <nPlanes>
        Specifies the number of planes in the XOR mask of the icon.

    <nBitsPixel>
        Specifies the number of bits per pixel in the XOR mask of the icon.

    <lpANDbits>
        Points to an array of bytes that contains the bit values for the AND
        mask of the icon. This array must specify a monochrome mask.

    <lpXORbits>
        Points to an array of bytes that contains the bit values for the XOR
        mask of the icon. This can be the bits of a monochrome or
        device-dependent color bitmap.

    The return value identifies an icon if the function is successful.
    Otherwise, it is NULL.
--*/

ULONG FASTCALL WU32CreateIcon(PVDMFRAME pFrame)
{
    ULONG   ul;
    register PCREATEICON16 parg16;
    int     nWidth;
    int     nHeight;
    BYTE    nPlanes;
    BYTE    nBitsPixel;
    DWORD   nBytesAND;
    DWORD   nBytesXOR;
    LPBYTE  lpBitsAND;
    LPBYTE  lpBitsXOR;
    int     ScanLen16;

    HANDLE  h32;
    HAND16  h16;
    HAND16  hInst16;

    GETARGPTR(pFrame, sizeof(CREATEICON16), parg16);

    hInst16    = parg16->f1;
    nWidth     = INT32(parg16->f2);
    nHeight    = INT32(parg16->f3);

    /*
    ** Convert the AND mask bits
    */
    nPlanes    = 1;     /* MONOCHROME BITMAP */
    nBitsPixel = 1;     /* MONOCHROME BITMAP */
    ScanLen16 = (((nWidth*nBitsPixel)+15)/16) * 2 ;  // bytes/scan in 16 bit world
    nBytesAND = ScanLen16*nHeight*nPlanes;

    GETVDMPTR(parg16->f6, nBytesAND, lpBitsAND);


    /*
    ** Convert the XOR mask bits
    */
    nPlanes    = BYTE32(parg16->f4);
    nBitsPixel = BYTE32(parg16->f5);

    ScanLen16 = (((nWidth*nBitsPixel)+15)/16) * 2 ;  // bytes/scan in 16 bit world
    nBytesXOR = ScanLen16*nHeight*nPlanes;

    GETVDMPTR(parg16->f7, nBytesXOR, lpBitsXOR);


    h32 = (HANDLE)CreateIcon(HMODINST32(hInst16),
                 nWidth,
                 nHeight,
                 nPlanes,
                 nBitsPixel,
                 lpBitsAND,
                 lpBitsXOR);

    if (h32) {
        h16 = (HAND16)W32Create16BitCursorIcon(hInst16,
                            nWidth/2, nHeight/2,
                            nWidth, nHeight, nPlanes, nBitsPixel,
                            lpBitsAND, lpBitsXOR,
                            nBytesAND, nBytesXOR);

        ul  = SetupCursorIconAlias(hInst16, h32, h16,
                                   HANDLE_TYPE_ICON, NULL, (WORD)NULL);
    } else {
        ul = 0;
    }

    FREEPSZPTR(lpBitsAND);
    FREEPSZPTR(lpBitsXOR);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL DestroyIcon(<hIcon>)
    HICON <hIcon>;

    This function destroys an icon that was previously created by the
    %CreateIcon% function and frees any memory that the icon occupied. It should
    not be used to destroy any icon that was not created with the %CreateIcon%
    function.

    <hIcon>
        Identifies the icon to be destroyed. The icon must not be in current
        use.

    The return value is TRUE if the function was successful. It is FALSE if
    the function failed.
--*/

ULONG FASTCALL WU32DestroyIcon(PVDMFRAME pFrame)
{
    ULONG ul;
    register PDESTROYICON16 parg16;

    GETARGPTR(pFrame, sizeof(DESTROYICON16), parg16);

    if (ul = GETBOOL16(DestroyIcon(HICON32(parg16->f1))))
        FREEHICON16(parg16->f1);

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32DragDetect(PVDMFRAME pFrame)
{
    ULONG ul;
    POINT pt;
    register PDRAGDETECT16 parg16;

    GETARGPTR(pFrame, sizeof(*parg16), parg16);
    COPYPOINT16(parg16->pt, pt);

    ul = (ULONG) DragDetect(
                     HWND32(parg16->hwnd),
                     pt
                     );

    FREEARGPTR(parg16);
    RETURN(ul);
}

/*++
    void DrawFocusRect(<hDC>, <lpRect>)
    HDC <hDC>;
    LPRECT <lpRect>;

    The %DrawFocusRect% function draws a rectangle in the style used to indicate
    focus.

    <hDC>
        Identifies the device context.

    <lpRect>
        Points to a %RECT% structure that specifies the
        coordinates of the rectangle to be drawn.

    This function does not return a value.

    Since this is an XOR function, calling this function a second time with the
    same rectangle removes the rectangle from the display.

    The rectangle drawn by this function cannot be scrolled. To scroll an area
    containing a rectangle drawn by this function, call %DrawFocusRect% to
    remove the rectangle from the display, scroll the area, and then call
    %DrawFocusRect% to draw the rectangle in the new position.
--*/

ULONG FASTCALL WU32DrawFocusRect(PVDMFRAME pFrame)
{
    RECT t2;
    register PDRAWFOCUSRECT16 parg16;

    GETARGPTR(pFrame, sizeof(DRAWFOCUSRECT16), parg16);

    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    DrawFocusRect(
    HDC32(parg16->f1),
    &t2
    );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    int DrawText(<hDC>, <lpString>, <nCount>, <lpRect>, <wFormat>)
    HDC <hDC>;
    LPSTR <lpString>;
    int <nCount>;
    LPRECT <lpRect>;
    WORD <wFormat>;

    The %DrawText% function draws formatted text in the rectangle specified by
    the <lpRect> parameter. It formats text by expanding tabs into appropriate
    spaces, justifying text to the left, right, or center of the given
    rectangle, and breaking text into lines that fit within the given
    rectangle. The type of formatting is specified by the <wFormat> parameter.

    The %DrawText% function uses the device context's selected font, text color,
    and background color to draw the text. Unless the DT_NOCLIP format is used,
    %DrawText% clips the text so that the text does not appear outside the given
    rectangle. All formatting is assumed to have multiple lines unless the
    DT_SINGLELINE format is given.

    <hDC>
        Identifies the device context.

    <lpString>
        Points to the string to be drawn. If the <nCount> parameter is -1, the
        string must be null-terminated.

    <nCount>
        Specifies the number of bytes in the string. If <nCount> is -1,
        then <lpString> is assumed to be a long pointer to a null-terminated
        string and %DrawText% computes the character count automatically.

    <lpRect>
        Points to a %RECT% structure that contains the rectangle
        (in logical coordinates) in which the text is to be formatted.

    <wFormat>
        Specifies the method of formatting the text. It can be any
        combination of the following values:

    DT_BOTTOM
        Specifies bottom-justified text. This value must be combined with
        DT_SINGLELINE.

    DT_CALCRECT
        Determines the width and height of the rectangle. If there are multiple
        lines of text, %DrawText% will use the width of the rectangle pointed to
        by the <lpRect> parameter and extend the base of the rectangle to bound
        the last line of text. If there is only one line of text, %DrawText%
        will modify the right side of the rectangle so that it bounds the last
        character in the line. In either case, %DrawText% returns the height of
        the formatted text but does not draw the text.

    DT_CENTER
        Centers text horizontally.

    DT_EXPANDTABS
        Expands tab characters. The default number of characters per tab is
        eight.

    DT_EXTERNALLEADING
        Includes the font external leading in line height. Normally, external
        leading is not included in the height of a line of text.

    DT_LEFT
        Aligns text flush-left.

    DT_NOCLIP
        Draws without clipping. %DrawText% is somewhat faster when DT_NOCLIP is
        used.

    DT_NOPREFIX
        Turns off processing of prefix characters. Normally, %DrawText%
        interprets the mnemonic-prefix character & as a directive to
        underscore the character that follows, and the mnemonic-prefix
        characters && as a directive to print a single &. By specifying
        DT_NOPREFIX, this processing is turned off.

    DT_RIGHT
        Aligns text flush-right.

    DT_SINGLELINE
        Specifies single line only. Carriage returns and linefeeds do not break
        the line.

    DT_TABSTOP
        Sets tab stops. The high-order byte of the <wFormat> parameter is the
        number of characters for each tab. The default number of characters per
        tab is eight.

    DT_TOP
        Specifies top-justified text (single line only).

    DT_VCENTER
        Specifies vertically centered text (single line only).

    DT_WORDBREAK
        Specifies word breaking. Lines are automatically broken between words if
        a word would extend past the edge of the rectangle specified by the
        <lpRect> parameter. A carriage return/line sequence will also break the
        line.

        Note that the DT_CALCRECT, DT_EXTERNALLEADING, DT_INTERNAL, DT_NOCLIP,
        and DT_NOPREFIX values cannot be used with the DT_TABSTOP value:

    The return value specifies the height of the text.

    If the selected font is too large for the specified rectangle, the
    %DrawText% function does not attempt to substitute a smaller font.
--*/

ULONG FASTCALL WU32DrawText(PVDMFRAME pFrame)
{
    ULONG ul;
    PSTR pstr2;
    RECT t4;
    register PDRAWTEXT16 parg16;

    GETARGPTR(pFrame, sizeof(DRAWTEXT16), parg16);
    GETVARSTRPTR(parg16->vpString, INT32(parg16->nCount), pstr2);

    WOW32VERIFY(GETRECT16(parg16->vpRect, &t4));

    ul = GETINT16(DrawText(
                      HDC32(parg16->hdc),
                      pstr2,
                      INT32(parg16->nCount),
                      &t4,
                      WORD32(parg16->wFormat)
                      ));

    PUTRECT16(parg16->vpRect, &t4);

    FREESTRPTR(pstr2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void EndPaint(<hwnd>, <lpPaint>)
    HWND <hwnd>;
    LPPAINTSTRUCT <lpPaint>;

    The %EndPaint% function marks the end of painting in the given window. The
    %EndPaint% function is required for each call to the %BeginPaint% function,
    but only after painting is complete.

    <hwnd>
        Identifies the window that is repainted.

    <lpPaint>
        Points to a %PAINTSTRUCT% structure that contains the painting
        information retrieved by the %BeginPaint% function.

    This function does not return a value.

    If the caret was hidden by the %BeginPaint% function, %EndPaint% restores
    the caret to the screen.
--*/

ULONG FASTCALL WU32EndPaint(PVDMFRAME pFrame)
{
    PAINTSTRUCT t2;
    register PENDPAINT16 parg16;

    GETARGPTR(pFrame, sizeof(ENDPAINT16), parg16);
    getpaintstruct16(parg16->vpPaint, &t2);

    EndPaint(
        HWND32(parg16->hwnd),
        &t2
        );

    FREEARGPTR(parg16);
    RETURN(0);
}


#define MAX_WIN16_PROP_TEXT 256     /* Taken from Win 3.1 - winprops.c */

static VPVOID   vpEnumPropsProc;
static VPVOID   vpString;

INT W32EnumPropsFunc( HWND hwnd, LPSTR lpString, HANDLE hData )
{
    PARM16 Parm16;
    LONG lReturn;
    VPVOID vp;

    if ( HIWORD(lpString) == 0 ) {
        vp = (DWORD)lpString;
    } else {
        INT cb;

        vp = vpString;
        cb = strlen(lpString)+1;
        if ( cb > MAX_WIN16_PROP_TEXT-1 ) {
            cb = MAX_WIN16_PROP_TEXT-1;
        }
        putstr16(vpString, lpString, cb);
    }

    Parm16.EnumPropsProc.hwnd = GETHWND16(hwnd);
    Parm16.EnumPropsProc.vpString = vp;
    Parm16.EnumPropsProc.hData = GETHANDLE16(hData);

    CallBack16(RET_ENUMPROPSPROC, &Parm16, vpEnumPropsProc, (PVPVOID)&lReturn);

    return (SHORT)lReturn;
}

/*++
    int EnumProps(<hwnd>, <lpEnumFunc>)
    HWND <hwnd>;
    FARPROC <lpEnumFunc>;

    The %EnumProps% function enumerates all entries in the property list of the
    specified window. It enumerates the entries by passing them, one by one, to
    the callback function specified by <lpEnumFunc>. %EnumProps% continues until
    the last entry is enumerated or the callback function returns zero.

    <hwnd>
        Identifies the window whose property list is to be enumerated.

    <lpEnumFunc>
        Specifies the procedure-instance address of the callback function.
        See the following Comments section for details.

    The return value specifies the last value returned by the callback function.
    It is -1 if the function did not find a property for enumeration.

    An application can remove only those properties which it has added. It
    should not remove properties added by other applications or by Windows
    itself.

    The following restrictions apply to the callback function:

    1   The callback function must not yield control or do anything that might
        yield control to other tasks.

    2   The callback function can call the %RemoveProp% function. However, the
        %RemoveProp% function can remove only the property passed to the
        callback function through the callback function's parameters.

    3   A callback function should not attempt to add properties.

    The address passed in the <lpEnumFunc> parameter must be created by using
    the %MakeProcInstance% function.

    Fixed Data Segments:

    The callback function must use the Pascal calling convention and must be
    declared %FAR%. In applications and dynamic libraries with fixed data
    segments and in dynamic libraries with moveable data segments that do not
    contain a stack, the callback function must have the form shown below.

    Callback Function:

    int  FAR PASCAL <EnumFunc>(<hwnd>, <lpString>, <hData>)
    HWND <hwnd>;
    LPSTR <lpString>;
    HANDLE <hData>;

    <EnumFunc> is a placeholder for the application-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the application's module-definition file.

    <hwnd>
        Identifies a handle to the window that contains the property list.

    <lpString>
        Points to the null-terminated string associated with the data handle
        when the application called the%SetProp% function to set the property.
        If the application passed an atom instead of a string to the %SetProp%
        function, the<lpString> parameter contains the atom in its low-order
        word, and the high-order word is zero.

    <hData>
        Identifies the data handle.

    The callback function can carry out any desired task. It must return a
    nonzero value to continue enumeration, or a zero value to stop it.

    Moveable Data Segments:

    The callback function must use the Pascal calling convention and must be
    declared %FAR%. In applications with moveable data segments and in dynamic
    libraries whose moveable data segments also contain a stack, the callback
    function must have the form shown below.

    Callback Function:

    int  FAR PASCAL <EnumFunc>(<hwnd>, <nDummy>, <pString>, <hData>)
    HWND <hwnd>;
    WORD <nDummy>;
    PSTR <pString>;
    HANDLE <hData>;

    <EnumFunc> is a placeholder for the application-supplied function name. The
    actual name must be exported by including it in an %EXPORTS% statement in
    the application's module-definition file.

    <hwnd>
        Identifies a handle to the window that contains the property list.

    <nDummy>
        Specifies a dummy parameter.

    <pString>
        Points to the null-terminated string associated with the data handle
        when the application called the %SetProp% function to set the property.
        If the application passed an atom instead of a string to the %SetProp%
        function, the <pString> parameter contains the atom.

    <hData>
        Identifies the data handle.

    The callback function can carry out any desired task. It should return a
    nonzero value to continue enumeration, or a zero value to stop it.

    The alternate form above is required since movement of the data will
    invalidate any long pointer to a variable on the stack, such as the
    <lpString> parameter. The data segment typically moves if the callback
    function allocates more space in the local heap than is currently
    available.
--*/

ULONG FASTCALL WU32EnumProps(PVDMFRAME pFrame)
{
    ULONG ul;
    HWND    hwnd;
    register PENUMPROPS16 parg16;

    GETARGPTR(pFrame, sizeof(ENUMPROPS16), parg16);

    hwnd            = HWND32(parg16->f1);
    vpEnumPropsProc =        parg16->f2;

    vpString = malloc16(MAX_WIN16_PROP_TEXT);
    // 16-bit memory may have moved - invalidate flat pointers
    FREEARGPTR(parg16);
    FREEVDMPTR(pFrame);

    if (vpString) {
        ul = GETINT16(EnumProps(hwnd,(PROPENUMPROC)W32EnumPropsFunc));
        free16(vpString);

    } else {
        ul = (ULONG)-1;
    }

    RETURN(ul);
}



/*++
    int FillWindow(<hWndParent>, <hWnd>, <hDC>, <hBrush>)
    HWND <hWndParent>;
    HWND  <hWnd>;
    HDC <hDC>;
    HBRUSH <hBrush>;

    The %FillWindow% function paints a given window by using the specified
    brush.

    <hWndParent>
        Identifies the parent of the window to be painted.

    <hWnd>
        Identifies the window to be painted.

    <hDC>
        Identifies the device context.

    <hBrush>
        Identifies the brush used to fill the rectangle.

--*/

ULONG FASTCALL WU32FillWindow(PVDMFRAME pFrame)
{
    register PFILLWINDOW16 parg16;

    GETARGPTR(pFrame, sizeof(FILLWINDOW16), parg16);

    (pfnOut.pfnFillWindow)(
        HWND32(parg16->f1),
        HWND32(parg16->f2),
        HDC32(parg16->f3),
        HBRUSH32(parg16->f4)
        );

    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    int FillRect(<hDC>, <lpRect>, <hBrush>)
    HDC <hDC>;
    LPRECT <lpRect>;
    HBRUSH <hBrush>;

    The %FillRect% function fills a given rectangle by using the specified
    brush. The %FillRect% function fills the complete rectangle, including the
    left and top borders, but does not fill the right and bottom borders.

    <hDC>
        Identifies the device context.

    <lpRect>
        Points to a %RECT% structure that contains the logical
        coordinates of the rectangle to be filled.

    <hBrush>
        Identifies the brush used to fill the rectangle.

    Although the %FillRect% function return type is an integer, the return value
    is not used and has no meaning.

    The brush must have been created previously by using either the
    %CreateHatchBrush%, %CreatePatternBrush%, or %CreateSolidBrush% function, or
    retrieved using the %GetStockObject% function.

    When filling the specified rectangle, the %FillRect% function does not
    include the rectangle's right and bottom sides. GDI fills a rectangle up to,
    but does not include, the right column and bottom row, regardless of the
    current mapping mode.

    %FillRect% compares the values of the %top%, %bottom%, %left%, and %right%
    members of the specified rectangle. If %bottom% is less than or equal to
    %top%, or if %right% is less than or equal to %left%, the rectangle is not
    drawn.
--*/

ULONG FASTCALL WU32FillRect(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PFILLRECT16 parg16;

    GETARGPTR(pFrame, sizeof(FILLRECT16), parg16);

    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    ul = GETINT16(FillRect(
                      HDC32(parg16->f1),
                      &t2,
                      HBRUSH32(parg16->f3)
                      ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int FrameRect(<hDC>, <lpRect>, <hBrush>)
    HDC <hDC>;
    LPRECT <lpRect>;
    HBRUSH <hBrush>;

    The %FrameRect% function draws a border around the rectangle specified by
    the <lpRect> parameter. The %FrameRect% function uses the given brush to
    draw the border. The width and height of the border is always one logical
    unit.

    <hDC>
        Identifies the device context of the window.

    <lpRect>
        Points to a %RECT% structure that contains the logical
        coordinates of the upper-left and lower-right corners of the rectangle.

    <hBrush>
        Identifies the brush to be used for framing the rectangle.

    Although the return value type is integer, its contents should be ignored.

    The brush identified by the <hBrush> parameter must have been created
    previously by using the %CreateHatchBrush%, %CreatePatternBrush%, or
    %CreateSolidBrush% function.

    If the %bottom% member is less than or equal to the %top% member, or if the
    %right% member is less than or equal to the %left% member, the rectangle is
    not drawn.
--*/

ULONG FASTCALL WU32FrameRect(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PFRAMERECT16 parg16;

    GETARGPTR(pFrame, sizeof(FRAMERECT16), parg16);

    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    ul = GETINT16(FrameRect(
                      HDC32(parg16->f1),
                      &t2,
                      HBRUSH32(parg16->f3)
                      ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HDC GetDC(<hwnd>)
    HWND <hwnd>;

    The %GetDC% function retrieves a handle to a display context for the client
    area of the given window. The display context can be used in subsequent GDI
    functions to draw in the client area.

    The %GetDC% function retrieves a common, class, or private display context
    depending on the class style specified for the given window. For common
    display contexts, %GetDC% assigns default attributes to the context each
    time it is retrieved. For class and private contexts, %GetDC% leaves the
    previously assigned attributes unchanged.

    <hwnd>
        Identifies the window whose display context is to be retrieved.

    The return value identifies the display context for the given window's
    client area if the function is successful. Otherwise, it is NULL.

    After painting with a common display context, the %ReleaseDC% function must
    be called to release the context. Class and private display contexts do not
    have to be released. Since only five common display contexts are available
    at any given time, failure to release a display context can prevent other
    applications from accessing a display context.
--*/

ULONG FASTCALL WU32GetDC(PVDMFRAME pFrame)
{
    ULONG ul;
    register PGETDC16 parg16;
    HAND16 htask16 = pFrame->wTDB;

    GETARGPTR(pFrame, sizeof(GETDC16), parg16);

    if (CACHENOTEMPTY()) {
        ReleaseCachedDCs(htask16, parg16->f1, 0, 0, SRCHDC_TASK16_HWND16);
    }

    CURRENTPTD()->ulLastDesktophDC = 0;
    
    ul = GETHDC16(GetDC(
                      HWND32(parg16->f1)
                      ));

    if (ul) {
// Some apps such as MSWORKS and MS PUBLISHER use some wizard code that accepts
// a hDC or a hWnd as a parameter and attempt to figure out what type of handle
// it is by using the IsWindow() call. Since both handles come from different
// handle spaces they may end up the same value and this wizard code will end
// up writing to the DC for a random window. By ORing in a 1 we ensure that the
// handle types will never share the same value since all hWnds are even. Note
// that this hack is also made in WG32CreateCompatibleDC()
//
// Note that there are some apps that use the lower 2 bits of the hDC for their
// own purposes.
        if (CURRENTPTD()->dwWOWCompatFlags & WOWCF_UNIQUEHDCHWND) {
            ul = ul | 1;
        } else if ((CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_FIXDCFONT4MENUSIZE) &&
                   (parg16->f1 == 0)) {
// WP tutorial assumes that the font selected in the hDC for desktop window
// (ie, result of GetDC(NULL)) is the same font as the font selected for 
// drawing the menu. Unfortunetly in SUR this is not true as the user can
// select any font for the menu. So we remember the hDC returned for GetDC(0)
// and check for it in GetTextExtentPoint. If the app does try to use it we
// find the hDC for the current menu window and substitute that. When the app
// does another GetDC or ReleaseDC we forget the hDC returned for the original
// GetDC(0).
            CURRENTPTD()->ulLastDesktophDC = ul;
        }



        StoreDC(htask16, parg16->f1, (HAND16)ul);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    void GetScrollRange(<hwnd>, <nBar>, <lpMinPos>, <lpMaxPos>)
    HWND <hwnd>;
    int <nBar>;
    LPINT <lpMinPos>;
    LPINT <lpMaxPos>;

    The %GetScrollRange% function copies the current minimum and maximum
    scroll-bar positions for the given scroll bar to the locations specified by
    the <lpMinPos> and <lpMaxPos> parameters. If the given window does not have
    standard scroll bars or is not a scroll-bar control, then the
    %GetScrollRange% function copies zero to <lpMinPos> and <lpMaxPos>.

    <hwnd>
        Identifies a window that has standard scroll bars or a scroll-bar
        control, depending on the value of the nBar parameter.

    <nBar>
        Specifies an integer value that identifies which scroll bar to
        retrieve. It can be one of the following values:

    SB_CTL
        Retrieves the position of a scroll-bar control; in this case, the hwnd
        parameter must be the handle of a scroll-bar control.

    SB_HORZ
        Retrieves the position of a window's horizontal scroll bar.

    SB_VERT
        Retrieves the position of a window's vertical scroll bar.

    <lpMinPos>
        Points to the integer variable that is to receive the minimum
        position.

    <lpMaxPos>
        Points to the integer variable that is to receive the maximum
        position.

    This function does not return a value.

    The default range for a standard scroll bar is 0 to 100. The default range
    for a scroll-bar control is empty (both values are zero).
--*/

ULONG FASTCALL WU32GetScrollRange(PVDMFRAME pFrame)
{
    INT t3;
    INT t4;
    register PGETSCROLLRANGE16 parg16;

    GETARGPTR(pFrame, sizeof(GETSCROLLRANGE16), parg16);

    GetScrollRange(
        HWND32(parg16->f1),
        INT32(parg16->f2),
        &t3,
        &t4
        );

    PUTINT16(parg16->f3, t3);
    PUTINT16(parg16->f4, t4);
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    ULONG GetTimerResolution(VOID)


    This function has no parameters.

    The Win 3.0 & 3.1 code just return 1000.

    Contacts on this: NeilK DarrinM

    The return value is always 1000.

--*/

ULONG FASTCALL WU32GetTimerResolution(PVDMFRAME pFrame)
{

    UNREFERENCED_PARAMETER(pFrame);

    RETURN(1000L);
}


/*++
    BOOL GetUpdateRect(<hwnd>, <lpRect>, <bErase>)
    HWND <hwnd>;
    LPRECT <lpRect>;
    BOOL <bErase>;

    The %GetUpdateRect% function retrieves the coordinates of the smallest
    rectangle that completely encloses the update region of the given window. If
    the window was created with the CS_OWNDC style and the mapping mode is not
    MM_TEXT, the %GetUpdateRect% function gives the rectangle in logical
    coordinates. Otherwise, %GetUpdateRect% gives the rectangle in client
    coordinates. If there is no update region, %GetUpdateRect% makes the
    rectangle empty (sets all coordinates to zero).

    The <bErase> parameter specifies whether %GetUpdateRect% should erase the
    background of the update region. If <bErase> is TRUE and the update region
    is not empty, the background is erased. To erase the background,
    %GetUpdateRect% sends a WM_ERASEBKGND message to the given window.

    <hwnd>
        Identifies the window whose update region is to be retrieved.

    <lpRect>
        Points to the %RECT% structure that is to receive the
        client coordinates of the enclosing rectangle.

    <bErase>
        Specifies whether the background in the update region is to be
        erased.

    The return value specifies the status of the update region of the given
    window. It is TRUE if the update region is not empty. Otherwise, it is
    FALSE.

    The update rectangle retrieved by the %BeginPaint% function is identical to
    that retrieved by the %GetUpdateRect% function.

    %BeginPaint% automatically validates the update region, so any call to
    %GetUpdateRect% made immediately after the %BeginPaint% call retrieves an
    empty update region.
--*/

ULONG FASTCALL WU32GetUpdateRect(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t2;
    register PGETUPDATERECT16 parg16;

    GETARGPTR(pFrame, sizeof(GETUPDATERECT16), parg16);

    ul = GETBOOL16(GetUpdateRect(
                       HWND32(parg16->f1),
                       &t2,
                       BOOL32(parg16->f3)
                       ));


    PUTRECT16(parg16->f2, &t2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32GlobalAddAtom(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz1;
    UINT dw1;
    register PGLOBALADDATOM16 parg16;

    GETARGPTR(pFrame, sizeof(GLOBALADDATOM16), parg16);

    dw1 = UINT32(parg16->f1);

    if (!HIWORD(dw1)) {

        //
        // If the hiword is zero, it's not a pointer.
        // Instead, it's an integer and we either return
        // the integer passed (if it's not a valid atom
        // value), or zero (if it is a valid atom value).
        //

        if (!dw1 || dw1 >= 0xc000) {
            ul = 0;
        } else {
            ul = dw1;
        }

    } else {

        GETPSZPTR(parg16->f1, psz1);

        ul = GETATOM16(GlobalAddAtom(
                 psz1
                 ));

        FREEPSZPTR(psz1);

    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32GlobalDeleteAtom(PVDMFRAME pFrame)
{

    // Envoy viewer (part of PerfectOffice) has a bug in GlobalDeleteAtom
    // where it expects the wrong return value (the app thought 0 was
    // failure while its for success). This causes the app to go in an
    // infinite loop trying to delete a global object. This app works on
    // Win3.1 because Win3.1 returns some garbage in AX if the atom is
    // already deleted which takes this app out of the loop. On Win95 and
    // NT3.51 that is not the case and 0 is always returned. The following
    // comaptibility fix mimics the win3.1 behavior for this app.

    ULONG ul;
    static USHORT envoyHandle16=0;
    static BOOL   fFoundEnvoyAtom = FALSE;
    BOOL    IsEnvoy;
    CHAR  envoyString [32];
    register PGLOBALDELETEATOM16 parg16;

    GETARGPTR(pFrame, sizeof(GLOBALDELETEATOM16), parg16);

    IsEnvoy = (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_GLOBALDELETEATOM);
    if (IsEnvoy){
        if (!fFoundEnvoyAtom && GlobalGetAtomName (ATOM32(parg16->f1),
                               envoyString,
                               32) &&
                !WOW32_stricmp (envoyString, "SomeEnvoyViewerIsRunning")) {
            envoyHandle16 = parg16->f1;
        }

    }
    ul = GETATOM16(GlobalDeleteAtom(
    ATOM32(parg16->f1)
    ));

    if (IsEnvoy){
        if (envoyHandle16 && !fFoundEnvoyAtom) {
            fFoundEnvoyAtom = TRUE;
        }
        else if (fFoundEnvoyAtom) {
            if (envoyHandle16 == parg16->f1) {
                envoyHandle16 = 0;
                fFoundEnvoyAtom = FALSE;
                ul = parg16->f1;
            }
        }
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32GlobalGetAtomName(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ psz2;
    register PGLOBALGETATOMNAME16 parg16;

    GETARGPTR(pFrame, sizeof(GLOBALGETATOMNAME16), parg16);
    ALLOCVDMPTR(parg16->f2, parg16->f3, psz2);

    if (parg16->f1) {
        ul = GETWORD16(GlobalGetAtomName(ATOM32(parg16->f1),
                                         psz2,
                                         INT32(parg16->f3)));

        FLUSHVDMPTR(parg16->f2, strlen(psz2)+1, psz2);
    }
    else {
        *psz2 = '\0';
    }


    FREEVDMPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL GrayString(<hDC>, <hBrush>, <lpOutputFunc>, <lpData>, <nCount>, <X>,
        <Y>, <nWidth>, <nHeight>)
    HDC <hDC>;
    HBRUSH <hBrush>;
    FARPROC <lpOutputFunc>;
    DWORD <lpData>;
    int <nCount>;
    int <X>;
    int <Y>;
    int <nWidth>;
    int <nHeight>;

    The %GrayString% function draws gray text at the given location. The
    %GrayString% function draws gray text by writing the text in a memory
    bitmap, graying the bitmap, and then copying the bitmap to the display. The
    function grays the text regardless of the selected brush and
    background. %GrayString% uses the font currently selected for the device
    context specified by the <hDC> parameter.

    If the <lpOutputFunc> parameter is NULL, GDI uses the %TextOut% function,
    and the <lpData> parameter is assumed to be a long pointer to the character
    string to be output. If the characters to be output cannot be handled by
    %TextOut% (for example, the string is stored as a bitmap), the application
    must supply its own output function.

    <hDC>
        Identifies the device context.

    <hBrush>
        Identifies the brush to be used for graying.

    <lpOutputFunc>
        Is the procedure-instance address of the application-supplied
        function that will draw the string, or, if the %TextOut% function is to
        be used to draw the string, it is a NULL pointer. See the following
        Comments section for details.

    <lpData>
        Specifies a long pointer to data to be passed to the output
        function. If the <lpOutputFunc> parameter is NULL, <lpData> must be a
        long pointer to the string to be output.

    <nCount>
        Specifies the number of characters to be output. If the <nCount>
        parameter is zero, %GrayString% calculates the length of the string
        (assuming that <lpData> is a pointer to the string). If <nCount> is -1
        and the function pointed to by <lpOutputFunc> returns zero, the image is
        shown but not grayed.

    <X>
        Specifies the logical <x>-coordinate of the starting position of
        the rectangle that encloses the string.

    <Y>
        Specifies the logical <y>-coordinate of the starting position of
        the rectangle that encloses the string.

    <nWidth>
        Specifies the width (in logical units) of the rectangle that
        encloses the string. If the <nWidth> parameter is zero, %GrayString%
        calculates the width of the area, assuming <lpData> is a pointer to the
        string.

    <nHeight>
        Specifies the height (in logical units) of the rectangle that
        encloses the string. If the <nHeight> parameter is zero, %GrayString%
        calculates the height of the area, assuming <lpData> is a pointer to the
        string.


    The return value specifies the outcome of the function. It is TRUE if the
    string is drawn. A return value of FALSE means that either the %TextOut%
    function or the application-supplied output function returned FALSE, or
    there was insufficient memory to create a memory bitmap for graying.

    An application can draw grayed strings on devices that support a solid gray
    color, without calling the %GrayString% function. The system color
    COLOR_GRAYTEXT is the solid-gray system color used to draw disabled text.
    The application can call the %GetSysColor% function to retrieve the color
    value of COLOR_GRAYTEXT. If the color is other than zero (black), the
    application can call the %SetTextColor% to set the text color to the color
    value and then draw the string directly. If the retrieved color is black,
    the application must call %GrayString% to gray the text.

    The callback function must use the Pascal calling convention and must be
    declared %FAR%.

    Callback Function:

    BOOL FAR PASCAL <OutputFunc>(<hDC>, <lpData>, <nCount>)
    HDC <hDC>;
    DWORD <lpData>;
    int <nCount>;

    <OutputFunc> is a placeholder for the application-supplied callback function
    name. The actual name must be exported by including it in an %EXPORTS%
    statement in the application's module-definition file.

    <hDC>
        Identifies a memory device context with a bitmap of at least the width
        and height specified by the nWidth and nHeight parameters,
        respectively.

    <lpData>
        Points to the character string to be drawn.

    <nCount>
        Specifies the number of characters to be output.

    The return value must be TRUE to indicate success. Otherwise, it is FALSE.

    This output function (<OutputFunc>) must draw an image relative to the
    coordinates (0,0) rather than (<X,Y>). The address passed as the
    <lpOutputFunc> parameter must be created by using the %MakeProcInstance%
    function, and the output function name must be exported; it must be
    explicitly defined in an %EXPORTS% statement of the application's
    module-definition file.

    The MM_TEXT mapping mode must be selected before using this function.
--*/

BOOL W32GrayStringProc(HDC hDC,PGRAYSTRINGDATA pGray, int n) {
    INT iReturn;
    PARM16 Parm16;

    WOW32ASSERT(pGray);

    if (pGray->fResetLengthToZero)
        n = 0;

    LOGDEBUG(12,("    Graystring callback function, n = %d, hdc = %lx, %lx\n",n,hDC,pGray->dwUserParam));

    Parm16.GrayStringProc.n = (SHORT)n;
    Parm16.GrayStringProc.data = pGray->dwUserParam;
    pGray->hdc=Parm16.GrayStringProc.hdc = GETHDC16(hDC);
    CallBack16(RET_GRAYSTRINGPROC, &Parm16, pGray->vpfnGrayStringProc, (PVPVOID)&iReturn);

    LOGDEBUG(12,("    Graystring callback function returns %x\n",iReturn));
    return (BOOL)((SHORT)iReturn);
}

ULONG FASTCALL WU32GrayString(PVDMFRAME pFrame)
{
    ULONG          ul=0;
    PSZ            psz2;
    HDC            hdc;
    INT            n,wid,hgt;
    VPVOID         vpfn;
    VPVOID         vpstr;
    GRAYSTRINGDATA Gray;
    register PGRAYSTRING16 parg16;

    GETARGPTR(pFrame, sizeof(GRAYSTRING16), parg16);

    hdc=HDC32(parg16->f1);

    vpfn = DWORD32(parg16->f3);

    vpstr = DWORD32(parg16->f4);

    n=INT32(parg16->f5);

    wid=INT32(parg16->f8);
    hgt=INT32(parg16->f9);


    if ( HIWORD(vpfn) ) {       // SQLWin/repowin passes junk in low word

        Gray.fResetLengthToZero = FALSE;

        if( n==0 ) {

            n = 1;              // Prevent USER from doing strlen on &Gray below

            if ( HIWORD(vpstr) != 0 ) {  // Blow off small integers right away

                GETVDMPTR(vpstr, 0, psz2); // This might assert on mips, ignore it!

                if ( psz2 ) {
                    try {
                        n = strlen(psz2);
                        if (!n) {
                            n = 1;
                            Gray.fResetLengthToZero = TRUE;
                        }

                    } except( EXCEPTION_EXECUTE_HANDLER ) {
                    }
                }

                FREEVDMPTR( psz2 );
            }
        }

        if ( wid == 0 || hgt == 0) {
            if ( HIWORD(vpstr) != 0 ) {
                GETVDMPTR(vpstr, 0, psz2); // This might assert on mips, ignore it!

                if (psz2) {
                    SIZE size;

                    try {
                        GetTextExtentPointA(hdc, (LPCSTR)psz2, n, &size);
                        wid = size.cx;
                        hgt = size.cy;
                    } except (EXCEPTION_EXECUTE_HANDLER) {
                    }
                }

                FREEVDMPTR( psz2 );
            }
        }

        Gray.vpfnGrayStringProc = DWORD32(parg16->f3);
        Gray.dwUserParam        = vpstr;
        Gray.hdc = 0;

        LOGDEBUG(12,("    Graystring with callback %lx  n,w,h = %d,%d,%d\n",
                 vpstr,n,wid,hgt));


        ul = GETBOOL16(GrayString(hdc,
                                  HBRUSH32(parg16->f2),
                                  (GRAYSTRINGPROC)W32GrayStringProc,
                                  (DWORD)&Gray,
                                  n,
                                  INT32(parg16->f6),
                                  INT32(parg16->f7),
                                  wid,
                                  hgt));

    } else {

       GETPSZPTR(vpstr, psz2);

#ifdef DOESNT_USER_DO_THIS
        if( n==0 ) {
            n=strlen(psz2);
        }
        if( ((wid == 0) || (hgt == 0)) ) {
            GetTextExtentPoint(hdc,psz2,n,&sz);

            wid=sz.cx;
            hgt=sz.cy;
        }
#endif
        ul = GETBOOL16(GrayString(hdc,
                                  HBRUSH32(parg16->f2),
                                  NULL,
                                  (DWORD)psz2,
                                  n,
                                  INT32(parg16->f6),
                                  INT32(parg16->f7),
                                  wid,
                                  hgt));

        FREEPSZPTR(psz2);
    }

    FREEARGPTR(parg16);
    RETURN(ul);
}



/*++
    void InvalidateRect(<hwnd>, <lpRect>, <bErase>)
    HWND <hwnd>;
    LPRECT <lpRect>;
    BOOL <bErase>;

    The %InvalidateRect% function invalidates the client area within the given
    rectangle by adding that rectangle to the window's update region. The
    invalidated rectangle, along with all other areas in the update region, is
    marked for painting when the next WM_PAINT message occurs. The invalidated
    areas accumulate in the update region until the region is processed when the
    next WM_PAINT message occurs, or the region is validated by using the
    %ValidateRect% or %ValidateRgn% function.

    The <bErase> parameter specifies whether the background within the update
    area is to be erased when the update region is processed. If <bErase> is
    TRUE, the background is erased when the %BeginPaint% function is called;
    if <bErase> is FALSE, the background remains unchanged. If <bErase> is
    TRUE for any part of the update region, the background in the entire
    region is erased, not just in the given part.

    <hwnd>
        Identifies the window whose update region is to be modified.

    <lpRect>
        Points to a %RECT% structure that contains the rectangle
        (in client coordinates) to be added to the update region. If the
        <lpRect> parameter is NULL, the entire client area is added to the
        region.

    <bErase>
        Specifies whether the background within the update region is to
        be erased.

    This function does not return a value.

    Windows sends a WM_PAINT message to a window whenever its update region is
    not empty and there are no other messages in the application queue for that
    window.
--*/

ULONG FASTCALL WU32InvalidateRect(PVDMFRAME pFrame)
{
    RECT t2, *p2;
    register PINVALIDATERECT16 parg16;

    GETARGPTR(pFrame, sizeof(INVALIDATERECT16), parg16);
    p2 = GETRECT16(parg16->f2, &t2);

    InvalidateRect(
        HWND32(parg16->f1),
        p2,
        BOOL32(parg16->f3)
        );

    FREEARGPTR(parg16);
    RETURN(1);    // Win 3.x always returned 1 as a side-effect of jmping to
                  // IRedrawWindow [core\user\wmupdate.c] - MarkRi 5/93
}


/*++
    void InvalidateRgn(<hwnd>, <hRgn>, <bErase>)
    HWND <hwnd>;
    HRGN <hRgn>;
    BOOL <bErase>;

    The %InvalidateRgn% function invalidates the client area within the given
    region by adding it to the current update region of the given window. The
    invalidated region, along with all other areas in the update region, is
    marked for painting when the next WM_PAINT message occurs. The invalidated
    areas accumulate in the update region until the region is processed when the
    next WM_PAINT message occurs, or the region is validated by using the
    %ValidateRect% or %ValidateRgn% function.

    The <bErase> parameter specifies whether the background within the update
    area is to be erased when the update region is processed. If <bErase> is
    TRUE, the background is erased when the %BeginPaint% function is called; if
    <bErase> is FALSE, the background remains unchanged. If <bErase> is TRUE for
    any part of the update region, the background in the entire region is
    erased, not just in the given part.

    <hwnd>
        Identifies the window whose update region is to be modified.

    <hRgn>
        Identifies the region to be added to the update region. The
        region is assumed to have client coordinates.

    <bErase>
        Specifies whether the background within the update region is to
        be erased.

    This function does not return a value.

    Windows sends a WM_PAINT message to a window whenever its update region is
    not empty and there are no other messages in the application queue for that
    window.

    The given region must have been previously created by using one of the
    region functions (for more information, see Chapter 1, Window Manager
    Interface Functions).
--*/

ULONG FASTCALL WU32InvalidateRgn(PVDMFRAME pFrame)
{
    register PINVALIDATERGN16 parg16;

    GETARGPTR(pFrame, sizeof(INVALIDATERGN16), parg16);

    InvalidateRgn(
        HWND32(parg16->f1),
        HRGN32(parg16->f2),
        BOOL32(parg16->f3)
        );

    FREEARGPTR(parg16);
    RETURN(1);    // Win 3.x always returned 1 as a side-effect of jmping to
                  // IRedrawWindow [core\user\wmupdate.c] - MarkRi 5/93
}


/*++
    void InvertRect(<hDC>, <lpRect>)
    HDC <hDC>;
    LPRECT <lpRect>;

    The %InvertRect% function inverts the contents of the given rectangle. On
    monochrome displays, the %InvertRect% function makes white pixels black, and
    black pixels white. On color displays, the inversion depends on how colors
    are generated for the display. Calling %InvertRect% twice with the same
    rectangle restores the display to its previous colors.

    <hDC>
        Identifies the device context.

    <lpRect>
        Points to a %RECT% structure that contains the logical coordinates of
        the rectangle to be inverted.

    This function does not return a value.

    The %InvertRect% function compares the values of the %top%, %bottom%,
    %left%, and %right% members of the specified rectangle. If %bottom% is less
    than or equal to %top%, or if %right% is less than or equal to %left%, the
    rectangle is not drawn.
--*/

ULONG FASTCALL WU32InvertRect(PVDMFRAME pFrame)
{
    RECT t2;
    register PINVERTRECT16 parg16;

    GETARGPTR(pFrame, sizeof(INVERTRECT16), parg16);

    WOW32VERIFY(GETRECT16(parg16->f2, &t2));

    InvertRect(
        HDC32(parg16->f1),
        &t2
        );

    FREEARGPTR(parg16);
    RETURN(0);
}


ULONG FASTCALL WU32LoadBitmap(PVDMFRAME pFrame)
{
    ULONG ul = 0;
    PSZ psz2;
    register PLOADBITMAP16 parg16;
    LPBYTE pResData = NULL;

    GETARGPTR(pFrame, sizeof(LOADBITMAP16), parg16);
    GETPSZIDPTR(parg16->f2, psz2);
    GETMISCPTR(parg16->f3, pResData);

    ul = GETHBITMAP16((pfnOut.pfnWOWLoadBitmapA)(HINSTRES32(parg16->f1),
                                     psz2,
                                     pResData,
                                     parg16->f4));

    FREEMISCPTR(pResData);
    FREEPSZIDPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    int ReleaseDC(<hwnd>, <hDC>)
    HWND <hwnd>;
    HDC <hDC>;

    The %ReleaseDC% function releases a device context, freeing it for use by
    other applications. The effect of the %ReleaseDC% function depends on the
    device-context type. It only frees common and window device contexts. It has
    no effect on class or private device contexts.

    <hwnd>
        Identifies the window whose device context is to be released.

    <hDC>
        Identifies the device context to be released.

    The return value specifies whether the device context is released. It is 1
    if the device context is released. Otherwise, it is zero.

    The application must call the %ReleaseDC% function for each call to the
    %GetWindowDC% function and for each call to the %GetDC% function that
    retrieves a common device context.
--*/

ULONG FASTCALL WU32ReleaseDC(PVDMFRAME pFrame)
{
    ULONG ul;
    register PRELEASEDC16 parg16;
    HAND16 htask16 = CURRENTPTD()->htask16;

    GETARGPTR(pFrame, sizeof(RELEASEDC16), parg16);

    CURRENTPTD()->ulLastDesktophDC = 0;

    CacheReleasedDC(htask16, parg16->f1, parg16->f2);
    ul = TRUE;

    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    BOOL ScrollDC(<hDC>, <dx>, <dy>, <lprcScroll>, <lprcClip>, <hrgnUpdate>,
        <lprcUpdate>)
    HDC <hDC>;
    int <dx>;
    int <dy>;
    LPRECT <lprcScroll>;
    LPRECT <lprcClip>;
    HRGN <hrgnUpdate>;
    LPRECT <lprcUpdate>;

    The %ScrollDC% function scrolls a rectangle of bits horizontally and
    vertically. The <lprcScroll> parameter points to the rectangle to be
    scrolled, the <dx> parameter specifies the number of units to be scrolled
    horizontally, and the <dy> parameter specifies the number of units to be
    scrolled vertically.

    <hDC>
        Identifies the device context that contains the bits to be scrolled.

    <dx>
        Specifies the number of horizontal scroll units.

    <dy>
        Specifies the number of vertical scroll units.

    <lprcScroll>
        Points to the %RECT% structure that contains the
        coordinates of the scrolling rectangle.

    <lprcClip>
        Points to the %RECT% structure that contains the
        coordinates of the clipping rectangle. When this rectangle is smaller
        than the original pointed to by <lprcScroll>, scrolling occurs only in
        the smaller rectangle.

    <hrgnUpdate>
        Identifies the region uncovered by the scrolling process. The
        %ScrollDC% function defines this region; it is not necessarily a
        rectangle.

    <lprcUpdate>
        Points to the %RECT% structure that, upon return, contains
        the coordinates of the rectangle that bounds the scrolling update
        region. This is the largest rectangular area that requires repainting.

    This value specifies the outcome of the function. It is TRUE if scrolling is
    executed. Otherwise, it is FALSE.

    If the <lprcUpdate> parameter is NULL, Windows does not compute the update
    rectangle. If both the <hrgnUpdate> and <lprcUpdate> parameters are NULL,
    Windows does not compute the update region. If <hrgnUpdate> is not NULL,
    Windows assumes that it contains a valid region handle to the region
    uncovered by the scrolling process (defined by the %ScrollDC% function).

    An application should use the %ScrollWindow% function when it is necessary
    to scroll the entire client area of a window. Otherwise, it should use
    %ScrollDC%.
--*/

ULONG FASTCALL WU32ScrollDC(PVDMFRAME pFrame)
{
    ULONG ul;
    RECT t4;
    RECT t5;
    RECT t7;
    register PSCROLLDC16 parg16;

    GETARGPTR(pFrame, sizeof(SCROLLDC16), parg16);

    ul = GETBOOL16(ScrollDC(
    HDC32(parg16->f1),
    INT32(parg16->f2),
    INT32(parg16->f3),
    GETRECT16(parg16->f4, &t4),
    GETRECT16(parg16->f5, &t5),
    HRGN32(parg16->f6),
    &t7
    ));

    PUTRECT16(parg16->f7, &t7);
    FREEARGPTR(parg16);
    RETURN(ul);
}


/*++
    HWND SetCapture(<hwnd>)
    HWND <hwnd>;

    The %SetCapture% function causes all subsequent mouse input to be sent to
    the window specified by the <hwnd> parameter, regardless of the position of
    the cursor.

    <hwnd>
        Identifies the window that is to receive the mouse input.

    The return value identifies the window that previously received all mouse
    input. It is NULL if there is no such window.

    When the window no longer requires all mouse input, the application should
    call the %ReleaseCapture% function so that other windows can receive mouse
    input.
--*/

ULONG FASTCALL WU32SetCapture(PVDMFRAME pFrame)
{
    ULONG ul;
    register PSETCAPTURE16 parg16;

    GETARGPTR(pFrame, sizeof(SETCAPTURE16), parg16);

    // MS Works Ver 3.0B has an unintialized local variable. We need to make
    // sure it see's a positive int value in the location on the stack where we
    // write the 32-bit thunk address for fast dispatching to this thunk.

    if (CURRENTPTD()->dwWOWCompatFlagsEx & WOWCFEX_SETCAPSTACK) {
        // wCallID has already been used for dispatch so we can overwrite it.
        // Note: This will cause the logging on checked builds show ISCHILD()
        //       as the return API instead of SetCapture().
        //       For folks grepping for this:  SetCapture() : IsChild()
        pFrame->wCallID = 0x100;
    }

    ul = GETHWND16(SetCapture(HWND32(parg16->f1)));

    FREEARGPTR(parg16);
    RETURN(ul);
}


ULONG FASTCALL WU32SetEventHook(PVDMFRAME pFrame)
{
    PTD     ptd;
    PTDB    pTDB;
    DWORD   dwButtonPushed;
#ifdef FE_SB
    CHAR    szErrorMessage[256];
#else // !FE_SB
    CHAR    szErrorMessage[200];
#endif // !FE_SB
    char    szModName[9];
    char    szTitle[100];
    register PSETEVENTHOOK16 parg16;

    GETARGPTR(pFrame, sizeof(SETEVENTHOOK16), parg16);

    // Retail Build

    ptd = CURRENTPTD();
    if (ptd->dwFlags & TDF_FORCETASKEXIT) {
        goto SetEventHookExit;
    }

    pTDB = (PVOID)SEGPTR(ptd->htask16,0);

    RtlCopyMemory(szModName, pTDB->TDB_ModName, sizeof(szModName)-1);
    szModName[sizeof(szModName)-1] = 0;

    if (!LoadString(hmodWOW32, iszEventHook,
                    szErrorMessage, sizeof(szErrorMessage)/sizeof(CHAR)))
    {
        szErrorMessage[0] = 0;
    }
    if (!LoadString(hmodWOW32, iszApplication,
                    szTitle, sizeof(szTitle)/sizeof(CHAR)))
    {
        szTitle[0] = 0;
    }
    strcat(szTitle, szModName);

    dwButtonPushed = WOWSysErrorBox(
            szTitle,
            szErrorMessage,
            SEB_CLOSE | SEB_DEFBUTTON,
            0,
            SEB_IGNORE
            );

    if (dwButtonPushed != 3) {
        //
        // If user typed Cancel or Any of the above fail,
        // force the task to die.
        //

        GETFRAMEPTR(ptd->vpStack, pFrame);
        pFrame->wRetID = RET_FORCETASKEXIT;

        ptd->dwFlags |= TDF_FORCETASKEXIT;
    }

SetEventHookExit:
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void SetKeyboardState(<lpKeyState>)
    LPBYTE <lpKeyState>;

    The %SetKeyboardState% function copies the 256 bytes pointed to by the
    <lpKeyState> parameter into the Windows keyboard-state table.

    <lpKeyState>
        Points to an array of 256 bytes that contains keyboard key states.

    This function does not return a value.

    In many cases, an application should call the %GetKeyboardState% function
    first to initialize the 256-byte array. The application should then change
    the desired bytes.

    %SetKeyboardState% sets the LEDs and BIOS flags for the ^NUMLOCK^,
    ^CAPSLOCK^, and ^SCROLL LOCK^ keys according to the toggle state of the
    VK_NUMLOCK, VK_CAPITAL, and VK_OEM_SCROLL entries of the array.

    For more information, see the description of %GetKeyboardState%, earlier in
    this chapter.
--*/

ULONG FASTCALL WU32SetKeyboardState(PVDMFRAME pFrame)
{
    PBYTE p1;
    register PSETKEYBOARDSTATE16 parg16;

    GETARGPTR(pFrame, sizeof(SETKEYBOARDSTATE16), parg16);
    GETVDMPTR(parg16->f1, 256, p1);

    SetKeyboardState(
    p1
    );

    FREEVDMPTR(p1);
    FREEARGPTR(parg16);
    RETURN(0);
}


/*++
    void SetSysColors(<cDspElements>, <aiDspElements>, <aRgbValues>)
    int <cDspElements>;
    LPINT <aiDspElements>;
    LPDWORD <aRgbValues>;

    The %SetSysColors% function sets the system colors for one or more display
    elements. Display elements are the various parts of a window and the Windows
    display that appear on the system display screen.

    The %SetSysColors% function sends a WM_SYSCOLORCHANGE message to all windows
    to inform them of the change in color. It also directs Windows to repaint
    the affected portions of all currently visible windows.

    <cDspElements>
        Specifies the number of display elements in the <aiDspElements> array.

    <aiDspElements>
        Points to an array of integers that specify the display elements
        to be changed. For a list of possible display elements, see the following
        "Comments" section.

    <aRgbValues>
        Points to an array of unsigned long integers that contains the new RGB
        color value for each display element in the <aiDspElements> array.

    This function does not return a value.

    The %SetSysColors% function changes the current Windows session only. The
    new colors are not saved when Windows terminates.

    The following is the list of display elements that may be used in the array
    of display elements pointed to by the <aiDspElements> parameter:

    COLOR_ACTIVEBORDER
        Active window border.

    COLOR_ACTIVECAPTION
        Active window caption.

    COLOR_APPWORKSPACE
        Background color of multiple document interface (MDI) applications.

    COLOR_BACKGROUND
        Desktop.

    COLOR_BTNFACE
        Face shading on push buttons.

    COLOR_BTNSHADOW
        Edge shading on push buttons.

    COLOR_BTNTEXT
        Text on push buttons.

    COLOR_CAPTIONTEXT
        Text in caption, size box, scroll-bar arrow box.

    COLOR_GRAYTEXT
        Grayed (disabled) text. This color is set to 0 if the current display
        driver does not support a solid gray color.

    COLOR_HIGHLIGHT
        Items selected item in a control.

    COLOR_HIGHLIGHTTEXT
        Text of item selected in a control.

    COLOR_INACTIVEBORDER
        Inactive window border.

    COLOR_INACTIVECAPTION
        Inactive window caption.

    COLOR_INACTIVECAPTIONTEXT
        Color of text in an inactive caption.

    COLOR_MENU
        Menu background.

    COLOR_MENUTEXT
        Text in menus.

    COLOR_SCROLLBAR
        Scroll-bar gray area.

    COLOR_WINDOW
        Window background.

    COLOR_WINDOWFRAME
        Window frame.

    COLOR_WINDOWTEXT
        Text in windows.
--*/

#define SSC_BUF_SIZE	    256

ULONG FASTCALL WU32SetSysColors(PVDMFRAME pFrame)
{
    PINT p2;
    PDWORD p3;
    register PSETSYSCOLORS16 parg16;
    INT BufElements[SSC_BUF_SIZE];

    GETARGPTR(pFrame, sizeof(SETSYSCOLORS16), parg16);
    p2 = STACKORHEAPALLOC(INT32(parg16->f1) * sizeof(INT), sizeof(BufElements), BufElements);
    getintarray16(parg16->f2, INT32(parg16->f1), p2);
    GETDWORDARRAY16(parg16->f3, INT32(parg16->f1), p3);

    if (SetSysColors(
        INT32(parg16->f1),
        p2,
        p3
        ) == FALSE) {
#ifndef i386
    PDWORD p4;
    ULONG   BufRGB [SSC_BUF_SIZE];

        // On RISC platforms, SetSysColors could fail if the third parameter
        // is unaligned. We need to check that and copy it to an aligned
        // buffer before making this call. Win16 SetSysColor never fails
        // so on x86 if this ever fails under NT, it will just pass through.

        if ((ULONG)p3 & 3) {

            p4 = STACKORHEAPALLOC(INT32(parg16->f1) * sizeof(INT), sizeof(BufRGB), BufRGB);

            RtlMoveMemory ((PVOID)p4, (CONST VOID *)p3,
                                       INT32(parg16->f1) * sizeof(ULONG));


            SetSysColors(
                INT32(parg16->f1),
                p2,
                p4
                );
            STACKORHEAPFREE(p4, BufRGB);
        }
#endif

    }

    FREEDWORDARRAY16(p3);
    STACKORHEAPFREE(p2, BufElements);
    FREEARGPTR(parg16);
    RETURN(0);
}

/*++
    void InvalidateRect(<hwnd>, <lpRect>, <bErase>)
    HWND <hwnd>;
    LPRECT <lpRect>;
    BOOL <bErase>;

    The %InvalidateRect% function invalidates the client area within the given
    rectangle by adding that rectangle to the window's update region. The
    invalidated rectangle, along with all other areas in the update region, is
    marked for painting when the next WM_PAINT message occurs. The invalidated
    areas accumulate in the update region until the region is processed when the
    next WM_PAINT message occurs, or the region is validated by using the
    %ValidateRect% or %ValidateRgn% function.

    The <bErase> parameter specifies whether the background within the update
    area is to be erased when the update region is processed. If <bErase> is
    TRUE, the background is erased when the %BeginPaint% function is called;
    if <bErase> is FALSE, the background remains unchanged. If <bErase> is
    TRUE for any part of the update region, the background in the entire
    region is erased, not just in the given part.

    <hwnd>
        Identifies the window whose update region is to be modified.

    <lpRect>
        Points to a %RECT% structure that contains the rectangle
        (in client coordinates) to be added to the update region. If the
        <lpRect> parameter is NULL, the entire client area is added to the
        region.

    <bErase>
        Specifies whether the background within the update region is to
        be erased.

    This function does not return a value.

    Windows sends a WM_PAINT message to a window whenever its update region is
    not empty and there are no other messages in the application queue for that
    window.
--*/

ULONG FASTCALL WU32ValidateRect(PVDMFRAME pFrame)
{
    RECT t2, *p2;
    register PVALIDATERECT16 parg16;

    GETARGPTR(pFrame, sizeof(VALIDATERECT16), parg16);
    p2 = GETRECT16(parg16->f2, &t2);

    ValidateRect(
        HWND32(parg16->f1),
        p2
        );

    FREEARGPTR(parg16);
    RETURN(1);    // Win 3.x always returned 1 as a side-effect of jmping to
                  // IRedrawWindow [core\user\wmupdate.c] - MarkRi 5/93
}


/*++
    void InvalidateRgn(<hwnd>, <hRgn>, <bErase>)
    HWND <hwnd>;
    HRGN <hRgn>;
    BOOL <bErase>;

    The %InvalidateRgn% function invalidates the client area within the given
    region by adding it to the current update region of the given window. The
    invalidated region, along with all other areas in the update region, is
    marked for painting when the next WM_PAINT message occurs. The invalidated
    areas accumulate in the update region until the region is processed when the
    next WM_PAINT message occurs, or the region is validated by using the
    %ValidateRect% or %ValidateRgn% function.

    The <bErase> parameter specifies whether the background within the update
    area is to be erased when the update region is processed. If <bErase> is
    TRUE, the background is erased when the %BeginPaint% function is called; if
    <bErase> is FALSE, the background remains unchanged. If <bErase> is TRUE for
    any part of the update region, the background in the entire region is
    erased, not just in the given part.

    <hwnd>
        Identifies the window whose update region is to be modified.

    <hRgn>
        Identifies the region to be added to the update region. The
        region is assumed to have client coordinates.

    <bErase>
        Specifies whether the background within the update region is to
        be erased.

    This function does not return a value.

    Windows sends a WM_PAINT message to a window whenever its update region is
    not empty and there are no other messages in the application queue for that
    window.

    The given region must have been previously created by using one of the
    region functions (for more information, see Chapter 1, Window Manager
    Interface Functions).
--*/

ULONG FASTCALL WU32ValidateRgn(PVDMFRAME pFrame)
{
    register PVALIDATERGN16 parg16;

    GETARGPTR(pFrame, sizeof(VALIDATERGN16), parg16);

    ValidateRgn(
        HWND32(parg16->f1),
        HRGN32(parg16->f2)
        );

    FREEARGPTR(parg16);
    RETURN(1);    // Win 3.x always returned 1 as a side-effect of jmping to
                  // IRedrawWindow [core\user\wmupdate.c] - MarkRi 5/93
}


/*++
    BOOL WinHelp(<hwnd>, <lpHelpFile>, <wCommand>, <dwData>)
    HWND <hwnd>;
    LPSTR <lpHelpFile>;
    WORD <wCommand>;
    DWORD <dwData>;

    This function invokes the Windows Help application and passes optional data
    indicating the nature of the help requested by the application. The
    application specifies the name and, where required, the directory path of
    the help file which the Help application is to display.

    <hwnd>
        Identifies the window requesting help.

    <lpHelpFile>
        Points to a null-terminated string containing the directory
        path, if needed, and the name of the help file which the Help
        application is to display.

    <wCommand>
        Specifies the type of help requested. It may be any one of the
        following values:

    HELP_CONTEXT
        Displays help for a particular context identified by a 32-bit unsigned
        integer value in dwData.

    HELP_HELPONHELP
        Displays help for using the help application itself. If the <wCommand>
        parameter is set to HELP_HELPONHELP, %WinHelp% ignores the
        <lpHelpFile> and <dwData> parameters.

    HELP_INDEX
        Displays the index of the specified help file. An application should use
        this value only for help files with a single index. It should not use
        this value with HELP_SETINDEX.

    HELP_MULTIKEY
        Displays help for a key word in an alternate keyword table.

    HELP_QUIT
        Notifies the help application that the specified help file is no longer
        in use.

    HELP_SETINDEX
        Sets the context specified by the <dwData> parameter as the current
        index for the help file specified by the <lpHelpFile> parameter. This
        index remains current until the user accesses a different help file. To
        help ensure that the correct index remains set, the application should
        call %WinHelp% with <wCommand> set to HELP_SETINDEX (with <dwData>
        specifying the corresponding context identifier) following each call to
        %WinHelp% with <wCommand> set to HELP_CONTEXT. An application should use
        this value only for help files with more than one index. It should not
        use this value with HELP_INDEX.

    <dwData>
        %DWORD% Specifies the context or key word of the help requested. If
        <wCommand> is HELP_CONTEXT, <dwData> is a 32-bit unsigned integer
        containing a context-identifier number. If <wCommand> is HELP_KEY,
        <dwData> is a long pointer to a null-terminated string that contains a
        key word identifying the help topic. If <wCommand> is HELP_MULTIKEY,
        <dwData> is a long pointer to a %MULTIKEYHELP% structure.
        Otherwise, <dwData> is ignored and should be set to NULL.

    The return value specifies the outcome of the function. It is TRUE if the
    function was successful. Otherwise it is FALSE.

    The application must call %WinHelp% with <wCommand> set to HELP_QUIT before
    closing the window that requested the help. The Help application will not
    actually terminate until all applications that have requested help have
    called %WinHelp% with <wCommand> set to HELP_QUIT.
--*/

#if 0
// this function no longer thunked
ULONG FASTCALL WU32WinHelp(PVDMFRAME pFrame)
{
    ULONG ul;
    PSZ psz2;
    register PWINHELP16 parg16;
    DWORD dwCommand;
    DWORD dwData;
    UINT  cb;
    MULTIKEYHELP *lpmkey;
    PMULTIKEYHELP16 pmkey16;
    HELPWININFO     hwinfo;
    PHELPWININFO16  phwinfo16;

    GETARGPTR(pFrame, sizeof(WINHELP16), parg16);
    GETPSZPTR(parg16->f2, psz2);
    dwCommand = WORD32(parg16->f3);

    switch (dwCommand) {
        case HELP_KEY:
        case HELP_COMMAND:
        case HELP_PARTIALKEY:
            GETPSZPTR(parg16->f4, (PSZ)dwData);
            break;

        case HELP_MULTIKEY:
            GETVDMPTR(parg16->f4, sizeof(MULTIKEYHELP16), pmkey16);
            cb = FETCHWORD(pmkey16->mkSize);
            FREEVDMPTR(pmkey16);
            GETVDMPTR(parg16->f4, cb, pmkey16);

            //
            // It is my understanding that 'mkSize' is the total
            // data length and NOT just sizeof(MULTIKEYHELP)
            //

            cb += sizeof(MULTIKEYHELP) - sizeof(MULTIKEYHELP16);
            lpmkey = (MULTIKEYHELP *)malloc_w(cb);
            if (lpmkey) {
                lpmkey->mkSize = cb;
                lpmkey->mkKeylist = pmkey16->mkKeylist;
                strcpy(lpmkey->szKeyphrase, pmkey16->szKeyphrase);
            }
            FREEVDMPTR(pmkey16);
            dwData = (DWORD)lpmkey;
            break;

        case HELP_SETWINPOS:
            GETVDMPTR(parg16->f4, sizeof(HELPWININFO16), phwinfo16);

            hwinfo.wStructSize = (int)(FETCHWORD(phwinfo16->wStructSize) +
                                 (sizeof(HELPWININFO) - sizeof(HELPWININFO16)));
            hwinfo.x           = (int)FETCHSHORT(phwinfo16->x);
            hwinfo.y           = (int)FETCHSHORT(phwinfo16->y);
            hwinfo.dx          = (int)FETCHSHORT(phwinfo16->dx);
            hwinfo.dy          = (int)FETCHSHORT(phwinfo16->dy);
            hwinfo.wMax        = (int)FETCHSHORT(phwinfo16->wMax);
            hwinfo.rgchMember[0] = (CHAR)phwinfo16->rgchMember[0];
            hwinfo.rgchMember[1] = (CHAR)phwinfo16->rgchMember[1];

            FREEVDMPTR(phwinfo16);
            dwData = (DWORD)&hwinfo;
            break;

        default:
            dwData = DWORD32(parg16->f4);
            break;
    }

    ul = GETBOOL16(WinHelp(HWND32(parg16->f1), psz2, dwCommand, dwData));

    switch (dwCommand) {
        case HELP_KEY:
        case HELP_COMMAND:
        case HELP_PARTIALKEY:
            FREEPSZPTR((PSZ)dwData);
            break;

        case HELP_MULTIKEY:
            if (lpmkey)
                free_w(lpmkey);
            break;

        case HELP_SETWINPOS:
            break;

        default:
            break;
    }


    FREEPSZPTR(psz2);
    FREEARGPTR(parg16);
    RETURN(ul);
}
#endif


#pragma pack(1)

//
// win16 Module Table structure (based off of ne header)
// see wow16\inc\newexe.inc
//

typedef struct _NE_MODULE {
    USHORT ne_magic;           // Magic number
    USHORT ne_usage;           // usage count of module
    USHORT ne_enttab;          // Offset of Entry Table
    USHORT ne_pnextexe;        // sel next module table
    USHORT ne_pautodata;       // offset autodata seg table
    USHORT ne_pfileinfo;       // offset load file info
    USHORT ne_flags;           // Flag word
    USHORT ne_autodata;        // Automatic data segment number
    USHORT ne_heap;            // Initial heap allocation
    USHORT ne_stack;           // Initial stack allocation
    ULONG  ne_csip;            // Initial CS:IP setting
    ULONG  ne_sssp;            // Initial SS:SP setting
    USHORT ne_cseg;            // Count of file segments
    USHORT ne_cmod;            // Entries in Module Reference Table
    USHORT ne_cbnrestab;       // Size of non-resident name table
    USHORT ne_segtab;          // Offset of Segment Table
    USHORT ne_rsrctab;         // Offset of Resource Table
    USHORT ne_restab;          // Offset of resident name table
    USHORT ne_modtab;          // Offset of Module Reference Table
    USHORT ne_imptab;          // Offset of Imported Names Table
    ULONG  ne_nrestab;         // Offset of Non-resident Names Table
    USHORT ne_cmovent;         // Count of movable entries
    USHORT ne_align;           // Segment alignment shift count
    USHORT ne_cres;            // Count of resource segments
    UCHAR  ne_exetyp;          // Target Operating system
    UCHAR  ne_flagsothers;     // Other .EXE flags
    USHORT ne_pretthunks;      // offset to return thunks
    USHORT ne_psegrefbytes;    // offset to segment ref. bytes
    USHORT ne_swaparea;        // Minimum code swap area size
    USHORT ne_expver;          // Expected Windows version number
} NEMODULE;
typedef NEMODULE UNALIGNED *PNEMODULE;

#pragma pack()

#ifdef FE_IME
VOID WN32WINNLSSImeNotifyTaskExit();      // wnman.c
#endif // FE_IME

//
//   Performs Module cleanup (win31:tmdstroy.c\ModuleUnload())
//
void
ModuleUnload(
   HAND16  hModule16,
   BOOL fTaskExit
   )
{
   PNEMODULE pNeModule = SEGPTR(hModule16, 0);
   PTD ptd = CURRENTPTD();

   if (pNeModule->ne_usage == 1 || fTaskExit) {
       W32UnhookHooks(hModule16,FALSE);
   }

   if (fTaskExit) {
       ptd->dwFlags |= TDF_TASKCLEANUPDONE;
       (pfnOut.pfnWOWCleanup)(HINSTRES32(ptd->hInst16), (DWORD) ptd->htask16);
   }

   if (pNeModule->ne_usage > 1) {
       return;
       }

#ifdef FE_IME
   /*
    * We need to notify IMM that this WOW task is quiting before
    * calling WowCleanup or IME windows can not receive WM_DESTROY
    * and will fail to clean up their 32bit resource.
    */
   if ( fTaskExit ) {
       WN32WINNLSSImeNotifyTaskExit();
   }
#endif // FE_IME


    /*   WowCleanup, UserSrv private api
     *   It cleans up any USER objects created by this hModule, most notably
     *   classes, and subclassed windows.
     */
   (pfnOut.pfnWOWModuleUnload)((HANDLE)hModule16);

   RemoveHmodFromCache(hModule16);

}


WORD
FASTCALL
WOWGetProcModule16(
    DWORD vpfn
    )
{
    WOW32ASSERTMSG(gpfn16GetProcModule, "WOWGetProcModule16 called before gpfn16GetProcModule initialized.\n");

    return (WORD) WOWCallback16(
                      gpfn16GetProcModule,
                      vpfn
                      );
}


/*++
    BOOL SignalProc(<hwnd>, <lpHelpFile>, <wCommand>, <dwData>)
    HWND <hwnd>;
    LPSTR <lpHelpFile>;
    WORD <wCommand>;
    DWORD <dwData>;

    This function provides the communication link between KERNEL and USER.

--*/

#define SG_EXIT         0x0020
#define SG_LOAD_DLL     0x0040
#define SG_EXIT_DLL     0x0080
#define SG_GP_FAULT     0x0666



ULONG FASTCALL WU32SignalProc(PVDMFRAME pFrame)
{
    WORD    message;
    LONG    lparam;
    register PSIGNALPROC16 parg16;
    HAND16  hModule16;
    PTD     ptd;

    GETARGPTR(pFrame, sizeof(SIGNALPROC16), parg16);
    message   = FETCHWORD(parg16->f2);

    switch( message ) {
        case SG_EXIT:
        case SG_GP_FAULT:
            lparam    = FETCHDWORD(parg16->f4);
            ptd = CURRENTPTD();
            ptd->dwFlags |= TDF_IGNOREINPUT;
            ptd->cStackAlloc16 = 0;
            ModuleUnload(GetExePtr16((HAND16)HIWORD(lparam)), TRUE);
            FreeCursorIconAlias(ptd->htask16, CIALIAS_HTASK);
            break;

        case SG_LOAD_DLL:
            break;

        case SG_EXIT_DLL:
            hModule16 = FETCHWORD(parg16->f1);
            ModuleUnload(hModule16, FALSE);
            FreeCursorIconAlias(hModule16, CIALIAS_HMOD);
            break;
    }

    FREEARGPTR(parg16);
    RETURN(0);
}





// This routine checks the RECT structure (in PAINTSTRUCT) on BeginPaint
// call and updates its fields for maximum positive and minimum negative
// numbers for 16 bit apps to be compatible with win 3.1.
//

void W32FixPaintRect (VPVOID vpPaint, LPPAINTSTRUCT ps)
{
    SHORT i;
    PPAINTSTRUCT16 pps16;

    GETVDMPTR(vpPaint, sizeof(PAINTSTRUCT16), pps16);

    if (i = ConvertInt16 (ps->rcPaint.left)) {
        STORESHORT(pps16->rcPaint.left, i);
    }

    if (i = ConvertInt16 (ps->rcPaint.top)) {
        STORESHORT(pps16->rcPaint.top, i);
    }

    if (i = ConvertInt16 (ps->rcPaint.right)) {
        STORESHORT(pps16->rcPaint.right, i);
    }

    if (i = ConvertInt16 (ps->rcPaint.bottom)) {
        STORESHORT(pps16->rcPaint.bottom, i);
    }

    FLUSHVDMPTR(vpPaint, sizeof(PAINTSTRUCT16), pps16);
    FREEVDMPTR(pps16);
}

SHORT   ConvertInt16 (LONG x)
{
    if (x > (LONG)0x7fff)
        return((SHORT)0x7fff);

    if (x < (LONG)0xffff8000)
        return((SHORT)0x8000);

    return ((SHORT)0);
}
