/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    ocpage.cpp

Abstract:

    This file implements the display page setup.

Environment:

    WIN32 User Mode

Author:

    Doron J. Holan 

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef WINNT

#include <stdio.h>
#include <tchar.h>
#include "ntreg.hxx"
#include "settings.hxx"
#include "migrate.h"

#if DEBUG
#define new DBG_NEW
#endif

#define NO_SETUP_PAGE 1

struct CDisplaySetup {
public:

    CDisplaySetup();
    ~CDisplaySetup();
    
    void Init(void);
    void Commit(void);

    static SETUP_INIT_COMPONENT m_SetupInitComponent;

    static BOOLEAN IsSetupBatch(void)
        {return m_SetupInitComponent.SetupData.OperationFlags & SETUPOP_BATCH ? TRUE : FALSE;}
    
    static BOOLEAN IsSetupNTUpgrade(void)
        {return m_SetupInitComponent.SetupData.OperationFlags & SETUPOP_NTUPGRADE ? TRUE : FALSE;}

    static BOOLEAN IsSetupStandAlone(void)
        {return m_SetupInitComponent.SetupData.OperationFlags & SETUPOP_STANDALONE ? TRUE : FALSE;}

    static INT_PTR DisplayDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam); 

private:
    HWND          m_hDlg;

    LPTSTR m_pszDisplay;

    BOOL m_IsVga                :1;
    BOOL m_PrimaryDisplay       :1;
    BOOL m_BadDriver            :1;
    BOOL m_DisplayIsBad         :1;
    BOOL m_bResolutionChanged   :1;
    BOOL m_bPrevSetActive       :1;
    BOOL m_bUnattendedPresent   :1;
    BOOL m_bInstalled           :1;

    LONG m_UnattendedBitsPerPel;
    LONG m_UnattendedXResolution;
    LONG m_UnattendedYResolution;
    LONG m_UnattendedVRefresh;

    CDeviceSettings *m_pds;
    DISPLAY_DEVICE m_DisplayDevice;

    int            m_iFrequency;
    PLONGLONG      m_FrequencyList;
    int            m_FrequencyCount;

    int            m_iColor;
    PLONGLONG      m_ColorList;
    int            m_ColorCount; 

    int            m_iResolution;
    PPOINT         m_ResolutionList;
    int            m_ResolutionCount;

    DWORD          m_timeout;

    UINT           m_fieldCount; // number of fields in the [Display] section
                                 // of the unattended answer file.
    //
    //
    // Message Handlers
    //
    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnCommand(DWORD NotifyCode, DWORD ControlId, HWND hwndControl);
    BOOL OnDestroy(void);
    BOOL OnHScroll(HWND hwndTB, DWORD iCode, DWORD iPos);
    BOOL OnInitDialog(IN HWND hDlg, IN HWND hwndFocus, IN LPARAM lParam);
    BOOL OnNotify(IN WPARAM ControlId, IN LPNMHDR pnmh);

    BOOL EnableNextButton(BOOL bEnable)
    { return PropSheet_SetWizButtons(GetParent(m_hDlg),
                                     bEnable ? (PSWIZB_BACK | PSWIZB_NEXT) :
                                               PSWIZB_BACK); }

    //
    // Worker functions
    //
    void ValidateUpgradeValues(LONG XResolution, LONG YResolution, LONG Color, LONG Frequency);
#if NO_SETUP_PAGE
    void DoCleanInstall(void);
#endif
    void DoRegistryUpgrade(void); 
    void DoUnattendedUpgrade(void);

    BOOL InitDisplayDevice(void);

    BOOL TestVideoSettings(void);

    void UpdateUI(void);
    BOOL GetRegistrySettings(LONG *pXRes, LONG *pYRes, LONG *pBpp, LONG *pFrequency);
    BOOL GetPreferredSettings(LONG *pXRes, LONG *pYRes, LONG *pBpp, LONG *pFrequency);
    void GetDefaultSettings(LONG *pXRes, LONG *pYRes, LONG *pBpp, LONG *pFrequency);
    BOOL IsVga(PDISPLAY_DEVICE pDisplayDevice);
};

#define DEFAULT_XRESOLUTION 640
#define DEFAULT_YRESOLUTION 480
#define DEFAULT_BPP          15
#define DEFAULT_VREFRESH     60

SETUP_INIT_COMPONENT CDisplaySetup::m_SetupInitComponent;


//
// defines
//

#define CCH_MAX_STRING          256
#define SZ_DISPLAY_4BPP_MODES   TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\Display4BppModes")
#define SZ_DISPLAY_SMALL_MODES  TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\DisplaySmallModes")

#define SZ_REBOOT_NECESSARY     TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\RebootNecessary")
#define SZ_DETECT_DISPLAY       TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\DetectDisplay")
#define SZ_CONFIGURE_AT_LOGON   TEXT("ConfigureAtLogon")
#define SZ_NEW_DISPLAY          TEXT("System\\CurrentControlSet\\Control\\GraphicsDrivers\\NewDisplay")

#define SZ_UNATTEND_INSTALL     TEXT("InstallDriver")
#define SZ_UNATTEND_INF         TEXT("InfFile")
#define SZ_UNATTEND_OPTION      TEXT("InfOption")
#define SZ_UNATTEND_BPP         TEXT("BitsPerPel")
#define SZ_UNATTEND_X           TEXT("XResolution")
#define SZ_UNATTEND_Y           TEXT("YResolution")
#define SZ_UNATTEND_REF         TEXT("VRefresh")
#define SZ_UNATTEND_FLAGS       TEXT("Flags")
#define SZ_UNATTEND_CONFIRM     TEXT("AutoConfirm")

#define SZ_PRIVATE_DATA_NAME    TEXT("Display.cpl - Private Data")

//
// Function prototypes
//
DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    );

DWORD
HandleOcRequestPages(
    PSETUP_REQUEST_PAGES SetupRequestPages,
    WizardPagesType      PageType
    );

DWORD
HandleOcCompleteInstallation(
    VOID
    );

DWORD
HandleOcCleanUp(
    VOID
    );

extern "C" {

DWORD
DisplayOcSetupProc(
    IN LPCVOID ComponentId,
    IN LPCVOID SubcomponentId,
    IN UINT Function,
    IN UINT Param1,
    IN OUT PVOID Param2
    )
{
    switch (Function) {
    
    case OC_PREINITIALIZE:
        return OCFLAG_UNICODE;

    case OC_INIT_COMPONENT:
        return HandleOcInitComponent((PSETUP_INIT_COMPONENT) Param2);

    case OC_REQUEST_PAGES:
        return HandleOcRequestPages((PSETUP_REQUEST_PAGES) Param2,
                                    (WizardPagesType) Param1);

    case OC_COMPLETE_INSTALLATION:
        return HandleOcCompleteInstallation();

    case OC_QUERY_STATE:
        // we are always installed
        return SubcompOn;

    case OC_CLEANUP:
        return HandleOcCleanUp();

    default:
        break;
    }

    return ERROR_SUCCESS;
}

} // extern "C"

