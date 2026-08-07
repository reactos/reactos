// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    bitmapres.h

Abstract:

    Bitmap resource. This file contains the implementation for all the
    bitmap resource functionality. This includes creating the resource,
    update, query, lock and unlock.

Environment:

    User mode only.

--*/

MtExtern(CMilSlaveBitmap);

class CMilSlaveBitmap : public CMilImageSource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilSlaveBitmap));

    CMilSlaveBitmap(__in_ecount(1) CComposition*);

    virtual ~CMilSlaveBitmap();

public:
    
    override virtual bool HasContent() const
    {
        return m_pIBitmap != NULL;
    }

    override virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDC,
        MilBitmapWrapMode::Enum wrapMode
        );

    override virtual HRESULT GetBounds(
        __in_ecount_opt(1) CContentBounder* pBounder,
        __out_ecount(1) CMilRectF *prcBounds
        );

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_BITMAPSOURCE || CMilImageSource::IsOfType(type);
    }

    override virtual HRESULT GetResolution(
        __out_ecount(1) double *dDpiX,
        __out_ecount(1) double *dDpiY
        ) const
    {
        HRESULT hr = S_OK;
        if (m_pIBitmap)
        {
            MIL_THR(m_pIBitmap->GetResolution(dDpiX, dDpiY));
        }
        else
        {
            MIL_THR(WGXERR_NOTINITIALIZED);
        }
        RRETURN(hr);
    }

    // The caller is responsible for calling Release() on the reference we hand out.
    __out_ecount_opt(1) IWGXBitmap *GetBitmap() 
    { 
        IWGXBitmap *pIBitmap = NULL;

        SetInterface(pIBitmap, m_pIBitmap); 

        return pIBitmap; 
    }

    override HRESULT GetBitmapSource(
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
        )
    {
        *ppIWGXBitmapSource = GetBitmap();

        return S_OK;
    }


    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessSource(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BITMAP_SOURCE* pBmp
        );

    HRESULT ProcessInvalidate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_BITMAP_INVALIDATE* pData
        );


private:

    IWGXBitmap *m_pIBitmap;

};


