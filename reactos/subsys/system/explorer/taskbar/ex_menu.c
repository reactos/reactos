//
// Explorer Start Menu PlugIn (Example)
// 
// Alexander Ciobanu
// alex@prodidactica.md
//

#include <windows.h>
#include <stdio.h>

#include "ex_bar.h"

HWND epl_AppButtons[10];
char epl_line[10][80];
int  epl_Buttons;

int  ex_x1;
int  ex_y1;
int  ex_dx;
int  ex_dy;

// Initialize the plugin
//
static int InitializePlugIn(HWND ExplorerHandle)
{
 FILE* Conf;    // Configuration File;
 char line[80];  // Blah Blah Blah
 char ttl[80];   // Title of the button
 int i;
 int x;

  fprintf(stderr,"EX_MENU : INITIALIZE PLUGIN call\n");

 if (!(Conf=fopen("explorer.lst","r")))   // Error !
  {
   fprintf(stderr,"DefaultPlugin : No configuration file found !\n");
   return 0;
  }
 
 fgets(line,80,Conf);              // Read how many entries are in the file
 epl_Buttons=atoi(line);           // atoi it !


 for (i=0;i<epl_Buttons;i++)
 {
   fgets(ttl,80,Conf);            // Read stuff :)
   fgets(line,80,Conf);

   for (x=0;ttl[x];x++) if (ttl[x]<14){ttl[x]=0;break;}
    for (x=0;line[x];x++) if (line[x]<14){line[x]=0;break;}
   
   // FIXME : Got to get rid of #13,#10 at the end of the lines !!!!!!!!!!!!!!!!!!!

   strcpy(epl_line[i],line);

   epl_AppButtons[i] = CreateWindow(
    TEXT("BUTTON"),ttl/*@@*/,WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
      (i*102)+2, 2, 100,  ex_dy-10, ExplorerHandle, NULL, (HINSTANCE) GetWindowLong(ExplorerHandle, GWL_HINSTANCE),NULL);
  }

  return 1;
}

// Get Information about the plugin
//
static char* PlugInInfo(int InfoNmbr)
{
  static char Info[256];

  fprintf(stderr,"EX_MENU : INFORMATION PLUGIN call\n");

  switch(InfoNmbr)
  {
    case 0:                                   // PlugIn Name
     strcpy(Info,"ApplicationLauncher");
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
  fprintf(stderr,"EX_MENU : RELOAD PLUGIN COFIGURATION call\n");
  return 1;
}

// Quit plugin
//
static int QuitPlugIn()
{
  fprintf(stderr,"EX_MENU : QUIT PLUGIN call\n");
  return 1;
}


 // display a windows error message
static void display_error(HWND hwnd, DWORD error)
{
	PTSTR msg;

	if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		0, error, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL))
		MessageBox(hwnd, msg, TEXT("ROS Explorer"), MB_OK);
	else
		MessageBox(hwnd, TEXT("Error"), TEXT("ROS Explorer"), MB_OK);

	LocalFree(msg);
}

 // launch a program or document file
static BOOL launch_file(HWND hwnd, LPSTR cmd, UINT nCmdShow)
{
	HINSTANCE hinst = ShellExecuteA(hwnd, NULL/*operation*/, cmd, NULL/*parameters*/, NULL/*dir*/, nCmdShow);

	if ((int)hinst <= 32) {
		display_error(hwnd, GetLastError());
		return FALSE;
	}

	return TRUE;
}


// Callback procedure for plugin
//
static int PlugInMessageProc(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  int i;
  
 // The plugin must decide whatever the handle passed is created by it !
 // Sorry for bad english :-)
 //
 switch(Msg)
  {
   case WM_COMMAND:
     for (i=0;i<epl_Buttons;i++)
      {
        if ((HWND)lParam==epl_AppButtons[i])
          {
            printf("Pressed Button Line : %s\n",epl_line[i]);
            launch_file(PlgnHandle, epl_line[i], SW_SHOWNORMAL);
          }
      }
    break;
  }
  return 1;
}

// Callback function to get ExplorerBar's information
//
static int ExplorerInfo(EXBARINFO* info)
{
  fprintf(stderr,"EX_MENU : EXPLORER INFO PLUGIN call\n");
  ex_x1=info->x;
  ex_y1=info->y;
  ex_dx=info->dx;
  ex_dy=info->dy;

  return 1;
}


#ifdef _PLUGIN
BOOL WINAPI DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
  fprintf(stderr,"EX_MENU PlugIn loaded succesefully\n");
  return TRUE;
}
#endif


struct PluginCalls plugincalls_Menu = {
	InitializePlugIn,
	QuitPlugIn,
	ReloadPlugInConfiguration,
	PlugInInfo,
	ExplorerInfo,
	PlugInMessageProc
};

