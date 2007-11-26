/* DO NOT EDIT - This file generated automatically by gl_table.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
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
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if !defined( _DISPATCH_H_ )
#  define _DISPATCH_H_

/**
 * \file dispatch.h
 * Macros for handling GL dispatch tables.
 *
 * For each known GL function, there are 3 macros in this file.  The first
 * macro is named CALL_FuncName and is used to call that GL function using
 * the specified dispatch table.  The other 2 macros, called GET_FuncName
 * can SET_FuncName, are used to get and set the dispatch pointer for the
 * named function in the specified dispatch table.
 */

#define CALL_by_offset(disp, cast, offset, parameters) \
    (*(cast (GET_by_offset(disp, offset)))) parameters
#define GET_by_offset(disp, offset) \
    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL
#define SET_by_offset(disp, offset, fn) \
    do { \
        if ( (offset) < 0 ) { \
            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\n", */ \
            /*         __func__, __LINE__, disp, offset, # fn); */ \
            /* abort(); */ \
        } \
        else { \
            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \
        } \
    } while(0)

#define CALL_NewList(disp, parameters) (*((disp)->NewList)) parameters
#define GET_NewList(disp) ((disp)->NewList)
#define SET_NewList(disp, fn) ((disp)->NewList = fn)
#define CALL_EndList(disp, parameters) (*((disp)->EndList)) parameters
#define GET_EndList(disp) ((disp)->EndList)
#define SET_EndList(disp, fn) ((disp)->EndList = fn)
#define CALL_CallList(disp, parameters) (*((disp)->CallList)) parameters
#define GET_CallList(disp) ((disp)->CallList)
#define SET_CallList(disp, fn) ((disp)->CallList = fn)
#define CALL_CallLists(disp, parameters) (*((disp)->CallLists)) parameters
#define GET_CallLists(disp) ((disp)->CallLists)
#define SET_CallLists(disp, fn) ((disp)->CallLists = fn)
#define CALL_DeleteLists(disp, parameters) (*((disp)->DeleteLists)) parameters
#define GET_DeleteLists(disp) ((disp)->DeleteLists)
#define SET_DeleteLists(disp, fn) ((disp)->DeleteLists = fn)
#define CALL_GenLists(disp, parameters) (*((disp)->GenLists)) parameters
#define GET_GenLists(disp) ((disp)->GenLists)
#define SET_GenLists(disp, fn) ((disp)->GenLists = fn)
#define CALL_ListBase(disp, parameters) (*((disp)->ListBase)) parameters
#define GET_ListBase(disp) ((disp)->ListBase)
#define SET_ListBase(disp, fn) ((disp)->ListBase = fn)
#define CALL_Begin(disp, parameters) (*((disp)->Begin)) parameters
#define GET_Begin(disp) ((disp)->Begin)
#define SET_Begin(disp, fn) ((disp)->Begin = fn)
#define CALL_Bitmap(disp, parameters) (*((disp)->Bitmap)) parameters
#define GET_Bitmap(disp) ((disp)->Bitmap)
#define SET_Bitmap(disp, fn) ((disp)->Bitmap = fn)
#define CALL_Color3b(disp, parameters) (*((disp)->Color3b)) parameters
#define GET_Color3b(disp) ((disp)->Color3b)
#define SET_Color3b(disp, fn) ((disp)->Color3b = fn)
#define CALL_Color3bv(disp, parameters) (*((disp)->Color3bv)) parameters
#define GET_Color3bv(disp) ((disp)->Color3bv)
#define SET_Color3bv(disp, fn) ((disp)->Color3bv = fn)
#define CALL_Color3d(disp, parameters) (*((disp)->Color3d)) parameters
#define GET_Color3d(disp) ((disp)->Color3d)
#define SET_Color3d(disp, fn) ((disp)->Color3d = fn)
#define CALL_Color3dv(disp, parameters) (*((disp)->Color3dv)) parameters
#define GET_Color3dv(disp) ((disp)->Color3dv)
#define SET_Color3dv(disp, fn) ((disp)->Color3dv = fn)
#define CALL_Color3f(disp, parameters) (*((disp)->Color3f)) parameters
#define GET_Color3f(disp) ((disp)->Color3f)
#define SET_Color3f(disp, fn) ((disp)->Color3f = fn)
#define CALL_Color3fv(disp, parameters) (*((disp)->Color3fv)) parameters
#define GET_Color3fv(disp) ((disp)->Color3fv)
#define SET_Color3fv(disp, fn) ((disp)->Color3fv = fn)
#define CALL_Color3i(disp, parameters) (*((disp)->Color3i)) parameters
#define GET_Color3i(disp) ((disp)->Color3i)
#define SET_Color3i(disp, fn) ((disp)->Color3i = fn)
#define CALL_Color3iv(disp, parameters) (*((disp)->Color3iv)) parameters
#define GET_Color3iv(disp) ((disp)->Color3iv)
#define SET_Color3iv(disp, fn) ((disp)->Color3iv = fn)
#define CALL_Color3s(disp, parameters) (*((disp)->Color3s)) parameters
#define GET_Color3s(disp) ((disp)->Color3s)
#define SET_Color3s(disp, fn) ((disp)->Color3s = fn)
#define CALL_Color3sv(disp, parameters) (*((disp)->Color3sv)) parameters
#define GET_Color3sv(disp) ((disp)->Color3sv)
#define SET_Color3sv(disp, fn) ((disp)->Color3sv = fn)
#define CALL_Color3ub(disp, parameters) (*((disp)->Color3ub)) parameters
#define GET_Color3ub(disp) ((disp)->Color3ub)
#define SET_Color3ub(disp, fn) ((disp)->Color3ub = fn)
#define CALL_Color3ubv(disp, parameters) (*((disp)->Color3ubv)) parameters
#define GET_Color3ubv(disp) ((disp)->Color3ubv)
#define SET_Color3ubv(disp, fn) ((disp)->Color3ubv = fn)
#define CALL_Color3ui(disp, parameters) (*((disp)->Color3ui)) parameters
#define GET_Color3ui(disp) ((disp)->Color3ui)
#define SET_Color3ui(disp, fn) ((disp)->Color3ui = fn)
#define CALL_Color3uiv(disp, parameters) (*((disp)->Color3uiv)) parameters
#define GET_Color3uiv(disp) ((disp)->Color3uiv)
#define SET_Color3uiv(disp, fn) ((disp)->Color3uiv = fn)
#define CALL_Color3us(disp, parameters) (*((disp)->Color3us)) parameters
#define GET_Color3us(disp) ((disp)->Color3us)
#define SET_Color3us(disp, fn) ((disp)->Color3us = fn)
#define CALL_Color3usv(disp, parameters) (*((disp)->Color3usv)) parameters
#define GET_Color3usv(disp) ((disp)->Color3usv)
#define SET_Color3usv(disp, fn) ((disp)->Color3usv = fn)
#define CALL_Color4b(disp, parameters) (*((disp)->Color4b)) parameters
#define GET_Color4b(disp) ((disp)->Color4b)
#define SET_Color4b(disp, fn) ((disp)->Color4b = fn)
#define CALL_Color4bv(disp, parameters) (*((disp)->Color4bv)) parameters
#define GET_Color4bv(disp) ((disp)->Color4bv)
#define SET_Color4bv(disp, fn) ((disp)->Color4bv = fn)
#define CALL_Color4d(disp, parameters) (*((disp)->Color4d)) parameters
#define GET_Color4d(disp) ((disp)->Color4d)
#define SET_Color4d(disp, fn) ((disp)->Color4d = fn)
#define CALL_Color4dv(disp, parameters) (*((disp)->Color4dv)) parameters
#define GET_Color4dv(disp) ((disp)->Color4dv)
#define SET_Color4dv(disp, fn) ((disp)->Color4dv = fn)
#define CALL_Color4f(disp, parameters) (*((disp)->Color4f)) parameters
#define GET_Color4f(disp) ((disp)->Color4f)
#define SET_Color4f(disp, fn) ((disp)->Color4f = fn)
#define CALL_Color4fv(disp, parameters) (*((disp)->Color4fv)) parameters
#define GET_Color4fv(disp) ((disp)->Color4fv)
#define SET_Color4fv(disp, fn) ((disp)->Color4fv = fn)
#define CALL_Color4i(disp, parameters) (*((disp)->Color4i)) parameters
#define GET_Color4i(disp) ((disp)->Color4i)
#define SET_Color4i(disp, fn) ((disp)->Color4i = fn)
#define CALL_Color4iv(disp, parameters) (*((disp)->Color4iv)) parameters
#define GET_Color4iv(disp) ((disp)->Color4iv)
#define SET_Color4iv(disp, fn) ((disp)->Color4iv = fn)
#define CALL_Color4s(disp, parameters) (*((disp)->Color4s)) parameters
#define GET_Color4s(disp) ((disp)->Color4s)
#define SET_Color4s(disp, fn) ((disp)->Color4s = fn)
#define CALL_Color4sv(disp, parameters) (*((disp)->Color4sv)) parameters
#define GET_Color4sv(disp) ((disp)->Color4sv)
#define SET_Color4sv(disp, fn) ((disp)->Color4sv = fn)
#define CALL_Color4ub(disp, parameters) (*((disp)->Color4ub)) parameters
#define GET_Color4ub(disp) ((disp)->Color4ub)
#define SET_Color4ub(disp, fn) ((disp)->Color4ub = fn)
#define CALL_Color4ubv(disp, parameters) (*((disp)->Color4ubv)) parameters
#define GET_Color4ubv(disp) ((disp)->Color4ubv)
#define SET_Color4ubv(disp, fn) ((disp)->Color4ubv = fn)
#define CALL_Color4ui(disp, parameters) (*((disp)->Color4ui)) parameters
#define GET_Color4ui(disp) ((disp)->Color4ui)
#define SET_Color4ui(disp, fn) ((disp)->Color4ui = fn)
#define CALL_Color4uiv(disp, parameters) (*((disp)->Color4uiv)) parameters
#define GET_Color4uiv(disp) ((disp)->Color4uiv)
#define SET_Color4uiv(disp, fn) ((disp)->Color4uiv = fn)
#define CALL_Color4us(disp, parameters) (*((disp)->Color4us)) parameters
#define GET_Color4us(disp) ((disp)->Color4us)
#define SET_Color4us(disp, fn) ((disp)->Color4us = fn)
#define CALL_Color4usv(disp, parameters) (*((disp)->Color4usv)) parameters
#define GET_Color4usv(disp) ((disp)->Color4usv)
#define SET_Color4usv(disp, fn) ((disp)->Color4usv = fn)
#define CALL_EdgeFlag(disp, parameters) (*((disp)->EdgeFlag)) parameters
#define GET_EdgeFlag(disp) ((disp)->EdgeFlag)
#define SET_EdgeFlag(disp, fn) ((disp)->EdgeFlag = fn)
#define CALL_EdgeFlagv(disp, parameters) (*((disp)->EdgeFlagv)) parameters
#define GET_EdgeFlagv(disp) ((disp)->EdgeFlagv)
#define SET_EdgeFlagv(disp, fn) ((disp)->EdgeFlagv = fn)
#define CALL_End(disp, parameters) (*((disp)->End)) parameters
#define GET_End(disp) ((disp)->End)
#define SET_End(disp, fn) ((disp)->End = fn)
#define CALL_Indexd(disp, parameters) (*((disp)->Indexd)) parameters
#define GET_Indexd(disp) ((disp)->Indexd)
#define SET_Indexd(disp, fn) ((disp)->Indexd = fn)
#define CALL_Indexdv(disp, parameters) (*((disp)->Indexdv)) parameters
#define GET_Indexdv(disp) ((disp)->Indexdv)
#define SET_Indexdv(disp, fn) ((disp)->Indexdv = fn)
#define CALL_Indexf(disp, parameters) (*((disp)->Indexf)) parameters
#define GET_Indexf(disp) ((disp)->Indexf)
#define SET_Indexf(disp, fn) ((disp)->Indexf = fn)
#define CALL_Indexfv(disp, parameters) (*((disp)->Indexfv)) parameters
#define GET_Indexfv(disp) ((disp)->Indexfv)
#define SET_Indexfv(disp, fn) ((disp)->Indexfv = fn)
#define CALL_Indexi(disp, parameters) (*((disp)->Indexi)) parameters
#define GET_Indexi(disp) ((disp)->Indexi)
#define SET_Indexi(disp, fn) ((disp)->Indexi = fn)
#define CALL_Indexiv(disp, parameters) (*((disp)->Indexiv)) parameters
#define GET_Indexiv(disp) ((disp)->Indexiv)
#define SET_Indexiv(disp, fn) ((disp)->Indexiv = fn)
#define CALL_Indexs(disp, parameters) (*((disp)->Indexs)) parameters
#define GET_Indexs(disp) ((disp)->Indexs)
#define SET_Indexs(disp, fn) ((disp)->Indexs = fn)
#define CALL_Indexsv(disp, parameters) (*((disp)->Indexsv)) parameters
#define GET_Indexsv(disp) ((disp)->Indexsv)
#define SET_Indexsv(disp, fn) ((disp)->Indexsv = fn)
#define CALL_Normal3b(disp, parameters) (*((disp)->Normal3b)) parameters
#define GET_Normal3b(disp) ((disp)->Normal3b)
#define SET_Normal3b(disp, fn) ((disp)->Normal3b = fn)
#define CALL_Normal3bv(disp, parameters) (*((disp)->Normal3bv)) parameters
#define GET_Normal3bv(disp) ((disp)->Normal3bv)
#define SET_Normal3bv(disp, fn) ((disp)->Normal3bv = fn)
#define CALL_Normal3d(disp, parameters) (*((disp)->Normal3d)) parameters
#define GET_Normal3d(disp) ((disp)->Normal3d)
#define SET_Normal3d(disp, fn) ((disp)->Normal3d = fn)
#define CALL_Normal3dv(disp, parameters) (*((disp)->Normal3dv)) parameters
#define GET_Normal3dv(disp) ((disp)->Normal3dv)
#define SET_Normal3dv(disp, fn) ((disp)->Normal3dv = fn)
#define CALL_Normal3f(disp, parameters) (*((disp)->Normal3f)) parameters
#define GET_Normal3f(disp) ((disp)->Normal3f)
#define SET_Normal3f(disp, fn) ((disp)->Normal3f = fn)
#define CALL_Normal3fv(disp, parameters) (*((disp)->Normal3fv)) parameters
#define GET_Normal3fv(disp) ((disp)->Normal3fv)
#define SET_Normal3fv(disp, fn) ((disp)->Normal3fv = fn)
#define CALL_Normal3i(disp, parameters) (*((disp)->Normal3i)) parameters
#define GET_Normal3i(disp) ((disp)->Normal3i)
#define SET_Normal3i(disp, fn) ((disp)->Normal3i = fn)
#define CALL_Normal3iv(disp, parameters) (*((disp)->Normal3iv)) parameters
#define GET_Normal3iv(disp) ((disp)->Normal3iv)
#define SET_Normal3iv(disp, fn) ((disp)->Normal3iv = fn)
#define CALL_Normal3s(disp, parameters) (*((disp)->Normal3s)) parameters
#define GET_Normal3s(disp) ((disp)->Normal3s)
#define SET_Normal3s(disp, fn) ((disp)->Normal3s = fn)
#define CALL_Normal3sv(disp, parameters) (*((disp)->Normal3sv)) parameters
#define GET_Normal3sv(disp) ((disp)->Normal3sv)
#define SET_Normal3sv(disp, fn) ((disp)->Normal3sv = fn)
#define CALL_RasterPos2d(disp, parameters) (*((disp)->RasterPos2d)) parameters
#define GET_RasterPos2d(disp) ((disp)->RasterPos2d)
#define SET_RasterPos2d(disp, fn) ((disp)->RasterPos2d = fn)
#define CALL_RasterPos2dv(disp, parameters) (*((disp)->RasterPos2dv)) parameters
#define GET_RasterPos2dv(disp) ((disp)->RasterPos2dv)
#define SET_RasterPos2dv(disp, fn) ((disp)->RasterPos2dv = fn)
#define CALL_RasterPos2f(disp, parameters) (*((disp)->RasterPos2f)) parameters
#define GET_RasterPos2f(disp) ((disp)->RasterPos2f)
#define SET_RasterPos2f(disp, fn) ((disp)->RasterPos2f = fn)
#define CALL_RasterPos2fv(disp, parameters) (*((disp)->RasterPos2fv)) parameters
#define GET_RasterPos2fv(disp) ((disp)->RasterPos2fv)
#define SET_RasterPos2fv(disp, fn) ((disp)->RasterPos2fv = fn)
#define CALL_RasterPos2i(disp, parameters) (*((disp)->RasterPos2i)) parameters
#define GET_RasterPos2i(disp) ((disp)->RasterPos2i)
#define SET_RasterPos2i(disp, fn) ((disp)->RasterPos2i = fn)
#define CALL_RasterPos2iv(disp, parameters) (*((disp)->RasterPos2iv)) parameters
#define GET_RasterPos2iv(disp) ((disp)->RasterPos2iv)
#define SET_RasterPos2iv(disp, fn) ((disp)->RasterPos2iv = fn)
#define CALL_RasterPos2s(disp, parameters) (*((disp)->RasterPos2s)) parameters
#define GET_RasterPos2s(disp) ((disp)->RasterPos2s)
#define SET_RasterPos2s(disp, fn) ((disp)->RasterPos2s = fn)
#define CALL_RasterPos2sv(disp, parameters) (*((disp)->RasterPos2sv)) parameters
#define GET_RasterPos2sv(disp) ((disp)->RasterPos2sv)
#define SET_RasterPos2sv(disp, fn) ((disp)->RasterPos2sv = fn)
#define CALL_RasterPos3d(disp, parameters) (*((disp)->RasterPos3d)) parameters
#define GET_RasterPos3d(disp) ((disp)->RasterPos3d)
#define SET_RasterPos3d(disp, fn) ((disp)->RasterPos3d = fn)
#define CALL_RasterPos3dv(disp, parameters) (*((disp)->RasterPos3dv)) parameters
#define GET_RasterPos3dv(disp) ((disp)->RasterPos3dv)
#define SET_RasterPos3dv(disp, fn) ((disp)->RasterPos3dv = fn)
#define CALL_RasterPos3f(disp, parameters) (*((disp)->RasterPos3f)) parameters
#define GET_RasterPos3f(disp) ((disp)->RasterPos3f)
#define SET_RasterPos3f(disp, fn) ((disp)->RasterPos3f = fn)
#define CALL_RasterPos3fv(disp, parameters) (*((disp)->RasterPos3fv)) parameters
#define GET_RasterPos3fv(disp) ((disp)->RasterPos3fv)
#define SET_RasterPos3fv(disp, fn) ((disp)->RasterPos3fv = fn)
#define CALL_RasterPos3i(disp, parameters) (*((disp)->RasterPos3i)) parameters
#define GET_RasterPos3i(disp) ((disp)->RasterPos3i)
#define SET_RasterPos3i(disp, fn) ((disp)->RasterPos3i = fn)
#define CALL_RasterPos3iv(disp, parameters) (*((disp)->RasterPos3iv)) parameters
#define GET_RasterPos3iv(disp) ((disp)->RasterPos3iv)
#define SET_RasterPos3iv(disp, fn) ((disp)->RasterPos3iv = fn)
#define CALL_RasterPos3s(disp, parameters) (*((disp)->RasterPos3s)) parameters
#define GET_RasterPos3s(disp) ((disp)->RasterPos3s)
#define SET_RasterPos3s(disp, fn) ((disp)->RasterPos3s = fn)
#define CALL_RasterPos3sv(disp, parameters) (*((disp)->RasterPos3sv)) parameters
#define GET_RasterPos3sv(disp) ((disp)->RasterPos3sv)
#define SET_RasterPos3sv(disp, fn) ((disp)->RasterPos3sv = fn)
#define CALL_RasterPos4d(disp, parameters) (*((disp)->RasterPos4d)) parameters
#define GET_RasterPos4d(disp) ((disp)->RasterPos4d)
#define SET_RasterPos4d(disp, fn) ((disp)->RasterPos4d = fn)
#define CALL_RasterPos4dv(disp, parameters) (*((disp)->RasterPos4dv)) parameters
#define GET_RasterPos4dv(disp) ((disp)->RasterPos4dv)
#define SET_RasterPos4dv(disp, fn) ((disp)->RasterPos4dv = fn)
#define CALL_RasterPos4f(disp, parameters) (*((disp)->RasterPos4f)) parameters
#define GET_RasterPos4f(disp) ((disp)->RasterPos4f)
#define SET_RasterPos4f(disp, fn) ((disp)->RasterPos4f = fn)
#define CALL_RasterPos4fv(disp, parameters) (*((disp)->RasterPos4fv)) parameters
#define GET_RasterPos4fv(disp) ((disp)->RasterPos4fv)
#define SET_RasterPos4fv(disp, fn) ((disp)->RasterPos4fv = fn)
#define CALL_RasterPos4i(disp, parameters) (*((disp)->RasterPos4i)) parameters
#define GET_RasterPos4i(disp) ((disp)->RasterPos4i)
#define SET_RasterPos4i(disp, fn) ((disp)->RasterPos4i = fn)
#define CALL_RasterPos4iv(disp, parameters) (*((disp)->RasterPos4iv)) parameters
#define GET_RasterPos4iv(disp) ((disp)->RasterPos4iv)
#define SET_RasterPos4iv(disp, fn) ((disp)->RasterPos4iv = fn)
#define CALL_RasterPos4s(disp, parameters) (*((disp)->RasterPos4s)) parameters
#define GET_RasterPos4s(disp) ((disp)->RasterPos4s)
#define SET_RasterPos4s(disp, fn) ((disp)->RasterPos4s = fn)
#define CALL_RasterPos4sv(disp, parameters) (*((disp)->RasterPos4sv)) parameters
#define GET_RasterPos4sv(disp) ((disp)->RasterPos4sv)
#define SET_RasterPos4sv(disp, fn) ((disp)->RasterPos4sv = fn)
#define CALL_Rectd(disp, parameters) (*((disp)->Rectd)) parameters
#define GET_Rectd(disp) ((disp)->Rectd)
#define SET_Rectd(disp, fn) ((disp)->Rectd = fn)
#define CALL_Rectdv(disp, parameters) (*((disp)->Rectdv)) parameters
#define GET_Rectdv(disp) ((disp)->Rectdv)
#define SET_Rectdv(disp, fn) ((disp)->Rectdv = fn)
#define CALL_Rectf(disp, parameters) (*((disp)->Rectf)) parameters
#define GET_Rectf(disp) ((disp)->Rectf)
#define SET_Rectf(disp, fn) ((disp)->Rectf = fn)
#define CALL_Rectfv(disp, parameters) (*((disp)->Rectfv)) parameters
#define GET_Rectfv(disp) ((disp)->Rectfv)
#define SET_Rectfv(disp, fn) ((disp)->Rectfv = fn)
#define CALL_Recti(disp, parameters) (*((disp)->Recti)) parameters
#define GET_Recti(disp) ((disp)->Recti)
#define SET_Recti(disp, fn) ((disp)->Recti = fn)
#define CALL_Rectiv(disp, parameters) (*((disp)->Rectiv)) parameters
#define GET_Rectiv(disp) ((disp)->Rectiv)
#define SET_Rectiv(disp, fn) ((disp)->Rectiv = fn)
#define CALL_Rects(disp, parameters) (*((disp)->Rects)) parameters
#define GET_Rects(disp) ((disp)->Rects)
#define SET_Rects(disp, fn) ((disp)->Rects = fn)
#define CALL_Rectsv(disp, parameters) (*((disp)->Rectsv)) parameters
#define GET_Rectsv(disp) ((disp)->Rectsv)
#define SET_Rectsv(disp, fn) ((disp)->Rectsv = fn)
#define CALL_TexCoord1d(disp, parameters) (*((disp)->TexCoord1d)) parameters
#define GET_TexCoord1d(disp) ((disp)->TexCoord1d)
#define SET_TexCoord1d(disp, fn) ((disp)->TexCoord1d = fn)
#define CALL_TexCoord1dv(disp, parameters) (*((disp)->TexCoord1dv)) parameters
#define GET_TexCoord1dv(disp) ((disp)->TexCoord1dv)
#define SET_TexCoord1dv(disp, fn) ((disp)->TexCoord1dv = fn)
#define CALL_TexCoord1f(disp, parameters) (*((disp)->TexCoord1f)) parameters
#define GET_TexCoord1f(disp) ((disp)->TexCoord1f)
#define SET_TexCoord1f(disp, fn) ((disp)->TexCoord1f = fn)
#define CALL_TexCoord1fv(disp, parameters) (*((disp)->TexCoord1fv)) parameters
#define GET_TexCoord1fv(disp) ((disp)->TexCoord1fv)
#define SET_TexCoord1fv(disp, fn) ((disp)->TexCoord1fv = fn)
#define CALL_TexCoord1i(disp, parameters) (*((disp)->TexCoord1i)) parameters
#define GET_TexCoord1i(disp) ((disp)->TexCoord1i)
#define SET_TexCoord1i(disp, fn) ((disp)->TexCoord1i = fn)
#define CALL_TexCoord1iv(disp, parameters) (*((disp)->TexCoord1iv)) parameters
#define GET_TexCoord1iv(disp) ((disp)->TexCoord1iv)
#define SET_TexCoord1iv(disp, fn) ((disp)->TexCoord1iv = fn)
#define CALL_TexCoord1s(disp, parameters) (*((disp)->TexCoord1s)) parameters
#define GET_TexCoord1s(disp) ((disp)->TexCoord1s)
#define SET_TexCoord1s(disp, fn) ((disp)->TexCoord1s = fn)
#define CALL_TexCoord1sv(disp, parameters) (*((disp)->TexCoord1sv)) parameters
#define GET_TexCoord1sv(disp) ((disp)->TexCoord1sv)
#define SET_TexCoord1sv(disp, fn) ((disp)->TexCoord1sv = fn)
#define CALL_TexCoord2d(disp, parameters) (*((disp)->TexCoord2d)) parameters
#define GET_TexCoord2d(disp) ((disp)->TexCoord2d)
#define SET_TexCoord2d(disp, fn) ((disp)->TexCoord2d = fn)
#define CALL_TexCoord2dv(disp, parameters) (*((disp)->TexCoord2dv)) parameters
#define GET_TexCoord2dv(disp) ((disp)->TexCoord2dv)
#define SET_TexCoord2dv(disp, fn) ((disp)->TexCoord2dv = fn)
#define CALL_TexCoord2f(disp, parameters) (*((disp)->TexCoord2f)) parameters
#define GET_TexCoord2f(disp) ((disp)->TexCoord2f)
#define SET_TexCoord2f(disp, fn) ((disp)->TexCoord2f = fn)
#define CALL_TexCoord2fv(disp, parameters) (*((disp)->TexCoord2fv)) parameters
#define GET_TexCoord2fv(disp) ((disp)->TexCoord2fv)
#define SET_TexCoord2fv(disp, fn) ((disp)->TexCoord2fv = fn)
#define CALL_TexCoord2i(disp, parameters) (*((disp)->TexCoord2i)) parameters
#define GET_TexCoord2i(disp) ((disp)->TexCoord2i)
#define SET_TexCoord2i(disp, fn) ((disp)->TexCoord2i = fn)
#define CALL_TexCoord2iv(disp, parameters) (*((disp)->TexCoord2iv)) parameters
#define GET_TexCoord2iv(disp) ((disp)->TexCoord2iv)
#define SET_TexCoord2iv(disp, fn) ((disp)->TexCoord2iv = fn)
#define CALL_TexCoord2s(disp, parameters) (*((disp)->TexCoord2s)) parameters
#define GET_TexCoord2s(disp) ((disp)->TexCoord2s)
#define SET_TexCoord2s(disp, fn) ((disp)->TexCoord2s = fn)
#define CALL_TexCoord2sv(disp, parameters) (*((disp)->TexCoord2sv)) parameters
#define GET_TexCoord2sv(disp) ((disp)->TexCoord2sv)
#define SET_TexCoord2sv(disp, fn) ((disp)->TexCoord2sv = fn)
#define CALL_TexCoord3d(disp, parameters) (*((disp)->TexCoord3d)) parameters
#define GET_TexCoord3d(disp) ((disp)->TexCoord3d)
#define SET_TexCoord3d(disp, fn) ((disp)->TexCoord3d = fn)
#define CALL_TexCoord3dv(disp, parameters) (*((disp)->TexCoord3dv)) parameters
#define GET_TexCoord3dv(disp) ((disp)->TexCoord3dv)
#define SET_TexCoord3dv(disp, fn) ((disp)->TexCoord3dv = fn)
#define CALL_TexCoord3f(disp, parameters) (*((disp)->TexCoord3f)) parameters
#define GET_TexCoord3f(disp) ((disp)->TexCoord3f)
#define SET_TexCoord3f(disp, fn) ((disp)->TexCoord3f = fn)
#define CALL_TexCoord3fv(disp, parameters) (*((disp)->TexCoord3fv)) parameters
#define GET_TexCoord3fv(disp) ((disp)->TexCoord3fv)
#define SET_TexCoord3fv(disp, fn) ((disp)->TexCoord3fv = fn)
#define CALL_TexCoord3i(disp, parameters) (*((disp)->TexCoord3i)) parameters
#define GET_TexCoord3i(disp) ((disp)->TexCoord3i)
#define SET_TexCoord3i(disp, fn) ((disp)->TexCoord3i = fn)
#define CALL_TexCoord3iv(disp, parameters) (*((disp)->TexCoord3iv)) parameters
#define GET_TexCoord3iv(disp) ((disp)->TexCoord3iv)
#define SET_TexCoord3iv(disp, fn) ((disp)->TexCoord3iv = fn)
#define CALL_TexCoord3s(disp, parameters) (*((disp)->TexCoord3s)) parameters
#define GET_TexCoord3s(disp) ((disp)->TexCoord3s)
#define SET_TexCoord3s(disp, fn) ((disp)->TexCoord3s = fn)
#define CALL_TexCoord3sv(disp, parameters) (*((disp)->TexCoord3sv)) parameters
#define GET_TexCoord3sv(disp) ((disp)->TexCoord3sv)
#define SET_TexCoord3sv(disp, fn) ((disp)->TexCoord3sv = fn)
#define CALL_TexCoord4d(disp, parameters) (*((disp)->TexCoord4d)) parameters
#define GET_TexCoord4d(disp) ((disp)->TexCoord4d)
#define SET_TexCoord4d(disp, fn) ((disp)->TexCoord4d = fn)
#define CALL_TexCoord4dv(disp, parameters) (*((disp)->TexCoord4dv)) parameters
#define GET_TexCoord4dv(disp) ((disp)->TexCoord4dv)
#define SET_TexCoord4dv(disp, fn) ((disp)->TexCoord4dv = fn)
#define CALL_TexCoord4f(disp, parameters) (*((disp)->TexCoord4f)) parameters
#define GET_TexCoord4f(disp) ((disp)->TexCoord4f)
#define SET_TexCoord4f(disp, fn) ((disp)->TexCoord4f = fn)
#define CALL_TexCoord4fv(disp, parameters) (*((disp)->TexCoord4fv)) parameters
#define GET_TexCoord4fv(disp) ((disp)->TexCoord4fv)
#define SET_TexCoord4fv(disp, fn) ((disp)->TexCoord4fv = fn)
#define CALL_TexCoord4i(disp, parameters) (*((disp)->TexCoord4i)) parameters
#define GET_TexCoord4i(disp) ((disp)->TexCoord4i)
#define SET_TexCoord4i(disp, fn) ((disp)->TexCoord4i = fn)
#define CALL_TexCoord4iv(disp, parameters) (*((disp)->TexCoord4iv)) parameters
#define GET_TexCoord4iv(disp) ((disp)->TexCoord4iv)
#define SET_TexCoord4iv(disp, fn) ((disp)->TexCoord4iv = fn)
#define CALL_TexCoord4s(disp, parameters) (*((disp)->TexCoord4s)) parameters
#define GET_TexCoord4s(disp) ((disp)->TexCoord4s)
#define SET_TexCoord4s(disp, fn) ((disp)->TexCoord4s = fn)
#define CALL_TexCoord4sv(disp, parameters) (*((disp)->TexCoord4sv)) parameters
#define GET_TexCoord4sv(disp) ((disp)->TexCoord4sv)
#define SET_TexCoord4sv(disp, fn) ((disp)->TexCoord4sv = fn)
#define CALL_Vertex2d(disp, parameters) (*((disp)->Vertex2d)) parameters
#define GET_Vertex2d(disp) ((disp)->Vertex2d)
#define SET_Vertex2d(disp, fn) ((disp)->Vertex2d = fn)
#define CALL_Vertex2dv(disp, parameters) (*((disp)->Vertex2dv)) parameters
#define GET_Vertex2dv(disp) ((disp)->Vertex2dv)
#define SET_Vertex2dv(disp, fn) ((disp)->Vertex2dv = fn)
#define CALL_Vertex2f(disp, parameters) (*((disp)->Vertex2f)) parameters
#define GET_Vertex2f(disp) ((disp)->Vertex2f)
#define SET_Vertex2f(disp, fn) ((disp)->Vertex2f = fn)
#define CALL_Vertex2fv(disp, parameters) (*((disp)->Vertex2fv)) parameters
#define GET_Vertex2fv(disp) ((disp)->Vertex2fv)
#define SET_Vertex2fv(disp, fn) ((disp)->Vertex2fv = fn)
#define CALL_Vertex2i(disp, parameters) (*((disp)->Vertex2i)) parameters
#define GET_Vertex2i(disp) ((disp)->Vertex2i)
#define SET_Vertex2i(disp, fn) ((disp)->Vertex2i = fn)
#define CALL_Vertex2iv(disp, parameters) (*((disp)->Vertex2iv)) parameters
#define GET_Vertex2iv(disp) ((disp)->Vertex2iv)
#define SET_Vertex2iv(disp, fn) ((disp)->Vertex2iv = fn)
#define CALL_Vertex2s(disp, parameters) (*((disp)->Vertex2s)) parameters
#define GET_Vertex2s(disp) ((disp)->Vertex2s)
#define SET_Vertex2s(disp, fn) ((disp)->Vertex2s = fn)
#define CALL_Vertex2sv(disp, parameters) (*((disp)->Vertex2sv)) parameters
#define GET_Vertex2sv(disp) ((disp)->Vertex2sv)
#define SET_Vertex2sv(disp, fn) ((disp)->Vertex2sv = fn)
#define CALL_Vertex3d(disp, parameters) (*((disp)->Vertex3d)) parameters
#define GET_Vertex3d(disp) ((disp)->Vertex3d)
#define SET_Vertex3d(disp, fn) ((disp)->Vertex3d = fn)
#define CALL_Vertex3dv(disp, parameters) (*((disp)->Vertex3dv)) parameters
#define GET_Vertex3dv(disp) ((disp)->Vertex3dv)
#define SET_Vertex3dv(disp, fn) ((disp)->Vertex3dv = fn)
#define CALL_Vertex3f(disp, parameters) (*((disp)->Vertex3f)) parameters
#define GET_Vertex3f(disp) ((disp)->Vertex3f)
#define SET_Vertex3f(disp, fn) ((disp)->Vertex3f = fn)
#define CALL_Vertex3fv(disp, parameters) (*((disp)->Vertex3fv)) parameters
#define GET_Vertex3fv(disp) ((disp)->Vertex3fv)
#define SET_Vertex3fv(disp, fn) ((disp)->Vertex3fv = fn)
#define CALL_Vertex3i(disp, parameters) (*((disp)->Vertex3i)) parameters
#define GET_Vertex3i(disp) ((disp)->Vertex3i)
#define SET_Vertex3i(disp, fn) ((disp)->Vertex3i = fn)
#define CALL_Vertex3iv(disp, parameters) (*((disp)->Vertex3iv)) parameters
#define GET_Vertex3iv(disp) ((disp)->Vertex3iv)
#define SET_Vertex3iv(disp, fn) ((disp)->Vertex3iv = fn)
#define CALL_Vertex3s(disp, parameters) (*((disp)->Vertex3s)) parameters
#define GET_Vertex3s(disp) ((disp)->Vertex3s)
#define SET_Vertex3s(disp, fn) ((disp)->Vertex3s = fn)
#define CALL_Vertex3sv(disp, parameters) (*((disp)->Vertex3sv)) parameters
#define GET_Vertex3sv(disp) ((disp)->Vertex3sv)
#define SET_Vertex3sv(disp, fn) ((disp)->Vertex3sv = fn)
#define CALL_Vertex4d(disp, parameters) (*((disp)->Vertex4d)) parameters
#define GET_Vertex4d(disp) ((disp)->Vertex4d)
#define SET_Vertex4d(disp, fn) ((disp)->Vertex4d = fn)
#define CALL_Vertex4dv(disp, parameters) (*((disp)->Vertex4dv)) parameters
#define GET_Vertex4dv(disp) ((disp)->Vertex4dv)
#define SET_Vertex4dv(disp, fn) ((disp)->Vertex4dv = fn)
#define CALL_Vertex4f(disp, parameters) (*((disp)->Vertex4f)) parameters
#define GET_Vertex4f(disp) ((disp)->Vertex4f)
#define SET_Vertex4f(disp, fn) ((disp)->Vertex4f = fn)
#define CALL_Vertex4fv(disp, parameters) (*((disp)->Vertex4fv)) parameters
#define GET_Vertex4fv(disp) ((disp)->Vertex4fv)
#define SET_Vertex4fv(disp, fn) ((disp)->Vertex4fv = fn)
#define CALL_Vertex4i(disp, parameters) (*((disp)->Vertex4i)) parameters
#define GET_Vertex4i(disp) ((disp)->Vertex4i)
#define SET_Vertex4i(disp, fn) ((disp)->Vertex4i = fn)
#define CALL_Vertex4iv(disp, parameters) (*((disp)->Vertex4iv)) parameters
#define GET_Vertex4iv(disp) ((disp)->Vertex4iv)
#define SET_Vertex4iv(disp, fn) ((disp)->Vertex4iv = fn)
#define CALL_Vertex4s(disp, parameters) (*((disp)->Vertex4s)) parameters
#define GET_Vertex4s(disp) ((disp)->Vertex4s)
#define SET_Vertex4s(disp, fn) ((disp)->Vertex4s = fn)
#define CALL_Vertex4sv(disp, parameters) (*((disp)->Vertex4sv)) parameters
#define GET_Vertex4sv(disp) ((disp)->Vertex4sv)
#define SET_Vertex4sv(disp, fn) ((disp)->Vertex4sv = fn)
#define CALL_ClipPlane(disp, parameters) (*((disp)->ClipPlane)) parameters
#define GET_ClipPlane(disp) ((disp)->ClipPlane)
#define SET_ClipPlane(disp, fn) ((disp)->ClipPlane = fn)
#define CALL_ColorMaterial(disp, parameters) (*((disp)->ColorMaterial)) parameters
#define GET_ColorMaterial(disp) ((disp)->ColorMaterial)
#define SET_ColorMaterial(disp, fn) ((disp)->ColorMaterial = fn)
#define CALL_CullFace(disp, parameters) (*((disp)->CullFace)) parameters
#define GET_CullFace(disp) ((disp)->CullFace)
#define SET_CullFace(disp, fn) ((disp)->CullFace = fn)
#define CALL_Fogf(disp, parameters) (*((disp)->Fogf)) parameters
#define GET_Fogf(disp) ((disp)->Fogf)
#define SET_Fogf(disp, fn) ((disp)->Fogf = fn)
#define CALL_Fogfv(disp, parameters) (*((disp)->Fogfv)) parameters
#define GET_Fogfv(disp) ((disp)->Fogfv)
#define SET_Fogfv(disp, fn) ((disp)->Fogfv = fn)
#define CALL_Fogi(disp, parameters) (*((disp)->Fogi)) parameters
#define GET_Fogi(disp) ((disp)->Fogi)
#define SET_Fogi(disp, fn) ((disp)->Fogi = fn)
#define CALL_Fogiv(disp, parameters) (*((disp)->Fogiv)) parameters
#define GET_Fogiv(disp) ((disp)->Fogiv)
#define SET_Fogiv(disp, fn) ((disp)->Fogiv = fn)
#define CALL_FrontFace(disp, parameters) (*((disp)->FrontFace)) parameters
#define GET_FrontFace(disp) ((disp)->FrontFace)
#define SET_FrontFace(disp, fn) ((disp)->FrontFace = fn)
#define CALL_Hint(disp, parameters) (*((disp)->Hint)) parameters
#define GET_Hint(disp) ((disp)->Hint)
#define SET_Hint(disp, fn) ((disp)->Hint = fn)
#define CALL_Lightf(disp, parameters) (*((disp)->Lightf)) parameters
#define GET_Lightf(disp) ((disp)->Lightf)
#define SET_Lightf(disp, fn) ((disp)->Lightf = fn)
#define CALL_Lightfv(disp, parameters) (*((disp)->Lightfv)) parameters
#define GET_Lightfv(disp) ((disp)->Lightfv)
#define SET_Lightfv(disp, fn) ((disp)->Lightfv = fn)
#define CALL_Lighti(disp, parameters) (*((disp)->Lighti)) parameters
#define GET_Lighti(disp) ((disp)->Lighti)
#define SET_Lighti(disp, fn) ((disp)->Lighti = fn)
#define CALL_Lightiv(disp, parameters) (*((disp)->Lightiv)) parameters
#define GET_Lightiv(disp) ((disp)->Lightiv)
#define SET_Lightiv(disp, fn) ((disp)->Lightiv = fn)
#define CALL_LightModelf(disp, parameters) (*((disp)->LightModelf)) parameters
#define GET_LightModelf(disp) ((disp)->LightModelf)
#define SET_LightModelf(disp, fn) ((disp)->LightModelf = fn)
#define CALL_LightModelfv(disp, parameters) (*((disp)->LightModelfv)) parameters
#define GET_LightModelfv(disp) ((disp)->LightModelfv)
#define SET_LightModelfv(disp, fn) ((disp)->LightModelfv = fn)
#define CALL_LightModeli(disp, parameters) (*((disp)->LightModeli)) parameters
#define GET_LightModeli(disp) ((disp)->LightModeli)
#define SET_LightModeli(disp, fn) ((disp)->LightModeli = fn)
#define CALL_LightModeliv(disp, parameters) (*((disp)->LightModeliv)) parameters
#define GET_LightModeliv(disp) ((disp)->LightModeliv)
#define SET_LightModeliv(disp, fn) ((disp)->LightModeliv = fn)
#define CALL_LineStipple(disp, parameters) (*((disp)->LineStipple)) parameters
#define GET_LineStipple(disp) ((disp)->LineStipple)
#define SET_LineStipple(disp, fn) ((disp)->LineStipple = fn)
#define CALL_LineWidth(disp, parameters) (*((disp)->LineWidth)) parameters
#define GET_LineWidth(disp) ((disp)->LineWidth)
#define SET_LineWidth(disp, fn) ((disp)->LineWidth = fn)
#define CALL_Materialf(disp, parameters) (*((disp)->Materialf)) parameters
#define GET_Materialf(disp) ((disp)->Materialf)
#define SET_Materialf(disp, fn) ((disp)->Materialf = fn)
#define CALL_Materialfv(disp, parameters) (*((disp)->Materialfv)) parameters
#define GET_Materialfv(disp) ((disp)->Materialfv)
#define SET_Materialfv(disp, fn) ((disp)->Materialfv = fn)
#define CALL_Materiali(disp, parameters) (*((disp)->Materiali)) parameters
#define GET_Materiali(disp) ((disp)->Materiali)
#define SET_Materiali(disp, fn) ((disp)->Materiali = fn)
#define CALL_Materialiv(disp, parameters) (*((disp)->Materialiv)) parameters
#define GET_Materialiv(disp) ((disp)->Materialiv)
#define SET_Materialiv(disp, fn) ((disp)->Materialiv = fn)
#define CALL_PointSize(disp, parameters) (*((disp)->PointSize)) parameters
#define GET_PointSize(disp) ((disp)->PointSize)
#define SET_PointSize(disp, fn) ((disp)->PointSize = fn)
#define CALL_PolygonMode(disp, parameters) (*((disp)->PolygonMode)) parameters
#define GET_PolygonMode(disp) ((disp)->PolygonMode)
#define SET_PolygonMode(disp, fn) ((disp)->PolygonMode = fn)
#define CALL_PolygonStipple(disp, parameters) (*((disp)->PolygonStipple)) parameters
#define GET_PolygonStipple(disp) ((disp)->PolygonStipple)
#define SET_PolygonStipple(disp, fn) ((disp)->PolygonStipple = fn)
#define CALL_Scissor(disp, parameters) (*((disp)->Scissor)) parameters
#define GET_Scissor(disp) ((disp)->Scissor)
#define SET_Scissor(disp, fn) ((disp)->Scissor = fn)
#define CALL_ShadeModel(disp, parameters) (*((disp)->ShadeModel)) parameters
#define GET_ShadeModel(disp) ((disp)->ShadeModel)
#define SET_ShadeModel(disp, fn) ((disp)->ShadeModel = fn)
#define CALL_TexParameterf(disp, parameters) (*((disp)->TexParameterf)) parameters
#define GET_TexParameterf(disp) ((disp)->TexParameterf)
#define SET_TexParameterf(disp, fn) ((disp)->TexParameterf = fn)
#define CALL_TexParameterfv(disp, parameters) (*((disp)->TexParameterfv)) parameters
#define GET_TexParameterfv(disp) ((disp)->TexParameterfv)
#define SET_TexParameterfv(disp, fn) ((disp)->TexParameterfv = fn)
#define CALL_TexParameteri(disp, parameters) (*((disp)->TexParameteri)) parameters
#define GET_TexParameteri(disp) ((disp)->TexParameteri)
#define SET_TexParameteri(disp, fn) ((disp)->TexParameteri = fn)
#define CALL_TexParameteriv(disp, parameters) (*((disp)->TexParameteriv)) parameters
#define GET_TexParameteriv(disp) ((disp)->TexParameteriv)
#define SET_TexParameteriv(disp, fn) ((disp)->TexParameteriv = fn)
#define CALL_TexImage1D(disp, parameters) (*((disp)->TexImage1D)) parameters
#define GET_TexImage1D(disp) ((disp)->TexImage1D)
#define SET_TexImage1D(disp, fn) ((disp)->TexImage1D = fn)
#define CALL_TexImage2D(disp, parameters) (*((disp)->TexImage2D)) parameters
#define GET_TexImage2D(disp) ((disp)->TexImage2D)
#define SET_TexImage2D(disp, fn) ((disp)->TexImage2D = fn)
#define CALL_TexEnvf(disp, parameters) (*((disp)->TexEnvf)) parameters
#define GET_TexEnvf(disp) ((disp)->TexEnvf)
#define SET_TexEnvf(disp, fn) ((disp)->TexEnvf = fn)
#define CALL_TexEnvfv(disp, parameters) (*((disp)->TexEnvfv)) parameters
#define GET_TexEnvfv(disp) ((disp)->TexEnvfv)
#define SET_TexEnvfv(disp, fn) ((disp)->TexEnvfv = fn)
#define CALL_TexEnvi(disp, parameters) (*((disp)->TexEnvi)) parameters
#define GET_TexEnvi(disp) ((disp)->TexEnvi)
#define SET_TexEnvi(disp, fn) ((disp)->TexEnvi = fn)
#define CALL_TexEnviv(disp, parameters) (*((disp)->TexEnviv)) parameters
#define GET_TexEnviv(disp) ((disp)->TexEnviv)
#define SET_TexEnviv(disp, fn) ((disp)->TexEnviv = fn)
#define CALL_TexGend(disp, parameters) (*((disp)->TexGend)) parameters
#define GET_TexGend(disp) ((disp)->TexGend)
#define SET_TexGend(disp, fn) ((disp)->TexGend = fn)
#define CALL_TexGendv(disp, parameters) (*((disp)->TexGendv)) parameters
#define GET_TexGendv(disp) ((disp)->TexGendv)
#define SET_TexGendv(disp, fn) ((disp)->TexGendv = fn)
#define CALL_TexGenf(disp, parameters) (*((disp)->TexGenf)) parameters
#define GET_TexGenf(disp) ((disp)->TexGenf)
#define SET_TexGenf(disp, fn) ((disp)->TexGenf = fn)
#define CALL_TexGenfv(disp, parameters) (*((disp)->TexGenfv)) parameters
#define GET_TexGenfv(disp) ((disp)->TexGenfv)
#define SET_TexGenfv(disp, fn) ((disp)->TexGenfv = fn)
#define CALL_TexGeni(disp, parameters) (*((disp)->TexGeni)) parameters
#define GET_TexGeni(disp) ((disp)->TexGeni)
#define SET_TexGeni(disp, fn) ((disp)->TexGeni = fn)
#define CALL_TexGeniv(disp, parameters) (*((disp)->TexGeniv)) parameters
#define GET_TexGeniv(disp) ((disp)->TexGeniv)
#define SET_TexGeniv(disp, fn) ((disp)->TexGeniv = fn)
#define CALL_FeedbackBuffer(disp, parameters) (*((disp)->FeedbackBuffer)) parameters
#define GET_FeedbackBuffer(disp) ((disp)->FeedbackBuffer)
#define SET_FeedbackBuffer(disp, fn) ((disp)->FeedbackBuffer = fn)
#define CALL_SelectBuffer(disp, parameters) (*((disp)->SelectBuffer)) parameters
#define GET_SelectBuffer(disp) ((disp)->SelectBuffer)
#define SET_SelectBuffer(disp, fn) ((disp)->SelectBuffer = fn)
#define CALL_RenderMode(disp, parameters) (*((disp)->RenderMode)) parameters
#define GET_RenderMode(disp) ((disp)->RenderMode)
#define SET_RenderMode(disp, fn) ((disp)->RenderMode = fn)
#define CALL_InitNames(disp, parameters) (*((disp)->InitNames)) parameters
#define GET_InitNames(disp) ((disp)->InitNames)
#define SET_InitNames(disp, fn) ((disp)->InitNames = fn)
#define CALL_LoadName(disp, parameters) (*((disp)->LoadName)) parameters
#define GET_LoadName(disp) ((disp)->LoadName)
#define SET_LoadName(disp, fn) ((disp)->LoadName = fn)
#define CALL_PassThrough(disp, parameters) (*((disp)->PassThrough)) parameters
#define GET_PassThrough(disp) ((disp)->PassThrough)
#define SET_PassThrough(disp, fn) ((disp)->PassThrough = fn)
#define CALL_PopName(disp, parameters) (*((disp)->PopName)) parameters
#define GET_PopName(disp) ((disp)->PopName)
#define SET_PopName(disp, fn) ((disp)->PopName = fn)
#define CALL_PushName(disp, parameters) (*((disp)->PushName)) parameters
#define GET_PushName(disp) ((disp)->PushName)
#define SET_PushName(disp, fn) ((disp)->PushName = fn)
#define CALL_DrawBuffer(disp, parameters) (*((disp)->DrawBuffer)) parameters
#define GET_DrawBuffer(disp) ((disp)->DrawBuffer)
#define SET_DrawBuffer(disp, fn) ((disp)->DrawBuffer = fn)
#define CALL_Clear(disp, parameters) (*((disp)->Clear)) parameters
#define GET_Clear(disp) ((disp)->Clear)
#define SET_Clear(disp, fn) ((disp)->Clear = fn)
#define CALL_ClearAccum(disp, parameters) (*((disp)->ClearAccum)) parameters
#define GET_ClearAccum(disp) ((disp)->ClearAccum)
#define SET_ClearAccum(disp, fn) ((disp)->ClearAccum = fn)
#define CALL_ClearIndex(disp, parameters) (*((disp)->ClearIndex)) parameters
#define GET_ClearIndex(disp) ((disp)->ClearIndex)
#define SET_ClearIndex(disp, fn) ((disp)->ClearIndex = fn)
#define CALL_ClearColor(disp, parameters) (*((disp)->ClearColor)) parameters
#define GET_ClearColor(disp) ((disp)->ClearColor)
#define SET_ClearColor(disp, fn) ((disp)->ClearColor = fn)
#define CALL_ClearStencil(disp, parameters) (*((disp)->ClearStencil)) parameters
#define GET_ClearStencil(disp) ((disp)->ClearStencil)
#define SET_ClearStencil(disp, fn) ((disp)->ClearStencil = fn)
#define CALL_ClearDepth(disp, parameters) (*((disp)->ClearDepth)) parameters
#define GET_ClearDepth(disp) ((disp)->ClearDepth)
#define SET_ClearDepth(disp, fn) ((disp)->ClearDepth = fn)
#define CALL_StencilMask(disp, parameters) (*((disp)->StencilMask)) parameters
#define GET_StencilMask(disp) ((disp)->StencilMask)
#define SET_StencilMask(disp, fn) ((disp)->StencilMask = fn)
#define CALL_ColorMask(disp, parameters) (*((disp)->ColorMask)) parameters
#define GET_ColorMask(disp) ((disp)->ColorMask)
#define SET_ColorMask(disp, fn) ((disp)->ColorMask = fn)
#define CALL_DepthMask(disp, parameters) (*((disp)->DepthMask)) parameters
#define GET_DepthMask(disp) ((disp)->DepthMask)
#define SET_DepthMask(disp, fn) ((disp)->DepthMask = fn)
#define CALL_IndexMask(disp, parameters) (*((disp)->IndexMask)) parameters
#define GET_IndexMask(disp) ((disp)->IndexMask)
#define SET_IndexMask(disp, fn) ((disp)->IndexMask = fn)
#define CALL_Accum(disp, parameters) (*((disp)->Accum)) parameters
#define GET_Accum(disp) ((disp)->Accum)
#define SET_Accum(disp, fn) ((disp)->Accum = fn)
#define CALL_Disable(disp, parameters) (*((disp)->Disable)) parameters
#define GET_Disable(disp) ((disp)->Disable)
#define SET_Disable(disp, fn) ((disp)->Disable = fn)
#define CALL_Enable(disp, parameters) (*((disp)->Enable)) parameters
#define GET_Enable(disp) ((disp)->Enable)
#define SET_Enable(disp, fn) ((disp)->Enable = fn)
#define CALL_Finish(disp, parameters) (*((disp)->Finish)) parameters
#define GET_Finish(disp) ((disp)->Finish)
#define SET_Finish(disp, fn) ((disp)->Finish = fn)
#define CALL_Flush(disp, parameters) (*((disp)->Flush)) parameters
#define GET_Flush(disp) ((disp)->Flush)
#define SET_Flush(disp, fn) ((disp)->Flush = fn)
#define CALL_PopAttrib(disp, parameters) (*((disp)->PopAttrib)) parameters
#define GET_PopAttrib(disp) ((disp)->PopAttrib)
#define SET_PopAttrib(disp, fn) ((disp)->PopAttrib = fn)
#define CALL_PushAttrib(disp, parameters) (*((disp)->PushAttrib)) parameters
#define GET_PushAttrib(disp) ((disp)->PushAttrib)
#define SET_PushAttrib(disp, fn) ((disp)->PushAttrib = fn)
#define CALL_Map1d(disp, parameters) (*((disp)->Map1d)) parameters
#define GET_Map1d(disp) ((disp)->Map1d)
#define SET_Map1d(disp, fn) ((disp)->Map1d = fn)
#define CALL_Map1f(disp, parameters) (*((disp)->Map1f)) parameters
#define GET_Map1f(disp) ((disp)->Map1f)
#define SET_Map1f(disp, fn) ((disp)->Map1f = fn)
#define CALL_Map2d(disp, parameters) (*((disp)->Map2d)) parameters
#define GET_Map2d(disp) ((disp)->Map2d)
#define SET_Map2d(disp, fn) ((disp)->Map2d = fn)
#define CALL_Map2f(disp, parameters) (*((disp)->Map2f)) parameters
#define GET_Map2f(disp) ((disp)->Map2f)
#define SET_Map2f(disp, fn) ((disp)->Map2f = fn)
#define CALL_MapGrid1d(disp, parameters) (*((disp)->MapGrid1d)) parameters
#define GET_MapGrid1d(disp) ((disp)->MapGrid1d)
#define SET_MapGrid1d(disp, fn) ((disp)->MapGrid1d = fn)
#define CALL_MapGrid1f(disp, parameters) (*((disp)->MapGrid1f)) parameters
#define GET_MapGrid1f(disp) ((disp)->MapGrid1f)
#define SET_MapGrid1f(disp, fn) ((disp)->MapGrid1f = fn)
#define CALL_MapGrid2d(disp, parameters) (*((disp)->MapGrid2d)) parameters
#define GET_MapGrid2d(disp) ((disp)->MapGrid2d)
#define SET_MapGrid2d(disp, fn) ((disp)->MapGrid2d = fn)
#define CALL_MapGrid2f(disp, parameters) (*((disp)->MapGrid2f)) parameters
#define GET_MapGrid2f(disp) ((disp)->MapGrid2f)
#define SET_MapGrid2f(disp, fn) ((disp)->MapGrid2f = fn)
#define CALL_EvalCoord1d(disp, parameters) (*((disp)->EvalCoord1d)) parameters
#define GET_EvalCoord1d(disp) ((disp)->EvalCoord1d)
#define SET_EvalCoord1d(disp, fn) ((disp)->EvalCoord1d = fn)
#define CALL_EvalCoord1dv(disp, parameters) (*((disp)->EvalCoord1dv)) parameters
#define GET_EvalCoord1dv(disp) ((disp)->EvalCoord1dv)
#define SET_EvalCoord1dv(disp, fn) ((disp)->EvalCoord1dv = fn)
#define CALL_EvalCoord1f(disp, parameters) (*((disp)->EvalCoord1f)) parameters
#define GET_EvalCoord1f(disp) ((disp)->EvalCoord1f)
#define SET_EvalCoord1f(disp, fn) ((disp)->EvalCoord1f = fn)
#define CALL_EvalCoord1fv(disp, parameters) (*((disp)->EvalCoord1fv)) parameters
#define GET_EvalCoord1fv(disp) ((disp)->EvalCoord1fv)
#define SET_EvalCoord1fv(disp, fn) ((disp)->EvalCoord1fv = fn)
#define CALL_EvalCoord2d(disp, parameters) (*((disp)->EvalCoord2d)) parameters
#define GET_EvalCoord2d(disp) ((disp)->EvalCoord2d)
#define SET_EvalCoord2d(disp, fn) ((disp)->EvalCoord2d = fn)
#define CALL_EvalCoord2dv(disp, parameters) (*((disp)->EvalCoord2dv)) parameters
#define GET_EvalCoord2dv(disp) ((disp)->EvalCoord2dv)
#define SET_EvalCoord2dv(disp, fn) ((disp)->EvalCoord2dv = fn)
#define CALL_EvalCoord2f(disp, parameters) (*((disp)->EvalCoord2f)) parameters
#define GET_EvalCoord2f(disp) ((disp)->EvalCoord2f)
#define SET_EvalCoord2f(disp, fn) ((disp)->EvalCoord2f = fn)
#define CALL_EvalCoord2fv(disp, parameters) (*((disp)->EvalCoord2fv)) parameters
#define GET_EvalCoord2fv(disp) ((disp)->EvalCoord2fv)
#define SET_EvalCoord2fv(disp, fn) ((disp)->EvalCoord2fv = fn)
#define CALL_EvalMesh1(disp, parameters) (*((disp)->EvalMesh1)) parameters
#define GET_EvalMesh1(disp) ((disp)->EvalMesh1)
#define SET_EvalMesh1(disp, fn) ((disp)->EvalMesh1 = fn)
#define CALL_EvalPoint1(disp, parameters) (*((disp)->EvalPoint1)) parameters
#define GET_EvalPoint1(disp) ((disp)->EvalPoint1)
#define SET_EvalPoint1(disp, fn) ((disp)->EvalPoint1 = fn)
#define CALL_EvalMesh2(disp, parameters) (*((disp)->EvalMesh2)) parameters
#define GET_EvalMesh2(disp) ((disp)->EvalMesh2)
#define SET_EvalMesh2(disp, fn) ((disp)->EvalMesh2 = fn)
#define CALL_EvalPoint2(disp, parameters) (*((disp)->EvalPoint2)) parameters
#define GET_EvalPoint2(disp) ((disp)->EvalPoint2)
#define SET_EvalPoint2(disp, fn) ((disp)->EvalPoint2 = fn)
#define CALL_AlphaFunc(disp, parameters) (*((disp)->AlphaFunc)) parameters
#define GET_AlphaFunc(disp) ((disp)->AlphaFunc)
#define SET_AlphaFunc(disp, fn) ((disp)->AlphaFunc = fn)
#define CALL_BlendFunc(disp, parameters) (*((disp)->BlendFunc)) parameters
#define GET_BlendFunc(disp) ((disp)->BlendFunc)
#define SET_BlendFunc(disp, fn) ((disp)->BlendFunc = fn)
#define CALL_LogicOp(disp, parameters) (*((disp)->LogicOp)) parameters
#define GET_LogicOp(disp) ((disp)->LogicOp)
#define SET_LogicOp(disp, fn) ((disp)->LogicOp = fn)
#define CALL_StencilFunc(disp, parameters) (*((disp)->StencilFunc)) parameters
#define GET_StencilFunc(disp) ((disp)->StencilFunc)
#define SET_StencilFunc(disp, fn) ((disp)->StencilFunc = fn)
#define CALL_StencilOp(disp, parameters) (*((disp)->StencilOp)) parameters
#define GET_StencilOp(disp) ((disp)->StencilOp)
#define SET_StencilOp(disp, fn) ((disp)->StencilOp = fn)
#define CALL_DepthFunc(disp, parameters) (*((disp)->DepthFunc)) parameters
#define GET_DepthFunc(disp) ((disp)->DepthFunc)
#define SET_DepthFunc(disp, fn) ((disp)->DepthFunc = fn)
#define CALL_PixelZoom(disp, parameters) (*((disp)->PixelZoom)) parameters
#define GET_PixelZoom(disp) ((disp)->PixelZoom)
#define SET_PixelZoom(disp, fn) ((disp)->PixelZoom = fn)
#define CALL_PixelTransferf(disp, parameters) (*((disp)->PixelTransferf)) parameters
#define GET_PixelTransferf(disp) ((disp)->PixelTransferf)
#define SET_PixelTransferf(disp, fn) ((disp)->PixelTransferf = fn)
#define CALL_PixelTransferi(disp, parameters) (*((disp)->PixelTransferi)) parameters
#define GET_PixelTransferi(disp) ((disp)->PixelTransferi)
#define SET_PixelTransferi(disp, fn) ((disp)->PixelTransferi = fn)
#define CALL_PixelStoref(disp, parameters) (*((disp)->PixelStoref)) parameters
#define GET_PixelStoref(disp) ((disp)->PixelStoref)
#define SET_PixelStoref(disp, fn) ((disp)->PixelStoref = fn)
#define CALL_PixelStorei(disp, parameters) (*((disp)->PixelStorei)) parameters
#define GET_PixelStorei(disp) ((disp)->PixelStorei)
#define SET_PixelStorei(disp, fn) ((disp)->PixelStorei = fn)
#define CALL_PixelMapfv(disp, parameters) (*((disp)->PixelMapfv)) parameters
#define GET_PixelMapfv(disp) ((disp)->PixelMapfv)
#define SET_PixelMapfv(disp, fn) ((disp)->PixelMapfv = fn)
#define CALL_PixelMapuiv(disp, parameters) (*((disp)->PixelMapuiv)) parameters
#define GET_PixelMapuiv(disp) ((disp)->PixelMapuiv)
#define SET_PixelMapuiv(disp, fn) ((disp)->PixelMapuiv = fn)
#define CALL_PixelMapusv(disp, parameters) (*((disp)->PixelMapusv)) parameters
#define GET_PixelMapusv(disp) ((disp)->PixelMapusv)
#define SET_PixelMapusv(disp, fn) ((disp)->PixelMapusv = fn)
#define CALL_ReadBuffer(disp, parameters) (*((disp)->ReadBuffer)) parameters
#define GET_ReadBuffer(disp) ((disp)->ReadBuffer)
#define SET_ReadBuffer(disp, fn) ((disp)->ReadBuffer = fn)
#define CALL_CopyPixels(disp, parameters) (*((disp)->CopyPixels)) parameters
#define GET_CopyPixels(disp) ((disp)->CopyPixels)
#define SET_CopyPixels(disp, fn) ((disp)->CopyPixels = fn)
#define CALL_ReadPixels(disp, parameters) (*((disp)->ReadPixels)) parameters
#define GET_ReadPixels(disp) ((disp)->ReadPixels)
#define SET_ReadPixels(disp, fn) ((disp)->ReadPixels = fn)
#define CALL_DrawPixels(disp, parameters) (*((disp)->DrawPixels)) parameters
#define GET_DrawPixels(disp) ((disp)->DrawPixels)
#define SET_DrawPixels(disp, fn) ((disp)->DrawPixels = fn)
#define CALL_GetBooleanv(disp, parameters) (*((disp)->GetBooleanv)) parameters
#define GET_GetBooleanv(disp) ((disp)->GetBooleanv)
#define SET_GetBooleanv(disp, fn) ((disp)->GetBooleanv = fn)
#define CALL_GetClipPlane(disp, parameters) (*((disp)->GetClipPlane)) parameters
#define GET_GetClipPlane(disp) ((disp)->GetClipPlane)
#define SET_GetClipPlane(disp, fn) ((disp)->GetClipPlane = fn)
#define CALL_GetDoublev(disp, parameters) (*((disp)->GetDoublev)) parameters
#define GET_GetDoublev(disp) ((disp)->GetDoublev)
#define SET_GetDoublev(disp, fn) ((disp)->GetDoublev = fn)
#define CALL_GetError(disp, parameters) (*((disp)->GetError)) parameters
#define GET_GetError(disp) ((disp)->GetError)
#define SET_GetError(disp, fn) ((disp)->GetError = fn)
#define CALL_GetFloatv(disp, parameters) (*((disp)->GetFloatv)) parameters
#define GET_GetFloatv(disp) ((disp)->GetFloatv)
#define SET_GetFloatv(disp, fn) ((disp)->GetFloatv = fn)
#define CALL_GetIntegerv(disp, parameters) (*((disp)->GetIntegerv)) parameters
#define GET_GetIntegerv(disp) ((disp)->GetIntegerv)
#define SET_GetIntegerv(disp, fn) ((disp)->GetIntegerv = fn)
#define CALL_GetLightfv(disp, parameters) (*((disp)->GetLightfv)) parameters
#define GET_GetLightfv(disp) ((disp)->GetLightfv)
#define SET_GetLightfv(disp, fn) ((disp)->GetLightfv = fn)
#define CALL_GetLightiv(disp, parameters) (*((disp)->GetLightiv)) parameters
#define GET_GetLightiv(disp) ((disp)->GetLightiv)
#define SET_GetLightiv(disp, fn) ((disp)->GetLightiv = fn)
#define CALL_GetMapdv(disp, parameters) (*((disp)->GetMapdv)) parameters
#define GET_GetMapdv(disp) ((disp)->GetMapdv)
#define SET_GetMapdv(disp, fn) ((disp)->GetMapdv = fn)
#define CALL_GetMapfv(disp, parameters) (*((disp)->GetMapfv)) parameters
#define GET_GetMapfv(disp) ((disp)->GetMapfv)
#define SET_GetMapfv(disp, fn) ((disp)->GetMapfv = fn)
#define CALL_GetMapiv(disp, parameters) (*((disp)->GetMapiv)) parameters
#define GET_GetMapiv(disp) ((disp)->GetMapiv)
#define SET_GetMapiv(disp, fn) ((disp)->GetMapiv = fn)
#define CALL_GetMaterialfv(disp, parameters) (*((disp)->GetMaterialfv)) parameters
#define GET_GetMaterialfv(disp) ((disp)->GetMaterialfv)
#define SET_GetMaterialfv(disp, fn) ((disp)->GetMaterialfv = fn)
#define CALL_GetMaterialiv(disp, parameters) (*((disp)->GetMaterialiv)) parameters
#define GET_GetMaterialiv(disp) ((disp)->GetMaterialiv)
#define SET_GetMaterialiv(disp, fn) ((disp)->GetMaterialiv = fn)
#define CALL_GetPixelMapfv(disp, parameters) (*((disp)->GetPixelMapfv)) parameters
#define GET_GetPixelMapfv(disp) ((disp)->GetPixelMapfv)
#define SET_GetPixelMapfv(disp, fn) ((disp)->GetPixelMapfv = fn)
#define CALL_GetPixelMapuiv(disp, parameters) (*((disp)->GetPixelMapuiv)) parameters
#define GET_GetPixelMapuiv(disp) ((disp)->GetPixelMapuiv)
#define SET_GetPixelMapuiv(disp, fn) ((disp)->GetPixelMapuiv = fn)
#define CALL_GetPixelMapusv(disp, parameters) (*((disp)->GetPixelMapusv)) parameters
#define GET_GetPixelMapusv(disp) ((disp)->GetPixelMapusv)
#define SET_GetPixelMapusv(disp, fn) ((disp)->GetPixelMapusv = fn)
#define CALL_GetPolygonStipple(disp, parameters) (*((disp)->GetPolygonStipple)) parameters
#define GET_GetPolygonStipple(disp) ((disp)->GetPolygonStipple)
#define SET_GetPolygonStipple(disp, fn) ((disp)->GetPolygonStipple = fn)
#define CALL_GetString(disp, parameters) (*((disp)->GetString)) parameters
#define GET_GetString(disp) ((disp)->GetString)
#define SET_GetString(disp, fn) ((disp)->GetString = fn)
#define CALL_GetTexEnvfv(disp, parameters) (*((disp)->GetTexEnvfv)) parameters
#define GET_GetTexEnvfv(disp) ((disp)->GetTexEnvfv)
#define SET_GetTexEnvfv(disp, fn) ((disp)->GetTexEnvfv = fn)
#define CALL_GetTexEnviv(disp, parameters) (*((disp)->GetTexEnviv)) parameters
#define GET_GetTexEnviv(disp) ((disp)->GetTexEnviv)
#define SET_GetTexEnviv(disp, fn) ((disp)->GetTexEnviv = fn)
#define CALL_GetTexGendv(disp, parameters) (*((disp)->GetTexGendv)) parameters
#define GET_GetTexGendv(disp) ((disp)->GetTexGendv)
#define SET_GetTexGendv(disp, fn) ((disp)->GetTexGendv = fn)
#define CALL_GetTexGenfv(disp, parameters) (*((disp)->GetTexGenfv)) parameters
#define GET_GetTexGenfv(disp) ((disp)->GetTexGenfv)
#define SET_GetTexGenfv(disp, fn) ((disp)->GetTexGenfv = fn)
#define CALL_GetTexGeniv(disp, parameters) (*((disp)->GetTexGeniv)) parameters
#define GET_GetTexGeniv(disp) ((disp)->GetTexGeniv)
#define SET_GetTexGeniv(disp, fn) ((disp)->GetTexGeniv = fn)
#define CALL_GetTexImage(disp, parameters) (*((disp)->GetTexImage)) parameters
#define GET_GetTexImage(disp) ((disp)->GetTexImage)
#define SET_GetTexImage(disp, fn) ((disp)->GetTexImage = fn)
#define CALL_GetTexParameterfv(disp, parameters) (*((disp)->GetTexParameterfv)) parameters
#define GET_GetTexParameterfv(disp) ((disp)->GetTexParameterfv)
#define SET_GetTexParameterfv(disp, fn) ((disp)->GetTexParameterfv = fn)
#define CALL_GetTexParameteriv(disp, parameters) (*((disp)->GetTexParameteriv)) parameters
#define GET_GetTexParameteriv(disp) ((disp)->GetTexParameteriv)
#define SET_GetTexParameteriv(disp, fn) ((disp)->GetTexParameteriv = fn)
#define CALL_GetTexLevelParameterfv(disp, parameters) (*((disp)->GetTexLevelParameterfv)) parameters
#define GET_GetTexLevelParameterfv(disp) ((disp)->GetTexLevelParameterfv)
#define SET_GetTexLevelParameterfv(disp, fn) ((disp)->GetTexLevelParameterfv = fn)
#define CALL_GetTexLevelParameteriv(disp, parameters) (*((disp)->GetTexLevelParameteriv)) parameters
#define GET_GetTexLevelParameteriv(disp) ((disp)->GetTexLevelParameteriv)
#define SET_GetTexLevelParameteriv(disp, fn) ((disp)->GetTexLevelParameteriv = fn)
#define CALL_IsEnabled(disp, parameters) (*((disp)->IsEnabled)) parameters
#define GET_IsEnabled(disp) ((disp)->IsEnabled)
#define SET_IsEnabled(disp, fn) ((disp)->IsEnabled = fn)
#define CALL_IsList(disp, parameters) (*((disp)->IsList)) parameters
#define GET_IsList(disp) ((disp)->IsList)
#define SET_IsList(disp, fn) ((disp)->IsList = fn)
#define CALL_DepthRange(disp, parameters) (*((disp)->DepthRange)) parameters
#define GET_DepthRange(disp) ((disp)->DepthRange)
#define SET_DepthRange(disp, fn) ((disp)->DepthRange = fn)
#define CALL_Frustum(disp, parameters) (*((disp)->Frustum)) parameters
#define GET_Frustum(disp) ((disp)->Frustum)
#define SET_Frustum(disp, fn) ((disp)->Frustum = fn)
#define CALL_LoadIdentity(disp, parameters) (*((disp)->LoadIdentity)) parameters
#define GET_LoadIdentity(disp) ((disp)->LoadIdentity)
#define SET_LoadIdentity(disp, fn) ((disp)->LoadIdentity = fn)
#define CALL_LoadMatrixf(disp, parameters) (*((disp)->LoadMatrixf)) parameters
#define GET_LoadMatrixf(disp) ((disp)->LoadMatrixf)
#define SET_LoadMatrixf(disp, fn) ((disp)->LoadMatrixf = fn)
#define CALL_LoadMatrixd(disp, parameters) (*((disp)->LoadMatrixd)) parameters
#define GET_LoadMatrixd(disp) ((disp)->LoadMatrixd)
#define SET_LoadMatrixd(disp, fn) ((disp)->LoadMatrixd = fn)
#define CALL_MatrixMode(disp, parameters) (*((disp)->MatrixMode)) parameters
#define GET_MatrixMode(disp) ((disp)->MatrixMode)
#define SET_MatrixMode(disp, fn) ((disp)->MatrixMode = fn)
#define CALL_MultMatrixf(disp, parameters) (*((disp)->MultMatrixf)) parameters
#define GET_MultMatrixf(disp) ((disp)->MultMatrixf)
#define SET_MultMatrixf(disp, fn) ((disp)->MultMatrixf = fn)
#define CALL_MultMatrixd(disp, parameters) (*((disp)->MultMatrixd)) parameters
#define GET_MultMatrixd(disp) ((disp)->MultMatrixd)
#define SET_MultMatrixd(disp, fn) ((disp)->MultMatrixd = fn)
#define CALL_Ortho(disp, parameters) (*((disp)->Ortho)) parameters
#define GET_Ortho(disp) ((disp)->Ortho)
#define SET_Ortho(disp, fn) ((disp)->Ortho = fn)
#define CALL_PopMatrix(disp, parameters) (*((disp)->PopMatrix)) parameters
#define GET_PopMatrix(disp) ((disp)->PopMatrix)
#define SET_PopMatrix(disp, fn) ((disp)->PopMatrix = fn)
#define CALL_PushMatrix(disp, parameters) (*((disp)->PushMatrix)) parameters
#define GET_PushMatrix(disp) ((disp)->PushMatrix)
#define SET_PushMatrix(disp, fn) ((disp)->PushMatrix = fn)
#define CALL_Rotated(disp, parameters) (*((disp)->Rotated)) parameters
#define GET_Rotated(disp) ((disp)->Rotated)
#define SET_Rotated(disp, fn) ((disp)->Rotated = fn)
#define CALL_Rotatef(disp, parameters) (*((disp)->Rotatef)) parameters
#define GET_Rotatef(disp) ((disp)->Rotatef)
#define SET_Rotatef(disp, fn) ((disp)->Rotatef = fn)
#define CALL_Scaled(disp, parameters) (*((disp)->Scaled)) parameters
#define GET_Scaled(disp) ((disp)->Scaled)
#define SET_Scaled(disp, fn) ((disp)->Scaled = fn)
#define CALL_Scalef(disp, parameters) (*((disp)->Scalef)) parameters
#define GET_Scalef(disp) ((disp)->Scalef)
#define SET_Scalef(disp, fn) ((disp)->Scalef = fn)
#define CALL_Translated(disp, parameters) (*((disp)->Translated)) parameters
#define GET_Translated(disp) ((disp)->Translated)
#define SET_Translated(disp, fn) ((disp)->Translated = fn)
#define CALL_Translatef(disp, parameters) (*((disp)->Translatef)) parameters
#define GET_Translatef(disp) ((disp)->Translatef)
#define SET_Translatef(disp, fn) ((disp)->Translatef = fn)
#define CALL_Viewport(disp, parameters) (*((disp)->Viewport)) parameters
#define GET_Viewport(disp) ((disp)->Viewport)
#define SET_Viewport(disp, fn) ((disp)->Viewport = fn)
#define CALL_ArrayElement(disp, parameters) (*((disp)->ArrayElement)) parameters
#define GET_ArrayElement(disp) ((disp)->ArrayElement)
#define SET_ArrayElement(disp, fn) ((disp)->ArrayElement = fn)
#define CALL_BindTexture(disp, parameters) (*((disp)->BindTexture)) parameters
#define GET_BindTexture(disp) ((disp)->BindTexture)
#define SET_BindTexture(disp, fn) ((disp)->BindTexture = fn)
#define CALL_ColorPointer(disp, parameters) (*((disp)->ColorPointer)) parameters
#define GET_ColorPointer(disp) ((disp)->ColorPointer)
#define SET_ColorPointer(disp, fn) ((disp)->ColorPointer = fn)
#define CALL_DisableClientState(disp, parameters) (*((disp)->DisableClientState)) parameters
#define GET_DisableClientState(disp) ((disp)->DisableClientState)
#define SET_DisableClientState(disp, fn) ((disp)->DisableClientState = fn)
#define CALL_DrawArrays(disp, parameters) (*((disp)->DrawArrays)) parameters
#define GET_DrawArrays(disp) ((disp)->DrawArrays)
#define SET_DrawArrays(disp, fn) ((disp)->DrawArrays = fn)
#define CALL_DrawElements(disp, parameters) (*((disp)->DrawElements)) parameters
#define GET_DrawElements(disp) ((disp)->DrawElements)
#define SET_DrawElements(disp, fn) ((disp)->DrawElements = fn)
#define CALL_EdgeFlagPointer(disp, parameters) (*((disp)->EdgeFlagPointer)) parameters
#define GET_EdgeFlagPointer(disp) ((disp)->EdgeFlagPointer)
#define SET_EdgeFlagPointer(disp, fn) ((disp)->EdgeFlagPointer = fn)
#define CALL_EnableClientState(disp, parameters) (*((disp)->EnableClientState)) parameters
#define GET_EnableClientState(disp) ((disp)->EnableClientState)
#define SET_EnableClientState(disp, fn) ((disp)->EnableClientState = fn)
#define CALL_IndexPointer(disp, parameters) (*((disp)->IndexPointer)) parameters
#define GET_IndexPointer(disp) ((disp)->IndexPointer)
#define SET_IndexPointer(disp, fn) ((disp)->IndexPointer = fn)
#define CALL_Indexub(disp, parameters) (*((disp)->Indexub)) parameters
#define GET_Indexub(disp) ((disp)->Indexub)
#define SET_Indexub(disp, fn) ((disp)->Indexub = fn)
#define CALL_Indexubv(disp, parameters) (*((disp)->Indexubv)) parameters
#define GET_Indexubv(disp) ((disp)->Indexubv)
#define SET_Indexubv(disp, fn) ((disp)->Indexubv = fn)
#define CALL_InterleavedArrays(disp, parameters) (*((disp)->InterleavedArrays)) parameters
#define GET_InterleavedArrays(disp) ((disp)->InterleavedArrays)
#define SET_InterleavedArrays(disp, fn) ((disp)->InterleavedArrays = fn)
#define CALL_NormalPointer(disp, parameters) (*((disp)->NormalPointer)) parameters
#define GET_NormalPointer(disp) ((disp)->NormalPointer)
#define SET_NormalPointer(disp, fn) ((disp)->NormalPointer = fn)
#define CALL_PolygonOffset(disp, parameters) (*((disp)->PolygonOffset)) parameters
#define GET_PolygonOffset(disp) ((disp)->PolygonOffset)
#define SET_PolygonOffset(disp, fn) ((disp)->PolygonOffset = fn)
#define CALL_TexCoordPointer(disp, parameters) (*((disp)->TexCoordPointer)) parameters
#define GET_TexCoordPointer(disp) ((disp)->TexCoordPointer)
#define SET_TexCoordPointer(disp, fn) ((disp)->TexCoordPointer = fn)
#define CALL_VertexPointer(disp, parameters) (*((disp)->VertexPointer)) parameters
#define GET_VertexPointer(disp) ((disp)->VertexPointer)
#define SET_VertexPointer(disp, fn) ((disp)->VertexPointer = fn)
#define CALL_AreTexturesResident(disp, parameters) (*((disp)->AreTexturesResident)) parameters
#define GET_AreTexturesResident(disp) ((disp)->AreTexturesResident)
#define SET_AreTexturesResident(disp, fn) ((disp)->AreTexturesResident = fn)
#define CALL_CopyTexImage1D(disp, parameters) (*((disp)->CopyTexImage1D)) parameters
#define GET_CopyTexImage1D(disp) ((disp)->CopyTexImage1D)
#define SET_CopyTexImage1D(disp, fn) ((disp)->CopyTexImage1D = fn)
#define CALL_CopyTexImage2D(disp, parameters) (*((disp)->CopyTexImage2D)) parameters
#define GET_CopyTexImage2D(disp) ((disp)->CopyTexImage2D)
#define SET_CopyTexImage2D(disp, fn) ((disp)->CopyTexImage2D = fn)
#define CALL_CopyTexSubImage1D(disp, parameters) (*((disp)->CopyTexSubImage1D)) parameters
#define GET_CopyTexSubImage1D(disp) ((disp)->CopyTexSubImage1D)
#define SET_CopyTexSubImage1D(disp, fn) ((disp)->CopyTexSubImage1D = fn)
#define CALL_CopyTexSubImage2D(disp, parameters) (*((disp)->CopyTexSubImage2D)) parameters
#define GET_CopyTexSubImage2D(disp) ((disp)->CopyTexSubImage2D)
#define SET_CopyTexSubImage2D(disp, fn) ((disp)->CopyTexSubImage2D = fn)
#define CALL_DeleteTextures(disp, parameters) (*((disp)->DeleteTextures)) parameters
#define GET_DeleteTextures(disp) ((disp)->DeleteTextures)
#define SET_DeleteTextures(disp, fn) ((disp)->DeleteTextures = fn)
#define CALL_GenTextures(disp, parameters) (*((disp)->GenTextures)) parameters
#define GET_GenTextures(disp) ((disp)->GenTextures)
#define SET_GenTextures(disp, fn) ((disp)->GenTextures = fn)
#define CALL_GetPointerv(disp, parameters) (*((disp)->GetPointerv)) parameters
#define GET_GetPointerv(disp) ((disp)->GetPointerv)
#define SET_GetPointerv(disp, fn) ((disp)->GetPointerv = fn)
#define CALL_IsTexture(disp, parameters) (*((disp)->IsTexture)) parameters
#define GET_IsTexture(disp) ((disp)->IsTexture)
#define SET_IsTexture(disp, fn) ((disp)->IsTexture = fn)
#define CALL_PrioritizeTextures(disp, parameters) (*((disp)->PrioritizeTextures)) parameters
#define GET_PrioritizeTextures(disp) ((disp)->PrioritizeTextures)
#define SET_PrioritizeTextures(disp, fn) ((disp)->PrioritizeTextures = fn)
#define CALL_TexSubImage1D(disp, parameters) (*((disp)->TexSubImage1D)) parameters
#define GET_TexSubImage1D(disp) ((disp)->TexSubImage1D)
#define SET_TexSubImage1D(disp, fn) ((disp)->TexSubImage1D = fn)
#define CALL_TexSubImage2D(disp, parameters) (*((disp)->TexSubImage2D)) parameters
#define GET_TexSubImage2D(disp) ((disp)->TexSubImage2D)
#define SET_TexSubImage2D(disp, fn) ((disp)->TexSubImage2D = fn)
#define CALL_PopClientAttrib(disp, parameters) (*((disp)->PopClientAttrib)) parameters
#define GET_PopClientAttrib(disp) ((disp)->PopClientAttrib)
#define SET_PopClientAttrib(disp, fn) ((disp)->PopClientAttrib = fn)
#define CALL_PushClientAttrib(disp, parameters) (*((disp)->PushClientAttrib)) parameters
#define GET_PushClientAttrib(disp) ((disp)->PushClientAttrib)
#define SET_PushClientAttrib(disp, fn) ((disp)->PushClientAttrib = fn)
#define CALL_BlendColor(disp, parameters) (*((disp)->BlendColor)) parameters
#define GET_BlendColor(disp) ((disp)->BlendColor)
#define SET_BlendColor(disp, fn) ((disp)->BlendColor = fn)
#define CALL_BlendEquation(disp, parameters) (*((disp)->BlendEquation)) parameters
#define GET_BlendEquation(disp) ((disp)->BlendEquation)
#define SET_BlendEquation(disp, fn) ((disp)->BlendEquation = fn)
#define CALL_DrawRangeElements(disp, parameters) (*((disp)->DrawRangeElements)) parameters
#define GET_DrawRangeElements(disp) ((disp)->DrawRangeElements)
#define SET_DrawRangeElements(disp, fn) ((disp)->DrawRangeElements = fn)
#define CALL_ColorTable(disp, parameters) (*((disp)->ColorTable)) parameters
#define GET_ColorTable(disp) ((disp)->ColorTable)
#define SET_ColorTable(disp, fn) ((disp)->ColorTable = fn)
#define CALL_ColorTableParameterfv(disp, parameters) (*((disp)->ColorTableParameterfv)) parameters
#define GET_ColorTableParameterfv(disp) ((disp)->ColorTableParameterfv)
#define SET_ColorTableParameterfv(disp, fn) ((disp)->ColorTableParameterfv = fn)
#define CALL_ColorTableParameteriv(disp, parameters) (*((disp)->ColorTableParameteriv)) parameters
#define GET_ColorTableParameteriv(disp) ((disp)->ColorTableParameteriv)
#define SET_ColorTableParameteriv(disp, fn) ((disp)->ColorTableParameteriv = fn)
#define CALL_CopyColorTable(disp, parameters) (*((disp)->CopyColorTable)) parameters
#define GET_CopyColorTable(disp) ((disp)->CopyColorTable)
#define SET_CopyColorTable(disp, fn) ((disp)->CopyColorTable = fn)
#define CALL_GetColorTable(disp, parameters) (*((disp)->GetColorTable)) parameters
#define GET_GetColorTable(disp) ((disp)->GetColorTable)
#define SET_GetColorTable(disp, fn) ((disp)->GetColorTable = fn)
#define CALL_GetColorTableParameterfv(disp, parameters) (*((disp)->GetColorTableParameterfv)) parameters
#define GET_GetColorTableParameterfv(disp) ((disp)->GetColorTableParameterfv)
#define SET_GetColorTableParameterfv(disp, fn) ((disp)->GetColorTableParameterfv = fn)
#define CALL_GetColorTableParameteriv(disp, parameters) (*((disp)->GetColorTableParameteriv)) parameters
#define GET_GetColorTableParameteriv(disp) ((disp)->GetColorTableParameteriv)
#define SET_GetColorTableParameteriv(disp, fn) ((disp)->GetColorTableParameteriv = fn)
#define CALL_ColorSubTable(disp, parameters) (*((disp)->ColorSubTable)) parameters
#define GET_ColorSubTable(disp) ((disp)->ColorSubTable)
#define SET_ColorSubTable(disp, fn) ((disp)->ColorSubTable = fn)
#define CALL_CopyColorSubTable(disp, parameters) (*((disp)->CopyColorSubTable)) parameters
#define GET_CopyColorSubTable(disp) ((disp)->CopyColorSubTable)
#define SET_CopyColorSubTable(disp, fn) ((disp)->CopyColorSubTable = fn)
#define CALL_ConvolutionFilter1D(disp, parameters) (*((disp)->ConvolutionFilter1D)) parameters
#define GET_ConvolutionFilter1D(disp) ((disp)->ConvolutionFilter1D)
#define SET_ConvolutionFilter1D(disp, fn) ((disp)->ConvolutionFilter1D = fn)
#define CALL_ConvolutionFilter2D(disp, parameters) (*((disp)->ConvolutionFilter2D)) parameters
#define GET_ConvolutionFilter2D(disp) ((disp)->ConvolutionFilter2D)
#define SET_ConvolutionFilter2D(disp, fn) ((disp)->ConvolutionFilter2D = fn)
#define CALL_ConvolutionParameterf(disp, parameters) (*((disp)->ConvolutionParameterf)) parameters
#define GET_ConvolutionParameterf(disp) ((disp)->ConvolutionParameterf)
#define SET_ConvolutionParameterf(disp, fn) ((disp)->ConvolutionParameterf = fn)
#define CALL_ConvolutionParameterfv(disp, parameters) (*((disp)->ConvolutionParameterfv)) parameters
#define GET_ConvolutionParameterfv(disp) ((disp)->ConvolutionParameterfv)
#define SET_ConvolutionParameterfv(disp, fn) ((disp)->ConvolutionParameterfv = fn)
#define CALL_ConvolutionParameteri(disp, parameters) (*((disp)->ConvolutionParameteri)) parameters
#define GET_ConvolutionParameteri(disp) ((disp)->ConvolutionParameteri)
#define SET_ConvolutionParameteri(disp, fn) ((disp)->ConvolutionParameteri = fn)
#define CALL_ConvolutionParameteriv(disp, parameters) (*((disp)->ConvolutionParameteriv)) parameters
#define GET_ConvolutionParameteriv(disp) ((disp)->ConvolutionParameteriv)
#define SET_ConvolutionParameteriv(disp, fn) ((disp)->ConvolutionParameteriv = fn)
#define CALL_CopyConvolutionFilter1D(disp, parameters) (*((disp)->CopyConvolutionFilter1D)) parameters
#define GET_CopyConvolutionFilter1D(disp) ((disp)->CopyConvolutionFilter1D)
#define SET_CopyConvolutionFilter1D(disp, fn) ((disp)->CopyConvolutionFilter1D = fn)
#define CALL_CopyConvolutionFilter2D(disp, parameters) (*((disp)->CopyConvolutionFilter2D)) parameters
#define GET_CopyConvolutionFilter2D(disp) ((disp)->CopyConvolutionFilter2D)
#define SET_CopyConvolutionFilter2D(disp, fn) ((disp)->CopyConvolutionFilter2D = fn)
#define CALL_GetConvolutionFilter(disp, parameters) (*((disp)->GetConvolutionFilter)) parameters
#define GET_GetConvolutionFilter(disp) ((disp)->GetConvolutionFilter)
#define SET_GetConvolutionFilter(disp, fn) ((disp)->GetConvolutionFilter = fn)
#define CALL_GetConvolutionParameterfv(disp, parameters) (*((disp)->GetConvolutionParameterfv)) parameters
#define GET_GetConvolutionParameterfv(disp) ((disp)->GetConvolutionParameterfv)
#define SET_GetConvolutionParameterfv(disp, fn) ((disp)->GetConvolutionParameterfv = fn)
#define CALL_GetConvolutionParameteriv(disp, parameters) (*((disp)->GetConvolutionParameteriv)) parameters
#define GET_GetConvolutionParameteriv(disp) ((disp)->GetConvolutionParameteriv)
#define SET_GetConvolutionParameteriv(disp, fn) ((disp)->GetConvolutionParameteriv = fn)
#define CALL_GetSeparableFilter(disp, parameters) (*((disp)->GetSeparableFilter)) parameters
#define GET_GetSeparableFilter(disp) ((disp)->GetSeparableFilter)
#define SET_GetSeparableFilter(disp, fn) ((disp)->GetSeparableFilter = fn)
#define CALL_SeparableFilter2D(disp, parameters) (*((disp)->SeparableFilter2D)) parameters
#define GET_SeparableFilter2D(disp) ((disp)->SeparableFilter2D)
#define SET_SeparableFilter2D(disp, fn) ((disp)->SeparableFilter2D = fn)
#define CALL_GetHistogram(disp, parameters) (*((disp)->GetHistogram)) parameters
#define GET_GetHistogram(disp) ((disp)->GetHistogram)
#define SET_GetHistogram(disp, fn) ((disp)->GetHistogram = fn)
#define CALL_GetHistogramParameterfv(disp, parameters) (*((disp)->GetHistogramParameterfv)) parameters
#define GET_GetHistogramParameterfv(disp) ((disp)->GetHistogramParameterfv)
#define SET_GetHistogramParameterfv(disp, fn) ((disp)->GetHistogramParameterfv = fn)
#define CALL_GetHistogramParameteriv(disp, parameters) (*((disp)->GetHistogramParameteriv)) parameters
#define GET_GetHistogramParameteriv(disp) ((disp)->GetHistogramParameteriv)
#define SET_GetHistogramParameteriv(disp, fn) ((disp)->GetHistogramParameteriv = fn)
#define CALL_GetMinmax(disp, parameters) (*((disp)->GetMinmax)) parameters
#define GET_GetMinmax(disp) ((disp)->GetMinmax)
#define SET_GetMinmax(disp, fn) ((disp)->GetMinmax = fn)
#define CALL_GetMinmaxParameterfv(disp, parameters) (*((disp)->GetMinmaxParameterfv)) parameters
#define GET_GetMinmaxParameterfv(disp) ((disp)->GetMinmaxParameterfv)
#define SET_GetMinmaxParameterfv(disp, fn) ((disp)->GetMinmaxParameterfv = fn)
#define CALL_GetMinmaxParameteriv(disp, parameters) (*((disp)->GetMinmaxParameteriv)) parameters
#define GET_GetMinmaxParameteriv(disp) ((disp)->GetMinmaxParameteriv)
#define SET_GetMinmaxParameteriv(disp, fn) ((disp)->GetMinmaxParameteriv = fn)
#define CALL_Histogram(disp, parameters) (*((disp)->Histogram)) parameters
#define GET_Histogram(disp) ((disp)->Histogram)
#define SET_Histogram(disp, fn) ((disp)->Histogram = fn)
#define CALL_Minmax(disp, parameters) (*((disp)->Minmax)) parameters
#define GET_Minmax(disp) ((disp)->Minmax)
#define SET_Minmax(disp, fn) ((disp)->Minmax = fn)
#define CALL_ResetHistogram(disp, parameters) (*((disp)->ResetHistogram)) parameters
#define GET_ResetHistogram(disp) ((disp)->ResetHistogram)
#define SET_ResetHistogram(disp, fn) ((disp)->ResetHistogram = fn)
#define CALL_ResetMinmax(disp, parameters) (*((disp)->ResetMinmax)) parameters
#define GET_ResetMinmax(disp) ((disp)->ResetMinmax)
#define SET_ResetMinmax(disp, fn) ((disp)->ResetMinmax = fn)
#define CALL_TexImage3D(disp, parameters) (*((disp)->TexImage3D)) parameters
#define GET_TexImage3D(disp) ((disp)->TexImage3D)
#define SET_TexImage3D(disp, fn) ((disp)->TexImage3D = fn)
#define CALL_TexSubImage3D(disp, parameters) (*((disp)->TexSubImage3D)) parameters
#define GET_TexSubImage3D(disp) ((disp)->TexSubImage3D)
#define SET_TexSubImage3D(disp, fn) ((disp)->TexSubImage3D = fn)
#define CALL_CopyTexSubImage3D(disp, parameters) (*((disp)->CopyTexSubImage3D)) parameters
#define GET_CopyTexSubImage3D(disp) ((disp)->CopyTexSubImage3D)
#define SET_CopyTexSubImage3D(disp, fn) ((disp)->CopyTexSubImage3D = fn)
#define CALL_ActiveTextureARB(disp, parameters) (*((disp)->ActiveTextureARB)) parameters
#define GET_ActiveTextureARB(disp) ((disp)->ActiveTextureARB)
#define SET_ActiveTextureARB(disp, fn) ((disp)->ActiveTextureARB = fn)
#define CALL_ClientActiveTextureARB(disp, parameters) (*((disp)->ClientActiveTextureARB)) parameters
#define GET_ClientActiveTextureARB(disp) ((disp)->ClientActiveTextureARB)
#define SET_ClientActiveTextureARB(disp, fn) ((disp)->ClientActiveTextureARB = fn)
#define CALL_MultiTexCoord1dARB(disp, parameters) (*((disp)->MultiTexCoord1dARB)) parameters
#define GET_MultiTexCoord1dARB(disp) ((disp)->MultiTexCoord1dARB)
#define SET_MultiTexCoord1dARB(disp, fn) ((disp)->MultiTexCoord1dARB = fn)
#define CALL_MultiTexCoord1dvARB(disp, parameters) (*((disp)->MultiTexCoord1dvARB)) parameters
#define GET_MultiTexCoord1dvARB(disp) ((disp)->MultiTexCoord1dvARB)
#define SET_MultiTexCoord1dvARB(disp, fn) ((disp)->MultiTexCoord1dvARB = fn)
#define CALL_MultiTexCoord1fARB(disp, parameters) (*((disp)->MultiTexCoord1fARB)) parameters
#define GET_MultiTexCoord1fARB(disp) ((disp)->MultiTexCoord1fARB)
#define SET_MultiTexCoord1fARB(disp, fn) ((disp)->MultiTexCoord1fARB = fn)
#define CALL_MultiTexCoord1fvARB(disp, parameters) (*((disp)->MultiTexCoord1fvARB)) parameters
#define GET_MultiTexCoord1fvARB(disp) ((disp)->MultiTexCoord1fvARB)
#define SET_MultiTexCoord1fvARB(disp, fn) ((disp)->MultiTexCoord1fvARB = fn)
#define CALL_MultiTexCoord1iARB(disp, parameters) (*((disp)->MultiTexCoord1iARB)) parameters
#define GET_MultiTexCoord1iARB(disp) ((disp)->MultiTexCoord1iARB)
#define SET_MultiTexCoord1iARB(disp, fn) ((disp)->MultiTexCoord1iARB = fn)
#define CALL_MultiTexCoord1ivARB(disp, parameters) (*((disp)->MultiTexCoord1ivARB)) parameters
#define GET_MultiTexCoord1ivARB(disp) ((disp)->MultiTexCoord1ivARB)
#define SET_MultiTexCoord1ivARB(disp, fn) ((disp)->MultiTexCoord1ivARB = fn)
#define CALL_MultiTexCoord1sARB(disp, parameters) (*((disp)->MultiTexCoord1sARB)) parameters
#define GET_MultiTexCoord1sARB(disp) ((disp)->MultiTexCoord1sARB)
#define SET_MultiTexCoord1sARB(disp, fn) ((disp)->MultiTexCoord1sARB = fn)
#define CALL_MultiTexCoord1svARB(disp, parameters) (*((disp)->MultiTexCoord1svARB)) parameters
#define GET_MultiTexCoord1svARB(disp) ((disp)->MultiTexCoord1svARB)
#define SET_MultiTexCoord1svARB(disp, fn) ((disp)->MultiTexCoord1svARB = fn)
#define CALL_MultiTexCoord2dARB(disp, parameters) (*((disp)->MultiTexCoord2dARB)) parameters
#define GET_MultiTexCoord2dARB(disp) ((disp)->MultiTexCoord2dARB)
#define SET_MultiTexCoord2dARB(disp, fn) ((disp)->MultiTexCoord2dARB = fn)
#define CALL_MultiTexCoord2dvARB(disp, parameters) (*((disp)->MultiTexCoord2dvARB)) parameters
#define GET_MultiTexCoord2dvARB(disp) ((disp)->MultiTexCoord2dvARB)
#define SET_MultiTexCoord2dvARB(disp, fn) ((disp)->MultiTexCoord2dvARB = fn)
#define CALL_MultiTexCoord2fARB(disp, parameters) (*((disp)->MultiTexCoord2fARB)) parameters
#define GET_MultiTexCoord2fARB(disp) ((disp)->MultiTexCoord2fARB)
#define SET_MultiTexCoord2fARB(disp, fn) ((disp)->MultiTexCoord2fARB = fn)
#define CALL_MultiTexCoord2fvARB(disp, parameters) (*((disp)->MultiTexCoord2fvARB)) parameters
#define GET_MultiTexCoord2fvARB(disp) ((disp)->MultiTexCoord2fvARB)
#define SET_MultiTexCoord2fvARB(disp, fn) ((disp)->MultiTexCoord2fvARB = fn)
#define CALL_MultiTexCoord2iARB(disp, parameters) (*((disp)->MultiTexCoord2iARB)) parameters
#define GET_MultiTexCoord2iARB(disp) ((disp)->MultiTexCoord2iARB)
#define SET_MultiTexCoord2iARB(disp, fn) ((disp)->MultiTexCoord2iARB = fn)
#define CALL_MultiTexCoord2ivARB(disp, parameters) (*((disp)->MultiTexCoord2ivARB)) parameters
#define GET_MultiTexCoord2ivARB(disp) ((disp)->MultiTexCoord2ivARB)
#define SET_MultiTexCoord2ivARB(disp, fn) ((disp)->MultiTexCoord2ivARB = fn)
#define CALL_MultiTexCoord2sARB(disp, parameters) (*((disp)->MultiTexCoord2sARB)) parameters
#define GET_MultiTexCoord2sARB(disp) ((disp)->MultiTexCoord2sARB)
#define SET_MultiTexCoord2sARB(disp, fn) ((disp)->MultiTexCoord2sARB = fn)
#define CALL_MultiTexCoord2svARB(disp, parameters) (*((disp)->MultiTexCoord2svARB)) parameters
#define GET_MultiTexCoord2svARB(disp) ((disp)->MultiTexCoord2svARB)
#define SET_MultiTexCoord2svARB(disp, fn) ((disp)->MultiTexCoord2svARB = fn)
#define CALL_MultiTexCoord3dARB(disp, parameters) (*((disp)->MultiTexCoord3dARB)) parameters
#define GET_MultiTexCoord3dARB(disp) ((disp)->MultiTexCoord3dARB)
#define SET_MultiTexCoord3dARB(disp, fn) ((disp)->MultiTexCoord3dARB = fn)
#define CALL_MultiTexCoord3dvARB(disp, parameters) (*((disp)->MultiTexCoord3dvARB)) parameters
#define GET_MultiTexCoord3dvARB(disp) ((disp)->MultiTexCoord3dvARB)
#define SET_MultiTexCoord3dvARB(disp, fn) ((disp)->MultiTexCoord3dvARB = fn)
#define CALL_MultiTexCoord3fARB(disp, parameters) (*((disp)->MultiTexCoord3fARB)) parameters
#define GET_MultiTexCoord3fARB(disp) ((disp)->MultiTexCoord3fARB)
#define SET_MultiTexCoord3fARB(disp, fn) ((disp)->MultiTexCoord3fARB = fn)
#define CALL_MultiTexCoord3fvARB(disp, parameters) (*((disp)->MultiTexCoord3fvARB)) parameters
#define GET_MultiTexCoord3fvARB(disp) ((disp)->MultiTexCoord3fvARB)
#define SET_MultiTexCoord3fvARB(disp, fn) ((disp)->MultiTexCoord3fvARB = fn)
#define CALL_MultiTexCoord3iARB(disp, parameters) (*((disp)->MultiTexCoord3iARB)) parameters
#define GET_MultiTexCoord3iARB(disp) ((disp)->MultiTexCoord3iARB)
#define SET_MultiTexCoord3iARB(disp, fn) ((disp)->MultiTexCoord3iARB = fn)
#define CALL_MultiTexCoord3ivARB(disp, parameters) (*((disp)->MultiTexCoord3ivARB)) parameters
#define GET_MultiTexCoord3ivARB(disp) ((disp)->MultiTexCoord3ivARB)
#define SET_MultiTexCoord3ivARB(disp, fn) ((disp)->MultiTexCoord3ivARB = fn)
#define CALL_MultiTexCoord3sARB(disp, parameters) (*((disp)->MultiTexCoord3sARB)) parameters
#define GET_MultiTexCoord3sARB(disp) ((disp)->MultiTexCoord3sARB)
#define SET_MultiTexCoord3sARB(disp, fn) ((disp)->MultiTexCoord3sARB = fn)
#define CALL_MultiTexCoord3svARB(disp, parameters) (*((disp)->MultiTexCoord3svARB)) parameters
#define GET_MultiTexCoord3svARB(disp) ((disp)->MultiTexCoord3svARB)
#define SET_MultiTexCoord3svARB(disp, fn) ((disp)->MultiTexCoord3svARB = fn)
#define CALL_MultiTexCoord4dARB(disp, parameters) (*((disp)->MultiTexCoord4dARB)) parameters
#define GET_MultiTexCoord4dARB(disp) ((disp)->MultiTexCoord4dARB)
#define SET_MultiTexCoord4dARB(disp, fn) ((disp)->MultiTexCoord4dARB = fn)
#define CALL_MultiTexCoord4dvARB(disp, parameters) (*((disp)->MultiTexCoord4dvARB)) parameters
#define GET_MultiTexCoord4dvARB(disp) ((disp)->MultiTexCoord4dvARB)
#define SET_MultiTexCoord4dvARB(disp, fn) ((disp)->MultiTexCoord4dvARB = fn)
#define CALL_MultiTexCoord4fARB(disp, parameters) (*((disp)->MultiTexCoord4fARB)) parameters
#define GET_MultiTexCoord4fARB(disp) ((disp)->MultiTexCoord4fARB)
#define SET_MultiTexCoord4fARB(disp, fn) ((disp)->MultiTexCoord4fARB = fn)
#define CALL_MultiTexCoord4fvARB(disp, parameters) (*((disp)->MultiTexCoord4fvARB)) parameters
#define GET_MultiTexCoord4fvARB(disp) ((disp)->MultiTexCoord4fvARB)
#define SET_MultiTexCoord4fvARB(disp, fn) ((disp)->MultiTexCoord4fvARB = fn)
#define CALL_MultiTexCoord4iARB(disp, parameters) (*((disp)->MultiTexCoord4iARB)) parameters
#define GET_MultiTexCoord4iARB(disp) ((disp)->MultiTexCoord4iARB)
#define SET_MultiTexCoord4iARB(disp, fn) ((disp)->MultiTexCoord4iARB = fn)
#define CALL_MultiTexCoord4ivARB(disp, parameters) (*((disp)->MultiTexCoord4ivARB)) parameters
#define GET_MultiTexCoord4ivARB(disp) ((disp)->MultiTexCoord4ivARB)
#define SET_MultiTexCoord4ivARB(disp, fn) ((disp)->MultiTexCoord4ivARB = fn)
#define CALL_MultiTexCoord4sARB(disp, parameters) (*((disp)->MultiTexCoord4sARB)) parameters
#define GET_MultiTexCoord4sARB(disp) ((disp)->MultiTexCoord4sARB)
#define SET_MultiTexCoord4sARB(disp, fn) ((disp)->MultiTexCoord4sARB = fn)
#define CALL_MultiTexCoord4svARB(disp, parameters) (*((disp)->MultiTexCoord4svARB)) parameters
#define GET_MultiTexCoord4svARB(disp) ((disp)->MultiTexCoord4svARB)
#define SET_MultiTexCoord4svARB(disp, fn) ((disp)->MultiTexCoord4svARB = fn)

#if !defined(IN_DRI_DRIVER)

#define CALL_AttachShader(disp, parameters) (*((disp)->AttachShader)) parameters
#define GET_AttachShader(disp) ((disp)->AttachShader)
#define SET_AttachShader(disp, fn) ((disp)->AttachShader = fn)
#define CALL_CreateProgram(disp, parameters) (*((disp)->CreateProgram)) parameters
#define GET_CreateProgram(disp) ((disp)->CreateProgram)
#define SET_CreateProgram(disp, fn) ((disp)->CreateProgram = fn)
#define CALL_CreateShader(disp, parameters) (*((disp)->CreateShader)) parameters
#define GET_CreateShader(disp) ((disp)->CreateShader)
#define SET_CreateShader(disp, fn) ((disp)->CreateShader = fn)
#define CALL_DeleteProgram(disp, parameters) (*((disp)->DeleteProgram)) parameters
#define GET_DeleteProgram(disp) ((disp)->DeleteProgram)
#define SET_DeleteProgram(disp, fn) ((disp)->DeleteProgram = fn)
#define CALL_DeleteShader(disp, parameters) (*((disp)->DeleteShader)) parameters
#define GET_DeleteShader(disp) ((disp)->DeleteShader)
#define SET_DeleteShader(disp, fn) ((disp)->DeleteShader = fn)
#define CALL_DetachShader(disp, parameters) (*((disp)->DetachShader)) parameters
#define GET_DetachShader(disp) ((disp)->DetachShader)
#define SET_DetachShader(disp, fn) ((disp)->DetachShader = fn)
#define CALL_GetAttachedShaders(disp, parameters) (*((disp)->GetAttachedShaders)) parameters
#define GET_GetAttachedShaders(disp) ((disp)->GetAttachedShaders)
#define SET_GetAttachedShaders(disp, fn) ((disp)->GetAttachedShaders = fn)
#define CALL_GetProgramInfoLog(disp, parameters) (*((disp)->GetProgramInfoLog)) parameters
#define GET_GetProgramInfoLog(disp) ((disp)->GetProgramInfoLog)
#define SET_GetProgramInfoLog(disp, fn) ((disp)->GetProgramInfoLog = fn)
#define CALL_GetProgramiv(disp, parameters) (*((disp)->GetProgramiv)) parameters
#define GET_GetProgramiv(disp) ((disp)->GetProgramiv)
#define SET_GetProgramiv(disp, fn) ((disp)->GetProgramiv = fn)
#define CALL_GetShaderInfoLog(disp, parameters) (*((disp)->GetShaderInfoLog)) parameters
#define GET_GetShaderInfoLog(disp) ((disp)->GetShaderInfoLog)
#define SET_GetShaderInfoLog(disp, fn) ((disp)->GetShaderInfoLog = fn)
#define CALL_GetShaderiv(disp, parameters) (*((disp)->GetShaderiv)) parameters
#define GET_GetShaderiv(disp) ((disp)->GetShaderiv)
#define SET_GetShaderiv(disp, fn) ((disp)->GetShaderiv = fn)
#define CALL_IsProgram(disp, parameters) (*((disp)->IsProgram)) parameters
#define GET_IsProgram(disp) ((disp)->IsProgram)
#define SET_IsProgram(disp, fn) ((disp)->IsProgram = fn)
#define CALL_IsShader(disp, parameters) (*((disp)->IsShader)) parameters
#define GET_IsShader(disp) ((disp)->IsShader)
#define SET_IsShader(disp, fn) ((disp)->IsShader = fn)
#define CALL_StencilFuncSeparate(disp, parameters) (*((disp)->StencilFuncSeparate)) parameters
#define GET_StencilFuncSeparate(disp) ((disp)->StencilFuncSeparate)
#define SET_StencilFuncSeparate(disp, fn) ((disp)->StencilFuncSeparate = fn)
#define CALL_StencilMaskSeparate(disp, parameters) (*((disp)->StencilMaskSeparate)) parameters
#define GET_StencilMaskSeparate(disp) ((disp)->StencilMaskSeparate)
#define SET_StencilMaskSeparate(disp, fn) ((disp)->StencilMaskSeparate = fn)
#define CALL_StencilOpSeparate(disp, parameters) (*((disp)->StencilOpSeparate)) parameters
#define GET_StencilOpSeparate(disp) ((disp)->StencilOpSeparate)
#define SET_StencilOpSeparate(disp, fn) ((disp)->StencilOpSeparate = fn)
#define CALL_UniformMatrix2x3fv(disp, parameters) (*((disp)->UniformMatrix2x3fv)) parameters
#define GET_UniformMatrix2x3fv(disp) ((disp)->UniformMatrix2x3fv)
#define SET_UniformMatrix2x3fv(disp, fn) ((disp)->UniformMatrix2x3fv = fn)
#define CALL_UniformMatrix2x4fv(disp, parameters) (*((disp)->UniformMatrix2x4fv)) parameters
#define GET_UniformMatrix2x4fv(disp) ((disp)->UniformMatrix2x4fv)
#define SET_UniformMatrix2x4fv(disp, fn) ((disp)->UniformMatrix2x4fv = fn)
#define CALL_UniformMatrix3x2fv(disp, parameters) (*((disp)->UniformMatrix3x2fv)) parameters
#define GET_UniformMatrix3x2fv(disp) ((disp)->UniformMatrix3x2fv)
#define SET_UniformMatrix3x2fv(disp, fn) ((disp)->UniformMatrix3x2fv = fn)
#define CALL_UniformMatrix3x4fv(disp, parameters) (*((disp)->UniformMatrix3x4fv)) parameters
#define GET_UniformMatrix3x4fv(disp) ((disp)->UniformMatrix3x4fv)
#define SET_UniformMatrix3x4fv(disp, fn) ((disp)->UniformMatrix3x4fv = fn)
#define CALL_UniformMatrix4x2fv(disp, parameters) (*((disp)->UniformMatrix4x2fv)) parameters
#define GET_UniformMatrix4x2fv(disp) ((disp)->UniformMatrix4x2fv)
#define SET_UniformMatrix4x2fv(disp, fn) ((disp)->UniformMatrix4x2fv = fn)
#define CALL_UniformMatrix4x3fv(disp, parameters) (*((disp)->UniformMatrix4x3fv)) parameters
#define GET_UniformMatrix4x3fv(disp) ((disp)->UniformMatrix4x3fv)
#define SET_UniformMatrix4x3fv(disp, fn) ((disp)->UniformMatrix4x3fv = fn)
#define CALL_LoadTransposeMatrixdARB(disp, parameters) (*((disp)->LoadTransposeMatrixdARB)) parameters
#define GET_LoadTransposeMatrixdARB(disp) ((disp)->LoadTransposeMatrixdARB)
#define SET_LoadTransposeMatrixdARB(disp, fn) ((disp)->LoadTransposeMatrixdARB = fn)
#define CALL_LoadTransposeMatrixfARB(disp, parameters) (*((disp)->LoadTransposeMatrixfARB)) parameters
#define GET_LoadTransposeMatrixfARB(disp) ((disp)->LoadTransposeMatrixfARB)
#define SET_LoadTransposeMatrixfARB(disp, fn) ((disp)->LoadTransposeMatrixfARB = fn)
#define CALL_MultTransposeMatrixdARB(disp, parameters) (*((disp)->MultTransposeMatrixdARB)) parameters
#define GET_MultTransposeMatrixdARB(disp) ((disp)->MultTransposeMatrixdARB)
#define SET_MultTransposeMatrixdARB(disp, fn) ((disp)->MultTransposeMatrixdARB = fn)
#define CALL_MultTransposeMatrixfARB(disp, parameters) (*((disp)->MultTransposeMatrixfARB)) parameters
#define GET_MultTransposeMatrixfARB(disp) ((disp)->MultTransposeMatrixfARB)
#define SET_MultTransposeMatrixfARB(disp, fn) ((disp)->MultTransposeMatrixfARB = fn)
#define CALL_SampleCoverageARB(disp, parameters) (*((disp)->SampleCoverageARB)) parameters
#define GET_SampleCoverageARB(disp) ((disp)->SampleCoverageARB)
#define SET_SampleCoverageARB(disp, fn) ((disp)->SampleCoverageARB = fn)
#define CALL_CompressedTexImage1DARB(disp, parameters) (*((disp)->CompressedTexImage1DARB)) parameters
#define GET_CompressedTexImage1DARB(disp) ((disp)->CompressedTexImage1DARB)
#define SET_CompressedTexImage1DARB(disp, fn) ((disp)->CompressedTexImage1DARB = fn)
#define CALL_CompressedTexImage2DARB(disp, parameters) (*((disp)->CompressedTexImage2DARB)) parameters
#define GET_CompressedTexImage2DARB(disp) ((disp)->CompressedTexImage2DARB)
#define SET_CompressedTexImage2DARB(disp, fn) ((disp)->CompressedTexImage2DARB = fn)
#define CALL_CompressedTexImage3DARB(disp, parameters) (*((disp)->CompressedTexImage3DARB)) parameters
#define GET_CompressedTexImage3DARB(disp) ((disp)->CompressedTexImage3DARB)
#define SET_CompressedTexImage3DARB(disp, fn) ((disp)->CompressedTexImage3DARB = fn)
#define CALL_CompressedTexSubImage1DARB(disp, parameters) (*((disp)->CompressedTexSubImage1DARB)) parameters
#define GET_CompressedTexSubImage1DARB(disp) ((disp)->CompressedTexSubImage1DARB)
#define SET_CompressedTexSubImage1DARB(disp, fn) ((disp)->CompressedTexSubImage1DARB = fn)
#define CALL_CompressedTexSubImage2DARB(disp, parameters) (*((disp)->CompressedTexSubImage2DARB)) parameters
#define GET_CompressedTexSubImage2DARB(disp) ((disp)->CompressedTexSubImage2DARB)
#define SET_CompressedTexSubImage2DARB(disp, fn) ((disp)->CompressedTexSubImage2DARB = fn)
#define CALL_CompressedTexSubImage3DARB(disp, parameters) (*((disp)->CompressedTexSubImage3DARB)) parameters
#define GET_CompressedTexSubImage3DARB(disp) ((disp)->CompressedTexSubImage3DARB)
#define SET_CompressedTexSubImage3DARB(disp, fn) ((disp)->CompressedTexSubImage3DARB = fn)
#define CALL_GetCompressedTexImageARB(disp, parameters) (*((disp)->GetCompressedTexImageARB)) parameters
#define GET_GetCompressedTexImageARB(disp) ((disp)->GetCompressedTexImageARB)
#define SET_GetCompressedTexImageARB(disp, fn) ((disp)->GetCompressedTexImageARB = fn)
#define CALL_DisableVertexAttribArrayARB(disp, parameters) (*((disp)->DisableVertexAttribArrayARB)) parameters
#define GET_DisableVertexAttribArrayARB(disp) ((disp)->DisableVertexAttribArrayARB)
#define SET_DisableVertexAttribArrayARB(disp, fn) ((disp)->DisableVertexAttribArrayARB = fn)
#define CALL_EnableVertexAttribArrayARB(disp, parameters) (*((disp)->EnableVertexAttribArrayARB)) parameters
#define GET_EnableVertexAttribArrayARB(disp) ((disp)->EnableVertexAttribArrayARB)
#define SET_EnableVertexAttribArrayARB(disp, fn) ((disp)->EnableVertexAttribArrayARB = fn)
#define CALL_GetProgramEnvParameterdvARB(disp, parameters) (*((disp)->GetProgramEnvParameterdvARB)) parameters
#define GET_GetProgramEnvParameterdvARB(disp) ((disp)->GetProgramEnvParameterdvARB)
#define SET_GetProgramEnvParameterdvARB(disp, fn) ((disp)->GetProgramEnvParameterdvARB = fn)
#define CALL_GetProgramEnvParameterfvARB(disp, parameters) (*((disp)->GetProgramEnvParameterfvARB)) parameters
#define GET_GetProgramEnvParameterfvARB(disp) ((disp)->GetProgramEnvParameterfvARB)
#define SET_GetProgramEnvParameterfvARB(disp, fn) ((disp)->GetProgramEnvParameterfvARB = fn)
#define CALL_GetProgramLocalParameterdvARB(disp, parameters) (*((disp)->GetProgramLocalParameterdvARB)) parameters
#define GET_GetProgramLocalParameterdvARB(disp) ((disp)->GetProgramLocalParameterdvARB)
#define SET_GetProgramLocalParameterdvARB(disp, fn) ((disp)->GetProgramLocalParameterdvARB = fn)
#define CALL_GetProgramLocalParameterfvARB(disp, parameters) (*((disp)->GetProgramLocalParameterfvARB)) parameters
#define GET_GetProgramLocalParameterfvARB(disp) ((disp)->GetProgramLocalParameterfvARB)
#define SET_GetProgramLocalParameterfvARB(disp, fn) ((disp)->GetProgramLocalParameterfvARB = fn)
#define CALL_GetProgramStringARB(disp, parameters) (*((disp)->GetProgramStringARB)) parameters
#define GET_GetProgramStringARB(disp) ((disp)->GetProgramStringARB)
#define SET_GetProgramStringARB(disp, fn) ((disp)->GetProgramStringARB = fn)
#define CALL_GetProgramivARB(disp, parameters) (*((disp)->GetProgramivARB)) parameters
#define GET_GetProgramivARB(disp) ((disp)->GetProgramivARB)
#define SET_GetProgramivARB(disp, fn) ((disp)->GetProgramivARB = fn)
#define CALL_GetVertexAttribdvARB(disp, parameters) (*((disp)->GetVertexAttribdvARB)) parameters
#define GET_GetVertexAttribdvARB(disp) ((disp)->GetVertexAttribdvARB)
#define SET_GetVertexAttribdvARB(disp, fn) ((disp)->GetVertexAttribdvARB = fn)
#define CALL_GetVertexAttribfvARB(disp, parameters) (*((disp)->GetVertexAttribfvARB)) parameters
#define GET_GetVertexAttribfvARB(disp) ((disp)->GetVertexAttribfvARB)
#define SET_GetVertexAttribfvARB(disp, fn) ((disp)->GetVertexAttribfvARB = fn)
#define CALL_GetVertexAttribivARB(disp, parameters) (*((disp)->GetVertexAttribivARB)) parameters
#define GET_GetVertexAttribivARB(disp) ((disp)->GetVertexAttribivARB)
#define SET_GetVertexAttribivARB(disp, fn) ((disp)->GetVertexAttribivARB = fn)
#define CALL_ProgramEnvParameter4dARB(disp, parameters) (*((disp)->ProgramEnvParameter4dARB)) parameters
#define GET_ProgramEnvParameter4dARB(disp) ((disp)->ProgramEnvParameter4dARB)
#define SET_ProgramEnvParameter4dARB(disp, fn) ((disp)->ProgramEnvParameter4dARB = fn)
#define CALL_ProgramEnvParameter4dvARB(disp, parameters) (*((disp)->ProgramEnvParameter4dvARB)) parameters
#define GET_ProgramEnvParameter4dvARB(disp) ((disp)->ProgramEnvParameter4dvARB)
#define SET_ProgramEnvParameter4dvARB(disp, fn) ((disp)->ProgramEnvParameter4dvARB = fn)
#define CALL_ProgramEnvParameter4fARB(disp, parameters) (*((disp)->ProgramEnvParameter4fARB)) parameters
#define GET_ProgramEnvParameter4fARB(disp) ((disp)->ProgramEnvParameter4fARB)
#define SET_ProgramEnvParameter4fARB(disp, fn) ((disp)->ProgramEnvParameter4fARB = fn)
#define CALL_ProgramEnvParameter4fvARB(disp, parameters) (*((disp)->ProgramEnvParameter4fvARB)) parameters
#define GET_ProgramEnvParameter4fvARB(disp) ((disp)->ProgramEnvParameter4fvARB)
#define SET_ProgramEnvParameter4fvARB(disp, fn) ((disp)->ProgramEnvParameter4fvARB = fn)
#define CALL_ProgramLocalParameter4dARB(disp, parameters) (*((disp)->ProgramLocalParameter4dARB)) parameters
#define GET_ProgramLocalParameter4dARB(disp) ((disp)->ProgramLocalParameter4dARB)
#define SET_ProgramLocalParameter4dARB(disp, fn) ((disp)->ProgramLocalParameter4dARB = fn)
#define CALL_ProgramLocalParameter4dvARB(disp, parameters) (*((disp)->ProgramLocalParameter4dvARB)) parameters
#define GET_ProgramLocalParameter4dvARB(disp) ((disp)->ProgramLocalParameter4dvARB)
#define SET_ProgramLocalParameter4dvARB(disp, fn) ((disp)->ProgramLocalParameter4dvARB = fn)
#define CALL_ProgramLocalParameter4fARB(disp, parameters) (*((disp)->ProgramLocalParameter4fARB)) parameters
#define GET_ProgramLocalParameter4fARB(disp) ((disp)->ProgramLocalParameter4fARB)
#define SET_ProgramLocalParameter4fARB(disp, fn) ((disp)->ProgramLocalParameter4fARB = fn)
#define CALL_ProgramLocalParameter4fvARB(disp, parameters) (*((disp)->ProgramLocalParameter4fvARB)) parameters
#define GET_ProgramLocalParameter4fvARB(disp) ((disp)->ProgramLocalParameter4fvARB)
#define SET_ProgramLocalParameter4fvARB(disp, fn) ((disp)->ProgramLocalParameter4fvARB = fn)
#define CALL_ProgramStringARB(disp, parameters) (*((disp)->ProgramStringARB)) parameters
#define GET_ProgramStringARB(disp) ((disp)->ProgramStringARB)
#define SET_ProgramStringARB(disp, fn) ((disp)->ProgramStringARB = fn)
#define CALL_VertexAttrib1dARB(disp, parameters) (*((disp)->VertexAttrib1dARB)) parameters
#define GET_VertexAttrib1dARB(disp) ((disp)->VertexAttrib1dARB)
#define SET_VertexAttrib1dARB(disp, fn) ((disp)->VertexAttrib1dARB = fn)
#define CALL_VertexAttrib1dvARB(disp, parameters) (*((disp)->VertexAttrib1dvARB)) parameters
#define GET_VertexAttrib1dvARB(disp) ((disp)->VertexAttrib1dvARB)
#define SET_VertexAttrib1dvARB(disp, fn) ((disp)->VertexAttrib1dvARB = fn)
#define CALL_VertexAttrib1fARB(disp, parameters) (*((disp)->VertexAttrib1fARB)) parameters
#define GET_VertexAttrib1fARB(disp) ((disp)->VertexAttrib1fARB)
#define SET_VertexAttrib1fARB(disp, fn) ((disp)->VertexAttrib1fARB = fn)
#define CALL_VertexAttrib1fvARB(disp, parameters) (*((disp)->VertexAttrib1fvARB)) parameters
#define GET_VertexAttrib1fvARB(disp) ((disp)->VertexAttrib1fvARB)
#define SET_VertexAttrib1fvARB(disp, fn) ((disp)->VertexAttrib1fvARB = fn)
#define CALL_VertexAttrib1sARB(disp, parameters) (*((disp)->VertexAttrib1sARB)) parameters
#define GET_VertexAttrib1sARB(disp) ((disp)->VertexAttrib1sARB)
#define SET_VertexAttrib1sARB(disp, fn) ((disp)->VertexAttrib1sARB = fn)
#define CALL_VertexAttrib1svARB(disp, parameters) (*((disp)->VertexAttrib1svARB)) parameters
#define GET_VertexAttrib1svARB(disp) ((disp)->VertexAttrib1svARB)
#define SET_VertexAttrib1svARB(disp, fn) ((disp)->VertexAttrib1svARB = fn)
#define CALL_VertexAttrib2dARB(disp, parameters) (*((disp)->VertexAttrib2dARB)) parameters
#define GET_VertexAttrib2dARB(disp) ((disp)->VertexAttrib2dARB)
#define SET_VertexAttrib2dARB(disp, fn) ((disp)->VertexAttrib2dARB = fn)
#define CALL_VertexAttrib2dvARB(disp, parameters) (*((disp)->VertexAttrib2dvARB)) parameters
#define GET_VertexAttrib2dvARB(disp) ((disp)->VertexAttrib2dvARB)
#define SET_VertexAttrib2dvARB(disp, fn) ((disp)->VertexAttrib2dvARB = fn)
#define CALL_VertexAttrib2fARB(disp, parameters) (*((disp)->VertexAttrib2fARB)) parameters
#define GET_VertexAttrib2fARB(disp) ((disp)->VertexAttrib2fARB)
#define SET_VertexAttrib2fARB(disp, fn) ((disp)->VertexAttrib2fARB = fn)
#define CALL_VertexAttrib2fvARB(disp, parameters) (*((disp)->VertexAttrib2fvARB)) parameters
#define GET_VertexAttrib2fvARB(disp) ((disp)->VertexAttrib2fvARB)
#define SET_VertexAttrib2fvARB(disp, fn) ((disp)->VertexAttrib2fvARB = fn)
#define CALL_VertexAttrib2sARB(disp, parameters) (*((disp)->VertexAttrib2sARB)) parameters
#define GET_VertexAttrib2sARB(disp) ((disp)->VertexAttrib2sARB)
#define SET_VertexAttrib2sARB(disp, fn) ((disp)->VertexAttrib2sARB = fn)
#define CALL_VertexAttrib2svARB(disp, parameters) (*((disp)->VertexAttrib2svARB)) parameters
#define GET_VertexAttrib2svARB(disp) ((disp)->VertexAttrib2svARB)
#define SET_VertexAttrib2svARB(disp, fn) ((disp)->VertexAttrib2svARB = fn)
#define CALL_VertexAttrib3dARB(disp, parameters) (*((disp)->VertexAttrib3dARB)) parameters
#define GET_VertexAttrib3dARB(disp) ((disp)->VertexAttrib3dARB)
#define SET_VertexAttrib3dARB(disp, fn) ((disp)->VertexAttrib3dARB = fn)
#define CALL_VertexAttrib3dvARB(disp, parameters) (*((disp)->VertexAttrib3dvARB)) parameters
#define GET_VertexAttrib3dvARB(disp) ((disp)->VertexAttrib3dvARB)
#define SET_VertexAttrib3dvARB(disp, fn) ((disp)->VertexAttrib3dvARB = fn)
#define CALL_VertexAttrib3fARB(disp, parameters) (*((disp)->VertexAttrib3fARB)) parameters
#define GET_VertexAttrib3fARB(disp) ((disp)->VertexAttrib3fARB)
#define SET_VertexAttrib3fARB(disp, fn) ((disp)->VertexAttrib3fARB = fn)
#define CALL_VertexAttrib3fvARB(disp, parameters) (*((disp)->VertexAttrib3fvARB)) parameters
#define GET_VertexAttrib3fvARB(disp) ((disp)->VertexAttrib3fvARB)
#define SET_VertexAttrib3fvARB(disp, fn) ((disp)->VertexAttrib3fvARB = fn)
#define CALL_VertexAttrib3sARB(disp, parameters) (*((disp)->VertexAttrib3sARB)) parameters
#define GET_VertexAttrib3sARB(disp) ((disp)->VertexAttrib3sARB)
#define SET_VertexAttrib3sARB(disp, fn) ((disp)->VertexAttrib3sARB = fn)
#define CALL_VertexAttrib3svARB(disp, parameters) (*((disp)->VertexAttrib3svARB)) parameters
#define GET_VertexAttrib3svARB(disp) ((disp)->VertexAttrib3svARB)
#define SET_VertexAttrib3svARB(disp, fn) ((disp)->VertexAttrib3svARB = fn)
#define CALL_VertexAttrib4NbvARB(disp, parameters) (*((disp)->VertexAttrib4NbvARB)) parameters
#define GET_VertexAttrib4NbvARB(disp) ((disp)->VertexAttrib4NbvARB)
#define SET_VertexAttrib4NbvARB(disp, fn) ((disp)->VertexAttrib4NbvARB = fn)
#define CALL_VertexAttrib4NivARB(disp, parameters) (*((disp)->VertexAttrib4NivARB)) parameters
#define GET_VertexAttrib4NivARB(disp) ((disp)->VertexAttrib4NivARB)
#define SET_VertexAttrib4NivARB(disp, fn) ((disp)->VertexAttrib4NivARB = fn)
#define CALL_VertexAttrib4NsvARB(disp, parameters) (*((disp)->VertexAttrib4NsvARB)) parameters
#define GET_VertexAttrib4NsvARB(disp) ((disp)->VertexAttrib4NsvARB)
#define SET_VertexAttrib4NsvARB(disp, fn) ((disp)->VertexAttrib4NsvARB = fn)
#define CALL_VertexAttrib4NubARB(disp, parameters) (*((disp)->VertexAttrib4NubARB)) parameters
#define GET_VertexAttrib4NubARB(disp) ((disp)->VertexAttrib4NubARB)
#define SET_VertexAttrib4NubARB(disp, fn) ((disp)->VertexAttrib4NubARB = fn)
#define CALL_VertexAttrib4NubvARB(disp, parameters) (*((disp)->VertexAttrib4NubvARB)) parameters
#define GET_VertexAttrib4NubvARB(disp) ((disp)->VertexAttrib4NubvARB)
#define SET_VertexAttrib4NubvARB(disp, fn) ((disp)->VertexAttrib4NubvARB = fn)
#define CALL_VertexAttrib4NuivARB(disp, parameters) (*((disp)->VertexAttrib4NuivARB)) parameters
#define GET_VertexAttrib4NuivARB(disp) ((disp)->VertexAttrib4NuivARB)
#define SET_VertexAttrib4NuivARB(disp, fn) ((disp)->VertexAttrib4NuivARB = fn)
#define CALL_VertexAttrib4NusvARB(disp, parameters) (*((disp)->VertexAttrib4NusvARB)) parameters
#define GET_VertexAttrib4NusvARB(disp) ((disp)->VertexAttrib4NusvARB)
#define SET_VertexAttrib4NusvARB(disp, fn) ((disp)->VertexAttrib4NusvARB = fn)
#define CALL_VertexAttrib4bvARB(disp, parameters) (*((disp)->VertexAttrib4bvARB)) parameters
#define GET_VertexAttrib4bvARB(disp) ((disp)->VertexAttrib4bvARB)
#define SET_VertexAttrib4bvARB(disp, fn) ((disp)->VertexAttrib4bvARB = fn)
#define CALL_VertexAttrib4dARB(disp, parameters) (*((disp)->VertexAttrib4dARB)) parameters
#define GET_VertexAttrib4dARB(disp) ((disp)->VertexAttrib4dARB)
#define SET_VertexAttrib4dARB(disp, fn) ((disp)->VertexAttrib4dARB = fn)
#define CALL_VertexAttrib4dvARB(disp, parameters) (*((disp)->VertexAttrib4dvARB)) parameters
#define GET_VertexAttrib4dvARB(disp) ((disp)->VertexAttrib4dvARB)
#define SET_VertexAttrib4dvARB(disp, fn) ((disp)->VertexAttrib4dvARB = fn)
#define CALL_VertexAttrib4fARB(disp, parameters) (*((disp)->VertexAttrib4fARB)) parameters
#define GET_VertexAttrib4fARB(disp) ((disp)->VertexAttrib4fARB)
#define SET_VertexAttrib4fARB(disp, fn) ((disp)->VertexAttrib4fARB = fn)
#define CALL_VertexAttrib4fvARB(disp, parameters) (*((disp)->VertexAttrib4fvARB)) parameters
#define GET_VertexAttrib4fvARB(disp) ((disp)->VertexAttrib4fvARB)
#define SET_VertexAttrib4fvARB(disp, fn) ((disp)->VertexAttrib4fvARB = fn)
#define CALL_VertexAttrib4ivARB(disp, parameters) (*((disp)->VertexAttrib4ivARB)) parameters
#define GET_VertexAttrib4ivARB(disp) ((disp)->VertexAttrib4ivARB)
#define SET_VertexAttrib4ivARB(disp, fn) ((disp)->VertexAttrib4ivARB = fn)
#define CALL_VertexAttrib4sARB(disp, parameters) (*((disp)->VertexAttrib4sARB)) parameters
#define GET_VertexAttrib4sARB(disp) ((disp)->VertexAttrib4sARB)
#define SET_VertexAttrib4sARB(disp, fn) ((disp)->VertexAttrib4sARB = fn)
#define CALL_VertexAttrib4svARB(disp, parameters) (*((disp)->VertexAttrib4svARB)) parameters
#define GET_VertexAttrib4svARB(disp) ((disp)->VertexAttrib4svARB)
#define SET_VertexAttrib4svARB(disp, fn) ((disp)->VertexAttrib4svARB = fn)
#define CALL_VertexAttrib4ubvARB(disp, parameters) (*((disp)->VertexAttrib4ubvARB)) parameters
#define GET_VertexAttrib4ubvARB(disp) ((disp)->VertexAttrib4ubvARB)
#define SET_VertexAttrib4ubvARB(disp, fn) ((disp)->VertexAttrib4ubvARB = fn)
#define CALL_VertexAttrib4uivARB(disp, parameters) (*((disp)->VertexAttrib4uivARB)) parameters
#define GET_VertexAttrib4uivARB(disp) ((disp)->VertexAttrib4uivARB)
#define SET_VertexAttrib4uivARB(disp, fn) ((disp)->VertexAttrib4uivARB = fn)
#define CALL_VertexAttrib4usvARB(disp, parameters) (*((disp)->VertexAttrib4usvARB)) parameters
#define GET_VertexAttrib4usvARB(disp) ((disp)->VertexAttrib4usvARB)
#define SET_VertexAttrib4usvARB(disp, fn) ((disp)->VertexAttrib4usvARB = fn)
#define CALL_VertexAttribPointerARB(disp, parameters) (*((disp)->VertexAttribPointerARB)) parameters
#define GET_VertexAttribPointerARB(disp) ((disp)->VertexAttribPointerARB)
#define SET_VertexAttribPointerARB(disp, fn) ((disp)->VertexAttribPointerARB = fn)
#define CALL_BindBufferARB(disp, parameters) (*((disp)->BindBufferARB)) parameters
#define GET_BindBufferARB(disp) ((disp)->BindBufferARB)
#define SET_BindBufferARB(disp, fn) ((disp)->BindBufferARB = fn)
#define CALL_BufferDataARB(disp, parameters) (*((disp)->BufferDataARB)) parameters
#define GET_BufferDataARB(disp) ((disp)->BufferDataARB)
#define SET_BufferDataARB(disp, fn) ((disp)->BufferDataARB = fn)
#define CALL_BufferSubDataARB(disp, parameters) (*((disp)->BufferSubDataARB)) parameters
#define GET_BufferSubDataARB(disp) ((disp)->BufferSubDataARB)
#define SET_BufferSubDataARB(disp, fn) ((disp)->BufferSubDataARB = fn)
#define CALL_DeleteBuffersARB(disp, parameters) (*((disp)->DeleteBuffersARB)) parameters
#define GET_DeleteBuffersARB(disp) ((disp)->DeleteBuffersARB)
#define SET_DeleteBuffersARB(disp, fn) ((disp)->DeleteBuffersARB = fn)
#define CALL_GenBuffersARB(disp, parameters) (*((disp)->GenBuffersARB)) parameters
#define GET_GenBuffersARB(disp) ((disp)->GenBuffersARB)
#define SET_GenBuffersARB(disp, fn) ((disp)->GenBuffersARB = fn)
#define CALL_GetBufferParameterivARB(disp, parameters) (*((disp)->GetBufferParameterivARB)) parameters
#define GET_GetBufferParameterivARB(disp) ((disp)->GetBufferParameterivARB)
#define SET_GetBufferParameterivARB(disp, fn) ((disp)->GetBufferParameterivARB = fn)
#define CALL_GetBufferPointervARB(disp, parameters) (*((disp)->GetBufferPointervARB)) parameters
#define GET_GetBufferPointervARB(disp) ((disp)->GetBufferPointervARB)
#define SET_GetBufferPointervARB(disp, fn) ((disp)->GetBufferPointervARB = fn)
#define CALL_GetBufferSubDataARB(disp, parameters) (*((disp)->GetBufferSubDataARB)) parameters
#define GET_GetBufferSubDataARB(disp) ((disp)->GetBufferSubDataARB)
#define SET_GetBufferSubDataARB(disp, fn) ((disp)->GetBufferSubDataARB = fn)
#define CALL_IsBufferARB(disp, parameters) (*((disp)->IsBufferARB)) parameters
#define GET_IsBufferARB(disp) ((disp)->IsBufferARB)
#define SET_IsBufferARB(disp, fn) ((disp)->IsBufferARB = fn)
#define CALL_MapBufferARB(disp, parameters) (*((disp)->MapBufferARB)) parameters
#define GET_MapBufferARB(disp) ((disp)->MapBufferARB)
#define SET_MapBufferARB(disp, fn) ((disp)->MapBufferARB = fn)
#define CALL_UnmapBufferARB(disp, parameters) (*((disp)->UnmapBufferARB)) parameters
#define GET_UnmapBufferARB(disp) ((disp)->UnmapBufferARB)
#define SET_UnmapBufferARB(disp, fn) ((disp)->UnmapBufferARB = fn)
#define CALL_BeginQueryARB(disp, parameters) (*((disp)->BeginQueryARB)) parameters
#define GET_BeginQueryARB(disp) ((disp)->BeginQueryARB)
#define SET_BeginQueryARB(disp, fn) ((disp)->BeginQueryARB = fn)
#define CALL_DeleteQueriesARB(disp, parameters) (*((disp)->DeleteQueriesARB)) parameters
#define GET_DeleteQueriesARB(disp) ((disp)->DeleteQueriesARB)
#define SET_DeleteQueriesARB(disp, fn) ((disp)->DeleteQueriesARB = fn)
#define CALL_EndQueryARB(disp, parameters) (*((disp)->EndQueryARB)) parameters
#define GET_EndQueryARB(disp) ((disp)->EndQueryARB)
#define SET_EndQueryARB(disp, fn) ((disp)->EndQueryARB = fn)
#define CALL_GenQueriesARB(disp, parameters) (*((disp)->GenQueriesARB)) parameters
#define GET_GenQueriesARB(disp) ((disp)->GenQueriesARB)
#define SET_GenQueriesARB(disp, fn) ((disp)->GenQueriesARB = fn)
#define CALL_GetQueryObjectivARB(disp, parameters) (*((disp)->GetQueryObjectivARB)) parameters
#define GET_GetQueryObjectivARB(disp) ((disp)->GetQueryObjectivARB)
#define SET_GetQueryObjectivARB(disp, fn) ((disp)->GetQueryObjectivARB = fn)
#define CALL_GetQueryObjectuivARB(disp, parameters) (*((disp)->GetQueryObjectuivARB)) parameters
#define GET_GetQueryObjectuivARB(disp) ((disp)->GetQueryObjectuivARB)
#define SET_GetQueryObjectuivARB(disp, fn) ((disp)->GetQueryObjectuivARB = fn)
#define CALL_GetQueryivARB(disp, parameters) (*((disp)->GetQueryivARB)) parameters
#define GET_GetQueryivARB(disp) ((disp)->GetQueryivARB)
#define SET_GetQueryivARB(disp, fn) ((disp)->GetQueryivARB = fn)
#define CALL_IsQueryARB(disp, parameters) (*((disp)->IsQueryARB)) parameters
#define GET_IsQueryARB(disp) ((disp)->IsQueryARB)
#define SET_IsQueryARB(disp, fn) ((disp)->IsQueryARB = fn)
#define CALL_AttachObjectARB(disp, parameters) (*((disp)->AttachObjectARB)) parameters
#define GET_AttachObjectARB(disp) ((disp)->AttachObjectARB)
#define SET_AttachObjectARB(disp, fn) ((disp)->AttachObjectARB = fn)
#define CALL_CompileShaderARB(disp, parameters) (*((disp)->CompileShaderARB)) parameters
#define GET_CompileShaderARB(disp) ((disp)->CompileShaderARB)
#define SET_CompileShaderARB(disp, fn) ((disp)->CompileShaderARB = fn)
#define CALL_CreateProgramObjectARB(disp, parameters) (*((disp)->CreateProgramObjectARB)) parameters
#define GET_CreateProgramObjectARB(disp) ((disp)->CreateProgramObjectARB)
#define SET_CreateProgramObjectARB(disp, fn) ((disp)->CreateProgramObjectARB = fn)
#define CALL_CreateShaderObjectARB(disp, parameters) (*((disp)->CreateShaderObjectARB)) parameters
#define GET_CreateShaderObjectARB(disp) ((disp)->CreateShaderObjectARB)
#define SET_CreateShaderObjectARB(disp, fn) ((disp)->CreateShaderObjectARB = fn)
#define CALL_DeleteObjectARB(disp, parameters) (*((disp)->DeleteObjectARB)) parameters
#define GET_DeleteObjectARB(disp) ((disp)->DeleteObjectARB)
#define SET_DeleteObjectARB(disp, fn) ((disp)->DeleteObjectARB = fn)
#define CALL_DetachObjectARB(disp, parameters) (*((disp)->DetachObjectARB)) parameters
#define GET_DetachObjectARB(disp) ((disp)->DetachObjectARB)
#define SET_DetachObjectARB(disp, fn) ((disp)->DetachObjectARB = fn)
#define CALL_GetActiveUniformARB(disp, parameters) (*((disp)->GetActiveUniformARB)) parameters
#define GET_GetActiveUniformARB(disp) ((disp)->GetActiveUniformARB)
#define SET_GetActiveUniformARB(disp, fn) ((disp)->GetActiveUniformARB = fn)
#define CALL_GetAttachedObjectsARB(disp, parameters) (*((disp)->GetAttachedObjectsARB)) parameters
#define GET_GetAttachedObjectsARB(disp) ((disp)->GetAttachedObjectsARB)
#define SET_GetAttachedObjectsARB(disp, fn) ((disp)->GetAttachedObjectsARB = fn)
#define CALL_GetHandleARB(disp, parameters) (*((disp)->GetHandleARB)) parameters
#define GET_GetHandleARB(disp) ((disp)->GetHandleARB)
#define SET_GetHandleARB(disp, fn) ((disp)->GetHandleARB = fn)
#define CALL_GetInfoLogARB(disp, parameters) (*((disp)->GetInfoLogARB)) parameters
#define GET_GetInfoLogARB(disp) ((disp)->GetInfoLogARB)
#define SET_GetInfoLogARB(disp, fn) ((disp)->GetInfoLogARB = fn)
#define CALL_GetObjectParameterfvARB(disp, parameters) (*((disp)->GetObjectParameterfvARB)) parameters
#define GET_GetObjectParameterfvARB(disp) ((disp)->GetObjectParameterfvARB)
#define SET_GetObjectParameterfvARB(disp, fn) ((disp)->GetObjectParameterfvARB = fn)
#define CALL_GetObjectParameterivARB(disp, parameters) (*((disp)->GetObjectParameterivARB)) parameters
#define GET_GetObjectParameterivARB(disp) ((disp)->GetObjectParameterivARB)
#define SET_GetObjectParameterivARB(disp, fn) ((disp)->GetObjectParameterivARB = fn)
#define CALL_GetShaderSourceARB(disp, parameters) (*((disp)->GetShaderSourceARB)) parameters
#define GET_GetShaderSourceARB(disp) ((disp)->GetShaderSourceARB)
#define SET_GetShaderSourceARB(disp, fn) ((disp)->GetShaderSourceARB = fn)
#define CALL_GetUniformLocationARB(disp, parameters) (*((disp)->GetUniformLocationARB)) parameters
#define GET_GetUniformLocationARB(disp) ((disp)->GetUniformLocationARB)
#define SET_GetUniformLocationARB(disp, fn) ((disp)->GetUniformLocationARB = fn)
#define CALL_GetUniformfvARB(disp, parameters) (*((disp)->GetUniformfvARB)) parameters
#define GET_GetUniformfvARB(disp) ((disp)->GetUniformfvARB)
#define SET_GetUniformfvARB(disp, fn) ((disp)->GetUniformfvARB = fn)
#define CALL_GetUniformivARB(disp, parameters) (*((disp)->GetUniformivARB)) parameters
#define GET_GetUniformivARB(disp) ((disp)->GetUniformivARB)
#define SET_GetUniformivARB(disp, fn) ((disp)->GetUniformivARB = fn)
#define CALL_LinkProgramARB(disp, parameters) (*((disp)->LinkProgramARB)) parameters
#define GET_LinkProgramARB(disp) ((disp)->LinkProgramARB)
#define SET_LinkProgramARB(disp, fn) ((disp)->LinkProgramARB = fn)
#define CALL_ShaderSourceARB(disp, parameters) (*((disp)->ShaderSourceARB)) parameters
#define GET_ShaderSourceARB(disp) ((disp)->ShaderSourceARB)
#define SET_ShaderSourceARB(disp, fn) ((disp)->ShaderSourceARB = fn)
#define CALL_Uniform1fARB(disp, parameters) (*((disp)->Uniform1fARB)) parameters
#define GET_Uniform1fARB(disp) ((disp)->Uniform1fARB)
#define SET_Uniform1fARB(disp, fn) ((disp)->Uniform1fARB = fn)
#define CALL_Uniform1fvARB(disp, parameters) (*((disp)->Uniform1fvARB)) parameters
#define GET_Uniform1fvARB(disp) ((disp)->Uniform1fvARB)
#define SET_Uniform1fvARB(disp, fn) ((disp)->Uniform1fvARB = fn)
#define CALL_Uniform1iARB(disp, parameters) (*((disp)->Uniform1iARB)) parameters
#define GET_Uniform1iARB(disp) ((disp)->Uniform1iARB)
#define SET_Uniform1iARB(disp, fn) ((disp)->Uniform1iARB = fn)
#define CALL_Uniform1ivARB(disp, parameters) (*((disp)->Uniform1ivARB)) parameters
#define GET_Uniform1ivARB(disp) ((disp)->Uniform1ivARB)
#define SET_Uniform1ivARB(disp, fn) ((disp)->Uniform1ivARB = fn)
#define CALL_Uniform2fARB(disp, parameters) (*((disp)->Uniform2fARB)) parameters
#define GET_Uniform2fARB(disp) ((disp)->Uniform2fARB)
#define SET_Uniform2fARB(disp, fn) ((disp)->Uniform2fARB = fn)
#define CALL_Uniform2fvARB(disp, parameters) (*((disp)->Uniform2fvARB)) parameters
#define GET_Uniform2fvARB(disp) ((disp)->Uniform2fvARB)
#define SET_Uniform2fvARB(disp, fn) ((disp)->Uniform2fvARB = fn)
#define CALL_Uniform2iARB(disp, parameters) (*((disp)->Uniform2iARB)) parameters
#define GET_Uniform2iARB(disp) ((disp)->Uniform2iARB)
#define SET_Uniform2iARB(disp, fn) ((disp)->Uniform2iARB = fn)
#define CALL_Uniform2ivARB(disp, parameters) (*((disp)->Uniform2ivARB)) parameters
#define GET_Uniform2ivARB(disp) ((disp)->Uniform2ivARB)
#define SET_Uniform2ivARB(disp, fn) ((disp)->Uniform2ivARB = fn)
#define CALL_Uniform3fARB(disp, parameters) (*((disp)->Uniform3fARB)) parameters
#define GET_Uniform3fARB(disp) ((disp)->Uniform3fARB)
#define SET_Uniform3fARB(disp, fn) ((disp)->Uniform3fARB = fn)
#define CALL_Uniform3fvARB(disp, parameters) (*((disp)->Uniform3fvARB)) parameters
#define GET_Uniform3fvARB(disp) ((disp)->Uniform3fvARB)
#define SET_Uniform3fvARB(disp, fn) ((disp)->Uniform3fvARB = fn)
#define CALL_Uniform3iARB(disp, parameters) (*((disp)->Uniform3iARB)) parameters
#define GET_Uniform3iARB(disp) ((disp)->Uniform3iARB)
#define SET_Uniform3iARB(disp, fn) ((disp)->Uniform3iARB = fn)
#define CALL_Uniform3ivARB(disp, parameters) (*((disp)->Uniform3ivARB)) parameters
#define GET_Uniform3ivARB(disp) ((disp)->Uniform3ivARB)
#define SET_Uniform3ivARB(disp, fn) ((disp)->Uniform3ivARB = fn)
#define CALL_Uniform4fARB(disp, parameters) (*((disp)->Uniform4fARB)) parameters
#define GET_Uniform4fARB(disp) ((disp)->Uniform4fARB)
#define SET_Uniform4fARB(disp, fn) ((disp)->Uniform4fARB = fn)
#define CALL_Uniform4fvARB(disp, parameters) (*((disp)->Uniform4fvARB)) parameters
#define GET_Uniform4fvARB(disp) ((disp)->Uniform4fvARB)
#define SET_Uniform4fvARB(disp, fn) ((disp)->Uniform4fvARB = fn)
#define CALL_Uniform4iARB(disp, parameters) (*((disp)->Uniform4iARB)) parameters
#define GET_Uniform4iARB(disp) ((disp)->Uniform4iARB)
#define SET_Uniform4iARB(disp, fn) ((disp)->Uniform4iARB = fn)
#define CALL_Uniform4ivARB(disp, parameters) (*((disp)->Uniform4ivARB)) parameters
#define GET_Uniform4ivARB(disp) ((disp)->Uniform4ivARB)
#define SET_Uniform4ivARB(disp, fn) ((disp)->Uniform4ivARB = fn)
#define CALL_UniformMatrix2fvARB(disp, parameters) (*((disp)->UniformMatrix2fvARB)) parameters
#define GET_UniformMatrix2fvARB(disp) ((disp)->UniformMatrix2fvARB)
#define SET_UniformMatrix2fvARB(disp, fn) ((disp)->UniformMatrix2fvARB = fn)
#define CALL_UniformMatrix3fvARB(disp, parameters) (*((disp)->UniformMatrix3fvARB)) parameters
#define GET_UniformMatrix3fvARB(disp) ((disp)->UniformMatrix3fvARB)
#define SET_UniformMatrix3fvARB(disp, fn) ((disp)->UniformMatrix3fvARB = fn)
#define CALL_UniformMatrix4fvARB(disp, parameters) (*((disp)->UniformMatrix4fvARB)) parameters
#define GET_UniformMatrix4fvARB(disp) ((disp)->UniformMatrix4fvARB)
#define SET_UniformMatrix4fvARB(disp, fn) ((disp)->UniformMatrix4fvARB = fn)
#define CALL_UseProgramObjectARB(disp, parameters) (*((disp)->UseProgramObjectARB)) parameters
#define GET_UseProgramObjectARB(disp) ((disp)->UseProgramObjectARB)
#define SET_UseProgramObjectARB(disp, fn) ((disp)->UseProgramObjectARB = fn)
#define CALL_ValidateProgramARB(disp, parameters) (*((disp)->ValidateProgramARB)) parameters
#define GET_ValidateProgramARB(disp) ((disp)->ValidateProgramARB)
#define SET_ValidateProgramARB(disp, fn) ((disp)->ValidateProgramARB = fn)
#define CALL_BindAttribLocationARB(disp, parameters) (*((disp)->BindAttribLocationARB)) parameters
#define GET_BindAttribLocationARB(disp) ((disp)->BindAttribLocationARB)
#define SET_BindAttribLocationARB(disp, fn) ((disp)->BindAttribLocationARB = fn)
#define CALL_GetActiveAttribARB(disp, parameters) (*((disp)->GetActiveAttribARB)) parameters
#define GET_GetActiveAttribARB(disp) ((disp)->GetActiveAttribARB)
#define SET_GetActiveAttribARB(disp, fn) ((disp)->GetActiveAttribARB = fn)
#define CALL_GetAttribLocationARB(disp, parameters) (*((disp)->GetAttribLocationARB)) parameters
#define GET_GetAttribLocationARB(disp) ((disp)->GetAttribLocationARB)
#define SET_GetAttribLocationARB(disp, fn) ((disp)->GetAttribLocationARB = fn)
#define CALL_DrawBuffersARB(disp, parameters) (*((disp)->DrawBuffersARB)) parameters
#define GET_DrawBuffersARB(disp) ((disp)->DrawBuffersARB)
#define SET_DrawBuffersARB(disp, fn) ((disp)->DrawBuffersARB = fn)
#define CALL_PolygonOffsetEXT(disp, parameters) (*((disp)->PolygonOffsetEXT)) parameters
#define GET_PolygonOffsetEXT(disp) ((disp)->PolygonOffsetEXT)
#define SET_PolygonOffsetEXT(disp, fn) ((disp)->PolygonOffsetEXT = fn)
#define CALL_GetPixelTexGenParameterfvSGIS(disp, parameters) (*((disp)->GetPixelTexGenParameterfvSGIS)) parameters
#define GET_GetPixelTexGenParameterfvSGIS(disp) ((disp)->GetPixelTexGenParameterfvSGIS)
#define SET_GetPixelTexGenParameterfvSGIS(disp, fn) ((disp)->GetPixelTexGenParameterfvSGIS = fn)
#define CALL_GetPixelTexGenParameterivSGIS(disp, parameters) (*((disp)->GetPixelTexGenParameterivSGIS)) parameters
#define GET_GetPixelTexGenParameterivSGIS(disp) ((disp)->GetPixelTexGenParameterivSGIS)
#define SET_GetPixelTexGenParameterivSGIS(disp, fn) ((disp)->GetPixelTexGenParameterivSGIS = fn)
#define CALL_PixelTexGenParameterfSGIS(disp, parameters) (*((disp)->PixelTexGenParameterfSGIS)) parameters
#define GET_PixelTexGenParameterfSGIS(disp) ((disp)->PixelTexGenParameterfSGIS)
#define SET_PixelTexGenParameterfSGIS(disp, fn) ((disp)->PixelTexGenParameterfSGIS = fn)
#define CALL_PixelTexGenParameterfvSGIS(disp, parameters) (*((disp)->PixelTexGenParameterfvSGIS)) parameters
#define GET_PixelTexGenParameterfvSGIS(disp) ((disp)->PixelTexGenParameterfvSGIS)
#define SET_PixelTexGenParameterfvSGIS(disp, fn) ((disp)->PixelTexGenParameterfvSGIS = fn)
#define CALL_PixelTexGenParameteriSGIS(disp, parameters) (*((disp)->PixelTexGenParameteriSGIS)) parameters
#define GET_PixelTexGenParameteriSGIS(disp) ((disp)->PixelTexGenParameteriSGIS)
#define SET_PixelTexGenParameteriSGIS(disp, fn) ((disp)->PixelTexGenParameteriSGIS = fn)
#define CALL_PixelTexGenParameterivSGIS(disp, parameters) (*((disp)->PixelTexGenParameterivSGIS)) parameters
#define GET_PixelTexGenParameterivSGIS(disp) ((disp)->PixelTexGenParameterivSGIS)
#define SET_PixelTexGenParameterivSGIS(disp, fn) ((disp)->PixelTexGenParameterivSGIS = fn)
#define CALL_SampleMaskSGIS(disp, parameters) (*((disp)->SampleMaskSGIS)) parameters
#define GET_SampleMaskSGIS(disp) ((disp)->SampleMaskSGIS)
#define SET_SampleMaskSGIS(disp, fn) ((disp)->SampleMaskSGIS = fn)
#define CALL_SamplePatternSGIS(disp, parameters) (*((disp)->SamplePatternSGIS)) parameters
#define GET_SamplePatternSGIS(disp) ((disp)->SamplePatternSGIS)
#define SET_SamplePatternSGIS(disp, fn) ((disp)->SamplePatternSGIS = fn)
#define CALL_ColorPointerEXT(disp, parameters) (*((disp)->ColorPointerEXT)) parameters
#define GET_ColorPointerEXT(disp) ((disp)->ColorPointerEXT)
#define SET_ColorPointerEXT(disp, fn) ((disp)->ColorPointerEXT = fn)
#define CALL_EdgeFlagPointerEXT(disp, parameters) (*((disp)->EdgeFlagPointerEXT)) parameters
#define GET_EdgeFlagPointerEXT(disp) ((disp)->EdgeFlagPointerEXT)
#define SET_EdgeFlagPointerEXT(disp, fn) ((disp)->EdgeFlagPointerEXT = fn)
#define CALL_IndexPointerEXT(disp, parameters) (*((disp)->IndexPointerEXT)) parameters
#define GET_IndexPointerEXT(disp) ((disp)->IndexPointerEXT)
#define SET_IndexPointerEXT(disp, fn) ((disp)->IndexPointerEXT = fn)
#define CALL_NormalPointerEXT(disp, parameters) (*((disp)->NormalPointerEXT)) parameters
#define GET_NormalPointerEXT(disp) ((disp)->NormalPointerEXT)
#define SET_NormalPointerEXT(disp, fn) ((disp)->NormalPointerEXT = fn)
#define CALL_TexCoordPointerEXT(disp, parameters) (*((disp)->TexCoordPointerEXT)) parameters
#define GET_TexCoordPointerEXT(disp) ((disp)->TexCoordPointerEXT)
#define SET_TexCoordPointerEXT(disp, fn) ((disp)->TexCoordPointerEXT = fn)
#define CALL_VertexPointerEXT(disp, parameters) (*((disp)->VertexPointerEXT)) parameters
#define GET_VertexPointerEXT(disp) ((disp)->VertexPointerEXT)
#define SET_VertexPointerEXT(disp, fn) ((disp)->VertexPointerEXT = fn)
#define CALL_PointParameterfEXT(disp, parameters) (*((disp)->PointParameterfEXT)) parameters
#define GET_PointParameterfEXT(disp) ((disp)->PointParameterfEXT)
#define SET_PointParameterfEXT(disp, fn) ((disp)->PointParameterfEXT = fn)
#define CALL_PointParameterfvEXT(disp, parameters) (*((disp)->PointParameterfvEXT)) parameters
#define GET_PointParameterfvEXT(disp) ((disp)->PointParameterfvEXT)
#define SET_PointParameterfvEXT(disp, fn) ((disp)->PointParameterfvEXT = fn)
#define CALL_LockArraysEXT(disp, parameters) (*((disp)->LockArraysEXT)) parameters
#define GET_LockArraysEXT(disp) ((disp)->LockArraysEXT)
#define SET_LockArraysEXT(disp, fn) ((disp)->LockArraysEXT = fn)
#define CALL_UnlockArraysEXT(disp, parameters) (*((disp)->UnlockArraysEXT)) parameters
#define GET_UnlockArraysEXT(disp) ((disp)->UnlockArraysEXT)
#define SET_UnlockArraysEXT(disp, fn) ((disp)->UnlockArraysEXT = fn)
#define CALL_CullParameterdvEXT(disp, parameters) (*((disp)->CullParameterdvEXT)) parameters
#define GET_CullParameterdvEXT(disp) ((disp)->CullParameterdvEXT)
#define SET_CullParameterdvEXT(disp, fn) ((disp)->CullParameterdvEXT = fn)
#define CALL_CullParameterfvEXT(disp, parameters) (*((disp)->CullParameterfvEXT)) parameters
#define GET_CullParameterfvEXT(disp) ((disp)->CullParameterfvEXT)
#define SET_CullParameterfvEXT(disp, fn) ((disp)->CullParameterfvEXT = fn)
#define CALL_SecondaryColor3bEXT(disp, parameters) (*((disp)->SecondaryColor3bEXT)) parameters
#define GET_SecondaryColor3bEXT(disp) ((disp)->SecondaryColor3bEXT)
#define SET_SecondaryColor3bEXT(disp, fn) ((disp)->SecondaryColor3bEXT = fn)
#define CALL_SecondaryColor3bvEXT(disp, parameters) (*((disp)->SecondaryColor3bvEXT)) parameters
#define GET_SecondaryColor3bvEXT(disp) ((disp)->SecondaryColor3bvEXT)
#define SET_SecondaryColor3bvEXT(disp, fn) ((disp)->SecondaryColor3bvEXT = fn)
#define CALL_SecondaryColor3dEXT(disp, parameters) (*((disp)->SecondaryColor3dEXT)) parameters
#define GET_SecondaryColor3dEXT(disp) ((disp)->SecondaryColor3dEXT)
#define SET_SecondaryColor3dEXT(disp, fn) ((disp)->SecondaryColor3dEXT = fn)
#define CALL_SecondaryColor3dvEXT(disp, parameters) (*((disp)->SecondaryColor3dvEXT)) parameters
#define GET_SecondaryColor3dvEXT(disp) ((disp)->SecondaryColor3dvEXT)
#define SET_SecondaryColor3dvEXT(disp, fn) ((disp)->SecondaryColor3dvEXT = fn)
#define CALL_SecondaryColor3fEXT(disp, parameters) (*((disp)->SecondaryColor3fEXT)) parameters
#define GET_SecondaryColor3fEXT(disp) ((disp)->SecondaryColor3fEXT)
#define SET_SecondaryColor3fEXT(disp, fn) ((disp)->SecondaryColor3fEXT = fn)
#define CALL_SecondaryColor3fvEXT(disp, parameters) (*((disp)->SecondaryColor3fvEXT)) parameters
#define GET_SecondaryColor3fvEXT(disp) ((disp)->SecondaryColor3fvEXT)
#define SET_SecondaryColor3fvEXT(disp, fn) ((disp)->SecondaryColor3fvEXT = fn)
#define CALL_SecondaryColor3iEXT(disp, parameters) (*((disp)->SecondaryColor3iEXT)) parameters
#define GET_SecondaryColor3iEXT(disp) ((disp)->SecondaryColor3iEXT)
#define SET_SecondaryColor3iEXT(disp, fn) ((disp)->SecondaryColor3iEXT = fn)
#define CALL_SecondaryColor3ivEXT(disp, parameters) (*((disp)->SecondaryColor3ivEXT)) parameters
#define GET_SecondaryColor3ivEXT(disp) ((disp)->SecondaryColor3ivEXT)
#define SET_SecondaryColor3ivEXT(disp, fn) ((disp)->SecondaryColor3ivEXT = fn)
#define CALL_SecondaryColor3sEXT(disp, parameters) (*((disp)->SecondaryColor3sEXT)) parameters
#define GET_SecondaryColor3sEXT(disp) ((disp)->SecondaryColor3sEXT)
#define SET_SecondaryColor3sEXT(disp, fn) ((disp)->SecondaryColor3sEXT = fn)
#define CALL_SecondaryColor3svEXT(disp, parameters) (*((disp)->SecondaryColor3svEXT)) parameters
#define GET_SecondaryColor3svEXT(disp) ((disp)->SecondaryColor3svEXT)
#define SET_SecondaryColor3svEXT(disp, fn) ((disp)->SecondaryColor3svEXT = fn)
#define CALL_SecondaryColor3ubEXT(disp, parameters) (*((disp)->SecondaryColor3ubEXT)) parameters
#define GET_SecondaryColor3ubEXT(disp) ((disp)->SecondaryColor3ubEXT)
#define SET_SecondaryColor3ubEXT(disp, fn) ((disp)->SecondaryColor3ubEXT = fn)
#define CALL_SecondaryColor3ubvEXT(disp, parameters) (*((disp)->SecondaryColor3ubvEXT)) parameters
#define GET_SecondaryColor3ubvEXT(disp) ((disp)->SecondaryColor3ubvEXT)
#define SET_SecondaryColor3ubvEXT(disp, fn) ((disp)->SecondaryColor3ubvEXT = fn)
#define CALL_SecondaryColor3uiEXT(disp, parameters) (*((disp)->SecondaryColor3uiEXT)) parameters
#define GET_SecondaryColor3uiEXT(disp) ((disp)->SecondaryColor3uiEXT)
#define SET_SecondaryColor3uiEXT(disp, fn) ((disp)->SecondaryColor3uiEXT = fn)
#define CALL_SecondaryColor3uivEXT(disp, parameters) (*((disp)->SecondaryColor3uivEXT)) parameters
#define GET_SecondaryColor3uivEXT(disp) ((disp)->SecondaryColor3uivEXT)
#define SET_SecondaryColor3uivEXT(disp, fn) ((disp)->SecondaryColor3uivEXT = fn)
#define CALL_SecondaryColor3usEXT(disp, parameters) (*((disp)->SecondaryColor3usEXT)) parameters
#define GET_SecondaryColor3usEXT(disp) ((disp)->SecondaryColor3usEXT)
#define SET_SecondaryColor3usEXT(disp, fn) ((disp)->SecondaryColor3usEXT = fn)
#define CALL_SecondaryColor3usvEXT(disp, parameters) (*((disp)->SecondaryColor3usvEXT)) parameters
#define GET_SecondaryColor3usvEXT(disp) ((disp)->SecondaryColor3usvEXT)
#define SET_SecondaryColor3usvEXT(disp, fn) ((disp)->SecondaryColor3usvEXT = fn)
#define CALL_SecondaryColorPointerEXT(disp, parameters) (*((disp)->SecondaryColorPointerEXT)) parameters
#define GET_SecondaryColorPointerEXT(disp) ((disp)->SecondaryColorPointerEXT)
#define SET_SecondaryColorPointerEXT(disp, fn) ((disp)->SecondaryColorPointerEXT = fn)
#define CALL_MultiDrawArraysEXT(disp, parameters) (*((disp)->MultiDrawArraysEXT)) parameters
#define GET_MultiDrawArraysEXT(disp) ((disp)->MultiDrawArraysEXT)
#define SET_MultiDrawArraysEXT(disp, fn) ((disp)->MultiDrawArraysEXT = fn)
#define CALL_MultiDrawElementsEXT(disp, parameters) (*((disp)->MultiDrawElementsEXT)) parameters
#define GET_MultiDrawElementsEXT(disp) ((disp)->MultiDrawElementsEXT)
#define SET_MultiDrawElementsEXT(disp, fn) ((disp)->MultiDrawElementsEXT = fn)
#define CALL_FogCoordPointerEXT(disp, parameters) (*((disp)->FogCoordPointerEXT)) parameters
#define GET_FogCoordPointerEXT(disp) ((disp)->FogCoordPointerEXT)
#define SET_FogCoordPointerEXT(disp, fn) ((disp)->FogCoordPointerEXT = fn)
#define CALL_FogCoorddEXT(disp, parameters) (*((disp)->FogCoorddEXT)) parameters
#define GET_FogCoorddEXT(disp) ((disp)->FogCoorddEXT)
#define SET_FogCoorddEXT(disp, fn) ((disp)->FogCoorddEXT = fn)
#define CALL_FogCoorddvEXT(disp, parameters) (*((disp)->FogCoorddvEXT)) parameters
#define GET_FogCoorddvEXT(disp) ((disp)->FogCoorddvEXT)
#define SET_FogCoorddvEXT(disp, fn) ((disp)->FogCoorddvEXT = fn)
#define CALL_FogCoordfEXT(disp, parameters) (*((disp)->FogCoordfEXT)) parameters
#define GET_FogCoordfEXT(disp) ((disp)->FogCoordfEXT)
#define SET_FogCoordfEXT(disp, fn) ((disp)->FogCoordfEXT = fn)
#define CALL_FogCoordfvEXT(disp, parameters) (*((disp)->FogCoordfvEXT)) parameters
#define GET_FogCoordfvEXT(disp) ((disp)->FogCoordfvEXT)
#define SET_FogCoordfvEXT(disp, fn) ((disp)->FogCoordfvEXT = fn)
#define CALL_PixelTexGenSGIX(disp, parameters) (*((disp)->PixelTexGenSGIX)) parameters
#define GET_PixelTexGenSGIX(disp) ((disp)->PixelTexGenSGIX)
#define SET_PixelTexGenSGIX(disp, fn) ((disp)->PixelTexGenSGIX = fn)
#define CALL_BlendFuncSeparateEXT(disp, parameters) (*((disp)->BlendFuncSeparateEXT)) parameters
#define GET_BlendFuncSeparateEXT(disp) ((disp)->BlendFuncSeparateEXT)
#define SET_BlendFuncSeparateEXT(disp, fn) ((disp)->BlendFuncSeparateEXT = fn)
#define CALL_FlushVertexArrayRangeNV(disp, parameters) (*((disp)->FlushVertexArrayRangeNV)) parameters
#define GET_FlushVertexArrayRangeNV(disp) ((disp)->FlushVertexArrayRangeNV)
#define SET_FlushVertexArrayRangeNV(disp, fn) ((disp)->FlushVertexArrayRangeNV = fn)
#define CALL_VertexArrayRangeNV(disp, parameters) (*((disp)->VertexArrayRangeNV)) parameters
#define GET_VertexArrayRangeNV(disp) ((disp)->VertexArrayRangeNV)
#define SET_VertexArrayRangeNV(disp, fn) ((disp)->VertexArrayRangeNV = fn)
#define CALL_CombinerInputNV(disp, parameters) (*((disp)->CombinerInputNV)) parameters
#define GET_CombinerInputNV(disp) ((disp)->CombinerInputNV)
#define SET_CombinerInputNV(disp, fn) ((disp)->CombinerInputNV = fn)
#define CALL_CombinerOutputNV(disp, parameters) (*((disp)->CombinerOutputNV)) parameters
#define GET_CombinerOutputNV(disp) ((disp)->CombinerOutputNV)
#define SET_CombinerOutputNV(disp, fn) ((disp)->CombinerOutputNV = fn)
#define CALL_CombinerParameterfNV(disp, parameters) (*((disp)->CombinerParameterfNV)) parameters
#define GET_CombinerParameterfNV(disp) ((disp)->CombinerParameterfNV)
#define SET_CombinerParameterfNV(disp, fn) ((disp)->CombinerParameterfNV = fn)
#define CALL_CombinerParameterfvNV(disp, parameters) (*((disp)->CombinerParameterfvNV)) parameters
#define GET_CombinerParameterfvNV(disp) ((disp)->CombinerParameterfvNV)
#define SET_CombinerParameterfvNV(disp, fn) ((disp)->CombinerParameterfvNV = fn)
#define CALL_CombinerParameteriNV(disp, parameters) (*((disp)->CombinerParameteriNV)) parameters
#define GET_CombinerParameteriNV(disp) ((disp)->CombinerParameteriNV)
#define SET_CombinerParameteriNV(disp, fn) ((disp)->CombinerParameteriNV = fn)
#define CALL_CombinerParameterivNV(disp, parameters) (*((disp)->CombinerParameterivNV)) parameters
#define GET_CombinerParameterivNV(disp) ((disp)->CombinerParameterivNV)
#define SET_CombinerParameterivNV(disp, fn) ((disp)->CombinerParameterivNV = fn)
#define CALL_FinalCombinerInputNV(disp, parameters) (*((disp)->FinalCombinerInputNV)) parameters
#define GET_FinalCombinerInputNV(disp) ((disp)->FinalCombinerInputNV)
#define SET_FinalCombinerInputNV(disp, fn) ((disp)->FinalCombinerInputNV = fn)
#define CALL_GetCombinerInputParameterfvNV(disp, parameters) (*((disp)->GetCombinerInputParameterfvNV)) parameters
#define GET_GetCombinerInputParameterfvNV(disp) ((disp)->GetCombinerInputParameterfvNV)
#define SET_GetCombinerInputParameterfvNV(disp, fn) ((disp)->GetCombinerInputParameterfvNV = fn)
#define CALL_GetCombinerInputParameterivNV(disp, parameters) (*((disp)->GetCombinerInputParameterivNV)) parameters
#define GET_GetCombinerInputParameterivNV(disp) ((disp)->GetCombinerInputParameterivNV)
#define SET_GetCombinerInputParameterivNV(disp, fn) ((disp)->GetCombinerInputParameterivNV = fn)
#define CALL_GetCombinerOutputParameterfvNV(disp, parameters) (*((disp)->GetCombinerOutputParameterfvNV)) parameters
#define GET_GetCombinerOutputParameterfvNV(disp) ((disp)->GetCombinerOutputParameterfvNV)
#define SET_GetCombinerOutputParameterfvNV(disp, fn) ((disp)->GetCombinerOutputParameterfvNV = fn)
#define CALL_GetCombinerOutputParameterivNV(disp, parameters) (*((disp)->GetCombinerOutputParameterivNV)) parameters
#define GET_GetCombinerOutputParameterivNV(disp) ((disp)->GetCombinerOutputParameterivNV)
#define SET_GetCombinerOutputParameterivNV(disp, fn) ((disp)->GetCombinerOutputParameterivNV = fn)
#define CALL_GetFinalCombinerInputParameterfvNV(disp, parameters) (*((disp)->GetFinalCombinerInputParameterfvNV)) parameters
#define GET_GetFinalCombinerInputParameterfvNV(disp) ((disp)->GetFinalCombinerInputParameterfvNV)
#define SET_GetFinalCombinerInputParameterfvNV(disp, fn) ((disp)->GetFinalCombinerInputParameterfvNV = fn)
#define CALL_GetFinalCombinerInputParameterivNV(disp, parameters) (*((disp)->GetFinalCombinerInputParameterivNV)) parameters
#define GET_GetFinalCombinerInputParameterivNV(disp) ((disp)->GetFinalCombinerInputParameterivNV)
#define SET_GetFinalCombinerInputParameterivNV(disp, fn) ((disp)->GetFinalCombinerInputParameterivNV = fn)
#define CALL_ResizeBuffersMESA(disp, parameters) (*((disp)->ResizeBuffersMESA)) parameters
#define GET_ResizeBuffersMESA(disp) ((disp)->ResizeBuffersMESA)
#define SET_ResizeBuffersMESA(disp, fn) ((disp)->ResizeBuffersMESA = fn)
#define CALL_WindowPos2dMESA(disp, parameters) (*((disp)->WindowPos2dMESA)) parameters
#define GET_WindowPos2dMESA(disp) ((disp)->WindowPos2dMESA)
#define SET_WindowPos2dMESA(disp, fn) ((disp)->WindowPos2dMESA = fn)
#define CALL_WindowPos2dvMESA(disp, parameters) (*((disp)->WindowPos2dvMESA)) parameters
#define GET_WindowPos2dvMESA(disp) ((disp)->WindowPos2dvMESA)
#define SET_WindowPos2dvMESA(disp, fn) ((disp)->WindowPos2dvMESA = fn)
#define CALL_WindowPos2fMESA(disp, parameters) (*((disp)->WindowPos2fMESA)) parameters
#define GET_WindowPos2fMESA(disp) ((disp)->WindowPos2fMESA)
#define SET_WindowPos2fMESA(disp, fn) ((disp)->WindowPos2fMESA = fn)
#define CALL_WindowPos2fvMESA(disp, parameters) (*((disp)->WindowPos2fvMESA)) parameters
#define GET_WindowPos2fvMESA(disp) ((disp)->WindowPos2fvMESA)
#define SET_WindowPos2fvMESA(disp, fn) ((disp)->WindowPos2fvMESA = fn)
#define CALL_WindowPos2iMESA(disp, parameters) (*((disp)->WindowPos2iMESA)) parameters
#define GET_WindowPos2iMESA(disp) ((disp)->WindowPos2iMESA)
#define SET_WindowPos2iMESA(disp, fn) ((disp)->WindowPos2iMESA = fn)
#define CALL_WindowPos2ivMESA(disp, parameters) (*((disp)->WindowPos2ivMESA)) parameters
#define GET_WindowPos2ivMESA(disp) ((disp)->WindowPos2ivMESA)
#define SET_WindowPos2ivMESA(disp, fn) ((disp)->WindowPos2ivMESA = fn)
#define CALL_WindowPos2sMESA(disp, parameters) (*((disp)->WindowPos2sMESA)) parameters
#define GET_WindowPos2sMESA(disp) ((disp)->WindowPos2sMESA)
#define SET_WindowPos2sMESA(disp, fn) ((disp)->WindowPos2sMESA = fn)
#define CALL_WindowPos2svMESA(disp, parameters) (*((disp)->WindowPos2svMESA)) parameters
#define GET_WindowPos2svMESA(disp) ((disp)->WindowPos2svMESA)
#define SET_WindowPos2svMESA(disp, fn) ((disp)->WindowPos2svMESA = fn)
#define CALL_WindowPos3dMESA(disp, parameters) (*((disp)->WindowPos3dMESA)) parameters
#define GET_WindowPos3dMESA(disp) ((disp)->WindowPos3dMESA)
#define SET_WindowPos3dMESA(disp, fn) ((disp)->WindowPos3dMESA = fn)
#define CALL_WindowPos3dvMESA(disp, parameters) (*((disp)->WindowPos3dvMESA)) parameters
#define GET_WindowPos3dvMESA(disp) ((disp)->WindowPos3dvMESA)
#define SET_WindowPos3dvMESA(disp, fn) ((disp)->WindowPos3dvMESA = fn)
#define CALL_WindowPos3fMESA(disp, parameters) (*((disp)->WindowPos3fMESA)) parameters
#define GET_WindowPos3fMESA(disp) ((disp)->WindowPos3fMESA)
#define SET_WindowPos3fMESA(disp, fn) ((disp)->WindowPos3fMESA = fn)
#define CALL_WindowPos3fvMESA(disp, parameters) (*((disp)->WindowPos3fvMESA)) parameters
#define GET_WindowPos3fvMESA(disp) ((disp)->WindowPos3fvMESA)
#define SET_WindowPos3fvMESA(disp, fn) ((disp)->WindowPos3fvMESA = fn)
#define CALL_WindowPos3iMESA(disp, parameters) (*((disp)->WindowPos3iMESA)) parameters
#define GET_WindowPos3iMESA(disp) ((disp)->WindowPos3iMESA)
#define SET_WindowPos3iMESA(disp, fn) ((disp)->WindowPos3iMESA = fn)
#define CALL_WindowPos3ivMESA(disp, parameters) (*((disp)->WindowPos3ivMESA)) parameters
#define GET_WindowPos3ivMESA(disp) ((disp)->WindowPos3ivMESA)
#define SET_WindowPos3ivMESA(disp, fn) ((disp)->WindowPos3ivMESA = fn)
#define CALL_WindowPos3sMESA(disp, parameters) (*((disp)->WindowPos3sMESA)) parameters
#define GET_WindowPos3sMESA(disp) ((disp)->WindowPos3sMESA)
#define SET_WindowPos3sMESA(disp, fn) ((disp)->WindowPos3sMESA = fn)
#define CALL_WindowPos3svMESA(disp, parameters) (*((disp)->WindowPos3svMESA)) parameters
#define GET_WindowPos3svMESA(disp) ((disp)->WindowPos3svMESA)
#define SET_WindowPos3svMESA(disp, fn) ((disp)->WindowPos3svMESA = fn)
#define CALL_WindowPos4dMESA(disp, parameters) (*((disp)->WindowPos4dMESA)) parameters
#define GET_WindowPos4dMESA(disp) ((disp)->WindowPos4dMESA)
#define SET_WindowPos4dMESA(disp, fn) ((disp)->WindowPos4dMESA = fn)
#define CALL_WindowPos4dvMESA(disp, parameters) (*((disp)->WindowPos4dvMESA)) parameters
#define GET_WindowPos4dvMESA(disp) ((disp)->WindowPos4dvMESA)
#define SET_WindowPos4dvMESA(disp, fn) ((disp)->WindowPos4dvMESA = fn)
#define CALL_WindowPos4fMESA(disp, parameters) (*((disp)->WindowPos4fMESA)) parameters
#define GET_WindowPos4fMESA(disp) ((disp)->WindowPos4fMESA)
#define SET_WindowPos4fMESA(disp, fn) ((disp)->WindowPos4fMESA = fn)
#define CALL_WindowPos4fvMESA(disp, parameters) (*((disp)->WindowPos4fvMESA)) parameters
#define GET_WindowPos4fvMESA(disp) ((disp)->WindowPos4fvMESA)
#define SET_WindowPos4fvMESA(disp, fn) ((disp)->WindowPos4fvMESA = fn)
#define CALL_WindowPos4iMESA(disp, parameters) (*((disp)->WindowPos4iMESA)) parameters
#define GET_WindowPos4iMESA(disp) ((disp)->WindowPos4iMESA)
#define SET_WindowPos4iMESA(disp, fn) ((disp)->WindowPos4iMESA = fn)
#define CALL_WindowPos4ivMESA(disp, parameters) (*((disp)->WindowPos4ivMESA)) parameters
#define GET_WindowPos4ivMESA(disp) ((disp)->WindowPos4ivMESA)
#define SET_WindowPos4ivMESA(disp, fn) ((disp)->WindowPos4ivMESA = fn)
#define CALL_WindowPos4sMESA(disp, parameters) (*((disp)->WindowPos4sMESA)) parameters
#define GET_WindowPos4sMESA(disp) ((disp)->WindowPos4sMESA)
#define SET_WindowPos4sMESA(disp, fn) ((disp)->WindowPos4sMESA = fn)
#define CALL_WindowPos4svMESA(disp, parameters) (*((disp)->WindowPos4svMESA)) parameters
#define GET_WindowPos4svMESA(disp) ((disp)->WindowPos4svMESA)
#define SET_WindowPos4svMESA(disp, fn) ((disp)->WindowPos4svMESA = fn)
#define CALL_MultiModeDrawArraysIBM(disp, parameters) (*((disp)->MultiModeDrawArraysIBM)) parameters
#define GET_MultiModeDrawArraysIBM(disp) ((disp)->MultiModeDrawArraysIBM)
#define SET_MultiModeDrawArraysIBM(disp, fn) ((disp)->MultiModeDrawArraysIBM = fn)
#define CALL_MultiModeDrawElementsIBM(disp, parameters) (*((disp)->MultiModeDrawElementsIBM)) parameters
#define GET_MultiModeDrawElementsIBM(disp) ((disp)->MultiModeDrawElementsIBM)
#define SET_MultiModeDrawElementsIBM(disp, fn) ((disp)->MultiModeDrawElementsIBM = fn)
#define CALL_DeleteFencesNV(disp, parameters) (*((disp)->DeleteFencesNV)) parameters
#define GET_DeleteFencesNV(disp) ((disp)->DeleteFencesNV)
#define SET_DeleteFencesNV(disp, fn) ((disp)->DeleteFencesNV = fn)
#define CALL_FinishFenceNV(disp, parameters) (*((disp)->FinishFenceNV)) parameters
#define GET_FinishFenceNV(disp) ((disp)->FinishFenceNV)
#define SET_FinishFenceNV(disp, fn) ((disp)->FinishFenceNV = fn)
#define CALL_GenFencesNV(disp, parameters) (*((disp)->GenFencesNV)) parameters
#define GET_GenFencesNV(disp) ((disp)->GenFencesNV)
#define SET_GenFencesNV(disp, fn) ((disp)->GenFencesNV = fn)
#define CALL_GetFenceivNV(disp, parameters) (*((disp)->GetFenceivNV)) parameters
#define GET_GetFenceivNV(disp) ((disp)->GetFenceivNV)
#define SET_GetFenceivNV(disp, fn) ((disp)->GetFenceivNV = fn)
#define CALL_IsFenceNV(disp, parameters) (*((disp)->IsFenceNV)) parameters
#define GET_IsFenceNV(disp) ((disp)->IsFenceNV)
#define SET_IsFenceNV(disp, fn) ((disp)->IsFenceNV = fn)
#define CALL_SetFenceNV(disp, parameters) (*((disp)->SetFenceNV)) parameters
#define GET_SetFenceNV(disp) ((disp)->SetFenceNV)
#define SET_SetFenceNV(disp, fn) ((disp)->SetFenceNV = fn)
#define CALL_TestFenceNV(disp, parameters) (*((disp)->TestFenceNV)) parameters
#define GET_TestFenceNV(disp) ((disp)->TestFenceNV)
#define SET_TestFenceNV(disp, fn) ((disp)->TestFenceNV = fn)
#define CALL_AreProgramsResidentNV(disp, parameters) (*((disp)->AreProgramsResidentNV)) parameters
#define GET_AreProgramsResidentNV(disp) ((disp)->AreProgramsResidentNV)
#define SET_AreProgramsResidentNV(disp, fn) ((disp)->AreProgramsResidentNV = fn)
#define CALL_BindProgramNV(disp, parameters) (*((disp)->BindProgramNV)) parameters
#define GET_BindProgramNV(disp) ((disp)->BindProgramNV)
#define SET_BindProgramNV(disp, fn) ((disp)->BindProgramNV = fn)
#define CALL_DeleteProgramsNV(disp, parameters) (*((disp)->DeleteProgramsNV)) parameters
#define GET_DeleteProgramsNV(disp) ((disp)->DeleteProgramsNV)
#define SET_DeleteProgramsNV(disp, fn) ((disp)->DeleteProgramsNV = fn)
#define CALL_ExecuteProgramNV(disp, parameters) (*((disp)->ExecuteProgramNV)) parameters
#define GET_ExecuteProgramNV(disp) ((disp)->ExecuteProgramNV)
#define SET_ExecuteProgramNV(disp, fn) ((disp)->ExecuteProgramNV = fn)
#define CALL_GenProgramsNV(disp, parameters) (*((disp)->GenProgramsNV)) parameters
#define GET_GenProgramsNV(disp) ((disp)->GenProgramsNV)
#define SET_GenProgramsNV(disp, fn) ((disp)->GenProgramsNV = fn)
#define CALL_GetProgramParameterdvNV(disp, parameters) (*((disp)->GetProgramParameterdvNV)) parameters
#define GET_GetProgramParameterdvNV(disp) ((disp)->GetProgramParameterdvNV)
#define SET_GetProgramParameterdvNV(disp, fn) ((disp)->GetProgramParameterdvNV = fn)
#define CALL_GetProgramParameterfvNV(disp, parameters) (*((disp)->GetProgramParameterfvNV)) parameters
#define GET_GetProgramParameterfvNV(disp) ((disp)->GetProgramParameterfvNV)
#define SET_GetProgramParameterfvNV(disp, fn) ((disp)->GetProgramParameterfvNV = fn)
#define CALL_GetProgramStringNV(disp, parameters) (*((disp)->GetProgramStringNV)) parameters
#define GET_GetProgramStringNV(disp) ((disp)->GetProgramStringNV)
#define SET_GetProgramStringNV(disp, fn) ((disp)->GetProgramStringNV = fn)
#define CALL_GetProgramivNV(disp, parameters) (*((disp)->GetProgramivNV)) parameters
#define GET_GetProgramivNV(disp) ((disp)->GetProgramivNV)
#define SET_GetProgramivNV(disp, fn) ((disp)->GetProgramivNV = fn)
#define CALL_GetTrackMatrixivNV(disp, parameters) (*((disp)->GetTrackMatrixivNV)) parameters
#define GET_GetTrackMatrixivNV(disp) ((disp)->GetTrackMatrixivNV)
#define SET_GetTrackMatrixivNV(disp, fn) ((disp)->GetTrackMatrixivNV = fn)
#define CALL_GetVertexAttribPointervNV(disp, parameters) (*((disp)->GetVertexAttribPointervNV)) parameters
#define GET_GetVertexAttribPointervNV(disp) ((disp)->GetVertexAttribPointervNV)
#define SET_GetVertexAttribPointervNV(disp, fn) ((disp)->GetVertexAttribPointervNV = fn)
#define CALL_GetVertexAttribdvNV(disp, parameters) (*((disp)->GetVertexAttribdvNV)) parameters
#define GET_GetVertexAttribdvNV(disp) ((disp)->GetVertexAttribdvNV)
#define SET_GetVertexAttribdvNV(disp, fn) ((disp)->GetVertexAttribdvNV = fn)
#define CALL_GetVertexAttribfvNV(disp, parameters) (*((disp)->GetVertexAttribfvNV)) parameters
#define GET_GetVertexAttribfvNV(disp) ((disp)->GetVertexAttribfvNV)
#define SET_GetVertexAttribfvNV(disp, fn) ((disp)->GetVertexAttribfvNV = fn)
#define CALL_GetVertexAttribivNV(disp, parameters) (*((disp)->GetVertexAttribivNV)) parameters
#define GET_GetVertexAttribivNV(disp) ((disp)->GetVertexAttribivNV)
#define SET_GetVertexAttribivNV(disp, fn) ((disp)->GetVertexAttribivNV = fn)
#define CALL_IsProgramNV(disp, parameters) (*((disp)->IsProgramNV)) parameters
#define GET_IsProgramNV(disp) ((disp)->IsProgramNV)
#define SET_IsProgramNV(disp, fn) ((disp)->IsProgramNV = fn)
#define CALL_LoadProgramNV(disp, parameters) (*((disp)->LoadProgramNV)) parameters
#define GET_LoadProgramNV(disp) ((disp)->LoadProgramNV)
#define SET_LoadProgramNV(disp, fn) ((disp)->LoadProgramNV = fn)
#define CALL_ProgramParameter4dNV(disp, parameters) (*((disp)->ProgramParameter4dNV)) parameters
#define GET_ProgramParameter4dNV(disp) ((disp)->ProgramParameter4dNV)
#define SET_ProgramParameter4dNV(disp, fn) ((disp)->ProgramParameter4dNV = fn)
#define CALL_ProgramParameter4dvNV(disp, parameters) (*((disp)->ProgramParameter4dvNV)) parameters
#define GET_ProgramParameter4dvNV(disp) ((disp)->ProgramParameter4dvNV)
#define SET_ProgramParameter4dvNV(disp, fn) ((disp)->ProgramParameter4dvNV = fn)
#define CALL_ProgramParameter4fNV(disp, parameters) (*((disp)->ProgramParameter4fNV)) parameters
#define GET_ProgramParameter4fNV(disp) ((disp)->ProgramParameter4fNV)
#define SET_ProgramParameter4fNV(disp, fn) ((disp)->ProgramParameter4fNV = fn)
#define CALL_ProgramParameter4fvNV(disp, parameters) (*((disp)->ProgramParameter4fvNV)) parameters
#define GET_ProgramParameter4fvNV(disp) ((disp)->ProgramParameter4fvNV)
#define SET_ProgramParameter4fvNV(disp, fn) ((disp)->ProgramParameter4fvNV = fn)
#define CALL_ProgramParameters4dvNV(disp, parameters) (*((disp)->ProgramParameters4dvNV)) parameters
#define GET_ProgramParameters4dvNV(disp) ((disp)->ProgramParameters4dvNV)
#define SET_ProgramParameters4dvNV(disp, fn) ((disp)->ProgramParameters4dvNV = fn)
#define CALL_ProgramParameters4fvNV(disp, parameters) (*((disp)->ProgramParameters4fvNV)) parameters
#define GET_ProgramParameters4fvNV(disp) ((disp)->ProgramParameters4fvNV)
#define SET_ProgramParameters4fvNV(disp, fn) ((disp)->ProgramParameters4fvNV = fn)
#define CALL_RequestResidentProgramsNV(disp, parameters) (*((disp)->RequestResidentProgramsNV)) parameters
#define GET_RequestResidentProgramsNV(disp) ((disp)->RequestResidentProgramsNV)
#define SET_RequestResidentProgramsNV(disp, fn) ((disp)->RequestResidentProgramsNV = fn)
#define CALL_TrackMatrixNV(disp, parameters) (*((disp)->TrackMatrixNV)) parameters
#define GET_TrackMatrixNV(disp) ((disp)->TrackMatrixNV)
#define SET_TrackMatrixNV(disp, fn) ((disp)->TrackMatrixNV = fn)
#define CALL_VertexAttrib1dNV(disp, parameters) (*((disp)->VertexAttrib1dNV)) parameters
#define GET_VertexAttrib1dNV(disp) ((disp)->VertexAttrib1dNV)
#define SET_VertexAttrib1dNV(disp, fn) ((disp)->VertexAttrib1dNV = fn)
#define CALL_VertexAttrib1dvNV(disp, parameters) (*((disp)->VertexAttrib1dvNV)) parameters
#define GET_VertexAttrib1dvNV(disp) ((disp)->VertexAttrib1dvNV)
#define SET_VertexAttrib1dvNV(disp, fn) ((disp)->VertexAttrib1dvNV = fn)
#define CALL_VertexAttrib1fNV(disp, parameters) (*((disp)->VertexAttrib1fNV)) parameters
#define GET_VertexAttrib1fNV(disp) ((disp)->VertexAttrib1fNV)
#define SET_VertexAttrib1fNV(disp, fn) ((disp)->VertexAttrib1fNV = fn)
#define CALL_VertexAttrib1fvNV(disp, parameters) (*((disp)->VertexAttrib1fvNV)) parameters
#define GET_VertexAttrib1fvNV(disp) ((disp)->VertexAttrib1fvNV)
#define SET_VertexAttrib1fvNV(disp, fn) ((disp)->VertexAttrib1fvNV = fn)
#define CALL_VertexAttrib1sNV(disp, parameters) (*((disp)->VertexAttrib1sNV)) parameters
#define GET_VertexAttrib1sNV(disp) ((disp)->VertexAttrib1sNV)
#define SET_VertexAttrib1sNV(disp, fn) ((disp)->VertexAttrib1sNV = fn)
#define CALL_VertexAttrib1svNV(disp, parameters) (*((disp)->VertexAttrib1svNV)) parameters
#define GET_VertexAttrib1svNV(disp) ((disp)->VertexAttrib1svNV)
#define SET_VertexAttrib1svNV(disp, fn) ((disp)->VertexAttrib1svNV = fn)
#define CALL_VertexAttrib2dNV(disp, parameters) (*((disp)->VertexAttrib2dNV)) parameters
#define GET_VertexAttrib2dNV(disp) ((disp)->VertexAttrib2dNV)
#define SET_VertexAttrib2dNV(disp, fn) ((disp)->VertexAttrib2dNV = fn)
#define CALL_VertexAttrib2dvNV(disp, parameters) (*((disp)->VertexAttrib2dvNV)) parameters
#define GET_VertexAttrib2dvNV(disp) ((disp)->VertexAttrib2dvNV)
#define SET_VertexAttrib2dvNV(disp, fn) ((disp)->VertexAttrib2dvNV = fn)
#define CALL_VertexAttrib2fNV(disp, parameters) (*((disp)->VertexAttrib2fNV)) parameters
#define GET_VertexAttrib2fNV(disp) ((disp)->VertexAttrib2fNV)
#define SET_VertexAttrib2fNV(disp, fn) ((disp)->VertexAttrib2fNV = fn)
#define CALL_VertexAttrib2fvNV(disp, parameters) (*((disp)->VertexAttrib2fvNV)) parameters
#define GET_VertexAttrib2fvNV(disp) ((disp)->VertexAttrib2fvNV)
#define SET_VertexAttrib2fvNV(disp, fn) ((disp)->VertexAttrib2fvNV = fn)
#define CALL_VertexAttrib2sNV(disp, parameters) (*((disp)->VertexAttrib2sNV)) parameters
#define GET_VertexAttrib2sNV(disp) ((disp)->VertexAttrib2sNV)
#define SET_VertexAttrib2sNV(disp, fn) ((disp)->VertexAttrib2sNV = fn)
#define CALL_VertexAttrib2svNV(disp, parameters) (*((disp)->VertexAttrib2svNV)) parameters
#define GET_VertexAttrib2svNV(disp) ((disp)->VertexAttrib2svNV)
#define SET_VertexAttrib2svNV(disp, fn) ((disp)->VertexAttrib2svNV = fn)
#define CALL_VertexAttrib3dNV(disp, parameters) (*((disp)->VertexAttrib3dNV)) parameters
#define GET_VertexAttrib3dNV(disp) ((disp)->VertexAttrib3dNV)
#define SET_VertexAttrib3dNV(disp, fn) ((disp)->VertexAttrib3dNV = fn)
#define CALL_VertexAttrib3dvNV(disp, parameters) (*((disp)->VertexAttrib3dvNV)) parameters
#define GET_VertexAttrib3dvNV(disp) ((disp)->VertexAttrib3dvNV)
#define SET_VertexAttrib3dvNV(disp, fn) ((disp)->VertexAttrib3dvNV = fn)
#define CALL_VertexAttrib3fNV(disp, parameters) (*((disp)->VertexAttrib3fNV)) parameters
#define GET_VertexAttrib3fNV(disp) ((disp)->VertexAttrib3fNV)
#define SET_VertexAttrib3fNV(disp, fn) ((disp)->VertexAttrib3fNV = fn)
#define CALL_VertexAttrib3fvNV(disp, parameters) (*((disp)->VertexAttrib3fvNV)) parameters
#define GET_VertexAttrib3fvNV(disp) ((disp)->VertexAttrib3fvNV)
#define SET_VertexAttrib3fvNV(disp, fn) ((disp)->VertexAttrib3fvNV = fn)
#define CALL_VertexAttrib3sNV(disp, parameters) (*((disp)->VertexAttrib3sNV)) parameters
#define GET_VertexAttrib3sNV(disp) ((disp)->VertexAttrib3sNV)
#define SET_VertexAttrib3sNV(disp, fn) ((disp)->VertexAttrib3sNV = fn)
#define CALL_VertexAttrib3svNV(disp, parameters) (*((disp)->VertexAttrib3svNV)) parameters
#define GET_VertexAttrib3svNV(disp) ((disp)->VertexAttrib3svNV)
#define SET_VertexAttrib3svNV(disp, fn) ((disp)->VertexAttrib3svNV = fn)
#define CALL_VertexAttrib4dNV(disp, parameters) (*((disp)->VertexAttrib4dNV)) parameters
#define GET_VertexAttrib4dNV(disp) ((disp)->VertexAttrib4dNV)
#define SET_VertexAttrib4dNV(disp, fn) ((disp)->VertexAttrib4dNV = fn)
#define CALL_VertexAttrib4dvNV(disp, parameters) (*((disp)->VertexAttrib4dvNV)) parameters
#define GET_VertexAttrib4dvNV(disp) ((disp)->VertexAttrib4dvNV)
#define SET_VertexAttrib4dvNV(disp, fn) ((disp)->VertexAttrib4dvNV = fn)
#define CALL_VertexAttrib4fNV(disp, parameters) (*((disp)->VertexAttrib4fNV)) parameters
#define GET_VertexAttrib4fNV(disp) ((disp)->VertexAttrib4fNV)
#define SET_VertexAttrib4fNV(disp, fn) ((disp)->VertexAttrib4fNV = fn)
#define CALL_VertexAttrib4fvNV(disp, parameters) (*((disp)->VertexAttrib4fvNV)) parameters
#define GET_VertexAttrib4fvNV(disp) ((disp)->VertexAttrib4fvNV)
#define SET_VertexAttrib4fvNV(disp, fn) ((disp)->VertexAttrib4fvNV = fn)
#define CALL_VertexAttrib4sNV(disp, parameters) (*((disp)->VertexAttrib4sNV)) parameters
#define GET_VertexAttrib4sNV(disp) ((disp)->VertexAttrib4sNV)
#define SET_VertexAttrib4sNV(disp, fn) ((disp)->VertexAttrib4sNV = fn)
#define CALL_VertexAttrib4svNV(disp, parameters) (*((disp)->VertexAttrib4svNV)) parameters
#define GET_VertexAttrib4svNV(disp) ((disp)->VertexAttrib4svNV)
#define SET_VertexAttrib4svNV(disp, fn) ((disp)->VertexAttrib4svNV = fn)
#define CALL_VertexAttrib4ubNV(disp, parameters) (*((disp)->VertexAttrib4ubNV)) parameters
#define GET_VertexAttrib4ubNV(disp) ((disp)->VertexAttrib4ubNV)
#define SET_VertexAttrib4ubNV(disp, fn) ((disp)->VertexAttrib4ubNV = fn)
#define CALL_VertexAttrib4ubvNV(disp, parameters) (*((disp)->VertexAttrib4ubvNV)) parameters
#define GET_VertexAttrib4ubvNV(disp) ((disp)->VertexAttrib4ubvNV)
#define SET_VertexAttrib4ubvNV(disp, fn) ((disp)->VertexAttrib4ubvNV = fn)
#define CALL_VertexAttribPointerNV(disp, parameters) (*((disp)->VertexAttribPointerNV)) parameters
#define GET_VertexAttribPointerNV(disp) ((disp)->VertexAttribPointerNV)
#define SET_VertexAttribPointerNV(disp, fn) ((disp)->VertexAttribPointerNV = fn)
#define CALL_VertexAttribs1dvNV(disp, parameters) (*((disp)->VertexAttribs1dvNV)) parameters
#define GET_VertexAttribs1dvNV(disp) ((disp)->VertexAttribs1dvNV)
#define SET_VertexAttribs1dvNV(disp, fn) ((disp)->VertexAttribs1dvNV = fn)
#define CALL_VertexAttribs1fvNV(disp, parameters) (*((disp)->VertexAttribs1fvNV)) parameters
#define GET_VertexAttribs1fvNV(disp) ((disp)->VertexAttribs1fvNV)
#define SET_VertexAttribs1fvNV(disp, fn) ((disp)->VertexAttribs1fvNV = fn)
#define CALL_VertexAttribs1svNV(disp, parameters) (*((disp)->VertexAttribs1svNV)) parameters
#define GET_VertexAttribs1svNV(disp) ((disp)->VertexAttribs1svNV)
#define SET_VertexAttribs1svNV(disp, fn) ((disp)->VertexAttribs1svNV = fn)
#define CALL_VertexAttribs2dvNV(disp, parameters) (*((disp)->VertexAttribs2dvNV)) parameters
#define GET_VertexAttribs2dvNV(disp) ((disp)->VertexAttribs2dvNV)
#define SET_VertexAttribs2dvNV(disp, fn) ((disp)->VertexAttribs2dvNV = fn)
#define CALL_VertexAttribs2fvNV(disp, parameters) (*((disp)->VertexAttribs2fvNV)) parameters
#define GET_VertexAttribs2fvNV(disp) ((disp)->VertexAttribs2fvNV)
#define SET_VertexAttribs2fvNV(disp, fn) ((disp)->VertexAttribs2fvNV = fn)
#define CALL_VertexAttribs2svNV(disp, parameters) (*((disp)->VertexAttribs2svNV)) parameters
#define GET_VertexAttribs2svNV(disp) ((disp)->VertexAttribs2svNV)
#define SET_VertexAttribs2svNV(disp, fn) ((disp)->VertexAttribs2svNV = fn)
#define CALL_VertexAttribs3dvNV(disp, parameters) (*((disp)->VertexAttribs3dvNV)) parameters
#define GET_VertexAttribs3dvNV(disp) ((disp)->VertexAttribs3dvNV)
#define SET_VertexAttribs3dvNV(disp, fn) ((disp)->VertexAttribs3dvNV = fn)
#define CALL_VertexAttribs3fvNV(disp, parameters) (*((disp)->VertexAttribs3fvNV)) parameters
#define GET_VertexAttribs3fvNV(disp) ((disp)->VertexAttribs3fvNV)
#define SET_VertexAttribs3fvNV(disp, fn) ((disp)->VertexAttribs3fvNV = fn)
#define CALL_VertexAttribs3svNV(disp, parameters) (*((disp)->VertexAttribs3svNV)) parameters
#define GET_VertexAttribs3svNV(disp) ((disp)->VertexAttribs3svNV)
#define SET_VertexAttribs3svNV(disp, fn) ((disp)->VertexAttribs3svNV = fn)
#define CALL_VertexAttribs4dvNV(disp, parameters) (*((disp)->VertexAttribs4dvNV)) parameters
#define GET_VertexAttribs4dvNV(disp) ((disp)->VertexAttribs4dvNV)
#define SET_VertexAttribs4dvNV(disp, fn) ((disp)->VertexAttribs4dvNV = fn)
#define CALL_VertexAttribs4fvNV(disp, parameters) (*((disp)->VertexAttribs4fvNV)) parameters
#define GET_VertexAttribs4fvNV(disp) ((disp)->VertexAttribs4fvNV)
#define SET_VertexAttribs4fvNV(disp, fn) ((disp)->VertexAttribs4fvNV = fn)
#define CALL_VertexAttribs4svNV(disp, parameters) (*((disp)->VertexAttribs4svNV)) parameters
#define GET_VertexAttribs4svNV(disp) ((disp)->VertexAttribs4svNV)
#define SET_VertexAttribs4svNV(disp, fn) ((disp)->VertexAttribs4svNV = fn)
#define CALL_VertexAttribs4ubvNV(disp, parameters) (*((disp)->VertexAttribs4ubvNV)) parameters
#define GET_VertexAttribs4ubvNV(disp) ((disp)->VertexAttribs4ubvNV)
#define SET_VertexAttribs4ubvNV(disp, fn) ((disp)->VertexAttribs4ubvNV = fn)
#define CALL_AlphaFragmentOp1ATI(disp, parameters) (*((disp)->AlphaFragmentOp1ATI)) parameters
#define GET_AlphaFragmentOp1ATI(disp) ((disp)->AlphaFragmentOp1ATI)
#define SET_AlphaFragmentOp1ATI(disp, fn) ((disp)->AlphaFragmentOp1ATI = fn)
#define CALL_AlphaFragmentOp2ATI(disp, parameters) (*((disp)->AlphaFragmentOp2ATI)) parameters
#define GET_AlphaFragmentOp2ATI(disp) ((disp)->AlphaFragmentOp2ATI)
#define SET_AlphaFragmentOp2ATI(disp, fn) ((disp)->AlphaFragmentOp2ATI = fn)
#define CALL_AlphaFragmentOp3ATI(disp, parameters) (*((disp)->AlphaFragmentOp3ATI)) parameters
#define GET_AlphaFragmentOp3ATI(disp) ((disp)->AlphaFragmentOp3ATI)
#define SET_AlphaFragmentOp3ATI(disp, fn) ((disp)->AlphaFragmentOp3ATI = fn)
#define CALL_BeginFragmentShaderATI(disp, parameters) (*((disp)->BeginFragmentShaderATI)) parameters
#define GET_BeginFragmentShaderATI(disp) ((disp)->BeginFragmentShaderATI)
#define SET_BeginFragmentShaderATI(disp, fn) ((disp)->BeginFragmentShaderATI = fn)
#define CALL_BindFragmentShaderATI(disp, parameters) (*((disp)->BindFragmentShaderATI)) parameters
#define GET_BindFragmentShaderATI(disp) ((disp)->BindFragmentShaderATI)
#define SET_BindFragmentShaderATI(disp, fn) ((disp)->BindFragmentShaderATI = fn)
#define CALL_ColorFragmentOp1ATI(disp, parameters) (*((disp)->ColorFragmentOp1ATI)) parameters
#define GET_ColorFragmentOp1ATI(disp) ((disp)->ColorFragmentOp1ATI)
#define SET_ColorFragmentOp1ATI(disp, fn) ((disp)->ColorFragmentOp1ATI = fn)
#define CALL_ColorFragmentOp2ATI(disp, parameters) (*((disp)->ColorFragmentOp2ATI)) parameters
#define GET_ColorFragmentOp2ATI(disp) ((disp)->ColorFragmentOp2ATI)
#define SET_ColorFragmentOp2ATI(disp, fn) ((disp)->ColorFragmentOp2ATI = fn)
#define CALL_ColorFragmentOp3ATI(disp, parameters) (*((disp)->ColorFragmentOp3ATI)) parameters
#define GET_ColorFragmentOp3ATI(disp) ((disp)->ColorFragmentOp3ATI)
#define SET_ColorFragmentOp3ATI(disp, fn) ((disp)->ColorFragmentOp3ATI = fn)
#define CALL_DeleteFragmentShaderATI(disp, parameters) (*((disp)->DeleteFragmentShaderATI)) parameters
#define GET_DeleteFragmentShaderATI(disp) ((disp)->DeleteFragmentShaderATI)
#define SET_DeleteFragmentShaderATI(disp, fn) ((disp)->DeleteFragmentShaderATI = fn)
#define CALL_EndFragmentShaderATI(disp, parameters) (*((disp)->EndFragmentShaderATI)) parameters
#define GET_EndFragmentShaderATI(disp) ((disp)->EndFragmentShaderATI)
#define SET_EndFragmentShaderATI(disp, fn) ((disp)->EndFragmentShaderATI = fn)
#define CALL_GenFragmentShadersATI(disp, parameters) (*((disp)->GenFragmentShadersATI)) parameters
#define GET_GenFragmentShadersATI(disp) ((disp)->GenFragmentShadersATI)
#define SET_GenFragmentShadersATI(disp, fn) ((disp)->GenFragmentShadersATI = fn)
#define CALL_PassTexCoordATI(disp, parameters) (*((disp)->PassTexCoordATI)) parameters
#define GET_PassTexCoordATI(disp) ((disp)->PassTexCoordATI)
#define SET_PassTexCoordATI(disp, fn) ((disp)->PassTexCoordATI = fn)
#define CALL_SampleMapATI(disp, parameters) (*((disp)->SampleMapATI)) parameters
#define GET_SampleMapATI(disp) ((disp)->SampleMapATI)
#define SET_SampleMapATI(disp, fn) ((disp)->SampleMapATI = fn)
#define CALL_SetFragmentShaderConstantATI(disp, parameters) (*((disp)->SetFragmentShaderConstantATI)) parameters
#define GET_SetFragmentShaderConstantATI(disp) ((disp)->SetFragmentShaderConstantATI)
#define SET_SetFragmentShaderConstantATI(disp, fn) ((disp)->SetFragmentShaderConstantATI = fn)
#define CALL_PointParameteriNV(disp, parameters) (*((disp)->PointParameteriNV)) parameters
#define GET_PointParameteriNV(disp) ((disp)->PointParameteriNV)
#define SET_PointParameteriNV(disp, fn) ((disp)->PointParameteriNV = fn)
#define CALL_PointParameterivNV(disp, parameters) (*((disp)->PointParameterivNV)) parameters
#define GET_PointParameterivNV(disp) ((disp)->PointParameterivNV)
#define SET_PointParameterivNV(disp, fn) ((disp)->PointParameterivNV = fn)
#define CALL_ActiveStencilFaceEXT(disp, parameters) (*((disp)->ActiveStencilFaceEXT)) parameters
#define GET_ActiveStencilFaceEXT(disp) ((disp)->ActiveStencilFaceEXT)
#define SET_ActiveStencilFaceEXT(disp, fn) ((disp)->ActiveStencilFaceEXT = fn)
#define CALL_BindVertexArrayAPPLE(disp, parameters) (*((disp)->BindVertexArrayAPPLE)) parameters
#define GET_BindVertexArrayAPPLE(disp) ((disp)->BindVertexArrayAPPLE)
#define SET_BindVertexArrayAPPLE(disp, fn) ((disp)->BindVertexArrayAPPLE = fn)
#define CALL_DeleteVertexArraysAPPLE(disp, parameters) (*((disp)->DeleteVertexArraysAPPLE)) parameters
#define GET_DeleteVertexArraysAPPLE(disp) ((disp)->DeleteVertexArraysAPPLE)
#define SET_DeleteVertexArraysAPPLE(disp, fn) ((disp)->DeleteVertexArraysAPPLE = fn)
#define CALL_GenVertexArraysAPPLE(disp, parameters) (*((disp)->GenVertexArraysAPPLE)) parameters
#define GET_GenVertexArraysAPPLE(disp) ((disp)->GenVertexArraysAPPLE)
#define SET_GenVertexArraysAPPLE(disp, fn) ((disp)->GenVertexArraysAPPLE = fn)
#define CALL_IsVertexArrayAPPLE(disp, parameters) (*((disp)->IsVertexArrayAPPLE)) parameters
#define GET_IsVertexArrayAPPLE(disp) ((disp)->IsVertexArrayAPPLE)
#define SET_IsVertexArrayAPPLE(disp, fn) ((disp)->IsVertexArrayAPPLE = fn)
#define CALL_GetProgramNamedParameterdvNV(disp, parameters) (*((disp)->GetProgramNamedParameterdvNV)) parameters
#define GET_GetProgramNamedParameterdvNV(disp) ((disp)->GetProgramNamedParameterdvNV)
#define SET_GetProgramNamedParameterdvNV(disp, fn) ((disp)->GetProgramNamedParameterdvNV = fn)
#define CALL_GetProgramNamedParameterfvNV(disp, parameters) (*((disp)->GetProgramNamedParameterfvNV)) parameters
#define GET_GetProgramNamedParameterfvNV(disp) ((disp)->GetProgramNamedParameterfvNV)
#define SET_GetProgramNamedParameterfvNV(disp, fn) ((disp)->GetProgramNamedParameterfvNV = fn)
#define CALL_ProgramNamedParameter4dNV(disp, parameters) (*((disp)->ProgramNamedParameter4dNV)) parameters
#define GET_ProgramNamedParameter4dNV(disp) ((disp)->ProgramNamedParameter4dNV)
#define SET_ProgramNamedParameter4dNV(disp, fn) ((disp)->ProgramNamedParameter4dNV = fn)
#define CALL_ProgramNamedParameter4dvNV(disp, parameters) (*((disp)->ProgramNamedParameter4dvNV)) parameters
#define GET_ProgramNamedParameter4dvNV(disp) ((disp)->ProgramNamedParameter4dvNV)
#define SET_ProgramNamedParameter4dvNV(disp, fn) ((disp)->ProgramNamedParameter4dvNV = fn)
#define CALL_ProgramNamedParameter4fNV(disp, parameters) (*((disp)->ProgramNamedParameter4fNV)) parameters
#define GET_ProgramNamedParameter4fNV(disp) ((disp)->ProgramNamedParameter4fNV)
#define SET_ProgramNamedParameter4fNV(disp, fn) ((disp)->ProgramNamedParameter4fNV = fn)
#define CALL_ProgramNamedParameter4fvNV(disp, parameters) (*((disp)->ProgramNamedParameter4fvNV)) parameters
#define GET_ProgramNamedParameter4fvNV(disp) ((disp)->ProgramNamedParameter4fvNV)
#define SET_ProgramNamedParameter4fvNV(disp, fn) ((disp)->ProgramNamedParameter4fvNV = fn)
#define CALL_DepthBoundsEXT(disp, parameters) (*((disp)->DepthBoundsEXT)) parameters
#define GET_DepthBoundsEXT(disp) ((disp)->DepthBoundsEXT)
#define SET_DepthBoundsEXT(disp, fn) ((disp)->DepthBoundsEXT = fn)
#define CALL_BlendEquationSeparateEXT(disp, parameters) (*((disp)->BlendEquationSeparateEXT)) parameters
#define GET_BlendEquationSeparateEXT(disp) ((disp)->BlendEquationSeparateEXT)
#define SET_BlendEquationSeparateEXT(disp, fn) ((disp)->BlendEquationSeparateEXT = fn)
#define CALL_BindFramebufferEXT(disp, parameters) (*((disp)->BindFramebufferEXT)) parameters
#define GET_BindFramebufferEXT(disp) ((disp)->BindFramebufferEXT)
#define SET_BindFramebufferEXT(disp, fn) ((disp)->BindFramebufferEXT = fn)
#define CALL_BindRenderbufferEXT(disp, parameters) (*((disp)->BindRenderbufferEXT)) parameters
#define GET_BindRenderbufferEXT(disp) ((disp)->BindRenderbufferEXT)
#define SET_BindRenderbufferEXT(disp, fn) ((disp)->BindRenderbufferEXT = fn)
#define CALL_CheckFramebufferStatusEXT(disp, parameters) (*((disp)->CheckFramebufferStatusEXT)) parameters
#define GET_CheckFramebufferStatusEXT(disp) ((disp)->CheckFramebufferStatusEXT)
#define SET_CheckFramebufferStatusEXT(disp, fn) ((disp)->CheckFramebufferStatusEXT = fn)
#define CALL_DeleteFramebuffersEXT(disp, parameters) (*((disp)->DeleteFramebuffersEXT)) parameters
#define GET_DeleteFramebuffersEXT(disp) ((disp)->DeleteFramebuffersEXT)
#define SET_DeleteFramebuffersEXT(disp, fn) ((disp)->DeleteFramebuffersEXT = fn)
#define CALL_DeleteRenderbuffersEXT(disp, parameters) (*((disp)->DeleteRenderbuffersEXT)) parameters
#define GET_DeleteRenderbuffersEXT(disp) ((disp)->DeleteRenderbuffersEXT)
#define SET_DeleteRenderbuffersEXT(disp, fn) ((disp)->DeleteRenderbuffersEXT = fn)
#define CALL_FramebufferRenderbufferEXT(disp, parameters) (*((disp)->FramebufferRenderbufferEXT)) parameters
#define GET_FramebufferRenderbufferEXT(disp) ((disp)->FramebufferRenderbufferEXT)
#define SET_FramebufferRenderbufferEXT(disp, fn) ((disp)->FramebufferRenderbufferEXT = fn)
#define CALL_FramebufferTexture1DEXT(disp, parameters) (*((disp)->FramebufferTexture1DEXT)) parameters
#define GET_FramebufferTexture1DEXT(disp) ((disp)->FramebufferTexture1DEXT)
#define SET_FramebufferTexture1DEXT(disp, fn) ((disp)->FramebufferTexture1DEXT = fn)
#define CALL_FramebufferTexture2DEXT(disp, parameters) (*((disp)->FramebufferTexture2DEXT)) parameters
#define GET_FramebufferTexture2DEXT(disp) ((disp)->FramebufferTexture2DEXT)
#define SET_FramebufferTexture2DEXT(disp, fn) ((disp)->FramebufferTexture2DEXT = fn)
#define CALL_FramebufferTexture3DEXT(disp, parameters) (*((disp)->FramebufferTexture3DEXT)) parameters
#define GET_FramebufferTexture3DEXT(disp) ((disp)->FramebufferTexture3DEXT)
#define SET_FramebufferTexture3DEXT(disp, fn) ((disp)->FramebufferTexture3DEXT = fn)
#define CALL_GenFramebuffersEXT(disp, parameters) (*((disp)->GenFramebuffersEXT)) parameters
#define GET_GenFramebuffersEXT(disp) ((disp)->GenFramebuffersEXT)
#define SET_GenFramebuffersEXT(disp, fn) ((disp)->GenFramebuffersEXT = fn)
#define CALL_GenRenderbuffersEXT(disp, parameters) (*((disp)->GenRenderbuffersEXT)) parameters
#define GET_GenRenderbuffersEXT(disp) ((disp)->GenRenderbuffersEXT)
#define SET_GenRenderbuffersEXT(disp, fn) ((disp)->GenRenderbuffersEXT = fn)
#define CALL_GenerateMipmapEXT(disp, parameters) (*((disp)->GenerateMipmapEXT)) parameters
#define GET_GenerateMipmapEXT(disp) ((disp)->GenerateMipmapEXT)
#define SET_GenerateMipmapEXT(disp, fn) ((disp)->GenerateMipmapEXT = fn)
#define CALL_GetFramebufferAttachmentParameterivEXT(disp, parameters) (*((disp)->GetFramebufferAttachmentParameterivEXT)) parameters
#define GET_GetFramebufferAttachmentParameterivEXT(disp) ((disp)->GetFramebufferAttachmentParameterivEXT)
#define SET_GetFramebufferAttachmentParameterivEXT(disp, fn) ((disp)->GetFramebufferAttachmentParameterivEXT = fn)
#define CALL_GetRenderbufferParameterivEXT(disp, parameters) (*((disp)->GetRenderbufferParameterivEXT)) parameters
#define GET_GetRenderbufferParameterivEXT(disp) ((disp)->GetRenderbufferParameterivEXT)
#define SET_GetRenderbufferParameterivEXT(disp, fn) ((disp)->GetRenderbufferParameterivEXT = fn)
#define CALL_IsFramebufferEXT(disp, parameters) (*((disp)->IsFramebufferEXT)) parameters
#define GET_IsFramebufferEXT(disp) ((disp)->IsFramebufferEXT)
#define SET_IsFramebufferEXT(disp, fn) ((disp)->IsFramebufferEXT = fn)
#define CALL_IsRenderbufferEXT(disp, parameters) (*((disp)->IsRenderbufferEXT)) parameters
#define GET_IsRenderbufferEXT(disp) ((disp)->IsRenderbufferEXT)
#define SET_IsRenderbufferEXT(disp, fn) ((disp)->IsRenderbufferEXT = fn)
#define CALL_RenderbufferStorageEXT(disp, parameters) (*((disp)->RenderbufferStorageEXT)) parameters
#define GET_RenderbufferStorageEXT(disp) ((disp)->RenderbufferStorageEXT)
#define SET_RenderbufferStorageEXT(disp, fn) ((disp)->RenderbufferStorageEXT = fn)
#define CALL_BlitFramebufferEXT(disp, parameters) (*((disp)->BlitFramebufferEXT)) parameters
#define GET_BlitFramebufferEXT(disp) ((disp)->BlitFramebufferEXT)
#define SET_BlitFramebufferEXT(disp, fn) ((disp)->BlitFramebufferEXT = fn)
#define CALL_StencilFuncSeparateATI(disp, parameters) (*((disp)->StencilFuncSeparateATI)) parameters
#define GET_StencilFuncSeparateATI(disp) ((disp)->StencilFuncSeparateATI)
#define SET_StencilFuncSeparateATI(disp, fn) ((disp)->StencilFuncSeparateATI = fn)
#define CALL_ProgramEnvParameters4fvEXT(disp, parameters) (*((disp)->ProgramEnvParameters4fvEXT)) parameters
#define GET_ProgramEnvParameters4fvEXT(disp) ((disp)->ProgramEnvParameters4fvEXT)
#define SET_ProgramEnvParameters4fvEXT(disp, fn) ((disp)->ProgramEnvParameters4fvEXT = fn)
#define CALL_ProgramLocalParameters4fvEXT(disp, parameters) (*((disp)->ProgramLocalParameters4fvEXT)) parameters
#define GET_ProgramLocalParameters4fvEXT(disp) ((disp)->ProgramLocalParameters4fvEXT)
#define SET_ProgramLocalParameters4fvEXT(disp, fn) ((disp)->ProgramLocalParameters4fvEXT = fn)
#define CALL_GetQueryObjecti64vEXT(disp, parameters) (*((disp)->GetQueryObjecti64vEXT)) parameters
#define GET_GetQueryObjecti64vEXT(disp) ((disp)->GetQueryObjecti64vEXT)
#define SET_GetQueryObjecti64vEXT(disp, fn) ((disp)->GetQueryObjecti64vEXT = fn)
#define CALL_GetQueryObjectui64vEXT(disp, parameters) (*((disp)->GetQueryObjectui64vEXT)) parameters
#define GET_GetQueryObjectui64vEXT(disp) ((disp)->GetQueryObjectui64vEXT)
#define SET_GetQueryObjectui64vEXT(disp, fn) ((disp)->GetQueryObjectui64vEXT = fn)

#else

#define driDispatchRemapTable_size 365
extern int driDispatchRemapTable[ driDispatchRemapTable_size ];

#define AttachShader_remap_index 0
#define CreateProgram_remap_index 1
#define CreateShader_remap_index 2
#define DeleteProgram_remap_index 3
#define DeleteShader_remap_index 4
#define DetachShader_remap_index 5
#define GetAttachedShaders_remap_index 6
#define GetProgramInfoLog_remap_index 7
#define GetProgramiv_remap_index 8
#define GetShaderInfoLog_remap_index 9
#define GetShaderiv_remap_index 10
#define IsProgram_remap_index 11
#define IsShader_remap_index 12
#define StencilFuncSeparate_remap_index 13
#define StencilMaskSeparate_remap_index 14
#define StencilOpSeparate_remap_index 15
#define UniformMatrix2x3fv_remap_index 16
#define UniformMatrix2x4fv_remap_index 17
#define UniformMatrix3x2fv_remap_index 18
#define UniformMatrix3x4fv_remap_index 19
#define UniformMatrix4x2fv_remap_index 20
#define UniformMatrix4x3fv_remap_index 21
#define LoadTransposeMatrixdARB_remap_index 22
#define LoadTransposeMatrixfARB_remap_index 23
#define MultTransposeMatrixdARB_remap_index 24
#define MultTransposeMatrixfARB_remap_index 25
#define SampleCoverageARB_remap_index 26
#define CompressedTexImage1DARB_remap_index 27
#define CompressedTexImage2DARB_remap_index 28
#define CompressedTexImage3DARB_remap_index 29
#define CompressedTexSubImage1DARB_remap_index 30
#define CompressedTexSubImage2DARB_remap_index 31
#define CompressedTexSubImage3DARB_remap_index 32
#define GetCompressedTexImageARB_remap_index 33
#define DisableVertexAttribArrayARB_remap_index 34
#define EnableVertexAttribArrayARB_remap_index 35
#define GetProgramEnvParameterdvARB_remap_index 36
#define GetProgramEnvParameterfvARB_remap_index 37
#define GetProgramLocalParameterdvARB_remap_index 38
#define GetProgramLocalParameterfvARB_remap_index 39
#define GetProgramStringARB_remap_index 40
#define GetProgramivARB_remap_index 41
#define GetVertexAttribdvARB_remap_index 42
#define GetVertexAttribfvARB_remap_index 43
#define GetVertexAttribivARB_remap_index 44
#define ProgramEnvParameter4dARB_remap_index 45
#define ProgramEnvParameter4dvARB_remap_index 46
#define ProgramEnvParameter4fARB_remap_index 47
#define ProgramEnvParameter4fvARB_remap_index 48
#define ProgramLocalParameter4dARB_remap_index 49
#define ProgramLocalParameter4dvARB_remap_index 50
#define ProgramLocalParameter4fARB_remap_index 51
#define ProgramLocalParameter4fvARB_remap_index 52
#define ProgramStringARB_remap_index 53
#define VertexAttrib1dARB_remap_index 54
#define VertexAttrib1dvARB_remap_index 55
#define VertexAttrib1fARB_remap_index 56
#define VertexAttrib1fvARB_remap_index 57
#define VertexAttrib1sARB_remap_index 58
#define VertexAttrib1svARB_remap_index 59
#define VertexAttrib2dARB_remap_index 60
#define VertexAttrib2dvARB_remap_index 61
#define VertexAttrib2fARB_remap_index 62
#define VertexAttrib2fvARB_remap_index 63
#define VertexAttrib2sARB_remap_index 64
#define VertexAttrib2svARB_remap_index 65
#define VertexAttrib3dARB_remap_index 66
#define VertexAttrib3dvARB_remap_index 67
#define VertexAttrib3fARB_remap_index 68
#define VertexAttrib3fvARB_remap_index 69
#define VertexAttrib3sARB_remap_index 70
#define VertexAttrib3svARB_remap_index 71
#define VertexAttrib4NbvARB_remap_index 72
#define VertexAttrib4NivARB_remap_index 73
#define VertexAttrib4NsvARB_remap_index 74
#define VertexAttrib4NubARB_remap_index 75
#define VertexAttrib4NubvARB_remap_index 76
#define VertexAttrib4NuivARB_remap_index 77
#define VertexAttrib4NusvARB_remap_index 78
#define VertexAttrib4bvARB_remap_index 79
#define VertexAttrib4dARB_remap_index 80
#define VertexAttrib4dvARB_remap_index 81
#define VertexAttrib4fARB_remap_index 82
#define VertexAttrib4fvARB_remap_index 83
#define VertexAttrib4ivARB_remap_index 84
#define VertexAttrib4sARB_remap_index 85
#define VertexAttrib4svARB_remap_index 86
#define VertexAttrib4ubvARB_remap_index 87
#define VertexAttrib4uivARB_remap_index 88
#define VertexAttrib4usvARB_remap_index 89
#define VertexAttribPointerARB_remap_index 90
#define BindBufferARB_remap_index 91
#define BufferDataARB_remap_index 92
#define BufferSubDataARB_remap_index 93
#define DeleteBuffersARB_remap_index 94
#define GenBuffersARB_remap_index 95
#define GetBufferParameterivARB_remap_index 96
#define GetBufferPointervARB_remap_index 97
#define GetBufferSubDataARB_remap_index 98
#define IsBufferARB_remap_index 99
#define MapBufferARB_remap_index 100
#define UnmapBufferARB_remap_index 101
#define BeginQueryARB_remap_index 102
#define DeleteQueriesARB_remap_index 103
#define EndQueryARB_remap_index 104
#define GenQueriesARB_remap_index 105
#define GetQueryObjectivARB_remap_index 106
#define GetQueryObjectuivARB_remap_index 107
#define GetQueryivARB_remap_index 108
#define IsQueryARB_remap_index 109
#define AttachObjectARB_remap_index 110
#define CompileShaderARB_remap_index 111
#define CreateProgramObjectARB_remap_index 112
#define CreateShaderObjectARB_remap_index 113
#define DeleteObjectARB_remap_index 114
#define DetachObjectARB_remap_index 115
#define GetActiveUniformARB_remap_index 116
#define GetAttachedObjectsARB_remap_index 117
#define GetHandleARB_remap_index 118
#define GetInfoLogARB_remap_index 119
#define GetObjectParameterfvARB_remap_index 120
#define GetObjectParameterivARB_remap_index 121
#define GetShaderSourceARB_remap_index 122
#define GetUniformLocationARB_remap_index 123
#define GetUniformfvARB_remap_index 124
#define GetUniformivARB_remap_index 125
#define LinkProgramARB_remap_index 126
#define ShaderSourceARB_remap_index 127
#define Uniform1fARB_remap_index 128
#define Uniform1fvARB_remap_index 129
#define Uniform1iARB_remap_index 130
#define Uniform1ivARB_remap_index 131
#define Uniform2fARB_remap_index 132
#define Uniform2fvARB_remap_index 133
#define Uniform2iARB_remap_index 134
#define Uniform2ivARB_remap_index 135
#define Uniform3fARB_remap_index 136
#define Uniform3fvARB_remap_index 137
#define Uniform3iARB_remap_index 138
#define Uniform3ivARB_remap_index 139
#define Uniform4fARB_remap_index 140
#define Uniform4fvARB_remap_index 141
#define Uniform4iARB_remap_index 142
#define Uniform4ivARB_remap_index 143
#define UniformMatrix2fvARB_remap_index 144
#define UniformMatrix3fvARB_remap_index 145
#define UniformMatrix4fvARB_remap_index 146
#define UseProgramObjectARB_remap_index 147
#define ValidateProgramARB_remap_index 148
#define BindAttribLocationARB_remap_index 149
#define GetActiveAttribARB_remap_index 150
#define GetAttribLocationARB_remap_index 151
#define DrawBuffersARB_remap_index 152
#define PolygonOffsetEXT_remap_index 153
#define GetPixelTexGenParameterfvSGIS_remap_index 154
#define GetPixelTexGenParameterivSGIS_remap_index 155
#define PixelTexGenParameterfSGIS_remap_index 156
#define PixelTexGenParameterfvSGIS_remap_index 157
#define PixelTexGenParameteriSGIS_remap_index 158
#define PixelTexGenParameterivSGIS_remap_index 159
#define SampleMaskSGIS_remap_index 160
#define SamplePatternSGIS_remap_index 161
#define ColorPointerEXT_remap_index 162
#define EdgeFlagPointerEXT_remap_index 163
#define IndexPointerEXT_remap_index 164
#define NormalPointerEXT_remap_index 165
#define TexCoordPointerEXT_remap_index 166
#define VertexPointerEXT_remap_index 167
#define PointParameterfEXT_remap_index 168
#define PointParameterfvEXT_remap_index 169
#define LockArraysEXT_remap_index 170
#define UnlockArraysEXT_remap_index 171
#define CullParameterdvEXT_remap_index 172
#define CullParameterfvEXT_remap_index 173
#define SecondaryColor3bEXT_remap_index 174
#define SecondaryColor3bvEXT_remap_index 175
#define SecondaryColor3dEXT_remap_index 176
#define SecondaryColor3dvEXT_remap_index 177
#define SecondaryColor3fEXT_remap_index 178
#define SecondaryColor3fvEXT_remap_index 179
#define SecondaryColor3iEXT_remap_index 180
#define SecondaryColor3ivEXT_remap_index 181
#define SecondaryColor3sEXT_remap_index 182
#define SecondaryColor3svEXT_remap_index 183
#define SecondaryColor3ubEXT_remap_index 184
#define SecondaryColor3ubvEXT_remap_index 185
#define SecondaryColor3uiEXT_remap_index 186
#define SecondaryColor3uivEXT_remap_index 187
#define SecondaryColor3usEXT_remap_index 188
#define SecondaryColor3usvEXT_remap_index 189
#define SecondaryColorPointerEXT_remap_index 190
#define MultiDrawArraysEXT_remap_index 191
#define MultiDrawElementsEXT_remap_index 192
#define FogCoordPointerEXT_remap_index 193
#define FogCoorddEXT_remap_index 194
#define FogCoorddvEXT_remap_index 195
#define FogCoordfEXT_remap_index 196
#define FogCoordfvEXT_remap_index 197
#define PixelTexGenSGIX_remap_index 198
#define BlendFuncSeparateEXT_remap_index 199
#define FlushVertexArrayRangeNV_remap_index 200
#define VertexArrayRangeNV_remap_index 201
#define CombinerInputNV_remap_index 202
#define CombinerOutputNV_remap_index 203
#define CombinerParameterfNV_remap_index 204
#define CombinerParameterfvNV_remap_index 205
#define CombinerParameteriNV_remap_index 206
#define CombinerParameterivNV_remap_index 207
#define FinalCombinerInputNV_remap_index 208
#define GetCombinerInputParameterfvNV_remap_index 209
#define GetCombinerInputParameterivNV_remap_index 210
#define GetCombinerOutputParameterfvNV_remap_index 211
#define GetCombinerOutputParameterivNV_remap_index 212
#define GetFinalCombinerInputParameterfvNV_remap_index 213
#define GetFinalCombinerInputParameterivNV_remap_index 214
#define ResizeBuffersMESA_remap_index 215
#define WindowPos2dMESA_remap_index 216
#define WindowPos2dvMESA_remap_index 217
#define WindowPos2fMESA_remap_index 218
#define WindowPos2fvMESA_remap_index 219
#define WindowPos2iMESA_remap_index 220
#define WindowPos2ivMESA_remap_index 221
#define WindowPos2sMESA_remap_index 222
#define WindowPos2svMESA_remap_index 223
#define WindowPos3dMESA_remap_index 224
#define WindowPos3dvMESA_remap_index 225
#define WindowPos3fMESA_remap_index 226
#define WindowPos3fvMESA_remap_index 227
#define WindowPos3iMESA_remap_index 228
#define WindowPos3ivMESA_remap_index 229
#define WindowPos3sMESA_remap_index 230
#define WindowPos3svMESA_remap_index 231
#define WindowPos4dMESA_remap_index 232
#define WindowPos4dvMESA_remap_index 233
#define WindowPos4fMESA_remap_index 234
#define WindowPos4fvMESA_remap_index 235
#define WindowPos4iMESA_remap_index 236
#define WindowPos4ivMESA_remap_index 237
#define WindowPos4sMESA_remap_index 238
#define WindowPos4svMESA_remap_index 239
#define MultiModeDrawArraysIBM_remap_index 240
#define MultiModeDrawElementsIBM_remap_index 241
#define DeleteFencesNV_remap_index 242
#define FinishFenceNV_remap_index 243
#define GenFencesNV_remap_index 244
#define GetFenceivNV_remap_index 245
#define IsFenceNV_remap_index 246
#define SetFenceNV_remap_index 247
#define TestFenceNV_remap_index 248
#define AreProgramsResidentNV_remap_index 249
#define BindProgramNV_remap_index 250
#define DeleteProgramsNV_remap_index 251
#define ExecuteProgramNV_remap_index 252
#define GenProgramsNV_remap_index 253
#define GetProgramParameterdvNV_remap_index 254
#define GetProgramParameterfvNV_remap_index 255
#define GetProgramStringNV_remap_index 256
#define GetProgramivNV_remap_index 257
#define GetTrackMatrixivNV_remap_index 258
#define GetVertexAttribPointervNV_remap_index 259
#define GetVertexAttribdvNV_remap_index 260
#define GetVertexAttribfvNV_remap_index 261
#define GetVertexAttribivNV_remap_index 262
#define IsProgramNV_remap_index 263
#define LoadProgramNV_remap_index 264
#define ProgramParameter4dNV_remap_index 265
#define ProgramParameter4dvNV_remap_index 266
#define ProgramParameter4fNV_remap_index 267
#define ProgramParameter4fvNV_remap_index 268
#define ProgramParameters4dvNV_remap_index 269
#define ProgramParameters4fvNV_remap_index 270
#define RequestResidentProgramsNV_remap_index 271
#define TrackMatrixNV_remap_index 272
#define VertexAttrib1dNV_remap_index 273
#define VertexAttrib1dvNV_remap_index 274
#define VertexAttrib1fNV_remap_index 275
#define VertexAttrib1fvNV_remap_index 276
#define VertexAttrib1sNV_remap_index 277
#define VertexAttrib1svNV_remap_index 278
#define VertexAttrib2dNV_remap_index 279
#define VertexAttrib2dvNV_remap_index 280
#define VertexAttrib2fNV_remap_index 281
#define VertexAttrib2fvNV_remap_index 282
#define VertexAttrib2sNV_remap_index 283
#define VertexAttrib2svNV_remap_index 284
#define VertexAttrib3dNV_remap_index 285
#define VertexAttrib3dvNV_remap_index 286
#define VertexAttrib3fNV_remap_index 287
#define VertexAttrib3fvNV_remap_index 288
#define VertexAttrib3sNV_remap_index 289
#define VertexAttrib3svNV_remap_index 290
#define VertexAttrib4dNV_remap_index 291
#define VertexAttrib4dvNV_remap_index 292
#define VertexAttrib4fNV_remap_index 293
#define VertexAttrib4fvNV_remap_index 294
#define VertexAttrib4sNV_remap_index 295
#define VertexAttrib4svNV_remap_index 296
#define VertexAttrib4ubNV_remap_index 297
#define VertexAttrib4ubvNV_remap_index 298
#define VertexAttribPointerNV_remap_index 299
#define VertexAttribs1dvNV_remap_index 300
#define VertexAttribs1fvNV_remap_index 301
#define VertexAttribs1svNV_remap_index 302
#define VertexAttribs2dvNV_remap_index 303
#define VertexAttribs2fvNV_remap_index 304
#define VertexAttribs2svNV_remap_index 305
#define VertexAttribs3dvNV_remap_index 306
#define VertexAttribs3fvNV_remap_index 307
#define VertexAttribs3svNV_remap_index 308
#define VertexAttribs4dvNV_remap_index 309
#define VertexAttribs4fvNV_remap_index 310
#define VertexAttribs4svNV_remap_index 311
#define VertexAttribs4ubvNV_remap_index 312
#define AlphaFragmentOp1ATI_remap_index 313
#define AlphaFragmentOp2ATI_remap_index 314
#define AlphaFragmentOp3ATI_remap_index 315
#define BeginFragmentShaderATI_remap_index 316
#define BindFragmentShaderATI_remap_index 317
#define ColorFragmentOp1ATI_remap_index 318
#define ColorFragmentOp2ATI_remap_index 319
#define ColorFragmentOp3ATI_remap_index 320
#define DeleteFragmentShaderATI_remap_index 321
#define EndFragmentShaderATI_remap_index 322
#define GenFragmentShadersATI_remap_index 323
#define PassTexCoordATI_remap_index 324
#define SampleMapATI_remap_index 325
#define SetFragmentShaderConstantATI_remap_index 326
#define PointParameteriNV_remap_index 327
#define PointParameterivNV_remap_index 328
#define ActiveStencilFaceEXT_remap_index 329
#define BindVertexArrayAPPLE_remap_index 330
#define DeleteVertexArraysAPPLE_remap_index 331
#define GenVertexArraysAPPLE_remap_index 332
#define IsVertexArrayAPPLE_remap_index 333
#define GetProgramNamedParameterdvNV_remap_index 334
#define GetProgramNamedParameterfvNV_remap_index 335
#define ProgramNamedParameter4dNV_remap_index 336
#define ProgramNamedParameter4dvNV_remap_index 337
#define ProgramNamedParameter4fNV_remap_index 338
#define ProgramNamedParameter4fvNV_remap_index 339
#define DepthBoundsEXT_remap_index 340
#define BlendEquationSeparateEXT_remap_index 341
#define BindFramebufferEXT_remap_index 342
#define BindRenderbufferEXT_remap_index 343
#define CheckFramebufferStatusEXT_remap_index 344
#define DeleteFramebuffersEXT_remap_index 345
#define DeleteRenderbuffersEXT_remap_index 346
#define FramebufferRenderbufferEXT_remap_index 347
#define FramebufferTexture1DEXT_remap_index 348
#define FramebufferTexture2DEXT_remap_index 349
#define FramebufferTexture3DEXT_remap_index 350
#define GenFramebuffersEXT_remap_index 351
#define GenRenderbuffersEXT_remap_index 352
#define GenerateMipmapEXT_remap_index 353
#define GetFramebufferAttachmentParameterivEXT_remap_index 354
#define GetRenderbufferParameterivEXT_remap_index 355
#define IsFramebufferEXT_remap_index 356
#define IsRenderbufferEXT_remap_index 357
#define RenderbufferStorageEXT_remap_index 358
#define BlitFramebufferEXT_remap_index 359
#define StencilFuncSeparateATI_remap_index 360
#define ProgramEnvParameters4fvEXT_remap_index 361
#define ProgramLocalParameters4fvEXT_remap_index 362
#define GetQueryObjecti64vEXT_remap_index 363
#define GetQueryObjectui64vEXT_remap_index 364

#define CALL_AttachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), driDispatchRemapTable[AttachShader_remap_index], parameters)
#define GET_AttachShader(disp) GET_by_offset(disp, driDispatchRemapTable[AttachShader_remap_index])
#define SET_AttachShader(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AttachShader_remap_index], fn)
#define CALL_CreateProgram(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(void)), driDispatchRemapTable[CreateProgram_remap_index], parameters)
#define GET_CreateProgram(disp) GET_by_offset(disp, driDispatchRemapTable[CreateProgram_remap_index])
#define SET_CreateProgram(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CreateProgram_remap_index], fn)
#define CALL_CreateShader(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[CreateShader_remap_index], parameters)
#define GET_CreateShader(disp) GET_by_offset(disp, driDispatchRemapTable[CreateShader_remap_index])
#define SET_CreateShader(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CreateShader_remap_index], fn)
#define CALL_DeleteProgram(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[DeleteProgram_remap_index], parameters)
#define GET_DeleteProgram(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteProgram_remap_index])
#define SET_DeleteProgram(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteProgram_remap_index], fn)
#define CALL_DeleteShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[DeleteShader_remap_index], parameters)
#define GET_DeleteShader(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteShader_remap_index])
#define SET_DeleteShader(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteShader_remap_index], fn)
#define CALL_DetachShader(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint)), driDispatchRemapTable[DetachShader_remap_index], parameters)
#define GET_DetachShader(disp) GET_by_offset(disp, driDispatchRemapTable[DetachShader_remap_index])
#define SET_DetachShader(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DetachShader_remap_index], fn)
#define CALL_GetAttachedShaders(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLuint *)), driDispatchRemapTable[GetAttachedShaders_remap_index], parameters)
#define GET_GetAttachedShaders(disp) GET_by_offset(disp, driDispatchRemapTable[GetAttachedShaders_remap_index])
#define SET_GetAttachedShaders(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetAttachedShaders_remap_index], fn)
#define CALL_GetProgramInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), driDispatchRemapTable[GetProgramInfoLog_remap_index], parameters)
#define GET_GetProgramInfoLog(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramInfoLog_remap_index])
#define SET_GetProgramInfoLog(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramInfoLog_remap_index], fn)
#define CALL_GetProgramiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetProgramiv_remap_index], parameters)
#define GET_GetProgramiv(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramiv_remap_index])
#define SET_GetProgramiv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramiv_remap_index], fn)
#define CALL_GetShaderInfoLog(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, GLsizei *, GLchar *)), driDispatchRemapTable[GetShaderInfoLog_remap_index], parameters)
#define GET_GetShaderInfoLog(disp) GET_by_offset(disp, driDispatchRemapTable[GetShaderInfoLog_remap_index])
#define SET_GetShaderInfoLog(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetShaderInfoLog_remap_index], fn)
#define CALL_GetShaderiv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetShaderiv_remap_index], parameters)
#define GET_GetShaderiv(disp) GET_by_offset(disp, driDispatchRemapTable[GetShaderiv_remap_index])
#define SET_GetShaderiv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetShaderiv_remap_index], fn)
#define CALL_IsProgram(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsProgram_remap_index], parameters)
#define GET_IsProgram(disp) GET_by_offset(disp, driDispatchRemapTable[IsProgram_remap_index])
#define SET_IsProgram(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsProgram_remap_index], fn)
#define CALL_IsShader(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsShader_remap_index], parameters)
#define GET_IsShader(disp) GET_by_offset(disp, driDispatchRemapTable[IsShader_remap_index])
#define SET_IsShader(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsShader_remap_index], fn)
#define CALL_StencilFuncSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), driDispatchRemapTable[StencilFuncSeparate_remap_index], parameters)
#define GET_StencilFuncSeparate(disp) GET_by_offset(disp, driDispatchRemapTable[StencilFuncSeparate_remap_index])
#define SET_StencilFuncSeparate(disp, fn) SET_by_offset(disp, driDispatchRemapTable[StencilFuncSeparate_remap_index], fn)
#define CALL_StencilMaskSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[StencilMaskSeparate_remap_index], parameters)
#define GET_StencilMaskSeparate(disp) GET_by_offset(disp, driDispatchRemapTable[StencilMaskSeparate_remap_index])
#define SET_StencilMaskSeparate(disp, fn) SET_by_offset(disp, driDispatchRemapTable[StencilMaskSeparate_remap_index], fn)
#define CALL_StencilOpSeparate(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), driDispatchRemapTable[StencilOpSeparate_remap_index], parameters)
#define GET_StencilOpSeparate(disp) GET_by_offset(disp, driDispatchRemapTable[StencilOpSeparate_remap_index])
#define SET_StencilOpSeparate(disp, fn) SET_by_offset(disp, driDispatchRemapTable[StencilOpSeparate_remap_index], fn)
#define CALL_UniformMatrix2x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix2x3fv_remap_index], parameters)
#define GET_UniformMatrix2x3fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix2x3fv_remap_index])
#define SET_UniformMatrix2x3fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix2x3fv_remap_index], fn)
#define CALL_UniformMatrix2x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix2x4fv_remap_index], parameters)
#define GET_UniformMatrix2x4fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix2x4fv_remap_index])
#define SET_UniformMatrix2x4fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix2x4fv_remap_index], fn)
#define CALL_UniformMatrix3x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix3x2fv_remap_index], parameters)
#define GET_UniformMatrix3x2fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix3x2fv_remap_index])
#define SET_UniformMatrix3x2fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix3x2fv_remap_index], fn)
#define CALL_UniformMatrix3x4fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix3x4fv_remap_index], parameters)
#define GET_UniformMatrix3x4fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix3x4fv_remap_index])
#define SET_UniformMatrix3x4fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix3x4fv_remap_index], fn)
#define CALL_UniformMatrix4x2fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix4x2fv_remap_index], parameters)
#define GET_UniformMatrix4x2fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix4x2fv_remap_index])
#define SET_UniformMatrix4x2fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix4x2fv_remap_index], fn)
#define CALL_UniformMatrix4x3fv(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix4x3fv_remap_index], parameters)
#define GET_UniformMatrix4x3fv(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix4x3fv_remap_index])
#define SET_UniformMatrix4x3fv(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix4x3fv_remap_index], fn)
#define CALL_LoadTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[LoadTransposeMatrixdARB_remap_index], parameters)
#define GET_LoadTransposeMatrixdARB(disp) GET_by_offset(disp, driDispatchRemapTable[LoadTransposeMatrixdARB_remap_index])
#define SET_LoadTransposeMatrixdARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[LoadTransposeMatrixdARB_remap_index], fn)
#define CALL_LoadTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[LoadTransposeMatrixfARB_remap_index], parameters)
#define GET_LoadTransposeMatrixfARB(disp) GET_by_offset(disp, driDispatchRemapTable[LoadTransposeMatrixfARB_remap_index])
#define SET_LoadTransposeMatrixfARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[LoadTransposeMatrixfARB_remap_index], fn)
#define CALL_MultTransposeMatrixdARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[MultTransposeMatrixdARB_remap_index], parameters)
#define GET_MultTransposeMatrixdARB(disp) GET_by_offset(disp, driDispatchRemapTable[MultTransposeMatrixdARB_remap_index])
#define SET_MultTransposeMatrixdARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultTransposeMatrixdARB_remap_index], fn)
#define CALL_MultTransposeMatrixfARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[MultTransposeMatrixfARB_remap_index], parameters)
#define GET_MultTransposeMatrixfARB(disp) GET_by_offset(disp, driDispatchRemapTable[MultTransposeMatrixfARB_remap_index])
#define SET_MultTransposeMatrixfARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultTransposeMatrixfARB_remap_index], fn)
#define CALL_SampleCoverageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), driDispatchRemapTable[SampleCoverageARB_remap_index], parameters)
#define GET_SampleCoverageARB(disp) GET_by_offset(disp, driDispatchRemapTable[SampleCoverageARB_remap_index])
#define SET_SampleCoverageARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SampleCoverageARB_remap_index], fn)
#define CALL_CompressedTexImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLint, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexImage1DARB_remap_index], parameters)
#define GET_CompressedTexImage1DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexImage1DARB_remap_index])
#define SET_CompressedTexImage1DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexImage1DARB_remap_index], fn)
#define CALL_CompressedTexImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexImage2DARB_remap_index], parameters)
#define GET_CompressedTexImage2DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexImage2DARB_remap_index])
#define SET_CompressedTexImage2DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexImage2DARB_remap_index], fn)
#define CALL_CompressedTexImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexImage3DARB_remap_index], parameters)
#define GET_CompressedTexImage3DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexImage3DARB_remap_index])
#define SET_CompressedTexImage3DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexImage3DARB_remap_index], fn)
#define CALL_CompressedTexSubImage1DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLsizei, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexSubImage1DARB_remap_index], parameters)
#define GET_CompressedTexSubImage1DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage1DARB_remap_index])
#define SET_CompressedTexSubImage1DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage1DARB_remap_index], fn)
#define CALL_CompressedTexSubImage2DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexSubImage2DARB_remap_index], parameters)
#define GET_CompressedTexSubImage2DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage2DARB_remap_index])
#define SET_CompressedTexSubImage2DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage2DARB_remap_index], fn)
#define CALL_CompressedTexSubImage3DARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[CompressedTexSubImage3DARB_remap_index], parameters)
#define GET_CompressedTexSubImage3DARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage3DARB_remap_index])
#define SET_CompressedTexSubImage3DARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompressedTexSubImage3DARB_remap_index], fn)
#define CALL_GetCompressedTexImageARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint, GLvoid *)), driDispatchRemapTable[GetCompressedTexImageARB_remap_index], parameters)
#define GET_GetCompressedTexImageARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetCompressedTexImageARB_remap_index])
#define SET_GetCompressedTexImageARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetCompressedTexImageARB_remap_index], fn)
#define CALL_DisableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[DisableVertexAttribArrayARB_remap_index], parameters)
#define GET_DisableVertexAttribArrayARB(disp) GET_by_offset(disp, driDispatchRemapTable[DisableVertexAttribArrayARB_remap_index])
#define SET_DisableVertexAttribArrayARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DisableVertexAttribArrayARB_remap_index], fn)
#define CALL_EnableVertexAttribArrayARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[EnableVertexAttribArrayARB_remap_index], parameters)
#define GET_EnableVertexAttribArrayARB(disp) GET_by_offset(disp, driDispatchRemapTable[EnableVertexAttribArrayARB_remap_index])
#define SET_EnableVertexAttribArrayARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[EnableVertexAttribArrayARB_remap_index], fn)
#define CALL_GetProgramEnvParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), driDispatchRemapTable[GetProgramEnvParameterdvARB_remap_index], parameters)
#define GET_GetProgramEnvParameterdvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramEnvParameterdvARB_remap_index])
#define SET_GetProgramEnvParameterdvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramEnvParameterdvARB_remap_index], fn)
#define CALL_GetProgramEnvParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), driDispatchRemapTable[GetProgramEnvParameterfvARB_remap_index], parameters)
#define GET_GetProgramEnvParameterfvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramEnvParameterfvARB_remap_index])
#define SET_GetProgramEnvParameterfvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramEnvParameterfvARB_remap_index], fn)
#define CALL_GetProgramLocalParameterdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble *)), driDispatchRemapTable[GetProgramLocalParameterdvARB_remap_index], parameters)
#define GET_GetProgramLocalParameterdvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramLocalParameterdvARB_remap_index])
#define SET_GetProgramLocalParameterdvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramLocalParameterdvARB_remap_index], fn)
#define CALL_GetProgramLocalParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat *)), driDispatchRemapTable[GetProgramLocalParameterfvARB_remap_index], parameters)
#define GET_GetProgramLocalParameterfvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramLocalParameterfvARB_remap_index])
#define SET_GetProgramLocalParameterfvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramLocalParameterfvARB_remap_index], fn)
#define CALL_GetProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid *)), driDispatchRemapTable[GetProgramStringARB_remap_index], parameters)
#define GET_GetProgramStringARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramStringARB_remap_index])
#define SET_GetProgramStringARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramStringARB_remap_index], fn)
#define CALL_GetProgramivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), driDispatchRemapTable[GetProgramivARB_remap_index], parameters)
#define GET_GetProgramivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramivARB_remap_index])
#define SET_GetProgramivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramivARB_remap_index], fn)
#define CALL_GetVertexAttribdvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), driDispatchRemapTable[GetVertexAttribdvARB_remap_index], parameters)
#define GET_GetVertexAttribdvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribdvARB_remap_index])
#define SET_GetVertexAttribdvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribdvARB_remap_index], fn)
#define CALL_GetVertexAttribfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), driDispatchRemapTable[GetVertexAttribfvARB_remap_index], parameters)
#define GET_GetVertexAttribfvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribfvARB_remap_index])
#define SET_GetVertexAttribfvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribfvARB_remap_index], fn)
#define CALL_GetVertexAttribivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetVertexAttribivARB_remap_index], parameters)
#define GET_GetVertexAttribivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribivARB_remap_index])
#define SET_GetVertexAttribivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribivARB_remap_index], fn)
#define CALL_ProgramEnvParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[ProgramEnvParameter4dARB_remap_index], parameters)
#define GET_ProgramEnvParameter4dARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4dARB_remap_index])
#define SET_ProgramEnvParameter4dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4dARB_remap_index], fn)
#define CALL_ProgramEnvParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), driDispatchRemapTable[ProgramEnvParameter4dvARB_remap_index], parameters)
#define GET_ProgramEnvParameter4dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4dvARB_remap_index])
#define SET_ProgramEnvParameter4dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4dvARB_remap_index], fn)
#define CALL_ProgramEnvParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[ProgramEnvParameter4fARB_remap_index], parameters)
#define GET_ProgramEnvParameter4fARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4fARB_remap_index])
#define SET_ProgramEnvParameter4fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4fARB_remap_index], fn)
#define CALL_ProgramEnvParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), driDispatchRemapTable[ProgramEnvParameter4fvARB_remap_index], parameters)
#define GET_ProgramEnvParameter4fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4fvARB_remap_index])
#define SET_ProgramEnvParameter4fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameter4fvARB_remap_index], fn)
#define CALL_ProgramLocalParameter4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[ProgramLocalParameter4dARB_remap_index], parameters)
#define GET_ProgramLocalParameter4dARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4dARB_remap_index])
#define SET_ProgramLocalParameter4dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4dARB_remap_index], fn)
#define CALL_ProgramLocalParameter4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), driDispatchRemapTable[ProgramLocalParameter4dvARB_remap_index], parameters)
#define GET_ProgramLocalParameter4dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4dvARB_remap_index])
#define SET_ProgramLocalParameter4dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4dvARB_remap_index], fn)
#define CALL_ProgramLocalParameter4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[ProgramLocalParameter4fARB_remap_index], parameters)
#define GET_ProgramLocalParameter4fARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4fARB_remap_index])
#define SET_ProgramLocalParameter4fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4fARB_remap_index], fn)
#define CALL_ProgramLocalParameter4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), driDispatchRemapTable[ProgramLocalParameter4fvARB_remap_index], parameters)
#define GET_ProgramLocalParameter4fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4fvARB_remap_index])
#define SET_ProgramLocalParameter4fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameter4fvARB_remap_index], fn)
#define CALL_ProgramStringARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[ProgramStringARB_remap_index], parameters)
#define GET_ProgramStringARB(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramStringARB_remap_index])
#define SET_ProgramStringARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramStringARB_remap_index], fn)
#define CALL_VertexAttrib1dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), driDispatchRemapTable[VertexAttrib1dARB_remap_index], parameters)
#define GET_VertexAttrib1dARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dARB_remap_index])
#define SET_VertexAttrib1dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dARB_remap_index], fn)
#define CALL_VertexAttrib1dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib1dvARB_remap_index], parameters)
#define GET_VertexAttrib1dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dvARB_remap_index])
#define SET_VertexAttrib1dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dvARB_remap_index], fn)
#define CALL_VertexAttrib1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), driDispatchRemapTable[VertexAttrib1fARB_remap_index], parameters)
#define GET_VertexAttrib1fARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fARB_remap_index])
#define SET_VertexAttrib1fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fARB_remap_index], fn)
#define CALL_VertexAttrib1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib1fvARB_remap_index], parameters)
#define GET_VertexAttrib1fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fvARB_remap_index])
#define SET_VertexAttrib1fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fvARB_remap_index], fn)
#define CALL_VertexAttrib1sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), driDispatchRemapTable[VertexAttrib1sARB_remap_index], parameters)
#define GET_VertexAttrib1sARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1sARB_remap_index])
#define SET_VertexAttrib1sARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1sARB_remap_index], fn)
#define CALL_VertexAttrib1svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib1svARB_remap_index], parameters)
#define GET_VertexAttrib1svARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1svARB_remap_index])
#define SET_VertexAttrib1svARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1svARB_remap_index], fn)
#define CALL_VertexAttrib2dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib2dARB_remap_index], parameters)
#define GET_VertexAttrib2dARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dARB_remap_index])
#define SET_VertexAttrib2dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dARB_remap_index], fn)
#define CALL_VertexAttrib2dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib2dvARB_remap_index], parameters)
#define GET_VertexAttrib2dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dvARB_remap_index])
#define SET_VertexAttrib2dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dvARB_remap_index], fn)
#define CALL_VertexAttrib2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib2fARB_remap_index], parameters)
#define GET_VertexAttrib2fARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fARB_remap_index])
#define SET_VertexAttrib2fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fARB_remap_index], fn)
#define CALL_VertexAttrib2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib2fvARB_remap_index], parameters)
#define GET_VertexAttrib2fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fvARB_remap_index])
#define SET_VertexAttrib2fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fvARB_remap_index], fn)
#define CALL_VertexAttrib2sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib2sARB_remap_index], parameters)
#define GET_VertexAttrib2sARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2sARB_remap_index])
#define SET_VertexAttrib2sARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2sARB_remap_index], fn)
#define CALL_VertexAttrib2svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib2svARB_remap_index], parameters)
#define GET_VertexAttrib2svARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2svARB_remap_index])
#define SET_VertexAttrib2svARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2svARB_remap_index], fn)
#define CALL_VertexAttrib3dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib3dARB_remap_index], parameters)
#define GET_VertexAttrib3dARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dARB_remap_index])
#define SET_VertexAttrib3dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dARB_remap_index], fn)
#define CALL_VertexAttrib3dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib3dvARB_remap_index], parameters)
#define GET_VertexAttrib3dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dvARB_remap_index])
#define SET_VertexAttrib3dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dvARB_remap_index], fn)
#define CALL_VertexAttrib3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib3fARB_remap_index], parameters)
#define GET_VertexAttrib3fARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fARB_remap_index])
#define SET_VertexAttrib3fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fARB_remap_index], fn)
#define CALL_VertexAttrib3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib3fvARB_remap_index], parameters)
#define GET_VertexAttrib3fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fvARB_remap_index])
#define SET_VertexAttrib3fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fvARB_remap_index], fn)
#define CALL_VertexAttrib3sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib3sARB_remap_index], parameters)
#define GET_VertexAttrib3sARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3sARB_remap_index])
#define SET_VertexAttrib3sARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3sARB_remap_index], fn)
#define CALL_VertexAttrib3svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib3svARB_remap_index], parameters)
#define GET_VertexAttrib3svARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3svARB_remap_index])
#define SET_VertexAttrib3svARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3svARB_remap_index], fn)
#define CALL_VertexAttrib4NbvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), driDispatchRemapTable[VertexAttrib4NbvARB_remap_index], parameters)
#define GET_VertexAttrib4NbvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NbvARB_remap_index])
#define SET_VertexAttrib4NbvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NbvARB_remap_index], fn)
#define CALL_VertexAttrib4NivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), driDispatchRemapTable[VertexAttrib4NivARB_remap_index], parameters)
#define GET_VertexAttrib4NivARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NivARB_remap_index])
#define SET_VertexAttrib4NivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NivARB_remap_index], fn)
#define CALL_VertexAttrib4NsvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib4NsvARB_remap_index], parameters)
#define GET_VertexAttrib4NsvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NsvARB_remap_index])
#define SET_VertexAttrib4NsvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NsvARB_remap_index], fn)
#define CALL_VertexAttrib4NubARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), driDispatchRemapTable[VertexAttrib4NubARB_remap_index], parameters)
#define GET_VertexAttrib4NubARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NubARB_remap_index])
#define SET_VertexAttrib4NubARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NubARB_remap_index], fn)
#define CALL_VertexAttrib4NubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), driDispatchRemapTable[VertexAttrib4NubvARB_remap_index], parameters)
#define GET_VertexAttrib4NubvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NubvARB_remap_index])
#define SET_VertexAttrib4NubvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NubvARB_remap_index], fn)
#define CALL_VertexAttrib4NuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), driDispatchRemapTable[VertexAttrib4NuivARB_remap_index], parameters)
#define GET_VertexAttrib4NuivARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NuivARB_remap_index])
#define SET_VertexAttrib4NuivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NuivARB_remap_index], fn)
#define CALL_VertexAttrib4NusvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), driDispatchRemapTable[VertexAttrib4NusvARB_remap_index], parameters)
#define GET_VertexAttrib4NusvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NusvARB_remap_index])
#define SET_VertexAttrib4NusvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4NusvARB_remap_index], fn)
#define CALL_VertexAttrib4bvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLbyte *)), driDispatchRemapTable[VertexAttrib4bvARB_remap_index], parameters)
#define GET_VertexAttrib4bvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4bvARB_remap_index])
#define SET_VertexAttrib4bvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4bvARB_remap_index], fn)
#define CALL_VertexAttrib4dARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib4dARB_remap_index], parameters)
#define GET_VertexAttrib4dARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dARB_remap_index])
#define SET_VertexAttrib4dARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dARB_remap_index], fn)
#define CALL_VertexAttrib4dvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib4dvARB_remap_index], parameters)
#define GET_VertexAttrib4dvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dvARB_remap_index])
#define SET_VertexAttrib4dvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dvARB_remap_index], fn)
#define CALL_VertexAttrib4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib4fARB_remap_index], parameters)
#define GET_VertexAttrib4fARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fARB_remap_index])
#define SET_VertexAttrib4fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fARB_remap_index], fn)
#define CALL_VertexAttrib4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib4fvARB_remap_index], parameters)
#define GET_VertexAttrib4fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fvARB_remap_index])
#define SET_VertexAttrib4fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fvARB_remap_index], fn)
#define CALL_VertexAttrib4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLint *)), driDispatchRemapTable[VertexAttrib4ivARB_remap_index], parameters)
#define GET_VertexAttrib4ivARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ivARB_remap_index])
#define SET_VertexAttrib4ivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ivARB_remap_index], fn)
#define CALL_VertexAttrib4sARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib4sARB_remap_index], parameters)
#define GET_VertexAttrib4sARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4sARB_remap_index])
#define SET_VertexAttrib4sARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4sARB_remap_index], fn)
#define CALL_VertexAttrib4svARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib4svARB_remap_index], parameters)
#define GET_VertexAttrib4svARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4svARB_remap_index])
#define SET_VertexAttrib4svARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4svARB_remap_index], fn)
#define CALL_VertexAttrib4ubvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), driDispatchRemapTable[VertexAttrib4ubvARB_remap_index], parameters)
#define GET_VertexAttrib4ubvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubvARB_remap_index])
#define SET_VertexAttrib4ubvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubvARB_remap_index], fn)
#define CALL_VertexAttrib4uivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLuint *)), driDispatchRemapTable[VertexAttrib4uivARB_remap_index], parameters)
#define GET_VertexAttrib4uivARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4uivARB_remap_index])
#define SET_VertexAttrib4uivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4uivARB_remap_index], fn)
#define CALL_VertexAttrib4usvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLushort *)), driDispatchRemapTable[VertexAttrib4usvARB_remap_index], parameters)
#define GET_VertexAttrib4usvARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4usvARB_remap_index])
#define SET_VertexAttrib4usvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4usvARB_remap_index], fn)
#define CALL_VertexAttribPointerARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)), driDispatchRemapTable[VertexAttribPointerARB_remap_index], parameters)
#define GET_VertexAttribPointerARB(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribPointerARB_remap_index])
#define SET_VertexAttribPointerARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribPointerARB_remap_index], fn)
#define CALL_BindBufferARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[BindBufferARB_remap_index], parameters)
#define GET_BindBufferARB(disp) GET_by_offset(disp, driDispatchRemapTable[BindBufferARB_remap_index])
#define SET_BindBufferARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindBufferARB_remap_index], fn)
#define CALL_BufferDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizeiptrARB, const GLvoid *, GLenum)), driDispatchRemapTable[BufferDataARB_remap_index], parameters)
#define GET_BufferDataARB(disp) GET_by_offset(disp, driDispatchRemapTable[BufferDataARB_remap_index])
#define SET_BufferDataARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BufferDataARB_remap_index], fn)
#define CALL_BufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *)), driDispatchRemapTable[BufferSubDataARB_remap_index], parameters)
#define GET_BufferSubDataARB(disp) GET_by_offset(disp, driDispatchRemapTable[BufferSubDataARB_remap_index])
#define SET_BufferSubDataARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BufferSubDataARB_remap_index], fn)
#define CALL_DeleteBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteBuffersARB_remap_index], parameters)
#define GET_DeleteBuffersARB(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteBuffersARB_remap_index])
#define SET_DeleteBuffersARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteBuffersARB_remap_index], fn)
#define CALL_GenBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenBuffersARB_remap_index], parameters)
#define GET_GenBuffersARB(disp) GET_by_offset(disp, driDispatchRemapTable[GenBuffersARB_remap_index])
#define SET_GenBuffersARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenBuffersARB_remap_index], fn)
#define CALL_GetBufferParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), driDispatchRemapTable[GetBufferParameterivARB_remap_index], parameters)
#define GET_GetBufferParameterivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetBufferParameterivARB_remap_index])
#define SET_GetBufferParameterivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetBufferParameterivARB_remap_index], fn)
#define CALL_GetBufferPointervARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLvoid **)), driDispatchRemapTable[GetBufferPointervARB_remap_index], parameters)
#define GET_GetBufferPointervARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetBufferPointervARB_remap_index])
#define SET_GetBufferPointervARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetBufferPointervARB_remap_index], fn)
#define CALL_GetBufferSubDataARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *)), driDispatchRemapTable[GetBufferSubDataARB_remap_index], parameters)
#define GET_GetBufferSubDataARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetBufferSubDataARB_remap_index])
#define SET_GetBufferSubDataARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetBufferSubDataARB_remap_index], fn)
#define CALL_IsBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsBufferARB_remap_index], parameters)
#define GET_IsBufferARB(disp) GET_by_offset(disp, driDispatchRemapTable[IsBufferARB_remap_index])
#define SET_IsBufferARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsBufferARB_remap_index], fn)
#define CALL_MapBufferARB(disp, parameters) CALL_by_offset(disp, (GLvoid * (GLAPIENTRYP)(GLenum, GLenum)), driDispatchRemapTable[MapBufferARB_remap_index], parameters)
#define GET_MapBufferARB(disp) GET_by_offset(disp, driDispatchRemapTable[MapBufferARB_remap_index])
#define SET_MapBufferARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MapBufferARB_remap_index], fn)
#define CALL_UnmapBufferARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[UnmapBufferARB_remap_index], parameters)
#define GET_UnmapBufferARB(disp) GET_by_offset(disp, driDispatchRemapTable[UnmapBufferARB_remap_index])
#define SET_UnmapBufferARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UnmapBufferARB_remap_index], fn)
#define CALL_BeginQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[BeginQueryARB_remap_index], parameters)
#define GET_BeginQueryARB(disp) GET_by_offset(disp, driDispatchRemapTable[BeginQueryARB_remap_index])
#define SET_BeginQueryARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BeginQueryARB_remap_index], fn)
#define CALL_DeleteQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteQueriesARB_remap_index], parameters)
#define GET_DeleteQueriesARB(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteQueriesARB_remap_index])
#define SET_DeleteQueriesARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteQueriesARB_remap_index], fn)
#define CALL_EndQueryARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[EndQueryARB_remap_index], parameters)
#define GET_EndQueryARB(disp) GET_by_offset(disp, driDispatchRemapTable[EndQueryARB_remap_index])
#define SET_EndQueryARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[EndQueryARB_remap_index], fn)
#define CALL_GenQueriesARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenQueriesARB_remap_index], parameters)
#define GET_GenQueriesARB(disp) GET_by_offset(disp, driDispatchRemapTable[GenQueriesARB_remap_index])
#define SET_GenQueriesARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenQueriesARB_remap_index], fn)
#define CALL_GetQueryObjectivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetQueryObjectivARB_remap_index], parameters)
#define GET_GetQueryObjectivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetQueryObjectivARB_remap_index])
#define SET_GetQueryObjectivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetQueryObjectivARB_remap_index], fn)
#define CALL_GetQueryObjectuivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint *)), driDispatchRemapTable[GetQueryObjectuivARB_remap_index], parameters)
#define GET_GetQueryObjectuivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetQueryObjectuivARB_remap_index])
#define SET_GetQueryObjectuivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetQueryObjectuivARB_remap_index], fn)
#define CALL_GetQueryivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), driDispatchRemapTable[GetQueryivARB_remap_index], parameters)
#define GET_GetQueryivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetQueryivARB_remap_index])
#define SET_GetQueryivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetQueryivARB_remap_index], fn)
#define CALL_IsQueryARB(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsQueryARB_remap_index], parameters)
#define GET_IsQueryARB(disp) GET_by_offset(disp, driDispatchRemapTable[IsQueryARB_remap_index])
#define SET_IsQueryARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsQueryARB_remap_index], fn)
#define CALL_AttachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), driDispatchRemapTable[AttachObjectARB_remap_index], parameters)
#define GET_AttachObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[AttachObjectARB_remap_index])
#define SET_AttachObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AttachObjectARB_remap_index], fn)
#define CALL_CompileShaderARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), driDispatchRemapTable[CompileShaderARB_remap_index], parameters)
#define GET_CompileShaderARB(disp) GET_by_offset(disp, driDispatchRemapTable[CompileShaderARB_remap_index])
#define SET_CompileShaderARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CompileShaderARB_remap_index], fn)
#define CALL_CreateProgramObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(void)), driDispatchRemapTable[CreateProgramObjectARB_remap_index], parameters)
#define GET_CreateProgramObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[CreateProgramObjectARB_remap_index])
#define SET_CreateProgramObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CreateProgramObjectARB_remap_index], fn)
#define CALL_CreateShaderObjectARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[CreateShaderObjectARB_remap_index], parameters)
#define GET_CreateShaderObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[CreateShaderObjectARB_remap_index])
#define SET_CreateShaderObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CreateShaderObjectARB_remap_index], fn)
#define CALL_DeleteObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), driDispatchRemapTable[DeleteObjectARB_remap_index], parameters)
#define GET_DeleteObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteObjectARB_remap_index])
#define SET_DeleteObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteObjectARB_remap_index], fn)
#define CALL_DetachObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLhandleARB)), driDispatchRemapTable[DetachObjectARB_remap_index], parameters)
#define GET_DetachObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[DetachObjectARB_remap_index])
#define SET_DetachObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DetachObjectARB_remap_index], fn)
#define CALL_GetActiveUniformARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), driDispatchRemapTable[GetActiveUniformARB_remap_index], parameters)
#define GET_GetActiveUniformARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetActiveUniformARB_remap_index])
#define SET_GetActiveUniformARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetActiveUniformARB_remap_index], fn)
#define CALL_GetAttachedObjectsARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)), driDispatchRemapTable[GetAttachedObjectsARB_remap_index], parameters)
#define GET_GetAttachedObjectsARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetAttachedObjectsARB_remap_index])
#define SET_GetAttachedObjectsARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetAttachedObjectsARB_remap_index], fn)
#define CALL_GetHandleARB(disp, parameters) CALL_by_offset(disp, (GLhandleARB (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[GetHandleARB_remap_index], parameters)
#define GET_GetHandleARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetHandleARB_remap_index])
#define SET_GetHandleARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetHandleARB_remap_index], fn)
#define CALL_GetInfoLogARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), driDispatchRemapTable[GetInfoLogARB_remap_index], parameters)
#define GET_GetInfoLogARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetInfoLogARB_remap_index])
#define SET_GetInfoLogARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetInfoLogARB_remap_index], fn)
#define CALL_GetObjectParameterfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLfloat *)), driDispatchRemapTable[GetObjectParameterfvARB_remap_index], parameters)
#define GET_GetObjectParameterfvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetObjectParameterfvARB_remap_index])
#define SET_GetObjectParameterfvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetObjectParameterfvARB_remap_index], fn)
#define CALL_GetObjectParameterivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLenum, GLint *)), driDispatchRemapTable[GetObjectParameterivARB_remap_index], parameters)
#define GET_GetObjectParameterivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetObjectParameterivARB_remap_index])
#define SET_GetObjectParameterivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetObjectParameterivARB_remap_index], fn)
#define CALL_GetShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)), driDispatchRemapTable[GetShaderSourceARB_remap_index], parameters)
#define GET_GetShaderSourceARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetShaderSourceARB_remap_index])
#define SET_GetShaderSourceARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetShaderSourceARB_remap_index], fn)
#define CALL_GetUniformLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), driDispatchRemapTable[GetUniformLocationARB_remap_index], parameters)
#define GET_GetUniformLocationARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetUniformLocationARB_remap_index])
#define SET_GetUniformLocationARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetUniformLocationARB_remap_index], fn)
#define CALL_GetUniformfvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLfloat *)), driDispatchRemapTable[GetUniformfvARB_remap_index], parameters)
#define GET_GetUniformfvARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetUniformfvARB_remap_index])
#define SET_GetUniformfvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetUniformfvARB_remap_index], fn)
#define CALL_GetUniformivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLint, GLint *)), driDispatchRemapTable[GetUniformivARB_remap_index], parameters)
#define GET_GetUniformivARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetUniformivARB_remap_index])
#define SET_GetUniformivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetUniformivARB_remap_index], fn)
#define CALL_LinkProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), driDispatchRemapTable[LinkProgramARB_remap_index], parameters)
#define GET_LinkProgramARB(disp) GET_by_offset(disp, driDispatchRemapTable[LinkProgramARB_remap_index])
#define SET_LinkProgramARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[LinkProgramARB_remap_index], fn)
#define CALL_ShaderSourceARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLsizei, const GLcharARB **, const GLint *)), driDispatchRemapTable[ShaderSourceARB_remap_index], parameters)
#define GET_ShaderSourceARB(disp) GET_by_offset(disp, driDispatchRemapTable[ShaderSourceARB_remap_index])
#define SET_ShaderSourceARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ShaderSourceARB_remap_index], fn)
#define CALL_Uniform1fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat)), driDispatchRemapTable[Uniform1fARB_remap_index], parameters)
#define GET_Uniform1fARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform1fARB_remap_index])
#define SET_Uniform1fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform1fARB_remap_index], fn)
#define CALL_Uniform1fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), driDispatchRemapTable[Uniform1fvARB_remap_index], parameters)
#define GET_Uniform1fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform1fvARB_remap_index])
#define SET_Uniform1fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform1fvARB_remap_index], fn)
#define CALL_Uniform1iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), driDispatchRemapTable[Uniform1iARB_remap_index], parameters)
#define GET_Uniform1iARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform1iARB_remap_index])
#define SET_Uniform1iARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform1iARB_remap_index], fn)
#define CALL_Uniform1ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), driDispatchRemapTable[Uniform1ivARB_remap_index], parameters)
#define GET_Uniform1ivARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform1ivARB_remap_index])
#define SET_Uniform1ivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform1ivARB_remap_index], fn)
#define CALL_Uniform2fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat)), driDispatchRemapTable[Uniform2fARB_remap_index], parameters)
#define GET_Uniform2fARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform2fARB_remap_index])
#define SET_Uniform2fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform2fARB_remap_index], fn)
#define CALL_Uniform2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), driDispatchRemapTable[Uniform2fvARB_remap_index], parameters)
#define GET_Uniform2fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform2fvARB_remap_index])
#define SET_Uniform2fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform2fvARB_remap_index], fn)
#define CALL_Uniform2iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), driDispatchRemapTable[Uniform2iARB_remap_index], parameters)
#define GET_Uniform2iARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform2iARB_remap_index])
#define SET_Uniform2iARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform2iARB_remap_index], fn)
#define CALL_Uniform2ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), driDispatchRemapTable[Uniform2ivARB_remap_index], parameters)
#define GET_Uniform2ivARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform2ivARB_remap_index])
#define SET_Uniform2ivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform2ivARB_remap_index], fn)
#define CALL_Uniform3fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[Uniform3fARB_remap_index], parameters)
#define GET_Uniform3fARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform3fARB_remap_index])
#define SET_Uniform3fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform3fARB_remap_index], fn)
#define CALL_Uniform3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), driDispatchRemapTable[Uniform3fvARB_remap_index], parameters)
#define GET_Uniform3fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform3fvARB_remap_index])
#define SET_Uniform3fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform3fvARB_remap_index], fn)
#define CALL_Uniform3iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), driDispatchRemapTable[Uniform3iARB_remap_index], parameters)
#define GET_Uniform3iARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform3iARB_remap_index])
#define SET_Uniform3iARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform3iARB_remap_index], fn)
#define CALL_Uniform3ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), driDispatchRemapTable[Uniform3ivARB_remap_index], parameters)
#define GET_Uniform3ivARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform3ivARB_remap_index])
#define SET_Uniform3ivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform3ivARB_remap_index], fn)
#define CALL_Uniform4fARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[Uniform4fARB_remap_index], parameters)
#define GET_Uniform4fARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform4fARB_remap_index])
#define SET_Uniform4fARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform4fARB_remap_index], fn)
#define CALL_Uniform4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLfloat *)), driDispatchRemapTable[Uniform4fvARB_remap_index], parameters)
#define GET_Uniform4fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform4fvARB_remap_index])
#define SET_Uniform4fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform4fvARB_remap_index], fn)
#define CALL_Uniform4iARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint)), driDispatchRemapTable[Uniform4iARB_remap_index], parameters)
#define GET_Uniform4iARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform4iARB_remap_index])
#define SET_Uniform4iARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform4iARB_remap_index], fn)
#define CALL_Uniform4ivARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, const GLint *)), driDispatchRemapTable[Uniform4ivARB_remap_index], parameters)
#define GET_Uniform4ivARB(disp) GET_by_offset(disp, driDispatchRemapTable[Uniform4ivARB_remap_index])
#define SET_Uniform4ivARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[Uniform4ivARB_remap_index], fn)
#define CALL_UniformMatrix2fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix2fvARB_remap_index], parameters)
#define GET_UniformMatrix2fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix2fvARB_remap_index])
#define SET_UniformMatrix2fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix2fvARB_remap_index], fn)
#define CALL_UniformMatrix3fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix3fvARB_remap_index], parameters)
#define GET_UniformMatrix3fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix3fvARB_remap_index])
#define SET_UniformMatrix3fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix3fvARB_remap_index], fn)
#define CALL_UniformMatrix4fvARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)), driDispatchRemapTable[UniformMatrix4fvARB_remap_index], parameters)
#define GET_UniformMatrix4fvARB(disp) GET_by_offset(disp, driDispatchRemapTable[UniformMatrix4fvARB_remap_index])
#define SET_UniformMatrix4fvARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UniformMatrix4fvARB_remap_index], fn)
#define CALL_UseProgramObjectARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), driDispatchRemapTable[UseProgramObjectARB_remap_index], parameters)
#define GET_UseProgramObjectARB(disp) GET_by_offset(disp, driDispatchRemapTable[UseProgramObjectARB_remap_index])
#define SET_UseProgramObjectARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UseProgramObjectARB_remap_index], fn)
#define CALL_ValidateProgramARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB)), driDispatchRemapTable[ValidateProgramARB_remap_index], parameters)
#define GET_ValidateProgramARB(disp) GET_by_offset(disp, driDispatchRemapTable[ValidateProgramARB_remap_index])
#define SET_ValidateProgramARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ValidateProgramARB_remap_index], fn)
#define CALL_BindAttribLocationARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, const GLcharARB *)), driDispatchRemapTable[BindAttribLocationARB_remap_index], parameters)
#define GET_BindAttribLocationARB(disp) GET_by_offset(disp, driDispatchRemapTable[BindAttribLocationARB_remap_index])
#define SET_BindAttribLocationARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindAttribLocationARB_remap_index], fn)
#define CALL_GetActiveAttribARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)), driDispatchRemapTable[GetActiveAttribARB_remap_index], parameters)
#define GET_GetActiveAttribARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetActiveAttribARB_remap_index])
#define SET_GetActiveAttribARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetActiveAttribARB_remap_index], fn)
#define CALL_GetAttribLocationARB(disp, parameters) CALL_by_offset(disp, (GLint (GLAPIENTRYP)(GLhandleARB, const GLcharARB *)), driDispatchRemapTable[GetAttribLocationARB_remap_index], parameters)
#define GET_GetAttribLocationARB(disp) GET_by_offset(disp, driDispatchRemapTable[GetAttribLocationARB_remap_index])
#define SET_GetAttribLocationARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetAttribLocationARB_remap_index], fn)
#define CALL_DrawBuffersARB(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLenum *)), driDispatchRemapTable[DrawBuffersARB_remap_index], parameters)
#define GET_DrawBuffersARB(disp) GET_by_offset(disp, driDispatchRemapTable[DrawBuffersARB_remap_index])
#define SET_DrawBuffersARB(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DrawBuffersARB_remap_index], fn)
#define CALL_PolygonOffsetEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), driDispatchRemapTable[PolygonOffsetEXT_remap_index], parameters)
#define GET_PolygonOffsetEXT(disp) GET_by_offset(disp, driDispatchRemapTable[PolygonOffsetEXT_remap_index])
#define SET_PolygonOffsetEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PolygonOffsetEXT_remap_index], fn)
#define CALL_GetPixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), driDispatchRemapTable[GetPixelTexGenParameterfvSGIS_remap_index], parameters)
#define GET_GetPixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[GetPixelTexGenParameterfvSGIS_remap_index])
#define SET_GetPixelTexGenParameterfvSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetPixelTexGenParameterfvSGIS_remap_index], fn)
#define CALL_GetPixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *)), driDispatchRemapTable[GetPixelTexGenParameterivSGIS_remap_index], parameters)
#define GET_GetPixelTexGenParameterivSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[GetPixelTexGenParameterivSGIS_remap_index])
#define SET_GetPixelTexGenParameterivSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetPixelTexGenParameterivSGIS_remap_index], fn)
#define CALL_PixelTexGenParameterfSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), driDispatchRemapTable[PixelTexGenParameterfSGIS_remap_index], parameters)
#define GET_PixelTexGenParameterfSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterfSGIS_remap_index])
#define SET_PixelTexGenParameterfSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterfSGIS_remap_index], fn)
#define CALL_PixelTexGenParameterfvSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), driDispatchRemapTable[PixelTexGenParameterfvSGIS_remap_index], parameters)
#define GET_PixelTexGenParameterfvSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterfvSGIS_remap_index])
#define SET_PixelTexGenParameterfvSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterfvSGIS_remap_index], fn)
#define CALL_PixelTexGenParameteriSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), driDispatchRemapTable[PixelTexGenParameteriSGIS_remap_index], parameters)
#define GET_PixelTexGenParameteriSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameteriSGIS_remap_index])
#define SET_PixelTexGenParameteriSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameteriSGIS_remap_index], fn)
#define CALL_PixelTexGenParameterivSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), driDispatchRemapTable[PixelTexGenParameterivSGIS_remap_index], parameters)
#define GET_PixelTexGenParameterivSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterivSGIS_remap_index])
#define SET_PixelTexGenParameterivSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PixelTexGenParameterivSGIS_remap_index], fn)
#define CALL_SampleMaskSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampf, GLboolean)), driDispatchRemapTable[SampleMaskSGIS_remap_index], parameters)
#define GET_SampleMaskSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[SampleMaskSGIS_remap_index])
#define SET_SampleMaskSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SampleMaskSGIS_remap_index], fn)
#define CALL_SamplePatternSGIS(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[SamplePatternSGIS_remap_index], parameters)
#define GET_SamplePatternSGIS(disp) GET_by_offset(disp, driDispatchRemapTable[SamplePatternSGIS_remap_index])
#define SET_SamplePatternSGIS(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SamplePatternSGIS_remap_index], fn)
#define CALL_ColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), driDispatchRemapTable[ColorPointerEXT_remap_index], parameters)
#define GET_ColorPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[ColorPointerEXT_remap_index])
#define SET_ColorPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ColorPointerEXT_remap_index], fn)
#define CALL_EdgeFlagPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLsizei, const GLboolean *)), driDispatchRemapTable[EdgeFlagPointerEXT_remap_index], parameters)
#define GET_EdgeFlagPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[EdgeFlagPointerEXT_remap_index])
#define SET_EdgeFlagPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[EdgeFlagPointerEXT_remap_index], fn)
#define CALL_IndexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), driDispatchRemapTable[IndexPointerEXT_remap_index], parameters)
#define GET_IndexPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[IndexPointerEXT_remap_index])
#define SET_IndexPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IndexPointerEXT_remap_index], fn)
#define CALL_NormalPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, GLsizei, const GLvoid *)), driDispatchRemapTable[NormalPointerEXT_remap_index], parameters)
#define GET_NormalPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[NormalPointerEXT_remap_index])
#define SET_NormalPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[NormalPointerEXT_remap_index], fn)
#define CALL_TexCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), driDispatchRemapTable[TexCoordPointerEXT_remap_index], parameters)
#define GET_TexCoordPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[TexCoordPointerEXT_remap_index])
#define SET_TexCoordPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[TexCoordPointerEXT_remap_index], fn)
#define CALL_VertexPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *)), driDispatchRemapTable[VertexPointerEXT_remap_index], parameters)
#define GET_VertexPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[VertexPointerEXT_remap_index])
#define SET_VertexPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexPointerEXT_remap_index], fn)
#define CALL_PointParameterfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), driDispatchRemapTable[PointParameterfEXT_remap_index], parameters)
#define GET_PointParameterfEXT(disp) GET_by_offset(disp, driDispatchRemapTable[PointParameterfEXT_remap_index])
#define SET_PointParameterfEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PointParameterfEXT_remap_index], fn)
#define CALL_PointParameterfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), driDispatchRemapTable[PointParameterfvEXT_remap_index], parameters)
#define GET_PointParameterfvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[PointParameterfvEXT_remap_index])
#define SET_PointParameterfvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PointParameterfvEXT_remap_index], fn)
#define CALL_LockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLsizei)), driDispatchRemapTable[LockArraysEXT_remap_index], parameters)
#define GET_LockArraysEXT(disp) GET_by_offset(disp, driDispatchRemapTable[LockArraysEXT_remap_index])
#define SET_LockArraysEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[LockArraysEXT_remap_index], fn)
#define CALL_UnlockArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), driDispatchRemapTable[UnlockArraysEXT_remap_index], parameters)
#define GET_UnlockArraysEXT(disp) GET_by_offset(disp, driDispatchRemapTable[UnlockArraysEXT_remap_index])
#define SET_UnlockArraysEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[UnlockArraysEXT_remap_index], fn)
#define CALL_CullParameterdvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLdouble *)), driDispatchRemapTable[CullParameterdvEXT_remap_index], parameters)
#define GET_CullParameterdvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[CullParameterdvEXT_remap_index])
#define SET_CullParameterdvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CullParameterdvEXT_remap_index], fn)
#define CALL_CullParameterfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat *)), driDispatchRemapTable[CullParameterfvEXT_remap_index], parameters)
#define GET_CullParameterfvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[CullParameterfvEXT_remap_index])
#define SET_CullParameterfvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CullParameterfvEXT_remap_index], fn)
#define CALL_SecondaryColor3bEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLbyte, GLbyte, GLbyte)), driDispatchRemapTable[SecondaryColor3bEXT_remap_index], parameters)
#define GET_SecondaryColor3bEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3bEXT_remap_index])
#define SET_SecondaryColor3bEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3bEXT_remap_index], fn)
#define CALL_SecondaryColor3bvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLbyte *)), driDispatchRemapTable[SecondaryColor3bvEXT_remap_index], parameters)
#define GET_SecondaryColor3bvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3bvEXT_remap_index])
#define SET_SecondaryColor3bvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3bvEXT_remap_index], fn)
#define CALL_SecondaryColor3dEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[SecondaryColor3dEXT_remap_index], parameters)
#define GET_SecondaryColor3dEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3dEXT_remap_index])
#define SET_SecondaryColor3dEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3dEXT_remap_index], fn)
#define CALL_SecondaryColor3dvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[SecondaryColor3dvEXT_remap_index], parameters)
#define GET_SecondaryColor3dvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3dvEXT_remap_index])
#define SET_SecondaryColor3dvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3dvEXT_remap_index], fn)
#define CALL_SecondaryColor3fEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[SecondaryColor3fEXT_remap_index], parameters)
#define GET_SecondaryColor3fEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3fEXT_remap_index])
#define SET_SecondaryColor3fEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3fEXT_remap_index], fn)
#define CALL_SecondaryColor3fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[SecondaryColor3fvEXT_remap_index], parameters)
#define GET_SecondaryColor3fvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3fvEXT_remap_index])
#define SET_SecondaryColor3fvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3fvEXT_remap_index], fn)
#define CALL_SecondaryColor3iEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), driDispatchRemapTable[SecondaryColor3iEXT_remap_index], parameters)
#define GET_SecondaryColor3iEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3iEXT_remap_index])
#define SET_SecondaryColor3iEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3iEXT_remap_index], fn)
#define CALL_SecondaryColor3ivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), driDispatchRemapTable[SecondaryColor3ivEXT_remap_index], parameters)
#define GET_SecondaryColor3ivEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ivEXT_remap_index])
#define SET_SecondaryColor3ivEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ivEXT_remap_index], fn)
#define CALL_SecondaryColor3sEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), driDispatchRemapTable[SecondaryColor3sEXT_remap_index], parameters)
#define GET_SecondaryColor3sEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3sEXT_remap_index])
#define SET_SecondaryColor3sEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3sEXT_remap_index], fn)
#define CALL_SecondaryColor3svEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), driDispatchRemapTable[SecondaryColor3svEXT_remap_index], parameters)
#define GET_SecondaryColor3svEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3svEXT_remap_index])
#define SET_SecondaryColor3svEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3svEXT_remap_index], fn)
#define CALL_SecondaryColor3ubEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLubyte, GLubyte, GLubyte)), driDispatchRemapTable[SecondaryColor3ubEXT_remap_index], parameters)
#define GET_SecondaryColor3ubEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ubEXT_remap_index])
#define SET_SecondaryColor3ubEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ubEXT_remap_index], fn)
#define CALL_SecondaryColor3ubvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLubyte *)), driDispatchRemapTable[SecondaryColor3ubvEXT_remap_index], parameters)
#define GET_SecondaryColor3ubvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ubvEXT_remap_index])
#define SET_SecondaryColor3ubvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3ubvEXT_remap_index], fn)
#define CALL_SecondaryColor3uiEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLuint)), driDispatchRemapTable[SecondaryColor3uiEXT_remap_index], parameters)
#define GET_SecondaryColor3uiEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3uiEXT_remap_index])
#define SET_SecondaryColor3uiEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3uiEXT_remap_index], fn)
#define CALL_SecondaryColor3uivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLuint *)), driDispatchRemapTable[SecondaryColor3uivEXT_remap_index], parameters)
#define GET_SecondaryColor3uivEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3uivEXT_remap_index])
#define SET_SecondaryColor3uivEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3uivEXT_remap_index], fn)
#define CALL_SecondaryColor3usEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLushort, GLushort, GLushort)), driDispatchRemapTable[SecondaryColor3usEXT_remap_index], parameters)
#define GET_SecondaryColor3usEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3usEXT_remap_index])
#define SET_SecondaryColor3usEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3usEXT_remap_index], fn)
#define CALL_SecondaryColor3usvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLushort *)), driDispatchRemapTable[SecondaryColor3usvEXT_remap_index], parameters)
#define GET_SecondaryColor3usvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColor3usvEXT_remap_index])
#define SET_SecondaryColor3usvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColor3usvEXT_remap_index], fn)
#define CALL_SecondaryColorPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[SecondaryColorPointerEXT_remap_index], parameters)
#define GET_SecondaryColorPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[SecondaryColorPointerEXT_remap_index])
#define SET_SecondaryColorPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SecondaryColorPointerEXT_remap_index], fn)
#define CALL_MultiDrawArraysEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint *, GLsizei *, GLsizei)), driDispatchRemapTable[MultiDrawArraysEXT_remap_index], parameters)
#define GET_MultiDrawArraysEXT(disp) GET_by_offset(disp, driDispatchRemapTable[MultiDrawArraysEXT_remap_index])
#define SET_MultiDrawArraysEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultiDrawArraysEXT_remap_index], fn)
#define CALL_MultiDrawElementsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLsizei *, GLenum, const GLvoid **, GLsizei)), driDispatchRemapTable[MultiDrawElementsEXT_remap_index], parameters)
#define GET_MultiDrawElementsEXT(disp) GET_by_offset(disp, driDispatchRemapTable[MultiDrawElementsEXT_remap_index])
#define SET_MultiDrawElementsEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultiDrawElementsEXT_remap_index], fn)
#define CALL_FogCoordPointerEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[FogCoordPointerEXT_remap_index], parameters)
#define GET_FogCoordPointerEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FogCoordPointerEXT_remap_index])
#define SET_FogCoordPointerEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FogCoordPointerEXT_remap_index], fn)
#define CALL_FogCoorddEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble)), driDispatchRemapTable[FogCoorddEXT_remap_index], parameters)
#define GET_FogCoorddEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FogCoorddEXT_remap_index])
#define SET_FogCoorddEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FogCoorddEXT_remap_index], fn)
#define CALL_FogCoorddvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[FogCoorddvEXT_remap_index], parameters)
#define GET_FogCoorddvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FogCoorddvEXT_remap_index])
#define SET_FogCoorddvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FogCoorddvEXT_remap_index], fn)
#define CALL_FogCoordfEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat)), driDispatchRemapTable[FogCoordfEXT_remap_index], parameters)
#define GET_FogCoordfEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FogCoordfEXT_remap_index])
#define SET_FogCoordfEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FogCoordfEXT_remap_index], fn)
#define CALL_FogCoordfvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[FogCoordfvEXT_remap_index], parameters)
#define GET_FogCoordfvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FogCoordfvEXT_remap_index])
#define SET_FogCoordfvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FogCoordfvEXT_remap_index], fn)
#define CALL_PixelTexGenSGIX(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[PixelTexGenSGIX_remap_index], parameters)
#define GET_PixelTexGenSGIX(disp) GET_by_offset(disp, driDispatchRemapTable[PixelTexGenSGIX_remap_index])
#define SET_PixelTexGenSGIX(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PixelTexGenSGIX_remap_index], fn)
#define CALL_BlendFuncSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), driDispatchRemapTable[BlendFuncSeparateEXT_remap_index], parameters)
#define GET_BlendFuncSeparateEXT(disp) GET_by_offset(disp, driDispatchRemapTable[BlendFuncSeparateEXT_remap_index])
#define SET_BlendFuncSeparateEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BlendFuncSeparateEXT_remap_index], fn)
#define CALL_FlushVertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), driDispatchRemapTable[FlushVertexArrayRangeNV_remap_index], parameters)
#define GET_FlushVertexArrayRangeNV(disp) GET_by_offset(disp, driDispatchRemapTable[FlushVertexArrayRangeNV_remap_index])
#define SET_FlushVertexArrayRangeNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FlushVertexArrayRangeNV_remap_index], fn)
#define CALL_VertexArrayRangeNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLvoid *)), driDispatchRemapTable[VertexArrayRangeNV_remap_index], parameters)
#define GET_VertexArrayRangeNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexArrayRangeNV_remap_index])
#define SET_VertexArrayRangeNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexArrayRangeNV_remap_index], fn)
#define CALL_CombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum)), driDispatchRemapTable[CombinerInputNV_remap_index], parameters)
#define GET_CombinerInputNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerInputNV_remap_index])
#define SET_CombinerInputNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerInputNV_remap_index], fn)
#define CALL_CombinerOutputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean)), driDispatchRemapTable[CombinerOutputNV_remap_index], parameters)
#define GET_CombinerOutputNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerOutputNV_remap_index])
#define SET_CombinerOutputNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerOutputNV_remap_index], fn)
#define CALL_CombinerParameterfNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLfloat)), driDispatchRemapTable[CombinerParameterfNV_remap_index], parameters)
#define GET_CombinerParameterfNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerParameterfNV_remap_index])
#define SET_CombinerParameterfNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerParameterfNV_remap_index], fn)
#define CALL_CombinerParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLfloat *)), driDispatchRemapTable[CombinerParameterfvNV_remap_index], parameters)
#define GET_CombinerParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerParameterfvNV_remap_index])
#define SET_CombinerParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerParameterfvNV_remap_index], fn)
#define CALL_CombinerParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), driDispatchRemapTable[CombinerParameteriNV_remap_index], parameters)
#define GET_CombinerParameteriNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerParameteriNV_remap_index])
#define SET_CombinerParameteriNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerParameteriNV_remap_index], fn)
#define CALL_CombinerParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), driDispatchRemapTable[CombinerParameterivNV_remap_index], parameters)
#define GET_CombinerParameterivNV(disp) GET_by_offset(disp, driDispatchRemapTable[CombinerParameterivNV_remap_index])
#define SET_CombinerParameterivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CombinerParameterivNV_remap_index], fn)
#define CALL_FinalCombinerInputNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum)), driDispatchRemapTable[FinalCombinerInputNV_remap_index], parameters)
#define GET_FinalCombinerInputNV(disp) GET_by_offset(disp, driDispatchRemapTable[FinalCombinerInputNV_remap_index])
#define SET_FinalCombinerInputNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FinalCombinerInputNV_remap_index], fn)
#define CALL_GetCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLfloat *)), driDispatchRemapTable[GetCombinerInputParameterfvNV_remap_index], parameters)
#define GET_GetCombinerInputParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetCombinerInputParameterfvNV_remap_index])
#define SET_GetCombinerInputParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetCombinerInputParameterfvNV_remap_index], fn)
#define CALL_GetCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLenum, GLint *)), driDispatchRemapTable[GetCombinerInputParameterivNV_remap_index], parameters)
#define GET_GetCombinerInputParameterivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetCombinerInputParameterivNV_remap_index])
#define SET_GetCombinerInputParameterivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetCombinerInputParameterivNV_remap_index], fn)
#define CALL_GetCombinerOutputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLfloat *)), driDispatchRemapTable[GetCombinerOutputParameterfvNV_remap_index], parameters)
#define GET_GetCombinerOutputParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetCombinerOutputParameterfvNV_remap_index])
#define SET_GetCombinerOutputParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetCombinerOutputParameterfvNV_remap_index], fn)
#define CALL_GetCombinerOutputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), driDispatchRemapTable[GetCombinerOutputParameterivNV_remap_index], parameters)
#define GET_GetCombinerOutputParameterivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetCombinerOutputParameterivNV_remap_index])
#define SET_GetCombinerOutputParameterivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetCombinerOutputParameterivNV_remap_index], fn)
#define CALL_GetFinalCombinerInputParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLfloat *)), driDispatchRemapTable[GetFinalCombinerInputParameterfvNV_remap_index], parameters)
#define GET_GetFinalCombinerInputParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetFinalCombinerInputParameterfvNV_remap_index])
#define SET_GetFinalCombinerInputParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetFinalCombinerInputParameterfvNV_remap_index], fn)
#define CALL_GetFinalCombinerInputParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), driDispatchRemapTable[GetFinalCombinerInputParameterivNV_remap_index], parameters)
#define GET_GetFinalCombinerInputParameterivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetFinalCombinerInputParameterivNV_remap_index])
#define SET_GetFinalCombinerInputParameterivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetFinalCombinerInputParameterivNV_remap_index], fn)
#define CALL_ResizeBuffersMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), driDispatchRemapTable[ResizeBuffersMESA_remap_index], parameters)
#define GET_ResizeBuffersMESA(disp) GET_by_offset(disp, driDispatchRemapTable[ResizeBuffersMESA_remap_index])
#define SET_ResizeBuffersMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ResizeBuffersMESA_remap_index], fn)
#define CALL_WindowPos2dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble)), driDispatchRemapTable[WindowPos2dMESA_remap_index], parameters)
#define GET_WindowPos2dMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2dMESA_remap_index])
#define SET_WindowPos2dMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2dMESA_remap_index], fn)
#define CALL_WindowPos2dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[WindowPos2dvMESA_remap_index], parameters)
#define GET_WindowPos2dvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2dvMESA_remap_index])
#define SET_WindowPos2dvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2dvMESA_remap_index], fn)
#define CALL_WindowPos2fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat)), driDispatchRemapTable[WindowPos2fMESA_remap_index], parameters)
#define GET_WindowPos2fMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2fMESA_remap_index])
#define SET_WindowPos2fMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2fMESA_remap_index], fn)
#define CALL_WindowPos2fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[WindowPos2fvMESA_remap_index], parameters)
#define GET_WindowPos2fvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2fvMESA_remap_index])
#define SET_WindowPos2fvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2fvMESA_remap_index], fn)
#define CALL_WindowPos2iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint)), driDispatchRemapTable[WindowPos2iMESA_remap_index], parameters)
#define GET_WindowPos2iMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2iMESA_remap_index])
#define SET_WindowPos2iMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2iMESA_remap_index], fn)
#define CALL_WindowPos2ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), driDispatchRemapTable[WindowPos2ivMESA_remap_index], parameters)
#define GET_WindowPos2ivMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2ivMESA_remap_index])
#define SET_WindowPos2ivMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2ivMESA_remap_index], fn)
#define CALL_WindowPos2sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort)), driDispatchRemapTable[WindowPos2sMESA_remap_index], parameters)
#define GET_WindowPos2sMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2sMESA_remap_index])
#define SET_WindowPos2sMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2sMESA_remap_index], fn)
#define CALL_WindowPos2svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), driDispatchRemapTable[WindowPos2svMESA_remap_index], parameters)
#define GET_WindowPos2svMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos2svMESA_remap_index])
#define SET_WindowPos2svMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos2svMESA_remap_index], fn)
#define CALL_WindowPos3dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[WindowPos3dMESA_remap_index], parameters)
#define GET_WindowPos3dMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3dMESA_remap_index])
#define SET_WindowPos3dMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3dMESA_remap_index], fn)
#define CALL_WindowPos3dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[WindowPos3dvMESA_remap_index], parameters)
#define GET_WindowPos3dvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3dvMESA_remap_index])
#define SET_WindowPos3dvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3dvMESA_remap_index], fn)
#define CALL_WindowPos3fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[WindowPos3fMESA_remap_index], parameters)
#define GET_WindowPos3fMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3fMESA_remap_index])
#define SET_WindowPos3fMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3fMESA_remap_index], fn)
#define CALL_WindowPos3fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[WindowPos3fvMESA_remap_index], parameters)
#define GET_WindowPos3fvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3fvMESA_remap_index])
#define SET_WindowPos3fvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3fvMESA_remap_index], fn)
#define CALL_WindowPos3iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint)), driDispatchRemapTable[WindowPos3iMESA_remap_index], parameters)
#define GET_WindowPos3iMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3iMESA_remap_index])
#define SET_WindowPos3iMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3iMESA_remap_index], fn)
#define CALL_WindowPos3ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), driDispatchRemapTable[WindowPos3ivMESA_remap_index], parameters)
#define GET_WindowPos3ivMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3ivMESA_remap_index])
#define SET_WindowPos3ivMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3ivMESA_remap_index], fn)
#define CALL_WindowPos3sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort)), driDispatchRemapTable[WindowPos3sMESA_remap_index], parameters)
#define GET_WindowPos3sMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3sMESA_remap_index])
#define SET_WindowPos3sMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3sMESA_remap_index], fn)
#define CALL_WindowPos3svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), driDispatchRemapTable[WindowPos3svMESA_remap_index], parameters)
#define GET_WindowPos3svMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos3svMESA_remap_index])
#define SET_WindowPos3svMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos3svMESA_remap_index], fn)
#define CALL_WindowPos4dMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[WindowPos4dMESA_remap_index], parameters)
#define GET_WindowPos4dMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4dMESA_remap_index])
#define SET_WindowPos4dMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4dMESA_remap_index], fn)
#define CALL_WindowPos4dvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLdouble *)), driDispatchRemapTable[WindowPos4dvMESA_remap_index], parameters)
#define GET_WindowPos4dvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4dvMESA_remap_index])
#define SET_WindowPos4dvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4dvMESA_remap_index], fn)
#define CALL_WindowPos4fMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[WindowPos4fMESA_remap_index], parameters)
#define GET_WindowPos4fMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4fMESA_remap_index])
#define SET_WindowPos4fMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4fMESA_remap_index], fn)
#define CALL_WindowPos4fvMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLfloat *)), driDispatchRemapTable[WindowPos4fvMESA_remap_index], parameters)
#define GET_WindowPos4fvMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4fvMESA_remap_index])
#define SET_WindowPos4fvMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4fvMESA_remap_index], fn)
#define CALL_WindowPos4iMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint)), driDispatchRemapTable[WindowPos4iMESA_remap_index], parameters)
#define GET_WindowPos4iMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4iMESA_remap_index])
#define SET_WindowPos4iMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4iMESA_remap_index], fn)
#define CALL_WindowPos4ivMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLint *)), driDispatchRemapTable[WindowPos4ivMESA_remap_index], parameters)
#define GET_WindowPos4ivMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4ivMESA_remap_index])
#define SET_WindowPos4ivMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4ivMESA_remap_index], fn)
#define CALL_WindowPos4sMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLshort, GLshort, GLshort, GLshort)), driDispatchRemapTable[WindowPos4sMESA_remap_index], parameters)
#define GET_WindowPos4sMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4sMESA_remap_index])
#define SET_WindowPos4sMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4sMESA_remap_index], fn)
#define CALL_WindowPos4svMESA(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLshort *)), driDispatchRemapTable[WindowPos4svMESA_remap_index], parameters)
#define GET_WindowPos4svMESA(disp) GET_by_offset(disp, driDispatchRemapTable[WindowPos4svMESA_remap_index])
#define SET_WindowPos4svMESA(disp, fn) SET_by_offset(disp, driDispatchRemapTable[WindowPos4svMESA_remap_index], fn)
#define CALL_MultiModeDrawArraysIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLint *, const GLsizei *, GLsizei, GLint)), driDispatchRemapTable[MultiModeDrawArraysIBM_remap_index], parameters)
#define GET_MultiModeDrawArraysIBM(disp) GET_by_offset(disp, driDispatchRemapTable[MultiModeDrawArraysIBM_remap_index])
#define SET_MultiModeDrawArraysIBM(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultiModeDrawArraysIBM_remap_index], fn)
#define CALL_MultiModeDrawElementsIBM(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(const GLenum *, const GLsizei *, GLenum, const GLvoid * const *, GLsizei, GLint)), driDispatchRemapTable[MultiModeDrawElementsIBM_remap_index], parameters)
#define GET_MultiModeDrawElementsIBM(disp) GET_by_offset(disp, driDispatchRemapTable[MultiModeDrawElementsIBM_remap_index])
#define SET_MultiModeDrawElementsIBM(disp, fn) SET_by_offset(disp, driDispatchRemapTable[MultiModeDrawElementsIBM_remap_index], fn)
#define CALL_DeleteFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteFencesNV_remap_index], parameters)
#define GET_DeleteFencesNV(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteFencesNV_remap_index])
#define SET_DeleteFencesNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteFencesNV_remap_index], fn)
#define CALL_FinishFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[FinishFenceNV_remap_index], parameters)
#define GET_FinishFenceNV(disp) GET_by_offset(disp, driDispatchRemapTable[FinishFenceNV_remap_index])
#define SET_FinishFenceNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FinishFenceNV_remap_index], fn)
#define CALL_GenFencesNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenFencesNV_remap_index], parameters)
#define GET_GenFencesNV(disp) GET_by_offset(disp, driDispatchRemapTable[GenFencesNV_remap_index])
#define SET_GenFencesNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenFencesNV_remap_index], fn)
#define CALL_GetFenceivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetFenceivNV_remap_index], parameters)
#define GET_GetFenceivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetFenceivNV_remap_index])
#define SET_GetFenceivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetFenceivNV_remap_index], fn)
#define CALL_IsFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsFenceNV_remap_index], parameters)
#define GET_IsFenceNV(disp) GET_by_offset(disp, driDispatchRemapTable[IsFenceNV_remap_index])
#define SET_IsFenceNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsFenceNV_remap_index], fn)
#define CALL_SetFenceNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum)), driDispatchRemapTable[SetFenceNV_remap_index], parameters)
#define GET_SetFenceNV(disp) GET_by_offset(disp, driDispatchRemapTable[SetFenceNV_remap_index])
#define SET_SetFenceNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SetFenceNV_remap_index], fn)
#define CALL_TestFenceNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[TestFenceNV_remap_index], parameters)
#define GET_TestFenceNV(disp) GET_by_offset(disp, driDispatchRemapTable[TestFenceNV_remap_index])
#define SET_TestFenceNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[TestFenceNV_remap_index], fn)
#define CALL_AreProgramsResidentNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLsizei, const GLuint *, GLboolean *)), driDispatchRemapTable[AreProgramsResidentNV_remap_index], parameters)
#define GET_AreProgramsResidentNV(disp) GET_by_offset(disp, driDispatchRemapTable[AreProgramsResidentNV_remap_index])
#define SET_AreProgramsResidentNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AreProgramsResidentNV_remap_index], fn)
#define CALL_BindProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[BindProgramNV_remap_index], parameters)
#define GET_BindProgramNV(disp) GET_by_offset(disp, driDispatchRemapTable[BindProgramNV_remap_index])
#define SET_BindProgramNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindProgramNV_remap_index], fn)
#define CALL_DeleteProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteProgramsNV_remap_index], parameters)
#define GET_DeleteProgramsNV(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteProgramsNV_remap_index])
#define SET_DeleteProgramsNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteProgramsNV_remap_index], fn)
#define CALL_ExecuteProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), driDispatchRemapTable[ExecuteProgramNV_remap_index], parameters)
#define GET_ExecuteProgramNV(disp) GET_by_offset(disp, driDispatchRemapTable[ExecuteProgramNV_remap_index])
#define SET_ExecuteProgramNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ExecuteProgramNV_remap_index], fn)
#define CALL_GenProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenProgramsNV_remap_index], parameters)
#define GET_GenProgramsNV(disp) GET_by_offset(disp, driDispatchRemapTable[GenProgramsNV_remap_index])
#define SET_GenProgramsNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenProgramsNV_remap_index], fn)
#define CALL_GetProgramParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLdouble *)), driDispatchRemapTable[GetProgramParameterdvNV_remap_index], parameters)
#define GET_GetProgramParameterdvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramParameterdvNV_remap_index])
#define SET_GetProgramParameterdvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramParameterdvNV_remap_index], fn)
#define CALL_GetProgramParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLfloat *)), driDispatchRemapTable[GetProgramParameterfvNV_remap_index], parameters)
#define GET_GetProgramParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramParameterfvNV_remap_index])
#define SET_GetProgramParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramParameterfvNV_remap_index], fn)
#define CALL_GetProgramStringNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLubyte *)), driDispatchRemapTable[GetProgramStringNV_remap_index], parameters)
#define GET_GetProgramStringNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramStringNV_remap_index])
#define SET_GetProgramStringNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramStringNV_remap_index], fn)
#define CALL_GetProgramivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetProgramivNV_remap_index], parameters)
#define GET_GetProgramivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramivNV_remap_index])
#define SET_GetProgramivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramivNV_remap_index], fn)
#define CALL_GetTrackMatrixivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLint *)), driDispatchRemapTable[GetTrackMatrixivNV_remap_index], parameters)
#define GET_GetTrackMatrixivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetTrackMatrixivNV_remap_index])
#define SET_GetTrackMatrixivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetTrackMatrixivNV_remap_index], fn)
#define CALL_GetVertexAttribPointervNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLvoid **)), driDispatchRemapTable[GetVertexAttribPointervNV_remap_index], parameters)
#define GET_GetVertexAttribPointervNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribPointervNV_remap_index])
#define SET_GetVertexAttribPointervNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribPointervNV_remap_index], fn)
#define CALL_GetVertexAttribdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLdouble *)), driDispatchRemapTable[GetVertexAttribdvNV_remap_index], parameters)
#define GET_GetVertexAttribdvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribdvNV_remap_index])
#define SET_GetVertexAttribdvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribdvNV_remap_index], fn)
#define CALL_GetVertexAttribfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLfloat *)), driDispatchRemapTable[GetVertexAttribfvNV_remap_index], parameters)
#define GET_GetVertexAttribfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribfvNV_remap_index])
#define SET_GetVertexAttribfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribfvNV_remap_index], fn)
#define CALL_GetVertexAttribivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint *)), driDispatchRemapTable[GetVertexAttribivNV_remap_index], parameters)
#define GET_GetVertexAttribivNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetVertexAttribivNV_remap_index])
#define SET_GetVertexAttribivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetVertexAttribivNV_remap_index], fn)
#define CALL_IsProgramNV(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsProgramNV_remap_index], parameters)
#define GET_IsProgramNV(disp) GET_by_offset(disp, driDispatchRemapTable[IsProgramNV_remap_index])
#define SET_IsProgramNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsProgramNV_remap_index], fn)
#define CALL_LoadProgramNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLubyte *)), driDispatchRemapTable[LoadProgramNV_remap_index], parameters)
#define GET_LoadProgramNV(disp) GET_by_offset(disp, driDispatchRemapTable[LoadProgramNV_remap_index])
#define SET_LoadProgramNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[LoadProgramNV_remap_index], fn)
#define CALL_ProgramParameter4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[ProgramParameter4dNV_remap_index], parameters)
#define GET_ProgramParameter4dNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameter4dNV_remap_index])
#define SET_ProgramParameter4dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameter4dNV_remap_index], fn)
#define CALL_ProgramParameter4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLdouble *)), driDispatchRemapTable[ProgramParameter4dvNV_remap_index], parameters)
#define GET_ProgramParameter4dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameter4dvNV_remap_index])
#define SET_ProgramParameter4dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameter4dvNV_remap_index], fn)
#define CALL_ProgramParameter4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[ProgramParameter4fNV_remap_index], parameters)
#define GET_ProgramParameter4fNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameter4fNV_remap_index])
#define SET_ProgramParameter4fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameter4fNV_remap_index], fn)
#define CALL_ProgramParameter4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, const GLfloat *)), driDispatchRemapTable[ProgramParameter4fvNV_remap_index], parameters)
#define GET_ProgramParameter4fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameter4fvNV_remap_index])
#define SET_ProgramParameter4fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameter4fvNV_remap_index], fn)
#define CALL_ProgramParameters4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, const GLdouble *)), driDispatchRemapTable[ProgramParameters4dvNV_remap_index], parameters)
#define GET_ProgramParameters4dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameters4dvNV_remap_index])
#define SET_ProgramParameters4dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameters4dvNV_remap_index], fn)
#define CALL_ProgramParameters4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, const GLfloat *)), driDispatchRemapTable[ProgramParameters4fvNV_remap_index], parameters)
#define GET_ProgramParameters4fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramParameters4fvNV_remap_index])
#define SET_ProgramParameters4fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramParameters4fvNV_remap_index], fn)
#define CALL_RequestResidentProgramsNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[RequestResidentProgramsNV_remap_index], parameters)
#define GET_RequestResidentProgramsNV(disp) GET_by_offset(disp, driDispatchRemapTable[RequestResidentProgramsNV_remap_index])
#define SET_RequestResidentProgramsNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[RequestResidentProgramsNV_remap_index], fn)
#define CALL_TrackMatrixNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLenum, GLenum)), driDispatchRemapTable[TrackMatrixNV_remap_index], parameters)
#define GET_TrackMatrixNV(disp) GET_by_offset(disp, driDispatchRemapTable[TrackMatrixNV_remap_index])
#define SET_TrackMatrixNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[TrackMatrixNV_remap_index], fn)
#define CALL_VertexAttrib1dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble)), driDispatchRemapTable[VertexAttrib1dNV_remap_index], parameters)
#define GET_VertexAttrib1dNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dNV_remap_index])
#define SET_VertexAttrib1dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dNV_remap_index], fn)
#define CALL_VertexAttrib1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib1dvNV_remap_index], parameters)
#define GET_VertexAttrib1dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dvNV_remap_index])
#define SET_VertexAttrib1dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1dvNV_remap_index], fn)
#define CALL_VertexAttrib1fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat)), driDispatchRemapTable[VertexAttrib1fNV_remap_index], parameters)
#define GET_VertexAttrib1fNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fNV_remap_index])
#define SET_VertexAttrib1fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fNV_remap_index], fn)
#define CALL_VertexAttrib1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib1fvNV_remap_index], parameters)
#define GET_VertexAttrib1fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fvNV_remap_index])
#define SET_VertexAttrib1fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1fvNV_remap_index], fn)
#define CALL_VertexAttrib1sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort)), driDispatchRemapTable[VertexAttrib1sNV_remap_index], parameters)
#define GET_VertexAttrib1sNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1sNV_remap_index])
#define SET_VertexAttrib1sNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1sNV_remap_index], fn)
#define CALL_VertexAttrib1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib1svNV_remap_index], parameters)
#define GET_VertexAttrib1svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib1svNV_remap_index])
#define SET_VertexAttrib1svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib1svNV_remap_index], fn)
#define CALL_VertexAttrib2dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib2dNV_remap_index], parameters)
#define GET_VertexAttrib2dNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dNV_remap_index])
#define SET_VertexAttrib2dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dNV_remap_index], fn)
#define CALL_VertexAttrib2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib2dvNV_remap_index], parameters)
#define GET_VertexAttrib2dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dvNV_remap_index])
#define SET_VertexAttrib2dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2dvNV_remap_index], fn)
#define CALL_VertexAttrib2fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib2fNV_remap_index], parameters)
#define GET_VertexAttrib2fNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fNV_remap_index])
#define SET_VertexAttrib2fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fNV_remap_index], fn)
#define CALL_VertexAttrib2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib2fvNV_remap_index], parameters)
#define GET_VertexAttrib2fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fvNV_remap_index])
#define SET_VertexAttrib2fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2fvNV_remap_index], fn)
#define CALL_VertexAttrib2sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib2sNV_remap_index], parameters)
#define GET_VertexAttrib2sNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2sNV_remap_index])
#define SET_VertexAttrib2sNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2sNV_remap_index], fn)
#define CALL_VertexAttrib2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib2svNV_remap_index], parameters)
#define GET_VertexAttrib2svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib2svNV_remap_index])
#define SET_VertexAttrib2svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib2svNV_remap_index], fn)
#define CALL_VertexAttrib3dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib3dNV_remap_index], parameters)
#define GET_VertexAttrib3dNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dNV_remap_index])
#define SET_VertexAttrib3dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dNV_remap_index], fn)
#define CALL_VertexAttrib3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib3dvNV_remap_index], parameters)
#define GET_VertexAttrib3dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dvNV_remap_index])
#define SET_VertexAttrib3dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3dvNV_remap_index], fn)
#define CALL_VertexAttrib3fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib3fNV_remap_index], parameters)
#define GET_VertexAttrib3fNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fNV_remap_index])
#define SET_VertexAttrib3fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fNV_remap_index], fn)
#define CALL_VertexAttrib3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib3fvNV_remap_index], parameters)
#define GET_VertexAttrib3fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fvNV_remap_index])
#define SET_VertexAttrib3fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3fvNV_remap_index], fn)
#define CALL_VertexAttrib3sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib3sNV_remap_index], parameters)
#define GET_VertexAttrib3sNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3sNV_remap_index])
#define SET_VertexAttrib3sNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3sNV_remap_index], fn)
#define CALL_VertexAttrib3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib3svNV_remap_index], parameters)
#define GET_VertexAttrib3svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib3svNV_remap_index])
#define SET_VertexAttrib3svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib3svNV_remap_index], fn)
#define CALL_VertexAttrib4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[VertexAttrib4dNV_remap_index], parameters)
#define GET_VertexAttrib4dNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dNV_remap_index])
#define SET_VertexAttrib4dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dNV_remap_index], fn)
#define CALL_VertexAttrib4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLdouble *)), driDispatchRemapTable[VertexAttrib4dvNV_remap_index], parameters)
#define GET_VertexAttrib4dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dvNV_remap_index])
#define SET_VertexAttrib4dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4dvNV_remap_index], fn)
#define CALL_VertexAttrib4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[VertexAttrib4fNV_remap_index], parameters)
#define GET_VertexAttrib4fNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fNV_remap_index])
#define SET_VertexAttrib4fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fNV_remap_index], fn)
#define CALL_VertexAttrib4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[VertexAttrib4fvNV_remap_index], parameters)
#define GET_VertexAttrib4fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fvNV_remap_index])
#define SET_VertexAttrib4fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4fvNV_remap_index], fn)
#define CALL_VertexAttrib4sNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)), driDispatchRemapTable[VertexAttrib4sNV_remap_index], parameters)
#define GET_VertexAttrib4sNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4sNV_remap_index])
#define SET_VertexAttrib4sNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4sNV_remap_index], fn)
#define CALL_VertexAttrib4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLshort *)), driDispatchRemapTable[VertexAttrib4svNV_remap_index], parameters)
#define GET_VertexAttrib4svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4svNV_remap_index])
#define SET_VertexAttrib4svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4svNV_remap_index], fn)
#define CALL_VertexAttrib4ubNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)), driDispatchRemapTable[VertexAttrib4ubNV_remap_index], parameters)
#define GET_VertexAttrib4ubNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubNV_remap_index])
#define SET_VertexAttrib4ubNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubNV_remap_index], fn)
#define CALL_VertexAttrib4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLubyte *)), driDispatchRemapTable[VertexAttrib4ubvNV_remap_index], parameters)
#define GET_VertexAttrib4ubvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubvNV_remap_index])
#define SET_VertexAttrib4ubvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttrib4ubvNV_remap_index], fn)
#define CALL_VertexAttribPointerNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLint, GLenum, GLsizei, const GLvoid *)), driDispatchRemapTable[VertexAttribPointerNV_remap_index], parameters)
#define GET_VertexAttribPointerNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribPointerNV_remap_index])
#define SET_VertexAttribPointerNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribPointerNV_remap_index], fn)
#define CALL_VertexAttribs1dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), driDispatchRemapTable[VertexAttribs1dvNV_remap_index], parameters)
#define GET_VertexAttribs1dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs1dvNV_remap_index])
#define SET_VertexAttribs1dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs1dvNV_remap_index], fn)
#define CALL_VertexAttribs1fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[VertexAttribs1fvNV_remap_index], parameters)
#define GET_VertexAttribs1fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs1fvNV_remap_index])
#define SET_VertexAttribs1fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs1fvNV_remap_index], fn)
#define CALL_VertexAttribs1svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), driDispatchRemapTable[VertexAttribs1svNV_remap_index], parameters)
#define GET_VertexAttribs1svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs1svNV_remap_index])
#define SET_VertexAttribs1svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs1svNV_remap_index], fn)
#define CALL_VertexAttribs2dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), driDispatchRemapTable[VertexAttribs2dvNV_remap_index], parameters)
#define GET_VertexAttribs2dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs2dvNV_remap_index])
#define SET_VertexAttribs2dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs2dvNV_remap_index], fn)
#define CALL_VertexAttribs2fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[VertexAttribs2fvNV_remap_index], parameters)
#define GET_VertexAttribs2fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs2fvNV_remap_index])
#define SET_VertexAttribs2fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs2fvNV_remap_index], fn)
#define CALL_VertexAttribs2svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), driDispatchRemapTable[VertexAttribs2svNV_remap_index], parameters)
#define GET_VertexAttribs2svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs2svNV_remap_index])
#define SET_VertexAttribs2svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs2svNV_remap_index], fn)
#define CALL_VertexAttribs3dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), driDispatchRemapTable[VertexAttribs3dvNV_remap_index], parameters)
#define GET_VertexAttribs3dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs3dvNV_remap_index])
#define SET_VertexAttribs3dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs3dvNV_remap_index], fn)
#define CALL_VertexAttribs3fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[VertexAttribs3fvNV_remap_index], parameters)
#define GET_VertexAttribs3fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs3fvNV_remap_index])
#define SET_VertexAttribs3fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs3fvNV_remap_index], fn)
#define CALL_VertexAttribs3svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), driDispatchRemapTable[VertexAttribs3svNV_remap_index], parameters)
#define GET_VertexAttribs3svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs3svNV_remap_index])
#define SET_VertexAttribs3svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs3svNV_remap_index], fn)
#define CALL_VertexAttribs4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLdouble *)), driDispatchRemapTable[VertexAttribs4dvNV_remap_index], parameters)
#define GET_VertexAttribs4dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs4dvNV_remap_index])
#define SET_VertexAttribs4dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs4dvNV_remap_index], fn)
#define CALL_VertexAttribs4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[VertexAttribs4fvNV_remap_index], parameters)
#define GET_VertexAttribs4fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs4fvNV_remap_index])
#define SET_VertexAttribs4fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs4fvNV_remap_index], fn)
#define CALL_VertexAttribs4svNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLshort *)), driDispatchRemapTable[VertexAttribs4svNV_remap_index], parameters)
#define GET_VertexAttribs4svNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs4svNV_remap_index])
#define SET_VertexAttribs4svNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs4svNV_remap_index], fn)
#define CALL_VertexAttribs4ubvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *)), driDispatchRemapTable[VertexAttribs4ubvNV_remap_index], parameters)
#define GET_VertexAttribs4ubvNV(disp) GET_by_offset(disp, driDispatchRemapTable[VertexAttribs4ubvNV_remap_index])
#define SET_VertexAttribs4ubvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[VertexAttribs4ubvNV_remap_index], fn)
#define CALL_AlphaFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[AlphaFragmentOp1ATI_remap_index], parameters)
#define GET_AlphaFragmentOp1ATI(disp) GET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp1ATI_remap_index])
#define SET_AlphaFragmentOp1ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp1ATI_remap_index], fn)
#define CALL_AlphaFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[AlphaFragmentOp2ATI_remap_index], parameters)
#define GET_AlphaFragmentOp2ATI(disp) GET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp2ATI_remap_index])
#define SET_AlphaFragmentOp2ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp2ATI_remap_index], fn)
#define CALL_AlphaFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[AlphaFragmentOp3ATI_remap_index], parameters)
#define GET_AlphaFragmentOp3ATI(disp) GET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp3ATI_remap_index])
#define SET_AlphaFragmentOp3ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[AlphaFragmentOp3ATI_remap_index], fn)
#define CALL_BeginFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), driDispatchRemapTable[BeginFragmentShaderATI_remap_index], parameters)
#define GET_BeginFragmentShaderATI(disp) GET_by_offset(disp, driDispatchRemapTable[BeginFragmentShaderATI_remap_index])
#define SET_BeginFragmentShaderATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BeginFragmentShaderATI_remap_index], fn)
#define CALL_BindFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[BindFragmentShaderATI_remap_index], parameters)
#define GET_BindFragmentShaderATI(disp) GET_by_offset(disp, driDispatchRemapTable[BindFragmentShaderATI_remap_index])
#define SET_BindFragmentShaderATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindFragmentShaderATI_remap_index], fn)
#define CALL_ColorFragmentOp1ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[ColorFragmentOp1ATI_remap_index], parameters)
#define GET_ColorFragmentOp1ATI(disp) GET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp1ATI_remap_index])
#define SET_ColorFragmentOp1ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp1ATI_remap_index], fn)
#define CALL_ColorFragmentOp2ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[ColorFragmentOp2ATI_remap_index], parameters)
#define GET_ColorFragmentOp2ATI(disp) GET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp2ATI_remap_index])
#define SET_ColorFragmentOp2ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp2ATI_remap_index], fn)
#define CALL_ColorFragmentOp3ATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint, GLuint)), driDispatchRemapTable[ColorFragmentOp3ATI_remap_index], parameters)
#define GET_ColorFragmentOp3ATI(disp) GET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp3ATI_remap_index])
#define SET_ColorFragmentOp3ATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ColorFragmentOp3ATI_remap_index], fn)
#define CALL_DeleteFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[DeleteFragmentShaderATI_remap_index], parameters)
#define GET_DeleteFragmentShaderATI(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteFragmentShaderATI_remap_index])
#define SET_DeleteFragmentShaderATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteFragmentShaderATI_remap_index], fn)
#define CALL_EndFragmentShaderATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(void)), driDispatchRemapTable[EndFragmentShaderATI_remap_index], parameters)
#define GET_EndFragmentShaderATI(disp) GET_by_offset(disp, driDispatchRemapTable[EndFragmentShaderATI_remap_index])
#define SET_EndFragmentShaderATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[EndFragmentShaderATI_remap_index], fn)
#define CALL_GenFragmentShadersATI(disp, parameters) CALL_by_offset(disp, (GLuint (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[GenFragmentShadersATI_remap_index], parameters)
#define GET_GenFragmentShadersATI(disp) GET_by_offset(disp, driDispatchRemapTable[GenFragmentShadersATI_remap_index])
#define SET_GenFragmentShadersATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenFragmentShadersATI_remap_index], fn)
#define CALL_PassTexCoordATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), driDispatchRemapTable[PassTexCoordATI_remap_index], parameters)
#define GET_PassTexCoordATI(disp) GET_by_offset(disp, driDispatchRemapTable[PassTexCoordATI_remap_index])
#define SET_PassTexCoordATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PassTexCoordATI_remap_index], fn)
#define CALL_SampleMapATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLuint, GLenum)), driDispatchRemapTable[SampleMapATI_remap_index], parameters)
#define GET_SampleMapATI(disp) GET_by_offset(disp, driDispatchRemapTable[SampleMapATI_remap_index])
#define SET_SampleMapATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SampleMapATI_remap_index], fn)
#define CALL_SetFragmentShaderConstantATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, const GLfloat *)), driDispatchRemapTable[SetFragmentShaderConstantATI_remap_index], parameters)
#define GET_SetFragmentShaderConstantATI(disp) GET_by_offset(disp, driDispatchRemapTable[SetFragmentShaderConstantATI_remap_index])
#define SET_SetFragmentShaderConstantATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[SetFragmentShaderConstantATI_remap_index], fn)
#define CALL_PointParameteriNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLint)), driDispatchRemapTable[PointParameteriNV_remap_index], parameters)
#define GET_PointParameteriNV(disp) GET_by_offset(disp, driDispatchRemapTable[PointParameteriNV_remap_index])
#define SET_PointParameteriNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PointParameteriNV_remap_index], fn)
#define CALL_PointParameterivNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, const GLint *)), driDispatchRemapTable[PointParameterivNV_remap_index], parameters)
#define GET_PointParameterivNV(disp) GET_by_offset(disp, driDispatchRemapTable[PointParameterivNV_remap_index])
#define SET_PointParameterivNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[PointParameterivNV_remap_index], fn)
#define CALL_ActiveStencilFaceEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[ActiveStencilFaceEXT_remap_index], parameters)
#define GET_ActiveStencilFaceEXT(disp) GET_by_offset(disp, driDispatchRemapTable[ActiveStencilFaceEXT_remap_index])
#define SET_ActiveStencilFaceEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ActiveStencilFaceEXT_remap_index], fn)
#define CALL_BindVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[BindVertexArrayAPPLE_remap_index], parameters)
#define GET_BindVertexArrayAPPLE(disp) GET_by_offset(disp, driDispatchRemapTable[BindVertexArrayAPPLE_remap_index])
#define SET_BindVertexArrayAPPLE(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindVertexArrayAPPLE_remap_index], fn)
#define CALL_DeleteVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteVertexArraysAPPLE_remap_index], parameters)
#define GET_DeleteVertexArraysAPPLE(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteVertexArraysAPPLE_remap_index])
#define SET_DeleteVertexArraysAPPLE(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteVertexArraysAPPLE_remap_index], fn)
#define CALL_GenVertexArraysAPPLE(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenVertexArraysAPPLE_remap_index], parameters)
#define GET_GenVertexArraysAPPLE(disp) GET_by_offset(disp, driDispatchRemapTable[GenVertexArraysAPPLE_remap_index])
#define SET_GenVertexArraysAPPLE(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenVertexArraysAPPLE_remap_index], fn)
#define CALL_IsVertexArrayAPPLE(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsVertexArrayAPPLE_remap_index], parameters)
#define GET_IsVertexArrayAPPLE(disp) GET_by_offset(disp, driDispatchRemapTable[IsVertexArrayAPPLE_remap_index])
#define SET_IsVertexArrayAPPLE(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsVertexArrayAPPLE_remap_index], fn)
#define CALL_GetProgramNamedParameterdvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble *)), driDispatchRemapTable[GetProgramNamedParameterdvNV_remap_index], parameters)
#define GET_GetProgramNamedParameterdvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramNamedParameterdvNV_remap_index])
#define SET_GetProgramNamedParameterdvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramNamedParameterdvNV_remap_index], fn)
#define CALL_GetProgramNamedParameterfvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat *)), driDispatchRemapTable[GetProgramNamedParameterfvNV_remap_index], parameters)
#define GET_GetProgramNamedParameterfvNV(disp) GET_by_offset(disp, driDispatchRemapTable[GetProgramNamedParameterfvNV_remap_index])
#define SET_GetProgramNamedParameterfvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetProgramNamedParameterfvNV_remap_index], fn)
#define CALL_ProgramNamedParameter4dNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLdouble, GLdouble, GLdouble, GLdouble)), driDispatchRemapTable[ProgramNamedParameter4dNV_remap_index], parameters)
#define GET_ProgramNamedParameter4dNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4dNV_remap_index])
#define SET_ProgramNamedParameter4dNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4dNV_remap_index], fn)
#define CALL_ProgramNamedParameter4dvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLdouble *)), driDispatchRemapTable[ProgramNamedParameter4dvNV_remap_index], parameters)
#define GET_ProgramNamedParameter4dvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4dvNV_remap_index])
#define SET_ProgramNamedParameter4dvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4dvNV_remap_index], fn)
#define CALL_ProgramNamedParameter4fNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, GLfloat, GLfloat, GLfloat, GLfloat)), driDispatchRemapTable[ProgramNamedParameter4fNV_remap_index], parameters)
#define GET_ProgramNamedParameter4fNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4fNV_remap_index])
#define SET_ProgramNamedParameter4fNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4fNV_remap_index], fn)
#define CALL_ProgramNamedParameter4fvNV(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLsizei, const GLubyte *, const GLfloat *)), driDispatchRemapTable[ProgramNamedParameter4fvNV_remap_index], parameters)
#define GET_ProgramNamedParameter4fvNV(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4fvNV_remap_index])
#define SET_ProgramNamedParameter4fvNV(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramNamedParameter4fvNV_remap_index], fn)
#define CALL_DepthBoundsEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLclampd, GLclampd)), driDispatchRemapTable[DepthBoundsEXT_remap_index], parameters)
#define GET_DepthBoundsEXT(disp) GET_by_offset(disp, driDispatchRemapTable[DepthBoundsEXT_remap_index])
#define SET_DepthBoundsEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DepthBoundsEXT_remap_index], fn)
#define CALL_BlendEquationSeparateEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum)), driDispatchRemapTable[BlendEquationSeparateEXT_remap_index], parameters)
#define GET_BlendEquationSeparateEXT(disp) GET_by_offset(disp, driDispatchRemapTable[BlendEquationSeparateEXT_remap_index])
#define SET_BlendEquationSeparateEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BlendEquationSeparateEXT_remap_index], fn)
#define CALL_BindFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[BindFramebufferEXT_remap_index], parameters)
#define GET_BindFramebufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[BindFramebufferEXT_remap_index])
#define SET_BindFramebufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindFramebufferEXT_remap_index], fn)
#define CALL_BindRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint)), driDispatchRemapTable[BindRenderbufferEXT_remap_index], parameters)
#define GET_BindRenderbufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[BindRenderbufferEXT_remap_index])
#define SET_BindRenderbufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BindRenderbufferEXT_remap_index], fn)
#define CALL_CheckFramebufferStatusEXT(disp, parameters) CALL_by_offset(disp, (GLenum (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[CheckFramebufferStatusEXT_remap_index], parameters)
#define GET_CheckFramebufferStatusEXT(disp) GET_by_offset(disp, driDispatchRemapTable[CheckFramebufferStatusEXT_remap_index])
#define SET_CheckFramebufferStatusEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[CheckFramebufferStatusEXT_remap_index], fn)
#define CALL_DeleteFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteFramebuffersEXT_remap_index], parameters)
#define GET_DeleteFramebuffersEXT(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteFramebuffersEXT_remap_index])
#define SET_DeleteFramebuffersEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteFramebuffersEXT_remap_index], fn)
#define CALL_DeleteRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, const GLuint *)), driDispatchRemapTable[DeleteRenderbuffersEXT_remap_index], parameters)
#define GET_DeleteRenderbuffersEXT(disp) GET_by_offset(disp, driDispatchRemapTable[DeleteRenderbuffersEXT_remap_index])
#define SET_DeleteRenderbuffersEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[DeleteRenderbuffersEXT_remap_index], fn)
#define CALL_FramebufferRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint)), driDispatchRemapTable[FramebufferRenderbufferEXT_remap_index], parameters)
#define GET_FramebufferRenderbufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FramebufferRenderbufferEXT_remap_index])
#define SET_FramebufferRenderbufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FramebufferRenderbufferEXT_remap_index], fn)
#define CALL_FramebufferTexture1DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), driDispatchRemapTable[FramebufferTexture1DEXT_remap_index], parameters)
#define GET_FramebufferTexture1DEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FramebufferTexture1DEXT_remap_index])
#define SET_FramebufferTexture1DEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FramebufferTexture1DEXT_remap_index], fn)
#define CALL_FramebufferTexture2DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint)), driDispatchRemapTable[FramebufferTexture2DEXT_remap_index], parameters)
#define GET_FramebufferTexture2DEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FramebufferTexture2DEXT_remap_index])
#define SET_FramebufferTexture2DEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FramebufferTexture2DEXT_remap_index], fn)
#define CALL_FramebufferTexture3DEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLuint, GLint, GLint)), driDispatchRemapTable[FramebufferTexture3DEXT_remap_index], parameters)
#define GET_FramebufferTexture3DEXT(disp) GET_by_offset(disp, driDispatchRemapTable[FramebufferTexture3DEXT_remap_index])
#define SET_FramebufferTexture3DEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[FramebufferTexture3DEXT_remap_index], fn)
#define CALL_GenFramebuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenFramebuffersEXT_remap_index], parameters)
#define GET_GenFramebuffersEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GenFramebuffersEXT_remap_index])
#define SET_GenFramebuffersEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenFramebuffersEXT_remap_index], fn)
#define CALL_GenRenderbuffersEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLsizei, GLuint *)), driDispatchRemapTable[GenRenderbuffersEXT_remap_index], parameters)
#define GET_GenRenderbuffersEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GenRenderbuffersEXT_remap_index])
#define SET_GenRenderbuffersEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenRenderbuffersEXT_remap_index], fn)
#define CALL_GenerateMipmapEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum)), driDispatchRemapTable[GenerateMipmapEXT_remap_index], parameters)
#define GET_GenerateMipmapEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GenerateMipmapEXT_remap_index])
#define SET_GenerateMipmapEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GenerateMipmapEXT_remap_index], fn)
#define CALL_GetFramebufferAttachmentParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLenum, GLint *)), driDispatchRemapTable[GetFramebufferAttachmentParameterivEXT_remap_index], parameters)
#define GET_GetFramebufferAttachmentParameterivEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GetFramebufferAttachmentParameterivEXT_remap_index])
#define SET_GetFramebufferAttachmentParameterivEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetFramebufferAttachmentParameterivEXT_remap_index], fn)
#define CALL_GetRenderbufferParameterivEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint *)), driDispatchRemapTable[GetRenderbufferParameterivEXT_remap_index], parameters)
#define GET_GetRenderbufferParameterivEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GetRenderbufferParameterivEXT_remap_index])
#define SET_GetRenderbufferParameterivEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetRenderbufferParameterivEXT_remap_index], fn)
#define CALL_IsFramebufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsFramebufferEXT_remap_index], parameters)
#define GET_IsFramebufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[IsFramebufferEXT_remap_index])
#define SET_IsFramebufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsFramebufferEXT_remap_index], fn)
#define CALL_IsRenderbufferEXT(disp, parameters) CALL_by_offset(disp, (GLboolean (GLAPIENTRYP)(GLuint)), driDispatchRemapTable[IsRenderbufferEXT_remap_index], parameters)
#define GET_IsRenderbufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[IsRenderbufferEXT_remap_index])
#define SET_IsRenderbufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[IsRenderbufferEXT_remap_index], fn)
#define CALL_RenderbufferStorageEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLsizei, GLsizei)), driDispatchRemapTable[RenderbufferStorageEXT_remap_index], parameters)
#define GET_RenderbufferStorageEXT(disp) GET_by_offset(disp, driDispatchRemapTable[RenderbufferStorageEXT_remap_index])
#define SET_RenderbufferStorageEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[RenderbufferStorageEXT_remap_index], fn)
#define CALL_BlitFramebufferEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum)), driDispatchRemapTable[BlitFramebufferEXT_remap_index], parameters)
#define GET_BlitFramebufferEXT(disp) GET_by_offset(disp, driDispatchRemapTable[BlitFramebufferEXT_remap_index])
#define SET_BlitFramebufferEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[BlitFramebufferEXT_remap_index], fn)
#define CALL_StencilFuncSeparateATI(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLenum, GLint, GLuint)), driDispatchRemapTable[StencilFuncSeparateATI_remap_index], parameters)
#define GET_StencilFuncSeparateATI(disp) GET_by_offset(disp, driDispatchRemapTable[StencilFuncSeparateATI_remap_index])
#define SET_StencilFuncSeparateATI(disp, fn) SET_by_offset(disp, driDispatchRemapTable[StencilFuncSeparateATI_remap_index], fn)
#define CALL_ProgramEnvParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[ProgramEnvParameters4fvEXT_remap_index], parameters)
#define GET_ProgramEnvParameters4fvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameters4fvEXT_remap_index])
#define SET_ProgramEnvParameters4fvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramEnvParameters4fvEXT_remap_index], fn)
#define CALL_ProgramLocalParameters4fvEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLenum, GLuint, GLsizei, const GLfloat *)), driDispatchRemapTable[ProgramLocalParameters4fvEXT_remap_index], parameters)
#define GET_ProgramLocalParameters4fvEXT(disp) GET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameters4fvEXT_remap_index])
#define SET_ProgramLocalParameters4fvEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[ProgramLocalParameters4fvEXT_remap_index], fn)
#define CALL_GetQueryObjecti64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLint64EXT *)), driDispatchRemapTable[GetQueryObjecti64vEXT_remap_index], parameters)
#define GET_GetQueryObjecti64vEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GetQueryObjecti64vEXT_remap_index])
#define SET_GetQueryObjecti64vEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetQueryObjecti64vEXT_remap_index], fn)
#define CALL_GetQueryObjectui64vEXT(disp, parameters) CALL_by_offset(disp, (void (GLAPIENTRYP)(GLuint, GLenum, GLuint64EXT *)), driDispatchRemapTable[GetQueryObjectui64vEXT_remap_index], parameters)
#define GET_GetQueryObjectui64vEXT(disp) GET_by_offset(disp, driDispatchRemapTable[GetQueryObjectui64vEXT_remap_index])
#define SET_GetQueryObjectui64vEXT(disp, fn) SET_by_offset(disp, driDispatchRemapTable[GetQueryObjectui64vEXT_remap_index], fn)

#endif /* !defined(IN_DRI_DRIVER) */

#endif /* !defined( _DISPATCH_H_ ) */
