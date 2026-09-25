// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Several tiny classes for debugging purposes:
//             read/write memory mapped files
//             read/write BMP files
//
//  $ENDTAG
//
//  Classes:
//      CFileReader
//      CFileWriter
//      CBitmapReader
//      CBitmapWriter
//
//------------------------------------------------------------------------------

//#define BITMAP_IO // uncomment to enable bitmap input/output
#ifdef BITMAP_IO

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFileReader
//
//  Synopsis:
//      Memory mapped file reader.
//
//      Usage:
//       - Create an instance of CFileReader.
//       - Call Init(path), should return true on success.
//       - Use GetSize() to detect file size.
//       - Use GetData() to access the file as contiguous array.
//       - Destruct the instance.
//
//      The buffer returned by GetData() is allowed to read and write. Changes
//      will not affect file data.
//
//------------------------------------------------------------------------------
class CFileReader
{
public:
    CFileReader();
    bool Init(char const* path);
    ~CFileReader() {Clean();}

    void* GetData() {return m_pData;}    // returns 0 on invalid file
    unsigned GetSize() {return m_uSize;}

private:
    void *m_hMap;
    char *m_pData;
    unsigned m_uSize;

private:
    void Clean();
    bool InitByHandle(void* hFile);
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CFileWriter
//
//  Synopsis:
//      Memory mapped file writer.
//
//      Usage:
//       - Create an instance of CFileWrite.
//       - Call Init(file path name, desired file size).
//       - Ensure that "true" has been returned.
//       - Use GetData() to access the file as contiguous array.
//       - Fill this array somehow.
//       - Destruct the instance.
//
//------------------------------------------------------------------------------
class CFileWriter
{
public:
    CFileWriter();
    bool Init(char const* path, unsigned size);
    ~CFileWriter() {Clean();}

    char* GetData() {return m_pData;}    // returns 0 on invalid file
    unsigned GetSize() {return m_uSize;}

private:
    void *m_hFile;
    void *m_hMap;
    char *m_pData;
    unsigned m_uSize;

private:
    void Clean();
};

enum BitmapFormat
{
    trueColor,
    blackWhite,
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBitmapReader
//
//  Synopsis:
//      Memory mapped bmp file reader.
//
//      Usage:
//       - Create an instance of CBitmapReader.
//       - Call Init(file path name).
//       - Ensure that "true" has been returned.
//       - Use GetWidth() and GetHeight() to get bitmap size in pixels.
//       - Use GetPixel() to fetch data, or, if you want better speed,
//         use GetFormat() to detect bitmap format (either 1 or 24 bpp,
//         others are not supported), and GetData/Pith/Row for fast access.
//       - Destruct the instance.
//
//------------------------------------------------------------------------------
class CBitmapReader
{
public:
    CBitmapReader();
    bool Init(char const* path);
    ~CBitmapReader() {Clean();}

    UINT GetWidth() const {return m_pih->biWidth;}
    UINT GetHeight() const {return m_pih->biHeight;}
    BitmapFormat GetFormat() const {return m_format;}

    BYTE* GetData() {return m_pData;}
    UINT GetPitch() {return m_pitch;}
    BYTE* GetRow(UINT y) {return m_pData + y*m_pitch;}
    COLORREF GetPixel(UINT x, UINT y);

private:
    CFileReader m_file;
    BITMAPFILEHEADER* m_pfh;
    BITMAPINFOHEADER* m_pih;
    BitmapFormat m_format;
    UINT m_pitch;
    BYTE* m_pData;
    RGBQUAD* m_pColorTable;
    UINT m_colorTableSize;
    COLORREF m_myColorTable[2];

private:
    void Clean() {m_file.~CFileReader();}
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBitmapWriter
//
//  Synopsis:
//      Memory mapped bmp file reader.
//
//      Usage:
//       - Create an instance of CBitmapWriter.
//       - Call Init() with desired bitmap attributes.
//       - Ensure that "true" has been returned.
//       - Use SetPixel() to fill the data, or, alternatively,
//       - use GetData/Pith/Row for fast access.
//       - Destruct the instance.
//
//------------------------------------------------------------------------------
class CBitmapWriter
{
public:
    CBitmapWriter();
    bool Init(char const* path, BitmapFormat format, UINT width, UINT height);
    ~CBitmapWriter() {Clean();}

    UINT GetWidth() const {return m_pih->biWidth;}
    UINT GetHeight() const {return m_pih->biHeight;}
    BitmapFormat GetFormat() const {return m_format;}

    BYTE* GetData() {return m_pData;}
    UINT GetPitch() {return m_pitch;}
    BYTE* GetRow(UINT y) {return m_pData + y*m_pitch;}
    COLORREF GetPixel(UINT x, UINT y);
    void SetPixel(UINT x, UINT y, COLORREF c);

private:
    CFileWriter m_file;
    BITMAPFILEHEADER* m_pfh;
    BITMAPINFOHEADER* m_pih;
    BitmapFormat m_format;
    UINT m_pitch;
    BYTE* m_pData;
    RGBQUAD* m_pColorTable;
    UINT m_colorTableSize;
    COLORREF m_myColorTable[2];

private:
    void Clean() {m_file.~CFileWriter();}
};

#endif //BITMAP_IO


