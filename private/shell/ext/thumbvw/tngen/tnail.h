/******************************Module*Header*******************************\
* Module Name: tnail.hxx
*
* Thumbnail API interface
*
* Created: 17-Oct-1997
* Author: Ori Gershony [orig]
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

HRESULT TN_Initialize(
    void
    );

HRESULT TN_Uninitialize(
    void
    );

HRESULT TN_GenerateCompressedThumbnail(
    IStream *pInputStream,
    CHAR *pJPEGBuf,
    ULONG *pulJPEGBufSize,
    FILETIME mtime
    );

HRESULT TN_GenerateThumbnailImage(
    PCHAR   prgbJPEGBuf,
    ULONG   ulLength,
    ULONG   ulWidth,
    PULONG  pulOutputLength,
    PULONG  pulOutputWidth,
    HBITMAP *phOutputBitmap,
    FILETIME mtime
    );
    
HRESULT TN_GenerateThumbnailBitmap(
    IStream *pInputStream,
    ULONG   ulLength,
    ULONG   ulWidth,
    PULONG  pulOutputLength,
    PULONG  pulOutputWidth,
    HBITMAP *phOutputBitmap
    );

extern CRITICAL_SECTION global_CriticalSection;

