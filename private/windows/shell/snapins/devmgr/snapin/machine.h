
#ifndef __MACHINE_H_
#define __MACHINE_H_

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    machine.h

Abstract:

    header file that declares CMachine, CDevInfoist and CMachineList classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

LRESULT CALLBACK dmNotifyWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef BOOL (*LPFNINSTALLDEVINST)(HWND hwndParent, LPCTSTR DeviceId, BOOL UpdateDriver, DWORD* pReboot);

class CDevice;
class CClass;
class CComputer;
class CLogConf;
class CResDes;
class CMachineList;
class CFolder;

#define DM_NOTIFY_TIMERID           0x444d4d44

#ifdef DEVL
#define LOGLASTERROR(FuntionName)   LogLastError(LPCTSTR FunctionName)
#define LOGMISCINFO(Info)           LogMiscInfo(Info)
#define LOGMACHINE()                LogObject()
#define LOGCLASS(Class)             LogObject(Class)
#define LOGDEVICE(Devcie)           LogObject(Device)
#else
#define LOGLASTERROR(FuntionName)
#define LOGMISCINFO(Info)
#define LOGMACHINE()
#define LOGCLASS(Class)
#define LOGDEVICE(Devcie)
#endif

#define     LOG_MASK_ERROR          0x00000001
#define     LOG_MASK_COMPUTER       0x00000100
#define     LOG_MASK_CLASS          0x00000200
#define     LOG_MASK_DEVICE         0x00000400
#define     LOG_MASK_OBJECTS        LOG_MASK_COMPUTER + LOG_MASK_CLASS + LOG_MASK_DEVICE
#define     LOG_MASK_DEVTREE        0x00000800
#define     LOG_MASK_CMAPI          0x00010000
#define     LOG_MASK_DIAPI          0x00020000
#define     LOG_MASK_MISC           0x00100000


//This class represents SETUPAPI's <Device Information List>
//
// WARNING !!!
// no copy constructor and assignment operator are provided ---
// DO NOT ASSIGN A CDevInfoList from one to another!!!!!
class CDevInfoList
{
public:

    CDevInfoList(HDEVINFO hDevInfo = INVALID_HANDLE_VALUE, HWND hwndParent = NULL)
            :m_hDevInfo(hDevInfo), m_hwndParent(hwndParent)
        {
        }
    virtual ~CDevInfoList()
        {
            if (INVALID_HANDLE_VALUE != m_hDevInfo)
                DiDestroyDeviceInfoList();
        }

