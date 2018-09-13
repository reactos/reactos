#ifndef _INC_WSNMPEXT
#define _INC_WSNMPEXT
//
// wsnmpext.h
//
// Externals include for NetPlus WinSNMP
// Copyright 1995-1997 ACE*COMM Corp
// Rleased to Microsoft under Contract
// Beta 1 version, 970228
// Bob Natale (bnatale@acecomm.com)
//
//
#ifdef __cplusplus
extern "C" {
#endif
extern TASK          TaskData;
extern SNMPTD        SessDescr;
extern SNMPTD        PDUsDescr;
extern SNMPTD        VBLsDescr;
extern SNMPTD        EntsDescr;
extern SNMPTD        CntxDescr;
extern SNMPTD        MsgDescr;       
extern SNMPTD        TrapDescr;
extern SNMPTD        AgentDescr;
extern CRITICAL_SECTION cs_TASK;
extern CRITICAL_SECTION cs_SESSION;
extern CRITICAL_SECTION cs_PDU;
extern CRITICAL_SECTION cs_VBL;
extern CRITICAL_SECTION cs_ENTITY;
extern CRITICAL_SECTION cs_CONTEXT;
extern CRITICAL_SECTION cs_MSG;
extern CRITICAL_SECTION cs_TRAP;
extern CRITICAL_SECTION cs_AGENT;
extern CRITICAL_SECTION cs_XMODE;

extern SNMPAPI_STATUS SaveError(HSNMP_SESSION hSession, SNMPAPI_STATUS nError);

extern SNMPAPI_STATUS snmpAllocTable(LPSNMPTD table);
extern SNMPAPI_STATUS SNMPAPI_CALL SnmpIpxAddressToStr (LPBYTE, LPBYTE, LPSTR);
extern BOOL BuildMessage (smiUINT32 version, smiLPOCTETS community,
            LPPDUS pdu, smiINT32 dllReqId, smiLPBYTE *msgAddr, smiLPUINT32 msgSize);
extern smiUINT32 ParseMessage (smiLPBYTE msgPtr, smiUINT32 msgLen, smiLPUINT32 version, smiLPOCTETS *community, LPPDUS pdu);
extern void FreeMsg (DWORD nMsg);
extern void FreeOctetString (smiLPOCTETS os_ptr);
extern void FreeVarBind (LPVARBIND vb_ptr);
extern void FreeVarBindList (LPVARBIND vb_ptr);
extern void FreeV1Trap (LPV1TRAP v1Trap_ptr);
extern SNMPAPI_STATUS CheckRange (DWORD index, LPSNMPTD block);

//-----------------------------------------------------------------
// snmpInitTableDescr - initializes the table descriptor with the 
// parameters given as arguments. Creates and zeroes a first chunck of table.
extern SNMPAPI_STATUS snmpInitTableDescr(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwBlocksToAdd, /*in*/DWORD dwBlockSize);
//-----------------------------------------------------------------
// snmpFreeTableDescr - releases any memory allocated for the table
extern VOID snmpFreeTableDescr(/*in*/LPSNMPTD pTableDescr);
//-----------------------------------------------------------------
// snmpAllocTableEntry - finds an empty entry into the table. If none
// already exists, table is enlarged.
extern SNMPAPI_STATUS snmpAllocTableEntry(/*in*/LPSNMPTD pTableDescr, /*out*/LPDWORD pIndex);
//-----------------------------------------------------------------
// snmpFreeTableEntry - free the location at index dwIndex from the
// table described by pTableDescr. 
extern SNMPAPI_STATUS snmpFreeTableEntry(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwIndex);
//-----------------------------------------------------------------
// snmpGetTableEntry - returns the entry at zero-based index dwIndex
// from the table described by pTableDescr
extern PVOID snmpGetTableEntry(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwIndex);
//-----------------------------------------------------------------
// snmpValidTableEntry - returns TRUE or FALSE as an entry in the table
// has valid data (is allocated) or not
extern BOOL snmpValidTableEntry(/*in*/LPSNMPTD pTableDescr, /*in*/DWORD dwIndex);


#ifdef __cplusplus
}
#endif
#endif