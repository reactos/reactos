/*
 * Copyright 2001 Peter Hunnisett
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

#ifndef __WINE_LOBBY_SP_H
#define __WINE_LOBBY_SP_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "dplobby.h"

/* GUID for IDPLobbySP {5A4E5A20-2CED-11d0-A889-00A0C905433C} */
DEFINE_GUID(IID_IDPLobbySP, 0x5a4e5a20, 0x2ced, 0x11d0, 0xa8, 0x89, 0x0, 0xa0, 0xc9, 0x5, 0x43, 0x3c);
typedef struct IDPLobbySP *LPDPLOBBYSP;

/* For SP. Top 16 bits is dplay, bottom 16 is SP */
#define DPLSP_MAJORVERSION               0x00050000

typedef struct SPDATA_ADDGROUPTOGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwParentID;
  DWORD       dwGroupID;
} SPDATA_ADDGROUPTOGROUP, *LPSPDATA_ADDGROUPTOGROUP;

typedef struct SPDATA_ADDPLAYERTOGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  DWORD       dwPlayerID;
} SPDATA_ADDPLAYERTOGROUP, *LPSPDATA_ADDPLAYERTOGROUP;

typedef struct SPDATA_ADDREMOTEGROUPTOGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD	      dwAnchorID;
  DWORD       dwGroupID;
  DWORD	      dwParentID;
  LPDPNAME    lpName;
  DWORD       dwGroupFlags;
} SPDATA_ADDREMOTEGROUPTOGROUP, *LPSPDATA_ADDREMOTEGROUPTOGROUP;

typedef struct SPDATA_ADDREMOTEPLAYERTOGROUP
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD	      dwGroupID;
  DWORD	      dwPlayerID;
  DWORD	      dwPlayerFlags;
  LPDPNAME    lpName;
} SPDATA_ADDREMOTEPLAYERTOGROUP, *LPSPDATA_ADDREMOTEPLAYERTOGROUP;

typedef struct SPDATA_BUILDPARENTALHIERARCHY
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  DWORD       dwMessage;
  DWORD       dwParentID;
} SPDATA_BUILDPARENTALHIERARCHY, *LPSPDATA_BUILDPARENTALHIERARCHY;

typedef struct SPDATA_CLOSE
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
} SPDATA_CLOSE, *LPSPDATA_CLOSE;

typedef struct SPDATA_CREATEGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_CREATEGROUP, *LPSPDATA_CREATEGROUP;

typedef struct SPDATA_CREATEGROUPINGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwParentID;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_CREATEGROUPINGROUP, *LPSPDATA_CREATEGROUPINGROUP;

typedef struct SPDATA_CREATEREMOTEGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_CREATEREMOTEGROUP, *LPSPDATA_CREATEREMOTEGROUP;

typedef struct SPDATA_CREATEREMOTEGROUPINGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwParentID;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  DWORD       dwFlags;
} SPDATA_CREATEREMOTEGROUPINGROUP, *LPSPDATA_CREATEREMOTEGROUPINGROUP;

typedef struct SPDATA_CREATEPLAYER
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
  LPDPNAME    lpName;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_CREATEPLAYER, *LPSPDATA_CREATEPLAYER;

typedef struct SPDATA_DELETEGROUPFROMGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwParentID;
  DWORD       dwGroupID;
} SPDATA_DELETEGROUPFROMGROUP, *LPSPDATA_DELETEGROUPFROMGROUP;

typedef struct SPDATA_DELETEPLAYERFROMGROUP
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  DWORD       dwPlayerID;
} SPDATA_DELETEPLAYERFROMGROUP, *LPSPDATA_DELETEPLAYERFROMGROUP;

typedef struct SPDATA_DELETEREMOTEGROUPFROMGROUP
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwParentID;
  DWORD       dwGroupID;
} SPDATA_DELETEREMOTEGROUPFROMGROUP, *LPSPDATA_DELETEREMOTEGROUPFROMGROUP;

typedef struct SPDATA_DELETEREMOTEPLAYERFROMGROUP
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  DWORD       dwPlayerID;
} SPDATA_DELETEREMOTEPLAYERFROMGROUP, *LPSPDATA_DELETEREMOTEPLAYERFROMGROUP;

typedef struct SPDATA_DESTROYGROUP
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD	      dwGroupID;
} SPDATA_DESTROYGROUP, *LPSPDATA_DESTROYGROUP;

