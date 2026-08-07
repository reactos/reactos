// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_interop
//      $Keywords:
//
//  $Description:
//      Contains declaration for CDeviceBitmap class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CHwDeviceBitmapColorSource;

MtExtern(CDeviceBitmap);

class CDeviceBitmap : public CWGXBitmap
{
public:

    static HRESULT Create(
        __in_range(0,SURFACE_RECT_MAX) UINT uWidth,
        __in_range(0,SURFACE_RECT_MAX) UINT uHeight,
        MilPixelFormat::Enum pixelFormat,
        __deref_out_ecount(1) CDeviceBitmap * &pBitmap
        );

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CDeviceBitmap));

    CDeviceBitmap(
        __in_range(0,SURFACE_RECT_MAX) UINT uWidth,
        __in_range(0,SURFACE_RECT_MAX) UINT uHeight,
        MilPixelFormat::Enum pixelFormat
        );

    virtual ~CDeviceBitmap();

public:

    // IWGXBitmap interfaces.

    STDMETHODIMP Lock(
        __in_ecount_opt(1) const WICRect *prcLock,
        DWORD dwFlags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        );

    // IWGXBitmap overrides.

    // Distinguishes between bitmaps with full source, no source, and
    // placeholder source for shared surfaces

    override STDMETHODIMP_(SourceState::Enum) SourceState() const
    {
        return SourceState::DeviceBitmap;
    };


    // CDeviceBitmap extensions

    HRESULT SetDeviceBitmapColorSource(
        __in_ecount_opt(1) HANDLE hShared,
        __inout_ecount(1) CHwDeviceBitmapColorSource *pbcs
        );

    void AddUpdateRect(
        __in_ecount(1) const CMilRectU &rcUpdate
        );

    bool ContainsValidArea(
        __in_ecount(1) CMilRectU const &rcArea) const;

    bool HasContributorFromDifferentAdapter(
        LUID luidAdapter
        );

    bool HasValidDeviceBitmap();

    HRESULT GetPointerToValidRectsForSurface(
        __out_ecount(1) UINT &cValidRects,
        __deref_out_ecount_full(cValidRects) CMilRectU const * &rgValidContents
        );

    virtual bool TryCreateDependentDeviceColorSource(
        __in const LUID &luidNewDevice,
        __in CHwBitmapCache *pNewCache
        );

    __out_opt CHwDeviceBitmapColorSource *GetDeviceColorSourceNoRef();

protected:

    void CleanupInvalidSource();

    struct DeviceBitmapInfo
    {
        ~DeviceBitmapInfo()
        {

            Destruct();
        }
    
        void Construct(
            __in_ecount_opt(1) HANDLE hShared,
            __inout_ecount(1) CHwDeviceBitmapColorSource *pbcs
            );

        void Destruct();

        HRESULT AddValidRect(
            __in_ecount(1) CMilRectU const &rcValid,
            __deref_inout_opt HRGN &hrgnNewValid,
            __out_ecount(1) bool &fContained
            );

        bool DoesIntersectValid(
            __in_ecount(1) CMilRectU const &rc
            ) const;

        void AssertRgnBoundsMatch(
            ) const;

        HRESULT EnsureRgnData(
            );

        HRESULT GetPointerToValidRects(
            __out_ecount(1) UINT &cValidRects,
            __deref_out_ecount_full(cValidRects) CMilRectU const * &rgValidRects
            );


        HANDLE m_hShared;
        CHwDeviceBitmapColorSource *m_pbcs;  // Device specific resource that
                                             // controls access to the shared
                                             // surface.  Note that before
                                             // making device dependent calls
                                             // to the object its validity
                                             // should be checked via
                                             // IsValid().

        CMilRectU m_rcValid;     // Bounding rectangle for the valid area of
                                    // the surface.

        HRGN m_hrgnValid;           // Region representing valid area of the
                                    // surface.  Used only when the valid area
                                    // of the surface is a complex region,
                                    // otherwise rcValid has all the data.

        PRGNDATA m_pRgnData;

        // Data for tracking contents copied to system memory buffer.  Ideally
        // copied contents would be tracked per individual valid rect, but
        // region processing relies on HRGN processing there is no good way to
        // individual rects within the region.  Simplified rectangle tracking
        // is used.  The rectangles always have this "containing" relationship:

        //          +---------------------------------------+
        //          |  rcValid                              |
        //          |           +-----------------------+   +
        //          |           | rcCopiedToSysMemBuffer|   |
        //          |           |                       |   |
        //          |           |                       |   |
        //          |           | +----------+          |   |
        //          |           | |  rcDirty |          |   |
        //          |           | |          |          |   |
        //          |           | |          |          |   |
        //          |           | +----------+          |   |
        //          |           |                       |   |
        //          |           |                       |   |
        //          |           +-----------------------+   +
        //          |                                       |
        //          +---------------------------------------+

        CMilRectU m_rcCopiedToSysMemBuffer;// Area of device bitmap that
                                              // has ever been copied to system
                                              // memory buffer.  Currently
                                              // limited to rcValid.
        CMilRectU m_rcDirty;               // Area of device bitmap that
                                              // has been updated since some
                                              // has been copied to system
                                              // memory buffer.  Always limited
                                              // to rcCopiedToSysMemBuffer.

        bool m_fSysMemBufferStale;  // Indicator if there is some valid part of
                                    // devie bitmap that has not been copied
                                    // to system memory buffer or what has been
                                    // copies is out of date.  The value should
                                    // match:

                                    //      !rcValid.IsEmpty()
                                    //  AND (   rcCopiedToSysMem != rcValid
                                    //       OR !rcDirty.IsEmpty())

                                    // It is okay if the value is true, but
                                    // nothing is actually stale, but it is not
                                    // okay for the value to be false when
                                    // there is something stale.
    };

    DeviceBitmapInfo *m_poDeviceBitmapInfo;

private:

    HRESULT PrepareSysMemBufferAsSourcePixels(
        );

    HRESULT EnsureUpdatedSysMemBuffer(
        __in_ecount(1) CMilRectU const &rc
        );

    __field_bcount_opt(m_cbBuffer) VOID *m_pPixels;
    UINT m_uStride;

    UINT m_cbBuffer;    // Bytes allocated for m_pPixels

    UINT m_cbPixel;     // Bytes per pixel
};



