// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------
//

//
// This file was generated, please do not edit it directly.
//
//
//---------------------------------------------------------------------------

internal enum MILCMD
{                                   
    /* 0x00 */ MilCmdInvalid                                 = 0x00,

    //--------------------------------------------------------------------------
    //
    //  Media Integration Layer Commands
    //
    //--------------------------------------------------------------------------

    /* 0x01 */ MilCmdTransportSyncFlush                      = 0x01,
    /* 0x02 */ MilCmdTransportDestroyResourcesOnChannel      = 0x02,
    /* 0x03 */ MilCmdPartitionRegisterForNotifications       = 0x03,
    /* 0x04 */ MilCmdChannelRequestTier                      = 0x04,
    /* 0x05 */ MilCmdPartitionSetVBlankSyncMode              = 0x05,
    /* 0x06 */ MilCmdPartitionNotifyPresent                  = 0x06,
    /* 0x07 */ MilCmdChannelCreateResource                   = 0x07,
    /* 0x08 */ MilCmdChannelDeleteResource                   = 0x08,
    /* 0x09 */ MilCmdChannelDuplicateHandle                  = 0x09,
    /* 0x0a */ MilCmdD3DImage                                = 0x0a,
    /* 0x0b */ MilCmdD3DImagePresent                         = 0x0b,
    /* 0x0c */ MilCmdBitmapSource                            = 0x0c,
    /* 0x0d */ MilCmdBitmapInvalidate                        = 0x0d,
    /* 0x0e */ MilCmdDoubleResource                          = 0x0e,
    /* 0x0f */ MilCmdColorResource                           = 0x0f,
    /* 0x10 */ MilCmdPointResource                           = 0x10,
    /* 0x11 */ MilCmdRectResource                            = 0x11,
    /* 0x12 */ MilCmdSizeResource                            = 0x12,
    /* 0x13 */ MilCmdMatrixResource                          = 0x13,
    /* 0x14 */ MilCmdPoint3DResource                         = 0x14,
    /* 0x15 */ MilCmdVector3DResource                        = 0x15,
    /* 0x16 */ MilCmdQuaternionResource                      = 0x16,
    /* 0x17 */ MilCmdMediaPlayer                             = 0x17,
    /* 0x18 */ MilCmdRenderData                              = 0x18,
    /* 0x19 */ MilCmdEtwEventResource                        = 0x19,
    /* 0x1a */ MilCmdVisualCreate                            = 0x1a,
    /* 0x1b */ MilCmdVisualSetOffset                         = 0x1b,
    /* 0x1c */ MilCmdVisualSetTransform                      = 0x1c,
    /* 0x1d */ MilCmdVisualSetEffect                         = 0x1d,
    /* 0x1e */ MilCmdVisualSetCacheMode                      = 0x1e,
    /* 0x1f */ MilCmdVisualSetClip                           = 0x1f,
    /* 0x20 */ MilCmdVisualSetAlpha                          = 0x20,
    /* 0x21 */ MilCmdVisualSetRenderOptions                  = 0x21,
    /* 0x22 */ MilCmdVisualSetContent                        = 0x22,
    /* 0x23 */ MilCmdVisualSetAlphaMask                      = 0x23,
    /* 0x24 */ MilCmdVisualRemoveAllChildren                 = 0x24,
    /* 0x25 */ MilCmdVisualRemoveChild                       = 0x25,
    /* 0x26 */ MilCmdVisualInsertChildAt                     = 0x26,
    /* 0x27 */ MilCmdVisualSetGuidelineCollection            = 0x27,
    /* 0x28 */ MilCmdVisualSetScrollableAreaClip             = 0x28,
    /* 0x29 */ MilCmdViewport3DVisualSetCamera               = 0x29,
    /* 0x2a */ MilCmdViewport3DVisualSetViewport             = 0x2a,
    /* 0x2b */ MilCmdViewport3DVisualSet3DChild              = 0x2b,
    /* 0x2c */ MilCmdVisual3DSetContent                      = 0x2c,
    /* 0x2d */ MilCmdVisual3DSetTransform                    = 0x2d,
    /* 0x2e */ MilCmdVisual3DRemoveAllChildren               = 0x2e,
    /* 0x2f */ MilCmdVisual3DRemoveChild                     = 0x2f,
    /* 0x30 */ MilCmdVisual3DInsertChildAt                   = 0x30,
    /* 0x31 */ MilCmdHwndTargetCreate                        = 0x31,
    /* 0x32 */ MilCmdHwndTargetSuppressLayered               = 0x32,
    /* 0x33 */ MilCmdTargetUpdateWindowSettings              = 0x33,
    /* 0x34 */ MilCmdGenericTargetCreate                     = 0x34,
    /* 0x35 */ MilCmdTargetSetRoot                           = 0x35,
    /* 0x36 */ MilCmdTargetSetClearColor                     = 0x36,
    /* 0x37 */ MilCmdTargetInvalidate                        = 0x37,
    /* 0x38 */ MilCmdTargetSetFlags                          = 0x38,
    /* 0x39 */ MilCmdHwndTargetDpiChanged                    = 0x39,
    /* 0x3a */ MilCmdGlyphRunCreate                          = 0x3a,
    /* 0x3b */ MilCmdDoubleBufferedBitmap                    = 0x3b,
    /* 0x3c */ MilCmdDoubleBufferedBitmapCopyForward         = 0x3c,
    /* 0x3d */ MilCmdPartitionNotifyPolicyChangeForNonInteractiveMode = 0x3d,


