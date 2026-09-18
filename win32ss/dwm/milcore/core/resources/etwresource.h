// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    etwresource.h
//
//---------------------------------------------------------------------------------

MtExtern(CSlaveEtwEventResource);

//---------------------------------------------------------------------------------
// class CSlaveEtwEventResource
//---------------------------------------------------------------------------------

class CSlaveEtwEventResource : public CMilSlaveResource
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSlaveEtwEventResource));

    CSlaveEtwEventResource(__in_ecount(1) CComposition* pComposition)
    {
        m_pDevice = pComposition;
    }

    virtual ~CSlaveEtwEventResource();

private:

    HRESULT Initialize();

public:

    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_ETWEVENTRESOURCE;
    }

    virtual VOID OutputEvent();


    // ------------------------------------------------------------------------
    //
    //   Command handlers
    //
    // ------------------------------------------------------------------------

    HRESULT ProcessUpdate(
        __in_ecount(1) CMilSlaveHandleTable *pHandleTable,
        __in_ecount(1) const MILCMD_ETWEVENTRESOURCE* pCmd
        );

private:
    CComposition *m_pDevice;
    DWORD m_dwId;
    DWORD m_fNeedToRaiseEvent;
};



