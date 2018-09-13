// wsnmp_ut.c
//
// WinSNMP Utility Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 980424 - BobN
//        - Mods to SnmpStrToIpxAddress() to permit '.' char
//        - as netnum/nodenum separator
// 970310 - Removed extraneous functions
//
#include "winsnmp.inc"

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetLastError (IN HSNMP_SESSION hSession)
{
	DWORD nSes;

	if (TaskData.hTask == 0)
	   return (SNMPAPI_NOT_INITIALIZED);
	nSes = HandleToUlong(hSession) - 1;
	if (snmpValidTableEntry(&SessDescr, nSes))
	{
		LPSESSION pSession = snmpGetTableEntry(&SessDescr, nSes);
		return pSession->nLastError;
	}
	else
	   return (TaskData.nLastError);
} // end_SnmpGetLastError

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpStrToOid (IN LPCSTR string,
                 OUT smiLPOID dstOID)
{
smiUINT32 i;
smiUINT32 compIdx;
SNMPAPI_STATUS lError;
CHAR c;
LPSTR pSep;

// Must be initialized
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }

// use __try, __except to figure out if 'string' is a
// valid pointer. We cannot use IsBadReadPtr() here, as far as
// we have no idea for how many octets we should look.
__try
{
    smiUINT32 sLen;

    sLen = strlen(string);
    if (sLen == 0 || sLen >= MAXOBJIDSTRSIZE)
    {
        lError = SNMPAPI_OID_INVALID;
        goto ERROR_OUT;
    }
}
__except(EXCEPTION_EXECUTE_HANDLER)
{
    lError = SNMPAPI_ALLOC_ERROR;
        goto ERROR_OUT;
}

// see if the dstOID pointer provided by the caller points to 
// a valid memory range. If null is provided, there is nothing
// the API was requested to do!
if (IsBadWritePtr (dstOID, sizeof(smiOID)))
{
   lError = (dstOID == NULL) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
}

// Ignore initial '.' in string (UNIX-ism)
if (string[0] == '.')
    string++;

// figure out how many components this OID has
// count the number of '.' in the string. The OID should 
// contain this count + 1 components
dstOID->len = 0;
pSep = (LPSTR)string;
while((pSep = strchr(pSep, '.')) != NULL)
{
    pSep++;
    dstOID->len++;
}
dstOID->len++;

// don't allow less than 2 components
if (dstOID->len < 2)
{
    lError = SNMPAPI_OID_INVALID;
    goto ERROR_OUT;
}

// allocate memory for holding the numeric OID components
// this should be released by the caller, through 'SnmpFreeDescriptor()'
dstOID->ptr = (smiLPUINT32)GlobalAlloc(GPTR, dstOID->len * sizeof(smiUINT32));
if (dstOID->ptr == NULL)
{
    lError = SNMPAPI_ALLOC_ERROR;
    goto ERROR_OUT;
}

SetLastError(ERROR_SUCCESS);
compIdx = 0;
// when entering the loop, 'string' doesn't have a heading '.'
while (*string != '\0')
{
    dstOID->ptr[compIdx++] = strtoul(string, &pSep, 10);

    // if one of the components was overflowing, release the memory and bail out.
    if (GetLastError() != ERROR_SUCCESS)
    {
        lError = SNMPAPI_OID_INVALID;
        GlobalFree(dstOID->ptr);
        dstOID->ptr = NULL;
        goto ERROR_OUT;
    }

    // if strtoul did not make any progress on the string (two successive dots)
    // or it was blocked on something else than a separator or null-termination, then
    // there was an error. The OID is still valid but truncated. API return failure
    if (pSep == string ||
        (*pSep != '.' && *pSep != '\0'))
    {
        lError =  SNMPAPI_OUTPUT_TRUNCATED; // invalid char in sequence
        dstOID->len = compIdx;              // update the len, for accuracy
        goto ERROR_OUT;                     // the OID is valid, but truncated. The caller should
                                            // release the memory with SnmpFreeDescriptor()
    }

    // pSep can point only to '.' or '\0'
    if (*pSep == '.')
        pSep++;

    // restart with string from this point
    string = pSep;
}

