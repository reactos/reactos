/* DirectPlay Conformance Tests
 *
 * Copyright 2007 - Alessandro Pignotti
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

#define COBJMACROS
#include "wine/test.h"
#include <stdio.h>
#define INITGUID
#include <dplay.h>
#include <dplobby.h>


#define check(expected, result)                 \
    ok( (expected) == (result),                 \
        "expected=%d got=%d\n",                 \
        (int)(expected), (int)(result) );
#define checkLP(expected, result)               \
    ok( (expected) == (result),                 \
        "expected=%p got=%p\n",                 \
        expected, result );
#define checkHR(expected, result)                       \
    ok( (expected) == (result),                         \
        "expected=%s got=%s\n",                         \
        dpResult2str(expected), dpResult2str(result) );
#define checkStr(expected, result)                              \
    ok( (result != NULL) && (!strcmp(expected, result)),        \
        "expected=%s got=%s\n",                                 \
        expected, result );
#define checkFlags(expected, result, flags)     \
    ok( (expected) == (result),                 \
        "expected=0x%08x(%s) got=0x%08x(%s)\n", \
        expected, dwFlags2str(expected, flags), \
        result, dwFlags2str(result, flags) );
#define checkGuid(expected, result)             \
    ok( IsEqualGUID(expected, result),          \
        "expected=%s got=%s\n",                 \
        Guid2str(expected), Guid2str(result) );
#define checkConv(expected, result, function)   \
    ok( (expected) == (result),                 \
        "expected=0x%08x(%s) got=0x%08x(%s)\n", \
        expected, function(expected),           \
        result, function(result) );


DEFINE_GUID(appGuid, 0xbdcfe03e, 0xf0ec, 0x415b, 0x82, 0x11, 0x6f, 0x86, 0xd8, 0x19, 0x7f, 0xe1);
DEFINE_GUID(appGuid2, 0x93417d3f, 0x7d26, 0x46ba, 0xb5, 0x76, 0xfe, 0x4b, 0x20, 0xbb, 0xad, 0x70);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);


typedef struct tagCallbackData
{
    IDirectPlay4 *pDP;
    UINT dwCounter1, dwCounter2;
    DWORD dwFlags;
    char szTrace1[1024], szTrace2[1024];
    DPID *dpid;
    UINT dpidSize;
} CallbackData, *lpCallbackData;

struct provider_data
{
    int call_count;
    GUID *guid_ptr[10];
    GUID guid_data[10];
    BOOL ret_value;
};

static LPSTR get_temp_buffer(void)
{
    static UINT index = 0;
    static char buff[10][256];

    index = (index + 1) % 10;
    *buff[index] = 0;

    return buff[index];
}


static LPCSTR Guid2str(const GUID *guid)
{
    if (!guid) return "(null)";

    /* Service providers */
    if (IsEqualGUID(guid, &DPSPGUID_IPX))
        return "DPSPGUID_IPX";
    if (IsEqualGUID(guid, &DPSPGUID_TCPIP))
        return "DPSPGUID_TCPIP";
    if (IsEqualGUID(guid, &DPSPGUID_SERIAL))
        return "DPSPGUID_SERIAL";
    if (IsEqualGUID(guid, &DPSPGUID_MODEM))
        return "DPSPGUID_MODEM";
    /* DirectPlay Address IDs */
    if (IsEqualGUID(guid, &DPAID_TotalSize))
        return "DPAID_TotalSize";
    if (IsEqualGUID(guid, &DPAID_ServiceProvider))
        return "DPAID_ServiceProvider";
    if (IsEqualGUID(guid, &DPAID_LobbyProvider))
        return "DPAID_LobbyProvider";
    if (IsEqualGUID(guid, &DPAID_Phone))
        return "DPAID_Phone";
    if (IsEqualGUID(guid, &DPAID_PhoneW))
        return "DPAID_PhoneW";
    if (IsEqualGUID(guid, &DPAID_Modem))
        return "DPAID_Modem";
    if (IsEqualGUID(guid, &DPAID_ModemW))
        return "DPAID_ModemW";
    if (IsEqualGUID(guid, &DPAID_INet))
        return "DPAID_INet";
    if (IsEqualGUID(guid, &DPAID_INetW))
        return "DPAID_INetW";
    if (IsEqualGUID(guid, &DPAID_INetPort))
        return "DPAID_INetPort";
    if (IsEqualGUID(guid, &DPAID_ComPort))
        return "DPAID_ComPort";

    return wine_dbgstr_guid(guid);
}


static LPCSTR dpResult2str(HRESULT hr)
{
    switch (hr)
    {
    case DP_OK:                          return "DP_OK";
    case DPERR_ALREADYINITIALIZED:       return "DPERR_ALREADYINITIALIZED";
    case DPERR_ACCESSDENIED:             return "DPERR_ACCESSDENIED";
    case DPERR_ACTIVEPLAYERS:            return "DPERR_ACTIVEPLAYERS";
    case DPERR_BUFFERTOOSMALL:           return "DPERR_BUFFERTOOSMALL";
    case DPERR_CANTADDPLAYER:            return "DPERR_CANTADDPLAYER";
    case DPERR_CANTCREATEGROUP:          return "DPERR_CANTCREATEGROUP";
    case DPERR_CANTCREATEPLAYER:         return "DPERR_CANTCREATEPLAYER";
    case DPERR_CANTCREATESESSION:        return "DPERR_CANTCREATESESSION";
    case DPERR_CAPSNOTAVAILABLEYET:      return "DPERR_CAPSNOTAVAILABLEYET";
    case DPERR_EXCEPTION:                return "DPERR_EXCEPTION";
    case DPERR_GENERIC:                  return "DPERR_GENERIC";
    case DPERR_INVALIDFLAGS:             return "DPERR_INVALIDFLAGS";
    case DPERR_INVALIDOBJECT:            return "DPERR_INVALIDOBJECT";
    case DPERR_INVALIDPARAMS:            return "DPERR_INVALIDPARAMS";
        /*           symbol with the same value: DPERR_INVALIDPARAM */
    case DPERR_INVALIDPLAYER:            return "DPERR_INVALIDPLAYER";
    case DPERR_INVALIDGROUP:             return "DPERR_INVALIDGROUP";
    case DPERR_NOCAPS:                   return "DPERR_NOCAPS";
    case DPERR_NOCONNECTION:             return "DPERR_NOCONNECTION";
    case DPERR_NOMEMORY:                 return "DPERR_NOMEMORY";
        /*           symbol with the same value: DPERR_OUTOFMEMORY */
    case DPERR_NOMESSAGES:               return "DPERR_NOMESSAGES";
    case DPERR_NONAMESERVERFOUND:        return "DPERR_NONAMESERVERFOUND";
    case DPERR_NOPLAYERS:                return "DPERR_NOPLAYERS";
    case DPERR_NOSESSIONS:               return "DPERR_NOSESSIONS";
    case DPERR_PENDING:                  return "DPERR_PENDING";
    case DPERR_SENDTOOBIG:               return "DPERR_SENDTOOBIG";
    case DPERR_TIMEOUT:                  return "DPERR_TIMEOUT";
    case DPERR_UNAVAILABLE:              return "DPERR_UNAVAILABLE";
    case DPERR_UNSUPPORTED:              return "DPERR_UNSUPPORTED";
    case DPERR_BUSY:                     return "DPERR_BUSY";
    case DPERR_USERCANCEL:               return "DPERR_USERCANCEL";
    case DPERR_NOINTERFACE:              return "DPERR_NOINTERFACE";
    case DPERR_CANNOTCREATESERVER:       return "DPERR_CANNOTCREATESERVER";
    case DPERR_PLAYERLOST:               return "DPERR_PLAYERLOST";
    case DPERR_SESSIONLOST:              return "DPERR_SESSIONLOST";
    case DPERR_UNINITIALIZED:            return "DPERR_UNINITIALIZED";
    case DPERR_NONEWPLAYERS:             return "DPERR_NONEWPLAYERS";
    case DPERR_INVALIDPASSWORD:          return "DPERR_INVALIDPASSWORD";
    case DPERR_CONNECTING:               return "DPERR_CONNECTING";
    case DPERR_CONNECTIONLOST:           return "DPERR_CONNECTIONLOST";
    case DPERR_UNKNOWNMESSAGE:           return "DPERR_UNKNOWNMESSAGE";
    case DPERR_CANCELFAILED:             return "DPERR_CANCELFAILED";
    case DPERR_INVALIDPRIORITY:          return "DPERR_INVALIDPRIORITY";
    case DPERR_NOTHANDLED:               return "DPERR_NOTHANDLED";
    case DPERR_CANCELLED:                return "DPERR_CANCELLED";
    case DPERR_ABORTED:                  return "DPERR_ABORTED";
    case DPERR_BUFFERTOOLARGE:           return "DPERR_BUFFERTOOLARGE";
    case DPERR_CANTCREATEPROCESS:        return "DPERR_CANTCREATEPROCESS";
    case DPERR_APPNOTSTARTED:            return "DPERR_APPNOTSTARTED";
    case DPERR_INVALIDINTERFACE:         return "DPERR_INVALIDINTERFACE";
    case DPERR_NOSERVICEPROVIDER:        return "DPERR_NOSERVICEPROVIDER";
    case DPERR_UNKNOWNAPPLICATION:       return "DPERR_UNKNOWNAPPLICATION";
    case DPERR_NOTLOBBIED:               return "DPERR_NOTLOBBIED";
    case DPERR_SERVICEPROVIDERLOADED:    return "DPERR_SERVICEPROVIDERLOADED";
    case DPERR_ALREADYREGISTERED:        return "DPERR_ALREADYREGISTERED";
    case DPERR_NOTREGISTERED:            return "DPERR_NOTREGISTERED";
    case DPERR_AUTHENTICATIONFAILED:     return "DPERR_AUTHENTICATIONFAILED";
    case DPERR_CANTLOADSSPI:             return "DPERR_CANTLOADSSPI";
    case DPERR_ENCRYPTIONFAILED:         return "DPERR_ENCRYPTIONFAILED";
    case DPERR_SIGNFAILED:               return "DPERR_SIGNFAILED";
    case DPERR_CANTLOADSECURITYPACKAGE:  return "DPERR_CANTLOADSECURITYPACKAGE";
    case DPERR_ENCRYPTIONNOTSUPPORTED:   return "DPERR_ENCRYPTIONNOTSUPPORTED";
    case DPERR_CANTLOADCAPI:             return "DPERR_CANTLOADCAPI";
    case DPERR_NOTLOGGEDIN:              return "DPERR_NOTLOGGEDIN";
    case DPERR_LOGONDENIED:              return "DPERR_LOGONDENIED";
    case CLASS_E_NOAGGREGATION:          return "CLASS_E_NOAGGREGATION";

    default:
    {
        LPSTR buffer = get_temp_buffer();
        sprintf( buffer, "%d", HRESULT_CODE(hr) );
        return buffer;
    }
    }
}

static LPCSTR dpMsgType2str(DWORD dwType)
{
    switch(dwType)
    {
    case DPSYS_CREATEPLAYERORGROUP:      return "DPSYS_CREATEPLAYERORGROUP";
    case DPSYS_DESTROYPLAYERORGROUP:     return "DPSYS_DESTROYPLAYERORGROUP";
    case DPSYS_ADDPLAYERTOGROUP:         return "DPSYS_ADDPLAYERTOGROUP";
    case DPSYS_DELETEPLAYERFROMGROUP:    return "DPSYS_DELETEPLAYERFROMGROUP";
    case DPSYS_SESSIONLOST:              return "DPSYS_SESSIONLOST";
    case DPSYS_HOST:                     return "DPSYS_HOST";
    case DPSYS_SETPLAYERORGROUPDATA:     return "DPSYS_SETPLAYERORGROUPDATA";
    case DPSYS_SETPLAYERORGROUPNAME:     return "DPSYS_SETPLAYERORGROUPNAME";
    case DPSYS_SETSESSIONDESC:           return "DPSYS_SETSESSIONDESC";
    case DPSYS_ADDGROUPTOGROUP:          return "DPSYS_ADDGROUPTOGROUP";
    case DPSYS_DELETEGROUPFROMGROUP:     return "DPSYS_DELETEGROUPFROMGROUP";
    case DPSYS_SECUREMESSAGE:            return "DPSYS_SECUREMESSAGE";
    case DPSYS_STARTSESSION:             return "DPSYS_STARTSESSION";
    case DPSYS_CHAT:                     return "DPSYS_DPSYS_CHAT";
    case DPSYS_SETGROUPOWNER:            return "DPSYS_SETGROUPOWNER";
    case DPSYS_SENDCOMPLETE:             return "DPSYS_SENDCOMPLETE";

    default:                             return "UNKNOWN";
    }
}

