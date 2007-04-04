#ifndef ODBCCP32_H__ // odbccp32.h
#define ODBCCP32_H__

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>
#include <tchar.h>

#include "resource.h"

extern HINSTANCE hApplet;

INT_PTR 
CALLBACK
UserDSNProc(IN HWND hwndDlg,
	    	IN UINT uMsg,
			IN WPARAM wParam,
			IN LPARAM lParam);

INT_PTR 
CALLBACK
SystemDSNProc(IN HWND hwndDlg,
			  IN UINT uMsg,
			  IN WPARAM wParam,
			  IN LPARAM lParam);

INT_PTR 
CALLBACK
FileDSNProc(IN HWND hwndDlg,
			IN UINT uMsg,
			IN WPARAM wParam,
			IN LPARAM lParam);


INT_PTR 
CALLBACK
DriversProc(IN HWND hwndDlg,
			IN UINT uMsg,
			IN WPARAM wParam,
			IN LPARAM lParam);

INT_PTR 
CALLBACK
TraceProc(IN HWND hwndDlg,
		  IN UINT uMsg,
		  IN WPARAM wParam,
		  IN LPARAM lParam);


INT_PTR 
CALLBACK
PoolProc(IN HWND hwndDlg,
		 IN UINT uMsg,
		 IN WPARAM wParam,
		 IN LPARAM lParam);

INT_PTR 
CALLBACK
AboutProc(IN HWND hwndDlg,
		  IN UINT uMsg,
		  IN WPARAM wParam,
		  IN LPARAM lParam);



#endif /* end of ODBCCP32_H__ */
