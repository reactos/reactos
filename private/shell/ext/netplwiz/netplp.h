/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    netplwiz.h

    This file contains the network places wizard private header info

    FILE HISTORY:
        Johnl   09-Jan-1992     Commented
        CongpaY Nov-4-1992      Add more defines.
        DSheldon Apr-16-1998    Total overhaul; modified for net places
                                wizard

*/

#ifndef NETPLP_H_INCLUDED
#define NETPLP_H_INCLUDED

#include "resource.h"


//
// Globals
//
extern HINSTANCE g_hInstance;
#define GLOBAL_HINSTANCE (g_hInstance)

extern const TCHAR c_szShell32Dll[];

//
// Wizard text-related constants
//
#define MAX_CAPTION         256     // Maximum size of a caption in the Wizard
#define MAX_STATIC          512     // Maximum size of static text in the Wizard

// Wizard error return value
#define RETCODE_CANCEL      0xffffffff

// Automatic wait cursor class
class CWaitCursor {
  public:
    CWaitCursor()  { hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT)); }
    ~CWaitCursor() { SetCursor(hCursor); }

  private:
    HCURSOR     hCursor;    // Storage space for the old cursor
};

#endif //NETPLP_H_INCLUDED
