//
// Explorer Shutdown PlugIn
// 
// Alexander Ciobanu
// alex@prodidactica.md
//

#include <windows.h>
#include <stdio.h>

#include "ex_bar.h"

HWND ShwButton;
HWND Wnd;

int ex_x1;
int ex_y1;
int ex_dx;
int ex_dy;

// Initialize the plugin
//
static int InitializePlugIn(HWND ExplorerHandle)
{
  fprintf(stderr,"EX_SHUTDWN : INITIALIZE PLUGIN call\n");

  ShwButton = CreateWindow(
    TEXT("BUTTON"),TEXT("+"),WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
      ex_dx+ex_x1-33, 4, 25, ex_dy-14, ExplorerHandle, NULL, 
     (HINSTANCE) GetWindowLong(ExplorerHandle, GWL_HINSTANCE),NULL);

  return 1;
}

// Get Information about the plugin
//
static char* PlugInInfo(int InfoNmbr)
{
  static char Info[256];

  fprintf(stderr,"EX_SHUTDWN : INFORMATION PLUGIN call\n");

  switch(InfoNmbr)
  {
    case 0:                                   // PlugIn Name
     strcpy(Info,"ReactOSShutdown");
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
  fprintf(stderr,"EX_SHUTDWN : RELOAD PLUGIN COFIGURATION call\n");
  return 1;
}

// Quit plugin
//
static int QuitPlugIn()
{
  fprintf(stderr,"EX_SHUTDWN : QUIT PLUGIN call\n");
  return 1;
}

// Callback procedure for plugin
//
static int PlugInMessageProc(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  
 // The plugin must decide whatever the handle passed is created by it !
 // Sorry for bad english :-)
 //
 switch(Msg)
  {
   case WM_COMMAND:
        if ((HWND)lParam==ShwButton)
          {
            printf("Pressed ShutDown Button : \n");
            DestroyWindow(PlgnHandle);
          }
    break;
  }

  return 1;
}

static int ExplorerInfo(EXBARINFO* info)
{
  fprintf(stderr,"EX_SHUTDWN : EXPLORER INFO PLUGIN call\n");
  ex_x1=info->x;
  ex_y1=info->y;
  ex_dx=info->dx;
  ex_dy=info->dy;

  return 1;
}

#ifdef _PLUGIN
BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
  fprintf(stderr,"EX_SHUTDWN PlugIn loaded succesefully\n");
  return TRUE;
}
#endif


struct PluginCalls plugincalls_Shutdown = {
	InitializePlugIn,
	QuitPlugIn,
	ReloadPlugInConfiguration,
	PlugInInfo,
	ExplorerInfo,
	PlugInMessageProc
};

