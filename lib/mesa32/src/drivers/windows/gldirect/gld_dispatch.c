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
* Environment:  Windows 9x/2000/XP/XBox (Win32)
*
* Description:  Thread-aware dispatch table.
*
****************************************************************************/

#include "glheader.h"
#include "glapi.h"
#include "glapitable.h"
#include "mtypes.h"
#include "context.h"

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#if defined(USE_MGL_NAMESPACE)
	#define NAME(func)  mgl##func
#else
	#define NAME(func)  gl##func
#endif

#if 0
// Altered these to get the dispatch table from 
// the current context of the calling thread.
#define DISPATCH(FUNC, ARGS, MESSAGE)	\
	GET_CURRENT_CONTEXT(gc);			\
	(gc->CurrentDispatch->FUNC) ARGS
#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
	GET_CURRENT_CONTEXT(gc);			\
	return (gc->CurrentDispatch->FUNC) ARGS
#else // #if 0
#define DISPATCH(FUNC, ARGS, MESSAGE)	\
	GET_CURRENT_CONTEXT(gc);			\
	(_glapi_Dispatch->FUNC) ARGS
#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
	GET_CURRENT_CONTEXT(gc);			\
	return (_glapi_Dispatch->FUNC) ARGS
#endif // #if 0

#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#include "glapitemp.h"