static LPCSTR dwFlags2str(DWORD dwFlags, DWORD flagType)
{

#define FLAGS_DPCONNECTION     (1<<0)
#define FLAGS_DPENUMPLAYERS    (1<<1)
#define FLAGS_DPENUMGROUPS     (1<<2)
#define FLAGS_DPPLAYER         (1<<3)
#define FLAGS_DPGROUP          (1<<4)
#define FLAGS_DPENUMSESSIONS   (1<<5)
#define FLAGS_DPGETCAPS        (1<<6)
#define FLAGS_DPGET            (1<<7)
#define FLAGS_DPRECEIVE        (1<<8)
#define FLAGS_DPSEND           (1<<9)
#define FLAGS_DPSET            (1<<10)
#define FLAGS_DPMESSAGEQUEUE   (1<<11)
#define FLAGS_DPCONNECT        (1<<12)
#define FLAGS_DPOPEN           (1<<13)
#define FLAGS_DPSESSION        (1<<14)
#define FLAGS_DPLCONNECTION    (1<<15)
#define FLAGS_DPESC            (1<<16)
#define FLAGS_DPCAPS           (1<<17)

    LPSTR flags = get_temp_buffer();

    /* EnumConnections */

    if (flagType & FLAGS_DPCONNECTION)
    {
        if (dwFlags & DPCONNECTION_DIRECTPLAY)
            strcat(flags, "DPCONNECTION_DIRECTPLAY,");
        if (dwFlags & DPCONNECTION_DIRECTPLAYLOBBY)
            strcat(flags, "DPCONNECTION_DIRECTPLAYLOBBY,");
    }

    /* EnumPlayers,
       EnumGroups */

    if (flagType & FLAGS_DPENUMPLAYERS)
    {
        if (dwFlags == DPENUMPLAYERS_ALL)
            strcat(flags, "DPENUMPLAYERS_ALL,");
        if (dwFlags & DPENUMPLAYERS_LOCAL)
            strcat(flags, "DPENUMPLAYERS_LOCAL,");
        if (dwFlags & DPENUMPLAYERS_REMOTE)
            strcat(flags, "DPENUMPLAYERS_REMOTE,");
        if (dwFlags & DPENUMPLAYERS_GROUP)
            strcat(flags, "DPENUMPLAYERS_GROUP,");
        if (dwFlags & DPENUMPLAYERS_SESSION)
            strcat(flags, "DPENUMPLAYERS_SESSION,");
        if (dwFlags & DPENUMPLAYERS_SERVERPLAYER)
            strcat(flags, "DPENUMPLAYERS_SERVERPLAYER,");
        if (dwFlags & DPENUMPLAYERS_SPECTATOR)
            strcat(flags, "DPENUMPLAYERS_SPECTATOR,");
        if (dwFlags & DPENUMPLAYERS_OWNER)
            strcat(flags, "DPENUMPLAYERS_OWNER,");
    }
    if (flagType & FLAGS_DPENUMGROUPS)
    {
        if (dwFlags == DPENUMGROUPS_ALL)
            strcat(flags, "DPENUMGROUPS_ALL,");
        if (dwFlags & DPENUMPLAYERS_LOCAL)
            strcat(flags, "DPENUMGROUPS_LOCAL,");
        if (dwFlags & DPENUMPLAYERS_REMOTE)
            strcat(flags, "DPENUMGROUPS_REMOTE,");
        if (dwFlags & DPENUMPLAYERS_GROUP)
            strcat(flags, "DPENUMGROUPS_GROUP,");
        if (dwFlags & DPENUMPLAYERS_SESSION)
            strcat(flags, "DPENUMGROUPS_SESSION,");
        if (dwFlags & DPENUMGROUPS_SHORTCUT)
            strcat(flags, "DPENUMGROUPS_SHORTCUT,");
        if (dwFlags & DPENUMGROUPS_STAGINGAREA)
            strcat(flags, "DPENUMGROUPS_STAGINGAREA,");
        if (dwFlags & DPENUMGROUPS_HIDDEN)
            strcat(flags, "DPENUMGROUPS_HIDDEN,");
    }

    /* CreatePlayer */

    if (flagType & FLAGS_DPPLAYER)
    {
        if (dwFlags & DPPLAYER_SERVERPLAYER)
            strcat(flags, "DPPLAYER_SERVERPLAYER,");
        if (dwFlags & DPPLAYER_SPECTATOR)
            strcat(flags, "DPPLAYER_SPECTATOR,");
        if (dwFlags & DPPLAYER_LOCAL)
            strcat(flags, "DPPLAYER_LOCAL,");
        if (dwFlags & DPPLAYER_OWNER)
            strcat(flags, "DPPLAYER_OWNER,");
    }

    /* CreateGroup */

    if (flagType & FLAGS_DPGROUP)
    {
        if (dwFlags & DPGROUP_STAGINGAREA)
            strcat(flags, "DPGROUP_STAGINGAREA,");
        if (dwFlags & DPGROUP_LOCAL)
            strcat(flags, "DPGROUP_LOCAL,");
        if (dwFlags & DPGROUP_HIDDEN)
            strcat(flags, "DPGROUP_HIDDEN,");
    }

    /* EnumSessions */

    if (flagType & FLAGS_DPENUMSESSIONS)
    {
        if (dwFlags & DPENUMSESSIONS_AVAILABLE)
            strcat(flags, "DPENUMSESSIONS_AVAILABLE,");
        if (dwFlags &  DPENUMSESSIONS_ALL)
            strcat(flags, "DPENUMSESSIONS_ALL,");
        if (dwFlags & DPENUMSESSIONS_ASYNC)
            strcat(flags, "DPENUMSESSIONS_ASYNC,");
        if (dwFlags & DPENUMSESSIONS_STOPASYNC)
            strcat(flags, "DPENUMSESSIONS_STOPASYNC,");
        if (dwFlags & DPENUMSESSIONS_PASSWORDREQUIRED)
            strcat(flags, "DPENUMSESSIONS_PASSWORDREQUIRED,");
        if (dwFlags & DPENUMSESSIONS_RETURNSTATUS)
            strcat(flags, "DPENUMSESSIONS_RETURNSTATUS,");
    }

    /* GetCaps,
       GetPlayerCaps */

    if (flagType & FLAGS_DPGETCAPS)
    {
        if (dwFlags & DPGETCAPS_GUARANTEED)
            strcat(flags, "DPGETCAPS_GUARANTEED,");
    }

    /* GetGroupData,
       GetPlayerData */

    if (flagType & FLAGS_DPGET)
    {
        if (dwFlags == DPGET_REMOTE)
            strcat(flags, "DPGET_REMOTE,");
        if (dwFlags & DPGET_LOCAL)
            strcat(flags, "DPGET_LOCAL,");
    }

    /* Receive */

    if (flagType & FLAGS_DPRECEIVE)
    {
        if (dwFlags & DPRECEIVE_ALL)
            strcat(flags, "DPRECEIVE_ALL,");
        if (dwFlags & DPRECEIVE_TOPLAYER)
            strcat(flags, "DPRECEIVE_TOPLAYER,");
        if (dwFlags & DPRECEIVE_FROMPLAYER)
            strcat(flags, "DPRECEIVE_FROMPLAYER,");
        if (dwFlags & DPRECEIVE_PEEK)
            strcat(flags, "DPRECEIVE_PEEK,");
    }

    /* Send */

    if (flagType & FLAGS_DPSEND)
    {
        /*if (dwFlags == DPSEND_NONGUARANTEED)
          strcat(flags, "DPSEND_NONGUARANTEED,");*/
        if (dwFlags == DPSEND_MAX_PRIORITY) /* = DPSEND_MAX_PRI */
        {
            strcat(flags, "DPSEND_MAX_PRIORITY,");
        }
        else
        {
            if (dwFlags & DPSEND_GUARANTEED)
                strcat(flags, "DPSEND_GUARANTEED,");
            if (dwFlags & DPSEND_HIGHPRIORITY)
                strcat(flags, "DPSEND_HIGHPRIORITY,");
            if (dwFlags & DPSEND_OPENSTREAM)
                strcat(flags, "DPSEND_OPENSTREAM,");
            if (dwFlags & DPSEND_CLOSESTREAM)
                strcat(flags, "DPSEND_CLOSESTREAM,");
            if (dwFlags & DPSEND_SIGNED)
                strcat(flags, "DPSEND_SIGNED,");
            if (dwFlags & DPSEND_ENCRYPTED)
                strcat(flags, "DPSEND_ENCRYPTED,");
            if (dwFlags & DPSEND_LOBBYSYSTEMMESSAGE)
                strcat(flags, "DPSEND_LOBBYSYSTEMMESSAGE,");
            if (dwFlags & DPSEND_ASYNC)
                strcat(flags, "DPSEND_ASYNC,");
            if (dwFlags & DPSEND_NOSENDCOMPLETEMSG)
                strcat(flags, "DPSEND_NOSENDCOMPLETEMSG,");
        }
    }

    /* SetGroupData,
       SetGroupName,
       SetPlayerData,
       SetPlayerName,
       SetSessionDesc */

    if (flagType & FLAGS_DPSET)
    {
        if (dwFlags == DPSET_REMOTE)
            strcat(flags, "DPSET_REMOTE,");
        if (dwFlags & DPSET_LOCAL)
            strcat(flags, "DPSET_LOCAL,");
        if (dwFlags & DPSET_GUARANTEED)
            strcat(flags, "DPSET_GUARANTEED,");
    }

    /* GetMessageQueue */

    if (flagType & FLAGS_DPMESSAGEQUEUE)
    {
        if (dwFlags & DPMESSAGEQUEUE_SEND)
            strcat(flags, "DPMESSAGEQUEUE_SEND,");
        if (dwFlags & DPMESSAGEQUEUE_RECEIVE)
            strcat(flags, "DPMESSAGEQUEUE_RECEIVE,");
    }

    /* Connect */

    if (flagType & FLAGS_DPCONNECT)
    {
        if (dwFlags & DPCONNECT_RETURNSTATUS)
            strcat(flags, "DPCONNECT_RETURNSTATUS,");
    }

    /* Open */

    if (flagType & FLAGS_DPOPEN)
    {
        if (dwFlags & DPOPEN_JOIN)
            strcat(flags, "DPOPEN_JOIN,");
        if (dwFlags & DPOPEN_CREATE)
            strcat(flags, "DPOPEN_CREATE,");
        if (dwFlags & DPOPEN_RETURNSTATUS)
            strcat(flags, "DPOPEN_RETURNSTATUS,");
    }

    /* DPSESSIONDESC2 */

    if (flagType & FLAGS_DPSESSION)
    {
        if (dwFlags & DPSESSION_NEWPLAYERSDISABLED)
            strcat(flags, "DPSESSION_NEWPLAYERSDISABLED,");
        if (dwFlags & DPSESSION_MIGRATEHOST)
            strcat(flags, "DPSESSION_MIGRATEHOST,");
        if (dwFlags & DPSESSION_NOMESSAGEID)
            strcat(flags, "DPSESSION_NOMESSAGEID,");
        if (dwFlags & DPSESSION_JOINDISABLED)
            strcat(flags, "DPSESSION_JOINDISABLED,");
        if (dwFlags & DPSESSION_KEEPALIVE)
            strcat(flags, "DPSESSION_KEEPALIVE,");
        if (dwFlags & DPSESSION_NODATAMESSAGES)
            strcat(flags, "DPSESSION_NODATAMESSAGES,");
        if (dwFlags & DPSESSION_SECURESERVER)
            strcat(flags, "DPSESSION_SECURESERVER,");
        if (dwFlags & DPSESSION_PRIVATE)
            strcat(flags, "DPSESSION_PRIVATE,");
        if (dwFlags & DPSESSION_PASSWORDREQUIRED)
            strcat(flags, "DPSESSION_PASSWORDREQUIRED,");
        if (dwFlags & DPSESSION_MULTICASTSERVER)
            strcat(flags, "DPSESSION_MULTICASTSERVER,");
        if (dwFlags & DPSESSION_CLIENTSERVER)
            strcat(flags, "DPSESSION_CLIENTSERVER,");

        if (dwFlags & DPSESSION_DIRECTPLAYPROTOCOL)
            strcat(flags, "DPSESSION_DIRECTPLAYPROTOCOL,");
        if (dwFlags & DPSESSION_NOPRESERVEORDER)
            strcat(flags, "DPSESSION_NOPRESERVEORDER,");
        if (dwFlags & DPSESSION_OPTIMIZELATENCY)
            strcat(flags, "DPSESSION_OPTIMIZELATENCY,");

    }

    /* DPLCONNECTION */

    if (flagType & FLAGS_DPLCONNECTION)
    {
        if (dwFlags & DPLCONNECTION_CREATESESSION)
            strcat(flags, "DPLCONNECTION_CREATESESSION,");
        if (dwFlags & DPLCONNECTION_JOINSESSION)
            strcat(flags, "DPLCONNECTION_JOINSESSION,");
    }

    /* EnumSessionsCallback2 */

    if (flagType & FLAGS_DPESC)
    {
        if (dwFlags & DPESC_TIMEDOUT)
            strcat(flags, "DPESC_TIMEDOUT,");
    }

    /* GetCaps,
       GetPlayerCaps */

    if (flagType & FLAGS_DPCAPS)
    {
        if (dwFlags & DPCAPS_ISHOST)
            strcat(flags, "DPCAPS_ISHOST,");
        if (dwFlags & DPCAPS_GROUPOPTIMIZED)
            strcat(flags, "DPCAPS_GROUPOPTIMIZED,");
        if (dwFlags & DPCAPS_KEEPALIVEOPTIMIZED)
            strcat(flags, "DPCAPS_KEEPALIVEOPTIMIZED,");
        if (dwFlags & DPCAPS_GUARANTEEDOPTIMIZED)
            strcat(flags, "DPCAPS_GUARANTEEDOPTIMIZED,");
        if (dwFlags & DPCAPS_GUARANTEEDSUPPORTED)
            strcat(flags, "DPCAPS_GUARANTEEDSUPPORTED,");
        if (dwFlags & DPCAPS_SIGNINGSUPPORTED)
            strcat(flags, "DPCAPS_SIGNINGSUPPORTED,");
        if (dwFlags & DPCAPS_ENCRYPTIONSUPPORTED)
            strcat(flags, "DPCAPS_ENCRYPTIONSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCCANCELSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCCANCELSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCCANCELALLSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCCANCELALLSUPPORTED,");
        if (dwFlags & DPCAPS_SENDTIMEOUTSUPPORTED)
            strcat(flags, "DPCAPS_SENDTIMEOUTSUPPORTED,");
        if (dwFlags & DPCAPS_SENDPRIORITYSUPPORTED)
            strcat(flags, "DPCAPS_SENDPRIORITYSUPPORTED,");
        if (dwFlags & DPCAPS_ASYNCSUPPORTED)
            strcat(flags, "DPCAPS_ASYNCSUPPORTED,");

        if (dwFlags & DPPLAYERCAPS_LOCAL)
            strcat(flags, "DPPLAYERCAPS_LOCAL,");
    }

    if ((strlen(flags) == 0) && (dwFlags != 0))
        strcpy(flags, "UNKNOWN");
    else
        flags[strlen(flags)-1] = '\0';

    return flags;
}

static char dpid2char(DPID* dpid, DWORD dpidSize, DPID idPlayer)
{
    UINT i;
    if ( idPlayer == DPID_SYSMSG )
        return 'S';
    for (i=0; i<dpidSize; i++)
    {
        if ( idPlayer == dpid[i] )
            return (char)(i+48);
    }
    return '?';
}

static void check_messages( IDirectPlay4 *pDP, DPID *dpid, DWORD dpidSize,
        lpCallbackData callbackData )
{
    /* Retrieves all messages from the queue of pDP, performing tests
     * to check if we are receiving what we expect.
     *
     * Information about the messages is stores in callbackData:
     *
     * callbackData->dwCounter1: Number of messages received.
     * callbackData->szTrace1: Traces for sender and receiver.
     *     We store the position a dpid holds in the dpid array.
     *     Example:
     *
     *       trace string: "01,02,03,14"
     *           expanded: [ '01', '02', '03', '14' ]
     *                         \     \     \     \
     *                          \     \     \     ) message 3: from 1 to 4
     *                           \     \     ) message 2: from 0 to 3
     *                            \     ) message 1: from 0 to 2
     *                             ) message 0: from 0 to 1
     *
     *     In general terms:
     *       sender of message i   = character in place 3*i of the array
     *       receiver of message i = character in place 3*i+1 of the array
     *
     *     A sender value of 'S' means DPID_SYSMSG, this is, a system message.
     *
     * callbackData->szTrace2: Traces for message sizes.
     */

    DPID idFrom, idTo;
    UINT i;
    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    HRESULT hr;
    char temp[5];

    callbackData->szTrace2[0] = '\0';

    i = 0;
    while ( DP_OK == (hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                                                 lpData, &dwDataSize )) )
    {

        callbackData->szTrace1[ 3*i   ] = dpid2char( dpid, dpidSize, idFrom );
        callbackData->szTrace1[ 3*i+1 ] = dpid2char( dpid, dpidSize, idTo );
        callbackData->szTrace1[ 3*i+2 ] = ',';

        sprintf( temp, "%d,", dwDataSize );
        strcat( callbackData->szTrace2, temp );

        dwDataSize = 1024;
        ++i;
    }

    checkHR( DPERR_NOMESSAGES, hr );

    callbackData->szTrace1[ 3*i ] = '\0';
    callbackData->dwCounter1 = i;


    HeapFree( GetProcessHeap(), 0, lpData );
}

static void init_TCPIP_provider( IDirectPlay4 *pDP, LPCSTR strIPAddressString, WORD port )
{

    DPCOMPOUNDADDRESSELEMENT addressElements[3];
    LPVOID pAddress = NULL;
    DWORD dwAddressSize = 0;
    IDirectPlayLobby3 *pDPL;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                           &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );
    ok (SUCCEEDED (hr), "CCI of CLSID_DirectPlayLobby / IID_IDirectPlayLobby3A failed\n");
    if (FAILED (hr)) return;

    /* Service provider */
    addressElements[0].guidDataType = DPAID_ServiceProvider;
    addressElements[0].dwDataSize   = sizeof(GUID);
    addressElements[0].lpData       = (LPVOID) &DPSPGUID_TCPIP;

    /* IP address string */
    addressElements[1].guidDataType = DPAID_INet;
    addressElements[1].dwDataSize   = lstrlenA(strIPAddressString) + 1;
    addressElements[1].lpData       = (LPVOID) strIPAddressString;

    /* Optional Port number */
    if( port > 0 )
    {
        addressElements[2].guidDataType = DPAID_INetPort;
        addressElements[2].dwDataSize   = sizeof(WORD);
        addressElements[2].lpData       = &port;
    }


    hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                 NULL, &dwAddressSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );

    if( hr == DPERR_BUFFERTOOSMALL )
    {
        pAddress = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwAddressSize );
        hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                     pAddress, &dwAddressSize );
        checkHR( DP_OK, hr );
    }

    hr = IDirectPlayX_InitializeConnection( pDP, pAddress, 0 );
    checkHR( DP_OK, hr );

    HeapFree( GetProcessHeap(), 0, pAddress );

}

static BOOL CALLBACK EnumSessions_cb_join( LPCDPSESSIONDESC2 lpThisSD,
                                           LPDWORD lpdwTimeOut,
                                           DWORD dwFlags,
                                           LPVOID lpContext )
{
    IDirectPlay4 *pDP = lpContext;
    DPSESSIONDESC2 dpsd;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
    {
        return FALSE;
    }

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = lpThisSD->guidInstance;

    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    checkHR( DP_OK, hr );

    return TRUE;
}


/* DirectPlayCreate */

static void test_DirectPlayCreate(void)
{

    IDirectPlay *pDP;
    HRESULT hr;

    /* TODO: Check how it behaves with pUnk!=NULL */

    /* pDP==NULL */
    hr = DirectPlayCreate( NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &GUID_NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* pUnk==NULL, pDP!=NULL */
    hr = DirectPlayCreate( NULL, &pDP, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = DirectPlayCreate( (LPGUID) &GUID_NULL, &pDP, NULL );
    checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );
    hr = DirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, &pDP, NULL );
    checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );

}

static BOOL CALLBACK callback_providersA(GUID* guid, char *name, DWORD major, DWORD minor, void *arg)
{
    struct provider_data *prov = arg;

    if (!prov) return TRUE;

    if (prov->call_count < sizeof(prov->guid_data) / sizeof(prov->guid_data[0]))
    {
        prov->guid_ptr[prov->call_count] = guid;
        prov->guid_data[prov->call_count] = *guid;

        prov->call_count++;
    }

    if (prov->ret_value) /* Only trace when looping all providers */
        trace("Provider #%d '%s' (%d.%d)\n", prov->call_count, name, major, minor);
    return prov->ret_value;
}

static BOOL CALLBACK callback_providersW(GUID* guid, WCHAR *name, DWORD major, DWORD minor, void *arg)
{
    struct provider_data *prov = arg;

    if (!prov) return TRUE;

    if (prov->call_count < sizeof(prov->guid_data) / sizeof(prov->guid_data[0]))
    {
        prov->guid_ptr[prov->call_count] = guid;
        prov->guid_data[prov->call_count] = *guid;

        prov->call_count++;
    }

    return prov->ret_value;
}