DWORD
HandleOcInitComponent(
    PSETUP_INIT_COMPONENT SetupInitComponent
    )
{
    TraceMsg(TF_OC, "OCMANAGER_VERSION = 0x%x, Version = 0x%x",
             OCMANAGER_VERSION, SetupInitComponent->OCManagerVersion);
    
    if (OCMANAGER_VERSION <= SetupInitComponent->OCManagerVersion) {
        SetupInitComponent->ComponentVersion = OCMANAGER_VERSION;
    }
    else {
        TraceMsg(TF_OC, "OC:  bailing on init component");
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    CopyMemory(&CDisplaySetup::m_SetupInitComponent,
               (LPVOID)SetupInitComponent,
               sizeof(SETUP_INIT_COMPONENT));

    return ERROR_SUCCESS;
}

LPTSTR OCDupStr(LPTSTR src)
{
    LPTSTR dest;
    dest = (LPTSTR) LocalAlloc(LPTR, (lstrlen(src) + 1) * sizeof(TCHAR));
    if (dest) {
        lstrcpy(dest, src);
    }
    return dest;
}

DWORD
HandleOcRequestPages(
    PSETUP_REQUEST_PAGES SetupRequestPages,
    WizardPagesType      PageType
    )
{
    PROPSHEETPAGE   psp;
    HPROPSHEETPAGE  hPsp;
    TCHAR           titleBuffer[256];
    CDisplaySetup   *display = NULL;

    //
    // If this is not gui mode setup, don't display pages
    //
    if (CDisplaySetup::IsSetupStandAlone()) {
        return ERROR_SUCCESS; 
    }

    if (PageType != WizPagesEarly) {
        return ERROR_SUCCESS;
    }

    TraceMsg(TF_OC, "adding an early page");

    gbExecMode = EXEC_SETUP;

    display = new CDisplaySetup;
    if (!display) {
        TraceMsg(TF_OC, "could not a CDisplaySetup object");
        return ERROR_SUCCESS;
    }

    (*CDisplaySetup::m_SetupInitComponent.HelperRoutines.SetPrivateData)(
        CDisplaySetup::m_SetupInitComponent.HelperRoutines.OcManagerContext,
        SZ_PRIVATE_DATA_NAME,
        (PVOID) &display,
        sizeof(display),
        REG_DWORD);
        
    display->Init();

    ZeroMemory(&psp, sizeof(PROPSHEETPAGE));

    psp.dwSize             = sizeof(PROPSHEETPAGE);
    psp.dwFlags            = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.hInstance          = hInstance;
    psp.pszTemplate        = MAKEINTRESOURCE(DLG_WIZPAGE);
    psp.pfnDlgProc         = CDisplaySetup::DisplayDlgProc;

    psp.lParam             = (LPARAM) display;

    if (LoadString(hInstance,
                   IDS_DISPLAY_WIZ_TITLE,
                   titleBuffer,
                   sizeof(titleBuffer)/sizeof(TCHAR) )) {
        psp.pszHeaderTitle = _wcsdup(titleBuffer);
    }

    if (LoadString(hInstance,
                   IDS_DISPLAY_WIZ_SUBTITLE,
                   titleBuffer,
                   sizeof(titleBuffer)/sizeof(TCHAR) )) {
        psp.pszHeaderSubTitle = _wcsdup(titleBuffer);
    }

    hPsp = CreatePropertySheetPage(&psp);

    if (hPsp) {
        TraceMsg(TF_OC, "successfully added page");
        SetupRequestPages->Pages[0] = hPsp;
        return 1;
    }

    TraceMsg(TF_OC, "failed in adding page!");
    return 0;
}

DWORD
HandleOcCompleteInstallation(
    VOID
    )
{
    CDisplaySetup *display = NULL;
    UINT          size, type, ret;

    size = sizeof(display);
    ret = (*CDisplaySetup::m_SetupInitComponent.HelperRoutines.GetPrivateData)(
        CDisplaySetup::m_SetupInitComponent.HelperRoutines.OcManagerContext,
        NULL,
        SZ_PRIVATE_DATA_NAME,
        (PVOID) &display,
        &size, 
        &type);
        
    if (type == REG_DWORD && display) {
        TraceMsg(TF_OC, "complete install got display object");

        if (!CDisplaySetup::IsSetupStandAlone()) {
            display->Commit();
        }
    }
    else {
        TraceMsg(TF_OC, "complete install did not get display object (0x%x)", ret);
    }

    return ERROR_SUCCESS;
}

DWORD
HandleOcCleanUp(
    VOID
    )
{
    CDisplaySetup *display = NULL;
    UINT          size, type, ret;

    size = sizeof(display);
    ret = (*CDisplaySetup::m_SetupInitComponent.HelperRoutines.GetPrivateData)(
        CDisplaySetup::m_SetupInitComponent.HelperRoutines.OcManagerContext,
        NULL,
        SZ_PRIVATE_DATA_NAME,
        (PVOID) &display,
        &size, 
        &type);
        
    if (type == REG_DWORD && display) {
        TraceMsg(TF_OC, "clean up got display object");
        delete display;
    }
    else {
        TraceMsg(TF_OC, "clean up did not get display object (0x%x)", ret);
    }

    return ERROR_SUCCESS;
}

typedef struct _NEW_DESKTOP_PARAM {
    LPDEVMODE lpdevmode;
    LPTSTR pwszDevice;
    DWORD  dwTimeout;
} NEW_DESKTOP_PARAM, *PNEW_DESKTOP_PARAM;



// table of resolutions that we show off.
// if the resolution is larger, then we show that one too.

typedef struct tagRESTAB {
    INT xRes;
    INT yRes;
    COLORREF crColor;           // color to paint this resolution
} RESTAB;

RESTAB ResTab[] ={
   { 1600, 1200, RGB(255,0,0)},
   { 1280, 1024, RGB(0,255,0)},
   { 1152,  900, RGB(0,0,255)},
   { 1024,  768, RGB(255,0,0)},
   {  800,  600, RGB(0,255,0)},
   // 640x480 or 640x400 handled specially
   { 0, 0, 0}         // end of table
   };

/****************************************************************************

    FUNCTION: Set1152Mode

    PURPOSE:  Set the height of the 1152 mode since it varies from card to
              card.

****************************************************************************/
VOID Set1152Mode(int height)
{
    ResTab[2].yRes = height;
}

#define CHARTOPSZ(ch)       ((LPTSTR)(DWORD)(WORD)(ch))
#define SZ_BACKBACKDOT          TEXT("\\\\.\\")
#define SZ_VIDEOMAP             TEXT("HARDWARE\\DEVICEMAP\\VIDEO")
#define SZ_REGISTRYMACHINE      TEXT("\\REGISTRY\\MACHINE\\")
#define SZ_DEVICEDESCRIPTION    TEXT("Device Description")
#define SZ_INSTALLEDDRIVERS     TEXT("InstalledDisplayDrivers")
#define SZ_DOTSYS               TEXT(".sys")
#define SZ_DOTDLL               TEXT(".dll")
#define SZ_FILE_SEPARATOR       TEXT(", ")

LPTSTR SubStrEnd(LPTSTR pszTarget, LPTSTR pszScan );

LPTSTR
FmtVSprint(
    DWORD id,
    va_list marker
    )
{
    LPTSTR pszMsg = NULL;
    TCHAR  szBuf[MAX_PATH]; 
    size_t size;
#define BUF_INCREMENT 50

    if (!LoadString(hInstance, id, szBuf, MAX_PATH)) {
        pszMsg = OCDupStr(TEXT("..."));
    }
    else {
        size = lstrlen(szBuf);
try_again:
        size += BUF_INCREMENT;
        if (pszMsg != NULL) {
            LocalFree(pszMsg);
            pszMsg = NULL;
        }

        pszMsg = (LPTSTR) LocalAlloc(LMEM_ZEROINIT, size * sizeof(TCHAR));

        if (pszMsg) {
           if (_vsntprintf(pszMsg, size, szBuf, marker) < 0) {
               // pszMsg is too small, retry w/a bigger buffer
               goto try_again;
           }
        }
        else {
            pszMsg = OCDupStr(TEXT("..."));
        }
    }

    return pszMsg;
}


LPTSTR
FmtSprint(
    DWORD id,
    ...
    )
{
    LPTSTR pszMsg;
    va_list marker;

    va_start(marker, id);
    pszMsg = FmtVSprint(id, marker);
    va_end(marker);

    return pszMsg;
}

/***************************************************************************\
*
*     FUNCTION: FmtMessageBox(HWND hwnd, int dwTitleID, UINT fuStyle,
*                   BOOL fSound, DWORD dwTextID, ...);
*
*     PURPOSE:  Formats messages with FormatMessage and then displays them
*               in a message box
*
*     PARAMETERS:
*               hwnd        - parent window for message box
*               fuStyle     - MessageBox style
*               fSound      - if TRUE, MessageBeep will be called with fuStyle
*               dwTitleID   - Message ID for optional title, "Error" will
*                             be displayed if dwTitleID == -1
*               dwTextID    - Message ID for the message box text
*               ...         - optional args to be embedded in dwTextID
*                             see FormatMessage for more details
* History:
* 22-Apr-1993 JonPa         Created it.
\***************************************************************************/
int
FmtMessageBox(
    HWND hwnd,
    UINT fuStyle,
    BOOL fSound,
    DWORD dwTitleID,
    DWORD dwTextID,
    ...
    )
{

    LPTSTR pszMsg;
    LPTSTR pszTitle;
    int idRet;

    va_list marker;

    va_start(marker, dwTextID);
    pszMsg = FmtVSprint(dwTextID, marker);
    va_end(marker);

    pszTitle = NULL;

    if (dwTitleID != -1) {
        pszTitle = FmtSprint(dwTitleID);
    }

    //
    // Turn on the beep if requested
    //
    if (fSound) {
        MessageBeep(fuStyle & (MB_ICONASTERISK | MB_ICONEXCLAMATION |
                MB_ICONHAND | MB_ICONQUESTION | MB_OK));
    }

    idRet = MessageBox(hwnd, pszMsg, pszTitle, fuStyle);

    if (pszTitle != NULL)
        LocalFree(pszTitle);

    if (pszMsg != NULL)
        LocalFree(pszMsg);

    return idRet;
}

/****************************************************************************\
*
* CloneString
*
* Makes a copy of a string.  By copy, I mean it actually allocs memeory
* and copies the chars across.
*
* NOTE: the caller must LocalFree the string when they are done with it.
*
* Returns a pointer to a LocalAlloced buffer that has a copy of the
* string in it.  If an error occurs, it retuns NULl.
*
* 16-Dec-1993 JonPa     Wrote it.
\****************************************************************************/
LPTSTR CloneString(LPTSTR psz ) {
    int cb;
    LPTSTR psz2;

    if (psz == NULL)
        return NULL;

    cb = (lstrlen(psz) + 1) * sizeof(TCHAR);

    psz2 = (LPTSTR)LocalAlloc(LMEM_ZEROINIT, cb);
    if (psz2 != NULL)
        CopyMemory(psz2, psz, cb);

    return psz2;
}

CDisplaySetup::CDisplaySetup() :
    m_pds(NULL), m_hDlg(0),
   
    m_BadDriver(FALSE), m_IsVga(FALSE), 
    m_PrimaryDisplay(FALSE), m_DisplayIsBad(FALSE), m_bPrevSetActive(FALSE),
    m_pszDisplay(NULL), 

    m_timeout(NORMAL_TIMEOUT),

    m_bUnattendedPresent(FALSE),  m_bInstalled(FALSE),
    m_UnattendedBitsPerPel(0), m_UnattendedXResolution(0),
    m_UnattendedYResolution(0), m_UnattendedVRefresh(0),

    m_ColorCount(0), m_ColorList(NULL), m_iColor(-1),
    m_ResolutionCount(0), m_ResolutionList(NULL), m_iResolution(-1),
    m_FrequencyCount(0), m_FrequencyList(0), m_iFrequency(-1), m_bResolutionChanged(0),
    m_fieldCount(0)
{
    DeskOpenLog();
}        

CDisplaySetup::~CDisplaySetup()
{
    if (m_pds) {
        delete m_pds;
        m_pds = 0;
    }

    if (m_ResolutionList) {
        LocalFree(m_ResolutionList);
        m_ResolutionList = 0;
    }
    if (m_FrequencyList) {
        LocalFree(m_FrequencyList);
        m_FrequencyList = 0;
    }
    if (m_ColorList) {
        LocalFree(m_ColorList);
        m_ColorList = 0;
    }
    
    DeskCloseLog();

    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_DETECT_DISPLAY);
    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_NEW_DISPLAY);
    RegDeleteKey(HKEY_LOCAL_MACHINE, SZ_UPDATE_SETTINGS);
}

