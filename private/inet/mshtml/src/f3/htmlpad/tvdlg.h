////////////////////////////////////////////////////////////////////////////
//
//      TVDLG.H
//
//  Copyright 1986-1996 Microsoft Corporation. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////

#ifndef __TVDLG__H__
#define __TVDLG__H__


enum { iEID, iDispName, iSubfldrs, nhtProps};
static SizedSPropTagArray(nhtProps, spthtProps) =
{ nhtProps,
    {   PR_ENTRYID,
        PR_DISPLAY_NAME_A,
        PR_SUBFOLDERS
    }
};  

#define Align4(_cb) (((_cb) + 3) & ~3)

extern LPTSTR g_szNoFolderName;
extern LPSTR g_szAllStoresA;

class CTVNodeFactory;
class CChsFldDlg;


/////////////////////////////////////////////////////////////////////////
// CTVNode

class CTVNode;
typedef CTVNode * LPTVNODE;

class CTVNode
{

friend CTVNodeFactory;
friend HTREEITEM HtiFindChild(HWND hwTreeCtl, HTREEITEM hti, ULONG cb,
                LPENTRYID pbEID, CChsFldDlg *pCFDlg, LPTVNODE *ppNode);


public:
    LPTSTR GetName(void);
    HRESULT HrExpand(CChsFldDlg * pCFDlg);
    HRESULT HrGetFolder(CChsFldDlg * pCFDlg, LPMAPIFOLDER * ppfld,
                            LPMDB *ppmdb);

    void SetHandle(HTREEITEM hItem) { _htiMe = hItem;}
    void SetKidsLoaded(BOOL fLoaded) { _fKidsLoaded = fLoaded;}

    HRESULT HrNewFolder(CChsFldDlg * pCFDlg, LPTSTR szFldName);

    static LPVOID operator new( size_t cb );
    static VOID   operator delete( LPVOID pv );

    void Write(BOOL fWrite, LONG iLevel, LPBYTE * ppb);
    
    ~CTVNode();

private:
    
    //can only be created in CTVNodeFactory::HrCreateNode
    CTVNode(LPSPropValue pvals, ULONG cprops, LPMDB pmdb);

    HRESULT HrOpenMDB(CChsFldDlg * pCFDlg);
    HRESULT HrOpenFolder(CChsFldDlg * pCFDlg);
    
    HTREEITEM       _htiMe;
    LPSPropValue    _pval;
    BOOL            _fKidsLoaded;
    LPMAPIFOLDER    _pfld;
    CTVNode         *_pNext;
    LPMDB           _pmdb;
};


////////////////////////////////////////////////////////////////////////
// CTVNodeFactory

class CTVNodeFactory
{
public:
    HRESULT HrCreateNode(LPSPropValue pval, ULONG cVals, LPMDB pmdb,
                            LPTVNODE * pptvnode);

    CTVNodeFactory();
    ~CTVNodeFactory();

private:
    void Insert(CTVNode * ptvnode);

    LPTVNODE    _pHead;
};


////////////////////////////////////////////////////////////////////////
// CChsFldDlg

class CChsFldDlg
{
public:
    HRESULT HrPick(LPCTSTR lpTemplateName, HWND hWnd,
                DLGPROC pfnDlgProc, LPMAPIFOLDER * ppfld, LPMDB *ppmdb);
    HRESULT HrInitTree(HWND hDlg, HWND hwTreeCtl);
    HRESULT HrLoadRoots(void);
    HRESULT HrInsertRoot(LPSPropValue pval);
    BOOL    IsTreeRoot(HTREEITEM hti) { return (hti == _hiRoot); }

    int     IndAllStores(void)  { return _iIconAllStores; }
    int     IndRootFld(void)    { return _iIconRootFld; }
    int     IndOpenFld(void)    { return _iIconOpenFld; }
    int     IndClsdFld(void)    { return _iIconClsdFld; }
    HWND    hwDialog(void)      { return _hDlg; }
    HWND    hwTreeCtl(void)     { return _hwTreeCtl; }

    LPMAPISESSION   Session(void) { return _pses; }

    
    HINSTANCE hInst(void)   { return _hInst; }

    HRESULT HrCreateNode(LPSPropValue pval, ULONG cvals, LPMDB pmdb,
                            LPTVNODE * ppNode)
        { return _NodeFactory.HrCreateNode(pval, cvals, pmdb, ppNode);}

    void    SetFolder(LPMAPIFOLDER plfd, LPMDB pmdb);
    void    SetError(HRESULT hr)    { _hr = hr;}

    HRESULT HrSaveTreeState(void);
    HRESULT HrRestoreTreeState(void);

    CChsFldDlg(LPMAPISESSION pses, HINSTANCE hInst, ULONG * pcb, LPBYTE * ppb);
    ~CChsFldDlg();

private:

    HRESULT HrSaveTreeStateEx(BOOL fWrite, ULONG * pcb, LPBYTE * ppb);
    HRESULT HrRestoreTreeState(ULONG cb, LPBYTE pb);
    
    LPMAPISESSION   _pses;
    LPMAPIFOLDER    _pfld;
    LPMDB           _pmdb;
    HRESULT         _hr;
    HTREEITEM       _hiRoot;
    HINSTANCE       _hInst;
    HWND            _hDlg;
    HWND            _hwTreeCtl;

    HIMAGELIST      _hIml;
    int             _iIconAllStores;
    int             _iIconRootFld;
    int             _iIconOpenFld;
    int             _iIconClsdFld;
    
    ULONG           *_pcbState;
    LPBYTE          *_ppbState;
    
    CTVNodeFactory  _NodeFactory;
    
};


HTREEITEM AddOneItem( HTREEITEM hParent, HTREEITEM hInsAfter, 
    int iImage, int iImageSel, HWND hwndTree, LPTVNODE pNode, int cKids);


#endif //__TVDLG__H__
