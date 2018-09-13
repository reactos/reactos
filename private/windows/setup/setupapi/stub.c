/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    stub.c

Abstract:

    Dynamic loading of routines that are implemented differently on Win9x and NT.

Author:

    Jim Schmidt (jimschm) 29-Apr-1997

Revision History:

    jimschm 26-Oct-1998     Added cfgmgr32, crypt32, mscat and wintrust APIs

--*/

#include "precomp.h"


//
// Stub & emulation prototypes -- implemented below
//

GETFILEATTRIBUTESEXA_PROTOTYPE EmulatedGetFileAttributesExA;
GETFILEATTRIBUTESEXW_PROTOTYPE EmulatedGetFileAttributesExW;

//
// Function ptr declarations.  When adding, prefix the function ptr with
// Dyn_ to indicate a dynamically loaded version of an API.
//

GETFILEATTRIBUTESEXA_PROC Dyn_GetFileAttributesExA;
GETFILEATTRIBUTESEXW_PROC Dyn_GetFileAttributesExW;
GETSYSTEMWINDOWSDIRECTORYA_PROC Dyn_GetSystemWindowsDirectoryA;
GETSYSTEMWINDOWSDIRECTORYW_PROC Dyn_GetSystemWindowsDirectoryW;

#ifdef ANSI_SETUPAPI

CM_QUERY_RESOURCE_CONFLICT_LIST Dyn_CM_Query_Resource_Conflict_List;
CM_FREE_RESOURCE_CONFLICT_HANDLE Dyn_CM_Free_Resource_Conflict_Handle;
CM_GET_RESOURCE_CONFLICT_COUNT Dyn_CM_Get_Resource_Conflict_Count;
CM_GET_RESOURCE_CONFLICT_DETAILSA Dyn_CM_Get_Resource_Conflict_DetailsA;
CM_GET_CLASS_REGISTRY_PROPERTYA Dyn_CM_Get_Class_Registry_PropertyA;
CM_SET_CLASS_REGISTRY_PROPERTYA Dyn_CM_Set_Class_Registry_PropertyA;
CM_GET_DEVICE_INTERFACE_ALIAS_EXA Dyn_CM_Get_Device_Interface_Alias_ExA;
CM_GET_DEVICE_INTERFACE_LIST_EXA Dyn_CM_Get_Device_Interface_List_ExA;
CM_GET_DEVICE_INTERFACE_LIST_SIZE_EXA Dyn_CM_Get_Device_Interface_List_Size_ExA;
CM_GET_LOG_CONF_PRIORITY_EX Dyn_CM_Get_Log_Conf_Priority_Ex;
CM_QUERY_AND_REMOVE_SUBTREE_EXA Dyn_CM_Query_And_Remove_SubTree_ExA;
CM_REGISTER_DEVICE_INTERFACE_EXA Dyn_CM_Register_Device_Interface_ExA;
CM_SET_DEVNODE_PROBLEM_EX Dyn_CM_Set_DevNode_Problem_Ex;
CM_UNREGISTER_DEVICE_INTERFACE_EXA Dyn_CM_Unregister_Device_Interface_ExA;

CRYPTCATADMINACQUIRECONTEXT Dyn_CryptCATAdminAcquireContext;
CRYPTCATADMINRELEASECONTEXT Dyn_CryptCATAdminReleaseContext;
CRYPTCATADMINRELEASECATALOGCONTEXT Dyn_CryptCATAdminReleaseCatalogContext;
CRYPTCATADMINADDCATALOG Dyn_CryptCATAdminAddCatalog;
CRYPTCATCATALOGINFOFROMCONTEXT Dyn_CryptCATCatalogInfoFromContext;
CRYPTCATADMINCALCHASHFROMFILEHANDLE Dyn_CryptCATAdminCalcHashFromFileHandle;
CRYPTCATADMINENUMCATALOGFROMHASH Dyn_CryptCATAdminEnumCatalogFromHash;

CERTFREECERTIFICATECONTEXT CertFreeCertificateContext;

WINVERIFYTRUST WinVerifyTrust;

#endif

//
// Private functions
//

FARPROC ObtainFnPtr (PCSTR, PCSTR, FARPROC);


VOID
InitializeStubFnPtrs (
    VOID
    )

/*++

Routine Description:

    This routine tries to load the function ptr of OS-provided APIs, and if
    they aren't available, stub versions are used instead.  We do this
    for APIs that are unimplemented on a platform that setupapi will
    run on.

Arguments:

    none

Return Value:

    none

--*/

