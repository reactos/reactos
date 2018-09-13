/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       POWERCFG.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*******************************************************************************/

#include "powrprof.h"
#include "batmeter.h"

// String constants and macros:
#define MAX_UI_STR_LEN          256
#define MAX_FRIENDLY_NAME_LEN   32      // Resource layout depends on this.
#define FREE_STR                TRUE
#define NO_FREE_STR             FALSE

#define PWRMANHLP TEXT("PWRMN.HLP")

// Hard limits, many are overridden by machine capabilities
// or registry settings:
#define MAX_VIDEO_TIMEOUT      300
#define MAX_SPINDOWN_TIMEOUT   300

/*******************************************************************************
*
*  Structures and constants to manage property pages in the applet.
*
*******************************************************************************/

#define MAX_PAGES      16+2         // Max number pages + caption & sentinal.
#define START_OF_PAGES 1            // Index to the property sheet pages.
#define CAPTION_INDEX  0            // Index to the overall caption.

typedef struct _POWER_PAGES
{
    LPCTSTR         pDlgTemplate;
    DLGPROC         pfnDlgProc;
    HPROPSHEETPAGE  hPropSheetPage;
} POWER_PAGES, *PPOWER_PAGES;

/*******************************************************************************
*
*  Structures and constants which manage dialog control information.
*
*******************************************************************************/

// Constants for MapXXXIndex functions:
#define VALUE_TO_INDEX TRUE
#define INDEX_TO_VALUE FALSE

// Constants used by SetControls in GETSET.C:
#define CONTROL_DISABLE     0
#define CONTROL_HIDE        1
#define CONTROL_ENABLE      2

// Proto for MapXXXIndex functions
typedef BOOL (*MAPFUNC)(LPVOID, PUINT, BOOL);

// Structure to manage the spin control data:
typedef struct _SPIN_DATA
{
    UINT    uiSpinId;
    PUINT   puiRange;
} SPIN_DATA, *PSPIN_DATA;

// Structure to manage the dialog controls specification:
typedef struct _POWER_CONTROLS
{
    UINT    uiID;
    UINT    uiType;
    LPVOID  lpvData;
    DWORD   dwSize;
    LPDWORD lpdwParam;
    LPDWORD lpdwState;

} POWER_CONTROLS, *PPOWER_CONTROLS;

// Dialog control constants:
#define CHECK_BOX               0
#define CHECK_BOX_ENABLE        1
#define SLIDER                  2
#define EDIT_UINT               3
#define EDIT_TEXT               6
#define EDIT_TEXT_RO            7
#define COMBO_BOX               8
#define PUSHBUTTON              9
#define STATIC_TEXT             10
#define GROUPBOX_TEXT           11

/*******************************************************************************
*
*                 P u b l i c   P r o t o t y p e s
*
*******************************************************************************/

// Public functions implemented in ALARM.C
INT_PTR CALLBACK AlarmDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AlarmActionsDlgProc(HWND, UINT, WPARAM, LPARAM);

// Public functions implemented in BATMTRCF.C
INT_PTR CALLBACK BatMeterCfgDlgProc(HWND, UINT, WPARAM, LPARAM);

// Public functions implemented in APM.C
INT_PTR CALLBACK APMDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOLEAN IsNtApmPresent(PSYSTEM_POWER_CAPABILITIES);

// Public functions implemented in ups.lib
INT_PTR CALLBACK UPSMainPageProc(HWND, UINT, WPARAM, LPARAM);
BOOLEAN IsUpsPresent(PSYSTEM_POWER_CAPABILITIES);

// Public functions implemented in GETSET.C
DWORD SelToFromPowerAction(HWND, UINT, LPVOID, LPARAM, BOOL);
DWORD PowerActionToStatus(HWND, UINT, LPVOID, LPARAM, BOOL);
VOID  DisableControls(HWND, UINT, PPOWER_CONTROLS);
VOID  HideControls(HWND, UINT, PPOWER_CONTROLS);
BOOL  SetControls(HWND, UINT, PPOWER_CONTROLS);
BOOL  GetControls(HWND, UINT, PPOWER_CONTROLS);
VOID  RangeLimitIDarray(PUINT, UINT, UINT);

// Public functions implemented in HIBERNAT.C:
void DoHibernateApply(void);
INT_PTR CALLBACK HibernateDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL MapPwrAct(PPOWER_ACTION, BOOL);

// Public functions implemented in POWERCFG.C:
LPTSTR CDECL   LoadDynamicString(UINT StringID, ... );
LPTSTR         DisplayFreeStr(HWND, UINT, LPTSTR, BOOL);
BOOLEAN        ValidateUISchemeFields(PPOWER_POLICY);
BOOLEAN        GetGlobalPwrPolicy(PGLOBAL_POWER_POLICY);
BOOLEAN        WritePwrSchemeReport(HWND, PUINT, LPTSTR, LPTSTR, PPOWER_POLICY);
BOOLEAN        WriteGlobalPwrPolicyReport(HWND, PGLOBAL_POWER_POLICY);
BOOLEAN        SetActivePwrSchemeReport(HWND, UINT, PGLOBAL_POWER_POLICY, PPOWER_POLICY);
int            ErrorMsgBox(HWND, DWORD, UINT);
BOOL           InitCapabilities(PSYSTEM_POWER_CAPABILITIES);

// Public functions implemented in PRSHTHLP.C:
BOOL AppendPropSheetPage(PPOWER_PAGES, UINT, DLGPROC);
UINT GetNumPropSheetPages(PPOWER_PAGES);
BOOL CALLBACK _AddPowerPropSheetPage(HPROPSHEETPAGE hpage, LPARAM lParam);
BOOL PASCAL DoPropSheetPages(HWND, PPOWER_PAGES, LPTSTR);
VOID MarkSheetDirty(HWND, PBOOL);

// Public functions implemented in PWRSCHEM.C
VOID InitSchemesList(VOID);
INT_PTR CALLBACK PowerSchemeDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AdvPowerSchemeDlgProc(HWND, UINT, WPARAM, LPARAM);

// Public functions implemented in PWRSWTCH.C
INT_PTR CALLBACK AdvancedDlgProc(HWND, UINT, WPARAM, LPARAM);


