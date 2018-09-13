/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
 *
 *  FILE:   DIGITALV.H
 *      
 *  DESCRIPTION:
 *      Include file for the MCI Digital Video Command Set
 *
 *  HISTORY:
 *      Aug 19, 1992    -Version 1.0 Release
 *
 **************************************************************************/

#ifndef _INC_DIGITALV
#define _INC_DIGITALV  100

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#define MCI_TEST                            0x00000020L

/* Message values */

#define MCI_CAPTURE                         0x0870 
#define MCI_MONITOR                         0x0871 
#define MCI_RESERVE                         0x0872 
#define MCI_SETAUDIO                        0x0873 
#define MCI_SIGNAL                          0x0875 
#define MCI_SETVIDEO                        0x0876 
#define MCI_QUALITY                         0x0877
#define MCI_LIST                            0x0878
#define MCI_UNDO                            0x0879
#define MCI_CONFIGURE                       0x087a
#define MCI_RESTORE                         0x087b

/* Return and string constant values */

#define MCI_ON   1
#define MCI_OFF  0

#define MCI_DGV_FILE_MODE_SAVING            0x0001
#define MCI_DGV_FILE_MODE_LOADING           0x0002
#define MCI_DGV_FILE_MODE_EDITING           0x0003
#define MCI_DGV_FILE_MODE_IDLE              0x0004

/* These identifiers are used only by device drivers */

#define MCI_ON_S                            0x00008000L
#define MCI_OFF_S                           0x00008001L
#define MCI_DGV_FILE_S                      0x00008002L
#define MCI_DGV_INPUT_S                     0x00008003L

#define MCI_DGV_FILE_MODE_SAVING_S          0x00008004L
#define MCI_DGV_FILE_MODE_LOADING_S         0x00008005L
#define MCI_DGV_FILE_MODE_EDITING_S         0x00008006L
#define MCI_DGV_FILE_MODE_IDLE_S            0x00008007L

#define MCI_DGV_SETVIDEO_SRC_NTSC_S         0x00008010L
#define MCI_DGV_SETVIDEO_SRC_RGB_S          0x00008011L
#define MCI_DGV_SETVIDEO_SRC_SVIDEO_S       0x00008012L
#define MCI_DGV_SETVIDEO_SRC_PAL_S          0x00008013L
#define MCI_DGV_SETVIDEO_SRC_SECAM_S        0x00008014L
#define MCI_DGV_SETVIDEO_SRC_GENERIC_S      0x00008015L

#define MCI_DGV_SETAUDIO_SRC_LEFT_S         0x00008020L
#define MCI_DGV_SETAUDIO_SRC_RIGHT_S        0x00008021L
#define MCI_DGV_SETAUDIO_SRC_AVERAGE_S      0x00008022L
#define MCI_DGV_SETAUDIO_SRC_STEREO_S       0x00008023L

/* Window message for signal notification */

#ifndef MM_MCISIGNAL
#define MM_MCISIGNAL                        0x3CB
#endif
				    
/* error values */

#define MCIERR_DGV_DEVICE_LIMIT             (MCIERR_CUSTOM_DRIVER_BASE+0)
#define MCIERR_DGV_IOERR                    (MCIERR_CUSTOM_DRIVER_BASE+1) 
#define MCIERR_DGV_WORKSPACE_EMPTY          (MCIERR_CUSTOM_DRIVER_BASE+2) 
#define MCIERR_DGV_DISK_FULL                (MCIERR_CUSTOM_DRIVER_BASE+3) 
#define MCIERR_DGV_DEVICE_MEMORY_FULL       (MCIERR_CUSTOM_DRIVER_BASE+4)
#define MCIERR_DGV_BAD_CLIPBOARD_RANGE      (MCIERR_CUSTOM_DRIVER_BASE+5)
 
/* defines for monitor methods */

#define MCI_DGV_METHOD_PRE                  0x0000a000L
#define MCI_DGV_METHOD_POST                 0x0000a001L
#define MCI_DGV_METHOD_DIRECT               0x0000a002L

/* defines for known file formats */

#define MCI_DGV_FF_AVSS                     0x00004000L
#define MCI_DGV_FF_AVI                      0x00004001L
#define MCI_DGV_FF_DIB                      0x00004002L
#define MCI_DGV_FF_RDIB                     0x00004003L
#define MCI_DGV_FF_JPEG                     0x00004004L
#define MCI_DGV_FF_RJPEG                    0x00004005L
#define MCI_DGV_FF_JFIF                     0x00004006L
#define MCI_DGV_FF_MPEG                     0x00004007L

/* values for dwItem field of MCI_CAPABILITY_PARMS structure */

