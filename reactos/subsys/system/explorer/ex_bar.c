// Explorer Panel (PlugIn based)
// 
// Alexander Ciobanu
// alex@prodidactica.md
//
//

#include <windows.h>
#include <stdio.h>

HFONT tf;
HINSTANCE PlugInsHI[2];                      // PlugIns table
int PlugNumber;				  // Number of loaded plugins

LRESULT WINAPI ExplorerBarProc(HWND, UINT, WPARAM, LPARAM);

// Loads a configuration style given by PInt
// FIXME : Load all these values from registry !
//
DWORD LoadProperty(int PInt)
{
 switch(PInt)
 {
  case 1:                      // WS_EX_Style for creating the bar
    return WS_EX_DLGMODALFRAME | WS_EX_TOPMOST;
  break;
  case 2:                      // WS_Style for creating the bar
    return 0;
  break;
  case 3:		       // Start X for the panel
    return 0;
  break;
  case 4:
    return 0;			 // Start Y for the panel
  break;
  case 5:
    return GetSystemMetrics(SM_CXSCREEN);     // XLen for the panel
  break;
  case 6:
    return 50;			// YLen for the panel
  break;
 }
 return 0;
}

// Initializez and creates the Explorer Panel
// HINSTANCE as a parameter
//
HWND InitializeExplorerBar(HINSTANCE hInstance, int nCmdShow)
{
  HWND ExplorerBar;
  WNDCLASS ExplorerBarClass;

  ExplorerBarClass.lpszClassName = "ExplorerBar";                   //  ExplorerBar classname
  ExplorerBarClass.lpfnWndProc = ExplorerBarProc;                   //  Default Explorer Callback Procedure
  ExplorerBarClass.style = CS_VREDRAW | CS_HREDRAW;                 // Styles
  ExplorerBarClass.hInstance = hInstance;                           // Instance
  ExplorerBarClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);         // Configurable ????
  ExplorerBarClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  ExplorerBarClass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH); // BackGround
  ExplorerBarClass.lpszMenuName = NULL;				       // No Menu needed for the bar
  ExplorerBarClass.cbClsExtra = 0;					// Nothing YET! !!
  ExplorerBarClass.cbWndExtra = 0;					//

  if (RegisterClass(&ExplorerBarClass) == 0)                          // Cold not register anything :(
    {
      fprintf(stderr, "Could not register Explorer Bar. Last error was 0x%X\n",GetLastError());
      return NULL;
    }

  ExplorerBar = CreateWindowEx(LoadProperty(1),"ExplorerBar",
		      "ReactOS Explorer Bar",LoadProperty(2),LoadProperty(3),LoadProperty(4),
                        LoadProperty(5),LoadProperty(6),NULL,NULL,hInstance,NULL);
  if (ExplorerBar == NULL)
    {
      fprintf(stderr, "Cold not create Explorer Bar.Last error 0x%X\n",GetLastError());
      return(NULL);
    }

	tf = CreateFontA(14, 0, 0, TA_BASELINE, FW_NORMAL, FALSE, FALSE, FALSE,
		ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		DEFAULT_QUALITY, FIXED_PITCH|FF_DONTCARE, "Timmons");

  ShowWindow(ExplorerBar, nCmdShow);                                             // Show the bar
  return ExplorerBar;
}


// **************************************************************************************
// *                                  Default Buit-in Plugin                            *
// **************************************************************************************
HWND epl_AppButtons[10];
char epl_line[10][80];
int epl_Buttons;



// Initialize the plugin
//
HINSTANCE InitializeExplorerPlugIn(HWND ExplorerHandle)
{
 FILE* Conf;    // Configuration File;
 char line[80];  // Blah Blah Blah
 char ttl[80];   // Title of the button
 int i;
 int x;

 if (!(Conf=fopen("explorer.lst","r")))   // Error !
  {
   fprintf(stderr,"DefaultPlugin : No configuration file found !\n");
   return NULL;
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

   printf("1.%s   2.%s\n",ttl,line);
   strcpy(epl_line[i],line);

   epl_AppButtons[i] = CreateWindow(
    "BUTTON",ttl,WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
      (i*102)+2, 2, 100, 20, ExplorerHandle, NULL, (HINSTANCE) GetWindowLong(ExplorerHandle, GWL_HINSTANCE),NULL);
  }

 return (HINSTANCE) GetWindowLong(ExplorerHandle, GWL_HINSTANCE);
}

// Get Information about the plugin
//
char* ExplorerPlugInInfo(int InfoNmbr)
{
  static char Info[256];

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
int ReloadExplorerPlugInConfinguration()
{
 return 1;
}

// Quit plugin
//
int QuitExplorerPlugIn()
{
 return 1;
}


// Callback procedure for plugin
//
int ExplorerPlugInMessageProc(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam)
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
            system(epl_line[i]);
          }
      }
    break;
  }
 return 1;
}

// **************************************************************************************
// **************************************************************************************




// ----------------------------------------------------------- PlugIns control Functions !


// Load Plugin Function
// FIXME : Really must load all plugins in the plugins directory in SYSTEM32/Explorer
//
int ExplorerLoadPlugins(HWND ExplWnd)
{
 PlugInsHI[0] = InitializeExplorerPlugIn(ExplWnd);
 if (PlugInsHI[0] == NULL)
   {
     return 0;
   }

 if (!ReloadExplorerPlugInConfinguration(PlugInsHI[0]))
   {
     fprintf(stderr,"PlugIn %s could not reload it's configuration !",ExplorerPlugInInfo(0));
     return 0;
   } 
 return 1;
}

int WINAPI Ex_BarMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
  MSG msg;
  HWND ExplHnd;

 // Initializing the Explorer Bar !
 //

  if (!(ExplHnd=InitializeExplorerBar(hInstance, nCmdShow)))
     {
       fprintf(stderr,"FATAL : Explorer bar could not be initialized properly ! Exiting !\n");
       return 1;
     }

 // Load plugins !
    if (!ExplorerLoadPlugins(ExplHnd))
    {
      fprintf(stderr,"FATAL : No plugin could be loaded ! Exiting !\n");
      return 1;
    }

  while(GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  DeleteObject(tf);

  return 0;
}

LRESULT CALLBACK ExplorerBarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;

	switch(msg)
	{
	case WM_PAINT:
          hDC = BeginPaint(hWnd, &ps);
	  SelectObject(hDC, tf);
	  EndPaint(hWnd, &ps);
          ExplorerPlugInMessageProc(hWnd,msg,wParam,lParam);
	  break;

	case WM_DESTROY:
	  PostQuitMessage(0);
          QuitExplorerPlugIn();
	  break;

	default:
          ExplorerPlugInMessageProc(hWnd,msg,wParam,lParam);
	  return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}
