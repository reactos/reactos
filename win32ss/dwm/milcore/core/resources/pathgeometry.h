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

MtExtern(CMilPathGeometryDuce);

// Class: CMilPathGeometryDuce
class CMilPathGeometryDuce : public CMilGeometryDuce
{
    friend class CResourceFactory;
    // Required so that CGlyphRunResource can create a PathGeometry directly
    // when using geometry realizations for glyphs. 
    friend class CGlyphRunResource;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilPathGeometryDuce));

    CMilPathGeometryDuce(__in_ecount(1) CComposition* pComposition)
        : CMilGeometryDuce(pComposition)
    {

    }

    virtual ~CMilPathGeometryDuce();

    CMilPathGeometryDuce() {};

    HRESULT Initialize(
         __in_ecount(1) CMilTransformDuce *pTransform,
         MilFillMode::Enum fillRule,
         UINT32 cbFiguresSize,
         __in_bcount(cbFiguresSize) MilPathGeometry *pFiguresData
        );
    
public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_PATHGEOMETRY || CMilGeometryDuce::IsOfType(type);
    }

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_PATHGEOMETRY* pCmd,
        __in_bcount(cbPayload) LPCVOID pPayload,
        UINT cbPayload
        );

    HRESULT ProcessUpdateCore();

    HRESULT RegisterNotifiers(__in_ecount(1) CMilSlaveHandleTable* pHandleTable);
    override void UnRegisterNotifiers();

    static HRESULT Create(
         __in_ecount(1) CMilTransformDuce *pTransform,
         MilFillMode::Enum fillRule,
         UINT32 cbFiguresSize,
         __in_bcount(cbFiguresSize) MilPathGeometry *pFiguresData,
        __deref_out CMilPathGeometryDuce **ppPathGeometry
        );

private:
    HRESULT ValidateData();
    
    PathGeometryData m_geometryData;
public:
    virtual HRESULT GetShapeDataCore(__deref_out_ecount(1) IShapeData **);

    CMilPathGeometryDuce_Data m_data;
    
};

