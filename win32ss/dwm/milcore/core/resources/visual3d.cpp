// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      Visual3D.cpp
//
// Abstract: 
//      Visual3D resource.
//
//---------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilVisual3D, MILRender, "CMilVisual3D");

//---------------------------------------------------------------------------------
//
// class CMilVisual3D
//
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// CMilVisual3D::dtor
//---------------------------------------------------------------------------------

CMilVisual3D::~CMilVisual3D()
{
    RemoveAllChildren();

    UnRegisterNotifier(m_pContent);
    UnRegisterNotifier(m_pTransform);
}

//---------------------------------------------------------------------------------
// CMilVisual3D::IsParent3D
//
//    Returns true if the parent is a CMilVisual3D (or NULL), false if 2D.
//---------------------------------------------------------------------------------

bool
CMilVisual3D::IsParent3D() const
{
    if (m_pParent == NULL || m_pParent->IsOfType(TYPE_VISUAL3D))
    {
        return true;
    }
    else
    {
        Assert(m_pParent->IsOfType(TYPE_VIEWPORT3DVISUAL));

        return false;
    }
}

//---------------------------------------------------------------------------------
// CMilVisual3D::GetParent2D
//
//    Returns the 2D parent of this node.  Use IsParent3D before calling.
//---------------------------------------------------------------------------------

__out_ecount(1) CMilVisual*
CMilVisual3D::GetParent2D()
{
    Assert(m_pParent != NULL && m_pParent->IsOfType(TYPE_VIEWPORT3DVISUAL));
    
    return DYNCAST(CMilVisual, m_pParent);
}

//---------------------------------------------------------------------------------
// CMilVisual3D::GetParent3D
//
//    Returns the 3D parent of this node.  Use IsParent3D before calling.  May be NULL.
//---------------------------------------------------------------------------------

__out_ecount_opt(1) CMilVisual3D*
CMilVisual3D::GetParent3D()
{
    Assert(m_pParent == NULL || m_pParent->IsOfType(TYPE_VISUAL3D));
    
    return DYNCAST(CMilVisual3D, m_pParent);
}

//---------------------------------------------------------------------------------
// CMilVisual3D::ValidateNode
//
//    Helper function for decoding command packets send to composition or window 
//    nodes. 
//
// Returns: WGXERR_UCE_MALFORMEDPACKET if the pNode argument is not a valid node type.
//---------------------------------------------------------------------------------

/* static */ HRESULT
CMilVisual3D::ValidateNode(
    __in_ecount_opt(1) const CMilVisual3D* pNode
    )
{
    if ((pNode == NULL) || !pNode->IsOfType(TYPE_VISUAL3D))
    {
        RRETURN(WGXERR_UCE_MALFORMEDPACKET);
    }
    else
    {
        RRETURN(S_OK);
    }     
}

//---------------------------------------------------------------------------------
// CMilVisual3D::PropagateFlags (static)
//
//      Ensures that the nodes from this node up the parent chain
//      are marked with the specified flags.
//---------------------------------------------------------------------------------

void
CMilVisual3D::PropagateFlags(
    __in_ecount(1) CMilVisual3D* pNode,
    BOOL fNeedsBoundingBoxUpdate,
    BOOL fDirtyForRender,
    BOOL fAdditionalDirtyRegion // Default value FALSE.
    )
{
    AssertMsg(fNeedsBoundingBoxUpdate || fDirtyForRender,
        "We shouldn't call this function in the first place if there is nothing to propagate.");

    //
    //  We do not yet keep dirty bits on Visual3D so we walk up the 3D parents
    //  until we find our Viewport3DVisual and call PropagateFlags there.
    //
    {
        CMilVisual3D *pParent;

        while (pNode->IsParent3D() && (pParent = pNode->GetParent3D()) != NULL)
        {
            pNode = pParent;
        }
    }

    if (pNode != NULL && !(pNode->IsParent3D()))
    {
        CMilVisual *pParent = pNode->GetParent2D();
        Assert(pParent);

        pParent->PropagateFlags(pParent, fNeedsBoundingBoxUpdate, fDirtyForRender, fAdditionalDirtyRegion);
        pParent->m_fHasContentChanged = TRUE;
    }
}

//---------------------------------------------------------------------------------
// CMilVisual3D::OnChanged (virtual)
//---------------------------------------------------------------------------------

BOOL
CMilVisual3D::OnChanged(
    CMilSlaveResource *pSender,
    NotificationEventArgs::Flags e)
{
    CMilVisual3D::PropagateFlags(
        this,
        TRUE  /* Needs bbox update. */ ,
        TRUE /* Needs to be added to dirty region. */);
    return FALSE;
}

//---------------------------------------------------------------------------------
// CMilVisual3D::SetParent
//---------------------------------------------------------------------------------

void
CMilVisual3D::SetParent(
    __in_ecount_opt(1) CMilSlaveResource *pParentNode
    )
{
    // We expect that a node is first disconnected before it is connected to another
    // node.
    Assert(m_pParent == NULL || pParentNode == NULL);

    // A Visual3D may be parented to nothing, another Visual3D, or a Viewport3DVisual.
    Assert(pParentNode == NULL ||
        pParentNode->IsOfType(TYPE_VISUAL3D) ||
        pParentNode->IsOfType(TYPE_VIEWPORT3DVISUAL));

    // Note that the parent is not add-refed to avoid circular references.
    // The child is kept alive by the parent node and therefore addref'd by the parent.

    m_pParent = pParentNode;
}

