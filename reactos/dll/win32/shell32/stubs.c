/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         shell32.dll
 * FILE:            dll/win32/shell32/stubs.c
 * PURPOSE:         shell32.dll stubs
 * PROGRAMMER:      Dmitry Chapyshev (dmitry@reactos.org)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      03/02/2009  Created
 */


#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalAlloc(UINT uFlags, SIZE_T uBytes)
{
    FIXME("SHLocalAlloc() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalFree(HLOCAL hMem)
{
    FIXME("SHLocalFree() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HLOCAL
WINAPI
SHLocalReAlloc(HLOCAL hMem,
               SIZE_T uBytes,
               UINT uFlags)
{
    FIXME("SHLocalReAlloc() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
AddCommasW(DWORD dwUnknown, LPWSTR lpNumber)
{
    LPWSTR lpRetBuf = L"0";

    FIXME("AddCommasW() stub\n");
    return lpRetBuf;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
ShortSizeFormatW(LONGLONG llNumber)
{
    FIXME("ShortSizeFormatW() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHFindComputer(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    FIXME("SHFindComputer() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHLimitInputEdit(HWND hWnd, LPVOID lpUnknown)
{
    FIXME("SHLimitInputEdit() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHLimitInputCombo(HWND hWnd, LPVOID lpUnknown)
{
    FIXME("SHLimitInputCombo() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
PathIsEqualOrSubFolder(LPWSTR lpFolder, LPWSTR lpSubFolder)
{
    FIXME("PathIsEqualOrSubFolder() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHCreateFileExtractIconW(LPCWSTR pszPath,
                         DWORD dwFileAttributes,
                         LPVOID lpUnknown1,
                         LPVOID lpUnknown2)
{
    FIXME("SHCreateFileExtractIconW() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
VOID
WINAPI
CheckDiskSpace(VOID)
{
    FIXME("CheckDiskSpace() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
SHReValidateDarwinCache(VOID)
{
    FIXME("SHReValidateDarwinCache() stub\n");
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
CopyStreamUI(IStream *pSrc, IStream *pDst, IProgressDialog *pProgDlg)
{
    FIXME("CopyStreamUI() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
FILEDESCRIPTOR*
WINAPI
GetFileDescriptor(FILEGROUPDESCRIPTOR *pFileGroupDesc, BOOL bUnicode, INT iIndex, LPWSTR lpName)
{
    FIXME("GetFileDescriptor() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHIsTempDisplayMode(VOID)
{
    FIXME("SHIsTempDisplayMode() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
LONG
WINAPI
SHCreateSessionKey(REGSAM regSam, PHKEY phKey)
{
    FIXME("SHCreateSessionKey() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
MakeShellURLFromPathW(LPCWSTR lpPath, LPWSTR lpUrl, INT cchMax)
{
    FIXME("MakeShellURLFromPathW() stub\n");
    lpUrl = NULL;
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
MakeShellURLFromPathA(LPCSTR lpPath, LPSTR lpUrl, INT cchMax)
{
    FIXME("MakeShellURLFromPathA() stub\n");
    lpUrl = NULL;
    return FALSE;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHParseDarwinIDFromCacheW(LPCWSTR lpUnknown1, LPWSTR lpUnknown2)
{
    FIXME("SHParseDarwinIDFromCacheW() stub\n");
    lpUnknown2 = NULL;
    return E_FAIL;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHMultiFileProperties(IDataObject *pDataObject, DWORD dwFlags)
{
    FIXME("SHMultiFileProperties() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHCreatePropertyBag(REFIID refIId, LPVOID *lpUnknown)
{
    /* Call SHCreatePropertyBagOnMemory() from shlwapi.dll */
    FIXME("SHCreatePropertyBag() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHCopyMonikerToTemp(IMoniker *pMoniker, LPCWSTR lpInput, LPWSTR lpOutput, INT cchMax)
{
    /* Unimplemented in XP SP3 */
    TRACE("SHCopyMonikerToTemp() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
HLOCAL
WINAPI
CheckWinIniForAssocs(VOID)
{
    FIXME("CheckWinIniForAssocs() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHGetSetFolderCustomSettingsW(LPSHFOLDERCUSTOMSETTINGSW pfcs,
                              LPCWSTR pszPath,
                              DWORD dwReadWrite)
{
    FIXME("SHGetSetFolderCustomSettingsW() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHGetSetFolderCustomSettingsA(LPSHFOLDERCUSTOMSETTINGSA pfcs,
                              LPCSTR pszPath,
                              DWORD dwReadWrite)
{
    FIXME("SHGetSetFolderCustomSettingsA() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHOpenPropSheetA(LPSTR lpCaption,
                 HKEY hKeys[],
                 UINT uCount,
                 CLSID *pClsID,
                 IDataObject *pDataObject,
                 IShellBrowser *pShellBrowser,
                 LPSTR lpStartPage)
{
    FIXME("SHOpenPropSheetA() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHOpenPropSheetW(LPWSTR lpCaption,
                 HKEY hKeys[],
                 UINT uCount,
                 CLSID *pClsID,
                 IDataObject *pDataObject,
                 IShellBrowser *pShellBrowser,
                 LPWSTR lpStartPage)
{
    FIXME("SHOpenPropSheetW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
VOID
WINAPI
CDefFolderMenu_MergeMenu(HINSTANCE hInstance,
                         UINT uMainMerge,
                         UINT uPopupMerge,
                         LPQCMINFO lpQcmInfo)
{
    FIXME("CDefFolderMenu_MergeMenu() stub\n");
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
CDefFolderMenu_Create(PCIDLIST_ABSOLUTE pidlFolder,
                      HWND hwnd,
                      UINT uidl,
                      PCUITEMID_CHILD_ARRAY *apidl,
                      IShellFolder *psf,
                      LPFNDFMCALLBACK lpfn,
                      HKEY hProgID,
                      HKEY hBaseProgID,
                      IContextMenu **ppcm)
{
    FIXME("CDefFolderMenu_Create() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHChangeRegistrationReceive(LPVOID lpUnknown1, DWORD dwUnknown2)
{
    FIXME("SHChangeRegistrationReceive() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
VOID
WINAPI
SHWaitOp_Operate(LPVOID lpUnknown1, DWORD dwUnknown2)
{
    FIXME("SHWaitOp_Operate() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
SHChangeNotifyReceive(LONG lUnknown, UINT uUnknown, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    FIXME("SHChangeNotifyReceive() stub\n");
}

/*
 * Unimplemented
 */
INT
WINAPI
RealDriveTypeFlags(INT iDrive, BOOL bUnknown)
{
    FIXME("RealDriveTypeFlags() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
HINSTANCE
WINAPI
WOWShellExecute(HWND hwnd,
                LPCWSTR lpOperation,
                LPCWSTR lpFile,
                LPCWSTR lpParameters,
                LPCWSTR lpDirectory,
                INT nShowCmd,
                void *lpfnCBWinExec)
{
    FIXME("WOWShellExecute() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
StrRStrW(LPWSTR lpSrc, LPWSTR lpLast, LPWSTR lpSearch)
{
    FIXME("StrRStrW() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
StrRStrA(LPSTR lpSrc, LPSTR lpLast, LPSTR lpSearch)
{
    FIXME("StrRStrA() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LONG
WINAPI
ShellHookProc(INT iCode, WPARAM wParam, LPARAM lParam)
{
    /* Unimplemented in WinXP SP3 */
    TRACE("ShellHookProc() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
VOID
WINAPI
ShellExec_RunDLL(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("ShellExec_RunDLL() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
ShellExec_RunDLLA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow)
{
    FIXME("ShellExec_RunDLLA() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
ShellExec_RunDLLW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("ShellExec_RunDLLW() stub\n");
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SheShortenPathW(LPWSTR lpPath, BOOL bShorten)
{
    FIXME("SheShortenPathW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SheShortenPathA(LPSTR lpPath, BOOL bShorten)
{
    FIXME("SheShortenPathA() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheSetCurDrive(INT iIndex)
{
    FIXME("SheSetCurDrive() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
LPWSTR
WINAPI
SheRemoveQuotesW(LPWSTR lpInput)
{
    FIXME("SheRemoveQuotesW() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
LPSTR
WINAPI
SheRemoveQuotesA(LPSTR lpInput)
{
    FIXME("SheRemoveQuotesA() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheGetPathOffsetW(LPWSTR lpPath)
{
    FIXME("SheGetPathOffsetW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SheGetDirExW(LPWSTR lpDrive,
             LPDWORD lpCurDirLen,
             LPWSTR lpCurDir)
{
    FIXME("SheGetDirExW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheGetCurDrive(VOID)
{
    FIXME("SheGetCurDrive() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheFullPathW(LPWSTR lpFullName, DWORD dwPathSize, LPWSTR lpBuffer)
{
    FIXME("SheFullPathW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheFullPathA(LPSTR lpFullName, DWORD dwPathSize, LPSTR lpBuffer)
{
    FIXME("SheFullPathA() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SheConvertPathW(LPWSTR lpCmd, LPWSTR lpFileName, UINT uCmdLen)
{
    FIXME("SheConvertPathW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheChangeDirExW(LPWSTR lpDir)
{
    FIXME("SheChangeDirExW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
INT
WINAPI
SheChangeDirExA(LPSTR lpDir)
{
    FIXME("SheChangeDirExA() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHInvokePrinterCommandW(HWND hwnd,
                        UINT uAction,
                        LPCWSTR lpBuf1,
                        LPCWSTR lpBuf2,
                        BOOL fModal)
{
    FIXME("SHInvokePrinterCommandW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHInvokePrinterCommandA(HWND hwnd,
                        UINT uAction,
                        LPCSTR lpBuf1,
                        LPCSTR lpBuf2,
                        BOOL fModal)
{
    FIXME("SHInvokePrinterCommandA() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
HRESULT
WINAPI
SHCreateQueryCancelAutoPlayMoniker(IMoniker **ppmoniker)
{
    FIXME("SHCreateQueryCancelAutoPlayMoniker() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHCreateProcessAsUserW(PSHCREATEPROCESSINFOW pscpi)
{
    FIXME("SHCreateProcessAsUserW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
SHChangeNotifySuspendResume(BOOL bSuspend, 
                            LPITEMIDLIST pidl, 
                            BOOL bRecursive, 
                            DWORD dwReserved)
{
    FIXME("SHChangeNotifySuspendResume() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
BOOL
WINAPI
RegenerateUserEnvironment(LPVOID *lpUnknown, BOOL bUnknown)
{
    FIXME("RegenerateUserEnvironment() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
HINSTANCE
WINAPI
RealShellExecuteExA(HWND hwnd,
                    LPCSTR lpOperation,
                    LPCSTR lpFile,
                    LPCSTR lpParameters,
                    LPCSTR lpDirectory,
                    LPSTR lpReturn,
                    LPCSTR lpTitle,
                    LPSTR lpReserved,
                    WORD nShowCmd,
                    HANDLE *lpProcess,
                    DWORD dwFlags)
{
    FIXME("RealShellExecuteExA() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
HINSTANCE
WINAPI
RealShellExecuteExW(HWND hwnd,
                    LPCWSTR lpOperation,
                    LPCWSTR lpFile,
                    LPCWSTR lpParameters,
                    LPCWSTR lpDirectory,
                    LPWSTR lpReturn,
                    LPCWSTR lpTitle,
                    LPWSTR lpReserved,
                    WORD nShowCmd,
                    HANDLE *lpProcess,
                    DWORD dwFlags)
{
    FIXME("RealShellExecuteExW() stub\n");
    return NULL;
}

/*
 * Implemented
 */
HINSTANCE
WINAPI
RealShellExecuteA(HWND hwnd,
                  LPCSTR lpOperation,
                  LPCSTR lpFile,
                  LPCSTR lpParameters,
                  LPCSTR lpDirectory,
                  LPSTR lpReturn,
                  LPCSTR lpTitle,
                  LPSTR lpReserved,
                  WORD nShowCmd,
                  HANDLE *lpProcess)
{
    return RealShellExecuteExA(hwnd,
                               lpOperation,
                               lpFile,
                               lpParameters,
                               lpDirectory,
                               lpReturn,
                               lpTitle,
                               lpReserved,
                               nShowCmd,
                               lpProcess,
                               0);
}

/*
 * Implemented
 */
HINSTANCE
WINAPI
RealShellExecuteW(HWND hwnd,
                  LPCWSTR lpOperation,
                  LPCWSTR lpFile,
                  LPCWSTR lpParameters,
                  LPCWSTR lpDirectory,
                  LPWSTR lpReturn,
                  LPCWSTR lpTitle,
                  LPWSTR lpReserved,
                  WORD nShowCmd,
                  HANDLE *lpProcess)
{
    return RealShellExecuteExW(hwnd,
                               lpOperation,
                               lpFile,
                               lpParameters,
                               lpDirectory,
                               lpReturn,
                               lpTitle,
                               lpReserved,
                               nShowCmd,
                               lpProcess,
                               0);
}

/*
 * Unimplemented
 */
VOID
WINAPI
PrintersGetCommand_RunDLL(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLL() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
PrintersGetCommand_RunDLLA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLLA() stub\n");
}

/*
 * Unimplemented
 */
VOID
WINAPI
PrintersGetCommand_RunDLLW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLLW() stub\n");
}

/*
 * Unimplemented
 */
IShellFolderViewCB*
WINAPI
SHGetShellFolderViewCB(HWND hwnd)
{
    FIXME("SHGetShellFolderViewCB() stub\n");
    return NULL;
}
