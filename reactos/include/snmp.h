/*
 *  ReactOS Simple Network Management Protocol - SNMP
 *
 *  snmp.h
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SNMP_H__
#define __SNMP_H__

#ifdef __cplusplus
extern "C" {
#endif


///////////////////////////////////////////////////////////////////////////////
// SNMP Error Codes
//
	
#define SNMP_MEM_ALLOC_ERROR            1
#define SNMP_BERAPI_INVALID_LENGTH      10
#define SNMP_BERAPI_INVALID_TAG         11
#define SNMP_BERAPI_OVERFLOW            12
#define SNMP_BERAPI_SHORT_BUFFER        13
#define SNMP_BERAPI_INVALID_OBJELEM     14
#define SNMP_PDUAPI_UNRECOGNIZED_PDU    20
#define SNMP_PDUAPI_INVALID_ES          21
#define SNMP_PDUAPI_INVALID_GT          22
#define SNMP_AUTHAPI_INVALID_VERSION    30
#define SNMP_AUTHAPI_INVALID_MSG_TYPE   31
#define SNMP_AUTHAPI_TRIV_AUTH_FAILED   32

////////////////////////////////////////////////////////////////////////////////
// SNMP Type Definitions
//

#include <pshpack4.h>

typedef struct {
    BYTE* stream;
    UINT length;
    BOOL dynamic;
} AsnOctetString;

typedef struct {
    UINT idLength;
    UINT* ids;
} AsnObjectIdentifier;

typedef LONG AsnInteger32;
typedef ULONG AsnUnsigned32;
typedef ULARGE_INTEGER AsnCounter64;
typedef AsnUnsigned32 AsnCounter32;
typedef AsnUnsigned32 AsnGauge32;
typedef AsnUnsigned32 AsnTimeticks;
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
        AsnInteger32 number;        // ASN_INTEGER & ASN_INTEGER32
        AsnUnsigned32 unsigned32;   // ASN_UNSIGNED32
        AsnCounter64 counter64;     // ASN_COUNTER64
        AsnOctetString string;      // ASN_OCTETSTRING
        AsnBits bits;               // ASN_BITS
        AsnObjectIdentifier object; // ASN_OBJECTIDENTIFIER
        AsnSequence sequence;       // ASN_SEQUENCE
        AsnIPAddress address;       // ASN_IPADDRESS
        AsnCounter32 counter;       // ASN_COUNTER32
        AsnGauge32 gauge;           // ASN_GAUGE32
        AsnTimeticks ticks;         // ASN_TIMETICKS
        AsnOpaque arbitrary;        // ASN_OPAQUE
    } asnValue;
} AsnAny;

typedef AsnObjectIdentifier AsnObjectName;
typedef AsnAny AsnObjectSyntax;

typedef struct {
    AsnObjectName name;
    AsnObjectSyntax value;
} SnmpVarBind;

typedef struct {
    SnmpVarBind* list;
    UINT len;
} SnmpVarBindList;

#include <poppack.h>

////////////////////////////////////////////////////////////////////////////////
// SNMP Function Prototypes
//

#define SNMPAPI INT
#ifndef SNMP_FUNC_TYPE
#define SNMP_FUNC_TYPE
#endif

//LPVOID SNMP_FUNC_TYPE SnmpUtilMemAlloc(UINT nBytes);
//VOID SNMP_FUNC_TYPE SnmpUtilMemFree(LPVOID pMem);
//SNMPAPI SNMP_FUNC_TYPE SnmpUtilOidCpy(AsnObjectIdentifier* pOidDst, AsnObjectIdentifier* pOidSrc);
//VOID SNMP_FUNC_TYPE SnmpUtilOidFree(AsnObjectIdentifier* pOid);
//VOID SNMP_FUNC_TYPE SnmpUtilVarBindFree(SnmpVarBind* pVb);

LPVOID SNMP_FUNC_TYPE SnmpUtilMemAlloc(
  UINT nBytes  // bytes to allocate for object
);

VOID SNMP_FUNC_TYPE SnmpUtilMemFree(
  LPVOID pMem  // pointer to memory object to release
);

LPVOID SNMP_FUNC_TYPE SnmpUtilMemReAlloc(
  LPVOID pMem,  // pointer to memory object
  UINT nBytes   // bytes to allocate
);

VOID SNMP_FUNC_TYPE SnmpSvcInitUptime();

DWORD SNMP_FUNC_TYPE SnmpSvcGetUptime();

VOID SNMP_FUNC_TYPE SnmpSvcSetLogLevel(
  INT nLogLevel  // level of severity of the event
);

VOID SNMP_FUNC_TYPE SnmpSvcSetLogType(
  INT nLogType  // destination for debug output
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilAsnAnyCpy(
  AsnAny *pAnyDst,  // destination structure
  AsnAny *pAnySrc   // source structure
);

VOID SNMP_FUNC_TYPE SnmpUtilAsnAnyFree(
  AsnAny *pAny  // pointer to structure to free
);

//VOID SNMP_FUNC_TYPE SnmpUtilDbgPrint(
//  INT nLogLevel,  // level of severity of event 
//  LPSTR szFormat  // pointer to a format string 
//);

LPSTR SNMP_FUNC_TYPE SnmpUtilIdsToA(
  UINT *Ids,     // object identifier to convert
  UINT IdLength  // number of elements
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOctetsCmp(
  AsnOctetString *pOctets1,  // first octet string
  AsnOctetString *pOctets2   // second octet string
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOctetsCpy(
  AsnOctetString *pOctetsDst,  // destination octet string
  AsnOctetString *pOctetsSrc   // source octet string
);

VOID SNMP_FUNC_TYPE SnmpUtilOctetsFree(
  AsnOctetString *pOctets  // octet string to free
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOctetsNCmp(
  AsnOctetString *pOctets1,  // first octet string
  AsnOctetString *pOctets2,  // second octet string
  UINT nChars                // maximum length to compare
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOidAppend(
  AsnObjectIdentifier *pOidDst,  // destination object identifier
  AsnObjectIdentifier *pOidSrc   // source object identifier
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOidCmp(
  AsnObjectIdentifier *pOid1,  // first object identifier
  AsnObjectIdentifier *pOid2   // second object identifier
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOidCpy(
  AsnObjectIdentifier *pOidDst,  // destination object identifier
  AsnObjectIdentifier *pOidSrc   // source object identifier
);

VOID SNMP_FUNC_TYPE SnmpUtilOidFree(
  AsnObjectIdentifier *pOid  // object identifier to free
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilOidNCmp(
  AsnObjectIdentifier *pOid1,  // first object identifier
  AsnObjectIdentifier *pOid2,  // second object identifier
  UINT nSubIds                 // maximum length to compare
);

LPSTR SNMP_FUNC_TYPE SnmpUtilOidToA(
  AsnObjectIdentifier *Oid  // object identifier to convert
);

VOID SNMP_FUNC_TYPE SnmpUtilPrintAsnAny(
  AsnAny *pAny  // pointer to value to print
);

VOID SNMP_FUNC_TYPE SnmpUtilPrintOid(
  AsnObjectIdentifier *Oid  // object identifier to print
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilVarBindCpy(
  SnmpVarBind *pVbDst,  // destination variable bindings
  SnmpVarBind *pVbSrc   // source variable bindings
);

VOID SNMP_FUNC_TYPE SnmpUtilVarBindFree(
  SnmpVarBind *pVb  // variable binding to free
);

SNMPAPI SNMP_FUNC_TYPE SnmpUtilVarBindListCpy(
  SnmpVarBindList *pVblDst,  // destination variable bindings list
  SnmpVarBindList *pVblSrc   // source variable bindings list
);

VOID SNMP_FUNC_TYPE SnmpUtilVarBindListFree(
  SnmpVarBindList *pVbl  // variable bindings list to free
);



////////////////////////////////////////////////////////////////////////////////
// SNMP Debugging Definitions
//

#define SNMP_LOG_SILENT                 0x0
#define SNMP_LOG_FATAL                  0x1
#define SNMP_LOG_ERROR                  0x2
#define SNMP_LOG_WARNING                0x3
#define SNMP_LOG_TRACE                  0x4
#define SNMP_LOG_VERBOSE                0x5

#define SNMP_OUTPUT_TO_CONSOLE          0x1
#define SNMP_OUTPUT_TO_LOGFILE          0x2
//#define SNMP_OUTPUT_TO_EVENTLOG         0x4  // no longer supported
#define SNMP_OUTPUT_TO_DEBUGGER         0x8

////////////////////////////////////////////////////////////////////////////////
// SNMP Debugging Prototypes
//

VOID
SNMP_FUNC_TYPE
SnmpUtilDbgPrint(
    IN INT nLogLevel,   // see log levels above...
    IN LPSTR szFormat,
    IN ...
    );

#if DBG
#define SNMPDBG(_x_)                    SnmpUtilDbgPrint _x_
#else
#define SNMPDBG(_x_)
#endif


////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
};
#endif

#endif // __SNMP_H__

