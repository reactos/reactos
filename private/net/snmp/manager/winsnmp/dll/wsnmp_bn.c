// wsnmp_bn.c
//
// WinSNMP Low-Level SNMP/ASN.1/BER Functions and helpers
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
// 980424 - Received msgLen may be larger than pduLen
//        - ParsePduHdr() and ParseMessage() now accommodate this.
// 980420 - Mods related to ParseCntr64() inspired by
//          MS bug ID 127357 (removal of temp64 variable)
//        - Mod to ParseOID() for MS bug ID 127353
//          (reset os_ptr->ptr to NULL on error)
//
// 970310 - Typographical changes
//
#include "winsnmp.inc"

long FindLenVarBind      (LPVARBIND vb_ptr);
long FindLenVALUE        (smiLPVALUE);
long FindLenOctetString  (smiLPOCTETS os_ptr);
long FindLenOID          (smiLPCOID oid_ptr);
long FindLenUInt         (smiUINT32 value);
long FindLenInt          (smiINT32 value);
long FindLenCntr64       (smiLPCNTR64 value);
long DoLenLen            (smiINT32 len);
void AddLen (smiLPBYTE *tmpPtr, smiINT32 lenlen, smiINT32 data_len);
long AddVarBind (smiLPBYTE *tmpPtr, LPVARBIND vb_ptr);
long AddOctetString (smiLPBYTE *tmpPtr, int type, smiLPOCTETS os_ptr);
long AddOID (smiLPBYTE *tmpPtr, smiLPOID oid_ptr);
long AddUInt (smiLPBYTE *tmpPtr, int type, smiUINT32 value);
long AddInt (smiLPBYTE *tmpPtr, smiINT32 value);
long AddCntr64 (smiLPBYTE *tmpPtr, smiLPCNTR64 value);
void AddNull (smiLPBYTE *tmpPtr, int type);
LPVARBIND ParseVarBind (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen);
BOOL ParseOctetString (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPOCTETS os_ptr);
BOOL ParseOID (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPOID oid_ptr);
BOOL ParseCntr64 (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPCNTR64 cntr64_ptr);
BOOL ParseUInt (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPUINT32 value);
BOOL ParseInt (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPINT value);
BOOL ParseNull (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen);
BOOL ParseSequence (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen);
smiINT32 ParseType (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen);
smiINT32 ParseLength (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen);

void FreeOctetString (smiLPOCTETS os_ptr)
{
if (os_ptr)
   {
   if (os_ptr->ptr)
      GlobalFree (os_ptr->ptr);
   GlobalFree (os_ptr);
   }
return;
}

void FreeVarBindList (LPVARBIND vb_ptr)
{
if (vb_ptr)
   { // NULLs are handled by downstream call
   FreeVarBindList (vb_ptr->next_var);
   FreeVarBind (vb_ptr);
   }
return;
}

void FreeVarBind (LPVARBIND vb_ptr)
{
if (vb_ptr)
   {
   if (vb_ptr->name.ptr)
      GlobalFree (vb_ptr->name.ptr);
   switch (vb_ptr->value.syntax)
      {
      case SNMP_SYNTAX_OID:
      if (vb_ptr->value.value.oid.ptr)
         GlobalFree (vb_ptr->value.value.oid.ptr);
      break;

      case SNMP_SYNTAX_OCTETS:
      case SNMP_SYNTAX_IPADDR:
      case SNMP_SYNTAX_OPAQUE:
      if (vb_ptr->value.value.string.ptr)
         GlobalFree (vb_ptr->value.value.string.ptr);
      break;

      default: // Remaining types do not have 'ptr' members
      break;
      } // end_switch
   GlobalFree (vb_ptr);
   } // end_if (vb_ptr)
return;
} // end_FreeVarBind

void FreeV1Trap (LPV1TRAP v1Trap_ptr)
{
if (v1Trap_ptr)
   {
   if (v1Trap_ptr->enterprise.ptr)
      GlobalFree (v1Trap_ptr->enterprise.ptr);
   if (v1Trap_ptr->agent_addr.ptr)
      GlobalFree (v1Trap_ptr->agent_addr.ptr);
   GlobalFree (v1Trap_ptr);
   }
} // end_FreeV1Trap

void AddLen (smiLPBYTE *tmpPtr, long lenlen, long data_len)
{
long i;
if (lenlen == 1)
   *(*tmpPtr)++ = (smiBYTE)data_len;
else
   {
   *(*tmpPtr)++ = (smiBYTE)(0x80 + lenlen - 1);
   for (i = 1; i < lenlen; i++)
      {
      *(*tmpPtr)++ = (smiBYTE)((data_len >>
         (8 * (lenlen - i - 1))) & 0xFF);
      } // end_for
   } // end_else
return;
} // end_AddLen

