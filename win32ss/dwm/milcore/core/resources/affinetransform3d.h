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

MtExtern(CMilAffineTransform3DDuce);

// Class: CMilAffineTransform3DDuce
class CMilAffineTransform3DDuce : public CMilTransform3DDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilAffineTransform3DDuce));

    CMilAffineTransform3DDuce(__in_ecount(1) CComposition* pComposition)
        : CMilTransform3DDuce(pComposition)
    {
    }

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_AFFINETRANSFORM3D || CMilTransform3DDuce::IsOfType(type);
    }
};


