/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Osdebug.c

Abstract:

    OSDebug version 4 API

Author:

    Kent D. Forschmiedt (kentf)

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <tchar.h>

#include "cvinfo.h"

#include "odtypes.h"
#include "od.h"
#include "dbgver.h"

#include "odp.h"
#include "odassert.h"



#define WINDBG_POINTERS_MACROS_ONLY
#include "sundown.h"
#undef WINDBG_POINTERS_MACROS_ONLY




#define CEXM_MDL_native 0x20


//
// debugger services vector
//

static LPDBF lpdbf;

//
// List roots
//

// Processes
static HLLI llpid;

// EMs
static HLLI llem;

// TLs
static HLLI lltl;


#ifdef __cplusplus
extern "C" {
#endif

extern AVS Avs;

XOSD
OSDDoCallBack (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 lParam1,
    DWORD64 lParam2
    );

XOSD
OSDDoCallBackToEM (
    DBC emf,
    HPID hpid,
    HTID htid,
    DWORD64 lParam1,
    DWORD64 lParam2
    );

XOSD
CallEM (
    EMF emf,
    HPID hpid,
    HTID htid,
    DWORD64 lParam,
    DWORD64 lpv
    );

XOSD
CallTL (
    TLF tlf,
    HPID hpid,
    DWORD64 lParam,
    DWORD64 lpv
    );

XOSD
EMCallBackDB (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 cb,
    DWORD64 lpv
    );

XOSD
EMCallBackTL (
    TLF tlf,
    HPID hpid,
    DWORD64 cb,
    DWORD64 lpv
    );

XOSD
EMCallBackNT (
    EMF emf,
    HPID hpid,
    HTID htid,
    DWORD64 cb,
    DWORD64 lpv
    );

XOSD
EMCallBackEM (
    EMF emf,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 cb,
    DWORD64 lpv
    );

XOSD
TLCallBack (
    HPID hpid,
    DWORD64 lParam,
    DWORD64 lpv
    );

XOSD
CreateProc (
    LPFNSVC lpfnsvc,
    HEM hem,
    HTL htl,
    LPHPID lphpid
    );

XOSD
CreateThd (
    HPID hpid,
    HTID htid,
    LPHTID lphtid
    );

static EMCB emcb = {
    EMCallBackDB,
    EMCallBackTL,
    EMCallBackNT,
    EMCallBackEM
};

static CRITICAL_SECTION CallbackCriticalSection;


#define CheckErr(xosd) if ( xosd != xosdNone ) return xosd




/************************************************************************/

/* OSDebug Controller Initialization / Termination                      */

/************************************************************************/


XOSD
OSDAPI
OSDInit (
    LPDBF lpdbfT
    )
/*++

Routine Description:

    Initialize the internal structures for osdebug.
    Register the debugger services callback vector.

    Use list manager to create the three global lists used by osdebug.
    These are the list of processes ( llpid ), the list of transport
    layers ( lltl ), and the list of execution models ( llem ).

    Registering the debugger services with osdebug is just an
    assignment of a pointer to the function structure.

    If this function fails, it is catastrophic.  No cleanup of
    partially allocated data is attempted.

Arguments:

    lpdbfT - Supplies the debugger services structure

Return Value:

    xosdNone        - Function succeeded

    xosdOutOfMemory - List manager was unable to allocate room
                      for its root structures.

--*/
{
    XOSD xosd = xosdNone;

    assert ( lpdbfT != NULL );

    InitializeCriticalSection( &CallbackCriticalSection );

    lpdbf = lpdbfT;

    llpid = LLInit (sizeof ( PROCESSINFO ), llfNull, ODPDKill, (LPFNFCMPNODE) NULL );
    lltl  = LLInit ( sizeof ( TLS ), llfNull, TLKill, (LPFNFCMPNODE) NULL );
    llem  = LLInit ( sizeof ( EMS ), llfNull, EMKill, (LPFNFCMPNODE) NULL );

    if ( llpid == 0 || lltl == 0 || llem == 0 ) {
        xosd = xosdOutOfMemory;
    }

    return xosd;
}



XOSD
OSDAPI
OSDTerm(
    VOID
    )
/*++

Routine Description:

    Deallocate resources used by OSD.  At present, this only destroys
    critical sections used by osdebug.

Arguments:

    None

Return Value:

    xosd - always xosdNone

--*/
{
    DeleteCriticalSection( &CallbackCriticalSection );
    return xosdNone;
}


/************************************************************************/

/*    Execution Model loading, unloading and configuration              */

/************************************************************************/


XOSD
OSDAPI
OSDAddEM (
    EMFUNC emfunc,
    LPDBF lpdbf,
    LPHEM lphem,
    EMTYPE emtype
    )
/*++

Routine Description:

    Create and initialize an execution model associated with the
    service function EMFunc and add it to the list of osdebug's
    available execution models ( llem ).

    Use list manager to create an execution model handle and add
    it to the execution model list ( llem ).

    Call the execution model service function ( lpfnsvcEM ) to
    initialize the execution model and to register the debugger
    service functions ( lpdbf ).

Arguments:

    emfunc - Supplies a pointer to the execution model service function
             to be associated with the execution model that is being
             created.

    lpdbf -  Supplies a pointer to the debugger services structure that
             will be registered with the execution model being created.

    lphem -  Returns the execution model handle.

    emtype - Supplies the type of EM; emNative or emNonNative.

Return Value:

    xosdNone - Success.

    xosdOutOfMemory - List manager was unable to allocate em or
                      add it to the execution model list ( llem ).

    Other xosd failure codes may be returned from the new EM while
    it is being initialized.

--*/
{
    HEM  hem;
    HEM hemm = 0;
    LPEMS lpem;
    LPEMS lpemm;
    XOSD xosd = xosdNone;
    DWORD dwModel;

    assert ( emfunc != NULL );
    assert ( lpdbf != NULL );
    assert ( lphem != NULL );

    hem = (HEM)LLCreate ( llem );
    if ( !hem ) {
        return xosdOutOfMemory;
    }

    if ( emtype == emNative ) {

        // native inserted at the tail of the list
        LLAdd ( llem, (HLLE)hem );
    }
    else {

        // non-native inserted at head of list
        LLAddHead ( llem, (HLLE)hem );
    }

    lpem = (LPEMS) LLLock ( (HLLE)hem );
    lpem->emfunc = (EMFUNC_ODP) emfunc;
    lpem->emtype = emtype;
    lpem->llhpid = LLInit ( sizeof ( HPID ), llfNull, NULL, EMHpidCmp );
    if ( lpem->llhpid == 0 ) {
        xosd = xosdOutOfMemory;
    }

    xosd = (*lpem->emfunc) ( emfRegisterDBF, NULL, NULL, 0, (DWORD64) lpdbf );
    CheckErr ( xosd );

    xosd = (*lpem->emfunc) ( emfInit, NULL, NULL, 0, (DWORD64)&emcb );
    CheckErr ( xosd );

    xosd = (*lpem->emfunc) ( emfGetModel, NULL, NULL, 0, (DWORD64)&dwModel );
    CheckErr ( xosd );

    while ( hemm = (HEM)LLNext ( llem, (HLLE)hemm ) ) {
        lpemm = (LPEMS) LLLock ( (HLLE)hemm );
        if ( lpemm->model == dwModel ) {

            // this is an error, cannot add the same model twice
            LLUnlock ( (HLLE)hemm );
            LLUnlock ( (HLLE)hem );
            return xosdDuplicate;
        }
        LLUnlock ( (HLLE)hemm );
    }

    lpem->model = dwModel;

    LLUnlock ( (HLLE)hem );

    *lphem = hem;

    return xosd;
}


XOSD
OSDAPI
OSDDeleteEM (
    HEM  hem
    )
/*++

Routine Description:

    Remove the execution model (hem) from os debug's list of available
    execution models ( llem ).

    Check the list of pids using this execution model.  If it is zero
    indicating that no pids are using it, then call the list manager
    to delete it from the list of available execution models ( llem ).

Arguments:

    hem  - Supplies handle to EM which is being removed

Return Value:

    xosdNone - Success.

    xosdInvalidEM - The execution model handle ( hem ) is invalid.

    xosdInUse - The execution model is still being used by some pid.
                  OSDDiscardEM must be called on all of the pids using
                  this particular em before OSDDeleteEM can be called
                  without error.

--*/
{
    XOSD xosd = xosdNone;
    LPEMS lpem;

    assert ( hem != NULL );

    lpem = (LPEMS) LLLock ( (HLLE)hem );

    if ( LLSize ( lpem->llhpid ) != 0 ) {

        LLUnlock ( (HLLE)hem );
        xosd = xosdInUse;

    } else {

        // Tell the EM and DM that they're about to be discarded
        (lpem->emfunc) ( emfUnInit, NULL, NULL, 0, 0 );

        LLUnlock ( (HLLE)hem );

        if ( !LLDelete ( llem, (HLLE)hem ) ) {
            xosd = xosdInvalidParameter;
        }
    }

    return xosd;
}


XOSD
OSDAPI
OSDEMGetInfo(
    HEM hem,
    LPEMIS lpemis
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    XOSD xosd = xosdNone;
    LPEMS lpem;

    assert ( hem != NULL );
    lpem = (LPEMS) LLLock ( (HLLE)hem );
    xosd = (lpem->emfunc) ( emfGetInfo, NULL, NULL, 0, (DWORD64)lpemis );
    LLUnlock( (HLLE)hem );
    return xosd;
}


XOSD
OSDAPI
OSDEMSetup(
    HEM hem,
    LPEMSS lpemss
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    XOSD xosd = xosdNone;
    LPEMS lpem;

    assert ( hem != NULL );
    lpem = (LPEMS) LLLock ( (HLLE)hem );
    xosd = (lpem->emfunc) ( emfSetup, NULL, NULL, 0, (DWORD64)lpemss );
    LLUnlock( (HLLE)hem );
    return xosd;
}




/************************************************************************/

/*    EM Manipulation                                                   */

/************************************************************************/



XOSD
OSDAPI
OSDGetCurrentEM (
    HPID hpid,
    HTID htid,
    LPHEM lphem
    )
/*++

Routine Description:

    Get the handle for the current execution model associated
    with hpid:htid.

Arguments:

    hpid  - Supplies process

    htid  - Supplies thread

    lphem - Returns the handle to the execution model.

Return Value:

    xosdNone - Success
    xosdInvalidEM - No valid execution model for this hpid:htid

--*/
{
    LPPROCESSINFO lppid;
    HEM hodem;
    LPEMP lpemp;
    LPEMS lpem;

    Unreferenced( htid );

    assert ( lphem != NULL );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    if ( lppid == NULL ) {
        return xosdInvalidParameter;
    }
    if ( lppid->lastmodel == CEXM_MDL_native ) {
        lpemp = (LPEMP) LLLock ( (HLLE)lppid->hempNative );
        *lphem = lpemp->hem;
        LLUnlock ( (HLLE)lppid->hempNative );
    }
    else {
        hodem = NULL;
        while ( hodem = (HEM)LLNext ( llem, (HLLE)hodem ) ) {
            lpem = (LPEMS) LLLock ( (HLLE)hodem );
            if ( lpem->model == lppid->lastmodel ) {
                *lphem = hodem;
            }
            LLUnlock ( (HLLE)hodem );
        }
    }
    LLUnlock ( (HLLE)hpid );
    return xosdNone;
}


XOSD
OSDAPI
OSDNativeOnly (
    HPID hpid,
    HTID htid,
    DWORD fNat
    )
/*++

Routine Description:

    Force the use of the native em even where there is
    non-native code (ie pcode or emulator).

Arguments:

    hpid  - Supplies process

    htid  - Supplies thread

    fNat - Supplies to set native only, false to return to normal
             mode of handling multiple em's

Return Value:

    xosdNone - Success

--*/
{
    HEMP hemp;
    HEMP hempTmp = NULL;
    XOSD xosd = xosdNone;
    LPEMP  lpemp;
    LPPROCESSINFO lppid;

    assert ( hpid != NULL );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );

    if ( lppid->fNative == fNat ) {
        LLUnlock((HLLE)hpid);
        return xosd;
    }
    else if ( fNat ) {

        // get the native em
        hemp = lppid->hempNative;

        // tell all of the non-native models to disconnect, cleanup or
        // whatever they need to do
        while ( ( hempTmp = (HEMP)LLNext ( lppid->llemp, (HLLE)hempTmp ) ) &&
                                                        hempTmp != hemp ) {
            lpemp = (LPEMP) LLLock ( (HLLE)hempTmp );
            xosd = (lpemp->emfunc) ( emfDetach, hpid, htid, 0, 0 );
            LLUnlock ( (HLLE)hempTmp );
        }

        // we then move the native em to the head of the list, forcing it
        // to be called first, until fNative is reset
        LLRemove ( lppid->llemp, (HLLE)hemp );
        LLAddHead ( lppid->llemp, (HLLE)hemp );

        // if current model used to be non-native, send notification
        if ( lppid->lastmodel != CEXM_MDL_native ) {
            OSDDoCallBack ( dbcEmChange,
                            hpid,
                            htid,
                            lppid->lastmodel,
                            CEXM_MDL_native,
                            0
                            );
            lppid->lastmodel = CEXM_MDL_native;
        }

        // finally, set our global flag to true
        lppid->fNative = fNat;
    }
    else {

        // put the native em back at the end of the list
        hemp = lppid->hempNative;
        LLRemove ( lppid->llemp, (HLLE)hemp );
        LLAdd ( lppid->llemp, (HLLE)hemp );

        // tell all of the non-native models that they can re-connect
        xosd = xosdPunt;
        while ( ( hempTmp = (HEMP)LLNext ( lppid->llemp, (HLLE)hempTmp ) ) &&
                                         hempTmp != hemp && xosd == xosdPunt) {
            DWORD dwModel;

            lpemp = (LPEMP) LLLock ( (HLLE)hempTmp );
            xosd = (lpemp->emfunc) ( emfAttach,
                                     hpid,
                                     htid,
                                     0,
                                     (DWORD64) &dwModel );
            if ( xosd == xosdNone ) {

                // send a dbcEmChange notification
                OSDDoCallBack ( dbcEmChange,
                                hpid,
                                htid,
                                lppid->lastmodel,
                                dwModel,
                                0 );
                lppid->lastmodel = dwModel;
            }
            LLUnlock ( (HLLE)hempTmp );
        }

        if ( xosd == xosdPunt ) {
            xosd = xosdNone;
        }

        // reset our global flag to false
        lppid->fNative = fNat;
    }

    LLUnlock ( (HLLE)hpid );
    return xosd;
}


