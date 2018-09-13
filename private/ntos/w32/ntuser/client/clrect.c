/****************************** Module Header ******************************\
* Module Name: clrect.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the various rectangle manipulation APIs.
*
* History:
* 04-05-91 DarrinM Pulled these routines from RTL because they call GDI.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* DrawFocusRect (API)
*
* Draw a rectangle in the style used to indicate focus
* Since this is an XOR function, calling it a second time with the same
* rectangle removes the rectangle from the screen
*
* History:
* 19-Jan-1993 mikeke   Client side version
\***************************************************************************/

BOOL DrawFocusRect(
    HDC hDC,
    CONST RECT *pRect)
{
    UserAssert(ghdcGray != NULL);
    return ClientFrame(hDC, pRect, gpsi->hbrGray, PATINVERT);
}

/***************************************************************************\
* FrameRect (API)
*
* History:
*  01-25-91 DavidPe     Created.
\***************************************************************************/

int APIENTRY FrameRect(
    HDC hdc,
    CONST RECT *lprc,
    HBRUSH hbr)
{
    return ClientFrame(hdc, lprc, hbr, PATCOPY);
}
