#ifndef __WINE_DIALOG_H
#define __WINE_DIALOG_H

#include <windows.h>
#include <user32/winproc.h>

  /* Dialog info structure.
   * This structure is stored into the window extra bytes (cbWndExtra).
   * sizeof(DIALOGINFO) must be <= DLGWINDOWEXTRA (=30).
   */


typedef struct
{
    INT         msgResult;   /* +00 Last message result */
    DLGPROC     dlgProc;     /* +04 Dialog procedure */
    LONG        userInfo;    /* +08 User information (for DWL_USER) */

    /* implementation-dependent part */

    HWND        hwndFocus;   /* Current control with focus */
    HFONT       hUserFont;   /* Dialog font */
    HMENU       hMenu;       /* Dialog menu */
    UINT        xBaseUnit;   /* Dialog units (depends on the font) */
    UINT        yBaseUnit;
    INT	        idResult;    /* EndDialog() result / default pushbutton ID */
    UINT        flags;       /* EndDialog() called for this dialog */
    HGLOBAL     hDialogHeap;
} DIALOGINFO;


  /* Dialog control information */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    INT        x;
    INT        y;
    INT        cx;
    INT        cy;
    UINT       id;
    LPCSTR     className;
    LPCSTR     windowName;
    LPVOID     data;
} DLG_CONTROL_INFO;

  /* Dialog template */
typedef struct
{
    DWORD      style;
    DWORD      exStyle;
    DWORD      helpId;
    UINT       nbItems;
    INT        x;
    INT        y;
    INT        cx;
    INT        cy;
    LPCSTR     menuName;
    LPCSTR     className;
    LPCSTR     caption;
    WORD       pointSize;
    WORD       weight;
    WINBOOL    italic;
    LPCSTR     faceName;
    WINBOOL    dialogEx;
} DLG_TEMPLATE;


typedef struct {  
    WORD   dlgVer; 
    WORD   signature; 
    DWORD  helpID; 
    DWORD  exStyle; 
    DWORD  style; 
    WORD   cDlgItems; 
    short  x; 
    short  y; 
    short  cx; 
    short  cy; 
    void   *menu;         
    void   *windowClass;  
    WCHAR  title[0]; 
    short  pointsize;       
    short  weight;          
    short  bItalic;         
    WCHAR  font[0];  
} DLGTEMPLATEEX, *LPDLGTEMPLATEEX, *LPCDLGTEMPLATEEX; 

typedef struct { 
    DWORD  helpID; 
    DWORD  exStyle; 
    DWORD  style; 
    short  x; 
    short  y; 
    short  cx; 
    short  cy; 
    WORD   id; 
    void   *windowClass; 
    void   *title;       
    WORD   extraCount;     
} DLGITEMTEMPLATEEX, *LPDLGITEMTEMPLATEEX, *LPCDLGITEMTEMPLATEEX; 



typedef const DLGITEMTEMPLATE * LPCDLGITEMTEMPLATE;

  /* Dialog base units */
extern WORD xBaseUnit, yBaseUnit;

#define DF_END  0x0001
#define DF_ENDING 0x0002

HWND DIALOG_CreateIndirect( HINSTANCE hInst, void *dlgTemplate, HWND owner,
                              DLGPROC dlgProc, LPARAM param,
                              WINBOOL bUnicode );

WINBOOL DIALOG_IsDialogMessage( HWND hwnd, HWND hwndDlg,
                                      UINT message, WPARAM wParam,
                                      LPARAM lParam, WINBOOL *translate,
                                      WINBOOL *dispatch, INT dlgCode );


WINBOOL DIALOG_Init(void);

INT DIALOG_DoDialogBox( HWND hwnd, HWND owner );

#endif  /* __WINE_DIALOG_H */



