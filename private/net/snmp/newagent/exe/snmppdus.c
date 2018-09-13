/*

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmppdus.c

Abstract:

    Contains routines for manipulating SNMP PDUs.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "snmppdus.h"
#include "snmpmgmt.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define BERERR  ((LONG)-1)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LONG
DoLenLen(
    LONG lLen
    )

/*

Routine Description:

    Calculates number of bytes required to encode length.

Arguments:

    lLen - length of interest.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // determine len length 
    if (0x80 > lLen) return (1);
    if (0x100 > lLen) return (2);
    if (0x10000 > lLen) return (3);
    if (0x1000000 > lLen) return (4);
    
    SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: SVC: length field too large.\n"
        ));

    // failure
    return BERERR; 
}


LONG
FindLenInt(
    AsnInteger32 nValue
    )

/*

Routine Description:

    Calculates length of integer.

Arguments:

    nValue - integer data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // negative?
    if (nValue < 0) {

        // determine length of negative int
        if ((ULONG)0x80 >= -nValue) return (1);
        if ((ULONG)0x8000 >= -nValue) return (2);
        if ((ULONG)0x800000 >= -nValue) return (3);

    } else {

        // determine length of positive int
        if ((ULONG)0x80 > nValue) return (1);
        if ((ULONG)0x8000 > nValue) return (2);
        if ((ULONG)0x800000 > nValue) return (3);
    }    
    
    // default
    return (4);
}


LONG
FindLenIntEx(
    AsnInteger32 nValue
    )

/*

Routine Description:

    Calculates length of integer (including type and lenlen).

Arguments:

    nValue - integer data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // negative?
    if (nValue < 0) {

        // determine length of negative int
        if ((ULONG)0x80 >= -nValue) return (3);
        if ((ULONG)0x8000 >= -nValue) return (4);
        if ((ULONG)0x800000 >= -nValue) return (5);

    } else {

        // determine length of positive int
        if ((ULONG)0x80 > nValue) return (3);
        if ((ULONG)0x8000 > nValue) return (4);
        if ((ULONG)0x800000 > nValue) return (5);
    }    
    
    // default
    return (6);
}


LONG 
FindLenUInt(
    AsnUnsigned32 nValue
    )

/*

Routine Description:

    Calculates encoded length of unsigned integer.

Arguments:

    nValue - integer data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{   
    // determine length of unsigned int
    if ((ULONG)0x80 > nValue) return (1);
    if ((ULONG)0x8000 > nValue) return (2);
    if ((ULONG)0x800000 > nValue) return (3);
    if ((ULONG)0x80000000 > nValue) return (4);

    // default
    return (5);
}



LONG 
FindLenUIntEx(
    AsnUnsigned32 nValue
    )

/*

Routine Description:

    Calculates encoded length of unsigned integer (including type and lenlen).

Arguments:

    nValue - integer data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{   
    // determine length of unsigned int
    if ((ULONG)0x80 > nValue) return (3);
    if ((ULONG)0x8000 > nValue) return (4);
    if ((ULONG)0x800000 > nValue) return (5);
    if ((ULONG)0x80000000 > nValue) return (6);

    // default
    return (7);
}


LONG
FindLenCntr64(
    AsnCounter64 * pCntr64
    )

/*

Routine Description:

    Calculates encoded length of 64-bit counter.

Arguments:

    pCntr64 - counter data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // retrieve 64-bit unsigned value
    ULONGLONG nValue = pCntr64->QuadPart;

    // determine length of unsigned int
    if ((ULONGLONG)0x80 > nValue) return (1);
    if ((ULONGLONG)0x8000 > nValue) return (2);
    if ((ULONGLONG)0x800000 > nValue) return (3);
    if ((ULONGLONG)0x80000000 > nValue) return (4);
    if ((ULONGLONG)0x8000000000 > nValue) return (5);
    if ((ULONGLONG)0x800000000000 > nValue) return (6);
    if ((ULONGLONG)0x80000000000000 > nValue) return (7);
    if ((ULONGLONG)0x8000000000000000 > nValue) return (8);

    // default
    return (9);
}


LONG
FindLenCntr64Ex(
    AsnCounter64 * pCntr64
    )

/*

Routine Description:

    Calculates encoded length of 64-bit counter (including type and lenlen).

Arguments:

    pCntr64 - counter data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // retrieve 64-bit unsigned value
    ULONGLONG nValue = pCntr64->QuadPart;

    // determine length of unsigned int
    if ((ULONGLONG)0x80 > nValue) return (3);
    if ((ULONGLONG)0x8000 > nValue) return (4);
    if ((ULONGLONG)0x800000 > nValue) return (5);
    if ((ULONGLONG)0x80000000 > nValue) return (6);
    if ((ULONGLONG)0x8000000000 > nValue) return (7);
    if ((ULONGLONG)0x800000000000 > nValue) return (8);
    if ((ULONGLONG)0x80000000000000 > nValue) return (9);
    if ((ULONGLONG)0x8000000000000000 > nValue) return (10);

    // default
    return (11);
}


LONG
FindLenOctets(
    AsnOctetString * pOctets
    )

/*

Routine Description:

    Calculates length of octet string.

Arguments:

    pOctets - pointer to octet string.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // return size
    return pOctets->length;
}


LONG
FindLenOctetsEx(
    AsnOctetString * pOctets
    )

/*

Routine Description:

    Calculates length of octet string (including type and lenlen).

Arguments:

    pOctets - pointer to octet string.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG lLenLen;

    // calculate bytes needed to encode 
    lLenLen = DoLenLen(pOctets->length);

    // return total size
    return (lLenLen != BERERR)
                ? (pOctets->length + lLenLen + 1)
                : BERERR
                ; 
}


LONG 
FindLenOid(
    AsnObjectIdentifier * pOid
    )

/*

Routine Description:

    Calculates length of object identifier.

Arguments:

    pOid - pointer object identifier.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lDataLen;

    // first two 
    lDataLen = 1;

    // assume first two oids present
    for (i = 2; i < pOid->idLength; i++) {

        if (0x80 > pOid->ids[i]) {         
            lDataLen += 1;
        } else if (0x4000 > pOid->ids[i]) {   
            lDataLen += 2;
        } else if (0x200000 > pOid->ids[i]) {  
            lDataLen += 3;
        } else if (0x10000000 > pOid->ids[i]) {     
            lDataLen += 4;
        } else {
            lDataLen += 5;
        }
    } 

    // return size
    return (pOid->idLength >= 2) ? lDataLen : BERERR;
} 


LONG 
FindLenOidEx(
    AsnObjectIdentifier * pOid
    )

/*

Routine Description:

    Calculates length of object identifier (including type and lenlen).

Arguments:

    pOid - pointer object identifier.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lLenLen;
    LONG lDataLen;

    // first two 
    lDataLen = 1;

    // assume first two oids present
    for (i = 2; i < pOid->idLength; i++) {

        if (0x80 > pOid->ids[i]) {         
            lDataLen += 1;
        } else if (0x4000 > pOid->ids[i]) {   
            lDataLen += 2;
        } else if (0x200000 > pOid->ids[i]) {  
            lDataLen += 3;
        } else if (0x10000000 > pOid->ids[i]) {     
            lDataLen += 4;
        } else {
            lDataLen += 5;
        }
    } 

    // calculate len length
    lLenLen = DoLenLen(lDataLen);

    // return total size
    return ((lLenLen != BERERR) &&
            (pOid->idLength >= 2))
                ? (lDataLen + lLenLen + 1)
                : BERERR
                ;
} 


LONG 
FindLenAsnAny(
    AsnAny * pAny       
    )

/*

Routine Description:

    Find length of variable binding value.

Arguments:

    pAny - pointer to variable binding value.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // determine syntax
    switch (pAny->asnType) {
    
    case ASN_OCTETSTRING:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:

        return FindLenOctets(&pAny->asnValue.string);

    case ASN_OBJECTIDENTIFIER:
       
        return FindLenOid(&pAny->asnValue.object);
    
    case ASN_NULL:
    case SNMP_EXCEPTION_NOSUCHOBJECT:
    case SNMP_EXCEPTION_NOSUCHINSTANCE:
    case SNMP_EXCEPTION_ENDOFMIBVIEW:

        return (0);
    
    case ASN_INTEGER32:

        return FindLenInt(pAny->asnValue.number);
    
    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
    case ASN_UNSIGNED32:

        return FindLenUInt(pAny->asnValue.unsigned32);

    case ASN_COUNTER64:

        return FindLenCntr64(&pAny->asnValue.counter64);
    } 

    return BERERR;
} 


LONG 
FindLenAsnAnyEx(
    AsnAny * pAny       
    )

/*

Routine Description:

    Find length of variable binding value (including type and lenlen).

Arguments:

    pAny - pointer to variable binding value.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // determine syntax
    switch (pAny->asnType) {
    
    case ASN_OCTETSTRING:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:

        return FindLenOctetsEx(&pAny->asnValue.string);

    case ASN_OBJECTIDENTIFIER:
       
        return FindLenOidEx(&pAny->asnValue.object);
    
    case ASN_NULL:
    case SNMP_EXCEPTION_NOSUCHOBJECT:
    case SNMP_EXCEPTION_NOSUCHINSTANCE:
    case SNMP_EXCEPTION_ENDOFMIBVIEW:

        return (2);
    
    case ASN_INTEGER32:

        return FindLenIntEx(pAny->asnValue.number);
    
    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
    case ASN_UNSIGNED32:

        return FindLenUIntEx(pAny->asnValue.unsigned32);

    case ASN_COUNTER64:

        return FindLenCntr64Ex(&pAny->asnValue.counter64);
    } 

    return BERERR;
} 


LONG 
FindLenVarBind(
    SnmpVarBind * pVb
    )

/*

Routine Description:

    Find length of variable binding.

Arguments:

    pVb - pointer to variable binding.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG lLenLen;
    LONG lOidLen;
    LONG lValueLen;

    // determine length of name
    lOidLen = FindLenOidEx(&pVb->name);
    
    // determine length of value
    lValueLen = FindLenAsnAnyEx(&pVb->value);

    // return total size
    return ((lOidLen != BERERR) &&
            (lValueLen != BERERR)) 
                ? (lOidLen + lValueLen)
                : BERERR
                ;    
} 


LONG 
FindLenVarBindEx(
    SnmpVarBind * pVb
    )

/*

Routine Description:

    Find length of variable binding (including type and lenlen).

Arguments:

    pVb - pointer to variable binding.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG lLenLen;
    LONG lOidLen;
    LONG lValueLen;

    // determine length of name
    lOidLen = FindLenOidEx(&pVb->name);
    
    // determine length of value
    lValueLen = FindLenAsnAnyEx(&pVb->value);

    // determine length of varbind length
    lLenLen = DoLenLen(lOidLen + lValueLen);

    // return total size
    return ((lLenLen != BERERR) &&
            (lOidLen != BERERR) &&
            (lValueLen != BERERR)) 
                ? (lOidLen + lValueLen + lLenLen + 1)
                : BERERR
                ;    
} 


LONG 
FindLenVarBindList(
    SnmpVarBindList * pVbl
    )

/*

Routine Description:

    Find length of variable binding list.

Arguments:

    pVbl - pointer to variable binding list.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lVbLen = 0;
    LONG lVblLen = 0;

    // process each variable binding in the list
    for (i = 0; (lVbLen != BERERR) && (i < pVbl->len); i++) {

        // determine length of variable binding
        lVbLen = FindLenVarBindEx(&pVbl->list[i]);

        // add to total
        lVblLen += lVbLen;
    }

    // return total size
    return (lVbLen != BERERR) 
                ? lVblLen 
                : BERERR
                ;
}


LONG 
FindLenVarBindListEx(
    SnmpVarBindList * pVbl
    )

/*

Routine Description:

    Find length of variable binding list (including type and lenlen).

Arguments:

    pVbl - pointer to variable binding list.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lVbLen = 0;
    LONG lVblLen = 0;
    LONG lLenLen;

    // process each variable binding in the list
    for (i = 0; (lVbLen != BERERR) && (i < pVbl->len); i++) {

        // determine length of variable binding
        lVbLen = FindLenVarBindEx(&pVbl->list[i]);

        // add to total
        lVblLen += lVbLen;
    }

    // determine list length 
    lLenLen = DoLenLen(lVblLen);

    // return total size
    return ((lVbLen != BERERR) &&
            (lLenLen != BERERR))
                ? (lVblLen + lLenLen + 1)
                : BERERR
                ;
}


VOID 
AddNull(
    LPBYTE * ppByte, 
    INT      nType
    )

/*

Routine Description:

    Adds null into stream.

Arguments:

    ppByte - pointer to pointer to current stream.

    nType - exact syntax.

Return Values:

    None.

*/