void
CDisplaySetup::Init(
    void
    )
{
    INFCONTEXT  context;
    HINF        hInf;
    TCHAR       szName[128];
    DWORD       value;

    //
    // the only thing we can safely do here is get the unattended
    // parameters.  the init of the display objects cannot happen here
    // because guimode setup has not yet completed the device
    // detection phase of setup.  the display init is delayed
    // until the wizard dialog is initialized.
    //

    m_fieldCount = 0;

    if (!IsSetupBatch()) {
        m_bUnattendedPresent = FALSE;
        return;
    }

    hInf = m_SetupInitComponent.HelperRoutines.GetInfHandle(
            INFINDEX_UNATTENDED,
            m_SetupInitComponent.HelperRoutines.OcManagerContext);

    if (hInf == NULL || hInf == (HINF) INVALID_HANDLE_VALUE) {
        m_bUnattendedPresent = FALSE;
        return;
    }

    if (SetupFindFirstLine(hInf, TEXT("Display"), NULL, &context)) {
        do {
            if (SetupGetStringField(&context,
                                    0,
                                    szName,
                                    sizeof(szName) / sizeof(TCHAR),
                                    &value)) {
    
                if (lstrcmpi(szName, TEXT("BitsPerPel")) == 0) {
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_058, 
                                     value);
                        TraceMsg(TF_SETUP, "Unattended BitsPerPel:  %d", value);
                        m_UnattendedBitsPerPel = value;
                        m_fieldCount++;
                    }
                    else {
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            sizeof(szName) / sizeof(TCHAR),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_096,
                                     szName);
                    }
                }
                else if (lstrcmpi(szName, TEXT("Xresolution")) == 0) {
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_059, 
                                     value);
                        TraceMsg(TF_SETUP, "Unattended Xresolution:  %d", value);
                        m_UnattendedXResolution = value;
                        m_fieldCount++;
                    }
                    else {
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            sizeof(szName) / sizeof(TCHAR),
                                            &value);
                        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_060);
                    }
                }
                else if (lstrcmpi(szName, TEXT("YResolution")) == 0) {
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_061, 
                                     value);
                        TraceMsg(TF_SETUP, "Unattended Yresolution:  %d", value);
                        m_UnattendedYResolution = value;
                        m_fieldCount++;
                    }
                    else {
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            sizeof(szName) / sizeof(TCHAR),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_062,
                                     szName);
                    }
                }
                else if (lstrcmpi( szName, TEXT("VRefresh")) == 0) {
                    if (SetupGetIntField(&context, 1, (PINT) &value)) {
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_063, 
                                     value);
                        TraceMsg(TF_SETUP, "Unattended Vrefresh:  %d", value);
                        m_UnattendedVRefresh = value;
                        m_fieldCount++;
                    }
                    else {
                        SetupGetStringField(&context,
                                            1,
                                            szName,
                                            sizeof(szName) / sizeof(TCHAR),
                                            &value);
                        DeskLogError(LogSevInformation,
                                     IDS_SETUPLOG_MSG_064,
                                     szName);
                    }
                }
                else {
                    DeskLogError(LogSevInformation,
                                 IDS_SETUPLOG_MSG_065,
                                 szName);
                }
            }
    
        } while (SetupFindNextLine(&context, &context));

    }

    if (m_fieldCount == 0 && IsSetupNTUpgrade()) {
        //
        // Unattended install, but no display values, use the previous OS's 
        // values when the time is right
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_066);

        m_bUnattendedPresent = FALSE;
        return;
    }

    m_bUnattendedPresent = TRUE;

    if(0 != m_fieldCount)
    {
        // if [Display] section doesn't contain all display settings fields, we fill in the missing ones with default settings 
        // if [Display] section is missing or empty, we get the display settings in DoUnattendedUpgrade.
        //              we can not do it here because we need to call EnumDisplaySettings.          

        if (m_UnattendedXResolution == 0 || m_UnattendedYResolution == 0) {
            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_067,
                         DEFAULT_XRESOLUTION, 
                         DEFAULT_YRESOLUTION);
            m_UnattendedXResolution = DEFAULT_XRESOLUTION;
            m_UnattendedYResolution = DEFAULT_YRESOLUTION;
        }                                                  

        if (m_UnattendedVRefresh == 0) {
            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_068,
                         DEFAULT_VREFRESH);
            m_UnattendedVRefresh = DEFAULT_VREFRESH;
        }

        if (m_UnattendedBitsPerPel == 0) {
            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_069,
                         DEFAULT_BPP);
            m_UnattendedBitsPerPel = DEFAULT_BPP;
        }
    }
}

void CDisplaySetup::ValidateUpgradeValues(
    LONG XResolution,
    LONG YResolution,
    LONG Color,
    LONG Frequency
    )
/*++

Validate the unattended values before applying them.  If a certain aspect of the
desired devmode is not supported, find the closest match.  The values have the 
following priority (most important to least):
1)  Resolution
2)  Color Depth (BPP)
3)  Refresh rate

  --*/
{
    PLONGLONG pFrequencyList, pColorList;
    PPOINT    pResolutionList;
    int       cColor, cFrequency, cResolution, i;
    int       difference, curDiff;
    LONG      validColor, validFrequency;
    POINT     res, validRes;
 
    //
    // !!! If the device is Vga, we want to use 640x480.
    // !!! Otherwise, we will end up using 800x600 in safe mode 
    // !!!    after an NT4 to NT5 upgrade.
    // 
    
    if(m_IsVga) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_070);
        GetDefaultSettings(&XResolution, &YResolution, &Color, &Frequency);
    }
        
    difference = -1;
    cResolution = m_pds->GetResolutionList(-1, &pResolutionList);
    MAKEXYRES(&res, XResolution, YResolution);
 
    //
    // Try to find an exact match on resolution
    //
    for (i = 0; i < cResolution; i++) {
        curDiff = abs(res.x - pResolutionList[i].x) +
                  abs(res.y - pResolutionList[i].y);
 
        if (curDiff == 0) 
            break;
 
        if (difference == -1 || curDiff < difference) {
            difference = curDiff;
            validRes = pResolutionList[i];
        }
    }
 
    if (curDiff != 0) {
        //
        // Desired resolution didn't exist, report this and use the valid one
        //
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_071,
                     res.x, res.y, validRes.x, validRes.y);
        res = validRes;
    }
    m_pds->SetCurResolution(&res);

    //
    // Enumerate all the valid BPPs for this resolution
    //
    cColor = m_pds->GetColorList(&res, &pColorList);
    difference = -1;
 
    for (i = 0; i < cColor; i++) {
        curDiff = abs(Color - (LONG) pColorList[i]);
 
        if (curDiff == 0) 
            break;
 
        if (difference == -1 || curDiff < difference) {
            difference = curDiff;
            validColor = (LONG) pColorList[i];
        }
    }
 
    if (curDiff != 0) {
        //
        // Desired BPP at the desired resolution didn't exist, report this and 
        // use the valid one
        //
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_072,
                     Color, res.x, res.y, validColor);
 
        Color = validColor;
    }
    m_pds->SetCurColor(Color);
 
    //
    // Enumerate all the valid refresh rates at the desired resolution and BPP
    //
    cFrequency = m_pds->GetFrequencyList(Color,
                                         &res,
                                         &pFrequencyList);
    difference = -1;
 
    for (i = 0; i < cFrequency; i++) {
        curDiff = abs(Frequency - (LONG) pFrequencyList[i]);
 
        if (curDiff == 0) 
            break;
       
        //
        // BUGBUG perhaps we should just default to 60 Hz because it is the most wide
        // spread refresh rate?
        //
        if (difference == -1 || curDiff < difference) {
            difference = curDiff;
            validFrequency = (LONG) pFrequencyList[i];
        }
    }
 
    if (curDiff != 0) {
        //
        // Desired refresh at the desired resolution and BPP didn't exist,
        // report this and use the valid one
        //
        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_073,
                     Frequency, res.x, res.y, Color, validFrequency);
        Frequency = validFrequency;
    }
    m_pds->SetCurFrequency(Frequency);
 
    LocalFree(pResolutionList);
    LocalFree(pColorList);
    LocalFree(pFrequencyList);
}