long AddVarBind (smiLPBYTE *tmpPtr, LPVARBIND vb_ptr)
{
long lenlen;
if (vb_ptr == NULL)
   return (0);
if ((lenlen = DoLenLen(vb_ptr->data_length)) == -1)
   return (-1);
*(*tmpPtr)++ = SNMP_SYNTAX_SEQUENCE;
AddLen (tmpPtr, lenlen, vb_ptr->data_length);
if (AddOID (tmpPtr, &vb_ptr->name) == -1)
   return (-1);

switch (vb_ptr->value.syntax)
   {
   case SNMP_SYNTAX_CNTR32:
   case SNMP_SYNTAX_GAUGE32:
   case SNMP_SYNTAX_TIMETICKS:
   case SNMP_SYNTAX_UINT32:
   AddUInt (tmpPtr, (int)vb_ptr->value.syntax, vb_ptr->value.value.uNumber);
   break;

   case SNMP_SYNTAX_INT:
   AddInt (tmpPtr, vb_ptr->value.value.sNumber);
   break;

   case SNMP_SYNTAX_OID:
   if (AddOID (tmpPtr, (smiLPOID)&(vb_ptr->value.value.oid)) == -1)
      return (-1);
   break;

   case SNMP_SYNTAX_CNTR64:
   AddCntr64 (tmpPtr, (smiLPCNTR64)&(vb_ptr->value.value.hNumber));
   break;

   case SNMP_SYNTAX_OCTETS:
   case SNMP_SYNTAX_IPADDR:
   case SNMP_SYNTAX_OPAQUE:
   if (AddOctetString (tmpPtr, (int)vb_ptr->value.syntax,
         (smiLPOCTETS)&(vb_ptr->value.value.string)) == -1)
      return -1;
   break;

   case SNMP_SYNTAX_NULL:
   case SNMP_SYNTAX_NOSUCHOBJECT:
   case SNMP_SYNTAX_NOSUCHINSTANCE:
   case SNMP_SYNTAX_ENDOFMIBVIEW:
   AddNull (tmpPtr, (int)vb_ptr->value.syntax);
   break;

   default:
   return (-1);
   } // end_switch
return (AddVarBind (tmpPtr, vb_ptr->next_var));
}

long AddOctetString (smiLPBYTE *tmpPtr, int type, smiLPOCTETS os_ptr)
{
UINT i;
long lenlen;
if ((lenlen = DoLenLen ((long)os_ptr->len)) == -1)
   return (-1);
*(*tmpPtr)++ = (smiBYTE)(0xFF & type);
AddLen (tmpPtr, lenlen, os_ptr->len);
for (i = 0; i < os_ptr->len; i++)
   *(*tmpPtr)++ = os_ptr->ptr[i];
return (0);
}

long AddOID (smiLPBYTE *tmpPtr, smiLPOID oid_ptr)
{
UINT i;
long lenlen = 0;
long encoded_len;
encoded_len = 1; // for first two SID's
for (i = 2; i < oid_ptr->len; i++)
   {
   if (oid_ptr->ptr[i] < 0x80)            // 0 - 0x7F
      encoded_len += 1;
   else if (oid_ptr->ptr[i] < 0x4000)     // 0x80 - 0x3FFF
      encoded_len += 2;
   else if (oid_ptr->ptr[i] < 0x200000)   // 0x4000 - 0x1FFFFF
      encoded_len += 3;
   else if (oid_ptr->ptr[i] < 0x10000000) // 0x200000 - 0xFFFFFFF
      encoded_len += 4;
   else
      encoded_len += 5;
   }
if ((lenlen = DoLenLen (encoded_len)) == -1)
   return (-1);
*(*tmpPtr)++ = (smiBYTE)(0xFF & SNMP_SYNTAX_OID);
AddLen (tmpPtr, lenlen, encoded_len);
if (oid_ptr->len < 2)
   *(*tmpPtr)++ = (smiBYTE)(oid_ptr->ptr[0] * 40);
else
   *(*tmpPtr)++ = (smiBYTE)((oid_ptr->ptr[0] * 40) + oid_ptr->ptr[1]);
for (i = 2; i < oid_ptr->len; i++)
   {
   if (oid_ptr->ptr[i] < 0x80)
      { // 0 - 0x7F
      *(*tmpPtr)++ = (smiBYTE)oid_ptr->ptr[i];
      }
   else if (oid_ptr->ptr[i] < 0x4000)
      { // 0x80 - 0x3FFF
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 7) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)(oid_ptr->ptr[i] & 0x7f);
      }
   else if (oid_ptr->ptr[i] < 0x200000)
      { // 0x4000 - 0x1FFFFF
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 14) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 7) | 0x80);  // set high bit
      *(*tmpPtr)++ = (smiBYTE)(oid_ptr->ptr[i] & 0x7f);
      }
   else if (oid_ptr->ptr[i] < 0x10000000)
      { // 0x200000 - 0xFFFFFFF
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 21) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 14) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 7) | 0x80);  // set high bit
      *(*tmpPtr)++ = (smiBYTE)(oid_ptr->ptr[i] & 0x7f);
      }
   else
      {
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 28) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 21) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 14) | 0x80); // set high bit
      *(*tmpPtr)++ = (smiBYTE)
      (((oid_ptr->ptr[i]) >> 7) | 0x80);  // set high bit
      *(*tmpPtr)++ = (smiBYTE)(oid_ptr->ptr[i] & 0x7f);
      }
   } // end_for
return (0);
} // end_AddOID

long AddUInt (smiLPBYTE *tmpPtr, int type, smiUINT32 value)
{
long i;
long datalen;
long lenlen;
// if high bit one, must use 5 octets (first with 00)
if (((value >> 24) & 0xFF) != 0)
   datalen = 4;
else if (((value >> 16) & 0xFF) != 0)
   datalen = 3;
else if (((value >> 8) & 0xFF) != 0)
   datalen = 2;
else
   datalen = 1;
if (((value >> (8 * (datalen - 1))) & 0x80) != 0)
   datalen++;
lenlen = 1; // < 127 octets
*(*tmpPtr)++ = (smiBYTE)(0xFF & type);
AddLen(tmpPtr, lenlen, datalen);
if (datalen == 5)
   { // gotta put a 00 in first octet
   *(*tmpPtr)++ = (smiBYTE)0;
   for (i = 1; i < datalen; i++)
      {
      *(*tmpPtr)++ = (smiBYTE)(value >>
         (8 * ((datalen - 1) - i) & 0xFF));
      }
   } // end_if
else
   {
   for (i = 0; i < datalen; i++)
      {
      *(*tmpPtr)++ = (smiBYTE)(value >>
         (8 * ((datalen - 1) - i) & 0xFF));
      }
   } // end_else
return (0);
} // end_AddUInt

