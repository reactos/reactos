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

MtExtern(CMilRotation3DDuce);

// Class: CMilRotation3DDuce
class CMilRotation3DDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilRotation3DDuce));

    CMilRotation3DDuce(__in_ecount(1) CComposition* pComposition)
    {
    }

    virtual ~CMilRotation3DDuce() {}

public:
    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_ROTATION3D;
    }

    virtual HRESULT GetRealization(__out_ecount(1) CMILMatrix *pRealization) = 0;
};

