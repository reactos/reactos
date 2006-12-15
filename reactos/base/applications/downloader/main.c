/*
 * PROJECT:         ReactOS Downloader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/downloader/xml.c
 * PURPOSE:         Main program
 * PROGRAMMERS:     Maarten Bosma
 */

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>
#include "resources.h"
#include "structures.h"

HWND hCategories, hApps, hDescription, hBtnDownload, hBtnUpdate, hBtnHelp;
HBITMAP hLogo;

struct Category Root;
struct Application* SelectedApplication;

INT_PTR CALLBACK DownloadProc (HWND, UINT, WPARAM, LPARAM);
BOOL ProcessXML (const char* filename, struct Category* Root);
void FreeTree (struct Category* Node);


void ShowMessage (WCHAR* title, WCHAR* message)
{
	SETTEXTEX Text = {ST_SELECTION, 1200};

	SendMessage(hDescription, WM_SETTEXT, 0, 0);
	SendMessage(hDescription, EM_SETTEXTEX, (WPARAM)&Text, (LPARAM)title);
	SendMessage(hDescription, EM_SETTEXTEX, (WPARAM)&Text, (LPARAM)L"\n----------------------------------------\n");
	SendMessage(hDescription, EM_SETTEXTEX, (WPARAM)&Text, (LPARAM)message);
}

void AddItems (HWND hwnd, struct Category* Category, struct Category* Parent)
{ 
	TV_INSERTSTRUCT Insert; 

	Insert.item.lParam = (UINT)Category;
	Insert.item.mask = TVIF_TEXT|TVIF_PARAM;
	Insert.item.pszText = Category->Name; //that is okay
	Insert.item.cchTextMax = lstrlen(Category->Name); 
	Insert.hInsertAfter = TVI_LAST;
	Insert.hParent = Category->Parent ? Category->Parent->TreeviewItem : TVI_ROOT;

	if(Category->Icon)
	{
		Insert.item.iImage = Category->Icon;
		Insert.item.iSelectedImage = Category->Icon;
		Insert.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	}

	Category->TreeviewItem = (HTREEITEM)SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);

	if(Category->Next)
		AddItems (hwnd,Category->Next,Parent);

	if(Category->Children)
		AddItems (hwnd,Category->Children,Category);
}

void DisplayApps (HWND hwnd, struct Category* Category)
{
	struct Application* CurrentApplication = Category->Apps;

	TreeView_DeleteItem(hwnd, TVI_ROOT);

	TV_INSERTSTRUCT Insert;
	Insert.item.mask = TVIF_TEXT|TVIF_PARAM;
	Insert.hInsertAfter = TVI_LAST;
	Insert.hParent = TVI_ROOT;

	while(CurrentApplication)
	{
		Insert.item.lParam = (UINT)CurrentApplication;
		Insert.item.pszText = CurrentApplication->Name;
		Insert.item.cchTextMax = lstrlen(CurrentApplication->Name); 
		SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);
		CurrentApplication = CurrentApplication->Next;
	}
} 

void SetupControls (HWND hwnd)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Set up the controls
	hCategories = CreateWindowEx(0, WC_TREEVIEW, L"Categories", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS, 
							0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

	hApps = CreateWindowEx(0, WC_TREEVIEW, L"Applications", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS, 
							0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

	hDescription = CreateWindowEx(WS_EX_WINDOWEDGE, RICHEDIT_CLASS, L"", WS_CHILD|WS_VISIBLE|ES_MULTILINE|ES_READONLY, //|ES_AUTOHSCROLL
							0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

	hLogo = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO));

	hBtnHelp = CreateWindow (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 550, 10, 40, 40, hwnd, 0, hInstance, NULL);
	hBtnUpdate = CreateWindow (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 500, 10, 40, 40, hwnd, 0, hInstance, NULL);
	hBtnDownload = CreateWindow (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 330, 505, 140, 33, hwnd, 0, hInstance, NULL);

	SendMessage (hBtnHelp, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_HELP)));
	SendMessage (hBtnUpdate, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UPDATE)));
	SendMessage (hBtnDownload, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DOWNLOAD)));

	// Create Tree Icons
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLORDDB, 1, 1);
	SendMessage(hCategories, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);
	SendMessage(hApps, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);

	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_0)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_1)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_2)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_3)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_4)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_5)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_6)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_7)), NULL); 
	ImageList_Add(hImageList, LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_TREEVIEW_ICON_8)), NULL); 

	// Fill the TreeViews
	ProcessXML ("apps.xml", &Root);
	AddItems (hCategories, Root.Children, NULL);
}

