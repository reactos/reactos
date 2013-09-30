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

/* Linux does not support better timing than 10ms */
#define DS_TIME_RES 2  /* Resolution of multimedia timer */
#define DS_TIME_DEL 10  /* Delay of multimedia timer callback, and duration of HEL fragment */

#include "wingdi.h"
#include "mmdeviceapi.h"
#include "audioclient.h"
#include "mmsystem.h"

#include "wine/list.h"

extern int ds_hel_buflen DECLSPEC_HIDDEN;
extern int ds_snd_queue_max DECLSPEC_HIDDEN;

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectSoundBufferImpl        IDirectSoundBufferImpl;
typedef struct DirectSoundDevice             DirectSoundDevice;

/* dsound_convert.h */
typedef float (*bitsgetfunc)(const IDirectSoundBufferImpl *, DWORD, DWORD);
typedef void (*bitsputfunc)(const IDirectSoundBufferImpl *, DWORD, DWORD, float);
extern const bitsgetfunc getbpp[5] DECLSPEC_HIDDEN;
void putieee32(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value) DECLSPEC_HIDDEN;
void mixieee32(float *src, float *dst, unsigned samples) DECLSPEC_HIDDEN;
typedef void (*normfunc)(const void *, void *, unsigned);
extern const normfunc normfunctions[5] DECLSPEC_HIDDEN;

typedef struct _DSVOLUMEPAN
{
    DWORD	dwTotalLeftAmpFactor;
    DWORD	dwTotalRightAmpFactor;
    LONG	lVolume;
    DWORD	dwVolAmpFactor;
    LONG	lPan;
    DWORD	dwPanLeftAmpFactor;
    DWORD	dwPanRightAmpFactor;
} DSVOLUMEPAN,*PDSVOLUMEPAN;

/*****************************************************************************
 * IDirectSoundDevice implementation structure
 */
struct DirectSoundDevice
{
    LONG                        ref;

    GUID                        guid;
    DSCAPS                      drvcaps;
    DWORD                       priolevel, sleeptime;
    PWAVEFORMATEX               pwfx, primary_pwfx;
    UINT                        playing_offs_bytes, in_mmdev_bytes, prebuf;
    DWORD                       fraglen;
    LPBYTE                      buffer;
    DWORD                       writelead, buflen, state, playpos, mixpos;
    int                         nrofbuffers;
    IDirectSoundBufferImpl**    buffers;
    RTL_RWLOCK                  buffer_list_lock;
    CRITICAL_SECTION            mixlock;
    IDirectSoundBufferImpl     *primary;
    DWORD                       speaker_config;
    float *mix_buffer, *tmp_buffer;
    DWORD                       tmp_buffer_len, mix_buffer_len;

    DSVOLUMEPAN                 volpan;

    normfunc normfunction;

    /* DirectSound3DListener fields */
    DS3DLISTENER                ds3dl;
    BOOL                        ds3dl_need_recalc;

    IMMDevice *mmdevice;
    IAudioClient *client;
    IAudioClock *clock;
    IAudioStreamVolume *volume;
    IAudioRenderClient *render;

    HANDLE sleepev, thread;
    struct list entry;
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
void DirectSoundDevice_RemoveBuffer(DirectSoundDevice * device, IDirectSoundBufferImpl * pDSB) DECLSPEC_HIDDEN;
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

/*****************************************************************************
 * IDirectSoundBuffer implementation structure
 */
struct IDirectSoundBufferImpl
{
    IDirectSoundBuffer8         IDirectSoundBuffer8_iface;
    IDirectSoundNotify          IDirectSoundNotify_iface;
    IDirectSound3DListener      IDirectSound3DListener_iface; /* only primary buffer */
    IDirectSound3DBuffer        IDirectSound3DBuffer_iface; /* only secondary buffer */
    IKsPropertySet              IKsPropertySet_iface;
    LONG                        numIfaces; /* "in use interfaces" refcount */
    LONG                        ref, refn, ref3D, refiks;
    /* IDirectSoundBufferImpl fields */
    DirectSoundDevice*          device;
    RTL_RWLOCK                  lock;
    PWAVEFORMATEX               pwfx;
    BufferMemory*               buffer;
    DWORD                       playflags,state,leadin;
    DWORD                       writelead,buflen;
    DWORD                       nAvgBytesPerSec;
    DWORD                       freq;
    DSVOLUMEPAN                 volpan;
    DSBUFFERDESC                dsbd;
    /* used for frequency conversion (PerfectPitch) */
    ULONG                       freqneeded;
    DWORD                       firstep;
    float freqAcc, freqAdjust, firgain;
    /* used for mixing */
    DWORD                       sec_mixpos;

    /* IDirectSoundNotify fields */
    LPDSBPOSITIONNOTIFY         notifies;
    int                         nrofnotifies;
    /* DirectSound3DBuffer fields */
    DS3DBUFFER                  ds3db_ds3db;
    LONG                        ds3db_lVolume;
    BOOL                        ds3db_need_recalc;
    /* Used for bit depth conversion */
    int                         mix_channels;
    bitsgetfunc get, get_aux;
    bitsputfunc put, put_aux;

