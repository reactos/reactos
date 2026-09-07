// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      Visual3D.h
//
// Abstract: 
//      Visual3D resource.
//
//---------------------------------------------------------------------------

MtExtern(CMilVisual3D);

//---------------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------------

class CMilTransformDuce;
class CMilBrushDuce;
class CGuidelineCollection;

//---------------------------------------------------------------------------------
// class CMilVisual3D
//---------------------------------------------------------------------------------

class CMilVisual3D : 
    public IGraphNode, 
    public CMilSlaveResource
{
    friend class CMilViewport3DVisual;
    friend class CPrerender3DContext;
    friend class CRender3DContext;
    friend class CResourceFactory;
    friend class CGraphWalker;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilVisual3D));

    CMilVisual3D(__in_ecount(1) CComposition* pComposition) {}

    virtual ~CMilVisual3D();

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_VISUAL3D;
    }

    override virtual BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        );

    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessSetTransform(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL3D_SETTRANSFORM* pCmd
        );

    HRESULT ProcessRemoveAllChildren(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL3D_REMOVEALLCHILDREN* pCmd
        );

    HRESULT ProcessRemoveChild(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL3D_REMOVECHILD* pCmd
        );

    HRESULT ProcessInsertChildAt(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL3D_INSERTCHILDAT* pCmd
        );

    HRESULT ProcessSetContent(
        __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
        __in_ecount(1) const MILCMD_VISUAL3D_SETCONTENT* pCmd
        );

public:
    // 
    // IGraphNode interface implementation.

    override UINT GetChildrenCount() const { return static_cast<UINT>(m_rgpChildren.GetCount()); }
    override IGraphNode* GetChildAt(UINT index);
    override bool EnterNode();
    override void LeaveNode();
    override bool CanEnterNode() const;

protected:
    void SetParent(
        __in_ecount_opt(1) CMilSlaveResource *pParentNode
        );

    HRESULT InsertChildAt(
        __inout_ecount(1) CMilVisual3D *pNewChild,
        UINT iPosition
        );

    HRESULT RemoveChild(
        __inout_ecount(1) CMilVisual3D *pChild
        );

    VOID RemoveAllChildren();

    bool IsParent3D() const;
    __out_ecount(1) CMilVisual *GetParent2D();
    __out_ecount_opt(1) CMilVisual3D *GetParent3D();

    static void PropagateFlags(
        __in_ecount(1) CMilVisual3D *pNode,
        BOOL fNeedsBoundingBoxUpdate,
        BOOL fDirtyForRender,
        BOOL fAdditionalDirtyRegion = FALSE);

    static HRESULT ValidateNode(
        __in_ecount_opt(1) const CMilVisual3D* pNode
        );
    
private:
    CMilModel3DDuce *m_pContent;  // Points to a Model3D
    CMilSlaveResource * m_pParent;
    CMilTransform3DDuce *m_pTransform;
    CPtrArray<CMilVisual3D> m_rgpChildren;
};


