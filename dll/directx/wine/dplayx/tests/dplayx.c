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
#include <netfw.h>
#include <winsock2.h>

static HRESULT (WINAPI *pDirectPlayEnumerateA)( LPDPENUMDPCALLBACKA, void* );
static HRESULT (WINAPI *pDirectPlayEnumerateW)( LPDPENUMDPCALLBACKW, void* );
static HRESULT (WINAPI *pDirectPlayCreate)( GUID *GUID, LPDIRECTPLAY *lplpDP, IUnknown *pUnk );

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
static LPCSTR dwFlags2str(DWORD dwFlags, DWORD flagType);
#define checkFlags(expected, result, flags) checkFlags_(__LINE__, expected, result, flags)
static void checkFlags_(unsigned line, DWORD expected, DWORD result, DWORD flags)
{
    ok_(__FILE__, line)( expected == result,
        "expected=0x%08lx(%s) got=0x%08lx(%s)\n",
        expected, dwFlags2str(expected, flags),
        result, dwFlags2str(result, flags) );
}
#define checkGuid(expected, result)             \
    ok( IsEqualGUID(expected, result),          \
        "expected=%s got=%s\n",                 \
        Guid2str(expected), Guid2str(result) );
#define checkConv(expected, result, function)    \
    ok( (expected) == (result),                  \
        "expected=0x%08x(%s) got=0x%08lx(%s)\n", \
        expected, function(expected),            \
        result, function(result) );

#define AW( str, ansi ) ((ansi) ? (const void *) str : (const void *) L##str)

static const char *dbgStrAW( const void *str, BOOL ansi )
{
    return ansi ? wine_dbgstr_a( str ) : wine_dbgstr_w( str );
}

static int strcmpAW( const void *str0, const void *str1, BOOL ansi )
{
    return ansi ? strcmp( str0, str1 ) : lstrcmpW( str0, str1 );
}

DEFINE_GUID(appGuid, 0xbdcfe03e, 0xf0ec, 0x415b, 0x82, 0x11, 0x6f, 0x86, 0xd8, 0x19, 0x7f, 0xe1);
DEFINE_GUID(appGuid2, 0x93417d3f, 0x7d26, 0x46ba, 0xb5, 0x76, 0xfe, 0x4b, 0x20, 0xbb, 0xad, 0x70);
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
DEFINE_GUID(invalid_guid, 0x7b48b707, 0x0034, 0xc000, 0x02, 0x00, 0x00, 0x00, 0xec, 0xf6, 0x32, 0x00);


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
        sprintf( buffer, "%ld", HRESULT_CODE(hr) );
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
    LPVOID lpData = calloc( 1, dwDataSize );
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

        sprintf( temp, "%ld,", dwDataSize );
        strcat( callbackData->szTrace2, temp );

        dwDataSize = 1024;
        ++i;
    }

    checkHR( DPERR_NOMESSAGES, hr );

    callbackData->szTrace1[ 3*i ] = '\0';
    callbackData->dwCounter1 = i;


    free( lpData );
}

typedef struct
{
    IDirectPlay4 *dp;
    DPSESSIONDESC2 *dpsd;
    DWORD timeout;
    LPDPENUMSESSIONSCALLBACK2 callback;
    void *context;
    DWORD flags;

    HRESULT hr;

    HANDLE thread;
} EnumSessionsParam;

static CALLBACK DWORD enumSessionsProc( void *p )
{
    EnumSessionsParam *param = p;

    param->hr = IDirectPlayX_EnumSessions( param->dp, param->dpsd, param->timeout, param->callback,
                                           param->context, param->flags );

    return 0;
}

static HANDLE enumSessionsAsync( IDirectPlay4 *dp, DPSESSIONDESC2 *dpsd, DWORD timeout,
                                 LPDPENUMSESSIONSCALLBACK2 callback, void *context, DWORD flags )
{
    EnumSessionsParam *param;

    param = calloc( 1, sizeof( EnumSessionsParam ) );

    param->dp = dp;
    param->dpsd = dpsd;
    param->timeout = timeout;
    param->callback = callback;
    param->context = context;
    param->flags = flags;

    param->thread = CreateThread( NULL, 0, enumSessionsProc, param, 0, NULL );

    return param;
}

static HRESULT enumSessionsAsyncWait( EnumSessionsParam *param, DWORD timeout )
{
    HRESULT hr = 0xdeadbeef;
    DWORD waitResult;

    waitResult = WaitForSingleObject( param->thread, timeout );
    CloseHandle( param->thread );
    if ( waitResult == WAIT_OBJECT_0 )
    {
        hr = param->hr;
        free( param );
    }

    return hr;
}

typedef struct
{
    IDirectPlay4 *dp;
    DPSESSIONDESC2 *dpsd;
    DWORD flags;

    HRESULT hr;

    HANDLE thread;
} OpenParam;

static CALLBACK DWORD openProc( void *p )
{
    OpenParam *param = p;

    param->hr = IDirectPlayX_Open( param->dp, param->dpsd, param->flags );

    return 0;
}

static OpenParam *openAsync( IDirectPlay4 *dp, DPSESSIONDESC2 *dpsd, DWORD flags )
{
    OpenParam *param;

    param = calloc( 1, sizeof( OpenParam ) );

    param->dp = dp;
    param->dpsd = dpsd;
    param->flags = flags;

    param->thread = CreateThread( NULL, 0, openProc, param, 0, NULL );

    return param;
}

static HRESULT openAsyncWait( OpenParam *param, DWORD timeout )
{
    HRESULT hr = 0xdeadbeef;
    DWORD waitResult;

    waitResult = WaitForSingleObject( param->thread, timeout );
    CloseHandle( param->thread );
    if ( waitResult == WAIT_OBJECT_0 )
    {
        hr = param->hr;
        free( param );
    }

    return hr;
}

typedef struct
{
    IDirectPlay4 *dp;
    DPID *dpid;
    DPNAME *name;
    HANDLE event;
    void *data;
    DWORD dataSize;
    DWORD flags;

    HRESULT hr;

    HANDLE thread;
} CreatePlayerParam;

static CALLBACK DWORD createPlayerProc( void *p )
{
    CreatePlayerParam *param = p;

    param->hr = IDirectPlayX_CreatePlayer( param->dp, param->dpid, param->name, param->event, param->data,
                                           param->dataSize, param->flags );

    return 0;
}

static CreatePlayerParam *createPlayerAsync( IDirectPlay4 *dp, DPID *dpid, DPNAME *name, HANDLE event, void *data,
                                             DWORD dataSize, DWORD flags )
{
    CreatePlayerParam *param;

    param = calloc( 1, sizeof( CreatePlayerParam ) );

    param->dp = dp;
    param->dpid = dpid;
    param->name = name;
    param->event = event;
    param->data = data;
    param->dataSize = dataSize;
    param->flags = flags;

    param->thread = CreateThread( NULL, 0, createPlayerProc, param, 0, NULL );

    return param;
}

static HRESULT createPlayerAsyncWait( CreatePlayerParam *param, DWORD timeout )
{
    HRESULT hr = 0xdeadbeef;
    DWORD waitResult;

    waitResult = WaitForSingleObject( param->thread, timeout );
    CloseHandle( param->thread );
    if ( waitResult == WAIT_OBJECT_0 )
    {
        hr = param->hr;
        free( param );
    }

    return hr;
}

#include "pshpack1.h"

typedef struct
{
    DWORD mixed;
    SOCKADDR_IN addr;
} SpHeader;

typedef struct
{
    DWORD magic;
    WORD command;
    WORD version;
} MessageHeader;

typedef struct
{
    SOCKADDR_IN tcpAddr;
    SOCKADDR_IN udpAddr;
} SpData;

typedef struct
{
    DWORD size;
    DWORD flags;
    DPID id;
    DWORD shortNameLength;
    DWORD longNameLength;
    DWORD spDataSize;
    DWORD playerDataSize;
    DWORD playerCount;
    DPID systemPlayerId;
    DWORD fixedSize;
    DWORD playerVersion;
    DPID parentId;
} PackedPlayer;

typedef struct
{
    DWORD size;
    DWORD flags;
    DPID id;
    DWORD infoMask;
    DWORD versionOrSystemPlayerId;
} SuperPackedPlayer;

typedef struct
{
    MessageHeader header;
    GUID appGuid;
    DWORD passwordOffset;
    DWORD flags;
} EnumSessionsRequest;

typedef struct
{
    MessageHeader header;
    DPSESSIONDESC2 dpsd;
    DWORD nameOffset;
} EnumSessionsReply;

typedef struct
{
    MessageHeader header;
    DWORD flags;
} RequestPlayerId;

typedef struct
{
    MessageHeader header;
    DPID id;
    DPSECURITYDESC secDesc;
    DWORD sspiProviderOffset;
    DWORD capiProviderOffset;
    HRESULT result;
    WCHAR sspiProvider[ 16 ];
    WCHAR capiProvider[ 16 ];
} RequestPlayerReply;

typedef struct
{
    MessageHeader header;
    DPID toId;
    DPID playerId;
    DPID groupId;
    DWORD createOffset;
    DWORD passwordOffset;
    PackedPlayer playerInfo;
    SpData spData;
} AddForwardRequest;

typedef struct
{
    MessageHeader header;
    DWORD playerCount;
    DWORD groupCount;
    DWORD packedOffset;
    DWORD shortcutCount;
    DWORD descriptionOffset;
    DWORD nameOffset;
    DWORD passwordOffset;
} SuperEnumPlayersReply;

typedef struct
{
    MessageHeader header;
    HRESULT result;
} AddForwardReply;

typedef struct
{
    MessageHeader header;
    DPID toId;
    DPID playerId;
    DPID groupId;
    DWORD createOffset;
    DWORD passwordOffset;
    PackedPlayer playerInfo;
    SpData spData;
} AddForward;

typedef struct
{
    MessageHeader header;
    DPID playerId;
} AddForwardAck;

typedef struct
{
    MessageHeader header;
    DPID toId;
    DPID playerId;
    DPID groupId;
    DWORD createOffset;
    DWORD passwordOffset;
    PackedPlayer playerInfo;
} CreatePlayer;

typedef struct
{
    DPID fromId;
    DPID toId;
} GameMessage;

typedef struct
{
    MessageHeader header;
    DPID fromId;
    DWORD tickCount;
} Ping;

typedef struct
{
    MessageHeader header;
    DPID toId;
    DPID playerId;
    DPID groupId;
    DWORD createOffset;
    DWORD passwordOffset;
} AddPlayerToGroup;

typedef struct
{
    MessageHeader header;
    DPID toId;
    DPID groupId;
    DWORD dataSize;
    DWORD dataOffset;
} GroupDataChanged;

#include "poppack.h"

#define bindUdp( port ) bindUdp_( __LINE__, port )
static SOCKET bindUdp_( int line, unsigned short port )
{
    u_long nbio = 1;
    SOCKADDR_IN addr;
    int wsResult;
    SOCKET sock;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    ok_( __FILE__, line) ( sock != INVALID_SOCKET, "got UDP socket %#Ix.\n", sock );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = htonl( INADDR_ANY );
    wsResult = bind( sock, (SOCKADDR *) &addr, sizeof( addr ) );
    ok_( __FILE__, line)( !wsResult, "bind() returned %d.\n", wsResult );

    wsResult = ioctlsocket( sock, FIONBIO, &nbio );
    ok_( __FILE__, line)( !wsResult, "ioctlsocket() returned %d.\n", wsResult );

    return sock;
}

#define listenTcp( port ) listenTcp_( __LINE__, port )
static SOCKET listenTcp_( int line, unsigned short port )
{
    u_long nbio = 1;
    SOCKADDR_IN addr;
    int wsResult;
    SOCKET sock;

    sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    ok_( __FILE__, line)( sock != INVALID_SOCKET, "got TCP listen socket %#Ix.\n", sock );

    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = INADDR_ANY;

    wsResult = bind( sock, (SOCKADDR *) &addr, sizeof( addr ) );
    ok_( __FILE__, line )( wsResult != SOCKET_ERROR, "bind() returned %d.\n", wsResult );

    wsResult = listen( sock, SOMAXCONN );
    ok_( __FILE__, line )( wsResult != SOCKET_ERROR, "listen() returned %d.\n", wsResult );

    wsResult = ioctlsocket( sock, FIONBIO, &nbio );
    ok_( __FILE__, line)( wsResult != SOCKET_ERROR, "ioctlsocket() returned %d.\n", wsResult );

    return sock;
}

#define acceptTcp( listenSock ) acceptTcp_( __LINE__, listenSock )
static SOCKET acceptTcp_( int line, SOCKET listenSock )
{
    struct timeval timeout;
    SOCKADDR_IN addr;
    int addrSize;
    int wsResult;
    SOCKET sock;
    fd_set fds;

    FD_ZERO( &fds );
    FD_SET( listenSock, &fds );
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    wsResult = select( listenSock + 1, &fds, NULL, &fds, &timeout );
    ok_( __FILE__, line )( wsResult != SOCKET_ERROR, "select() returned %d.\n", wsResult );

    addrSize = sizeof( addr );
    sock = accept( listenSock, (SOCKADDR *) &addr, &addrSize );

    return sock;
}

#define checkNoMoreAccepts( listenSock ) checkNoMoreAccepts_( __LINE__, listenSock )
static void checkNoMoreAccepts_( int line, SOCKET listenSock )
{
    struct timeval timeout;
    int wsResult;
    fd_set fds;

    FD_ZERO( &fds );
    FD_SET( listenSock, &fds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    wsResult = select( listenSock + 1, &fds, NULL, &fds, &timeout );
    ok_( __FILE__, line )( !wsResult, "select() returned %d.\n", wsResult );
}

#define connectTcp( port ) connectTcp_( __LINE__, port )
static SOCKET connectTcp_( int line, unsigned short port )
{
    SOCKADDR_IN addr;
    int wsResult;
    SOCKET sock;

    sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    ok_( __FILE__, line)( sock != INVALID_SOCKET, "got send socket %#Ix.\n", sock );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    wsResult = connect( sock, (SOCKADDR *) &addr, sizeof( addr ) );
    ok_( __FILE__, line)( !wsResult, "connect returned %d.\n", wsResult );

    return sock;
}

#define connectUdp( port ) connectUdp_( __LINE__, port)
static SOCKET connectUdp_( int line, unsigned short port )
{
    SOCKADDR_IN addr;
    int wsResult;
    SOCKET sock;

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    ok_( __FILE__, line)( sock != INVALID_SOCKET, "got send socket %#Ix.\n", sock );

    addr.sin_family = AF_INET;
    addr.sin_port = htons( port );
    addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
    wsResult = connect( sock, (SOCKADDR *) &addr, sizeof( addr ) );
    ok_( __FILE__, line)( !wsResult, "connect returned %d.\n", wsResult );

    return sock;
}

#define receiveMessage( sock, buffer, size ) receiveMessage_( __LINE__, sock, buffer, size )
static int receiveMessage_( int line, SOCKET sock, void *buffer, int size )
{
    struct timeval timeout;
    int recvResult;
    int wsResult;
    fd_set fds;

    FD_ZERO( &fds );
    FD_SET( sock, &fds );
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    wsResult = select( sock + 1, &fds, NULL, &fds, &timeout );
    ok_( __FILE__, line )( wsResult != SOCKET_ERROR, "select() returned %d.\n", wsResult );

    memset( buffer, 0xcc, size );
    recvResult = recv( sock, buffer, size, 0 );

    return recvResult;
}

#define checkNoMoreMessages( sock ) checkNoMoreMessages_( __LINE__, sock )
static void checkNoMoreMessages_( int line, SOCKET sock )
{
    struct timeval timeout;
    char buffer[ 1024 ];
    int wsResult;
    fd_set fds;

    FD_ZERO( &fds );
    FD_SET( sock, &fds );
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    select( sock + 1, &fds, NULL, &fds, &timeout );

    wsResult = recv( sock, buffer, sizeof( buffer ), 0 );
    ok_( __FILE__, line )( !wsResult || wsResult == SOCKET_ERROR, "recv() returned %d.\n", wsResult );
}

#define checkSpHeader( header, expectedSize ) checkSpHeader_( __LINE__, header, expectedSize )
static unsigned short checkSpHeader_( int line, SpHeader *header, DWORD expectedSize )
{
    ok_( __FILE__, line )( header->mixed == 0xfab00000 + expectedSize, "got mixed %#lx.\n", header->mixed );
    ok_( __FILE__, line )( header->addr.sin_family == AF_INET, "got family %d.\n", header->addr.sin_family );
    ok_( __FILE__, line )( 2300 <= ntohs( header->addr.sin_port ) && ntohs( header->addr.sin_port ) < 2350,
                           "got port %d.\n", ntohs( header->addr.sin_port ) );
    ok_( __FILE__, line )( !header->addr.sin_addr.s_addr, "got address %#lx.\n", header->addr.sin_addr.s_addr );

    return ntohs( header->addr.sin_port );
}

#define checkMessageHeader( header, expectedCommand ) checkMessageHeader_( __LINE__, header, expectedCommand )
static void checkMessageHeader_( int line, MessageHeader *header, WORD expectedCommand )
{
    ok_( __FILE__, line )( header->magic == 0x79616c70, "got magic %#lx.\n", header->magic );
    ok_( __FILE__, line )( header->command == expectedCommand, "got command %d.\n", header->command );
}

#define checkSpData( spData ) checkSpData_( __LINE__, spData, udpPort )
static void checkSpData_( int line, SpData *spData, unsigned short *udpPort )
{
    ok_( __FILE__, line )( spData->tcpAddr.sin_family == AF_INET, "got TCP family %d.\n", spData->tcpAddr.sin_family );
    ok_( __FILE__, line )( 2300 <= ntohs( spData->tcpAddr.sin_port ) && ntohs( spData->tcpAddr.sin_port ) < 2350,
                           "got TCP port %d.\n", ntohs( spData->tcpAddr.sin_port ) );
    ok_( __FILE__, line )( !spData->tcpAddr.sin_addr.s_addr, "got TCP address %#lx.\n",
                           spData->tcpAddr.sin_addr.s_addr );
    ok_( __FILE__, line )( spData->udpAddr.sin_family == AF_INET, "got UDP family %d.\n", spData->udpAddr.sin_family );
    ok_( __FILE__, line )( 2350 <= ntohs( spData->udpAddr.sin_port ) && ntohs( spData->udpAddr.sin_port ) < 2400,
                           "got UDP port %d.\n", ntohs( spData->udpAddr.sin_port ) );
    ok_( __FILE__, line )( !spData->udpAddr.sin_addr.s_addr, "got UDP address %#lx.\n",
                           spData->udpAddr.sin_addr.s_addr );

    if ( udpPort )
        *udpPort = ntohs( spData->udpAddr.sin_port );
}

#define checkPackedPlayer( player, expectedFlags, expectedId, expectedShortNameLength, expectedLongNameLength, \
                           expectedPlayerDataSize, expectedSystemPlayerId ) \
        checkPackedPlayer_( __LINE__, player, expectedFlags, expectedId, expectedShortNameLength, expectedLongNameLength, \
                            expectedPlayerDataSize, expectedSystemPlayerId )