static void test_EnumerateProviders(void)
{
    HRESULT hr;
    int i;
    struct provider_data arg;

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = TRUE;

    hr = DirectPlayEnumerateA(callback_providersA, NULL);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateA failed\n");

    SetLastError(0xdeadbeef);
    hr = DirectPlayEnumerateA(NULL, &arg);
    ok(FAILED(hr), "DirectPlayEnumerateA expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%x\n", GetLastError());

    SetLastError(0xdeadbeef);
    hr = DirectPlayEnumerateA(NULL, NULL);
    ok(FAILED(hr), "DirectPlayEnumerateA expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%x\n", GetLastError());

    hr = DirectPlayEnumerateA(callback_providersA, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateA failed\n");
    ok(arg.call_count > 0, "Expected at least one valid provider\n");
    trace("Found %d providers\n", arg.call_count);

    /* The returned GUID values must have persisted after enumeration (bug 37185) */
    for(i = 0; i < arg.call_count; i++)
    {
        ok(IsEqualGUID(arg.guid_ptr[i], &arg.guid_data[i]), "#%d Expected equal GUID values\n", i);
    }

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = FALSE;
    hr = DirectPlayEnumerateA(callback_providersA, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateA failed\n");
    ok(arg.call_count == 1, "Expected 1, got %d\n", arg.call_count);

    hr = DirectPlayEnumerateW(callback_providersW, NULL);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateW failed\n");

    SetLastError(0xdeadbeef);
    hr = DirectPlayEnumerateW(NULL, &arg);
    ok(FAILED(hr), "DirectPlayEnumerateW expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%x\n", GetLastError());

    SetLastError(0xdeadbeef);
    hr = DirectPlayEnumerateW(NULL, NULL);
    ok(FAILED(hr), "DirectPlayEnumerateW expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%x\n", GetLastError());

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = TRUE;
    hr = DirectPlayEnumerateW(callback_providersW, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateW failed\n");
    ok(arg.call_count > 0, "Expected at least one valid provider\n");

    /* The returned GUID values must have persisted after enumeration (bug 37185) */
    for(i = 0; i < arg.call_count; i++)
    {
        ok(IsEqualGUID(arg.guid_ptr[i], &arg.guid_data[i]), "#%d Expected equal GUID values\n", i);
    }

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = FALSE;
    hr = DirectPlayEnumerateW(callback_providersW, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateW failed\n");
    ok(arg.call_count == 1, "Expected 1, got %d\n", arg.call_count);
}

/* EnumConnections */

static BOOL CALLBACK EnumAddress_cb2( REFGUID guidDataType,
                                      DWORD dwDataSize,
                                      LPCVOID lpData,
                                      LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;

    static REFGUID types[] = { &DPAID_TotalSize,
                               &DPAID_ServiceProvider,
                               &GUID_NULL };
    static DWORD sizes[] = { 4, 16, 0  };
    static REFGUID sps[] = { &DPSPGUID_SERIAL, &DPSPGUID_MODEM,
                             &DPSPGUID_IPX, &DPSPGUID_TCPIP };


    checkGuid( types[ callbackData->dwCounter2 ], guidDataType );
    check( sizes[ callbackData->dwCounter2 ], dwDataSize );

    if ( IsEqualGUID( types[0], guidDataType ) )
    {
        todo_wine check( 80, *((LPDWORD) lpData) );
    }
    else if ( IsEqualGUID( types[1], guidDataType ) )
    {
        todo_wine checkGuid( sps[ callbackData->dwCounter1 ], lpData );
    }

    callbackData->dwCounter2++;

    return TRUE;
}

static BOOL CALLBACK EnumConnections_cb( LPCGUID lpguidSP,
                                         LPVOID lpConnection,
                                         DWORD dwConnectionSize,
                                         LPCDPNAME lpName,
                                         DWORD dwFlags,
                                         LPVOID lpContext )
{

    lpCallbackData callbackData = lpContext;
    IDirectPlayLobby *pDPL;
    HRESULT hr;


    if (!callbackData->dwFlags)
    {
        callbackData->dwFlags = DPCONNECTION_DIRECTPLAY;
    }

    checkFlags( callbackData->dwFlags, dwFlags, FLAGS_DPCONNECTION );

    /* Get info from lpConnection */
    hr = CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                           &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlayLobby / IID_IDirectPlayLobby3A failed\n");
    if (FAILED(hr))
        return FALSE;

    callbackData->dwCounter2 = 0;
    IDirectPlayLobby_EnumAddress( pDPL, EnumAddress_cb2, lpConnection,
                                  dwConnectionSize, callbackData );
    todo_wine check( 3, callbackData->dwCounter2 );

    callbackData->dwCounter1++;

    return TRUE;
}

static void test_EnumConnections(void)
{

    IDirectPlay4 *pDP;
    CallbackData callbackData;
    HRESULT hr;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );

    ok (SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n");
    if (FAILED(hr)) return;

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, NULL, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, NULL,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );


    /* Flag tests */
    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = DPCONNECTION_DIRECTPLAY;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = DPCONNECTION_DIRECTPLAYLOBBY;
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = ( DPCONNECTION_DIRECTPLAY |
                             DPCONNECTION_DIRECTPLAYLOBBY );
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = ~( DPCONNECTION_DIRECTPLAY |
                              DPCONNECTION_DIRECTPLAYLOBBY );
    hr = IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( 0, callbackData.dwCounter1 );


    IDirectPlayX_Release( pDP );
}

/* InitializeConnection */

static BOOL CALLBACK EnumConnections_cb2( LPCGUID lpguidSP,
                                          LPVOID lpConnection,
                                          DWORD dwConnectionSize,
                                          LPCDPNAME lpName,
                                          DWORD dwFlags,
                                          LPVOID lpContext )
{
    IDirectPlay4 *pDP = lpContext;
    HRESULT hr;

    /* Incorrect parameters */
    hr = IDirectPlayX_InitializeConnection( pDP, NULL, 1 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 1 );
    checkHR( DPERR_INVALIDFLAGS, hr );

    /* Normal operation.
       We're only interested in ensuring that the TCP/IP provider works */

    if( IsEqualGUID(lpguidSP, &DPSPGUID_TCPIP) )
    {
        hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 0 );
        checkHR( DP_OK, hr );
        hr = IDirectPlayX_InitializeConnection( pDP, lpConnection, 0 );
        checkHR( DPERR_ALREADYINITIALIZED, hr );
    }

    return TRUE;
}

static void test_InitializeConnection(void)
{

    IDirectPlay4 *pDP;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );

    ok (SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n");
    if (FAILED(hr)) return;

    IDirectPlayX_EnumConnections( pDP, &appGuid, EnumConnections_cb2, pDP, 0 );

    IDirectPlayX_Release( pDP );
}

/* GetCaps */

static void test_GetCaps(void)
{

    IDirectPlay4 *pDP;
    DPCAPS dpcaps;
    DWORD dwFlags;
    HRESULT hr;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok (SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n");
    if (FAILED(hr)) return;

    ZeroMemory( &dpcaps, sizeof(DPCAPS) );

    /* Service provider not ininitialized */
    hr = IDirectPlayX_GetCaps( pDP, &dpcaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    /* dpcaps not ininitialized */
    hr = IDirectPlayX_GetCaps( pDP, &dpcaps, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpcaps.dwSize = sizeof(DPCAPS);

    for (dwFlags=0;
         dwFlags<=DPGETCAPS_GUARANTEED;
         dwFlags+=DPGETCAPS_GUARANTEED)
    {

        hr = IDirectPlayX_GetCaps( pDP, &dpcaps, dwFlags );
        checkHR( DP_OK, hr );


        if ( hr == DP_OK )
        {
            check( sizeof(DPCAPS), dpcaps.dwSize );
            check( DPCAPS_ASYNCSUPPORTED |
                   DPCAPS_GUARANTEEDOPTIMIZED |
                   DPCAPS_GUARANTEEDSUPPORTED,
                   dpcaps.dwFlags );
            check( 0,     dpcaps.dwMaxQueueSize );
            check( 0,     dpcaps.dwHundredBaud );
            check( 500,   dpcaps.dwLatency );
            check( 65536, dpcaps.dwMaxLocalPlayers );
            check( 20,    dpcaps.dwHeaderLength );
            check( 5000,  dpcaps.dwTimeout );

            switch (dwFlags)
            {
            case 0:
                check( 65479,   dpcaps.dwMaxBufferSize );
                check( 65536,   dpcaps.dwMaxPlayers );
                break;
            case DPGETCAPS_GUARANTEED:
                check( 1048547, dpcaps.dwMaxBufferSize );
                check( 64,      dpcaps.dwMaxPlayers );
                break;
            default: break;
            }
        }
    }

    IDirectPlayX_Release( pDP );
}

/* Open */

static BOOL CALLBACK EnumSessions_cb2( LPCDPSESSIONDESC2 lpThisSD,
                                       LPDWORD lpdwTimeOut,
                                       DWORD dwFlags,
                                       LPVOID lpContext )
{
    IDirectPlay4 *pDP = lpContext;
    DPSESSIONDESC2 dpsd;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
        return FALSE;


    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = lpThisSD->guidInstance;

    if ( lpThisSD->dwFlags & DPSESSION_PASSWORDREQUIRED )
    {
        /* Incorrect password */
        U2(dpsd).lpszPasswordA = (LPSTR) "sonic boom";
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DPERR_INVALIDPASSWORD, hr );

        /* Correct password */
        U2(dpsd).lpszPasswordA = (LPSTR) "hadouken";
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DP_OK, hr );
    }
    else
    {
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DP_OK, hr );
    }

    hr = IDirectPlayX_Close( pDP );
    checkHR( DP_OK, hr );

    return TRUE;
}

static void test_Open(void)
{

    IDirectPlay4 *pDP, *pDP_server;
    DPSESSIONDESC2 dpsd, dpsd_server;
    HRESULT hr;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP_server );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    ZeroMemory( &dpsd_server, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );

    /* Service provider not initialized */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    init_TCPIP_provider( pDP_server, "127.0.0.1", 0 );
    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    /* Uninitialized  dpsd */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    checkHR( DPERR_INVALIDPARAMS, hr );


    dpsd_server.dwSize = sizeof(DPSESSIONDESC2);
    dpsd_server.guidApplication = appGuid;
    dpsd_server.dwMaxPlayers = 10;


    /* Regular operation */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    checkHR( DP_OK, hr );

    /* Opening twice */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_ALREADYINITIALIZED, hr );

    /* Session flags */
    IDirectPlayX_Close( pDP_server );

    dpsd_server.dwFlags = DPSESSION_CLIENTSERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );

    dpsd_server.dwFlags = DPSESSION_MULTICASTSERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );

    dpsd_server.dwFlags = DPSESSION_SECURESERVER | DPSESSION_MIGRATEHOST;
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDFLAGS, hr );


    /* Joining sessions */
    /* - Checking how strict dplay is with sizes */
    dpsd.dwSize = 0;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)-1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)+1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_NOSESSIONS, hr ); /* Only checks for size, not guids */


    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = appGuid;


    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN | DPOPEN_CREATE );
    todo_wine checkHR( DPERR_NOSESSIONS, hr ); /* Second flag is ignored */

    dpsd_server.dwFlags = 0;


    /* Join to normal session */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb2, pDP, 0 );


    /* Already initialized session */
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_ALREADYINITIALIZED, hr );


    /* Checking which is the error checking order */
    dpsd_server.dwSize = 0;

    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd_server.dwSize = sizeof(DPSESSIONDESC2);


    /* Join to protected session */
    IDirectPlayX_Close( pDP_server );
    U2(dpsd_server).lpszPasswordA = (LPSTR) "hadouken";
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb2,
                               pDP, DPENUMSESSIONS_PASSWORDREQUIRED );


    IDirectPlayX_Release( pDP );
    IDirectPlayX_Release( pDP_server );

}

/* EnumSessions */

static BOOL CALLBACK EnumSessions_cb( LPCDPSESSIONDESC2 lpThisSD,
                                      LPDWORD lpdwTimeOut,
                                      DWORD dwFlags,
                                      LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    callbackData->dwCounter1++;

    if ( dwFlags & DPESC_TIMEDOUT )
    {
        check( TRUE, lpThisSD == NULL );
        return FALSE;
    }
    check( FALSE, lpThisSD == NULL );


    if ( U2(*lpThisSD).lpszPasswordA != NULL )
    {
        check( TRUE, (lpThisSD->dwFlags & DPSESSION_PASSWORDREQUIRED) != 0 );
    }

    if ( lpThisSD->dwFlags & DPSESSION_NEWPLAYERSDISABLED )
    {
        check( 0, lpThisSD->dwCurrentPlayers );
    }

    check( sizeof(*lpThisSD), lpThisSD->dwSize );
    checkLP( NULL, U2(*lpThisSD).lpszPasswordA );

    return TRUE;
}

static IDirectPlay4 *create_session(DPSESSIONDESC2 *lpdpsd)
{

    IDirectPlay4 *pDP;
    DPNAME name;
    DPID dpid;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return NULL;

    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    hr = IDirectPlayX_Open( pDP, lpdpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    if ( ! (lpdpsd->dwFlags & DPSESSION_NEWPLAYERSDISABLED) )
    {
        ZeroMemory( &name, sizeof(DPNAME) );
        name.dwSize = sizeof(DPNAME);
        U1(name).lpszShortNameA = (LPSTR) "bofh";

        hr = IDirectPlayX_CreatePlayer( pDP, &dpid, &name, NULL, NULL,
                                        0, DPPLAYER_SERVERPLAYER );
        todo_wine checkHR( DP_OK, hr );
    }

    return pDP;

}

static void test_EnumSessions(void)
{

#define N_SESSIONS 6

    IDirectPlay4 *pDP, *pDPserver[N_SESSIONS];
    DPSESSIONDESC2 dpsd, dpsd_server[N_SESSIONS];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    callbackData.dwCounter1 = -1; /* So that after a call to EnumSessions
                                     we get the exact number of sessions */
    callbackData.dwFlags = 0;


    /* Service provider not initialized */
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP, "127.0.0.1", 0 );


    /* Session with no size */
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "EnumSessions not implemented\n" );
        return;
    }

    dpsd.dwSize = sizeof(DPSESSIONDESC2);


    /* No sessions */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );


    dpsd.guidApplication = appGuid;

    /* Set up sessions */
    for (i=0; i<N_SESSIONS; i++)
    {
        memcpy( &dpsd_server[i], &dpsd, sizeof(DPSESSIONDESC2) );
    }

    U1(dpsd_server[0]).lpszSessionNameA = (LPSTR) "normal";
    dpsd_server[0].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[0].dwMaxPlayers = 10;

    U1(dpsd_server[1]).lpszSessionNameA = (LPSTR) "full";
    dpsd_server[1].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[1].dwMaxPlayers = 1;

    U1(dpsd_server[2]).lpszSessionNameA = (LPSTR) "no new";
    dpsd_server[2].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_NEWPLAYERSDISABLED );
    dpsd_server[2].dwMaxPlayers = 10;

    U1(dpsd_server[3]).lpszSessionNameA = (LPSTR) "no join";
    dpsd_server[3].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_JOINDISABLED );
    dpsd_server[3].dwMaxPlayers = 10;

    U1(dpsd_server[4]).lpszSessionNameA = (LPSTR) "private";
    dpsd_server[4].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PRIVATE );
    dpsd_server[4].dwMaxPlayers = 10;
    U2(dpsd_server[4]).lpszPasswordA = (LPSTR) "password";

    U1(dpsd_server[5]).lpszSessionNameA = (LPSTR) "protected";
    dpsd_server[5].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PASSWORDREQUIRED );
    dpsd_server[5].dwMaxPlayers = 10;
    U2(dpsd_server[5]).lpszPasswordA = (LPSTR) "password";


    for (i=0; i<N_SESSIONS; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
        if (!pDPserver[i]) return;
    }


    /* Invalid params */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, -1 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_EnumSessions( pDP, NULL, 0, EnumSessions_cb,
                                    &callbackData, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    check( -1, callbackData.dwCounter1 );


    /* Flag tests */
    callbackData.dwFlags = DPENUMSESSIONS_ALL; /* Doesn't list private,
                                                  protected */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-2, callbackData.dwCounter1 );

    /* Doesn't list private */
    callbackData.dwFlags = ( DPENUMSESSIONS_ALL |
                             DPENUMSESSIONS_PASSWORDREQUIRED );
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-1, callbackData.dwCounter1 );

    /* Doesn't list full, no new, no join, private, protected */
    callbackData.dwFlags = DPENUMSESSIONS_AVAILABLE;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-5, callbackData.dwCounter1 );

    /* Like with DPENUMSESSIONS_AVAILABLE */
    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-5, callbackData.dwCounter1 );

    /* Doesn't list full, no new, no join, private */
    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-4, callbackData.dwCounter1 );


    /* Async enumeration */
    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-4, callbackData.dwCounter1 ); /* Read cache of last
                                                       sync enumeration */

    callbackData.dwFlags = DPENUMSESSIONS_STOPASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 ); /* Stop enumeration */

    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 ); /* Start enumeration */

    Sleep(500); /* Give time to fill the cache */

    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( N_SESSIONS-5, callbackData.dwCounter1 ); /* Retrieve results */

    callbackData.dwFlags = DPENUMSESSIONS_STOPASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 ); /* Stop enumeration */


    /* Specific tests for passworded sessions */

    for (i=0; i<N_SESSIONS; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
    }

    /* - Only session password set */
    for (i=4;i<=5;i++)
    {
        U2(dpsd_server[i]).lpszPasswordA = (LPSTR) "password";
        dpsd_server[i].dwFlags = 0;
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Both sessions automatically
                                            set DPSESSION_PASSWORDREQUIRED */

    /* - Only session flag set */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        U2(dpsd_server[i]).lpszPasswordA = NULL;
    }
    dpsd_server[4].dwFlags = DPSESSION_PRIVATE;
    dpsd_server[5].dwFlags = DPSESSION_PASSWORDREQUIRED;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Without password,
                                            the flag is ignored */

    /* - Both session flag and password set */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        U2(dpsd_server[i]).lpszPasswordA = (LPSTR) "password";
    }
    dpsd_server[4].dwFlags = DPSESSION_PRIVATE;
    dpsd_server[5].dwFlags = DPSESSION_PASSWORDREQUIRED;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    /* - Listing without password */
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );

    /* - Listing with incorrect password */
    U2(dpsd).lpszPasswordA = (LPSTR) "bad_password";
    callbackData.dwFlags = 0;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwFlags = DPENUMSESSIONS_PASSWORDREQUIRED;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );

    /* - Listing with  correct password */
    U2(dpsd).lpszPasswordA = (LPSTR) "password";
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );


    U2(dpsd).lpszPasswordA = NULL;
    callbackData.dwFlags = DPENUMSESSIONS_ASYNC;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Read cache of last sync enumeration,
                                            even private sessions */


    /* GUID tests */

    /* - Creating two servers with different application GUIDs */
    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
        dpsd_server[i].dwFlags = ( DPSESSION_CLIENTSERVER |
                                   DPSESSION_DIRECTPLAYPROTOCOL );
        U2(dpsd_server[i]).lpszPasswordA = NULL;
        dpsd_server[i].dwMaxPlayers = 10;
    }
    U1(dpsd_server[4]).lpszSessionNameA = (LPSTR) "normal1";
    dpsd_server[4].guidApplication = appGuid;
    U1(dpsd_server[5]).lpszSessionNameA = (LPSTR) "normal2";
    dpsd_server[5].guidApplication = appGuid2;
    for (i=4; i<=5; i++)
    {
        pDPserver[i] = create_session( &dpsd_server[i] );
    }

    callbackData.dwFlags = 0;

    dpsd.guidApplication = appGuid2;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 ); /* Only one of the sessions */

    dpsd.guidApplication = appGuid;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 ); /* The other session */
    /* FIXME:
       For some reason, if we enum 1st with appGuid and 2nd with appGuid2,
       in the second enum we get the 2 sessions. Dplay fault? Elves? */

    dpsd.guidApplication = GUID_NULL;
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Both sessions */

    for (i=4; i<=5; i++)
    {
        IDirectPlayX_Release( pDPserver[i] );
    }
    IDirectPlayX_Release( pDP );

}

/* SetSessionDesc
   GetSessionDesc */

static void test_SessionDesc(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    LPDPSESSIONDESC2 lpData[2];
    LPVOID lpDataMsg;
    DPID dpid[2];
    DWORD dwDataSize;
    HRESULT hr;
    UINT i;
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );

    /* Service provider not initialized */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No sessions open */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip("Get/SetSessionDesc not implemented\n");
        return;
    }

    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_NOSESSIONS, hr );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;


    /* Host */
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    /* Peer */
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    for (i=0; i<2; i++)
    {
        /* Players, only to receive messages */
        IDirectPlayX_CreatePlayer( pDP[i], &dpid[i], NULL, NULL, NULL, 0, 0 );

        lpData[i] = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );
    }
    lpDataMsg = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );


    /* Incorrect parameters */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], NULL, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, NULL );
    checkHR( DPERR_INVALIDPARAM, hr );
