// devgenpg.h : header file
//

#ifndef __DEVGENPG_H__
#define __DEVGENPG_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devgenpg.h

Abstract:

    header file for devgenpg.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "proppage.h"
//#include "tshooter.h"

//
// help topic ids
//
#define IDH_DISABLEHELP (DWORD(-1))
#define idh_devmgr_general_devicetype   103100  // General: "" (Static)
#define idh_devmgr_general_manufacturer 103110  // General: "" (Static)
#define idh_devmgr_general_hardware_revision    103120  // General: "Not Available" (Static)
#define idh_devmgr_general_device_status    103130  // General: "" (Static)
#define idh_devmgr_general_device_usage 103140  // General: "List1" (SysListView32)
#define idh_devmgr_general_location 103160
#define idh_devmgr_general_trouble  103150  // troubelshooting button

class CHwProfileList;
class CProblemAgent;

const int LVIS_GCNOCHECK = INDEXTOSTATEIMAGEMASK(1);
const int LVIS_GCCHECK = INDEXTOSTATEIMAGEMASK(2);

extern const int CMProblemNumberToStringID[];

#define DI_NEEDPOWERCYCLE   0x400000L

#define DEVICE_ENABLE           0
#define DEVICE_DISABLE          1
#define DEVICE_DISABLE_GLOBAL   2

class CDeviceGeneralPage : public CPropSheetPage
{
public:
    CDeviceGeneralPage() :
    m_pDevice(NULL),
    m_pHwProfileList(NULL),
    m_RestartFlags(0),
    m_pProblemAgent(NULL),
    m_hwndLocationTip(NULL),
    CPropSheetPage(g_hInstance, IDD_DEVGEN_PAGE)
    {}
    ~CDeviceGeneralPage();
    HPROPSHEETPAGE Create(CDevice* pDevice);
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
    virtual BOOL OnApply(void);
    virtual BOOL OnQuerySiblings(WPARAM wParam, LPARAM lParam);
    virtual void UpdateControls(LPARAM lParam = 0);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
    virtual UINT  DestroyCallback();

private:
    void UpdateHwProfileStates();
    CHwProfileList* m_pHwProfileList;
    DWORD   m_hwpfCur;
    CDevice* m_pDevice;
    int     m_CurrentDeviceUsage;
    int     m_SelectedDeviceUsage;
    DWORD   m_RestartFlags;
    CProblemAgent*  m_pProblemAgent;
    HWND    m_hwndLocationTip;
};


#endif // __DEVGENPG_H__
