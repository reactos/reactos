//==========================================================================;
//  COMMENTS DO NOT YET APPLY TO MSVIDEO.DLL/MSVFW32.DLL
//  thunk.c
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
//==========================================================================;

#define _INC_COMPMAN

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmddk.h>
#include <mmreg.h>
#include <memory.h>
#include <win32.h>
#include <vfw.h>
#include <msviddrv.h>
#ifdef _WIN32
#ifdef DAYTONA
    #include <wownt32.h>
#endif
    #include <stdlib.h>        // for mbstowcs and wcstombs
#endif // _WIN32

#ifdef _WIN32
#include "compmn16.h"
#endif
#include "compmani.h"
#include "thunk.h"

#include "debug.h"



#ifdef NT_THUNK32

//==========================================================================;
//
//
//	--- === 32 BIT SIDE === ---
//
//
//==========================================================================;


/* -------------------------------------------------------------------------
** Handle and memory mapping functions.
** -------------------------------------------------------------------------
*/

LPWOWHANDLE32          lpWOWHandle32;
LPWOWHANDLE16          lpWOWHandle16;
LPGETVDMPOINTER        GetVDMPointer;
LPWOWCALLBACK16	       lpWOWCallback16;
int                    ThunksInitialized;

//
//  These wrap around whatever mapping mechanism is used on the platform
//  we are compiling for.
//
INLINE PVOID ptrFixMap16To32(const VOID * pv, DWORD cb);
INLINE VOID  ptrUnFix16(const VOID * pv);


#ifdef CHICAGO
//
//  -= Chicago implementation of memory mapping functions =-
//

//
//  Thunk helper routines in Chicago kernel
//
extern PVOID WINAPI MapSL(const VOID * pv);
extern PVOID WINAPI MapSLFix(const VOID * pv);
extern VOID  WINAPI UnMapSLFixArray(DWORD dwCnt, const VOID * lpSels[]);

PVOID INLINE ptrFixMap16To32(const VOID * pv, DWORD cb)
{
    return MapSLFix(pv);
}

VOID INLINE ptrUnFix16(const VOID * pv)
{
    UnMapSLFixArray(1, &pv);
}


#else	// CHICAGO ELSE

//
//  -= Daytona implementation of memory mapping functions =-
//
//  Use #define to avoid having a function call

#define ptrFixMap16To32(spv, cb)	\
	    GetVDMPointer( (DWORD) (DWORD_PTR) (spv), (cb), TRUE )

//PVOID ptrFixMap16To32(const VOID * pv, DWORD cb)
//{
//    return GetVDMPointer( (DWORD)pv, cb, TRUE );
//}

//
// The unfix routine is a total noop.
// We should really call WOWGetVDMPointerUnfix...
//

#define ptrUnFix16(spv)

//VOID ptrUnFix16(const VOID * pv)
//{
//    return;
//}

#endif	// !CHICAGO


//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;

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

    Dest32 = ptrFixMap16To32(Dest16, Length);

    CopyMemory(Dest32, Src32, Length);

    ptrUnFix16(Dest16);
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

    Src32 = ptrFixMap16To32(Src16, Length);

    CopyMemory(Dest32, Src32, Length);

    ptrUnFix16(Src16);
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
    if (DrawBegin16.dwFlags & ICDRAW_HDC) {
	DrawBegin32->hwnd = ThunkHWND(DrawBegin16.hwnd);
	DrawBegin32->hdc = ThunkHDC(DrawBegin16.hdc);
    }
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
 *  if ( nBitCount > 8 ) {  // true colour
 *      nEntries = 0; (ordinary case)
 *	nEntries = 3; (BI_BITFIELDS case)
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

   if ( nBitCount <= 8 ) {
       nEntries = 1 << nBitCount;
       // sanity check for apps (lots) that don't init the dwClrUsed field
       if(dwClrUsed) {
           nEntries = (int)min((DWORD)nEntries, dwClrUsed);
       }
   } else {
       if (pbmi16->biCompression == BI_BITFIELDS)
	   nEntries = 3;
       else
	   nEntries = 0;
   }

   return ( nHdrSize + (nEntries * nEntSize) );
}

