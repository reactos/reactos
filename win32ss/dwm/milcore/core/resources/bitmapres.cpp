// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    bitmapres.cpp

Abstract:

    Bitmap resource. This file contains the implementation for all the
    bitmap resource functionality.

Environment:

    User mode only.


--*/

#include "precomp.hpp"

MtDefine(CMilSlaveBitmap, MILRender, "CMilSlaveBitmap");


/*override*/ HRESULT
CMilSlaveBitmap::Draw(
    __in_ecount(1) CDrawingContext *pDC,
    MilBitmapWrapMode::Enum wrapMode
    )
{
    RRETURN(pDC->DrawBitmap(
        this,
        wrapMode
        ));
}

/*++

Routine Description:

Initialize a bitmap from IWGXBitmap.

Arguments:

MILCMD_BITMAP_SOURCE - Packed data structure that contains basic
information about the bitmap.
Return Value:

HRESULT

--*/

HRESULT CMilSlaveBitmap::ProcessSource(
    __in_ecount(1) CMilSlaveHandleTable*,
    __in_ecount(1) const MILCMD_BITMAP_SOURCE* pBmp
    )
{
    HRESULT hr = S_OK;
    IWGXBitmap *pCWICWrapperBitmap = static_cast<IWGXBitmap *>(static_cast<CWICWrapperBitmap *>(pBmp->pIBitmap));

    IFCNULL(pCWICWrapperBitmap);

    ReplaceInterface(m_pIBitmap, pCWICWrapperBitmap);

Cleanup:
    NotifyOnChanged(this);

    //
    // Do not increase the reference count for pCWICWrapperBitmap -- it has
    // already been increased by the transport
    //
    ReleaseInterface(pCWICWrapperBitmap);

    RRETURN(hr);
}

HRESULT CMilSlaveBitmap::ProcessInvalidate(
    __in_ecount(1) CMilSlaveHandleTable*,
    __in_ecount(1) const MILCMD_BITMAP_INVALIDATE* pData
    )
{
    HRESULT hr = S_OK;
    
    if (m_pIBitmap)
    {
        const RECT * pDirtyRect = NULL;

        // Use the dirty rect specified in the payload only if told to.
        if (pData->UseDirtyRect)
        {
            pDirtyRect = &pData->DirtyRect;
        }
        
        IFC(m_pIBitmap->AddDirtyRect(pDirtyRect));
    }

Cleanup:
    NotifyOnChanged(this);
    
    RRETURN(hr);
}

HRESULT CMilSlaveBitmap::GetBounds(
    __in_ecount_opt(1) CContentBounder *pBounder,
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    HRESULT hr = S_OK;
    if (m_pIBitmap)
    {
        Assert(prcBounds);

        IFC(GetBitmapSourceBounds(m_pIBitmap, prcBounds));
    }
    else
    {
        IFC(WGXERR_NOTINITIALIZED);
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

Constructor - initialize the bitmap resource to an empty bitmap for the
given device object.

Return Value:

NONE

--*/

CMilSlaveBitmap::CMilSlaveBitmap(__in_ecount(1) CComposition*)
{
    m_pIBitmap = NULL;
}

/*++

Routine Description:

  Destructor

--*/

CMilSlaveBitmap::~CMilSlaveBitmap()
{
    ReleaseInterface(m_pIBitmap);
}



