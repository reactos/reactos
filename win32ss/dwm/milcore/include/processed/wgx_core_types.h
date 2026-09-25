// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.



#ifndef _MILCORETYPES_
#define _MILCORETYPES_

//+-----------------------------------------------------------------------------
//
//  Class:
//                  MilColorF
//
//  Note:
//                  Typedef of D3DCOLORVALUE
//
//------------------------------------------------------------------------------

#ifndef D3DCOLORVALUE_DEFINED

typedef struct _D3DCOLORVALUE {
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
} D3DCOLORVALUE;

  
#define D3DCOLORVALUE_DEFINED
#endif D3DCOLORVALUE_DEFINED

typedef D3DCOLORVALUE MilColorF;

typedef UINT32 MilColorB;


// MIL protocol fingerprint
#define MIL_SDK_VERSION 0x200184C0

// DWM protocol fingerprint
#define DWM_SDK_VERSION 0xBDDCB2B


#define DECLARE_MIL_HANDLE(name) struct name##__ { int unused; }; typedef struct name##__ *name

#define MAX_LENGTH_TASK_NAME 64

#if WINVER < 0x0600
typedef struct tagTITLEBARINFOEX
{
    DWORD cbSize;
    RECT rcTitleBar;
    DWORD rgstate[CCHILDREN_TITLEBAR + 1];
    RECT rgrect[CCHILDREN_TITLEBAR + 1];
} TITLEBARINFOEX, *PTITLEBARINFOEX, *LPTITLEBARINFOEX;
#endif


typedef UINT32 HMIL_OBJECT;

typedef HMIL_OBJECT HMIL_RESOURCE;
typedef HMIL_OBJECT HMIL_CHANNEL;

DECLARE_MIL_HANDLE(MIL_CHANNEL);
typedef void *HMIL_PLAYER;
typedef class CMilConnectionManager *HMIL_CONNECTIONMANAGER;
typedef interface IMilCommandTransport *HMIL_COMMANDTRANSPORT;
typedef class CMilCrossThreadTransport *HMIL_CROSSTHREADCOMMANDTRANSPORT;

DECLARE_MIL_HANDLE(HMIL_CONNECTION);

#define MILTYPES_DEFINED // Needed for MILEffects.idl


//
// Primitive Types
//

#ifdef MILCORE_KERNEL_COMPONENT

#ifndef BEGIN_MILENUM
#define BEGIN_MILENUM(type)                     \
    enum type##_Enum {                          \

#define END_MILENUM                             \
    };
#define MILENUM(type) enum type##_Enum
#endif /*BEGIN_MILENUM*/

#ifndef BEGIN_MILFLAGENUM
#define BEGIN_MILFLAGENUM(type)                 \
    enum type##_Flags {                         \

#define END_MILFLAGENUM                         \
    };
#define MILFLAGENUM(type) enum type##_Flags
#endif /*BEGIN_MILFLAGENUM*/

#endif /* MILCORE_KERNEL_COMPONENT */


//
// Some enums shouldn't be available in kernel mode, so we need to
// protect them by ifdef'ing them out.
//
#ifndef MILCORE_KERNEL_COMPONENT

//
// This determines how the colors in a gradient are interpolated.
//
BEGIN_MILENUM( MilColorInterpolationMode )
    //
    // Colors are interpolated in the scRGB color space
    //
    ScRgbLinearInterpolation = 0,

    //
    // Colors are interpolated in the sRGB color space
    //
    SRgbLinearInterpolation = 1,
END_MILENUM

//
// Enum which describes whether certain values should be considered as absolute 
// local coordinates or whether they should be considered multiples of a bounding 
// box's size.
//
BEGIN_MILENUM( MilBrushMappingMode )
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
END_MILENUM

//
// This determines how a gradient fills the space outside its primary area.
//
BEGIN_MILENUM( MilGradientSpreadMethod )
    //
    // Pad - The final color in the gradient is used to fill the remaining area.
    //
    Pad = 0,

    //
    // Reflect - The gradient is mirrored and repeated, then mirrored again, etc.
    //
    Reflect = 1,

    //
    // Repeat - The gradient is drawn again and again.
    //
    Repeat = 2,
END_MILENUM

//
// Enum which descibes how a source rect should be stretched to fit a destination 
// rect.
//
BEGIN_MILENUM( MilStretch )
    //
    // Preserve original size
    //
    None = 0,

    //
    // Aspect ratio is not preserved, source rect fills destination rect.
    //
    Fill = 1,

    //
    // Aspect ratio is preserved, Source rect is uniformly scaled as large as possible 
    // such that both width and height fit within destination rect.  This will not 
    // cause source clipping, but it may result in unfilled areas of the destination 
    // rect, if the aspect ratio of source and destination are different.
    //
    Uniform = 2,

    //
    // Aspect ratio is preserved, Source rect is uniformly scaled as small as possible 
    // such that the entire destination rect is filled.  This can cause source 
    // clipping, if the aspect ratio of source and destination are different.
    //
    UniformToFill = 3,
END_MILENUM

//
// Flags determining the transparency of a render target.
//
BEGIN_MILFLAGENUM( MilTransparency )
    //
    // Default is opaque.
    //
    Opaque = 0,

    //
    // Constant alpha.
    //
    ConstantAlpha = 1,

    //
    // Per pixel alpha.
    //
    PerPixelAlpha = 2,

    //
    // Color key.
    //
    ColorKey = 4,
END_MILFLAGENUM

//
// Enum determining the window layer type.
//
BEGIN_MILENUM( MilWindowLayerType )
    //
    // Not layered.
    //
    NotLayered = 0,

    //
    // System managed layer.
    //
    SystemManagedLayer = 1,

    //
    // Application managed layer
    //
    ApplicationManagedLayer = 2,
END_MILENUM

//
// Enum determining the caching mode for the hosted window target.
//
BEGIN_MILENUM( MilWindowTargetCachingMode )
    //
    // Not cached, visuals are directly connected.
    //
    NotCached = 0,

    //
    // Cached, single buffered.
    //
    Cached = 1,
END_MILENUM

//
// Enum which descibes the drawing of the ends of a line.
//
BEGIN_MILENUM( MilTileMode )
    //
    // Do not tile only the base tile is drawn, the remaining area is left as 
    // transparent
    //
    None = 0,

    //
    // The basic tile mode  the base tile is drawn and the remaining area is filled by 
    // repeating the base tile such that the right edge of one tile butts the left 
    // edge of the next, and similarly for bottom and top
    //
    Tile = 4,

    //
    // The same as tile, but alternate columns of tiles are flipped horizontally.  The 
    // base tile is drawn untransformed.
    //
    FlipX = 1,

    //
    // The same as tile, but alternate rows of tiles are flipped vertically.  The base 
    // tile is drawn untransformed.
    //
    FlipY = 2,

    //
    // The combination of FlipX and FlipY.  The base tile is drawn untransformed.
    //
    FlipXY = 3,

    //
    // Extend the edges of the tile out indefinitely
    //
    Extend = 5,
END_MILENUM

//
// The AlignmentX enum is used to describe how content is positioned horizontally 
// within a container.
//
BEGIN_MILENUM( MilHorizontalAlignment )
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
END_MILENUM

//
// The AlignmentY enum is used to describe how content is positioned vertically 
// within a container.
//
BEGIN_MILENUM( MilVerticalAlignment )
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
END_MILENUM

//
// Enum which descibes the drawing of the ends of a line.
//
BEGIN_MILENUM( MilPenCap )
    //
    // Flat line cap.
    //
    Flat = 0,

    //
    // Square line cap.
    //
    Square = 1,

    //
    // Round line cap.
    //
    Round = 2,

    //
    // Triangle line cap.
    //
    Triangle = 3,
