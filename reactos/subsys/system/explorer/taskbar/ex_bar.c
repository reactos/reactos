// Explorer Panel (PlugIn based)
// 
// Alexander Ciobanu
// alex@prodidactica.md
//
//

#include <windows.h>
#include <stdio.h>

#include "../utility/utility.h"

#include "ex_bar.h"

HFONT tf;

#ifdef _PLUGINS
HINSTANCE PlugInsHI[4]; 				// PlugIns table
#else
struct PluginCalls* PlugInsCallTable[4]; // PlugIn Call table
#endif

int PlugNumber = -1;					// Number of loaded plugins

LRESULT WINAPI ExplorerBarProc(HWND, UINT, WPARAM, LPARAM);


//#define TASKBAR_AT_TOP
#define	TASKBAR_WIDTH	30


// Loads a configuration style given by PInt
// FIXME : Load all these values from registry !
//
DWORD LoadProperty(int PInt)
{
 switch(PInt)
 {
  case 1:						// WS_EX_Style for creating the bar
	return WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_PALETTEWINDOW;
  break;
  case 2:						// WS_Style for creating the bar
	return WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN | WS_VISIBLE ;
  break;
  case 3:						// Start X for the panel
	return -2;	// hide border
  break;
  case 4:						// Start Y for the panel
#ifdef TASKBAR_AT_TOP
	return -2;
#else
	return GetSystemMetrics(SM_CYSCREEN)-TASKBAR_WIDTH;
#endif
  break;
  case 5:
	return GetSystemMetrics(SM_CXSCREEN)+4;	  // XLen for the panel
  break;
  case 6:
	return TASKBAR_WIDTH+2;				// YLen for the panel
  break;
 }

 return 0;
}

// Initialize and create the Explorer Panel
HWND InitializeExplorerBar(HINSTANCE hInstance)
{
	HWND ExplorerBar;
	WNDCLASS ExplorerBarClass;

	ExplorerBarClass.lpszClassName = TEXT("Shell_TrayWnd");			//	ExplorerBar classname
	ExplorerBarClass.lpfnWndProc = ExplorerBarProc;					//	Default Explorer Callback Procedure
	ExplorerBarClass.style = 0;				  // Styles
	ExplorerBarClass.hInstance = hInstance;							// Instance
	ExplorerBarClass.hIcon = LoadIcon(0, IDI_APPLICATION); 			// Configurable ????
	ExplorerBarClass.hCursor = LoadCursor(0, IDC_ARROW);
	ExplorerBarClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);		// BackGround
	ExplorerBarClass.lpszMenuName = NULL; 							// No Menu needed for the bar
	ExplorerBarClass.cbClsExtra = 0;
	ExplorerBarClass.cbWndExtra = 0;

	if (!RegisterClass(&ExplorerBarClass))
	{
	  fprintf(stderr, "Could not register Explorer Bar. Last error was 0x%X\n",GetLastError());
	  return 0;
	}

	ExplorerBar = CreateWindowEx(LoadProperty(1), TEXT("Shell_TrayWnd"),
					  TEXT("ReactOS Explorer Bar"), LoadProperty(2), LoadProperty(3), LoadProperty(4),
						LoadProperty(5), LoadProperty(6), 0, 0, hInstance, 0);
	if (!ExplorerBar)
	{
	  fprintf(stderr, "Cold not create Explorer Bar. Last error 0x%X\n",GetLastError());
	  return 0;
	}

	tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

	ShowWindow(ExplorerBar, SW_SHOW);								// Show the bar

	return ExplorerBar;
}


// **************************************************************************************
// *						 GENERAL PLUGIN CONTROL ROUTINES							*
// **************************************************************************************


// Reload a plug-in's configuration
//
static int ReloadPlugInConfiguration(int ID)
{
#ifdef _PLUGINS
 PReloadConfig PP = (PReloadConfig)GetProcAddress(PlugInsHI[ID],/*"_"*/"ReloadPlugInConfiguration");

 if (!PP)
   {
	fprintf(stderr,"PLUGIN %d, of Instance %0x ReloadPlugInConfig Failed\n",ID,PlugInsHI[ID]);
	return 0;
   }
#else
 PReloadConfig PP = PlugInsCallTable[ID]->ReloadPlugInConfiguration;
#endif

 return PP();
}

