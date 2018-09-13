/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mib.c

Abstract:

    Contains definition of LAN Manager MIB.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>

#include "mibfuncs.h"
#include "hash.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "mib.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

   // If an addition or deletion to the MIB is necessary, there are several
   // places in the code that must be checked and possibly changed.
   //
   // 1.  There are 4 constants that are used as indexes to the start of each
   //     group in the MIB.  These are defined in MIB.H and must be adjusted
   //     to reflect any changes that affect them.
   //
   // 2.  The last field in each MIB entry is used to point to the NEXT
   //     leaf variable or table root.  If an AGGREGATE is next in the MIB,
   //     this pointer should skip it, because an AGGREGATE can never be
   //     accessed.  The last variable in the MIB is NULL.  Using the constants
   //     defined in step 1 above provides some flexibility.
   //
   // 3.  Following the MIB is a table of TABLE pointers into the MIB.  These
   //     pointers must be updated to reflect any changes made to the MIB.
   //     Each entry should point to the variable immediately below the table
   //     root.  (ie The entry in the table for "Session Table" should point
   //     to the MIB variable { svSessionTable 1 } in the server group.)

   // The prefix to all of the LM mib names is 1.3.6.1.4.1.77.1
UINT OID_Prefix[] = { 1, 3, 6, 1, 4, 1, 77, 1 };
AsnObjectIdentifier MIB_OidPrefix = { sizeof OID_Prefix / sizeof(UINT),
                                      OID_Prefix };

   // OID definitions for MIB -- group partitions
UINT MIB_common_group[] = { 1 };
UINT MIB_server_group[] = { 2 };
UINT MIB_wksta_group[]  = { 3 };
UINT MIB_domain_group[] = { 4 };

   // OID definitions for MIB -- COMMON group
UINT MIB_comVersionMaj[]    = { 1, 1, 0 };
UINT MIB_comVersionMin[]    = { 1, 2, 0 };
UINT MIB_comType[]          = { 1, 3, 0 };
UINT MIB_comStatStart[]     = { 1, 4, 0 };
UINT MIB_comStatNumNetIOs[] = { 1, 5, 0 };
UINT MIB_comStatFiNetIOs[]  = { 1, 6, 0 };
UINT MIB_comStatFcNetIOs[]  = { 1, 7, 0 };

   // OID definitions for MIB -- SERVER group
UINT MIB_svDescription[]         = { 2, 1, 0 };
UINT MIB_svSvcNumber[]           = { 2, 2, 0 };
UINT MIB_svSvcTable[]            = { 2, 3 };
UINT MIB_svSvcEntry[]            = { 2, 3, 1 };
UINT MIB_svStatOpens[]           = { 2, 4, 0 };
UINT MIB_svStatDevOpens[]        = { 2, 5, 0 };
UINT MIB_svStatQueuedJobs[]      = { 2, 6, 0 };
UINT MIB_svStatSOpens[]          = { 2, 7, 0 };
UINT MIB_svStatErrorOuts[]       = { 2, 8, 0 };
UINT MIB_svStatPwErrors[]        = { 2, 9, 0 };
UINT MIB_svStatPermErrors[]      = { 2, 10, 0 };
UINT MIB_svStatSysErrors[]       = { 2, 11, 0 };
UINT MIB_svStatSentBytes[]       = { 2, 12, 0 };
UINT MIB_svStatRcvdBytes[]       = { 2, 13, 0 };
UINT MIB_svStatAvResponse[]      = { 2, 14, 0 };
UINT MIB_svSecurityMode[]        = { 2, 15, 0 };
UINT MIB_svUsers[]               = { 2, 16, 0 };
UINT MIB_svStatReqBufsNeeded[]   = { 2, 17, 0 };
UINT MIB_svStatBigBufsNeeded[]   = { 2, 18, 0 };
UINT MIB_svSessionNumber[]       = { 2, 19, 0 };
UINT MIB_svSessionTable[]        = { 2, 20 };
UINT MIB_svSessionEntry[]        = { 2, 20, 1 };
UINT MIB_svAutoDisconnects[]     = { 2, 21, 0 };
UINT MIB_svDisConTime[]          = { 2, 22, 0 };
UINT MIB_svAuditLogSize[]        = { 2, 23, 0 };
UINT MIB_svUserNumber[]          = { 2, 24, 0 };
UINT MIB_svUserTable[]           = { 2, 25 };
UINT MIB_svUserEntry[]           = { 2, 25, 1 };
UINT MIB_svShareNumber[]         = { 2, 26, 0 };
UINT MIB_svShareTable[]          = { 2, 27 };
UINT MIB_svShareEntry[]          = { 2, 27, 1 };
UINT MIB_svPrintQNumber[]        = { 2, 28, 0 };
UINT MIB_svPrintQTable[]         = { 2, 29 };
UINT MIB_svPrintQEntry[]         = { 2, 29, 1 };

   // OID definitions for MIB - WORKSTATION group