long AddInt (smiLPBYTE *tmpPtr, smiINT32 value)
{
long i;
long datalen;
long lenlen;
switch ((smiBYTE) ((value >> 24) & 0xFF))
   {
   case 0x00:
   if (((value >> 16) & 0xFF) != 0)
      datalen = 3;
   else if (((value >> 8) & 0xFF) != 0)
      datalen = 2;
   else
      datalen = 1;
   if (((value >> (8 * (datalen - 1))) & 0x80) != 0)
      datalen++;
   break;

   case 0xFF:
   if (((value >> 16) & 0xFF) != 0xFF)
      datalen = 3;
   else if (((value >> 8) & 0xFF) != 0xFF)
      datalen = 2;
   else
      datalen = 1;
   if (((value >> (8 * (datalen - 1))) & 0x80) == 0)
      datalen++;
   break;

   default:
   datalen = 4;
   } // end_switch
lenlen = 1; // < 127 octets
*(*tmpPtr)++ = (smiBYTE)(0xFF & SNMP_SYNTAX_INT);
AddLen(tmpPtr, lenlen, datalen);
for (i = 0; i < datalen; i++)
   {
   *(*tmpPtr)++  = (smiBYTE) (value >>
      (8 * ((datalen - 1) - i) & 0xFF));
   }
return (0);
} // end_AddInt()

long AddCntr64 (smiLPBYTE *tmpPtr, smiLPCNTR64 value)
{
long i;
long datalen;
long lenlen;
datalen = FindLenCntr64(value) - 2;
lenlen = 1; // < 127 octets
*(*tmpPtr)++ = (smiBYTE)(0xFF & SNMP_SYNTAX_CNTR64);
AddLen(tmpPtr, lenlen, datalen);
if (datalen == 9)
   { // gotta put a 00 in first octet
   *(*tmpPtr)++ = (smiBYTE)0;
   datalen--;
   }
for (i = datalen; i > 4; i--)
   {
   *(*tmpPtr)++ = (smiBYTE)(value->hipart >>
      (8 * (i - 5) & 0xFF));
   }
for (; i > 0; i--)
   {
   *(*tmpPtr)++ = (smiBYTE)(value->lopart >>
      (8 * (i - 1) & 0xFF));
   }
return (0);
}

long FindLenVarBind (LPVARBIND vb_ptr)
{
long lenlen;
long tot_so_far;
if (!vb_ptr) return (0);
tot_so_far = FindLenVarBind (vb_ptr->next_var);
if (tot_so_far == -1)
   return (-1);
vb_ptr->data_length = FindLenOID (&vb_ptr->name) +
                      FindLenVALUE (&vb_ptr->value);
if ((lenlen = DoLenLen (vb_ptr->data_length)) == -1)
   return (-1);
return (1 + lenlen + vb_ptr->data_length + tot_so_far);
} // end_FindLenVarBind

long FindLenVALUE (smiLPVALUE value_ptr)
{
if (value_ptr)
   {
   switch (value_ptr->syntax)
      {
      case SNMP_SYNTAX_OCTETS:
      case SNMP_SYNTAX_IPADDR:
      case SNMP_SYNTAX_OPAQUE:
      return (FindLenOctetString (&value_ptr->value.string));

      case SNMP_SYNTAX_OID:
      return (FindLenOID (&value_ptr->value.oid));

      case SNMP_SYNTAX_NULL:
      case SNMP_SYNTAX_NOSUCHOBJECT:
      case SNMP_SYNTAX_NOSUCHINSTANCE:
      case SNMP_SYNTAX_ENDOFMIBVIEW:
      return (2);

      case SNMP_SYNTAX_INT:
      return (FindLenInt (value_ptr->value.sNumber));

      case SNMP_SYNTAX_CNTR32:
      case SNMP_SYNTAX_GAUGE32:
      case SNMP_SYNTAX_TIMETICKS:
      case SNMP_SYNTAX_UINT32:
      return (FindLenUInt (value_ptr->value.uNumber));

      case SNMP_SYNTAX_CNTR64:
      return (FindLenCntr64 (&value_ptr->value.hNumber));
      } // end_switch
   } // end_if
return (-1);
} // end_FindLenVALUE

long FindLenOctetString (smiLPOCTETS os_ptr)
{
long lenlen;
if (!os_ptr) return (-1);
if ((lenlen = DoLenLen (os_ptr->len)) == -1)
   return (-1);
 return (1 + lenlen + os_ptr->len);
}

long FindLenOID (smiLPCOID oid_ptr)
{
long lenlen;
UINT i;
UINT encoded_len;
encoded_len = 1; // for first two Sub-IDs
// beware of i = 2
for (i = 2; i < oid_ptr->len; i++)
   {
   if (oid_ptr->ptr[i] < 0x80)            // 0 - 0x7F
      encoded_len += 1;
   else if (oid_ptr->ptr[i] < 0x4000)     // 0x80 - 0x3FFF
      encoded_len += 2;
   else if (oid_ptr->ptr[i] < 0x200000)   // 0x4000 - 0x1FFFFF
      encoded_len += 3;
   else if (oid_ptr->ptr[i] < 0x10000000) // 0x200000 - 0xFFFFFFF
      encoded_len += 4;
   else
      encoded_len += 5;
   } // end_for
if ((lenlen = DoLenLen (encoded_len)) == -1)
   return (-1);
return (1 + lenlen + encoded_len);
} // end_FindLenOID

