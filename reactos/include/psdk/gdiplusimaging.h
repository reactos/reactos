/*
 * Copyright (C) 2007 Google (Evan Stade)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _GDIPLUSIMAGING_H
#define _GDIPLUSIMAGING_H

enum ImageLockMode
{
    ImageLockModeRead           = 1,
    ImageLockModeWrite          = 2,
    ImageLockModeUserInputBuf   = 4
};

#ifdef __cplusplus
class EncoderParameter
{
public:
    GUID    Guid;
    ULONG   NumberOfValues;
    ULONG   Type;
    VOID*   Value;
};

class EncoderParameters
{
public:
    UINT Count;
    EncoderParameter Parameter[1];
};

class ImageCodecInfo
{
public:
    CLSID Clsid;
    GUID  FormatID;
    const WCHAR* CodecName;
    const WCHAR* DllName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
};

class BitmapData
{
public:
    UINT Width;
    UINT Height;
    INT Stride;
    PixelFormat PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved;
};

class ImageItemData
{
public:
    UINT  Size;
    UINT  Position;
    VOID* Desc;
    UINT  DescSize;
    VOID* Data;
    UINT  DataSize;
    UINT  Cookie;
};

#else /* end of c++ typedefs */

typedef enum ImageLockMode ImageLockMode;

typedef struct EncoderParameter
{
    GUID Guid;
    ULONG NumberOfValues;
    ULONG Type;
    VOID* Value;
} EncoderParameter;

typedef struct EncoderParameters
{
    UINT Count;
    EncoderParameter Parameter[1];
} EncoderParameters;

typedef struct ImageCodecInfo
{
    CLSID Clsid;
    GUID  FormatID;
    const WCHAR* CodecName;
    const WCHAR* DllName;
    const WCHAR* FormatDescription;
    const WCHAR* FilenameExtension;
    const WCHAR* MimeType;
    DWORD Flags;
    DWORD Version;
    DWORD SigCount;
    DWORD SigSize;
    const BYTE* SigPattern;
    const BYTE* SigMask;
} ImageCodecInfo;

typedef struct BitmapData
{
    UINT Width;
    UINT Height;
    INT Stride;
    PixelFormat PixelFormat;
    VOID* Scan0;
    UINT_PTR Reserved; /* undocumented: stores the lock mode */
} BitmapData;

typedef struct ImageItemData
{
    UINT  Size;
    UINT  Position;
    VOID* Desc;
    UINT  DescSize;
    VOID* Data;
    UINT  DataSize;
    UINT  Cookie;
} ImageItemData;

#endif /* end of c typedefs */

#endif /* _GDIPLUSIMAGING_H */
