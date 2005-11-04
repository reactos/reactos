/*
 * Mesa 3-D graphics library
 * Version:  5.0.1
 * 
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

/*
 * Mesa/FX device driver. Interface to Glide3.
 *
 *  Copyright (c) 2003 - Daniel Borca
 *  Email : dborca@users.sourceforge.net
 *  Web   : http://www.geocities.com/dborca
 */


#ifdef FX

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#define FX_TRAP_GLIDE_internal
#include "fxg.h"



/****************************************************************************\
* logging                                                                    *
\****************************************************************************/
#if FX_TRAP_GLIDE
#define TRAP_LOG trp_printf
#ifdef __GNUC__
__attribute__ ((format(printf, 1, 2)))
#endif /* __GNUC__ */
int trp_printf (const char *format, ...)
{
 va_list arg;
 int n;
 FILE *trap_file;
 va_start(arg, format);
 trap_file = fopen("trap.log", "a");
 if (trap_file == NULL) {
    trap_file = stderr;
 }
 n = vfprintf(trap_file, format, arg);
 fclose(trap_file);
 va_end(arg);
 return n;
}
#else  /* FX_TRAP_GLIDE */
#ifdef __GNUC__
#define TRAP_LOG(format, ...) do {} while (0)
#else  /* __GNUC__ */
#define TRAP_LOG              0 && (unsigned long)
#endif /* __GNUC__ */
#endif /* FX_TRAP_GLIDE */



#if FX_TRAP_GLIDE
/****************************************************************************\
* helpers                                                                    *
\****************************************************************************/

#define GOT "\t"

const char *TRP_BOOL (FxBool b)
{
 return b ? "FXTRUE" : "FXFALSE";
}

#define TRAP_CASE_STRING(name) case name: return #name
#define TRAP_NODEFAULT default: assert(0)

