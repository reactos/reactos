/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            include/win32k/ntusrtyp.h
 * PURPOSE:         Win32 Shared USER Types for NtUser*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTUSRTYP_
#define _NTUSRTYP_

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _PATRECT
{
    RECT r;
    HBRUSH hBrush;
} PATRECT, * PPATRECT;

/* Structures for reading icon/cursor files and resources */
#pragma pack(push,1)
typedef struct _ICONIMAGE
{
    BITMAPINFOHEADER icHeader;      // DIB header
    RGBQUAD icColors[1];            // Color table
    BYTE icXOR[1];                  // DIB bits for XOR mask
    BYTE icAND[1];                  // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;

typedef struct _CURSORIMAGE
{
    BITMAPINFOHEADER icHeader;       // DIB header
    RGBQUAD icColors[1];             // Color table
    BYTE icXOR[1];                   // DIB bits for XOR mask
    BYTE icAND[1];                   // DIB bits for AND mask
} CURSORIMAGE, *LPCURSORIMAGE;

typedef struct
{
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
} ICONRESDIR;

typedef struct
{
    WORD wWidth;
    WORD wHeight;
} CURSORRESDIR;

typedef struct
{
    WORD wPlanes;                   // Number of Color Planes in the XOR image
    WORD wBitCount;                 // Bits per pixel in the XOR image
} ICONDIR;

typedef struct
{
    WORD wXHotspot;                 // Number of Color Planes in the XOR image
    WORD wYHotspot;                 // Bits per pixel in the XOR image
} CURSORDIR;

typedef struct
{
    BYTE bWidth;                    // Width, in pixels, of the icon image
    BYTE bHeight;                   // Height, in pixels, of the icon image
    BYTE bColorCount;               // Number of colors in image (0 if >=8bpp)
    BYTE bReserved;                 // Reserved ( must be 0)
    union
    {
        ICONDIR icon;
        CURSORDIR  cursor;
    } Info;
    DWORD dwBytesInRes;             // How many bytes in this resource?
    DWORD dwImageOffset;            // Where in the file is this image?
} CURSORICONDIRENTRY;

typedef struct
{
    WORD idReserved;                // Reserved (must be 0)
    WORD idType;                    // Resource Type (1 for icons, 0 for cursors)
    WORD idCount;                   // How many images?
    CURSORICONDIRENTRY idEntries[1];// An entry for idCount number of images
} CURSORICONDIR;

typedef struct
{
    union
    {
        ICONRESDIR icon;
        CURSORRESDIR cursor;
    } ResInfo;
    WORD wPlanes;                   // Color Planes
    WORD wBitCount;                 // Bits per pixel
    DWORD dwBytesInRes;             // how many bytes in this resource?
    WORD nID;                       // the ID
} GRPCURSORICONDIRENTRY;

typedef struct
{
    WORD idReserved;                    // Reserved (must be 0)
    WORD idType;                        // Resource type (1 for icons)
    WORD idCount;                       // How many images?
    GRPCURSORICONDIRENTRY idEntries[1]; // The entries for each image
} GRPCURSORICONDIR;
#pragma pack(pop)

typedef struct _THRDCARETINFO
{
    HWND hWnd;
    HBITMAP Bitmap;
    POINT Pos;
    SIZE Size;
    BYTE Visible;
    BYTE Showing;
} THRDCARETINFO, *PTHRDCARETINFO;

#endif
