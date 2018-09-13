
#include "stdafx.h"
#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "pbrusvw.h"
#include "minifwnd.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgbrush.h"
#include "imgwell.h"
#include "imgtools.h"
#include "toolbox.h"
#include "imgfile.h"
#include "colorsrc.h"
#include "undo.h"
#include "props.h"
#include "ferr.h"
#include "cmpmsg.h"
#include "loadimag.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE( CBitmapObj, CObject )

#include "memtrace.h"

/***************************************************************************/
// Map from the value in CBitmapObj::m_nColors to bits per pixel

int mpncolorsbits [] =
    {
    1, 4, 8, 24
    };

/***************************************************************************/

CBitmapObj::CBitmapObj() : CObject(), m_dependants()
    {
    m_bDirty      = FALSE;
    m_bTempName   = FALSE;
    m_lpvThing    = NULL;
    m_lMemSize    = 0L;
    m_pImg        = NULL;
    m_nWidth      = 0;
    m_nHeight     = 0;
    m_nColors     = 0;
    m_nSaveColors = -1;
#ifdef ICO_SUPPORT
    m_bSaveIcon   = FALSE;
#endif
#ifdef PCX_SUPPORT
    m_bPCX        = FALSE;
#endif
    m_bCompressed = FALSE;
    m_nShrink     = 0;
    m_dwOffBits   = 0;
    }

/***************************************************************************/

CBitmapObj::~CBitmapObj()
    {
    ASSERT_VALID(this);

    InformDependants( SN_DESTROY );

    if (m_lpvThing != NULL)
        {
        Free();
        }
    if (m_pImg)
        FreeImg(m_pImg);
    }

/***************************************************************************/

void CBitmapObj::AddDependant( CBitmapObj* newDependant )
    {
    POSITION pos = m_dependants.Find( newDependant );

    if (pos == NULL)
        m_dependants.AddTail( newDependant );
    }

/***************************************************************************/

void CBitmapObj::RemoveDependant( CBitmapObj* oldDependant )
    {
    POSITION pos = m_dependants.Find(oldDependant);

    if (pos != NULL)
        m_dependants.RemoveAt(pos);
    }

/***************************************************************************/

void CBitmapObj::InformDependants( UINT idChange )
    {
    POSITION pos = m_dependants.GetHeadPosition();

    while (pos != NULL)
        {
        CBitmapObj* pSlob = (CBitmapObj*)m_dependants.GetNext(pos);
        pSlob->OnInform(this, idChange);
        }
    }

/***************************************************************************/

void CBitmapObj::OnInform( CBitmapObj* pChangedSlob, UINT idChange )
    {
    if (idChange == SN_DESTROY)
        {
        POSITION pos = m_dependants.Find(pChangedSlob);

        if (pos != NULL)
            m_dependants.RemoveAt(pos);
        }
    }

/***************************************************************************/

void CBitmapObj::SetDirty(BOOL bDirty)
    {
    m_bDirty = bDirty;
    }

/*****************************************************************************/

void CBitmapObj::Zap()
    {
    m_bDirty = FALSE;
    }

/*****************************************************************************/

BOOL CBitmapObj::Alloc()  // m_lpvThing of size m_lMemSize
    {
    if (m_lMemSize == 0L)
        return FALSE;

    m_lpvThing = (LPVOID)new BYTE[m_lMemSize];

    if (m_lpvThing == NULL)
        {
        theApp.SetMemoryEmergency( TRUE );
        return FALSE;
        }

    memset( m_lpvThing, 0, m_lMemSize );

    return TRUE;
    }

/*****************************************************************************/

void CBitmapObj::Free()             // m_lpvThing and set m_lMemSize to zero
    {
    if (m_lpvThing == NULL)
        {
        TRACE(TEXT("Warning: called Free on a CBitmapObj with no thing!\n"));
        return;
        }

    delete m_lpvThing;

    m_lpvThing = NULL;
    m_lMemSize = 0;
    }

/***************************************************************************/

CString CBitmapObj::GetDefExtension(int iStringId)
    {
    CString cStringExtension;

    if (iStringId != 0)
        {
        TRY
            {
            cStringExtension.LoadString( iStringId );
            }
        CATCH(CMemoryException,e)
            {
            cStringExtension.Empty();
            }
        END_CATCH
        }
    else
        {
        cStringExtension.Empty();
        }

    return cStringExtension;
    }

void PBGetDefDims(int &pnWidth, int &pnHeight)
{
    // Setup default parameters...
    // Don't use the whole screen, those bitmaps get HUGE
    //
    pnWidth  = GetSystemMetrics( SM_CXSCREEN )/2;
    pnHeight = GetSystemMetrics( SM_CYSCREEN )/2;

    // Check if this is a low memory machine and use a small default bitmap
    // size
    if (GetSystemMetrics(SM_SLOWMACHINE) & 0x0002)
    {
        pnWidth  = 640/2;
        pnHeight = 480/2;
    }
}

/*****************************************************************************/

BOOL CBitmapObj::MakeEmpty()
    {
    PBGetDefDims(m_nWidth, m_nHeight);

    if (theApp.m_sizeBitmap.cx
    &&  theApp.m_sizeBitmap.cy)
        {
        m_nWidth  = theApp.m_sizeBitmap.cx;
        m_nHeight = theApp.m_sizeBitmap.cy;
        }

    if (theApp.m_bEmbedded)
        {
       // make a nice size for embedded objects, lets try for 5 centimeters
        m_nWidth  = theApp.ScreenDeviceInfo.ixPelsPerDM / 2;
        m_nHeight = theApp.ScreenDeviceInfo.iyPelsPerDM / 2;
        }

    //
    // default to 256 colors if not monochrome
    //
    m_nColors = theApp.m_bMonoDevice? 0 : 2;
    m_bDirty  = TRUE;

    return TRUE;
    }

/*****************************************************************************/
// Create and setup an IMG for this resource

