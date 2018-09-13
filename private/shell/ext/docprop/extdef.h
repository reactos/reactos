// Forwards...

typedef struct _ALLOBJS ALLOBJS, *LPALLOBJS;

////////////////////////////////////////////////////////////////////////////////
//
// FOfficeInitPropInfo
//
// Purpose:
//  Initializes PropetySheet page structures, etc.
//
// Notes:
//  Use this routine to add the Summary, Statistics, Custom and Contents
//  Property pages to a pre-allocted array of PROPSHEETPAGEs.
//
////////////////////////////////////////////////////////////////////////////////
void FOfficeInitPropInfo (PROPSHEETPAGE * lpPsp, DWORD dwFlags, LPARAM lParam, LPFNPSPCALLBACK pfnCallback);

////////////////////////////////////////////////////////////////////////////////
//
// Attach
//
// Purpose:
//  Assigns HPROPSHEETPAGE to appropriate data block member.
//
////////////////////////////////////////////////////////////////////////////////
BOOL FAttach( LPALLOBJS lpallobjs, PROPSHEETPAGE* ppsp, HPROPSHEETPAGE hPage );

////////////////////////////////////////////////////////////////////////////////
//
// FLoadTextStrings
//
// Purpose:
//  Initializes and load test strings for dll
//
// Notes:
//
////////////////////////////////////////////////////////////////////////////////
BOOL PASCAL FLoadTextStrings (void);

#include "offcapi.h"

////////////////////////////////////////////////////////////////////////////////
//
// Define the structure that is used to hold all instance data for the set of
// property sheet pages.
//
////////////////////////////////////////////////////////////////////////////////

// Max size for temp buffers & edit controls
#define BUFMAX          256 // Includes the NULL terminator.


// All the objects that dialogs need.
typedef struct _ALLOBJS
{
  LPSIOBJ         lpSIObj;
  LPDSIOBJ        lpDSIObj;
  LPUDOBJ         lpUDObj;
  WIN32_FIND_DATA filedata;
  BOOL            fFiledataInit;
  BOOL            fFindFileSuccess;    // Did it succeed
  DWQUERYLD       lpfnDwQueryLinkData;
  DWORD           dwMask;

  int             iMaxPageInit;         // what is the maximum page that was init?

  // Other stuff that needs to be per file...
  BOOL            fPropDlgChanged;          // Did the user make any changes?
  BOOL            fPropDlgPrompted;          // To make sure we don't prompt the user twice to apply changes

  // Global Buffers
  BOOL            fOleInit;
  UINT            uPageRef;
  TCHAR           szPath[MAX_PATH];

  // Variables used in Custom Dialog proc
  HWND            CDP_hWndBoolTrue;
  HWND            CDP_hWndBoolFalse;
  HWND            CDP_hWndGroup;
  HWND            CDP_hWndVal;
  HWND            CDP_hWndName;
  HWND            CDP_hWndLinkVal;
  HWND            CDP_hWndValText;
  HWND            CDP_hWndAdd;
  HWND            CDP_hWndDelete;
  HWND            CDP_hWndType;
  HWND            CDP_hWndCustomLV;
  DWORD           CDP_cLinks;                   // Link data
  TCHAR           CDP_sz[BUFMAX];               // Links the app supports
  int             CDP_iItem;                    // Index of currently selected item

  BOOL            CDP_fLink;                    // Link checkbox state
  BOOL            CDP_fAdd;                     // Add button state
  DWORD           CDP_iszType;                  // Index of currently selected type
  HIMAGELIST      CDP_hImlS;

} ALLOBJS, *LPALLOBJS;



