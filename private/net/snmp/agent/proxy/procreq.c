/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    procreq.c

Abstract:

    Provides SNMP message dispatch/processing functionality for Proxy Agent.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <windows.h>

#include <winsock.h>
#include <wsipx.h>

#include <errno.h>
#include <stdio.h>
#include <process.h>
#include <string.h>


//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>
#include "..\common\wellknow.h"

#include "..\authapi.\berapi.h"
#include "..\authapi.\pduapi.h"
#include "..\authapi.\auth1157.h"
#include "..\authapi.\authxxxx.h"
#include "..\authapi.\authapi.h"

#include "regconf.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

extern DWORD platformId;


//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SGTTimeout ((DWORD)3600000)


//--------------------------- PRIVATE STRUCTS -------------------------------

typedef struct _RFC1157VarBindXlat {

    UINT view;  // view associated with varbind
    UINT index; // varbind placement in incoming pdu

} RFC1157VarBindXlat;

typedef struct _SnmpMgmtQuery {

    RFC1157VarBindList  vbl;  // list of varbinds for extension agent
    RFC1157VarBindXlat *xlat; // information to reorder varbinds
    FARPROC             addr; // address of extension agent

} SnmpMgmtQuery; 

typedef struct _SnmpMgmtQueryList {

    SnmpMgmtQuery *query; // list of subagent queries
    UINT           len;   // number of queries in list
    UINT           type;  // type of query (get, set, or getnext)

} SnmpMgmtQueryList;

//--------------------------- PRIVATE VARIABLES -----------------------------

static UINT *vl    = NULL; // list of extension agent views
static UINT  vlLen = 0;    // length of the above view list

//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL SnmpSvcAddrToSocket(
    LPSTR addrText,
    struct sockaddr *addrEncoding);

int gethostname(OUT char *,IN int );
void dp_ipx(int, char *, SOCKADDR_IPX *, char *);

void initvl();

void vbltoql(RFC1157VarBindList *, SnmpMgmtQueryList *, UINT *, UINT *);
void qltovbl(SnmpMgmtQueryList *, RFC1157VarBindList *, UINT *, UINT *);

void addql(SnmpMgmtQueryList *, RFC1157VarBindList *, UINT, UINT);
void delql(SnmpMgmtQueryList *);

void addq(SnmpMgmtQuery *, RFC1157VarBind *, UINT, UINT);

void fixupql(SnmpMgmtQueryList *, UINT *, UINT *);
void nextvb(SnmpMgmtQuery *, UINT, UINT, UINT *, UINT *);

void fixuperr(SnmpMgmtQuery *, UINT *);

//--------------------------- PRIVATE PROCEDURES ----------------------------

#define bzero(lp, size)         (void)memset(lp, 0, size)

void initvl()
    {
    UINT i;
    UINT j;
    UINT temp;

    // allocate an index for each registered subagent 
    vl = (INT *)SnmpUtilMemAlloc(extAgentsLen * sizeof(INT));

    // flag only if properly initialized 
    for(i=0, vlLen=0; i < (UINT)extAgentsLen; i++)
        {
        if (extAgents[i].fInitedOk)
            {
            vl[vlLen++] = i;
            }
        }

    // sort these indexes...
    for(i=0; i < vlLen; i++)
        {
        for(j=i + 1; j < vlLen; j++)
            {
            // if item[i] > item[j]
            if (0 < SnmpUtilOidCmp(
                    &(extAgents[vl[i]].supportedView),
                    &(extAgents[vl[j]].supportedView)))
                {
                temp  = vl[i];
                vl[i] = vl[j];
                vl[j] = temp;
                }
            }
        }

    } // end initvl()


