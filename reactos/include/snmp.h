/*
  snmp.h - Header file for the Windows SNMP API

  Written by Filip Navara <xnavara@volny.cz>

  References (2003-08-25):
    http://msdn.microsoft.com/library/en-us/snmp/snmp/snmp_reference.asp

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef _SNMP_H
#define _SNMP_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifndef _WINDOWS_H
#include <windows.h>
#endif

#include <pshpack4.h>

#ifndef WINSNMPAPI
#define WINSNMPAPI WINAPI
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SNMP_PORT_UDP	161
#define DEFAULT_SNMP_PORT_IPX	36879
#define DEFAULT_SNMPTRAP_PORT_UDP	162
#define DEFAULT_SNMPTRAP_PORT_IPX	36880
#ifndef _SNMP_ASN_DEFINED
#define _SNMP_ASN_DEFINED
#define ASN_UNIVERSAL	0x00
#define ASN_PRIMITIVE	0x00
#define ASN_CONSTRUCTOR	0x20
#define ASN_APPLICATION	0x40
#define ASN_CONTEXT	0x80
#define ASN_PRIVATE	0xC0
#define SNMP_PDU_GET	(ASN_CONTEXT | ASN_CONSTRUCTOR | 0)
#define SNMP_PDU_GETNEXT	(ASN_CONTEXT | ASN_CONSTRUCTOR | 1)
#define SNMP_PDU_RESPONSE	(ASN_CONTEXT | ASN_CONSTRUCTOR | 2)
#define SNMP_PDU_SET	(ASN_CONTEXT | ASN_CONSTRUCTOR | 3)
#define SNMP_PDU_GETBULK	(ASN_CONTEXT | ASN_CONSTRUCTOR | 4)
#define SNMP_PDU_V1TRAP	(ASN_CONTEXT | ASN_CONSTRUCTOR | 4)
#define SNMP_PDU_INFORM	(ASN_CONTEXT | ASN_CONSTRUCTOR | 6)
#define SNMP_PDU_TRAP	(ASN_CONTEXT | ASN_CONSTRUCTOR | 7) 
#define SNMP_PDU_REPORT	(ASN_CONTEXT | ASN_CONSTRUCTOR | 8)
#endif /* _SNMP_ASN_DEFINED */
#define ASN_INTEGER	(ASN_UNIVERSAL | ASN_PRIMITIVE | 2)
#define ASN_BITS	(ASN_UNIVERSAL | ASN_PRIMITIVE | 3)
#define ASN_OCTETSTRING	(ASN_UNIVERSAL | ASN_PRIMITIVE | 4)
#define ASN_NULL	(ASN_UNIVERSAL | ASN_PRIMITIVE | 5)
#define ASN_OBJECTIDENTIFIER	(ASN_UNIVERSAL | ASN_PRIMITIVE | 6)
#define ASN_INTEGER32	ASN_INTEGER
#define ASN_SEQUENCE	(ASN_UNIVERSAL | ASN_CONSTRUCTOR | 0x10)
#define ASN_SEQUENCEOF	ASN_SEQUENCE
#define ASN_IPADDRESS	(ASN_APPLICATION | ASN_PRIMITIVE | 0x00)
#define ASN_COUNTER32	(ASN_APPLICATION | ASN_PRIMITIVE | 0x01)
#define ASN_GAUGE32	(ASN_APPLICATION | ASN_PRIMITIVE | 0x02)
#define ASN_TIMETICKS	(ASN_APPLICATION | ASN_PRIMITIVE | 0x03)
#define ASN_OPAQUE	(ASN_APPLICATION | ASN_PRIMITIVE | 0x04)
#define ASN_COUNTER64	(ASN_APPLICATION | ASN_PRIMITIVE | 0x06)
#define ASN_UNSIGNED32	(ASN_APPLICATION | ASN_PRIMITIVE | 0x07)
#define SNMP_EXCEPTION_NOSUCHOBJECT	(ASN_CONTEXT | ASN_PRIMITIVE | 0x00)
#define SNMP_EXCEPTION_NOSUCHINSTANCE	(ASN_CONTEXT | ASN_PRIMITIVE | 0x01)
#define SNMP_EXCEPTION_ENDOFMIBVIEW	(ASN_CONTEXT | ASN_PRIMITIVE | 0x02)
#define SNMP_EXTENSION_GET	SNMP_PDU_GET
#define SNMP_EXTENSION_GET_NEXT	SNMP_PDU_GETNEXT
#define SNMP_EXTENSION_GET_BULK	SNMP_PDU_GETBULK
#define SNMP_EXTENSION_SET_TEST	(ASN_PRIVATE | ASN_CONSTRUCTOR | 0x0)
#define SNMP_EXTENSION_SET_COMMIT	SNMP_PDU_SET
#define SNMP_EXTENSION_SET_UNDO	(ASN_PRIVATE | ASN_CONSTRUCTOR | 0x1)
#define SNMP_EXTENSION_SET_CLEANUP	(ASN_PRIVATE | ASN_CONSTRUCTOR | 0x2)
#define SNMP_ERRORSTATUS_NOERROR	0
#define SNMP_ERRORSTATUS_TOOBIG	1
#define SNMP_ERRORSTATUS_NOSUCHNAME	2
#define SNMP_ERRORSTATUS_BADVALUE	3
#define SNMP_ERRORSTATUS_READONLY	4
#define SNMP_ERRORSTATUS_GENERR	5
#define SNMP_ERRORSTATUS_NOACCESS	6
#define SNMP_ERRORSTATUS_WRONGTYPE	7
#define SNMP_ERRORSTATUS_WRONGLENGTH	8
#define SNMP_ERRORSTATUS_WRONGENCODING	9
#define SNMP_ERRORSTATUS_WRONGVALUE	10
#define SNMP_ERRORSTATUS_NOCREATION	11
#define SNMP_ERRORSTATUS_INCONSISTENTVALUE	12
#define SNMP_ERRORSTATUS_RESOURCEUNAVAILABLE	13
#define SNMP_ERRORSTATUS_COMMITFAILED	14
#define SNMP_ERRORSTATUS_UNDOFAILED	15
#define SNMP_ERRORSTATUS_AUTHORIZATIONERROR	16
#define SNMP_ERRORSTATUS_NOTWRITABLE	17
#define SNMP_ERRORSTATUS_INCONSISTENTNAME	18
#define SNMP_GENERICTRAP_COLDSTART	0
#define SNMP_GENERICTRAP_WARMSTART	1
#define SNMP_GENERICTRAP_LINKDOWN	2
#define SNMP_GENERICTRAP_LINKUP	3
#define SNMP_GENERICTRAP_AUTHFAILURE	4
#define SNMP_GENERICTRAP_EGPNEIGHLOSS	5
#define SNMP_GENERICTRAP_ENTERSPECIFIC	6
#define SNMP_ACCESS_NONE	0
#define SNMP_ACCESS_NOTIFY	1
#define SNMP_ACCESS_READ_ONLY	2
#define SNMP_ACCESS_READ_WRITE	3
#define SNMP_ACCESS_READ_CREATE	4
#define SNMPAPI_ERROR	FALSE
#define SNMPAPI_NOERROR	TRUE
#define SNMP_LOG_SILENT	0
#define SNMP_LOG_FATAL	1
#define SNMP_LOG_ERROR	2
#define SNMP_LOG_WARNING	3
#define SNMP_LOG_TRACE	4
#define SNMP_LOG_VERBOSE	5
#define SNMP_OUTPUT_TO_CONSOLE	1
#define SNMP_OUTPUT_TO_LOGFILE	2
#define SNMP_OUTPUT_TO_EVENTLOG	4 
#define SNMP_OUTPUT_TO_DEBUGGER	8
#define SNMP_MAX_OID_LEN	128
#define SNMP_MEM_ALLOC_ERROR	1
#define SNMP_BERAPI_INVALID_LENGTH	10
#define SNMP_BERAPI_INVALID_TAG	11
#define SNMP_BERAPI_OVERFLOW	12
#define SNMP_BERAPI_SHORT_BUFFER	13
#define SNMP_BERAPI_INVALID_OBJELEM	14
#define SNMP_PDUAPI_UNRECOGNIZED_PDU	20
#define SNMP_PDUAPI_INVALID_ES	21
#define SNMP_PDUAPI_INVALID_GT	22
#define SNMP_AUTHAPI_INVALID_VERSION	30
#define SNMP_AUTHAPI_INVALID_MSG_TYPE	31
#define SNMP_AUTHAPI_TRIV_AUTH_FAILED	32