long FindLenUInt (smiUINT32 value)
{
long datalen;
// if high bit one, must use 5 octets (first with 00)
if (((value >> 24) & 0xFF) != 0)
   datalen = 4;
else if (((value >> 16) & 0xFF) != 0)
   datalen = 3;
else if (((value >> 8) & 0xFF) != 0)
   datalen = 2;
else
   datalen = 1;
if (((value >> (8 * (datalen - 1))) & 0x80) != 0)
   datalen++;
// length of length  < 127 octets
return (1 + 1 + datalen);
}

long FindLenInt (smiINT32 value)
{
long datalen;
switch ((smiBYTE) ((value >> 24) & 0xFF))
   {
   case 0x00:
   if (((value >> 16) & 0xFF) != 0)
      datalen = 3;
   else if (((value >> 8) & 0xFF) != 0)
      datalen = 2;
   else
      datalen = 1;
   if (((value >> (8 * (datalen - 1))) & 0x80) != 0)
      datalen++;
   break;

   case 0xFF:
   if (((value >> 16) & 0xFF) != 0xFF)
      datalen = 3;
   else if (((value >> 8) & 0xFF) != 0xFF)
      datalen = 2;
   else
      datalen = 1;
   if (((value >> (8 * (datalen - 1))) & 0x80) == 0)
      datalen++;
   break;

   default:
   datalen = 4;
   } // end_switch
return (1 + 1 + datalen);
}

long FindLenCntr64 (smiLPCNTR64 value)
{
long datalen;

// if high bit one, must use 5 octets (first with 00)
if (((value->hipart >> 24) & 0xFF) != 0)
   {
   datalen = 8;
   if (((value->hipart >> 24) & 0x80) != 0) datalen++;
   }
else if (((value->hipart >> 16) & 0xFF) != 0)
   {
   datalen = 7;
   if (((value->hipart >> 16) & 0x80) != 0) datalen++;
   }
else if (((value->hipart >> 8) & 0xFF) != 0)
   {
   datalen = 6;
   if (((value->hipart >> 8) & 0x80) != 0) datalen++;
   }
else if (((value->hipart) & 0xFF) != 0)
   {
   datalen = 5;
   if (((value->hipart) & 0x80) != 0) datalen++;
   }
else if (((value->lopart>> 24) & 0xFF) != 0)
   {
   datalen = 4;
   if (((value->lopart >> 24) & 0x80) != 0) datalen++;
   }
else if (((value->lopart >> 16) & 0xFF) != 0)
   {
   datalen = 3;
   if (((value->lopart >> 16) & 0x80) != 0) datalen++;
   }
else if (((value->lopart >> 8) & 0xFF) != 0)
   {
   datalen = 2;
   if (((value->lopart >> 8) & 0x80) != 0) datalen++;
   }
else
   {
   datalen = 1;
   if (((value->lopart) & 0x80) != 0) datalen++;
   }
// length of length  < 127 octets
return (1 + 1 + datalen);
}

long DoLenLen (long len)
{
// short form?
if (len < 128) return (1);
if (len < 0x100) return (2);
if (len < 0x10000) return (3);
if (len < 0x1000000) return (4);
return (-1);
}

void AddNull (smiLPBYTE *tmpPtr, int type)
{
*(*tmpPtr)++ = (smiBYTE)(0xFF & type);
*(*tmpPtr)++ = 0x00;
return;
}

