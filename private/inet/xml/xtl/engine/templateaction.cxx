/*
 * @(#)TemplateAction.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL TemplateAction object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "core/util/hashtable.hxx"
#include "xql/query/absolutequery.hxx"
#include "xtlkeywords.hxx"
#include "processor.hxx"
#include "copyaction.hxx"
#include "evalaction.hxx"
#include "getitemaction.hxx"
#include "nodenameaction.hxx"
#include "ifaction.hxx"
#include "repeataction.hxx"
#include "scriptaction.hxx"
#include "usetemplatesaction.hxx"


DEFINE_CLASS_MEMBERS(TemplateAction, _T("TemplateAction"), GetItemAction);



/*  ----------------------------------------------------------------------------
    newTemplateAction()

    Public "static constructor"

    @param tmplOuter    -   The template's outer lexical scope
    @param e            -   The XTL element for this template

    @return             -   Pointer to new TemplateAction object
*/

TemplateAction *
TemplateAction::newTemplateAction(TemplateAction * tmplOuter, Element * e, Query * qy)
{
    return new TemplateAction(tmplOuter, e, qy);
}


/*  ----------------------------------------------------------------------------

    newTemplateAction()

    Public "static constructor"

    @param tmplOuter    -   The template's outer lexical scope
    @param e            -   The XTL element for this template

    @return             -   Pointer to new TemplateAction object
*/

TemplateAction *
TemplateAction::newChooseAction(TemplateAction * tmplOuter, Element * e)
{
    return new TemplateAction(tmplOuter, e, null, true);
}        

/*  ----------------------------------------------------------------------------
    TemplateAction()

    Protected constructor

    @param tmplOuter    -   The template's outer lexical scope
    @param e            -   The XTL element for this template

*/

TemplateAction::TemplateAction(TemplateAction * tmplOuter, Element * e, Query * qy, bool fIsChoose) : GetItemAction(e)
{
    _tmplOuter = tmplOuter;
    _qy = qy;
    _fIsChoose = fIsChoose;
}
    

/*  ----------------------------------------------------------------------------
    finalize()

    Release all references to other objects.
*/

void 
TemplateAction::finalize()
{
    _tmplOuter = null;
    _table = null;
    _templates = null;
    _actions = null;
    super::finalize();
}


/*  ----------------------------------------------------------------------------
    execute()

    @param state        -   The current action state
    @param xtl          -   The XTL processor

    @return             -   The new state of the action.
*/

void
TemplateAction::execute(int state, Processor * xtl)
{
    Processor::ElementSource eSource;

    switch(state)
    {
    case 0:  
        push(xtl);

        if (_fCopy)
        {
            // Copy the current input element
            xtl->beginElement(Processor::CODEELEMENT, Element::ANY, null);
            eSource = Processor::CODEELEMENT;
        }
        else
        {
            eSource = Processor::PARENTELEMENT;
            goto ProcessChildren;
        }

        // fall through

    case Processor::PROCESS_ATTRIBUTES:
        // Push a frame that copies the attributes

        if (xtl->pushAction(Processor::ENUMATTRIBUTES, Processor::CURRENTDATA, eSource) == Processor::ACTION_OK)
        {
            break;
        }
        else
        {
            // Move to next state

            xtl->setState(Processor::PROCESS_CHILDREN);
        }

        // fall through
    
    case Processor::PROCESS_CHILDREN:
        eSource = Processor::CODEELEMENT;

ProcessChildren:
        // Push a frame that copies the children

        if (xtl->pushAction(Processor::ENUMELEMENTS, Processor::CURRENTDATA, eSource) == Processor::ACTION_OK)
        {
            break;
        }

        // fall through 

    default:  
        if (_fCopy)
        {
            xtl->endElement();
        }
        pop(xtl);
    }
}


/*  ----------------------------------------------------------------------------
    push()

    @param xtl          -   The XTL processor
*/

void
TemplateAction::push(Processor * xtl)
{
    // Setup the template frame for this template
    xtl->pushTemplate(this);

    xtl->setState(Processor::PROCESS_CHILDREN);

    xtl->setForceEndTag(_fForceEndTag);

    // executing - Don't increment until after pushTemplate because it can throw an E_PENDING
    // exception.

    _cExecuting++;
}


/*  ----------------------------------------------------------------------------
    pop()

    @param xtl          -   The XTL processor
*/

void
TemplateAction::pop(Processor * xtl)
{
    // executing
    _cExecuting--;

    xtl->setState(0);

    // Setup the template frame for this template
    xtl->popTemplate();
}


/*  ----------------------------------------------------------------------------
    getSelectQuery()

    @return             -   The query for this template.  
*/

Query *         
TemplateAction::getSelectQuery()
{
    return null;
}   



