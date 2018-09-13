// wsnmp_sp.c
//
// Special functions for the WinSNMP library
// Copyright 1998 ACE*COMM Corp
//
// Bob Natale (bnatale@acecomm.com)
//
#include "winsnmp.inc"

__declspec(dllexport)
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetAgentAddress (LPSTR agentAddress)
{
DWORD tmpAddress = 0;
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
// A null arg resets the localAddress trigger value
if (agentAddress == NULL)
   goto DONE;
// Otherwise, convert to IP address
tmpAddress = inet_addr (agentAddress);
if (tmpAddress == INADDR_NONE)
   { // Invalid IP addresses cannot be accepted
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_OUT;
   }
DONE:
// Plug new agent_address value into localAddress
// for future v1 trap sends
EnterCriticalSection (&cs_TASK);
TaskData.localAddress = tmpAddress;
LeaveCriticalSection (&cs_TASK);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
}

__declspec(dllexport)
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpConveyAgentAddress (SNMPAPI_STATUS mode)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
// mode can only be on or off...
if (mode != SNMPAPI_ON)
   mode = SNMPAPI_OFF;  // ...force off if not on
EnterCriticalSection (&cs_TASK);
TaskData.conveyAddress = mode;
LeaveCriticalSection (&cs_TASK);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
}