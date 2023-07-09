/*
 * propsht.hpp - IPropSheetExt implementation description.
 */


/* Types
 ********/

/* User MessageBox() flags */

typedef enum msg_box_flags
{
   ALL_MSG_BOX_FLAGS    = (MB_OK |
                           MB_OKCANCEL |
                           MB_ABORTRETRYIGNORE |
                           MB_YESNOCANCEL |
                           MB_YESNO |
                           MB_RETRYCANCEL |
                           MB_ICONHAND |
                           MB_ICONQUESTION |
                           MB_ICONEXCLAMATION |
                           MB_ICONASTERISK |
                           MB_DEFBUTTON1 |
                           MB_DEFBUTTON2 |
                           MB_DEFBUTTON3 |
                           MB_DEFBUTTON4 |
                           MB_APPLMODAL |
                           MB_SYSTEMMODAL |
                           MB_TASKMODAL |
                           MB_HELP |
                           MB_RIGHT |
                           MB_RTLREADING |
                           MB_NOFOCUS |
                           MB_SETFOREGROUND |
                           MB_DEFAULT_DESKTOP_ONLY |
                           MB_USERICON)
}
MSG_BOX_FLAGS;


/* Prototypes
 *************/

// propsht.cpp

extern void SetEditFocus(HWND hwnd);
extern BOOL ConstructMessageString(PCSTR pcszFormat, PSTR *ppszMsg, va_list *ArgList);
extern BOOL __cdecl MyMsgBox(HWND hwndParent, PCSTR pcszTitle, PCSTR pcszMsgFormat, DWORD dwMsgBoxFlags, PINT pnResult, ...);
extern HRESULT CopyDlgItemText(HWND hdlg, int nControlID, PSTR *ppszText);
extern BOOL RegisterGlobalHotkey(WORD wOldHotkey, WORD wNewHotkey, PCSTR pcszPath);