static void checkPackedPlayer_( int line, PackedPlayer *player, DWORD expectedFlags, DPID expectedId,
                                DWORD expectedShortNameLength, DWORD expectedLongNameLength,
                                DWORD expectedPlayerDataSize, DPID expectedSystemPlayerId )
{
    DWORD expectedSize = sizeof( PackedPlayer ) + expectedShortNameLength + expectedLongNameLength + sizeof( SpData )
                       + expectedPlayerDataSize;

    ok_( __FILE__, line )( player->size == expectedSize, "got player info size %lu.\n", player->size );
    ok_( __FILE__, line )( player->flags == expectedFlags, "got flags %#lx.\n", player->flags );
    ok_( __FILE__, line )( player->id == expectedId, "got player info player id %#lx.\n", player->id );
    ok_( __FILE__, line )( player->shortNameLength == expectedShortNameLength, "got short name length %lu.\n",
                           player->shortNameLength );
    ok_( __FILE__, line )( player->longNameLength == expectedLongNameLength, "got long name length %lu.\n",
                           player->longNameLength );
    ok_( __FILE__, line )( player->spDataSize == sizeof( SpData ), "got SP data size %lu.\n", player->spDataSize );
    ok_( __FILE__, line )( player->playerDataSize == expectedPlayerDataSize, "got player data size %lu.\n",
                           player->playerDataSize );
    ok_( __FILE__, line )( !player->playerCount, "got player count %lu.\n", player->playerCount );
    ok_( __FILE__, line )( player->systemPlayerId == expectedSystemPlayerId, "got system player id %#lx.\n",
                           player->systemPlayerId );
    ok_( __FILE__, line )( player->fixedSize == sizeof( PackedPlayer ), "got fixed size %lu.\n", player->fixedSize );
    ok_( __FILE__, line )( !player->parentId, "got parent id %#lx.\n", player->parentId );
}

#define checkGameMessage( message, expectedFromId, expectedToId ) \
        checkGameMessage_( __LINE__, message, expectedFromId, expectedToId )
static void checkGameMessage_( int line, GameMessage *message, DPID expectedFromId, DPID expectedToId )
{
    ok_( __FILE__, line )( message->fromId == expectedFromId, "got source id %#lx.\n", message->fromId );
    ok_( __FILE__, line )( message->toId == expectedToId, "got destination id %#lx.\n", message->toId );
}

#define receiveEnumSessionsRequest( sock, expectedAppGuid, expectedPassword, expectedFlags ) \
        receiveEnumSessionsRequest_( __LINE__, sock, expectedAppGuid, expectedPassword, expectedFlags )
static unsigned short receiveEnumSessionsRequest_( int line, SOCKET sock, const GUID *expectedAppGuid,
                                                   const WCHAR *expectedPassword, DWORD expectedFlags )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        EnumSessionsRequest request;
        WCHAR password[ 256 ];
    } request;
#include "poppack.h"
    DWORD expectedPasswordSize;
    unsigned short port;
    DWORD expectedSize;
    int wsResult;

    expectedPasswordSize = expectedPassword ? (lstrlenW( expectedPassword ) + 1) * sizeof( WCHAR ) : 0;
    expectedSize = sizeof( request.spHeader ) + sizeof( request.request ) + expectedPasswordSize;

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == expectedSize, "recv() returned %d.\n", wsResult );
    if ( wsResult == SOCKET_ERROR )
        return 0;

    port = checkSpHeader_( line, &request.spHeader, expectedSize );
    checkMessageHeader_( line, &request.request.header, 2 );
    ok_( __FILE__, line )( IsEqualGUID( &request.request.appGuid, expectedAppGuid ), "got app guid %s.\n",
                           wine_dbgstr_guid( &request.request.appGuid ) );
    if ( expectedPassword )
    {
        ok_( __FILE__, line )( request.request.passwordOffset == 32, "got password offset %lu.\n",
                               request.request.passwordOffset );
        ok_( __FILE__, line )( !lstrcmpW( request.password, expectedPassword ), "got password %s.\n",
                               wine_dbgstr_w( request.password ) );
    }
    else
    {
        ok_( __FILE__, line )( !request.request.passwordOffset, "got password offset %lu.\n",
                               request.request.passwordOffset );
    }
    ok_( __FILE__, line )( request.request.flags == expectedFlags, "got flags %#lx.\n", request.request.flags );

    return port;
}

#define sendEnumSessionsReply( sock, port, dpsd ) sendEnumSessionsReply_( __LINE__, sock, port, dpsd )
static void sendEnumSessionsReply_( int line, SOCKET sock, unsigned short port, const DPSESSIONDESC2 *dpsd )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        EnumSessionsReply reply;
        WCHAR name[ 256 ];
    } reply =
    {
        .spHeader =
        {
            .mixed = 0xfab00000,
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .reply =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 1,
                .version = 14,
            },
            .dpsd = *dpsd,
            .nameOffset = sizeof( reply.reply ),
        },
    };
#include "poppack.h"
    DWORD passwordSize;
    int wsResult;
    DWORD size;

    passwordSize = (lstrlenW( dpsd->lpszSessionName ) + 1) * sizeof( WCHAR );
    size = sizeof( reply.spHeader ) + sizeof( reply.reply ) + passwordSize;

    reply.spHeader.mixed += size;
    reply.reply.dpsd.lpszSessionName = NULL;
    reply.reply.dpsd.lpszPassword = NULL;
    memcpy( reply.name, dpsd->lpszSessionName, passwordSize );

    wsResult = send( sock, (char *) &reply, size, 0 );
    ok_( __FILE__, line )( wsResult == size, "send() returned %d.\n", wsResult );
}

#define receiveRequestPlayerId( sock, expectedFlags ) receiveRequestPlayerId_( __LINE__, sock, expectedFlags )
static unsigned short receiveRequestPlayerId_( int line, SOCKET sock, DWORD expectedFlags )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        RequestPlayerId request;
    } request;
#include "poppack.h"
    unsigned short port;
    int wsResult;

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, sizeof( request ) );
    checkMessageHeader_( line, &request.request.header, 5 );
    ok_( __FILE__, line )( request.request.flags == expectedFlags, "got flags %#lx.\n", request.request.flags );

    return port;
}

#define sendRequestPlayerReply( sock, port, id, result ) sendRequestPlayerReply_( __LINE__, sock, port, id, result )
static void sendRequestPlayerReply_( int line, SOCKET sock, unsigned short port, DPID id, HRESULT result )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        RequestPlayerReply reply;
    } reply =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( reply ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .reply =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 7,
                .version = 14,
            },
            .id = id,
            .result = result,
        },
    };
#include "poppack.h"
    int wsResult;

    wsResult = send( sock, (char *) &reply, sizeof( reply ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( reply ), "send() returned %d.\n", wsResult );
}

#define receiveAddForwardRequest( sock, expectedPlayerId, expectedPassword, expectedTickCount, udpPort ) \
        receiveAddForwardRequest_( __LINE__, sock, expectedPlayerId, expectedPassword, expectedTickCount, udpPort )
static unsigned short receiveAddForwardRequest_( int line, SOCKET sock, DPID expectedPlayerId,
                                                 const WCHAR *expectedPassword, DWORD expectedTickCount,
                                                 unsigned short *udpPort )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        AddForwardRequest request;
    } request;
#include "poppack.h"
    DWORD expectedPasswordSize;
    WCHAR password[ 256 ];
    unsigned short port;
    DWORD expectedSize;
    DWORD tickCount;
    int wsResult;

    expectedPasswordSize = (lstrlenW( expectedPassword ) + 1) * sizeof( WCHAR );
    expectedSize = sizeof( request ) + expectedPasswordSize + sizeof( DWORD );

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );
    if ( wsResult == SOCKET_ERROR )
        return 0;

    port = checkSpHeader_( line, &request.spHeader, expectedSize );
    checkMessageHeader_( line, &request.request.header, 19 );
    ok_( __FILE__, line )( !request.request.toId, "got destination id %#lx.\n", request.request.toId );
    ok_( __FILE__, line )( request.request.playerId == expectedPlayerId, "got player id %#lx.\n",
                           request.request.playerId );
    ok_( __FILE__, line )( !request.request.groupId, "got group id %#lx.\n", request.request.groupId );
    ok_( __FILE__, line )( request.request.createOffset == 28, "got create offset %lu.\n",
                           request.request.createOffset );
    ok_( __FILE__, line )( request.request.passwordOffset == 108, "got password offset %lu.\n",
                           request.request.passwordOffset );
    checkPackedPlayer_( line, &request.request.playerInfo, 0x9, expectedPlayerId, 0, 0, 0, expectedPlayerId );
    checkSpData_( line, &request.request.spData, udpPort );

    wsResult = receiveMessage_( line, sock, password, expectedPasswordSize );

    ok_( __FILE__, line )( wsResult == expectedPasswordSize, "recv() returned %d.\n", wsResult );
    ok_( __FILE__, line )( !lstrcmpW( password, expectedPassword ), "got password %s.\n", wine_dbgstr_w( password ) );

    wsResult = receiveMessage_( line, sock, &tickCount, sizeof( DWORD ) );

    ok_( __FILE__, line )( wsResult == sizeof( DWORD ), "recv() returned %d.\n", wsResult );
    ok_( __FILE__, line )( tickCount == expectedTickCount, "got tick count %#lx.\n", tickCount );

    return port;
}

#define sendSuperEnumPlayersReply( sock, tcpPort, udpPort, dpsd, sessionName ) \
        sendSuperEnumPlayersReply_( __LINE__, sock, tcpPort, udpPort, dpsd, sessionName )
static void sendSuperEnumPlayersReply_( int line, SOCKET sock, unsigned short tcpPort, unsigned short udpPort,
                                        const DPSESSIONDESC2 *dpsd, const WCHAR *sessionName )
{
#define SHORT_NAME L"short name"
#define LONG_NAME L"long name"
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        SuperEnumPlayersReply reply;
        DPSESSIONDESC2 dpsd;
        WCHAR sessionName[ 256 ];
        struct
        {
            SuperPackedPlayer superPackedPlayer;
            BYTE spDataLength;
            SpData spData;
        } player0;
        struct
        {
            SuperPackedPlayer superPackedPlayer;
            BYTE spDataLength;
            SpData spData;
        } player1;
        struct
        {
            SuperPackedPlayer superPackedPlayer;
            WCHAR shortName[ ARRAYSIZE( SHORT_NAME ) ];
            WCHAR longName[ ARRAYSIZE( LONG_NAME ) ];
            BYTE playerDataLength;
            BYTE playerData[ 4 ];
            BYTE spDataLength;
            SpData spData;
        } player2;
        struct
        {
            SuperPackedPlayer superPackedPlayer;
            BYTE spDataLength;
            SpData spData;
        } player3;
        struct
        {
            SuperPackedPlayer superPackedPlayer;
            WCHAR shortName[ ARRAYSIZE( SHORT_NAME ) ];
            WCHAR longName[ ARRAYSIZE( LONG_NAME ) ];
            BYTE playerDataLength;
            BYTE playerData[ 4 ];
            BYTE spDataLength;
            SpData spData;
            BYTE playerCount;
            DPID playerIds[ 1 ];
        } group0;
    } reply =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( reply ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( tcpPort ),
            },
        },
        .reply =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 41,
                .version = 14,
            },
            .playerCount = 4,
            .groupCount = 1,
            .packedOffset = sizeof( reply.reply ) + sizeof( reply.dpsd ) + sizeof( reply.sessionName ),
            .shortcutCount = 0,
            .descriptionOffset = sizeof( reply.reply ),
            .nameOffset = sizeof( reply.reply ) + sizeof( reply.dpsd ),
            .passwordOffset = 0,
        },
        .dpsd = *dpsd,
        .player0 =
        {
            .superPackedPlayer =
            {
                .size = 16,
                .flags = 0x5,
                .id = 0x12345678,
                .infoMask = 0x4,
                .versionOrSystemPlayerId = 14,
            },
            .spDataLength = sizeof( SpData ),
            .spData =
            {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
        },
        .player1 =
        {
            .superPackedPlayer =
            {
                .size = 16,
                .flags = 0xf,
                .id = 0x51573,
                .infoMask = 0x4,
                .versionOrSystemPlayerId = 14,
            },
            .spDataLength = sizeof( SpData ),
            .spData =
            {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
        },
        .player2 =
        {
            .superPackedPlayer =
            {
                .size = 16,
                .flags = 0x8,
                .id = 0x1337,
                .infoMask = 0x17,
                .versionOrSystemPlayerId = 0x51573,
            },
            .shortName = SHORT_NAME,
            .longName = LONG_NAME,
            .playerDataLength = 4,
            .playerData = { 1, 2, 3, 4, },
            .spDataLength = sizeof( SpData ),
            .spData =
            {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
        },
        .player3 =
        {
            .superPackedPlayer =
            {
                .size = 16,
                .flags = 0x8,
                .id = 0xd00de,
                .infoMask = 0x4,
                .versionOrSystemPlayerId = 0x51573,
            },
            .spDataLength = sizeof( SpData ),
            .spData =
            {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
        },
        .group0 =
        {
            .superPackedPlayer =
            {
                .size = 16,
                .flags = 0x48,
                .id = 0x5e7,
                .infoMask = 0x57,
                .versionOrSystemPlayerId = 0x51573,
            },
            .shortName = SHORT_NAME,
            .longName = LONG_NAME,
            .playerDataLength = 4,
            .playerData = { 1, 2, 3, 4, },
            .spDataLength = sizeof( SpData ),
            .spData =
            {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
            .playerCount = 1,
            .playerIds = { 0xd00de, },
        },
    };
#include "poppack.h"
#undef LONG_NAME
#undef SHORT_NAME
    int wsResult;

    reply.dpsd.lpszSessionName = NULL;
    reply.dpsd.lpszPassword = NULL;

    lstrcpyW( reply.sessionName, sessionName );

    wsResult = send( sock, (char *) &reply, sizeof( reply ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( reply ), "send() returned %d.\n", wsResult );
}

#define sendAddForwardReply( sock, port, result ) sendAddForwardReply_( __LINE__, sock, port, result )
static void sendAddForwardReply_( int line, SOCKET sock, unsigned short port, HRESULT result )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        AddForwardReply reply;
    } reply =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( reply ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .reply =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 36,
                .version = 14,
            },
            .result = result,
        },
    };
#include "poppack.h"
    int wsResult;

    wsResult = send( sock, (char *) &reply, sizeof( reply ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( reply ), "send() returned %d.\n", wsResult );
}

#define sendAddForward( sock, port, tcpPort, udpPort ) sendAddForward_( __LINE__, sock, port, tcpPort, udpPort )
static void sendAddForward_( int line, SOCKET sock, unsigned short port, unsigned short tcpPort, unsigned short udpPort )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        AddForward reply;
    } reply =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( reply ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .reply =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 46,
                .version = 14,
            },
            .toId = 0,
            .playerId = 0x07734,
            .groupId = 0,
            .createOffset = 28,
            .passwordOffset = 0,
            .playerInfo =
            {
                .size = 48 + sizeof( SpData ),
                .flags = 0x5,
                .id = 0x07734,
                .shortNameLength = 0,
                .longNameLength = 0,
                .spDataSize = sizeof( SpData ),
                .playerDataSize = 0,
                .playerCount = 0,
                .systemPlayerId = 0x07734,
                .fixedSize = 48,
                .playerVersion = 14,
                .parentId = 0,
            },
            .spData = {
                .tcpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( tcpPort ),
                },
                .udpAddr =
                {
                    .sin_family = AF_INET,
                    .sin_port = htons( udpPort ),
                },
            },
        },
    };
#include "poppack.h"

    int wsResult;

    wsResult = send( sock, (char *) &reply, sizeof( reply ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( reply ), "send() returned %d.\n", wsResult );
}

#define receiveAddForwardAck( sock, expectedPlayerId ) receiveAddForwardAck_( __LINE__, sock, expectedPlayerId )
static unsigned short receiveAddForwardAck_( int line, SOCKET sock, DPID expectedPlayerId )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        AddForwardAck request;
    } request;
#include "poppack.h"
    unsigned short port;
    int wsResult;

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, sizeof( request ) );
    checkMessageHeader_( line, &request.request.header, 47 );

    return port;
}

#define receiveCreatePlayer( sock, expectedPlayerId, expectedFlags, expectedShortName, expectedLongName, \
                             expectedPlayerData, expectedPlayerDataSize ) \
        receiveCreatePlayer_( __LINE__, sock, expectedPlayerId, expectedFlags, expectedShortName, expectedLongName, \
                              expectedPlayerData, expectedPlayerDataSize )
static unsigned short receiveCreatePlayer_( int line, SOCKET sock, DPID expectedPlayerId, DWORD expectedFlags,
                                            const WCHAR *expectedShortName, const WCHAR *expectedLongName,
                                            void *expectedPlayerData, DWORD expectedPlayerDataSize )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        CreatePlayer request;
    } request;
#include "poppack.h"
    DWORD expectedShortNameSize;
    DWORD expectedLongNameSize;
    WCHAR shortName[ 256 ];
    BYTE playerData[ 256 ];
    WCHAR longName[ 256 ];
    unsigned short port;
    DWORD expectedSize;
    DWORD reserved2;
    WORD reserved1;
    SpData spData;
    int wsResult;

    expectedShortNameSize = expectedShortName ? (lstrlenW( expectedShortName ) + 1) * sizeof( WCHAR ) : 0;
    expectedLongNameSize = expectedLongName ? (lstrlenW( expectedLongName ) + 1) * sizeof( WCHAR ) : 0;
    expectedSize = sizeof( request ) + expectedShortNameSize + expectedLongNameSize + sizeof( spData )
                 + expectedPlayerDataSize + sizeof( reserved1 ) + sizeof( reserved2 );

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, expectedSize );
    checkMessageHeader_( line, &request.request.header, 8 );
    ok_( __FILE__, line )( !request.request.toId, "got destination id %#lx.\n", request.request.toId );
    ok_( __FILE__, line )( request.request.playerId == expectedPlayerId, "got player id %#lx.\n",
                           request.request.playerId );
    ok_( __FILE__, line )( !request.request.groupId, "got group id %#lx.\n", request.request.groupId );
    ok_( __FILE__, line )( request.request.createOffset == 28, "got create offset %lu.\n",
                           request.request.createOffset );
    ok_( __FILE__, line )( !request.request.passwordOffset, "got password offset %lu.\n",
                           request.request.passwordOffset );
    checkPackedPlayer_( line, &request.request.playerInfo, expectedFlags, expectedPlayerId, expectedShortNameSize,
                        expectedLongNameSize, expectedPlayerDataSize, 0x12345678 );

    if ( expectedShortName )
    {
        wsResult = receiveMessage_( line, sock, shortName, expectedShortNameSize );

        ok_( __FILE__, line )( wsResult == expectedShortNameSize, "recv() returned %d.\n", wsResult );
        ok_( __FILE__, line )( !lstrcmpW( shortName, expectedShortName ), "got short name %s.\n",
                               wine_dbgstr_w( shortName ) );
    }

    if ( expectedLongName )
    {
        wsResult = receiveMessage_( line, sock, longName, expectedLongNameSize );

        ok_( __FILE__, line )( wsResult == expectedLongNameSize, "recv() returned %d.\n", wsResult );
        ok_( __FILE__, line )( !lstrcmpW( longName, expectedLongName ), "got long name %s.\n",
                               wine_dbgstr_w( longName ) );
    }

    wsResult = receiveMessage_( line, sock, &spData, sizeof( spData ) );
    ok_( __FILE__, line )( wsResult == sizeof( spData ), "recv() returned %d.\n", wsResult );
    checkSpData_( line, &spData, NULL );

    if ( expectedPlayerDataSize )
    {
        wsResult = receiveMessage_( line, sock, playerData, expectedPlayerDataSize );

        ok_( __FILE__, line )( wsResult == expectedPlayerDataSize, "recv() returned %d.\n", wsResult );
        ok_( __FILE__, line )( !memcmp( playerData, expectedPlayerData, expectedPlayerDataSize ),
                               "player data didn't match.\n" );
    }

    wsResult = receiveMessage_( line, sock, &reserved1, sizeof( reserved1 ) );

    ok_( __FILE__, line )( wsResult == sizeof( reserved1 ), "recv() returned %d.\n", wsResult );
    ok_( __FILE__, line )( !reserved1, "got reserved1 %d.\n", reserved1 );

    wsResult = receiveMessage_( line, sock, &reserved2, sizeof( reserved2 ) );

    ok_( __FILE__, line )( wsResult == sizeof( reserved2 ), "recv() returned %d.\n", wsResult );
    ok_( __FILE__, line )( !reserved2, "got reserved2 %lu.\n", reserved2 );

    return port;
}

