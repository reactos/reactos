//==========================================================================;
//  thunk32.c
//
//  Copyright (c) 1991-1994 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//      This module contains routines for thunking the video APIs
//      from 16-bit Windows to 32-bit WOW.
//
//  History:
//
//==========================================================================;

/*

    WOW Thunking design:

        Thunks are generated as follows :

        16-bit :

*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <win32.h>
#ifdef _WIN32
#include <ivideo32.h>
#else
#include <msvideo.h>
#endif
#include <msviddrv.h>
#include <msvideoi.h>
#ifdef _WIN32
    #include <wownt32.h>
    #include <stdlib.h>        // for mbstowcs and wcstombs
    #include <video16.h>
#ifdef UNICODE
    #include "profile.h"       // NT only (for now?)
#endif
#endif // WIN32

// in capinit.c
BOOL capInternalGetDriverDescA(UINT wDriverIndex,
        LPSTR lpszName, int cbName,
        LPSTR lpszVer, int cbVer);

//
// pick up the function definitions
//

#include "vidthunk.h"

#ifdef DEBUG
#define MODNAME "AVICAP32"
int videoDebugLevel = -1;
void videoDebugInit(VOID)
{
    if (videoDebugLevel == -1)
        videoDebugLevel = GetProfileIntA("Debug", MODNAME, 0);
}
#else
    #define videoDebugInit()
#endif

/* -------------------------------------------------------------------------
** Handle and memory mapping functions.
** -------------------------------------------------------------------------
*/
LPWOWHANDLE32          lpWOWHandle32;
LPWOWHANDLE16          lpWOWHandle16;
LPWOWCALLBACK16        lpWOWCallback16;
LPGETVDMPOINTER        GetVdmPointer;
int                    ThunksInitialized;

#ifdef WIN32
#ifdef DEBUG
void FAR cdecl thkdprintf(LPSTR szFormat, ...)
{
    char ach[128];
    va_list va;

#define MARKER "AVICAP (thunk): "
    lstrcpyA(ach, MARKER);

    va_start(va, szFormat);
    wvsprintfA(ach+sizeof(MARKER), szFormat, va);
    va_end(va);
    OutputDebugStringA(ach);
}
#endif
#endif

//
//  Useful functions
//

//
//  CopyAlloc - allocate a new piece of memory, and copy the data in
//  Must use LocalFree to release the memory later
//
PVOID CopyAlloc(PVOID   pvSrc, UINT    uSize)
{
    PVOID   pvDest;

    pvDest = (PVOID)LocalAlloc(LMEM_FIXED, uSize);

    if (pvDest != NULL) {
        CopyMemory(pvDest, pvSrc, uSize);
    }

    return pvDest;
}

/*
 *  Copy data from source to dest where source is a 32bit pointer
 *  and dest is a 16bit pointer
 */
void CopyTo16Bit(LPVOID Dest16, LPVOID Src32, DWORD Length)
{
    PVOID Dest32;

    if (Src32 == NULL) {
        return;
    }

    Dest32 = GetVdmPointer((DWORD)Dest16, Length, TRUE);

    CopyMemory(Dest32, Src32, Length);
}


/*
 *  Copy data from source to dest where source is a 16bit pointer
 *  and dest is a 32bit pointer
 */
void CopyTo32Bit(LPVOID Dest32, LPVOID Src16, DWORD Length)
{
    PVOID Src32;

    if (Src16 == NULL) {
        return;
    }

    Src32 = GetVdmPointer((DWORD)Src16, Length, TRUE);

    CopyMemory(Dest32, Src32, Length);
}

/*
 *  Copy data from source to dest where source is a 16bit pointer
 *  and dest is a 32bit pointer ONLY if the source is not aligned
 *
 *  Returns which pointer to use (src or dest)
 */
LPVOID CopyIfNotAligned(LPVOID Dest32, LPVOID Src16, DWORD Length)
{
    PVOID Src32;

    if (Src16 == NULL) {
        return Dest32;
    }

    Src32 = GetVdmPointer((DWORD)Src16, Length, TRUE);

    CopyMemory(Dest32, Src32, Length);

    return Dest32;
}