#define MCI_DGV_GETDEVCAPS_CAN_LOCK         0x00004000L
#define MCI_DGV_GETDEVCAPS_CAN_STRETCH      0x00004001L 
#define MCI_DGV_GETDEVCAPS_CAN_FREEZE       0x00004002L
#define MCI_DGV_GETDEVCAPS_MAX_WINDOWS      0x00004003L 
#define MCI_DGV_GETDEVCAPS_CAN_REVERSE      0x00004004L 
#define MCI_DGV_GETDEVCAPS_HAS_STILL        0x00004005L
#define MCI_DGV_GETDEVCAPS_PALETTES         0x00004006L
#define MCI_DGV_GETDEVCAPS_CAN_STR_IN       0x00004008L 
#define MCI_DGV_GETDEVCAPS_CAN_TEST         0x00004009L
#define MCI_DGV_GETDEVCAPS_MAXIMUM_RATE     0x0000400aL
#define MCI_DGV_GETDEVCAPS_MINIMUM_RATE     0x0000400bL

/* flags for dwFlags parameter of MCI_CAPTURE command message */

#define MCI_DGV_CAPTURE_AS                  0x00010000L
#define MCI_DGV_CAPTURE_AT                  0x00020000L

/* flags for dwFlags parameter of MCI_COPY command message */

#define MCI_DGV_COPY_AT                     0x00010000L
#define MCI_DGV_COPY_AUDIO_STREAM           0x00020000L
#define MCI_DGV_COPY_VIDEO_STREAM           0x00040000L

/* flags for dwFlags parameter of MCI_CUE command message */

#define MCI_DGV_CUE_INPUT                   0x00010000L 
#define MCI_DGV_CUE_OUTPUT                  0x00020000L
#define MCI_DGV_CUE_NOSHOW                  0x00040000L 

/* flags for dwFlags parameter of MCI_CUT command message */

#define MCI_DGV_CUT_AT                      0x00010000L
#define MCI_DGV_CUT_AUDIO_STREAM            0x00020000L
#define MCI_DGV_CUT_VIDEO_STREAM            0x00040000L

/* flags for dwFlags parameter of MCI_DELETE command message */

#define MCI_DGV_DELETE_AT                   0x00010000L
#define MCI_DGV_DELETE_AUDIO_STREAM         0x00020000L
#define MCI_DGV_DELETE_VIDEO_STREAM         0x00040000L

/* flags for dwFlags parameter of MCI_FREEZE command message */

#define MCI_DGV_FREEZE_AT                   0x00010000L
#define MCI_DGV_FREEZE_OUTSIDE              0x00020000L

/* flags for dwFlags parameter of MCI_INFO command message */

#define MCI_DGV_INFO_TEXT                   0x00010000L 
#define MCI_DGV_INFO_ITEM                   0X00020000L

/* values for dwItem field of MCI_DGV_INFO_PARMS structure */

#define MCI_INFO_VERSION                    0x00000400L

#define MCI_DGV_INFO_USAGE                  0x00004000L
#define MCI_DGV_INFO_AUDIO_QUALITY          0x00004001L
#define MCI_DGV_INFO_STILL_QUALITY          0x00004002L
#define MCI_DGV_INFO_VIDEO_QUALITY          0x00004003L
#define MCI_DGV_INFO_AUDIO_ALG              0x00004004L
#define MCI_DGV_INFO_STILL_ALG              0x00004005L
#define MCI_DGV_INFO_VIDEO_ALG              0x00004006L

/* flags for dwFlags parameter of MCI_LIST command message */

#define MCI_DGV_LIST_ITEM                   0x00010000L
#define MCI_DGV_LIST_COUNT                  0x00020000L
#define MCI_DGV_LIST_NUMBER                 0x00040000L
#define MCI_DGV_LIST_ALG                    0x00080000L

/* values for dwItem field of MCI_DGV_LIST_PARMS structure */

#define MCI_DGV_LIST_AUDIO_ALG              0x00004000L
#define MCI_DGV_LIST_AUDIO_QUALITY          0x00004001L
#define MCI_DGV_LIST_AUDIO_STREAM           0x00004002L
#define MCI_DGV_LIST_STILL_ALG              0x00004003L
#define MCI_DGV_LIST_STILL_QUALITY          0x00004004L
#define MCI_DGV_LIST_VIDEO_ALG              0x00004005L
#define MCI_DGV_LIST_VIDEO_QUALITY          0x00004006L
#define MCI_DGV_LIST_VIDEO_STREAM           0x00004007L
#define MCI_DGV_LIST_VIDEO_SOURCE           0x00004008L


/* flags for dwFlags parameter of MCI_MONITOR command message */

#define MCI_DGV_MONITOR_METHOD              0x00010000L
#define MCI_DGV_MONITOR_SOURCE              0x00020000L

