//////////////////////////////////////////////////////////////////////////
//
//  handle AVI RLE files with custom code.
//
//  use this code to deal with .AVI files without the MCIAVI runtime
//
//  restrictions:
//          AVI file must be a simple DIB format (RLE or none)
//          AVI file must fit into memory.
//
//  ToddLa
//
//////////////////////////////////////////////////////////////////////////


#include "ctlspriv.h"
extern "C" {
#include "rlefile.h"
}

#ifdef UNIX
#include <mwavi.h>
#include "unixstuff.h"
#endif

#include <lendian.hpp>

extern "C"
BOOL RleFile_Init(RLEFILE *prle, LPVOID pFile, HANDLE hRes, DWORD dwFileLen);

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

LPVOID LoadFile(LPCTSTR szFile, DWORD * pFileLength)
{
    LPVOID pFile;
    HANDLE hFile;
    HANDLE h;
    DWORD  FileLength;

#ifdef WIN32

#ifndef MAINWIN
    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#else
    // sunos5 does not want to map NFS files when there is locking on them
    hFile = CreateFile(szFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
#endif
    if (hFile == INVALID_HANDLE_VALUE)
        return 0;

    FileLength = (LONG)GetFileSize(hFile, NULL);

    if (pFileLength)
       *pFileLength = FileLength ;

    h = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (!h)
    {
        CloseHandle(hFile);
        return 0;
    }

    pFile = MapViewOfFile(h, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hFile);
    CloseHandle(h);

    if (pFile == NULL)
        return 0;
#else
    hFile = (HANDLE)_lopen(szFile, OF_READ);

    if (hFile == (HANDLE)-1)
        return 0;

    FileLength = _llseek((int)hFile, 0, SEEK_END);
    _llseek((int)hFile, 0, SEEK_SET);

    pFile = GlobalAllocPtr(GHND, FileLength);

    if (pFile && _hread((int)hFile, pFile, FileLength) != FileLength)
    {
        GlobalFreePtr(pFile);
        pFile = NULL;
    }
    _lclose((int)hFile);
#endif
    return pFile;
}


//////////////////////////////////////////////////////////////////////////
//
//  RleFile_OpenFromFile
//
//  load a .AVI file into memory and setup all of our pointers so we
//  know how to deal with it.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_OpenFromFile(RLEFILE *prle, LPCTSTR szFile)
{
    DWORD dwFileLen;
    LPVOID pFile;

    // MAKEINTRESOURCE() things can't come from files
    if (IS_INTRESOURCE(szFile))	
	return FALSE;

    if (pFile = LoadFile(szFile, &dwFileLen))
        return RleFile_Init(prle, pFile, NULL, dwFileLen);
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_OpenFromResource
//
//  load a .AVI file into memory and setup all of our pointers so we
//  know how to deal with it.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_OpenFromResource(RLEFILE *prle, HINSTANCE hInstance, LPCTSTR szName, LPCTSTR szType)
{
    HRSRC h;
    HANDLE hRes;

    // not a MAKEINTRESOURCE(), and points to NULL
#ifndef MAINWIN
    if (!IS_INTRESOURCE(szName) && (*szName == 0))
        return FALSE;
#else
    if (!MwIsIntegerResource(szName) && (*szName == 0))
	return FALSE;
#endif

    h = FindResource(hInstance, szName, szType);

    if (h == NULL)
        return FALSE;

    if (hRes = LoadResource(hInstance, h))
        return RleFile_Init(prle, LockResource(hRes), hRes, 0);
    else
        return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Close
//
//  nuke all stuff we did to open the file.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Close(RLEFILE *prle)
{
    if (prle->hpal)
        DeleteObject(prle->hpal);

    if (prle->pFile)
    {
#ifdef WIN32
        if (prle->hRes)
        {
#ifdef UNIX
            UnlockResource(prle->hRes);
#endif
            FreeResource(prle->hRes);
        }
        else
            UnmapViewOfFile(prle->pFile);
#else
        GlobalFreePtr(prle->pFile);
#endif
    }

#ifdef UNIX
    CHECK_FREE( prle->pStream );
    CHECK_FREE( prle->pFormat );
#endif

    prle->hpal = NULL;
    prle->pFile = NULL;
    prle->hRes = NULL;
    prle->pMainHeader = NULL;
    prle->pStream = NULL;
    prle->pFormat = NULL;
    prle->pMovie = NULL;
    prle->pIndex = NULL;
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Init
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Init(RLEFILE *prle, LPVOID pFile, HANDLE hRes, DWORD dwFileLen)
{
    DWORD_LENDIAN UNALIGNED *pdw;
    DWORD_LENDIAN UNALIGNED *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;
    int stream;

    if (prle->pFile == pFile)
        return TRUE;

    RleFile_Close(prle);
    prle->pFile = pFile;
    prle->hRes = hRes;

    if (prle->pFile == NULL)
        return FALSE;

    //
    //  now that the file is in memory walk the memory image filling in
    //  interesting stuff.
    //
    pdw = (DWORD_LENDIAN UNALIGNED *)prle->pFile;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

#ifndef UNIX
    if ((dwFileLen > 0) && (dwLength > dwFileLen)) {
        // File is physically shorter than the length written in its header.
        // Can't handle it.
        goto exit;
    }
#endif

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even a RIFF file

    if (dwType != formtypeAVI)
        goto exit;      // not a AVI file

    pdwEnd = (DWORD_LENDIAN UNALIGNED *)((BYTE PTR *)pdw + dwLength-4);
    stream = 0;

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
            case mmioFOURCC('L', 'I', 'S', 'T'):
                dwType = *pdw++;
                dwLength -= 4;

                switch (dwType)
                {
                    case listtypeAVIMOVIE:
                        prle->pMovie = (LPVOID)pdw;
                        break;

                    case listtypeSTREAMHEADER:
                    case listtypeAVIHEADER:
                        dwLength = 0;           // decend
                        break;

                    default:
                        break;                  // ignore
                }
                break;

            case ckidAVIMAINHDR:
            {
#ifdef UNIX
                MainAVIHeader mavih;
                prle->pMainHeader=&mavih;
                MwReadMainAVIHeader( (BYTE*) pdw,
                                     sizeof(*prle->pMainHeader),
                                     prle->pMainHeader );
#else
                prle->pMainHeader = (MainAVIHeader PTR *)pdw;
#endif
                prle->NumFrames = (int)prle->pMainHeader->dwTotalFrames;
                prle->Width     = (int)prle->pMainHeader->dwWidth;
                prle->Height    = (int)prle->pMainHeader->dwHeight;
                prle->Rate      = (int)(prle->pMainHeader->dwMicroSecPerFrame/1000);

                if (prle->pMainHeader->dwInitialFrames != 0)
                    goto exit;

                if (prle->pMainHeader->dwStreams > 2)
                    goto exit;

            }
                break;

            case ckidSTREAMHEADER:
            {
#ifdef UNIX
                AVIStreamHeader *paviSH;
#endif

                stream++;

                if (prle->pStream != NULL)
                    break;

#ifndef UNIX
                if (((AVIStreamHeader PTR *)pdw)->fccType != streamtypeVIDEO)
                    break;
#else
                paviSH = (AVIStreamHeader*) malloc(sizeof(AVIStreamHeader));
                if ( paviSH == NULL )
                    break;

                MwReadAVIStreamHeader( (BYTE*) pdw,sizeof(*paviSH),paviSH );
                if (paviSH->fccType != streamtypeVIDEO) {
                    CHECK_FREE( paviSH );
                    break;
                }
#endif

                prle->iStream = stream-1;

#ifndef UNIX
                prle->pStream = (AVIStreamHeader PTR*)pdw;
#else
                prle->pStream = paviSH;
#endif

                if (prle->pStream->dwFlags & AVISF_VIDEO_PALCHANGES)
                    goto exit;
            }
            break;

            case ckidSTREAMFORMAT:
                if (prle->pFormat != NULL)
                    break;

                if (prle->pStream == NULL)
                    break;

#ifdef UNIX
                prle->pFormat = (LPBITMAPINFOHEADER) malloc( dwLength );
                if ( prle->pFormat == NULL )
                   goto exit;

                MwReadBITMAPINFO( (BYTE*) pdw, dwLength,
                                  (BITMAPINFO*) prle->pFormat );
#else
                prle->pFormat = (LPBITMAPINFOHEADER)pdw;
#endif

                if (prle->pFormat->biSize != sizeof(BITMAPINFOHEADER))
                    goto exit;

                if (prle->pFormat->biCompression != 0 &&
                    prle->pFormat->biCompression != BI_RLE8)
                    goto exit;

                if (prle->pFormat->biWidth != prle->Width)
                    goto exit;

                if (prle->pFormat->biHeight != prle->Height)
                    goto exit;

                hmemcpy(&prle->bi, prle->pFormat, dwLength);
                prle->bi.biSizeImage = 0;
                prle->FullSizeImage = ((prle->bi.biWidth * prle->bi.biBitCount + 31) & ~31)/8U * prle->bi.biHeight;
                break;

            case ckidAVINEWINDEX:
                // we dont convert indexes because we dont know how many there are
                // but we will have to convert each usage of it
                prle->pIndex = (AVIINDEXENTRY PTR *)pdw;
                break;
        }

        pdw = (DWORD_LENDIAN *)((BYTE PTR *)pdw + ((dwLength+1)&~1));
    }

    //
    //  if the file has nothing in it we care about get out, note
    //  we dont need a index, we do need some data though.
    //
    if (prle->NumFrames == 0 ||
        prle->pMainHeader == NULL ||
        prle->pStream == NULL ||
        prle->pFormat == NULL ||
        prle->pMovie == NULL )
    {
        goto exit;
    }

    //
    //  if we cared about a palette we would create it here.
    //

    //
    //  file open'ed ok seek to the first frame.
    //
    prle->iFrame = -42;
    RleFile_Seek(prle, 0);
    return TRUE;

exit:
    RleFile_Close(prle);
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_ChangeColor
//
//  change the color table of the AVI
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_ChangeColor(RLEFILE *prle, COLORREF rgbS, COLORREF rgbD)
{
#ifndef UNIX

    DWORD dwS;
    DWORD dwD;
    DWORD PTR *ColorTable;
    int i;

    dwS = RGB(GetBValue(rgbS), GetGValue(rgbS), GetRValue(rgbS));
    dwD = RGB(GetBValue(rgbD), GetGValue(rgbD), GetRValue(rgbD));

    if (prle == NULL || prle->pFormat == NULL)
        return FALSE;

    ColorTable = (DWORD PTR *)((BYTE PTR *)&prle->bi + prle->bi.biSize);

    for (i=0; i<(int)prle->bi.biClrUsed; i++)
    {
        if (ColorTable[i] == dwS)
            ColorTable[i] = dwD;
    }

    return TRUE;

#else

    RGBQUAD dwS;
    RGBQUAD dwD;
    RGBQUAD PTR *ColorTable;
    int i;

    dwS.rgbRed      = GetRValue(rgbS);
    dwS.rgbGreen    = GetGValue(rgbS);
    dwS.rgbBlue     = GetBValue(rgbS);
    dwS.rgbReserved = 0;

    dwD.rgbRed   = GetRValue(rgbD);
    dwD.rgbGreen = GetGValue(rgbD);
    dwD.rgbBlue  = GetBValue(rgbD);

    // Support for CDE colormap index colors (davidd)
    dwD.rgbReserved = (BYTE)(rgbD >> 24);

    if (prle == NULL || prle->pFormat == NULL)
        return FALSE;

    ColorTable = (RGBQUAD PTR *)((BYTE PTR *)&prle->bi + prle->bi.biSize);

    for (i=0; i<(int)prle->bi.biClrUsed; i++)
    {
        if ((ColorTable[i].rgbRed == dwS.rgbRed) &&
	    (ColorTable[i].rgbGreen == dwS.rgbGreen) &&
	    (ColorTable[i].rgbBlue == dwS.rgbBlue)) {
            ColorTable[i] = dwD;
	}
    }

    return TRUE;
#endif

}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Seek
//
//  find the data for the specifed frame.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Seek(RLEFILE *prle, int iFrame)
{
    int n;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

#if 0
    if (iFrame == FRAME_CURRENT)
        iFrame = prle->iFrame;

    if (iFrame == FRAME_NEXT)
    {
        iFrame = prle->iFrame+1;
        if (iFrame >= prle->NumFrames)
            iFrame = 0;
    }

    if (iFrame == FRAME_PREV)
    {
        iFrame = prle->iFrame-1;
        if (iFrame == -1)
            iFrame = prle->NumFrames-1;
    }
#endif

    if (iFrame >= prle->NumFrames)
        return FALSE;

    if (iFrame < 0)
        return FALSE;

    if (iFrame == prle->iFrame)
        return TRUE;

    if (prle->iFrame >= 0 && prle->iFrame < iFrame)
    {
        n = prle->nFrame;       // start where you left off last time
    }
    else
    {
        n = -1;                 // start at the begining
        prle->iFrame = -1;      // current frame
        prle->iKeyFrame = 0;    // current key
    }

    while (prle->iFrame < iFrame)
    {
        n++;
        if (StreamFromFOURCC(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].ckid)) == (UINT)prle->iStream)
        {
            prle->iFrame++;         // new frame

            if ((long)(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwFlags)) & AVIIF_KEYFRAME)
                prle->iKeyFrame = prle->iFrame;     /* // new key frame */
        }
    }

    prle->nFrame = n;
/* warning this points to bitmap bits in wintel format ! */
    prle->pFrame = (BYTE PTR *)prle->pMovie +
	(int)(*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwChunkOffset)) + 4;
    prle->cbFrame = *(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].dwChunkLength);

    ASSERT( (DWORD)(*(DWORD_LENDIAN UNALIGNED *)&(((DWORD PTR *)prle->pFrame)[-1])) == (DWORD)prle->cbFrame);
    ASSERT( (DWORD)(*(DWORD_LENDIAN UNALIGNED *)&(((DWORD PTR *)prle->pFrame)[-2])) == (DWORD)*(DWORD_LENDIAN UNALIGNED *)(&prle->pIndex[n].ckid));

    prle->bi.biSizeImage = prle->cbFrame;

    if (prle->cbFrame == prle->FullSizeImage)
        prle->bi.biCompression = 0;
    else
        prle->bi.biCompression = BI_RLE8;
		
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Paint
//
//  draw the specifed frame, makes sure the entire frame is updated
//  dealing with non-key frames correctly.
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Paint(RLEFILE *prle, HDC hdc, int iFrame, int x, int y)
{
    int i;
    BOOL f;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

    if (f = RleFile_Seek(prle, iFrame))
    {
        iFrame = prle->iFrame;

        for (i=prle->iKeyFrame; i<=iFrame; i++)
            RleFile_Draw(prle, hdc, i, x, y);
    }

    return f;
}

//////////////////////////////////////////////////////////////////////////
//
//  RleFile_Draw
//
//  draw the data for a specifed frame
//
//////////////////////////////////////////////////////////////////////////

extern "C"
BOOL RleFile_Draw(RLEFILE *prle, HDC hdc, int iFrame, int x, int y)
{
    BOOL f;

    if (prle == NULL || prle->pMovie == NULL)
        return FALSE;

    if (prle->hpal)
    {
        SelectPalette(hdc, prle->hpal, FALSE);
        RealizePalette(hdc);
    }

    if (f = RleFile_Seek(prle, iFrame))
    {
        if (prle->cbFrame > 0)
        {
            StretchDIBits(hdc,
                    x, y, prle->Width, prle->Height,
                    0, 0, prle->Width, prle->Height,
                    prle->pFrame, (LPBITMAPINFO)&prle->bi,
                    DIB_RGB_COLORS, SRCCOPY);
        }
    }

    return f;
}
