#pragma warning( disable: 4103)
#include "mmcpl.h"
#include <cpl.h>
#define NOSTATUSBAR
#include <commctrl.h>
#include <prsht.h>
#include <regstr.h>
#include <infstr.h>
#include <devguid.h>

#include "draw.h"
#include "utils.h"
#include "drivers.h"
#include "sulib.h"
#include "medhelp.h"
#include <mmdet.h>
#include <tchar.h>

#define GetString(_psz,_id) LoadString(myInstance,(_id),(_psz),sizeof((_psz))/sizeof(TCHAR))

// Global info struct. One instance for the whole dialog
typedef struct _OUR_PROP_PARAMS
{
    HDEVINFO            DeviceInfoSet;
    PSP_DEVINFO_DATA    DeviceInfoData;
    HKEY                hkDrv;      // Key to classguid\0000
    HKEY                hkDrivers;  // Key to classguid\0000\Drivers
    BOOL                bClosing;   // Set to TRUE while dialog is closing
    TCHAR szSubClasses[256];         // Subclasses to process
} OUR_PROP_PARAMS, *POUR_PROP_PARAMS;

typedef enum
{
    NodeTypeRoot,
    NodeTypeClass,
    NodeTypeDriver
} NODETYPE;

// Tree node. One per node on tree.
typedef struct _DMTREE_NODE;
typedef BOOL (*PFNCONFIG)     (HWND ParentHwnd, struct _DMTREE_NODE *pTreeNode);
typedef BOOL (*PFNQUERYCONFIG)(HWND ParentHwnd, struct _DMTREE_NODE *pTreeNode);
typedef struct _DMTREE_NODE
{
    NODETYPE NodeType;              // Type of node
    PFNCONFIG pfnConfig;            // Ptr to config function
    PFNQUERYCONFIG pfnQueryConfig;  // Ptr to query config function
    int QueryConfigInfo;            // Data for config function
    TCHAR szDescription[MAXSTR];     // Node description
    TCHAR szDriver[MAXSTR];          // Driver name of this node
    WCHAR wszDriver[MAXSTR];        // Wide char driver name
    TCHAR szAlias[MAXSTR];          // Alias
    WCHAR wszAlias[MAXSTR];         // Wide char alias
    DriverClass dc;                 // Legacy-style driver class, if available
    HTREEITEM hti;                  // For use with MIDI prop sheet callback
} DMTREE_NODE, *PDMTREE_NODE;

INT_PTR APIENTRY DmAdvPropPageDlgProc(IN HWND   hDlg,
                                      IN UINT   uMessage,
                                      IN WPARAM wParam,
                                      IN LPARAM lParam
                                     );

UINT CALLBACK DmAdvPropPageDlgCallback(HWND hwnd,
                                       UINT uMsg,
                                       LPPROPSHEETPAGE ppsp
                                      );

BOOL DmAdvPropPage_OnCommand(
                            HWND ParentHwnd,
                            int  ControlId,
                            HWND ControlHwnd,
                            UINT NotifyCode
                            );

BOOL DmAdvPropPage_OnContextMenu(
                                HWND HwndControl,
                                WORD Xpos,
                                WORD Ypos
                                );

BOOL DmAdvPropPage_OnHelp(
                         HWND       ParentHwnd,
                         LPHELPINFO HelpInfo
                         );

BOOL DmAdvPropPage_OnInitDialog(
                               HWND    ParentHwnd,
                               HWND    FocusHwnd,
                               LPARAM  Lparam
                               );

BOOL DmAdvPropPage_OnNotify(
                           HWND    ParentHwnd,
                           LPNMHDR NmHdr
                           );

void DmAdvPropPage_OnPropertiesClicked(
                                      HWND             ParentHwnd,
                                      POUR_PROP_PARAMS Params
                                      );


BOOL DmOverrideResourcesPage(LPVOID        Info,
                             LPFNADDPROPSHEETPAGE AddFunc,
                             LPARAM               Lparam,
                             POUR_PROP_PARAMS     Params
                            );

BOOL AddCDROMPropertyPage( HDEVINFO             hDeviceInfoSet,
                           PSP_DEVINFO_DATA     pDeviceInfoData,
                           LPFNADDPROPSHEETPAGE AddFunc,
                           LPARAM               Lparam
                          );

BOOL AddSpecialPropertyPage( DWORD                SpecialDriverType,
                             LPFNADDPROPSHEETPAGE AddFunc,
                             LPARAM               Lparam
                            );

BOOL DmInitDeviceTree(HWND hwndTree, POUR_PROP_PARAMS Params);

BOOL DmAdvPropPage_OnDestroy(
                            HWND    ParentHwnd,
                            LPNMHDR NmHdr
                            );

void DoProperties(HWND ParentHwnd, HWND hWndI, HTREEITEM htiCur);

BOOL QueryConfigDriver(HWND ParentHwnd, PDMTREE_NODE pTreeNode)
{
    HANDLE       hDriver;

    if (pTreeNode->NodeType!=NodeTypeDriver)
    {
        return FALSE;
    }

    if (pTreeNode->QueryConfigInfo==0)  // if 0, the we haven't checked yet
    {
        INT_PTR IsConfigurable;

        //  open the driver
        hDriver = OpenDriver(pTreeNode->wszDriver, NULL, 0L);
        if (!hDriver)
        {
            return FALSE;
        }

        // Send the DRV_CONFIGURE message to the driver
        IsConfigurable = SendDriverMessage(hDriver,
                                           DRV_QUERYCONFIGURE,
                                           0L,
                                           0L);

        CloseDriver(hDriver, 0L, 0L);

        // 1->Is configurable, -1->Not configurable
        pTreeNode->QueryConfigInfo = IsConfigurable ? 1 : -1;
    }

    return (pTreeNode->QueryConfigInfo>0);
}

