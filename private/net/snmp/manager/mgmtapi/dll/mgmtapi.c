/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mgmtapi.c

Abstract:

    SNMP Management API (wrapped around WinSNMP API).

Environment:

    User Mode - Win32

Revision History:

    05-Feb-1997 DonRyan
        Rewrote functions to be wrappers around WinSNMP.

--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include Files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wsipx.h>
#include <winsnmp.h>
#include <mgmtapi.h>
#include <oidconv.h>
#include <snmputil.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SNMP_MGR_SESSION {

    SOCKET            UnusedSocket;     // WARNING: Previous versions of the
    struct sockaddr   UnusedDestAddr;   // MGMTAPI.H header file exposed the
    LPSTR             UnusedCommunity;  // SNMP_MGR_SESSION structure which
    INT               UnusedTimeout;    // unfortunately encouraged people to
    INT               UnusedNumRetries; // muck with it.  Since this structure
    AsnInteger        UnusedRequestId;  // has now changed we must protect it.

    CRITICAL_SECTION  SessionLock;      // multiple threads may share session

    HSNMP_SESSION     hSnmpSession;     // handle to winsnmp session
    HSNMP_ENTITY      hAgentEntity;     // handle to agent entity
    HSNMP_ENTITY      hManagerEntity;   // handle to manager entity
    HSNMP_CONTEXT     hViewContext;     // handle to view context
    HSNMP_PDU         hPdu;             // handle to snmp pdu
    HSNMP_VBL         hVbl;             // handle to snmp pdu
    HWND              hWnd;             // handle to window

    smiINT32          nPduType;         // current pdu type
    smiINT32          nRequestId;       // current request id
    smiINT32          nErrorIndex;      // error index from pdu
    smiINT32          nErrorStatus;     // error status from pdu
    smiINT32          nLastError;       // last system error
    SnmpVarBindList * pVarBindList;     // pointer to varbind list

} SNMP_MGR_SESSION, *PSNMP_MGR_SESSION;

typedef struct _TRAP_LIST_ENTRY {

    LIST_ENTRY          Link;           // linked-list link
    AsnObjectIdentifier EnterpriseOID;  // generating enterprise
    AsnNetworkAddress   AgentAddress;   // generating agent addr
    AsnNetworkAddress   SourceAddress;  // generating network addr
    AsnInteger          nGenericTrap;   // generic trap type
    AsnInteger          nSpecificTrap;  // enterprise specific type
    AsnOctetString      Community;      // generating community
    AsnTimeticks        TimeStamp;      // time stamp
    SnmpVarBindList     VarBindList;    // variable bindings

} TRAP_LIST_ENTRY, * PTRAP_LIST_ENTRY;

#define IPADDRLEN           4
#define IPXADDRLEN          10

#define MAXENTITYSTRLEN     128

#define MINVARBINDLEN       2
#define SYSUPTIMEINDEX      0
#define SNMPTRAPOIDINDEX    1

#define DEFAULT_ADDRESS_IP  "127.0.0.1"
#define DEFAULT_ADDRESS_IPX "00000000.000000000000"

#define NOTIFICATION_CLASS  "MGMTAPI Notification Class"
#define WM_WSNMP_INCOMING   (WM_USER + 1)
#define WM_WSNMP_DONE       (WM_USER + 2)

#define WSNMP_FAILED(s)     ((s) == SNMPAPI_FAILURE)
#define WSNMP_SUCCEEDED(s)  ((s) != SNMPAPI_FAILURE)

#define WSNMP_ASSERT(s)     { if (!(s)); }


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HINSTANCE         g_hDll;                       // module handle
HANDLE            g_hTrapEvent = NULL;          // trap event handle
HANDLE            g_hTrapThread = NULL;         // trap thread handle
HANDLE            g_hTrapRegisterdEvent = NULL; // event to sync. SnmpMgrTrapListen
BOOL              g_fIsSnmpStarted = FALSE;     // indicates winsnmp inited
BOOL              g_fIsSnmpListening = FALSE;   // indicates trap thread on
BOOL              g_fIsTrapRegistered = FALSE;  // indicates trap registered
DWORD             g_dwRequestId = 1;            // unique pdu request id
LIST_ENTRY        g_IncomingTraps;              // incoming trap queue
CRITICAL_SECTION  g_GlobalLock;                 // process resource lock
SNMP_MGR_SESSION  g_TrapSMS;                    // process trap session


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private Procedures                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD
GetRequestId(
    )

/*++

Routine Description:

    Retrieve next global request id.

Arguments:

    None.

Return Values:

    Returns request id.

--*/

{
    DWORD dwRequestId;

    // obtain exclusive access to request id
    EnterCriticalSection(&g_GlobalLock);

    // obtain copy of request id
    dwRequestId = g_dwRequestId++;

    // obtain exclusive access to request id
    LeaveCriticalSection(&g_GlobalLock);

    return dwRequestId;
}


BOOL
TransferVb(
    PSNMP_MGR_SESSION pSMS,
    SnmpVarBind *     pVarBind
    )

