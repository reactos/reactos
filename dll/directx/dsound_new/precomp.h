#ifndef PRECOMP_H
#define PRECOMP_H

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windows.h>
#include <setupapi.h>
#include <mmddk.h>
#include <objbase.h>
#include <olectl.h>
#include <unknwn.h>
#include <dsound.h>
#include <dsconf.h>
#include <vfwmsgs.h>
#include <setupapi.h>
#define YDEBUG
#include <debug.h>
#include <ks.h>
#include <ksmedia.h>
#include <limits.h>
#include <stdio.h>

#include "resource.h"


/* factory method */
typedef HRESULT (CALLBACK *LPFNCREATEINSTANCE)(IUnknown* pUnkOuter, REFIID riid, LPVOID* ppvObject);

/* factory table */
typedef struct
{
    REFIID riid;
    LPFNCREATEINSTANCE lpfnCI;
} INTERFACE_TABLE;


typedef struct tagFILTERINFO
{
    SP_DEVINFO_DATA DeviceData;
    WCHAR DevicePath[MAX_PATH];
    HANDLE hFilter;
    ULONG PinCount;
    PULONG Pin;
    GUID DeviceGuid[2];
    ULONG MappedId[2];

    struct tagFILTERINFO *lpNext;
}FILTERINFO, *LPFILTERINFO;

#define INIT_GUID(guid, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)      \
        guid.Data1 = l; guid.Data2 = w1; guid.Data3 = w2;               \
        guid.Data4[0] = b1; guid.Data4[1] = b2; guid.Data4[2] = b3;     \
        guid.Data4[3] = b4; guid.Data4[4] = b5; guid.Data4[5] = b6;     \
        guid.Data4[6] = b7; guid.Data4[7] = b8;

typedef enum
{
    PIN_TYPE_NONE = 0,
    PIN_TYPE_PLAYBACK = 1,
    PIN_TYPE_RECORDING = 2
}PIN_TYPE;

/* globals */
extern HINSTANCE dsound_hInstance;
extern LPFILTERINFO RootInfo;

/* classfactory.c */

IClassFactory *
IClassFactory_fnConstructor(
    LPFNCREATEINSTANCE lpfnCI,
    PLONG pcRefDll,
    REFIID riidInst);


/* devicelist.c */

HRESULT
EnumAudioDeviceInterfaces(
    LPFILTERINFO *OutRootInfo);

BOOL
FindDeviceByGuid(
    LPCGUID pGuidSrc,
    LPFILTERINFO *Filter);

BOOL
FindDeviceByMappedId(
    IN ULONG DeviceID,
    LPFILTERINFO *Filter,
    BOOL bPlayback);

ULONG
GetPinIdFromFilter(
    LPFILTERINFO Filter,
    BOOL bCapture,
    ULONG Offset);

/* directsound.c */

HRESULT
CALLBACK
NewDirectSound(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject);


/* misc.c */

VOID
PerformChannelConversion(
    PUCHAR Buffer,
    ULONG BufferLength,
    PULONG BytesRead,
    ULONG OldChannels,
    ULONG NewChannels,
    ULONG BitsPerSample,
    PUCHAR Result,
    ULONG ResultLength,
    PULONG BytesWritten);

BOOL
SetPinFormat(
    IN HANDLE hPin,
    IN LPWAVEFORMATEX WaveFormatEx);

BOOL
CreateCompatiblePin(
    IN HANDLE hFilter,
    IN DWORD PinId,
    IN BOOL bLoop,
    IN LPWAVEFORMATEX WaveFormatEx,
    OUT LPWAVEFORMATEX WaveFormatOut,
    OUT PHANDLE hPin);


DWORD
SyncOverlappedDeviceIoControl(
    IN  HANDLE Handle,
    IN  DWORD IoControlCode,
    IN  LPVOID InBuffer,
    IN  DWORD InBufferSize,
    OUT LPVOID OutBuffer,
    IN  DWORD OutBufferSize,
    OUT LPDWORD BytesTransferred OPTIONAL);

DWORD
PrimaryDirectSoundBuffer_Write(
    LPDIRECTSOUNDBUFFER8 iface,
    LPVOID Buffer,
    DWORD  BufferSize);


DWORD
OpenPin(
    HANDLE hFilter,
    ULONG PinId,
    LPWAVEFORMATEX WaveFormatEx,
    PHANDLE hPin,
    BOOL bLoop);

DWORD
OpenFilter(
    IN LPCWSTR lpFileName,
    IN PHANDLE OutHandle);

DWORD
GetFilterPinCount(
    IN HANDLE hFilter,
    OUT PULONG NumPins);

DWORD
GetFilterPinCommunication(
    IN HANDLE hFilter,
    IN ULONG PinId,
    OUT PKSPIN_COMMUNICATION Communication);

DWORD
GetFilterPinDataFlow(
    IN HANDLE hFilter,
    IN ULONG PinId,
    OUT PKSPIN_DATAFLOW DataFlow);

/* primary.c */

HRESULT
PrimaryDirectSoundBuffer_GetPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwCurrentPlayCursor,
    LPDWORD pdwCurrentWriteCursor);

VOID
PrimaryDirectSoundBuffer_SetState(
    LPDIRECTSOUNDBUFFER8 iface,
    KSSTATE State);

HRESULT
NewPrimarySoundBuffer(
    LPDIRECTSOUNDBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    DWORD dwLevel,
    DWORD dwFlags);

HRESULT
PrimaryDirectSoundBuffer_SetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX pcfxFormat,
    BOOL bLooped);

VOID
PrimaryDirectSoundBuffer_AcquireLock(
    LPDIRECTSOUNDBUFFER8 iface);

VOID
PrimaryDirectSoundBuffer_ReleaseLock(
    LPDIRECTSOUNDBUFFER8 iface);

/* secondary.c */

HRESULT
NewSecondarySoundBuffer(
    LPDIRECTSOUNDBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    DWORD dwLevel,
    LPCDSBUFFERDESC lpcDSBufferDesc,
    LPDIRECTSOUNDBUFFER8 PrimaryBuffer);

/* property.c */
HRESULT
CALLBACK
NewKsPropertySet(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject);

/* capture.c */

HRESULT
CALLBACK
NewDirectSoundCapture(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject);


/* capturebuffer.c */
HRESULT
NewDirectSoundCaptureBuffer(
    LPDIRECTSOUNDCAPTUREBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    LPCDSCBUFFERDESC lpcDSBufferDesc);

/* notify.c */
VOID
DoNotifyPositionEvents(
    LPDIRECTSOUNDNOTIFY iface,
    DWORD OldPosition,
    DWORD NewPosition);

HRESULT
NewDirectSoundNotify(
    LPDIRECTSOUNDNOTIFY * Notify,
    BOOL bLoop,
    BOOL bMix,
    HANDLE hPin,
    DWORD BufferSize);

#endif