if (dstOID->len < 2)
{
    GlobalFree(dstOID->ptr);
    dstOID->ptr = NULL;
    lError = SNMPAPI_OID_INVALID;
    goto ERROR_OUT;
}

return dstOID->len;

ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpStrToOid()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpOidToStr (IN smiLPCOID srcOID,
                 IN smiUINT32 strLen,
                 OUT LPSTR strPtr)
{
SNMPAPI_STATUS lError;
smiUINT32 retLen = 0;      // used for successful return
smiUINT32 oidIdx = 0;      // max subids is 128
smiUINT32 tmpLen;          // used for size of decoded string (with '.')
LPSTR tmpPtr = strPtr;     // used for advancing strPtr
char tmpBuf[16];           // room for 1 32-bit decode and '.'
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }

if (!strLen)
   {
   lError = SNMPAPI_SIZE_INVALID;
   goto ERROR_OUT;
   }

if (IsBadReadPtr(srcOID, sizeof(smiOID)) ||
    IsBadWritePtr(strPtr, strLen))
   {
    lError = (strPtr == NULL) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
    goto ERROR_OUT;
   }

if (srcOID->len == 0 || srcOID->len > 128 ||
    IsBadReadPtr (srcOID->ptr, srcOID->len * sizeof(smiUINT32)))
   {
   lError = SNMPAPI_OID_INVALID;
   goto ERROR_OUT;
   }

while (oidIdx < srcOID->len)
   {
   _ultoa (srcOID->ptr[oidIdx++], tmpBuf, 10);
   lstrcat (tmpBuf, ".");
   tmpLen = lstrlen (tmpBuf);
   if (strLen < (tmpLen + 1))
      {
      tmpBuf[strLen] = '\0';
      lstrcpy (tmpPtr, tmpBuf);
      lError = SNMPAPI_OUTPUT_TRUNCATED;
      goto ERROR_OUT;
      }
   lstrcpy (tmpPtr, tmpBuf);
   strLen -= tmpLen;
   tmpPtr += tmpLen;
   retLen += tmpLen;
   }  // end_while