void
CDisplaySetup::DoRegistryUpgrade(void)
{
    HKEY            hKey;
    DWORD           dwSize, dwType, dwVersion = 0xff, dwFailed = FALSE, dwPlanes;
    LONG            xres, yres, bpp, frequency, res;

    if (!m_pszDisplay) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_074);
        return;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     SZ_UPDATE_SETTINGS,
                     0,
                     KEY_READ,
                     &hKey) != ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_075);
        return;
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FAILED_ALLOW_INSTALL,
                        NULL,
                        &dwType,
                        (LPBYTE) &dwFailed,
                        &dwSize) == ERROR_SUCCESS &&
        dwFailed) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_076);
        return;
    }

    //
    // If we have unattended values, use them instead
    //
    if (m_bUnattendedPresent) {
        // delete the key before we leave
        RegCloseKey(hKey);
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_077);
        return;
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FROM_MAJOR_VERSION,
                        NULL,
                        &dwType,
                        (LPBYTE) &dwVersion,
                        &dwSize) == ERROR_SUCCESS &&
        dwVersion >= 5) {
        //
        // Only perform the upgrade on a non NT5 upgrade 
        //
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_078);
        RegCloseKey(hKey);
        return;
    }

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_079, m_pszDisplay);

    dwSize = sizeof(LONG);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FROM_PELS_WIDTH,
                        NULL,
                        &dwType,
                        (LPBYTE) &xres,
                        &dwSize) == ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_080, xres);
        TraceMsg(TF_GENERAL, "width is %d\n", xres);
    }

    dwSize = sizeof(LONG);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FROM_PELS_HEIGHT,
                        NULL,
                        &dwType,
                        (LPBYTE) &yres,
                        &dwSize) == ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_081, yres);
        TraceMsg(TF_GENERAL, "height is %d\n", yres);
    }

    dwSize = sizeof(LONG);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FROM_BITS_PER_PEL,
                        NULL,
                        &dwType,
                        (LPBYTE) &bpp,
                        &dwSize) == ERROR_SUCCESS) {
        dwSize = sizeof(DWORD);
        if (RegQueryValueEx(hKey,
                            SZ_UPGRADE_FROM_PLANES,
                            NULL,
                            &dwType,
                            (LPBYTE) &dwPlanes,
                            &dwSize) == ERROR_SUCCESS) {
            bpp *= dwPlanes;
            DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_082, bpp);
            TraceMsg(TF_GENERAL, "BPP is %d\n", bpp);
        }
    }

    dwSize = sizeof(DWORD);
    if (RegQueryValueEx(hKey,
                        SZ_UPGRADE_FROM_DISPLAY_FREQ,
                        NULL,
                        &dwType,
                        (LPBYTE) &frequency,
                        &dwSize) == ERROR_SUCCESS) {
        DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_083, frequency);
        TraceMsg(TF_GENERAL, "freq is %d\n", frequency);
    }

    ValidateUpgradeValues(xres, yres, bpp, frequency);
    m_pds->ConfirmChangeSettings();
    res = m_pds->SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET);

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_084, res);
    TraceMsg(TF_GENERAL, "CDS returned %d\n", res);

    RegCloseKey(hKey);
}

void
CDisplaySetup::DoUnattendedUpgrade(void)
{
    if (!m_pds) {
        DeskLogError(LogSevError, IDS_SETUPLOG_MSG_085);
        return;
    }


    int         ret;
    POINT       res;

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_086, m_pszDisplay);

    if(0 == m_fieldCount)
    {
        //
        // [Display] section of the answer file doesn't contain any display settings
        //
        // Get the display settings in the following order:
        // 1. registry settings
        // 2. monitor preferred mode
        // 3. default settings
        //
        if(!GetRegistrySettings(&m_UnattendedXResolution,
                                &m_UnattendedYResolution,
                                &m_UnattendedBitsPerPel,
                                &m_UnattendedVRefresh))
        {
            if(!GetPreferredSettings(&m_UnattendedXResolution,
                                     &m_UnattendedYResolution,
                                     &m_UnattendedBitsPerPel,
                                     &m_UnattendedVRefresh))
            {
                GetDefaultSettings(&m_UnattendedXResolution,
                                   &m_UnattendedYResolution,
                                   &m_UnattendedBitsPerPel,
                                   &m_UnattendedVRefresh);
            }
        }
    }
    ValidateUpgradeValues(m_UnattendedXResolution,
                          m_UnattendedYResolution,
                          m_UnattendedBitsPerPel,
                          m_UnattendedVRefresh);

    m_pds->ConfirmChangeSettings();
    ret = m_pds->SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET);

    m_pds->GetCurResolution(&res);
    DeskLogError(LogSevInformation,
                 IDS_SETUPLOG_MSG_087,
                 res.x, res.y, m_pds->GetCurColor(), m_pds->GetCurFrequency());

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_088, ret);
    TraceMsg(TF_GENERAL, "CDS returned %d\n", ret);
}

#if NO_SETUP_PAGE
void
CDisplaySetup::DoCleanInstall(void)
{
    if (!m_pds) {
        DeskLogError(LogSevError, IDS_SETUPLOG_MSG_089);
        return;
    }

    int         ret;
    POINT       res;
    LONG        xres, yres, bpp, frequency; 

    if (m_bInstalled) {
        return;
    }
    m_bInstalled = TRUE;

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_086, m_pszDisplay);

    //
    // Get the display settings in the following order:
    // 1. registry settings
    // 2. monitor preferred mode
    // 3. default settings
    //
    if(!GetRegistrySettings(&xres, &yres, &bpp, &frequency))
    {
        if(!GetPreferredSettings(&xres, &yres, &bpp, &frequency))
        {
            GetDefaultSettings(&xres, &yres, &bpp, &frequency);
        }
    }
    ValidateUpgradeValues(xres , yres , bpp, frequency);

    m_pds->ConfirmChangeSettings();
    ret = m_pds->SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET);

    m_pds->GetCurResolution(&res);
    DeskLogError(LogSevInformation,
                 IDS_SETUPLOG_MSG_090,
                 res.x, res.y, m_pds->GetCurColor(), m_pds->GetCurFrequency());

    DeskLogError(LogSevInformation, IDS_SETUPLOG_MSG_088, ret);
    TraceMsg(TF_GENERAL, "CDS returned %d\n", ret);
}
#endif

BOOL
CDisplaySetup::InitDisplayDevice(void)
{
    int passes = 0;
    BOOL VgaPrimary = FALSE;

    for (passes = 1; (passes <= 3) && (m_pszDisplay == NULL); passes++)
    {
        int iDevNum = 0;
        DISPLAY_DEVICE displayDevice;

        displayDevice.cb = sizeof(DISPLAY_DEVICE);

        while (EnumDisplayDevices(NULL, iDevNum++, &displayDevice, 0))
        {
            m_PrimaryDisplay = (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) ? TRUE : FALSE;

            if (passes == 1)
            {
                //
                // Search for the primary device.
                // However, we don't want to use VGA if that's the primary.
                //

                //
                // Determine if the VGA is the primary.
                //

                if (m_PrimaryDisplay)
                {
                    //
                    // If VGA is active, then go to pass 2.
                    // Otherwise, let's try to use this device
                    //
                    
                    VgaPrimary = IsVga(&displayDevice);
                }

                //
                // If the VGA is primary, then lets looks for a non-primary
                // by going to pass 2
                //

                if (VgaPrimary == TRUE) {

                    break;
                }

                //
                // If this is not a primary device, continue looking
                //

                if (!m_PrimaryDisplay) {

                    continue;
                }
            }
            else if (passes == 2) {

                //
                // Search for a non-primary, and try that for initialization.
                //
                if (m_PrimaryDisplay) {

                    continue;
                }

                //
                // Make sure it is a non mirroring device
                //
                if (displayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) {
                    continue;
                }

                //
                // We have a non-primary and non mirroring device - use it!
                //
            }
            else {
                //
                // If we make it to pass 3, then we want to try any device ...
                // just drop through ...
                //
            }

            CopyMemory(&m_DisplayDevice, &displayDevice, sizeof(DISPLAY_DEVICE));

            if (m_pds) {
                delete m_pds;
                m_pds = NULL;
            }

            m_pds = new CDeviceSettings;
            if (m_pds->InitSettings(&m_DisplayDevice)) {
                POINT pt = {0,0};

                m_IsVga = IsVga(&m_DisplayDevice);
                m_pds->SetAttached(TRUE);
                m_pds->SetCurPosition(&pt);
                m_pszDisplay = m_DisplayDevice.DeviceName; 
                m_BadDriver = FALSE;

                //
                // EnumDisplayDevices can cause a flash during setup.
                // So let's update the setup window here.
                //
                
                if (m_hDlg) {
                    HWND hSetup = GetParent(m_hDlg);
                    if (hSetup)
                        UpdateWindow(hSetup);
                }
                
                return TRUE;
            }
            else {
                delete m_pds;
                m_pds = NULL;

                //
                // We failed. Try the next video device.
                //
                m_BadDriver = TRUE;
                m_pszDisplay = NULL;
            }
        }
    }

    return FALSE;
}

BOOL 
CDisplaySetup::IsVga(
    PDISPLAY_DEVICE pDisplayDevice
    )
{
    CRegistrySettings rs(pDisplayDevice->DeviceKey);
    LPTSTR pszMini = rs.GetMiniPort();
    return (pszMini && (!lstrcmpi(TEXT("vga"), pszMini)));
}

void
CDisplaySetup::Commit(
    void
    )
{
    // on upgrade, there is nothing to commit
    if (IsSetupNTUpgrade()) {
        return;
    }

    if (m_DisplayIsBad) {
        return;
    }

    m_pds->SaveSettings(CDS_UPDATEREGISTRY | CDS_NORESET);
}

BOOL
CDisplaySetup::OnInitDialog(
    HWND hDlg,
    HWND hwndFocus,
    LPARAM lParam
    )
{
    int i;
    HWND hControl;

    m_hDlg = hDlg;

    //
    // initialize the display objects
    //
    if (!InitDisplayDevice()) {
        m_DisplayIsBad = TRUE;
        return TRUE;
    }

#if NO_SETUP_PAGE
    return TRUE;
#endif
    

    //
    // populate the controls with their data
    //       
    // EnableWindow(GetDlgItem(m_hDlg, IDC_TEST), FALSE);

    m_ResolutionCount = m_pds->GetResolutionList(-1, &m_ResolutionList);

    SendDlgItemMessage(m_hDlg,
                       IDC_SCREENSIZE,
                       TBM_SETRANGE,
                       (WPARAM) TRUE,
                       (LPARAM) MAKELONG(0, m_ResolutionCount - 1));

    m_ColorCount = m_pds->GetColorList(NULL, &m_ColorList);

    hControl = GetDlgItem(m_hDlg, IDC_COLORBOX);
    for (i = 0; i < m_ColorCount; i++) {
        TCHAR  achColor[50];
        int color;
        DWORD  idColor;

        color = (int) *(m_ColorList + i);

        //
        // convert bit count to number of colors and make it a string
        //
        switch (color) {
        case 32: idColor = ID_DSP_TXT_TRUECOLOR32; break;
        case 24: idColor = ID_DSP_TXT_TRUECOLOR24; break;
        case 16: idColor = ID_DSP_TXT_16BIT_COLOR; break;
        case 15: idColor = ID_DSP_TXT_15BIT_COLOR; break;
        case  8: idColor = ID_DSP_TXT_8BIT_COLOR; break;
        case  4: idColor = ID_DSP_TXT_4BIT_COLOR; break;
        default:
            ASSERT(FALSE);
        }

        LoadString(hInstance, idColor, achColor, sizeof(achColor));
        ComboBox_InsertString(hControl, i, achColor);
    }

    //
    // Set the default color value to 15 or 16 bpp, with 15 being the first choice,
    // otherwise leave it as is
    //
    POINT       curRes;
    PLONGLONG   curColorList = NULL;
    int         curColorCount = 0;
    BOOL        found15 = FALSE, found16 = FALSE;

    m_pds->GetCurResolution(&curRes);
    curColorCount = m_pds->GetColorList(&curRes, &curColorList);
    for (i = 0; i < curColorCount; i++) {
        if (curColorList[i] == 15) {
            found15 = TRUE;
        }
        if (curColorList[i] == 16) {
            found16 = TRUE;
        }
    }

    if (found15) {
        m_pds->SetCurColor(15);
    }
    else if (found16) {
        m_pds->SetCurColor(16);
    }
    LocalFree(curColorList);

    UpdateUI();

    return TRUE;
}

// 
// Update the resolution and refresh rates
//
// doronh TODO add updating of refresh rates
//
void CDisplaySetup::UpdateUI()
{
    int     i;
    POINT   res;
    int     color;
    int     frequency;
    HWND    hControl;

    //
    // Get the current values
    //
    m_pds->GetCurResolution(&res);
    color = m_pds->GetCurColor();
    frequency = m_pds->GetCurFrequency();
                
    //
    // Update the color listbox
    //
    hControl = GetDlgItem(m_hDlg, IDC_COLORBOX);
    for (i = 0; i < m_ColorCount; i++) {
        if (color == (int) *(m_ColorList + i)) {
            if (m_iColor == i) {
                TraceMsg(TF_SETUP, "UpdateUI() -- Set Color index %d - is current", i);
                break;
            }

            ComboBox_SetCurSel(hControl, i);
            m_iColor = i;

            break;
        }
    }

    if (i == m_ColorCount) {
        TraceMsg(TF_SETUP, "UpdateUI -- !!! inconsistent color list !!!");
    }


    //
    // Update the resolution string
    //
    TCHAR achStr[80];
    TCHAR achRes[120];

    LoadString(hInstance, ID_DSP_TXT_XBYY, achStr, sizeof(achStr));
    wsprintf(achRes, achStr, res.x, res.y);

    SendDlgItemMessage(m_hDlg,
                       IDC_RESXY,
                       WM_SETTEXT,
                       (WPARAM) 0,
                       (LPARAM) achRes);

    //
    // Update the resolution slider
    //
    for (i = 0; i < m_ResolutionCount; i++) {
        if ((res.x == (*(m_ResolutionList + i)).x) &&
            (res.y == (*(m_ResolutionList + i)).y)) {
            TraceMsg(TF_SETUP, "UpdateUI() -- Set Resolution index %d", i);

            if (m_iResolution == i) {
                TraceMsg(TF_SETUP, "UpdateUI() -- Set Resolution index %d - is current", i);
                break;
            }

            SendDlgItemMessage(m_hDlg,
                               IDC_SCREENSIZE,
                               TBM_SETPOS,
                               (WPARAM) TRUE,
                               (LPARAM) i);
            break;
        }
    }

    if (i == m_ResolutionCount) {
        TraceMsg(TF_SETUP, "UpdateUI -- !!! inconsistent color list !!!");
    }

    m_iResolution = i;

    hControl = GetDlgItem(m_hDlg, IDC_REFRESH_RATE);
    if (!m_bResolutionChanged || !m_FrequencyList) {
        TCHAR achFre[50];
        TCHAR achText[80];
    
        TraceMsg(TF_SETUP, "Getting new frequencies (changed = %d, list = 0x%x)\n",
                 m_bResolutionChanged, m_FrequencyList);

        if (m_FrequencyList) {
            LocalFree(m_FrequencyList);
            m_FrequencyList = NULL;
        }
    
        m_FrequencyCount = m_pds->GetFrequencyList(-1, NULL, &m_FrequencyList);
        frequency = m_pds->GetCurFrequency();
    
        ComboBox_ResetContent(hControl);
        for (i = 0; i < m_FrequencyCount; i++) {
            
            if (frequency == (int) m_FrequencyList[i]) {
                m_iFrequency = i;
            }
    
            if (m_FrequencyList[i] == 1) {
                LoadString(hInstance, IDS_DEFFREQ, achText, sizeof(achText));
            }
            else {
                DWORD  idFreq = IDS_FREQ;
    
                if (m_FrequencyList[i] < 50) {                   
                    idFreq = IDS_INTERLACED;
                }
    
                LoadString(hInstance, idFreq, achFre, sizeof(achFre));
                // this cast is crucial, otherwise the value passed is 64 bits
                wsprintf(achText, TEXT("%d %s"), (LONG) m_FrequencyList[i], achFre);
            }
    
            ComboBox_InsertString(hControl, i, achText);
        }
    }

    m_bResolutionChanged = FALSE;

    for (i = 0; i < m_FrequencyCount; i++) {
        if (frequency == (int) *(m_FrequencyList + i)) {
            if (m_iFrequency == i) {
                TraceMsg(TF_SETUP, "UpdateUI() -- Set Frequency index %d - is current", i);
                break;
            }

            ComboBox_SetCurSel(hControl, i);
            m_iFrequency = i;

            break;
        }
    }

    if (i == m_FrequencyCount) {
        TraceMsg(TF_SETUP, "UpdateUI -- !!! inconsistent frequency list !!!");
    }

    TraceMsg(TF_SETUP, "m_iFrequency = %d, count = %d, CB count = %d\n",
             m_iFrequency, m_FrequencyCount, ComboBox_GetCount(hControl));

    ComboBox_SetCurSel(hControl, m_iFrequency);

    EnableNextButton(TRUE);
}

DWORD WINAPI
ApplyNowThd(
    LPVOID lpThreadParameter
    );

DWORD
DisplayTestSettingsW(
    LPDEVMODEW lpDevMode,
    LPWSTR     pwszDevice,
    DWORD      dwTimeout
    )
{
    HANDLE hThread;
    DWORD idThread;
    DWORD bTest;
    NEW_DESKTOP_PARAM desktopParam;

    if (!lpDevMode || !pwszDevice) 
        return FALSE;

    if (dwTimeout == 0) 
        dwTimeout = NORMAL_TIMEOUT;

    desktopParam.lpdevmode = lpDevMode; 
    desktopParam.pwszDevice = pwszDevice;
    desktopParam.dwTimeout = dwTimeout;

    hThread = CreateThread(NULL,
                           4096,
                           ApplyNowThd,
                           (LPVOID) &desktopParam,
                           SYNCHRONIZE | THREAD_QUERY_INFORMATION,
                           &idThread
                           );

    WaitForSingleObject(hThread, INFINITE);
    GetExitCodeThread(hThread, &bTest);
    CloseHandle(hThread);

    return bTest;
}

