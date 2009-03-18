/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sockets 2 Simple Network Management Protocol API DLL
 * FILE:        snmpapi.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Robert Dickenson (robd@reactos.org)
 * REVISIONS:
 *   RDD August 18, 2002 Created
 */
//#include "snmpapi.h"

#include <winsock2.h>
#include <windows.h>

#ifdef __GNUC__
#define SNMP_FUNC_TYPE WINAPI
#endif
#include <snmp.h>
#include "debug.h"


#ifdef __GNUC__
#define EXPORT WINAPI
#else
#define EXPORT CALLBACK
#endif


#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */


DWORD dwUptimeStartTicks;


/* To make the linker happy */
//VOID WINAPI KeBugCheck (ULONG	BugCheckCode) {}

////////////////////////////////////////////////////////////////////////////////
/*
? SnmpSvcAddrIsIpx
? SnmpSvcAddrToSocket
? SnmpSvcGetEnterpriseOID
? SnmpTfxClose
? SnmpTfxOpen
? SnmpTfxQuery
? SnmpUtilAnsiToUnicode
? SnmpUtilUTF8ToUnicode
? SnmpUtilUnicodeToAnsi
? SnmpUtilUnicodeToUTF8
 */
////////////////////////////////////////////////////////////////////////////////

/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcAddrIsIpx(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcAddrToSocket(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcGetEnterpriseOID(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcInitUptime()
{
    dwUptimeStartTicks = GetTickCount();
}


/*
 * @implemented
 */
DWORD
SNMP_FUNC_TYPE
SnmpSvcGetUptime()
{
	DWORD dwUptime;
	DWORD dwTickCount = GetTickCount();
	dwUptime = dwTickCount - dwUptimeStartTicks;
    return dwUptime;
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcSetLogLevel(INT nLogLevel)
{
    switch (nLogLevel) {
    case SNMP_LOG_SILENT:
        break;
    case SNMP_LOG_FATAL:
        break;
    case SNMP_LOG_ERROR:
        break;
    case SNMP_LOG_WARNING:
        break;
    case SNMP_LOG_TRACE:
        break;
    case SNMP_LOG_VERBOSE:
        break;
    }
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpSvcSetLogType(INT nLogType)
{
    switch (nLogType) {
    case SNMP_OUTPUT_TO_CONSOLE:
        break;
    case SNMP_OUTPUT_TO_LOGFILE:
        break;
    case SNMP_OUTPUT_TO_DEBUGGER:
        break;
    }
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpTfxClose(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpTfxOpen(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpTfxQuery(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpUtilUTF8ToUnicode(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
SNMP_FUNC_TYPE
SnmpUtilUnicodeToUTF8(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

/* EOF */

