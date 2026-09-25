// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Name: etwtrace.h
//
//  Description:   Base ETW tracing provider
//
//------------------------------------------------------------------------

#pragma once

#include <wmistr.h>
#include <evntrace.h>
#define  EVENT_TRACE_FLAG_NONE 0x0
#define  EVENT_TRACE_FLAG_ALL 0xFFFFFFFF

class CETWTraceProvider
{
  public:
    static const TRACEHANDLE INVALID_TRACEHANDLE_VALUE = reinterpret_cast<TRACEHANDLE>(INVALID_HANDLE_VALUE);
    
    CETWTraceProvider(GUID guidProvider,
                  __in_ecount(cGuids) TRACE_GUID_REGISTRATION *pguidTrace,
                  UINT cGuids);

    virtual ~CETWTraceProvider();

    static ULONG WINAPI ControlCallback(
        IN WMIDPREQUESTCODE RequestCode,
        IN PVOID pvContext,
        IN ULONG *pulReserved,
        IN PVOID pvBuffer
        );
    inline UCHAR GetTracingLevel() const { return m_uchLevel;}
 
    inline ULONG GetTracingFlags()  const{ return m_ulFlags;}
    
    inline bool IsTracingEnabled(ULONG ulFlags = EVENT_TRACE_FLAG_ALL,
                                 UCHAR uchLevel = TRACE_LEVEL_INFORMATION
                                 ) const
        {
            return ((m_hLogSession != INVALID_TRACEHANDLE_VALUE) && 
                    (uchLevel <= m_uchLevel) &&
                    (m_ulFlags & ulFlags));
        }

    inline void TraceEvent(EVENT_TRACE_HEADER *pHeader) 
        { ::TraceEvent(m_hLogSession, pHeader);}


    void  TraceEvent(
        IN GUID eventGuid,
        IN UCHAR uchEtwType,
        IN PVOID pvData = NULL,
        IN UINT cbData = 0
        );

    HRESULT Register();

    HRESULT UnRegister();

  private:
    CETWTraceProvider();

    TRACEHANDLE      m_hLogSession;    // Handle required by TraceEvent
    TRACEHANDLE      m_hRegistration;  // Handle required by UnregisterTraceGuids
    UCHAR            m_uchLevel;  //Tracing level
    ULONG            m_ulFlags;   //Tracing flags

    GUID             m_guidProvider;
    TRACE_GUID_REGISTRATION *m_pGuidTrace;
    UINT             m_cGuids;
};

