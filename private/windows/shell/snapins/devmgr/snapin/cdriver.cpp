/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    CDriver.cpp

Abstract:

    This module implements CDriver and CService classes

Author:

    William Hsieh (williamh) created

Revision History:


--*/

#include "devmgr.h"
#include "cdriver.h"

const TCHAR*  tszStringFileInfo = TEXT("StringFileInfo\\%04X%04X\\");
const TCHAR*  tszFileVersion = TEXT("FileVersion");
const TCHAR*  tszLegalCopyright = TEXT("LegalCopyright");
const TCHAR*  tszCompanyName = TEXT("CompanyName");
const TCHAR*  tszTranslation = TEXT("VarFileInfo\\Translation");
const TCHAR*  tszStringFileInfoDefault = TEXT("StringFileInfo\\040904B0\\");


BOOL
CDriver::Create(
    CDevice* pDevice,
    PSP_DRVINFO_DATA pDrvInfoData
    )
{
    SP_DRVINFO_DATA DrvInfoData;
    
    ASSERT(pDevice);
    
    m_pDevice = pDevice;
    m_OnLocalMachine  = pDevice->m_pMachine->IsLocal();
    
    if (!m_OnLocalMachine)
    {
        CreateFromService(pDevice);
        return m_listDriverFile.GetCount();
    }

    SP_DEVINSTALL_PARAMS DevInstParams;
    CMachine* pMachine = m_pDevice->m_pMachine;
    
    ASSERT(pMachine);

    DevInstParams.cbSize = sizeof(DevInstParams);
    
    if (!pDrvInfoData)
    {
        // reset all fields to default, 0.
        memset(&DrvInfoData, 0, sizeof(DrvInfoData));
    
        // if no driver node is provided,  create one
        if (pMachine->DiGetDeviceInstallParams(*m_pDevice, &DevInstParams))
        {
            HKEY hKey;
            // open drvice's driver registry key
            hKey = pMachine->DiOpenDevRegKey(*m_pDevice, DICS_FLAG_GLOBAL,
                         0, DIREG_DRV, KEY_ALL_ACCESS);
            if (INVALID_HANDLE_VALUE != hKey)
            {
                DWORD regType;
                DWORD Len = sizeof(DevInstParams.DriverPath);
                CSafeRegistry regDrv(hKey);
                
                // get the inf path from the driver key
                if (regDrv.GetValue(REGSTR_VAL_INFPATH, &regType,
                            (PBYTE)DevInstParams.DriverPath,
                            &Len))
                {
                    DevInstParams.Flags |= DI_ENUMSINGLEINF;
                    DevInstParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
                    
                    if (pMachine->DiSetDeviceInstallParams(*m_pDevice,
                                       &DevInstParams) &&
                        pMachine->DiBuildDriverInfoList(*m_pDevice,
                                        SPDIT_CLASSDRIVER))
                    {
                        // find out the provider
                        Len  = sizeof(DrvInfoData.ProviderName);
                        
                        if (!regDrv.GetValue(REGSTR_VAL_PROVIDER_NAME, &regType,
                                     (PBYTE)DrvInfoData.ProviderName,
                                      &Len) || !Len)
                        {
                            DrvInfoData.ProviderName[0] = _T('\0');
                        }
                    
                        pDrvInfoData = &DrvInfoData;
                    }
                }
            }
        }

        if (pDrvInfoData &&
            pMachine->DiGetDeviceRegistryProperty(*m_pDevice, SPDRP_MFG,
                    NULL, (PBYTE)DrvInfoData.MfgName,
                    sizeof(DrvInfoData.MfgName), NULL) &&
            pMachine->DiGetDeviceRegistryProperty(*m_pDevice, SPDRP_DEVICEDESC,
                    NULL, (PBYTE)DrvInfoData.Description,
                    sizeof(DrvInfoData.Description), NULL))
        {
            DrvInfoData.cbSize = sizeof(DrvInfoData);
            DrvInfoData.DriverType = SPDIT_CLASSDRIVER;
            DrvInfoData.Reserved = 0;
            
            if (!pMachine->DiSetSelectedDriver(*m_pDevice, &DrvInfoData))
            {
                pDrvInfoData = NULL;
                pMachine->DiDestroyDriverInfoList(*pDevice, SPDIT_CLASSDRIVER);
            }
        }
    }

    if (pDrvInfoData)
    {
        HSPFILEQ hFileQueue;

        //
        // Get the full INF path
        //
        SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

        DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
        if (pMachine->DiGetDriverInfoDetail(*pDevice,
                                            pDrvInfoData,
                                            &DriverInfoDetailData,
                                            sizeof(DriverInfoDetailData),
                                            NULL
                                            ) ||
            (ERROR_INSUFFICIENT_BUFFER == GetLastError())) {

            pMachine->GetDigitalSigner(DriverInfoDetailData.InfFileName, m_DigitalSigner);
        }

        //
        // Get a list of all the files installed for this device
        //
        hFileQueue = SetupOpenFileQueue();
        
        if (INVALID_HANDLE_VALUE != hFileQueue)
        {
            DevInstParams.FileQueue = hFileQueue;
            DevInstParams.Flags |= DI_NOVCP;
            // we are about to assign a new file queue to the device info
            //
            if (pMachine->DiSetDeviceInstallParams(*pDevice, &DevInstParams) &&
                pMachine->DiCallClassInstaller(DIF_INSTALLDEVICEFILES, *pDevice))
            {
                DWORD ScanResult;
                SetupScanFileQueue(hFileQueue, SPQ_SCAN_USE_CALLBACK, NULL,
                           (PSP_FILE_CALLBACK)ScanQueueCallback,
                           (PVOID)this,
                           &ScanResult
                           );
                
                // dereference the file queue so that we can close it
                DevInstParams.FileQueue = NULL;
                DevInstParams.Flags &= ~DI_NOVCP;
                pMachine->DiSetDeviceInstallParams(*pDevice, &DevInstParams);
            }

            // close the file queue
            if (!SetupCloseFileQueue(hFileQueue))
            {
                OutputDebugString(TEXT("Failed to close file queue\n"));
            }
        }
        
        if (pDrvInfoData == &DrvInfoData)
        {
            pMachine->DiDestroyDriverInfoList(*pDevice, SPDIT_CLASSDRIVER);
        }
    }

    // if fail to the driver files from driver key,
    // try service
    if (m_listDriverFile.IsEmpty())
    {
        CreateFromService(pDevice);
    }

    return m_listDriverFile.GetCount();
}