if(0)
{
    /* Crashes under Win7 */
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], NULL );
    checkHR( DPERR_INVALIDPARAM, hr );
    dwDataSize=-1;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwDataSize );
}

    /* Get: Insufficient buffer size */
    dwDataSize=0;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );
    dwDataSize=4;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );
    dwDataSize=1024;
    hr = IDirectPlayX_GetSessionDesc( pDP[0], NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dpsd.dwSize, dwDataSize );

    /* Get: Regular operation
     *  i=0: Local session
     *  i=1: Remote session */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_GetSessionDesc( pDP[i], lpData[i], &dwDataSize );
        checkHR( DP_OK, hr );
        check( sizeof(DPSESSIONDESC2), dwDataSize );
        check( sizeof(DPSESSIONDESC2), lpData[i]->dwSize );
        checkGuid( &appGuid, &lpData[i]->guidApplication );
        check( dpsd.dwMaxPlayers, lpData[i]->dwMaxPlayers );
    }

    checkGuid( &lpData[0]->guidInstance, &lpData[1]->guidInstance );

    /* Set: Regular operation */
    U1(dpsd).lpszSessionNameA = (LPSTR) "Wahaa";
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetSessionDesc( pDP[1], lpData[1], &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( U1(dpsd).lpszSessionNameA, U1(*lpData[1]).lpszSessionNameA );


    /* Set: Failing to modify a remote session */
    hr = IDirectPlayX_SetSessionDesc( pDP[1], &dpsd, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );

    /* Trying to change immutable properties */
    /*  Flags */
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    dpsd.dwFlags = DPSESSION_SECURESERVER;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    dpsd.dwFlags = 0;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    /*  Size */
    dpsd.dwSize = 2048;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    /* Changing the GUIDs and size is ignored */
    dpsd.guidApplication = appGuid2;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );
    dpsd.guidInstance = appGuid2;
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_GetSessionDesc( pDP[0], lpData[0], &dwDataSize );
    checkHR( DP_OK, hr );
    checkGuid( &appGuid, &lpData[0]->guidApplication );
    checkGuid( &lpData[1]->guidInstance, &lpData[0]->guidInstance );
    check( sizeof(DPSESSIONDESC2), lpData[0]->dwSize );


    /* Checking system messages */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    checkStr( "48,90,90,90,90,90,90,", callbackData.szTrace2 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "S1,S1,S1,S1,S1,S1,", callbackData.szTrace1 );
    checkStr( "90,90,90,90,90,90,", callbackData.szTrace2 );

    HeapFree( GetProcessHeap(), 0, lpDataMsg );
    for (i=0; i<2; i++)
    {
        HeapFree( GetProcessHeap(), 0, lpData[i] );
        IDirectPlayX_Release( pDP[i] );
    }

}

/* CreatePlayer */

static void test_CreatePlayer(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPNAME name;
    DPID dpid;
    HRESULT hr;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP[0] );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP[1] );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &name, sizeof(DPNAME) );


    /* Connection not initialized */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* Session not open */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "CreatePlayer not implemented\n" );
        return;
    }

    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );


    /* Player name */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    name.dwSize = -1;


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, &name, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    name.dwSize = sizeof(DPNAME);
    U1(name).lpszShortNameA = (LPSTR) "test";
    U2(name).lpszLongNameA = NULL;


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, &name, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );


    /* Null dpid */
    hr = IDirectPlayX_CreatePlayer( pDP[0], NULL, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* There can only be one server player */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    IDirectPlayX_DestroyPlayer( pDP[0], dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );


    /* Flags */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SPECTATOR );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, ( DPPLAYER_SERVERPLAYER |
                                         DPPLAYER_SPECTATOR ) );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );
    IDirectPlayX_DestroyPlayer( pDP[0], dpid );


    /* Session with DPSESSION_NEWPLAYERSDISABLED */
    IDirectPlayX_Close( pDP[0] );
    dpsd.dwFlags = DPSESSION_NEWPLAYERSDISABLED;
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_CANTCREATEPLAYER, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SPECTATOR );
    checkHR( DPERR_CANTCREATEPLAYER, hr );


    /* Creating players in a Client/Server session */
    IDirectPlayX_Close( pDP[0] );
    dpsd.dwFlags = DPSESSION_CLIENTSERVER;
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( DPID_SERVERPLAYER, dpid );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid, NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DPERR_INVALIDFLAGS, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid, NULL, NULL, NULL,
                                    0, 0 );
    checkHR( DP_OK, hr );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetPlayerCaps */

static void test_GetPlayerCaps(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    HRESULT hr;
    UINT i;

    DPCAPS playerCaps;
    DWORD dwFlags;


    for (i=0; i<2; i++)
    {
        hr= CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                              &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;

    ZeroMemory( &playerCaps, sizeof(DPCAPS) );


    /* Uninitialized service provider */
    playerCaps.dwSize = 0;
    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    playerCaps.dwSize = sizeof(DPCAPS);
    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No session */
    playerCaps.dwSize = 0;

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "GetPlayerCaps not implemented\n" );
        return;
    }

    playerCaps.dwSize = sizeof(DPCAPS);

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );


    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i],
                                        NULL, NULL, NULL, 0, 0 );
        checkHR( DP_OK, hr );
    }


    /* Uninitialized playerCaps */
    playerCaps.dwSize = 0;

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[0], &playerCaps, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* Invalid player */
    playerCaps.dwSize = sizeof(DPCAPS);

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 0, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], 2, &playerCaps, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[0], &playerCaps, 0 );
    checkHR( DP_OK, hr );


    /* Regular parameters */
    for (i=0; i<2; i++)
    {
        for (dwFlags=0;
             dwFlags<=DPGETCAPS_GUARANTEED;
             dwFlags+=DPGETCAPS_GUARANTEED)
        {

            hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[i],
                                             &playerCaps, dwFlags );
            checkHR( DP_OK, hr );


            check( sizeof(DPCAPS), playerCaps.dwSize );
            check( 40,    playerCaps.dwSize );
            check( 0,     playerCaps.dwMaxQueueSize );
            check( 0,     playerCaps.dwHundredBaud );
            check( 0,     playerCaps.dwLatency );
            check( 65536, playerCaps.dwMaxLocalPlayers );
            check( 20,    playerCaps.dwHeaderLength );

            if ( i == 0 )
            {
                checkFlags( DPCAPS_ISHOST |
                            DPCAPS_GUARANTEEDOPTIMIZED |
                            DPCAPS_GUARANTEEDSUPPORTED |
                            DPCAPS_ASYNCSUPPORTED |
                            DPPLAYERCAPS_LOCAL,
                            playerCaps.dwFlags, FLAGS_DPCAPS );
            }
            else
                checkFlags( DPCAPS_ISHOST |
                            DPCAPS_GUARANTEEDOPTIMIZED |
                            DPCAPS_GUARANTEEDSUPPORTED |
                            DPCAPS_ASYNCSUPPORTED,
                            playerCaps.dwFlags, FLAGS_DPCAPS );

            if ( dwFlags == DPGETCAPS_GUARANTEED )
            {
                check( 1048547, playerCaps.dwMaxBufferSize );
                check( 64,      playerCaps.dwMaxPlayers );
            }
            else
            {
                check( 65479, playerCaps.dwMaxBufferSize );
                check( 65536, playerCaps.dwMaxPlayers );
            }

        }
    }


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* SetPlayerData
   GetPlayerData */

static void test_PlayerData(void)
{
    IDirectPlay4 *pDP;
    DPSESSIONDESC2 dpsd;
    DPID dpid;
    HRESULT hr;

    /* lpDataFake has to be bigger than the rest, limits lpDataGet size */
    LPCSTR lpDataFake     = "big_fake_data_chunk";
    DWORD dwDataSizeFake  = strlen(lpDataFake)+1;

    LPCSTR lpData         = "remote_data";
    DWORD dwDataSize      = strlen(lpData)+1;

    LPCSTR lpDataLocal    = "local_data";
    DWORD dwDataSizeLocal = strlen(lpDataLocal)+1;

    LPSTR lpDataGet       = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                       dwDataSizeFake );
    DWORD dwDataSizeGet   = dwDataSizeFake;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    /* No service provider */
    hr = IDirectPlayX_SetPlayerData( pDP, 0, (LPVOID) lpData,
                                     dwDataSize, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, 0, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );


    /* Invalid player */
    hr = IDirectPlayX_SetPlayerData( pDP, 0, (LPVOID) lpData,
                                     dwDataSize, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, 0, lpDataGet, &dwDataSizeGet, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "Get/SetPlayerData not implemented\n" );
        return;
    }

    /* Create the player */
    /* By default, the data is remote */
    hr = IDirectPlayX_CreatePlayer( pDP, &dpid, NULL, NULL, (LPVOID) lpData,
                                    dwDataSize, 0 );
    checkHR( DP_OK, hr );

    /* Invalid parameters */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, NULL, dwDataSize, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, lpDataGet, -1, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, NULL, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /*
     * Remote data (default)
     */


    /* Buffer redimension */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL,
                                     &dwDataSizeGet, 0 );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = 2;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSize, dwDataSizeGet );

    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpData, lpDataGet );

    /* Normal operation */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet );
    checkStr( lpData, lpDataGet );

    /* Flag tests */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Remote: works as expected */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Same behaviour as in previous test */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 (as local data doesn't exist) */
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL | DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Same behaviour as in previous test */
    checkStr( lpDataFake, lpDataGet );

    /* Getting local data (which doesn't exist), buffer size is ignored */
    dwDataSizeGet = 0;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 */
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL, &dwDataSizeGet,
                                     DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, dwDataSizeGet ); /* Sets size to 0 */
    checkStr( lpDataFake, lpDataGet );


    /*
     * Local data
     */


    /* Invalid flags */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal,
                                     DPSET_LOCAL | DPSET_GUARANTEED );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* Correct parameters */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal, DPSET_LOCAL );
    checkHR( DP_OK, hr );

    /* Flag tests (again) */
    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Remote: works as expected */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSize, dwDataSizeGet ); /* Like in previous test */
    checkStr( lpData, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet ); /* Local: works as expected */
    checkStr( lpDataLocal, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL | DPGET_REMOTE );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet ); /* Like in previous test */
    checkStr( lpDataLocal, lpDataGet );

    /* Small buffer works as expected again */
    dwDataSizeGet = 0;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet,
                                     DPGET_LOCAL );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, NULL,
                                     &dwDataSizeGet, DPGET_LOCAL );
    check( DPERR_BUFFERTOOSMALL, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );


    /*
     * Changing remote data
     */


    /* Remote data := local data */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal,
                                     DPSET_GUARANTEED | DPSET_REMOTE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataLocal,
                                     dwDataSizeLocal, 0 );
    checkHR( DP_OK, hr );

    dwDataSizeGet = dwDataSizeFake;
    strcpy(lpDataGet, lpDataFake);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSizeLocal, dwDataSizeGet );
    checkStr( lpDataLocal, lpDataGet );

    /* Remote data := fake data */
    hr = IDirectPlayX_SetPlayerData( pDP, dpid, (LPVOID) lpDataFake,
                                     dwDataSizeFake, DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSizeGet = dwDataSizeFake + 1;
    strcpy(lpDataGet, lpData);
    hr = IDirectPlayX_GetPlayerData( pDP, dpid, lpDataGet, &dwDataSizeGet, 0 );
    checkHR( DP_OK, hr );
    check( dwDataSizeFake, dwDataSizeGet );
    checkStr( lpDataFake, lpDataGet );


    HeapFree( GetProcessHeap(), 0, lpDataGet );
    IDirectPlayX_Release( pDP );
}

/* GetPlayerName
   SetPlayerName */

static void test_PlayerName(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    HRESULT hr;
    UINT i;

    DPNAME playerName;
    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        hr= CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                              &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    ZeroMemory( &playerName, sizeof(DPNAME) );


    /* Service provider not initialized */
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_UNINITIALIZED, hr );
    check( 1024, dwDataSize );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* Session not initialized */
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "Get/SetPlayerName not implemented\n" );
        return;
    }

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[1], &dpid[1], NULL, NULL, NULL, 0, 0 );


    /* Name not initialized */
    playerName.dwSize = -1;
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, 0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    playerName.dwSize = sizeof(DPNAME);
    U1(playerName).lpszShortNameA = (LPSTR) "player_name";
    U2(playerName).lpszLongNameA = (LPSTR) "player_long_name";


    /* Invalid parameters */
    hr = IDirectPlayX_SetPlayerName( pDP[0], -1, &playerName, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );
    hr = IDirectPlayX_SetPlayerName( pDP[0], 0, &playerName, 0 );
    checkHR( DPERR_INVALIDPLAYER, hr );
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, -1 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );

if(0)
{
    /* Crashes under Win7 */
    dwDataSize = -1;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwDataSize );
}

    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* Trying to modify remote player */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[1], &playerName, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );


    /* Regular operation */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName, 0 );
    checkHR( DP_OK, hr );
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( U1(playerName).lpszShortNameA, U1(*(LPDPNAME)lpData).lpszShortNameA );
    checkStr( U2(playerName).lpszLongNameA,  U2(*(LPDPNAME)lpData).lpszLongNameA );
    check( 0,                            ((LPDPNAME)lpData)->dwFlags );

    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], NULL, 0 );
    checkHR( DP_OK, hr );
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 16, dwDataSize );
    checkLP( NULL, U1(*(LPDPNAME)lpData).lpszShortNameA );
    checkLP( NULL, U2(*(LPDPNAME)lpData).lpszLongNameA );
    check( 0,      ((LPDPNAME)lpData)->dwFlags );


    /* Small buffer in get operation */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 16, dwDataSize );

    dwDataSize = 0;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 16, dwDataSize );

    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 16, dwDataSize );
    checkLP( NULL, U1(*(LPDPNAME)lpData).lpszShortNameA );
    checkLP( NULL, U2(*(LPDPNAME)lpData).lpszLongNameA );
    check( 0, ((LPDPNAME)lpData)->dwFlags );


    /* Flags */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_GUARANTEED );
    checkHR( DP_OK, hr );
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( U1(playerName).lpszShortNameA, U1(*(LPDPNAME)lpData).lpszShortNameA );
    checkStr( U2(playerName).lpszLongNameA,  U2(*(LPDPNAME)lpData).lpszLongNameA );
    check( 0, ((LPDPNAME)lpData)->dwFlags );

    /* - Local (no propagation) */
    U1(playerName).lpszShortNameA = (LPSTR) "no_propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 48, dwDataSize );
    checkStr( "no_propagation", U1(*(LPDPNAME)lpData).lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", U1(*(LPDPNAME)lpData).lpszShortNameA );

    /* -- 2 */

    U1(playerName).lpszShortNameA = (LPSTR) "no_propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL | DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 50, dwDataSize );
    checkStr( "no_propagation_2", U1(*(LPDPNAME)lpData).lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", U1(*(LPDPNAME)lpData).lpszShortNameA );

    /* - Remote (propagation, default) */
    U1(playerName).lpszShortNameA = (LPSTR) "propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "propagation", U1(*(LPDPNAME)lpData).lpszShortNameA );

    /* -- 2 */
    U1(playerName).lpszShortNameA = (LPSTR) "propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 47, dwDataSize );
    checkStr( "propagation_2", U1(*(LPDPNAME)lpData).lpszShortNameA );


    /* Checking system messages */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    checkStr( "48,28,57,28,57,57,59,", callbackData.szTrace2 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "S1,S1,S1,S1,S1,S1,", callbackData.szTrace1 );
    checkStr( "28,57,28,57,57,59,", callbackData.szTrace2 );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetPlayerAccount */

static BOOL CALLBACK EnumSessions_cb_join_secure( LPCDPSESSIONDESC2 lpThisSD,
                                                  LPDWORD lpdwTimeOut,
                                                  DWORD dwFlags,
                                                  LPVOID lpContext )
{
    IDirectPlay4 *pDP = lpContext;
    DPSESSIONDESC2 dpsd;
    DPCREDENTIALS dpCredentials;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
    {
        return FALSE;
    }

    checkFlags( DPSESSION_SECURESERVER, lpThisSD->dwFlags, FLAGS_DPSESSION );

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.guidInstance = lpThisSD->guidInstance;

    ZeroMemory( &dpCredentials, sizeof(DPCREDENTIALS) );
    dpCredentials.dwSize = sizeof(DPCREDENTIALS);
    U1(dpCredentials).lpszUsernameA = (LPSTR) "user";
    U2(dpCredentials).lpszPasswordA = (LPSTR) "pass";
    hr = IDirectPlayX_SecureOpen( pDP, &dpsd, DPOPEN_JOIN,
                                  NULL, &dpCredentials );
    checkHR( DPERR_LOGONDENIED, hr ); /* TODO: Make this work */

    return TRUE;
}

static void test_GetPlayerAccount(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    HRESULT hr;
    UINT i;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;

    /* Uninitialized service provider */
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], 0, 0, lpData, &dwDataSize );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetPlayerAccount not implemented\n" );
        return;
    }


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No session */
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], 0, 0, lpData, &dwDataSize );
    checkHR( DPERR_NOSESSIONS, hr );


    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i], NULL, NULL, NULL,
                                        0, 0 );
        checkHR( DP_OK, hr );
    }


    /* Session is not secure */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_UNSUPPORTED, hr );
    check( 1024, dwDataSize );


    /* Open a secure session */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_Close( pDP[i] );
        checkHR( DP_OK, hr );
    }

    dpsd.dwFlags = DPSESSION_SECURESERVER;
    hr = IDirectPlayX_SecureOpen( pDP[0], &dpsd, DPOPEN_CREATE, NULL, NULL );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[0],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0,
                                    EnumSessions_cb_join_secure, pDP[1], 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid[1],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* TODO: Player creation so that this works */

    /* Invalid player */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], 0, 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );

    /* Invalid flags */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], -1,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( 1024, dwDataSize );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 1,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( 1024, dwDataSize );

    /* Small buffer */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 0,
                                        NULL, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 0, dwDataSize );

    dwDataSize = 0;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 0, dwDataSize );

    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 0, dwDataSize );

    /* Normal operation */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAccount( pDP[0], dpid[0], 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetPlayerAddress */

static BOOL CALLBACK EnumAddress_cb( REFGUID guidDataType,
                                     DWORD dwDataSize,
                                     LPCVOID lpData,
                                     LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    static REFGUID types[] = { &DPAID_TotalSize,
                               &DPAID_ServiceProvider,
                               &DPAID_INet,
                               &DPAID_INetW };
    static DWORD sizes[] = { 4, 16, 12, 24, 4, 16, 10, 20 };


    checkGuid( types[callbackData->dwCounter1%4], guidDataType );
    check( sizes[callbackData->dwCounter1], dwDataSize );

    switch(callbackData->dwCounter1)
    {
    case 0:
        check( 136, *(LPDWORD) lpData );
        break;
    case 4:
        check( 130, *(LPDWORD) lpData );
        break;
    case 1:
    case 5:
        checkGuid( &DPSPGUID_TCPIP, lpData );
        break;
    case 6:
        checkStr( "127.0.0.1", (LPSTR) lpData );
        break;
    default: break;
    }


    callbackData->dwCounter1++;

    return TRUE;
}

static void test_GetPlayerAddress(void)
{

    IDirectPlay4 *pDP[2];
    IDirectPlayLobby3 *pDPL;
    DPSESSIONDESC2 dpsd;
    DPID dpid[2];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    hr = CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                           &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlayLobby / IID_IDirectPlayLobby3A failed\n" );
    if (FAILED(hr)) return;

    /* Uninitialized service provider */
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], 0, lpData, &dwDataSize );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetPlayerAddress not implemented\n" );
        return;
    }

    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No session */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], 0, lpData, &dwDataSize );
    checkHR( DPERR_UNSUPPORTED, hr );
    check( 1024, dwDataSize );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], 1, lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i], NULL, NULL, NULL,
                                        0, 0 );
        checkHR( DP_OK, hr );
    }

    /* Invalid player */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], 0,
                                        lpData, &dwDataSize );
    checkHR( DPERR_UNSUPPORTED, hr );
    check( 1024, dwDataSize );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], 1,
                                        lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( 1024, dwDataSize );

    /* Small buffer */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], dpid[0],
                                        NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 136, dwDataSize );

    dwDataSize = 0;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], dpid[0],
                                        lpData, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 136, dwDataSize );

    hr = IDirectPlayX_GetPlayerAddress( pDP[0], dpid[0],
                                        lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 136, dwDataSize );


    /* Regular parameters */
    callbackData.dwCounter1 = 0;

    /* - Local */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], dpid[0],
                                        lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 136, dwDataSize );

    hr = IDirectPlayLobby_EnumAddress( pDPL, EnumAddress_cb, lpData, dwDataSize,
                                       &callbackData );
    checkHR( DP_OK, hr );

    check( 4, callbackData.dwCounter1 );

    /* - Remote */
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerAddress( pDP[0], dpid[1],
                                        lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 130, dwDataSize );

    hr = IDirectPlayLobby_EnumAddress( pDPL, EnumAddress_cb, lpData, dwDataSize,
                                       &callbackData );
    checkHR( DP_OK, hr );

    check( 8, callbackData.dwCounter1 );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetPlayerFlags */

