//
// info.h: Declares data, defines and struct types for twin creation
//          module.
//
//

#ifndef __INFO_H__
#define __INFO_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

/////////////////////////////////////////////////////  TYPEDEFS

typedef struct
    {
    LPBRIEFCASESTG      pbrfstg;        // IBriefcaseStg instance
    // Params
    //
    CBS  * pcbs;
    int atomPath;
    HDPA   hdpaTwins;    // handle to array of twin handles which will
                         //  be filled by dialog.
                         //  N.b.  Caller must release these twins!
    
    BOOL bStandAlone;    // private: should only be set by Info_DoModal
    } XINFOSTRUCT,  * LPXINFOSTRUCT;


/////////////////////////////////////////////////////  EXPORTED DATA

/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

BOOL _export CALLBACK Info_WrapperProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int PUBLIC Info_DoModal (HWND hwndParent, LPXINFOSTRUCT lpxinfo);

#endif // __INFO_H__

