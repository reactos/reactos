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

HWND hCategories, hApps, hDownloadButton, hUpdateButton, hHelpButton;
HBITMAP hLogo, hUnderline;
WCHAR* DescriptionHeadline = L"";
WCHAR* DescriptionText = L"";

struct Category Root;
struct Application* SelectedApplication;

INT_PTR CALLBACK DownloadProc (HWND, UINT, WPARAM, LPARAM);
BOOL ProcessXML (const char* filename, struct Category* Root);
VOID FreeTree (struct Category* Node);
WCHAR Strings [STRING_COUNT][MAX_STRING_LENGHT];

void ShowMessage (WCHAR* title, WCHAR* message)
{
	DescriptionHeadline = title;
	DescriptionText = message;

	HWND hwnd = GetParent(hCategories);
	InvalidateRect(hwnd,NULL,TRUE); 
	UpdateWindow(hwnd);
}

void AddItems (HWND hwnd, struct Category* Category, struct Category* Parent)
{ 
	TV_INSERTSTRUCT Insert; 

	Insert.item.lParam = (UINT)Category;
	Insert.item.mask = TVIF_TEXT|TVIF_PARAM|TVIF_IMAGE|TVIF_SELECTEDIMAGE;;
	Insert.item.pszText = Category->Name; //that is okay
	Insert.item.cchTextMax = lstrlen(Category->Name); 
	Insert.item.iImage = Category->Icon;
	Insert.item.iSelectedImage = Category->Icon;
	Insert.hInsertAfter = TVI_LAST;
	Insert.hParent = Category->Parent ? Category->Parent->TreeviewItem : TVI_ROOT;

	Category->TreeviewItem = (HTREEITEM)SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);

	if(Category->Next)
		AddItems (hwnd,Category->Next,Parent);

	if(Category->Children)
		AddItems (hwnd,Category->Children,Category);
}

void CategoryChoosen (HWND hwnd, struct Category* Category)
{
	struct Application* CurrentApplication;
	SelectedApplication = NULL;
	
	if(Category->Children && !Category->Apps)
		ShowMessage(Category->Name, Strings[IDS_CHOOSE_SUB]);
	else if(!Category->Children && Category->Apps)
		ShowMessage(Category->Name, Strings[IDS_CHOOSE_APP]);
	else if(Category->Children && Category->Apps)
		ShowMessage(Category->Name, Strings[IDS_CHOOSE_BOTH]);
	else
		ShowMessage(Category->Name, Strings[IDS_NO_APPS]);

	TreeView_DeleteItem(hwnd, TVI_ROOT);

	TV_INSERTSTRUCT Insert;
	Insert.item.mask = TVIF_TEXT|TVIF_PARAM;
	Insert.hInsertAfter = TVI_LAST;
	Insert.hParent = TVI_ROOT;

	CurrentApplication = Category->Apps;

	while(CurrentApplication)
	{
		Insert.item.lParam = (UINT)CurrentApplication;
		Insert.item.pszText = CurrentApplication->Name;
		Insert.item.cchTextMax = lstrlen(CurrentApplication->Name); 
		SendMessage(hwnd, TVM_INSERTITEM, 0, (LPARAM)&Insert);
		CurrentApplication = CurrentApplication->Next;
	}
}

BOOL SetupControls (HWND hwnd)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);

	// Parse the XML file
	if (ProcessXML ("apps.xml", &Root) == FALSE)
		return FALSE;

	// Set up the controls
	hCategories = CreateWindowExW(0, WC_TREEVIEW, L"Categories", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS, 
							0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

	hApps = CreateWindowExW(0, WC_TREEVIEW, L"Applications", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS|TVS_SHOWSELALWAYS, 
							0, 0, 0, 0, hwnd, NULL, hInstance, NULL);

	hLogo = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_LOGO));
	hUnderline = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_UNDERLINE));

	hHelpButton = CreateWindowW (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 550, 10, 40, 40, hwnd, 0, hInstance, NULL);
	hUpdateButton = CreateWindowW (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 500, 10, 40, 40, hwnd, 0, hInstance, NULL);
	hDownloadButton = CreateWindowW (L"Button", L"", WS_CHILD|WS_VISIBLE|BS_BITMAP, 330, 505, 140, 33, hwnd, 0, hInstance, NULL);

	SendMessageW(hHelpButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_HELP)));
	SendMessageW(hUpdateButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_UPDATE)));
	SendMessageW(hDownloadButton, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP,(LPARAM)(HANDLE)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_DOWNLOAD)));

	// Set deflaut entry for hApps
	TV_INSERTSTRUCT Insert = {0}; 
	Insert.item.mask = TVIF_TEXT;
	Insert.item.pszText = Strings[IDS_CHOOSE_CATEGORY];
	Insert.item.cchTextMax = lstrlen(Strings[IDS_CHOOSE_CATEGORY]); 
	SendMessage(hApps, TVM_INSERTITEM, 0, (LPARAM)&Insert); 

	// Create Tree Icons
	HIMAGELIST hImageList = ImageList_Create(16, 16, ILC_COLORDDB, 1, 1);
	SendMessageW(hCategories, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);
	SendMessageW(hApps, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hImageList);

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
	AddItems (hCategories, Root.Children, NULL);

	return TRUE;
}

static void ResizeControl (HWND hwnd, int x1, int y1, int x2, int y2)
{
	// Make resizing a little easier
	MoveWindow(hwnd, x1, y1, x2-x1, y2-y1, TRUE);
}