    operator HDEVINFO()
        {
            return m_hDevInfo;
        }
    BOOL DiGetDeviceInfoListDetail(PSP_DEVINFO_LIST_DETAIL_DATA DetailData)
        {
           return SetupDiGetDeviceInfoListDetail(m_hDevInfo, DetailData);
        }
    BOOL DiOpenDeviceInfo(LPCTSTR DeviceID, HWND hwndParent, DWORD OpenFlags,
                          PSP_DEVINFO_DATA DevData)
        {
           return SetupDiOpenDeviceInfo(m_hDevInfo, DeviceID, hwndParent, OpenFlags,
                                        DevData);
        }
    BOOL DiEnumDeviceInfo(DWORD Index, PSP_DEVINFO_DATA DevData)
        {
            return SetupDiEnumDeviceInfo(m_hDevInfo, Index, DevData);
        }
    BOOL DiBuildDriverInfoList(PSP_DEVINFO_DATA DevData, DWORD DriverType)
        {
            return SetupDiBuildDriverInfoList(m_hDevInfo, DevData, DriverType);
        }
    BOOL DiEnumDriverInfo(PSP_DEVINFO_DATA DevData, DWORD DriverType, DWORD Index,
                          PSP_DRVINFO_DATA DrvData)
        {
            return  SetupDiEnumDriverInfo(m_hDevInfo, DevData, DriverType, Index, DrvData);
        }
    BOOL DiGetDriverInfoDetail(PSP_DEVINFO_DATA DevData, PSP_DRVINFO_DATA DrvData,
                               PSP_DRVINFO_DETAIL_DATA DrvDetailData,
                               DWORD DrvDetailDataSize,
                               PDWORD RequiredSize)
        {
            return SetupDiGetDriverInfoDetail(m_hDevInfo, DevData, DrvData,
                                              DrvDetailData,
                                              DrvDetailDataSize,
                                              RequiredSize);
        }
    BOOL DiDestroyDriverInfoList(PSP_DEVINFO_DATA DevData, DWORD DriverType)
        {
            return SetupDiDestroyDriverInfoList(m_hDevInfo, DevData, DriverType);
        }
    BOOL DiCallClassInstaller(DI_FUNCTION InstallFunction, PSP_DEVINFO_DATA DevData)
        {
            return SetupDiCallClassInstaller(InstallFunction, m_hDevInfo, DevData);
        }
    BOOL DiRemoveDevice(PSP_DEVINFO_DATA DevData)
        {
            return SetupDiRemoveDevice(m_hDevInfo, DevData);
        }
    BOOL DiChangeState(PSP_DEVINFO_DATA DevData)
        {
            return SetupDiChangeState(m_hDevInfo, DevData);
        }
    HKEY DiOpenDevRegKey(PSP_DEVINFO_DATA DevData, DWORD Scope, DWORD HwProfile,
                         DWORD KeyType, REGSAM samDesired)
        {
            return  SetupDiOpenDevRegKey(m_hDevInfo, DevData, Scope, HwProfile,
                                  KeyType, samDesired);
        }
    BOOL DiGetDeviceRegistryProperty(PSP_DEVINFO_DATA DevData, DWORD Property,
                                     PDWORD PropertyDataType, PBYTE PropertyBuffer,
                                     DWORD PropertyBufferSize, PDWORD RequiredSize)
        {
            return SetupDiGetDeviceRegistryProperty(m_hDevInfo, DevData,
                                              Property, PropertyDataType,
                                              PropertyBuffer,
                                              PropertyBufferSize,
                                              RequiredSize
                                              );
        }
    BOOL DiGetDeviceInstallParams(PSP_DEVINFO_DATA DevData,
                                  PSP_DEVINSTALL_PARAMS DevInstParams)
        {
            return SetupDiGetDeviceInstallParams(m_hDevInfo, DevData, DevInstParams);
        }
    BOOL DiGetClassInstallParams(PSP_DEVINFO_DATA DevData,
                                 PSP_CLASSINSTALL_HEADER ClassInstallHeader,
                                 DWORD ClassInstallParamsSize,
                                 PDWORD RequiredSize)
        {
            return SetupDiGetClassInstallParams(m_hDevInfo, DevData,
                                          ClassInstallHeader,
                                          ClassInstallParamsSize,
                                          RequiredSize);
        }
    BOOL DiSetDeviceInstallParams(PSP_DEVINFO_DATA DevData,
                                PSP_DEVINSTALL_PARAMS DevInstParams)
        {
             return SetupDiSetDeviceInstallParams(m_hDevInfo, DevData, DevInstParams);
        }
    BOOL DiSetClassInstallParams(PSP_DEVINFO_DATA DevData,
                                 PSP_CLASSINSTALL_HEADER ClassInstallHeader,
                                 DWORD ClassInstallParamsSize)
        {
            return SetupDiSetClassInstallParams(m_hDevInfo, DevData,
                                     ClassInstallHeader,
                                     ClassInstallParamsSize);
        }
    BOOL DiGetClassDevPropertySheet(PSP_DEVINFO_DATA DevData,
                                    LPPROPSHEETHEADER PropertySheetHeader,
                                    DWORD PagesAllowed,
                                    DWORD Flags)
        {
            return  SetupDiGetClassDevPropertySheets(m_hDevInfo, DevData,
                                        PropertySheetHeader, PagesAllowed,
                                        NULL, Flags);
        }