INLINE LPBITMAPINFO CopyBitmapInfo(DWORD Bi16)
{
    UNALIGNED BITMAPINFOHEADER *pbmi16;
    LPBITMAPINFO    lpbmi;

    pbmi16 = ptrFixMap16To32((PVOID)Bi16, 0);

    if ((int)pbmi16->biSize == 0) {
        pbmi16->biSize = sizeof(BITMAPINFOHEADER);
	DPF(0, "WARNING: null bitmap info size, setting it correctly");
    }

    lpbmi = (LPBITMAPINFO)CopyAlloc((LPVOID)pbmi16, GetBMI16Size(pbmi16));

    ptrUnFix16((PVOID)Bi16);

    return (lpbmi);
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
    LPBITMAPINFOHEADER lpbmi;

    pbmi16 = ptrFixMap16To32((PVOID)Bi16, 0);

    lpbmi = (LPBITMAPINFOHEADER)CopyAlloc((LPVOID)pbmi16, pbmi16->biSize);

    ptrUnFix16((PVOID)Bi16);

    return (lpbmi);
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

STATICFN DWORD DoICM_DecompressX(DWORD hic, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2)
{
    ICDECOMPRESSEX16 UNALIGNED     *lpicdmpr16;
    ICDECOMPRESSEX                  ICDecompressEx;
    LRESULT                         l;
    BOOL			    fQuery = TRUE;

    /* Copy the standard or extended structure */
    lpicdmpr16 = ptrFixMap16To32( (PVOID)dwP1, (DWORD) dwP2 );
    ICDecompressEx.dwFlags = lpicdmpr16->dwFlags;


    ICDecompressEx.lpbiSrc = (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)(DWORD_PTR)lpicdmpr16->lpbiSrc);
    if (NULL == ICDecompressEx.lpbiSrc) {
	ptrUnFix16( (PVOID)dwP1 );
        return (DWORD)ICERR_MEMORY;
    }

    ICDecompressEx.lpbiDst =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)(DWORD_PTR)lpicdmpr16->lpbiDst);

    if ((NULL == ICDecompressEx.lpbiDst) && (msg != ICM_DECOMPRESSEX_QUERY)) {
        LocalFree( (HLOCAL)ICDecompressEx.lpbiSrc );
	ptrUnFix16( (PVOID)dwP1 );
        return (DWORD)ICERR_MEMORY;
    }

    if (msg == ICM_DECOMPRESSEX || msg == ICM_DECOMPRESS) {
	// map the source and destination pointers
	ICDecompressEx.lpSrc = ptrFixMap16To32(lpicdmpr16->lpSrc,
					       ICDecompressEx.lpbiSrc->biSizeImage);

	ICDecompressEx.lpDst = ptrFixMap16To32(lpicdmpr16->lpDst,
					       ICDecompressEx.lpbiDst->biSizeImage);
	fQuery = FALSE;	 // remember
    } else { // it is a query and we do not map the pointers
	ICDecompressEx.lpSrc = NULL;
	ICDecompressEx.lpDst = NULL;
    }

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


    l = ICSendMessage( (HIC)hic, (UINT)msg, (LPARAM)&ICDecompressEx, dwP2 );

    /* I don't think the following is needed // FrankYe 11/18/94
    // If we do, don't just uncomment this.  You gotta use biUnMapSL
    if ( l == ICERR_OK) {

        CopyTo16Bit( lpicdmpr16->lpbiDst, ICDecompressEx.lpbiDst,
                     sizeof(BITMAPINFOHEADER) );
    }
    */

    LocalFree( (HLOCAL)ICDecompressEx.lpbiSrc );
    if (ICDecompressEx.lpbiDst) {
	LocalFree( (HLOCAL)ICDecompressEx.lpbiDst );
    }
    if (!fQuery) {
	ptrUnFix16( lpicdmpr16->lpSrc );
	ptrUnFix16( lpicdmpr16->lpDst );
    }
    ptrUnFix16( (PVOID)dwP1 );

    return (DWORD) l;
}


/*
 *  Generate our thunks - refer to msvideo!compman.c for definitions of
 *  functions.
 *
 *  NOTE - we often rely here on the fact that most of the message
 *  parameter structures are identical for 16 and 32-bit - ie they
 *  contain DWORDs and 32-bit pointers.
 */

//--------------------------------------------------------------------------;
//
//  LONG thkStatusProc32
//
//	When a client calls the 16-bit ICSetStatusProc while using a
//	32-bit codec, this function is set as the StatusProc in 32-bit codec.
//	This function will then thunk down to 16-bits and call the
//	client's StatusProc.
//
//  Arguments:
//	LPARAM lParam : contains linear ptr to an ICSTATUSTHUNKDESC.  The
//	    ICSTATUSTHUNKDESC is created during the call to ICSetStatusProc.
//
//	UINT uMsg :
//
//	LONG l :
//
//  Return value:
//	LONG :
//
//--------------------------------------------------------------------------;

LONG CALLBACK thkStatusProc32(LPARAM lParam, UINT uMsg, LONG l)
{
    LPICSTATUSTHUNKDESC lpstd;
    LONG lr;

    DPFS(dbgThunks, 4, "thkStatusProc32()");

    lpstd = (LPICSTATUSTHUNKDESC)lParam;
    ASSERT( lpstd->fnStatusProcS );
    ASSERT( lpstd->pfnthkStatusProc16S );


    lr = 0;

    //
    //	TODO: Thunk error string for ICSTATUS_ERROR.  Currently I'm not
    //	    sure if ICSTATUS_ERROR is documented.
    //
//#pragma message(REMIND("thkStatusProc32: thunk ICSTATUS_ERROR"))
    if (ICSTATUS_ERROR == uMsg) {
	//
	//  Is l supposed to be an LPSTR to an error string???
	//
	l = (LONG)0;
    }

    //
    //
    //
    lpstd->uMsg = uMsg;
    lpstd->l    = l;

    lr = lpWOWCallback16(lpstd->pfnthkStatusProc16S, (DWORD)lpstd->lpstdS);

    return (lr);
}

//--------------------------------------------------------------------------;
//
//  LRESULT thkSetStatusProc
//
//	This function is called as a result of thunking up from a call to
//	the 16-bit ICSetStatusProc.  It will call the 32-bit ICSetStatusProc
//	to install thkStatusProc32.
//
//  Arguments:
//	HIC hic : handle to 32-bit codec
//
//	LPARAM lParam : contains linear ptr to an ICSTATUSTHUNKDESC.  The
//	    ICSTATUSTHUNKDESC is created in the call to the 16-bit
//	    ICSetStatusProc.  This will be set as the lParam to be passed
//	    to our thkStatusProc32.
//
//  Return value:
//	LRESULT :
//
//--------------------------------------------------------------------------;

LRESULT WINAPI thkSetStatusProc(HIC hic, LPICSTATUSTHUNKDESC lpstd)
{
    DPFS(dbgThunks, 3, "thkSetStatusProc()");

    return ICSetStatusProc(hic, 0L, (LPARAM)lpstd, thkStatusProc32);
}

