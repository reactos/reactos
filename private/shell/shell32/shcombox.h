// shcombox.h : Shared shell comboboxEx methods

#ifndef __SHCOMBOX_H__
#define __SHCOMBOX_H__

//-------------------------------------------------------------------------//
//  COMBOITEMEX wrap with string storage.
typedef struct tagCBXITEMA
{
    UINT    mask;
    INT_PTR iItem;
    CHAR    szText[MAX_PATH] ;
    int     cchTextMax;
    int     iImage;
    int     iSelectedImage;
    int     iOverlay;
    int     iIndent;
    int     iID;  // application-specific item identifier.
    ULONG   Reserved; 
    LPARAM  lParam;

} CBXITEMA, *PCBXITEMA ;
typedef CBXITEMA CONST *PCCBXITEMA;

typedef struct tagCBXITEMW
{
    UINT    mask;
    INT_PTR iItem;
    WCHAR   szText[MAX_PATH] ;
    int     cchTextMax;
    int     iImage;
    int     iSelectedImage;
    int     iOverlay;
    int     iIndent;
    int     iID;  // application-specific item identifier.
    ULONG   Reserved; 
    LPARAM  lParam;

} CBXITEMW, *PCBXITEMW ;
typedef CBXITEMW CONST *PCCBXITEMW;

#ifdef UNICODE
#define CBXITEM            CBXITEMW
#define PCBXITEM           PCBXITEMW
#define PCCBXITEM          PCCBXITEMW
#else
#define CBXITEM            CBXITEMA
#define PCBXITEM           PCBXITEMA
#define PCCBXITEM          PCCBXITEMA
#endif

//  ADDCBXITEMCALLBACK fAction flags
#define CBXCB_ADDING       0x00000001     // if callback returns E_ABORT, combo population aborts
#define CBXCB_ADDED        0x00000002     // callback's return value is ignored.

//  SendMessageTimeout constants
#define CBX_SNDMSG_TIMEOUT_FLAGS          SMTO_BLOCK
#define CBX_SNDMSG_TIMEOUT                15000 // milliseconds
#define CBX_SNDMSG_TIMEOUT_HRESULT        HRESULT_FROM_WIN32(ERROR_TIMEOUT)

//  Misc constants
#define NO_ITEM_NOICON_INDENT -2 // -1 to make up for the icon indent.
#define NO_ITEM_INDENT       0
#define ITEM_INDENT          1

#define LISTINSERT_FIRST    0
#define LISTINSERT_LAST     -1

#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------------//
//  General shell comboboxex methods
typedef HRESULT (*LPFNPIDLENUM_CB)(LPITEMIDLIST, LPVOID);
typedef HRESULT (WINAPI * ADDCBXITEMCALLBACK)( ULONG fAction, PCBXITEM pItem, LPARAM lParam ) ;

