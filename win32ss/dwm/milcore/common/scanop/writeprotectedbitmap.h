// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Implementation for CWriteProtectedBitmap.  Allocates system memory
//      bitmaps which are write protected outside of Lock/Unlock clauses.
//
//------------------------------------------------------------------------------

MtExtern(CWriteProtectedBitmap);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CWriteProtectedBitmap
//
//  Synopsis:
//      A derivative of CSystemMemoryBitmap, this class allocates a system
//      memory bitmap using a call to VirtualAlloc.  In DBG builds, the bitmap's 
//      memory is set to read-only, but a call to lock will change that 
//      permission to read-write.  This permission is reset to read-only by
//      calling in unlock. In non-DBG builds only one page at the end of the 
//      image buffer is reserved without write permission.
//
//------------------------------------------------------------------------------

class CWriteProtectedBitmap : public CSystemMemoryBitmap
{
public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CWriteProtectedBitmap));

    static HRESULT Create(
        UINT width,
        UINT height,
        double dpiX,
        double dpiY,
        MilPixelFormat::Enum pixelFormat,
        __in_opt IWICPalette * pPalette,
        __deref_out CWriteProtectedBitmap **ppWriteProtectedBitmap
        );
    
    //
    // IWGXBitmap interface overrides.
    //

    STDMETHOD(Lock)(
        __in_ecount_opt(1) IN const WICRect *prcLock,
        __in DWORD dwFlags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        ) override;

    STDMETHOD(Unlock)(
        __in_ecount(1) CWGXBitmapLock *pBitmapLock
        ) override;
    

    UINT GetBufferSize() const { return m_numBytesForBitmap; }

    STDMETHOD(ProtectBitmap)();
    STDMETHOD(UnprotectBitmap)();

protected:

    virtual ~CWriteProtectedBitmap();

private:

    CWriteProtectedBitmap();

    HRESULT Init(
        UINT width,
        UINT height,
        double dpiX,
        double dpiY,
        MilPixelFormat::Enum pixelFormat,
        __in_opt IWICPalette *pPalette
        );

    UINT m_numBytesForBitmap;

    #ifdef DBG
    // Indicates the number of locks currently held.
    // When this goes to 0 we will ensure the memory is write protected.
    UINT m_dbgLockCount;
    #endif
    
    // True if the image bits are currently write protected.
    bool m_memoryProtected;
};

