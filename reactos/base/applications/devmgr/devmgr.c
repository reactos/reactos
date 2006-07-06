/* Device manager
 * (C) 2005 - Hervé Poussineau (hpoussin@reactos.org)
 * GUI: Michael Fritscher (michael@fritscher.net)
 *
 */

#define INITGUID
#include <windows.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <commctrl.h>
#include <windows.h>
#include <commctrl.h>
#if defined (__GNUC__)
#include <winioctl.h>
#endif

/* FIXME: should be in cfgmgr32.h */
typedef DWORD               CONFIGRET;
typedef DWORD               DEVINST, *PDEVINST;
#define CM_DRP_DEVICEDESC   0x00000001
#define MAX_DEVICE_ID_LEN   200
#define MAX_CLASS_NAME_LEN  32
#define CR_SUCCESS          0x00000000
#define CR_NO_SUCH_DEVINST  0x0000000D
#define CR_NO_SUCH_VALUE    0x00000025
#ifdef _UNICODE
typedef WCHAR               *DEVINSTID_W;
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyW(DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG);
CONFIGRET WINAPI CM_Locate_DevNodeW(PDEVINST, DEVINSTID_W, ULONG);
#define CM_Get_DevNode_Registry_Property CM_Get_DevNode_Registry_PropertyW
#define CM_Locate_DevNode                CM_Locate_DevNodeW
#else
typedef CHAR                *DEVINSTID_A;
CONFIGRET WINAPI CM_Get_DevNode_Registry_PropertyA(DEVINST, ULONG, PULONG, PVOID, PULONG, ULONG);
CONFIGRET WINAPI CM_Locate_DevNodeA(PDEVINST, DEVINSTID_A, ULONG);
#define CM_Get_DevNode_Registry_Property CM_Get_DevNode_Registry_PropertyA
#define CM_Locate_DevNode                CM_Locate_DevNodeA
#endif
CONFIGRET WINAPI CM_Enumerate_Classes(ULONG, LPGUID, ULONG);
CONFIGRET WINAPI CM_Get_Child(PDEVINST, DEVINST, ULONG);
CONFIGRET WINAPI CM_Get_Sibling(PDEVINST, DEVINST, ULONG);
/* end of cfgmgr32.h */

/**************************************************************************
   Function Prototypes
**************************************************************************/

BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
void CreateListView(HINSTANCE, HWND);
void ResizeListView(HWND);
BOOL InitListView();
void InsertIntoListView(int, LPTSTR, LPTSTR);


LRESULT ListViewNotify(HWND, LPARAM);
void SwitchView(HWND, DWORD);
BOOL DoContextMenu(HWND, WPARAM, LPARAM);
void UpdateMenu(HWND, HMENU);
BOOL InsertListViewItems();
void PositionHeader();

void CreateButtons();
void ListByClass();

/**************************************************************************
   Global Variables
**************************************************************************/

HINSTANCE   g_hInst;
TCHAR       g_szClassName[] = TEXT("VListVwClass");
HWND        hWnd;
HWND        hwndListView;
HWND        hwndButtonListByClass;
HWND        hwndButtonListByConnection;
HWND        hwndButtonListByInterface;
HWND        hwndButtonExit;
TCHAR       temp [255];
HDC         hDC;
TCHAR       empty [255] = TEXT("                                                                                                                            ");

