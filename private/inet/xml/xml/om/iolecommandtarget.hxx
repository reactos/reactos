/*
 * @(#)IOleCommandTarget.hxx 1.0 04/14/98
 * 
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
#ifndef _PARSER_OM_IOLECOMMANDTARGET
#define _PARSER_OM_IOLECOMMANDTARGET

#include <docobj.h>


class NOVTABLE OleCommandTarget : public Object
{
    public: virtual void queryStatus(const GUID *pguidCmdGroup,
                                    ULONG cCmds,
                                    OLECMD prgCmds[],
                                    OLECMDTEXT *pCmdText) = 0;

    public: virtual void exec(const GUID *pguidCmdGroup,
                                DWORD nCmdID,
                                DWORD nCmdexecopt,
                                VARIANT *pvaIn,
                                VARIANT *pvaOut) = 0;
};

class IOleCommandTargetWrapper : public _comexport<OleCommandTarget, IOleCommandTarget, &IID_IOleCommandTarget>
{

    public: IOleCommandTargetWrapper(OleCommandTarget * p, Mutex * pMutex)
                : _comexport<OleCommandTarget, IOleCommandTarget, &IID_IOleCommandTarget>(p)
            {
                _pMutex = pMutex;
            }

    public:
        virtual /* [input_sync] */ HRESULT STDMETHODCALLTYPE QueryStatus( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ ULONG cCmds,
            /* [out][in][size_is] */ OLECMD __RPC_FAR prgCmds[  ],
            /* [unique][out][in] */ OLECMDTEXT __RPC_FAR *pCmdText);
        
        virtual HRESULT STDMETHODCALLTYPE Exec( 
            /* [unique][in] */ const GUID __RPC_FAR *pguidCmdGroup,
            /* [in] */ DWORD nCmdID,
            /* [in] */ DWORD nCmdexecopt,
            /* [unique][in] */ VARIANT __RPC_FAR *pvaIn,
            /* [unique][out][in] */ VARIANT __RPC_FAR *pvaOut);

    private:
        RMutex _pMutex;
};



#endif _PARSER_OM_IOLECOMMANDTARGET

