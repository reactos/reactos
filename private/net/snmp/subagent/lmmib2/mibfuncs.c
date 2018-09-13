/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    mibfuncs.c

Abstract:

    Contains MIB functions for GET's and SET's for LM MIB.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#ifdef DOS
#if 0
#define INCL_NETWKSTA
#define INCL_NETERRORS
#include <lan.h>
#endif
#endif

#ifdef WIN32
#include <windows.h>
#include <lm.h>
#endif
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>
//---ft:07/13---LsaGetUserName substitutes NetWkstaGetUserInfo
#include <subauth.h>    // needed for the definition of PUNICODE_STRING
#include <ntlsa.h>      // LSA APIs
//---tf---
#include "mib.h"
#include "lmcache.h"


//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "mibfuncs.h"
#include "odom_tbl.h"
#include "user_tbl.h"
#include "shar_tbl.h"
#include "srvr_tbl.h"
#include "prnt_tbl.h"
#include "uses_tbl.h"


//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define SafeBufferFree(x)       if(NULL != x) NetApiBufferFree( x )

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

void  * MIB_common_func(
           IN UINT Action,  // Action to perform on Data
           IN LDATA LMData, // LM Data to manipulate
           IN void *SetData
           )

{
SNMPAPI nResult;
unsigned lmCode;
WKSTA_INFO_101 *wksta_info_one;
SERVER_INFO_102 *server_info_two;
STAT_SERVER_0 *server_stats_zero;
STAT_WORKSTATION_0 *wrk_stats_zero;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
BYTE *stream;
char temp[80];
BOOL cache_it ;
time_t curr_time ;

UNREFERENCED_PARAMETER(SetData);

   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {
            case MIB_LM_COMVERSIONMAJ:

              if((NULL == cache_table[C_NETWKSTAGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETWKSTAGETINFO].bufptr ) ;
                //
                lmCode =
                NetWkstaGetInfo( NULL,                  // local server
                                101,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
                       _itoa(wksta_info_one->wki101_ver_major,temp,10) ;
                       if(NULL ==
                        (stream = SnmpUtilMemAlloc( strlen(temp) ))
                       )  {
                          SnmpUtilMemFree(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       memcpy(stream,&temp,strlen(temp));
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(temp);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_COMVERSIONMAJ.\n" ));
               break;

            case MIB_LM_COMVERSIONMIN:

              if((NULL == cache_table[C_NETWKSTAGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETWKSTAGETINFO].bufptr ) ;
                //
               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                101,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
                       _itoa(wksta_info_one->wki101_ver_minor,temp,10) ;
                       if(NULL ==
                        (stream = SnmpUtilMemAlloc( strlen(temp) ))
                       ){
                          SnmpUtilMemFree(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       memcpy(stream,&temp,strlen(temp));
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(temp);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_COMVERSIONMIN.\n" ));
               break;

            case MIB_LM_COMTYPE:
              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr ) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       server_info_two = (SERVER_INFO_102 *) bufptr ;
                       if(NULL ==
                        (stream = SnmpUtilMemAlloc( 4 * sizeof(BYTE) ))
                       ){
                          SnmpUtilMemFree(retval);
                          retval=NULL;
                          goto Exit ;
                       }
                       *(DWORD*)stream=server_info_two->sv102_type & 0x000000FF;
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = 4 * sizeof(BYTE);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_COMTYPE.\n" ));
               break;

            case MIB_LM_COMSTATSTART:
              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_start;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_STATSTART.\n" ));
               break;

            case MIB_LM_COMSTATNUMNETIOS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval =
                  (wrk_stats_zero->SmbsReceived).LowPart +
                          (wrk_stats_zero->SmbsTransmitted).LowPart;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_STATNUMNETIOS.\n" ));
               break;

            case MIB_LM_COMSTATFINETIOS:


              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->InitiallyFailedOperations;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_STATFINETIOS.\n" ));
               break;

            case MIB_LM_COMSTATFCNETIOS:


              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedCompletionOperations;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)

               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_STATFCNETIOS.\n" ));
               break;

            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Data not supported by function.\n" ));

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
} // MIB_common_func

void  * MIB_server_func(
           IN UINT Action,  // Action to perform on Data
           IN LDATA LMData, // LM Data to manipulate
           IN void *SetData
           )

{

lan_return_info_type *retval=NULL;
SERVER_INFO_102 *server_info_two;
SERVER_INFO_102 server_info_10two;
STAT_SERVER_0 *server_stats_zero;
SERVER_INFO_102 *server_info_102 ;
SERVER_INFO_403 *server_info_four ;
SESSION_INFO_2 * session_info_two;
SERVER_INFO_402 *server_info_402 ;
#if 1
USER_INFO_0 *user_info_zero ;
#endif
unsigned lmCode;
BYTE *stream;
AsnOctetString *strvalue;
AsnInteger intvalue;
DWORD entriesread;
DWORD totalentries;
SNMPAPI nResult;
LPBYTE bufptr;
BOOL cache_it ;
time_t curr_time ;
#ifdef UNICODE
LPWSTR unitemp ;
#endif

   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

        case MIB_LM_SVDESCRIPTION:
              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr );

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       server_info_two = (SERVER_INFO_102 *) bufptr ;

                       #ifdef UNICODE
                                if (SnmpUtilUnicodeToUTF8(
                                        &stream,
                                        server_info_two->sv102_comment,
                                        TRUE))
                                {
                                    SnmpUtilMemFree(retval);
                                    retval = NULL;
                                    goto Exit;
                                }
                       #else
                               if(NULL ==
                                (stream = SnmpUtilMemAlloc( strlen(server_info_two->sv102_comment) + 1 ))
                               ) {
                                  SnmpUtilMemFree(retval);
                                  retval=NULL;
                                  goto Exit ;
                               }

                                memcpy(stream,server_info_two->sv102_comment,
                                        strlen(server_info_two->sv102_comment));
                       #endif
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length =
                                strlen(stream);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVDESCRIPTION.\n" ));
               break;


                case MIB_LM_SVSVCNUMBER:

              if((NULL == cache_table[C_NETSERVICEENUM].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVICEENUM].acquisition_time
                         + cache_expire[C_NETSERVICEENUM]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVICEENUM].bufptr) ;

               lmCode =
               NetServiceEnum( NULL,                    // local server
                                0,                      // level 0
                                &bufptr,                        // data structure to return
                                MAX_PREFERRED_LENGTH,
                                &entriesread,
                                &totalentries,
                                NULL);
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVICEENUM].bufptr ;
                totalentries =  cache_table[C_NETSERVICEENUM].totalentries ;
                entriesread =  cache_table[C_NETSERVICEENUM].entriesread ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       retval->d.intval = totalentries; // LOOK OUT!!
                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVICEENUM].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVICEENUM].bufptr = bufptr ;
                                cache_table[C_NETSERVICEENUM].totalentries =
                                                totalentries ;
                                cache_table[C_NETSERVICEENUM].entriesread =
                                                entriesread ;
                        } // if (cache_it)
               }


               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSVCNUMBER.\n" ));
               break;


                case MIB_LM_SVSTATOPENS:


              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_fopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)

               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATOPENS.\n" ));
               break;


                case MIB_LM_SVSTATDEVOPENS:



              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_devopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATDEVOPENS.\n" ));
               break;

                case MIB_LM_SVSTATQUEUEDJOBS:



              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_jobsqueued;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATQUEUEDJOBS.\n" ));
               break;

                case MIB_LM_SVSTATSOPENS:


              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_sopens;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATSOPENS.\n" ));
               break;

                case MIB_LM_SVSTATERROROUTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_serrorout;
                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATSERROROUTS.\n" ));
               break;

                case MIB_LM_SVSTATPWERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_pwerrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATPWERRORS.\n" ));
               break;

                case MIB_LM_SVSTATPERMERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_permerrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATPERMERRORS.\n" ));
               break;

                case MIB_LM_SVSTATSYSERRORS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_syserrors;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATSYSERRORS.\n" ));
               break;

                case MIB_LM_SVSTATSENTBYTES:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bytessent_low;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATSENTBYTES.\n" ));
               break;

                case MIB_LM_SVSTATRCVDBYTES:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bytesrcvd_low;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATRCVDBYTES.\n" ));
               break;

                case MIB_LM_SVSTATAVRESPONSE:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_avresponse;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATAVRESPONSE.\n" ));
               break;

         case MIB_LM_SVSECURITYMODE:

             // hard code USER security per dwaink
             //
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       retval->d.intval = 2 ;