const char *TRP_PARAM (FxU32 mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_PARAM_DISABLE);
        TRAP_CASE_STRING(GR_PARAM_ENABLE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_VTX (FxU32 param)
{
 switch (param) {
        TRAP_CASE_STRING(GR_PARAM_XY);
        TRAP_CASE_STRING(GR_PARAM_Z);
        TRAP_CASE_STRING(GR_PARAM_W);
        TRAP_CASE_STRING(GR_PARAM_Q);
        TRAP_CASE_STRING(GR_PARAM_FOG_EXT);
        TRAP_CASE_STRING(GR_PARAM_A);
        TRAP_CASE_STRING(GR_PARAM_RGB);
        TRAP_CASE_STRING(GR_PARAM_PARGB);
        TRAP_CASE_STRING(GR_PARAM_ST0);
        TRAP_CASE_STRING(GR_PARAM_ST1);
        TRAP_CASE_STRING(GR_PARAM_ST2);
        TRAP_CASE_STRING(GR_PARAM_Q0);
        TRAP_CASE_STRING(GR_PARAM_Q1);
        TRAP_CASE_STRING(GR_PARAM_Q2);
        TRAP_NODEFAULT;
 }
}

const char *TRP_ARRAY (FxU32 mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_POINTS);
        TRAP_CASE_STRING(GR_LINE_STRIP);
        TRAP_CASE_STRING(GR_LINES);
        TRAP_CASE_STRING(GR_POLYGON);
        TRAP_CASE_STRING(GR_TRIANGLE_STRIP);
        TRAP_CASE_STRING(GR_TRIANGLE_FAN);
        TRAP_CASE_STRING(GR_TRIANGLES);
        TRAP_CASE_STRING(GR_TRIANGLE_STRIP_CONTINUE);
        TRAP_CASE_STRING(GR_TRIANGLE_FAN_CONTINUE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_BUFFER (GrBuffer_t buffer)
{
 switch (buffer) {
        TRAP_CASE_STRING(GR_BUFFER_FRONTBUFFER);
        TRAP_CASE_STRING(GR_BUFFER_BACKBUFFER);
        TRAP_CASE_STRING(GR_BUFFER_AUXBUFFER);
        TRAP_CASE_STRING(GR_BUFFER_DEPTHBUFFER);
        TRAP_CASE_STRING(GR_BUFFER_ALPHABUFFER);
        TRAP_CASE_STRING(GR_BUFFER_TRIPLEBUFFER);
        TRAP_CASE_STRING(GR_BUFFER_TEXTUREBUFFER_EXT);
        TRAP_CASE_STRING(GR_BUFFER_TEXTUREAUXBUFFER_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_ORIGIN (GrOriginLocation_t origin_location)
{
 switch (origin_location) {
        TRAP_CASE_STRING(GR_ORIGIN_UPPER_LEFT);
        TRAP_CASE_STRING(GR_ORIGIN_LOWER_LEFT);
        TRAP_CASE_STRING(GR_ORIGIN_ANY);
        TRAP_NODEFAULT;
 }
}

const char *TRP_REFRESH (GrScreenRefresh_t refresh_rate)
{
 switch (refresh_rate) {
        TRAP_CASE_STRING(GR_REFRESH_60Hz);
        TRAP_CASE_STRING(GR_REFRESH_70Hz);
        TRAP_CASE_STRING(GR_REFRESH_72Hz);
        TRAP_CASE_STRING(GR_REFRESH_75Hz);
        TRAP_CASE_STRING(GR_REFRESH_80Hz);
        TRAP_CASE_STRING(GR_REFRESH_90Hz);
        TRAP_CASE_STRING(GR_REFRESH_100Hz);
        TRAP_CASE_STRING(GR_REFRESH_85Hz);
        TRAP_CASE_STRING(GR_REFRESH_120Hz);
        TRAP_CASE_STRING(GR_REFRESH_NONE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_COLFMT (GrColorFormat_t color_format)
{
 switch (color_format) {
        TRAP_CASE_STRING(GR_COLORFORMAT_ARGB);
        TRAP_CASE_STRING(GR_COLORFORMAT_ABGR);
        TRAP_CASE_STRING(GR_COLORFORMAT_RGBA);
        TRAP_CASE_STRING(GR_COLORFORMAT_BGRA);
        TRAP_NODEFAULT;
 }
}

const char *TRP_RESOLUTION (GrScreenResolution_t screen_resolution)
{
 switch (screen_resolution) {
        TRAP_CASE_STRING(GR_RESOLUTION_320x200);
        TRAP_CASE_STRING(GR_RESOLUTION_320x240);
        TRAP_CASE_STRING(GR_RESOLUTION_400x256);
        TRAP_CASE_STRING(GR_RESOLUTION_512x384);
        TRAP_CASE_STRING(GR_RESOLUTION_640x200);
        TRAP_CASE_STRING(GR_RESOLUTION_640x350);
        TRAP_CASE_STRING(GR_RESOLUTION_640x400);
        TRAP_CASE_STRING(GR_RESOLUTION_640x480);
        TRAP_CASE_STRING(GR_RESOLUTION_800x600);
        TRAP_CASE_STRING(GR_RESOLUTION_960x720);
        TRAP_CASE_STRING(GR_RESOLUTION_856x480);
        TRAP_CASE_STRING(GR_RESOLUTION_512x256);
        TRAP_CASE_STRING(GR_RESOLUTION_1024x768);
        TRAP_CASE_STRING(GR_RESOLUTION_1280x1024);
        TRAP_CASE_STRING(GR_RESOLUTION_1600x1200);
        TRAP_CASE_STRING(GR_RESOLUTION_400x300);
        TRAP_CASE_STRING(GR_RESOLUTION_1152x864);
        TRAP_CASE_STRING(GR_RESOLUTION_1280x960);
        TRAP_CASE_STRING(GR_RESOLUTION_1600x1024);
        TRAP_CASE_STRING(GR_RESOLUTION_1792x1344);
        TRAP_CASE_STRING(GR_RESOLUTION_1856x1392);
        TRAP_CASE_STRING(GR_RESOLUTION_1920x1440);
        TRAP_CASE_STRING(GR_RESOLUTION_2048x1536);
        TRAP_CASE_STRING(GR_RESOLUTION_2048x2048);
        TRAP_CASE_STRING(GR_RESOLUTION_NONE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_BLEND (GrAlphaBlendFnc_t func)
{
 switch (func) {
        TRAP_CASE_STRING(GR_BLEND_ZERO);
        TRAP_CASE_STRING(GR_BLEND_SRC_ALPHA);
        TRAP_CASE_STRING(GR_BLEND_SRC_COLOR);
        /*TRAP_CASE_STRING(GR_BLEND_DST_COLOR); ==GR_BLEND_SRC_COLOR*/
        TRAP_CASE_STRING(GR_BLEND_DST_ALPHA);
        TRAP_CASE_STRING(GR_BLEND_ONE);
        TRAP_CASE_STRING(GR_BLEND_ONE_MINUS_SRC_ALPHA);
        TRAP_CASE_STRING(GR_BLEND_ONE_MINUS_SRC_COLOR);
        /*TRAP_CASE_STRING(GR_BLEND_ONE_MINUS_DST_COLOR); ==GR_BLEND_ONE_MINUS_SRC_COLOR*/
        TRAP_CASE_STRING(GR_BLEND_ONE_MINUS_DST_ALPHA);
        TRAP_CASE_STRING(GR_BLEND_SAME_COLOR_EXT);
        /*TRAP_CASE_STRING(GR_BLEND_RESERVED_8); ==GR_BLEND_SAME_COLOR_EXT*/
        TRAP_CASE_STRING(GR_BLEND_ONE_MINUS_SAME_COLOR_EXT);
        /*TRAP_CASE_STRING(GR_BLEND_RESERVED_9); ==GR_BLEND_ONE_MINUS_SAME_COLOR_EXT*/
        TRAP_CASE_STRING(GR_BLEND_RESERVED_A);
        TRAP_CASE_STRING(GR_BLEND_RESERVED_B);
        TRAP_CASE_STRING(GR_BLEND_RESERVED_C);
        TRAP_CASE_STRING(GR_BLEND_RESERVED_D);
        TRAP_CASE_STRING(GR_BLEND_RESERVED_E);
        TRAP_CASE_STRING(GR_BLEND_ALPHA_SATURATE);
        /*TRAP_CASE_STRING(GR_BLEND_PREFOG_COLOR); ==GR_BLEND_ALPHA_SATURATE*/
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMBFUNC (GrCombineFunction_t cfunc)
{
 switch (cfunc) {
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_ZERO);
        /*TRAP_CASE_STRING(GR_COMBINE_FUNCTION_NONE); ==GR_COMBINE_FUNCTION_ZERO*/
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_LOCAL);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_LOCAL_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER);
        /*TRAP_CASE_STRING(GR_COMBINE_FUNCTION_BLEND_OTHER); ==GR_COMBINE_FUNCTION_SCALE_OTHER*/
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL);
        /*TRAP_CASE_STRING(GR_COMBINE_FUNCTION_BLEND); ==GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL*/
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL);
        /*TRAP_CASE_STRING(GR_COMBINE_FUNCTION_BLEND_LOCAL); ==GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL*/
        TRAP_CASE_STRING(GR_COMBINE_FUNCTION_SCALE_MINUS_LOCAL_ADD_LOCAL_ALPHA);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMBFACT (GrCombineFactor_t cfactor)
{
 switch (cfactor) {
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ZERO);
        /*TRAP_CASE_STRING(GR_COMBINE_FACTOR_NONE); ==GR_COMBINE_FACTOR_ZERO*/
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_LOCAL);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_OTHER_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_LOCAL_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_TEXTURE_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_TEXTURE_RGB);
        /*TRAP_CASE_STRING(GR_COMBINE_FACTOR_DETAIL_FACTOR); ==GR_COMBINE_FACTOR_TEXTURE_ALPHA*/
        /*TRAP_CASE_STRING(GR_COMBINE_FACTOR_LOD_FRACTION); ==GR_COMBINE_FACTOR_TEXTURE_RGB ???*/
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_LOCAL);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_OTHER_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_LOCAL_ALPHA);
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA);
        /*TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_DETAIL_FACTOR); ==GR_COMBINE_FACTOR_ONE_MINUS_TEXTURE_ALPHA*/
        TRAP_CASE_STRING(GR_COMBINE_FACTOR_ONE_MINUS_LOD_FRACTION);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMBLOCAL (GrCombineLocal_t clocal)
{
 switch (clocal) {
        TRAP_CASE_STRING(GR_COMBINE_LOCAL_ITERATED);
        TRAP_CASE_STRING(GR_COMBINE_LOCAL_CONSTANT);
        /*TRAP_CASE_STRING(GR_COMBINE_LOCAL_NONE); ==GR_COMBINE_LOCAL_CONSTANT*/
        TRAP_CASE_STRING(GR_COMBINE_LOCAL_DEPTH);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMBOTHER (GrCombineOther_t cother)
{
 switch (cother) {
        TRAP_CASE_STRING(GR_COMBINE_OTHER_ITERATED);
        TRAP_CASE_STRING(GR_COMBINE_OTHER_TEXTURE);
        TRAP_CASE_STRING(GR_COMBINE_OTHER_CONSTANT);
        /*TRAP_CASE_STRING(GR_COMBINE_OTHER_NONE); ==GR_COMBINE_OTHER_CONSTANT*/
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMPFUNC (GrCmpFnc_t function)
{
 switch (function) {
        TRAP_CASE_STRING(GR_CMP_NEVER);
        TRAP_CASE_STRING(GR_CMP_LESS);
        TRAP_CASE_STRING(GR_CMP_EQUAL);
        TRAP_CASE_STRING(GR_CMP_LEQUAL);
        TRAP_CASE_STRING(GR_CMP_GREATER);
        TRAP_CASE_STRING(GR_CMP_NOTEQUAL);
        TRAP_CASE_STRING(GR_CMP_GEQUAL);
        TRAP_CASE_STRING(GR_CMP_ALWAYS);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CKMODE (GrChromakeyMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_CHROMAKEY_DISABLE);
        TRAP_CASE_STRING(GR_CHROMAKEY_ENABLE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CULLMODE (GrCullMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_CULL_DISABLE);
        TRAP_CASE_STRING(GR_CULL_NEGATIVE);
        TRAP_CASE_STRING(GR_CULL_POSITIVE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_DEPTHMODE (GrDepthBufferMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_DEPTHBUFFER_DISABLE);
        TRAP_CASE_STRING(GR_DEPTHBUFFER_ZBUFFER);
        TRAP_CASE_STRING(GR_DEPTHBUFFER_WBUFFER);
        TRAP_CASE_STRING(GR_DEPTHBUFFER_ZBUFFER_COMPARE_TO_BIAS);
        TRAP_CASE_STRING(GR_DEPTHBUFFER_WBUFFER_COMPARE_TO_BIAS);
        TRAP_NODEFAULT;
 }
}

const char *TRP_DITHERMODE (GrDitherMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_DITHER_DISABLE);
        TRAP_CASE_STRING(GR_DITHER_2x2);
        TRAP_CASE_STRING(GR_DITHER_4x4);
        TRAP_NODEFAULT;
 }
}

const char *TRP_FOGMODE (GrFogMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_FOG_DISABLE);
        TRAP_CASE_STRING(GR_FOG_WITH_TABLE_ON_FOGCOORD_EXT);
        TRAP_CASE_STRING(GR_FOG_WITH_TABLE_ON_Q);
        /*TRAP_CASE_STRING(GR_FOG_WITH_TABLE_ON_W); ==GR_FOG_WITH_TABLE_ON_Q*/
        TRAP_CASE_STRING(GR_FOG_WITH_ITERATED_Z);
        TRAP_CASE_STRING(GR_FOG_WITH_ITERATED_ALPHA_EXT);
        TRAP_CASE_STRING(GR_FOG_MULT2);
        TRAP_CASE_STRING(GR_FOG_ADD2);
        TRAP_NODEFAULT;
 }
}

const char *TRP_GETNAME (FxU32 pname)
{
 switch (pname) {
        TRAP_CASE_STRING(GR_BITS_DEPTH);
        TRAP_CASE_STRING(GR_BITS_RGBA);
        TRAP_CASE_STRING(GR_FIFO_FULLNESS);
        TRAP_CASE_STRING(GR_FOG_TABLE_ENTRIES);
        TRAP_CASE_STRING(GR_GAMMA_TABLE_ENTRIES);
        TRAP_CASE_STRING(GR_GLIDE_STATE_SIZE);
        TRAP_CASE_STRING(GR_GLIDE_VERTEXLAYOUT_SIZE);
        TRAP_CASE_STRING(GR_IS_BUSY);
        TRAP_CASE_STRING(GR_LFB_PIXEL_PIPE);
        TRAP_CASE_STRING(GR_MAX_TEXTURE_SIZE);
        TRAP_CASE_STRING(GR_MAX_TEXTURE_ASPECT_RATIO);
        TRAP_CASE_STRING(GR_MEMORY_FB);
        TRAP_CASE_STRING(GR_MEMORY_TMU);
        TRAP_CASE_STRING(GR_MEMORY_UMA);
        TRAP_CASE_STRING(GR_NUM_BOARDS);
        TRAP_CASE_STRING(GR_NON_POWER_OF_TWO_TEXTURES);
        TRAP_CASE_STRING(GR_NUM_FB);
        TRAP_CASE_STRING(GR_NUM_SWAP_HISTORY_BUFFER);
        TRAP_CASE_STRING(GR_NUM_TMU);
        TRAP_CASE_STRING(GR_PENDING_BUFFERSWAPS);
        TRAP_CASE_STRING(GR_REVISION_FB);
        TRAP_CASE_STRING(GR_REVISION_TMU);
        TRAP_CASE_STRING(GR_STATS_LINES);
        TRAP_CASE_STRING(GR_STATS_PIXELS_AFUNC_FAIL);
        TRAP_CASE_STRING(GR_STATS_PIXELS_CHROMA_FAIL);
        TRAP_CASE_STRING(GR_STATS_PIXELS_DEPTHFUNC_FAIL);
        TRAP_CASE_STRING(GR_STATS_PIXELS_IN);
        TRAP_CASE_STRING(GR_STATS_PIXELS_OUT);
        TRAP_CASE_STRING(GR_STATS_PIXELS);
        TRAP_CASE_STRING(GR_STATS_POINTS);
        TRAP_CASE_STRING(GR_STATS_TRIANGLES_IN);
        TRAP_CASE_STRING(GR_STATS_TRIANGLES_OUT);
        TRAP_CASE_STRING(GR_STATS_TRIANGLES);
        TRAP_CASE_STRING(GR_SWAP_HISTORY);
        TRAP_CASE_STRING(GR_SUPPORTS_PASSTHRU);
        TRAP_CASE_STRING(GR_TEXTURE_ALIGN);
        TRAP_CASE_STRING(GR_VIDEO_POSITION);
        TRAP_CASE_STRING(GR_VIEWPORT);
        TRAP_CASE_STRING(GR_WDEPTH_MIN_MAX);
        TRAP_CASE_STRING(GR_ZDEPTH_MIN_MAX);
        TRAP_CASE_STRING(GR_VERTEX_PARAMETER);
        TRAP_CASE_STRING(GR_BITS_GAMMA);
        TRAP_CASE_STRING(GR_GET_RESERVED_1);
        TRAP_NODEFAULT;
 }
}

const char *TRP_GETSTRING (FxU32 pname)
{
 switch (pname) {
        TRAP_CASE_STRING(GR_EXTENSION);
        TRAP_CASE_STRING(GR_HARDWARE);
        TRAP_CASE_STRING(GR_RENDERER);
        TRAP_CASE_STRING(GR_VENDOR);
        TRAP_CASE_STRING(GR_VERSION);
        TRAP_NODEFAULT;
 }
}

const char *TRP_ENABLE (GrEnableMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_AA_ORDERED);
        TRAP_CASE_STRING(GR_ALLOW_MIPMAP_DITHER);
        TRAP_CASE_STRING(GR_PASSTHRU);
        TRAP_CASE_STRING(GR_SHAMELESS_PLUG);
        TRAP_CASE_STRING(GR_VIDEO_SMOOTHING);
        TRAP_CASE_STRING(GR_TEXTURE_UMA_EXT);
        TRAP_CASE_STRING(GR_STENCIL_MODE_EXT);
        TRAP_CASE_STRING(GR_OPENGL_MODE_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_COORD (GrCoordinateSpaceMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_WINDOW_COORDS);
        TRAP_CASE_STRING(GR_CLIP_COORDS);
        TRAP_NODEFAULT;
 }
}

const char *TRP_STIPPLEMODE (GrStippleMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_STIPPLE_DISABLE);
        TRAP_CASE_STRING(GR_STIPPLE_PATTERN);
        TRAP_CASE_STRING(GR_STIPPLE_ROTATE);
        TRAP_NODEFAULT;
 }
}

const char *TRP_LODLEVEL (GrLOD_t lod)
{
 switch (lod) {
        TRAP_CASE_STRING(GR_LOD_LOG2_2048);
        TRAP_CASE_STRING(GR_LOD_LOG2_1024);
        TRAP_CASE_STRING(GR_LOD_LOG2_512);
        TRAP_CASE_STRING(GR_LOD_LOG2_256);
        TRAP_CASE_STRING(GR_LOD_LOG2_128);
        TRAP_CASE_STRING(GR_LOD_LOG2_64);
        TRAP_CASE_STRING(GR_LOD_LOG2_32);
        TRAP_CASE_STRING(GR_LOD_LOG2_16);
        TRAP_CASE_STRING(GR_LOD_LOG2_8);
        TRAP_CASE_STRING(GR_LOD_LOG2_4);
        TRAP_CASE_STRING(GR_LOD_LOG2_2);
        TRAP_CASE_STRING(GR_LOD_LOG2_1);
        TRAP_NODEFAULT;
 }
}

const char *TRP_ASPECTRATIO (GrAspectRatio_t aspect)
{
 switch (aspect) {
        TRAP_CASE_STRING(GR_ASPECT_LOG2_8x1);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_4x1);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_2x1);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_1x1);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_1x2);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_1x4);
        TRAP_CASE_STRING(GR_ASPECT_LOG2_1x8);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TEXFMT (GrTextureFormat_t fmt)
{
 switch (fmt) {
        TRAP_CASE_STRING(GR_TEXFMT_8BIT);
        /*TRAP_CASE_STRING(GR_TEXFMT_RGB_332); ==GR_TEXFMT_8BIT*/
        TRAP_CASE_STRING(GR_TEXFMT_YIQ_422);
        TRAP_CASE_STRING(GR_TEXFMT_ALPHA_8);
        TRAP_CASE_STRING(GR_TEXFMT_INTENSITY_8);
        TRAP_CASE_STRING(GR_TEXFMT_ALPHA_INTENSITY_44);
        TRAP_CASE_STRING(GR_TEXFMT_P_8);
        TRAP_CASE_STRING(GR_TEXFMT_RSVD0);
        /*TRAP_CASE_STRING(GR_TEXFMT_P_8_6666); ==GR_TEXFMT_RSVD0*/
        /*TRAP_CASE_STRING(GR_TEXFMT_P_8_6666_EXT); ==GR_TEXFMT_RSVD0*/
        TRAP_CASE_STRING(GR_TEXFMT_RSVD1);
        TRAP_CASE_STRING(GR_TEXFMT_16BIT);
        /*TRAP_CASE_STRING(GR_TEXFMT_ARGB_8332); ==GR_TEXFMT_16BIT*/
        TRAP_CASE_STRING(GR_TEXFMT_AYIQ_8422);
        TRAP_CASE_STRING(GR_TEXFMT_RGB_565);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_1555);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_4444);
        TRAP_CASE_STRING(GR_TEXFMT_ALPHA_INTENSITY_88);
        TRAP_CASE_STRING(GR_TEXFMT_AP_88);
        TRAP_CASE_STRING(GR_TEXFMT_RSVD2);
        /*TRAP_CASE_STRING(GR_TEXFMT_RSVD4); ==GR_TEXFMT_RSVD2*/
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_FXT1);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_8888);
        TRAP_CASE_STRING(GR_TEXFMT_YUYV_422);
        TRAP_CASE_STRING(GR_TEXFMT_UYVY_422);
        TRAP_CASE_STRING(GR_TEXFMT_AYUV_444);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_DXT1);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_DXT2);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_DXT3);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_DXT4);
        TRAP_CASE_STRING(GR_TEXFMT_ARGB_CMP_DXT5);
        TRAP_CASE_STRING(GR_TEXTFMT_RGB_888);
        TRAP_NODEFAULT;
 }
}

