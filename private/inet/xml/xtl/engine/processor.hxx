/* @(#)Processor.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of Processor object
 * 
 */


#ifndef _XTL_ENGINE_PROCESSOR
#define _XTL_ENGINE_PROCESSOR

#ifndef _XTL_ENGINE_TEMPLATEFRAME
#include "templateframe.hxx"
#endif

#ifndef _XTL_ENGINE_RUNTIME
#include "runtime.hxx"
#endif

#ifndef XTL_ENGINE_XTLPROCESSOR
#include "xtlprocessor.hxx"
#endif

class ActionFrame;
typedef _array<ActionFrame> AActionFrame;
typedef _reference<AActionFrame> RAActionFrame;

DEFINE_CLASS(Action);
DEFINE_CLASS(TemplateAction);
DEFINE_CLASS(XTLProcessorEvents);
DEFINE_CLASS(ScriptEngine);
DEFINE_CLASS(Processor);
DEFINE_STRUCT(IDispatch);

void TransformNode(IXMLDOMNode *pStyleSheet, IXMLDOMNode *pNode, IStream *pstm);


/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: xtl
 *
 */

class Processor : public GenericBase, public IXTLProcessor, public QueryContext
{
    friend class XTLKeywords;

    DECLARE_CLASS_MEMBERS_I1(Processor, GenericBase, IXTLProcessor);

    public:

        // IXTLProcessor interface

        virtual HRESULT STDMETHODCALLTYPE Init(IXMLDOMNode *pStyleSheet, IXMLDOMNode *pNode, IStream * pstm);
        virtual HRESULT STDMETHODCALLTYPE Execute(BSTR *errMsg);
        virtual HRESULT STDMETHODCALLTYPE Close();

        enum EnumAction
        {
            ENUMELEMENTS,
            ENUMATTRIBUTES,
            FINDTEMPLATE
        };

        enum QueryAction
        {
            CURRENTDATA,
            NEXTDATA
        };

        enum ElementSource
        {
            NOELEMENT,
            CODEELEMENT,
            DATAELEMENT,
            PARENTELEMENT
        };

        enum ActionResult
        {
            ACTION_OK,
            ACTION_NO_DATA,
            ACTION_NO_ELEMENTS
        };


        // Common Action states
        // BUGBUG - can these be merged with EnumAction?

        enum 
        {
            PROCESS_ATTRIBUTES = 1,
            PROCESS_CHILDREN = 2,
            END_ELEMENT = 3
        };

        TemplateAction *    compile(Element * e);
        static void         Error(ResourceID Resid, Object * o1 = null, Object * o2 = null);
        void                init(Element * eXTL, Element * eData, OutputHelper * out);
        void                init(TemplateAction * tmpl, Element * eData, OutputHelper * out);
        void                init(Element * eXTL, Element * eData, IStream * pstm);
        void                execute();
        void                close();


        ActionResult  pushAction(EnumAction eAction, QueryAction qAction, ElementSource eSource = PARENTELEMENT);
        ActionResult  pushAction(Action * action, Element * eXTLParent, EnumAction eAction, ElementSource eSource);

        void pushTemplate(TemplateAction * block, Element * eData);
        void pushTemplate(TemplateAction * block);
        void popTemplate() { Assert(_spTemplates > 0); _spTemplates--; _tmplframe = const_cast<TemplateFrame *>(_stkTemplates->getData()) + _spTemplates - 1;}
        void exitTemplate(); 

        ActionFrame * getActionFrame(int sp);
        ActionFrame * getActionFrame() {return  getActionFrame(_spActions - 1);}
        TemplateFrame * getTemplateFrame() {return _tmplframe;}
        TemplateFrame * getTemplateFrame(int idx);
        Action * getAction(Element * e) {return getTemplateFrame()->getAction(e);}
        virtual Element * getDataElement(int idx);
        Element * getDataElement() {return getTemplateFrame()->getData();}
        Query * getQuery() {return getTemplateFrame()->getQuery();}
        virtual Query * getQuery(int idx);
        TemplateAction * findTemplate() {return getTemplateFrame()->getTemplate(this);}
        Element * getCodeElement(); 
        Element * nextData();
        void setNextData(bool f) {_fNextData = f;}      
        void setState(int state);
        void setForceEndTag(bool fForceEndTag);

        IUnknown *getRuntimeObject();
        void initRuntimeObject();

        ScriptEngine * getScriptEngine(String * scriptName);
        void parseScriptText();
        void checkDocLoaded();

        void beginElement(ElementSource eSource, int type, String * name);
        void beginElement(int type, String * name, String * text, bool fNoEntities = false);
        void endElement();

        Query * compileQuery(Element * e, Name * nm);
        Query * compileOrderBy(Query * qy, Element * e);
        void    pushNamespace(Element * e) {_nsmgr->pushScope(CAST_TO(ElementNode *, e));}
        void    popNamespace(Element * e) {_nsmgr->popScope(e);}

        Element *getStyleElement() { return _eStyle; }

        // Debugging helper.  This allows writing messages to the output
        void print(String * text); 

        static Action * GetDefaultAction() {return s_actDefault;}

        void markReadOnly();
        void unmarkReadOnly();

#if DBG == 1
        virtual String * toString();
#endif

