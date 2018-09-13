/*
 * @(#)ScriptEngine.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL ScriptEngine object
 * 
 */


#ifndef XTL_ENGINE_SCRIPTENGINE
#define XTL_ENGINE_SCRIPTENGINE


DEFINE_CLASS(Processor);
DEFINE_CLASS(ActiveScript);
DEFINE_CLASS(ActiveScriptParse);
DEFINE_CLASS(ScriptSite);
DEFINE_CLASS(ScriptEngine);


/**
 * The simplest XTL action
 *
 * Hungarian: scriptinfo
 *
 */


class ScriptEngine : public Base
{
    DECLARE_CLASS_MEMBERS(ScriptEngine, Base);

    public:

        static ScriptEngine * newScriptEngine(Processor * xtl, REFCLSID clsid);
        void addScriptText(String * str);
        void parseScriptText();
        void parseScriptText(Processor * xtl, String * text, VARTYPE vt, VARIANT * pvar);
        void close();

    protected: 

        ScriptEngine(Processor * xtl, REFCLSID clsid);

        virtual void finalize();

         // hide these (not implemented)

        ScriptEngine(){}
        ScriptEngine( const ScriptEngine &);
        void operator =( const ScriptEngine &);

    private:

        void throwError(HRESULT hr);


        enum 
        {
            DEFAULT_SCRIPTTEXT_SIZE = 512
        };

        _reference<IUnknown>    _unk;
	    RActiveScript	        _as;
	    RActiveScriptParse      _asp;
        RScriptSite             _site;
        RStringBuffer           _scriptText;
        WProcessor              _xtl;
};


#endif _XTL_ENGINE_SCRIPTENGINE

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
