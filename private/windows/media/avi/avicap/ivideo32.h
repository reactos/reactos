/****************************************************************************/
/*                                                                          */
/*  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY   */
/*  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE     */
/*  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR   */
/*  PURPOSE.								    */
/*        MSVIDEO.H - Include file for Video APIs                           */
/*                                                                          */
/*        Note: You must include WINDOWS.H before including this file.      */
/*                                                                          */
/*        Copyright (c) 1990-1993, Microsoft Corp.  All rights reserved.    */
/*                                                                          */
/****************************************************************************/

#ifndef _INC_IVIDEO32
#define _INC_IVIDEO32   50      /* version number */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

#include <msvideo.h>

#define LOADDS
#define EXPORT

#if defined(_WIN32) && defined(UNICODE)
// unicode conversions

int Iwcstombs(LPSTR lpstr, LPCWSTR lpwstr, int len);
int Imbstowcs(LPWSTR lpwstr, LPCSTR lpstr, int len);

#endif

//#ifndef _RCINVOKED
///* video data types */
//DECLARE_HANDLE(HVIDEO);                 // generic handle
//typedef HVIDEO FAR * LPHVIDEO;
//#endif                                  // ifndef RCINVOKED

/****************************************************************************

                        video APIs

****************************************************************************/

#if defined _WIN32

#if defined DAYTONA
void videoInitHandleList(void);
void videoDeleteHandleList(void);
#endif

#if defined CHICAGO

  typedef struct _thk_videohdr {
      VIDEOHDR vh;
      LPBYTE   p32Buff;
      DWORD    p16Alloc;
      DWORD    dwMemHandle;
      DWORD    dwReserved;
  } THKVIDEOHDR, FAR *LPTHKVIDEOHDR;

  WORD FAR PASCAL _loadds capxGetDriverDescription (WORD wDriverIndex,
        LPSTR lpszName, WORD cbName,
        LPSTR lpszVer, WORD cbVer);

  DWORD WINAPI vidxAllocHeaders(
      HVIDEO          hVideo,
      UINT            nHeaders,
      LPTHKVIDEOHDR * lpHdrs);

  DWORD WINAPI vidxFreeHeaders(
      HVIDEO hv);

  DWORD WINAPI vidxAllocBuffer (
      HVIDEO          hv,
      UINT            iHdr,
      LPTHKVIDEOHDR * pp32Hdr,
      DWORD           dwSize);

  DWORD vidxFreeBuffer (
      HVIDEO hv,
      DWORD  p32Hdr);

  DWORD WINAPI vidxSetRect (
      HVIDEO hv,
      UINT wMsg,
      int left,
      int top,
      int right,
      int bottom);

  DWORD WINAPI vidxFrame (
      HVIDEO     hVideo,
      LPVIDEOHDR lpVHdr);

  #define videoSetRect(h,msg,rc) vidxSetRect (h, msg, rc.left, rc.top, rc.right, rc.bottom)

  DWORD WINAPI vidxAddBuffer (
      HVIDEO     hVideo,
      LPVIDEOHDR lpVHdr,
      DWORD      cbData);

  DWORD WINAPI vidxAllocPreviewBuffer (
      HVIDEO     hVideo,
      LPVOID     *lpBits,
      DWORD      cbData);

  DWORD WINAPI vidxFreePreviewBuffer (
      HVIDEO     hVideo,
      LPVOID     lpBits);

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

  DWORD WINAPI videoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
  DWORD WINAPI videoGetErrorText(HVIDEO hVideo, UINT wError,
              LPSTR lpText, UINT wSize);
  DWORD WINAPI videoStreamInit(HVIDEO hVideo,
              DWORD dwMicroSecPerFrame, DWORD dwCallback,
              DWORD dwCallbackInst, DWORD dwFlags);
  DWORD WINAPI videoStreamFini(HVIDEO hVideo);

  //DWORD WINAPI videoStreamPrepareHeader(HVIDEO hVideo,
  //            LPVIDEOHDR lpVHdr, DWORD dwSize);
  //DWORD WINAPI videoStreamAddBuffer(HVIDEO hVideo,
  //            LPVIDEOHDR lpVHdr, DWORD dwSize);
  DWORD WINAPI videoStreamReset(HVIDEO hVideo);
  DWORD WINAPI videoStreamStart(HVIDEO hVideo);
  DWORD WINAPI videoStreamStop(HVIDEO hVideo);
  DWORD WINAPI videoStreamUnprepareHeader(HVIDEO hVideo,
              LPVIDEOHDR lpVHdr, DWORD dwSize);


#else
 #define videoSetRect(h,msg,rc) videoMessage (h, msg, (DWORD)(LPVOID)&rc, VIDEO_CONFIGURE_SET)

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

DWORD WINAPI videoConfigureStorage (HVIDEO hVideo,
                      LPTSTR lpstrIdent, DWORD dwFlags);

DWORD WINAPI videoFrame(HVIDEO hVideo, LPVIDEOHDR lpVHdr);
DWORD WINAPI videoMessage(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2);

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

// Added for Win95 & NT PPC
//
DWORD WINAPI videoStreamAllocBuffer(HVIDEO hVideo,
              LPVOID FAR * plpBuffer, DWORD dwSize);
DWORD WINAPI videoStreamFreeBuffer(HVIDEO hVideo,
              LPVOID lpBuffer);
#endif // CHICAGO
#endif // _WIN32

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#endif  /* _INC_MSVIDEO */