    //--------------------------------------------------------------------------
    //
    //  Render Data Commands
    //
    //--------------------------------------------------------------------------

    /* 0x3e */ MilDrawLine                                   = 0x3e,
    /* 0x3f */ MilDrawLineAnimate                            = 0x3f,
    /* 0x40 */ MilDrawRectangle                              = 0x40,
    /* 0x41 */ MilDrawRectangleAnimate                       = 0x41,
    /* 0x42 */ MilDrawRoundedRectangle                       = 0x42,
    /* 0x43 */ MilDrawRoundedRectangleAnimate                = 0x43,
    /* 0x44 */ MilDrawEllipse                                = 0x44,
    /* 0x45 */ MilDrawEllipseAnimate                         = 0x45,
    /* 0x46 */ MilDrawGeometry                               = 0x46,
    /* 0x47 */ MilDrawImage                                  = 0x47,
    /* 0x48 */ MilDrawImageAnimate                           = 0x48,
    /* 0x49 */ MilDrawGlyphRun                               = 0x49,
    /* 0x4a */ MilDrawDrawing                                = 0x4a,
    /* 0x4b */ MilDrawVideo                                  = 0x4b,
    /* 0x4c */ MilDrawVideoAnimate                           = 0x4c,
    /* 0x4d */ MilPushClip                                   = 0x4d,
    /* 0x4e */ MilPushOpacityMask                            = 0x4e,
    /* 0x4f */ MilPushOpacity                                = 0x4f,
    /* 0x50 */ MilPushOpacityAnimate                         = 0x50,
    /* 0x51 */ MilPushTransform                              = 0x51,
    /* 0x52 */ MilPushGuidelineSet                           = 0x52,
    /* 0x53 */ MilPushGuidelineY1                            = 0x53,
    /* 0x54 */ MilPushGuidelineY2                            = 0x54,
    /* 0x55 */ MilPushEffect                                 = 0x55,
    /* 0x56 */ MilPop                                        = 0x56,


    //--------------------------------------------------------------------------
    //
    //  MIL resources
    //
    //--------------------------------------------------------------------------

