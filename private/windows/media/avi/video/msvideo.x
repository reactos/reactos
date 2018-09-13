/****************************************************************************/
/*                                                                          */
/*        MSVIDEO.H - Include file for Video APIs                           */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1992, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifndef _INC_MSVIDEO
#define _INC_MSVIDEO	50	/* version number */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifndef _RCINVOKED

#ifdef BUILDDLL                         /* ;Internal */
#undef WINAPI                           /* ;Internal */
#define WINAPI FAR PASCAL _loadds       /* ;Internal */
#endif                                  /* ;Internal */

/* video data types */
DECLARE_HANDLE(HVIDEO);                 // generic handle
typedef HVIDEO FAR * LPHVIDEO;
#endif                                  // ifndef RCINVOKED

/****************************************************************************

                        version api

****************************************************************************/

DWORD FAR PASCAL VideoForWindowsVersion(void);

/****************************************************************************

                            Error Return Values

****************************************************************************/
#define DV_ERR_OK               (0)                  /* No error */
#define DV_ERR_BASE             (1)                  /* Error Base */
#define DV_ERR_NONSPECIFIC      (DV_ERR_BASE)
#define DV_ERR_BADFORMAT        (DV_ERR_BASE + 1)
				/* unsupported video format */
#define DV_ERR_STILLPLAYING     (DV_ERR_BASE + 2)
				/* still something playing */
#define DV_ERR_UNPREPARED       (DV_ERR_BASE + 3)
				/* header not prepared */
#define DV_ERR_SYNC             (DV_ERR_BASE + 4)
				/* device is synchronous */
#define DV_ERR_TOOMANYCHANNELS  (DV_ERR_BASE + 5)
				/* number of channels exceeded */
#define DV_ERR_NOTDETECTED	(DV_ERR_BASE + 6)    /* HW not detected */
#define DV_ERR_BADINSTALL	(DV_ERR_BASE + 7)    /* Can not get Profile */
#define DV_ERR_CREATEPALETTE	(DV_ERR_BASE + 8)
#define DV_ERR_SIZEFIELD	(DV_ERR_BASE + 9)
#define DV_ERR_PARAM1		(DV_ERR_BASE + 10)
#define DV_ERR_PARAM2		(DV_ERR_BASE + 11)
#define DV_ERR_CONFIG1		(DV_ERR_BASE + 12)
#define DV_ERR_CONFIG2		(DV_ERR_BASE + 13)
#define DV_ERR_FLAGS		(DV_ERR_BASE + 14)
#define DV_ERR_13		(DV_ERR_BASE + 15)

#define DV_ERR_NOTSUPPORTED     (DV_ERR_BASE + 16)   /* function not suported */
#define DV_ERR_NOMEM            (DV_ERR_BASE + 17)   /* out of memory */
#define DV_ERR_ALLOCATED        (DV_ERR_BASE + 18)   /* device is allocated */
#define DV_ERR_BADDEVICEID      (DV_ERR_BASE + 19)
#define DV_ERR_INVALHANDLE      (DV_ERR_BASE + 20)
#define DV_ERR_BADERRNUM        (DV_ERR_BASE + 21)
#define DV_ERR_NO_BUFFERS       (DV_ERR_BASE + 22)   /* out of buffers */

#define DV_ERR_MEM_CONFLICT     (DV_ERR_BASE + 23)   /* Mem conflict detected */
#define DV_ERR_IO_CONFLICT      (DV_ERR_BASE + 24)   /* I/O conflict detected */
#define DV_ERR_DMA_CONFLICT     (DV_ERR_BASE + 25)   /* DMA conflict detected */
#define DV_ERR_INT_CONFLICT     (DV_ERR_BASE + 26)   /* Interrupt conflict detected */
#define DV_ERR_PROTECT_ONLY     (DV_ERR_BASE + 27)   /* Can not run in standard mode */
#define DV_ERR_LASTERROR        (DV_ERR_BASE + 27)

#define DV_IDS_PROFILING        (DV_ERR_BASE + 900)
#define DV_IDS_LISTBOX          (DV_ERR_BASE + 901)

#define DV_ERR_USER_MSG         (DV_ERR_BASE + 1000) /* Hardware specific errors */

/****************************************************************************

                         Callback Messages

Note that the values for all installable driver callback messages are
identical, (ie. MM_DRVM_DATA has the same value for capture drivers,
installable video codecs, and the audio compression manager).
****************************************************************************/
#ifndef _RCINVOKED

#ifndef MM_DRVM_OPEN
#define MM_DRVM_OPEN       0x3D0
#define MM_DRVM_CLOSE      0x3D1
#define MM_DRVM_DATA       0x3D2
#define MM_DRVM_ERROR      0x3D3
#endif

#define DV_VM_OPEN         MM_DRVM_OPEN         // Obsolete messages
#define DV_VM_CLOSE        MM_DRVM_CLOSE
#define DV_VM_DATA         MM_DRVM_DATA
#define DV_VM_ERROR        MM_DRVM_ERROR

/****************************************************************************

                         Structures

****************************************************************************/
/* video data block header */
typedef struct videohdr_tag {
    LPBYTE      lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* Length of data buffer */
    DWORD       dwBytesUsed;            /* Bytes actually used */
    DWORD       dwTimeCaptured;         /* Milliseconds from start of stream */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwReserved[4];          /* reserved for driver */
} VIDEOHDR, NEAR *PVIDEOHDR, FAR * LPVIDEOHDR;

