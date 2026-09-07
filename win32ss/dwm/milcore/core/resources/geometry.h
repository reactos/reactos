// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGeometryDuce);

// Class: CMilGeometryDuce
class CMilGeometryDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometryDuce));

    CMilGeometryDuce(__in_ecount(1) CComposition*)
    {
        SetDirty(TRUE);
    }

    CMilGeometryDuce() { };

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRY;
    }

private:
    IShapeData *m_pCachedShapeData;

protected:
    
    override BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        )
    {
        SetDirty(TRUE);
        return TRUE;
    }

public:
    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **) = 0;

    HRESULT GetShapeData(OUT IShapeData ** ppShapeData);

    HRESULT GetBounds(CMilRectF *pRect);

    // Returns infinite bounds upon encountering numerical error.
    HRESULT GetBoundsSafe(CMilRectF *pRect);

    // GetPointAtLengthFraction - Used by PathAnimations
    HRESULT GetPointAtLengthFraction(
        DOUBLE rFraction,
        OUT MilPoint2D &pt,
        OUT MilPoint2D *pvecTangent
        );
};

