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
 */
#include "config.h"
#include "wine/debug.h"
#include "nbcmdqueue.h"
#include "netbios.h"

WINE_DEFAULT_DEBUG_CHANNEL(netbios);

/* This file provides a NetBIOS emulator that implements the NetBIOS interface,
 * including thread safety and asynchronous call support.  The protocol
 * implementation is separate, with blocking (synchronous) functions.
 */

#define ADAPTERS_INCR 8
#define DEFAULT_NUM_SESSIONS 16

typedef struct _NetBIOSTransportTableEntry
{
    ULONG            id;
    NetBIOSTransport transport;
} NetBIOSTransportTableEntry;

typedef struct _NetBIOSSession
{
    BOOL  inUse;
    UCHAR state;
    UCHAR local_name[NCBNAMSZ];
    UCHAR remote_name[NCBNAMSZ];
    void *data;
} NetBIOSSession;

/* This struct needs a little explanation, unfortunately.  enabled is only
 * used by nbInternalEnum (see).  If transport_id is not 0 and transport
 * is not NULL, the adapter is considered valid.  (transport is a pointer to
 * an entry in a NetBIOSTransportTableEntry.)  data has data for the callers of
 * NetBIOSEnumAdapters to be able to see.  The lana is repeated there, even
 * though I don't use it internally--it's for transports to use reenabling
 * adapters using NetBIOSEnableAdapter.
 */
typedef struct _NetBIOSAdapter
{
    BOOL               enabled;
    BOOL               shuttingDown;
    LONG               resetting;
    ULONG              transport_id;
    NetBIOSTransport  *transport;
    NetBIOSAdapterImpl impl;
    struct NBCmdQueue *cmdQueue;
    CRITICAL_SECTION   cs;
    DWORD              sessionsLen;
    NetBIOSSession    *sessions;
} NetBIOSAdapter;

typedef struct _NetBIOSAdapterTable {
    CRITICAL_SECTION cs;
    BOOL             enumerated;
    BOOL             enumerating;
    UCHAR            tableSize;
    NetBIOSAdapter  *table;
} NetBIOSAdapterTable;

/* Just enough space for NBT right now */
static NetBIOSTransportTableEntry gTransports[1];
static UCHAR gNumTransports = 0;
static NetBIOSAdapterTable gNBTable;

static UCHAR nbResizeAdapterTable(UCHAR newSize)
{
    UCHAR ret;

    if (gNBTable.table)
        gNBTable.table = HeapReAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, gNBTable.table,
         newSize * sizeof(NetBIOSAdapter));
    else
        gNBTable.table = HeapAlloc(GetProcessHeap(),
         HEAP_ZERO_MEMORY, newSize * sizeof(NetBIOSAdapter));
    if (gNBTable.table)
    {
        gNBTable.tableSize = newSize;
        ret = NRC_GOODRET;
    }
    else
        ret = NRC_OSRESNOTAV;
    return ret;
}

void NetBIOSInit(void)
{
    memset(&gNBTable, 0, sizeof(gNBTable));
    InitializeCriticalSection(&gNBTable.cs);
    gNBTable.cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NetBIOSAdapterTable.cs");
}

void NetBIOSShutdown(void)
{
    UCHAR i;

    EnterCriticalSection(&gNBTable.cs);
    for (i = 0; i < gNBTable.tableSize; i++)
    {
        if (gNBTable.table[i].transport &&
         gNBTable.table[i].transport->cleanupAdapter)
            gNBTable.table[i].transport->cleanupAdapter(
             gNBTable.table[i].impl.data);
    }
    for (i = 0; i < gNumTransports; i++)
        if (gTransports[i].transport.cleanup)
            gTransports[i].transport.cleanup();
    LeaveCriticalSection(&gNBTable.cs);
    gNBTable.cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&gNBTable.cs);
    HeapFree(GetProcessHeap(), 0, gNBTable.table);
}