void
CDriver::CreateFromService(
    CDevice* pDevice
    )
{
    TCHAR ServiceName[MAX_PATH];
    // get the device's SERVICE name
    if (pDevice->m_pMachine->DiGetDeviceRegistryProperty(*pDevice,
                     SPDRP_SERVICE,
                     NULL,
                     (PBYTE)ServiceName,
                     sizeof(ServiceName),
                     NULL
                     ))
    {

    SC_HANDLE hscManager = NULL;
    SC_HANDLE hscService = NULL;
    try
    {
        BOOL ComposePathNameFromServiceName = TRUE;
        // open default database on local machine for now
        hscManager = OpenSCManager(m_OnLocalMachine ? NULL : pDevice->m_pMachine->GetMachineFullName(),
                       NULL, GENERIC_READ);
        if (NULL != hscManager)
        {
        hscService =  OpenService(hscManager, ServiceName, GENERIC_READ);
        if (NULL != hscService)
        {
            QUERY_SERVICE_CONFIG qsc;
            DWORD BytesRequired;
            // first, probe for buffer size
            if (!QueryServiceConfig(hscService, NULL, 0, &BytesRequired) &&
             ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
            BufferPtr<BYTE> BufPtr(BytesRequired);
            LPQUERY_SERVICE_CONFIG pqsc;
            pqsc = (LPQUERY_SERVICE_CONFIG)(PBYTE)BufPtr;
            DWORD Size;
            if (QueryServiceConfig(hscService, pqsc, BytesRequired, &Size) &&
                pqsc->lpBinaryPathName && _T('\0') != pqsc->lpBinaryPathName[0])
            {
                ComposePathNameFromServiceName = FALSE;
                SafePtr<CDriverFile> DrvFilePtr;
                CDriverFile* pDrvFile = new CDriverFile();
                DrvFilePtr.Attach(pDrvFile);
                if (pDrvFile->Create(pqsc->lpBinaryPathName, m_OnLocalMachine))
                {
                AddDriverFile(pDrvFile);
                DrvFilePtr.Detach();
                }
            }
            }
            CloseServiceHandle(hscService);
            hscService = NULL;
        }
        CloseServiceHandle(hscManager);
        hscManager = NULL;
        }
        if (ComposePathNameFromServiceName)
        {
        TCHAR FullPathName[MAX_PATH + 1];
        TCHAR SysDir[MAX_PATH + 1];
        GetSystemDirectory(SysDir, ARRAYLEN(SysDir));
        lstrcpy(FullPathName, SysDir);
        lstrcat(FullPathName, TEXT("\\drivers\\"));
        lstrcat(FullPathName, ServiceName);
        lstrcat(FullPathName, TEXT(".sys"));
        SafePtr<CDriverFile> DrvFilePtr;
        CDriverFile* pDrvFile = new CDriverFile();
        DrvFilePtr.Attach(pDrvFile);
        if (pDrvFile->Create(FullPathName, m_OnLocalMachine))
        {
            AddDriverFile(pDrvFile);
            DrvFilePtr.Detach();
        }
        }
    }
    catch (CMemoryException* e)
    {
        if (hscService)
        CloseServiceHandle(hscService);
        if (hscManager)
        CloseServiceHandle(hscManager);
        throw;
    }
    }
}
CDriver::~CDriver()
{
    if (!m_listDriverFile.IsEmpty())
    {
    POSITION pos = m_listDriverFile.GetHeadPosition();
    while (NULL != pos) {
        CDriverFile* pDrvFile = m_listDriverFile.GetNext(pos);
        delete pDrvFile;
    }
    m_listDriverFile.RemoveAll();
    }
}

