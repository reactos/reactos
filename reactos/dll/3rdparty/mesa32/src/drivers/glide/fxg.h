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


#ifndef TDFX_GLIDE_H_included
#define TDFX_GLIDE_H_included

#include <glide.h>
#include <g3ext.h>

#ifndef FX_TRAP_GLIDE
#define FX_TRAP_GLIDE 0
#endif

#if FX_TRAP_GLIDE
/*
** rendering functions
*/
void FX_CALL trap_grDrawPoint (const void *pt);
void FX_CALL trap_grDrawLine (const void *v1, const void *v2);
void FX_CALL trap_grDrawTriangle (const void *a, const void *b, const void *c);
void FX_CALL trap_grVertexLayout (FxU32 param, FxI32 offset, FxU32 mode);
void FX_CALL trap_grDrawVertexArray (FxU32 mode, FxU32 Count, void *pointers);
void FX_CALL trap_grDrawVertexArrayContiguous (FxU32 mode, FxU32 Count, void *pointers, FxU32 stride);

/*
**  Antialiasing Functions
*/
void FX_CALL trap_grAADrawTriangle (const void *a, const void *b, const void *c, FxBool ab_antialias, FxBool bc_antialias, FxBool ca_antialias);

/*
** buffer management
*/
void FX_CALL trap_grBufferClear (GrColor_t color, GrAlpha_t alpha, FxU32 depth);
void FX_CALL trap_grBufferSwap (FxU32 swap_interval);
void FX_CALL trap_grRenderBuffer (GrBuffer_t buffer);

/*
** error management
*/
void FX_CALL trap_grErrorSetCallback (GrErrorCallbackFnc_t fnc);

/*
** SST routines
*/
void FX_CALL trap_grFinish (void);
void FX_CALL trap_grFlush (void);
GrContext_t FX_CALL trap_grSstWinOpen (FxU32 hWnd, GrScreenResolution_t screen_resolution, GrScreenRefresh_t refresh_rate, GrColorFormat_t color_format, GrOriginLocation_t origin_location, int nColBuffers, int nAuxBuffers);
FxBool FX_CALL trap_grSstWinClose (GrContext_t context);
FxBool FX_CALL trap_grSelectContext (GrContext_t context);
void FX_CALL trap_grSstOrigin (GrOriginLocation_t origin);
void FX_CALL trap_grSstSelect (int which_sst);

/*
** Glide configuration and special effect maintenance functions
*/
void FX_CALL trap_grAlphaBlendFunction (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df);
void FX_CALL trap_grAlphaCombine (GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, FxBool invert);
void FX_CALL trap_grAlphaControlsITRGBLighting (FxBool enable);
void FX_CALL trap_grAlphaTestFunction (GrCmpFnc_t function);
void FX_CALL trap_grAlphaTestReferenceValue (GrAlpha_t value);
void FX_CALL trap_grChromakeyMode (GrChromakeyMode_t mode);
void FX_CALL trap_grChromakeyValue (GrColor_t value);
void FX_CALL trap_grClipWindow (FxU32 minx, FxU32 miny, FxU32 maxx, FxU32 maxy);
void FX_CALL trap_grColorCombine (GrCombineFunction_t function, GrCombineFactor_t factor, GrCombineLocal_t local, GrCombineOther_t other, FxBool invert);
void FX_CALL trap_grColorMask (FxBool rgb, FxBool a);
void FX_CALL trap_grCullMode (GrCullMode_t mode);
void FX_CALL trap_grConstantColorValue (GrColor_t value);
void FX_CALL trap_grDepthBiasLevel (FxI32 level);
void FX_CALL trap_grDepthBufferFunction (GrCmpFnc_t function);
void FX_CALL trap_grDepthBufferMode (GrDepthBufferMode_t mode);
void FX_CALL trap_grDepthMask (FxBool mask);
void FX_CALL trap_grDisableAllEffects (void);
void FX_CALL trap_grDitherMode (GrDitherMode_t mode);
void FX_CALL trap_grFogColorValue (GrColor_t fogcolor);
void FX_CALL trap_grFogMode (GrFogMode_t mode);
void FX_CALL trap_grFogTable (const GrFog_t ft[]);
void FX_CALL trap_grLoadGammaTable (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue);
void FX_CALL trap_grSplash (float x, float y, float width, float height, FxU32 frame);
FxU32 FX_CALL trap_grGet (FxU32 pname, FxU32 plength, FxI32 *params);
const char * FX_CALL trap_grGetString (FxU32 pname);
FxI32 FX_CALL trap_grQueryResolutions (const GrResolution *resTemplate, GrResolution *output);
FxBool FX_CALL trap_grReset (FxU32 what);
GrProc FX_CALL trap_grGetProcAddress (char *procName);
void FX_CALL trap_grEnable (GrEnableMode_t mode);
void FX_CALL trap_grDisable (GrEnableMode_t mode);
void FX_CALL trap_grCoordinateSpace (GrCoordinateSpaceMode_t mode);
void FX_CALL trap_grDepthRange (FxFloat n, FxFloat f);
void FX_CALL trap_grStippleMode (GrStippleMode_t mode);
void FX_CALL trap_grStipplePattern (GrStipplePattern_t mode);
void FX_CALL trap_grViewport (FxI32 x, FxI32 y, FxI32 width, FxI32 height);