BOOL BuildMessage (smiUINT32 version, smiLPOCTETS community,
                   LPPDUS pdu, smiINT32 requestId,
                   smiLPBYTE *msgAddr, smiLPUINT32 msgSize)
{
LPVARBIND vbList = NULL;
long nVbDataLen, nVbLenLen, nVbTotalLen;
long nPduDataLen, nPduLenLen, nPduTotalLen;
long nMsgDataLen, nMsgLenLen, nMsgTotalLen;
long nTmpDataLen;
smiLPBYTE tmpPtr = NULL;
*msgAddr = NULL;
*msgSize = 0;
if (pdu == NULL || community == NULL)
   return (FALSE);
// Determine length of VarBind list part
vbList = pdu->VBL_addr;
if (vbList == NULL && pdu->VBL != 0)
   vbList = ((LPVBLS)snmpGetTableEntry(&VBLsDescr, HandleToUlong(pdu->VBL)-1))->vbList;
// vbList == NULL is ok
if ((nVbDataLen = FindLenVarBind (vbList)) == -1)
   return (FALSE);
if ((nVbLenLen = DoLenLen (nVbDataLen)) == -1)
   return (FALSE);
nVbTotalLen = 1 + nVbLenLen + nVbDataLen;
// Determine length of PDU overhead part
switch (pdu->type)
   {
   case SNMP_PDU_GET:
   case SNMP_PDU_GETNEXT:
   case SNMP_PDU_RESPONSE:
   case SNMP_PDU_SET:
   case SNMP_PDU_GETBULK:
   case SNMP_PDU_INFORM:
   case SNMP_PDU_TRAP:
   nPduDataLen = FindLenInt (requestId)
               + FindLenInt (pdu->errStatus)
               + FindLenInt (pdu->errIndex)
               + nVbTotalLen;
   break;

   case SNMP_PDU_V1TRAP:
   if (!pdu->v1Trap)
      return (FALSE);
   nPduDataLen = FindLenInt (pdu->v1Trap->generic_trap)
               + FindLenInt (pdu->v1Trap->specific_trap)
               + FindLenUInt (pdu->v1Trap->time_ticks)
               + nVbTotalLen;
   if ((nTmpDataLen = FindLenOID (&pdu->v1Trap->enterprise)) == -1)
      return (FALSE);
   nPduDataLen += nTmpDataLen;
   if ((nTmpDataLen = FindLenOctetString (&pdu->v1Trap->agent_addr)) == -1)
      return (FALSE);
   nPduDataLen += nTmpDataLen;
   break;

   default:
   return (FALSE);
   } // end_switch
if ((nPduLenLen = DoLenLen(nPduDataLen)) == -1)
   return (FALSE);
nPduTotalLen = 1 + nPduLenLen + nPduDataLen;
nMsgDataLen = FindLenUInt (version)
            + FindLenOctetString (community)
            + nPduTotalLen;
nMsgLenLen = DoLenLen (nMsgDataLen);
nMsgTotalLen = 1 + nMsgLenLen + nMsgDataLen;
// Allocate the necessary memory for the message
tmpPtr = GlobalAlloc (GPTR, nMsgTotalLen);
if (tmpPtr == NULL)
   return (FALSE);
*msgAddr = tmpPtr;
*msgSize = nMsgTotalLen;
// Now plug in the values in the message bytes
*tmpPtr++ = SNMP_SYNTAX_SEQUENCE;
// Wrapper portion
AddLen (&tmpPtr, nMsgLenLen, nMsgDataLen);
AddInt (&tmpPtr, version);
AddOctetString (&tmpPtr, SNMP_SYNTAX_OCTETS, community);
// PDU header portion
// "Downgrade" GetBulk to GetNext if target is SNMPv1
if (pdu->type == SNMP_PDU_GETBULK && version == 0)
   *tmpPtr++ = SNMP_PDU_GETNEXT;
else
   *tmpPtr++ = (BYTE) pdu->type;
AddLen (&tmpPtr, nPduLenLen, nPduDataLen);
switch (pdu->type)
   {
   case SNMP_PDU_GET:
   case SNMP_PDU_GETNEXT:
   case SNMP_PDU_RESPONSE:
   case SNMP_PDU_SET:
   case SNMP_PDU_INFORM:
   case SNMP_PDU_TRAP:
   case SNMP_PDU_GETBULK:
   AddInt (&tmpPtr, requestId);
   AddInt (&tmpPtr, pdu->errStatus);
   AddInt (&tmpPtr, pdu->errIndex);
   break;

   case SNMP_PDU_V1TRAP:
   if (AddOID (&tmpPtr, &pdu->v1Trap->enterprise)== -1)
      goto error_out;
   if (AddOctetString (&tmpPtr, SNMP_SYNTAX_IPADDR, &pdu->v1Trap->agent_addr) == -1)
      goto error_out;
   AddInt (&tmpPtr, pdu->v1Trap->generic_trap);
   AddInt (&tmpPtr, pdu->v1Trap->specific_trap);
   AddUInt (&tmpPtr, SNMP_SYNTAX_TIMETICKS, pdu->v1Trap->time_ticks);
   break;

   default:
   goto error_out;
   } // end_switch
// VarBindList portion
*tmpPtr++ = SNMP_SYNTAX_SEQUENCE;
AddLen (&tmpPtr, nVbLenLen, nVbDataLen);
if (AddVarBind (&tmpPtr, vbList) == -1)
   {
error_out:
   if (*msgAddr)
      GlobalFree (*msgAddr);
   *msgAddr = NULL;
   *msgSize = 0;
   return (FALSE);
   }
// Success
return (TRUE);
} // end_BuildMessage()


BOOL SetPduType (smiLPBYTE msgPtr, smiUINT32 msgLen, int pduType)
{
smiLPBYTE tmpPtr;
smiUINT32 tmp;
if (!(tmpPtr = msgPtr))                    // Deliberate assignment
   return (FALSE);
if (!(ParseSequence (&tmpPtr, &msgLen)))   // sequence
   return (FALSE);
if (!(ParseUInt (&tmpPtr, &msgLen, &tmp))) // version
   return (FALSE);
// Jump over communityString...not needed here
if (ParseType (&tmpPtr, &msgLen) == -1)
   return (FALSE);
if ((tmp = ParseLength (&tmpPtr, &msgLen)) == -1)
   return (FALSE);
if (tmp > msgLen)
   return (FALSE);
tmpPtr += tmp; // Jump!
// Set the PDU type byte
*tmpPtr = (smiBYTE)pduType;
return (TRUE);
} // End_SetPduType()


smiUINT32 ParsePduHdr (smiLPBYTE msgPtr, smiUINT32 msgLen,
                       smiLPUINT32 version, smiLPINT32 type, smiLPUINT32 reqID)
{
// This is a private function (not exported via WinSNMP)
// It is called only once by msgNotify() (another private function)
// to "peek ahead" at certain PDU attributes to determine the next
// procesing steps.
smiUINT32 pduLen;
smiUINT32 length;
long errcode = 1;
if (msgPtr == NULL)
   goto DONE;
errcode++; // 2
// Parse initial Sequence field...
if (ParseType (&msgPtr, &msgLen) != SNMP_SYNTAX_SEQUENCE)
   goto DONE;
errcode++; // 3
// ...to get the remaining pduLen out of it
if ((pduLen = ParseLength (&msgPtr, &msgLen)) == -1)
   goto DONE;
errcode++; // 4
if (pduLen > msgLen)
   goto DONE;
errcode++; // 5
msgLen = pduLen; // Only pduLen counts now
if (!(ParseUInt (&msgPtr, &msgLen, version)))
   goto DONE;
errcode++; // 6
// Jump over communityString...not needed here
if (ParseType (&msgPtr, &msgLen) == -1)
   goto DONE;
errcode++; // 7
if ((length = ParseLength (&msgPtr, &msgLen)) == -1)
   goto DONE;
errcode++; // 8
if (length > msgLen)
   goto DONE;
errcode++; // 9
msgPtr += length; // Jump!
msgLen -= length;
// Get PDU type
if ((*type = ParseType (&msgPtr, &msgLen)) == -1)
   goto DONE;
errcode++; // 10
// Check PDU type for requestID semantics
if (*type == SNMP_PDU_V1TRAP)
   *reqID = 0; // No requestID on v1 trapPDU
else // Not a v1 trapPDU, therefore
   { // must get requestID
   if ((ParseLength (&msgPtr, &msgLen)) == -1)
      goto DONE;
   errcode++; // 11
   if (!(ParseInt (&msgPtr, &msgLen, reqID)))
      goto DONE;
   }
errcode = 0;
DONE:
return (errcode);
} // end_ParsePduHdr

