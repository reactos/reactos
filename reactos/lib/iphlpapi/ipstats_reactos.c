/* Copyright (C) 2003 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This file implements statistics getting using the /proc filesystem exported
 * by Linux, and maybe other OSes.
 */

#include "ipprivate.h"
#include "ipstats.h"

#ifndef TCPS_ESTABLISHED
# define TCPS_ESTABLISHED TCP_ESTABLISHED
#endif
#ifndef TCPS_SYN_SENT
# define TCPS_SYN_SENT TCP_SYN_SENT
#endif
#ifndef TCPS_SYN_RECEIVED
# define TCPS_SYN_RECEIVED TCP_SYN_RECV
#endif
#ifndef TCPS_FIN_WAIT_1
# define TCPS_FIN_WAIT_1 TCP_FIN_WAIT1
#endif
#ifndef TCPS_FIN_WAIT_2
# define TCPS_FIN_WAIT_2 TCP_FIN_WAIT2
#endif
#ifndef TCPS_TIME_WAIT
# define TCPS_TIME_WAIT TCP_TIME_WAIT
#endif
#ifndef TCPS_CLOSED
# define TCPS_CLOSED TCP_CLOSE
#endif
#ifndef TCPS_CLOSE_WAIT
# define TCPS_CLOSE_WAIT TCP_CLOSE_WAIT
#endif
#ifndef TCPS_LAST_ACK
# define TCPS_LAST_ACK TCP_LAST_ACK
#endif
#ifndef TCPS_LISTEN
# define TCPS_LISTEN TCP_LISTEN
#endif
#ifndef TCPS_CLOSING
# define TCPS_CLOSING TCP_CLOSING
#endif

DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry)
{
  if (!name)
    return ERROR_INVALID_PARAMETER;
  if (!entry)
    return ERROR_INVALID_PARAMETER;

  return NO_ERROR;
}

DWORD getICMPStats(MIB_ICMP *stats)
{
  FILE *fp;

  if (!stats)
    return ERROR_INVALID_PARAMETER;

  memset(stats, 0, sizeof(MIB_ICMP));
  /* get most of these stats from /proc/net/snmp, no error if can't */
  fp = fopen("/proc/net/snmp", "r");
  if (fp) {
    const char hdr[] = "Icmp:";
    char buf[512] = { 0 }, *ptr;

    do {
      ptr = fgets(buf, sizeof(buf), fp);
    } while (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1));
    if (ptr) {
      /* last line was a header, get another */
      ptr = fgets(buf, sizeof(buf), fp);
      if (ptr && strncasecmp(buf, hdr, sizeof(hdr) - 1) == 0) {
        char *endPtr;

        ptr += sizeof(hdr);
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwMsgs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwDestUnreachs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimeExcds = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwParmProbs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwSrcQuenchs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwRedirects = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwEchoReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimestamps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwTimestampReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwAddrMasks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpInStats.dwAddrMaskReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwMsgs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwErrors = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwDestUnreachs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimeExcds = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwParmProbs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwSrcQuenchs = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwRedirects = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwEchoReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimestamps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwTimestampReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwAddrMasks = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
        if (ptr && *ptr) {
          stats->stats.icmpOutStats.dwAddrMaskReps = strtoul(ptr, &endPtr, 10);
          ptr = endPtr;
        }
      }
    }
    fclose(fp);
  }
  return NO_ERROR;
}

DWORD getIPStats(PMIB_IPSTATS stats, DWORD family)
{
  if (!stats)
    return ERROR_INVALID_PARAMETER;
  return NO_ERROR;
}

DWORD getTCPStats(MIB_TCPSTATS *stats, DWORD family)
{
  if (!stats)
    return ERROR_INVALID_PARAMETER;
  return NO_ERROR;
}

DWORD getUDPStats(MIB_UDPSTATS *stats, DWORD family)
{
  if (!stats)
    return ERROR_INVALID_PARAMETER;
  return NO_ERROR;
}

static DWORD getNumWithOneHeader(const char *filename)
{
    return 0;
}

DWORD getNumRoutes(void)
{
    return 0;
}

RouteTable *getRouteTable(void)
{
    return (RouteTable *)"";
}

DWORD getNumArpEntries(void)
{
  return getNumWithOneHeader("/proc/net/arp");
}

PMIB_IPNETTABLE getArpTable(void)
{
    return 0;
}

DWORD getNumUdpEntries(void)
{
  return getNumWithOneHeader("/proc/net/udp");
}

PMIB_UDPTABLE getUdpTable(void)
{
  DWORD numEntries = getNumUdpEntries();
  PMIB_UDPTABLE ret;

  ret = (PMIB_UDPTABLE)calloc(1, sizeof(MIB_UDPTABLE) +
   (numEntries - 1) * sizeof(MIB_UDPROW));
  if (ret) {
    FILE *fp;

    /* get from /proc/net/udp, no error if can't */
    fp = fopen("/proc/net/udp", "r");
    if (fp) {
      char buf[512] = { 0 }, *ptr;

      /* skip header line */
      ptr = fgets(buf, sizeof(buf), fp);
      while (ptr && ret->dwNumEntries < numEntries) {
        ptr = fgets(buf, sizeof(buf), fp);
        if (ptr) {
          char *endPtr;

          if (ptr && *ptr) {
            strtoul(ptr, &endPtr, 16); /* skip */
            ptr = endPtr;
          }
          if (ptr && *ptr) {
            ptr++;
            ret->table[ret->dwNumEntries].dwLocalAddr = strtoul(ptr, &endPtr,
             16);
            ptr = endPtr;
          }
          if (ptr && *ptr) {
            ptr++;
            ret->table[ret->dwNumEntries].dwLocalPort = strtoul(ptr, &endPtr,
             16);
            ptr = endPtr;
          }
          ret->dwNumEntries++;
        }
      }
      fclose(fp);
    }
  }
  return ret;
}

DWORD getNumTcpEntries(void)
{
  return getNumWithOneHeader("/proc/net/tcp");
}

PMIB_TCPTABLE getTcpTable(void)
{
    return 0;
}
