/* Copyright (c) 2003 Juan Lang
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * I am heavily indebted to Chris Hertel's excellent Implementing CIFS,
 * http://ubiqx.org/cifs/ , for whatever understanding I have of NBT.
 * I also stole from Mike McCormack's smb.c and netapi32.c, although little of
 * that code remains.
 * Lack of understanding and bugs are my fault.
 *
 * FIXME:
 * - Of the NetBIOS session functions, only client functions are supported, and
 *   it's likely they'll be the only functions supported.  NBT requires session
 *   servers to listen on TCP/139.  This requires root privilege, and Samba is
 *   likely to be listening here already.  This further restricts NetBIOS
 *   applications, both explicit users and implicit ones:  CreateNamedPipe
 *   won't actually create a listening pipe, for example, so applications can't
 *   act as RPC servers using a named pipe protocol binding, DCOM won't be able
 *   to support callbacks or servers over the named pipe protocol, etc.
 *
 * - Datagram support is omitted for the same reason.  To send a NetBIOS
 *   datagram, you must include the NetBIOS name by which your application is
 *   known.  This requires you to have registered the name previously, and be
 *   able to act as a NetBIOS datagram server (listening on UDP/138).
 *
 * - Name registration functions are omitted for the same reason--registering a
 *   name requires you to be able to defend it, and this means listening on
 *   UDP/137.
 *   Win98 requires you either use your computer's NetBIOS name (with the NULL
 *   suffix byte) as the calling name when creating a session, or to register
 *   a new name before creating one:  it disallows '*' as the calling name.
 *   Win2K initially starts with an empty name table, and doesn't allow you to
 *   use the machine's NetBIOS name (with the NULL suffix byte) as the calling
 *   name.  Although it allows sessions to be created with '*' as the calling
 *   name, doing so results in timeouts for all receives, because the
 *   application never gets them.
 *   So, a well-behaved NetBIOS application will typically want to register a
 *   name.  I should probably support a do-nothing name list that allows
 *   NCBADDNAME to add to it, but doesn't actually register the name, or does
 *   attempt to register it without being able to defend it.
 *
 * - Name lookups may not behave quite as you'd expect/like if you have
 *   multiple LANAs.  If a name is resolvable through DNS, or if you're using
 *   WINS, it'll resolve on _any_ LANA.  So, a Call will succeed on any LANA as
 *   well.
 *   I'm not sure how Windows behaves in this case.  I could try to force
 *   lookups to the correct adapter by using one of the GetPreferred*
 *   functions, but with the possibility of multiple adapters in the same
 *   same subnet, there's no guarantee that what IpHlpApi thinks is the
 *   preferred adapter will actually be a LANA.  (It's highly probable because
 *   this is an unusual configuration, but not guaranteed.)
 *
 * See also other FIXMEs in the code.
 */

#include "netapi32.h"

#include <winsock2.h>
#include <winreg.h>

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

#define PORT_NBNS 137
#define PORT_NBDG 138
#define PORT_NBSS 139

#ifndef INADDR_NONE
#define INADDR_NONE ~0UL
#endif

#define NBR_ADDWORD(p,word) (*(WORD *)(p)) = htons(word)
#define NBR_GETWORD(p) ntohs(*(WORD *)(p))

#define MIN_QUERIES         1
#define MAX_QUERIES         0xffff
#define MIN_QUERY_TIMEOUT   100
#define MAX_QUERY_TIMEOUT   0xffffffff
#define BCAST_QUERIES       3
#define BCAST_QUERY_TIMEOUT 750
#define WINS_QUERIES        3
#define WINS_QUERY_TIMEOUT  750
#define MAX_WINS_SERVERS    2
#define MIN_CACHE_TIMEOUT   60000
#define CACHE_TIMEOUT       360000

#define MAX_NBT_NAME_SZ            255
#define SIMPLE_NAME_QUERY_PKT_SIZE 16 + MAX_NBT_NAME_SZ

#define NBNS_TYPE_NB             0x0020
#define NBNS_TYPE_NBSTAT         0x0021
#define NBNS_CLASS_INTERNET      0x00001
#define NBNS_HEADER_SIZE         (sizeof(WORD) * 6)
#define NBNS_RESPONSE_AND_OPCODE 0xf800
#define NBNS_RESPONSE_AND_QUERY  0x8000
#define NBNS_REPLYCODE           0x0f

#define NBSS_HDRSIZE 4

#define NBSS_MSG       0x00
#define NBSS_REQ       0x81
#define NBSS_ACK       0x82
#define NBSS_NACK      0x83
#define NBSS_RETARGET  0x84
#define NBSS_KEEPALIVE 0x85

#define NBSS_ERR_NOT_LISTENING_ON_NAME    0x80
#define NBSS_ERR_NOT_LISTENING_FOR_CALLER 0x81
#define NBSS_ERR_BAD_NAME                 0x82
#define NBSS_ERR_INSUFFICIENT_RESOURCES   0x83

#define NBSS_EXTENSION 0x01

typedef struct _NetBTSession
{
    CRITICAL_SECTION cs;
    SOCKET           fd;
    DWORD            bytesPending;
} NetBTSession;

typedef struct _NetBTAdapter
{
    MIB_IPADDRROW       ipr;
    WORD                nameQueryXID;
    struct NBNameCache *nameCache;
    DWORD               xmit_success;
    DWORD               recv_success;
} NetBTAdapter;

static ULONG gTransportID;
static BOOL  gEnableDNS;
static DWORD gBCastQueries;
static DWORD gBCastQueryTimeout;
static DWORD gWINSQueries;
static DWORD gWINSQueryTimeout;
static DWORD gWINSServers[MAX_WINS_SERVERS];
static int   gNumWINSServers;
static char  gScopeID[MAX_SCOPE_ID_LEN];
static DWORD gCacheTimeout;
static struct NBNameCache *gNameCache;

/* Converts from a NetBIOS name into a Second Level Encoding-formatted name.
 * Assumes p is not NULL and is either NULL terminated or has at most NCBNAMSZ
 * bytes, and buffer has at least MAX_NBT_NAME_SZ bytes.  Pads with space bytes
 * if p is NULL-terminated.  Returns the number of bytes stored in buffer.
 */
static int NetBTNameEncode(const UCHAR *p, UCHAR *buffer)
{
    int i,len=0;

    if (!p) return 0;
    if (!buffer) return 0;

    buffer[len++] = NCBNAMSZ * 2;
    for (i = 0; i < NCBNAMSZ && p[i]; i++)
    {
        buffer[len++] = ((p[i] & 0xf0) >> 4) + 'A';
        buffer[len++] =  (p[i] & 0x0f) + 'A';
    }
    while (len < NCBNAMSZ * 2)
    {
        buffer[len++] = 'C';
        buffer[len++] = 'A';
    }
    if (*gScopeID)
    {
        int scopeIDLen = strlen(gScopeID);

        memcpy(buffer + len, gScopeID, scopeIDLen);
        len += scopeIDLen;
    }
    buffer[len++] = 0;     /* add second terminator */
    return len;
}

/* Creates a NBT name request packet for name in buffer.  If broadcast is true,
 * creates a broadcast request, otherwise creates a unicast request.
 * Returns the number of bytes stored in buffer.
 */
