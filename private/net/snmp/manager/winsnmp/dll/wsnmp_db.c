// wsnmp_db.c
//
// WinSNMP Local Database Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
#include "winsnmp.inc"

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetVendorInfo (OUT smiLPVENDORINFO vendorInfo)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (vendorInfo == NULL)
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_OUT;
   }
if (IsBadWritePtr(vendorInfo, sizeof(smiVENDORINFO)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// Max len = 64
lstrcpy (&vendorInfo->vendorName[0], "Microsoft Corporation");
lstrcpy (&vendorInfo->vendorContact[0], "snmpinfo@microsoft.com");
// Max len = 32
lstrcpy (&vendorInfo->vendorVersionId[0], "v2.32.19980808");
lstrcpy (&vendorInfo->vendorVersionDate[0], "August 8, 1998");
vendorInfo->vendorEnterprise = 311;
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpGetVendorInfo()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetTranslateMode (OUT smiLPUINT32 nTranslateMode)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
// Must have some place to write answer to...
if (IsBadWritePtr (nTranslateMode, sizeof(smiUINT32)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// Ok to write value
*nTranslateMode = TaskData.nTranslateMode;
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpGetTranslateMode()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetTranslateMode (IN smiUINT32 nTranslateMode)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
switch (nTranslateMode)
   {
   case SNMPAPI_TRANSLATED:
   case SNMPAPI_UNTRANSLATED_V1:
   case SNMPAPI_UNTRANSLATED_V2:
   EnterCriticalSection (&cs_TASK);
   TaskData.nTranslateMode = nTranslateMode;
   LeaveCriticalSection (&cs_TASK);
   break;

   default:
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_OUT;
   }
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpSetTranslateMode()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetRetransmitMode (OUT smiLPUINT32 nRetransmitMode)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
// Must have some place to write answer to...
if (IsBadWritePtr (nRetransmitMode, sizeof(smiUINT32)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// Ok to write value
*nRetransmitMode = TaskData.nRetransmitMode;
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpGetRetransmitMode()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetRetransmitMode (IN smiUINT32 nRetransmitMode)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (nRetransmitMode != SNMPAPI_OFF && nRetransmitMode != SNMPAPI_ON)
   {
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_OUT;
   }
EnterCriticalSection (&cs_TASK);
TaskData.nRetransmitMode = nRetransmitMode;
LeaveCriticalSection (&cs_TASK);
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpSetRetransmitMode()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetTimeout (IN  HSNMP_ENTITY hEntity,
                   OUT smiLPTIMETICKS nPolicyTimeout,
                   OUT smiLPTIMETICKS nActualTimeout)
{
DWORD nEntity;
SNMPAPI_STATUS lError;
LPENTITY pEntity;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nEntity = HandleToUlong(hEntity) - 1;
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);

if (!nPolicyTimeout && !nActualTimeout)
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_OUT;
   }
// Intervals are specified and stored as centiseconds
if (nPolicyTimeout)
   {
   if (IsBadWritePtr (nPolicyTimeout, sizeof(smiTIMETICKS)))
      {
      lError  = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   *nPolicyTimeout = pEntity->nPolicyTimeout;
   }
if (nActualTimeout)
   {
   if (IsBadWritePtr (nActualTimeout, sizeof(smiTIMETICKS)))
      {
      lError  = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   *nActualTimeout = pEntity->nActualTimeout;
   }
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpGetTimeout()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetTimeout (IN HSNMP_ENTITY hEntity,
                   IN smiTIMETICKS nPolicyTimeout)
{
DWORD nEntity;
SNMPAPI_STATUS lError;
LPENTITY pEntity;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nEntity = HandleToUlong(hEntity) - 1;
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);
EnterCriticalSection (&cs_ENTITY);
// Timeout interval is specified and stored in centiseconds
pEntity->nPolicyTimeout = nPolicyTimeout;
LeaveCriticalSection (&cs_ENTITY);
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpSetTimeout()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetRetry (IN HSNMP_ENTITY hEntity,
                 OUT smiLPUINT32 nPolicyRetry,
                 OUT smiLPUINT32 nActualRetry)
{
DWORD nEntity;
SNMPAPI_STATUS lError;
LPENTITY pEntity;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nEntity = HandleToUlong(hEntity) - 1;
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);
if (!nPolicyRetry && !nActualRetry)
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_OUT;
   }
if (nPolicyRetry)
   {
   if (IsBadWritePtr (nPolicyRetry, sizeof(smiUINT32)))
      {
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   *nPolicyRetry = pEntity->nPolicyRetry;
   }
if (nActualRetry)
   {
   if (IsBadWritePtr (nActualRetry, sizeof(smiUINT32)))
      {
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   *nActualRetry = pEntity->nActualRetry;
   }
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpGetRetry()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetRetry (IN HSNMP_ENTITY hEntity,
                 IN smiUINT32 nPolicyRetry)
{
DWORD nEntity;
SNMPAPI_STATUS lError;
LPENTITY pEntity;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nEntity = HandleToUlong(hEntity) - 1;
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_INVALID;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);

EnterCriticalSection (&cs_ENTITY);
pEntity->nPolicyRetry = nPolicyRetry;
LeaveCriticalSection (&cs_ENTITY);
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpSetRetry()
