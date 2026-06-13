// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Definition for CSwDoubleBufferedBitmap which provides a pair of bitmaps
//      with a synchronized CopyForward operation.
//
//------------------------------------------------------------------------------

MtExtern(CSwDoubleBufferedBitmap);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSwDoubleBufferedBitmap
//
//  Synopsis:
//      Manages a pair of IMILBitmaps.  The client should acquire and
//      release the back buffer directly and send a "CopyForward" command to
//      a managing MIL resource each time a new frame is produced in the back
//      buffer.  ProcessCopyForward() will take this cue to copy forward dirty
//      regions to the front buffer.
//
//------------------------------------------------------------------------------

class CSwDoubleBufferedBitmap :
    public CMILCOMBase
{
public:

    DECLARE_COM_BASE;

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwDoubleBufferedBitmap));

    static HRESULT Create(
        UINT width,
        UINT height,
        double dpiX,
        double dpiY,
        __in MilPixelFormat::Enum pixelFormat,
        __in_opt IWICPalette *pPalette,
        __deref_out CSwDoubleBufferedBitmap **ppSwDoubleBufferedBitmap
        );

    void GetBackBuffer(
        __deref_out IWICBitmap **ppBackBuffer,
        __out_opt UINT *pBackBufferSize
        ) const;

    void GetPossiblyFormatConvertedBackBuffer(
        __deref_out IWGXBitmapSource **ppPossiblyFormatConvertedBackBuffer
        ) const;

    void GetFrontBuffer(
        __deref_out IWGXBitmap **ppFrontBuffer
        ) const;

    HRESULT AddDirtyRect(
        __in const CMilRectU *pRect
        );

    HRESULT CopyForwardDirtyRects();

    HRESULT ProtectBackBuffer();

protected:

    virtual ~CSwDoubleBufferedBitmap();
    STDMETHOD(HrFindInterface)(__in REFIID riid, __deref_out void **ppv) override;

private:

    CSwDoubleBufferedBitmap();

    HRESULT HrInit(
        UINT width,
        UINT height,
        double dpiX,
        double dpiY,
        MilPixelFormat::Enum pixelFormat,
        __in_opt IWICPalette *pPalette
        );

    MilPixelFormat::Enum                m_backBufferPixelFormat;

    UINT                                m_width;
    UINT                                m_height;
    UINT                                m_backBufferSize;

    // This needs to deriver from IWICBitmap* because managed code will QI() to those interfaces
    IWICBitmap *                        m_pBackBuffer;
    CWriteProtectedBitmap *             m_pBackBufferAsWriteProtectedBitmap;
    IWGXBitmapSource *                  m_pFormatConverter; // Performs format conversion on copy-forward
    IWGXBitmap *                        m_pFrontBuffer;

    // We have to track our own dirty rects because the built-in dirty rect
    // handling ignores them if the bitmap has not been cached yet.  (See
    // IWGXBitmap::AddDirtRect.)  The dirty rects are for the back buffer, and
    // the back buffer is never cached because it is never directly accessed
    // by the render pass.
    CMilRectU *                         m_pDirtyRects;
    UINT                                m_numDirtyRects;
    enum { c_maxBitmapDirtyListSize = 5 };
}; // Class CSwDoubleBufferedBitmap