BOOL NetBIOSRegisterTransport(ULONG id, NetBIOSTransport *transport)
{
    BOOL ret;

    TRACE(": transport 0x%08x, p %p\n", id, transport);
    if (!transport)
        ret = FALSE;
    else if (gNumTransports >= sizeof(gTransports) / sizeof(gTransports[0]))
    {
        FIXME("Too many transports %d\n", gNumTransports + 1);
        ret = FALSE;
    }
    else
    {
        UCHAR i;

        ret = FALSE;
        for (i = 0; !ret && i < gNumTransports; i++)
        {
            if (gTransports[i].id == id)
            {
                WARN("Replacing NetBIOS transport ID %d\n", id);
                memcpy(&gTransports[i].transport, transport,
                 sizeof(NetBIOSTransport));
                ret = TRUE;
            }
        }
        if (!ret)
        {
            gTransports[gNumTransports].id = id;
            memcpy(&gTransports[gNumTransports].transport, transport,
             sizeof(NetBIOSTransport));
            gNumTransports++;
            ret = TRUE;
        }
    }
    TRACE("returning %d\n", ret);
    return ret;
}

/* In this, I acquire the table lock to make sure no one else is modifying it.
 * This is _probably_ overkill since it should only be called during the
 * context of a NetBIOSEnum call, but just to be safe..
 */
BOOL NetBIOSRegisterAdapter(ULONG transport, DWORD ifIndex, void *data)
{
    BOOL ret;
    UCHAR i;

    TRACE(": transport 0x%08x, ifIndex 0x%08x, data %p\n", transport, ifIndex,
     data);
    for (i = 0; i < gNumTransports && gTransports[i].id != transport; i++)
        ;
    if ((i < gNumTransports) && gTransports[i].id == transport)
    {
        NetBIOSTransport *transportPtr = &gTransports[i].transport;

        TRACE(": found transport %p for id 0x%08x\n", transportPtr, transport);

        EnterCriticalSection(&gNBTable.cs);
        ret = FALSE;
        for (i = 0; i < gNBTable.tableSize &&
         gNBTable.table[i].transport != 0; i++)
            ;
        if (i == gNBTable.tableSize && gNBTable.tableSize < MAX_LANA + 1)
        {
            UCHAR newSize;

            if (gNBTable.tableSize < (MAX_LANA + 1) - ADAPTERS_INCR)
                newSize = gNBTable.tableSize + ADAPTERS_INCR;
            else
                newSize = MAX_LANA + 1;
            nbResizeAdapterTable(newSize);
        }
        if (i < gNBTable.tableSize && gNBTable.table[i].transport == 0)
        {
            TRACE(": registering as LANA %d\n", i);
            gNBTable.table[i].transport_id = transport;
            gNBTable.table[i].transport = transportPtr;
            gNBTable.table[i].impl.lana = i;
            gNBTable.table[i].impl.ifIndex = ifIndex;
            gNBTable.table[i].impl.data = data;
            gNBTable.table[i].cmdQueue = NBCmdQueueCreate(GetProcessHeap());
            InitializeCriticalSection(&gNBTable.table[i].cs);
            gNBTable.table[i].cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NetBIOSAdapterTable.NetBIOSAdapter.cs");
            gNBTable.table[i].enabled = TRUE;
            ret = TRUE;
        }
        LeaveCriticalSection(&gNBTable.cs);
    }
    else
        ret = FALSE;
    TRACE("returning %d\n", ret);
    return ret;
}

/* In this, I acquire the table lock to make sure no one else is modifying it.
 * This is _probably_ overkill since it should only be called during the
 * context of a NetBIOSEnum call, but just to be safe..
 */
void NetBIOSEnableAdapter(UCHAR lana)
{
    TRACE(": %d\n", lana);
    if (lana < gNBTable.tableSize)
    {
        EnterCriticalSection(&gNBTable.cs);
        if (gNBTable.table[lana].transport != 0)
            gNBTable.table[lana].enabled = TRUE;
        LeaveCriticalSection(&gNBTable.cs);
    }
}

static void nbShutdownAdapter(NetBIOSAdapter *adapter)
{
    if (adapter)
    {
        adapter->shuttingDown = TRUE;
        NBCmdQueueCancelAll(adapter->cmdQueue);
        if (adapter->transport->cleanupAdapter)
            adapter->transport->cleanupAdapter(adapter->impl.data);
        NBCmdQueueDestroy(adapter->cmdQueue);
        adapter->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&adapter->cs);
        memset(adapter, 0, sizeof(NetBIOSAdapter));
    }
}

