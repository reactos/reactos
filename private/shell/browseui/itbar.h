#ifndef _ITBAR_H
#define _ITBAR_H

// initial layout information
typedef struct tagBANDSAVE
{
    UINT wID;
    UINT fStyle;
    UINT cx;
} BANDSAVE, *PBANDSAVE;

// CItnernet Toolbar - Private commands 
#define CITIDM_ONINTERNET    1  // nCmdexecopt ? Web : Shell
#define CITE_INTERNET       0
#define CITE_SHELL          1
#define CITE_QUERY          2

#define CITIDM_THEATER       2  // nCmdexecopt..
#define CITIDM_TEXTLABELS    3  // Toggle Text Labels
// the modes for theater mode
#define THF_ON  0            
#define THF_OFF 1
#define THF_UNHIDE 2 
#define THF_HIDE  3
                   
#define CITIDM_VIEWTOOLS     4      // This toggles on/off
#define CITIDM_VIEWADDRESS   5      // This toggles on/off
#define CITIDM_VIEWLINKS     6      // This toggles on/off
#define CITIDM_SHOWTOOLS     7      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_SHOWADDRESS   8      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_SHOWLINKS     9      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_EDITPAGE      10
#define CITIDM_BRANDSIZE     11     // brand at minimum always or not
#define CITIDM_VIEWMENU      12      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_VIEWAUTOHIDE  13      // nCmdExecOpt: TRUE or FALSE
#define CITIDM_GETMINROWHEIGHT 14    // gets the minimum height of row 0... for branding
#define CITIDM_SHOWMENU      15
#define CITIDM_STATUSCHANGED 16
#define CITIDM_GETDEFAULTBRANDCOLOR 17
#define CITIDM_DISABLESHOWMENU      18
#define CITIDM_SET_DIRTYBIT         19  // nCmdexecopt equals TRUE or FALSE which will overwrite _fDirty.
#define CITIDM_VIEWTOOLBARCUSTOMIZE       20
#define CITIDM_VIEWEXTERNALBAND_BYCLASSID 21
#define CITIDM_DISABLEVISIBILITYSAVE 22 // bands can choose not to persist their visibility state
#ifdef UNIX
#define CITIDM_SHOWBRAND     25     //nCmdExecOpt:  TRUE or FALSE 
#endif 
#define CITIDM_GETFOLDERSEARCHES        26

// Indicies for Coolbar bands
// These indexes are 1 based since band array is memset to 0 and ShowDW would 
// think that an unused item would belong to IDX0.

// IMPORTANT: don't change the value of anything between CBIDX_FIRST and CBIDX_LAST.
// CInternetToolbar::_LoadUpgradeSettings assumes these values haven't changed from
// version to version.
#define CBIDX_MENU              1
#define CBIDX_TOOLS             2
#define CBIDX_LINKS             3
#define CBIDX_ADDRESS           4
#define CBIDX_BRAND             5
#define CBIDX_FIRST             CBIDX_MENU
#define CBIDX_LAST              CBIDX_BRAND

#define MAXEXTERNALBANDS        16
#define CBIDX_EXTERNALFIRST     (CBIDX_LAST + 1)
#define CBIDX_EXTERNALLAST      (CBIDX_EXTERNALFIRST + MAXEXTERNALBANDS - 1)

#define CBANDSMAX               (CBIDX_LAST + MAXEXTERNALBANDS)

#define CITIDM_VIEWEXTERNALBAND_FIRST 30
#define CITIDM_VIEWEXTERNALBAND_LAST (CITIDM_VIEWEXTERNALBAND_FIRST + MAXEXTERNALBANDS - 1)

#define BandIDtoIndex(hwnd, idx) SendMessage(hwnd, RB_IDTOINDEX, idx, 0)


// Indices for Toolbar imagelists
#define IMLIST_DEFAULT          0
#define IMLIST_HOT              1

#define ITBS_SHELL    0
#define ITBS_WEB      1
#define ITBS_EXPLORER 2
IStream *GetITBarStream(BOOL fWebBrowser, DWORD grfMode);

// number of bitmaps in the IDB_IETOOLBAR strips
#define MAX_TB_BUTTONS          16

#define SHELLGLYPHS_OFFSET      MAX_TB_BUTTONS

#endif /* _ITBAR_H */
