//+------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       EDROUTER.HXX
//
//  Contents:   Infrastructure for appropriate routing of edit commands
//
//  Classes:    CEditRouter
//
//-------------------------------------------------------------------------

#ifndef I_EDROUTER_HXX_
#define I_EDROUTER_HXX_
#pragma INCMSG("--- Beg 'edrouter.hxx'")

class CEditRouter
{
public:
    CEditRouter();
        
    ~CEditRouter();

    void Passivate();
    
    HRESULT
    ExecEditCommand(
        GUID *          pguidCmdGroup,
        DWORD           nCmdID,
        DWORD           nCmdexecopt,
        VARIANTARG *    pvarargIn,
        VARIANTARG *    pvarargOut,
        IUnknown *      punkContext,
        CDoc *          pDoc );

    HRESULT
    QueryStatusEditCommand(
        GUID *          pguidCmdGroup,
        ULONG           cCmds,
        MSOCMD          rgCmds[],
        MSOCMDTEXT *    pcmdtext,
        IUnknown *      punkContext,
        CDoc *          pDoc );

private:

    HRESULT
    SetHostEditHandler(
        IUnknown *      punkContext,
        CDoc *          pDoc );

    HRESULT
    SetInternalEditHandler(
        IUnknown *      punkContext,
        CDoc *          pDoc,
        BOOL            fForceCreate );

    IOleCommandTarget * _pHostCmdTarget;     // Alternate Editing Interface provided by the host
    BOOL                _fHostChecked;
    IOleCommandTarget * _pInternalCmdTarget;
};

#pragma INCMSG("--- End 'edrouter.hxx'")
#else
#pragma INCMSG("*** Dup 'edrouter.hxx'")
#endif
