/*
 * Copyright (C) 2005 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef _WINE_SNMP_H
#define _WINE_SNMP_H

#include <windows.h>

#include <pshpack4.h>

typedef struct {
    BYTE *stream;
    UINT  length;
    BOOL  dynamic;
} AsnOctetString;

typedef struct {
    UINT  idLength;
    UINT *ids;
} AsnObjectIdentifier;

typedef LONG           AsnInteger32;
typedef ULONG          AsnUnsigned32;
typedef ULARGE_INTEGER AsnCounter64;
typedef AsnUnsigned32  AsnCounter32;
typedef AsnUnsigned32  AsnGauge32;
typedef AsnUnsigned32  AsnTimeticks;
typedef AsnOctetString AsnBits;
typedef AsnOctetString AsnSequence;
typedef AsnOctetString AsnImplicitSequence;
typedef AsnOctetString AsnIPAddress;
typedef AsnOctetString AsnNetworkAddress;
typedef AsnOctetString AsnDisplayString;
typedef AsnOctetString AsnOpaque;

typedef struct {
    BYTE asnType;
    union {
        AsnInteger32        number;
        AsnUnsigned32       unsigned32;
        AsnCounter64        counter64;
        AsnOctetString      string;
        AsnBits             bits;
        AsnObjectIdentifier object;
        AsnSequence         sequence;
        AsnIPAddress        address;
        AsnCounter32        counter;
        AsnGauge32          gauge;
        AsnTimeticks        ticks;
        AsnOpaque           arbitrary;
    } asnValue;
} AsnAny;

typedef AsnObjectIdentifier AsnObjectName;
typedef AsnAny              AsnObjectSyntax;

typedef struct {
    AsnObjectName   name;
    AsnObjectSyntax value;
} SnmpVarBind;

typedef struct {
    SnmpVarBind *list;
    UINT         len;
} SnmpVarBindList;

#include <poppack.h>

#define ASN_UNIVERSAL   0x00
#define ASN_APPLICATION 0x40
#define ASN_CONTEXT     0x80
#define ASN_PRIVATE     0xc0
#define ASN_PRIMITIVE   0x00
#define ASN_CONSTRUCTOR 0x20

#define SNMP_PDU_GET         (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x00)
#define SNMP_PDU_GETNEXT     (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x01)
#define SNMP_PDU_RESPONSE    (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x02)
#define SNMP_PDU_SET         (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x03)
#define SNMP_PDU_V1TRAP      (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x04)
#define SNMP_PDU_GETBULK     (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x05)
#define SNMP_PDU_INFORM      (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x06)
#define SNMP_PDU_TRAP        (ASN_CONTEXT     | ASN_CONSTRUCTOR | 0x07)

#define ASN_INTEGER          (ASN_UNIVERSAL   | ASN_PRIMITIVE   | 0x02)
#define ASN_BITS             (ASN_UNIVERSAL   | ASN_PRIMITIVE   | 0x03)
#define ASN_OCTETSTRING      (ASN_UNIVERSAL   | ASN_PRIMITIVE   | 0x04)
#define ASN_NULL             (ASN_UNIVERSAL   | ASN_PRIMITIVE   | 0x05)
#define ASN_OBJECTIDENTIFIER (ASN_UNIVERSAL   | ASN_PRIMITIVE   | 0x06)
#define ASN_INTEGER32        ASN_INTEGER

#define ASN_SEQUENCE         (ASN_UNIVERSAL   | ASN_CONSTRUCTOR | 0x10)
#define ASN_SEQUENCEOF       ASN_SEQUENCE

#define ASN_IPADDRESS        (ASN_APPLICATION | ASN_PRIMITIVE   | 0x00)
#define ASN_COUNTER32        (ASN_APPLICATION | ASN_PRIMITIVE   | 0x01)
#define ASN_GAUGE32          (ASN_APPLICATION | ASN_PRIMITIVE   | 0x02)
#define ASN_TIMETICKS        (ASN_APPLICATION | ASN_PRIMITIVE   | 0x03)
#define ASN_OPAQUE           (ASN_APPLICATION | ASN_PRIMITIVE   | 0x04)
#define ASN_COUNTER64        (ASN_APPLICATION | ASN_PRIMITIVE   | 0x06)
#define ASN_UNSIGNED32       (ASN_APPLICATION | ASN_PRIMITIVE   | 0x07)

#define SNMP_EXCEPTION_NOSUCHOBJECT   (ASN_CONTEXT | ASN_PRIMITIVE | 0x00)
#define SNMP_EXCEPTION_NOSUCHINSTANCE (ASN_CONTEXT | ASN_PRIMITIVE | 0x01)
#define SNMP_EXCEPTION_ENDOFMIBVIEW   (ASN_CONTEXT | ASN_PRIMITIVE | 0x02)

#define SNMP_EXTENSION_GET         SNMP_PDU_GET
#define SNMP_EXTENSION_GET_NEXT    SNMP_PDU_GETNEXT
#define SNMP_EXTENSION_GET_BULK    SNMP_PDU_GETBULK
#define SNMP_EXTENSION_SET_TEST    (ASN_PRIVATE | ASN_CONSTRUCTOR | 0x0)
#define SNMP_EXTENSION_SET_COMMIT  SNMP_PDU_SET
#define SNMP_EXTENSION_SET_UNDO    (ASN_PRIVATE | ASN_CONSTRUCTOR | 0x1)
#define SNMP_EXTENSION_SET_CLEANUP (ASN_PRIVATE | ASN_CONSTRUCTOR | 0x2)

#define SNMP_ERRORSTATUS_NOERROR             0
#define SNMP_ERRORSTATUS_TOOBIG              1
#define SNMP_ERRORSTATUS_NOSUCHNAME          2
#define SNMP_ERRORSTATUS_BADVALUE            3
#define SNMP_ERRORSTATUS_READONLY            4
#define SNMP_ERRORSTATUS_GENERR              5
#define SNMP_ERRORSTATUS_NOACCESS            6
#define SNMP_ERRORSTATUS_WRONGTYPE           7
#define SNMP_ERRORSTATUS_WRONGLENGTH         8
#define SNMP_ERRORSTATUS_WRONGENCODING       9
#define SNMP_ERRORSTATUS_WRONGVALUE          10
#define SNMP_ERRORSTATUS_NOCREATION          11
#define SNMP_ERRORSTATUS_INCONSISTENTVALUE   12
#define SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE 13
#define SNMP_ERRORSTATUS_COMMITFAILED        14
#define SNMP_ERRORSTATUS_UNDOFAILED          15
#define SNMP_ERRORSTATUS_AUTHORIZATIONERROR  16
#define SNMP_ERRORSTATUS_NOTWRITABLE         17
#define SNMP_ERRORSTATUS_INCONSISTENTNAME    18

#define SNMP_GENERICTRAP_COLDSTART           0
#define SNMP_GENERICTRAP_WARMSTART           1
#define SNMP_GENERICTRAP_LINKDOWN            2
#define SNMP_GENERICTRAP_LINKUP              3
#define SNMP_GENERICTRAP_AUTHFAILURE         4
#define SNMP_GENERICTRAP_EGPNEIGHLOSS        5
#define SNMP_GENERICTRAP_ENTERSPECIFIC       6

#define SNMP_ACCESS_NONE        0
#define SNMP_ACCESS_NOTIFY      1
#define SNMP_ACCESS_READ_ONLY   2
#define SNMP_ACCESS_READ_WRITE  3
#define SNMP_ACCESS_READ_CREATE 4

#define SNMP_LOG_SILENT  0
#define SNMP_LOG_FATAL   1
#define SNMP_LOG_ERROR   2
#define SNMP_LOG_WARNING 3
#define SNMP_LOG_TRACE   4
#define SNMP_LOG_VERBOSE 5

#define SNMP_OUTPUT_TO_CONSOLE  1
#define SNMP_OUTPUT_TO_LOGFILE  2
#define SNMP_OUTPUT_TO_EVENTLOG 4
#define SNMP_OUTPUT_TO_DEBUGGER 8

#define DEFINE_SIZEOF(x)     (sizeof(x)/sizeof((x)[0]))
#define DEFINE_OID(x)        { DEFINE_SIZEOF(x),(x) }
#define DEFINE_NULLOID()     { 0, NULL }
#define DEFINE_NULLOCTENTS() { NULL, 0, FALSE }

#define DEFAULT_SNMP_PORT_UDP     161
#define DEFAULT_SNMP_PORT_IPX     36879
#define DEFAULT_SNMPTRAP_PORT_UDP 162
#define DEFAULT_SNMPTRAP_PORT_IPX 36880

#define SNMP_MAX_OID_LEN 128

#define SNMP_MEM_ALLOC_ERROR          0
#define SNMP_BERAPI_INVALID_LENGTH    10
#define SNMP_BERAPI_INVALID_TAG       11
#define SNMP_BERAPI_OVERFLOW          12
#define SNMP_BERAPI_SHORT_BUFFER      13
#define SNMP_BERAPI_INVALID_OBJELEM   14
#define SNMP_PDUAPI_UNRECOGNIZED_PDU  20
#define SNMP_PDUAPI_INVALID_ES        21
#define SNMP_PDUAPI_INVALID_GT        22
#define SNMP_AUTHAPI_INVALID_VERSION  30
#define SNMP_AUTHAPI_INVALID_MSG_TYPE 31
#define SNMP_AUTHAPI_TRIV_AUTH_FAILED 32

#define SNMPAPI_NOERROR TRUE
#define SNMPAPI_ERROR   FALSE

#ifdef __cplusplus
extern "C" {
#endif

BOOL WINAPI SnmpExtensionInit(DWORD dwUptimeReference,
 HANDLE *phSubagentTrapEvent, AsnObjectIdentifier *pFirstSupportedRegion);
BOOL WINAPI SnmpExtensionInitEx(AsnObjectIdentifier *pNextSupportedRegion);

BOOL WINAPI SnmpExtensionMonitor(LPVOID pAgentMgmtData);

BOOL WINAPI SnmpExtensionQuery(BYTE bPduType, SnmpVarBindList *pVarBindList,
 AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex);
BOOL WINAPI SnmpExtensionQueryEx(UINT nRequestType, UINT nTransactionId,
 SnmpVarBindList *pVarBindList, AsnOctetString *pContextInfo,
 AsnInteger32 *pErrorStatus, AsnInteger32 *pErrorIndex);

BOOL WINAPI SnmpExtensionTrap(AsnObjectIdentifier *pEnterpriseOid,
 AsnInteger32 *pGenericTrapId, AsnInteger32 *pSpecificTrapId,
 AsnTimeticks *pTimeStamp, SnmpVarBindList *pVarBindList);

VOID WINAPI SnmpExtensionClose(VOID);

typedef BOOL (WINAPI *PFNSNMPEXTENSIONINIT)(DWORD dwUptimeReference,
 HANDLE *phSubagentTrapEvent, AsnObjectIdentifier *pFirstSupportedRegion);
typedef BOOL (WINAPI *PFNSNMPEXTENSIONINITEX)(
 AsnObjectIdentifier *pNextSupportedRegion);

typedef BOOL (WINAPI *PFNSNMPEXTENSIONMONITOR)(LPVOID pAgentMgmtData);

typedef BOOL (WINAPI *PFNSNMPEXTENSIONQUERY)(BYTE bPduType,
 SnmpVarBindList *pVarBindList, AsnInteger32 *pErrorStatus,
 AsnInteger32 *pErrorIndex);
typedef BOOL (WINAPI *PFNSNMPEXTENSIONQUERYEX)(UINT nRequestType,
 UINT nTransactionId, SnmpVarBindList *pVarBindList,
 AsnOctetString *pContextInfo, AsnInteger32 *pErrorStatus,
 AsnInteger32 *pErrorIndex);

typedef BOOL (WINAPI *PFNSNMPEXTENSIONTRAP)(AsnObjectIdentifier *pEnterpriseOid,
 AsnInteger32 *pGenericTrapId, AsnInteger32 *pSpecificTrapId,
 AsnTimeticks *pTimeStamp, SnmpVarBindList *pVarBindList);

typedef VOID (WINAPI *PFNSNMPEXTENSIONCLOSE)(VOID);

INT WINAPI SnmpUtilOidCpy(AsnObjectIdentifier *pOidDst,
 AsnObjectIdentifier *pOidSrc);
INT WINAPI SnmpUtilOidAppend(AsnObjectIdentifier *pOidDst,
 AsnObjectIdentifier *pOidSrc);
INT WINAPI SnmpUtilOidCmp(AsnObjectIdentifier *pOid1,
 AsnObjectIdentifier *pOid2);
INT WINAPI SnmpUtilOidNCmp(AsnObjectIdentifier *pOid1,
 AsnObjectIdentifier *pOid2, UINT nSubIds);
VOID WINAPI SnmpUtilOidFree(AsnObjectIdentifier *pOid);

INT WINAPI SnmpUtilOctetsCmp(AsnOctetString *pOctets1,
 AsnOctetString *pOctets2);
INT WINAPI SnmpUtilOctetsNCmp(AsnOctetString *pOctets1,
 AsnOctetString *pOctets2, UINT nChars);
INT WINAPI SnmpUtilOctetsCpy(AsnOctetString *pOctetsDst,
 AsnOctetString *pOctetsSrc);
VOID WINAPI SnmpUtilOctetsFree(AsnOctetString *pOctets);

INT WINAPI SnmpUtilAsnAnyCpy(AsnAny *pAnyDst, AsnAny *pAnySrc);
VOID WINAPI SnmpUtilAsnAnyFree(AsnAny *pAny);

INT WINAPI SnmpUtilVarBindCpy(SnmpVarBind *pVbDst, SnmpVarBind *pVbSrc);
VOID WINAPI SnmpUtilVarBindFree(SnmpVarBind *pVb);

INT WINAPI SnmpUtilVarBindListCpy(SnmpVarBindList *pVblDst,
 SnmpVarBindList *pVblSrc);
VOID WINAPI SnmpUtilVarBindListFree(SnmpVarBindList *pVbl);

LPVOID WINAPI SnmpUtilMemAlloc(UINT nBytes) __WINE_ALLOC_SIZE(1);
LPVOID WINAPI SnmpUtilMemReAlloc(LPVOID pMem, UINT nBytes) __WINE_ALLOC_SIZE(2);
VOID WINAPI SnmpUtilMemFree(LPVOID pMem);

LPSTR WINAPI SnmpUtilOidToA(AsnObjectIdentifier *Oid);
LPSTR WINAPI SnmpUtilIdsToA(UINT *Ids, UINT IdLength);

VOID WINAPI SnmpUtilPrintOid(AsnObjectIdentifier *Oid);
VOID WINAPI SnmpUtilPrintAsnAny(AsnAny *pAny);

DWORD WINAPI SnmpSvcGetUptime(VOID);
VOID WINAPI SnmpSvcSetLogLevel(INT nLogLevel);
VOID WINAPI SnmpSvcSetLogType(INT nLogType);

VOID WINAPIV SnmpUtilDbgPrint(INT nLogLevel, LPSTR szFormat, ...);

#ifdef __cplusplus
}
#endif

#endif /* _WINE_SNMP_H */
