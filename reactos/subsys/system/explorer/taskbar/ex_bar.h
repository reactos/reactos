//
// Explorer Shutdown PlugIn
// 
// Alexander Ciobanu
// alex@prodidactica.md
//

/*
  This file Contains structures and other stuff needed to develop plugins for
  Explorer Bar
*/
typedef struct _EXBAR_INFO {
   int x;
   int y;
   int dx;
   int dy;
} EXBARINFO, *PEXBARINFO;


typedef int (*PInitializePlugIn)(HWND ExplorerHandle);
typedef int (*PQuitPlugIn)();
typedef char*(*PPlugInInfo)(int InfoNmbr);
typedef int (*PPlugInCallBack)(HWND PlgnHandle, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef int (*PReloadConfig)();
typedef int (*PExplorerInfo)(EXBARINFO* info);

struct PluginCalls {
	PInitializePlugIn	InitializePlugIn;
	PQuitPlugIn			QuitPlugIn;
	PReloadConfig		ReloadPlugInConfiguration;
	PPlugInInfo			PlugInInfo;
	PExplorerInfo		ExplorerInfo;
	PPlugInCallBack		PlugInMessageProc;
};


#ifndef _PLUGINS
extern struct PluginCalls plugincalls_Menu;
extern struct PluginCalls plugincalls_Shutdown;
extern struct PluginCalls plugincalls_Clock;
#endif