    protected: 
        virtual void finalize();



        Processor(){};

    private:

        Processor( const Processor &);
        void operator =( const Processor &);

        bool fireOnTransform();

        bool _fScriptReadOnlyMarked;
        Document * _pDocSrc;
        
        enum
        {
#if DBG == 1
            DEFAULT_STACK_SIZE = 4, 
#else
            DEFAULT_STACK_SIZE = 32, 
#endif
            MAX_STACK_SIZE = 1024 
        };

        /**
         * Current ActionFrame
         */

        ActionFrame *   _actframe;

        /**
         * ActionFrame stack
         */

        RAActionFrame    _stkActions;

        /**
         * Current TemplateFrame
         */

        TemplateFrame *   _tmplframe;


        /**
         * TemplateFrame stack
         */

        RATemplateFrame  _stkTemplates;

        /**
         * ActionFrame stack pointer
         */

        short            _spActions;


        /**
         * TemplateFrame stack pointer
         */

        short            _spTemplates;


        /**
         * Script engines that are hooked up
         */

// BUGBUG - The script engine shouldn't be stored in the processor.  It should be in the templateaction.
// otherwise, the processor can't be reused by multiple template's. The processor shouldn't hold the script
// engine unless it is actually executing the templateaction.

        RHashtable       _scriptEngines;


        RXTLProcessorEvents _xtlevents;

        /**
         * Runtime object which is passed to script code
         */

        RCXTLRuntimeObject _XRT;

        /**
         * Parser to use to compile XQL queries
         */

        RXQLParser         _xql;

        /**
         * Namespacemgr to use when compiling XQL queries
         */

        RNamespaceMgr      _nsmgr;

        /**
         * Various processor flags
         */

        unsigned        _fNextData:1;       // has nextData been called for this instruction cycle?
        unsigned        _fDocLoaded:1;      // is the document fully downloaded?

        static SRString s_strDefaultLanguage;

        static SRAction s_actDefault;

        Element *           _eStyle;

        RIDispatch          _dispOnTransformNode;
};


/**
 * An action that contains other actions and establishes a lexical scope for template lookup.
 *
 * Hungarian: actframe
 *
 */

class ActionFrame 
{
    friend class Processor;

    public:

        ActionFrame(){}
        ~ActionFrame();
        void operator =( const ActionFrame &);

        /**
         * Return true when the frame is done
         */

        bool execute(Processor * p);
        Action * getAction(Element * e);
        Action * getCurrentAction() { return _action; }
        Element * getElement() {return _eXTL;}
        int getType() {return _type - 1;}
        void setType(int type) {_type = type + 1;};
        int getParentType() {return _typeParent - 1;}
        void setParentType(int type) {_typeParent = type + 1;}
        void setState(int state) {_state = state;}

#if DBG == 1
        String * toString();
#endif

    protected: 
        Processor::ActionResult init(Processor * xtl, Action * action, Element * eXTLParent,  Processor::EnumAction eAction);
     
         // hide these (not implemented)
        ActionFrame( const ActionFrame &);

    private:

        Element * firstElement();
        Element * nextElement();

        /**
         * The parent XTL element.  The actionframe will process 
         * the this element's children.
         */

        // WAA - change RElement to ROElement
        Element        * _eXTLParent;

        /**
         * The XTL element to process.   This element is a
         * child or attribute of the _eXTLParent.
         */

        // WAA - change RElement to ROElement
        Element        * _eXTL;  

        /**
         * handle used when getting the next eXTL
         */

        HANDLE          _hXTL;

        union
        {
            struct
            {
                unsigned        _enumAction:4;
                unsigned        _type:5;
                unsigned        _typeParent:5;
                unsigned        _source:2;
                unsigned        _fBeginChildren:1;
                unsigned        _fHasChildren:1;
                unsigned        _fHasWSInside:1;
                unsigned        _fHasWSAfter:1;
                unsigned        _fParentHasWSInside:1;
                unsigned        _fForceEndTag:1;
            };
            unsigned _flags;
        };

         /**
         * The action for the XTL element
         */

        RAction         _action;


        /**
         * The action state
         */

        int             _state;    // BUGBUG - State can be removed - It is duplicated by enumAction?
        
        /**
         * The name to use for this element
         */

        RString        _name;
};

inline void Processor::setForceEndTag(bool fForceEndTag) 
{
    if (fForceEndTag && _spActions >= 2)
    {
        ActionFrame * parentframe = getActionFrame(_spActions - 2);
        Assert(parentframe);
        parentframe->_fForceEndTag = true;
    }
}

inline ActionFrame * Processor::getActionFrame(int sp) {return  const_cast<ActionFrame *>(_stkActions->getData()) + sp;}

inline Element * Processor::getCodeElement()
{
    return _actframe->getElement();
}

inline void Processor::setState(int state)
{
    _actframe->setState(state);
}

inline void Processor::exitTemplate()
{
    // State must be 0 for this instruction to stop executing

    _actframe->_state = 0;
    _actframe->_eXTLParent = null;
}

#if DBG == 1

extern TAG tagXTLProcessor;

#endif

#endif _XTL_ENGINE_PROCESSOR

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////