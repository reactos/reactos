// registered window messages global variables, defined in mlui.cpp
extern UINT g_ML_GETTEXT,
            g_ML_GETTEXTLENGTH,
            g_ML_SETTEXT;

extern UINT g_ML_LB_ADDSTRING,
            g_ML_LB_FINDSTRING,
            g_ML_LB_FINDSTRINGEXACT,
            g_ML_LB_GETTEXT,
            g_ML_LB_GETTEXTLEN,
            g_ML_LB_INSERTSTRING,
            g_ML_LB_SELECTSTRING;

extern UINT g_ML_CB_ADDSTRING,
            g_ML_CB_FINDSTRING,
            g_ML_CB_FINDSTRINGEXACT,
            g_ML_CB_GETLBTEXT,
            g_ML_CB_GETLBTEXTLEN,
            g_ML_CB_INSERTSTRING,
            g_ML_CB_SELECTSTRING;


BOOL fDoMungeUI(HINSTANCE hinst);
INT_PTR MLDialogBoxIndirectParamI(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
INT_PTR MLDialogBoxParamI(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND MLCreateDialogIndirectParamI(HINSTANCE hInstance, LPCDLGTEMPLATE lpTemplate, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
HWND MLCreateDialogParamI(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hwndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
BOOL MLIsEnabled(HWND hwnd);
int MLGetControlTextI(HWND hWnd, LPCWSTR lpString, int nMaxCount);
BOOL MLSetControlTextI(HWND hWnd, LPCWSTR lpString);