#ifndef RC_INVOKED

typedef INT SNMPAPI;
typedef LONG AsnInteger32;
typedef ULONG AsnUnsigned32;
typedef ULARGE_INTEGER AsnCounter64;
typedef AsnUnsigned32 AsnCounter32;
typedef AsnUnsigned32 AsnGauge32;
typedef AsnUnsigned32 AsnTimeticks;
typedef struct {
	BYTE *stream;
	UINT length;
	BOOL dynamic;
} AsnOctetString, AsnBits, AsnSequence, AsnImplicitSequence, AsnIPAddress, AsnNetworkAddress, AsnDisplayString, AsnOpaque;
typedef struct {
	UINT idLength;
	UINT *ids;
} AsnObjectIdentifier, AsnObjectName;
typedef struct {
	BYTE asnType;
	union {
		AsnInteger32 number; 
		AsnUnsigned32 unsigned32; 
		AsnCounter64 counter64; 
		AsnOctetString string; 
		AsnBits bits; 
		AsnObjectIdentifier object; 
		AsnSequence sequence; 
		AsnIPAddress address; 
		AsnCounter32 counter; 
		AsnGauge32 gauge; 
		AsnTimeticks ticks; 
		AsnOpaque arbitrary; 
	} asnValue;
} AsnAny, AsnObjectSyntax;
typedef struct {
	AsnObjectName name;
	AsnObjectSyntax value;
} SnmpVarBind;
typedef struct {
	SnmpVarBind *list;
	UINT len;
} SnmpVarBindList;