/*  ----------------------------------------------------------------------------
    getAction()

    @return             -   The actions for this template.
*/

Action * 
TemplateAction::getAction(int i)
{
    if (_actions && i >= 0 && i < _cActions)
    {
        return (*_actions)[i];
    }
    return null;
}


/*  ----------------------------------------------------------------------------
    getTemplate() 

    Find a template that matches an XML element
  
    @param e            -   an XML element

    @return             -   The template for this element
*/


TemplateAction *
TemplateAction::getTemplate(QueryContext * inContext, Element * e)
{
    TemplateAction * tmpl1, *tmpl2, *tmpl;
    int id1, id2;
    int i1, i2;
    Vector * v;
    Name * name;
    Atom * atom;

    i1 = -1;

    if (_table)
    {
        name = e->getTagName();
        if (name)
        {
            atom = name->getName();
            v = (Vector *) _table->get(atom);
            if (v)
            {
                i1 = v->size() - 1;
            }
        }
    }

    i2 = _cTemplates - 1;

    if (i1 >= 0 && i2 >= 0)
    {
        tmpl1 = ICAST_TO(TemplateAction *, v->elementAt(i1--));
        tmpl2 = (* _templates)[i2--];

        while (true)
        {
            if (tmpl1->getId() > tmpl2->getId())
            {
                if (!tmpl1->_qy || tmpl1->_qy->contains(inContext,null, e))
                {
                    return tmpl1;
                }

                if (i1 <= 0)
                {
                    break;
                }

                tmpl1 = ICAST_TO(TemplateAction *, v->elementAt(i1--));
            }
            else
            {
                if (!tmpl2->_qy || tmpl2->_qy->contains(inContext,null, e))
                {
                    return tmpl2;
                }

                if (i2 <= 0)
                {
                    break;
                }         
                
                tmpl2 = (* _templates)[i2--];
            }

        }
    }

    while (i1 >= 0)
    {
        tmpl = ICAST_TO(TemplateAction *, v->elementAt(i1--));

        if (!tmpl->_qy || tmpl->_qy->contains(inContext,null, e))
        {
            return tmpl;
        }

    }

    while (i2 >= 0)
    {
        tmpl = (* _templates)[i2--];

        if (!tmpl->_qy || tmpl->_qy->contains(inContext,null, e))
        {
            return tmpl;
        }
    }

    // Find first outer template with templates

    while (_tmplOuter)
    {
        if (_tmplOuter->_table || _tmplOuter->_cTemplates)
        {
            return _tmplOuter->getTemplate(inContext, e);
        }

        _tmplOuter = _tmplOuter->_tmplOuter;
    }

    return null;
}


/*  ----------------------------------------------------------------------------
    compile()

    Compiles a TemplateAction by walking XTL element tree and creating the necessary
    action objects.

    @return             -   none
*/

bool
TemplateAction::compile(Processor * xtl, String * defaultLang)
{
    Element * e = getElement();  
    HANDLE      hNext;
    Element *   eNext;
    Name *      nm;
    Atom *      ns;
    Atom *      gi;
    int         type;
    Query *     qy;
    // Only look for a language if this is a real xsl template.  If _fCopy is true then
    // this is a root element that is functioning as a template.  In this case, use the default
    // language.
    String *    language = _fCopy ? defaultLang : getLanguage(defaultLang);
    Action *    action;

    compileQuery(xtl, language);

    eNext = e->getFirstChild(&hNext);

    while (eNext)
    {
        type = eNext->getType();

        if (type == Element::ELEMENT)
        {
            nm = eNext->getTagName();
            ns = nm->getNameSpace();
            action = null;

            if (XTLKeywords::IsXSLNS(ns))
            {
                xtl->pushNamespace(eNext);

                gi = nm->getName();
                if (gi == XTLKeywords::s_atomDefineTemplateSet)
                {
                     action = Action::newAction(eNext);
                     addAction(action);
                     compileTemplates(xtl, eNext,language);
                }
                else if (gi == XTLKeywords::s_atomScript)
                {
                    action = ScriptAction::newScriptAction(eNext);
                    addAction(action);
                    action->compile(xtl, language);
                }

                xtl->popNamespace(eNext);

            }

            if (!action)
            {          
                compileBody(xtl, eNext, nm, language);
            }
        }

        eNext = e->getNextChild(&hNext);
    }


    hashTemplates();
    return true;
}


