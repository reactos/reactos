/*****************************************************************/ 
/**						 Microsoft Windows						**/
/**				Copyright (C) Microsoft Corp., 1993				**/
/*****************************************************************/ 

/*
    msshrui.h
    Prototypes and definitions for sharing APIs

    FILE HISTORY:
    gregj	06/03/93	Created
*/

#ifndef _INC_MSSHRUI
#define _INC_MSSHRUI

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


BOOL WINAPI IsPathShared(LPCSTR lpPath, BOOL fRefresh);
UINT WINAPI ShareDirectoryNotify(HWND hwnd, LPCSTR lpDir, DWORD dwOper);

#ifndef WNDN_MKDIR
#define WNDN_MKDIR  1
#define WNDN_RMDIR  2
#define WNDN_MVDIR  3
#endif

#define ORD_SHARESHUTDOWNNOTIFY	12

BOOL WINAPI ShareShutdownNotify(DWORD dwFlags, UINT uiMessage, WPARAM wParam, LPARAM lParam);
typedef BOOL (WINAPI *pfnShareShutdownNotify)(DWORD dwFlags, UINT uiMessage, WPARAM wParam, LPARAM lParam);


#ifndef RC_INVOKED
#pragma pack()
#endif

#ifdef __cplusplus
};
#endif  /* __cplusplus */

#endif  /* !_INC_MSSHRUI */