#define sendCreatePlayer( sock, tcpPort, udpPort, shortName, longName, playerData, playerDataSize ) \
        sendCreatePlayer_( __LINE__, sock, tcpPort, udpPort, shortName, longName, playerData, playerDataSize )
static void sendCreatePlayer_( int line, SOCKET sock, unsigned short tcpPort, unsigned short udpPort,
                               const WCHAR *shortName, const WCHAR *longName, void *playerData, DWORD playerDataSize )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        CreatePlayer request;
    } request =
    {
        .spHeader =
        {
            .mixed = 0xfab00000,
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( tcpPort ),
            },
        },
        .request =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 8,
                .version = 14,
            },
            .toId = 0,
            .playerId = 0x07734,
            .groupId = 0,
            .createOffset = 28,
            .passwordOffset = 0,
            .playerInfo =
            {
                .size = sizeof( request.request.playerInfo ) + sizeof( SpData ),
                .flags = 0x8,
                .id = 0x07734,
                .shortNameLength = 0,
                .longNameLength = 0,
                .spDataSize = sizeof( SpData ),
                .playerDataSize = 0,
                .playerCount = 0,
                .systemPlayerId = 0x51573,
                .fixedSize = 48,
                .playerVersion = 14,
                .parentId = 0,
            },
        },
    };
#include "poppack.h"
    SpData spData = {
        .tcpAddr.sin_family = AF_INET,
        .tcpAddr.sin_port = htons( tcpPort ),
        .udpAddr.sin_family = AF_INET,
        .udpAddr.sin_port = htons( udpPort ),
    };
    char reserved[ 6 ] = { 0 };
    DWORD shortNameSize = 0;
    DWORD longNameSize = 0;
    int wsResult;

    if ( shortName )
        shortNameSize = (wcslen( shortName ) + 1) * sizeof( WCHAR );

    if ( longName )
        longNameSize = (wcslen( longName ) + 1) * sizeof( WCHAR );

    request.spHeader.mixed += sizeof( request ) + shortNameSize + longNameSize + sizeof( spData )
                            + playerDataSize + sizeof( reserved );
    request.request.playerInfo.size += shortNameSize + longNameSize + playerDataSize;
    request.request.playerInfo.shortNameLength = shortNameSize;
    request.request.playerInfo.longNameLength = longNameSize;
    request.request.playerInfo.playerDataSize = playerDataSize;

    wsResult = send( sock, (char *) &request, sizeof( request ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "send() returned %d.\n", wsResult );

    if ( shortName )
    {
        wsResult = send( sock, (char *) shortName, shortNameSize, 0 );
        ok_( __FILE__, line )( wsResult == shortNameSize, "send() returned %d.\n", wsResult );
    }

    if ( longName )
    {
        wsResult = send( sock, (char *) longName, longNameSize, 0 );
        ok_( __FILE__, line )( wsResult == longNameSize, "send() returned %d.\n", wsResult );
    }

    wsResult = send( sock, (char *) &spData, sizeof( spData ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( spData ), "send() returned %d.\n", wsResult );

    if ( playerData )
    {
        wsResult = send( sock, playerData, playerDataSize, 0 );
        ok_( __FILE__, line )( wsResult == playerDataSize, "send() returned %d.\n", wsResult );
    }

    wsResult = send( sock, reserved, sizeof( reserved ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( reserved ), "send() returned %d.\n", wsResult );
}

#define receiveGuaranteedGameMessage( sock, expectedFromId, expectedToId, expectedData, expectedDataSize ) \
        receiveGuaranteedGameMessage_( __LINE__, sock, expectedFromId, expectedToId, expectedData, expectedDataSize )
static unsigned short receiveGuaranteedGameMessage_( int line, SOCKET sock, DPID expectedFromId, DPID expectedToId,
                                                     void *expectedData, DWORD expectedDataSize )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        GameMessage request;
        BYTE data[ 256 ];
    } request;
#include "poppack.h"
    unsigned short port;
    int wsResult;

    DWORD expectedSize = sizeof( request.spHeader ) + sizeof( request.request ) + expectedDataSize;

    wsResult = receiveMessage_( line, sock, &request, expectedSize );
    ok_( __FILE__, line )( wsResult == expectedSize, "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, expectedSize );
    checkGameMessage_( line, &request.request, expectedFromId, expectedToId );
    ok_( __FILE__, line )( !memcmp( &request.data, expectedData, expectedDataSize ), "message data didn't match.\n" );

    return port;
}

#define receiveGameMessage( sock, expectedFromId, expectedToId, expectedData, expectedDataSize ) \
        receiveGameMessage_( __LINE__, sock, expectedFromId, expectedToId, expectedData, expectedDataSize )
static void receiveGameMessage_( int line, SOCKET sock, DPID expectedFromId, DPID expectedToId, void *expectedData,
                                 DWORD expectedDataSize )
{
#include "pshpack1.h"
    struct
    {
        GameMessage request;
        BYTE data[ 256 ];
    } request;
#include "poppack.h"
    int wsResult;

    DWORD expectedSize = sizeof( request.request ) + expectedDataSize;

    wsResult = receiveMessage_( line, sock, &request, expectedSize );
    ok_( __FILE__, line )( wsResult == expectedSize, "recv() returned %d.\n", wsResult );

    checkGameMessage_( line, &request.request, expectedFromId, expectedToId );
    ok_( __FILE__, line )( !memcmp( &request.data, expectedData, expectedDataSize ), "message data didn't match.\n" );
}

#define sendGuaranteedGameMessage( sock, port, fromId, toId, data, dataSize ) \
        sendGuaranteedGameMessage_( __LINE__, sock, port, fromId, toId, data, dataSize )
static void sendGuaranteedGameMessage_( int line, SOCKET sock, unsigned short port, DPID fromId, DPID toId, void *data,
                                        DWORD dataSize )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        GameMessage request;
    } request =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( request ) + dataSize,
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .request =
        {
            .fromId = fromId,
            .toId = toId,
        }
    };
#include "poppack.h"
    int wsResult;

    wsResult = send( sock, (char *) &request, sizeof( request ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "send() returned %d.\n", wsResult );

    wsResult = send( sock, data, dataSize, 0 );
    ok_( __FILE__, line )( wsResult == dataSize, "send() returned %d.\n", wsResult );
}

#define sendGameMessage( sock, fromId, toId, data, dataSize ) \
        sendGameMessage_( __LINE__, sock, fromId, toId, data, dataSize )
static void sendGameMessage_( int line, SOCKET sock, DPID fromId, DPID toId, void *data, DWORD dataSize )
{
#include "pshpack1.h"
    struct
    {
        GameMessage request;
        BYTE data[ 256 ];
    } request =
    {
        .request =
        {
            .fromId = fromId,
            .toId = toId,
        }
    };
#include "poppack.h"
    int wsResult;
    DWORD size;

    size = sizeof( request.request ) + dataSize;

    memcpy( request.data, data, dataSize );

    wsResult = send( sock, (char *) &request, size, 0 );
    ok_( __FILE__, line )( wsResult == size, "send() returned %d.\n", wsResult );
}

#define sendPing( sock, port, fromId, tickCount ) \
        sendPing_( __LINE__, sock, port, fromId, tickCount )
static void sendPing_( int line, SOCKET sock, unsigned short port, DPID fromId, DWORD tickCount )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        Ping request;
    } request =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( request ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .request =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 22,
                .version = 14,
            },
            .fromId = fromId,
            .tickCount = tickCount,
        }
    };
#include "poppack.h"
    int wsResult;

    wsResult = send( sock, (char *) &request, sizeof( request ), 0 );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "send() returned %d.\n", wsResult );
}

#define receivePingReply( sock, expectedFromId, expectedTickCount ) \
        receivePingReply_( __LINE__, sock, expectedFromId, expectedTickCount )
static unsigned short receivePingReply_( int line, SOCKET sock, DPID expectedFromId, DWORD expectedTickCount )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        Ping request;
    } request;
#include "poppack.h"
    unsigned short port;
    int wsResult;

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, sizeof( request ) );
    checkMessageHeader_( line, &request.request.header, 23 );
    ok_( __FILE__, line )( request.request.fromId == expectedFromId, "got source id %#lx.\n", request.request.fromId );
    ok_( __FILE__, line )( request.request.tickCount == expectedTickCount, "got tick count %#lx.\n",
                           request.request.tickCount );

    return port;
}

#define receiveAddPlayerToGroup( sock, expectedPlayerId, expectedGroupId ) \
        receiveAddPlayerToGroup_( __LINE__, sock, expectedPlayerId, expectedGroupId )
static unsigned short receiveAddPlayerToGroup_( int line, SOCKET sock, DPID expectedPlayerId, DPID expectedGroupId )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        AddPlayerToGroup request;
    } request;
#include "poppack.h"
    unsigned short port;
    int wsResult;

    wsResult = receiveMessage_( line, sock, &request, sizeof( request ) );
    ok_( __FILE__, line )( wsResult == sizeof( request ), "recv() returned %d.\n", wsResult );

    port = checkSpHeader_( line, &request.spHeader, sizeof( request ) );
    checkMessageHeader_( line, &request.request.header, 13 );

    ok_( __FILE__, line )( !request.request.toId, "got destination id %#lx.\n", request.request.toId );
    ok_( __FILE__, line )( request.request.playerId == expectedPlayerId, "got player id %#lx.\n",
                           request.request.playerId );
    ok_( __FILE__, line )( request.request.groupId == expectedGroupId, "got group id %#lx.\n",
                           request.request.groupId );
    ok_( __FILE__, line )( !request.request.createOffset, "got create offset %lu.\n", request.request.createOffset );
    ok_( __FILE__, line )( !request.request.passwordOffset, "got password offset %lu.\n", request.request.passwordOffset );

    return port;
}

#define sendGroupDataChanged( sock, port, groupId, data, dataSize ) \
        sendGroupDataChanged_( __LINE__, sock, port, groupId, data, dataSize )
static void sendGroupDataChanged_( int line, SOCKET sock, unsigned short port, DPID groupId, void *data, DWORD dataSize )
{
#include "pshpack1.h"
    struct
    {
        SpHeader spHeader;
        GroupDataChanged request;
        BYTE data[ 256 ];
    } request =
    {
        .spHeader =
        {
            .mixed = 0xfab00000 + sizeof( request ),
            .addr =
            {
                .sin_family = AF_INET,
                .sin_port = htons( port ),
            },
        },
        .request =
        {
            .header =
            {
                .magic = 0x79616c70,
                .command = 17,
                .version = 14,
            },
            .toId = 0,
            .groupId = groupId,
            .dataSize = dataSize,
            .dataOffset = sizeof( request.request ),
        },
    };
#include "poppack.h"
    int wsResult;
    int size;

    size = sizeof( request ) + dataSize;
    memcpy( &request.data, data, dataSize );

    wsResult = send( sock, (char *) &request, size, 0 );
    ok_( __FILE__, line )( wsResult == size, "send() returned %d.\n", wsResult );
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
        pAddress = calloc( 1, dwAddressSize );
        hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                     pAddress, &dwAddressSize );
        checkHR( DP_OK, hr );
    }

    hr = IDirectPlayX_InitializeConnection( pDP, pAddress, 0 );
    checkHR( DP_OK, hr );

    free( pAddress );
    IDirectPlayLobby_Release(pDPL);
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
    hr = pDirectPlayCreate( NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = pDirectPlayCreate( (LPGUID) &GUID_NULL, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = pDirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, NULL, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );

    /* pUnk==NULL, pDP!=NULL */
    hr = pDirectPlayCreate( NULL, &pDP, NULL );
    checkHR( DPERR_INVALIDPARAMS, hr );
    hr = pDirectPlayCreate( (LPGUID) &GUID_NULL, &pDP, NULL );
    checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );
    hr = pDirectPlayCreate( (LPGUID) &DPSPGUID_TCPIP, &pDP, NULL );
    checkHR( DP_OK, hr );
    if ( hr == DP_OK )
        IDirectPlayX_Release( pDP );

}

static BOOL CALLBACK callback_providersA(GUID* guid, char *name, DWORD major, DWORD minor, void *arg)
{
    struct provider_data *prov = arg;

    if (!prov) return TRUE;

    if (prov->call_count < ARRAY_SIZE(prov->guid_data))
    {
        prov->guid_ptr[prov->call_count] = guid;
        prov->guid_data[prov->call_count] = *guid;

        prov->call_count++;
    }

    if (prov->ret_value) /* Only trace when looping all providers */
        trace("Provider #%d '%s' (%ld.%ld)\n", prov->call_count, name, major, minor);
    return prov->ret_value;
}