/* values for dwSource parameter of the MCI_DGV_MONITOR_PARMS struture */

#define MCI_DGV_MONITOR_INPUT               0x00004000L
#define MCI_DGV_MONITOR_FILE                0x00004001L

/* flags for dwFlags parameter of MCI_OPEN command message */

#define MCI_DGV_OPEN_WS                     0x00010000L 
#define MCI_DGV_OPEN_PARENT                 0x00020000L
#define MCI_DGV_OPEN_NOSTATIC               0x00040000L

/* flags for dwFlags parameter of MCI_PASTE command message */

#define MCI_DGV_PASTE_AT                    0x00010000L
#define MCI_DGV_PASTE_AUDIO_STREAM          0x00020000L
#define MCI_DGV_PASTE_VIDEO_STREAM          0x00040000L
#define MCI_DGV_PASTE_INSERT                0x00080000L
#define MCI_DGV_PASTE_OVERWRITE             0x00100000L

/* flags for dwFlags parameter of MCI_PLAY command message */

#define MCI_DGV_PLAY_REPEAT                 0x00010000L                
#define MCI_DGV_PLAY_REVERSE                0x00020000L 
 
/* flags for dwFlags parameter of MCI_PUT command message */

#define MCI_DGV_RECT                        0x00010000L 
#define MCI_DGV_PUT_SOURCE                  0x00020000L 
#define MCI_DGV_PUT_DESTINATION             0x00040000L 
#define MCI_DGV_PUT_FRAME                   0x00080000L
#define MCI_DGV_PUT_VIDEO                   0x00100000L
#define MCI_DGV_PUT_WINDOW                  0x00200000L
#define MCI_DGV_PUT_CLIENT                  0x00400000L

/* flags for dwFlags parameter of MCI_QUALITY command message */

#define MCI_QUALITY_ITEM                    0x00010000L
#define MCI_QUALITY_NAME                    0x00020000L
#define MCI_QUALITY_ALG                     0x00040000L
#define MCI_QUALITY_DIALOG                  0x00080000L
#define MCI_QUALITY_HANDLE                  0x00100000L

/* values for dwItem field of MCI_QUALITY_PARMS structure */

#define MCI_QUALITY_ITEM_AUDIO              0x00004000L
#define MCI_QUALITY_ITEM_STILL              0x00004001L
#define MCI_QUALITY_ITEM_VIDEO              0x00004002L

/* flags for dwFlags parameter of MCI_REALIZE command message */

#define MCI_DGV_REALIZE_NORM                0x00010000L
#define MCI_DGV_REALIZE_BKGD                0x00020000L

/* flags for dwFlags parameter of MCI_RECORD command message */

#define MCI_DGV_RECORD_HOLD                 0x00020000L
#define MCI_DGV_RECORD_AUDIO_STREAM         0x00040000L
#define MCI_DGV_RECORD_VIDEO_STREAM         0x00080000L

/* flags for dwFlags parameters of MCI_RESERVE command message */

#define MCI_DGV_RESERVE_IN                  0x00010000L
#define MCI_DGV_RESERVE_SIZE                0x00020000L

/* flags for dwFlags parameter of MCI_RESTORE command message */

#define MCI_DGV_RESTORE_FROM                0x00010000L
#define MCI_DGV_RESTORE_AT                  0x00020000L

/* flags for dwFlags parameters of MCI_SAVE command message */

#define MCI_DGV_SAVE_ABORT                  0x00020000L
#define MCI_DGV_SAVE_KEEPRESERVE            0x00040000L

/* flags for dwFlags parameters of MCI_SET command message */

#define MCI_DGV_SET_SEEK_EXACTLY            0x00010000L
#define MCI_DGV_SET_SPEED                   0x00020000L
#define MCI_DGV_SET_STILL                   0x00040000L
#define MCI_DGV_SET_FILEFORMAT              0x00080000L

/* flags for the dwFlags parameter of MCI_SETAUDIO command message */

#define MCI_DGV_SETAUDIO_OVER               0x00010000L
#define MCI_DGV_SETAUDIO_CLOCKTIME          0x00020000L
#define MCI_DGV_SETAUDIO_ALG                0x00040000L
#define MCI_DGV_SETAUDIO_QUALITY            0x00080000L
#define MCI_DGV_SETAUDIO_RECORD             0x00100000L
#define MCI_DGV_SETAUDIO_LEFT               0x00200000L
#define MCI_DGV_SETAUDIO_RIGHT              0x00400000L
#define MCI_DGV_SETAUDIO_ITEM               0x00800000L
#define MCI_DGV_SETAUDIO_VALUE              0x01000000L
#define MCI_DGV_SETAUDIO_INPUT              0x02000000L
#define MCI_DGV_SETAUDIO_OUTPUT             0x04000000L

