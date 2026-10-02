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

MtExtern(CMilTransformDuce);

// Class: CMilTransformDuce
class CMilTransformDuce : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilTransformDuce));

    CMilTransformDuce(__in_ecount(1) CComposition*)
    {
        SetDirty(TRUE);
    }

    CMilTransformDuce() { };

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_TRANSFORM;
    }

    HRESULT GetMatrix(OUT CMILMatrix const **ppMatrix);

protected:

    virtual HRESULT GetMatrixCore(CMILMatrix *pMatrix) = 0;

protected:

    override BOOL OnChanged(
        CMilSlaveResource *pSender,
        NotificationEventArgs::Flags e
        )
    {
        SetDirty(TRUE);
        return TRUE;
    }

private:
    CMILMatrix m_matrix;
};

