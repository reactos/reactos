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

// This code is generated from mcg\generators\CommandStructure.cs

using System;
using System.Windows.Media.Composition;
using System.Runtime.InteropServices;
using System.Windows.Media.Effects;
using System.Security;

using BOOL = System.UInt32;

namespace System.Windows.Media.Composition
{
    internal partial class DUCE
    {
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PARTITION_REGISTERFORNOTIFICATIONS
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal BOOL Enable;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_CHANNEL_REQUESTTIER
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal BOOL ReturnCommonMinimum;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PARTITION_SETVBLANKSYNCMODE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal BOOL Enable;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PARTITION_NOTIFYPRESENT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal UInt64 FrameTime;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_D3DIMAGE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 pInteropDeviceBitmap;
        [FieldOffset(16)] internal UInt64 pSoftwareBitmap;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_D3DIMAGE_PRESENT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 hEvent;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_BITMAP_INVALIDATE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal BOOL UseDirtyRect;
        [FieldOffset(12)] internal MS.Win32.NativeMethods.RECT DirtyRect;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DOUBLERESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_COLORRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_POINTRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Point Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_RECTRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Rect Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SIZERESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Size Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MATRIXRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilMatrix3x2D Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_POINT3DRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilPoint3F Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VECTOR3DRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilPoint3F Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_QUATERNIONRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilQuaternionF Value;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_RENDERDATA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 cbData;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_ETWEVENTRESOURCE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 id;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETOFFSET
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double offsetX;
        [FieldOffset(16)] internal double offsetY;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETTRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hTransform;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETEFFECT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hEffect;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETCACHEMODE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hCacheMode;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETCLIP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hClip;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETALPHA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double alpha;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETRENDEROPTIONS
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilRenderOptions renderOptions;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETCONTENT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hContent;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETALPHAMASK
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hAlphaMask;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_REMOVEALLCHILDREN
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_REMOVECHILD
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_INSERTCHILDAT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
        [FieldOffset(12)] internal UInt32 index;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETGUIDELINECOLLECTION
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt16 countX;
        [FieldOffset(12)] internal UInt16 countY;
        [FieldOffset(15)] private byte BYTEPacking0;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL_SETSCROLLABLEAREACLIP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Rect Clip;
        [FieldOffset(40)] internal BOOL IsEnabled;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VIEWPORT3DVISUAL_SETCAMERA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hCamera;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VIEWPORT3DVISUAL_SETVIEWPORT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Rect Viewport;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VIEWPORT3DVISUAL_SET3DCHILD
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL3D_SETCONTENT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hContent;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL3D_SETTRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hTransform;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL3D_REMOVEALLCHILDREN
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL3D_REMOVECHILD
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUAL3D_INSERTCHILDAT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hChild;
        [FieldOffset(12)] internal UInt32 index;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_HWNDTARGET_CREATE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 hwnd;
        [FieldOffset(16)] internal UInt64 hSection;
        [FieldOffset(24)] internal UInt64 masterDevice;
        [FieldOffset(32)] internal UInt32 width;
        [FieldOffset(36)] internal UInt32 height;
        [FieldOffset(40)] internal MilColorF clearColor;
        [FieldOffset(56)] internal UInt32 flags;
        [FieldOffset(60)] internal DUCE.ResourceHandle hBitmap;
        [FieldOffset(64)] internal UInt32 stride;
        [FieldOffset(68)] internal UInt32 ePixelFormat;
        [FieldOffset(72)] internal int DpiAwarenessContext;
        [FieldOffset(76)] internal double DpiX;
        [FieldOffset(84)] internal double DpiY;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_HWNDTARGET_SUPPRESSLAYERED
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal BOOL Suppress;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TARGET_UPDATEWINDOWSETTINGS
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MS.Win32.NativeMethods.RECT windowRect;
        [FieldOffset(24)] internal MILWindowLayerType windowLayerType;
        [FieldOffset(28)] internal MILTransparencyFlags transparencyMode;
        [FieldOffset(32)] internal float constantAlpha;
        [FieldOffset(36)] internal BOOL isChild;
        [FieldOffset(40)] internal BOOL isRTL;
        [FieldOffset(44)] internal BOOL renderingEnabled;
        [FieldOffset(48)] internal MilColorF colorKey;
        [FieldOffset(64)] internal UInt32 disableCookie;
        [FieldOffset(68)] internal BOOL gdiBlt;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GENERICTARGET_CREATE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 hwnd;
        [FieldOffset(16)] internal UInt64 pRenderTarget;
        [FieldOffset(24)] internal UInt32 width;
        [FieldOffset(28)] internal UInt32 height;
        [FieldOffset(32)] internal UInt32 dummy;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TARGET_SETROOT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hRoot;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TARGET_SETCLEARCOLOR
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF clearColor;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TARGET_INVALIDATE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MS.Win32.NativeMethods.RECT rc;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TARGET_SETFLAGS
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 flags;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_HWNDTARGET_DPICHANGED
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double DpiX;
        [FieldOffset(16)] internal double DpiY;
        [FieldOffset(24)] internal BOOL AfterParent;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GLYPHRUN_CREATE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 pIDWriteFont;
        [FieldOffset(16)] internal UInt16 GlyphRunFlags;
        [FieldOffset(20)] internal MilPoint2F Origin;
        [FieldOffset(28)] internal float MuSize;
        [FieldOffset(32)] internal Rect ManagedBounds;
        [FieldOffset(64)] internal UInt16 GlyphCount;
        [FieldOffset(68)] internal UInt16 BidiLevel;
        [FieldOffset(72)] internal UInt16 DWriteTextMeasuringMethod;
        [FieldOffset(75)] private byte BYTEPacking0;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DOUBLEBUFFEREDBITMAP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 SwDoubleBufferedBitmap;
        [FieldOffset(16)] internal BOOL UseBackBuffer;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DOUBLEBUFFEREDBITMAP_COPYFORWARD
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt64 CopyCompletedEvent;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PARTITION_NOTIFYPOLICYCHANGEFORNONINTERACTIVEMODE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal BOOL ShouldRenderEvenWhenNoDisplayDevicesAreAvailable;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_AXISANGLEROTATION3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double angle;
        [FieldOffset(16)] internal MilPoint3F axis;
        [FieldOffset(28)] internal DUCE.ResourceHandle hAxisAnimations;
        [FieldOffset(32)] internal DUCE.ResourceHandle hAngleAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_QUATERNIONROTATION3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilQuaternionF quaternion;
        [FieldOffset(24)] internal DUCE.ResourceHandle hQuaternionAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PERSPECTIVECAMERA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double nearPlaneDistance;
        [FieldOffset(16)] internal double farPlaneDistance;
        [FieldOffset(24)] internal double fieldOfView;
        [FieldOffset(32)] internal MilPoint3F position;
        [FieldOffset(44)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(48)] internal MilPoint3F lookDirection;
        [FieldOffset(60)] internal DUCE.ResourceHandle hNearPlaneDistanceAnimations;
        [FieldOffset(64)] internal MilPoint3F upDirection;
        [FieldOffset(76)] internal DUCE.ResourceHandle hFarPlaneDistanceAnimations;
        [FieldOffset(80)] internal DUCE.ResourceHandle hPositionAnimations;
        [FieldOffset(84)] internal DUCE.ResourceHandle hLookDirectionAnimations;
        [FieldOffset(88)] internal DUCE.ResourceHandle hUpDirectionAnimations;
        [FieldOffset(92)] internal DUCE.ResourceHandle hFieldOfViewAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_ORTHOGRAPHICCAMERA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double nearPlaneDistance;
        [FieldOffset(16)] internal double farPlaneDistance;
        [FieldOffset(24)] internal double width;
        [FieldOffset(32)] internal MilPoint3F position;
        [FieldOffset(44)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(48)] internal MilPoint3F lookDirection;
        [FieldOffset(60)] internal DUCE.ResourceHandle hNearPlaneDistanceAnimations;
        [FieldOffset(64)] internal MilPoint3F upDirection;
        [FieldOffset(76)] internal DUCE.ResourceHandle hFarPlaneDistanceAnimations;
        [FieldOffset(80)] internal DUCE.ResourceHandle hPositionAnimations;
        [FieldOffset(84)] internal DUCE.ResourceHandle hLookDirectionAnimations;
        [FieldOffset(88)] internal DUCE.ResourceHandle hUpDirectionAnimations;
        [FieldOffset(92)] internal DUCE.ResourceHandle hWidthAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MATRIXCAMERA
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal D3DMATRIX viewMatrix;
        [FieldOffset(72)] internal D3DMATRIX projectionMatrix;
        [FieldOffset(136)] internal DUCE.ResourceHandle htransform;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MODEL3DGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(12)] internal UInt32 ChildrenSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_AMBIENTLIGHT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(28)] internal DUCE.ResourceHandle hColorAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DIRECTIONALLIGHT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal MilPoint3F direction;
        [FieldOffset(36)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(40)] internal DUCE.ResourceHandle hColorAnimations;
        [FieldOffset(44)] internal DUCE.ResourceHandle hDirectionAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_POINTLIGHT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal double range;
        [FieldOffset(32)] internal double constantAttenuation;
        [FieldOffset(40)] internal double linearAttenuation;
        [FieldOffset(48)] internal double quadraticAttenuation;
        [FieldOffset(56)] internal MilPoint3F position;
        [FieldOffset(68)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(72)] internal DUCE.ResourceHandle hColorAnimations;
        [FieldOffset(76)] internal DUCE.ResourceHandle hPositionAnimations;
        [FieldOffset(80)] internal DUCE.ResourceHandle hRangeAnimations;
        [FieldOffset(84)] internal DUCE.ResourceHandle hConstantAttenuationAnimations;
        [FieldOffset(88)] internal DUCE.ResourceHandle hLinearAttenuationAnimations;
        [FieldOffset(92)] internal DUCE.ResourceHandle hQuadraticAttenuationAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SPOTLIGHT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal double range;
        [FieldOffset(32)] internal double constantAttenuation;
        [FieldOffset(40)] internal double linearAttenuation;
        [FieldOffset(48)] internal double quadraticAttenuation;
        [FieldOffset(56)] internal double outerConeAngle;
        [FieldOffset(64)] internal double innerConeAngle;
        [FieldOffset(72)] internal MilPoint3F position;
        [FieldOffset(84)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(88)] internal MilPoint3F direction;
        [FieldOffset(100)] internal DUCE.ResourceHandle hColorAnimations;
        [FieldOffset(104)] internal DUCE.ResourceHandle hPositionAnimations;
        [FieldOffset(108)] internal DUCE.ResourceHandle hRangeAnimations;
        [FieldOffset(112)] internal DUCE.ResourceHandle hConstantAttenuationAnimations;
        [FieldOffset(116)] internal DUCE.ResourceHandle hLinearAttenuationAnimations;
        [FieldOffset(120)] internal DUCE.ResourceHandle hQuadraticAttenuationAnimations;
        [FieldOffset(124)] internal DUCE.ResourceHandle hDirectionAnimations;
        [FieldOffset(128)] internal DUCE.ResourceHandle hOuterConeAngleAnimations;
        [FieldOffset(132)] internal DUCE.ResourceHandle hInnerConeAngleAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GEOMETRYMODEL3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle htransform;
        [FieldOffset(12)] internal DUCE.ResourceHandle hgeometry;
        [FieldOffset(16)] internal DUCE.ResourceHandle hmaterial;
        [FieldOffset(20)] internal DUCE.ResourceHandle hbackMaterial;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MESHGEOMETRY3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 PositionsSize;
        [FieldOffset(12)] internal UInt32 NormalsSize;
        [FieldOffset(16)] internal UInt32 TextureCoordinatesSize;
        [FieldOffset(20)] internal UInt32 TriangleIndicesSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MATERIALGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 ChildrenSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DIFFUSEMATERIAL
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal MilColorF ambientColor;
        [FieldOffset(40)] internal DUCE.ResourceHandle hbrush;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SPECULARMATERIAL
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal double specularPower;
        [FieldOffset(32)] internal DUCE.ResourceHandle hbrush;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_EMISSIVEMATERIAL
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilColorF color;
        [FieldOffset(24)] internal DUCE.ResourceHandle hbrush;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TRANSFORM3DGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 ChildrenSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TRANSLATETRANSFORM3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double offsetX;
        [FieldOffset(16)] internal double offsetY;
        [FieldOffset(24)] internal double offsetZ;
        [FieldOffset(32)] internal DUCE.ResourceHandle hOffsetXAnimations;
        [FieldOffset(36)] internal DUCE.ResourceHandle hOffsetYAnimations;
        [FieldOffset(40)] internal DUCE.ResourceHandle hOffsetZAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SCALETRANSFORM3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double scaleX;
        [FieldOffset(16)] internal double scaleY;
        [FieldOffset(24)] internal double scaleZ;
        [FieldOffset(32)] internal double centerX;
        [FieldOffset(40)] internal double centerY;
        [FieldOffset(48)] internal double centerZ;
        [FieldOffset(56)] internal DUCE.ResourceHandle hScaleXAnimations;
        [FieldOffset(60)] internal DUCE.ResourceHandle hScaleYAnimations;
        [FieldOffset(64)] internal DUCE.ResourceHandle hScaleZAnimations;
        [FieldOffset(68)] internal DUCE.ResourceHandle hCenterXAnimations;
        [FieldOffset(72)] internal DUCE.ResourceHandle hCenterYAnimations;
        [FieldOffset(76)] internal DUCE.ResourceHandle hCenterZAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_ROTATETRANSFORM3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double centerX;
        [FieldOffset(16)] internal double centerY;
        [FieldOffset(24)] internal double centerZ;
        [FieldOffset(32)] internal DUCE.ResourceHandle hCenterXAnimations;
        [FieldOffset(36)] internal DUCE.ResourceHandle hCenterYAnimations;
        [FieldOffset(40)] internal DUCE.ResourceHandle hCenterZAnimations;
        [FieldOffset(44)] internal DUCE.ResourceHandle hrotation;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MATRIXTRANSFORM3D
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal D3DMATRIX matrix;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PIXELSHADER
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal ShaderRenderMode ShaderRenderMode;
        [FieldOffset(12)] internal UInt32 PixelShaderBytecodeSize;
        [FieldOffset(16)] internal BOOL CompileSoftwareShader;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_IMPLICITINPUTBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(20)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(24)] internal DUCE.ResourceHandle hRelativeTransform;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_BLUREFFECT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Radius;
        [FieldOffset(16)] internal DUCE.ResourceHandle hRadiusAnimations;
        [FieldOffset(20)] internal KernelType KernelType;
        [FieldOffset(24)] internal RenderingBias RenderingBias;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DROPSHADOWEFFECT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double ShadowDepth;
        [FieldOffset(16)] internal MilColorF Color;
        [FieldOffset(32)] internal double Direction;
        [FieldOffset(40)] internal double Opacity;
        [FieldOffset(48)] internal double BlurRadius;
        [FieldOffset(56)] internal DUCE.ResourceHandle hShadowDepthAnimations;
        [FieldOffset(60)] internal DUCE.ResourceHandle hColorAnimations;
        [FieldOffset(64)] internal DUCE.ResourceHandle hDirectionAnimations;
        [FieldOffset(68)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(72)] internal DUCE.ResourceHandle hBlurRadiusAnimations;
        [FieldOffset(76)] internal RenderingBias RenderingBias;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SHADEREFFECT
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double TopPadding;
        [FieldOffset(16)] internal double BottomPadding;
        [FieldOffset(24)] internal double LeftPadding;
        [FieldOffset(32)] internal double RightPadding;
        [FieldOffset(40)] internal DUCE.ResourceHandle hPixelShader;
        [FieldOffset(44)] internal int DdxUvDdyUvRegisterIndex;
        [FieldOffset(48)] internal UInt32 ShaderConstantFloatRegistersSize;
        [FieldOffset(52)] internal UInt32 DependencyPropertyFloatValuesSize;
        [FieldOffset(56)] internal UInt32 ShaderConstantIntRegistersSize;
        [FieldOffset(60)] internal UInt32 DependencyPropertyIntValuesSize;
        [FieldOffset(64)] internal UInt32 ShaderConstantBoolRegistersSize;
        [FieldOffset(68)] internal UInt32 DependencyPropertyBoolValuesSize;
        [FieldOffset(72)] internal UInt32 ShaderSamplerRegistrationInfoSize;
        [FieldOffset(76)] internal UInt32 DependencyPropertySamplerValuesSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DRAWINGIMAGE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hDrawing;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TRANSFORMGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 ChildrenSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_TRANSLATETRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double X;
        [FieldOffset(16)] internal double Y;
        [FieldOffset(24)] internal DUCE.ResourceHandle hXAnimations;
        [FieldOffset(28)] internal DUCE.ResourceHandle hYAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SCALETRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double ScaleX;
        [FieldOffset(16)] internal double ScaleY;
        [FieldOffset(24)] internal double CenterX;
        [FieldOffset(32)] internal double CenterY;
        [FieldOffset(40)] internal DUCE.ResourceHandle hScaleXAnimations;
        [FieldOffset(44)] internal DUCE.ResourceHandle hScaleYAnimations;
        [FieldOffset(48)] internal DUCE.ResourceHandle hCenterXAnimations;
        [FieldOffset(52)] internal DUCE.ResourceHandle hCenterYAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SKEWTRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double AngleX;
        [FieldOffset(16)] internal double AngleY;
        [FieldOffset(24)] internal double CenterX;
        [FieldOffset(32)] internal double CenterY;
        [FieldOffset(40)] internal DUCE.ResourceHandle hAngleXAnimations;
        [FieldOffset(44)] internal DUCE.ResourceHandle hAngleYAnimations;
        [FieldOffset(48)] internal DUCE.ResourceHandle hCenterXAnimations;
        [FieldOffset(52)] internal DUCE.ResourceHandle hCenterYAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_ROTATETRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Angle;
        [FieldOffset(16)] internal double CenterX;
        [FieldOffset(24)] internal double CenterY;
        [FieldOffset(32)] internal DUCE.ResourceHandle hAngleAnimations;
        [FieldOffset(36)] internal DUCE.ResourceHandle hCenterXAnimations;
        [FieldOffset(40)] internal DUCE.ResourceHandle hCenterYAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_MATRIXTRANSFORM
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal MilMatrix3x2D Matrix;
        [FieldOffset(56)] internal DUCE.ResourceHandle hMatrixAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_LINEGEOMETRY
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Point StartPoint;
        [FieldOffset(24)] internal Point EndPoint;
        [FieldOffset(40)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(44)] internal DUCE.ResourceHandle hStartPointAnimations;
        [FieldOffset(48)] internal DUCE.ResourceHandle hEndPointAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_RECTANGLEGEOMETRY
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double RadiusX;
        [FieldOffset(16)] internal double RadiusY;
        [FieldOffset(24)] internal Rect Rect;
        [FieldOffset(56)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(60)] internal DUCE.ResourceHandle hRadiusXAnimations;
        [FieldOffset(64)] internal DUCE.ResourceHandle hRadiusYAnimations;
        [FieldOffset(68)] internal DUCE.ResourceHandle hRectAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_ELLIPSEGEOMETRY
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double RadiusX;
        [FieldOffset(16)] internal double RadiusY;
        [FieldOffset(24)] internal Point Center;
        [FieldOffset(40)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(44)] internal DUCE.ResourceHandle hRadiusXAnimations;
        [FieldOffset(48)] internal DUCE.ResourceHandle hRadiusYAnimations;
        [FieldOffset(52)] internal DUCE.ResourceHandle hCenterAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GEOMETRYGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(12)] internal FillRule FillRule;
        [FieldOffset(16)] internal UInt32 ChildrenSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_COMBINEDGEOMETRY
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(12)] internal GeometryCombineMode GeometryCombineMode;
        [FieldOffset(16)] internal DUCE.ResourceHandle hGeometry1;
        [FieldOffset(20)] internal DUCE.ResourceHandle hGeometry2;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PATHGEOMETRY
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(12)] internal FillRule FillRule;
        [FieldOffset(16)] internal UInt32 FiguresSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_SOLIDCOLORBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal MilColorF Color;
        [FieldOffset(32)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(36)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(40)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(44)] internal DUCE.ResourceHandle hColorAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_LINEARGRADIENTBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal Point StartPoint;
        [FieldOffset(32)] internal Point EndPoint;
        [FieldOffset(48)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(52)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(56)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(60)] internal ColorInterpolationMode ColorInterpolationMode;
        [FieldOffset(64)] internal BrushMappingMode MappingMode;
        [FieldOffset(68)] internal GradientSpreadMethod SpreadMethod;
        [FieldOffset(72)] internal UInt32 GradientStopsSize;
        [FieldOffset(76)] internal DUCE.ResourceHandle hStartPointAnimations;
        [FieldOffset(80)] internal DUCE.ResourceHandle hEndPointAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_RADIALGRADIENTBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal Point Center;
        [FieldOffset(32)] internal double RadiusX;
        [FieldOffset(40)] internal double RadiusY;
        [FieldOffset(48)] internal Point GradientOrigin;
        [FieldOffset(64)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(68)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(72)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(76)] internal ColorInterpolationMode ColorInterpolationMode;
        [FieldOffset(80)] internal BrushMappingMode MappingMode;
        [FieldOffset(84)] internal GradientSpreadMethod SpreadMethod;
        [FieldOffset(88)] internal UInt32 GradientStopsSize;
        [FieldOffset(92)] internal DUCE.ResourceHandle hCenterAnimations;
        [FieldOffset(96)] internal DUCE.ResourceHandle hRadiusXAnimations;
        [FieldOffset(100)] internal DUCE.ResourceHandle hRadiusYAnimations;
        [FieldOffset(104)] internal DUCE.ResourceHandle hGradientOriginAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_IMAGEBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal Rect Viewport;
        [FieldOffset(48)] internal Rect Viewbox;
        [FieldOffset(80)] internal double CacheInvalidationThresholdMinimum;
        [FieldOffset(88)] internal double CacheInvalidationThresholdMaximum;
        [FieldOffset(96)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(100)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(104)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(108)] internal BrushMappingMode ViewportUnits;
        [FieldOffset(112)] internal BrushMappingMode ViewboxUnits;
        [FieldOffset(116)] internal DUCE.ResourceHandle hViewportAnimations;
        [FieldOffset(120)] internal DUCE.ResourceHandle hViewboxAnimations;
        [FieldOffset(124)] internal Stretch Stretch;
        [FieldOffset(128)] internal TileMode TileMode;
        [FieldOffset(132)] internal AlignmentX AlignmentX;
        [FieldOffset(136)] internal AlignmentY AlignmentY;
        [FieldOffset(140)] internal CachingHint CachingHint;
        [FieldOffset(144)] internal DUCE.ResourceHandle hImageSource;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DRAWINGBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal Rect Viewport;
        [FieldOffset(48)] internal Rect Viewbox;
        [FieldOffset(80)] internal double CacheInvalidationThresholdMinimum;
        [FieldOffset(88)] internal double CacheInvalidationThresholdMaximum;
        [FieldOffset(96)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(100)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(104)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(108)] internal BrushMappingMode ViewportUnits;
        [FieldOffset(112)] internal BrushMappingMode ViewboxUnits;
        [FieldOffset(116)] internal DUCE.ResourceHandle hViewportAnimations;
        [FieldOffset(120)] internal DUCE.ResourceHandle hViewboxAnimations;
        [FieldOffset(124)] internal Stretch Stretch;
        [FieldOffset(128)] internal TileMode TileMode;
        [FieldOffset(132)] internal AlignmentX AlignmentX;
        [FieldOffset(136)] internal AlignmentY AlignmentY;
        [FieldOffset(140)] internal CachingHint CachingHint;
        [FieldOffset(144)] internal DUCE.ResourceHandle hDrawing;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VISUALBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal Rect Viewport;
        [FieldOffset(48)] internal Rect Viewbox;
        [FieldOffset(80)] internal double CacheInvalidationThresholdMinimum;
        [FieldOffset(88)] internal double CacheInvalidationThresholdMaximum;
        [FieldOffset(96)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(100)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(104)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(108)] internal BrushMappingMode ViewportUnits;
        [FieldOffset(112)] internal BrushMappingMode ViewboxUnits;
        [FieldOffset(116)] internal DUCE.ResourceHandle hViewportAnimations;
        [FieldOffset(120)] internal DUCE.ResourceHandle hViewboxAnimations;
        [FieldOffset(124)] internal Stretch Stretch;
        [FieldOffset(128)] internal TileMode TileMode;
        [FieldOffset(132)] internal AlignmentX AlignmentX;
        [FieldOffset(136)] internal AlignmentY AlignmentY;
        [FieldOffset(140)] internal CachingHint CachingHint;
        [FieldOffset(144)] internal DUCE.ResourceHandle hVisual;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_BITMAPCACHEBRUSH
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(20)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(24)] internal DUCE.ResourceHandle hRelativeTransform;
        [FieldOffset(28)] internal DUCE.ResourceHandle hBitmapCache;
        [FieldOffset(32)] internal DUCE.ResourceHandle hInternalTarget;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DASHSTYLE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Offset;
        [FieldOffset(16)] internal DUCE.ResourceHandle hOffsetAnimations;
        [FieldOffset(20)] internal UInt32 DashesSize;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_PEN
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Thickness;
        [FieldOffset(16)] internal double MiterLimit;
        [FieldOffset(24)] internal DUCE.ResourceHandle hBrush;
        [FieldOffset(28)] internal DUCE.ResourceHandle hThicknessAnimations;
        [FieldOffset(32)] internal PenLineCap StartLineCap;
        [FieldOffset(36)] internal PenLineCap EndLineCap;
        [FieldOffset(40)] internal PenLineCap DashCap;
        [FieldOffset(44)] internal PenLineJoin LineJoin;
        [FieldOffset(48)] internal DUCE.ResourceHandle hDashStyle;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GEOMETRYDRAWING
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hBrush;
        [FieldOffset(12)] internal DUCE.ResourceHandle hPen;
        [FieldOffset(16)] internal DUCE.ResourceHandle hGeometry;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GLYPHRUNDRAWING
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal DUCE.ResourceHandle hGlyphRun;
        [FieldOffset(12)] internal DUCE.ResourceHandle hForegroundBrush;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_IMAGEDRAWING
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Rect Rect;
        [FieldOffset(40)] internal DUCE.ResourceHandle hImageSource;
        [FieldOffset(44)] internal DUCE.ResourceHandle hRectAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_VIDEODRAWING
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal Rect Rect;
        [FieldOffset(40)] internal DUCE.ResourceHandle hPlayer;
        [FieldOffset(44)] internal DUCE.ResourceHandle hRectAnimations;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_DRAWINGGROUP
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double Opacity;
        [FieldOffset(16)] internal UInt32 ChildrenSize;
        [FieldOffset(20)] internal DUCE.ResourceHandle hClipGeometry;
        [FieldOffset(24)] internal DUCE.ResourceHandle hOpacityAnimations;
        [FieldOffset(28)] internal DUCE.ResourceHandle hOpacityMask;
        [FieldOffset(32)] internal DUCE.ResourceHandle hTransform;
        [FieldOffset(36)] internal DUCE.ResourceHandle hGuidelineSet;
        [FieldOffset(40)] internal EdgeMode EdgeMode;
        [FieldOffset(44)] internal BitmapScalingMode bitmapScalingMode;
        [FieldOffset(48)] internal ClearTypeHint ClearTypeHint;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_GUIDELINESET
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal UInt32 GuidelinesXSize;
        [FieldOffset(12)] internal UInt32 GuidelinesYSize;
        [FieldOffset(16)] internal BOOL IsDynamic;
        };
        [StructLayout(LayoutKind.Explicit, Pack=1)]
        internal struct MILCMD_BITMAPCACHE
        {
        [FieldOffset(0)] internal MILCMD Type;
        [FieldOffset(4)] internal DUCE.ResourceHandle Handle;
        [FieldOffset(8)] internal double RenderAtScale;
        [FieldOffset(16)] internal DUCE.ResourceHandle hRenderAtScaleAnimations;
        [FieldOffset(20)] internal BOOL SnapsToDevicePixels;
        [FieldOffset(24)] internal BOOL EnableClearType;
        };
    };
}


