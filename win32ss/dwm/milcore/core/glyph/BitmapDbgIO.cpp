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
//      See header file for details.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.h"

#ifdef BITMAP_IO

CFileReader::CFileReader()
{
    m_pData = 0;
    m_hMap = 0;
    m_uSize = 0;
}

bool CFileReader::Init(char const* path)
{
    Clean();
    void *hFile = CreateFileA(
        path,              // pointer to name of the file 
        GENERIC_READ,      // access (read-write) mode 
        FILE_SHARE_READ,   // share mode 
        0,                 // pointer to security attributes 
        OPEN_EXISTING,     // how to create 
        0,                 // file attributes 
        0                  // handle to file with attributes to copy 
        );

   
    if (hFile == (HANDLE)HFILE_ERROR) return false;

    bool ok = InitByHandle(hFile);

    CloseHandle(hFile);
    return ok;
}

__forceinline bool CFileReader::InitByHandle(void* hFile)
{
    m_uSize = GetFileSize(hFile, 0);

    m_hMap = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
    if (m_hMap == 0) return false;

    m_pData = (char*)MapViewOfFile(m_hMap, FILE_MAP_READ, 0, 0, 0);
    if (m_pData == 0) return false;

    return true;
}

void CFileReader::Clean()
{
    if (m_pData) UnmapViewOfFile(m_pData);
    if (m_hMap) CloseHandle(m_hMap);

    m_pData = 0;
    m_hMap = 0;
    m_uSize = 0;
}

CFileWriter::CFileWriter()
{
    m_hFile = 0;
    m_hMap = 0;
    m_pData = 0;
    m_uSize = 0;
}    

bool CFileWriter::Init(char const* path, unsigned size)
{
    Clean();

    m_hFile = CreateFileA(
        path,              // pointer to name of the file 
        GENERIC_READ | GENERIC_WRITE,     // access (read-write) mode 
        FILE_SHARE_WRITE,  // share mode 
        0,                 // pointer to security attributes 
        CREATE_ALWAYS,     // how to create 
        0,                 // file attributes 
        0                  // handle to file with attributes to copy 
        );
   
    if (m_hFile == (HANDLE)HFILE_ERROR) return false;

    m_hMap = CreateFileMapping(m_hFile, 0, PAGE_READWRITE, 0, size, 0);
    if (m_hMap == 0) return false;

    m_pData = (char*)MapViewOfFile(m_hMap, FILE_MAP_WRITE, 0, 0, size);
    if(m_pData == 0) return false;

    m_uSize = size;
    return true;
}

void CFileWriter::Clean()
{
    if (m_pData) UnmapViewOfFile(m_pData);
    if (m_hMap) CloseHandle(m_hMap);

    if (m_hFile != (HANDLE)HFILE_ERROR) CloseHandle(m_hFile);

    m_hFile = 0;
    m_hMap = 0;
    m_pData = 0;
    m_uSize = 0;
}

CBitmapReader::CBitmapReader()
{
}

bool CBitmapReader::Init(char const* path)
{
    if (!m_file.Init(path)) return false;
    m_pfh = (BITMAPFILEHEADER*)m_file.GetData();
    if (m_pfh == 0) return false;
    if (m_file.GetSize() < sizeof(BITMAPFILEHEADER)) return false;
    if (m_pfh->bfType != 'MB') return false;
    if (m_pfh->bfSize > m_file.GetSize()) return false;

    m_pData = (BYTE*)m_pfh + m_pfh->bfOffBits;
    m_pih = (BITMAPINFOHEADER*)(m_pfh+1);
    if (m_pData < (BYTE*)(m_pih+1)) return false;

    if (m_pih->biPlanes == 1 &&
        m_pih->biBitCount == 24 &&
        m_pih->biCompression == 0)
    {
        m_format = trueColor;
    }
    else if (m_pih->biPlanes == 1 &&
        m_pih->biBitCount == 1 &&
        m_pih->biCompression == 0)
    {
        m_format = blackWhite;
    }
    else return false;

    UINT bitPerRow = m_pih->biBitCount * m_pih->biWidth;
    UINT dwordsPerRow = (bitPerRow + 31)/32;
    m_pitch = dwordsPerRow*4;

    m_pColorTable = (RGBQUAD*)((BYTE*)m_pih + m_pih->biSize);
    m_colorTableSize = (RGBQUAD*)m_pData - m_pColorTable;

    m_myColorTable[0] = RGB(0,0,0);
    m_myColorTable[1] = RGB(255,255,255);

    int n = m_colorTableSize < 2 ? m_colorTableSize : 2;
    for (int i = 0; i < n; i++)
    {
        m_myColorTable[i] = RGB(
            m_pColorTable[i].rgbRed,
            m_pColorTable[i].rgbGreen,
            m_pColorTable[i].rgbBlue
            );
    }

    return true;
}