static void test_GetPlayerFlags(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4];
    HRESULT hr;
    UINT i;

    DWORD dwFlags = 0;


    for (i=0; i<2; i++)
    {
        hr= CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                              &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;

    /* Uninitialized service provider */
    hr = IDirectPlayX_GetPlayerFlags( pDP[0], 0, &dwFlags );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetPlayerFlags not implemented\n" );
        return;
    }

    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    /* No session */
    hr = IDirectPlayX_GetPlayerFlags( pDP[0], 0, &dwFlags );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerFlags( pDP[0], 1, &dwFlags );
    checkHR( DPERR_INVALIDPLAYER, hr );


    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i],
                                        NULL, NULL, NULL, 0, 0 );
        checkHR( DP_OK, hr );
    }
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[2],
                                    NULL, NULL, NULL,
                                    0, DPPLAYER_SPECTATOR );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[3],
                                    NULL, NULL, NULL,
                                    0, DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );


    /* Invalid player */
    hr = IDirectPlayX_GetPlayerFlags( pDP[0], 0, &dwFlags );
    checkHR( DPERR_INVALIDPLAYER, hr );

    hr = IDirectPlayX_GetPlayerFlags( pDP[0], 2, &dwFlags );
    checkHR( DPERR_INVALIDPLAYER, hr );

    /* Invalid parameters */
    hr = IDirectPlayX_GetPlayerFlags( pDP[0], dpid[0], NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* Regular parameters */
    hr = IDirectPlayX_GetPlayerFlags( pDP[0], dpid[0], &dwFlags );
    checkHR( DP_OK, hr );
    checkFlags( dwFlags, DPPLAYER_LOCAL, FLAGS_DPPLAYER );

    hr = IDirectPlayX_GetPlayerFlags( pDP[1], dpid[1], &dwFlags );
    checkHR( DP_OK, hr );
    checkFlags( dwFlags, DPPLAYER_LOCAL, FLAGS_DPPLAYER );

    hr = IDirectPlayX_GetPlayerFlags( pDP[0], dpid[1], &dwFlags );
    checkHR( DP_OK, hr );
    checkFlags( dwFlags, 0, FLAGS_DPPLAYER );

    hr = IDirectPlayX_GetPlayerFlags( pDP[0], dpid[2], &dwFlags );
    checkHR( DP_OK, hr );
    checkFlags( dwFlags, DPPLAYER_SPECTATOR | DPPLAYER_LOCAL, FLAGS_DPPLAYER );

    hr = IDirectPlayX_GetPlayerFlags( pDP[1], dpid[3], &dwFlags );
    checkHR( DP_OK, hr );
    checkFlags( dwFlags, DPPLAYER_SERVERPLAYER, FLAGS_DPPLAYER );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* CreateGroup
   CreateGroupInGroup */

static void test_CreateGroup(void)
{

    IDirectPlay4 *pDP;
    DPSESSIONDESC2 dpsd;
    DPID idFrom, idTo, dpid, idGroup, idGroupParent;
    DPNAME groupName;
    HRESULT hr;
    UINT i;

    LPCSTR lpData = "data";
    DWORD dwDataSize = strlen(lpData)+1;
    LPDPMSG_CREATEPLAYERORGROUP lpDataGet = HeapAlloc( GetProcessHeap(),
                                                       HEAP_ZERO_MEMORY,
                                                       1024 );
    DWORD dwDataSizeGet = 1024;
    CallbackData callbackData;


    hr= CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                          &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    ZeroMemory( &groupName, sizeof(DPNAME) );


    /* No service provider */
    hr = IDirectPlayX_CreateGroup( pDP, &idGroup, NULL, NULL, 0, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, 0, &idGroup, NULL, NULL, 0, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );



    init_TCPIP_provider( pDP, "127.0.0.1", 0 );


    /* No session */
    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, 0 );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "CreateGroup not implemented\n" );
        return;
    }

    hr = IDirectPlayX_CreateGroupInGroup( pDP, 0, &idGroup,
                                          NULL, NULL, 0, 0 );
    checkHR( DPERR_INVALIDGROUP, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, 2, &idGroup,
                                          NULL, NULL, 0, 0 );
    checkHR( DPERR_INVALIDGROUP, hr );


    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    IDirectPlayX_CreatePlayer( pDP, &dpid,
                               NULL, NULL, NULL, 0, 0 );



    /* With name */
    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   &groupName, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          &groupName, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    groupName.dwSize = sizeof(DPNAME);
    U1(groupName).lpszShortNameA = (LPSTR) lpData;


    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   &groupName, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          &groupName, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Message checking */
    for (i=0; i<6; i++)
    {
        dwDataSizeGet = 1024;
        hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpDataGet,
                                   &dwDataSizeGet );
        checkHR( DP_OK, hr );
        if ( NULL == U1(lpDataGet->dpnName).lpszShortNameA )
        {
            check( 48, dwDataSizeGet );
        }
        else
        {
            check( 48 + dwDataSize, dwDataSizeGet );
            checkStr( lpData, U1(lpDataGet->dpnName).lpszShortNameA );
        }
        check( DPID_SYSMSG, idFrom );
        checkConv( DPSYS_CREATEPLAYERORGROUP, lpDataGet->dwType, dpMsgType2str );
        check( DPPLAYERTYPE_GROUP,            lpDataGet->dwPlayerType );
        checkFlags( DPGROUP_LOCAL,            lpDataGet->dwFlags, FLAGS_DPGROUP );
    }
    check_messages( pDP, &dpid, 1, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* With data */
    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, (LPVOID) lpData, -1, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, (LPVOID) lpData, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, dwDataSize, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, (LPVOID) lpData, dwDataSize, 0 );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          NULL, (LPVOID) lpData, -1, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          NULL, (LPVOID) lpData, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          NULL, NULL, dwDataSize, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroup, &idGroup,
                                          NULL, (LPVOID)lpData, dwDataSize, 0 );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreateGroup( pDP, &idGroupParent,
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Message checking */
    for (i=0; i<5; i++)
    {
        dwDataSizeGet = 1024;
        hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpDataGet,
                                   &dwDataSizeGet );
        checkHR( DP_OK, hr );
        check( 48 + lpDataGet->dwDataSize, dwDataSizeGet );
        check( DPID_SYSMSG, idFrom );
        checkConv( DPSYS_CREATEPLAYERORGROUP, lpDataGet->dwType, dpMsgType2str );
        check( DPPLAYERTYPE_GROUP,            lpDataGet->dwPlayerType );
        checkFlags( DPGROUP_LOCAL,            lpDataGet->dwFlags, FLAGS_DPGROUP );
    }
    check_messages( pDP, &dpid, 1, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* Flags and idGroupParent */
    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, DPGROUP_HIDDEN );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0,
                                   DPGROUP_HIDDEN | DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );


    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroupParent, &idGroup,
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroupParent, &idGroup,
                                          NULL, NULL, 0, DPGROUP_HIDDEN );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroupParent, &idGroup,
                                          NULL, NULL, 0, DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroupInGroup( pDP, idGroupParent, &idGroup,
                                          NULL, NULL, 0,
                                          DPGROUP_HIDDEN |
                                          DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );


    /* Message checking */
    for (i=0; i<8; i++)
    {
        dwDataSizeGet = 1024;
        hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpDataGet,
                                   &dwDataSizeGet );
        checkHR( DP_OK, hr );
        check( 48, dwDataSizeGet );
        check( DPID_SYSMSG, idFrom );
        checkConv( DPSYS_CREATEPLAYERORGROUP, lpDataGet->dwType, dpMsgType2str );
        check( DPPLAYERTYPE_GROUP,            lpDataGet->dwPlayerType );

        if ( lpDataGet->dpIdParent != 0 )
        {
            check( idGroupParent, lpDataGet->dpIdParent );
        }

        switch (i%4)
        {
        case 0:
            checkFlags( DPGROUP_LOCAL,
                        lpDataGet->dwFlags, FLAGS_DPGROUP );
            break;
        case 1:
            checkFlags( DPGROUP_LOCAL | DPGROUP_HIDDEN,
                        lpDataGet->dwFlags, FLAGS_DPGROUP );
            break;
        case 2:
            checkFlags( DPGROUP_STAGINGAREA | DPGROUP_LOCAL,
                        lpDataGet->dwFlags, FLAGS_DPGROUP );
            break;
        case 3:
            checkFlags( DPGROUP_STAGINGAREA | DPGROUP_LOCAL | DPGROUP_HIDDEN,
                        lpDataGet->dwFlags, FLAGS_DPGROUP );
            break;
        default: break;
        }
    }
    check_messages( pDP, &dpid, 1, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* If a group is created in C/S mode, no messages are sent */

    /* - Peer 2 peer */
    IDirectPlayX_Close( pDP );

    dpsd.dwFlags = 0;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP, &dpid, NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* Messages are received */
    check_messages( pDP, &dpid, 1, &callbackData );
    checkStr( "S0,", callbackData.szTrace1 );


    /* - Client/Server */
    IDirectPlayX_Close( pDP );

    dpsd.dwFlags = DPSESSION_CLIENTSERVER;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP, &dpid,
                                    NULL, NULL, NULL, 0,
                                    DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP, &idGroup,
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* No messages */
    check_messages( pDP, &dpid, 1, &callbackData );
    checkStr( "S0,", callbackData.szTrace1 ); /* Or at least there
                                                 shouldn't be messages... */


    HeapFree( GetProcessHeap(), 0, lpDataGet );
    IDirectPlayX_Release( pDP );

}

/* GroupOwner */

static void test_GroupOwner(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2], idGroup, idOwner;
    HRESULT hr;
    UINT i;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    idGroup = 0;
    idOwner = 0;

    /* Service provider not initialized */
    hr = IDirectPlayX_GetGroupOwner( pDP[0], idGroup, &idOwner );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );
    check( 0, idOwner );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetGroupOwner not implemented\n" );
        return;
    }


    for (i=0; i<2; i++)
        init_TCPIP_provider( pDP[i], "127.0.0.1", 0 );

    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );

    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_CreatePlayer( pDP[i], &dpid[i],
                                        NULL, NULL, NULL, 0, 0 );
        checkHR( DP_OK, hr );
    }

    /* Invalid group */
    hr = IDirectPlayX_GetGroupOwner( pDP[0], idGroup, &idOwner );
    checkHR( DPERR_INVALIDGROUP, hr );

    hr = IDirectPlayX_CreateGroup( pDP[0], &idGroup, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* Fails, because we need a lobby session */
    hr = IDirectPlayX_GetGroupOwner( pDP[0], idGroup, &idOwner );
    checkHR( DPERR_UNSUPPORTED, hr );


    /* TODO:
     * - Make this work
     * - Check migration of the ownership of a group
     *   when the owner leaves
     */


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* EnumPlayers */

static BOOL CALLBACK EnumPlayers_cb( DPID dpId,
                                     DWORD dwPlayerType,
                                     LPCDPNAME lpName,
                                     DWORD dwFlags,
                                     LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    char playerIndex = dpid2char( callbackData->dpid,
                                  callbackData->dpidSize,
                                  dpId );


    /* Trace to study player ids */
    callbackData->szTrace1[ callbackData->dwCounter1 ] = playerIndex;
    callbackData->dwCounter1++;
    callbackData->szTrace1[ callbackData->dwCounter1 ] = '\0';

    /* Trace to study flags received */
    strcat( callbackData->szTrace2,
            ( dwFlags2str(dwFlags, FLAGS_DPENUMPLAYERS) +
              strlen("DPENUMPLAYERS_") ) );
    strcat( callbackData->szTrace2, ":" );


    if ( playerIndex < '5' )
    {
        check( DPPLAYERTYPE_PLAYER, dwPlayerType );
    }
    else
    {
        check( DPPLAYERTYPE_GROUP, dwPlayerType );
    }

    return TRUE;

}

static BOOL CALLBACK EnumSessions_cb_EnumPlayers( LPCDPSESSIONDESC2 lpThisSD,
                                                  LPDWORD lpdwTimeOut,
                                                  DWORD dwFlags,
                                                  LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
    {
        return FALSE;
    }

    /* guid = NULL */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( callbackData->pDP, NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    /* guid = appGuid */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( callbackData->pDP, (LPGUID) &appGuid,
                                   EnumPlayers_cb, &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( callbackData->pDP, (LPGUID) &appGuid,
                                   EnumPlayers_cb, &callbackData,
                                   DPENUMPLAYERS_SESSION );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    /* guid = guidInstance */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( callbackData->pDP,
                                   (LPGUID) &lpThisSD->guidInstance,
                                   EnumPlayers_cb, &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( callbackData->pDP,
                                   (LPGUID) &lpThisSD->guidInstance,
                                   EnumPlayers_cb, &callbackData,
                                   DPENUMPLAYERS_SESSION );
    checkHR( DPERR_GENERIC, hr ); /* Why? */
    check( 0, callbackData->dwCounter1 );

    return TRUE;

}

static void test_EnumPlayers(void)
{
    IDirectPlay4 *pDP[3];
    DPSESSIONDESC2 dpsd[3];
    DPID dpid[5+2]; /* 5 players, 2 groups */
    CallbackData callbackData;
    HRESULT hr;
    UINT i;


    for (i=0; i<3; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;

        ZeroMemory( &dpsd[i], sizeof(DPSESSIONDESC2) );
        dpsd[i].dwSize = sizeof(DPSESSIONDESC2);
    }

    dpsd[0].guidApplication = appGuid;
    dpsd[1].guidApplication = appGuid2;
    dpsd[2].guidApplication = GUID_NULL;

    callbackData.dpid = dpid;
    callbackData.dpidSize = 5+2;


    /* Uninitialized service provider */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], (LPGUID) &appGuid, NULL,
                                   &callbackData, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );
    check( 0, callbackData.dwCounter1 );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[2], "127.0.0.1", 0 );


    /* No session */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "EnumPlayers not implemented\n" );
        return;
    }

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], (LPGUID) &appGuid, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], (LPGUID) &appGuid, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_SESSION );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );


    hr = IDirectPlayX_Open( pDP[0], &dpsd[0], DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_Open( pDP[1], &dpsd[1], DPOPEN_CREATE );
    checkHR( DP_OK, hr );


    /* No players */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );


    /* Create players */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[0],
                                    NULL, NULL, NULL, 0,
                                    DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid[1],
                                    NULL, NULL, NULL, 0,
                                    0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[2],
                                    NULL, NULL, NULL, 0,
                                    0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[5],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Invalid parameters */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], (LPGUID) &appGuid, NULL,
                                   &callbackData, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_SESSION );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );


    /* Regular operation */
    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );
    checkStr( "20", callbackData.szTrace1 );
    checkStr( "ALL:SERVERPLAYER:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[1], NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "1", callbackData.szTrace1 );
    checkStr( "ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[0], (LPGUID) &appGuid, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Guid is ignored */
    checkStr( "20", callbackData.szTrace1 );
    checkStr( "ALL:SERVERPLAYER:", callbackData.szTrace2 );


    /* Enumerating from a remote session */
    /* - Session not open */
    callbackData.pDP = pDP[2];
    hr = IDirectPlayX_EnumSessions( pDP[2], &dpsd[2], 0,
                                    EnumSessions_cb_EnumPlayers,
                                    &callbackData, 0 );
    checkHR( DP_OK, hr );


    /* - Open session */
    callbackData.pDP = pDP[2];
    hr = IDirectPlayX_EnumSessions( pDP[2], &dpsd[0], 0, EnumSessions_cb_join,
                                    pDP[2], 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[2], &dpid[3],
                                    NULL, NULL, NULL, 0,
                                    DPPLAYER_SPECTATOR );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[2], &dpid[4],
                                    NULL, NULL, NULL, 0,
                                    0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[2], &dpid[6],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "4302", callbackData.szTrace1 );
    checkStr( "ALL:SPECTATOR:SERVERPLAYER:ALL:", callbackData.szTrace2 );


    /* Flag tests */

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_ALL );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "4302", callbackData.szTrace1 );
    checkStr( "ALL:SPECTATOR:SERVERPLAYER:ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_GROUP );
    checkHR( DP_OK, hr );
    check( 6, callbackData.dwCounter1 );
    checkStr( "430256", callbackData.szTrace1 );
    checkStr( "GROUP:"
              "GROUP,DPENUMPLAYERS_SPECTATOR:"
              "GROUP,DPENUMPLAYERS_SERVERPLAYER:"
              "GROUP:ALL:ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_LOCAL );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );
    checkStr( "43", callbackData.szTrace1 );
    checkStr( "LOCAL:"
              "LOCAL,DPENUMPLAYERS_SPECTATOR:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_SERVERPLAYER );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "0", callbackData.szTrace1 );
    checkStr( "SERVERPLAYER:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumPlayers( pDP[2], NULL, EnumPlayers_cb,
                                   &callbackData, DPENUMPLAYERS_SPECTATOR );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "3", callbackData.szTrace1 );
    checkStr( "SPECTATOR:", callbackData.szTrace2 );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );
    IDirectPlayX_Release( pDP[2] );

}

