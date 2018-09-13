/*
 * Dynamically link to DCI entry points.
 * To implement dynamic linking
 * 1. include this file in DRAWDIB.C
 * 2. include a call in DRAWDIB.C to InitialiseDCI()
 * 3. include a call in DRAWDIB.C to TerminateDCI()
 * 	at the appropriate points.
 * There are no other changes.
 * To revert to static linking you can simply modify this file or
 * #define STATIC_LINK_TO_DCI during the compilation phase.
 * AND link to DCIMAN32.LIB
 *
 * IF the code adds calls to other DCI entry points they will have to be
 * added to this file.  You should find that out easily enough as there will
 * be an unresolved reference on linking (assuming that DCIMAN32.LIB is not
 * included in the libraries list).
 *
 * The process is straightforward.
 * string variables are defined to hold the
 *   names of the DCI function entry points.
 * function variables are defined for each entry point being indirected
 * the code is added to GetProcAddress for each entry
 * a #define is added to point the DCI entry point name at the function variable
 */

#ifdef STATIC_LINK_TO_DCI

#define InitialiseDCI() 1
#define TerminateDCI()

#else

static const char DCILIBRARY[] = "DCIMAN32.DLL";
static HINSTANCE  hlibDCI;
static BOOL       fDCILinked;
static UINT	  cDCIUsers; // Count of active DCI users

char szDCIOpenProvider[]  =  "DCIOpenProvider";
char szDCICloseProvider[] =  "DCICloseProvider";
char szDCICreatePrimary[] =  "DCICreatePrimary";
char szDCIEndAccess[]     =  "DCIEndAccess";
char szDCIBeginAccess[]   =  "DCIBeginAccess";
char szDCIDestroy[]       =  "DCIDestroy";

HDC 	(WINAPI *pfnDCIOpenProvider)(void);
void 	(WINAPI *pfnDCICloseProvider)(HDC hdc);
int 	(WINAPI *pfnDCICreatePrimary)(HDC hdc, LPDCISURFACEINFO FAR *lplpSurface);
void 	(WINAPI *pfnDCIEndAccess)(LPDCISURFACEINFO pdci);
DCIRVAL (WINAPI *pfnDCIBeginAccess)(LPDCISURFACEINFO pdci, int x, int y, int dx, int dy);
void 	(WINAPI *pfnDCIDestroy)(LPDCISURFACEINFO pdci);

BOOL InitialiseDCI(void);
__inline BOOL InitialiseDCI()	
{
    ++cDCIUsers;
    if (fDCILinked) {
	// Already linked
	return(TRUE);
    }
    hlibDCI = LoadLibraryA(DCILIBRARY);						
    if (hlibDCI) {									
        (FARPROC)pfnDCIOpenProvider  = GetProcAddress(hlibDCI, szDCIOpenProvider);	
        (FARPROC)pfnDCICloseProvider = GetProcAddress(hlibDCI, szDCICloseProvider);	
        (FARPROC)pfnDCICreatePrimary = GetProcAddress(hlibDCI, szDCICreatePrimary);	
        (FARPROC)pfnDCIEndAccess     = GetProcAddress(hlibDCI, szDCIEndAccess);	
        (FARPROC)pfnDCIBeginAccess   = GetProcAddress(hlibDCI, szDCIBeginAccess);	
        (FARPROC)pfnDCIDestroy       = GetProcAddress(hlibDCI, szDCIDestroy);	
        if (pfnDCIOpenProvider && 							
            pfnDCICloseProvider &&							
            pfnDCICreatePrimary &&							
            pfnDCIEndAccess &&    							
            pfnDCIBeginAccess &&  							
            pfnDCIDestroy) {							
    	    fDCILinked = TRUE;							
        } else {									
	    --cDCIUsers;
	    FreeLibrary(hlibDCI);							
	    hlibDCI = NULL;								
        }										
    }										
    return fDCILinked;

}

#define TerminateDCI() \
	if (hlibDCI && !--cDCIUsers) {\
	    FreeLibrary(hlibDCI);     \
	    fDCILinked = FALSE;       \
	    hlibDCI = NULL;	      \
	}


// Map the static names to the function pointers.

#define DCIOpenProvider  pfnDCIOpenProvider
#define DCICloseProvider pfnDCICloseProvider
#define DCICreatePrimary pfnDCICreatePrimary
#define DCIEndAccess     pfnDCIEndAccess
#define DCIBeginAccess   pfnDCIBeginAccess
#define DCIDestroy       pfnDCIDestroy

#endif