{
    //
    // GetFileAttributesEx - try loading from the OS dll, and if the DLL
    // doesn't exist, use an emulation version
    //

    (FARPROC) Dyn_GetFileAttributesExA = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetFileAttributesExA",
                                                (FARPROC) EmulatedGetFileAttributesExA
                                                );

    (FARPROC) Dyn_GetFileAttributesExW = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetFileAttributesExW",
                                                (FARPROC) EmulatedGetFileAttributesExW
                                                );
    //
    // if the hydra GetSystemWindowsDirectory functions exist, use them, otherwise use GetWindowsDirectory
    //
    (FARPROC) Dyn_GetSystemWindowsDirectoryA = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetSystemWindowsDirectoryA",
                                                (FARPROC) GetWindowsDirectoryA
                                                );

    (FARPROC) Dyn_GetSystemWindowsDirectoryW = ObtainFnPtr (
                                                "kernel32.dll",
                                                "GetSystemWindowsDirectoryW",
                                                (FARPROC) GetWindowsDirectoryW
                                                );
    

#ifdef ANSI_SETUPAPI

    //
    // use Win9x config manager APIs if they exist, otherwise return ERROR_CALL_NOT_IMPLEMENTED
    //
    (FARPROC) Dyn_CM_Get_Class_Registry_PropertyA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Class_Registry_PropertyA",
                                                        (FARPROC) Stub_CM_Get_Class_Registry_PropertyA
                                                        );

    (FARPROC) Dyn_CM_Set_Class_Registry_PropertyA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Set_Class_Registry_PropertyA",
                                                        (FARPROC) Stub_CM_Set_Class_Registry_PropertyA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_Alias_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_Alias_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_Alias_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_List_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_List_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_List_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Device_Interface_List_Size_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Device_Interface_List_Size_ExA",
                                                        (FARPROC) Stub_CM_Get_Device_Interface_List_Size_ExA
                                                        );

    (FARPROC) Dyn_CM_Get_Log_Conf_Priority_Ex = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Log_Conf_Priority_Ex",
                                                        (FARPROC) Stub_CM_Get_Log_Conf_Priority_Ex
                                                        );

    (FARPROC) Dyn_CM_Query_And_Remove_SubTree_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Query_And_Remove_SubTree_ExA",
                                                        (FARPROC) Stub_CM_Query_And_Remove_SubTree_ExA
                                                        );

    (FARPROC) Dyn_CM_Register_Device_Interface_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Register_Device_Interface_ExA",
                                                        (FARPROC) Stub_CM_Register_Device_Interface_ExA
                                                        );

    (FARPROC) Dyn_CM_Set_DevNode_Problem_Ex = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Set_DevNode_Problem_Ex",
                                                        (FARPROC) Stub_CM_Set_DevNode_Problem_Ex
                                                        );

    (FARPROC) Dyn_CM_Unregister_Device_Interface_ExA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Unregister_Device_Interface_ExA",
                                                        (FARPROC) Stub_CM_Unregister_Device_Interface_ExA
                                                        );

    (FARPROC)Dyn_CM_Query_Resource_Conflict_List = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Query_Resource_Conflict_List",
                                                        (FARPROC) Stub_CM_Query_Resource_Conflict_List
                                                        );

    (FARPROC)Dyn_CM_Free_Resource_Conflict_Handle = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Free_Resource_Conflict_Handle",
                                                        (FARPROC) Stub_CM_Free_Resource_Conflict_Handle
                                                        );

    (FARPROC)Dyn_CM_Get_Resource_Conflict_Count = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Resource_Conflict_Count",
                                                        (FARPROC) Stub_CM_Get_Resource_Conflict_Count
                                                        );

    (FARPROC)Dyn_CM_Get_Resource_Conflict_DetailsA = ObtainFnPtr (
                                                        "cfgmgr32.dll",
                                                        "CM_Get_Resource_Conflict_DetailsA",
                                                        (FARPROC) Stub_CM_Get_Resource_Conflict_DetailsA
                                                        );
    
    (FARPROC) Dyn_CryptCATAdminAcquireContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminAcquireContext",
                                                        (FARPROC) Stub_CryptCATAdminAcquireContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminReleaseContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminReleaseContext",
                                                        (FARPROC) Stub_CryptCATAdminReleaseContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminReleaseCatalogContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminReleaseCatalogContext",
                                                        (FARPROC) Stub_CryptCATAdminReleaseCatalogContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminAddCatalog = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminAddCatalog",
                                                        (FARPROC) Stub_CryptCATAdminAddCatalog
                                                        );

    (FARPROC) Dyn_CryptCATCatalogInfoFromContext = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATCatalogInfoFromContext",
                                                        (FARPROC) Stub_CryptCATCatalogInfoFromContext
                                                        );

    (FARPROC) Dyn_CryptCATAdminCalcHashFromFileHandle = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminCalcHashFromFileHandle",
                                                        (FARPROC) Stub_CryptCATAdminCalcHashFromFileHandle
                                                        );

    (FARPROC) Dyn_CryptCATAdminEnumCatalogFromHash = ObtainFnPtr (
                                                        "wintrust.dll",
                                                        "CryptCATAdminEnumCatalogFromHash",
                                                        (FARPROC) Stub_CryptCATAdminCalcHashFromFileHandle
                                                        );

    (FARPROC) Dyn_CertFreeCertificateContext = ObtainFnPtr (
                                                        "crypt32.dll",
                                                        "CertFreeCertificateContext",
                                                        (FARPROC) Stub_CertFreeCertificateContext
                                                        );

    (FARPROC) Dyn_WinVerifyTrust = ObtainFnPtr (
                                        "wintrust.dll",
                                        "WinVerifyTrust",
                                        (FARPROC) Stub_WinVerifyTrust
                                        );