static void nbInternalEnum(void)
{
    UCHAR i;

    EnterCriticalSection(&gNBTable.cs);
    TRACE("before mark\n");
    /* mark: */
    for (i = 0; i < gNBTable.tableSize; i++)
        if (gNBTable.table[i].enabled && gNBTable.table[i].transport != 0)
            gNBTable.table[i].enabled = FALSE;

    TRACE("marked, before store, %d transports\n", gNumTransports);
    /* store adapters: */
    for (i = 0; i < gNumTransports; i++)
        if (gTransports[i].transport.enumerate)
            gTransports[i].transport.enumerate();

    TRACE("before sweep\n");
    /* sweep: */
    for (i = 0; i < gNBTable.tableSize; i++)
        if (!gNBTable.table[i].enabled && gNBTable.table[i].transport != 0)
            nbShutdownAdapter(&gNBTable.table[i]);
    gNBTable.enumerated = TRUE;
    LeaveCriticalSection(&gNBTable.cs);
}

UCHAR NetBIOSNumAdapters(void)
{
    UCHAR ret, i;

    if (!gNBTable.enumerated)
        nbInternalEnum();
    for (i = 0, ret = 0; i < gNBTable.tableSize; i++)
        if (gNBTable.table[i].transport != 0)
            ret++;
    return ret;
}

void NetBIOSEnumAdapters(ULONG transport, NetBIOSEnumAdaptersCallback cb,
 void *closure)
{
    TRACE("transport 0x%08x, callback %p, closure %p\n", transport, cb,
     closure);
    if (cb)
    {
        BOOL enumAll = memcmp(&transport, ALL_TRANSPORTS, sizeof(ULONG)) == 0;
        UCHAR i, numLANAs = 0;

        EnterCriticalSection(&gNBTable.cs);
        if (!gNBTable.enumerating)
        {
            gNBTable.enumerating = TRUE;
            nbInternalEnum();
            gNBTable.enumerating = FALSE;
        }
        for (i = 0; i < gNBTable.tableSize; i++)
            if (enumAll || gNBTable.table[i].transport_id == transport)
                numLANAs++;
        if (numLANAs > 0)
        {
            UCHAR lanaIndex = 0;

            for (i = 0; i < gNBTable.tableSize; i++)
                if (gNBTable.table[i].transport_id != 0 &&
                 (enumAll || gNBTable.table[i].transport_id == transport))
                    cb(numLANAs, lanaIndex++, gNBTable.table[i].transport_id,
                     &gNBTable.table[i].impl, closure);
        }
        LeaveCriticalSection(&gNBTable.cs);
    }
}

static NetBIOSAdapter *nbGetAdapter(UCHAR lana)
{
    NetBIOSAdapter *ret = NULL;

    TRACE(": lana %d, num allocated adapters %d\n", lana, gNBTable.tableSize);
    if (lana < gNBTable.tableSize && gNBTable.table[lana].transport_id != 0
     && gNBTable.table[lana].transport)
        ret = &gNBTable.table[lana];
    TRACE("returning %p\n", ret);
    return ret;
}