/*
** texture mapping control functions
*/
FxU32 FX_CALL trap_grTexCalcMemRequired (GrLOD_t lodmin, GrLOD_t lodmax, GrAspectRatio_t aspect, GrTextureFormat_t fmt);
FxU32 FX_CALL trap_grTexTextureMemRequired (FxU32 evenOdd, GrTexInfo *info);
FxU32 FX_CALL trap_grTexMinAddress (GrChipID_t tmu);
FxU32 FX_CALL trap_grTexMaxAddress (GrChipID_t tmu);
void FX_CALL trap_grTexNCCTable (GrNCCTable_t table);
void FX_CALL trap_grTexSource (GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info);
void FX_CALL trap_grTexClampMode (GrChipID_t tmu, GrTextureClampMode_t s_clampmode, GrTextureClampMode_t t_clampmode);
void FX_CALL trap_grTexCombine (GrChipID_t tmu, GrCombineFunction_t rgb_function, GrCombineFactor_t rgb_factor, GrCombineFunction_t alpha_function, GrCombineFactor_t alpha_factor, FxBool rgb_invert, FxBool alpha_invert);
void FX_CALL trap_grTexDetailControl (GrChipID_t tmu, int lod_bias, FxU8 detail_scale, float detail_max);
void FX_CALL trap_grTexFilterMode (GrChipID_t tmu, GrTextureFilterMode_t minfilter_mode, GrTextureFilterMode_t magfilter_mode);
void FX_CALL trap_grTexLodBiasValue (GrChipID_t tmu, float bias);
void FX_CALL trap_grTexDownloadMipMap (GrChipID_t tmu, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info);
void FX_CALL trap_grTexDownloadMipMapLevel (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 evenOdd, void *data);
FxBool FX_CALL trap_grTexDownloadMipMapLevelPartial (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLod, GrLOD_t largeLod, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 evenOdd, void *data, int start, int end);
void FX_CALL trap_grTexDownloadTable (GrTexTable_t type, void *data);
void FX_CALL trap_grTexDownloadTablePartial (GrTexTable_t type, void *data, int start, int end);
void FX_CALL trap_grTexMipMapMode (GrChipID_t tmu, GrMipMapMode_t mode, FxBool lodBlend);
void FX_CALL trap_grTexMultibase (GrChipID_t tmu, FxBool enable);
void FX_CALL trap_grTexMultibaseAddress (GrChipID_t tmu, GrTexBaseRange_t range, FxU32 startAddress, FxU32 evenOdd, GrTexInfo *info);

/*
** linear frame buffer functions
*/
FxBool FX_CALL trap_grLfbLock (GrLock_t type, GrBuffer_t buffer, GrLfbWriteMode_t writeMode, GrOriginLocation_t origin, FxBool pixelPipeline, GrLfbInfo_t *info);
FxBool FX_CALL trap_grLfbUnlock (GrLock_t type, GrBuffer_t buffer);
void FX_CALL trap_grLfbConstantAlpha (GrAlpha_t alpha);
void FX_CALL trap_grLfbConstantDepth (FxU32 depth);
void FX_CALL trap_grLfbWriteColorSwizzle (FxBool swizzleBytes, FxBool swapWords);
void FX_CALL trap_grLfbWriteColorFormat (GrColorFormat_t colorFormat);
FxBool FX_CALL trap_grLfbWriteRegion (GrBuffer_t dst_buffer, FxU32 dst_x, FxU32 dst_y, GrLfbSrcFmt_t src_format, FxU32 src_width, FxU32 src_height, FxBool pixelPipeline, FxI32 src_stride, void *src_data);
FxBool FX_CALL trap_grLfbReadRegion (GrBuffer_t src_buffer, FxU32 src_x, FxU32 src_y, FxU32 src_width, FxU32 src_height, FxU32 dst_stride, void *dst_data);