/* EnumGroups */

static BOOL CALLBACK EnumGroups_cb( DPID dpId,
                                    DWORD dwPlayerType,
                                    LPCDPNAME lpName,
                                    DWORD dwFlags,
                                    LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    char playerIndex = dpid2char( callbackData->dpid,
                                  callbackData->dpidSize,
                                  dpId );


    /* Trace to study player ids */
    callbackData->szTrace1[ callbackData->dwCounter1 ] = playerIndex;
    callbackData->dwCounter1++;
    callbackData->szTrace1[ callbackData->dwCounter1 ] = '\0';

    /* Trace to study flags received */
    strcat( callbackData->szTrace2,
            ( dwFlags2str(dwFlags, FLAGS_DPENUMGROUPS) +
              strlen("DPENUMGROUPS_") ) );
    strcat( callbackData->szTrace2, ":" );


    check( DPPLAYERTYPE_GROUP, dwPlayerType );

    return TRUE;
}

static BOOL CALLBACK EnumSessions_cb_EnumGroups( LPCDPSESSIONDESC2 lpThisSD,
                                                 LPDWORD lpdwTimeOut,
                                                 DWORD dwFlags,
                                                 LPVOID lpContext )
{
    lpCallbackData callbackData = lpContext;
    HRESULT hr;

    if (dwFlags & DPESC_TIMEDOUT)
    {
        return FALSE;
    }

    /* guid = NULL */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( callbackData->pDP, NULL,
                                  EnumGroups_cb, &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    /* guid = appGuid */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( callbackData->pDP, (LPGUID) &appGuid,
                                  EnumGroups_cb, &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( callbackData->pDP, (LPGUID) &appGuid,
                                  EnumGroups_cb, &callbackData,
                                  DPENUMGROUPS_SESSION );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    /* guid = guidInstance */
    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( callbackData->pDP,
                                  (LPGUID) &lpThisSD->guidInstance,
                                  EnumGroups_cb, &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData->dwCounter1 );

    callbackData->dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( callbackData->pDP,
                                  (LPGUID) &lpThisSD->guidInstance,
                                  EnumGroups_cb, &callbackData,
                                  DPENUMGROUPS_SESSION );
    checkHR( DPERR_GENERIC, hr ); /* Why? */
    check( 0, callbackData->dwCounter1 );

    return TRUE;

}

static void test_EnumGroups(void)
{
    IDirectPlay4 *pDP[3];
    DPSESSIONDESC2 dpsd[3];
    DPID dpid[5];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;


    for (i=0; i<3; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;

        ZeroMemory( &dpsd[i], sizeof(DPSESSIONDESC2) );
        dpsd[i].dwSize = sizeof(DPSESSIONDESC2);
    }

    dpsd[0].guidApplication = appGuid;
    dpsd[1].guidApplication = appGuid2;
    dpsd[2].guidApplication = GUID_NULL;

    callbackData.dpid = dpid;
    callbackData.dpidSize = 5;


    /* Uninitialized service provider */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );
    check( 0, callbackData.dwCounter1 );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[2], "127.0.0.1", 0 );


    /* No session */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    todo_wine checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "EnumGroups not implemented\n" );
        return;
    }

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], (LPGUID) &appGuid, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], (LPGUID) &appGuid, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_SESSION );
    checkHR( DPERR_NOSESSIONS, hr );
    check( 0, callbackData.dwCounter1 );


    hr = IDirectPlayX_Open( pDP[0], &dpsd[0], DPOPEN_CREATE );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_Open( pDP[1], &dpsd[1], DPOPEN_CREATE );
    checkHR( DP_OK, hr );


    /* No groups */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );


    /* Create groups */
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[0],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[0], dpid[0], &dpid[3],
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr ); /* Not a superior level group,
                             won't appear in the enumerations */
    hr = IDirectPlayX_CreateGroup( pDP[1], &dpid[1],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[2],
                                   NULL, NULL, 0, DPGROUP_HIDDEN );
    checkHR( DP_OK, hr );


    /* Invalid parameters */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], (LPGUID) &appGuid, NULL,
                                  &callbackData, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_SESSION );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );


    /* Regular operation */
    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );
    checkStr( "02", callbackData.szTrace1 );
    checkStr( "ALL:HIDDEN:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[1], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "1", callbackData.szTrace1 );
    checkStr( "ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[0], (LPGUID) &appGuid, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 ); /* Guid is ignored */
    checkStr( "02", callbackData.szTrace1 );
    checkStr( "ALL:HIDDEN:", callbackData.szTrace2 );


    /* Enumerating from a remote session */
    /* - Session not open */
    callbackData.pDP = pDP[2];
    hr = IDirectPlayX_EnumSessions( pDP[2], &dpsd[2], 0,
                                    EnumSessions_cb_EnumGroups,
                                    &callbackData, 0 );
    checkHR( DP_OK, hr );

    /* - Open session */
    callbackData.pDP = pDP[2];
    hr = IDirectPlayX_EnumSessions( pDP[2], &dpsd[0], 0, EnumSessions_cb_join,
                                    pDP[2], 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP[2], &dpid[3],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[2], &dpid[4],
                                   NULL, NULL, 0, DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );


    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "0234", callbackData.szTrace1 );
    checkStr( "ALL:HIDDEN:ALL:STAGINGAREA:", callbackData.szTrace2 );

    /* Flag tests */
    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_ALL );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "0234", callbackData.szTrace1 );
    checkStr( "ALL:HIDDEN:ALL:STAGINGAREA:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_HIDDEN );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "2", callbackData.szTrace1 );
    checkStr( "HIDDEN:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_LOCAL );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );
    checkStr( "34", callbackData.szTrace1 );
    checkStr( "LOCAL:"
              "LOCAL,DPENUMGROUPS_STAGINGAREA:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_REMOTE );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );
    checkStr( "02", callbackData.szTrace1 );
    checkStr( "REMOTE:"
              "REMOTE,DPENUMGROUPS_HIDDEN:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroups( pDP[2], NULL, EnumGroups_cb,
                                  &callbackData, DPENUMGROUPS_STAGINGAREA );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "4", callbackData.szTrace1 );
    checkStr( "STAGINGAREA:", callbackData.szTrace2 );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );
    IDirectPlayX_Release( pDP[2] );

}

static void test_EnumGroupsInGroup(void)
{
    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd[2];
    DPID dpid[6];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;

        ZeroMemory( &dpsd[i], sizeof(DPSESSIONDESC2) );
        dpsd[i].dwSize = sizeof(DPSESSIONDESC2);
    }

    dpsd[0].guidApplication = appGuid;
    dpsd[1].guidApplication = GUID_NULL;

    callbackData.dpid = dpid;
    callbackData.dpidSize = 6;


    /* Uninitialized service provider */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], 0, NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DPERR_UNINITIALIZED, hr );
    check( 0, callbackData.dwCounter1 );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );

    hr = IDirectPlayX_Open( pDP[0], &dpsd[0], DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "EnumGroupsInGroup not implemented\n" );
        return;
    }

    /* Create groups */
    /*
     * 0
     *   / 2
     * 1 | 3
     *   | 4
     *   \ 5 (shortcut)
     */
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[0],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[1],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[0], dpid[1], &dpid[2],
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[0], dpid[1], &dpid[3],
                                          NULL, NULL, 0,
                                          DPGROUP_HIDDEN );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[0], dpid[1], &dpid[4],
                                          NULL, NULL, 0,
                                          DPGROUP_STAGINGAREA );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[0], &dpid[5],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_AddGroupToGroup( pDP[0], dpid[1], dpid[5] );
    checkHR( DP_OK, hr );


    /* Invalid parameters */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], 0, NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DPERR_INVALIDGROUP, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], 10, NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DPERR_INVALIDGROUP, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], (LPGUID) &appGuid,
                                         NULL, &callbackData, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_SESSION );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( 0, callbackData.dwCounter1 );


    /* Regular operation */
    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[0], NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "SHORTCUT:STAGINGAREA:HIDDEN:ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], (LPGUID) &appGuid,
                                         EnumGroups_cb, &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 ); /* Guid is ignored */
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "SHORTCUT:STAGINGAREA:HIDDEN:ALL:", callbackData.szTrace2 );


    /* Enumerating from a remote session */
    /* - Session not open */
    callbackData.pDP = pDP[1];
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd[1], 0,
                                    EnumSessions_cb_EnumGroups,
                                    &callbackData, 0 );
    checkHR( DP_OK, hr );

    /* - Open session */
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd[0], 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );


    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[1], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, 0 );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "SHORTCUT:STAGINGAREA:HIDDEN:ALL:", callbackData.szTrace2 );

    /* Flag tests */
    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_ALL );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "SHORTCUT:STAGINGAREA:HIDDEN:ALL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_HIDDEN );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "3", callbackData.szTrace1 );
    checkStr( "HIDDEN:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_LOCAL );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "LOCAL,DPENUMGROUPS_SHORTCUT:"
              "LOCAL,DPENUMGROUPS_STAGINGAREA:"
              "LOCAL,DPENUMGROUPS_HIDDEN:LOCAL:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_REMOTE );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[1], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_LOCAL );
    checkHR( DP_OK, hr );
    check( 0, callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[1], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_REMOTE );
    checkHR( DP_OK, hr );
    check( 4, callbackData.dwCounter1 );
    checkStr( "5432", callbackData.szTrace1 );
    checkStr( "REMOTE,DPENUMGROUPS_SHORTCUT:"
              "REMOTE,DPENUMGROUPS_STAGINGAREA:"
              "REMOTE,DPENUMGROUPS_HIDDEN:REMOTE:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData, DPENUMGROUPS_SHORTCUT );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "5", callbackData.szTrace1 );
    checkStr( "SHORTCUT:", callbackData.szTrace2 );

    callbackData.dwCounter1 = 0;
    callbackData.szTrace2[0] = 0;
    hr = IDirectPlayX_EnumGroupsInGroup( pDP[0], dpid[1], NULL, EnumGroups_cb,
                                         &callbackData,
                                         DPENUMGROUPS_STAGINGAREA );
    checkHR( DP_OK, hr );
    check( 1, callbackData.dwCounter1 );
    checkStr( "4", callbackData.szTrace1 );
    checkStr( "STAGINGAREA:", callbackData.szTrace2 );


    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

static void test_groups_p2p(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID idPlayer[6], idGroup[3];
    HRESULT hr;
    UINT i;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );

    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    todo_wine checkHR( DP_OK, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "dplay not implemented enough for this test yet\n" );
        return;
    }


    /* Create players */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[0],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[1],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[2],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[3],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[4],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[5],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP[0], &idGroup[0],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[1], &idGroup[2],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[1], idGroup[2], &idGroup[1],
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Purge queues */
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "S0," "S1,S0,"
              "S2,S1,S0," "S2,S1,S0,"
              "S2,S1,S0," "S2,S1,S0,"
              "S2,S1,S0," "S2,S1,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "S3," "S4,S3,"
              "S5,S4,S3," "S5,S4,S3,"
              "S5,S4,S3,", callbackData.szTrace1 );


    /*
     * Player 0   |                  |
     * Player 1   | Group 0          | pDP 0
     * Player 2   |                  |
     * Player 3  | Group 1 )          |
     * Player 4  |         | Group 2  | pDP 1
     * Player 5            |          |
     */

    /* Build groups */
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[0] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[1] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[2] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[1], idPlayer[3] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[1], idPlayer[4] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[2], idPlayer[4] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[2], idPlayer[5] );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_AddGroupToGroup( pDP[1], idGroup[2], idGroup[1] );
    checkHR( DP_OK, hr );

    /* Purge queues */
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "S2,S1,S0," "S2,S1,S0," "S2,S1,S0,"
              "S2,S1,S0," "S2,S1,S0," "S2,S1,S0,"
              "S2,S1,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "S5,S4,S3," "S5,S4,S3," "S5,S4,S3,"
              "S5,S4,S3," "S5,S4,S3," "S5,S4,S3,"
              "S5,S4,S3,", callbackData.szTrace1 );


    /* Sending broadcast messages, and checking who receives them */

    dwDataSize = 4;
    /* 0 -> * */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "02,01,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "05,04,03,", callbackData.szTrace1 );

    /* 0 -> g0 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "02,01,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 0 -> g1 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "04,03,", callbackData.szTrace1 );
    /* 0 -> g2 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "05,04,", callbackData.szTrace1 );

    /* 3 -> * */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "32,31,30,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "35,34,", callbackData.szTrace1 );
    /* 3 -> g0 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "32,31,30,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 3 -> g1 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "34,", callbackData.szTrace1 );
    /* 3 -> g2 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "35,34,", callbackData.szTrace1 );

    /* 5 -> * */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "52,51,50,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,53,", callbackData.szTrace1 );
    /* 5 -> g0 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "52,51,50,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 5 -> g1 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,53,", callbackData.szTrace1 );
    /* 5 -> g2 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,", callbackData.szTrace1 );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

static void test_groups_cs(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID idPlayer[6], idGroup[3];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 );


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );

    dpsd.dwFlags = DPSESSION_CLIENTSERVER;
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );
    dpsd.dwFlags = 0;
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    todo_wine checkHR( DP_OK, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "dplay not implemented enough for this test yet\n" );
        return;
    }


    /* Create players */
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[0],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[0],
                                    NULL, NULL, NULL, 0,
                                    DPPLAYER_SERVERPLAYER );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[0], &idPlayer[1],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DPERR_ACCESSDENIED, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[1],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[2],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[3],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[4],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreatePlayer( pDP[1], &idPlayer[5],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreateGroup( pDP[0], &idGroup[0],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroup( pDP[1], &idGroup[2],
                                   NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_CreateGroupInGroup( pDP[1], idGroup[2], &idGroup[1],
                                          NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Purge queues */
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "S1," "S2,S1," "S3,S2,S1," "S4,S3,S2,S1,"
              "S5,S4,S3,S2,S1," "S5,S4,S3,S2,S1,", callbackData.szTrace1 );

    /*
     * Player 0   |                  | pDP 0
     * Player 1   | Group 0           |
     * Player 2   |                   |
     * Player 3  | Group 1 )          |
     * Player 4  |         | Group 2  | pDP 1
     * Player 5            |          |
     */

    /* Build groups */
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[0] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[1] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[0], idGroup[0], idPlayer[2] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[1], idPlayer[3] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[1], idPlayer[4] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[2], idPlayer[4] );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_AddPlayerToGroup( pDP[1], idGroup[2], idPlayer[5] );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_AddGroupToGroup( pDP[1], idGroup[2], idGroup[1] );
    checkHR( DP_OK, hr );

    /* Purge queues */
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "S0,S0,S0,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "S5," "S4,S3,S2,S1," "S5,S4,S3,S2,S1,"
              "S5,S4,S3,S2,S1," "S5,S4,S3,S2,S1,", callbackData.szTrace1 );


    /* Sending broadcast messages, and checking who receives them */
    dwDataSize = 4;
    /* 0 -> * */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "05,04,03,02,01,", callbackData.szTrace1 );

    /* 0 -> g0 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "02,01,", callbackData.szTrace1 );
    /* 0 -> g1 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 0 -> g2 */
    hr = IDirectPlayX_Send( pDP[0], idPlayer[0], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );

    /* 3 -> * */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "30,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "35,34,32,31,", callbackData.szTrace1 );
    /* 3 -> g0 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 3 -> g1 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "34,", callbackData.szTrace1 );
    /* 3 -> g2 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[3], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "35,34,", callbackData.szTrace1 );

    /* 5 -> * */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], DPID_ALLPLAYERS, 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "50,", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,53,52,51,", callbackData.szTrace1 );
    /* 5 -> g0 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[0], 0,
                            lpData, dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    /* 5 -> g1 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[1], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,53,", callbackData.szTrace1 );
    /* 5 -> g2 */
    hr = IDirectPlayX_Send( pDP[1], idPlayer[5], idGroup[2], 0,
                            lpData, dwDataSize );
    checkHR( DP_OK, hr );
    check_messages( pDP[0], idPlayer, 6, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], idPlayer, 6, &callbackData );
    checkStr( "54,", callbackData.szTrace1 );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Send */