void vbltoql(RFC1157VarBindList *vbl, SnmpMgmtQueryList *ql, UINT *errorStatus, UINT *errorIndex)
    {
    UINT v;  // index into view list
    UINT vb; // index into varbind list

    INT  nDiff;
    
    BOOL fAnyOk;
    BOOL fFoundOk = FALSE;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: processing %s request containing %d variable(s).\n",
        (ql->type == ASN_RFC1157_GETREQUEST)
            ? "get"
            : (ql->type == ASN_RFC1157_SETREQUEST)
                ? "set"
                : (ql->type == ASN_RFC1157_GETNEXTREQUEST)
                    ? "getnext"
                    : "unknown", vbl->len));

    // check to see if getnext is being requested
    fAnyOk = (ql->type == ASN_RFC1157_GETNEXTREQUEST);

    // initialize status return values
    *errorStatus = SNMP_ERRORSTATUS_NOERROR;
    *errorIndex  = 0;

    // process variable bindings
    for (vb=0; vb < vbl->len; vb++)
        {
        // process supported views
        for (v=0; v < vlLen; v++)
            {
            // compare request
            nDiff = SnmpUtilOidNCmp(
                        &vbl->list[vb].name,
                        &extAgents[vl[v]].supportedView,
                        extAgents[vl[v]].supportedView.idLength
                        );

            // analyze results based on request type
            fFoundOk = (!nDiff || (fAnyOk && (nDiff < 0)));

            // save
            if (fFoundOk)
                {
                // insert into query
                addql(ql, vbl, vb, v);
                break;
                }
            }

        // not found
        if (!fFoundOk)
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: %s not in supported view(s).\n", SnmpUtilOidToA(&vbl->list[vb].name)));
            *errorStatus = SNMP_ERRORSTATUS_NOSUCHNAME;
            *errorIndex  = vb+1;
            break;
            }
        }

    } // end vbltoql()


void addql(SnmpMgmtQueryList *ql, RFC1157VarBindList *vbl, UINT vb, UINT v)
    {
    UINT q; // index into query list

    // make sure that the type is correct
    if ((ql->type != ASN_RFC1157_SETREQUEST) && 
        (vbl->list[vb].value.asnType != ASN_NULL)) {
        
        SNMPDBG((
            SNMP_LOG_TRACE,
            "SNMP: PDU: forcing asnType to NULL (asnType=%d).\n",
            (DWORD)(BYTE)vbl->list[vb].value.asnType
            ));

        // force the asn type to be null
        vbl->list[vb].value.asnType = ASN_NULL;        
    }     

    // process existing queries
    for (q=0; q < ql->len; q++)
        {
        // compare existing extension agent addresses 
        if (ql->query[q].addr == extAgents[vl[v]].queryAddr)
            {
            // add to existing query
            addq(&ql->query[q], &vbl->list[vb], vb, v);
            return;
            }
        }

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: creating query for %s (0x%08lx).\n", extAgents[vl[v]].pathName, extAgents[vl[v]].queryAddr));

    ql->len++; // add new query to end of list
    ql->query = (SnmpMgmtQuery *)SnmpUtilMemReAlloc(ql->query, (ql->len * sizeof(SnmpMgmtQuery)));

    ql->query[q].xlat = NULL;
    ql->query[q].addr = extAgents[vl[v]].queryAddr;

    ql->query[q].vbl.len = 0;
    ql->query[q].vbl.list = NULL;

    // add to newly created query
    addq(&ql->query[q], &vbl->list[vb], vb, v);

    } // end addql()


void addq(SnmpMgmtQuery *q, RFC1157VarBind *vb, UINT i, UINT v)
    {

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: adding variable %d to query 0x%08lx (%s).\n", i+1, q->addr, SnmpUtilOidToA(&vb->name)));

    q->vbl.len++; // add varbind to end of list
    q->vbl.list = (RFC1157VarBind *)SnmpUtilMemReAlloc(q->vbl.list, (q->vbl.len * sizeof(RFC1157VarBind)));
    q->xlat = (RFC1157VarBindXlat *)SnmpUtilMemReAlloc(q->xlat, (q->vbl.len * sizeof(RFC1157VarBindXlat)));

    q->xlat[q->vbl.len-1].view  = v;
    q->xlat[q->vbl.len-1].index = i;

    SnmpUtilVarBindCpy(&q->vbl.list[q->vbl.len-1], vb);

    } // end addq()


void delql(SnmpMgmtQueryList *ql)
    {
    UINT q; // index into query list

    // process queries
    for (q=0; q < ql->len; q++)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: deleting query 0x%08lx.\n", ql->query[q].addr));

        // free translation info
        SnmpUtilMemFree(ql->query[q].xlat);
        // free query varbind list (and variables)
        SnmpUtilVarBindListFree(&ql->query[q].vbl);
        }

    // free query list
    SnmpUtilMemFree(ql->query);

    } // end delql()


