/*--------------------------------------------------------------------------*\
|   RLECIF.C - Interface to RLE Comressor                                    |
|//@@BEGIN_MSINTERNAL									      |
|   History:                                                                 |
|   01/01/88 toddla     Created                                              |
|   10/30/90 davidmay   Reorganized, rewritten somewhat.                     |
|   07/11/91 dannymi    Un-hacked                                            |
|   09/15/91 ToddLa     Re-hacked                                            |
|   09/18/91 DavidMay	Separated from RLEC.C				     |
|   06/01/92 ToddLa     Moved into a installable compressor                  |
|//@@END_MSINTERNAL									      |
|                                                                            |
\*--------------------------------------------------------------------------*/
/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1991 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/

//@@BEGIN_MSINTERNAL									      |
#ifndef _WIN32
#include <win32.h>
#endif
//@@END_MSINTERNAL									      |
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#ifndef _INC_COMPDDK
#define _INC_COMPDDK    50      /* version number */
#endif

#include <vfw.h>
#include "msrle.h"
#include <stdarg.h>

//@@BEGIN_MSINTERNAL									      |
#ifdef UNICODE
#include "profile.h"   // map to registry for NT
#endif
//@@END_MSINTERNAL									      |

RLESTATE DefaultRleState = {0, 0, -1, 187, 1500, 4};

#define FOURCC_DIB      mmioFOURCC('D','I','B',' ')
#define FOURCC_RLE      mmioFOURCC('M','R','L','E') //mmioFOURCC('R','L','E',' ')
			
#define TWOCC_DIB       aviTWOCC('d','b')
#define TWOCC_RLE       aviTWOCC('d','c')
#define TWOCC_DIBX      aviTWOCC('d','x')

/****************************************************************************
****************************************************************************/

#pragma optimize("", off)

static BOOL NEAR PASCAL IsApp(LPTSTR szApp)
{
    TCHAR ach[128];
    int  i;
    HINSTANCE hInstance;

#ifdef _WIN32
    hInstance = GetModuleHandle(NULL);
#else
    _asm mov hInstance,ss
#endif

    GetModuleFileName(hInstance, ach, sizeof(ach));

    for (i = lstrlen(ach);
        i > 0 && ach[i-1] != '\\' && ach[i-1] != '/' && ach[i] != ':';
        i--)
        ;

    return lstrcmpi(ach + i, szApp) == 0;
}
#pragma optimize("", on)

/*****************************************************************************
 ****************************************************************************/
//
//  RleLoad()
//
void NEAR PASCAL RleLoad()
{
}

/*****************************************************************************
 ****************************************************************************/
//
//  RleFree()
//
void NEAR PASCAL RleFree()
{
    if (gRgbTol.hpTable)
        GlobalFreePtr(gRgbTol.hpTable);

    gRgbTol.hpTable = NULL;
}

/*****************************************************************************
 ****************************************************************************/

//
//  RleOpen()   - open a instance of the rle compressor
//
PRLEINST NEAR PASCAL RleOpen()
{
    PRLEINST pri;

    //
    //  VIDEDIT Hack
    //
    //  we dont want to see two "Microsoft RLE" compressors.
    //  so lie to VidEdit and fail to open.
    //

    if (GetModuleHandle(TEXT("MEDDIBS")) && IsApp(TEXT("VIDEDIT.EXE")))
        return NULL;

    pri = (PRLEINST)LocalAlloc(LPTR, sizeof(RLEINST));

    if (pri)
    {
        RleSetState(pri, NULL, 0);
    }
    return pri;
}

/*****************************************************************************
 ****************************************************************************/
//
//  RleClose()   - close a instance of the rle compressor
//
DWORD NEAR PASCAL RleClose(PRLEINST pri)
{
    if (!pri)
        return FALSE;

    if (pri->lpbiPrev) {
        GlobalFreePtr(pri->lpbiPrev);
	pri->lpbiPrev = NULL;
    }

    LocalFree((LOCALHANDLE)pri);
    return TRUE;
}

