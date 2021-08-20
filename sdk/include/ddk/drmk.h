/*
    ReactOS Kernel Streaming
    Digital Rights Management

    Author: Andrew Greenwood
*/

#ifndef DRMK_H
#define DRMK_H

typedef struct {
  DWORD Flags;
  PDEVICE_OBJECT DeviceObject;
  PFILE_OBJECT FileObject;
  PVOID Context;
} DRMFORWARD, *PDRMFORWARD, *PCDRMFORWARD;

typedef struct {
  BOOL CopyProtect;
  ULONG Reserved;
  BOOL DigitalOutputDisable;
} DRMRIGHTS, *PDRMRIGHTS;

typedef const DRMRIGHTS *PCDRMRIGHTS;

/* ===============================================================
    Digital Rights Management Functions
    TODO: Check calling convention
*/

#ifdef __cplusplus
extern "C" {
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmAddContentHandlers(
  _In_ ULONG ContentId,
  _In_reads_(NumHandlers) PVOID *paHandlers,
  _In_ ULONG NumHandlers);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmCreateContentMixed(
  _In_ PULONG paContentId,
  _In_ ULONG cContentId,
  _Out_ PULONG pMixedContentId);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmDestroyContent(
  _In_ ULONG ContentId);

NTSTATUS
NTAPI
DrmForwardContentToDeviceObject(
  _In_ ULONG ContentId,
  _In_opt_ PVOID Reserved,
  _In_ PCDRMFORWARD DrmForward);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmForwardContentToFileObject(
  _In_ ULONG ContentId,
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmForwardContentToInterface(
  _In_ ULONG ContentId,
  _In_ PUNKNOWN pUnknown,
  _In_ ULONG NumMethods);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
DrmGetContentRights(
  _In_ ULONG ContentId,
  _Out_ PDRMRIGHTS DrmRights);

#ifdef __cplusplus
}
#endif

DEFINE_GUID(IID_IDrmAudioStream,
  0x1915c967, 0x3299, 0x48cb, 0xa3, 0xe4, 0x69, 0xfd, 0x1d, 0x1b, 0x30, 0x6e);

#undef INTERFACE
#define INTERFACE IDrmAudioStream

DECLARE_INTERFACE_(IDrmAudioStream, IUnknown) {
  STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
    _In_ REFIID InterfaceId,
    _Out_ PVOID* Interface
  ) PURE;
  STDMETHOD_(ULONG,AddRef)(THIS) PURE;
  STDMETHOD_(ULONG,Release)(THIS) PURE;
  STDMETHOD_(NTSTATUS,SetContentId)(THIS_
    _In_ ULONG ContentId,
    _In_ PCDRMRIGHTS DrmRights
  ) PURE;
};

typedef IDrmAudioStream *PDRMAUDIOSTREAM;

#define IMP_IDrmAudioStream             \
  STDMETHODIMP_(NTSTATUS) SetContentId( \
    _In_ ULONG ContentId,                 \
    _In_ PCDRMRIGHTS DrmRights);

#endif /* DRMK_H */
