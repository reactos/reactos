/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS gdiplus.dll
 * FILE:        include/GdiPlusPrivate.h
 * PURPOSE:     GDI+ private definitions
 */
#ifndef __GDIPLUSPRIVATE_H
#define __GDIPLUSPRIVATE_H

#include <gdiplusenums.h>
#include <gdipluspixelformats.h>

typedef float REAL;
typedef ARGB Color;
#include <gdipluscolormatrix.h>

#define WINGDIPAPI __stdcall
#define GDIPCONST const

typedef BOOL (CALLBACK * ImageAbort)(VOID *);
typedef ImageAbort DrawImageAbort;
typedef ImageAbort GetThumbnailImageAbort;
typedef BOOL (CALLBACK * EnumerateMetafileProc)(EmfPlusRecordType,
  UINT,
  UINT,
  const BYTE*,
  VOID*);

typedef enum {
  Ok = 0,
  GenericError = 1,
  InvalidParameter = 2,
  OutOfMemory = 3,
  ObjectBusy = 4,
  InsufficientBuffer = 5,
  NotImplemented = 6,
  Win32Error = 7,
  WrongState = 8,
  Aborted = 9,
  FileNotFound = 10,
  ValueOverflow = 11,
  AccessDenied = 12,
  UnknownImageFormat = 13,
  FontFamilyNotFound = 14,
  FontStyleNotFound = 15,
  NotTrueTypeFont = 16,
  UnsupportedGdiplusVersion = 17,
  GdiplusNotInitialized = 18,
  PropertyNotFound = 19,
  PropertyNotSupported = 20,
  ProfileNotFound = 21
} GpStatus;

typedef GpStatus Status;

typedef struct
{
} GpAdjustableArrowCap;

typedef struct
{
} GpBitmap;

typedef struct
{
} GpGraphics;

typedef struct
{
} BitmapData;

typedef struct
{
} CGpEffect;

typedef struct
{
} GpCachedBitmap;

typedef struct
{
} IDirectDrawSurface7;

typedef struct
{
} GpBrush;

typedef BrushType GpBrushType;

typedef HatchStyle GpHatchStyle;

typedef struct
{
} GpHatch;

typedef struct
{
} GpPath;

typedef struct
{
} GpRegion;

typedef struct
{
  CLSID Clsid;
  GUID FormatID;
  WCHAR *CodecName;
  WCHAR *DllName;
  WCHAR *FormatDescription;
  WCHAR *FilenameExtension;
  WCHAR *MimeType;
  DWORD Flags;
  DWORD Version;
  DWORD SigCount;
  DWORD SigSize;
  BYTE *SigPattern;
  BYTE *SigMask;
} ImageCodecInfo;

typedef Unit GpUnit;

typedef struct
{
} GpPen;

typedef struct
{
  REAL X;
  REAL Y;
} GpPointF;

typedef struct
{
  INT X;
  INT Y;
} GpPoint;

typedef FillMode GpFillMode;

typedef struct
{
} GpSolidFill;

typedef struct
{
} GpFont;

typedef struct
{
} GpFontFamily;

typedef struct
{
} GpFontCollection;

typedef FlushIntention GpFlushIntention;

typedef struct
{
} GpImage;

typedef struct
{
} GpImageAttributes;

typedef struct
{
} GpMatrix;

typedef struct
{
  GUID Guid;
  ULONG NumberOfValues;
  ULONG Type;
  VOID *Value;
} EncoderParameter;

typedef struct
{
  UINT Count;
  EncoderParameter Parameter[1];
} EncoderParameters;

typedef struct
{
  PROPID id;
  ULONG length;
  WORD type;
  VOID *value;
} PropertyItem;

typedef struct
{
  UINT Size;
  UINT Position;
  VOID *Desc;
  UINT DescSize;
  UINT *Data;
  UINT DataSize;
  UINT Cookie;
} ImageItemData;

typedef enum {
  ColorChannelFlagsC = 0,
  ColorChannelFlagsM = 1,
  ColorChannelFlagsY = 2,
  ColorChannelFlagsK = 3,
  ColorChannelFlagsLast = 4
} ColorChannelFlags;

typedef LineCap GpLineCap;

typedef struct
{
} GpCustomLineCap;

typedef LineJoin GpLineJoin;

typedef WrapMode GpWrapMode;

typedef struct
{
} GpLineGradient;

typedef MatrixOrder GpMatrixOrder;

typedef struct
{
} GpMetafile;

typedef struct
{
  REAL X;
  REAL Y;
} PointF;

typedef struct
{
  INT X;
  INT Y;
} Point;

typedef struct
{
  REAL X;
  REAL Y;
  REAL Width;
  REAL Height;
} RectF;

typedef RectF GpRectF;

typedef struct
{
  INT X;
  INT Y;
  INT Width;
  INT Height;
} Rect;

typedef Rect GpRect;

typedef struct
{
} MetafileHeader;

typedef struct {
  INT16 Left;
  INT16 Top;
  INT16 Right;
  INT16 Bottom;
} PWMFRect16;

typedef struct {
  UINT32 Key;
  INT16 Hmf;
  PWMFRect16 BoundingBox;
  INT16 Inch;
  UINT32 Reserved;
  INT16 Checksum;
} WmfPlaceableFileHeader;

typedef struct
{
} GpPathData;

typedef struct
{
} GpStringFormat;

typedef struct
{
} GpPathGradient;

typedef struct
{
} GpPathIterator;

typedef DashCap GpDashCap;

typedef PenAlignment GpPenAlignment;

typedef PenType GpPenType;

typedef DashStyle GpDashStyle;

typedef struct
{
} CharacterRange;

typedef struct
{
} GpTexture;

typedef CoordinateSpace GpCoordinateSpace;

typedef enum {
  DebugEventLevelFatal,
  DebugEventLevelWarning
} DebugEventLevel;

typedef VOID (WINAPI *DebugEventProc)(DebugEventLevel level,
  CHAR *message);

typedef struct
{
  UINT32 GdiplusVersion;
  DebugEventProc DebugEventCallback;
  BOOL SuppressBackgroundThread;
  BOOL SuppressExternalCodecs;
} GdiplusStartupInput;

typedef Status (WINAPI *NotificationHookProc)(OUT ULONG_PTR *token);
typedef VOID (WINAPI *NotificationUnhookProc)(ULONG_PTR token);

typedef struct {
  NotificationHookProc NotificationHook;
  NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

#endif /* __GDIPLUSPRIVATE_H */