#endif

    //
    // ***Add other dynamic loading here***
    //
}


BOOL
EmulatedGetFileAttributesExA (
    IN      PCSTR FileName,
    IN      GET_FILEEX_INFO_LEVELS InfoLevelId,
    OUT     LPVOID FileInformation
    )

/*++

Routine Description:

    Implements an emulation of the NT-specific function GetFileAttributesEx.
    Basic exception handling is implemented, but parameters are not otherwise
    validated.

Arguments:

    FileName - Specifies file to get attributes for

    InfoLevelId - Must be GetFileExInfoStandard

    FileInformation - Must be a valid pointer to WIN32_FILE_ATTRIBUTE_DATA struct

Return Value:

    TRUE for success, FALSE for failure.  GetLastError provided error code.

--*/


{
    //
    // GetFileAttributesEx does not exist on Win95, and ANSI version of setupapi.dll
    // is required for Win9x to NT 5 upgrade
    //

    HANDLE FileEnum;
    WIN32_FIND_DATAA fd;
    PCSTR p;
    WIN32_FILE_ATTRIBUTE_DATA *FileAttribData = (WIN32_FILE_ATTRIBUTE_DATA *) FileInformation;

    __try {
        //
        // We only support GetFileExInfoStandard
        //

        if (InfoLevelId != GetFileExInfoStandard) {
            SetLastError (ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        //
        // Locate file name
        //

        // BUGBUG: Should be _mbsrchr
        p = strrchr (FileName, TEXT('\\'));
        if (!p) {
            p = FileName;
        }

        ZeroMemory (FileAttribData, sizeof (WIN32_FILE_ATTRIBUTE_DATA));

        FileEnum = FindFirstFileA (FileName, &fd);

        //
        // Prohibit caller-supplied pattern
        //

        if (FileEnum!=INVALID_HANDLE_VALUE && lstrcmpiA (p, fd.cFileName)) {
            FindClose (FileEnum);
            FileEnum = INVALID_HANDLE_VALUE;
            SetLastError (ERROR_INVALID_PARAMETER);
        }

        //
        // If exact match found, fill in the attributes
        //

        if (FileEnum) {
            FileAttribData->dwFileAttributes = fd.dwFileAttributes;
            FileAttribData->nFileSizeHigh = fd.nFileSizeHigh;
            FileAttribData->nFileSizeLow  = fd.nFileSizeLow;

            CopyMemory (&FileAttribData->ftCreationTime, &fd.ftCreationTime, sizeof (FILETIME));
            CopyMemory (&FileAttribData->ftLastAccessTime, &fd.ftLastAccessTime, sizeof (FILETIME));
            CopyMemory (&FileAttribData->ftLastWriteTime, &fd.ftLastWriteTime, sizeof (FILETIME));

            FindClose (FileEnum);
        }

        return FileEnum != INVALID_HANDLE_VALUE;
    }

    __except (TRUE) {
        //
        // If bogus FileInformation pointer is passed, an exception is thrown.
        //

        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }
}


BOOL
EmulatedGetFileAttributesExW (
    IN      PCWSTR FileName,
    IN      GET_FILEEX_INFO_LEVELS InfoLevelId,
    OUT     LPVOID FileInformation
    )
{
    BOOL b = FALSE;
    PSTR AnsiFileName;

    AnsiFileName = UnicodeToAnsi (FileName);
    if (AnsiFileName) {
        b = EmulatedGetFileAttributesExA (AnsiFileName, InfoLevelId, FileInformation);
        MyFree (AnsiFileName);
    }

    return b;
}


//
// DLL array structures
//

#define MAX_DLL_ARRAY   5

typedef struct {
    PCSTR DllName;
    HINSTANCE DllInst;
} DLLTABLE, *PDLLTABLE;

static INT g_ArraySize = 0;
static DLLTABLE g_DllArray[MAX_DLL_ARRAY];



FARPROC
ObtainFnPtr (
    IN      PCSTR DllName,
    IN      PCSTR ProcName,
    IN      FARPROC Default
    )

/*++

Routine Description:

    This routine manages an array of DLL instance handles and returns the
    proc address of the caller-specified routine.  The DLL is loaded
    and remains loaded until the DLL terminates.  This array is not
    synchronized.

Arguments:

    DllName - The ANSI DLL name to load

    ProcName - The ANSI procedure name to locate

    Default - The default procedure, if the export was not found

Return Value:

    The address of the requested function, or NULL if the DLL could not
    be loaded, or the function is not implemented in the loaded DLL.

--*/

{
    INT i;
    PSTR DupBuf;
    FARPROC Address = NULL;

    //
    // Search for loaded DLL
    //

    for (i = 0 ; i < g_ArraySize ; i++) {
        if (!lstrcmpiA (DllName, g_DllArray[i].DllName)) {
            break;
        }
    }

    do {
        //
        // If necessary, load the DLL
        //

        if (i == g_ArraySize) {
            if (g_ArraySize == MAX_DLL_ARRAY) {
                // Constant limit needs to be raised
                MYASSERT (FALSE);
                break;
            }

            g_DllArray[i].DllInst = LoadLibraryA (DllName);
            if (!g_DllArray[i].DllInst) {
                break;
            }

            DupBuf = (PSTR) MyMalloc (lstrlenA (DllName) + 1);
            if (!DupBuf) {
                break;
            }
            lstrcpyA (DupBuf, DllName);
            g_DllArray[i].DllName = DupBuf;

            g_ArraySize++;
        }

        //
        // Now that DLL is loaded, return the proc address if it exists
        //

        Address = GetProcAddress (g_DllArray[i].DllInst, ProcName);

    } while (FALSE);

    if (!Address) {
        return Default;
    }

    return Address;
}


VOID
pCleanUpDllArray (
    VOID
    )

/*++

Routine Description:

    Cleans up the DLL array resources.

Arguments:

    none

Return Value:

    none

--*/

{
    INT i;

    for (i = 0 ; i < g_ArraySize ; i++) {
        FreeLibrary (g_DllArray[i].DllInst);
        MyFree (g_DllArray[i].DllName);
    }

    g_ArraySize = 0;
}


VOID
CleanUpStubFns (
    VOID
    )

/*++

Routine Description:

    Cleans up all resources used by emulation routines and funciton pointer list.

Arguments:

    none

Return Value:

    none

--*/

{
    pCleanUpDllArray();
}



#ifdef ANSI_SETUPAPI

CONFIGRET
WINAPI
Stub_CM_Query_Resource_Conflict_List(
             OUT PCONFLICT_LIST pclConflictList,
             IN  DEVINST        dnDevInst,
             IN  RESOURCEID     ResourceID,
             IN  PCVOID         ResourceData,
             IN  ULONG          ResourceLen,
             IN  ULONG          ulFlags,
             IN  HMACHINE       hMachine
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Free_Resource_Conflict_Handle(
             IN CONFLICT_LIST   clConflictList
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Resource_Conflict_Count(
             IN CONFLICT_LIST   clConflictList,
             OUT PULONG         pulCount
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Resource_Conflict_DetailsA(
             IN CONFLICT_LIST         clConflictList,
             IN ULONG                 ulIndex,
             IN OUT PCONFLICT_DETAILS_A pConflictDetails
             )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Class_Registry_PropertyA(
    IN  LPGUID      ClassGUID,
    IN  ULONG       ulProperty,
    OUT PULONG      pulRegDataType,    OPTIONAL
    OUT PVOID       Buffer,            OPTIONAL
    IN  OUT PULONG  pulLength,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Set_Class_Registry_PropertyA(
    IN LPGUID      ClassGUID,
    IN ULONG       ulProperty,
    IN PCVOID      Buffer,       OPTIONAL
    IN ULONG       ulLength,
    IN ULONG       ulFlags,
    IN HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}

CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_Alias_ExA(
    IN     PCSTR   pszDeviceInterface,
    IN     LPGUID   AliasInterfaceGuid,
    OUT    PSTR    pszAliasDeviceInterface,
    IN OUT PULONG   pulLength,
    IN     ULONG    ulFlags,
    IN     HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_List_ExA(
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID,      OPTIONAL
    OUT PCHAR       Buffer,
    IN  ULONG       BufferLen,
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Device_Interface_List_Size_ExA(
    IN  PULONG      pulLen,
    IN  LPGUID      InterfaceClassGuid,
    IN  DEVINSTID_A pDeviceID,      OPTIONAL
    IN  ULONG       ulFlags,
    IN  HMACHINE    hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Get_Log_Conf_Priority_Ex(
    IN  LOG_CONF  lcLogConf,
    OUT PPRIORITY pPriority,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Query_And_Remove_SubTree_ExA(
    IN  DEVINST        dnAncestor,
    OUT PPNP_VETO_TYPE pVetoType,
    OUT PSTR          pszVetoName,
    IN  ULONG          ulNameLength,
    IN  ULONG          ulFlags,
    IN  HMACHINE       hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Register_Device_Interface_ExA(
    IN  DEVINST   dnDevInst,
    IN  LPGUID    InterfaceClassGuid,
    IN  PCSTR    pszReference,         OPTIONAL
    OUT PSTR     pszDeviceInterface,
    IN OUT PULONG pulLength,
    IN  ULONG     ulFlags,
    IN  HMACHINE  hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Set_DevNode_Problem_Ex(
    IN DEVINST   dnDevInst,
    IN ULONG     ulProblem,
    IN  ULONG    ulFlags,
    IN  HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


CONFIGRET
WINAPI
Stub_CM_Unregister_Device_Interface_ExA(
    IN PCSTR   pszDeviceInterface,
    IN ULONG    ulFlags,
    IN HMACHINE hMachine
    )
{
    return CR_CALL_NOT_IMPLEMENTED;
}


BOOL
WINAPI
Stub_CryptCATAdminAcquireContext (
    OUT HCATADMIN *phCatAdmin,
    IN const GUID *pgSubsystem,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminReleaseContext (
    IN HCATADMIN hCatAdmin,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminReleaseCatalogContext (
    IN HCATADMIN hCatAdmin,
    IN HCATINFO hCatInfo,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


HCATINFO
WINAPI
Stub_CryptCATAdminAddCatalog (
    IN HCATADMIN hCatAdmin,
    IN WCHAR *pwszCatalogFile,
    IN OPTIONAL WCHAR *pwszSelectBaseName,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATCatalogInfoFromContext (
    IN HCATINFO hCatInfo,
    IN OUT CATALOG_INFO *psCatInfo,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CryptCATAdminCalcHashFromFileHandle (
    IN HANDLE hFile,
    IN OUT DWORD *pcbHash,
    OUT OPTIONAL BYTE *pbHash,
    IN DWORD dwFlags
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


BOOL
WINAPI
Stub_CertFreeCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
    SetLastError (ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


LONG
WINAPI
Stub_WinVerifyTrust(
    HWND hwnd,
    GUID *pgActionID,
    LPVOID pWVTData
    )
{
    return ERROR_SUCCESS;
}


#pragma warning (disable:4273)

VOID
WINAPI
RtlAssert(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
    CHAR Buf[2048];

    wsprintfA (
        Buf,
        "\r\n*** Assertion failed: %s%s\r\n***   Source File: %s, line %u\r\n\r\n",
        Message ? Message : "",
        FailedAssertion,
        FileName,
        LineNumber
        );

    OutputDebugString (Buf);
}

//
// Aparently Required by dload.lib
//
ULONG
DbgPrint(
    PSTR Format,
    ...
    )
{
    va_list arglist;
    CHAR Buffer[512];
    INT cb;

    //
    // Format the output into a buffer and then print it.
    //

    va_start(arglist, Format);

    cb = _vsnprintf(Buffer, sizeof(Buffer), Format, arglist);
    if (cb == -1) {             // detect buffer overflow
        Buffer[sizeof(Buffer) - 3] = 0;
    }

    strcat (Buffer, "\r\n");

    OutputDebugString (Buffer);

    va_end(arglist);

    return 0;
}


#endif