//--------------------------------------------------------------------------;
//
//
//
//--------------------------------------------------------------------------;
LRESULT FAR PASCAL ICInfo32(DWORD fccType, DWORD fccHandler, ICINFO16 FAR * lpicInfo)
{
    ICINFO ReturnedICInfo;
    StartThunk(ICInfo);

    //DPF2(("Calling ICInfo %4.4hs %4.4hs %8X", &fccType, &fccHandler, lpicInfo));

    ReturnedICInfo.fccHandler = 0;  // Initialise...
    ReturnCode = ICInfo(fccType, fccHandler, &ReturnedICInfo);

    CopyICINFOTo16bit((DWORD)(DWORD_PTR)lpicInfo, &ReturnedICInfo, sizeof(ReturnedICInfo));
    EndThunk()
}

LRESULT FAR PASCAL ICInfoInternal32(DWORD fccType, DWORD fccHandler, ICINFO16 FAR * lpicInfo, ICINFOI FAR * lpicInfoI)
{
    return ICInfo32(fccType, fccHandler, lpicInfo);
#if 0
    ICINFO  ReturnedICInfo;
    ICINFOI ReturnedICInfoI;

    StartThunk(ICInfo);

    // DPF(1, "Calling ICInfo %4.4hs %4.4hs %8X", &fccType, &fccHandler, lpicInfo);

    ReturnedICInfo.fccHandler = 0;  // Initialise...
    ReturnCode = ICInfoInternal(fccType, fccHandler, &ReturnedICInfo, &ReturnedICInfoI);

    if (NULL != lpicInfoI) {
	//
	//  Assuming no members of ICINFOI need special thunking
	//
	CopyTo16Bit(lpicInfoI, &ReturnedICInfoI, sizeof(*lpicInfoI));
    }

    CopyICINFOTo16bit((DWORD)(DWORD_PTR)lpicInfo, &ReturnedICInfo, sizeof(ReturnedICInfo));

    EndThunk()
#endif
}

LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD_PTR dwP1, DWORD_PTR dwP2)
{
    StartThunk(ICSendMessage);
    DPF(2,"Calling ICSendMessage %4X %4X %8X %8X",
              hic, msg, dwP1, dwP2);

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
                ReturnCode = 0;
            } else {

                if ((UINT)msg == ICM_SETSTATE) {
                    // Copy the data from 16 bit land
                    CopyTo32Bit(pState, (LPVOID)dwP1, (DWORD) dwP2);
                }
                ReturnCode = ICSendMessage((HIC)(DWORD)hic, (UINT)msg,
                                           (DWORD_PTR)pState, dwP2);

               /*
                *  Copy back the state, if the driver returned any data
                */

                if (ReturnCode > 0 && (UINT)msg == ICM_GETSTATE) {
                    CopyTo16Bit((LPVOID)dwP1, pState,
                                min((DWORD)ReturnCode, (DWORD) dwP2));
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
                ReturnCode = CopyICINFOTo16bit((DWORD) (DWORD_PTR) dwP1, &IcInfo, (DWORD) ReturnCode);
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
                                   dwP1 == (DWORD_PTR)-1 ?
                                       (DWORD_PTR)-1 :
                                       (DWORD_PTR)ThunkHWND(LOWORD(dwP1)),
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

		LPVOID lpInput, lpOutput, lpPrev;

                DWORD ckid;

                ICCOMPRESS IcCompress;

                ReturnCode = ICERR_OK;

               /*
                *  We need to convert the various fields in the ICCOMPRESS
                *  structure
                */

                CopyTo32Bit( &IcCompress, (LPVOID)dwP1, (DWORD) dwP2 );

                IcCompress.lpbiInput =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)(DWORD_PTR)IcCompress.lpbiInput);
                if (NULL == IcCompress.lpbiInput) {
                    ReturnCode = ICERR_MEMORY;
                    break;
                }

                lpbih16 = IcCompress.lpbiOutput;
                IcCompress.lpbiOutput =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)(DWORD_PTR)IcCompress.lpbiOutput);
                if (NULL == IcCompress.lpbiOutput) {
                    LocalFree((HLOCAL)IcCompress.lpbiInput);
                    ReturnCode = ICERR_MEMORY;
                    break;
                }

		lpInput = IcCompress.lpInput;
                IcCompress.lpInput = ptrFixMap16To32(IcCompress.lpInput, IcCompress.lpbiInput->biSizeImage);

		lpOutput = IcCompress.lpOutput;
                IcCompress.lpOutput = ptrFixMap16To32(IcCompress.lpOutput, IcCompress.lpbiOutput->biSizeImage);

		lpPrev = NULL;
		if (IcCompress.lpbiPrev) {

		    IcCompress.lpbiPrev =  (LPBITMAPINFOHEADER)CopyBitmapInfo((DWORD)(DWORD_PTR)IcCompress.lpbiPrev);
                    if (NULL == IcCompress.lpbiPrev) {
                        LocalFree((HLOCAL)IcCompress.lpbiOutput);
                        LocalFree((HLOCAL)IcCompress.lpbiInput);
			ptrUnFix16(lpOutput);
			ptrUnFix16(lpInput);
                        ReturnCode = ICERR_MEMORY;
                        break;
                    }


		    lpPrev = IcCompress.lpPrev;
                    IcCompress.lpPrev = ptrFixMap16To32(IcCompress.lpPrev, IcCompress.lpbiPrev->biSizeImage);
                }

		lpdwFlags16 = IcCompress.lpdwFlags;
		if (lpdwFlags16 != NULL) {
		    CopyTo32Bit(&dwFlags, lpdwFlags16, sizeof(DWORD));
		    IcCompress.lpdwFlags = &dwFlags;
		}

                if (IcCompress.lpckid != NULL) {
                    lpckid16 = IcCompress.lpckid;
                    IcCompress.lpckid = &ckid;
                }


                ReturnCode = ICSendMessage((HIC)(DWORD)hic, (UINT)msg,
                                           (DWORD_PTR)&IcCompress, dwP2);

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

                if (NULL != IcCompress.lpbiPrev) {
                    LocalFree((HLOCAL)IcCompress.lpbiPrev);
                }

                LocalFree((HLOCAL)IcCompress.lpbiOutput);
                LocalFree((HLOCAL)IcCompress.lpbiInput);

		if (NULL != lpPrev)
		{
		    ptrUnFix16(lpPrev);
		}
		ptrUnFix16(lpOutput);
		ptrUnFix16(lpInput);
            }

        }
        break;


    case ICM_COMPRESS_GET_SIZE:
    case ICM_COMPRESS_BEGIN:
    case ICM_COMPRESS_QUERY:

    case ICM_DECOMPRESS_BEGIN:
    case ICM_DECOMPRESS_GET_PALETTE:
    case ICM_DECOMPRESS_SET_PALETTE:  // only takes one BitmapInfoHeader
    case ICM_DECOMPRESS_QUERY:

        {
            LPBITMAPINFO bmi1, bmi2;

	    bmi1 = bmi2 = NULL;
	    if (dwP1 != 0) {
		bmi1 = CopyBitmapInfo((DWORD) (DWORD_PTR) dwP1);
	    }
	    if (dwP2 != 0) {
		bmi2 = CopyBitmapInfo((DWORD) (DWORD_PTR) dwP2);
	    }

            if ( (NULL == bmi1  &&  0 != dwP1) || (NULL == bmi2  &&  0 != dwP2) )
	    {
		if (NULL != bmi1) LocalFree((HLOCAL)bmi1);
		if (NULL != bmi2) LocalFree((HLOCAL)bmi2);
                ReturnCode = (UINT)msg == ICM_COMPRESS_GET_SIZE ? 0 : ICERR_MEMORY;
		
            } else {

                ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                           (UINT)msg,
                                           (DWORD_PTR)bmi1,
                                           (DWORD_PTR)bmi2);

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
                                       (DWORD_PTR)&dwReturn,
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

            bmi1 = CopyBitmapInfo((DWORD) (DWORD_PTR) dwP1);

            if (bmi1 == NULL) {
                ReturnCode = ICERR_MEMORY;
            } else {

                ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                           (UINT)msg,
                                           (DWORD_PTR)bmi1,
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

                        Length = (DWORD) ReturnCode; /* preserve length */

                        ReturnCode =
                            ICSendMessage((HIC)(DWORD)hic,
                                          (UINT)msg,
                                          (DWORD_PTR)bmi1,
                                          (DWORD_PTR)bmi2);

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
    case ICM_DECOMPRESSEX_QUERY:
        if (dwP2 != sizeof(ICDECOMPRESSEX16)) {
            ReturnCode = ICERR_BADSIZE;
        } else {
            ReturnCode = DoICM_DecompressX(hic, msg, dwP1, dwP2);
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
	    LPVOID lpData;

            CopyTo32Bit(&ICDraw, (LPVOID)dwP1, (DWORD) dwP2);

           /*
            *  We have to assume this is a draw for video
            */

            CopyTo32Bit(&bmi, ICDraw.lpFormat, sizeof(BITMAPINFOHEADER));

            ICDraw.lpFormat = (LPVOID)&bmi;

	    lpData = ICDraw.lpData;
            ICDraw.lpData = ptrFixMap16To32(ICDraw.lpData, ICDraw.cbData);

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD_PTR)&ICDraw,
                                       dwP2);
	    ptrUnFix16(lpData);
        }
        break;

    case ICM_DRAW_BEGIN:
        {
            ICDRAWBEGIN InputFormat;
            BITMAPINFOHEADER bmihInput;

            ConvertICDRAWBEGIN(&InputFormat, &bmihInput, (DWORD) (DWORD_PTR) dwP1);

            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD_PTR)&InputFormat,
                                       dwP2);
        }
        break;

    case ICM_DRAW_CHANGEPALETTE:
    case ICM_DRAW_QUERY:
        {
            LPBITMAPINFO lpbi;

            lpbi = CopyBitmapInfo((DWORD) (DWORD_PTR) dwP1);

            if (lpbi == NULL) {
                ReturnCode = ICERR_MEMORY;
            } else {
                ReturnCode = ICSendMessage((HIC)(DWORD_PTR)hic,
                                           (UINT)msg,
                                           (DWORD_PTR)lpbi,
                                           dwP2);

                LocalFree((HLOCAL)lpbi);
            }
        }
        break;

    case ICM_DRAW_REALIZE:

        ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                   (UINT)msg,
                                   (DWORD_PTR)ThunkHDC(LOWORD(dwP1)),
                                   dwP2);

        break;

    case ICM_DRAW_WINDOW:
        {
            RECT_SHORT SRect;
            RECT Rect;

            CopyTo32Bit(&SRect, (LPVOID)dwP1, sizeof(SRect));

            SHORT_RECT_TO_RECT(Rect, SRect);

            ReturnCode = ICSendMessage((HIC)(DWORD_PTR)hic,
                                       (UINT)msg,
                                       (DWORD_PTR)&Rect,
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

	    // We might explode if we get too small a length and start treating
	    // some of the elements as pointers anyway.  Just fail the call.
	    // (WIN95C bug 8615).
	    // dwP2 is the length of the 16 bit structure and we only know
	    // the size of the 32 bit structure, but thankfully they're the
	    // same.
	    if (dwP2 < sizeof(ICCOMPRESSFRAMES)) {
		ReturnCode = ICERR_BADPARAM;
		break;
	    }

	    CopyTo32Bit(&icf32, (LPBYTE)dwP1, (DWORD) dwP2);

            // Now fix up the 32 bit structure
            icf32.PutData = icf32.GetData = NULL;  // For safety.  should not be used for this message

            if (icf32.lpbiOutput) {
                icf32.lpbiOutput = CopyBitmapInfoHeader((DWORD)(DWORD_PTR)icf32.lpbiOutput);
            }
            if (icf32.lpbiInput) {
                icf32.lpbiInput = CopyBitmapInfoHeader((DWORD)(DWORD_PTR)icf32.lpbiInput);
            }
	
// According to the documentation, lInput and lOutput are undefined.  Treating
// them as pointers is as scary as a really scary thing.
#if 0
	    lInput = icf32.lInput;
            icf32.lInput = (LPARAM)ptrFixMap16To32((LPVOID)icf32.lInput, icf32.lpbiInput->biSizeImage);

	    lOutput = icf32.lOutput;
            icf32.lOutput = (LPARAM)ptrFixMap16To32((LPVOID)icf32.lOutput, icf32.lpbiOutput->biSizeImage);
#endif
            // After the fixups have been done, call the actual routine
            ReturnCode = ICSendMessage((HIC)(DWORD_PTR)hic,
                                       (UINT)msg,
                                       (DWORD_PTR)&icf32,
                                       dwP2);

#if 0
	    ptrUnFix16((LPVOID)lOutput);
	    ptrUnFix16((LPVOID)lInput);
#endif
	
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
#ifdef CHICAGO
		ReturnCode = (LRESULT)(WORD)ReturnCode;
#else

                FARPROC lpWOWHandle16;
                HMODULE hmodWow;

                if ( (hmodWow = GetModuleHandle( GET_MAPPING_MODULE_NAME ))
                   && (lpWOWHandle16 = GetProcAddress(hmodWow, "WOWHandle16"))) {
                    ReturnCode = (WORD)(lpWOWHandle16((HANDLE)ReturnCode, (WOW_HANDLE_TYPE)WOW_TYPE_HPALETTE));
                } else {
                    ReturnCode = ICERR_ERROR;
                }
#endif
            }
        }
        break;

    case ICM_DRAW_SUGGESTFORMAT:
        {
            ICDRAWSUGGEST icdrwsug;
            UNALIGNED LPBITMAPINFOHEADER lpbiSuggest16;
            UNALIGNED LPBITMAPINFOHEADER lpbiSuggest;

	    DPF(4, "!ICMSendMessage32: ICM_DRAW_SUGGESTFORMAT: dwP1=%08lXh, dwP2=%08lXh", dwP1, dwP2);

            //
	    CopyTo32Bit(&icdrwsug, (LPBYTE)dwP1, (DWORD) dwP2);

	    lpbiSuggest = NULL;
	    lpbiSuggest16 = icdrwsug.lpbiSuggest;
	    if (lpbiSuggest16) {
		lpbiSuggest = ptrFixMap16To32(lpbiSuggest16, 0);
	    }

            // Now fix up the 32 bit structure

            if (icdrwsug.lpbiIn) {
                icdrwsug.lpbiIn = CopyBitmapInfoHeader((DWORD)(DWORD_PTR)icdrwsug.lpbiIn);
            }

            if (icdrwsug.lpbiSuggest) {
                icdrwsug.lpbiSuggest = CopyBitmapInfoHeader((DWORD)(DWORD_PTR)icdrwsug.lpbiSuggest);
            }

            // After the fixups have been done, call the actual routine
            ReturnCode = ICSendMessage((HIC)(DWORD)hic,
                                       (UINT)msg,
                                       (DWORD_PTR)&icdrwsug,
                                       dwP2);
            if (icdrwsug.lpbiIn) {
                LocalFree(icdrwsug.lpbiIn);
            }

            // We must return the 32 bit suggested format to 16 bit land
            if (icdrwsug.lpbiSuggest) {
                if (ReturnCode == ICERR_OK) {
		    CopyMemory( lpbiSuggest, icdrwsug.lpbiSuggest,
				lpbiSuggest->biSize);
                }
                LocalFree(icdrwsug.lpbiSuggest);
		ptrUnFix16(lpbiSuggest16);
            }
        }
        break;

    case ICM_SET_STATUS_PROC:
        // We do not need to support this under NT.  It is much
        // easier not to add the callback support... even if we could
        // guarantee to be on the right thread to actually do the callback.

	//
	// This message has its own thunk!
	//
	ASSERT( FALSE );


    default:
        ReturnCode = ICERR_UNSUPPORTED;
        break;
    }

