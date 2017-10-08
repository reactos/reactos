#ifndef _PCMCIA_PCH_
#define _PCMCIA_PCH_

#include <wdm.h>

typedef enum
{
    dsStopped,
    dsStarted,
    dsPaused,
    dsRemoved,
    dsSurpriseRemoved
} PCMCIA_DEVICE_STATE;

typedef struct _PCMCIA_COMMON_EXTENSION
{
    PDEVICE_OBJECT Self;
    BOOLEAN IsFDO;
    POWER_SEQUENCE PowerSequence;
    PCMCIA_DEVICE_STATE State;
    DEVICE_POWER_STATE DevicePowerState;
    SYSTEM_POWER_STATE SystemPowerState;
} PCMCIA_COMMON_EXTENSION, *PPCMCIA_COMMON_EXTENSION;

typedef struct _PCMCIA_PDO_EXTENSION
{
    PCMCIA_COMMON_EXTENSION Common;
} PCMCIA_PDO_EXTENSION, *PPCMCIA_PDO_EXTENSION;

typedef struct _PCMCIA_FDO_EXTENSION
{
    PCMCIA_COMMON_EXTENSION Common;
    PDEVICE_OBJECT Ldo;
    LIST_ENTRY ChildDeviceList;
    KSPIN_LOCK Lock;
} PCMCIA_FDO_EXTENSION, *PPCMCIA_FDO_EXTENSION;

/* pdo.c */
NTSTATUS
NTAPI
PcmciaPdoPlugPlay(PPCMCIA_PDO_EXTENSION PdoExt,
                  PIRP Irp);

NTSTATUS
NTAPI
PcmciaPdoSetPowerState(PPCMCIA_PDO_EXTENSION PdoExt);

/* fdo.c */
NTSTATUS
NTAPI
PcmciaFdoPlugPlay(PPCMCIA_FDO_EXTENSION FdoExt,
                  PIRP Irp);

#endif /* _PCMCIA_PCH_ */