BOOL PNPDriverToIResource(PDMTREE_NODE pTreeNode, IRESOURCE* pir)
{
    IDRIVER tempIDriver;

    if ((pir->pcn = (PCLASSNODE)LocalAlloc (LPTR, sizeof(CLASSNODE))) == NULL)
    {
        return FALSE;
    }

    if (!DriverClassToClassNode(pir->pcn, pTreeNode->dc))
    {
        LocalFree ((HANDLE)pir->pcn);
        return FALSE;
    }

    pir->iNode = 2;   // 1=class, 2=device, 3=acm, 4=instmt

    lstrcpy (pir->szFriendlyName, pTreeNode->szDescription);
    lstrcpy (pir->szDesc,         pTreeNode->szDescription);
    lstrcpy (pir->szFile,         pTreeNode->szDriver);
    lstrcpy (pir->szDrvEntry,     pTreeNode->szAlias);
    lstrcpy (pir->szClass,        pir->pcn->szClass);

    pir->fQueryable = (short)QueryConfigDriver(NULL, pTreeNode);
    pir->iClassID = (short)DriverClassToOldClassID(pTreeNode->dc);
    pir->szParam[0] = 0;
    pir->dnDevNode = 0;
    pir->hDriver = NULL;

    // Find fStatus, which despite its name is really a series of
    // flags--in Win95 it's composed of DEV_* flags (from the old
    // mmcpl.h), but those are tied with PNP.  Here, we use the
    // dwStatus* flags:
    //
    ZeroMemory(&tempIDriver,sizeof(IDRIVER));

    lstrcpy(tempIDriver.wszAlias,pTreeNode->wszAlias);
    lstrcpy(tempIDriver.szAlias,pTreeNode->szAlias);
    lstrcpy(tempIDriver.wszFile,pTreeNode->wszDriver);
    lstrcpy(tempIDriver.szFile,pTreeNode->szDriver);
    lstrcpy(tempIDriver.szDesc,pTreeNode->szDescription);
    lstrcpy(tempIDriver.szSection,wcsstr(pTreeNode->szDescription, TEXT("MCI")) ? szMCI : szDrivers);
    lstrcpy(tempIDriver.wszSection,wcsstr(pTreeNode->szDescription, TEXT("MCI")) ? szMCI : szDrivers);

    pir->fStatus = (int)GetDriverStatus (&tempIDriver);

    return TRUE;
}

BOOL ConfigDriver(HWND ParentHwnd, PDMTREE_NODE pTreeNode)
{
    //need to pop up the legacy properties dialog
    IRESOURCE ir;
    DEVTREENODE dtn;
    TCHAR        szTab[ cchRESOURCE ];

    if ((pTreeNode->NodeType == NodeTypeDriver) && (pTreeNode->dc != dcINVALID))
    {
        if (PNPDriverToIResource(pTreeNode, &ir))
        {
            GetString (szTab, IDS_GENERAL);

            dtn.lParam = (LPARAM)&ir;
            dtn.hwndTree = ParentHwnd;

            //must call this function twice to fill in the array of PIDRIVERs in drivers.c
            //otherwise, many of the "settings" calls won't work
            InitInstalled (GetParent (ParentHwnd), szDrivers);
            InitInstalled (GetParent (ParentHwnd), szMCI);

            switch (pTreeNode->dc)
            {
                case dcMIDI :
                   ShowWithMidiDevPropSheet (szTab,
                                             DevPropDlg,
                                             DLG_DEV_PROP,
                                             ParentHwnd,
                                             pTreeNode->szDescription,
                                             pTreeNode->hti,
                                             (LPARAM)&dtn,
                                             (LPARAM)&ir,
                                             (LPARAM)ParentHwnd);
                break;

                case dcWAVE :
                    ShowPropSheet (szTab,
                              DevPropDlg,
                              DLG_WAVDEV_PROP,
                              ParentHwnd,
                              pTreeNode->szDescription,
                              (LPARAM)&dtn);
                break;

                default:
                   ShowPropSheet (szTab,
                                  DevPropDlg,
                                  DLG_DEV_PROP,
                                  ParentHwnd,
                                  pTreeNode->szDescription,
                                  (LPARAM)&dtn);
                break;
            } //end switch

            FreeIResource (&ir);
        }
    }

    return (FALSE);
}

const static DWORD aDMPropHelpIds[] = {  // Context Help IDs
    IDC_ADV_TREE,    IDH_GENERIC_DEVICES,
    ID_ADV_PROP,     IDH_ADV_PROPERTIES,
    0, 0
};

//******************************************************************************
//* Subtype code
//******************************************************************************
//
// Subtype info. Array of one per device class subtype
typedef struct _SUBTYPE_INFO
{
    TCHAR *szClass;
    DWORD DescId;
    DWORD IconId;
    PFNCONFIG pfnConfig;
    PFNQUERYCONFIG pfnQueryConfig;
    DriverClass dc;
    TCHAR  szDescription[64];
    DWORD IconIndex;
} SUBTYPE_INFO;

