/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2001 TransGaming Technologies, Inc.
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

#ifndef _DSOUND_PRIVATE_H_
#define _DSOUND_PRIVATE_H_

#include <wine/config.h>

#include <assert.h>
#include <math.h>
#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <wingdi.h>
#include <winternl.h>
#include <objbase.h>
#include <audioclient.h>
#include <mmddk.h>
#include <dsound.h>
#include <dsconf.h>
#include <dsdriver.h>
#include <vfwmsgs.h>

#include <wine/debug.h>
#include <wine/list.h>

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 2  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */

/* direct sound hardware acceleration levels */
#define DS_HW_ACCEL_FULL        0	/* default on Windows 98 */
#define DS_HW_ACCEL_STANDARD    1	/* default on Windows 2000 */
#define DS_HW_ACCEL_BASIC       2
#define DS_HW_ACCEL_EMULATION   3

extern int ds_emuldriver DECLSPEC_HIDDEN;
extern int ds_hel_buflen DECLSPEC_HIDDEN;
extern int ds_snd_queue_max DECLSPEC_HIDDEN;
extern int ds_snd_queue_min DECLSPEC_HIDDEN;
extern int ds_snd_shadow_maxsize DECLSPEC_HIDDEN;
extern int ds_hw_accel DECLSPEC_HIDDEN;
extern int ds_default_sample_rate DECLSPEC_HIDDEN;
extern int ds_default_bits_per_sample DECLSPEC_HIDDEN;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundImpl              IDirectSoundImpl;
typedef struct IDirectSound_IUnknown         IDirectSound_IUnknown;
typedef struct IDirectSound_IDirectSound     IDirectSound_IDirectSound;
typedef struct IDirectSound8_IUnknown        IDirectSound8_IUnknown;
typedef struct IDirectSound8_IDirectSound    IDirectSound8_IDirectSound;
typedef struct IDirectSound8_IDirectSound8   IDirectSound8_IDirectSound8;
typedef struct IDirectSoundBufferImpl        IDirectSoundBufferImpl;
typedef struct IDirectSoundCaptureImpl       IDirectSoundCaptureImpl;
typedef struct IDirectSoundCaptureBufferImpl IDirectSoundCaptureBufferImpl;
typedef struct IDirectSoundNotifyImpl        IDirectSoundNotifyImpl;
typedef struct IDirectSoundCaptureNotifyImpl IDirectSoundCaptureNotifyImpl;
typedef struct IDirectSound3DListenerImpl    IDirectSound3DListenerImpl;
typedef struct IDirectSound3DBufferImpl      IDirectSound3DBufferImpl;
typedef struct IKsBufferPropertySetImpl      IKsBufferPropertySetImpl;
typedef struct DirectSoundDevice             DirectSoundDevice;
typedef struct DirectSoundCaptureDevice      DirectSoundCaptureDevice;

/* dsound_convert.h */
typedef void (*bitsconvertfunc)(const void *, void *, UINT, UINT, INT, UINT, UINT);
extern const bitsconvertfunc convertbpp[5][4] DECLSPEC_HIDDEN;
typedef void (*mixfunc)(const void *, void *, unsigned);
extern const mixfunc mixfunctions[4] DECLSPEC_HIDDEN;
typedef void (*normfunc)(const void *, void *, unsigned);
extern const normfunc normfunctions[4] DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectSoundDevice implementation structure
 */
struct DirectSoundDevice
{
    LONG                        ref;

    GUID                        guid;
    PIDSDRIVER                  driver;
    DSDRIVERDESC                drvdesc;
    DSDRIVERCAPS                drvcaps;
    DWORD                       priolevel;
    PWAVEFORMATEX               pwfx;
    HWAVEOUT                    hwo;
    LPWAVEHDR                   pwave;
    UINT                        timerID, pwplay, pwqueue, prebuf, helfrags;
    DWORD                       fraglen;
    PIDSDRIVERBUFFER            hwbuf;
    LPBYTE                      buffer;
    DWORD                       writelead, buflen, state, playpos, mixpos;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    RTL_RWLOCK                  buffer_list_lock;
    CRITICAL_SECTION            mixlock;
    IDirectSoundBufferImpl     *primary;
    DWORD                       speaker_config;
    LPBYTE                      tmp_buffer, mix_buffer;
    DWORD                       tmp_buffer_len, mix_buffer_len;

    DSVOLUMEPAN                 volpan;

    mixfunc mixfunction;
    normfunc normfunction;

    /* DirectSound3DListener fields */
    IDirectSound3DListenerImpl*	listener;
    DS3DLISTENER                ds3dl;
    BOOL                        ds3dl_need_recalc;
};

/* reference counted buffer memory for duplicated buffer memory */
typedef struct BufferMemory
{
    LONG                        ref;
    LPBYTE                      memory;
    struct list buffers;
} BufferMemory;

