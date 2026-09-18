// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_transform
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilTransform3DDuce);

// Class: CMilTransform3DDuce
class CMilTransform3DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTransform3DDuce));

    CMilTransform3DDuce(__in_ecount(1) CComposition*)
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TRANSFORM3D;
    }

    virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization) = 0;
    virtual HRESULT Append(__inout_ecount(1) CMILMatrix *pMat) = 0;
};