    struct list entry;
};

float get_mono(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel) DECLSPEC_HIDDEN;
void put_mono2stereo(const IDirectSoundBufferImpl *dsb, DWORD pos, DWORD channel, float value) DECLSPEC_HIDDEN;

HRESULT IDirectSoundBufferImpl_Create(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    LPCDSBUFFERDESC dsbd) DECLSPEC_HIDDEN;
HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb) DECLSPEC_HIDDEN;
void secondarybuffer_destroy(IDirectSoundBufferImpl *This) DECLSPEC_HIDDEN;
const IDirectSound3DListenerVtbl ds3dlvt DECLSPEC_HIDDEN;
const IDirectSound3DBufferVtbl ds3dbvt DECLSPEC_HIDDEN;
const IKsPropertySetVtbl iksbvt DECLSPEC_HIDDEN;

HRESULT IKsPrivatePropertySetImpl_Create(REFIID riid, void **ppv) DECLSPEC_HIDDEN;

/*******************************************************************************
 */

/* dsound.c */

HRESULT DSOUND_Create(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT DSOUND_Create8(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT IDirectSoundImpl_Create(IUnknown *outer_unk, REFIID riid, void **ppv, BOOL has_ds8) DECLSPEC_HIDDEN;

/* primary.c */

HRESULT DSOUND_PrimaryCreate(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryGetPosition(DirectSoundDevice *device, LPDWORD playpos, LPDWORD writepos) DECLSPEC_HIDDEN;
LPWAVEFORMATEX DSOUND_CopyFormat(LPCWAVEFORMATEX wfex) DECLSPEC_HIDDEN;
HRESULT DSOUND_ReopenDevice(DirectSoundDevice *device, BOOL forcewave) DECLSPEC_HIDDEN;
HRESULT DSOUND_PrimaryOpen(DirectSoundDevice *device) DECLSPEC_HIDDEN;
HRESULT primarybuffer_create(DirectSoundDevice *device, IDirectSoundBufferImpl **ppdsb,
    const DSBUFFERDESC *dsbd) DECLSPEC_HIDDEN;
void primarybuffer_destroy(IDirectSoundBufferImpl *This) DECLSPEC_HIDDEN;
HRESULT primarybuffer_SetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX wfex) DECLSPEC_HIDDEN;
LONG capped_refcount_dec(LONG *ref) DECLSPEC_HIDDEN;

/* duplex.c */

HRESULT DSOUND_FullDuplexCreate(REFIID riid, void **ppv) DECLSPEC_HIDDEN;

/* mixer.c */
void DSOUND_CheckEvent(const IDirectSoundBufferImpl *dsb, DWORD playpos, int len) DECLSPEC_HIDDEN;
void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan) DECLSPEC_HIDDEN;
void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan) DECLSPEC_HIDDEN;
void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb) DECLSPEC_HIDDEN;
DWORD DSOUND_secpos_to_bufpos(const IDirectSoundBufferImpl *dsb, DWORD secpos, DWORD secmixpos, float *overshot) DECLSPEC_HIDDEN;

DWORD CALLBACK DSOUND_mixthread(void *ptr) DECLSPEC_HIDDEN;

/* sound3d.c */

void DSOUND_Calc3DBuffer(IDirectSoundBufferImpl *dsb) DECLSPEC_HIDDEN;

/* capture.c */
 
HRESULT DSOUND_CaptureCreate(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT DSOUND_CaptureCreate8(REFIID riid, void **ppv) DECLSPEC_HIDDEN;
HRESULT IDirectSoundCaptureImpl_Create(IUnknown *outer_unk, REFIID riid, void **ppv, BOOL has_dsc8) DECLSPEC_HIDDEN;

#define STATE_STOPPED   0
#define STATE_STARTING  1
#define STATE_PLAYING   2
#define STATE_CAPTURING 2
#define STATE_STOPPING  3

extern CRITICAL_SECTION DSOUND_renderers_lock DECLSPEC_HIDDEN;
extern CRITICAL_SECTION DSOUND_capturers_lock DECLSPEC_HIDDEN;
extern struct list DSOUND_capturers DECLSPEC_HIDDEN;
extern struct list DSOUND_renderers DECLSPEC_HIDDEN;

extern GUID DSOUND_renderer_guids[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;
extern GUID DSOUND_capture_guids[MAXWAVEDRIVERS] DECLSPEC_HIDDEN;

extern WCHAR wine_vxd_drv[] DECLSPEC_HIDDEN;

void setup_dsound_options(void) DECLSPEC_HIDDEN;

HRESULT get_mmdevice(EDataFlow flow, const GUID *tgt, IMMDevice **device) DECLSPEC_HIDDEN;

BOOL DSOUND_check_supported(IAudioClient *client, DWORD rate,
        DWORD depth, WORD channels) DECLSPEC_HIDDEN;
HRESULT enumerate_mmdevices(EDataFlow flow, GUID *guids,
        LPDSENUMCALLBACKW cb, void *user) DECLSPEC_HIDDEN;