    BOOL DiGetExtensionPropSheetPage(PSP_DEVINFO_DATA DevData,
                                     LPFNADDPROPSHEETPAGE pfnAddPropSheetPage,
                                     DWORD PageType,
                                     LPARAM lParam
                                     );

    BOOL DiGetSelectedDriver(PSP_DEVINFO_DATA DevData, PSP_DRVINFO_DATA DriverInfoData)
        {
            return SetupDiGetSelectedDriver(m_hDevInfo, DevData, DriverInfoData);
        }
    BOOL DiSetSelectedDriver(PSP_DEVINFO_DATA DevData, PSP_DRVINFO_DATA DriverInfoData)
        {
            return SetupDiSetSelectedDriver(m_hDevInfo, DevData, DriverInfoData);
        }
    BOOL DiEnumDeviceInterfaces(DWORD Index, LPGUID InterfaceGuid, PSP_DEVICE_INTERFACE_DATA InterfaceData)
        {
            return SetupDiEnumDeviceInterfaces(m_hDevInfo, NULL, InterfaceGuid, Index, InterfaceData);
        }
    BOOL DiGetInterfaceDetailData(PSP_DEVICE_INTERFACE_DATA pInterfaceData,
                                        PSP_DEVICE_INTERFACE_DETAIL_DATA pDetailData,
                                        DWORD Size, DWORD* pRequiredSize)
        {
            return SetupDiGetDeviceInterfaceDetail(m_hDevInfo, pInterfaceData,
                                                   pDetailData, Size,
                                                   pRequiredSize, NULL);
        }
    BOOL DiTurnOnDiFlags(PSP_DEVINFO_DATA DevData, DWORD FlagMask );
    BOOL DiTurnOffDiFlags(PSP_DEVINFO_DATA DevData, DWORD FlagsMask);
    BOOL DiTurnOnDiExFlags(PSP_DEVINFO_DATA DevData, DWORD FlagMask );
    BOOL DiTurnOffDiExFlags(PSP_DEVINFO_DATA DevData, DWORD FlagsMask);
    BOOL InstallDevInst(HWND hwndParent, LPCTSTR DeviceID, BOOL UpdateDriver, DWORD* pReboot);
    DWORD DiGetFlags(PSP_DEVINFO_DATA DevData = NULL);
    DWORD DiGetExFlags(PSP_DEVINFO_DATA DevData = NULL);
    void DiDestroyDeviceInfoList();
    BOOL DiGetDeviceDescriptionString(PSP_DEVINFO_DATA DevData, String& str);
    BOOL DiGetDeviceDescription(PSP_DEVINFO_DATA DevData, TCHAR* pBuffer, DWORD Size, DWORD* pRequriedSize = NULL);
    BOOL DiGetDeviceMFGString(PSP_DEVINFO_DATA DevData, String& str);
    BOOL DiGetDeviceMFGString(PSP_DEVINFO_DATA DevData, TCHAR* pBuffer, DWORD Size, DWORD* pRequiredSize = NULL);
    BOOL DiGetDeviceIDString(PSP_DEVINFO_DATA DevData, String& str);
    BOOL DiGetDeviceIDString(PSP_DEVINFO_DATA DevData, TCHAR* pBuffer, DWORD Size, DWORD* pRequiredSize = NULL);
    HWND OwnerWindow()
        {
            return m_hwndParent;
        }
protected:
    HDEVINFO    m_hDevInfo;
    HWND        m_hwndParent;

private:
    CDevInfoList(const CDevInfoList&);
    CDevInfoList& operator=(const CDevInfoList&);
};

class CMachine : public CDevInfoList
{
public:
    CMachine(LPCTSTR MachineName = NULL);