static UCHAR nbEnum(PNCB ncb)
{
    PLANA_ENUM lanas = (PLANA_ENUM)ncb->ncb_buffer;
    UCHAR i, ret;

    TRACE(": ncb %p\n", ncb);

    if (!lanas)
        ret = NRC_BUFLEN;
    else if (ncb->ncb_length < sizeof(LANA_ENUM))
        ret = NRC_BUFLEN;
    else
    {
        nbInternalEnum();
        lanas->length = 0;
        for (i = 0; i < gNBTable.tableSize; i++)
            if (gNBTable.table[i].transport)
            {
                lanas->length++;
                lanas->lana[i] = i;
            }
        ret = NRC_GOODRET;
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR nbInternalHangup(NetBIOSAdapter *adapter, NetBIOSSession *session);

static UCHAR nbCancel(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret;

    TRACE(": adapter %p, ncb %p\n", adapter, ncb);

    if (!adapter) return NRC_BRIDGE;
    if (!ncb) return NRC_INVADDRESS;

    switch (ncb->ncb_command & 0x7f)
    {
        case NCBCANCEL:
        case NCBADDNAME:
        case NCBADDGRNAME:
        case NCBDELNAME:
        case NCBRESET:
        case NCBSSTAT:
            ret = NRC_CANCEL;
            break;

        /* NCBCALL, NCBCHAINSEND/NCBSEND, NCBHANGUP all close the associated
         * session if cancelled */
        case NCBCALL:
        case NCBSEND:
        case NCBCHAINSEND:
        case NCBSENDNA:
        case NCBCHAINSENDNA:
        case NCBHANGUP:
        {
            if (ncb->ncb_lsn >= adapter->sessionsLen)
                ret = NRC_SNUMOUT;
            else if (!adapter->sessions[ncb->ncb_lsn].inUse)
                ret = NRC_SNUMOUT;
            else
            {
                ret = NBCmdQueueCancel(adapter->cmdQueue, ncb);
                if (ret == NRC_CMDCAN || ret == NRC_CANOCCR)
                    nbInternalHangup(adapter, &adapter->sessions[ncb->ncb_lsn]);
            }
            break;
        }

        default:
            ret = NBCmdQueueCancel(adapter->cmdQueue, ncb);
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

/* Resizes adapter to contain space for at least sessionsLen sessions.
 * If allocating more space for sessions, sets the adapter's sessionsLen to
 * sessionsLen.  If the adapter's sessionsLen was already at least sessionsLen,
 * does nothing.  Does not modify existing sessions.  Assumes the adapter is
 * locked.
 * Returns NRC_GOODRET on success, and something else on failure.
 */
static UCHAR nbResizeAdapter(NetBIOSAdapter *adapter, UCHAR sessionsLen)
{
    UCHAR ret = NRC_GOODRET;

    if (adapter && adapter->sessionsLen < sessionsLen)
    {
        NetBIOSSession *newSessions;

        if (adapter->sessions)
            newSessions = HeapReAlloc(GetProcessHeap(),
             HEAP_ZERO_MEMORY, adapter->sessions, sessionsLen *
             sizeof(NetBIOSSession));
        else
            newSessions = HeapAlloc(GetProcessHeap(),
             HEAP_ZERO_MEMORY, sessionsLen * sizeof(NetBIOSSession));
        if (newSessions)
        {
            adapter->sessions = newSessions;
            adapter->sessionsLen = sessionsLen;
        }
        else
            ret = NRC_OSRESNOTAV;
    }
    return ret;
}

static UCHAR nbReset(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret;

    TRACE(": adapter %p, ncb %p\n", adapter, ncb);

    if (!adapter) return NRC_BRIDGE;
    if (!ncb) return NRC_INVADDRESS;

    if (InterlockedIncrement(&adapter->resetting) == 1)
    {
        UCHAR i, resizeTo;

        NBCmdQueueCancelAll(adapter->cmdQueue);

        EnterCriticalSection(&adapter->cs);
        for (i = 0; i < adapter->sessionsLen; i++)
            if (adapter->sessions[i].inUse)
                nbInternalHangup(adapter, &adapter->sessions[i]);
        if (!ncb->ncb_lsn)
            resizeTo = ncb->ncb_callname[0] == 0 ? DEFAULT_NUM_SESSIONS :
             ncb->ncb_callname[0];
        else if (adapter->sessionsLen == 0)
            resizeTo = DEFAULT_NUM_SESSIONS;
        else
            resizeTo = 0;
        if (resizeTo > 0)
            ret = nbResizeAdapter(adapter, resizeTo);
        else
            ret = NRC_GOODRET;
        LeaveCriticalSection(&adapter->cs);
    }
    else
        ret = NRC_TOOMANY;
    InterlockedDecrement(&adapter->resetting);
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR nbSStat(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret, i, spaceFor;
    PSESSION_HEADER sstat;

    TRACE(": adapter %p, NCB %p\n", adapter, ncb);

    if (!adapter) return NRC_BADDR;
    if (adapter->sessionsLen == 0) return NRC_ENVNOTDEF;
    if (!ncb) return NRC_INVADDRESS;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (ncb->ncb_length < sizeof(SESSION_HEADER)) return NRC_BUFLEN;

    sstat = (PSESSION_HEADER)ncb->ncb_buffer;
    ret = NRC_GOODRET;
    memset(sstat, 0, sizeof(SESSION_HEADER));
    spaceFor = (ncb->ncb_length - sizeof(SESSION_HEADER)) /
     sizeof(SESSION_BUFFER);
    EnterCriticalSection(&adapter->cs);
    for (i = 0; ret == NRC_GOODRET && i < adapter->sessionsLen; i++)
    {
        if (adapter->sessions[i].inUse && (ncb->ncb_name[0] == '*' ||
         !memcmp(ncb->ncb_name, adapter->sessions[i].local_name, NCBNAMSZ)))
        {
            if (sstat->num_sess < spaceFor)
            {
                PSESSION_BUFFER buf;
               
                buf = (PSESSION_BUFFER)((PUCHAR)sstat + sizeof(SESSION_HEADER)
                 + sstat->num_sess * sizeof(SESSION_BUFFER));
                buf->lsn = i;
                buf->state = adapter->sessions[i].state;
                memcpy(buf->local_name, adapter->sessions[i].local_name,
                 NCBNAMSZ);
                memcpy(buf->remote_name, adapter->sessions[i].remote_name,
                 NCBNAMSZ);
                buf->rcvs_outstanding = buf->sends_outstanding = 0;
                sstat->num_sess++;
            }
            else
                ret = NRC_BUFLEN;
        }
    }
    LeaveCriticalSection(&adapter->cs);

    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR nbCall(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret, i;

    TRACE(": adapter %p, NCB %p\n", adapter, ncb);

    if (!adapter) return NRC_BRIDGE;
    if (adapter->sessionsLen == 0) return NRC_ENVNOTDEF;
    if (!adapter->transport->call) return NRC_ILLCMD;
    if (!ncb) return NRC_INVADDRESS;

    EnterCriticalSection(&adapter->cs);
    for (i = 0; i < adapter->sessionsLen && adapter->sessions[i].inUse; i++)
        ;
    if (i < adapter->sessionsLen)
    {
        adapter->sessions[i].inUse = TRUE;
        adapter->sessions[i].state = CALL_PENDING;
        memcpy(adapter->sessions[i].local_name, ncb->ncb_name, NCBNAMSZ);
        memcpy(adapter->sessions[i].remote_name, ncb->ncb_callname, NCBNAMSZ);
        ret = NRC_GOODRET;
    }
    else
        ret = NRC_LOCTFUL;
    LeaveCriticalSection(&adapter->cs);

    if (ret == NRC_GOODRET)
    {
        ret = adapter->transport->call(adapter->impl.data, ncb,
         &adapter->sessions[i].data);
        if (ret == NRC_GOODRET)
        {
            ncb->ncb_lsn = i;
            adapter->sessions[i].state = SESSION_ESTABLISHED;
        }
        else
        {
            adapter->sessions[i].inUse = FALSE;
            adapter->sessions[i].state = 0;
        }
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static UCHAR nbSend(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret;
    NetBIOSSession *session;

    if (!adapter) return NRC_BRIDGE;
    if (!adapter->transport->send) return NRC_ILLCMD;
    if (!ncb) return NRC_INVADDRESS;
    if (ncb->ncb_lsn >= adapter->sessionsLen) return NRC_SNUMOUT;
    if (!adapter->sessions[ncb->ncb_lsn].inUse) return NRC_SNUMOUT;
    if (!ncb->ncb_buffer) return NRC_BADDR;

    session = &adapter->sessions[ncb->ncb_lsn];
    if (session->state != SESSION_ESTABLISHED)
        ret = NRC_SNUMOUT;
    else
        ret = adapter->transport->send(adapter->impl.data, session->data, ncb);
    return ret;
}

static UCHAR nbRecv(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret;
    NetBIOSSession *session;

    if (!adapter) return NRC_BRIDGE;
    if (!adapter->transport->recv) return NRC_ILLCMD;
    if (!ncb) return NRC_INVADDRESS;
    if (ncb->ncb_lsn >= adapter->sessionsLen) return NRC_SNUMOUT;
    if (!adapter->sessions[ncb->ncb_lsn].inUse) return NRC_SNUMOUT;
    if (!ncb->ncb_buffer) return NRC_BADDR;

    session = &adapter->sessions[ncb->ncb_lsn];
    if (session->state != SESSION_ESTABLISHED)
        ret = NRC_SNUMOUT;
    else
        ret = adapter->transport->recv(adapter->impl.data, session->data, ncb);
    return ret;
}

static UCHAR nbInternalHangup(NetBIOSAdapter *adapter, NetBIOSSession *session)
{
    UCHAR ret;

    if (!adapter) return NRC_BRIDGE;
    if (!session) return NRC_SNUMOUT;

    if (adapter->transport->hangup)
        ret = adapter->transport->hangup(adapter->impl.data, session->data);
    else
        ret = NRC_ILLCMD;
    EnterCriticalSection(&adapter->cs);
    memset(session, 0, sizeof(NetBIOSSession));
    LeaveCriticalSection(&adapter->cs);
    return NRC_GOODRET;
}

static UCHAR nbHangup(NetBIOSAdapter *adapter, const NCB *ncb)
{
    UCHAR ret;
    NetBIOSSession *session;

    if (!adapter) return NRC_BRIDGE;
    if (!ncb) return NRC_INVADDRESS;
    if (ncb->ncb_lsn >= adapter->sessionsLen) return NRC_SNUMOUT;
    if (!adapter->sessions[ncb->ncb_lsn].inUse) return NRC_SNUMOUT;

    session = &adapter->sessions[ncb->ncb_lsn];
    if (session->state != SESSION_ESTABLISHED)
        ret = NRC_SNUMOUT;
    else
    {
        session->state = HANGUP_PENDING;
        ret = nbInternalHangup(adapter, session);
    }
    return ret;
}

void NetBIOSHangupSession(const NCB *ncb)
{
    NetBIOSAdapter *adapter;

    if (!ncb) return;

    adapter = nbGetAdapter(ncb->ncb_lana_num);
    if (adapter)
    {
        if (ncb->ncb_lsn < adapter->sessionsLen &&
         adapter->sessions[ncb->ncb_lsn].inUse)
            nbHangup(adapter, ncb);
    }
}

static UCHAR nbAStat(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret;

    if (!adapter) return NRC_BRIDGE;
    if (!adapter->transport->astat) return NRC_ILLCMD;
    if (!ncb) return NRC_INVADDRESS;
    if (!ncb->ncb_buffer) return NRC_BADDR;
    if (ncb->ncb_length < sizeof(ADAPTER_STATUS)) return NRC_BUFLEN;

    ret = adapter->transport->astat(adapter->impl.data, ncb);
    if (ncb->ncb_callname[0] == '*')
    {
        PADAPTER_STATUS astat = (PADAPTER_STATUS)ncb->ncb_buffer;

        astat->max_sess = astat->max_cfg_sess = adapter->sessionsLen;
    }
    return ret;
}

static UCHAR nbDispatch(NetBIOSAdapter *adapter, PNCB ncb)
{
    UCHAR ret, cmd;

    TRACE(": adapter %p, ncb %p\n", adapter, ncb);

    if (!adapter) return NRC_BRIDGE;
    if (!ncb) return NRC_INVADDRESS;

    cmd = ncb->ncb_command & 0x7f;
    if (cmd == NCBRESET)
        ret = nbReset(adapter, ncb);
    else
    {
        ret = NBCmdQueueAdd(adapter->cmdQueue, ncb);
        if (ret == NRC_GOODRET)
        {
            switch (cmd)
            {
                case NCBCALL:
                    ret = nbCall(adapter, ncb);
                    break;

                /* WinNT doesn't chain sends, it always sends immediately.
                 * Doubt there's any real significance to the NA variants.
                 */
                case NCBSEND:
                case NCBSENDNA:
                case NCBCHAINSEND:
                case NCBCHAINSENDNA:
                    ret = nbSend(adapter, ncb);
                    break;

                case NCBRECV:
                    ret = nbRecv(adapter, ncb);
                    break;

                case NCBHANGUP:
                    ret = nbHangup(adapter, ncb);
                    break;

                case NCBASTAT:
                    ret = nbAStat(adapter, ncb);
                    break;

                case NCBFINDNAME:
                    if (adapter->transport->findName)
                        ret = adapter->transport->findName(adapter->impl.data,
                         ncb);
                    else
                        ret = NRC_ILLCMD;
                    break;

                default:
                    FIXME("(%p): command code 0x%02x\n", ncb, ncb->ncb_command);
                    ret = NRC_ILLCMD;
            }
            NBCmdQueueComplete(adapter->cmdQueue, ncb, ret);
        }
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}

static DWORD WINAPI nbCmdThread(LPVOID lpVoid)
{
    PNCB ncb = (PNCB)lpVoid;

    if (ncb)
    {
        UCHAR ret;
        NetBIOSAdapter *adapter = nbGetAdapter(ncb->ncb_lana_num);

        if (adapter)
            ret = nbDispatch(adapter, ncb);
        else
            ret = NRC_BRIDGE;
        ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret;
        if (ncb->ncb_post)
            ncb->ncb_post(ncb);
        else if (ncb->ncb_event)
            SetEvent(ncb->ncb_event);
    }
    return 0;
}

UCHAR WINAPI Netbios(PNCB ncb)
{
    UCHAR ret, cmd;

    TRACE("ncb = %p\n", ncb);

    if (!ncb) return NRC_INVADDRESS;

    TRACE("ncb_command 0x%02x, ncb_lana_num %d, ncb_buffer %p, ncb_length %d\n",
     ncb->ncb_command, ncb->ncb_lana_num, ncb->ncb_buffer, ncb->ncb_length);
    cmd = ncb->ncb_command & 0x7f;

    if (cmd == NCBENUM)
        ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret = nbEnum(ncb);
    else if (cmd == NCBADDNAME)
    {
        FIXME("NCBADDNAME: stub, returning success\n");
        ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret = NRC_GOODRET;
    }
    else
    {
        NetBIOSAdapter *adapter;

        /* Apps not specifically written for WinNT won't do an NCBENUM first,
         * so make sure the table has been enumerated at least once
         */
        if (!gNBTable.enumerated)
            nbInternalEnum();
        adapter = nbGetAdapter(ncb->ncb_lana_num);
        if (!adapter)
            ret = NRC_BRIDGE;
        else
        {
            if (adapter->shuttingDown)
                ret = NRC_IFBUSY;
            else if (adapter->resetting)
                ret = NRC_TOOMANY;
            else
            {
                /* non-asynch commands first */
                if (cmd == NCBCANCEL)
                    ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret =
                     nbCancel(adapter, ncb);
                else if (cmd == NCBSSTAT)
                    ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret =
                     nbSStat(adapter, ncb);
                else
                {
                    if (ncb->ncb_command & ASYNCH)
                    {
                        HANDLE thread = CreateThread(NULL, 0, nbCmdThread, ncb,
                         CREATE_SUSPENDED, NULL);

                        if (thread != NULL)
                        {
                            ncb->ncb_retcode = ncb->ncb_cmd_cplt = NRC_PENDING;
                            if (ncb->ncb_event)
                                ResetEvent(ncb->ncb_event);
                            ResumeThread(thread);
                            CloseHandle(thread);
                            ret = NRC_GOODRET;
                        }
                        else
                        ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret =
                         NRC_OSRESNOTAV;
                    }
                    else
                        ncb->ncb_retcode = ncb->ncb_cmd_cplt = ret =
                         nbDispatch(adapter, ncb);
                }
            }
        }
    }
    TRACE("returning 0x%02x\n", ret);
    return ret;
}
