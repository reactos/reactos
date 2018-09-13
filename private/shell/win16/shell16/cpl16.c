#include "shprv.h"
#include <cpl.h>


// 16 bit half of thunk.  other half is in shell32.dll
// used by copy

HMODULE FAR PASCAL GetModuleHandle16(LPCSTR szName)
{
    return(GetModuleHandle(szName));
}


// 16 bit half of thunk.  other half is in shell32.dll
// used by control panel folder

int FAR PASCAL GetModuleFileName16(HINSTANCE hinst, LPSTR szFileName,
				         int cbMax)
{
    return GetModuleFileName(hinst, szFileName, cbMax);
}

typedef LONG	INT32;
typedef DWORD	HICON32;

typedef struct tagCPLINFO32
{
    INT32   idIcon;     /* icon resource id, provided by CPlApplet() */
    INT32   idName;     /* name string res. id, provided by CPlApplet() */
    INT32   idInfo;     /* info string res. id, provided by CPlApplet() */
    LONG    lData;      /* user defined data */
} CPLINFO32, *PCPLINFO32, FAR *LPCPLINFO32;

typedef struct tagNEWCPLINFO32
{
    DWORD       dwSize;         /* similar to the commdlg */
    DWORD	dwFlags;
    DWORD       dwHelpContext;  /* help context to use */
    LONG        lData;          /* user defined data */
    HICON32     hIcon;          /* icon to use, this is owned by CONTROL.EXE (may be deleted) */
    char        szName[32];     /* short name */
    char        szInfo[64];     /* long name (status line) */
    char        szHelpFile[128];/* path to help file to use */
} NEWCPLINFO32, *PNEWCPLINFO32, FAR *LPNEWCPLINFO32;


// HACK: this thunk gets used for OEM APM extensions because they
// look just like CPlApplet calls, but they have extra messages
// (which conveniently use a NEWCPLINFO to communicate...)
// the messages are presently not defined in any public header
// our caller sets the high bit to tell us it's an APM message
// the second is message number 100
//
#define APM_HACK    (0x8000)


// 16 bit half of thunk.  other half is in shell32.dll
// used by control panel folder

LRESULT FAR PASCAL
CallCPLEntry16( HINSTANCE hinst, APPLET_PROC lpfn, HWND hwndCPL, UINT msg,
			   LPARAM lParam1, LPARAM lParam2 )
{
    // strip APM_HACK from APM messages when calling real entry point
    LRESULT lret = lpfn( hwndCPL, ( msg & ~APM_HACK ), lParam1, lParam2 );

    // turn all APM messages into APM_HACK for the rest of the function
    if( msg & APM_HACK )
        msg = APM_HACK;

    switch( msg )
    {
        case CPL_INQUIRE:
	{
	    union
            {
		LPARAM      lParam;
		LPCPLINFO   lpcpl;
		LPCPLINFO32 lpcpl32;
	    } lp;

	    lp.lParam = lParam2;

	    // MUST copy from bottom to top
	    lp.lpcpl32->lData = lp.lpcpl->lData;
	    lp.lpcpl32->idInfo = (INT32)lp.lpcpl->idInfo;
	    lp.lpcpl32->idName = (INT32)lp.lpcpl->idName;
	    lp.lpcpl32->idIcon = (INT32)lp.lpcpl->idIcon;
	}
	break;

        case APM_HACK:
        case CPL_NEWINQUIRE:
	{
	    union
            {
		LPARAM          lParam;
		LPNEWCPLINFO    lpcpl;
		LPNEWCPLINFO32  lpcpl32;
	    } lp;

	    lp.lParam = lParam2;

            // fall back to CPL_INQUIRE if this was an unhandled CPL_NEWINQUIRE
	    if( ( msg == CPL_NEWINQUIRE ) &&
		( lp.lpcpl->dwSize != sizeof( NEWCPLINFO ) ) )
	    {
		CPLINFO CPLInfo = { 0 };

		lpfn( NULL, CPL_INQUIRE, lParam1, (LPARAM)(LPCPLINFO)&CPLInfo );

                if( CPLInfo.idIcon )
                {
                    lp.lpcpl->hIcon = LoadIcon( hinst,
                        MAKEINTRESOURCE( CPLInfo.idIcon ) );
                }
                else
                    lp.lpcpl->hIcon = NULL;

                if( CPLInfo.idName )
                {
                    LoadString( hinst, CPLInfo.idName,
		        lp.lpcpl->szName, sizeof( lp.lpcpl->szName ) );
                }
                else
                    *lp.lpcpl->szName = 0;

                if( CPLInfo.idInfo )
                {
                    LoadString( hinst, CPLInfo.idInfo,
                        lp.lpcpl->szInfo, sizeof( lp.lpcpl->szInfo ) );
                }
                else
                    *lp.lpcpl->szInfo = 0;

		lp.lpcpl->lData = CPLInfo.lData;
	    }

	    // MUST copy from bottom to top.
	    // note: dwFlags, dwHelpContext and lData in same place 16 or 32
	    hmemcpy(lp.lpcpl32->szName, lp.lpcpl->szName,
		    sizeof(lp.lpcpl->szName)+sizeof(lp.lpcpl->szInfo)+sizeof(lp.lpcpl->szHelpFile));
	    lp.lpcpl32->hIcon = (HICON32)lp.lpcpl->hIcon;
	    lp.lpcpl32->dwSize = sizeof( NEWCPLINFO32 ); // different from 16
            break;
	}
    }

    return lret;
}

void RunDll_CallEntry16(RUNDLLPROC pfn, HWND hwndStub, HINSTANCE hinst, LPSTR pszParam, int nCmdShow)
{
    pfn(hwndStub, hinst, pszParam, nCmdShow);
}

