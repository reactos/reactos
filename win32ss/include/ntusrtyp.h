/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Win32 Graphical Subsystem (WIN32K)
 * FILE:            win32ss/include/ntusrtyp.h
 * PURPOSE:         Win32 Shared USER Types for NtUser*
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#ifndef _NTUSRTYP_
#define _NTUSRTYP_

#include <ntwin32.h>

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

/* Bitfields for UserpreferencesMask SPI_ values (with defaults) */
/* See also https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-2000-server/cc957204(v=technet.10) */
typedef struct {
    DWORD bActiveWindowTracking:1;      //0 SPI_GETACTIVEWINDOWTRACKING
    DWORD bMenuAnimation:1;             //1 SPI_GETMENUANIMATION
    DWORD bComboBoxAnimation:1;         //1 SPI_GETCOMBOBOXANIMATION
    DWORD bListBoxSmoothScrolling:1;    //1 SPI_GETLISTBOXSMOOTHSCROLLING
    DWORD bGradientCaptions:1;          //1 SPI_GETGRADIENTCAPTIONS
    DWORD bKeyboardCues:1;              //0 SPI_GETKEYBOARDCUES
    DWORD bActiveWndTrkZorder:1;        //0 SPI_GETACTIVEWNDTRKZORDER
    DWORD bHotTracking:1;               //1 SPI_GETHOTTRACKING
    DWORD bReserved1:1;                 //0 Reserved
    DWORD bMenuFade:1;                  //1 SPI_GETMENUFADE
    DWORD bSelectionFade:1;             //1 SPI_GETSELECTIONFADE
    DWORD bTooltipAnimation:1;          //1 SPI_GETTOOLTIPANIMATION
    DWORD bTooltipFade:1;               //1 SPI_GETTOOLTIPFADE
    DWORD bCursorShadow:1;              //1 SPI_GETCURSORSHADOW
    DWORD bReserved2:17;                //0 Reserved
    DWORD bUiEffects:1;                 //1 SPI_GETUIEFFECTS
} USERPREFERENCESMASK, *PUSERPREFERENCESMASK;

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
    WORD   wWidth;
    WORD   wHeight;
} CURSORDIR;

typedef struct
{   union
    {
        ICONRESDIR icon;
        CURSORRESDIR  cursor;
    } ResInfo;
    WORD   wPlanes;
    WORD   wBitCount;
    DWORD  dwBytesInRes;
    WORD   wResId;
} CURSORICONDIRENTRY;

typedef struct
{
    WORD                idReserved;
    WORD                idType;
    WORD                idCount;
    CURSORICONDIRENTRY  idEntries[1];
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
