#ifndef _FSMENU_H
#define _FSMENU_H
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------

typedef enum
{
	FMF_NONE	 	= 0x0000,
	FMF_NOEMPTYITEM		= 0x0001,
        FMF_INCLUDEFOLDERS      = 0x0002,
        FMF_NOPROGRAMS      	= 0x0004,
        FMF_FILESMASK      	= 0x0007,
        FMF_LARGEICONS      	= 0x0008,
        FMF_NOBREAK      	= 0x0010,
        FMF_NOABORT      	= 0x0020,
} FMFLAGS;

#define FMAI_SEPARATOR		0x00000001

typedef void (CALLBACK *PFNFMCALLBACK)(LPITEMIDLIST, LPITEMIDLIST);

WINSHELLAPI UINT 	WINAPI FileMenu_ReplaceUsingPidl(HMENU hmenu, UINT idNewItems,  LPITEMIDLIST pidl, UINT fMenuFilter, PFNFMCALLBACK pfncb);
WINSHELLAPI BOOL 	WINAPI FileMenu_InitMenuPopup(HMENU hmenu);
WINSHELLAPI LRESULT WINAPI FileMenu_DrawItem(HWND hwnd, DRAWITEMSTRUCT FAR *lpdi);
WINSHELLAPI LRESULT WINAPI FileMenu_MeasureItem(HWND hwnd, MEASUREITEMSTRUCT FAR *lpmi);
WINSHELLAPI UINT 	WINAPI FileMenu_DeleteAllItems(HMENU hmenu);
WINSHELLAPI LRESULT WINAPI FileMenu_HandleMenuChar(HMENU hmenu, char ch);
WINSHELLAPI BOOL 	WINAPI FileMenu_GetLastSelectedItemPidls(HMENU hmenu, LPITEMIDLIST *ppidlFolder, LPITEMIDLIST *ppidlItem);
WINSHELLAPI HMENU 	WINAPI FileMenu_FindSubMenuByPidl(HMENU hmenu, LPITEMIDLIST pidl);
WINSHELLAPI UINT 	WINAPI FileMenu_InsertUsingPidl(HMENU hmenu, UINT idNewItems,  LPITEMIDLIST pidl, FMFLAGS fmf, UINT fMenuFilter, PFNFMCALLBACK pfncb);
WINSHELLAPI void 	WINAPI FileMenu_Invalidate(HMENU hmenu);
WINSHELLAPI HMENU   WINAPI FileMenu_Create(COLORREF clr, int cxBmpGap, HBITMAP hbmp, int cySel, FMFLAGS fmf);
WINSHELLAPI BOOL    WINAPI FileMenu_AppendItem(HMENU hmenu, LPSTR psz, UINT id, int iImage, HMENU hmenuSub, UINT cyItem);
WINSHELLAPI BOOL    WINAPI FileMenu_TrackPopupMenuEx(HMENU hmenu, UINT Flags, int x, int y, HWND hwndOwner, LPTPMPARAMS lpTpm);
WINSHELLAPI BOOL 	WINAPI FileMenu_DeleteItemByCmd(HMENU hmenu, UINT id);
WINSHELLAPI void 	WINAPI FileMenu_Destroy(HMENU hmenu);
WINSHELLAPI BOOL 	WINAPI FileMenu_EnableItemByCmd(HMENU hmenu, UINT id, BOOL fEnable);
WINSHELLAPI BOOL 	WINAPI FileMenu_DeleteSeparator(HMENU hmenu);
WINSHELLAPI BOOL 	WINAPI FileMenu_DeleteMenuItemByFirstID(HMENU hmenu, UINT id);
WINSHELLAPI DWORD 	WINAPI FileMenu_GetItemExtent(HMENU hmenu, UINT iItem);
WINSHELLAPI BOOL 	WINAPI FileMenu_DeleteItemByIndex(HMENU hmenu, UINT iItem);
WINSHELLAPI void 	WINAPI FileMenu_AbortInitMenu(void);

#endif //_FSMENU_H