/* values for the dwItem parameter of MCI_DGV_SETAUDIO_PARMS */

#define MCI_DGV_SETAUDIO_TREBLE             0x00004000L
#define MCI_DGV_SETAUDIO_BASS               0x00004001L
#define MCI_DGV_SETAUDIO_VOLUME             0x00004002L
#define MCI_DGV_SETAUDIO_STREAM             0x00004003L
#define MCI_DGV_SETAUDIO_SOURCE             0x00004004L
#define MCI_DGV_SETAUDIO_SAMPLESPERSEC      0x00004005L
#define MCI_DGV_SETAUDIO_AVGBYTESPERSEC     0x00004006L
#define MCI_DGV_SETAUDIO_BLOCKALIGN         0x00004007L
#define MCI_DGV_SETAUDIO_BITSPERSAMPLE      0x00004008L

/* values for the dwValue parameter of MCI_DGV_SETAUDIO_PARMS
   used with MCI_DGV_SETAUDIO_SOURCE */

#define MCI_DGV_SETAUDIO_SOURCE_STEREO      0x00000000L
#define MCI_DGV_SETAUDIO_SOURCE_LEFT        0x00000001L
#define MCI_DGV_SETAUDIO_SOURCE_RIGHT       0x00000002L
#define MCI_DGV_SETAUDIO_SOURCE_AVERAGE     0x00004000L

/* flags for the dwFlags parameter of MCI_SETVIDEO command */

#define MCI_DGV_SETVIDEO_QUALITY            0x00010000L
#define MCI_DGV_SETVIDEO_ALG                0x00020000L
#define MCI_DGV_SETVIDEO_CLOCKTIME          0x00040000L
#define MCI_DGV_SETVIDEO_SRC_NUMBER         0x00080000L
#define MCI_DGV_SETVIDEO_ITEM               0x00100000L
#define MCI_DGV_SETVIDEO_OVER               0x00200000L
#define MCI_DGV_SETVIDEO_RECORD             0x00400000L
#define MCI_DGV_SETVIDEO_STILL              0x00800000L
#define MCI_DGV_SETVIDEO_VALUE              0x01000000L
#define MCI_DGV_SETVIDEO_INPUT              0x02000000L
#define MCI_DGV_SETVIDEO_OUTPUT             0x04000000L

/* values for the dwTo field of MCI_SETVIDEO_PARMS
   used with MCI_DGV_SETVIDEO_SOURCE */

#define MCI_DGV_SETVIDEO_SRC_NTSC           0x00004000L
#define MCI_DGV_SETVIDEO_SRC_RGB            0x00004001L
#define MCI_DGV_SETVIDEO_SRC_SVIDEO         0x00004002L
#define MCI_DGV_SETVIDEO_SRC_PAL            0x00004003L
#define MCI_DGV_SETVIDEO_SRC_SECAM          0x00004004L
#define MCI_DGV_SETVIDEO_SRC_GENERIC        0x00004005L

/* values for the dwItem field of MCI_SETVIDEO_PARMS */

#define MCI_DGV_SETVIDEO_BRIGHTNESS         0x00004000L
#define MCI_DGV_SETVIDEO_COLOR              0x00004001L
#define MCI_DGV_SETVIDEO_CONTRAST           0x00004002L
#define MCI_DGV_SETVIDEO_TINT               0x00004003L
#define MCI_DGV_SETVIDEO_SHARPNESS          0x00004004L
#define MCI_DGV_SETVIDEO_GAMMA              0x00004005L
#define MCI_DGV_SETVIDEO_STREAM             0x00004006L
#define MCI_DGV_SETVIDEO_PALHANDLE          0x00004007L
#define MCI_DGV_SETVIDEO_FRAME_RATE         0x00004008L
#define MCI_DGV_SETVIDEO_SOURCE             0x00004009L
#define MCI_DGV_SETVIDEO_KEY_INDEX          0x0000400aL
#define MCI_DGV_SETVIDEO_KEY_COLOR          0x0000400bL
#define MCI_DGV_SETVIDEO_BITSPERPEL         0x0000400cL

/* flags for the dwFlags parameter of MCI_SIGNAL */

#define MCI_DGV_SIGNAL_AT                   0x00010000L
#define MCI_DGV_SIGNAL_EVERY                0x00020000L
#define MCI_DGV_SIGNAL_USERVAL              0x00040000L
#define MCI_DGV_SIGNAL_CANCEL               0x00080000L
#define MCI_DGV_SIGNAL_POSITION             0x00100000L

