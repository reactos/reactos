
/******************************Module*Header*******************************\
* Module Name: tnapi.cpp
*
* Methods for CThumbnailFCNContainer called directly from the shell
*
* Created: 27-Jan-1998
* Author: Ori Gershony [orig]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

/**************************************************************************\
*
* This module is responsible for encoding and decoding JPEG thumbnails, and
* for storing and retrieving them from the summary information property
* set of image files.  Here is the layout of the thumbnail data on disk:
*
*
* -------------------------------------------------------------------------
* | signature             | fileMiniHeader | jpegMiniHeader | jpeg stream |
* | (THUMBNAIL_CLIPDATA)  |                |                |             |
* -------------------------------------------------------------------------
*
* The 4 APIs in this module are called directly by the shell, and are also
* called by IExtractThumbnail APIs.
*
* Created: 27-Jan-1998
* Author: Ori Gershony [orig]
*
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
#include "shlwapi.h"

#include "objbase.h"

EXTERN_C const IID IID_IFlatStorage;  // Defined in ctngen.cpp

//
// JPEG mini-header
//
typedef struct _jpegMiniHeader
{
    ULONG ulVersion;
    ULONG ulStreamSize;
    ULONG ul_x;
    ULONG ul_y;
} jpegMiniHeader;

typedef struct _fileMiniHeader
{
    ULONG ulSize;
    FILETIME filetime;
} fileMiniHeader;

#define TNAIL_CURRENT_VER         1


/******************************Public*Routine******************************\
* CThumbnailFCNContainer::EncodeThumbnail
*
*   Encodes a 32BPP thumbnail
*
* Arguments:
*
*   pInputBitmapBits -- A pointer to the bits of the bitmap.  Must be in 32BPP
*       format!
*   ulWidth -- The width of the bitmap
*   ulHeight -- The height of the bitmap
*   ppJPEGBuffer -- if *ppJPEGBuffer is NULL, we will set this to point to the
*       buffer containing the
*       compressed JPEG bytes.  This must be freed by the caller by calling
*       CoTaskMemFree!  If not NULL, the existing pointer will be used.
*   pulBufferSize -- upon exit this will indicate the size of ppJPEGBuffer
*
* Return Value:
*
*     none
*
* History:
*
*     27-Jan-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::EncodeThumbnail(
    VOID *pInputBitmapBits,
    ULONG ulWidth,
    ULONG ulHeight,
    VOID **ppJPEGBuffer,
    ULONG *pulBufferSize
    )
{
    //
    // Allocate JPEG buffer if not allocated yet.  Compressed stream should never
    // be larger than uncompressed thumbnail
    //
    if (*ppJPEGBuffer == NULL)
    {
        *ppJPEGBuffer = (VOID *) CoTaskMemAlloc (sizeof(jpegMiniHeader) + ulWidth * ulHeight * 4 + 4096);
        if (*ppJPEGBuffer == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }

    EnterCriticalSection ( &g_csTNGEN );

    //
    // Compress thumbnail into JPEG stream
    //
    ULONG ulJpegSize;
    JPEGFromRGBA((unsigned char *) pInputBitmapBits,
        (BYTE *) *ppJPEGBuffer + sizeof(jpegMiniHeader),
        Thumbnail_Quality,
        &ulJpegSize,
        m_hJpegC,
        JCS_RGBA,
        ulWidth,
        ulHeight
        );

    //
    // Now fill in the header
    //
    jpegMiniHeader *pjMiniH = (jpegMiniHeader *) *ppJPEGBuffer;

    pjMiniH->ulVersion = TNAIL_CURRENT_VER;
    pjMiniH->ulStreamSize = ulJpegSize;
    pjMiniH->ul_x = ulWidth;
    pjMiniH->ul_y = ulHeight;

    *pulBufferSize = ulJpegSize + sizeof(jpegMiniHeader);

    LeaveCriticalSection ( &g_csTNGEN );
    return S_OK;
}


/******************************Public*Routine******************************\
* CThumbnailFCNContainer::DecodeThumbnail
*
*   Decode a JPEG stream
*
* Arguments:
*
*     phBitmap -- thumbnail bitmap
*     pulWidth -- width of thumbnail
*     pulHeight -- height of thumbnail
*     pJPEGBuffer -- the compressed JPEG stream
*     ulBufferSize -- the length of pBuffer
*
* Return Value:
*
*     none
*
* History:
*
*     27-Jan-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::DecodeThumbnail(
    HBITMAP *phBitmap,
    ULONG *pulWidth,
    ULONG *pulHeight,
    VOID *pJPEGBuffer,
    ULONG ulBufferSize
    )
{
    //
    // Make sure the header is current
    //
    jpegMiniHeader *pjMiniH = (jpegMiniHeader *)pJPEGBuffer;
    if (pjMiniH->ulVersion != TNAIL_CURRENT_VER)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection ( &g_csTNGEN );

    //
    // Allocate dibsection for thumbnail
    //
    BITMAPINFO bmi;

    bmi.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth           = pjMiniH->ul_x;
    bmi.bmiHeader.biHeight          = pjMiniH->ul_y;
    bmi.bmiHeader.biPlanes          = 1;
    bmi.bmiHeader.biBitCount        = 32;
    bmi.bmiHeader.biCompression     = BI_RGB;
    bmi.bmiHeader.biSizeImage       = 0;
    bmi.bmiHeader.biXPelsPerMeter   = 0;
    bmi.bmiHeader.biYPelsPerMeter   = 0;
    bmi.bmiHeader.biClrUsed         = 0;
    bmi.bmiHeader.biClrImportant    = 0;

    INT *ppvbits=NULL;
    *phBitmap = CreateDIBSection (NULL, &bmi, DIB_RGB_COLORS, (VOID **)&ppvbits, NULL, 0);

    //
    // Decode the JPEG data
    //
    ULONG ulReturnedNumChannels;

    RGBAFromJPEG((BYTE *)pJPEGBuffer + sizeof(jpegMiniHeader),
        (BYTE *) ppvbits,
        m_hJpegD,
        pjMiniH->ulStreamSize,
        1,
        &ulReturnedNumChannels,
        pjMiniH->ul_x,
        pjMiniH->ul_y
        );

    *pulWidth  = pjMiniH->ul_x;
    *pulHeight = pjMiniH->ul_y;

    LeaveCriticalSection ( &g_csTNGEN );
    return S_OK;
}



/******************************Public*Routine******************************\
* CThumbnailFCNContainer::WriteThumbnail
*
*     Write a thumbnail into the summary set property set of an image set
*
* Arguments:
*
*     pBuffer -- points to the buffer containing the compressed thumbnail
*     ulBufferSize -- The size of the buffer pointed to by pBuffer
*     pIStorage -- The storage to use
*     bAlwaysWrite -- If true, write the thumbnail to the file even if the
*         size of the image is less than MIN_JPEG_SIZE.
*
* Return Value:
*
*     none
*
* History:
*
*     27-Jan-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::WriteThumbnail(
    VOID *pBuffer,
    ULONG ulBufferSize,
    IPropertySetStorage *pPropSetStg,
    BOOLEAN bAlwaysWrite
    )
{
    HRESULT retVal;
    BOOLEAN bSaveThumbnail;

    //
    // A little bit of validation won't hurt, and can protect us against
    // bad callers
    //
    if (pPropSetStg == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Suppress timestamp changes
    //
    retVal = SuppressTimestampChanges(pPropSetStg);
    if (FAILED(retVal))
    {
        return retVal;
    }

    EnterCriticalSection ( &g_csTNGEN );
    //
    // Make sure pPropSetStg stays for the duration of this call
    //
    pPropSetStg->AddRef();

    //
    // If the force flag is not set, then don't save thumbnails when the
    // JPEG data is small.
    //
    bSaveThumbnail=TRUE;
    retVal=S_OK;

    if (! bAlwaysWrite)
    {
        IStorage *pStg;
        IStream *pInputStream;

        retVal = pPropSetStg->QueryInterface( IID_IFlatStorage, (PVOID*) & pStg );

        if (SUCCEEDED(retVal))
        {
            //
            // Open the stream containing image data and measure it's size.
            //
            DWORD grfMode = STGM_READ | STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
            retVal = pStg->OpenStream( L"CONTENTS", NULL, grfMode,
                                        0, &pInputStream);
            if (SUCCEEDED(retVal))
            {
                //
                // Only save thumbnails for images bigger than MIN_JPEG_SIZE
                //
                STATSTG statstg;
                retVal = pInputStream->Stat(&statstg, STATFLAG_NONAME);

                if (SUCCEEDED(retVal))
                {
                    if ((statstg.cbSize.HighPart == 0)
                        && (statstg.cbSize.LowPart <= MIN_JPEG_SIZE))
                    {
                        bSaveThumbnail = FALSE;
                    }
                }
                pInputStream->Release();
            }
            pStg->Release();
        }
    }

    //
    // If nothing else has failed so far and we want to save the thumbnail.
    //
    if ((SUCCEEDED(retVal)) && bSaveThumbnail)
    {
        //
        // Now access the property set
        //
        IPropertyStorage *pPropStg = NULL;

        retVal = pPropSetStg->Open(FMTID_DiscardableInformation,
            STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
            &pPropStg);
        if (STG_E_FILENOTFOUND == retVal)
        {
            retVal = pPropSetStg->Create(FMTID_DiscardableInformation,
                &CLSID_NULL,
                PROPSETFLAG_DEFAULT,
                STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                &pPropStg);
        }
        if (SUCCEEDED(retVal))
        {
            PROPSPEC propSpec[1];
            PROPVARIANT propVar[1];
            CHAR *pTNBuffer;

            if (pTNBuffer = (CHAR *) LocalAlloc(0, JPEG_BUFFER_SIZE))
            {
                propSpec[0].ulKind = PRSPEC_PROPID;
                propSpec[0].propid = PIDDI_THUMBNAIL;

                propVar[0].vt = VT_BLOB;
                propVar[0].wReserved1 = 0;
                propVar[0].wReserved2 = 0;
                propVar[0].wReserved3 = 0;

                propVar[0].blob.cbSize = ulBufferSize + sizeof(fileMiniHeader);
                propVar[0].blob.pBlobData = (unsigned char *) pTNBuffer;

                //
                // Now get the timestamp
                //

                FILETIME filetime;

                retVal = GetTimeStamp(pPropSetStg, &filetime);

                if (SUCCEEDED(retVal))
                {
                    ((fileMiniHeader *) pTNBuffer)->filetime = filetime;
                    ((fileMiniHeader *) pTNBuffer)->ulSize = ulBufferSize + sizeof(fileMiniHeader);

                    CopyMemory (pTNBuffer + sizeof(fileMiniHeader), pBuffer, ulBufferSize);

                    retVal = pPropStg->WriteMultiple(1,
                        propSpec,
                        propVar,
                        PID_FIRST_USABLE);

                    pPropStg->Commit(STGC_DEFAULT);
                }

                LocalFree(pTNBuffer);
            }
            else
            {
                retVal = E_OUTOFMEMORY;
            }
            pPropStg->Release();
        }
    }
    else
    {
        if (SUCCEEDED(retVal))
        {
            //
            // We decided not to save the thumbnail because the file is too small
            //
            retVal = E_FAIL; // A bit cryptic but I can't find anything better...
        }

    }
    pPropSetStg->Release();     // release the stablizing reference.

    LeaveCriticalSection ( &g_csTNGEN );
    return retVal;
}


/******************************Public*Routine******************************\
* CThumbnailFCNContainer::ReadThumbnail
*
*   Reads a compressed thumbnail from the summary information property set
*   of an image file.
*
* Arguments:
*
*       ppBuffer -- The buffer where the compressed JPEG stream will be stored.  If
*           *ppBuffer is NULL we will allocate correct amount of memory which must
*           then be freed by the caller by calling CoTaskMemFree.
*       pulBufferSize -- Upon exit this will contain the number of bytes deposited in
*           **ppBuffer.
*       pPropSetStg -- A pointer for an IPropertySetStorage on the image file
*
*
* Return Value:
*
*     none
*
* History:
*
*     27-Jan-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::ReadThumbnail(
    VOID **ppBuffer,
    ULONG *pulBufferSize,
    IPropertySetStorage *pPropSetStg
    )
{
    HRESULT retVal;

    //
    // A little bit of validation won't hurt, and can protect us against
    // bad callers
    //
    if (pPropSetStg == NULL)
    {
        return E_INVALIDARG;
    }

    //
    // Suppress timestamp changes
    //  (this should not be necessary on a read.  We should test this.)
    //
    //SuppressTimestampChanges(pPropSetStg);

    EnterCriticalSection ( &g_csTNGEN );
    //
    // Make sure pPropSetStg stays for the duration of this call
    //
    pPropSetStg->AddRef();

    //
    // Access the property set
    //
    IPropertyStorage *pPropStg = NULL;

    //
    // If we can't access the summary information stream, there's nothing
    // more to do (don't create it because we're readers, not writers)
    //
    retVal = pPropSetStg->Open(FMTID_DiscardableInformation,
        STGM_READ | STGM_SHARE_EXCLUSIVE,
        &pPropStg);
    if (SUCCEEDED(retVal))
    {
        //
        // Now try to read the compressed thumbnail
        //
        PROPSPEC propSpec[1];
        PROPVARIANT propVar[1];

        propSpec[0].ulKind = PRSPEC_PROPID;
        propSpec[0].propid = PIDDI_THUMBNAIL;

        retVal = pPropStg->ReadMultiple(1,
            propSpec,
            propVar);
        if (SUCCEEDED(retVal))
        {
            //
            // Make sure we recognize the format of this thumbnail
            //
            if (propVar[0].vt == VT_BLOB)
            {
                FILETIME filetime;

                retVal = GetTimeStamp(pPropSetStg, &filetime);

                if (SUCCEEDED(retVal))
                {
                    // Should be at least large enough to hold file mini header
                    if (propVar[0].blob.cbSize >= sizeof(fileMiniHeader))
                    {
                        VOID *pThumbnailData = (VOID *) propVar[0].blob.pBlobData;

                        fileMiniHeader *pFileMH = (fileMiniHeader *) pThumbnailData;

                        if ((filetime.dwLowDateTime  == pFileMH->filetime.dwLowDateTime) &&
                            (filetime.dwHighDateTime == pFileMH->filetime.dwHighDateTime))
                        {
                            *pulBufferSize = pFileMH->ulSize - sizeof(fileMiniHeader);

                            if (*pulBufferSize > 0)
                            {

                                if (*ppBuffer == NULL)
                                {
                                    *ppBuffer = (VOID *) CoTaskMemAlloc (*pulBufferSize);
                                }

                                if (*ppBuffer)
                                {
                                    CopyMemory (*ppBuffer, (VOID *) (((CHAR *)pThumbnailData) + sizeof(fileMiniHeader)), *pulBufferSize);
                                }
                                else
                                {
                                    retVal = E_OUTOFMEMORY;
                                }
                            }
                            else
                            {
                                retVal = E_FAIL;
                            }
                        }
                        else
                        {
                            retVal = TRUST_E_TIME_STAMP;
                        }
                    }
                    else
                    {
                        // Not enough bytes to read file mini header--thumbnail is seriously broken
                        retVal = E_FAIL;
                    }
                }
            }
            else
            {
                retVal = E_FAIL;
            }
            FreePropVariantArray(1, propVar);
        }
        else
        {
            retVal = E_FAIL;
        }
        pPropStg->Release();
    }
    else
    {
        retVal = E_FAIL;
    }

    pPropSetStg->Release(); // Release the Stablizing reference.
    LeaveCriticalSection ( &g_csTNGEN );

    return retVal;
}
