/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/testset/user32/kbdlayout/kbdlayout.c
 * PURPOSE:         Keyboard layout testapp
 * COPYRIGHT:       Copyright 2007 Saveliy Tretiakov
 */

#define UNICODE
#include<wchar.h>
#include <windows.h>
#include "resource.h"



LRESULT MainDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);


HINSTANCE hInst;
HWND hMainDlg;


typedef struct {
	WNDPROC OrigProc;
	WCHAR WndName[25];
} WND_DATA;

DWORD WINAPI ThreadProc(LPVOID lpParam)
{

	DialogBoxParam(hInst,
		MAKEINTRESOURCE(IDD_MAINDIALOG),
		NULL,
		(DLGPROC)MainDialogProc,
		(LPARAM)NULL);

	return 0;
}

INT WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{


	hInst = hInstance;

	ThreadProc(0);

	return 0;
}


int GetKlList(HKL **list)
{
	HKL *ret;
	int n;

	n = GetKeyboardLayoutList(0, NULL);
	ret = HeapAlloc(GetProcessHeap(), 0, sizeof(HKL)*n);
	GetKeyboardLayoutList(n, ret);
	*list = ret;
	return n;
}

void FreeKlList(HKL *list)
{
	HeapFree(GetProcessHeap(), 0, list);
}


void UpdateData(HWND hDlg)
{
	WCHAR buf[KL_NAMELENGTH];
	WCHAR buf2[512];

	HWND hList;
	HKL *klList, hKl;
	int n, i,j;

	GetKeyboardLayoutName(buf);
	swprintf(buf2, L"Active: %s (%x)", buf, GetKeyboardLayout(0));
	SetWindowText(GetDlgItem(hDlg, IDC_ACTIVE), buf2);

	hList = GetDlgItem(hDlg, IDC_LIST);
	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	n = GetKlList(&klList);
	hKl = GetKeyboardLayout(0);
	for(i = 0; i < n; i++)
	{
		swprintf(buf, L"%x", klList[i] );
		j = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM) buf);
		SendMessage(hList, LB_SETITEMDATA, j, (LPARAM) klList[i]);
		if(klList[i] == hKl) SendMessage(hList, LB_SETCURSEL, j, 0);
	}

	FreeKlList(klList);
}

void FormatMsg(WCHAR *format, ...)
{
	WCHAR buf[255];
	va_list argptr;
	va_start(argptr, format);
	_vsnwprintf(buf, sizeof(buf)-1, format, argptr);
	MessageBox(0, buf, L"msg", 0);
	va_end(argptr);
}

void FormatBox(HWND hWnd, DWORD Flags, WCHAR *Caption, WCHAR *Format, ...)
{
	WCHAR buf[255];
	va_list argptr;
	va_start(argptr, Format);
	_vsnwprintf(buf, sizeof(buf)-1, Format, argptr);
	MessageBox(hWnd, buf, Caption, Flags);
	va_end(argptr);
}


LRESULT CALLBACK WndSubclassProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WND_DATA *data = (WND_DATA*)GetWindowLong(hwnd, GWL_USERDATA);

	if(uMsg == WM_INPUTLANGCHANGE)
	{
		FormatMsg(L"%s: WM_INPUTLANGCHANGE lParam=%x wParam=%x\n", data->WndName, lParam, wParam);
		UpdateData(hMainDlg);
		//Pass message to defwindowproc
	}
	else if(uMsg == WM_INPUTLANGCHANGEREQUEST)
	{
		FormatMsg(L"%s: WM_INPUTLANGCHANGEREQUEST lParam=%x wParam=%x\n", data->WndName, lParam, wParam);
		UpdateData(hMainDlg);
		//Pass message to defwindowproc
	}

	return ( CallWindowProc( data->OrigProc, hwnd, uMsg, wParam, lParam) );
}

