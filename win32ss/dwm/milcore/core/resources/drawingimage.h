// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Unmanaged representation of DrawingImage
//
//------------------------------------------------------------------------

MtExtern(CMilDrawingImageDuce);

// Class: CMilDrawingImageDuce
class CMilDrawingImageDuce : public CMilImageSource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilDrawingImageDuce));

    CMilDrawingImageDuce(__in_ecount(1) CComposition* pComposition) {}
    virtual ~CMilDrawingImageDuce()
    {
        UnRegisterNotifiers();   

    }

public:

    __override virtual bool HasContent() const
    {
        return m_data.m_pDrawing != NULL;
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
        ) const
    {
        *dDpiX = *dDpiY = 96.0f;

        RRETURN(S_OK);
    }
    
    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DRAWINGIMAGE || CMilImageSource::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_DRAWINGIMAGE* pCmd
        );

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable *pHandleTable);
    override void UnRegisterNotifiers();

    CMilDrawingImageDuce_Data m_data;
};