const char *TRP_EVENODD (FxU32 evenOdd)
{
 switch (evenOdd) {
        TRAP_CASE_STRING(GR_MIPMAPLEVELMASK_EVEN);
        TRAP_CASE_STRING(GR_MIPMAPLEVELMASK_ODD);
        TRAP_CASE_STRING(GR_MIPMAPLEVELMASK_BOTH);
        TRAP_NODEFAULT;
 }
}

const char *TRP_NCC (GrNCCTable_t table)
{
 switch (table) {
        TRAP_CASE_STRING(GR_NCCTABLE_NCC0);
        TRAP_CASE_STRING(GR_NCCTABLE_NCC1);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CLAMPMODE (GrTextureClampMode_t clampmode)
{
 switch (clampmode) {
        TRAP_CASE_STRING(GR_TEXTURECLAMP_WRAP);
        TRAP_CASE_STRING(GR_TEXTURECLAMP_CLAMP);
        TRAP_CASE_STRING(GR_TEXTURECLAMP_MIRROR_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TEXFILTER (GrTextureFilterMode_t filter_mode)
{
 switch (filter_mode) {
        TRAP_CASE_STRING(GR_TEXTUREFILTER_POINT_SAMPLED);
        TRAP_CASE_STRING(GR_TEXTUREFILTER_BILINEAR);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TABLE (GrTexTable_t type)
{
 switch (type) {
        TRAP_CASE_STRING(GR_TEXTABLE_NCC0);
        TRAP_CASE_STRING(GR_TEXTABLE_NCC1);
        TRAP_CASE_STRING(GR_TEXTABLE_PALETTE);
        TRAP_CASE_STRING(GR_TEXTABLE_PALETTE_6666_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_MIPMODE (GrMipMapMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_MIPMAP_DISABLE);
        TRAP_CASE_STRING(GR_MIPMAP_NEAREST);
        TRAP_CASE_STRING(GR_MIPMAP_NEAREST_DITHER);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TEXBASERANGE (GrTexBaseRange_t range)
{
 switch (range) {
        TRAP_CASE_STRING(GR_TEXBASE_2048);
        TRAP_CASE_STRING(GR_TEXBASE_1024);
        TRAP_CASE_STRING(GR_TEXBASE_512);
        TRAP_CASE_STRING(GR_TEXBASE_256_TO_1);
        TRAP_CASE_STRING(GR_TEXBASE_256);
        TRAP_CASE_STRING(GR_TEXBASE_128);
        TRAP_CASE_STRING(GR_TEXBASE_64);
        TRAP_CASE_STRING(GR_TEXBASE_32_TO_1);
        TRAP_NODEFAULT;
 }
}

const char *TRP_LOCKTYPE (GrLock_t type)
{
 switch (type) {
        TRAP_CASE_STRING(GR_LFB_READ_ONLY);
        TRAP_CASE_STRING(GR_LFB_WRITE_ONLY);
        /*TRAP_CASE_STRING(GR_LFB_IDLE); ==GR_LFB_READ_ONLY*/
        TRAP_CASE_STRING(GR_LFB_NOIDLE);
        TRAP_CASE_STRING(GR_LFB_WRITE_ONLY_EXPLICIT_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_WRITEMODE (GrLfbWriteMode_t writeMode)
{
 switch (writeMode) {
        TRAP_CASE_STRING(GR_LFBWRITEMODE_565);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_555);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_1555);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED1);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_888);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_8888);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED2);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED3);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_Z32);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED5);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED6);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_RESERVED7);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_565_DEPTH);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_555_DEPTH);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_1555_DEPTH);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_ZA16);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_ANY);
        TRAP_NODEFAULT;
 }
}

const char *TRP_SRCFMT (GrLfbSrcFmt_t src_format)
{
 switch (src_format) {
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_565);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_555);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_1555);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_888);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_8888);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_565_DEPTH);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_555_DEPTH);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_1555_DEPTH);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_ZA16);
        TRAP_CASE_STRING(GR_LFB_SRC_FMT_RLE16);
        TRAP_CASE_STRING(GR_LFBWRITEMODE_Z32); /*???*/
        TRAP_NODEFAULT;
 }
}

const char *TRP_CRMODE (GrChromaRangeMode_t mode)
{
 switch (mode) {
        TRAP_CASE_STRING(GR_CHROMARANGE_DISABLE_EXT);
        /*TRAP_CASE_STRING(GR_CHROMARANGE_RGB_ALL_EXT); ==GR_CHROMARANGE_DISABLE_EXT*/
        TRAP_CASE_STRING(GR_CHROMARANGE_ENABLE_EXT);
        TRAP_NODEFAULT;
 }
}

const char *TRP_PIXFMT (GrPixelFormat_t pixelformat)
{
 switch (pixelformat) {
        TRAP_CASE_STRING(GR_PIXFMT_I_8);
        TRAP_CASE_STRING(GR_PIXFMT_AI_88);
        TRAP_CASE_STRING(GR_PIXFMT_RGB_565);
        TRAP_CASE_STRING(GR_PIXFMT_ARGB_1555);
        TRAP_CASE_STRING(GR_PIXFMT_ARGB_8888);
        TRAP_CASE_STRING(GR_PIXFMT_AA_2_RGB_565);
        TRAP_CASE_STRING(GR_PIXFMT_AA_2_ARGB_1555);
        TRAP_CASE_STRING(GR_PIXFMT_AA_2_ARGB_8888);
        TRAP_CASE_STRING(GR_PIXFMT_AA_4_RGB_565);
        TRAP_CASE_STRING(GR_PIXFMT_AA_4_ARGB_1555);
        TRAP_CASE_STRING(GR_PIXFMT_AA_4_ARGB_8888);
        TRAP_CASE_STRING(GR_PIXFMT_AA_8_RGB_565);
        TRAP_CASE_STRING(GR_PIXFMT_AA_8_ARGB_1555);
        TRAP_CASE_STRING(GR_PIXFMT_AA_8_ARGB_8888);
        TRAP_NODEFAULT;
 }
}

const char *TRP_STENCILOP (GrStencilOp_t op)
{
 switch (op) {
        TRAP_CASE_STRING(GR_STENCILOP_KEEP);
        TRAP_CASE_STRING(GR_STENCILOP_ZERO);
        TRAP_CASE_STRING(GR_STENCILOP_REPLACE);
        TRAP_CASE_STRING(GR_STENCILOP_INCR_CLAMP);
        TRAP_CASE_STRING(GR_STENCILOP_DECR_CLAMP);
        TRAP_CASE_STRING(GR_STENCILOP_INVERT);
        TRAP_CASE_STRING(GR_STENCILOP_INCR_WRAP);
        TRAP_CASE_STRING(GR_STENCILOP_DECR_WRAP);
        TRAP_NODEFAULT;
 }
}