void ListByClass()
{
	GUID ClassGuid;
	TCHAR ClassDescription[MAX_PATH];
	TCHAR ClassName[MAX_CLASS_NAME_LEN];
	TCHAR PropertyBuffer[256];
	HKEY KeyClass;
	HDEVINFO hDevInfo;
	SP_DEVINFO_DATA DeviceInfoData;
	int i = 0, j;
	long Size;
	long rc;

	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);

	(void)ListView_DeleteAllItems(hwndListView);
	while (1)
	{
		CONFIGRET res;
		res = CM_Enumerate_Classes(i, &ClassGuid, 0);
		if (res == CR_NO_SUCH_VALUE)
			break;

		i++;
		ClassName[0] = '\0';
		if (!SetupDiClassNameFromGuid(
			&ClassGuid,
			ClassName,
			sizeof(ClassName) / sizeof(ClassName[0]),
			NULL))
		{
			_tprintf(_T("SetupDiClassNameFromGuid() failed with status 0x%lx\n"), GetLastError());
			continue;
		}

		/* Get class description */
		KeyClass = SetupDiOpenClassRegKey(
			&ClassGuid,
			KEY_READ);
		if (KeyClass == INVALID_HANDLE_VALUE)
		{
			_tprintf(_T("SetupDiOpenClassRegKey() failed with status 0x%lx\n"), GetLastError());
			continue;
		}
		Size = sizeof(ClassDescription);
		rc = RegQueryValue(KeyClass, NULL, ClassDescription, &Size);
		if (rc == ERROR_SUCCESS)
		{
			InsertIntoListView(i,ClassDescription,ClassName);
			TextOut(hDC, 200, 40, empty, (int) strlen(empty));
			TextOut(hDC, 200, 40, ClassDescription, (int) strlen(ClassDescription));
			_tprintf(_T("%d %s (%s)\n"), i, ClassName, ClassDescription);
		}
		else
			_tprintf(_T("RegQueryValue() failed with status 0x%lx\n"), rc);
		RegCloseKey(KeyClass);

		/* Enumerate devices in the class */
		hDevInfo = SetupDiGetClassDevs(
			&ClassGuid,
			NULL, /* Enumerator */
			NULL, /* hWnd parent */
			DIGCF_PRESENT);
		if (hDevInfo == INVALID_HANDLE_VALUE)
			continue;

		j = 0;
		while (1)
		{
			DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
			if (!SetupDiEnumDeviceInfo(
				hDevInfo,
				j,
				&DeviceInfoData))
			{
				break;
			}
			j++;
			if (SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&DeviceInfoData,
				SPDRP_DEVICEDESC,
				NULL, /* Property reg data type */
				(PBYTE)PropertyBuffer,
				sizeof(PropertyBuffer),
				NULL) /* Required size */)
			{
				_tprintf(_T("- %s\n"), PropertyBuffer);
				InsertIntoListView(0,PropertyBuffer," ");
			}
			else if (SetupDiGetDeviceRegistryProperty(
				hDevInfo,
				&DeviceInfoData,
				SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
				NULL, /* Property reg data type */
				(PBYTE)PropertyBuffer,
				sizeof(PropertyBuffer),
				NULL) /* Required size */)
			{
				_tprintf(_T("- %s\n"), PropertyBuffer);
				InsertIntoListView(0,PropertyBuffer," ");
				TextOut(hDC, 200, 40, empty, (int) strlen(empty));
				TextOut(hDC, 200, 40, PropertyBuffer, (int) strlen(PropertyBuffer));
			}
			else
				_tprintf(_T("SetupDiGetDeviceRegistryProperty() failed with status 0x%lx\n"), GetLastError());
		}
		SetupDiDestroyDeviceInfoList(hDevInfo);
	}
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
}

CONFIGRET GetDeviceName(DEVINST DevInst, LPTSTR Buffer, DWORD BufferLength)
{
	ULONG BufferSize = BufferLength * sizeof(TCHAR);
	CONFIGRET cr;
	cr = CM_Get_DevNode_Registry_Property(DevInst, CM_DRP_DEVICEDESC, NULL, Buffer, &BufferSize, 0);
	if (cr != CR_SUCCESS)
	{
		_tprintf(_T("CM_Get_DevNode_Registry_Property() failed, cr= 0x%lx\n"), cr);
	}
	return cr;
}

CONFIGRET ListSubNodes(DEVINST parent, DWORD Level)
{
	CONFIGRET cr;
	DEVINST child;

	cr = CM_Get_Child(&child, parent, 0);
	if (cr == CR_NO_SUCH_DEVINST)
		return CR_SUCCESS;
	else if (cr != CR_SUCCESS)
	{
		_tprintf(_T("CM_Get_Child() failed, cr= 0x%lx\n"), cr);
		return cr;
	}

	do
	{
#define DISPLAY_LENGTH (MAX_PATH + MAX_DEVICE_ID_LEN)
		DWORD DisplayLength = DISPLAY_LENGTH;
		TCHAR DisplayName[DISPLAY_LENGTH];
		ULONG i = Level;
		TCHAR LevelSpaces [ 255 ];
		cr = GetDeviceName(child, DisplayName, DisplayLength);
		LevelSpaces[0] = '\0';
		while (i-- != 0)
		{
			_tprintf(_T("   "));
			sprintf(LevelSpaces,"%s%s",LevelSpaces,"    ");
		}
		if (cr == CR_SUCCESS)
		{
			_tprintf(_T("%s\n"), DisplayName);
			sprintf(temp,"%s%s",LevelSpaces,DisplayName);
			InsertIntoListView(0,temp," ");
			TextOut(hDC, 200, 40, empty, (int) strlen(empty));
			TextOut(hDC, 200, 40, DisplayName, (int) strlen(DisplayName));
		}
		else
		{
			_tprintf(_T("(unknown device)\n"));
			sprintf(temp,"%s%s",LevelSpaces,"(unknown device)");
			InsertIntoListView(0,temp," ");
			TextOut(hDC, 200, 40, empty, (int) strlen(empty));
			TextOut(hDC, 200, 40, "(unknown device)", (int) strlen("(unknown device)"));
		}
		cr = ListSubNodes(child, Level + 1);
		if (cr != CR_SUCCESS)
			return cr;
		cr = CM_Get_Sibling(&child, child, 0);
		if (cr != CR_SUCCESS && cr != CR_NO_SUCH_DEVINST)
		{
			_tprintf(_T("CM_Get_Sibling() failed, cr= 0x%lx\n"), cr);
			return cr;
		}
	} while (cr == CR_SUCCESS);
	return CR_SUCCESS;
}

