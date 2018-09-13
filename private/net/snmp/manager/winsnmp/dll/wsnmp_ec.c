// wsnmp_ec.c
//
// WinSNMP Entity/Context Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 980424 - BobN
//        - Mods to SnmpStrToEntity() to support corresponding
//        - mods to SnmpStrToIpxAddress() to permit '.' char as
//        - netnum/nodenum separator
// 970310 - Typographical changes
//
#include "winsnmp.inc"
SNMPAPI_STATUS SNMPAPI_CALL SnmpStrToIpxAddress (LPCSTR, LPBYTE, LPBYTE);

// SnmpStrToEntity
HSNMP_ENTITY SNMPAPI_CALL
   SnmpStrToEntity (IN HSNMP_SESSION hSession,
                    IN LPCSTR entityString)
{
DWORD strLen;
LPCSTR tstStr;
LPSTR profilePtr;
LPSTR comma = ",";
DWORD nEntity;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
char profileBuf[256];
LPENTITY pEntity;

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
if (!entityString || (strLen = lstrlen(entityString)) == 0)
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
// Must go through ERROR_PRECHECK label after next statement...
EnterCriticalSection (&cs_ENTITY);
// Search for Entity table entry to use
lError = snmpAllocTableEntry(&EntsDescr, &nEntity);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);

pEntity->version = 0;
pEntity->nPolicyTimeout = DEFTIMEOUT;
pEntity->nPolicyRetry = DEFRETRY;
if (strLen > MAX_FRIEND_NAME_LEN)
   strLen = MAX_FRIEND_NAME_LEN;
switch (TaskData.nTranslateMode)
   {
   case SNMPAPI_TRANSLATED:
   // the entity is picked up from NP_WSNMP.INI, from [Entities] section:
   // [Entities]
   // EntityFriendlyName = ver#, ipaddr, timeout#, retries#, port#[,]
   // ------
   // Get the whole buffer
   if (!GetPrivateProfileString ("Entities", entityString, "",
                     profileBuf, sizeof(profileBuf), "NP_WSNMP.INI"))
      {
      snmpFreeTableEntry(&EntsDescr, nEntity);
      lError = SNMPAPI_ENTITY_UNKNOWN;
      goto ERROR_PRECHECK;
      }
   // pick up the ver# first (mandatory)
   profilePtr = strtok (profileBuf, comma);
   // if no token, is like we have a key with no value
   // bail out with SNMPAPI_NOOP
   if (profilePtr == NULL)
   {
       snmpFreeTableEntry(&EntsDescr, nEntity);
       lError = SNMPAPI_NOOP;
       goto ERROR_PRECHECK;
   }
   pEntity->version = atoi (profilePtr);

   // pick up the dotted ip address (mandatory)
   tstStr = strtok (NULL, comma); // Save real address string
   // if no address is specified, we don't have the vital info, so there's nothing to do
   // bail with SNMPAPI_NOOP
   if (tstStr == NULL)
   {
       snmpFreeTableEntry(&EntsDescr, nEntity);
	   lError = SNMPAPI_NOOP;
	   goto ERROR_PRECHECK;
   }

   // pick up the timeout# (optional)
   if (profilePtr = strtok (NULL, comma))
   {
		// The local database entry uses milliseconds for the timeout interval
		pEntity->nPolicyTimeout = atol (profilePtr);
		// Adjust for centiseconds, as used by the WinSNMP API
		pEntity->nPolicyTimeout /= 10;

		// pick up the retry# (optional)
		if (profilePtr = strtok (NULL, comma))
		{
			pEntity->nPolicyRetry = atol (profilePtr);

			// pick up the port# (optional)
			if (profilePtr = strtok (NULL, comma))
				pEntity->addr.inet.sin_port = htons ((short)atoi (profilePtr));
         }
      }
   break;

   // "version" was set to 0 above
   // if _V2, it will be incremented twice
   // if _V1, it will be incremented only once
   case SNMPAPI_UNTRANSLATED_V2:
   pEntity->version++;
   case SNMPAPI_UNTRANSLATED_V1:
   pEntity->version++;
   tstStr = entityString;           // Save real address string
   break;

   default:
   snmpFreeTableEntry(&EntsDescr, nEntity);
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_PRECHECK;
   } // end_switch