static void test_Send(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4], idFrom, idTo;
    CallbackData callbackData;
    HRESULT hr;
    LPCSTR message = "message";
    DWORD messageSize = strlen(message) + 1;
    DWORD dwDataSize = 1024;
    LPDPMSG_GENERIC lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    LPDPMSG_SECUREMESSAGE lpDataSecure;
    UINT i;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );


    /* Uninitialized service provider */
    hr = IDirectPlayX_Send( pDP[0], 0, 0, 0,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_UNINITIALIZED, hr );


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );

    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );


    /* Incorrect players */
    hr = IDirectPlayX_Send( pDP[0], 0, 1, 2,
                            (LPVOID) message, messageSize );
    todo_wine checkHR( DPERR_INVALIDPLAYER, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "Send not implemented\n" );
        return;
    }


    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[1], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[2], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[1], &dpid[3], NULL, NULL, NULL, 0, 0 );

    /* Purge player creation messages */
    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "S0," "S1,S0," "S2,S1,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* Message to self: no error, but no message is sent */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[0], 0,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );

    /* Send a message from a remote player */
    hr = IDirectPlayX_Send( pDP[1], dpid[0], dpid[1], 0,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_ACCESSDENIED, hr );
    hr = IDirectPlayX_Send( pDP[1], dpid[0], dpid[3], 0,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_ACCESSDENIED, hr );

    /* Null message */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1], 0,
                            NULL, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1], 0,
                            (LPVOID) message, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );


    /* Checking no message was sent */
    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* Regular parameters */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            0,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_Receive( pDP[0], &dpid[0], &dpid[1],
                               DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( message, (LPSTR) lpData );
    check( strlen(message)+1, dwDataSize );

    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* Message to a remote player */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[3], 0,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_Receive( pDP[0], &dpid[0], &dpid[3],
                               DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                               lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );
    hr = IDirectPlayX_Receive( pDP[1], &dpid[0], &dpid[3],
                               DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( message, (LPSTR) lpData );
    check( strlen(message)+1, dwDataSize );

    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* Broadcast */

    hr = IDirectPlayX_Send( pDP[0], dpid[0], DPID_ALLPLAYERS, 0,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );

    for (i=1; i<3; i++)
    {
        hr = IDirectPlayX_Receive( pDP[0], &dpid[0], &dpid[i],
                                   DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                                   lpData, &dwDataSize );
        checkHR( DP_OK, hr );
        checkStr( message, (LPSTR) lpData );
    }
    hr = IDirectPlayX_Receive( pDP[1], &dpid[0], &dpid[3],
                               DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( message, (LPSTR) lpData );

    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    hr = IDirectPlayX_Send( pDP[0], DPID_ALLPLAYERS, dpid[1],
                            0,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPLAYER, hr );
    hr = IDirectPlayX_Send( pDP[0], DPID_ALLPLAYERS, DPID_ALLPLAYERS,
                            0,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPLAYER, hr );


    /* Flags */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_Receive( pDP[0], &dpid[0], &dpid[1],
                               DPRECEIVE_FROMPLAYER | DPRECEIVE_TOPLAYER,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( message, (LPSTR)lpData );

    /* - Inorrect flags */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_SIGNED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED | DPSEND_SIGNED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* - Correct flags, but session is not secure */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_SIGNED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            ( DPSEND_ENCRYPTED |
                              DPSEND_SIGNED |
                              DPSEND_GUARANTEED ),
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* - Correct flags, secure session incorrectly opened (without flags) */
    hr = IDirectPlayX_Close( pDP[0] );
    checkHR( DP_OK, hr );

    dpsd.dwFlags = 0;
    hr = IDirectPlayX_SecureOpen( pDP[0], &dpsd, DPOPEN_CREATE, NULL, NULL );
    checkHR( DP_OK, hr );
    for (i=0; i<2; i++)
        IDirectPlayX_CreatePlayer( pDP[0], &dpid[i], NULL, NULL, NULL, 0, 0 );

    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_SIGNED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            ( DPSEND_ENCRYPTED |
                              DPSEND_SIGNED |
                              DPSEND_GUARANTEED ),
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* - Correct flags, secure session */
    hr = IDirectPlayX_Close( pDP[0] );
    checkHR( DP_OK, hr );

    dpsd.dwFlags = DPSESSION_SECURESERVER;
    hr = IDirectPlayX_SecureOpen( pDP[0], &dpsd, DPOPEN_CREATE, NULL, NULL );
    checkHR( DP_OK, hr );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[1], NULL, NULL, NULL, 0, 0 );

    /* Purge */
    check_messages( pDP[0], dpid, 6, &callbackData );
    checkStr( "S0,", callbackData.szTrace1 );


    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_SIGNED | DPSEND_GUARANTEED,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            ( DPSEND_ENCRYPTED |
                              DPSEND_SIGNED |
                              DPSEND_GUARANTEED ),
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );


    for (i=0; i<3; i++)
    {
        dwDataSize = 1024;
        hr = IDirectPlayX_Receive( pDP[0], &idFrom, &idTo, 0, lpData,
                                   &dwDataSize );

        lpDataSecure = (LPDPMSG_SECUREMESSAGE) lpData;

        checkHR( DP_OK, hr );
        checkConv( DPSYS_SECUREMESSAGE,   lpData->dwType, dpMsgType2str );
        check( DPID_SYSMSG,               idFrom );
        check( dpid[1],                   idTo );
        check( dpid[0],                   lpDataSecure->dpIdFrom );
        checkStr( message,        (LPSTR) lpDataSecure->lpData );
        check( strlen(message)+1,         lpDataSecure->dwDataSize );

        switch(i)
        {
        case 0:
            checkFlags( DPSEND_ENCRYPTED,
                        lpDataSecure->dwFlags,
                        FLAGS_DPSEND );
            break;
        case 1:
            checkFlags( DPSEND_SIGNED,
                        lpDataSecure->dwFlags,
                        FLAGS_DPSEND );
            break;
        case 2:
            checkFlags( DPSEND_SIGNED | DPSEND_ENCRYPTED,
                        lpDataSecure->dwFlags,
                        FLAGS_DPSEND );
            break;
        default: break;
        }
    }
    check_messages( pDP[0], dpid, 4, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    /* - Even in a secure session, incorrect flags still not working */
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_SIGNED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Send( pDP[0], dpid[0], dpid[1],
                            DPSEND_ENCRYPTED | DPSEND_SIGNED,
                            (LPVOID) message, messageSize );
    checkHR( DPERR_INVALIDPARAMS, hr );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Receive */

static void test_Receive(void)
{

    IDirectPlay4 *pDP;
    DPSESSIONDESC2 dpsd;
    DPID dpid[4], idFrom, idTo;
    HRESULT hr;
    LPCSTR message = "message";
    DWORD messageSize = strlen(message) + 1;
    DWORD dwDataSize = 1024;
    LPDPMSG_GENERIC lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        dwDataSize );
    LPDPMSG_CREATEPLAYERORGROUP lpDataCreate;
    LPDPMSG_DESTROYPLAYERORGROUP lpDataDestroy;

    DWORD dwCount;
    UINT i;


    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr)) return;

    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;

    init_TCPIP_provider( pDP, "127.0.0.1", 0 );

    IDirectPlayX_Open( pDP, &dpsd, DPOPEN_CREATE );


    /* Invalid parameters */
    hr = IDirectPlayX_Receive( pDP, NULL, &idTo, 0,
                               lpData, &dwDataSize );
    todo_wine checkHR( DPERR_INVALIDPARAMS, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "Receive not implemented\n" );
        return;
    }

    hr = IDirectPlayX_Receive( pDP, &idFrom, NULL, 0,
                               lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                               lpData, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    dwDataSize = -1;
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                               lpData, &dwDataSize );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* No messages yet */
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                               NULL, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );
    dwDataSize = 0;
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                               lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );


    IDirectPlayX_CreatePlayer( pDP, &dpid[0], NULL, 0, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP, &dpid[1], NULL, 0, NULL, 0,
                               DPPLAYER_SPECTATOR );
    IDirectPlayX_CreatePlayer( pDP, &dpid[2], NULL, 0, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP, &dpid[3], NULL, 0, NULL, 0, 0 );


    /* 0, 1, 2, 3 */
    /* 3, 2, 1, 0 */
    for (i=0; i<4; i++)
    {
        IDirectPlayX_GetMessageCount( pDP, dpid[i], &dwCount );
        check( 3-i, dwCount );
    }


    IDirectPlayX_DestroyPlayer( pDP, dpid[3] );
    IDirectPlayX_DestroyPlayer( pDP, dpid[1] );


    /* 0, 1, 2, 3 */
    /* 5, 5, 3, 3 */
    IDirectPlayX_GetMessageCount( pDP, dpid[0], &dwCount );
    check( 5, dwCount );
    IDirectPlayX_GetMessageCount( pDP, dpid[1], &dwCount );
    check( 5, dwCount );
    IDirectPlayX_GetMessageCount( pDP, dpid[2], &dwCount );
    check( 3, dwCount );
    IDirectPlayX_GetMessageCount( pDP, dpid[3], &dwCount );
    check( 3, dwCount );


    /* Buffer too small */
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idFrom, 0,
                               NULL, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 48, dwDataSize );
    dwDataSize = 0;
    hr = IDirectPlayX_Receive( pDP, &idTo, &idFrom, 0,
                               lpData, &dwDataSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );
    check( 48, dwDataSize );


    /* Checking the order or reception */
    for (i=0; i<11; i++)
    {
        dwDataSize = 1024;
        hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0,
                                   lpData, &dwDataSize );

        checkHR( DP_OK, hr );
        check( DPID_SYSMSG, idFrom );

        if (i<6)  /* Player creation */
        {
            checkConv( DPSYS_CREATEPLAYERORGROUP, lpData->dwType, dpMsgType2str );
            check( 48, dwDataSize );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( DPPLAYERTYPE_PLAYER,   lpDataCreate->dwPlayerType );
            checkLP( NULL,                lpDataCreate->lpData );
            check( 0,                     lpDataCreate->dwDataSize );
            checkLP( NULL,                U1(lpDataCreate->dpnName).lpszShortNameA );
            check( 0,                     lpDataCreate->dpIdParent );
        }
        else  /* Player destruction */
        {
            checkConv( DPSYS_DESTROYPLAYERORGROUP, lpData->dwType,
                       dpMsgType2str );
            check( 52, dwDataSize );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( DPPLAYERTYPE_PLAYER,   lpDataDestroy->dwPlayerType );
            checkLP( NULL,                lpDataDestroy->lpLocalData );
            check( 0,                     lpDataDestroy->dwLocalDataSize );
            checkLP( NULL,                lpDataDestroy->lpRemoteData );
            check( 0,                     lpDataDestroy->dwRemoteDataSize );
            checkLP( NULL,                U1(lpDataDestroy->dpnName).lpszShortNameA );
            check( 0,                     lpDataDestroy->dpIdParent );
        }

        switch(i)
        {
            /* 1 -> 0 */
        case 0:
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[0], idTo );
            check( dpid[1],              lpDataCreate->dpId );
            check( 1,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL|DPPLAYER_SPECTATOR, lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER|FLAGS_DPGROUP );
            break;

            /* 2 -> 1,0 */
        case 1:
            check( dpid[1], idTo );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[2],              lpDataCreate->dpId );
            check( 2,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL,  lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 2:
            check( dpid[0], idTo );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[2],              lpDataCreate->dpId );
            check( 2,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL,  lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;

            /* 3 -> 2,1,0 */
        case 3:
            check( dpid[2], idTo );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataCreate->dpId );
            check( 3,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL,  lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 4:
            check( dpid[1], idTo );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataCreate->dpId );
            check( 3,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL,  lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 5:
            check( dpid[0], idTo );
            lpDataCreate = (LPDPMSG_CREATEPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataCreate->dpId );
            check( 3,                    lpDataCreate->dwCurrentPlayers );
            checkFlags( DPPLAYER_LOCAL,  lpDataCreate->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;

            /* 3 -> 2,1,0 */
        case 6:
            check( dpid[2], idTo );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataDestroy->dpId );
            checkFlags( DPPLAYER_LOCAL,  lpDataDestroy->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 7:
            check( dpid[1], idTo );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataDestroy->dpId );
            checkFlags( DPPLAYER_LOCAL,  lpDataDestroy->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 8:
            check( dpid[0], idTo );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( dpid[3],              lpDataDestroy->dpId );
            checkFlags( DPPLAYER_LOCAL,  lpDataDestroy->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;

            /* 1 -> 2,0 */
        case 9:
            check( dpid[2], idTo );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( dpid[1],                 lpDataDestroy->dpId );
            checkFlags( DPPLAYER_LOCAL |
                        DPPLAYER_SPECTATOR, lpDataDestroy->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 10:
            check( dpid[0], idTo );
            lpDataDestroy = (LPDPMSG_DESTROYPLAYERORGROUP) lpData;
            check( dpid[1],                 lpDataDestroy->dpId );
            checkFlags( DPPLAYER_LOCAL |
                        DPPLAYER_SPECTATOR, lpDataDestroy->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;

        default:
            trace( "%s\n", dpMsgType2str(lpData->dwType) );
            break;
        }
    }

    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );


    /* New data message */
    hr = IDirectPlayX_Send( pDP, dpid[0], dpid[2], 0,
                            (LPVOID) message, messageSize );
    checkHR( DP_OK, hr );


    /* Ensuring DPRECEIVE_PEEK doesn't remove the messages from the queue */
    for (i=0; i<10; i++)
    {
        hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, DPRECEIVE_PEEK,
                                   lpData, &dwDataSize );
        checkHR( DP_OK, hr );
        checkStr( message, (LPSTR) lpData );
    }

    /* Removing the message from the queue */
    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( idFrom, dpid[0] );
    check( idTo, dpid[2] );
    checkStr( message, (LPSTR) lpData );

    hr = IDirectPlayX_Receive( pDP, &idFrom, &idTo, 0, lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP );

}

/* GetMessageCount */

static void test_GetMessageCount(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4];
    HRESULT hr;
    UINT i;
    DWORD dwCount;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );
    CallbackData callbackData;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0,  &dwCount );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );
    check( -1, dwCount );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetMessageCount not implemented\n" );
        return;
    }


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[1], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[1], &dpid[3], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[2], NULL, NULL, NULL, 0, 0 );


    /* Incorrect parameters */
    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[0], NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    check( -1, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], -1, &dwCount );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( -1, dwCount );


    /* Correct parameters */
    /* Player creation messages */
    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 5, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 1, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[0], &dwCount );
    checkHR( DP_OK, hr );
    check( 3, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[1], &dwCount );
    checkHR( DP_OK, hr );
    check( 2, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[3], &dwCount );
    checkHR( DP_OK, hr );
    /* Remote player: doesn't throw error but result is 0 and not 1 */
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], dpid[3], &dwCount );
    checkHR( DP_OK, hr );
    check( 1, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[1], &dwCount );
    checkHR( DP_OK, hr );
    check( 2, dwCount );


    /* Purge queues */
    check_messages( pDP[0], dpid, 6, &callbackData );
    checkStr( "S0,S1,S0,S1,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 6, &callbackData );
    checkStr( "S3,", callbackData.szTrace1 );


    /* Ensure queues is purged */
    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );


    /* Send data messages */
    for (i=0; i<5; i++)
        IDirectPlayX_Send( pDP[0], dpid[0], dpid[1], 0, lpData, dwDataSize );
    for (i=0; i<6; i++)
        IDirectPlayX_Send( pDP[0], dpid[1], dpid[2], 0, lpData, dwDataSize );
    for (i=0; i<7; i++)
        IDirectPlayX_Send( pDP[0], dpid[2], dpid[3], 0, lpData, dwDataSize );


    /* Check all messages are in the queues */
    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 11, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 7, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[0], &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[1], &dwCount );
    checkHR( DP_OK, hr );
    check( 5, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[2], &dwCount );
    checkHR( DP_OK, hr );
    check( 6, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], dpid[3], &dwCount );
    checkHR( DP_OK, hr );
    check( 7, dwCount );


    /* Purge queues again */
    check_messages( pDP[0], dpid, 6, &callbackData );
    checkStr( "01,01,01,01,01,"
              "12,12,12,12,12,12,", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 6, &callbackData );
    checkStr( "23,23,23,23,23,23,23,", callbackData.szTrace1 );


    /* Check queues are purged */
    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], 0, &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[0], &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[1], &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[0], dpid[2], &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );

    dwCount = -1;
    hr = IDirectPlayX_GetMessageCount( pDP[1], dpid[3], &dwCount );
    checkHR( DP_OK, hr );
    check( 0, dwCount );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetMessageQueue */

static void test_GetMessageQueue(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;
    DWORD dwNumMsgs, dwNumBytes;

    DWORD dwDataSize = 1024;
    LPVOID lpData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );


    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, 0, 0,
                                       &dwNumMsgs, &dwNumBytes );
    todo_wine checkHR( DPERR_UNINITIALIZED, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    if ( hr == DP_OK )
    {
        todo_wine win_skip( "GetMessageQueue not implemented\n" );
        return;
    }


    init_TCPIP_provider( pDP[0], "127.0.0.1", 0 );
    init_TCPIP_provider( pDP[1], "127.0.0.1", 0 );


    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, 0, 0,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );


    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                               pDP[1], 0 );

    IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[1], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[1], &dpid[3], NULL, NULL, NULL, 0, 0 );
    IDirectPlayX_CreatePlayer( pDP[0], &dpid[2], NULL, NULL, NULL, 0, 0 );



    /* Incorrect parameters */
    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], -1, dpid[1],
                                       0,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], -1,
                                       0,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDPLAYER, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[0],
                                       -1,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       ( DPMESSAGEQUEUE_SEND |
                                         DPMESSAGEQUEUE_RECEIVE ),
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDFLAGS, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    /* - Remote players */
if(0)
{
    /* Crash under Win7 */
    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, dpid[3],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDPLAYER, hr ); /* Player 3 is remote */
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );
}

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[3], 0,
                                       DPMESSAGEQUEUE_SEND,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DPERR_INVALIDPLAYER, hr ); /* Player 3 is remote */
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    /* - Remote players, this time in the right place */
    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, dpid[3],
                                       DPMESSAGEQUEUE_SEND,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[3], 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );


    /* Correct parameters */
    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 2, dwNumMsgs );
    check( 96, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 5, dwNumMsgs );
    check( 240, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       NULL, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( -1, dwNumMsgs );
    check( 0, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, NULL );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( -1, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       NULL, NULL );
    checkHR( DP_OK, hr );
    check( -1, dwNumMsgs );
    check( -1, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );


    /* Purge messages */
    check_messages( pDP[0], dpid, 6, &callbackData );
    checkStr( "S0,S1,S0,S1,S0,", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 6, &callbackData );
    checkStr( "S3,", callbackData.szTrace1 );

    /* Check queues are empty */
    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );


    /* Sending 4 data messages from 0 to 1 */
    /*         3               from 0 to 3 */
    /*         2               from 1 to 3 */
    for (i=0; i<4; i++)
        IDirectPlayX_Send( pDP[0], dpid[0], dpid[1], 0, lpData, dwDataSize );
    for (i=0; i<3; i++)
        IDirectPlayX_Send( pDP[0], dpid[0], dpid[3], 0, lpData, dwDataSize );
    for (i=0; i<2; i++)
        IDirectPlayX_Send( pDP[0], dpid[1], dpid[3], 0, lpData, dwDataSize );


    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 4, dwNumMsgs );
    check( 4*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[1], dpid[0], dpid[3],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 3, dwNumMsgs );
    check( 3*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[1], dpid[1], dpid[3],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 2, dwNumMsgs );
    check( 2*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 4, dwNumMsgs );
    check( 4*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[1], dpid[0], 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 3, dwNumMsgs );
    check( 3*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[1], 0, dpid[3],
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 5, dwNumMsgs );
    check( 5*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], 0, 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 4, dwNumMsgs );
    check( 4*dwDataSize, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[1], 0, 0,
                                       DPMESSAGEQUEUE_RECEIVE,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 5, dwNumMsgs );
    check( 5*dwDataSize, dwNumBytes );


    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       DPMESSAGEQUEUE_SEND,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );

    dwNumMsgs = dwNumBytes = -1;
    hr = IDirectPlayX_GetMessageQueue( pDP[0], dpid[0], dpid[1],
                                       0,
                                       &dwNumMsgs, &dwNumBytes );
    checkHR( DP_OK, hr );
    check( 0, dwNumMsgs );
    check( 0, dwNumBytes );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Remote data replication */

