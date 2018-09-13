/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    getstats.c

Abstract:

    Tests NT-level NetStatisticsGet API

Author:

    Richard L Firth (rfirth) 08-Aug-1991

Revision History:

    09-May-1992 rfirth
        Change to use new redirector/wksta statistics

    08-Aug-1991 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <lmcons.h>
#include <lmstats.h>
#include <lmsname.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <tstring.h>

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

void main(int, char**);
void usage(void);

void main(int argc, char** argv) {
    LPTSTR  service_name;
    LPTSTR  server_name = NULL;
    DWORD   level = 0;
    DWORD   options = 0;
    LPBYTE  buffer;
    NET_API_STATUS  rc;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            ++*argv;
            switch (tolower(**argv)) {
            case 'c':
                options = STATSOPT_CLR;
                break;

            case 'h':
            case '?':
                usage();
                break;

            case 'l':
                level = (DWORD)atoi(++*argv);
                break;

            case 'w':
                service_name = SERVICE_WORKSTATION;
                break;

            case 's':
                service_name = SERVICE_SERVER;
                break;

            default:
                printf("error: bad flag: '%c'\n", **argv);
                usage();
            }
        } else if (server_name) {

            //
            // allow the user to enter a service name. This allows us to expand
            // the test to cover other services which may be included in future
            //

            service_name = *argv;
        } else {
            server_name = *argv;
        }
    }
    rc = NetStatisticsGet(server_name, service_name, level, options, &buffer);
    if (rc != NERR_Success) {
        printf("error: NetStatisticsGet returns %u\n", rc);
        exit(2);
    } else {
        if (!STRCMP(service_name, SERVICE_SERVER)) {
            printf("Server statistics for %s:\n"
                   "Time stats started............................: %u\n"
                   "Number of file opens..........................: %u\n"
                   "Number of device opens........................: %u\n"
                   "Number of print jobs spooled..................: %u\n"
                   "Number of server session started..............: %u\n"
                   "Number of sessions auto disconnected..........: %u\n"
                   "Number of server sessions failed with error...: %u\n"
                   "Number of password violations.................: %u\n"
                   "Number of server system errors................: %u\n"
                   "Number of bytes sent to net (low).............: %u\n"
                   "Number of bytes sent to net (high)............: %u\n"
                   "Number of bytes received from net (low).......: %u\n"
                   "Number of bytes received from net (high)......: %u\n"
                   "Average response time.........................: %u\n"
                   "Number of failures to allocate buffer.........: %u\n"
                   "Number of failures to allocate big buf........: %u\n",
                    server_name,
                    ((LPSTAT_SERVER_0)buffer)->sts0_start,
                    ((LPSTAT_SERVER_0)buffer)->sts0_fopens,
                    ((LPSTAT_SERVER_0)buffer)->sts0_devopens,
                    ((LPSTAT_SERVER_0)buffer)->sts0_jobsqueued,
                    ((LPSTAT_SERVER_0)buffer)->sts0_sopens,
                    ((LPSTAT_SERVER_0)buffer)->sts0_stimedout,
                    ((LPSTAT_SERVER_0)buffer)->sts0_serrorout,
                    ((LPSTAT_SERVER_0)buffer)->sts0_pwerrors,
                    ((LPSTAT_SERVER_0)buffer)->sts0_permerrors,
                    ((LPSTAT_SERVER_0)buffer)->sts0_syserrors,
                    ((LPSTAT_SERVER_0)buffer)->sts0_bytessent_low,
                    ((LPSTAT_SERVER_0)buffer)->sts0_bytessent_high,
                    ((LPSTAT_SERVER_0)buffer)->sts0_bytesrcvd_low,
                    ((LPSTAT_SERVER_0)buffer)->sts0_bytesrcvd_high,
                    ((LPSTAT_SERVER_0)buffer)->sts0_avresponse,
                    ((LPSTAT_SERVER_0)buffer)->sts0_reqbufneed,
                    ((LPSTAT_SERVER_0)buffer)->sts0_bigbufneed
                    );
        } else if (!STRCMP(service_name, SERVICE_WORKSTATION)) {
            printf("Workstation statistics for %s:\n"
#ifdef LM20_WORKSTATION_STATISTICS
                   "Time stats started..............................: %u\n"
                   "Total NCBs issued by redirector.................: %u\n"
                   "Total NCBs issued by server.....................: %u\n"
                   "Total NCBs issued by apps.......................: %u\n"
                   "Failed NCBs issued by redirector................: %u\n"
                   "Failed NCBs issued by server....................: %u\n"
                   "Failed NCBs issued by apps......................: %u\n"
                   "NCBs issued by redir failing before completion..: %u\n"
                   "NCBs issued by server failing before completion.: %u\n"
                   "NCBs issued by apps failing before completion...: %u\n"
                   "Number of sessions started......................: %u\n"
                   "Number of sessions failed to connect............: %u\n"
                   "Number of sessions failed after connecting......: %u\n"
                   "Number of uses..................................: %u\n"
                   "Number of failed uses...........................: %u\n"
                   "Number of auto reconnections....................: %u\n"
                   "Number of bytes sent to net (high)..............: %u\n"
                   "Number of bytes sent to net (low)...............: %u\n"
                   "Number of bytes received from net (high)........: %u\n"
                   "Number of bytes received from net (low).........: %u\n"
                   "Number of server bytes sent to net (high).......: %u\n"
                   "Number of server bytes sent to net (low)........: %u\n"
                   "Number of server bytes received from net (high).: %u\n"
                   "Number of server bytes received from net (low)..: %u\n"
                   "Number of app bytes sent to net (high)..........: %u\n"
                   "Number of app bytes sent to net (low)...........: %u\n"
                   "Number of app bytes received from net (high)....: %u\n"
                   "Number of app bytes received from net (low).....: %u\n"
                   "Number of failures to allocate buffer...........: %u\n"
                   "Number of failures to allocate big buf..........: %u\n",
                   server_name,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_start,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_numNCB_r,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_numNCB_s,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_numNCB_a,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fiNCB_r,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fiNCB_s,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fiNCB_a,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fcNCB_r,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fcNCB_s,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_fcNCB_a,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_sesstart,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_sessfailcon,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_sessbroke,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_uses,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_usefail,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_autorec,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_r_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_r_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_r_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_r_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_s_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_s_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_s_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_s_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_a_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytessent_a_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_a_hi,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bytesrcvd_a_lo,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_reqbufneed,
                   ((LPSTAT_WORKSTATION_0)buffer)->stw0_bigbufneed
#else
                   "Bytes Received....................: %08x%08x\n"
                   "SMBs Received.....................: %08x%08x\n"
                   "Paging Read Bytes Requested.......: %08x%08x\n"
                   "Non-paging Read Bytes Requested...: %08x%08x\n"
                   "Cache Read Bytes Requested........: %08x%08x\n"
                   "Network Read Bytes Requested......: %08x%08x\n"
                   "Bytes Transmitted.................: %08x%08x\n"
                   "SMBs Transmitted..................: %08x%08x\n"
                   "Paging Write Bytes Requested......: %08x%08x\n"
                   "Non-paging Write Bytes Requested..: %08x%08x\n"
                   "Cache Write Bytes Requested.......: %08x%08x\n"
                   "Network Write Bytes Requested.....: %08x%08x\n"
                   "Read Operations...................: %u\n"
                   "Random Read Operations............: %u\n"
                   "Read SMBs.........................: %u\n"
                   "Large Read SMBs...................: %u\n"
                   "Small Read SMBs...................: %u\n"
                   "Write Operations..................: %u\n"
                   "Random Write Operations...........: %u\n"
                   "Write SMBs........................: %u\n"
                   "Large Write SMBs..................: %u\n"
                   "Small Write SMBs..................: %u\n"
                   "Raw Reads Denied..................: %u\n"
                   "Raw Writes Dennied................: %u\n"
                   "Network Errors....................: %u\n"
                   "Sessions..........................: %u\n"
                   "Reconnects........................: %u\n"
                   "Core Connects.....................: %u\n"
                   "Lanman 2.0 Connects...............: %u\n"
                   "Lanman 2.1 Connects...............: %u\n"
                   "Lanman NT Connects................: %u\n"
                   "Server Disconnects................: %u\n"
                   "Hung Sessions.....................: %u\n"
                   "Current Commands..................: %u\n",
                   server_name,
                   ((LPSTAT_WORKSTATION_0)buffer)->BytesReceived.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->BytesReceived.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmbsReceived.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmbsReceived.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->PagingReadBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->PagingReadBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NonPagingReadBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NonPagingReadBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->CacheReadBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->CacheReadBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NetworkReadBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NetworkReadBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->BytesTransmitted.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->BytesTransmitted.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmbsTransmitted.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmbsTransmitted.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->PagingWriteBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->PagingWriteBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NonPagingWriteBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NonPagingWriteBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->CacheWriteBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->CacheWriteBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NetworkWriteBytesRequested.HighPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->NetworkWriteBytesRequested.LowPart,
                   ((LPSTAT_WORKSTATION_0)buffer)->ReadOperations,
                   ((LPSTAT_WORKSTATION_0)buffer)->RandomReadOperations,
                   ((LPSTAT_WORKSTATION_0)buffer)->ReadSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->LargeReadSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmallReadSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->WriteOperations,
                   ((LPSTAT_WORKSTATION_0)buffer)->RandomWriteOperations,
                   ((LPSTAT_WORKSTATION_0)buffer)->WriteSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->LargeWriteSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->SmallWriteSmbs,
                   ((LPSTAT_WORKSTATION_0)buffer)->RawReadsDenied,
                   ((LPSTAT_WORKSTATION_0)buffer)->RawWritesDenied,
                   ((LPSTAT_WORKSTATION_0)buffer)->NetworkErrors,
                   ((LPSTAT_WORKSTATION_0)buffer)->Sessions,
                   ((LPSTAT_WORKSTATION_0)buffer)->Reconnects,
                   ((LPSTAT_WORKSTATION_0)buffer)->CoreConnects,
                   ((LPSTAT_WORKSTATION_0)buffer)->Lanman20Connects,
                   ((LPSTAT_WORKSTATION_0)buffer)->Lanman21Connects,
                   ((LPSTAT_WORKSTATION_0)buffer)->LanmanNtConnects,
                   ((LPSTAT_WORKSTATION_0)buffer)->ServerDisconnects,
                   ((LPSTAT_WORKSTATION_0)buffer)->HungSessions,
                   ((LPSTAT_WORKSTATION_0)buffer)->CurrentCommands
#endif
                   );
        } else {
            printf("warning: don't know how to display info for service %s\n", service_name);
        }
        NetApiBufferFree(buffer);
        exit(0);
    }
}

void usage() {
    printf("usage: GetStats -w|-s -c -l# [\\\\ServerName] [optional_service_name]\n"
           "where: -w : gets WorkStation statistics\n"
           "       -s : gets Server      statistics\n"
           "       -c : clears statistics\n"
           "       -l#: level of information required (API=MBZ, default)\n"
           );
    exit(1);
}