EndThunk()
}

INLINE LRESULT FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode)
{
    StartThunk(ICOpen);

    DPF(1, "Calling ICOpen %4.4hs %4.4hs %4X", &fccType, &fccHandler, wMode);
    ReturnCode = (LONG_PTR)ICOpen(fccType, fccHandler, (UINT)wMode);
    EndThunk();
}

INLINE LRESULT FAR PASCAL ICClose32(DWORD hic)
{
    StartThunk(ICClose);
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

        case compThunkICInfoInternal32:
            lstrcpyA( szMsg, "ICInfoInternal32" );
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
            return (DWORD)ICERR_ERROR;
        }

        hMod = GetModuleHandle(GET_MAPPING_MODULE_NAME);
        if (hMod != NULL) {

            GetVDMPointer =
                (LPGETVDMPOINTER)GetProcAddress(hMod, GET_VDM_POINTER_NAME);
            lpWOWHandle32 =
                (LPWOWHANDLE32)GetProcAddress(hMod, GET_HANDLE_MAPPER32 );
            lpWOWHandle16 =
                (LPWOWHANDLE16)GetProcAddress(hMod, GET_HANDLE_MAPPER16 );
	    lpWOWCallback16 =
		(LPWOWCALLBACK16)GetProcAddress(hMod, GET_CALLBACK16);
        }

        if ( GetVDMPointer   == NULL ||
	     lpWOWHandle16   == NULL ||
	     lpWOWHandle32   == NULL ||
	     lpWOWCallback16 == NULL ) {

            ThunksInitialized = -1;
            return (DWORD)ICERR_ERROR;

        } else {
            ThunksInitialized = 1;
        }
    }

    //
    //  Perform the requested function
    //

    switch (dwThunkId) {

        case compThunkICSendMessage32:
            return (DWORD) ICSendMessage32(dw1, (UINT)dw2, dw3, dw4);
            break;

        case compThunkICInfoInternal32:
            return (DWORD) ICInfoInternal32(dw1, dw2, (ICINFOA FAR * )dw3, (ICINFOI FAR *)dw4);
            break;

        case compThunkICOpen32:
            return (DWORD) ICOpen32(dw1, dw2, (UINT)dw3);
            break;

        case compThunkICClose32:
            return (DWORD) ICClose32(dw1);
            break;

	case compThunkICOpenFunction32:
	{
	    DWORD fccType;
	    DWORD fccHandler;
	    UINT  uMode;
	    FARPROC lpfnHandler;

	    fccType	= dw1;
	    fccHandler	= dw2;
	    uMode	= (UINT)dw3;
	    lpfnHandler	= (FARPROC)dw4;

	    // !!! this won't work
	    return (DWORD) (DWORD_PTR) ICOpenFunction( fccType, fccHandler, uMode, lpfnHandler );
	    break;
	}

	case compThunkICSetStatusProc32:
	{
	    DWORD lpstdS;
	    DWORD cbStruct;
	    HIC   hic;
	    LPICSTATUSTHUNKDESC lpstd;
	    LRESULT lr;

	    hic      = (HIC)dw1;
	    lpstdS   = dw2;
	    cbStruct = dw3;
	
	    lpstd = ptrFixMap16To32((LPVOID)lpstdS, sizeof(*lpstd));

	    lr = thkSetStatusProc(hic, lpstd);

	    ptrUnFix16((LPVOID)lpstdS);

	    return (DWORD)lr;
	}

        default:
            return(0);
    }
}