BOOL
CDisplaySetup::TestVideoSettings(
    void
    )
{
    DWORD       bTest;
    LPDEVMODE   lpdm;
    BOOL        success;

    if (!m_pds->IsColorChanged()      &&
        !m_pds->IsResolutionChanged() &&
        !m_pds->IsFrequencyChanged()) {
        return TRUE;
    }
    
    if (FmtMessageBox(m_hDlg,
                      MB_OKCANCEL | MB_ICONINFORMATION,
                      FALSE,
                      ID_DSP_TXT_TEST_MODE,
                      m_timeout == NORMAL_TIMEOUT   ?
                        ID_DSP_TXT_DID_TEST_WARNING :
                        ID_DSP_TXT_DID_TEST_WARNING_LONG) == IDCANCEL) {
        return FALSE;
    }

    EnableWindow(m_hDlg, FALSE);
    lpdm = m_pds->GetCurrentDevMode();
    if (!lpdm) {
        return FALSE;
    }

    bTest = DisplayTestSettingsW(lpdm, m_pszDisplay, m_timeout);
    EnableWindow(m_hDlg, TRUE);

    if (bTest &&
        FmtMessageBox(m_hDlg,
                      MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2,
                      FALSE,
                      ID_DSP_TXT_TEST_MODE,
                      ID_DSP_TXT_DID_TEST_RESULT) == IDYES) {

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_091,
                     lpdm->dmPelsWidth, lpdm->dmPelsHeight, 
                     lpdm->dmBitsPerPel,
                     lpdm->dmDisplayFrequency);

        m_pds->ConfirmChangeSettings();

        //
        // This really isn't necessary, but let's keep failure/success cases
        // consistent
        //
        m_timeout = NORMAL_TIMEOUT;
        EnableNextButton(TRUE);

        success = TRUE;
    }
    else {
        FmtMessageBox(m_hDlg,
                      MB_ICONEXCLAMATION | MB_OK,
                      FALSE,
                      ID_DSP_TXT_TEST_MODE,
                      ID_DSP_TXT_TEST_FAILED);

        DeskLogError(LogSevInformation,
                     IDS_SETUPLOG_MSG_092,
                     lpdm->dmPelsWidth, lpdm->dmPelsHeight, 
                     lpdm->dmBitsPerPel,
                     lpdm->dmDisplayFrequency);
    
        EnableNextButton(FALSE);
        m_timeout = SLOW_TIMEOUT;

        success = FALSE;
    }
    
    LocalFree(lpdm);
    return success;
}

BOOL
CDisplaySetup::OnHScroll(HWND hwndTB, DWORD iCode, DWORD iPos)
{
    BOOL fRealize = TRUE;
    int iRes = m_iResolution, maxRes;

    maxRes = (int)SendMessage(hwndTB, TBM_GETRANGEMAX, TRUE, 0);
    TraceMsg(TF_SETUP, "HandleHScroll: MaxRange = %d", maxRes);

    switch(iCode ) {
        case TB_LINEUP:
        case TB_PAGEUP:
            if (iRes != 0)
                iRes--;
            break;

        case TB_LINEDOWN:
        case TB_PAGEDOWN:
            if (++(iRes) >= maxRes) {
                iRes = maxRes;
            }
            break;

        case TB_BOTTOM:
            iRes = maxRes;
            break;

        case TB_TOP:
            iRes = 0;
            break;

        case TB_THUMBTRACK:
        case TB_THUMBPOSITION:
            iRes = iPos;
            break;

        default:
            return FALSE;
    }

    m_pds->SetCurResolution(m_ResolutionList + iRes);
    UpdateUI();

    return TRUE;
}


BOOL
CDisplaySetup::OnCommand(IN DWORD NotifyCode,
                         IN DWORD ControlId,
                         IN HWND hwndControl)
{
    switch (ControlId) {

    case IDC_COLORBOX:
        if (NotifyCode == CBN_SELCHANGE) {

            int selColor = ComboBox_GetCurSel(GetDlgItem(m_hDlg, IDC_COLORBOX));

            if (selColor != CB_ERR) {
                m_pds->SetCurColor((int) *(m_ColorList + selColor));
                UpdateUI();
            }
        }
        return TRUE;

    case IDC_REFRESH_RATE:
        if (NotifyCode == CBN_SELCHANGE) {

            int selFreq =
                ComboBox_GetCurSel(GetDlgItem(m_hDlg, IDC_REFRESH_RATE));
                
            if (selFreq != CB_ERR ) {
                m_pds->SetCurFrequency((int) *(m_FrequencyList + selFreq));
                m_bResolutionChanged = TRUE;
                UpdateUI();
            }
        }
        return TRUE;

    default:
        break;
    }

    return TRUE;
}

BOOL
CDisplaySetup::OnNotify(IN WPARAM ControlId, IN LPNMHDR pnmh)
{
    BOOL prevSetActive = m_bPrevSetActive;

    UNREFERENCED_PARAMETER(ControlId);

    m_bPrevSetActive = TRUE;

    switch (pnmh->code) {
    case PSN_SETACTIVE:

        TraceMsg(TF_SETUP, "Got PSN_SETACTIVE");

        if (m_DisplayIsBad) {
            TraceMsg(TF_SETUP, "display is bad, quitting now");

            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, -1);

            return TRUE;
        }

        //
        // The unattended file takes precedence overall other cases, next is 
        // the previous OS's display values, and last is our default settings
        //
        if (m_bUnattendedPresent) {
            TraceMsg(TF_SETUP, "...using unattended info...");

            DoUnattendedUpgrade();
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, -1);

            return TRUE;
        }
        else if (IsSetupNTUpgrade()) {
            TraceMsg(TF_SETUP, "...upgrading display...");

            DoRegistryUpgrade();
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, -1);

            return TRUE;
        }
#if NO_SETUP_PAGE
        else {
            TraceMsg(TF_SETUP, "...clean install...");

            DoCleanInstall();
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, -1);

            return TRUE;
        }
#else
        if (prevSetActive && !m_pds->bIsModeChanged()) {
            TraceMsg(TF_SETUP, " reenabling buttons...");
            EnableNextButton(TRUE);
        }
#endif
        break;

        // We don't dispaly this page. Just use it to end the Wizard set
        // PropSheet_SetWizButtons(GetParent(hdlg),PSWIZB_FINISH);
        // PropSheet_PressButton(GetParent(hdlg),PSBTN_FINISH);
        // fall through

    case PSN_WIZNEXT:
        TraceMsg(TF_SETUP, "Got PSN_WIZNEXT");

        if (!TestVideoSettings()) {
            SetWindowLongPtr(m_hDlg, DWLP_MSGRESULT, -1);
            return TRUE;
        }
        break;
    }

    return TRUE;
}

BOOL
CDisplaySetup::OnDestroy(void)
{
    return TRUE;
}


LRESULT 
CDisplaySetup::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg ) {

        case WM_COMMAND:
            return OnCommand(HIWORD(wParam),
                             LOWORD(wParam),
                             (HWND)lParam);

        case WM_NOTIFY:
            return OnNotify(wParam,
                            (LPNMHDR) lParam);

        case WM_HSCROLL:
            return OnHScroll((HWND) lParam,
                             LOWORD(wParam),
                             HIWORD(wParam));

        case WM_DESTROY:
            return OnDestroy(); 
    }

    return 0;
}

INT_PTR
CDisplaySetup::DisplayDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    CDisplaySetup *display = (CDisplaySetup *) GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
        case WM_INITDIALOG:
            ASSERT(!display);
            display = (CDisplaySetup*) ((LPPROPSHEETPAGE)lParam)->lParam;
            if (display) {
                SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)display);
                display->OnInitDialog(hDlg, (HWND) wParam, lParam);
                return TRUE;
            }
            break;

        case WM_DESTROY:
            if (display) {
                display->WndProc(uMsg, wParam, lParam);
                SetWindowLongPtr(hDlg, DWLP_USER, NULL);
                // delete display; // deleted in commit
            }
            break;

        default:
            if (display) {
                return display->WndProc(uMsg, wParam, lParam);
            }
            break;
    }

    return FALSE;
}


