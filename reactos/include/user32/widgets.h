#ifndef __WINE_WIDGETS_H
#define __WINE_WIDGETS_H

#include <user32/win.h>

#define WM_CTLCOLOR             0x0019




#define POPUPMENU_CLASS_NAME 	L"PopupMenu"
#define DESKTOP_CLASS_NAME   	L"Desktop"
#define DIALOG_CLASS_NAMEW    	L"DialogW"
#define DIALOG_CLASS_NAMEA    	L"DialogA"
#define DIALOG_CLASS_NAME_A     "DialogA"
#define WINSWITCH_CLASS_NAME 	L"WinSwitch"
#define ICONTITLE_CLASS_NAME 	L"IconTitle"
#define ICONTITLE_CLASS_NAME_A 	"IconTitle"
#define SCROLLBAR_CLASS_NAME 	L"ScrollBar"
#define BUTTON_CLASS_NAME 	L"Button"
#define EDIT_CLASS_NAME 	L"Edit"
#define STATIC_CLASS_NAME  	L"Static"
#define COMBOLBOX_CLASS_NAME	L"ComboLBox" 
#define COMBOBOX_CLASS_NAME	L"ComboBox" 
#define LISTBOX_CLASS_NAME	L"ListBox" 

typedef enum
{
    BIC_BUTTON,
    BIC_EDIT,
    BIC_LISTBOX,
    BIC_COMBO,
    BIC_COMBOLB,
//    BIC_POPUPMENU,
    BIC_STATIC,
    BIC_SCROLL,
//    BIC_MDICLIENT,
    BIC_DESKTOP,
    BIC_DIALOG,
    BIC_ICONTITLE,
    BIC_NB_CLASSES
} BUILTIN_CLASS;


/* Window procedures */

LRESULT STDCALL EditWndProc( HWND hwnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam );
LRESULT STDCALL ComboWndProc( HWND hwnd, UINT msg,
                                    WPARAM wParam, LPARAM lParam );
LRESULT STDCALL ComboLBWndProc( HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam );
LRESULT STDCALL ListBoxWndProc( HWND hwnd, UINT msg,
                                      WPARAM wParam, LPARAM lParam );
LRESULT STDCALL PopupMenuWndProc( HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam );
LRESULT STDCALL IconTitleWndProc( HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam );
LRESULT WINAPI StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam );
LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message,
                               WPARAM wParam, LPARAM lParam );

WINBOOL	WIDGETS_IsControl( WND* pWnd, BUILTIN_CLASS cls );

#endif