#endif	// NT_THUNK32

#ifdef NT_THUNK16

//==========================================================================;
//
//
//	--- === 16 BIT SIDE === ---
//
//
//==========================================================================;

//
//  --== Global garbage ==--
//
DWORD           (FAR PASCAL *lpfnCallproc32W)(DWORD, DWORD, DWORD,
					      DWORD, DWORD,
					      LPVOID, DWORD, DWORD);
DWORD		(FAR PASCAL *lpfnFreeLibrary32W)(DWORD);
TCHAR		gszKernel[]             = TEXT("KERNEL");

TCHAR		gszLoadLibraryEx32W[]   = TEXT("LoadLibraryEx32W");
TCHAR		gszFreeLibrary32W[]     = TEXT("FreeLibrary32W");
TCHAR		gszGetProcAddress32W[]  = TEXT("GetProcAddress32W");
TCHAR		gszCallproc32W[]        = TEXT("CallProc32W");

TCHAR		gszThunkEntry[]         = TEXT("ICMThunk32");
TCHAR		gszMsvfw32[]            = TEXT("msvfw32.dll");

//
//  --==  ==--
//

//--------------------------------------------------------------------------;
//
//  PICMGARB thunkInitialize
//
//  Description:
//	Thunk initialization under NT WOW or Chicago 16-bit.
//
//  Arguments:
//
//  Return (PICMGARB):
//	NULL if thunks are not functional.  Otherwise, a pointer to the
//	ICMGARB structure for the current process is returned.
//
//  History:
//	07/07/94    frankye
//
//  Notes:
//	The lack of a pig for the current process implies that a thunk
//	is being called in the context of a process in which we were never
//	loaded.  If this happens, we allocate a new pig for this process
//	and continue the thunk initialization.  Since we were never
//	loaded in the process, we *probably* will never be freed.  We
//	cannot reliably determine when we can thunkTerminate in the process.
//
//--------------------------------------------------------------------------;
PICMGARB WINAPI thunkInitialize(VOID)
{
    HMODULE   hmodKernel;
    DWORD     (FAR PASCAL *lpfnLoadLibraryEx32W)(LPCSTR, DWORD, DWORD);
    LPVOID    (FAR PASCAL *lpfnGetProcAddress32W)(DWORD, LPCSTR);

    PICMGARB	pig;


    if (NULL == (pig = pigFind()))
    {
	DPF(0, "thunkInitialize: WARNING: ICM called from process %08lXh in which it was never loaded.", GetCurrentProcessId());
	DebugErr(DBF_WARNING, "thunkInitialize: WARNING: ICM called from process in which it was never loaded.");
	return NULL;
    }

    //
    //	If we've already tried and failed...
    //
    if (pig->fThunkError)
    {
	return NULL;
    }

    //
    //	If we already successfully initialized thunks...
    //
    if (pig->fThunksInitialized)
    {
	return pig;
    }

    //
    //	Try to initialize our connection the the 32-bit side...
    //
    DPFS(dbgInit, 1, "thunkInitialize()");

    //
    //	For now, we'll assume we hit an error...
    //
    pig->fThunkError = TRUE;

    //
    //  See if we can find the thunking routine entry points in KERNEL
    //
    hmodKernel = GetModuleHandle(gszKernel);

    if (hmodKernel == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetModuleHandle(kernel)");
        return NULL;
    }

    *(FARPROC *)&lpfnLoadLibraryEx32W =
        GetProcAddress(hmodKernel, gszLoadLibraryEx32W);

    if (lpfnLoadLibraryEx32W == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetProcAddress(kernel, LoadLibraryEx32W)");
        return NULL;
    }

    *(FARPROC *)&lpfnFreeLibrary32W =
        GetProcAddress(hmodKernel, gszFreeLibrary32W);

    if (lpfnFreeLibrary32W == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetProcAddress(kernel, FreeLibrary32W)");
        return NULL;
    }

    *(FARPROC *)&lpfnGetProcAddress32W = GetProcAddress(hmodKernel, gszGetProcAddress32W);

    if (lpfnGetProcAddress32W == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetProcAddress(kernel, GetProcAddress32W)");
        return NULL;
    }

    *(FARPROC *)&lpfnCallproc32W = GetProcAddress(hmodKernel, gszCallproc32W);

    if (lpfnCallproc32W == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetProcAddress(kernel, CallProc32W)");
        return NULL;
    }

    //
    //  See if we can get a pointer to our thunking entry points
    //
    pig->dwMsvfw32Handle = (*lpfnLoadLibraryEx32W)(gszMsvfw32, 0L, 0L);

    if (pig->dwMsvfw32Handle == 0)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't LoadLibraryEx32W(msvfw32.dll)");
        return NULL;
    }

    pig->lpvThunkEntry = (*lpfnGetProcAddress32W)(pig->dwMsvfw32Handle, gszThunkEntry);

    if (pig->lpvThunkEntry == NULL)
    {
	DPFS(dbgInit, 0, "thunkInitialize: Couldn't GetProcAddress32W(msvfw32, ICMThunk32)");
	(*lpfnFreeLibrary32W)(pig->dwMsvfw32Handle);
        return NULL;
    }

    pig->fThunkError = FALSE;
    pig->fThunksInitialized = TRUE;

    DPFS(dbgInit, 4, "thunkInitialize: Done!");

    return pig;
}

