// wsnmp_pd.c
//
// WinSNMP PDU Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// Removed extraneous functions
//
#include "winsnmp.inc"
LPVARBIND SnmpCopyVbl (LPVARBIND);
// PDU Functions
// SnmpCreatePdu
HSNMP_PDU SNMPAPI_CALL
   SnmpCreatePdu  (IN HSNMP_SESSION hSession,
                   IN smiINT PDU_type,
                   IN smiINT32 request_id,
                   IN smiINT error_status,
                   IN smiINT error_index,
                   IN HSNMP_VBL hVbl)
{
DWORD nPdu;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPPDUS pPdu;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
// We have a valid session at this point...
lSession = hSession; // save it for possible error return
if (hVbl)
	{
	if (!snmpValidTableEntry(&VBLsDescr, HandleToUlong(hVbl)-1))
		{
		lError = SNMPAPI_VBL_INVALID;
		goto ERROR_OUT;
		}
	}

if (!PDU_type)  // NULL is allowed and defaults to SNMP_PDU_GETNEXT
   PDU_type = SNMP_PDU_GETNEXT;
switch (PDU_type)
   {
   case SNMP_PDU_GET:
   case SNMP_PDU_GETNEXT:
   case SNMP_PDU_RESPONSE:
   case SNMP_PDU_SET:
   case SNMP_PDU_INFORM:
   case SNMP_PDU_TRAP:
   case SNMP_PDU_GETBULK:
   break;

   case SNMP_PDU_V1TRAP:
   default:
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }

EnterCriticalSection (&cs_PDU);
lError = snmpAllocTableEntry(&PDUsDescr, &nPdu);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

// Initialize new PDU components
pPdu->Session  = hSession;
pPdu->type = PDU_type;
pPdu->errStatus = error_status;
pPdu->errIndex  = error_index;
// If RequestID=0 at this point, assign one (which may be 0)
pPdu->appReqId = (request_id) ? request_id : ++(TaskData.nLastReqId);
pPdu->VBL_addr = NULL;
pPdu->VBL = (hVbl) ? hVbl : 0;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_PDU);

if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_PDU)(nPdu+1));
else
ERROR_OUT:
   return ((HSNMP_PDU)SaveError (lSession, lError));
} // end_SnmpCreatePdu