static DWORD NetBTNameReq(const UCHAR name[NCBNAMSZ], WORD xid, WORD qtype,
 BOOL broadcast, UCHAR *buffer, int len)
{
    int i = 0;

    if (len < SIMPLE_NAME_QUERY_PKT_SIZE) return 0;

    NBR_ADDWORD(&buffer[i],xid);    i+=2; /* transaction */
    if (broadcast)
    {
        NBR_ADDWORD(&buffer[i],0x0110); /* flags: r=req,op=query,rd=1,b=1 */
        i+=2;
    }
    else
    {
        NBR_ADDWORD(&buffer[i],0x0100); /* flags: r=req,op=query,rd=1,b=0 */
        i+=2;
    }
    NBR_ADDWORD(&buffer[i],0x0001); i+=2; /* one name query */
    NBR_ADDWORD(&buffer[i],0x0000); i+=2; /* zero answers */
    NBR_ADDWORD(&buffer[i],0x0000); i+=2; /* zero authorities */
    NBR_ADDWORD(&buffer[i],0x0000); i+=2; /* zero additional */

    i += NetBTNameEncode(name, &buffer[i]);

    NBR_ADDWORD(&buffer[i],qtype); i+=2;
    NBR_ADDWORD(&buffer[i],NBNS_CLASS_INTERNET); i+=2;

    return i;
}

/* Sends a name query request for name on fd to destAddr.  Sets SO_BROADCAST on
 * fd if broadcast is TRUE.  Assumes fd is not INVALID_SOCKET, and name is not
 * NULL.
 * Returns 0 on success, -1 on failure.
 */
static int NetBTSendNameQuery(SOCKET fd, const UCHAR name[NCBNAMSZ], WORD xid,
 WORD qtype, DWORD destAddr, BOOL broadcast)
{
    int ret = 0, on = 1;
    struct in_addr addr;

    addr.s_addr = destAddr;
    TRACE("name %s, dest addr %s\n", name, inet_ntoa(addr));

    if (broadcast)
        ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on));
    if(ret == 0)
    {
        WSABUF wsaBuf;
        UCHAR buf[SIMPLE_NAME_QUERY_PKT_SIZE];
        struct sockaddr_in sin;

        memset(&sin, 0, sizeof(sin));
        sin.sin_addr.s_addr = destAddr;
        sin.sin_family      = AF_INET;
        sin.sin_port        = htons(PORT_NBNS);

        wsaBuf.buf = (CHAR*)buf;
        wsaBuf.len = NetBTNameReq(name, xid, qtype, broadcast, buf,
         sizeof(buf));
        if (wsaBuf.len > 0)
        {
            DWORD bytesSent;

            ret = WSASendTo(fd, &wsaBuf, 1, &bytesSent, 0,
             (struct sockaddr*)&sin, sizeof(sin), NULL, NULL);
            if (ret < 0 || bytesSent < wsaBuf.len)
                ret = -1;
            else
                ret = 0;
        }
        else
            ret = -1;
    }
    return ret;
}

typedef BOOL (*NetBTAnswerCallback)(void *data, WORD answerCount,
 WORD answerIndex, PUCHAR rData, WORD rdLength);

/* Waits on fd until GetTickCount() returns a value greater than or equal to
 * waitUntil for a name service response.  If a name response matching xid
 * is received, calls answerCallback once for each answer resource record in
 * the response.  (The callback's answerCount will be the total number of
 * answers to expect, and answerIndex will be the 0-based index that's being
 * sent this time.)  Quits parsing if answerCallback returns FALSE.
 * Returns NRC_GOODRET on timeout or a valid response received, something else
 * on error.
 */