BOOL CBitmapObj::CreateImg()
    {
    ASSERT(! m_pImg);

    LPSTR lpbi = (LPSTR)m_lpvThing; // NOTE: this is NULL for new resources!

    if (lpbi)
        {
        if (IS_WIN30_DIB( lpbi ))
            m_bCompressed = ((LPBITMAPINFOHEADER)lpbi)->biCompression != BI_RGB;

        m_nWidth  = (int)DIBWidth ( lpbi );
        m_nHeight = (int)DIBHeight( lpbi );
        m_nColors =   DIBNumColors( lpbi, FALSE );

        if (m_nColors <= 0 || m_nColors > 256)
            m_nColors  = 3;
        else
            if (m_nColors <= 2)
                m_nColors = 0;
            else
                if (m_nColors <= 16)
                    m_nColors  = 1;
                else
                    if (m_nColors <= 256)
                        m_nColors  = 2;
        }

    UINT nColors = (m_nColors? 0: 1);

    m_pImg = ::CreateImg( lpbi ? 0 : m_nWidth, lpbi ? 0 : m_nHeight,
                nColors, nColors, theApp.m_bPaletted );

    if (! m_pImg)
        {
        TRACE(TEXT("CreateImg failed\n"));

        theApp.SetMemoryEmergency();

        return FALSE;
        }

    if (g_pColors)
    {
       g_pColors->ResetColors ((m_nColors==1)?16:256);
    }

    m_pImg->cxWidth  = m_nWidth;
    m_pImg->cyHeight = m_nHeight;

    if (! lpbi)
        {
        nColors = m_pImg->cPlanes * m_pImg->cBitCount;

        //BUGBUG- Shouldn't this be " == 0 || == 1" ??
        //Half a page up negative values == TRUE color!
                //This shell game with the values sucks...
        if (nColors <= 1)
            m_nColors = 0;
        else
            if (nColors <= 4)
                m_nColors = 1;
            else
                if (nColors <= 8)
                    m_nColors = 2;
                else // 24-bit image
                    m_nColors = 3;
        }

    m_pImg->m_pBitmapObj = this;
    m_pImg->bDirty       = m_bDirty;

   if (lpbi)
        {
        // Load the bitmap/icon/cursor...
        HBITMAP hbm = DIBToDS( lpbi, m_dwOffBits, m_pImg->hDC );

        if (! hbm)
            {
            theApp.SetMemoryEmergency();
            return FALSE;
            }

        m_pImg->hBitmap    = hbm;
        m_pImg->hBitmapOld = (HBITMAP)::SelectObject( m_pImg->hDC, hbm );
        }

        if ( theApp.m_bPaletted)
        // If LoadImage was used && paletted
                {
                // Create the Palette from the dib section instead.
        m_pImg->m_pPalette = PaletteFromDS(m_pImg->hDC);
                }

    theApp.m_pPalette = NULL;

    if (m_pImg->m_pPalette && theApp.m_bPaletted)
        {
        m_pImg->m_hPalOld = SelectPalette( m_pImg->hDC,
                                    (HPALETTE)m_pImg->m_pPalette->m_hObject,
                                              FALSE );
        RealizePalette( m_pImg->hDC );

        theApp.m_pPalette = m_pImg->m_pPalette;
        }
    else
        if (m_pImg->m_pPalette)
            {
            delete m_pImg->m_pPalette;
            m_pImg->m_pPalette = NULL;
            m_pImg->m_hPalOld  = NULL;
            }

    if (g_pColors)
        g_pColors->SetMono( ! m_nColors );

    return TRUE;
    }

/*****************************************************************************/

BOOL CBitmapObj::Export(const TCHAR* szFileName)
    {
    // If the file already exists and we aren't dirty, then don't bother
    // saving, just return...
    CFileStatus fStat;
    CString strFullName;

    MkFullPath( strFullName, (const TCHAR*)szFileName );

    if (CFile::GetStatus( strFullName, fStat ) && ! m_bDirty)
        return TRUE;

    CFile file;

    CFileException e;
    CFileSaver saver( szFileName );

    if (! saver.CanSave())
        return FALSE;

    theApp.SetFileError( IDS_ERROR_EXPORT, CFileException::none, szFileName );

    if (! OpenSubFile( file, saver, CFile::modeWrite
                                  | CFile::modeCreate
                                  | CFile::typeBinary, &e ))
        {
        theApp.SetFileError( IDS_ERROR_EXPORT, e.m_cause );
        return FALSE;
        }

    BOOL bWritten = FALSE;

    TRY
        {
#ifdef PCX_SUPPORT
        if (m_bPCX)
            bWritten = WritePCX( &file );
        else
#endif
            bWritten = WriteResource( &file );

        file.Close();
        }
    CATCH( CFileException, ex )
        {
        file.Abort();
        theApp.SetFileError( IDS_ERROR_EXPORT, ex->m_cause );
        return FALSE;
        }
    END_CATCH

    if (bWritten)
        bWritten = saver.Finish();
    else
                   saver.Finish();

    return bWritten;
    }

typedef union _BITMAPHEADER
{
        BITMAPINFOHEADER bmi;
        BITMAPCOREHEADER bmc;
} BITMAPHEADER, *LPBITMAPHEADER;

inline WORD PaletteSize(LPBITMAPHEADER lpHdr) {return(PaletteSize((LPSTR)lpHdr));}
inline WORD DIBNumColors(LPBITMAPHEADER lpHdr) {return(DIBNumColors((LPSTR)lpHdr));}
inline DWORD DIBWidth(LPBITMAPHEADER lpHdr) {return(DIBWidth((LPSTR)lpHdr));}
inline DWORD DIBHeight(LPBITMAPHEADER lpHdr) {return(DIBHeight((LPSTR)lpHdr));}

/*****************************************************************************/

