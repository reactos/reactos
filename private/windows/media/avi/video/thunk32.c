//==========================================================================;
//  COMMENTS DO NOT YET APPLY TO MSVFW32.DLL
//  thunk32.c
//
//  Copyright (c) 1991-1994 Microsoft Corporation.  All Rights Reserved.
//
//  Description:
//      This module contains routines for thunking the
//      ICM APIs (messages) from 16-bit Windows to 32-bit WOW.
//
//  History:
//
//==========================================================================;

/*

    WOW Thunking design:

        Thunks are generated as follows :

        16-bit :
           acmBootDrivers->acmInitThunks :

               Generate calls to 32-bit drivers if we're in WOW call across
               to KERNEL to find thunking entry points.

               If we're thunking 'load' all the 32-bit ACM drivers as well as
               the 16-bit ones.

               Priority is always to find a 32-bit driver first but this is
               done via searching for one on open.

               The internal flag ACM_DRIVERADDF_32BIT is specified when
               calling IDriverAdd and this flag is stored in the ACMDRIVERID
               structure.

           IDriverAdd->IDriverLoad->IDriverLoad32

               The 16-bit side calls the 32-bit side passing in the driver
               alias which is used to compare against the aliases on the 32
               bit side and the 32-bit HACMDRIVERID is passed back for the
               relevant driver and stored in the hdrvr field of the
               ACMDRIVERID structure.

           IDriverOpen->IDriverOpen32

               The parameters are passed to the 32-bit side using the hdrvr
               field deduced from the HACMDRIVERID as the 32-bit HACMDRIVERID.

           IDriverMessageId->IDriverMessageId32 :

               If the driver is 32-bit (as identified in the ACMDRIVERID
               structure) then call IDriverMessageId32.  The hadid for
               the 32-bit driver is stored in the hdrvr field of ACMDRIVERID
               on the 16-bit side.

           IDriverMessage->IDriverMessage32

               If the driver is 32-bit (as identified in the ACMDRIVERID
               structure pointed to by the ACMDRIVER structure) then call
               IDriverMessage32.  The had for the 32-bit driver is stored
               in the hdrvr field of ACMDRIVER on the 16-bit side.

           Stream headers

               These must be persistent on the 32-bit side too and kept
               in synch.

               They are allocated on the 32-bit side for ACMDM_STREAM_PREPARE
               and freed on ACMDM_STREAM_UNPREPARE.  While in existence
               the 32-bit stream header is stored in the dwDriver field in

*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <win32.h>
#include <vfw.h>
#include <msviddrv.h>
#include <msvideoi.h>
#ifdef WIN32
    #include <wownt32.h>
    #include <stdlib.h>        // for mbstowcs and wcstombs
#endif // WIN32
#include "compmn16.h"

//
// pick up the function definitions
//

int thunkDebugLevel = 1;

#include "vidthunk.h"

/* -------------------------------------------------------------------------
** Handle and memory mapping functions.
** -------------------------------------------------------------------------
*/
LPWOWHANDLE32          lpWOWHandle32;
LPWOWHANDLE16          lpWOWHandle16;
LPWOWCALLBACK16        lpWOWCallback16;
LPGETVDMPOINTER        GetVdmPointer;
int                    ThunksInitialized;

//
//  16-bit structures
//