static UCHAR NetBTWaitForNameResponse(const NetBTAdapter *adapter, SOCKET fd,
 DWORD waitUntil, NetBTAnswerCallback answerCallback, void *data)
{
    BOOL found = FALSE;
    DWORD now;
    UCHAR ret = NRC_GOODRET;

    if (!adapter) return NRC_BADDR;
    if (fd == INVALID_SOCKET) return NRC_BADDR;
    if (!answerCallback) return NRC_BADDR;

    while (!found && ret == NRC_GOODRET && (int)((now = GetTickCount()) - waitUntil) < 0)
    {
        DWORD msToWait = waitUntil - now;
        struct fd_set fds;
        struct timeval timeout = { msToWait / 1000, msToWait % 1000 };
        int r;

        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        r = select(fd + 1, &fds, NULL, NULL, &timeout);
        if (r < 0)
            ret = NRC_SYSTEM;
        else if (r == 1)
        {
            /* FIXME: magic #, is this always enough? */
            UCHAR buffer[256];
            int fromsize;
            struct sockaddr_in fromaddr;
            WORD respXID, flags, queryCount, answerCount;
            WSABUF wsaBuf = { sizeof(buffer), (CHAR*)buffer };
            DWORD bytesReceived, recvFlags = 0;

            fromsize = sizeof(fromaddr);
            r = WSARecvFrom(fd, &wsaBuf, 1, &bytesReceived, &recvFlags,
             (struct sockaddr*)&fromaddr, &fromsize, NULL, NULL);
            if(r < 0)
            {
                ret = NRC_SYSTEM;
                break;
            }

            if (bytesReceived < NBNS_HEADER_SIZE)
                continue;

            respXID = NBR_GETWORD(buffer);
            if (adapter->nameQueryXID != respXID)
                continue;

            flags = NBR_GETWORD(buffer + 2);
            queryCount = NBR_GETWORD(buffer + 4);
            answerCount = NBR_GETWORD(buffer + 6);

            /* a reply shouldn't contain a query, ignore bad packet */
            if (queryCount > 0)
                continue;

            if ((flags & NBNS_RESPONSE_AND_OPCODE) == NBNS_RESPONSE_AND_QUERY)
            {
                if ((flags & NBNS_REPLYCODE) != 0)
                    ret = NRC_NAMERR;
                else if ((flags & NBNS_REPLYCODE) == 0 && answerCount > 0)
                {
                    PUCHAR ptr = buffer + NBNS_HEADER_SIZE;
                    BOOL shouldContinue = TRUE;
                    WORD answerIndex = 0;

                    found = TRUE;
                    /* decode one answer at a time */
                    while (ret == NRC_GOODRET && answerIndex < answerCount &&
                     ptr - buffer < bytesReceived && shouldContinue)
                    {
                        WORD rLen;

                        /* scan past name */
                        for (; ptr[0] && ptr - buffer < bytesReceived; )
                            ptr += ptr[0] + 1;
                        ptr++;
                        ptr += 2; /* scan past type */
                        if (ptr - buffer < bytesReceived && ret == NRC_GOODRET
                         && NBR_GETWORD(ptr) == NBNS_CLASS_INTERNET)
                            ptr += sizeof(WORD);
                        else
                            ret = NRC_SYSTEM; /* parse error */
                        ptr += sizeof(DWORD); /* TTL */
                        rLen = NBR_GETWORD(ptr);
                        rLen = min(rLen, bytesReceived - (ptr - buffer));
                        ptr += sizeof(WORD);
                        shouldContinue = answerCallback(data, answerCount,
                         answerIndex, ptr, rLen);
                        ptr += rLen;
                        answerIndex++;
                    }
                }
            }
        }
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

typedef struct _NetBTNameQueryData {
    NBNameCacheEntry *cacheEntry;
    UCHAR ret;
} NetBTNameQueryData;

/* Name query callback function for NetBTWaitForNameResponse, creates a cache
 * entry on the first answer, adds each address as it's called again (as long
 * as there's space).  If there's an error that should be propagated as the
 * NetBIOS error, modifies queryData's ret member to the proper return code.
 */
static BOOL NetBTFindNameAnswerCallback(void *pVoid, WORD answerCount,
 WORD answerIndex, PUCHAR rData, WORD rLen)
{
    NetBTNameQueryData *queryData = pVoid;
    BOOL ret;

    if (queryData)
    {
        if (queryData->cacheEntry == NULL)
        {
            queryData->cacheEntry = HeapAlloc(GetProcessHeap(), 0,
             FIELD_OFFSET(NBNameCacheEntry, addresses[answerCount]));
            if (queryData->cacheEntry)
                queryData->cacheEntry->numAddresses = 0;
            else
                queryData->ret = NRC_OSRESNOTAV;
        }
        if (rLen == 6 && queryData->cacheEntry &&
         queryData->cacheEntry->numAddresses < answerCount)
        {
            queryData->cacheEntry->addresses[queryData->cacheEntry->
             numAddresses++] = *(const DWORD *)(rData + 2);
            ret = queryData->cacheEntry->numAddresses < answerCount;
        }
        else
            ret = FALSE;
    }
    else
        ret = FALSE;
    return ret;
}

/* Workhorse NetBT name lookup function.  Sends a name lookup query for
 * ncb->ncb_callname to sendTo, as a broadcast if broadcast is TRUE, using
 * adapter->nameQueryXID as the transaction ID.  Waits up to timeout
 * milliseconds, and retries up to maxQueries times, waiting for a reply.
 * If a valid response is received, stores the looked up addresses as a
 * NBNameCacheEntry in *cacheEntry.
 * Returns NRC_GOODRET on success, though this may not mean the name was
 * resolved--check whether *cacheEntry is NULL.
 */
static UCHAR NetBTNameWaitLoop(const NetBTAdapter *adapter, SOCKET fd, const NCB *ncb,
 DWORD sendTo, BOOL broadcast, DWORD timeout, DWORD maxQueries,
 NBNameCacheEntry **cacheEntry)
{
    unsigned int queries;
    NetBTNameQueryData queryData;

    if (!adapter) return NRC_BADDR;
    if (fd == INVALID_SOCKET) return NRC_BADDR;
    if (!ncb) return NRC_BADDR;
    if (!cacheEntry) return NRC_BADDR;

    queryData.cacheEntry = NULL;
    queryData.ret = NRC_GOODRET;
    for (queries = 0; queryData.cacheEntry == NULL && queries < maxQueries;
     queries++)
    {
        if (!NCB_CANCELLED(ncb))
        {
            int r = NetBTSendNameQuery(fd, ncb->ncb_callname,
             adapter->nameQueryXID, NBNS_TYPE_NB, sendTo, broadcast);

            if (r == 0)
                queryData.ret = NetBTWaitForNameResponse(adapter, fd,
                 GetTickCount() + timeout, NetBTFindNameAnswerCallback,
                 &queryData);
            else
                queryData.ret = NRC_SYSTEM;
        }
        else
            queryData.ret = NRC_CMDCAN;
    }
    if (queryData.cacheEntry)
    {
        memcpy(queryData.cacheEntry->name, ncb->ncb_callname, NCBNAMSZ);
        memcpy(queryData.cacheEntry->nbname, ncb->ncb_callname, NCBNAMSZ);
    }
    *cacheEntry = queryData.cacheEntry;
    return queryData.ret;
}

/* Attempts to add cacheEntry to the name cache in *nameCache; if *nameCache
 * has not yet been created, creates it, using gCacheTimeout as the cache
 * entry timeout.  If memory allocation fails, or if NBNameCacheAddEntry fails,
 * frees cacheEntry.
 * Returns NRC_GOODRET on success, and something else on failure.
 */
static UCHAR NetBTStoreCacheEntry(struct NBNameCache **nameCache,
 NBNameCacheEntry *cacheEntry)
{
    UCHAR ret;

    if (!nameCache) return NRC_BADDR;
    if (!cacheEntry) return NRC_BADDR;

    if (!*nameCache)
        *nameCache = NBNameCacheCreate(GetProcessHeap(), gCacheTimeout);
    if (*nameCache)
        ret = NBNameCacheAddEntry(*nameCache, cacheEntry)
         ?  NRC_GOODRET : NRC_OSRESNOTAV;
    else
    {
        HeapFree(GetProcessHeap(), 0, cacheEntry);
        ret = NRC_OSRESNOTAV;
    }
    return ret;
}

/* Attempts to resolve name using inet_addr(), then gethostbyname() if
 * gEnableDNS is TRUE, if the suffix byte is either <00> or <20>.  If the name
 * can be looked up, returns 0 and stores the looked up addresses as a
 * NBNameCacheEntry in *cacheEntry.
 * Returns NRC_GOODRET on success, though this may not mean the name was
 * resolved--check whether *cacheEntry is NULL.  Returns something else on
 * error.
 */
static UCHAR NetBTinetResolve(const UCHAR name[NCBNAMSZ],
 NBNameCacheEntry **cacheEntry)
{
    UCHAR ret = NRC_GOODRET;

    TRACE("name %s, cacheEntry %p\n", name, cacheEntry);

    if (!name) return NRC_BADDR;
    if (!cacheEntry) return NRC_BADDR;

    if (isalnum(name[0]) && (name[NCBNAMSZ - 1] == 0 ||
     name[NCBNAMSZ - 1] == 0x20))
    {
        CHAR toLookup[NCBNAMSZ];
        unsigned int i;

        for (i = 0; i < NCBNAMSZ - 1 && name[i] && name[i] != ' '; i++)
            toLookup[i] = name[i];
        toLookup[i] = '\0';

        if (isdigit(toLookup[0]))
        {
            unsigned long addr = inet_addr(toLookup);

            if (addr != INADDR_NONE)
            {
                *cacheEntry = HeapAlloc(GetProcessHeap(), 0,
                 FIELD_OFFSET(NBNameCacheEntry, addresses[1]));
                if (*cacheEntry)
                {
                    memcpy((*cacheEntry)->name, name, NCBNAMSZ);
                    memset((*cacheEntry)->nbname, 0, NCBNAMSZ);
                    (*cacheEntry)->nbname[0] = '*';
                    (*cacheEntry)->numAddresses = 1;
                    (*cacheEntry)->addresses[0] = addr;
                }
                else
                    ret = NRC_OSRESNOTAV;
            }
        }
        if (gEnableDNS && ret == NRC_GOODRET && !*cacheEntry)
        {
            struct hostent *host;

            if ((host = gethostbyname(toLookup)) != NULL)
            {
                for (i = 0; ret == NRC_GOODRET && host->h_addr_list &&
                 host->h_addr_list[i]; i++)
                    ;
                if (host->h_addr_list && host->h_addr_list[0])
                {
                    *cacheEntry = HeapAlloc(GetProcessHeap(), 0,
                     FIELD_OFFSET(NBNameCacheEntry, addresses[i]));
                    if (*cacheEntry)
                    {
                        memcpy((*cacheEntry)->name, name, NCBNAMSZ);
                        memset((*cacheEntry)->nbname, 0, NCBNAMSZ);
                        (*cacheEntry)->nbname[0] = '*';
                        (*cacheEntry)->numAddresses = i;
                        for (i = 0; i < (*cacheEntry)->numAddresses; i++)
                            (*cacheEntry)->addresses[i] =
                             *(DWORD*)host->h_addr_list[i];
                    }
                    else
                        ret = NRC_OSRESNOTAV;
                }
            }
        }
    }

    TRACE("returning 0x%02x\n", ret);
    return ret;
}

/* Looks up the name in ncb->ncb_callname, first in the name caches (global
 * and this adapter's), then using gethostbyname(), next by WINS if configured,
 * and finally using broadcast NetBT name resolution.  In NBT parlance, this
 * makes this an "H-node".  Stores an entry in the appropriate name cache for a
 * found node, and returns it as *cacheEntry.
 * Assumes data, ncb, and cacheEntry are not NULL.
 * Returns NRC_GOODRET on success--which doesn't mean the name was resolved,
 * just that all name lookup operations completed successfully--and something
 * else on failure.  *cacheEntry will be NULL if the name was not found.
 */
static UCHAR NetBTInternalFindName(NetBTAdapter *adapter, PNCB ncb,
 const NBNameCacheEntry **cacheEntry)
{
    UCHAR ret = NRC_GOODRET;

    TRACE("adapter %p, ncb %p, cacheEntry %p\n", adapter, ncb, cacheEntry);

    if (!cacheEntry) return NRC_BADDR;
    *cacheEntry = NULL;

    if (!adapter) return NRC_BADDR;
    if (!ncb) return NRC_BADDR;

    if (ncb->ncb_callname[0] == '*')
        ret = NRC_NOWILD;
    else
    {
        *cacheEntry = NBNameCacheFindEntry(gNameCache, ncb->ncb_callname);
        if (!*cacheEntry)
            *cacheEntry = NBNameCacheFindEntry(adapter->nameCache,
             ncb->ncb_callname);
        if (!*cacheEntry)
        {
            NBNameCacheEntry *newEntry = NULL;

            ret = NetBTinetResolve(ncb->ncb_callname, &newEntry);
            if (ret == NRC_GOODRET && newEntry)
            {
                ret = NetBTStoreCacheEntry(&gNameCache, newEntry);
                if (ret != NRC_GOODRET)
                    newEntry = NULL;
            }
            else
            {
                SOCKET fd = WSASocketA(PF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL,
                 0, WSA_FLAG_OVERLAPPED);

                if(fd == INVALID_SOCKET)
                    ret = NRC_OSRESNOTAV;
                else
                {
                    int winsNdx;

                    adapter->nameQueryXID++;
                    for (winsNdx = 0; ret == NRC_GOODRET && *cacheEntry == NULL
                     && winsNdx < gNumWINSServers; winsNdx++)
                        ret = NetBTNameWaitLoop(adapter, fd, ncb,
                         gWINSServers[winsNdx], FALSE, gWINSQueryTimeout,
                         gWINSQueries, &newEntry);
                    if (ret == NRC_GOODRET && newEntry)
                    {
                        ret = NetBTStoreCacheEntry(&gNameCache, newEntry);
                        if (ret != NRC_GOODRET)
                            newEntry = NULL;
                    }
                    if (ret == NRC_GOODRET && *cacheEntry == NULL)
                    {
                        DWORD bcastAddr =
                         adapter->ipr.dwAddr & adapter->ipr.dwMask;

                        if (adapter->ipr.dwBCastAddr)
                            bcastAddr |= ~adapter->ipr.dwMask;
                        ret = NetBTNameWaitLoop(adapter, fd, ncb, bcastAddr,
                         TRUE, gBCastQueryTimeout, gBCastQueries, &newEntry);
                        if (ret == NRC_GOODRET && newEntry)
                        {
                            ret = NetBTStoreCacheEntry(&adapter->nameCache,
                             newEntry);
                            if (ret != NRC_GOODRET)
                                newEntry = NULL;
                        }
                    }
                    closesocket(fd);
                }
            }
            *cacheEntry = newEntry;
        }
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

typedef struct _NetBTNodeQueryData
{
    BOOL gotResponse;
    PADAPTER_STATUS astat;
    WORD astatLen;
} NetBTNodeQueryData;

/* Callback function for NetBTAstatRemote, parses the rData for the node
 * status and name list of the remote node.  Always returns FALSE, since
 * there's never more than one answer we care about in a node status response.
 */
static BOOL NetBTNodeStatusAnswerCallback(void *pVoid, WORD answerCount,
 WORD answerIndex, PUCHAR rData, WORD rLen)
{
    NetBTNodeQueryData *data = pVoid;

    if (data && !data->gotResponse && rData && rLen >= 1)
    {
        /* num names is first byte; each name is NCBNAMSZ + 2 bytes */
        if (rLen >= rData[0] * (NCBNAMSZ + 2))
        {
            WORD i;
            PUCHAR src;
            PNAME_BUFFER dst;

            data->gotResponse = TRUE;
            data->astat->name_count = rData[0];
            for (i = 0, src = rData + 1,
             dst = (PNAME_BUFFER)((PUCHAR)data->astat +
              sizeof(ADAPTER_STATUS));
             i < data->astat->name_count && src - rData < rLen &&
             (PUCHAR)dst - (PUCHAR)data->astat < data->astatLen;
             i++, dst++, src += NCBNAMSZ + 2)
            {
                UCHAR flags = *(src + NCBNAMSZ);

                memcpy(dst->name, src, NCBNAMSZ);
                /* we won't actually see a registering name in the returned
                 * response.  It's useful to see if no other flags are set; if
                 * none are, then the name is registered. */
                dst->name_flags = REGISTERING;
                if (flags & 0x80)
                    dst->name_flags |= GROUP_NAME;
                if (flags & 0x10)
                    dst->name_flags |= DEREGISTERED;
                if (flags & 0x08)
                    dst->name_flags |= DUPLICATE;
                if (dst->name_flags == REGISTERING)
                    dst->name_flags = REGISTERED;
            }
            /* arbitrarily set HW type to Ethernet */
            data->astat->adapter_type = 0xfe;
            if (src - rData < rLen)
                memcpy(data->astat->adapter_address, src,
                 min(rLen - (src - rData), 6));
        }
    }
    return FALSE;
}

/* This uses the WINS timeout and query values, as they're the
 * UCAST_REQ_RETRY_TIMEOUT and UCAST_REQ_RETRY_COUNT according to the RFCs.
 */
static UCHAR NetBTAstatRemote(NetBTAdapter *adapter, PNCB ncb)
{
    UCHAR ret = NRC_GOODRET;
    const NBNameCacheEntry *cacheEntry = NULL;

    TRACE("adapter %p, NCB %p\n", adapter, ncb);

    if (!adapter) return NRC_BADDR;
    if (!ncb) return NRC_INVADDRESS;

    ret = NetBTInternalFindName(adapter, ncb, &cacheEntry);
    if (ret == NRC_GOODRET && cacheEntry)
    {
        if (cacheEntry->numAddresses > 0)
        {
            SOCKET fd = WSASocketA(PF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0,
             WSA_FLAG_OVERLAPPED);

            if(fd == INVALID_SOCKET)
                ret = NRC_OSRESNOTAV;
            else
            {
                NetBTNodeQueryData queryData;
                DWORD queries;
                PADAPTER_STATUS astat = (PADAPTER_STATUS)ncb->ncb_buffer;

                adapter->nameQueryXID++;
                astat->name_count = 0;
                queryData.gotResponse = FALSE;
                queryData.astat = astat;
                queryData.astatLen = ncb->ncb_length;
                for (queries = 0; !queryData.gotResponse &&
                 queries < gWINSQueries; queries++)
                {
                    if (!NCB_CANCELLED(ncb))
                    {
                        int r = NetBTSendNameQuery(fd, ncb->ncb_callname,
                         adapter->nameQueryXID, NBNS_TYPE_NBSTAT,
                         cacheEntry->addresses[0], FALSE);

                        if (r == 0)
                            ret = NetBTWaitForNameResponse(adapter, fd,
                             GetTickCount() + gWINSQueryTimeout,
                             NetBTNodeStatusAnswerCallback, &queryData);
                        else
                            ret = NRC_SYSTEM;
                    }
                    else
                        ret = NRC_CMDCAN;
                }
                closesocket(fd);
            }
        }
        else
            ret = NRC_CMDTMO;
    }
    else if (ret == NRC_CMDCAN)
        ; /* do nothing, we were cancelled */
    else
        ret = NRC_CMDTMO;
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTAstat(void *adapt, PNCB ncb)
{
    NetBTAdapter *adapter = adapt;
    UCHAR ret;

    TRACE("adapt %p, NCB %p\n", adapt, ncb);

    if (!adapter) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_INVADDRESS;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (ncb->ncb_length < sizeof(ADAPTER_STATUS)) return NRC_BUFLEN;

    if (ncb->ncb_callname[0] == '*')
    {
        DWORD physAddrLen;
        MIB_IFROW ifRow;
        PADAPTER_STATUS astat = (PADAPTER_STATUS)ncb->ncb_buffer;
  
        memset(astat, 0, sizeof(ADAPTER_STATUS));
        astat->rev_major = 3;
        ifRow.dwIndex = adapter->ipr.dwIndex;
        if (GetIfEntry(&ifRow) != NO_ERROR)
            ret = NRC_BRIDGE;
        else
        {
            physAddrLen = min(ifRow.dwPhysAddrLen, 6);
            if (physAddrLen > 0)
                memcpy(astat->adapter_address, ifRow.bPhysAddr, physAddrLen);
            /* doubt anyone cares, but why not.. */
            if (ifRow.dwType == MIB_IF_TYPE_TOKENRING)
                astat->adapter_type = 0xff;
            else
                astat->adapter_type = 0xfe; /* for Ethernet */
            astat->max_sess_pkt_size = 0xffff;
            astat->xmit_success = adapter->xmit_success;
            astat->recv_success = adapter->recv_success;
            ret = NRC_GOODRET;
        }
    }
    else
        ret = NetBTAstatRemote(adapter, ncb);
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTFindName(void *adapt, PNCB ncb)
{
    NetBTAdapter *adapter = adapt;
    UCHAR ret;
    const NBNameCacheEntry *cacheEntry = NULL;
    PFIND_NAME_HEADER foundName;

    TRACE("adapt %p, NCB %p\n", adapt, ncb);

    if (!adapter) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_INVADDRESS;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (ncb->ncb_length < sizeof(FIND_NAME_HEADER)) return NRC_BUFLEN;

    foundName = (PFIND_NAME_HEADER)ncb->ncb_buffer;
    memset(foundName, 0, sizeof(FIND_NAME_HEADER));

    ret = NetBTInternalFindName(adapter, ncb, &cacheEntry);
    if (ret == NRC_GOODRET)
    {
        if (cacheEntry)
        {
            DWORD spaceFor = min((ncb->ncb_length - sizeof(FIND_NAME_HEADER)) /
             sizeof(FIND_NAME_BUFFER), cacheEntry->numAddresses);
            DWORD ndx;

            for (ndx = 0; ndx < spaceFor; ndx++)
            {
                PFIND_NAME_BUFFER findNameBuffer;

                findNameBuffer =
                 (PFIND_NAME_BUFFER)((PUCHAR)foundName +
                 sizeof(FIND_NAME_HEADER) + foundName->node_count *
                 sizeof(FIND_NAME_BUFFER));
                memset(findNameBuffer->destination_addr, 0, 2);
                memcpy(findNameBuffer->destination_addr + 2,
                 &adapter->ipr.dwAddr, sizeof(DWORD));
                memset(findNameBuffer->source_addr, 0, 2);
                memcpy(findNameBuffer->source_addr + 2,
                 &cacheEntry->addresses[ndx], sizeof(DWORD));
                foundName->node_count++;
            }
            if (spaceFor < cacheEntry->numAddresses)
                ret = NRC_BUFLEN;
        }
        else
            ret = NRC_CMDTMO;
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTSessionReq(SOCKET fd, const UCHAR *calledName,
 const UCHAR *callingName)
{
    UCHAR buffer[NBSS_HDRSIZE + MAX_DOMAIN_NAME_LEN * 2], ret;
    int r;
    unsigned int len = 0;
    DWORD bytesSent, bytesReceived, recvFlags = 0;
    WSABUF wsaBuf;

    buffer[0] = NBSS_REQ;
    buffer[1] = 0;

    len += NetBTNameEncode(calledName, &buffer[NBSS_HDRSIZE]);
    len += NetBTNameEncode(callingName, &buffer[NBSS_HDRSIZE + len]);

    NBR_ADDWORD(&buffer[2], len);

    wsaBuf.len = len + NBSS_HDRSIZE;
    wsaBuf.buf = (char*)buffer;

    r = WSASend(fd, &wsaBuf, 1, &bytesSent, 0, NULL, NULL);
    if(r < 0 || bytesSent < len + NBSS_HDRSIZE)
    {
        ERR("send failed\n");
        return NRC_SABORT;
    }

    /* I've already set the recv timeout on this socket (if it supports it), so
     * just block.  Hopefully we'll always receive the session acknowledgement
     * within one timeout.
     */
    wsaBuf.len = NBSS_HDRSIZE + 1;
    r = WSARecv(fd, &wsaBuf, 1, &bytesReceived, &recvFlags, NULL, NULL);
    if (r < 0 || bytesReceived < NBSS_HDRSIZE)
        ret = NRC_SABORT;
    else if (buffer[0] == NBSS_NACK)
    {
        if (r == NBSS_HDRSIZE + 1)
        {
            switch (buffer[NBSS_HDRSIZE])
            {
                case NBSS_ERR_INSUFFICIENT_RESOURCES:
                    ret = NRC_REMTFUL;
                    break;
                default:
                    ret = NRC_NOCALL;
            }
        }
        else
            ret = NRC_NOCALL;
    }
    else if (buffer[0] == NBSS_RETARGET)
    {
        FIXME("Got a session retarget, can't deal\n");
        ret = NRC_NOCALL;
    }
    else if (buffer[0] == NBSS_ACK)
        ret = NRC_GOODRET;
    else
        ret = NRC_SYSTEM;

    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTCall(void *adapt, PNCB ncb, void **sess)
{
    NetBTAdapter *adapter = adapt;
    UCHAR ret;
    const NBNameCacheEntry *cacheEntry = NULL;

    TRACE("adapt %p, ncb %p\n", adapt, ncb);

    if (!adapter) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_INVADDRESS;
    if (!sess) return NRC_BADDR;

    ret = NetBTInternalFindName(adapter, ncb, &cacheEntry);
    if (ret == NRC_GOODRET)
    {
        if (cacheEntry && cacheEntry->numAddresses > 0)
        {
            SOCKET fd;

            fd = WSASocketA(PF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0,
             WSA_FLAG_OVERLAPPED);
            if (fd != INVALID_SOCKET)
            {
                DWORD timeout;
                struct sockaddr_in sin;

                if (ncb->ncb_rto > 0)
                {
                    timeout = ncb->ncb_rto * 500;
                    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                     sizeof(timeout));
                }
                if (ncb->ncb_sto > 0)
                {
                    timeout = ncb->ncb_sto * 500;
                    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,
                     sizeof(timeout));
                }

                memset(&sin, 0, sizeof(sin));
                memcpy(&sin.sin_addr, &cacheEntry->addresses[0],
                 sizeof(sin.sin_addr));
                sin.sin_family = AF_INET;
                sin.sin_port   = htons(PORT_NBSS);
                /* FIXME: use nonblocking mode for the socket, check the
                 * cancel flag periodically
                 */
                if (connect(fd, (struct sockaddr *)&sin, sizeof(sin))
                 == SOCKET_ERROR)
                    ret = NRC_CMDTMO;
                else
                {
                    static const UCHAR fakedCalledName[] = "*SMBSERVER";
                    const UCHAR *calledParty = cacheEntry->nbname[0] == '*'
                     ? fakedCalledName : cacheEntry->nbname;

                    ret = NetBTSessionReq(fd, calledParty, ncb->ncb_name);
                    if (ret != NRC_GOODRET && calledParty[0] == '*')
                    {
                        FIXME("NBT session to \"*SMBSERVER\" refused,\n");
                        FIXME("should try finding name using ASTAT\n");
                    }
                }
                if (ret != NRC_GOODRET)
                    closesocket(fd);
                else
                {
                    NetBTSession *session = HeapAlloc(
                     GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NetBTSession));

                    if (session)
                    {
                        session->fd = fd;
                        InitializeCriticalSection(&session->cs);
                        session->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NetBTSession.cs");
                        *sess = session;
                    }
                    else
                    {
                        ret = NRC_OSRESNOTAV;
                        closesocket(fd);
                    }
                }
            }
            else
                ret = NRC_OSRESNOTAV;
        }
        else
            ret = NRC_NAMERR;
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

/* Notice that I don't protect against multiple thread access to NetBTSend.
 * This is because I don't update any data in the adapter, and I only make a
 * single call to WSASend, which I assume to act atomically (not interleaving
 * data from other threads).
 * I don't lock, because I only depend on the fd being valid, and this won't be
 * true until a session setup is completed.
 */
static UCHAR NetBTSend(void *adapt, void *sess, PNCB ncb)
{
    NetBTAdapter *adapter = adapt;
    NetBTSession *session = sess;
    UCHAR buffer[NBSS_HDRSIZE], ret;
    int r;
    WSABUF wsaBufs[2];
    DWORD bytesSent;

    TRACE("adapt %p, session %p, NCB %p\n", adapt, session, ncb);

    if (!adapter) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_INVADDRESS;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (!session) return NRC_SNUMOUT;
    if (session->fd == INVALID_SOCKET) return NRC_SNUMOUT;

    buffer[0] = NBSS_MSG;
    buffer[1] = 0;
    NBR_ADDWORD(&buffer[2], ncb->ncb_length);

    wsaBufs[0].len = NBSS_HDRSIZE;
    wsaBufs[0].buf = (char*)buffer;
    wsaBufs[1].len = ncb->ncb_length;
    wsaBufs[1].buf = (char*)ncb->ncb_buffer;

    r = WSASend(session->fd, wsaBufs, sizeof(wsaBufs) / sizeof(wsaBufs[0]),
     &bytesSent, 0, NULL, NULL);
    if (r == SOCKET_ERROR)
    {
        NetBIOSHangupSession(ncb);
        ret = NRC_SABORT;
    }
    else if (bytesSent < NBSS_HDRSIZE + ncb->ncb_length)
    {
        FIXME("Only sent %d bytes (of %d), hanging up session\n", bytesSent,
         NBSS_HDRSIZE + ncb->ncb_length);
        NetBIOSHangupSession(ncb);
        ret = NRC_SABORT;
    }
    else
    {
        ret = NRC_GOODRET;
        adapter->xmit_success++;
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTRecv(void *adapt, void *sess, PNCB ncb)
{
    NetBTAdapter *adapter = adapt;
    NetBTSession *session = sess;
    UCHAR buffer[NBSS_HDRSIZE], ret;
    int r;
    WSABUF wsaBufs[2];
    DWORD bufferCount, bytesReceived, flags;

    TRACE("adapt %p, session %p, NCB %p\n", adapt, session, ncb);

    if (!adapter) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_BADDR;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (!session) return NRC_SNUMOUT;
    if (session->fd == INVALID_SOCKET) return NRC_SNUMOUT;

    EnterCriticalSection(&session->cs);
    bufferCount = 0;
    if (session->bytesPending == 0)
    {
        bufferCount++;
        wsaBufs[0].len = NBSS_HDRSIZE;
        wsaBufs[0].buf = (char*)buffer;
    }
    wsaBufs[bufferCount].len = ncb->ncb_length;
    wsaBufs[bufferCount].buf = (char*)ncb->ncb_buffer;
    bufferCount++;

    flags = 0;
    /* FIXME: should poll a bit so I can check the cancel flag */
    r = WSARecv(session->fd, wsaBufs, bufferCount, &bytesReceived, &flags,
     NULL, NULL);
    if (r == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
    {
        LeaveCriticalSection(&session->cs);
        ERR("Receive error, WSAGetLastError() returns %d\n", WSAGetLastError());
        NetBIOSHangupSession(ncb);
        ret = NRC_SABORT;
    }
    else if (NCB_CANCELLED(ncb))
    {
        LeaveCriticalSection(&session->cs);
        ret = NRC_CMDCAN;
    }
    else
    {
        if (bufferCount == 2)
        {
            if (buffer[0] == NBSS_KEEPALIVE)
            {
                LeaveCriticalSection(&session->cs);
                FIXME("Oops, received a session keepalive and lost my place\n");
                /* need to read another session header until we get a session
                 * message header. */
                NetBIOSHangupSession(ncb);
                ret = NRC_SABORT;
                goto error;
            }
            else if (buffer[0] != NBSS_MSG)
            {
                LeaveCriticalSection(&session->cs);
                FIXME("Received unexpected session msg type %d\n", buffer[0]);
                NetBIOSHangupSession(ncb);
                ret = NRC_SABORT;
                goto error;
            }
            else
            {
                if (buffer[1] & NBSS_EXTENSION)
                {
                    LeaveCriticalSection(&session->cs);
                    FIXME("Received a message that's too long for my taste\n");
                    NetBIOSHangupSession(ncb);
                    ret = NRC_SABORT;
                    goto error;
                }
                else
                {
                    session->bytesPending = NBSS_HDRSIZE
                     + NBR_GETWORD(&buffer[2]) - bytesReceived;
                    ncb->ncb_length = bytesReceived - NBSS_HDRSIZE;
                    LeaveCriticalSection(&session->cs);
                }
            }
        }
        else
        {
            if (bytesReceived < session->bytesPending)
                session->bytesPending -= bytesReceived;
            else
                session->bytesPending = 0;
            LeaveCriticalSection(&session->cs);
            ncb->ncb_length = bytesReceived;
        }
        if (session->bytesPending > 0)
            ret = NRC_INCOMP;
        else
        {
            ret = NRC_GOODRET;
            adapter->recv_success++;
        }
    }
error:
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR NetBTHangup(void *adapt, void *sess)
{
    NetBTSession *session = sess;

    TRACE("adapt %p, session %p\n", adapt, session);

    if (!session) return NRC_SNUMOUT;

    /* I don't lock the session, because NetBTRecv knows not to decrement
     * past 0, so if a receive completes after this it should still deal.
     */
    closesocket(session->fd);
    session->fd = INVALID_SOCKET;
    session->bytesPending = 0;
    session->cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&session->cs);
    HeapFree(GetProcessHeap(), 0, session);

    return NRC_GOODRET;
}

static void NetBTCleanupAdapter(void *adapt)
{
    TRACE("adapt %p\n", adapt);
    if (adapt)
    {
        NetBTAdapter *adapter = adapt;

        if (adapter->nameCache)
            NBNameCacheDestroy(adapter->nameCache);
        HeapFree(GetProcessHeap(), 0, adapt);
    }
}

static void NetBTCleanup(void)
{
    TRACE("\n");
    if (gNameCache)
    {
        NBNameCacheDestroy(gNameCache);
        gNameCache = NULL;
    }
}

static UCHAR NetBTRegisterAdapter(const MIB_IPADDRROW *ipRow)
{
    UCHAR ret;
    NetBTAdapter *adapter;

    if (!ipRow) return NRC_BADDR;

    adapter = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(NetBTAdapter));
    if (adapter)
    {
        adapter->ipr = *ipRow;
        if (!NetBIOSRegisterAdapter(gTransportID, ipRow->dwIndex, adapter))
        {
            NetBTCleanupAdapter(adapter);
            ret = NRC_SYSTEM;
        }
        else
            ret = NRC_GOODRET;
    }
    else
        ret = NRC_OSRESNOTAV;
    return ret;
}

/* Callback for NetBIOS adapter enumeration.  Assumes closure is a pointer to
 * a MIB_IPADDRTABLE containing all the IP adapters needed to be added to the
 * NetBIOS adapter table.  For each callback, checks if the passed-in adapt
 * has an entry in the table; if so, this adapter was enumerated previously,
 * and it's enabled.  As a flag, the table's dwAddr entry is changed to
 * INADDR_LOOPBACK, since this is an invalid address for a NetBT adapter.
 * The NetBTEnum function will add any remaining adapters from the
 * MIB_IPADDRTABLE to the NetBIOS adapter table.
 */
static BOOL NetBTEnumCallback(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure)
{
    BOOL ret;
    PMIB_IPADDRTABLE table = closure;

    if (table && data)
    {
        DWORD ndx;

        ret = FALSE;
        for (ndx = 0; !ret && ndx < table->dwNumEntries; ndx++)
        {
            const NetBTAdapter *adapter = data->data;

            if (table->table[ndx].dwIndex == adapter->ipr.dwIndex)
            {
                NetBIOSEnableAdapter(data->lana);
                table->table[ndx].dwAddr = INADDR_LOOPBACK;
                ret = TRUE;
            }
        }
    }
    else
        ret = FALSE;
    return ret;
}

/* Enumerates adapters by:
 * - retrieving the IP address table for the local machine
 * - eliminating loopback addresses from the table
 * - eliminating redundant addresses, that is, multiple addresses on the same
 *   subnet
 * Calls NetBIOSEnumAdapters, passing the resulting table as the callback
 * data.  The callback reenables each adapter that's already in the NetBIOS
 * table.  After NetBIOSEnumAdapters returns, this function adds any remaining
 * adapters to the NetBIOS table.
 */
static UCHAR NetBTEnum(void)
{
    UCHAR ret;
    DWORD size = 0;

    TRACE("\n");

    if (GetIpAddrTable(NULL, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER)
    {
        PMIB_IPADDRTABLE ipAddrs, coalesceTable = NULL;
        DWORD numIPAddrs = (size - sizeof(MIB_IPADDRTABLE)) /
         sizeof(MIB_IPADDRROW) + 1;

        ipAddrs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        if (ipAddrs)
            coalesceTable = HeapAlloc(GetProcessHeap(),
             HEAP_ZERO_MEMORY, sizeof(MIB_IPADDRTABLE) +
             (min(numIPAddrs, MAX_LANA + 1) - 1) * sizeof(MIB_IPADDRROW));
        if (ipAddrs && coalesceTable)
        {
            if (GetIpAddrTable(ipAddrs, &size, FALSE) == ERROR_SUCCESS)
            {
                DWORD ndx;

                for (ndx = 0; ndx < ipAddrs->dwNumEntries; ndx++)
                {
                    if ((ipAddrs->table[ndx].dwAddr &
                     ipAddrs->table[ndx].dwMask) !=
                     htonl((INADDR_LOOPBACK & IN_CLASSA_NET)))
                    {
                        BOOL newNetwork = TRUE;
                        DWORD innerIndex;

                        /* make sure we don't have more than one entry
                         * for a subnet */
                        for (innerIndex = 0; newNetwork &&
                         innerIndex < coalesceTable->dwNumEntries; innerIndex++)
                            if ((ipAddrs->table[ndx].dwAddr &
                             ipAddrs->table[ndx].dwMask) ==
                             (coalesceTable->table[innerIndex].dwAddr
                             & coalesceTable->table[innerIndex].dwMask))
                                newNetwork = FALSE;

                        if (newNetwork)
                            memcpy(&coalesceTable->table[
                             coalesceTable->dwNumEntries++],
                             &ipAddrs->table[ndx], sizeof(MIB_IPADDRROW));
                    }
                }

                NetBIOSEnumAdapters(gTransportID, NetBTEnumCallback,
                 coalesceTable);
                ret = NRC_GOODRET;
                for (ndx = 0; ret == NRC_GOODRET &&
                 ndx < coalesceTable->dwNumEntries; ndx++)
                    if (coalesceTable->table[ndx].dwAddr != INADDR_LOOPBACK)
                        ret = NetBTRegisterAdapter(&coalesceTable->table[ndx]);
            }
            else
                ret = NRC_SYSTEM;
            HeapFree(GetProcessHeap(), 0, ipAddrs);
            HeapFree(GetProcessHeap(), 0, coalesceTable);
        }
        else
            ret = NRC_OSRESNOTAV;
    }
    else
        ret = NRC_SYSTEM;
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static const WCHAR VxD_MSTCPW[] = { 'S','Y','S','T','E','M','\\','C','u','r',
 'r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','S','e','r','v',
 'i','c','e','s','\\','V','x','D','\\','M','S','T','C','P','\0' };
static const WCHAR NetBT_ParametersW[] = { 'S','Y','S','T','E','M','\\','C','u',
 'r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\','S','e','r',
 'v','i','c','e','s','\\','N','e','t','B','T','\\','P','a','r','a','m','e','t',
 'e','r','s','\0' };
static const WCHAR EnableDNSW[] = { 'E','n','a','b','l','e','D','N','S','\0' };
static const WCHAR BcastNameQueryCountW[] = { 'B','c','a','s','t','N','a','m',
 'e','Q','u','e','r','y','C','o','u','n','t','\0' };
static const WCHAR BcastNameQueryTimeoutW[] = { 'B','c','a','s','t','N','a','m',
 'e','Q','u','e','r','y','T','i','m','e','o','u','t','\0' };
static const WCHAR NameSrvQueryCountW[] = { 'N','a','m','e','S','r','v',
 'Q','u','e','r','y','C','o','u','n','t','\0' };
static const WCHAR NameSrvQueryTimeoutW[] = { 'N','a','m','e','S','r','v',
 'Q','u','e','r','y','T','i','m','e','o','u','t','\0' };
static const WCHAR ScopeIDW[] = { 'S','c','o','p','e','I','D','\0' };
static const WCHAR CacheTimeoutW[] = { 'C','a','c','h','e','T','i','m','e','o',
 'u','t','\0' };
static const WCHAR Config_NetworkW[] = { 'S','o','f','t','w','a','r','e','\\',
                                         'W','i','n','e','\\','N','e','t','w','o','r','k','\0' };

/* Initializes global variables and registers the NetBT transport */
void NetBTInit(void)
{
    HKEY hKey;
    NetBIOSTransport transport;
    LONG ret;

    TRACE("\n");

    gEnableDNS = TRUE;
    gBCastQueries = BCAST_QUERIES;
    gBCastQueryTimeout = BCAST_QUERY_TIMEOUT;
    gWINSQueries = WINS_QUERIES;
    gWINSQueryTimeout = WINS_QUERY_TIMEOUT;
    gNumWINSServers = 0;
    memset(gWINSServers, 0, sizeof(gWINSServers));
    gScopeID[0] = '\0';
    gCacheTimeout = CACHE_TIMEOUT;

    /* Try to open the Win9x NetBT configuration key */
    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, VxD_MSTCPW, 0, KEY_READ, &hKey);
    /* If that fails, try the WinNT NetBT configuration key */
    if (ret != ERROR_SUCCESS)
        ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, NetBT_ParametersW, 0, KEY_READ,
         &hKey);
    if (ret == ERROR_SUCCESS)
    {
        DWORD dword, size;

        size = sizeof(dword);
        if (RegQueryValueExW(hKey, EnableDNSW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS)
            gEnableDNS = dword;
        size = sizeof(dword);
        if (RegQueryValueExW(hKey, BcastNameQueryCountW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS && dword >= MIN_QUERIES
         && dword <= MAX_QUERIES)
            gBCastQueries = dword;
        size = sizeof(dword);
        if (RegQueryValueExW(hKey, BcastNameQueryTimeoutW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS && dword >= MIN_QUERY_TIMEOUT)
            gBCastQueryTimeout = dword;
        size = sizeof(dword);
        if (RegQueryValueExW(hKey, NameSrvQueryCountW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS && dword >= MIN_QUERIES
         && dword <= MAX_QUERIES)
            gWINSQueries = dword;
        size = sizeof(dword);
        if (RegQueryValueExW(hKey, NameSrvQueryTimeoutW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS && dword >= MIN_QUERY_TIMEOUT)
            gWINSQueryTimeout = dword;
        size = sizeof(gScopeID) - 1;
        if (RegQueryValueExW(hKey, ScopeIDW, NULL, NULL, (LPBYTE)gScopeID + 1, &size)
         == ERROR_SUCCESS)
        {
            /* convert into L2-encoded version, suitable for use by
               NetBTNameEncode */
            char *ptr, *lenPtr;

            for (ptr = gScopeID + 1, lenPtr = gScopeID; ptr - gScopeID < sizeof(gScopeID) && *ptr; ++ptr)
            {
                if (*ptr == '.')
                {
                    lenPtr = ptr;
                    *lenPtr = 0;
                }
                else
                {
                    ++*lenPtr;
                }
            }
        }
        if (RegQueryValueExW(hKey, CacheTimeoutW, NULL, NULL,
         (LPBYTE)&dword, &size) == ERROR_SUCCESS && dword >= MIN_CACHE_TIMEOUT)
            gCacheTimeout = dword;
        RegCloseKey(hKey);
    }
    /* WINE-specific NetBT registry settings.  Because our adapter naming is
     * different than MS', we can't do per-adapter WINS configuration in the
     * same place.  Just do a global WINS configuration instead.
     */
    /* @@ Wine registry key: HKCU\Software\Wine\Network */
    if (RegOpenKeyW(HKEY_CURRENT_USER, Config_NetworkW, &hKey) == ERROR_SUCCESS)
    {
        static const char *nsValueNames[] = { "WinsServer", "BackupWinsServer" };
        char nsString[16];
        DWORD size, ndx;

        for (ndx = 0; ndx < sizeof(nsValueNames) / sizeof(nsValueNames[0]);
         ndx++)
        {
            size = sizeof(nsString) / sizeof(char);
            if (RegQueryValueExA(hKey, nsValueNames[ndx], NULL, NULL,
             (LPBYTE)nsString, &size) == ERROR_SUCCESS)
            {
                unsigned long addr = inet_addr(nsString);

                if (addr != INADDR_NONE && gNumWINSServers < MAX_WINS_SERVERS)
                    gWINSServers[gNumWINSServers++] = addr;
            }
        }
        RegCloseKey(hKey);
    }

    transport.enumerate      = NetBTEnum;
    transport.astat          = NetBTAstat;
    transport.findName       = NetBTFindName;
    transport.call           = NetBTCall;
    transport.send           = NetBTSend;
    transport.recv           = NetBTRecv;
    transport.hangup         = NetBTHangup;
    transport.cleanupAdapter = NetBTCleanupAdapter;
    transport.cleanup        = NetBTCleanup;
    memcpy(&gTransportID, TRANSPORT_NBT, sizeof(ULONG));
    NetBIOSRegisterTransport(gTransportID, &transport);
}