    virtual ~CMachine();
    operator HMACHINE()
        {
            return m_hMachine;
        }
    LPCTSTR GetMachineDisplayName()
        {
            return (LPCTSTR)m_strMachineDisplayName;
        }
    LPCTSTR GetMachineFullName()
        {
            return (LPCTSTR)m_strMachineFullName;
        }
    LPCTSTR GetRemoteMachineFullName()
        {
            return m_IsLocal ? NULL : (LPCTSTR)m_strMachineFullName;
        }
    HMACHINE GetHMachine()
        {
            return m_hMachine;
        }
    BOOL LoadStringWithMachineName(int StringId, LPTSTR Buffer, DWORD* BufferLen);

    BOOL IsLocal()
        {
            return m_IsLocal;
        }
    UINT GetActivePropSheetCount();
    BOOL AttachFolder(CFolder* pFolder);
    void DetachFolder(CFolder* pFolder);
    BOOL IsFolderAttached(CFolder* pFolder);
    BOOL AttachPropertySheet(HWND hwndPropertySheet);
    void DetachPropertySheet(HWND hwndPropertySheet);
    BOOL EnuemerateDevice(int Index, CDevice** ppDevice);

    BOOL Initialize(HWND hwndParent = NULL, LPCTSTR DeviceId = NULL);

    DWORD GetNumberOfClasses() const
        {
            return m_listClass.GetCount();
        }
    DWORD GetNumberOfDevices() const
        {
            return (DWORD)m_listDevice.GetCount();
        }
    BOOL GetFirstClass(CClass** ppClass, PVOID& Context);
    BOOL GetNextClass(CClass** ppClass, PVOID&  Context);
    BOOL GetFirstDevice(CDevice** ppDevice,  PVOID&  Context);
    BOOL GetNextDevice(CDevice** ppDevice, PVOID&  Context);
    CDevice* DevNodeToDevice(DEVNODE dn);
    CDevice* DeviceIDToDevice(LPCTSTR DeviceID);
    CClass* ClassGuidToClass(LPGUID ClassGuid);
    BOOL Reenumerate();
    BOOL Refresh();
    int GetComputerIconIndex()
    {
        return m_ComputerIndex;
    }
    int GetResourceIconIndex()
    {
        return m_ResourceIndex;
    }
    BOOL GetDigitalSigner(LPTSTR FullInfPath, String& DigitalSigner);
    BOOL DoNotCreateDevice(SC_HANDLE SCMHandle, LPGUID ClassGuid, DEVINST DevInst);
    HDEVINFO DiCreateDeviceInfoList(LPGUID ClassGuid, HWND hwndParent)
        {
            return SetupDiCreateDeviceInfoListEx(ClassGuid, hwndParent,
                                GetRemoteMachineFullName(), NULL);
        }
    HDEVINFO DiCreateDeviceInterfaceList(LPGUID InterfaceGuid, LPCTSTR DeviceId, HWND hwndParent)
        {
            return SetupDiGetClassDevsEx(InterfaceGuid, DeviceId, m_hwndParent, DIGCF_DEVICEINTERFACE,
                                         NULL, GetRemoteMachineFullName(), NULL);
        }