static BOOL CALLBACK callback_providersW(GUID* guid, WCHAR *name, DWORD major, DWORD minor, void *arg)
{
    struct provider_data *prov = arg;

    if (!prov) return TRUE;

    if (prov->call_count < ARRAY_SIZE(prov->guid_data))
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

    hr = pDirectPlayEnumerateA(callback_providersA, NULL);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateA failed\n");

    SetLastError(0xdeadbeef);
    hr = pDirectPlayEnumerateA(NULL, &arg);
    ok(FAILED(hr), "DirectPlayEnumerateA expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    hr = pDirectPlayEnumerateA(NULL, NULL);
    ok(FAILED(hr), "DirectPlayEnumerateA expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%lx\n", GetLastError());

    hr = pDirectPlayEnumerateA(callback_providersA, &arg);
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
    hr = pDirectPlayEnumerateA(callback_providersA, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateA failed\n");
    ok(arg.call_count == 1, "Expected 1, got %d\n", arg.call_count);

    hr = pDirectPlayEnumerateW(callback_providersW, NULL);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateW failed\n");

    SetLastError(0xdeadbeef);
    hr = pDirectPlayEnumerateW(NULL, &arg);
    ok(FAILED(hr), "DirectPlayEnumerateW expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    hr = pDirectPlayEnumerateW(NULL, NULL);
    ok(FAILED(hr), "DirectPlayEnumerateW expected to fail\n");
    ok(GetLastError() == 0xdeadbeef, "Expected 0xdeadbeef, got 0x%lx\n", GetLastError());

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = TRUE;
    hr = pDirectPlayEnumerateW(callback_providersW, &arg);
    ok(SUCCEEDED(hr), "DirectPlayEnumerateW failed\n");
    ok(arg.call_count > 0, "Expected at least one valid provider\n");

    /* The returned GUID values must have persisted after enumeration (bug 37185) */
    for(i = 0; i < arg.call_count; i++)
    {
        ok(IsEqualGUID(arg.guid_ptr[i], &arg.guid_data[i]), "#%d Expected equal GUID values\n", i);
    }

    memset(&arg, 0, sizeof(arg));
    arg.ret_value = FALSE;
    hr = pDirectPlayEnumerateW(callback_providersW, &arg);
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
        BOOL found = FALSE;
        int i;
        for( i=0; i < ARRAY_SIZE(sps) && !found; i++ )
            found = IsEqualGUID( sps[i], lpData );
        ok( found, "Unknown Address type found %s\n", wine_dbgstr_guid(lpData) );
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

    IDirectPlayLobby_Release(pDPL);

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
    ok( callbackData.dwCounter1 == 4 || callbackData.dwCounter1 == 3, "got=%d\n", callbackData.dwCounter1 );

    callbackData.dwCounter1 = 0;
    callbackData.dwFlags = 0;
    hr = IDirectPlayX_EnumConnections( pDP, NULL, EnumConnections_cb,
                                       &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    ok( callbackData.dwCounter1 == 4 || callbackData.dwCounter1 == 3, "got=%d\n", callbackData.dwCounter1 );

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
    ok( callbackData.dwCounter1 == 4 || callbackData.dwCounter1 == 3, "got=%d\n", callbackData.dwCounter1 );

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
    ok( callbackData.dwCounter1 == 4 || callbackData.dwCounter1 == 3, "got=%d\n", callbackData.dwCounter1 );

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
    checkHR( DPERR_INVALIDPARAMS, hr );

    dpcaps.dwSize = sizeof(DPCAPS);

    for (dwFlags=0;
         dwFlags<=DPGETCAPS_GUARANTEED;
         dwFlags+=DPGETCAPS_GUARANTEED)
    {

        hr = IDirectPlayX_GetCaps( pDP, &dpcaps, dwFlags );
        checkHR( DP_OK, hr );
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

    IDirectPlayX_Release( pDP );
}

static int enum_addresses_cb_count = 0;
static BOOL CALLBACK EnumAddressesCallback(REFGUID guidDataType, DWORD size, const void *data, void *context)
{
    if (IsEqualGUID(guidDataType, &DPAID_TotalSize))
        enum_addresses_cb_count++;
    else if (IsEqualGUID(guidDataType, &DPAID_ServiceProvider))
        enum_addresses_cb_count++;
    else if(IsEqualGUID(guidDataType, &invalid_guid))
        enum_addresses_cb_count++;
    else
        ok(0, "guidDataType %s\n", wine_dbgstr_guid(guidDataType));

    return TRUE;
}

static void test_EnumAddresses(void)
{
    IDirectPlay4 *pDP;
    HRESULT hr;
    DPCOMPOUNDADDRESSELEMENT addressElements[2];
    LPVOID pAddress = NULL;
    DWORD dwAddressSize = 0;
    IDirectPlayLobby3 *pDPL;
    WORD port = 6001;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_ALL,
                           &IID_IDirectPlay4A, (LPVOID*) &pDP );
    ok( SUCCEEDED(hr), "CCI of CLSID_DirectPlay / IID_IDirectPlay4A failed\n" );
    if (FAILED(hr))
        return;

    hr = CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                           &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );
    ok (SUCCEEDED (hr), "CCI of CLSID_DirectPlayLobby / IID_IDirectPlayLobby3A failed\n");
    if (FAILED (hr)) return;

    addressElements[0].guidDataType = DPAID_ServiceProvider;
    addressElements[0].dwDataSize   = sizeof(GUID);
    addressElements[0].lpData       = (void*) &DPSPGUID_TCPIP;

    addressElements[1].guidDataType = invalid_guid;
    addressElements[1].dwDataSize   = sizeof(WORD);
    addressElements[1].lpData       = &port;

    hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2, NULL, &dwAddressSize );
    checkHR( DPERR_BUFFERTOOSMALL, hr );

    if( hr == DPERR_BUFFERTOOSMALL )
    {
        pAddress = calloc( 1, dwAddressSize );
        hr = IDirectPlayLobby_CreateCompoundAddress( pDPL, addressElements, 2,
                                                     pAddress, &dwAddressSize );
        checkHR( DP_OK, hr );
    }

    enum_addresses_cb_count = 0;
    hr = IDirectPlayLobby_EnumAddress(pDPL, EnumAddressesCallback, pAddress, dwAddressSize, NULL);
    ok (SUCCEEDED (hr), "IDirectPlayLobby3A_EnumAddress %lx\n", hr);
    todo_wine ok (enum_addresses_cb_count == 3, "wrong count %d\n", enum_addresses_cb_count);

    IDirectPlayX_Close(pDP);
    IDirectPlayX_Release(pDP);
    IDirectPlayLobby_Release(pDPL);

    free( pAddress );
}

static int enum_address_cb_count = 0;
static BOOL CALLBACK EnumAddressTypeCallback(REFGUID guidDataType, LPVOID lpContext, DWORD flags)
{
    if (IsEqualGUID(guidDataType, &DPAID_INet))
        enum_address_cb_count++;
    else
        ok(0, "guidDataType %s\n", wine_dbgstr_guid(guidDataType));

    return TRUE;
}

static void test_EnumAddressTypes(void)
{
    HRESULT hr;
    IDirectPlayLobby3 *pDPL;

    hr = CoCreateInstance( &CLSID_DirectPlayLobby, NULL, CLSCTX_ALL,
                           &IID_IDirectPlayLobby3A, (LPVOID*) &pDPL );
    ok (SUCCEEDED (hr), "CCI of CLSID_DirectPlayLobby / IID_IDirectPlayLobby3A failed\n");
    if (FAILED (hr)) return;

    enum_address_cb_count = 0;
    hr = IDirectPlayLobby_EnumAddressTypes(pDPL, EnumAddressTypeCallback, &DPSPGUID_TCPIP, NULL, 0);
    ok (SUCCEEDED (hr), "IDirectPlayLobby3A_EnumAddress %lx\n", hr);
    todo_wine ok (enum_address_cb_count == 1, "IDirectPlayLobby3A_EnumAddress %lx\n", hr);

    IDirectPlayLobby_Release(pDPL);
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
        dpsd.lpszPasswordA = (LPSTR) "sonic boom";
        hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
        checkHR( DPERR_INVALIDPASSWORD, hr );

        /* Correct password */
        dpsd.lpszPasswordA = (LPSTR) "hadouken";
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

typedef struct
{
    DPID expectedDpid;
    DWORD expectedPlayerType;
    const char *expectedShortName;
    const char *expectedLongName;
    DWORD expectedFlags;
    BYTE *expectedPlayerData;
    DWORD expectedPlayerDataSize;
    int actualCount;
} ExpectedPlayer;

typedef struct
{
    int line;
    IDirectPlay4 *dp;
    ExpectedPlayer *expectedPlayers;
    int expectedPlayerCount;
    BOOL ignoreUnexpected;
    int actualPlayerCount;
} CheckPlayerListCallbackData;

static BOOL CALLBACK checkPlayerListCallback( DPID dpid, DWORD playerType, const DPNAME *name, DWORD flags,
                                              void *context )
{
    CheckPlayerListCallbackData *data = context;
    int i;

    for ( i = 0; i < data->expectedPlayerCount; ++i )
    {
        ExpectedPlayer *player = &data->expectedPlayers[ i ];
        if ( player->expectedDpid == dpid )
        {
            BYTE playerData[ 256 ];
            DWORD playerDataSize;
            BYTE nameData[ 256 ];
            DWORD nameDataSize;
            char *shortName;
            char *longName;
            HRESULT hr;

            if ( player->actualCount )
                ok_( __FILE__, data->line )( 0, "duplicate player dpid %#lx.\n", dpid );
            ok_( __FILE__, data->line )( playerType == player->expectedPlayerType, "got player type %lu.\n",
                                         playerType );
            if ( player->expectedShortName )
            {
                ok_( __FILE__, data->line )( name->lpszShortNameA && !strcmp( name->lpszShortNameA, player->expectedShortName ),
                                             "got short name %s.\n", wine_dbgstr_a( name->lpszShortNameA ) );
            }
            else
            {
                ok_( __FILE__, data->line )( !name->lpszShortNameA, "got short name %s.\n",
                                             wine_dbgstr_a( name->lpszShortNameA ) );
            }
            if ( player->expectedLongName )
            {
                ok_( __FILE__, data->line )( name->lpszLongNameA && !strcmp( name->lpszLongNameA, player->expectedLongName ),
                                             "got long name %s.\n", wine_dbgstr_a( name->lpszLongNameA ) );
            }
            else
            {
                ok_( __FILE__, data->line )( !name->lpszLongNameA, "got long name %s.\n",
                                             wine_dbgstr_a( name->lpszLongNameA ) );
            }
            ok_( __FILE__, data->line )( flags == player->expectedFlags, "got flags %#lx.\n", flags );

            memset( &playerData, 0xcc, sizeof( playerData ) );
            playerDataSize = sizeof( playerData );
            if ( playerType == DPPLAYERTYPE_PLAYER )
                hr = IDirectPlayX_GetPlayerData( data->dp, dpid, playerData, &playerDataSize, DPGET_REMOTE );
            else
                hr = IDirectPlayX_GetGroupData( data->dp, dpid, playerData, &playerDataSize, DPGET_REMOTE );
            ok_( __FILE__, data->line )( hr == DP_OK, "GetPlayerData() returned %#lx.\n", hr );
            ok_( __FILE__, data->line )( playerDataSize == player->expectedPlayerDataSize,
                                         "got player data size %lu.\n", playerDataSize );
            ok_( __FILE__, data->line )( !memcmp( playerData, player->expectedPlayerData, player->expectedPlayerDataSize ),
                                         "player data doesn't match.\n" );

            memset( &nameData, 0xcc, sizeof( nameData ) );
            nameDataSize = sizeof( nameData );
            if ( playerType == DPPLAYERTYPE_PLAYER )
                hr = IDirectPlayX_GetPlayerName( data->dp, dpid, &nameData, &nameDataSize );
            else
                hr = IDirectPlayX_GetGroupName( data->dp, dpid, &nameData, &nameDataSize );
            ok_( __FILE__, data->line )( hr == DP_OK, "GetPlayerName() returned %#lx.\n", hr );
            ok_( __FILE__, data->line )( ((DPNAME *) nameData)->dwSize == sizeof( DPNAME ),
                                         "got name size %lu.\n", ((DPNAME *) nameData)->dwSize );
            ok_( __FILE__, data->line )( !((DPNAME *) nameData)->dwFlags, "got name flags %#lx.\n",
                                         ((DPNAME *) nameData)->dwFlags );
            shortName = ((DPNAME *) nameData)->lpszShortNameA;
            if ( player->expectedShortName )
            {
                if ( (char *) nameData <= shortName && shortName < (char *) nameData + nameDataSize )
                {
                    ok_( __FILE__, data->line )( shortName && !strcmp( shortName, player->expectedShortName ),
                                                 "got short name %s.\n", wine_dbgstr_a( shortName ) );
                }
                else
                {
                    ok_( __FILE__, data->line)( 0, "got short name %p.\n", shortName );
                }
            }
            else
            {
                ok_( __FILE__, data->line )( !shortName, "got short name %s.\n", wine_dbgstr_a( shortName ) );
            }
            longName = ((DPNAME *) nameData)->lpszLongNameA;
            if ( player->expectedLongName )
            {
                if ( (char *) nameData <= longName && longName < (char *) nameData + nameDataSize )
                {
                    ok_( __FILE__, data->line )( longName && !strcmp( longName, player->expectedLongName ),
                                                 "got long name %s.\n", wine_dbgstr_a( longName ) );
                }
                else
                {
                    ok_( __FILE__, data->line)( 0, "got long name %p.\n", longName );
                }
            }
            else
            {
                ok_( __FILE__, data->line )( !longName, "got long name %s.\n", wine_dbgstr_a( longName ) );
            }

            ++player->actualCount;
            ++data->actualPlayerCount;

            return TRUE;
        }
    }

    if ( !data->ignoreUnexpected )
        ok_( __FILE__, data->line )( 0, "unexpected player dpid %#lx.\n", dpid );

    ++data->actualPlayerCount;

    return TRUE;
}

#define checkPlayerList( dp, expectedPlayers, expectedPlayerCount ) \
        checkPlayerList_( __LINE__, dp, expectedPlayers, expectedPlayerCount )
static void checkPlayerList_( int line, IDirectPlay4 *dp, ExpectedPlayer *expectedPlayers, int expectedPlayerCount )
{
    CheckPlayerListCallbackData data = {
        .line = line,
        .dp = dp,
        .expectedPlayers = expectedPlayers,
        .expectedPlayerCount = expectedPlayerCount,
    };
    HRESULT hr;

    hr = IDirectPlayX_EnumPlayers( dp, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_LOCAL );
    ok_( __FILE__, line )( hr == DP_OK, "EnumPlayers() returned %#lx.\n", hr );

    hr = IDirectPlayX_EnumPlayers( dp, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_REMOTE );
    ok_( __FILE__, line )( hr == DP_OK, "EnumPlayers() returned %#lx.\n", hr );

    ok_( __FILE__, line )( data.actualPlayerCount == data.expectedPlayerCount, "got player count %d.\n",
                           data.actualPlayerCount );
}

#define checkGroupPlayerList( dp, group, expectedPlayers, expectedPlayerCount ) \
        checkGroupPlayerList_( __LINE__, dp, group, expectedPlayers, expectedPlayerCount )
static void checkGroupPlayerList_( int line, DPID group, IDirectPlay4 *dp, ExpectedPlayer *expectedPlayers,
                                   int expectedPlayerCount )
{
    CheckPlayerListCallbackData data = {
        .line = line,
        .dp = dp,
        .expectedPlayers = expectedPlayers,
        .expectedPlayerCount = expectedPlayerCount,
    };
    HRESULT hr;

    hr = IDirectPlayX_EnumGroupPlayers( dp, group, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_LOCAL );
    ok_( __FILE__, line )( hr == DP_OK, "EnumGroupPlayers() returned %#lx.\n", hr );

    hr = IDirectPlayX_EnumGroupPlayers( dp, group, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_REMOTE );
    ok_( __FILE__, line )( hr == DP_OK, "EnumGroupPlayers() returned %#lx.\n", hr );

    ok_( __FILE__, line )( data.actualPlayerCount == data.expectedPlayerCount, "got player count %d.\n",
                           data.actualPlayerCount );
}

#define checkGroupList( dp, expectedGroups, expectedGroupCount ) \
        checkGroupList_( __LINE__, dp, expectedGroups, expectedGroupCount )
static void checkGroupList_( int line, IDirectPlay4 *dp, ExpectedPlayer *expectedGroups, int expectedGroupCount )
{
    CheckPlayerListCallbackData data = {
        .line = line,
        .dp = dp,
        .expectedPlayers = expectedGroups,
        .expectedPlayerCount = expectedGroupCount,
    };
    HRESULT hr;

    hr = IDirectPlayX_EnumGroups( dp, NULL, checkPlayerListCallback, &data, DPENUMGROUPS_LOCAL );
    ok_( __FILE__, line )( hr == DP_OK, "EnumGroups() returned %#lx.\n", hr );

    hr = IDirectPlayX_EnumGroups( dp, NULL, checkPlayerListCallback, &data, DPENUMGROUPS_REMOTE );
    ok_( __FILE__, line )( hr == DP_OK, "EnumGroups() returned %#lx.\n", hr );

    ok_( __FILE__, line )( data.actualPlayerCount == data.expectedPlayerCount, "got group count %d.\n",
                           data.actualPlayerCount );
}

#define checkPlayerExists( dp, expectedDpid, expectedPlayerType, expectedShortName, expectedLongName, expectedFlags, \
                           expectedPlayerData, expectedPlayerDataSize ) \
        checkPlayerExists_( __LINE__, dp, expectedDpid, expectedPlayerType, expectedShortName, expectedLongName, \
                            expectedFlags, expectedPlayerData, expectedPlayerDataSize )
static void checkPlayerExists_( int line, IDirectPlay4 *dp, DPID expectedDpid, DWORD expectedPlayerType,
                               const char *expectedShortName, const char *expectedLongName, DWORD expectedFlags,
                               BYTE *expectedPlayerData, DWORD expectedPlayerDataSize )
{
    ExpectedPlayer expectedPlayer =
    {
        .expectedDpid = expectedDpid,
        .expectedPlayerType = expectedPlayerType,
        .expectedShortName = expectedShortName,
        .expectedLongName = expectedLongName,
        .expectedFlags = expectedFlags,
        .expectedPlayerData = expectedPlayerData,
        .expectedPlayerDataSize = expectedPlayerDataSize,
    };
    CheckPlayerListCallbackData data =
    {
        .line = line,
        .dp = dp,
        .expectedPlayers = &expectedPlayer,
        .expectedPlayerCount = 1,
        .ignoreUnexpected = TRUE,
    };
    HRESULT hr;

    hr = IDirectPlayX_EnumPlayers( dp, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_LOCAL );
    ok_( __FILE__, line )( hr == DP_OK, "EnumPlayers() returned %#lx.\n", hr );

    hr = IDirectPlayX_EnumPlayers( dp, NULL, checkPlayerListCallback, &data, DPENUMPLAYERS_REMOTE );
    ok_( __FILE__, line )( hr == DP_OK, "EnumPlayers() returned %#lx.\n", hr );

    ok_( __FILE__, line )( expectedPlayer.actualCount == 1, "got player count %d.\n", expectedPlayer.actualCount );
}

#define check_Open( dp, dpsd, serverDpsd, idRequestExpected, forwardRequestExpected, listenPort, expectedPassword, \
                    idReplyHr, addForwardReplyHr, expectedHr ) \
        check_Open_( __LINE__, dp, dpsd, serverDpsd, idRequestExpected, forwardRequestExpected, listenPort, expectedPassword, \
                     idReplyHr, addForwardReplyHr, expectedHr )
static void check_Open_( int line, IDirectPlay4A *dp, DPSESSIONDESC2 *dpsd, const DPSESSIONDESC2 *serverDpsd,
                         BOOL idRequestExpected, BOOL forwardRequestExpected, unsigned short listenPort,
                         const WCHAR *expectedPassword, HRESULT idReplyHr, HRESULT addForwardReplyHr,
                         HRESULT expectedHr )
{
    SOCKET listenSock;
    OpenParam *param;
    WSADATA wsaData;
    SOCKET recvSock;
    SOCKET sendSock;
    int wsResult;
    HRESULT hr;

    wsResult = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
    ok_( __FILE__, line )( !wsResult, "WSAStartup() returned %d.\n", wsResult );

    listenSock = listenTcp_( line, listenPort );

    param = openAsync( dp, dpsd, DPOPEN_JOIN );

    if ( idRequestExpected )
    {
        unsigned short port;

        recvSock = acceptTcp_( line, listenSock );
        ok_( __FILE__, line )( recvSock != INVALID_SOCKET, "accept() returned %#Ix.\n", recvSock );

        port = receiveRequestPlayerId_( line, recvSock, 0x9 );

        sendSock = connectTcp_( line, port );

        sendRequestPlayerReply_( line, sendSock, listenPort, 0x12345678, idReplyHr );

        if ( forwardRequestExpected )
        {
            receiveAddForwardRequest_( line, recvSock, 0x12345678, expectedPassword, serverDpsd->dwReserved1, NULL );

            if ( addForwardReplyHr == DP_OK )
                sendSuperEnumPlayersReply_( line, sendSock, listenPort, 2399, serverDpsd, L"normal" );
            else
                sendAddForwardReply_( line, sendSock, listenPort, addForwardReplyHr );

            hr = openAsyncWait( param, 7000 );
            ok_( __FILE__, line )( hr == expectedHr, "Open() returned %#lx.\n", hr );

            checkNoMoreMessages_( line, recvSock );

            if ( hr == DP_OK )
            {
                BYTE expectedPlayerData[] = { 1, 2, 3, 4, };
                ExpectedPlayer expectedPlayers[] = {
                    {
                        .expectedDpid = 0x1337,
                        .expectedPlayerType = DPPLAYERTYPE_PLAYER,
                        .expectedShortName = "short name",
                        .expectedLongName = "long name",
                        .expectedFlags = DPENUMPLAYERS_REMOTE,
                        .expectedPlayerData = expectedPlayerData,
                        .expectedPlayerDataSize = sizeof( expectedPlayerData ),
                    },
                    {
                        .expectedDpid = 0xd00de,
                        .expectedPlayerType = DPPLAYERTYPE_PLAYER,
                        .expectedFlags = DPENUMPLAYERS_REMOTE,
                    },
                };
                ExpectedPlayer expectedGroups[] =
                {
                    {
                        .expectedDpid = 0x5e7,
                        .expectedPlayerType = DPPLAYERTYPE_GROUP,
                        .expectedShortName = "short name",
                        .expectedLongName = "long name",
                        .expectedFlags = DPENUMPLAYERS_REMOTE,
                        .expectedPlayerData = expectedPlayerData,
                        .expectedPlayerDataSize = sizeof( expectedPlayerData ),
                    },
                };
                ExpectedPlayer expectedGroupPlayers[] =
                {
                    {
                        .expectedDpid = 0xd00de,
                        .expectedPlayerType = DPPLAYERTYPE_PLAYER,
                        .expectedFlags = DPENUMPLAYERS_REMOTE,
                    },
                };

                checkPlayerList_( line, dp, expectedPlayers, ARRAYSIZE( expectedPlayers ) );
                checkGroupList_( line, dp, expectedGroups, ARRAYSIZE( expectedGroups ) );
                checkGroupPlayerList_( line, 0x5e7, dp, expectedGroupPlayers, ARRAYSIZE( expectedGroupPlayers ) );

                hr = IDirectPlayX_Close( dp );
                checkHR( DP_OK, hr );
            }
        }
        else
        {
            hr = openAsyncWait( param, 7000 );
            ok_( __FILE__, line )( hr == expectedHr, "Open() returned %#lx.\n", hr );
        }
        closesocket( sendSock );
        closesocket( recvSock );
    }
    else
    {
        hr = openAsyncWait( param, 7000 );
        ok_( __FILE__, line )( hr == expectedHr, "Open() returned %#lx.\n", hr );
    }

    checkNoMoreAccepts_( line, listenSock );

    closesocket( listenSock );
    WSACleanup();
}

static BOOL CALLBACK countSessionsCallback( const DPSESSIONDESC2 *thisSd,
                                            DWORD *timeout,
                                            DWORD flags,
                                            void *context )
{
    int *count = context;

    if (flags & DPESC_TIMEDOUT)
        return FALSE;

    ++*count;

    return TRUE;
}

static void test_Open(void)
{
    DPSESSIONDESC2 dpsdZero =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
    };
    DPSESSIONDESC2 dpsdAppGuid =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidInstance = appGuid,
        .guidApplication = appGuid,
    };
    DPSESSIONDESC2 normalDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .dwMaxPlayers = 10,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    DPSESSIONDESC2 protectedDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .dwFlags = DPSESSION_PASSWORDREQUIRED,
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .dwMaxPlayers = 10,
        .lpszSessionName = (WCHAR *) L"protected",
        .lpszPassword = (WCHAR *) L"hadouken",
        .dwReserved1 = 0xaabbccdd,
    };
    EnumSessionsParam *enumSessionsParam;
    DPSESSIONDESC2 replyDpsd;
    DPSESSIONDESC2 dpsd;
    unsigned short port;
    IDirectPlay4 *dp;
    SOCKET enumSock;
    int tryIndex;
    SOCKET sock;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    dpsd = dpsdZero;
    dpsd.dwSize = 0;
    check_Open( dp, &dpsd, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_INVALIDPARAMS );

    check_Open( dp, &dpsdZero, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_UNINITIALIZED );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    /* Joining sessions */
    /* - Checking how strict dplay is with sizes */
    dpsd = dpsdZero;
    dpsd.dwSize = 0;
    check_Open( dp, &dpsd, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_INVALIDPARAMS );

    dpsd = dpsdZero;
    dpsd.dwSize = sizeof( DPSESSIONDESC2 ) - 1;
    check_Open( dp, &dpsd, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_INVALIDPARAMS );

    dpsd = dpsdZero;
    dpsd.dwSize = sizeof( DPSESSIONDESC2 ) + 1;
    check_Open( dp, &dpsd, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_INVALIDPARAMS );

    check_Open( dp, &dpsdZero, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_NOSESSIONS );

    check_Open( dp, &dpsdAppGuid, NULL, FALSE, FALSE, 2349, NULL, DP_OK, DP_OK, DPERR_NOSESSIONS );

    enumSock = bindUdp( 47624 );

    /* Join to normal session */
    for ( tryIndex = 0; ; ++tryIndex )
    {
        int count = 0;

        enumSessionsParam = enumSessionsAsync( dp, &dpsdAppGuid, 100, countSessionsCallback, &count, 0 );

        port = receiveEnumSessionsRequest( enumSock, &appGuid, NULL, 0 );

        sock = connectTcp( port );

        sendEnumSessionsReply( sock, 2349, &normalDpsd );

        replyDpsd = normalDpsd;
        replyDpsd.guidInstance = appGuid2;
        sendEnumSessionsReply( sock, 2348, &replyDpsd );

        hr = enumSessionsAsyncWait( enumSessionsParam, 2000 );
        checkHR( DP_OK, hr );

        closesocket( sock );

        if ( tryIndex < 19 && count < 2 )
            continue;

        ok( count == 2, "got session count %d.\n", count );

        break;
    }

    check_Open( dp, &dpsdAppGuid, &normalDpsd, TRUE, FALSE, 2349, L"", DPERR_CANTCREATEPLAYER, DP_OK,
                DPERR_CANTCREATEPLAYER );

    check_Open( dp, &dpsdAppGuid, &normalDpsd, TRUE, TRUE, 2349, L"", DP_OK, DP_OK, DP_OK );

    dpsd = dpsdAppGuid;
    dpsd.guidInstance = appGuid2;
    replyDpsd = normalDpsd;
    replyDpsd.guidInstance = appGuid2;
    check_Open( dp, &dpsd, &replyDpsd, TRUE, TRUE, 2348, L"", DP_OK, DP_OK, DP_OK );

    /* Join to protected session */
    for ( tryIndex = 0; ; ++tryIndex )
    {
        int count = 0;

        enumSessionsParam = enumSessionsAsync( dp, &dpsdAppGuid, 100, countSessionsCallback, &count,
                                               DPENUMSESSIONS_PASSWORDREQUIRED );

        port = receiveEnumSessionsRequest( enumSock, &appGuid, NULL, DPENUMSESSIONS_PASSWORDREQUIRED );

        sock = connectTcp( port );

        sendEnumSessionsReply( sock, 2349, &protectedDpsd );

        hr = enumSessionsAsyncWait( enumSessionsParam, 2000 );
        checkHR( DP_OK, hr );

        closesocket( sock );

        if ( tryIndex < 19 && count < 1 )
            continue;

        ok( count == 1, "got session count %d.\n", count );

        break;
    }

    dpsd = dpsdAppGuid;
    dpsd.lpszPasswordA = (char *) "hadouken";
    check_Open( dp, &dpsd, &protectedDpsd, TRUE, TRUE, 2349, L"hadouken", DP_OK, DP_OK, DP_OK );

    dpsd = dpsdAppGuid;
    dpsd.lpszPasswordA = (char *) "sonic boom";
    check_Open( dp, &dpsd, &protectedDpsd, TRUE, TRUE, 2349, L"sonic boom", DP_OK, DPERR_INVALIDPASSWORD,
                DPERR_INVALIDPASSWORD );

    closesocket( enumSock );

    IDirectPlayX_Release( dp );
}