const char *TRP_BLENDOP (GrAlphaBlendOp_t op)
{
 switch (op) {
        TRAP_CASE_STRING(GR_BLEND_OP_ADD);
        TRAP_CASE_STRING(GR_BLEND_OP_SUB);
        TRAP_CASE_STRING(GR_BLEND_OP_REVSUB);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CU (GrCCUColor_t a)
{
 switch (a) {
        TRAP_CASE_STRING(GR_CMBX_ZERO);
        TRAP_CASE_STRING(GR_CMBX_TEXTURE_ALPHA);
        TRAP_CASE_STRING(GR_CMBX_ALOCAL);
        TRAP_CASE_STRING(GR_CMBX_AOTHER);
        TRAP_CASE_STRING(GR_CMBX_B);
        TRAP_CASE_STRING(GR_CMBX_CONSTANT_ALPHA);
        TRAP_CASE_STRING(GR_CMBX_CONSTANT_COLOR);
        TRAP_CASE_STRING(GR_CMBX_DETAIL_FACTOR);
        TRAP_CASE_STRING(GR_CMBX_ITALPHA);
        TRAP_CASE_STRING(GR_CMBX_ITRGB);
        TRAP_CASE_STRING(GR_CMBX_LOCAL_TEXTURE_ALPHA);
        TRAP_CASE_STRING(GR_CMBX_LOCAL_TEXTURE_RGB);
        TRAP_CASE_STRING(GR_CMBX_LOD_FRAC);
        TRAP_CASE_STRING(GR_CMBX_OTHER_TEXTURE_ALPHA);
        TRAP_CASE_STRING(GR_CMBX_OTHER_TEXTURE_RGB);
        TRAP_CASE_STRING(GR_CMBX_TEXTURE_RGB);
        TRAP_CASE_STRING(GR_CMBX_TMU_CALPHA);
        TRAP_CASE_STRING(GR_CMBX_TMU_CCOLOR);
        TRAP_NODEFAULT;
 }
}

const char *TRP_CMBMODE (GrCombineMode_t a_mode)
{
 switch (a_mode) {
        TRAP_CASE_STRING(GR_FUNC_MODE_ZERO);
        TRAP_CASE_STRING(GR_FUNC_MODE_X);
        TRAP_CASE_STRING(GR_FUNC_MODE_ONE_MINUS_X);
        TRAP_CASE_STRING(GR_FUNC_MODE_NEGATIVE_X);
        TRAP_CASE_STRING(GR_FUNC_MODE_X_MINUS_HALF);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TMU (GrChipID_t tmu)
{
 switch (tmu) {
        TRAP_CASE_STRING(GR_TMU0);
        TRAP_CASE_STRING(GR_TMU1);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TXDITHER (FxU32 dither)
{
 switch (dither) {
        TRAP_CASE_STRING(TX_DITHER_NONE);
        TRAP_CASE_STRING(TX_DITHER_4x4);
        TRAP_CASE_STRING(TX_DITHER_ERR);
        TRAP_NODEFAULT;
 }
}

const char *TRP_TXCOMPRESS (FxU32 compress)
{
 switch (compress) {
        TRAP_CASE_STRING(TX_COMPRESSION_STATISTICAL);
        TRAP_CASE_STRING(TX_COMPRESSION_HEURISTIC);
        TRAP_NODEFAULT;
 }
}



/****************************************************************************\
* REAL POINTERS                                                              *
\****************************************************************************/

/*
** glide extensions
*/
void (FX_CALL *real_grSetNumPendingBuffers) (FxI32 NumPendingBuffers);
char * (FX_CALL *real_grGetRegistryOrEnvironmentStringExt) (char *theEntry);
void (FX_CALL *real_grGetGammaTableExt) (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue);
void (FX_CALL *real_grChromaRangeModeExt) (GrChromakeyMode_t mode);
void (FX_CALL *real_grChromaRangeExt) (GrColor_t color, GrColor_t range, GrChromaRangeMode_t match_mode);
void (FX_CALL *real_grTexChromaModeExt) (GrChipID_t tmu, GrChromakeyMode_t mode);
void (FX_CALL *real_grTexChromaRangeExt) (GrChipID_t tmu, GrColor_t min, GrColor_t max, GrTexChromakeyMode_t mode);

/* pointcast */
void (FX_CALL *real_grTexDownloadTableExt) (GrChipID_t tmu, GrTexTable_t type, void *data);
void (FX_CALL *real_grTexDownloadTablePartialExt) (GrChipID_t tmu, GrTexTable_t type, void *data, int start, int end);
void (FX_CALL *real_grTexNCCTableExt) (GrChipID_t tmu, GrNCCTable_t table);

/* tbext */
void (FX_CALL *real_grTextureBufferExt) (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask);
void (FX_CALL *real_grTextureAuxBufferExt) (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask);
void (FX_CALL *real_grAuxBufferExt) (GrBuffer_t buffer);

/* napalm */
GrContext_t (FX_CALL *real_grSstWinOpenExt) (FxU32 hWnd, GrScreenResolution_t resolution, GrScreenRefresh_t refresh, GrColorFormat_t format, GrOriginLocation_t origin, GrPixelFormat_t pixelformat, int nColBuffers, int nAuxBuffers);
void (FX_CALL *real_grStencilFuncExt) (GrCmpFnc_t fnc, GrStencil_t ref, GrStencil_t mask);
void (FX_CALL *real_grStencilMaskExt) (GrStencil_t value);
void (FX_CALL *real_grStencilOpExt) (GrStencilOp_t stencil_fail, GrStencilOp_t depth_fail, GrStencilOp_t depth_pass);
void (FX_CALL *real_grLfbConstantStencilExt) (GrStencil_t value);
void (FX_CALL *real_grBufferClearExt) (GrColor_t color, GrAlpha_t alpha, FxU32 depth, GrStencil_t stencil);
void (FX_CALL *real_grColorCombineExt) (GrCCUColor_t a, GrCombineMode_t a_mode, GrCCUColor_t b, GrCombineMode_t b_mode, GrCCUColor_t c, FxBool c_invert, GrCCUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
void (FX_CALL *real_grAlphaCombineExt) (GrACUColor_t a, GrCombineMode_t a_mode, GrACUColor_t b, GrCombineMode_t b_mode, GrACUColor_t c, FxBool c_invert, GrACUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
void (FX_CALL *real_grTexColorCombineExt) (GrChipID_t tmu, GrTCCUColor_t a, GrCombineMode_t a_mode, GrTCCUColor_t b, GrCombineMode_t b_mode, GrTCCUColor_t c, FxBool c_invert, GrTCCUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
void (FX_CALL *real_grTexAlphaCombineExt) (GrChipID_t tmu, GrTACUColor_t a, GrCombineMode_t a_mode, GrTACUColor_t b, GrCombineMode_t b_mode, GrTACUColor_t c, FxBool c_invert, GrTACUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
void (FX_CALL *real_grConstantColorValueExt) (GrChipID_t tmu, GrColor_t value);
void (FX_CALL *real_grColorMaskExt) (FxBool r, FxBool g, FxBool b, FxBool a);
void (FX_CALL *real_grAlphaBlendFunctionExt) (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendOp_t rgb_op, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df, GrAlphaBlendOp_t alpha_op);
void (FX_CALL *real_grTBufferWriteMaskExt) (FxU32 tmask);

/*
** texus
*/
void (FX_CALL *real_txImgQuantize) (char *dst, char *src, int w, int h, FxU32 format, FxU32 dither);
void (FX_CALL *real_txMipQuantize) (TxMip *pxMip, TxMip *txMip, int fmt, FxU32 d, FxU32 comp);
void (FX_CALL *real_txPalToNcc) (GuNccTable *ncc_table, const FxU32 *pal);



/****************************************************************************\
* DEBUG HOOKS                                                                *
\****************************************************************************/

/*
** rendering functions
*/
void FX_CALL trap_grDrawPoint (const void *pt)
{
#define FN_NAME "grDrawPoint"
 TRAP_LOG("%s(%p)\n", FN_NAME, pt);
 grDrawPoint(pt);
#undef FN_NAME
}

void FX_CALL trap_grDrawLine (const void *v1,
                              const void *v2)
{
#define FN_NAME "grDrawLine"
 TRAP_LOG("%s(%p, %p)\n", FN_NAME, v1, v2);
 grDrawLine(v1, v2);
#undef FN_NAME
}

void FX_CALL trap_grDrawTriangle (const void *a,
                                  const void *b,
                                  const void *c)
{
#define FN_NAME "grDrawTriangle"
 TRAP_LOG("%s(%p, %p, %p)\n", FN_NAME, a, b, c);
 grDrawTriangle(a, b, c);
#undef FN_NAME
}

void FX_CALL trap_grVertexLayout (FxU32 param,
                                  FxI32 offset,
                                  FxU32 mode)
{
#define FN_NAME "grVertexLayout"
 TRAP_LOG("%s(%s, %ld, %s)\n", FN_NAME, TRP_VTX(param), offset, TRP_PARAM(mode));
 grVertexLayout(param, offset, mode);
#undef FN_NAME
}

void FX_CALL trap_grDrawVertexArray (FxU32 mode,
                                     FxU32 Count,
                                     void  *pointers)
{
#define FN_NAME "grDrawVertexArray"
 TRAP_LOG("%s(%s, %lu, %p)\n", FN_NAME, TRP_ARRAY(mode), Count, pointers);
 grDrawVertexArray(mode, Count, pointers);
#undef FN_NAME
}

void FX_CALL trap_grDrawVertexArrayContiguous (FxU32 mode,
                                               FxU32 Count,
                                               void  *pointers,
                                               FxU32 stride)
{
#define FN_NAME "grDrawVertexArrayContiguous"
 TRAP_LOG("%s(%s, %lu, %p, %lu)\n", FN_NAME, TRP_ARRAY(mode), Count, pointers, stride);
 grDrawVertexArrayContiguous(mode, Count, pointers, stride);
#undef FN_NAME
}

/*
**  Antialiasing Functions
*/
void FX_CALL trap_grAADrawTriangle (const void *a,
                                    const void *b,
                                    const void *c,
                                    FxBool     ab_antialias,
                                    FxBool     bc_antialias,
                                    FxBool     ca_antialias)
{
#define FN_NAME "grAADrawTriangle"
 TRAP_LOG("%s(%p, %p, %p, %s, %s, %s)\n", FN_NAME, a, b, c, TRP_BOOL(ab_antialias), TRP_BOOL(bc_antialias), TRP_BOOL(ca_antialias));
 grAADrawTriangle(a, b, c, ab_antialias, bc_antialias, ca_antialias);
#undef FN_NAME
}

/*
** buffer management
*/
void FX_CALL trap_grBufferClear (GrColor_t color,
                                 GrAlpha_t alpha,
                                 FxU32     depth)
{
#define FN_NAME "grBufferClear"
 TRAP_LOG("%s(%08lx, %02x, %08lx)\n", FN_NAME, color, alpha, depth);
 grBufferClear(color, alpha, depth);
#undef FN_NAME
}

void FX_CALL trap_grBufferSwap (FxU32 swap_interval)
{
#define FN_NAME "grBufferSwap"
 TRAP_LOG("%s(%lu)\n", FN_NAME, swap_interval);
 grBufferSwap(swap_interval);
#undef FN_NAME
}

void FX_CALL trap_grRenderBuffer (GrBuffer_t buffer)
{
#define FN_NAME "grRenderBuffer"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_BUFFER(buffer));
 grRenderBuffer(buffer);
#undef FN_NAME
}

/*
** error management
*/
void FX_CALL trap_grErrorSetCallback (GrErrorCallbackFnc_t fnc)
{
#define FN_NAME "grErrorSetCallback"
 TRAP_LOG("%s(%p)\n", FN_NAME, (void *)fnc);
 grErrorSetCallback(fnc);
#undef FN_NAME
}

/*
** SST routines
*/
void FX_CALL trap_grFinish (void)
{
#define FN_NAME "grFinish"
 TRAP_LOG("%s()\n", FN_NAME);
 grFinish();
#undef FN_NAME
}

void FX_CALL trap_grFlush (void)
{
#define FN_NAME "grFlush"
 TRAP_LOG("%s()\n", FN_NAME);
 grFlush();
#undef FN_NAME
}

GrContext_t FX_CALL trap_grSstWinOpen (FxU32                hWnd,
                                       GrScreenResolution_t screen_resolution,
                                       GrScreenRefresh_t    refresh_rate,
                                       GrColorFormat_t      color_format,
                                       GrOriginLocation_t   origin_location,
                                       int                  nColBuffers,
                                       int                  nAuxBuffers)
{
#define FN_NAME "grSstWinOpen"
 GrContext_t rv;
 TRAP_LOG("%s(%08lx, %s, %s, %s, %s, %d, %d)\n", FN_NAME, hWnd, TRP_RESOLUTION(screen_resolution), TRP_REFRESH(refresh_rate), TRP_COLFMT(color_format), TRP_ORIGIN(origin_location), nColBuffers, nAuxBuffers);
 rv = grSstWinOpen(hWnd, screen_resolution, refresh_rate, color_format, origin_location, nColBuffers, nAuxBuffers);
 TRAP_LOG(GOT "%p\n", (void *)rv);
 return rv;
#undef FN_NAME
}

FxBool FX_CALL trap_grSstWinClose (GrContext_t context)
{
#define FN_NAME "grSstWinClose"
 FxBool rv;
 TRAP_LOG("%s(%p)\n", FN_NAME, (void *)context);
 rv = grSstWinClose(context);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

FxBool FX_CALL trap_grSelectContext (GrContext_t context)
{
#define FN_NAME "grSelectContext"
 FxBool rv;
 TRAP_LOG("%s(%p)\n", FN_NAME, (void *)context);
 rv = grSelectContext(context);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grSstOrigin (GrOriginLocation_t origin)
{
#define FN_NAME "grSstOrigin"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_ORIGIN(origin));
 grSstOrigin(origin);
#undef FN_NAME
}

void FX_CALL trap_grSstSelect (int which_sst)
{
#define FN_NAME "grSstSelect"
 TRAP_LOG("%s(%d)\n", FN_NAME, which_sst);
 grSstSelect(which_sst);
#undef FN_NAME
}

/*
** Glide configuration and special effect maintenance functions
*/
void FX_CALL trap_grAlphaBlendFunction (GrAlphaBlendFnc_t rgb_sf,
                                        GrAlphaBlendFnc_t rgb_df,
                                        GrAlphaBlendFnc_t alpha_sf,
                                        GrAlphaBlendFnc_t alpha_df)
{
#define FN_NAME "grAlphaBlendFunction"
 TRAP_LOG("%s(%s, %s, %s, %s)\n", FN_NAME, TRP_BLEND(rgb_sf), TRP_BLEND(rgb_df), TRP_BLEND(alpha_sf), TRP_BLEND(alpha_df));
 grAlphaBlendFunction(rgb_sf, rgb_df, alpha_sf, alpha_df);
#undef FN_NAME
}

void FX_CALL trap_grAlphaCombine (GrCombineFunction_t function,
                                  GrCombineFactor_t   factor,
                                  GrCombineLocal_t    local,
                                  GrCombineOther_t    other,
                                  FxBool              invert)
{
#define FN_NAME "grAlphaCombine"
 TRAP_LOG("%s(%s, %s, %s, %s, %s)\n", FN_NAME, TRP_CMBFUNC(function), TRP_CMBFACT(factor), TRP_CMBLOCAL(local), TRP_CMBOTHER(other), TRP_BOOL(invert));
 grAlphaCombine(function, factor, local, other, invert);
#undef FN_NAME
}

void FX_CALL trap_grAlphaControlsITRGBLighting (FxBool enable)
{
#define FN_NAME "grAlphaControlsITRGBLighting"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_BOOL(enable));
 grAlphaControlsITRGBLighting(enable);
#undef FN_NAME
}

void FX_CALL trap_grAlphaTestFunction (GrCmpFnc_t function)
{
#define FN_NAME "grAlphaTestFunction"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_CMPFUNC(function));
 grAlphaTestFunction(function);
#undef FN_NAME
}

void FX_CALL trap_grAlphaTestReferenceValue (GrAlpha_t value)
{
#define FN_NAME "grAlphaTestReferenceValue"
 TRAP_LOG("%s(%02x)\n", FN_NAME, value);
 grAlphaTestReferenceValue(value);
#undef FN_NAME
}

void FX_CALL trap_grChromakeyMode (GrChromakeyMode_t mode)
{
#define FN_NAME "grChromakeyMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_CKMODE(mode));
 grChromakeyMode(mode);
#undef FN_NAME
}

void FX_CALL trap_grChromakeyValue (GrColor_t value)
{
#define FN_NAME "grChromakeyValue"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, value);
 grChromakeyValue(value);
#undef FN_NAME
}

void FX_CALL trap_grClipWindow (FxU32 minx,
                                FxU32 miny,
                                FxU32 maxx,
                                FxU32 maxy)
{
#define FN_NAME "grClipWindow"
 TRAP_LOG("%s(%lu, %lu, %lu, %lu)\n", FN_NAME, minx, miny, maxx, maxy);
 grClipWindow(minx, miny, maxx, maxy);
#undef FN_NAME
}

void FX_CALL trap_grColorCombine (GrCombineFunction_t function,
                                  GrCombineFactor_t   factor,
                                  GrCombineLocal_t    local,
                                  GrCombineOther_t    other,
                                  FxBool              invert)
{
#define FN_NAME "grColorCombine"
 TRAP_LOG("%s(%s, %s, %s, %s, %s)\n", FN_NAME, TRP_CMBFUNC(function), TRP_CMBFACT(factor), TRP_CMBLOCAL(local), TRP_CMBOTHER(other), TRP_BOOL(invert));
 grColorCombine(function, factor, local, other, invert);
#undef FN_NAME
}

void FX_CALL trap_grColorMask (FxBool rgb,
                               FxBool a)
{
#define FN_NAME "grColorMask"
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_BOOL(rgb), TRP_BOOL(a));
 grColorMask(rgb, a);
#undef FN_NAME
}

void FX_CALL trap_grCullMode (GrCullMode_t mode)
{
#define FN_NAME "grCullMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_CULLMODE(mode));
 grCullMode(mode);
#undef FN_NAME
}

void FX_CALL trap_grConstantColorValue (GrColor_t value)
{
#define FN_NAME "grConstantColorValue"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, value);
 grConstantColorValue(value);
#undef FN_NAME
}

void FX_CALL trap_grDepthBiasLevel (FxI32 level)
{
#define FN_NAME "grDepthBiasLevel"
 TRAP_LOG("%s(%ld)\n", FN_NAME, level);
 grDepthBiasLevel(level);
#undef FN_NAME
}

void FX_CALL trap_grDepthBufferFunction (GrCmpFnc_t function)
{
#define FN_NAME "grDepthBufferFunction"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_CMPFUNC(function));
 grDepthBufferFunction(function);
#undef FN_NAME
}

void FX_CALL trap_grDepthBufferMode (GrDepthBufferMode_t mode)
{
#define FN_NAME "grDepthBufferMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_DEPTHMODE(mode));
 grDepthBufferMode(mode);
#undef FN_NAME
}

void FX_CALL trap_grDepthMask (FxBool mask)
{
#define FN_NAME "grDepthMask"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_BOOL(mask));
 grDepthMask(mask);
#undef FN_NAME
}

void FX_CALL trap_grDisableAllEffects (void)
{
#define FN_NAME "grDisableAllEffects"
 TRAP_LOG("%s()\n", FN_NAME);
 grDisableAllEffects();
#undef FN_NAME
}

void FX_CALL trap_grDitherMode (GrDitherMode_t mode)
{
#define FN_NAME "grDitherMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_DITHERMODE(mode));
 grDitherMode(mode);
#undef FN_NAME
}

void FX_CALL trap_grFogColorValue (GrColor_t fogcolor)
{
#define FN_NAME "grFogColorValue"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, fogcolor);
 grFogColorValue(fogcolor);
#undef FN_NAME
}

void FX_CALL trap_grFogMode (GrFogMode_t mode)
{
#define FN_NAME "grFogMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_FOGMODE(mode));
 grFogMode(mode);
#undef FN_NAME
}

void FX_CALL trap_grFogTable (const GrFog_t ft[])
{
#define FN_NAME "grFogTable"
 TRAP_LOG("%s(%p)\n", FN_NAME, ft);
 grFogTable(ft);
#undef FN_NAME
}

void FX_CALL trap_grLoadGammaTable (FxU32 nentries,
                                    FxU32 *red,
                                    FxU32 *green,
                                    FxU32 *blue)
{
#define FN_NAME "grLoadGammaTable"
 TRAP_LOG("%s(%lu, %p, %p, %p)\n", FN_NAME, nentries, (void *)red, (void *)green, (void *)blue);
 grLoadGammaTable(nentries, red, green, blue);
#undef FN_NAME
}

void FX_CALL trap_grSplash (float x,
                            float y,
                            float width,
                            float height,
                            FxU32 frame)
{
#define FN_NAME "grSplash"
 TRAP_LOG("%s(%f, %f, %f, %f, %lu)\n", FN_NAME, x, y, width, height, frame);
 grSplash(x, y, width, height, frame);
#undef FN_NAME
}

FxU32 FX_CALL trap_grGet (FxU32 pname,
                          FxU32 plength,
                          FxI32 *params)
{
#define FN_NAME "grGet"
 FxU32 rv, i;
 TRAP_LOG("%s(%s, %lu, %p)\n", FN_NAME, TRP_GETNAME(pname), plength, (void *)params);
 rv = grGet(pname, plength, params);
 TRAP_LOG(GOT "[");
 for (i = 0; i < (rv/sizeof(FxI32)); i++) {
     TRAP_LOG("%s%ld", i ? ", " : "", params[i]);
 }
 TRAP_LOG("]\n");
 return rv;
#undef FN_NAME
}

const char *FX_CALL trap_grGetString (FxU32 pname)
{
#define FN_NAME "grGetString"
 const char *rv;
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_GETSTRING(pname));
 rv = grGetString(pname);
 if (rv) {
    TRAP_LOG(GOT "\"%s\"\n", rv);
 } else {
    TRAP_LOG(GOT "NULL\n");
 }
 return rv;
#undef FN_NAME
}

FxI32 FX_CALL trap_grQueryResolutions (const GrResolution *resTemplate,
                                       GrResolution       *output)
{
#define FN_NAME "grQueryResolutions"
 FxI32 rv;
 TRAP_LOG("%s(%p, %p)\n", FN_NAME, (void *)resTemplate, (void *)output);
 rv = grQueryResolutions(resTemplate, output);
 TRAP_LOG(GOT "%ld\n", rv);
 return rv;
#undef FN_NAME
}

FxBool FX_CALL trap_grReset (FxU32 what)
{
#define FN_NAME "grReset"
 FxBool rv;
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_GETNAME(what));
 rv = grReset(what);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

GrProc FX_CALL trap_grGetProcAddress (char *procName)
{
#define FN_NAME "grGetProcAddress"
 GrProc rv;
 TRAP_LOG("%s(%s)\n", FN_NAME, procName);
 rv = grGetProcAddress(procName);
 TRAP_LOG(GOT "%p\n", (void *)rv);
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grEnable (GrEnableMode_t mode)
{
#define FN_NAME "grEnable"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_ENABLE(mode));
 grEnable(mode);
#undef FN_NAME
}

void FX_CALL trap_grDisable (GrEnableMode_t mode)
{
#define FN_NAME "grDisable"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_ENABLE(mode));
 grDisable(mode);
#undef FN_NAME
}

void FX_CALL trap_grCoordinateSpace (GrCoordinateSpaceMode_t mode)
{
#define FN_NAME "grCoordinateSpace"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_COORD(mode));
 grCoordinateSpace(mode);
#undef FN_NAME
}

void FX_CALL trap_grDepthRange (FxFloat n,
                                FxFloat f)
{
#define FN_NAME "grDepthRange"
 TRAP_LOG("%s(%f, %f)\n", FN_NAME, n, f);
 grDepthRange(n, f);
#undef FN_NAME
}

void FX_CALL trap_grStippleMode (GrStippleMode_t mode)
{
#define FN_NAME "grStippleMode"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_STIPPLEMODE(mode));
 grStippleMode(mode); /* some Glide libs don't have it; not used anyway */
#undef FN_NAME
}

void FX_CALL trap_grStipplePattern (GrStipplePattern_t mode)
{
#define FN_NAME "grStipplePattern"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, mode);
 grStipplePattern(mode); /* some Glide libs don't have it; not used anyway */
#undef FN_NAME
}

void FX_CALL trap_grViewport (FxI32 x,
                              FxI32 y,
                              FxI32 width,
                              FxI32 height)
{
#define FN_NAME "grViewport"
 TRAP_LOG("%s(%ld, %ld, %ld, %ld)\n", FN_NAME, x, y, width, height);
 grViewport(x, y, width, height);
#undef FN_NAME
}

/*
** texture mapping control functions
*/
FxU32 FX_CALL trap_grTexCalcMemRequired (GrLOD_t           lodmin,
                                         GrLOD_t           lodmax,
                                         GrAspectRatio_t   aspect,
                                         GrTextureFormat_t fmt)
{
#define FN_NAME "grTexCalcMemRequired"
 FxU32 rv;
 TRAP_LOG("%s(%s, %s, %s, %s)\n", FN_NAME, TRP_LODLEVEL(lodmin), TRP_LODLEVEL(lodmax), TRP_ASPECTRATIO(aspect), TRP_TEXFMT(fmt));
 rv = grTexCalcMemRequired(lodmin, lodmax, aspect, fmt);
 TRAP_LOG(GOT "%lu\n", rv);
 return rv;
#undef FN_NAME
}

FxU32 FX_CALL trap_grTexTextureMemRequired (FxU32     evenOdd,
                                            GrTexInfo *info)
{
#define FN_NAME "grTexTextureMemRequired"
 FxU32 rv;
 TRAP_LOG("%s(%s, %p)\n", FN_NAME, TRP_EVENODD(evenOdd), (void *)info);
 rv = grTexTextureMemRequired(evenOdd, info);
 TRAP_LOG(GOT "%lu\n", rv);
 return rv;
#undef FN_NAME
}

FxU32 FX_CALL trap_grTexMinAddress (GrChipID_t tmu)
{
#define FN_NAME "grTexMinAddress"
 FxU32 rv;
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_TMU(tmu));
 rv = grTexMinAddress(tmu);
 TRAP_LOG(GOT "%lu\n", rv);
 return rv;
#undef FN_NAME
}

FxU32 FX_CALL trap_grTexMaxAddress (GrChipID_t tmu)
{
#define FN_NAME "grTexMaxAddress"
 FxU32 rv;
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_TMU(tmu));
 rv = grTexMaxAddress(tmu);
 TRAP_LOG(GOT "%lu\n", rv);
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grTexNCCTable (GrNCCTable_t table)
{
#define FN_NAME "grTexNCCTable"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_NCC(table));
 grTexNCCTable(table);
#undef FN_NAME
}

void FX_CALL trap_grTexSource (GrChipID_t tmu,
                               FxU32      startAddress,
                               FxU32      evenOdd,
                               GrTexInfo  *info)
{
#define FN_NAME "grTexSource"
 TRAP_LOG("%s(%s, %08lx, %s, %p)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_EVENODD(evenOdd), (void *)info);
 grTexSource(tmu, startAddress, evenOdd, info);
#undef FN_NAME
}

void FX_CALL trap_grTexClampMode (GrChipID_t           tmu,
                                  GrTextureClampMode_t s_clampmode,
                                  GrTextureClampMode_t t_clampmode)
{
#define FN_NAME "grTexClampMode"
 TRAP_LOG("%s(%s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_CLAMPMODE(s_clampmode), TRP_CLAMPMODE(t_clampmode));
 grTexClampMode(tmu, s_clampmode, t_clampmode);
#undef FN_NAME
}

void FX_CALL trap_grTexCombine (GrChipID_t          tmu,
                                GrCombineFunction_t rgb_function,
                                GrCombineFactor_t   rgb_factor,
                                GrCombineFunction_t alpha_function,
                                GrCombineFactor_t   alpha_factor,
                                FxBool              rgb_invert,
                                FxBool              alpha_invert)
{
#define FN_NAME "grTexCombine"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_CMBFUNC(rgb_function), TRP_CMBFACT(rgb_factor), TRP_CMBFUNC(alpha_function), TRP_CMBFACT(alpha_factor), TRP_BOOL(rgb_invert), TRP_BOOL(alpha_invert));
 grTexCombine(tmu, rgb_function, rgb_factor, alpha_function, alpha_factor, rgb_invert, alpha_invert);
#undef FN_NAME
}

void FX_CALL trap_grTexDetailControl (GrChipID_t tmu,
                                      int        lod_bias,
                                      FxU8       detail_scale,
                                      float      detail_max)
{
#define FN_NAME "grTexDetailControl"
 TRAP_LOG("%s(%s, %u, %d, %f)\n", FN_NAME, TRP_TMU(tmu), lod_bias, detail_scale, detail_max);
 grTexDetailControl(tmu, lod_bias, detail_scale, detail_max);
#undef FN_NAME
}

void FX_CALL trap_grTexFilterMode (GrChipID_t            tmu,
                                   GrTextureFilterMode_t minfilter_mode,
                                   GrTextureFilterMode_t magfilter_mode)
{
#define FN_NAME "grTexFilterMode"
 TRAP_LOG("%s(%s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_TEXFILTER(minfilter_mode), TRP_TEXFILTER(magfilter_mode));
 grTexFilterMode(tmu, minfilter_mode, magfilter_mode);
#undef FN_NAME
}

void FX_CALL trap_grTexLodBiasValue (GrChipID_t tmu,
                                     float      bias)
{
#define FN_NAME "grTexLodBiasValue"
 TRAP_LOG("%s(%s, %f)\n", FN_NAME, TRP_TMU(tmu), bias);
 grTexLodBiasValue(tmu, bias);
#undef FN_NAME
}

void FX_CALL trap_grTexDownloadMipMap (GrChipID_t tmu,
                                       FxU32      startAddress,
                                       FxU32      evenOdd,
                                       GrTexInfo  *info)
{
#define FN_NAME "grTexDownloadMipMap"
 TRAP_LOG("%s(%s, %08lx, %s, %p)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_EVENODD(evenOdd), (void *)info);
 grTexDownloadMipMap(tmu, startAddress, evenOdd, info);
#undef FN_NAME
}

void FX_CALL trap_grTexDownloadMipMapLevel (GrChipID_t        tmu,
                                            FxU32             startAddress,
                                            GrLOD_t           thisLod,
                                            GrLOD_t           largeLod,
                                            GrAspectRatio_t   aspectRatio,
                                            GrTextureFormat_t format,
                                            FxU32             evenOdd,
                                            void              *data)
{
#define FN_NAME "grTexDownloadMipMapLevel"
 TRAP_LOG("%s(%s, %08lx, %s, %s, %s, %s, %s, %p)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_LODLEVEL(thisLod), TRP_LODLEVEL(largeLod), TRP_ASPECTRATIO(aspectRatio), TRP_TEXFMT(format), TRP_EVENODD(evenOdd), data);
 grTexDownloadMipMapLevel(tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data);
#undef FN_NAME
}

FxBool FX_CALL trap_grTexDownloadMipMapLevelPartial (GrChipID_t        tmu,
                                                     FxU32             startAddress,
                                                     GrLOD_t           thisLod,
                                                     GrLOD_t           largeLod,
                                                     GrAspectRatio_t   aspectRatio,
                                                     GrTextureFormat_t format,
                                                     FxU32             evenOdd,
                                                     void              *data,
                                                     int               start,
                                                     int               end)
{
#define FN_NAME "grTexDownloadMipMapLevelPartial"
 FxBool rv;
 TRAP_LOG("%s(%s, %08lx, %s, %s, %s, %s, %s, %p, %d, %d)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_LODLEVEL(thisLod), TRP_LODLEVEL(largeLod), TRP_ASPECTRATIO(aspectRatio), TRP_TEXFMT(format), TRP_EVENODD(evenOdd), data, start, end);
 rv = grTexDownloadMipMapLevelPartial(tmu, startAddress, thisLod, largeLod, aspectRatio, format, evenOdd, data, start, end);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grTexDownloadTable (GrTexTable_t type,
                                      void         *data)
{
#define FN_NAME "grTexDownloadTable"
 TRAP_LOG("%s(%s, %p)\n", FN_NAME, TRP_TABLE(type), data);
 grTexDownloadTable(type, data);
#undef FN_NAME
}

void FX_CALL trap_grTexDownloadTablePartial (GrTexTable_t type,
                                             void         *data,
                                             int          start,
                                             int          end)
{
#define FN_NAME "grTexDownloadTablePartial"
 TRAP_LOG("%s(%s, %p, %d, %d)\n", FN_NAME, TRP_TABLE(type), data, start, end);
 grTexDownloadTablePartial(type, data, start, end);
#undef FN_NAME
}

void FX_CALL trap_grTexMipMapMode (GrChipID_t     tmu,
                                   GrMipMapMode_t mode,
                                   FxBool         lodBlend)
{
#define FN_NAME "grTexMipMapMode"
 TRAP_LOG("%s(%s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_MIPMODE(mode), TRP_BOOL(lodBlend));
 grTexMipMapMode(tmu, mode, lodBlend);
#undef FN_NAME
}

void FX_CALL trap_grTexMultibase (GrChipID_t tmu,
                                  FxBool     enable)
{
#define FN_NAME "grTexMultibase"
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_BOOL(enable));
 grTexMultibase(tmu, enable);
#undef FN_NAME
}

void FX_CALL trap_grTexMultibaseAddress (GrChipID_t       tmu,
                                         GrTexBaseRange_t range,
                                         FxU32            startAddress,
                                         FxU32            evenOdd,
                                         GrTexInfo        *info)
{
#define FN_NAME "grTexMultibaseAddress"
 TRAP_LOG("%s(%s, %s, %08lx, %s, %p)\n", FN_NAME, TRP_TMU(tmu), TRP_TEXBASERANGE(range), startAddress, TRP_EVENODD(evenOdd), (void *)info);
 grTexMultibaseAddress(tmu, range, startAddress, evenOdd, info);
#undef FN_NAME
}

/*
** linear frame buffer functions
*/
FxBool FX_CALL trap_grLfbLock (GrLock_t           type,
                               GrBuffer_t         buffer,
                               GrLfbWriteMode_t   writeMode,
                               GrOriginLocation_t origin,
                               FxBool             pixelPipeline,
                               GrLfbInfo_t        *info)
{
#define FN_NAME "grLfbLock"
 FxBool rv;
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %p)\n", FN_NAME, TRP_LOCKTYPE(type), TRP_BUFFER(buffer), TRP_WRITEMODE(writeMode), TRP_ORIGIN(origin), TRP_BOOL(pixelPipeline), (void *)info);
 rv = grLfbLock(type, buffer, writeMode, origin, pixelPipeline, info);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

FxBool FX_CALL trap_grLfbUnlock (GrLock_t   type,
                                 GrBuffer_t buffer)
{
#define FN_NAME "grLfbUnlock"
 FxBool rv;
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_LOCKTYPE(type), TRP_BUFFER(buffer));
 rv = grLfbUnlock(type, buffer);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grLfbConstantAlpha (GrAlpha_t alpha)
{
#define FN_NAME "grLfbConstantAlpha"
 TRAP_LOG("%s(%02x)\n", FN_NAME, alpha);
 grLfbConstantAlpha(alpha);
#undef FN_NAME
}

void FX_CALL trap_grLfbConstantDepth (FxU32 depth)
{
#define FN_NAME "grLfbConstantDepth"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, depth);
 grLfbConstantDepth(depth);
#undef FN_NAME
}

void FX_CALL trap_grLfbWriteColorSwizzle (FxBool swizzleBytes,
                                          FxBool swapWords)
{
#define FN_NAME "grLfbWriteColorSwizzle"
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_BOOL(swizzleBytes), TRP_BOOL(swapWords));
 grLfbWriteColorSwizzle(swizzleBytes, swapWords);
#undef FN_NAME
}

void FX_CALL trap_grLfbWriteColorFormat (GrColorFormat_t colorFormat)
{
#define FN_NAME "grLfbWriteColorFormat"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_COLFMT(colorFormat));
 grLfbWriteColorFormat(colorFormat);
#undef FN_NAME
}

FxBool FX_CALL trap_grLfbWriteRegion (GrBuffer_t    dst_buffer,
                                      FxU32         dst_x,
                                      FxU32         dst_y,
                                      GrLfbSrcFmt_t src_format,
                                      FxU32         src_width,
                                      FxU32         src_height,
                                      FxBool        pixelPipeline,
                                      FxI32         src_stride,
                                      void          *src_data)
{
#define FN_NAME "grLfbWriteRegion"
 FxBool rv;
 TRAP_LOG("%s(%s, %lu, %lu, %s, %lu, %lu, %s, %ld, %p)\n", FN_NAME, TRP_BUFFER(dst_buffer), dst_x, dst_y, TRP_SRCFMT(src_format), src_width, src_height, TRP_BOOL(pixelPipeline), src_stride, src_data);
 rv = grLfbWriteRegion(dst_buffer, dst_x, dst_y, src_format, src_width, src_height, pixelPipeline, src_stride, src_data);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

FxBool FX_CALL trap_grLfbReadRegion (GrBuffer_t src_buffer,
                                     FxU32      src_x,
                                     FxU32      src_y,
                                     FxU32      src_width,
                                     FxU32      src_height,
                                     FxU32      dst_stride,
                                     void       *dst_data)
{
#define FN_NAME "grLfbReadRegion"
 FxBool rv;
 TRAP_LOG("%s(%s, %lu, %lu, %lu, %lu, %ld, %p)\n", FN_NAME, TRP_BUFFER(src_buffer), src_x, src_y, src_width, src_height, dst_stride, dst_data);
 rv = grLfbReadRegion(src_buffer, src_x, src_y, src_width, src_height, dst_stride, dst_data);
 TRAP_LOG(GOT "%s\n", TRP_BOOL(rv));
 return rv;
#undef FN_NAME
}

/*
** glide management functions
*/
void FX_CALL trap_grGlideInit (void)
{
#define FN_NAME "grGlideInit"
 TRAP_LOG("%s()\n", FN_NAME);
 grGlideInit();
#undef FN_NAME
}

void FX_CALL trap_grGlideShutdown (void)
{
#define FN_NAME "grGlideShutdown"
 TRAP_LOG("%s()\n", FN_NAME);
 grGlideShutdown();
#undef FN_NAME
}

void FX_CALL trap_grGlideGetState (void *state)
{
#define FN_NAME "grGlideGetState"
 TRAP_LOG("%s(%p)\n", FN_NAME, state);
 grGlideGetState(state);
#undef FN_NAME
}

void FX_CALL trap_grGlideSetState (const void *state)
{
#define FN_NAME "grGlideSetState"
 TRAP_LOG("%s(%p)\n", FN_NAME, state);
 grGlideSetState(state);
#undef FN_NAME
}

void FX_CALL trap_grGlideGetVertexLayout (void *layout)
{
#define FN_NAME "grGlideGetVertexLayout"
 TRAP_LOG("%s(%p)\n", FN_NAME, layout);
 grGlideGetVertexLayout(layout);
#undef FN_NAME
}

void FX_CALL trap_grGlideSetVertexLayout (const void *layout)
{
#define FN_NAME "grGlideSetVertexLayout"
 TRAP_LOG("%s(%p)\n", FN_NAME, layout);
 grGlideSetVertexLayout(layout);
#undef FN_NAME
}

/*
** glide utility functions
*/
void FX_CALL trap_guGammaCorrectionRGB (FxFloat red,
                                        FxFloat green,
                                        FxFloat blue)
{
#define FN_NAME "guGammaCorrectionRGB"
 TRAP_LOG("%s(%f, %f, %f)\n", FN_NAME, red, green, blue);
 guGammaCorrectionRGB(red, green, blue);
#undef FN_NAME
}

float FX_CALL trap_guFogTableIndexToW (int i)
{
#define FN_NAME "guFogTableIndexToW"
 float rv;
 TRAP_LOG("%s(%d)\n", FN_NAME, i);
 rv = guFogTableIndexToW(i);
 TRAP_LOG(GOT "%f\n", rv);
 return rv;
#undef FN_NAME
}

void FX_CALL trap_guFogGenerateExp (GrFog_t *fogtable,
                                    float   density)
{
#define FN_NAME "guFogGenerateExp"
 TRAP_LOG("%s(%p, %f)\n", FN_NAME, fogtable, density);
 guFogGenerateExp(fogtable, density);
#undef FN_NAME
}

void FX_CALL trap_guFogGenerateExp2 (GrFog_t *fogtable,
                                     float   density)
{
#define FN_NAME "guFogGenerateExp2"
 TRAP_LOG("%s(%p, %f)\n", FN_NAME, fogtable, density);
 guFogGenerateExp2(fogtable, density);
#undef FN_NAME
}

void FX_CALL trap_guFogGenerateLinear (GrFog_t *fogtable,
                                       float   nearZ,
                                       float   farZ)
{
#define FN_NAME "guFogGenerateLinear"
 TRAP_LOG("%s(%p, %f, %f)\n", FN_NAME, fogtable, nearZ, farZ);
 guFogGenerateLinear(fogtable, nearZ, farZ);
#undef FN_NAME
}

/*
** glide extensions
*/
void FX_CALL trap_grSetNumPendingBuffers (FxI32 NumPendingBuffers)
{
#define FN_NAME "grSetNumPendingBuffers"
 TRAP_LOG("%s(%ld)\n", FN_NAME, NumPendingBuffers);
 assert(real_grSetNumPendingBuffers);
 (*real_grSetNumPendingBuffers)(NumPendingBuffers);
#undef FN_NAME
}

char *FX_CALL trap_grGetRegistryOrEnvironmentStringExt (char *theEntry)
{
#define FN_NAME "grGetRegistryOrEnvironmentStringExt"
 char *rv;
 TRAP_LOG("%s(\"%s\")\n", FN_NAME, theEntry);
 assert(real_grGetRegistryOrEnvironmentStringExt);
 rv = (*real_grGetRegistryOrEnvironmentStringExt)(theEntry);
 if (rv) {
    TRAP_LOG(GOT "\"%s\"\n", rv);
 } else {
    TRAP_LOG(GOT "NULL\n");
 }
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grGetGammaTableExt (FxU32 nentries,
                                      FxU32 *red,
                                      FxU32 *green,
                                      FxU32 *blue)
{
#define FN_NAME "grGetGammaTableExt"
 TRAP_LOG("%s(%lu, %p, %p, %p)\n", FN_NAME, nentries, (void *)red, (void *)green, (void *)blue);
 assert(real_grGetGammaTableExt);
 (*real_grGetGammaTableExt)(nentries, red, green, blue);
#undef FN_NAME
}

void FX_CALL trap_grChromaRangeModeExt (GrChromakeyMode_t mode)
{
#define FN_NAME "grChromaRangeModeExt"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_CKMODE(mode));
 assert(real_grChromaRangeModeExt);
 (*real_grChromaRangeModeExt)(mode);
#undef FN_NAME
}

void FX_CALL trap_grChromaRangeExt (GrColor_t           color,
                                    GrColor_t           range,
                                    GrChromaRangeMode_t match_mode)
{
#define FN_NAME "grChromaRangeExt"
 TRAP_LOG("%s(%08lx, %08lx, %s)\n", FN_NAME, color, range, TRP_CRMODE(match_mode));
 assert(real_grChromaRangeExt);
 (*real_grChromaRangeExt)(color, range, match_mode);
#undef FN_NAME
}

void FX_CALL trap_grTexChromaModeExt (GrChipID_t        tmu,
                                      GrChromakeyMode_t mode)
{
#define FN_NAME "grTexChromaModeExt"
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_CKMODE(mode));
 assert(real_grTexChromaModeExt);
 (*real_grTexChromaModeExt)(tmu, mode);
#undef FN_NAME
}

void FX_CALL trap_grTexChromaRangeExt (GrChipID_t           tmu,
                                       GrColor_t            min,
                                       GrColor_t            max,
                                       GrTexChromakeyMode_t mode)
{
#define FN_NAME "grTexChromaRangeExt"
 TRAP_LOG("%s(%s, %08lx, %08lx, %s)\n", FN_NAME, TRP_TMU(tmu), min, max, TRP_CRMODE(mode));
 assert(real_grTexChromaRangeExt);
 (*real_grTexChromaRangeExt)(tmu, min, max, mode);
#undef FN_NAME
}

        /* pointcast */
void FX_CALL trap_grTexDownloadTableExt (GrChipID_t   tmu,
                                         GrTexTable_t type,
                                         void         *data)
{
#define FN_NAME "grTexDownloadTableExt"
 TRAP_LOG("%s(%s, %s, %p)\n", FN_NAME, TRP_TMU(tmu), TRP_TABLE(type), data);
 assert(real_grTexDownloadTableExt);
 (*real_grTexDownloadTableExt)(tmu, type, data);
#undef FN_NAME
}

void FX_CALL trap_grTexDownloadTablePartialExt (GrChipID_t   tmu,
                                                GrTexTable_t type,
                                                void         *data,
                                                int          start,
                                                int          end)
{
#define FN_NAME "grTexDownloadTablePartialExt"
 TRAP_LOG("%s(%s, %s, %p, %d, %d)\n", FN_NAME, TRP_TMU(tmu), TRP_TABLE(type), data, start, end);
 assert(real_grTexDownloadTablePartialExt);
 (*real_grTexDownloadTablePartialExt)(tmu, type, data, start, end);
#undef FN_NAME
}

void FX_CALL trap_grTexNCCTableExt (GrChipID_t   tmu,
                                    GrNCCTable_t table)
{
#define FN_NAME "grTexNCCTableExt"
 TRAP_LOG("%s(%s, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_NCC(table));
 assert(real_grTexNCCTableExt);
 (*real_grTexNCCTableExt)(tmu, table);
#undef FN_NAME
}

        /* tbext */
void FX_CALL trap_grTextureBufferExt (GrChipID_t        tmu,
                                      FxU32             startAddress,
                                      GrLOD_t           thisLOD,
                                      GrLOD_t           largeLOD,
                                      GrAspectRatio_t   aspectRatio,
                                      GrTextureFormat_t format,
                                      FxU32             odd_even_mask)
{
#define FN_NAME "grTextureBufferExt"
 TRAP_LOG("%s(%s, %08lx, %s, %s, %s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_LODLEVEL(thisLOD), TRP_LODLEVEL(largeLOD), TRP_ASPECTRATIO(aspectRatio), TRP_TEXFMT(format), TRP_EVENODD(odd_even_mask));
 assert(real_grTextureBufferExt);
 (*real_grTextureBufferExt)(tmu, startAddress, thisLOD, largeLOD, aspectRatio, format, odd_even_mask);
#undef FN_NAME
}

void FX_CALL trap_grTextureAuxBufferExt (GrChipID_t        tmu,
                                         FxU32             startAddress,
                                         GrLOD_t           thisLOD,
                                         GrLOD_t           largeLOD,
                                         GrAspectRatio_t   aspectRatio,
                                         GrTextureFormat_t format,
                                         FxU32             odd_even_mask)
{
#define FN_NAME "grTextureAuxBufferExt"
 TRAP_LOG("%s(%s, %08lx, %s, %s, %s, %s, %s)\n", FN_NAME, TRP_TMU(tmu), startAddress, TRP_LODLEVEL(thisLOD), TRP_LODLEVEL(largeLOD), TRP_ASPECTRATIO(aspectRatio), TRP_TEXFMT(format), TRP_EVENODD(odd_even_mask));
 assert(real_grTextureAuxBufferExt);
 (*real_grTextureAuxBufferExt)(tmu, startAddress, thisLOD, largeLOD, aspectRatio, format, odd_even_mask);
#undef FN_NAME
}

void FX_CALL trap_grAuxBufferExt (GrBuffer_t buffer)
{
#define FN_NAME "grAuxBufferExt"
 TRAP_LOG("%s(%s)\n", FN_NAME, TRP_BUFFER(buffer));
 assert(real_grAuxBufferExt);
 (*real_grAuxBufferExt)(buffer);
#undef FN_NAME
}

        /* napalm */
GrContext_t FX_CALL trap_grSstWinOpenExt (FxU32                hWnd,
                                          GrScreenResolution_t resolution,
                                          GrScreenRefresh_t    refresh,
                                          GrColorFormat_t      format,
                                          GrOriginLocation_t   origin,
                                          GrPixelFormat_t      pixelformat,
                                          int                  nColBuffers,
                                          int                  nAuxBuffers)
{
#define FN_NAME "grSstWinOpenExt"
 GrContext_t rv;
 TRAP_LOG("%s(%08lx, %s, %s, %s, %s, %s, %d, %d)\n", FN_NAME, hWnd, TRP_RESOLUTION(resolution), TRP_REFRESH(refresh), TRP_COLFMT(format), TRP_ORIGIN(origin), TRP_PIXFMT(pixelformat), nColBuffers, nAuxBuffers);
 assert(real_grSstWinOpenExt);
 rv = (*real_grSstWinOpenExt)(hWnd, resolution, refresh, format, origin, pixelformat, nColBuffers, nAuxBuffers);
 TRAP_LOG(GOT "%p\n", (void *)rv);
 return rv;
#undef FN_NAME
}

void FX_CALL trap_grStencilFuncExt (GrCmpFnc_t  fnc,
                                    GrStencil_t ref,
                                    GrStencil_t mask)
{
#define FN_NAME "grStencilFuncExt"
 TRAP_LOG("%s(%s, %02x, %02x)\n", FN_NAME, TRP_CMPFUNC(fnc), ref, mask);
 assert(real_grStencilFuncExt);
 (*real_grStencilFuncExt)(fnc, ref, mask);
#undef FN_NAME
}

void FX_CALL trap_grStencilMaskExt (GrStencil_t value)
{
#define FN_NAME "grStencilMaskExt"
 TRAP_LOG("%s(%02x)\n", FN_NAME, value);
 assert(real_grStencilMaskExt);
 (*real_grStencilMaskExt)(value);
#undef FN_NAME
}

void FX_CALL trap_grStencilOpExt (GrStencilOp_t stencil_fail,
                                  GrStencilOp_t depth_fail,
                                  GrStencilOp_t depth_pass)
{
#define FN_NAME "grStencilOpExt"
 TRAP_LOG("%s(%s, %s, %s)\n", FN_NAME, TRP_STENCILOP(stencil_fail), TRP_STENCILOP(depth_fail), TRP_STENCILOP(depth_pass));
 assert(real_grStencilOpExt);
 (*real_grStencilOpExt)(stencil_fail, depth_fail, depth_pass);
#undef FN_NAME
}

void FX_CALL trap_grLfbConstantStencilExt (GrStencil_t value)
{
#define FN_NAME "grLfbConstantStencilExt"
 TRAP_LOG("%s(%02x)\n", FN_NAME, value);
 assert(real_grLfbConstantStencilExt);
 (*real_grLfbConstantStencilExt)(value);
#undef FN_NAME
}

void FX_CALL trap_grBufferClearExt (GrColor_t   color,
                                    GrAlpha_t   alpha,
                                    FxU32       depth,
                                    GrStencil_t stencil)
{
#define FN_NAME "grBufferClearExt"
 TRAP_LOG("%s(%08lx, %02x, %08lx, %02x)\n", FN_NAME, color, alpha, depth, stencil);
 assert(real_grBufferClearExt);
 (*real_grBufferClearExt)(color, alpha, depth, stencil);
#undef FN_NAME
}

void FX_CALL trap_grColorCombineExt (GrCCUColor_t    a,
                                     GrCombineMode_t a_mode,
                                     GrCCUColor_t    b,
                                     GrCombineMode_t b_mode,
                                     GrCCUColor_t    c,
                                     FxBool          c_invert,
                                     GrCCUColor_t    d,
                                     FxBool          d_invert,
                                     FxU32           shift,
                                     FxBool          invert)
{
#define FN_NAME "grColorCombineExt"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s, %s, %s, %lu, %s)\n", FN_NAME, TRP_CU(a), TRP_CMBMODE(a_mode), TRP_CU(b), TRP_CMBMODE(b_mode), TRP_CU(c), TRP_BOOL(c_invert), TRP_CU(d), TRP_BOOL(d_invert), shift, TRP_BOOL(invert));
 assert(real_grColorCombineExt);
 (*real_grColorCombineExt)(a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
#undef FN_NAME
}

void FX_CALL trap_grAlphaCombineExt (GrACUColor_t    a,
                                     GrCombineMode_t a_mode,
                                     GrACUColor_t    b,
                                     GrCombineMode_t b_mode,
                                     GrACUColor_t    c,
                                     FxBool          c_invert,
                                     GrACUColor_t    d,
                                     FxBool          d_invert,
                                     FxU32           shift,
                                     FxBool          invert)
{
#define FN_NAME "grAlphaCombineExt"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s, %s, %s, %lu, %s)\n", FN_NAME, TRP_CU(a), TRP_CMBMODE(a_mode), TRP_CU(b), TRP_CMBMODE(b_mode), TRP_CU(c), TRP_BOOL(c_invert), TRP_CU(d), TRP_BOOL(d_invert), shift, TRP_BOOL(invert));
 assert(real_grAlphaCombineExt);
 (*real_grAlphaCombineExt)(a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
#undef FN_NAME
}

void FX_CALL trap_grTexColorCombineExt (GrChipID_t      tmu,
                                        GrTCCUColor_t   a,
                                        GrCombineMode_t a_mode,
                                        GrTCCUColor_t   b,
                                        GrCombineMode_t b_mode,
                                        GrTCCUColor_t   c,
                                        FxBool          c_invert,
                                        GrTCCUColor_t   d,
                                        FxBool          d_invert,
                                        FxU32           shift,
                                        FxBool          invert)
{
#define FN_NAME "grTexColorCombineExt"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s, %s, %s, %s, %lu, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_CU(a), TRP_CMBMODE(a_mode), TRP_CU(b), TRP_CMBMODE(b_mode), TRP_CU(c), TRP_BOOL(c_invert), TRP_CU(d), TRP_BOOL(d_invert), shift, TRP_BOOL(invert));
 assert(real_grTexColorCombineExt);
 (*real_grTexColorCombineExt)(tmu, a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
#undef FN_NAME
}

void FX_CALL trap_grTexAlphaCombineExt (GrChipID_t      tmu,
                                        GrTACUColor_t   a,
                                        GrCombineMode_t a_mode,
                                        GrTACUColor_t   b,
                                        GrCombineMode_t b_mode,
                                        GrTACUColor_t   c,
                                        FxBool          c_invert,
                                        GrTACUColor_t   d,
                                        FxBool          d_invert,
                                        FxU32           shift,
                                        FxBool          invert)
{
#define FN_NAME "grTexAlphaCombineExt"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s, %s, %s, %s, %lu, %s)\n", FN_NAME, TRP_TMU(tmu), TRP_CU(a), TRP_CMBMODE(a_mode), TRP_CU(b), TRP_CMBMODE(b_mode), TRP_CU(c), TRP_BOOL(c_invert), TRP_CU(d), TRP_BOOL(d_invert), shift, TRP_BOOL(invert));
 assert(real_grTexAlphaCombineExt);
 (*real_grTexAlphaCombineExt)(tmu, a, a_mode, b, b_mode, c, c_invert, d, d_invert, shift, invert);
#undef FN_NAME
}

void FX_CALL trap_grConstantColorValueExt (GrChipID_t tmu,
                                           GrColor_t  value)
{
#define FN_NAME "grConstantColorValueExt"
 TRAP_LOG("%s(%s, %08lx)\n", FN_NAME, TRP_TMU(tmu), value);
 assert(real_grConstantColorValueExt);
 (*real_grConstantColorValueExt)(tmu, value);
#undef FN_NAME
}

void FX_CALL trap_grColorMaskExt (FxBool r,
                                  FxBool g,
                                  FxBool b,
                                  FxBool a)
{
#define FN_NAME "grColorMaskExt"
 TRAP_LOG("%s(%s, %s, %s, %s)\n", FN_NAME, TRP_BOOL(r), TRP_BOOL(g), TRP_BOOL(b), TRP_BOOL(a));
 assert(real_grColorMaskExt);
 (*real_grColorMaskExt)(r, g, b, a);
#undef FN_NAME
}

void FX_CALL trap_grAlphaBlendFunctionExt (GrAlphaBlendFnc_t rgb_sf,
                                           GrAlphaBlendFnc_t rgb_df,
                                           GrAlphaBlendOp_t  rgb_op,
                                           GrAlphaBlendFnc_t alpha_sf,
                                           GrAlphaBlendFnc_t alpha_df,
                                           GrAlphaBlendOp_t  alpha_op)
{
#define FN_NAME "grAlphaBlendFunctionExt"
 TRAP_LOG("%s(%s, %s, %s, %s, %s, %s)\n", FN_NAME, TRP_BLEND(rgb_sf), TRP_BLEND(rgb_df), TRP_BLENDOP(rgb_op), TRP_BLEND(alpha_sf), TRP_BLEND(alpha_df), TRP_BLENDOP(alpha_op));
 assert(real_grAlphaBlendFunctionExt);
 (*real_grAlphaBlendFunctionExt)(rgb_sf, rgb_df, rgb_op, alpha_sf, alpha_df, alpha_op);
#undef FN_NAME
}

void FX_CALL trap_grTBufferWriteMaskExt (FxU32 tmask)
{
#define FN_NAME "grTBufferWriteMaskExt"
 TRAP_LOG("%s(%08lx)\n", FN_NAME, tmask);
 assert(real_grTBufferWriteMaskExt);
 (*real_grTBufferWriteMaskExt)(tmask);
#undef FN_NAME
}

/*
** texus functions
*/
void FX_CALL trap_txImgQuantize (char  *dst,
                                 char  *src,
                                 int   w,
                                 int   h,
                                 FxU32 format,
                                 FxU32 dither)
{
#define FN_NAME "txImgQuantize"
 TRAP_LOG("%s(%p, %p, %d, %d, %s, %s)\n", FN_NAME, dst, src, w, h, TRP_TEXFMT(format), TRP_TXDITHER(dither));
 assert(real_txImgQuantize);
 (*real_txImgQuantize)(dst, src, w, h, format, dither);
#undef FN_NAME
}

void FX_CALL trap_txMipQuantize (TxMip *pxMip,
                                 TxMip *txMip,
                                 int   fmt,
                                 FxU32 d,
                                 FxU32 comp)
{
#define FN_NAME "txMipQuantize"
 TRAP_LOG("%s(%p, %p, %s, %s, %s)\n", FN_NAME, (void *)pxMip, (void *)txMip, TRP_TEXFMT(fmt), TRP_TXDITHER(d), TRP_TXCOMPRESS(comp));
 assert(real_txMipQuantize);
 (*real_txMipQuantize)(pxMip, txMip, fmt, d, comp);
#undef FN_NAME
}

void FX_CALL trap_txPalToNcc (GuNccTable *ncc_table,
                              const FxU32 *pal)
{
#define FN_NAME "txPalToNcc"
 TRAP_LOG("%s(%p, %p)\n", FN_NAME, (void *)ncc_table, (void *)pal);
 assert(real_txPalToNcc);
 (*real_txPalToNcc)(ncc_table, pal);
#undef FN_NAME
}
#endif



/****************************************************************************\
* housekeeping (fake pointers)                                               *
\****************************************************************************/
char *FX_CALL fake_grGetRegistryOrEnvironmentStringExt (char *theEntry)
{
 return getenv(theEntry);
}

void FX_CALL fake_grTexDownloadTableExt (GrChipID_t   tmu,
                                         GrTexTable_t type,
                                         void         *data)
{
 (void)tmu;
 grTexDownloadTable(type, data);
}

void FX_CALL fake_grTexDownloadTablePartialExt (GrChipID_t   tmu,
                                                GrTexTable_t type,
                                                void         *data,
                                                int          start,
                                                int          end)
{
 (void)tmu;
 grTexDownloadTablePartial(type, data, start, end);
}

void FX_CALL fake_grTexNCCTableExt (GrChipID_t   tmu,
                                    GrNCCTable_t table)
{
 (void)tmu;
 grTexNCCTable(table);
}



/****************************************************************************\
* interface                                                                  *
\****************************************************************************/
void tdfx_hook_glide (struct tdfx_glide *Glide, int pointcast)
{
/* GET_EXT_ADDR: get function pointer
 * GET_EXT_FAKE: get function pointer if possible, else use a fake function
 * GET_EXT_NULL: get function pointer if possible, else leave NULL pointer
 */
#if FX_TRAP_GLIDE
#define GET_EXT_ADDR(name) *(GrProc *)&real_##name = grGetProcAddress(#name), Glide->name = trap_##name
#define GET_EXT_FAKE(name) GET_EXT_ADDR(name); if (real_##name == NULL) real_##name = fake_##name
#define GET_EXT_NULL(name) GET_EXT_ADDR(name); if (real_##name == NULL) Glide->name = NULL
#else  /* FX_TRAP_GLIDE */
#define GET_EXT_ADDR(name) *(GrProc *)&Glide->name = grGetProcAddress(#name)
#define GET_EXT_FAKE(name) GET_EXT_ADDR(name); if (Glide->name == NULL) Glide->name = fake_##name
#define GET_EXT_NULL(name) GET_EXT_ADDR(name)
#endif /* FX_TRAP_GLIDE */

 /*
 ** glide extensions
 */
 GET_EXT_NULL(grSetNumPendingBuffers);
 GET_EXT_FAKE(grGetRegistryOrEnvironmentStringExt);
 GET_EXT_ADDR(grGetGammaTableExt);
 GET_EXT_ADDR(grChromaRangeModeExt);
 GET_EXT_ADDR(grChromaRangeExt);
 GET_EXT_ADDR(grTexChromaModeExt);
 GET_EXT_ADDR(grTexChromaRangeExt);
 /* pointcast */
 if (pointcast) {
    GET_EXT_FAKE(grTexDownloadTableExt);
    GET_EXT_FAKE(grTexDownloadTablePartialExt);
    GET_EXT_FAKE(grTexNCCTableExt);
 } else {
    Glide->grTexDownloadTableExt = fake_grTexDownloadTableExt;
    Glide->grTexDownloadTablePartialExt = fake_grTexDownloadTablePartialExt;
    Glide->grTexNCCTableExt = fake_grTexNCCTableExt;
 }
 /* tbext */
 GET_EXT_ADDR(grTextureBufferExt);
 GET_EXT_ADDR(grTextureAuxBufferExt);
 GET_EXT_ADDR(grAuxBufferExt);
 /* napalm */
 GET_EXT_ADDR(grSstWinOpenExt);
 GET_EXT_ADDR(grStencilFuncExt);
 GET_EXT_ADDR(grStencilMaskExt);
 GET_EXT_ADDR(grStencilOpExt);
 GET_EXT_ADDR(grLfbConstantStencilExt);
 GET_EXT_ADDR(grBufferClearExt);
 GET_EXT_ADDR(grColorCombineExt);
 GET_EXT_ADDR(grAlphaCombineExt);
 GET_EXT_ADDR(grTexColorCombineExt);
 GET_EXT_ADDR(grTexAlphaCombineExt);
 GET_EXT_ADDR(grConstantColorValueExt);
 GET_EXT_ADDR(grColorMaskExt);
 GET_EXT_ADDR(grAlphaBlendFunctionExt);
 GET_EXT_ADDR(grTBufferWriteMaskExt);

 /*
 ** texus
 */
 GET_EXT_NULL(txImgQuantize);
 GET_EXT_NULL(txMipQuantize);
 GET_EXT_NULL(txPalToNcc);

#undef GET_EXT_ADDR
}

#endif /* FX */
