
 // launch start programs
extern int startup( int argc, char *argv[] );

 // winefile main routine
extern int winefile_main(HINSTANCE hinstance, HWND hwndParent, int cmdshow);

 // display file manager window
extern void ShowFileMgr(HWND hWndParent, int cmdshow);

extern int WINAPI Ex_BarMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpszCmdLine,
	int nCmdShow);