void
TemplateAction::compileBody(Processor * xtl, Element * e, Name * nm, String * language)
{
    int         type;
    Atom *      ns;
    Action *    action;
    Element *   eNext;
    HANDLE      hNext;
    bool        done = false;

    // BUGBUG - The keywords should be in a hashtable so all of these if's can be eliminated.
    // The hashtable should contain a pointer to the function used to create the action.

    xtl->pushNamespace(e);

    ns = nm->getNameSpace();

    if (XTLKeywords::IsXSLNS(ns))
    {
        if (_fIsChoose)
        {
            action = compileChoose(xtl, e, nm, language);
        }
        else
        {
            action = compileTemplate(xtl, e, nm, language);
        }

        if (action)
        {
            addAction(action);
            done = action->compile(xtl, language);
        }
    }

    if (!done)
    {
        eNext = e->getFirstChild(&hNext);
        while (eNext)
        {
            type = eNext->getType();

            if (type == Element::ELEMENT)
            {
                nm = eNext->getTagName();
                compileBody(xtl, eNext, nm, language);
            }

            eNext = e->getNextChild(&hNext);
        }
    }

    xtl->popNamespace(e);
}


Action *
TemplateAction::compileTemplate(Processor * xtl, Element * e, Name * nm, String * language)
{
    Action *    action;
    Atom *      gi = nm->getName();

    // BUGBUG - The keywords should be in a hashtable so all of these if's can be eliminated.
    // The hashtable should contain a pointer to the function used to create the action.

    if (gi == XTLKeywords::s_atomForEach)
    {
        action = RepeatAction::newRepeatAction(this, e);
    }
    else if (gi == XTLKeywords::s_atomValueOf)
    {
        action = GetItemAction::newGetItemAction(e);
    }
    else if (gi == XTLKeywords::s_atomApplyTemplates)
    {
        action = UseTemplatesAction::newUseTemplatesAction(this, e);
    }
    else if (gi == XTLKeywords::s_atomChoose)
    {
        action = TemplateAction::newChooseAction(this, e);
    }
    else if (gi == XTLKeywords::s_atomIf)
    {
        action = IfAction::newIfAction(this, e);
    }
    else if (gi == XTLKeywords::s_atomEval)
    {
        action = EvalAction::newEvalAction(e);
    }
    else if (gi == XTLKeywords::s_atomNodeName)
    {
        action = NodeNameAction::newNodeNameAction(e);
    }
    else if (gi == XTLKeywords::s_atomCopy)
    {
        action = CopyAction::newCopyAction(e, Element::ANY);
    }
    else if (gi == XTLKeywords::s_atomAttribute)
    {
        action = CopyAction::newCopyAction(e, Element::ATTRIBUTE);
    }
    else if (gi == XTLKeywords::s_atomElement)
    {
        action = CopyAction::newCopyAction(e, Element::ELEMENT);
    }
    else if (gi == XTLKeywords::s_atomEntityRef)
    {
        action = CopyAction::newCopyAction(e, Element::ENTITYREF);
    }
    else if (gi == XTLKeywords::s_atomCDATA)
    {
        action = CopyAction::newCopyAction(e, Element::CDATA);
    }
    else if (gi == XTLKeywords::s_atomComment)
    {
        action = CopyAction::newCopyAction(e, Element::COMMENT);
    }
    else if (gi == XTLKeywords::s_atomPI)
    {
        action = CopyAction::newCopyAction(e, Element::PI);
    }
    else if (gi == XTLKeywords::s_atomDoctype)
    {
        action = CopyAction::newCopyAction(e, Element::DOCTYPE);
    }            
    else 
    {
        Processor::Error(XSL_PROCESSOR_UNEXPECTEDKEYWORD, e->getNameDef());
    }

    return action;
}


Action *
TemplateAction::compileChoose(Processor * xtl, Element * e, Name * nm, String * language)
{
    Action *  action;
    Atom *      gi = nm->getName();

    // BUGBUG - The keywords should be in a hashtable so all of these if's can be eliminated.
    // The hashtable should contain a pointer to the function used to create the action.

    // BUGBUG - If otherwise appears, it must be the last keyword in the choose block.

    // Make xsl:otherwise an if so that it exits its parent's template after it executes.
    // BUGBUG - should the exitTemplate bit be on xsl:if or xsl:template?

    if (gi == XTLKeywords::s_atomWhen || gi == XTLKeywords::s_atomOtherwise)
    {
        action = IfAction::newIfAction(this, e, true);
    }
    else 
    {
        Processor::Error(XSL_PROCESSOR_UNEXPECTEDKEYWORD, e->getNameDef());
    }

    return action;
}


