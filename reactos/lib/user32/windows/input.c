
#include <windows.h>
#include <user32/debug.h>

static INT  captureHT = HTCLIENT;
static HWND captureWnd = 0;

WINBOOL MouseButtonsStates[3];
WINBOOL AsyncMouseButtonsStates[3];
BYTE InputKeyStateTable[256];
BYTE QueueKeyStateTable[256];
BYTE AsyncKeyStateTable[256];

HWND EVENT_Capture(HWND hwnd, INT ht);

HWND STDCALL SetCapture( HWND hwnd )
{
    return EVENT_Capture( hwnd, HTCLIENT );
}


WINBOOL STDCALL ReleaseCapture(void)
{
    if( captureWnd ) EVENT_Capture( 0, 0 );
}


HWND STDCALL GetCapture(void)
{
    return captureWnd;
}

/**********************************************************************
 *              EVENT_Capture
 *
 * We need this to be able to generate double click messages
 * when menu code captures mouse in the window without CS_DBLCLK style.
 */
HWND EVENT_Capture(HWND hwnd, INT ht)
{
    HWND capturePrev = captureWnd;

    if (!hwnd)
    {
        captureWnd = 0L;
        captureHT = 0;
    }
    else
    {
        if( IsWindow(hwnd) )
        {
            DPRINT("(0x%04x)\n", hwnd );
            captureWnd   = hwnd;
            captureHT    = ht;
        }
    }

    if( capturePrev && capturePrev != captureWnd )
    {
        if( IsWindow(capturePrev) )
            SendMessageA( capturePrev, WM_CAPTURECHANGED, 0L, hwnd);
    }
    return capturePrev;
}