int ListByConnection()
{
	CONFIGRET cr;
	DEVINST root;
	(void)ListView_DeleteAllItems(hwndListView);

	cr = CM_Locate_DevNode(&root, TEXT("Root\\*PNP0A03\\0000"), 0);

	if (cr != CR_SUCCESS)
	{
		_tprintf(_T("CM_Locate_DevNode() failed, cr= 0x%lx\n"), cr);
		return 1;
	}
	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
	cr = ListSubNodes(root, 0);
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
	if (cr != CR_SUCCESS)
		return 2;
	return 0;
}

int ListByInterface(const GUID* guid)
{
	HDEVINFO hDevInfo;
	CHAR Buffer[sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 0x100];
	PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData;
	DWORD i;
	SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;

	(void)ListView_DeleteAllItems(hwndListView);

	DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)Buffer;
	DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

	hDevInfo = SetupDiGetClassDevs(
		guid,
		NULL, /* Enumerator */
		NULL, /* hwndParent */
		DIGCF_DEVICEINTERFACE);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs() failed with status 0x%lx\n", GetLastError());
		return 1;
	}

	i = 0;
	DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	SendMessage(hwndListView, WM_SETREDRAW, FALSE, 0);
	while (TRUE)
	{
		if (!SetupDiEnumDeviceInterfaces(
			hDevInfo,
			NULL,
			guid,
			i,
			&DeviceInterfaceData))
		{
			if (GetLastError() != ERROR_NO_MORE_ITEMS)
				printf("SetupDiEnumDeviceInterfaces() failed with status 0x%lx\n", GetLastError());
			break;
		}
		i++;
		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DeviceInterfaceData, DeviceInterfaceDetailData, sizeof(Buffer), NULL, NULL))
		{
			_tprintf(_T("- device %-2ld: %s\n"), i, DeviceInterfaceDetailData->DevicePath);
			TextOut(hDC, 200, 40, empty, (int) strlen(empty));
			TextOut(hDC, 200, 40, DeviceInterfaceDetailData->DevicePath, (int) strlen(DeviceInterfaceDetailData->DevicePath));
			InsertIntoListView(i,DeviceInterfaceDetailData->DevicePath," ");
		}
		else
		{
			_tprintf(_T("- device %ld\n"), i);
			InsertIntoListView(i," "," ");
		}

	}
	SendMessage(hwndListView, WM_SETREDRAW, TRUE, 0);
	SetupDiDestroyDeviceInfoList(hDevInfo);
	return 0;
}

/*int main(void)
{
	ListByClass();
	ListByInterface(&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR);
	ListByConnection();
	return 0;
}*/



//GUI
int WINAPI WinMain(  HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nCmdShow)
{
MSG  msg;

g_hInst = hInstance;

if(!hPrevInstance)
   if(!InitApplication(hInstance))
      return FALSE;

//required to use the common controls
InitCommonControls();

/* Perform initializations that apply to a specific instance */

if (!InitInstance(hInstance, nCmdShow))
   return FALSE;

/* Acquire and dispatch messages until a WM_QUIT uMessage is received. */

while(GetMessage( &msg, NULL, 0x00, 0x00))
   {
   TranslateMessage(&msg);
   DispatchMessage(&msg);
   }

return (int)msg.wParam;
}

