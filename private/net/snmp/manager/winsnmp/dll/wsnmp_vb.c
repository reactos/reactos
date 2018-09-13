// wsnmp_vb.c
//
// WinSNMP VarBind Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 980705 - Changed test on return from SnmpMakeVB()
//          in SnmpCreateVbl() to "!= SNMPAPI_SUCCESS".
//
#include "winsnmp.inc"

BOOL IsBadReadSMIValue(smiLPCVALUE value)
{
    if (IsBadReadPtr((LPVOID)value, sizeof(smiLPCVALUE)))
        return TRUE;

    switch(value->syntax)
    {
    case SNMP_SYNTAX_OCTETS:
    case SNMP_SYNTAX_BITS:
    case SNMP_SYNTAX_OPAQUE:
    case SNMP_SYNTAX_IPADDR:
    case SNMP_SYNTAX_NSAPADDR:
        return IsBadReadPtr((LPVOID)(value->value.string.ptr),
                            value->value.string.len);

    case SNMP_SYNTAX_OID:
        return IsBadReadPtr((LPVOID)(value->value.oid.ptr),
                            value->value.oid.len);
    }
    return FALSE;
}

SNMPAPI_STATUS SnmpMakeVB (smiLPCOID name,
                           smiLPCVALUE value,
                           LPVARBIND FAR *pvb)
{
LPVARBIND vb;
if (!(vb =(LPVARBIND)GlobalAlloc (GPTR, sizeof(VARBIND))))
   return (SNMPAPI_ALLOC_ERROR);
vb->next_var = NULL;
vb->name.ptr = NULL;
if (vb->name.len = name->len) // Deliberate assignment in conditional
   {
   if (name->ptr)
      {
      smiUINT32 len = vb->name.len * sizeof(smiUINT32);
      if (vb->name.ptr = (smiLPUINT32)GlobalAlloc (GPTR, len))
         CopyMemory (vb->name.ptr, name->ptr, len);
      }
   }
if (!vb->name.ptr)
   {
   FreeVarBind (vb);
   return (SNMPAPI_OID_INVALID);
   }
//
if (value)
   {
   switch (value->syntax)
      {
      case SNMP_SYNTAX_OCTETS:
//      case SNMP_SYNTAX_BITS:  -- removed per Bob Natale mail from 10/09/98
      case SNMP_SYNTAX_OPAQUE:
      case SNMP_SYNTAX_IPADDR:
      case SNMP_SYNTAX_NSAPADDR:
      vb->value.value.string.ptr = NULL;
      if (vb->value.value.string.len = value->value.string.len)
         { // Deliberate assignment, above and below
         if (!(vb->value.value.string.ptr =
            (smiLPBYTE)GlobalAlloc (GPTR, value->value.string.len)))
            {
            FreeVarBind (vb);
            return (SNMPAPI_ALLOC_ERROR);
            }
         CopyMemory (vb->value.value.string.ptr, value->value.string.ptr,
                      value->value.string.len);
         }
      break;

      case SNMP_SYNTAX_OID:
      vb->value.value.oid.ptr = NULL;
      if (vb->value.value.oid.len = value->value.oid.len)
         { // Deliberate assignment, above and below
         smiUINT32 len = value->value.oid.len * sizeof(smiUINT32);
         if (!(vb->value.value.oid.ptr = (smiLPUINT32)GlobalAlloc (GPTR, len)))
            {
            FreeVarBind (vb);
            return (SNMPAPI_ALLOC_ERROR);
            }
         CopyMemory (vb->value.value.oid.ptr, value->value.oid.ptr, len);
         }
      break;

      case SNMP_SYNTAX_NULL:
      case SNMP_SYNTAX_NOSUCHOBJECT:
      case SNMP_SYNTAX_NOSUCHINSTANCE:
      case SNMP_SYNTAX_ENDOFMIBVIEW:
      break;

      case SNMP_SYNTAX_INT:
      //case SNMP_SYNTAX_INT32: -- it have the same value as above
      vb->value.value.sNumber = value->value.sNumber;
      break;

      case SNMP_SYNTAX_CNTR32:
      case SNMP_SYNTAX_GAUGE32:
      case SNMP_SYNTAX_TIMETICKS:
      case SNMP_SYNTAX_UINT32:
      vb->value.value.uNumber = value->value.uNumber;
      break;

      case SNMP_SYNTAX_CNTR64:
      vb->value.value.hNumber = value->value.hNumber;
      break;

      default:
      // Clean up the allocated VarBind structure
      FreeVarBind (vb);
      return (SNMPAPI_SYNTAX_INVALID);
      } // end_switch
   vb->value.syntax = value->syntax;
   } // end_if
else
   vb->value.syntax = SNMP_SYNTAX_NULL;
//
*pvb = vb;
return (SNMPAPI_SUCCESS);
} //end_SnmpMakeVB

