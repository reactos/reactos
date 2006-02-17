/****************************************************************************
*
*                        Mesa 3-D graphics library
*                        Direct3D Driver Interface
*
*  ========================================================================
*
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
*  ======================================================================
*
* Language:     ANSI C
* Environment:  Windows 9x (Win32)
*
* Description:  Win32 DllMain functions.
*
****************************************************************************/

#ifndef __DLLMAIN_H
#define __DLLMAIN_H

// Macros to control compilation
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#ifndef _USE_GLD3_WGL
#include "DirectGL.h"
#endif // _USE_GLD3_WGL

//#include "gldirect/regkeys.h"
#include "dglglobals.h"
#include "ddlog.h"
#ifndef _USE_GLD3_WGL
#include "d3dtexture.h"
#endif // _USE_GLD3_WGL

#include "dglwgl.h"

extern BOOL bInitialized;

BOOL dglInitDriver(void);
void dglExitDriver(void);

#endif
