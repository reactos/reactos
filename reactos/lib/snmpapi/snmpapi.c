/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sockets 2 Simple Network Management Protocol API DLL
 * FILE:        snmpapi.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Robert Dickenson (robd@reactos.org)
 * REVISIONS:
 *   RDD August 18, 2002 Created
 */
#include "snmpapi.h"

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */

/* To make the linker happy */
//VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


BOOL
EXPORT
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    WSH_DbgPrint(MIN_TRACE, ("DllMain of snmpapi.dll\n"));

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

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

VOID
SNMP_FUNC_TYPE
SnmpSvcAddrIsIpx(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpSvcAddrToSocket(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpSvcGetEnterpriseOID(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

LPVOID
SNMP_FUNC_TYPE
SnmpUtilMemAlloc(UINT nBytes)
{
    UNIMPLEMENTED
    return NULL;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilMemFree(LPVOID pMem)
{
    UNIMPLEMENTED
}

LPVOID
SNMP_FUNC_TYPE
SnmpUtilMemReAlloc(LPVOID pMem, UINT nBytes)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpSvcInitUptime()
{
    UNIMPLEMENTED
}

DWORD 
SNMP_FUNC_TYPE
SnmpSvcGetUptime()
{
    UNIMPLEMENTED
    return 0L;
}

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

VOID
SNMP_FUNC_TYPE
SnmpTfxClose(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpTfxOpen(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpTfxQuery(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilAnsiToUnicode(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilAsnAnyCpy(AsnAny *pAnyDst, AsnAny *pAnySrc)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilAsnAnyFree(AsnAny *pAny)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilDbgPrint(INT nLogLevel, LPSTR szFormat, ...)
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

LPSTR
SNMP_FUNC_TYPE
SnmpUtilIdsToA(UINT *Ids, UINT IdLength)
{
    UNIMPLEMENTED
    return 0;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOctetsCmp(AsnOctetString *pOctets1, AsnOctetString *pOctets2)
{
    UNIMPLEMENTED
    return 0;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOctetsCpy(AsnOctetString *pOctetsDst, AsnOctetString *pOctetsSrc)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilOctetsFree(AsnOctetString *pOctets)
{
    UNIMPLEMENTED
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOctetsNCmp(AsnOctetString *pOctets1, AsnOctetString *pOctets2, UINT nChars)
{
    UNIMPLEMENTED
    return 0;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidAppend(AsnObjectIdentifier *pOidDst, AsnObjectIdentifier *pOidSrc)
{
    UNIMPLEMENTED
//    return SNMP_BERAPI_OVERFLOW;
//    return SNMP_MEM_ALLOC_ERROR;
    return 0;
}


SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidCmp(AsnObjectIdentifier *pOid1, AsnObjectIdentifier *pOid2)
{
    UNIMPLEMENTED
    return 0;
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidCpy(AsnObjectIdentifier *pOidDst, AsnObjectIdentifier *pOidSrc)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilOidFree(AsnObjectIdentifier *pOid)
{
    UNIMPLEMENTED
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilOidNCmp(AsnObjectIdentifier *pOid1, AsnObjectIdentifier *pOid2, UINT nSubIds)
{
    UNIMPLEMENTED
    return 0;
}

LPSTR
SNMP_FUNC_TYPE
SnmpUtilOidToA(AsnObjectIdentifier *Oid)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilPrintAsnAny(AsnAny *pAny)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilPrintOid(AsnObjectIdentifier *Oid)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilUTF8ToUnicode(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilUnicodeToAnsi(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

VOID
SNMP_FUNC_TYPE
SnmpUtilUnicodeToUTF8(void* unknown, void* unknown2)
{
    UNIMPLEMENTED
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilVarBindCpy(SnmpVarBind *pVbDst, SnmpVarBind *pVbSrc)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilVarBindFree(SnmpVarBind *pVb)
{
    UNIMPLEMENTED
}

SNMPAPI
SNMP_FUNC_TYPE
SnmpUtilVarBindListCpy(SnmpVarBindList *pVblDst, SnmpVarBindList *pVblSrc)
{
    UNIMPLEMENTED
    return 0;
}

VOID
SNMP_FUNC_TYPE
SnmpUtilVarBindListFree(SnmpVarBindList *pVbl)
{
    UNIMPLEMENTED
}

/* EOF */
