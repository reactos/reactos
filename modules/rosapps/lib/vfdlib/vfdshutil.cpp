/*
	vfdshutil.cpp

	Virtual Floppy Drive for Windows
	Driver control library
	shell extension utility functions

	Copyright (c) 2003-2005 Ken Kato
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"
#include "vfdshcfact.h"

//=====================================
// Initialize the GUID instance
//=====================================

#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma data_seg(".text")
#endif
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "vfdshguid.h"
#if !defined(__REACTOS__) || defined(_MSC_VER)
#pragma data_seg()
#endif

//
//	Registry path to the approved shell extensions key
//
#define REGKEY_APPROVED \
	"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"


//=====================================
//	Shell extension library requirements
//=====================================

//
//	Creates a class factory instance
//
STDAPI DllGetClassObject(
	REFCLSID		rclsid,
	REFIID			riid,
	LPVOID			*ppvOut)
{
	VFDTRACE(0,
		("DllGetClassObject\n"));

	*ppvOut = NULL;

	if (IsEqualIID(rclsid, CLSID_VfdShellExt)) {
		CVfdFactory *pFactory = new CVfdFactory;

		if (!pFactory) {
			return E_OUTOFMEMORY;
		}

		return pFactory->QueryInterface(riid, ppvOut);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

//
//	DllCanUnloadNow
//
STDAPI DllCanUnloadNow(void)
{
	VFDTRACE(0,
		("DllCanUnloadNow - %s\n", (g_cDllRefCnt ? "No" : "Yes")));

	return (g_cDllRefCnt ? S_FALSE : S_OK);
}

//=====================================
// Shell extension register functions
//=====================================

static inline void MakeGuidString(LPTSTR str, const GUID &guid)
{
	sprintf(str, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3], guid.Data4[4],
		guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

//
//	Regster this dll as shell extention handlers
//
DWORD WINAPI VfdRegisterHandlers()
{
	TCHAR	buf[MAX_PATH];
	TCHAR	guid_str[40];
	HKEY	hKey;
	DWORD	temp;
	DWORD	ret;

	MakeGuidString(guid_str, CLSID_VfdShellExt);

	//
	//	Register the GUID in the CLSID subtree
	//
	sprintf(buf, "CLSID\\%s", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	ret = RegCreateKeyEx(
		HKEY_CLASSES_ROOT, buf, 0, NULL,
		0, KEY_ALL_ACCESS, NULL, &hKey, &temp);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	if (temp == REG_OPENED_EXISTING_KEY) {
		temp = sizeof(buf);

		ret = RegQueryValueEx(
			hKey, NULL, NULL, NULL, (PBYTE)buf, &temp);

		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			return ret;
		}

		if (_stricmp(buf, VFDEXT_DESCRIPTION)) {
			RegCloseKey(hKey);
			return ERROR_FILE_EXISTS;
		}
	}
	else {

		VFDTRACE(0, ("@=" VFDEXT_DESCRIPTION "\n"));

		ret = RegSetValueEx(hKey, NULL, NULL, REG_SZ,
			(PBYTE)VFDEXT_DESCRIPTION, sizeof(VFDEXT_DESCRIPTION));

		RegCloseKey(hKey);

		if (ret != ERROR_SUCCESS) {
			return ret;
		}
	}

	//
	//	Register the executable path
	//
	sprintf(buf, "CLSID\\%s\\InProcServer32", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	ret = RegCreateKeyEx(
		HKEY_CLASSES_ROOT, buf, 0, NULL,
		0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	temp = GetModuleFileName(g_hDllModule, buf, sizeof(buf));

	VFDTRACE(0, ("@=%s\n", buf));

	ret = RegSetValueEx(
		hKey, NULL, NULL, REG_SZ, (PBYTE)buf, temp + 1);

	if (ret != ERROR_SUCCESS) {
		RegCloseKey(hKey);
		return ret;
	}

	VFDTRACE(0, ("ThreadingModel=Apartment\n"));

	ret = RegSetValueEx(hKey, "ThreadingModel", NULL, REG_SZ,
		(PBYTE)"Apartment", sizeof("Apartment"));

	RegCloseKey(hKey);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//
	//	Register context menu handler
	//
	VFDTRACE(0, ("HKCR\\" VFDEXT_MENU_REGKEY "\n"));

	ret = RegCreateKeyEx(
		HKEY_CLASSES_ROOT, VFDEXT_MENU_REGKEY, 0, NULL,
		0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	VFDTRACE(0, ("@=%s\n", guid_str));

	ret = RegSetValueEx(hKey, NULL, NULL, REG_SZ,
		(PBYTE)guid_str, strlen(guid_str) + 1);

	RegCloseKey(hKey);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//
	//	Register Drag&Drop handler
	//
	if (!IS_WINDOWS_NT()) {
		//
		//	Windows NT does not support Drag&Drop handlers ???
		//
		VFDTRACE(0, ("HKCR\\" VFDEXT_DND_REGKEY "\n"));

		ret = RegCreateKeyEx(
			HKEY_CLASSES_ROOT, VFDEXT_DND_REGKEY, 0, NULL,
			0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

		if (ret != ERROR_SUCCESS) {
			return ret;
		}

		VFDTRACE(0, ("@=%s\n", guid_str));

		ret = RegSetValueEx(hKey, NULL, NULL, REG_SZ,
			(PBYTE)guid_str, strlen(guid_str) + 1);

		RegCloseKey(hKey);

		if (ret != ERROR_SUCCESS) {
			return ret;
		}
	}

	//
	//	Register property sheet handler
	//
	VFDTRACE(0, ("HKCR\\" VFDEXT_PROP_REGKEY "\n"));

	ret = RegCreateKeyEx(
		HKEY_CLASSES_ROOT, VFDEXT_PROP_REGKEY, 0, NULL,
		0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	VFDTRACE(0, ("@=%s\n", guid_str));

	ret = RegSetValueEx(hKey, NULL, NULL, REG_SZ,
		(PBYTE)guid_str, strlen(guid_str) + 1);

	RegCloseKey(hKey);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//
	//	Register approved extensions entry
	//
	VFDTRACE(0, ("HKLM\\" REGKEY_APPROVED "\n"));

	ret = RegCreateKeyEx(
		HKEY_LOCAL_MACHINE, REGKEY_APPROVED,
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	VFDTRACE(0,
		("%s=" VFDEXT_DESCRIPTION "\n", guid_str));

	ret = RegSetValueEx(hKey, guid_str, NULL, REG_SZ,
		(PBYTE)VFDEXT_DESCRIPTION, sizeof(VFDEXT_DESCRIPTION));

	RegCloseKey(hKey);

	return ret;
}

//
//	Unregister context menu handler
//
DWORD WINAPI VfdUnregisterHandlers()
{
	TCHAR	buf[MAX_PATH];
	TCHAR	guid_str[40];
	HKEY	hKey;
	DWORD	temp;
	DWORD	ret;

	MakeGuidString(guid_str, CLSID_VfdShellExt);

	sprintf(buf, "CLSID\\%s", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	temp = sizeof(buf);

	ret = RegQueryValue(HKEY_CLASSES_ROOT, buf, buf, (PLONG)&temp);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	if (_stricmp(buf, VFDEXT_DESCRIPTION)) {
		return ERROR_PATH_NOT_FOUND;
	}

	sprintf(buf, "CLSID\\%s\\InProcServer32", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	ret = RegDeleteKey(HKEY_CLASSES_ROOT, buf);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	sprintf(buf, "CLSID\\%s", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	ret = RegDeleteKey(HKEY_CLASSES_ROOT, buf);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	VFDTRACE(0, ("HKCR\\" VFDEXT_MENU_REGKEY "\n"));

	ret = RegDeleteKey(HKEY_CLASSES_ROOT, VFDEXT_MENU_REGKEY);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	if (!IS_WINDOWS_NT()) {

		//	Windows NT doesn't support Drag & Drop handlers ???

		VFDTRACE(0, ("HKCR\\" VFDEXT_DND_REGKEY "\n"));

		ret = RegDeleteKey(HKEY_CLASSES_ROOT, VFDEXT_DND_REGKEY);

		if (ret != ERROR_SUCCESS) {
			return ret;
		}
	}

	VFDTRACE(0, ("HKCR\\" VFDEXT_PROP_REGKEY "\n"));

	ret = RegDeleteKey(HKEY_CLASSES_ROOT, VFDEXT_PROP_REGKEY);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	VFDTRACE(0, ("HKLM\\" REGKEY_APPROVED "\n"));

	ret = RegCreateKeyEx(
		HKEY_LOCAL_MACHINE,
		REGKEY_APPROVED,
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, NULL);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	ret = RegDeleteValue(hKey, guid_str);

	RegCloseKey(hKey);

	return ret;
}

//
//	Check if context menu handler is registered
//
DWORD WINAPI VfdCheckHandlers()
{
	TCHAR	buf[MAX_PATH];
	TCHAR	guid_str[40];
	DWORD	temp;
	DWORD	ret;

	MakeGuidString(guid_str, CLSID_VfdShellExt);

	sprintf(buf, "CLSID\\%s", guid_str);

	VFDTRACE(0, ("HKCR\\%s\n", buf));

	temp = sizeof(buf);

	ret = RegQueryValue(HKEY_CLASSES_ROOT, buf, buf, (PLONG)&temp);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	if (_stricmp(buf, VFDEXT_DESCRIPTION)) {
		return ERROR_PATH_NOT_FOUND;
	}

	return ERROR_SUCCESS;
}
