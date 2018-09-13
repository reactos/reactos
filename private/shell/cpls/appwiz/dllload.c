#include "priv.h"

// Some macros below use shdocvw's WhichPlatform.
#include <shellp.h>     // shdocvw.h should really include this
#include <shdocvw.h>


#ifdef DEBUG
#define DF_DELAYLOADDLL     0x10000000
#endif

#include "shguidp.h"

#include "uemapp.h"

#define NO_LOADING_OF_SHDOCVW_ONLY_FOR_WHICHPLATFORM
#include "..\lib\dllload.c"


/**********************************************************************/
/**********************************************************************/


// --------- SYSSETUP.DLL ---------------

HINSTANCE g_hinstSYSSETUP = NULL;


DELAY_LOAD_BOOL(g_hinstSYSSETUP, SYSSETUP, SetupCreateOptionalComponentsPage,
    (LPFNADDPROPSHEETPAGE pfnAdd, LPARAM lParam), (pfnAdd, lParam));

#pragma warning(default:4229)