static void DrawBitmap (HDC hdc, int x, int y, HBITMAP hBmp)
{
	BITMAP bm;
	HDC hdcMem = CreateCompatibleDC(hdc);

	SelectObject(hdcMem, hBmp);
	GetObject(hBmp, sizeof(bm), &bm);
	TransparentBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, 0xFFFFFF);

	DeleteDC(hdcMem);
}

HFONT GetFont (BOOL Title)
{
    LOGFONT Font;
    GetObject(GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &Font);

	int Height = Title ? 20 : 19;
	int Scale = Font.lfWidth/Font.lfHeight;
 
	return CreateFont(Height, Height*Scale, Font.lfEscapement, Font.lfOrientation, Title ? FW_EXTRABOLD : FW_NORMAL, Font.lfItalic, 
		Font.lfUnderline, Font.lfStrikeOut, Font.lfCharSet, Font.lfOutPrecision, Font.lfClipPrecision, Font.lfQuality, 
		Font.lfPitchAndFamily, Font.lfFaceName); 
}

static void DrawDescription (HDC hdc, RECT DescriptionRect)
{
	int i;

	// Backgroud
	Rectangle(hdc, DescriptionRect.left, DescriptionRect.top, DescriptionRect.right, DescriptionRect.bottom);

	// Underline
	for (i=DescriptionRect.left+1;i<DescriptionRect.right-1;i++)
		DrawBitmap(hdc, i, DescriptionRect.top+22, hUnderline); // less code then stretching ;)

	// Headline
	HFONT Font = GetFont(TRUE);
	SelectObject(hdc, Font);
	RECT Rect = {DescriptionRect.left+5, DescriptionRect.top+3, DescriptionRect.right-2, DescriptionRect.top+22};
	DrawTextW(hdc, DescriptionHeadline, lstrlen(DescriptionHeadline), &Rect, DT_SINGLELINE|DT_NOPREFIX);
	DeleteObject(Font);

	// Description
	Font = GetFont(FALSE);
	SelectObject(hdc, Font);
	Rect.top += 40;
	Rect.bottom = DescriptionRect.bottom-2;
	DrawTextW(hdc, DescriptionText, lstrlen(DescriptionText), &Rect, DT_WORDBREAK|DT_NOPREFIX); // ToDo: Call TabbedTextOut to draw a nice table
	DeleteObject(Font);

}

LRESULT CALLBACK WndProc (HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	static RECT DescriptionRect;

	switch (Message)
	{
		case WM_CREATE:
		{
			if(!SetupControls(hwnd))
				return -1;
			ShowMessage(Strings[IDS_WELCOME_TITLE], Strings[IDS_WELCOME]);
		} 
		break;

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, CreateSolidBrush(RGB(235,233,237)));
			DrawBitmap(hdc, 10, 12, hLogo);
			DrawDescription(hdc, DescriptionRect);
			EndPaint(hwnd, &ps);
		}
		break;

		case WM_COMMAND:
		{
			if(HIWORD(wParam) == BN_CLICKED)
			{
				if (lParam == (LPARAM)hDownloadButton)
				{
					if(SelectedApplication)
						DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_DOWNLOAD), 0, DownloadProc);
					else
						ShowMessage(Strings[IDS_NO_APP_TITLE], Strings[IDS_NO_APP]);
				}
				else if (lParam == (LPARAM)hUpdateButton)
				{
					ShowMessage(Strings[IDS_UPDATE_TITLE], Strings[IDS_UPDATE]);
				}
				else if (lParam == (LPARAM)hHelpButton)
				{
					ShowMessage(Strings[IDS_HELP_TITLE], Strings[IDS_HELP]);
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
					struct Category* Category = (struct Category*) ((LPNMTREEVIEW)lParam)->itemNew.lParam;
					CategoryChoosen (hApps, Category);
				}
				else if(data->hwndFrom == hApps) 
				{
					SelectedApplication = (struct Application*) ((LPNMTREEVIEW)lParam)->itemNew.lParam;
					if(SelectedApplication)
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
			RECT Rect = {Split_Vertical+5, Split_Hozizontal+5, LOWORD(lParam)-10, HIWORD(lParam)-50};
			DescriptionRect = Rect;

			MoveWindow(hHelpButton, LOWORD(lParam)-50, 10, 40, 40, FALSE);
			MoveWindow(hUpdateButton, LOWORD(lParam)-100, 10, 40, 40, FALSE);
			MoveWindow(hDownloadButton, (Split_Vertical+LOWORD(lParam))/2-70, HIWORD(lParam)-45, 140, 35, FALSE);

			RECT Top = {0,0,LOWORD(lParam),60};
			InvalidateRect(hwnd, &Top, TRUE);
			RECT Description = {Split_Vertical, Split_Hozizontal, LOWORD(lParam), HIWORD(lParam)};
			InvalidateRect(hwnd, &Description, TRUE);
		}
		break;

		case WM_DESTROY:
		{
			DeleteObject(hLogo);
			if(Root.Children)
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
	int i;

	LoadLibrary(L"riched20.dll");
	InitCommonControls();

	// Load strings
	for(i=0; i<STRING_COUNT; i++)
		LoadStringW(hInstance, i, Strings[i], MAX_STRING_LENGHT); // if you know a better method please tell me. 

	// Create the window
	WNDCLASSEXW WndClass = {0};
	WndClass.cbSize			= sizeof(WNDCLASSEX); 
	WndClass.lpszClassName	= L"Downloader";
	WndClass.lpfnWndProc	= WndProc;
	WndClass.hInstance		= hInstance;
	WndClass.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
	WndClass.hCursor		= LoadCursor(NULL, IDC_ARROW);

	RegisterClassEx(&WndClass);

	hwnd = CreateWindowW(L"Downloader", 
						Strings[IDS_WINDOW_TITLE],
						WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
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
