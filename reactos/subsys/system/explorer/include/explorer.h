
 // launch start programs
extern int startup( int argc, char *argv[] );

 // winefile main routine
extern int winefile_main(HINSTANCE hinstance, HWND hwndParent, int cmdshow);

 // display file manager window
extern void ShowFileMgr(HWND hWndParent, int cmdshow);

 // start desktop bar
extern HWND InitializeExplorerBar(HINSTANCE hInstance, int nCmdShow);

 // load plugins
extern int ExplorerLoadPlugins(HWND ExplWnd);

 // launch a program or document file
extern BOOL launch_file(HWND hwnd, LPCTSTR cmd, UINT nCmdShow);
#ifdef UNICODE
extern BOOL launch_fileA(HWND hwnd, LPSTR cmd, UINT nCmdShow);
#else
#define	launch_fileA launch_file
#endif

