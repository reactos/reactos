/******************************Module*Header*******************************\
* Module Name: ctngen.h
*
* Implementation of the IFileChangeNotify and IThumbnailGenerator interfaces
*
* Created: 14-Nov-1997
* Author: Ori Gershony [orig]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#ifndef _CTNGEN_H_
#define _CTNGEN_H_

#include <objidl.h>
#include "..\resource.h"
#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include "ddraw.h"
#include "ImgUtil.H"
#include "ocmm.h"
#include "decoder.h"

// Jpeg buffer large enough to fit string and compressed JPEG stream.  To be
// conservative, this should be at least as large as width*height*4 of the
// thumbnail (4 because we store it as 32BPP).
#define JPEG_BUFFER_SIZE 60000

#define MIN_JPEG_SIZE    32768
#define MAX_IMAGE_SIZE   4*1024*1024 // Don't decode images that take more storage than
                                     // a 1024 by 1024 32BPP DIB in the content indexer
                                     // codepath

extern CRITICAL_SECTION g_csTNGEN;


//
// This is defined in uuid.lib
//

EXTERN_C const GUID CLSID_ThumbnailFCNHandler;

//
// The CThumbnailFCNContainer class implements the IThumbnailExtractor interface
//

class CThumbnailFCNContainer : public IThumbnailExtractor,
                               public IImageDecodeEventSink,
                               public CComObjectRoot,
                               public CComCoClass <CThumbnailFCNContainer,&CLSID_ThumbnailFCNHandler>
{
    public:

        CThumbnailFCNContainer(VOID);
        ~CThumbnailFCNContainer(VOID);

        BEGIN_COM_MAP( CThumbnailFCNContainer )
            COM_INTERFACE_ENTRY( IThumbnailExtractor )
            COM_INTERFACE_ENTRY( IImageDecodeEventSink )
        END_COM_MAP( )

        DECLARE_REGISTRY( CThumbnailFCNContainer,
                          _T("tngen.1"),
                          _T("tngen"),
                          IDS_THUMBNAILGEN_DESC,
                          THREADFLAGS_BOTH);


        DECLARE_NOT_AGGREGATABLE( CThumbnailFCNContainer );

        // IThumbnailExtractor members

        STDMETHODIMP        OnFileUpdated (IStorage *pStg);

        STDMETHODIMP        ExtractThumbnail(
                                IStorage *pStg,
                                ULONG ulLength,
                                ULONG ulHeight,
                                ULONG *pulOutputLength,
                                ULONG *pulOutputHeight,
                                HBITMAP *phOutputBitmap
                                );

        // Other public members (to be used by the shell)

        STDMETHODIMP        EncodeThumbnail(
                                VOID *pInputBitmapBits,
                                ULONG ulWidth,
                                ULONG ulHeight,
                                VOID **ppJPEGBuffer,
                                ULONG *pulBufferSize
                                );

        STDMETHODIMP        DecodeThumbnail(
                                HBITMAP *phBitmap,
                                ULONG *pulWidth,
                                ULONG *pulHeight,
                                VOID *pJPEGBuffer,
                                ULONG ulBufferSize
                                );

        STDMETHODIMP        WriteThumbnail(
                                VOID *pBuffer,
                                ULONG ulBufferSize,
                                IPropertySetStorage *pPropSetStg,
                                BOOLEAN bAlwaysWrite
                                );

        STDMETHODIMP        ReadThumbnail(
                                VOID **ppBuffer,
                                ULONG *pulBufferSize,
                                IPropertySetStorage *pPropSetStg
                                );

        STDMETHODIMP        GetTimeStamp(
                                IPropertySetStorage *pPropSetStg,
                                FILETIME *pFiletime
                                );

    private:
        // private members

        STDMETHODIMP        ExtractThumbnailHelper(
                                IStorage *pStg,
                                ULONG ulLength,
                                ULONG ulHeight,
                                ULONG *pulOutputLength,
                                ULONG *pulOutputHeight,
                                HBITMAP *phOutputBitmap
                                );

        STDMETHODIMP        CThumbnailFCNContainer::SuppressTimestampChanges(
                                IUnknown *punkStorage
                                );


        // TN_ members

        STDMETHODIMP        CThumbnailFCNContainer::TN_Initialize(
                                VOID
                                );

        STDMETHODIMP        CThumbnailFCNContainer::TN_Uninitialize(
                                VOID
                                );

        STDMETHODIMP        CThumbnailFCNContainer::deliverDecompressedImage(
                                HBITMAP hbp,
                                ULONG x,
                                ULONG y
                                );

        STDMETHODIMP        CThumbnailFCNContainer::createThumbnailFromImage(
                                HBITMAP *phbp,
                                ULONG *px,
                                ULONG *py,
                                INT **ppvbits
                                );

        STDMETHODIMP        CThumbnailFCNContainer::createCompressedThumbnail(
                                HBITMAP hbp,
                                ULONG x,
                                ULONG y,
                                CHAR *pJPEGBuf,
                                ULONG *pulJPEGBufSize,
                                FILETIME mtime
                                );

        STDMETHODIMP        CThumbnailFCNContainer::makeSquareThumbnail(
                                ULONG *pulOutputLength,
                                ULONG *pulOutputWidth,
                                HBITMAP *phOutputBitmap
                                );

        STDMETHODIMP        CThumbnailFCNContainer::TN_GenerateCompressedThumbnail(
                                IStream *pInputStream,
                                CHAR *pJPEGBuf,
                                ULONG *pulJPEGBufSize,
                                FILETIME mtime
                                );

        STDMETHODIMP        CThumbnailFCNContainer::TN_GenerateThumbnailBitmap(
                                IStream *pInputStream,
                                ULONG   ulLength,
                                ULONG   ulWidth,
                                PULONG  pulOutputLength,
                                PULONG  pulOutputWidth,
                                HBITMAP *phOutputBitmap
                                );

        STDMETHODIMP        CThumbnailFCNContainer::TN_GenerateThumbnailImage(
                                PCHAR   prgbJPEGBuf,
                                ULONG   ulLength,
                                ULONG   ulWidth,
                                PULONG  pulOutputLength,
                                PULONG  pulOutputWidth,
                                HBITMAP *phOutputBitmap
                                );

        // TN_ variables:

        //
        // The following globals should get their values from the registry
        // during TN_Initialize
        //
        // WARNING:  for large Thumbnail_X and Thumbnail_Y values, we will also
        // need to increase INPUT_vBUF_SIZE and OUTPUT_BUF_SIZE in jdatasrc.cpp and
        // jdatadst.cpp (lovely jpeg decompression code...).  Also need to modify our
        // own JPEG_BUFFER_SIZE in ctngen.cxx.
        //
        ULONG Thumbnail_Quality;
        ULONG Thumbnail_X;
        ULONG Thumbnail_Y;

        //
        // JPEG globals
        //
        HANDLE m_hJpegC, m_hJpegD;
        BYTE * m_JPEGheader;
        ULONG  m_JPEGheaderSize;

        struct {
            HBITMAP hbp;
            ULONG x;
            ULONG y;
        } m_imageData;

        // IImageDecodeEventSink members:

        STDMETHODIMP        CThumbnailFCNContainer::GetSurface(
                                LONG nWidth,
                                LONG nHeight,
                                REFGUID bfid,
                                ULONG nPasses,
                                DWORD dwHints,
                                IUnknown** ppSurface
                                );

        STDMETHODIMP        CThumbnailFCNContainer::OnBeginDecode(
                                DWORD* pdwEvents,
                                ULONG* pnFormats,
                                GUID** ppFormats
                                );

        STDMETHODIMP        CThumbnailFCNContainer::OnBitsComplete(
                                VOID
                                );

        STDMETHODIMP        CThumbnailFCNContainer::OnDecodeComplete(
                                HRESULT hrStatus
                                );

        STDMETHODIMP        CThumbnailFCNContainer::OnPalette(
                                VOID
                                );

        STDMETHODIMP        CThumbnailFCNContainer::OnProgress(
                                RECT* pBounds,
                                BOOL bFinal
                                );

        // Decoder variables:
        FILTERINFO m_Filter;
        CComPtr< IDirectDrawSurface > m_pDDrawSurface;
        RECT m_rcProg;
        DWORD m_dwLastTick;

        // Other decoder privates

        STDMETHODIMP        CThumbnailFCNContainer::InitializeEventSink(
                                VOID
                                );

};

typedef CThumbnailFCNContainer *PCThumbnailFCNContainer;


//
// Decoder publics
//
HRESULT decoderInitialize(VOID);
VOID decoderUninitialize(VOID);

#endif