/*****************************************************************************
 ****************************************************************************/
//
//  RleGetState()   - get the current state of the rle compressor
//
//  will copy current state into passed buffer.
//  returns the size in bytes required to store the entire state.
//
DWORD NEAR PASCAL RleGetState(PRLEINST pri, LPVOID pv, DWORD dwSize)
{
    if (pv == NULL || dwSize == 0)
        return sizeof(RLESTATE);

    if (pri == NULL || dwSize < sizeof(RLESTATE))
        return 0;

    *(LPRLESTATE)pv = pri->RleState;
    return sizeof(RLESTATE);
}

/*****************************************************************************
 ****************************************************************************/
//
//  RleSetState()   - sets the current state of the rle compressor
//
DWORD NEAR PASCAL RleSetState(PRLEINST pri, LPVOID pv, DWORD dwSize)
{
    if (pv == NULL || dwSize == 0)
    {
        pv = &DefaultRleState;
        dwSize = sizeof(RLESTATE);
    }

    if (pri == NULL || dwSize < sizeof(RLESTATE))
        return 0;

    pri->RleState = *(LPRLESTATE)pv;
    return sizeof(RLESTATE);
}

#if !defined NUMELMS
 #define NUMELMS(aa) (sizeof(aa)/sizeof((aa)[0]))
#endif

#if defined _WIN32 && !defined UNICODE

int LoadUnicodeString(HINSTANCE hinst, UINT wID, LPWSTR lpBuffer, int cchBuffer)
{
    char    ach[128];
    int	    i;

    i = LoadString(hinst, wID, ach, sizeof(ach));

    if (i > 0)
	MultiByteToWideChar(CP_ACP, 0, ach, -1, lpBuffer, cchBuffer);

    return i;
}

#else
#define LoadUnicodeString   LoadString
#endif


