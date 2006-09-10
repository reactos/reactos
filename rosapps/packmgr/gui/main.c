////////////////////////////////////////////////////////
//
// main.cpp
// 
// Implementation of the Package Manager GUI
//
//
// Maarten Bosma, 09.01.2004
// maarten.paul@bosma.de
//
////////////////////////////////////////////////////////////////////

#include "main.h"

// This is the struct where the toolbar is defined
const TBBUTTON Buttons [] =
{
	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

	{0, 1, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON}, // No Action
	{1, 2, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON}, // Install
	{2, 3, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON}, // Install from source
	{3, 4, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON}, // Update
	{4, 5, TBSTATE_INDETERMINATE, TBSTYLE_BUTTON}, // Unistall

	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
	{5, 6, TBSTATE_ENABLED, TBSTYLE_BUTTON}, // DoIt (tm)
	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},

	{6, 7, TBSTATE_ENABLED, TBSTYLE_BUTTON}, // Help
	{7, 8, TBSTATE_ENABLED, TBSTYLE_BUTTON}, // Options

	{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
};


// Application's Entry Point
int WINAPI WinMain (HINSTANCE hinst, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND       hwnd;
	MSG        msg;
	WNDCLASSEX wc = {0};
	WCHAR errbuf[2000];

	// Window creation
	wc.cbSize        = sizeof(WNDCLASSEX); 
	wc.lpszClassName = L"pgkmgr";
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC)WndProc;
	wc.hInstance     = hinst;
	wc.hIcon		 = LoadIcon(hinst, MAKEINTRESOURCE(IDI_MAIN));
	wc.hbrBackground = (HBRUSH)(COLOR_SCROLLBAR);

	RegisterClassEx(&wc);

	hwnd = CreateWindow(L"pgkmgr",
                       L"ReactOS - Package Manager v0.3",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,  
                       CW_USEDEFAULT,   
                       500, 600, 
                       NULL, NULL,
                       hinst, 
					   NULL);


	// Toolbar creation
	InitCommonControls();

	hTBar = CreateToolbarEx(hwnd, WS_CHILD|WS_VISIBLE|TBSTYLE_FLAT, 0, 8, hinst, IDB_TOOLBAR, 
										Buttons, sizeof(Buttons)/sizeof(TBBUTTON), TBSIZE, TBSIZE, TBSIZE, TBSIZE, sizeof(TBBUTTON));

	// Show the windows
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	// Load the tree
	int error = PML_LoadTree(&tree, "tree_bare.xml", AddItem);
	
	if(error)
	{
		MessageBox(0,PML_TransError(error, errbuf, sizeof(errbuf)/sizeof(WCHAR)),0,0);
		return 0;
	}
	
	// Read the help
	Help();

	// Start getting messages
	while(GetMessage(&msg,NULL,0,0))
	{
		if(!TranslateAccelerator(hwnd, hHotKeys, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	// Close our handle
	PML_CloseTree (tree);

	return 0;
}

// Add a item to our tree
int AddItem (int id, const char* name, int parent, int icon)
{ 
	TV_INSERTSTRUCT tvins; 

	tvins.item.lParam = (UINT)id;
	tvins.item.mask = TVIF_TEXT|TVIF_PARAM;
	tvins.item.pszText = (WCHAR*)name; //that is ok
	tvins.item.cchTextMax = strlen(name); 
	tvins.hInsertAfter = TVI_LAST;

	if(icon)
	{
		tvins.item.iImage = icon;
		tvins.item.iSelectedImage = icon;
		tvins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	}

	if (parent==0)
		tvins.hParent = TVI_ROOT;
	else
		tvins.hParent = nodes[parent];

	nodes[id] = (HTREEITEM)SendMessage(hTree, TVM_INSERTITEMA, 0, (LPARAM)&tvins);

	return 0;
} 

// Load the Help from file and display it
void Help (void)
{
	int i;
	char buffer [2000];
	FILE* file = fopen ("help.txt", "r");

	if(!file)
		return;

	for(i=0; i<2000; i++)
	{
		buffer[i] = getc(file);
		if(buffer[i]==EOF) break;
	}
	buffer[i] = 0;

	SetText(buffer);
}

// Create our Controls
void InitControls (HWND hwnd)
{

	HINSTANCE hinst = GetModuleHandle(NULL);
	WCHAR errbuf[2000];

	// Create the controls
	hTree = CreateWindowEx(0, WC_TREEVIEW, L"TreeView", WS_CHILD|WS_VISIBLE|WS_BORDER|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS, 
							0, 0, 0, 0, hwnd, NULL, hinst, NULL);

	hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"edit", PML_TransError(IDS_LOAD, errbuf, sizeof(errbuf)/sizeof(WCHAR)), WS_CHILD|WS_VISIBLE|ES_MULTILINE, 
							0, 0, 100, 100, hwnd, NULL, hinst, NULL);
	
	hPopup = LoadMenu(hinst, MAKEINTRESOURCE(IDR_POPUP));

	// Create Tree Icons
	HIMAGELIST hIcon = ImageList_Create(16, 16, ILC_MASK|ILC_COLOR32, 1, 1);
	SendMessage(hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)(HIMAGELIST)hIcon);
		
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(1))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(11))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(12))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(13))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(14))); 

	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(2))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(3))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(4))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(5))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(6))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(7))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(8))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(9))); 
	ImageList_AddIcon(hIcon, LoadIcon(hinst, MAKEINTRESOURCE(10))); 

	// Setup Hotkeys
	hHotKeys = LoadAccelerators (hinst, MAKEINTRESOURCE(IDR_HOTKEYS));
}