static void test_interactive_Open(void)
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
    todo_wine checkHR( DP_OK, hr );

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
    checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)-1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd.dwSize = sizeof(DPSESSIONDESC2)+1;
    hr = IDirectPlayX_Open( pDP, &dpsd, DPOPEN_JOIN );
    checkHR( DPERR_INVALIDPARAMS, hr );

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
    checkHR( DPERR_INVALIDPARAMS, hr );

    dpsd_server.dwSize = sizeof(DPSESSIONDESC2);


    /* Join to protected session */
    IDirectPlayX_Close( pDP_server );
    dpsd_server.lpszPasswordA = (LPSTR) "hadouken";
    hr = IDirectPlayX_Open( pDP_server, &dpsd_server, DPOPEN_CREATE );
    todo_wine checkHR( DP_OK, hr );

    IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb2,
                               pDP, DPENUMSESSIONS_PASSWORDREQUIRED );


    IDirectPlayX_Release( pDP );
    IDirectPlayX_Release( pDP_server );

}

#define joinSession( dp, dpsd, serverDpsd, sendSock, recvSock, udpPort ) \
        joinSession_( __LINE__, dp, dpsd, serverDpsd, sendSock, recvSock, udpPort )
static void joinSession_( int line, IDirectPlay4 *dp, DPSESSIONDESC2 *dpsd, DPSESSIONDESC2 *serverDpsd,
                          SOCKET *sendSock, SOCKET *recvSock, unsigned short *udpPort )
{
    EnumSessionsParam *enumSessionsParam;
    OpenParam *openParam;
    unsigned short port;
    SOCKET listenSock;
    SOCKET enumSock;
    int tryIndex;
    HRESULT hr;

    enumSock = bindUdp_( line, 47624 );

    listenSock = listenTcp_( line, 2349 );

    for ( tryIndex = 0; ; ++tryIndex )
    {
        int count = 0;

        enumSessionsParam = enumSessionsAsync( dp, dpsd, 100, countSessionsCallback, &count, 0 );

        port = receiveEnumSessionsRequest_( line, enumSock, &appGuid, NULL, 0 );

        *sendSock = connectTcp_( line, port );

        sendEnumSessionsReply_( line, *sendSock, 2349, serverDpsd );

        hr = enumSessionsAsyncWait( enumSessionsParam, 2000 );
        ok_( __FILE__, line )( hr == DP_OK, "EnumSessions() returned %#lx.\n", hr );

        if ( tryIndex < 19 && count < 1 )
            continue;

        ok( count == 1, "got session count %d.\n", count );

        break;
    }

    openParam = openAsync( dp, dpsd, DPOPEN_JOIN );

    *recvSock = acceptTcp_( line, listenSock );
    ok_( __FILE__, line )( *recvSock != INVALID_SOCKET, "accept() returned %#Ix.\n", *recvSock );

    receiveRequestPlayerId_( line, *recvSock, 0x9 );
    sendRequestPlayerReply_( line, *sendSock, 2349, 0x12345678, DP_OK );
    receiveAddForwardRequest_( line, *recvSock, 0x12345678, L"", serverDpsd->dwReserved1, udpPort );
    sendSuperEnumPlayersReply_( line, *sendSock, 2349, 2399, serverDpsd, L"normal" );
    checkNoMoreMessages_( line, *recvSock );

    checkNoMoreAccepts_( line, listenSock );

    hr = openAsyncWait( openParam, 2000 );
    ok_( __FILE__, line )( hr == DP_OK, "Open() returned %#lx.\n", hr );

    closesocket( listenSock );
    closesocket( enumSock );
}

static void test_ADDFORWARD(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidInstance = appGuid,
        .guidApplication = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    IDirectPlay4A *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    sendAddForward( sendSock, 2349, 2348, 2398 );
    receiveAddForwardAck( recvSock, 0x07734 );
    checkNoMoreMessages( recvSock );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
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


    if ( lpThisSD->lpszPasswordA != NULL )
    {
        check( TRUE, (lpThisSD->dwFlags & DPSESSION_PASSWORDREQUIRED) != 0 );
    }

    if ( lpThisSD->dwFlags & DPSESSION_NEWPLAYERSDISABLED )
    {
        check( 0, lpThisSD->dwCurrentPlayers );
    }

    check( sizeof(*lpThisSD), lpThisSD->dwSize );
    checkLP( NULL, lpThisSD->lpszPasswordA );

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
        name.lpszShortNameA = (LPSTR) "bofh";

        hr = IDirectPlayX_CreatePlayer( pDP, &dpid, &name, NULL, NULL,
                                        0, DPPLAYER_SERVERPLAYER );
        todo_wine checkHR( DP_OK, hr );
    }

    return pDP;

}

typedef struct
{
    DPSESSIONDESC2 dpsd;
    int actualCount;
} ExpectedSession;

typedef struct
{
    int line;
    ExpectedSession *expectedSessions;
    int expectedCount;
    int actualCount;
    int timeoutCount;
    BOOL ansi;
} CheckSessionListCallbackData;

static BOOL CALLBACK checkSessionListCallback( const DPSESSIONDESC2 *thisSd, DWORD *timeout, DWORD flags, void *context )
{
    CheckSessionListCallbackData *data = context;
    ExpectedSession *expectedSession;
    int i;

    if ( flags & DPESC_TIMEDOUT )
    {
        ++data->timeoutCount;

        return FALSE;
    }

    ++data->actualCount;

    for ( i = 0; i < data->expectedCount; ++i )
    {
        expectedSession = &data->expectedSessions[ i ];
        if ( !IsEqualGUID( &expectedSession->dpsd.guidInstance, &thisSd->guidInstance ) )
            continue;

        ok_( __FILE__, data->line )( !expectedSession->actualCount, "duplicate session %s.\n",
                                     wine_dbgstr_guid( &thisSd->guidInstance ) );

        ok_( __FILE__, data->line )( thisSd->dwSize == expectedSession->dpsd.dwSize, "got size %lu.\n",
                                     thisSd->dwSize );
        ok_( __FILE__, data->line )( thisSd->dwFlags == expectedSession->dpsd.dwFlags, "got flags %#lx.\n",
                                     thisSd->dwFlags );
        ok_( __FILE__, data->line )( IsEqualGUID( &thisSd->guidApplication, &expectedSession->dpsd.guidApplication ),
                                     "got application GUID %s.\n", wine_dbgstr_guid( &thisSd->guidApplication ) );
        ok_( __FILE__, data->line )( thisSd->dwMaxPlayers == expectedSession->dpsd.dwMaxPlayers,
                                     "got current player count %lu.\n", thisSd->dwMaxPlayers );
        ok_( __FILE__, data->line )( thisSd->dwCurrentPlayers == expectedSession->dpsd.dwCurrentPlayers,
                                     "got max player count %lu.\n", thisSd->dwCurrentPlayers );
        ok_( __FILE__, data->line )( !strcmpAW( thisSd->lpszSessionNameA, expectedSession->dpsd.lpszSessionNameA, data->ansi ),
                                     "got session name %s.\n", dbgStrAW( thisSd->lpszSessionNameA, data->ansi ) );
        ok_( __FILE__, data->line )( !thisSd->lpszPasswordA, "got password %s.\n",
                                     dbgStrAW( thisSd->lpszPasswordA, data->ansi ) );
        ok_( __FILE__, data->line )( thisSd->dwReserved1 == expectedSession->dpsd.dwReserved1, "got reserved1 %#lx.\n",
                                     thisSd->dwReserved1 );
        ok_( __FILE__, data->line )( thisSd->dwReserved2 == expectedSession->dpsd.dwReserved2, "got reserved2 %#lx.\n",
                                     thisSd->dwReserved2 );
        ok_( __FILE__, data->line )( thisSd->dwUser1 == expectedSession->dpsd.dwUser1, "got user1 %#lx.\n",
                                     thisSd->dwUser1 );
        ok_( __FILE__, data->line )( thisSd->dwUser2 == expectedSession->dpsd.dwUser2, "got user2 %#lx.\n",
                                     thisSd->dwUser2 );
        ok_( __FILE__, data->line )( thisSd->dwUser3 == expectedSession->dpsd.dwUser3, "got user3 %#lx.\n",
                                     thisSd->dwUser3 );
        ok_( __FILE__, data->line )( thisSd->dwUser4 == expectedSession->dpsd.dwUser4, "got user4 %#lx.\n",
                                     thisSd->dwUser4 );

        ++expectedSession->actualCount;

        return TRUE;
    }

    ok_( __FILE__, data->line )( 0, "unexpected session %s.\n", wine_dbgstr_guid( &thisSd->guidInstance ) );

    return TRUE;
}

#define check_EnumSessions( dp, dpsd, flags, expectedHr, expectedSessionCount, timeoutExpected, \
                            requestExpected, expectedPassword, replyCount, ansi, hrTodo ) \
        check_EnumSessions_( __LINE__, dp, dpsd, flags, expectedHr, expectedSessionCount, timeoutExpected, \
                             requestExpected, expectedPassword, replyCount, ansi, hrTodo )
static void check_EnumSessions_( int line, IDirectPlay4 *dp, DPSESSIONDESC2 *dpsd, DWORD flags, HRESULT expectedHr,
                                 DWORD expectedSessionCount, BOOL timeoutExpected, BOOL requestExpected,
                                 const WCHAR *expectedPassword, DWORD replyCount, BOOL ansi, BOOL hrTodo )
{
    DPSESSIONDESC2 replyDpsds[] =
    {
        {
            .dwSize = sizeof( DPSESSIONDESC2 ),
            .dwFlags = 0,
            .guidInstance = appGuid,
            .guidApplication = appGuid,
            .dwMaxPlayers = 10,
            .dwCurrentPlayers = 0,
            .lpszSessionName = (WCHAR *) L"normal",
            .dwReserved1 = 0x11223344,
            .dwReserved2 = 0,
            .dwUser1 = 1,
            .dwUser2 = 2,
            .dwUser3 = 3,
            .dwUser4 = 4,
        },
        {
            .dwSize = sizeof( DPSESSIONDESC2 ),
            .dwFlags = DPSESSION_JOINDISABLED | DPSESSION_PASSWORDREQUIRED | DPSESSION_PRIVATE,
            .guidInstance = appGuid2,
            .guidApplication = appGuid2,
            .dwMaxPlayers = 10,
            .dwCurrentPlayers = 10,
            .lpszSessionName = (WCHAR *) L"private",
            .dwReserved1 = 0xaabbccdd,
            .dwReserved2 = 0,
            .dwUser1 = 5,
            .dwUser2 = 6,
            .dwUser3 = 7,
            .dwUser4 = 8,
        },
    };
    ExpectedSession expectedSessions[ ARRAYSIZE( replyDpsds ) ];
    CheckSessionListCallbackData callbackData;
    EnumSessionsParam *param;
    unsigned short port = 0;
    WSADATA wsaData;
    SOCKET enumSock;
    int tryIndex;
    int wsResult;
    SOCKET sock;
    HRESULT hr;
    int i;

    wsResult = WSAStartup( MAKEWORD( 2, 0 ), &wsaData );
    ok_( __FILE__, line )( !wsResult, "WSAStartup() returned %d.\n", wsResult );

    enumSock = bindUdp_( line, 47624 );

    for ( tryIndex = 0; ; ++tryIndex )
    {
        memset( &expectedSessions, 0, sizeof( expectedSessions ) );
        expectedSessions[ 0 ].dpsd = replyDpsds [ 0 ];
        expectedSessions[ 0 ].dpsd.lpszSessionNameA = (char *) AW( "normal", ansi );
        expectedSessions[ 1 ].dpsd = replyDpsds [ 1 ];
        expectedSessions[ 1 ].dpsd.lpszSessionNameA = (char *) AW( "private", ansi );

        memset( &callbackData, 0, sizeof( callbackData ) );
        callbackData.line = line;
        callbackData.expectedSessions = expectedSessions;
        callbackData.expectedCount = expectedSessionCount;
        callbackData.ansi = ansi;

        param = enumSessionsAsync( dp, dpsd, 100, checkSessionListCallback, &callbackData, flags );

        if ( requestExpected )
            port = receiveEnumSessionsRequest_( line, enumSock, &appGuid, expectedPassword, flags );

        for ( i = 0; i < replyCount; ++i )
        {
            sock = connectTcp_( line, port );
            if ( sock == INVALID_SOCKET )
                continue;

            sendEnumSessionsReply_( line, sock, 2349 - i, &replyDpsds[ i ] );

            closesocket( sock );
        }

        checkNoMoreMessages_( line, enumSock );

        hr = enumSessionsAsyncWait( param, 2000 );
        todo_wine_if( hrTodo ) ok_( __FILE__, line )( hr == expectedHr, "got hr %#lx.\n", hr );

        if ( tryIndex < 19 && callbackData.actualCount < callbackData.expectedCount )
            continue;

        ok_( __FILE__, line )( callbackData.actualCount == callbackData.expectedCount, "got session count %d.\n",
                               callbackData.actualCount );
        ok_( __FILE__, line )( !!callbackData.timeoutCount == timeoutExpected, "got timeout count %d.\n",
                               callbackData.timeoutCount );

        break;
    }

    closesocket( enumSock );
    WSACleanup();
}