/*++

Routine Description:

    Transfer VarBind structure to WinSNMP structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

    pVarBind - pointer to varbind to transfer.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SNMPAPI_STATUS status;
    smiVALUE tmpValue;
    smiOID tmpOID;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // validate pointers
    if ((pVarBind != NULL) &&
        (pVarBind->name.ids != NULL) &&
        (pVarBind->name.idLength != 0)) {

        // re-init
        fOk = TRUE;

        // transfer oid information
        tmpOID.len = pVarBind->name.idLength;
        tmpOID.ptr = pVarBind->name.ids;

        // only initialize value if set
        if (pSMS->nPduType == SNMP_PDU_SET) {

            // syntax values are equivalent
            tmpValue.syntax = (smiINT32)(BYTE)pVarBind->value.asnType;

            // determine type
            switch (pVarBind->value.asnType) {

            case ASN_INTEGER32:

                // transfer signed int
                tmpValue.value.sNumber = pVarBind->value.asnValue.number;
                break;

            case ASN_UNSIGNED32:
            case ASN_COUNTER32:
            case ASN_GAUGE32:
            case ASN_TIMETICKS:

                // transfer unsigned int
                tmpValue.value.uNumber = pVarBind->value.asnValue.unsigned32;
                break;

            case ASN_COUNTER64:

                // transfer 64-bit counter
                tmpValue.value.hNumber.lopart =
                    pVarBind->value.asnValue.counter64.LowPart;
                tmpValue.value.hNumber.hipart =
                    pVarBind->value.asnValue.counter64.HighPart;
                break;

            case ASN_OPAQUE:
            case ASN_IPADDRESS:
            case ASN_OCTETSTRING:
            case ASN_BITS:

                // transfer octet string
                tmpValue.value.string.len =
                    pVarBind->value.asnValue.string.length;
                tmpValue.value.string.ptr =
                    pVarBind->value.asnValue.string.stream;
                break;

            case ASN_OBJECTIDENTIFIER:

                // transfer object id
                tmpValue.value.oid.len =
                    pVarBind->value.asnValue.object.idLength;
                tmpValue.value.oid.ptr =
                    pVarBind->value.asnValue.object.ids;
                break;

            case ASN_NULL:
            case SNMP_EXCEPTION_NOSUCHOBJECT:
            case SNMP_EXCEPTION_NOSUCHINSTANCE:
            case SNMP_EXCEPTION_ENDOFMIBVIEW:

                // initialize empty byte
                tmpValue.value.empty = 0;
                break;

            default:

                // failure
                fOk = FALSE;
                break;
            }
        }

        if (fOk) {

            // register varbind
            status = SnmpSetVb(
                        pSMS->hVbl,
                        0, // index
                        &tmpOID,
                        (pSMS->nPduType == SNMP_PDU_SET)
                            ? &tmpValue
                            : NULL
                        );

            // validate return code
            if (WSNMP_FAILED(status)) {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: SnmpSetVb returned %d.\n",
                    SnmpGetLastError(pSMS->hSnmpSession)
                    ));

                // failure
                fOk = FALSE;
            }
        }
    }

    return fOk;
}


BOOL
AllocateVbl(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Transfer VarBindList structure to WinSNMP structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SNMPAPI_STATUS status;
    SnmpVarBind * pVarBind;
    DWORD cVarBind;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // validate parameters
    WSNMP_ASSERT(pSMS->pVarBindList != NULL);
    WSNMP_ASSERT(pSMS->pVarBindList->len != 0);
    WSNMP_ASSERT(pSMS->pVarBindList->list != NULL);

    // allocate resources for variable bindings list
    pSMS->hVbl = SnmpCreateVbl(pSMS->hSnmpSession, NULL, NULL);

    // validate varbind handle
    if (WSNMP_SUCCEEDED(pSMS->hVbl)) {

        // re-init
        fOk = TRUE;

        // initialize varbind pointer
        pVarBind = pSMS->pVarBindList->list;

        // initialize varbind count
        cVarBind = pSMS->pVarBindList->len;

        // process each varbind
        while (fOk && cVarBind--) {

            // transfer variable binding
            fOk = TransferVb(pSMS, pVarBind++);
        }

        if (!fOk) {

            // release varbind list handle
            status = SnmpFreeVbl(pSMS->hVbl);

            // validate return code
            if (WSNMP_FAILED(status)) {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: SnmpFreeVbl returned %d.\n",
                    SnmpGetLastError(pSMS->hSnmpSession)
                    ));
            }

            // re-initialize
            pSMS->hVbl = (HSNMP_VBL)NULL;
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: SnmpCreateVbl returned %d.\n",
            SnmpGetLastError(pSMS->hSnmpSession)
            ));
    }

    return fOk;
}


BOOL
FreeVbl(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Cleanup VarBind resources from WinSNMP structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    SNMPAPI_STATUS status;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // validate handle
    if (pSMS->hVbl != (HSNMP_VBL)NULL) {

        // actually release vbl handle
        status = SnmpFreeVbl(pSMS->hVbl);

        // validate return code
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpFreeVbl returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // failure
            fOk = FALSE;
        }

        // re-initialize handle
        pSMS->hVbl = (HSNMP_VBL)NULL;
    }

    return fOk;
}


BOOL
AllocatePdu(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Initialize session structure for sending request.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // transfer varbinds
    if (AllocateVbl(pSMS)) {

        // grab next shared request id
        pSMS->nRequestId = GetRequestId();

        // create request pdu
        pSMS->hPdu = SnmpCreatePdu(
                        pSMS->hSnmpSession,
                        pSMS->nPduType,
                        pSMS->nRequestId,
                        0, // errorStatus
                        0, // errorIndex
                        pSMS->hVbl
                        );

        // validate return status
        if (WSNMP_SUCCEEDED(pSMS->hPdu)) {

            // success
            fOk = TRUE;

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpCreatePdu returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // free resources
            FreeVbl(pSMS);
        }
    }

    return fOk;
}


BOOL
FreePdu(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Cleanup session structure after processing response.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    SNMPAPI_STATUS status;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // validate handle
    if (pSMS->hPdu != (HSNMP_PDU)NULL) {

        // free vbl
        FreeVbl(pSMS);

        // actually release pdu handle
        status = SnmpFreePdu(pSMS->hPdu);

        // validate return code
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpFreePdu returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // failure
            fOk = FALSE;
        }

        // re-initialize handle
        pSMS->hPdu = (HSNMP_PDU)NULL;
    }

    return fOk;
}


BOOL
CopyOid(
    AsnObjectIdentifier * pDstOID,
    smiLPOID              pSrcOID
    )

/*++

Routine Description:

    Copies object identifier from WinSNMP format to MGMTAPI format.

Arguments:

    pDstOID - points to MGMTAPI structure to receive OID.

    pSrcOID - points to WinSNMP structure to copy.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // validate pointers
    WSNMP_ASSERT(pDstOID != NULL);
    WSNMP_ASSERT(pSrcOID != NULL);
    WSNMP_ASSERT(pSrcOID->len != 0);
    WSNMP_ASSERT(pSrcOID->ptr != NULL);

    // store the number of subids
    pDstOID->idLength = pSrcOID->len;

    // allocate memory for subidentifiers
    pDstOID->ids = SnmpUtilMemAlloc(pDstOID->idLength * sizeof(DWORD));

    // validate pointer
    if (pDstOID->ids != NULL) {

        // transfer memory
        memcpy(pDstOID->ids,
               pSrcOID->ptr,
               pDstOID->idLength * sizeof(DWORD)
               );

        // success
        fOk = TRUE;
    }

    // now release memory for original oid
    SnmpFreeDescriptor(SNMP_SYNTAX_OID, (smiLPOPAQUE)pSrcOID);

    return fOk;
}


BOOL
CopyOctets(
    AsnOctetString * pDstOctets,
    smiLPOCTETS      pSrcOctets
    )

/*++

Routine Description:

    Copies octet string from WinSNMP format to MGMTAPI format.

Arguments:

    pDstOctets - points to MGMTAPI structure to receive octets.

    pSrcOctets - points to WinSNMP structure to copy.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SNMPAPI_STATUS status;

    // validate pointers
    WSNMP_ASSERT(pDstOctets != NULL);
    WSNMP_ASSERT(pSrcOctets != NULL);
    WSNMP_ASSERT(pSrcOctets->len != 0);
    WSNMP_ASSERT(pSrcOctets->ptr != NULL);

    // octet string allocated
    pDstOctets->dynamic = TRUE;

    // store the number of bytes
    pDstOctets->length = pSrcOctets->len;

    // allocate memory for octet string
    pDstOctets->stream = SnmpUtilMemAlloc(pDstOctets->length);

    // validate pointer
    if (pDstOctets->stream != NULL) {

        // transfer memory
        memcpy(pDstOctets->stream,
               pSrcOctets->ptr,
               pDstOctets->length
               );

        // success
        fOk = TRUE;
    }

    // now release memory for original string
    SnmpFreeDescriptor(SNMP_SYNTAX_OCTETS, (smiLPOPAQUE)pSrcOctets);

    return fOk;
}


CopyVb(
    PSNMP_MGR_SESSION pSMS,
    DWORD             iVarBind,
    SnmpVarBind *     pVarBind
    )

/*++

Routine Description:

    Copy variable binding from WinSNMP structure to MGMTAPI structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

    iVarBind - index of varbind structure to copy.

    pVarBind - pointer to varbind structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SNMPAPI_STATUS status;
    smiOID tmpOID;
    smiVALUE tmpValue;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);
    WSNMP_ASSERT(pVarBind != NULL);

    // attempt to retrieve varbind data from winsnmp structure
    status = SnmpGetVb(pSMS->hVbl, iVarBind, &tmpOID, &tmpValue);

    // validate return code
    if (WSNMP_SUCCEEDED(status)) {

        // transfer object identifier value
        fOk = CopyOid(&pVarBind->name, &tmpOID);

        // syntax values are equivalent
        pVarBind->value.asnType = (BYTE)(smiINT32)tmpValue.syntax;

        // determine syntax
        switch (tmpValue.syntax) {

        case SNMP_SYNTAX_INT32:

            // transfer signed int
            pVarBind->value.asnValue.number = tmpValue.value.sNumber;
            break;

        case SNMP_SYNTAX_UINT32:
        case SNMP_SYNTAX_CNTR32:
        case SNMP_SYNTAX_GAUGE32:
        case SNMP_SYNTAX_TIMETICKS:

            // transfer unsigned int
            pVarBind->value.asnValue.unsigned32 = tmpValue.value.uNumber;
            break;

        case SNMP_SYNTAX_CNTR64:

            // transfer 64-bit counter
            pVarBind->value.asnValue.counter64.LowPart =
                tmpValue.value.hNumber.lopart;
            pVarBind->value.asnValue.counter64.HighPart =
                tmpValue.value.hNumber.hipart;
            break;

        case SNMP_SYNTAX_OPAQUE:
        case SNMP_SYNTAX_IPADDR:
        case SNMP_SYNTAX_OCTETS:
        case SNMP_SYNTAX_BITS:

            // transfer octet string
            if (!CopyOctets(&pVarBind->value.asnValue.string,
                            &tmpValue.value.string)) {

                // re-initialize
                pVarBind->value.asnType = ASN_NULL;

                // failure
                fOk = FALSE;
            }

            break;

        case SNMP_SYNTAX_OID:

            // transfer object identifier
            if (!CopyOid(&pVarBind->value.asnValue.object,
                         &tmpValue.value.oid)) {

                // re-initialize
                pVarBind->value.asnType = ASN_NULL;

                // failure
                fOk = FALSE;
            }

            break;

        case SNMP_SYNTAX_NULL:
        case SNMP_SYNTAX_NOSUCHOBJECT:
        case SNMP_SYNTAX_NOSUCHINSTANCE:
        case SNMP_SYNTAX_ENDOFMIBVIEW:

            break; // do nothing...

        default:

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpGetVb returned invalid type.\n"
                ));

            // re-initialize
            pVarBind->value.asnType = ASN_NULL;

           // failure
            fOk = FALSE;

            break;
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: SnmpGetVb returned %d.\n",
            SnmpGetLastError(pSMS->hSnmpSession)
            ));
    }

    return fOk;
}


BOOL
CopyVbl(
    PSNMP_MGR_SESSION pSMS,
    SnmpVarBindList * pVarBindList
    )

/*++

Routine Description:

    Copy variable bindings from WinSNMP structure to MGMTAPI structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

    pVarBindList - pointer to varbind list structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);
    WSNMP_ASSERT(pVarBindList != NULL);

    // initialize
    pVarBindList->len  = 0;
    pVarBindList->list = NULL;

    // validate varbind list handle
    if (pSMS->hVbl != (HSNMP_VBL)NULL) {

        // determine number of varbinds
        pVarBindList->len = SnmpCountVbl(pSMS->hVbl);

        // validate number of varbinds
        if (WSNMP_SUCCEEDED(pVarBindList->len)) {

            // allocate memory for varbinds
            pVarBindList->list = SnmpUtilMemAlloc(
                                    pVarBindList->len *
                                    sizeof(SnmpVarBind)
                                    );

            // validate pointer
            if (pVarBindList->list != NULL) {

                DWORD cVarBind = 1;
                SnmpVarBind * pVarBind;

                // save pointer to varbinds
                pVarBind = pVarBindList->list;

                // process varbinds in the list
                while (fOk && (cVarBind <= pVarBindList->len)) {

                    // copy varbind from winsnmp to mgmtapi
                    fOk = CopyVb(pSMS, cVarBind++, pVarBind++);
                }

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: Could not allocate VBL.\n"
                    ));

                // re-initialize
                pVarBindList->len = 0;

                // failure
                fOk = FALSE;
            }

        } else if (pVarBindList->len != SNMPAPI_NOOP) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpCountVbl returned %s.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // re-initialize
            pVarBindList->len = 0;

            // failure
            fOk = FALSE;
        }
    }

    if (!fOk) {

        // cleanup any varbinds allocated
        SnmpUtilVarBindListFree(pVarBindList);
    }

    return fOk;
}


BOOL
ParseVbl(
    PSNMP_MGR_SESSION pSMS,
    PTRAP_LIST_ENTRY  pTLE
    )

/*++

Routine Description:

    Parse varbind list for trap-related varbinds.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

    pTLE - pointer to trap list entry.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SnmpVarBind * pVarBind;
    AsnObjectIdentifier * pOID;
    AsnNetworkAddress   * pAgentAddress = NULL;
    AsnObjectIdentifier * pEnterpriseOID = NULL;

    // object identifiers to convert snmpv2 trap format
    static UINT _sysUpTime[]             = { 1, 3, 6, 1, 2, 1, 1, 3       };
    static UINT _snmpTrapOID[]           = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 1 };
    static UINT _snmpAddress[]           = { 1, 3, 6, 1, 3, 1057, 1       };
    static UINT _snmpTrapEnterprise[]    = { 1, 3, 6, 1, 6, 3, 1, 1, 4, 3 };
    static UINT _snmpTraps[]             = { 1, 3, 6, 1, 6, 3, 1, 1, 5    };

    static AsnObjectIdentifier sysUpTime          = DEFINE_OID(_sysUpTime);
    static AsnObjectIdentifier snmpTrapOID        = DEFINE_OID(_snmpTrapOID);
    static AsnObjectIdentifier snmpAddress        = DEFINE_OID(_snmpAddress);
    static AsnObjectIdentifier snmpTrapEnterprise = DEFINE_OID(_snmpTrapEnterprise);
    static AsnObjectIdentifier snmpTraps          = DEFINE_OID(_snmpTraps);

    // validate pointers
    WSNMP_ASSERT(pSMS != NULL);
    WSNMP_ASSERT(pTLE != NULL);

    // validate vbl have minimum entries
    if (pTLE->VarBindList.len >= MINVARBINDLEN) {

        // point to sysUpTime varbind structure
        pVarBind = &pTLE->VarBindList.list[SYSUPTIMEINDEX];

        // verify variable is sysUpTime
        if ((pVarBind->value.asnType == ASN_TIMETICKS) &&
            !SnmpUtilOidNCmp(&pVarBind->name,
                             &sysUpTime,
                             sysUpTime.idLength)) {

            // transfer sysUpTime value to trap entry
            pTLE->TimeStamp = pVarBind->value.asnValue.ticks;

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: Could not find sysUpTime.\n"
                ));

            goto cleanup; // bail...
        }

        // see if any additional varbinds present
        if (pTLE->VarBindList.len > MINVARBINDLEN) {

            // point to snmpTrapEnterprise varbind structure (maybe)
            pVarBind = &pTLE->VarBindList.list[pTLE->VarBindList.len - 1];

            // verify variable is snmpTrapEnterprise
            if ((pVarBind->value.asnType == ASN_OBJECTIDENTIFIER) &&
                !SnmpUtilOidNCmp(&pVarBind->name,
                                 &snmpTrapEnterprise,
                                 snmpTrapEnterprise.idLength))  {

                // transfer enterprise oid to list entry
                pTLE->EnterpriseOID = pVarBind->value.asnValue.object;

                // store enterprise oid for later
                pEnterpriseOID = &pTLE->EnterpriseOID;

                // modify type to avoid deallocation
                pVarBind->value.asnType = ASN_NULL;

            } else {

                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "MGMTAPI: Could not find snmpTrapEnterprise.\n"
                    ));
            }
        }

        // see if the agent address is present
        if (pTLE->VarBindList.len > MINVARBINDLEN+1) {
            
            // point to snmpAddress varbind structure (maybe)
            pVarBind = &pTLE->VarBindList.list[pTLE->VarBindList.len - 2];

            // verify variable is snmpAddress
            if ((pVarBind->value.asnType == SNMP_SYNTAX_IPADDR) &&
                !SnmpUtilOidNCmp(&pVarBind->name,
                                 &snmpAddress,
                                 snmpAddress.idLength))  {

                // transfer agent address oid to list entry
                pTLE->AgentAddress = pVarBind->value.asnValue.address;

                // store agent address for later
                pAgentAddress = &pTLE->AgentAddress;

                // modify type to avoid deallocation
                pVarBind->value.asnType = ASN_NULL;

            } else {

                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "MGMTAPI: Could not find snmpAddress.\n"
                    ));
            }
        }

        // point to snmpTrapOID varbind structure
        pVarBind = &pTLE->VarBindList.list[SNMPTRAPOIDINDEX];

        // verify variable is snmpTrapOID
        if ((pVarBind->value.asnType == ASN_OBJECTIDENTIFIER) &&
            !SnmpUtilOidNCmp(&pVarBind->name,
                             &snmpTrapOID,
                             snmpTrapOID.idLength))  {

            // retrieve pointer to oid
            pOID = &pVarBind->value.asnValue.object;

            // check for generic trap
            if (!SnmpUtilOidNCmp(pOID,
                                 &snmpTraps,
                                 snmpTraps.idLength)) {

                // validate size is one greater than root
                if (pOID->idLength == (snmpTraps.idLength + 1)) {

                    // retrieve trap id
                    // --ft:10/01/98 (bug #231344): WINSNMP gives up the V2 syntax => pOID->ids[snmpTraps.idLength] = [1..6]
                    // --ft:10/01/98 (bug #231344): as MGMTAPI turns back to V1, we need to decrement this value.
                    pTLE->nGenericTrap = (pOID->ids[snmpTraps.idLength])-1;

                    // re-initialize
                    pTLE->nSpecificTrap = 0;

                } else {

                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "MGMTAPI: Invalid snmpTrapOID.\n"
                        ));

                    goto cleanup; // bail...
                }

            // check for specific trap
            } else if ((pEnterpriseOID != NULL) &&
                       !SnmpUtilOidNCmp(pOID,
                                        pEnterpriseOID,
                                        pEnterpriseOID->idLength)) {

                // validate size is two greater than root
                if (pOID->idLength == (pEnterpriseOID->idLength + 2)) {

                    // validate separator sub-identifier
                    WSNMP_ASSERT(pOID->ids[pEnterpriseOID->idLength] == 0);

                    // retrieve trap id
                    pTLE->nSpecificTrap = pOID->ids[pEnterpriseOID->idLength + 1];

                    // re-initialize
                    pTLE->nGenericTrap = SNMP_GENERICTRAP_ENTERSPECIFIC;

                } else {

                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "MGMTAPI: Invalid snmpTrapOID.\n"
                        ));

                    goto cleanup; // bail...
                }

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: Could not identify snmpTrapOID.\n"
                    ));

               goto cleanup; // bail...
            }

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: Could not find snmpTrapOID.\n"
                ));

            goto cleanup; // bail...
        }

        // check for enterprise oid
        if (pEnterpriseOID != NULL) {

            // release snmpTrapEnterprise varbind structure
            SnmpUtilVarBindFree(&pTLE->VarBindList.list[pTLE->VarBindList.len - 1]);

            // decrement the list length as the last varbind was freed
            pTLE->VarBindList.len--;
        }

        // check for agent address
        if (pAgentAddress != NULL) {

            // release snmpAgentAddress varbind structure
            SnmpUtilVarBindFree(&pTLE->VarBindList.list[pTLE->VarBindList.len - 1]);

            // decrement the list length as the last varbind was again freed
            pTLE->VarBindList.len--;
        }

        // release sysUpTime varbind structure
        SnmpUtilVarBindFree(&pTLE->VarBindList.list[SYSUPTIMEINDEX]);

        // release snmpTrapOID varbind structure
        SnmpUtilVarBindFree(&pTLE->VarBindList.list[SNMPTRAPOIDINDEX]);

        // subtract released varbinds
        pTLE->VarBindList.len -= MINVARBINDLEN;

        // check if all varbinds freed
        if (pTLE->VarBindList.len == 0) {

            // release memory for list
            SnmpUtilMemFree(pTLE->VarBindList.list);

            // re-initialize
            pTLE->VarBindList.list = NULL;

        } else {

            // shift varbind list up two spaces
            memmove((LPBYTE)(pTLE->VarBindList.list),
                    (LPBYTE)(pTLE->VarBindList.list + MINVARBINDLEN),
                    (pTLE->VarBindList.len * sizeof(SnmpVarBind))
                    );
        }

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: Too few subidentifiers.\n"
            ));
    }

    // success
    return TRUE;

cleanup:

    // failure
    return FALSE;
}


BOOL
FreeTle(
    PTRAP_LIST_ENTRY pTLE
    )

/*++

Routine Description:

    Release memory used for trap entry.

Arguments:

    pTLE - pointer to trap list entry.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    WSNMP_ASSERT(pTLE != NULL);

    // release memory for enterprise oid
    SnmpUtilOidFree(&pTLE->EnterpriseOID);

    // release memory for community string
    SnmpUtilMemFree(pTLE->Community.stream);

    // release memory used in varbind list
    SnmpUtilVarBindListFree(&pTLE->VarBindList);

    // release list entry
    SnmpUtilMemFree(pTLE);

    return TRUE;
}


BOOL
AllocateTle(
    PSNMP_MGR_SESSION  pSMS,
    PTRAP_LIST_ENTRY * ppTLE,
    HSNMP_ENTITY       hAgentEntity,
    HSNMP_CONTEXT      hViewContext
    )

/*++

Routine Description:

    Allocate memory for trap entry.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

    ppTLE - pointer to pointer to trap list entry.

    hAgentEntity - handle to agent sending trap.

    hViewContext - handle to view context of trap.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PTRAP_LIST_ENTRY pTLE;
    SNMPAPI_STATUS status;
    smiOCTETS CommunityStr;
    CHAR SourceStrAddr[MAXENTITYSTRLEN+1];
    struct sockaddr SourceSockAddr;

    // validate pointers
    WSNMP_ASSERT(pSMS != NULL);
    WSNMP_ASSERT(ppTLE != NULL);

    // allocate memory from list entry
    pTLE = SnmpUtilMemAlloc(sizeof(TRAP_LIST_ENTRY));

    // validate pointer
    if (pTLE == NULL) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: Could not allocate trap entry.\n"
            ));

        return FALSE; // bail...
    }

    // initialize
    *ppTLE = NULL;

    // copy varbinds to trap list entry
    if (!CopyVbl(pSMS, &pTLE->VarBindList)) {
        goto cleanup; // bail...
    }

    // parse trap-related varbinds
    if (!ParseVbl(pSMS, pTLE)) {
        goto cleanup; // bail...
    }

    // check if source address is specified
    if (hAgentEntity != (HSNMP_ENTITY)NULL) {

        // convert addr to string
        status = SnmpEntityToStr(
                    hAgentEntity,
                    sizeof(SourceStrAddr),
                    SourceStrAddr
                    );

        // validate error code
        if (WSNMP_SUCCEEDED(status)) {

            DWORD  AddrLen = 0;
            LPBYTE AddrPtr = NULL;

            // convert string to socket address structure
            SnmpSvcAddrToSocket(SourceStrAddr, &SourceSockAddr);

            // validate address family
            if (SourceSockAddr.sa_family == AF_INET) {

                // assign ip values
                AddrLen = IPADDRLEN;
                AddrPtr = (LPBYTE)&(((struct sockaddr_in *)
                            (&SourceSockAddr))->sin_addr);

            } else if (SourceSockAddr.sa_family == AF_IPX) {

                // assign ipx values
                AddrLen = IPXADDRLEN;
                AddrPtr = (LPBYTE)&(((struct sockaddr_ipx *)
                            (&SourceSockAddr))->sa_netnum);

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: Ignoring invalid address.\n"
                    ));

                goto cleanup; // bail...
            }

            // allocate address to return (if specified)
            pTLE->SourceAddress.stream = SnmpUtilMemAlloc(AddrLen);

            // validate pointer
            if (pTLE->SourceAddress.stream != NULL) {

                // initialize length values
                pTLE->SourceAddress.length  = AddrLen;
                pTLE->SourceAddress.dynamic = TRUE;

                // transfer agent address information
                memcpy(pTLE->SourceAddress.stream, AddrPtr, AddrLen);
            }

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpEntityToStr returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            goto cleanup; // bail...
        }
    }

    // check if community specified
    if (hViewContext != (HSNMP_CONTEXT)NULL) {

        // convert agent entity to string
        status = SnmpContextToStr(hViewContext, &CommunityStr);

        // validate error code
        if (WSNMP_SUCCEEDED(status)) {

            // copy octet string
            CopyOctets(&pTLE->Community, &CommunityStr);

            // release memory for string
            SnmpFreeDescriptor(SNMP_SYNTAX_OCTETS, &CommunityStr);

            // ignore terminating character
            if ((pTLE->Community.length != 0) &&
                (pTLE->Community.stream != NULL) &&
                (pTLE->Community.stream[pTLE->Community.length] == 0)) {

                // decrement count
                pTLE->Community.length--;
            }

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpContextToStr returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            goto cleanup; // bail...
        }
    }

    // transfer
    *ppTLE = pTLE;

    // success
    return TRUE;

cleanup:

    // release
    FreeTle(pTLE);

    // failure
    return FALSE;
}


BOOL
NotificationCallback(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Callback for processing notification messages.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    Returns true if processing finished.

--*/