typedef struct {
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
} DRVCONFIGINFO16;

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
PVOID CopyTo32Bit(LPVOID Dest32, LPVOID Src16, DWORD Length)
{
    PVOID Src32;

    if (Src16 == NULL) {
        return NULL;
    }

    Src32 = GetVdmPointer((DWORD)Src16, Length, TRUE);

    CopyMemory(Dest32, Src32, Length);
    return(Src32);
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

#ifdef _INC_COMPMAN


/*--------------------------------------------------------------------------*\
|                                                                            |
| Now thunk the compman functions                                            |
|                                                                            |
|                                                                            |
|                                                                            |
|                                                                            |
\*--------------------------------------------------------------------------*/

/*
 *  Convert ICDRAWBEGIN structures
 */

INLINE STATICFN void ConvertICDRAWBEGIN(ICDRAWBEGIN *DrawBegin32,
                                        LPBITMAPINFOHEADER lpBmi,
                                        DWORD dw)
{
    ICDRAWBEGIN16 DrawBegin16;

    CopyTo32Bit(&DrawBegin16, (LPVOID)dw, sizeof(ICDRAWBEGIN16));

    DrawBegin32->dwFlags = DrawBegin16.dwFlags;
    DrawBegin32->hpal = ThunkHPAL(DrawBegin16.hpal);
    DrawBegin32->hwnd = ThunkHWND(DrawBegin16.hwnd);
    DrawBegin32->hdc = ThunkHDC(DrawBegin16.hdc);
    DrawBegin32->xDst = (int)DrawBegin16.xDst;
    DrawBegin32->yDst = (int)DrawBegin16.yDst;
    DrawBegin32->dxDst = (int)DrawBegin16.dxDst;
    DrawBegin32->dyDst = (int)DrawBegin16.dyDst;

    CopyTo32Bit(lpBmi, DrawBegin16.lpbi, sizeof(BITMAPINFOHEADER));

    DrawBegin32->lpbi = lpBmi;
    DrawBegin32->xSrc = (int)DrawBegin16.xSrc;
    DrawBegin32->ySrc = (int)DrawBegin16.ySrc;
    DrawBegin32->dxSrc = (int)DrawBegin16.dxSrc;
    DrawBegin32->dySrc = (int)DrawBegin16.dySrc;
    DrawBegin32->dwRate = DrawBegin16.dwRate;
    DrawBegin32->dwScale = DrawBegin16.dwScale;
}


/*
 *  Following logic copied from mvdm\wow32\wstruc.c - however since we
 *  don't have the usage parameter we're a bit stuck on the size of the
 *  entries.
 *
 *  See also the video for windows documentation - only a limited range of
 *  bitmap types are discussed.
 */

INT GetBMI16Size(UNALIGNED BITMAPINFOHEADER *pbmi16)
{
   int      nHdrSize;
   int      nEntSize;
   int      nEntries;
   int      nBitCount;
   DWORD    dwClrUsed;

   nHdrSize = (int)pbmi16->biSize;

  /*
   *  We don't have some of the info we need so assume RGBQUAD
   */

   nEntSize = sizeof(RGBQUAD);

   nBitCount = pbmi16->biBitCount;
   dwClrUsed = pbmi16->biClrUsed;

/* the following block of code should be this:
 *
 *  if ( nBitCount >= 9 ) { // this is how Win3.1 code says == 24
 *      nEntries = 1;
 *  }
 *  else if ( dwClrUsed ) {
 *      nEntries = dwClrUsed;
 *  }
 *  else {
 *      nEntries = 1 << nBitCount;
 *  }
 *
 *  but due to the fact that many apps don't initialize the biBitCount &
 *  biClrUsed fields (especially biClrUsed) we have to do the following
 *  sanity checking instead.  v-cjones
 */

   nEntries = 1;
   if ( nBitCount < 9 ) {
       if((nBitCount == 4) || (nBitCount == 8) || (nBitCount == 1)) {
           nEntries = 1 << nBitCount;
       }
       // sanity check for apps (lots) that don't init the dwClrUsed field
       if(dwClrUsed) {
           nEntries = (int)min((DWORD)nEntries, dwClrUsed);
       }
   }

   return ( nHdrSize + (nEntries * nEntSize) );
}

INLINE LPBITMAPINFO CopyBitmapInfo(DWORD Bi16)
{
    UNALIGNED BITMAPINFOHEADER *pbmi16;

    pbmi16 = WOW32ResolveMemory(Bi16);
    if ((int)pbmi16->biSize == 0) {
        pbmi16->biSize = sizeof(BITMAPINFOHEADER);
        DPF1(("WARNING: null bitmap info size, setting it correctly"));
    }

    return (LPBITMAPINFO)CopyAlloc((LPVOID)pbmi16, GetBMI16Size(pbmi16));
}

/*
 * Allocate a BITMAPINFO structure to contain 256 colours
 */
INLINE LPBITMAPINFO AllocBitmapInfo()
{
    return (PVOID)LocalAlloc(LMEM_FIXED, sizeof(BITMAPINFOHEADER)+
                                         (sizeof(RGBQUAD)*256));
}

INLINE LPBITMAPINFOHEADER CopyBitmapInfoHeader(DWORD Bi16)
{
    UNALIGNED BITMAPINFOHEADER *pbmi16;

    pbmi16 = WOW32ResolveMemory(Bi16);

    return (LPBITMAPINFOHEADER)CopyAlloc((LPVOID)pbmi16, pbmi16->biSize);
}



DWORD CopyICINFOTo16bit(DWORD dw, ICINFO *IcInfoCopy, DWORD Length)
{
    ICINFO16 IcInfo;
    LONG   ReturnCode;

   /*
    *  Make a copy since the behaviour of wcstombs is undefined
    *  for overlapping input and output
    */

    memcpy(&IcInfo, IcInfoCopy, FIELD_OFFSET(ICINFO, szName[0]));

   /*
    *  Massage the strings
    */

    wcstombs(IcInfo.szName,
             IcInfoCopy->szName,
             sizeof(IcInfo.szName));
    // HACK : overwrite the last five characters with "[32]\0"

    if ((IcInfo.szName[0]))
    {
        UINT n = min(sizeof(IcInfo.szName)-5, lstrlenA(IcInfo.szName));
        IcInfo.szName[n++] = '[';
        IcInfo.szName[n++] = '3';
        IcInfo.szName[n++] = '2';
        IcInfo.szName[n++] = ']';
        IcInfo.szName[n]   = '\0';
    }

    wcstombs(IcInfo.szDescription,
             IcInfoCopy->szDescription,
             sizeof(IcInfo.szDescription));
    // HACK : overwrite the last five characters with "[32]\0"
    if ((IcInfo.szDescription[0]))
    {
        UINT n = min(sizeof(IcInfo.szDescription)-5, lstrlenA(IcInfo.szDescription));
        IcInfo.szDescription[n++] = '[';
        IcInfo.szDescription[n++] = '3';
        IcInfo.szDescription[n++] = '2';
        IcInfo.szDescription[n++] = ']';
        IcInfo.szDescription[n]   = '\0';
    }

    wcstombs(IcInfo.szDriver,
             IcInfoCopy->szDriver,
             sizeof(IcInfo.szDriver));


    IcInfo.dwSize = sizeof(IcInfo);

    ReturnCode = min(Length, IcInfo.dwSize);

    CopyTo16Bit((LPVOID)dw, &IcInfo, ReturnCode);

    return ReturnCode;
}

STATICFN DWORD DoICM_DecompressX(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2)
{
   /*
    *  We need to convert the various fields in the ICDECOMPRESS/EX
    *  structure(s).  Fortunately(?) the EX structure is a simple
    *  extension.
    */
    typedef struct {
        //
        // same as ICM_DECOMPRESS
        //
        DWORD               dwFlags;

        LPBITMAPINFOHEADER  lpbiSrc;    // BITMAPINFO of compressed data
        LPVOID              lpSrc;      // compressed data

        LPBITMAPINFOHEADER  lpbiDst;    // DIB to decompress to
        LPVOID              lpDst;      // output data

        //
        // new for ICM_DECOMPRESSEX
        //
        short               xDst;       // destination rectangle
        short               yDst;
        short               dxDst;
        short               dyDst;

        short               xSrc;       // source rectangle
        short               ySrc;
        short               dxSrc;
        short               dySrc;

    } ICDECOMPRESSEX16;

    ICDECOMPRESSEX16 UNALIGNED     *lpicdmpr16;
    ICDECOMPRESSEX                  ICDecompressEx;
    DWORD                           l;

    /* Copy the standard or extended structure */
    lpicdmpr16 = GetVdmPointer( dwP1, dwP2, TRUE );
    ICDecompressEx.dwFlags = lpicdmpr16->dwFlags;


    ICDecompressEx.lpbiSrc = (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)lpicdmpr16->lpbiSrc);
    if (NULL == ICDecompressEx.lpbiSrc) {
        return (DWORD)ICERR_MEMORY;
    }


    ICDecompressEx.lpbiDst =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)lpicdmpr16->lpbiDst);
    if (NULL == ICDecompressEx.lpbiDst) {
        LocalFree( (HLOCAL)ICDecompressEx.lpbiSrc );
        return (DWORD)ICERR_MEMORY;
    }

    ICDecompressEx.lpSrc =
        GetVdmPointer((DWORD)lpicdmpr16->lpSrc,
                  ICDecompressEx.lpbiSrc->biSizeImage, TRUE);

    ICDecompressEx.lpDst =
        GetVdmPointer((DWORD)lpicdmpr16->lpDst,
                  ICDecompressEx.lpbiDst->biSizeImage, TRUE);

    if (dwP2 == sizeof(ICDECOMPRESSEX16) ) {

        ICDecompressEx.xDst     = (int)lpicdmpr16->xDst;
        ICDecompressEx.yDst     = (int)lpicdmpr16->yDst;
        ICDecompressEx.dxDst    = (int)lpicdmpr16->dxDst;
        ICDecompressEx.dyDst    = (int)lpicdmpr16->dyDst;

        ICDecompressEx.xSrc     = (int)lpicdmpr16->xSrc;
        ICDecompressEx.ySrc     = (int)lpicdmpr16->ySrc;
        ICDecompressEx.dxSrc    = (int)lpicdmpr16->dxSrc;
        ICDecompressEx.dySrc    = (int)lpicdmpr16->dySrc;
	dwP2 = sizeof(ICDecompressEx);  // Make the size relate to 32 bit
    }


    l = ICSendMessage( (HIC)hic, (UINT)msg, (DWORD)&ICDecompressEx, dwP2 );

    if ( l == ICERR_OK) {

        CopyTo16Bit( lpicdmpr16->lpbiDst, ICDecompressEx.lpbiDst,
                     sizeof(BITMAPINFOHEADER) );
    }


    LocalFree( (HLOCAL)ICDecompressEx.lpbiSrc );
    LocalFree( (HLOCAL)ICDecompressEx.lpbiDst );

    return l;
}



