// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Name: ETWtrace.cpp
//
//  Description: Tracing code provider class
//
//------------------------------------------------------------------------
#include "precomp.hpp"
//+-----------------------------------------------------------------------
//
//  Member:    CETWTraceProvider::Register
//
//  Synopsis:  Registers the event guids with ETW.
//
//  Returns:   Success if registration completed.
//
//------------------------------------------------------------------------
HRESULT
CETWTraceProvider::Register()
{
    HRESULT hr = S_OK;

    // Register as an event provider.  RegisterTraceGuids
    // also creates a seperate thread that calls ControlCallback
    // when the provider is enabled/disabled.
    if (m_hRegistration == INVALID_TRACEHANDLE_VALUE)
    {
        IFCW32(ERROR_SUCCESS ==
            RegisterTraceGuids (
                    static_cast<WMIDPREQUEST>(CETWTraceProvider::ControlCallback), // enable/disable function
                    this,                // RequestContext parameter
                    &m_guidProvider,     // provider GUID
                    m_cGuids,            // Number of trace guids
                    m_pGuidTrace,        // array of TraceGuidReg structures
                    NULL,                // reserved
                    NULL,                // reserved
                    &m_hRegistration     // handle required to unregister
                    )
               );
    }


  Cleanup:
    return hr;

}
//+-----------------------------------------------------------------------
//
//  Member:    CETWTraceProvider::UnInitialize
//
//  Synopsis:  Unregisters the event provider with ETW
//
//  Returns:   Success if trace provider was unregistered successfully
//
//------------------------------------------------------------------------

HRESULT CETWTraceProvider::UnRegister()
{
    HRESULT hr = S_OK;

    if (m_hRegistration != INVALID_TRACEHANDLE_VALUE)
    {
        IFCW32(ERROR_SUCCESS == UnregisterTraceGuids(m_hRegistration));
        m_hRegistration = INVALID_TRACEHANDLE_VALUE;
    }

  Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    CETWTraceProvider::ControlCallback
//
//  Synopsis:  Callback function called by the ETW-created thread
//              to inform us of status changes (e.g. enabled/disable)
//
//  Returns:   Success if handled
//
//------------------------------------------------------------------------
ULONG WINAPI
CETWTraceProvider::ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID pvContext,
    IN ULONG * /* pulBufferSize*/,
    IN PVOID pvBuffer
    )
{
    CETWTraceProvider *pThis = reinterpret_cast<CETWTraceProvider *>(pvContext);
    ULONG ulReturn = ERROR_INVALID_PARAMETER;

    if (pThis)
    {
        switch (RequestCode)
        {
        case WMI_ENABLE_EVENTS:  //Enable Provider.
            pThis->m_hLogSession = GetTraceLoggerHandle(pvBuffer);
            pThis->m_uchLevel = GetTraceEnableLevel(pThis->m_hLogSession);
            pThis->m_ulFlags = GetTraceEnableFlags(pThis->m_hLogSession);
            ulReturn = ERROR_SUCCESS;
            break;

        case WMI_DISABLE_EVENTS:  //Disable Provider.
            pThis->m_hLogSession = INVALID_TRACEHANDLE_VALUE;
            ulReturn = ERROR_SUCCESS;
            break;
        }
    }

    return ulReturn;
}

//+-----------------------------------------------------------------------
//
//  Member:  CETWTraceProvider::CETWTraceProvider
//
//  Synopsis: Constructor
//
//------------------------------------------------------------------------

CETWTraceProvider::CETWTraceProvider(
    GUID guidProvider,
    __in_ecount(cGuids) TRACE_GUID_REGISTRATION *pGuidTrace,
    UINT cGuids
    ) :     
    m_guidProvider(guidProvider),
    m_pGuidTrace(pGuidTrace),
    m_cGuids(cGuids),
    m_uchLevel(TRACE_LEVEL_INFORMATION),
    m_ulFlags(0xFFFFFFFF),
    m_hRegistration(INVALID_TRACEHANDLE_VALUE),
    m_hLogSession(INVALID_TRACEHANDLE_VALUE)
{
}
//+-----------------------------------------------------------------------
//
//  Member: CETWTraceProvider::~CETWTraceProvider
//
//  Synopsis: Destructor
//
//------------------------------------------------------------------------
CETWTraceProvider::~CETWTraceProvider()
{
    UnRegister();
}

//+-----------------------------------------------------------------------
//
//  Member: CETWTraceProvider::TraceEvent 
//
//  Synopsis:  Trace a simple event
//
//------------------------------------------------------------------------
void
CETWTraceProvider::TraceEvent(
    IN GUID eventGuid,
    IN UCHAR uchEtwType,  //Type of ETW Event (START, STOP, INFO)
    IN PVOID pvData,  // Binary data being sent with trace
    IN UINT cbData  // Size of binary data
    )
{
    struct CUSTOMTRACEEVENT : public EVENT_TRACE_HEADER
    {
        MOF_FIELD aMofField[MAX_MOF_FIELDS];
    };

    CUSTOMTRACEEVENT event;   // Trace event
    ZEROMEM(event);

    event.Guid = eventGuid;

    //initialize the header
    event.Class.Type = uchEtwType;
    event.Class.Version = 0;
    event.Flags = WNODE_FLAG_TRACED_GUID |  WNODE_FLAG_USE_MOF_PTR;
    event.Size = sizeof(EVENT_TRACE_HEADER);

    if (cbData > 0)
    {
        // Set event data
        Assert(NULL != pvData);
        event.aMofField[0].DataPtr = reinterpret_cast<ULONGLONG>(pvData);
        event.aMofField[0].Length  = cbData;
        event.Size += static_cast<USHORT>(sizeof(MOF_FIELD));
    }

    TraceEvent(&event);

}