{
    BOOL fDone = TRUE;
    SNMPAPI_STATUS status;
    HSNMP_ENTITY   hAgentEntity   = (HSNMP_ENTITY)NULL;
    HSNMP_ENTITY   hManagerEntity = (HSNMP_ENTITY)NULL;
    HSNMP_CONTEXT  hViewContext   = (HSNMP_CONTEXT)NULL;
    smiINT32       nPduType;
    smiINT32       nRequestId;

    // validate pointer
    WSNMP_ASSERT(pSMS != NULL);

    // retrieve message
    status = SnmpRecvMsg(
                pSMS->hSnmpSession,
                &hAgentEntity,
                &hManagerEntity,
                &hViewContext,
                &pSMS->hPdu
                );

    // validate return code
    if (WSNMP_SUCCEEDED(status)) {

        // retrieve pdu data
        status = SnmpGetPduData(
                    pSMS->hPdu,
                    &nPduType,
                    &nRequestId,
                    &pSMS->nErrorStatus,
                    &pSMS->nErrorIndex,
                    &pSMS->hVbl
                    );

        // validate return code
        if (WSNMP_SUCCEEDED(status)) {

            // process reponse to request
            if (nPduType == SNMP_PDU_RESPONSE) {

                // validate context information
                if ((pSMS->nRequestId == nRequestId) &&
                    (pSMS->hViewContext == hViewContext) &&
                    (pSMS->hAgentEntity == hAgentEntity) &&
                    (pSMS->hManagerEntity == hManagerEntity)) {

                    // validate returned error status
                    if (pSMS->nErrorStatus == SNMP_ERROR_NOERROR) {

                        SnmpVarBindList VarBindList;

                        // copy variable binding list
                        if (CopyVbl(pSMS, &VarBindList)) {

                            // release existing varbind list
                            SnmpUtilVarBindListFree(pSMS->pVarBindList);

                            // manually copy new varbind list
                            *pSMS->pVarBindList = VarBindList;

                        } else {

                            // modify last error status
                            pSMS->nLastError = SNMPAPI_ALLOC_ERROR;
                        }
                    }

                } else {

                    SNMPDBG((
                        SNMP_LOG_TRACE,
                        "MGMTAPI: Ignoring invalid context.\n"
                        ));

                    // continue
                    fDone = FALSE;
                }

            } else if (nPduType == SNMP_PDU_TRAP) {

                PTRAP_LIST_ENTRY pTLE;

                // allocate trap list entry (transfers varbinds etc.)
                if (AllocateTle(pSMS, &pTLE, hAgentEntity, hViewContext)) {

                    // obtain exclusive access
                    EnterCriticalSection(&g_GlobalLock);

                    // insert new trap into the incoming queue
                    InsertTailList(&g_IncomingTraps, &pTLE->Link);

                    // alert user
                    SetEvent(g_hTrapEvent);

                    // release exclusive access
                    LeaveCriticalSection(&g_GlobalLock);
                }

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: Ignoring invalid pdu type %d.\n",
                    nPduType
                    ));

                // continue
                fDone = FALSE;
            }

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpGetPduData returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // retrieve last error status from winsnmp
            pSMS->nLastError = SnmpGetLastError(pSMS->hSnmpSession);
        }

        // release temporary entity
        SnmpFreeEntity(hAgentEntity);

        // release temporary entity
        SnmpFreeEntity(hManagerEntity);

        // release temporary context
        SnmpFreeContext(hViewContext);

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: SnmpRecvMsg returned %d.\n",
            SnmpGetLastError(pSMS->hSnmpSession)
            ));

        // retrieve last error status from winsnmp
        pSMS->nLastError = SnmpGetLastError(pSMS->hSnmpSession);
    }

    // release pdu
    FreePdu(pSMS);

    return fDone;
}


