/****************************************************************************/
/*                                                                          */
/* ivideo32.h                                                               */
/*                                                                          */
/* private structures & prototypes for 32bit videoXXX api's                 */
/* this header file is specific to WIN32                                    */
/*                                                                          */
/****************************************************************************/

// include public stuff about the videoXXX interface
//
#include <vfw.h>

// include private stuff IFF _WIN32 and we have not already done so
//
#if !defined _INC_IVIDEO32 && defined _WIN32
#define _INC_IVIDEO32

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#ifndef _RCINVOKED


/****************************************************************************

                         Structures

****************************************************************************/
#if 0
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
#endif

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

DWORD WINAPI videoConfigureStorageA(HVIDEO hVideo,
                      LPSTR lpstrIdent, DWORD dwFlags);
DWORD WINAPI videoConfigureStorageW(HVIDEO hVideo,
                      LPWSTR lpstrIdent, DWORD dwFlags);
#ifdef UNICODE
  #define videoConfigureStorage  videoConfigureStorageW
#else
  #define videoConfigureStorage  videoConfigureStorageA
#endif // !UNICODE

DWORD WINAPI videoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
DWORD WINAPI videoMessage(HVIDEO hVideo, UINT msg, LPARAM dwP1, LPARAM dwP2);

/* streaming APIs */
DWORD WINAPI videoStreamAddBuffer(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);
DWORD WINAPI videoStreamGetError(HVIDEO hVideo, LPDWORD lpdwErrorFirst,
        LPDWORD lpdwErrorLast);

DWORD WINAPI videoGetErrorTextA(HVIDEO hVideo, UINT wError,
              LPSTR lpText, UINT wSize);
DWORD WINAPI videoGetErrorTextW(HVIDEO hVideo, UINT wError,
              LPWSTR lpText, UINT wSize);

#ifdef UNICODE
  #define videoGetErrorText  videoGetErrorTextW
#else
  #define videoGetErrorText  videoGetErrorTextA
#endif // !UNICODE

DWORD WINAPI videoStreamGetPosition(HVIDEO hVideo, MMTIME FAR* lpInfo,
              DWORD dwSize);
DWORD WINAPI videoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD_PTR dwCallback,
              DWORD_PTR dwCallbackInst, DWORD dwFlags);
DWORD WINAPI videoStreamFini(HVIDEO hVideo);
DWORD WINAPI videoStreamPrepareHeader(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);
DWORD WINAPI videoStreamReset(HVIDEO hVideo);
DWORD WINAPI videoStreamStart(HVIDEO hVideo);
DWORD WINAPI videoStreamStop(HVIDEO hVideo);
DWORD WINAPI videoStreamUnprepareHeader(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);

// Added post VFW1.1a
DWORD WINAPI videoStreamAllocHdrAndBuffer(HVIDEO hVideo,
              LPVIDEOHDR FAR * plpVHdr, DWORD dwSize);
DWORD WINAPI videoStreamFreeHdrAndBuffer(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr);


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
#define DVM_DST_RECT                    (DVM_CONFIGURE_START + 5)

#endif  /* ifndef _RCINVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif // _INC_VIDEO32