BOOL CBitmapObj::WriteResource( CFile* pfile, PBResType rtType )
    {
    BOOL bPBrushOLEHeader = (rtType == rtPBrushOLEObj);
    BOOL bFileHeader = (rtType == rtFile)|| (rtType == rtPaintOLEObj) || bPBrushOLEHeader;

    if (m_pImg == NULL)
        {
        // The image has not been loaded, so we'll just copy the
        // original out to the file...
        ASSERT( m_lpvThing );

        if (! m_lpvThing)
            return FALSE;
        }
    else
        {
        // The image has been loaded and may have been edited, so
        // we'll convert it back to a dib to save...
        if (! m_lpvThing)
            SaveResource( FALSE );

        if (! m_lpvThing)
            return FALSE;
        }

    LPBITMAPHEADER lpDib    = (LPBITMAPHEADER)m_lpvThing;
    DWORD dwLength = m_lMemSize;
    DWORD dwWriteLength = dwLength;
    DWORD dwHeadLength = 0;

        struct _BMINFO
        {
                BITMAPINFOHEADER hdr;
                RGBQUAD rgb[256];
        } bmInfo;

        LPBITMAPHEADER lpOldHdr = lpDib;
        LPBITMAPHEADER lpNewHdr = lpOldHdr;

        DWORD dwOldHdrLen = lpOldHdr->bmi.biSize + PaletteSize(lpOldHdr);
        DWORD dwNewHdrLen = dwOldHdrLen;

    if (bPBrushOLEHeader)
        {
                if (!IS_WIN30_DIB(lpDib))
                {
                        LPBITMAPCOREINFO lpCoreInfo = (LPBITMAPCOREINFO)(&lpOldHdr->bmc);
                        memset(&bmInfo.hdr, 0, sizeof(bmInfo.hdr));
                        bmInfo.hdr.biSize = sizeof(bmInfo.hdr);
                        bmInfo.hdr.biWidth  = lpCoreInfo->bmciHeader.bcWidth;
                        bmInfo.hdr.biHeight = lpCoreInfo->bmciHeader.bcHeight;
                        bmInfo.hdr.biPlanes   = lpCoreInfo->bmciHeader.bcPlanes;
                        bmInfo.hdr.biBitCount = lpCoreInfo->bmciHeader.bcBitCount;
                        bmInfo.hdr.biCompression = BI_RGB;

                        for (int i=DIBNumColors(lpOldHdr)-1; i>=0; --i)
                        {
                                bmInfo.rgb[i].rgbBlue  = lpCoreInfo->bmciColors[i].rgbtBlue;
                                bmInfo.rgb[i].rgbGreen = lpCoreInfo->bmciColors[i].rgbtGreen;
                                bmInfo.rgb[i].rgbRed   = lpCoreInfo->bmciColors[i].rgbtRed;
                                bmInfo.rgb[i].rgbReserved = 0;
                        }

                        lpNewHdr = (LPBITMAPHEADER)(&bmInfo);
                        dwNewHdrLen = lpNewHdr->bmi.biSize + PaletteSize(lpNewHdr);
                }

                dwWriteLength += dwNewHdrLen - dwOldHdrLen;
                dwLength      += dwNewHdrLen - dwOldHdrLen;

                if (bFileHeader)
                {
       #ifdef ICO_SUPPORT
                        if (IsSaveIcon())
                        {
                                dwHeadLength = sizeof(ICONFILEHEADER);
                                dwWriteLength += dwHeadLength;
                        }
                        else
       #endif
                        {
                                dwHeadLength = sizeof(BITMAPFILEHEADER);
                                dwWriteLength += dwHeadLength;

                                // PBrush rounded up to 32 bytes (I don't know why)
                                dwWriteLength = (dwWriteLength+31) & ~31;
                        }
                }

        pfile->Write( &dwWriteLength, sizeof( dwWriteLength ));
        }

    if (bFileHeader)
        {
                //BUGBUG-Icon support is not in application anymore, right?
    #ifdef ICO_SUPPORT
        if (IsSaveIcon())
            {
            ICONFILEHEADER hdr;

            hdr.icoReserved      = 0;
            hdr.icoResourceType  = 1;
            hdr.icoResourceCount = 1;

            pfile->Write( &hdr, sizeof( ICONFILEHEADER ) );
            pfile->Seek( sizeof( ICONDIRENTRY ), CFile::current );
            }
        else
    #endif
            {
            BITMAPFILEHEADER hdr;

            hdr.bfType      = ((WORD)('M' << 8) | 'B');
            hdr.bfSize      = dwLength + sizeof( BITMAPFILEHEADER );
            hdr.bfReserved1 = 0;
            hdr.bfReserved2 = 0;
            hdr.bfOffBits   = (DWORD)sizeof(hdr)
                            + lpNewHdr->bmi.biSize
                            + PaletteSize(lpNewHdr);

            pfile->Write( &hdr, sizeof( hdr ));
            }
        }

        pfile->Write(lpNewHdr, dwNewHdrLen);

    BYTE* hp  = ((BYTE*)lpDib) + dwOldHdrLen;
        // We subtract the new header length because we have already translated
        // dwLength to the new size
    DWORD dwWrite   = dwLength - dwNewHdrLen;
    DWORD dwIconPos = pfile->GetPosition();;

    while (dwWrite > 0)
        {
        UINT cbWrite = (UINT)min( dwWrite, 16384 );

        pfile->Write( (LPVOID)hp, cbWrite );

        hp      += cbWrite;
        dwWrite -= cbWrite;
        }

        dwWriteLength -= dwHeadLength;
        if (dwWriteLength > dwLength)
        {
                // We rounded up to 32 bytes above, so this should always be < 32
                ASSERT(dwWriteLength-dwLength < 32);

                DWORD dwZeros[] =
                {
                        0, 0, 0, 0, 0, 0, 0, 0,
                } ;

                pfile->Write( dwZeros, dwWriteLength-dwLength );
        }

    ASSERT( dwWrite == 0 );

        //BUGBUG-Icon support is not in application anymore, right?
   #ifdef ICO_SUPPORT
    if (IsSaveIcon())
        {
        DWORD nextPos = pfile->GetPosition();

        pfile->Seek( (bFileHeader? sizeof( ICONFILEHEADER ): 0), CFile::begin );

        ICONDIRENTRY dir;

        dir.nWidth       = (BYTE)DIBWidth    ( lpDib );
        dir.nHeight      = (BYTE)DIBHeight   ( lpDib ) / 2;
        dir.nColorCount  = (BYTE)DIBNumColors( lpDib );
        dir.bReserved    = 0;
        dir.wReserved1   = 0;
        dir.wReserved2   = 0;
        dir.icoDIBSize   = dwLength;
        dir.icoDIBOffset = dwIconPos;

        pfile->Write( &dir, sizeof( ICONDIRENTRY ) );
        pfile->Seek( nextPos, CFile::begin );
        }
    else
   #endif
        m_bDirty = FALSE;

    pfile->Flush();

    return TRUE;
    }

