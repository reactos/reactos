
/******************************Module*Header*******************************\
* Module Name: ctngen.cpp
*
* Implementation of the IThumbnailExtractor interface
*
* Created: 14-Nov-1997
* Author: Ori Gershony [orig]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "objbase.h"

#include "stdafx.h"
#include "tnail.h"
#include "ctngen.h"

extern CHAR THUMBNAIL_CLIPDATA[];

//
// Ntfs Flat File objects do not respond to QI for IID_IStorage any more.
// If you really need an IStorage* and you are a system component then
// you can use IID_IFlatStorage.  (IFlatStorage does not marshal)
//
EXTERN_C const IID IID_IFlatStorage = { /* b29d6138-b92f-11d1-83ee-00c04fc2c6d4 */
    0xb29d6138,
    0xb92f,
    0x11d1,
    {0x83, 0xee, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0xd4}
  };

// Global critical section to make code single threaded.
// Actually the code doesn't look to bad, but we are calling the JPG library
// and I know nothing about that.  Perhaps with alot more thought, study and
// investigation this code could be shown to be thread safe.
//
CRITICAL_SECTION g_csTNGEN;

/******************************Public*Routine******************************\
* CThumbnailFCNContainer::CThumbnailFCNContainer
*
*   Constructor for CThumbnailFCNContainer
*
* Arguments:
*
* Return Value:
*
*     none
*
* History:
*
*     14-Nov-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
CThumbnailFCNContainer::CThumbnailFCNContainer(VOID)
{
    TN_Initialize();
}


/******************************Public*Routine******************************\
* CThumbnailFCNContainer::~CThumbnailFCNContainer
*
*   Destructor for CThumbnailFCNContainer
*
* Arguments:
*
* Return Value:
*
*     none
*
* History:
*
*     14-Nov-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
CThumbnailFCNContainer::~CThumbnailFCNContainer(VOID)
{
    TN_Uninitialize();
}


/******************************Public*Routine******************************\
* CThumbnailFCNContainer::OnFileUpdated
*
*   This method notifies us that the file indicated by pStg has been
*   modified and therefore we should generate a new thumbnail for it.
*
* Arguments:
*
*     pStg -- A pointer to a storage of the file which was modified
*
* Return Value:
*
*     NOERROR upon success, or appropriate error code
*
* History:
*
*     14-Nov-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::OnFileUpdated(
    IStorage *pStg
    )
{
    HRESULT retVal;
    IStream *pInputStream;
    IPropertySetStorage *pPropSetStg=NULL;

    //
    // A little bit of validation won't hurt, and can protect us against
    // bad callers
    //
    if (pStg == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection ( &g_csTNGEN );
    //
    // Get the PropertySetStorage Interface.  This also serves as the
    // stablizing reference.
    //
    retVal = pStg->QueryInterface( IID_IPropertySetStorage,
                                       (PVOID*)&pPropSetStg );


    // if (FAILED(retVal))
    // {
    //     return retVal;
    // }
    // else
    // {
    //     VOID* pvData;
    //     DWORD cbSize;
    //
    //     retVal = ReadThumbnail(&pvData, &cbSize, pPropSetStg );
    //     if (SUCCEEDED(retVal))
    //     {
    //         CoTaskMemFree( pvData );
    //     }
    //     else
    //     {
    //         pPropSetStg->Release();
    //         return S_OK;
    //     }
    // }

    //
    // First open the stream containing image data
    //
    retVal = pStg->OpenStream(L"CONTENTS",
        NULL,
        STGM_READ | STGM_SHARE_EXCLUSIVE | STGM_DIRECT,
        0,
        &pInputStream);

    if (SUCCEEDED(retVal))
    {
        VOID *pBuffer = (VOID *) LocalAlloc (0, JPEG_BUFFER_SIZE);

        if (pBuffer)
        {
            ULONG ulBufferSize;

            FILETIME filetime = { 0 };

            retVal = TN_GenerateCompressedThumbnail(pInputStream,
                (CHAR *) pBuffer,
                &ulBufferSize,
                filetime);

            pInputStream->Release();

            if (SUCCEEDED(retVal))
            {
                retVal = WriteThumbnail(pBuffer, ulBufferSize, pPropSetStg, FALSE);
            }
            LocalFree(pBuffer);
        }
        else
        {
            //
            // Release pInputStream if memory allocation fails.  We can't use a common
            // code path for both because WriteThumbnail requires the stream to have already
            // been released.
            //
            pInputStream->Release();
        }
    }
    pPropSetStg->Release();

    LeaveCriticalSection ( &g_csTNGEN );

    return retVal;
}



/******************************Public*Routine******************************\
* CThumbnailFCNContainer::ExtractThumbnail
*
*   Returns a bitmap of the thumbnail for the storage pointed to by pStg
*   Note:  The caller is responsible for freeing the bitmap pointed to by
*          phOutputBitmap by calling DeleteObject!
*
* Arguments:
*       pStg -- The storage containing the thumbnail
*       ulLength -- The requested length
*       ulWidth -- The requested width
*       pulOutputLength -- will hold the actual output length
*       pulOutputWidth -- will hold the actual output width
*       phOutputBitmap -- A pointer that will point to the bitmap containing
*           the thumbnail.  This will be released by the caller!
*
* Return Value:
*
*     NOERROR upon success, or appropriate error code
*
* History:
*
*     14-Nov-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::ExtractThumbnail(
    IStorage *pStg,
    ULONG ulLength,
    ULONG ulHeight,
    ULONG *pulOutputLength,
    ULONG *pulOutputHeight,
    HBITMAP *phOutputBitmap
    )
{
    HRESULT retVal;

    //
    // A little bit of validation won't hurt, and can protect us against
    // bad callers
    //

    if (pStg == NULL)
    {
        return E_INVALIDARG;
    }

    EnterCriticalSection ( &g_csTNGEN );

    retVal = SuppressTimestampChanges(pStg);

    if (SUCCEEDED(retVal))
    {
        retVal = ExtractThumbnailHelper(pStg,
                ulLength,
                ulHeight,
                pulOutputLength,
                pulOutputHeight,
                phOutputBitmap);
    }

    //
    // If no thumbnail was extracted then generate a thumbnail
    // and try again.
    //

    if (FAILED(retVal))
    {
        retVal = OnFileUpdated(pStg);

        if (SUCCEEDED(retVal))
        {
            //
            // Try again--now thumbnail should be there
            //
            retVal = ExtractThumbnailHelper(pStg,
                ulLength,
                ulHeight,
                pulOutputLength,
                pulOutputHeight,
                phOutputBitmap);
        }
    }
    LeaveCriticalSection ( &g_csTNGEN );
    return retVal;
}



/******************************Public*Routine******************************\
* CThumbnailFCNContainer::ExtractThumbnailHelper
*
*   Returns a bitmap of the thumbnail for the storage pointed to by pStg
*   Note:  The caller is responsible for freeing the bitmap pointed to by
*          phOutputBitmap by calling DeleteObject!
*
* Arguments:
*       pStg -- The storage containing the thumbnail
*       ulLength -- The requested length
*       ulWidth -- The requested width
*       pulOutputLength -- will hold the actual output length
*       pulOutputWidth -- will hold the actual output width
*       phOutputBitmap -- A pointer that will point to the bitmap containing
*           the thumbnail.  This will be released by the caller!
*
* Return Value:
*
*     NOERROR upon success, or appropriate error code
*
* History:
*
*     14-Nov-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::ExtractThumbnailHelper(
    IStorage *pStg,
    ULONG ulLength,
    ULONG ulHeight,
    ULONG *pulOutputLength,
    ULONG *pulOutputHeight,
    HBITMAP *phOutputBitmap
    )
{
    HRESULT retVal;
    IPropertySetStorage *pPropSetStg;

    //
    // Read the thumbnail
    //
    VOID *pBuffer=NULL;
    ULONG ulBufferSize=0;

    retVal = pStg->QueryInterface( IID_IPropertySetStorage,
                                   (PVOID*) & pPropSetStg );

    if (SUCCEEDED(retVal))
    {
        retVal = ReadThumbnail(&pBuffer, &ulBufferSize, pPropSetStg);

        if (SUCCEEDED(retVal))
        {
            retVal = TN_GenerateThumbnailImage(
                 (CHAR *) pBuffer,
                 ulLength,
                 ulHeight,
                 pulOutputLength,
                 pulOutputHeight,
                 phOutputBitmap);

            CoTaskMemFree(pBuffer);
        }
        pPropSetStg->Release();
    }

    return retVal;
}




/******************************Public*Routine******************************\
* CThumbnailFCNContainer::GetTimeStamp
*
*   This method obtains the timestamp for the last modification of the CONTENTS
*   stream of pStg
*
*
* Arguments:
*
*     pStg -- A pointer to a storage of the file
*     pFileTime -- A pointer to a variable which will receive the timestamp
*
* Return Value:
*
*     NOERROR upon success, or appropriate error code
*
* History:
*
*     30-Dec-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::GetTimeStamp(
    IPropertySetStorage *pPropSetStg,
    FILETIME *pFiletime
    )
{
    //
    // Obtain the timestamp
    //

    HRESULT retVal;
    IStorage *pStg=NULL;
    STATSTG statstg;

    retVal = pPropSetStg->QueryInterface( IID_IFlatStorage, (PVOID*) & pStg );

    if(SUCCEEDED(retVal))
    {
        retVal = pStg->Stat(&statstg, STATFLAG_NONAME);

        if (SUCCEEDED(retVal))
        {
            *pFiletime = statstg.mtime;
        }
        pStg->Release();
    }
    return retVal;
}



/******************************Public*Routine******************************\
* CThumbnailFCNContainer::SuppressTimestampChanges
*
*   This method suppresses timestamp notifications for a storage
*
*
* Arguments:
*
*     punkStorage -- A pointer to a storage of the file
*
* Return Value:
*
*     NOERROR upon success, or appropriate error code
*
* History:
*
*     30-Dec-1997 -by- Ori Gershony [orig]
*
\**************************************************************************/
STDMETHODIMP
CThumbnailFCNContainer::SuppressTimestampChanges(
    IUnknown *punkStorage
    )
{
    HRESULT retVal;

    //
    // Supress file time modification
    //
    ITimeAndNoticeControl *pITimeAndNoticeControl;

    retVal = punkStorage->QueryInterface(IID_ITimeAndNoticeControl,
        (VOID **) &pITimeAndNoticeControl);

    if (SUCCEEDED(retVal))
    {
        retVal = pITimeAndNoticeControl->SuppressChanges(1,0);
        if (FAILED(retVal))
            retVal = pITimeAndNoticeControl->SuppressChanges(0,0);
        pITimeAndNoticeControl->Release();
    }
    return retVal;
}