{
    // encode actual syntax 
    *(*ppByte)++ = (BYTE)(0xFF & nType);
    *(*ppByte)++ = 0x00;
}


VOID
AddLen(
    LPBYTE * ppByte, 
    LONG     lLenLen, 
    LONG     lDataLen
    )

/*

Routine Description:

    Adds data length field to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    lLenLen - length of data length.

    lDataLen - actual data length.

Return Values:

    None.

*/

{
    LONG i;
    if (lLenLen == 1) {
        *(*ppByte)++ = (BYTE)lDataLen;
    } else {
        *(*ppByte)++ = (BYTE)(0x80 + lLenLen - 1);
        for (i = 1; i < lLenLen; i++) {
            *(*ppByte)++ = (BYTE)((lDataLen >>
                (8 * (lLenLen - i - 1))) & 0xFF);
        } 
    } 
} 


LONG
AddInt(
    LPBYTE *     ppByte, 
    INT          nType, 
    AsnInteger32 nInteger32
    )

/*

Routine Description:

    Adds integer to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    nType - exact syntax of integer.

    nInteger32 - actual data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;
    LONG lLenLen;

    // determine length of integer
    lDataLen = FindLenInt(nInteger32);

    // lenlen
    lLenLen = 1;  

    // encode nType of integer
    *(*ppByte)++ = (BYTE)(0xFF & nType);

    // encode length of integer
    AddLen(ppByte, lLenLen, lDataLen);

    // add encoded integer
    for (i = 0; i < lDataLen; i++) {
       *(*ppByte)++ = (BYTE)(nInteger32 >> 
            (8 * ((lDataLen - 1) - i) & 0xFF));
    }

    return (0);
}


LONG 
AddUInt(
    LPBYTE *      ppByte, 
    INT           nType, 
    AsnUnsigned32 nUnsigned32
    )

/*

Routine Description:

    Adds unsigned integer to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    nType - exact syntax of integer.

    nUnsigned32 - actual data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;
    LONG lLenLen;

    // determine length of integer
    lDataLen = FindLenUInt(nUnsigned32);

    // < 127 octets 
    lLenLen = 1; 

    // encode actual syntax
    *(*ppByte)++ = (BYTE)(0xFF & nType);
    
    // encode data length
    AddLen(ppByte, lLenLen, lDataLen);

    // analyze length
    if (lDataLen == 5) {

        // put 00 in first octet 
        *(*ppByte)++ = (BYTE)0;

        // encode unsigned integer
        for (i = 1; i < lDataLen; i++) {
            *(*ppByte)++ = (BYTE)(nUnsigned32 >>
                (8 * ((lDataLen - 1) - i) & 0xFF));
        }
    
    } else {

        // encode unsigned integer
        for (i = 0; i < lDataLen; i++) {
            *(*ppByte)++ = (BYTE)(nUnsigned32 >>
                (8 * ((lDataLen - 1) - i) & 0xFF));
        }
    } 

    return (0);
}


LONG 
AddCntr64(
    LPBYTE *       ppByte, 
    INT            nType, 
    AsnCounter64 * pCntr64
    )

/*

Routine Description:

    Adds 64-bit counter to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    nType - exact syntax of counter.

    pCntr64 - actual data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;
    LONG lLenLen;

    // determine length of counter64
    lDataLen = FindLenCntr64(pCntr64);

    // < 127 octets 
    lLenLen = 1; 

    // encode actual syntax        
    *(*ppByte)++ = (BYTE)(0xFF & nType);

    // encode data length
    AddLen(ppByte, lLenLen, lDataLen);

    // adjust lDataLen
    if (lDataLen == 9) {
        // put 00 in first octet 
        *(*ppByte)++ = (BYTE)0;
        lDataLen--;
    }

    // encode counter data
    for (i = lDataLen; i > 4; i--) {
        *(*ppByte)++ = (BYTE)(pCntr64->HighPart >>
            (8 * (i - 5) & 0xFF));
    }
    for (; i > 0; i--) {
        *(*ppByte)++ = (BYTE)(pCntr64->LowPart >>
            (8 * (i - 1) & 0xFF));
    }

    return (0);
}


LONG 
AddOctets(
    LPBYTE *         ppByte, 
    INT              nType, 
    AsnOctetString * pOctets
    )

/*

Routine Description:

    Adds octet string to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    nType - exact syntax of string.

    pOctets - actual data.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lLenLen;
    LONG lDataLen;

    // determine oid length
    if ((lDataLen = FindLenOctets(pOctets)) == BERERR)
        return BERERR;

    // calculate octet string length
    if ((lLenLen = DoLenLen(lDataLen)) == BERERR)
        return BERERR;

    // encode actual syntax 
    *(*ppByte)++ = (BYTE)(0xFF & nType);

    // encode octet string length
    AddLen(ppByte, lLenLen, lDataLen);

	// usless copy avoided
	if (*ppByte != pOctets->stream)
	{
		// encode actual octets    
		for (i = 0; i < pOctets->length; i++)
			*(*ppByte)++ = pOctets->stream[i];
	}
	else
	{
		(*ppByte) += pOctets->length;
	}

    return (0);
}


LONG 
AddOid(
    LPBYTE * ppByte, 
    INT      nType, 
    AsnObjectIdentifier * pOid
    )

/*

Routine Description:

    Adds object identifier to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    nType - exact syntax of object identifier.

    pOid - pointer to object identifier.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;
    LONG lLenLen = 0;
    LONG lDataLen;

    // determine oid length
    if ((lDataLen = FindLenOid(pOid)) == BERERR)
        return BERERR;

    // calculate number of bytes required for length
    if ((lLenLen = DoLenLen(lDataLen)) == BERERR)
        return BERERR;

    // add syntax to stream
    *(*ppByte)++ = (BYTE)(0xFF & nType);

    // add object identifier length
    AddLen(ppByte, lLenLen, lDataLen);

    // add first subid
    if (pOid->idLength < 2)
       *(*ppByte)++ = (BYTE)(pOid->ids[0] * 40);
    else
       *(*ppByte)++ = (BYTE)((pOid->ids[0] * 40) + pOid->ids[1]);

    // walk remaining subidentifiers
    for (i = 2; i < pOid->idLength; i++) {

        if (pOid->ids[i] < 0x80) {

            // 0 - 0x7f 
            *(*ppByte)++ = (BYTE)pOid->ids[i];

        } else if (pOid->ids[i] < 0x4000) {

            // 0x80 - 0x3fff 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 7) | 0x80);  // set high bit 
            *(*ppByte)++ = (BYTE)(pOid->ids[i] & 0x7f);

        } else if (pOid->ids[i] < 0x200000) {
   
            // 0x4000 - 0x1FFFFF 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 14) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 7) | 0x80);  // set high bit 
            *(*ppByte)++ = (BYTE)(pOid->ids[i] & 0x7f);
      
        } else if (pOid->ids[i] < 0x10000000) {
      
            // 0x200000 - 0xFFfffff 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 21) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 14) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 7) | 0x80);  // set high bit 
            *(*ppByte)++ = (BYTE)(pOid->ids[i] & 0x7f);

        } else {
      
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 28) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 21) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 14) | 0x80); // set high bit 
            *(*ppByte)++ = (BYTE)
            (((pOid->ids[i]) >> 7) | 0x80);  // set high bit 
            *(*ppByte)++ = (BYTE)(pOid->ids[i] & 0x7f);
        }
    } 

    return (0);
}


LONG 
AddAsnAny(
    LPBYTE * ppByte, 
    AsnAny * pAny
    )

/*

Routine Description:

    Adds variable binding value to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pAny - variable binding value.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    // determine syntax        
    switch (pAny->asnType) {

    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
    case ASN_UNSIGNED32:

        return AddUInt(
                ppByte, 
                (INT)pAny->asnType, 
                pAny->asnValue.unsigned32
                );

    case ASN_INTEGER32:

        return AddInt(
                ppByte, 
                (INT)pAny->asnType, 
                pAny->asnValue.number
                );

    case ASN_OBJECTIDENTIFIER:

        return AddOid(
                ppByte, 
                (INT)pAny->asnType,
                &pAny->asnValue.object
                );

    case ASN_COUNTER64:

        return AddCntr64(
                ppByte, 
                (INT)pAny->asnType,
                &pAny->asnValue.counter64
                );

    case ASN_OCTETSTRING:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:

        return AddOctets(
                ppByte, 
                (INT)pAny->asnType,
                &pAny->asnValue.string
                );

    case ASN_NULL:
    case SNMP_EXCEPTION_NOSUCHOBJECT:
    case SNMP_EXCEPTION_NOSUCHINSTANCE:
    case SNMP_EXCEPTION_ENDOFMIBVIEW:

        AddNull(ppByte, (INT)pAny->asnType);
        return (0);
    }
    
    return BERERR;
}


LONG 
AddVarBind(
    LPBYTE *      ppByte, 
    SnmpVarBind * pVb 
    )

/*

Routine Description:

    Adds variable binding to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pVb - pointer to variable binding.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG lLenLen;
    LONG lDataLen;

    // determine actual length of varbind data
    if ((lDataLen = FindLenVarBind(pVb)) == BERERR)
       return BERERR;

    // determine length of varbind data length
    if ((lLenLen = DoLenLen(lDataLen)) == BERERR)
       return BERERR;

    // encode as sequence
    *(*ppByte)++ = ASN_SEQUENCE;

    // encode data length    
    AddLen(ppByte, lLenLen, lDataLen);

    // encode variable binding name
    if (AddOid(ppByte, ASN_OBJECTIDENTIFIER, &pVb->name) == BERERR)
        return BERERR;

    // encode variable binding value
    if (AddAsnAny(ppByte, &pVb->value) == BERERR)
        return BERERR;

    return (0);
}


LONG 
AddVarBindList(
    LPBYTE *          ppByte, 
    SnmpVarBindList * pVbl
    )

/*

Routine Description:

    Adds variable binding list to current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pVbl - pointer to variable binding list.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    UINT i;

    // add each variable binding 
    for (i = 0; i < pVbl->len; i++) {
        if (AddVarBind(ppByte, &pVbl->list[i]) == BERERR)
            return BERERR;
    }

    return (0);
}


LONG
ParseLength(
    LPBYTE * ppByte, 
    LPBYTE   pLastByte
    )

/*

Routine Description:

    Parse length from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    LONG i;
    LONG lLenLen;
    LONG lDataLen;

    lDataLen = (LONG)*(*ppByte)++;

    if (lDataLen < 0x80)
       return (lDataLen);

    // check for long form
    lLenLen = lDataLen & 0x7f;

    // validate long form 
    if ((lLenLen > 4) || (lLenLen < 1)) 
       return BERERR;

    lDataLen = 0L;

    for (i = 0; i < lLenLen; i++) {
       lDataLen = (lDataLen << 8) + *(*ppByte)++;
    }

    if (*ppByte > pLastByte)
       return BERERR;

    return (lDataLen);
}


LONG 
ParseType(
    LPBYTE * ppByte, 
    LPBYTE   pLastByte
    )

/*

Routine Description:

    Parse type from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

Return Values:

    Returns BERERR if unsuccessful.

*/