smiUINT32 ParseMessage (smiLPBYTE msgPtr, smiUINT32 msgLen,
                        smiLPUINT32 version, smiLPOCTETS *community, LPPDUS pdu)
{
smiUINT32 pduLen;
smiLPOCTETS os_ptr;
LPVARBIND vb_ptr;
LPVARBIND vb_end_ptr;
long errcode = 1;
if (msgPtr == NULL)
   goto DONE;
errcode++; // 2
// Parse initial Sequence field...
if (ParseType (&msgPtr, &msgLen) != SNMP_SYNTAX_SEQUENCE)
   goto DONE;
errcode++; // 3
// ...to get the remaining pduLen out of it
if ((pduLen = ParseLength (&msgPtr, &msgLen)) == -1)
   goto DONE;
errcode++; // 4
if (pduLen > msgLen)
   goto DONE;
errcode++; // 5
msgLen = pduLen; // Only pduLen counts now
if (!(ParseUInt (&msgPtr, &msgLen, version)))
   goto DONE;
errcode++; // 5
if (*version != 0 && *version != 1) // SNMPv1 or SNMPv2c
   goto DONE;
errcode++; // 6
if (!(os_ptr = GlobalAlloc (GPTR, sizeof(smiOCTETS))))
   goto DONE;
errcode++; // 7
if (!(ParseOctetString (&msgPtr, &msgLen, os_ptr)))
   goto DONE_OS;
errcode++; // 8
if (pdu == NULL)
   goto DONE_OS;
ZeroMemory (pdu, sizeof(PDUS));
if ((pdu->type = ParseType (&msgPtr, &msgLen)) == -1)
   goto DONE_PDU;
errcode++; // 9
if ((ParseLength (&msgPtr, &msgLen)) == -1)
   goto DONE_PDU;
errcode++; // 10
switch (pdu->type)
   {
   case SNMP_PDU_GET:
   case SNMP_PDU_GETNEXT:
   case SNMP_PDU_RESPONSE:
   case SNMP_PDU_SET:
   case SNMP_PDU_GETBULK:
   case SNMP_PDU_INFORM:
   case SNMP_PDU_TRAP:
   if (!(ParseInt (&msgPtr, &msgLen, &pdu->appReqId)))
      goto DONE_PDU;
errcode++; // 11
   if (!(ParseInt (&msgPtr, &msgLen, &pdu->errStatus)))
      goto DONE_PDU;
errcode++; // 12
   if (!(ParseInt (&msgPtr, &msgLen, &pdu->errIndex)))
      goto DONE_PDU;
errcode++; // 13
   break;

   case SNMP_PDU_V1TRAP:
   pdu->v1Trap = GlobalAlloc (GPTR, sizeof(V1TRAP));
   if (pdu->v1Trap == NULL)
      goto DONE_PDU;
errcode++; // 11
   if (!(ParseOID (&msgPtr, &msgLen, &pdu->v1Trap->enterprise)))
      goto DONE_PDU;
errcode++; // 12
   if (!(ParseOctetString (&msgPtr, &msgLen, &pdu->v1Trap->agent_addr)))
      goto DONE_PDU;
errcode++; // 13
   if (!(ParseInt (&msgPtr, &msgLen, &pdu->v1Trap->generic_trap)))
      goto DONE_PDU;
errcode++; // 14
   if (!(ParseInt (&msgPtr, &msgLen, &pdu->v1Trap->specific_trap)))
      goto DONE_PDU;
errcode++; // 15
   if (!(ParseUInt (&msgPtr, &msgLen, &pdu->v1Trap->time_ticks)))
      goto DONE_PDU;
errcode++; // 16
   break;

   default:
   goto DONE_PDU;
   } // end_switch
errcode = 20; // re-normalize
// Waste the SEQUENCE tag
if (!(ParseSequence (&msgPtr, &msgLen)))
      goto DONE_PDU;
errcode++; // 21
// Parse the varbind list
pdu->VBL = 0;
pdu->VBL_addr = NULL;
while (msgLen)
   {
   if (!(vb_ptr = ParseVarBind (&msgPtr, &msgLen)))
      goto DONE_PDU;
errcode++; // 22+
   if (!pdu->VBL_addr)                     // Is this the first one?
      vb_end_ptr = pdu->VBL_addr = vb_ptr; // If so, start a list
   else
      { // tack onto end of list
      vb_end_ptr->next_var = vb_ptr;
      vb_end_ptr = vb_ptr;
      }
   } // end_while
errcode = 0;
*community = os_ptr;
goto DONE;
DONE_PDU:
FreeVarBindList (pdu->VBL_addr); // Checks for NULL
FreeV1Trap (pdu->v1Trap);        // Checks for NULL
ZeroMemory (pdu, sizeof(PDUS));
DONE_OS:
FreeOctetString (os_ptr);
DONE:
return (errcode);
} // end_ParseMessage