static SUBTYPE_INFO SubtypeInfo[] =
{
    { TEXT(""),            IDS_MM_HEADER,       IDI_MMICON,   ConfigDriver, QueryConfigDriver, dcOTHER},
    { TEXT("waveaudio"),   IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("wavemap"),     IDS_WAVE_HEADER,     IDI_WAVE,     ConfigDriver, QueryConfigDriver, dcWAVE},
    { TEXT("wave"),        IDS_WAVE_HEADER,     IDI_WAVE,     ConfigDriver, QueryConfigDriver, dcWAVE},
    { TEXT("vids"),        IDS_ICM_HEADER,      IDI_ICM,      ConfigDriver, QueryConfigDriver, dcVCODEC},
    { TEXT("vidc"),        IDS_ICM_HEADER,      IDI_ICM,      ConfigDriver, QueryConfigDriver, dcVCODEC},
    { TEXT("sequencer"),   IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("msvideo"),     IDS_VIDCAP_HEADER,   IDI_VIDEO,    ConfigDriver, QueryConfigDriver, dcVIDCAP},
    { TEXT("msacm"),       IDS_ACM_HEADER,      IDI_ACM,      ConfigDriver, QueryConfigDriver, dcACODEC},
    { TEXT("mpegvideo"),   IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("mixer"),       IDS_MIXER_HEADER,    IDI_MIXER,    ConfigDriver, QueryConfigDriver, dcMIXER},
    { TEXT("midimapper"),  IDS_MIDI_HEADER,     IDI_MIDI,     ConfigDriver, QueryConfigDriver, dcMIDI},
    { TEXT("midi"),        IDS_MIDI_HEADER,     IDI_MIDI,     ConfigDriver, QueryConfigDriver, dcMIDI},
    { TEXT("mci"),         IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("icm"),         IDS_ICM_HEADER,      IDI_ICM,      ConfigDriver, QueryConfigDriver, dcVCODEC},
    { TEXT("cdaudio"),     IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("avivideo"),    IDS_MCI_HEADER,      IDI_MCI,      ConfigDriver, QueryConfigDriver, dcMCI},
    { TEXT("aux"),         IDS_AUX_HEADER,      IDI_AUX,      ConfigDriver, QueryConfigDriver, dcAUX},
    { TEXT("acm"),         IDS_ACM_HEADER,      IDI_ACM,      ConfigDriver, QueryConfigDriver, dcACODEC},
    { TEXT("joy"),         IDS_JOYSTICK_HEADER, IDI_JOYSTICK, ConfigDriver, QueryConfigDriver, dcJOY}
};

#define SUBTYPE_INFO_SIZE (sizeof(SubtypeInfo)/sizeof(SUBTYPE_INFO))

BOOL LoadSubtypeInfo(HWND hwndTree)
{
    UINT i;
    int cxMiniIcon;
    int cyMiniIcon;

    HIMAGELIST hImagelist;

    // Create the image list
    cxMiniIcon = (int)GetSystemMetrics(SM_CXSMICON);
    cyMiniIcon = (int)GetSystemMetrics(SM_CYSMICON);
    hImagelist = ImageList_Create(cxMiniIcon, cyMiniIcon, TRUE, SUBTYPE_INFO_SIZE, 4);
    if (!hImagelist)
        return FALSE;

    for (i=0;i<SUBTYPE_INFO_SIZE;i++)
    {
        HICON hIcon;

        // Load the description
        LoadString(ghInstance, SubtypeInfo[i].DescId, SubtypeInfo[i].szDescription, 64);

        // Load the image into the image list
        hIcon = LoadImage (ghInstance,
                           MAKEINTRESOURCE( SubtypeInfo[i].IconId ),
                           IMAGE_ICON,
                           cxMiniIcon,
                           cyMiniIcon,
                           LR_DEFAULTCOLOR);

        SubtypeInfo[i].IconIndex = ImageList_AddIcon(hImagelist, hIcon);
        DestroyIcon(hIcon);
    }

    // Clean out and initialize tree control
    TreeView_SetImageList(hwndTree, hImagelist, TVSIL_NORMAL);

    return TRUE;
}

SUBTYPE_INFO *GetSubtypeInfo(TCHAR *pszClass)
{
    UINT iClass;
    if (pszClass)
    {
        for (iClass=0;iClass<SUBTYPE_INFO_SIZE;iClass++)
        {
            if (!lstrcmpi(pszClass,SubtypeInfo[iClass].szClass))
                return &SubtypeInfo[iClass];
        }
    }

    return &SubtypeInfo[0];
}

//******************************************************************************