{
    SHORT nType = *(*ppByte)++;

    if (*ppByte > pLastByte)
       return BERERR;

    switch (nType) {

    case ASN_INTEGER32:
    case ASN_OCTETSTRING:
    case ASN_OBJECTIDENTIFIER:
    case ASN_SEQUENCE:
    case ASN_IPADDRESS:
    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
    case ASN_OPAQUE:
    case ASN_UNSIGNED32:
    case ASN_COUNTER64:
    case ASN_NULL:
    case SNMP_EXCEPTION_NOSUCHOBJECT:
    case SNMP_EXCEPTION_NOSUCHINSTANCE:
    case SNMP_EXCEPTION_ENDOFMIBVIEW:
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
        nType = BERERR;
        break;
    }
    
    return (LONG)(SHORT)(nType);
} 


BOOL 
ParseNull(
    LPBYTE * ppByte, 
    LPBYTE   pLastByte
    )

/*

Routine Description:

    Parse null from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG lDataLen;

    if (!(ParseType(ppByte, pLastByte)))
        return (FALSE);
    
    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
        return (FALSE);
    
    if (lDataLen != 0)
        return (FALSE);
    
    if (*ppByte > pLastByte)
        return (FALSE);
    
    return (TRUE);
} 


BOOL 
ParseSequence(
    LPBYTE * ppByte, 
    LPBYTE   pLastByte, 
    LONG *   plDataLen
    )

/*

Routine Description:

    Parse sequence from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    plDataLen - pointer to receive sequence length.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG lDataLen;

    if ((ParseType(ppByte, pLastByte)) != ASN_SEQUENCE)
        return (FALSE);

    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
        return (FALSE);

    if (*ppByte > pLastByte)
        return (FALSE);

    if (plDataLen)
        *plDataLen = lDataLen;

    return (TRUE);
} 


BOOL 
ParseInt(
    LPBYTE *       ppByte, 
    LPBYTE         pLastByte, 
    AsnInteger32 * pInteger32
    )

/*

Routine Description:

    Parse integer from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pInteger32 - pointer to receive integer.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG i;
    LONG lSign;
    LONG lDataLen;

    if (ParseType(ppByte, pLastByte) == BERERR)
       return (FALSE);

    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
       return (FALSE);

    if (lDataLen > 4)
       return (FALSE);

    lSign = ((*(*ppByte) & 0x80) == 0x00) ? 0x00 : 0xFF;

    *pInteger32 = 0;

    for (i = 0; i < lDataLen; i++)
       *pInteger32 = (*pInteger32 << 8) + (UINT)*(*ppByte)++;

    // sign-extend upper bits
    for (i = lDataLen; i < 4; i++)
       *pInteger32 = *pInteger32 + (lSign << i * 8);

    if (*ppByte > pLastByte)
       return (FALSE);

    return (TRUE);
}


BOOL 
ParseUInt(
    LPBYTE *        ppByte, 
    LPBYTE          pLastByte, 
    AsnUnsigned32 * pUnsigned32
    )

/*

Routine Description:

    Parse unsigned integer from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pUnsigned32 - pointer to receive integer.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;

    if (ParseType(ppByte, pLastByte) == BERERR)
       return (FALSE);

    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
       return (FALSE);

    if ((lDataLen > 5) || ((lDataLen > 4) && (*(*ppByte) != 0x00)))
       return (FALSE);

    // leading null octet?
    if (*(*ppByte) == 0x00)  {
       (*ppByte)++;          // if so, skip it
       lDataLen--;           // and don't count it
    }

    *pUnsigned32 = 0;

    for (i = 0; i < lDataLen; i++)
       *pUnsigned32 = (*pUnsigned32 << 8) + (UINT)*(*ppByte)++;

    if (*ppByte > pLastByte)
       return (FALSE);

    return (TRUE);
} 


BOOL
ParseCntr64(
    LPBYTE *       ppByte, 
    LPBYTE         pLastByte,
    AsnCounter64 * pCntr64
    )

/*

Routine Description:

    Parse 64-bit counter from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pCntr64 - pointer to receive counter.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;
    LONG nType;

    // initialize
    pCntr64->HighPart = 0L;
    pCntr64->LowPart = 0L;

    if ((nType = ParseType(ppByte, pLastByte)) == BERERR)
        return (FALSE);

    if (nType != ASN_COUNTER64)
        return (FALSE);

    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
        return (FALSE);

    if ((lDataLen > 9) || ((lDataLen > 8) && (*(*ppByte) != 0x00)))
        return (FALSE);

    // leading null octet?
    if (*(*ppByte) == 0x00) { 
       (*ppByte)++;          // if so, skip it
       lDataLen--;           // and don't count it
    }

    for (i = 0; i < lDataLen; i++) {
        pCntr64->HighPart = (pCntr64->HighPart << 8) +
            (pCntr64->LowPart >> 24);
        pCntr64->LowPart = (pCntr64->LowPart << 8) +
            (unsigned long) *(*ppByte)++;
    }

    if (*ppByte > pLastByte) 
       return (FALSE);

    return TRUE;
} 


BOOL
ParseOctets(
    LPBYTE *         ppByte, 
    LPBYTE           pLastByte, 
    AsnOctetString * pOctets
    )

/*

Routine Description:

    Parse octet string from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pOctets - pointer to receive string.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
	LONG errCode;
    // initialize
    pOctets->length  = 0;
    pOctets->stream  = NULL;
    pOctets->dynamic = FALSE;

    if (ParseType(ppByte, pLastByte) == BERERR)
        return (FALSE);

	// make sure no conversion to UINT is done before testing
	// (pOctets->length is UINT)
    if ((errCode = ParseLength(ppByte, pLastByte)) == BERERR)
        return (FALSE);

	pOctets->length = (UINT)errCode;

	// avoid wrapping around on machines with 4-byte pointers
    if ((((*ppByte) + pOctets->length) < *ppByte) || 
		((*ppByte) + pOctets->length) > pLastByte)
        return (FALSE);

    // validate length
    if (pOctets->length) {

        // point into buffer
        pOctets->stream = *ppByte;  // WARNING! WARNING! 
    }

    *ppByte += pOctets->length;

    return (TRUE);
} 


BOOL 
ParseOid(
    LPBYTE *              ppByte, 
    LPBYTE                pLastByte, 
    AsnObjectIdentifier * pOid
    )

/*

Routine Description:

    Parse object identifier from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pOid - pointer to receive oid.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    LONG i;
    LONG lDataLen;
    LONG nType;

    // initialize
    pOid->idLength = 0;
    pOid->ids = NULL;

    if ((nType = ParseType(ppByte, pLastByte)) == BERERR)
        return (FALSE);

    if (nType != ASN_OBJECTIDENTIFIER)
        return (FALSE);

    if ((lDataLen = ParseLength(ppByte, pLastByte)) == BERERR)
        return (FALSE);

    if (lDataLen == 0 ) //--ft 03/02/98 removed trailing "|| lDataLen > SNMP_MAX_OID_LEN)"
        return (FALSE);	// check is done in the for loop below

    pOid->ids = SnmpUtilMemAlloc((DWORD)((lDataLen + 2) * sizeof(UINT)));

    if (pOid->ids == NULL)
        return (FALSE);

    pOid->ids[0] = (UINT)(*(*ppByte) / 40);
    pOid->ids[1] = (UINT)(*(*ppByte)++ - (pOid->ids[0] * 40));
    pOid->idLength = 2;
    pOid->ids[2] = 0;

    for (i = 0; i < lDataLen - 1 && pOid->idLength < SNMP_MAX_OID_LEN; i++) {

        pOid->ids[pOid->idLength] =
            (pOid->ids[pOid->idLength] << 7) | (*(*ppByte) & 0x7F);

        // is last portion of sub-id?
        if ((*(*ppByte)++ & 0x80) == 0) {

            // increment count
            pOid->idLength++;
        }
    }

	if (i < lDataLen - 1 || *ppByte > pLastByte)
	{
		SnmpUtilMemFree(pOid->ids);
		return (FALSE);
	}

    return (TRUE);
} 


BOOL
ParseAsnAny(
    LPBYTE * ppByte, 
    LPBYTE   pLastByte, 
    AsnAny * pAny
    )

/*

Routine Description:

    Parse variable binding value from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pAny - pointer to variable binding value.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    // determine asn type
    switch (pAny->asnType) {
   
    case ASN_COUNTER32:
    case ASN_GAUGE32:
    case ASN_TIMETICKS:
    case ASN_UNSIGNED32:

        return ParseUInt(
                ppByte, 
                pLastByte, 
                &pAny->asnValue.unsigned32
                );

    case ASN_INTEGER32:

        return ParseInt(
                ppByte, 
                pLastByte, 
                &pAny->asnValue.number
                );

    case ASN_OBJECTIDENTIFIER:

        return ParseOid(
                ppByte, 
                pLastByte, 
                &pAny->asnValue.object
                );

    case ASN_COUNTER64:

        return ParseCntr64(
                ppByte, 
                pLastByte,
                &pAny->asnValue.counter64
                );

    case ASN_OCTETSTRING:
    case ASN_IPADDRESS:
    case ASN_OPAQUE:

        return ParseOctets(
                ppByte, 
                pLastByte, 
                &pAny->asnValue.string
                );

    case ASN_NULL:
    case SNMP_EXCEPTION_NOSUCHOBJECT:
    case SNMP_EXCEPTION_NOSUCHINSTANCE:
    case SNMP_EXCEPTION_ENDOFMIBVIEW:

        return ParseNull(ppByte, pLastByte);
    } 

    return (FALSE);
}


BOOL
ParseVarBind(
    LPBYTE *      ppByte, 
    LPBYTE        pLastByte,
    SnmpVarBind * pVb
    )

/*

Routine Description:

    Parse variable binding from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pVb - pointer to variable binding.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    if (!(ParseSequence(ppByte, pLastByte, NULL)))
        return (FALSE);

    if (!(ParseOid(ppByte, pLastByte, &pVb->name)))
        return (FALSE);

    pVb->value.asnType = (UINT)*(*ppByte);

    if (!(ParseAsnAny(ppByte, pLastByte, &pVb->value)))
        return (FALSE);

    return TRUE;    
}


BOOL
ParseVarBindList(
    LPBYTE *          ppByte, 
    LPBYTE            pLastByte,
    SnmpVarBindList * pVbl
    )

/*

Routine Description:

    Parse variable binding from current stream.

Arguments:

    ppByte - pointer to pointer to current stream.
     
    pLastByte - pointer to end of current stream.

    pVbl - pointer to variable binding list.

Return Values:

    Returns FALSE if unsuccessful.

*/