#define check_EnumSessions_async( dpsd, dp, ansi ) check_EnumSessions_async_( __LINE__, dpsd, dp, ansi )
static void check_EnumSessions_async_( int line, DPSESSIONDESC2 *dpsd, IDirectPlay4 *dp, BOOL ansi )
{
    DPSESSIONDESC2 replyDpsds[] =
    {
        {
            .dwSize = sizeof( DPSESSIONDESC2 ),
            .dwFlags = 0,
            .guidInstance = appGuid,
            .guidApplication = appGuid,
            .dwMaxPlayers = 10,
            .dwCurrentPlayers = 0,
            .lpszSessionName = (WCHAR *) L"normal",
            .dwReserved1 = 0x11223344,
            .dwReserved2 = 0,
            .dwUser1 = 1,
            .dwUser2 = 2,
            .dwUser3 = 3,
            .dwUser4 = 4,
        },
        {
            .dwSize = sizeof( DPSESSIONDESC2 ),
            .dwFlags = DPSESSION_JOINDISABLED | DPSESSION_PASSWORDREQUIRED | DPSESSION_PRIVATE,
            .guidInstance = appGuid2,
            .guidApplication = appGuid2,
            .dwMaxPlayers = 10,
            .dwCurrentPlayers = 10,
            .lpszSessionName = (WCHAR *) L"private",
            .dwReserved1 = 0xaabbccdd,
            .dwReserved2 = 0,
            .dwUser1 = 5,
            .dwUser2 = 6,
            .dwUser3 = 7,
            .dwUser4 = 8,
        },
    };
    CheckSessionListCallbackData callbackData;
    ExpectedSession expectedSessions[ 2 ];
    EnumSessionsParam *param;
    unsigned short port;
    SOCKET enumSock;
    int tryIndex;
    SOCKET sock;
    HRESULT hr;
    int i;

    enumSock = bindUdp_( line, 47624 );

    for ( tryIndex = 0; ; ++tryIndex )
    {
        memset( expectedSessions, 0, sizeof( expectedSessions ) );
        expectedSessions[ 0 ].dpsd = replyDpsds [ 0 ];
        expectedSessions[ 0 ].dpsd.lpszSessionNameA = (char *) AW( "normal", ansi );

        memset( &callbackData, 0, sizeof( callbackData ) );
        callbackData.line = line;
        callbackData.expectedSessions = expectedSessions;
        callbackData.expectedCount = 1;
        callbackData.ansi = ansi;

        /* Do a sync enumeration first to fill the cache */
        param = enumSessionsAsync( dp, dpsd, 100, checkSessionListCallback, &callbackData, 0 );

        port = receiveEnumSessionsRequest_( line, enumSock, &appGuid, NULL, 0 );

        sock = connectTcp_( line, port );

        if ( sock != INVALID_SOCKET )
        {
            sendEnumSessionsReply_( line, sock, 2349, &replyDpsds[ 0 ] );

            closesocket( sock );
        }

        checkNoMoreMessages_( line, enumSock );

        hr = enumSessionsAsyncWait( param, 2000 );
        ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

        if ( tryIndex < 19 && callbackData.actualCount < callbackData.expectedCount )
            continue;

        ok_( __FILE__, line )( callbackData.actualCount == callbackData.expectedCount, "got session count %d.\n",
                               callbackData.actualCount );
        ok_( __FILE__, line )( callbackData.timeoutCount, "got timeout count %d.\n", callbackData.timeoutCount );

        break;
    }

    memset( expectedSessions, 0, sizeof( expectedSessions ) );
    expectedSessions[ 0 ].dpsd = replyDpsds [ 0 ];
    expectedSessions[ 0 ].dpsd.lpszSessionNameA = (char *) AW( "normal", ansi );

    memset( &callbackData, 0, sizeof( callbackData ) );
    callbackData.line = line;
    callbackData.expectedSessions = expectedSessions;
    callbackData.expectedCount = 1;
    callbackData.ansi = ansi;

    /* Read cache of last sync enumeration */
    param = enumSessionsAsync( dp, dpsd, 100, checkSessionListCallback, &callbackData, DPENUMSESSIONS_ASYNC );

    receiveEnumSessionsRequest_( line, enumSock, &appGuid, NULL, DPENUMSESSIONS_ASYNC );

    hr = enumSessionsAsyncWait( param, 2000 );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

    ok_( __FILE__, line )( callbackData.actualCount == callbackData.expectedCount, "got session count %d.\n",
                           callbackData.actualCount );
    ok_( __FILE__, line )( callbackData.timeoutCount, "got timeout count %d.\n", callbackData.timeoutCount );

    /* Check that requests are sent periodically */
    for ( i = 0; i < 2; ++i )
        port = receiveEnumSessionsRequest_( line, enumSock, &appGuid, NULL, DPENUMSESSIONS_ASYNC );

    for ( i = 0; i < ARRAYSIZE( replyDpsds ); ++i )
    {
        sock = connectTcp_( line, port );
        if ( sock == INVALID_SOCKET )
            continue;

        sendEnumSessionsReply_( line, sock, 2349 - i, &replyDpsds[ i ] );

        closesocket( sock );
    }

    for ( tryIndex = 0; ; ++tryIndex )
    {
        memset( expectedSessions, 0, sizeof( expectedSessions ) );
        expectedSessions[ 0 ].dpsd = replyDpsds [ 0 ];
        expectedSessions[ 0 ].dpsd.lpszSessionNameA = (char *) AW( "normal", ansi );
        expectedSessions[ 1 ].dpsd = replyDpsds [ 1 ];
        expectedSessions[ 1 ].dpsd.lpszSessionNameA = (char *) AW( "private", ansi );

        memset( &callbackData, 0, sizeof( callbackData ) );
        callbackData.line = line;
        callbackData.expectedSessions = expectedSessions;
        callbackData.expectedCount = ARRAYSIZE( expectedSessions );
        callbackData.ansi = ansi;

        /* Retrieve results */
        param = enumSessionsAsync( dp, dpsd, 100, checkSessionListCallback, &callbackData, DPENUMSESSIONS_ASYNC );
        hr = enumSessionsAsyncWait( param, 2000 );
        ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

        if ( tryIndex < 19 && callbackData.actualCount < callbackData.expectedCount )
            continue;

        ok_( __FILE__, line )( callbackData.actualCount == callbackData.expectedCount, "got session count %d.\n",
                               callbackData.actualCount );
        ok_( __FILE__, line )( callbackData.timeoutCount, "got timeout count %d.\n", callbackData.timeoutCount );

        break;
    }

    memset( &callbackData, 0, sizeof( callbackData ) );
    callbackData.line = line;
    callbackData.expectedSessions = NULL;
    callbackData.expectedCount = 0;
    callbackData.ansi = ansi;

    /* Stop enumeration */
    param = enumSessionsAsync( dp, dpsd, 100, checkSessionListCallback, &callbackData, DPENUMSESSIONS_STOPASYNC );
    hr = enumSessionsAsyncWait( param, 2000 );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

    ok_( __FILE__, line )( !callbackData.actualCount, "got session count %d.\n", callbackData.actualCount );
    todo_wine ok_( __FILE__, line )( callbackData.timeoutCount, "got timeout count %d.\n", callbackData.timeoutCount );

    closesocket( enumSock );
}

static void test_EnumSessions(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 dpsd;
    IDirectPlay4 *dpA;
    IDirectPlay4 *dp;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dpA );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    hr = IDirectPlayX_QueryInterface( dpA, &IID_IDirectPlay4, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    /* Service provider not initialized */
    check_EnumSessions( dpA, &appGuidDpsd, 0, DPERR_UNINITIALIZED, 0, FALSE, FALSE, NULL, 0, TRUE, FALSE );
    check_EnumSessions( dp, &appGuidDpsd, 0, DPERR_UNINITIALIZED, 0, FALSE, FALSE, NULL, 0, FALSE, FALSE );

    init_TCPIP_provider( dpA, "127.0.0.1", 0 );

    /* Session with no size */
    dpsd = appGuidDpsd;
    dpsd.dwSize = 0;
    check_EnumSessions( dpA, &dpsd, 0, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, TRUE, FALSE );
    check_EnumSessions( dp, &dpsd, 0, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, FALSE, FALSE );

    /* No sessions */
    check_EnumSessions( dpA, &appGuidDpsd, 0, DP_OK, 0, TRUE, TRUE, NULL, 0, TRUE, FALSE );
    check_EnumSessions( dp, &appGuidDpsd, 0, DP_OK, 0, TRUE, TRUE, NULL, 0, FALSE, FALSE );

    /* Invalid params */
    check_EnumSessions( dpA, &appGuidDpsd, -1, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, TRUE, TRUE );
    check_EnumSessions( dp, &appGuidDpsd, -1, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, FALSE, TRUE );
    check_EnumSessions( dpA, NULL, 0, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, TRUE, FALSE );
    check_EnumSessions( dp, NULL, 0, DPERR_INVALIDPARAMS, 0, FALSE, FALSE, NULL, 0, FALSE, FALSE );

    /* All sessions are enumerated regardless of flags */
    check_EnumSessions( dpA, &appGuidDpsd, 0, DP_OK, 2, TRUE, TRUE, NULL, 2, TRUE, FALSE );
    check_EnumSessions( dp, &appGuidDpsd, 0, DP_OK, 2, TRUE, TRUE, NULL, 2, FALSE, FALSE );
    check_EnumSessions( dpA, &appGuidDpsd, DPENUMSESSIONS_AVAILABLE, DP_OK, 2, TRUE, TRUE, NULL, 2, TRUE, FALSE );
    check_EnumSessions( dp, &appGuidDpsd, DPENUMSESSIONS_AVAILABLE, DP_OK, 2, TRUE, TRUE, NULL, 2, FALSE, FALSE );

    /* Async enumeration */
    check_EnumSessions_async( &appGuidDpsd, dpA, TRUE );
    check_EnumSessions_async( &appGuidDpsd, dp, FALSE );

    /* Enumeration with password */
    dpsd = appGuidDpsd;
    dpsd.lpszPasswordA = (char *) "password";
    check_EnumSessions( dpA, &dpsd, 0, DP_OK, 2, TRUE, TRUE, L"password", 2, TRUE, FALSE );
    dpsd = appGuidDpsd;
    dpsd.lpszPassword = (WCHAR *) L"password";
    check_EnumSessions( dp, &dpsd, 0, DP_OK, 2, TRUE, TRUE, L"password", 2, FALSE, FALSE );

    IDirectPlayX_Release( dp );
    IDirectPlayX_Release( dpA );
}

static void test_interactive_EnumSessions(void)
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

    dpsd_server[0].lpszSessionNameA = (LPSTR) "normal";
    dpsd_server[0].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[0].dwMaxPlayers = 10;

    dpsd_server[1].lpszSessionNameA = (LPSTR) "full";
    dpsd_server[1].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL );
    dpsd_server[1].dwMaxPlayers = 1;

    dpsd_server[2].lpszSessionNameA = (LPSTR) "no new";
    dpsd_server[2].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_NEWPLAYERSDISABLED );
    dpsd_server[2].dwMaxPlayers = 10;

    dpsd_server[3].lpszSessionNameA = (LPSTR) "no join";
    dpsd_server[3].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_JOINDISABLED );
    dpsd_server[3].dwMaxPlayers = 10;

    dpsd_server[4].lpszSessionNameA = (LPSTR) "private";
    dpsd_server[4].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PRIVATE );
    dpsd_server[4].dwMaxPlayers = 10;
    dpsd_server[4].lpszPasswordA = (LPSTR) "password";

    dpsd_server[5].lpszSessionNameA = (LPSTR) "protected";
    dpsd_server[5].dwFlags = ( DPSESSION_CLIENTSERVER |
                               DPSESSION_DIRECTPLAYPROTOCOL |
                               DPSESSION_PASSWORDREQUIRED );
    dpsd_server[5].dwMaxPlayers = 10;
    dpsd_server[5].lpszPasswordA = (LPSTR) "password";


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
        dpsd_server[i].lpszPasswordA = (LPSTR) "password";
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
        dpsd_server[i].lpszPasswordA = NULL;
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
        dpsd_server[i].lpszPasswordA = (LPSTR) "password";
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
    dpsd.lpszPasswordA = (LPSTR) "bad_password";
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
    dpsd.lpszPasswordA = (LPSTR) "password";
    callbackData.dwCounter1 = -1;
    hr = IDirectPlayX_EnumSessions( pDP, &dpsd, 0, EnumSessions_cb,
                                    &callbackData, callbackData.dwFlags );
    checkHR( DP_OK, hr );
    check( 2, callbackData.dwCounter1 );


    dpsd.lpszPasswordA = NULL;
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
        dpsd_server[i].lpszPasswordA = NULL;
        dpsd_server[i].dwMaxPlayers = 10;
    }
    dpsd_server[4].lpszSessionNameA = (LPSTR) "normal1";
    dpsd_server[4].guidApplication = appGuid;
    dpsd_server[5].lpszSessionNameA = (LPSTR) "normal2";
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

        lpData[i] = calloc( 1, 1024 );
    }
    lpDataMsg = calloc( 1, 1024 );


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
    dpsd.lpszSessionNameA = (LPSTR) "Wahaa";
    hr = IDirectPlayX_SetSessionDesc( pDP[0], &dpsd, 0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetSessionDesc( pDP[1], lpData[1], &dwDataSize );
    checkHR( DP_OK, hr );
    checkStr( dpsd.lpszSessionNameA, lpData[1]->lpszSessionNameA );


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

    free( lpDataMsg );
    for (i=0; i<2; i++)
    {
        free( lpData[i] );
        IDirectPlayX_Release( pDP[i] );
    }

}

/* CreatePlayer */

#define checkCreatePlayerOrGroupMessage( dp, expectedType, expectedDpid, expectedCurrentPlayers, expectedPlayerData, \
                                         expectedPlayerDataSize, expectedShortName, expectedLongName, expectedParent, \
                                         expectedFlags ) \
        checkCreatePlayerOrGroupMessage_( __LINE__, dp, expectedType, expectedDpid, expectedCurrentPlayers, \
                                          expectedPlayerData, expectedPlayerDataSize, expectedShortName, \
                                          expectedLongName, expectedParent, expectedFlags )
static DPID checkCreatePlayerOrGroupMessage_( int line, IDirectPlay4 *dp, DWORD expectedType, DPID expectedDpid,
                                              DWORD expectedCurrentPlayers, void *expectedPlayerData,
                                              DWORD expectedPlayerDataSize, const char *expectedShortName,
                                              const char *expectedLongName, DPID expectedParent, DWORD expectedFlags )
{
    DPMSG_CREATEPLAYERORGROUP *msg;
    DWORD expectedShortNameSize;
    DWORD expectedLongNameSize;
    DWORD expectedMsgDataSize;
    BYTE msgData[ 256 ];
    DWORD msgDataSize;
    DPID fromId, toId;
    HRESULT hr;

    memset( &msgData, 0, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );
    ok_( __FILE__, line )( fromId == DPID_SYSMSG, "got source id %#lx.\n", fromId );

    msg = (DPMSG_CREATEPLAYERORGROUP *) msgData;
    ok_( __FILE__, line )( msg->dwType == DPSYS_CREATEPLAYERORGROUP, "got message type %#lx.\n", msg->dwType );
    ok_( __FILE__, line )( msg->dwPlayerType == expectedType, "got player type %#lx.\n", msg->dwPlayerType );
    ok_( __FILE__, line )( msg->dpId == expectedDpid, "got id %#lx.\n", msg->dpId );
    ok_( __FILE__, line )( msg->dwCurrentPlayers == expectedCurrentPlayers, "got current players %lu.\n",
                           msg->dwCurrentPlayers );
    ok_( __FILE__, line )( msg->dwDataSize == expectedPlayerDataSize, "got player data size %lu.\n", msg->dwDataSize );
    if ( expectedPlayerData )
    {
        ok_( __FILE__, line )( msg->lpData && !memcmp( msg->lpData, expectedPlayerData, expectedPlayerDataSize ),
                               "player data didn't match.\n" );
    }
    else
    {
        ok_( __FILE__, line )( !msg->lpData, "got player data %p.\n", msg->lpData );
    }
    ok_( __FILE__, line )( msg->dpnName.dwSize == sizeof( DPNAME ), "got name size %lu.\n", msg->dpnName.dwSize );
    ok_( __FILE__, line )( !msg->dpnName.dwFlags, "got name flags %#lx.\n", msg->dpnName.dwFlags );
    if ( expectedShortName )
    {
        ok_( __FILE__, line )( msg->dpnName.lpszShortNameA && !strcmp( msg->dpnName.lpszShortNameA, expectedShortName ),
                               "got short name %s.\n", wine_dbgstr_a( msg->dpnName.lpszShortNameA ) );
    }
    else
    {
        ok_( __FILE__, line )( !msg->dpnName.lpszShortNameA, "got short name %s.\n",
                               wine_dbgstr_a( msg->dpnName.lpszShortNameA ) );
    }
    if ( expectedLongName )
    {
        ok_( __FILE__, line )( msg->dpnName.lpszLongNameA && !strcmp( msg->dpnName.lpszLongNameA, expectedLongName ),
                               "got long name %s.\n", wine_dbgstr_a( msg->dpnName.lpszLongNameA ) );
    }
    else
    {
        ok_( __FILE__, line )( !msg->dpnName.lpszLongNameA, "got long name %s.\n",
                               wine_dbgstr_a( msg->dpnName.lpszLongNameA ) );
    }
    ok_( __FILE__, line )( msg->dpIdParent == expectedParent, "got parent id %#lx.\n", msg->dpIdParent );
    ok_( __FILE__, line )( msg->dwFlags == expectedFlags, "got flags %#lx.\n", msg->dwFlags );

    expectedShortNameSize = expectedShortName ? strlen( expectedShortName ) + 1 : 0;
    expectedLongNameSize = expectedLongName ? strlen( expectedLongName ) + 1 : 0;
    expectedMsgDataSize = sizeof( DPMSG_CREATEPLAYERORGROUP ) + expectedShortNameSize + expectedLongNameSize
                        + expectedPlayerDataSize;

    ok_( __FILE__, line )( msgDataSize == expectedMsgDataSize, "got message size %lu.\n", msgDataSize );

    return toId;
}

#define checkPlayerMessage( dp, expectedFromId, expectedData, expectedDataSize ) \
        checkPlayerMessage_( __LINE__, dp, expectedFromId, expectedData, expectedDataSize )
static DPID checkPlayerMessage_( int line, IDirectPlay4 *dp, DPID expectedFromId, void *expectedData,
                                 DWORD expectedDataSize )
{
    DPID fromId, toId;
    BYTE data[ 1024 ];
    DWORD dataSize;
    HRESULT hr;

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( data, 0xcc, sizeof( data ) );
    dataSize = sizeof( data );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, data, &dataSize );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

    ok_( __FILE__, line )( fromId == expectedFromId, "got source id %#lx.\n", fromId );
    ok_( __FILE__, line )( !memcmp( data, expectedData, expectedDataSize ), "message data didn't match.\n" );
    ok_( __FILE__, line )( dataSize == expectedDataSize, "got data size %lu.\n", dataSize );

    return toId;
}

#define checkNoMorePlayerMessages( dp ) checkNoMorePlayerMessages_( __LINE__, dp )
static void checkNoMorePlayerMessages_( int line, IDirectPlay4 *dp )
{
    char msgData[ 256 ];
    DWORD msgDataSize;
    DPID fromId, toId;
    HRESULT hr;

    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok_( __FILE__, line )( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );
}

typedef struct
{
    DPID ids[ 256 ];
    DWORD idCount;
} GetPlayerIdsCallbackData;

static BOOL CALLBACK getPlayerIdsCallback( DPID dpid, DWORD playerType, const DPNAME *name, DWORD flags,
                                           void *context )
{
    GetPlayerIdsCallbackData *data = context;

    if ( data->idCount >= ARRAYSIZE( data->ids ) )
        return FALSE;

    data->ids[ data->idCount ] = dpid;
    ++data->idCount;

    return TRUE;
}

#define checkCreatePlayerOrGroupMessages( dp, expectedType, expectedDpid, expectedCurrentPlayers, expectedPlayerData, \
                                          expectedPlayerDataSize, expectedShortName, expectedLongName, expectedParent, \
                                          expectedFlags ) \
        checkCreatePlayerOrGroupMessages_( __LINE__, dp, expectedType, expectedDpid, expectedCurrentPlayers, \
                                           expectedPlayerData, expectedPlayerDataSize, expectedShortName, \
                                           expectedLongName, expectedParent, expectedFlags )
static void checkCreatePlayerOrGroupMessages_( int line, IDirectPlay4 *dp, DWORD expectedType, DPID expectedDpid,
                                               DWORD expectedCurrentPlayers, void *expectedPlayerData,
                                               DWORD expectedPlayerDataSize, const char *expectedShortName,
                                               const char *expectedLongName, DPID expectedParent, DWORD expectedFlags )
{
    GetPlayerIdsCallbackData data = { 0 };
    HRESULT hr;
    DPID dpid;
    DWORD i;
    DWORD j;

    hr = IDirectPlayX_EnumPlayers( dp, NULL, getPlayerIdsCallback, &data, DPENUMPLAYERS_LOCAL );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );

    for ( i = 0; i < data.idCount - 1; ++i )
    {
        dpid = checkCreatePlayerOrGroupMessage_( line, dp, expectedType, expectedDpid, expectedCurrentPlayers,
                                                 expectedPlayerData, expectedPlayerDataSize, expectedShortName,
                                                 expectedLongName, expectedParent, expectedFlags );
        for ( j = 0; j < data.idCount; ++j )
        {
            if ( data.ids[ j ] == dpid )
            {
                data.ids[ j ] = 0;
                break;
            }
        }
        ok_( __FILE__, line )( dpid && dpid != expectedDpid && j < data.idCount, "got destination id %#lx.\n", dpid );
    }
}

#define check_CreatePlayer( dp, dpid, name, flags, expectedHr, expectedDpid, recvSock, requestExpected, \
                            expectedFlags, expectedShortName, expectedShortNameA, expectedLongName, expectedLongNameA, \
                            expectedCurrentPlayers ) \
        check_CreatePlayer_( __LINE__, dp, dpid, name, flags, expectedHr, expectedDpid, recvSock, requestExpected, \
                             expectedFlags, expectedShortName, expectedShortNameA, expectedLongName, expectedLongNameA, \
                             expectedCurrentPlayers )
