//--------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------

//----------------------------------------------------------------------------
typedef enum
{
    FMIUP_NONE                          = 0x0000,
    FMIUP_NO_EMPTY_ITEM                 = 0x0001,
    FMIUP_NO_PROGRAMS_SUBFOLDER         = 0x0002,         
    FMIUP_NO_SUBFOLDERS                 = 0x0004,
    FMIUP_DELAY                 	    = 0x0008,
} FMIUP_FLAGS;

//---------------------------------------------------------------------------
BOOL FM_Create(HIMAGELIST himlLarge, HIMAGELIST himlSmall, HMENU *phmenu, UINT cyExtra);
BOOL FM_AppendSubMenu(HMENU hmenu, LPCTSTR pszText, UINT id, UINT iImage, UINT fState, HMENU hmenuSub);
BOOL FM_AppendItem(HMENU hmenu, LPCTSTR pszText, UINT id, UINT fState, UINT iImage);
BOOL FM_InsertUsingPidl(HMENU hmenu, LPITEMIDLIST pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems);
BOOL FM_GetPidlFromMenuCommand(HMENU hmenu, UINT idCmd, LPITEMIDLIST *ppidlFolder, LPITEMIDLIST *ppidItem);
void FM_InvalidateAllImages(HMENU hmenu, BOOL fRecurse);
BOOL FM_SetImageSize(HMENU hmenu, BOOL fLarge);
BOOL FM_EnableItemByCmd(HMENU hmenu, UINT idCmd, BOOL fEnable);
BOOL FM_Destroy(HMENU hmenu);
BOOL FM_GetMenuFromPidl(HMENU hmenu, LPITEMIDLIST pidl, HMENU *phmenuSub, BOOL *pfExists);
BOOL FM_InsertItemByPidl(HMENU hmenu, LPITEMIDLIST pidl, FMIUP_FLAGS flags);
BOOL FM_DeleteItemByPidl(HMENU hmenu, LPITEMIDLIST pidl, FMIUP_FLAGS flags);
void FM_InvalidateItems(HMENU hmenu);
void FM_InvalidateItemsByPidl(HMENU hmenu, LPITEMIDLIST pidl);
BOOL FM_InvalidateImage(HMENU hmenu, UINT iImage);
BOOL FM_InvalidateAllImagesByPidl(HMENU hmenu, LPITEMIDLIST pidl);
BOOL FM_CreateFromMenu(HMENU hmenu, HIMAGELIST himlLarge, HIMAGELIST himlSmall, UINT cyExtra);
BOOL FM_DeleteAllItems(HMENU hmenu);
BOOL FM_ReplaceUsingPidl(HMENU hmenu, LPITEMIDLIST pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems);
BOOL FM_AppendUsingPidl(HMENU hmenu, LPITEMIDLIST pidlFolder, UINT idFirst, FMIUP_FLAGS flags, PUINT pcItems);

LRESULT FM_OnMeasureItem(LPMEASUREITEMSTRUCT pmi);
LRESULT FM_OnDrawItem(LPDRAWITEMSTRUCT pdi);
LRESULT FM_OnInitMenuPopup(HMENU hmenuPopup);