#if 0
              if((NULL == cache_table[C_NETSERVERGETINFO_403].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO_403].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO_403]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO_403].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                403,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO_403].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_four = (SERVER_INFO_403 *) bufptr ;
                       retval->d.intval = server_info_four->sv403_security;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO_403].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO_403].bufptr = bufptr ;
                        } // if (cache_it)
               }
#endif
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSECURITYMODE.\n" ));
               break;



                case MIB_LM_SVUSERS:

              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_102 = (SERVER_INFO_102 *) bufptr ;
                       retval->d.intval = server_info_102->sv102_users;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVUSERS.\n" ));
               break;

                case MIB_LM_SVSTATREQBUFSNEEDED:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_reqbufneed;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATREQBUFSNEEDED.\n" ));
               break;

                case MIB_LM_SVSTATBIGBUFSNEEDED:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ;

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_bigbufneed;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATBIGBUFNEEDED.\n" ));
               break;

                case MIB_LM_SVSESSIONNUMBER:

              if((NULL == cache_table[C_NETSESSIONENUM].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSESSIONENUM].acquisition_time
                         + cache_expire[C_NETSESSIONENUM]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSESSIONENUM].bufptr) ;

               lmCode =
               NetSessionEnum(  NULL,                   // local server
                                NULL,           // get server stats
                                NULL,
                                2,                      // level
                                &bufptr,                // data structure to return
                                MAX_PREFERRED_LENGTH,
                                &entriesread,
                                &totalentries,
                                NULL                    // no resume handle
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSESSIONENUM].bufptr ;
                totalentries =  cache_table[C_NETSESSIONENUM].totalentries ;
                entriesread =  cache_table[C_NETSESSIONENUM].entriesread ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       session_info_two = (SESSION_INFO_2 *) bufptr ;
                       retval->d.intval = totalentries;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSESSIONENUM].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSESSIONENUM].bufptr = bufptr ;
                                cache_table[C_NETSESSIONENUM].totalentries =
                                                totalentries ;
                                cache_table[C_NETSESSIONENUM].entriesread =
                                                entriesread ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSESSIONNUMBER.\n" ));
               break;

                case MIB_LM_SVAUTODISCONNECTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_SERVER].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_SERVER]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSTATISTICSGET_SERVER].bufptr);

               lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_SERVER,         // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_SERVER].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_stats_zero = (STAT_SERVER_0 *) bufptr ;
                       retval->d.intval = server_stats_zero->sts0_stimedout;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_SERVER].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_SERVER].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSTATAUTODISCONNECT.\n" ));
               break;

                case MIB_LM_SVDISCONTIME:

              if((NULL == cache_table[C_NETSERVERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                102,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_102 = (SERVER_INFO_102 *) bufptr ;
                       retval->d.intval = server_info_102->sv102_disc ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVDISCONTIME.\n" ));
               break;

                case MIB_LM_SVAUDITLOGSIZE:


            {
                HANDLE hEventLog;
                DWORD  cRecords;

                hEventLog = OpenEventLog( NULL,
                                          TEXT("APPLICATION"));
                    if(NULL ==
                       (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                    retval->data_element_type = ASN_INTEGER ;
                if(GetNumberOfEventLogRecords( hEventLog, &cRecords )){

                       retval->d.intval = cRecords ;
                } else {
                       retval->d.intval = 0 ;
                }
                DeregisterEventSource( hEventLog );
            }
#if 0
              if((NULL == cache_table[C_NETSERVERGETINFO_402].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSERVERGETINFO_402].acquisition_time
                         + cache_expire[C_NETSERVERGETINFO_402]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree( cache_table[C_NETSERVERGETINFO_402].bufptr) ;

               lmCode =
               NetServerGetInfo( NULL,                  // local server
                                402,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSERVERGETINFO_402].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       server_info_402 = (SERVER_INFO_402 *) bufptr ;
                       retval->d.intval = server_info_402->sv402_maxauditsz ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSERVERGETINFO_402].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSERVERGETINFO_402].bufptr = bufptr ;
                        } // if (cache_it)
                }
#endif
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVAUDITLOGSIZE.\n" ));
               break;


                case MIB_LM_SVUSERNUMBER:



                MIB_users_lmget();   // fire off the table get
                if(NULL ==
                   (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_UserTable.Len;

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVUSERNUMBER.\n" ));
               break;


                case MIB_LM_SVSHARENUMBER:


                MIB_shares_lmget();   // fire off the table get
                if(NULL ==
                   (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ) )
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_ShareTable.Len;


               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_SVSHARENUMBER.\n" ));
               break;


        case MIB_LM_SVPRINTQNUMBER:

                MIB_prntq_lmget();   // fire off the table get
                if(NULL ==
                    (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_PrintQTable.Len;


               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_PRINTQNUMBER.\n" ));
               break;



            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Data not supported by function.\n" ));

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {

        case MIB_LM_SVDESCRIPTION:

                // retrieve string to be written
                strvalue = (AsnOctetString *) SetData ;

                // convert it to zero terminated string
                stream = SnmpUtilMemAlloc( strvalue->length+1 );
                if (stream == NULL) {
                    retval = (void *) FALSE;
                    break;
                }
                memcpy(stream,strvalue->stream,strvalue->length);
                stream[strvalue->length] = 0;

                SNMPDBG((
                    SNMP_LOG_TRACE,
                    "LMMIB2: changing server description to %s.\n",
                    stream
                    ));

                #ifdef UNICODE
                if (SnmpUtilUTF8ToUnicode(&unitemp,
                                          stream,
                                          TRUE
                                          )) {
                    SnmpUtilMemFree(stream);
                    retval = (void *) FALSE;
                    break;
                }
                SnmpUtilMemFree(stream);
                stream = (LPBYTE)unitemp;
                #endif

                lmCode = NetServerSetInfo(
                                NULL,                   // this server
                                SV_COMMENT_INFOLEVEL,   // level
                                (LPBYTE)&stream,        // data
                                NULL );                 // option

                SnmpUtilMemFree(stream);

                if(NERR_Success == lmCode) {

                        retval = (void *) TRUE;
                        cache_table[C_NETSERVERGETINFO].acquisition_time = 0;

                        SNMPDBG((
                            SNMP_LOG_TRACE,
                            "LMMIB2: server description changed, invalidating cache.\n"
                            ));

                } else {

                        retval = (void *) FALSE;

                        SNMPDBG((
                            SNMP_LOG_TRACE,
                            "LMMIB2: server description not changed 0x%08lx.\n",
                            lmCode
                            ));
                }
                break ;

        case MIB_LM_SVDISCONTIME:

                intvalue = (AsnInteger)((ULONG_PTR) SetData) ;
                memset(&server_info_10two,0,sizeof(server_info_10two));
                server_info_10two.sv102_disc = intvalue ;
                lmCode = NetServerSetInfo(
                                NULL,                   // this server
                                SV_DISC_INFOLEVEL,                      // level
                                (LPBYTE)&server_info_10two,     // data
                                NULL );                 // option
                if(NERR_Success == lmCode) {
                        retval = (void *)TRUE;
                } else {
                        retval = (void *) FALSE;
                }
                break ;

        case MIB_LM_SVAUDITLOGSIZE:

                retval =  (void *) FALSE;
                break ;

            }  // switch(LMData)

         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
} // MIB_server_func

void  * MIB_workstation_func(
           IN UINT Action,   // Action to perform on Data
           IN LDATA LMData,    // LM Data to manipulate
           IN void *SetData
           )

{

SNMPAPI nResult;
unsigned lmCode;
STAT_WORKSTATION_0 *wrk_stats_zero;
WKSTA_INFO_502 *wksta_info_five;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
DWORD entriesread;
DWORD totalentries;
BOOL cache_it ;
time_t curr_time ;


UNREFERENCED_PARAMETER(SetData);
   time(&curr_time);    // get the time

   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

                case MIB_LM_WKSTASTATSESSSTARTS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //

                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->Sessions;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTASTATSESSSTARTS.\n" ));
               break;


                case MIB_LM_WKSTASTATSESSFAILS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedSessions;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTASTATSESSFAILS.\n" ));
               break;

                case MIB_LM_WKSTASTATUSES:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->UseCount;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTASTATUSES.\n" ));
               break;

                case MIB_LM_WKSTASTATUSEFAILS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->FailedUseCount;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTASTATUSEFAILS.\n" ));
               break;

                case MIB_LM_WKSTASTATAUTORECS:

              if((NULL == cache_table[C_NETSTATISTICSGET_WORKST].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time
                         + cache_expire[C_NETSTATISTICSGET_WORKST]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETSTATISTICSGET_WORKST].bufptr);

                lmCode =
               NetStatisticsGet( NULL,                  // local server
                                SERVICE_WORKSTATION,    // get server stats
                                0,                      // level 0
                                0,                      // don't clear stats
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETSTATISTICSGET_WORKST].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wrk_stats_zero = (STAT_WORKSTATION_0 *) bufptr ;
                       retval->d.intval = wrk_stats_zero->Reconnects;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETSTATISTICSGET_WORKST].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETSTATISTICSGET_WORKST].bufptr = bufptr ;
                        } // if (cache_it)
                }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTASTATAUTORECS.\n" ));
               break;

                case MIB_LM_WKSTAERRORLOGSIZE:

              if((NULL == cache_table[C_NETWKSTAGETINFO_502].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO_502].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO_502]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAGETINFO_502].bufptr) ;

               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                502,                    // level 10, no admin priv.
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO_502].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_INTEGER ;
                       wksta_info_five = (WKSTA_INFO_502 *) bufptr ;
                       retval->d.intval =
                           wksta_info_five->wki502_maximum_collection_count ;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO_502].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO_502].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTAERRORLOGSIZE.\n" ));
               break;


                case MIB_LM_WKSTAUSENUMBER:

                MIB_wsuses_lmget();   // fire off the table get
                if(NULL ==
                   (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                retval->data_element_type = ASN_INTEGER ;
                retval->d.intval = MIB_WkstaUsesTable.Len;


               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_WKSTAUSENUMBER.\n" ));
               break;

            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Data not supported by function.\n" ));

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;


      case MIB_ACTION_SET:
         switch ( LMData )
            {

                case MIB_LM_WKSTAERRORLOGSIZE:
                        ;
            }

         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
}

void  * MIB_domain_func(
           IN UINT Action,   // Action to perform on Data
           IN LDATA LMData,  // LM Data to manipulate
           void *SetData
           )

{


SNMPAPI nResult;
unsigned lmCode;
WKSTA_INFO_101 *wksta_info_one;
LPBYTE bufptr;
lan_return_info_type *retval=NULL;
DWORD entriesread;
DWORD totalentries;
BYTE *stream;
BOOL cache_it ;
time_t curr_time ;

UNREFERENCED_PARAMETER(SetData);
   time(&curr_time);    // get the time


   switch ( Action )
      {
      case MIB_ACTION_GET:
         // Check to see if data is cached
         //if ( Cached )
            //{
            // Retrieve from cache
            //}
         //else
            //{
            // Call LM call to get data

            // Put data in cache
            //}

         // See if data is supported
         switch ( LMData )
            {

                case MIB_LM_DOMPRIMARYDOMAIN:

              if((NULL == cache_table[C_NETWKSTAGETINFO_101].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAGETINFO_101].acquisition_time
                         + cache_expire[C_NETWKSTAGETINFO_101]              ) ) )
              {  // it has expired!
                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAGETINFO_101].bufptr) ;

               lmCode =
               NetWkstaGetInfo( NULL,                   // local server
                                101,                    // level 101,
                                &bufptr                 // data structure to return
                                );
                cache_it = TRUE ;
              } else {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAGETINFO_101].bufptr ;
                cache_it = FALSE ;
              }


               if(lmCode == 0)  {  // valid so return it, otherwise error NULL
                       if(NULL ==
                        (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
                       retval->data_element_type = ASN_OCTETSTRING ;
                       wksta_info_one = (WKSTA_INFO_101 *) bufptr ;
#ifdef UNICODE
                       if (SnmpUtilUnicodeToUTF8(
                                        &stream,
                                        wksta_info_one->wki101_langroup,
                                        TRUE))
                       {
                          SnmpUtilMemFree(retval);
                          retval=NULL;
                          goto Exit ;
                       }
#else
                       if(NULL ==
                        (stream = SnmpUtilMemAlloc( strlen(wksta_info_one->wki101_langroup)+2 ))){
                          SnmpUtilMemFree(retval);
                          retval=NULL;
                          goto Exit ;
                       }

                       memcpy(stream,
                                        wksta_info_one->wki101_langroup,
                                        strlen(wksta_info_one->wki101_langroup));
#endif
                       retval->d.octstrval.stream = stream;
                       retval->d.octstrval.length = strlen(stream);
                       retval->d.octstrval.dynamic = TRUE;

                       if(cache_it) {
                       // now save it in the cache
                                cache_table[C_NETWKSTAGETINFO_101].acquisition_time =
                                        curr_time ;
                                cache_table[C_NETWKSTAGETINFO_101].bufptr = bufptr ;
                        } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_DOMPRIMARYDOMAIN.\n" ));
               break;

               case MIB_LM_DOMLOGONDOMAIN:
              if((NULL == cache_table[C_NETWKSTAUSERGETINFO].bufptr) ||
                 (curr_time >
                        (cache_table[C_NETWKSTAUSERGETINFO].acquisition_time
                         + cache_expire[C_NETWKSTAUSERGETINFO]              ) ) )
              {
                // it has expired!
                PLSA_UNICODE_STRING logonUserName, logonDomainName;

                //
                // remember to free the existing data
                //
                SafeBufferFree(cache_table[C_NETWKSTAUSERGETINFO].bufptr) ;
                logonUserName = logonDomainName = NULL;

                // sensible point. Hard to get a call that is returning the logonDomainName:
                // NetWkstaUserGetInfo() is returning NO_LOGON_SESSION (for level 1)
                // LsaGetUserName() is returning inaccurate info when running from service controller.
                lmCode = LsaGetUserName( &logonUserName, &logonDomainName);

                SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: got '%c'\n", (LPBYTE)(logonDomainName->Buffer)[0] ));
                if (lmCode == 0)
                    bufptr = (LPBYTE)logonDomainName->Buffer;
                cache_it = TRUE ;
              }
              else
              {
                lmCode = 0 ;  // fake a sucessful lan man call
                bufptr =  cache_table[C_NETWKSTAUSERGETINFO].bufptr ;
                cache_it = FALSE ;
              }

               if(lmCode == 0)  
              {
                  // valid so return it, otherwise error NULL
                  if(NULL == (retval = SnmpUtilMemAlloc(sizeof(lan_return_info_type))) )
                          goto Exit ;
                  retval->data_element_type = ASN_OCTETSTRING ;
#ifdef UNICODE
                  if (SnmpUtilUnicodeToUTF8(
                    &stream,
                    (LPWSTR)bufptr,
                    TRUE))
                  {
                      SnmpUtilMemFree(retval);
                      retval=NULL;
                      goto Exit ;
                  }
#else
                  if(NULL == (stream = SnmpUtilMemAlloc(strlen((LPWSTR)bufptr)+2)) )
                  {
                      SnmpUtilMemFree(retval);
                      retval=NULL;
                      goto Exit ;
                  }

                  memcpy(stream,
                         bufptr,
                         strlen(bufptr));
#endif
                  retval->d.octstrval.stream = stream;
                  retval->d.octstrval.length = strlen(stream);
                  retval->d.octstrval.dynamic = TRUE;

                  if(cache_it)
                  {
                    // now save it in the cache
                    cache_table[C_NETWKSTAUSERGETINFO].acquisition_time = curr_time ;
                    cache_table[C_NETWKSTAUSERGETINFO].bufptr = bufptr ;
                  } // if (cache_it)
               }

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_DOMLOGONDOMAIN.\n" ));
               break;


               case MIB_LM_DOMOTHERDOMAINNUMBER:

               MIB_odoms_lmget();   // fire off the table get
               if(NULL ==
                   (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
               retval->data_element_type = ASN_INTEGER ;
               retval->d.intval = MIB_DomOtherDomainTable.Len;

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_DOMOTHERDOMAINNUMBER.\n" ));
               break;


               case MIB_LM_DOMSERVERNUMBER:

               MIB_svsond_lmget();   // fire off the table get
                
               if(NULL ==
                  (retval = SnmpUtilMemAlloc( sizeof(lan_return_info_type) ))
                       )
                          goto Exit ;
               retval->data_element_type = ASN_INTEGER ;
               retval->d.intval = MIB_DomServerTable.Len;

               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: processing MIB_LM_DOMSERVERNUMBER.\n" ));
               break;


//
// OPENISSUE --> NETLOGONENUM permanently eliminated
//
#if 0
// did some  of these guys get lost ???
// double check there is a mistake in the mib table
//
               case MIB_LM_DOMLOGONNUMBER:
               case MIB_LM_DOMLOGONTABLE:
               case MIB_LM_DOMLOGONENTRY:
               case MIB_LM_DOMLOGONUSER:
               case MIB_LM_DOMLOGONMACHINE:
#endif
            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Data not supported by function.\n" ));

               nResult = SNMPAPI_ERROR;
               goto Exit;
            }

         break;

      case MIB_ACTION_SET:
         switch ( LMData )
            {
                case MIB_LM_DOMOTHERNAME:
                        ;
            }
         break;


      default:
         // Signal an error

         nResult = SNMPAPI_ERROR;
         goto Exit;
      }

Exit:
   return retval /*nResult*/;
}



//
// MIB_leaf_func
//    Performs actions on LEAF variables in the MIB.
//
// Notes:
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
UINT MIB_leaf_func(
        IN UINT Action,
        IN MIB_ENTRY *MibPtr,
        IN RFC1157VarBind *VarBind
        )

{
lan_return_info_type *MibVal;
UINT                 nResult;

   switch ( Action )
      {
      case MIB_ACTION_GETNEXT:
         if ( MibPtr->MibNext == NULL )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         nResult = (*MibPtr->MibNext->MibFunc)( MIB_ACTION_GETFIRST,
                                                MibPtr->MibNext, VarBind );
         break;

      case MIB_ACTION_GETFIRST:

         // Check to see if this variable is accessible for GET
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
            {
            if ( MibPtr->MibNext != NULL )
               {
               nResult = (*MibPtr->MibNext->MibFunc)( Action,
                                                      MibPtr->MibNext,
                                                      VarBind );
               }
            else
               {
               nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
               }

            break;
            }
         else
            {
            // Place correct OID in VarBind
            SnmpUtilOidFree( &VarBind->name );
            SnmpUtilOidCpy( &VarBind->name, &MIB_OidPrefix );
            SnmpUtilOidAppend( &VarBind->name, &MibPtr->Oid );
            }

         // Purposefully let fall through to GET

      case MIB_ACTION_GET:
         // Make sure that this variable is GET'able
         if ( MibPtr->Access != MIB_ACCESS_READ &&
              MibPtr->Access != MIB_ACCESS_READWRITE )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Call the LM call to get data
         MibVal = (*MibPtr->LMFunc)( MIB_ACTION_GET, MibPtr->LMData, NULL );
         if ( MibVal == NULL )
            {
            nResult = SNMP_ERRORSTATUS_GENERR;
            goto Exit;
            }

         // Setup varbind's return value
         VarBind->value.asnType = MibPtr->Type;
         switch ( MibPtr->Type )
            {
            case ASN_RFC1155_COUNTER:
            case ASN_RFC1155_GAUGE:
            case ASN_RFC1155_TIMETICKS:
            case ASN_INTEGER:
               VarBind->value.asnValue.number = MibVal->d.intval;
               break;

            case ASN_RFC1155_IPADDRESS:
            case ASN_RFC1155_OPAQUE:
            case ASN_OCTETSTRING:
               // This is non-standard copy of structure
               VarBind->value.asnValue.string = MibVal->d.octstrval;
               break;

            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Processing LAN Manager LEAF Variable\n" ));
               nResult = SNMP_ERRORSTATUS_GENERR;
               SnmpUtilMemFree( MibVal );
               goto Exit;
            } // type switch

         // Free memory alloc'ed by LM API call
         SnmpUtilMemFree( MibVal );
         nResult = SNMP_ERRORSTATUS_NOERROR;
         break;

      case MIB_ACTION_SET:
         // Check for writable attribute
         if ( MibPtr->Access != MIB_ACCESS_READWRITE &&
              MibPtr->Access != MIB_ACCESS_WRITE )
            {
            nResult = SNMP_ERRORSTATUS_NOSUCHNAME;
            goto Exit;
            }

         // Check for proper type before setting
         if ( MibPtr->Type != VarBind->value.asnType )
            {
            nResult = SNMP_ERRORSTATUS_BADVALUE;
            goto Exit;
            }

         // Call LM routine to set variable
         switch ( VarBind->value.asnType )
            {
            case ASN_RFC1155_COUNTER:
            case ASN_INTEGER:
               if ( SNMPAPI_ERROR ==
                    (*MibPtr->LMFunc)(MIB_ACTION_SET, MibPtr->LMData,
                                      (void *)&VarBind->value.asnValue.number) )
                  {
                  nResult = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
                  }
               break;

            case ASN_OCTETSTRING: // This entails ASN_RFC1213_DISPSTRING also
               if ( SNMPAPI_ERROR ==
                    (*MibPtr->LMFunc)(MIB_ACTION_SET, MibPtr->LMData,
                                      (void *)&VarBind->value.asnValue.string) )
                  {
                  nResult = SNMP_ERRORSTATUS_GENERR;
                  goto Exit;
                  }
               break;
            default:
               SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Processing LAN Manager LEAF Variable\n" ));
               nResult = SNMP_ERRORSTATUS_GENERR;
               goto Exit;
            }

         nResult = SNMP_ERRORSTATUS_NOERROR;
         break;

      default:
         SNMPDBG(( SNMP_LOG_TRACE, "LMMIB2: Internal Error Processing LAN Manager LEAF Variable\n" ));
         nResult = SNMP_ERRORSTATUS_GENERR;
      } // switch

Exit:
   return nResult;
} // MIB_leaf_func

//-------------------------------- END --------------------------------------
