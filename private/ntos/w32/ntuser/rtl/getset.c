/****************************** Module Header ******************************\
* Module Name: getset.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains window manager information routines
*
* History:
* 10-22-90 MikeHar      Ported functions from Win 3.0 sources.
* 13-Feb-1991 mikeke    Added Revalidation code (None)
* 08-Feb-1991 IanJa     Unicode/ANSI aware and neutral
\***************************************************************************/

/***************************************************************************\
* MapServerToClientPfn
*
* Returns the client wndproc representing the server wndproc passed in
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

ULONG_PTR MapServerToClientPfn(
    KERNEL_ULONG_PTR dw,
    BOOL bAnsi)
{
    int i;

    for (i = FNID_WNDPROCSTART; i <= FNID_WNDPROCEND; i++) {
        if ((WNDPROC_PWND)dw == STOCID(i)) {
            if (bAnsi) {
                return FNID_TO_CLIENT_PFNA_CLIENT(i);
            } else {
                return FNID_TO_CLIENT_PFNW_CLIENT(i);
            }
        }
    }
    return 0;
}

/***************************************************************************\
* MapClientNeuterToClientPfn
*
* Maps client Neuter routines like editwndproc to Ansi or Unicode versions
* and back again.
*
* 01-13-92 ScottLu      Created.
\***************************************************************************/

ULONG_PTR MapClientNeuterToClientPfn(
    PCLS pcls,
    KERNEL_ULONG_PTR dw,
    BOOL bAnsi)
{
    /*
     * Default to the class window proc.
     */
    if (dw == 0) {
        dw = (KERNEL_ULONG_PTR)pcls->lpfnWndProc;
    }

    /*
     * If this is one of our controls and it hasn't been subclassed, try
     * to return the correct ANSI/Unicode function.
     */
    if (pcls->fnid >= FNID_CONTROLSTART && pcls->fnid <= FNID_CONTROLEND) {
        if (!bAnsi) {
            if (FNID_TO_CLIENT_PFNA_KERNEL(pcls->fnid) == dw)
                return FNID_TO_CLIENT_PFNW_CLIENT(pcls->fnid);
        } else {
            if (FNID_TO_CLIENT_PFNW_KERNEL(pcls->fnid) == dw)
                return FNID_TO_CLIENT_PFNA_CLIENT(pcls->fnid);
        }
#ifdef BUILD_WOW6432
        if (!bAnsi) {
            if (FNID_TO_CLIENT_PFNW_KERNEL(pcls->fnid) == dw)
                return FNID_TO_CLIENT_PFNW_CLIENT(pcls->fnid);
        } else {
            if (FNID_TO_CLIENT_PFNA_KERNEL(pcls->fnid) == dw)
                return FNID_TO_CLIENT_PFNA_CLIENT(pcls->fnid);
        }
#endif
    }

    return (ULONG_PTR)dw;
}