END_MILENUM

//
// Enum which descibes the drawing of the corners on the line.
//
BEGIN_MILENUM( MilPenJoin )
    //
    // Miter join.
    //
    Miter = 0,

    //
    // Bevel join.
    //
    Bevel = 1,

    //
    // Round join.
    //
    Round = 2,
END_MILENUM

//
// This enumeration describes the type of combine operation to be performed.
//
BEGIN_MILENUM( MilCombineMode )
    //
    // Produce a geometry representing the set of points contained in either
    // the first or the second geometry.
    //
    Union = 0,

    //
    // Produce a geometry representing the set of points common to the first
    // and the second geometries.
    //
    Intersect = 1,

    //
    // Produce a geometry representing the set of points contained in the
    // first geometry or the second geometry, but not both.
    //
    Xor = 2,

    //
    // Produce a geometry representing the set of points contained in the
    // first geometry but not the second geometry.
    //
    Exclude = 3,
END_MILENUM

//
// Enum which descibes the manner in which we render edges of non-text primitives.
//
BEGIN_MILENUM( MilEdgeMode )
    //
    // No edge mode specfied - do not alter the current edge mode applied to this 
    // content.
    //
    Unspecified = 0,

    //
    // Render edges of non-text primitives as aliased edges.
    //
    Aliased = 1,

    Last,
END_MILENUM

//
// Enum which describes the manner in which we scale the images.
//
BEGIN_MILENUM( MilBitmapScalingMode )
    //
    // Rendering engine will chose the optimal algorithm
    //
    Unspecified = 0,

    //
    // Rendering engine will use the fastest mode to scale the images. This may mean a 
    // low quality image
    //
    LowQuality = 1,

    //
    // Rendering engine will use the mode which produces a most quality image
    //
    HighQuality = 2,

    //
    // Rendering engine will use linear interpolation.
    //
    Linear = 1,

    //
    // Rendering engine will use fant interpolation.
    //
    Fant = 2,

    //
    // Rendering engine will use nearest-neighbor interpolation.
    //
    NearestNeighbor = 3,

    Last,
END_MILENUM

//
// Enum used for hinting the rendering engine that text can be rendered with 
// ClearType.
//
BEGIN_MILENUM( MilClearTypeHint )
    //
    // Rendering engine will use ClearType when it is determined possible.  If an 
    // intermediate render target has been introduced in the ancestor tree, ClearType 
    // will be disabled.
    //
    Auto = 0,

    //
    // Rendering engine will enable ClearType for this element subtree.  Where an 
    // intermediate render target is introduced in this subtree, ClearType will once 
    // again be disabled.
    //
    Enabled = 1,

    Last,
END_MILENUM

//
// Enum used for hinting the rendering engine that rendered content can be cached
//
BEGIN_MILENUM( MilCachingHint )
    //
    // Rendering engine will choose algorithm.
    //
    Unspecified = 0,

    //
    // Cache rendered content when possible.
    //
    Cache = 1,

    Last,
END_MILENUM

//
// Enum used for specifying what filter mode text should be rendered with 
// (ClearType, grayscale, aliased).
//
BEGIN_MILENUM( MilTextRenderingMode )
    //
    // Rendering engine will use a rendering mode compatible with the 
    // TextFormattingMode specified for the control
    //
    Auto = 0,

    //
    // Rendering engine will render text with aliased filtering when possible
    //
    Aliased = 1,

    //
    // Rendering engine will render text with grayscale filtering when possible
    //
    Grayscale = 2,

    //
    // Rendering engine will render text with ClearType filtering when possible
    //
    ClearType = 3,

    Last,
END_MILENUM

//
// Enum used for specifying how text should be rendered with respect to animated 
// or static text
//
BEGIN_MILENUM( MilTextHintingMode )
    //
    // Rendering engine will automatically determine whether to draw text with quality 
    // settings appropriate to animated or static text
    //
    Auto = 0,

    //
    // Rendering engine will render text for highest static quality
    //
    Fixed = 1,

    //
    // Rendering engine will render text for highest animated quality
    //
    Animated = 2,

    Last,
END_MILENUM

//
// Type of blur kernel to use.
//
BEGIN_MILENUM( MilKernelType )
    //
    // Use a Guassian filter
    //
    Gaussian = 0,

    //
    // Use a Box filter
    //
    Box = 1,
END_MILENUM

//
// Type of edge profile to use.
//
BEGIN_MILENUM( MilEdgeProfile )
    //
    // Use a Linear edge profile
    //
    Linear = 0,

    //
    // Use a curved in edge profile
    //
    CurvedIn = 1,

    //
    // Use a curved out edge profile
    //
    CurvedOut = 2,

    //
    // Use a bulged up edge profile
    //
    BulgedUp = 3,
END_MILENUM

//
// Policy for rendering the shader in software.
//
BEGIN_MILENUM( ShaderEffectShaderRenderMode )
    //
    // Allow hardware and software
    //
    Auto = 0,

    //
    // Force software rendering
    //
    SoftwareOnly = 1,

    //
    // Require hardware rendering, ignore otherwise
    //
    HardwareOnly = 2,
END_MILENUM

//
// Type of bias to give rendering of the effect
//
BEGIN_MILENUM( MilEffectRenderingBias )
    //
    // Bias towards performance
    //
    Performance = 0,

    //
    // Bias towards quality
    //
    Quality = 1,
END_MILENUM

BEGIN_MILFLAGENUM( MilGlyphRun )
    //
    // Exposed flags: these values are used in third party rasterizers.
    //

    Sideways = 0x00000001,


    //
    // Internal flags:
    //

    HasOffsets = 0x00000010,
END_MILFLAGENUM

BEGIN_MILENUM( MilMessageClass )
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
END_MILENUM

//
// This enumeration determines the type of the segment.
//
BEGIN_MILENUM( MilSegmentType )
    //
    // The segment is invalid. This enumeration value SHOULD never be used.
    //
    None = 0,

    //
    // The segment is a line segment.
    //
    Line = 1,

    //
    // The segment is a cubic Bezier segment.
    //
    Bezier = 2,

    //
    // The segment is a quadratic Bezier segment.
    //
    QuadraticBezier = 3,

    //
    // The segment is an elliptical arc segment.
    //
    Arc = 4,

    //
    // This segment is a series of line segments.
    //
    PolyLine = 5,

    //
    // This segment is a series of cubic Bezier segments.
    //
    PolyBezier = 6,

    //
    // This segment is a series of quadratic Bezier segments.
    //
    PolyQuadraticBezier = 7,
END_MILENUM

//
// This enumeration defines flags of the segment.
//
BEGIN_MILFLAGENUM( MilCoreSeg )
    TypeLine = 0x00000001,
    TypeBezier = 0x00000002,
    TypeMask = 0x00000003,

    //
    // When this bit is set then this segment is not to be stroked
    //
    IsAGap = 0x00000004,

    //
    // When this bit is set then the join between this segment and the PREVIOUS
    // segment will be rounded upon widening, regardless of the pen line join
    // property.
    //
    SmoothJoin = 0x00000008,

    //
    // When this bit is set on the first type then the figure should be
    // closed.
    //
    Closed = 0x00000010,

    //
    // This bit indicates whether the segment is curved.
    //
    IsCurved = 0x00000020,
END_MILFLAGENUM

