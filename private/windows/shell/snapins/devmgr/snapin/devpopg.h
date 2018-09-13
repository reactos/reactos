// devpopg.h : header file
//

#ifndef __DEVPOPG_H__
#define __DEVPOPG_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    devpopg.h

Abstract:

    header file for devpopg.cpp

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "proppage.h"
#include "wmium.h"
#include "wdmguid.h"


//
// help topic ids
//
#define IDH_DISABLEHELP (DWORD(-1))
#define IDH_DEVMGR_PWRMGR_WAKEENABLE    2003170
#define IDH_DEVMGR_PWRMGR_SHUTDOWNENABLE    2003180


typedef struct tagPowerTimeouts
{
    ULONG   ConservationIdleTime;
    ULONG   PerformanceIdleTime;
} DM_POWER_DEVICE_TIMEOUTS, *PDM_POWER_DEVICE_TIMEOUTS;

class CPowerEnable
{
public:
    CPowerEnable(const GUID& wmiGuid, ULONG DataBlockSize)
    : m_hWmiBlock(INVALID_HANDLE_VALUE),
      m_WmiInstDataSize(0), m_pWmiInstData(NULL),
      m_DataBlockSize(DataBlockSize)
    {
        m_wmiGuid = wmiGuid;
        m_DevInstId[0] = _T('\0');

    }

    virtual ~CPowerEnable()
    {
        Close();
    }
    BOOL IsOpened()
    {
        return INVALID_HANDLE_VALUE != m_hWmiBlock;
    }
    BOOL Get(BOOLEAN& fEnabled);
    BOOL Set(BOOLEAN fEnable);
    Open(LPCTSTR DeviceId);
    BOOL Close()
    {
        if (INVALID_HANDLE_VALUE != m_hWmiBlock)
        WmiCloseBlock(m_hWmiBlock);
        m_hWmiBlock = INVALID_HANDLE_VALUE;
        m_DevInstId[0] = _T('\0');
        if (m_pWmiInstData)
        {
        delete [] m_pWmiInstData;
        m_pWmiInstData = NULL;
        }
        m_WmiInstDataSize = 0;

        return TRUE;
    }
    operator GUID&()
    {
        return m_wmiGuid;
    }
protected:
    WMIHANDLE       m_hWmiBlock;
    ULONG       m_Version;
    GUID        m_wmiGuid;
    ULONG       m_WmiInstDataSize;
    BYTE*       m_pWmiInstData;
    ULONG       m_DataBlockSize;
    TCHAR       m_DevInstId[MAX_DEVICE_ID_LEN + 2];
};

class CPowerShutdownEnable : public CPowerEnable
{
public:
    CPowerShutdownEnable() : CPowerEnable(GUID_POWER_DEVICE_ENABLE, sizeof(BOOLEAN))
    {}
    // override dtor is not necessary
};
class CPowerWakeEnable : public CPowerEnable
{
public:
    CPowerWakeEnable() : CPowerEnable(GUID_POWER_DEVICE_WAKE_ENABLE, sizeof(BOOLEAN))
    {}
    // override dtor is not necessary
};

#if ENABLE_POWER_TIMEOUTS
class CPowerTimeout
{
public:
    CPowerTimeout()
    : m_hWmiBlock(INVALID_HANDLE_VALUE),
      m_pWmiInstData(NULL),
      m_WmiInstDataSize(0)
    {
        m_DevInstId[0] = _T('\0');
        m_DataBlockSize = sizeof(DM_POWER_DEVICE_TIMEOUTS);
    }

    ~CPowerTimeout()
    {
        Close();
    }
    BOOL IsOpened()
    {
        return INVALID_HANDLE_VALUE != m_hWmiBlock;
    }
    BOOL Open(LPCTSTR DevceId);
    BOOL Get(ULONG& Conservation, ULONG& Performance);
    BOOL Set(ULONG ConservationIdleTime, ULONG PerformanceIdleTime);
    BOOL Close()
    {
        if (INVALID_HANDLE_VALUE != m_hWmiBlock)
        {
        WmiCloseBlock(m_hWmiBlock);
        m_hWmiBlock = INVALID_HANDLE_VALUE;
        }
        if (m_pWmiInstData)
        {
        delete [] m_pWmiInstData;
        m_pWmiInstData = NULL;
        }
        m_DevInstId[0] = _T('\0');
        return TRUE;
    }
private:
    WMIHANDLE       m_hWmiBlock;
    ULONG       m_WmiInstDataSize;
    BYTE*       m_pWmiInstData;
    ULONG       m_Version;
    ULONG       m_DataBlockSize;
};
#endif

class CDevicePowerMgmtPage : public CPropSheetPage
{
public:
    CDevicePowerMgmtPage() :
        m_pDevice(NULL),
        CPropSheetPage(g_hInstance, IDD_DEVPOWER_PAGE)
        {}
    ~CDevicePowerMgmtPage()
        {}
    HPROPSHEETPAGE Create(CDevice* pDevice)
    {
        ASSERT(pDevice);
        m_pDevice = pDevice;
        //if (!m_poTimeout.Open(pDevice) && !m_poWakeEnable.Open(pDevice))
        //  return NULL;
        // override PROPSHEETPAGE structure here...
        m_psp.lParam = (LPARAM)this;
        return CreatePage();
    }

protected:
    virtual BOOL OnInitDialog(LPPROPSHEETPAGE ppsp);
#if ENABLE_POWER_TIMEOUTS
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
#endif
    virtual BOOL OnApply();
    virtual void UpdateControls(LPARAM lParam = 0);
    virtual BOOL OnHelp(LPHELPINFO pHelpInfo);
    virtual BOOL OnContextMenu(HWND hWnd, WORD xPos, WORD yPos);
private:
    CDevice*    m_pDevice;
    CPowerShutdownEnable m_poShutdownEnable;
    CPowerWakeEnable m_poWakeEnable;

#if ENABLE_POWER_TIMEOUTS
    CPowerTimeout   m_poTimeout;
#endif
};

#endif // _DEVPOPG_H__