/*****************************************************************************/

BOOL CBitmapObj::Import( LPCTSTR szFileName )
    {
    CFile          file;
    CFileException e;

    theApp.SetFileError( IDS_ERROR_READLOAD, CFileException::none, szFileName );

    if (! file.Open( szFileName, CFile::modeRead | CFile::typeBinary, &e ))
        {
        theApp.SetFileError( IDS_ERROR_READLOAD, e.m_cause );
        return FALSE;
        }

    BOOL bGoodFile = TRUE;


    TRY
        {
        bGoodFile = ReadResource( &file );
        file.Close();
        }
    CATCH(CFileException, ex)
        {
        file.Abort();
        bGoodFile = FALSE;
        }
    END_CATCH

    if (!bGoodFile)
        {
        LPBITMAPINFOHEADER lpbi;

        if (lpbi = LoadDIBFromFile(szFileName))
            {
            bGoodFile = ReadResource(lpbi);
            FreeDIB(lpbi);

            if (bGoodFile)
                {
                theApp.SetFileError(0, CFileException::none);
                }
            else
                {
                theApp.SetFileError( IDS_ERROR_READLOAD, ferrNotValidBmp);
                }
            }
        }

    return bGoodFile;
    }

/*****************************************************************************/

BOOL CBitmapObj::ReadResource( CFile* pfile, PBResType rtType )
    {

    BOOL bPBrushOLEHeader = (rtType == rtPBrushOLEObj);
    BOOL bFileHeader = (rtType == rtFile)
     || (rtType == rtPaintOLEObj)|| bPBrushOLEHeader;

    DWORD dwLength = pfile->GetLength();
    // special case zero length files.
    if (! dwLength)
        {
        if (m_lpvThing)
            Free();

        m_bDirty = TRUE;

        return TRUE;
        }

        if (bPBrushOLEHeader)
        {
                DWORD dwReadLen;

                if (pfile->Read( &dwReadLen, sizeof( dwReadLen )) != sizeof( dwReadLen )
                        || dwReadLen > dwLength)
                {
                        theApp.SetFileError( IDS_ERROR_READLOAD, ferrNotValidBmp );
                        return FALSE;
                }
                dwLength -= sizeof(dwReadLen);
        }

        m_dwOffBits = 0;

    if (bFileHeader)
        {
        BITMAPFILEHEADER hdr;

        if (pfile->Read( &hdr, sizeof( hdr )) != sizeof( hdr ))
            {
            theApp.SetFileError( IDS_ERROR_READLOAD, ferrNotValidBmp );
            return FALSE;
            }


        if (hdr.bfType != ((WORD)('M' << 8) | 'B'))
            {
            theApp.SetFileError( IDS_ERROR_READLOAD, ferrNotValidBmp );
            return FALSE;
            }


        dwLength -= sizeof( hdr );

        // Store the offset from the beginning of the BITMAPINFO
        if (hdr.bfOffBits)
        {
            m_dwOffBits = hdr.bfOffBits - sizeof(hdr);
        }
        else
        {
            m_dwOffBits = 0;
        }


       }

    if (m_lpvThing != NULL)
        Free();

    m_lMemSize = dwLength;

    if (! Alloc())
        return FALSE;

    ASSERT( m_lpvThing );

    BYTE* hp = (BYTE*)m_lpvThing;

    while (dwLength > 0)
        {
        UINT cbRead = (UINT)min( dwLength, 16384 );

        if (pfile->Read( (void FAR*)hp, cbRead ) != cbRead)
            {
            theApp.SetFileError( IDS_ERROR_READLOAD, ferrReadFailed );
            return FALSE;
            }

        dwLength -= cbRead;
        hp       += cbRead;
        }

    ASSERT( dwLength == 0 );

    //
    // Calculate the bits offset because the BITMAPFILEHEADER had 0
    //
    if (!m_dwOffBits)
    {
        m_dwOffBits = (DWORD)(FindDIBBits ((LPSTR)m_lpvThing, 0) -
	                      (LPSTR)m_lpvThing);
    }
    return TRUE;
    }

/*****************************************************************************/

BOOL CBitmapObj::ReadResource( LPBITMAPINFOHEADER lpbi )
    {
    DWORD dwSizeImage;

    if (lpbi == NULL || lpbi->biSize != sizeof(BITMAPINFOHEADER))
        {
        theApp.SetFileError( IDS_ERROR_READLOAD, ferrNotValidBmp );
        return FALSE;
        }

    m_dwOffBits = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
        m_dwOffBits += (1 << lpbi->biBitCount) * sizeof(RGBQUAD);

    if (lpbi->biSizeImage)
        dwSizeImage = lpbi->biSizeImage;
    else
        dwSizeImage = abs(lpbi->biHeight) * ((lpbi->biWidth*lpbi->biBitCount+31)&~31)/8;

    if (m_lpvThing != NULL)
        Free();

    m_lMemSize = m_dwOffBits + dwSizeImage;

    if (!Alloc())
        return FALSE;

    CopyMemory(m_lpvThing, lpbi, m_lMemSize);

    lpbi = (LPBITMAPINFOHEADER)m_lpvThing;
    lpbi->biXPelsPerMeter = 0;
    lpbi->biYPelsPerMeter = 0;

    return TRUE;
    }

/*****************************************************************************/

void CBitmapObj::ReLoadImage( CPBDoc* pbDoc )
    {
    FreeImg( m_pImg );
    CleanupImgUndo();
    CleanupImgRubber();
    m_pImg = NULL;
    CreateImg();

    POSITION pos   = pbDoc->GetFirstViewPosition();
    CPBView* pView = (CPBView*)pbDoc->GetNextView( pos );

    if (pView)
        {
        pView->m_pImgWnd->SetImg( m_pImg );
        pbDoc->UpdateAllViews( pView );
        InvalImgRect( m_pImg, NULL );
        }
    }