typedef struct _callback {
    WORD flags;
    WORD hVideo16;
    WORD msg;
    DWORD dwCallback16inst;
    DWORD dw1;
    DWORD dw2;
}  CALLBACK16;
typedef CALLBACK16 * PCALLBACK16;

/*
 *  Callbacks
 */

void MyVideoCallback(HANDLE handle,
                     UINT msg,
                     DWORD dwUser,
                     DWORD dw1,
                     DWORD dw2)
{
    PVIDEOINSTANCEDATA32 pInst;
    BOOL fFree = FALSE;

    pInst = (PVIDEOINSTANCEDATA32)dwUser;

    DPF3(("Video callback - handle = %8X, msg = %8X, dwUser = %8X, dw1 = %8X, dw2 = %8X\n",
              handle, msg, dwUser, dw1, dw2));

    switch (msg) {

   /*
    *  What are the parameters for these messages ??
    */

    case MM_DRVM_OPEN:

       /*
        *  We get this when we INIT_STREAM
        */

        break;

    case MM_DRVM_CLOSE:

       /*
        *  Device is closing - this is where we free our structures
        *  (just in case the 32-bit side called close to clean up).
        *  dwUser points to our data
        */

        fFree = TRUE;

        break;

    case MM_DRVM_DATA:

       /*
        *  We have data - this means a buffer has been returned in
        *  dw1
        */

        {
            PVIDEOHDR32 pHdr32;

            pHdr32 = CONTAINING_RECORD((PVIDEOHDR)dw1,
                                       VIDEOHDR32,
                                       videoHdr);

            dw1 = (DWORD)pHdr32->pHdr16; // For callback below

           /*
            *  Map back the data and free our structure
            */

            {
                VIDEOHDR Hdr16;
                Hdr16 = pHdr32->videoHdr;
                Hdr16.lpData = pHdr32->lpData16;
                memcpy(pHdr32->pHdr32, (LPVOID)&Hdr16, sizeof(VIDEOHDR));
            }

           /*
            *  Clean up our local structure
            */

            LocalFree((HLOCAL)pHdr32);

        }

        break;

    case MM_DRVM_ERROR:
       /*
        *  dw1 = frames skipped - unfortunately there's nobody to tell!
        */

        break;
    }

   /*
    *  Call back the application if appropriate
    */

    switch (pInst->dwFlags & CALLBACK_TYPEMASK) {
        case CALLBACK_WINDOW:
            PostMessage(ThunkHWND(LOWORD(pInst->dwCallback)),
                    msg, (WPARAM)handle, (LPARAM)dw1);
            break;

        case CALLBACK_FUNCTION:
#if 0
            // Must call a generic 16 bit callback passing a pointer to
            // a parameter array.
            {

                WORD hMem;
                PCALLBACK16 pCallStruct;
                pCallStruct = WOWGlobalAllocLock16(0, sizeof(CALLBACK16), &hMem);
                if (pCallStruct) {
                    pCallStruct->flags = HIWORD(pInst->dwFlags);
                    pCallStruct->hVideo16 = (WORD)pInst->hVideo;
                    pCallStruct->msg = (WORD)msg;
                    pCallStruct->dwCallback16inst = pInst->dwCallbackInst;
                    pCallStruct->dw1 = (DWORD)dw1;
                    pCallStruct->dw2 = (DWORD)dw2;

                    lpWOWCallback16(pInst->dwCallback, pCallStruct);

                    // Now free off the callback structure
                    WOWGlobalUnlockFree16(pCallStruct);

                }
            }
#endif
            break;
    }

    if (fFree) {
        LocalFree((HLOCAL)pInst);
    }
}

//
//  Thunking callbacks to WOW32 (or wherever)
//

//--------------------------------------------------------------------------;
//
//  DWORD videoThunk32
//
//  Description:
//
//      32-bit function dispatcher for thunks.
//
//  Arguments:
//      DWORD dwThunkId:
//
//      DWORD dw1:
//
//      DWORD dw2:
//
//      DWORD dw3:
//
//      DWORD dw4:
//
//  Return (DWORD):
//
//  History:
//
//--------------------------------------------------------------------------;

