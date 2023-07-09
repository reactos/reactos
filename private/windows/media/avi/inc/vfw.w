/****************************************************************************
 *
 *      VfW.H - Video for windows include file for WIN32
 *
 *      Copyright (c) 1991-1995, Microsoft Corp.  All rights reserved.
 *
 *      This include files defines interfaces to the following
 *      video components
 *
 *          COMPMAN         - Installable Compression Manager.
 *          DRAWDIB         - Routines for drawing to the display.
 *          VIDEO           - Video Capture Driver Interface
 *
 *          AVIFMT          - AVI File Format structure definitions.
 *          MMREG           - FOURCC and other things
 *
 *          AVIFile         - Interface for reading AVI Files and AVI Streams
 *          MCIWND          - MCI/AVI window class
 *          AVICAP          - AVI Capture Window class
 *
 *          MSACM           - Audio compression manager.
 *
 *      The following symbols control inclusion of various parts of this file:
 *
 *          NOCOMPMAN       - dont include COMPMAN
 *          NODRAWDIB       - dont include DRAWDIB
 *          NOVIDEO         - dont include video capture interface
 *
 *          NOAVIFMT        - dont include AVI file format structs
 *          NOMMREG         - dont include MMREG
 *
 *          NOAVIFILE       - dont include AVIFile interface
 *          NOMCIWND        - dont include AVIWnd class.
 *          NOAVICAP        - dont include AVICap class.
 *
 *          NOMSACM         - dont include ACM stuff.
 *
 ****************************************************************************/

#ifndef _INC_VFW
#define _INC_VFW

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

/****************************************************************************
 *
 *  types
 *
 ***************************************************************************/

#define VFWAPI  WINAPI
#define VFWAPIV WINAPIV

/****************************************************************************
 *
 *  VideoForWindowsVersion() - returns version of VfW
 *
 ***************************************************************************/

DWORD FAR PASCAL VideoForWindowsVersion(void);

/****************************************************************************
 *
 *  call these to start stop using VfW from your app.
 *
 ***************************************************************************/

LONG VFWAPI InitVFW(void);
LONG VFWAPI TermVFW(void);

/****************************************************************************
 *
 *  do we need MMSYSTEM?
 *
 ****************************************************************************/

#if !defined(_INC_MMSYSTEM) && (!defined(NOVIDEO) || !defined(NOAVICAP))
    #include <mmsystem.h>
#endif

/****************************************************************************/
/*                                                                          */
/*        Macros                                                            */
/*                                                                          */
/*  should we define this??                                                 */
/*                                                                          */
/****************************************************************************/

#ifndef MKFOURCC
#define MKFOURCC( ch0, ch1, ch2, ch3 )                                    \
		( (DWORD)(BYTE)(ch0) | ( (DWORD)(BYTE)(ch1) << 8 ) |	\
		( (DWORD)(BYTE)(ch2) << 16 ) | ( (DWORD)(BYTE)(ch3) << 24 ) )
#endif

#if !defined(_INC_MMSYSTEM)
    #define mmioFOURCC MKFOURCC
#endif