LRESULT
CALLBACK
NotificationWndProc(
    HWND   hWnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Callback that processes WinSNMP notifications.

Arguments:

    hWnd - window handle.

    uMsg - message identifier.

    wParam - first message parameter.

    lParam - second message parameter.

Return Values:

    The return value is the result of the message processing and
    depends on the message sent.

--*/

{
    // check for winsnmp notification
    if (uMsg == WM_WSNMP_INCOMING) {

        PSNMP_MGR_SESSION pSMS;

        // retrieve mgmtapi session pointer from window
        pSMS = (PSNMP_MGR_SESSION)GetWindowLongPtr(hWnd, 0);

        // validate session ptr
        WSNMP_ASSERT(pSMS != NULL);

        // process notification message
        if (NotificationCallback(pSMS)) {

            // post message to break out of message pump
            PostMessage(pSMS->hWnd, WM_WSNMP_DONE, (WPARAM)0, (LPARAM)0);
        }

        return (LRESULT)0;

    } else {

        // forward all other messages to windows
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}


BOOL
RegisterNotificationClass(
    )

/*++

Routine Description:

    Register notification class for sessions.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;
    WNDCLASS wc;

    // initialize notification window class
    wc.lpfnWndProc   = (WNDPROC)NotificationWndProc;
    wc.lpszClassName = NOTIFICATION_CLASS;
    wc.lpszMenuName  = NULL;
    wc.hInstance     = g_hDll;
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = NULL;
    wc.cbWndExtra    = sizeof(PSNMP_MGR_SESSION);
    wc.cbClsExtra    = 0;
    wc.style         = 0;

    // register class
    fOk = RegisterClass(&wc);

    if (!fOk) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: RegisterClass returned %d.\n",
            GetLastError()
            ));
    }

    return fOk;
}


BOOL
UnregisterNotificationClass(
    )

/*++

Routine Description:

    Unregister notification class.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;

    // unergister notification window class
    fOk = UnregisterClass(NOTIFICATION_CLASS, g_hDll);

    if (!fOk) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: UnregisterClass returned %d.\n",
            GetLastError()
            ));
    }

    return fOk;
}


BOOL
StartSnmpIfNecessary(
    )

/*++

Routine Description:

    Initialize WinSNMP DLL if necessary.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // serialize access to startup code
    EnterCriticalSection(&g_GlobalLock);

    // see if already started
    if (g_fIsSnmpStarted != TRUE) {

        SNMPAPI_STATUS status;

        // initialize start params
        smiUINT32 nMajorVersion   = 0;
        smiUINT32 nMinorVersion   = 0;
        smiUINT32 nLevel          = 0;
        smiUINT32 nTranslateMode  = 0;
        smiUINT32 nRetransmitMode = 0;

        // start winsnmp
        status = SnmpStartup(
                    &nMajorVersion,
                    &nMinorVersion,
                    &nLevel,
                    &nTranslateMode,
                    &nRetransmitMode
                    );

        // validate return code
        if (WSNMP_SUCCEEDED(status)) {

            SNMPDBG((
                SNMP_LOG_TRACE,
                "MGMTAPI: SnmpStartup succeeded:\n"
                "MGMTAPI:\tnMajorVersion   = %d\n"
                "MGMTAPI:\tnMinorVersion   = %d\n"
                "MGMTAPI:\tnLevel          = %d\n"
                "MGMTAPI:\tnTranslateMode  = %d\n"
                "MGMTAPI:\tnRetransmitMode = %d\n",
                nMajorVersion,
                nMinorVersion,
                nLevel,
                nTranslateMode,
                nRetransmitMode
                ));

            // allocate global trap available event
            g_hTrapEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            // allocate global event to sync. SnmpMgrTrapListen
            g_hTrapRegisterdEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            // make sure translate mode is snmp v1
            SnmpSetTranslateMode(SNMPAPI_UNTRANSLATED_V1);

            // make sure retransmit mode is on
            SnmpSetRetransmitMode(SNMPAPI_ON);

            // register notification class
            RegisterNotificationClass();

            // save new status
            g_fIsSnmpStarted = TRUE;

            // success
            fOk = TRUE;

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpStartup returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            // failure
            fOk = FALSE;
        }
    }

    // serialize access to startup code
    LeaveCriticalSection(&g_GlobalLock);

    return fOk;
}


BOOL
CleanupIfNecessary(
    )

/*++

Routine Description:

    Cleanup WinSNMP DLL if necessary.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;

    // serialize access to startup code
    EnterCriticalSection(&g_GlobalLock);

    // see if already started
    if (g_fIsSnmpStarted == TRUE) {

        SNMPAPI_STATUS status;

        // shutdown winsnmp
        status = SnmpCleanup();

        // validate return code
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpCleanup returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            // failure
            fOk = FALSE;
        }

        // unregister notification class
        UnregisterNotificationClass();

        // save new status
        g_fIsSnmpStarted = FALSE;
    }

    // check trap handle
    if (g_hTrapEvent != NULL) {

        // close trap handle
        CloseHandle(g_hTrapEvent);

        // re-initialize
        g_hTrapEvent = NULL;
    }

    // serialize access to startup code
    LeaveCriticalSection(&g_GlobalLock);

    return fOk;
}


BOOL
CreateNotificationWindow(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Create notification window for session.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // create notification window
    pSMS->hWnd = CreateWindow(
                    NOTIFICATION_CLASS,
                    NULL,       // pointer to window name
                    0,          // window style
                    0,          // horizontal position of window
                    0,          // vertical position of window
                    0,          // window width
                    0,          // window height
                    NULL,       // handle to parent or owner window
                    NULL,       // handle to menu or child-window identifier
                    g_hDll,     // handle to application instance
                    NULL        // pointer to window-creation data
                    );

    // validate window handle
    if (pSMS->hWnd != NULL) {

        // store pointer to session in window
        SetWindowLongPtr(pSMS->hWnd, 0, (LONG_PTR)pSMS);

        // success
        fOk = TRUE;

    } else {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: CreateWindow returned %d.\n",
            GetLastError()
            ));

        // failure
        fOk = FALSE;
    }

    return fOk;
}


BOOL
DestroyNotificationWindow(
    HWND hWnd
    )

/*++

Routine Description:

    Destroy notification window for session.

Arguments:

    hWnd - window handle for session.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;

    // destroy notification window
    fOk = DestroyWindow(hWnd);

    if (!fOk) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: DestroyWindow returned %d.\n",
            GetLastError()
            ));
    }

    return fOk;
}


BOOL
CloseSession(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Close WinSNMP session associated with MGMTAPI session.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = TRUE;
    SNMPAPI_STATUS status;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // check if window opened
    if (pSMS->hWnd != (HWND)NULL) {

        // destroy notification window
        fOk = DestroyNotificationWindow(pSMS->hWnd);
    }

    // check if agent entity allocated
    if (pSMS->hAgentEntity != (HSNMP_ENTITY)NULL) {

        // close the entity handle
        status = SnmpFreeEntity(pSMS->hAgentEntity);

        // validate status
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpFreeEntity returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            // failure
            fOk = FALSE;
        }

        // re-initialize
        pSMS->hAgentEntity = (HSNMP_ENTITY)NULL;
    }

    // check if manager entity allocated
    if (pSMS->hManagerEntity != (HSNMP_ENTITY)NULL) {

        // close the entity handle
        status = SnmpFreeEntity(pSMS->hManagerEntity);

        // validate status
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpFreeEntity returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            // failure
            fOk = FALSE;
        }

        // re-initialize
        pSMS->hManagerEntity = (HSNMP_ENTITY)NULL;
    }

    // check if session allocated
    if (pSMS->hSnmpSession != (HSNMP_SESSION)NULL) {

        // close the winsnmp session
        status = SnmpClose(pSMS->hSnmpSession);

        // validate status
        if (WSNMP_FAILED(status)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpClose returned %d.\n",
                SnmpGetLastError((HSNMP_SESSION)NULL)
                ));

            // failure
            fOk = FALSE;
        }

        // re-initialize
        pSMS->hSnmpSession = (HSNMP_SESSION)NULL;
    }

    return fOk;
}

SNMPAPI_STATUS SNMPAPI_CALL
   SnmpConveyAgentAddress (SNMPAPI_STATUS mode);


BOOL
OpenSession(
    PSNMP_MGR_SESSION pSMS,
    LPSTR             pAgentAddress,
    LPSTR             pAgentCommunity,
    INT               nTimeOut,
    INT               nRetries
    )

/*++

Routine Description:

    Open WinSNMP session and associate with MGMTAPI session.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

    pAgentAddress - points to a null-terminated string specifying either a
        dotted-decimal IP address or a host name that can be resolved to an
        IP address, an IPX address (in 8.12 notation), or an ethernet address.

    pAgentCommunity - points to a null-terminated string specifying the
        SNMP community name used when communicating with the agent specified
        in the lpAgentAddress parameter

    nTimeOut - specifies the communications time-out in milliseconds.

    nRetries - specifies the communications retry count.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk;
    struct sockaddr AgentSockAddr;
    CHAR AgentStrAddr[MAXENTITYSTRLEN+1];
    smiOCTETS smiCommunity;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // initialize notification window
    if (!CreateNotificationWindow(pSMS)) {
        return FALSE; // bail...
    }

    // open a winsnmp session which corresponds to mgmtapi session
    pSMS->hSnmpSession = SnmpOpen(pSMS->hWnd, WM_WSNMP_INCOMING);

    // --ft
    // we need to turn this on in order to have WINSNMP to pass back not
    // only the entity standing for the source Ip address but also the
    // agent address as it was sent into the V1 Trap Pdu. Without it,
    // SnmpMgrGetTrapEx() will return a NULL address for the pSourceAddress
    // paramter. However, SnmpMgrGetTrapEx() is not documented!!!
    SnmpConveyAgentAddress(SNMPAPI_ON);

    // validate session handle returned
    if (WSNMP_FAILED(pSMS->hSnmpSession)) {

        SNMPDBG((
            SNMP_LOG_ERROR,
            "MGMTAPI: SnmpOpen returned %d.\n",
            SnmpGetLastError((HSNMP_SESSION)NULL)
            ));

        // re-initialize
        pSMS->hSnmpSession = (HSNMP_SESSION)NULL;

        goto cleanup; // bail...
    }

    // validate pointer
    if (pAgentAddress != NULL) {

        // use snmpapi.dll to do convert to sockets structure
        if (!SnmpSvcAddrToSocket(pAgentAddress, &AgentSockAddr)) {
            goto cleanup; // bail...
        }

        // check address family of agent
        if (AgentSockAddr.sa_family == AF_INET) {

            LPSTR pAgentStrAddr;
            struct sockaddr_in * pAgentSockAddr;

            // cast generic socket address structure to inet
            pAgentSockAddr = (struct sockaddr_in *)&AgentSockAddr;

            // obtain exclusive access to api
            EnterCriticalSection(&g_GlobalLock);

            // attempt to convert address into string
            pAgentStrAddr = inet_ntoa(pAgentSockAddr->sin_addr);

            // copy to stack variable
            strcpy(AgentStrAddr, pAgentStrAddr);

            // release exclusive access to api
            LeaveCriticalSection(&g_GlobalLock);

        } else if (AgentSockAddr.sa_family == AF_IPX) {

            // simply copy original string
            strcpy(AgentStrAddr, pAgentAddress);

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: Incorrect address family.\n"
                ));

            goto cleanup; // bail...
        }

        // create remote agent entity
        pSMS->hAgentEntity = SnmpStrToEntity(
                                    pSMS->hSnmpSession,
                                    AgentStrAddr
                                    );

        // validate agent entity returned
        if (WSNMP_FAILED(pSMS->hAgentEntity)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpStrToEntity returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // re-initialize
            pSMS->hAgentEntity = (HSNMP_ENTITY)NULL;

            goto cleanup; // bail...
        }

        // attach timeout specified with agent
        SnmpSetTimeout(pSMS->hAgentEntity, nTimeOut / 10);

        // attach retries specified with agent
        SnmpSetRetry(pSMS->hAgentEntity, nRetries);

        // create local manager entity
        pSMS->hManagerEntity = SnmpStrToEntity(
                                        pSMS->hSnmpSession,
                                        (AgentSockAddr.sa_family == AF_INET)
                                            ? DEFAULT_ADDRESS_IP
                                            : DEFAULT_ADDRESS_IPX
                                        );

        // validate manager entity returned
        if (WSNMP_FAILED(pSMS->hManagerEntity)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpStrToEntity returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // re-initialize
            pSMS->hManagerEntity = (HSNMP_ENTITY)NULL;

            goto cleanup; // bail...
        }

        // attach timeout specified with manager
        SnmpSetTimeout(pSMS->hManagerEntity, nTimeOut / 10);

        // attach retries specified with manager
        SnmpSetRetry(pSMS->hManagerEntity, nRetries);
    }

    // validate pointer
    if (pAgentCommunity != NULL) {

        // transfer community string
        smiCommunity.ptr = (smiLPBYTE)pAgentCommunity;
        smiCommunity.len = pAgentCommunity ? lstrlen(pAgentCommunity) : 0;

        // obtain context from community string
        pSMS->hViewContext = SnmpStrToContext(
                                pSMS->hSnmpSession,
                                &smiCommunity
                                );

        // validate context handle
        if (WSNMP_FAILED(pSMS->hViewContext)) {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpStrToContext returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // re-initialize
            pSMS->hViewContext = (HSNMP_CONTEXT)NULL;

            goto cleanup; // bail...
        }
    }

    // success
    return TRUE;

cleanup:

    // cleanup resources
    CloseSession(pSMS);

    // failure
    return FALSE;
}


BOOL
AllocateSession(
    PSNMP_MGR_SESSION * ppSMS
    )

/*++

Routine Description:

    Allocate mgmtapi session structure.

Arguments:

    ppSMS - pointer to session pointer to return.

Return Values:

    Returns true if successful.

--*/

{
    PSNMP_MGR_SESSION pSMS = NULL;

       __try
    {
        // allocate new session table entry
        pSMS = SnmpUtilMemAlloc(sizeof(SNMP_MGR_SESSION));

        // validate pointer
        if (pSMS != NULL) {

            // initialize session level lock
            InitializeCriticalSection(&pSMS->SessionLock);

        } else {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: Could not allocate session.\n"
                ));

            // notify application of error
            SetLastError(SNMP_MEM_ALLOC_ERROR);
        }

        // transfer
        *ppSMS = pSMS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        if (pSMS != NULL)
        {
            SnmpUtilMemFree(pSMS);
            pSMS = NULL;
        }
    }

    // return status
    return (pSMS != NULL);
}