//
// This enumeration specifies the render target initialization flags. These
// flags can be combined using the bit-wise alternative operation to describe
// more complex properties.
//
BEGIN_MILFLAGENUM( MilRTInitialization )
    //
    // Default initialization flags (0) imply hardware with software fallback,
    // synchronized to reduce tearing for hw RTs, and no retention of contents
    // between scenes.
    //
    Default = 0x00000000,

    //
    // This flag disables the hardware accelerated RT. Use only software.
    //
    SoftwareOnly = 0x00000001,

    //
    // This flag disables the software RT. Use only hardware.
    //
    HardwareOnly = 0x00000002,

    //
    // Creates a dummy render target that consumes all calls
    //
    Null = 0x00000003,

    //
    // Mask for choice of render target
    //
    TypeMask = 0x00000003,

    //
    // This flag indicates that presentation should not wait for any specific
    // time to promote the results to the display. This may result in display
    // tearing.
    //
    PresentImmediately = 0x00000004,

    //
    // This flag makes the RT reatin the contents from one frame to the next.
    // Retaining the contents has performance implications.  For scene changes
    // with little to update retaining contents may help, but if most of the
    // scene will be repainted anyway, retention may hurt some hw scenarios.
    //
    PresentRetainContents = 0x00000008,

    //
    // This flag indicates that the render target backbuffer will have
    // an alpha channel that is at least 8 bits wide.
    //
    NeedDestinationAlpha = 0x00000040,

    //
    // This flag assumes that all resources (such as bitmaps and render
    // targets) are released on the same thread as the rendering device.  This
    // flag enables us to use a single threaded dx device instead of a
    // multi-threaded one.
    //
    SingleThreadedUsage = 0x00000100,

    //
    // This flag directs the render target to extend its presentation area
    // to include the non-client area.  The origin of the render target space
    // will be equal to the origin of the window.
    //
    RenderNonClient = 0x00000200,

    //
    // This flag directed the render target not to restrict its rendering and
    // presentation to the visible portion of window on the desktop.  This is
    // useful for when the window position may be faked or the system may try
    // to make use of window contents that are not recognized as visible.  For
    // example DWM thumbnails expect a fully rendered and presented window.
    // Note: This does not guarantee that some clipping will not be used. 
    //
    DisableDisplayClipping = 0x00001000,

    //
    // This flag forces the creation of a render target bitmap to match its
    // parent's type, so a software surface only creates software RTs and a
    // hardware surface only creates hardware RTs.  This is necessary for the 
    // hardware-accelerated bitmap effects pipeline to guarantee that we do
    // not encounter a situation where we're trying to run shaders sampling 
    // from a hardware texture to render into a software intermediate.
    //
    ForceCompatible = 0x00002000,

    //
    // This flag is the same as DisableDisplayClipping except that it disables 
    // display clipping on multi-monitor configurations in all OS'. This flag is 
    // automatically 
    // set on Windows 8 and newer systems. If WPF decides to unset 
    // DisableDisplayClipping, then DisableMultimonDisplayClipping flag will not be 
    // respected even if set by an applicaiton via its manifest
    //
    DisableMultimonDisplayClipping = 0x00004000,

    //
    // This flag is passed down by PresentationCore to tell wpfgfx that 
    // the DisableMultimonDisplayClipping compatibity flag is set by the user. This 
    // allows us to distinguish between when DisableMultimonDisplayClipping == 0 means
    // that the user set it to false explicitly, versus when the user didn't set it 
    // and the DisableMultimonDisplayClipping bit happens to be implicitly set to 0
    //
    IsDisableMultimonDisplayClippingValid = 0x00008000,


    //
    // Test only / internal flags
    //


    //
    // This flag forces the render target to use the d3d9 reference raster
    // when using d3d. (Should be combined with Default or HardwareOnly)
    // This is designed for test apps only
    //
    UseRefRast = 0x01000000,

    //
    // This flag forces the render target to use the rgb reference raster
    // when using d3d.( Should be combined with Default or HardwareOnly )
    // This is designed for test apps only
    //
    UseRgbRast = 0x02000000,


    //
    // We support 4 primary present modes:
    // 1) Present using D3D
    // 2) Present using BitBlt to a DC
    // 3) Present using AlphaBlend to a DC
    // 4) Present using UpdateLayeredWindow
    //

    PresentUsingMask = 0xC0000000,
    PresentUsingHal = 0x00000000,
    PresentUsingBitBlt = 0x40000000,
    PresentUsingAlphaBlend = 0x80000000,
    PresentUsingUpdateLayeredWindow = 0xC0000000,
END_MILFLAGENUM

BEGIN_MILENUM( MilPresentationResults )
    VSync,
    NoPresent,
    VSyncUnsupported,
    Dwm,
END_MILENUM

BEGIN_MILFLAGENUM( MilRenderOptionFlags )
    BitmapScalingMode = 0x00000001,
    EdgeMode = 0x00000002,
    CompositingMode = 0x00000004,
    ClearTypeHint = 0x00000008,
    TextRenderingMode = 0x00000010,
    TextHintingMode = 0x00000020,
    Last,
END_MILFLAGENUM

BEGIN_MILENUM( MilBitmapWrapMode )
    Extend = 0,
    FlipX = 1,
    FlipY = 2,
    FlipXY = 3,
    Tile = 4,
    Border = 5,
END_MILENUM

BEGIN_MILFLAGENUM( MilWindowProperties )
    //
    // WS_EX_LAYOUTRTL
    //
    RtlLayout = 0x0001,

    Redirected = 0x0002,

    //
    // WS_EX_COMPOSITED
    //
    Compositited = 0x0004,

    //
    // Present this window using GDI
    //
    PresentUsingGdi = 0x0008,
END_MILFLAGENUM

BEGIN_MILFLAGENUM( MilPathGeometryFlags )
    HasCurves = 0x00000001,
    BoundsValid = 0x00000002,
    HasGaps = 0x00000004,
    HasHollows = 0x00000008,
    IsRegionData = 0x00000010,
    Mask = 0x0000001F,
END_MILFLAGENUM

BEGIN_MILFLAGENUM( MilPathFigureFlags )
    HasGaps = 0x00000001,
    HasCurves = 0x00000002,
    IsClosed = 0x00000004,
    IsFillable = 0x00000008,
    IsRectangleData = 0x00000010,
    Mask = 0x0000001F,
END_MILFLAGENUM

BEGIN_MILENUM( MilDashStyle )
    Solid = 0,
    Dash = 1,
    Dot = 2,
    DashDot = 3,
    DashDotDot = 4,
    Custom = 5,
END_MILENUM

BEGIN_MILENUM( MilFillMode )
    Alternate = 0,
    Winding = 1,
END_MILENUM

BEGIN_MILENUM( MilGradientWrapMode )
    Extend = 0,
    Flip = 1,
    Tile = 2,
END_MILENUM

//
// This enumeration describes how a source rectangle should be stretched to fit
// a destination rectangle.
//
BEGIN_MILENUM( MilStretchMode )
    None = 0,
    Fill = 1,
    Uniform = 2,
    UniformToFill = 3,
END_MILENUM

BEGIN_MILENUM( MilCompositingMode )
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
END_MILENUM

BEGIN_MILENUM( MilCompositionDeviceState )
    Normal = 0,
    NoDevice = 1,
    Occluded = 2,
    Last,
END_MILENUM

//
// MIL marshal type (related to the transport type)
//
BEGIN_MILENUM( MilMarshalType )
    Invalid = 0x0,
    SameThread,
    CrossThread,
END_MILENUM

#endif // MILCORE_KERNEL_COMPONENT


#ifndef _MilMatrix3x2D_DEFINED

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMatrix3x2D
//
//------------------------------------------------------------------------------
struct MilMatrix3x2D
{
    DOUBLE S_11;
    DOUBLE S_12;
    DOUBLE S_21;
    DOUBLE S_22;
    DOUBLE DX;
    DOUBLE DY;
};