//--------------------------------------------------------------------------;
//
//  VOID thunkTerminate
//
//  Description:
//	Thunk termination under NT WOW or Chicago.  Called for each process
//	by 16-bit side to terminate thunks.
//
//  Arguments:
//	PICMGARB pig:
//
//  Return (VOID):
//
//  History:
//	07/07/94    frankye
//
//  Notes:
//
//--------------------------------------------------------------------------;
VOID WINAPI thunkTerminate(PICMGARB pig)
{
    ASSERT( NULL != pig );

    if (pig->fThunksInitialized)
    {
	lpfnFreeLibrary32W(pig->dwMsvfw32Handle);
	pig->fThunksInitialized = FALSE;
    }

    return;
}

//--------------------------------------------------------------------------;
//
//
//
//
//--------------------------------------------------------------------------;

/*
 *  Generate our thunks - refer to msvideo!compman.c for definitions of
 *  functions.
 *
 *  NOTE - we often rely here on the fact that most of the message
 *  parameter structures are identical for 16 and 32-bit - ie they
 *  contain DWORDs and 32-bit pointers.
 */

//--------------------------------------------------------------------------;
//
//  LONG thkStatusProc
//
//	This is reached by thunking down from the 32-bit thkStatusProc32.
//	It will call the client's StatusProc.
//
//  Arguments:
//	DWORD dwParam : contains segmented ptr to an ICSTATUSTHUNKDESC.
//	    The ICSTATUSTHUNKDESC is created during the call to ICSetStatusProc.
//
//  Return value:
//	LONG :
//
//--------------------------------------------------------------------------;

LONG FAR PASCAL _loadds thkStatusProc(DWORD dwParam)
{
    LPICSTATUSTHUNKDESC lpstd;

    DPFS(dbgThunks, 4, "thkStatusProc()");

    lpstd = (LPICSTATUSTHUNKDESC)dwParam;
    ASSERT( NULL != lpstd );
    ASSERT( NULL != lpstd->fnStatusProcS );

    return (lpstd->fnStatusProcS)(lpstd->lParam, (UINT)lpstd->uMsg, lpstd->l);
}

//--------------------------------------------------------------------------;
//
//  LRESULT ICSendSetStatusProc32
//
//  Called from 16-bit ICSendMessage to thunk the ICM_SET_STATUS_PROC
//  message to 32-bits
//
//  Arguments:
//	HIC hic :
//
//	LPICSETSTATUSPROC lpissp :
//
//	DWORD : cbStruct
//
//  Return value:
//	LRESULT :
//
//  Notes:
//	!!! For Daytona, we fail this.  If we ever decide to support this
//	on Daytona, then I think we have much more work to do since linear
//	addresses in use by the 32-bit codec may change when the status
//	callback thunks down to 16-bits.  On Chicago, all the linear addresses
//	our fixed when we convert from the segmented address to the linear
//	address.  WHY DOESN'T NT GIVE US AN EASY WAY TO FIX LINEAR ADDRESSES?
//
//--------------------------------------------------------------------------;

