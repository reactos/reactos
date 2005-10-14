
/* $Id: $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 
 * PURPOSE:              Directx headers
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#ifndef __DVP_INCLUDED__
#define __DVP_INCLUDED__

DEFINE_GUID( IID_IDDVideoPortContainer,		0x6C142760,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawVideoPort,		0xB36D93E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDrawVideoPortNotify,    0xA655FB94,0x0589,0x4E57,0xB3,0x33,0x56,0x7A,0x89,0x46,0x8C,0x88);

DEFINE_GUID( DDVPTYPE_E_HREFH_VREFH, 0x54F39980L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFH_VREFL, 0x92783220L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFL_VREFH, 0xA07A02E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_E_HREFL_VREFL, 0xE09C77E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_CCIR656,	     0xFCA326A0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_BROOKTREE,     0x1352A560L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);
DEFINE_GUID( DDVPTYPE_PHILIPS,	     0x332CF160L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8);


typedef struct _DDVIDEOPORTCONNECT
{
    DWORD dwSize;
    DWORD dwPortWidth;
    GUID  guidTypeID;
    DWORD dwFlags;
    ULONG_PTR dwReserved1;
} DDVIDEOPORTCONNECT;

typedef struct _DDVIDEOPORTDESC {
  DWORD              dwSize;
  DWORD              dwFieldWidth;
  DWORD              dwVBIWidth;
  DWORD              dwFieldHeight;
  DWORD              dwMicrosecondsPerField;
  DWORD              dwMaxPixelsPerSecond;
  DWORD              dwVideoPortID;
  DWORD              dwReserved1;
  DDVIDEOPORTCONNECT VideoPortType;
  ULONG_PTR          dwReserved2;
  ULONG_PTR          dwReserved3;
} DDVIDEOPORTDESC;

typedef struct _DDVIDEOPORTBANDWIDTH
{
  DWORD dwSize;
  DWORD dwOverlay;    
  DWORD dwColorkey;	
  DWORD dwYInterpolate;
  DWORD dwYInterpAndColorkey;
  ULONG_PTR dwReserved1;	
  ULONG_PTR dwReserved2;	
} DDVIDEOPORTBANDWIDTH;

typedef struct _DDVIDEOPORTCAPS
{
   DWORD dwSize;		
   DWORD dwFlags;		
   DWORD dwMaxWidth;	
   DWORD dwMaxVBIWidth;
   DWORD dwMaxHeight; 	
   DWORD dwVideoPortID;
   DWORD dwCaps;		
   DWORD dwFX;			
   DWORD dwNumAutoFlipSurfaces;
   DWORD dwAlignVideoPortBoundary;	
   DWORD dwAlignVideoPortPrescaleWidth;
   DWORD dwAlignVideoPortCropBoundary;	
   DWORD dwAlignVideoPortCropWidth;	
   DWORD dwPreshrinkXStep;	
   DWORD dwPreshrinkYStep;	
   DWORD dwNumVBIAutoFlipSurfaces;	
   DWORD dwNumPreferredAutoflip;
   WORD  wNumFilterTapsX;
   WORD  wNumFilterTapsY;
} DDVIDEOPORTCAPS;

typedef struct _DDVIDEOPORTINFO
{
    DWORD           dwSize;
    DWORD           dwOriginX;
    DWORD           dwOriginY;
    DWORD           dwVPFlags;
    RECT            rCrop;
    DWORD           dwPrescaleWidth;
    DWORD           dwPrescaleHeight;
    LPDDPIXELFORMAT lpddpfInputFormat;
    LPDDPIXELFORMAT lpddpfVBIInputFormat;
    LPDDPIXELFORMAT lpddpfVBIOutputFormat;
    DWORD           dwVBIHeight;
    ULONG_PTR       dwReserved1;
    ULONG_PTR       dwReserved2;
} DDVIDEOPORTINFO;

typedef struct _DDVIDEOPORTSTATUS
{
    DWORD              dwSize;
    BOOL               bInUse;
    DWORD              dwFlags;
    DWORD              dwReserved1;
    DDVIDEOPORTCONNECT VideoPortType;
    ULONG_PTR          dwReserved2;
    ULONG_PTR          dwReserved3;
} DDVIDEOPORTSTATUS;

typedef struct _DDVIDEOPORTNOTIFY
{
    LARGE_INTEGER ApproximateTimeStamp;	
    LONG          lField;                        
    UINT          dwSurfaceIndex;                
    LONG          lDone;                         
} DDVIDEOPORTNOTIFY;



typedef struct _DDVIDEOPORTCONNECT   *LPDDVIDEOPORTCONNECT;
typedef struct _DDVIDEOPORTCAPS      *LPDDVIDEOPORTCAPS;
typedef struct _DDVIDEOPORTDESC      *LPDDVIDEOPORTDESC;
typedef struct _DDVIDEOPORTINFO      *LPDDVIDEOPORTINFO;
typedef struct _DDVIDEOPORTBANDWIDTH *LPDDVIDEOPORTBANDWIDTH;
typedef struct _DDVIDEOPORTSTATUS    *LPDDVIDEOPORTSTATUS;
typedef struct _DDVIDEOPORTNOTIFY    *LPDDVIDEOPORTNOTIFY;

#endif