UINT MIB_wkstaStatSessStarts[] = { 3, 1, 0 };
UINT MIB_wkstaStatSessFails[]  = { 3, 2, 0 };
UINT MIB_wkstaStatUses[]       = { 3, 3, 0 };
UINT MIB_wkstaStatUseFails[]   = { 3, 4, 0 };
UINT MIB_wkstaStatAutoRecs[]   = { 3, 5, 0 };
UINT MIB_wkstaErrorLogSize[]   = { 3, 6, 0 };
UINT MIB_wkstaUseNumber[]      = { 3, 7, 0 };
UINT MIB_wkstaUseTable[]       = { 3, 8 };
UINT MIB_wkstaUseEntry[]       = { 3, 8, 1 };

   // OID definitions for MIB - DOMAIN group
UINT MIB_domPrimaryDomain[]     = { 4, 1, 0 };
UINT MIB_domLogonDomain[]       = { 4, 2, 0 };
UINT MIB_domOtherDomainNumber[] = { 4, 3, 0 };
UINT MIB_domOtherDomainTable[]  = { 4, 4 };
UINT MIB_domOtherDomainEntry[]  = { 4, 4, 1 };
UINT MIB_domServerNumber[]      = { 4, 5, 0 };
UINT MIB_domServerTable[]       = { 4, 6 };
UINT MIB_domServerEntry[]       = { 4, 6, 1 };
UINT MIB_domLogonNumber[]       = { 4, 7, 0 };
UINT MIB_domLogonTable[]        = { 4, 8 };
UINT MIB_domLogonEntry[]        = { 4, 8, 1 };


   // LAN Manager MIB definiton
