
#define DESKCPLEXT_DISPLAY_DEVICE       TEXT("Display Device")  // "\DisplayX"
#define DESKCPLEXT_DISPLAY_NAME         TEXT("Display Name")    // " ATI Mach64 Turbo 3 "
#define DESKCPLEXT_MONITOR_DEVICE       TEXT("Monitor Device")  // " \DisplayX\MonitorX "
#define DESKCPLEXT_MONITOR_NAME         TEXT("Monitor Name")    // " NEC Multi-sync II "
#define DESKCPLEXT_PRUNING_MODE         TEXT("Pruning Mode")    // 1 = on (default value), 0 = off

#define DESKCPLEXT_DISPLAY_DEVICE_KEY   TEXT("Display Key")     // "\REGISTRY\MACHINE\SYSTEM\ControlSet00X\Services\<driver>\DeviceY"
#define DESKCPLEXT_DISPLAY_ID           TEXT("Display ID")      // "ROOT\*PNP0F03\1-0-21-0-31-0"
#define DESKCPLEXT_DISPLAY_STATE_FLAGS  TEXT("Display State Flags")
#define DESKCPLEXT_MONITOR_ID           TEXT("Monitor ID")      // "ROOT\*PNP0F04\1-0-21-0-31-0"
#define DESKCPLEXT_INTERFACE            TEXT("Desk.cpl extension interface")

typedef
LPDEVMODEW
(*LPDESKCPLEXT_ENUM_ALL_MODES) (
    LPVOID pContext,
    DWORD iMode
    );

typedef
LPDEVMODEW
(*LPDESKCPLEXT_GET_SELECTED_MODE) (
    LPVOID pContext
    );

typedef
BOOL
(*LPDESKCPLEXT_SET_SELECTED_MODE) (
    LPVOID pContext,
    LPDEVMODEW lpdm
    );

typedef
VOID 
(*LPDESKCPLEXT_GET_PRUNING_MODE) (
    LPVOID pContext,
    BOOL*  pbCanBePruned,
    BOOL*  pbIsReadOnly,
    BOOL*  pbIsPruningOn
    );
    
typedef
VOID 
(*LPDESKCPLEXT_SET_PRUNING_MODE) (
    LPVOID pContext,
    BOOL   bIsPruningOn
    );


typedef struct _DISPLAY_REGISTRY_HARDWARE_INFO {

    WCHAR MemSize[128];
    WCHAR ChipType[128];
    WCHAR DACType[128];
    WCHAR AdapString[128];
    WCHAR BiosString[128];

} DISPLAY_REGISTRY_HARDWARE_INFO, *PDISPLAY_REGISTRY_HARDWARE_INFO;




typedef struct _DESK_EXTENSION_INTERFACE {

    DWORD   cbSize;
    LPVOID  pContext;

    LPDESKCPLEXT_ENUM_ALL_MODES    lpfnEnumAllModes;
    LPDESKCPLEXT_SET_SELECTED_MODE lpfnSetSelectedMode;
    LPDESKCPLEXT_GET_SELECTED_MODE lpfnGetSelectedMode;
    LPDESKCPLEXT_SET_PRUNING_MODE  lpfnSetPruningMode;
    LPDESKCPLEXT_GET_PRUNING_MODE  lpfnGetPruningMode;
    
    DISPLAY_REGISTRY_HARDWARE_INFO Info;

} DESK_EXTENSION_INTERFACE, *PDESK_EXTENSION_INTERFACE;

#define NORMAL_TIMEOUT  7000
#define SLOW_TIMEOUT   12000

typedef
int
(*LPDISPLAY_SAVE_SETTINGS)   (
    LPVOID pContext,
    HWND   hwnd
    );

typedef
DWORD
(*LPDISPLAY_TEST_SETTINGS) (
    LPDEVMODEW lpDevMode,
    LPWSTR     pwszDevice,
    DWORD      dwTimeout
    );