// Set the Icons
int SetIcon (int id, int icon) 
{
    TVITEMEX item;
	
	item.hItem = nodes[id];
	item.iImage = icon;
	item.iSelectedImage = icon;
	item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;

	return (int)TreeView_SetItem(hTree, &item);
}

// Set the Icons
int Ask (const WCHAR* message) 
{	
	int ans = MessageBox (0,message,0,MB_YESNO);

	if(ans == IDYES)
		return 1;

    return 0;
}

// En- or Disable a Button inside of the toolbar and the Context Menu
int SetButton (DWORD id, BOOL state) 
{
	// Change the Toorbar Button
    TBBUTTONINFO ti;

    ti.cbSize = sizeof (ti);
    ti.dwMask = TBIF_STATE;

	if(state)
		ti.fsState = TBSTATE_ENABLED;
	else
		ti.fsState = TBSTATE_INDETERMINATE;

    SendMessage (hTBar, TB_SETBUTTONINFO, id, (LPARAM)&ti);

	// Change the Context Menu item
	MENUITEMINFO mi;

    mi.cbSize = sizeof (mi);
    mi.fMask = MIIM_STATE;

	if(state)
		mi.fState = MFS_ENABLED;

	else
		mi.fState = MFS_GRAYED;

    SetMenuItemInfo(hPopup, id, FALSE, &mi);

	return 0;
}

// Set the text of the text box
int SetText (const char* text) 
{
	int i, j;
	char buffer [2000];

	if(!text)
		return 1;

	// the windows does not need "\n"
	// for new lines but "\r\n"
	for(i=0,j=0; text[i]; i++,j++)
	{
		buffer[j] = text[i];
		if(buffer[j] == '\n')
		{
			buffer[j] = '\r';
			buffer[++j] = '\n';
		}
	}
	buffer[j] = 0;

	SetWindowTextA(hEdit, buffer);

    return 0;
}