/*
** glide management functions
*/
void FX_CALL trap_grGlideInit (void);
void FX_CALL trap_grGlideShutdown (void);
void FX_CALL trap_grGlideGetState (void *state);
void FX_CALL trap_grGlideSetState (const void *state);
void FX_CALL trap_grGlideGetVertexLayout (void *layout);
void FX_CALL trap_grGlideSetVertexLayout (const void *layout);

/*
** glide utility functions
*/
void FX_CALL trap_guGammaCorrectionRGB (FxFloat red, FxFloat green, FxFloat blue);
float FX_CALL trap_guFogTableIndexToW (int i);
void FX_CALL trap_guFogGenerateExp (GrFog_t *fogtable, float density);
void FX_CALL trap_guFogGenerateExp2 (GrFog_t *fogtable, float density);
void FX_CALL trap_guFogGenerateLinear (GrFog_t *fogtable, float nearZ, float farZ);

#ifndef FX_TRAP_GLIDE_internal
/*
** rendering functions
*/
#define grDrawPoint                     trap_grDrawPoint
#define grDrawLine                      trap_grDrawLine
#define grDrawTriangle                  trap_grDrawTriangle
#define grVertexLayout                  trap_grVertexLayout
#define grDrawVertexArray               trap_grDrawVertexArray
#define grDrawVertexArrayContiguous     trap_grDrawVertexArrayContiguous

/*
**  Antialiasing Functions
*/
#define grAADrawTriangle                trap_grAADrawTriangle

/*
** buffer management
*/
#define grBufferClear                   trap_grBufferClear
#define grBufferSwap                    trap_grBufferSwap
#define grRenderBuffer                  trap_grRenderBuffer

/*
** error management
*/
#define grErrorSetCallback              trap_grErrorSetCallback

/*
** SST routines
*/
#define grFinish                        trap_grFinish
#define grFlush                         trap_grFlush
#define grSstWinOpen                    trap_grSstWinOpen
#define grSstWinClose                   trap_grSstWinClose
#define grSelectContext                 trap_grSelectContext
#define grSstOrigin                     trap_grSstOrigin
#define grSstSelect                     trap_grSstSelect

/*
** Glide configuration and special effect maintenance functions
*/
#define grAlphaBlendFunction            trap_grAlphaBlendFunction
#define grAlphaCombine                  trap_grAlphaCombine
#define grAlphaControlsITRGBLighting    trap_grAlphaControlsITRGBLighting
#define grAlphaTestFunction             trap_grAlphaTestFunction
#define grAlphaTestReferenceValue       trap_grAlphaTestReferenceValue
#define grChromakeyMode                 trap_grChromakeyMode
#define grChromakeyValue                trap_grChromakeyValue
#define grClipWindow                    trap_grClipWindow
#define grColorCombine                  trap_grColorCombine
#define grColorMask                     trap_grColorMask
#define grCullMode                      trap_grCullMode
#define grConstantColorValue            trap_grConstantColorValue
#define grDepthBiasLevel                trap_grDepthBiasLevel
#define grDepthBufferFunction           trap_grDepthBufferFunction
#define grDepthBufferMode               trap_grDepthBufferMode
#define grDepthMask                     trap_grDepthMask
#define grDisableAllEffects             trap_grDisableAllEffects
#define grDitherMode                    trap_grDitherMode
#define grFogColorValue                 trap_grFogColorValue
#define grFogMode                       trap_grFogMode
#define grFogTable                      trap_grFogTable
#define grLoadGammaTable                trap_grLoadGammaTable
#define grSplash                        trap_grSplash
#define grGet                           trap_grGet
#define grGetString                     trap_grGetString
#define grQueryResolutions              trap_grQueryResolutions
#define grReset                         trap_grReset
#define grGetProcAddress                trap_grGetProcAddress
#define grEnable                        trap_grEnable
#define grDisable                       trap_grDisable
#define grCoordinateSpace               trap_grCoordinateSpace
#define grDepthRange                    trap_grDepthRange
#define grStippleMode                   trap_grStippleMode
#define grStipplePattern                trap_grStipplePattern
#define grViewport                      trap_grViewport

