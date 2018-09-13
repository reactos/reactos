
#ifndef _CDRIVER_H__
#define _CDRIVER_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    cdriver.h

Abstract:

    header file for cdriver.cpp. Defined CDriverFile, CDriver and CService

Author:

    William Hsieh (williamh) created

Revision History:


--*/
class CDriverFile
{

public:
    BOOL Create(LPCTSTR pszFullPathName, BOOL LocalMachine);
    LPCTSTR GetProvider()
    {
        return m_strProvider.IsEmpty() ? NULL : (LPCTSTR)m_strProvider;
    }
    LPCTSTR GetCopyright(void)
    {
        return m_strCopyright.IsEmpty() ? NULL : (LPCTSTR)m_strCopyright;

    }
    LPCTSTR GetVersion(void)
    {
        return m_strVersion.IsEmpty() ? NULL : (LPCTSTR)m_strVersion;
    }
    LPCTSTR GetFullPathName(void)
    {
        return m_strFullPathName.IsEmpty() ? NULL : (LPCTSTR)m_strFullPathName;
    }
    BOOL HasVersionInfo()
    {
        return m_HasVersionInfo;
    }
    BOOL operator ==(CDriverFile& OtherDrvFile);

private:
    BOOL    GetVersionInfo();
    String  m_strFullPathName;
    String  m_strProvider;
    String  m_strCopyright;
    String  m_strVersion;
    BOOL    m_HasVersionInfo;
};

class CDriver
{
// do we have to keep HDEVINFO and SP_DEVINFO_DATA here?????
public:
    CDriver() : m_pDevice(NULL), m_OnLocalMachine(TRUE)
    {};
    ~CDriver();
    BOOL Create(SC_HANDLE hscManager, LPTSTR tszServiceName);
    BOOL Create(CDevice* pDevice, PSP_DRVINFO_DATA pDrvInfoData = NULL);
    BOOL GetFirstDriverFile(CDriverFile** ppDrvFile, PVOID& Context);
    BOOL GetNextDriverFile(CDriverFile** ppDrvFile, PVOID& Context);
    void AddDriverFile(CDriverFile* pDrvFile)
    {
        m_listDriverFile.AddTail(pDrvFile);
    }
    BOOL IsLocal()
    {
        return m_OnLocalMachine;
    }
    void GetDriverSignerString(String& strDriverSigner);
    BOOL operator ==(CDriver& OtherDriver);

private:
    // call back must be a static function(because of the hidden this parameter
    static UINT ScanQueueCallback(PVOID Context, UINT Notification, UINT Param1, UINT Param2);
    void CreateFromService(CDevice* pDevice);
    CList<CDriverFile*,CDriverFile* > m_listDriverFile;
    CDevice* m_pDevice;
    BOOL    m_OnLocalMachine;
    String m_DigitalSigner;
};

#endif // _CDRIVER_H__