// Windows Message Callback (this is where most things happen)
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		// at the very beginning ...
		case WM_CREATE:
		{
			InitControls(hwnd);
		} 
		break;

		// calculate the size of the controls
		case WM_SIZE:
		{
            RECT rcl;
            SendMessage(hTBar, TB_AUTOSIZE, 0L, 0L);
            GetWindowRect(hTBar, &rcl);

			int win_top = rcl.bottom - rcl.top;
			int win_hight = HIWORD(lParam) - win_top;

            MoveWindow(hTree, 0, win_top, LOWORD(lParam), splitter_pos*win_hight/100, TRUE);
            MoveWindow(hEdit, 0, (splitter_pos*win_hight/100)+win_top, LOWORD(lParam), win_hight, TRUE);
		}
	    break;

		// for the treeview
		case WM_NOTIFY:
		{
			if(((LPNMHDR)lParam)->code == TVN_SELCHANGED) 
			{
				selected = ((LPNMTREEVIEW)lParam)->itemNew.lParam; 
				PML_LoadPackage (tree, selected, SetButton);
				SetText(PML_GetDescription (tree, selected));
			}

			else if ((int)(((LPNMHDR)lParam)->code) == NM_RCLICK) // <= aarrggg LISP
			{ 
				// which item has been click on
				HTREEITEM item = TreeView_GetDropHilight(hTree);

				if(item != NULL)
				{
					// mark the one as selected
					SendMessage (hTree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)item);
					(void)TreeView_EnsureVisible (hTree, item);
				}

				// create the context menu
				if(selected != 0)
				{
					POINT pt;
					GetCursorPos (&pt);
					TrackPopupMenu (GetSubMenu(hPopup, 0), 0, (UINT)pt.x, (UINT)pt.y, 0, hwnd, NULL);
				}
			}
		}
		break;

		// for the toolbar
		case WM_COMMAND:
		{
			// All Actions
			if(LOWORD(wParam) <= 5 && LOWORD(wParam) >= 1)
			{
				if(selected)
					if(PML_SetAction(tree, selected, LOWORD(wParam)-1, SetIcon, Ask) == ERR_OK)
						break;

				MessageBeep(MB_ICONHAND);
			}

			// DoIt
			else if(LOWORD(wParam)==6)
			{
				if(PML_DoIt(tree, SetStatus, Ask) == ERR_OK)
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DOIT), hwnd, StatusProc);
				else
					MessageBeep(MB_ICONHAND);
			}

			// Help
			else if(LOWORD(wParam)==7)
				Help();

			// Options
			else if(LOWORD(wParam)==8)
				DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_OPTIONS), hwnd, OptionsProc);
		}
		break;

		// prozess hotkeys
		case WM_HOTKEY:
		{
			if(PML_SetAction(tree, selected, wParam, SetIcon, Ask) != ERR_OK)
				MessageBeep(MB_ICONHAND);
		}
		break;

		// ... at the very end
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProc (hwnd, message, wParam, lParam);
}

// Warning: This function is called from another thread
int SetStatus (int status1, int status2, WCHAR* text)
{
	WCHAR errbuf[2000];

	// Set the Rage to 1000
	SendMessage(GetDlgItem(hStatus, IDC_STATUS1), PBM_SETRANGE32, 0, 1000);
	SendMessage(GetDlgItem(hStatus, IDC_STATUS2), PBM_SETRANGE32, 0, 1000);

	// The prozessbars and the text filds
	if(text)
		SetDlgItemText(hStatus, IDC_TSTATUS, text);

	if(status1!=-1)
		SendMessage(GetDlgItem(hStatus, IDC_STATUS1), PBM_SETPOS, status1, 0);

	if(status2!=-1)
		SendMessage(GetDlgItem(hStatus, IDC_STATUS2), PBM_SETPOS, status2, 0);

	// If the Status is 1000 everything is done
	if(status1==1000)
	{
		EndDialog(hStatus, TRUE);
		MessageBox(0,PML_TransError(status2, errbuf, sizeof(errbuf)/sizeof(WCHAR)),0,0);
	}

	return 0;
}

// Callback for the Status Dialog
INT_PTR CALLBACK StatusProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
		case WM_INITDIALOG:
		{
			hStatus = hwnd;
			
		} break;

		case WM_COMMAND: // can only be the about button
		case WM_CLOSE: // the close-window-[x]
		{
			PML_Abort();
			EndDialog(hwnd, TRUE);
			return 0;
		}
    }

    return 0;
}

// Callback for the Options Dialog
INT_PTR CALLBACK OptionsProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
		case WM_CLOSE:
			EndDialog(hwnd, TRUE);
			return 0;
    }

    return 0;
}