    HDEVINFO DiGetClassDevs(LPGUID ClassGuid, LPCTSTR Enumerator, HWND hwndParent, DWORD Flags)
        {
            return SetupDiGetClassDevsEx(ClassGuid, Enumerator, hwndParent,
                                     Flags, NULL, GetRemoteMachineFullName(), NULL
                                     );
        }
    HIMAGELIST DiGetClassImageList()
        {
            return m_ImageListData.ImageList;
        }
    BOOL DiBuildClassInfoList(DWORD Flags, LPGUID ClassGuid,
                              DWORD ClassGuidListSize, PDWORD RequiredSize)
        {
            return SetupDiBuildClassInfoListEx(Flags, ClassGuid,
                                     ClassGuidListSize, RequiredSize,
                                     GetRemoteMachineFullName(), NULL);
        }
    BOOL DiGetClassDescription(LPGUID ClassGuid, LPTSTR ClassDescription,
                               DWORD ClassDescriptionSize, PDWORD RequiredSize)
        {
            return SetupDiGetClassDescriptionEx(ClassGuid, ClassDescription,
                                        ClassDescriptionSize,
                                        RequiredSize,
                                        GetRemoteMachineFullName(),
                                        NULL);
        }
    HKEY DiOpenClassRegKey(LPGUID ClassGuid, REGSAM samDesired, DWORD Flags)
        {
            return SetupDiOpenClassRegKeyEx(ClassGuid, samDesired, Flags,
                                    GetRemoteMachineFullName(), NULL);
        }
    BOOL DiGetHwProfileList(PDWORD HwProfileList, DWORD HwProfileListSize,
                            PDWORD RequiredSize, PDWORD CurrentIndex)
        {
            return  SetupDiGetHwProfileListEx(HwProfileList, HwProfileListSize,
                                       RequiredSize, CurrentIndex,
                                       GetRemoteMachineFullName(), NULL);
        }
    BOOL DiLoadClassIcon(LPGUID ClassGuid, HICON* LargeIcon, PINT MiniIconIndex)
        {
            return SetupDiLoadClassIcon(ClassGuid, LargeIcon, MiniIconIndex);
        }
    BOOL DiGetClassImageList(PSP_CLASSIMAGELIST_DATA pImageListData)
        {
            return SetupDiGetClassImageListEx(pImageListData,
                                   GetRemoteMachineFullName(), NULL);
        }
    BOOL DiDestroyClassImageList(PSP_CLASSIMAGELIST_DATA pImageListData)
        {
            return SetupDiDestroyClassImageList(pImageListData);
        }

    BOOL DiGetClassImageIndex(LPGUID ClassGuid, PINT ImageIndex)
        {
            return SetupDiGetClassImageIndex(&m_ImageListData, ClassGuid, ImageIndex);
        }
    int  DiDrawMiniIcon(HDC hdc, RECT rc, int IconIndex, DWORD Flags)
        {
            return SetupDiDrawMiniIcon(hdc, rc, IconIndex, Flags);
        }
    BOOL DiClassNameFromGuid(LPGUID ClassGuid, LPTSTR ClassName,
                             DWORD ClassNameSize, PDWORD RequiredSize)
        {
            return SetupDiClassNameFromGuidEx(ClassGuid, ClassName,
                                      ClassNameSize, RequiredSize,
                                      GetRemoteMachineFullName(), NULL);
        }
    BOOL DiGetHwProfileFriendlyName(DWORD HwProfile, LPTSTR FriendlyName,
                                    DWORD FriendlyNameSize,
                                    PDWORD RequiredSize)