static void ResizeControl (HWND hwnd, int x1, int y1, int x2, int y2)
{
	// Make resizing a little easier
	MoveWindow(hwnd, x1, y1, x2-x1, y2-y1, TRUE);
}

static void DrawBitmap (HWND hwnd, int x, int y, HBITMAP hBmp)
{
	BITMAP bm;
	PAINTSTRUCT ps;

	HDC hdc = BeginPaint(hwnd, &ps);
	HDC hdcMem = CreateCompatibleDC(hdc);

	SelectObject(hdcMem, hBmp);
	GetObject(hBmp, sizeof(bm), &bm);
	
	//BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
	TransparentBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, 0xFFFFFF);

	DeleteDC(hdcMem);
	EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
		case WM_CREATE:
		{
			SetupControls(hwnd);
			ShowMessage(L"ReactOS Downloader", L"Welcome to ReactOS's Downloader\nPlease choose a category on the right. This is version 0.5.");
		} 
		break;

		case WM_PAINT:
		{
			DrawBitmap(hwnd, 10, 12, hLogo);
		}
		break;

		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED)
			{
				if (lParam == (LPARAM)hBtnDownload)
				{
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc);
				}
				else if (lParam == (LPARAM)hBtnUpdate)
				{
					ShowMessage (L"Update", L"Feature not implemented yet.");
				}
				else if (lParam == (LPARAM)hBtnHelp)
				{
					ShowMessage (L"Help", L"Choose a category on the right, then choose a application and click the download button.To update the application information click the button next to the help button.");
				}
			}
		}
		break;

		case WM_NOTIFY:
		{
			LPNMHDR data = (LPNMHDR)lParam;
			if(data->code == TVN_SELCHANGED)
			{
				if(data->hwndFrom == hCategories) 
				{
					DisplayApps (hApps, (struct Category*) ((LPNMTREEVIEW)lParam)->itemNew.lParam);
				}
				else if(data->hwndFrom == hApps) 
				{
					SelectedApplication = (struct Application*) ((LPNMTREEVIEW)lParam)->itemNew.lParam;
					ShowMessage(SelectedApplication->Name, SelectedApplication->Description);
				}
			}
		}
		break;

		case WM_SIZE:
		{
			int Split_Hozizontal = (HIWORD(lParam)-(45+60))/2 + 60;
			int Split_Vertical = 200;

			ResizeControl(hCategories, 10, 60, Split_Vertical, HIWORD(lParam)-10);
			ResizeControl(hApps, Split_Vertical+5, 60, LOWORD(lParam)-10, Split_Hozizontal);
			ResizeControl(hDescription, Split_Vertical+5, Split_Hozizontal+5, LOWORD(lParam)-10, HIWORD(lParam)-50);

			MoveWindow(hBtnHelp, LOWORD(lParam)-50, 10, 40, 40, 0);
			MoveWindow(hBtnUpdate, LOWORD(lParam)-100, 10, 40, 40, 0);
			MoveWindow(hBtnDownload, (Split_Vertical+LOWORD(lParam))/2-70, HIWORD(lParam)-45, 140, 35, 0);
		}
		break;

		case WM_DESTROY:
		{
			DeleteObject(hLogo);
			FreeTree(Root.Children);
			PostQuitMessage(0);
			return 0;
		}
		break;
	}

	return DefWindowProc (hwnd, Message, wParam, lParam);
}

INT WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInst,
					LPSTR lpCmdLine, INT nCmdShow)
{
	HWND hwnd;

	LoadLibrary(L"riched20.dll");
	InitCommonControls();

	// Create the window
	WNDCLASSEX WndClass = {0};
	WndClass.cbSize			= sizeof(WNDCLASSEX); 
	WndClass.lpszClassName	= L"Downloader";
	WndClass.style			= CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc	= WndProc;
	WndClass.hInstance		= hInstance;
	WndClass.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	WndClass.hCursor		= LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground	= (HBRUSH)CreateSolidBrush(RGB(235,233,237));

	RegisterClassEx(&WndClass);

	hwnd = CreateWindow(L"Downloader", 
						L"Download ! - ReactOS Downloader",
						WS_OVERLAPPEDWINDOW,
						CW_USEDEFAULT,  
						CW_USEDEFAULT,   
						600, 550, 
						NULL, NULL,
						hInstance, 
						NULL);

	// Show it
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// Message Loop
	MSG msg;
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
