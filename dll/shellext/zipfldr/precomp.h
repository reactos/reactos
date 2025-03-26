#ifndef ZIPFLDR_PRECOMP_H
#define ZIPFLDR_PRECOMP_H

#define COBJMACROS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlconv.h>
#include <atlstr.h>
#include <ui/rosdlgs.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <strsafe.h>
// debug.h needs this:
#define NTSTATUS LONG
#include <reactos/debug.h>
#include <shellutils.h>

#define EXTRACT_VERBA "extract"
#define EXTRACT_VERBW L"extract"

EXTERN_C const GUID CLSID_ZipFolderStorageHandler;
EXTERN_C const GUID CLSID_ZipFolderSendTo;
EXTERN_C const GUID CLSID_ZipFolderContextMenu;
EXTERN_C const GUID CLSID_ZipFolderRightDragHandler;
EXTERN_C const GUID CLSID_ZipFolderDropHandler;

EXTERN_C const GUID CLSID_ZipFolderExtractAllCommand;

extern LONG g_ModuleRefCnt;

UINT GetZipCodePage(BOOL bUnZip);
WCHAR* guid2string(REFCLSID iid);

#define MINIZIP_COMPATIBLE_VERSION 36
#define MINIZIP_PASSWORD_FLAG   1
#define MINIZIP_UTF8_FLAG       (1 << 11)

#include "minizip/unzip.h"
#include "minizip/ioapi.h"

extern zlib_filefunc64_def g_FFunc;

#include "resource.h"

#include "zippidl.hpp"
#include "IZip.hpp"

HRESULT _CEnumZipContents_CreateInstance(IZip* zip, DWORD flags, PCWSTR prefix, REFIID riid, LPVOID * ppvOut);
HRESULT _CExplorerCommandProvider_CreateInstance(IContextMenu* zipObject, REFIID riid, LPVOID * ppvOut);
HRESULT _CFolderViewCB_CreateInstance(REFIID riid, LPVOID * ppvOut);
void _CZipExtract_runWizard(PCWSTR Filename);

enum eZipPasswordResponse
{
    eAbort,
    eSkip,
    eAccept,
};

eZipPasswordResponse _CZipAskPassword(HWND hDlg, PCWSTR filename, CStringA& Password);

enum eZipConfirmResponse
{
    eYes,
    eYesToAll,
    eNo,
    eCancel
};

eZipConfirmResponse _CZipAskReplace(HWND hDlg, PCWSTR FullPath);

enum eZipExtractError
{
    eNoError,
    eExtractAbort,
    eDirectoryError,
    eFileError,
    eOpenError,
    eUnpackError,
};

#include "CZipEnumerator.hpp"
#include "CZipFolder.hpp"
#include "CZipCreator.hpp"
#include "CSendToZip.hpp"

#endif /* ZIPFLDR_PRECOMP_H */
