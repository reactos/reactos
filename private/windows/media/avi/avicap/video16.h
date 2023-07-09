/***************************************************************************
 *
 *  video16.h
 *
 *  Copyright (c) 1994  Microsoft Corporation
 *
 *  32-bit Thunks for avicap32.dll
 *
 *  Structures for mapping video
 *
 **************************************************************************/


/*
 *  Make sure the compiler doesn't think it knows better about packing
 *  The 16-bit stack is effectively pack(2)
 */

 #pragma pack(2)

/*
 *  Definitions to help with thunking video calls
 */

 typedef WORD HVIDEO16;
 typedef HVIDEO16 *LPHVIDEO16;


/*
 *  Note that everything is in the reverse order to keep with the PASCAL
 *  calling convention on the other side
 */


/****************************************************************************

   video entry point parameter lists

 ****************************************************************************/


typedef struct {
    DWORD    dwP2;
    DWORD    dwP1;
    WORD     msg;
    HVIDEO16 hVideo;
} UNALIGNED *PvideoMessageParms16;

typedef struct {
    DWORD    dwFlags;
    DWORD    dwDeviceId;
    LPHVIDEO16 lphVideo;
} UNALIGNED *PvideoOpenParms16;

typedef struct {
    HVIDEO16 hVideo;
} UNALIGNED *PvideoCloseParms16;


/*
 *  Our shadow header structure for use with callbacks
 *  (see videoStreamAddBuffer)
 */

typedef struct {
    LPVOID      pHdr16;        /* Remember address on 16-bit side */
    LPVOID      pHdr32;        /* 32-bit version of pHdr16        */
    LPBYTE      lpData16;      /* Remember pointer for flushing   */
    VIDEOHDR    videoHdr;
} VIDEOHDR32, *PVIDEOHDR32;


/*
 *  Instance data for videoStreamInit - contains pointer to 16-bit side
 *  instance data
 */

typedef struct {
    DWORD dwFlags;                // Real flags
    DWORD dwCallbackInst;         // Real instance data
    DWORD dwCallback;
    HVIDEO16 hVideo;
} VIDEOINSTANCEDATA32, *PVIDEOINSTANCEDATA32;

/*
 *  Thunk 16-bit mmtime
 */

#pragma pack(2)

typedef struct {
   WORD    wType;              /* indicates the contents of the union */
   union {
       DWORD ms;               /* milliseconds */
       DWORD sample;           /* samples */
       DWORD cb;               /* byte count */
       struct {                /* SMPTE */
           BYTE hour;          /* hours */
           BYTE min;           /* minutes */
           BYTE sec;           /* seconds */
           BYTE frame;         /* frames  */
           BYTE fps;           /* frames per second */
           BYTE dummy;         /* pad */
           } smpte;
       struct {                /* MIDI */
           DWORD songptrpos;   /* song pointer position */
           } midi;
       } u;
   } MMTIME16;

#pragma pack()