/*****************************************************************************/

void SwapBitmaps(HDC hDC1, int x1, int y1, int wid, int hgt,
        HDC hDC2, int x2, int y2, CPalette* pPalette)
{
#if 0
// We would like to just XOR 3 times to swap, but sometimes the middle of the
// palette is empty, so we cannot
    BitBlt(m_pImg->hDC, rect.left   , rect.top,
                        rect.Width(), rect.Height(), hDC, 0, 0, DSx);
    BitBlt(hDC, 0, 0, rect.Width(), rect.Height(), m_pImg->hDC,
                                           rect.left, rect.top, DSx);
    BitBlt(m_pImg->hDC, rect.left, rect.top, rect.Width(), rect.Height(),
                                                     hDC, 0, 0, DSx);
#else
        CDC dcTemp;

        CDC dc1, dc2;
        dc1.Attach(hDC1);
        dc2.Attach(hDC2);



        BOOL bSuccess = dcTemp.CreateCompatibleDC(&dc1);

        // Don't create a bitmap that is too large, or we will spend all our time
        // swapping
        int hgtTemp = 0x10000/wid;
        hgtTemp = min(hgt, max(1, hgtTemp));

        CBitmap bmTemp;
        bSuccess = bSuccess && bmTemp.CreateCompatibleBitmap(&dc1, wid, hgtTemp);
        bSuccess = bSuccess && dcTemp.SelectObject(&bmTemp)!=NULL;

        if (!bSuccess)
        {
                // Make sure the DC's do not get deleted
                dc1.Detach();
                dc2.Detach();
                return;
        }

        if (pPalette)
        {
                dcTemp.SelectPalette(pPalette, TRUE);
                dcTemp.RealizePalette();
        }

        int yTemp;
        for (yTemp=0; yTemp<hgt; yTemp+=hgtTemp)
        {
                hgtTemp = min(hgtTemp, hgt-yTemp);

                dcTemp.BitBlt(0, 0, wid, hgtTemp, &dc1, x1, y1+yTemp, SRCCOPY);
                dc1.BitBlt(x1, y1+yTemp, wid, hgtTemp, &dc2   , x2, y2+yTemp, SRCCOPY);
                dc2.BitBlt(x2, y2+yTemp, wid, hgtTemp, &dcTemp, 0 , 0, SRCCOPY);
        }

        // Make sure the DC's do not get deleted
        dc1.Detach();
        dc2.Detach();

        // Note that I explicitly delete the DC first, so I do not have to worry
        // about selecting old objects back in
        dcTemp.DeleteDC();
#endif
}

void CBitmapObj::UndoAction( CBmObjSequence* pSeq, UINT nActionID )
    {
    switch (nActionID)
        {
        default:
            break;

        case A_ImageChange:

            if (((CImgTool::GetCurrentID() == IDMB_PICKTOOL)
            ||   (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL))
               && theImgBrush.m_pImg != NULL)
                {
                HideBrush();
                InvalImgRect( theImgBrush.m_pImg, NULL ); // hide tracker
                theImgBrush.m_pImg = NULL;
                }

            int     cb;
            CRect   rect;
            HBITMAP hImgBitmap;

            pSeq->RetrieveInt( cb );

            ASSERT(cb == sizeof( CRect ) + sizeof( hImgBitmap  ));

            pSeq->RetrieveRect( rect );

            int nCursor = pSeq->m_nCursor;

            pSeq->Retrieve( (BYTE*)&hImgBitmap, sizeof( hImgBitmap ) );

            // Wipe out the old handles since we're reusing them in the
            // new record and we don't what them deleted when this record
            // is removed!
            memset(&pSeq->ElementAt(nCursor), 0, sizeof( hImgBitmap ));

            // Perform undo using these parameters...

            SetupRubber(m_pImg);
            SetUndo(m_pImg); // For redo...
            HideBrush();

            ASSERT(m_pImg != NULL);

            HDC hDC = CreateCompatibleDC(m_pImg->hDC);

            if (hDC == NULL)
                {
                theApp.SetGdiEmergency();
                return;
                }

            HPALETTE hOldPalette = NULL;
            HBITMAP  hOldBitmap  = (HBITMAP)SelectObject(hDC, hImgBitmap);

            ASSERT(hOldBitmap != NULL);

            if (m_pImg->m_pPalette)
                {
                hOldPalette = SelectPalette( hDC, (HPALETTE)m_pImg->m_pPalette->m_hObject,
                                                   FALSE ); // Background ??
                RealizePalette( hDC );
                }

            // Three blits here swap the image and the undo bits, that
            // way the undo bits are set up for a redo!

            ASSERT(m_pImg->hDC != NULL);
            SwapBitmaps(m_pImg->hDC, rect.left, rect.top,
                rect.Width(), rect.Height(), hDC, 0, 0, m_pImg->m_pPalette);

            if (hOldPalette)
                SelectPalette( hDC, hOldPalette, FALSE ); // Background ??

            SelectObject(hDC, hOldBitmap);
            DeleteDC(hDC);

            InvalImgRect (m_pImg, &rect);
            CommitImgRect(m_pImg, &rect);

            // Record the redo information...

            theUndo.Insert((BYTE*)&hImgBitmap, sizeof (hImgBitmap));
            theUndo.InsertRect(rect);
            theUndo.InsertInt(sizeof (CRect) + sizeof (hImgBitmap));
            theUndo.InsertInt(A_ImageChange);
            theUndo.InsertPtr(m_pImg->m_pBitmapObj);
            theUndo.InsertByte(CUndoBmObj::opAction);

            break;
        }
    }

/*****************************************************************************/

void CBitmapObj::DeleteUndoAction(CBmObjSequence* pSeq, UINT nActionID)
    {
    switch (nActionID)
        {
        default:
            break;

        case A_ImageChange:
            CRect rect;
            HBITMAP hImgBitmap;

            pSeq->RetrieveRect(rect);
            pSeq->Retrieve((BYTE*)&hImgBitmap, sizeof (hImgBitmap));

            if (hImgBitmap != NULL)
                DeleteObject(hImgBitmap);
            break;
        }
    }