BOOL
CDriver::operator ==(
    CDriver& OtherDriver
    )
{
    CDriverFile* pThisDrvFile;
    CDriverFile* pOtherDrvFile;
    BOOL DrvFileFound = FALSE;
    PVOID ThisContext, OtherContext;
    if (GetFirstDriverFile(&pThisDrvFile, ThisContext))
    {
    do {
        DrvFileFound = FALSE;
        if (OtherDriver.GetFirstDriverFile(&pOtherDrvFile, OtherContext))
        {
        do {
            if (*pThisDrvFile == *pOtherDrvFile)
            {
            DrvFileFound = TRUE;
            break;
            }
        } while (OtherDriver.GetNextDriverFile(&pOtherDrvFile, OtherContext));
        }
    } while (DrvFileFound && GetNextDriverFile(&pThisDrvFile, ThisContext));
    return DrvFileFound;
    }
    else
    {
    // if both do not have driver file, they are equal.
    return !OtherDriver.GetFirstDriverFile(&pOtherDrvFile, OtherContext);
    }
}

//
// Can not throw a exception from this function because it is a callback
//
UINT
CDriver::ScanQueueCallback(
    PVOID Context,
    UINT  Notification,
    UINT  Param1,
    UINT  Param2
    )
{

    try
    {
    if (SPFILENOTIFY_QUEUESCAN == Notification && Param1)
    {
        CDriver* pDriver = (CDriver*)Context;
        if (pDriver)
        {
        SafePtr<CDriverFile> DrvFilePtr;
        CDriverFile* pDrvFile = new CDriverFile();
        DrvFilePtr.Attach(pDrvFile);
        if (pDrvFile->Create((LPCTSTR)Param1, pDriver->IsLocal()))
        {
            pDriver->AddDriverFile(pDrvFile);
            DrvFilePtr.Detach();
        }
        }
    }
    }
    catch (CMemoryException* e)
    {
    e->Delete();
    return ERROR_NOT_ENOUGH_MEMORY;
    }
    return NO_ERROR;
}

BOOL
CDriver::GetFirstDriverFile(
    CDriverFile** ppDrvFile,
    PVOID&  Context
    )
{
    ASSERT(ppDrvFile);
    if (!m_listDriverFile.IsEmpty())
    {
    POSITION pos = m_listDriverFile.GetHeadPosition();
    *ppDrvFile = m_listDriverFile.GetNext(pos);
    Context = pos;
    return TRUE;
    }
    Context = NULL;
    *ppDrvFile = NULL;
    return FALSE;
}

BOOL
CDriver::GetNextDriverFile(
    CDriverFile** ppDrvFile,
    PVOID&  Context
    )
{
    ASSERT(ppDrvFile);
    POSITION pos = (POSITION)Context;
    if (NULL != pos)
    {
    *ppDrvFile = m_listDriverFile.GetNext(pos);
    Context = pos;
    return TRUE;
    }
    *ppDrvFile = NULL;
    return FALSE;
}

void
CDriver::GetDriverSignerString(
    String& strDriverSigner
    )
{
    if (m_DigitalSigner.IsEmpty()) {

        strDriverSigner.LoadString(g_hInstance, IDS_NO_DIGITALSIGNATURE);
    
    } else {

        strDriverSigner = m_DigitalSigner;
    }
}