HRESULT _AddCbxItemToComboBox( IN HWND hwndComboEx, IN PCCBXITEM pItem, IN INT_PTR *pnPosAdded ) ;
HRESULT _AddCbxItemToComboBoxCallback( IN HWND hwndComboEx, IN OUT PCBXITEM pItem, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam ) ;
HRESULT _MakeCbxItem( OUT PCBXITEM pcbi, IN  LPCTSTR pszDisplayName, IN  LPVOID pvData, IN  LPCITEMIDLIST pidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeCbxItemKnownImage( OUT PCBXITEM pcbi,IN  LPCTSTR pszDisplayName, IN  LPVOID pvData, IN  int iImage, IN  int iSelectedImage, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeCsidlIconCbxItem( OUT PCBXITEM pcbi,  LPCTSTR pszDisplayName,  LPVOID pvData, int nCsidlIcon, INT_PTR nPos, int iIndent ) ;
HRESULT _MakeCsidlItemStrCbxItem( OUT PCBXITEM pcbi, IN  int nCsidlItem, IN  int nCsidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeResourceAndCsidlStrCbxItem( OUT PCBXITEM pcbi,  IN  UINT idString, IN  int nCsidlItem, IN  int nCsidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeResourceCbxItem( OUT PCBXITEM pcbi, IN  int idString, IN  DWORD dwData, IN  int nCsidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeFileTypeCbxItem( OUT PCBXITEM pcbi, IN LPCTSTR pszDisplayName, IN  LPCTSTR pszExt, IN  LPCITEMIDLIST pidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakeCsidlCbxItem( OUT PCBXITEM pcbi, IN  int nCsidlItem, IN  int nCsidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _MakePidlCbxItem( OUT PCBXITEM pcbi, IN  LPITEMIDLIST pidl, IN  LPITEMIDLIST pidlIcon, IN  INT_PTR nPos, IN  int iIndent ) ;
HRESULT _EnumSpecialItemIDs(int csidl, DWORD dwSHCONTF, LPFNPIDLENUM_CB pfn, LPVOID pvData);
HRESULT _GetPidlIcon( IN LPCITEMIDLIST pidl, OUT int *piImage, OUT int *piSelectedImage ) ;
HRESULT _EnumSpecialItemIDs( IN int csidl, IN DWORD dwSHCONTF, IN LPFNPIDLENUM_CB pfn, IN LPVOID pvData ) ;
HIMAGELIST WINAPI _GetSystemImageListSmallIcons() ;


//----------------------------------------
//  Namespace picker combo methods.
HRESULT WINAPI _PopulateNamespaceCombo( IN HWND hwndComboEx, IN ADDCBXITEMCALLBACK pfn, IN LPARAM lParam) ;
LONG_PTR WINAPI _GetNamespaceComboItemText( IN HWND hwndComboEx, IN INT_PTR iItem, IN BOOL fPath, OUT LPTSTR pszText, IN int cchText) ;
LONG_PTR WINAPI _GetNamespaceComboSelItemText( IN HWND hwndComboEx, IN BOOL fPath, OUT LPTSTR pszText, IN int cchText) ;
LRESULT WINAPI _DeleteNamespaceComboItem( IN LPNMHDR pnmh ) ;

//  helpers (note: once all dependents are brought into line using the above methods, we can eliminate
//  decl of the following:
HRESULT        _BuildDrivesList( IN UINT uiFilter, IN LPCTSTR pszSeparator, IN LPCTSTR pszEnd, OUT LPTSTR pszString, IN DWORD cchSize ) ;
HRESULT        _MakeMyComputerCbxItem( OUT PCBXITEM pItem ) ;
HRESULT        _MakeLocalHardDrivesCbxItem( OUT PCBXITEM pItem ) ;
HRESULT        _MakeMappedDrivesCbxItem( OUT PCBXITEM pItem, IN LPITEMIDLIST pidl) ;
HRESULT        _MakeNethoodDirsCbxItem( OUT PCBXITEM pItem, IN LPITEMIDLIST pidl ) ;
HRESULT        _MakeBrowseForCbxItem( OUT PCBXITEM pItem ) ;
HRESULT        _MakeNetworkPlacesCbxItem( OUT PCBXITEM pItem, IN LPVOID lParam ) ;
HRESULT        _MakeRecentFolderCbxItem( OUT PCBXITEM pItem ) ;
typedef HRESULT (*LPFNRECENTENUM_CB)(IN LPCTSTR pszPath, IN BOOL fAddEntries, IN LPVOID pvParam);
HRESULT        _EnumRecentAndGeneratePath( IN BOOL fAddEntries, IN LPFNRECENTENUM_CB pfn, IN LPVOID pvParam ) ;

#define NAMESPACECOMBO_RECENT_PARAM    TEXT("$RECENT$")
#define CBX_CSIDL_LOCALDRIVES          0x04FF   // arbitrarily out of range of other CSIDL_xxx values.

//----------------------------------------
//  File Associations picker combo methods.
HRESULT WINAPI    _PopulateFileAssocCombo( IN HWND, IN ADDCBXITEMCALLBACK, IN LPARAM) ;
LONG WINAPI       _GetFileAssocComboSelItemText( IN HWND, OUT LPTSTR *ppszText) ;
LRESULT WINAPI    _DeleteFileAssocComboItem( IN LPNMHDR pnmh ) ;

//  helpers (note: once all dependents are brought into line using the above methods, we can eliminate
//  decl of the following:
HRESULT _AddFileType( IN HWND hwndComboBox, IN LPCTSTR pszDisplayName, IN LPCTSTR pszExt, IN LPCITEMIDLIST pidlIcon, IN int iIndent,
                      IN OPTIONAL ADDCBXITEMCALLBACK pfn, IN OPTIONAL LPARAM lParam ) ;
HRESULT _AddFileTypes( IN HWND hwndComboBox, IN OPTIONAL ADDCBXITEMCALLBACK pfn, IN OPTIONAL LPARAM lParam ) ;

// Hack Alert: impl in filetype.cpp
STDAPI_(DWORD) GetFileTypeAttributes( HKEY hkeyFT );

#define FILEASSOCIATIONSID_ALLFILETYPES          20
#define FILEASSOCIATIONSID_FILE_PATH             1   // Go parse it.
#define FILEASSOCIATIONSID_MAX                   FILEASSOCIATIONSID_ALLFILETYPES

//-------------------------------------------------------------------------//

#ifdef __cplusplus
}
#endif


#endif __SHCOMBOX_H__