void qltovbl(SnmpMgmtQueryList *ql, RFC1157VarBindList *vbl, UINT *errorStatus, UINT *errorIndex)
    {
    UINT q;   // index into queue list
    UINT vb;  // index into queue varbind list 
    UINT i;   // index into original varbind list 

    // only convert back if error not reported
    if (*errorStatus != SNMP_ERRORSTATUS_NOERROR)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: request failed, errorStatus=%s, errorIndex=%d.\n",
            (*errorStatus == SNMP_ERRORSTATUS_NOSUCHNAME)
                ? "NOSUCHNAME"
                : (*errorStatus == SNMP_ERRORSTATUS_BADVALUE)
                    ? "BADVALUE"
                    : (*errorStatus == SNMP_ERRORSTATUS_READONLY)
                        ? "READONLY"
                        : (*errorStatus == SNMP_ERRORSTATUS_TOOBIG)
                            ? "TOOBIG"
                            : "GENERR", *errorIndex
                            ));
        // free
        delql(ql);
        return;
        }

    // process queries
    for (q=0; q < ql->len; q++)
        {
        // process variable bindings gathered
        for (vb=0; vb < ql->query[q].vbl.len; vb++)
            {
            // calculate original index
            i = ql->query[q].xlat[vb].index;
            // free original variable
            SnmpUtilVarBindFree(&vbl->list[i]);
            // replace with new variable
            vbl->list[i] = ql->query[q].vbl.list[vb];

            SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: updating variable %d from query 0x%08lx (%s).\n", i+1, ql->query[q].addr, SnmpUtilOidToA(&vbl->list[i].name)));
            }

        // free translation info
        SnmpUtilMemFree(ql->query[q].xlat);
        // free query varbind list 
        SnmpUtilMemFree(ql->query[q].vbl.list);
        }

    // free query list
    SnmpUtilMemFree(ql->query);

    } // end qltovbl()


void fixupql(SnmpMgmtQueryList *ql, UINT *errorStatus, UINT *errorIndex)
    {
    UINT v;   // index into view list 
    UINT q;   // index into queue list
    UINT vb;  // index into varbind list 

    // process queries
    for (q=0; (q < ql->len) && !(*errorStatus); q++)
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: validating query 0x%08lx.\n", ql->query[q].addr));

        // process variable bindings gathered
        for (vb=0; (vb < ql->query[q].vbl.len) && !(*errorStatus); vb++)
            {
            // calculate view index
            v = ql->query[q].xlat[vb].view;
            // check oid returned from subagent
            if (0 < SnmpUtilOidNCmp(
                        &ql->query[q].vbl.list[vb].name,
                        &extAgents[vl[v]].supportedView,
                        extAgents[vl[v]].supportedView.idLength
                        ))
                {
                // retry getnext using next view 
                nextvb(&ql->query[q], vb, v, errorStatus, errorIndex); 
                }
            }
        }

    } // end fixupql()


void nextvb(SnmpMgmtQuery *q, UINT vb, UINT v, UINT *errorStatus, UINT *errorIndex)
    {
    RFC1157VarBindList vbl;

    // process single varbind
    vbl.list = &q->vbl.list[vb];
    vbl.len  = 1;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: %s no longer in view.\n", SnmpUtilOidToA(&vbl.list[0].name)));

    // process remaining views
    while ((++v < vlLen) && !(*errorStatus))
        {
        // trim oid if necessary
        vbl.list[0].name.idLength = min(
            vbl.list[0].name.idLength,
            extAgents[vl[v]].supportedView.idLength
            );

        // validate order
        if (0 >= SnmpUtilOidNCmp(
                    &vbl.list[0].name,
                    &extAgents[vl[v]].supportedView,
                    extAgents[vl[v]].supportedView.idLength
                    ))
            {
            SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: re-routing request to %s.\n", extAgents[vl[v]].pathName));

            // send getnext query to subagent
            if ((*(extAgents[vl[v]].queryAddr))(
                    ASN_RFC1157_GETNEXTREQUEST,
                    &vbl,
                    errorStatus,
                    errorIndex
                    ))
                {
                // check the subagent status code returned
                if (*errorStatus == SNMP_ERRORSTATUS_NOERROR)
                    {
                    // check oid returned from subagent
                    if (0 >= SnmpUtilOidNCmp(
                                &vbl.list[0].name,
                                &extAgents[vl[v]].supportedView,
                                extAgents[vl[v]].supportedView.idLength
                                ))
                        {
                        return; // success...
                        }
                    }
                }
            else
                {
                // subagent unable to process query
                *errorStatus = SNMP_ERRORSTATUS_GENERR;
                }
            }
        }

    *errorStatus = *errorStatus ? *errorStatus : SNMP_ERRORSTATUS_NOSUCHNAME;
    *errorIndex  = q->xlat[vb].index+1;

    SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: %s not in supported view(s).\n", SnmpUtilOidToA(&vbl.list[0].name)));

    } // end nextvb()