VOID WINSNMPAPI SnmpExtensionClose(void);
BOOL WINSNMPAPI SnmpExtensionInit(DWORD,HANDLE*,AsnObjectIdentifier*);
BOOL WINSNMPAPI SnmpExtensionInitEx(AsnObjectIdentifier*);
BOOL WINSNMPAPI SnmpExtensionMonitor(LPVOID);
BOOL WINSNMPAPI SnmpExtensionQuery(BYTE,SnmpVarBindList*,AsnInteger32*,AsnInteger32*);
BOOL WINSNMPAPI SnmpExtensionQueryEx(DWORD,DWORD,SnmpVarBindList*,AsnOctetString*,AsnInteger32*,AsnInteger32*);
BOOL WINSNMPAPI SnmpExtensionTrap(AsnObjectIdentifier*,AsnInteger32*,AsnInteger32*,AsnTimeticks*,SnmpVarBindList*);
DWORD WINSNMPAPI SnmpSvcGetUptime(void);
VOID WINSNMPAPI SnmpSvcSetLogLevel(INT);
VOID WINSNMPAPI SnmpSvcSetLogType(INT);
SNMPAPI WINSNMPAPI SnmpUtilAsnAnyCpy(AsnAny*,AsnAny*);
VOID WINSNMPAPI SnmpUtilAsnAnyFree(AsnAny*);
VOID WINSNMPAPI SnmpUtilDbgPrint(INT,LPSTR,...);
LPSTR WINSNMPAPI SnmpUtilIdsToA(UINT*,UINT);
LPVOID WINSNMPAPI SnmpUtilMemAlloc(UINT);
VOID WINSNMPAPI SnmpUtilMemFree(LPVOID);
LPVOID WINSNMPAPI SnmpUtilMemReAlloc(LPVOID,UINT);
SNMPAPI WINSNMPAPI SnmpUtilOctetsCmp(AsnOctetString*,AsnOctetString*);
SNMPAPI WINSNMPAPI SnmpUtilOctetsCpy(AsnOctetString*,AsnOctetString*);
VOID WINSNMPAPI SnmpUtilOctetsFree(AsnOctetString*);
SNMPAPI WINSNMPAPI SnmpUtilOctetsNCmp(AsnOctetString*,AsnOctetString*,UINT);
SNMPAPI WINSNMPAPI SnmpUtilOidAppend(AsnObjectIdentifier*,AsnObjectIdentifier*);
SNMPAPI WINSNMPAPI SnmpUtilOidCmp(AsnObjectIdentifier*,AsnObjectIdentifier*);
SNMPAPI WINSNMPAPI SnmpUtilOidCpy(AsnObjectIdentifier*,AsnObjectIdentifier*);
VOID WINSNMPAPI SnmpUtilOidFree(AsnObjectIdentifier*);
SNMPAPI WINSNMPAPI SnmpUtilOidNCmp(AsnObjectIdentifier*,AsnObjectIdentifier*,UINT);
LPSTR WINSNMPAPI SnmpUtilOidToA(AsnObjectIdentifier*);
VOID WINSNMPAPI SnmpUtilPrintAsnAny(AsnAny*);
VOID WINSNMPAPI SnmpUtilPrintOid(AsnObjectIdentifier*);
SNMPAPI WINSNMPAPI SnmpUtilVarBindCpy(  SnmpVarBind*,SnmpVarBind*);
SNMPAPI WINSNMPAPI SnmpUtilVarBindListCpy(SnmpVarBindList*,SnmpVarBindList*);
VOID WINSNMPAPI SnmpUtilVarBindFree(SnmpVarBind*);
VOID WINSNMPAPI SnmpUtilVarBindListFree(SnmpVarBindList*);