/*

Routine Description: MediaPropPageProvider

    Entry-point for adding additional device manager property
    sheet pages.  Registry specifies this routine under
    HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Class\{4D36E96C-E325-11CE-BFC1-08002BE10318}
    EnumPropPage32="mmsys.cpl,thisproc"

    This entry-point gets called only when the DeviceManager asks for additional property pages.

Arguments:

    Info  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    AddFunc - function ptr to call to add sheet.
    Lparam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

*/
BOOL APIENTRY MediaPropPageProvider(LPVOID               Info,
                                    LPFNADDPROPSHEETPAGE AddFunc,
                                    LPARAM               Lparam
                                   )
{
    PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
    PROPSHEETPAGE             psp;
    HPROPSHEETPAGE            hpsp;
    POUR_PROP_PARAMS          Params;
    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    DWORD SpecialDriverType;

    HKEY hkDrv;
    HKEY hkDrivers;
    DWORD cbLen;

    pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;

    if (pprPropPageRequest->PageRequested != SPPSR_ENUM_ADV_DEVICE_PROPERTIES)
    {
        return TRUE;
    }

    DeviceInfoSet  = pprPropPageRequest->DeviceInfoSet;
    DeviceInfoData = pprPropPageRequest->DeviceInfoData;

    // This API is called for both devices and the class as a whole
    // (when someone right-clicks on the class and chooses properties).
    // In the class case the DeviceInfoData field of the propPageRequest structure is NULL.
    // We don't do anything in that case, so just return.
    if (!DeviceInfoData)
    {
        return TRUE;
    }

    SpecialDriverType = IsSpecialDriver(DeviceInfoSet, DeviceInfoData);
    if (SpecialDriverType)
    {
        SP_DEVINSTALL_PARAMS DeviceInstallParams;

        DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
        SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
        DeviceInstallParams.Flags |= DI_RESOURCEPAGE_ADDED | DI_DRIVERPAGE_ADDED; // | DI_GENERALPAGE_ADDED;
        SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
        AddSpecialPropertyPage(SpecialDriverType, AddFunc, Lparam);
        return TRUE;
    }

    if (AddCDROMPropertyPage(DeviceInfoSet,DeviceInfoData, AddFunc, Lparam))
    {
        return TRUE;
    }

    // Open device reg key to see if this is a WDM driver
    hkDrv = SetupDiOpenDevRegKey(DeviceInfoSet,
                                 DeviceInfoData,
                                 DICS_FLAG_GLOBAL,
                                 0,
                                 DIREG_DRV,
                                 KEY_ALL_ACCESS);
    if (!hkDrv)
        return FALSE;

    // Allocate and zero out memory for the struct that will contain page specific data
    Params = (POUR_PROP_PARAMS)LocalAlloc(LPTR, sizeof(OUR_PROP_PARAMS));
    if (!Params)
    {
        RegCloseKey(hkDrv);
        return FALSE;
    }

    // Initialize Params structure
    Params->DeviceInfoSet  = DeviceInfoSet;
    Params->DeviceInfoData = DeviceInfoData;
    Params->hkDrv          = hkDrv;

    // Override Resource page if this is not a WDM (PNP) driver
    DmOverrideResourcesPage(Info, AddFunc, Lparam, Params);

    // Try a couple of things to see if there are actually any drivers under this key
    // and cache the results

    // Try to open up the Drivers subkey
    if (RegOpenKey(Params->hkDrv, TEXT("Drivers"), &hkDrivers))
    {
        RegCloseKey(hkDrv);
        LocalFree(Params);
        return TRUE;
    }

    // Try to read the SubClasses key to determine which subclasses to process
    cbLen=sizeof(Params->szSubClasses);
    if (RegQueryValueEx(hkDrivers, TEXT("Subclasses"), NULL, NULL, (LPBYTE)Params->szSubClasses, &cbLen))
    {
        RegCloseKey(hkDrv);
        RegCloseKey(hkDrivers);
        LocalFree(Params);
        return TRUE;
    }

    Params->hkDrivers      = hkDrivers;

    // Initialize the property sheet page
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp.hInstance   = ghInstance;
    psp.pszTemplate = MAKEINTRESOURCE(DLG_DM_ADVDLG);
    psp.pfnDlgProc  = DmAdvPropPageDlgProc;     // dlg window proc
    psp.lParam      = (LPARAM) Params;
    psp.pfnCallback = DmAdvPropPageDlgCallback; // control callback of the dlg window proc

    // Create the page & get back a handle
    hpsp = CreatePropertySheetPage(&psp);
    if (!hpsp)
    {
        RegCloseKey(hkDrv);
        LocalFree(Params);
        return FALSE;
    }

    // Add the property page
    if (!(*AddFunc)(hpsp, Lparam))
    {
        DestroyPropertySheetPage(hpsp);
        return FALSE;
    }

    return TRUE;
} /* DmAdvPropPageProvider */

UINT CALLBACK DmAdvPropPageDlgCallback(HWND hwnd,
                                       UINT uMsg,
                                       LPPROPSHEETPAGE ppsp)
{
    POUR_PROP_PARAMS Params;

    switch (uMsg)
    {
    case PSPCB_CREATE:  // This gets called when the page is created
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE: // This gets called when the page is destroyed
        Params = (POUR_PROP_PARAMS) ppsp->lParam;
        RegCloseKey(Params->hkDrv);
        RegCloseKey(Params->hkDrivers);
        LocalFree(Params);  // Free our local params

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

/*++

Routine Description: DmAdvPropPageDlgProc

    The windows control function for the Port Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
INT_PTR APIENTRY DmAdvPropPageDlgProc(IN HWND   hDlg,
                                      IN UINT   uMessage,
                                      IN WPARAM wParam,
                                      IN LPARAM lParam)
{
    switch (uMessage)
    {
    case WM_COMMAND:
        return DmAdvPropPage_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));

    case WM_CONTEXTMENU:
        return DmAdvPropPage_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        return DmAdvPropPage_OnHelp(hDlg, (LPHELPINFO) lParam);

    case WM_INITDIALOG:
        return DmAdvPropPage_OnInitDialog(hDlg, (HWND)wParam, lParam);

    case WM_NOTIFY:
        return DmAdvPropPage_OnNotify(hDlg,  (NMHDR *)lParam);

    case WM_DESTROY:
        return DmAdvPropPage_OnDestroy(hDlg, (NMHDR *)lParam);
    }

    return FALSE;
} /* DmAdvPropPageDlgProc */

BOOL DmAdvPropPage_OnCommand(
                            HWND ParentHwnd,
                            int  ControlId,
                            HWND ControlHwnd,
                            UINT NotifyCode
                            )
{
    POUR_PROP_PARAMS params =
        (POUR_PROP_PARAMS) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    switch (ControlId)
    {
    case ID_ADV_PROP:
        DmAdvPropPage_OnPropertiesClicked(ParentHwnd, params);
        break;
    }

    return FALSE;
}

BOOL DmAdvPropPage_OnContextMenu(
                                HWND HwndControl,
                                WORD Xpos,
                                WORD Ypos
                                )
{
    WinHelp(HwndControl,
            gszWindowsHlp,
            HELP_CONTEXTMENU,
            (ULONG_PTR) aDMPropHelpIds);
    return FALSE;
}

BOOL DmAdvPropPage_OnHelp(
                         HWND       ParentHwnd,
                         LPHELPINFO HelpInfo
                         )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW)
    {
        WinHelp((HWND) HelpInfo->hItemHandle,
                gszWindowsHlp,
                HELP_WM_HELP,
                (ULONG_PTR) aDMPropHelpIds);
    }
    return FALSE;
}

