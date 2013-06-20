#include <stdio.h>
#include <windows.h>
#include <setupapi.h>
#include "http.h"

#define ST_NEUTRAL      0
#define ST_ROS_CRASH    1
#define ST_DEV_NOTWORK  2
#define ST_DEV_OK       3
#define ST_ERROR        4
#define ST_LAST_STATUS  ST_ERROR

static int      is_show_hw;
static int      is_check_hw;
static wchar_t  gl_ini_file[MAX_PATH];
static wchar_t *gl_report;

static int hw_check_ini(wchar_t *name)
{
	wchar_t buff[MAX_PATH];

	if (GetPrivateProfileString(L"HW", name, NULL, buff, MAX_PATH, gl_ini_file) == 0) {
		return ST_NEUTRAL;
	}
	if (_wcsicmp(buff, L"ok") == 0) return ST_DEV_OK;
	if (_wcsicmp(buff, L"notwork") == 0) return ST_DEV_NOTWORK;
	if (_wcsicmp(buff, L"crash") == 0) return ST_ROS_CRASH;
	return ST_NEUTRAL;
}

static int hw_check_base(wchar_t *hw_id, wchar_t *hw_name)
{
	wchar_t buff[MAX_PATH], *p;
	int     status;

	if ( (status = hw_check_ini(hw_id)) != ST_NEUTRAL ) {
		return status;
	}
	if ( (status = hw_check_ini(hw_name)) != ST_NEUTRAL ) {
		return status;
	}

	if (wcsncmp(hw_id, L"PCI\\VEN_", 8) == 0)
	{
		wcsncpy(buff, hw_id, 21); buff[21] = 0;

		if ( (status = hw_check_ini(buff)) != ST_NEUTRAL ) {
			return status;
		}

		if (p = wcsstr(hw_id, L"&REV_")) {
			wcscat(buff, p); status = hw_check_ini(buff);
		}
	} else if ( (wcsncmp(hw_id, L"USB\\", 4) == 0) && (p = wcsstr(hw_id, L"&VID")) )
	{
		wsprintf(buff, L"USB\\%s", p+1);
		
		if ( (status = hw_check_ini(buff)) != ST_NEUTRAL ) {
			return status;
		}

		if (p = wcsstr(buff, L"&REV")) {
			*p = 0; status = hw_check_ini(buff);
		}
	}
	return status;
}

static void trim(wchar_t *s) 
{
	wchar_t *p;
	for (p = s + wcslen(s) - 1; (p > s) && (*p == L' '); *p-- = 0);
}

static int hw_check_device(HDEVINFO h_info, SP_DEVINFO_DATA *d_inf)
{
	wchar_t *hw_id   = NULL;
	wchar_t *hw_name = NULL;
	u_long   type, bytes;
	int      status;
	char     name[MAX_PATH];
	wchar_t  w_name[MAX_PATH];
	
	do
	{
		if ( (hw_id = malloc(4096)) == NULL ) {
			status = ST_ERROR; break;
		}
		if ( (hw_name = malloc(4096)) == NULL ) {
			status = ST_ERROR; break;
		}
		hw_id[0] = 0, hw_name[0] = 0;

		SetupDiGetDeviceRegistryProperty(h_info, d_inf, SPDRP_HARDWAREID, &type, (void*)hw_id, 4096, &bytes);
		SetupDiGetDeviceRegistryProperty(h_info, d_inf, SPDRP_DEVICEDESC, &type, (void*)hw_name, 4096, &bytes);

		if (hw_id[0] == 0 || hw_name[0] == 0) {
			status = ST_NEUTRAL; break;
		}	
		/* trim strings */
		trim(hw_id); trim(hw_name);

		if ( (wcschr(hw_id, L'\\') == NULL) || (_wcsnicmp(hw_id, L"ROOT\\", 5) == 0) ||
			 (_wcsicmp(hw_id, L"STORAGE\\Volume") == 0) || (_wcsicmp(hw_id, L"UMB\\UMBUS") == 0) ||
			 (_wcsnicmp(hw_id, L"SW\\", 3) == 0) )
		{
			status = ST_NEUTRAL; break;
		}

		if (is_show_hw != 0) {
			CharToOem(hw_name, name);
			wprintf(L"%s - [%S]\n", hw_id, name);
		}

		if (gl_report != NULL) {
			wsprintf(w_name, L"%s - [%s]\n", hw_id, hw_name);
			wcscat(gl_report, w_name);
		}

		if (is_check_hw != 0) 
		{
			status = hw_check_base(hw_id, hw_name);

			if (status == ST_DEV_NOTWORK) {
				CharToOem(hw_name, name);
				wprintf(L"Device \"%S\" does not work in ReactOS\n", name);
			}
			if (status == ST_ROS_CRASH) {
				CharToOem(hw_name, name);
				wprintf(L"ReactOS does not work with device \"%S\"\n", name);
			}			
		} else {
			status = ST_NEUTRAL;
		}
	} while (0);

	if (hw_id != NULL) free(hw_id);
	if (hw_name != NULL) free(hw_name);

	return status;
}

