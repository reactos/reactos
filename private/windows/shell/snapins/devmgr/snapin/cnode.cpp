/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cnode.cpp

Abstract:

    This module implements CDevice, CClass, CResource and CComputer classes.

Author:

    William Hsieh (williamh) created

Revision History:


--*/
#include "devmgr.h"
#include "cdriver.h"
#include "hwprof.h"
#include "sysinfo.h"
#include <initguid.h>
#include <mountmgr.h>
#include <devguid.h>
#include <wdmguid.h>


#if mymachine
TCHAR* DEVICEID_COM1    = TEXT("ROOT\\*PNP0501\\2_0_17_0_0_0");
TCHAR* DEVICEID_COM2    = TEXT("ROOT\\*PNP0501\\2_0_17_1_0_0");
TCHAR* DEVICEID_LPT1    = TEXT("ROOT\\*PNP0400\\2_0_20_0_0_0");
#else
TCHAR* DEVICEID_COM1    = TEXT("ACPI\\PNP0501\\0X00000001");
TCHAR* DEVICEID_COM2    = TEXT("ACPI\\PNP0501\\0X00000002");
TCHAR* DEVICEID_LPT1    = TEXT("ACPI\\PNP0401\\3&");
#endif


//
// CClass implementation
//
CClass::CClass(
    CMachine* pMachine,
    GUID* pGuid
    )
{
    m_Guid = *pGuid;
    ASSERT(pMachine);
    ASSERT(pGuid);

    HKEY hKey = NULL;

    m_NoDisplay = FALSE;
    m_pMachine = pMachine;
    m_TotalDevices = 0;
    m_TotalHiddenDevices = 0;
    m_pDevInfoList = NULL;
    m_pos = NULL;

    if (!m_pMachine->DiGetClassFriendlyNameString(pGuid, m_strDisplayName)) 
    {
        m_strDisplayName.LoadString(g_hInstance, IDS_UNKNOWN);
    }
    
    m_pMachine->DiGetClassImageIndex(pGuid, &m_iImage);

    hKey = m_pMachine->DiOpenClassRegKey(pGuid, KEY_READ, DIOCR_INSTALLER);

    if (INVALID_HANDLE_VALUE != hKey) 
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_NODISPLAYCLASS, NULL, NULL, NULL, NULL)) 
            m_NoDisplay = TRUE;
        
        RegCloseKey(hKey);
    }
}

CDevInfoList*
CClass::GetDevInfoList(
    HWND hwndParent
    )
{
    if (!m_pDevInfoList) 
    {
        HDEVINFO hDevInfo = m_pMachine->DiCreateDeviceInfoList(&m_Guid, hwndParent);

        if (hDevInfo && INVALID_HANDLE_VALUE != hDevInfo) 
        {
            m_pDevInfoList = new CDevInfoList(hDevInfo, hwndParent);
        }
    }
    
    return m_pDevInfoList;
}

inline
CItemIdentifier*
CClass::CreateIdentifier()
{
    return new CClassIdentifier(*this);
}

CClass::~CClass()
{
    m_listDevice.RemoveAll();

    if (m_pDevInfoList)
        delete m_pDevInfoList;
}

HICON
CClass::LoadIcon()
{
    HICON hClassIcon;

    if (!m_pMachine->DiLoadClassIcon(&m_Guid, &hClassIcon, NULL))
        return NULL;
    
    return hClassIcon;
}

void
CClass::AddDevice(CDevice* pDevice)
{
    ASSERT(pDevice);

    m_listDevice.AddTail(pDevice);
    m_TotalDevices++;

    // Every device under a NoDisplay class is a hidden device
    if (m_NoDisplay || pDevice->IsHidden()) {
    
        m_TotalHiddenDevices++;
    }
}

BOOL
CClass::GetFirstDevice(
    CDevice** ppDevice,
    PVOID&    Context
    )
{
    ASSERT(ppDevice);

    if (!m_listDevice.IsEmpty()) 
    {
        POSITION pos;

        pos = m_listDevice.GetHeadPosition();
        *ppDevice = m_listDevice.GetNext(pos);
        Context = pos;

        return TRUE;
    }
    
    *ppDevice = NULL;

    return FALSE;
}

BOOL
CClass::GetNextDevice(
    CDevice** ppDevice,
    PVOID&    Context
    )
{
    ASSERT(ppDevice);

    POSITION pos = (POSITION)(Context);

    if(NULL != pos) 
    {
        *ppDevice = m_listDevice.GetNext(pos);
        Context = pos;
        return TRUE;
    }
    
    *ppDevice = NULL;

    return FALSE;
}

void
CClass::PropertyChanged()
{
    HKEY hKey = NULL;

    if (!m_pMachine->DiGetClassFriendlyNameString(&m_Guid, m_strDisplayName))
        m_strDisplayName.LoadString(g_hInstance, IDS_UNKNOWN);
    
    m_pMachine->DiGetClassImageIndex(&m_Guid, &m_iImage);

    if (m_pDevInfoList) 
    {
        delete m_pDevInfoList;
        m_pDevInfoList = NULL;
    }
    
    hKey = m_pMachine->DiOpenClassRegKey(&m_Guid, KEY_READ, DIOCR_INSTALLER);

    if (INVALID_HANDLE_VALUE != hKey) 
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, REGSTR_VAL_NODISPLAYCLASS, NULL, NULL, NULL, NULL))
            m_NoDisplay = TRUE;
        
        RegCloseKey(hKey);
    }
}

// CDevice implementation
//
CDevice::CDevice(
    CMachine* pMachine,
    CClass* pClass,
    PSP_DEVINFO_DATA pDevData
    )
{

    ASSERT(pMachine && pDevData && pClass);
    m_DevData = *pDevData;
    m_pClass = pClass;
    m_pMachine = pMachine;
    m_pSibling = NULL;
    m_pParent = NULL;
    m_pChild = NULL;

    if (!m_pMachine->CmGetDescriptionString(m_DevData.DevInst, m_strDisplayName))
        m_strDisplayName.LoadString(g_hInstance, IDS_UNKNOWN_DEVICE);
    
    m_pMachine->CmGetDeviceIDString(m_DevData.DevInst, m_strDeviceID);
    m_iImage = m_pClass->GetImageIndex();
}

