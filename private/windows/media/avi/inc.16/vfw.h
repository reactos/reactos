/****************************************************************************
 *
 *      VfW.H - Video for windows include file version 1.1
 *
 *      Copyright (c) 1991-1994, Microsoft Corp.  All rights reserved.
 *
 *      This include files defines interfaces to the following
 *      components of VfW 1.0 OR VFW 1.1
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

#ifdef WIN32
#define VFWAPI  WINAPI
#define VFWAPIV WINAPIV
#else
#ifndef VFWAPI
#define VFWAPI  FAR PASCAL
#define VFWAPIV FAR CDECL
#endif
#endif

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

/**************************************************************************
 *
 *  DRAWDIB - Routines for drawing to the display.
 *
 *************************************************************************/

#ifndef NODRAWDIB
    #include <drawdib.h>
#endif  /* NODRAWDIB */

/****************************************************************************
 *
 *  AVIFMT - AVI file format definitions
 *
 ****************************************************************************/

#ifndef NOAVIFMT
    #ifndef _INC_MMSYSTEM
        typedef DWORD FOURCC;
    #endif
    #include <avifmt.h>
#endif /* NOAVIFMT */

/****************************************************************************
 *
 *  MMREG.H (standard include file for MM defines, like FOURCC and things)
 *
 ***************************************************************************/

#ifndef NOMMREG
    #include <mmreg.h>
#endif

/****************************************************************************
 *
 *  AVIFile - routines for reading/writing standard AVI files
 *
 ***************************************************************************/

#ifndef NOAVIFILE
    #include <avifile.h>
#endif  /* NOAVIFILE */

/****************************************************************************
 *
 *  COMPMAN - Installable Compression Manager.
 *
 ****************************************************************************/

#ifndef NOCOMPMAN
    #include <compman.h>
#endif  /* NOCOMPMAN */

/****************************************************************************
 *
 *  MCIWnd - Window class for MCI objects
 *
 ***************************************************************************/

#ifndef NOMCIWND
    #include <mciwnd.h>
#endif  /* NOAVIFILE */

/****************************************************************************
 *
 *  VIDEO - Video Capture Driver Interface
 *
 ****************************************************************************/

#if !defined(NOAVICAP) || !defined(NOVIDEO)
    #include <msvideo.h>
#endif  /* NOVIDEO */

/****************************************************************************
 *
 *  AVICAP - Window class for AVI capture
 *
 ***************************************************************************/

#ifndef NOAVICAP
    #include <avicap.h>
#endif  /* NOAVIFILE */

/****************************************************************************
 *
 *  ACM (Audio compression manager)
 *
 ***************************************************************************/

#ifndef NOMSACM
    #include <msacm.h>
#endif

/****************************************************************************
 *
 *  File Preview dialog (if commdlg.h was included already)
 *
 ***************************************************************************/

#ifdef OFN_READONLY
    BOOL VFWAPI GetOpenFileNamePreview(LPOPENFILENAME lpofn);
    BOOL VFWAPI GetSaveFileNamePreview(LPOPENFILENAME lpofn);
#endif

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  /* _INC_VFW */