/*
 *  Generate our thunks - refer to msvideo!compman.c for definitions of
 *  functions.
 *
 *  NOTE - we often rely here on the fact that most of the message
 *  parameter structures are identical for 16 and 32-bit - ie they
 *  contain DWORDs and 32-bit pointers.
 */


BOOL FAR PASCAL ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO16 FAR * lpicInfo)
{
    ICINFO ReturnedICInfo;
    StartThunk(ICInfo);

    DPF2(("Calling ICInfo %4.4hs %4.4hs %8X", &fccType, &fccHandler, lpicInfo));

    ReturnedICInfo.fccHandler = 0;  // Initialise...
    ReturnCode = ICInfo(fccType, fccHandler, &ReturnedICInfo);

    CopyICINFOTo16bit((DWORD)lpicInfo, &ReturnedICInfo, sizeof(ReturnedICInfo));
    EndThunk()
}

LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2)
{
    StartThunk(ICSendMessage);
    DPF2(("Calling ICSendMessage %4X %4X %8X %8X",
              hic, msg, dwP1, dwP2));

    switch (msg) {
    case ICM_GETSTATE:
    case ICM_SETSTATE:

        if (dwP1 == 0) {
            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       dwP1,
                                       dwP2);
        } else {
            PVOID pState;
           /*
            *  Create some aligned memory to return or pass on the data
            */
            pState = (PVOID)LocalAlloc(LPTR, dwP2);

            if (pState == NULL) {
                ReturnCode = ICERR_MEMORY;
            } else {

                if ((UINT)msg == ICM_SETSTATE) {
                    // Copy the data from 16 bit land
                    CopyTo32Bit(pState, (LPVOID)dwP1, dwP2);
                }
                ReturnCode = ICSendMessage((HIC)(DWORD)hic, (UINT)msg,
                                           (DWORD)pState, dwP2);

               /*
                *  Copy back the state, if the driver returned any data
                */

                if (ReturnCode > 0 && (UINT)msg == ICM_GETSTATE) {
                    CopyTo16Bit((LPVOID)dwP1, pState,
                                min((DWORD)ReturnCode, dwP2));
                }

                LocalFree((HLOCAL)pState);
            }
        }
        break;

    case ICM_GETINFO:
        {
            ICINFO IcInfo;

            ReturnCode = ICGetInfo((HIC)(DWORD)hic, &IcInfo, sizeof(IcInfo));

            if (ReturnCode != 0) {
                ReturnCode = CopyICINFOTo16bit(dwP1, &IcInfo, ReturnCode);
            }
        }


        break;

    case ICM_CONFIGURE:
    case ICM_ABOUT:

       /*
        *  dwP1 = -1 is a special value asking if config is supported,
        *  otherwise it's a window handle
        */

        ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                   (UINT)msg,
                                   dwP1 == (DWORD)-1 ?
                                       (DWORD)-1 :
                                       (DWORD)ThunkHWND(LOWORD(dwP1)),
                                   dwP2);


        break;

    case ICM_COMPRESS:
        {

            if (dwP2 != sizeof(ICCOMPRESS)) {  // validation code
                ReturnCode = ICERR_BADSIZE;
            } else {

                DWORD dwFlags;
                LPDWORD lpdwFlags16, lpckid16;
                LPBITMAPINFOHEADER  lpbih16;

                DWORD ckid;

                ICCOMPRESS IcCompress;

                ReturnCode = ICERR_OK;

                /*
                 *  We need to convert the various fields in the ICCOMPRESS
                 *  structure
                 */

                CopyTo32Bit( &IcCompress, (LPVOID)dwP1, dwP2 );

                IcCompress.lpbiInput =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)IcCompress.lpbiInput);
                if (NULL == IcCompress.lpbiInput) {
                    ReturnCode = ICERR_MEMORY;
                    break;
                }

                lpbih16 = IcCompress.lpbiOutput;
                IcCompress.lpbiOutput =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)IcCompress.lpbiOutput);
                if (NULL == IcCompress.lpbiOutput) {
                    LocalFree((HLOCAL)IcCompress.lpbiInput);
                    ReturnCode = ICERR_MEMORY;
                    break;
                }

                IcCompress.lpInput =
                    GetVdmPointer((DWORD)IcCompress.lpInput,
                              IcCompress.lpbiInput->biSizeImage, TRUE);

                IcCompress.lpOutput =
                    GetVdmPointer((DWORD)IcCompress.lpOutput,
                              IcCompress.lpbiOutput->biSizeImage, TRUE);

                if (IcCompress.lpbiPrev) {

                    IcCompress.lpbiPrev =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)IcCompress.lpbiPrev);
                    if (NULL == IcCompress.lpbiPrev) {
                        LocalFree((HLOCAL)IcCompress.lpbiOutput);
                        LocalFree((HLOCAL)IcCompress.lpbiInput);
                        ReturnCode = ICERR_MEMORY;
                        break;
                    }

                    IcCompress.lpPrev =
                        GetVdmPointer((DWORD)IcCompress.lpPrev,
                                  IcCompress.lpbiPrev->biSizeImage, TRUE);
                }

                lpdwFlags16 = IcCompress.lpdwFlags;

                IcCompress.lpdwFlags = &dwFlags;

                if (IcCompress.lpckid != NULL) {
                    lpckid16 = IcCompress.lpckid;
                    IcCompress.lpckid = &ckid;
                }


                ReturnCode = ICSendMessage((HIC)(DWORD)hic, (UINT)msg,
                                           (DWORD)&IcCompress, dwP2);

                if (ReturnCode == ICERR_OK) {


                    CopyTo16Bit( lpbih16, IcCompress.lpbiOutput,
                                 sizeof(BITMAPINFOHEADER) );

                    if (lpdwFlags16 != NULL) {
                        CopyTo16Bit(lpdwFlags16, &dwFlags, sizeof(DWORD));
                    }

                    if (IcCompress.lpckid != NULL) {
                        CopyTo16Bit(lpckid16, &ckid, sizeof(DWORD));
                    }
                }



                /*
                ** Free the bitmap info storage regardless of the return code
                */

                if (NULL == IcCompress.lpbiPrev) {
                    LocalFree((HLOCAL)IcCompress.lpbiPrev);
                }

                LocalFree((HLOCAL)IcCompress.lpbiOutput);
                LocalFree((HLOCAL)IcCompress.lpbiInput);
            }

        }
        break;


    case ICM_DECOMPRESS_SET_PALETTE:
	// takes one (optionally null) bitmapinfo
	{
	    LPBITMAPINFO bmi = 0;

	    if (dwP1 != 0) {
		bmi = CopyBitmapInfo(dwP1);
	    }

	    ReturnCode = ICSendMessage((HIC)(DWORD)hic,
				       (UINT)msg,
				       (DWORD)bmi,
				       (DWORD)0);
            if (bmi != NULL) {
                LocalFree((HLOCAL)bmi);
            }

	    break;
	}



    case ICM_COMPRESS_GET_SIZE:
    case ICM_COMPRESS_BEGIN:
    case ICM_COMPRESS_QUERY:

    case ICM_DECOMPRESS_BEGIN:
    case ICM_DECOMPRESS_GET_PALETTE:
    case ICM_DECOMPRESS_QUERY:

        {
            LPBITMAPINFO bmi1, bmi2;

	    bmi1 = CopyBitmapInfo(dwP1);

            if (dwP2 != 0) {
                bmi2 = CopyBitmapInfo(dwP2);
            } else {
                bmi2 = NULL;
            }

            if (bmi1 == NULL || bmi2 == NULL && dwP2 != 0) {
                ReturnCode =
                    (UINT)msg == ICM_COMPRESS_GET_SIZE ? 0 : ICERR_MEMORY;
            } else {

                ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                           (UINT)msg,
                                           (DWORD)bmi1,
                                           (DWORD)bmi2);

                if (bmi2 != NULL) {

                    // We might have to return data to the 16 bit side.
                    // The messages for which we have to do this are:
                    //      ICM_DECOMPRESS_QUERY (iff retcode == ICERR_OK)
                    //      ICM_DECOMPRESS_GET_PALETTE (iff retcode >= 0)

                    if (((ReturnCode == ICERR_OK) && (msg == ICM_DECOMPRESS_QUERY))
                        || ((ReturnCode >= 0) && (msg == ICM_DECOMPRESS_GET_PALETTE)))
                    {
                        CopyTo16Bit((LPVOID)dwP2, bmi2, GetBMI16Size((LPBITMAPINFOHEADER)bmi2));
                    }
                    LocalFree((HLOCAL)bmi2);
                }

            }

            if (bmi1 != NULL) {
                LocalFree((HLOCAL)bmi1);
            }
        }
        break;

    case ICM_COMPRESS_END:

    case ICM_DECOMPRESS_END:
    case ICM_DECOMPRESSEX_END:
    case ICM_DRAW_END:
    case ICM_DRAW_FLUSH:
    case ICM_DRAW_START:         //??
    case ICM_DRAW_STOP:          //??
    case ICM_DRAW_SETTIME:
    case ICM_DRAW_RENDERBUFFER:
    case ICM_SETQUALITY:

        ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                   (UINT)msg,
                                   dwP1,
                                   dwP2);
        break;

    case ICM_DRAW_GETTIME:
    case ICM_GETBUFFERSWANTED:
    case ICM_GETDEFAULTQUALITY:
    case ICM_GETDEFAULTKEYFRAMERATE:
    case ICM_GETQUALITY:
        {
            DWORD dwReturn;

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&dwReturn,
                                       dwP2);

            // Note: although the definition of these messages state
            // that dwParam2 is not used, trouble will brew if the
            // decompressor ever tries to use dwParam2.  We cannot
            // thunk non-standard uses of this parameter.

            if (ReturnCode == ICERR_OK) {
                CopyTo16Bit((LPVOID)dwP1, &dwReturn, sizeof(DWORD));
            }

        }
        break;

    case ICM_COMPRESS_GET_FORMAT:
    case ICM_DECOMPRESS_GET_FORMAT:

       /*
        *  This is a tricky one - we first have to find the size of
        *  the output format so we can get a copy of the (aligned)
        *  version before passing it back to the app
        */

        {
            LPBITMAPINFO bmi1, bmi2;

            if ( dwP1 == 0L ) {
                ReturnCode = ICERR_OK;
                break;
            }

            bmi1 = CopyBitmapInfo(dwP1);

            if (bmi1 == NULL) {
                ReturnCode = ICERR_MEMORY;
            } else {

                ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                           (UINT)msg,
                                           (DWORD)bmi1,
                                           0);

                if (ReturnCode > 0 && dwP2 != 0) {
                    bmi2 = LocalAlloc(LMEM_FIXED, ReturnCode);

                    if (bmi2 == NULL) {
                       /*
                        *  Can't do anything!!! - there's not good return code
                        */

                        ReturnCode = ICERR_MEMORY;
                    } else {
                        DWORD Length;

                        Length = ReturnCode; /* preserve length */

                        ReturnCode =
                            ICSendMessage((HIC)(DWORD)hic,
                                          (UINT)msg,
                                          (DWORD)bmi1,
                                          (DWORD)bmi2);

                        if (ReturnCode >= 0) {
                            CopyTo16Bit((LPVOID)dwP2, bmi2, Length);
                        }

                        LocalFree((HLOCAL)bmi2);
                    }
                }

                LocalFree((HLOCAL)bmi1);
            }

        }
        break;

    case ICM_DECOMPRESS:
        if (dwP2 != sizeof(ICDECOMPRESS)) {
            ReturnCode = ICERR_BADSIZE;
        } else {
            ReturnCode = DoICM_DecompressX(hic, msg, dwP1, dwP2);
        }
        break;

    case ICM_DECOMPRESSEX:
    case ICM_DECOMPRESSEX_BEGIN:
        if (dwP2 != sizeof(ICDECOMPRESSEX)) {
            ReturnCode = ICERR_BADSIZE;
        } else {
            ReturnCode = DoICM_DecompressX(hic, msg, dwP1, dwP2);
        }
        break;

    case ICM_DECOMPRESSEX_QUERY:
            {
               /*
                *  We need to convert the various fields in the ICDECOMPRESSEX
                *  structure(s).
                */

                ICDECOMPRESSEX ICDecompressEx1;
                ICDECOMPRESSEX ICDecompressEx2;
                BITMAPINFOHEADER biInput1, biOutput1;
                BITMAPINFOHEADER biInput2, biOutput2;

                /* Copy the structure */
                CopyTo32Bit(&ICDecompressEx1,
                            (LPVOID)dwP1,
                            sizeof(ICDECOMPRESSEX));

               /*
                *  We now need : converted forms of the bitmap info headers
                *  an aligned version of the input bytes (CHECK and if
                *  already aligned do nothing)
                *  and an aligned (again only if ncessary) version of the
                *  output data buffer - actually we'll assume there aren't
                *  necessary
                */

                ICDecompressEx1.lpbiSrc =
                    CopyIfNotAligned(&biInput1, ICDecompressEx1.lpbiSrc,
                                     sizeof(BITMAPINFOHEADER));

                ICDecompressEx1.lpbiDst =
                    CopyIfNotAligned(&biOutput1, ICDecompressEx1.lpbiDst,
                                     sizeof(BITMAPINFOHEADER));

                ICDecompressEx1.lpSrc =
                    GetVdmPointer((DWORD)ICDecompressEx1.lpSrc,
                              ICDecompressEx1.lpbiSrc->biSizeImage, TRUE);

                ICDecompressEx1.lpDst =
                    GetVdmPointer((DWORD)ICDecompressEx1.lpDst,
                              ICDecompressEx1.lpbiDst->biSizeImage, TRUE);

                /* Copy the optional structure */
                if (dwP2) {
                    CopyTo32Bit(&ICDecompressEx2,
                                 (LPVOID)dwP2,
                                 sizeof(ICDECOMPRESSEX));
                    dwP2 = (DWORD)&ICDecompressEx2;

                    ICDecompressEx2.lpbiSrc =
                        CopyIfNotAligned(&biInput2, ICDecompressEx2.lpbiSrc,
                                         sizeof(BITMAPINFOHEADER));

                    ICDecompressEx2.lpbiDst =
                        CopyIfNotAligned(&biOutput2, ICDecompressEx2.lpbiDst,
                                         sizeof(BITMAPINFOHEADER));

                    ICDecompressEx2.lpSrc =
                        GetVdmPointer((DWORD)ICDecompressEx2.lpSrc,
                                  ICDecompressEx2.lpbiSrc->biSizeImage, TRUE);

                    ICDecompressEx2.lpDst =
                        GetVdmPointer((DWORD)ICDecompressEx2.lpDst,
                                  ICDecompressEx2.lpbiDst->biSizeImage, TRUE);
                }

                return ICSendMessage((HIC)hic,
                                     (UINT)msg,
                                     (DWORD)&ICDecompressEx1,
                                     (DWORD)dwP2);
            }

        break;


    case ICM_DRAW:

       /*
        *  We can't support unknown extensions
        */

        if (dwP2 != sizeof(ICDRAW)) {
            ReturnCode = ICERR_BADSIZE;
        } else {
            ICDRAW ICDraw;
            BITMAPINFOHEADER bmi;

            CopyTo32Bit(&ICDraw, (LPVOID)dwP1, dwP2);

           /*
            *  We have to assume this is a draw for video
            */

            CopyTo32Bit(&bmi, ICDraw.lpFormat, sizeof(BITMAPINFOHEADER));

            ICDraw.lpFormat = (LPVOID)&bmi;

            ICDraw.lpData = GetVdmPointer((DWORD)ICDraw.lpData, ICDraw.cbData, TRUE);

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&ICDraw,
                                       dwP2);
        }
        break;

    case ICM_DRAW_BEGIN:
        {
            ICDRAWBEGIN InputFormat;
            BITMAPINFOHEADER bmihInput;

            ConvertICDRAWBEGIN(&InputFormat, &bmihInput, dwP1);

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&InputFormat,
                                       dwP2);
        }
        break;

    case ICM_DRAW_CHANGEPALETTE:
    case ICM_DRAW_QUERY:
        {
            LPBITMAPINFO lpbi;

            lpbi = CopyBitmapInfo(dwP1);

            if (lpbi == NULL) {
                ReturnCode = ICERR_MEMORY;
            } else {
                ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                           (UINT)msg,
                                           (DWORD)lpbi,
                                           dwP2);

                LocalFree((HLOCAL)lpbi);
            }
        }
        break;

    case ICM_DRAW_REALIZE:

        ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                   (UINT)msg,
                                   (DWORD)ThunkHDC(LOWORD(dwP1)),
                                   dwP2);

        break;

    case ICM_DRAW_WINDOW:
        {
            RECT_SHORT SRect;
            RECT Rect;

            CopyTo32Bit(&SRect, (LPVOID)dwP1, sizeof(SRect));

            SHORT_RECT_TO_RECT(Rect, SRect);

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&Rect,
                                       dwP2);
        }
        break;

    // The next three messages are INTERNAL ones
    case ICM_GETERRORTEXT:
        break;

    case ICM_GETFORMATNAME:
        break;

    case ICM_ENUMFORMATS:
        break;

    case ICM_COMPRESS_FRAMES_INFO:
        {
            ICCOMPRESSFRAMES icf32;
            CopyTo32Bit(&icf32, (LPBYTE)dwP1, dwP2);

            // Now fix up the 32 bit structure
            icf32.PutData = icf32.GetData = NULL;  // For safety.  should not be used for this message

            if (icf32.lpbiOutput) {
                icf32.lpbiOutput = CopyBitmapInfoHeader((DWORD)icf32.lpbiOutput);
            }
            if (icf32.lpbiInput) {
                icf32.lpbiInput = CopyBitmapInfoHeader((DWORD)icf32.lpbiInput);
            }

            icf32.lInput = (LPARAM)
                GetVdmPointer((DWORD)icf32.lInput,
                          icf32.lpbiInput->biSizeImage, TRUE);

            icf32.lOutput = (LPARAM)
                GetVdmPointer((DWORD)icf32.lOutput,
                          icf32.lpbiOutput->biSizeImage, TRUE);

            // After the fixups have been done, call the actual routine
            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&icf32,
                                       dwP2);
            if (icf32.lpbiOutput) {
                LocalFree(icf32.lpbiOutput);
            }
            if (icf32.lpbiInput) {
                LocalFree(icf32.lpbiInput);
            }
        }
        break;

    case ICM_DRAW_GET_PALETTE:
        {
            ReturnCode = ICSendMessage((HIC) hic,
                                       (UINT)msg,
                                       dwP1,
                                       dwP2);
            if ((ReturnCode != 0L) && (ReturnCode != ICERR_UNSUPPORTED)) {

                FARPROC lpWOWHandle16;
                HMODULE hmodWow;

                if ( (hmodWow = GetModuleHandle( GET_MAPPING_MODULE_NAME ))
                   && (lpWOWHandle16 = GetProcAddress(hmodWow, "WOWHandle16"))) {
                    ReturnCode = (WORD)(lpWOWHandle16((HANDLE)ReturnCode, (WOW_HANDLE_TYPE)WOW_TYPE_HPALETTE));
                } else {
                    ReturnCode = ICERR_ERROR;
                }
            }
        }
        break;

    case ICM_DRAW_SUGGESTFORMAT:
        {
            ICDRAWSUGGEST icdrwsug;
            UNALIGNED ICDRAWSUGGEST * pic;
            UNALIGNED LPBITMAPINFOHEADER lpbmihdr16;

            // Remember the 16 bit address
            pic = (ICDRAWSUGGEST *)CopyTo32Bit(&icdrwsug, (LPBYTE)dwP1, dwP2);
            lpbmihdr16 = pic->lpbiSuggest;

            // Now fix up the 32 bit structure

            if (icdrwsug.lpbiIn) {
                icdrwsug.lpbiIn = CopyBitmapInfoHeader((DWORD)icdrwsug.lpbiIn);
            }

            if (icdrwsug.lpbiSuggest) {
                icdrwsug.lpbiSuggest = CopyBitmapInfoHeader((DWORD)icdrwsug.lpbiSuggest);
            }

            // After the fixups have been done, call the actual routine
            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD)&icdrwsug,
                                       dwP2);
            if (icdrwsug.lpbiIn) {
                LocalFree(icdrwsug.lpbiIn);
            }

            // We must return the 32 bit suggested format to 16 bit land
            if (icdrwsug.lpbiSuggest) {
                if (ReturnCode == ICERR_OK) {
                    CopyTo16Bit( pic->lpbiSuggest, icdrwsug.lpbiSuggest,
                                 lpbmihdr16->biSize);
                }
                LocalFree(icdrwsug.lpbiSuggest);
            }
        }
        break;

    case ICM_SET_STATUS_PROC:
        // We do not need to support this under NT.  It is much
        // easier not to add the callback support... even if we could
        // guarantee to be on the right thread to actually do the callback.

    default:
        ReturnCode = ICERR_UNSUPPORTED;
        break;
    }

