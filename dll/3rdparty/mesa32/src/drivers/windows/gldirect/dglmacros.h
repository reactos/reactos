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
* Description:  Useful generic macros.
*
****************************************************************************/

#ifndef __DGLMACROS_H
#define __DGLMACROS_H

#include <ddraw.h>

// Define the relevant RELEASE macro depending on C or C++
#if !defined(__cplusplus) || defined(CINTERFACE)
	// Standard C version
	#define RELEASE(x) if (x!=NULL) { x->lpVtbl->Release(x); x=NULL; }
#else
	// C++ version
	#define RELEASE(x) if (x!=NULL) { x->Release(); x=NULL; }
#endif

// We don't want a message *every* time we call an unsupported function
#define UNSUPPORTED(x)												\
	{																\
		static BOOL bFirstTime = TRUE;								\
		if (bFirstTime) {											\
			bFirstTime = FALSE;										\
			ddlogError(DDLOG_WARN, (x), DDERR_CURRENTLYNOTAVAIL);	\
		}															\
	}

#define DGL_CHECK_CONTEXT		\
	if (ctx == NULL) return;

// Don't render if bCanRender is not TRUE.
#define DGL_CHECK_RENDER		\
	if (!dgl->bCanRender) return;

#if 0
#define TRY(a,b) (a)
#define TRY_ERR(a,b) (a)
#else
// hResult should be defined in the function
// Return codes should be checked via SUCCEDDED and FAILED macros
#define TRY(a,b)									\
	{												\
		if (FAILED(hResult=(a)))					\
			ddlogError(DDLOG_ERROR, (b), hResult);	\
	}

// hResult is a global
// The label exit_with_error should be defined within the calling scope
#define TRY_ERR(a,b)								\
	{												\
		if (FAILED(hResult=(a))) {					\
			ddlogError(DDLOG_ERROR, (b), hResult);	\
			goto exit_with_error;					\
		}											\
	}
#endif // #if 1

#endif
