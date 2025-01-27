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


#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
CheckDiskSpace(VOID)
{
    FIXME("CheckDiskSpace() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
SHReValidateDarwinCache(VOID)
{
    FIXME("SHReValidateDarwinCache() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C FILEDESCRIPTOR*
WINAPI
GetFileDescriptor(FILEGROUPDESCRIPTOR *pFileGroupDesc, BOOL bUnicode, INT iIndex, LPWSTR lpName)
{
    FIXME("GetFileDescriptor() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
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
EXTERN_C BOOL
WINAPI
MakeShellURLFromPathA(LPCSTR lpPath, LPSTR lpUrl, INT cchMax)
{
    WCHAR szPath[MAX_PATH], szURL[MAX_PATH];
    BOOL ret;
    SHAnsiToUnicode(lpPath, szPath, _countof(szPath));
    ret = MakeShellURLFromPathW(szPath, szURL, _countof(szURL));
    SHUnicodeToAnsi(szURL, lpUrl, cchMax);
    return ret;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHParseDarwinIDFromCacheW(LPCWSTR lpUnknown1, LPWSTR lpUnknown2)
{
    FIXME("SHParseDarwinIDFromCacheW() stub\n");
    lpUnknown2 = NULL;
    return E_FAIL;
}

static HRESULT DataObject_GetHIDACount(IDataObject *pdo)
{
    if (!pdo)
        return E_INVALIDARG;
    CDataObjectHIDA cida(pdo);
    HRESULT hr = cida.hr();
    return SUCCEEDED(hr) ? cida->cidl : hr;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHMultiFileProperties(IDataObject *pDataObject, DWORD dwFlags)
{
    FIXME("SHMultiFileProperties() stub\n");

    // Temporary workaround to display a property sheet if possible
    if (DataObject_GetHIDACount(pDataObject) == 1)
        return SHELL32_ShowPropertiesDialog(pDataObject);

    if (pDataObject)
    {
        HWND hWnd;
        if (FAILED(IUnknown_GetWindow(pDataObject, &hWnd))) // Will probably not work but we have no other option
            hWnd = NULL;
        SHELL_ErrorBox(hWnd, HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }  

    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
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
EXTERN_C HLOCAL
WINAPI
CheckWinIniForAssocs(VOID)
{
    FIXME("CheckWinIniForAssocs() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
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
EXTERN_C HRESULT
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
EXTERN_C VOID
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
EXTERN_C HRESULT
WINAPI
CDefFolderMenu_Create(LPITEMIDLIST pidlFolder,
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
EXTERN_C VOID
WINAPI
SHWaitOp_Operate(LPVOID lpUnknown1, DWORD dwUnknown2)
{
    FIXME("SHWaitOp_Operate() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
RealDriveTypeFlags(INT iDrive, BOOL bUnknown)
{
    FIXME("RealDriveTypeFlags() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
EXTERN_C LONG
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
EXTERN_C BOOL
WINAPI
SheShortenPathW(LPWSTR lpPath, BOOL bShorten)
{
    FIXME("SheShortenPathW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
SheShortenPathA(LPSTR lpPath, BOOL bShorten)
{
    BOOL ret;
    WCHAR szPath[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpPath, -1, szPath, _countof(szPath));
    ret = SheShortenPathW(szPath, bShorten);
    WideCharToMultiByte(CP_ACP, 0, szPath, -1, lpPath, MAX_PATH, NULL, NULL);
    return ret;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheSetCurDrive(INT iIndex)
{
    FIXME("SheSetCurDrive() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheGetPathOffsetW(LPWSTR lpPath)
{
    FIXME("SheGetPathOffsetW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
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
EXTERN_C INT
WINAPI
SheGetCurDrive(VOID)
{
    FIXME("SheGetCurDrive() stub\n");
    return 1;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheFullPathW(LPWSTR lpFullName, DWORD dwPathSize, LPWSTR lpBuffer)
{
    FIXME("SheFullPathW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheFullPathA(LPSTR lpFullName, DWORD dwPathSize, LPSTR lpBuffer)
{
    FIXME("SheFullPathA() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
SheConvertPathW(LPWSTR lpCmd, LPWSTR lpFileName, UINT uCmdLen)
{
    FIXME("SheConvertPathW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheChangeDirExW(LPWSTR lpDir)
{
    FIXME("SheChangeDirExW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SheChangeDirExA(LPSTR lpDir)
{
    FIXME("SheChangeDirExA() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
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
EXTERN_C BOOL
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
EXTERN_C BOOL
WINAPI
SHCreateProcessAsUserW(PSHCREATEPROCESSINFOW pscpi)
{
    FIXME("SHCreateProcessAsUserW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
PrintersGetCommand_RunDLL(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLL() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
PrintersGetCommand_RunDLLA(HWND hwnd, HINSTANCE hInstance, LPSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLLA() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
PrintersGetCommand_RunDLLW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("PrintersGetCommand_RunDLLW() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C IShellFolderViewCB*
WINAPI
SHGetShellFolderViewCB(HWND hwnd)
{
    FIXME("SHGetShellFolderViewCB() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SHLookupIconIndexA(LPCSTR lpName, INT iIndex, UINT uFlags)
{
    FIXME("SHLookupIconIndexA() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
SHLookupIconIndexW(LPCWSTR lpName, INT iIndex, UINT uFlags)
{
    FIXME("SHLookupIconIndexW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C HANDLE
WINAPI
PifMgr_OpenProperties(LPCWSTR lpAppPath, LPCWSTR lpPifPath, UINT hInfIndex, UINT options)
{
    FIXME("PifMgr_OpenProperties() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
PifMgr_GetProperties(HANDLE hHandle, LPCSTR lpName, LPVOID lpUnknown, INT iUnknown, UINT uUnknown)
{
    FIXME("PifMgr_GetProperties() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C INT
WINAPI
PifMgr_SetProperties(HANDLE hHandle, LPCSTR lpName, LPCVOID lpUnknown, INT iUnknown, UINT uUnknown)
{
    FIXME("PifMgr_SetProperties() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHStartNetConnectionDialogW(
    _In_ HWND hwnd,
    _In_ LPCWSTR pszRemoteName,
    _In_ DWORD dwType)
{
    FIXME("SHStartNetConnectionDialogW() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C HANDLE
WINAPI
PifMgr_CloseProperties(HANDLE hHandle, UINT uUnknown)
{
    FIXME("PifMgr_CloseProperties() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
DAD_DragEnterEx2(HWND hwndTarget,
                 POINT ptStart,
                 IDataObject *pdtObject)
{
    FIXME("DAD_DragEnterEx2() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C UINT
WINAPI
SHGetNetResource(LPVOID lpUnknown1, UINT iIndex, LPVOID lpUnknown2, UINT cchMax)
{
    FIXME("SHGetNetResource() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
DragQueryInfo(HDROP hDrop, DRAGINFO *pDragInfo)
{
    FIXME("DragQueryInfo() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C LPVOID
WINAPI
DDECreatePostNotify(LPVOID lpUnknown)
{
    FIXME("DDECreatePostNotify() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
AppCompat_RunDLLW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("AppCompat_RunDLLW() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
Control_RunDLLAsUserW(HWND hwnd, HINSTANCE hInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    FIXME("Control_RunDLLAsUserW() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C UINT
WINAPI
DragQueryFileAorW(HDROP hDrop, UINT iIndex, LPWSTR lpFile, UINT ucb, BOOL bUnicode, BOOL bShorten)
{
    FIXME("DragQueryFileAorW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C DWORD
WINAPI
SHNetConnectionDialog(HWND hwndOwner,
                      LPCWSTR lpstrRemoteName,
                      DWORD dwType)
{
    FIXME("SHNetConnectionDialog() stub\n");
    return ERROR_INVALID_PARAMETER;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
DAD_SetDragImageFromListView(HWND hwnd, POINT pt)
{
    FIXME("DAD_SetDragImageFromListView() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C void
WINAPI
SHHandleDiskFull(HWND hwndOwner, UINT uDrive)
{
    FIXME("SHHandleDiskFull() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
ILGetPseudoNameW(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, LPWSTR szStr, INT iUnknown)
{
    /* Unimplemented in WinXP SP3 */
    TRACE("ILGetPseudoNameW() stub\n");
    *szStr = 0;
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C VOID
WINAPI
SHGlobalDefect(DWORD dwUnknown)
{
    /* Unimplemented in WinXP SP3 */
    TRACE("SHGlobalDefect() stub\n");
}

/*
 * Unimplemented
 */
EXTERN_C LPITEMIDLIST
WINAPI
Printers_GetPidl(LPCITEMIDLIST pidl, LPCWSTR lpName, DWORD dwUnknown1, DWORD dwUnknown2)
{
    FIXME("Printers_GetPidl() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C LONG
WINAPI
Printers_AddPrinterPropPages(LPVOID lpUnknown1, LPVOID lpUnknown2)
{
    FIXME("Printers_AddPrinterPropPages() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C WORD
WINAPI
ExtractIconResInfoW(
    _In_ HANDLE hHandle,
    _In_ LPCWSTR lpFileName,
    _In_ WORD wIndex,
    _Out_ LPWORD lpSize,
    _Out_ LPHANDLE lpIcon)
{
    FIXME("ExtractIconResInfoW() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C DWORD
WINAPI
ExtractVersionResource16W(LPWSTR lpName, LPHANDLE lpHandle)
{
    FIXME("ExtractVersionResource16W() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL*
WINAPI
FindExeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    FIXME("FindExeDlgProc() stub\n");
    return 0;
}

/*
 * Unimplemented
 */
EXTERN_C HANDLE
WINAPI
InternalExtractIconListW(HANDLE hHandle,
                         LPWSTR lpFileName,
                         LPINT lpCount)
{
    FIXME("InternalExtractIconListW() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C HANDLE
WINAPI
InternalExtractIconListA(HANDLE hHandle,
                         LPSTR lpFileName,
                         LPINT lpCount)
{
    FIXME("InternalExtractIconListA() stub\n");
    return NULL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
FirstUserLogon(LPWSTR lpUnknown1, LPWSTR lpUnknown2)
{
    FIXME("FirstUserLogon() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHSetFolderPathW(
    _In_ INT csidl,
    _In_ HANDLE hToken,
    _In_ DWORD dwFlags,
    _In_ LPCWSTR pszPath)
{
    FIXME("SHSetFolderPathW() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHGetUserPicturePathW(LPCWSTR lpPath, int csidl, LPVOID lpUnknown)
{
    FIXME("SHGetUserPicturePathW() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C HRESULT
WINAPI
SHSetUserPicturePathW(LPCWSTR lpPath, int csidl, LPVOID lpUnknown)
{
    FIXME("SHGetUserPicturePathA() stub\n");
    return E_FAIL;
}

/*
 * Unimplemented
 */
EXTERN_C BOOL
WINAPI
PathIsSlowW(
    _In_ LPCWSTR pszFile,
    _In_ DWORD dwAttr)
{
    FIXME("PathIsSlowW() stub\n");
    return FALSE;
}

/*
 * Unimplemented
 */
EXTERN_C DWORD
WINAPI
SHGetProcessDword(DWORD dwUnknown1, DWORD dwUnknown2)
{
    /* Unimplemented in WinXP SP3 */
    TRACE("SHGetProcessDword() stub\n");
    return 0;
}

EXTERN_C
DWORD WINAPI CheckStagingArea(VOID)
{
    /* Called by native explorer */
    return 0;
}