inline
CItemIdentifier*
CDevice::CreateIdentifier()
{
    return new CDeviceIdentifier(*this);
}

void
CDevice::PropertyChanged()
{
    if (!m_pMachine->CmGetDescriptionString(m_DevData.DevInst, m_strDisplayName))
        m_strDisplayName.LoadString(g_hInstance, IDS_UNKNOWN_DEVICE);
    
    m_pMachine->CmGetDeviceIDString(m_DevData.DevInst, m_strDeviceID);
    m_iImage = m_pClass->GetImageIndex();
}

HICON
CDevice::LoadClassIcon()
{
    HICON hClassIcon;
    hClassIcon = NULL;

    if (m_pMachine->DiLoadClassIcon(&m_DevData.ClassGuid, &hClassIcon, NULL))
        return hClassIcon;

    return NULL;
}

BOOL
CDevice::GetStatus(
    DWORD* pStatus,
    DWORD* pProblem
    )
{
    return  m_pMachine->CmGetStatus(m_DevData.DevInst, pProblem, pStatus);
}

BOOL
CDevice::IsRAW()
{
    DWORD Capabilities;
    
    return (m_pMachine->CmGetCapabilities(m_DevData.DevInst, &Capabilities) &&
            (CM_DEVCAP_RAWDEVICEOK & Capabilities));
}

BOOL
CDevice::IsHidden()
{
    CClass *pClass = GetClass();

    //
    // A device is hidden if one of the following are TRUE:
    //
    // - It's class is a NoDisplayClass
    // - It has the DN_NO_SHOW_IN_DM Status flag set
    // - It is a Phantom devnode
    //
    return (NoShowInDM() || IsPhantom() || pClass->NoDisplay());
}

BOOL
CDevice::IsPhantom()
{
    DWORD Status, Problem;

    return !m_pMachine->CmGetStatus(m_DevData.DevInst, &Problem, &Status) &&
            (CR_NO_SUCH_VALUE == m_pMachine->GetLastCR() ||
            CR_NO_SUCH_DEVINST == m_pMachine->GetLastCR());
}

BOOL
CDevice::NoShowInDM()
{
    DWORD Status, Problem;
    Status = 0;
    GetStatus(&Status, &Problem);
    return (Status & DN_NO_SHOW_IN_DM);
}

BOOL
CDevice::IsUninstallable(
    )
/*++

    This function determins whether a device can be uninstalled. A device
    cannot be uninstalled if it is a ROOT device and it does not have
    the DN_DISABLEABLE DevNode status bit set.
    
Return Value:
    TRUE if the device can be uninstalled.
    FALSE if the device cannot be uninstalled.    
    
--*/
{
    DWORD Status, Problem;

    if (GetStatus(&Status, &Problem) &&
        !(Status & DN_DISABLEABLE) &&
         (Status & DN_ROOT_ENUMERATED)) {

        return FALSE;
    }
    
    return TRUE;
}

BOOL
CDevice::IsDisableable(
    )
/*++

    This function determins whether a device can be disabled or not by 
    checking the DN_DISABLEABLE DevNode status bit.
    
    A device that is currently Hardware Disabled cannot be software disabled.
    
Return Value:
    TRUE if the device can be disabled.
    FALSE if the device cannot be disabled.    

--*/
{
    DWORD Status, Problem;

    if (GetStatus(&Status, &Problem) &&
        (Status & DN_DISABLEABLE) &&
        (CM_PROB_HARDWARE_DISABLED != Problem)) {

        return TRUE;
    }

    return FALSE;
}

BOOL
CDevice::IsDisabled(
    )
/*++

    A device is disabled if it has the problem CM_PROB_DISABLED.  
    
Return Value:    
    TRUE if device is disabled.
    FALSE if device is NOT disabled.
    
--*/
{
    DWORD Status, Problem;

    if (GetStatus(&Status, &Problem)) 
    {
        return ((Status & DN_HAS_PROBLEM) && (CM_PROB_DISABLED == Problem));
    }

    return FALSE;
}

BOOL
CDevice::IsStateDisabled(
    )
/*++

    A device state is disabled if it has the CONFIGFLAG_DISABLED ConfigFlag
    set or the CSCONFIGFLAG_DISABLED Config Specific ConfigFlag disabled in 
    the current profile.
    
    Note that a device disabled State has nothing to do with whether the device
    is currently physically disabled or not.  The disabled state is just a registry
    flag that tells Plug and Play what to do with the device the next time it is 
    started.
    
Return Value:    
    TRUE if device's state is disabled.
    FALSE if device's state is NOT disabled.
    
--*/
{
    ULONG hwpfCurrent;
    DWORD Flags;

    //
    // Check if the device state is globally disabled by checking it's ConfigFlags
    //
    GetConfigFlags(&Flags);
    if (Flags & CONFIGFLAG_DISABLED) {
        return TRUE;
    }

    //
    // Check if the device state is disabled in the current hardware profile by 
    // checking it's Config Specific ConfigFlags.
    //
    if (m_pMachine->CmGetCurrentHwProfile(&hwpfCurrent) &&
        m_pMachine->CmGetHwProfileFlags(m_DevData.DevInst, hwpfCurrent, &Flags) &&
        (Flags & CSCONFIGFLAG_DISABLED)) {
        return TRUE;
    }

    return FALSE;
}

BOOL
CDevice::IsStarted()
{
    ULONG hwpfCurrent;
    DWORD Flags;

    if (m_pMachine->CmGetCurrentHwProfile(&hwpfCurrent) &&
        m_pMachine->CmGetHwProfileFlags(m_DevData.DevInst, hwpfCurrent, &Flags) &&
        !(Flags & CSCONFIGFLAG_DO_NOT_START))
        return TRUE;
    
    return FALSE;
}

BOOL
CDevice::HasProblem(
    )