DWORD videoThunk32(DWORD dwThunkId,DWORD dw1,DWORD dw2,DWORD dw3,DWORD dw4)
{
    //
    //  Make sure we've got thunking functionality
    //
    if (ThunksInitialized <= 0) {

        HMODULE hMod;

        if (ThunksInitialized == -1) {
            return MMSYSERR_ERROR;
        }

        videoDebugInit();

        hMod = GetModuleHandle(GET_MAPPING_MODULE_NAME);
        if (hMod != NULL) {

            GetVdmPointer =
                (LPGETVDMPOINTER)GetProcAddress(hMod, GET_VDM_POINTER_NAME);
            lpWOWHandle32 =
                (LPWOWHANDLE32)GetProcAddress(hMod, GET_HANDLE_MAPPER32 );
            lpWOWHandle16 =
                (LPWOWHANDLE16)GetProcAddress(hMod, GET_HANDLE_MAPPER16 );
            lpWOWCallback16 =
                (LPWOWCALLBACK16)GetProcAddress(hMod, GET_CALLBACK16 );
        }

        if ( GetVdmPointer == NULL
          || lpWOWHandle16 == NULL
          || lpWOWHandle32 == NULL ) {

            ThunksInitialized = -1;
            return MMSYSERR_ERROR;

        } else {
            ThunksInitialized = 1;
        }
    }


    //
    //  Perform the requested function
    //

    switch (dwThunkId) {

        case vidThunkvideoMessage32:
            return videoMessage32((HVIDEO)dw1, (UINT)dw2, dw3, dw4);
            break;

        case vidThunkvideoGetNumDevs32:
            return videoGetNumDevs32();
            break;

        case vidThunkvideoOpen32:
            return videoOpen32((LPHVIDEO)dw1, dw2, dw3);
            break;

        case vidThunkvideoClose32:
            return videoClose32((HVIDEO)dw1);
            break;

	case vidThunkvideoGetDriverDesc32:
	{
	    LPSTR lpszName = NULL, lpszVer = NULL;
	    short cbName, cbVer;
	    DWORD dwRet;

	    cbName = (short) LOWORD(dw4);
	    cbVer = (short) HIWORD(dw4);

	    // for chicago, need to call WOW32GetVdmPointerFix
	    // (via getprocaddr!)

	    if ((dw2 != 0) && (cbName > 0)) {
		lpszName = WOW32ResolveMemory(dw2);
	    }
	    if ((dw3 != 0) && (cbVer > 0)) {
		lpszVer = WOW32ResolveMemory(dw3);
	    }


	    dwRet = capInternalGetDriverDescA(
	    		dw1,   // device id
			lpszName,
			cbName,
			lpszVer,
			cbVer);

#if 0 //should do this for chicago
	    if (lpszName) {
		WOWGetVDMPointerUnfix(dw2);
	    }
	    if (lpszVer) {
		WOWGetVDMPointerUnfix(dw3);
	    }
#endif
	    return dwRet;
	}


        default:
            return(0);
    }
}