/*
** texture mapping control functions
*/
#define grTexCalcMemRequired            trap_grTexCalcMemRequired
#define grTexTextureMemRequired         trap_grTexTextureMemRequired
#define grTexMinAddress                 trap_grTexMinAddress
#define grTexMaxAddress                 trap_grTexMaxAddress
#define grTexNCCTable                   trap_grTexNCCTable
#define grTexSource                     trap_grTexSource
#define grTexClampMode                  trap_grTexClampMode
#define grTexCombine                    trap_grTexCombine
#define grTexDetailControl              trap_grTexDetailControl
#define grTexFilterMode                 trap_grTexFilterMode
#define grTexLodBiasValue               trap_grTexLodBiasValue
#define grTexDownloadMipMap             trap_grTexDownloadMipMap
#define grTexDownloadMipMapLevel        trap_grTexDownloadMipMapLevel
#define grTexDownloadMipMapLevelPartial trap_grTexDownloadMipMapLevelPartial
#define grTexDownloadTable              trap_grTexDownloadTable
#define grTexDownloadTablePartial       trap_grTexDownloadTablePartial
#define grTexMipMapMode                 trap_grTexMipMapMode
#define grTexMultibase                  trap_grTexMultibase
#define grTexMultibaseAddress           trap_grTexMultibaseAddress

/*
** linear frame buffer functions
*/
#define grLfbLock                       trap_grLfbLock
#define grLfbUnlock                     trap_grLfbUnlock
#define grLfbConstantAlpha              trap_grLfbConstantAlpha
#define grLfbConstantDepth              trap_grLfbConstantDepth
#define grLfbWriteColorSwizzle          trap_grLfbWriteColorSwizzle
#define grLfbWriteColorFormat           trap_grLfbWriteColorFormat
#define grLfbWriteRegion                trap_grLfbWriteRegion
#define grLfbReadRegion                 trap_grLfbReadRegion

/*
** glide management functions
*/
#define grGlideInit                     trap_grGlideInit
#define grGlideShutdown                 trap_grGlideShutdown
#define grGlideGetState                 trap_grGlideGetState
#define grGlideSetState                 trap_grGlideSetState
#define grGlideGetVertexLayout          trap_grGlideGetVertexLayout
#define grGlideSetVertexLayout          trap_grGlideSetVertexLayout

/*
** glide utility functions
*/
#define guGammaCorrectionRGB            trap_guGammaCorrectionRGB
#define guFogTableIndexToW              trap_guFogTableIndexToW
#define guFogGenerateExp                trap_guFogGenerateExp
#define guFogGenerateExp2               trap_guFogGenerateExp2
#define guFogGenerateLinear             trap_guFogGenerateLinear
#endif /* FX_TRAP_GLIDE_internal */
#endif /* FX_TRAP_GLIDE */



/* <texus.h> */
#define TX_MAX_LEVEL 16
typedef struct _TxMip {
        int format;
        int width;
        int height;
        int depth;
        int size;
        void *data[TX_MAX_LEVEL];
        FxU32 pal[256];
} TxMip;

#define TX_DITHER_NONE                                  0x00000000
#define TX_DITHER_4x4                                   0x00000001
#define TX_DITHER_ERR                                   0x00000002

#define TX_COMPRESSION_STATISTICAL                      0x00000000
#define TX_COMPRESSION_HEURISTIC                        0x00000010
/* <texus.h> */



struct tdfx_glide {
   /*
   ** glide extensions
   */
   void (FX_CALL *grSetNumPendingBuffers) (FxI32 NumPendingBuffers);
   char * (FX_CALL *grGetRegistryOrEnvironmentStringExt) (char *theEntry);
   void (FX_CALL *grGetGammaTableExt) (FxU32 nentries, FxU32 *red, FxU32 *green, FxU32 *blue);
   void (FX_CALL *grChromaRangeModeExt) (GrChromakeyMode_t mode);
   void (FX_CALL *grChromaRangeExt) (GrColor_t color, GrColor_t range, GrChromaRangeMode_t match_mode);
   void (FX_CALL *grTexChromaModeExt) (GrChipID_t tmu, GrChromakeyMode_t mode);
   void (FX_CALL *grTexChromaRangeExt) (GrChipID_t tmu, GrColor_t min, GrColor_t max, GrTexChromakeyMode_t mode);