XOSD
OSDAPI
OSDUseEM (
    HPID hpid,
    HEM hem
    )
/*++

Routine Description:

    To tell osdebug that it should pass commands and callbacks for the
    process hpid through the execution model whose handle is hem.

Arguments:

    hpid - Supplies process

    hem  - Supplies handle to the execution model

Return Value:

    xosdNone - Success
    xosdOutOfMemory - Not enough memory to create the reference
                      to the execution model in the process structure.
    xosdInvalidEM - tried to add a native em when one was already present

--*/
{
    LPPROCESSINFO lppid;
    HEMP hemp = NULL;
    LPEMS lpem;
    LPEMP lpemp;
    HPID hpidem;
    LPHPID lphpid;

    assert ( hem != NULL );
    assert ( hpid != NULL );

    // Add hem to pid's hem list

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );

    while ( hemp = (HEMP)LLNext ( lppid->llemp, (HLLE)hemp ) ) {
        lpemp = (LPEMP) LLLock ( (HLLE)hemp );
        if ( lpemp->hem == hem ) {

            // this is an error, cannot add the same model twice
            LLUnlock ( (HLLE)hemp );
            LLUnlock ( (HLLE)hpid );
            return xosdDuplicate;
        }
        LLUnlock ( (HLLE)hemp );
    }

    hemp = (HEMP)LLCreate ( lppid->llemp );
    if ( hemp == NULL ) {
        LLUnlock ( (HLLE)hpid );
        return xosdOutOfMemory;
    }

    lpem = (LPEMS) LLLock ( (HLLE)hem );
    if ( lpem->emtype ) {
        LLAddHead ( lppid->llemp, (HLLE)hemp );
    }
    else {
        // new native em
        LLAdd ( lppid->llemp, (HLLE)hemp );
        lppid->hempNative = hemp;
    }
    LLUnlock ( (HLLE)hpid );

    // add hpid to hem in llem

    hpidem = (HPID)LLCreate ( lpem->llhpid );
    if ( hpidem == NULL ) {
        return xosdOutOfMemory;
    }
    lphpid = (LPHPID) LLLock ( (HLLE)hpidem );

    // puts hpid in node
    *lphpid = hpid;

    LLUnlock ( (HLLE)hpidem );
    LLUnlock ( (HLLE)hem );
    LLAdd ( lpem->llhpid, (HLLE)hpidem );

    return xosdNone;
}


XOSD
OSDAPI
OSDDiscardEM (
    HPID hpid,
    HTID htid,
    HEM hem
    )
/*++

Routine Description:

    Remove an execution model from a process' list of available
    execution models.

Arguments:

    hpid - Supplies process

    hem  - Supplies handle to the execution model

Return Value:

    xosdNone - Success
    xosdInvalidParameter - There is no execution model associated with hem.

--*/
{
    LPPROCESSINFO lppid;
    HEM hemdis;
    LPEMS lpem;
    HEMP hemp = NULL;
    HEMP hempdis;
    LPEMP lpemp;
    HPID hpiddis;
    XOSD xosd = xosdNone;

    assert ( hpid != NULL );

    // find the hem in the lppid's list
    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );

    lpemp = (LPEMP) LLLock ( (HLLE)lppid->hempNative );
    if ( lpemp->hem == hem ) {

        // trying to remove the native em
        lppid->hempNative = 0;
    }
    LLUnlock ( (HLLE)lppid->hempNative );

    while (hemp = (HEMP)LLNext ( lppid->llemp, (HLLE)hemp ) ) {
        lpemp = (LPEMP) LLLock ( (HLLE)hemp );
        if ( lpemp->hem == hem ) {
            hemdis = hem;
            hempdis = hemp;
        }
        LLUnlock ( (HLLE)hemp );
    }
    if ( ! hemdis ) {

        // no em found, return error
        xosd = xosdInvalidParameter;
    }
    else {

        // delete the em from the hpid, then remove hpid from em in llem
        LLDelete ( lppid->llemp, (HLLE)hempdis );

        lpem = (LPEMS) LLLock ( (HLLE)hemdis );
        if ( hpiddis = (HPID)LLFind ( lpem->llhpid, 0, &hpid, 0 ) ) {
            LLDelete ( lpem->llhpid, (HLLE)hpiddis );
        }
        else {

            // hpid not found: internal cosistancy error
            assert ( FALSE );
        }
        LLUnlock ( (HLLE)hemdis );
    }
    LLUnlock ( (HLLE)hpid );
    return xosd;
}




/************************************************************************/

/*    Transport layer loading, unloading and configuration              */

/************************************************************************/



XOSD
OSDAPI
OSDAddTL (
    TLFUNC tlfunc,
    LPDBF lpbdf,
    LPHTL lphtl
    )
/*++

Routine Description:

    Create and initialize a transport layer associated with the
    service function tlfunc and add it to the list of osdebug's
    available transport layers ( lltl ).

    Use list manager to create a  transport layer handle and add it
    to the transport layer list ( lltl ).

    Call the transport layer service function ( tlfunc ) to initialize
    the transport layer and to register the debugger service functions
    ( lpdbf ) and OSDebug TL callback function ( TLCallBack ).

Arguments:

    tlfunc - Supplies pointer to the transport layer service function
             to be associated with the transport layer that is being
             created.

    lpdbf -  Supplies pointer to the debugger services structure that
             will be registered with the transport layer being created.

    lphtl -  Returns the transport layer handle.

Return Value:

    xosdNone - Success.

    xosdOutOfMemory - List manager was unable to allocate TL
                      or add it to the transport layer list ( lltl ).

    Other xosd failure codes may be returned by the transport layer
    if its initialization fails.

--*/
{
    HTL  htl;
    LPTL lptl;
    XOSD xosd = xosdNone;

    assert ( tlfunc != NULL );
    assert ( lpdbf != NULL );
    assert ( lphtl != NULL );

    htl = (HTL)LLCreate ( lltl );
    if ( htl == NULL ) {

        xosd = xosdOutOfMemory;

    } else {

        LLAdd ( lltl, (HLLE)htl );

        lptl = (LPTL) LLLock ( (HLLE)htl );

        lptl->tlfunc = (TLFUNC_ODP) tlfunc;

        if (xosd == xosdNone) {
            xosd = lptl->tlfunc ( tlfInit, NULL, (UINT_PTR)lpdbf, (UINT_PTR)TLCallBack);
        }

        LLUnlock ( (HLLE)htl );

        if (xosd != xosdNone) {
            LLDelete(lltl, (HLLE)htl);
        } else {
            *lphtl = htl;
        }

    }
    return xosd;
}

XOSD
OSDAPI
OSDStartTL(
    HTL htl
    )
{
    LPTL lptl;
    XOSD xosd;

    lptl = (LPTL) LLLock ( (HLLE)htl );

    xosd = lptl->tlfunc ( tlfConnect, NULL, 0, 0 );

    LLUnlock( (HLLE)htl );
    return xosd;
}


UINT
CountProcessesUsingTl(
    HTL htl
    )
/*++

Routine Description:

    This function counts the number of hpid's currently using the given TL.

Arguments:

    htl - The TL to to count Hpids for.

Return Value:

    The number of Hpids using the given TL.

--*/
{
    HPID            hPid = NULL;
    LPPROCESSINFO   lppid;
    UINT            nUsers = 0;
    HTL             htlT;

    while (hPid = (HPID) LLNext (llpid, (HLLE) hPid)) {
        lppid = (LPPROCESSINFO) LLLock ((HLLE) hPid);
        htlT = lppid->htl;
        LLUnlock ((HLLE)hPid);

        if (htlT == htl) {
            nUsers++;
        }
    }

    return nUsers;
}



XOSD
OSDAPI
OSDDeleteTL (
    HTL htl
    )
/*++

Routine Description:

    Remove the transport layer (htl) from OSDebug's list of available
    transport layers ( lltl ).

    Check the list of pids using this transport layer.  If it is zero
    indicating that no pids are using it, then call the list manager
    to delete it from the list of available transport layers ( lltl ).

Arguments:

    htl - A transport layer that has previously been returned by
          OSDAddTL.

Return Value:

    xosdNone - Success.

    xosdInUse - The transport layer is still being used by some pid.
                  OSDDiscardTL must be called on all of the pids using
                  this particular em before OSDDeleteTL can be called
                  without error.

    xosdInvalidParameter - The transport layer handle ( htl ) is invalid.

--*/
{
    XOSD xosd = xosdNone;
    LPTL lptl;

    assert ( htl != NULL );

    lptl = (LPTL) LLLock ( (HLLE)htl );

    if ( CountProcessesUsingTl (htl) != 0 ) {

        LLUnlock ( (HLLE)htl );
        xosd = xosdInUse;

    } else {

        (*lptl->tlfunc)(tlfDestroy, NULL, 0, 0);

        LLUnlock ( (HLLE)htl );

        if ( !LLDelete ( lltl, (HLLE)htl ) ) {
            xosd = xosdInvalidParameter;
        }
    }

    return xosd;
}


XOSD
OSDAPI
OSDDiscardTL(
    HPID    hpid,
    HTL     htl
    )
/*++

Routine Description:

    Remove a reference to a tl by a pid.  In particular, this allows us to
    remove a TL before we destroy the PID.  This is sometimes necessary in
    error conditions.

--*/
{
    HPID                    hpidT = NULL;
    LPPROCESSINFO   lppid;

    lppid = (LPPROCESSINFO) LLLock ((HLLE) hpid);

    if (lppid->htl == htl) {
        lppid->htl = NULL;
    }

    LLUnlock ((HLLE) hpid);


    return xosdNone;
}


XOSD
OSDAPI
OSDTLGetInfo(
    HTL htl,
    LPTLIS lptlis
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    XOSD xosd;
    LPTL lptl;
    assert ( htl != NULL );
    lptl = (LPTL) LLLock ( (HLLE)htl );
    xosd = (*lptl->tlfunc) ( tlfGetInfo, NULL, 0, (UINT_PTR) lptlis );
    LLUnlock ( (HLLE)htl );
    return xosd;
}


XOSD
OSDAPI
OSDTLSetup(
    HTL htl,
    LPTLSS lptlss
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    XOSD xosd;
    LPTL lptl;
    assert ( htl != NULL );
    lptl = (LPTL) LLLock ( (HLLE)htl );
    xosd = (*lptl->tlfunc) ( tlfSetup, NULL, 0, (UINT_PTR) lptlss );
    LLUnlock ( (HLLE)htl );
    return xosd;
}



XOSD
OSDAPI
OSDDisconnect(
    HPID hpid,
    HTID htid
    )
/*++

Routine Description:

    Performs a brute force disconnect of the TL.  This effectively simulates
    hanging up the phone or unplugging the network on a remote TL.

Arguments:

    hpid - Supplies process to bind the TL

    htid - Supplies thread, ignored

Return Value:

    xosdNone, usually.

--*/
{
    if (!hpid) {
        return xosdUnknown;
    }

    return CallTL( tlfDisconnect, hpid, (UINT_PTR)htid, 0 );
}




/************************************************************************/

/*    Target process initialization / deletion                          */

/************************************************************************/



XOSD
OSDAPI
OSDCreateHpid (
    LPFNSVC lpfnsvcCallBack,
    HEM     hemNative,
    HTL     htl,
    LPHPID  lphpid
    )
