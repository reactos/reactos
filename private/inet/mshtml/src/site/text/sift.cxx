/*
 *  SIFT.C
 *
 *  Purpose:
 *      Provides top-level support for sift testing
 *
 *
 *  Author:
 *      alexgo  3/24/95
 *
 */

#include "headers.hxx"

#if DBG==1

BOOL SiftCloseClipboard(void)
{
    // TODO (alexgo): add sift support
    return CloseClipboard();
}

BOOL SiftEmptyClipboard(void)
{
    // TODO (alexgo): add sift support
    return EmptyClipboard();
}

BOOL SiftOpenClipboard( HWND hwnd )
{
    // TODO (alexgo): add sift support
    return OpenClipboard( hwnd );
}

HANDLE SiftGetClipboardData( UINT cf )
{
    // TODO (alexgo): add sift support
    return GetClipboardData( cf );
}

HANDLE SiftSetClipboardData( UINT cf, HANDLE hdata )
{
    // TODO (alexgo): add sift support
    return SetClipboardData(cf, hdata);
}

#endif


