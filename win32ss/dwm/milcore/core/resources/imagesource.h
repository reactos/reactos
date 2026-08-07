// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//------------------------------------------------------------------------

// No metering needed because the class is abstract

// Class: CMilImageSource
class CMilImageSource : public CMilSlaveResource
{
public:

    // Returns true if the ImageSource's content is non-NULL
    virtual bool HasContent() const = 0;

    // Image is capable of being drawn to an intermediate
    virtual bool CanDrawToIntermediate() { return true; }

    // Use the DrawingContext to draw the image
    virtual HRESULT Draw(
        __in_ecount(1) CDrawingContext *pDC,
        MilBitmapWrapMode::Enum wrapMode
        ) = 0;

    // Returns the bounds of the ImageSource (use pBounder if necessary)
    virtual HRESULT GetBounds(
        __in_ecount_opt(1) CContentBounder* pBounder,
        __out_ecount(1) CMilRectF *prcBounds
        ) = 0;

    // Returns the resolution of the ImageSource
    virtual HRESULT GetResolution(
        __out_ecount(1) double *dDpiX,
        __out_ecount(1) double *dDpiY
        ) const = 0;

    virtual HRESULT GetBitmapSource(
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppIWGXBitmapSource
        ) 
    {
        *ppIWGXBitmapSource = NULL;
        return S_OK;
    }

    // Returns true if this imagesource creates its bitmap on the fly and therefore
    // GetBitmapSource should be called on each frame as the bitmap may have changed.
    virtual bool IsDynamicBitmap()
    {
        return false;
    }

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_IMAGESOURCE;
    }
    
protected:

    CMilImageSource() {}
    virtual ~CMilImageSource() {}
};