/*++

Routine Description:

    Create the structures associated with a process

    Create the osdebug pid structure and add it to the list of processes
    ( llpid ).  Notify the native execution model ( hem ) and the
    transport layer ( htl ) of the new process.

    When connecting to a remote transport layer, check the version
    signature to verify that the components are compatible.

Arguments:

    lpfnsvcCallBack   - Supplies a pointer to the debugger callback function.

    hemNative - Supplies a handle to the native execution model.

    htl       - Supplies a handle to the transport layer.  This
                    may be NULL, in which case the new hpid can ONLY
                    be used for operations which don't need to go to
                    the remote side, such as querying (most) metrics.

    lphpid    - Returns the handle to the pid that is generated.

Return Value:

    xosdOutOfMemory - The List manager was unable to allocate
                      memory for the structures to be created.

    Any error values that may be generated by the transport
    layer or execution model initialization of a process.

--*/
{
 /*                                                                         *
  *      Use the list manager to create a process structure and add it to   *
  *      the process list ( llpid ).                                        *
  *                                                                         *
  *      Initialize the fields of the process structure.                    *
  *                                                                         *
  *      Call the execution model service function for the native execution *
  *      model with the emfCreatePid command and the initialization string  *
  *      ( lszEMData ).                                                     *
  *                                                                         *
  *      Call the transport layer service function for the process's        *
  *      transport layer with the tlfCreatePid command and the              *
  *      initialization string ( lszTLData ).                               */

    HEM   hodem;
    HPID  oldhpid;
    AVS   RemoteAvs;

    HPID  hpid;
    LPEMS lpem;
    XOSD  xosd = xosdNone;
    XOSD  xosdSave = xosdNone;

    assert ( lpfnsvcCallBack != NULL );
    assert ( hemNative != NULL );
    assert ( lphpid != NULL );
    assert ( llpid != 0 );

    // Create and initialize the process structure and add to llpid

    xosd = CreateProc ( lpfnsvcCallBack, hemNative, htl, &hpid );
    if (xosd != xosdNone) {
        return xosd;
    }

    // Notify the transport layer of the new process

    if (htl) {
        xosd = CallTL ( tlfConnect, hpid, 0, 0 );

        switch(xosd = CallTL (tlfGetVersion, hpid, sizeof(Avs), (DWORD64)&RemoteAvs)) {
            case xosdNone:
                // got version info for remote side of transport...
                // verify it.
                if (Avs.rlvt != RemoteAvs.rlvt || Avs.iRmj != RemoteAvs.iRmj) {
                    xosd = xosdBadVersion;    // bogus version
                }
                break;

            case xosdNotRemote:
                xosd = xosdNone;
                break;

            case xosdBadVersion:
                default:
                break;
            }
    }


    if (xosd == xosdNone) {

        // Notify the native execution model of the new process

        lpem = (LPEMS) LLLock ( (HLLE)hemNative );

        if (htl) {
            xosd = lpem->emfunc (emfConnect, hpid, NULL, 0, 0);
        }

        //
        // BUGBUG: Need to verify that we have the right DM when
        // BUGBUG: xosd == xosdInUse.
        //

        if (xosd == xosdInUse) {
            xosdSave = xosdInUse;
            xosd = xosdNone;
        }
        if (xosd == xosdNone) {
            xosd = lpem->emfunc (emfCreateHpid, hpid, NULL, 0, 0);
        }

        LLUnlock ( (HLLE)hemNative );

    }

    if (xosd == xosdNone) {

        *lphpid = hpid;

    } else {

        if (htl) {
            CallTL(tlfDisconnect, hpid, 0, 0);
        }

        // remove hpid from the lists of hpids in llem
        for (hodem = (HEM)LLNext((HLLI) llem, NULL );
             hodem;
             hodem = (HEM)LLNext( (HLLI)llem, (HLLE)hodem )) {

            lpem = (LPEMS) LLLock ( (HLLE)hodem );
            if ( oldhpid = (HPID)LLFind ( lpem->llhpid, 0, &hpid, 0L ) ) {
                LLDelete ( lpem->llhpid, (HLLE)oldhpid );
            }
            LLUnlock ( (HLLE)hodem );
        }

        LLUnlock ( (HLLE)hodem );
        LLDelete ( llpid, (HLLE)hpid );

    }

    return xosd == xosdNone? xosdSave : xosd;
}



XOSD
OSDAPI
OSDDestroyHpid (
    HPID hpid
    )
/*++

Routine Description:

    Destroy the structures associated with a process

    Delete the osdebug pid structure from the list of processes
    ( llpid ).  Notify the native execution model ( hem ) and the
    transport layer ( htl ) that it's been deleted.

Arguments:

    hpid - Supplies the process to destroy.

Return Value:

    xosdInvalidProc - The hpid given was invalid.

    Any error values that may be generated by the transport
    layer or execution model during destruction of a process.

--*/
{

/***************************************************************************
 *                                                                         *
 *      Call the execution model service functions to let them know that   *
 *      this hpid is being destroyed.                                      *
 *                                                                         *
 *      Use the list manager to delete the process structure from the      *
 *      process list ( llpid ).                                            *
 *                                                                         *
 ***************************************************************************/

    XOSD    xosd;
    HEM     hodem;
    LPEMS   lpem;
    HPID    oldhpid;
    LPPROCESSINFO   lppid;
    HTL     htl, htlT;

    xosd = CallEM ( emfDestroyHpid, hpid, NULL, 0, 0 );

    if (xosd == xosdNone) {

        lppid = (LPPROCESSINFO) LLLock((HLLE)hpid);
        htl = lppid->htl;
        LLUnlock((HLLE)hpid);

        //
        // if this is the last Process using this TL, disconnect
        //

        if (htl && CountProcessesUsingTl (htl) == 1) {
            xosd = CallEM ( emfDisconnect, hpid, 0, 0, 0 );
            xosd = CallTL ( tlfDisconnect, hpid, 0, 0 );
        }

        // remove hpid from the lists of hpids in llem
        for (hodem = (HEM)LLNext( (HLLI)llem, NULL );
             hodem;
             hodem = (HEM)LLNext( (HLLI)llem, (HLLE)hodem )) {

            lpem = (LPEMS) LLLock ( (HLLE)hodem );
            if ( oldhpid = (HPID)LLFind ( lpem->llhpid, 0, &hpid, 0L ) ) {
                LLDelete ( lpem->llhpid, (HLLE)oldhpid );
            }
            LLUnlock ( (HLLE)hodem );
        }

        LLDelete ( llpid, (HLLE)hpid );
    }

    return xosd;

}



XOSD
OSDAPI
OSDDestroyHtid(
    HPID hpid,
    HTID htid
    )
/*++

Routine Description:

    Companion for OSDDestroyPID(); unhooks and deletes an htid.
    There is less to this than destroying a PID: the EM has to be
    notified, but the TL does not.

Arguments:

    htid  - osdebug htid

Return Value:

    xosdNone, xosdInvalidProc, xosdInvalidThread

--*/
{
    LPTHREADINFO   lptid;
    LPPROCESSINFO   lppid;
    XOSD    xosd;

    assert( htid != NULL );
    lptid = (LPTHREADINFO) LLLock((HLLE) htid );
    if (!lptid) {
        return xosdInvalidHandle;
    }
    LLUnlock((HLLE) htid );

    xosd = CallEM ( emfDestroyHtid, hpid, htid, 0, 0 );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    if (!lppid) {
        return xosdBadProcess;
    }
    LLDelete ( lppid->lltid, (HLLE)htid );
    LLUnlock ( (HLLE)hpid );

    return xosdNone;
}



/************************************************************************/

/*    Address manipulation                                              */

/************************************************************************/


XOSD
OSDAPI
OSDGetAddr (
    HPID hpid,
    HTID htid,
    ADR adr,
    LPADDR lpaddr
    )
/*++

Routine Description:

    To get one of the special addresses in a process/thread which
    the EM can supply.

Arguments:

    hpid   - The process

    htid   - The thread

    adr    - The address to get. One of the following.

             adrPC         - The program counter of the thread.

             adrBase       - The frame base address of the thread.

             adrStack      - The current stack pointer of the thread.

             adrData       - The data area for the thread/process

             adrBaseProlog - The frame base address will be fixed up as
                             if the prolog code of the existing function
                             were already executed.  This requires that
                             the address passed in be the start of the
                             function.

             adrTlsBase    - The base of the thread local storage area.

    lpaddr - Returns the requested address.

Return Value:

    xosdNone - Success

    Any error that may be generated by the emfGetAddr function
    of the execution model associated with hpid:htid.

--*/
{
    assert ( lpaddr != NULL );

    if ( htid == NULL && adr != adrData ) {
        _fmemset ( lpaddr, 0, sizeof ( ADDR ) );
        return xosdNone;
    }
    else {
        XOSD xosd = CallEM ( emfGetAddr, hpid, htid, adr, (DWORD64)lpaddr );
        assert(Is64PtrSE(lpaddr->addr.off));
        return xosd;
    }

}


XOSD
OSDAPI
OSDSetAddr (
    HPID hpid,
    HTID htid,
    ADR adr,
    LPADDR lpaddr
    )
/*++

Routine Description:

    Sets one of the addresses associated with a thread.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread

    adr     - Supplies id of the address to set, and may be
              one of the following:

              adrPC         - The program counter of the thread.

              adrBase       - The frame base address of the thread.

              adrStack      - The current stack pointer of the thread.

              adrData       - The data area for the thread/process

              adrBaseProlog - N/A ( can't be set )

              adrTlsBase    - N/A ( can't be set )

    lpaddr - Supplies the address to set the hpid:htid with

Return Value:

    xosdNone - Success.

    Any error that may be generated by the emfSetAddr function
    of the execution model associated with hpid:htid.

--*/
{
    assert ( lpaddr != NULL );
    assert(Is64PtrSE(lpaddr->addr.off));

    if ( htid == NULL && adr != adrData ) {
        return xosdInvalidParameter;
    }
    else {
        return CallEM ( emfSetAddr, hpid, htid, adr, (DWORD64)lpaddr );
    }
}