ULONG DirectSoundDevice_Release(DirectSoundDevice * device) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_Initialize(
    DirectSoundDevice ** ppDevice,
    LPCGUID lpcGUID) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_AddBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_RemoveBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_GetCaps(DirectSoundDevice * device, LPDSCAPS lpDSCaps) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_CreateSoundBuffer(
    DirectSoundDevice * device,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk,
    BOOL from8) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_DuplicateSoundBuffer(
    DirectSoundDevice * device,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_SetCooperativeLevel(
    DirectSoundDevice * devcie,
    HWND hwnd,
    DWORD level) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_Compact(DirectSoundDevice * device) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_GetSpeakerConfig(
    DirectSoundDevice * device,
    LPDWORD lpdwSpeakerConfig) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_SetSpeakerConfig(
    DirectSoundDevice * device,
    DWORD config) DECLSPEC_HIDDEN;
HRESULT DirectSoundDevice_VerifyCertification(DirectSoundDevice * device,
    LPDWORD pdwCertified) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    IDirectSoundBuffer8         IDirectSoundBuffer8_iface;
    LONG                        numIfaces; /* "in use interfaces" refcount */
    LONG                        ref;
    /* IDirectSoundBufferImpl fields */
    DirectSoundDevice*          device;
    RTL_RWLOCK                  lock;
    PIDSDRIVERBUFFER            hwbuf;
    PWAVEFORMATEX               pwfx;
    BufferMemory*               buffer;
    LPBYTE                      tmp_buffer;
    DWORD                       playflags,state,leadin;
    DWORD                       writelead,buflen;
    DWORD                       nAvgBytesPerSec;
    DWORD                       freq, tmp_buffer_len, max_buffer_len;
    DSVOLUMEPAN                 volpan;
    DSBUFFERDESC                dsbd;
    /* used for frequency conversion (PerfectPitch) */
    ULONG                       freqneeded, freqAdjust, freqAcc, freqAccNext, resampleinmixer;
    /* used for mixing */
    DWORD                       primary_mixpos, buf_mixpos, sec_mixpos;

    /* IDirectSoundNotifyImpl fields */
    IDirectSoundNotifyImpl*     notify;
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;
    PIDSDRIVERNOTIFY            hwnotify;

    /* DirectSound3DBuffer fields */
    IDirectSound3DBufferImpl*   ds3db;
    DS3DBUFFER                  ds3db_ds3db;
    LONG                        ds3db_lVolume;
    BOOL                        ds3db_need_recalc;

    /* IKsPropertySet fields */
    IKsBufferPropertySetImpl*   iks;
    bitsconvertfunc convert;
    struct list entry;
};

HRESULT IDirectSoundBufferImpl_Create(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    LPCDSBUFFERDESC dsbd) DECLSPEC_HIDDEN;
HRESULT IDirectSoundBufferImpl_Destroy(
    IDirectSoundBufferImpl *pdsb) DECLSPEC_HIDDEN;
HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb) DECLSPEC_HIDDEN;
void secondarybuffer_destroy(IDirectSoundBufferImpl *This) DECLSPEC_HIDDEN;

/*****************************************************************************
 * DirectSoundCaptureDevice implementation structure
 */
struct DirectSoundCaptureDevice
{
    /* IDirectSoundCaptureImpl fields */
    GUID                               guid;
    LONG                               ref;

    /* DirectSound driver stuff */
    PIDSCDRIVER                        driver;
    DSDRIVERDESC                       drvdesc;
    DSCDRIVERCAPS                      drvcaps;
    PIDSCDRIVERBUFFER                  hwbuf;

    /* wave driver info */
    HWAVEIN                            hwi;

    /* more stuff */
    LPBYTE                             buffer;
    DWORD                              buflen;

    PWAVEFORMATEX                      pwfx;

    IDirectSoundCaptureBufferImpl*     capture_buffer;
    DWORD                              state;
    LPWAVEHDR                          pwave;
    int                                nrofpwaves;
    int                                index;
    CRITICAL_SECTION                   lock;
};

/*****************************************************************************
 * IDirectSoundCaptureBuffer implementation structure
 */
struct IDirectSoundCaptureBufferImpl
{
    /* IUnknown fields */
    const IDirectSoundCaptureBuffer8Vtbl *lpVtbl;
    LONG                                ref;

    /* IDirectSoundCaptureBufferImpl fields */
    DirectSoundCaptureDevice*           device;
    /* FIXME: don't need this */
    LPDSCBUFFERDESC                     pdscbd;
    DWORD                               flags;

    /* IDirectSoundCaptureNotifyImpl fields */
    IDirectSoundCaptureNotifyImpl*      notify;
    LPDSBPOSITIONNOTIFY                 notifies;
    int                                 nrofnotifies;
    PIDSDRIVERNOTIFY                    hwnotify;
};

/*****************************************************************************
 *  IDirectSound3DListener implementation structure
 */
struct IDirectSound3DListenerImpl
{
    /* IUnknown fields */
    const IDirectSound3DListenerVtbl *lpVtbl;
    LONG                        ref;
    /* IDirectSound3DListenerImpl fields */
    DirectSoundDevice*          device;
};

HRESULT IDirectSound3DListenerImpl_Create(
    DirectSoundDevice           *device,
    IDirectSound3DListenerImpl **pdsl) DECLSPEC_HIDDEN;