/* The following is the function digitalvideo drivers must use 
 * to signal when a frame marked by the SIGNAL command has been rendered:
 *
 *  SEND_DGVSIGNAL(dwFlags, dwCallback, hDriver, wDeviceID, dwUser, dwPos )
 *
 * The following is a description of the parameters:
 *
 *  dwFlags    - the dwFlags parameter passed when the signal was set
 *  dwCallback - the dwCallback value from the MCI_DGV_SIGNAL_PARMS struct
 *               used to set the signal
 *  hDriver    - the handle assigned to the driver by MMSYSTEM when the 
 *               device was opened
 *  wDeviceID  - the device ID
 *  dwUser     - the dwUserParm value from the MCI_DGV_SIGNAL_PARMS struct
 *               used to set the signal
 *  dwPos      - the position at which the signal was sent, in the current
 *               time format.
 *
 * The window indicated by the handle in the dwCallback field is notified 
 * by means of a Windows message with the following form:
 *
 * msg    = MM_MCISIGNAL
 * wParam = wDeviceID of the sending driver 
 * lParam = the uservalue specified or the position the signal was sent
 *          at; the latter if the MCI_DGV_SIGNAL_POSITION flag was set 
 *          in the dwFlags parameter when the signal was created.
 */

#define SEND_DGVSIGNAL(dwFlags, dwCallback, hDriver, wDeviceID, dwUser, dwPos ) \
  DriverCallback( (dwCallback), DCB_WINDOW, (HANDLE)(wDeviceID), MM_MCISIGNAL,\
  hDriver, ((dwFlags) & MCI_DGV_SIGNAL_POSITION) ? (dwPos):(dwUser),\
  ((dwFlags) & MCI_DGV_SIGNAL_POSITION) ? (dwUser):(dwPos))

/* flags for the dwFlags parameter of MCI_STATUS command */
                                            
#define MCI_DGV_STATUS_NOMINAL              0x00020000L
#define MCI_DGV_STATUS_REFERENCE            0x00040000L
#define MCI_DGV_STATUS_LEFT                 0x00080000L
#define MCI_DGV_STATUS_RIGHT                0x00100000L
#define MCI_DGV_STATUS_DISKSPACE            0x00200000L
#define MCI_DGV_STATUS_INPUT                0x00400000L
#define MCI_DGV_STATUS_OUTPUT               0x00800000L
#define MCI_DGV_STATUS_RECORD               0x01000000L

/* values for dwItem field of MCI_STATUS_PARMS structure */

#define MCI_DGV_STATUS_AUDIO_INPUT          0x00004000L
#define MCI_DGV_STATUS_HWND                 0x00004001L
#define MCI_DGV_STATUS_SPEED                0x00004003L
#define MCI_DGV_STATUS_HPAL                 0x00004004L
#define MCI_DGV_STATUS_BRIGHTNESS           0x00004005L
#define MCI_DGV_STATUS_COLOR                0x00004006L
#define MCI_DGV_STATUS_CONTRAST             0x00004007L
#define MCI_DGV_STATUS_FILEFORMAT           0x00004008L
#define MCI_DGV_STATUS_AUDIO_SOURCE         0x00004009L
#define MCI_DGV_STATUS_GAMMA                0x0000400aL
#define MCI_DGV_STATUS_MONITOR              0x0000400bL
#define MCI_DGV_STATUS_MONITOR_METHOD       0x0000400cL
#define MCI_DGV_STATUS_FRAME_RATE           0x0000400eL
#define MCI_DGV_STATUS_BASS                 0x0000400fL
#define MCI_DGV_STATUS_SIZE                 0x00004010L
#define MCI_DGV_STATUS_SEEK_EXACTLY         0x00004011L
#define MCI_DGV_STATUS_SHARPNESS            0x00004012L
#define MCI_DGV_STATUS_SMPTE                0x00004013L
#define MCI_DGV_STATUS_AUDIO                0x00004014L
#define MCI_DGV_STATUS_TINT                 0x00004015L
#define MCI_DGV_STATUS_TREBLE               0x00004016L
#define MCI_DGV_STATUS_UNSAVED              0x00004017L
#define MCI_DGV_STATUS_VIDEO                0x00004018L
#define MCI_DGV_STATUS_VOLUME               0x00004019L
#define MCI_DGV_STATUS_AUDIO_RECORD         0x0000401aL
#define MCI_DGV_STATUS_VIDEO_SOURCE         0x0000401bL
#define MCI_DGV_STATUS_VIDEO_RECORD         0x0000401cL
#define MCI_DGV_STATUS_STILL_FILEFORMAT     0x0000401dL
#define MCI_DGV_STATUS_VIDEO_SRC_NUM        0x0000401eL
#define MCI_DGV_STATUS_FILE_MODE            0x0000401fL
#define MCI_DGV_STATUS_FILE_COMPLETION      0x00004020L
#define MCI_DGV_STATUS_WINDOW_VISIBLE       0x00004021L
#define MCI_DGV_STATUS_WINDOW_MINIMIZED     0x00004022L
#define MCI_DGV_STATUS_WINDOW_MAXIMIZED     0x00004023L
#define MCI_DGV_STATUS_KEY_INDEX            0x00004024L
#define MCI_DGV_STATUS_KEY_COLOR            0x00004025L
#define MCI_DGV_STATUS_PAUSE_MODE           0x00004026L
#define MCI_DGV_STATUS_SAMPLESPERSEC        0x00004027L
#define MCI_DGV_STATUS_AVGBYTESPERSEC       0x00004028L
#define MCI_DGV_STATUS_BLOCKALIGN           0x00004029L
#define MCI_DGV_STATUS_BITSPERSAMPLE        0x0000402aL
#define MCI_DGV_STATUS_BITSPERPEL           0x0000402bL
#define MCI_DGV_STATUS_FORWARD              0x0000402cL
#define MCI_DGV_STATUS_AUDIO_STREAM         0x0000402dL
#define MCI_DGV_STATUS_VIDEO_STREAM         0x0000402eL