#define _MilMatrix3x2D_DEFINED

#endif // _MilMatrix3x2D_DEFINED

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPoint2F
//
//------------------------------------------------------------------------------
struct MilPoint2F
{
    FLOAT X;
    FLOAT Y;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilColorI
//
//------------------------------------------------------------------------------
struct MilColorI
{
    INT r;
    INT g;
    INT b;
    INT a;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPoint3F
//
//------------------------------------------------------------------------------
struct MilPoint3F
{
    FLOAT X;
    FLOAT Y;
    FLOAT Z;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilQuaternionF
//
//------------------------------------------------------------------------------
struct MilQuaternionF
{
    FLOAT X;
    FLOAT Y;
    FLOAT Z;
    FLOAT W;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMatrix4x4D
//
//------------------------------------------------------------------------------
struct MilMatrix4x4D
{
    DOUBLE M_11;
    DOUBLE M_12;
    DOUBLE M_13;
    DOUBLE M_14;
    DOUBLE M_21;
    DOUBLE M_22;
    DOUBLE M_23;
    DOUBLE M_24;
    DOUBLE M_31;
    DOUBLE M_32;
    DOUBLE M_33;
    DOUBLE M_34;
    DOUBLE M_41;
    DOUBLE M_42;
    DOUBLE M_43;
    DOUBLE M_44;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilGraphicsAccelerationCaps
//
//  Synopsis:
//      Description of a display or display set's graphics capabilities.
//
//------------------------------------------------------------------------------
struct MilGraphicsAccelerationCaps
{
    //
    // Tier value
    //
    INT TierValue;

    //
    // True if WDDM driver is supporting display
    //
    INT HasWDDMSupport;

    //
    // pixel shader version
    //
    UINT PixelShaderVersion;

    //
    // vertex shader version
    //
    UINT VertexShaderVersion;

    //
    // max texture width
    //
    UINT MaxTextureWidth;

    //
    // max texture height
    //
    UINT MaxTextureHeight;

    //
    // The accelerated rendering is supported for a windowed application
    //
    INT WindowCompatibleMode;

    //
    // Per pixel bit depth of display
    //
    UINT BitsPerPixel;

    //
    // Processor support for SSE2 instruction set.
    //
    UINT HasSSE2Support;

    //
    // Maximum number of instruction slots, if pixel shader 3.0 is supported
    //
    UINT MaxPixelShader30InstructionSlots;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilGraphicsAccelerationAssessment
//
//  Synopsis:
//      Assessment of the video memory bandwidth and total video memory as set by 
//      WinSAT. Used by the DWM to determine glass and opaque glass capability of the 
//      display machine.
//
//------------------------------------------------------------------------------
struct MilGraphicsAccelerationAssessment
{
    UINT VideoMemoryBandwidth;
    UINT VideoMemorySize;
};


//
// Some structs shouldn't be available in kernel mode, so we need to
// protect them by ifdef'ing them out.
//
#ifndef MILCORE_KERNEL_COMPONENT

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPoint2L
//
//------------------------------------------------------------------------------
struct MilPoint2L
{
    INT X;
    INT Y;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPoint2D
//
//------------------------------------------------------------------------------
struct MilPoint2D
{
    DOUBLE X;
    DOUBLE Y;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPointAndSizeL
//
//------------------------------------------------------------------------------
struct MilPointAndSizeL
{
    INT X;
    INT Y;
    INT Width;
    INT Height;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPointAndSizeF
//
//------------------------------------------------------------------------------
struct MilPointAndSizeF
{
    FLOAT X;
    FLOAT Y;
    FLOAT Width;
    FLOAT Height;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilRectF
//
//------------------------------------------------------------------------------
struct MilRectF
{
    FLOAT left;
    FLOAT top;
    FLOAT right;
    FLOAT bottom;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPointAndSizeD
//
//------------------------------------------------------------------------------
struct MilPointAndSizeD
{
    DOUBLE X;
    DOUBLE Y;
    DOUBLE Width;
    DOUBLE Height;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilRectD
//
//------------------------------------------------------------------------------
struct MilRectD
{
    DOUBLE left;
    DOUBLE top;
    DOUBLE right;
    DOUBLE bottom;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSizeD
//
//------------------------------------------------------------------------------
struct MilSizeD
{
    DOUBLE Width;
    DOUBLE Height;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilGradientStop
//
//------------------------------------------------------------------------------
struct MilGradientStop
{
    DOUBLE Position;
    MilColorF Color;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPathGeometry
//
//------------------------------------------------------------------------------
struct MilPathGeometry
{
    DWORD Size;
    DWORD Flags;
    MilRectD Bounds;
    UINT FigureCount;
    DWORD ForcePacking;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPathFigure
//
//------------------------------------------------------------------------------
struct MilPathFigure
{
    DWORD BackSize;
    DWORD Flags;
    UINT Count;
    UINT Size;
    MilPoint2D StartPoint;
    UINT OffsetToLastSegment;

    //
    // See ForcePacking comment at beginning of this file.
    //
    UINT ForcePacking;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegment
//
//------------------------------------------------------------------------------
struct MilSegment
{
    MilSegmentType::Enum Type;
    DWORD Flags;
    DWORD BackSize;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegmentLine
//
//------------------------------------------------------------------------------
struct MilSegmentLine : MilSegment
{
    //
    // See ForcePacking comment at beginning of this file.
    //
    UINT ForcePacking;

    MilPoint2D Point;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegmentBezier
//
//------------------------------------------------------------------------------
struct MilSegmentBezier : MilSegment
{
    //
    // See ForcePacking comment at beginning of this file.
    //
    UINT ForcePacking;

    MilPoint2D Point1;
    MilPoint2D Point2;
    MilPoint2D Point3;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegmentQuadraticBezier
//
//------------------------------------------------------------------------------
struct MilSegmentQuadraticBezier : MilSegment
{
    //
    // See ForcePacking comment at beginning of this file.
    //
    UINT ForcePacking;

    MilPoint2D Point1;
    MilPoint2D Point2;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegmentArc
//
//------------------------------------------------------------------------------
struct MilSegmentArc : MilSegment
{
    UINT LargeArc;
    MilPoint2D Point;
    MilSizeD Size;
    DOUBLE XRotation;
    UINT Sweep;

