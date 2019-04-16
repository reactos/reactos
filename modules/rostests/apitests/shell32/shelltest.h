#ifndef _SHELLTEST_H_
#define _SHELLTEST_H_

//#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <apitest.h>
#include <winreg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <atlbase.h>
#include <atlcom.h>

VOID PathToIDList(LPCWSTR pszPath, ITEMIDLIST** ppidl);

#endif /* !_SHELLTEST_H_ */
