/*++

Copyright (C) 1997-1999  Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    This module implements global functions needed for the program.
    It also contain global variables/classes.

Author:

    William Hsieh (williamh) created

Revision History:


--*/


#include "devmgr.h"
#include <winioctl.h>
#include <shellapi.h>
#include <shlobj.h>
#define NO_SHELL_TREE_TYPE
#include <shlobjp.h>


//
//  global classes and variables
//

// this, of course, our dll's instance handle.
HINSTANCE g_hInstance = NULL;

//
// A CMachineList is created for each instance of DLL. It is shared
// by all the CComponentData the instance might create. The class CMachine
// contains all the information about all the classes and devices on the
// machine. Each CComponent should register itself to CMachine. This way,
// A CComponent will get notification whenever there are changes in
// the CMachine(Refresh, Property changes on a device, for example).
// We do not rely on MMC's view notification(UpdatAllView) because
// it only reaches all the CComponents created by a CComponenetData.
//
//
CMachineList    g_MachineList;
CMemoryException g_MemoryException(TRUE);
String      g_strStartupMachineName;
String      g_strStartupDeviceId;
String      g_strStartupCommand;
String      g_strDevMgr;
BOOL        g_HasLoadDriverNamePrivilege = FALSE;
CPrintDialog    g_PrintDlg;


//
// UUID consts
//

const CLSID CLSID_DEVMGR = {0x74246BFC,0x4C96,0x11D0,{0xAB,0xEF,0x00,0x20,0xAF,0x6B,0x0B,0x7A}};
const CLSID CLSID_DEVMGR_EXTENSION = {0x90087284,0xd6d6,0x11d0,{0x83,0x53,0x00,0xa0,0xc9,0x06,0x40,0xbf}};
const CLSID CLSID_SYSTOOLS = {0x476e6448,0xaaff,0x11d0,{0xb9,0x44,0x00,0xc0,0x4f,0xd8,0xd5,0xb0}};
const CLSID CLSID_DEVMGR_ABOUT = {0x94abaf2a,0x892a,0x11d1,{0xbb,0xc4,0x00,0xa0,0xc9,0x06,0x40,0xbf}};

const TCHAR* const CLSID_STRING_DEVMGR = TEXT("{74246bfc-4c96-11d0-abef-0020af6b0b7a}");
const TCHAR* const CLSID_STRING_DEVMGR_EXTENSION = TEXT("{90087284-d6d6-11d0-8353-00a0c90640bf}");
const TCHAR* const CLSID_STRING_SYSTOOLS = TEXT("{476e6448-aaff-11d0-b944-00c04fd8d5b0}");
const TCHAR* const CLSID_STRING_DEVMGR_ABOUT = TEXT("{94abaf2a-892a-11d1-bbc4-00a0c90640bf}");
//
// ProgID
//

const TCHAR* const PROGID_DEVMGR = TEXT("DevMgrSnapin.DevMgrSnapin.1");
const TCHAR* const PROGID_DEVMGREXT = TEXT("DevMgrExtension.DevMgrExtension.1");
const TCHAR* const PROGID_DEVMGR_ABOUT = TEXT("DevMgrAbout.DevMgrAbout.1");

const TCHAR* const ENV_NAME_SYSTEMDRIVE = TEXT("SystemDrive");