typedef struct SPDATA_DESTROYREMOTEGROUP
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
} SPDATA_DESTROYREMOTEGROUP, *LPSPDATA_DESTROYREMOTEGROUP;

typedef struct SPDATA_DESTROYPLAYER
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
} SPDATA_DESTROYPLAYER, *LPSPDATA_DESTROYPLAYER;

typedef struct SPDATA_ENUMSESSIONS
{
  DWORD            dwSize;
  LPDPLOBBYSP      lpISP;
  LPDPSESSIONDESC2 lpsd;
  DWORD            dwTimeout;
  DWORD            dwFlags;
} SPDATA_ENUMSESSIONS, *LPSPDATA_ENUMSESSIONS;

typedef struct SPDATA_ENUMSESSIONSRESPONSE
{
  DWORD            dwSize;
  LPDPSESSIONDESC2 lpsd;
} SPDATA_ENUMSESSIONSRESPONSE, *LPSPDATA_ENUMSESSIONSRESPONSE;

typedef struct SPDATA_GETCAPS
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwFlags;
  LPDPCAPS    lpcaps;
} SPDATA_GETCAPS, *LPSPDATA_GETCAPS;

typedef struct SPDATA_GETGROUPCONNECTIONSETTINGS
{
  DWORD       dwSize;
  DWORD       dwFlags;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDWORD     lpdwBufferSize;
  LPVOID      lpBuffer;
} SPDATA_GETGROUPCONNECTIONSETTINGS, *LPSPDATA_GETGROUPCONNECTIONSETTINGS;

typedef struct SPDATA_GETGROUPDATA
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDWORD     lpdwDataSize;
  LPVOID      lpData;
} SPDATA_GETGROUPDATA, *LPSPDATA_GETGROUPDATA;

typedef struct SPDATA_GETPLAYERCAPS
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwFlags;
  DWORD       dwPlayerID;
  LPDPCAPS    lpcaps;
} SPDATA_GETPLAYERCAPS, *LPSPDATA_GETPLAYERCAPS;

typedef struct SPDATA_GETPLAYERDATA
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
  LPDWORD     lpdwDataSize;
  LPVOID      lpData;
} SPDATA_GETPLAYERDATA, *LPSPDATA_GETPLAYERDATA;

typedef struct SPDATA_HANDLEMESSAGE
{
  DWORD	 dwSize;
  DWORD	 dwFromID;
  DWORD	 dwToID;
  LPVOID lpBuffer;
  DWORD	 dwBufSize;
} SPDATA_HANDLEMESSAGE, *LPSPDATA_HANDLEMESSAGE;

typedef struct SPDATA_OPEN
{
  DWORD	           dwSize;
  LPDPLOBBYSP      lpISP;
  LPDPSESSIONDESC2 lpsd;
  DWORD            dwFlags;
  LPCDPCREDENTIALS lpCredentials;
} SPDATA_OPEN, *LPSPDATA_OPEN;

typedef struct SPDATA_SEND
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwFromID;
  DWORD       dwToID;
  DWORD       dwFlags;
  LPVOID      lpBuffer;
  DWORD       dwBufSize;
} SPDATA_SEND, *LPSPDATA_SEND;

typedef struct SPDATA_CHATMESSAGE
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwFromID;
  DWORD       dwToID;
  DWORD       dwFlags;
  LPDPCHAT    lpChat;
} SPDATA_CHATMESSAGE, *LPSPDATA_CHATMESSAGE;

typedef struct SPDATA_SETGROUPCONNECTIONSETTINGS
{
  DWORD           dwSize;
  DWORD           dwFlags;
  LPDPLOBBYSP     lpISP;
  DWORD           dwGroupID;
  LPDPLCONNECTION lpConn;
} SPDATA_SETGROUPCONNECTIONSETTINGS, *LPSPDATA_SETGROUPCONNECTIONSETTINGS;

typedef struct SPDATA_SETGROUPDATA
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_SETGROUPDATA, *LPSPDATA_SETGROUPDATA;

typedef struct SPDATA_SETGROUPNAME
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  DWORD       dwFlags;
} SPDATA_SETGROUPNAME, *LPSPDATA_SETGROUPNAME;

typedef struct SPDATA_SETREMOTEGROUPNAME
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwGroupID;
  LPDPNAME    lpName;
  DWORD       dwFlags;
} SPDATA_SETREMOTEGROUPNAME, *LPSPDATA_SETREMOTEGROUPNAME;

typedef struct SPDATA_SETPLAYERDATA
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
  LPVOID      lpData;
  DWORD       dwDataSize;
  DWORD       dwFlags;
} SPDATA_SETPLAYERDATA, *LPSPDATA_SETPLAYERDATA;

typedef struct SPDATA_SETPLAYERNAME
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
  LPDPNAME    lpName;
  DWORD	      dwFlags;
} SPDATA_SETPLAYERNAME, *LPSPDATA_SETPLAYERNAME;

typedef struct SPDATA_SETREMOTEPLAYERNAME
{
  DWORD	      dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwPlayerID;
  LPDPNAME    lpName;
  DWORD       dwFlags;
} SPDATA_SETREMOTEPLAYERNAME, *LPSPDATA_SETREMOTEPLAYERNAME;

typedef struct SPDATA_SETSESSIONDESC
{
  DWORD            dwSize;
  LPDPSESSIONDESC2 lpsd;
} SPDATA_SETSESSIONDESC, *LPSPDATA_SETSESSIONDESC;

typedef struct SPDATA_SHUTDOWN
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
} SPDATA_SHUTDOWN, *LPSPDATA_SHUTDOWN;

typedef struct SPDATA_STARTSESSION
{
  DWORD       dwSize;
  LPDPLOBBYSP lpISP;
  DWORD       dwFlags;
  DWORD       dwGroupID;
} SPDATA_STARTSESSION, *LPSPDATA_STARTSESSION;

typedef struct SPDATA_STARTSESSIONCOMMAND
{
  DWORD           dwFlags;
  DWORD           dwGroupID;
  DWORD           dwHostID;
  LPDPLCONNECTION lpConn;
} SPDATA_STARTSESSIONCOMMAND, *LPSPDATA_STARTSESSIONCOMMAND;

/* Prototypes for callbacks returned by DPLSPInit */
typedef HRESULT (WINAPI *LPSP_ADDGROUPTOGROUP)(LPSPDATA_ADDGROUPTOGROUP);
typedef HRESULT	(WINAPI *LPSP_ADDPLAYERTOGROUP)(LPSPDATA_ADDPLAYERTOGROUP);
typedef HRESULT	(WINAPI *LPSP_BUILDPARENTALHIERARCHY)(LPSPDATA_BUILDPARENTALHIERARCHY);
typedef HRESULT	(WINAPI *LPSP_CLOSE)(LPSPDATA_CLOSE);
typedef HRESULT	(WINAPI *LPSP_CREATEGROUP)(LPSPDATA_CREATEGROUP);
typedef HRESULT (WINAPI *LPSP_CREATEGROUPINGROUP)(LPSPDATA_CREATEGROUPINGROUP);
typedef HRESULT	(WINAPI *LPSP_CREATEPLAYER)(LPSPDATA_CREATEPLAYER);
typedef HRESULT (WINAPI *LPSP_DELETEGROUPFROMGROUP)(LPSPDATA_DELETEGROUPFROMGROUP);
typedef HRESULT	(WINAPI *LPSP_DELETEPLAYERFROMGROUP)(LPSPDATA_DELETEPLAYERFROMGROUP);
typedef HRESULT	(WINAPI *LPSP_DESTROYGROUP)(LPSPDATA_DESTROYGROUP);
typedef HRESULT	(WINAPI *LPSP_DESTROYPLAYER)(LPSPDATA_DESTROYPLAYER);
typedef HRESULT	(WINAPI *LPSP_ENUMSESSIONS)(LPSPDATA_ENUMSESSIONS);
typedef HRESULT (WINAPI *LPSP_GETCAPS)(LPSPDATA_GETCAPS);
typedef HRESULT (WINAPI *LPSP_GETGROUPCONNECTIONSETTINGS)(LPSPDATA_GETGROUPCONNECTIONSETTINGS);
typedef HRESULT	(WINAPI *LPSP_GETGROUPDATA)(LPSPDATA_GETGROUPDATA);
typedef HRESULT (WINAPI *LPSP_GETPLAYERCAPS)(LPSPDATA_GETPLAYERCAPS);
typedef HRESULT	(WINAPI *LPSP_GETPLAYERDATA)(LPSPDATA_GETPLAYERDATA);
typedef HRESULT	(WINAPI *LPSP_HANDLEMESSAGE)(LPSPDATA_HANDLEMESSAGE);
typedef HRESULT	(WINAPI *LPSP_OPEN)(LPSPDATA_OPEN);
typedef HRESULT	(WINAPI *LPSP_SEND)(LPSPDATA_SEND);
typedef HRESULT	(WINAPI *LPSP_SENDCHATMESSAGE)(LPSPDATA_CHATMESSAGE);
typedef HRESULT (WINAPI *LPSP_SETGROUPCONNECTIONSETTINGS)(LPSPDATA_SETGROUPCONNECTIONSETTINGS);
typedef HRESULT	(WINAPI *LPSP_SETGROUPDATA)(LPSPDATA_SETGROUPDATA);
typedef HRESULT	(WINAPI *LPSP_SETGROUPNAME)(LPSPDATA_SETGROUPNAME);
typedef HRESULT	(WINAPI *LPSP_SETPLAYERDATA)(LPSPDATA_SETPLAYERDATA);
typedef HRESULT	(WINAPI *LPSP_SETPLAYERNAME)(LPSPDATA_SETPLAYERNAME);
typedef HRESULT	(WINAPI *LPSP_SHUTDOWN)(LPSPDATA_SHUTDOWN);
typedef HRESULT (WINAPI *LPSP_STARTSESSION)(LPSPDATA_STARTSESSION);

/* Callback table for dplay to call into service provider */
typedef struct SP_CALLBACKS
{
  DWORD                            dwSize;
  DWORD                            dwFlags;
  LPSP_ADDGROUPTOGROUP             AddGroupToGroup;
  LPSP_ADDPLAYERTOGROUP            AddPlayerToGroup;
  LPSP_BUILDPARENTALHIERARCHY      BuildParentalHierarchy;
  LPSP_CLOSE                       Close;
  LPSP_CREATEGROUP                 CreateGroup;
  LPSP_CREATEGROUPINGROUP          CreateGroupInGroup;
  LPSP_CREATEPLAYER                CreatePlayer;
  LPSP_DELETEGROUPFROMGROUP        DeleteGroupFromGroup;
  LPSP_DELETEPLAYERFROMGROUP       DeletePlayerFromGroup;
  LPSP_DESTROYGROUP                DestroyGroup;
  LPSP_DESTROYPLAYER               DestroyPlayer;
  LPSP_ENUMSESSIONS                EnumSessions;
  LPSP_GETCAPS                     GetCaps;
  LPSP_GETGROUPCONNECTIONSETTINGS  GetGroupConnectionSettings;
  LPSP_GETGROUPDATA                GetGroupData;
  LPSP_GETPLAYERCAPS               GetPlayerCaps;
  LPSP_GETPLAYERDATA               GetPlayerData;
  LPSP_OPEN                        Open;
  LPSP_SEND                        Send;
  LPSP_SENDCHATMESSAGE	           SendChatMessage;
  LPSP_SETGROUPCONNECTIONSETTINGS  SetGroupConnectionSettings;
  LPSP_SETGROUPDATA                SetGroupData;
  LPSP_SETGROUPNAME                SetGroupName;
  LPSP_SETPLAYERDATA               SetPlayerData;
  LPSP_SETPLAYERNAME               SetPlayerName;
  LPSP_SHUTDOWN                    Shutdown;
  LPSP_STARTSESSION                StartSession;
} SP_CALLBACKS, *LPSP_CALLBACKS;

typedef struct SPDATA_INIT
{
  LPSP_CALLBACKS lpCB;
  DWORD          dwSPVersion;
  LPDPLOBBYSP    lpISP;
  LPDPADDRESS    lpAddress;
} SPDATA_INIT, *LPSPDATA_INIT;

typedef HRESULT (WINAPI *LPSP_INIT)(LPSPDATA_INIT);

/* Define the COM interface */
#define INTERFACE IDPLobbySP
DECLARE_INTERFACE_(IDPLobbySP,IUnknown)
{
    /*** IUnknown methods ***/
    STDMETHOD_(HRESULT,QueryInterface)(THIS_ REFIID riid, void** ppvObject) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    /*** IDPLobbySP methods ***/
    STDMETHOD(AddGroupToGroup)(THIS_ LPSPDATA_ADDREMOTEGROUPTOGROUP  argtg ) PURE;
    STDMETHOD(AddPlayerToGroup)(THIS_ LPSPDATA_ADDREMOTEPLAYERTOGROUP  arptg ) PURE;
    STDMETHOD(CreateGroup)(THIS_ LPSPDATA_CREATEREMOTEGROUP  crg ) PURE;
    STDMETHOD(CreateGroupInGroup)(THIS_ LPSPDATA_CREATEREMOTEGROUPINGROUP  crgig ) PURE;
    STDMETHOD(DeleteGroupFromGroup)(THIS_ LPSPDATA_DELETEREMOTEGROUPFROMGROUP  drgfg ) PURE;
    STDMETHOD(DeletePlayerFromGroup)(THIS_ LPSPDATA_DELETEREMOTEPLAYERFROMGROUP  drpfg ) PURE;
    STDMETHOD(DestroyGroup)(THIS_ LPSPDATA_DESTROYREMOTEGROUP  drg ) PURE;
    STDMETHOD(EnumSessionsResponse)(THIS_ LPSPDATA_ENUMSESSIONSRESPONSE  er ) PURE;
    STDMETHOD(GetSPDataPointer)(THIS_ LPVOID * lplpData ) PURE;
    STDMETHOD(HandleMessage)(THIS_ LPSPDATA_HANDLEMESSAGE  hm ) PURE;
    STDMETHOD(SendChatMessage)(THIS_ LPSPDATA_CHATMESSAGE  cm ) PURE;
    STDMETHOD(SetGroupName)(THIS_ LPSPDATA_SETREMOTEGROUPNAME  srgn ) PURE;
    STDMETHOD(SetPlayerName)(THIS_ LPSPDATA_SETREMOTEPLAYERNAME  srpn ) PURE;
    STDMETHOD(SetSessionDesc)(THIS_ LPSPDATA_SETSESSIONDESC  ssd ) PURE;
    STDMETHOD(SetSPDataPointer)(THIS_ LPVOID  lpData ) PURE;
    STDMETHOD(StartSession)(THIS_ LPSPDATA_STARTSESSIONCOMMAND  ssc ) PURE;
};
#undef INTERFACE

#if !defined (__cplusplus) || defined(CINTERFACE)
/*** IUnknown methods ***/
#define IDPLobbySP_QueryInterface(p,a,b)         (p)->lpVtbl->QueryInterface(p,a,b)
#define IDPLobbySP_AddRef(p)                     (p)->lpVtbl->AddRef(p)
#define IDPLobbySP_Release(p)                    (p)->lpVtbl->Release(p)

/*** IDPLobbySP methods ***/
#define IDPLobbySP_AddGroupToGroup(p,a)          (p)->lpVtbl->AddGroupToGroup(p,a)
#define IDPLobbySP_AddPlayerToGroup(p,a)         (p)->lpVtbl->AddPlayerToGroup(p,a)
#define IDPLobbySP_CreateGroup(p,a)              (p)->lpVtbl->CreateGroup(p,a)
#define IDPLobbySP_CreateGroupInGroup(p,a)       (p)->lpVtbl->CreateGroupInGroup(p,a)
#define IDPLobbySP_DeleteGroupFromGroup(p,a)     (p)->lpVtbl->DeleteGroupFromGroup(p,a)
#define IDPLobbySP_DeletePlayerFromGroup(p,a)    (p)->lpVtbl->DeletePlayerFromGroup(p,a)
#define IDPLobbySP_DestroyGroup(p,a)             (p)->lpVtbl->DestroyGroup(p,a)
#define IDPLobbySP_EnumSessionsResponse(p,a)     (p)->lpVtbl->EnumSessionsResponse(p,a)
#define IDPLobbySP_GetSPDataPointer(p,a)         (p)->lpVtbl->GetSPDataPointer(p,a)
#define IDPLobbySP_HandleMessage(p,a)            (p)->lpVtbl->HandleMessage(p,a)
#define IDPLobbySP_SetGroupName(p,a)             (p)->lpVtbl->SetGroupName(p,a)
#define IDPLobbySP_SetPlayerName(p,a)            (p)->lpVtbl->SetPlayerName(p,a)
#define IDPLobbySP_SetSessionDesc(p,a)           (p)->lpVtbl->SetSessionDesc(p,a)
#define IDPLobbySP_StartSession(p,a)             (p)->lpVtbl->StartSession(p,a)
#define IDPLobbySP_SetSPDataPointer(p,a)         (p)->lpVtbl->SetSPDataPointer(p,a)
#endif

/* This variable is exported from the DLL at ordinal 6 to be accessed by the
 * SP directly. This is the same variable that the DP SP will use.
 */
extern DWORD gdwDPlaySPRefCount;

#endif
