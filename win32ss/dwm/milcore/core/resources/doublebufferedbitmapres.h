// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#pragma once

//+-----------------------------------------------------------------------------
//

//
//  Description:
//      Definition for CMilSlaveDoubleBufferedBitmap, the resource responsible
//      for managing a CSwDoubleBufferedBitmap object.
//
//------------------------------------------------------------------------------

MtExtern(CMilSlaveDoubleBufferedBitmap);

class CMilSlaveDoubleBufferedBitmap : public CMilImageSource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilSlaveDoubleBufferedBitmap));

    CMilSlaveDoubleBufferedBitmap(__in CComposition *pComposition);

    virtual ~CMilSlaveDoubleBufferedBitmap();

public:

    //+-------------------------------------------------------------------------
    //  CMilImageSource Overrides
    //--------------------------------------------------------------------------

    override virtual bool HasContent() const
    {
        return (m_pDoubleBufferedBitmap != NULL);
    }

    override virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDC,
        __in MilBitmapWrapMode::Enum wrapMode
        );

    override virtual HRESULT GetBounds(
        __in_ecount_opt(1) CContentBounder* pBounder,
        __out_ecount(1) CMilRectF *prcBounds
        );

    override virtual HRESULT GetResolution(
        __out_ecount(1) double *dDpiX,
        __out_ecount(1) double *dDpiY
        ) const;

    override virtual HRESULT GetBitmapSource(
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
        );

    //+-------------------------------------------------------------------------
    //  CMilSlaveResource Overrides
    //--------------------------------------------------------------------------

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return (type == TYPE_DOUBLEBUFFEREDBITMAP) || CMilImageSource::IsOfType(type);
    }

    STDMETHOD(RegisterNotifiers)(CMilSlaveHandleTable* pHandleTable);

    override virtual void UnRegisterNotifiers();

    //+-------------------------------------------------------------------------
    //   Command handlers
    //--------------------------------------------------------------------------

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_DOUBLEBUFFEREDBITMAP *pCmd
        );

    HRESULT ProcessCopyForward(
        __in_ecount(1) CMilSlaveHandleTable const *pHandleTable,
        __in_ecount(1) const MILCMD_DOUBLEBUFFEREDBITMAP_COPYFORWARD *pCmd
        );

private:

    CSwDoubleBufferedBitmap *   m_pDoubleBufferedBitmap;
    bool                        m_useBackBuffer;
}; // Class CMilSlaveDoubleBufferedBitmap

