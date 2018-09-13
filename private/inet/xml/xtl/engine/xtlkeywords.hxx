/*
 * @(#)XTLKeywords.hxx 1.0 3/19/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * definition of XTL XTLKeywords object
 * 
 */


#ifndef XTL_ENGINE_XTLKEYWORDS
#define XTL_ENGINE_XTLKEYWORDS


DEFINE_CLASS(XTLKeywords);


/**
 * XTL XTLKeywords Class
 *
 */

class XTLKeywords 
{
    public:

        static void classInit();
        static bool IsXSLNS(Atom * ns) {return ns == s_atomNSXSL || ns == s_atomNSXSLTemp;}

        // Atoms
        static SRAtom s_atomNSXSL;
        static SRAtom s_atomNSXSLTemp;
        static SRAtom s_atomPrefix;


        // Element Atoms
        static SRAtom s_atomStylesheet;
        static SRAtom s_atomDefineTemplateSet;
        static SRAtom s_atomTemplate;
        static SRAtom s_atomWhen;
        static SRAtom s_atomOtherwise;
        static SRAtom s_atomForEach;
        static SRAtom s_atomRepeat;
        static SRAtom s_atomApplyTemplates;
        static SRAtom s_atomChoose;
        static SRAtom s_atomIf;
        static SRAtom s_atomValueOf;
        static SRAtom s_atomCopy; 
        static SRAtom s_atomElement;
        static SRAtom s_atomAttribute;
        static SRAtom s_atomText;
        static SRAtom s_atomCDATA;
        static SRAtom s_atomEntityRef;
        static SRAtom s_atomPI;
        static SRAtom s_atomComment;
        static SRAtom s_atomDoctype;
        static SRAtom s_atomScript;
        static SRAtom s_atomEval;
        static SRAtom s_atomNodeName;
        static SRAtom s_atomNoEntities;


        // Attribute Names
        static SRName s_nmSelect;
        static SRName s_nmMatch;
        static SRName s_nmTest;
        static SRName s_nmOrderBy;
        static SRName s_nmType;
        static SRName s_nmName;
        static SRName s_nmLanguage;
        static SRName s_nmExpr;

        // BUGBUG Proposed attributes
        static SRName s_nmFormat;
        static SRName s_nmDefault;
        static SRName s_nmNoEntities;
      
    private:

        struct KeywordInfo
        {
            TCHAR *     _pszKeyword;
            Object **   _pObject;
            unsigned    _fIsKeyword:1;
        };

        static TCHAR * s_pszNSXSL;
        static TCHAR * s_pszNSXSLTemp;
        static TCHAR * s_pszPrefix;

        static KeywordInfo  s_ki[];
};


#endif _XTL_ENGINE_XTLKEYWORDS

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