*(--tmpPtr) = '\0';
return (retLen);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpOidToStr

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpOidCopy (IN smiLPCOID srcOID,
                OUT smiLPOID dstOID)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (IsBadReadPtr (srcOID, sizeof(smiOID)) ||
    IsBadReadPtr (srcOID->ptr, srcOID->len) ||
    IsBadWritePtr (dstOID, sizeof(smiOID)))
   {
   lError = (dstOID == NULL) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// Check input OID size
if ((srcOID->len == 0) ||(srcOID->len > MAXOBJIDSIZE))
   {
   lError = SNMPAPI_OID_INVALID;
   goto ERROR_OUT;
   }
// Using dstOID-> temporarily for byte count
dstOID->len = srcOID->len * sizeof(smiUINT32);
// App must free following alloc via SnmpFreeDescriptor()
if (!(dstOID->ptr = (smiLPUINT32)GlobalAlloc (GPTR, dstOID->len)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
CopyMemory (dstOID->ptr, srcOID->ptr, dstOID->len);
// Now make dstOID->len mean the right thing
dstOID->len = srcOID->len;
return (dstOID->len);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpOidCopy()

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpFreeDescriptor (IN smiUINT32 syntax,
                       IN smiLPOPAQUE ptr)
{
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!syntax || !ptr || !ptr->ptr)
   {
   lError = SNMPAPI_OPERATION_INVALID;
   goto ERROR_OUT;
   }
switch (syntax)
   {
   case SNMP_SYNTAX_OCTETS:
   case SNMP_SYNTAX_IPADDR:
   case SNMP_SYNTAX_OPAQUE:
   case SNMP_SYNTAX_OID:
   if (GlobalFree (ptr->ptr)) // returns not-NULL on error
      {
      lError = SNMPAPI_OTHER_ERROR;
      goto ERROR_OUT;
      }
   ptr->ptr = NULL;
   ptr->len = 0;
   break;

   default:
   lError = SNMPAPI_SYNTAX_INVALID;
   goto ERROR_OUT;
   } // end_switch
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
}  // end_SnmpFreeDescriptor

// SnmpOidCompare
//
// Re-worked by 3/17/95 BobN
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpOidCompare (IN smiLPCOID xOID,
                   IN smiLPCOID yOID,
                   IN smiUINT32 maxlen,
                   OUT smiLPINT result)
{
smiUINT32 i = 0;
smiUINT32 j = 0;
SNMPAPI_STATUS lError;
if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (maxlen > MAXOBJIDSIZE)
   {
   lError = SNMPAPI_SIZE_INVALID;
   goto ERROR_OUT;
   }

if (IsBadReadPtr (xOID, sizeof(smiOID)) ||
    IsBadReadPtr (yOID, sizeof(smiOID)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }

if (IsBadReadPtr (xOID->ptr, xOID->len * sizeof(UINT)) ||
    IsBadReadPtr (yOID->ptr, yOID->len * sizeof(UINT)))
   {
   lError = SNMPAPI_OID_INVALID;
   goto ERROR_OUT;
   }

// Test input pointers for readability
if (IsBadWritePtr (result, sizeof(smiINT)))
    {
    lError = (result == NULL) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
    goto ERROR_OUT;
    }

j = min(xOID->len, yOID->len);
if (maxlen) j = min(j, maxlen);
while (i < j)
   {
   if (*result = xOID->ptr[i] - yOID->ptr[i]) // deliberate assignment
      return (SNMPAPI_SUCCESS);               // not equal...got a winner!
   i++;
   }
if (j == maxlen)                              // asked for a limit
   return (SNMPAPI_SUCCESS);                  // and...got a draw!
*result = xOID->len - yOID->len;              // size matters!
return SNMPAPI_SUCCESS;
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpOidCompare

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpEncodeMsg (IN HSNMP_SESSION hSession,
                  IN HSNMP_ENTITY hSrc,
                  IN HSNMP_ENTITY hDst,
                  IN HSNMP_CONTEXT hCtx,
                  IN HSNMP_PDU hPdu,
                  IN OUT smiLPOCTETS msgBufDesc)
{
smiUINT32 version = 0;
DWORD nCtx;
DWORD nPdu;
DWORD nVbl;
smiOCTETS tmpContext;
smiLPBYTE msgAddr = NULL;
smiUINT32 lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPPDUS pPdu;
LPENTITY pEntSrc, pEntDst;
LPCTXT pCtxt;

// Basic error checks
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
// Check for writable output buffer
if (IsBadWritePtr (msgBufDesc, sizeof(smiOCTETS)))
   {
   lError = (msgBufDesc == NULL) ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// srcEntity not currently used
if (hSrc)
   {
    if (!snmpValidTableEntry(&EntsDescr, HandleToUlong(hSrc)-1))
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
    pEntSrc = snmpGetTableEntry(&EntsDescr, HandleToUlong(hSrc)-1);
   }
// dstEntity is required for *accurate* msg version info
if (hDst)
   {
   if (!snmpValidTableEntry(&EntsDescr, HandleToUlong(hDst)-1))
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
   pEntDst = snmpGetTableEntry(&EntsDescr, HandleToUlong(hDst)-1);
   version = pEntDst->version-1;
   }
nCtx = HandleToUlong(hCtx) - 1;
if (!snmpValidTableEntry(&CntxDescr, nCtx))
   {
   lError = SNMPAPI_CONTEXT_INVALID;
   goto ERROR_OUT;
   }
pCtxt = snmpGetTableEntry(&CntxDescr, nCtx);

nPdu = HandleToUlong(hPdu) - 1;
if (!snmpValidTableEntry(&PDUsDescr, nPdu))
   {
ERROR_PDU:
   lError = SNMPAPI_PDU_INVALID;
   goto ERROR_OUT;
   }
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

// Necessary PDU data checks
nVbl = HandleToUlong(pPdu->VBL);
if (!snmpValidTableEntry(&VBLsDescr, nVbl-1))
   goto ERROR_PDU;
// Check out for SNMPv1 Trap PDU type...uses different PDU structure!
// ???
// Check for SNMPv2c PDU types
if (pPdu->type == SNMP_PDU_TRAP ||
    pPdu->type == SNMP_PDU_INFORM)
   version = 1;
// Now Build it
tmpContext.len = pCtxt->commLen;
tmpContext.ptr = pCtxt->commStr;
if (!(BuildMessage (version, &tmpContext,
                    pPdu, pPdu->appReqId,
                    &msgAddr, &msgBufDesc->len)))
   goto ERROR_PDU;
// Copy Snmp message to caller's buffer...
// App must free following alloc via SnmpFreeDescriptor()
if (!(msgBufDesc->ptr = (smiLPBYTE)GlobalAlloc (GPTR, msgBufDesc->len)))
   lError = SNMPAPI_ALLOC_ERROR;
else // SUCCESS
   CopyMemory (msgBufDesc->ptr, msgAddr, msgBufDesc->len);
ERROR_OUT:
// Clean up
if (msgAddr)
   GlobalFree (msgAddr);
if (lError == SNMPAPI_SUCCESS)
   return (msgBufDesc->len);
else // Failure cases
   return (SaveError (lSession, lError));
}

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpDecodeMsg (IN HSNMP_SESSION hSession,
                  OUT LPHSNMP_ENTITY hSrc,
                  OUT LPHSNMP_ENTITY hDst,
                  OUT LPHSNMP_CONTEXT hCtx,
                  OUT LPHSNMP_PDU hPdu,
                  IN smiLPCOCTETS msgPtr)
{
DWORD nPdu;
smiLPOCTETS community;
smiUINT32 version;
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
// Valid session...save for possible error return
lSession = hSession;

if (IsBadReadPtr(msgPtr, sizeof(smiOCTETS)) ||
    IsBadReadPtr(msgPtr->ptr, msgPtr->len))
{
    lError = SNMPAPI_ALLOC_ERROR;
    goto ERROR_OUT;
}

if (hSrc == NULL && hDst == NULL && hCtx == NULL && hPdu == NULL)
{
    lError = SNMPAPI_NOOP;
    goto ERROR_OUT;
}

if ((hDst != NULL && IsBadWritePtr(hDst, sizeof(HSNMP_ENTITY))) ||
    (hSrc != NULL && IsBadWritePtr(hSrc, sizeof(HSNMP_ENTITY))))
{
    lError = SNMPAPI_ENTITY_INVALID;
    goto ERROR_OUT;
}

if (hCtx != NULL && IsBadWritePtr(hCtx, sizeof(HSNMP_CONTEXT)))
{
    lError = SNMPAPI_CONTEXT_INVALID;
    goto ERROR_OUT;
}

if (IsBadWritePtr(hPdu, sizeof(HSNMP_PDU)))
{
    lError = SNMPAPI_PDU_INVALID;
    goto ERROR_OUT;
}

EnterCriticalSection (&cs_PDU);
lError = snmpAllocTableEntry(&PDUsDescr, &nPdu);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pPdu = snmpGetTableEntry(&PDUsDescr, nPdu);

if (ParseMessage (msgPtr->ptr, msgPtr->len, &version, &community, pPdu))
   { // non-zero = some error code
   lError = SNMPAPI_MESSAGE_INVALID;
   SnmpFreePdu((HSNMP_PDU)(nPdu+1));
   goto ERROR_PRECHECK;
   }
if (hDst != NULL) *hDst = 0;
if (hSrc != NULL) *hSrc = 0;
if (hCtx != NULL)
   {
   smiUINT32 nMode;
   EnterCriticalSection (&cs_XMODE);
   SnmpGetTranslateMode (&nMode);
   SnmpSetTranslateMode (SNMPAPI_UNTRANSLATED_V1);
   *hCtx = SnmpStrToContext (hSession, community);
   SnmpSetTranslateMode (nMode);
   LeaveCriticalSection (&cs_XMODE);
   }
FreeOctetString (community);
pPdu->Session  = hSession;
if (hPdu != NULL)
    *hPdu = (HSNMP_PDU)(nPdu+1);
else
    SnmpFreePdu((HSNMP_PDU)(nPdu+1));

ERROR_PRECHECK:
LeaveCriticalSection (&cs_PDU);
if (lError == SNMPAPI_SUCCESS)
{
   SaveError(lSession, lError);
   return (msgPtr->len);
}

ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpDecodeMsg()

#define NETLEN  4
#define NODELEN 6
char *cHexDigits = "0123456789ABCDEF";
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpStrToIpxAddress (LPCSTR str, LPBYTE netnum, LPBYTE nodenum)
{
LPSTR netPtr, nodePtr, pStr;
DWORD i, j;
char tmpStr[24];
BYTE c1, c2;
if (!str || !netnum || !nodenum)
   return (SNMPAPI_FAILURE);
lstrcpy (tmpStr, str);
netPtr = strtok (tmpStr, "-:.");
if (netPtr == NULL)
   return (SNMPAPI_FAILURE);
if (lstrlen (netPtr) != NETLEN*2)
   return (SNMPAPI_FAILURE);
nodePtr = netPtr + (NETLEN*2) + 1;
if (lstrlen (nodePtr) != NODELEN*2)
   return (SNMPAPI_FAILURE);
_strupr (netPtr);
for (i = 0, j = 0; j < NETLEN; j++)
   {
   pStr = strchr (cHexDigits, netPtr[i++]);
   if (pStr == NULL)
       return (SNMPAPI_FAILURE);
   c1 = (BYTE)(pStr - cHexDigits);
   pStr = strchr (cHexDigits, netPtr[i++]);
   if (pStr == NULL)
       return (SNMPAPI_FAILURE);
   c2 = (BYTE)(pStr - cHexDigits);
   netnum[j] = c2 | c1 << 4;
   }
_strupr (nodePtr);
for (i = 0, j = 0; j < NODELEN; j++)
   {
   pStr = strchr (cHexDigits, nodePtr[i++]);
   if (pStr == NULL)
       return (SNMPAPI_FAILURE);
   c1 = (BYTE)(pStr - cHexDigits);
   pStr = strchr (cHexDigits, nodePtr[i++]);
   if (pStr == NULL)
       return (SNMPAPI_FAILURE);
   c2 = (BYTE)(pStr - cHexDigits);
   nodenum[j] = c2 | c1 << 4;
   }
return (SNMPAPI_SUCCESS);
}

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpIpxAddressToStr (LPBYTE netnum, LPBYTE nodenum, LPSTR str)
{
DWORD i, j;
for (i = 0, j = 0; i < NETLEN; i++)
   {
   str[j++] = cHexDigits[(netnum[i] & 0xF0) >> 4];
   str[j++] = cHexDigits[netnum[i] & 0x0F];
   }
str[j++] = ':';
for (i = 0; i < NODELEN; i++)
   {
   str[j++] = cHexDigits[(nodenum[i] & 0xF0) >> 4];
   str[j++] = cHexDigits[nodenum[i] & 0x0F];
   }
str[j] = '\0';
return (SNMPAPI_SUCCESS);
}