BOOL DmAdvPropPage_OnInitDialog(
                               HWND    ParentHwnd,
                               HWND    FocusHwnd,
                               LPARAM  Lparam
                               )
{
    HWND hwndTree;
    POUR_PROP_PARAMS Params;
    HCURSOR hCursor;
    BOOL bSuccess;

    // on WM_INITDIALOG call, lParam points to the property sheet page.
    //
    // The lParam field in the property sheet page struct is set by the
    // caller. When I created the property sheet, I passed in a pointer
    // to a struct containing information about the device. Save this in
    // the user window long so I can access it on later messages.
    Params = (POUR_PROP_PARAMS) ((LPPROPSHEETPAGE)Lparam)->lParam;
    SetWindowLongPtr(ParentHwnd, DWLP_USER, (ULONG_PTR) Params);

    // Put up the wait cursor
    hCursor = SetCursor(LoadCursor(NULL,IDC_WAIT));

    //create the device tree.
    hwndTree = GetDlgItem(ParentHwnd, IDC_ADV_TREE);

    // Initialize the tree
    bSuccess = DmInitDeviceTree(hwndTree, Params);

    // Enable the adv properties button
    EnableWindow(GetDlgItem(ParentHwnd, ID_ADV_PROP), TRUE);

    // Tear down the wait cursor
    SetCursor(hCursor);

    return bSuccess;
}

BOOL DmAdvPropPage_OnNotify(
                           HWND    ParentHwnd,
                           LPNMHDR NmHdr
                           )
{
    POUR_PROP_PARAMS Params = (POUR_PROP_PARAMS) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    switch (NmHdr->code)
    {
    case PSN_APPLY:    // Sent when the user clicks on Apply OR OK !!
        SetWindowLongPtr(ParentHwnd, DWLP_MSGRESULT, (LONG_PTR)PSNRET_NOERROR);
        return TRUE;

    case TVN_SELCHANGED:
        //Don't bother if we are closing. This helps avoid irritating
        //redraw problems as destroy causes several of these messages to be sent.
        if (!Params->bClosing)
        {
            LPNM_TREEVIEW   lpnmtv;
            TV_ITEM         tvi;
            PDMTREE_NODE    pTreeNode;
            BOOL            fEnablePropButton;
            HWND            hwndProp;

            lpnmtv    = (LPNM_TREEVIEW)NmHdr;
            tvi       = lpnmtv->itemNew;
            pTreeNode = (PDMTREE_NODE)tvi.lParam;
            fEnablePropButton = pTreeNode->pfnQueryConfig(ParentHwnd, pTreeNode);

            //override the enabling for driver entries
            if ((pTreeNode->NodeType == NodeTypeDriver) && (pTreeNode->dc != dcINVALID))
            {
                fEnablePropButton = TRUE;
            }

            // Enable or disable the Properties button depending upon
            // whether this driver can be configured
            hwndProp  = GetDlgItem(ParentHwnd, ID_ADV_PROP);
            EnableWindow(hwndProp, fEnablePropButton);
        }
        break;

    case NM_DBLCLK:
        //show properties on a double-click
        if (NmHdr->idFrom == (DWORD)IDC_ADV_TREE)
        {
            HWND            hwndTree;
            TV_HITTESTINFO  tvht;

            hwndTree = GetDlgItem(ParentHwnd, IDC_ADV_TREE);

            // Find out which tree item the cursor is on and call properties on it
            GetCursorPos(&tvht.pt);
            ScreenToClient(hwndTree, &tvht.pt);
            TreeView_HitTest(hwndTree, &tvht);
            if (tvht.flags & TVHT_ONITEM)
            {
                DoProperties(ParentHwnd, hwndTree, tvht.hItem);
            }
        }
        break;

#if 0 // stolen from Win98, not integrated yet
    case PSN_KILLACTIVE:
        FORWARD_WM_COMMAND(hDlg, IDOK, 0, 0, SendMessage);
        break;

    case PSN_APPLY:
        FORWARD_WM_COMMAND(hDlg, ID_APPLY, 0, 0, SendMessage);
        break;

    case PSN_SETACTIVE:
        FORWARD_WM_COMMAND(hDlg, ID_INIT, 0, 0, SendMessage);
        break;

    case PSN_RESET:
        FORWARD_WM_COMMAND(hDlg, IDCANCEL, 0, 0, SendMessage);
        break;

    case TVN_ITEMEXPANDING:
        {
            TV_ITEM tvi;
            HWND hwndTree = GetDlgItem(hDlg,IDC_ADV_TREE);

            tvi = lpnmtv->itemNew;
            tvi.mask = TVIF_PARAM;
            if (!TreeView_GetItem(hwndTree, &tvi))
                break;

            if (!tvi.lParam || IsBadReadPtr((LPVOID)tvi.lParam, 2))
            {
                DPF("****TVN_ITEMEXPANDING: lParam = 0 || BadReadPtr***\r\n");
                break;
            }
            if (*((short *)(tvi.lParam)) == 1)
            {
                //re-enum ACM codecs on expand because their states could have been programmatically changed.
                PCLASSNODE     pcn = (PCLASSNODE)(tvi.lParam);

                if (lpnmtv->action == TVE_EXPAND && !lstrcmpi(pcn->szClass, ACM))
                {
                    if (gfLoadedACM)
                        ACMNodeChange(hDlg);
                }
            }
            else if (!tvi.lParam && lpnmtv->action == TVE_COLLAPSE)
            {
                //dont let the root collapse.
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
                return TRUE;
            }
            break;
        }


    case TVN_BEGINLABELEDIT:
        //we don't want to allow editing of label unless the user explicitly wants it by
        //clicking context menu item
        if (!gfEditLabel)
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
        return TRUE;

    case TVN_ENDLABELEDIT:
        {
            HWND hwndTree;
            LPSTR pszFriendlyName = ((TV_DISPINFO *) lpnm)->item.pszText;
            TV_ITEM item;
            HTREEITEM hti;
            PIRESOURCE pIResource;
            char szWarn[128];
            char ach[MAXSTR];

            //user has chosen a new friendly name. COnfirm with the user and put it in the
            //registry. ALso unhook KB hook which was used to track Esc and Return
            if (gfnKBHook)
            {
                UnhookWindowsHookEx(gfnKBHook);
                gfnKBHook = NULL;
            }
            if (!pszFriendlyName)
                return FALSE;
            hwndTree = GetDlgItem(hDlg, IDC_ADV_TREE);
            hti = TreeView_GetSelection(hwndTree);
            item.hItem =  hti;
            item.mask = TVIF_PARAM;
            TreeView_GetItem(hwndTree, &item);

            LoadString(ghInstance, IDS_FRIENDLYWARNING, szWarn, sizeof(szWarn));
            wsprintf(ach, szWarn, pszFriendlyName);
            LoadString(ghInstance, IDS_FRIENDLYNAME, szWarn, sizeof(szWarn));
            if (mmse_MessageBox(hDlg, ach, szWarn, MMSE_YESNO) == MMSE_NO)
            {
                SetFocus(hwndTree);
                return FALSE;
            }
            if (*((short *)(item.lParam)) == 2)
            {
                pIResource = (PIRESOURCE)item.lParam;
                lstrcpy(pIResource->szFriendlyName, pszFriendlyName);
                SaveDevFriendlyName(pIResource);
            }
            else
            {
                PINSTRUMENT pInstr = (PINSTRUMENT)item.lParam;
                lstrcpy(pInstr->szFriendlyName, pszFriendlyName);
                SaveInstrFriendlyName(pInstr);
            }
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LPARAM)(LRESULT)TRUE);
            return TRUE;
        }
    case NM_RCLICK:
        //popup context menu.
        TreeContextMenu(hDlg,  GetDlgItem(hDlg, IDC_ADV_TREE));
        return TRUE;