CopyMemory (pEntity->name, entityString, strLen);
if (strncmp(tstStr, "255.255.255.255", 15) && inet_addr (tstStr) == INADDR_NONE)
   { // Not AF_INET, try AF_IPX
   if (SnmpStrToIpxAddress (tstStr,
                            pEntity->addr.ipx.sa_netnum,
                            pEntity->addr.ipx.sa_nodenum) == SNMPAPI_FAILURE)
      {
      LeaveCriticalSection (&cs_ENTITY);
      return ((HSNMP_ENTITY)SaveError (hSession, SNMPAPI_ENTITY_UNKNOWN));
      }
   pEntity->addr.ipx.sa_family = AF_IPX;
   if (pEntity->addr.ipx.sa_socket == 0)
      pEntity->addr.ipx.sa_socket = htons (IPX_SNMP_PORT);
   }
else
   { // AF_INET
   pEntity->addr.inet.sin_family = AF_INET;
   if (pEntity->addr.inet.sin_port == 0)
      pEntity->addr.inet.sin_port = htons (IP_SNMP_PORT);
   pEntity->addr.inet.sin_addr.s_addr = inet_addr (tstStr);
   }
// Record the creating session
pEntity->Session = hSession;
// Initialize refCount for SnmpFreeEntity garbage collection
pEntity->refCount = 1;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_ENTITY);
ERROR_OUT:
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_ENTITY)(nEntity+1));
else // Failure cases
   return ((HSNMP_ENTITY)SaveError (lSession, lError));
} //end_SnmpStrToEntity

// SnmpEntityToStr
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpEntityToStr (IN HSNMP_ENTITY hEntity,
                    IN smiUINT32 size,
                    OUT LPSTR string)
{
DWORD nEntity = HandleToUlong(hEntity) - 1;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPSTR str;
smiUINT32 len;
char tmpStr[24];
LPENTITY pEntity;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&EntsDescr, nEntity))
   {
   lError = SNMPAPI_ENTITY_UNKNOWN;
   goto ERROR_OUT;
   }
pEntity = snmpGetTableEntry(&EntsDescr, nEntity);
lSession = pEntity->Session;
if (size == 0)
   {
   lError = SNMPAPI_SIZE_INVALID;
   goto ERROR_OUT;
   }
if (IsBadWritePtr(string, size))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
len = 0;
if (TaskData.nTranslateMode == SNMPAPI_TRANSLATED)
   {
   str = pEntity->name;
   len = lstrlen (str);
   }
else
   {
   if (pEntity->addr.inet.sin_family == AF_INET)
      {
      str = inet_ntoa (pEntity->addr.inet.sin_addr);
      len = lstrlen (str);
      }
   else if (pEntity->addr.ipx.sa_family == AF_IPX)
      {
      SnmpIpxAddressToStr (pEntity->addr.ipx.sa_netnum,
                           pEntity->addr.ipx.sa_nodenum,
                           tmpStr);
      str = tmpStr;
      len = lstrlen (str);
      }
   else
      {
      lError = SNMPAPI_ENTITY_INVALID;
      goto ERROR_OUT;
      }
   }
if (len >= size)
   {
   CopyMemory (string, str, size);
   string[size-1] = '\0';
   lError = SNMPAPI_OUTPUT_TRUNCATED;
   goto ERROR_OUT;
   }
else
   {
   lstrcpy (string, str);
   return (len+1);
   }
// Failure cases
ERROR_OUT:
return (SaveError (lSession, lError));
} // End_SnmpEntityToStr

