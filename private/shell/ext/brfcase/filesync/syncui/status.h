//
// status.h: Declares data, defines and struct types for twin property
//                                module.
//
//

#ifndef __STATUS_H__
#define __STATUS_H__


/////////////////////////////////////////////////////  INCLUDES

/////////////////////////////////////////////////////  DEFINES

/////////////////////////////////////////////////////  MACROS

/////////////////////////////////////////////////////  TYPEDEFS

typedef struct tagXSTAT
    {
    LPBRIEFCASESTG      pbrfstg;        // IBriefcaseStg instance

    CBS  * pcbs;
    int atomPath;

    } XSTATSTRUCT,  * LPXSTATSTRUCT;


/////////////////////////////////////////////////////  EXPORTED DATA

/////////////////////////////////////////////////////  PUBLIC PROTOTYPES

BOOL _export CALLBACK Stat_WrapperProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


#endif // __STATUS_H__

