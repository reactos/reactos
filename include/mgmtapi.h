/*
  mgmtapi.h - Header file for the SNMP Management API

  Written by Filip Navara <xnavara@volny.cz>

  References (2003-08-25):
    http://msdn.microsoft.com/library/en-us/snmp/snmp/snmp_reference.asp

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef _MGMTAPI_H
#define _MGMTAPI_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#ifndef _SNMP_H
#include <snmp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SNMP_MGMTAPI_TIMEOUT	40
#define SNMP_MGMTAPI_SELECT_FDERRORS	41
#define SNMP_MGMTAPI_TRAP_ERRORS	42
#define SNMP_MGMTAPI_TRAP_DUPINIT	43
#define SNMP_MGMTAPI_NOTRAPS	44
#define SNMP_MGMTAPI_AGAIN	45
#define SNMP_MGMTAPI_INVALID_CTL	46
#define SNMP_MGMTAPI_INVALID_SESSION	47
#define SNMP_MGMTAPI_INVALID_BUFFER	48
#define MGMCTL_SETAGENTPORT	1

#ifndef RC_INVOKED

typedef PVOID LPSNMP_MGR_SESSION;

BOOL WINSNMPAPI SnmpMgrClose(LPSNMP_MGR_SESSION);
BOOL WINSNMPAPI SnmpMgrCtl(LPSNMP_MGR_SESSION,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD);
BOOL WINSNMPAPI SnmpMgrGetTrap(AsnObjectIdentifier*,AsnNetworkAddress*,AsnInteger*,AsnInteger*,AsnTimeticks*,SnmpVarBindList*);
BOOL WINSNMPAPI SnmpMgrGetTrapEx(AsnObjectIdentifier*,AsnNetworkAddress*,AsnNetworkAddress*,AsnInteger*,AsnInteger*,AsnOctetString*,AsnTimeticks*,SnmpVarBindList*);
BOOL WINSNMPAPI SnmpMgrOidToStr(AsnObjectIdentifier*,LPSTR*);
LPSNMP_MGR_SESSION WINSNMPAPI SnmpMgrOpen(LPSTR,LPSTR,INT,INT);
INT WINSNMPAPI SnmpMgrRequest(LPSNMP_MGR_SESSION,BYTE,SnmpVarBindList*,AsnInteger*,AsnInteger*);
BOOL WINSNMPAPI SnmpMgrStrToOid(LPSTR,AsnObjectIdentifier*);
BOOL WINSNMPAPI SnmpMgrTrapListen(HANDLE*);

#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif
#endif
