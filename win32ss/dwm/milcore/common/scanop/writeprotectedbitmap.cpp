// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Implementation for CWriteProtectedBitmap.  Allocates system memory
//      bitmaps which are write protected outside of Lock/Unlock clauses.
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CWriteProtectedBitmap, MILImaging, "CWriteProtectedBitmap");

//+-----------------------------------------------------------------------------
//  Constructor / Destructor
//------------------------------------------------------------------------------

CWriteProtectedBitmap::CWriteProtectedBitmap() :
#ifdef DBG
    m_dbgLockCount(0),
#endif
    m_memoryProtected(true),
    m_numBytesForBitmap(0)
{
}

CWriteProtectedBitmap::~CWriteProtectedBitmap()
{
    if (m_pPixels)
    {
#ifdef DBG
        BOOL result = 
#endif
        VirtualFree(m_pPixels, 0, MEM_RELEASE);
#ifdef DBG  // Suppresses PreFast warning about undeclared identifier 'result'
        Assert(result != 0);
#endif
        // We must set this member to NULL so that CSystemMemoryBitmap won't
        // try to free the memory in its destructor.
        m_pPixels = NULL;
    }
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::Create
//
//  Synopsis:
//      Creates a new instance of the CWriteProtectedBitmap class.  The
//      bitmap's memory is initiallly in a read-only state for DBG builds.
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::Create(
    UINT width,
    UINT height,
    double dpiX,
    double dpiY,
    MilPixelFormat::Enum pixelFormat,
    __in_opt IWICPalette * pPalette,
    __deref_out CWriteProtectedBitmap **ppWriteProtectedBitmap
    )
{
    HRESULT hr = S_OK;

    CWriteProtectedBitmap *pWriteProtectedBitmap = new CWriteProtectedBitmap();
    IFCOOM(pWriteProtectedBitmap);
    pWriteProtectedBitmap->AddRef();

    IFC(pWriteProtectedBitmap->Init(width, height, dpiX, dpiY, pixelFormat, pPalette));

    pWriteProtectedBitmap->AddRef();
    *ppWriteProtectedBitmap = pWriteProtectedBitmap;

Cleanup:

    ReleaseInterfaceNoNULL(pWriteProtectedBitmap);

    RRETURN(hr);    
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::Lock
//
//  Synopsis:
//      Provides access to the pixels. The image is locked, but no processing
//      is performed - the pointer returned is a pointer to the actual bitmap
//      so that read and write semantics are supported.
//
//      Before calling lock the bitmap will be in a read-only state, calling
//      lock unsets the write-protect bit via a call to VirtualProtect.
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::Lock(
    __in_ecount_opt(1) const WICRect *prcLock,
    __in DWORD dwFlags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{
    HRESULT hr = S_OK;

#ifdef DBG
    // Remember that we unprotected the memory so we can recover gracefully from failure
    bool fUnprotectedMemory = false;

    // only unprotect the memory if it's currently protected, and if this is a Write Lock 
    if (dwFlags & MilBitmapLock::Write &&
        m_memoryProtected)
    {
        IFC(UnprotectBitmap());
        fUnprotectedMemory = true;
    }
#endif //DBG

    IFC(CSystemMemoryBitmap::Lock(prcLock, dwFlags, ppILock));

Cleanup:

#ifdef DBG
    if (FAILED(hr) && 
        fUnprotectedMemory)
    {
        IFC(ProtectBitmap());
    }
    else
    {
        m_dbgLockCount++;
    }
#endif //DBG

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::Unlock
//
//  Synopsis:
//      Releases the lock, and resets the write-protect bit using VirtualProtect.
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::Unlock(
    __in_ecount(1) CWGXBitmapLock *pBitmapLock
    )
{
    HRESULT hr = S_OK;

#ifdef DBG
    Assert(m_dbgLockCount > 0);
#endif //DBG

    // Call our base class to do most of the work.
    IFC(CSystemMemoryBitmap::Unlock(pBitmapLock)); 

#ifdef DBG
    // after our last lock is Released, we should re-protect the memory if needed.
    if (m_dbgLockCount == 0 &&
        !m_memoryProtected)
    {
        ProtectBitmap();
    }
#endif //DBG

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::Init
//
//  Synopsis:
//      Initializes a CWriteProtectedBitmap.  The bitmap's memory is initiallly
//      in a read-only state.  This memory becomes write-able to the users via
//      a call to CWriteProtectedBitmap::Lock().
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::Init(
    UINT width,
    UINT height,
    double dpiX,
    double dpiY,
    MilPixelFormat::Enum pixelFormat,
    __in_opt IWICPalette *pPalette
    )
{
    HRESULT hr = S_OK;
    UINT stride;
    UINT numBytesForBitmap = 0;
    UINT numPagesToAllocate = 0;
    UINT numBytesToAllocate = 0;

    Assert(m_pPixels == NULL); // Init should only be called once.

    if (pixelFormat == MilPixelFormat::DontCare)
    {
        IFC(E_INVALIDARG);
    }

    IFC(HrCalcDWordAlignedScanlineStride(width, pixelFormat, /*out*/ stride));
    IFC(HrGetRequiredBufferSize(pixelFormat, stride, width, height, &numBytesForBitmap));

    m_nWidth = width;
    m_nHeight = height;
    m_PixelFormat = pixelFormat;
    m_nStride = stride;
    m_numBytesForBitmap = numBytesForBitmap;

    IFC(SetResolution(dpiX, dpiY));

    if (IsIndexedPixelFormat(pixelFormat))
    {
        CHECKPTR(pPalette);
        IFC(SetPalette(pPalette));
    }
    else
    {
        // Non-Indexed formats must not specify a palette.
        if (pPalette)
        {
            IFC(E_INVALIDARG);
        }
    }

    // We want to allocate a guard page at the end of the allocated buffer.  This
    // page will be marked as read-only, to prevent buffer overruns.  This is
    // just to be helpful since we expose this functionality to partial-trust
    // via WriteableBitmap.  We must query the page size from the system since
    // it can differ between architectures.
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    IFC(UIntAdd(m_numBytesForBitmap, sysInfo.dwPageSize - 1, &numPagesToAllocate));
    numPagesToAllocate = numPagesToAllocate / sysInfo.dwPageSize;
    IFC(UIntAdd(numPagesToAllocate, 1, &numPagesToAllocate)); // Extra guard page
    IFC(UIntMult(numPagesToAllocate, sysInfo.dwPageSize, &numBytesToAllocate));

    // Allocate all of the memory at once, so it is contiguous.  Mark all pages
    // initially as read-only, and this will include the guard page.  We never
    // change the protection of the guard page after this.
    m_pPixels = VirtualAlloc(
        NULL,
        numBytesToAllocate,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READONLY
        );

    IFCW32(m_pPixels);
    
#ifndef DBG
    // Mark the pages backing the bitmap as read/write.  This un-protects all
    // but the last page (guard page).
    IFC(UnprotectBitmap());
#endif //!DBG

Cleanup:

    if (FAILED(hr))
    {
        if (m_pPixels != NULL)
        {
            VirtualFree(m_pPixels, 0, MEM_RELEASE);
            m_pPixels = NULL;
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::ProtectBitmap
//
//  Synopsis:
//      Disables writing to the bitmap bits    
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::ProtectBitmap()
{
    HRESULT hr = S_OK;
    DWORD fOldProtect = 0;

    // Mark the pages backing the bitmap as read only.
    IFCW32(VirtualProtect(
        m_pPixels,
        m_numBytesForBitmap,
        PAGE_READONLY,
        &fOldProtect
        ));

    m_memoryProtected = true;
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//  
//  Method:
//      CWriteProtectedBitmap::UnprotectBitmap
//
//  Synopsis:
//      Enables writing to the bitmap bits.    
//
//------------------------------------------------------------------------------

HRESULT CWriteProtectedBitmap::UnprotectBitmap()
{
    HRESULT hr = S_OK;
    DWORD fOldProtect = 0;

    // Mark the pages backing the bitmap as read only.
    IFCW32(VirtualProtect(
        m_pPixels,
        m_numBytesForBitmap,
        PAGE_READWRITE,
        &fOldProtect
        ));

    m_memoryProtected = false;
Cleanup:
    RRETURN(hr);
}