#ifndef SNMPSTRICT
#define SNMP_malloc SnmpUtilMemAlloc
#define SNMP_free SnmpUtilMemFree
#define SNMP_realloc SnmpUtilMemReAlloc
#define SNMP_DBG_malloc SnmpUtilMemAlloc
#define SNMP_DBG_free SnmpUtilMemFree
#define SNMP_DBG_realloc SnmpUtilMemReAlloc
#define SNMP_oidappend SnmpUtilOidAppend
#define SNMP_oidcmp SnmpUtilOidCmp
#define SNMP_oidcpy SnmpUtilOidCpy
#define SNMP_oidfree SnmpUtilOidFree
#define SNMP_oidncmp SnmpUtilOidNCmp
#define SNMP_printany SnmpUtilPrintAsnAny
#define SNMP_CopyVarBind SnmpUtilVarBindCpy
#define SNMP_CopyVarBindList SnmpUtilVarBindListCpy
#define SNMP_FreeVarBind SnmpUtilVarBindFree
#define SNMP_FreeVarBindList SnmpUtilVarBindListFree
#define ASN_RFC1155_IPADDRESS ASN_IPADDRESS
#define ASN_RFC1155_COUNTER ASN_COUNTER32
#define ASN_RFC1155_GAUGE ASN_GAUGE32
#define ASN_RFC1155_TIMETICKS ASN_TIMETICKS
#define ASN_RFC1155_OPAQUE ASN_OPAQUE
#define ASN_RFC1213_DISPSTRING ASN_OCTETSTRING
#define ASN_RFC1157_GETREQUEST SNMP_PDU_GET
#define ASN_RFC1157_GETNEXTREQUEST SNMP_PDU_GETNEXT
#define ASN_RFC1157_GETRESPONSE SNMP_PDU_RESPONSE
#define ASN_RFC1157_SETREQUEST SNMP_PDU_SET
#define ASN_RFC1157_TRAP SNMP_PDU_V1TRAP
#define ASN_CONTEXTSPECIFIC ASN_CONTEXT
#define ASN_PRIMATIVE ASN_PRIMITIVE
#define RFC1157VarBindList SnmpVarBindList
#define RFC1157VarBind SnmpVarBind
#define AsnInteger AsnInteger32
#define AsnCounter AsnCounter32
#define AsnGauge AsnGauge32
#endif /* SNMPSTRICT */

#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif
#include <poppack.h>
#endif