MIB_ENTRY Mib[] = {

             // LAN MGR 2 Root

          { { 0, NULL }, MIB_AGGREGATE, // { lanmanager 1 }
            0, 0, FALSE,
            NULL, NULL, 0,
            &Mib[MIB_COM_START] },

             // COMMON group

          { { 1, MIB_common_group }, MIB_AGGREGATE, // { lanmgr-2 1 }
            0, 0, FALSE,
            NULL, NULL, 0,
            &Mib[MIB_COM_START] },

          { { 3, MIB_comVersionMaj }, ASN_OCTETSTRING, // { common 1 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMVERSIONMAJ,
            &Mib[MIB_COM_START+1] },
          { { 3, MIB_comVersionMin }, ASN_OCTETSTRING, // { common 2 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMVERSIONMIN,
            &Mib[MIB_COM_START+2] },
          { { 3, MIB_comType }, ASN_OCTETSTRING, // { common 3 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMTYPE,
            &Mib[MIB_COM_START+3] },
          { { 3, MIB_comStatStart }, ASN_INTEGER, // { common 4 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMSTATSTART,
            &Mib[MIB_COM_START+4] },
          { { 3, MIB_comStatNumNetIOs }, ASN_RFC1155_COUNTER, // { common 5 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMSTATNUMNETIOS,
            &Mib[MIB_COM_START+5] },
          { { 3, MIB_comStatFiNetIOs }, ASN_RFC1155_COUNTER, // { common 6 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMSTATFINETIOS,
            &Mib[MIB_COM_START+6] },
          { { 3, MIB_comStatFcNetIOs }, ASN_RFC1155_COUNTER, // { common 7 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_common_func, MIB_leaf_func, MIB_LM_COMSTATFCNETIOS,
            &Mib[MIB_SV_START] },

             // SERVER group

          { { 1, MIB_server_group }, MIB_AGGREGATE, // { lanmgr-2 2 }
            0, 0, FALSE,
            NULL, NULL, 0,
            &Mib[MIB_SV_START] },

          { { 3, MIB_svDescription }, ASN_RFC1213_DISPSTRING, // { Server 1 }
            MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVDESCRIPTION,
            &Mib[MIB_SV_START+1] },
          { { 3, MIB_svSvcNumber }, ASN_INTEGER, // { Server 2 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSVCNUMBER,
            &Mib[MIB_SV_START+3] },
          { { 2, MIB_svSvcTable }, MIB_AGGREGATE, // { Server 3 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_SVSVCTABLE,
            &Mib[MIB_SV_START+3] },
          { { 3, MIB_svSvcEntry }, MIB_TABLE, // { svSvcTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_srvcs_func, MIB_LM_SVSVCENTRY,
            &Mib[MIB_SV_START+4] },
          { { 3, MIB_svStatOpens }, ASN_RFC1155_COUNTER, // { server 4 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATOPENS,
            &Mib[MIB_SV_START+5] },
          { { 3, MIB_svStatDevOpens }, ASN_RFC1155_COUNTER, // { server 5 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATDEVOPENS,
            &Mib[MIB_SV_START+6] },
          { { 3, MIB_svStatQueuedJobs }, ASN_RFC1155_COUNTER, // { server 6 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATQUEUEDJOBS,
            &Mib[MIB_SV_START+7] },
          { { 3, MIB_svStatSOpens }, ASN_RFC1155_COUNTER, // { server 7 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATSOPENS,
            &Mib[MIB_SV_START+8] },
          { { 3, MIB_svStatErrorOuts }, ASN_RFC1155_COUNTER, // { server 8 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATERROROUTS,
            &Mib[MIB_SV_START+9] },
          { { 3, MIB_svStatPwErrors }, ASN_RFC1155_COUNTER, // { server 9 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATPWERRORS,
            &Mib[MIB_SV_START+10] },
          { { 3, MIB_svStatPermErrors }, ASN_RFC1155_COUNTER, // { server 10 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATPERMERRORS,
            &Mib[MIB_SV_START+11] },
          { { 3, MIB_svStatSysErrors }, ASN_RFC1155_COUNTER, // { server 11 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATSYSERRORS,
            &Mib[MIB_SV_START+12] },
          { { 3, MIB_svStatSentBytes }, ASN_RFC1155_COUNTER, // { server 12 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATSENTBYTES,
            &Mib[MIB_SV_START+13] },
          { { 3, MIB_svStatRcvdBytes }, ASN_RFC1155_COUNTER, // { server 13 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATRCVDBYTES,
            &Mib[MIB_SV_START+14] },
          { { 3, MIB_svStatAvResponse }, ASN_INTEGER, // { server 14 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATAVRESPONSE,
            &Mib[MIB_SV_START+15] },
          { { 3, MIB_svSecurityMode }, ASN_INTEGER, // { server 15 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSECURITYMODE,
            &Mib[MIB_SV_START+16] },
          { { 3, MIB_svUsers }, ASN_INTEGER, // { server 16 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVUSERS,
            &Mib[MIB_SV_START+17] },
          { { 3, MIB_svStatReqBufsNeeded }, ASN_RFC1155_COUNTER, // { server 17}
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATREQBUFSNEEDED,
            &Mib[MIB_SV_START+18] },
          { { 3, MIB_svStatBigBufsNeeded }, ASN_RFC1155_COUNTER, // { server 18}
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSTATBIGBUFSNEEDED,
            &Mib[MIB_SV_START+19] },
          { { 3, MIB_svSessionNumber }, ASN_INTEGER, // { server 19 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSESSIONNUMBER,
            &Mib[MIB_SV_START+21] },
          { { 2, MIB_svSessionTable }, MIB_AGGREGATE, // { server 20 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_SVSESSIONTABLE,
            &Mib[MIB_SV_START+21] },
          { { 3, MIB_svSessionEntry }, MIB_TABLE, // { svSessionTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_sess_func, MIB_LM_SVSESSIONENTRY,
            &Mib[MIB_SV_START+22] },
          { { 3, MIB_svAutoDisconnects }, ASN_INTEGER, // { server 21 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVAUTODISCONNECTS,
            &Mib[MIB_SV_START+23] },
          { { 3, MIB_svDisConTime }, ASN_INTEGER, // { server 22 }
            MIB_ACCESS_READWRITE, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVDISCONTIME,
#if 0
            &Mib[MIB_SV_START+24] },
#else
            &Mib[MIB_SV_START+25] },
