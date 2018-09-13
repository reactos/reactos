//-------------------------------------------------------------------------//
#include "pch.h"

#if 0
#   ifdef _X86_
#       define _USE_OFFICE_TEXT_
#       define _USE_OFFICE_TIFF_
#   endif _X86_
#else 0
#   define _USE_MSFAX_TEXT_
#   define _USE_MSFAX_TIFF_
#endif 0

#include "imageprop.h"
#include "tiff.h"

// {E2300424-8950-11d2-BE79-00A0C9A83DA1}
static const GUID FMTID_FaxSummaryInformation = 
    { 0xe2300424, 0x8950, 0x11d2, { 0xbe, 0x79, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

#ifdef _X86_

#define IFLSUCCEEDED( iflErr )  (iflErr==IFLERR_NONE)
#define IFLFAILED( iflErr )     (iflErr!=IFLERR_NONE)

//-------------------------------------------------------------------------//
BOOL InitIflProperties( IFLPROPERTIES* pIP )
{
    if( !pIP ) return FALSE ;

    memset( pIP, 0, sizeof(*pIP) ) ;
    pIP->cbStruct = sizeof(*pIP) ;      
    
    pIP->mask       = 0L ;
    pIP->type       = IFLT_UNKNOWN  ;
    pIP->imageclass = IFLCL_NONE ;
    
    /*
    pIP->width      = 0L ;
    pIP->height     = 0L ;
    pIP->linecount  = 0L ;
    pIP->bpp        = 0L ;
    pIP->bpc        = 0L ;
    pIP->dpm        = 0L ;
    pIP->imagecount = 0L ;
    */

    pIP->tilefmt    = IFLTF_NONE ;
    pIP->lineseq    = IFLSEQ_TOPDOWN ;
    pIP->compression= IFLCOMP_NONE ;

    return TRUE ;
}

//-------------------------------------------------------------------------//
BOOL ClearIflProperties( IFLPROPERTIES* pIP )
{
    if( !pIP ) return FALSE ;
    return InitIflProperties( pIP ) ;   
}

#ifdef _USE_OFFICE_TEXT_
//-------------------------------------------------------------------------//
//  Retrieves text metadata from image file using office graphics import filters.
BOOL GetImageDescription( IFLHANDLE iflHandle, IFLDESC iflSupported, IFLDESC iflDesc, 
                          LPSTR pszBuf, int cchBuf )
{
    ASSERT( pszBuf ) ;
    *pszBuf = 0 ;

    CHAR*    pszDesc = NULL ;
    IFLERROR iflErr = iflGetDesc( iflHandle, iflDesc, &pszDesc ) ;

    if( IFLERR_NONE == iflErr )
    {
        lstrcpynA( pszBuf, pszDesc, cchBuf ) ;
        return TRUE ;
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Acquires text metadata properties using office graphics import filters
static HRESULT AcquireTextProperties( IFLHANDLE iflHandle, IFLPROPERTIES* pIP, ULONG* pcProps )
{
    IFLDESC     iflSupported ;
    IFLERROR    iflErr    = IFLERR_NONE ;
    CHAR        szText[MAX_METADATA_TEXT] ;
    HRESULT     hr ;
    
    if( (iflErr = iflSupportedDesc( iflHandle, &iflSupported )) == IFLERR_NONE )
    {
        USES_CONVERSION ;
        if( (pIP->textmask & ITPF_TITLE) == 0 &&
                GetImageDescription( iflHandle, iflSupported, IFLDESC_DOCUMENTNAME, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szTitle, A2W(szText), ARRAYSIZE(pIP->szTitle) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_TITLE ;
        }

        if( (pIP->textmask & ITPF_AUTHOR) == 0 &&
            GetImageDescription( iflHandle, iflSupported, IFLDESC_ARTISTNAME, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szAuthor, A2W(szText), ARRAYSIZE(pIP->szAuthor) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_AUTHOR ;
        }

        if( (pIP->textmask & ITPF_DESCRIPTION) == 0 &&
            GetImageDescription( iflHandle, iflSupported, IFLDESC_DESCRIPTION, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szDescription, A2W(szText), ARRAYSIZE(pIP->szDescription) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_DESCRIPTION ;
        }

        if( (pIP->textmask & ITPF_COMMENTS) == 0 &&
            GetImageDescription( iflHandle, iflSupported, IFLDESC_COMMENT, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szComments, A2W(szText), ARRAYSIZE(pIP->szComments) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_COMMENTS ;
        }

        if( (pIP->textmask & ITPF_SOFTWARE) == 0 &&
            GetImageDescription( iflHandle, iflSupported, IFLDESC_SOFTWARENAME, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szSoftware, A2W(szText), ARRAYSIZE(pIP->szSoftware) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_SOFTWARE ;
        }

        if( (pIP->textmask & ITPF_COPYRIGHT) == 0 &&
            GetImageDescription( iflHandle, iflSupported, IFLDESC_COPYRIGHT, 
                                 szText, ARRAYSIZE(szText) ) )
        {
            lstrcpynW( pIP->szCopyright, A2W(szText), ARRAYSIZE(pIP->szCopyright) ) ;
            (*pcProps)++ ;
            pIP->mask |= IPF_TEXT ;
            pIP->textmask |= ITPF_COPYRIGHT ;
        }
        hr = (pIP->mask & IPF_TEXT) != 0 ? S_OK : S_FALSE ;
    }
    else
        hr = IflErrorToHResult( iflErr, FALSE ) ;

    return hr ;
}
#endif _USE_OFFICE_TEXT_

#ifdef  _USE_OFFICE_TIFF_
//-------------------------------------------------------------------------//
//  retrieves a tiff metadata string value using office graphics import filters.
BOOL GetTiffTagString( IN IFLHANDLE iflHandle, IN WORD tagID, OUT LPWSTR pszBuf, IN OUT DWORD* pcchBuf )
{
    IFLERROR iflErr ;
    TIFF_TAG tag ;
    BOOL     bRet = FALSE ;
    ASSERT( pszBuf ) ;
    ASSERT( pcchBuf && *pcchBuf ) ;
    
    if( (iflErr = iflControl( iflHandle, IFLCMD_TIFFTAG, tagID, 0L, &tag )) == IFLERR_NONE )
    {
        if( TIFF_ASCII == tag.DataType )
        {
            if( tag.DataCount <= *pcchBuf )
            {
                CHAR* pszBufA = new CHAR[tag.DataCount] ;
                if( pszBufA )
                {
                    if( iflControl( iflHandle, IFLCMD_TIFFTAGDATA, tagID, 0L, pszBufA ) == IFLERR_NONE )
                    {
                        USES_CONVERSION ;
                        lstrcpynW( pszBuf, A2W( pszBufA ), *pcchBuf ) ;
                        bRet = TRUE ;
                    }
                    delete [] pszBufA ;
                }
            }
            *pcchBuf = tag.DataCount ;
        }
    }
    return bRet ;
}

//-------------------------------------------------------------------------//
//  retrieves a tiff metadata time value using office graphics import filters.
BOOL GetTiffTagTime( IN IFLHANDLE iflHandle, IN WORD tagID, OUT FILETIME* pft )
{
    IFLERROR iflErr ;
    TIFF_TAG tag ;
    ASSERT( pszBuf ) ;
    ASSERT( pcchBuf && *pcchBuf ) ;
    
    if( (iflErr = iflControl( iflHandle, IFLCMD_TIFFTAG, tagID, 0L, &tag )) == IFLERR_NONE )
    {
        if( TIFF_SRATIONAL == tag.DataType )
        {
            BYTE ull[256] ;
            ZeroMemory( ull, ARRAYSIZE(ull) ) ;
            if( iflControl( iflHandle, IFLCMD_TIFFTAGDATA, tagID, 0L, ull ) == IFLERR_NONE )
            {
                *pft = *((FILETIME*)ull) ;
                //pft->dwLowDateTime  = *((DWORD*)&ull[0]) ;
                //pft->dwHighDateTime = *((DWORD*)&ull[4]) ;
                return TRUE ;
            }
        }
    }
    return FALSE ;
}

//-------------------------------------------------------------------------//
//  Acquires TIFF fax properties using office TIFF import filter
static HRESULT AcquireFaxProperties( IFLHANDLE iflHandle, IFLPROPERTIES* pIP, ULONG* pcProps )
{
    ASSERT( iflHandle ) ;
    ASSERT( pIP ) ;
    ASSERT( sizeof(*pIP) == pIP->cbStruct ) ;
    ULONG cch ;
    
    cch = ARRAYSIZE(pIP->szFaxRecipName) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_RECIP_NAME, pIP->szFaxRecipName, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_RECIPIENTNAME ;
    }

    cch = ARRAYSIZE(pIP->szFaxRecipNumber) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_RECIP_NUMBER, pIP->szFaxRecipNumber, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_RECIPIENTNUMBER ;
    }

    cch = ARRAYSIZE(pIP->szFaxSenderName) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_SENDER_NAME, pIP->szFaxSenderName, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_SENDERNAME ;
    }

    cch = ARRAYSIZE(pIP->szFaxRouting) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_ROUTING, pIP->szFaxRouting, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_ROUTING ;
    }

    cch = ARRAYSIZE(pIP->szFaxCallerID) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_CALLERID, pIP->szFaxCallerID, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_CALLERID ;
    }

    cch = ARRAYSIZE(pIP->szFaxTSID) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_TSID, pIP->szFaxTSID, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_TSID ;
    }

    cch = ARRAYSIZE(pIP->szFaxCSID) ;
    if( GetTiffTagString( iflHandle, TIFFTAG_CSID, pIP->szFaxCSID, &cch ) )
    {
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_CSID ;
    }

    FILETIME ft ;
    if( GetTiffTagTime( iflHandle, TIFFTAG_FAX_TIME, &ft ) )
    {
        pIP->ftFaxTime = ft ;
        (*pcProps)++ ;
        pIP->mask |= IPF_FAX ;
        pIP->faxmask |= IFPF_FAXTIME ;
    }

    return HasFaxProperties( pIP ) ? S_OK : S_FALSE ;
}

#endif  _USE_OFFICE_TIFF_



#if defined (_USE_MSFAX_TIFF_) || defined(_USE_MSFAX_TEXT_)
//-------------------------------------------------------------------------//
#include "faxcom.h"
static const IID IID_IFaxTiff  = { 0xb19bb45f, 0xb91c, 0x11d1, {0x83,0xe1,0x00,0xc0,0x4f,0xb6,0xe9,0x84} };
static const IID CLSID_FaxTiff = { 0x87099231, 0xC7AF, 0x11D0, {0xB2,0x25,0x00,0xC0,0x4F,0xB6,0xC2,0xF5 } } ;
#endif //(_USE_MSFAX_TIFF_) || defined(_USE_MSFAX_TEXT_)

//-------------------------------------------------------------------------//
//  Acquires metadata properties using MSFax FaxCom object
#ifdef _USE_MSFAX_TEXT_
static HRESULT AcquireTextProperties( const WCHAR* pwszFileName, IFLPROPERTIES* pIP, ULONG* pcProps )
{
    HRESULT hr = S_FALSE ;
    ASSERT( pIP ) ;
    ASSERT( sizeof(*pIP) == pIP->cbStruct ) ;
    
    pIP->mask &= ~IPF_TEXT ;

    IFaxTiff* pft ;
    if( SUCCEEDED( (hr = CoCreateInstance( CLSID_FaxTiff, NULL, CLSCTX_INPROC_SERVER, IID_IFaxTiff, (void**)&pft )) ) )
    {
        BSTR    bstrImage = SysAllocString( pwszFileName ) ;

        //  CR: ActiveFax\faxtiff.cpp: CFaxTiff::put_Image should free bstrImage.
        if( SUCCEEDED( pft->put_Image( bstrImage ) ) )
        {
            VARIANT var ;
            BSTR    bstr ;

            if( S_OK == pft->get_TiffTagString( TIFFTAG_DOCUMENTNAME, &bstr ) )
            {
                lstrcpynW( pIP->szTitle, bstr, ARRAYSIZE(pIP->szTitle) ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_TEXT ;
                pIP->textmask |= ITPF_TITLE ;
            }

            if( S_OK == pft->get_TiffTagString( TIFFTAG_ARTIST, &bstr ) )
            {
                lstrcpynW( pIP->szAuthor, bstr, ARRAYSIZE(pIP->szAuthor) ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_TEXT ;
                pIP->textmask |= ITPF_AUTHOR ;
            }

            if( S_OK == pft->get_TiffTagString( TIFFTAG_IMAGEDESCRIPTION, &bstr ) )
            {
                lstrcpynW( pIP->szDescription, bstr, ARRAYSIZE(pIP->szDescription) ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_TEXT ;
                pIP->textmask |= ITPF_DESCRIPTION ;
            }

            if( S_OK == pft->get_TiffTagString( TIFFTAG_SOFTWARE, &bstr ) )
            {
                lstrcpynW( pIP->szSoftware, bstr, ARRAYSIZE(pIP->szSoftware) ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_TEXT ;
                pIP->textmask |= ITPF_SOFTWARE ;
            }

            if( S_OK == pft->get_TiffTagString( TIFFTAG_COPYRIGHT, &bstr ) )
            {
                lstrcpynW( pIP->szCopyright, bstr, ARRAYSIZE(pIP->szCopyright) ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_TEXT ;
                pIP->textmask |= ITPF_COPYRIGHT ;
            }

        }

        hr = (pIP->mask & IPF_TEXT) != 0 ? S_OK : S_FALSE ;
        pft->Release() ;
    }

    return hr ;
}
#endif //_USE_MSFAX_TEXT_

//-------------------------------------------------------------------------//
//  Acquires TIFF fax properties using MSFax FaxCom object
#ifdef _USE_MSFAX_TIFF_
HRESULT AcquireFaxProperties( const WCHAR* pwszFileName, IFLPROPERTIES* pIP, ULONG* pcProps )
{
    HRESULT hr = S_FALSE ;
    ASSERT( pIP ) ;
    ASSERT( sizeof(*pIP) == pIP->cbStruct ) ;
    
    IFaxTiff* pft ;
    if( SUCCEEDED( (hr = CoCreateInstance( CLSID_FaxTiff, NULL, CLSCTX_INPROC_SERVER, IID_IFaxTiff, (void**)&pft )) ) )
    {
        BSTR    bstrImage = SysAllocString( pwszFileName ) ;

        //  CR: ActiveFax\faxtiff.cpp: CFaxTiff::put_Image should free bstrImage.
        if( SUCCEEDED( pft->put_Image( bstrImage ) ) )
        {
            VARIANT var ;
            BSTR    bstr ;

            if( S_OK == (hr = pft->get_RecipientName( &bstr )) )
            {
                lstrcpyW( pIP->szFaxRecipName, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_RECIPIENTNAME ;
            }

            if( S_OK == (hr = pft->get_SenderName( &bstr )) )
            {
                lstrcpyW( pIP->szFaxSenderName, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_SENDERNAME ;
            }

            if( S_OK == (hr = pft->get_Routing( &bstr )) )
            {
                lstrcpyW( pIP->szFaxRouting, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_ROUTING ;
            }

            if( S_OK == (hr = pft->get_CallerId( &bstr )) )
            {
                lstrcpyW( pIP->szFaxCallerID, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_CALLERID ;
            }

            if( S_OK == (hr = pft->get_Csid( &bstr )) )
            {
                lstrcpyW( pIP->szFaxCSID, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_CSID ;
            }

            if( S_OK == (hr = pft->get_Tsid( &bstr )) )
            {
                lstrcpyW( pIP->szFaxTSID, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_TSID ;
            }

            if( S_OK == (hr = pft->get_RecipientNumber( &bstr )) )
            {
                lstrcpyW( pIP->szFaxRecipNumber, bstr ) ;
                SysFreeString( bstr ) ;
                (*pcProps)++ ;
                pIP->mask |= IPF_FAX ;
                pIP->faxmask |= IFPF_RECIPIENTNUMBER ;
            }

            if( S_OK == (hr = pft->get_RawReceiveTime( &var )) )
            {
                FILETIME    ft, ftLocal ;
                SYSTEMTIME  st ;
                BOOL        bConverted = FALSE ;

                if( VT_CY == var.vt )
                {
                    pIP->ftFaxTime = *((FILETIME*)&var.cyVal) ;
                    bConverted = TRUE ;
                }
                else if( VT_DATE == var.vt )
                {
                    //  do the conversion dance
                    SYSTEMTIME st ;
                    bConverted = VariantTimeToSystemTime( var.date, &st ) &&
                                 SystemTimeToFileTime( &st, &pIP->ftFaxTime ) ;
                }
                if( bConverted )
                {
                    (*pcProps)++ ;
                    pIP->mask |= IPF_FAX ;
                    pIP->faxmask |= IFPF_FAXTIME ;
                }
            }
        }

        pft->Release() ;
        hr = HasFaxProperties( pIP ) ? S_OK : S_FALSE ;
    }
    
    return hr ;
}
#endif //_USE_MSFAX_TIFF_

#if defined(_X86_)  // as long as we're using office graphics filters, this is X86 only.
//-------------------------------------------------------------------------//
HRESULT AcquireImageProperties( const WCHAR* pwszFileName, IFLPROPERTIES* pIP, ULONG* pcProps )
{
    IFLHANDLE iflHandle = NULL ;
    IFLTYPE   iflType   = IFLT_UNKNOWN ;
    IFLERROR  iflErr    = IFLERR_NONE ;
    ULONG     cProps    = 0 ;
    ULONG     dwRequested    = 0 ;
    USES_CONVERSION ;

    //  Validate args
    if( !( pwszFileName && *pwszFileName && pIP && pIP->cbStruct==sizeof(*pIP) ) )
        return E_INVALIDARG ;
    if( pcProps ) 
        *pcProps = 0 ;
    else
        pcProps = &cProps ;

    //  Save request bits and clear for output.
    dwRequested = pIP->mask ;
    pIP->mask = 0 ;

    //  Determine image type
    if( IFLFAILED( (iflErr = iflImageType( W2A( pwszFileName ), &pIP->type )) ) )
        return IflErrorToHResult( iflErr, FALSE ) ;

    //  Not an image; abort.
    if( pIP->type == IFLT_UNKNOWN )
        return E_ABORT ;
    (*pcProps)++ ;
    pIP->mask |= IPF_TYPE ;
    
    //  Allocate memory block for image data
    if( (iflHandle = iflCreateReadHandle( pIP->type ))==NULL )
        return E_OUTOFMEMORY ;

    //  Open the file
    if( (iflErr = iflOpen( iflHandle, W2A( pwszFileName ), IFLM_READ )) != IFLERR_NONE )
    {
        iflFreeHandle( iflHandle ) ;
        return IflErrorToHResult( iflErr, FALSE ) ;
    }

    if( dwRequested & IPF_IMAGE )
    {
        //  Image type
        if( pIP->type != IFLT_UNKNOWN )
        {
            //  BMP version
            if( pIP->type == IFLT_BMP &&
                iflControl( iflHandle, IFLCMD_BMP_VERSION, 0, 0L, 
                            &pIP->bmpver ) == IFLERR_NONE )
            {
                (*pcProps)++ ;
                pIP->mask |= IPF_BMPVER ;
            }
            else
            {
                pIP->bmpver = (IFLBMPVERSION)0 ;
            }
        
            //  Image count
            if( iflControl( iflHandle, IFLCMD_GETNUMIMAGES, (SHORT)pIP->type, 0L,
                            &pIP->imagecount ) != IFLERR_NONE )
                pIP->imagecount = 0 ;
            else
            {
                (*pcProps)++ ;
                pIP->mask |= IPF_IMAGECOUNT ;
            }
        }

        //  Image class
        if( (pIP->imageclass = iflGetClass( iflHandle )) != IFLCL_NONE )
        {
            (*pcProps)++ ;
            pIP->mask |= IPF_CLASS ;
        }

        //  Width
        pIP->cx = iflGetWidth( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_CX ;

        //  Height
        pIP->cy = iflGetHeight( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_CY ;

        //  Horiz resolution
        if( iflControl( iflHandle, IFLCMD_RESOLUTION, 1, 0L, &pIP->dpmX ) != IFLERR_NONE )
            pIP->dpmX = 0 ;
        else
        {
            (*pcProps)++ ;
            pIP->mask |= IPF_DPMX ;
        }

        //  Vert resolution
        if( iflControl( iflHandle, IFLCMD_RESOLUTION, 2, 0L, &pIP->dpmY ) != IFLERR_NONE )
            pIP->dpmY = 0 ;
        else
        {
            (*pcProps)++ ;
            pIP->mask |= IPF_DPMY ;
        }

        //  Gamma correction value
        if( (iflErr = iflControl( iflHandle, IFLCMD_GAMMA_VALUE, 0, 0L, &pIP->gamma )) != IFLERR_NONE )
            pIP->gamma = 0 ;
        else
        {
            (*pcProps)++ ;
            pIP->mask |= IPF_GAMMA ;
        }

        //  Raster linecount
        pIP->linecount = iflGetRasterLineCount( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_LINECOUNT ;

        //  Raster line sequence
        pIP->lineseq = iflGetSequence( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_LINESEQ ;

        //  Compression mode
        pIP->compression = iflGetCompression( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_COMPRESSION ;
    
        //  Bit depth
        pIP->bpp = iflGetBitsPerPixel( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_BPP ;

        //  Bits per channel
        pIP->bpc = iflGetBitsPerChannel( iflHandle ) ;
        (*pcProps)++ ;
        pIP->mask |= IPF_BPC ;

        if( iflControl( iflHandle, IFLCMD_TILEFORMAT, 0, 0L, &pIP->tilefmt )!=IFLERR_NONE )
            pIP->tilefmt = IFLTF_NONE ;
        else
        {
            (*pcProps)++ ;
            pIP->mask |= IPF_TILEFMT ;
        }
    }

    if( dwRequested & IPF_FAX )
    {
        if( IFLT_TIFF == pIP->type )
        {
#ifdef  _USE_MSFAX_TIFF_
            AcquireFaxProperties( pwszFileName, pIP, pcProps ) ;
#endif  _USE_MSFAX_TIFF_

#ifdef _USE_OFFICE_TIFF_
            AcquireFaxProperties( iflHandle, pIP, pcProps ) ;
#endif _USE_OFFICE_TIFF_
        }
    }

    if( dwRequested & IPF_TEXT )
    {
        if( IFLT_TIFF == pIP->type )
        {
#ifdef  _USE_MSFAX_TEXT_
            AcquireTextProperties( pwszFileName, pIP, pcProps ) ;
#endif  _USE_MSFAX_TEXT_
        }

#ifdef _X86_
#   ifdef _USE_OFFICE_TEXT_
        //  Fill in any holes using office filters.
        AcquireTextProperties( iflHandle, pIP, pcProps ) ;
#   endif 
#endif _X86_
    }


    //  Clean up.
    iflClose( iflHandle ) ;
    iflFreeHandle( iflHandle ) ;

    return S_OK ;
}
#endif _X86_

//-------------------------------------------------------------------------//
#if defined(_X86_)
BOOL HasFaxProperties( IFLPROPERTIES* pIP )
{
    return pIP && 
           pIP->cbStruct == sizeof(*pIP) && 
           (pIP->mask & IPF_FAX) !=0 && 
           pIP->faxmask != 0 ;

}
#endif //defined(_X86_)

//-------------------------------------------------------------------------//
#if defined(_X86_)
BOOL HasImageProperties( IFLPROPERTIES* pIP )
{
    return pIP && 
           pIP->cbStruct == sizeof(*pIP) && 
           (pIP->mask & IPF_IMAGE) !=0 ;
}
#endif //defined(_X86_)


//-------------------------------------------------------------------------//
HRESULT IflErrorToHResult( IFLERROR iflError, BOOL bExtended )
{
    if( bExtended )
    {

    }
    else
    {
        switch( iflError )
        {
            case IFLERR_NONE:           return S_OK ;
            case IFLERR_HANDLELIMIT:    return E_HANDLE ;
            case IFLERR_PARAMETER:      return E_INVALIDARG ;
            case IFLERR_NOTSUPPORTED:   return E_NOTIMPL ;
            case IFLERR_NOTAVAILABLE:   return E_NOTIMPL ;
            case IFLERR_MEMORY:         return E_OUTOFMEMORY ;
            
            case IFLERR_IMAGE:          
            case IFLERR_HEADER:         
            case IFLERR_IO_OPEN:        
            case IFLERR_IO_CLOSE:
            case IFLERR_IO_READ:
            case IFLERR_IO_WRITE:
            case IFLERR_IO_SEEK:
            default:
                break ;
        }
    }
    return E_FAIL ;
}

//-------------------------------------------------------------------------//
//  BUGBUG: image.lib calls strcmpi, which is obsolete, and no longer
//          in the runtime library.  So we'll provide a shim to its replacement...
int __cdecl strcmpi( const char *string1, const char *string2 )
{
    return _stricmp( string1, string2 ) ;
}

#endif _X86_

