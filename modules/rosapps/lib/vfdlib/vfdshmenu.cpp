/*
	vfdshmenu.cpp

	Virtual Floppy Drive for Windows
	Driver control library
	COM shell extension class context menu functions

	Copyright (c) 2003-2005 Ken Kato
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "vfdtypes.h"
#include "vfdapi.h"
#include "vfdlib.h"
#ifndef __REACTOS__
#include "vfdmsg.h"
#else
#include "vfdmsg_lib.h"
#endif

//	class header
#include "vfdshext.h"

//
//	Undocumented windows API to handle shell property sheets
//

typedef BOOL (WINAPI *SHOBJECTPROPERTIES)(
	HWND hwnd, DWORD dwType, LPCWSTR lpObject, LPCWSTR lpPage);

#ifndef SHOP_FILEPATH
#define SHOP_FILEPATH				0x00000002
#endif

#define SHOP_EXPORT_ORDINAL			178

//
//	Context Menu Items
//
enum {
	VFD_CMD_OPEN = 0,
	VFD_CMD_SAVE,
	VFD_CMD_CLOSE,
	VFD_CMD_PROTECT,
	VFD_CMD_DROP,
	VFD_CMD_PROP,
	VFD_CMD_MAX
};

static struct _vfd_menu {
	UINT	textid;			//	menu item text id
	UINT	helpid;			//	menu item help id
#ifndef __REACTOS__
	PCHAR	verbA;			//	ansi verb text
	PWCHAR	verbW;			//	unicode verb text
#else
	LPCSTR	verbA;			//	ansi verb text
	LPCWSTR	verbW;			//	unicode verb text
#endif
}
g_VfdMenu[VFD_CMD_MAX] = {
	{ MSG_MENU_OPEN,	MSG_HELP_OPEN,		"vfdopen",	L"vfdopen"	},
	{ MSG_MENU_SAVE,	MSG_HELP_SAVE,		"vfdsave",	L"vfdsave"	},
	{ MSG_MENU_CLOSE,	MSG_HELP_CLOSE,		"vfdclose",	L"vfdclose"	},
	{ MSG_MENU_PROTECT,	MSG_HELP_PROTECT,	"protect",	L"protect"	},
	{ MSG_MENU_DROP,	MSG_HELP_DROP,		"vfddrop",	L"vfddrop"	},
	{ MSG_MENU_PROP,	MSG_HELP_PROP,		"vfdprop",	L"vfdprop"	},
};

//
//	local functions
//
static void AddMenuItem(
	HMENU			hMenu,
	UINT			uPos,
	UINT			uFlags,
	UINT			uCmd,
	UINT			uText)
{
	PSTR text = ModuleMessage(uText);

	if (text) {
		InsertMenu(hMenu, uPos, uFlags, uCmd, text);
		LocalFree(text);
	}
}


//
//	FUNCTION: CVfdShExt::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//	PURPOSE: Called by the shell just before the context menu is displayed.
//			 This is where you add your specific menu items.
//
//	PARAMETERS:
//	  hMenu		 - Handle to the context menu
//	  indexMenu	 - Index of where to begin inserting menu items
//	  idCmdFirst - Lowest value for new menu ID's
//	  idCmtLast	 - Highest value for new menu ID's
//	  uFlags	 - Specifies the context of the menu event
//
STDMETHODIMP CVfdShExt::QueryContextMenu(
	HMENU			hMenu,
	UINT			indexMenu,
	UINT			idCmdFirst,
	UINT			idCmdLast,
	UINT			uFlags)
{
	UNREFERENCED_PARAMETER(idCmdLast);
	VFDTRACE(0, ("CVfdShExt::QueryContextMenu()\n"));

	//
	//	Check if menu items should be added
	//
	if ((CMF_DEFAULTONLY & uFlags) ||
		!m_pDataObj || m_nDevice == (ULONG)-1) {

		VFDTRACE(0, ("Don't add any items.\n"));
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
	}

	//
	//	Drag & Drop handler?
	//
	if (m_bDragDrop) {

		VFDTRACE(0, ("Invoked as the Drop handler.\n"));

		if (GetFileAttributes(m_sTarget) & FILE_ATTRIBUTE_DIRECTORY) {

			// if the dropped item is a directory, nothing to do here
			VFDTRACE(0, ("Dropped object is a directory.\n"));

			return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
		}

		//	Add a drop context menu item
		AddMenuItem(
			hMenu,
			indexMenu,
			MF_BYPOSITION | MF_STRING,
			idCmdFirst + VFD_CMD_DROP,
			g_VfdMenu[VFD_CMD_DROP].textid);

		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, VFD_CMD_DROP + 1);
	}

	//
	//	Context menu handler
	//
	VFDTRACE(0, ("Invoked as the context menu handler.\n"));

	//
	//	Get the VFD media state
	//
	HANDLE hDevice = VfdOpenDevice(m_nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		VFDTRACE(0, ("device open failed.\n"));
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);
	}

	DWORD status = VfdGetMediaState(hDevice);

	CloseHandle(hDevice);

	//
	//	Add context menu items
	//

	InsertMenu(hMenu, indexMenu++,
		MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

	if (status == ERROR_SUCCESS ||
		status == ERROR_WRITE_PROTECT) {

		//	An image is opened

		//	insert the "save" menu item

		AddMenuItem(
			hMenu,
			indexMenu++,
			MF_BYPOSITION | MF_STRING,
			idCmdFirst + VFD_CMD_SAVE,
			g_VfdMenu[VFD_CMD_SAVE].textid);

		//	insert the "close" menu item

		AddMenuItem(
			hMenu,
			indexMenu++,
			MF_BYPOSITION | MF_STRING,
			idCmdFirst + VFD_CMD_CLOSE,
			g_VfdMenu[VFD_CMD_CLOSE].textid);

		//	insert the "protect" menu item

		AddMenuItem(
			hMenu,
			indexMenu++,
			MF_BYPOSITION | MF_STRING,
			idCmdFirst + VFD_CMD_PROTECT,
			g_VfdMenu[VFD_CMD_PROTECT].textid);

		//	check "protect" menu item

		if (status == ERROR_WRITE_PROTECT) {
			CheckMenuItem(hMenu, indexMenu - 1,
				MF_BYPOSITION | MF_CHECKED);
		}
	}
	else {
		//	The drive is empty

		//	insert the "open" menu item

		AddMenuItem(
			hMenu,
			indexMenu++,
			MF_BYPOSITION | MF_STRING,
			idCmdFirst + VFD_CMD_OPEN,
			g_VfdMenu[VFD_CMD_OPEN].textid);
	}

	//	Insert the "proterty" menu item

	AddMenuItem(
		hMenu,
		indexMenu++,
		MF_BYPOSITION | MF_STRING,
		idCmdFirst + VFD_CMD_PROP,
		g_VfdMenu[VFD_CMD_PROP].textid);

	//	Insert a separator

	InsertMenu(hMenu, indexMenu,
		MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, VFD_CMD_PROP + 1);
}

//
//	FUNCTION: CVfdShExt::GetCommandString(LPCMINVOKECOMMANDINFO)
//
//	PURPOSE:	Retrieves information about a shortcut menu command,
//				including the Help string and the language-independent,
//				or canonical, name for the command.
//
//	PARAMETERS:
//		idCmd -		Menu command identifier offset.
//		uFlags -	Flags specifying the information to return.
//					This parameter can have one of the following values.
//					GCS_HELPTEXTA  Sets pszName to an ANSI string containing the Help text for the command.
//					GCS_HELPTEXTW  Sets pszName to a Unicode string containing the Help text for the command.
//					GCS_VALIDATEA  Returns S_OK if the menu item exists, or S_FALSE otherwise.
//					GCS_VALIDATEW Returns S_OK if the menu item exists, or S_FALSE otherwise.
//					GCS_VERBA Sets pszName to an ANSI string containing the language-independent command name for the menu item.
//					GCS_VERBW Sets pszName to a Unicode string containing the language-independent command name for the menu item.
//		pwReserved - Reserved. Applications must specify NULL when calling this method, and handlers must ignore this parameter when called.
//		pszName -	Address of the buffer to receive the null-terminated string being retrieved.
//		cchMax -	Size of the buffer to receive the null-terminated string.
//

STDMETHODIMP CVfdShExt::GetCommandString(
#ifndef __REACTOS__
	UINT			idCmd,
#else
	UINT_PTR		idCmd,
#endif
	UINT			uFlags,
	UINT			*reserved,
	LPSTR			pszName,
	UINT			cchMax)
{
	VFDTRACE(0,
		("CVfdShExt::GetCommandString(%u,...)\n", idCmd));

	UNREFERENCED_PARAMETER(reserved);

	if (idCmd >= sizeof(g_VfdMenu) / sizeof(g_VfdMenu[0])) {
		return S_FALSE;
	}

	switch (uFlags) {
	case GCS_HELPTEXTA:
		FormatMessageA(
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			g_hDllModule, g_VfdMenu[idCmd].helpid,
			0, pszName, cchMax, NULL);

		VFDTRACE(0, ("HELPTEXTA: %s\n", pszName));
		break;

	case GCS_HELPTEXTW:
		FormatMessageW(
			FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			g_hDllModule, g_VfdMenu[idCmd].helpid,
			0, (LPWSTR)pszName, cchMax, NULL);

		VFDTRACE(0, ("HELPTEXTW: %ws\n", pszName));
		break;

	case GCS_VERBA:
		lstrcpynA(pszName, g_VfdMenu[idCmd].verbA, cchMax);
		break;

	case GCS_VERBW:
		lstrcpynW((LPWSTR)pszName, g_VfdMenu[idCmd].verbW, cchMax);
		break;
	}

	return NOERROR;
}

//
//	FUNCTION: CVfdShExt::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//	PURPOSE: Called by the shell after the user has selected on of the
//			 menu items that was added in QueryContextMenu().
//
//	PARAMETERS:
//	  lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//

STDMETHODIMP CVfdShExt::InvokeCommand(
	LPCMINVOKECOMMANDINFO	lpcmi)
{
	VFDTRACE(0, ("CVfdShExt::InvokeCommand()\n"));

	BOOL	unicode = FALSE;
	UINT	id;
	DWORD	ret;
	CMINVOKECOMMANDINFOEX *excmi = (CMINVOKECOMMANDINFOEX *)lpcmi;

#ifdef __REACTOS__
	unicode = lpcmi->cbSize >= FIELD_OFFSET(CMINVOKECOMMANDINFOEX, ptInvoke) &&
	          (lpcmi->fMask & CMIC_MASK_UNICODE);
#else
	if (lpcmi->cbSize >= sizeof(CMINVOKECOMMANDINFOEX) &&
		(lpcmi->fMask & CMIC_MASK_UNICODE)) {

		unicode = TRUE;
	}
#endif


	if (!unicode && HIWORD(lpcmi->lpVerb)) {

		VFDTRACE(0, ("ANSI: %s\n", lpcmi->lpVerb));

		// ANSI verb
		for (id = 0; id < sizeof(g_VfdMenu) / sizeof(g_VfdMenu[0]); id++) {
			if (!lstrcmpi(lpcmi->lpVerb, g_VfdMenu[id].verbA)) {
				break;
			}
		}
	}
	else if (unicode && HIWORD(excmi->lpVerbW)) {

		VFDTRACE(0, ("UNICODE: %ws\n", excmi->lpVerbW));

		// UNICODE verb
		for (id = 0; id < sizeof(g_VfdMenu) / sizeof(g_VfdMenu[0]); id++) {
			if (!lstrcmpiW(excmi->lpVerbW, g_VfdMenu[id].verbW)) {
				break;
			}
		}
	}
	else {

		VFDTRACE(0, ("Command: %u\n", LOWORD(lpcmi->lpVerb)));

		// Command ID
		id = LOWORD(lpcmi->lpVerb);
	}

	VFDTRACE(0, ("MenuItem: %u\n", id));

	switch (id) {
	case VFD_CMD_OPEN:
		ret = DoVfdOpen(lpcmi->hwnd);

		if (ret == ERROR_SUCCESS) {
			VfdImageTip(lpcmi->hwnd, m_nDevice);
		}
		break;

	case VFD_CMD_SAVE:
		ret = DoVfdSave(lpcmi->hwnd);
		break;

	case VFD_CMD_CLOSE:
		ret = DoVfdClose(lpcmi->hwnd);
		break;

	case VFD_CMD_PROTECT:
		ret = DoVfdProtect(lpcmi->hwnd);

		if (ret == ERROR_SUCCESS) {
			VfdImageTip(lpcmi->hwnd, m_nDevice);
		}
		else if (ret == ERROR_WRITE_PROTECT) {
			VfdImageTip(lpcmi->hwnd, m_nDevice);
			ret = ERROR_SUCCESS;
		}
		break;

	case VFD_CMD_DROP:
		ret = DoVfdDrop(lpcmi->hwnd);

		if (ret == ERROR_SUCCESS) {
			VfdImageTip(lpcmi->hwnd, m_nDevice);
		}
		break;

	case VFD_CMD_PROP:
		{
			SHOBJECTPROPERTIES pSHObjectProperties;
			WCHAR path[4] = L" :\\";

			pSHObjectProperties = (SHOBJECTPROPERTIES)GetProcAddress(
				LoadLibrary("shell32"), "SHObjectProperties");

			if (!pSHObjectProperties) {
				pSHObjectProperties = (SHOBJECTPROPERTIES)GetProcAddress(
					LoadLibrary("shell32"), (LPCSTR)SHOP_EXPORT_ORDINAL);
			}

			if (pSHObjectProperties) {
				path[0] = m_sTarget[0];

				pSHObjectProperties(lpcmi->hwnd,
					SHOP_FILEPATH, path, L"VFD");
			}
		}
		ret = ERROR_SUCCESS;
		break;

	default:
		return E_INVALIDARG;
	}

	if (ret != ERROR_SUCCESS &&
		ret != ERROR_CANCELLED) {

		MessageBox(lpcmi->hwnd,
			SystemMessage(ret), VFD_MSGBOX_TITLE, MB_ICONSTOP);
	}

	return NOERROR;
}

//=====================================
//	perform VFD menu operation
//=====================================

DWORD CVfdShExt::DoVfdOpen(
	HWND			hParent)
{
	DWORD ret = VfdGuiOpen(hParent, m_nDevice);

	if (ret != ERROR_SUCCESS && ret != ERROR_CANCELLED) {
		MessageBox(hParent, SystemMessage(ret),
			VFD_MSGBOX_TITLE, MB_ICONSTOP);
	}

	return ret;
}

//
//	Save the VFD image
//
DWORD CVfdShExt::DoVfdSave(
	HWND			hParent)
{
	return VfdGuiSave(hParent, m_nDevice);
}

//
//	Close current VFD image
//
DWORD CVfdShExt::DoVfdClose(
	HWND			hParent)
{
	return VfdGuiClose(hParent, m_nDevice);
}

//
//	Enable/disable media write protection
//
DWORD CVfdShExt::DoVfdProtect(
	HWND			hParent)
{
	HANDLE			hDevice;
	DWORD			ret;

	UNREFERENCED_PARAMETER(hParent);
	VFDTRACE(0, ("CVfdShExt::DoVfdProtect()\n"));

	hDevice = VfdOpenDevice(m_nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	ret = VfdGetMediaState(hDevice);

	if (ret == ERROR_SUCCESS) {
		ret = VfdWriteProtect(hDevice, TRUE);
	}
	else if (ret == ERROR_WRITE_PROTECT) {
		ret = VfdWriteProtect(hDevice, FALSE);
	}

	if (ret == ERROR_SUCCESS) {
		ret = VfdGetMediaState(hDevice);
	}

	CloseHandle(hDevice);

	return ret;
}

//
//	Open dropped file with VFD
//
DWORD CVfdShExt::DoVfdDrop(
	HWND			hParent)
{
	HANDLE			hDevice;
	DWORD			file_attr;
	ULONG			file_size;
	VFD_FILETYPE	file_type;

	VFD_DISKTYPE	disk_type;
	VFD_MEDIA		media_type;

	DWORD			ret;

	VFDTRACE(0, ("CVfdShExt::DoVfdDropOpen()\n"));

	//	check if dropped file is a valid image

	ret = VfdCheckImageFile(
		m_sTarget, &file_attr, &file_type, &file_size);

	if (ret != ERROR_SUCCESS) {
		return ret;
	}

	//	check file size
	media_type = VfdLookupMedia(file_size);

	if (!media_type) {
		PSTR msg = ModuleMessage(MSG_FILE_TOO_SMALL);

		MessageBox(hParent, msg ? msg : "Bad size",
			VFD_MSGBOX_TITLE, MB_ICONSTOP);

		if (msg) {
			LocalFree(msg);
		}

		return ERROR_CANCELLED;
	}

	if ((file_type == VFD_FILETYPE_ZIP) ||
		(file_attr & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED))) {

		disk_type = VFD_DISKTYPE_RAM;
	}
	else {
		disk_type = VFD_DISKTYPE_FILE;
	}

	//	close current image (if opened)

	ret = DoVfdClose(hParent);

	if (ret != ERROR_SUCCESS &&
		ret != ERROR_NOT_READY) {
		return ret;
	}

	//	open dropped file

	hDevice = VfdOpenDevice(m_nDevice);

	if (hDevice == INVALID_HANDLE_VALUE) {
		return GetLastError();
	}

	ret = VfdOpenImage(
		hDevice, m_sTarget, disk_type, media_type, FALSE);

	CloseHandle(hDevice);

	return ret;
}