DWORD FAR PASCAL videoMessage32(HVIDEO hVideo, UINT msg, DWORD dwP1, DWORD dwP2)
{
    StartThunk(videoMessage);
    DPF2(("\tvideoMessage id = %4X, lParam1 = %8X, lParam2 = %8X",
              msg, dwP1, dwP2));

   /*
    *  We ONLY support (and we only ever will support) messages which
    *  have ALREADY been defined.  New 32-bit driver messages will NOT
    *  be supported from 16-bit apps.
    */

    switch (msg) {
    case DVM_GETVIDEOAPIVER:
        {
            DWORD ApiVer;

            ReturnCode = videoMessage((HVIDEO)hVideo,
                                      (UINT)msg,
                                      (DWORD)&ApiVer,
                                      dwP2);

            if (ReturnCode == DV_ERR_OK) {
                CopyTo16Bit((LPVOID)dwP1, &ApiVer, sizeof(DWORD));
            }
        }
        break;

    case DVM_GETERRORTEXT:
        {
            VIDEO_GETERRORTEXT_PARMS vet;
            VIDEO_GETERRORTEXT_PARMS MappedVet;

           /*
            *  Get the parameter block
            */

            CopyTo32Bit((LPVOID)&vet, (LPVOID)dwP1, sizeof(vet));
            MappedVet = vet;

           /*
            *  Map the string pointer
            */

            MappedVet.lpText = WOW32ResolveMemory(vet.lpText);

            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)&MappedVet,
                                      0);
        }
        break;

    case DVM_GET_CHANNEL_CAPS:
        {
            CHANNEL_CAPS Caps;

            ReturnCode = videoMessage((HVIDEO)hVideo,
                                      (UINT)msg,
                                      (DWORD)&Caps,
                                      dwP2);

           /*
            *  If successful return the data to the 16-bit app
            */

            if (ReturnCode == DV_ERR_OK) {
                 CopyTo16Bit((LPVOID)dwP1, (LPVOID)&Caps,
                             sizeof(Caps));
            }

        }
        break;

    case DVM_UPDATE:
        {
            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)ThunkHWND(dwP1),
                                      (DWORD)ThunkHDC(dwP2));
        }
        break;

    case DVM_PALETTE:
    case DVM_PALETTERGB555:
    case DVM_FORMAT:
       /*
        *  This stuff all comes from videoConfigure
        *
        *  Let's hope this data is all DWORDs!
        */
        {
            VIDEOCONFIGPARMS vcp, MappedVcp;
            DWORD dwReturn;

            BOOL Ok;

            Ok = TRUE;

            CopyTo32Bit((LPVOID)&vcp, (LPVOID)dwP2, sizeof(vcp));
            MappedVcp.lpdwReturn = &dwReturn;
            MappedVcp.dwSize1 = vcp.dwSize1;
            MappedVcp.dwSize2 = vcp.dwSize2;

           /*
            *  Get some storage to store the answer
            */

            if (MappedVcp.dwSize1 != 0) {
                MappedVcp.lpData1 = (LPSTR)LocalAlloc(LPTR, MappedVcp.dwSize1);
                if (MappedVcp.lpData1 == NULL) {
                    Ok = FALSE;
                } else {
                    if (MappedVcp.dwSize2 != 0) {
                        MappedVcp.lpData2 = (LPSTR)LocalAlloc(LPTR, MappedVcp.dwSize2);
                        if (MappedVcp.lpData2 == NULL) {
                            Ok = FALSE;

                            if (MappedVcp.dwSize1 != 0) {
                                LocalFree((HLOCAL)MappedVcp.lpData1);
                            }
                        }
                    }
                }
            }

            if (Ok) {

                CopyTo32Bit(MappedVcp.lpData1, vcp.lpData1, MappedVcp.dwSize1);
                CopyTo32Bit(MappedVcp.lpData2, vcp.lpData2, MappedVcp.dwSize2);

                ReturnCode = videoMessage(hVideo,
                                          msg,
                                          dwP1,
                                          (DWORD)&MappedVcp);

                if (ReturnCode == DV_ERR_OK) {

                    if (vcp.lpdwReturn != NULL) {
                        CopyTo16Bit(vcp.lpdwReturn, MappedVcp.lpdwReturn,
                                    sizeof(DWORD));
                    }

                    CopyTo16Bit(vcp.lpData1, MappedVcp.lpData1, MappedVcp.dwSize1);
                    CopyTo16Bit(vcp.lpData2, MappedVcp.lpData2, MappedVcp.dwSize2);
                }

                if (MappedVcp.dwSize1 != 0) {
                    LocalFree((HLOCAL)MappedVcp.lpData1);
                }
                if (MappedVcp.dwSize2 != 0) {
                    LocalFree((HLOCAL)MappedVcp.lpData2);
                }
            } else {
                ReturnCode = DV_ERR_NOMEM;
            }
        }
        break;

    case DVM_CONFIGURESTORAGE:
        {
            LPSTR lpStrIdent;
            lpStrIdent = WOW32ResolveMemory(dwP1);

            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)lpStrIdent,
                                      dwP2);

        }
        break;

    case DVM_DIALOG:
        {
            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)ThunkHWND(dwP1),
                                      dwP2);
        }
        break;

    case DVM_SRC_RECT:
    case DVM_DST_RECT:
       /*
        *  If it's a query only then don't bother with the
        *  rectangle
        */

        if (dwP2 & VIDEO_CONFIGURE_QUERY) {
            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      dwP1,
                                      dwP2);
        } else {

           /*
            *  The rectangle is regarded as 'in' and 'out'
            *  We need to translate between 16-bit and 32-bit rectangle structures
            */

            RECT_SHORT SRect;
            RECT Rect;

            CopyTo32Bit((LPVOID)&SRect, (LPVOID)dwP1, sizeof(SRect));

            SHORT_RECT_TO_RECT(Rect, SRect);

            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)&Rect,
                                      dwP2);

            if (ReturnCode == DV_ERR_OK) {
                RECT_TO_SHORT_RECT(SRect, Rect);
                CopyTo16Bit((LPVOID)dwP1, (LPVOID)&SRect, sizeof(SRect));
            }
        }
        break;

    case DVM_STREAM_PREPAREHEADER:
    case DVM_STREAM_UNPREPAREHEADER:
    case DVM_FRAME:
    case DVM_STREAM_ADDBUFFER:
        {
            VIDEOHDR Hdr32;
            LPBYTE pData16, pData32;
            DWORD dwSize;

            dwSize = (UINT)msg == DVM_FRAME ? sizeof(VIDEOHDR) :
                                    min(dwP2, sizeof(VIDEOHDR));

            CopyTo32Bit((LPVOID)&Hdr32, (LPVOID)dwP1, dwSize);

            pData16 = Hdr32.lpData;

           /*
            *  Create a mapping for the pointer
            */

            pData32 = GetVdmPointer((DWORD)pData16, Hdr32.dwBufferLength, TRUE);
            Hdr32.lpData = pData32;

            if (msg == DVM_STREAM_ADDBUFFER) {

                PVIDEOHDR32 pHdr32;

               /*
                *  Allocate our callback structure and pass this
                *  as our header (suitably offset to the video header part).
                */

                pHdr32 = (PVIDEOHDR32)LocalAlloc(LPTR, sizeof(VIDEOHDR32));

                if (pHdr32 == NULL) {
                    ReturnCode = DV_ERR_NOMEM;
                } else {

                   /*
                    *  Remember the old header so we can pass it back
                    *  and the old data pointer so we can flush it
                    */

                    pHdr32->pHdr16 = (LPVOID)dwP1;

                    /*
                     *  Some systems can't handle GetVdmPointer at interrupt
                     *  time so get a pointer here
                     */

                    pHdr32->pHdr32 = WOW32ResolveMemory(dwP1);
                    pHdr32->lpData16 = pData16;
                    pHdr32->videoHdr = Hdr32;

                    ReturnCode = videoMessage(hVideo,
                                              msg,
                                              (DWORD)&pHdr32->videoHdr,
                                              dwP2);
                   /*
                    *  If everything was OK copy it back
                    */

                    if (ReturnCode == DV_ERR_OK) {
                        Hdr32.lpData = pData16;
                        CopyTo16Bit((LPVOID)dwP1, (LPVOID)&Hdr32, dwSize);
                    }
                }

            } else {

               /*
                *  Prepare/unprepare the header for 32bit
                */

                ReturnCode = videoMessage(hVideo,
                                          msg,
                                          (DWORD)&Hdr32,
                                          dwP2);

               /*
                *  If everything was OK copy it back
                */

                if (ReturnCode == DV_ERR_OK) {
                    Hdr32.lpData = pData16;
                    CopyTo16Bit((LPVOID)dwP1, (LPVOID)&Hdr32, dwSize);
                }
            }
        }
        break;

    case DVM_STREAM_RESET:
    case DVM_STREAM_FINI:
    case DVM_STREAM_STOP:
    case DVM_STREAM_START:

       /*
        *  Note that the MM_DRVM_CLOSE message will cause us to clean up our
        *  callback structures on DVM_STREAM_FINI.
        */

        ReturnCode = videoMessage(hVideo,
                                  msg,
                                  0,
                                  0);
        break;

    case DVM_STREAM_GETPOSITION:
        {
            MMTIME mmTime;
            MMTIME16 mmTime16;

            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)&mmTime,
                                      sizeof(mmTime));

            if (ReturnCode == DV_ERR_OK) {
                mmTime16.wType = (WORD)mmTime.wType;
                CopyMemory((LPVOID)&mmTime16.u,
                           (LPVOID)&mmTime.u, sizeof(mmTime16.u));

                CopyTo16Bit((LPVOID)dwP1, (LPVOID)&mmTime16,
                            min(sizeof(mmTime16), dwP2));

            }
        }

        break;

    case DVM_STREAM_INIT:
        {
            VIDEO_STREAM_INIT_PARMS vsip;
            VIDEO_STREAM_INIT_PARMS * pvsip = WOW32ResolveMemory(dwP1);
            PVIDEOINSTANCEDATA32 pInst32;

#if 0
// always do callback
            if (!(pvsip->dwFlags & CALLBACK_TYPEMASK)) {
                // No callback wanted by the 16 bit code.  Pass call
                // straight through

                ReturnCode = videoMessage((HVIDEO)hVideo,
                                          (UINT)msg,
                                          (DWORD)pvsip,
                                          (DWORD)dwP2);

            } else
#endif
	    {
                // We set up a callback to a 32 bit routine, that in
                // turn will callback to the 16 bit function/window
                pInst32 = (PVIDEOINSTANCEDATA32)
                            LocalAlloc(LPTR, sizeof(VIDEOINSTANCEDATA32));

                if (pInst32 == NULL) {
                    ReturnCode = DV_ERR_NOMEM;
                } else {
                    CopyTo32Bit((LPVOID)&vsip, (LPVOID)dwP1,
                                min(sizeof(vsip), dwP2));

                    pInst32->dwFlags = vsip.dwFlags;
                    pInst32->dwCallbackInst = vsip.dwCallbackInst;
                    pInst32->dwCallback = vsip.dwCallback;
                    pInst32->hVideo = (HVIDEO16)vsip.hVideo;

                   /*
                    *  Make up our own parms.  Only set up a callback if
                    *  the user wanted one
                    */

                    vsip.dwCallback = (DWORD)MyVideoCallback;
                    vsip.dwFlags = (vsip.dwFlags & ~CALLBACK_TYPEMASK) |
                                   CALLBACK_FUNCTION;
                    vsip.dwCallbackInst = (DWORD)pInst32;

                    ReturnCode = videoMessage((HVIDEO)hVideo,
                                              (UINT)msg,
                                              (DWORD)&vsip,
                                              (DWORD)dwP2);

                    if (ReturnCode != DV_ERR_OK) {
                        LocalFree((HLOCAL)pInst32);
                    } else {
                        // The instance block will be freed off by the
                        // 32 bit callback routine when all over
                    }
                }
            }
        }
        break;

    case DVM_STREAM_GETERROR:
        {
            DWORD dwError;
            DWORD dwFramesSkipped;

            ReturnCode = videoMessage(hVideo,
                                      msg,
                                      (DWORD)&dwError,
                                      (DWORD)&dwFramesSkipped);

            if (ReturnCode == DV_ERR_OK) {
                CopyTo16Bit((LPVOID)dwP1, &dwError, sizeof(DWORD));
                CopyTo16Bit((LPVOID)dwP2, &dwFramesSkipped, sizeof(DWORD));
            }
        }
        break;

    default:
        DPF2(("videoMessage - Message not implemented %X\n", (UINT)msg));
        ReturnCode = DV_ERR_NOTSUPPORTED;

    }
    EndThunk();
}

INLINE DWORD FAR PASCAL videoGetNumDevs32(void)
{
    StartThunk(videoGetNumDevs);
    ReturnCode = videoGetNumDevs();
    EndThunk();
}

DWORD FAR PASCAL videoClose32(HVIDEO hVideo)
{
    StartThunk(videoClose)
    ReturnCode = videoClose(hVideo);
    EndThunk();
}

DWORD FAR PASCAL videoOpen32(LPHVIDEO lphVideo, DWORD dwDeviceID, DWORD dwFlags)
{
    HVIDEO  hVideo;
    StartThunk(videoOpen);

    ReturnCode = videoOpen(
                      &hVideo,
                      dwDeviceID,
                      dwFlags);

    if (ReturnCode == DV_ERR_OK) {
        lphVideo = WOW32ResolveMemory((PVOID)lphVideo);
        * (HVIDEO UNALIGNED *)lphVideo = hVideo;
    }
    EndThunk();
}