void
TemplateAction::compileTemplates(Processor * xtl, Element * e, String * language)
{
    Element *   eNext;
    HANDLE      hNext;
    Name *      nm;
    Atom *      ns;
    Atom *      gi;
    int         type;
    TemplateAction * tmpl;
    Action * action;

    eNext = e->getFirstChild(&hNext);
    while (eNext)
    {
        type = eNext->getType();
        if (type == Element::ELEMENT)
        {
            nm = eNext->getTagName();
            ns = nm->getNameSpace();
            if (XTLKeywords::IsXSLNS(ns))
            {
                xtl->pushNamespace(eNext);

                gi = nm->getName();
                if (gi == XTLKeywords::s_atomTemplate) 
                {
                    tmpl = TemplateAction::newTemplateAction(this, eNext);
                    tmpl->compile(xtl, language);
                    addTemplate(tmpl);
                }
                else if (gi == XTLKeywords::s_atomScript)  
                {
                    action = ScriptAction::newScriptAction(eNext);
                    action->compile(xtl, language);
                }
                else 
                {
                    Processor::Error(XSL_PROCESSOR_UNEXPECTEDKEYWORD, eNext->getNameDef());
                }

                xtl->popNamespace(eNext);
            }
        }

        // BUGBUG - The only other elements inside of Templates should be Element::COMMENT and
        // whitespace.

        eNext = e->getNextChild(&hNext);
    }
}


void
TemplateAction::compileQuery(Processor * xtl, String * language)
{
    if (!_fIsChoose && !_qy)
    {
        _qy = xtl->compileQuery(getElement(), XTLKeywords::s_nmMatch);

        // BUGBUG _qy must suuport contains 
        // _qy->getFlags() & Query::SUPPORTS_CONTAINS
    }
}


void
TemplateAction::addAction(Action * action)
{
    int l;

    // TraceTag((tagXTLProcessor, "TemplateAction::addAction %s", (char *) AsciiText(action->toString())));

    if (!_actions)
    {
        _actions = new (DEFAULT_ACTIONS_SIZE) AAction;
    }                    

    l = _actions->length();

    if (_cActions >= l)
    {
        _actions = _actions->resize(2 * l);
    }

    (*_actions)[_cActions++] = action;

    // TraceTag((tagXTLProcessor, "TemplateAction::addAction %s", (char *) AsciiText(toString())));
}


void
TemplateAction::addTemplate(TemplateAction * tmpl)
{
    int l;

    if (!_templates)
    {
        _templates = new (DEFAULT_TEMPLATES_SIZE) ATemplateAction;
    }                    

    l = _templates->length();

    if (_cTemplates >= l)
    {
        _templates = _templates->resize(2 * l);
    }

    tmpl->setId(_cTemplates);
    (*_templates)[_cTemplates++] = tmpl;
}


void
TemplateAction::hashTemplates()
{
    int i, j, cTemplates, cAtoms;
    TemplateAction * tmpl;
    Query * qy;
    Atom * atom;

    if (!_table && _cTemplates >= MIN_TEMPLATES_FOR_HASHTABLE)
    {
        _table = Hashtable::newHashtable();
    }
    
    if (_table)
    {
        cTemplates = _cTemplates;
        Vector * v = new Vector(2);

        for (i = 0; i < cTemplates; i++)
        {
            // Add Templates to hash table

            tmpl = (*_templates)[i];
            qy = tmpl->_qy;

            if (qy)
            {
                qy->target(v);
                cAtoms = v->size();

                Assert(cAtoms >= 1);

                for (j = 0; j < cAtoms; j++)
                {
                    atom = (Atom *) v->elementAt(j);
                    if (atom)
                    {
                        Vector * templates = (Vector *) _table->get(atom);
                        if (!templates)
                        {
                            templates = new Vector(DEFAULT_TEMPLATES_SIZE);
                            _table->put(atom, templates);
                        }
    
                        templates->addElement(tmpl);
                        (*_templates)[i] = null;
                        _cTemplates--;
                    }                    
                }
        
                v->removeAllElements();
            }
        }

        if (cTemplates != _cTemplates)
        {
            // Compact the default templates
            
            if (_cTemplates)
            {

                ATemplateAction * templates = new (_cTemplates) ATemplateAction;

                i = 0;
                j = 0;
                while(j < _cTemplates)
                {
                    tmpl = (*_templates)[i++];
                    if (tmpl)
                    {
                        (*templates)[j++] = tmpl;
                    }
                }

                _templates = templates;
            }
            else
            {
                _templates = null;
            }
        }
        else
        {
            _table = null;
        }
    }

}


#if DBG == 1
/**
 * Retrieves the string representation of this query.
 * @return a string representing the query.
 */

String * 
TemplateAction::toString()
{
    StringBuffer * sb = StringBuffer::newStringBuffer();
    int i;
    Action * action;

    sb->append(super::toString());
    sb->append(_T("("));
    if (_actions)
    {
        i = 0;
        while (true)
        {
            action = (*_actions)[i++];
            sb->append(action->toString());
            if (i < _cActions)
            {
                sb->append(_T(","));
            }
            else
            {
                break;
            }
        }
    }
    sb->append(_T(")"));
    return sb->toString();
}
#endif

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