    //
    // See ForcePacking comment at beginning of this file.
    //
    UINT ForcePacking;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilSegmentPoly
//
//------------------------------------------------------------------------------
struct MilSegmentPoly : MilSegment
{
    UINT Count;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilPenData
//
//------------------------------------------------------------------------------
struct MilPenData
{
    DOUBLE Thickness;
    DOUBLE MiterLimit;
    DOUBLE DashOffset;
    MilPenCap::Enum StartLineCap;
    MilPenCap::Enum EndLineCap;
    MilPenCap::Enum DashCap;
    MilPenJoin::Enum LineJoin;
    UINT DashArraySize;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilRenderOptions
//
//------------------------------------------------------------------------------
struct MilRenderOptions
{
    MilRenderOptionFlags::Flags Flags;
    MilEdgeMode::Enum EdgeMode;
    MilCompositingMode::Enum CompositingMode;
    MilBitmapScalingMode::Enum BitmapScalingMode;
    MilClearTypeHint::Enum ClearTypeHint;
    MilTextRenderingMode::Enum TextRenderingMode;
    MilTextHintingMode::Enum TextHintingMode;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgCompositionDeviceStateChangeData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgCompositionDeviceStateChangeData
{
    MilCompositionDeviceState::Enum deviceStateOld;
    MilCompositionDeviceState::Enum deviceStateNew;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgSyncFlushReplyData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgSyncFlushReplyData
{
    HRESULT hr;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgVersionReplyData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgVersionReplyData
{
    UINT SupportedVersionsCount;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgTierData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgTierData
{
    //
    // Is this caps description specific to the primary display or is it the minimum 
    // common
    // value across all the displays?
    //
    UINT CommonMinimumCaps;

    //
    // Display uniqueness signature. These caps are only valid for given signature.
    //
    UINT DisplayUniqueness;

    MilGraphicsAccelerationCaps Caps;
    MilGraphicsAccelerationAssessment Assessment;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgPartitionIsZombieData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgPartitionIsZombieData
{
    HRESULT hrFailureCode;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgSyncModeStatusData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgSyncModeStatusData
{
    HRESULT hrEnabled;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgPresentedData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgPresentedData
{
    MilPresentationResults::Enum presentationResults;
    UINT refreshRate;
    LARGE_INTEGER presentationTime;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgSysMemUsageData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgSysMemUsageData
{
    UINT percentSystemMemoryUsed;
    size_t totalClientSystemMemory;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgAsyncFlushReplyData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgAsyncFlushReplyData
{
    UINT responseToken;
    HRESULT hrCode;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MilMsgRenderStatusData
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MilMsgRenderStatusData
{
    HRESULT hrCode;
};
#pragma pack(pop)

//+-----------------------------------------------------------------------------
//
//  Class:
//      MIL_MESSAGE
//
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct MIL_MESSAGE
{
    MilMessageClass::Enum type;
    DWORD dwReserved;

    union
    {
        MilMsgSyncFlushReplyData syncFlushReplyData;
        MilMsgTierData tierData;
        MilMsgPartitionIsZombieData partitionIsZombieData;
        MilMsgCompositionDeviceStateChangeData deviceStateChangeData;
        MilMsgSyncModeStatusData syncModeStatusData;
        MilMsgPresentedData presentationTimeData;
        MilMsgSysMemUsageData systemMemoryUsageData;
        MilMsgAsyncFlushReplyData asyncFlushData;
        MilMsgRenderStatusData renderStatusData;
    };
};
#pragma pack(pop)

#endif // MILCORE_KERNEL_COMPONENT


//
// Protocol Types
//

#pragma pack(push, 1)


//
// The MILCE resource type enumeration.
//

enum MIL_RESOURCE_TYPE
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
    /* 0x62 */ TYPE_LAST = 98,
    /* ---- */ TYPE_FORCE_DWORD = 0xFFFFFFFF
};

#ifndef MILCORE_KERNEL_COMPONENT


typedef enum
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
} MILCMD;



struct MILCMD_TRANSPORT_SYNCFLUSH
{
    MILCMD Type;
};

struct MILCMD_TRANSPORT_DESTROYRESOURCESONCHANNEL
{
    MILCMD Type;
    HMIL_CHANNEL hChannel;
};

struct MILCMD_PARTITION_REGISTERFORNOTIFICATIONS
{
    MILCMD Type;
    BOOL Enable;
};

struct MILCMD_CHANNEL_REQUESTTIER
{
    MILCMD Type;
    BOOL ReturnCommonMinimum;
};

struct MILCMD_PARTITION_SETVBLANKSYNCMODE
{
    MILCMD Type;
    BOOL Enable;
};

struct MILCMD_PARTITION_NOTIFYPRESENT
{
    MILCMD Type;
    UINT64 FrameTime;
};

struct MILCMD_CHANNEL_CREATERESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MIL_RESOURCE_TYPE resType;
};

struct MILCMD_CHANNEL_DELETERESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MIL_RESOURCE_TYPE resType;
};

struct MILCMD_CHANNEL_DUPLICATEHANDLE
{
    MILCMD Type;
    HMIL_RESOURCE Original;
    HMIL_CHANNEL TargetChannel;
    HMIL_RESOURCE Duplicate;
};

struct MILCMD_D3DIMAGE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 pInteropDeviceBitmap;
    UINT64 pSoftwareBitmap;
};

struct MILCMD_D3DIMAGE_PRESENT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 hEvent;
};

struct MILCMD_BITMAP_SOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    IWICBitmapSource* pIBitmap;
};

struct MILCMD_BITMAP_INVALIDATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    BOOL UseDirtyRect;
    RECT DirtyRect;
};

struct MILCMD_DOUBLERESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Value;
};

struct MILCMD_COLORRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF Value;
};

struct MILCMD_POINTRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPoint2D Value;
};

struct MILCMD_RECTRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPointAndSizeD Value;
};

struct MILCMD_SIZERESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilSizeD Value;
};

struct MILCMD_MATRIXRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilMatrix3x2D Value;
};

struct MILCMD_POINT3DRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPoint3F Value;
};

struct MILCMD_VECTOR3DRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPoint3F Value;
};

struct MILCMD_QUATERNIONRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilQuaternionF Value;
};

struct MILCMD_MEDIAPLAYER
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 pMedia;
    BOOL notifyUceDirect;
};

struct MILCMD_RENDERDATA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT cbData;
};

struct MILCMD_ETWEVENTRESOURCE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT id;
};

struct MILCMD_VISUAL_CREATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
};

struct MILCMD_VISUAL_SETOFFSET
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE offsetX;
    DOUBLE offsetY;
};

struct MILCMD_VISUAL_SETTRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hTransform;
};

struct MILCMD_VISUAL_SETEFFECT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hEffect;
};

struct MILCMD_VISUAL_SETCACHEMODE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hCacheMode;
};

struct MILCMD_VISUAL_SETCLIP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hClip;
};

struct MILCMD_VISUAL_SETALPHA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE alpha;
};

struct MILCMD_VISUAL_SETRENDEROPTIONS
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilRenderOptions renderOptions;
};

struct MILCMD_VISUAL_SETCONTENT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hContent;
};

struct MILCMD_VISUAL_SETALPHAMASK
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hAlphaMask;
};

struct MILCMD_VISUAL_REMOVEALLCHILDREN
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
};

struct MILCMD_VISUAL_REMOVECHILD
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hChild;
};

struct MILCMD_VISUAL_INSERTCHILDAT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hChild;
    UINT index;
};

struct MILCMD_VISUAL_SETGUIDELINECOLLECTION
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    WORD countX;
    UINT16 UINT16Padding0;
    WORD countY;
    UINT16 UINT16Padding1;
};

struct MILCMD_VISUAL_SETSCROLLABLEAREACLIP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPointAndSizeD Clip;
    BOOL IsEnabled;
};

struct MILCMD_VIEWPORT3DVISUAL_SETCAMERA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hCamera;
};

struct MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPointAndSizeD Viewport;
};

struct MILCMD_VIEWPORT3DVISUAL_SET3DCHILD
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hChild;
};

struct MILCMD_VISUAL3D_SETCONTENT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hContent;
};

struct MILCMD_VISUAL3D_SETTRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hTransform;
};

struct MILCMD_VISUAL3D_REMOVEALLCHILDREN
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
};

struct MILCMD_VISUAL3D_REMOVECHILD
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hChild;
};

struct MILCMD_VISUAL3D_INSERTCHILDAT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hChild;
    UINT index;
};

struct MILCMD_HWNDTARGET_CREATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 hwnd;
    UINT64 hSection;
    UINT64 masterDevice;
    UINT width;
    UINT height;
    MilColorF clearColor;
    UINT flags;
    HMIL_RESOURCE hBitmap;
    UINT stride;
    MilPixelFormat::Enum ePixelFormat;
    INT DpiAwarenessContext;
    DOUBLE DpiX;
    DOUBLE DpiY;
};

struct MILCMD_HWNDTARGET_SUPPRESSLAYERED
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    BOOL Suppress;
};