#endif
          { { 3, MIB_svAuditLogSize }, ASN_INTEGER, // { server 23 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVAUDITLOGSIZE,
            &Mib[MIB_SV_START+25] },
          { { 3, MIB_svUserNumber }, ASN_INTEGER, // { server 24 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVUSERNUMBER,
            &Mib[MIB_SV_START+27] },
          { { 2, MIB_svUserTable }, MIB_AGGREGATE, // { server 25 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_SVUSERTABLE,
            &Mib[MIB_SV_START+27] },
          { { 3, MIB_svUserEntry }, MIB_TABLE, // { svUserTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_users_func, MIB_LM_SVUSERENTRY,
            &Mib[MIB_SV_START+28] },
          { { 3, MIB_svShareNumber }, ASN_INTEGER, // { server 26 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVSHARENUMBER,
            &Mib[MIB_SV_START+30] },
          { { 2, MIB_svShareTable }, MIB_AGGREGATE, // { server 27 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_SVSHARETABLE,
            &Mib[MIB_SV_START+30] },
          { { 3, MIB_svShareEntry }, MIB_TABLE, // { svShareTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_shares_func, MIB_LM_SVSHAREENTRY,
            &Mib[MIB_SV_START+31] },
          { { 3, MIB_svPrintQNumber }, ASN_INTEGER, // { server 28 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_server_func, MIB_leaf_func, MIB_LM_SVPRINTQNUMBER,
            &Mib[MIB_SV_START+33] },
          { { 2, MIB_svPrintQTable }, MIB_AGGREGATE, // { server 29 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_SVPRINTQTABLE,
            &Mib[MIB_SV_START+33] },
          { { 3, MIB_svPrintQEntry }, MIB_TABLE, // { svPrintQTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_prntq_func, MIB_LM_SVPRINTQENTRY,
            &Mib[MIB_WKSTA_START] },

          // WORKSTATION group

          { { 1, MIB_wksta_group }, MIB_AGGREGATE, // { lanmgr-2 3 }
            0, 0, FALSE,
            NULL, NULL, 0,
            &Mib[MIB_WKSTA_START] },

          { { 3, MIB_wkstaStatSessStarts }, ASN_RFC1155_COUNTER, // { wrksta 1 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTASTATSESSSTARTS,
            &Mib[MIB_WKSTA_START+1] },
          { { 3, MIB_wkstaStatSessFails }, ASN_RFC1155_COUNTER, // { wrksta 2 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTASTATSESSFAILS,
            &Mib[MIB_WKSTA_START+2] },
          { { 3, MIB_wkstaStatUses }, ASN_RFC1155_COUNTER, // { wrksta 3 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTASTATUSES,
            &Mib[MIB_WKSTA_START+3] },
          { { 3, MIB_wkstaStatUseFails }, ASN_RFC1155_COUNTER, // { wrksta 4 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTASTATUSEFAILS,
            &Mib[MIB_WKSTA_START+4] },
          { { 3, MIB_wkstaStatAutoRecs }, ASN_RFC1155_COUNTER, // { wrksta 5 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTASTATAUTORECS,
#if 0
            &Mib[MIB_WKSTA_START+5] },
#else
            &Mib[MIB_WKSTA_START+6] },
#endif
          { { 3, MIB_wkstaErrorLogSize }, ASN_INTEGER, // { wrksta 6 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTAERRORLOGSIZE,
            &Mib[MIB_WKSTA_START+6] },
          { { 3, MIB_wkstaUseNumber }, ASN_INTEGER, // { wrksta 7 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_workstation_func, MIB_leaf_func, MIB_LM_WKSTAUSENUMBER,
            &Mib[MIB_WKSTA_START+8] },
          { { 2, MIB_wkstaUseTable }, MIB_AGGREGATE, // { wrksta 8 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_WKSTAUSETABLE,
            &Mib[MIB_WKSTA_START+8] },
          { { 3, MIB_wkstaUseEntry }, MIB_TABLE, // { wrkstaUseTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_wsuses_func, MIB_LM_WKSTAUSEENTRY,
            &Mib[MIB_DOM_START] },

             // DOMAIN group

          { { 1, MIB_domain_group }, MIB_AGGREGATE, // { lanmgr-2 4 }
            0, 0, FALSE,
            NULL, NULL, 0,
            &Mib[MIB_DOM_START] },

          { { 3, MIB_domPrimaryDomain }, ASN_RFC1213_DISPSTRING, // { domain 1 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_domain_func, MIB_leaf_func, MIB_LM_DOMPRIMARYDOMAIN,
#if 0
            &Mib[MIB_DOM_START+1] },
#else
            NULL },
