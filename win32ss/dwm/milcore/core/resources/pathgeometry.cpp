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

#include "precomp.hpp"

MtDefine(PathGeometryResource, MILRender, "PathGeometry Resource");

MtDefine(CMilPathGeometryDuce, PathGeometryResource, "CMilPathGeometryDuce");

CMilPathGeometryDuce::~CMilPathGeometryDuce()
{   
    UnRegisterNotifiers();
}

#define MAX_POINT_DATA_SIZE 16

/*++

Routine Description:

    CMilPathGeometryDuce::GetShapeDataCore

--*/

HRESULT CMilPathGeometryDuce::GetShapeDataCore(
    __deref_out_ecount(1) IShapeData **ppShapeData
    )
{
    Assert(ppShapeData != NULL);

    HRESULT hr = S_OK;

    const CMILMatrix *pMatrix;

    if (m_data.m_pFiguresData == NULL)
    {
        // We haven't been initialized. Use a dummy geometry.

        *ppShapeData = const_cast<CShape *>(CShape::EmptyShape());
    }
    else
    {
        IFC(GetMatrixCurrentValue(m_data.m_pTransform, &pMatrix));

        m_geometryData.SetPathData(
            m_data.m_pFiguresData,
            m_data.m_cbFiguresSize,
            m_data.m_FillRule,
            pMatrix);

        *ppShapeData = DYNCAST(IShapeData, &m_geometryData);
    }

Cleanup:    

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilPathGeometryDuce::Create
//
//-----------------------------------------------------------------------------
HRESULT 
CMilPathGeometryDuce::Create(
     __in_ecount(1) CMilTransformDuce *pTransform,
     MilFillMode::Enum fillRule,
     UINT32 cbFiguresSize,
     __in_bcount(cbFiguresSize) MilPathGeometry *pFiguresData,
    __deref_out CMilPathGeometryDuce **ppPathGeometry
    )
{
    HRESULT hr = S_OK;

    CMilPathGeometryDuce *pPathGeometry = NULL;

    IFCOOM(pPathGeometry = new CMilPathGeometryDuce());    
    pPathGeometry->AddRef();
   
    IFC(pPathGeometry->Initialize(pTransform, fillRule, cbFiguresSize, pFiguresData));

    *ppPathGeometry = pPathGeometry; // Transitioning ref count to out argument
    pPathGeometry = NULL;
   
Cleanup:
    ReleaseInterface(pPathGeometry);
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilPathGeometryDuce::Initialize
//
// Synopsis: Initializes the m_data struct with the argument values.
//
//-----------------------------------------------------------------------------
HRESULT
CMilPathGeometryDuce::Initialize(
     __in_ecount(1) CMilTransformDuce *pTransform,
     MilFillMode::Enum fillRule,
     UINT32 cbFiguresSize,
     __in_bcount(cbFiguresSize) MilPathGeometry *pFiguresData
    )
{
    HRESULT hr = S_OK;

    // Register the transform, it is released in UnRegisterNotifiers.
    IFC(RegisterNotifier(pTransform));
    m_data.m_pTransform = pTransform;
    
    m_data.m_FillRule = fillRule;
    m_data.m_cbFiguresSize = cbFiguresSize;

    // We adopt the reference to pFiguresData. It is released in UnRegisterNotifiers.
    m_data.m_pFiguresData = pFiguresData;
    
    IFC(ValidateData());
    SetDirty(true);

Cleanup:
    RRETURN(hr);  
}

inline HRESULT IncrementPolySegment(
    __inout_ecount(1) BYTE *&pFigureData,
    __inout_ecount(1) size_t &nBytesLeft)
{
    HRESULT hr = S_OK;
    const MilSegmentPoly *pSeg = reinterpret_cast<MilSegmentPoly*>(pFigureData);
    size_t cbPoints = 0;

    IFC(SizeTMult(pSeg->Count, sizeof(MilPoint2D), &cbPoints));
    IFC(SizeTAdd(cbPoints, sizeof(MilSegmentPoly), &cbPoints));

    if (nBytesLeft < cbPoints)
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }
    else
    {
        pFigureData += cbPoints;
        nBytesLeft -= cbPoints;
    }
Cleanup:
    RRETURN(hr);
}



HRESULT
CMilPathGeometryDuce::ProcessUpdateCore()
{
    RRETURN(ValidateData());
}

HRESULT
CMilPathGeometryDuce::ValidateData()
{
    HRESULT hr = S_OK;
    
    MilPathGeometry *pGeometryData = m_data.m_pFiguresData;
    MilPathFigure *pOrigFigureData = NULL;
    BYTE *pFigureData = NULL;
    MilPathFigure *pCurFigure = NULL;
    bool fIsARegion = ((pGeometryData->Flags & MilPathGeometryFlags::IsRegionData) != 0);

    size_t nBytesLeft = m_data.m_cbFiguresSize;


    //
    // Validate the path geometry packet header
    //

    if (nBytesLeft < sizeof(MilPathGeometry)) 
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    pOrigFigureData = reinterpret_cast<MilPathFigure*>(pGeometryData + 1);
    pFigureData = reinterpret_cast<BYTE *>(pOrigFigureData);
    nBytesLeft -= sizeof(MilPathGeometry);


    //
    // Parse the path geometry data.
    //

    const BYTE *pPreviousFigure = pFigureData;

    while (nBytesLeft >= sizeof(MilPathFigure))
    {
        MilSegment *pSegHeader = NULL;
        pCurFigure = reinterpret_cast<MilPathFigure*>(pFigureData);

        //
        // Validate the back pointer
        //

        if (reinterpret_cast<BYTE*>(pCurFigure) - pCurFigure->BackSize != pPreviousFigure)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        pFigureData += sizeof(MilPathFigure);
        nBytesLeft -= sizeof(MilPathFigure);

        UINT nSegments = pCurFigure->Count;


        //
        // Parse the path figure data.
        //

        const BYTE *pPreviousSegment = pFigureData;

        while ((nBytesLeft >= sizeof(MilSegment)) && nSegments > 0)
        {
            pSegHeader = reinterpret_cast<MilSegment *>(pFigureData);
            if (fIsARegion && (pSegHeader->Type != MilSegmentType::PolyLine))
            {
                IFC(WGXERR_UCE_MALFORMEDPACKET);
            }

            //
            // Validate the back pointer
            //

            if (reinterpret_cast<BYTE*>(pSegHeader) - pSegHeader->BackSize != pPreviousSegment)
            {
                IFC(WGXERR_UCE_MALFORMEDPACKET);
            }

            switch (pSegHeader->Type)
            {
            case MilSegmentType::None:
                pFigureData += sizeof(MilSegment);
                nBytesLeft -= sizeof(MilSegment);
                break;

            case MilSegmentType::Line:
                if (nBytesLeft < sizeof(MilSegmentLine))
                {
                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                }
                else
                {
                    pFigureData += sizeof(MilSegmentLine);
                    nBytesLeft -= sizeof(MilSegmentLine);
                }
                break;

            case MilSegmentType::Bezier:
                if (nBytesLeft < sizeof(MilSegmentBezier))
                {
                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                }
                else
                {
                    pFigureData += sizeof(MilSegmentBezier);
                    nBytesLeft -= sizeof(MilSegmentBezier);
                }
                break;

            case MilSegmentType::QuadraticBezier:
                if (nBytesLeft < sizeof(MilSegmentQuadraticBezier))
                {
                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                }
                else
                {
                    pFigureData += sizeof(MilSegmentQuadraticBezier);
                    nBytesLeft -= sizeof(MilSegmentQuadraticBezier);
                }
                break;

            case MilSegmentType::Arc:
                if (nBytesLeft < sizeof(MilSegmentArc))
                {
                    IFC(WGXERR_UCE_MALFORMEDPACKET);
                }
                else
                {
                    pFigureData += sizeof(MilSegmentArc);
                    nBytesLeft -= sizeof(MilSegmentArc);
                }
                break;

            case MilSegmentType::PolyLine:
                {
                    const MilSegmentPoly *pPoly = reinterpret_cast<MilSegmentPoly*>(pFigureData);
                    if ((pPoly->Count == 0) || (fIsARegion && (pPoly->Count != 3)))
                    {
                        IFC(WGXERR_UCE_MALFORMEDPACKET);
                    }
                    IFC(IncrementPolySegment(pFigureData, nBytesLeft));
                }
                break;

            case MilSegmentType::PolyBezier:
                {
                    const MilSegmentPoly *pPoly = reinterpret_cast<MilSegmentPoly*>(pFigureData);
                    UINT cPoints = pPoly->Count;
                    if ((cPoints == 0) || ((cPoints % 3) != 0))
                    {
                        IFC(WGXERR_UCE_MALFORMEDPACKET);
                    }
                    IFC(IncrementPolySegment(pFigureData, nBytesLeft));
                }
                break;

            case MilSegmentType::PolyQuadraticBezier:
                {
                    const MilSegmentPoly *pPoly = reinterpret_cast<MilSegmentPoly*>(pFigureData);
                    UINT cPoints = pPoly->Count;
                    if ((cPoints == 0) || ((cPoints & 1) != 0))
                    {
                        IFC(WGXERR_UCE_MALFORMEDPACKET);
                    }
                    IFC(IncrementPolySegment(pFigureData, nBytesLeft));
                }
                break;

            default:
                IFC(WGXERR_UCE_MALFORMEDPACKET);
            }

            pPreviousSegment = reinterpret_cast<BYTE*>(pSegHeader);
            
            nSegments--;
        }

        pPreviousFigure = reinterpret_cast<BYTE*>(pCurFigure);

        //
        // Validate OffsetToLastSegment
        //

        if (pSegHeader &&
            (reinterpret_cast<BYTE*>(pCurFigure) + pCurFigure->OffsetToLastSegment != 
            reinterpret_cast<BYTE*>(pSegHeader)))
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        //
        // Make sure that we have parsed as many segments as advertised.
        //

        if (nSegments != 0)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }


    //
    // Make sure that we did not ignore any data from the command payload.
    //

    if (nBytesLeft > 0)
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }


Cleanup:
    RRETURN(hr);
}