// SnmpFreeEntity
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpFreeEntity (IN HSNMP_ENTITY hEntity)
{
DWORD nEntity;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
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
// Decrement refCount (unless already 0 [error])
if (pEntity->refCount)
   pEntity->refCount--;
// Now actually free it...
if (pEntity->Agent == 0 &&     // but not if it's an Agent
    pEntity->refCount == 0)   // nor if other references exist
    snmpFreeTableEntry(&EntsDescr, nEntity);

LeaveCriticalSection (&cs_ENTITY);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
}

// SnmpStrToContext
// Allow for zero-length/NULL context...BN 3/12/96
HSNMP_CONTEXT  SNMPAPI_CALL
   SnmpStrToContext (IN HSNMP_SESSION hSession,
                     IN smiLPCOCTETS contextString)
{
DWORD strLen;
DWORD nContext;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
char profileBuf[256];
LPSTR profilePtr;
LPSTR comma = ",";
LPCTXT pCtxt;

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
lSession = hSession; // Save for possible error return
if (IsBadReadPtr (contextString, sizeof(smiOCTETS)))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }

if (IsBadReadPtr (contextString->ptr, contextString->len))
   {
   lError = SNMPAPI_CONTEXT_INVALID;
   goto ERROR_OUT;
   }
// Remember to allow for 0-len contexts (as above does)
EnterCriticalSection (&cs_CONTEXT);
// Search for Entity table entry to use
lError = snmpAllocTableEntry(&CntxDescr, &nContext);
if (lError != SNMPAPI_SUCCESS)
    goto ERROR_PRECHECK;
pCtxt = snmpGetTableEntry(&CntxDescr, nContext);

pCtxt->version = 0; // just to be sure
pCtxt->name[0] = pCtxt->commStr[0] = '\0';
// Following "if" test allows for zero-length/NULL community string
// (deliberate assignment in conditional...)
if (pCtxt->commLen = contextString->len)
   {
   switch (TaskData.nTranslateMode)
      {
      case SNMPAPI_TRANSLATED:
      if (!GetPrivateProfileString ("Contexts", contextString->ptr, "",
                                    profileBuf, sizeof(profileBuf), "NP_WSNMP.INI"))
         {
         snmpFreeTableEntry(&CntxDescr, nContext);
         lError = SNMPAPI_CONTEXT_UNKNOWN;
         goto ERROR_PRECHECK;
         }
      strLen = min(lstrlen (contextString->ptr), MAX_FRIEND_NAME_LEN);
      CopyMemory (pCtxt->name, contextString->ptr, strLen);

	  // pick up the version# for this context (mandatory)
      profilePtr = strtok (profileBuf, comma);
	  // if there is no such version# is like we have a INI key without its value,
	  // so bail out with SNMPAPI_NOOP
	  if (profilePtr == NULL)
	  {
          snmpFreeTableEntry(&CntxDescr, nContext);
		  lError = SNMPAPI_NOOP;
		  goto ERROR_PRECHECK;
	  }
      pCtxt->version = (DWORD) atoi (profilePtr);

	  // pick up the actual context value (mandatory)
      profilePtr = strtok (NULL, comma);
	  // if there is no such value, is like we have the friendly name but this is malformed
	  // and doesn't point to any actual context.
	  // bail out with SNMPAPI_NOOP
	  if (profilePtr == NULL)
	  {
          snmpFreeTableEntry(&CntxDescr, nContext);
		  lError = SNMPAPI_NOOP;
		  goto ERROR_PRECHECK;
	  }
      strLen = min(lstrlen (profilePtr), MAX_CONTEXT_LEN);
      pCtxt->commLen = strLen;
      CopyMemory (pCtxt->commStr, profilePtr, strLen);
      break;

      // "version" was set to 0 above
      // if _V2, it will be incremented twice
      // if _V1, it will be incremented only once
      case SNMPAPI_UNTRANSLATED_V2:
      pCtxt->version++;
      case SNMPAPI_UNTRANSLATED_V1:
      pCtxt->version++;
      strLen = min(contextString->len, MAX_CONTEXT_LEN);
      CopyMemory (pCtxt->commStr, contextString->ptr, strLen);
      break;

      default:
      snmpFreeTableEntry(&CntxDescr, nContext);
      lError = SNMPAPI_MODE_INVALID;
      goto ERROR_PRECHECK;
      } // end_switch
   // Remember that NULL community strings are allowed!
   } // end_if (on len)