#endif
          { { 3, MIB_domLogonDomain }, ASN_RFC1213_DISPSTRING, // { domain 2 }
#if 0
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
#else
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, TRUE,
#endif
            MIB_domain_func, MIB_leaf_func, MIB_LM_DOMLOGONDOMAIN,
            &Mib[MIB_DOM_START+2] },
          { { 3, MIB_domOtherDomainNumber }, ASN_INTEGER, // { domain 3 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_domain_func, MIB_leaf_func, MIB_LM_DOMOTHERDOMAINNUMBER,
            &Mib[MIB_DOM_START+4] },
          { { 2, MIB_domOtherDomainTable }, MIB_AGGREGATE, // { domain 4 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_DOMOTHERDOMAINTABLE,
            &Mib[MIB_DOM_START+4] },
          { { 3, MIB_domOtherDomainEntry }, MIB_TABLE, // { domOtherDomTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_odoms_func, MIB_LM_DOMOTHERDOMAINENTRY,
            &Mib[MIB_DOM_START+5] },
          { { 3, MIB_domServerNumber }, ASN_INTEGER, // { domain 5 }
            MIB_ACCESS_READ, MIB_STATUS_MANDATORY, TRUE,
            MIB_domain_func, MIB_leaf_func, MIB_LM_DOMSERVERNUMBER,
            &Mib[MIB_DOM_START+7] },
          { { 2, MIB_domServerTable }, MIB_AGGREGATE, // { domain 6 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, NULL, MIB_LM_DOMSERVERTABLE,
            &Mib[MIB_DOM_START+7] },
          { { 3, MIB_domServerEntry }, MIB_TABLE, // { domServerTable 1 }
            MIB_ACCESS_NOT, MIB_STATUS_MANDATORY, FALSE,
            NULL, MIB_svsond_func, MIB_LM_DOMSERVERENTRY,
            NULL }
          };
UINT MIB_num_variables = sizeof Mib / sizeof( MIB_ENTRY );


//
// List of table pointers - References must agree with MIB
//
MIB_ENTRY *MIB_Tables[] = {
             &Mib[MIB_SV_START+3],     // Service
             &Mib[MIB_SV_START+21],    // Session
             &Mib[MIB_SV_START+27],    // User
             &Mib[MIB_SV_START+30],    // Share
             &Mib[MIB_SV_START+33],    // Print Queue
             &Mib[MIB_WKSTA_START+8],  // Uses
             &Mib[MIB_DOM_START+4],    // Other domain
             &Mib[MIB_DOM_START+7]     // Server
             };
UINT MIB_table_list_size = sizeof MIB_Tables / sizeof( MIB_ENTRY * );

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// MIB_get_entry
//    Lookup OID in MIB, and return pointer to its entry.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    SNMP_MIB_UNKNOWN_OID
//
MIB_ENTRY *MIB_get_entry(
              IN AsnObjectIdentifier *Oid
              )

{
AsnObjectIdentifier TempOid;
UINT                I;
MIB_ENTRY           *pResult;


   // Check prefix
   if ( SnmpUtilOidNCmp(&MIB_OidPrefix, Oid, MIB_PREFIX_LEN) )
      {
      pResult = NULL;
      goto Exit;
      }

   // Strip prefix by placing in temp
   TempOid.idLength = Oid->idLength - MIB_PREFIX_LEN;
   TempOid.ids      = &Oid->ids[MIB_PREFIX_LEN];

   // Get pointer into MIB
   pResult = MIB_HashLookup( &TempOid );

   // Check for possible table entry
   if ( pResult == NULL )
      {
      for ( I=0;I < MIB_table_list_size;I++ )
         {
         if ( !SnmpUtilOidNCmp(&TempOid, &MIB_Tables[I]->Oid,
                            MIB_Tables[I]->Oid.idLength) )
            {
            pResult = MIB_Tables[I];
            goto Exit;
            }
         }
      }

Exit:
   return pResult;
} // MIB_get_entry



//
// MakeOidFromStr
//    Makes an OID out of string so a table can be indexed.
//
// Notes:
//
// Return Codes:
//
// Error Codes:
//    None.
//
SNMPAPI MakeOidFromStr(
           IN AsnDisplayString *Str,    // String to make OID
           OUT AsnObjectIdentifier *Oid // Resulting OID
           )

{
UINT    I;
SNMPAPI nResult;


   if ( NULL == (Oid->ids = SnmpUtilMemAlloc((Str->length+1) * sizeof(UINT))) )
      {
      nResult = SNMP_MEM_ALLOC_ERROR;
      goto Exit;
      }

   // Place length as first OID sub-id
   Oid->ids[0] = Str->length;

   // Place each character of string as sub-id
   for ( I=0;I < Str->length;I++ )
      {
      Oid->ids[I+1] = Str->stream[I];
      }

   Oid->idLength = Str->length + 1;


Exit:
   return nResult;
} // MakeOidFromStr

//-------------------------------- END --------------------------------------