EndThunk()
}

INLINE DWORD FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode)
{
    StartThunk(ICOpen);
    DPF2(("Calling ICOpen %4.4hs %4.4hs %4X", &fccType, &fccHandler, wMode));
    ReturnCode = (LONG)ICOpen(fccType, fccHandler, (UINT)wMode);
    EndThunk();
}

INLINE LRESULT FAR PASCAL ICClose32(DWORD hic)
{
    StartThunk(ICClose);
    DPF2(("Calling ICClose %4X", hic));
    ReturnCode = ICClose((HIC)hic);
    EndThunk();
}

#endif // _INC_COMPMAN


DWORD ICMThunk32(DWORD dwThunkId,DWORD dw1,DWORD dw2,DWORD dw3,DWORD dw4)
{
    //
    //  Make sure we've got thunking functionality
    //

#if 0
    {
        char    szBuffer[80];
        char    szMsg[32];


        switch (dwThunkId) {

        case compThunkICSendMessage32:
            lstrcpyA( szMsg, "ICSendMessage32" );
            break;

        case compThunkICInfo32:
            lstrcpyA( szMsg, "ICInfo32" );
            break;

        case compThunkICOpen32:
            lstrcpyA( szMsg, "ICOpen32" );
            break;

        case compThunkICClose32:
            lstrcpyA( szMsg, "ICClose32" );
            break;

        default:
            lstrcpyA( szMsg, "Unknown" );
            break;
        }

        wsprintfA( szBuffer, "%18.18s 0x%08X 0x%08X 0x%08X 0x%08X\r\n",
                   szMsg, dw1, dw2, dw3, dw4);

        OutputDebugStringA( szBuffer );

    }
#endif

    if (ThunksInitialized <= 0) {

        HMODULE hMod;

        if (ThunksInitialized == -1) {
            return MMSYSERR_ERROR;
        }

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

        case compThunkICSendMessage32:
            return ICSendMessage32(dw1, (UINT)dw2, dw3, dw4);
            break;

        case compThunkICInfo32:
            return ICInfo32(dw1, dw2, (ICINFOA FAR * )dw3);
            break;

        case compThunkICOpen32:
            return ICOpen32(dw1, dw2, (UINT)dw3);
            break;

        case compThunkICClose32:
            return ICClose32(dw1);
            break;

        default:
            return(0);
    }
}