COLORREF CBitmapReader::GetPixel(UINT x, UINT y)
{
    BYTE* pRow = GetRow(y);
    switch(m_format)
    {
    case trueColor:
        {
            BYTE* pRGB = pRow + 3*x;
            return RGB(pRGB[2], pRGB[1], pRGB[0]);
        }
    case blackWhite:
        {
            BYTE b = pRow[x/8];
            b >>= 7 - x%8;
            return m_myColorTable[b&1];
        }
    }
    return 0;
}

CBitmapWriter::CBitmapWriter()
{
}

bool CBitmapWriter::Init(char const* path, BitmapFormat format, UINT width, UINT height)
{
    UINT bitsPerPixel = 0;
    switch (m_format = format)
    {
    case trueColor:
        bitsPerPixel = 24;
        m_colorTableSize = 0;
        break;
    case blackWhite:
        bitsPerPixel = 1;
        m_colorTableSize = 2;
        break;
    default: return false;
    }

    UINT bitPerRow = bitsPerPixel * width;
    UINT dwordsPerRow = (bitPerRow + 31)/32;
    m_pitch = dwordsPerRow*4;
    UINT headerSize = sizeof(BITMAPFILEHEADER)
                    + sizeof(BITMAPINFOHEADER)
                    + sizeof(RGBQUAD)*m_colorTableSize;

    UINT fileSize = headerSize + m_pitch*height;

    if (!m_file.Init(path, fileSize)) return false;
    m_pfh = (BITMAPFILEHEADER*)m_file.GetData();
    if (m_pfh == 0) return false;

    m_pfh->bfType = 'MB';
    m_pfh->bfSize = fileSize;
    m_pfh->bfReserved1 = 0;
    m_pfh->bfReserved2 = 0;
    m_pfh->bfOffBits = headerSize;

    m_pData = (BYTE*)m_pfh + m_pfh->bfOffBits;
    m_pih = (BITMAPINFOHEADER*)(m_pfh+1);

    m_pih->biSize = sizeof(BITMAPINFOHEADER);
    m_pih->biWidth = width;
    m_pih->biHeight = height;
    m_pih->biPlanes = 1;
    m_pih->biBitCount = (WORD)bitsPerPixel;
    m_pih->biCompression = 0;
    m_pih->biSizeImage = m_pitch*height;
    m_pih->biXPelsPerMeter = 0;
    m_pih->biYPelsPerMeter = 0;
    m_pih->biClrUsed = 0;
    m_pih->biClrImportant = 0;

    m_myColorTable[0] = RGB(0,0,0);
    m_myColorTable[1] = RGB(255,255,255);

    m_pColorTable = (RGBQUAD*)((BYTE*)m_pih + m_pih->biSize);
    for (UINT i = 0; i < m_colorTableSize; i++)
    {
        m_pColorTable[i].rgbRed   = GetRValue(m_myColorTable[i]);
        m_pColorTable[i].rgbGreen = GetGValue(m_myColorTable[i]);
        m_pColorTable[i].rgbBlue  = GetBValue(m_myColorTable[i]);
        m_pColorTable[i].rgbReserved = 0;
    }

    return true;
}

COLORREF CBitmapWriter::GetPixel(UINT x, UINT y)
{
    BYTE* pRow = GetRow(y);
    switch(m_format)
    {
    case trueColor:
        {
            BYTE* pRGB = pRow + 3*x;
            return RGB(pRGB[2], pRGB[1], pRGB[0]);
        }
    case blackWhite:
        {
            BYTE b = pRow[x/8];
            b >>= 7 - x%8;
            return m_myColorTable[b&1];
        }
    }
    return 0;
}

void CBitmapWriter::SetPixel(UINT x, UINT y, COLORREF c)
{
    BYTE* pRow = GetRow(y);
    switch(m_format)
    {
    case trueColor:
        {
            BYTE* pRGB = pRow + 3*x;
            pRGB[2] = GetRValue(c);
            pRGB[1] = GetGValue(c);
            pRGB[0] = GetBValue(c);
        }
        break;
    case blackWhite:
        {
            BYTE &b = pRow[x/8];
            BYTE mask = 0x80 >> x%8;
            if (c == 0) b &= ~mask;
            else        b |= mask;
        }
        break;
    }
}

#endif //BITMAP_IO


