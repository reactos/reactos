///////////////////////////////////////////////////////////////////////////////
//
// SCRINST.C
//
// Provides a RUNDLL32-callable routine to install a screen saver
//
///////////////////////////////////////////////////////////////////////////////

#include "desk.h"
#include "rc.h"


void WINAPI
InstallScreenSaver( HWND wnd, HINSTANCE inst, LPSTR cmd, int shw )
{
    char buf[ PATHMAX ];
    int timeout;

    lstrcpyn( buf, cmd, sizeof(buf) );
    PathGetShortPath( buf ); // so msscenes doesn't die
    WritePrivateProfileString( "boot", "SCRNSAVE.EXE", buf, "system.ini" );

    SystemParametersInfo( SPI_SETSCREENSAVEACTIVE, TRUE, NULL,
        SPIF_UPDATEINIFILE );

    // make sure the user has a non-stupid timeout set
    SystemParametersInfo( SPI_GETSCREENSAVETIMEOUT, 0, &timeout, 0 );
    if( timeout <= 0 )
    {
        // 15 minutes seems like a nice default
        SystemParametersInfo( SPI_SETSCREENSAVETIMEOUT, 900, NULL,
            SPIF_UPDATEINIFILE );
    }

    // bring up the screen saver page on our rundll
    Control_RunDLL( wnd, inst, "DESK.CPL,,1", shw );
}