        {
            return SetupDiGetHwProfileFriendlyNameEx(HwProfile, FriendlyName,
                                             FriendlyNameSize,
                                             RequiredSize,
                                             GetRemoteMachineFullName(),
                                             NULL);
        }
    BOOL DiGetClassFriendlyNameString(LPGUID Guid, String& strClass);
    BOOL DiDestroyDeviceInfoList(HDEVINFO hDevInfo)
        {
            return SetupDiDestroyDeviceInfoList(hDevInfo);
        }

///////////////////////////////////////////////////////////////////////////
//// Configuration Manager APIs
////
    CONFIGRET GetLastCR()
        {
            return m_LastCR;
        }
    BOOL CmGetConfigFlags(DEVNODE dn, DWORD* pFlags);
    BOOL CmGetCapabilities(DEVNODE dn, DWORD* pCapabilities);
    BOOL CmGetDeviceIDString(DEVNODE dn, String& str);
    BOOL CmGetDescriptionString(DEVNODE dn, String& str);
    BOOL CmGetMFGString(DEVNODE dn, String& str);
    BOOL CmGetProviderString(DEVNODE dn, String& str);
    BOOL CmGetDriverDateString(DEVNODE dn, String& str);
    BOOL CmGetDriverDateData(DEVNODE dn, FILETIME *ft);
    BOOL CmGetDriverVersionString(DEVNODE dn, String& str);
    BOOL CmGetClassGuid(DEVNODE dn, GUID& Guid);
    BOOL CmGetHardwareIDs(DEVNODE dn, PVOID Buffer, ULONG* BufferLen);
    BOOL CmGetCompatibleIDs(DEVNODE dn, PVOID Buffer, ULONG* BufferLen);
    BOOL CmGetStatus(DEVNODE dn, DWORD* pProblem, DWORD* pStatus);
    BOOL CmGetKnownLogConf(DEVNODE dn, LOG_CONF* plc, DWORD* plcType);
    BOOL CmReenumerate(DEVNODE dn, ULONG Flags);
    BOOL CmGetHwProfileFlags(DEVNODE dn, ULONG Profile, ULONG* pFlags);
    BOOL CmGetHwProfileFlags(LPCTSTR DeviceID, ULONG Profile, ULONG* pFlags);
    BOOL CmSetHwProfileFlags(DEVNODE dn, ULONG Profile, ULONG Flags);
    BOOL CmSetHwProfileFlags(LPCTSTR DeviceID, ULONG Profile, ULONG Flags);
    BOOL CmGetCurrentHwProfile(ULONG* phwpf);
    BOOL CmGetHwProfileInfo(int Index, PHWPROFILEINFO pHwProfileInfo);
    BOOL CmGetBusGuid(DEVNODE dn, LPGUID Guid);
    BOOL CmGetBusGuidString(DEVNODE dn, String& str);
    DEVNODE CmGetParent(DEVNODE dn);
    DEVNODE CmGetChild(DEVNODE dn);
    DEVNODE CmGetSibling(DEVNODE dn);
    DEVNODE CmGetRootDevNode();
    BOOL CmHasResources(DEVNODE dn);
    BOOL CmHasDrivers(DEVNODE dn);
    DWORD CmGetResDesDataSize(RES_DES rd);
    BOOL CmGetResDesData(RES_DES rd, PVOID pData, ULONG DataSize);

    BOOL CmGetNextResDes(PRES_DES prdNext, RES_DES rd, RESOURCEID ForResource,
                          PRESOURCEID pTheResource);
    void CmFreeResDesHandle(RES_DES rd);
    void CmFreeResDes(PRES_DES prdPrev, RES_DES rd);
    void CmFreeLogConfHandle(LOG_CONF lc);
    int CmGetNumberOfBasicLogConf(DEVNODE dn);
    BOOL CmGetFirstLogConf(DEVNODE dn, LOG_CONF* plc, ULONG Type);
    BOOL CmGetNextLogConf(LOG_CONF* plcNext, LOG_CONF lcRef, ULONG Type);
    ULONG CmGetArbitratorFreeDataSize(DEVNODE dn, RESOURCEID ResType);
    BOOL CmGetArbitratorFreeData(DEVNODE dn, PVOID pBuffer, ULONG BufferSize,
                                 RESOURCEID ResType);
    BOOL CmTestRangeAvailable(RANGE_LIST RangeList, DWORDLONG dlBase,
                              DWORDLONG dlEnd);
    void CmDeleteRange(RANGE_LIST RangeList, DWORDLONG dlBase, DWORDLONG dlLen);
    BOOL CmGetFirstRange(RANGE_LIST RangeList, DWORDLONG* pdlBase,
                         DWORDLONG* pdlLen, RANGE_ELEMENT* pre);
    BOOL CmGetNextRange(RANGE_ELEMENT* pre, DWORDLONG* pdlBase, DWORDLONG* pdlLen);
    void CmFreeRangeList(RANGE_LIST RangeList);
    BOOL CmGetDeviceIdListSize(LPCTSTR Fileter, ULONG* Size, ULONG Flags);
    BOOL CmGetDeviceIdList(LPCTSTR Filter, TCHAR* Buffer, ULONG BufferSize, ULONG Flags);