/*****************************************************************************/

BOOL CBitmapObj::FinishUndo(const CRect* pRect)
    {
    ASSERT( g_hUndoImgBitmap );

    CRect rect;
    if (pRect == NULL)
        rect.SetRect(0, 0, m_pImg->cxWidth, m_pImg->cyHeight);
    else
        rect = *pRect;

    HDC      hDC1 = NULL;
    HDC      hDC2 = NULL;
    HPALETTE hOldPalette  = NULL;
    HPALETTE hOldPalette2 = NULL;
    HBITMAP  hImgBitmap  = NULL;
    HBITMAP  hOldBitmap1;
    HBITMAP  hOldBitmap2;

    if (rect.left >= rect.right || rect.top >= rect.bottom)
        {
        // Not an error, just nothing to do...
        return TRUE;
        }

    hImgBitmap = CreateCompatibleBitmap( m_pImg->hDC, rect.Width(), rect.Height() );

    if (hImgBitmap == NULL)
        goto LError;

    if ((hDC1 = CreateCompatibleDC(m_pImg->hDC)) == NULL)
        goto LError;

    if ((hDC2 = CreateCompatibleDC(m_pImg->hDC)) == NULL)
        goto LError;

    if (m_pImg->m_pPalette)
        {
        hOldPalette = SelectPalette(hDC1, (HPALETTE)m_pImg->m_pPalette->m_hObject, FALSE );
        RealizePalette( hDC1 );

        hOldPalette2 = SelectPalette(hDC2, (HPALETTE)m_pImg->m_pPalette->m_hObject, FALSE );
        RealizePalette( hDC2 );
        }

    VERIFY((hOldBitmap1 = (HBITMAP)SelectObject(hDC1,       hImgBitmap)) != NULL);
    VERIFY((hOldBitmap2 = (HBITMAP)SelectObject(hDC2, g_hUndoImgBitmap)) != NULL);

    BitBlt(hDC1, 0, 0, rect.Width(), rect.Height(),
           hDC2,       rect.left   , rect.top, SRCCOPY);

    SelectObject(hDC1, hOldBitmap1);
    SelectObject(hDC2, hOldBitmap2);

    if (hOldPalette != NULL)
        {
        ::SelectPalette(hDC1, hOldPalette, FALSE ); // Background ??
        }
    if (hOldPalette2 != NULL)
        {
        ::SelectPalette(hDC2, hOldPalette2, FALSE ); // Background ??
        }

    DeleteDC(hDC1);
    DeleteDC(hDC2);

    theUndo.BeginUndo( IDS_UNDO_PAINTING );

    theUndo.Insert((BYTE*)&hImgBitmap , sizeof (hImgBitmap));
    theUndo.InsertRect(rect);
    theUndo.InsertInt(sizeof (CRect) + sizeof (hImgBitmap));
    theUndo.InsertInt(A_ImageChange);
    theUndo.InsertPtr(this);
    theUndo.InsertByte(CUndoBmObj::opAction);

    theUndo.EndUndo();

    // NOTE: At this point, we could free the undo bitmaps, but instead
    // they are left around for next time...

    return TRUE;

LError:

    if (hImgBitmap != NULL)
        DeleteObject(hImgBitmap);

    if (hDC1 != NULL)
        DeleteDC(hDC1);

    if (hDC2 != NULL)
        DeleteDC(hDC2);

    // REVIEW: Since we couldn't allocate something here, there will
    // be no way to undo the last operation...  What should we do?
    // Chances are, the system is so low on memory, a message box
    // giving an option might even fail.
    //
    // For now, let's just beep to try to tell the user that whatever
    // just happend can't be undone.  Also, free the image sized bitmaps
    // so the system has a little free memory.

    CleanupImgUndo();

    MessageBeep(0);

    #ifdef _DEBUG
    TRACE(TEXT("Not enough memory to undo image change!\n"));
    #endif

    return FALSE;
    }


/*****************************************************************************/

