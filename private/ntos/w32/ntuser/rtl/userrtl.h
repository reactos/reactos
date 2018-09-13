/*++ BUILD Version: 0001    // Increment this if a change has global effects

/****************************** Module Header ******************************\
* Module Name: userrtl.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Typedefs, defines, and prototypes that are used by the User
* RTL library.
*
* History:
* 04-27-91 DarrinM      Created from PROTO.H, MACRO.H and STRTABLE.H
\***************************************************************************/

#ifndef _USERRTL_
#define _USERRTL_

/*
 * Typedefs copied from winbase.h to avoid using nturtl.h
 */
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
#define MAKEINTATOM(i)  (LPTSTR)((ULONG_PTR)((WORD)(i)))

#ifdef _USERK_
    #undef _USERK_
    #include "..\kernel\precomp.h"
    #define _USERK_
#else
    #include "..\client\precomp.h"
#endif

#include <w32p.h>

#include "ntuser.h"

/*
 * REBASE macros take kernel desktop addresses and convert them into
 * user addresses.
 *
 * REBASEALWAYS converts a kernel address contained in an object
 * REBASEPWND casts REBASEALWAYS to a PWND
 * REBASE only converts if the address is in kernel space.  Also works for NULL
 * REBASEPTR converts a random kernel address
 */

#ifdef _USERK_

#define REBASEALWAYS(p, elem) ((p)->elem)
#define REBASEPTR(obj, p) (p)
#define REBASE(p, elem) ((p)->elem)
#define REBASEPWND(p, elem) ((p)->elem)
#endif  // _USERK_


// jcjc extern SHAREDINFO gSharedInfo;
// jcjc extern HFONT ghFontSys;

PVOID UserRtlAllocMem(
    ULONG uBytes);
VOID UserRtlFreeMem(
    PVOID pMem);


#ifdef FE_SB // Prototype for FarEast Line break & NLS conversion.

    #ifdef _USERK_
        #define USERGETCODEPAGE(hdc) (GreGetCharSet(hdc) & 0x0000FFFF)
    #else
        #define USERGETCODEPAGE(hdc) (GdiGetCodePage(hdc))
    #endif // _USERK_


    BOOL UserIsFullWidth(
        DWORD dwCodePage,
        WCHAR wChar);
    BOOL UserIsFELineBreak(
        DWORD dwCodePage,
        WCHAR wChar);

#endif // FE_SB


/***************************************************************************\
*
* Function prototypes for client/server-specific routines
* called from rtl routines.
*
\***************************************************************************/

#ifdef _USERK_

BOOL _TextOutW(
    HDC     hdc,
    int     x,
    int     y,
    LPCWSTR lp,
    UINT    cc);

UINT APIENTRY GreSetTextAlign(HDC, UINT);
UINT APIENTRY GreGetTextAlign(HDC);

#define UserCreateFontIndirectW   GreCreateFontIndirectW
#define UserCreateRectRgn         GreCreateRectRgn
#define UserDeleteObject          GreDeleteObject
#define UserExtSelectClipRgn      GreExtSelectClipRgn
#define UserExtTextOutW           GreExtTextOutW
#define UserGetCharDimensionsW    GetCharDimensions
#define UserGetClipRgn(hdc, hrgnClip) \
        GreGetRandomRgn(hdc, hrgnClip, 1)
#define UserGetHFONT              GreGetHFONT
#define UserGetMapMode            GreGetMapMode
#define UserGetTextColor          GreGetTextColor
#define UserGetTextExtentPointW(hdc, pstr, i, psize) \
        GreGetTextExtentW(hdc, (LPWSTR)pstr, i, psize, GGTE_WIN3_EXTENT)
#define UserGetTextMetricsW       _GetTextMetricsW
#define UserGetViewportExtEx      GreGetViewportExt
#define UserGetWindowExtEx        GreGetWindowExt
#define UserIntersectClipRect     GreIntersectClipRect
#define UserPatBlt                GrePatBlt
#define UserPolyPatBlt            GrePolyPatBlt
#define UserSelectBrush           GreSelectBrush
#define UserSelectFont            GreSelectFont
#define UserSetBkColor            GreSetBkColor
#define UserSetBkMode             GreSetBkMode
#define UserSetTextColor          GreSetTextColor
#define UserTextOutW              _TextOutW
#define UserGetTextCharsetInfo    GreGetTextCharsetInfo
#define UserGetTextAlign          GreGetTextAlign
#define UserSetTextAlign          GreSetTextAlign
#define UserLpkDrawTextEx         xxxClientLpkDrawTextEx
#define UserGetLayout             GreGetLayout
#define UserSetGraphicsMode       GreSetGraphicsMode

#else

#define UserCreateFontIndirectW   CreateFontIndirectW
#define UserCreateRectRgn         CreateRectRgn
#define UserDeleteObject          DeleteObject
#define UserExtSelectClipRgn      ExtSelectClipRgn
#define UserExtTextOutW           ExtTextOutW
#define UserGetCharDimensionsW    GdiGetCharDimensions
#define UserGetClipRgn            GetClipRgn
#define UserGetHFONT              GetHFONT
#define UserGetMapMode            GetMapMode
#define UserGetTextColor          GetTextColor
#define UserGetTextExtentPointW   GetTextExtentPointW
#define UserGetTextMetricsW       GetTextMetricsW
#define UserGetViewportExtEx      GetViewportExtEx
#define UserGetWindowExtEx        GetWindowExtEx
#define UserIntersectClipRect     IntersectClipRect
#define UserPatBlt                PatBlt
#define UserPolyPatBlt            PolyPatBlt
#define UserSelectBrush           SelectObject
#define UserSelectFont            SelectObject
#define UserSetBkColor            SetBkColor
#define UserSetBkMode             SetBkMode
#define UserSetTextColor          SetTextColor
#define UserTextOutW              TextOutW
#define UserGetTextCharsetInfo    GetTextCharsetInfo
#define UserGetTextAlign          GetTextAlign
#define UserSetTextAlign          SetTextAlign
#define UserLpkDrawTextEx         fpLpkDrawTextEx
#define UserGetLayout             GetLayout
#define UserSetGraphicsMode       SetGraphicsMode

#endif // _USERK_

#endif  // !_USERRTL_