/* dwFlags field of VIDEOHDR */
#define VHDR_DONE       0x00000001  /* Done bit */
#define VHDR_PREPARED   0x00000002  /* Set if this header has been prepared */
#define VHDR_INQUEUE    0x00000004  /* Reserved for driver */
#define VHDR_KEYFRAME   0x00000008  /* Key Frame */
#define VHDR_VALID      0x0000000F  /* valid flags */     /* ;Internal */

/* Channel capabilities structure */
typedef struct channel_caps_tag {
    DWORD       dwFlags;                /* Capability flags*/
    DWORD       dwSrcRectXMod;          /* Granularity of src rect in x */
    DWORD       dwSrcRectYMod;          /* Granularity of src rect in y */
    DWORD       dwSrcRectWidthMod;      /* Granularity of src rect width */
    DWORD       dwSrcRectHeightMod;     /* Granularity of src rect height */
    DWORD       dwDstRectXMod;          /* Granularity of dst rect in x */
    DWORD       dwDstRectYMod;          /* Granularity of dst rect in y */
    DWORD       dwDstRectWidthMod;      /* Granularity of dst rect width */
    DWORD       dwDstRectHeightMod;     /* Granularity of dst rect height */
} CHANNEL_CAPS, NEAR *PCHANNEL_CAPS, FAR * LPCHANNEL_CAPS;

/* dwFlags of CHANNEL_CAPS */
#define VCAPS_OVERLAY       0x00000001      /* overlay channel */
#define VCAPS_SRC_CAN_CLIP  0x00000002      /* src rect can clip */
#define VCAPS_DST_CAN_CLIP  0x00000004      /* dst rect can clip */
#define VCAPS_CAN_SCALE     0x00000008      /* allows src != dst */

/****************************************************************************

                        video APIs

****************************************************************************/

DWORD WINAPI videoGetNumDevs(void);

DWORD WINAPI videoOpen  (LPHVIDEO lphVideo,
              DWORD dwDevice, DWORD dwFlags);
DWORD WINAPI videoClose (HVIDEO hVideo);
DWORD WINAPI videoDialog(HVIDEO hVideo, HWND hWndParent, DWORD dwFlags);
DWORD WINAPI videoGetChannelCaps(HVIDEO hVideo, LPCHANNEL_CAPS lpChannelCaps,
                DWORD dwSize);
DWORD WINAPI videoUpdate (HVIDEO hVideo, HWND hWnd, HDC hDC);
DWORD WINAPI videoConfigure (HVIDEO hVideo, UINT msg, DWORD dwFlags,
		LPDWORD lpdwReturn, LPVOID lpData1, DWORD dwSize1,
                LPVOID lpData2, DWORD dwSize2);
DWORD WINAPI videoConfigureStorage%(HVIDEO hVideo,
			LPTSTR% lpstrIdent, DWORD dwFlags);
DWORD WINAPI videoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
DWORD WINAPI videoMessage(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2);

/* streaming APIs */
DWORD WINAPI videoStreamAddBuffer(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);
DWORD WINAPI videoStreamGetError(HVIDEO hVideo, LPDWORD lpdwErrorFirst,
        LPDWORD lpdwErrorLast);
DWORD WINAPI videoGetErrorText%(HVIDEO hVideo, UINT wError,
	        LPTSTR% lpText, UINT wSize);
DWORD WINAPI videoStreamGetPosition(HVIDEO hVideo, MMTIME FAR* lpInfo,
              DWORD dwSize);
DWORD WINAPI videoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD dwCallback,
              DWORD dwCallbackInst, DWORD dwFlags);
DWORD WINAPI videoStreamFini(HVIDEO hVideo);
DWORD WINAPI videoStreamPrepareHeader(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);
DWORD WINAPI videoStreamReset(HVIDEO hVideo);
DWORD WINAPI videoStreamStart(HVIDEO hVideo);
DWORD WINAPI videoStreamStop(HVIDEO hVideo);
DWORD WINAPI videoStreamUnprepareHeader(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);


/****************************************************************************

			API Flags

****************************************************************************/

// Types of channels to open with the videoOpen function
#define VIDEO_EXTERNALIN		0x0001
#define VIDEO_EXTERNALOUT		0x0002
#define VIDEO_IN			0x0004
#define VIDEO_OUT			0x0008

// Is a driver dialog available for this channel?
#define VIDEO_DLG_QUERY			0x0010

// videoConfigure (both GET and SET)
#define VIDEO_CONFIGURE_QUERY   	0x8000

// videoConfigure (SET only)
#define VIDEO_CONFIGURE_SET		0x1000

// videoConfigure (GET only)
#define VIDEO_CONFIGURE_GET		0x2000
#define VIDEO_CONFIGURE_QUERYSIZE	0x0001

#define VIDEO_CONFIGURE_CURRENT		0x0010
#define VIDEO_CONFIGURE_NOMINAL		0x0020
#define VIDEO_CONFIGURE_MIN		0x0040
#define VIDEO_CONFIGURE_MAX		0x0080


/****************************************************************************

			CONFIGURE MESSAGES

****************************************************************************/
#define DVM_USER                        0X4000

#define DVM_CONFIGURE_START		0x1000
#define DVM_CONFIGURE_END		0x1FFF

#define DVM_PALETTE			(DVM_CONFIGURE_START + 1)
#define DVM_FORMAT			(DVM_CONFIGURE_START + 2)
#define DVM_PALETTERGB555		(DVM_CONFIGURE_START + 3)
#define DVM_SRC_RECT    		(DVM_CONFIGURE_START + 4)
#define DVM_DST_RECT    		(DVM_CONFIGURE_START + 5)

#endif  /* ifndef _RCINVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_MSVIDEO */
