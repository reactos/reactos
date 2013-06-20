/* DO NOT EDIT - This file generated automatically by gl_gen_table.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004, 2005
 * (C) Copyright Apple Inc 2011
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* GLXEXT is the define used in the xserver when the GLX extension is being
 * built.  Hijack this to determine whether this file is being built for the
 * server or the client.
 */
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#if (defined(GLXEXT) && defined(HAVE_BACKTRACE)) \
	|| (!defined(GLXEXT) && defined(DEBUG) && !defined(_WIN32_WCE))
#define USE_BACKTRACE
#endif

#ifdef USE_BACKTRACE
#include <execinfo.h>
#endif

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"

#ifdef GLXEXT
#include "os.h"
#endif

static void
__glapi_gentable_NoOp(void) {
    const char *fstr = "Unknown";

    /* Silence potential GCC warning for some #ifdef paths.
     */
    (void) fstr;
#if defined(USE_BACKTRACE)
#if !defined(GLXEXT)
    if (getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
#endif
    {
        void *frames[2];

        if(backtrace(frames, 2) == 2) {
            Dl_info info;
            dladdr(frames[1], &info);
            if(info.dli_sname)
                fstr = info.dli_sname;
        }

#if !defined(GLXEXT)
        fprintf(stderr, "Call to unimplemented API: %s\n", fstr);
#endif
    }
#endif
#if defined(GLXEXT)
    LogMessage(X_ERROR, "GLX: Call to unimplemented API: %s\n", fstr);
#endif
}

static void
__glapi_gentable_set_remaining_noop(struct _glapi_table *disp) {
    GLuint entries = _glapi_get_dispatch_table_size();
    void **dispatch = (void **) disp;
    int i;

    /* ISO C is annoying sometimes */
    union {_glapi_proc p; void *v;} p;
    p.p = __glapi_gentable_NoOp;

    for(i=0; i < entries; i++)
        if(dispatch[i] == NULL)
            dispatch[i] = p.v;
}

struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
    struct _glapi_table *disp = calloc(_glapi_get_dispatch_table_size(), sizeof(void *));
    char symboln[512];

    if(!disp)
        return NULL;

    if(symbol_prefix == NULL)
        symbol_prefix = "";


    if(!disp->NewList) {
        void ** procp = (void **) &disp->NewList;
        snprintf(symboln, sizeof(symboln), "%sNewList", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndList) {
        void ** procp = (void **) &disp->EndList;
        snprintf(symboln, sizeof(symboln), "%sEndList", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CallList) {
        void ** procp = (void **) &disp->CallList;
        snprintf(symboln, sizeof(symboln), "%sCallList", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CallLists) {
        void ** procp = (void **) &disp->CallLists;
        snprintf(symboln, sizeof(symboln), "%sCallLists", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteLists) {
        void ** procp = (void **) &disp->DeleteLists;
        snprintf(symboln, sizeof(symboln), "%sDeleteLists", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenLists) {
        void ** procp = (void **) &disp->GenLists;
        snprintf(symboln, sizeof(symboln), "%sGenLists", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ListBase) {
        void ** procp = (void **) &disp->ListBase;
        snprintf(symboln, sizeof(symboln), "%sListBase", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Begin) {
        void ** procp = (void **) &disp->Begin;
        snprintf(symboln, sizeof(symboln), "%sBegin", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Bitmap) {
        void ** procp = (void **) &disp->Bitmap;
        snprintf(symboln, sizeof(symboln), "%sBitmap", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3b) {
        void ** procp = (void **) &disp->Color3b;
        snprintf(symboln, sizeof(symboln), "%sColor3b", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3bv) {
        void ** procp = (void **) &disp->Color3bv;
        snprintf(symboln, sizeof(symboln), "%sColor3bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3d) {
        void ** procp = (void **) &disp->Color3d;
        snprintf(symboln, sizeof(symboln), "%sColor3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3dv) {
        void ** procp = (void **) &disp->Color3dv;
        snprintf(symboln, sizeof(symboln), "%sColor3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3f) {
        void ** procp = (void **) &disp->Color3f;
        snprintf(symboln, sizeof(symboln), "%sColor3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3fv) {
        void ** procp = (void **) &disp->Color3fv;
        snprintf(symboln, sizeof(symboln), "%sColor3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3i) {
        void ** procp = (void **) &disp->Color3i;
        snprintf(symboln, sizeof(symboln), "%sColor3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3iv) {
        void ** procp = (void **) &disp->Color3iv;
        snprintf(symboln, sizeof(symboln), "%sColor3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3s) {
        void ** procp = (void **) &disp->Color3s;
        snprintf(symboln, sizeof(symboln), "%sColor3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3sv) {
        void ** procp = (void **) &disp->Color3sv;
        snprintf(symboln, sizeof(symboln), "%sColor3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3ub) {
        void ** procp = (void **) &disp->Color3ub;
        snprintf(symboln, sizeof(symboln), "%sColor3ub", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3ubv) {
        void ** procp = (void **) &disp->Color3ubv;
        snprintf(symboln, sizeof(symboln), "%sColor3ubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3ui) {
        void ** procp = (void **) &disp->Color3ui;
        snprintf(symboln, sizeof(symboln), "%sColor3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3uiv) {
        void ** procp = (void **) &disp->Color3uiv;
        snprintf(symboln, sizeof(symboln), "%sColor3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3us) {
        void ** procp = (void **) &disp->Color3us;
        snprintf(symboln, sizeof(symboln), "%sColor3us", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color3usv) {
        void ** procp = (void **) &disp->Color3usv;
        snprintf(symboln, sizeof(symboln), "%sColor3usv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4b) {
        void ** procp = (void **) &disp->Color4b;
        snprintf(symboln, sizeof(symboln), "%sColor4b", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4bv) {
        void ** procp = (void **) &disp->Color4bv;
        snprintf(symboln, sizeof(symboln), "%sColor4bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4d) {
        void ** procp = (void **) &disp->Color4d;
        snprintf(symboln, sizeof(symboln), "%sColor4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4dv) {
        void ** procp = (void **) &disp->Color4dv;
        snprintf(symboln, sizeof(symboln), "%sColor4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4f) {
        void ** procp = (void **) &disp->Color4f;
        snprintf(symboln, sizeof(symboln), "%sColor4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4fv) {
        void ** procp = (void **) &disp->Color4fv;
        snprintf(symboln, sizeof(symboln), "%sColor4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4i) {
        void ** procp = (void **) &disp->Color4i;
        snprintf(symboln, sizeof(symboln), "%sColor4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4iv) {
        void ** procp = (void **) &disp->Color4iv;
        snprintf(symboln, sizeof(symboln), "%sColor4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4s) {
        void ** procp = (void **) &disp->Color4s;
        snprintf(symboln, sizeof(symboln), "%sColor4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4sv) {
        void ** procp = (void **) &disp->Color4sv;
        snprintf(symboln, sizeof(symboln), "%sColor4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4ub) {
        void ** procp = (void **) &disp->Color4ub;
        snprintf(symboln, sizeof(symboln), "%sColor4ub", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4ubv) {
        void ** procp = (void **) &disp->Color4ubv;
        snprintf(symboln, sizeof(symboln), "%sColor4ubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4ui) {
        void ** procp = (void **) &disp->Color4ui;
        snprintf(symboln, sizeof(symboln), "%sColor4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4uiv) {
        void ** procp = (void **) &disp->Color4uiv;
        snprintf(symboln, sizeof(symboln), "%sColor4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4us) {
        void ** procp = (void **) &disp->Color4us;
        snprintf(symboln, sizeof(symboln), "%sColor4us", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Color4usv) {
        void ** procp = (void **) &disp->Color4usv;
        snprintf(symboln, sizeof(symboln), "%sColor4usv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EdgeFlag) {
        void ** procp = (void **) &disp->EdgeFlag;
        snprintf(symboln, sizeof(symboln), "%sEdgeFlag", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagv) {
        void ** procp = (void **) &disp->EdgeFlagv;
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->End) {
        void ** procp = (void **) &disp->End;
        snprintf(symboln, sizeof(symboln), "%sEnd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexd) {
        void ** procp = (void **) &disp->Indexd;
        snprintf(symboln, sizeof(symboln), "%sIndexd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexdv) {
        void ** procp = (void **) &disp->Indexdv;
        snprintf(symboln, sizeof(symboln), "%sIndexdv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexf) {
        void ** procp = (void **) &disp->Indexf;
        snprintf(symboln, sizeof(symboln), "%sIndexf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexfv) {
        void ** procp = (void **) &disp->Indexfv;
        snprintf(symboln, sizeof(symboln), "%sIndexfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexi) {
        void ** procp = (void **) &disp->Indexi;
        snprintf(symboln, sizeof(symboln), "%sIndexi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexiv) {
        void ** procp = (void **) &disp->Indexiv;
        snprintf(symboln, sizeof(symboln), "%sIndexiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexs) {
        void ** procp = (void **) &disp->Indexs;
        snprintf(symboln, sizeof(symboln), "%sIndexs", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexsv) {
        void ** procp = (void **) &disp->Indexsv;
        snprintf(symboln, sizeof(symboln), "%sIndexsv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3b) {
        void ** procp = (void **) &disp->Normal3b;
        snprintf(symboln, sizeof(symboln), "%sNormal3b", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3bv) {
        void ** procp = (void **) &disp->Normal3bv;
        snprintf(symboln, sizeof(symboln), "%sNormal3bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3d) {
        void ** procp = (void **) &disp->Normal3d;
        snprintf(symboln, sizeof(symboln), "%sNormal3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3dv) {
        void ** procp = (void **) &disp->Normal3dv;
        snprintf(symboln, sizeof(symboln), "%sNormal3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3f) {
        void ** procp = (void **) &disp->Normal3f;
        snprintf(symboln, sizeof(symboln), "%sNormal3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3fv) {
        void ** procp = (void **) &disp->Normal3fv;
        snprintf(symboln, sizeof(symboln), "%sNormal3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3i) {
        void ** procp = (void **) &disp->Normal3i;
        snprintf(symboln, sizeof(symboln), "%sNormal3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3iv) {
        void ** procp = (void **) &disp->Normal3iv;
        snprintf(symboln, sizeof(symboln), "%sNormal3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3s) {
        void ** procp = (void **) &disp->Normal3s;
        snprintf(symboln, sizeof(symboln), "%sNormal3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Normal3sv) {
        void ** procp = (void **) &disp->Normal3sv;
        snprintf(symboln, sizeof(symboln), "%sNormal3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2d) {
        void ** procp = (void **) &disp->RasterPos2d;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2dv) {
        void ** procp = (void **) &disp->RasterPos2dv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2f) {
        void ** procp = (void **) &disp->RasterPos2f;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2fv) {
        void ** procp = (void **) &disp->RasterPos2fv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2i) {
        void ** procp = (void **) &disp->RasterPos2i;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2iv) {
        void ** procp = (void **) &disp->RasterPos2iv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2s) {
        void ** procp = (void **) &disp->RasterPos2s;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos2sv) {
        void ** procp = (void **) &disp->RasterPos2sv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3d) {
        void ** procp = (void **) &disp->RasterPos3d;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3dv) {
        void ** procp = (void **) &disp->RasterPos3dv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3f) {
        void ** procp = (void **) &disp->RasterPos3f;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3fv) {
        void ** procp = (void **) &disp->RasterPos3fv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3i) {
        void ** procp = (void **) &disp->RasterPos3i;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3iv) {
        void ** procp = (void **) &disp->RasterPos3iv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3s) {
        void ** procp = (void **) &disp->RasterPos3s;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos3sv) {
        void ** procp = (void **) &disp->RasterPos3sv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4d) {
        void ** procp = (void **) &disp->RasterPos4d;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4dv) {
        void ** procp = (void **) &disp->RasterPos4dv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4f) {
        void ** procp = (void **) &disp->RasterPos4f;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4fv) {
        void ** procp = (void **) &disp->RasterPos4fv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4i) {
        void ** procp = (void **) &disp->RasterPos4i;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4iv) {
        void ** procp = (void **) &disp->RasterPos4iv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4s) {
        void ** procp = (void **) &disp->RasterPos4s;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RasterPos4sv) {
        void ** procp = (void **) &disp->RasterPos4sv;
        snprintf(symboln, sizeof(symboln), "%sRasterPos4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectd) {
        void ** procp = (void **) &disp->Rectd;
        snprintf(symboln, sizeof(symboln), "%sRectd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectdv) {
        void ** procp = (void **) &disp->Rectdv;
        snprintf(symboln, sizeof(symboln), "%sRectdv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectf) {
        void ** procp = (void **) &disp->Rectf;
        snprintf(symboln, sizeof(symboln), "%sRectf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectfv) {
        void ** procp = (void **) &disp->Rectfv;
        snprintf(symboln, sizeof(symboln), "%sRectfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Recti) {
        void ** procp = (void **) &disp->Recti;
        snprintf(symboln, sizeof(symboln), "%sRecti", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectiv) {
        void ** procp = (void **) &disp->Rectiv;
        snprintf(symboln, sizeof(symboln), "%sRectiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rects) {
        void ** procp = (void **) &disp->Rects;
        snprintf(symboln, sizeof(symboln), "%sRects", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rectsv) {
        void ** procp = (void **) &disp->Rectsv;
        snprintf(symboln, sizeof(symboln), "%sRectsv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1d) {
        void ** procp = (void **) &disp->TexCoord1d;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1dv) {
        void ** procp = (void **) &disp->TexCoord1dv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1f) {
        void ** procp = (void **) &disp->TexCoord1f;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1fv) {
        void ** procp = (void **) &disp->TexCoord1fv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1i) {
        void ** procp = (void **) &disp->TexCoord1i;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1iv) {
        void ** procp = (void **) &disp->TexCoord1iv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1s) {
        void ** procp = (void **) &disp->TexCoord1s;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord1sv) {
        void ** procp = (void **) &disp->TexCoord1sv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord1sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2d) {
        void ** procp = (void **) &disp->TexCoord2d;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2dv) {
        void ** procp = (void **) &disp->TexCoord2dv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2f) {
        void ** procp = (void **) &disp->TexCoord2f;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2fv) {
        void ** procp = (void **) &disp->TexCoord2fv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2i) {
        void ** procp = (void **) &disp->TexCoord2i;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2iv) {
        void ** procp = (void **) &disp->TexCoord2iv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2s) {
        void ** procp = (void **) &disp->TexCoord2s;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord2sv) {
        void ** procp = (void **) &disp->TexCoord2sv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3d) {
        void ** procp = (void **) &disp->TexCoord3d;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3dv) {
        void ** procp = (void **) &disp->TexCoord3dv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3f) {
        void ** procp = (void **) &disp->TexCoord3f;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3fv) {
        void ** procp = (void **) &disp->TexCoord3fv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3i) {
        void ** procp = (void **) &disp->TexCoord3i;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3iv) {
        void ** procp = (void **) &disp->TexCoord3iv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3s) {
        void ** procp = (void **) &disp->TexCoord3s;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord3sv) {
        void ** procp = (void **) &disp->TexCoord3sv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4d) {
        void ** procp = (void **) &disp->TexCoord4d;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4dv) {
        void ** procp = (void **) &disp->TexCoord4dv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4f) {
        void ** procp = (void **) &disp->TexCoord4f;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4fv) {
        void ** procp = (void **) &disp->TexCoord4fv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4i) {
        void ** procp = (void **) &disp->TexCoord4i;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4iv) {
        void ** procp = (void **) &disp->TexCoord4iv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4s) {
        void ** procp = (void **) &disp->TexCoord4s;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoord4sv) {
        void ** procp = (void **) &disp->TexCoord4sv;
        snprintf(symboln, sizeof(symboln), "%sTexCoord4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2d) {
        void ** procp = (void **) &disp->Vertex2d;
        snprintf(symboln, sizeof(symboln), "%sVertex2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2dv) {
        void ** procp = (void **) &disp->Vertex2dv;
        snprintf(symboln, sizeof(symboln), "%sVertex2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2f) {
        void ** procp = (void **) &disp->Vertex2f;
        snprintf(symboln, sizeof(symboln), "%sVertex2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2fv) {
        void ** procp = (void **) &disp->Vertex2fv;
        snprintf(symboln, sizeof(symboln), "%sVertex2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2i) {
        void ** procp = (void **) &disp->Vertex2i;
        snprintf(symboln, sizeof(symboln), "%sVertex2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2iv) {
        void ** procp = (void **) &disp->Vertex2iv;
        snprintf(symboln, sizeof(symboln), "%sVertex2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2s) {
        void ** procp = (void **) &disp->Vertex2s;
        snprintf(symboln, sizeof(symboln), "%sVertex2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex2sv) {
        void ** procp = (void **) &disp->Vertex2sv;
        snprintf(symboln, sizeof(symboln), "%sVertex2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3d) {
        void ** procp = (void **) &disp->Vertex3d;
        snprintf(symboln, sizeof(symboln), "%sVertex3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3dv) {
        void ** procp = (void **) &disp->Vertex3dv;
        snprintf(symboln, sizeof(symboln), "%sVertex3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3f) {
        void ** procp = (void **) &disp->Vertex3f;
        snprintf(symboln, sizeof(symboln), "%sVertex3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3fv) {
        void ** procp = (void **) &disp->Vertex3fv;
        snprintf(symboln, sizeof(symboln), "%sVertex3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3i) {
        void ** procp = (void **) &disp->Vertex3i;
        snprintf(symboln, sizeof(symboln), "%sVertex3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3iv) {
        void ** procp = (void **) &disp->Vertex3iv;
        snprintf(symboln, sizeof(symboln), "%sVertex3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3s) {
        void ** procp = (void **) &disp->Vertex3s;
        snprintf(symboln, sizeof(symboln), "%sVertex3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex3sv) {
        void ** procp = (void **) &disp->Vertex3sv;
        snprintf(symboln, sizeof(symboln), "%sVertex3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4d) {
        void ** procp = (void **) &disp->Vertex4d;
        snprintf(symboln, sizeof(symboln), "%sVertex4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4dv) {
        void ** procp = (void **) &disp->Vertex4dv;
        snprintf(symboln, sizeof(symboln), "%sVertex4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4f) {
        void ** procp = (void **) &disp->Vertex4f;
        snprintf(symboln, sizeof(symboln), "%sVertex4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4fv) {
        void ** procp = (void **) &disp->Vertex4fv;
        snprintf(symboln, sizeof(symboln), "%sVertex4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4i) {
        void ** procp = (void **) &disp->Vertex4i;
        snprintf(symboln, sizeof(symboln), "%sVertex4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4iv) {
        void ** procp = (void **) &disp->Vertex4iv;
        snprintf(symboln, sizeof(symboln), "%sVertex4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4s) {
        void ** procp = (void **) &disp->Vertex4s;
        snprintf(symboln, sizeof(symboln), "%sVertex4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Vertex4sv) {
        void ** procp = (void **) &disp->Vertex4sv;
        snprintf(symboln, sizeof(symboln), "%sVertex4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClipPlane) {
        void ** procp = (void **) &disp->ClipPlane;
        snprintf(symboln, sizeof(symboln), "%sClipPlane", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorMaterial) {
        void ** procp = (void **) &disp->ColorMaterial;
        snprintf(symboln, sizeof(symboln), "%sColorMaterial", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CullFace) {
        void ** procp = (void **) &disp->CullFace;
        snprintf(symboln, sizeof(symboln), "%sCullFace", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Fogf) {
        void ** procp = (void **) &disp->Fogf;
        snprintf(symboln, sizeof(symboln), "%sFogf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Fogfv) {
        void ** procp = (void **) &disp->Fogfv;
        snprintf(symboln, sizeof(symboln), "%sFogfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Fogi) {
        void ** procp = (void **) &disp->Fogi;
        snprintf(symboln, sizeof(symboln), "%sFogi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Fogiv) {
        void ** procp = (void **) &disp->Fogiv;
        snprintf(symboln, sizeof(symboln), "%sFogiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FrontFace) {
        void ** procp = (void **) &disp->FrontFace;
        snprintf(symboln, sizeof(symboln), "%sFrontFace", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Hint) {
        void ** procp = (void **) &disp->Hint;
        snprintf(symboln, sizeof(symboln), "%sHint", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Lightf) {
        void ** procp = (void **) &disp->Lightf;
        snprintf(symboln, sizeof(symboln), "%sLightf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Lightfv) {
        void ** procp = (void **) &disp->Lightfv;
        snprintf(symboln, sizeof(symboln), "%sLightfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Lighti) {
        void ** procp = (void **) &disp->Lighti;
        snprintf(symboln, sizeof(symboln), "%sLighti", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Lightiv) {
        void ** procp = (void **) &disp->Lightiv;
        snprintf(symboln, sizeof(symboln), "%sLightiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LightModelf) {
        void ** procp = (void **) &disp->LightModelf;
        snprintf(symboln, sizeof(symboln), "%sLightModelf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LightModelfv) {
        void ** procp = (void **) &disp->LightModelfv;
        snprintf(symboln, sizeof(symboln), "%sLightModelfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LightModeli) {
        void ** procp = (void **) &disp->LightModeli;
        snprintf(symboln, sizeof(symboln), "%sLightModeli", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LightModeliv) {
        void ** procp = (void **) &disp->LightModeliv;
        snprintf(symboln, sizeof(symboln), "%sLightModeliv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LineStipple) {
        void ** procp = (void **) &disp->LineStipple;
        snprintf(symboln, sizeof(symboln), "%sLineStipple", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LineWidth) {
        void ** procp = (void **) &disp->LineWidth;
        snprintf(symboln, sizeof(symboln), "%sLineWidth", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Materialf) {
        void ** procp = (void **) &disp->Materialf;
        snprintf(symboln, sizeof(symboln), "%sMaterialf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Materialfv) {
        void ** procp = (void **) &disp->Materialfv;
        snprintf(symboln, sizeof(symboln), "%sMaterialfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Materiali) {
        void ** procp = (void **) &disp->Materiali;
        snprintf(symboln, sizeof(symboln), "%sMateriali", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Materialiv) {
        void ** procp = (void **) &disp->Materialiv;
        snprintf(symboln, sizeof(symboln), "%sMaterialiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointSize) {
        void ** procp = (void **) &disp->PointSize;
        snprintf(symboln, sizeof(symboln), "%sPointSize", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PolygonMode) {
        void ** procp = (void **) &disp->PolygonMode;
        snprintf(symboln, sizeof(symboln), "%sPolygonMode", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PolygonStipple) {
        void ** procp = (void **) &disp->PolygonStipple;
        snprintf(symboln, sizeof(symboln), "%sPolygonStipple", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Scissor) {
        void ** procp = (void **) &disp->Scissor;
        snprintf(symboln, sizeof(symboln), "%sScissor", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ShadeModel) {
        void ** procp = (void **) &disp->ShadeModel;
        snprintf(symboln, sizeof(symboln), "%sShadeModel", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterf) {
        void ** procp = (void **) &disp->TexParameterf;
        snprintf(symboln, sizeof(symboln), "%sTexParameterf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterfv) {
        void ** procp = (void **) &disp->TexParameterfv;
        snprintf(symboln, sizeof(symboln), "%sTexParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameteri) {
        void ** procp = (void **) &disp->TexParameteri;
        snprintf(symboln, sizeof(symboln), "%sTexParameteri", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameteriv) {
        void ** procp = (void **) &disp->TexParameteriv;
        snprintf(symboln, sizeof(symboln), "%sTexParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexImage1D) {
        void ** procp = (void **) &disp->TexImage1D;
        snprintf(symboln, sizeof(symboln), "%sTexImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexImage2D) {
        void ** procp = (void **) &disp->TexImage2D;
        snprintf(symboln, sizeof(symboln), "%sTexImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexEnvf) {
        void ** procp = (void **) &disp->TexEnvf;
        snprintf(symboln, sizeof(symboln), "%sTexEnvf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexEnvfv) {
        void ** procp = (void **) &disp->TexEnvfv;
        snprintf(symboln, sizeof(symboln), "%sTexEnvfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexEnvi) {
        void ** procp = (void **) &disp->TexEnvi;
        snprintf(symboln, sizeof(symboln), "%sTexEnvi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexEnviv) {
        void ** procp = (void **) &disp->TexEnviv;
        snprintf(symboln, sizeof(symboln), "%sTexEnviv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGend) {
        void ** procp = (void **) &disp->TexGend;
        snprintf(symboln, sizeof(symboln), "%sTexGend", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGendv) {
        void ** procp = (void **) &disp->TexGendv;
        snprintf(symboln, sizeof(symboln), "%sTexGendv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGenf) {
        void ** procp = (void **) &disp->TexGenf;
        snprintf(symboln, sizeof(symboln), "%sTexGenf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGenfv) {
        void ** procp = (void **) &disp->TexGenfv;
        snprintf(symboln, sizeof(symboln), "%sTexGenfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGeni) {
        void ** procp = (void **) &disp->TexGeni;
        snprintf(symboln, sizeof(symboln), "%sTexGeni", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexGeniv) {
        void ** procp = (void **) &disp->TexGeniv;
        snprintf(symboln, sizeof(symboln), "%sTexGeniv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FeedbackBuffer) {
        void ** procp = (void **) &disp->FeedbackBuffer;
        snprintf(symboln, sizeof(symboln), "%sFeedbackBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SelectBuffer) {
        void ** procp = (void **) &disp->SelectBuffer;
        snprintf(symboln, sizeof(symboln), "%sSelectBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RenderMode) {
        void ** procp = (void **) &disp->RenderMode;
        snprintf(symboln, sizeof(symboln), "%sRenderMode", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->InitNames) {
        void ** procp = (void **) &disp->InitNames;
        snprintf(symboln, sizeof(symboln), "%sInitNames", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadName) {
        void ** procp = (void **) &disp->LoadName;
        snprintf(symboln, sizeof(symboln), "%sLoadName", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PassThrough) {
        void ** procp = (void **) &disp->PassThrough;
        snprintf(symboln, sizeof(symboln), "%sPassThrough", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PopName) {
        void ** procp = (void **) &disp->PopName;
        snprintf(symboln, sizeof(symboln), "%sPopName", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PushName) {
        void ** procp = (void **) &disp->PushName;
        snprintf(symboln, sizeof(symboln), "%sPushName", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawBuffer) {
        void ** procp = (void **) &disp->DrawBuffer;
        snprintf(symboln, sizeof(symboln), "%sDrawBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Clear) {
        void ** procp = (void **) &disp->Clear;
        snprintf(symboln, sizeof(symboln), "%sClear", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearAccum) {
        void ** procp = (void **) &disp->ClearAccum;
        snprintf(symboln, sizeof(symboln), "%sClearAccum", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearIndex) {
        void ** procp = (void **) &disp->ClearIndex;
        snprintf(symboln, sizeof(symboln), "%sClearIndex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearColor) {
        void ** procp = (void **) &disp->ClearColor;
        snprintf(symboln, sizeof(symboln), "%sClearColor", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearStencil) {
        void ** procp = (void **) &disp->ClearStencil;
        snprintf(symboln, sizeof(symboln), "%sClearStencil", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearDepth) {
        void ** procp = (void **) &disp->ClearDepth;
        snprintf(symboln, sizeof(symboln), "%sClearDepth", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilMask) {
        void ** procp = (void **) &disp->StencilMask;
        snprintf(symboln, sizeof(symboln), "%sStencilMask", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorMask) {
        void ** procp = (void **) &disp->ColorMask;
        snprintf(symboln, sizeof(symboln), "%sColorMask", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DepthMask) {
        void ** procp = (void **) &disp->DepthMask;
        snprintf(symboln, sizeof(symboln), "%sDepthMask", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IndexMask) {
        void ** procp = (void **) &disp->IndexMask;
        snprintf(symboln, sizeof(symboln), "%sIndexMask", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Accum) {
        void ** procp = (void **) &disp->Accum;
        snprintf(symboln, sizeof(symboln), "%sAccum", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Disable) {
        void ** procp = (void **) &disp->Disable;
        snprintf(symboln, sizeof(symboln), "%sDisable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Enable) {
        void ** procp = (void **) &disp->Enable;
        snprintf(symboln, sizeof(symboln), "%sEnable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Finish) {
        void ** procp = (void **) &disp->Finish;
        snprintf(symboln, sizeof(symboln), "%sFinish", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Flush) {
        void ** procp = (void **) &disp->Flush;
        snprintf(symboln, sizeof(symboln), "%sFlush", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PopAttrib) {
        void ** procp = (void **) &disp->PopAttrib;
        snprintf(symboln, sizeof(symboln), "%sPopAttrib", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PushAttrib) {
        void ** procp = (void **) &disp->PushAttrib;
        snprintf(symboln, sizeof(symboln), "%sPushAttrib", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Map1d) {
        void ** procp = (void **) &disp->Map1d;
        snprintf(symboln, sizeof(symboln), "%sMap1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Map1f) {
        void ** procp = (void **) &disp->Map1f;
        snprintf(symboln, sizeof(symboln), "%sMap1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Map2d) {
        void ** procp = (void **) &disp->Map2d;
        snprintf(symboln, sizeof(symboln), "%sMap2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Map2f) {
        void ** procp = (void **) &disp->Map2f;
        snprintf(symboln, sizeof(symboln), "%sMap2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapGrid1d) {
        void ** procp = (void **) &disp->MapGrid1d;
        snprintf(symboln, sizeof(symboln), "%sMapGrid1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapGrid1f) {
        void ** procp = (void **) &disp->MapGrid1f;
        snprintf(symboln, sizeof(symboln), "%sMapGrid1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapGrid2d) {
        void ** procp = (void **) &disp->MapGrid2d;
        snprintf(symboln, sizeof(symboln), "%sMapGrid2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapGrid2f) {
        void ** procp = (void **) &disp->MapGrid2f;
        snprintf(symboln, sizeof(symboln), "%sMapGrid2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1d) {
        void ** procp = (void **) &disp->EvalCoord1d;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1dv) {
        void ** procp = (void **) &disp->EvalCoord1dv;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1f) {
        void ** procp = (void **) &disp->EvalCoord1f;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord1fv) {
        void ** procp = (void **) &disp->EvalCoord1fv;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord1fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2d) {
        void ** procp = (void **) &disp->EvalCoord2d;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2dv) {
        void ** procp = (void **) &disp->EvalCoord2dv;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2f) {
        void ** procp = (void **) &disp->EvalCoord2f;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalCoord2fv) {
        void ** procp = (void **) &disp->EvalCoord2fv;
        snprintf(symboln, sizeof(symboln), "%sEvalCoord2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalMesh1) {
        void ** procp = (void **) &disp->EvalMesh1;
        snprintf(symboln, sizeof(symboln), "%sEvalMesh1", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalPoint1) {
        void ** procp = (void **) &disp->EvalPoint1;
        snprintf(symboln, sizeof(symboln), "%sEvalPoint1", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalMesh2) {
        void ** procp = (void **) &disp->EvalMesh2;
        snprintf(symboln, sizeof(symboln), "%sEvalMesh2", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EvalPoint2) {
        void ** procp = (void **) &disp->EvalPoint2;
        snprintf(symboln, sizeof(symboln), "%sEvalPoint2", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AlphaFunc) {
        void ** procp = (void **) &disp->AlphaFunc;
        snprintf(symboln, sizeof(symboln), "%sAlphaFunc", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFunc) {
        void ** procp = (void **) &disp->BlendFunc;
        snprintf(symboln, sizeof(symboln), "%sBlendFunc", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LogicOp) {
        void ** procp = (void **) &disp->LogicOp;
        snprintf(symboln, sizeof(symboln), "%sLogicOp", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilFunc) {
        void ** procp = (void **) &disp->StencilFunc;
        snprintf(symboln, sizeof(symboln), "%sStencilFunc", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilOp) {
        void ** procp = (void **) &disp->StencilOp;
        snprintf(symboln, sizeof(symboln), "%sStencilOp", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DepthFunc) {
        void ** procp = (void **) &disp->DepthFunc;
        snprintf(symboln, sizeof(symboln), "%sDepthFunc", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelZoom) {
        void ** procp = (void **) &disp->PixelZoom;
        snprintf(symboln, sizeof(symboln), "%sPixelZoom", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTransferf) {
        void ** procp = (void **) &disp->PixelTransferf;
        snprintf(symboln, sizeof(symboln), "%sPixelTransferf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTransferi) {
        void ** procp = (void **) &disp->PixelTransferi;
        snprintf(symboln, sizeof(symboln), "%sPixelTransferi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelStoref) {
        void ** procp = (void **) &disp->PixelStoref;
        snprintf(symboln, sizeof(symboln), "%sPixelStoref", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelStorei) {
        void ** procp = (void **) &disp->PixelStorei;
        snprintf(symboln, sizeof(symboln), "%sPixelStorei", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelMapfv) {
        void ** procp = (void **) &disp->PixelMapfv;
        snprintf(symboln, sizeof(symboln), "%sPixelMapfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelMapuiv) {
        void ** procp = (void **) &disp->PixelMapuiv;
        snprintf(symboln, sizeof(symboln), "%sPixelMapuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelMapusv) {
        void ** procp = (void **) &disp->PixelMapusv;
        snprintf(symboln, sizeof(symboln), "%sPixelMapusv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ReadBuffer) {
        void ** procp = (void **) &disp->ReadBuffer;
        snprintf(symboln, sizeof(symboln), "%sReadBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyPixels) {
        void ** procp = (void **) &disp->CopyPixels;
        snprintf(symboln, sizeof(symboln), "%sCopyPixels", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ReadPixels) {
        void ** procp = (void **) &disp->ReadPixels;
        snprintf(symboln, sizeof(symboln), "%sReadPixels", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawPixels) {
        void ** procp = (void **) &disp->DrawPixels;
        snprintf(symboln, sizeof(symboln), "%sDrawPixels", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBooleanv) {
        void ** procp = (void **) &disp->GetBooleanv;
        snprintf(symboln, sizeof(symboln), "%sGetBooleanv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetClipPlane) {
        void ** procp = (void **) &disp->GetClipPlane;
        snprintf(symboln, sizeof(symboln), "%sGetClipPlane", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetDoublev) {
        void ** procp = (void **) &disp->GetDoublev;
        snprintf(symboln, sizeof(symboln), "%sGetDoublev", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetError) {
        void ** procp = (void **) &disp->GetError;
        snprintf(symboln, sizeof(symboln), "%sGetError", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFloatv) {
        void ** procp = (void **) &disp->GetFloatv;
        snprintf(symboln, sizeof(symboln), "%sGetFloatv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetIntegerv) {
        void ** procp = (void **) &disp->GetIntegerv;
        snprintf(symboln, sizeof(symboln), "%sGetIntegerv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetLightfv) {
        void ** procp = (void **) &disp->GetLightfv;
        snprintf(symboln, sizeof(symboln), "%sGetLightfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetLightiv) {
        void ** procp = (void **) &disp->GetLightiv;
        snprintf(symboln, sizeof(symboln), "%sGetLightiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMapdv) {
        void ** procp = (void **) &disp->GetMapdv;
        snprintf(symboln, sizeof(symboln), "%sGetMapdv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMapfv) {
        void ** procp = (void **) &disp->GetMapfv;
        snprintf(symboln, sizeof(symboln), "%sGetMapfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMapiv) {
        void ** procp = (void **) &disp->GetMapiv;
        snprintf(symboln, sizeof(symboln), "%sGetMapiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMaterialfv) {
        void ** procp = (void **) &disp->GetMaterialfv;
        snprintf(symboln, sizeof(symboln), "%sGetMaterialfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMaterialiv) {
        void ** procp = (void **) &disp->GetMaterialiv;
        snprintf(symboln, sizeof(symboln), "%sGetMaterialiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapfv) {
        void ** procp = (void **) &disp->GetPixelMapfv;
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapuiv) {
        void ** procp = (void **) &disp->GetPixelMapuiv;
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPixelMapusv) {
        void ** procp = (void **) &disp->GetPixelMapusv;
        snprintf(symboln, sizeof(symboln), "%sGetPixelMapusv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPolygonStipple) {
        void ** procp = (void **) &disp->GetPolygonStipple;
        snprintf(symboln, sizeof(symboln), "%sGetPolygonStipple", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetString) {
        void ** procp = (void **) &disp->GetString;
        snprintf(symboln, sizeof(symboln), "%sGetString", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexEnvfv) {
        void ** procp = (void **) &disp->GetTexEnvfv;
        snprintf(symboln, sizeof(symboln), "%sGetTexEnvfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexEnviv) {
        void ** procp = (void **) &disp->GetTexEnviv;
        snprintf(symboln, sizeof(symboln), "%sGetTexEnviv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexGendv) {
        void ** procp = (void **) &disp->GetTexGendv;
        snprintf(symboln, sizeof(symboln), "%sGetTexGendv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexGenfv) {
        void ** procp = (void **) &disp->GetTexGenfv;
        snprintf(symboln, sizeof(symboln), "%sGetTexGenfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexGeniv) {
        void ** procp = (void **) &disp->GetTexGeniv;
        snprintf(symboln, sizeof(symboln), "%sGetTexGeniv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexImage) {
        void ** procp = (void **) &disp->GetTexImage;
        snprintf(symboln, sizeof(symboln), "%sGetTexImage", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterfv) {
        void ** procp = (void **) &disp->GetTexParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameteriv) {
        void ** procp = (void **) &disp->GetTexParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexLevelParameterfv) {
        void ** procp = (void **) &disp->GetTexLevelParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexLevelParameteriv) {
        void ** procp = (void **) &disp->GetTexLevelParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetTexLevelParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsEnabled) {
        void ** procp = (void **) &disp->IsEnabled;
        snprintf(symboln, sizeof(symboln), "%sIsEnabled", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsList) {
        void ** procp = (void **) &disp->IsList;
        snprintf(symboln, sizeof(symboln), "%sIsList", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DepthRange) {
        void ** procp = (void **) &disp->DepthRange;
        snprintf(symboln, sizeof(symboln), "%sDepthRange", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Frustum) {
        void ** procp = (void **) &disp->Frustum;
        snprintf(symboln, sizeof(symboln), "%sFrustum", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadIdentity) {
        void ** procp = (void **) &disp->LoadIdentity;
        snprintf(symboln, sizeof(symboln), "%sLoadIdentity", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadMatrixf) {
        void ** procp = (void **) &disp->LoadMatrixf;
        snprintf(symboln, sizeof(symboln), "%sLoadMatrixf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadMatrixd) {
        void ** procp = (void **) &disp->LoadMatrixd;
        snprintf(symboln, sizeof(symboln), "%sLoadMatrixd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MatrixMode) {
        void ** procp = (void **) &disp->MatrixMode;
        snprintf(symboln, sizeof(symboln), "%sMatrixMode", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultMatrixf) {
        void ** procp = (void **) &disp->MultMatrixf;
        snprintf(symboln, sizeof(symboln), "%sMultMatrixf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultMatrixd) {
        void ** procp = (void **) &disp->MultMatrixd;
        snprintf(symboln, sizeof(symboln), "%sMultMatrixd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Ortho) {
        void ** procp = (void **) &disp->Ortho;
        snprintf(symboln, sizeof(symboln), "%sOrtho", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PopMatrix) {
        void ** procp = (void **) &disp->PopMatrix;
        snprintf(symboln, sizeof(symboln), "%sPopMatrix", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PushMatrix) {
        void ** procp = (void **) &disp->PushMatrix;
        snprintf(symboln, sizeof(symboln), "%sPushMatrix", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rotated) {
        void ** procp = (void **) &disp->Rotated;
        snprintf(symboln, sizeof(symboln), "%sRotated", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Rotatef) {
        void ** procp = (void **) &disp->Rotatef;
        snprintf(symboln, sizeof(symboln), "%sRotatef", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Scaled) {
        void ** procp = (void **) &disp->Scaled;
        snprintf(symboln, sizeof(symboln), "%sScaled", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Scalef) {
        void ** procp = (void **) &disp->Scalef;
        snprintf(symboln, sizeof(symboln), "%sScalef", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Translated) {
        void ** procp = (void **) &disp->Translated;
        snprintf(symboln, sizeof(symboln), "%sTranslated", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Translatef) {
        void ** procp = (void **) &disp->Translatef;
        snprintf(symboln, sizeof(symboln), "%sTranslatef", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Viewport) {
        void ** procp = (void **) &disp->Viewport;
        snprintf(symboln, sizeof(symboln), "%sViewport", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ArrayElement) {
        void ** procp = (void **) &disp->ArrayElement;
        snprintf(symboln, sizeof(symboln), "%sArrayElement", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ArrayElement) {
        void ** procp = (void **) &disp->ArrayElement;
        snprintf(symboln, sizeof(symboln), "%sArrayElementEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindTexture) {
        void ** procp = (void **) &disp->BindTexture;
        snprintf(symboln, sizeof(symboln), "%sBindTexture", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindTexture) {
        void ** procp = (void **) &disp->BindTexture;
        snprintf(symboln, sizeof(symboln), "%sBindTextureEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorPointer) {
        void ** procp = (void **) &disp->ColorPointer;
        snprintf(symboln, sizeof(symboln), "%sColorPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DisableClientState) {
        void ** procp = (void **) &disp->DisableClientState;
        snprintf(symboln, sizeof(symboln), "%sDisableClientState", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawArrays) {
        void ** procp = (void **) &disp->DrawArrays;
        snprintf(symboln, sizeof(symboln), "%sDrawArrays", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawArrays) {
        void ** procp = (void **) &disp->DrawArrays;
        snprintf(symboln, sizeof(symboln), "%sDrawArraysEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElements) {
        void ** procp = (void **) &disp->DrawElements;
        snprintf(symboln, sizeof(symboln), "%sDrawElements", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagPointer) {
        void ** procp = (void **) &disp->EdgeFlagPointer;
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EnableClientState) {
        void ** procp = (void **) &disp->EnableClientState;
        snprintf(symboln, sizeof(symboln), "%sEnableClientState", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IndexPointer) {
        void ** procp = (void **) &disp->IndexPointer;
        snprintf(symboln, sizeof(symboln), "%sIndexPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexub) {
        void ** procp = (void **) &disp->Indexub;
        snprintf(symboln, sizeof(symboln), "%sIndexub", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Indexubv) {
        void ** procp = (void **) &disp->Indexubv;
        snprintf(symboln, sizeof(symboln), "%sIndexubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->InterleavedArrays) {
        void ** procp = (void **) &disp->InterleavedArrays;
        snprintf(symboln, sizeof(symboln), "%sInterleavedArrays", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->NormalPointer) {
        void ** procp = (void **) &disp->NormalPointer;
        snprintf(symboln, sizeof(symboln), "%sNormalPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PolygonOffset) {
        void ** procp = (void **) &disp->PolygonOffset;
        snprintf(symboln, sizeof(symboln), "%sPolygonOffset", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordPointer) {
        void ** procp = (void **) &disp->TexCoordPointer;
        snprintf(symboln, sizeof(symboln), "%sTexCoordPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexPointer) {
        void ** procp = (void **) &disp->VertexPointer;
        snprintf(symboln, sizeof(symboln), "%sVertexPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AreTexturesResident) {
        void ** procp = (void **) &disp->AreTexturesResident;
        snprintf(symboln, sizeof(symboln), "%sAreTexturesResident", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AreTexturesResident) {
        void ** procp = (void **) &disp->AreTexturesResident;
        snprintf(symboln, sizeof(symboln), "%sAreTexturesResidentEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage1D) {
        void ** procp = (void **) &disp->CopyTexImage1D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage1D) {
        void ** procp = (void **) &disp->CopyTexImage1D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage2D) {
        void ** procp = (void **) &disp->CopyTexImage2D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexImage2D) {
        void ** procp = (void **) &disp->CopyTexImage2D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexImage2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage1D) {
        void ** procp = (void **) &disp->CopyTexSubImage1D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage1D) {
        void ** procp = (void **) &disp->CopyTexSubImage1D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage2D) {
        void ** procp = (void **) &disp->CopyTexSubImage2D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage2D) {
        void ** procp = (void **) &disp->CopyTexSubImage2D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteTextures) {
        void ** procp = (void **) &disp->DeleteTextures;
        snprintf(symboln, sizeof(symboln), "%sDeleteTextures", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteTextures) {
        void ** procp = (void **) &disp->DeleteTextures;
        snprintf(symboln, sizeof(symboln), "%sDeleteTexturesEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenTextures) {
        void ** procp = (void **) &disp->GenTextures;
        snprintf(symboln, sizeof(symboln), "%sGenTextures", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenTextures) {
        void ** procp = (void **) &disp->GenTextures;
        snprintf(symboln, sizeof(symboln), "%sGenTexturesEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPointerv) {
        void ** procp = (void **) &disp->GetPointerv;
        snprintf(symboln, sizeof(symboln), "%sGetPointerv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPointerv) {
        void ** procp = (void **) &disp->GetPointerv;
        snprintf(symboln, sizeof(symboln), "%sGetPointervEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsTexture) {
        void ** procp = (void **) &disp->IsTexture;
        snprintf(symboln, sizeof(symboln), "%sIsTexture", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsTexture) {
        void ** procp = (void **) &disp->IsTexture;
        snprintf(symboln, sizeof(symboln), "%sIsTextureEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PrioritizeTextures) {
        void ** procp = (void **) &disp->PrioritizeTextures;
        snprintf(symboln, sizeof(symboln), "%sPrioritizeTextures", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PrioritizeTextures) {
        void ** procp = (void **) &disp->PrioritizeTextures;
        snprintf(symboln, sizeof(symboln), "%sPrioritizeTexturesEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage1D) {
        void ** procp = (void **) &disp->TexSubImage1D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage1D) {
        void ** procp = (void **) &disp->TexSubImage1D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage2D) {
        void ** procp = (void **) &disp->TexSubImage2D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage2D) {
        void ** procp = (void **) &disp->TexSubImage2D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PopClientAttrib) {
        void ** procp = (void **) &disp->PopClientAttrib;
        snprintf(symboln, sizeof(symboln), "%sPopClientAttrib", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PushClientAttrib) {
        void ** procp = (void **) &disp->PushClientAttrib;
        snprintf(symboln, sizeof(symboln), "%sPushClientAttrib", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendColor) {
        void ** procp = (void **) &disp->BlendColor;
        snprintf(symboln, sizeof(symboln), "%sBlendColor", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendColor) {
        void ** procp = (void **) &disp->BlendColor;
        snprintf(symboln, sizeof(symboln), "%sBlendColorEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquation) {
        void ** procp = (void **) &disp->BlendEquation;
        snprintf(symboln, sizeof(symboln), "%sBlendEquation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquation) {
        void ** procp = (void **) &disp->BlendEquation;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElements) {
        void ** procp = (void **) &disp->DrawRangeElements;
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElements", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElements) {
        void ** procp = (void **) &disp->DrawRangeElements;
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        void ** procp = (void **) &disp->ColorTable;
        snprintf(symboln, sizeof(symboln), "%sColorTable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        void ** procp = (void **) &disp->ColorTable;
        snprintf(symboln, sizeof(symboln), "%sColorTableSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTable) {
        void ** procp = (void **) &disp->ColorTable;
        snprintf(symboln, sizeof(symboln), "%sColorTableEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameterfv) {
        void ** procp = (void **) &disp->ColorTableParameterfv;
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameterfv) {
        void ** procp = (void **) &disp->ColorTableParameterfv;
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterfvSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameteriv) {
        void ** procp = (void **) &disp->ColorTableParameteriv;
        snprintf(symboln, sizeof(symboln), "%sColorTableParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorTableParameteriv) {
        void ** procp = (void **) &disp->ColorTableParameteriv;
        snprintf(symboln, sizeof(symboln), "%sColorTableParameterivSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyColorTable) {
        void ** procp = (void **) &disp->CopyColorTable;
        snprintf(symboln, sizeof(symboln), "%sCopyColorTable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyColorTable) {
        void ** procp = (void **) &disp->CopyColorTable;
        snprintf(symboln, sizeof(symboln), "%sCopyColorTableSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        void ** procp = (void **) &disp->GetColorTable;
        snprintf(symboln, sizeof(symboln), "%sGetColorTable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        void ** procp = (void **) &disp->GetColorTable;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTable) {
        void ** procp = (void **) &disp->GetColorTable;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        void ** procp = (void **) &disp->GetColorTableParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        void ** procp = (void **) &disp->GetColorTableParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameterfv) {
        void ** procp = (void **) &disp->GetColorTableParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        void ** procp = (void **) &disp->GetColorTableParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        void ** procp = (void **) &disp->GetColorTableParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivSGI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetColorTableParameteriv) {
        void ** procp = (void **) &disp->GetColorTableParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetColorTableParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorSubTable) {
        void ** procp = (void **) &disp->ColorSubTable;
        snprintf(symboln, sizeof(symboln), "%sColorSubTable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorSubTable) {
        void ** procp = (void **) &disp->ColorSubTable;
        snprintf(symboln, sizeof(symboln), "%sColorSubTableEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyColorSubTable) {
        void ** procp = (void **) &disp->CopyColorSubTable;
        snprintf(symboln, sizeof(symboln), "%sCopyColorSubTable", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyColorSubTable) {
        void ** procp = (void **) &disp->CopyColorSubTable;
        snprintf(symboln, sizeof(symboln), "%sCopyColorSubTableEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter1D) {
        void ** procp = (void **) &disp->ConvolutionFilter1D;
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter1D) {
        void ** procp = (void **) &disp->ConvolutionFilter1D;
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter2D) {
        void ** procp = (void **) &disp->ConvolutionFilter2D;
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionFilter2D) {
        void ** procp = (void **) &disp->ConvolutionFilter2D;
        snprintf(symboln, sizeof(symboln), "%sConvolutionFilter2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterf) {
        void ** procp = (void **) &disp->ConvolutionParameterf;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterf) {
        void ** procp = (void **) &disp->ConvolutionParameterf;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterfv) {
        void ** procp = (void **) &disp->ConvolutionParameterfv;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameterfv) {
        void ** procp = (void **) &disp->ConvolutionParameterfv;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteri) {
        void ** procp = (void **) &disp->ConvolutionParameteri;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteri", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteri) {
        void ** procp = (void **) &disp->ConvolutionParameteri;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteriv) {
        void ** procp = (void **) &disp->ConvolutionParameteriv;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ConvolutionParameteriv) {
        void ** procp = (void **) &disp->ConvolutionParameteriv;
        snprintf(symboln, sizeof(symboln), "%sConvolutionParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter1D) {
        void ** procp = (void **) &disp->CopyConvolutionFilter1D;
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter1D) {
        void ** procp = (void **) &disp->CopyConvolutionFilter1D;
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter2D) {
        void ** procp = (void **) &disp->CopyConvolutionFilter2D;
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyConvolutionFilter2D) {
        void ** procp = (void **) &disp->CopyConvolutionFilter2D;
        snprintf(symboln, sizeof(symboln), "%sCopyConvolutionFilter2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionFilter) {
        void ** procp = (void **) &disp->GetConvolutionFilter;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilter", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionFilter) {
        void ** procp = (void **) &disp->GetConvolutionFilter;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionFilterEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameterfv) {
        void ** procp = (void **) &disp->GetConvolutionParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameterfv) {
        void ** procp = (void **) &disp->GetConvolutionParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameteriv) {
        void ** procp = (void **) &disp->GetConvolutionParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetConvolutionParameteriv) {
        void ** procp = (void **) &disp->GetConvolutionParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetConvolutionParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSeparableFilter) {
        void ** procp = (void **) &disp->GetSeparableFilter;
        snprintf(symboln, sizeof(symboln), "%sGetSeparableFilter", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSeparableFilter) {
        void ** procp = (void **) &disp->GetSeparableFilter;
        snprintf(symboln, sizeof(symboln), "%sGetSeparableFilterEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SeparableFilter2D) {
        void ** procp = (void **) &disp->SeparableFilter2D;
        snprintf(symboln, sizeof(symboln), "%sSeparableFilter2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SeparableFilter2D) {
        void ** procp = (void **) &disp->SeparableFilter2D;
        snprintf(symboln, sizeof(symboln), "%sSeparableFilter2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogram) {
        void ** procp = (void **) &disp->GetHistogram;
        snprintf(symboln, sizeof(symboln), "%sGetHistogram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogram) {
        void ** procp = (void **) &disp->GetHistogram;
        snprintf(symboln, sizeof(symboln), "%sGetHistogramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameterfv) {
        void ** procp = (void **) &disp->GetHistogramParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameterfv) {
        void ** procp = (void **) &disp->GetHistogramParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameteriv) {
        void ** procp = (void **) &disp->GetHistogramParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHistogramParameteriv) {
        void ** procp = (void **) &disp->GetHistogramParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetHistogramParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmax) {
        void ** procp = (void **) &disp->GetMinmax;
        snprintf(symboln, sizeof(symboln), "%sGetMinmax", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmax) {
        void ** procp = (void **) &disp->GetMinmax;
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameterfv) {
        void ** procp = (void **) &disp->GetMinmaxParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameterfv) {
        void ** procp = (void **) &disp->GetMinmaxParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameteriv) {
        void ** procp = (void **) &disp->GetMinmaxParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetMinmaxParameteriv) {
        void ** procp = (void **) &disp->GetMinmaxParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetMinmaxParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Histogram) {
        void ** procp = (void **) &disp->Histogram;
        snprintf(symboln, sizeof(symboln), "%sHistogram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Histogram) {
        void ** procp = (void **) &disp->Histogram;
        snprintf(symboln, sizeof(symboln), "%sHistogramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Minmax) {
        void ** procp = (void **) &disp->Minmax;
        snprintf(symboln, sizeof(symboln), "%sMinmax", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Minmax) {
        void ** procp = (void **) &disp->Minmax;
        snprintf(symboln, sizeof(symboln), "%sMinmaxEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResetHistogram) {
        void ** procp = (void **) &disp->ResetHistogram;
        snprintf(symboln, sizeof(symboln), "%sResetHistogram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResetHistogram) {
        void ** procp = (void **) &disp->ResetHistogram;
        snprintf(symboln, sizeof(symboln), "%sResetHistogramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResetMinmax) {
        void ** procp = (void **) &disp->ResetMinmax;
        snprintf(symboln, sizeof(symboln), "%sResetMinmax", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResetMinmax) {
        void ** procp = (void **) &disp->ResetMinmax;
        snprintf(symboln, sizeof(symboln), "%sResetMinmaxEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexImage3D) {
        void ** procp = (void **) &disp->TexImage3D;
        snprintf(symboln, sizeof(symboln), "%sTexImage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexImage3D) {
        void ** procp = (void **) &disp->TexImage3D;
        snprintf(symboln, sizeof(symboln), "%sTexImage3DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage3D) {
        void ** procp = (void **) &disp->TexSubImage3D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexSubImage3D) {
        void ** procp = (void **) &disp->TexSubImage3D;
        snprintf(symboln, sizeof(symboln), "%sTexSubImage3DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage3D) {
        void ** procp = (void **) &disp->CopyTexSubImage3D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyTexSubImage3D) {
        void ** procp = (void **) &disp->CopyTexSubImage3D;
        snprintf(symboln, sizeof(symboln), "%sCopyTexSubImage3DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ActiveTextureARB) {
        void ** procp = (void **) &disp->ActiveTextureARB;
        snprintf(symboln, sizeof(symboln), "%sActiveTexture", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ActiveTextureARB) {
        void ** procp = (void **) &disp->ActiveTextureARB;
        snprintf(symboln, sizeof(symboln), "%sActiveTextureARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClientActiveTextureARB) {
        void ** procp = (void **) &disp->ClientActiveTextureARB;
        snprintf(symboln, sizeof(symboln), "%sClientActiveTexture", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClientActiveTextureARB) {
        void ** procp = (void **) &disp->ClientActiveTextureARB;
        snprintf(symboln, sizeof(symboln), "%sClientActiveTextureARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dARB) {
        void ** procp = (void **) &disp->MultiTexCoord1dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dARB) {
        void ** procp = (void **) &disp->MultiTexCoord1dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord1dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord1dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fARB) {
        void ** procp = (void **) &disp->MultiTexCoord1fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fARB) {
        void ** procp = (void **) &disp->MultiTexCoord1fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord1fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord1fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1iARB) {
        void ** procp = (void **) &disp->MultiTexCoord1iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1iARB) {
        void ** procp = (void **) &disp->MultiTexCoord1iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord1ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord1ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1sARB) {
        void ** procp = (void **) &disp->MultiTexCoord1sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1sARB) {
        void ** procp = (void **) &disp->MultiTexCoord1sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1svARB) {
        void ** procp = (void **) &disp->MultiTexCoord1svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord1svARB) {
        void ** procp = (void **) &disp->MultiTexCoord1svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord1svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dARB) {
        void ** procp = (void **) &disp->MultiTexCoord2dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dARB) {
        void ** procp = (void **) &disp->MultiTexCoord2dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord2dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord2dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fARB) {
        void ** procp = (void **) &disp->MultiTexCoord2fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fARB) {
        void ** procp = (void **) &disp->MultiTexCoord2fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord2fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord2fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2iARB) {
        void ** procp = (void **) &disp->MultiTexCoord2iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2iARB) {
        void ** procp = (void **) &disp->MultiTexCoord2iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord2ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord2ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2sARB) {
        void ** procp = (void **) &disp->MultiTexCoord2sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2sARB) {
        void ** procp = (void **) &disp->MultiTexCoord2sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2svARB) {
        void ** procp = (void **) &disp->MultiTexCoord2svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord2svARB) {
        void ** procp = (void **) &disp->MultiTexCoord2svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord2svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dARB) {
        void ** procp = (void **) &disp->MultiTexCoord3dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dARB) {
        void ** procp = (void **) &disp->MultiTexCoord3dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord3dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord3dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fARB) {
        void ** procp = (void **) &disp->MultiTexCoord3fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fARB) {
        void ** procp = (void **) &disp->MultiTexCoord3fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord3fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord3fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3iARB) {
        void ** procp = (void **) &disp->MultiTexCoord3iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3iARB) {
        void ** procp = (void **) &disp->MultiTexCoord3iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord3ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord3ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3sARB) {
        void ** procp = (void **) &disp->MultiTexCoord3sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3sARB) {
        void ** procp = (void **) &disp->MultiTexCoord3sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3svARB) {
        void ** procp = (void **) &disp->MultiTexCoord3svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord3svARB) {
        void ** procp = (void **) &disp->MultiTexCoord3svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord3svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dARB) {
        void ** procp = (void **) &disp->MultiTexCoord4dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dARB) {
        void ** procp = (void **) &disp->MultiTexCoord4dARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord4dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4dvARB) {
        void ** procp = (void **) &disp->MultiTexCoord4dvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fARB) {
        void ** procp = (void **) &disp->MultiTexCoord4fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fARB) {
        void ** procp = (void **) &disp->MultiTexCoord4fARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord4fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4fvARB) {
        void ** procp = (void **) &disp->MultiTexCoord4fvARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4iARB) {
        void ** procp = (void **) &disp->MultiTexCoord4iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4iARB) {
        void ** procp = (void **) &disp->MultiTexCoord4iARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord4ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4ivARB) {
        void ** procp = (void **) &disp->MultiTexCoord4ivARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4sARB) {
        void ** procp = (void **) &disp->MultiTexCoord4sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4sARB) {
        void ** procp = (void **) &disp->MultiTexCoord4sARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4svARB) {
        void ** procp = (void **) &disp->MultiTexCoord4svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoord4svARB) {
        void ** procp = (void **) &disp->MultiTexCoord4svARB;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoord4svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AttachShader) {
        void ** procp = (void **) &disp->AttachShader;
        snprintf(symboln, sizeof(symboln), "%sAttachShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CreateProgram) {
        void ** procp = (void **) &disp->CreateProgram;
        snprintf(symboln, sizeof(symboln), "%sCreateProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CreateShader) {
        void ** procp = (void **) &disp->CreateShader;
        snprintf(symboln, sizeof(symboln), "%sCreateShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteProgram) {
        void ** procp = (void **) &disp->DeleteProgram;
        snprintf(symboln, sizeof(symboln), "%sDeleteProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteShader) {
        void ** procp = (void **) &disp->DeleteShader;
        snprintf(symboln, sizeof(symboln), "%sDeleteShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DetachShader) {
        void ** procp = (void **) &disp->DetachShader;
        snprintf(symboln, sizeof(symboln), "%sDetachShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetAttachedShaders) {
        void ** procp = (void **) &disp->GetAttachedShaders;
        snprintf(symboln, sizeof(symboln), "%sGetAttachedShaders", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramInfoLog) {
        void ** procp = (void **) &disp->GetProgramInfoLog;
        snprintf(symboln, sizeof(symboln), "%sGetProgramInfoLog", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramiv) {
        void ** procp = (void **) &disp->GetProgramiv;
        snprintf(symboln, sizeof(symboln), "%sGetProgramiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetShaderInfoLog) {
        void ** procp = (void **) &disp->GetShaderInfoLog;
        snprintf(symboln, sizeof(symboln), "%sGetShaderInfoLog", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetShaderiv) {
        void ** procp = (void **) &disp->GetShaderiv;
        snprintf(symboln, sizeof(symboln), "%sGetShaderiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsProgram) {
        void ** procp = (void **) &disp->IsProgram;
        snprintf(symboln, sizeof(symboln), "%sIsProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsShader) {
        void ** procp = (void **) &disp->IsShader;
        snprintf(symboln, sizeof(symboln), "%sIsShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilFuncSeparate) {
        void ** procp = (void **) &disp->StencilFuncSeparate;
        snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparate", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilMaskSeparate) {
        void ** procp = (void **) &disp->StencilMaskSeparate;
        snprintf(symboln, sizeof(symboln), "%sStencilMaskSeparate", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilOpSeparate) {
        void ** procp = (void **) &disp->StencilOpSeparate;
        snprintf(symboln, sizeof(symboln), "%sStencilOpSeparate", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilOpSeparate) {
        void ** procp = (void **) &disp->StencilOpSeparate;
        snprintf(symboln, sizeof(symboln), "%sStencilOpSeparateATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2x3fv) {
        void ** procp = (void **) &disp->UniformMatrix2x3fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2x4fv) {
        void ** procp = (void **) &disp->UniformMatrix2x4fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2x4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3x2fv) {
        void ** procp = (void **) &disp->UniformMatrix3x2fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3x4fv) {
        void ** procp = (void **) &disp->UniformMatrix3x4fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3x4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4x2fv) {
        void ** procp = (void **) &disp->UniformMatrix4x2fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4x3fv) {
        void ** procp = (void **) &disp->UniformMatrix4x3fv;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4x3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClampColor) {
        void ** procp = (void **) &disp->ClampColor;
        snprintf(symboln, sizeof(symboln), "%sClampColor", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearBufferfi) {
        void ** procp = (void **) &disp->ClearBufferfi;
        snprintf(symboln, sizeof(symboln), "%sClearBufferfi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearBufferfv) {
        void ** procp = (void **) &disp->ClearBufferfv;
        snprintf(symboln, sizeof(symboln), "%sClearBufferfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearBufferiv) {
        void ** procp = (void **) &disp->ClearBufferiv;
        snprintf(symboln, sizeof(symboln), "%sClearBufferiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearBufferuiv) {
        void ** procp = (void **) &disp->ClearBufferuiv;
        snprintf(symboln, sizeof(symboln), "%sClearBufferuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetStringi) {
        void ** procp = (void **) &disp->GetStringi;
        snprintf(symboln, sizeof(symboln), "%sGetStringi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexBuffer) {
        void ** procp = (void **) &disp->TexBuffer;
        snprintf(symboln, sizeof(symboln), "%sTexBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture) {
        void ** procp = (void **) &disp->FramebufferTexture;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameteri64v) {
        void ** procp = (void **) &disp->GetBufferParameteri64v;
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameteri64v", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetInteger64i_v) {
        void ** procp = (void **) &disp->GetInteger64i_v;
        snprintf(symboln, sizeof(symboln), "%sGetInteger64i_v", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribDivisor) {
        void ** procp = (void **) &disp->VertexAttribDivisor;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisor", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixdARB) {
        void ** procp = (void **) &disp->LoadTransposeMatrixdARB;
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixdARB) {
        void ** procp = (void **) &disp->LoadTransposeMatrixdARB;
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixdARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixfARB) {
        void ** procp = (void **) &disp->LoadTransposeMatrixfARB;
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadTransposeMatrixfARB) {
        void ** procp = (void **) &disp->LoadTransposeMatrixfARB;
        snprintf(symboln, sizeof(symboln), "%sLoadTransposeMatrixfARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixdARB) {
        void ** procp = (void **) &disp->MultTransposeMatrixdARB;
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixdARB) {
        void ** procp = (void **) &disp->MultTransposeMatrixdARB;
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixdARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixfARB) {
        void ** procp = (void **) &disp->MultTransposeMatrixfARB;
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultTransposeMatrixfARB) {
        void ** procp = (void **) &disp->MultTransposeMatrixfARB;
        snprintf(symboln, sizeof(symboln), "%sMultTransposeMatrixfARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SampleCoverageARB) {
        void ** procp = (void **) &disp->SampleCoverageARB;
        snprintf(symboln, sizeof(symboln), "%sSampleCoverage", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SampleCoverageARB) {
        void ** procp = (void **) &disp->SampleCoverageARB;
        snprintf(symboln, sizeof(symboln), "%sSampleCoverageARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage1DARB) {
        void ** procp = (void **) &disp->CompressedTexImage1DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage1DARB) {
        void ** procp = (void **) &disp->CompressedTexImage1DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage1DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage2DARB) {
        void ** procp = (void **) &disp->CompressedTexImage2DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage2DARB) {
        void ** procp = (void **) &disp->CompressedTexImage2DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage2DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage3DARB) {
        void ** procp = (void **) &disp->CompressedTexImage3DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexImage3DARB) {
        void ** procp = (void **) &disp->CompressedTexImage3DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexImage3DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage1DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage1DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage1DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage1DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage1DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage2DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage2DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage2DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage2DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage2DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage3DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage3DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompressedTexSubImage3DARB) {
        void ** procp = (void **) &disp->CompressedTexSubImage3DARB;
        snprintf(symboln, sizeof(symboln), "%sCompressedTexSubImage3DARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCompressedTexImageARB) {
        void ** procp = (void **) &disp->GetCompressedTexImageARB;
        snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImage", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCompressedTexImageARB) {
        void ** procp = (void **) &disp->GetCompressedTexImageARB;
        snprintf(symboln, sizeof(symboln), "%sGetCompressedTexImageARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DisableVertexAttribArrayARB) {
        void ** procp = (void **) &disp->DisableVertexAttribArrayARB;
        snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArray", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DisableVertexAttribArrayARB) {
        void ** procp = (void **) &disp->DisableVertexAttribArrayARB;
        snprintf(symboln, sizeof(symboln), "%sDisableVertexAttribArrayARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EnableVertexAttribArrayARB) {
        void ** procp = (void **) &disp->EnableVertexAttribArrayARB;
        snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArray", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EnableVertexAttribArrayARB) {
        void ** procp = (void **) &disp->EnableVertexAttribArrayARB;
        snprintf(symboln, sizeof(symboln), "%sEnableVertexAttribArrayARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramEnvParameterdvARB) {
        void ** procp = (void **) &disp->GetProgramEnvParameterdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterdvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramEnvParameterfvARB) {
        void ** procp = (void **) &disp->GetProgramEnvParameterfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramEnvParameterfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramLocalParameterdvARB) {
        void ** procp = (void **) &disp->GetProgramLocalParameterdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterdvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramLocalParameterfvARB) {
        void ** procp = (void **) &disp->GetProgramLocalParameterfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramLocalParameterfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramStringARB) {
        void ** procp = (void **) &disp->GetProgramStringARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramStringARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramivARB) {
        void ** procp = (void **) &disp->GetProgramivARB;
        snprintf(symboln, sizeof(symboln), "%sGetProgramivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvARB) {
        void ** procp = (void **) &disp->GetVertexAttribdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvARB) {
        void ** procp = (void **) &disp->GetVertexAttribdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvARB) {
        void ** procp = (void **) &disp->GetVertexAttribfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvARB) {
        void ** procp = (void **) &disp->GetVertexAttribfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivARB) {
        void ** procp = (void **) &disp->GetVertexAttribivARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivARB) {
        void ** procp = (void **) &disp->GetVertexAttribivARB;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4dARB;
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4dARB;
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dvARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4dvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4dvARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4dvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4fARB;
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4fARB;
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fvARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4fvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameter4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameter4fvARB) {
        void ** procp = (void **) &disp->ProgramEnvParameter4fvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramParameter4fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4dARB) {
        void ** procp = (void **) &disp->ProgramLocalParameter4dARB;
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4dvARB) {
        void ** procp = (void **) &disp->ProgramLocalParameter4dvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4fARB) {
        void ** procp = (void **) &disp->ProgramLocalParameter4fARB;
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameter4fvARB) {
        void ** procp = (void **) &disp->ProgramLocalParameter4fvARB;
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameter4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramStringARB) {
        void ** procp = (void **) &disp->ProgramStringARB;
        snprintf(symboln, sizeof(symboln), "%sProgramStringARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dARB) {
        void ** procp = (void **) &disp->VertexAttrib1dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dARB) {
        void ** procp = (void **) &disp->VertexAttrib1dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvARB) {
        void ** procp = (void **) &disp->VertexAttrib1dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvARB) {
        void ** procp = (void **) &disp->VertexAttrib1dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fARB) {
        void ** procp = (void **) &disp->VertexAttrib1fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fARB) {
        void ** procp = (void **) &disp->VertexAttrib1fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvARB) {
        void ** procp = (void **) &disp->VertexAttrib1fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvARB) {
        void ** procp = (void **) &disp->VertexAttrib1fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sARB) {
        void ** procp = (void **) &disp->VertexAttrib1sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sARB) {
        void ** procp = (void **) &disp->VertexAttrib1sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svARB) {
        void ** procp = (void **) &disp->VertexAttrib1svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svARB) {
        void ** procp = (void **) &disp->VertexAttrib1svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dARB) {
        void ** procp = (void **) &disp->VertexAttrib2dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dARB) {
        void ** procp = (void **) &disp->VertexAttrib2dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvARB) {
        void ** procp = (void **) &disp->VertexAttrib2dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvARB) {
        void ** procp = (void **) &disp->VertexAttrib2dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fARB) {
        void ** procp = (void **) &disp->VertexAttrib2fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fARB) {
        void ** procp = (void **) &disp->VertexAttrib2fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvARB) {
        void ** procp = (void **) &disp->VertexAttrib2fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvARB) {
        void ** procp = (void **) &disp->VertexAttrib2fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sARB) {
        void ** procp = (void **) &disp->VertexAttrib2sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sARB) {
        void ** procp = (void **) &disp->VertexAttrib2sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svARB) {
        void ** procp = (void **) &disp->VertexAttrib2svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svARB) {
        void ** procp = (void **) &disp->VertexAttrib2svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dARB) {
        void ** procp = (void **) &disp->VertexAttrib3dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dARB) {
        void ** procp = (void **) &disp->VertexAttrib3dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvARB) {
        void ** procp = (void **) &disp->VertexAttrib3dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvARB) {
        void ** procp = (void **) &disp->VertexAttrib3dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fARB) {
        void ** procp = (void **) &disp->VertexAttrib3fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fARB) {
        void ** procp = (void **) &disp->VertexAttrib3fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvARB) {
        void ** procp = (void **) &disp->VertexAttrib3fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvARB) {
        void ** procp = (void **) &disp->VertexAttrib3fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sARB) {
        void ** procp = (void **) &disp->VertexAttrib3sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sARB) {
        void ** procp = (void **) &disp->VertexAttrib3sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svARB) {
        void ** procp = (void **) &disp->VertexAttrib3svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svARB) {
        void ** procp = (void **) &disp->VertexAttrib3svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NbvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NbvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nbv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NbvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NbvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NbvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NivARB) {
        void ** procp = (void **) &disp->VertexAttrib4NivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Niv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NivARB) {
        void ** procp = (void **) &disp->VertexAttrib4NivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NsvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NsvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nsv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NsvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NsvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NsvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubARB) {
        void ** procp = (void **) &disp->VertexAttrib4NubARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nub", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubARB) {
        void ** procp = (void **) &disp->VertexAttrib4NubARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NubvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NubvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NubvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NubvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NuivARB) {
        void ** procp = (void **) &disp->VertexAttrib4NuivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NuivARB) {
        void ** procp = (void **) &disp->VertexAttrib4NuivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NuivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NusvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NusvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4Nusv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4NusvARB) {
        void ** procp = (void **) &disp->VertexAttrib4NusvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4NusvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4bvARB) {
        void ** procp = (void **) &disp->VertexAttrib4bvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4bvARB) {
        void ** procp = (void **) &disp->VertexAttrib4bvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4bvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dARB) {
        void ** procp = (void **) &disp->VertexAttrib4dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dARB) {
        void ** procp = (void **) &disp->VertexAttrib4dARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvARB) {
        void ** procp = (void **) &disp->VertexAttrib4dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvARB) {
        void ** procp = (void **) &disp->VertexAttrib4dvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fARB) {
        void ** procp = (void **) &disp->VertexAttrib4fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fARB) {
        void ** procp = (void **) &disp->VertexAttrib4fARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvARB) {
        void ** procp = (void **) &disp->VertexAttrib4fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvARB) {
        void ** procp = (void **) &disp->VertexAttrib4fvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ivARB) {
        void ** procp = (void **) &disp->VertexAttrib4ivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ivARB) {
        void ** procp = (void **) &disp->VertexAttrib4ivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sARB) {
        void ** procp = (void **) &disp->VertexAttrib4sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sARB) {
        void ** procp = (void **) &disp->VertexAttrib4sARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svARB) {
        void ** procp = (void **) &disp->VertexAttrib4svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svARB) {
        void ** procp = (void **) &disp->VertexAttrib4svARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvARB) {
        void ** procp = (void **) &disp->VertexAttrib4ubvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvARB) {
        void ** procp = (void **) &disp->VertexAttrib4ubvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4uivARB) {
        void ** procp = (void **) &disp->VertexAttrib4uivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4uivARB) {
        void ** procp = (void **) &disp->VertexAttrib4uivARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4uivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4usvARB) {
        void ** procp = (void **) &disp->VertexAttrib4usvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4usvARB) {
        void ** procp = (void **) &disp->VertexAttrib4usvARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4usvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerARB) {
        void ** procp = (void **) &disp->VertexAttribPointerARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerARB) {
        void ** procp = (void **) &disp->VertexAttribPointerARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferARB) {
        void ** procp = (void **) &disp->BindBufferARB;
        snprintf(symboln, sizeof(symboln), "%sBindBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferARB) {
        void ** procp = (void **) &disp->BindBufferARB;
        snprintf(symboln, sizeof(symboln), "%sBindBufferARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BufferDataARB) {
        void ** procp = (void **) &disp->BufferDataARB;
        snprintf(symboln, sizeof(symboln), "%sBufferData", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BufferDataARB) {
        void ** procp = (void **) &disp->BufferDataARB;
        snprintf(symboln, sizeof(symboln), "%sBufferDataARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BufferSubDataARB) {
        void ** procp = (void **) &disp->BufferSubDataARB;
        snprintf(symboln, sizeof(symboln), "%sBufferSubData", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BufferSubDataARB) {
        void ** procp = (void **) &disp->BufferSubDataARB;
        snprintf(symboln, sizeof(symboln), "%sBufferSubDataARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteBuffersARB) {
        void ** procp = (void **) &disp->DeleteBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDeleteBuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteBuffersARB) {
        void ** procp = (void **) &disp->DeleteBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDeleteBuffersARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenBuffersARB) {
        void ** procp = (void **) &disp->GenBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sGenBuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenBuffersARB) {
        void ** procp = (void **) &disp->GenBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sGenBuffersARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameterivARB) {
        void ** procp = (void **) &disp->GetBufferParameterivARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferParameterivARB) {
        void ** procp = (void **) &disp->GetBufferParameterivARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferParameterivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferPointervARB) {
        void ** procp = (void **) &disp->GetBufferPointervARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferPointerv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferPointervARB) {
        void ** procp = (void **) &disp->GetBufferPointervARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferPointervARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferSubDataARB) {
        void ** procp = (void **) &disp->GetBufferSubDataARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferSubData", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBufferSubDataARB) {
        void ** procp = (void **) &disp->GetBufferSubDataARB;
        snprintf(symboln, sizeof(symboln), "%sGetBufferSubDataARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsBufferARB) {
        void ** procp = (void **) &disp->IsBufferARB;
        snprintf(symboln, sizeof(symboln), "%sIsBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsBufferARB) {
        void ** procp = (void **) &disp->IsBufferARB;
        snprintf(symboln, sizeof(symboln), "%sIsBufferARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapBufferARB) {
        void ** procp = (void **) &disp->MapBufferARB;
        snprintf(symboln, sizeof(symboln), "%sMapBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapBufferARB) {
        void ** procp = (void **) &disp->MapBufferARB;
        snprintf(symboln, sizeof(symboln), "%sMapBufferARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UnmapBufferARB) {
        void ** procp = (void **) &disp->UnmapBufferARB;
        snprintf(symboln, sizeof(symboln), "%sUnmapBuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UnmapBufferARB) {
        void ** procp = (void **) &disp->UnmapBufferARB;
        snprintf(symboln, sizeof(symboln), "%sUnmapBufferARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginQueryARB) {
        void ** procp = (void **) &disp->BeginQueryARB;
        snprintf(symboln, sizeof(symboln), "%sBeginQuery", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginQueryARB) {
        void ** procp = (void **) &disp->BeginQueryARB;
        snprintf(symboln, sizeof(symboln), "%sBeginQueryARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteQueriesARB) {
        void ** procp = (void **) &disp->DeleteQueriesARB;
        snprintf(symboln, sizeof(symboln), "%sDeleteQueries", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteQueriesARB) {
        void ** procp = (void **) &disp->DeleteQueriesARB;
        snprintf(symboln, sizeof(symboln), "%sDeleteQueriesARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndQueryARB) {
        void ** procp = (void **) &disp->EndQueryARB;
        snprintf(symboln, sizeof(symboln), "%sEndQuery", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndQueryARB) {
        void ** procp = (void **) &disp->EndQueryARB;
        snprintf(symboln, sizeof(symboln), "%sEndQueryARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenQueriesARB) {
        void ** procp = (void **) &disp->GenQueriesARB;
        snprintf(symboln, sizeof(symboln), "%sGenQueries", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenQueriesARB) {
        void ** procp = (void **) &disp->GenQueriesARB;
        snprintf(symboln, sizeof(symboln), "%sGenQueriesARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectivARB) {
        void ** procp = (void **) &disp->GetQueryObjectivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectivARB) {
        void ** procp = (void **) &disp->GetQueryObjectivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectuivARB) {
        void ** procp = (void **) &disp->GetQueryObjectuivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectuivARB) {
        void ** procp = (void **) &disp->GetQueryObjectuivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectuivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryivARB) {
        void ** procp = (void **) &disp->GetQueryivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryivARB) {
        void ** procp = (void **) &disp->GetQueryivARB;
        snprintf(symboln, sizeof(symboln), "%sGetQueryivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsQueryARB) {
        void ** procp = (void **) &disp->IsQueryARB;
        snprintf(symboln, sizeof(symboln), "%sIsQuery", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsQueryARB) {
        void ** procp = (void **) &disp->IsQueryARB;
        snprintf(symboln, sizeof(symboln), "%sIsQueryARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AttachObjectARB) {
        void ** procp = (void **) &disp->AttachObjectARB;
        snprintf(symboln, sizeof(symboln), "%sAttachObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompileShaderARB) {
        void ** procp = (void **) &disp->CompileShaderARB;
        snprintf(symboln, sizeof(symboln), "%sCompileShader", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CompileShaderARB) {
        void ** procp = (void **) &disp->CompileShaderARB;
        snprintf(symboln, sizeof(symboln), "%sCompileShaderARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CreateProgramObjectARB) {
        void ** procp = (void **) &disp->CreateProgramObjectARB;
        snprintf(symboln, sizeof(symboln), "%sCreateProgramObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CreateShaderObjectARB) {
        void ** procp = (void **) &disp->CreateShaderObjectARB;
        snprintf(symboln, sizeof(symboln), "%sCreateShaderObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteObjectARB) {
        void ** procp = (void **) &disp->DeleteObjectARB;
        snprintf(symboln, sizeof(symboln), "%sDeleteObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DetachObjectARB) {
        void ** procp = (void **) &disp->DetachObjectARB;
        snprintf(symboln, sizeof(symboln), "%sDetachObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetActiveUniformARB) {
        void ** procp = (void **) &disp->GetActiveUniformARB;
        snprintf(symboln, sizeof(symboln), "%sGetActiveUniform", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetActiveUniformARB) {
        void ** procp = (void **) &disp->GetActiveUniformARB;
        snprintf(symboln, sizeof(symboln), "%sGetActiveUniformARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetAttachedObjectsARB) {
        void ** procp = (void **) &disp->GetAttachedObjectsARB;
        snprintf(symboln, sizeof(symboln), "%sGetAttachedObjectsARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetHandleARB) {
        void ** procp = (void **) &disp->GetHandleARB;
        snprintf(symboln, sizeof(symboln), "%sGetHandleARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetInfoLogARB) {
        void ** procp = (void **) &disp->GetInfoLogARB;
        snprintf(symboln, sizeof(symboln), "%sGetInfoLogARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterfvARB) {
        void ** procp = (void **) &disp->GetObjectParameterfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterivARB) {
        void ** procp = (void **) &disp->GetObjectParameterivARB;
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetShaderSourceARB) {
        void ** procp = (void **) &disp->GetShaderSourceARB;
        snprintf(symboln, sizeof(symboln), "%sGetShaderSource", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetShaderSourceARB) {
        void ** procp = (void **) &disp->GetShaderSourceARB;
        snprintf(symboln, sizeof(symboln), "%sGetShaderSourceARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformLocationARB) {
        void ** procp = (void **) &disp->GetUniformLocationARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformLocation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformLocationARB) {
        void ** procp = (void **) &disp->GetUniformLocationARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformLocationARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformfvARB) {
        void ** procp = (void **) &disp->GetUniformfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformfvARB) {
        void ** procp = (void **) &disp->GetUniformfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformivARB) {
        void ** procp = (void **) &disp->GetUniformivARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformivARB) {
        void ** procp = (void **) &disp->GetUniformivARB;
        snprintf(symboln, sizeof(symboln), "%sGetUniformivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LinkProgramARB) {
        void ** procp = (void **) &disp->LinkProgramARB;
        snprintf(symboln, sizeof(symboln), "%sLinkProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LinkProgramARB) {
        void ** procp = (void **) &disp->LinkProgramARB;
        snprintf(symboln, sizeof(symboln), "%sLinkProgramARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ShaderSourceARB) {
        void ** procp = (void **) &disp->ShaderSourceARB;
        snprintf(symboln, sizeof(symboln), "%sShaderSource", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ShaderSourceARB) {
        void ** procp = (void **) &disp->ShaderSourceARB;
        snprintf(symboln, sizeof(symboln), "%sShaderSourceARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1fARB) {
        void ** procp = (void **) &disp->Uniform1fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1fARB) {
        void ** procp = (void **) &disp->Uniform1fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1fvARB) {
        void ** procp = (void **) &disp->Uniform1fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1fvARB) {
        void ** procp = (void **) &disp->Uniform1fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1iARB) {
        void ** procp = (void **) &disp->Uniform1iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1iARB) {
        void ** procp = (void **) &disp->Uniform1iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1ivARB) {
        void ** procp = (void **) &disp->Uniform1ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1ivARB) {
        void ** procp = (void **) &disp->Uniform1ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform1ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2fARB) {
        void ** procp = (void **) &disp->Uniform2fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2fARB) {
        void ** procp = (void **) &disp->Uniform2fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2fvARB) {
        void ** procp = (void **) &disp->Uniform2fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2fvARB) {
        void ** procp = (void **) &disp->Uniform2fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2iARB) {
        void ** procp = (void **) &disp->Uniform2iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2iARB) {
        void ** procp = (void **) &disp->Uniform2iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2ivARB) {
        void ** procp = (void **) &disp->Uniform2ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2ivARB) {
        void ** procp = (void **) &disp->Uniform2ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform2ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3fARB) {
        void ** procp = (void **) &disp->Uniform3fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3fARB) {
        void ** procp = (void **) &disp->Uniform3fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3fvARB) {
        void ** procp = (void **) &disp->Uniform3fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3fvARB) {
        void ** procp = (void **) &disp->Uniform3fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3iARB) {
        void ** procp = (void **) &disp->Uniform3iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3iARB) {
        void ** procp = (void **) &disp->Uniform3iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3ivARB) {
        void ** procp = (void **) &disp->Uniform3ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3ivARB) {
        void ** procp = (void **) &disp->Uniform3ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform3ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4fARB) {
        void ** procp = (void **) &disp->Uniform4fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4fARB) {
        void ** procp = (void **) &disp->Uniform4fARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4fvARB) {
        void ** procp = (void **) &disp->Uniform4fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4fvARB) {
        void ** procp = (void **) &disp->Uniform4fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4iARB) {
        void ** procp = (void **) &disp->Uniform4iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4iARB) {
        void ** procp = (void **) &disp->Uniform4iARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4ivARB) {
        void ** procp = (void **) &disp->Uniform4ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4ivARB) {
        void ** procp = (void **) &disp->Uniform4ivARB;
        snprintf(symboln, sizeof(symboln), "%sUniform4ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2fvARB) {
        void ** procp = (void **) &disp->UniformMatrix2fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix2fvARB) {
        void ** procp = (void **) &disp->UniformMatrix2fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix2fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3fvARB) {
        void ** procp = (void **) &disp->UniformMatrix3fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix3fvARB) {
        void ** procp = (void **) &disp->UniformMatrix3fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix3fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4fvARB) {
        void ** procp = (void **) &disp->UniformMatrix4fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UniformMatrix4fvARB) {
        void ** procp = (void **) &disp->UniformMatrix4fvARB;
        snprintf(symboln, sizeof(symboln), "%sUniformMatrix4fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UseProgramObjectARB) {
        void ** procp = (void **) &disp->UseProgramObjectARB;
        snprintf(symboln, sizeof(symboln), "%sUseProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UseProgramObjectARB) {
        void ** procp = (void **) &disp->UseProgramObjectARB;
        snprintf(symboln, sizeof(symboln), "%sUseProgramObjectARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ValidateProgramARB) {
        void ** procp = (void **) &disp->ValidateProgramARB;
        snprintf(symboln, sizeof(symboln), "%sValidateProgram", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ValidateProgramARB) {
        void ** procp = (void **) &disp->ValidateProgramARB;
        snprintf(symboln, sizeof(symboln), "%sValidateProgramARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindAttribLocationARB) {
        void ** procp = (void **) &disp->BindAttribLocationARB;
        snprintf(symboln, sizeof(symboln), "%sBindAttribLocation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindAttribLocationARB) {
        void ** procp = (void **) &disp->BindAttribLocationARB;
        snprintf(symboln, sizeof(symboln), "%sBindAttribLocationARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetActiveAttribARB) {
        void ** procp = (void **) &disp->GetActiveAttribARB;
        snprintf(symboln, sizeof(symboln), "%sGetActiveAttrib", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetActiveAttribARB) {
        void ** procp = (void **) &disp->GetActiveAttribARB;
        snprintf(symboln, sizeof(symboln), "%sGetActiveAttribARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetAttribLocationARB) {
        void ** procp = (void **) &disp->GetAttribLocationARB;
        snprintf(symboln, sizeof(symboln), "%sGetAttribLocation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetAttribLocationARB) {
        void ** procp = (void **) &disp->GetAttribLocationARB;
        snprintf(symboln, sizeof(symboln), "%sGetAttribLocationARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        void ** procp = (void **) &disp->DrawBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDrawBuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        void ** procp = (void **) &disp->DrawBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDrawBuffersARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        void ** procp = (void **) &disp->DrawBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDrawBuffersATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawBuffersARB) {
        void ** procp = (void **) &disp->DrawBuffersARB;
        snprintf(symboln, sizeof(symboln), "%sDrawBuffersNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClampColorARB) {
        void ** procp = (void **) &disp->ClampColorARB;
        snprintf(symboln, sizeof(symboln), "%sClampColorARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        void ** procp = (void **) &disp->DrawArraysInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        void ** procp = (void **) &disp->DrawArraysInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstancedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawArraysInstancedARB) {
        void ** procp = (void **) &disp->DrawArraysInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawArraysInstanced", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        void ** procp = (void **) &disp->DrawElementsInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        void ** procp = (void **) &disp->DrawElementsInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedARB) {
        void ** procp = (void **) &disp->DrawElementsInstancedARB;
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstanced", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageMultisample) {
        void ** procp = (void **) &disp->RenderbufferStorageMultisample;
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisample", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageMultisample) {
        void ** procp = (void **) &disp->RenderbufferStorageMultisample;
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageMultisampleEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureARB) {
        void ** procp = (void **) &disp->FramebufferTextureARB;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureFaceARB) {
        void ** procp = (void **) &disp->FramebufferTextureFaceARB;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureFaceARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramParameteriARB) {
        void ** procp = (void **) &disp->ProgramParameteriARB;
        snprintf(symboln, sizeof(symboln), "%sProgramParameteriARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribDivisorARB) {
        void ** procp = (void **) &disp->VertexAttribDivisorARB;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribDivisorARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FlushMappedBufferRange) {
        void ** procp = (void **) &disp->FlushMappedBufferRange;
        snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRange", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MapBufferRange) {
        void ** procp = (void **) &disp->MapBufferRange;
        snprintf(symboln, sizeof(symboln), "%sMapBufferRange", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexBufferARB) {
        void ** procp = (void **) &disp->TexBufferARB;
        snprintf(symboln, sizeof(symboln), "%sTexBufferARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindVertexArray) {
        void ** procp = (void **) &disp->BindVertexArray;
        snprintf(symboln, sizeof(symboln), "%sBindVertexArray", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenVertexArrays) {
        void ** procp = (void **) &disp->GenVertexArrays;
        snprintf(symboln, sizeof(symboln), "%sGenVertexArrays", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CopyBufferSubData) {
        void ** procp = (void **) &disp->CopyBufferSubData;
        snprintf(symboln, sizeof(symboln), "%sCopyBufferSubData", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClientWaitSync) {
        void ** procp = (void **) &disp->ClientWaitSync;
        snprintf(symboln, sizeof(symboln), "%sClientWaitSync", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteSync) {
        void ** procp = (void **) &disp->DeleteSync;
        snprintf(symboln, sizeof(symboln), "%sDeleteSync", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FenceSync) {
        void ** procp = (void **) &disp->FenceSync;
        snprintf(symboln, sizeof(symboln), "%sFenceSync", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetInteger64v) {
        void ** procp = (void **) &disp->GetInteger64v;
        snprintf(symboln, sizeof(symboln), "%sGetInteger64v", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSynciv) {
        void ** procp = (void **) &disp->GetSynciv;
        snprintf(symboln, sizeof(symboln), "%sGetSynciv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsSync) {
        void ** procp = (void **) &disp->IsSync;
        snprintf(symboln, sizeof(symboln), "%sIsSync", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WaitSync) {
        void ** procp = (void **) &disp->WaitSync;
        snprintf(symboln, sizeof(symboln), "%sWaitSync", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElementsBaseVertex) {
        void ** procp = (void **) &disp->DrawElementsBaseVertex;
        snprintf(symboln, sizeof(symboln), "%sDrawElementsBaseVertex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawElementsInstancedBaseVertex) {
        void ** procp = (void **) &disp->DrawElementsInstancedBaseVertex;
        snprintf(symboln, sizeof(symboln), "%sDrawElementsInstancedBaseVertex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawRangeElementsBaseVertex) {
        void ** procp = (void **) &disp->DrawRangeElementsBaseVertex;
        snprintf(symboln, sizeof(symboln), "%sDrawRangeElementsBaseVertex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsBaseVertex) {
        void ** procp = (void **) &disp->MultiDrawElementsBaseVertex;
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsBaseVertex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateiARB) {
        void ** procp = (void **) &disp->BlendEquationSeparateiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateiARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateiARB) {
        void ** procp = (void **) &disp->BlendEquationSeparateiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateIndexedAMD", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationiARB) {
        void ** procp = (void **) &disp->BlendEquationiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationiARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationiARB) {
        void ** procp = (void **) &disp->BlendEquationiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationIndexedAMD", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateiARB) {
        void ** procp = (void **) &disp->BlendFuncSeparateiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateiARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateiARB) {
        void ** procp = (void **) &disp->BlendFuncSeparateiARB;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateIndexedAMD", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFunciARB) {
        void ** procp = (void **) &disp->BlendFunciARB;
        snprintf(symboln, sizeof(symboln), "%sBlendFunciARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFunciARB) {
        void ** procp = (void **) &disp->BlendFunciARB;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncIndexedAMD", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindSampler) {
        void ** procp = (void **) &disp->BindSampler;
        snprintf(symboln, sizeof(symboln), "%sBindSampler", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteSamplers) {
        void ** procp = (void **) &disp->DeleteSamplers;
        snprintf(symboln, sizeof(symboln), "%sDeleteSamplers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenSamplers) {
        void ** procp = (void **) &disp->GenSamplers;
        snprintf(symboln, sizeof(symboln), "%sGenSamplers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterIiv) {
        void ** procp = (void **) &disp->GetSamplerParameterIiv;
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterIuiv) {
        void ** procp = (void **) &disp->GetSamplerParameterIuiv;
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterIuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameterfv) {
        void ** procp = (void **) &disp->GetSamplerParameterfv;
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetSamplerParameteriv) {
        void ** procp = (void **) &disp->GetSamplerParameteriv;
        snprintf(symboln, sizeof(symboln), "%sGetSamplerParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsSampler) {
        void ** procp = (void **) &disp->IsSampler;
        snprintf(symboln, sizeof(symboln), "%sIsSampler", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterIiv) {
        void ** procp = (void **) &disp->SamplerParameterIiv;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterIiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterIuiv) {
        void ** procp = (void **) &disp->SamplerParameterIuiv;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterIuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterf) {
        void ** procp = (void **) &disp->SamplerParameterf;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameterfv) {
        void ** procp = (void **) &disp->SamplerParameterfv;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameteri) {
        void ** procp = (void **) &disp->SamplerParameteri;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameteri", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplerParameteriv) {
        void ** procp = (void **) &disp->SamplerParameteriv;
        snprintf(symboln, sizeof(symboln), "%sSamplerParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorP3ui) {
        void ** procp = (void **) &disp->ColorP3ui;
        snprintf(symboln, sizeof(symboln), "%sColorP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorP3uiv) {
        void ** procp = (void **) &disp->ColorP3uiv;
        snprintf(symboln, sizeof(symboln), "%sColorP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorP4ui) {
        void ** procp = (void **) &disp->ColorP4ui;
        snprintf(symboln, sizeof(symboln), "%sColorP4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorP4uiv) {
        void ** procp = (void **) &disp->ColorP4uiv;
        snprintf(symboln, sizeof(symboln), "%sColorP4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP1ui) {
        void ** procp = (void **) &disp->MultiTexCoordP1ui;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP1ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP1uiv) {
        void ** procp = (void **) &disp->MultiTexCoordP1uiv;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP1uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP2ui) {
        void ** procp = (void **) &disp->MultiTexCoordP2ui;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP2uiv) {
        void ** procp = (void **) &disp->MultiTexCoordP2uiv;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP3ui) {
        void ** procp = (void **) &disp->MultiTexCoordP3ui;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP3uiv) {
        void ** procp = (void **) &disp->MultiTexCoordP3uiv;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP4ui) {
        void ** procp = (void **) &disp->MultiTexCoordP4ui;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiTexCoordP4uiv) {
        void ** procp = (void **) &disp->MultiTexCoordP4uiv;
        snprintf(symboln, sizeof(symboln), "%sMultiTexCoordP4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->NormalP3ui) {
        void ** procp = (void **) &disp->NormalP3ui;
        snprintf(symboln, sizeof(symboln), "%sNormalP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->NormalP3uiv) {
        void ** procp = (void **) &disp->NormalP3uiv;
        snprintf(symboln, sizeof(symboln), "%sNormalP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorP3ui) {
        void ** procp = (void **) &disp->SecondaryColorP3ui;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorP3uiv) {
        void ** procp = (void **) &disp->SecondaryColorP3uiv;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP1ui) {
        void ** procp = (void **) &disp->TexCoordP1ui;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP1ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP1uiv) {
        void ** procp = (void **) &disp->TexCoordP1uiv;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP1uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP2ui) {
        void ** procp = (void **) &disp->TexCoordP2ui;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP2uiv) {
        void ** procp = (void **) &disp->TexCoordP2uiv;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP3ui) {
        void ** procp = (void **) &disp->TexCoordP3ui;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP3uiv) {
        void ** procp = (void **) &disp->TexCoordP3uiv;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP4ui) {
        void ** procp = (void **) &disp->TexCoordP4ui;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordP4uiv) {
        void ** procp = (void **) &disp->TexCoordP4uiv;
        snprintf(symboln, sizeof(symboln), "%sTexCoordP4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP1ui) {
        void ** procp = (void **) &disp->VertexAttribP1ui;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP1ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP1uiv) {
        void ** procp = (void **) &disp->VertexAttribP1uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP1uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP2ui) {
        void ** procp = (void **) &disp->VertexAttribP2ui;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP2uiv) {
        void ** procp = (void **) &disp->VertexAttribP2uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP3ui) {
        void ** procp = (void **) &disp->VertexAttribP3ui;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP3uiv) {
        void ** procp = (void **) &disp->VertexAttribP3uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP4ui) {
        void ** procp = (void **) &disp->VertexAttribP4ui;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribP4uiv) {
        void ** procp = (void **) &disp->VertexAttribP4uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribP4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP2ui) {
        void ** procp = (void **) &disp->VertexP2ui;
        snprintf(symboln, sizeof(symboln), "%sVertexP2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP2uiv) {
        void ** procp = (void **) &disp->VertexP2uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexP2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP3ui) {
        void ** procp = (void **) &disp->VertexP3ui;
        snprintf(symboln, sizeof(symboln), "%sVertexP3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP3uiv) {
        void ** procp = (void **) &disp->VertexP3uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexP3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP4ui) {
        void ** procp = (void **) &disp->VertexP4ui;
        snprintf(symboln, sizeof(symboln), "%sVertexP4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexP4uiv) {
        void ** procp = (void **) &disp->VertexP4uiv;
        snprintf(symboln, sizeof(symboln), "%sVertexP4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindTransformFeedback) {
        void ** procp = (void **) &disp->BindTransformFeedback;
        snprintf(symboln, sizeof(symboln), "%sBindTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteTransformFeedbacks) {
        void ** procp = (void **) &disp->DeleteTransformFeedbacks;
        snprintf(symboln, sizeof(symboln), "%sDeleteTransformFeedbacks", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DrawTransformFeedback) {
        void ** procp = (void **) &disp->DrawTransformFeedback;
        snprintf(symboln, sizeof(symboln), "%sDrawTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenTransformFeedbacks) {
        void ** procp = (void **) &disp->GenTransformFeedbacks;
        snprintf(symboln, sizeof(symboln), "%sGenTransformFeedbacks", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsTransformFeedback) {
        void ** procp = (void **) &disp->IsTransformFeedback;
        snprintf(symboln, sizeof(symboln), "%sIsTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PauseTransformFeedback) {
        void ** procp = (void **) &disp->PauseTransformFeedback;
        snprintf(symboln, sizeof(symboln), "%sPauseTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResumeTransformFeedback) {
        void ** procp = (void **) &disp->ResumeTransformFeedback;
        snprintf(symboln, sizeof(symboln), "%sResumeTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearDepthf) {
        void ** procp = (void **) &disp->ClearDepthf;
        snprintf(symboln, sizeof(symboln), "%sClearDepthf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DepthRangef) {
        void ** procp = (void **) &disp->DepthRangef;
        snprintf(symboln, sizeof(symboln), "%sDepthRangef", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetShaderPrecisionFormat) {
        void ** procp = (void **) &disp->GetShaderPrecisionFormat;
        snprintf(symboln, sizeof(symboln), "%sGetShaderPrecisionFormat", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ReleaseShaderCompiler) {
        void ** procp = (void **) &disp->ReleaseShaderCompiler;
        snprintf(symboln, sizeof(symboln), "%sReleaseShaderCompiler", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ShaderBinary) {
        void ** procp = (void **) &disp->ShaderBinary;
        snprintf(symboln, sizeof(symboln), "%sShaderBinary", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetGraphicsResetStatusARB) {
        void ** procp = (void **) &disp->GetGraphicsResetStatusARB;
        snprintf(symboln, sizeof(symboln), "%sGetGraphicsResetStatusARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnColorTableARB) {
        void ** procp = (void **) &disp->GetnColorTableARB;
        snprintf(symboln, sizeof(symboln), "%sGetnColorTableARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnCompressedTexImageARB) {
        void ** procp = (void **) &disp->GetnCompressedTexImageARB;
        snprintf(symboln, sizeof(symboln), "%sGetnCompressedTexImageARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnConvolutionFilterARB) {
        void ** procp = (void **) &disp->GetnConvolutionFilterARB;
        snprintf(symboln, sizeof(symboln), "%sGetnConvolutionFilterARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnHistogramARB) {
        void ** procp = (void **) &disp->GetnHistogramARB;
        snprintf(symboln, sizeof(symboln), "%sGetnHistogramARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnMapdvARB) {
        void ** procp = (void **) &disp->GetnMapdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnMapdvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnMapfvARB) {
        void ** procp = (void **) &disp->GetnMapfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnMapfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnMapivARB) {
        void ** procp = (void **) &disp->GetnMapivARB;
        snprintf(symboln, sizeof(symboln), "%sGetnMapivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnMinmaxARB) {
        void ** procp = (void **) &disp->GetnMinmaxARB;
        snprintf(symboln, sizeof(symboln), "%sGetnMinmaxARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapfvARB) {
        void ** procp = (void **) &disp->GetnPixelMapfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapuivARB) {
        void ** procp = (void **) &disp->GetnPixelMapuivARB;
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapuivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnPixelMapusvARB) {
        void ** procp = (void **) &disp->GetnPixelMapusvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnPixelMapusvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnPolygonStippleARB) {
        void ** procp = (void **) &disp->GetnPolygonStippleARB;
        snprintf(symboln, sizeof(symboln), "%sGetnPolygonStippleARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnSeparableFilterARB) {
        void ** procp = (void **) &disp->GetnSeparableFilterARB;
        snprintf(symboln, sizeof(symboln), "%sGetnSeparableFilterARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnTexImageARB) {
        void ** procp = (void **) &disp->GetnTexImageARB;
        snprintf(symboln, sizeof(symboln), "%sGetnTexImageARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnUniformdvARB) {
        void ** procp = (void **) &disp->GetnUniformdvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnUniformdvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnUniformfvARB) {
        void ** procp = (void **) &disp->GetnUniformfvARB;
        snprintf(symboln, sizeof(symboln), "%sGetnUniformfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnUniformivARB) {
        void ** procp = (void **) &disp->GetnUniformivARB;
        snprintf(symboln, sizeof(symboln), "%sGetnUniformivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetnUniformuivARB) {
        void ** procp = (void **) &disp->GetnUniformuivARB;
        snprintf(symboln, sizeof(symboln), "%sGetnUniformuivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ReadnPixelsARB) {
        void ** procp = (void **) &disp->ReadnPixelsARB;
        snprintf(symboln, sizeof(symboln), "%sReadnPixelsARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexStorage1D) {
        void ** procp = (void **) &disp->TexStorage1D;
        snprintf(symboln, sizeof(symboln), "%sTexStorage1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexStorage2D) {
        void ** procp = (void **) &disp->TexStorage2D;
        snprintf(symboln, sizeof(symboln), "%sTexStorage2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexStorage3D) {
        void ** procp = (void **) &disp->TexStorage3D;
        snprintf(symboln, sizeof(symboln), "%sTexStorage3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TextureStorage1DEXT) {
        void ** procp = (void **) &disp->TextureStorage1DEXT;
        snprintf(symboln, sizeof(symboln), "%sTextureStorage1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TextureStorage2DEXT) {
        void ** procp = (void **) &disp->TextureStorage2DEXT;
        snprintf(symboln, sizeof(symboln), "%sTextureStorage2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TextureStorage3DEXT) {
        void ** procp = (void **) &disp->TextureStorage3DEXT;
        snprintf(symboln, sizeof(symboln), "%sTextureStorage3DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PolygonOffsetEXT) {
        void ** procp = (void **) &disp->PolygonOffsetEXT;
        snprintf(symboln, sizeof(symboln), "%sPolygonOffsetEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPixelTexGenParameterfvSGIS) {
        void ** procp = (void **) &disp->GetPixelTexGenParameterfvSGIS;
        snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterfvSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetPixelTexGenParameterivSGIS) {
        void ** procp = (void **) &disp->GetPixelTexGenParameterivSGIS;
        snprintf(symboln, sizeof(symboln), "%sGetPixelTexGenParameterivSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterfSGIS) {
        void ** procp = (void **) &disp->PixelTexGenParameterfSGIS;
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterfvSGIS) {
        void ** procp = (void **) &disp->PixelTexGenParameterfvSGIS;
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterfvSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameteriSGIS) {
        void ** procp = (void **) &disp->PixelTexGenParameteriSGIS;
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameteriSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenParameterivSGIS) {
        void ** procp = (void **) &disp->PixelTexGenParameterivSGIS;
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenParameterivSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SampleMaskSGIS) {
        void ** procp = (void **) &disp->SampleMaskSGIS;
        snprintf(symboln, sizeof(symboln), "%sSampleMaskSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SampleMaskSGIS) {
        void ** procp = (void **) &disp->SampleMaskSGIS;
        snprintf(symboln, sizeof(symboln), "%sSampleMaskEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplePatternSGIS) {
        void ** procp = (void **) &disp->SamplePatternSGIS;
        snprintf(symboln, sizeof(symboln), "%sSamplePatternSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SamplePatternSGIS) {
        void ** procp = (void **) &disp->SamplePatternSGIS;
        snprintf(symboln, sizeof(symboln), "%sSamplePatternEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorPointerEXT) {
        void ** procp = (void **) &disp->ColorPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sColorPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EdgeFlagPointerEXT) {
        void ** procp = (void **) &disp->EdgeFlagPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sEdgeFlagPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IndexPointerEXT) {
        void ** procp = (void **) &disp->IndexPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sIndexPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->NormalPointerEXT) {
        void ** procp = (void **) &disp->NormalPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sNormalPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexCoordPointerEXT) {
        void ** procp = (void **) &disp->TexCoordPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sTexCoordPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexPointerEXT) {
        void ** procp = (void **) &disp->VertexPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        void ** procp = (void **) &disp->PointParameterfEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        void ** procp = (void **) &disp->PointParameterfEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        void ** procp = (void **) &disp->PointParameterfEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfEXT) {
        void ** procp = (void **) &disp->PointParameterfEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        void ** procp = (void **) &disp->PointParameterfvEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        void ** procp = (void **) &disp->PointParameterfvEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        void ** procp = (void **) &disp->PointParameterfvEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterfvEXT) {
        void ** procp = (void **) &disp->PointParameterfvEXT;
        snprintf(symboln, sizeof(symboln), "%sPointParameterfvSGIS", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LockArraysEXT) {
        void ** procp = (void **) &disp->LockArraysEXT;
        snprintf(symboln, sizeof(symboln), "%sLockArraysEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UnlockArraysEXT) {
        void ** procp = (void **) &disp->UnlockArraysEXT;
        snprintf(symboln, sizeof(symboln), "%sUnlockArraysEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bEXT) {
        void ** procp = (void **) &disp->SecondaryColor3bEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3b", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bEXT) {
        void ** procp = (void **) &disp->SecondaryColor3bEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3bvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3bvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3bvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3bvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dEXT) {
        void ** procp = (void **) &disp->SecondaryColor3dEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dEXT) {
        void ** procp = (void **) &disp->SecondaryColor3dEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3dvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3dvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3dvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3dvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fEXT) {
        void ** procp = (void **) &disp->SecondaryColor3fEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fEXT) {
        void ** procp = (void **) &disp->SecondaryColor3fEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3fvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3fvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3fvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3fvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3iEXT) {
        void ** procp = (void **) &disp->SecondaryColor3iEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3iEXT) {
        void ** procp = (void **) &disp->SecondaryColor3iEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ivEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ivEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ivEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ivEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3sEXT) {
        void ** procp = (void **) &disp->SecondaryColor3sEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3sEXT) {
        void ** procp = (void **) &disp->SecondaryColor3sEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3svEXT) {
        void ** procp = (void **) &disp->SecondaryColor3svEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3svEXT) {
        void ** procp = (void **) &disp->SecondaryColor3svEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3svEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ubEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ub", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ubEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ubvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3ubvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3ubvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ubvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uiEXT) {
        void ** procp = (void **) &disp->SecondaryColor3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uiEXT) {
        void ** procp = (void **) &disp->SecondaryColor3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uivEXT) {
        void ** procp = (void **) &disp->SecondaryColor3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3uivEXT) {
        void ** procp = (void **) &disp->SecondaryColor3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usEXT) {
        void ** procp = (void **) &disp->SecondaryColor3usEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3us", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usEXT) {
        void ** procp = (void **) &disp->SecondaryColor3usEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3usvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColor3usvEXT) {
        void ** procp = (void **) &disp->SecondaryColor3usvEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColor3usvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorPointerEXT) {
        void ** procp = (void **) &disp->SecondaryColorPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SecondaryColorPointerEXT) {
        void ** procp = (void **) &disp->SecondaryColorPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sSecondaryColorPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiDrawArraysEXT) {
        void ** procp = (void **) &disp->MultiDrawArraysEXT;
        snprintf(symboln, sizeof(symboln), "%sMultiDrawArrays", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiDrawArraysEXT) {
        void ** procp = (void **) &disp->MultiDrawArraysEXT;
        snprintf(symboln, sizeof(symboln), "%sMultiDrawArraysEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsEXT) {
        void ** procp = (void **) &disp->MultiDrawElementsEXT;
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElements", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiDrawElementsEXT) {
        void ** procp = (void **) &disp->MultiDrawElementsEXT;
        snprintf(symboln, sizeof(symboln), "%sMultiDrawElementsEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordPointerEXT) {
        void ** procp = (void **) &disp->FogCoordPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordPointerEXT) {
        void ** procp = (void **) &disp->FogCoordPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoorddEXT) {
        void ** procp = (void **) &disp->FogCoorddEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordd", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoorddEXT) {
        void ** procp = (void **) &disp->FogCoorddEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoorddEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoorddvEXT) {
        void ** procp = (void **) &disp->FogCoorddvEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoorddv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoorddvEXT) {
        void ** procp = (void **) &disp->FogCoorddvEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoorddvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordfEXT) {
        void ** procp = (void **) &disp->FogCoordfEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordf", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordfEXT) {
        void ** procp = (void **) &disp->FogCoordfEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordfEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordfvEXT) {
        void ** procp = (void **) &disp->FogCoordfvEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordfv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FogCoordfvEXT) {
        void ** procp = (void **) &disp->FogCoordfvEXT;
        snprintf(symboln, sizeof(symboln), "%sFogCoordfvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PixelTexGenSGIX) {
        void ** procp = (void **) &disp->PixelTexGenSGIX;
        snprintf(symboln, sizeof(symboln), "%sPixelTexGenSGIX", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        void ** procp = (void **) &disp->BlendFuncSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparate", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        void ** procp = (void **) &disp->BlendFuncSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendFuncSeparateEXT) {
        void ** procp = (void **) &disp->BlendFuncSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendFuncSeparateINGR", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FlushVertexArrayRangeNV) {
        void ** procp = (void **) &disp->FlushVertexArrayRangeNV;
        snprintf(symboln, sizeof(symboln), "%sFlushVertexArrayRangeNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexArrayRangeNV) {
        void ** procp = (void **) &disp->VertexArrayRangeNV;
        snprintf(symboln, sizeof(symboln), "%sVertexArrayRangeNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerInputNV) {
        void ** procp = (void **) &disp->CombinerInputNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerInputNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerOutputNV) {
        void ** procp = (void **) &disp->CombinerOutputNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerOutputNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterfNV) {
        void ** procp = (void **) &disp->CombinerParameterfNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterfNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterfvNV) {
        void ** procp = (void **) &disp->CombinerParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerParameteriNV) {
        void ** procp = (void **) &disp->CombinerParameteriNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerParameteriNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CombinerParameterivNV) {
        void ** procp = (void **) &disp->CombinerParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sCombinerParameterivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FinalCombinerInputNV) {
        void ** procp = (void **) &disp->FinalCombinerInputNV;
        snprintf(symboln, sizeof(symboln), "%sFinalCombinerInputNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCombinerInputParameterfvNV) {
        void ** procp = (void **) &disp->GetCombinerInputParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCombinerInputParameterivNV) {
        void ** procp = (void **) &disp->GetCombinerInputParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sGetCombinerInputParameterivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCombinerOutputParameterfvNV) {
        void ** procp = (void **) &disp->GetCombinerOutputParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetCombinerOutputParameterivNV) {
        void ** procp = (void **) &disp->GetCombinerOutputParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sGetCombinerOutputParameterivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFinalCombinerInputParameterfvNV) {
        void ** procp = (void **) &disp->GetFinalCombinerInputParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFinalCombinerInputParameterivNV) {
        void ** procp = (void **) &disp->GetFinalCombinerInputParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sGetFinalCombinerInputParameterivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ResizeBuffersMESA) {
        void ** procp = (void **) &disp->ResizeBuffersMESA;
        snprintf(symboln, sizeof(symboln), "%sResizeBuffersMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        void ** procp = (void **) &disp->WindowPos2dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        void ** procp = (void **) &disp->WindowPos2dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dMESA) {
        void ** procp = (void **) &disp->WindowPos2dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        void ** procp = (void **) &disp->WindowPos2dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        void ** procp = (void **) &disp->WindowPos2dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2dvMESA) {
        void ** procp = (void **) &disp->WindowPos2dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2dvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        void ** procp = (void **) &disp->WindowPos2fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        void ** procp = (void **) &disp->WindowPos2fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fMESA) {
        void ** procp = (void **) &disp->WindowPos2fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        void ** procp = (void **) &disp->WindowPos2fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        void ** procp = (void **) &disp->WindowPos2fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2fvMESA) {
        void ** procp = (void **) &disp->WindowPos2fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2fvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        void ** procp = (void **) &disp->WindowPos2iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        void ** procp = (void **) &disp->WindowPos2iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2iMESA) {
        void ** procp = (void **) &disp->WindowPos2iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        void ** procp = (void **) &disp->WindowPos2ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        void ** procp = (void **) &disp->WindowPos2ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2ivMESA) {
        void ** procp = (void **) &disp->WindowPos2ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2ivMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        void ** procp = (void **) &disp->WindowPos2sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        void ** procp = (void **) &disp->WindowPos2sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2sMESA) {
        void ** procp = (void **) &disp->WindowPos2sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        void ** procp = (void **) &disp->WindowPos2svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        void ** procp = (void **) &disp->WindowPos2svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos2svMESA) {
        void ** procp = (void **) &disp->WindowPos2svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos2svMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        void ** procp = (void **) &disp->WindowPos3dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3d", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        void ** procp = (void **) &disp->WindowPos3dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dMESA) {
        void ** procp = (void **) &disp->WindowPos3dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        void ** procp = (void **) &disp->WindowPos3dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        void ** procp = (void **) &disp->WindowPos3dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3dvMESA) {
        void ** procp = (void **) &disp->WindowPos3dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3dvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        void ** procp = (void **) &disp->WindowPos3fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3f", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        void ** procp = (void **) &disp->WindowPos3fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fMESA) {
        void ** procp = (void **) &disp->WindowPos3fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        void ** procp = (void **) &disp->WindowPos3fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        void ** procp = (void **) &disp->WindowPos3fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fvARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3fvMESA) {
        void ** procp = (void **) &disp->WindowPos3fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3fvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        void ** procp = (void **) &disp->WindowPos3iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        void ** procp = (void **) &disp->WindowPos3iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3iMESA) {
        void ** procp = (void **) &disp->WindowPos3iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        void ** procp = (void **) &disp->WindowPos3ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        void ** procp = (void **) &disp->WindowPos3ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3ivARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3ivMESA) {
        void ** procp = (void **) &disp->WindowPos3ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3ivMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        void ** procp = (void **) &disp->WindowPos3sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3s", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        void ** procp = (void **) &disp->WindowPos3sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3sMESA) {
        void ** procp = (void **) &disp->WindowPos3sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        void ** procp = (void **) &disp->WindowPos3svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        void ** procp = (void **) &disp->WindowPos3svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3svARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos3svMESA) {
        void ** procp = (void **) &disp->WindowPos3svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos3svMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4dMESA) {
        void ** procp = (void **) &disp->WindowPos4dMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4dMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4dvMESA) {
        void ** procp = (void **) &disp->WindowPos4dvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4dvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4fMESA) {
        void ** procp = (void **) &disp->WindowPos4fMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4fMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4fvMESA) {
        void ** procp = (void **) &disp->WindowPos4fvMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4fvMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4iMESA) {
        void ** procp = (void **) &disp->WindowPos4iMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4iMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4ivMESA) {
        void ** procp = (void **) &disp->WindowPos4ivMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4ivMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4sMESA) {
        void ** procp = (void **) &disp->WindowPos4sMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4sMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->WindowPos4svMESA) {
        void ** procp = (void **) &disp->WindowPos4svMESA;
        snprintf(symboln, sizeof(symboln), "%sWindowPos4svMESA", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiModeDrawArraysIBM) {
        void ** procp = (void **) &disp->MultiModeDrawArraysIBM;
        snprintf(symboln, sizeof(symboln), "%sMultiModeDrawArraysIBM", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->MultiModeDrawElementsIBM) {
        void ** procp = (void **) &disp->MultiModeDrawElementsIBM;
        snprintf(symboln, sizeof(symboln), "%sMultiModeDrawElementsIBM", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteFencesNV) {
        void ** procp = (void **) &disp->DeleteFencesNV;
        snprintf(symboln, sizeof(symboln), "%sDeleteFencesNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FinishFenceNV) {
        void ** procp = (void **) &disp->FinishFenceNV;
        snprintf(symboln, sizeof(symboln), "%sFinishFenceNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenFencesNV) {
        void ** procp = (void **) &disp->GenFencesNV;
        snprintf(symboln, sizeof(symboln), "%sGenFencesNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFenceivNV) {
        void ** procp = (void **) &disp->GetFenceivNV;
        snprintf(symboln, sizeof(symboln), "%sGetFenceivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsFenceNV) {
        void ** procp = (void **) &disp->IsFenceNV;
        snprintf(symboln, sizeof(symboln), "%sIsFenceNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SetFenceNV) {
        void ** procp = (void **) &disp->SetFenceNV;
        snprintf(symboln, sizeof(symboln), "%sSetFenceNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TestFenceNV) {
        void ** procp = (void **) &disp->TestFenceNV;
        snprintf(symboln, sizeof(symboln), "%sTestFenceNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AreProgramsResidentNV) {
        void ** procp = (void **) &disp->AreProgramsResidentNV;
        snprintf(symboln, sizeof(symboln), "%sAreProgramsResidentNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindProgramNV) {
        void ** procp = (void **) &disp->BindProgramNV;
        snprintf(symboln, sizeof(symboln), "%sBindProgramARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindProgramNV) {
        void ** procp = (void **) &disp->BindProgramNV;
        snprintf(symboln, sizeof(symboln), "%sBindProgramNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteProgramsNV) {
        void ** procp = (void **) &disp->DeleteProgramsNV;
        snprintf(symboln, sizeof(symboln), "%sDeleteProgramsARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteProgramsNV) {
        void ** procp = (void **) &disp->DeleteProgramsNV;
        snprintf(symboln, sizeof(symboln), "%sDeleteProgramsNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ExecuteProgramNV) {
        void ** procp = (void **) &disp->ExecuteProgramNV;
        snprintf(symboln, sizeof(symboln), "%sExecuteProgramNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenProgramsNV) {
        void ** procp = (void **) &disp->GenProgramsNV;
        snprintf(symboln, sizeof(symboln), "%sGenProgramsARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenProgramsNV) {
        void ** procp = (void **) &disp->GenProgramsNV;
        snprintf(symboln, sizeof(symboln), "%sGenProgramsNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramParameterdvNV) {
        void ** procp = (void **) &disp->GetProgramParameterdvNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramParameterdvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramParameterfvNV) {
        void ** procp = (void **) &disp->GetProgramParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramStringNV) {
        void ** procp = (void **) &disp->GetProgramStringNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramStringNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramivNV) {
        void ** procp = (void **) &disp->GetProgramivNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTrackMatrixivNV) {
        void ** procp = (void **) &disp->GetTrackMatrixivNV;
        snprintf(symboln, sizeof(symboln), "%sGetTrackMatrixivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        void ** procp = (void **) &disp->GetVertexAttribPointervNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointerv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        void ** procp = (void **) &disp->GetVertexAttribPointervNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribPointervNV) {
        void ** procp = (void **) &disp->GetVertexAttribPointervNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribPointervNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribdvNV) {
        void ** procp = (void **) &disp->GetVertexAttribdvNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribdvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribfvNV) {
        void ** procp = (void **) &disp->GetVertexAttribfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribivNV) {
        void ** procp = (void **) &disp->GetVertexAttribivNV;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsProgramNV) {
        void ** procp = (void **) &disp->IsProgramNV;
        snprintf(symboln, sizeof(symboln), "%sIsProgramARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsProgramNV) {
        void ** procp = (void **) &disp->IsProgramNV;
        snprintf(symboln, sizeof(symboln), "%sIsProgramNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->LoadProgramNV) {
        void ** procp = (void **) &disp->LoadProgramNV;
        snprintf(symboln, sizeof(symboln), "%sLoadProgramNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramParameters4dvNV) {
        void ** procp = (void **) &disp->ProgramParameters4dvNV;
        snprintf(symboln, sizeof(symboln), "%sProgramParameters4dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramParameters4fvNV) {
        void ** procp = (void **) &disp->ProgramParameters4fvNV;
        snprintf(symboln, sizeof(symboln), "%sProgramParameters4fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RequestResidentProgramsNV) {
        void ** procp = (void **) &disp->RequestResidentProgramsNV;
        snprintf(symboln, sizeof(symboln), "%sRequestResidentProgramsNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TrackMatrixNV) {
        void ** procp = (void **) &disp->TrackMatrixNV;
        snprintf(symboln, sizeof(symboln), "%sTrackMatrixNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dNV) {
        void ** procp = (void **) &disp->VertexAttrib1dNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1dvNV) {
        void ** procp = (void **) &disp->VertexAttrib1dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fNV) {
        void ** procp = (void **) &disp->VertexAttrib1fNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1fvNV) {
        void ** procp = (void **) &disp->VertexAttrib1fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1sNV) {
        void ** procp = (void **) &disp->VertexAttrib1sNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1sNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib1svNV) {
        void ** procp = (void **) &disp->VertexAttrib1svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib1svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dNV) {
        void ** procp = (void **) &disp->VertexAttrib2dNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2dvNV) {
        void ** procp = (void **) &disp->VertexAttrib2dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fNV) {
        void ** procp = (void **) &disp->VertexAttrib2fNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2fvNV) {
        void ** procp = (void **) &disp->VertexAttrib2fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2sNV) {
        void ** procp = (void **) &disp->VertexAttrib2sNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2sNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib2svNV) {
        void ** procp = (void **) &disp->VertexAttrib2svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib2svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dNV) {
        void ** procp = (void **) &disp->VertexAttrib3dNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3dvNV) {
        void ** procp = (void **) &disp->VertexAttrib3dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fNV) {
        void ** procp = (void **) &disp->VertexAttrib3fNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3fvNV) {
        void ** procp = (void **) &disp->VertexAttrib3fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3sNV) {
        void ** procp = (void **) &disp->VertexAttrib3sNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3sNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib3svNV) {
        void ** procp = (void **) &disp->VertexAttrib3svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib3svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dNV) {
        void ** procp = (void **) &disp->VertexAttrib4dNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4dvNV) {
        void ** procp = (void **) &disp->VertexAttrib4dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fNV) {
        void ** procp = (void **) &disp->VertexAttrib4fNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4fvNV) {
        void ** procp = (void **) &disp->VertexAttrib4fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4sNV) {
        void ** procp = (void **) &disp->VertexAttrib4sNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4sNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4svNV) {
        void ** procp = (void **) &disp->VertexAttrib4svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubNV) {
        void ** procp = (void **) &disp->VertexAttrib4ubNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttrib4ubvNV) {
        void ** procp = (void **) &disp->VertexAttrib4ubvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttrib4ubvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribPointerNV) {
        void ** procp = (void **) &disp->VertexAttribPointerNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribPointerNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1dvNV) {
        void ** procp = (void **) &disp->VertexAttribs1dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1fvNV) {
        void ** procp = (void **) &disp->VertexAttribs1fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs1svNV) {
        void ** procp = (void **) &disp->VertexAttribs1svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs1svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2dvNV) {
        void ** procp = (void **) &disp->VertexAttribs2dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2fvNV) {
        void ** procp = (void **) &disp->VertexAttribs2fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs2svNV) {
        void ** procp = (void **) &disp->VertexAttribs2svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs2svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3dvNV) {
        void ** procp = (void **) &disp->VertexAttribs3dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3fvNV) {
        void ** procp = (void **) &disp->VertexAttribs3fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs3svNV) {
        void ** procp = (void **) &disp->VertexAttribs3svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs3svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4dvNV) {
        void ** procp = (void **) &disp->VertexAttribs4dvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4fvNV) {
        void ** procp = (void **) &disp->VertexAttribs4fvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4svNV) {
        void ** procp = (void **) &disp->VertexAttribs4svNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4svNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribs4ubvNV) {
        void ** procp = (void **) &disp->VertexAttribs4ubvNV;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribs4ubvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexBumpParameterfvATI) {
        void ** procp = (void **) &disp->GetTexBumpParameterfvATI;
        snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterfvATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexBumpParameterivATI) {
        void ** procp = (void **) &disp->GetTexBumpParameterivATI;
        snprintf(symboln, sizeof(symboln), "%sGetTexBumpParameterivATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexBumpParameterfvATI) {
        void ** procp = (void **) &disp->TexBumpParameterfvATI;
        snprintf(symboln, sizeof(symboln), "%sTexBumpParameterfvATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexBumpParameterivATI) {
        void ** procp = (void **) &disp->TexBumpParameterivATI;
        snprintf(symboln, sizeof(symboln), "%sTexBumpParameterivATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp1ATI) {
        void ** procp = (void **) &disp->AlphaFragmentOp1ATI;
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp1ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp2ATI) {
        void ** procp = (void **) &disp->AlphaFragmentOp2ATI;
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp2ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->AlphaFragmentOp3ATI) {
        void ** procp = (void **) &disp->AlphaFragmentOp3ATI;
        snprintf(symboln, sizeof(symboln), "%sAlphaFragmentOp3ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginFragmentShaderATI) {
        void ** procp = (void **) &disp->BeginFragmentShaderATI;
        snprintf(symboln, sizeof(symboln), "%sBeginFragmentShaderATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindFragmentShaderATI) {
        void ** procp = (void **) &disp->BindFragmentShaderATI;
        snprintf(symboln, sizeof(symboln), "%sBindFragmentShaderATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp1ATI) {
        void ** procp = (void **) &disp->ColorFragmentOp1ATI;
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp1ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp2ATI) {
        void ** procp = (void **) &disp->ColorFragmentOp2ATI;
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp2ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorFragmentOp3ATI) {
        void ** procp = (void **) &disp->ColorFragmentOp3ATI;
        snprintf(symboln, sizeof(symboln), "%sColorFragmentOp3ATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteFragmentShaderATI) {
        void ** procp = (void **) &disp->DeleteFragmentShaderATI;
        snprintf(symboln, sizeof(symboln), "%sDeleteFragmentShaderATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndFragmentShaderATI) {
        void ** procp = (void **) &disp->EndFragmentShaderATI;
        snprintf(symboln, sizeof(symboln), "%sEndFragmentShaderATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenFragmentShadersATI) {
        void ** procp = (void **) &disp->GenFragmentShadersATI;
        snprintf(symboln, sizeof(symboln), "%sGenFragmentShadersATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PassTexCoordATI) {
        void ** procp = (void **) &disp->PassTexCoordATI;
        snprintf(symboln, sizeof(symboln), "%sPassTexCoordATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SampleMapATI) {
        void ** procp = (void **) &disp->SampleMapATI;
        snprintf(symboln, sizeof(symboln), "%sSampleMapATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->SetFragmentShaderConstantATI) {
        void ** procp = (void **) &disp->SetFragmentShaderConstantATI;
        snprintf(symboln, sizeof(symboln), "%sSetFragmentShaderConstantATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameteriNV) {
        void ** procp = (void **) &disp->PointParameteriNV;
        snprintf(symboln, sizeof(symboln), "%sPointParameteri", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameteriNV) {
        void ** procp = (void **) &disp->PointParameteriNV;
        snprintf(symboln, sizeof(symboln), "%sPointParameteriNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterivNV) {
        void ** procp = (void **) &disp->PointParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sPointParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PointParameterivNV) {
        void ** procp = (void **) &disp->PointParameterivNV;
        snprintf(symboln, sizeof(symboln), "%sPointParameterivNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ActiveStencilFaceEXT) {
        void ** procp = (void **) &disp->ActiveStencilFaceEXT;
        snprintf(symboln, sizeof(symboln), "%sActiveStencilFaceEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindVertexArrayAPPLE) {
        void ** procp = (void **) &disp->BindVertexArrayAPPLE;
        snprintf(symboln, sizeof(symboln), "%sBindVertexArrayAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteVertexArraysAPPLE) {
        void ** procp = (void **) &disp->DeleteVertexArraysAPPLE;
        snprintf(symboln, sizeof(symboln), "%sDeleteVertexArrays", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteVertexArraysAPPLE) {
        void ** procp = (void **) &disp->DeleteVertexArraysAPPLE;
        snprintf(symboln, sizeof(symboln), "%sDeleteVertexArraysAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenVertexArraysAPPLE) {
        void ** procp = (void **) &disp->GenVertexArraysAPPLE;
        snprintf(symboln, sizeof(symboln), "%sGenVertexArraysAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsVertexArrayAPPLE) {
        void ** procp = (void **) &disp->IsVertexArrayAPPLE;
        snprintf(symboln, sizeof(symboln), "%sIsVertexArray", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsVertexArrayAPPLE) {
        void ** procp = (void **) &disp->IsVertexArrayAPPLE;
        snprintf(symboln, sizeof(symboln), "%sIsVertexArrayAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramNamedParameterdvNV) {
        void ** procp = (void **) &disp->GetProgramNamedParameterdvNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterdvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetProgramNamedParameterfvNV) {
        void ** procp = (void **) &disp->GetProgramNamedParameterfvNV;
        snprintf(symboln, sizeof(symboln), "%sGetProgramNamedParameterfvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4dNV) {
        void ** procp = (void **) &disp->ProgramNamedParameter4dNV;
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4dvNV) {
        void ** procp = (void **) &disp->ProgramNamedParameter4dvNV;
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4dvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4fNV) {
        void ** procp = (void **) &disp->ProgramNamedParameter4fNV;
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramNamedParameter4fvNV) {
        void ** procp = (void **) &disp->ProgramNamedParameter4fvNV;
        snprintf(symboln, sizeof(symboln), "%sProgramNamedParameter4fvNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartIndexNV) {
        void ** procp = (void **) &disp->PrimitiveRestartIndexNV;
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndexNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartIndexNV) {
        void ** procp = (void **) &disp->PrimitiveRestartIndexNV;
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartIndex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->PrimitiveRestartNV) {
        void ** procp = (void **) &disp->PrimitiveRestartNV;
        snprintf(symboln, sizeof(symboln), "%sPrimitiveRestartNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DepthBoundsEXT) {
        void ** procp = (void **) &disp->DepthBoundsEXT;
        snprintf(symboln, sizeof(symboln), "%sDepthBoundsEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        void ** procp = (void **) &disp->BlendEquationSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparate", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        void ** procp = (void **) &disp->BlendEquationSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlendEquationSeparateEXT) {
        void ** procp = (void **) &disp->BlendEquationSeparateEXT;
        snprintf(symboln, sizeof(symboln), "%sBlendEquationSeparateATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindFramebufferEXT) {
        void ** procp = (void **) &disp->BindFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBindFramebuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindFramebufferEXT) {
        void ** procp = (void **) &disp->BindFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBindFramebufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindRenderbufferEXT) {
        void ** procp = (void **) &disp->BindRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBindRenderbuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindRenderbufferEXT) {
        void ** procp = (void **) &disp->BindRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBindRenderbufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CheckFramebufferStatusEXT) {
        void ** procp = (void **) &disp->CheckFramebufferStatusEXT;
        snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatus", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CheckFramebufferStatusEXT) {
        void ** procp = (void **) &disp->CheckFramebufferStatusEXT;
        snprintf(symboln, sizeof(symboln), "%sCheckFramebufferStatusEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteFramebuffersEXT) {
        void ** procp = (void **) &disp->DeleteFramebuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteFramebuffersEXT) {
        void ** procp = (void **) &disp->DeleteFramebuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sDeleteFramebuffersEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteRenderbuffersEXT) {
        void ** procp = (void **) &disp->DeleteRenderbuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DeleteRenderbuffersEXT) {
        void ** procp = (void **) &disp->DeleteRenderbuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sDeleteRenderbuffersEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferRenderbufferEXT) {
        void ** procp = (void **) &disp->FramebufferRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferRenderbufferEXT) {
        void ** procp = (void **) &disp->FramebufferRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferRenderbufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture1DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture1DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture1DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture1DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture1DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture2DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture2DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture2DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture2DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture2DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture3DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture3DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3D", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTexture3DEXT) {
        void ** procp = (void **) &disp->FramebufferTexture3DEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTexture3DEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenFramebuffersEXT) {
        void ** procp = (void **) &disp->GenFramebuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sGenFramebuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenFramebuffersEXT) {
        void ** procp = (void **) &disp->GenFramebuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sGenFramebuffersEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenRenderbuffersEXT) {
        void ** procp = (void **) &disp->GenRenderbuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sGenRenderbuffers", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenRenderbuffersEXT) {
        void ** procp = (void **) &disp->GenRenderbuffersEXT;
        snprintf(symboln, sizeof(symboln), "%sGenRenderbuffersEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenerateMipmapEXT) {
        void ** procp = (void **) &disp->GenerateMipmapEXT;
        snprintf(symboln, sizeof(symboln), "%sGenerateMipmap", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GenerateMipmapEXT) {
        void ** procp = (void **) &disp->GenerateMipmapEXT;
        snprintf(symboln, sizeof(symboln), "%sGenerateMipmapEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
        void ** procp = (void **) &disp->GetFramebufferAttachmentParameterivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFramebufferAttachmentParameterivEXT) {
        void ** procp = (void **) &disp->GetFramebufferAttachmentParameterivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetFramebufferAttachmentParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetRenderbufferParameterivEXT) {
        void ** procp = (void **) &disp->GetRenderbufferParameterivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameteriv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetRenderbufferParameterivEXT) {
        void ** procp = (void **) &disp->GetRenderbufferParameterivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetRenderbufferParameterivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsFramebufferEXT) {
        void ** procp = (void **) &disp->IsFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sIsFramebuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsFramebufferEXT) {
        void ** procp = (void **) &disp->IsFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sIsFramebufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsRenderbufferEXT) {
        void ** procp = (void **) &disp->IsRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sIsRenderbuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsRenderbufferEXT) {
        void ** procp = (void **) &disp->IsRenderbufferEXT;
        snprintf(symboln, sizeof(symboln), "%sIsRenderbufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageEXT) {
        void ** procp = (void **) &disp->RenderbufferStorageEXT;
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorage", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->RenderbufferStorageEXT) {
        void ** procp = (void **) &disp->RenderbufferStorageEXT;
        snprintf(symboln, sizeof(symboln), "%sRenderbufferStorageEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlitFramebufferEXT) {
        void ** procp = (void **) &disp->BlitFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBlitFramebuffer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BlitFramebufferEXT) {
        void ** procp = (void **) &disp->BlitFramebufferEXT;
        snprintf(symboln, sizeof(symboln), "%sBlitFramebufferEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BufferParameteriAPPLE) {
        void ** procp = (void **) &disp->BufferParameteriAPPLE;
        snprintf(symboln, sizeof(symboln), "%sBufferParameteriAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FlushMappedBufferRangeAPPLE) {
        void ** procp = (void **) &disp->FlushMappedBufferRangeAPPLE;
        snprintf(symboln, sizeof(symboln), "%sFlushMappedBufferRangeAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindFragDataLocationEXT) {
        void ** procp = (void **) &disp->BindFragDataLocationEXT;
        snprintf(symboln, sizeof(symboln), "%sBindFragDataLocationEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindFragDataLocationEXT) {
        void ** procp = (void **) &disp->BindFragDataLocationEXT;
        snprintf(symboln, sizeof(symboln), "%sBindFragDataLocation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFragDataLocationEXT) {
        void ** procp = (void **) &disp->GetFragDataLocationEXT;
        snprintf(symboln, sizeof(symboln), "%sGetFragDataLocationEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetFragDataLocationEXT) {
        void ** procp = (void **) &disp->GetFragDataLocationEXT;
        snprintf(symboln, sizeof(symboln), "%sGetFragDataLocation", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformuivEXT) {
        void ** procp = (void **) &disp->GetUniformuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetUniformuivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetUniformuivEXT) {
        void ** procp = (void **) &disp->GetUniformuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetUniformuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIivEXT) {
        void ** procp = (void **) &disp->GetVertexAttribIivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIivEXT) {
        void ** procp = (void **) &disp->GetVertexAttribIivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIuivEXT) {
        void ** procp = (void **) &disp->GetVertexAttribIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetVertexAttribIuivEXT) {
        void ** procp = (void **) &disp->GetVertexAttribIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetVertexAttribIuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1uiEXT) {
        void ** procp = (void **) &disp->Uniform1uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform1uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1uiEXT) {
        void ** procp = (void **) &disp->Uniform1uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform1ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1uivEXT) {
        void ** procp = (void **) &disp->Uniform1uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform1uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform1uivEXT) {
        void ** procp = (void **) &disp->Uniform1uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform1uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2uiEXT) {
        void ** procp = (void **) &disp->Uniform2uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform2uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2uiEXT) {
        void ** procp = (void **) &disp->Uniform2uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2uivEXT) {
        void ** procp = (void **) &disp->Uniform2uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform2uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform2uivEXT) {
        void ** procp = (void **) &disp->Uniform2uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3uiEXT) {
        void ** procp = (void **) &disp->Uniform3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform3uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3uiEXT) {
        void ** procp = (void **) &disp->Uniform3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3uivEXT) {
        void ** procp = (void **) &disp->Uniform3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform3uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform3uivEXT) {
        void ** procp = (void **) &disp->Uniform3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4uiEXT) {
        void ** procp = (void **) &disp->Uniform4uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform4uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4uiEXT) {
        void ** procp = (void **) &disp->Uniform4uiEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4uivEXT) {
        void ** procp = (void **) &disp->Uniform4uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform4uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->Uniform4uivEXT) {
        void ** procp = (void **) &disp->Uniform4uivEXT;
        snprintf(symboln, sizeof(symboln), "%sUniform4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1iEXT) {
        void ** procp = (void **) &disp->VertexAttribI1iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1iEXT) {
        void ** procp = (void **) &disp->VertexAttribI1iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI1ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI1ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI1uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI1uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI1uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI1uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI1uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI1uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2iEXT) {
        void ** procp = (void **) &disp->VertexAttribI2iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2iEXT) {
        void ** procp = (void **) &disp->VertexAttribI2iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI2ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI2ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI2uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI2uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI2uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI2uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI2uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI2uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3iEXT) {
        void ** procp = (void **) &disp->VertexAttribI3iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3iEXT) {
        void ** procp = (void **) &disp->VertexAttribI3iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI3ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI3ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI3uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI3uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI3uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI3uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4bvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4bvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4bvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4bvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4bv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4iEXT) {
        void ** procp = (void **) &disp->VertexAttribI4iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4iEXT) {
        void ** procp = (void **) &disp->VertexAttribI4iEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4i", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI4ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ivEXT) {
        void ** procp = (void **) &disp->VertexAttribI4ivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4iv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4svEXT) {
        void ** procp = (void **) &disp->VertexAttribI4svEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4svEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4svEXT) {
        void ** procp = (void **) &disp->VertexAttribI4svEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4sv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ubvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4ubvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4ubvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4ubvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ubv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI4uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uiEXT) {
        void ** procp = (void **) &disp->VertexAttribI4uiEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4ui", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI4uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4uivEXT) {
        void ** procp = (void **) &disp->VertexAttribI4uivEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4uiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4usvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4usvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribI4usvEXT) {
        void ** procp = (void **) &disp->VertexAttribI4usvEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribI4usv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribIPointerEXT) {
        void ** procp = (void **) &disp->VertexAttribIPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->VertexAttribIPointerEXT) {
        void ** procp = (void **) &disp->VertexAttribIPointerEXT;
        snprintf(symboln, sizeof(symboln), "%sVertexAttribIPointer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureLayerEXT) {
        void ** procp = (void **) &disp->FramebufferTextureLayerEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayer", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureLayerEXT) {
        void ** procp = (void **) &disp->FramebufferTextureLayerEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayerARB", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->FramebufferTextureLayerEXT) {
        void ** procp = (void **) &disp->FramebufferTextureLayerEXT;
        snprintf(symboln, sizeof(symboln), "%sFramebufferTextureLayerEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorMaskIndexedEXT) {
        void ** procp = (void **) &disp->ColorMaskIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sColorMaskIndexedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ColorMaskIndexedEXT) {
        void ** procp = (void **) &disp->ColorMaskIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sColorMaski", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DisableIndexedEXT) {
        void ** procp = (void **) &disp->DisableIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sDisableIndexedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->DisableIndexedEXT) {
        void ** procp = (void **) &disp->DisableIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sDisablei", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EnableIndexedEXT) {
        void ** procp = (void **) &disp->EnableIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sEnableIndexedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EnableIndexedEXT) {
        void ** procp = (void **) &disp->EnableIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sEnablei", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBooleanIndexedvEXT) {
        void ** procp = (void **) &disp->GetBooleanIndexedvEXT;
        snprintf(symboln, sizeof(symboln), "%sGetBooleanIndexedvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetBooleanIndexedvEXT) {
        void ** procp = (void **) &disp->GetBooleanIndexedvEXT;
        snprintf(symboln, sizeof(symboln), "%sGetBooleani_v", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetIntegerIndexedvEXT) {
        void ** procp = (void **) &disp->GetIntegerIndexedvEXT;
        snprintf(symboln, sizeof(symboln), "%sGetIntegerIndexedvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetIntegerIndexedvEXT) {
        void ** procp = (void **) &disp->GetIntegerIndexedvEXT;
        snprintf(symboln, sizeof(symboln), "%sGetIntegeri_v", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsEnabledIndexedEXT) {
        void ** procp = (void **) &disp->IsEnabledIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sIsEnabledIndexedEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->IsEnabledIndexedEXT) {
        void ** procp = (void **) &disp->IsEnabledIndexedEXT;
        snprintf(symboln, sizeof(symboln), "%sIsEnabledi", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearColorIiEXT) {
        void ** procp = (void **) &disp->ClearColorIiEXT;
        snprintf(symboln, sizeof(symboln), "%sClearColorIiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ClearColorIuiEXT) {
        void ** procp = (void **) &disp->ClearColorIuiEXT;
        snprintf(symboln, sizeof(symboln), "%sClearColorIuiEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIivEXT) {
        void ** procp = (void **) &disp->GetTexParameterIivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIivEXT) {
        void ** procp = (void **) &disp->GetTexParameterIivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIuivEXT) {
        void ** procp = (void **) &disp->GetTexParameterIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterIuivEXT) {
        void ** procp = (void **) &disp->GetTexParameterIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterIuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterIivEXT) {
        void ** procp = (void **) &disp->TexParameterIivEXT;
        snprintf(symboln, sizeof(symboln), "%sTexParameterIivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterIivEXT) {
        void ** procp = (void **) &disp->TexParameterIivEXT;
        snprintf(symboln, sizeof(symboln), "%sTexParameterIiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterIuivEXT) {
        void ** procp = (void **) &disp->TexParameterIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sTexParameterIuivEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TexParameterIuivEXT) {
        void ** procp = (void **) &disp->TexParameterIuivEXT;
        snprintf(symboln, sizeof(symboln), "%sTexParameterIuiv", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginConditionalRenderNV) {
        void ** procp = (void **) &disp->BeginConditionalRenderNV;
        snprintf(symboln, sizeof(symboln), "%sBeginConditionalRenderNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginConditionalRenderNV) {
        void ** procp = (void **) &disp->BeginConditionalRenderNV;
        snprintf(symboln, sizeof(symboln), "%sBeginConditionalRender", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndConditionalRenderNV) {
        void ** procp = (void **) &disp->EndConditionalRenderNV;
        snprintf(symboln, sizeof(symboln), "%sEndConditionalRenderNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndConditionalRenderNV) {
        void ** procp = (void **) &disp->EndConditionalRenderNV;
        snprintf(symboln, sizeof(symboln), "%sEndConditionalRender", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginTransformFeedbackEXT) {
        void ** procp = (void **) &disp->BeginTransformFeedbackEXT;
        snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedbackEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BeginTransformFeedbackEXT) {
        void ** procp = (void **) &disp->BeginTransformFeedbackEXT;
        snprintf(symboln, sizeof(symboln), "%sBeginTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferBaseEXT) {
        void ** procp = (void **) &disp->BindBufferBaseEXT;
        snprintf(symboln, sizeof(symboln), "%sBindBufferBaseEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferBaseEXT) {
        void ** procp = (void **) &disp->BindBufferBaseEXT;
        snprintf(symboln, sizeof(symboln), "%sBindBufferBase", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferOffsetEXT) {
        void ** procp = (void **) &disp->BindBufferOffsetEXT;
        snprintf(symboln, sizeof(symboln), "%sBindBufferOffsetEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferRangeEXT) {
        void ** procp = (void **) &disp->BindBufferRangeEXT;
        snprintf(symboln, sizeof(symboln), "%sBindBufferRangeEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->BindBufferRangeEXT) {
        void ** procp = (void **) &disp->BindBufferRangeEXT;
        snprintf(symboln, sizeof(symboln), "%sBindBufferRange", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndTransformFeedbackEXT) {
        void ** procp = (void **) &disp->EndTransformFeedbackEXT;
        snprintf(symboln, sizeof(symboln), "%sEndTransformFeedbackEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EndTransformFeedbackEXT) {
        void ** procp = (void **) &disp->EndTransformFeedbackEXT;
        snprintf(symboln, sizeof(symboln), "%sEndTransformFeedback", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
        void ** procp = (void **) &disp->GetTransformFeedbackVaryingEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVaryingEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTransformFeedbackVaryingEXT) {
        void ** procp = (void **) &disp->GetTransformFeedbackVaryingEXT;
        snprintf(symboln, sizeof(symboln), "%sGetTransformFeedbackVarying", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
        void ** procp = (void **) &disp->TransformFeedbackVaryingsEXT;
        snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryingsEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TransformFeedbackVaryingsEXT) {
        void ** procp = (void **) &disp->TransformFeedbackVaryingsEXT;
        snprintf(symboln, sizeof(symboln), "%sTransformFeedbackVaryings", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProvokingVertexEXT) {
        void ** procp = (void **) &disp->ProvokingVertexEXT;
        snprintf(symboln, sizeof(symboln), "%sProvokingVertexEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProvokingVertexEXT) {
        void ** procp = (void **) &disp->ProvokingVertexEXT;
        snprintf(symboln, sizeof(symboln), "%sProvokingVertex", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetTexParameterPointervAPPLE) {
        void ** procp = (void **) &disp->GetTexParameterPointervAPPLE;
        snprintf(symboln, sizeof(symboln), "%sGetTexParameterPointervAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TextureRangeAPPLE) {
        void ** procp = (void **) &disp->TextureRangeAPPLE;
        snprintf(symboln, sizeof(symboln), "%sTextureRangeAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetObjectParameterivAPPLE) {
        void ** procp = (void **) &disp->GetObjectParameterivAPPLE;
        snprintf(symboln, sizeof(symboln), "%sGetObjectParameterivAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ObjectPurgeableAPPLE) {
        void ** procp = (void **) &disp->ObjectPurgeableAPPLE;
        snprintf(symboln, sizeof(symboln), "%sObjectPurgeableAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ObjectUnpurgeableAPPLE) {
        void ** procp = (void **) &disp->ObjectUnpurgeableAPPLE;
        snprintf(symboln, sizeof(symboln), "%sObjectUnpurgeableAPPLE", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ActiveProgramEXT) {
        void ** procp = (void **) &disp->ActiveProgramEXT;
        snprintf(symboln, sizeof(symboln), "%sActiveProgramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->CreateShaderProgramEXT) {
        void ** procp = (void **) &disp->CreateShaderProgramEXT;
        snprintf(symboln, sizeof(symboln), "%sCreateShaderProgramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->UseShaderProgramEXT) {
        void ** procp = (void **) &disp->UseShaderProgramEXT;
        snprintf(symboln, sizeof(symboln), "%sUseShaderProgramEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->TextureBarrierNV) {
        void ** procp = (void **) &disp->TextureBarrierNV;
        snprintf(symboln, sizeof(symboln), "%sTextureBarrierNV", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->StencilFuncSeparateATI) {
        void ** procp = (void **) &disp->StencilFuncSeparateATI;
        snprintf(symboln, sizeof(symboln), "%sStencilFuncSeparateATI", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramEnvParameters4fvEXT) {
        void ** procp = (void **) &disp->ProgramEnvParameters4fvEXT;
        snprintf(symboln, sizeof(symboln), "%sProgramEnvParameters4fvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->ProgramLocalParameters4fvEXT) {
        void ** procp = (void **) &disp->ProgramLocalParameters4fvEXT;
        snprintf(symboln, sizeof(symboln), "%sProgramLocalParameters4fvEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjecti64vEXT) {
        void ** procp = (void **) &disp->GetQueryObjecti64vEXT;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjecti64vEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->GetQueryObjectui64vEXT) {
        void ** procp = (void **) &disp->GetQueryObjectui64vEXT;
        snprintf(symboln, sizeof(symboln), "%sGetQueryObjectui64vEXT", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EGLImageTargetRenderbufferStorageOES) {
        void ** procp = (void **) &disp->EGLImageTargetRenderbufferStorageOES;
        snprintf(symboln, sizeof(symboln), "%sEGLImageTargetRenderbufferStorageOES", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    if(!disp->EGLImageTargetTexture2DOES) {
        void ** procp = (void **) &disp->EGLImageTargetTexture2DOES;
        snprintf(symboln, sizeof(symboln), "%sEGLImageTargetTexture2DOES", symbol_prefix);
        *procp = dlsym(handle, symboln);
    }


    __glapi_gentable_set_remaining_noop(disp);

    return disp;
}

