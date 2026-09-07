// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Unmanaged representation of D3DImage
//
//------------------------------------------------------------------------

MtExtern(CMilD3DImageDuce);

class CMilD3DImageDuce : public CMilImageSource
{
    friend class CResourceFactory;

public:

    __override virtual bool HasContent() const
    {
    	return m_pInteropDeviceBitmap != NULL;
    }

    __override virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDC,
        MilBitmapWrapMode::Enum wrapMode
        );

    __override virtual HRESULT GetBounds(
        __in_ecount_opt(1) CContentBounder *pBounder,
        __out_ecount(1) CMilRectF *prcBounds
        );

    __override virtual HRESULT GetResolution(
        __out_ecount(1) double *dDpiX,
        __out_ecount(1) double *dDpiY
        ) const;

    __override HRESULT GetBitmapSource(
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
        );
	
    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_D3DIMAGE || CMilImageSource::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_D3DIMAGE* pCmd
        );

    HRESULT ProcessPresent(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_D3DIMAGE_PRESENT* pCmd
        );


    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilD3DImageDuce));

    CMilD3DImageDuce(__in_ecount(1) CComposition* pComposition);
    virtual ~CMilD3DImageDuce();

private:

    // Can be NULL as the update packet can contain NULL
    CInteropDeviceBitmap *m_pInteropDeviceBitmap;

    // Only used on synchronous channels or when software fallback is enabled. When this is !NULL, 
    // m_pInteropDeviceBitmap is not rendered.
    IWGXBitmapSource *m_pISoftwareBitmap;
};