/* flags for dwFlags parameter of MCI_STEP command message */

#define MCI_DGV_STEP_REVERSE                0x00010000L 
#define MCI_DGV_STEP_FRAMES                 0x00020000L 

/* flags for dwFlags parameter of MCI_STOP command message */

#define MCI_DGV_STOP_HOLD                   0x00010000L

/* flags for dwFlags parameter of MCI_UPDATE command message */

#define MCI_DGV_UPDATE_HDC                  0x00020000L
#define MCI_DGV_UPDATE_PAINT                0x00040000L

/* flags for dwFlags parameter of MCI_WHERE command message */

#define MCI_DGV_WHERE_SOURCE                0x00020000L
#define MCI_DGV_WHERE_DESTINATION           0x00040000L
#define MCI_DGV_WHERE_FRAME                 0x00080000L
#define MCI_DGV_WHERE_VIDEO                 0x00100000L
#define MCI_DGV_WHERE_WINDOW                0x00200000L
#define MCI_DGV_WHERE_MAX                   0x00400000L

/* flags for dwFlags parameter of MCI_WINDOW command message */

#define MCI_DGV_WINDOW_HWND                 0x00010000L 
#define MCI_DGV_WINDOW_STATE                0x00040000L 
#define MCI_DGV_WINDOW_TEXT                 0x00080000L 

/* flags for hWnd parameter of MCI_DGV_WINDOW_PARMS parameter block */

#define MCI_DGV_WINDOW_DEFAULT              0x00000000L

/* parameter block for MCI_WHERE, MCI_PUT, MCI_FREEZE, MCI_UNFREEZE cmds */

typedef struct {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
} MCI_DGV_RECT_PARMS;
typedef MCI_DGV_RECT_PARMS FAR * LPMCI_DGV_RECT_PARMS;

/* parameter block for MCI_CAPTURE command message */

typedef struct {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrFileName;
#else
    LPSTR   lpstrFileName;
#endif
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
} MCI_DGV_CAPTURE_PARMS;
typedef MCI_DGV_CAPTURE_PARMS FAR * LPMCI_DGV_CAPTURE_PARMS;

/* parameter block for MCI_CLOSE command message */

typedef MCI_GENERIC_PARMS MCI_CLOSE_PARMS;
typedef MCI_CLOSE_PARMS FAR * LPMCI_CLOSE_PARMS;

/* parameter block for MCI_COPY command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;    
    DWORD   dwTo;  
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_COPY_PARMS;
typedef MCI_DGV_COPY_PARMS FAR * LPMCI_DGV_COPY_PARMS;

/* parameter block for MCI_CUE command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwTo;    
} MCI_DGV_CUE_PARMS;
typedef MCI_DGV_CUE_PARMS FAR * LPMCI_DGV_CUE_PARMS;

/* parameter block for MCI_CUT command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;  
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_CUT_PARMS;
typedef MCI_DGV_CUT_PARMS FAR * LPMCI_DGV_CUT_PARMS;

/* parameter block for MCI_DELETE command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_DELETE_PARMS;
typedef MCI_DGV_DELETE_PARMS FAR * LPMCI_DGV_DELETE_PARMS;

/* parameter block for MCI_FREEZE command message */

typedef MCI_DGV_RECT_PARMS MCI_DGV_FREEZE_PARMS;
typedef MCI_DGV_FREEZE_PARMS FAR * LPMCI_DGV_FREEZE_PARMS;

