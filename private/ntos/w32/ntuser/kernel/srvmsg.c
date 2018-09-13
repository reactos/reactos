/****************************** Module Header ******************************\
* Module Name: srvmsg.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Includes the mapping table for messages when calling the client.
*
* 04-11-91 ScottLu      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define SfnDDEINIT               SfnDWORD
#define SfnKERNELONLY            SfnDWORD

#ifdef FE_SB
/*
 * SfnEMGETSEL, SfnSETSEL, SfnGBGETEDITSEL
 */
#define SfnEMGETSEL              SfnOPTOUTLPDWORDOPTOUTLPDWORD
#define SfnEMSETSEL              SfnDWORD
#define SfnCBGETEDITSEL          SfnOPTOUTLPDWORDOPTOUTLPDWORD
#endif // FE_SB

#define MSGFN(func) Sfn ## func
#define FNSCSENDMESSAGE SFNSCSENDMESSAGE
#include <messages.h>

/***************************************************************************\
* fnINLBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LRESULT SfnINLBOXSTRING(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam,
    PROC xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(HW(pwnd))) {
        return 0L;
    }
    dw = pwnd->style;

    if (!(dw & LBS_HASSTRINGS) &&
            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return SfnDWORD(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return SfnINSTRING(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;

        case LB_FINDSTRING:
            return SfnINSTRINGNULL(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;
    }
}


/***************************************************************************\
* SfnOUTLBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on LBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LRESULT SfnOUTLBOXSTRING(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam,
    PROC xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;
    BOOL bNotString;
    DWORD dwRet;
    TL tlpwnd;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(HW(pwnd))) {
        return 0L;
    }
    dw = pwnd->style;

    /*
     * See if the control is ownerdraw and does not have the LBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    bNotString =  (!(dw & LBS_HASSTRINGS) &&
            (dw & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)));

    /*
     * Make this special call which'll know how to copy this string.
     */
    ThreadLock(pwnd, &tlpwnd);
    dwRet = ClientGetListboxString(pwnd, msg, wParam,
            (PLARGE_UNICODE_STRING)lParam,
            xParam, xpfn, dwSCMSFlags, bNotString, psms);
    ThreadUnlock(&tlpwnd);
    return dwRet;
}


/***************************************************************************\
* fnINCBOXSTRING
*
* Takes a lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LRESULT SfnINCBOXSTRING(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam,
    PROC xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;

    /*
     * See if the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */
    if (!RevalidateHwnd(HW(pwnd))) {
        return 0L;
    }
    dw = pwnd->style;

    if (!(dw & CBS_HASSTRINGS) &&
            (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE))) {

        /*
         * Treat lParam as a dword.
         */
        return SfnDWORD(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
    }

    /*
     * Treat as a string pointer.   Some messages allowed or had certain
     * error codes for NULL so send them through the NULL allowed thunk.
     * Ventura Publisher does this
     */
    switch (msg) {
        default:
            return SfnINSTRING(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;

        case CB_FINDSTRING:
            return SfnINSTRINGNULL(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
            break;
    }
}


/***************************************************************************\
* fnOUTCBOXSTRING
*
* Returns an lbox string - a string that treats lParam as a string pointer or
* a DWORD depending on CBS_HASSTRINGS and ownerdraw.
*
* 04-12-91 ScottLu      Created.
\***************************************************************************/

LRESULT SfnOUTCBOXSTRING(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam,
    PROC xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    DWORD dw;
    BOOL bNotString;
    DWORD dwRet;
    TL tlpwnd;

    /*
     * See if the control is ownerdraw and does not have the CBS_HASSTRINGS
     * style.  If so, treat lParam as a DWORD.
     */

    if (!RevalidateHwnd(HW(pwnd))) {
        return 0L;
    }
    dw = pwnd->style;

    bNotString = (!(dw & CBS_HASSTRINGS) &&
            (dw & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)));

    /*
     * Make this special call which'll know how to copy this string.
     */
    ThreadLock(pwnd, &tlpwnd);
    dwRet = ClientGetListboxString(pwnd, msg, wParam,
            (PLARGE_UNICODE_STRING)lParam,
            xParam, xpfn, dwSCMSFlags, bNotString, psms);
    ThreadUnlock(&tlpwnd);
    return dwRet;
}


/***************************************************************************\
* fnPOWERBROADCAST
*
* Make sure we send the correct message when we resume.
*
* History:
* 02-Dec-1996 JerrySh   Created.
\***************************************************************************/

LRESULT SfnPOWERBROADCAST(
    PWND pwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam,
    ULONG_PTR xParam,
    PROC xpfn,
    DWORD dwSCMSFlags,
    PSMS psms)
{
    switch (wParam) {
    case PBT_APMQUERYSUSPEND:
        SetWF(pwnd, WFGOTQUERYSUSPENDMSG);
        break;
    case PBT_APMQUERYSUSPENDFAILED:
        if (!TestWF(pwnd, WFGOTQUERYSUSPENDMSG))
            return 0;
        ClrWF(pwnd, WFGOTQUERYSUSPENDMSG);
        break;
    case PBT_APMSUSPEND:
        ClrWF(pwnd, WFGOTQUERYSUSPENDMSG);
        SetWF(pwnd, WFGOTSUSPENDMSG);
        break;
    case PBT_APMRESUMESUSPEND:
        if (TestWF(pwnd, WFGOTSUSPENDMSG)) {
            break;
        }
        wParam = PBT_APMRESUMECRITICAL;
        // FALL THRU
    case PBT_APMRESUMECRITICAL:
        ClrWF(pwnd, WFGOTQUERYSUSPENDMSG);
        ClrWF(pwnd, WFGOTSUSPENDMSG);
        break;
    }

    return SfnDWORD(pwnd, msg, wParam, lParam, xParam, xpfn, dwSCMSFlags, psms);
}