// Record the creating session value
pCtxt->Session = hSession;
// Initialize refCount for SnmpFreeContext garbage collection
pCtxt->refCount = 1;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_CONTEXT);
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_CONTEXT)(nContext+1));
ERROR_OUT:
return ((HSNMP_CONTEXT)SaveError (lSession, lError));
} // end_SnmpStrToContext

// SnmpContextToStr
// Revised to allow for zero-length/NULL context...BN 3/12/96
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpContextToStr (IN HSNMP_CONTEXT hContext,
                     OUT smiLPOCTETS string)
{
smiUINT32 len;
smiLPBYTE str;
DWORD nCtx;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPCTXT pCtxt;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nCtx = HandleToUlong(hContext) - 1;
if (!snmpValidTableEntry(&CntxDescr, nCtx))
   {
   lError = SNMPAPI_CONTEXT_INVALID;
   goto ERROR_OUT;
   }
pCtxt = snmpGetTableEntry(&CntxDescr, nCtx);

// save session for possible error return
lSession = pCtxt->Session;
if (IsBadWritePtr(string, sizeof(smiLPOCTETS)))
   {
   lError = string == NULL ? SNMPAPI_NOOP : SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }
switch (TaskData.nTranslateMode)
   {
   case SNMPAPI_TRANSLATED:
   str = pCtxt->name;
   len = lstrlen (str);
// If calling mode is TRANSLATED, and friendly value was stored,
   if (len)
// then we are done here.
      break;
// If calling mode is TRANSLATED, but no value stored,
// then fall through to UNTRANSLATED default...
   case SNMPAPI_UNTRANSLATED_V1:
   case SNMPAPI_UNTRANSLATED_V2:
   str = pCtxt->commStr;
   len = pCtxt->commLen;
   break;

   default:
   lError = SNMPAPI_MODE_INVALID;
   goto ERROR_OUT;
   }
// Setup for possible zero-length/NULL context return
string->ptr = NULL;
// (deliberate assignment in conditional...)
if (string->len = len)
   {
   // App must free following alloc via SnmpFreeDescriptor()
   if (!(string->ptr = (smiLPBYTE)GlobalAlloc (GPTR, len)))
      {
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   CopyMemory (string->ptr, str, len);
   }
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpContextToStr()

// SnmpFreeContext
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpFreeContext (IN HSNMP_CONTEXT hContext)
{
DWORD nCtx;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
LPCTXT pCtxt;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nCtx = HandleToUlong(hContext) - 1;
if (!snmpValidTableEntry(&CntxDescr, nCtx))
   {
   lError = SNMPAPI_CONTEXT_INVALID;
   goto ERROR_OUT;
   }
pCtxt = snmpGetTableEntry(&CntxDescr, nCtx);

EnterCriticalSection (&cs_CONTEXT);
// Decrement refCount (unless already 0 [error])
if (pCtxt->refCount)
   pCtxt->refCount--;
// Now test refCount again
if (pCtxt->refCount == 0)
   snmpFreeTableEntry(&CntxDescr, nCtx);

LeaveCriticalSection (&cs_CONTEXT);
return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
}

// SnmpSetPort
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetPort (IN HSNMP_ENTITY hEntity,
                IN UINT port)
{
DWORD nEntity;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
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
if (pEntity->Agent)
   { // Entity running as agent, cannot change port now
   lError = SNMPAPI_OPERATION_INVALID;
   goto ERROR_PRECHECK;
   }
pEntity->addr.inet.sin_port = htons ((WORD)port);
ERROR_PRECHECK:
LeaveCriticalSection (&cs_ENTITY);
if (lError == SNMPAPI_SUCCESS)
   return (SNMPAPI_SUCCESS);
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpSetPort()