/*****************************************************************************
 *  IKsBufferPropertySet implementation structure
 */
struct IKsBufferPropertySetImpl
{
    /* IUnknown fields */
    const IKsPropertySetVtbl   *lpVtbl;
    LONG 			ref;
    /* IKsPropertySetImpl fields */
    IDirectSoundBufferImpl*	dsb;
};

HRESULT IKsBufferPropertySetImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IKsBufferPropertySetImpl **piks) DECLSPEC_HIDDEN;
HRESULT IKsBufferPropertySetImpl_Destroy(
    IKsBufferPropertySetImpl *piks) DECLSPEC_HIDDEN;

HRESULT IKsPrivatePropertySetImpl_Create(REFIID riid, IKsPropertySet **piks) DECLSPEC_HIDDEN;

/*****************************************************************************
 * IDirectSound3DBuffer implementation structure
 */
struct IDirectSound3DBufferImpl
{
    /* IUnknown fields */
    const IDirectSound3DBufferVtbl *lpVtbl;
    LONG                        ref;
    /* IDirectSound3DBufferImpl fields */
    IDirectSoundBufferImpl*     dsb;
};

HRESULT IDirectSound3DBufferImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IDirectSound3DBufferImpl **pds3db) DECLSPEC_HIDDEN;
HRESULT IDirectSound3DBufferImpl_Destroy(
    IDirectSound3DBufferImpl *pds3db) DECLSPEC_HIDDEN;

/*******************************************************************************
 */

/* dsound.c */

HRESULT DSOUND_Create(REFIID riid, LPDIRECTSOUND *ppDS) DECLSPEC_HIDDEN;
HRESULT DSOUND_Create8(REFIID riid, LPDIRECTSOUND8 *ppDS) DECLSPEC_HIDDEN;

/* primary.c */

DWORD DSOUND_fraglen(DWORD nSamplesPerSec, DWORD nBlockAlign) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryCreate(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryGetPosition(DirectSoundDevice *device, LPDWORD playpos, LPDWORD writepos) DECLSPEC_HIDDEN;
LPWAVEFORMATEX DSOUND_CopyFormat(LPCWAVEFORMATEX wfex) DECLSPEC_HIDDEN;
HRESULT DSOUND_ReopenDevice(DirectSoundDevice *device, BOOL forcewave) DECLSPEC_HIDDEN;
HRESULT primarybuffer_create(DirectSoundDevice *device, IDirectSoundBufferImpl **ppdsb,
    const DSBUFFERDESC *dsbd) DECLSPEC_HIDDEN;
void primarybuffer_destroy(IDirectSoundBufferImpl *This) DECLSPEC_HIDDEN;
HRESULT primarybuffer_SetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX wfex) DECLSPEC_HIDDEN;

/* duplex.c */
 
HRESULT DSOUND_FullDuplexCreate(REFIID riid, LPDIRECTSOUNDFULLDUPLEX* ppDSFD) DECLSPEC_HIDDEN;

/* mixer.c */
DWORD DSOUND_bufpos_to_mixpos(const DirectSoundDevice* device, DWORD pos) DECLSPEC_HIDDEN;
void DSOUND_CheckEvent(const IDirectSoundBufferImpl *dsb, DWORD playpos, int len) DECLSPEC_HIDDEN;
void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan) DECLSPEC_HIDDEN;
void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan) DECLSPEC_HIDDEN;
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb) DECLSPEC_HIDDEN;
void DSOUND_MixToTemporary(const IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD mixlen, BOOL inmixer) DECLSPEC_HIDDEN;
DWORD DSOUND_secpos_to_bufpos(const IDirectSoundBufferImpl *dsb, DWORD secpos, DWORD secmixpos, DWORD* overshot) DECLSPEC_HIDDEN;

void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) DECLSPEC_HIDDEN;
void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) DECLSPEC_HIDDEN;

/* sound3d.c */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb) DECLSPEC_HIDDEN;

/* capture.c */
 
HRESULT DSOUND_CaptureCreate(REFIID riid, LPDIRECTSOUNDCAPTURE *ppDSC) DECLSPEC_HIDDEN;
HRESULT DSOUND_CaptureCreate8(REFIID riid, LPDIRECTSOUNDCAPTURE8 *ppDSC8) DECLSPEC_HIDDEN;

#define STATE_STOPPED   0
#define STATE_STARTING  1
#define STATE_PLAYING   2
#define STATE_CAPTURING 2
#define STATE_STOPPING  3

#define DSOUND_FREQSHIFT (20)

extern DirectSoundDevice* DSOUND_renderer[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;
extern GUID DSOUND_renderer_guids[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;

extern DirectSoundCaptureDevice * DSOUND_capture[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;
extern GUID DSOUND_capture_guids[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;

HRESULT mmErr(UINT err) DECLSPEC_HIDDEN;
void setup_dsound_options(void) DECLSPEC_HIDDEN;
const char * dumpCooperativeLevel(DWORD level) DECLSPEC_HIDDEN;

#endif /* _DSOUND_PRIVATE_H_ */
