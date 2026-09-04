// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_mesh
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGeometry3DDuce);

// Class: CMilGeometry3DDuce
class CMilGeometry3DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGeometry3DDuce));

    CMilGeometry3DDuce(__in_ecount(1) CComposition*)
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GEOMETRY3D;
    }

    virtual HRESULT GetRealization(__deref_out_ecount(1) CMILMesh3D **ppRealization) = 0;
};