static void do_update_base()
{
	wchar_t up_url[MAX_PATH];
	void   *data;
	u_long  size;
	FILE   *f;

	if (GetPrivateProfileString(L"URL", L"udpate", NULL, up_url, MAX_PATH, gl_ini_file) == 0) {
		wprintf(L"Update URL not found in rosddt.ini\n"); return;
	}

	wprintf(L"Downloading new rosddt.ini...\n");

	if (data = http_get(up_url, &size)) 
	{
		if (f = _wfopen(gl_ini_file, L"wb")) {
			fwrite(data, 1, size, f);
			fclose(f);
			wprintf(L"Update completed\n");
		} else {
			wprintf(L"Can not open rosddt.ini for writing\n"); 
		}
		free(data);
	} else {
		wprintf(L"Error, rosddt.ini can not be loaded\n");
	}
}

static void do_send_report(wchar_t *report)
{
	wchar_t up_url[MAX_PATH];
	int     utf_sz;
	char   *utf, *p;

	if (GetPrivateProfileString(L"URL", L"report", NULL, up_url, MAX_PATH, gl_ini_file) == 0) {
		wprintf(L"Report URL not found in rosddt.ini\n"); return;
	}

	utf_sz = WideCharToMultiByte(CP_UTF8, 0, report, -1, NULL, 0, NULL, NULL);
	utf    = malloc(utf_sz);
	utf_sz = WideCharToMultiByte(CP_UTF8, 0, report, -1, utf, utf_sz, NULL, NULL);

	wprintf(L"Sending report...\n");

	if (p = http_post(up_url, utf, utf_sz-1, NULL)) {
		wprintf(L"%S\n", p); free(p);
	} else {
		wprintf(L"Report can not be sended, connection error\n");
	}
}

int wmain(int argc, wchar_t *argv[])
{
	HDEVINFO        h_info;
	SP_DEVINFO_DATA d_inf;
	int             codes[ST_LAST_STATUS + 1] = {0};
	int             i;
	wchar_t        *p;
		
	if (argc != 2)
	{
		wprintf(
			L"rosddt [parameters]\n"
			L" -enum   enumerate all installed hardware\n"
			L" -check  check hardware compatibility with ReactOS\n"
			L" -update update hardware compatibility database\n"
			L" -report send your hardware configuration to ReactOS team\n"
			);
		return 0;
	}

	/* get path to ini file */
	GetModuleFileName(NULL, gl_ini_file, MAX_PATH);	
	for (p = gl_ini_file + wcslen(gl_ini_file); *p != L'\\'; *p-- = 0);
	wcscat(gl_ini_file, L"rosddt.ini");

	if (_wcsicmp(argv[1], L"-update") == 0) {
		do_update_base(); return 0;
	}
	if (_wcsicmp(argv[1], L"-enum") == 0) {
		is_show_hw = 1; is_check_hw = 0;
	}
	if (_wcsicmp(argv[1], L"-check") == 0) {
		is_show_hw = 0; is_check_hw = 1;
	}
	if (_wcsicmp(argv[1], L"-report") == 0) {
		is_show_hw = 0; is_check_hw = 0;
		gl_report = malloc(65536); gl_report[0] = 0;
	}

	h_info = SetupDiGetClassDevs(NULL, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);

	if (h_info == INVALID_HANDLE_VALUE) {
		wprintf(L"SetupDiGetClassDevs error\n"); return 1;
	}

	d_inf.cbSize = sizeof(d_inf); i = 0;

	while (SetupDiEnumDeviceInfo(h_info, i++, &d_inf) != 0) {
		codes[hw_check_device(h_info, &d_inf)]++;		
	}

	if (is_check_hw != 0) 
	{
		wprintf(
			L"Checking completed.\nFound %d supported devices, %d unsupported devices and %d incompatible devices\n", 
			codes[ST_DEV_OK], codes[ST_DEV_NOTWORK], codes[ST_ROS_CRASH]);

		if (codes[ST_ROS_CRASH] == 0) {
			wprintf(L"ReactOS can be installed on your computer\n");
		} else {
			wprintf(L"ReactOS can not be installed on your computer\n");
		}
	}

	if (gl_report != NULL) {
		do_send_report(gl_report);
	}

	return 0; 
}