BOOL CBitmapObj::SetIntProp(UINT nPropID, int val)
    {
    CWaitCursor waitCursor; // these all take awhile!

    switch (nPropID)
        {
        case P_Width:
            return SetSizeProp( P_Size, CSize( val, m_nHeight ) );
            break;

        case P_Height:
            return SetSizeProp( P_Size, CSize( m_nWidth, val ) );
            break;

        case P_Colors:
            if (CImgTool::GetCurrentID() == IDMB_PICKTOOL
            ||  CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL)
                {
                CommitSelection( TRUE );
                theImgBrush.m_pImg = NULL;
                }

            SetUndo( m_pImg );
            FinishUndo( NULL );

            // Perform the color-count conversion with DIBs
            DWORD dwSize;

            ::SelectObject( m_pImg->hDC, m_pImg->hBitmapOld );

            LPSTR lpDib = DibFromBitmap( m_pImg->hBitmap, BI_RGB,
                                         m_pImg->cPlanes * m_pImg->cBitCount,
                                         m_pImg->m_pPalette,
                                         NULL, dwSize );

            ::SelectObject( m_pImg->hDC, m_pImg->hBitmap );

            if (lpDib == NULL)
                {
                theApp.SetGdiEmergency();
                return FALSE;
                }

            // Make a new palette appropriate for this colors setting
            CPalette* pNewPalette = NULL;

            int iPlanes = (val? 1: ::GetDeviceCaps( m_pImg->hDC, PLANES    ));
            int iBitCnt = (val? 1: ::GetDeviceCaps( m_pImg->hDC, BITSPIXEL ));
            int iColors = iPlanes * iBitCnt;

            val = 3;

            if (theApp.m_bPaletted)
                switch (iColors)
                    {
                    case 1:
                        pNewPalette = GetStd2Palette();
                        break;

                    case 4:
                        pNewPalette = GetStd16Palette();
                        break;

                    case 8:
                        pNewPalette = GetStd256Palette();
                        break;
                    }

            switch (iColors)
                {
                case 8:
                    val = 2;
                    break;

                case 4:
                    val = 1;
                    break;

                case 1:
                    val = 0;
                    break;
                }

            HBITMAP hTmpBitmap = CreateBitmap( 1, 1, iPlanes, iBitCnt, NULL );
            HBITMAP hNewBitmap = CreateBitmap( m_pImg->cxWidth,
                                               m_pImg->cyHeight,
                                               iPlanes, iBitCnt, NULL );
            if (! hTmpBitmap || ! hNewBitmap)
                {
                FreeDib( lpDib );

                if (hTmpBitmap)
                    ::DeleteObject( hTmpBitmap );

                if (hNewBitmap)
                    ::DeleteObject( hNewBitmap );

                if (pNewPalette)
                    delete pNewPalette;

                theApp.SetGdiEmergency();
                return FALSE;
                }

            HPALETTE hPalOld = NULL;

            ::SelectObject( m_pImg->hDC, hTmpBitmap );

            if (pNewPalette)
                {
                hPalOld = ::SelectPalette( m_pImg->hDC, (HPALETTE)pNewPalette->m_hObject, FALSE );
                ::RealizePalette( m_pImg->hDC );
                }

            int iLinesDone = SetDIBits( m_pImg->hDC, hNewBitmap, 0,
                                        m_pImg->cyHeight,
                                        FindDIBBits( lpDib ),
                                        (LPBITMAPINFO)lpDib, DIB_RGB_COLORS );
            FreeDib( lpDib );

            if (iLinesDone != m_pImg->cyHeight)
                {
                ::SelectObject( m_pImg->hDC, m_pImg->hBitmap );

                if (hPalOld)
                    {
                    ::SelectPalette( m_pImg->hDC, hPalOld, FALSE );
                    ::RealizePalette( m_pImg->hDC );

                    delete pNewPalette;
                    }

                ::DeleteObject( hTmpBitmap );
                ::DeleteObject( hNewBitmap );

                theApp.SetGdiEmergency();

                return FALSE;
                }
            m_pImg->cPlanes   = iPlanes;
            m_pImg->cBitCount = iBitCnt;

            m_nColors = val;

            ::SelectObject( m_pImg->hDC, hNewBitmap );
            ::DeleteObject( m_pImg->hBitmap );

            m_pImg->hBitmap = hNewBitmap;

            if (m_pImg->m_pPalette)
                {
                if (! pNewPalette)
                    {
                    ::SelectPalette( m_pImg->hDC, m_pImg->m_hPalOld, FALSE );
                    m_pImg->m_hPalOld = NULL;
                    }
                delete m_pImg->m_pPalette;
                }

            m_pImg->m_pPalette = pNewPalette;
             theApp.m_pPalette = pNewPalette;

            ::DeleteObject( hTmpBitmap );

            DirtyImg( m_pImg );
            InvalImgRect( m_pImg, NULL );

            // The rubber-banding bitmap is now invalid...
            if (m_pImg == pRubberImg)
                {
                TRACE(TEXT("Clearing rubber\n"));
                pRubberImg = NULL;
                SetupRubber( m_pImg );
                }

            if (g_pColors)
                g_pColors->SetMono( ! m_nColors );

            InformDependants( P_Image );
            break;
        }

    m_bDirty = TRUE;

    return TRUE;
    }

/*****************************************************************************/

GPT CBitmapObj::GetIntProp(UINT nPropID, int& val)
    {
    switch (nPropID)
        {
        case P_Colors:
            val = m_nColors;
            return valid;
            break;

        case P_Image:
            val = NULL;
            return valid; // Must return now since this is a fake prop...
        }

    return invalid;
    }


/*****************************************************************************/

BOOL CBitmapObj::SetSizeProp(UINT nPropID, const CSize& val)
    {
    ASSERT(m_pImg != NULL);

    if ((CImgTool::GetCurrentID() == IDMB_PICKTOOL)
    ||  (CImgTool::GetCurrentID() == IDMB_PICKRGNTOOL))
        {
        CommitSelection(TRUE);
        theImgBrush.m_pImg = NULL;
        }

    switch (nPropID)
        {
        default:
            ASSERT(FALSE);

        case P_Size:
            if (val.cx == m_pImg->cxWidth && val.cy == m_pImg->cyHeight)
                return TRUE;

            if (val.cx < 1 || val.cy < 1)
                {
                CmpMessageBox(IDS_ERROR_BITMAPSIZE, AFX_IDS_APP_TITLE,
                              MB_OK | MB_ICONEXCLAMATION);
                return FALSE;
                }

            CWaitCursor waitCursor;
            BOOL bStretch = FALSE;

            CSize curSize;
            GetImgSize(m_pImg, curSize);

            bStretch = m_nShrink;

            SetUndo(m_pImg);
            CRect undoRect(0, 0, m_pImg->cxWidth, m_pImg->cyHeight);

            if (! SetImgSize(m_pImg, (CSize)val, bStretch))
                {
                theApp.SetMemoryEmergency();
                return FALSE;
                }

            FinishUndo(&undoRect);

            DirtyImg(m_pImg);

            pRubberImg = NULL;
            SetupRubber(m_pImg);

            if (theUndo.IsRecording())
                {
                theUndo.OnSetIntProp(this, P_Width, m_nWidth);
                theUndo.OnSetIntProp(this, P_Height, m_nHeight);
                }

            int nOldWidth = m_nWidth;
            int nOldHeight = m_nHeight;

            m_nWidth = val.cx;
            m_nHeight = val.cy;

            if (m_nWidth != nOldWidth)
                InformDependants(P_Width);

            if (m_nHeight != nOldHeight)
                InformDependants(P_Height);

            InformDependants(P_Image);

            break;
        }

    return TRUE;
    }

/*****************************************************************************/