    BOOL EnableRefresh(BOOL fEnable);
    BOOL ScheduleRefresh();
    void Lock()
        {
            EnterCriticalSection(&m_CriticalSection);
        }
    void Unlock()
        {
            LeaveCriticalSection(&m_CriticalSection);
        }
    CComputer*  m_pComputer;
    UINT  m_msgRefresh;
    static DWORD WaitDialogThread(PVOID Parameter);
#ifdef DEVL
    BOOL LogLastError(LPCTSTR FunctionName)
        {
            if(m_LoggingMask & LOG_MASK_ERROR)
                return m_LogFile.LogLastError(FunctionName);
            return TRUE;
        }
    BOOL LogMiscInfo(LPCTSTR Info)
        {
            if (m_LoggingMask & LOG_MASK_MISC)
                return m_LogFile.Log(Info);
            return TRUE;
        }
        BOOL LogObject();
        BOOL LogObject(CClass* pClass);
        BOOL LogObject(CDevice* pDevice);
#endif

private:

        // no copy constructor and no assigment operator
        CMachine(const CMachine& MachineSrc);
        CMachine& operator=(const CMachine& MachineSrc);

        BOOL BuildClassesFromGuidList(LPGUID GuidList, DWORD Guids);
        CONFIGRET CmGetRegistryProperty(DEVNODE dn, ULONG Property,
                                        PVOID pBuffer,
                                        ULONG* BufferSize);
        CONFIGRET CmGetRegistrySoftwareProperty(DEVNODE dn, LPCTSTR ValueName,
                                        PVOID pBuffer, ULONG* pBufferSize);
        void CreateDeviceTree(CDevice* pParent, CDevice* pSibling, DEVNODE dn);
        BOOL CreateClassesAndDevices(LPCTSTR DeviceId = NULL);
        void DestroyClassesAndDevices();
        BOOL CreateNotifyWindow();
        BOOL pGetOriginalInfName(LPTSTR InfName, String& OriginalInfName);
#if DBG
    void DumpClassDevices();
    void DumpDeviceTree();
    void DumpDeviceSubtree(BOOL HasParent, LPCTSTR Insert, CDevice* pDevice);
    void DumpDevNodeTree();
    void DumpDevNodeSubtree(LPCTSTR Insert, DEVNODE dn);
#endif
        String m_strMachineDisplayName;
        String m_strMachineFullName;
        HMACHINE m_hMachine;
        CONFIGRET   m_LastCR;
        CList<CClass*, CClass*> m_listClass;
        SP_CLASSIMAGELIST_DATA  m_ImageListData;
        int             m_ComputerIndex;
        int             m_ResourceIndex;
        CList<CDevice*, CDevice*> m_listDevice;
        DWORD           m_Flags;
        BOOL            m_Initialized;
        BOOL            m_IsLocal;
        HWND            m_hwndNotify;
        CList<CFolder*, CFolder*> m_listFolders;
        int             m_RefreshDisableCounter;
        BOOL            m_RefreshPending;
        BOOL            m_ShowNonPresentDevices;
        CRITICAL_SECTION m_CriticalSection;
        CList<HWND, HWND> m_listPropertySheets;
#ifdef DEVL
        DWORD           m_LoggingMask;
        CLogFile        m_LogFile;
#endif
};


class CMachineList
{
public:
    CMachineList() {};
    ~CMachineList();
    BOOL CreateMachine(HWND hwndParent, LPCTSTR MachineName, CMachine** ppMachine);
    CMachine* FindMachine(LPCTSTR MachineName);
private:
    CMachineList(const CMachineList& MachineListSrc);
    CMachineList& operator=(const CMachineList& MachineListSrc);
    CList<CMachine*, CMachine*> m_listMachines;
};
#endif  //__MACHINE_H_
