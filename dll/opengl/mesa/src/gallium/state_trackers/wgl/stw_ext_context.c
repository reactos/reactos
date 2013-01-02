/*
 * Mesa 3-D graphics library
 * Version:  7.11
 *
 * Copyright (C) 2011 Morgan Armand <morgan.devel@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <windows.h>

#define WGL_WGLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/wglext.h>

#include "stw_icd.h"
#include "stw_context.h"

HGLRC WINAPI
wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList)
{
   int majorVersion = 1, minorVersion = 0, layerPlane = 0;
   int contextFlags = 0x0;
   int profileMask = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
   int i;
   BOOL done = FALSE;

   /* parse attrib_list */
   if (attribList) {
      for (i = 0; !done && attribList[i]; i++) {
         switch (attribList[i]) {
         case WGL_CONTEXT_MAJOR_VERSION_ARB:
            majorVersion = attribList[++i];
            break;
         case WGL_CONTEXT_MINOR_VERSION_ARB:
            minorVersion = attribList[++i];
            break;
         case WGL_CONTEXT_LAYER_PLANE_ARB:
            layerPlane = attribList[++i];
            break;
         case WGL_CONTEXT_FLAGS_ARB:
            contextFlags = attribList[++i];
            break;
         case WGL_CONTEXT_PROFILE_MASK_ARB:
            profileMask = attribList[++i];
            break;
         case 0:
            /* end of list */
            done = TRUE;
            break;
         default:
            /* bad attribute */
            SetLastError(ERROR_INVALID_PARAMETER);
            return NULL;
         }
      }
   }

   /* check version (generate ERROR_INVALID_VERSION_ARB if bad) */
   switch (majorVersion) {
   case 1:
      if (minorVersion < 0 || minorVersion > 5) {
         SetLastError(ERROR_INVALID_VERSION_ARB);
         return NULL;
      }
      break;
   case 2:
      if (minorVersion < 0 || minorVersion > 1) {
         SetLastError(ERROR_INVALID_VERSION_ARB);
         return NULL;
      }
      break;
   case 3:
      if (minorVersion < 0 || minorVersion > 3) {
         SetLastError(ERROR_INVALID_VERSION_ARB);
         return NULL;
      }
      break;
   case 4:
      if (minorVersion < 0 || minorVersion > 2) {
         SetLastError(ERROR_INVALID_VERSION_ARB);
         return NULL;
      }
      break;
   default:
      return NULL;
   }

   if ((contextFlags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB) && majorVersion < 3) {
      SetLastError(ERROR_INVALID_VERSION_ARB);
      return NULL;
   }

   /* check profileMask */
   if (profileMask != WGL_CONTEXT_CORE_PROFILE_BIT_ARB &&
       profileMask != WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB) {
      SetLastError(ERROR_INVALID_PROFILE_ARB);
      return NULL;
   }

   return (HGLRC) stw_create_context_attribs( hDC, layerPlane, (DHGLRC)(UINT_PTR)hShareContext,
                                              majorVersion, minorVersion, contextFlags, profileMask );
}