/* parameter block for MCI_INFO command message */

typedef struct  {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrReturn;
#else
    LPSTR   lpstrReturn;
#endif
    DWORD   dwRetSize;
    DWORD   dwItem;
} MCI_DGV_INFO_PARMS;
typedef MCI_INFO_PARMS FAR * LPMCI_INFO_PARMS;
typedef MCI_DGV_INFO_PARMS FAR * LPMCI_DGV_INFO_PARMS;

/* parameter block for MCI_LIST command message */

typedef struct {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrReturn;
#else
    LPSTR   lpstrReturn;
#endif
    DWORD   dwLength;
    DWORD   dwNumber;
    DWORD   dwItem;
#ifdef WIN32
    LPTSTR  lpstrAlgorithm;
#else
    LPSTR   lpstrAlgorithm;
#endif
} MCI_DGV_LIST_PARMS;
typedef MCI_DGV_LIST_PARMS FAR * LPMCI_DGV_LIST_PARMS;

/* parameter block for MCI_LOAD command message */

typedef MCI_LOAD_PARMS MCI_DGV_LOAD_PARMS;   
typedef MCI_DGV_LOAD_PARMS FAR * LPMCI_DGV_LOAD_PARMS;

/* parameter block for MCI_MONITOR command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwSource;
    DWORD   dwMethod;
} MCI_DGV_MONITOR_PARMS;
typedef MCI_DGV_MONITOR_PARMS FAR * LPMCI_DGV_MONITOR_PARMS;

/* parameter block for MCI_OPEN command message */

typedef struct {
    DWORD   dwCallback;
    WORD    wDeviceID;
    WORD    wReserved0;
#ifdef WIN32
    LPTSTR  lpstrDeviceType;
    LPTSTR  lpstrElementName;
    LPTSTR  lpstrAlias;
#else
    LPSTR   lpstrDeviceType;
    LPSTR   lpstrElementName;
    LPSTR   lpstrAlias;
#endif
    DWORD   dwStyle;
    HWND    hWndParent;
#ifndef WIN32
    WORD    wReserved1;
#endif
} MCI_DGV_OPEN_PARMS;
typedef MCI_DGV_OPEN_PARMS FAR * LPMCI_DGV_OPEN_PARMS;

/* parameter block for MCI_PAUSE command message */

typedef MCI_GENERIC_PARMS MCI_DGV_PAUSE_PARMS;
typedef MCI_DGV_PAUSE_PARMS FAR * LPMCI_DGV_PAUSE_PARMS;

/* parameter block for MCI_PASTE command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwTo;  
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_PASTE_PARMS;
typedef MCI_DGV_PASTE_PARMS FAR * LPMCI_DGV_PASTE_PARMS;

/* parameter block for MCI_PLAY command message */

typedef MCI_PLAY_PARMS MCI_DGV_PLAY_PARMS;
typedef MCI_DGV_PLAY_PARMS FAR * LPMCI_DGV_PLAY_PARMS;

/* parameter block for MCI_PUT command message */

typedef MCI_DGV_RECT_PARMS MCI_DGV_PUT_PARMS;
typedef MCI_DGV_PUT_PARMS FAR * LPMCI_DGV_PUT_PARMS;

/* parameter block for MCI_QUALITY command message */

typedef struct {
    DWORD       dwCallback;
    DWORD       dwItem;
#ifdef WIN32
    LPTSTR      lpstrName;
#else
    LPSTR       lpstrName;
#endif
    DWORD       lpstrAlgorithm;
    DWORD       dwHandle;
} MCI_DGV_QUALITY_PARMS;
typedef MCI_DGV_QUALITY_PARMS FAR * LPMCI_DGV_QUALITY_PARMS;

/* parameter block for MCI_REALIZE command message */

typedef MCI_GENERIC_PARMS MCI_REALIZE_PARMS;
typedef MCI_REALIZE_PARMS FAR * LPMCI_REALIZE_PARMS;

/* parameter block for MCI_RECORD command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_RECORD_PARMS;
typedef MCI_DGV_RECORD_PARMS FAR * LPMCI_DGV_RECORD_PARMS;

/* parameter block for MCI_RESERVE command message */

typedef struct {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrPath;
#else
    LPSTR   lpstrPath;
#endif
    DWORD   dwSize;
} MCI_DGV_RESERVE_PARMS;
typedef MCI_DGV_RESERVE_PARMS FAR * LPMCI_DGV_RESERVE_PARMS;

/* parameter block for MCI_RESTORE command message */

typedef struct {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrFileName;
#else
    LPSTR   lpstrFileName;
#endif
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
} MCI_DGV_RESTORE_PARMS;
typedef MCI_DGV_RESTORE_PARMS FAR * LPMCI_DGV_RESTORE_PARMS;