void SubclassWnd(HWND hWnd, WCHAR* Name)
{
	WND_DATA *data = HeapAlloc(GetProcessHeap(), 0, sizeof(WND_DATA));
	data->OrigProc = (WNDPROC)SetWindowLong( hWnd, GWL_WNDPROC, (LONG)WndSubclassProc);
	wcsncpy(data->WndName, Name, 25);
	SetWindowLong(hWnd, GWL_USERDATA, (LONG)data);
	return;
}

DWORD GetActivateFlags(HWND hDlg)
{
	DWORD ret = 0;

	if(IsDlgButtonChecked(hDlg, IDC_KLF_REORDER))
		ret |= KLF_REORDER;

	if(IsDlgButtonChecked(hDlg, IDC_KLF_RESET))
		ret |= KLF_RESET;

	if(IsDlgButtonChecked(hDlg, IDC_KLF_SHIFTLOCK))
		ret |= KLF_SHIFTLOCK;

	if(IsDlgButtonChecked(hDlg, IDC_KLF_SETFORPROCESS))
		ret |= KLF_SETFORPROCESS;

	return ret;

}

DWORD GetLoadFlags(HWND hDlg)
{
	DWORD ret = 0;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_ACTIVATE))
		ret |= KLF_ACTIVATE;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_NOTELLSHELL))
		ret |= KLF_NOTELLSHELL;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_REORDER))
		ret |= KLF_REORDER;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_REPLACELANG))
		ret |= KLF_REPLACELANG;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_SUBSTITUTE_OK))
		ret |= KLF_SUBSTITUTE_OK;

	if(IsDlgButtonChecked(hDlg, IDL_KLF_SETFORPROCESS))
		ret |= KLF_SETFORPROCESS;

	return ret;
}

UINT GetDelayMilliseconds(HWND hDlg)
{
	WCHAR Buf[255];
	UINT ret;

	GetWindowText(GetDlgItem(hDlg, IDC_DELAY), Buf, sizeof(Buf));

	swscanf(Buf, L"%d", &ret);

	return ret*1000;
}

HKL GetSelectedLayout(HWND hDlg)
{
	int n;
	HWND hList;
	hList = GetDlgItem(hDlg, IDC_LIST);
	if((n = SendMessage(hList, LB_GETCURSEL, 0, 0)) != LB_ERR)
		return (HKL) SendMessage(hList, LB_GETITEMDATA, n, 0);
	else return INVALID_HANDLE_VALUE;
}

HKL GetActivateHandle(HWND hDlg)
{

	if(IsDlgButtonChecked(hDlg, IDC_FROMLIST))
		return GetSelectedLayout(hDlg);
	else if(IsDlgButtonChecked(hDlg, IDC_HKL_NEXT))
		return (HKL)HKL_NEXT;

	return (HKL)HKL_PREV;

}


/***************************************************
 * MainDialogProc                                  *
 ***************************************************/