struct MILCMD_TARGET_UPDATEWINDOWSETTINGS
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    RECT windowRect;
    MilWindowLayerType::Enum windowLayerType;
    MilTransparency::Flags transparencyMode;
    FLOAT constantAlpha;
    BOOL isChild;
    BOOL isRTL;
    BOOL renderingEnabled;
    MilColorF colorKey;
    UINT disableCookie;
    BOOL gdiBlt;
};

struct MILCMD_GENERICTARGET_CREATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 hwnd;
    UINT64 pRenderTarget;
    UINT width;
    UINT height;
    UINT dummy;
};

struct MILCMD_TARGET_SETROOT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hRoot;
};

struct MILCMD_TARGET_SETCLEARCOLOR
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF clearColor;
};

struct MILCMD_TARGET_INVALIDATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    RECT rc;
};

struct MILCMD_TARGET_SETFLAGS
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT flags;
};

struct MILCMD_HWNDTARGET_DPICHANGED
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE DpiX;
    DOUBLE DpiY;
    BOOL AfterParent;
};

struct MILCMD_GLYPHRUN_CREATE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 pIDWriteFont;
    WORD GlyphRunFlags;
    UINT16 UINT16Padding0;
    MilPoint2F Origin;
    FLOAT MuSize;
    MilPointAndSizeD ManagedBounds;
    WORD GlyphCount;
    UINT16 UINT16Padding1;
    WORD BidiLevel;
    UINT16 UINT16Padding2;
    WORD DWriteTextMeasuringMethod;
    UINT16 UINT16Padding3;
};

struct MILCMD_DOUBLEBUFFEREDBITMAP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 SwDoubleBufferedBitmap;
    BOOL UseBackBuffer;
};

struct MILCMD_DOUBLEBUFFEREDBITMAP_COPYFORWARD
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT64 CopyCompletedEvent;
};

struct MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE
{
    MILCMD Type;
    BOOL ShouldRenderEvenWhenNoDisplayDevicesAreAvailable;
};

struct MILCMD_AXISANGLEROTATION3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE angle;
    MilPoint3F axis;
    HMIL_RESOURCE hAxisAnimations;
    HMIL_RESOURCE hAngleAnimations;
};

struct MILCMD_QUATERNIONROTATION3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilQuaternionF quaternion;
    HMIL_RESOURCE hQuaternionAnimations;
};

struct MILCMD_PERSPECTIVECAMERA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE nearPlaneDistance;
    DOUBLE farPlaneDistance;
    DOUBLE fieldOfView;
    MilPoint3F position;
    HMIL_RESOURCE htransform;
    MilPoint3F lookDirection;
    HMIL_RESOURCE hNearPlaneDistanceAnimations;
    MilPoint3F upDirection;
    HMIL_RESOURCE hFarPlaneDistanceAnimations;
    HMIL_RESOURCE hPositionAnimations;
    HMIL_RESOURCE hLookDirectionAnimations;
    HMIL_RESOURCE hUpDirectionAnimations;
    HMIL_RESOURCE hFieldOfViewAnimations;
};

struct MILCMD_ORTHOGRAPHICCAMERA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE nearPlaneDistance;
    DOUBLE farPlaneDistance;
    DOUBLE width;
    MilPoint3F position;
    HMIL_RESOURCE htransform;
    MilPoint3F lookDirection;
    HMIL_RESOURCE hNearPlaneDistanceAnimations;
    MilPoint3F upDirection;
    HMIL_RESOURCE hFarPlaneDistanceAnimations;
    HMIL_RESOURCE hPositionAnimations;
    HMIL_RESOURCE hLookDirectionAnimations;
    HMIL_RESOURCE hUpDirectionAnimations;
    HMIL_RESOURCE hWidthAnimations;
};

struct MILCMD_MATRIXCAMERA
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    D3DMATRIX viewMatrix;
    D3DMATRIX projectionMatrix;
    HMIL_RESOURCE htransform;
};

struct MILCMD_MODEL3DGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE htransform;
    UINT32 ChildrenSize;
};

struct MILCMD_AMBIENTLIGHT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    HMIL_RESOURCE htransform;
    HMIL_RESOURCE hColorAnimations;
};

struct MILCMD_DIRECTIONALLIGHT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    MilPoint3F direction;
    HMIL_RESOURCE htransform;
    HMIL_RESOURCE hColorAnimations;
    HMIL_RESOURCE hDirectionAnimations;
};

struct MILCMD_POINTLIGHT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    DOUBLE range;
    DOUBLE constantAttenuation;
    DOUBLE linearAttenuation;
    DOUBLE quadraticAttenuation;
    MilPoint3F position;
    HMIL_RESOURCE htransform;
    HMIL_RESOURCE hColorAnimations;
    HMIL_RESOURCE hPositionAnimations;
    HMIL_RESOURCE hRangeAnimations;
    HMIL_RESOURCE hConstantAttenuationAnimations;
    HMIL_RESOURCE hLinearAttenuationAnimations;
    HMIL_RESOURCE hQuadraticAttenuationAnimations;
};

struct MILCMD_SPOTLIGHT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    DOUBLE range;
    DOUBLE constantAttenuation;
    DOUBLE linearAttenuation;
    DOUBLE quadraticAttenuation;
    DOUBLE outerConeAngle;
    DOUBLE innerConeAngle;
    MilPoint3F position;
    HMIL_RESOURCE htransform;
    MilPoint3F direction;
    HMIL_RESOURCE hColorAnimations;
    HMIL_RESOURCE hPositionAnimations;
    HMIL_RESOURCE hRangeAnimations;
    HMIL_RESOURCE hConstantAttenuationAnimations;
    HMIL_RESOURCE hLinearAttenuationAnimations;
    HMIL_RESOURCE hQuadraticAttenuationAnimations;
    HMIL_RESOURCE hDirectionAnimations;
    HMIL_RESOURCE hOuterConeAngleAnimations;
    HMIL_RESOURCE hInnerConeAngleAnimations;
};

struct MILCMD_GEOMETRYMODEL3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE htransform;
    HMIL_RESOURCE hgeometry;
    HMIL_RESOURCE hmaterial;
    HMIL_RESOURCE hbackMaterial;
};

struct MILCMD_MESHGEOMETRY3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT32 PositionsSize;
    UINT32 NormalsSize;
    UINT32 TextureCoordinatesSize;
    UINT32 TriangleIndicesSize;
};

struct MILCMD_MATERIALGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT32 ChildrenSize;
};

struct MILCMD_DIFFUSEMATERIAL
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    MilColorF ambientColor;
    HMIL_RESOURCE hbrush;
};

struct MILCMD_SPECULARMATERIAL
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    DOUBLE specularPower;
    HMIL_RESOURCE hbrush;
};

struct MILCMD_EMISSIVEMATERIAL
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilColorF color;
    HMIL_RESOURCE hbrush;
};

struct MILCMD_TRANSFORM3DGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT32 ChildrenSize;
};

struct MILCMD_TRANSLATETRANSFORM3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE offsetX;
    DOUBLE offsetY;
    DOUBLE offsetZ;
    HMIL_RESOURCE hOffsetXAnimations;
    HMIL_RESOURCE hOffsetYAnimations;
    HMIL_RESOURCE hOffsetZAnimations;
};

struct MILCMD_SCALETRANSFORM3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE scaleX;
    DOUBLE scaleY;
    DOUBLE scaleZ;
    DOUBLE centerX;
    DOUBLE centerY;
    DOUBLE centerZ;
    HMIL_RESOURCE hScaleXAnimations;
    HMIL_RESOURCE hScaleYAnimations;
    HMIL_RESOURCE hScaleZAnimations;
    HMIL_RESOURCE hCenterXAnimations;
    HMIL_RESOURCE hCenterYAnimations;
    HMIL_RESOURCE hCenterZAnimations;
};