LPVARBIND ParseVarBind (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen)
{
LPVARBIND vb_ptr;
if (!(ParseSequence (tmpPtr, tmpLen)))
   return (NULL);
if ((vb_ptr = (LPVARBIND)GlobalAlloc(GPTR, sizeof(VARBIND))) == NULL)
   return (NULL);
if (!(ParseOID(tmpPtr, tmpLen, &vb_ptr->name)))
   goto ERROROUT;
vb_ptr->value.syntax = (smiUINT32)*(*tmpPtr);
switch (vb_ptr->value.syntax)
   {
   case SNMP_SYNTAX_CNTR32:
   case SNMP_SYNTAX_GAUGE32:
   case SNMP_SYNTAX_TIMETICKS:
   case SNMP_SYNTAX_UINT32:
   if (!(ParseUInt (tmpPtr, tmpLen, &vb_ptr->value.value.uNumber)))
      goto ERROROUT;
   break;

   case SNMP_SYNTAX_INT:
   if (!(ParseInt (tmpPtr, tmpLen, &vb_ptr->value.value.sNumber)))
      goto ERROROUT;
   break;

   case SNMP_SYNTAX_OID:
   if (!(ParseOID (tmpPtr, tmpLen, &vb_ptr->value.value.oid)))
      goto ERROROUT;
   break;

   case SNMP_SYNTAX_CNTR64:
   if (!(ParseCntr64 (tmpPtr, tmpLen, &vb_ptr->value.value.hNumber)))
      goto ERROROUT;
   break;

   case SNMP_SYNTAX_OCTETS:
   case SNMP_SYNTAX_IPADDR:
   case SNMP_SYNTAX_OPAQUE:
   if (!(ParseOctetString (tmpPtr, tmpLen, &vb_ptr->value.value.string)))
      goto ERROROUT;
   break;

   case SNMP_SYNTAX_NULL:
   case SNMP_SYNTAX_NOSUCHOBJECT:
   case SNMP_SYNTAX_NOSUCHINSTANCE:
   case SNMP_SYNTAX_ENDOFMIBVIEW:
   if (!(ParseNull (tmpPtr, tmpLen)))
      goto ERROROUT;
   break;

   default:
   goto ERROROUT;
   } // end_switch
return (vb_ptr); // Success
//
ERROROUT:
FreeVarBind(vb_ptr);
return (NULL);   // Failure
} // end_ParseVarBind

BOOL ParseOctetString
      (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPOCTETS os_ptr)
{
if (!os_ptr)
   return (FALSE);
os_ptr->ptr = NULL;
os_ptr->len = 0;
if (ParseType (tmpPtr, tmpLen) == -1)
   return (FALSE);
if ((os_ptr->len = ParseLength (tmpPtr, tmpLen)) == -1)
   return (FALSE);
if (os_ptr->len > *tmpLen)
   return (FALSE);
if (os_ptr->len)
   { // Does not allocate "string" space on "length = 0"
   if (!(os_ptr->ptr = (smiLPBYTE)GlobalAlloc (GPTR, os_ptr->len)))
      return (FALSE);
   CopyMemory (os_ptr->ptr, *tmpPtr, os_ptr->len);
   }
*tmpPtr += os_ptr->len;
*tmpLen -= os_ptr->len;
return (TRUE);
} // end_ParseOctetString

BOOL ParseOID (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPOID oid_ptr)
{
smiINT32 length;
if (!oid_ptr)
   return (FALSE);
oid_ptr->ptr = NULL;
oid_ptr->len = 0;
if (ParseType (tmpPtr, tmpLen) != SNMP_SYNTAX_OID)
   return (FALSE);
length = ParseLength (tmpPtr, tmpLen);
// -1 is error return from ParseLength()
// Valid lengths are 1 thru MAXOBJIDSIZE
if (length <= 0 || length > MAXOBJIDSIZE)
   return (FALSE);
if ((smiUINT32)length > *tmpLen)
   return (FALSE);
// the sub-id array will by 1 longer than the ASN.1/BER array
oid_ptr->ptr = (smiLPUINT32)GlobalAlloc (GPTR, sizeof(smiUINT32) * (length+1));
if (oid_ptr->ptr == NULL)
   return (FALSE);
// oid_ptr structure space is pre-zero'd via GlobalAlloc()
while (length)
   {
   oid_ptr->ptr[oid_ptr->len] =
      (oid_ptr->ptr[oid_ptr->len] << 7) + (*(*tmpPtr) & 0x7F);
   if ((*(*tmpPtr)++ & 0x80) == 0)
      { // on the last octet of this sub-id
      if (oid_ptr->len == 0) // check for first sub-id
         {                   // ASN.1/BER packs two into it
         oid_ptr->ptr[1] = oid_ptr->ptr[0];
         oid_ptr->ptr[0] /= 40;
         if (oid_ptr->ptr[0] > 2)
            oid_ptr->ptr[0] = 2;
         oid_ptr->ptr[1] -= (oid_ptr->ptr[0] * 40);
         oid_ptr->len++; // extra bump
         }
      oid_ptr->len++;
      }
   length--;
   (*tmpLen)--;
   } // end_while (length)
return (TRUE);
} // end_ParseOID