static void check_CreatePlayer_( int line, IDirectPlay4 *dp, DPID *dpid, DPNAME *name, DWORD flags, HRESULT expectedHr,
                                 DPID expectedDpid, SOCKET recvSock, BOOL requestExpected, DWORD expectedFlags,
                                 const WCHAR *expectedShortName, const char *expectedShortNameA,
                                 const WCHAR *expectedLongName, const char *expectedLongNameA,
                                 DWORD expectedCurrentPlayers )
{
    BYTE playerData[] = { 1, 2, 3, 4, 5, 6, 7, 8, };
    CreatePlayerParam *param;
    SOCKET sendSock;
    HRESULT hr;

    param = createPlayerAsync( dp, dpid, name, NULL, playerData, sizeof( playerData ), flags );

    if ( requestExpected )
    {
        unsigned short port;

        port = receiveRequestPlayerId_( line, recvSock, expectedFlags );

        sendSock = connectTcp_( line, port );

        sendRequestPlayerReply_( line, sendSock, 2349, expectedDpid, DP_OK );
        receiveCreatePlayer_( line, recvSock, expectedDpid, expectedFlags, expectedShortName, expectedLongName,
                              playerData, sizeof( playerData ) );

        hr = createPlayerAsyncWait( param, 2000 );
        ok_( __FILE__, line )( hr == expectedHr, "CreatePlayer() returned %#lx.\n", hr );
        if ( dpid )
            ok_( __FILE__, line )( *dpid == expectedDpid, "got dpid %#lx.\n", *dpid );

        checkPlayerExists_( line, dp, expectedDpid, DPPLAYERTYPE_PLAYER, expectedShortNameA, expectedLongNameA,
                            expectedFlags, playerData, sizeof( playerData ) );

        if ( hr == DP_OK )
            checkCreatePlayerOrGroupMessages_( line, dp, DPPLAYERTYPE_PLAYER, expectedDpid, expectedCurrentPlayers,
                                               playerData, sizeof( playerData ), expectedShortNameA, expectedLongNameA,
                                               0, expectedFlags );

        checkNoMorePlayerMessages_( line, dp );

        closesocket( sendSock );
    }
    else
    {
        hr = createPlayerAsyncWait( param, 2000 );
        ok_( __FILE__, line )( hr == expectedHr, "CreatePlayer() returned %#lx.\n", hr );
        if ( dpid )
            ok_( __FILE__, line )( *dpid == expectedDpid, "got dpid %#lx.\n", *dpid );
    }

    if ( recvSock != INVALID_SOCKET )
        checkNoMoreMessages_( line, recvSock );
}

static void test_CreatePlayer(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    DPNAME fullName =
    {
        .dwSize = sizeof( DPNAME ),
        .lpszShortNameA = (char *) "short player name",
        .lpszLongNameA = (char *) "long player name",
    };
    DPNAME nullName =
    {
        .dwSize = sizeof( DPNAME ),
    };
    IDirectPlay4 *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    DPNAME name;
    HRESULT hr;
    DPID dpid;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    /* Connection not initialized */
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, NULL, 0, DPERR_UNINITIALIZED, 0xdeadbeef, INVALID_SOCKET, FALSE, 0, NULL, NULL,
                        NULL, NULL, 0 );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    /* Session not open */
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, NULL, 0, DPERR_INVALIDPARAMS, 0xdeadbeef, INVALID_SOCKET, FALSE, 0, NULL, NULL,
                        NULL, NULL, 0 );

    /* Join to normal session */
    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    /* Player name */
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, NULL, 0, DP_OK, 2, recvSock, TRUE, 0x8, NULL, NULL, NULL, NULL, 2 );

    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, &fullName, 0, DP_OK, 3, recvSock, TRUE, 0x8, L"short player name",
                        "short player name", L"long player name", "long player name", 3 );

    name = fullName;
    name.dwSize = 1;
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, &name, 0, DP_OK, 4, recvSock, TRUE, 0x8, L"short player name", "short player name",
                        L"long player name", "long player name", 4 );

    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, &nullName, 0, DP_OK, 5, recvSock, TRUE, 0x8, NULL, NULL, NULL, NULL, 5 );

    /* Null dpid */
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, NULL, NULL, 0, DPERR_INVALIDPARAMS, 0, recvSock, FALSE, 0, NULL, NULL, NULL, NULL, 0 );

    /* Flags */
    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, NULL, 0, DP_OK, 6, recvSock, TRUE, 0x8, NULL, NULL, NULL, NULL, 6 );

    dpid = 0xdeadbeef;
    check_CreatePlayer( dp, &dpid, NULL, DPPLAYER_SPECTATOR, DP_OK, 7, recvSock, TRUE, 0x208, NULL, NULL, NULL, NULL,
                        7 );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
}

static void test_interactive_CreatePlayer(void)
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
    name.lpszShortNameA = (LPSTR) "test";
    name.lpszLongNameA = NULL;


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

#define createPlayer( dp, inDpid, event, data, dataSize, flags, sendSock, recvSock ) \
        createPlayer_( __LINE__, dp, inDpid, event, data, dataSize, flags, sendSock, recvSock )
static void createPlayer_( int line, IDirectPlay4 *dp, DPID inDpid, HANDLE event, void *data,
                           DWORD dataSize, DWORD flags, SOCKET sendSock, SOCKET recvSock )
{
    CreatePlayerParam *param;
    HRESULT hr;
    DPID dpid;

    dpid = 0xdeadbeef;
    param = createPlayerAsync( dp, &dpid, NULL, event, data, dataSize, flags );

    receiveRequestPlayerId_( line, recvSock, flags | DPPLAYER_LOCAL );

    sendRequestPlayerReply_( line, sendSock, 2349, inDpid, DP_OK );

    receiveCreatePlayer_( line, recvSock, inDpid, flags | DPPLAYER_LOCAL, NULL, NULL, data, dataSize );

    hr = createPlayerAsyncWait( param, 2000 );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );
    ok_( __FILE__, line )( dpid == inDpid, "got dpid %#lx.\n", dpid );
}

static void test_CREATEPLAYER(void)
{
    BYTE playerData[] = { 4, 3, 2, 1, };
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidInstance = appGuid,
        .guidApplication = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    IDirectPlay4A *dp;
    DWORD waitResult;
    SOCKET sendSock;
    SOCKET recvSock;
    HANDLE event;
    HRESULT hr;
    DPID dpid;

    event = CreateEventA( NULL, FALSE, FALSE, NULL );

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    createPlayer( dp, 0x11223344, event, NULL, 0, 0, sendSock, recvSock );

    sendCreatePlayer( sendSock, 2349, 2399, L"new player short name", L"new player long name", playerData,
                      sizeof( playerData ) );

    waitResult = WaitForSingleObject( event, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    checkPlayerExists( dp, 0x07734, DPPLAYERTYPE_PLAYER, "new player short name", "new player long name",
                       DPENUMPLAYERS_REMOTE, playerData, sizeof( playerData ) );

    dpid = checkCreatePlayerOrGroupMessage( dp, DPPLAYERTYPE_PLAYER, 0x07734, 4, playerData, sizeof( playerData ),
                                            "new player short name", "new player long name", 0, 0 );
    ok( dpid == 0x11223344, "got destination id %#lx.\n", dpid );

    checkNoMorePlayerMessages( dp );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );

    CloseHandle( event );
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

    hr = IDirectPlayX_GetPlayerCaps( pDP[0], dpid[0], NULL, 0 );
    checkHR( DPERR_INVALIDPARAMS, hr );

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

    LPSTR lpDataGet       = calloc( 1, dwDataSizeFake );
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


    free( lpDataGet );
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
    LPVOID lpData = calloc( 1, dwDataSize );
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
    playerName.lpszShortNameA = (LPSTR) "player_name";
    playerName.lpszLongNameA = (LPSTR) "player_long_name";


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
    checkStr( playerName.lpszShortNameA, ((LPDPNAME)lpData)->lpszShortNameA );
    checkStr( playerName.lpszLongNameA,  ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0,                            ((LPDPNAME)lpData)->dwFlags );

    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], NULL, 0 );
    checkHR( DP_OK, hr );
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 16, dwDataSize );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszShortNameA );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszLongNameA );
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
    checkLP( NULL, ((LPDPNAME)lpData)->lpszShortNameA );
    checkLP( NULL, ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0, ((LPDPNAME)lpData)->dwFlags );


    /* Flags */
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_GUARANTEED );
    checkHR( DP_OK, hr );
    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0], lpData, &dwDataSize );
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( playerName.lpszShortNameA, ((LPDPNAME)lpData)->lpszShortNameA );
    checkStr( playerName.lpszLongNameA,  ((LPDPNAME)lpData)->lpszLongNameA );
    check( 0, ((LPDPNAME)lpData)->dwFlags );

    /* - Local (no propagation) */
    playerName.lpszShortNameA = (LPSTR) "no_propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 48, dwDataSize );
    checkStr( "no_propagation", ((LPDPNAME)lpData)->lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", ((LPDPNAME)lpData)->lpszShortNameA );

    /* -- 2 */

    playerName.lpszShortNameA = (LPSTR) "no_propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_LOCAL | DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[0], dpid[0],
                                     lpData, &dwDataSize ); /* Local fetch */
    checkHR( DP_OK, hr );
    check( 50, dwDataSize );
    checkStr( "no_propagation_2", ((LPDPNAME)lpData)->lpszShortNameA );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "player_name", ((LPDPNAME)lpData)->lpszShortNameA );

    /* - Remote (propagation, default) */
    playerName.lpszShortNameA = (LPSTR) "propagation";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     DPSET_REMOTE );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 45, dwDataSize );
    checkStr( "propagation", ((LPDPNAME)lpData)->lpszShortNameA );

    /* -- 2 */
    playerName.lpszShortNameA = (LPSTR) "propagation_2";
    hr = IDirectPlayX_SetPlayerName( pDP[0], dpid[0], &playerName,
                                     0 );
    checkHR( DP_OK, hr );

    dwDataSize = 1024;
    hr = IDirectPlayX_GetPlayerName( pDP[1], dpid[0],
                                     lpData, &dwDataSize ); /* Remote fetch */
    checkHR( DP_OK, hr );
    check( 47, dwDataSize );
    checkStr( "propagation_2", ((LPDPNAME)lpData)->lpszShortNameA );


    /* Checking system messages */
    check_messages( pDP[0], dpid, 2, &callbackData );
    checkStr( "S0,S0,S0,S0,S0,S0,S0,", callbackData.szTrace1 );
    checkStr( "48,28,57,28,57,57,59,", callbackData.szTrace2 );
    check_messages( pDP[1], dpid, 2, &callbackData );
    checkStr( "S1,S1,S1,S1,S1,S1,", callbackData.szTrace1 );
    checkStr( "28,57,28,57,57,59,", callbackData.szTrace2 );


    free( lpData );
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
    dpCredentials.lpszUsernameA = (LPSTR) "user";
    dpCredentials.lpszPasswordA = (LPSTR) "pass";
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
    LPVOID lpData = calloc( 1, dwDataSize );


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


    free( lpData );
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
    LPVOID lpData = calloc( 1, dwDataSize );


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
        goto cleanup;
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


    free( lpData );

