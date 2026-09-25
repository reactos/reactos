// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



using System;
using System.Runtime.InteropServices;

namespace System.Windows.Media.Composition
{

    internal enum MIL_SEGMENT_TYPE    {
        MilSegmentNone,
        MilSegmentLine,
        MilSegmentBezier,
        MilSegmentQuadraticBezier,
        MilSegmentArc,
        MilSegmentPolyLine,
        MilSegmentPolyBezier,
        MilSegmentPolyQuadraticBezier,
    
        MIL_SEGMENT_TYPE_FORCE_DWORD = unchecked((int)0xffffffff)
    };

    [System.Flags]
    internal enum MILCoreSegFlags
    {
    
        SegTypeLine                  = 0x00000001,
        SegTypeBezier                = 0x00000002,
        SegTypeMask                  = 0x00000003,
    
        // When this bit is set then this segment is not to be stroked
        SegIsAGap                    = 0x00000004,
    
        // When this bit is set then the join between this segment and the PREVIOUS segment
        // will be rounded upon widening, regardless of the pen line join property.
        SegSmoothJoin                = 0x00000008,
    
        // When this bit is set on the first type then the figure should be closed.
        SegClosed                    = 0x00000010,
    
        // This bit indicates whether the segment is curved.
        SegIsCurved                  = 0x00000020,
    
        FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    internal enum MIL_PEN_CAP    {
        MilPenCapFlat = 0,
        MilPenCapSquare = 1,
        MilPenCapRound = 2,
        MilPenCapTriangle = 3,
    
        MIL_PEN_CAP_FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    internal enum MIL_PEN_JOIN    {
        MilPenJoinMiter = 0,
        MilPenJoinBevel = 1,
        MilPenJoinRound = 2,
    
        MIL_PEN_JOIN_FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    [System.Flags]
    internal enum MILRTInitializationFlags
    {
    
        // Default initialization flags (0) imply hardware with software fallback,
        // synchronized to reduce tearing for hw RTs, and no retention of contents
        // between scenes.
    
        MIL_RT_INITIALIZE_DEFAULT       = 0x00000000,
    
        // This flag disables the hardware accelerated RT. Use only software.
    
        MIL_RT_SOFTWARE_ONLY            = 0x00000001,
    
        // This flag disables the software RT. Use only hardware.
    
        MIL_RT_HARDWARE_ONLY            = 0x00000002,
    
        // Creates a dummy render target that consumes all calls
    
        MIL_RT_NULL                     = 0x00000003,
    
        // Mask for choice of render target
    
        MIL_RT_TYPE_MASK                = 0x00000003,
    
        // This flag indicates that presentation should not wait for any specific
        // time to promote the results to the display. This may result in display
        // tearing.
    
        MIL_RT_PRESENT_IMMEDIATELY      = 0x00000004,
    
        // This flag makes the RT reatin the contents from one frame to the next.
        // Retaining the contents has performance implications.  For scene changes
        // with little to update retaining contents may help, but if most of the
        // scene will be repainted anyway, retention may hurt some hw scenarios.
    
        MIL_RT_PRESENT_RETAIN_CONTENTS  = 0x00000008,
    
        // This flag indicates that we should create a full screen RT.
    
        MIL_RT_FULLSCREEN               = 0x00000010,
    
        // This flag indicates that the render target backbuffer will have
        // linear gamma.
    
        MIL_RT_LINEAR_GAMMA             = 0x00000020,
    
        // This flag indicates that the render target backbuffer will have
        // an alpha channel that is at least 8 bits wide.
    
        MIL_RT_NEED_DESTINATION_ALPHA   = 0x00000040,
    
        // This flag allows the render target backbuffer to contain
        // 10 bits per channel rather than 32. This flag only has
        // meaning when linear gamma is also present.
    
        MIL_RT_ALLOW_LOW_PRECISION      = 0x00000080,
    
        // This flag assumes that all resources (such as bitmaps and render
        // targets) are released on the same thread as the rendering device.  This
        // flag enables us to use a single threaded dx device instead of a
        // multi-threaded one.
    
        MIL_RT_SINGLE_THREADED_USAGE    = 0x00000100,
    
        // This flag directs the render target to extend its presentation area
        // to include the non-client area.  The origin of the render target space
        // will be equal to the origin of the window.
    
        MIL_RT_RENDER_NONCLIENT         = 0x00000200,
    
        // This flag enables tear free composition by using the SWAPEFFECT D3D_FLIP.
    
        MIL_RT_PRESENT_FLIP             = 0x00000400,
    
        // Setting this flag results in the DX device instructing the driver not to
        // autorotate if the monitor is in a rotated mode.  Only makes sense for
        // fullscreen RTs
    
        MIL_RT_FULLSCREEN_NO_AUTOROTATE = 0x00000800,
    
        // This flag directed the render target not to restrict its rendering and
        // presentation to the visible portion of window on the desktop.  This is
        // useful for when the window position may be faked or the system may try
        // to make use of window contents that are not recognized as visible.  For
        // example DWM thumbnails expect a fully rendered and presented window.
        //
        // Note: This does not guarantee that some clipping will not be used.
    
        MIL_RT_DISABLE_DISPLAY_CLIPPING = 0x00001000,

        //
        // This flag is the same as MIL_RT_DISABLE_DISPLAY_CLIPPING except that it disables 
        // display clipping on multi-monitor configurations in all OS'. This flag is automatically 
        // set on Windows 8 and newer systems. If WPF decides to unset 
        // MIL_RT_DISABLE_DISPLAY_CLIPPING, then MIL_RT_DISABLE_MULTIMON_DISPLAY_CLIPPING flag
        // will not be respected even if set by an applicaiton via its manifest
        //
        MIL_RT_DISABLE_MULTIMON_DISPLAY_CLIPPING = 0x00004000,

        //
        // This flag is passed down by PresentationCore to tell wpfgfx that 
        // the DisableMultimonDisplayClipping compatibity flag is set by the user. This 
        // allows us to distinguish between when DisableMultimonDisplayClipping == 0 means
        // that the user set it to false explicitly, versus when the user didn't set it 
        // and the DisableMultimonDisplayClipping bit happens to be implicitly set to 0
        //
        MIL_RT_IS_DISABLE_MULTIMON_DISPLAY_CLIPPING_VALID = 0x00008000,

        // 
        // UCE only flags
        //
    
        // This flag directs the composition rendertarget to enable the occlusion
        // culling optimization.
        MIL_UCE_RT_ENABLE_OCCLUSION     = 0x00010000,
    
    
        //
        // Test only / internal flags
        //
    
        // This flag forces the render target to use the d3d9 reference raster
        // when using d3d. (Should be combined with MIL_RT_INITIALIZE_DEFAULT or
        // MIL_RT_HARDWARE_ONLY)
        // This is designed for test apps only
    
        MIL_RT_USE_REF_RAST             = 0x01000000,
    
        // This flag forces the render target to use the rgb reference raster
        // when using d3d.( Should be combined with MIL_RT_INITIALIZE_DEFAULT or
        // MIL_RT_HARDWARE_ONLY )
        // This is designed for test apps only
    
        MIL_RT_USE_RGB_RAST             = 0x02000000,
    
    
        // MIL Core Rendering Internal flag (=Do NOT pass to RT Create methods)
        // This flag enables the buffer to be set up in a transposed mode
        // in order to manage our own rotation for 90 and 270 degree rotations.
    
        MIL_RT_FULLSCREEN_TRANSPOSE_XY  = unchecked((int)0x10000000),
    
    
        // We support 4 primary present modes:
        //
        // 1) Present using D3D
        // 2) Present using BitBlt to a DC
        // 3) Present using AlphaBlend to a DC
        // 4) Present using UpdateLayeredWindow
        //
        MIL_RT_PRESENT_USING_MASK       = unchecked((int)0xC0000000),
        MIL_RT_PRESENT_USING_HAL        = 0x00000000,
        MIL_RT_PRESENT_USING_BITBLT     = unchecked((int)0x40000000),
        MIL_RT_PRESENT_USING_ALPHABLEND = unchecked((int)0x80000000),
        MIL_RT_PRESENT_USING_ULW        = unchecked((int)0xC0000000),
    
        FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    
    internal enum MIL_PRESENTATION_RESULTS    {
        MIL_PRESENTATION_VSYNC,
        MIL_PRESENTATION_NOPRESENT,
        MIL_PRESENTATION_VSYNC_UNSUPPORTED,
        MIL_PRESENTATION_DWM,
        MIL_PRESENTATION_FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    [StructLayout(LayoutKind.Sequential, Pack=1)]
    internal struct MIL_PEN_DATA    {
        internal double Thickness;
        internal double MiterLimit;
        internal double DashOffset;
        internal MIL_PEN_CAP StartLineCap;
        internal MIL_PEN_CAP EndLineCap;
        internal MIL_PEN_CAP DashCap;
        internal MIL_PEN_JOIN LineJoin;
        internal UInt32 DashArraySize;
    };
    
    [System.Flags]
    internal enum MILTransparencyFlags
    {
    
        Opaque = 0x0,
        ConstantAlpha = 0x1,
        PerPixelAlpha = 0x2,
        ColorKey = 0x4,
    
        FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
    internal enum MILWindowLayerType
    {
    
        NotLayered = 0,
        SystemManagedLayer = 1,
        ApplicationManagedLayer = 2,
    
        FORCE_DWORD = unchecked((int)0xffffffff)
    };
    
//
// Enum which describes whether certain values should be considered as absolute 
// local coordinates or whether they should be considered multiples of a bounding 
// box's size.
//
internal enum MilBrushMappingMode
{
    //
    // Absolute means that the values in question will be interpreted directly in 
    // local space.
    //
    Absolute = 0,

    //
    // RelativeToBoundingBox means that the values will be interpreted as a multiples 
    // of a bounding box, where 1.0 is considered 100% of the bounding box measure.
    //
    RelativeToBoundingBox = 1,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

//
// The AlignmentX enum is used to describe how content is positioned horizontally 
// within a container.
//
internal enum MilHorizontalAlignment
{
    //
    // Align contents towards the left of a space.
    //
    Left = 0,

    //
    // Center contents horizontally.
    //
    Center = 1,

    //
    // Align contents towards the right of a space.
    //
    Right = 2,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

//
// The AlignmentY enum is used to describe how content is positioned vertically 
// within a container.
//
internal enum MilVerticalAlignment
{
    //
    // Align contents towards the top of a space.
    //
    Top = 0,

    //
    // Center contents vertically.
    //
    Center = 1,

    //
    // Align contents towards the bottom of a space.
    //
    Bottom = 2,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

[System.Flags]
internal enum MilGlyphRun : ushort
{
    //
    // Exposed flags: these values are used in third party rasterizers.
    //

    Sideways = 0x00000001,


    //
    // Internal flags:
    //

    HasOffsets = 0x00000010,

    FORCE_WORD = unchecked((int)0xffff)
}

internal enum MilMessageClass
{
    //
    // invalid message
    //
    Invalid = 0x00,


    //
    // messages
    //

    SyncFlushReply = 0x01,
    Tier = 0x04,
    CompositionDeviceStateChange = 0x05,
    PartitionIsZombie = 0x06,
    SyncModeStatus = 0x09,
    Presented = 0x0A,
    RenderStatus = 0x0E,
    BadPixelShader = 0x10,


    //
    // Not a real message. This value is one more than message with the greatest 
    // numerical value.
    //
    Last,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

[System.Flags]
internal enum MilRenderOptionFlags
{
    BitmapScalingMode = 0x00000001,
    EdgeMode = 0x00000002,
    CompositingMode = 0x00000004,
    ClearTypeHint = 0x00000008,
    TextRenderingMode = 0x00000010,
    TextHintingMode = 0x00000020,
    Last,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

[System.Flags]
internal enum MilPathGeometryFlags
{
    HasCurves = 0x00000001,
    BoundsValid = 0x00000002,
    HasGaps = 0x00000004,
    HasHollows = 0x00000008,
    IsRegionData = 0x00000010,
    Mask = 0x0000001F,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

[System.Flags]
internal enum MilPathFigureFlags
{
    HasGaps = 0x00000001,
    HasCurves = 0x00000002,
    IsClosed = 0x00000004,
    IsFillable = 0x00000008,
    IsRectangleData = 0x00000010,
    Mask = 0x0000001F,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

internal enum MilCompositingMode
{
    SourceOver = 0,
    SourceCopy = 1,
    SourceAdd = 2,
    SourceAlphaMultiply = 3,
    SourceInverseAlphaMultiply = 4,
    SourceUnder = 5,

    //
    // Do not use the non-premultiplied blend with premultiplied
    // sources.  Use non-premultiplied sources carefully.
    //
    SourceOverNonPremultiplied = 6,

    SourceInverseAlphaOverNonPremultiplied = 7,
    DestInvert = 8,
    Last,

    FORCE_DWORD = unchecked((int)0xffffffff)
}

/// <summary>
///     MilColorF
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilColorF
{
    internal float r;
    internal float g;
    internal float b;
    internal float a;

    public override int GetHashCode()
    {
        return a.GetHashCode() ^ r.GetHashCode() ^ g.GetHashCode() ^ b.GetHashCode();
    }
    public override bool Equals(object obj)
    {
        return base.Equals(obj);
    }
};

/// <summary>
///     MilPoint2F
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilPoint2F
{
    internal float X;
    internal float Y;
};

/// <summary>
///     MilColorI
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilColorI
{
    internal int r;
    internal int g;
    internal int b;
    internal int a;
};

/// <summary>
///     MilPoint3F
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilPoint3F
{
    internal float X;
    internal float Y;
    internal float Z;
};

/// <summary>
///     MilQuaternionF
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilQuaternionF
{
    internal float X;
    internal float Y;
    internal float Z;
    internal float W;
};

/// <summary>
///     MilMatrix4x4D
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilMatrix4x4D
{
    internal double M_11;
    internal double M_12;
    internal double M_13;
    internal double M_14;
    internal double M_21;
    internal double M_22;
    internal double M_23;
    internal double M_24;
    internal double M_31;
    internal double M_32;
    internal double M_33;
    internal double M_34;
    internal double M_41;
    internal double M_42;
    internal double M_43;
    internal double M_44;
};

/// <summary>
///     MilGraphicsAccelerationCaps
///     Description of a display or display set's graphics capabilities.
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilGraphicsAccelerationCaps
{
    internal int TierValue;
    internal int HasWDDMSupport;
    internal UInt32 PixelShaderVersion;
    internal UInt32 VertexShaderVersion;
    internal UInt32 MaxTextureWidth;
    internal UInt32 MaxTextureHeight;
    internal int WindowCompatibleMode;
    internal UInt32 BitsPerPixel;
    internal UInt32 HasSSE2Support;
    internal UInt32 MaxPixelShader30InstructionSlots;
};

/// <summary>
///     MilGraphicsAccelerationAssessment
///     Assessment of the video memory bandwidth and total video memory as set by 
///     WinSAT. Used by the DWM to determine glass and opaque glass capability of the 
///     display machine.
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilGraphicsAccelerationAssessment
{
    internal UInt32 VideoMemoryBandwidth;
    internal UInt32 VideoMemorySize;
};

/// <summary>
///     MilRenderOptions
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilRenderOptions
{
    internal MilRenderOptionFlags Flags;
    internal EdgeMode EdgeMode;
    internal MilCompositingMode CompositingMode;
    internal BitmapScalingMode BitmapScalingMode;
    internal ClearTypeHint ClearTypeHint;
    internal TextRenderingMode TextRenderingMode;
    internal TextHintingMode TextHintingMode;
};

/// <summary>
///     MilMatrix3x2D
/// </summary>
[StructLayout(LayoutKind.Sequential, Pack=1)]
internal struct MilMatrix3x2D
{
    internal double S_11;
    internal double S_12;
    internal double S_21;
    internal double S_22;
    internal double DX;
    internal double DY;
};
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

    internal partial class DUCE
    {
        //
        // The MILCE resource type enumeration.
        //

        internal enum ResourceType
        {
            /* 0x00 */ TYPE_NULL = 0,
            /* 0x01 */ TYPE_MEDIAPLAYER = 1,
            /* 0x02 */ TYPE_ROTATION3D = 2,
            /* 0x03 */ TYPE_AXISANGLEROTATION3D = 3,
            /* 0x04 */ TYPE_QUATERNIONROTATION3D = 4,
            /* 0x05 */ TYPE_CAMERA = 5,
            /* 0x06 */ TYPE_PROJECTIONCAMERA = 6,
            /* 0x07 */ TYPE_PERSPECTIVECAMERA = 7,
            /* 0x08 */ TYPE_ORTHOGRAPHICCAMERA = 8,
            /* 0x09 */ TYPE_MATRIXCAMERA = 9,
            /* 0x0a */ TYPE_MODEL3D = 10,
            /* 0x0b */ TYPE_MODEL3DGROUP = 11,
            /* 0x0c */ TYPE_LIGHT = 12,
            /* 0x0d */ TYPE_AMBIENTLIGHT = 13,
            /* 0x0e */ TYPE_DIRECTIONALLIGHT = 14,
            /* 0x0f */ TYPE_POINTLIGHTBASE = 15,
            /* 0x10 */ TYPE_POINTLIGHT = 16,
            /* 0x11 */ TYPE_SPOTLIGHT = 17,
            /* 0x12 */ TYPE_GEOMETRYMODEL3D = 18,
            /* 0x13 */ TYPE_GEOMETRY3D = 19,
            /* 0x14 */ TYPE_MESHGEOMETRY3D = 20,
            /* 0x15 */ TYPE_MATERIAL = 21,
            /* 0x16 */ TYPE_MATERIALGROUP = 22,
            /* 0x17 */ TYPE_DIFFUSEMATERIAL = 23,
            /* 0x18 */ TYPE_SPECULARMATERIAL = 24,
            /* 0x19 */ TYPE_EMISSIVEMATERIAL = 25,
            /* 0x1a */ TYPE_TRANSFORM3D = 26,
            /* 0x1b */ TYPE_TRANSFORM3DGROUP = 27,
            /* 0x1c */ TYPE_AFFINETRANSFORM3D = 28,
            /* 0x1d */ TYPE_TRANSLATETRANSFORM3D = 29,
            /* 0x1e */ TYPE_SCALETRANSFORM3D = 30,
            /* 0x1f */ TYPE_ROTATETRANSFORM3D = 31,
            /* 0x20 */ TYPE_MATRIXTRANSFORM3D = 32,
            /* 0x21 */ TYPE_PIXELSHADER = 33,
            /* 0x22 */ TYPE_IMPLICITINPUTBRUSH = 34,
            /* 0x23 */ TYPE_EFFECT = 35,
            /* 0x24 */ TYPE_BLUREFFECT = 36,
            /* 0x25 */ TYPE_DROPSHADOWEFFECT = 37,
            /* 0x26 */ TYPE_SHADEREFFECT = 38,
            /* 0x27 */ TYPE_VISUAL = 39,
            /* 0x28 */ TYPE_VIEWPORT3DVISUAL = 40,
            /* 0x29 */ TYPE_VISUAL3D = 41,
            /* 0x2a */ TYPE_GLYPHRUN = 42,
            /* 0x2b */ TYPE_RENDERDATA = 43,
            /* 0x2c */ TYPE_DRAWINGCONTEXT = 44,
            /* 0x2d */ TYPE_RENDERTARGET = 45,
            /* 0x2e */ TYPE_HWNDRENDERTARGET = 46,
            /* 0x2f */ TYPE_GENERICRENDERTARGET = 47,
            /* 0x30 */ TYPE_ETWEVENTRESOURCE = 48,
            /* 0x31 */ TYPE_DOUBLERESOURCE = 49,
            /* 0x32 */ TYPE_COLORRESOURCE = 50,
            /* 0x33 */ TYPE_POINTRESOURCE = 51,
            /* 0x34 */ TYPE_RECTRESOURCE = 52,
            /* 0x35 */ TYPE_SIZERESOURCE = 53,
            /* 0x36 */ TYPE_MATRIXRESOURCE = 54,
            /* 0x37 */ TYPE_POINT3DRESOURCE = 55,
            /* 0x38 */ TYPE_VECTOR3DRESOURCE = 56,
            /* 0x39 */ TYPE_QUATERNIONRESOURCE = 57,
            /* 0x3a */ TYPE_IMAGESOURCE = 58,
            /* 0x3b */ TYPE_DRAWINGIMAGE = 59,
            /* 0x3c */ TYPE_TRANSFORM = 60,
            /* 0x3d */ TYPE_TRANSFORMGROUP = 61,
            /* 0x3e */ TYPE_TRANSLATETRANSFORM = 62,
            /* 0x3f */ TYPE_SCALETRANSFORM = 63,
            /* 0x40 */ TYPE_SKEWTRANSFORM = 64,
            /* 0x41 */ TYPE_ROTATETRANSFORM = 65,
            /* 0x42 */ TYPE_MATRIXTRANSFORM = 66,
            /* 0x43 */ TYPE_GEOMETRY = 67,
            /* 0x44 */ TYPE_LINEGEOMETRY = 68,
            /* 0x45 */ TYPE_RECTANGLEGEOMETRY = 69,
            /* 0x46 */ TYPE_ELLIPSEGEOMETRY = 70,
            /* 0x47 */ TYPE_GEOMETRYGROUP = 71,
            /* 0x48 */ TYPE_COMBINEDGEOMETRY = 72,
            /* 0x49 */ TYPE_PATHGEOMETRY = 73,
            /* 0x4a */ TYPE_BRUSH = 74,
            /* 0x4b */ TYPE_SOLIDCOLORBRUSH = 75,
            /* 0x4c */ TYPE_GRADIENTBRUSH = 76,
            /* 0x4d */ TYPE_LINEARGRADIENTBRUSH = 77,
            /* 0x4e */ TYPE_RADIALGRADIENTBRUSH = 78,
            /* 0x4f */ TYPE_TILEBRUSH = 79,
            /* 0x50 */ TYPE_IMAGEBRUSH = 80,
            /* 0x51 */ TYPE_DRAWINGBRUSH = 81,
            /* 0x52 */ TYPE_VISUALBRUSH = 82,
            /* 0x53 */ TYPE_BITMAPCACHEBRUSH = 83,
            /* 0x54 */ TYPE_DASHSTYLE = 84,
            /* 0x55 */ TYPE_PEN = 85,
            /* 0x56 */ TYPE_DRAWING = 86,
            /* 0x57 */ TYPE_GEOMETRYDRAWING = 87,
            /* 0x58 */ TYPE_GLYPHRUNDRAWING = 88,
            /* 0x59 */ TYPE_IMAGEDRAWING = 89,
            /* 0x5a */ TYPE_VIDEODRAWING = 90,
            /* 0x5b */ TYPE_DRAWINGGROUP = 91,
            /* 0x5c */ TYPE_GUIDELINESET = 92,
            /* 0x5d */ TYPE_CACHEMODE = 93,
            /* 0x5e */ TYPE_BITMAPCACHE = 94,
            /* 0x5f */ TYPE_BITMAPSOURCE = 95,
            /* 0x60 */ TYPE_DOUBLEBUFFEREDBITMAP = 96,
            /* 0x61 */ TYPE_D3DIMAGE = 97,
        };
    }

    internal partial class DUCE
    {
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MIL_GRADIENTSTOP
        {
            [FieldOffset(0)] internal double Position;
            [FieldOffset(8)] internal MilColorF Color;
        }
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MilRectD
    {
        internal MilRectD(double left, double top, double right, double bottom)
        {
            _left = left;
            _top = top;
            _right = right;
            _bottom = bottom;
        }

        static internal MilRectD Empty
        {
            get
            {
                return new MilRectD(0,0,0,0);
            }
        }

        static internal MilRectD NaN
        {
            get
            {
                return new MilRectD(Double.NaN,Double.NaN,Double.NaN,Double.NaN);
            }
        }


        internal Rect AsRect
        {
            get
            {
                if (_right >= _left &&
                    _bottom >= _top)
                {
                    return new Rect(_left, _top, _right - _left, _bottom - _top);
                }
                else
                {
                    // In particular, we treat NaN rectangles as empty rects.
                    return Rect.Empty;
                }
            }
        }

        [FieldOffset(0)]  internal double _left;
        [FieldOffset(8)]  internal double _top;
        [FieldOffset(16)] internal double _right;
        [FieldOffset(24)] internal double _bottom;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MilRectF
    {
        [FieldOffset(0)]  internal float _left;
        [FieldOffset(4)]  internal float _top;
        [FieldOffset(8)]  internal float _right;
        [FieldOffset(12)] internal float _bottom;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct D3DMATRIX
    {
        internal D3DMATRIX(float m11, float m12, float m13, float m14,
                           float m21, float m22, float m23, float m24,
                           float m31, float m32, float m33, float m34,
                           float m41, float m42, float m43, float m44)
        {
            _11 = m11;
            _12 = m12;
            _13 = m13;
            _14 = m14;

            _21 = m21;
            _22 = m22;
            _23 = m23;
            _24 = m24;

            _31 = m31;
            _32 = m32;
            _33 = m33;
            _34 = m34;

            _41 = m41;
            _42 = m42;
            _43 = m43;
            _44 = m44;
        }

        [FieldOffset(0)] internal float _11;
        [FieldOffset(4)] internal float _12;
        [FieldOffset(8)] internal float _13;
        [FieldOffset(12)] internal float _14;
        [FieldOffset(16)] internal float _21;
        [FieldOffset(20)] internal float _22;
        [FieldOffset(24)] internal float _23;
        [FieldOffset(28)] internal float _24;
        [FieldOffset(32)] internal float _31;
        [FieldOffset(36)] internal float _32;
        [FieldOffset(40)] internal float _33;
        [FieldOffset(44)] internal float _34;
        [FieldOffset(48)] internal float _41;
        [FieldOffset(52)] internal float _42;
        [FieldOffset(56)] internal float _43;
        [FieldOffset(60)] internal float _44;
    }

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_PATHGEOMETRY
    {
        [FieldOffset(0)] internal UInt32 Size;
        [FieldOffset(4)] internal MilPathGeometryFlags Flags;
        [FieldOffset(8)] internal MilRectD Bounds;
        [FieldOffset(40)] internal UInt32 FigureCount;
        [FieldOffset(44)] internal UInt32 ForcePacking;    // See "ForcePacking" comment at beginning of this file.
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_PATHFIGURE
    {
        [FieldOffset(0)] internal UInt32 BackSize;
        [FieldOffset(4)] internal MilPathFigureFlags Flags;
        [FieldOffset(8)] internal UInt32 Count;
        [FieldOffset(12)] internal UInt32 Size;
        [FieldOffset(16)] internal Point StartPoint;
        [FieldOffset(32)] internal UInt32 OffsetToLastSegment;
        [FieldOffset(36)] internal UInt32 ForcePacking;         // See "ForcePacking" comment at beginning of this file.
    };


    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT_LINE
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
        [FieldOffset(12)] internal UInt32 ForcePacking;         // See "ForcePacking" comment at beginning of this file.
        [FieldOffset(16)] internal Point Point;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT_BEZIER
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
        [FieldOffset(12)] internal UInt32 ForcePacking;         // See "ForcePacking" comment at beginning of this file.
        [FieldOffset(16)] internal Point Point1;
        [FieldOffset(32)] internal Point Point2;
        [FieldOffset(48)] internal Point Point3;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT_QUADRATICBEZIER
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
        [FieldOffset(12)] internal UInt32 ForcePacking;          // See "ForcePacking" comment at beginning of this file.
        [FieldOffset(16)] internal Point Point1;
        [FieldOffset(32)] internal Point Point2;
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT_ARC
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
        [FieldOffset(12)] internal UInt32 LargeArc;
        [FieldOffset(16)] internal Point Point;
        [FieldOffset(32)] internal Size Size;
        [FieldOffset(48)] internal double XRotation;
        [FieldOffset(56)] internal UInt32 Sweep;
        [FieldOffset(60)] internal UInt32 ForcePacking;          // See "ForcePacking" comment at beginning of this file.
    };

    [StructLayout(LayoutKind.Explicit)]
    internal struct MIL_SEGMENT_POLY
    {
        [FieldOffset(0)] internal MIL_SEGMENT_TYPE Type;
        [FieldOffset(4)] internal MILCoreSegFlags Flags;
        [FieldOffset(8)] internal UInt32 BackSize;
        [FieldOffset(12)] internal UInt32 Count;
    };
} // namespace System.Windows.Media.Composition