XOSD
OSDAPI
OSDFixupAddr (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
/*++

Routine Description:

    Take an unfixed-up (logical) address and transform it into the
    equivalent fixed up (physical) address.

    Uses emfFixupAddr.

Arguments:

    hpid - Supplies the process to which the address is associated.

    htid - Supplies the thread to which the address is associated.

    lpaddr - Supplies the address which is to be fixed up.

Return Value:

    xosdNone - Success
    xosdInvalidMTE - The logical address could not be found in any of the
                     modules in the given process.


--*/
{
    ADDR addrTmp = {0};
    XOSD xosd = xosdNone;

    assert(Is64PtrSE(lpaddr->addr.off));
    addrTmp = *lpaddr;

    xosd = CallEM( emfFixupAddr, hpid, htid, 0, (DWORD64)&addrTmp );

    if (xosdNone == xosd) {
        *lpaddr = addrTmp;
    }
    
    return xosd;
}



XOSD
OSDAPI
OSDUnFixupAddr (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
/*++

Routine Description:

    Take a fixed up (physical) address and transform it into the
    equivalent unfixed up (logical) address.

    Note:  It is essential that there be complete mapping (determined by
    the em) between logical and physical addresses associated with a
    process.  Since it is often the case that part of the address space is
    not mapped via symbol information to any exe/dll, in order to work
    with the symbol handler, when these addresses are unfixed up, the hpid
    should be entered in the emi field.

    Uses emfUnfixupAddr.

Arguments:

    hpid  - Supplies the process of address to be unfixed up.

    htid  - Supplies the thread

    lpaddr - Supplies the address to be unfixed up.

Return Value:

    xosdNone - Success

--*/
{
    ADDR addrTmp = *lpaddr;
    XOSD xosd = CallEM( emfUnFixupAddr, hpid, htid, 0, (DWORD64)&addrTmp );

    if (xosdNone == xosd) {
        assert(Is64PtrSE(addrTmp.addr.off));
        *lpaddr = addrTmp;
    }
    
    return xosd;
}


XOSD
OSDAPI
OSDCompareAddrs (
    HPID hpid,
    LPADDR lpaddr1,
    LPADDR lpaddr2,
    LPDWORD lpResult
    )
/*++

Routine Description:

    Compare address *lpaddr1 to address *lpaddr2.

    Note:  It is the case in some (DOS) supported operating systems
    address comparison may not be a simple matter of comparing segments
    and offsets.  Therefore address comparison should be done through
    this function.

    Uses emfCompareAddrs.

Arguments:

    hpid -  Supplies the process with which the addresses are associated.

    lpaddr1 -  Supplies the first address to compare.

    lpaddr2 -  A pointer to the second address to compare.

    lpResult - Returns result of comparison

Return Value:

    xosdNone: Success

    *lpResult < 0:  *lpaddr1 < *lpaddr2
    *lpResult == 0:  *lpaddr1 == *lpaddr2
    *lpResult >0:  *lpaddr1 > *lpaddr2

--*/
{
    CAS cas;
    cas.lpaddr1 = lpaddr1;
    cas.lpaddr2 = lpaddr2;
    cas.lpResult = lpResult;

    assert(Is64PtrSE(lpaddr1->addr.off));
    assert(Is64PtrSE(lpaddr2->addr.off));
    
    return CallEM( emfCompareAddrs, hpid, NULL, 0, (DWORD64)&cas );
}


XOSD
OSDAPI
OSDRegisterEmi (
    HPID hpid,
    HEMI hemi,
    LPTSTR lsz
    )
/*++

Routine Description:

    Register an EMI and filename with the EM.  This hooks up the
    symbol table info for a module to the EM's representation of
    the module in the process.

Arguments:

    hpid    - Supplies process

    hemi    - Supplies handle from SH to symbolic info

    lsz     - Supplies the name of the module

Return Value:

    xosdNone on success.  Current implementation never reports failure.

--*/
{
    REMI    remi;
    remi.hemi   = hemi;
    remi.lsz    = lsz;
    return CallEM ( emfRegisterEmi, hpid, NULL, 0, (DWORD64)&remi );
}


XOSD
OSDAPI
OSDUnRegisterEmi (
    HPID hpid,
    HEMI hemi
    )
/*++

Routine Description:

    As modules are loaded, address mapping and stack walking info are
    stored in the EM.  This tells the EM to discard that info.

Arguments:

    hpid - Supplies handle to the process

    hemi - Supplies emi for module being unloaded

Return Value:

    xosdNone - Success

--*/
{
    return CallEM ( emfUnRegisterEmi, hpid, NULL, 0, (DWORD64)hemi );
}


XOSD
OSDAPI
OSDSetEmi (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return CallEM ( emfSetEmi, hpid, htid, 0, (DWORD64)lpaddr );
}



XOSD
OSDAPI
OSDGetMemoryInformation(
    HPID hpid,
    HTID htid,
    LPMEMINFO lpMemInfo
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return CallEM( emfGetMemoryInfo, hpid, htid, sizeof(MEMINFO), (DWORD64)lpMemInfo );
}




/************************************************************************/

/*    Modules and Segments                                              */

/************************************************************************/

XOSD
OSDAPI
OSDGetModuleNameFromAddress (
    IN  HPID            hpid,
    IN  DWORD64         Address,
    OUT LPTSTR          pszModuleName,
    IN  size_t          SizeOf
    )
/*++

Routine Description:

    Get a module name via the PID and address. This is done by using the EMs module
    list, and bypasses SAPI.

Arguments:

    hpid - Supplies process

    Address - Address used to find the module.

    pszModuleName - If successful will contain the module name. Recommendation: it be at 
                    least MAX_PATH in length.

    SizeOf - Size of pszModuleName in characters.

Return Value:

    xosdNone if successful.  Other xosd codes indicate the
    cause of failure.

--*/
{
    XOSD xosd;
    TCHAR szModName[_MAX_PATH];

    assert(pszModuleName);

    *szModName = 0;

    xosd = CallEM ( emfGetModuleNameFromAddress,
                    hpid,
                    NULL,
                    Address,
                    (DWORD64)szModName
                    );

    if (xosdNone == xosd) {
        _tcsncpy(pszModuleName, szModName, SizeOf-1);
        pszModuleName[SizeOf-1] = 0;
    }

    return xosd;
}


XOSD
OSDAPI
OSDGetModuleList (
    HPID            hpid,
    HTID            htid,
    LPTSTR          lpModuleName,
    LPMODULE_LIST * lplpModuleList
    )
/*++

Routine Description:

    Get the list of modules loaded in a process, or look up one
    module.  The returned module list contains one entry per
    segment per module.

Arguments:

    hpid - Supplies process

    htid - Supplies thread.  NULL is valid.

    lpModuleName - Supplies optional name of module to look for.
                 If this arg is NULL, all modules will be found.

    lplpModuleList - Returns list of modules in provided pointer.
                   This will be memory allocated with MHAlloc.

Return Value:

    xosdNone if successful.  Other xosd codes indicate the
    cause of failure.

--*/
{
    return CallEM ( emfGetModuleList,
                    hpid,
                    htid,
                    (DWORD64)lpModuleName,
                    (DWORD64)lplpModuleList
                    );
}

XOSD
OSDAPI
OSDSpawnOrphan (
    HPID hpid,
    LPCTSTR lszRemoteExe,
    LPCTSTR lszCmdLine,
    LPCTSTR lszRemoteDir,
    LPSPAWNORPHAN FAR lpso,
    DWORD   dwFlags
    )

/*++

Routine Description:

    Create a process which is not a debuggee.
    This is used for VC++'s Project.Execute.

Arguments:

    hpid - Supplies TL binding to target machine.  Only used for determining which
        EM, TL, etc. to use; will NOT attach the new process to this HPID.  You
        can think of this as sort of like OSDReadFile: it just does something
        useful on the target machine, without actually affecting the debuggee.

    lszRemoteExe - Supplies name of remote exe.                                                                *

    lszCmdLine - Supplies command line.                                                                      *

    lszRemoteDir - OPTIONAL Supplies the initial directory of the program.  This
        argument may be NULL or "", in which case the DM must choose a sensible
        default, such as the directory of the EXE.

    lpso - Returns a SPAWNORPHAN structure, in which data about the newly created
        process is returned.  lpso->dwPid is set to the PID of the new process, and
        lpso->rgchErr[] is set to either "" for a successful spawn, or an error
        string which should be displayed to the user if the spawn failed.                                                                                    *

     dwFlags -     Supplies flags that are passed to the EM.  The following
        flags are defined; any or none may be supported by any EM/DM pair:

              ulfMinimizeApp   - New process will be iconic

              ulfNoActivate    - New process will not receive focus

              ulfInheritHandles - New app will inherit handles
                                 from debugger.  This is useful
                                 when debugging an app which inherits
                                 handles from its parent.

Return Value:

    Any xosd other than xosdNone indicates failure, and the
    debugger should display an appropriate error message
    (preferably depending on the XOSD returned).  If xosdNone
    is returned, the caller must check lpso->rgchErr[0]; if
    this byte is '\0', then the spawn was successful, otherwise
    rgchErr contains an error message which should be displayed
    to the user.

--*/
{
    SOS     sos;
    XOSD    xosd;

#ifdef DEBUG
    // As long as we're un-consting the strings, we should verify that they're not changed
    unsigned cb = _ftcslen (lszRemoteExe) + _ftcslen (lszRemoteExe) +_ftcslen (lszRemoteExe);
#endif

    assert (!(dwFlags & ulfMultiProcess));
    assert (hpid);
    assert (lszRemoteExe);
    assert (lpso);

    memset (&sos, 0, sizeof(sos));

    sos.dwChildFlags        = dwFlags;
    sos.lszRemoteExe        = (LPTSTR) lszRemoteExe;
    sos.lszCmdLine          = (LPTSTR) lszCmdLine;
    sos.lszRemoteDir        = (LPTSTR) lszRemoteDir;
    sos.lpso                = lpso;

    xosd = CallEM ( emfSpawnOrphan, hpid, NULL, sizeof (sos), (DWORD64)&sos );

#ifdef DEBUG
    assert (cb == _ftcslen (lszRemoteExe) + _ftcslen (lszRemoteExe) +_ftcslen (lszRemoteExe));
#endif

    return xosd;
}

/************************************************************************/

/*    Target Application loading / unloading                            */

/************************************************************************/


XOSD
OSDAPI
OSDProgramLoad (
    HPID hpid,
    LPTSTR lszRemoteExe,
    LPTSTR lszCmdLine,
    LPTSTR lszRemoteDir,
    LPTSTR lszDebugger,
    DWORD dwFlags
    )
/*++

Routine Description:

    Send a program load request to the EM.  If the process is created,
    the shell will be notified of a new process creation.

Arguments:

    hpid -        Supplies the process the process handle that binds to the
                  EM and TL.  This is not neccessarily the hpid that will
                  correspond to the new process.

    lszRemoteExe -

    lszCmdLine -  Supplies the command line that specifies the child program
                  and its arguments.

    lszRemoteDir -

    lszDebugger - Supplies the name of the debugger to the EM.  This is
                  provided so that the DM can print the debugger's name
                  in error messages, and so the DM can put it in the
                  debuggee's window title.

    dwFlags -     Supplies flags that are passed to the EM.  The following
                  flags are defined; any or none may be supported by any
                  EM/DM pair:

                      ulfMultiProcess  - children of the new process will
                                         be debugged.

                      ulfMinimizeApp   - New process will be iconic

                      ulfNoActivate    - New process will not receive focus

                      ulfInheritHandles - New app will inherit handles
                                         from debugger.  This is useful
                                         when debugging an app which inherits
                                         handles from its parent.

Return Value:

    xosdOutOfMemory - could not allocate a buffer to copy the arguments into
    xosdFileNotFound
    xosdAccessDenied
    xosdLoadChild

--*/
{
    XOSD    xosd = xosdNone;
    PRL     prl;

    assert ( hpid != NULL );
    assert ( lszRemoteExe != NULL );

    memset (&prl, 0, sizeof(prl));

    xosd = CallEM (
        emfSetMulti,
        hpid,
        NULL,
        ( dwFlags & ulfMultiProcess ) != 0,
        0
        );
    CheckErr(xosd);

    xosd = CallEM (
        emfDebugger,
        hpid,
        NULL,
        (_ftcslen ( lszDebugger ) + 1) * sizeof(TCHAR),
        (DWORD64)lszDebugger
        );
    CheckErr(xosd);

    prl.lszRemoteExe = lszRemoteExe;
    prl.lszCmdLine = lszCmdLine;
    prl.lszRemoteDir = lszRemoteDir;
    prl.dwChildFlags = dwFlags;
    prl.lpso = NULL;
    xosd = CallEM ( emfProgramLoad,
                    hpid,
                    NULL,
                    sizeof( prl ),
                    (DWORD64)&prl
                    );
    return xosd;
}


XOSD
OSDAPI
OSDDebugActive(
    HPID   hpid,
    LPVOID lpvPrivate,
    DWORD  cbData
    )
/*++

Routine Description:

    Tell the EM to debug a process which is already running and not
    being debugged.  If this succeeds, the debugger will be notified
    of a new process, very similar to the creation of a child of
    a process already being debugged.

Arguments:

    hpid   - Supplies process to bind to the EM.  If no process is
             already being debugged, this will be the "root" process,
             and will not yet have a real debuggee attached to it.
             Otherwise, this may be any process which is bound to the
             right native EM.

    cbData - Supplies size of data passed in lpvPrivate

    lpvPrivate - Supplies pointer to a private data structure recognized
                 by the EM.

Return Value:

    xosdNone

--*/
{
    return CallEM( emfDebugActive, hpid, NULL, cbData, (DWORD64)lpvPrivate);
}



XOSD
OSDAPI
OSDSetPath(
    HPID        hpid,
    DWORD       fSet,
    LPTSTR      lszPath
    )
/*++

Routine Description:

    Sets the search path in the DM

Arguments:

    hpid - Supplies process to bind EM

    fSet - Supplies TRUE to set new path, FALSE to turn
              off path searching.

    lszPath - Supplies search path string

Return Value:

    XOSD error code

--*/
{
    return CallEM( emfSetPath, hpid, 0, fSet, (DWORD64)lszPath );
}



XOSD
OSDAPI
OSDProgramFree (
    HPID hpid
    )
/*++

Routine Description:

    Terminate and unload the program being debugged in process hpid.
    This does not guarantee termination of child processes.

Arguments:

    hpid    - Supplies the process to kill

Return Value:

    xosdNone - Success

    Any error that can be generated by emfProgramFree

--*/
{
    assert ( hpid != NULL);
    return CallEM ( emfProgramFree, hpid, NULL, 0, 0 );
}


XOSD
OSDAPI
OSDNewSymbolsLoaded (
    HPID hpid
    )
/*++

Routine Description:

    Notify the DM that new symbols have been loaded via a !reload.

Arguments:

    hpid    - Supplies the process whose EM function we should use.

Return Value:

    xosdNone - Success

    Any xosd error

--*/
{
    assert ( hpid != NULL);
    return CallEM ( emfNewSymbolsLoaded, hpid, NULL, 0, 0 );
}


XOSD
OSDAPI
OSDSignalKernelLoadCompleted (
    HPID hpid
    )
/*++

Routine Description:

    Notify the DM that the synchronous module load of the kernel is
    complete.

Arguments:

    hpid    - Supplies the process whose EM function we should use.

Return Value:

    xosdNone - Success

    Any xosd error

--*/
{
    assert ( hpid != NULL);
    return CallEM ( emfKernelLoaded, hpid, NULL, 0, 0 );
}



/************************************************************************/

/*    Target execution                                                  */

/************************************************************************/


XOSD
OSDAPI
OSDGo (
    HPID hpid,
    HTID htid,
    LPEXOP lpExop
    )
/*++

Routine Description:

    Run the target program for the process hpid [thread htid].

    uses emfGo.

    Note:  This is an asynchronous API.

Parameters:

    hpid - Supplies process to run

    htid - Supplies the thread to run

    lpExop - Supplies options to control execution

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfGo, hpid, htid, 0, (DWORD64)lpExop );
}




XOSD
OSDAPI
OSDSingleStep (
    HPID hpid,
    HTID htid,
    LPEXOP lpExop
    )
/*++

Routine Description:

    Execute a single processor instruction of the process hpid
    and thread htid.

    uses emfSingleStep

    Note:  This is an asynchronous API.

Parameters:

    hpid - Supplies process

    htid - Supplies thread

    lpExop - Supplies options to control execution

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfSingleStep, hpid, htid, 0, (DWORD64)lpExop );
}


XOSD
OSDAPI
OSDRangeStep (
    HPID hpid,
    HTID htid,
    LPADDR lpaddrMin,
    LPADDR lpaddrMax,
    LPEXOP lpExop
    )
/*++

Routine Description:

    Execute single processor instructions for hpid/htid while *lpaddrMin
    <= adrPC <= *lpaddrMax.

    Both of the boundary addresses are inclusive of the range.

    uses emfRangeStep.

    Note:  This is an asynchronous API.

Parameters:

    hpid - Supplies process

    htid - Supplies thread

    lpaddrMin - Supplies the lower boundary address

    lpaddrMax - Supplies the upper boundary address

    lpExop - Supplies options to control execution

Return Value:

    xosdNone: Success

--*/
{
    RSS rss;
    rss.lpaddrMin = lpaddrMin;
    rss.lpaddrMax = lpaddrMax;
    rss.lpExop = lpExop;

    assert(Is64PtrSE(lpaddrMin->addr.off));
    assert(Is64PtrSE(lpaddrMax->addr.off));

    return CallEM( emfRangeStep, hpid, htid, 0 , (DWORD64)&rss );
}


XOSD
OSDAPI
OSDReturnStep (
    HPID hpid,
    HTID htid,
    LPEXOP lpExop
    )
/*++

Routine Description:

    Execute the thread until it has returned to the caller.

    uses emfReturnStep

    Note:  This is an asynchronous API.

Parameters:

    hpid - Supplies process

    htid - Supplies thread

    lpExop - Supplies options to control execution

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfReturnStep, hpid, htid, 0, (DWORD64)lpExop );
}


XOSD
OSDAPI
OSDAsyncStop (
    HPID hpid,
    DWORD fSetFocus
)
/*++

Description:

    Halt the execution of the target process hpid.

    uses emfStop.

    Note:  This is an asynchronous API.

Parameters:

    hpid - Supplies process

    fSetFocus - Supplies option of changing focus to debuggee to avoid
                user having to nudge debuggee into stop


Return Value:

--*/
{
    return CallEM( emfStop, hpid, NULL, fSetFocus, 0 );
}



/************************************************************************/

/*    Calling functions in the target                                   */

/************************************************************************/



XOSD
OSDAPI
OSDSetupExecute(
    HPID    hpid,
    HTID    htid,
    LPHIND  lphind
    )
/*++

Routine Description:

    This routine is called to set up a thread for doing a function
    evaluation.  This is passed on the the EM for processing.

Arguments:

    hpid        - Supplies the handle to the process

    htid        - Supplies the handle to the thread

    lphind      - Supplies a pointer to save the SAVEINFO handle at

Return Value:

    XOSD error code

--*/

{
    XOSD xosd;
    xosd = CallEM( emfSetupExecute, hpid, htid, 0, (DWORD64)lphind);

    return xosd;
}                               /* OSDSetupExecute() */


XOSD
OSDAPI
OSDStartExecute(
    HPID    hpid,
    HIND    hind,
    LPADDR  lpaddr,
    DWORD   fIgnoreEvents,
    DWORD   fFar
    )

/*++

Routine Description:

    Execute function evaluation.

Arguments:

    hpid        - Supplies the handle to the process

    hind        - Supplies the exeute object handle

    lpaddr      - Supplies the address to start evaluation at

    fIgnoreEvents - Supplies TRUE if sub-events are to be ignored

    fFar        - Supplies TRUE if this is an _far function

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    EXECUTE_STRUCT es;

    assert(Is64PtrSE(lpaddr->addr.off));

    es.addr = *lpaddr;
    es.fIgnoreEvents = fIgnoreEvents;
    es.fFar = fFar;

    return CallEM( emfStartExecute, hpid, 0, (DWORD64)hind, (DWORD64)&es);
}                               /* OSDStartExecute() */


XOSD
OSDAPI
OSDCleanUpExecute (
    HPID hpid,
    HIND hind
    )

/*++

Routine Description:

    This routine is called to clean up a current pending function
    evaluation.

Arguments:

    hind      Supplies the handle to the execute object

Return Value:

    XOSD error code

--*/

{
    return CallEM( emfCleanUpExecute, hpid, NULL, (DWORD64)hind, 0);
}                               /* OSDCleanUpExecute() */


/************************************************************************/

/*    General target information                                        */

/************************************************************************/


XOSD
OSDAPI
OSDGetDebugMetric (
    HPID hpid,
    HTID htid,
    MTRC mtrc,
    LPVOID lpv
    )
/*++

Routine Description:

    This is a facility for getting various information about
    the execution model associated with a process.

Arguments:

    mtrc    - Supplies metric to query, one of:

        mtrcAsync        whether debuggee runs asynchronously from debugger

        mtrcWatchPoints  whether watchpoints are supported on target

        mtrcPidSize      size in bytes of an OS PID

        mtrcTidSize      size in bytes of an OS TID

        mtrcPidValue     get OS PID for the specified HPID

        mtrcTidValue     get OS TID for the specified HPID/HTID pair

        mtrcProcessorType   debuggee's processor (return value from enum MPT)

        mtrcProcessorLevel  debuggee's processor level

        mtrcThreads      whether target OS supports multiple threads

        mtrcEndian       big Endian or little Endian

        mtrcCRegs        Number of Registers

        mtrcCFlags       Number of Flags

        mtrcAssembler    Assembler exists for the Execution model

    hpid    - Supplies process

    htid    - Supplies thread (optional for some metrics)

    lpv     - Returns requested info

Return Value:

    xosdNone for success, other xosd codes describe the cause of failure.
    lpl must point to a LONG which will receive the info.

--*/
{
    return CallEM ( emfMetric, hpid, htid, mtrc, (DWORD64)lpv );
}


/************************************************************************/

/*    Target memory                                                     */

/************************************************************************/


XOSD
OSDAPI
OSDReadMemory (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPVOID lpbBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbRead
    )
/*++

Routine Description:

    Read a stream of cbBuffer bytes from the target process hpid at the
    address *lpaddr into the buffer *lpbBuffer.  A negative return value
    is an error, positive return values represent the number of bytes
    actually read. The return value will never be greater than the number
    of bytes requested.

    uses emfReadMemory

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpaddr - Supplies address to read from

    lpbBuffer - Returns the results

    cbBuffer - Supplies the number of bytes to read

    lpcbRead - Returns the nuber of bytes actually read

Return Value:

    xosdNone: Success

--*/
{
    RWMS rwms;
    
    assert(Is64PtrSE(lpaddr->addr.off));
    
    rwms.lpaddr = lpaddr;
    rwms.lpbBuffer = (LPBYTE) lpbBuffer;
    rwms.cbBuffer = cbBuffer;
    rwms.lpcb = lpcbRead;

    return CallEM( emfReadMemory, hpid, htid, 0, (DWORD64)&rwms );
}


XOSD
OSDAPI
OSDWriteMemory (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPVOID lpbBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbWritten
    )
/*++

Routine Description:

    Write a stream of cbBuffer bytes from the buffer *lpbBuffer to the
    target process specified by hpid at the address *lpaddr.

    uses emfWriteMemory

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpaddr - Supplies address to write data to

    lpbBuffer - Supplies data to write

    cbBuffer - Supplies the number of bytes to write

    lpcbWritten - Returns the number of bytes actually written

Return Value

    xosdNone: Success

--*/
{
    RWMS rwms;

    assert(Is64PtrSE(lpaddr->addr.off));

    rwms.lpaddr = lpaddr;
    rwms.lpbBuffer = (LPBYTE) lpbBuffer;
    rwms.cbBuffer = cbBuffer;
    rwms.lpcb = lpcbWritten;
    return CallEM( emfWriteMemory, hpid, htid, 0, (DWORD64)&rwms );
}


XOSD
OSDAPI
OSDGetObjectLength (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPUOFFSET lpuoffStart,
    LPUOFFSET lpuoffLength
    )
/*++

Routine Description:

    Get the length of a given linear exe object

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpaddr - Supplies address within the object in question

    lpuoffStart - Returns linear address of beginning of object

    lpuoffLength - Returns the length, in bytes, of the object

Return Value:

    xosdNone - Success

    Any value that can be returned by the emfGetObjLength of the
    execution model called.

--*/
{
    GOL gol;

    assert ( lpaddr != NULL );
    assert(Is64PtrSE(lpaddr->addr.off));

    gol.lplBase = lpuoffStart;
    gol.lplLen  = lpuoffLength;
    gol.lpaddr  = lpaddr;

    return CallEM ( emfGetObjLength, hpid, htid, 0, (DWORD64)&gol );
}



XOSD
OSDAPI
OSDGetFunctionInformation(
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPFUNCTION_INFORMATION lpFunctionInformation
    )
/*++

Routine Description:

    Get information describing the layout of a function.  The data available
    are platform dependent, but the FUNCTION_INFORMATION structure is
    portable.

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpaddr - Supplies address within the object in question

    lpFunctionInformation - Returns a packet describing the function

Return Value:

    xosdNone - Success

    Any value that can be returned by the emfGetFunctionInfo of the
    execution model called.

--*/
{
    GFI gfi;

    assert ( lpaddr != NULL );
    assert(Is64PtrSE(lpaddr->addr.off));

    gfi.lpaddr = lpaddr;
    gfi.lpFunctionInformation = lpFunctionInformation;

    return CallEM ( emfGetFunctionInfo, hpid, htid, 0, (DWORD64)&gfi );
}

/************************************************************************/

/*    Register management                                               */

/************************************************************************/



XOSD
OSDAPI
OSDGetRegDesc (
    HPID hpid,
    HTID htid,
    DWORD iReg,
    LPRD lprd
    )
/*++

Routine Description:

    Get the internal description of what a specific register looks
    like.  This is provided for general UI interfaces.  See the
    RD structure for more info.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread (optional?)

    iReg    - Supplies index of register

    lprd    - Returns requested info in an RD structure

Return Value:

    xosdNone.  Results are undefined if iReg is out of range.

--*/
{
    return CallEM ( emfGetRegStruct, hpid, htid, iReg, (DWORD64)lprd );
}


XOSD
OSDAPI
OSDGetFlagDesc (
    HPID hpid,
    HTID htid,
    DWORD iFlag,
    LPFD lpfd
    )
/*++

Routine Description:

    Get the internal description of what a specific flag looks
    like.  This is provided for general UI interfaces.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread (unused?)

    iReg    - Supplies index of requested flag

    lprd    - Returns FD structure describing flag

Return Value:

    xosdNone.  If iReg is out of range, behaviour is undefined.

--*/
{
    return CallEM ( emfGetFlagStruct, hpid, htid, iFlag, (DWORD64)lpfd );
}


XOSD
OSDAPI
OSDReadRegister (
    HPID hpid,
    HTID htid,
    DWORD dwIndex,
    LPVOID lpv
    )
/*++

Routine Description:

    Read a register in a debuggee thread

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    dwIndex - Supplies register index

    lpv - Returns register value

Return Value:

    xosdNone - Success

    The EM may return an xosd failure status if wIndex is invalid
    or for other reasons.

--*/
{
    return CallEM ( emfGetReg, hpid, htid, dwIndex, (DWORD64)lpv );
}


XOSD
OSDAPI
OSDWriteRegister (
    HPID hpid,
    HTID htid,
    DWORD dwIndex,
    LPVOID lpv
    )
/*++

Routine Description:

    Write a register in a debuggee thread

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    dwIndex - Supplies register index

    lpv - Supplies register value

Return Value:

    xosdNone - Success

    The EM may return an xosd failure status if wIndex is invalid
    or for other reasons.

--*/
{
    return CallEM ( emfSetReg, hpid, htid, dwIndex, (DWORD64)lpv );
}


XOSD
OSDAPI
OSDReadFlag (
    HPID hpid,
    HTID htid,
    DWORD dwIndex,
    LPVOID lpv
    )
/*++

Routine Description:

    Read the flag specified by wIndex in the processor flag set
    for hpid:htid pair.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread

    dwIndex  - Supplies register index

    lpv     - Returns value

Return Value:

    xosdNone - Success

    Any error that may be generated by the emfGetFlag function
    of the execution model associated with hpid:htid.

--*/
{
    assert ( lpv != NULL );
    return CallEM ( emfGetFlag, hpid, htid, dwIndex, (DWORD64)lpv );
}


XOSD
OSDAPI
OSDWriteFlag (
    HPID hpid,
    HTID htid,
    DWORD dwIndex,
    LPVOID lpv
    )
/*++

Routine Description:

    Write the flag specified by wIndex in the processor flag set
    for hpid:htid pair.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread

    dwIndex  - Supplies register index

    lpv     - Supplies new value

Return Value:

    xosdNone - Success

    Any error that may be generated by the emfSetFlag function
    of the execution model associated with hpid:htid.

--*/
{
    return CallEM ( emfSetFlag, hpid, htid, dwIndex, (DWORD64)lpv );
}


XOSD
OSDAPI
OSDSaveRegs (
    HPID hpid,
    HTID htid,
    LPHIND lphmem
    )
/*++

Routine Description:

    Save the register set of a thread so that it may be restored later.

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lphmem - Returns a handle to the stored context

Return Value:

    xosdNone - Success

    Any value that can be returned by the emfSaveRegs of the execution
    model called.

--*/
{
    return CallEM ( emfSaveRegs, hpid, htid, 0, (DWORD64)lphmem );
}


XOSD
OSDAPI
OSDRestoreRegs (
    HPID hpid,
    HTID htid,
    HIND hmem
    )
/*++

Routine Description:

    Restore a thread context stored by OSDSaveRegs.

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    hmem - Supplies saved context

Return Value:

    xosdNone - Success
    Any value that can be returned by the emfRestoreRegs of the
    execution model called.

--*/
{
    return CallEM ( emfRestoreRegs, hpid, htid, 0, (DWORD64)hmem );
}



/************************************************************************/

/*     Breakpoints                                                      */

/************************************************************************/


XOSD
OSDAPI
OSDBreakpoint (
    HPID hpid,
    LPBPS lpbps
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
    return CallEM ( emfBreakPoint,
                    hpid,
                    NULL,
                    SizeofBPS(lpbps),
                    (DWORD64)lpbps
                    );
}


/************************************************************************/

/*    Assembly and Disassembly                                          */

/************************************************************************/



XOSD
OSDAPI
OSDUnassemble (
    HPID hpid,
    HTID htid,
    LPSDI lpsdi
    )
/*++

Routine Description:

    Disassemble one machine instruction.  The address of the
    instruction to disassemble is in the SDI structure pointed
    to by lpsdi.

Arguments:

    hpid    - Supplies process to get data from

    htid    - Supplies thread

    lpsdi   - Supplies and Returns disassembly info

Return Value:

    xosdNone for success, other xosd codes describe the reason
    for failure.

    When the call succeeds, lpsdi->lsz will point to a static
    string containing the disassembled instruction.  The SDI
    structure contains ADDR fields which will contain effective
    addresses computed for the instruction, and int fields which
    describe the length of each of the parts of the disassembly
    text, to facilitate formatting the text for display.

    The addr field will be updated to point to the next instruction
    in the stream.

--*/
{
    assert ( lpsdi != NULL );
    return CallEM ( emfUnassemble, hpid, htid, 0, (DWORD64)lpsdi );
}



XOSD
OSDAPI
OSDAssemble (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPTSTR lsz
    )
/*++

Routine Description:

    Assemble an instruction into the debuggee.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread (optional?)

    lpaddr  - Supplies address to assemble to

    lsz     - Supplies instruction text

Return Value:

    xosdNone for success, other xosd codes describe cause of failure.

--*/
{
    assert(Is64PtrSE(lpaddr->addr.off));

    return CallEM ( emfAssemble, hpid, htid, (DWORD64)lpaddr, (DWORD64)lsz );
}

#define doffMax 120     // Double the Max number of instructions guaranteed
                        // to get you back in sync.  We use the second half
                        // of the buffer for cache'n.

static HPID hpidGPI = NULL;
static BYTE rgbGPI [ doffMax ];
static ADDR addrGPI;

XOSD
GPIBuildCache (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{
    XOSD xosd   =  xosdBadAddress;
    int  fFound =  FALSE;
    ADDR addr   = *lpaddr;
    ADDR addrT;
    int  ib = 0;

    _fmemset ( rgbGPI, 0, doffMax );

    addrGPI = *lpaddr;
    hpidGPI = hpid;

    GetAddrOff ( addr ) -= min ( (UOFFSET) doffMax, GetAddrOff ( *lpaddr ) );

    while ( !fFound && GetAddrOff ( addr ) < GetAddrOff ( *lpaddr ) ) {
        SDI  sdi;

        sdi.dop    = dopNone;
        sdi.addr   = addr;

        addrT = addr;

        CallEM ( emfUnassemble, hpid, htid, sizeof( sdi ), (DWORD64)&sdi );

        /*
        ** If we wrapped, because of disasm on the offset boundary
        */
        if ( GetAddrOff ( sdi.addr ) < GetAddrOff ( addr ) ) {
            break;
        }
        addr = sdi.addr;

        rgbGPI [ ib ] = (BYTE) ( GetAddrOff ( addrGPI ) - GetAddrOff ( addr ) );

        if ( GetAddrOff ( addr ) == GetAddrOff ( *lpaddr ) ) {
            xosd   = xosdNone;
            *lpaddr= addrT;
            fFound = TRUE;
        }

        ib += 1;
    }

    // We haven't synced yet, so *lpaddr is probably pointing
    //  to something that isn't really synchronous

    if ( !fFound ) {
        xosd   = (XOSD) ( GetAddrOff ( *lpaddr ) - GetAddrOff ( addrT ) );
        GetAddrOff ( *lpaddr ) -= xosd;
        if ( GetAddrOff ( *lpaddr ) != 0 ) {
            UOFFSET uoff;
            OSDGetPrevInst ( hpid, htid, lpaddr, &uoff );
        }
    }

    return xosd;
}


void
GPIShiftCache (
    LPADDR lpaddr,
    int *pib
    )
{
    int doff = (int) ( GetAddrOff ( addrGPI ) - GetAddrOff ( *lpaddr ) );
    int ib   = 0;
    int ibRet = -1;

    for( ib = 0, ibRet = -1; ibRet == -1 && ib < doffMax; ++ib ) {
        rgbGPI [ ib ] = (BYTE) max ( (int) rgbGPI [ ib ] - doff, 0 );

        if ( rgbGPI[ ib ] == 0 ) {
            ibRet = ib;
        }
    }

    if ( ib == doffMax ) {
        ibRet = doffMax - 1;
    }

    *pib = ibRet;

    addrGPI = *lpaddr;
}

XOSD
GPIUseCache (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr
    )
{
    XOSD xosd   =  xosdBadAddress;
    int  fFound =  FALSE;
    ADDR addr   = *lpaddr;
    int  ib     =  0;
    int  ibCache=  0;
    int  ibMax  =  0;
    BYTE rgb [ doffMax ];


    GPIShiftCache ( lpaddr, &ibMax );

    _fmemset ( rgb, 0, doffMax );

    GetAddrOff ( addr ) -= min ( (UOFFSET) doffMax, GetAddrOff ( *lpaddr ) );

    while ( !fFound && GetAddrOff ( addr ) < GetAddrOff ( *lpaddr ) ) {
        ADDR addrT;
        BYTE doff = (BYTE) ( GetAddrOff ( *lpaddr ) - GetAddrOff ( addr ) );

        // Attempt to align with the cache

        while ( doff < rgbGPI [ ibCache ] ) {
            ibCache += 1;
        }

        if ( doff == rgbGPI [ ibCache ] ) {

            // We have alignment with the cache

            addr  = *lpaddr;
            addrT = addr;
            GetAddrOff ( addrT ) -= rgbGPI [ ibMax - 1 ];
        }
        else {
            SDI  sdi;

            sdi.dop = dopNone;
            sdi.addr = addr;
            addrT = addr;

            CallEM ( emfUnassemble, hpid, htid, sizeof( sdi ), (DWORD64)&sdi );

            addr = sdi.addr;

            rgb [ ib ] = (BYTE) ( GetAddrOff ( addrGPI ) - GetAddrOff ( addr ) );

            ib += 1;
        }

        if ( GetAddrOff ( addr ) == GetAddrOff ( *lpaddr ) ) {
            xosd   = xosdNone;
            *lpaddr= addrT;
            fFound = TRUE;
        }

    }

    // Rebuild the cache

    _fmemmove ( &rgbGPI [ ib - 1 ], &rgbGPI [ ibCache ], ibMax - ibCache );
    _fmemcpy  ( rgbGPI, rgb, ib - 1 );

    return xosd;
}


XOSD
OSDAPI
OSDGetPrevInst (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPUOFFSET lpuoffset
    )
{
    if ( GetAddrOff ( *lpaddr ) == 0 ) {

        return xosdBadAddress;
    }
    else if (
        hpid == hpidGPI &&
        GetAddrSeg ( *lpaddr ) == GetAddrSeg ( addrGPI ) &&
        GetAddrOff ( *lpaddr ) <  GetAddrOff ( addrGPI ) &&
        GetAddrOff ( *lpaddr ) >  GetAddrOff ( addrGPI ) - doffMax / 2
    ) {

        return GPIUseCache ( hpid, htid, lpaddr );
    }
    else {

        return GPIBuildCache ( hpid, htid, lpaddr );
    }
}


/************************************************************************/

/*    Stack Tracing                                                     */

/************************************************************************/


XOSD
OSDAPI
OSDGetFrame (
    HPID hpid,
    HTID htid,
    DWORD cFrame,
    LPHTID lphtid
    )
/*++

Routine Description:

    Put the frame on the stack previous to the frame specified by
    *lphtid into *lphtid.

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    cFrame - Used to tell the EM how many frames total the caller will
                be requesting.  a value of zero indicates all frames.

    phTid - Pointer to real HTID, or last virtual HTID.  If the HTID
             is real the routine will return a virtualized version of
             the HTID in the lphTid paramater.  If the HTID is virtual
             then the routine will return the next virtualized HTID for
             the next frame.

Return Value:

    xosdNone - Success.
    xosdEndOfStack - No more frames.

    Any value that can be returned by the Get Frame Function
    of the execution model called.

--*/
{
    return CallEM ( emfGetFrame, hpid, htid, cFrame, (DWORD64)lphtid );
}


/************************************************************************/

/*    Process and Thread manipulation                                   */

/************************************************************************/


XOSD
OSDAPI
OSGetThreadStatus (
    HPID hpid,
    HTID htid,
    LPTST lptst
    )
/*++

Routine Description:

    Gets a thread state structure for the given hpid/htid.  This routine
    can fail if an operation is invalid for the target architetecture.

    uses emfThreadStatus

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lptst - Returns a thread state structure.

Return Value:

    xosdNone:  Success

--*/
{
    return CallEM( emfThreadStatus, hpid, htid, 0, (DWORD64)lptst );
}


XOSD
OSDAPI
OSDGetProcessStatus (
    HPID hpid,
    LPPST lppst
    )
/*++

Routine Description:

    Returns a structure describing the process status for the given hpid.

    uses emfProcessStatus

Arguments:

    hpid - Supplies process

    lppst - Returns process state structure

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfProcessStatus, hpid, NULL, 0, (DWORD64)lppst );
}


XOSD
OSDAPI
OSDGetThreadStatus (
    HPID hpid,
    HTID htid,
    LPTST lptst
    )
/*++

Routine Description:

    Returns a structure describing the thread status for the given htid.

    uses emfThreadStatus

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lptst - Returns thread state structure

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfThreadStatus, hpid, htid, 0, (DWORD64)lptst );
}


XOSD
OSDAPI
OSDFreezeThread (
    HPID hpid,
    HTID htid,
    DWORD fFreeze
    )
/*++

Routine Description:

    Freezes or thaws the thread specified by hpid/htid.

    uses emfFreezeThread

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    fFreeze - Supplies TRUE to freeze, FALSE to thaw.

Return Value:

    xosdNone:  Success

--*/
{
    return CallEM( emfFreezeThread, hpid, htid, fFreeze, 0 );
}


XOSD
OSDAPI
OSDSetThreadPriority (
    HPID hpid,
    HTID htid,
    DWORD dwPriority
)
/*++

Routine Description:

    Sets the priority of the thread specified by hpid/htid to the given
    meta priority level.

    uses emfSetThreadPriority

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    dwPriority - Supplies the meta priority level to assign to the thread.

Return Value:

    xosdNone:  Success

--*/
{
    return CallEM( emfSetThreadPriority, hpid, htid, dwPriority, 0 );
}


/************************************************************************/

/*    File manipulation                                                 */

/************************************************************************/




/************************************************************************/

/*    Exception Handling                                                */

/************************************************************************/


XOSD
OSDAPI
OSDGetExceptionState(
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpExd
    )
/*++

Routine Description:

    Given an address of an EXCEPTION_DESC filled in with the exception
    code look up the code in the table and return a pointer to the
    real exception info structure for that exception.

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpExd - A pointer to an exception structure to be filled in.
             The dwExceptionCode member of this structure must be set
             in order to use the exfSpecified or exfNext parmeter

Return Value:

    xosdNone - Success

--*/
{
    if (hpid == 0) {
        //
        // an annoying little hack, forcing htid to be pointer sized:
        // if hpid is 0, there is no OSDebug bining yet, so htid is really
        // a pointer to an EMFunc.
        //
        return ((XOSD (OSDAPI *)(EMF, HPID, HTID, DWORD64, DWORD64))htid) (
                emfGetExceptionState, 0, 0, 0, SEPtrTo64(lpExd) );
    } else {
        return CallEM ( emfGetExceptionState, hpid, htid, 0, (DWORD64)lpExd );
    }
}


XOSD
OSDAPI
OSDSetExceptionState (
    HPID hpid,
    HTID htid,
    LPEXCEPTION_DESCRIPTION lpExd
    )
/*++

Routine Description:

    Set the exception state for the given hpid/htid.

    First implementation of this API should ignore the HTID parameter
    as it is intended that it will only be able to Get/Set thread
    exception states on a per process basis rather than on a per thread
    basis.   Should we choose in the future that there is a need to
    Get/Set exception states on a per thread basis we can make use of
    the HTID parameter at that time.  At that time an htid of NULL would
    indicate all threads.

Parameters:

    hpid - Supplies process

    htid - Supplies thread

    lpExd - Supplies an exception structure to be Set.  Should this
           exception not be found in the current list of exceptions for
           the given process/thread it will be added.

Return Value:

    xosdNone:  Success

--*/
{
    return CallEM( emfSetExceptionState, hpid, htid, 0, (DWORD64)lpExd );
}


/************************************************************************/

/*    Message handling                                                  */

/************************************************************************/


XOSD
OSDAPI
OSDGetMessageMap (
    HPID            hpid,
    HTID            htid,
    LPMESSAGEMAP*   lplpMessageMap
    )
/*++

Routine Description:

    Retrieves a table from the EM of the messages that may be used
    in the process represented by hpid.  In the WIN32 implementation,
    this is a pointer to a static structure, and does not need to be
    freed or otherwise messed with.

Arguments:

    hpid    - Supplies process

    htid    - Supplies thread

    lplpMessageMap  - Returns pointer to MSGMAP structure

Return Value:

    xosdNone for success, other xosd codes describe the cause of
    failure.  If a Win32 EM is associated with hpid, this always succeeds.

--*/
{
    return CallEM ( emfGetMessageMap, hpid, htid, 0, (DWORD64)lplpMessageMap );
}


/**** OSDGetMessageMaskMap ****/

XOSD
OSDAPI
OSDGetMessageMaskMap (
    HPID hpid,
    HTID htid,
    LPMASKMAP * lplpMaskMap
)
/*++

Routine Description:

    Retrieves a table of message mask descriptions from the EM.  The
    first element in the table represents the least significant bit in
    the mask.  The second  element represents the the next bit and so on.

    uses emfGetMessageMaskMap

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lplpMaskMap - Returns a pointer to MSKMAP

Return Value:

    xosdNone:  Sucess

--*/
{
    return CallEM ( emfGetMessageMaskMap, hpid, htid, 0, (DWORD64)lplpMaskMap );
}



/************************************************************************/

/*    Screen manipulation                                               */

/************************************************************************/


XOSD
OSDAPI
OSDShowDebuggee (
    HPID hpid,
    DWORD fShow
    )
/*++

Routine Description:

    Direct the debug monitor to set or not focus to the debuggee when
    executing.  This should be called if the debug monitor is responsible
    for setting focus.  See mtrcShowDebuggee in the General Target
    Information section.  If the debugger host is responsible for
    setting focus then the DM will be calling back with a dbcShowDebuggee
    ( Formerly dbcFlipScreen ) to set focus when executing.

    uses emfShowDebuggee

Arguments:

    hpid - Supplies process to bring to the foreground
    fShow - Supplies TRUE if the debug monitor should force the debuggee
        to have focus when executing, FALSE if not.

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfShowDebuggee, hpid, NULL, fShow, 0 );
}



/************************************************************************/

/*    I/O requests                                                      */

/************************************************************************/


XOSD
OSDAPI
OSDInfoReply (
    HPID hpid,
    HTID htid,
    LPVOID lpvData,
    DWORD cbData
    )
/*++

Routine Description:

    This routine is to allow the DM to request information from the host.
    The DM will callback with a dbcInfoRequest with one of the paramaters
    being a pointer to a string indiciating the request.  This routine is
    use to convey the requested information to the DM.  This routine is
    intended solely for RIP's on Windows/Window NT but can be used for
    other things.

    Need to examine subcatagories for types of requests that will be coming
    from dbcInfoRequest so the host can display and query for information
    in a non-console fashion.

    uses emfInfoReply

Arguments:

    hpid - Supplies process

    htid - Supplies thread

    lpvData - Supplies pointer to data which is being sent back to the DM

    cbData - Supplies count in bytes of data being sent.

Return Value:

    xosdNone: Success

--*/
{
    return CallEM( emfInfoReply, hpid, htid, cbData, (DWORD64)lpvData );
}



/************************************************************************/

/*    OS Specific services                                              */

/************************************************************************/


XOSD
OSDAPI
OSDGetTaskList (
    HPID hpid,
    LPTASKLIST * lplpTaskList
    )
/*++

Routine Description:

    Get a list of processes from the target system.  This is intended to
    give the debugger the ability to browse for a process to debug.

    uses emfGetTaskList

Arguments:

    hpid - Supplies process.  The process is not relevant, only the
            EM which it is bound to.  Hpid may be NULL, and the EM
            selected will be the oldest native EM loaded.

    lplpTaskList - Returns the list of items.  This will be allocated
                   so that it can be freed with MHFree.

Return Value:

    xosdNone
    xosdInvalidProc
    xosdOutOfMemory

--*/
{
    return CallEM ( emfGetTaskList, hpid, NULL, 0, (DWORD64)lplpTaskList );
}


XOSD
OSDAPI
OSDSetDebugMode (
    HPID hpid,
    DBM dbmService,
    LPVOID lpvData,
    DWORD cbData
    )
/*++

Routine Description:

    If the target supports hard mode/ soft mode switching ( See
    mtrcHardSoftMode in the General Target Information Section ) then this
    routine can be called to set the target into hard or soft mode.  The
    data provided is specific to the target machine and the host must be
    aware of the format of the data.

    uses emfSetDebugMode

Parameters:

    hpid - Supplies process

    dbmService - Supplies new mode, either dmbSoftMode or dbmHardMode

    lpvData - Supplies a pointer to a DBMI

    cbData - Supplies size of a DBMI structure

Return Value:

    xosdNone:  Success

--*/
{
    SDMS sdms;
    sdms.dbmService = dbmService;
    sdms.lpvData = lpvData;
    sdms.cbData = cbData;
    return CallEM( emfSetDebugMode, hpid, NULL, 0, (DWORD64)&sdms );
}


XOSD
OSDAPI
OSDSystemService (
    HPID hpid,
    HTID htid,
    SSVC ssvc,
    LPVOID lpvData,
    DWORD cbData,
    LPDWORD lpcbReturn
    )
/*++

Routine Description:

    Provide OS specific functionality.  Calls the EM service function,
    requesting emfSystemService.

Arguments:

    hpid      - Supplies handle to the process to perform the operation
                 on.  Some SSVCs may not apply to a process, but the hpid
                 is required to provide the hook to the EM and TL.

    htid      - Supplies optional thread structure

    wFunction - Supplies the SSVC function index

    lpvData   - Supplies and returns data.  Use of this field depends on
                 the SSVC.

    cbData    - Supplies the number of bytes in the data packet

    lpcbReturn - Returns the number of bytes written to lpvData

Return Value:

    xosd return value is supplied by the service function.

--*/
{
    XOSD xosd = xosdNone;
    LPSSS lpsss = (LPSSS) malloc( sizeof( SSS ) + cbData );
    DWORD cbr;

    if ( lpsss == NULL ) {
        return xosdOutOfMemory;
    }

    lpsss->ssvc = ssvc;
    lpsss->cbSend = cbData;

    if (cbData) {
       _fmemcpy ( lpsss->rgbData, lpvData, cbData );
    }

    xosd = CallEM ((EMF) emfSystemService,
                   hpid,
                   htid,
                   sizeof ( SSS ) + cbData,
                   (DWORD64) lpsss );


    if (xosd == xosdNone) {
        cbr = lpsss->cbReturned;
        if (cbr > cbData) {
            cbr = cbData;
        }
        if (cbr) {
            _fmemcpy ( lpvData, lpsss->rgbData, cbr);
        }
        if (lpcbReturn) {
            *lpcbReturn = cbr;
        }
    }

    _ffree ( lpsss );

    return xosd;
}



















/***************************************************************************
 *                                                                         *
 *  Services provided for execution model and transport layer              *
 *                                                                         *
 ***************************************************************************/

XOSD
EMCallBackDB (
    DBC dbc,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 cb,
    DWORD64 lparam
    )
/*++

Routine Description:

    Send callback messages from an execution model to the debugger.
    The messages get passed through all intervening higher-level
    execution models.

    If dbc is a dbco value, it creates the appropriate thread or
    process and returns the hpid or htid in *lpv.

    Otherwise, it calls OSDCallbackToEM to query all of the higher
    level EM's associated with the process.  If none of them
    handled the callback, it then calls the debugger's callback
    function via OSDDoCallback.

Arguments:

    dbc    - Supplies the callback message

    hpid   - Supplies handle to the process

    htid   - Supplies handle to the thread

    dwModel - Supplies index of EM initiating the callback

    cb     - Supplies the number of bytes pointed to by lpv

    lparam    - Supplies data associated with the callback,
             Returns data for dbco callbacks.

Return Value:

    xosdNone - Success

    Any value that can be returned by the debugger or higher
    level execution models.

--*/
{
    XOSD  xosd = xosdNone;

    //
    //  Several threads can execute this, so we protect it with a critical
    //  section.
    //
    EnterCriticalSection( &CallbackCriticalSection );

    switch ( dbc ) {

        case dbcoCreateThread:

            xosd = CreateThd ( hpid, htid, &htid );
            *( (LPHTID) lparam) = htid;
            goto Return;
            //return xosd;

        case dbcoNewProc: {
                LPPROCESSINFO lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
                HPID  hpidT = hpid;
                LPEMP lpemp = (LPEMP) LLLock ( (HLLE)(lppid->hempNative) );

                xosd = CreateProc (
                    lppid->lpfnsvcCC,
                    lpemp->hem,
                    lppid->htl,
                    &hpid
                );

                *( (LPHPID) lparam) = hpid;
                LLUnlock ( (HLLE)(lppid->hempNative) );
                LLUnlock ( (HLLE)hpidT );
            }
            goto Return;
            //return xosd;
    }

    //  hit all of the EMs with the DoCallback, a la CallEM,
    //   hitting the debugger last

    xosd = OSDDoCallBackToEM ( dbc, hpid, htid, cb, lparam );

    // if xosd is xosdPunt then all non-native EMs punted on the notification
    if ( xosd == xosdPunt ) {
        xosd = OSDDoCallBack ( dbc, hpid, htid, dwModel, cb, lparam );
    }

Return:

    LeaveCriticalSection( &CallbackCriticalSection );

    return xosd;
}


XOSD
EMCallBackTL (
    TLF wCmd,
    HPID hpid,
    DWORD64 cb,
    DWORD64 lparam
    )
/*++

Routine Description:

    Send messages from the execution model (native) to the transport layer.

Arguments:

    wCmd - Supplies the message

    hpid - Supplies process

    cb   - Supplies the size of the buffer at lpv, in bytes

    lpv  - Supplies or Returns data specific to the command

Return Value:

    xosdNone for success, TL may return other xosd codes to
    describe failure.

--*/
{
    return CallTL ( wCmd, hpid, cb, lparam );
}



XOSD
EMCallBackNT (
    EMF emf,
    HPID hpid,
    HTID htid,
    DWORD64 cb,
    DWORD64 lparam
    )
/*++

Routine Description:

    Provides native em services to higher level EMs.  Sends messages
    from the execution model (non-native) to the native execution model
    for the process hpid.

Arguments:

    emf  - Supplies the execution model function number

    hpid - Supplies process

    htid - Supplies thread

    cb   - Supplies size of buffer at lpv in bytes

    lparam  - Supplies and Returns data for EM service

Return Value:

    xosdNone - Success

    xosdInvalidPID - hpid is not a handle to a process.

    Any value that can be returned by the native execution model.

--*/
{
    XOSD  xosd = xosdNone;
    LPPROCESSINFO lppid;
    HEMP   hemp;
    LPEMP  lpemp;

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    if ( lppid == NULL ) {
        return xosdInvalidParameter;
    }

    hemp = lppid->hempNative;
    LLUnlock ( (HLLE)hpid );

    lpemp = (LPEMP) LLLock ( (HLLE)hemp );
    xosd = (lpemp->emfunc) ( emf, hpid, htid, cb, lparam );
    LLUnlock ( (HLLE)hemp );

    return xosd;
}


XOSD
EMCallBackEM (
    EMF emf,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 cb,
    DWORD64 lpv
    )
/*++

Routine Description:

    Provides EM services to other EMs.  Sends messages from one
    non-native EM to the next EM in the chain for the process hpid.

Arguments:

    emf    - Supplies EM function number

    hpid   - Supplies handle to the process

    htid   - Supplies handle to the thread

    dwModel - Supplies EM # of calling EM

    cb     - Supplies size in bytes of buffer at lpv.

    lpv    - Supplies and Returns data for EM service

Return Value:

    xosd code returned by the EM that handles the service request.

--*/
{
    XOSD  xosd = xosdNone;
    LPPROCESSINFO lppid;
    HEMP   hemp = 0;
    HEMP hempnext = 0;
    LPEMP  lpemp;

    assert ( hpid != NULL );

    //native ems can never make this callback!
    assert ( dwModel != CEXM_MDL_native );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );

    if ( lppid == NULL ) {
        return xosdInvalidParameter;
    }

    while ( hemp = (HEMP)LLNext ( (HLLI)(lppid->llemp), (HLLE)hemp ) ) {
        lpemp = (LPEMP) LLLock ( (HLLE)hemp );
        if ( lpemp->model == dwModel ) {
            hempnext = (HEMP)LLNext ( (HLLI)(lppid->llemp), (HLLE)hemp );
            break;
        }
        LLUnlock ( (HLLE)hemp );
    }

    assert ( hempnext != 0 );

    lpemp = (LPEMP) LLLock ( (HLLE)hempnext );
    xosd = (lpemp->emfunc) ( emf, hpid, htid, cb, (DWORD64) lpv );
    LLUnlock ( (HLLE)hempnext );

    LLUnlock ( (HLLE)hpid );

    return xosd;
}


XOSD
TLCallBack (
    HPID hpid,
    DWORD64 cb,
    DWORD64 lpv
    )
/*++

Routine Description:

    Call the native execution model for the process hpid with the
    package sent from the transport layer.  This is how the DM
    sends things to its native EM.

Arguments:

    hpid - Supplies handle to the process

    cb   - Supplies size in bytes of the packet

    lpv  - Supplies the packet

Return Value:

    xosdNone - Success

    Any return value that can be generated by a native execution model.

--*/
{
    XOSD xosd = xosdNone;
    LPEMP  lpemp;
    LPPROCESSINFO lppid;
    EMFUNC_ODP emfunc;

    assert ( hpid != NULL );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    lpemp = (LPEMP) LLLock ( (HLLE)(lppid->hempNative) );
    emfunc = lpemp->emfunc;
    LLUnlock ( (HLLE)(lppid->hempNative) );
    LLUnlock ( (HLLE)hpid );

    xosd = (*emfunc)( emfDebugPacket, hpid, NULL, cb, (DWORD64)lpv );

    return xosd;
}


/***************************************************************************
 *                                                                         *
 *  Internal Support functions                                             *
 *                                                                         *
 ***************************************************************************/



/**** CallEM - Call the execution model for hpid:htid                   ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *      This function multiplexes on the hpid and htid values to call      *
 *      the most specific execution model possible.                        *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *      emf, wValue, lValue, lpxosd: These are all just passed through     *
 *          to the actual execution model service function.                *
 *                                                                         *
 *      hpid - This is the process that the execution model service        *
 *             function is to handle.  If it is null, the first execution  *
 *             model that was registered with osdebug is called.           *
 *             Hpid must not be invalid.                                   *
 *                                                                         *
 *      htid - This is the thread that the execution model service         *
 *             function is to handle. If it is null, the native execution  *
 *             model for the pid is called. Htid must not be invalid.      *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *      Return Value - This will return whatever the execution model       *
 *          service function that it calls returns.                        *
 *                                                                         *
 *      *lpxosd - The errors returned here are those defined by the        *
 *          execution model service function that was called.              *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *      Find the most specific em available.  This is the current em       *
 *      for the pid:tid if both pid & tid are non-Null, the native         *
 *      execution model for the pid if the tid is null but the pid is      *
 *      non-Null, or the first em installed if both pid & tid are null.    *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/


XOSD
CallEM(
    EMF  emf,
    HPID hpid,
    HTID htid,
    DWORD64 wValue,
    DWORD64 lpv
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

    XOSD  xosd = xosdNone;
    HEMP  hemp;
    LPPROCESSINFO lppid;
    LPEMP lpemp;
    HLLI  llemx;

    if ( hpid == NULL ) {

        // get oldest native EM.
        if (llem != NULL) {
           hemp = (HEMP)LLLast ( llemx = llem );
        } else {
           return(xosdInvalidParameter);
        }
        if (hemp == NULL) {
           return(xosdInvalidParameter);
        }
    } else {

        lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
        assert ( lppid != NULL );
        hemp = (HEMP)LLNext ( (llemx = lppid->llemp), NULL );
        LLUnlock ( (HLLE)hpid );

    }

    lpemp = (LPEMP) LLLock ( (HLLE)hemp );
    if (lpemp->emfunc != NULL) {
       xosd = (lpemp->emfunc)( emf, hpid, htid, wValue, (DWORD64) lpv );
    } else {
       xosd = xosdInvalidParameter;
    }
    LLUnlock ( (HLLE)hemp );

    if (xosd == xosdPunt) {

        hemp = (HEMP)LLNext ( llemx, (HLLE)hemp );

        do {
            lpemp = (LPEMP) LLLock ( (HLLE)hemp );
            xosd = (lpemp->emfunc) ( emf, hpid, htid, wValue, (DWORD64) lpv );
            LLUnlock ( (HLLE)hemp );
        } while (xosd == xosdPunt && (hemp = (HEMP)LLNext ( llemx, (HLLE)hemp )) );

        if (xosd == xosdPunt) {
            xosd = xosdUnsupported;
        }
    }

    return xosd;
}


/**** CallTL - Call the transport layer for hpid                        ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *      tlf, htid, wValue, lpv - These are all just passed                 *
 *          through to the actual transport layer service function.        *
 *                                                                         *
 *      hpid - This is the process that the transport layer service        *
 *             function is to handle.  If it is null, the first transport  *
 *             layer that was registered with osdebug is called.           *
 *             Hpid must not be invalid.                                   *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *      Return Value - This will return whatever the transport layer       *
 *          service function that it calls returns.                        *
 *                                                                         *
 *      *lpxosd - The errors returned here are those defined by the        *
 *          transport layer service function that was called.              *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *      Find the most specific tl available.  This is the transport        *
 *      layer associated with the pid if the pid is non-Null, otherwise    *
 *      it is the first transport layer registered with osdebug.           *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

XOSD
CallTL (
    TLF tlf,
    HPID hpid,
    DWORD64 wValue,
    DWORD64 lpv
    )
/*++

Routine Description:

    Call the transport layer associated with hpid, and send it a
    message and packet.

Arguments:

    tlf - Supplies transport layer function number.

    hpid - Supplies process.

    wValue - Supplies function dependent data.

    lpv - Supplies and Returns data depending on function.

Return Value:

    xosd status code, as returned by the TL.

--*/
{
    XOSD xosd = xosdNone;
    HTL  htl;
    LPTL lptl;

    if ( hpid == NULL ) {
        htl = (HTL)LLNext ( lltl, NULL );
    }
    else {
        LPPROCESSINFO lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
        htl = lppid->htl;
        LLUnlock ( (HLLE)hpid );
    }

    if (!htl) {
        // Do NOT assert here!  The debugger is allowed to call any
        // OSDebug function with an HPID which was created with (HTL)0;
        // if the debugger then calls some OSDebug function which
        // requires remote communication, the function will not assert,
        // but will return xosdInvalidParameter.  The caller may check the
        // return code of the OSDebug call with an assertion if he so chooses.
        xosd = xosdInvalidParameter;
    }
    else {
        lptl = (LPTL) LLLock ( (HLLE)htl );
        xosd = lptl->tlfunc ( tlf, hpid, wValue, (DWORD64)lpv );
        LLUnlock ( (HLLE)htl );
    }

    return xosd;
}



/**** OSDDOCALLBACK - Call the debug client with a notification message ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

XOSD
OSDDoCallBack (
    DWORD wCommand,
    HPID hpid,
    HTID htid,
    DWORD dwModel,
    DWORD64 wValue,
    DWORD64 lValue
    )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

    XOSD  xosd = xosdNone;
    LPPROCESSINFO lppid;

    assert ( hpid != NULL );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    switch ( wCommand ) {
        case dbcBpt:
        case dbcProcTerm:
        case dbcThreadTerm:
        case dbcException:
        case dbcWatchPoint:
        case dbcStep:
        case dbcEntryPoint:

            if ( lppid->lastmodel != dwModel ) {

                // model has changed, issue dbcEmChange notification
                xosd = lppid->lpfnsvcCC ( dbcEmChange, hpid, htid, dwModel, 0 );
                lppid->lastmodel = dwModel;
            }
            break;

        default:

                // same model as before, do nothing
            break;
    }

    xosd = lppid->lpfnsvcCC (( USHORT ) wCommand, hpid, htid, wValue, lValue );

    LLUnlock ( (HLLE)hpid );

    switch ( wCommand ) {
        case dbcDeleteThread:
            // The shell will call OSDDestroyTID(), instead of
            // us doing it here.
            break;

        case dbcProcTerm:
            // same here; the shell calls OSDDestroyPID() when it
            // is finished using the structures.
            break;
    }

    return xosd;
}


XOSD
OSDDoCallBackToEM (
    DBC wCommand,
    HPID hpid,
    HTID htid,
    DWORD64 wValue,
    DWORD64 lValue
    )
/*++

Routine Description:

    Call all of the non-native EMs with a notification message.

Arguments:

    wCommand - Supplies the message

    hpid - Supplies process

    htid - Supplies thread

    wValue - Supplies message dependent data

    lValue - Supplies message dependent data

Return Value:

    An xosd status code.  xosdPunt means it was not handled by any EM.
    If an EM handles it, xosdNone or a failure status will be returned.

--*/
{

    XOSD  xosd = xosdPunt;
    HEMP  hemp;
    LPEMP lpemp;
    LPPROCESSINFO lppid;
    DBCPK dbcpk;

    assert ( hpid != NULL );

    // get handle to the native EM
    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    if ( lppid->fNative ) {
        LLUnlock((HLLE) hpid );
        // native only mode, return
        return xosd;
    }
    LLUnlock ( (HLLE)hpid );

    dbcpk.dbc = wCommand;
    dbcpk.hpid = hpid;
    dbcpk.htid = htid;
    dbcpk.wValue = wValue;
    dbcpk.lValue = lValue;

    // M00BUG - this is not correct. we should be hitting the ems
    //          in the reverse order that they are in the list.
    //          This will bite us when we move to multiple nms

    hemp = NULL;
    while ( xosd == xosdPunt ) {
        if ( ! ( hemp = (HEMP)LLNext ( lppid->llemp, (HLLE)hemp ) ) ) {
            return xosd;
        }

        lpemp = (LPEMP) LLLock ( (HLLE)hemp );

        if ( lpemp->emtype == emNative ) {
            LLUnlock ( (HLLE)hemp );
            return xosd;
        }

        xosd = (lpemp->emfunc) ( emfDbc, hpid, htid, sizeof(DBCPK), (DWORD64)&dbcpk );
        LLUnlock ( (HLLE)hemp );
    }
    return xosd;
}


XOSD
OSDGetLastTLError(
    HTL     htl,
    HPID    hpid,
    LPSTR   Buffer,
    ULONG   Length
    )
/*++

Routine Description:

        Get the last error from the TL.

--*/
{
    LPTL    lptl;
    XOSD    xosd;

    assert (htl);

    lptl = (LPTL) LLLock ((HLLE)htl);

    xosd = lptl->tlfunc (tlfGetLastError, hpid, Length, (DWORD64)Buffer);

    LLUnlock ((HLLE) htl);

    return xosd;
}



XOSD
OSDGetTimeStamp(
    HPID    hpid,
    HTID    htid,
    LPTSTR  ImageName,
    PULONG  TimeStamp,
    PULONG  CheckSum
    )
{
    XOSD    xosd;
    TCS     tcs = {0};

    tcs.ImageName = ImageName;

    xosd = CallEM (emfGetTimeStamp, hpid, htid, sizeof (tcs), (DWORD64)&tcs);

    if (TimeStamp) {
        *TimeStamp = tcs.TimeStamp;
    }

    if (CheckSum) {
        *CheckSum = tcs.CheckSum;
    }

    return xosd;
}


/**** CREATEPROC - Create a process data struct and add to proc list    ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/


XOSD
CreateProc (
    LPFNSVC lpfnsvc,
    HEM hem,
    HTL htl,
    LPHPID lphpid
    )
{

    XOSD  xosd = xosdNone;
    HPID  hpid;
    LPPROCESSINFO lppid;
    HEMP hemp;
    LPEMP lpemp;
    HEM hodem = NULL;
    LPEMS lpem;
    HPID hpidem = NULL;
    LPHPID lphpidt;

    hpid = (HPID)LLCreate ((HLLI) llpid );
    if ( hpid == NULL ) {
        return xosdOutOfMemory;
    }

    LLAdd ( (HLLI)llpid, (HLLE)hpid );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );
    lppid->htl       = htl;
    lppid->fNative   = FALSE;
    lppid->lastmodel = CEXM_MDL_native;
    lppid->lpfnsvcCC = lpfnsvc;

    lppid->lltid = LLInit ( sizeof ( THREADINFO ), llfNull, NULL, NULL );
    lppid->llemp = LLInit ( sizeof ( EMP ), llfNull, EMPKill, NULL );

    if ( lppid->llemp == 0 || lppid->lltid == 0 ) {
        xosd = xosdOutOfMemory;
    }

    // create llem in lppid

    while ( hodem = (HEM)LLNext ( (HLLI)llem, (HLLE)hodem ) ) {
        hemp = (HEMP)LLCreate ( (HLLI)(lppid->llemp) );
        if ( hemp == NULL ) {
            return xosdOutOfMemory;
        }
        lpem = (LPEMS) LLLock ( (HLLE)hodem );
        lpemp = (LPEMP) LLLock ( (HLLE)hemp );

        // copy relevant info to hpid's emp
        lpemp->hem = hodem;
        lpemp->emfunc = lpem->emfunc;
        lpemp->emtype = lpem->emtype;
        if ( ! lpem->emtype && hodem == hem ) {
            lppid->hempNative = hemp;
        }
        lpemp->model = lpem->model;

        LLUnlock ( (HLLE)hemp );
        LLUnlock ( (HLLE)hodem );
        LLAdd ( (HLLI)(lppid->llemp), (HLLE)hemp );
    }

    // add the hpid to all em's list of hpids

    hemp = NULL;
    hodem = NULL;
    while ( hodem = (HEM)LLNext ( (HLLI)llem, (HLLE)hodem ) ) {

        lpem = (LPEMS) LLLock ( (HLLE)hodem );
        hpidem = (HPID)LLCreate ( (HLLI)(lpem->llhpid) );
        if ( hpidem == NULL ) {
            LLUnlock((HLLE)hpid);
            return xosdOutOfMemory;
        }
        lphpidt = (LPHPID) LLLock ( (HLLE)hpidem );

        // puts hpid in node
        *lphpidt = hpid;

        LLUnlock ( (HLLE)hpidem );
        LLUnlock ( (HLLE)hodem );
        LLAdd ( (HLLI)(lpem->llhpid), (HLLE)hpidem );
    }

    // clean up

    LLUnlock ( (HLLE)hpid );
    CheckErr ( xosd );

    *lphpid = hpid;

    return xosd;
}


/**** CREATETHD - Create a thread data struct & add to process       ****
 *                                                                         *
 *  PURPOSE:                                                               *
 *                                                                         *
 *  INPUTS:                                                                *
 *                                                                         *
 *  OUTPUTS:                                                               *
 *                                                                         *
 *  IMPLEMENTATION:                                                        *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/


XOSD
CreateThd (
    HPID hpid,
    HTID htid,
    LPHTID lphtid
    )
{
    HTID  htidT;
    LPTHREADINFO lptidT;
    LPPROCESSINFO lppid;

    Unreferenced( htid );

    lppid = (LPPROCESSINFO) LLLock ( (HLLE)hpid );

    htidT = (HTID)LLCreate ( (HLLI)(lppid->lltid) );
    if ( htidT == NULL ) {
        LLUnlock ( (HLLE)hpid );
        return xosdOutOfMemory;
    }

    LLAdd((HLLI)(lppid->lltid), (HLLE)htidT);

    lptidT = (LPTHREADINFO) LLLock ( (HLLE)htidT );
    lptidT->hpid = hpid;

    LLUnlock ( (HLLE)htidT );
    LLUnlock ( (HLLE)hpid );

    *lphtid = htidT;
    return xosdNone;
}

/****************************************************************************
 *                                                                          *
 *  Kill and compare functions for the various lists used by osdebug        *
 *                                                                          *
 ****************************************************************************/


void
ODPDKill (
    LPVOID lpv
    )
{
    LPPROCESSINFO   lppid = (LPPROCESSINFO) lpv;

    LLDestroy ( lppid->llemp );
    LLDestroy ( lppid->lltid );
}


void
EMKill (
    LPVOID lpv
    )
{
    LPEMS    lpem = (LPEMS) lpv;

    LLDestroy ( lpem->llhpid );
}


void
EMPKill (
    LPVOID lpv
    )
{
    Unreferenced( lpv );
    return;
}

void
TLKill (
    LPVOID lpv
    )
{
    //
    // nothing to destroy in the current implementation
    //
}

int
EMHpidCmp (
    LPVOID lpv1,
    LPVOID lpv2,
    LONG lParam
    )
{
    Unreferenced( lParam );

    if ( *( (LPHPID) lpv1) == *( (LPHPID) lpv2 ) ) {
        return fCmpEQ;
    } else {
        return fCmpLT;
    }
}

#ifdef __cplusplus
}       // cplusplus
#endif