#endif
    default:
        return FALSE;
    }

    return FALSE;
}


void DoProperties(HWND ParentHwnd, HWND hWndI, HTREEITEM htiCur)
{
    TV_ITEM      tvi;
    PDMTREE_NODE pTreeNode;
    BOOL         bRestart;

    // Get item struct attached to selected node
    tvi.mask = TVIF_PARAM;
    tvi.hItem = htiCur;
    if (TreeView_GetItem (hWndI, &tvi))
    {
        // Get my private data structure from item struct
        pTreeNode = (PDMTREE_NODE)tvi.lParam;

        if (pTreeNode->NodeType != NodeTypeDriver)
        {
            if (!pTreeNode->pfnQueryConfig(ParentHwnd, pTreeNode))
            {
                return;
            }
        }

        // Call config and get back whether restart needed
        pTreeNode->hti = htiCur; //this allows us to work with the legacy MIDI setup code
        bRestart = pTreeNode->pfnConfig(ParentHwnd, pTreeNode);

        if (bRestart)
        {
            PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
        }
    }

    return;
}

void DmAdvPropPage_OnPropertiesClicked(
                                      HWND             ParentHwnd,
                                      POUR_PROP_PARAMS Params
                                      )
{
    HWND         hWndI;
    HTREEITEM    htiCur;

    // Get handle to treeview control
    hWndI  = GetDlgItem(ParentHwnd, IDC_ADV_TREE);

    // Get handle to currently selected node
    htiCur = TreeView_GetSelection (hWndI);

    if (htiCur != NULL)
    {
        DoProperties(ParentHwnd, hWndI, htiCur);
    }

    return;
}

INT_PTR APIENTRY DmResourcesPageDlgProc(IN HWND   hDlg,
                                        IN UINT   uMessage,
                                        IN WPARAM wParam,
                                        IN LPARAM lParam)
{
    return FALSE;
} /* DmAdvPropPageDlgProc */



INT_PTR CALLBACK CDDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);


BOOL AddCDROMPropertyPage( HDEVINFO             hDeviceInfoSet,
                           PSP_DEVINFO_DATA     pDeviceInfoData,
                           LPFNADDPROPSHEETPAGE AddFunc,
                           LPARAM               Lparam
                          )
{
    BOOL fHandled = FALSE;

    if (IsEqualGUID(&pDeviceInfoData->ClassGuid,&GUID_DEVCLASS_CDROM))
    {
        PROPSHEETPAGE    psp;
        HPROPSHEETPAGE   hpsp;
        PALLDEVINFO      padi;

        padi = GlobalAllocPtr(GHND, sizeof(ALLDEVINFO));

        padi->hDevInfo = hDeviceInfoSet;
        padi->pDevInfoData = pDeviceInfoData;

        // Add our own page for DLG_DM_LEGACY_RESOURCES
        // Initialize the property sheet page
        psp.dwSize      = sizeof(PROPSHEETPAGE);
        psp.dwFlags     = 0;
        psp.hInstance   = ghInstance;
        psp.pszTemplate = MAKEINTRESOURCE(DM_CDDLG);
        psp.pfnDlgProc  = CDDlg;                       // dlg window proc
        psp.lParam      = (LPARAM) padi;
        psp.pfnCallback = 0;                          // control callback of the dlg window proc

        // Create the page & get back a handle
        hpsp = CreatePropertySheetPage(&psp);
        if (!hpsp)
        {
            fHandled = TRUE;
        }
        else if (!(*AddFunc)(hpsp, Lparam))
        {
            GlobalFreePtr(padi);
            DestroyPropertySheetPage(hpsp);
            fHandled = FALSE;
        }
    }

    return(fHandled);
}

