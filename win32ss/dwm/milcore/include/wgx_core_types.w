/*
 Licensed to the .NET Foundation under one or more agreements.
 The .NET Foundation licenses this file to you under the MIT license.
 See the LICENSE file in the project root for more information.
*/


/*=========================================================================*\



    File: Milcoretypes.w

    Module Name: Milcore

    Description: Milcore types and command structures.

\*=========================================================================*/
;begin CSHARP_ONLY

using System;
using System.Runtime.InteropServices;

namespace System.Windows.Media.Composition
{

;include_header wgx_core_types_compat.cs
;include_header Generated\wgx_misc.cs
;include_header Generated\wgx_command_types.cs
;include_header Generated\wgx_resource_types.cs

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

;end CSHARP_ONLY
;begin CPP_ONLY

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

;include_header wgx_sdk_version.h

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

;include_header Generated\wgx_misc.h


//
// Protocol Types
//

#pragma pack(push, 1)

;include_header Generated\wgx_resource_types.h

#ifndef MILCORE_KERNEL_COMPONENT

;include_header Generated\wgx_command_types.h
;include_header Generated\wgx_commands.h
;include_header Generated\wgx_renderdata_commands.h

#endif /*MILCORE_KERNEL_COMPONENT*/

#pragma pack(pop)

const MilPointAndSizeD MilEmptyPointAndSizeD = {0.0, 0.0, -1.0, -1.0 };

#endif _MILCORETYPES_

;end CPP_ONLY