BOOL InitApplication(HINSTANCE hInstance)
{
WNDCLASSEX  wcex;
ATOM        aReturn;

wcex.cbSize          = sizeof(WNDCLASSEX);
wcex.style           = 0;
wcex.lpfnWndProc     = (WNDPROC)MainWndProc;
wcex.cbClsExtra      = 0;
wcex.cbWndExtra      = 0;
wcex.hInstance       = hInstance;
wcex.hCursor         = LoadCursor(NULL, IDC_ARROW);
wcex.hbrBackground   = (HBRUSH)(COLOR_WINDOW );
wcex.lpszMenuName    = 0;
wcex.lpszClassName   = g_szClassName;
wcex.hIcon           = 0;
wcex.hIconSm         = 0;

aReturn = RegisterClassEx(&wcex);

if(0 == aReturn)
   {
   WNDCLASS wc;

   wc.style          = 0;
   wc.lpfnWndProc    = (WNDPROC)MainWndProc;
   wc.cbClsExtra     = 0;
   wc.cbWndExtra     = 0;
   wc.hInstance      = hInstance;
   wc.hIcon          = 0;
   wc.hCursor        = 0;
   wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW);
   wc.lpszMenuName   = 0;
   wc.lpszClassName  = g_szClassName;

   aReturn = RegisterClass(&wc);
   }

return aReturn;
}

