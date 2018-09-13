/****************************************************************************/
/*                                                                          */
/*        MSVIDDRV.H - Include file for messages to video drivers           */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1993, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifndef _INC_MSVIDDRV
#define _INC_MSVIDDRV	50	/* version number */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

/****************************************************************************

                 Digital Video Messages (DVM_)

****************************************************************************/

// General messages
#define DVM_START                         DRV_USER
#define DVM_GETERRORTEXT                  (DVM_START + 0)
#define DVM_GETVIDEOAPIVER                (DVM_START + 1)

// This value increments each time the API changes
// It is passed to the driver in the DRV_OPEN message.
#define VIDEOAPIVERSION 		2

// General messages applicable to all channel types
#define DVM_DIALOG			(DVM_START + 100)
#define DVM_CONFIGURESTORAGE		(DVM_START + 101)
#define DVM_GET_CHANNEL_CAPS         	(DVM_START + 102)
#define DVM_UPDATE         		(DVM_START + 103) 

// Single frame msg
#define DVM_FRAME			(DVM_START + 200)  

// stream messages
#define DVM_STREAM_MSG_START            (DVM_START + 300)
#define DVM_STREAM_MSG_END              (DVM_START + 399)

#define DVM_STREAM_ADDBUFFER            (DVM_START + 300)  
#define DVM_STREAM_FINI                 (DVM_START + 301)  
#define DVM_STREAM_GETERROR             (DVM_START + 302)  
#define DVM_STREAM_GETPOSITION          (DVM_START + 303)  
#define DVM_STREAM_INIT                 (DVM_START + 304)  
#define DVM_STREAM_PREPAREHEADER        (DVM_START + 305)  
#define DVM_STREAM_RESET                (DVM_START + 306)
#define DVM_STREAM_START                (DVM_START + 307)  
#define DVM_STREAM_STOP                 (DVM_START + 308)  
#define DVM_STREAM_UNPREPAREHEADER      (DVM_START + 309)  

// NOTE that DVM_CONFIGURE numbers will start at 0x1000 (for configure API)


/****************************************************************************

                            Open Definitions

****************************************************************************/
#define OPEN_TYPE_VCAP mmioFOURCC('v', 'c', 'a', 'p')

// The following structure is the same as IC_OPEN
// to allow compressors and capture devices to share
// the same DriverProc.

typedef struct tag_video_open_parms {
    DWORD               dwSize;         // sizeof(VIDEO_OPEN_PARMS)
    FOURCC              fccType;        // 'vcap'
    FOURCC              fccComp;        // unused
    DWORD               dwVersion;      // version of msvideo opening you
    DWORD               dwFlags;        // channel type
    DWORD               dwError;        // if open fails, this is why
} VIDEO_OPEN_PARMS, FAR * LPVIDEO_OPEN_PARMS;

typedef struct tag_video_geterrortext_parms {
       DWORD  dwError;          // The error number to identify
       LPSTR  lpText;		// Text buffer to fill
       DWORD  dwLength;		// Size of text buffer.
} VIDEO_GETERRORTEXT_PARMS, FAR * LPVIDEO_GETERRORTEXT_PARMS;

typedef struct tag_video_stream_init_parms {
       DWORD  dwMicroSecPerFrame;
       DWORD  dwCallback;
       DWORD  dwCallbackInst;
       DWORD  dwFlags;
       DWORD  hVideo;
} VIDEO_STREAM_INIT_PARMS, FAR * LPVIDEO_STREAM_INIT_PARMS;

typedef struct tag_video_configure_parms {
       LPDWORD  lpdwReturn;	// Return parameter from configure MSG.
       LPVOID	lpData1;	// Pointer to data 1.
       DWORD	dwSize1;	// size of data buffer 1.
       LPVOID	lpData2;	// Pointer to data 2.
       DWORD	dwSize2;	// size of data buffer 2.
} VIDEOCONFIGPARMS, FAR * LPVIDEOCONFIGPARMS;

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_MSVIDDRV */