BOOL 
CDisplaySetup::GetRegistrySettings(LONG *pXRes, 
                                   LONG *pYRes, 
                                   LONG *pBpp, 
                                   LONG *pFrequency)
{
    ASSERT(NULL != m_pds);
    ASSERT((NULL != pXRes) && 
           (NULL != pYRes) && 
           (NULL != pBpp) && 
           (NULL != pFrequency));

    LONG XRes = DEFAULT_XRESOLUTION;
    LONG YRes = DEFAULT_YRESOLUTION;
    LONG Bpp = DEFAULT_BPP;
    LONG Frequency = DEFAULT_VREFRESH;

    BYTE byDevMode[sizeof(DEVMODE) + 0xFFFF];
    LPDEVMODE pDevMode = (LPDEVMODE)byDevMode;
    ZeroMemory(pDevMode, sizeof(DEVMODE) + 0xFFFF);
    pDevMode->dmSize = sizeof(DEVMODE);
    pDevMode->dmDriverExtra = 0xFFFF;

    BOOL bReturn = EnumDisplaySettingsEx(m_DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, pDevMode, 0);
    if(bReturn)
    {
        bReturn = FALSE;

        if((pDevMode->dmFields & DM_PELSWIDTH) && 
           (pDevMode->dmFields & DM_PELSHEIGHT) &&
           (pDevMode->dmPelsWidth != 0) && 
           (pDevMode->dmPelsHeight != 0))
        {
            XRes = pDevMode->dmPelsWidth;
            YRes = pDevMode->dmPelsHeight;

            TraceMsg(TF_SETUP, "Current registry Xresolution:  %d", XRes);
            TraceMsg(TF_SETUP, "Current registry Yresolution:  %d", YRes);

            bReturn = TRUE;
        }

        if((pDevMode->dmFields & DM_BITSPERPEL) &&
           (pDevMode->dmBitsPerPel != 0))
        {
            Bpp = pDevMode->dmBitsPerPel;
            
            TraceMsg(TF_SETUP, "Current registry Bpp:  %d", Bpp);

            bReturn = TRUE;
        }
       
        if((pDevMode->dmFields & DM_DISPLAYFREQUENCY) &&
           (pDevMode->dmDisplayFrequency != 0))
        {
            Frequency = pDevMode->dmDisplayFrequency;
            
            TraceMsg(TF_SETUP, "Current registry Vrefresh:  %d", Frequency);

            bReturn = TRUE;
        }

        if(bReturn)
        {
            *pXRes = XRes;
            *pYRes = YRes;
            *pBpp = Bpp;
            *pFrequency = Frequency;

            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_093,
                         XRes, YRes, Bpp, Frequency);
        }
    }

    return bReturn;
}


BOOL
CDisplaySetup::GetPreferredSettings(LONG *pXRes, 
                                    LONG *pYRes, 
                                    LONG *pBpp, 
                                    LONG *pFrequency)
{
    ASSERT(m_pds != NULL);
    ASSERT((pXRes != NULL) && 
           (pYRes != NULL) && 
           (pBpp != NULL) && 
           (pFrequency != NULL));

    BYTE      byDevMode[sizeof(DEVMODE)];
    LPDEVMODE pDevMode = (LPDEVMODE)byDevMode;
    ZeroMemory(pDevMode, sizeof(DEVMODE));
    pDevMode->dmSize = sizeof(DEVMODE);
    
    LONG XRes = DEFAULT_XRESOLUTION;
    LONG YRes = DEFAULT_YRESOLUTION;
    LONG Bpp = DEFAULT_BPP;
    LONG Frequency = DEFAULT_VREFRESH;

    // Get the preferred mode
    BOOL bReturn = EnumDisplaySettingsEx(m_DisplayDevice.DeviceName, -3, pDevMode, 0);
    if(bReturn)
    {
        bReturn = FALSE;

        if((pDevMode->dmFields & DM_PELSWIDTH) && 
            (pDevMode->dmFields & DM_PELSHEIGHT) &&
            (pDevMode->dmPelsWidth != 0) &&
            (pDevMode->dmPelsHeight != 0))
        {
            XRes = pDevMode->dmPelsWidth;
            YRes = pDevMode->dmPelsHeight;

            TraceMsg(TF_SETUP, "Using display preferred mode Xresolution:  %d", XRes);
            TraceMsg(TF_SETUP, "Using display preferred mode Yresolution:  %d", YRes);

            bReturn = TRUE;
        }
            
        if((pDevMode->dmFields & DM_DISPLAYFREQUENCY) &&
           (pDevMode->dmDisplayFrequency != 0))
        {
            Frequency = pDevMode->dmDisplayFrequency;
            
            TraceMsg(TF_SETUP, "Using display preferred mode Vrefresh:  %d", Frequency);

            bReturn = TRUE;
        }

        if(bReturn)
        {
            *pXRes = XRes;
            *pYRes = YRes;
            *pBpp = Bpp;
            *pFrequency = Frequency;

            DeskLogError(LogSevInformation,
                         IDS_SETUPLOG_MSG_094,
                         XRes, YRes, Bpp, Frequency);
        }
    }

    return bReturn;
}


void 
CDisplaySetup::GetDefaultSettings(LONG *pXRes, 
                                  LONG *pYRes, 
                                  LONG *pBpp, 
                                  LONG *pFrequency)
{
    ASSERT((pXRes != NULL) && 
           (pYRes != NULL) && 
           (pBpp != NULL) && 
           (pFrequency != NULL));

    *pXRes = DEFAULT_XRESOLUTION;
    *pYRes = DEFAULT_YRESOLUTION;
    *pBpp = DEFAULT_BPP;
    *pFrequency = DEFAULT_VREFRESH;

    TraceMsg(TF_SETUP, "Using default settings");
    DeskLogError(LogSevInformation,
                 IDS_SETUPLOG_MSG_095,
                 *pXRes, *pYRes, *pBpp, *pFrequency);
}


/****************************************************************************

    FUNCTION: MakeRect

    PURPOSE:  Fill in RECT structure given contents.

****************************************************************************/
VOID MakeRect( PRECT pRect, INT xmin, INT ymin, INT xmax, INT ymax )
{
    pRect->left= xmin;
    pRect->right= xmax;
    pRect->bottom= ymin;
    pRect->top= ymax;
}

// type constants for DrawArrow

#define AW_TOP      1   // top
#define AW_BOTTOM   2   // bottom
#define AW_LEFT     3   // left
#define AW_RIGHT    4   // right

/****************************************************************************

    FUNCTION: DrawArrow

    PURPOSE:  Draw one arrow in a given color.

****************************************************************************/
static
VOID DrawArrow( HDC hDC, INT type, INT xPos, INT yPos, COLORREF crPenColor )
{
    INT shaftlen=30;         // length of arrow shaft
    INT headlen=15;          // height or width of arrow head (not length)
    HPEN hPen, hPrevPen = NULL;   // pens
    INT x,y;
    INT xdir, ydir;          // directions of x and y (1,-1)

    hPen= CreatePen( PS_SOLID, 1, crPenColor );
    if( hPen )
        hPrevPen= (HPEN) SelectObject( hDC, hPen );

    MoveToEx( hDC, xPos, yPos, NULL );

    xdir= ydir= 1;   // defaults
    switch( type )
    {
        case AW_BOTTOM:
            ydir= -1;
        case AW_TOP:
            LineTo(hDC, xPos, yPos+ydir*shaftlen);

            for( x=0; x<3; x++ )
            {
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos-(headlen-x), yPos+ydir*headlen );
                MoveToEx( hDC, xPos,             yPos+ydir*x, NULL );
                LineTo(   hDC, xPos+(headlen-x), yPos+ydir*headlen );
            }
            break;

        case AW_RIGHT:
            xdir= -1;
        case AW_LEFT:
            LineTo( hDC, xPos + xdir*shaftlen, yPos );

            for( y=0; y<3; y++ )
            {
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos+(headlen-y));
                MoveToEx( hDC, xPos + xdir*y, yPos, NULL );
                LineTo(   hDC, xPos + xdir*headlen, yPos-(headlen-y));
            }
            break;
    }

    if( hPrevPen )
        SelectObject( hDC, hPrevPen );

    if (hPen)
        DeleteObject(hPen);

}

/****************************************************************************

    FUNCTION: LabelRect

    PURPOSE:  Label a rectangle with centered text given resource ID.

****************************************************************************/

static
VOID LabelRect(HDC hDC, PRECT pRect, UINT idString )
{
    UINT iStatus;
    INT xStart, yStart;
    SIZE size;              // for size of string
    TCHAR szMsg[CCH_MAX_STRING];

    if (idString == 0)     // make it easy to ignore call
        return;

    SetBkMode(hDC, OPAQUE);
    SetBkColor(hDC, RGB(0,0,0));
    SetTextColor(hDC, RGB(255,255,255));

    // center
    xStart = (pRect->left + pRect->right) / 2;
    yStart = (pRect->top + pRect->bottom) / 2;

    iStatus = LoadString(hInstance, idString, szMsg, sizeof(szMsg));
    if (!iStatus) {
        return;      // can't find string - print nothing
    }

    GetTextExtentPoint32(hDC, szMsg, lstrlen(szMsg), &size);
    TextOut(hDC, xStart-size.cx/2, yStart-size.cy/2, szMsg, lstrlen(szMsg));
}

/****************************************************************************

    FUNCTION: PaintRect

    PURPOSE:  Color in a rectangle and label it.

****************************************************************************/
static
VOID PaintRect(
HDC hDC,         // DC to paint
INT lowx,        // coordinates describing rectangle to fill
INT lowy,        //
INT highx,       //
INT highy,       //
COLORREF rgb,    // color to fill in rectangle with
UINT idString )  // resource ID to use to label or 0 is none
{
    RECT rct;
    HBRUSH hBrush;

    MakeRect(&rct, lowx, lowy, highx, highy);

    hBrush = CreateSolidBrush(rgb);
    FillRect(hDC, &rct, hBrush);
    DeleteObject(hBrush);

    LabelRect(hDC, &rct, idString);
}

/****************************************************************************

    FUNCTION: DrawArrows

    PURPOSE:  Draw all the arrows showing edges of resolution.

****************************************************************************/
VOID DrawArrows( HDC hDC, INT xRes, INT yRes )
{
    INT dx,dy;
    INT x,y;
    COLORREF color= RGB(0,0,0);    // color of arrow

    dx= xRes/8;
    dy= yRes/8;

    for (x = 0; x < xRes; x += dx) {
        DrawArrow(hDC, AW_TOP,    dx/2+x,   0,      color);
        DrawArrow(hDC, AW_BOTTOM, dx/2+x,   yRes-1, color);
    }

    for (y = 0; y < yRes; y += dy) {
        DrawArrow(hDC, AW_LEFT,       0, dy/2+y,   color);
        DrawArrow(hDC, AW_RIGHT, xRes-1, dy/2+y,   color);
    }
}
/****************************************************************************

    FUNCTION: LabelResolution

    PURPOSE:  Labels the resolution in a form a user may understand.
              bugbug: We could label vertically too.

****************************************************************************/