int QuitPlugIn(int ID)
{
#ifdef _PLUGINS
 PQuitPlugIn PP = (PQuitPlugIn)GetProcAddress(PlugInsHI[ID],/*"_"*/"QuitPlugIn");

 if (!PP)
   {
	fprintf(stderr,"PLUGIN %d, of Instance %0x QuitPlugIn Failed\n",ID,PlugInsHI[ID]);
	return 0;
   }
#else
 PQuitPlugIn PP = PlugInsCallTable[ID]->QuitPlugIn;
#endif

 PP();

// FreeLibrary(PlugInsHI[ID]);

 return 1;
}

int CallBackPlugIn(int ID, HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
#ifdef _PLUGINS
 PPlugInCallBack PP = (PPlugInCallBack)GetProcAddress(PlugInsHI[ID],/*"_"*/"PlugInMessageProc");
 if (!PP)
   {
	fprintf(stderr,"PLUGIN %d, of Instance %0x CallBackPlugIn Failed\n",ID,PlugInsHI[ID]);
	return 0;
   }
#else
 PPlugInCallBack PP = PlugInsCallTable[ID]->PlugInMessageProc;
#endif

 return PP(PlgnHandle,Msg,wParam,lParam);
}

int PostExplorerInfo(int ID, HWND ExplHandle)
{
 EXBARINFO Info;
 RECT rect;

#ifdef _PLUGINS
 PExplorerInfo PP = (PExplorerInfo)GetProcAddress(PlugInsHI[ID],/*"_"*/"ExplorerInfo");

 if (!PP)
   {
	fprintf(stderr,"PLUGIN %d, of Instance %0x PostExplorerInfo Failed\n",ID,PlugInsHI[ID]);
	return 0;
   }
#else
 PExplorerInfo PP = PlugInsCallTable[ID]->ExplorerInfo;
#endif

 GetWindowRect(ExplHandle,&rect);
 Info.x=rect.left;
 Info.dx=rect.right-rect.left;

 Info.y=rect.top;
 Info.dy=rect.bottom-rect.top; 

 return PP(&Info);
}


int InitializePlugIn(int ID, HWND ExplHandle)
{
#ifdef _PLUGINS
 PInitializePlugIn PP = (PInitializePlugIn)GetProcAddress(PlugInsHI[ID],/*"_"*/"InitializePlugIn");
 if (!PP)
   {
	fprintf(stderr,"PLUGIN %d, of Instance %0x InitializePlugIn Failed\n",ID,PlugInsHI[ID]);
	return 0;
   }
#else
 PInitializePlugIn PP = PlugInsCallTable[ID]->InitializePlugIn;
#endif

 return PP(ExplHandle);
}


int InitPlugin(int ID, HWND ExplWnd)
{
 if (!PostExplorerInfo(ID, ExplWnd))
   {
	fprintf(stderr, "PLUGIN %d : WARNING : Haven't received Explorer information !\n");
   }

 if (!InitializePlugIn(ID, ExplWnd))
  {
   fprintf(stderr,"PLUGIN %d : WARNING : Could not be initialized !\n",ID);
  }

 if (!ReloadPlugInConfiguration(ID))
  {
   fprintf(stderr,"PLUGIN %d : WARNING : Could not load configuration !\n",ID);
  }

 return 1;
}


#ifdef _PLUGINS
int LoadLocalPlugIn(char* fname, HWND ExplWnd)
{
 PlugNumber++;
 PlugInsHI[PlugNumber]=LoadLibraryA(fname);
 if (!(PlugInsHI[PlugNumber])) return 0;				// Could not load plugin

 InitPlugin(PlugNumber);

 return 1;
}
#endif

