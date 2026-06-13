// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------

//
//    *** WARNING *** WARNING *** WARNING ***
//
//    A lot of these types are generated for unmanaged
//    so be sure to keep them in sync. Better yet, if you
//    update one, move it to codegen completely
//
//    *** WARNING *** WARNING *** WARNING ***
//
//---------------------------------------------------------------------------

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
    



