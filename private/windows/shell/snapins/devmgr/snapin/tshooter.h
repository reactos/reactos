
// tshooter.h : header file
//

#ifndef __TSHOOTER_H__
#define __TSHOOTER_H__

/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    tshooter.h

Abstract:

    header file for tshooter.cpp

Author:

    William Hsieh (williamh) created

Revision History:



--*/

#include <tslauncher.h>
#include <tslerror.h>

extern const TCHAR* const DEVMGR_CALLERNAME;
extern const TCHAR* const DEVMGR_CALLERVERSION;

inline
LPTSTR
MAKE_PROBLEM_STRING(ULONG Problem)
{
    ASSERT(Problem <= 255);
    return (LPTSTR)(ULONG_PTR)((BYTE)Problem);
}

class CTroubleshooter
{
public:
    CTroubleshooter()
    : m_hTSL(NULL),
      m_pDeviceID(NULL),
      m_pClassGuidString(NULL),
      m_Problem(0)
    {}
    virtual ~CTroubleshooter()
    {
        Close();
    }
    void Close()
    {
        if (m_hTSL)
        {
        TSLClose(m_hTSL);
        m_hTSL = NULL;
        }
        if (m_pDeviceID)
        {
        delete [] m_pDeviceID;
        m_pDeviceID = NULL;
        }
        if (m_pClassGuidString)
        {
        delete [] m_pClassGuidString;
        m_pClassGuidString = NULL;
        }
    }
    BOOL ReOpen()
    {
        if (m_hTSL)
        {
        m_hTSL = TSLReInit(m_hTSL);
        return NULL != m_hTSL;
        }
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    BOOL Open(LPCTSTR MachineName = NULL);
    BOOL Open(LPCTSTR MachineName, LPGUID ClassGuid);
    BOOL Open(CMachine* pMachine = NULL);
    BOOL Open(CClass* pClass);
    BOOL Open(CDevice* pDevice);
    BOOL Run();
private:
    void     DumpErrors(LPCTSTR APIName, DWORD TslError);
    HANDLE    m_hTSL;
    TCHAR*    m_pDeviceID;
    TCHAR*    m_pClassGuidString;
    ULONG     m_Problem;
    DWORD TSLError2WinError(DWORD TSLError);
    BOOL Test(LPGUID pClassGuid = NULL, LPCTSTR pDeviceID = NULL, ULONG Problem = 0);
    BOOL SetDeviceID(LPCTSTR DeviceID);
    BOOL SetClassGuid(LPGUID pClassGuid);
    BOOL SetProblem(ULONG Problem);
    LPCTSTR TestIDs(LPCTSTR DeviceIDs, LPGUID ClassGuid, ULONG Problem);
};

#endif      // !defined(__TSHOOTER_H__)