// Load All Available plugins
// FIXME : Plugins MUST be listed in Registry ! not in file
// For now, it only loads the buit-in plugin
//
int LoadAvailablePlugIns(HWND ExplWnd)
{
#ifdef _PLUGINS

 FILE* Conf;	 // Configuration File;
 char line[80];  // Blah Blah Blah
 int i;
 int x;
 int k;

 if (!(Conf=fopen("ex_bar.ini","r")))	// Error !
  {
   fprintf(stderr,"DefaultPlugin : No PLUGIN configuration file found !\n");
   return 0;
  }

 fgets(line,80,Conf);			   // Read how many entries are in the file
 k = atoi(line);				   // atoi it ! We get how many plugIns do we have


 for (i=0;i<k;i++)
 {		  // Read stuff :)
   fgets(line,80,Conf);
	for (x=0;line[x];x++) if (line[x]<14){line[x]=0;break;}

   if (!LoadLocalPlugIn(line,ExplWnd)) PlugNumber--; 
 }
 fclose(Conf);

#else

		 // static initialisation of plugins

		PlugInsCallTable[++PlugNumber] = &plugincalls_Menu;
		InitPlugin(PlugNumber, ExplWnd);

		PlugInsCallTable[++PlugNumber] = &plugincalls_Shutdown;
		InitPlugin(PlugNumber, ExplWnd);

		PlugInsCallTable[++PlugNumber] = &plugincalls_Clock;
		InitPlugin(PlugNumber, ExplWnd);

#endif

 return PlugNumber+1;	 // Just one plugin loaded for now !
}

// Release all available plugins
// FIXME : MUST really quit all plugins
//
int ReleaseAvailablePlugIns()
{
 int i;
 for (i=0;i<PlugNumber+1;i++)
			QuitPlugIn(i);
 return i;
}

// Pass messages to all available plugins
// FIXME : MUST pass messages to all available plugins NOT just Default one

int CallBackPlugIns(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 int i;
 for (i=0;i<PlugNumber+1;i++)
	 CallBackPlugIn(i, PlgnHandle, Msg, wParam, lParam);
 return 1;
}

// **************************************************************************************
// **************************************************************************************



// ----------------------------------------------------------- PlugIns control Functions !


/*
int WINAPI WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpszCmdLine,
		int nCmdShow)
{
  MSG msg;
  HWND ExplHnd;

 // Initializing the Explorer Bar !
 //

  if (!(ExplHnd=InitializeExplorerBar(hInstance)))
	 {
	   fprintf(stderr,"FATAL : Explorer bar could not be initialized properly ! Exiting !\n");
	   return 1;
	 }
//	Load plugins !
   if (!LoadAvailablePlugIns(ExplHnd))
	{
	  fprintf(stderr,"FATAL : No plugin could be loaded ! Exiting !\n");
	  return 1;
	}

  while(GetMessage(&msg, 0, 0, 0))
  {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
  }

  DeleteObject(tf);
  ReleaseAvailablePlugIns();
  return 0;
}
*/

LRESULT CALLBACK ExplorerBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;

	switch(msg) {
	  case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		SelectObject(hDC, tf);
		EndPaint(hWnd, &ps);
		CallBackPlugIns(hWnd,msg,wParam,lParam);
		break;

	  case WM_CLOSE:
		// Over-ride close. We close desktop with shutdown button
		break;

	  case WM_DESTROY:
		PostQuitMessage(0);
		break;

	  case WM_NCHITTEST: {
		LRESULT res = DefWindowProc(hWnd, msg, wParam, lParam);
		if (res>=HTSIZEFIRST && res<=HTSIZELAST) {
#ifdef TASKBAR_AT_TOP
			if (res == HTBOTTOM)	// enable vertical resizing at the lower border
#else
			if (res == HTTOP)		// enable vertical resizing at the upper border
#endif
				return res;
			else
				return HTCLIENT;	// disable any other resizing
		}
		return res;}

	  case WM_SYSCOMMAND:
		if ((wParam&0xFFF0) == SC_SIZE) {
#ifdef TASKBAR_AT_TOP
			if (wParam == SC_SIZE+6)// enable vertical resizing at the lower border
#else
			if (wParam == SC_SIZE+3)// enable vertical resizing at the upper border
#endif
				goto def;
			else
				return 0;			// disable any other resizing
		}
		goto def;

	  default: def:
		CallBackPlugIns(hWnd, msg, wParam, lParam);

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}
