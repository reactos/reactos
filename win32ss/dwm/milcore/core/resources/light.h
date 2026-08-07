// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilLightDuce);

// Class: CMilLightDuce
class CMilLightDuce : public CMilModel3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilLightDuce));

    CMilLightDuce(__in_ecount(1) CComposition* pComposition)
        : CMilModel3DDuce(pComposition)
    {
    }

public:

    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_LIGHT || CMilModel3DDuce::IsOfType(type);
    }
};