BOOL ParseCntr64 (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPCNTR64 Cntr64_ptr)
{
smiINT32 i;
smiINT32 length;
if (ParseType (tmpPtr, tmpLen) != SNMP_SYNTAX_CNTR64)
   return (FALSE);
if ((length = ParseLength(tmpPtr, tmpLen)) == -1)
   return (FALSE);
if ((smiUINT32)length > *tmpLen || length > 9 ||
   (length == 9 && *(*tmpPtr) != 0x00))
   return (FALSE);
while (length && *(*tmpPtr) == 0x00)
   {            // leading null octet?
   (*tmpPtr)++; // if so, skip it
   length--;    // and don't count it
   (*tmpLen)--;   // Adjust remaining msg length
   }
Cntr64_ptr->hipart = Cntr64_ptr->lopart = 0;
for (i = 0; i < length; i++)
   {
   Cntr64_ptr->hipart = (Cntr64_ptr->hipart << 8) +
                        (Cntr64_ptr->lopart >> 24);
   Cntr64_ptr->lopart = (Cntr64_ptr->lopart << 8) +
                        (smiUINT32) *(*tmpPtr)++;
   }
*tmpLen -= length;
return (TRUE);
} // end_ParseCntr64

BOOL ParseUInt (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPUINT32 value)
{
smiINT32 length;
smiINT32 i;
if (ParseType (tmpPtr, tmpLen) == -1)
   return (FALSE);
if ((length = ParseLength(tmpPtr, tmpLen)) == -1)
   return (FALSE);
if ((smiUINT32)length > *tmpLen)
   return (FALSE);
if ((length > 5) || ((length > 4) && (*(*tmpPtr) != 0x00)))
   return (FALSE);
while (length && *(*tmpPtr) == 0x00)
   {            // leading null octet?
   (*tmpPtr)++; // if so, skip it
   length--;    // and don't count it
   (*tmpLen)--;   // Adjust remaining msg length
   }
*value = 0;
for (i = 0; i < length; i++)
   *value = (*value << 8) + (smiUINT32)*(*tmpPtr)++;
*tmpLen -= length;
return (TRUE);
} // end_ParseUInt()

BOOL ParseInt (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen, smiLPINT value)
{
smiINT32 length;
smiINT32 i;
smiINT32 sign;
if (ParseType (tmpPtr, tmpLen) != SNMP_SYNTAX_INT)
   return (FALSE);
if ((length = ParseLength (tmpPtr, tmpLen)) == -1)
   return (FALSE);
if ((smiUINT32)length > *tmpLen || length > 4)
   return (FALSE);
sign = ((*(*tmpPtr) & 0x80) == 0x00) ? 0x00 : 0xFF;
*value = 0;
for (i = 0; i < length; i++)
   *value = (*value << 8) + (smiUINT32) *(*tmpPtr)++;
// sign-extend upper bits
for (i = length; i < 4; i++)
   *value = *value + (sign << i * 8);
*tmpLen -= length;
return (TRUE);
} // end_ParseInt()

BOOL ParseNull (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen)
{
smiINT32 length;
if (ParseType (tmpPtr, tmpLen) == -1)
   return (FALSE);
length = ParseLength (tmpPtr, tmpLen);
if (length != 0) // NULLs have no length
   return (FALSE);
return (TRUE);
} // end_ParseNull

BOOL ParseSequence (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen)
{
if (ParseType (tmpPtr, tmpLen) != SNMP_SYNTAX_SEQUENCE)
   return (FALSE);
if (ParseLength (tmpPtr, tmpLen) == -1)
   return (FALSE);
return (TRUE);
} // end_ParseSequence

smiINT32 ParseType (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen)
{
// 980421 - BobN
//        - replaced tmpLen logic with working_len logic
//        - working_len is always checked on entry into a
//        - Parse<xxx> function
smiINT32 type;
if (*tmpLen == 0)
   return (-1);
type = *(*tmpPtr)++;
(*tmpLen)--; // Adjust remaining msg length
switch (type)
   {
   case SNMP_SYNTAX_INT:
   case SNMP_SYNTAX_OCTETS:
   case SNMP_SYNTAX_OID:
   case SNMP_SYNTAX_SEQUENCE:
   case SNMP_SYNTAX_IPADDR:
   case SNMP_SYNTAX_CNTR32:
   case SNMP_SYNTAX_GAUGE32:
   case SNMP_SYNTAX_TIMETICKS:
   case SNMP_SYNTAX_OPAQUE:
   case SNMP_SYNTAX_UINT32:
   case SNMP_SYNTAX_CNTR64:
   case SNMP_SYNTAX_NULL:
   case SNMP_SYNTAX_NOSUCHOBJECT:
   case SNMP_SYNTAX_NOSUCHINSTANCE:
   case SNMP_SYNTAX_ENDOFMIBVIEW:
   case SNMP_PDU_GET:
   case SNMP_PDU_GETNEXT:
   case SNMP_PDU_RESPONSE:
   case SNMP_PDU_SET:
   case SNMP_PDU_V1TRAP:
   case SNMP_PDU_GETBULK:
   case SNMP_PDU_INFORM:
   case SNMP_PDU_TRAP:
   break;

   default:
   type = -1;
   break;
   }
return (type);
} // end_ParseType

smiINT32 ParseLength (smiLPBYTE *tmpPtr, smiLPUINT32 tmpLen)
{
// 980421 - BobN
//        - replaced end_ptr logic with tmpLen logic
//        - tmpLen is always checked on entry into a Parse<xxx>
//        - function and is decremented as used therein.
smiINT32 length;
smiINT32 lenlen;
if (*tmpLen == 0)
   return (-1);
length = (smiINT32) *(*tmpPtr)++;
(*tmpLen)--; // Adjust remaining msg length
// Check for short-form value
if (length < 0x80)
   return (length);
// Long form
lenlen = length & 0x7F;
if ((smiUINT32)lenlen > *tmpLen || lenlen > 4 || lenlen < 1)
   return (-1); // Out of bounds
*tmpLen -= lenlen; // Adjust remaining msg length
length = 0;
while (lenlen)
   {
   length = (length << 8) + *(*tmpPtr)++;
   lenlen--;
   }
return (length);
} // end_ParseLength