struct MILCMD_ROTATETRANSFORM3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE centerX;
    DOUBLE centerY;
    DOUBLE centerZ;
    HMIL_RESOURCE hCenterXAnimations;
    HMIL_RESOURCE hCenterYAnimations;
    HMIL_RESOURCE hCenterZAnimations;
    HMIL_RESOURCE hrotation;
};

struct MILCMD_MATRIXTRANSFORM3D
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    D3DMATRIX matrix;
};

struct MILCMD_PIXELSHADER
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    ShaderEffectShaderRenderMode::Enum ShaderRenderMode;
    UINT32 PixelShaderBytecodeSize;
    BOOL CompileSoftwareShader;
};

struct MILCMD_IMPLICITINPUTBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
};

struct MILCMD_BLUREFFECT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Radius;
    HMIL_RESOURCE hRadiusAnimations;
    MilKernelType::Enum KernelType;
    MilEffectRenderingBias::Enum RenderingBias;
};

struct MILCMD_DROPSHADOWEFFECT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE ShadowDepth;
    MilColorF Color;
    DOUBLE Direction;
    DOUBLE Opacity;
    DOUBLE BlurRadius;
    HMIL_RESOURCE hShadowDepthAnimations;
    HMIL_RESOURCE hColorAnimations;
    HMIL_RESOURCE hDirectionAnimations;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hBlurRadiusAnimations;
    MilEffectRenderingBias::Enum RenderingBias;
};

struct MILCMD_SHADEREFFECT
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE TopPadding;
    DOUBLE BottomPadding;
    DOUBLE LeftPadding;
    DOUBLE RightPadding;
    HMIL_RESOURCE hPixelShader;
    INT DdxUvDdyUvRegisterIndex;
    UINT32 ShaderConstantFloatRegistersSize;
    UINT32 DependencyPropertyFloatValuesSize;
    UINT32 ShaderConstantIntRegistersSize;
    UINT32 DependencyPropertyIntValuesSize;
    UINT32 ShaderConstantBoolRegistersSize;
    UINT32 DependencyPropertyBoolValuesSize;
    UINT32 ShaderSamplerRegistrationInfoSize;
    UINT32 DependencyPropertySamplerValuesSize;
};

struct MILCMD_DRAWINGIMAGE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hDrawing;
};

struct MILCMD_TRANSFORMGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT32 ChildrenSize;
};

struct MILCMD_TRANSLATETRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE X;
    DOUBLE Y;
    HMIL_RESOURCE hXAnimations;
    HMIL_RESOURCE hYAnimations;
};

struct MILCMD_SCALETRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE ScaleX;
    DOUBLE ScaleY;
    DOUBLE CenterX;
    DOUBLE CenterY;
    HMIL_RESOURCE hScaleXAnimations;
    HMIL_RESOURCE hScaleYAnimations;
    HMIL_RESOURCE hCenterXAnimations;
    HMIL_RESOURCE hCenterYAnimations;
};

struct MILCMD_SKEWTRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE AngleX;
    DOUBLE AngleY;
    DOUBLE CenterX;
    DOUBLE CenterY;
    HMIL_RESOURCE hAngleXAnimations;
    HMIL_RESOURCE hAngleYAnimations;
    HMIL_RESOURCE hCenterXAnimations;
    HMIL_RESOURCE hCenterYAnimations;
};

struct MILCMD_ROTATETRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Angle;
    DOUBLE CenterX;
    DOUBLE CenterY;
    HMIL_RESOURCE hAngleAnimations;
    HMIL_RESOURCE hCenterXAnimations;
    HMIL_RESOURCE hCenterYAnimations;
};

struct MILCMD_MATRIXTRANSFORM
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilMatrix3x2D Matrix;
    HMIL_RESOURCE hMatrixAnimations;
};

struct MILCMD_LINEGEOMETRY
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPoint2D StartPoint;
    MilPoint2D EndPoint;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hStartPointAnimations;
    HMIL_RESOURCE hEndPointAnimations;
};

struct MILCMD_RECTANGLEGEOMETRY
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE RadiusX;
    DOUBLE RadiusY;
    MilPointAndSizeD Rect;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRadiusXAnimations;
    HMIL_RESOURCE hRadiusYAnimations;
    HMIL_RESOURCE hRectAnimations;
};

struct MILCMD_ELLIPSEGEOMETRY
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE RadiusX;
    DOUBLE RadiusY;
    MilPoint2D Center;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRadiusXAnimations;
    HMIL_RESOURCE hRadiusYAnimations;
    HMIL_RESOURCE hCenterAnimations;
};

struct MILCMD_GEOMETRYGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hTransform;
    MilFillMode::Enum FillRule;
    UINT32 ChildrenSize;
};

struct MILCMD_COMBINEDGEOMETRY
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hTransform;
    MilCombineMode::Enum GeometryCombineMode;
    HMIL_RESOURCE hGeometry1;
    HMIL_RESOURCE hGeometry2;
};

struct MILCMD_PATHGEOMETRY
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hTransform;
    MilFillMode::Enum FillRule;
    UINT32 FiguresSize;
};

struct MILCMD_SOLIDCOLORBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilColorF Color;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    HMIL_RESOURCE hColorAnimations;
};

struct MILCMD_LINEARGRADIENTBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilPoint2D StartPoint;
    MilPoint2D EndPoint;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    MilColorInterpolationMode::Enum ColorInterpolationMode;
    MilBrushMappingMode::Enum MappingMode;
    MilGradientSpreadMethod::Enum SpreadMethod;
    UINT32 GradientStopsSize;
    HMIL_RESOURCE hStartPointAnimations;
    HMIL_RESOURCE hEndPointAnimations;
};

struct MILCMD_RADIALGRADIENTBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilPoint2D Center;
    DOUBLE RadiusX;
    DOUBLE RadiusY;
    MilPoint2D GradientOrigin;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    MilColorInterpolationMode::Enum ColorInterpolationMode;
    MilBrushMappingMode::Enum MappingMode;
    MilGradientSpreadMethod::Enum SpreadMethod;
    UINT32 GradientStopsSize;
    HMIL_RESOURCE hCenterAnimations;
    HMIL_RESOURCE hRadiusXAnimations;
    HMIL_RESOURCE hRadiusYAnimations;
    HMIL_RESOURCE hGradientOriginAnimations;
};

struct MILCMD_IMAGEBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilPointAndSizeD Viewport;
    MilPointAndSizeD Viewbox;
    DOUBLE CacheInvalidationThresholdMinimum;
    DOUBLE CacheInvalidationThresholdMaximum;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    MilBrushMappingMode::Enum ViewportUnits;
    MilBrushMappingMode::Enum ViewboxUnits;
    HMIL_RESOURCE hViewportAnimations;
    HMIL_RESOURCE hViewboxAnimations;
    MilStretch::Enum Stretch;
    MilTileMode::Enum TileMode;
    MilHorizontalAlignment::Enum AlignmentX;
    MilVerticalAlignment::Enum AlignmentY;
    MilCachingHint::Enum CachingHint;
    HMIL_RESOURCE hImageSource;
};

struct MILCMD_DRAWINGBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilPointAndSizeD Viewport;
    MilPointAndSizeD Viewbox;
    DOUBLE CacheInvalidationThresholdMinimum;
    DOUBLE CacheInvalidationThresholdMaximum;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    MilBrushMappingMode::Enum ViewportUnits;
    MilBrushMappingMode::Enum ViewboxUnits;
    HMIL_RESOURCE hViewportAnimations;
    HMIL_RESOURCE hViewboxAnimations;
    MilStretch::Enum Stretch;
    MilTileMode::Enum TileMode;
    MilHorizontalAlignment::Enum AlignmentX;
    MilVerticalAlignment::Enum AlignmentY;
    MilCachingHint::Enum CachingHint;
    HMIL_RESOURCE hDrawing;
};