LRESULT WINAPI ICSendSetStatusProc32(HIC hic, ICSETSTATUSPROC FAR* lpissp, DWORD cbStruct)
{
#ifdef DAYTONA
    return ICERR_UNSUPPORTED;
#else
    PICMGARB		pig;
    PIC			pic;
    LPICSTATUSTHUNKDESC lpstd;
    LRESULT             lr;

    DPFS(dbgThunks, 4, "ICSendSetStatusProc32()");

    if (NULL == (pig = thunkInitialize())) {
	return ICERR_ERROR;
    }

    pic = (PIC)hic;

    //
    //	This should never fail since the structure was allocated by
    //	our own code in the ICSetStatusProc macro.
    //
    if (cbStruct < sizeof(*lpissp)) {
	ASSERT( FALSE );
	return ICERR_BADPARAM;
    }

    //
    //
    //
    if (NULL != pic->lpstd) {
	DPFS(dbgThunks, 1, "ICSendSetStatusProc32: replacing existing status proc!");
	GlobalUnfix(GlobalPtrHandle(pic->lpstd));
	GlobalFreePtr(pic->lpstd);
	pic->lpstd = NULL;
    }

    //
    //	Allocate a ICSTATUSTHUNKDESC status thunk descriptor.  We fix it
    //	in linear memory here so that it's linear address remains constant
    //	throughout it use.  It's linear address will be given to the 32-bit
    //	codec which will cache it for use in callbacks.
    //
    lpstd = (LPICSTATUSTHUNKDESC)GlobalAllocPtr(GHND, sizeof(*lpstd));
    if (NULL == lpstd) {
	return (ICERR_MEMORY);
    }
    GlobalFix(GlobalPtrHandle(lpstd));

    pic->lpstd = lpstd;

    lpstd->lpstdS		= lpstd;
    lpstd->dwFlags		= lpissp->dwFlags;
    lpstd->lParam		= lpissp->lParam;
    lpstd->fnStatusProcS	= lpissp->Status;
    lpstd->pfnthkStatusProc16S	= thkStatusProc;

    lr = (LRESULT)(*lpfnCallproc32W)(compThunkICSetStatusProc32,
				     (DWORD)pic->h32,
				     (DWORD)lpstd,
				     (DWORD)0,
				     (DWORD)0,
				     pig->lpvThunkEntry,
				     0L,    // Don't map pointers
				     5L);

    if (0 != lr) {
	DPFS(dbgThunks, 1, "ICSendSetStatusProc32: fail");
	GlobalUnfix(GlobalPtrHandle(pic->lpstd));
	GlobalFreePtr(pic->lpstd);
	pic->lpstd = NULL;
    }

    return lr;
#endif
}


BOOL FAR PASCAL ICInfoInternal32(DWORD fccType, DWORD fccHandler, ICINFO FAR * lpicInfo, ICINFOI FAR * lpicInfoI)
{
    PICMGARB	pig;

    DPFS(dbgThunks, 4, "ICInfoInternal32");

    if (NULL == (pig = thunkInitialize()))
    {
	return FALSE;
    }

    return (BOOL)(*lpfnCallproc32W)(compThunkICInfoInternal32,
				    (DWORD)fccType,
				    (DWORD)fccHandler,
				    (DWORD)lpicInfo,
				    (DWORD)lpicInfoI,
				    pig->lpvThunkEntry,
				    0L,    // Don't map pointers
				    5L);
}

LRESULT FAR PASCAL ICSendMessage32(DWORD hic, UINT msg, DWORD dwP1, DWORD dwP2)
{
    PICMGARB	pig;

    DPFS(dbgThunks, 4, "ICSendMessage32(hic=%08lXh, msg=%04Xh, dwP1=%08lXh, dwP2=%08lXh)", hic, msg, dwP1, dwP2);

    if (NULL == (pig = thunkInitialize()))
    {
	switch (msg) {
	case ICM_GETSTATE:
	case ICM_SETSTATE:
	case ICM_GETINFO:
	case ICM_COMPRESS_GET_SIZE:
	    return 0;
	}
	return (LRESULT)ICERR_ERROR;
    }

    return (LRESULT)(*lpfnCallproc32W)(compThunkICSendMessage32,
				       (DWORD)hic,
				       (DWORD)msg,
				       (DWORD)dwP1,
				       (DWORD)dwP2,
				       pig->lpvThunkEntry,
				       0L,    // Don't map pointers
				       5L);
}

LRESULT FAR PASCAL ICOpen32(DWORD fccType, DWORD fccHandler, UINT wMode)
{
    PICMGARB	pig;

    DPFS(dbgThunks, 4, "ICOpen32");

    if (NULL == (pig = thunkInitialize()))
    {
	return NULL;
    }

    return (DWORD)(lpfnCallproc32W)(compThunkICOpen32,
				    (DWORD)fccType,
				    (DWORD)fccHandler,
				    (DWORD)wMode,
				    (DWORD)0L,
				    pig->lpvThunkEntry,
				    0L,    // Don't map pointers
				    5L);
}

LRESULT FAR PASCAL ICOpenFunction32(DWORD fccType, DWORD fccHandler, UINT wMode, FARPROC lpfnHandler)
{
    PICMGARB	pig;

    DPFS(dbgThunks, 4, "ICOpenFunction32");

    if (NULL == (pig = thunkInitialize()))
    {
	return NULL;
    }

    return (LRESULT)(lpfnCallproc32W)(compThunkICOpenFunction32,
				    (DWORD)fccType,
				    (DWORD)fccHandler,
				    (DWORD)wMode,
				    (DWORD)lpfnHandler,
				    pig->lpvThunkEntry,
				    0L,    // Don't map pointers
				    5L);
}

LRESULT FAR PASCAL ICClose32(DWORD hic)
{
    PICMGARB	pig;

    DPFS(dbgThunks, 4, "ICClose32");

    if (NULL == (pig = thunkInitialize()))
    {
	return (LRESULT)ICERR_ERROR;
    }

    return (DWORD)(lpfnCallproc32W)(compThunkICClose32,
				    (DWORD)hic,
				    (DWORD)0L,
				    (DWORD)0L,
				    (DWORD)0L,
				    pig->lpvThunkEntry,
				    0L,    // Don't map pointers
				    5L);
}

#endif // NT_THUNK16

