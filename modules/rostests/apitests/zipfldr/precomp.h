#ifndef _ZIPFLDR_APITEST_PRECOMP_H_
#define _ZIPFLDR_APITEST_PRECOMP_H_

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H


#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <atlbase.h>
#include <atlcom.h> // gcc needs to resolve unused template content
#include <shellapi.h>
#include <shlwapi.h>
#include <strsafe.h>

#include <shellutils.h>

#include <apitest.h>

#include "resource.h"

BOOL extract_resource(WCHAR* Filename, LPCWSTR ResourceName);
#define InitializeShellFolder(Filename, pFolder)            InitializeShellFolder_(__FILE__, __LINE__, Filename, pFolder)
bool InitializeShellFolder_(const char* file, int line, const WCHAR* Filename, CComPtr<IShellFolder>& spFolder);

#define IsFormatAdvertised(pDataObj, cfFormat, tymed)   IsFormatAdvertised_(__FILE__, __LINE__, pDataObj, cfFormat, tymed)
bool IsFormatAdvertised_(const char* file, int line, IDataObject* pDataObj, CLIPFORMAT cfFormat, TYMED tymed);


#endif /* _ZIPFLDR_APITEST_PRECOMP_H_ */