LRESULT MainDialogProc(HWND hDlg,
	UINT Msg,
	WPARAM wParam,
	LPARAM lParam)
{
	HKL hKl;

	switch (Msg)
	{
		case WM_INITDIALOG:
		{
			WCHAR Buf[255];
			UpdateData(hDlg);
			hMainDlg = hDlg;

			SubclassWnd(GetDlgItem(hDlg, IDC_LIST), L"List");
			SubclassWnd(GetDlgItem(hDlg, IDC_EDIT1), L"Edit1");
			SubclassWnd(GetDlgItem(hDlg, IDC_KLID), L"Klid");
			SubclassWnd(GetDlgItem(hDlg, ID_CANCEL), L"CancelB");
			SubclassWnd(GetDlgItem(hDlg, IDC_ACTIVATE), L"ActivateB");
			SubclassWnd(GetDlgItem(hDlg, IDC_REFRESH), L"RefreshB");
			SubclassWnd(GetDlgItem(hDlg, IDC_UNLOAD), L"UnloadB");
			SubclassWnd(GetDlgItem(hDlg, IDC_LOAD), L"LoadB");

			CheckRadioButton(hDlg, IDC_FROMLIST, IDC_FROMEDIT, IDC_FROMLIST);
			SetWindowText(GetDlgItem(hDlg, IDC_KLID), L"00000419");

			swprintf(Buf, L"Current thread id: %d", GetCurrentThreadId());
			SetWindowText(GetDlgItem(hDlg, IDC_CURTHREAD), Buf);

			SetWindowText(GetDlgItem(hDlg, IDC_DELAY), L"0");

			return 0;
		} /* WM_INITDIALOG */

		case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
				case ID_CANCEL:
				{
					EndDialog(hDlg, ERROR_CANCELLED);
					break;
				}

				case IDC_ACTIVATE:
				{
					if((hKl = GetActivateHandle(hDlg)) != INVALID_HANDLE_VALUE)
					{
						Sleep(GetDelayMilliseconds(hDlg));
						if(!(hKl = ActivateKeyboardLayout(hKl, GetActivateFlags(hDlg))))
							FormatBox(hDlg, MB_ICONERROR, L"Error",
								L"ActivateKeyboardLayout() failed. %d", GetLastError());
						else UpdateData(hDlg);
						//FormatBox(hDlg, 0, L"Activated", L"Prev - %x, err - %d.", hKl,
						// GetLastError());
					}
					else MessageBox(hDlg, L"No item selected", L"Error", MB_ICONERROR);
					break;
				}

				case IDC_UNLOAD:
				{
					if((hKl = GetSelectedLayout(hDlg)) != INVALID_HANDLE_VALUE)
					{
						Sleep(GetDelayMilliseconds(hDlg));
						if(!UnloadKeyboardLayout(hKl))
							FormatBox(hDlg, MB_ICONERROR, L"Error",
								L"UnloadKeyboardLayout() failed. %d",
								GetLastError());
						else UpdateData(hDlg);
					}
					else MessageBox(hDlg,  L"No item selected", L"Error", MB_ICONERROR);
					break;
				}

				case IDC_LOAD:
				{
					WCHAR buf[255];
					GetWindowText(GetDlgItem(hDlg, IDC_KLID), buf, sizeof(buf));
					Sleep(GetDelayMilliseconds(hDlg));
					if(!LoadKeyboardLayout(buf, GetLoadFlags(hDlg)))
						FormatBox(hDlg, MB_ICONERROR, L"Error",
							L"LoadKeyboardLayout() failed. %d",
							GetLastError());
					else UpdateData(hDlg);
					break;
				}

				case IDC_REFRESH:
				{
					UpdateData(hDlg);
					break;
				}

				case IDC_NEWTHREAD:
				{
					if(!CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL))
					{
						FormatBox(hDlg, MB_ICONERROR, L"Error!",
							L"Can not create thread (%d).", GetLastError());
					}
				}

				case IDC_LIST:
				{
					if(HIWORD(wParam) == LBN_SELCHANGE)
					{
						WCHAR buf[25];
						if((hKl = GetSelectedLayout(hDlg)) != NULL)
						{
							swprintf(buf, L"%x", hKl);
							SetWindowText(GetDlgItem(hDlg, IDC_HANDLE), buf);
						}
					}
					break;
				}

			}

			return TRUE;
		} /* WM_COMMAND */


		case WM_INPUTLANGCHANGE:
		{
			FormatMsg(L"dlg WM_INPUTLANGCHANGE lParam=%x wParam=%x\n", lParam, wParam);
			return FALSE;
		}

		case WM_INPUTLANGCHANGEREQUEST:
		{
			FormatMsg(L"dlg WM_INPUTLANGCHANGEREQUEST lParam=%x wParam=%x\n", lParam, wParam);
			UpdateData(hDlg);
			return FALSE;
		}

		case WM_CLOSE:
		{
			EndDialog(hDlg, ERROR_CANCELLED);
			return TRUE;
		} /* WM_CLOSE */

		default:
			return FALSE;
	}

}