//
// Node types const
//
const NODEINFO NodeInfo[TOTAL_COOKIE_TYPES] =
{

    { COOKIE_TYPE_SCOPEITEM_DEVMGR,
      IDS_NAME_DEVMGR,
      IDS_DISPLAYNAME_SCOPE_DEVMGR,
      {0xc41dfb2a,0x4d5b,0x11d0,{0xab,0xef,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{c41dfb2a-4d5b-11d0-abef-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_IRQ,
      IDS_NAME_IRQ,
      0,
      {0x494535fe,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{494535fe-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_DMA,
      IDS_NAME_DMA,
      0,
      {0x49f0df4e,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{49f0df4e-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_IO,
      IDS_NAME_IO,
      0,
      {0xa2958d7a,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7a-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESOURCE_MEMORY,
      IDS_NAME_MEMORY,
      0,
      {0xa2958d7b,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7b-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_COMPUTER,
      IDS_NAME_COMPUTER,
      0,
      {0xa2958d7c,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7c-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_DEVICE,
      IDS_NAME_DEVICE,
      0,
      {0xa2958d7d,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7d-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_CLASS,
      IDS_NAME_CLASS,
      0,
      {0xe677e204,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{e677e204-5aa2-11d0-abf0-0020af6b0b7a}")
    },
    { COOKIE_TYPE_RESULTITEM_RESTYPE,
      IDS_NAME_RESOURCES,
      0,
      {0xa2958d7e,0x5aa2,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}},
      TEXT("{a2958d7e-5aa2-11d0-abf0-0020af6b0b7a}")
    }
};

const IID IID_IDMTVOCX =    \
    {0x142525f2,0x59d8,0x11d0,{0xab,0xf0,0x00,0x20,0xaf,0x6b,0x0b,0x7a}};
const IID IID_ISnapinCallback = \
    {0x8e0ba98a,0xd161,0x11d0,{0x83,0x53,0x00,0xa0,0xc9,0x06,0x40,0xbf}};

//
// cliboard format strings
//
const TCHAR* const MMC_SNAPIN_MACHINE_NAME = TEXT("MMC_SNAPIN_MACHINE_NAME");
const TCHAR* const SNAPIN_INTERNAL = TEXT("SNAPIN_INTERNAL");
const TCHAR* const DEVMGR_SNAPIN_CLASS_GUID = TEXT("DEVMGR_SNAPIN_CLASS_GUID");
const TCHAR* const DEVMGR_SNAPIN_DEVICE_ID  = TEXT("DEVMGR_SNAPIN_DEVICE_ID");
const TCHAR* const DEVMGR_COMMAND_PROPERTY = TEXT("Property");
const TCHAR* const REG_PATH_DEVICE_MANAGER = TEXT("SOFTWARE\\Microsoft\\DeviceManager");
const TCHAR* const REG_STR_BUS_TYPES    = TEXT("BusTypes");
const TCHAR* const REG_STR_TROUBLESHOOTERS = TEXT("TroubleShooters");
const TCHAR* const DEVMGR_HELP_FILE_NAME = TEXT("devmgr.hlp");
const TCHAR* const REG_VAL_DEVMGR_LOGGING_MASK = TEXT("LoggingMask");
const TCHAR* const DEVMGR_HTML_HELP_FILE_NAME = TEXT("\\help\\devmgr.chm");
#if DEVL
const TCHAR* const REG_VAL_DEVMGR_DEBUGBREAKOPTIONS = TEXT("DebugBreakOptions");
DWORD  g_DebugBreakOptions = 0;
#endif

// lookup table to translate problem number to its text resource id.
const PROBLEMINFO  g_ProblemInfo[] =
{
    {IDS_PROB_NOPROBLEM, 0},                // NO PROBLEM
    {IDS_PROB_NOT_CONFIGURED, PIF_CODE_EMBEDDED},   // CM_PROB_NOT_CONFIGURED
    {IDS_PROB_DEVLOADERFAILED, PIF_CODE_EMBEDDED},  // CM_PROB_DEVLOADER_FAILED
    {IDS_PROB_OUT_OF_MEMORY, PIF_CODE_EMBEDDED},    // CM_PROB_OUT_OF_MEMORY
    {IDS_PROB_WRONG_TYPE, PIF_CODE_EMBEDDED},       // CM_PROB_ENTRY_IS_WRONG_TYPE
    {IDS_PROB_LACKEDARBITRATOR, PIF_CODE_EMBEDDED}, // CM_PROB_LACKED_ARBITRATOR
    {IDS_PROB_BOOT_CONFIG_CONFLICT, PIF_CODE_EMBEDDED}, // CM_PROB_BOOT_CONFIG_CONFLICT
    {IDS_PROB_FAILED_FILTER, PIF_CODE_EMBEDDED},    // CM_PROB_FAILED_FILTER
    {IDS_PROB_DEVLOADER_NOT_FOUND, PIF_CODE_EMBEDDED},  // CM_PROB_DEVLOADER_NOT_FOUND
    {IDS_PROB_INVALID_DATA, PIF_CODE_EMBEDDED},     // CM_PROB_INVALID_DATA
    {IDS_PROB_FAILED_START, PIF_CODE_EMBEDDED},     // CM_PROB_FAILED_START
    {IDS_PROB_LIAR, PIF_CODE_EMBEDDED},         // CM_PROB_LIAR
    {IDS_PROB_NORMAL_CONFLICT, PIF_CODE_EMBEDDED},  // CM_PROB_NORMAL_CONFLICT
    {IDS_PROB_NOT_VERIFIED, PIF_CODE_EMBEDDED},     // CM_PROB_NOT_VERIFIED
    {IDS_PROB_NEEDRESTART, PIF_CODE_EMBEDDED},      // CM_PROB_NEED_RESTART
    {IDS_PROB_REENUMERATION, PIF_CODE_EMBEDDED},    // CM_PROB_REENUMERATION
    {IDS_PROB_PARTIALCONFIG, PIF_CODE_EMBEDDED},    // CM_PROB_PARTIAL_LOG_CONF
    {IDS_PROB_UNKNOWN_RESOURCE, PIF_CODE_EMBEDDED}, // CM_PROB_UNKNOWN_RESOURCE
    {IDS_PROB_REINSTALL, PIF_CODE_EMBEDDED},        // CM_PROB_REINSTALL
    {IDS_PROB_REGISTRY, PIF_CODE_EMBEDDED},     // CM_PROB_REGISTRY
    {IDS_PROB_SYSTEMFAILURE, PIF_CODE_EMBEDDED},    // CM_PROB_VXDLDR
    {IDS_PROB_WILL_BE_REMOVED, PIF_CODE_EMBEDDED},  // CM_PROB_WILL_BE_REMOVED
    {IDS_PROB_DISABLED, PIF_CODE_EMBEDDED},     // CM_PROB_DISABLED
    {IDS_PROB_SYSTEMFAILURE, PIF_CODE_EMBEDDED},    // CM_PROB_DEVLOADER_NOT_READY
    {IDS_PROB_DEVICENOTPRESENT, PIF_CODE_EMBEDDED}, // CM_PROB_DEVICE_NOT_THERE
    {IDS_PROB_MOVED, PIF_CODE_EMBEDDED},        // CM_PROB_MOVED
    {IDS_PROB_TOO_EARLY, PIF_CODE_EMBEDDED},        // CM_PROB_TOO_EARLY
    {IDS_PROB_NO_VALID_LOG_CONF, PIF_CODE_EMBEDDED},    // CM_PROB_NO_VALID_LOG_CONF
    {IDS_PROB_FAILEDINSTALL, PIF_CODE_EMBEDDED},    // CM_PROB_FAILED_INSTALL
    {IDS_PROB_HARDWAREDISABLED, PIF_CODE_EMBEDDED}, // CM_PROB_HARDWARE_DISABLED
    {IDS_PROB_CANT_SHARE_IRQ, PIF_CODE_EMBEDDED},   // CM_PROB_CANT_SHARE_IRQ
    {IDS_PROB_ADD_DEVICE_FAILED, PIF_CODE_EMBEDDED},    // CM_PROB_FAILED_ADD
    {IDS_PROB_DISABLED_SERVICE, PIF_CODE_EMBEDDED},    // CM_PROB_DISABLED_SERVICE
    {IDS_PROB_TRANSLATION_FAILED, PIF_CODE_EMBEDDED},    // CM_PROB_TRANSLATION_FAILED
    {IDS_PROB_NO_SOFTCONFIG, PIF_CODE_EMBEDDED},    // CM_PROB_NO_SOFTCONFIG
    {IDS_PROB_BIOS_TABLE, PIF_CODE_EMBEDDED},    // CM_PROB_BIOS_TABLE
    {IDS_PROB_IRQ_TRANSLATION_FAILED, PIF_CODE_EMBEDDED},    // CM_PROB_IRQ_TRANSLATION_FAILED
    {IDS_PROB_UNKNOWN_WITHCODE, PIF_CODE_EMBEDDED}  // UNKNOWN PROBLEM
};


//
// Global functions
//

#if DBG
//
// Debugging aids
//
void
Trace(
    LPCTSTR format,
    ...
    )
{
    // according to wsprintf specification, the max buffer size is
    // 1024
    TCHAR Buffer[1024];
    va_list arglist;
    va_start(arglist, format);
    wvsprintf(Buffer, format, arglist);
    va_end(arglist);
    OutputDebugString(TEXT("DEVMGR: "));
    OutputDebugString(Buffer);
    OutputDebugString(TEXT("\r\n"));
}
#endif


inline
BOOL
IsBlankChar(TCHAR ch)
{
    return (_T(' ') == ch || _T('\t') == ch);
}

inline
LPTSTR
SkipBlankChars(
    LPTSTR psz
    )
{
    while (IsBlankChar(*psz))
    psz++;
    return psz;
}

// We import the following two "private" apis to avoid linking with rpcrt4.dll
// which contains UuidFromString and StringToUuid functions.
extern "C" {
extern DWORD pSetupGuidFromString(LPCTSTR GuidString, LPGUID pGuid);
extern DWORD pSetupStringFromGuid(const GUID* pGuid, LPTSTR Buffer, DWORD BufferLen);
}

//
// This function converts a given string to GUID
// INPUT:
//  GuidString   -- the null terminated guid string
//  LPGUID       -- buffer to receive the GUID
// OUTPUT:
//     TRUE if the conversion succeeded.
//     FALSE if failed.
//
inline
BOOL
GuidFromString(
    LPCTSTR GuidString,
    LPGUID  pGuid
    )
{
    return ERROR_SUCCESS == pSetupGuidFromString(GuidString, pGuid);
}

// This function converts the given GUID to a string
// INPUT:
//  pGuid    -- the guid
//  Buffer   -- the buffer to receive the string
//  BufferLen -- the buffer size in char unit
// OUTPUT:
//  TRUE  if the conversion succeeded.
//  FLASE if failed.
//
//
inline
BOOL
GuidToString(
    LPGUID pGuid,
    LPTSTR Buffer,
    DWORD  BufferLen
    )
{
    return ERROR_SUCCESS == pSetupStringFromGuid(pGuid, Buffer, BufferLen);
}

//
// This function allocates an OLE string from OLE task memory pool
// It does necessary char set conversion before copying the string.
//
// INPUT: LPCTSTR str  -- the initial string
// OUTPUT:  LPOLESTR   -- the result OLE string. NULL if the function fails.
//
LPOLESTR
AllocOleTaskString(
    LPCTSTR str
    )
{
    if (!str)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
    }
// if _UNICODE is defined, OLECHAR == WCHAR
// if OLE2ANSI is defined, OLECHAR == CHAR

#if defined(UNICODE) || defined(OLE2ANSI)
    size_t Len = lstrlen(str);
    // allocate the null terminate char also.
    LPOLESTR olestr = (LPOLESTR)::CoTaskMemAlloc((Len + 1) * sizeof(TCHAR));
    if (olestr)
    {
    lstrcpy((LPTSTR)olestr, str);
    return olestr;
    }
    return NULL;
#else
    // OLE is in UNICODE while TCHAR is CHAR
    // We need to convert ANSI(TCHAR) to WCHAR
    size_t Len = strlen(str);
    LPOLESTR olestr = (LPOLESTR)::CoTaskMemAlloc((Len + 1) * sizeof(WCHAR));
    if (olestr)
    {
    MultiByteToWideChar(CP_ACP, 0, str, -1, (LPWSTR)olestr, Len + 1);
    return olestr;
    }
    return NULL;
#endif
}

inline
void
FreeOleTaskString(
    LPOLESTR olestr
    )
{
    if (olestr)
    ::CoTaskMemFree(olestr);
}
//
// This function addes the given menu item to snapin
// INPUT:
//  iNameStringId    -- menu item text resource id
//  iStatusBarStringId -- status bar text resource id.
//  iCommandId     -- command id to be assigned to the menu item
//  InsertionPointId   -- Insertion point id
//  Flags          -- flags
//  SpecialFlags       -- special flags
// OUTPUT:
//  HRESULT
//
HRESULT
AddMenuItem(
    LPCONTEXTMENUCALLBACK pCallback,
    int   iNameStringId,
    int   iStatusBarStringId,
    long  lCommandId,
    long  InsertionPointId,
    long  Flags,
    long  SpecialFlags
    )
{
    ASSERT(pCallback);

    CONTEXTMENUITEM tCMI;
    memset(&tCMI, 0, sizeof(tCMI));
    tCMI.lCommandID = lCommandId;
    tCMI.lInsertionPointID = InsertionPointId;
    tCMI.fFlags = Flags;
    tCMI.fSpecialFlags = SpecialFlags;
    TCHAR Name[MAX_PATH];
    TCHAR Status[MAX_PATH];
    ::LoadString(g_hInstance, iNameStringId, Name, ARRAYLEN(Name));
    tCMI.strName = Name;
    if (iStatusBarStringId) {
       TCHAR Status[MAX_PATH];
       ::LoadString(g_hInstance, iStatusBarStringId, Status, ARRAYLEN(Status));
       tCMI.strStatusBarText = Status;
    }
    return pCallback->AddItem(&tCMI);
}

//
// This function verifies the given machine name can be accessed remotely.
// INPUT:
//  MachineName -- the machine name. The machine name must be
//             led by "\\\\".
// OUTPUT:
//  CONFIGRET CODE
//      CR_SUCCESS for success or CR_xxx failure reason.
CONFIGRET
VerifyMachineName(
    LPCTSTR MachineName
    )
{
    CONFIGRET cr = CR_SUCCESS;
    HMACHINE hMachine = NULL;
    HKEY hRemoteKey = NULL;
    HKEY hClass = NULL;

    if (MachineName)
    {
        int len = lstrlen(MachineName);

        //
        // empty means local machine.
        //
        if (!len)
            return TRUE;
        
        if (len > 2 &&  _T('\\') == MachineName[0] && _T('\\') == MachineName[1])
        {
            //
            // make sure we can connect the machine using cfgmgr32.
            //
            cr = CM_Connect_Machine(MachineName, &hMachine);

            //
            // We could not connect to the machine using cfgmgr32
            //
            if (CR_SUCCESS != cr)
            {
                goto clean0;
            }

            //
            // make sure we can connect to the registry of the remote machine
            //
            if (RegConnectRegistry(MachineName, HKEY_LOCAL_MACHINE,
                                           &hRemoteKey) != ERROR_SUCCESS) {

                cr = CR_REGISTRY_ERROR;
                goto clean0;
            }

            cr = CM_Open_Class_Key_Ex(NULL,
                                      NULL,
                                      KEY_READ,
                                      RegDisposition_OpenExisting,
                                      &hClass,
                                      CM_OPEN_CLASS_KEY_INSTALLER,
                                      hMachine
                                      );
        }
    }
    
clean0:

    if (hMachine) {
        
        CM_Disconnect_Machine(hMachine);
    }

    if (hRemoteKey) {

        RegCloseKey(hRemoteKey);
    }

    if (hClass) {

        RegCloseKey(hClass);
    }

    return cr;
}

// This function loads the string designated by the given
// string id(resource id) from the module's resource to the provided
// buffer. It returns the required buffer size(in chars) to hold the string,
// not including the terminated NULL chars. Last error will be set
// appropaitely.
//
// input: int StringId  -- the resource id of the string to be loaded.
//    LPTSTR Buffer -- provided buffer to receive the string
//    UINT   BufferSize -- the size of Buffer in chars
// output:
//    UINT the required buffer size to receive the string
//    if it returns 0, GetLastError() returns the error code.
//
UINT
LoadResourceString(
    int StringId,
    LPTSTR Buffer,
    UINT   BufferSize
    )
{
    // do some trivial tests.
    if (BufferSize && !Buffer)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
    }
    // if caller provides buffer, try to load the string with the given buffer
    // and length.
    UINT FinalLen;

    if (Buffer)
    {
    FinalLen = ::LoadString(g_hInstance, StringId, Buffer, BufferSize);
    if (BufferSize > FinalLen)
    {
        return FinalLen;
    }
    }
    // Either the caller does not provide the buffer or the given buffer
    // is too small. Try to figure out the requried size.
    //

    // first use a stack-based buffer to get the string. If the buffer
    // is big enough, we are happy.
    TCHAR Temp[256];
    UINT ArrayLen = ARRAYLEN(Temp);
    FinalLen = ::LoadString(g_hInstance, StringId, Temp, ArrayLen);

    DWORD LastError = ERROR_SUCCESS;

    if (ArrayLen <= FinalLen)
    {   // the stack-based buffer is too small, use heap-based buffer.
    // we have not got all the chars. we increase the buffer size of 256
    // chars each time it fails. The initial size is 512(256+256)
    // the max size is 32K
    ArrayLen = 256;
    TCHAR* HeapBuffer;
    FinalLen = 0;
    while (ArrayLen < 0x8000)
    {
        ArrayLen += 256;
        HeapBuffer = new TCHAR[ArrayLen];
        if (HeapBuffer)
        {
        FinalLen = ::LoadString(g_hInstance, StringId, HeapBuffer, ArrayLen);
        delete [] HeapBuffer;
        if (FinalLen < ArrayLen)
            break;
        }
        else
        {
        LastError = ERROR_NOT_ENOUGH_MEMORY;
        break;
        }
    }
    if (ERROR_SUCCESS != LastError)
    {
        SetLastError(LastError);
        FinalLen = 0;
    }
    }
    return FinalLen;
}

// This function get the problem text designated by the given problem number
// for the given devnode on the given machine.
//
//
// input: HMACHINE hMachine -- machine handle(NULL for local machine)
//    DEVNODE  DevNode  -- the device
//    ULONG ProblemNumber -- the problem number
//    LPTSTR Buffer -- provided buffer to receive the string
//    UINT   BufferSize -- the size of Buffer in chars
// output:
//    UINT the required buffer size to receive the string
//    if it returns 0, GetLastError() returns the error code.
//
UINT
GetDeviceProblemText(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPTSTR Buffer,
    UINT   BufferSize
    )
{
    // first does a trivial test
    if (!Buffer && BufferSize)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
    }

    int StringId;
    TCHAR*  pMainText;
    UINT RequiredSize;
    UINT Length;
    pMainText = NULL;
    RequiredSize = 0;

    PROBLEMINFO pi;
    // get the PROBLEMINFO for the problem number
    pi = g_ProblemInfo[min(ProblemNumber, NUM_CM_PROB)];
    // figure out the main text length
    Length = LoadResourceString(pi.StringId, Buffer, BufferSize);
    if (Length)
    {
    try
    {
        BufferPtr<TCHAR> MainTextPtr(Length + 1);
        LoadResourceString(pi.StringId, MainTextPtr, Length + 1);
        if (pi.Flags & PIF_CODE_EMBEDDED)
        {
        // embedded problem code in the main text.
        Length = LoadResourceString(IDS_PROB_CODE, NULL, 0);
        if (Length)
        {
            BufferPtr<TCHAR> FormatPtr(Length + 1);
            LoadResourceString(IDS_PROB_CODE, FormatPtr, Length + 1);
            BufferPtr<TCHAR> CodeTextPtr(Length + 1 + 32);
            wsprintf(CodeTextPtr, FormatPtr, ProblemNumber);
            pMainText = new TCHAR[lstrlen(MainTextPtr) + lstrlen(CodeTextPtr) + 32];
            wsprintf(pMainText, MainTextPtr, CodeTextPtr);
        }
        }
        else
        {
        pMainText = new TCHAR[Length + 1];
        lstrcpy(pMainText, MainTextPtr);
        }
        RequiredSize += lstrlen(pMainText);
        // copy the main text
        if (RequiredSize && BufferSize > RequiredSize)
        {
        lstrcpy(Buffer, pMainText);
        }
    }
    catch (CMemoryException* e)
    {
        e->Delete();
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RequiredSize = 0;
    }
    }
    if (pMainText)
    delete [] pMainText;
    return RequiredSize;
}

//
// This function creates and shows a message box
// INPUT:
//  hwndParent -- the window handle servers as the parent window to the
//        message box
//  MsgId      -- string id for message box body. The string can be
//        a format string.
//  CaptionId  -- string id for caption. if 0, default is device manager
//  Type       -- the standard message box flags(MB_xxxx)
//  ...        -- parameters to MsgId string if it contains any
//        format chars.
//OUTPUT:
//  return value from MessageBox(IDOK, IDYES...)

int MsgBoxParam(
    HWND hwndParent,
    int MsgId,
    int CaptionId,
    DWORD Type,
    ...
    )
{
    TCHAR szMsg[MAX_PATH * 4], szCaption[MAX_PATH];;
    LPCTSTR pCaption;

    va_list parg;
    int Result;
    // if no MsgId is given, it is for no memory error;
    if (MsgId)
    {
    va_start(parg, Type);

    // load the msg string to szCaption(temp). The text may contain
    // format information
    if (!::LoadString(g_hInstance, MsgId, szCaption, ARRAYLEN(szCaption)))
        goto NoMemory;
    //finish up format string
    wvsprintf(szMsg, szCaption, parg);
    // if caption id is given, load it.
    if (CaptionId)
    {
        if (!::LoadString(g_hInstance, CaptionId, szCaption, ARRAYLEN(szCaption)))
        goto NoMemory;
        pCaption = szCaption;
    }
    else
        pCaption = g_strDevMgr;

    if (!(Result = MessageBox(hwndParent, szMsg, pCaption, Type)))
        goto NoMemory;
    return Result;
    }
NoMemory:
    g_MemoryException.ReportError(hwndParent);
    return 0;
}


// This function creates and displays a message box for the given
// win32 error(or last error)
// INPUT:
//  hwndParent -- the parent window for the will-be-created message box
//  CaptionId  -- optional string id for caption
//  Type       -- message styles(MB_xxxx)
//  Error      -- Error code. If the value is ERROR_SUCCESS,
//            GetLastError() will be called to retreive the
//            real error code.
// OUTPUT:
//  the value return from MessageBox
//
int
MsxBoxWinError(
    HWND hwndParent,
    int  CaptionId,
    DWORD Type,
    DWORD Error
    )
{
    if (ERROR_SUCCESS == Error)
    Error = GetLastError();

    // nonsense to report success!
    ASSERT(ERROR_SUCCESS != Error);

    TCHAR szMsg[MAX_PATH];
    TCHAR szCaption[MAX_PATH];
    LPCTSTR Caption;

    if (CaptionId && ::LoadString(g_hInstance, CaptionId, szCaption, ARRAYLEN(szCaption)))
    {
    Caption = szCaption;
    }
    else
    {
    Caption = g_strDevMgr;
    }
    FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
          NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
          szMsg, ARRAYLEN(szMsg), NULL);
    return MessageBox(hwndParent, szMsg, Caption, Type);
}

// This functin prompts for restart.
// INPUT:
//  hwndParent -- the window handle to be used as the parent window
//            to the restart dialog
//  RestartFlags -- flags(RESTART/REBOOT/POWERRECYCLE)
//  ResId        -- designated string resource id. If 0, default will
//          be used.
// OUTPUT:
//  None
void
PromptForRestart(
    HWND hwndParent,
    DWORD RestartFlags,
    int   ResId
    )
{
    if (RestartFlags & (DI_NEEDRESTART | DI_NEEDREBOOT))
    {
        DWORD ExitWinCode;

        try
        {
            String str;
    
            if (RestartFlags & DI_NEEDRESTART)
            {
                if (!ResId)
                {
                    ResId = IDS_DEVCHANGE_RESTART;
                }

                str.LoadString(g_hInstance, ResId);
                ExitWinCode = EWX_REBOOT;
            }
            
            else if (RestartFlags & DI_NEEDREBOOT)
            {
                if (!ResId && RestartFlags & DI_NEEDPOWERCYCLE)
                {
        
                    String str2;
                    str.LoadString(g_hInstance, IDS_POWERCYC1);
                    str2.LoadString(g_hInstance, IDS_POWERCYC2);
                    str += str2;
                    ExitWinCode = EWX_SHUTDOWN;
                }
                
                else
                {
                    if (!ResId)
                    {
                        ResId = IDS_DEVCHANGE_RESTART;
                    }

                    str.LoadString(g_hInstance, ResId);
                    ExitWinCode = EWX_REBOOT;
                }
            }
            
            RestartDialog(hwndParent, str, ExitWinCode);
        }
        
        catch(CMemoryException* e)
        {
            e->Delete();
            MsgBoxParam(hwndParent, 0, 0, 0);
        }
    }
}

BOOL
LoadEnumPropPage32(
    LPCTSTR RegString,
    HMODULE* pDll,
    FARPROC* pProcAddress
    )
{
    // verify parameters
    if (!RegString || _T('\0') == RegString[0] || !pDll || !pProcAddress)
    {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
    }
    // make a copy of the string because we have to party on it
    TCHAR* psz = new TCHAR[lstrlen(RegString) + 1];
    lstrcpy(psz, RegString);
    LPTSTR DllName;
    LPTSTR DllNameEnd;
    LPTSTR FunctionName;
    LPTSTR FunctionNameEnd;
    LPTSTR p;
    p = psz;
    SetLastError(ERROR_SUCCESS);
    // the format of the  string is "dllname, dllentryname"
    p = SkipBlankChars(p);
    if (_T('\0') != *p)
    {
    // looking for dllname which could be enclosed
    // inside double quote chars.
    // NOTE: not double quote chars inside double quoted string is allowed.
    if (_T('\"') == *p)
    {
        DllName = ++p;
        while (_T('\"') != *p && _T('\0') != *p)
        p++;
        DllNameEnd = p;
        if (_T('\"') == *p)
        p++;
    }
    else
    {
        DllName = p;
        while (!IsBlankChar(*p) && _T(',') != *p)
        p++;
        DllNameEnd = p;
    }
    // looking for ','
    p = SkipBlankChars(p);
    if (_T('\0') != *p && _T(',') == *p)
    {
        p = SkipBlankChars(p + 1);
        if (_T('\0') != *p)
        {
        FunctionName = p++;
        while (!IsBlankChar(*p) && _T('\0') != *p)
            p++;
        FunctionNameEnd = p;
        }
    }
    }

    if (DllName && FunctionName)
    {
    *DllNameEnd = _T('\0');
    *FunctionNameEnd = _T('\0');
    *pDll = LoadLibrary(DllName);
    if (*pDll)
    {
#ifdef UNICODE
        // convert Wide char to ANSI which is GetProcAddress expected.
        // We do not append a 'A" or a "W' here.
        CHAR FuncNameA[256];
        int LenA = WideCharToMultiByte(CP_ACP, 0,
                       FunctionName,
                       wcslen(FunctionName) + 1,
                       FuncNameA,
                       sizeof(FuncNameA),
                       NULL, NULL);
        *pProcAddress = GetProcAddress(*pDll, FuncNameA);
#else
        *pProcAddress = GetProcAddress(*pDll, FunctionName);
#endif
    }
    }
    delete [] psz;
    if (!*pProcAddress && *pDll)
    FreeLibrary(*pDll);
    return (*pDll && *pProcAddress);
}


BOOL
AddPropPageCallback(
    HPROPSHEETPAGE hPage,
    LPARAM lParam
    )
{
    CPropSheetData* ppsData = (CPropSheetData*)lParam;
    ASSERT(ppsData);
    return ppsData->InsertPage(hPage);
}

PCTSTR
MyGetFileTitle(
    IN PCTSTR FilePath
    )

/*++

Routine Description:

    This routine returns a pointer to the first character in the
    filename part of the supplied path.  If only a filename was given,
    then this will be a pointer to the first character in the string
    (i.e., the same as what was passed in).

    To find the filename part, the routine returns the last component of
    the string, beginning with the character immediately following the
    last '\', '/' or ':'. (NB NT treats '/' as equivalent to '\' )

Arguments:

    FilePath - Supplies the file path from which to retrieve the filename
        portion.

Return Value:

    A pointer to the beginning of the filename portion of the path.

--*/

{
    PCTSTR LastComponent = FilePath;
    TCHAR  CurChar;

    while(CurChar = *FilePath) {
        FilePath = CharNext(FilePath);
        if((CurChar == TEXT('\\')) || (CurChar == TEXT('/')) || (CurChar == TEXT(':'))) {
            LastComponent = FilePath;
        }
    }

    return LastComponent;
}

BOOL
AddToolTips(
    HWND hDlg,
    UINT id,
    LPCTSTR pszText,
    HWND *phwnd
    )
{
    if (*phwnd == NULL)
    {
        *phwnd = CreateWindow(TOOLTIPS_CLASS,
                              TEXT(""),
                              WS_POPUP | TTS_NOPREFIX,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              CW_USEDEFAULT,
                              hDlg,
                              NULL,
                              g_hInstance,
                              NULL);
        if (*phwnd)
        {
            TOOLINFO ti;

            ti.cbSize = sizeof(ti);
            ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            ti.hwnd = hDlg;
            ti.uId = (UINT_PTR)GetDlgItem(hDlg, id);
            ti.lpszText = (LPTSTR)pszText;  // const -> non const
            ti.hinst = g_hInstance;
            SendMessage(*phwnd, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
        }
    }

    return (*phwnd) ? TRUE : FALSE;
}

BOOL
EnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    )

/*++

Routine Description:

    Enable or disable a given named privilege.

Arguments:

    PrivilegeName - supplies the name of a system privilege.

    Enable - flag indicating whether to enable or disable the privilege.

Return Value:

    Boolean value indicating whether the operation was successful.

--*/

{
    HANDLE Token;
    BOOL b;
    TOKEN_PRIVILEGES NewPrivileges;
    LUID Luid;

    if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&Token)) {
        return(FALSE);
    }

    if(!LookupPrivilegeValue(NULL,PrivilegeName,&Luid)) {
        CloseHandle(Token);
        return(FALSE);
    }

    NewPrivileges.PrivilegeCount = 1;
    NewPrivileges.Privileges[0].Luid = Luid;
    NewPrivileges.Privileges[0].Attributes = Enable ? SE_PRIVILEGE_ENABLED : 0;

    b = AdjustTokenPrivileges(
            Token,
            FALSE,
            &NewPrivileges,
            0,
            NULL,
            NULL
            );

    CloseHandle(Token);

    return(b);
}