BOOL
CDriverFile::Create(
    LPCTSTR PathName,
    BOOL LocalMachine
    )
{
    if (!PathName || _T('\0') == PathName[0])
    {
        return FALSE;
    }

    // for remote machine, we can not verify if the driver file exits.
    // we only show the driver name.
    if (LocalMachine)
    {
        DWORD Attributes;
        Attributes = GetFileAttributes(PathName);
        
        if (0xFFFFFFFF != Attributes)
        {
            m_strFullPathName = PathName;
        }
        
        else
        {
            // The driver is a service. Do not search for the current director --
            // GetFullPathName is useless here.
            // Search for Windows dir and System directory
    
            TCHAR BaseDir[MAX_PATH];
            GetWindowsDirectory(BaseDir, ARRAYLEN(BaseDir));
            int Len;
            Len = lstrlen(BaseDir);
            
            if (Len)
            {
                if (_T('\\') != BaseDir[Len - 1])
                {
                    lstrcat(BaseDir, TEXT("\\"));
                }

                lstrcat(BaseDir, PathName);
                Attributes = GetFileAttributes(BaseDir);
            }
            
            if (0xFFFFFFFF == Attributes)
            {
                GetSystemDirectory(BaseDir, ARRAYLEN(BaseDir));
                Len = lstrlen(BaseDir);
                
                if (Len)
                {
                    if (_T('\\') != BaseDir[Len - 1])
                    {
                        lstrcat(BaseDir, TEXT("\\"));
                    }

                    lstrcat(BaseDir, PathName);
                    Attributes = GetFileAttributes(BaseDir);
                }
            }

            // hopeless, we could find the path
            if (0xFFFFFFFF == Attributes)
            {
                return FALSE;
            }

            m_strFullPathName = BaseDir;
        }

        m_HasVersionInfo = GetVersionInfo();
    }
    
    else
    {
        m_strFullPathName = PathName;
        
        //we do not have version info
        m_HasVersionInfo = FALSE;
    }
    
    return TRUE;
}
BOOL
CDriverFile::GetVersionInfo()
{
    DWORD Size, dwHandle;

    Size = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)m_strFullPathName, &dwHandle);
    
    if (!Size)
    {
        return FALSE;
    }

    BufferPtr<BYTE> BufPtr(Size);
    PVOID pVerInfo = BufPtr;
    
    if (GetFileVersionInfo((LPTSTR)(LPCTSTR)m_strFullPathName, dwHandle, Size,
                pVerInfo))
    {
        // get VarFileInfo\Translation
        PVOID pBuffer;
        UINT Len;
        String strStringFileInfo;
        
        if (!VerQueryValue(pVerInfo, (LPTSTR)tszTranslation, &pBuffer, &Len))
        {
            strStringFileInfo = tszStringFileInfoDefault;
        }
        
        else
        {
            strStringFileInfo.Format(tszStringFileInfo, *((WORD*)pBuffer),
                         *(((WORD*)pBuffer) + 1));
        }
        
        String str;
        str = strStringFileInfo + tszFileVersion;
        
        if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
        {
            m_strVersion = (LPTSTR)pBuffer;
            str = strStringFileInfo + tszLegalCopyright;
            
            if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
            {
                m_strCopyright = (LPTSTR)pBuffer;
                str = strStringFileInfo + tszCompanyName;
                
                if (VerQueryValue(pVerInfo, (LPTSTR)(LPCTSTR)str, &pBuffer, &Len))
                {
                    m_strProvider = (LPTSTR)pBuffer;
                }
            }
        }
    }

    return TRUE;
}


BOOL
CDriverFile::operator ==(
    CDriverFile& OtherDrvFile
    )
{
    return \
       m_HasVersionInfo == OtherDrvFile.HasVersionInfo() &&
       (GetFullPathName() == OtherDrvFile.GetFullPathName() ||
        (GetFullPathName() && OtherDrvFile.GetFullPathName() &&
         !lstrcmpi(GetFullPathName(), OtherDrvFile.GetFullPathName())
        )
       ) &&
       (GetProvider() == OtherDrvFile.GetProvider() ||
        (GetProvider() && OtherDrvFile.GetProvider() &&
         !lstrcmpi(GetProvider(), OtherDrvFile.GetProvider())
        )
       ) &&
       (GetCopyright() == OtherDrvFile.GetCopyright() ||
        (GetCopyright() && OtherDrvFile.GetCopyright() &&
         !lstrcmpi(GetCopyright(), OtherDrvFile.GetCopyright())
        )
       ) &&
       (GetVersion() == OtherDrvFile.GetVersion() ||
        (GetVersion() && OtherDrvFile.GetVersion() &&
         !lstrcmpi(GetVersion(), OtherDrvFile.GetVersion())
        )
       );
}
