#ifndef __props_h
#define __props_h


INT_PTR CALLBACK TargetDlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

#define MAX_NAME_LEN 20
#define FILENAME_LEN_WITH_SLASH_AND_NULL 14
#define MAX_DIR_PATH (MAX_PATH-FILENAME_LEN_WITH_SLASH_AND_NULL)


#endif