VOID
FreeSession(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Frees mgmtapi session structure.

Arguments:

    pSMS - pointer to mgmtapi session structure.

Return Values:

    None.

--*/

{
    // is session valid?
    if (pSMS != NULL) {

        // destroy the session level lock
        DeleteCriticalSection(&pSMS->SessionLock);

        // free session object
        SnmpUtilMemFree(pSMS);
    }
}


BOOL
ProcessAgentResponse(
    PSNMP_MGR_SESSION pSMS
    )

/*++

Routine Description:

    Message pump for notification window.

Arguments:

    pSMS - pointer to MGMTAPI session structure.

Return Values:

    Returns true if agent responded.

--*/

{
    MSG msg;
    BOOL fOk = FALSE;

    // validate session ptr
    WSNMP_ASSERT(pSMS != NULL);

    // get the next message for this session
    while (GetMessage(&msg, pSMS->hWnd, 0, 0)) {

        // check for private message
        if (msg.message != WM_WSNMP_DONE) {

            // translate message
            TranslateMessage(&msg);

            // dispatch message
            DispatchMessage(&msg);

        } else {

            // success
            fOk = TRUE;

            break;
        }
    }

    return fOk;
}


DWORD
WINAPI
TrapThreadProc(
    LPVOID lpParam
    )

/*++

Routine Description:

    Trap processing procedure.

Arguments:

    lpParam - unused thread parameter.

Return Values:

    Returns NOERROR if successful.

--*/

{
    SNMPAPI_STATUS status;
    PSNMP_MGR_SESSION pSMS;

    SNMPDBG((
        SNMP_LOG_TRACE,
        "MGMTAPI: Trap thread starting...\n"
        ));

    // obtain pointer
    pSMS = &g_TrapSMS;

    // re-initialize
    pSMS->nLastError = 0;

    g_fIsTrapRegistered = FALSE; // init to failure. Note that there will
                                 // be only 1 instance of this thread


    // initialize winsnmp trap session
    if (OpenSession(pSMS, NULL, NULL, 0, 0)) 
    {

        // register
        status = SnmpRegister(
                    pSMS->hSnmpSession,
                    (HSNMP_ENTITY)NULL,     // hAgentEntity
                    (HSNMP_ENTITY)NULL,     // hManagerEntity
                    (HSNMP_CONTEXT)NULL,    // hViewContext
                    (smiLPCOID)NULL,        // notification
                    SNMPAPI_ON
                    );

        // validate return code
        if (WSNMP_SUCCEEDED(status)) 
        {
            // signal main thread that Trap has been registered with WinSNMP
            g_fIsTrapRegistered = TRUE;
            SetEvent(g_hTrapRegisterdEvent);

            // loop processing responses
            while (ProcessAgentResponse(pSMS)) 
            {

                //
                // processing done in window procedure...
                //
            }

        } 
        else 
        {

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: SnmpRegister returned %d.\n",
                SnmpGetLastError(pSMS->hSnmpSession)
                ));

            // transfer last error to global structure
            pSMS->nLastError = SnmpGetLastError(pSMS->hSnmpSession);

            // signal main thread that there is an error 
            // in registering Trap with WinSNMP
            
            SetEvent(g_hTrapRegisterdEvent);
        }

    } 
    else 
    {

        // transfer last error to global structure
        pSMS->nLastError = SnmpGetLastError((HSNMP_SESSION)NULL);
        
        // signal main thread that there is an error 
        // in registering Trap with WinSNMP
      
        SetEvent(g_hTrapRegisterdEvent);
    }

    // free session
    CloseSession(pSMS);

    // obtain exclusive access
    EnterCriticalSection(&g_GlobalLock);

    // signal successful start
    g_fIsSnmpListening = FALSE;

    // release exclusive access
    LeaveCriticalSection(&g_GlobalLock);

    SNMPDBG((
        SNMP_LOG_TRACE,
        "MGMTAPI: Trap thread exiting...\n"
        ));

    // success
    return NOERROR;
}