VOID LabelResolution( HDC hDC, INT xmin, INT ymin, INT xmax, INT ymax )
{
   TCHAR szRes[120];    // text for resolution
   TCHAR szFmt[CCH_MAX_STRING];    // format string
   SIZE  size;
   INT iStatus;

   iStatus = LoadString(hInstance, ID_DSP_TXT_XBYY /* remove IDS_RESOLUTION_FMT */, szFmt, sizeof(szFmt) );
   if (!iStatus || iStatus == sizeof(szFmt)) {
       lstrcpy(szFmt,TEXT("%d x %d"));   // make sure we get something
   }

   wsprintf(szRes, szFmt, xmax, ymax);

   SetBkMode(hDC, TRANSPARENT);
   SetTextColor(hDC, RGB(0,0,0));

   GetTextExtentPoint32(hDC, szRes, lstrlen(szRes), &size);

   // Text near bottom of screen ~10 pixels from bottom
   TextOut(hDC, xmax/2 - size.cx/2, ymax - 10-size.cy, szRes, lstrlen(szRes));
}

/****************************************************************************

    FUNCTION: DrawBmp

    PURPOSE:  Show off a fancy screen so the user has some idea
              of what will be seen given this resolution, colour
              depth and vertical refresh rate.  Note that we do not
              try to simulate the font sizes.

****************************************************************************/
void
DrawBmp(
    HDC hDC
    )
{
    INT    nBpp;          // bits per pixel
    INT    nWidth;        // width of screen in pixels
    INT    nHeight;       // height of screen in pixels
    INT    xUsed,yUsed;   // amount of x and y to use for dense bitmap
    INT    dx,dy;         // delta x and y for color bars
    RECT   rct;           // rectangle for passing bounds
    HFONT  hPrevFont=0;   // previous font in DC
    HFONT  hNewFont;      // new font if possible
    HPEN   hPrevPen;      // previous pen handle
    INT    x,y,i;
    INT    off;           // offset in dx units

    hNewFont = (HFONT)NULL;

    if (hNewFont)                              // if no font, use old
        hPrevFont= (HFONT) SelectObject(hDC, hNewFont);

    // get surface information
    nBpp= GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES);
    nWidth= GetDeviceCaps(hDC, HORZRES);
    nHeight= GetDeviceCaps(hDC, VERTRES);

    // background for everything is yellow.
    PaintRect(hDC, 0, 0, nWidth, nHeight, RGB(255,255,0), 0);
    LabelResolution( hDC, 0,0,nWidth, nHeight );

    // Background for various resolutions
    // biggest ones first
    for(i = 0; ResTab[i].xRes !=0; i++) {

        // Only draw if it will show
        //if ((nWidth >= ResTab[i].xRes) | ( nHeight>=ResTab[i].yRes ) )
        if ((nWidth >= ResTab[i].xRes) || (nHeight >= ResTab[i].yRes)) {
           PaintRect(hDC, 0, 0, ResTab[i].xRes, ResTab[i].yRes, ResTab[i].crColor, 0);
           LabelResolution(hDC, 0, 0, ResTab[i].xRes, ResTab[i].yRes);
        }
    }

    // color bars - only in standard vga area

    xUsed= min(nWidth, 640);    // only use vga width
    yUsed= min(nHeight, 480);   // could be 400 on some boards
    dx = xUsed / 2;
    dy = yUsed / 6;

    PaintRect(hDC, 0,    0, dx, dy*1,  RGB(255,0,0),   IDS_COLOR_RED);
    PaintRect(hDC, 0, dy*1, dx, dy*2,  RGB(0,255,0),   IDS_COLOR_GREEN);
    PaintRect(hDC, 0, dy*2, dx, dy*3,  RGB(0,0,255),   IDS_COLOR_BLUE);
    PaintRect(hDC, 0, dy*3, dx, dy*4,  RGB(255,255,0), IDS_COLOR_YELLOW);
    PaintRect(hDC, 0, dy*4, dx, dy*5,  RGB(255,0,255), IDS_COLOR_MAGENTA);
    PaintRect(hDC, 0, dy*5, dx, yUsed, RGB(0,255,255), IDS_COLOR_CYAN);

    // gradations of colors for true color detection
    for (x = dx; x < xUsed; x++) {
        int level;

        level = 255 - (256 * (x-dx)) / dx;
        PaintRect(hDC, x, dy*0, x+1,  dy*1, RGB(level,0,0 ), 0);
        PaintRect(hDC, x, dy*1, x+1,  dy*2, RGB(0,level,0 ), 0);
        PaintRect(hDC, x, dy*2, x+1,  dy*3, RGB(0,0,level ), 0);
        PaintRect(hDC, x, dy*5, x+1,  dy*6, RGB(level,level,level), 0);
    }

    MakeRect(&rct, dx, 0, dx * 2, dy * 1);
    LabelRect(hDC, &rct, IDS_RED_SHADES);
    MakeRect(&rct, dx, dy, dx * 2, dy * 2);
    LabelRect(hDC, &rct, IDS_GREEN_SHADES);
    MakeRect(&rct, dx, 2 * dy, dx * 2, dy * 3);
    LabelRect(hDC, &rct, IDS_BLUE_SHADES);
    MakeRect(&rct, dx, 5 * dy, dx * 2, dy * 6);
    LabelRect(hDC, &rct, IDS_GRAY_SHADES);

    // horizontal lines for interlace detection
    off = 3;
    PaintRect(hDC, dx, dy*off, xUsed, dy * (off+1), RGB(255,255,255), 0); // white
    hPrevPen = (HPEN) SelectObject(hDC, GetStockObject(BLACK_PEN));

    for (y = dy * off; y < dy * (off+1); y = y+2) {
        MoveToEx(hDC, dx,     y, NULL);
        LineTo(  hDC, dx * 2, y);
    }

    SelectObject(hDC, hPrevPen);
    MakeRect(&rct, dx, dy * off, dx * 2, dy * (off+1));
    LabelRect(hDC, &rct, IDS_PATTERN_HORZ);

    // vertical lines for bad dac detection
    off = 4;
    PaintRect(hDC, dx, dy * off, xUsed,dy * (off+1), RGB(255,255,255), 0); // white
    hPrevPen= (HPEN) SelectObject(hDC, GetStockObject(BLACK_PEN));

    for (x = dx; x < xUsed; x = x+2) {
        MoveToEx(hDC, x, dy * off, NULL);
        LineTo(  hDC, x, dy * (off+1));
    }

    SelectObject(hDC, hPrevPen);
    MakeRect(&rct, dx, dy * off, dx * 2, dy * (off+1));
    LabelRect(hDC, &rct, IDS_PATTERN_VERT);

    DrawArrows(hDC, nWidth, nHeight);

    LabelResolution(hDC, 0, 0, xUsed, yUsed);

    // delete created font if one was created
    if (hPrevFont) {
        hPrevFont = (HFONT) SelectObject(hDC, hPrevFont);
        DeleteObject(hPrevFont);
    }
}

DWORD WINAPI
ApplyNowThd(
    LPVOID lpThreadParameter
    )
{
    PNEW_DESKTOP_PARAM lpDesktopParam = (PNEW_DESKTOP_PARAM) lpThreadParameter;
    HDESK hdsk = NULL;
    HDESK hdskDefault = NULL;
    BOOL bTest = FALSE;
    HDC hdc;


    //
    // HACK:
    // We need to make a USER call before calling the desktop stuff so we can
    // sure our threads internal data structure are associated with the default
    // desktop.
    // Otherwise USER has problems closing the desktop with our thread on it.
    //
    GetSystemMetrics(SM_CXSCREEN);

    //
    // Create the desktop
    //
    hdskDefault = GetThreadDesktop(GetCurrentThreadId());

    if (hdskDefault != NULL) {
        hdsk = CreateDesktop(TEXT("Display.Cpl Desktop"),
                             lpDesktopParam->pwszDevice,
                             lpDesktopParam->lpdevmode,
                             0,
                             MAXIMUM_ALLOWED,
                             NULL);

        if (hdsk != NULL) {
            //
            // use the desktop for this thread
            //
            if (SetThreadDesktop(hdsk)) {
                hdc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
                if (hdc) {
                    DrawBmp(hdc);
                    DeleteDC(hdc);
                    bTest = TRUE;
                }

                //
                // Sleep for some seconds so you have time to look at the screen.
                //
                Sleep(lpDesktopParam->dwTimeout);
            }
        }

        //
        // Reset the thread to the right desktop
        //
        SetThreadDesktop(hdskDefault);
        SwitchDesktop(hdskDefault);

        //
        // Can only close the desktop after we have switched to the new one.
        //
        if (hdsk != NULL) {
            CloseDesktop(hdsk);
        }
    }

    ExitThread((DWORD) bTest);
    return 0;
}

#endif // WINNT