void fixuperr(SnmpMgmtQuery *q, UINT *errorIndex)
    {
    UINT errorIndexOld = *errorIndex;

    // ignore zero
    if (errorIndexOld)
        {    
        // make sure within bounds
        if (errorIndexOld <= q->vbl.len)
            {
            *errorIndex = q->xlat[errorIndexOld-1].index+1;
            }
        else 
            {
            // default to first variable
            *errorIndex = q->xlat[0].index+1;
            }
        }

    } // end fixuperr()

//--------------------------- PUBLIC PROCEDURES -----------------------------

SNMPAPI SnmpServiceProcessMessage(
    IN OUT BYTE **pBuf,
    IN OUT UINT *length)
    {
    static BOOL fInitedOk = FALSE;

    RFC1157VarBindList vbl;
    SnmpMgmtQueryList  ql;
    SnmpMgmtCom        request;

    AsnInteger errorStatus;
    AsnInteger errorIndex;

    BOOL fEncodedOk;

    UINT packetType;
    UINT q;

    // init views
    if (!fInitedOk)
        {
        initvl();
        fInitedOk = TRUE;
        }

    // decode received request into a management comm
    if (!SnmpSvcDecodeMessage(&packetType, &request, *pBuf, *length, TRUE))
        return FALSE;

    // initialize variables
    vbl = request.pdu.pduValue.pdu.varBinds;

    ql.query = NULL;
    ql.len   = 0;
    ql.type  = request.pdu.pduType;

    // disassemble varbinds into queries
    vbltoql(&vbl, &ql, &errorStatus, &errorIndex);

    // process list of individual queries    
    for (q=0; (q < ql.len) && !errorStatus; q++ )
        {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: processing query 0x%08lx.\n", ql.query[q].addr));

        // send query to subagent
        if ((*(ql.query[q].addr))(
                  ql.type, 
                  &ql.query[q].vbl, 
                  &errorStatus, 
                  &errorIndex
                  ))
            {
            // check the subagent status code returned
            if (errorStatus != SNMP_ERRORSTATUS_NOERROR)
                {
                // adjust index to match request pdu
                fixuperr(&ql.query[q], &errorIndex);
                }
            }
        else 
            {
            // subagent unable to process query
            errorStatus = SNMP_ERRORSTATUS_GENERR;
            errorIndex  = 1;
            // adjust index to match request pdu
            fixuperr(&ql.query[q], &errorIndex);
            }
        }

    // make sure queries ready for conversion
    if (errorStatus == SNMP_ERRORSTATUS_NOERROR)
        {
        // special processing needed for getnext 
        if (ql.type == ASN_RFC1157_GETNEXTREQUEST)
            {
            // returned oids must match agent view
            fixupql(&ql, &errorStatus, &errorIndex);
            }
        }

    // reassemble queries into response varbinds
    qltovbl(&ql, &vbl, &errorStatus, &errorIndex);  

    // construct reponse pdu with varbinds
    request.pdu.pduType = ASN_RFC1157_GETRESPONSE;

    request.pdu.pduValue.pdu.errorStatus = errorStatus;
    request.pdu.pduValue.pdu.errorIndex  = errorIndex;

    request.pdu.pduValue.pdu.varBinds = vbl;

    *pBuf   = NULL;
    *length = 0;

    // encode response pdu with gathered varbinds
    fEncodedOk = SnmpSvcEncodeMessage(packetType, &request, pBuf, length);

    // release response message
    SnmpSvcReleaseMessage(&request);

    return fEncodedOk;

    } // end SnmpServiceProcessMessage()

