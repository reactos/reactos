/*
    ReactOS Kernel Streaming
    Digital Rights Management

    Author: Andrew Greenwood
*/

#ifndef DRMK_H
#define DRMK_H

typedef struct
{
    DWORD Flags;
    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;
    PVOID Context;
} DRMFORWARD, *PDRMFORWARD, *PCDRMFORWARD;

typedef struct
{
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
extern "C"
{
#endif

NTAPI NTSTATUS
DrmAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers);

NTAPI NTSTATUS
DrmCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId);

NTAPI NTSTATUS
DrmDestroyContent(
    IN  ULONG ContentId);

NTAPI NTSTATUS
DrmForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward);

NTAPI NTSTATUS
DrmForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject);

NTAPI NTSTATUS
DrmForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods);

NTAPI NTSTATUS
DrmGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights);

#ifdef __cplusplus
}
#endif



DEFINE_GUID(IID_IDrmAudioStream,
    0x1915c967, 0x3299, 0x48cb, 0xa3, 0xe4, 0x69, 0xfd, 0x1d, 0x1b, 0x30, 0x6e);

#undef INTERFACE
#define INTERFACE IDrmAudioStream

DECLARE_INTERFACE_(IDrmAudioStream, IUnknown)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;
    STDMETHOD_(NTSTATUS,SetContentId)(THIS_
        IN ULONG ContentId,
        IN PCDRMRIGHTS DrmRights) PURE;
};

typedef IDrmAudioStream *PDRMAUDIOSTREAM;

#define IMP_IDrmAudioStream                 \
    STDMETHODIMP_(NTSTATUS) SetContentId    \
    (   IN      ULONG	    ContentId,      \
        IN      PCDRMRIGHTS DrmRights       \
    );


#endif