   /* pointcast */
   void (FX_CALL *grTexDownloadTableExt) (GrChipID_t tmu, GrTexTable_t type, void *data);
   void (FX_CALL *grTexDownloadTablePartialExt) (GrChipID_t tmu, GrTexTable_t type, void *data, int start, int end);
   void (FX_CALL *grTexNCCTableExt) (GrChipID_t tmu, GrNCCTable_t table);

   /* tbext */
   void (FX_CALL *grTextureBufferExt) (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask);
   void (FX_CALL *grTextureAuxBufferExt) (GrChipID_t tmu, FxU32 startAddress, GrLOD_t thisLOD, GrLOD_t largeLOD, GrAspectRatio_t aspectRatio, GrTextureFormat_t format, FxU32 odd_even_mask);
   void (FX_CALL *grAuxBufferExt) (GrBuffer_t buffer);

   /* napalm */
   GrContext_t (FX_CALL *grSstWinOpenExt) (FxU32 hWnd, GrScreenResolution_t resolution, GrScreenRefresh_t refresh, GrColorFormat_t format, GrOriginLocation_t origin, GrPixelFormat_t pixelformat, int nColBuffers, int nAuxBuffers);
   void (FX_CALL *grStencilFuncExt) (GrCmpFnc_t fnc, GrStencil_t ref, GrStencil_t mask);
   void (FX_CALL *grStencilMaskExt) (GrStencil_t value);
   void (FX_CALL *grStencilOpExt) (GrStencilOp_t stencil_fail, GrStencilOp_t depth_fail, GrStencilOp_t depth_pass);
   void (FX_CALL *grLfbConstantStencilExt) (GrStencil_t value);
   void (FX_CALL *grBufferClearExt) (GrColor_t color, GrAlpha_t alpha, FxU32 depth, GrStencil_t stencil);
   void (FX_CALL *grColorCombineExt) (GrCCUColor_t a, GrCombineMode_t a_mode, GrCCUColor_t b, GrCombineMode_t b_mode, GrCCUColor_t c, FxBool c_invert, GrCCUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
   void (FX_CALL *grAlphaCombineExt) (GrACUColor_t a, GrCombineMode_t a_mode, GrACUColor_t b, GrCombineMode_t b_mode, GrACUColor_t c, FxBool c_invert, GrACUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
   void (FX_CALL *grTexColorCombineExt) (GrChipID_t tmu, GrTCCUColor_t a, GrCombineMode_t a_mode, GrTCCUColor_t b, GrCombineMode_t b_mode, GrTCCUColor_t c, FxBool c_invert, GrTCCUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
   void (FX_CALL *grTexAlphaCombineExt) (GrChipID_t tmu, GrTACUColor_t a, GrCombineMode_t a_mode, GrTACUColor_t b, GrCombineMode_t b_mode, GrTACUColor_t c, FxBool c_invert, GrTACUColor_t d, FxBool d_invert, FxU32 shift, FxBool invert);
   void (FX_CALL *grConstantColorValueExt) (GrChipID_t tmu, GrColor_t value);
   void (FX_CALL *grColorMaskExt) (FxBool r, FxBool g, FxBool b, FxBool a);
   void (FX_CALL *grAlphaBlendFunctionExt) (GrAlphaBlendFnc_t rgb_sf, GrAlphaBlendFnc_t rgb_df, GrAlphaBlendOp_t rgb_op, GrAlphaBlendFnc_t alpha_sf, GrAlphaBlendFnc_t alpha_df, GrAlphaBlendOp_t alpha_op);
   void (FX_CALL *grTBufferWriteMaskExt) (FxU32 tmask);

   /*
   ** Texus2 functions
   */
   void (FX_CALL *txImgQuantize) (char *dst, char *src, int w, int h, FxU32 format, FxU32 dither);
   void (FX_CALL *txMipQuantize) (TxMip *pxMip, TxMip *txMip, int fmt, FxU32 d, FxU32 comp);
   void (FX_CALL *txPalToNcc) (GuNccTable *ncc_table, const FxU32 *pal);
};

void tdfx_hook_glide (struct tdfx_glide *Glide, int pointcast);

#endif