// SnmpGetPduData
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetPduData (IN HSNMP_PDU hPdu,
                   OUT smiLPINT PDU_type,
                   OUT smiLPINT32 request_id,
                   OUT smiLPINT error_status,
                   OUT smiLPINT error_index,
                   OUT LPHSNMP_VBL hVbl)
{
DWORD nPdu = HandleToUlong(hPdu) - 1;
DWORD done = 0;
HSNMP_SESSION hSession;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPPDUS pPdu;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&PDUsDescr, nPdu))
   {
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

// Use PDU's session for (possibly) creating hVbl later
hSession = pPdu->Session;
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
lSession = hSession; // Save for possible error return
if (PDU_type)
   {
   *PDU_type = pPdu->type;
   done++;
   }
if (request_id)
   {
   *request_id = pPdu->appReqId;
   done++;
   }
if (error_status)
   {
   *error_status = pPdu->errStatus;
   done++;
   }
if (error_index)
   {
   *error_index = pPdu->errIndex;
   done++;
   }
if (hVbl)
   {
   DWORD nVbl; // Important control variable
   *hVbl = 0;
   EnterCriticalSection (&cs_VBL);
   // First case is on a created (not received) PDU
   // which has not yet been assigned a VBL
   if ((!pPdu->VBL) && (!pPdu->VBL_addr))
      goto DONE_VBL;
   // If there is a VBL already assinged to the PDU,
   // then duplicate it
   if (pPdu->VBL)
      { // Per policy, create a new hVbl resource for the calling app
      if (!(*hVbl = SnmpDuplicateVbl (hSession, pPdu->VBL)))
         {
         lError = SNMPAPI_VBL_INVALID;
         goto ERROR_PRECHECK;
         }
      goto DONE_VBL;
      }
   // This must be a received PDU and
   // the first call to extract the VBL
   lError = snmpAllocTableEntry(&VBLsDescr, &nVbl);
   if (lError != SNMPAPI_SUCCESS)
	   goto ERROR_PRECHECK;
   pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

   pVbl->Session = hSession;
   pVbl->vbList = pPdu->VBL_addr;
   // Clear received vbList address...IMPORTANT!
   pPdu->VBL_addr = NULL;
   *hVbl = pPdu->VBL = (HSNMP_VBL)(nVbl+1);
DONE_VBL:
   done++;
ERROR_PRECHECK:
   LeaveCriticalSection (&cs_VBL);
   } // end_if vbl
if (done == 0)
   lError = SNMPAPI_NOOP;
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpGetPduData

// SnmpSetPduData
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetPduData (IN HSNMP_PDU hPdu,
                   IN const smiINT FAR *PDU_type,
                   IN const smiINT32 FAR *request_id,
                   IN const smiINT FAR *non_repeaters,
                   IN const smiINT FAR *max_repetitions,
                   IN const HSNMP_VBL FAR *hVbl)
{
DWORD nPdu = HandleToUlong(hPdu) - 1;
DWORD done = 0;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPPDUS pPdu;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }

if (!snmpValidTableEntry(&PDUsDescr, nPdu))
   {
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

lSession = pPdu->Session; // Save for possible error return
EnterCriticalSection (&cs_PDU);
if (!IsBadReadPtr((LPVOID)PDU_type, sizeof(smiINT)))
   {
   if (*PDU_type == SNMP_PDU_V1TRAP)
       {
           lError = SNMPAPI_PDU_INVALID;
           goto ERROR_OUT;
       }
   pPdu->type = *PDU_type;
   done++;
   }
if (!IsBadReadPtr((LPVOID)request_id, sizeof(smiINT32)))
   {
   pPdu->appReqId = *request_id;
   done++;
   }
if (!IsBadReadPtr((LPVOID)non_repeaters, sizeof(smiINT)))
   {
   pPdu->errStatus = *non_repeaters;
   done++;
   }
if (!IsBadReadPtr((LPVOID)max_repetitions, sizeof(smiINT)))
   {
   pPdu->errIndex = *max_repetitions;
   done++;
   }
if (!IsBadReadPtr((LPVOID)hVbl, sizeof(HSNMP_VBL)))
   { // Assign new vbl
   HSNMP_VBL tVbl = *hVbl;
   // Check for validity
   if (!snmpValidTableEntry(&VBLsDescr, HandleToUlong(tVbl)-1))
      { // If not, disallow operation
      lError = SNMPAPI_VBL_INVALID;
      goto ERROR_PRECHECK;
      }
   pPdu->VBL = tVbl;
   // Following case can happen if a a vbl is assigned to a
   // response pdu which never had its vbl dereferenced...
   if (pPdu->VBL_addr) //...then is must be freed
      FreeVarBindList (pPdu->VBL_addr);
   pPdu->VBL_addr = NULL;
   done++;
   } // end_if vbl
ERROR_PRECHECK:
LeaveCriticalSection (&cs_PDU);
if (done == 0)
   lError = ((PDU_type == NULL) && 
            (request_id == NULL) &&
            (non_repeaters == NULL) &&
            (max_repetitions == NULL) &&
            (hVbl == NULL)) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpSetPduData

// SnmpDuplicatePdu
HSNMP_PDU SNMPAPI_CALL
   SnmpDuplicatePdu  (IN HSNMP_SESSION hSession,
                      IN HSNMP_PDU hPdu)
{
DWORD lPdu = HandleToUlong(hPdu) - 1;
DWORD nPdu;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPPDUS pOldPdu, pNewPdu;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
lSession = hSession; // save for possible error return
if (!snmpValidTableEntry(&PDUsDescr, lPdu) ||
    (pOldPdu = snmpGetTableEntry(&PDUsDescr, lPdu))->type == SNMP_PDU_V1TRAP)
   {
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }

EnterCriticalSection (&cs_PDU);

lError = snmpAllocTableEntry(&PDUsDescr, &nPdu);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pNewPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

CopyMemory (pNewPdu, pOldPdu, sizeof(PDUS));
pNewPdu->Session = hSession; // Can be different
// Setting the VBL value in the duplicated PDU is "tricky"...
// If there is a VBL value, just replicate it and that's that.
// If VBL = 0 and there is no VBL_addr attached to the pdu_ptr,
// then the VBL value is 0 and is already set from the original PDU.
// Otherwise, this is a received PDU from which the varbindlist has
// not yet been extracted and must, therefore, be re-produced.
// This third case is covered in the next *2* lines.
if ((!pOldPdu->VBL) && pOldPdu->VBL_addr)
   pNewPdu->VBL_addr = SnmpCopyVbl (pOldPdu->VBL_addr);
ERROR_PRECHECK:
LeaveCriticalSection (&cs_PDU);
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_PDU)(nPdu+1));
ERROR_OUT:
return ((HSNMP_PDU)SaveError (lSession, lError));
} // end_SnmpDuplicatePdu

// SnmpFreePdu
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpFreePdu (IN HSNMP_PDU hPdu)
{
DWORD nPdu = HandleToUlong(hPdu) - 1;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
LPPDUS pPdu;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&PDUsDescr, nPdu))
   {
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

EnterCriticalSection (&cs_PDU);
if (pPdu->VBL_addr)
   FreeVarBindList (pPdu->VBL_addr);
snmpFreeTableEntry(&PDUsDescr, nPdu);
LeaveCriticalSection (&cs_PDU);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpfreePdu
