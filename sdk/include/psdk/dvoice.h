 /*
 * DirectPlay Voice Interfaces
 *
 * Copyright (C) 2014 Alistair Leslie-Hughes
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
#ifndef __DVOICE_INCLUDED__
#define __DVOICE_INCLUDED__

#include <ole2.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <dsound.h>

#ifdef __cplusplus
extern "C" {
#endif


DEFINE_GUID(CLSID_DirectPlayVoiceClient, 0xb9f3eb85, 0xb781, 0x4ac1, 0x8d, 0x90, 0x93, 0xa0, 0x5e, 0xe3, 0x7d, 0x7d);
DEFINE_GUID(CLSID_DirectPlayVoiceServer, 0xd3f5b8e6, 0x9b78, 0x4a4c, 0x94, 0xea, 0xca, 0x23, 0x97, 0xb6, 0x63, 0xd3);
DEFINE_GUID(CLSID_DirectPlayVoiceTest,   0x0f0f094b, 0xb01c, 0x4091, 0xa1, 0x4d, 0xdd, 0x0c, 0xd8, 0x07, 0x71, 0x1a);

DEFINE_GUID(IID_IDirectPlayVoiceClient,  0x1dfdc8ea, 0xbcf7, 0x41d6, 0xb2, 0x95, 0xab, 0x64, 0xb3, 0xb2, 0x33, 0x06);
DEFINE_GUID(IID_IDirectPlayVoiceServer,  0xfaa1c173, 0x0468, 0x43b6, 0x8a, 0x2a, 0xea, 0x8a, 0x4f, 0x20, 0x76, 0xc9);
DEFINE_GUID(IID_IDirectPlayVoiceTest,    0xd26af734, 0x208b, 0x41da, 0x82, 0x24, 0xe0, 0xce, 0x79, 0x81, 0x0b, 0xe1);

DEFINE_GUID(DPVCTGUID_ADPCM,             0x699b52c1, 0xa885, 0x46a8, 0xa3, 0x08, 0x97, 0x17, 0x24, 0x19, 0xad, 0xc7);
DEFINE_GUID(DPVCTGUID_GSM,               0x24768c60, 0x5a0d, 0x11d3, 0x9b, 0xe4, 0x52, 0x54, 0x00, 0xd9, 0x85, 0xe7);
DEFINE_GUID(DPVCTGUID_NONE,              0x8de12fd4, 0x7cb3, 0x48ce, 0xa7, 0xe8, 0x9c, 0x47, 0xa2, 0x2e, 0x8a, 0xc5);
DEFINE_GUID(DPVCTGUID_SC03,              0x7d82a29b, 0x2242, 0x4f82, 0x8f, 0x39, 0x5d, 0x11, 0x53, 0xdf, 0x3e, 0x41);
DEFINE_GUID(DPVCTGUID_SC06,              0x53def900, 0x7168, 0x4633, 0xb4, 0x7f, 0xd1, 0x43, 0x91, 0x6a, 0x13, 0xc7);
DEFINE_GUID(DPVCTGUID_TRUESPEECH,        0xd7954361, 0x5a0b, 0x11d3, 0x9b, 0xe4, 0x52, 0x54, 0x00, 0xd9, 0x85, 0xe7);
DEFINE_GUID(DPVCTGUID_VR12,              0xfe44a9fe, 0x8ed4, 0x48bf, 0x9d, 0x66, 0x1b, 0x1a, 0xdf, 0xf9, 0xff, 0x6d);

#define DPVCTGUID_DEFAULT    DPVCTGUID_SC03

typedef struct IDirectPlayVoiceClient *LPDIRECTPLAYVOICECLIENT, *PDIRECTPLAYVOICECLIENT;
typedef struct IDirectPlayVoiceServer *LPDIRECTPLAYVOICESERVER, *PDIRECTPLAYVOICESERVER;
typedef struct IDirectPlayVoiceTest   *LPDIRECTPLAYVOICETEST,   *PDIRECTPLAYVOICETEST;


typedef HRESULT (PASCAL *PDVMESSAGEHANDLER)(PVOID pvUserContext, DWORD dwMessageType, LPVOID lpMessage);
typedef PDVMESSAGEHANDLER LPDVMESSAGEHANDLER;

typedef DWORD DVID, *LPDVID, *PDVID;

#define DVMSGID_MINBASE                     DVMSGID_CREATEVOICEPLAYER
#define DVMSGID_CREATEVOICEPLAYER           0x0001
#define DVMSGID_DELETEVOICEPLAYER           0x0002
#define DVMSGID_SESSIONLOST                 0x0003
#define DVMSGID_PLAYERVOICESTART            0x0004
#define DVMSGID_PLAYERVOICESTOP             0x0005
#define DVMSGID_RECORDSTART                 0x0006
#define DVMSGID_RECORDSTOP                  0x0007
#define DVMSGID_CONNECTRESULT               0x0008
#define DVMSGID_DISCONNECTRESULT            0x0009
#define DVMSGID_INPUTLEVEL                  0x000A
#define DVMSGID_OUTPUTLEVEL                 0x000B
#define DVMSGID_HOSTMIGRATED                0x000C
#define DVMSGID_SETTARGETS                  0x000D
#define DVMSGID_PLAYEROUTPUTLEVEL           0x000E
#define DVMSGID_LOSTFOCUS                   0x0010
#define DVMSGID_GAINFOCUS                   0x0011
#define DVMSGID_LOCALHOSTSETUP              0x0012
#define DVMSGID_MAXBASE                     DVMSGID_LOCALHOSTSETUP

#define DVBUFFERAGGRESSIVENESS_MIN          0x00000001
#define DVBUFFERAGGRESSIVENESS_MAX          0x00000064
#define DVBUFFERAGGRESSIVENESS_DEFAULT      0x00000000

#define DVBUFFERQUALITY_MIN                 0x00000001
#define DVBUFFERQUALITY_MAX                 0x00000064
#define DVBUFFERQUALITY_DEFAULT             0x00000000

#define DVID_SERVERPLAYER                   1
#define DVID_ALLPLAYERS                     0
#define DVID_REMAINING                      0xFFFFFFFF

#define DVINPUTLEVEL_MIN                    0x00000000
#define DVINPUTLEVEL_MAX                    0x00000063

#define DVNOTIFYPERIOD_MINPERIOD            20

#define DVPLAYBACKVOLUME_DEFAULT            DSBVOLUME_MAX

#define DVRECORDVOLUME_LAST                 0x00000001


#define DVTHRESHOLD_DEFAULT                 0xFFFFFFFF
#define DVTHRESHOLD_MIN                     0x00000000
#define DVTHRESHOLD_MAX                     0x00000063
#define DVTHRESHOLD_UNUSED                  0xFFFFFFFE


#define DVSESSIONTYPE_PEER                  0x00000001
#define DVSESSIONTYPE_MIXING                0x00000002
#define DVSESSIONTYPE_FORWARDING            0x00000003
#define DVSESSIONTYPE_ECHO                  0x00000004

#define DVCLIENTCONFIG_RECORDMUTE           0x00000001
#define DVCLIENTCONFIG_PLAYBACKMUTE         0x00000002
#define DVCLIENTCONFIG_MANUALVOICEACTIVATED 0x00000004
#define DVCLIENTCONFIG_AUTORECORDVOLUME     0x00000008
#define DVCLIENTCONFIG_MUTEGLOBAL           0x00000010
#define DVCLIENTCONFIG_AUTOVOICEACTIVATED   0x00000020
#define DVCLIENTCONFIG_ECHOSUPPRESSION      0x08000000

#define DVFLAGS_SYNC                        0x00000001
#define DVFLAGS_QUERYONLY                   0x00000002
#define DVFLAGS_NOHOSTMIGRATE               0x00000008
#define DVFLAGS_ALLOWBACK                   0x00000010

#define DVSESSION_NOHOSTMIGRATION           0x00000001
#define DVSESSION_SERVERCONTROLTARGET       0x00000002

#define DVSOUNDCONFIG_NORMALMODE            0x00000001
#define DVSOUNDCONFIG_AUTOSELECT            0x00000002
#define DVSOUNDCONFIG_HALFDUPLEX            0x00000004
#define DVSOUNDCONFIG_NORECVOLAVAILABLE     0x00000010
#define DVSOUNDCONFIG_NOFOCUS               0x20000000

#define DVSOUNDCONFIG_SETCONVERSIONQUALITY  0x00000008
#define DVSOUNDCONFIG_STRICTFOCUS           0x40000000
#define DVPLAYERCAPS_HALFDUPLEX             0x00000001
#define DVPLAYERCAPS_LOCAL                  0x00000002

typedef struct
{
    DWORD   dwSize;
    DWORD   dwFlags;
} DVCAPS, *LPDVCAPS, *PDVCAPS;

typedef struct
{
    DWORD   dwSize;
    DWORD   dwFlags;
    LONG    lRecordVolume;
    LONG    lPlaybackVolume;
    DWORD   dwThreshold;
    DWORD   dwBufferQuality;
    DWORD   dwBufferAggressiveness;
    DWORD   dwNotifyPeriod;
} DVCLIENTCONFIG, *LPDVCLIENTCONFIG, *PDVCLIENTCONFIG;

typedef struct
{
    DWORD   dwSize;
    GUID    guidType;
    LPWSTR  lpszName;
    LPWSTR  lpszDescription;
    DWORD   dwFlags;
    DWORD   dwMaxBitsPerSecond;
} DVCOMPRESSIONINFO, *LPDVCOMPRESSIONINFO, *PDVCOMPRESSIONINFO;

typedef struct
{
    DWORD   dwSize;
    DWORD   dwFlags;
    DWORD   dwSessionType;
    GUID    guidCT;
    DWORD   dwBufferQuality;
    DWORD   dwBufferAggressiveness;
} DVSESSIONDESC, *LPDVSESSIONDESC, *PDVSESSIONDESC;

typedef struct
{
    DWORD                   dwSize;
    DWORD                   dwFlags;
    GUID                    guidPlaybackDevice;
    LPDIRECTSOUND           lpdsPlaybackDevice;
    GUID                    guidCaptureDevice;
    LPDIRECTSOUNDCAPTURE    lpdsCaptureDevice;
    HWND                    hwndAppWindow;
    LPDIRECTSOUNDBUFFER     lpdsMainBuffer;
    DWORD                   dwMainBufferFlags;
    DWORD                   dwMainBufferPriority;
} DVSOUNDDEVICECONFIG, *LPDVSOUNDDEVICECONFIG, *PDVSOUNDDEVICECONFIG;

typedef struct
{
    DWORD    dwSize;
    HRESULT  hrResult;
} DVMSG_CONNECTRESULT, *LPDVMSG_CONNECTRESULT, *PDVMSG_CONNECTRESULT;

typedef struct
{
    DWORD    dwSize;
    DVID     dvidPlayer;
    DWORD    dwFlags;
    PVOID    pvPlayerContext;
} DVMSG_CREATEVOICEPLAYER, *LPDVMSG_CREATEVOICEPLAYER, *PDVMSG_CREATEVOICEPLAYER;

typedef struct
{
    DWORD   dwSize;
    DVID    dvidPlayer;
    PVOID	pvPlayerContext;
} DVMSG_DELETEVOICEPLAYER, *LPDVMSG_DELETEVOICEPLAYER, *PDVMSG_DELETEVOICEPLAYER;

typedef struct
{
    DWORD   dwSize;
    HRESULT hrResult;
} DVMSG_DISCONNECTRESULT, *LPDVMSG_DISCONNECTRESULT, *PDVMSG_DISCONNECTRESULT;

typedef struct
{
    DWORD                   dwSize;
    DVID                    dvidNewHostID;
    LPDIRECTPLAYVOICESERVER pdvServerInterface;
} DVMSG_HOSTMIGRATED, *LPDVMSG_HOSTMIGRATED, *PDVMSG_HOSTMIGRATED;

typedef struct
{
    DWORD    dwSize;
    DWORD    dwPeakLevel;
    LONG     lRecordVolume;
    PVOID    pvLocalPlayerContext;
} DVMSG_INPUTLEVEL, *LPDVMSG_INPUTLEVEL, *PDVMSG_INPUTLEVEL;

typedef struct
{
    DWORD               dwSize;
    PVOID               pvContext;
    PDVMESSAGEHANDLER   pMessageHandler;
} DVMSG_LOCALHOSTSETUP, *LPDVMSG_LOCALHOSTSETUP, *PDVMSG_LOCALHOSTSETUP;

typedef struct
{
    DWORD   dwSize;
    DWORD   dwPeakLevel;
    LONG    lOutputVolume;
    PVOID   pvLocalPlayerContext;
} DVMSG_OUTPUTLEVEL, *LPDVMSG_OUTPUTLEVEL, *PDVMSG_OUTPUTLEVEL;

typedef struct
{
    DWORD   dwSize;
    DVID    dvidSourcePlayerID;
    DWORD   dwPeakLevel;
    PVOID   pvPlayerContext;
} DVMSG_PLAYEROUTPUTLEVEL, *LPDVMSG_PLAYEROUTPUTLEVEL, *PDVMSG_PLAYEROUTPUTLEVEL;

typedef struct
{
    DWORD   dwSize;
    DVID    dvidSourcePlayerID;
    PVOID   pvPlayerContext;
} DVMSG_PLAYERVOICESTART, *LPDVMSG_PLAYERVOICESTART, *PDVMSG_PLAYERVOICESTART;

typedef struct
{
    DWORD    dwSize;
    DVID     dvidSourcePlayerID;
    PVOID    pvPlayerContext;
} DVMSG_PLAYERVOICESTOP, *LPDVMSG_PLAYERVOICESTOP, *PDVMSG_PLAYERVOICESTOP;

typedef struct
{
    DWORD    dwSize;
    DWORD    dwPeakLevel;
    PVOID    pvLocalPlayerContext;
} DVMSG_RECORDSTART, *LPDVMSG_RECORDSTART, *PDVMSG_RECORDSTART;

typedef struct
{
    DWORD    dwSize;
    DWORD    dwPeakLevel;
    PVOID    pvLocalPlayerContext;
} DVMSG_RECORDSTOP, *LPDVMSG_RECORDSTOP, *PDVMSG_RECORDSTOP;

typedef struct
{
    DWORD    dwSize;
    HRESULT  hrResult;
} DVMSG_SESSIONLOST, *LPDVMSG_SESSIONLOST, *PDVMSG_SESSIONLOST;

typedef struct
{
    DWORD    dwSize;
    DWORD    dwNumTargets;
    PDVID    pdvidTargets;
} DVMSG_SETTARGETS, *LPDVMSG_SETTARGETS, *PDVMSG_SETTARGETS;


#define _FACDPV  0x15
#define MAKE_DVHRESULT( code )          MAKE_HRESULT( 1, _FACDPV, code )

#define DV_OK                           S_OK
#define DV_FULLDUPLEX                   MAKE_HRESULT(0, _FACDPV, 0x0005)
#define DV_HALFDUPLEX                   MAKE_HRESULT(0, _FACDPV, 0x000A)
#define DV_PENDING                      MAKE_HRESULT(0, _FACDPV, 0x0010)

#define DVERR_BUFFERTOOSMALL            MAKE_DVHRESULT(0x001E)
#define DVERR_EXCEPTION                 MAKE_DVHRESULT(0x004A)
#define DVERR_GENERIC                   E_FAIL
#define DVERR_INVALIDFLAGS              MAKE_DVHRESULT(0x0078)
#define DVERR_INVALIDOBJECT             MAKE_DVHRESULT(0x0082)
#define DVERR_INVALIDPARAM              E_INVALIDARG
#define DVERR_INVALIDPLAYER             MAKE_DVHRESULT(0x0087)
#define DVERR_INVALIDGROUP              MAKE_DVHRESULT(0x0091)
#define DVERR_INVALIDHANDLE             MAKE_DVHRESULT(0x0096)
#define DVERR_OUTOFMEMORY               E_OUTOFMEMORY
#define DVERR_PENDING                   DV_PENDING
#define DVERR_NOTSUPPORTED              E_NOTIMPL
#define DVERR_NOINTERFACE               E_NOINTERFACE
#define DVERR_SESSIONLOST               MAKE_DVHRESULT(0x012C)
#define DVERR_NOVOICESESSION            MAKE_DVHRESULT(0x012E)
#define DVERR_CONNECTIONLOST            MAKE_DVHRESULT(0x0168)
#define DVERR_NOTINITIALIZED            MAKE_DVHRESULT(0x0169)
#define DVERR_CONNECTED                 MAKE_DVHRESULT(0x016A)
#define DVERR_NOTCONNECTED              MAKE_DVHRESULT(0x016B)
#define DVERR_CONNECTABORTING           MAKE_DVHRESULT(0x016E)
#define DVERR_NOTALLOWED                MAKE_DVHRESULT(0x016F)
#define DVERR_INVALIDTARGET             MAKE_DVHRESULT(0x0170)
#define DVERR_TRANSPORTNOTHOST          MAKE_DVHRESULT(0x0171)
#define DVERR_COMPRESSIONNOTSUPPORTED   MAKE_DVHRESULT(0x0172)
#define DVERR_ALREADYPENDING            MAKE_DVHRESULT(0x0173)
#define DVERR_SOUNDINITFAILURE          MAKE_DVHRESULT(0x0174)
#define DVERR_TIMEOUT                   MAKE_DVHRESULT(0x0175)
#define DVERR_CONNECTABORTED            MAKE_DVHRESULT(0x0176)
#define DVERR_NO3DSOUND                 MAKE_DVHRESULT(0x0177)
#define DVERR_ALREADYBUFFERED           MAKE_DVHRESULT(0x0178)
#define DVERR_NOTBUFFERED               MAKE_DVHRESULT(0x0179)
#define DVERR_HOSTING                   MAKE_DVHRESULT(0x017A)
#define DVERR_NOTHOSTING                MAKE_DVHRESULT(0x017B)
#define DVERR_INVALIDDEVICE             MAKE_DVHRESULT(0x017C)
#define DVERR_RECORDSYSTEMERROR         MAKE_DVHRESULT(0x017D)
#define DVERR_PLAYBACKSYSTEMERROR       MAKE_DVHRESULT(0x017E)
#define DVERR_SENDERROR                 MAKE_DVHRESULT(0x017F)
#define DVERR_USERCANCEL                MAKE_DVHRESULT(0x0180)
#define DVERR_RUNSETUP                  MAKE_DVHRESULT(0x0183)
#define DVERR_INCOMPATIBLEVERSION       MAKE_DVHRESULT(0x0184)
#define DVERR_INITIALIZED               MAKE_DVHRESULT(0x0187)
#define DVERR_INVALIDPOINTER            E_POINTER
#define DVERR_NOTRANSPORT               MAKE_DVHRESULT(0x0188)
#define DVERR_NOCALLBACK                MAKE_DVHRESULT(0x0189)
#define DVERR_TRANSPORTNOTINIT          MAKE_DVHRESULT(0x018A)
#define DVERR_TRANSPORTNOSESSION        MAKE_DVHRESULT(0x018B)
#define DVERR_TRANSPORTNOPLAYER         MAKE_DVHRESULT(0x018C)
#define DVERR_USERBACK                  MAKE_DVHRESULT(0x018D)
#define DVERR_NORECVOLAVAILABLE         MAKE_DVHRESULT(0x018E)
#define DVERR_INVALIDBUFFER             MAKE_DVHRESULT(0x018F)
#define DVERR_LOCKEDBUFFER              MAKE_DVHRESULT(0x0190)

#undef INTERFACE
#define INTERFACE IDirectPlayVoiceClient
DECLARE_INTERFACE_(IDirectPlayVoiceClient, IUnknown)
{
    STDMETHOD(QueryInterface)      (THIS_ REFIID riid, PVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)       (THIS) PURE;
    STDMETHOD_(ULONG,Release)      (THIS) PURE;
    STDMETHOD(Initialize)          (THIS_ LPUNKNOWN, PDVMESSAGEHANDLER, PVOID, PDWORD, DWORD) PURE;
    STDMETHOD(Connect)             (THIS_ PDVSOUNDDEVICECONFIG, PDVCLIENTCONFIG, DWORD) PURE;
    STDMETHOD(Disconnect)          (THIS_ DWORD) PURE;
    STDMETHOD(GetSessionDesc)      (THIS_ PDVSESSIONDESC) PURE;
    STDMETHOD(GetClientConfig)     (THIS_ PDVCLIENTCONFIG) PURE;
    STDMETHOD(SetClientConfig)     (THIS_ PDVCLIENTCONFIG) PURE;
    STDMETHOD(GetCaps)             (THIS_ PDVCAPS) PURE;
    STDMETHOD(GetCompressionTypes) (THIS_ PVOID, PDWORD, PDWORD, DWORD) PURE;
    STDMETHOD(SetTransmitTargets)  (THIS_ PDVID, DWORD, DWORD) PURE;
    STDMETHOD(GetTransmitTargets)  (THIS_ PDVID, PDWORD, DWORD) PURE;
    STDMETHOD(Create3DSoundBuffer) (THIS_ DVID, LPDIRECTSOUNDBUFFER, DWORD, DWORD, LPDIRECTSOUND3DBUFFER *) PURE;
    STDMETHOD(Delete3DSoundBuffer) (THIS_ DVID, LPDIRECTSOUND3DBUFFER *) PURE;
    STDMETHOD(SetNotifyMask)       (THIS_ PDWORD, DWORD) PURE;
    STDMETHOD(GetSoundDeviceConfig)(THIS_ PDVSOUNDDEVICECONFIG, PDWORD) PURE;
};
#undef INTERFACE

#define INTERFACE IDirectPlayVoiceServer
DECLARE_INTERFACE_(IDirectPlayVoiceServer, IUnknown)
{
    STDMETHOD(QueryInterface)      (THIS_ REFIID riid, LPVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)       (THIS) PURE;
    STDMETHOD_(ULONG,Release)      (THIS) PURE;
    STDMETHOD(Initialize)          (THIS_ LPUNKNOWN, PDVMESSAGEHANDLER, PVOID, LPDWORD, DWORD) PURE;
    STDMETHOD(StartSession)        (THIS_ PDVSESSIONDESC, DWORD) PURE;
    STDMETHOD(StopSession)         (THIS_ DWORD) PURE;
    STDMETHOD(GetSessionDesc)      (THIS_ PDVSESSIONDESC) PURE;
    STDMETHOD(SetSessionDesc)      (THIS_ PDVSESSIONDESC) PURE;
    STDMETHOD(GetCaps)             (THIS_ PDVCAPS) PURE;
    STDMETHOD(GetCompressionTypes) (THIS_ PVOID, PDWORD, PDWORD, DWORD) PURE;
    STDMETHOD(SetTransmitTargets)  (THIS_ DVID, PDVID, DWORD, DWORD) PURE;
    STDMETHOD(GetTransmitTargets)  (THIS_ DVID, PDVID, PDWORD, DWORD) PURE;
    STDMETHOD(SetNotifyMask)       (THIS_ PDWORD, DWORD) PURE;
};
#undef INTERFACE

#define INTERFACE IDirectPlayVoiceTest
DECLARE_INTERFACE_(IDirectPlayVoiceTest, IUnknown)
{
    STDMETHOD(QueryInterface)  (THIS_ REFIID riid, PVOID *ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
    STDMETHOD_(ULONG,Release)  (THIS) PURE;
    STDMETHOD(CheckAudioSetup) (THIS_ const GUID *, const GUID *, HWND, DWORD) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDirectPlayVoiceClient_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceClient_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceClient_Release(p)                   (p)->lpVtbl->Release(p)

#define IDirectPlayVoiceClient_Initialize(p,a,b,c,d,e)          (p)->lpVtbl->Initialize(p,a,b,c,d,e)
#define IDirectPlayVoiceClient_Connect(p,a,b,c)                 (p)->lpVtbl->Connect(p,a,b,c)
#define IDirectPlayVoiceClient_Disconnect(p,a)                  (p)->lpVtbl->Disconnect(p,a)
#define IDirectPlayVoiceClient_GetSessionDesc(p,a)              (p)->lpVtbl->GetSessionDesc(p,a)
#define IDirectPlayVoiceClient_GetClientConfig(p,a)             (p)->lpVtbl->GetClientConfig(p,a)
#define IDirectPlayVoiceClient_SetClientConfig(p,a)             (p)->lpVtbl->SetClientConfig(p,a)
#define IDirectPlayVoiceClient_GetCaps(p,a)                     (p)->lpVtbl->GetCaps(p,a)
#define IDirectPlayVoiceClient_GetCompressionTypes(p,a,b,c,d)   (p)->lpVtbl->GetCompressionTypes(p,a,b,c,d)
#define IDirectPlayVoiceClient_SetTransmitTargets(p,a,b,c)      (p)->lpVtbl->SetTransmitTargets(p,a,b,c)
#define IDirectPlayVoiceClient_GetTransmitTargets(p,a,b,c)      (p)->lpVtbl->GetTransmitTargets(p,a,b,c)
#define IDirectPlayVoiceClient_Create3DSoundBuffer(p,a,b,c,d,e) (p)->lpVtbl->Create3DSoundBuffer(p,a,b,c,d,e)
#define IDirectPlayVoiceClient_Delete3DSoundBuffer(p,a,b)       (p)->lpVtbl->Delete3DSoundBuffer(p,a,b)
#define IDirectPlayVoiceClient_SetNotifyMask(p,a,b)             (p)->lpVtbl->SetNotifyMask(p,a,b)
#define IDirectPlayVoiceClient_GetSoundDeviceConfig(p,a,b)      (p)->lpVtbl->GetSoundDeviceConfig(p,a,b)

#define IDirectPlayVoiceServer_QueryInterface(p,a,b)          (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceServer_AddRef(p)                      (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceServer_Release(p)                     (p)->lpVtbl->Release(p)

#define IDirectPlayVoiceServer_Initialize(p,a,b,c,d,e)        (p)->lpVtbl->Initialize(p,a,b,c,d,e)
#define IDirectPlayVoiceServer_StartSession(p,a,b)            (p)->lpVtbl->StartSession(p,a,b)
#define IDirectPlayVoiceServer_StopSession(p,a)               (p)->lpVtbl->StopSession(p,a)
#define IDirectPlayVoiceServer_GetSessionDesc(p,a)            (p)->lpVtbl->GetSessionDesc(p,a)
#define IDirectPlayVoiceServer_SetSessionDesc(p,a)            (p)->lpVtbl->SetSessionDesc(p,a)
#define IDirectPlayVoiceServer_GetCaps(p,a)                   (p)->lpVtbl->GetCaps(p,a)
#define IDirectPlayVoiceServer_GetCompressionTypes(p,a,b,c,d) (p)->lpVtbl->GetCompressionTypes(p,a,b,c,d)
#define IDirectPlayVoiceServer_SetTransmitTargets(p,a,b,c,d)  (p)->lpVtbl->SetTransmitTargets(p,a,b,c,d)
#define IDirectPlayVoiceServer_GetTransmitTargets(p,a,b,c,d)  (p)->lpVtbl->GetTransmitTargets(p,a,b,c,d)
#define IDirectPlayVoiceServer_SetNotifyMask(p,a,b)           (p)->lpVtbl->SetNotifyMask(p,a,b)
#define IDirectPlayVoiceTest_QueryInterface(p,a,b)            (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlayVoiceTest_AddRef(p)                        (p)->lpVtbl->AddRef(p)
#define IDirectPlayVoiceTest_Release(p)                       (p)->lpVtbl->Release(p)
#define IDirectPlayVoiceTest_CheckAudioSetup(p,a,b,c,d)       (p)->lpVtbl->CheckAudioSetup(p,a,b,c,d)


#else /* C++ */

#define IDirectPlayVoiceClient_QueryInterface(p,a,b)            (p)->QueryInterface(a,b)
#define IDirectPlayVoiceClient_AddRef(p)                        (p)->AddRef()
#define IDirectPlayVoiceClient_Release(p)                       (p)->Release()
#define IDirectPlayVoiceClient_Initialize(p,a,b,c,d,e)          (p)->Initialize(a,b,c,d,e)
#define IDirectPlayVoiceClient_Connect(p,a,b,c)                 (p)->Connect(a,b,c)
#define IDirectPlayVoiceClient_Disconnect(p,a)                  (p)->Disconnect(a)
#define IDirectPlayVoiceClient_GetSessionDesc(p,a)              (p)->GetSessionDesc(a)
#define IDirectPlayVoiceClient_GetClientConfig(p,a)             (p)->GetClientConfig(a)
#define IDirectPlayVoiceClient_SetClientConfig(p,a)             (p)->SetClientConfig(a)
#define IDirectPlayVoiceClient_GetCaps(p,a)                     (p)->GetCaps(a)
#define IDirectPlayVoiceClient_GetCompressionTypes(p,a,b,c,d)   (p)->GetCompressionTypes(a,b,c,d)
#define IDirectPlayVoiceClient_SetTransmitTargets(p,a,b,c)      (p)->SetTransmitTargets(a,b,c)
#define IDirectPlayVoiceClient_GetTransmitTargets(p,a,b,c)      (p)->GetTransmitTargets(a,b,c)
#define IDirectPlayVoiceClient_Create3DSoundBuffer(p,a,b,c,d,e) (p)->Create3DSoundBuffer(a,b,c,d,e)
#define IDirectPlayVoiceClient_Delete3DSoundBuffer(p,a,b)       (p)->Delete3DSoundBuffer(a,b)
#define IDirectPlayVoiceClient_SetNotifyMask(p,a,b)             (p)->SetNotifyMask(a,b)
#define IDirectPlayVoiceClient_GetSoundDeviceConfig(p,a,b)      (p)->GetSoundDeviceConfig(a,b)

#define IDirectPlayVoiceServer_QueryInterface(p,a,b)          (p)->QueryInterface(a,b)
#define IDirectPlayVoiceServer_AddRef(p)                      (p)->AddRef()
#define IDirectPlayVoiceServer_Release(p)                     (p)->Release()
#define IDirectPlayVoiceServer_Initialize(p,a,b,c,d,e)        (p)->Initialize(a,b,c,d,e)
#define IDirectPlayVoiceServer_StartSession(p,a,b)            (p)->StartSession(a,b)
#define IDirectPlayVoiceServer_StopSession(p,a)               (p)->StopSession(a)
#define IDirectPlayVoiceServer_GetSessionDesc(p,a)            (p)->GetSessionDesc(a)
#define IDirectPlayVoiceServer_SetSessionDesc(p,a)            (p)->SetSessionDesc(a)
#define IDirectPlayVoiceServer_GetCaps(p,a)                   (p)->GetCaps(a)
#define IDirectPlayVoiceServer_GetCompressionTypes(p,a,b,c,d) (p)->GetCompressionTypes(a,b,c,d)
#define IDirectPlayVoiceServer_SetTransmitTargets(p,a,b,c,d)  (p)->SetTransmitTargets(a,b,c,d)
#define IDirectPlayVoiceServer_GetTransmitTargets(p,a,b,c,d)  (p)->GetTransmitTargets(a,b,c,d)
#define IDirectPlayVoiceServer_SetNotifyMask(p,a,b)           (p)->SetNotifyMask(a,b)

#define IDirectPlayVoiceTest_QueryInterface(p,a,b)            (p)->QueryInterface(a,b)
#define IDirectPlayVoiceTest_AddRef(p)                        (p)->AddRef()
#define IDirectPlayVoiceTest_Release(p)                       (p)->Release()
#define IDirectPlayVoiceTest_CheckAudioSetup(p,a,b,c,d)       (p)->CheckAudioSetup(a,b,c,d)

#endif


#ifdef __cplusplus
}
#endif

#endif /* __DVOICE_INCLUDED__ */