/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleGetInfo(PRLEINST pri, ICINFO FAR *icinfo, DWORD dwSize)
{
    if (icinfo == NULL)
        return sizeof(ICINFO);

    if (dwSize < sizeof(ICINFO))
        return 0;

    icinfo->dwSize      = sizeof(ICINFO);
    icinfo->fccType     = ICTYPE_VIDEO;
    icinfo->fccHandler  = FOURCC_RLE;
    icinfo->dwFlags     = VIDCF_QUALITY   |  // supports quality
                          VIDCF_TEMPORAL  |  // supports inter-frame
                          VIDCF_CRUNCH;      // can crunch to a data rate
    icinfo->dwVersion   = ICVERSION;

    LoadUnicodeString(ghModule, IDS_DESCRIPTION, icinfo->szDescription, NUMELMS(icinfo->szDescription));
    LoadUnicodeString(ghModule, IDS_NAME, icinfo->szName, NUMELMS(icinfo->szName));

    return sizeof(ICINFO);
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompressQuery(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    //
    // determine if the input DIB data is in a format we like.
    //
    if (lpbiIn == NULL ||
        lpbiIn->biBitCount != 8 ||
        lpbiIn->biCompression != BI_RGB)
        return (DWORD)ICERR_BADFORMAT;

    //
    //  are we being asked to query just the input format?
    //
    if (lpbiOut == NULL)
        return ICERR_OK;

    //
    // make sure we can handle the format to compress to also.
    //
    if (lpbiOut->biCompression != BI_RLE8 ||        // must be rle format
        lpbiOut->biBitCount != 8 ||                 // must be 8bpp
        lpbiOut->biWidth  != lpbiIn->biWidth ||     // must be 1:1 (no stretch)
        lpbiOut->biHeight != lpbiIn->biHeight)
        return (DWORD)ICERR_BADFORMAT;

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompressGetFormat(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;

    if (dw = RleCompressQuery(pri, lpbiIn, NULL))
        return dw;

    dw = lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD);

    //
    // if lpbiOut == NULL then, return the size required to hold a output
    // format
    //
    if (lpbiOut == NULL)
        return dw;

    hmemcpy(lpbiOut, lpbiIn, dw);

    lpbiOut->biBitCount    = 8;
    lpbiOut->biCompression = BI_RLE8;
    lpbiOut->biSizeImage   = RleCompressGetSize(pri, lpbiIn, lpbiOut);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompressBegin(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;

    if (dw = RleCompressQuery(pri, lpbiIn, lpbiOut))
        return dw;

    if (pri->lpbiPrev) {
        GlobalFreePtr(pri->lpbiPrev);
	pri->lpbiPrev = NULL;
    }

    pri->iStart = 0;
    pri->lLastParm = 0L;

    pri->fCompressBegin = TRUE;

    MakeRgbTable(lpbiIn);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompressGetSize(PRLEINST pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    int dx,dy;

    //
    // we assume RLE data will never be twice the size of a full frame.
    //
    dx = (int)lpbiIn->biWidth;
    dy = (int)lpbiIn->biHeight;

    return (DWORD)(UINT)dx * (DWORD)(UINT)dy * 2;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompress(PRLEINST pri, ICCOMPRESS FAR *icinfo, DWORD dwSize)
{
    DWORD dw;
    BOOL  fFrameHalvingOccurred = FALSE;

    LPBITMAPINFOHEADER lpbi;

    if (!pri->fCompressBegin)
    {
        if (dw = RleCompressBegin(pri, icinfo->lpbiInput, icinfo->lpbiOutput))
            return dw;

        pri->fCompressBegin = FALSE;
    }

    //
    //  we can compress in one of two ways:
    //
    //      if a frame size is given (>0) then call CrunchDib using the passed
    //      quality as the "frame half" setting.
    //
    //      if a frame size is not given (==0) then use the passed quality
    //      as the tolerance and do a normal RleDeltaFrame()
    //

    if (icinfo->dwQuality == ICQUALITY_DEFAULT)
        icinfo->dwQuality = QUALITY_DEFAULT;

    if (icinfo->dwFrameSize > 0)
    {
        dw = ICQUALITY_HIGH - icinfo->dwQuality;

        pri->RleState.lMaxFrameSize = icinfo->dwFrameSize;
        pri->RleState.lMinFrameSize = icinfo->dwFrameSize - 500;
        pri->RleState.tolMax        = dw;
        pri->RleState.tolSpatial    = dw / 8;
        pri->RleState.tolTemporal   = ADAPTIVE;

// SplitDib makes really ugly artifacts by splitting the frame into who knows
// how many pieces which will be pieced together like a bad jigsaw puzzle where
// each piece is from a different picture.  I decided never to use this method
// of compression.
#if 0
        if (dw == 0)
        {
            pri->RleState.tolSpatial  = 0;
            pri->RleState.tolTemporal = 0;

            SplitDib(pri,
                icinfo->lpbiOutput, icinfo->lpOutput,
                icinfo->lpbiPrev,   icinfo->lpPrev,
                icinfo->lpbiInput,  icinfo->lpInput);
        }
        else
#endif
        {
            CrunchDib(pri,
                icinfo->lpbiOutput, icinfo->lpOutput,
                icinfo->lpbiPrev,   icinfo->lpPrev,
                icinfo->lpbiInput,  icinfo->lpInput);
        }

        lpbi = icinfo->lpbiOutput;

        if (lpbi->biCompression == BI_DIBX)
            fFrameHalvingOccurred = TRUE;

        if (icinfo->lpckid)
        {
            if (fFrameHalvingOccurred)
                *icinfo->lpckid = TWOCC_DIBX;
            else
                *icinfo->lpckid = TWOCC_RLE;
        }

        lpbi->biCompression = BI_RLE8;      // biSizeImage is filled in
    }
    else
    {
        dw = ICQUALITY_HIGH - icinfo->dwQuality;

        pri->RleState.tolSpatial    = dw;
        pri->RleState.tolTemporal   = dw / 8;

        RleDeltaFrame(
            icinfo->lpbiOutput, icinfo->lpOutput,
            icinfo->lpbiPrev,   icinfo->lpPrev,
            icinfo->lpbiInput,  icinfo->lpInput,
            0,-1,
	    pri->RleState.tolTemporal,
	    pri->RleState.tolSpatial,
	    pri->RleState.iMaxRunLen,4);

        if (icinfo->lpckid)
            *icinfo->lpckid = TWOCC_RLE;
    }

    //
    // set the AVI index flags,
    //
    //    make it a keyframe, if no previous frame
    //
    if (icinfo->lpdwFlags) {
        if (icinfo->lpbiPrev == NULL && !fFrameHalvingOccurred)
            *icinfo->lpdwFlags |= AVIIF_TWOCC | AVIIF_KEYFRAME;
        else
            *icinfo->lpdwFlags |= AVIIF_TWOCC;
    }

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleCompressEnd(PRLEINST pri)
{
    pri->fCompressBegin = FALSE;
    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleDecompressQuery(RLEINST * pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    //
    // determine if the input DIB data is in a format we like.
    // We like all RGB.  We like 8bit RLE.
    //
    if (lpbiIn == NULL ||
	  (lpbiIn->biBitCount != 8 && lpbiIn->biCompression == BI_RLE8) ||
          (lpbiIn->biCompression != BI_RGB && lpbiIn->biCompression != BI_RLE8))
	return (DWORD)ICERR_BADFORMAT;

    //
    //  are we being asked to query just the input format?
    //
    if (lpbiOut == NULL)
	return ICERR_OK;

    //
    // make sure we can handle the format to decompress too.
    //
    if (lpbiOut->biCompression != BI_RGB ||         // must be full dib
	lpbiOut->biBitCount != lpbiIn->biBitCount ||// must match
	lpbiOut->biWidth  != lpbiIn->biWidth ||     // must be 1:1 (no stretch)
	lpbiOut->biHeight != lpbiIn->biHeight)
	return (DWORD)ICERR_BADFORMAT;

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleDecompressGetFormat(RLEINST * pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;

    if (dw = RleDecompressQuery(pri, lpbiIn, NULL))
        return dw;

    dw = lpbiIn->biSize + (int)lpbiIn->biClrUsed * sizeof(RGBQUAD);

    //
    // if lpbiOut == NULL then, return the size required to hold a output
    // format
    //
    if (lpbiOut == NULL)
        return dw;

    hmemcpy(lpbiOut, lpbiIn, dw);

    lpbiOut->biBitCount    = lpbiIn->biBitCount;
    lpbiOut->biCompression = BI_RGB;
    lpbiOut->biSizeImage   = lpbiIn->biHeight * DibWidthBytes(lpbiIn);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleDecompressBegin(RLEINST * pri, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
    DWORD dw;

    if (dw = RleDecompressQuery(pri, lpbiIn, lpbiOut))
        return dw;

    pri->fDecompressBegin = TRUE;

    // Make sure we know the size of an uncompressed DIB
    if (lpbiOut->biSizeImage == 0)
	lpbiOut->biSizeImage = lpbiOut->biHeight * DibWidthBytes(lpbiOut);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleDecompress(RLEINST * pri, ICDECOMPRESS FAR *icinfo, DWORD dwSize)
{
    DWORD dw;

    if (!pri->fDecompressBegin)
    {
        if (dw = RleDecompressBegin(pri, icinfo->lpbiInput, icinfo->lpbiOutput))
            return dw;

        pri->fDecompressBegin = FALSE;
    }

    //
    //  handle a decompress of 'DIB ' (ie full frame) data.  Just return it.
    //  It may be disguised an an RLE.  We can tell by how big it is
    //
    if (icinfo->lpbiInput->biCompression == BI_RGB ||
	icinfo->lpbiInput->biSizeImage == icinfo->lpbiOutput->biSizeImage)
    {
        hmemcpy(icinfo->lpOutput, icinfo->lpInput,
			icinfo->lpbiInput->biSizeImage);
        return ICERR_OK;
    }

    DecodeRle(icinfo->lpbiOutput, icinfo->lpOutput, icinfo->lpInput);

    return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL RleDecompressEnd(RLEINST * pri)
{
    pri->fDecompressBegin = FALSE;
    return ICERR_OK;
}

/***************************************************************************

  DecodeRle   - 'C' version

  Play back a RLE buffer into a DIB buffer

  returns
      none

 ***************************************************************************/

void NEAR PASCAL DecodeRle(LPBITMAPINFOHEADER lpbi, LPVOID lp, LPVOID lpRle)
{
    UINT    cnt;
    BYTE    b;
    UINT    x;		
    UINT    dx,dy;
    UINT    wWidthBytes;

    #define RLE_ESCAPE  0
    #define RLE_EOL     0
    #define RLE_EOF     1
    #define RLE_JMP     2
    #define RLE_RUN     3

#ifndef _WIN32
    extern FAR PASCAL __WinFlags;
    #define WinFlags (UINT)(&__WinFlags)
    //
    // this uses ASM code found in RLEA.ASM
    //
    if (!(WinFlags & WF_CPU286))
        DecodeRle386(lpbi, lp, lpRle);
    else if (lpbi->biSizeImage < 65536l)
	DecodeRle286(lpbi, lp, lpRle);
    else
#endif
    {
        BYTE _huge *pb   = lp;
        BYTE _huge *prle = lpRle;

        wWidthBytes = (UINT)lpbi->biWidth+3 & ~3;

	x = 0;

	for(;;)
	{
	    cnt = (UINT)*prle++;
	    b   = *prle++;

	    if (cnt == RLE_ESCAPE)
	    {
		switch (b)
		{
		    case RLE_EOF:
			return;

		    case RLE_EOL:
			pb += wWidthBytes - x;
			x = 0;
			break;

		    case RLE_JMP:
			dx = (UINT)*prle++;
			dy = (UINT)*prle++;

			pb += (DWORD)wWidthBytes * dy + dx;
			x  += dx;

			break;

		    default:
			cnt = b;
			x  += cnt;
        		// If the count was sufficiently large it would be worthwhile
        		// using an inline memcpy function.  The code could
        		// be faster.  Even doing this as a series of word
        		// moves would be quicker.  However, RLE is not the highest
        		// priority.
			while (cnt-- > 0)
			    *pb++ = *prle++;   // copy

			if (b & 1)
			    prle++;

			break;
		}
	    }
	    else
	    {
		x += cnt;

		// If the count was sufficiently large it would be worthwhile
		// using an inline memset function.  The code could
		// be faster.  Even doing this as a series of word
		// moves would be quicker.  However, RLE is not the highest
		// priority.
#if 1
		// at least on the x86... this way persuades the compiler
		// to use registers more effectively through the whole of
		// the decode routine
		while (cnt-- > 0) {
		    *pb++ = b;  // set
		}

#else // the alternative
		memset(pb, b, cnt);
		pb += cnt;
#endif
	    }
	}
    }
}

#ifdef DEBUG

void FAR cdecl dprintf(LPSTR szFormat, ...)
{
    char ach[256];
    va_list va;

    static BOOL fDebug = -1;

    if (fDebug == -1)
        fDebug = GetProfileIntA("Debug", "MSRLE", FALSE);

    if (!fDebug)
        return;

    lstrcpyA(ach, "MSRLE: ");
    va_start(va, szFormat);
    wvsprintfA(ach+7, szFormat, va);
    va_end(va);
    lstrcatA(ach, "\r\n");

    OutputDebugStringA(ach);
}

#endif