INT_PTR CALLBACK AdvDlg(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);



BOOL AddSpecialPropertyPage( DWORD                SpecialDriverType,
                             LPFNADDPROPSHEETPAGE AddFunc,
                             LPARAM               Lparam
                            )
{
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE   hpsp;

    // Add our own page for DLG_DM_LEGACY_RESOURCES
    // Initialize the property sheet page
    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = 0;
    psp.hInstance   = ghInstance;
    psp.pszTemplate = MAKEINTRESOURCE(DM_ADVDLG);
    psp.pfnDlgProc  = AdvDlg;                       // dlg window proc
    psp.lParam      = (LPARAM)SpecialDriverType;
    psp.pfnCallback = 0;                          // control callback of the dlg window proc

    // Create the page & get back a handle
    hpsp = CreatePropertySheetPage(&psp);
    if (!hpsp)
    {
        return FALSE;
    }

    // Add the property page
    if (!(*AddFunc)(hpsp, Lparam))
    {
        DestroyPropertySheetPage(hpsp);
        return FALSE;
    }

    return TRUE;
}



BOOL DmOverrideResourcesPage(LPVOID        Info,
                             LPFNADDPROPSHEETPAGE AddFunc,
                             LPARAM               Lparam,
                             POUR_PROP_PARAMS     Params
                            )
{
    HKEY             hkDrv;
    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    PROPSHEETPAGE    psp;
    HPROPSHEETPAGE            hpsp;

    TCHAR szDriverType[16];
    DWORD cbLen;

    hkDrv          = Params->hkDrv;
    DeviceInfoSet  = Params->DeviceInfoSet;
    DeviceInfoData = Params->DeviceInfoData;

    // Query value of DriverType field to decide if this is a WDM driver
    cbLen = sizeof(szDriverType);
    if (!RegQueryValueEx(hkDrv, TEXT("DriverType"), NULL, NULL, (LPBYTE)szDriverType, &cbLen))
    {
        if ( lstrcmpi(szDriverType,TEXT("Legacy")) || lstrcmpi(szDriverType,TEXT("PNPISA")) )
        {
            // This is a PNPISA or Legacy device. Override resource page.
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);
            DeviceInstallParams.Flags |= DI_RESOURCEPAGE_ADDED;
            SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams);

            // Add our own page for DLG_DM_LEGACY_RESOURCES
            // Initialize the property sheet page
            psp.dwSize      = sizeof(PROPSHEETPAGE);
            psp.dwFlags     = 0;
            psp.hInstance   = ghInstance;
            psp.pszTemplate = MAKEINTRESOURCE(DLG_DM_LEGACY_RESOURCES);
            psp.pfnDlgProc  = DmResourcesPageDlgProc;     // dlg window proc
            psp.lParam      = (LPARAM)0;
            psp.pfnCallback = 0;                          // control callback of the dlg window proc

            // Create the page & get back a handle
            hpsp = CreatePropertySheetPage(&psp);
            if (!hpsp)
            {
                return FALSE;
            }

            // Add the property page
            if (!(*AddFunc)(hpsp, Lparam))
            {
                DestroyPropertySheetPage(hpsp);
                return FALSE;
            }

        }
    }

    return TRUE;
}

/*
 ***************************************************************
 * BOOL DmInitDeviceTree
 *
 *      This function calls commctrl to create the image list and tree and
 *      the opens the registry, reads each class and loads all devices under
 *      the class by calling ReadNodes. For ACM however it uses ACM
 *      APIs (this enumeration code is in msacmcpl.c)
 ***************************************************************
 */
