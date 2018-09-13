/******************************Module*Header*******************************\
* Module Name: tnail.cpp
*
* Thumbnail API interface
*
* Created: 17-Oct-1997
* Author: Ori Gershony [orig]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "stdafx.h"
#include "jinclude.h"
#include "jpeglib.h"
#include "jversion.h"
#include "jerror.h"
#include "ddraw.h"
#include "jpegapi.h"
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "ImgUtil.h"
#include "ocmm.h"
#include "ctngen.h"
#include "tnail.h"


int vfMMXMachine=FALSE;

//
// WARNING:  for large Thumbnail_X and Thumbnail_Y values, we will also
// need to increase INPUT_vBUF_SIZE and OUTPUT_BUF_SIZE in jdatasrc.cpp and
// jdatadst.cpp (lovely jpeg decompression code...).  Also need to modify our
// own JPEG_BUFFER_SIZE in ctngen.cxx.
//
#define DEFAULT_THUMBNAIL_QUALITY 50
#define DEFAULT_THUMBNAIL_X       120
#define DEFAULT_THUMBNAIL_Y       120



/******************************Public*Routine******************************\
* TN_Initialize
*
*   Initialize the thumbnail state
*
* Arguments:
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::TN_Initialize(
    VOID
    )
{
    HRESULT retVal;

    //
    // Initialize class globals
    //
    Thumbnail_Quality = DEFAULT_THUMBNAIL_QUALITY;
    Thumbnail_X       = DEFAULT_THUMBNAIL_X;
    Thumbnail_Y       = DEFAULT_THUMBNAIL_Y;

    m_hJpegC = m_hJpegD = NULL;
    m_JPEGheaderSize = 0;

    m_imageData.hbp = NULL;
    m_imageData.x   = 0;
    m_imageData.y   = 0;

    //
    // Find out if processor supports MMX technology and whether it's available
    //

#if defined(_X86_)
//    vfMMXMachine = IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE);
#endif

    //
    // Proceed with initialization of OLE and Direct Draw
    //
    
    if ((retVal=decoderInitialize()) != ERROR_SUCCESS)
    {
        return retVal;
    }

    //
    // Now for JPEG initialization
    //
    
    if (! (m_JPEGheader = (BYTE *) LocalAlloc (0, 1024)))
    {
        decoderUninitialize();
        return E_OUTOFMEMORY;
    }
    
    JPEGCompressHeader (m_JPEGheader, 
        Thumbnail_Quality, 
        &m_JPEGheaderSize, 
        &m_hJpegC,
        JCS_RGBA
        );

    JPEGDecompressHeader(m_JPEGheader,
        &m_hJpegD,
        m_JPEGheaderSize
        );
        
    return ERROR_SUCCESS;
}



/******************************Public*Routine******************************\
* TN_Uninitialize
*
*   Cleans up the thumbnail state
*
* Arguments:
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::TN_Uninitialize(
    VOID
    )
{
    DestroyJPEGCompressHeader  (m_hJpegC);
    DestroyJPEGDecompressHeader(m_hJpegD);
    LocalFree (m_JPEGheader);

    decoderUninitialize();
    
    return ERROR_SUCCESS;
}



/******************************Public*Routine******************************\
* deliverDecompressedImage
*
*     This function accepts a decompressed image from the filter, reduces
*     it to a thumbnail, and stores it in compressed JPEG format. 
*
* Arguments:
*
*     hbp -- The bitmap which stores the decompressed image.  We are responsible
*            for deleting it
*     x   -- The length of the image
*     y   -- The width of the image
*
* Return Value:
*
*    none
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::deliverDecompressedImage(
    HBITMAP hbp,
    ULONG x,
    ULONG y
    )
{
    m_imageData.hbp = hbp;
    m_imageData.x = x;
    m_imageData.y = y;

    return S_OK;
}





/******************************Public*Routine******************************\
* createThumbnailFromImage
*
*     This function takes a bitmap and exchanges it with that of a smaller
*     thumbnail.
*
* Arguments:
*
*     phbp -- points to a bitmap that will be freed and replaced with a 
*         smaller bitmap of the same image
*     x -- width of image
*     y -- height of image
*
*
* Return Value:
*
*    none
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::createThumbnailFromImage(
    HBITMAP *phbp,
    ULONG *px,
    ULONG *py,
    INT **ppvbits
    )
{
    HBITMAP hbp = *phbp;

    double x_reduction, y_reduction, reduction;
    ULONG final_x, final_y;

    //
    // Compute dimensions of thumbnail
    //

    x_reduction = ((double) *px) / ((double) Thumbnail_X);
    y_reduction = ((double) *py) / ((double) Thumbnail_Y);
    if (x_reduction > y_reduction)
    {
        reduction = x_reduction;
    }
    else
    {
        reduction = y_reduction;
    }
    if (reduction < (double) 1)
    {
        reduction = 1; // Already the right size;
    }
    final_x = (ULONG) (((double) *px)/reduction);
    final_y = (ULONG) (((double) *py)/reduction);

    //
    // Now Allocate buffer for thumbnail
    //

    PBITMAPINFO pbmi = (PBITMAPINFO) LocalAlloc(0, sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
    if (!pbmi)
    {
        return E_OUTOFMEMORY;
    }
    pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biWidth           = final_x;
    pbmi->bmiHeader.biHeight          = final_y;
    pbmi->bmiHeader.biPlanes          = 1;
    pbmi->bmiHeader.biBitCount        = 32;
    pbmi->bmiHeader.biCompression     = BI_RGB;
    pbmi->bmiHeader.biSizeImage       = 0;
    pbmi->bmiHeader.biXPelsPerMeter   = 0;
    pbmi->bmiHeader.biYPelsPerMeter   = 0;
    pbmi->bmiHeader.biClrUsed         = 0;
    pbmi->bmiHeader.biClrImportant    = 0;
    HBITMAP hReducedBitmap = CreateDIBSection (NULL, pbmi, DIB_RGB_COLORS, (VOID **) ppvbits, NULL, 0);
    if (!hReducedBitmap)
    {
        LocalFree(pbmi);
        return E_OUTOFMEMORY;
    }

    //
    // Copy image into thumbnail buffer
    //

    HDC hdc1, hdc2;
    hdc1 = CreateCompatibleDC(NULL);
    hdc2 = CreateCompatibleDC(NULL);
    SelectObject(hdc1, hbp);
    SelectObject(hdc2, hReducedBitmap);
    SetStretchBltMode (hdc2, HALFTONE);
    StretchBlt (hdc2, 0, 0, final_x, final_y, hdc1, 0, 0, *px, *py, SRCCOPY);
    DeleteDC(hdc1);
    DeleteDC(hdc2);
    
    LocalFree(pbmi);
    DeleteObject(hbp);
    *phbp = hReducedBitmap;
    *px = final_x;
    *py = final_y;
    
    return ERROR_SUCCESS;
}





/******************************Public*Routine******************************\
* createCompressedThumbnail
*
*     This function accepts a decompressed image from the filter, reduces
*     it to a thumbnail, and stores it in compressed JPEG format. 
*
* Arguments:
*
*     hbp -- The bitmap which stores the decompressed image.  We are responsible
*            for deleting it
*     x   -- The length of the image
*     y   -- The width of the image
*     pJPEGBuf -- The buffer where we write the compressed
*            JPEG thumbnail
*     pulJPEGBufSize -- The number of bytes written to
*            m_pJPEGBuf 
*     mtime - modification time of the file
*
*
* Return Value:
*
*    none
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::createCompressedThumbnail(
    HBITMAP hbp,
    ULONG x,
    ULONG y,
    CHAR *pJPEGBuf,
    ULONG *pulJPEGBufSize,
    FILETIME mtime    
    )
{    
    HRESULT retVal;
    INT *pvbits=NULL;
    
    retVal = createThumbnailFromImage (&hbp, &x, &y, &pvbits);

    if (SUCCEEDED(retVal))
    {
        retVal = EncodeThumbnail(pvbits, x, y, (VOID **) &pJPEGBuf, pulJPEGBufSize);
    }

    //
    // Finally free all allocated objects
    //

    DeleteObject(hbp);
    return retVal;
}


/******************************Public*Routine******************************\
* makeSquareThumbnail
*
*     Converts a rectangular thumbnail into a square thumbnail while
*     preserving the aspect ratio and window color
*
* Arguments:
*
*     pulOutputLength - length of thumbnail
*     pulOutputWidth -  width of thumbnail
*     phOutputBitmap -  thumbnail bitmap
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::makeSquareThumbnail(
    ULONG *pulOutputLength,
    ULONG *pulOutputWidth,
    HBITMAP *phOutputBitmap
    )
{
    //
    // HACK, HACK, ugly, ugly...
    // It turns out that the shell wants a square thumbnail (while preserving the aspect ratio).
    // It would be really bad to store the wasted bits in the thumbnail, so instead we have to
    // "squarify" the thumbnail here.  The right way to do it is for us to hand the rectangular
    // thumbnail to the shell along with the dimensions and have it BitBlt the correct number
    // of bits to the screen.  However, they are reluctant to do this and would rather us copy
    // the thumbnail to a square thumbnail which we will hand over to them...  Very lame!
    //

    if ((Thumbnail_X != *pulOutputLength) || (Thumbnail_Y != *pulOutputWidth))
    {
        //
        // Thumbnail better be too small or something is really wrong
        //
        if ((Thumbnail_X < *pulOutputLength) || (Thumbnail_Y < *pulOutputWidth))
        {
            return E_FAIL;
        }

        //
        // Allocate the square thumbnail
        //
        // Now Allocate buffer for thumbnail
        //
    
        PBITMAPINFO pbmi = (PBITMAPINFO) LocalAlloc(0, sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        if (!pbmi)
        {
            return E_OUTOFMEMORY;
        }
        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = Thumbnail_X;
        pbmi->bmiHeader.biHeight          = Thumbnail_Y;        
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        INT *ppvbits=NULL;
        HBITMAP hSquareBitmap = CreateDIBSection (NULL, pbmi, DIB_RGB_COLORS, (void **)&ppvbits, NULL, 0);

        //
        // Now paint it with the window background color
        //

        HDC hdcSquareBitmap = CreateCompatibleDC(NULL);
        SelectObject (hdcSquareBitmap, hSquareBitmap);
        HBRUSH hbr = CreateSolidBrush (GetSysColor(COLOR_WINDOW));
        SelectObject (hdcSquareBitmap, hbr);
        PatBlt(hdcSquareBitmap, 0, 0, Thumbnail_X, Thumbnail_Y, PATCOPY);

        //
        // Finally copy the thumbnail to it
        //
        HDC hdcMem = CreateCompatibleDC(NULL);
        SelectObject (hdcMem, *phOutputBitmap);
        BitBlt(hdcSquareBitmap, 
            (Thumbnail_X - *pulOutputLength) / 2,
            (Thumbnail_Y - *pulOutputWidth) / 2,
            *pulOutputLength,
            *pulOutputWidth,
            hdcMem,
            0,
            0,
            SRCCOPY
            );

        // 
        // Fix output parameters
        //
        DeleteObject(*phOutputBitmap);
        *phOutputBitmap = hSquareBitmap;
        *pulOutputLength = Thumbnail_X;
        *pulOutputWidth  = Thumbnail_Y;

        //
        // And clean up a bit
        //
        DeleteDC (hdcSquareBitmap);
        DeleteDC (hdcMem);
        DeleteObject(hbr);
        LocalFree(pbmi);
    }
    return S_OK;
}








/******************************Public*Routine******************************\
* TN_GenerateCompressedThumbnail
*
*   Generates a JPEG compressed thumbnail image from an input stream in a
*   format recognized by ImageDecodeFilter
*
* Arguments:
*
*     pInputStream -- An input stream in a format recognized by 
*         ImageDecodeFilter
*     pJPEGBuf -- A pointer to the buffer where the compressed JPEG
*         thumbnail will be stored
*     pulJPEGBufSize -- The number of bytes stored in pJPEGBuf 
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::TN_GenerateCompressedThumbnail(
    IStream *pInputStream,
    CHAR *pJPEGBuf,
    ULONG *pulJPEGBufSize,
    FILETIME mtime
    )
{
    HRESULT retVal;
    IImageDecodeEventSink *pOurEventSink;

    retVal = ((IThumbnailExtractor *) this)->QueryInterface(IID_IImageDecodeEventSink,
        (void **) &pOurEventSink);
    
    if (FAILED(retVal))
    {
        return retVal;
    }

    InitializeEventSink();

    //
    // Decode the image
    //
    
    _try
    {
        retVal = DecodeImage(pInputStream, NULL, pOurEventSink);    
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // imgutil.dll can't be delay loaded--fail gracefuly.
        //
        retVal = E_FAIL;
    }

    //
    // Fail if image data not delivered by the filter
    //

    if (!m_imageData.hbp)
    {
        retVal = E_FAIL;
    }
    
    if (SUCCEEDED(retVal))
    {
        //
        // Create the compressed thumbnail
        //
        
        retVal = createCompressedThumbnail (m_imageData.hbp,
            m_imageData.x,
            m_imageData.y,
            pJPEGBuf,
            pulJPEGBufSize,
            mtime
            );
    }

    //
    // Clean up old data to simplify debugging
    //
#ifdef DEBUG
    m_imageData.hbp = NULL;
    m_imageData.x = NULL;
    m_imageData.y = NULL;
#endif

    pOurEventSink->Release();

    return retVal;
}






/******************************Public*Routine******************************\
* TN_GenerateThumbnailBitmap
*
*   Generates a 32BPP bitmap from an image stream.
*   Note:  The caller is responsible for freeing phOutputBitmap by
*          calling DeleteObject!!
*
* Arguments:
*
*     prgbJPEGBuf -- A pointer to a buffer holding the compressed JPEG image
*     ulLength -- The requested length
*     ulWidth  -- the requested width
*     pulOutputLength -- will hold the actual output length
*     pulOutputWidth -- will hold the actual output width
*     phOutputBitmap -- will hold a pointer to a bitmap handle containing the
*         thumbnail image.  This buffer will be freed by the caller!
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::TN_GenerateThumbnailBitmap(
    IStream *pInputStream,
    ULONG   ulLength,
    ULONG   ulWidth,
    PULONG  pulOutputLength,
    PULONG  pulOutputWidth,
    HBITMAP *phOutputBitmap
    )
{
    *phOutputBitmap = NULL;

    //
    // For now don't handle the case of size other than 120x120
    //
    if ((ulLength != Thumbnail_X) ||
        (ulWidth  != Thumbnail_Y))
    {
        return E_INVALIDARG;
    }

    HRESULT retVal;

    InitializeEventSink();

    //
    // Decode the image
    //
    
    IImageDecodeEventSink *pOurEventSink;

    retVal = ((IThumbnailExtractor *) this)->QueryInterface(IID_IImageDecodeEventSink,
        (void **) &pOurEventSink);
    
    if (FAILED(retVal))
    {
        return retVal;
    }

    
    _try
    {
        retVal = DecodeImage(pInputStream, NULL, pOurEventSink);    
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // imgutil.dll can't be delay loaded--fail gracefuly.
        //
        retVal = E_FAIL;
    }



    if (SUCCEEDED(retVal))
    {

        *phOutputBitmap = m_imageData.hbp;
        *pulOutputLength = m_imageData.x;
        *pulOutputWidth  = m_imageData.y;

        //
        // Make into thumbnail
        //
        INT *pvBits;
        retVal = createThumbnailFromImage(phOutputBitmap, pulOutputLength, pulOutputWidth, &pvBits);

        if (SUCCEEDED(retVal))
        {
        
            //
            // Create a square thumbnail 
            //
        
            retVal = makeSquareThumbnail(pulOutputLength, pulOutputWidth, phOutputBitmap);
        }
    }

    //
    // Clean up old data to simplify debugging
    //

    m_imageData.hbp = NULL;
    m_imageData.x = NULL;
    m_imageData.y = NULL;
    
    pOurEventSink->Release();

    return retVal;
}





/******************************Public*Routine******************************\
* TN_GenerateThumbnailImage
*
*   Generates a 32BPP bitmap from a JPEG compressed thumbnail.
*   Note:  The caller is responsible for freeing phOutputBitmap by
*          calling DeleteObject!!
*
* Arguments:
*
*     prgbJPEGBuf -- A pointer to a buffer holding the compressed JPEG image
*     ulLength -- The requested length
*     ulWidth  -- the requested width
*     pulOutputLength -- will hold the actual output length
*     pulOutputWidth -- will hold the actual output width
*     phOutputBitmap -- will hold a pointer to a bitmap handle containing the
*         thumbnail image.  This buffer will be freed by the caller!
*
* Return Value:
*
*   ERROR_SUCCESS if successful, otherwise appropriate error code 
*
* History:
*
*    17-Oct-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP 
CThumbnailFCNContainer::TN_GenerateThumbnailImage(
    PCHAR   prgbJPEGBuf,
    ULONG   ulLength,
    ULONG   ulWidth,
    PULONG  pulOutputLength,
    PULONG  pulOutputWidth,
    HBITMAP *phOutputBitmap
    )
{
    HRESULT retVal;

    *phOutputBitmap = NULL;

    //
    // For now don't handle the case of size other than 120x120
    //
    if ((ulLength != Thumbnail_X) ||
        (ulWidth  != Thumbnail_Y))
    {
        return E_INVALIDARG;
    }
 
    retVal = DecodeThumbnail(phOutputBitmap, pulOutputLength, pulOutputWidth, (VOID *) prgbJPEGBuf, 0);

    if ((SUCCEEDED(retVal)))
    {
        retVal = makeSquareThumbnail(pulOutputLength, pulOutputWidth, phOutputBitmap);
    }
    return retVal;
}