/*++

    This function returns whether a device has a problem or not.
    
Return Value:    
    TRUE if device has a problem.
    FALSE if device does not have a problem.
    
--*/
{
    DWORD Status, Problem;    
    
    if (GetStatus(&Status, &Problem))
    {
        //
        // If the DN_HAS_PROBLEM or DN_PRIVATE_PROBLEM status bits are set
        // then this device has a problem, unless the problem is CM_PROB_MOVED
        // or CM_PROB_DISABLED_SERVICE.
        //
        if ((Status & DN_PRIVATE_PROBLEM) ||
            ((Status & DN_HAS_PROBLEM) && (Problem != CM_PROB_MOVED) &&
             (Problem != CM_PROB_DISABLED_SERVICE)))
        {
            return TRUE;
        }

        //
        // If the device is not started and RAW capable then it also has a problem
        //
        if (!(Status & DN_STARTED) && IsRAW()) 
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
CDevice::NeedsRestart(
    )
/*++

    This function returns whether a device needs a restart or not.  It checks the
    DN_NEED_RESTART Status flag.  
    
Return Value:    
    TRUE if device needs the computer to be restarted for it to work properly.
    FALSE if device does not need the computer to be restarted.
    
--*/
{
    DWORD Status, Problem;

    if (GetStatus(&Status, &Problem)) 
    {
        return (Status & DN_NEED_RESTART);
    }

    return FALSE;
}

CDevice*
CDevice::FindMFParent()
{
    if (!IsMFChild())
        return NULL;
    
    ASSERT(m_pParent);

    CDevice* pDevice = m_pParent;

    while (pDevice->IsMFChild())
        pDevice = pDevice->GetParent();
    
    return pDevice;
}

BOOL
CDevice::IsMFChild(
    )
{
    DWORD Status, Problem;
    Status = 0;
    GetStatus(&Status, &Problem);
    return (Status & DN_MF_CHILD);
}

BOOL
CDevice::IsSpecialMFChild(
    )
{
    if (IsMFChild()) 
    {
        DWORD Flags;

        if (GetConfigFlags(&Flags))
            return (Flags & CONFIGFLAG_CANTSTOPACHILD);
    }
    
    return FALSE;
}

// This function determines if the device is a pnp device
// INPUT:
//  None
// OUTPUT:
//  TRUE if the device is a pnp device
//  FLASE if the device is not a pnp device or undetermined
BOOL
CDevice::IsPnpDevice()
{
    // A device is pnp device if the device id is not led
    // by "root" and DN_ROOT_ENUMERATED is not set
    // in its status.
    if (!m_strDeviceID.IsEmpty()) 
    {
        // unfortunately, we do not have lstrcmpin library function
        TCHAR DeviceId[MAX_DEVICE_ID_LEN + 1];
        int len = lstrlen(REGSTR_KEY_ROOTENUM);

        lstrcpyn(DeviceId, (LPCTSTR)m_strDeviceID, len);
        DeviceId[len] = _T('\0');

        if (lstrcmpi(DeviceId, REGSTR_KEY_ROOTENUM)) 
        {
            // the device id does not start with "root"
            // check the status
            DWORD Problem, Status;

            if (GetStatus(&Status, &Problem))
            {
                return !(Status & DN_ROOT_ENUMERATED);
            }
        }
    }
    
    return FALSE;
}

BOOL
CDevice::IsBiosDevice()
{
    // A device is bios enumerated if its device id
    // is led by "root" and its status does not
    // have DN_ROOT_ENUMERATED
    if (!m_strDeviceID.IsEmpty()) 
    {
        TCHAR DeviceId[MAX_DEVICE_ID_LEN + 1];
        int len = lstrlen(REGSTR_KEY_ROOTENUM);
        
        lstrcpyn(DeviceId, (LPCTSTR)m_strDeviceID, len);
        DeviceId[len] = _T('\0');

        if (!lstrcmpi(DeviceId, REGSTR_KEY_ROOTENUM)) 
        {
            // the device id starts with "root"
            // check the status
            DWORD Problem, Status;

            if (GetStatus(&Status, &Problem)) 
            {
                return !(Status & DN_ROOT_ENUMERATED);
            }
        }
    }
    
    return FALSE;
}

BOOL
CDevice::IsPCMCIA()
{
    ASSERT(m_pClass);

    // if the device's class is PCMCIA, it is a PCMCIA device
    if (IsEqualGUID(*m_pClass, GUID_DEVCLASS_PCMCIA))
        return TRUE;
    
    // if the device has ancestor(s), it is a PCMCIA if one of
    // its ancestor is PCMCIA
    if (m_pParent)
        return m_pParent->IsPCMCIA();
    
    return FALSE;
}

BOOL
CDevice::IsPCIDevice()
{
    GUID BusGuid;
    CONFIGRET ConfigRet;

    if (m_pMachine->CmGetBusGuid(GetDevNode(), &BusGuid) &&
        IsEqualGUID(BusGuid, GUID_BUS_TYPE_PCI)) {

        return TRUE;
    }

    return FALSE;
}

BOOL
CDevice::GetConfigFlags(DWORD* pFlags)
{
    return m_pMachine->CmGetConfigFlags(m_DevData.DevInst, pFlags);
}

BOOL
CDevice::GetKnownLogConf(LOG_CONF* plc, DWORD* plcType)
{
    return m_pMachine->CmGetKnownLogConf(m_DevData.DevInst, plc, plcType);
}

BOOL
CDevice::HasResources()
{
    return m_pMachine->CmHasResources(m_DevData.DevInst);
}

void
CDevice::GetMFGString(
    String& strMFG
    )
{
    m_pMachine->CmGetMFGString(m_DevData.DevInst, strMFG);

    if (strMFG.IsEmpty()) 
    {
        strMFG.LoadString(g_hInstance, IDS_UNKNOWN);
    }
}

void
CDevice::GetProviderString(
    String& strProvider
    )
{
    m_pMachine->CmGetProviderString(m_DevData.DevInst, strProvider);
    
    if (strProvider.IsEmpty()) {
    
        strProvider.LoadString(g_hInstance, IDS_UNKNOWN);
    }
}

void
CDevice::GetDriverDateString(
    String& strDriverDate
    )
{
    FILETIME ft;

    strDriverDate.Empty();

    //
    // First try to get the driver date FileTime data from the registry,
    // this way we can localize the date.
    //
    if (m_pMachine->CmGetDriverDateData(m_DevData.DevInst, &ft)) {

        SYSTEMTIME SystemTime;
        TCHAR DriverDate[MAX_PATH];

        DriverDate[0] = TEXT('\0');

        if (FileTimeToSystemTime(&ft, &SystemTime)) {

            if (GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &SystemTime,
                          NULL,
                          DriverDate,
                          sizeof(DriverDate)/sizeof(TCHAR)
                          ) != 0) {

                strDriverDate = DriverDate;
            }
        }

    } else {
    
        //
        // We couldn't get the FileTime data so just get the DriverDate string
        // from the registry.
        //
        m_pMachine->CmGetDriverDateString(m_DevData.DevInst, strDriverDate);
    }

    if (strDriverDate.IsEmpty()) {
    
        strDriverDate.LoadString(g_hInstance, IDS_NOT_AVAILABLE);
    }
}

void
CDevice::GetDriverVersionString(
    String& strDriverVersion
    )
{
    m_pMachine->CmGetDriverVersionString(m_DevData.DevInst, strDriverVersion);

    if (strDriverVersion.IsEmpty()) {
    
        strDriverVersion.LoadString(g_hInstance, IDS_NOT_AVAILABLE);
    }
}

LPCTSTR
CDevice::GetClassDisplayName()
{
    if (m_pClass)
        return m_pClass->GetDisplayName();
    
    else
        return NULL;
}

BOOL
CDevice::NoChangeUsage()
{
    SP_DEVINSTALL_PARAMS dip;
    dip.cbSize = sizeof(dip);

    if (m_pMachine->DiGetDeviceInstallParams(&m_DevData, &dip))
        return (dip.Flags & DI_PROPS_NOCHANGEUSAGE);
    
    else
        return TRUE;
}

CDriver*
CDevice::CreateDriver()
{
    SP_DRVINFO_DATA DrvInfoData;
    PSP_DRVINFO_DATA pDrvInfoData;
    CDriver* pDriver = NULL;

    DrvInfoData.cbSize = sizeof(DrvInfoData);

    // If the device has a selected driver, use it. Otherwise,
    // we pass a NULL drvinfodata so that CDriver will search
    // for the device driver key or service to create
    // a list of driver files.
    if (m_pMachine->DiGetSelectedDriver(&m_DevData, &DrvInfoData))
    {
        pDrvInfoData = &DrvInfoData;
    }
    
    else
    {
        pDrvInfoData = NULL;
    }

    pDriver = new CDriver();

    // if we have a selected driver and we failed to
    // to create the driver, retry it without the selected driver
    //
    if (pDrvInfoData && pDriver->Create(this, pDrvInfoData)) {

        return pDriver;
    }

    pDriver->Create(this, NULL);

    return pDriver;
}

BOOL
CDevice::HasDrivers()
{
    HKEY hKey;
    BOOL HasDriverKey, HasServiceKey;
    CDriver DeviceDriver;
    BOOL Result;

    DEBUGBREAK_ON(DEBUG_OPTIONS_BREAKON_CHECKDRIVERS);

    HasDriverKey = FALSE;

    if (m_pMachine->IsLocal()) 
    {
        // open drvice's driver registry key
        hKey = m_pMachine->DiOpenDevRegKey(&m_DevData, DICS_FLAG_GLOBAL,
                        0, DIREG_DRV, KEY_ALL_ACCESS);
                        
        HasDriverKey = INVALID_HANDLE_VALUE != hKey;

        if (INVALID_HANDLE_VALUE != hKey) 
            RegCloseKey(hKey);
    }
    
    DWORD Size = 0;

    m_pMachine->DiGetDeviceRegistryProperty(&m_DevData, SPDRP_SERVICE,
                        NULL, NULL, 0, &Size);

    HasServiceKey = (0 != Size);

    if (HasServiceKey || HasDriverKey) 
    {
        // either we have a driver or a service key.
        // try to find out if we can find any valid driver files.
        Result = DeviceDriver.Create(this);
    }
    
    else 
    {
        if (m_pMachine->IsLocal() && g_HasLoadDriverNamePrivilege) 
        {
            // If the target machine is local and the user has the
            // administrator privilege, we need the driver page
            // for users to update the drivers.
            //
            Result = TRUE;
        }
        
        else 
        {
            Result = FALSE;
        }
    }
    
    return Result;
}

DWORD
CDevice::EnableDisableDevice(
    HWND hDlg,
    BOOL Enabling
    )
{
    BOOL Disabling = !Enabling;
    BOOL Canceled;
    BOOL Refresh = FALSE;
    Canceled = FALSE;
    DWORD RestartFlags = 0;
    DWORD ConfigFlags;
    HCURSOR hCursorOld = NULL;

    //
    // Disable refreshing the TREE while we are enabling/disabling this device
    //
    m_pMachine->EnableRefresh(FALSE);

    if (!GetConfigFlags(&ConfigFlags)) {
    
        ConfigFlags = 0;
    }

    // only want the disabled bit
    ConfigFlags &= CONFIGFLAG_DISABLED;

    CHwProfileList* pHwProfileList = new CHwProfileList();
    pHwProfileList->Create(this, ConfigFlags);

    //
    // Get the current profile
    //
    CHwProfile* phwpf;

    if (!(pHwProfileList->GetCurrentHwProfile(&phwpf))) {

        goto clean0;
    }

    //
    // Can only enable a device that is currently disabled
    //
    if (IsStateDisabled() && Enabling) {

        phwpf->SetEnablePending();
    }

    //
    // Can only disable a device that is currently enabled
    //
    else if (!IsStateDisabled() && Disabling) {

        phwpf->SetDisablePending();
    }

    //
    // If we don't have a valid enable or disable then exit
    //
    if (!(phwpf->IsEnablePending()) && !(phwpf->IsDisablePending())) {
    
        goto clean0;
    }

    //
    // This device is not a boot device so just display the normal disable
    // warning to the user.
    //
    if (Disabling) {

        int MsgBoxResult;
        TCHAR szText[MAX_PATH];
        DWORD Size;
        
        Size = LoadResourceString(IDS_WARN_NORMAL_DISABLE, szText, ARRAYLEN(szText));
        MsgBoxResult = MessageBox(hDlg, 
                                  szText,
                                  GetDisplayName(),
                                  MB_ICONEXCLAMATION | MB_YESNO | MB_DEFBUTTON2
                                  );

        if (IDYES != MsgBoxResult) {
            goto clean0;
        }
    }

    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    m_pMachine->DiTurnOnDiFlags(*this, DI_NODI_DEFAULTACTION);

    SP_PROPCHANGE_PARAMS pcp;
    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

    //
    // Ask the class installer if the device can be generally enabled/disabled
    //
    pcp.Scope = DICS_FLAG_CONFIGGENERAL;

    if ((phwpf->IsEnablePending())) {

        pcp.StateChange = DICS_ENABLE;

    } else {

        pcp.StateChange = DICS_DISABLE;

    }

    m_pMachine->DiSetClassInstallParams(*this,
                                        (PSP_CLASSINSTALL_HEADER)&pcp,
                                        sizeof(pcp)
                                        );
                    
    m_pMachine->DiCallClassInstaller(DIF_PROPERTYCHANGE, *this);

    Canceled = (ERROR_CANCELLED == GetLastError());

    if (Enabling && Canceled && ((m_pMachine->DiGetFlags(*this)) & DI_NEEDREBOOT) &&
        IsPCMCIA()) {
        
        RestartFlags |= DI_NEEDPOWERCYCLE;
    }
    
    //
    // Now ask the class installer if the device can be specifically enabled/disabled
    //
    if (!Canceled) {
    
        pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
        pcp.StateChange = DICS_ENABLE;
        
        if (phwpf->IsEnablePending()) {
        
            pcp.StateChange = DICS_ENABLE;
        }
        
        else {
        
            pcp.StateChange = DICS_DISABLE;
        }
        
        pcp.HwProfile = phwpf->GetHwProfile();

        m_pMachine->DiSetClassInstallParams(*this,
                                            (PSP_CLASSINSTALL_HEADER)&pcp,
                                            sizeof(pcp)
                                            );
                
        m_pMachine->DiCallClassInstaller(DIF_PROPERTYCHANGE, *this);
        Canceled = (ERROR_CANCELLED == GetLastError());
    }
    

    //
    // class installer has not objection of our enabling/disabling,
    // do real enabling/disabling.
    //
    if (!Canceled) {

        Refresh = TRUE;
    
        if (phwpf->IsDisablePending()) {

            pcp.StateChange = DICS_DISABLE;
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = phwpf->GetHwProfile();
            m_pMachine->DiSetClassInstallParams(*this,
                                                (PSP_CLASSINSTALL_HEADER)&pcp,
                                                sizeof(pcp)
                                                );
                    
            m_pMachine->DiChangeState(*this);
        }
                    
        else {

            // we are enabling the device,
            // do a specific enabling then a globally enabling.
            // the globally enabling will start the device
            // The implementation here is different from
            // Win9x which does a global enabling, a config
            // specific enabling and then a start.
            
            pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;
            pcp.HwProfile = phwpf->GetHwProfile();
            m_pMachine->DiSetClassInstallParams(*this,
                                                (PSP_CLASSINSTALL_HEADER)&pcp,
                                                sizeof(pcp)
                                                );
                        
            m_pMachine->DiChangeState(*this);

            // this call will start the device is it not started.
            pcp.Scope = DICS_FLAG_GLOBAL;
            m_pMachine->DiSetClassInstallParams(*this,
                                                (PSP_CLASSINSTALL_HEADER)&pcp,
                                                sizeof(pcp)
                                                );
                        
            m_pMachine->DiChangeState(*this);
        }
                    
        if (phwpf->IsEnablePending()) {
        
            phwpf->ResetEnablePending();
        }
        
        else if (phwpf->IsDisablePending()) {
        
            phwpf->ResetDisablePending();
        }

        //
        // signal that the property of the device is changed.
        //
        m_pMachine->DiTurnOnDiFlags(*this, DI_PROPERTIES_CHANGE);

        //
        // See if we need a restart.
        //
        RestartFlags |= (m_pMachine->DiGetFlags(*this)) & (DI_NEEDRESTART | DI_NEEDREBOOT);

        if (NeedsRestart()) {

            RestartFlags |= DI_NEEDRESTART;
        }
    }

    // remove class install parameters, this also reset
    // DI_CLASSINATLLPARAMS
    m_pMachine->DiSetClassInstallParams(*this, NULL, 0);

    m_pMachine->DiTurnOffDiFlags(*this, DI_NODI_DEFAULTACTION);

clean0:
    delete pHwProfileList;

    //
    // Refresh the TREE
    //
    m_pMachine->EnableRefresh(TRUE);

    if (Refresh) {
    
        m_pMachine->ScheduleRefresh();
    }

    if (hCursorOld != NULL)
    {
        SetCursor(hCursorOld);
    }

    return RestartFlags;
}

BOOL
CDevice::operator ==(
    CDevice& OtherDevice
    )
{
    return \
        (*m_pClass == *(OtherDevice.GetClass())) &&
        !lstrcmpi(m_strDeviceID, OtherDevice.GetDeviceID()) &&
        !lstrcmpi(m_strDisplayName, OtherDevice.GetDisplayName());
}

//
// CComputer implementation
//

CComputer::CComputer(
    CMachine* pMachine,
    DEVNODE dnRoot
    )
{
    ASSERT(pMachine);
    ASSERT(!GetChild() && !GetParent() && !GetSibling());
    m_pMachine = pMachine;
    m_strDisplayName.Empty();
    m_strDisplayName = pMachine->GetMachineDisplayName();
    m_iImage = pMachine->GetComputerIconIndex();
    m_dnRoot = dnRoot;
}

inline
CItemIdentifier*
CComputer::CreateIdentifier()
{
    return new CComputerIdentifier(*this);
}


CResource::CResource(
    CDevice* pDevice,
    RESOURCEID ResType,
    DWORDLONG dlBase,
    DWORDLONG dlLen,
    BOOL Forced,
    BOOL Free
    )
{

    m_pChild = NULL;
    m_pSibling = NULL;
    m_pParent =  NULL;
    m_ResType = ResType;
    m_dlBase = dlBase;
    m_dlLen = dlLen;
    m_Forced = Forced;
    m_dlEnd = m_dlBase + m_dlLen - 1;
    m_Allocated = !Free;
    ASSERT(pDevice);
    m_pDevice = pDevice;
    m_iImage = pDevice->GetImageIndex();
    ASSERT(ResType >= ResType_Mem && ResType <= ResType_IRQ);

    m_strDisplayName.Empty();
    m_strDisplayName = pDevice->GetDisplayName();

    if (ResType_IRQ == m_ResType)
    {
        String strBus;

        strBus.LoadString(g_hInstance, 
                          pDevice->IsPCIDevice() ? IDS_PCI : IDS_ISA
                          );

        m_strViewName.Format(TEXT("%2d    "), m_dlBase);
        m_strViewName = strBus + m_strViewName;
    }
    else if (ResType_DMA == m_ResType)
    {
        m_strViewName.Format(TEXT("%2d    " ), m_dlBase);
    }
    else 
    {
        m_strViewName.Format(TEXT("[%08lX - %08lX]  "), (ULONG)m_dlBase, (ULONG)m_dlEnd);
    }
    
    if (m_Allocated) 
    {
        m_strViewName += pDevice->GetDisplayName();
    }
}

void
CResource::GetRangeString(
    String& strRange
    )
{
    if (ResType_IRQ == m_ResType ||
        ResType_DMA == m_ResType)
    {
        strRange.Format(TEXT("%2d"), m_dlBase);
    }
    
    else 
    {
        strRange.Format(TEXT("[%08lX - %08lX]"), (ULONG)m_dlBase, (ULONG)m_dlEnd);
    }
}

#ifdef RESOURCE_STATUS
void
CResource::GetStatusString(
    String& strStatus
    )
{
    if (m_Allocated) 
    {
        strStatus.LoadString(g_hInstance, IDS_ALLOCATED);
    }
    
    else 
    {
        strStatus.LoadString(g_hInstance, IDS_FREE);
    }
}
#endif

BOOL
CResource::operator <=(
    const CResource& resSrc
    )
{
    DWORDLONG dlBase, dlLen;

    resSrc.GetValue(&dlBase, &dlLen);

    if (m_dlBase < dlBase)
        return TRUE;
    
    // if this resource contain the given resource,
    // we are smaller!
    if (m_dlBase == dlBase)
        return (m_dlBase + m_dlLen > dlBase + dlLen);
    
    return FALSE;
}

BOOL
CResource::EnclosedBy(
    const CResource& resSrc
    )
{
    DWORDLONG dlBase, dlLen;
    resSrc.GetValue(&dlBase, &dlLen);
    return m_dlBase >= dlBase && m_dlBase + m_dlLen <= dlBase + dlLen;
}


CResourceType::CResourceType(
    CMachine* pMachine,
    RESOURCEID ResType
    )
{
    int iStringID;

    m_ResType = ResType;
    m_pChild = NULL;
    m_pSibling = NULL;
    m_pParent =  NULL;
    m_pMachine = pMachine;
    ASSERT(ResType >= ResType_Mem && ResType <= ResType_IRQ);

    switch (ResType)
    {
        case ResType_IRQ:
            iStringID = IDS_VIEW_RESOURCE_IRQ;
            break;
        case ResType_IO:
            iStringID = IDS_VIEW_RESOURCE_IO;
            break;
        case ResType_DMA:
            iStringID = IDS_VIEW_RESOURCE_DMA;
            break;
        case ResType_Mem:
            iStringID = IDS_VIEW_RESOURCE_MEM;
            break;
        default:
            iStringID = IDS_UNKNOWN;
            break;
    }
    m_strDisplayName.Empty();
    m_strDisplayName.LoadString(g_hInstance, iStringID);
    m_iImage = pMachine->GetResourceIconIndex();
}

inline
CItemIdentifier*
CResourceType::CreateIdentifier()
{
    return new CResourceTypeIdentifier(*this);
}


// This function creates CResourceList object to contain the designated
// resources for the given device.
// INPUT:
//      pDevice -- the device
//      ResType  -- what type of resource
//      LogConfType -- what type of logconf
// OUTPUT:
//      NONE.
//
// This function may throw CMemoryException
//
CResourceList::CResourceList(
    CDevice* pDevice,
    RESOURCEID ResType,
    ULONG LogConfType,
    ULONG AltLogConfType
    )
{
    ASSERT(ResType_All != ResType);
    ASSERT(BOOT_LOG_CONF == LogConfType ||
           FORCED_LOG_CONF == LogConfType ||
           ALLOC_LOG_CONF == LogConfType);
    ASSERT(pDevice);

    LOG_CONF lc;
    RES_DES rd, rdPrev;
    rdPrev;
    RESOURCEID ResId;
    BOOL Forced;
    CMachine* pMachine = pDevice->m_pMachine;
    ASSERT(pMachine);

    rdPrev = 0;

    // even though we have a valid logconf, it does not mean
    // GetNextResDes would succeed because the ResType is not
    // ResType_All.
    if (pMachine->CmGetFirstLogConf(pDevice->GetDevNode(), &lc, LogConfType)) 
    {
        if (pMachine->CmGetNextResDes(&rd, lc, ResType, &ResId)) 
        {
            ULONG DataSize;
            DWORDLONG dlBase, dlLen;
            
            do 
            {
                DataSize = pMachine->CmGetResDesDataSize(rd);

                if (DataSize) 
                {
                    BufferPtr<BYTE> DataPtr(DataSize);

                    if (pMachine->CmGetResDesData(rd, DataPtr, DataSize)) 
                    {
                        // need this to use a different image overlay for
                        // forced allocated resource
                        Forced = pMachine->CmGetFirstLogConf(pDevice->GetDevNode(),
                                NULL, FORCED_LOG_CONF);
                        if (ExtractResourceValue(ResType, DataPtr, &dlBase, &dlLen)) 
                        {
                            SafePtr<CResource> ResPtr;
                            CResource* pRes;
    
                            pRes = new CResource(pDevice, ResType, dlBase,
                                                 dlLen, Forced, FALSE);
                                                 
                            ResPtr.Attach(pRes);
                            InsertResourceToList(pRes);
                            ResPtr.Detach();
                        }
                    }
                }
                
                if (rdPrev)
                    pMachine->CmFreeResDesHandle(rdPrev);
                
                rdPrev = rd;
            }while (pMachine->CmGetNextResDes(&rd, rdPrev, ResType, &ResId));
            
            //free the last resource descriptor handle
            pMachine->CmFreeResDesHandle(rd);
        }

        pMachine->CmFreeLogConfHandle(lc);
    }
}



// This function creates CResourceList object to contain the designated
// resources for the given machine.
// INPUT:
//  pMachine -- the machine
//  ResType  -- what type of resource
//  LogConfType -- what type of logconf
// OUTPUT:
//  NONE.
//
// This function may throw CMemoryException
//
CResourceList::CResourceList(
    CMachine* pMachine,
    RESOURCEID ResType,
    ULONG LogConfType,
    ULONG AltLogConfType
    )
{
    ASSERT(ResType_All != ResType);
    ASSERT(BOOT_LOG_CONF == LogConfType ||
           FORCED_LOG_CONF == LogConfType ||
           ALLOC_LOG_CONF == LogConfType ||
           ALL_LOG_CONF == LogConfType);

    ASSERT(pMachine);

    if (pMachine->GetNumberOfDevices()) 
    {
        ASSERT(pMachine->m_pComputer && pMachine->m_pComputer->GetChild());

        CDevice* pFirstDevice;

        pFirstDevice = pMachine->m_pComputer->GetChild();
        CreateSubtreeResourceList(pFirstDevice, ResType, LogConfType, AltLogConfType);
    }
}

//
// This function extracts resource value from the provided buffer
//
// INPUT:
//  ResType     -- resource type the data contain
//  pData       -- the raw data
//  pdlBase     -- buffer to hold the base of the value
//  pdlLen      -- buffer to hold the length of the value
//
// OUTPUT:
//  TRUE if this is a valid resource descriptor or FALSE if we should ignore it.
//  
// NOTE:
//  If the return value is FALSE then pdlBase and pdlLen are not filled in.
//
BOOL
CResourceList::ExtractResourceValue(
    RESOURCEID ResType,
    PVOID pData,
    DWORDLONG* pdlBase,
    DWORDLONG* pdlLen
    )
{
    BOOL bValidResDes = TRUE;

    ASSERT(pData && pdlBase && pdlLen);

    switch (ResType)
    {
    case ResType_Mem:
        if (pMemResData(pData)->MEM_Header.MD_Alloc_Base <= pMemResData(pData)->MEM_Header.MD_Alloc_End) {
        
            *pdlBase = pMemResData(pData)->MEM_Header.MD_Alloc_Base;
            *pdlLen = pMemResData(pData)->MEM_Header.MD_Alloc_End -
                *pdlBase + 1;
        } else {
            //
            // If base > end then ignore this resource descriptor
            //
            *pdlBase = 0;
            *pdlLen = 0;
            bValidResDes = FALSE;
        }
        break;
            
    case ResType_IRQ:
        *pdlBase = pIRQResData(pData)->IRQ_Header.IRQD_Alloc_Num;
        // IRQ len is always 1
        *pdlLen = 1;
        break;
            
    case ResType_DMA:
        *pdlBase = pDMAResData(pData)->DMA_Header.DD_Alloc_Chan;
        // DMA len is always 1
        *pdlLen = 1;
        break;
            
    case ResType_IO:
        if (pIOResData(pData)->IO_Header.IOD_Alloc_Base <= pIOResData(pData)->IO_Header.IOD_Alloc_End) {
        
            *pdlBase = pIOResData(pData)->IO_Header.IOD_Alloc_Base;
            *pdlLen = pIOResData(pData)->IO_Header.IOD_Alloc_End -
                *pdlBase + 1;
        } else {
            //
            // If base > end then ignore this resource descriptor
            //
            *pdlBase = 0;
            *pdlLen = 0;
            bValidResDes = FALSE;
        }
        break;
          
    default:
        ASSERT(FALSE);
        *pdlBase = 0;
        *pdlLen = 0;
        break;
    }

    return bValidResDes;
}


//
//This function creates resources for the given subtree rooted at
//the given device
//
//INPUT:
//  pDevice -- the root device of the subtree
//  ResType -- resource type to be created
//  LogConfType -- logconf type to be created from
//
//OUTPUT:
//  NONE
//
// This function may throw CMemoryException
//
void
CResourceList::CreateSubtreeResourceList(
    CDevice* pDeviceStart,
    RESOURCEID ResType,
    ULONG LogConfType,
    ULONG AltLogConfType
    )
{
    LOG_CONF lc;
    RES_DES rd, rdPrev;
    RESOURCEID ResId;
    BOOL Forced;
    CMachine* pMachine = pDeviceStart->m_pMachine;
    ASSERT(pMachine);

    while (pDeviceStart) 
    {
        //
        // We will try to get a LogConf for either the LogConfType (which defaults to
        // ALLOC_LOG_CONF) or the AltLogConfType (which defaults to BOOT_LOG_CONF).
        // We need to do this because on Win2000 a device that only has a BOOT_LOG_CONF
        // will still consume those resources, even if it does not have an ALLOC_LOG_CONF.
        // So we need to first check the ALLOC_LOG_CONF and if that fails check the
        // BOOT_LOG_CONF.
        //
        if (pMachine->CmGetFirstLogConf(pDeviceStart->GetDevNode(), &lc, LogConfType) ||
            pMachine->CmGetFirstLogConf(pDeviceStart->GetDevNode(), &lc, AltLogConfType)) 
        {
            rdPrev = 0;

            if (pMachine->CmGetNextResDes(&rd, lc, ResType, &ResId)) 
            {
                ULONG DataSize;
                DWORDLONG dlBase, dlLen;

                do 
                {
                    DataSize = pMachine->CmGetResDesDataSize(rd);

                    if (DataSize) 
                    {
                        // need this to use a different image overlay for
                        // forced allocated resource
                        Forced = pMachine->CmGetFirstLogConf(pDeviceStart->GetDevNode(),
                                NULL, FORCED_LOG_CONF);
                                
                        BufferPtr<BYTE> DataPtr(DataSize);
                        
                        if (pMachine->CmGetResDesData(rd, DataPtr, DataSize)) 
                        {
                            if (ExtractResourceValue(ResType, DataPtr, &dlBase, &dlLen))
                            {
                                SafePtr<CResource> ResPtr;
                                
                                CResource* pRes;
                                
                                pRes = new CResource(pDeviceStart, ResType, dlBase,
                                        dlLen, Forced, FALSE);
                                ResPtr.Attach(pRes);
                                InsertResourceToList(pRes);
                                ResPtr.Detach();
                            }
                        }
                    }
                    
                    if (rdPrev)
                        pMachine->CmFreeResDesHandle(rdPrev);
                    
                    rdPrev = rd;
                    
                }while (pMachine->CmGetNextResDes(&rd, rdPrev, ResType, &ResId));
                
                //free the last resource descriptor handle
                pMachine->CmFreeResDesHandle(rd);
            }
            
            pMachine->CmFreeLogConfHandle(lc);
        }
        
        if (pDeviceStart->GetChild())
            CreateSubtreeResourceList(pDeviceStart->GetChild(), ResType, LogConfType, AltLogConfType);
        
        pDeviceStart = pDeviceStart->GetSibling();
    }
}


// This function creates a resource tree
// INPUT:
//  ppResRoot   -- buffer to receive the tree root
//
BOOL
CResourceList::CreateResourceTree(
    CResource** ppResRoot
    )
{
    ASSERT(ppResRoot);

    *ppResRoot = NULL;

    if (!m_listRes.IsEmpty()) 
    {
        POSITION pos = m_listRes.GetHeadPosition();
        CResource* pResFirst;

        pResFirst = m_listRes.GetNext(pos);
        *ppResRoot = pResFirst;

        while (NULL != pos) 
        {
            CResource* pRes = m_listRes.GetNext(pos);
            InsertResourceToTree(pRes, pResFirst, TRUE);
        }
    }
    
    return TRUE;
}

BOOL
CResourceList::InsertResourceToTree(
    CResource* pRes,
    CResource* pResRoot,
    BOOL       ForcedInsert
    )
{
    CResource* pResLast;

    while (pResRoot) 
    {
        if (pRes->EnclosedBy(*pResRoot)) 
        {
            // this resource is either the pResRoot child or grand child
            // figure out which one it is
            if (!pResRoot->GetChild()) 
            {
                pResRoot->SetChild(pRes);
                pRes->SetParent(pResRoot);
            }
            
            else if (!InsertResourceToTree(pRes, pResRoot->GetChild(), FALSE)) 
            {
                // the Resource is not a grand child of pResRoot.
                // search for the last child of pResRoot
                CResource* pResSibling;
                pResSibling = pResRoot->GetChild();

                while (pResSibling->GetSibling()) 
                    pResSibling = pResSibling->GetSibling();
                
                pResSibling->SetSibling(pRes);
                pRes->SetParent(pResRoot);
            }
            
            return TRUE;
        }
        
        pResLast = pResRoot;
        pResRoot = pResRoot->GetSibling();
    }
    
    if (ForcedInsert) 
    {
        // when we reach here, pResLast is the last child
        pResLast->SetSibling(pRes);
        pRes->SetParent(pResLast->GetParent());
        return TRUE;
    }
    
    return FALSE;
}


CResourceList::~CResourceList()
{
    if (!m_listRes.IsEmpty()) 
    {
        POSITION pos = m_listRes.GetHeadPosition();

        while (NULL != pos) 
        {
            delete m_listRes.GetNext(pos);
        }
        
        m_listRes.RemoveAll();
    }
}

BOOL
CResourceList::GetFirst(
    CResource** ppRes,
    PVOID&      Context
    )
{
    ASSERT(ppRes);

    if (!m_listRes.IsEmpty()) 
    {
        POSITION pos = m_listRes.GetHeadPosition();
        *ppRes = m_listRes.GetNext(pos);
        Context = pos;
        return TRUE;
    }
    
    Context = NULL;
    *ppRes = NULL;

    return FALSE;
}

BOOL
CResourceList::GetNext(
    CResource** ppRes,
    PVOID&      Context
    )
{
    ASSERT(ppRes);

    POSITION pos = (POSITION)Context;

    if (NULL != pos) 
    {
        *ppRes = m_listRes.GetNext(pos);
        Context = pos;
        return TRUE;
    }
    
    *ppRes = NULL;
    return FALSE;
}

//
// This function inserts the given resource to class's resource list
// The resources are kept in accending sorted order
void
CResourceList::InsertResourceToList(
    CResource* pRes
    )
{
    POSITION pos;
    CResource* pSrc;
    DWORDLONG dlBase, dlLen;
    pRes->GetValue(&dlBase, &dlLen);

    pos = m_listRes.GetHeadPosition();

    while (NULL != pos) 
    {
        POSITION posSave = pos;
        pSrc = m_listRes.GetNext(pos);

        if (*pRes <= *pSrc) 
        {
            m_listRes.InsertBefore(posSave, pRes);
            return;
        }
    }
    
    m_listRes.AddTail(pRes);
}


inline
CItemIdentifier*
CResource::CreateIdentifier()
{
    return new CResourceIdentifier(*this);
}