BOOL DmInitDeviceTree(HWND hwndTree, POUR_PROP_PARAMS Params)
{
    TV_INSERTSTRUCT ti;

    HDEVINFO         DeviceInfoSet;
    PSP_DEVINFO_DATA DeviceInfoData;

    TCHAR *strtok_State;         // strtok state
    TCHAR *pszClass;             // Information about e.g. classguid\0000\Drivers\wave
    HKEY hkClass;

    DWORD idxR3DriverName;      // Information about e.g. classguid\0000\Drivers\wave\foo.drv
    HKEY hkR3DriverName;
    TCHAR szR3DriverName[64];

    DWORD cbLen;

    PDMTREE_NODE pTreeNode;

    SUBTYPE_INFO *pSubtypeInfo;

    HTREEITEM htiRoot;
    HTREEITEM htiClass;
    HTREEITEM htiDriver;

    // Load up all the class descriptions and icons
    LoadSubtypeInfo(hwndTree);

    // Clear out the tree
    SendMessage(hwndTree, WM_SETREDRAW, FALSE, 0L);

    // Allocate my private data structure for this class
    pTreeNode = (PDMTREE_NODE)LocalAlloc(LPTR, sizeof(DMTREE_NODE));
    if (!pTreeNode)
    {
        return FALSE;
    }

    pSubtypeInfo = GetSubtypeInfo(NULL);

    pTreeNode->NodeType = NodeTypeRoot;
    pTreeNode->pfnConfig = pSubtypeInfo->pfnConfig;
    pTreeNode->pfnQueryConfig = pSubtypeInfo->pfnQueryConfig;

    // Insert root entry
    ti.hParent = TVI_ROOT;
    ti.hInsertAfter = TVI_LAST;
    ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    ti.item.iImage = ti.item.iSelectedImage = pSubtypeInfo->IconIndex;
    ti.item.pszText = pSubtypeInfo->szDescription;
    ti.item.lParam = (LPARAM)pTreeNode;
    htiRoot = NULL; //TreeView_InsertItem(hwndTree, &ti);

    // Enumerate all the subclasses
    for (
        pszClass = mystrtok(Params->szSubClasses,NULL,&strtok_State);
        pszClass;
        pszClass = mystrtok(NULL,NULL,&strtok_State)
        )
    {

        // Get an ID for this class
        pSubtypeInfo = GetSubtypeInfo(pszClass);

        // Open up each subclass
        if (RegOpenKey(Params->hkDrivers, pszClass, &hkClass))
        {
            continue;
        }

        // Allocate my private data structure for this class
        pTreeNode = (PDMTREE_NODE)LocalAlloc(LPTR, sizeof(DMTREE_NODE));
        if (!pTreeNode)
        {
            RegCloseKey(hkClass);
            continue;
        }

        pTreeNode->NodeType = NodeTypeClass;
        pTreeNode->pfnConfig = pSubtypeInfo->pfnConfig;
        pTreeNode->pfnQueryConfig = pSubtypeInfo->pfnQueryConfig;

        // Initialize tree insert struct
        ti.hParent = htiRoot;
        ti.hInsertAfter = TVI_LAST;
        ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        ti.item.iImage = ti.item.iSelectedImage = pSubtypeInfo->IconIndex;
        ti.item.pszText = pSubtypeInfo->szDescription;
        ti.item.lParam = (LPARAM)pTreeNode;

        // Insert Class entry into tree
        htiClass = TreeView_InsertItem(hwndTree, &ti);

        // Under each class is a set of driver name subkeys.
        // For each driver (e.g. foo1.drv, foo2.drv, etc.)
        for (idxR3DriverName = 0;
            !RegEnumKey(hkClass, idxR3DriverName, szR3DriverName, sizeof(szR3DriverName)/sizeof(TCHAR));
            idxR3DriverName++)
        {

            // Open the key to the driver name
            if (RegOpenKey(hkClass, szR3DriverName, &hkR3DriverName))
            {
                continue;
            }

            // Create the branch for this subclass
            pTreeNode = (PDMTREE_NODE)LocalAlloc(LPTR, sizeof(DMTREE_NODE));
            pTreeNode->NodeType = NodeTypeDriver;
            pTreeNode->pfnConfig = pSubtypeInfo->pfnConfig;
            pTreeNode->pfnQueryConfig = pSubtypeInfo->pfnQueryConfig;
            pTreeNode->dc = pSubtypeInfo->dc;

            // Get driver name
            cbLen = sizeof(pTreeNode->szDriver);
            RegQueryValueEx(hkR3DriverName, TEXT("Driver"), NULL, NULL, (LPBYTE)pTreeNode->szDriver, &cbLen);
            wcscpy(pTreeNode->wszDriver, pTreeNode->szDriver);

            // Get driver description
            cbLen = sizeof(pTreeNode->szDescription);
            RegQueryValueEx(hkR3DriverName, TEXT("Description"), NULL, NULL, (LPBYTE)pTreeNode->szDescription, &cbLen);

            // Get driver alias
            cbLen = sizeof(pTreeNode->szAlias);
            RegQueryValueEx(hkR3DriverName, TEXT("Alias"), NULL, NULL, (LPBYTE)pTreeNode->szAlias, &cbLen);
            wcscpy(pTreeNode->wszAlias, pTreeNode->szAlias);

            // Insert Class entry
            ti.hParent = htiClass;
            ti.hInsertAfter = TVI_LAST;
            ti.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            ti.item.iImage = ti.item.iSelectedImage = pSubtypeInfo->IconIndex;
            ti.item.pszText = pTreeNode->szDescription;
            ti.item.lParam = (LPARAM)pTreeNode;

            htiDriver = TreeView_InsertItem(hwndTree, &ti);

            // Close the Driver Name key
            RegCloseKey(hkR3DriverName);
        }
        // Close the class key
        RegCloseKey(hkClass);
    }

    // Open up the tree and display
    TreeView_Expand(hwndTree, htiRoot, TVE_EXPAND);
    SendMessage(hwndTree, WM_SETREDRAW, TRUE, 0L);

    return TRUE;
}


// Free up the tree
void DmFreeAdvDlgTree (HWND hTree, HTREEITEM hti)
{
    HTREEITEM htiChild;
    TV_ITEM tvi;

    // Delete all children by calling myself recursively
    while ((htiChild = TreeView_GetChild(hTree, hti)) != NULL)
    {
        DmFreeAdvDlgTree(hTree, htiChild);
    }

    if (hti!=TVI_ROOT)
    {
        // Delete my attached data structures
        tvi.mask = TVIF_PARAM;
        tvi.hItem = hti;
        tvi.lParam = 0;
        TreeView_GetItem(hTree, &tvi);
        if (tvi.lParam != 0)
            LocalFree ((HANDLE)tvi.lParam);

        // Delete myself
        TreeView_DeleteItem (hTree, hti);
    }

    return;
}

BOOL DmAdvPropPage_OnDestroy(
                            HWND    ParentHwnd,
                            LPNMHDR NmHdr
                            )
{
    HWND hTree;
    HIMAGELIST hImageList;
    POUR_PROP_PARAMS Params = (POUR_PROP_PARAMS) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    Params->bClosing = TRUE;    // Remember that we're now closing

    // Get handle to treeview control
    hTree = GetDlgItem(ParentHwnd, IDC_ADV_TREE);

    // Free all the entries on the control
    DmFreeAdvDlgTree(hTree,TVI_ROOT);

    // Free up the image list attached to the control
    hImageList = TreeView_GetImageList(hTree, TVSIL_NORMAL);
    if (hImageList)
    {
        TreeView_SetImageList(hTree, NULL, TVSIL_NORMAL);
        ImageList_Destroy (hImageList);
    }

    return FALSE;
}