BOOL InitInstance(   HINSTANCE hInstance,
                     int nCmdShow)
{
HWND     hWnd;
TCHAR    szTitle[MAX_PATH] = TEXT("Device viewer");

g_hInst = hInstance;

/* Create a main window for this application instance.  */
hWnd = CreateWindowEx(  0,
                        g_szClassName,
                        szTitle,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

/* If window could not be created, return "failure" */

if (!hWnd)
   return FALSE;

/* Make the window visible; update its client area; and return "success" */

ShowWindow(hWnd, nCmdShow);
UpdateWindow(hWnd);
hDC = GetDC(hWnd);
return TRUE;

}

LRESULT CALLBACK MainWndProc( HWND hWnd,
                              UINT uMessage,
                              WPARAM wParam,
                              LPARAM lParam)
{


switch (uMessage)
   {
   case WM_CREATE:
      // create the TreeView control
      CreateListView(g_hInst, hWnd);

      //initialize the TreeView control
      InitListView();

      CreateButtons(g_hInst, hWnd);
      TextOut(hDC, 200, 40, empty, (int) strlen(empty));
      break;


   case WM_SIZE:
      ResizeListView(hWnd);
      break;
   case WM_DESTROY:
      ReleaseDC(hWnd, hDC);
      PostQuitMessage(0);
      break;
   case WM_COMMAND:
      if (HIWORD(wParam) == BN_CLICKED &&
         (HWND) lParam == hwndButtonListByClass)
      {
	 ListByClass();
      }
      if (HIWORD(wParam) == BN_CLICKED &&
         (HWND) lParam == hwndButtonListByConnection)
      {
          ListByConnection();
      }
      if (HIWORD(wParam) == BN_CLICKED &&
         (HWND) lParam == hwndButtonListByInterface)
      {
         ListByInterface(&GUID_DEVINTERFACE_SERENUM_BUS_ENUMERATOR);
      }
      if (HIWORD(wParam) == BN_CLICKED &&
         (HWND) lParam == hwndButtonExit)
      {
          /* Close the window. */
          DestroyWindow (hWnd);
      }      return 0;
      break;
   default:
      break;
   }
return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

void CreateListView(HINSTANCE hInstance, HWND hwndParent)
{
DWORD       dwStyle;

dwStyle =   WS_TABSTOP |
            WS_CHILD |
            WS_BORDER |
            WS_VISIBLE |
            LVS_AUTOARRANGE |
            LVS_REPORT ;//|
            //LVS_OWNERDATA;

hwndListView = CreateWindowEx(   WS_EX_CLIENTEDGE,          // ex style
                                 WC_LISTVIEW,               // class name - defined in commctrl.h
                                 TEXT(""),                        // dummy text
                                 dwStyle,                   // style
                                 0,                         // x position
                                 0,                         // y position
                                 0,                         // width
                                 0,                         // height
                                 hwndParent,                // parent
                                 0,//(HMENU)ID_LISTVIEW,        // ID
                                 g_hInst,                   // instance
                                 NULL);                     // no extra data


ResizeListView(hwndParent);
}

void ResizeListView(HWND hwndParent)
{
RECT  rc;

GetClientRect(hwndParent, &rc);

MoveWindow( hwndListView,
            rc.left,
            rc.top+60,
            rc.right - rc.left,
            rc.bottom - rc.top-60,
            TRUE);
}

void PositionHeader()
{
HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

/*To ensure that the first item will be visible, create the control without
the LVS_NOSCROLL style and then add it here*/
dwStyle |= LVS_NOSCROLL;
SetWindowLong(hwndListView, GWL_STYLE, dwStyle);

//only do this if we are in report view and were able to get the header hWnd
if(((dwStyle & LVS_TYPEMASK) == LVS_REPORT) && hwndHeader)
   {
   RECT        rc;
   HD_LAYOUT   hdLayout;
   WINDOWPOS   wpos;

   GetClientRect(hwndListView, &rc);
   hdLayout.prc = &rc;
   hdLayout.pwpos = &wpos;

   (void)Header_Layout(hwndHeader, &hdLayout);

   SetWindowPos(  hwndHeader,
                  wpos.hwndInsertAfter,
                  wpos.x,
                  wpos.y,
                  wpos.cx,
                  wpos.cy,
                  wpos.flags | SWP_SHOWWINDOW);

   (void)ListView_EnsureVisible(hwndListView, 0, FALSE);
   }
}

BOOL InitListView()
{
LV_COLUMN   lvColumn;
int         i;
TCHAR       szString[3][20] = {TEXT("#"), TEXT("Name"), TEXT("Intern name")};

//empty the list
(void)ListView_DeleteAllItems(hwndListView);

//initialize the columns
lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
lvColumn.fmt = LVCFMT_LEFT;
i=0;
lvColumn.cx = 20;
lvColumn.pszText = szString[i];
(void)ListView_InsertColumn(hwndListView, i, &lvColumn);
i=1;
lvColumn.cx = 400;
lvColumn.pszText = szString[i];
(void)ListView_InsertColumn(hwndListView, i, &lvColumn);
i=2;
lvColumn.cx = 150;
lvColumn.pszText = szString[i];
(void)ListView_InsertColumn(hwndListView, i, &lvColumn);


return TRUE;
}

typedef struct tagLINE_INFO
{
    DWORD dwValType;
    LPTSTR name;
    void* val;
    size_t val_len;
} LINE_INFO, *PLINE_INFO;

void InsertIntoListView(int typ, LPTSTR name, LPTSTR intern_name)
{
   //MessageBox(hWnd, "You just pressed Ctrl+a", "Hotkey", MB_OK | MB_ICONINFORMATION);
    TCHAR temp[ 255 ];
    //LINE_INFO* linfo;
    LVITEM item;
    int index;
    //linfo->name = Name;
    item.mask = LVIF_TEXT;
    item.iItem = 9999;
    item.iSubItem = 0;
    item.state = 0;
    //item.statemask = 0;
    item.pszText=malloc(10);
    if (typ>=1)
    {
    	sprintf(temp,"%i",typ);
    	item.pszText = temp;
    }
    else
    	item.pszText = "";
    item.cchTextMax = (int) _tcslen(item.pszText);
    if (item.cchTextMax == 0)
        item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = 0;
    //item.iIdent = 0;
    //item.iGroupId = 0;
    //item.cColumns = 0 ;
    //item.puColumns = 0;
    //item.lParam = (LPARAM)linfo;
    index = ListView_InsertItem(hwndListView, &item);
    ListView_SetItemText(hwndListView, index, 1, name);
    ListView_SetItemText(hwndListView, index, 2, intern_name);
}



void CreateButtons(HINSTANCE hInstance, HWND hwndParent)
{

	  hwndButtonListByClass = CreateWindowEx (
	          0,
	          "button",         /* Builtin button class */
	          "List by Class",
	          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
	          0, 0, 190, 30,
	          hwndParent,             /* Parent is this window. */
	          0,        /* Control ID: 1 */
	          g_hInst,
	          NULL
	          );
	  hwndButtonListByConnection = CreateWindowEx (
	          0,
	          "button",         /* Builtin button class */
	          "List by Connection (PCI)",
	          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
	          200, 0, 190, 30,
	          hwndParent,             /* Parent is this window. */
	          0,        /* Control ID: 1 */
	          g_hInst,
	          NULL
	          );
	  hwndButtonListByInterface = CreateWindowEx (
	          0,
	          "button",         /* Builtin button class */
	          "List by Interface",
	          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
	          400, 0, 190, 30,
	          hwndParent,             /* Parent is this window. */
	          0,        /* Control ID: 1 */
	          g_hInst,
	          NULL
	          );
	  hwndButtonExit = CreateWindowEx (
	          0,
	          "button",         /* Builtin button class */
	          "Exit",
	          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
	          0, 30, 190, 30,
	          hwndParent,             /* Parent is this window. */
	          0,        /* Control ID: 1 */
	          g_hInst,
	          NULL
	          );
}