cleanup:
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );
    IDirectPlayLobby_Release(pDPL);
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
    LPDPMSG_CREATEPLAYERORGROUP lpDataGet = calloc( 1, 1024 );
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
    groupName.lpszShortNameA = (LPSTR) lpData;


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
        if ( NULL == lpDataGet->dpnName.lpszShortNameA )
        {
            check( 48, dwDataSizeGet );
        }
        else
        {
            check( 48 + dwDataSize, dwDataSizeGet );
            checkStr( lpData, lpDataGet->dpnName.lpszShortNameA );
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


    free( lpDataGet );
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
    LPVOID lpData = calloc( 1, 1024 );
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


    free( lpData );
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
    LPVOID lpData = calloc( 1, 1024 );


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


    free( lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Send */

static void test_Send(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    BYTE data[] = { 1, 2, 3, 4, 5, 6, 7, 8, };
    IDirectPlay4 *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    SOCKET udpSock;
    HRESULT hr;
    DPID dpid;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x1337, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DPERR_UNINITIALIZED, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x1337, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    createPlayer( dp, 0x07734, NULL, NULL, 0, 0, sendSock, recvSock );
    createPlayer( dp, 0x14, NULL, NULL, 0, 0, sendSock, recvSock );

    checkCreatePlayerOrGroupMessage( dp, DPPLAYERTYPE_PLAYER, 0x14, 3, NULL, 0, NULL, NULL, 0, DPPLAYER_LOCAL );

    hr = IDirectPlayX_Send( dp, 0x07734, 0xdeadbeef, DPSEND_GUARANTEED, data, sizeof( data ) );
    todo_wine ok( hr == DPERR_INVALIDPARAM, "got hr %#lx.\n", hr );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x1337, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    receiveGuaranteedGameMessage( recvSock, 0x07734, 0x1337, data, sizeof( data ) );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x14, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    dpid = checkPlayerMessage( dp, 0x07734, data, sizeof( data ) );
    ok( dpid == 0x14, "got destination id %#lx.\n", dpid );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x07734, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    checkNoMorePlayerMessages( dp );
    checkNoMoreMessages( recvSock );

    hr = IDirectPlayX_Send( dp, 0x07734, DPID_ALLPLAYERS, DPSEND_GUARANTEED, data, sizeof( data ) );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    dpid = checkPlayerMessage( dp, 0x07734, data, sizeof( data ) );
    ok( dpid == 0x14, "got destination id %#lx.\n", dpid );
    receiveGuaranteedGameMessage( recvSock, 0x07734, DPID_ALLPLAYERS, data, sizeof( data ) );

    udpSock = bindUdp( 2399 );

    hr = IDirectPlayX_Send( dp, 0x07734, 0x1337, 0, data, sizeof( data ) );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    receiveGameMessage( udpSock, 0x07734, 0x1337, data, sizeof( data ) );

    checkNoMorePlayerMessages( dp );
    checkNoMoreMessages( udpSock );
    checkNoMoreMessages( recvSock );

    closesocket( udpSock );
    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
}

static void test_interactive_Send(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4], idFrom, idTo;
    CallbackData callbackData;
    HRESULT hr;
    LPCSTR message = "message";
    DWORD messageSize = strlen(message) + 1;
    DWORD dwDataSize = 1024;
    LPDPMSG_GENERIC lpData = calloc( 1, dwDataSize );
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


    free( lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* Receive */

static void test_Receive(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    BYTE data0[] = { 1, 2, 3, 4, 5, 6, 7, 8, };
    BYTE data1[] = { 4, 3, 2, 1, };
    unsigned short udpPort;
    BYTE msgData[ 256 ];
    DWORD msgDataSize;
    DPID fromId, toId;
    DWORD waitResult;
    IDirectPlay4 *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    SOCKET udpSock;
    HANDLE event0;
    HANDLE event1;
    HRESULT hr;

    event0 = CreateEventA( NULL, FALSE, FALSE, NULL );
    event1 = CreateEventA( NULL, FALSE, FALSE, NULL );

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DPERR_UNINITIALIZED, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, &udpPort );

    createPlayer( dp, 0x07734, event0, NULL, 0, 0, sendSock, recvSock );
    createPlayer( dp, 0x14, event1, NULL, 0, 0, sendSock, recvSock );

    waitResult = WaitForSingleObject( event0, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    checkCreatePlayerOrGroupMessage( dp, DPPLAYERTYPE_PLAYER, 0x14, 3, NULL, 0, NULL, NULL, 0, DPPLAYER_LOCAL );

    sendGuaranteedGameMessage( sendSock, 2349, 0x1337, 0x07734, data0, sizeof( data0 ) );
    sendGuaranteedGameMessage( sendSock, 2349, 0x1337, 0x14, data1, sizeof( data1 ) );

    waitResult = WaitForSingleObject( event0, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );
    waitResult = WaitForSingleObject( event1, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    msgDataSize = sizeof( data0 ) - 1;
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_ALL | DPRECEIVE_PEEK, msgData, &msgDataSize );
    ok( hr == DPERR_BUFFERTOOSMALL, "got hr %#lx.\n", hr );
    ok( fromId == 0xdeadbeef, "got source id %#lx.\n", fromId );
    ok( toId == 0xdeadbeef, "got destination id %#lx.\n", toId );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, NULL, &msgDataSize );
    ok( hr == DPERR_BUFFERTOOSMALL, "got hr %#lx.\n", hr );
    ok( fromId == 0xdeadbeef, "got source id %#lx.\n", fromId );
    ok( toId == 0xdeadbeef, "got destination id %#lx.\n", toId );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_ALL | DPRECEIVE_PEEK, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_PEEK, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x14, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data1, sizeof( data1 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data1 ), "got message size %lu.\n", msgDataSize );

    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    sendGuaranteedGameMessage( sendSock, 2349, 0x1337, 0x07734, data0, sizeof( data0 ) );

    waitResult = WaitForSingleObject( event0, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    fromId = 0x14;
    toId = 0xdeadbeef;
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_FROMPLAYER, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    fromId = 0x1337;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_FROMPLAYER, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    fromId = 0x1337;
    toId = 0xdeadbeef;
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_FROMPLAYER, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    sendGuaranteedGameMessage( sendSock, 2349, 0x1337, DPID_ALLPLAYERS, data0, sizeof( data0 ) );

    waitResult = WaitForSingleObject( event0, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );
    waitResult = WaitForSingleObject( event1, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    fromId = 0xdeadbeef;
    toId = 0x07734;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_TOPLAYER, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    toId = 0x07734;
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_TOPLAYER, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    fromId = 0xdeadbeef;
    toId = 0x14;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_TOPLAYER, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x14, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    toId = 0x14;
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, DPRECEIVE_TOPLAYER, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    udpSock = connectUdp( udpPort );

    sendGameMessage( udpSock, 0x1337, 0x07734, data0, sizeof( data0 ) );

    waitResult = WaitForSingleObject( event0, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    memset( msgData, 0xcc, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( fromId == 0x1337, "got source id %#lx.\n", fromId );
    ok( toId == 0x07734, "got destination id %#lx.\n", toId );
    ok( !memcmp( msgData, data0, sizeof( data0 ) ), "message data didn't match.\n" );
    ok( msgDataSize == sizeof( data0 ), "got message size %lu.\n", msgDataSize );

    msgDataSize = sizeof( msgData );
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok( hr == DPERR_NOMESSAGES, "got hr %#lx.\n", hr );

    closesocket( udpSock );
    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );

    CloseHandle( event1 );
    CloseHandle( event0 );
}

static void test_interactive_Receive(void)
{

    IDirectPlay4 *pDP;
    DPSESSIONDESC2 dpsd;
    DPID dpid[4], idFrom, idTo;
    HRESULT hr;
    LPCSTR message = "message";
    DWORD messageSize = strlen(message) + 1;
    DWORD dwDataSize = 1024;
    LPDPMSG_GENERIC lpData = calloc( 1, dwDataSize );
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
            checkLP( NULL,                lpDataCreate->dpnName.lpszShortNameA );
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
            checkLP( NULL,                lpDataDestroy->dpnName.lpszShortNameA );
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


    free( lpData );
    IDirectPlayX_Release( pDP );

}

static void test_PING(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidInstance = appGuid,
        .guidApplication = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    unsigned short udpPort;
    SOCKET udpSendSock;
    SOCKET udpRecvSock;
    IDirectPlay4A *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    HRESULT hr;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, &udpPort );

    udpSendSock = connectUdp( udpPort );

    udpRecvSock = bindUdp( 2399 );

    sendPing( udpSendSock, 2349, 0x51573, 0x44332211 );
    receivePingReply( udpRecvSock, 0x12345678, 0x44332211 );
    checkNoMoreMessages( udpRecvSock );

    closesocket( udpRecvSock );
    closesocket( udpSendSock );
    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
}

/* AddPlayerToGroup */

#define checkAddPlayerToGroupMessage( dp, expectedGroupId, expectedPlayerId ) \
        checkAddPlayerToGroupMessage_( __LINE__, dp, expectedGroupId, expectedPlayerId )
static DPID checkAddPlayerToGroupMessage_( int line, IDirectPlay4 *dp, DPID expectedGroupId, DPID expectedPlayerId )
{
    DPMSG_ADDPLAYERTOGROUP *msg;
    BYTE msgData[ 256 ];
    DWORD msgDataSize;
    DPID fromId, toId;
    HRESULT hr;

    memset( &msgData, 0, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );
    ok_( __FILE__, line )( fromId == DPID_SYSMSG, "got source id %#lx.\n", fromId );

    msg = (DPMSG_ADDPLAYERTOGROUP *) msgData;
    ok_( __FILE__, line )( msg->dwType == DPSYS_ADDPLAYERTOGROUP, "got message type %#lx.\n", msg->dwType );
    ok_( __FILE__, line )( msg->dpIdGroup == expectedGroupId, "got id %#lx.\n", msg->dpIdGroup );
    ok_( __FILE__, line )( msg->dpIdPlayer == expectedPlayerId, "got id %#lx.\n", msg->dpIdPlayer );

    return toId;
}

static void test_AddPlayerToGroup(void)
{
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    IDirectPlay4 *dp;
    SOCKET sendSock;
    SOCKET recvSock;
    HRESULT hr;
    DPID dpid;

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    hr = IDirectPlayX_AddPlayerToGroup( dp, 0x5e7, 0x07734 );
    ok( hr == DPERR_UNINITIALIZED, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    hr = IDirectPlayX_AddPlayerToGroup( dp, 0x5e7, 0x07734 );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );

    createPlayer( dp, 0x07734, NULL, NULL, 0, 0, sendSock, recvSock );

    hr = IDirectPlayX_AddPlayerToGroup( dp, 0x5e7, 0x07734 );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    dpid = checkAddPlayerToGroupMessage( dp, 0x5e7, 0x07734 );
    ok( dpid == 0x07734, "got destination id %#lx.\n", dpid );

    receiveAddPlayerToGroup( recvSock, 0x07734, 0x5e7 );

    checkNoMorePlayerMessages( dp );
    checkNoMoreMessages( recvSock );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
}

/* GROUPDATACHANGED */

#define checkSetPlayerOrGroupDataMessage( dp, expectedPlayerType, expectedDpid, expectedData, expectedDataSize ) \
        checkSetPlayerOrGroupDataMessage_( __LINE__, dp, expectedPlayerType, expectedDpid, expectedData, expectedDataSize )
static DPID checkSetPlayerOrGroupDataMessage_( int line, IDirectPlay4 *dp, DWORD expectedPlayerType, DPID expectedDpid,
                                               void *expectedData, DWORD expectedDataSize )
{
    DPMSG_SETPLAYERORGROUPDATA *msg;
    BYTE msgData[ 256 ];
    DWORD msgDataSize;
    DPID fromId, toId;
    HRESULT hr;

    memset( &msgData, 0, sizeof( msgData ) );
    msgDataSize = sizeof( msgData );
    fromId = 0xdeadbeef;
    toId = 0xdeadbeef;
    hr = IDirectPlayX_Receive( dp, &fromId, &toId, 0, msgData, &msgDataSize );
    ok_( __FILE__, line )( hr == DP_OK, "got hr %#lx.\n", hr );
    ok_( __FILE__, line )( fromId == DPID_SYSMSG, "got source id %#lx.\n", fromId );

    msg = (DPMSG_SETPLAYERORGROUPDATA *) msgData;
    ok_( __FILE__, line )( msg->dwType == DPSYS_SETPLAYERORGROUPDATA, "got message type %#lx.\n", msg->dwType );
    ok_( __FILE__, line )( msg->dwPlayerType == expectedPlayerType, "got player type %#lx.\n", msg->dwPlayerType );
    ok_( __FILE__, line )( msg->dpId == expectedDpid, "got dpid %#lx.\n", msg->dpId );
    ok_( __FILE__, line )( msg->dwDataSize == expectedDataSize, "got player data size %lu.\n", msg->dwDataSize );
    if ( expectedData )
    {
        ok_( __FILE__, line )( msg->lpData && !memcmp( msg->lpData, expectedData, expectedDataSize ),
                               "data didn't match.\n" );
    }
    else
    {
        ok_( __FILE__, line )( !msg->lpData, "got data %p.\n", msg->lpData );
    }

    return toId;
}

static void test_GROUPDATACHANGED(void)
{
    BYTE expectedGroupData[] = { 8, 7, 6, 5, 4, 3, 2, 1, };
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidInstance = appGuid,
        .guidApplication = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    BYTE groupData[ 256 ];
    DWORD groupDataSize;
    IDirectPlay4A *dp;
    DWORD waitResult;
    SOCKET sendSock;
    SOCKET recvSock;
    HANDLE event;
    HRESULT hr;
    DPID dpid;

    event = CreateEventA( NULL, FALSE, FALSE, NULL );

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    init_TCPIP_provider( dp, "127.0.0.1", 0 );

    joinSession( dp, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    createPlayer( dp, 0x11223344, event, NULL, 0, 0, sendSock, recvSock );

    sendGroupDataChanged( sendSock, 2349, 0x5e7, expectedGroupData, sizeof( expectedGroupData ) );

    waitResult = WaitForSingleObject( event, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    dpid = checkSetPlayerOrGroupDataMessage( dp, DPPLAYERTYPE_GROUP, 0x5e7, expectedGroupData,
                                             sizeof( expectedGroupData ) );
    ok( dpid == 0x11223344, "got destination id %#lx.\n", dpid );

    memset( groupData, 0xcc, sizeof( groupData ) );
    groupDataSize = sizeof( groupData );
    hr = IDirectPlayX_GetGroupData( dp, 0x5e7, groupData, &groupDataSize, DPGET_REMOTE );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( groupDataSize == sizeof( expectedGroupData ), "got group data size %lu.\n", groupDataSize );
    ok( !memcmp( groupData, expectedGroupData, sizeof( expectedGroupData ) ), "group data didn't match.\n" );

    checkNoMorePlayerMessages( dp );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );

    CloseHandle( event );
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
    LPVOID lpData = calloc( 1, dwDataSize );
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


    free( lpData );
    IDirectPlayX_Release( pDP[0] );
    IDirectPlayX_Release( pDP[1] );

}

/* GetMessageQueue */

static void test_GetMessageQueue(void)
{
    BYTE playerData[] = { 1, 2, 3, 4, 5, 6, 7, 8, };
    BYTE data[] = { 1, 2, 3, 4, };
    DPSESSIONDESC2 appGuidDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
    };
    DPSESSIONDESC2 serverDpsd =
    {
        .dwSize = sizeof( DPSESSIONDESC2 ),
        .guidApplication = appGuid,
        .guidInstance = appGuid,
        .lpszSessionName = (WCHAR *) L"normal",
        .dwReserved1 = 0xaabbccdd,
    };
    DPNAME name =
    {
        .dwSize = sizeof( DPNAME ),
        .lpszShortNameA = (char *) "short name",
    };
    DWORD createPlayerMsgSize;
    CreatePlayerParam *param;
    IDirectPlay4 *dpA;
    IDirectPlay4 *dp;
    DWORD waitResult;
    SOCKET sendSock;
    SOCKET recvSock;
    DWORD byteCount;
    DWORD msgCount;
    HANDLE event;
    HRESULT hr;
    DPID dpid;

    event = CreateEventA( NULL, FALSE, FALSE, NULL );

    hr = CoCreateInstance( &CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4A, (void **) &dpA );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    hr = IDirectPlayX_QueryInterface( dpA, &IID_IDirectPlay4, (void **) &dp );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0, 0, &msgCount, &byteCount );
    ok( hr == DPERR_UNINITIALIZED, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    init_TCPIP_provider( dpA, "127.0.0.1", 0 );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0, DPMESSAGEQUEUE_SEND | DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDFLAGS, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0, 4, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDFLAGS, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0x07734, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0x07734, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 0, "got message count %lu.\n", msgCount );
    ok( byteCount == 0, "got byte count %lu.\n", byteCount );

    joinSession( dpA, &appGuidDpsd, &serverDpsd, &sendSock, &recvSock, NULL );

    createPlayer( dpA, 0x07734, NULL, NULL, 0, 0, sendSock, recvSock );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0x07734, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 0, "got message count %lu.\n", msgCount );
    ok( byteCount == 0, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0x07734, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 0, "got message count %lu.\n", msgCount );
    ok( byteCount == 0, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0x1337, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 0, "got message count %lu.\n", msgCount );
    ok( byteCount == 0, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0x1337, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0x5e7, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0x5e7, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DPERR_INVALIDPLAYER, "got hr %#lx.\n", hr );
    ok( msgCount == 0xdeadbeef, "got message count %lu.\n", msgCount );
    ok( byteCount == 0xdeadbeef, "got byte count %lu.\n", byteCount );

    dpid = 0xdeadbeef;
    param = createPlayerAsync( dpA, &dpid, &name, event, playerData, sizeof( playerData ), 0 );
    receiveRequestPlayerId( recvSock, DPPLAYER_LOCAL );
    sendRequestPlayerReply( sendSock, 2349, 0x14, DP_OK );
    receiveCreatePlayer( recvSock, 0x14, DPPLAYER_LOCAL, L"short name", NULL, playerData, sizeof( playerData ) );
    hr = createPlayerAsyncWait( param, 2000 );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( dpid == 0x14, "got dpid %#lx.\n", dpid );

    sendGuaranteedGameMessage( sendSock, 2349, 0x1337, 0x14, data, sizeof( data ) );

    waitResult = WaitForSingleObject( event, 2000 );
    ok( waitResult == WAIT_OBJECT_0, "message wait returned %lu\n", waitResult );

    /* Both UNICODE and ANSI GetMessageQueue() return the same size. */
    createPlayerMsgSize = sizeof( DPMSG_CREATEPLAYERORGROUP ) + sizeof( playerData ) + sizeof( L"short name" );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0x07734, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 1, "got message count %lu.\n", msgCount );
    ok( byteCount == createPlayerMsgSize, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dp, 0, 0x07734, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 1, "got message count %lu.\n", msgCount );
    ok( byteCount == createPlayerMsgSize, "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0x1337, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 1, "got message count %lu.\n", msgCount );
    ok( byteCount == sizeof( data ), "got byte count %lu.\n", byteCount );

    msgCount = 0xdeadbeef;
    byteCount = 0xdeadbeef;
    hr = IDirectPlayX_GetMessageQueue( dpA, 0, 0, DPMESSAGEQUEUE_RECEIVE, &msgCount, &byteCount );
    ok( hr == DP_OK, "got hr %#lx.\n", hr );
    ok( msgCount == 2, "got message count %lu.\n", msgCount );
    ok( byteCount == createPlayerMsgSize + sizeof( data ), "got byte count %lu.\n", byteCount );

    closesocket( recvSock );
    closesocket( sendSock );

    IDirectPlayX_Release( dp );
    IDirectPlayX_Release( dpA );

    CloseHandle( event );
}

static void test_interactive_GetMessageQueue(void)
{

    IDirectPlay4 *pDP[2];
    DPSESSIONDESC2 dpsd;
    DPID dpid[4];
    CallbackData callbackData;
    HRESULT hr;
    UINT i;
    DWORD dwNumMsgs, dwNumBytes;

    DWORD dwDataSize = 1024;
    LPVOID lpData = calloc( 1, dwDataSize );


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


    free( lpData );
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

    LPDPMSG_SETPLAYERORGROUPDATA lpData = calloc( 1, dwDataSize );

    LPCSTR lpDataLocal[] = { "local_0", "local_1" };
    LPCSTR lpDataRemote[] = { "remote_0", "remote_1" };
    LPCSTR lpDataFake = "ugly_fake_data";
    LPSTR lpDataGet = calloc( 1, 32 );
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


    free( lpDataGet );
    free( lpData );
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
    LPDPMSG_DESTROYPLAYERORGROUP lpData = calloc( 1, dwDataSize );


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
            checkLP( NULL,              lpData->dpnName.lpszShortNameA );
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


    free( lpData );
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
    hr = CoCreateInstance(&CLSID_DirectPlay, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dp4);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == E_INVALIDARG),
            "DirectPlay create failed: %08lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dp4 || dp4 == (IDirectPlay4*)0xdeadbeef, "dp4 = %p\n", dp4);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayLobby,
            (void**)&dp4);
    ok(hr == E_NOINTERFACE, "DirectPlay create failed: %08lx, expected E_NOINTERFACE\n", hr);

    /* Different refcount for all DirectPlay Interfaces */
    hr = CoCreateInstance(&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay4,
            (void**)&dp4);
    ok(hr == S_OK, "DirectPlay create failed: %08lx, expected S_OK\n", hr);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay2A, (void**)&dp2A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay2A failed: %08lx\n", hr);
    IDirectPlay2_Release(dp2A);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay2, (void**)&dp2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay2 failed: %08lx\n", hr);
    IDirectPlay2_Release(dp2);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay3A, (void**)&dp3A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay3A failed: %08lx\n", hr);
    IDirectPlay3_Release(dp3A);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay3, (void**)&dp3);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay3 failed: %08lx\n", hr);
    IDirectPlay3_Release(dp3);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay4A, (void**)&dp4A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay4A failed: %08lx\n", hr);
    IDirectPlayX_Release(dp4A);

    /* IDirectPlay and IUnknown share a refcount */
    hr = IDirectPlayX_QueryInterface(dp4, &IID_IDirectPlay, (void**)&dp);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlay failed: %08lx\n", hr);
    IDirectPlay_Release(dp);

    hr = IDirectPlayX_QueryInterface(dp4, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08lx\n", hr);

    IUnknown_Release(unk);
    refcount = IDirectPlayX_Release(dp4);
    ok(refcount == 0, "refcount == %lu, expected 0\n", refcount);
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
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, (IUnknown*)0xdeadbeef, CLSCTX_INPROC_SERVER,
            &IID_IUnknown, (void**)&dpl);
    ok(hr == CLASS_E_NOAGGREGATION || broken(hr == E_INVALIDARG),
            "DirectPlayLobby create failed: %08lx, expected CLASS_E_NOAGGREGATION\n", hr);
    ok(!dpl || dpl == (IDirectPlayLobby*)0xdeadbeef, "dpl = %p\n", dpl);

    /* Invalid RIID */
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay,
            (void**)&dpl);
    ok(hr == E_NOINTERFACE, "DirectPlayLobby create failed: %08lx, expected E_NOINTERFACE\n", hr);

    /* Different refcount for all DirectPlayLobby Interfaces */
    hr = CoCreateInstance(&CLSID_DirectPlayLobby, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayLobby,
            (void**)&dpl);
    ok(hr == S_OK, "DirectPlayLobby create failed: %08lx, expected S_OK\n", hr);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobbyA, (void**)&dplA);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobbyA failed: %08lx\n", hr);
    IDirectPlayLobby_Release(dplA);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby2, (void**)&dpl2);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby2 failed: %08lx\n", hr);
    IDirectPlayLobby_Release(dpl2);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby2A, (void**)&dpl2A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby2A failed: %08lx\n", hr);
    IDirectPlayLobby_Release(dpl2A);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby3, (void**)&dpl3);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby3 failed: %08lx\n", hr);
    IDirectPlayLobby_Release(dpl3);

    hr = IDirectPlayLobby_QueryInterface(dpl, &IID_IDirectPlayLobby3A, (void**)&dpl3A);
    ok(hr == S_OK, "QueryInterface for IID_IDirectPlayLobby3A failed: %08lx\n", hr);
    IDirectPlayLobby_Release(dpl3A);

    /* IDirectPlayLobby and IUnknown share a refcount */
    hr = IDirectPlayX_QueryInterface(dpl, &IID_IUnknown, (void**)&unk);
    ok(hr == S_OK, "QueryInterface for IID_IUnknown failed: %08lx\n", hr);
    IUnknown_Release(unk);

    refcount = IDirectPlayLobby_Release(dpl);
    ok(refcount == 0, "refcount == %lu, expected 0\n", refcount);
}

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08lx\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

static HRESULT set_firewall( enum firewall_op op )
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );
    WCHAR path[MAX_PATH];

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }

    if(!GetSystemDirectoryW(path, MAX_PATH))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    lstrcatW( path, L"\\dplaysvr.exe" );

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"dplay_client" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;
    if (hr != S_OK) goto done;

    INetFwAuthorizedApplication_Release( app );
    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    SysFreeString( image );
    image = SysAllocString( path );
    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( L"dplay_server" );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08lx\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;

done:
    if (app) INetFwAuthorizedApplication_Release( app );
    if (apps) INetFwAuthorizedApplications_Release( apps );
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    SysFreeString( image );
    return hr;
}

/* taken from programs/winetest/main.c */
static BOOL is_stub_dll(const char *filename)
{
    UINT size;
    DWORD ver;
    BOOL isstub = FALSE;
    char *p, *data;

    size = GetFileVersionInfoSizeA(filename, &ver);
    if (!size) return FALSE;

    data = malloc(size);
    if (!data) return FALSE;

    if (GetFileVersionInfoA(filename, ver, size, data))
    {
        char buf[256];

        sprintf(buf, "\\StringFileInfo\\%04x%04x\\OriginalFilename", MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 1200);
        if (VerQueryValueA(data, buf, (void**)&p, &size))
            isstub = !lstrcmpiA("wcodstub.dll", p);
    }
    free(data);

    return isstub;
}

START_TEST(dplayx)
{
    BOOL firewall_enabled;
    HRESULT hr;
    char path[MAX_PATH];
    HMODULE module;

    if(!GetSystemDirectoryA(path, MAX_PATH))
    {
        skip("Failed to get systems directory\n");
        return;
    }
    strcat(path, "\\dplayx.dll");

    if (!winetest_interactive && (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES || is_stub_dll(path)))
    {
        win_skip("dplayx is missing or a stub dll, skipping tests\n");
        return;
    }

    if ((firewall_enabled = is_firewall_enabled()) && !is_process_elevated())
    {
        skip("no privileges, skipping tests to avoid firewall dialog\n");
        return;
    }

    if (firewall_enabled)
    {
        hr = set_firewall(APP_ADD);
        if (hr != S_OK)
        {
            skip("can't authorize app in firewall %08lx\n", hr);
            return;
        }
    }

    CoInitialize( NULL );

    module = LoadLibraryA("dplayx.dll");

    pDirectPlayEnumerateA = (void *)GetProcAddress(module, "DirectPlayEnumerateA");
    pDirectPlayEnumerateW = (void *)GetProcAddress(module, "DirectPlayEnumerateW");
    pDirectPlayCreate = (void *)GetProcAddress(module, "DirectPlayCreate");

    test_COM();
    test_COM_dplobby();
    test_EnumerateProviders();
    test_DirectPlayCreate();
    test_EnumConnections();
    test_InitializeConnection();
    test_GetCaps();
    test_EnumAddressTypes();
    test_EnumAddresses();
    test_EnumSessions();
    test_Open();
    test_ADDFORWARD();
    test_CreatePlayer();
    test_CREATEPLAYER();
    test_Send();
    test_Receive();
    test_PING();
    test_AddPlayerToGroup();
    test_GROUPDATACHANGED();
    test_GetMessageQueue();

    if (!winetest_interactive)
    {
        skip("Run in interactive mode to run all dplayx tests.\n");
        goto done;
    }

    trace("Running in interactive mode, tests will take a while\n");

    /* test_interactive_Open() takes almost a minute, */
    test_interactive_Open();
    /* test_interactive_EnumSessions takes three minutes */
    test_interactive_EnumSessions();
    test_SessionDesc();

    /* test_interactive_CreatePlayer() takes over a minute */
    test_interactive_CreatePlayer();
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

    test_interactive_Send();
    test_interactive_Receive();
    test_GetMessageCount();
    test_interactive_GetMessageQueue();

    test_remote_data_replication();
    test_host_migration();

done:
    FreeLibrary(module);
    CoUninitialize();
    if (firewall_enabled) set_firewall(APP_REMOVE);
}