BOOL
StartTrapsIfNecessary(
    HANDLE * phTrapAvailable
    )

/*++

Routine Description:

    Initializes global structures for trap listening.

Arguments:

    phTrapAvailable - pointer to event for signalling traps.

Return Values:

    Returns true if successful (must be called only once).

--*/

{
    BOOL fOk = FALSE;
    DWORD dwTrapThreadId;
    DWORD dwWaitTrapRegisterd;

    // validate pointer
    if (phTrapAvailable != NULL) 
    {

        // obtain exclusive access
        EnterCriticalSection(&g_GlobalLock);

        // transfer trap event to app
        *phTrapAvailable = g_hTrapEvent;

        // only start listening once
        if (g_fIsSnmpListening == FALSE) 
        {

            // spawn client trap thread
            g_hTrapThread = CreateThread(
                                NULL,   // lpThreadAttributes
                                0,      // dwStackSize
                                TrapThreadProc,
                                NULL,   // lpParameter
                                0,      // dwCreationFlags
                                &dwTrapThreadId
                                );

            // signal successful start
            g_fIsSnmpListening = TRUE;

            // release exclusive access
            LeaveCriticalSection(&g_GlobalLock);

            // WinSE bug 6182
            // wait for TrapThreadProc to signal sucessful or failure
            dwWaitTrapRegisterd = WaitForSingleObject(g_hTrapRegisterdEvent, INFINITE);
            if (dwWaitTrapRegisterd == WAIT_OBJECT_0)
            {
                if (g_fIsTrapRegistered == TRUE)
                    fOk = TRUE;  // success
                else
                {
                    SetLastError(SNMP_MGMTAPI_TRAP_ERRORS);

                    SNMPDBG((
                        SNMP_LOG_ERROR,
                        "MGMTAPI: Traps are not accessible.\n"
                        ));

                }
            }
            else
            {
                SetLastError(SNMP_MGMTAPI_TRAP_ERRORS);

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: Traps are not accessible.\n"
                    ));
            }
        
        } 
        else 
        {

            // whine about having called this before
            SetLastError(SNMP_MGMTAPI_TRAP_DUPINIT);

            SNMPDBG((
                SNMP_LOG_ERROR,
                "MGMTAPI: Duplicate registration detected.\n"
                ));
            // release exclusive access
            LeaveCriticalSection(&g_GlobalLock);
        }

    }

    return fOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Dll Entry Point                                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
