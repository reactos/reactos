//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       devmgr.h
//
//--------------------------------------------------------------------------

#ifndef __DEVMGR_H_
#define __DEVMGR_H_

//
// header files from public tree
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <new.h>
#include <stdio.h>
#include <stdlib.h>
#include <prsht.h>
#include <commctrl.h>
#include <commctrl.h>
#include <ole2.h>
#include <mmc.h>
#include <objsel.h>
#include <htmlhelp.h>
#include <wincrypt.h>
#include <mscat.h>
#include <softpub.h>
#include <wintrust.h>

extern "C" {
#include <commdlg.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <regstr.h>
}


#define ARRAYLEN(array)     (sizeof(array) / sizeof(array[0]))


typedef enum tagCookieType
{
    COOKIE_TYPE_SCOPEITEM_DEVMGR = 0,
    COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ,
    COOKIE_TYPE_RESULTITEM_RESOURCE_DMA,
    COOKIE_TYPE_RESULTITEM_RESOURCE_IO,
    COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY,
    COOKIE_TYPE_RESULTITEM_COMPUTER,
    COOKIE_TYPE_RESULTITEM_DEVICE,
    COOKIE_TYPE_RESULTITEM_CLASS,
    COOKIE_TYPE_RESULTITEM_RESTYPE,
    COOKIE_TYPE_RESULTITEM_PHANTOMROOT,
    COOKIE_TYPE_UNKNOWN
} COOKIE_TYPE, *PCOOKIE_TYPE;


#define COOKIE_FLAGS_EXPANDED           0x00000001
#define COOKIE_FLAGS_SELECTED           0x00000002

const int TOTAL_COOKIE_TYPES = COOKIE_TYPE_RESULTITEM_RESTYPE - COOKIE_TYPE_SCOPEITEM_DEVMGR + 1;

const int NODETYPE_FIRST =      (int)COOKIE_TYPE_SCOPEITEM_DEVMGR;
const int NODETYPE_LAST  =      (int)COOKIE_TYPE_RESULTITEM_RESTYPE;


typedef struct tagNodeInfo {
    COOKIE_TYPE     ct;
    int             idsName;
    int             idsFormat;
    GUID            Guid;
    TCHAR*          GuidString;
} NODEINFO, *PNODEINFO;


#define PIF_CODE_EMBEDDED       0x01

typedef struct tagProblemInfo {
    int     StringId;
    DWORD   Flags;
} PROBLEMINFO, *PPROBLEMINFO;

typedef struct tagInternalData {
    DATA_OBJECT_TYPES   dot;
    COOKIE_TYPE ct;
    MMC_COOKIE  cookie;
} INTERNAL_DATA, *PINTERNAL_DATA;

typedef enum tagTreeType {
    TREE_TYPE_CLASS = 0,
    TREE_TYPE_CONNECTION
} TREE_TYPE, *PTREE_TYPE;

typedef enum tagDriverType{
    DRIVER_TYPE_CLASS = 0,
    DRIVER_TYPE_COMPATIBLE
} DRIVER_TYPE, *PDRIVER_TYPE;


typedef enum tagPropertyChangeType
{
    PCT_STARTUP_INFODATA = 0,
    PCT_DEVICE,
    PCT_CLASS
}PROPERTY_CHANGE_TYPE, *PPROPERTY_CHANGE_TYPE;

typedef struct tagPropertyChangeInfo
{
    PROPERTY_CHANGE_TYPE Type;
    BYTE                 InfoData[1];
}PROPERTY_CHANGE_INFO, *PPROPERTY_CHANGE_INFO;

typedef struct tagStartupInfoData
{
    DWORD           Size;
    COOKIE_TYPE     ct;
    TCHAR           MachineName[MAX_COMPUTERNAME_LENGTH + 3];
}STARTUP_INFODATA, *PSTARTUP_INFODATA;

typedef struct tagDeviceInfoData
{
    DWORD           Size;
    TCHAR           DeviceId[MAX_DEVICE_ID_LEN];
} DEVICE_INFODATA, *PDEVICE_INFODATA;

typedef struct tagClassInfoData
{
    DWORD           Size;
    GUID            ClassGuid;
} CLASS_INFODATA, *PCLASS_INFODATA;


typedef enum tagdmQuerySiblingCode
{
    QSC_TO_FOREGROUND = 0,
    QSC_PROPERTY_CHANGED,
}DMQUERYSIBLINGCODE, *PDMQUERYSIBLINGCODE;

//
// private header files
//

#include "..\inc\tvintf.h"
#include "resource.h"
#include "prndlg.h"
#include "globals.h"
#include "utils.h"
#include "ccookie.h"
#include "machine.h"
#include "compdata.h"
#include "componet.h"
#include "cnode.h"
#include "cfolder.h"
#include "dataobj.h"

BOOL InitGlobals(HINSTANCE hInstance);

extern LPCTSTR DEVMGR_DEVICEID_SWITCH;
extern LPCTSTR DEVMGR_MACHINENAME_SWITCH;
extern LPCTSTR DEVMGR_COMMAND_SWITCH;

void * __cdecl operator new(size_t size);
void __cdecl operator delete(void *ptr);
__cdecl _purecall(void);


STDAPI
DllRegisterServer();

STDAPI
DllUnregisterServer();

STDAPI
DllCanUnloadNow();

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID iid, void** ppv);


//
// CDMCommandLine class defination
//

class CDMCommandLine : public CCommandLine
{
public:
    CDMCommandLine() : m_WaitForDeviceId(FALSE),
                       m_WaitForMachineName(FALSE),
                       m_WaitForCommand(FALSE)
    {}
    virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
    {
        if (bFlag)
        {
            if (!lstrcmpi(DEVMGR_DEVICEID_SWITCH, lpszParam))
                m_WaitForDeviceId = TRUE;
            else if (!lstrcmpi(DEVMGR_MACHINENAME_SWITCH, lpszParam))
                m_WaitForMachineName = TRUE;
            else if (!lstrcmpi(DEVMGR_COMMAND_SWITCH, lpszParam))
                m_WaitForCommand = TRUE;
        }
        else
        {
            if (m_WaitForDeviceId) {
                m_strDeviceId = lpszParam;
                m_WaitForDeviceId = FALSE;
            }
            else if (m_WaitForMachineName) {
                m_strMachineName = lpszParam;
                m_WaitForMachineName = FALSE;
            }
            else if (m_WaitForCommand) {

                m_strCommand = lpszParam;
                m_WaitForCommand = FALSE;
            }
        }
    }
    LPCTSTR GetDeviceId()
    {
        return m_strDeviceId.IsEmpty() ? NULL : (LPCTSTR)m_strDeviceId;
    }
    LPCTSTR GetMachineName()
    {
        return m_strMachineName.IsEmpty() ? NULL : (LPCTSTR)m_strMachineName;
    }
    LPCTSTR GetCommand()
    {
        return m_strCommand.IsEmpty() ? NULL : (LPCTSTR)m_strCommand;
    }
private:
    String      m_strDeviceId;
    String      m_strMachineName;
    String      m_strCommand;
    BOOL        m_WaitForDeviceId;
    BOOL        m_WaitForMachineName;
    BOOL        m_WaitForCommand;
};

#endif
