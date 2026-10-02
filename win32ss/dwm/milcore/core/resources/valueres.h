// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Abstract:

    Value resources.

--*/

#pragma once

MtExtern(TMilSlaveValue);

//+-----------------------------------------------------------------------------
//
//  Class:      TMilSlaveValue
//
//------------------------------------------------------------------------------
template<class TValue, class TCommand, MIL_RESOURCE_TYPE ResType>
class TMilSlaveValue : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(TMilSlaveValue));

    TMilSlaveValue(__in_ecount(1) CComposition*);

    virtual ~TMilSlaveValue() {}

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == ResType;
    }

    //
    // Interface for compositor objects
    //

    inline HRESULT GetValue(TValue * pValue) const
    {
        *pValue = m_value;
        INLINED_RRETURN(S_OK);
    }

    inline TValue *GetValue()
    {
        return &m_value;
    }


    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const TCommand* pCmd
        );

protected:

    //
    // Internal methods
    //

    inline HRESULT SetValue(TValue * pValue)
    {
        m_value = *pValue;
        INLINED_RRETURN(S_OK);
    }

    static inline TValue Zero() { TValue ret = {0}; return ret; }

    //
    // Data members
    //

    TValue m_value;
};

//+-----------------------------------------------------------------------------
//
//  Method:     TMilSlaveValue::TMilSlaveValue
//
//------------------------------------------------------------------------------
template <class TValue, class TCommand, MIL_RESOURCE_TYPE ResType>
TMilSlaveValue<TValue, TCommand, ResType>::TMilSlaveValue(
    __in_ecount(1) CComposition*
    )
{
    //
    // Zero initialize data
    //

    m_value = Zero();
}

//+-----------------------------------------------------------------------------
//
//  Method:     TMilSlaveValue::ProcessUpdate
//
//------------------------------------------------------------------------------
template <class TValue, class TCommand, MIL_RESOURCE_TYPE ResType>
HRESULT
TMilSlaveValue<TValue, TCommand, ResType>::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_ecount(1) const TCommand* pCmd
    )
{
    C_ASSERT(sizeof(pCmd->Value) == sizeof(TValue));

    RtlCopyMemory(&m_value, &pCmd->Value, sizeof(TValue));

    NotifyOnChanged(this);

    return S_OK;
}

//
// Typedefs for value resource instantiations
//

typedef TMilSlaveValue<double,             MILCMD_DOUBLERESOURCE,           TYPE_DOUBLERESOURCE>            CMilSlaveDouble;
typedef TMilSlaveValue<MilColorF,         MILCMD_COLORRESOURCE,            TYPE_COLORRESOURCE>             CMilSlaveColor;
typedef TMilSlaveValue<MilPoint2D,       MILCMD_POINTRESOURCE,            TYPE_POINTRESOURCE>             CMilSlavePoint;
typedef TMilSlaveValue<MilPointAndSizeD,          MILCMD_RECTRESOURCE,             TYPE_RECTRESOURCE>              CMilSlaveRect;
typedef TMilSlaveValue<MilSizeD,          MILCMD_SIZERESOURCE,             TYPE_SIZERESOURCE>              CMilSlaveSize;
typedef TMilSlaveValue<MilMatrix3x2D,     MILCMD_MATRIXRESOURCE,           TYPE_MATRIXRESOURCE>            CMilSlaveMatrix;
typedef TMilSlaveValue<MilPoint3F,       MILCMD_POINT3DRESOURCE,          TYPE_POINT3DRESOURCE>           CMilSlavePoint3D;
typedef TMilSlaveValue<MilPoint3F,       MILCMD_VECTOR3DRESOURCE,         TYPE_VECTOR3DRESOURCE>          CMilSlaveVector3D;
typedef TMilSlaveValue<MilQuaternionF,  MILCMD_QUATERNIONRESOURCE,       TYPE_QUATERNIONRESOURCE>        CMilSlaveQuaternion;


