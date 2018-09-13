/*
 * @(#)ScriptSite.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL ScriptSite object
 * 
 */


#ifndef XTL_ENGINE_SCRIPTSITE
#define XTL_ENGINE_SCRIPTSITE

#include <activdbg.h>

#ifndef _XTL_SCRIPT_IACTIVESCRIPTSITE
#include "xtl/script/iactivescriptsite.hxx"
#endif


DEFINE_CLASS(Processor);
DEFINE_CLASS(ScriptSite);

/**
 * The simplest XTL action
 *
 * Hungarian: scriptsite
 *
 */


class ScriptSite : public Base, public ActiveScriptSite, public IServiceProvider, public IActiveScriptSiteDebug, public IBindHost
{
    DECLARE_CLASS_MEMBERS_NOQI_I1(ScriptSite, Base, ActiveScriptSite);

    public: 

        static ScriptSite * newScriptSite(Processor * xtl);

        /**
         * IActiveScriptSite method.
         */
        LCID getLCID();

        /**
         * IActiveScriptSite method.
         */
        void getItemInfo(String * pstrName, DWORD dwReturnMask, IUnknown ** ppunkItem, ITypeInfo ** ppTI);

        /**
         * IActiveScriptSite method.
         */
        String * getDocVersionString();

        /**
         * IActiveScriptSite method.
         */
        void onScriptTerminate(Variant * pvarResult, const EXCEPINFO * pexcepinfo);

        /**
         * IActiveScriptSite method.
         */
        void onStateChange(SCRIPTSTATE ssScriptState);

        /**
         * IActiveScriptSite method.
         */
        void onScriptError(ActiveScriptError * pscripterror);

        /**
         * IActiveScriptSite method.
         */
        void onEnterScript();

        /**
         * IActiveScriptSite method.
         */
        void onLeaveScript();

        String * getLastError() {return _lastError;}

        /**
         * IUnknown::QueryInterface over-ride
         */
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void ** ppvObject);

        /**
         * IServiceProvider method
         */
        HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppv);

        /**
        * IActiveScriptSiteDebug methods which are not implemented.
        */       
        HRESULT STDMETHODCALLTYPE GetDocumentContextFromPosition( 
            DWORD dwSourceContext,
            ULONG uCharacterOffset,
            ULONG uNumChars,
            IDebugDocumentContext ** ppsc)
                { return E_NOTIMPL; }

        HRESULT STDMETHODCALLTYPE GetApplication(IDebugApplication ** ppda)  { return E_NOTIMPL; }

        HRESULT STDMETHODCALLTYPE GetRootApplicationNode(IDebugApplicationNode ** ppdanRoot)  { return E_NOTIMPL; }

         /**
         * IActiveScriptSiteDebug method.
         */
         HRESULT STDMETHODCALLTYPE OnScriptErrorDebug( 
            IActiveScriptErrorDebug __RPC_FAR * pErrorDebug,
            BOOL * pfEnterDebugger, 
            BOOL __RPC_FAR * pfCallOnScriptErrorWhenContinuing);

         /**
         * IBindHost method.
         */
         HRESULT STDMETHODCALLTYPE CreateMoniker( 
            /* [in] */ LPOLESTR szName,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [out] */ IMoniker __RPC_FAR *__RPC_FAR *ppmk,
            /* [in] */ DWORD dwReserved);
        
         /**
         * IBindHost methods which are not implemented.
         */       
         HRESULT STDMETHODCALLTYPE MonikerBindToStorage( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
                { return E_NOTIMPL; }
        
         HRESULT STDMETHODCALLTYPE MonikerBindToObject( 
            /* [in] */ IMoniker __RPC_FAR *pMk,
            /* [in] */ IBindCtx __RPC_FAR *pBC,
            /* [in] */ IBindStatusCallback __RPC_FAR *pBSC,
            /* [in] */ REFIID riid,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppvObj)
                { return E_NOTIMPL; }
        

    protected: 
        ScriptSite(Processor * xtl);

        virtual void finalize();

         // hide these (not implemented)

        ScriptSite(){}
        ScriptSite( const ScriptSite &);
        void operator =( const ScriptSite &);

    private:

        WProcessor  _xtl;
        RString     _lastError;
};


#endif _XTL_ENGINE_SCRIPTSITE

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
