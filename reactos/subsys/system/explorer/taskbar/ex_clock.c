//
// Explorer Clock Plugin
// 
// Alexander Ciobanu
// alex@prodidactica.md
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "ex_bar.h"

HWND Static;
HWND Wnd;
char locstr[20];

int ex_x1;
int ex_y1;
int ex_dx;
int ex_dy;

// Initialize the plugin
//
static int InitializePlugIn(HWND ExplorerHandle)
{
 SYSTEMTIME systime;
 TCHAR TimeStr[20];

  fprintf(stderr,"EX_CLOCK : INITIALIZE PLUGIN call\n");

 SetTimer(ExplorerHandle,500,1000,NULL);
  GetLocalTime(&systime);
  wsprintf(TimeStr,TEXT("%02d:%02d"),systime.wHour,systime.wMinute);

 Static = CreateWindow(
    TEXT("STATIC"),TimeStr,WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN,
      ex_dx+ex_x1-100, 4, 50, ex_dy-14, ExplorerHandle, NULL, 
     (HINSTANCE) GetWindowLong(ExplorerHandle, GWL_HINSTANCE),NULL);

  return 1;
}

// Get Information about the plugin
//
char* PlugInInfo(int InfoNmbr)
{
  static char Info[256];

  fprintf(stderr,"EX_CLOCK : INFORMATION PLUGIN call\n");

  switch(InfoNmbr)
  {
    case 0:                                   // PlugIn Name
     strcpy(Info,"ReactOSClock");
     break;

    case 1:				      // Version
     strcpy(Info,"0.1");
     break;

    case 2:				      // Vendor name
     strcpy(Info,"ReactOS team");
     break;

    default:     			      // Default : Error
     strcpy(Info,"-");
     break;
 
  }

  return Info;
}

// Reload plugin's configuration
//
static int ReloadPlugInConfiguration()
{
  fprintf(stderr,"EX_CLOCK : RELOAD PLUGIN COFIGURATION call\n");
  return 1;
}

// Quit plugin
//
static int QuitPlugIn()
{
  fprintf(stderr,"EX_CLOCK : QUIT PLUGIN call\n");
  return 1;
}

// Callback procedure for plugin
//
static int PlugInMessageProc(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
 static int blink = 0;

 SYSTEMTIME systime;
 TCHAR TimeStr[20];

 // The plugin must decide whatever the handle passed is created by it !
 // Sorry for bad english :-)
 //
 switch(Msg)
  {
   case WM_TIMER:
		GetLocalTime(&systime);
		wsprintf(TimeStr, TEXT("%02d%c%02d"), systime.wHour, blink?':':' ', systime.wMinute);
		blink ^= 1;
		SendMessage(Static,WM_SETTEXT,0,(LPARAM)TimeStr);
    break;
  }

  return 1;
}

static int ExplorerInfo(EXBARINFO* info)
{
  fprintf(stderr,"EX_CLOCK : EXPLORER INFO PLUGIN call\n");
  ex_x1=info->x;
  ex_y1=info->y;
  ex_dx=info->dx;
  ex_dy=info->dy;
  return 1;
}

#ifdef _PLUGIN
BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
  fprintf(stderr,"EX_CLOCK PlugIn loaded succesefully\n");
  return TRUE;
}
#endif


struct PluginCalls plugincalls_Clock = {
	InitializePlugIn,
	QuitPlugIn,
	ReloadPlugInConfiguration,
	PlugInInfo,
	ExplorerInfo,
	PlugInMessageProc
};

