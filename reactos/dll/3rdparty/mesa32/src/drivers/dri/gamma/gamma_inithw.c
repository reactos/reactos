/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *           Kevin E. Martin <martin@valinux.com>
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_inithw.c,v 1.9 2002/10/30 12:51:29 alanh Exp $ */

#include "gamma_context.h"
#include "glint_dri.h"

void gammaInitHW( gammaContextPtr gmesa )
{
    GLINTDRIPtr gDRIPriv = (GLINTDRIPtr)gmesa->driScreen->pDevPriv;
    int i;

    if (gDRIPriv->numMultiDevices == 2) {
	/* Set up each MX's ScanLineOwnership for OpenGL */
	CHECK_DMA_BUFFER(gmesa, 5);
	WRITE(gmesa->buf, BroadcastMask, 1);
	WRITE(gmesa->buf, ScanLineOwnership, 5); /* Use bottom left as [0,0] */
	WRITE(gmesa->buf, BroadcastMask, 2);
	WRITE(gmesa->buf, ScanLineOwnership, 1); /* Use bottom left as [0,0] */
	/* Broadcast to both MX's */
	WRITE(gmesa->buf, BroadcastMask, 3);
	FLUSH_DMA_BUFFER(gmesa); 
    }

    gmesa->AlphaBlendMode = (AlphaBlendModeDisable |
			     AB_Src_One |
			     AB_Dst_Zero |
			     AB_NoAlphaBufferPresent |
			     AB_ColorFmt_8888 |
			     AB_ColorOrder_RGB |
			     AB_OpenGLType |
			     AB_AlphaDst_FBData |
			     AB_ColorConversionScale |
			     AB_AlphaConversionScale);

    gmesa->DitherMode = DitherModeEnable | DM_ColorOrder_RGB;

    switch (gmesa->gammaScreen->cpp) {
	case 2:
    		gmesa->DitherMode |= DM_ColorFmt_5555;
		gmesa->AlphaBlendMode |= AB_ColorFmt_5555;
		CHECK_DMA_BUFFER(gmesa, 1);
		WRITE(gmesa->buf, PixelSize, 1);
		break;
	case 4:
    		gmesa->DitherMode |= DM_ColorFmt_8888;
		gmesa->AlphaBlendMode |= AB_ColorFmt_8888;
		WRITE(gmesa->buf, PixelSize, 0);
		break;
    }

    /* FIXME for stencil, gid, etc */
    switch (gmesa->DepthSize) {
	case 16:
   		gmesa->LBReadFormat = 
			 (LBRF_DepthWidth16    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos16    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos24 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos32         );
   		gmesa->LBWriteFormat = 
			 (LBRF_DepthWidth16    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos16    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos24 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos32         );
		break;
	case 24:
   		gmesa->LBReadFormat = 
			 (LBRF_DepthWidth24    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos24    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos32 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos36         );
   		gmesa->LBWriteFormat = 
			 (LBRF_DepthWidth24    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos24    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos32 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos36         );
		break;
	case 32:
   		gmesa->LBReadFormat = 
			 (LBRF_DepthWidth32    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos32    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos40 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos44         );
   		gmesa->LBWriteFormat = 
			 (LBRF_DepthWidth32    | 
                          LBRF_StencilWidth8   |
                          LBRF_StencilPos32    |
                          LBRF_FrameCount8     |
                          LBRF_FrameCountPos40 |
                          LBRF_GIDWidth4       |
                          LBRF_GIDPos44         );
		break;
    }

    gmesa->FBHardwareWriteMask = 0xffffffff;
    gmesa->FogMode = FogModeDisable;
    gmesa->ClearDepth = 0xffffffff;
    gmesa->AreaStippleMode = AreaStippleModeDisable;
    gmesa->x = 0;
    gmesa->y = 0;
    gmesa->w = 0;
    gmesa->h = 0;
    gmesa->FrameCount = 0;
    gmesa->MatrixMode = GL_MODELVIEW;
    gmesa->ModelViewCount = 0;
    gmesa->ProjCount = 0;
    gmesa->TextureCount = 0;
    gmesa->PointMode = PM_AntialiasQuality_4x4;
    gmesa->LineMode = LM_AntialiasQuality_4x4;
    gmesa->TriangleMode = TM_AntialiasQuality_4x4;
    gmesa->AntialiasMode = AntialiasModeDisable;

    for (i = 0; i < 16; i++)
	if (i % 5 == 0)
	    gmesa->ModelView[i] =
		gmesa->Proj[i] =
		gmesa->ModelViewProj[i] =
		gmesa->Texture[i] = 1.0;
	else
	    gmesa->ModelView[i] =
		gmesa->Proj[i] =
		gmesa->ModelViewProj[i] =
		gmesa->Texture[i] = 0.0;

    gmesa->LBReadMode = (LBReadSrcDisable |
			 LBReadDstDisable |
			 LBDataTypeDefault |
			 LBWindowOriginBot | 
			 gDRIPriv->pprod);
    gmesa->FBReadMode = (FBReadSrcDisable |
			 FBReadDstDisable |
			 FBDataTypeDefault |
			 FBWindowOriginBot |  
			 gDRIPriv->pprod);
 
    if (gDRIPriv->numMultiDevices == 2) {
	gmesa->LBReadMode |= LBScanLineInt2;
	gmesa->FBReadMode |= FBScanLineInt2;
        gmesa->LBWindowBase = gmesa->driScreen->fbWidth *
			     (gmesa->driScreen->fbHeight/2 - 1);
    	gmesa->FBWindowBase = gmesa->driScreen->fbWidth * 
			     (gmesa->driScreen->fbHeight/2 - 1);
    } else {
        gmesa->LBWindowBase = gmesa->driScreen->fbWidth *
			     (gmesa->driScreen->fbHeight - 1);
    	gmesa->FBWindowBase = gmesa->driScreen->fbWidth * 
			     (gmesa->driScreen->fbHeight - 1);
    }

    gmesa->Begin = (B_AreaStippleDisable |
		    B_LineStippleDisable |
		    B_AntiAliasDisable |
		    B_TextureDisable |
		    B_FogDisable |
		    B_SubPixelCorrectEnable |
		    B_PrimType_Null);

    gmesa->ColorDDAMode = (ColorDDAEnable |
			   ColorDDAGouraud);

    gmesa->GeometryMode = (GM_TextureDisable |
			   GM_FogDisable |
			   GM_FogExp |
			   GM_FrontPolyFill |
			   GM_BackPolyFill |
			   GM_FrontFaceCCW |
			   GM_PolyCullDisable |
			   GM_PolyCullBack |
			   GM_ClipShortLinesDisable |
			   GM_ClipSmallTrisDisable |
			   GM_RenderMode |
			   GM_Feedback2D |
			   GM_CullFaceNormDisable |
			   GM_AutoFaceNormDisable |
			   GM_GouraudShading |
			   GM_UserClipNone |
			   GM_PolyOffsetPointDisable |
			   GM_PolyOffsetLineDisable |
			   GM_PolyOffsetFillDisable |
			   GM_InvertFaceNormCullDisable);

    gmesa->AlphaTestMode = (AlphaTestModeDisable |
			    AT_Always);

    gmesa->AB_FBReadMode_Save = gmesa->AB_FBReadMode = 0;

    gmesa->Window = (WindowEnable  | /* For GID testing */
		     W_PassIfEqual |
		     (0 << 5)); /* GID part is set from draw priv (below) */

    gmesa->NotClipped = GL_FALSE;
    gmesa->WindowChanged = GL_TRUE;

    gmesa->Texture1DEnabled = GL_FALSE;
    gmesa->Texture2DEnabled = GL_FALSE;

    gmesa->DepthMode |= (DepthModeDisable |
			DM_WriteMask |
			DM_Less);

    gmesa->DeltaMode |= (DM_SubPixlCorrectionEnable |
			DM_SmoothShadingEnable |
			DM_Target500TXMX);

    gmesa->LightingMode = LightingModeDisable | LightingModeSpecularEnable;
    gmesa->Light0Mode = LNM_Off;
    gmesa->Light1Mode = LNM_Off;
    gmesa->Light2Mode = LNM_Off;
    gmesa->Light3Mode = LNM_Off;
    gmesa->Light4Mode = LNM_Off;
    gmesa->Light5Mode = LNM_Off;
    gmesa->Light6Mode = LNM_Off;
    gmesa->Light7Mode = LNM_Off;
    gmesa->Light8Mode = LNM_Off;
    gmesa->Light9Mode = LNM_Off;
    gmesa->Light10Mode = LNM_Off;
    gmesa->Light11Mode = LNM_Off;
    gmesa->Light12Mode = LNM_Off;
    gmesa->Light13Mode = LNM_Off;
    gmesa->Light14Mode = LNM_Off;
    gmesa->Light15Mode = LNM_Off;

    gmesa->LogicalOpMode = LogicalOpModeDisable;

    gmesa->MaterialMode = MaterialModeDisable;

    gmesa->ScissorMode = UserScissorDisable | ScreenScissorDisable;

    gmesa->TransformMode = XM_UseModelViewProjMatrix |
				    XM_TexGenModeS_None |
				    XM_TexGenModeT_None |
				    XM_TexGenModeR_None |
				    XM_TexGenModeQ_None;

    CHECK_DMA_BUFFER(gmesa, 20);
    WRITE(gmesa->buf, LineStippleMode, 0);
    WRITE(gmesa->buf, RouterMode, 0);
    WRITE(gmesa->buf, TextureAddressMode, 0);
    WRITE(gmesa->buf, TextureReadMode, 0);
    WRITE(gmesa->buf, TextureFilterMode, 0);
    WRITE(gmesa->buf, TextureColorMode, 0);
    WRITE(gmesa->buf, StencilMode, 0);
    WRITE(gmesa->buf, PatternRamMode, 0);
    WRITE(gmesa->buf, ChromaTestMode, 0);
    WRITE(gmesa->buf, StatisticMode, 0);
    WRITE(gmesa->buf, AreaStippleMode, gmesa->AreaStippleMode);
    WRITE(gmesa->buf, ScissorMode, gmesa->ScissorMode);
    WRITE(gmesa->buf, FogMode, gmesa->FogMode);
    WRITE(gmesa->buf, AntialiasMode, gmesa->AntialiasMode);
    WRITE(gmesa->buf, LogicalOpMode, gmesa->LogicalOpMode);
    WRITE(gmesa->buf, TriangleMode, gmesa->TriangleMode);
    WRITE(gmesa->buf, PointMode, gmesa->PointMode);
    WRITE(gmesa->buf, LineMode, gmesa->LineMode);
    WRITE(gmesa->buf, LBWriteFormat, gmesa->LBWriteFormat);
    WRITE(gmesa->buf, LBReadFormat,  gmesa->LBReadFormat);

    /* Framebuffer initialization */
    CHECK_DMA_BUFFER(gmesa, 10);
    WRITE(gmesa->buf, FBSourceData, 0);
    WRITE(gmesa->buf, FBReadMode, gmesa->FBReadMode);
    if (gmesa->EnabledFlags & GAMMA_BACK_BUFFER) {
	if (gDRIPriv->numMultiDevices == 2) {
	    WRITE(gmesa->buf, FBPixelOffset,
	      (gmesa->driScreen->fbHeight/2)*gmesa->driScreen->fbWidth);
	} else {
	    WRITE(gmesa->buf, FBPixelOffset,
	      gmesa->driScreen->fbHeight*gmesa->driScreen->fbWidth);
	}
    } else
	WRITE(gmesa->buf, FBPixelOffset, 0);
    WRITE(gmesa->buf, FBSourceOffset, 0);
    WRITE(gmesa->buf, FBHardwareWriteMask, 0xffffffff);
    WRITE(gmesa->buf, FBSoftwareWriteMask, 0xffffffff);
    WRITE(gmesa->buf, FBWriteMode, FBWriteModeEnable);
    WRITE(gmesa->buf, FBWindowBase, gmesa->FBWindowBase);
    WRITE(gmesa->buf, ScreenSize, ((gmesa->driScreen->fbHeight << 16) |
				 (gmesa->driScreen->fbWidth)));
    WRITE(gmesa->buf, WindowOrigin, 0x00000000);

    /* Localbuffer initialization */
    CHECK_DMA_BUFFER(gmesa, 5);
    WRITE(gmesa->buf, LBReadMode, gmesa->LBReadMode);
    WRITE(gmesa->buf, LBSourceOffset, 0);
    WRITE(gmesa->buf, LBWriteMode, LBWriteModeEnable);
    WRITE(gmesa->buf, LBWindowOffset, 0);
    WRITE(gmesa->buf, LBWindowBase, gmesa->LBWindowBase);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, Rectangle2DControl, 1);

    CHECK_DMA_BUFFER(gmesa, 11);
    WRITE(gmesa->buf, DepthMode, gmesa->DepthMode);
    WRITE(gmesa->buf, ColorDDAMode, gmesa->ColorDDAMode);
    WRITE(gmesa->buf, FBBlockColor, 0x00000000);
    WRITE(gmesa->buf, ConstantColor, 0x00000000);
    WRITE(gmesa->buf, AlphaTestMode, gmesa->AlphaTestMode);
    WRITE(gmesa->buf, AlphaBlendMode, gmesa->AlphaBlendMode);
    WRITE(gmesa->buf, DitherMode, gmesa->DitherMode);
    if (gDRIPriv->numMultiDevices == 2)
    	WRITE(gmesa->buf, RasterizerMode, RM_MultiGLINT | RM_BiasCoordNearHalf);
    else
    	WRITE(gmesa->buf, RasterizerMode, RM_BiasCoordNearHalf);
    WRITE(gmesa->buf, GLINTWindow, gmesa->Window);
    WRITE(gmesa->buf, FastClearDepth, gmesa->ClearDepth);
    WRITE(gmesa->buf, GLINTDepth, gmesa->ClearDepth);

    CHECK_DMA_BUFFER(gmesa, 1);
    WRITE(gmesa->buf, EdgeFlag, EdgeFlagEnable);

    CHECK_DMA_BUFFER(gmesa, 16);
    WRITEF(gmesa->buf, ModelViewMatrix0,  1.0);
    WRITEF(gmesa->buf, ModelViewMatrix1,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix2,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix3,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix4,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix5,  1.0);
    WRITEF(gmesa->buf, ModelViewMatrix6,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix7,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix8,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix9,  0.0);
    WRITEF(gmesa->buf, ModelViewMatrix10, 1.0);
    WRITEF(gmesa->buf, ModelViewMatrix11, 0.0);
    WRITEF(gmesa->buf, ModelViewMatrix12, 0.0);
    WRITEF(gmesa->buf, ModelViewMatrix13, 0.0);
    WRITEF(gmesa->buf, ModelViewMatrix14, 0.0);
    WRITEF(gmesa->buf, ModelViewMatrix15, 1.0);

    CHECK_DMA_BUFFER(gmesa, 16);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix0,  1.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix1,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix2,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix3,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix4,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix5,  1.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix6,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix7,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix8,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix9,  0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix10, 1.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix11, 0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix12, 0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix13, 0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix14, 0.0);
    WRITEF(gmesa->buf, ModelViewProjectionMatrix15, 1.0);

    CHECK_DMA_BUFFER(gmesa, 16);
    WRITEF(gmesa->buf, TextureMatrix0,  1.0);
    WRITEF(gmesa->buf, TextureMatrix1,  0.0);
    WRITEF(gmesa->buf, TextureMatrix2,  0.0);
    WRITEF(gmesa->buf, TextureMatrix3,  0.0);
    WRITEF(gmesa->buf, TextureMatrix4,  0.0);
    WRITEF(gmesa->buf, TextureMatrix5,  1.0);
    WRITEF(gmesa->buf, TextureMatrix6,  0.0);
    WRITEF(gmesa->buf, TextureMatrix7,  0.0);
    WRITEF(gmesa->buf, TextureMatrix8,  0.0);
    WRITEF(gmesa->buf, TextureMatrix9,  0.0);
    WRITEF(gmesa->buf, TextureMatrix10, 1.0);
    WRITEF(gmesa->buf, TextureMatrix11, 0.0);
    WRITEF(gmesa->buf, TextureMatrix12, 0.0);
    WRITEF(gmesa->buf, TextureMatrix13, 0.0);
    WRITEF(gmesa->buf, TextureMatrix14, 0.0);
    WRITEF(gmesa->buf, TextureMatrix15, 1.0);

    CHECK_DMA_BUFFER(gmesa, 16);
    WRITEF(gmesa->buf, TexGen0,  0.0);
    WRITEF(gmesa->buf, TexGen1,  0.0);
    WRITEF(gmesa->buf, TexGen2,  0.0);
    WRITEF(gmesa->buf, TexGen3,  0.0);
    WRITEF(gmesa->buf, TexGen4,  0.0);
    WRITEF(gmesa->buf, TexGen5,  0.0);
    WRITEF(gmesa->buf, TexGen6,  0.0);
    WRITEF(gmesa->buf, TexGen7,  0.0);
    WRITEF(gmesa->buf, TexGen8,  0.0);
    WRITEF(gmesa->buf, TexGen9,  0.0);
    WRITEF(gmesa->buf, TexGen10, 0.0);
    WRITEF(gmesa->buf, TexGen11, 0.0);
    WRITEF(gmesa->buf, TexGen12, 0.0);
    WRITEF(gmesa->buf, TexGen13, 0.0);
    WRITEF(gmesa->buf, TexGen14, 0.0);
    WRITEF(gmesa->buf, TexGen15, 0.0);

    CHECK_DMA_BUFFER(gmesa, 9);
    WRITEF(gmesa->buf, NormalMatrix0, 1.0);
    WRITEF(gmesa->buf, NormalMatrix1, 0.0);
    WRITEF(gmesa->buf, NormalMatrix2, 0.0);
    WRITEF(gmesa->buf, NormalMatrix3, 0.0);
    WRITEF(gmesa->buf, NormalMatrix4, 1.0);
    WRITEF(gmesa->buf, NormalMatrix5, 0.0);
    WRITEF(gmesa->buf, NormalMatrix6, 0.0);
    WRITEF(gmesa->buf, NormalMatrix7, 0.0);
    WRITEF(gmesa->buf, NormalMatrix8, 1.0);

    CHECK_DMA_BUFFER(gmesa, 3);
    WRITEF(gmesa->buf, FogDensity, 0.0);
    WRITEF(gmesa->buf, FogEnd,     0.0);
    WRITEF(gmesa->buf, FogScale,   0.0);

    CHECK_DMA_BUFFER(gmesa, 2);
    WRITEF(gmesa->buf, LineClipLengthThreshold,   0.0);
    WRITEF(gmesa->buf, TriangleClipAreaThreshold, 0.0);

    CHECK_DMA_BUFFER(gmesa, 5);
    WRITE(gmesa->buf, GeometryMode, gmesa->GeometryMode);
    WRITE(gmesa->buf, NormalizeMode, NormalizeModeDisable);
    WRITE(gmesa->buf, LightingMode, gmesa->LightingMode);
    WRITE(gmesa->buf, ColorMaterialMode, ColorMaterialModeDisable);
    WRITE(gmesa->buf, MaterialMode, MaterialModeDisable);

    CHECK_DMA_BUFFER(gmesa, 2);
    WRITE(gmesa->buf, FrontSpecularExponent, 0); /* fixed point */
    WRITE(gmesa->buf, BackSpecularExponent,  0); /* fixed point */

    CHECK_DMA_BUFFER(gmesa, 29);
    WRITEF(gmesa->buf, FrontAmbientColorRed,    0.2);
    WRITEF(gmesa->buf, FrontAmbientColorGreen,  0.2);
    WRITEF(gmesa->buf, FrontAmbientColorBlue,   0.2);
    WRITEF(gmesa->buf, BackAmbientColorRed,     0.2);
    WRITEF(gmesa->buf, BackAmbientColorGreen,   0.2);
    WRITEF(gmesa->buf, BackAmbientColorBlue,    0.2);
    WRITEF(gmesa->buf, FrontDiffuseColorRed,    0.8);
    WRITEF(gmesa->buf, FrontDiffuseColorGreen,  0.8);
    WRITEF(gmesa->buf, FrontDiffuseColorBlue,   0.8);
    WRITEF(gmesa->buf, BackDiffuseColorRed,     0.8);
    WRITEF(gmesa->buf, BackDiffuseColorGreen,   0.8);
    WRITEF(gmesa->buf, BackDiffuseColorBlue,    0.8);
    WRITEF(gmesa->buf, FrontSpecularColorRed,   0.0);
    WRITEF(gmesa->buf, FrontSpecularColorGreen, 0.0);
    WRITEF(gmesa->buf, FrontSpecularColorBlue,  0.0);
    WRITEF(gmesa->buf, BackSpecularColorRed,    0.0);
    WRITEF(gmesa->buf, BackSpecularColorGreen,  0.0);
    WRITEF(gmesa->buf, BackSpecularColorBlue,   0.0);
    WRITEF(gmesa->buf, FrontEmissiveColorRed,   0.0);
    WRITEF(gmesa->buf, FrontEmissiveColorGreen, 0.0);
    WRITEF(gmesa->buf, FrontEmissiveColorBlue,  0.0);
    WRITEF(gmesa->buf, BackEmissiveColorRed,    0.0);
    WRITEF(gmesa->buf, BackEmissiveColorGreen,  0.0);
    WRITEF(gmesa->buf, BackEmissiveColorBlue,   0.0);
    WRITEF(gmesa->buf, SceneAmbientColorRed,    0.2);
    WRITEF(gmesa->buf, SceneAmbientColorGreen,  0.2);
    WRITEF(gmesa->buf, SceneAmbientColorBlue,   0.2);
    WRITEF(gmesa->buf, FrontAlpha,              1.0);
    WRITEF(gmesa->buf, BackAlpha,               1.0);

    CHECK_DMA_BUFFER(gmesa, 7);
    WRITE(gmesa->buf, PointSize, 1);
    WRITEF(gmesa->buf, AApointSize, 1.0);
    WRITE(gmesa->buf, LineWidth, 1);
    WRITEF(gmesa->buf, AAlineWidth, 1.0);
    WRITE(gmesa->buf, LineWidthOffset, 0);
    WRITE(gmesa->buf, TransformMode, gmesa->TransformMode);
    WRITE(gmesa->buf, DeltaMode, gmesa->DeltaMode);

    CHECK_DMA_BUFFER(gmesa, 16);
    WRITE(gmesa->buf, Light0Mode,  LNM_Off);
    WRITE(gmesa->buf, Light1Mode,  LNM_Off);
    WRITE(gmesa->buf, Light2Mode,  LNM_Off);
    WRITE(gmesa->buf, Light3Mode,  LNM_Off);
    WRITE(gmesa->buf, Light4Mode,  LNM_Off);
    WRITE(gmesa->buf, Light5Mode,  LNM_Off);
    WRITE(gmesa->buf, Light6Mode,  LNM_Off);
    WRITE(gmesa->buf, Light7Mode,  LNM_Off);
    WRITE(gmesa->buf, Light8Mode,  LNM_Off);
    WRITE(gmesa->buf, Light9Mode,  LNM_Off);
    WRITE(gmesa->buf, Light10Mode, LNM_Off);
    WRITE(gmesa->buf, Light11Mode, LNM_Off);
    WRITE(gmesa->buf, Light12Mode, LNM_Off);
    WRITE(gmesa->buf, Light13Mode, LNM_Off);
    WRITE(gmesa->buf, Light14Mode, LNM_Off);
    WRITE(gmesa->buf, Light15Mode, LNM_Off);

    CHECK_DMA_BUFFER(gmesa, 22);
    WRITEF(gmesa->buf, Light0AmbientIntensityBlue, 0.0);
    WRITEF(gmesa->buf, Light0AmbientIntensityGreen, 0.0);
    WRITEF(gmesa->buf, Light0AmbientIntensityRed, 0.0);
    WRITEF(gmesa->buf, Light0DiffuseIntensityBlue, 1.0);
    WRITEF(gmesa->buf, Light0DiffuseIntensityGreen, 1.0);
    WRITEF(gmesa->buf, Light0DiffuseIntensityRed, 1.0);
    WRITEF(gmesa->buf, Light0SpecularIntensityBlue, 1.0);
    WRITEF(gmesa->buf, Light0SpecularIntensityGreen, 1.0);
    WRITEF(gmesa->buf, Light0SpecularIntensityRed, 1.0);
    WRITEF(gmesa->buf, Light0SpotlightDirectionZ, 0.0);
    WRITEF(gmesa->buf, Light0SpotlightDirectionY, 0.0);
    WRITEF(gmesa->buf, Light0SpotlightDirectionX, -1.0);
    WRITEF(gmesa->buf, Light0SpotlightExponent, 0.0);
    WRITEF(gmesa->buf, Light0PositionZ, 0.0);
    WRITEF(gmesa->buf, Light0PositionY, 0.0);
    WRITEF(gmesa->buf, Light0PositionX, 1.0);
    WRITEF(gmesa->buf, Light0PositionW, 0.0);
    WRITEF(gmesa->buf, Light0CosSpotlightCutoffAngle, -1.0);
    WRITEF(gmesa->buf, Light0ConstantAttenuation, 1.0);
    WRITEF(gmesa->buf, Light0LinearAttenuation,   0.0);
    WRITEF(gmesa->buf, Light0QuadraticAttenuation,0.0);

    CHECK_DMA_BUFFER(gmesa, 2);
    WRITEF(gmesa->buf, XBias, 0.0);
    WRITEF(gmesa->buf, YBias, 0.0);

    CHECK_DMA_BUFFER(gmesa, 6);
    WRITEF(gmesa->buf, ViewPortScaleX, gmesa->driScreen->fbWidth/4);
    WRITEF(gmesa->buf, ViewPortScaleY, gmesa->driScreen->fbHeight/4);
    WRITEF(gmesa->buf, ViewPortScaleZ, 1.0f);
    WRITEF(gmesa->buf, ViewPortOffsetX, gmesa->x);
    WRITEF(gmesa->buf, ViewPortOffsetY, gmesa->y);
    WRITEF(gmesa->buf, ViewPortOffsetZ, 0.0f);

    CHECK_DMA_BUFFER(gmesa, 3);
    WRITEF(gmesa->buf, Nz, 1.0);
    WRITEF(gmesa->buf, Ny, 0.0);
    WRITEF(gmesa->buf, Nx, 0.0);

    /* Send the initialization commands to the HW */
    FLUSH_DMA_BUFFER(gmesa);
}
