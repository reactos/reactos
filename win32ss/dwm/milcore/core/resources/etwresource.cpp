// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//  etwresource.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSlaveEtwEventResource, MILRender, "CSlaveEtwEventResource");

//---------------------------------------------------------------------------------
//
// class CSlaveEtwEventResource
//
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// CSlaveEtwEventResource::Intialize
//---------------------------------------------------------------------------------

HRESULT
CSlaveEtwEventResource::Initialize()
{
    // Call base class
    CMilSlaveResource::Initialize();
    m_pDevice->AddRef();
    RRETURN(m_pDevice->AddEtwEvent(this));
}

//---------------------------------------------------------------------------------
// CSlaveEtwEventResource::dtor
//---------------------------------------------------------------------------------

CSlaveEtwEventResource::~CSlaveEtwEventResource()
{
    m_pDevice->RemoveEtwEvent(this);
    ReleaseInterface(m_pDevice);
}

//---------------------------------------------------------------------------------
// CSlaveEtwEventResource::ProcessUpdate
//---------------------------------------------------------------------------------

HRESULT
CSlaveEtwEventResource::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
    __in_ecount(1) const MILCMD_ETWEVENTRESOURCE* pCmd
    )
{
    m_dwId = pCmd->id;

    //
    // Ignore any empty packets
    //

    if (m_dwId != 0)
    {
        m_fNeedToRaiseEvent = TRUE;
    }

    return S_OK;
}

//---------------------------------------------------------------------------------
// CSlaveEtwEventResource::OutputEvent
//---------------------------------------------------------------------------------

VOID
CSlaveEtwEventResource::OutputEvent()
{
    if (m_fNeedToRaiseEvent)
    {
        //
        // ETW Windows Response trace event
        //

        EventWriteWClientUceResponse(m_dwId);

        m_fNeedToRaiseEvent = FALSE;
    }
}



