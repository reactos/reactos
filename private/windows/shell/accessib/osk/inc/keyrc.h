/*Filename: KEYRC.H                                         */

/* Give access to handles in all code modules */
extern HINSTANCE hInst;
extern HWND MainhWnd;
 



/***************************************************************/ 
/* Variables, types and constants for controls in main window. */ 
/***************************************************************/ 
 
extern HWND hClient;     /* Handle to window in client area.   */
extern FARPROC lpClient; /* Function for window in client area.*/
 
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
 
typedef BLD_DLGTEMPLATE  *LPBLD_DLGTEMPLATE;
 

    
/* User Defined ID Values               */
    
#define IDOK							  1
#define IDCANCEL						  2
#define COLORKBBACKGROUND			 10
#define COLORKBCHARACTERKEYS	  	 11
#define COLORKBTEXT				  	 12
#define COLORKBMODIFIERKEYS	  	 13
#define COLORKBDEFAULTSETTING		 14
#define COLORKBDEADKEYS				 15

#define SETDEFAULTSETTINGS			108
#define KBLANGUAGECOMBO				110
#define ALWAYSONTOP					111
#define KEYSIZE_0						112
#define KEYSIZE_1						113
#define KEYSIZE_2						114
#define KEYSIZE_3						115
#define KEYSIZE_4						116
#define DWELLONKEYS					118
#define DWELLMS					   119
#define HILITEKEY						120
#define DWELLTIMESCROLL				121
#define KBSHOWACTIVEKEYSCK			122
#define KBKEEPPROPORTIONSCK	 	123
#define KBUSE3DKEYSCK			  	124
#define KBUSECLICKSOUNDCK		 	125
#define KBSIZEBUT						126
#define KBFONTBUT						127
#define KBCOLORSBUT					128
#define IDC_LTEXT1					129