// filter managers with permitted managers in registry
BOOL filtmgrs(struct sockaddr *source, INT sourceLen)
{
    BOOL fFound = FALSE;
    INT  i;

    if (permitMgrsLen > 0)
    {
        for(i=0; i < permitMgrsLen && !fFound; i++)
        {
            DWORD   test;
            SOCKADDR_IPX *  pIpx;
// --------- BEGIN: PROTOCOL SPECIFIC SOCKET CODE BEGIN... ---------
            switch (source->sa_family)
            {
                case AF_INET:
                if ((*((struct sockaddr_in *)source)).sin_addr.s_addr ==
                    (*((struct sockaddr_in *)&permitMgrs[i].addrEncoding)).sin_addr.s_addr)
                {
                    fFound = TRUE;
                }
                break;

                case AF_IPX:

#ifdef debug
                dp_ipx(SNMP_LOG_TRACE, "SNMP: PDU: validating IPX manager @ ",
                       (SOCKADDR_IPX *) source, " against ");
                SNMPDBG((SNMP_LOG_TRACE, "(%04X)", permitMgrs[i].addrEncoding.sa_family));
                dp_ipx(SNMP_LOG_TRACE, "", (SOCKADDR_IPX*) &permitMgrs[i].addrEncoding, "\n");
#endif
                pIpx = (SOCKADDR_IPX *) &permitMgrs[i].addrEncoding;
                test = *(DWORD *)((SOCKADDR_IPX *)(&permitMgrs[i].addrEncoding))->sa_netnum;
                //Below checks for nodenumber regardless of netnumber if the user specified
                // a netnumber of zero for the permitted IPX mgr
                if (*(DWORD *)((SOCKADDR_IPX *)(&permitMgrs[i].addrEncoding))->sa_netnum == 0)
                {
                    if (memcmp(((SOCKADDR_IPX *)source)->sa_nodenum, 
                           ((SOCKADDR_IPX *)(&permitMgrs[i].addrEncoding))->sa_nodenum, 
                           sizeof(((SOCKADDR_IPX *)source)->sa_nodenum)) == 0)
                    {
                        fFound = TRUE;
                    }
                }
                else
                {
                    //Compare the entire address
                    if (memcmp(source, 
                           &permitMgrs[i].addrEncoding, 
                           sizeof(((SOCKADDR_IPX *)source)->sa_netnum) +
                           sizeof(((SOCKADDR_IPX *)source)->sa_nodenum)) == 0)
                    {
                        fFound = TRUE;
                    }
                }
// --------- END: PROTOCOL SPECIFIC SOCKET CODE END. ---------------
            } // end switch   
        } // end for()
    }
    else
    {
        fFound = TRUE; // no entries means all managers allowed
    } // end if

    if (!fFound)
    {
        SNMPDBG((SNMP_LOG_TRACE, "SNMP: PDU: invalid manager filtered.\n"));
    }

    return fFound;

} // end filtmgrs()


//-------------------------------- END --------------------------------------

// display IPX address in 00000001.123456789ABC form

void dp_ipx(int level, char *msg1, SOCKADDR_IPX* addr, char *msg2)
    {
    SNMPDBG((level, "%s%02X%02X%02X%02X.%02X%02X%02X%02X%02X%02X%s",
              msg1,
              (unsigned char)addr->sa_netnum[0],
              (unsigned char)addr->sa_netnum[1],
              (unsigned char)addr->sa_netnum[2],
              (unsigned char)addr->sa_netnum[3],
              (unsigned char)addr->sa_nodenum[0],
              (unsigned char)addr->sa_nodenum[1],
              (unsigned char)addr->sa_nodenum[2],
              (unsigned char)addr->sa_nodenum[3],
              (unsigned char)addr->sa_nodenum[4],
              (unsigned char)addr->sa_nodenum[5],
              msg2));
    }