struct MILCMD_VISUALBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    MilPointAndSizeD Viewport;
    MilPointAndSizeD Viewbox;
    DOUBLE CacheInvalidationThresholdMinimum;
    DOUBLE CacheInvalidationThresholdMaximum;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    MilBrushMappingMode::Enum ViewportUnits;
    MilBrushMappingMode::Enum ViewboxUnits;
    HMIL_RESOURCE hViewportAnimations;
    HMIL_RESOURCE hViewboxAnimations;
    MilStretch::Enum Stretch;
    MilTileMode::Enum TileMode;
    MilHorizontalAlignment::Enum AlignmentX;
    MilVerticalAlignment::Enum AlignmentY;
    MilCachingHint::Enum CachingHint;
    HMIL_RESOURCE hVisual;
};

struct MILCMD_BITMAPCACHEBRUSH
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hRelativeTransform;
    HMIL_RESOURCE hBitmapCache;
    HMIL_RESOURCE hInternalTarget;
};

struct MILCMD_DASHSTYLE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Offset;
    HMIL_RESOURCE hOffsetAnimations;
    UINT32 DashesSize;
};

struct MILCMD_PEN
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Thickness;
    DOUBLE MiterLimit;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hThicknessAnimations;
    MilPenCap::Enum StartLineCap;
    MilPenCap::Enum EndLineCap;
    MilPenCap::Enum DashCap;
    MilPenJoin::Enum LineJoin;
    HMIL_RESOURCE hDashStyle;
};

struct MILCMD_GEOMETRYDRAWING
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hGeometry;
};

struct MILCMD_GLYPHRUNDRAWING
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    HMIL_RESOURCE hGlyphRun;
    HMIL_RESOURCE hForegroundBrush;
};

struct MILCMD_IMAGEDRAWING
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPointAndSizeD Rect;
    HMIL_RESOURCE hImageSource;
    HMIL_RESOURCE hRectAnimations;
};

struct MILCMD_VIDEODRAWING
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    MilPointAndSizeD Rect;
    HMIL_RESOURCE hPlayer;
    HMIL_RESOURCE hRectAnimations;
};

struct MILCMD_DRAWINGGROUP
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE Opacity;
    UINT32 ChildrenSize;
    HMIL_RESOURCE hClipGeometry;
    HMIL_RESOURCE hOpacityAnimations;
    HMIL_RESOURCE hOpacityMask;
    HMIL_RESOURCE hTransform;
    HMIL_RESOURCE hGuidelineSet;
    MilEdgeMode::Enum EdgeMode;
    MilBitmapScalingMode::Enum bitmapScalingMode;
    MilClearTypeHint::Enum ClearTypeHint;
};

struct MILCMD_GUIDELINESET
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    UINT32 GuidelinesXSize;
    UINT32 GuidelinesYSize;
    BOOL IsDynamic;
};

struct MILCMD_BITMAPCACHE
{
    MILCMD Type;
    HMIL_RESOURCE Handle;
    DOUBLE RenderAtScale;
    HMIL_RESOURCE hRenderAtScaleAnimations;
    BOOL SnapsToDevicePixels;
    BOOL EnableClearType;
};


struct MILCMD_DRAW_LINE
{
    MILCMD type;
    MilPoint2D point0;
    MilPoint2D point1;
    HMIL_RESOURCE hPen;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_LINE_ANIMATE
{
    MILCMD type;
    MilPoint2D point0;
    MilPoint2D point1;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hPoint0Animations;
    HMIL_RESOURCE hPoint1Animations;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_RECTANGLE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
};


struct MILCMD_DRAW_RECTANGLE_ANIMATE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hRectangleAnimations;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_ROUNDED_RECTANGLE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    DOUBLE radiusX;
    DOUBLE radiusY;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
};


struct MILCMD_DRAW_ROUNDED_RECTANGLE_ANIMATE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    DOUBLE radiusX;
    DOUBLE radiusY;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hRectangleAnimations;
    HMIL_RESOURCE hRadiusXAnimations;
    HMIL_RESOURCE hRadiusYAnimations;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_ELLIPSE
{
    MILCMD type;
    MilPoint2D center;
    DOUBLE radiusX;
    DOUBLE radiusY;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
};


struct MILCMD_DRAW_ELLIPSE_ANIMATE
{
    MILCMD type;
    MilPoint2D center;
    DOUBLE radiusX;
    DOUBLE radiusY;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hCenterAnimations;
    HMIL_RESOURCE hRadiusXAnimations;
    HMIL_RESOURCE hRadiusYAnimations;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_GEOMETRY
{
    MILCMD type;
    HMIL_RESOURCE hBrush;
    HMIL_RESOURCE hPen;
    HMIL_RESOURCE hGeometry;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_IMAGE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hImageSource;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_IMAGE_ANIMATE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hImageSource;
    HMIL_RESOURCE hRectangleAnimations;
};


struct MILCMD_DRAW_GLYPH_RUN
{
    MILCMD type;
    HMIL_RESOURCE hForegroundBrush;
    HMIL_RESOURCE hGlyphRun;
};


struct MILCMD_DRAW_DRAWING
{
    MILCMD type;
    HMIL_RESOURCE hDrawing;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_VIDEO
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hPlayer;
    UINT32 QuadWordPad0;
};


struct MILCMD_DRAW_VIDEO_ANIMATE
{
    MILCMD type;
    MilPointAndSizeD rectangle;
    HMIL_RESOURCE hPlayer;
    HMIL_RESOURCE hRectangleAnimations;
};


struct MILCMD_PUSH_CLIP
{
    MILCMD type;
    HMIL_RESOURCE hClipGeometry;
    UINT32 QuadWordPad0;
};


struct MILCMD_PUSH_OPACITY_MASK
{
    MILCMD type;
    MilRectF boundingBoxCacheLocalSpace;
    HMIL_RESOURCE hOpacityMask;
    UINT32 QuadWordPad0;
};


struct MILCMD_PUSH_OPACITY
{
    MILCMD type;
    DOUBLE opacity;
};


struct MILCMD_PUSH_OPACITY_ANIMATE
{
    MILCMD type;
    DOUBLE opacity;
    HMIL_RESOURCE hOpacityAnimations;
    UINT32 QuadWordPad0;
};


struct MILCMD_PUSH_TRANSFORM
{
    MILCMD type;
    HMIL_RESOURCE hTransform;
    UINT32 QuadWordPad0;
};


struct MILCMD_PUSH_GUIDELINE_SET
{
    MILCMD type;
    HMIL_RESOURCE hGuidelines;
    UINT32 QuadWordPad0;
};


struct MILCMD_PUSH_GUIDELINE_Y1
{
    MILCMD type;
    DOUBLE coordinate;
};


struct MILCMD_PUSH_GUIDELINE_Y2
{
    MILCMD type;
    DOUBLE leadingCoordinate;
    DOUBLE offsetToDrivenCoordinate;
};


struct MILCMD_PUSH_EFFECT
{
    MILCMD type;

};


struct MILCMD_POP
{
    MILCMD type;

};


#endif /*MILCORE_KERNEL_COMPONENT*/

#pragma pack(pop)

const MilPointAndSizeD MilEmptyPointAndSizeD = {0.0, 0.0, -1.0, -1.0 };

#endif _MILCORETYPES_