static void test_remote_data_replication(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2], idFrom, idTo;
    CallbackData callbackData;
    HRESULT hr;
    UINT i, j;
    DWORD dwFlags, dwDataSize = 1024;
    DWORD dwCount;

    LPDPMSG_SETPLAYERORGROUPDATA lpData = HeapAlloc( GetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     dwDataSize );

    LPCSTR lpDataLocal[] = { "local_0", "local_1" };
    LPCSTR lpDataRemote[] = { "remote_0", "remote_1" };
    LPCSTR lpDataFake = "ugly_fake_data";
    LPSTR lpDataGet = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 32 );
    DWORD dwDataSizeLocal = strlen(lpDataLocal[0])+1,
        dwDataSizeRemote = strlen(lpDataRemote[0])+1,
        dwDataSizeFake = strlen(lpDataFake)+1,
        dwDataSizeGet;


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
        init_TCPIP_provider( pDP[i], "127.0.0.1", 0 );
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;

    /* Host */
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    if ( hr == DPERR_UNINITIALIZED )
    {
        todo_wine win_skip( "dplay not implemented enough for this test yet\n" );
        return;
    }

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[0],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* Peer */
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid[1],
                                    NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* Check players */
    for (i=0; i<2; i++)
    {
        /* Local (0,0) (1,1) */
        IDirectPlayX_GetPlayerFlags( pDP[i], dpid[i], &dwFlags );
        checkFlags( DPPLAYER_LOCAL, dwFlags, FLAGS_DPPLAYER );
        /* Remote (0,1) (1,0) */
        IDirectPlayX_GetPlayerFlags( pDP[i], dpid[!i], &dwFlags );
        checkFlags( 0, dwFlags, FLAGS_DPPLAYER );
    }

    /* Set data for a local player */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_SetPlayerData( pDP[i], dpid[i],
                                         (LPVOID) lpDataLocal[i],
                                         dwDataSizeLocal,
                                         DPSET_LOCAL );
        checkHR( DP_OK, hr );
        hr = IDirectPlayX_SetPlayerData( pDP[i], dpid[i],
                                         (LPVOID) lpDataRemote[i],
                                         dwDataSizeRemote,
                                         DPSET_REMOTE );
        checkHR( DP_OK, hr );
    }

    /* Retrieve data locally (0->0, 1->1) */
    for (i=0; i<2; i++)
    {
        dwDataSizeGet = dwDataSizeFake;
        strcpy( lpDataGet, lpDataFake );
        hr = IDirectPlayX_GetPlayerData( pDP[i], dpid[i],
                                         lpDataGet, &dwDataSizeGet,
                                         DPGET_LOCAL );
        checkHR( DP_OK, hr );
        check( dwDataSizeLocal, dwDataSizeGet );
        checkStr( lpDataLocal[i], lpDataGet );

        dwDataSizeGet = dwDataSizeFake;
        strcpy( lpDataGet, lpDataFake );
        hr = IDirectPlayX_GetPlayerData( pDP[i], dpid[i],
                                         lpDataGet, &dwDataSizeGet,
                                         DPGET_REMOTE );
        checkHR( DP_OK, hr );
        check( dwDataSizeRemote, dwDataSizeGet );
        checkStr( lpDataRemote[i], lpDataGet );
    }


    /* Set data for a remote player */
    /* This should fail with DPERR_ACCESSDENIED,
       but for some reason it doesn't */
    for (i=0; i<2; i++)
    {
        IDirectPlayX_SetPlayerData( pDP[i], dpid[!i],
                                    (LPVOID) lpDataLocal[!i],
                                    dwDataSizeLocal,
                                    DPSET_LOCAL );
        checkHR( DP_OK, hr );
        IDirectPlayX_SetPlayerData( pDP[i], dpid[!i],
                                    (LPVOID) lpDataRemote[!i],
                                    dwDataSizeRemote,
                                    DPSET_REMOTE );
        checkHR( DP_OK, hr );
    }

    /* Retrieve crossed data (0->1, 1->0) */
    for (i=0; i<2; i++)
    {
        dwDataSizeGet = dwDataSizeFake;
        strcpy( lpDataGet, lpDataFake );
        hr = IDirectPlayX_GetPlayerData( pDP[i], dpid[!i],
                                         lpDataGet, &dwDataSizeGet,
                                         DPGET_LOCAL );
        checkHR( DP_OK, hr );
        check( dwDataSizeLocal, dwDataSizeGet );
        checkStr( lpDataLocal[!i], lpDataGet );

        dwDataSizeGet = dwDataSizeFake;
        strcpy( lpDataGet, lpDataFake );
        hr = IDirectPlayX_GetPlayerData( pDP[i], dpid[!i],
                                         lpDataGet, &dwDataSizeGet,
                                         DPGET_REMOTE );
        checkHR( DP_OK, hr );
        check( dwDataSizeRemote, dwDataSizeGet );
        checkStr( lpDataRemote[!i], lpDataGet );
    }


    /* Purge "new player" messages from queue */
    hr = IDirectPlayX_Receive( pDP[0], &idFrom, &idTo, 0, lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkConv( DPSYS_CREATEPLAYERORGROUP, lpData->dwType, dpMsgType2str );

    /* Check number of messages in queue */
    for (i=0; i<2; i++)
    {
        IDirectPlayX_GetMessageCount( pDP[i], dpid[i], &dwCount );
        check( 2, dwCount );
        IDirectPlayX_GetMessageCount( pDP[i], dpid[!i], &dwCount );
        check( 0, dwCount );
    }

    /* Checking system messages */
    for (i=0; i<2; i++)
    {
        for (j=0; j<2; j++)
        {
            hr = IDirectPlayX_Receive( pDP[i], &idFrom, &idTo, 0, lpData,
                                       &dwDataSize );
            checkHR( DP_OK, hr );
            check( 29, dwDataSize );
            check( DPID_SYSMSG, idFrom );
            check( dpid[i], idTo );
            checkConv( DPSYS_SETPLAYERORGROUPDATA, lpData->dwType,
                       dpMsgType2str );
            check( DPPLAYERTYPE_PLAYER,            lpData->dwPlayerType );
            check( dpid[j],                        lpData->dpId );
            checkStr( lpDataRemote[j],     (LPSTR) lpData->lpData );
            check( dwDataSizeRemote,               lpData->dwDataSize );
            dwDataSize = 1024;
        }
        hr = IDirectPlayX_Receive( pDP[i], &idFrom, &idTo, 0,
                                   lpData, &dwDataSize );
        checkHR( DPERR_NOMESSAGES, hr );
    }


    /* Changing remote data */
    hr = IDirectPlayX_SetPlayerData( pDP[0], dpid[0],
                                     (LPVOID) lpDataRemote[0], dwDataSizeRemote,
                                     DPSET_REMOTE );
    checkHR( DP_OK, hr );

    /* Checking system messages (j=0) */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_Receive( pDP[i], &idFrom, &idTo, 0,
                                   lpData, &dwDataSize );
        checkHR( DP_OK, hr );
        check( 29, dwDataSize );
        check( DPID_SYSMSG, idFrom );
        check( dpid[i], idTo );
        checkConv( DPSYS_SETPLAYERORGROUPDATA, lpData->dwType, dpMsgType2str );
        check( DPPLAYERTYPE_PLAYER,            lpData->dwPlayerType );
        check( dpid[0],                        lpData->dpId );
        checkStr( lpDataRemote[0],     (LPSTR) lpData->lpData );
        check( dwDataSizeRemote,               lpData->dwDataSize );
        dwDataSize = 1024;
    }

    /* Queue is empty */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "", callbackData.szTrace1 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "", callbackData.szTrace1 );


    HeapFree( GetProcessHeap(), 0, lpDataGet );
    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Host migration */

static void test_host_migration(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[2], idFrom, idTo;
    HRESULT hr;
    UINT i;
    DWORD dwCount;

    DWORD dwDataSize = 1024;
    LPDPMSG_DESTROYPLAYERORGROUP lpData = HeapAlloc( GetProcessHeap(),
                                                     HEAP_ZERO_MEMORY,
                                                     dwDataSize );


    for (i=0; i<2; i++)
    {
        hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                               &IID_IDirectPlay4A, (LPVOID*) &pDP[i] );
        ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
        if (FAILED(hr)) return;
        init_TCPIP_provider( pDP[i], "127.0.0.1", 0 );
    }
    ZeroMemory( &dpsd, sizeof(DPSESSIONDESC2) );
    dpsd.dwSize = sizeof(DPSESSIONDESC2);
    dpsd.guidApplication = appGuid;
    dpsd.dwMaxPlayers = 10;
    dpsd.dwFlags = DPSESSION_MIGRATEHOST;

    /* Host */
    hr = IDirectPlayX_Open( pDP[0], &dpsd, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    if ( hr != DP_OK )
    {
        todo_wine win_skip( "dplay not implemented enough for this test yet\n" );
        return;
    }

    hr = IDirectPlayX_CreatePlayer( pDP[0], &dpid[0], NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );

    /* Peer */
    hr = IDirectPlayX_EnumSessions( pDP[1], &dpsd, 0, EnumSessions_cb_join,
                                    pDP[1], 0 );
    checkHR( DP_OK, hr );

    hr = IDirectPlayX_CreatePlayer( pDP[1], &dpid[1], NULL, NULL, NULL, 0, 0 );
    checkHR( DP_OK, hr );


    /* Host: One message in queue */
    IDirectPlayX_GetMessageCount( pDP[0], dpid[0], &dwCount );
    check( 1, dwCount );
    dwDataSize = 1024;
    hr = IDirectPlayX_Receive( pDP[0], &idFrom, &idTo, DPRECEIVE_PEEK,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkConv( DPSYS_CREATEPLAYERORGROUP, lpData->dwType, dpMsgType2str );

    /* Peer: No messages */
    IDirectPlayX_GetMessageCount( pDP[1], dpid[1], &dwCount );
    check( 0, dwCount );
    hr = IDirectPlayX_Receive( pDP[1], &idFrom, &idTo, DPRECEIVE_PEEK,
                               lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );


    /* Closing host */
    IDirectPlayX_Close( pDP[0] );


    /* Host: Queue is cleaned */
    IDirectPlayX_GetMessageCount( pDP[0], dpid[0], &dwCount );
    check( 0, dwCount );
    hr = IDirectPlayX_Receive( pDP[0], &idFrom, &idTo, DPRECEIVE_PEEK,
                               lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );

    /* Peer: gets message of player destruction */
    IDirectPlayX_GetMessageCount( pDP[1], dpid[1], &dwCount );
    check( 2, dwCount );
    dwDataSize = 1024;
    hr = IDirectPlayX_Receive( pDP[1], &idFrom, &idTo, DPRECEIVE_PEEK,
                               lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    checkConv( DPSYS_DESTROYPLAYERORGROUP, lpData->dwType, dpMsgType2str );


    /* Message analysis */
    for (i=0; i<2; i++)
    {
        hr = IDirectPlayX_Receive( pDP[1], &idFrom, &idTo, 0,
                                   lpData, &dwDataSize );
        checkHR( DP_OK, hr );
        check( DPID_SYSMSG, idFrom );
        check( dpid[1], idTo ); /* Peer player id */
        switch(i)
        {
        case 0:
            checkConv( DPSYS_DESTROYPLAYERORGROUP, lpData->dwType,
                       dpMsgType2str );
            check( DPPLAYERTYPE_PLAYER, lpData->dwPlayerType );
            check( dpid[0],             lpData->dpId ); /* Host player id */
            checkLP( NULL,              lpData->lpLocalData );
            check( 0,                   lpData->dwLocalDataSize );
            checkLP( NULL,              lpData->lpRemoteData );
            check( 0,                   lpData->dwRemoteDataSize );
            checkLP( NULL,              U1(lpData->dpnName).lpszShortNameA );
            check( 0,                   lpData->dpIdParent );
            checkFlags( 0,              lpData->dwFlags,
                        FLAGS_DPPLAYER | FLAGS_DPGROUP );
            break;
        case 1:
            checkConv( DPSYS_HOST, lpData->dwType, dpMsgType2str );
            break;
        default:
            break;
        }
        dwDataSize = 1024;
    }
    hr = IDirectPlayX_Receive( pDP[1], &idFrom, &idTo, 0, lpData, &dwDataSize );
    checkHR( DPERR_NOMESSAGES, hr );


    HeapFree( GetProcessHeap(), 0, lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

static void test_COM(void)
{
    IDirectPlay *dp;
    IDirectPlay2A *dp2A;
    IDirectPlay2 *dp2;
    IDirectPlay3A *dp3A;
    IDirectPlay3 *dp3;
    IDirectPlay4A *dp4A;
    IDirectPlay4 *dp4 = (IDirectPlay4*)0xdeadbeef;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectPlay, (IUnknown*)&dp4, CLSCTX_INPROC_SERVER, &IID_IUnknown,
            (void**)&dp4);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == E_INVALIDARG),
            "DirectPlay create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dp4 || dp4 == (IDirectPlay4*)0xdeadbeef, "dp4 = %p\n", dp4);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayLobby,
            (void**)&dp4);
    ok(hr == E_NOINTERFACE, "DirectPlay create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Different refcount for all DirectPlay Interfaces */
    hr = CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4,
            (void**)&dp4);
    ok(hr == S_OK, "DirectPlay create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectPlayX_AddRef(dp4);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay2A, (void**)&dp2A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay2A failed: %08x\n", hr);
    refcount = IDirectPlay2_AddRef(dp2A);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlay2_Release(dp2A);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay2, (void**)&dp2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay2 failed: %08x\n", hr);
    refcount = IDirectPlay2_AddRef(dp2);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlay2_Release(dp2);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay3A, (void**)&dp3A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay3A failed: %08x\n", hr);
    refcount = IDirectPlay3_AddRef(dp3A);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlay3_Release(dp3A);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay3, (void**)&dp3);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay3 failed: %08x\n", hr);
    refcount = IDirectPlay3_AddRef(dp3);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlay3_Release(dp3);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay4A, (void**)&dp4A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay4A failed: %08x\n", hr);
    refcount = IDirectPlayX_AddRef(dp4A);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayX_Release(dp4A);

    /* IDirectPlay and IUnknown share a refcount */
    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay, (void**)&dp);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay failed: %08x\n", hr);
    refcount = IDirectPlayX_AddRef(dp);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlay_Release(dp);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 3, "refcount == %u, expected 3\n", refcount);
    refcount = IUnknown_Release(unk);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    IUnknown_Release(unk);
    IDirectPlay_Release(dp);
    IDirectPlayX_Release(dp4A);
    IDirectPlay3_Release(dp3);
    IDirectPlay3_Release(dp3A);
    IDirectPlay2_Release(dp2);
    IDirectPlay2_Release(dp2A);
    IDirectPlayX_Release(dp4);
    refcount = IDirectPlayX_Release(dp4);
    ok(refcount == 0, "refcount == %u, expected 0\n", refcount);
}

static void test_COM_dplobby(void)
{
    IDirectPlayLobby *dpl = (IDirectPlayLobby*)0xdeadbeef;
    IDirectPlayLobbyA *dplA;
    IDirectPlayLobby2A *dpl2A;
    IDirectPlayLobby2 *dpl2;
    IDirectPlayLobby3A *dpl3A;
    IDirectPlayLobby3 *dpl3;
    IUnknown *unk;
    ULONG refcount;
    HRESULT hr;

    /* COM aggregation */
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, (IUnknown*)&dpl, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dpl);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == E_INVALIDARG),
            "DirectPlayLobby create failed: %08x, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dpl || dpl == (IDirectPlayLobby*)0xdeadbeef, "dpl = %p\n", dpl);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay,
            (void**)&dpl);
    ok(hr == E_NOINTERFACE, "DirectPlayLobby create failed: %08x, expected E_NOINTERFACE\n", hr);

    /* Different refcount for all DirectPlayLobby Interfaces */
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayLobby,
            (void**)&dpl);
    ok(hr == S_OK, "DirectPlayLobby create failed: %08x, expected S_OK\n", hr);
    refcount = IDirectPlayLobby_AddRef(dpl);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobbyA, (void**)&dplA);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobbyA failed: %08x\n", hr);
    refcount = IDirectPlayLobby_AddRef(dplA);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayLobby_Release(dplA);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby2, (void**)&dpl2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby2 failed: %08x\n", hr);
    refcount = IDirectPlayLobby_AddRef(dpl2);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayLobby_Release(dpl2);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby2A, (void**)&dpl2A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby2A failed: %08x\n", hr);
    refcount = IDirectPlayLobby_AddRef(dpl2A);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayLobby_Release(dpl2A);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby3, (void**)&dpl3);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby3 failed: %08x\n", hr);
    refcount = IDirectPlayLobby_AddRef(dpl3);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayLobby_Release(dpl3);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby3A, (void**)&dpl3A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby3A failed: %08x\n", hr);
    refcount = IDirectPlayLobby_AddRef(dpl3A);
    ok(refcount == 2, "refcount == %u, expected 2\n", refcount);
    IDirectPlayLobby_Release(dpl3A);

    /* IDirectPlayLobby and IUnknown share a refcount */
    hr = IDirectPlayX_QueryInterface(dpl, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08x\n", hr);
    refcount = IUnknown_AddRef(unk);
    ok(refcount == 4, "refcount == %u, expected 4\n", refcount);
    IDirectPlayLobby_Release(unk);

    IUnknown_Release(unk);
    IDirectPlayLobby_Release(dpl3);
    IDirectPlayLobby_Release(dpl3A);
    IDirectPlayLobby_Release(dpl2);
    IDirectPlayLobby_Release(dpl2A);
    IDirectPlayLobby_Release(dplA);
    IDirectPlayLobby_Release(dpl);
    refcount = IDirectPlayLobby_Release(dpl);
    ok(refcount == 0, "refcount == %u, expected 0\n", refcount);
}


START_TEST(dplayx)
{
    CoInitialize( NULL );

    test_COM();
    test_COM_dplobby();
    test_EnumerateProviders();

    if (!winetest_interactive)
    {
        skip("Run in interactive mode to run dplayx tests.\n");
        return;
    }

    trace("Running in interactive mode, tests will take a while\n");

    test_DirectPlayCreate();
    test_EnumConnections();
    test_InitializeConnection();

    test_GetCaps();
    /* test_Open() takes almost a minute, */
    test_Open();
    /* test_EnumSession takes three minutes */
    test_EnumSessions();
    test_SessionDesc();

    /* test_CreatePlayer() takes over a minute */
    test_CreatePlayer();
    test_GetPlayerCaps();
    test_PlayerData();
    test_PlayerName();

    /* test_GetPlayerAccount() takes over 30s */
    test_GetPlayerAccount();
    test_GetPlayerAddress();
    test_GetPlayerFlags();

    test_CreateGroup();
    test_GroupOwner();

    test_EnumPlayers();
    test_EnumGroups();
    test_EnumGroupsInGroup();

    test_groups_p2p();
    test_groups_cs();

    test_Send();
    test_Receive();
    test_GetMessageCount();
    test_GetMessageQueue();

    test_remote_data_replication();
    test_host_migration();

    CoUninitialize();
}