DllMain(
    HANDLE hDll,
    DWORD  dwReason,
    LPVOID lpReserved
    )

/*++

Routine Description:

    Dll entry point.

Arguments:

    hDll - module handle.

    dwReason - reason DllMain is being called.

    lpReserved - unused.

Return Values:

    None.

--*/

{
    BOOL bOk = TRUE;

    __try
    {
        // determine reason for being called
        if (dwReason == DLL_PROCESS_ATTACH)
        {

            // initialize startup critical section
            InitializeCriticalSection(&g_GlobalLock);

            // initialize list of incoming traps
            InitializeListHead(&g_IncomingTraps);

            // optimize thread startup
            DisableThreadLibraryCalls(hDll);

            // save handle
            g_hDll = hDll;
        }
        else if (dwReason == DLL_PROCESS_DETACH)
        {

            // cleanup winsnmp
            CleanupIfNecessary();

            // nuke startup critical section
            DeleteCriticalSection(&g_GlobalLock);
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bOk = FALSE;
    }

    return bOk;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public Procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
LPSNMP_MGR_SESSION
SNMP_FUNC_TYPE
SnmpMgrOpen(
    LPSTR pAgentAddress,
    LPSTR pAgentCommunity,
    INT   nTimeOut,
    INT   nRetries
    )

/*++

Routine Description:

    Initializes resources necessary for communication with specified agent.

Arguments:

    pAgentAddress - points to a null-terminated string specifying either a
        dotted-decimal IP address or a host name that can be resolved to an
        IP address, an IPX address (in 8.12 notation), or an ethernet address.

    pAgentCommunity - points to a null-terminated string specifying the
        SNMP community name used when communicating with the agent specified
        in the lpAgentAddress parameter

    nTimeOut - specifies the communications time-out in milliseconds.

    nRetries - specifies the communications retry count.

Return Values:

    Returns session handle if successful.

--*/

{
    PSNMP_MGR_SESSION pSMS = NULL;

    // initialize winsnmp
    if (StartSnmpIfNecessary()) {

        // allocate mgmtapi session
        if (AllocateSession(&pSMS)) {

            // open session
            if (!OpenSession(
                    pSMS,
                    pAgentAddress,
                    pAgentCommunity,
                    nTimeOut,
                    nRetries)) {

                // free session
                FreeSession(pSMS);

                // reset
                pSMS = NULL;
            }
        }
    }

    // return opaque pointer
    return (LPSNMP_MGR_SESSION)pSMS;
}

BOOL
SNMP_FUNC_TYPE
SnmpMgrCtl(
    LPSNMP_MGR_SESSION session,             // pointer to the MGMTAPI session
    DWORD              dwCtlCode,           // control code for the command requested
    LPVOID             lpvInBuffer,         // buffer with the input parameters for the operation
    DWORD              cbInBuffer,          // size of lpvInBuffer in bytes
    LPVOID             lpvOUTBuffer,        // buffer for all the output parameters of the command
    DWORD              cbOUTBuffer,         // size of lpvOUTBuffer
    LPDWORD            lpcbBytesReturned    // space used from lpvOutBuffer
    )
/*++

Routine Description:

    Operates several control operations over the MGMTAPI session

Arguments:

    pSession - pointer to the session to 


Return Values:


--*/
{
    BOOL bOk = FALSE;
    PSNMP_MGR_SESSION pSMS = (PSNMP_MGR_SESSION)session;

    switch(dwCtlCode)
    {
    case MGMCTL_SETAGENTPORT:
        if (pSMS == NULL)
            SetLastError(SNMP_MGMTAPI_INVALID_SESSION);
        else if (lpvInBuffer == NULL || cbInBuffer < sizeof(UINT))
            SetLastError(SNMP_MGMTAPI_INVALID_BUFFER);
        else if (WSNMP_FAILED(SnmpSetPort(pSMS->hAgentEntity, *(UINT*)lpvInBuffer)))
            SetLastError(SnmpGetLastError(pSMS->hSnmpSession));
        else
            bOk = TRUE;
        break;

    default:
        SetLastError(SNMP_MGMTAPI_INVALID_CTL);
        break;
    }

    return bOk;
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrClose(
    LPSNMP_MGR_SESSION session
    )

/*++

Routine Description:

    Cleanups resources needed for communication with specified agent.

Arguments:

    session - points to an internal structure that specifies
        which session to close.

Return Values:

    Returns true if successful.

--*/

{
    PSNMP_MGR_SESSION pSMS = (PSNMP_MGR_SESSION)session;

    // validate pointer
    if (pSMS != NULL) {

        // close session
        CloseSession(pSMS);

        // free session
        FreeSession(pSMS);
    }

    return TRUE;
}


SNMPAPI
SNMP_FUNC_TYPE
SnmpMgrRequest(
    LPSNMP_MGR_SESSION session,
    BYTE               requestType,
    SnmpVarBindList  * pVarBindList,
    AsnInteger       * pErrorStatus,
    AsnInteger       * pErrorIndex
    )

/*++

Routine Description:

    Requests the specified operation be performed with the specified agent.

Arguments:

    session - points to an internal structure that specifies the session
        that will perform the request.

    requestType - specifies the SNMP request type.

    pVarBindList - points to the variable bindings list

    pErrorStatus - points to a variable in which the error status result
        will be returned.

    pErrorIndex - points to a variable in which the error index result
        will be returned.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    SNMPAPI_STATUS status;
    PSNMP_MGR_SESSION pSMS = (PSNMP_MGR_SESSION)session;

    // validate pointers
    if ((pSMS != NULL) &&
        (pErrorIndex != NULL) &&
        (pErrorStatus != NULL) &&
        (pVarBindList != NULL) &&
        (pVarBindList->len != 0) &&
        (pVarBindList->list != NULL)) {

        // obtain exclusive access to session
        EnterCriticalSection(&pSMS->SessionLock);

        // initialize session structure
        pSMS->pVarBindList = pVarBindList;
        pSMS->nPduType = (smiINT32)(BYTE)requestType;
        pSMS->hVbl = (HSNMP_VBL)NULL;
        pSMS->hPdu = (HSNMP_PDU)NULL;
        pSMS->nErrorStatus = 0;
        pSMS->nErrorIndex = 0;
        pSMS->nLastError = 0;

        // allocate resources
        if (AllocatePdu(pSMS)) {

            // actually send
            status = SnmpSendMsg(
                        pSMS->hSnmpSession,
                        pSMS->hManagerEntity,
                        pSMS->hAgentEntity,
                        pSMS->hViewContext,
                        pSMS->hPdu
                        );

            // release now
            FreePdu(pSMS);

            // validate return code
            if (WSNMP_SUCCEEDED(status)) {

                // process agent response
                if (ProcessAgentResponse(pSMS) &&
                   (pSMS->nLastError == SNMP_ERROR_NOERROR)) {

                    // update error status and index
                    *pErrorStatus = pSMS->nErrorStatus;
                    *pErrorIndex  = pSMS->nErrorIndex;

                    // success
                    fOk = TRUE;

                } else {

                    // set error to winsnmp error
                    SetLastError(pSMS->nLastError);

                    // failure
                    fOk = FALSE;
                }

            } else {

                SNMPDBG((
                    SNMP_LOG_ERROR,
                    "MGMTAPI: SnmpSendMsg returned %d.\n",
                    SnmpGetLastError(pSMS->hSnmpSession)
                    ));
            }
        }

        // release exclusive access to session
        LeaveCriticalSection(&pSMS->SessionLock);
    }

    return fOk;
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrStrToOid(
    LPSTR                 pString,
    AsnObjectIdentifier * pOID
    )

/*++

Routine Description:

    Converts a string object identifier or object descriptor representation
    to an internal object identifier.

Arguments:

    pString - points to a null-terminated string to be converted.

    pOID - points to an object identifier variable that will receive the
        converted value.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer to oid and string
    if ((pOID != NULL) && (pString != NULL)) {

        // forward to lame mibcc code for now
        return SnmpMgrText2Oid(pString, pOID);
    }

    return FALSE;
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrOidToStr(
    AsnObjectIdentifier * pOID,
    LPSTR               * ppString
    )

/*++

Routine Description:

    Converts an internal object identifier to a string object identifier or
    object descriptor representation.

Arguments:

    pOID - pointers to object identifier to be converted.

    ppString - points to string pointer to receive converted value.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer to oid and string
    if ((pOID != NULL) && (ppString != NULL)) {

        // forward to lame mibcc code for now
        return SnmpMgrOid2Text(pOID, ppString);
    }

    return FALSE;
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrTrapListen(
    HANDLE * phTrapAvailable
    )

/*++

Routine Description:

    Registers the ability of a manager application to receive SNMP traps.

Arguments:

    phTrapAvailable - points to an event handle that will be used to indicate
        that there are traps available

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;

    // startup winsnmp
    if (StartSnmpIfNecessary()) {

        // spawn only one trap client thread
        if (StartTrapsIfNecessary(phTrapAvailable)) {

            // success
            fOk = TRUE;
        }
    }

    return fOk;
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrGetTrap(
    AsnObjectIdentifier * pEnterpriseOID,
    AsnNetworkAddress   * pAgentAddress,
    AsnInteger          * pGenericTrap,
    AsnInteger          * pSpecificTrap,
    AsnTimeticks        * pTimeStamp,
    SnmpVarBindList     * pVarBindList
    )

/*++

Routine Description:

    Returns outstanding trap data that the caller has not received if
    trap reception is enabled.

Arguments:

    pEnterpriseOID - points to an object identifier that specifies the
        enterprise that generated the SNMP trap

    pAgentAddress - points to the address of the agent that generated the
        SNMP trap (retrieved from PDU).

    pGenericTrap - points to an indicator of the generic trap id.

    pSpecificTrap - points to an indicator of the specific trap id.

    pTimeStamp - points to a variable to receive the time stamp.

    pVarBindList - points to the associated variable bindings.

Return Values:

    Returns true if successful.

--*/

{
    // forward to new api
    return SnmpMgrGetTrapEx(
                pEnterpriseOID,
                pAgentAddress,
                NULL,
                pGenericTrap,
                pSpecificTrap,
                NULL,
                pTimeStamp,
                pVarBindList
                );
}


BOOL
SNMP_FUNC_TYPE
SnmpMgrGetTrapEx(
    AsnObjectIdentifier * pEnterpriseOID,
    AsnNetworkAddress   * pAgentAddress,
    AsnNetworkAddress   * pSourceAddress,
    AsnInteger          * pGenericTrap,
    AsnInteger          * pSpecificTrap,
    AsnOctetString      * pCommunity,
    AsnTimeticks        * pTimeStamp,
    SnmpVarBindList     * pVarBindList
    )

/*++

Routine Description:

    Returns outstanding trap data that the caller has not received if
    trap reception is enabled.

Arguments:

    pEnterpriseOID - points to an object identifier that specifies the
        enterprise that generated the SNMP trap

    pAgentAddress - points to the address of the agent that generated the
        SNMP trap (retrieved from PDU).

    pSourceAddress - points to the address of the agent that generated the
        SNMP trap (retrieved from network transport).

    pGenericTrap - points to an indicator of the generic trap id.

    pSpecificTrap - points to an indicator of the specific trap id.

    pCommunity - points to structure to receive community string.

    pTimeStamp - points to a variable to receive the time stamp.

    pVarBindList - points to the associated variable bindings.

Return Values:

    Returns true if successful.

--*/

{
    BOOL fOk = FALSE;
    PLIST_ENTRY pLE = NULL;
    PTRAP_LIST_ENTRY pTLE = NULL;
    smiINT32 nLastError;

    // obtain exclusive access
    EnterCriticalSection(&g_GlobalLock);

    // make sure list has entries
    if (!IsListEmpty(&g_IncomingTraps)) {

        // remove first item from list
        pLE = RemoveHeadList(&g_IncomingTraps);

    } else {

        // check for trap thread failure
        nLastError = g_TrapSMS.nLastError;
    }

    // release exclusive access
    LeaveCriticalSection(&g_GlobalLock);

    // validate pointer
    if (pLE != NULL) {

        // retrieve pointer to trap list entry
        pTLE = CONTAINING_RECORD(pLE, TRAP_LIST_ENTRY, Link);

        // validate pointer
        if (pEnterpriseOID != NULL) {

            // manually copy enterprise oid
            *pEnterpriseOID = pTLE->EnterpriseOID;

            // re-initialize list entry
            pTLE->EnterpriseOID.ids = NULL;
            pTLE->EnterpriseOID.idLength = 0;
        }

        // validate pointer
        if (pCommunity != NULL) {

            // transfer string info
            *pCommunity = pTLE->Community;

            // re-initialize list entry
            pTLE->Community.length  = 0;
            pTLE->Community.stream  = NULL;
            pTLE->Community.dynamic = FALSE;
        }

        // validate pointer
        if (pVarBindList != NULL) {

            // transfer varbindlist
            *pVarBindList = pTLE->VarBindList;

            // re-initialize list entry
            pTLE->VarBindList.len  = 0;
            pTLE->VarBindList.list = NULL;
        }

        // validate pointer
        if (pAgentAddress != NULL) {

            // copy structure
            memcpy(pAgentAddress,
                   &pTLE->AgentAddress,
                   sizeof(pTLE->AgentAddress)
                   );
        }

        // validate pointer
        if (pSourceAddress != NULL) {

            // copy structure
            memcpy(pSourceAddress,
                   &pTLE->SourceAddress,
                   sizeof(pTLE->SourceAddress)
                   );
        }

        // validate pointer
        if (pGenericTrap != NULL) {

            // transfer generic trap info
            *pGenericTrap = pTLE->nGenericTrap;
        }

        // validate pointer
        if (pSpecificTrap != NULL) {

            // transfer generic trap info
            *pSpecificTrap = pTLE->nSpecificTrap;
        }

        // validate pointer
        if (pTimeStamp != NULL) {

            // transfer time info
            *pTimeStamp = pTLE->TimeStamp;
        }

        // release
        FreeTle(pTLE);

        // success
        fOk = TRUE;

    } else if (nLastError != NOERROR) {

        // indicate there was an thread error
        SetLastError(SNMP_MGMTAPI_TRAP_ERRORS);

    } else {

        // indicate there are no traps
        SetLastError(SNMP_MGMTAPI_NOTRAPS);
    }

    return fOk;
}


VOID
serverTrapThread(
    LPVOID pUnused
    )

/*++

Routine Description:

    Old thread procedure used by the SNMP Trap Service.

Arguments:

    pUnused - unused parameter.

Return Values:

    None.

--*/

{
    //
    // do nothing here...
    //
}

