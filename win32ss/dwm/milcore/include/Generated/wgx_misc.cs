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


