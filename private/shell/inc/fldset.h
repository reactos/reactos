#ifndef FLDSET_H_
#define FLDSET_H_


//
// view priority values
//
// NOTE: left gaps so that we can add more later
//
#define VIEW_PRIORITY_RESTRICTED    0x00000070  // a shell restriction is in place that forces this view to be the one we use
#define VIEW_PRIORITY_CACHEHIT      0x00000050  // we have registry info for the view, so the user has been there before
#define VIEW_PRIORITY_STALECACHEHIT 0x00000045  // we have stale registry info for the view, so we fall back to the "all folders like this" default
#define VIEW_PRIORITY_SHELLEXT      0x00000040  // next we let the shell extension have its say
#define VIEW_PRIORITY_CACHEMISS     0x00000030  // if we have a cache miss, then we fall back to the "all folders like this" default
#define VIEW_PRIORITY_INHERIT       0x00000020  // then try to inherit the view from the previous window
#define VIEW_PRIORITY_DESPERATE     0x00000010  // just pick the last view that the window supports
#define VIEW_PRIORITY_NONE          0x00000000  // dont have a view yet

typedef struct CShellViews
{
    HDPA _dpaViews;
} CShellViews;

void CShellViews_Delete(CShellViews*);

typedef struct tagFolderSetData {
    FOLDERSETTINGS  _fs;
    SHELLVIEWID     _vidRestore;
    DWORD           _dwViewPriority; // one of the VIEW_PRIORITY_* from above
} FOLDERSETDATA, *LPFOLDERSETDATA;

typedef struct tagFolderSetDataBase {
    FOLDERSETDATA   _fld;
    CShellViews     _cViews;
    UINT            _iViewSet;
} FOLDERSETDATABASE, *LPFOLDERSETDATABASE;    
    

HRESULT FileCabinet_CreateViewWindow2(IShellBrowser* psb, FOLDERSETDATABASE* that, IShellView *psvNew,
    IShellView *psvOld, RECT *prcView, HWND *phWnd);
BOOL FileCabinet_GetDefaultViewID2(FOLDERSETDATABASE* that, SHELLVIEWID* pvid);
#endif