{
    SnmpVarBind Vb;
    SnmpVarBind * pVb = NULL;

    // initialize
    pVbl->list = NULL;
    pVbl->len = 0;

    // loop while data is left
    while (*ppByte < pLastByte) {
        
        if (!(ParseVarBind(ppByte, pLastByte, &Vb)))
            return (FALSE);

        // copy pointer
        pVb = pVbl->list;

        // attempt to allocate new variable binding
        pVb = SnmpUtilMemReAlloc(pVb, (pVbl->len + 1) * sizeof(SnmpVarBind));

        // validate
        if (pVb == NULL) 
            return FALSE;

        // update varbind
        pVb[pVbl->len] = Vb;

        // update list
        pVbl->list = pVb;
        pVbl->len++;            
    }
    
    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures (based on snmp\manager\winsnmp\dll\wsnmp_bn.c)          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
BuildMessage(
    AsnInteger32      nVersion,
    AsnOctetString *  pCommunity,
    PSNMP_PDU         pPdu,
    PBYTE             pMessage,
    PDWORD            pMessageSize
    )

/*

Routine Description:

    Builds outgoing SNMP PDU based on structure.

Arguments:

    nVersion - SNMP version.

    pCommunity - pointer to community string.

    pPdu - pointer to PDU data structure.

    pMessage - pointer to buffer in which to build message.

    pMessageSize - pointer to receive size of message.

Return Values:

    Returns true if successful.

*/

{
    LONG nVbDataLength;
    LONG nVbLenLength; 
    LONG nVbTotalLength;
    LONG nPduDataLength; 
    LONG nPduLenLength;
    LONG nPduTotalLength;
    LONG nMsgDataLength; 
    LONG nMsgLenLength;
    LONG nMsgTotalLength;    
    LONG nMsgAvailLength;
    LONG nTmpDataLength;

    LPBYTE tmpPtr = pMessage;

    // determine bytes available
    nMsgAvailLength = *pMessageSize;

    // find length of variable bindings list
    if ((nVbDataLength = FindLenVarBindList(&pPdu->Vbl)) == BERERR)
        return FALSE; 

    // find length of length of variable bindings    
    if ((nVbLenLength = DoLenLen(nVbDataLength)) == BERERR)
        return FALSE; 

    // calculate total bytes required to encode varbinds
    nVbTotalLength = 1 + nVbLenLength + nVbDataLength;

    // determine pdu nType
    switch (pPdu->nType) {

    case SNMP_PDU_GET:
    case SNMP_PDU_GETNEXT:
    case SNMP_PDU_RESPONSE:
    case SNMP_PDU_SET:
    case SNMP_PDU_GETBULK:
    case SNMP_PDU_INFORM:
    case SNMP_PDU_TRAP:

        // calculate bytes required to encode pdu entries
        nPduDataLength = FindLenIntEx(pPdu->Pdu.NormPdu.nRequestId)
                       + FindLenIntEx(pPdu->Pdu.NormPdu.nErrorStatus)
                       + FindLenIntEx(pPdu->Pdu.NormPdu.nErrorIndex)
                       + nVbTotalLength;
        break;

    case SNMP_PDU_V1TRAP:

        // calculate bytes required to encode pdu entries
        nPduDataLength = FindLenIntEx(pPdu->Pdu.TrapPdu.nGenericTrap)
                       + FindLenIntEx(pPdu->Pdu.TrapPdu.nSpecificTrap)
                       + FindLenUIntEx(pPdu->Pdu.TrapPdu.nTimeticks)
                       + nVbTotalLength;
        
        // find oid length
        if ((nTmpDataLength = 
                FindLenOidEx(&pPdu->Pdu.TrapPdu.EnterpriseOid)) == BERERR)
            return FALSE; 

        // add EnterpriseOid oid length
        nPduDataLength += nTmpDataLength;

        // find address length
        if ((nTmpDataLength = 
                FindLenOctetsEx(&pPdu->Pdu.TrapPdu.AgentAddr)) == BERERR)
            return FALSE; 

        // add agent address length
        nPduDataLength += nTmpDataLength;
        break;

    default:
        return FALSE; 
    }

    // find length of pdu length
    if ((nPduLenLength = DoLenLen(nPduDataLength)) == BERERR)
        return FALSE; 

    // calculate total bytes required to encode pdu
    nPduTotalLength = 1 + nPduLenLength + nPduDataLength;

    // find length of message data
    nMsgDataLength = FindLenUIntEx(nVersion)
                   + FindLenOctetsEx(pCommunity)
                   + nPduTotalLength;

    // find length of message data length
    nMsgLenLength = DoLenLen(nMsgDataLength);

    // calculate total bytes required to encode message
    nMsgTotalLength = 1 + nMsgLenLength + nMsgDataLength;

    // record bytes required
    *pMessageSize = nMsgTotalLength;

    // make sure message fits in buffer
    if (nMsgTotalLength <= nMsgAvailLength) {
		LONG oldLength;	// the length of the request PDU
		LONG delta;		// difference between the request PDU length and the responce PDU length
		BYTE *newStream;// new location for the community stream inside the response PDU.

        // encode message as asn sequence        
        *tmpPtr++ = ASN_SEQUENCE;

        // the pointer to the community string points either directly in the incoming buffer 
        // (for req PDUs) or in the TRAP_DESTINATION_LIST_ENTRY for the outgoing traps.
        // In the first case, when building the outgoing message on the same buffer as the
        // incoming message, we need to take care to no overwrite the community name (in case
        // the length field is larger than for the initial message). Hence, in this case only
        // we shift the community name with a few octets, as many as the difference between the
        // encodings of the two lengths (the length of the outgoing response - the length of the
        // incoming request).
        if (pPdu->nType != SNMP_PDU_V1TRAP)
        {
		    // here tmpPtr points exactly to the length of the request pdu
		    oldLength = *(LONG *)tmpPtr;
		    // compute the offset the community stream should be shifted with
		    delta = nMsgLenLength - ((oldLength & 0x80) ? (oldLength & 0x7f) + 1 : 1);
		    newStream = pCommunity->stream + delta;
		    // pCommunity->stream is shifted regardles memory regions overlapp
		    memmove(newStream, pCommunity->stream, pCommunity->length);
		    // make old community to point to the new location
		    pCommunity->stream = newStream;
        }

        // encode global message information
        AddLen(&tmpPtr, nMsgLenLength, nMsgDataLength);
        AddUInt(&tmpPtr, ASN_INTEGER32, nVersion);
        AddOctets(&tmpPtr, ASN_OCTETSTRING, pCommunity);

        // encode pdu header information
        *tmpPtr++ = (BYTE)pPdu->nType;
        AddLen(&tmpPtr, nPduLenLength, nPduDataLength);        

        // determine pdu nType
        switch (pPdu->nType) {

        case SNMP_PDU_RESPONSE:
        case SNMP_PDU_TRAP:

            AddInt(&tmpPtr, ASN_INTEGER32, pPdu->Pdu.NormPdu.nRequestId);
            AddInt(&tmpPtr, ASN_INTEGER32, pPdu->Pdu.NormPdu.nErrorStatus);
            AddInt(&tmpPtr, ASN_INTEGER32, pPdu->Pdu.NormPdu.nErrorIndex);
            break;

        case SNMP_PDU_V1TRAP:

            if (AddOid(
                    &tmpPtr, 
                    ASN_OBJECTIDENTIFIER,        
                    &pPdu->Pdu.TrapPdu.EnterpriseOid)== BERERR)
                return FALSE; 

            if (AddOctets(
                    &tmpPtr, 
                    ASN_IPADDRESS, 
                    &pPdu->Pdu.TrapPdu.AgentAddr) == BERERR)
                return FALSE; 

            AddInt(&tmpPtr, ASN_INTEGER32, pPdu->Pdu.TrapPdu.nGenericTrap);
            AddInt(&tmpPtr, ASN_INTEGER32, pPdu->Pdu.TrapPdu.nSpecificTrap);
            AddUInt(&tmpPtr, ASN_TIMETICKS, pPdu->Pdu.TrapPdu.nTimeticks);
            break;

        case SNMP_PDU_GET:
        case SNMP_PDU_GETNEXT:
        case SNMP_PDU_SET:
        case SNMP_PDU_INFORM:
        case SNMP_PDU_GETBULK:
        default:
            return FALSE; 
        } 

        // encode variable bindings
        *tmpPtr++ = ASN_SEQUENCE;

        AddLen(&tmpPtr, nVbLenLength, nVbDataLength);

        if (AddVarBindList(&tmpPtr, &pPdu->Vbl) == BERERR)
            return FALSE; 

        // success
        return TRUE; 
    }

    // failure
    return FALSE;
}

BOOL
ParseMessage(
    AsnInteger32 *   pVersion,
    AsnOctetString * pCommunity,
    PSNMP_PDU        pPdu,
    PBYTE            pMessage,
    DWORD            dwMessageSize
    )

/*

Routine Description:

    Parses incoming SNMP PDU into structure.

Arguments:

    pVersion - pointer to receive SNMP version.

    pCommunity - pointer to receive community string.

    pPdu - pointer to receive remaining PDU data.

    pMessage - pointer to message to parse.

    dwMessageSize - number of bytes in message.

Return Values:

    Returns true if successful.

*/

{
    LONG lLength;
    LPBYTE pByte;
    LPBYTE pLastByte;

    // initialize community
    pCommunity->stream = NULL;
    pCommunity->length = 0;

    // initialize vbl
    pPdu->Vbl.len = 0;
    pPdu->Vbl.list = NULL;

    // validate pointer
    if (!(pByte = pMessage))  
        goto cleanup;

    // set limit based on packet size
    pLastByte = pByte + dwMessageSize;

    // decode asn sequence message wrapper     
    if (!(ParseSequence(&pByte, pLastByte, &lLength)))
        goto cleanup;

    // check for packet fragments
    if (pLastByte < (pByte + lLength))
        goto cleanup;

    // re-adjust based on data
    pLastByte = pByte + lLength;
    
    // decode snmp version
    if (!(ParseUInt(&pByte, pLastByte, pVersion)))
        goto cleanup;

    // validate snmp version
    if ((*pVersion != SNMP_VERSION_1) && 
        (*pVersion != SNMP_VERSION_2C)) 
    {
        // register version mismatch into the management structure
        mgmtCTick(CsnmpInBadVersions);

        goto cleanup;
    }

    // decode community string
    if (!(ParseOctets(&pByte, pLastByte, pCommunity)))
        goto cleanup;

    // decode nType of incoming pdu
    if ((pPdu->nType = ParseType(&pByte, pLastByte)) == BERERR)
        goto cleanup;

    // decode length of incoming pdu
    if ((lLength = ParseLength(&pByte, pLastByte)) == BERERR)
        goto cleanup;

    // validate length
    if (pByte + lLength > pLastByte)
        goto cleanup;

    // determine pdu nType
    switch (pPdu->nType) {

    case SNMP_PDU_GET:                                                          
    case SNMP_PDU_GETNEXT:                                                      
    case SNMP_PDU_SET:                                                          

        // decode the pdu header information
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.NormPdu.nRequestId)))    
            goto cleanup;                                                           
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.NormPdu.nErrorStatus)))  
            goto cleanup;                                                           
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.NormPdu.nErrorIndex)))   
            goto cleanup;                                                           

        // update the management counters for the incoming errorStatus coding
        mgmtUtilUpdateErrStatus(IN_errStatus, pPdu->Pdu.NormPdu.nErrorStatus);

        // no reason here to have any ErrorStatus and ErrorIndex.
		// initialize error status variables to NOERROR
        pPdu->Pdu.NormPdu.nErrorStatus = SNMP_ERRORSTATUS_NOERROR;
        pPdu->Pdu.NormPdu.nErrorIndex  = 0;

        break;                                                                      
                                                                                   
    case SNMP_PDU_GETBULK:                                                      

        // decode the getbulk pdu header information
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.BulkPdu.nRequestId)))    
            goto cleanup;                                                           
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.BulkPdu.nNonRepeaters)))  
            goto cleanup;                                                           
        if (!(ParseInt(&pByte, pLastByte, &pPdu->Pdu.BulkPdu.nMaxRepetitions)))   
            goto cleanup;                                                           

        // see if value needs to be adjusted
        if (pPdu->Pdu.BulkPdu.nNonRepeaters < 0) {

            // adjust non-repeaters to zero
            pPdu->Pdu.BulkPdu.nNonRepeaters = 0;    
        }

        // see if value needs to be adjusted
        if (pPdu->Pdu.BulkPdu.nMaxRepetitions < 0) {

            // adjust max-repetitions to zero
            pPdu->Pdu.BulkPdu.nMaxRepetitions = 0;
        }

        // initialize status information
        pPdu->Pdu.BulkPdu.nErrorStatus = SNMP_ERRORSTATUS_NOERROR;
        pPdu->Pdu.BulkPdu.nErrorIndex  = 0;

        break;                                                                      
                                                                                   
    case SNMP_PDU_INFORM:                                                       
    case SNMP_PDU_RESPONSE:                                                     
    case SNMP_PDU_TRAP:                                                         
    case SNMP_PDU_V1TRAP:                                                       
    default:                                                                    
        goto cleanup;
    } 

    // parse over sequence
    if (!(ParseSequence(&pByte, pLastByte, NULL)))                            
        goto cleanup;                                                           

    // parse variable binding list
    if (!(ParseVarBindList(&pByte, pLastByte, &pPdu->Vbl)))
        goto cleanup;                                                           

    // success
    return TRUE;

cleanup:

    // cleanup community string    
    SnmpUtilOctetsFree(pCommunity);

    // cleanup any allocated varbinds 
    SnmpUtilVarBindListFree(&pPdu->Vbl);

    // failure
    return FALSE;
}