    /* 0x57 */ MilCmdAxisAngleRotation3D                     = 0x57,
    /* 0x58 */ MilCmdQuaternionRotation3D                    = 0x58,
    /* 0x59 */ MilCmdPerspectiveCamera                       = 0x59,
    /* 0x5a */ MilCmdOrthographicCamera                      = 0x5a,
    /* 0x5b */ MilCmdMatrixCamera                            = 0x5b,
    /* 0x5c */ MilCmdModel3DGroup                            = 0x5c,
    /* 0x5d */ MilCmdAmbientLight                            = 0x5d,
    /* 0x5e */ MilCmdDirectionalLight                        = 0x5e,
    /* 0x5f */ MilCmdPointLight                              = 0x5f,
    /* 0x60 */ MilCmdSpotLight                               = 0x60,
    /* 0x61 */ MilCmdGeometryModel3D                         = 0x61,
    /* 0x62 */ MilCmdMeshGeometry3D                          = 0x62,
    /* 0x63 */ MilCmdMaterialGroup                           = 0x63,
    /* 0x64 */ MilCmdDiffuseMaterial                         = 0x64,
    /* 0x65 */ MilCmdSpecularMaterial                        = 0x65,
    /* 0x66 */ MilCmdEmissiveMaterial                        = 0x66,
    /* 0x67 */ MilCmdTransform3DGroup                        = 0x67,
    /* 0x68 */ MilCmdTranslateTransform3D                    = 0x68,
    /* 0x69 */ MilCmdScaleTransform3D                        = 0x69,
    /* 0x6a */ MilCmdRotateTransform3D                       = 0x6a,
    /* 0x6b */ MilCmdMatrixTransform3D                       = 0x6b,
    /* 0x6c */ MilCmdPixelShader                             = 0x6c,
    /* 0x6d */ MilCmdImplicitInputBrush                      = 0x6d,
    /* 0x6e */ MilCmdBlurEffect                              = 0x6e,
    /* 0x6f */ MilCmdDropShadowEffect                        = 0x6f,
    /* 0x70 */ MilCmdShaderEffect                            = 0x70,
    /* 0x71 */ MilCmdDrawingImage                            = 0x71,
    /* 0x72 */ MilCmdTransformGroup                          = 0x72,
    /* 0x73 */ MilCmdTranslateTransform                      = 0x73,
    /* 0x74 */ MilCmdScaleTransform                          = 0x74,
    /* 0x75 */ MilCmdSkewTransform                           = 0x75,
    /* 0x76 */ MilCmdRotateTransform                         = 0x76,
    /* 0x77 */ MilCmdMatrixTransform                         = 0x77,
    /* 0x78 */ MilCmdLineGeometry                            = 0x78,
    /* 0x79 */ MilCmdRectangleGeometry                       = 0x79,
    /* 0x7a */ MilCmdEllipseGeometry                         = 0x7a,
    /* 0x7b */ MilCmdGeometryGroup                           = 0x7b,
    /* 0x7c */ MilCmdCombinedGeometry                        = 0x7c,
    /* 0x7d */ MilCmdPathGeometry                            = 0x7d,
    /* 0x7e */ MilCmdSolidColorBrush                         = 0x7e,
    /* 0x7f */ MilCmdLinearGradientBrush                     = 0x7f,
    /* 0x80 */ MilCmdRadialGradientBrush                     = 0x80,
    /* 0x81 */ MilCmdImageBrush                              = 0x81,
    /* 0x82 */ MilCmdDrawingBrush                            = 0x82,
    /* 0x83 */ MilCmdVisualBrush                             = 0x83,
    /* 0x84 */ MilCmdBitmapCacheBrush                        = 0x84,
    /* 0x85 */ MilCmdDashStyle                               = 0x85,
    /* 0x86 */ MilCmdPen                                     = 0x86,
    /* 0x87 */ MilCmdGeometryDrawing                         = 0x87,
    /* 0x88 */ MilCmdGlyphRunDrawing                         = 0x88,
    /* 0x89 */ MilCmdImageDrawing                            = 0x89,
    /* 0x8a */ MilCmdVideoDrawing                            = 0x8a,
    /* 0x8b */ MilCmdDrawingGroup                            = 0x8b,
    /* 0x8c */ MilCmdGuidelineSet                            = 0x8c,
    /* 0x8d */ MilCmdBitmapCache                             = 0x8d,

#if DBG
    //
    // This command should always remain at the end of the list. It is
    // not actually a command - rather it is used to validate the internal
    // structure mapping to the enum.
    //
    // NOTE: if you put anything after this, you have broken the debugger
    // extension. Also, there will be a mismatch of enum IDs between
    // debug/retail and managed/unmanaged code.
    //

    /* 0x8e */ MilCmdValidateStructureOrder                  = 0x8e
#endif
};