//---------------------------------------------------------------------------------
// CMilVisual3D::InsertChildAt
//---------------------------------------------------------------------------------

HRESULT
CMilVisual3D::InsertChildAt(
    __inout_ecount(1) CMilVisual3D *pNewChild, 
    UINT iPosition
    )
{
    HRESULT hr = S_OK;
    
    IFC(m_rgpChildren.InsertAt(pNewChild, iPosition));
    
    pNewChild->AddRef();
    pNewChild->SetParent(this);

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilVisual3D::RemoveChild
//---------------------------------------------------------------------------------

HRESULT
CMilVisual3D::RemoveChild(
    __inout_ecount(1) CMilVisual3D *pChild
    )
{
    HRESULT hr = S_OK;

    if (!m_rgpChildren.Remove(pChild))
    {
        IFC(E_INVALIDARG);
    }

    pChild->SetParent(NULL);
    pChild->Release();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilVisual3D::RemoveAllChildren
//---------------------------------------------------------------------------------

VOID
CMilVisual3D::RemoveAllChildren()
{
    UINT count = static_cast<UINT>(m_rgpChildren.GetCount());
    for (UINT i = 0; i < count; i++)
    {
        CMilVisual3D *pChild = m_rgpChildren[i];
        if (pChild)
        {
            pChild->SetParent(NULL);
            pChild->Release();
        }
    }
    m_rgpChildren.Clear();
}

//---------------------------------------------------------------------------------
// IGraphNode Interface
//      CMilVisual3D::GetChildAt
//---------------------------------------------------------------------------------

override IGraphNode* CMilVisual3D::GetChildAt(UINT index)
{
    if (m_rgpChildren.GetCount() <= index)
    {
        return NULL;
    }
    else
    {
        return m_rgpChildren[index];
    }
}

//+------------------------------------------------------------------------
//  IGraphNode Interface:  
//      CMilVisual3D::EnterNode
//
//  Synopsis:
//      This is used for cycle detection. Currently we ignore cycles.
//      A count is maintained. The count can only go upto 2 as when the 
//      node tries to enter the second time (loop!!!) it should not 
//      be able to enter and LeaveNode() should be called.
//      Each call to this function should match a call to LeaveNode()
//      It calls the base functions defined in Resslave.h
//
//  Example Usage:
//      To implement this check for cycles, these functions are used as follows:-
//      if (EnterNode())
//      {
//          ...
//      }
//
//      LeaveNode();
//-------------------------------------------------------------------------

override bool CMilVisual3D::EnterNode()
{
    return EnterResource();
}

override void CMilVisual3D::LeaveNode()
{
    LeaveResource();
}

override bool CMilVisual3D::CanEnterNode() const
{
    return CanEnterResource();
}

// ----------------------------------------------------------------------------
//
//   Command handlers
//
// ----------------------------------------------------------------------------

HRESULT 
CMilVisual3D::ProcessSetTransform(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL3D_SETTRANSFORM* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource
    CMilTransform3DDuce *pTransform = NULL;
    if (pCmd->hTransform != HMIL_RESOURCE_NULL)
    {
        pTransform = 
            static_cast<CMilTransform3DDuce*>(pHandleTable->GetResource(
                pCmd->hTransform, 
                TYPE_TRANSFORM3D
                ));

        if (pTransform == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (pTransform != m_pTransform)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pTransform));
        UnRegisterNotifier(m_pTransform);
        m_pTransform = pTransform;

        // Mark the node as dirty and propagate flags
        CMilVisual3D::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual3D::ProcessSetContent(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL3D_SETCONTENT* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource
    CMilModel3DDuce *pContent = NULL;
    if (pCmd->hContent != HMIL_RESOURCE_NULL)
    {
        pContent = 
            static_cast<CMilModel3DDuce*>(pHandleTable->GetResource(
                pCmd->hContent, 
                TYPE_MODEL3D
                ));

        if (pContent == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

    }

    if (pContent != m_pContent)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pContent));
        UnRegisterNotifier(m_pContent);
        m_pContent = pContent;

        // Mark the node as dirty and propagate flags
        CMilVisual3D::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual3D::ProcessRemoveAllChildren(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL3D_REMOVEALLCHILDREN* pCmd
    )
{
    RemoveAllChildren();
    CMilVisual3D::PropagateFlags(this, TRUE, TRUE);

    return S_OK;
}

HRESULT 
CMilVisual3D::ProcessRemoveChild(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL3D_REMOVECHILD* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisual3D* pChild =
        static_cast<CMilVisual3D*>(pHandleTable->GetResource(
            pCmd->hChild, 
            TYPE_VISUAL3D
            ));

    Assert(pChild);
    IFC(CMilVisual3D::ValidateNode(pChild));

    IFC(RemoveChild(pChild));

    //
    // This causes us to re-render too much. Possible
    // optimization is to put the child bbox on the parent.
    //

    CMilVisual3D::PropagateFlags(this, TRUE, TRUE);

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual3D::ProcessInsertChildAt(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL3D_INSERTCHILDAT* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisual3D* pChild =
        static_cast<CMilVisual3D*>(pHandleTable->GetResource(
            pCmd->hChild, 
            TYPE_VISUAL3D
            ));

    Assert(pChild);
    IFC(CMilVisual3D::ValidateNode(pChild));

    IFC(InsertChildAt(pChild, pCmd->index));

    CMilVisual3D::PropagateFlags(this, TRUE, FALSE);
    CMilVisual3D::PropagateFlags(pChild, FALSE, TRUE);

Cleanup:
    RRETURN(hr);
}