/* parameter block for MCI_RESUME command message */

typedef MCI_GENERIC_PARMS MCI_DGV_RESUME_PARMS;
typedef MCI_DGV_RESUME_PARMS FAR * LPMCI_DGV_RESUME_PARMS;

/* parameter block for MCI_SAVE command message */

typedef struct {
    DWORD   dwCallback;
#ifdef WIN32
    LPTSTR  lpstrFileName;
#else
    LPSTR   lpstrFileName;
#endif
    RECT    rc;
} MCI_DGV_SAVE_PARMS;
typedef MCI_DGV_SAVE_PARMS FAR * LPMCI_DGV_SAVE_PARMS;

/* parameter block for MCI_SET command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    DWORD   dwFileFormat;
    DWORD   dwSpeed;
} MCI_DGV_SET_PARMS;
typedef MCI_DGV_SET_PARMS FAR * LPMCI_DGV_SET_PARMS;

/* parameter block for MCI_SETAUDIO command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwItem;
    DWORD   dwValue;
    DWORD   dwOver;
#ifdef WIN32
    LPTSTR  lpstrAlgorithm;
    LPTSTR  lpstrQuality;
#else
    LPSTR   lpstrAlgorithm;
    LPSTR   lpstrQuality;
#endif
} MCI_DGV_SETAUDIO_PARMS;
typedef MCI_DGV_SETAUDIO_PARMS FAR * LPMCI_DGV_SETAUDIO_PARMS;

/* parameter block for MCI_SIGNAL command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwPosition;
    DWORD   dwPeriod;
    DWORD   dwUserParm;
} MCI_DGV_SIGNAL_PARMS;
typedef MCI_DGV_SIGNAL_PARMS FAR * LPMCI_DGV_SIGNAL_PARMS;

/* parameter block for MCI_SETVIDEO command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwItem;
    DWORD   dwValue;
    DWORD   dwOver;
#ifdef WIN32
    LPTSTR  lpstrAlgorithm;
    LPTSTR  lpstrQuality;
#else
    LPSTR   lpstrAlgorithm;
    LPSTR   lpstrQuality;
#endif
    DWORD   dwSourceNumber;
} MCI_DGV_SETVIDEO_PARMS;
typedef MCI_DGV_SETVIDEO_PARMS FAR * LPMCI_DGV_SETVIDEO_PARMS;

/* parameter block for MCI_STATUS command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
#ifdef WIN32
    LPTSTR  lpstrDrive;
#else
    LPSTR   lpstrDrive;
#endif
    DWORD   dwReference;
} MCI_DGV_STATUS_PARMS;
typedef MCI_DGV_STATUS_PARMS FAR * LPMCI_DGV_STATUS_PARMS;

/* parameter block for MCI_STEP command message */

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_DGV_STEP_PARMS;
typedef MCI_DGV_STEP_PARMS FAR *LPMCI_DGV_STEP_PARMS;

/* parameter block for MCI_STOP command message */

typedef MCI_GENERIC_PARMS MCI_DGV_STOP_PARMS;
typedef MCI_DGV_STOP_PARMS FAR * LPMCI_DGV_STOP_PARMS;

/* parameter block for MCI_UNFREEZE command message */

typedef MCI_DGV_RECT_PARMS MCI_DGV_UNFREEZE_PARMS;
typedef MCI_DGV_UNFREEZE_PARMS FAR * LPMCI_DGV_UNFREEZE_PARMS;

/* parameter block for MCI_UPDATE command message */

typedef struct {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else	
    RECT    rc;
#endif  
    HDC     hDC;
    WORD    wReserved0;
} MCI_DGV_UPDATE_PARMS;
typedef MCI_DGV_UPDATE_PARMS FAR * LPMCI_DGV_UPDATE_PARMS;

/* parameter block for MCI_WHERE command message */

typedef MCI_DGV_RECT_PARMS MCI_DGV_WHERE_PARMS;
typedef MCI_DGV_WHERE_PARMS FAR * LPMCI_DGV_WHERE_PARMS;

/* parameter block for MCI_WINDOW command message */

typedef struct {
    DWORD   dwCallback;
    HWND    hWnd;
#ifdef WIN32
    UINT    nCmdShow;
    LPTSTR  lpstrText;
#else
    WORD    wReserved1;
    UINT    nCmdShow;
    WORD    wReserved2;
    LPSTR   lpstrText;
#endif
} MCI_DGV_WINDOW_PARMS;
typedef MCI_DGV_WINDOW_PARMS FAR * LPMCI_DGV_WINDOW_PARMS;

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* !_INC_DIGITALV */
