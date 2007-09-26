#ifndef __DESKCPLX__H
#define __DESKCPLX__H

#define DESK_EXT_CALLBACK CALLBACK

#define DESK_EXT_EXTINTERFACE TEXT("Desk.cpl extension interface")
#define DESK_EXT_PRUNINGMODE TEXT("Pruning Mode")
#define DESK_EXT_DISPLAYDEVICE TEXT("Display Device")
#define DESK_EXT_DISPLAYNAME TEXT("Display Name")
#define DESK_EXT_DISPLAYID TEXT("Display ID")
#define DESK_EXT_DISPLAYKEY TEXT("Display Key")
#define DESK_EXT_DISPLAYSTATEFLAGS TEXT("Display State Flags")
#define DESK_EXT_MONITORNAME TEXT("Monitor Name")
#define DESK_EXT_MONITORDEVICE TEXT("Monitor Device")
#define DESK_EXT_MONITORID TEXT("Monitor ID")

typedef PDEVMODEW (DESK_EXT_CALLBACK *PDESK_EXT_ENUMALLMODES)(PVOID Context, DWORD Index);
typedef PDEVMODEW (DESK_EXT_CALLBACK *PDESK_EXT_GETCURRENTMODE)(PVOID Context);
typedef BOOL (DESK_EXT_CALLBACK *PDESK_EXT_SETCURRENTMODE)(PVOID Context, const DEVMODEW *pDevMode);
typedef VOID (DESK_EXT_CALLBACK *PDESK_EXT_GETPRUNINGMODE)(PVOID Context, PBOOL pbModesPruned, PBOOL pbKeyIsReadOnly, PBOOL pbPruningOn);
typedef VOID (DESK_EXT_CALLBACK *PDESK_EXT_SETPRUNINGMODE)(PVOID Context, BOOL PruningOn);

typedef struct _DESK_EXT_INTERFACE
{
    /* NOTE: This structure is binary compatible to XP. The windows shell
             extensions rely on this structure to be properly filled! */
    DWORD cbSize;

    PVOID Context; /* This value is passed on to the callback routines */

    /* Callback routines called by the shell extensions */
    PDESK_EXT_ENUMALLMODES EnumAllModes;
    PDESK_EXT_SETCURRENTMODE SetCurrentMode;
    PDESK_EXT_GETCURRENTMODE GetCurrentMode;
    PDESK_EXT_SETPRUNINGMODE SetPruningMode;
    PDESK_EXT_GETPRUNINGMODE GetPruningMode;

    /* HardwareInformation.* values provided in the device registry key */
    WCHAR MemorySize[128];
    WCHAR ChipType[128];
    WCHAR DacType[128];
    WCHAR AdapterString[128];
    WCHAR BiosString[128];
} DESK_EXT_INTERFACE, *PDESK_EXT_INTERFACE;

#endif /* __DESKCPLX__H */
