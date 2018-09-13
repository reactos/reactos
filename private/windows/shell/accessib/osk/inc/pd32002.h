/*Filename: PD32002.H                                       */


/* Global vars */
extern HINSTANCE hInst;
extern HWND MainhWnd;
extern BOOL settingChanged;
extern HWND hClient;     /* Handle to window in client area.   */
extern FARPROC lpClient; /* Function for window in client area.*/

extern DWORD platform;




#define BLD_CannotRun          5000
#define BLD_CannotCreate       5001
#define BLD_CannotLoadMenu     5002
#define BLD_CannotLoadIcon     5003
#define BLD_CannotLoadBitmap   5004

#if !defined(THISISBLDRC)


/***************************************************************/
/* Variables, types and constants for controls in main window. */
/***************************************************************/

#define CLIENTSTRIP WS_MINIMIZE|WS_MAXIMIZE|WS_CAPTION|WS_BORDER|WS_DLGFRAME|WS_SYSMENU|WS_POPUP|WS_THICKFRAME|DS_MODALFRAME

typedef struct
  {
  unsigned long dtStyle;
  BYTE dtItemCount;
  int dtX;
  int dtY;
  int dtCX;
  int dtCY;
  } BLD_DLGTEMPLATE;

typedef BLD_DLGTEMPLATE            *LPBLD_DLGTEMPLATE;

#endif


//DIALOG DEFINES

#define CHECK     1
#define UNCHECK   0

/* User Defined ID Values               */
#define DLG_OK			 1
#define DLG_CANCEL  	 2

//About Dlg
#define About_Credits_but	101
#define shade	102



/**************************************************************/
//          Functions in this file
/**************************************************************/
void Create_The_Rest(LPSTR lpCmdLine, HINSTANCE hInstance);
void ReadIn_OldDict(HINSTANCE hInstance);


/****************************************************************/
//      Functions in pd32f2.c  and pd32002.c
/****************************************************************/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine
, int nCmdShow);
LRESULT WINAPI MainWndProc(HWND,unsigned,WPARAM, LPARAM);
BOOL BLDKeyTranslation(HWND, HACCEL, MSG *);
BOOL BLDInitApplication(HINSTANCE,HINSTANCE,int *,LPSTR);
BOOL BLDExitApplication(HWND hWnd);      /* Called just before exit of applicati
on  */
BOOL BLDMenuCommand(HWND, unsigned , WPARAM, LPARAM);
int SaveChangesMessage(HWND hwnd, char *msg);


