/*-----------------------------------------------------------------------
|	CTL3D.DLL
|	
|	Adds 3d effects to Windows controls
|
|	See ctl3d.doc for info
|		
-----------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif


BOOL WINAPI Ctl3dSubclassDlg(HWND, WORD);
BOOL WINAPI Ctl3dSubclassDlgEx(HWND, DWORD);

WORD WINAPI Ctl3dGetVer(void);
BOOL WINAPI Ctl3dEnabled(void);

HBRUSH WINAPI Ctl3dCtlColor(HDC, LONG);	// ARCHAIC, use Ctl3dCtlColorEx
HBRUSH WINAPI Ctl3dCtlColorEx(UINT wm, WPARAM wParam, LPARAM lParam);

BOOL WINAPI Ctl3dColorChange(void);

BOOL WINAPI Ctl3dSubclassCtl(HWND);
BOOL WINAPI Ctl3dSubclassCtlEx(HWND, int);
BOOL WINAPI Ctl3dUnsubclassCtl(HWND);

LONG WINAPI Ctl3dDlgFramePaint(HWND, UINT, WPARAM, LPARAM);

BOOL WINAPI Ctl3dAutoSubclass(HINSTANCE);
BOOL WINAPI Ctl3dAutoSubclassEx(HINSTANCE, DWORD);
BOOL WINAPI Ctl3dIsAutoSubclass(VOID);
BOOL WINAPI Ctl3dUnAutoSubclass(VOID);

BOOL WINAPI Ctl3dRegister(HINSTANCE);
BOOL WINAPI Ctl3dUnregister(HINSTANCE);

//begin DBCS: far east short cut key support
VOID WINAPI Ctl3dWinIniChange(void);
//end DBCS

/* Ctl3dAutoSubclassEx flags */
#define CTL3D_SUBCLASS_DYNCREATE	0x0001
#define CTL3D_NOSUBCLASS_DYNCREATE	0x0002

/* Ctl3d Control ID */
#define CTL3D_BUTTON_CTL	0
#define CTL3D_LISTBOX_CTL	1
#define CTL3D_EDIT_CTL		2
#define CTL3D_COMBO_CTL 	3
#define CTL3D_STATIC_CTL	4

/* Ctl3dSubclassDlg3d flags */
#define CTL3D_BUTTONS		0x0001
#define CTL3D_LISTBOXES		0x0002		
#define CTL3D_EDITS			0x0004	
#define CTL3D_COMBOS		0x0008
#define CTL3D_STATICTEXTS	0x0010		
#define CTL3D_STATICFRAMES	0x0020

#define CTL3D_NODLGWINDOW       0x00010000
#define CTL3D_ALL				0xffff

#define WM_DLGBORDER (WM_USER+3567)
/* WM_DLGBORDER *(int FAR *)lParam return codes */
#define CTL3D_NOBORDER		0
#define CTL3D_BORDER			1

#define WM_DLGSUBCLASS (WM_USER+3568)
/* WM_DLGSUBCLASS *(int FAR *)lParam return codes */
#define CTL3D_NOSUBCLASS	0
#define CTL3D_SUBCLASS		1

#define CTLMSGOFFSET 3569
#ifdef WIN32
#define CTL3D_CTLCOLORMSGBOX	(WM_USER+CTLMSGOFFSET)
#define CTL3D_CTLCOLOREDIT		(WM_USER+CTLMSGOFFSET+1)
#define CTL3D_CTLCOLORLISTBOX	(WM_USER+CTLMSGOFFSET+2)
#define CTL3D_CTLCOLORBTN		(WM_USER+CTLMSGOFFSET+3)
#define CTL3D_CTLCOLORSCROLLBAR (WM_USER+CTLMSGOFFSET+4)
#define CTL3D_CTLCOLORSTATIC	(WM_USER+CTLMSGOFFSET+5)
#define CTL3D_CTLCOLORDLG		(WM_USER+CTLMSGOFFSET+6)
#else
#define CTL3D_CTLCOLOR (WM_USER+CTLMSGOFFSET)
#endif


/* Resource ID for 3dcheck.bmp (for .lib version of ctl3d) */
#define CTL3D_3DCHECK 26567


#ifdef __cplusplus
}
#endif