BOOL CBitmapObj::SaveResource( BOOL bClear )
    {
    if (m_pImg == NULL)
        return TRUE;

    if (bClear)
        {
        if (m_lpvThing && ! m_pImg->bDirty && ! m_bDirty)
            return TRUE; // nothing to save

        m_bDirty |= m_pImg->bDirty;

        if (m_pImg == theImgBrush.m_pImg)
            theImgBrush.m_pImg = NULL;

        if (m_pImg == pRubberImg)
            pRubberImg = NULL;

        HideBrush();
        }

    DWORD dwStyle = BI_RGB;
    int   iColors = m_nColors;

    if (m_nSaveColors >= 0)
        {
        iColors = m_nSaveColors;
        m_nSaveColors = -1;
        }

    if (m_bCompressed)
        {
        switch (iColors)
            {
            case 1:
                dwStyle = BI_RLE4;
                break;

            case 2:
                dwStyle = BI_RLE8;
                break;
            }
        }

    switch (iColors)
        {
        case 0:
            iColors = 1;
            break;

        case 1:
            iColors = 4;
            break;

        case 2:
            iColors = 8;
            break;

        case 3:
            iColors = 24;
            break;

        default:
            iColors = 0;
            break;
        }

    HBITMAP hBitmap     = m_pImg->hBitmap;
    HBITMAP hMaskBitmap = NULL;
    BOOL    bNewBitmap  = FALSE;
    LPSTR   lpDIB;
    DWORD   dwSize;

        //BUGBUG-Icon support is not in application anymore, right?
   #ifdef ICO_SUPPORT
    if (IsSaveIcon())
        {
        // build a mask based on the current background color
        // and make sure the bitmap is the icon size
        bNewBitmap = SetupForIcon( hBitmap, hMaskBitmap );

        if (iColors > 4 || iColors < 1)
            iColors = 4;
        }
   #endif

    ::SelectObject( m_pImg->hDC, m_pImg->hBitmapOld );

    lpDIB = DibFromBitmap( hBitmap, dwStyle, (WORD)iColors, theApp.m_pPalette,
                       hMaskBitmap, dwSize );
    ::SelectObject( m_pImg->hDC, m_pImg->hBitmap );

    if (bNewBitmap)
        {
        ::DeleteObject(     hBitmap );
        ::DeleteObject( hMaskBitmap );
        }

    if (lpDIB == NULL)
        {
        theApp.SetMemoryEmergency();

        return FALSE;
        }

    if (m_lpvThing != NULL)
        Free();

    // We packed the DIB, so the offset will always be right after the palette,
    // which is implied by this being 0
    m_dwOffBits = 0;
    m_lpvThing = (LPVOID)lpDIB;
    m_lMemSize = dwSize;

    if (bClear)
        m_pImg->bDirty = FALSE;

    return TRUE;
    }

/*****************************************************************************/

//BUGBUG-Icon support is not in application anymore, right?
BOOL CBitmapObj::SetupForIcon( HBITMAP& hBitmap, HBITMAP& hMaskBitmap )
    {
    CDC       dcIcon;
    CDC       dcMask;
    CBitmap   bmIcon;
    CBitmap   bmMask;
    CDC*      pdcBitmap = CDC::FromHandle( m_pImg->hDC );
    CSize     sizeIcon( ::GetSystemMetrics( SM_CXICON ),
                        ::GetSystemMetrics( SM_CYICON ) );
    BOOL      bNewBitmap = FALSE;

    if (dcIcon.CreateCompatibleDC( pdcBitmap )
    &&  dcMask.CreateCompatibleDC( pdcBitmap )
    &&  bmIcon.CreateCompatibleBitmap( pdcBitmap, sizeIcon.cx, sizeIcon.cy )
    &&  bmMask.CreateBitmap( sizeIcon.cx, sizeIcon.cy, 1, 1, NULL ))
        {
        CPalette* ppalOld = NULL;
        CBitmap*  pbmOldIcon = dcIcon.SelectObject( &bmIcon );
        CBitmap*  pbmOldMask = dcMask.SelectObject( &bmMask );

        if (theApp.m_pPalette)
            {
            ppalOld = dcIcon.SelectPalette( theApp.m_pPalette, FALSE );
            dcIcon.RealizePalette();
            }
        dcIcon.PatBlt( 0, 0, sizeIcon.cx, sizeIcon.cy, WHITENESS );

        CBrush brBackGround( crRight );

        if (brBackGround.GetSafeHandle() != NULL)
            {
            CRect rect( 0, 0, sizeIcon.cx, sizeIcon.cy );

            dcIcon.FillRect( &rect, &brBackGround );

            brBackGround.DeleteObject();
            }
        int iWidth  = min( sizeIcon.cx, m_pImg->cxWidth );
        int iHeight = min( sizeIcon.cy, m_pImg->cyHeight );

        dcIcon.BitBlt( 0, 0, iWidth, iHeight, pdcBitmap, 0, 0, SRCCOPY );

        COLORREF oldBkColor = dcIcon.SetBkColor( crRight );

        dcMask.BitBlt( 0, 0, sizeIcon.cx, sizeIcon.cy, &dcIcon, 0, 0, SRCCOPY );

        COLORREF cRefFGColorOld = dcMask.SetTextColor( RGB(   0,   0,   0 ) );
        COLORREF cRefBKColorOld = dcMask.SetBkColor  ( RGB( 255, 255, 255 ) );

        dcIcon.BitBlt( 0, 0, sizeIcon.cx, sizeIcon.cy, &dcMask, 0, 0, DSna );

        dcMask.SetTextColor( cRefFGColorOld );
        dcMask.SetBkColor  ( cRefBKColorOld );
        dcIcon.SetBkColor  ( oldBkColor );

        if (ppalOld != NULL)
            dcIcon.SelectPalette( ppalOld, FALSE );

        if (pbmOldIcon != NULL)
            dcIcon.SelectObject( pbmOldIcon );

        if (pbmOldMask != NULL)
            dcMask.SelectObject( pbmOldMask );

            hBitmap = (HBITMAP)bmIcon.Detach();
        hMaskBitmap = (HBITMAP)bmMask.Detach();
         bNewBitmap = TRUE;
        }

    if (dcIcon.GetSafeHdc() != NULL)
        dcIcon.DeleteDC();

    if (dcMask.GetSafeHdc() != NULL)
        dcMask.DeleteDC();

    if (bmIcon.GetSafeHandle() != NULL)
        bmIcon.DeleteObject();

    if (bmMask.GetSafeHandle() != NULL)
        bmMask.DeleteObject();

    return bNewBitmap;
    }

/*****************************************************************************/
