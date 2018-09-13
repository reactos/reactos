/*
 * @(#)XTLKeywords.cxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of XTL XTLKeywords object
 * 
 */

#include "core.hxx"
#pragma hdrstop

#include "xtlkeywords.hxx"
#include "copyaction.hxx"
#include "processor.hxx"

// XTL Keywords

TCHAR * XTLKeywords::s_pszNSXSL        = _T("http://www.w3.org/TR/WD-xsl");
TCHAR * XTLKeywords::s_pszNSXSLTemp    = _T("uri:xsl");  //BUGBUG - This should be removed or renamed urn:xsl, It's certainly more convenient than the above.
TCHAR * XTLKeywords::s_pszPrefix       = _T("xsl");

SRAtom XTLKeywords::s_atomNSXSL;
SRAtom XTLKeywords::s_atomNSXSLTemp;
SRAtom XTLKeywords::s_atomPrefix;


SRAtom XTLKeywords::s_atomStylesheet;
SRAtom XTLKeywords::s_atomDefineTemplateSet;
SRAtom XTLKeywords::s_atomTemplate;
SRAtom XTLKeywords::s_atomWhen;
SRAtom XTLKeywords::s_atomOtherwise;      // bugbug - This is completely unnecessary because it is equivalent to <xsl:case> and <xsl:template>
SRAtom XTLKeywords::s_atomForEach;
SRAtom XTLKeywords::s_atomApplyTemplates;
SRAtom XTLKeywords::s_atomChoose;
SRAtom XTLKeywords::s_atomIf;
SRAtom XTLKeywords::s_atomValueOf;
SRAtom XTLKeywords::s_atomCopy;
SRAtom XTLKeywords::s_atomElement;
SRAtom XTLKeywords::s_atomAttribute;
SRAtom XTLKeywords::s_atomText;
SRAtom XTLKeywords::s_atomCDATA;
SRAtom XTLKeywords::s_atomEntityRef;
SRAtom XTLKeywords::s_atomPI;
SRAtom XTLKeywords::s_atomComment;
SRAtom XTLKeywords::s_atomDoctype;
SRAtom XTLKeywords::s_atomScript;
SRAtom XTLKeywords::s_atomEval;
SRAtom XTLKeywords::s_atomNodeName; 


SRName XTLKeywords::s_nmSelect;
SRName XTLKeywords::s_nmMatch;
SRName XTLKeywords::s_nmTest;
SRName XTLKeywords::s_nmOrderBy;    // removed from spec as part of new sort syntax
SRName XTLKeywords::s_nmName;
SRName XTLKeywords::s_nmLanguage;
SRName XTLKeywords::s_nmExpr;
SRName XTLKeywords::s_nmNoEntities;

// BUGBUG - These aren't in the spec but would be very useful in <xsl:getValue>
// Format is a format string to use when displaying the value
// Default is a default value to use if no data is present - Should this be a function call??
SRName XTLKeywords::s_nmFormat;
SRName XTLKeywords::s_nmDefault;


XTLKeywords::KeywordInfo XTLKeywords::s_ki[] =
{
    {_T("stylesheet"), (Object **) &s_atomStylesheet, true},
    {_T("define-template-set"), (Object **) &s_atomDefineTemplateSet, true},
    {_T("template"), (Object **) &s_atomTemplate, true},
    {_T("when"), (Object **) &s_atomWhen, true},
    {_T("otherwise"), (Object **) &s_atomOtherwise, true},
    {_T("for-each"), (Object **) &s_atomForEach,true},
    {_T("apply-templates"), (Object **) &s_atomApplyTemplates, true},
    {_T("choose"), (Object **) &s_atomChoose, true},
    {_T("if"), (Object **) &s_atomIf, true},
    {_T("value-of"), (Object **) &s_atomValueOf, true},
    {_T("copy"), (Object **) &s_atomCopy, true}, 
    {_T("element"), (Object **) &s_atomElement, true},
    {_T("attribute"), (Object **) &s_atomAttribute, true},
    {_T("text"), (Object **) &s_atomText, true},
    {_T("cdata"), (Object **) &s_atomCDATA, true},
    {_T("entity-ref"), (Object **) &s_atomEntityRef, true},
    {_T("pi"), (Object **) &s_atomPI, true},
    {_T("comment"), (Object **) &s_atomComment, true},
    {_T("doctype"), (Object **) &s_atomDoctype, true},
    {_T("script"), (Object **) &s_atomScript, true},
    {_T("eval"), (Object **) &s_atomEval, true},
    {_T("node-name"), (Object **) &s_atomNodeName, true},
    {_T("select"), (Object **) &s_nmSelect, false},
    {_T("match"), (Object **) &s_nmMatch, false},
    {_T("test"), (Object **) &s_nmTest, false},
    {_T("order-by"), (Object **) &s_nmOrderBy, false},
    {_T("name"), (Object **) &s_nmName, false},
    {_T("language"), (Object **) &s_nmLanguage, false},
    {_T("expr"), (Object **) &s_nmExpr, false},
    {_T("format"), (Object **) &s_nmFormat, false},
    {_T("default"), (Object **) &s_nmDefault, false},
    {_T("no-entities"), (Object **) &s_nmNoEntities, false},
    {null, null, false},
};

extern CSMutex * g_pMutex;

void
XTLKeywords::classInit()
{
    NameSpace * ns;
    KeywordInfo * pki;
    Name * nm;
    Atom * atom;

    if (!Processor::s_strDefaultLanguage)
    {
        MutexLock lock(g_pMutex);
#ifdef RENTAL_MODEL
        Model model(MultiThread);
#endif
        TRY
        {
            // check if it is still null in case an other thread entered first...
            if (!Processor::s_strDefaultLanguage)
            {
                s_atomNSXSL = Atom::create(s_pszNSXSL);
                s_atomNSXSLTemp = Atom::create(s_pszNSXSLTemp);
                s_atomPrefix = Atom::create(s_pszPrefix);

                for (pki = s_ki; pki->_pszKeyword; pki++)
                {
                    if (pki->_fIsKeyword)
                    {
                        // Elements use atoms
                        atom = Atom::create(pki->_pszKeyword);
                        * (SRAtom *) pki->_pObject = atom;
                    }
                    else
                    {
                        // Attributes use names
                        nm = Name::create(pki->_pszKeyword, _tcslen(pki->_pszKeyword), (Atom *) null);
                        * (SRName *) pki->_pObject = nm;
                    }

                }

                //  BUGBUG - These shouldn't be here!

                Processor::s_strDefaultLanguage = String::newString(_T("jscript"));
                Processor::s_actDefault = CopyAction::newCopyAction(null, Processor::CODEELEMENT, Element::ANY);
            }
        }
        CATCH
        {
            lock.Release();
#ifdef RENTAL_MODEL
            model.Release();
#endif
            Exception::throwAgain();
        }
        ENDTRY
    }
}


/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