LPVARBIND SnmpCopyVbl (LPVARBIND VarBindFrom)
{
SNMPAPI_STATUS status;
LPVARBIND VarBindTo;
LPVARBIND VarBindToPrev;
LPVARBIND VarBindNewFrom = NULL; // base VB address
DWORD count = 0;
while (VarBindFrom)
   {
   status = SnmpMakeVB (&VarBindFrom->name, &VarBindFrom->value, &VarBindTo);
   if (status != SNMPAPI_SUCCESS)
      { // Next two lines preserve case where count > 0 at this error
      SaveError (0, status);
      break;
      }
   if (!count)
      VarBindNewFrom = VarBindTo;
   else
      VarBindToPrev->next_var = VarBindTo;
   VarBindToPrev = VarBindTo;
   VarBindFrom = VarBindFrom->next_var;
   count++;
   }
return (VarBindNewFrom);
} // end_SnmpCopyVBL

HSNMP_VBL SNMPAPI_CALL
   SnmpCreateVbl  (IN HSNMP_SESSION hSession,
                   IN smiLPCOID name,
                   IN smiLPCVALUE value)
{
DWORD nVbl;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPVARBIND VarBindPtr = NULL;  // must initialize to NULL
LPVBLS pVbl;

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

lSession = hSession; // save it for possible error return

if (name != NULL)
   {
   if (IsBadReadPtr((LPVOID)name, sizeof(smiOID)))
      {
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
       
   if (name->len != 0 &&
       name->ptr != NULL &&
       IsBadReadPtr((LPVOID)name->ptr, name->len))
      {
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_OUT;
      }
   }
if (value != NULL &&
    IsBadReadSMIValue(value))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }

// We have a valid session at this point...
if (name != NULL && name->ptr != NULL)
   {
   lError = SnmpMakeVB (name, value, &VarBindPtr);
   if (lError != SNMPAPI_SUCCESS)
      goto ERROR_OUT;
   else
      VarBindPtr->next_var = NULL;
   }
EnterCriticalSection (&cs_VBL);
lError = snmpAllocTableEntry(&VBLsDescr, &nVbl);
if (lError != SNMPAPI_SUCCESS)
	goto ERROR_PRECHECK;
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

pVbl->Session = hSession;
pVbl->vbList = VarBindPtr;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_VBL);
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_VBL)(nVbl+1));
ERROR_OUT:
FreeVarBind (VarBindPtr); // Hnadles NULL case
return ((HSNMP_VBL)SaveError (lSession, lError));
} // end_SnmpCreateVbl

// SnmpSetVb
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpSetVb(IN HSNMP_VBL hVbl,
             IN smiUINT32 index,
             IN smiLPCOID name,
             IN smiLPCVALUE value)
{
DWORD nVbl = HandleToUlong(hVbl) - 1;
LPVARBIND VarBindList = NULL;
LPVARBIND VarBindPtr  = NULL;
LPVARBIND VarBindPrev = NULL;
SNMPAPI_STATUS lError = 0;
HSNMP_SESSION lSession = 0;
smiUINT32 i;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&VBLsDescr, nVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

// We have a valid session at this point...
lSession = pVbl->Session; // save it for possible error return

i = SnmpCountVbl (hVbl);

// make sure the index has a valid value
if ( index < 0 || index > i)
{
    lError = SNMPAPI_INDEX_INVALID;
    goto ERROR_OUT;
}

// check for valid data in 'name' parameter
if (IsBadReadPtr((LPVOID)name, sizeof(smiOID)))
{
   // if index points to an existent varbind and the
   // name parameter was not provided, take it from the
   // original varbind.
   if (index != 0 && name == NULL)
   {
       smiUINT32 iVar;

       // look for the original varbind
       for (iVar = 1, VarBindPtr = pVbl->vbList;
            iVar < index;
            iVar++, VarBindPtr = VarBindPtr->next_var);

       // make name to point to that varbind name
       name = &(VarBindPtr->name);
   }
   else
   {
       // either adding a value with NULL OID or specifying an
       // invalid value for 'name' is an SNMPAPI_ALLOC_ERROR
       lError = SNMPAPI_ALLOC_ERROR;
       goto ERROR_OUT;
   }
}

// If the index is 0 then a new varbind is to be added to the list.
// If it is non-zero it references a varbind in the list.
// Except we are currently allow index = count+1 to signal add to
// accommodate FTP Software's faulty implementation as used by HP OpenView
if (!index)      // Allow 0 for FTP/HP OpenView mistake
   index = i+1;  // But make it look like the right thing!
       
if (name->len != 0 &&
   name->ptr != NULL &&
   IsBadReadPtr((LPVOID)name->ptr, name->len))
   {
   lError = SNMPAPI_OID_INVALID;
   goto ERROR_OUT;
   }

if (value != NULL &&
    IsBadReadSMIValue(value))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }

lError = SnmpMakeVB (name, value, &VarBindPtr);
if (lError != SNMPAPI_SUCCESS)
   goto ERROR_OUT;
VarBindPrev = VarBindList = pVbl->vbList;
if (index == i+1)
   { // Adding a VarBind
   if (VarBindList)
      {
      while (VarBindList->next_var != NULL)
         VarBindList = VarBindList->next_var;
      VarBindList->next_var = VarBindPtr;
      }
   else
      {
      VarBindList = VarBindPtr;
      pVbl->vbList = VarBindPtr;
      }
   }
else
   { // Updating a VarBind
   for (i = 1; i < index; i++)
      { // Position and prepare
      VarBindPrev = VarBindList;
      VarBindList = VarBindList->next_var;
      } // end_for
   // Replace
   VarBindPtr->next_var = VarBindList->next_var;
   VarBindPrev->next_var = VarBindPtr;
   if (index == 1)
      pVbl->vbList = VarBindPtr;
   FreeVarBind (VarBindList);
  } // end_else
return (index);
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpSetVb

HSNMP_VBL SNMPAPI_CALL
   SnmpDuplicateVbl  (IN HSNMP_SESSION hSession, IN HSNMP_VBL hVbl)
{
DWORD lVbl;
DWORD nVbl;
SNMPAPI_STATUS lError = SNMPAPI_SUCCESS;
HSNMP_SESSION lSession = 0;
LPVBLS pVblOld, pVblNew;

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
lVbl = HandleToUlong(hVbl) - 1;
if (!snmpValidTableEntry(&VBLsDescr, lVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVblOld = snmpGetTableEntry(&VBLsDescr, lVbl);

EnterCriticalSection (&cs_VBL);
lError = snmpAllocTableEntry(&VBLsDescr, &nVbl);
if (lError != SNMPAPI_SUCCESS)
	goto ERROR_PRECHECK;
pVblNew = snmpGetTableEntry(&VBLsDescr, nVbl);

if (pVblOld->vbList)
   { // Deliberate assignment follows
   if (!(pVblNew->vbList = SnmpCopyVbl (pVblOld->vbList)))
      { // Inherit error code from SnmpCopy Vbl
      snmpFreeTableEntry(&VBLsDescr, nVbl);
      lError = SNMPAPI_ALLOC_ERROR;
      goto ERROR_PRECHECK;
      }
   }
pVblNew->Session = hSession;
ERROR_PRECHECK:
LeaveCriticalSection (&cs_VBL);
if (lError == SNMPAPI_SUCCESS)
   return ((HSNMP_VBL)(nVbl+1));
ERROR_OUT:
return ((HSNMP_VBL)SaveError (lSession, lError));
} // end_SnmpDuplicateVbl

// SnmpFreeVbl
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpFreeVbl (IN HSNMP_VBL hVbl)
{
DWORD nVbl = HandleToUlong(hVbl) - 1;
SNMPAPI_STATUS lError = 0;
DWORD i;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&VBLsDescr, nVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

EnterCriticalSection (&cs_PDU);
if (PDUsDescr.Used)
   {
   for (i = 0; i < PDUsDescr.Allocated; i++)
      {
      LPPDUS pPdu = snmpGetTableEntry(&PDUsDescr, i);
      if (pPdu->VBL == hVbl)
         pPdu->VBL = 0;
      }
   }
LeaveCriticalSection (&cs_PDU);
EnterCriticalSection (&cs_VBL);
// Free all substructures
FreeVarBindList (pVbl->vbList);
// Clean VBL List
snmpFreeTableEntry(&VBLsDescr, nVbl);
LeaveCriticalSection (&cs_VBL);
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (0, lError));
} // end_SnmpFreeVbl

// SnmpCountVbl
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpCountVbl (IN HSNMP_VBL hVbl)
{
DWORD nVbl;
smiUINT32 count;
SNMPAPI_STATUS lError;
LPVARBIND VarBindPtr;
HSNMP_SESSION lSession = 0;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
nVbl = HandleToUlong(hVbl) - 1;
if (!snmpValidTableEntry(&VBLsDescr, nVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

count = 0;
VarBindPtr = pVbl->vbList;
lSession = pVbl->Session;
while (VarBindPtr)
   {
   VarBindPtr = VarBindPtr->next_var;
   count++;
   }
if (!count)  // No varbinds
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_OUT;
   }
return (count);
//
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpCountVbl

// SnmpDeleteVb
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpDeleteVb (IN HSNMP_VBL hVbl, IN smiUINT32 index)
{
DWORD nVbl = HandleToUlong(hVbl) - 1;
HSNMP_SESSION hSession;
smiUINT32 status;
smiUINT32 lError = 0;
HSNMP_SESSION lSession = 0;
UINT i= 0;
LPVARBIND VarBindList;
LPVARBIND VarBindPtr;
LPVARBIND VarBindPrev;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&VBLsDescr, nVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);

hSession =  pVbl->Session;
if (!snmpValidTableEntry(&SessDescr, HandleToUlong(hSession)-1))
   {
   lError = SNMPAPI_SESSION_INVALID;
   goto ERROR_OUT;
   }
lSession = hSession; // Load for SaveError() return
status = SnmpCountVbl (hVbl);
if ((!index) || (index > status))
   {
   lError = SNMPAPI_INDEX_INVALID;
   goto ERROR_OUT;
   }
EnterCriticalSection (&cs_VBL);
// Following cannot be NULL due to passing above test
VarBindPtr = VarBindList = pVbl->vbList;
// Deleting a VarBind
for (i = 1; i <= index; i++)
   { // Position
   VarBindPrev = VarBindPtr;
   VarBindPtr  = VarBindList;
   VarBindList = VarBindList->next_var;
   } // end_for
if (index == 1)
   { // Replace
   pVbl->vbList = VarBindList;
   }
else
   { // Skip
   VarBindPrev->next_var = VarBindList;
   } // end_else
FreeVarBind (VarBindPtr);
LeaveCriticalSection (&cs_VBL);
return (SNMPAPI_SUCCESS);
//
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpDeleteVb

// SnmpGetVb
SNMPAPI_STATUS SNMPAPI_CALL
   SnmpGetVb (IN HSNMP_VBL hVbl,
              IN smiUINT32 index,
              OUT smiLPOID name,
              OUT smiLPVALUE value)
{
DWORD nVbl = HandleToUlong(hVbl) - 1;
LPVARBIND VarBindPtr;
SNMPAPI_STATUS lError = 0;
HSNMP_SESSION lSession = 0;
smiUINT32 nLength;
LPVBLS pVbl;

if (TaskData.hTask == 0)
   {
   lError = SNMPAPI_NOT_INITIALIZED;
   goto ERROR_OUT;
   }
if (!snmpValidTableEntry(&VBLsDescr, nVbl))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
pVbl = snmpGetTableEntry(&VBLsDescr, nVbl);
lSession = pVbl->Session;

if (!name && !value)
   {
   lError = SNMPAPI_NOOP;
   goto ERROR_OUT;
   }

// Test for output descriptor address validity
if ((name && IsBadWritePtr (name, sizeof(smiOID))) ||
    (value && IsBadWritePtr (value, sizeof(smiVALUE))))
   {
   lError = SNMPAPI_ALLOC_ERROR;
   goto ERROR_OUT;
   }

nLength = SnmpCountVbl (hVbl);
if ((!index) || (index > nLength))
   {
   lError = SNMPAPI_INDEX_INVALID;
   goto ERROR_OUT;
   }
if (!(VarBindPtr = pVbl->vbList))
   {
   lError = SNMPAPI_VBL_INVALID;
   goto ERROR_OUT;
   }
// SnmpFillOidValue
for (nLength = 1; nLength < index; nLength++)
      VarBindPtr = VarBindPtr->next_var;

if (name != NULL)
{
    ZeroMemory (name, sizeof(smiOID));

    // Copy the name OID
    if ((VarBindPtr->name.len == 0) || (VarBindPtr->name.len > MAXOBJIDSIZE))
       {
       lError = SNMPAPI_OID_INVALID;
       goto ERROR_OUT;
       }
    nLength = VarBindPtr->name.len * sizeof(smiUINT32);
    // App must free following alloc via SnmpFreeDescriptor()
    if (!(name->ptr = (smiLPUINT32)GlobalAlloc (GPTR, nLength)))
       {
       lError = SNMPAPI_ALLOC_ERROR;
       goto ERROR_OUT;
       }
    CopyMemory (name->ptr, VarBindPtr->name.ptr, nLength);
    name->len = VarBindPtr->name.len;
}

if (value != NULL)
{
    // Initialize output structure
    ZeroMemory (value, sizeof(smiVALUE));
    // Copy the VALUE structure
    switch (VarBindPtr->value.syntax)
       {
       case SNMP_SYNTAX_OCTETS:
       case SNMP_SYNTAX_IPADDR:
       case SNMP_SYNTAX_OPAQUE:
       case SNMP_SYNTAX_NSAPADDR:
       // Do copy only if nLength is non-zero
       if (nLength = VarBindPtr->value.value.string.len) // Deliberate assignment
          { // App must free following alloc via SnmpFreeDescriptor()
          if (!(value->value.string.ptr = (smiLPBYTE)GlobalAlloc (GPTR, nLength)))
             {
             lError = SNMPAPI_ALLOC_ERROR;
             goto ERROR_PRECHECK;
             }
          CopyMemory (value->value.string.ptr, VarBindPtr->value.value.string.ptr, nLength);
          value->value.string.len = nLength;
          }
       break;

       case SNMP_SYNTAX_OID:
       nLength = VarBindPtr->value.value.oid.len;
       if (nLength > MAXOBJIDSIZE)
          {
          lError = SNMPAPI_OID_INVALID;
          goto ERROR_PRECHECK;
          }
       if (nLength)
          { // Do copy only if nLength is non-zero
          nLength *= sizeof(smiUINT32);
          // App must free following alloc via SnmpFreeDescriptor()
          if (!(value->value.oid.ptr = (smiLPUINT32)GlobalAlloc (GPTR, nLength)))
             {
             lError = SNMPAPI_ALLOC_ERROR;
             goto ERROR_PRECHECK;
             }
          CopyMemory (value->value.oid.ptr,
                       VarBindPtr->value.value.oid.ptr, nLength);
          value->value.oid.len = VarBindPtr->value.value.oid.len;
          }
       break;

       case SNMP_SYNTAX_NULL:
       case SNMP_SYNTAX_NOSUCHOBJECT:
       case SNMP_SYNTAX_NOSUCHINSTANCE:
       case SNMP_SYNTAX_ENDOFMIBVIEW:
       // Use initialized (NULL) smiVALUE
       break;

       case SNMP_SYNTAX_INT:
       value->value.sNumber = VarBindPtr->value.value.sNumber;
       break;

       case SNMP_SYNTAX_CNTR32:
       case SNMP_SYNTAX_GAUGE32:
       case SNMP_SYNTAX_TIMETICKS:
       case SNMP_SYNTAX_UINT32:
       value->value.uNumber = VarBindPtr->value.value.uNumber;
       break;

       case SNMP_SYNTAX_CNTR64:
       value->value.hNumber = VarBindPtr->value.value.hNumber;
       break;

       default:
       lError = SNMPAPI_SYNTAX_INVALID;
       goto ERROR_PRECHECK;
       } // end_switch
    value->syntax = VarBindPtr->value.syntax;
}
return (SNMPAPI_SUCCESS);
// Post-name allocation failure modes
ERROR_PRECHECK:
if (name && name->ptr)
   {
   GlobalFree (name->ptr);
   ZeroMemory (name, sizeof(smiOID));
   }
if (value && value->value.string.ptr)
   {
   GlobalFree (value->value.string.ptr);
   ZeroMemory (value, sizeof(smiVALUE));
   }
ERROR_OUT:
return (SaveError (lSession, lError));
} // end_SnmpGetVb
