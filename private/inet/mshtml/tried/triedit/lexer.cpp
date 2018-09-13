// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
/* copied from ..\htmed\lexer.cpp */

/*++

  Copyright (c) 1995 Microsoft Corporation

  File: lexer.cpp

  Abstract:
        Nitty Gritty Lexer stuff

  Contents:
      SetValueSeen()
      IsSingleOp()
      IsWhiteSpace()
      MapToken()
      FindEndTag()
      MakeSublang()
      SetLanguage()
      FindTable()
      FindTable()
      RemoveTable()
      MakeTableSet()
      GetToken()
      IfHackComment()
      FindServerScript()
      FindEndComment()
      FindEndEntity()
      FindEntityRef()
      FindValue()
      FindEndString()
      FindTagOpen()
      FindText()
      FindNextToken()
      GetTextHint()
      GetHint()
      GetTokenLength()
      GetValueTokenLength()
      IsElementName()
      IsAttributeName()
      IsIdentifier()
      IsUnknownID()
      IsNumber()
      CColorHtml::SetTable()
      CColorHtml::InitSublanguages()

  History:
      2/14/97   cgomes:   Created


--*/

#include "stdafx.h"

#include "resource.h"
#include "guids.h"
#include "token.h"
#include "table.h"
#include "lexer.h"

UINT FindClientScriptEnd(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token);

#undef ASSERT
#define ASSERT(b) _ASSERTE(b)
// HACK: we keep a copy of a ptr to the ASP table and sublang
// so we can do special behavior for ASP files
CTableSet* g_ptabASP = 0;
PSUBLANG g_psublangASP = 0;

PTABLESET g_arpTables[CV_MAX+1];

// NOTE: added to handle value tokens properly.
UINT GetValueTokenLength(LPCTSTR pchLine, UINT cbLen, UINT cbCur);

// mark state transition from value -> next attribute
inline int SetValueSeen(DWORD *plxs)
{
    if (*plxs & inValue)
    {
        *plxs &= ~inValue;
        *plxs |= inAttribute;
        return TRUE;
    }
    else
        return FALSE;
}

// REVIEW (walts) - need better way
inline void SetScriptLanguage(LPCTSTR pchLine, DWORD *plxs)
{
    LPCTSTR strJavaScript       = _T("javascript");
    LPCTSTR strVBScript         = _T("vbscript");
    // triedit's special language. Its set when we convert server-side scripts into
    // client-side scripts. Its a dummy language. if we find that as language, we
    // set in ServerASP. It is reset(removed) in FindNextToken().
    LPCTSTR strServerAsp        = _T("serverasp");

    // language attribute may have quotes around it.
    // if it does then advance past the first quote.
    //      ex. <SCRIPT LANGUAGE="VBScript">
    if(*pchLine == L'\"')
        pchLine++;

    if (_tcsnicmp(pchLine, strJavaScript, lstrlen(strJavaScript)) == 0)
    {
        *plxs &= ~inVBScript;
        *plxs &= ~inServerASP;
        *plxs |= inJavaScript;
    }
    else if (_tcsnicmp(pchLine, strVBScript, lstrlen(strVBScript)) == 0)
    {
        *plxs &= ~inJavaScript;
        *plxs &= ~inServerASP;
        *plxs |= inVBScript;
    }
    else if (_tcsnicmp(pchLine, strServerAsp, lstrlen(strServerAsp)) == 0)
    {
        *plxs &= ~inJavaScript;
        *plxs &= ~inVBScript;
        *plxs |= inServerASP;
    }
}

inline BOOL IsSingleOp(HINT hint)
{
    return ((hint >= tokOP_SINGLE) && (hint < tokOP_MAX));
};

inline BOOL IsWhiteSpace(TCHAR c)
{
    return _istspace(c);
};


// NOTE: Added to handle value tokens properly
inline IsValueChar(TCHAR ch)
{
    // REVIEW(cgomes): specify all the invalid value characters
    return ch != _T('<') && ch != _T('>');
};

////////////////////////////////////////////////////////////////////////////
//
// map parsed token to returned token

// left column must be in ascending order
static TOKEN _rgTokenMap[] =
{
    tokName,     tokSpace,
    tokNum,      tokSpace,
    tokParEnt,   tokSpace,
    tokResName,  tokSpace,
    0,           0
};

static TOKEN MapToken(TOKEN tokClass, DWORD lxs)
{
    if (IsSingleOp((HINT)tokClass))
        return tokOp;
    else if ((tokClass == tokTag) && (lxs & inHTXTag))
        return tokSSS;
    for (int i = 0; (_rgTokenMap[i] != 0) && (_rgTokenMap[i] >= tokClass); i += 2)
    {
        if (_rgTokenMap[i] == tokClass)
            return _rgTokenMap[i + 1];
    }
    return tokClass;
}

////////////////////////////////////////////////////////////////////////////

UINT FindEndTag(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD *plxs, TXTB & token)
{
    ASSERT(pchLine);
    TCHAR szEnd[16];
    ELLEX * pellex = pellexFromTextState(*plxs);
    ASSERT(0 != pellex); // shouldn't be called with something other than special text state
    UINT cbCmp = 3 + pellex->cb; // length of end tag
    ASSERT(cbCmp < sizeof szEnd);
    _tcscpy(szEnd, _T("</"));
    _tcscat(szEnd, pellex->sz);
    _tcscat(szEnd, _T(">"));

    while (cbCur < cbLen)
    {
        if (_T('<') == pchLine[cbCur])
        {
            if ((cbLen - cbCur >= cbCmp) && (0 == _tcsnicmp(szEnd, &pchLine[cbCur], cbCmp)))
            {
                *plxs &= ~TEXTMASK; // special text modes are exclusive
                token.ibTokMac = cbCur;
                return cbCur;
            }
            else if ((cbCur + 1 < cbLen) && (_T('%') == pchLine[cbCur+1]))
            {
                *plxs |= inHTXTag;
                token.ibTokMac = cbCur;
                break;
            }
            else
                cbCur++;
        }
        else
            cbCur += _tclen(&pchLine[cbCur]);
    }
    token.ibTokMac = cbCur;
    return cbCur;
}

////////////////////////////////////////////////////////////////////////////

BOOL MakeSublang(PSUBLANG ps, UINT id, const TCHAR *strName, UINT nIdTemplate, CLSID clsid)
{
    int len;

    ASSERT( NULL != ps );

    ps->szSubLang = NULL;
    ps->lxsInitial = LxsFromSubLangIndex(id);
    ps->nIdTemplate = nIdTemplate;
    ps->clsidTemplate = clsid;

    if ((len = lstrlen(strName)) != 0)
    {
        LPTSTR szNew = new TCHAR [len+1];
        if (NULL != szNew)
        {
            _tcscpy(szNew,strName);
            ps->szSubLang = szNew;
            return TRUE;
        }
    }
    return FALSE;
}

// Set sublang and tableset array members,
// putting the default one in 0th position.
//
void SetLanguage(TCHAR * strDefault, PSUBLANG rgSublang,
                 PTABLESET pTab, UINT & index, UINT nIdTemplate, CLSID clsid)
{
    if (pTab != NULL)
    {
        int i;
        if (lstrcmp(strDefault, pTab->Name()) == 0)
            i = 0;
        else
            i = index;
        if (MakeSublang(rgSublang+i, i, pTab->Name(), nIdTemplate, clsid))
        {
            g_arpTables[i] = pTab;
            if (i)
                index++;
            else
                g_pTable = pTab;
        }
        else
            delete pTab;
    }
}

CTableSet * FindTable(CTableSet ** rgpts, TCHAR *strName)
{
    for (int n = 0; rgpts[n]; n++)
    {
        if (rgpts[n]->Name() == strName)
        //if (strcmp(rgpts[n]->Name(), strName) == 0)
            return rgpts[n];
    }
    return NULL;
}

CTableSet * FindTable(CTableSet ** rgpts, CTableSet * pts)
{
    for (int n = 0; rgpts[n]; n++)
    {
        if (rgpts[n] == pts)
            return rgpts[n];
    }
    return NULL;
}

void RemoveTable(CTableSet ** rgpts, CTableSet *pts)
{
    int n;
    for (n = 0; rgpts[n]; n++)
    {
        if (rgpts[n] == pts)
        {
            for(; rgpts[n]; n++)
                rgpts[n] = rgpts[n+1];
            return;
        }
    }
}

CTableSet * MakeTableSet(CTableSet ** /*rgpts*/, RWATT_T att, UINT nIdName)
{
    return new CStaticTableSet(att, nIdName);
}

////////////////////////////////////////////////////////////////////////
// GetToken()
//
UINT GetToken(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    ASSERT (cbCur < cbLen);
    if(cbCur > cbLen)
        return cbCur;

    UINT cbCount = 0;

    // init token
    token.tok = 0;

    // initialize location where token starts
    token.ibTokMin = cbCur;

    if (*plxs & inHTXTag)
        cbCount = FindServerScript(pchLine, cbLen, cbCur, plxs, token);
    else if (*plxs & inSCRIPT && !(*plxs & inTag) && !(*plxs & inServerASP))
    {
        // NOTE that we want to skip tokenizing scripts that are special to triedit
        // when we wrap server-side scripts in client-side scripts, we set a dummy
        // language as 'serverasp'. inServerASP is set in that case.
        cbCount = FindClientScriptEnd(pchLine, cbLen, cbCur, plxs, token);
    }
    else if (*plxs & inComment)  // in a comment
    {
        if (*plxs & inSCRIPT)
            *plxs |= inScriptText;
        COMMENTTYPE ct = IfHackComment(pchLine, cbLen, cbCur, plxs, token);
        if (ct == CT_METADATA)
        {
            // Treat as an element
            cbCount = FindNextToken(pchLine, cbLen, cbCur, plxs, token);
            // Remove inBangTag
            *plxs &= ~inBangTag;
        }
        else if (ct == CT_IECOMMENT)
            cbCount = token.ibTokMac;
        else
            cbCount = FindEndComment(pchLine, cbLen, cbCur, plxs, token);
    }
    else if (*plxs & INSTRING)  // in a string
        cbCount = FindEndString(pchLine, cbLen, cbCur, plxs, token);
    else
        cbCount = FindNextToken(pchLine, cbLen, cbCur, plxs, token);

    token.tokClass = MapToken(token.tokClass, *plxs);
    return cbCount;
}

///////////////////////////////////////////////////////////////////////////////////
// IfHackComment
//
// Probe ahead in the current line to see if we have what IE recognizes
// as the end of a comment ("->"). This does not conform to RFC 1866 or SGML,
// but suppports browser behavior. This lets us tolerate comments of the
// form: "<!--- whatever ->"
// (note how it ends)
//
// Returns a COMMENTTYPE enum.
//  0 if norma comment
//  1 if IE comment
//  -1 if METADATA comment
//
// Proper comments are scanned using FindEndComment().
//
COMMENTTYPE IfHackComment(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    token.tokClass = tokComment;
    while (cbCur+1 < cbLen)
    {
        if(_tcsnicmp(&pchLine[cbCur], _T("METADATA"), lstrlen(_T("METADATA"))) == 0)
        {
            token.ibTokMac = cbCur + 1; // include second dash??
            *plxs &= ~inComment;
            // Remove inBangTag
            *plxs &= ~inBangTag;
            *plxs |= inTag;
            return CT_METADATA; // METADATA
        }
        else if (pchLine[cbCur] == '-' && pchLine[cbCur + 1] == '>')
        {
            token.ibTokMac = cbCur + 1;
            *plxs &= ~inComment;
            *plxs &= ~inScriptText;
            return CT_IECOMMENT;
        }
        else
        {
            cbCur += _tclen(&pchLine[cbCur]);
        }
    }
    return CT_NORMAL;
}


UINT FindServerScript(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    int cb;

    // parse HTX start tag
    if (*pCurrent == _T('<') && (cbCur+1 < cbLen) && *(pCurrent+1) == '%')
    {
        token.tokClass = tokTag;
        token.tok = TokTag_SSSOPEN;
        token.ibTokMac = cbCur + 2;
        *plxs |= inHTXTag;
        return token.ibTokMac;
    }

    ASSERT(*plxs & inHTXTag); // should be in HTXTag state here

    if (*pCurrent == _T('%') && (cbCur+1 < cbLen) && *(pCurrent+1) == '>')
    {
        token.tok = TokTag_SSSCLOSE;
        token.tokClass = tokSSS; //tokTag;
        token.ibTokMac = cbCur + 2;
        *plxs &= ~inHTXTag;
        if (*plxs & inNestedQuoteinSSS)
            *plxs &= ~inNestedQuoteinSSS;
        return token.ibTokMac;
    }

    token.tokClass = tokSSS;

    while (cbCur < cbLen)
    {
        if (*pCurrent == _T('%') && (cbCur+1 < cbLen) && (*(pCurrent+1) == _T('>')))
            break;
        if (   *pCurrent == _T('"') 
            && *plxs&inTag
            && *plxs&inHTXTag
            && *plxs&inAttribute
            && *plxs&inString
            )
            *plxs |= inNestedQuoteinSSS;

        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }

    token.ibTokMac = cbCur;
    return cbCur;
}

///////////////////////////////////////////////////////////////////////////////////
// FindClientScriptEnd()
//
// HTMED CHANGE: Find the end of client script block
//
UINT FindClientScriptEnd(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    int cb;

    TCHAR rgEndScript[] = _T("</SCRIPT");
    int cchEndScript = (wcslen(rgEndScript) - 1);

    if( cbCur + cchEndScript < cbLen &&
        0 == _tcsnicmp(pCurrent, rgEndScript, cchEndScript))
    {
        token.tokClass = tokTag;
        token.tok = TokTag_END;
        *plxs &= ~inSCRIPT;
        *plxs |= inEndTag;
        token.ibTokMac = cbCur + 2;
        return token.ibTokMac;
    }

    token.tokClass = tokSpace;

    while (cbCur < cbLen)
    {
        if (*pCurrent == _T('<') && (cbCur+1 < cbLen) && (*(pCurrent+1) == _T('/')))
        {
            // Check if found end </SCRIPT
            if( cbCur + cchEndScript < cbLen &&
                0 == _tcsnicmp(pCurrent, rgEndScript, cchEndScript))
            {
                // Check if found end </SCRIPT
                break;
            }
        }
        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }

    token.ibTokMac = cbCur;
    return cbCur;
}

///////////////////////////////////////////////////////////////////////////////////
// FindEndComment()
//
// Find the end of comment ("--").
//
UINT FindEndComment(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    BOOL bEndComment = FALSE;
    int cb;

    ASSERT(*plxs & inComment); // must be in a comment now

    token.tokClass = tokComment;

    while (!bEndComment && cbCur < cbLen)
    {
        if (*pCurrent == _T('-'))  // check the character to see if it's the first "-" in "--"
        {
            pCurrent++;
            cbCur++;
            if ((cbCur < cbLen) &&
                (*pCurrent == _T('-'))) // we're possibly at the end, so search for the final "--" pair
            {
                bEndComment = TRUE;
            }
        }
        else
        {
            cb = _tclen(pCurrent);
            cbCur += cb;
            pCurrent += cb;
        }
    }
    if (cbCur < cbLen)
    {
        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }

    token.ibTokMac = cbCur;

    // reset state if we reach end of comment
    if (bEndComment)
        *plxs &= ~inComment;

    return cbCur;
}

/////////////////////////////////////////////////////////////
// FindEndEntity()
//
// Find the end of the special character sequence (ends with ; or whitespace).
//
UINT FindEndEntity(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * /*plxs*/, TXTB & token)
{
    token.tokClass = tokEntity;
    int cb = GetTokenLength(pchLine, cbLen, cbCur);
    if (pchLine[cbCur + cb] == ';')
        cb++;
    token.ibTokMac = cbCur + cb;
    return token.ibTokMac;
}

/////////////////////////////////////////////////////////////
// Find an entity reference or non-entity ref, literal "&..."
//
UINT FindEntityRef(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * /*plxs*/, TXTB & token)
{
    ASSERT(cbCur < cbLen);
    ASSERT(pchLine[cbCur] == '&'); // must be on ERO

    cbCur++;
    if (cbCur == cbLen)
    {
NotEntity:
        token.tokClass = tokIDENTIFIER; // plain text
        token.ibTokMac = cbCur;
        return cbCur;
    }

    if (pchLine[cbCur] == '#')
    {
        // parse and check valid number
        if (!IsNumber(pchLine, cbLen, cbCur + 1, token))
            goto NotEntity;

        // must be <= 3 digits
        if (token.ibTokMac - (cbCur + 1) > 3)
            goto NotEntity;

        // validate range
        TCHAR szNum[4];
        _tcsncpy(szNum, &pchLine[cbCur + 1], 3);
        if (_tcstoul(szNum, 0, 10) > 255)
            goto NotEntity;

        // we now have a valid numeric entity ref

        token.tokClass = tokEntity;
        cbCur = token.ibTokMac;

        // scan for end of entity ref

        // scan rest of alphanumeric token
        // REVIEW: Is this correct? IE 4.40.308 behaves this way
        while ((cbCur < cbLen) && IsCharAlphaNumeric(pchLine[cbCur]))
            cbCur++;

        // scan delimiter
        if (cbCur < cbLen)
            cbCur++;
        token.ibTokMac = cbCur;
        return cbCur;
    }
    else if (!IsCharAlpha(pchLine[cbCur]))
    {
        goto NotEntity;
    }
    else
    {
        // parse and check entity name
        UINT nLen = GetTokenLength(pchLine, cbLen, cbCur);
        if (!g_pTable->FindEntity(&pchLine[cbCur], nLen))
            goto NotEntity;

        cbCur += nLen;
        // eat delimiter if necessary
        if ((cbCur < cbLen) &&
            (pchLine[cbCur] == ';' || IsWhiteSpace(pchLine[cbCur])))
            cbCur++;
        token.tokClass = tokEntity;
        token.ibTokMac = cbCur;
        return cbCur;
    }
}


/////////////////////////////////////////////////////////////
// FindEndValue
// Find the end of an unquoted value.
//
// Scan for whitespace or end if tag
//
UINT FindValue(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    ASSERT(cbCur < cbLen);

    do
    {
        cbCur++;
    } while ( cbCur < cbLen &&
        !IsWhiteSpace(pchLine[cbCur]) &&
        pchLine[cbCur] != '>' );

    token.tokClass = tokValue;
    token.ibTokMac = cbCur;

    // switch from value to attribute
    *plxs &= ~inValue;
    *plxs |= inAttribute;

    return cbCur;
}

/////////////////////////////////////////////////////////////
// FindEndString()
// Find the end of the string.
// Should only be called when we are in the string mode already.
//
UINT FindEndString (LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    int cb;
    BOOL bInString = TRUE;
    TCHAR chDelim;

    ASSERT (*plxs & INSTRING); // must be in a string now

    token.tokClass = tokString;
    chDelim = (*plxs & inStringA) ? _T('\'') : _T('"');

    while (bInString && cbCur < cbLen)
    {
        if (*pCurrent == chDelim)
        {
            *plxs &= ~INSTRING;
            bInString = FALSE;
            SetValueSeen(plxs);
        }
        else if (*pCurrent == _T('<') &&
            cbCur+1 < cbLen &&
            *(pCurrent+1) == _T('%'))
        {
            *plxs |= inHTXTag;
            break;
        }
        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }
    token.ibTokMac = cbCur;
    return cbCur;
}

//////////////////////////////////////////////////////////////////
//
UINT FindTagOpen(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    ASSERT(pchLine[cbCur] == '<');
    token.tokClass = tokTag;
    *plxs &= ~inScriptText;     // turn off script coloring when inside tags
    cbCur++;

    if (cbCur == cbLen)
    {
        *plxs |= inTag;
    }
    else
    {
#ifdef NEEDED // copied from htmed\lexer.cpp
        //
        // HTMED CHANGE:
        // REVIEW(cgomes): Figure out if I should turn off inSCRIPT in any of the
        // following cases.  Right now I only do it for the </ case.
        //
#endif //NEEDED         
        switch (pchLine[cbCur])
        {
        case '!': // MDO - Markup Declaration Open
            cbCur++;
            *plxs |= inBangTag;
            token.tok = TokTag_BANG;
            break;

        case '/': // End tag
            cbCur++;
            *plxs |= inEndTag;
            token.tok = TokTag_END;
#ifdef NEEDED // copied from htmed\lexer.cpp
            // HTMED CHANGE:
            // REVIEW(cgomes): Colorizer bug: it never removes the inSCRIPT state
            //  This removes the inSCRIPT in the case <SCRIPT <BODY>
            //  in this case <BODY is in error.
            //
            *plxs &= ~inSCRIPT;
#endif //NEEDED         
            break;

        // REVIEW: PI is SGML -- not in HTML, but might be added
        case '?': // PI - Processing Instruction
            cbCur++;
            *plxs |= inPITag;
            token.tok = TokTag_PI;
            break;

        case '%': // HTX -- ODBC server HTML extension
            cbCur++;
            *plxs |= inHTXTag;
            token.tok = TokTag_SSSOPEN;
            break;

        default: // Tag
            if (IsCharAlpha(pchLine[cbCur]))
            {
                *plxs |= inTag;
                token.tok = TokTag_START;
            }
            else
                token.tokClass = tokIDENTIFIER; // NOT a TAG
            break;
        }
    }
    token.ibTokMac = cbCur;
    return cbCur;
}

//////////////////////////////////////////////////////////////////
//  FindText
//  Scan a token of text
//      NOTE DO NOT MODIFY this function, mainly b/c the side effects
//              will be hard to find, and will break the way
//              that everything works.
//
UINT FindText(LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token)
{
    //BOOL fExtraSpace = FALSE;
    //int cSpace = 0;

    ASSERT (cbCur < cbLen);

    token.tokClass = tokIDENTIFIER;

    //if (pchLine[cbCur] == ' ' && !fExtraSpace)
    //  fExtraSpace = TRUE;
    cbCur += _tclen(&pchLine[cbCur]);
    while (cbCur < cbLen)
    {
        switch (pchLine[cbCur])
        {
        case _T('\0'):
        case _T('\n'):
        case _T('<'):
        case _T('&'):
            //if (cSpace > 0) // found extra spaces so remember them somewhere
            goto ret;
            break;
        //case _T(' '):
        //  if (!fExtraSpace)
        //      fExtraSpace = TRUE;
        //  else
        //      cSpace++;
        //  break;
        default:
            //if (cSpace > 0) // found extra spaces so remember them somewhere
            //cSpace = 0;
            //fExtraSpace = FALSE;
            break;
        }
        cbCur += _tclen(&pchLine[cbCur]);
    }

ret:
    token.ibTokMac = cbCur;
    return cbCur;
}

//////////////////////////////////////////////////////////////////
// FindNextToken()
//  Find the next token in the line
//
UINT FindNextToken(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    ASSERT (cbCur < cbLen);
    HINT hint;

    if (!(*plxs & INTAG)) // scanning text
    {
        if (*plxs & TEXTMASK)
        {
            if (*plxs & inCOMMENT)
                token.tokClass = tokComment;
            else
                token.tokClass = tokIDENTIFIER;
            // probe for end tag </comment>
            UINT cbEnd = FindEndTag(pchLine, cbLen, cbCur, plxs, token);
            if (cbEnd > cbCur) // parsed a nonzero-length token
            {
                return cbEnd;
            }
            //else fall through to normal processing
        }
        hint = GetTextHint(pchLine, cbLen, cbCur, plxs, token);
        switch (hint)
        {
        case HTA:
            // begin a tag
            return FindTagOpen(pchLine, cbLen, cbCur, plxs, token);

        case HEN:
            // scan an entity reference
            token.ibTokMac = FindEntityRef(pchLine, cbLen, cbCur, plxs, token);
            return token.ibTokMac;

        case EOS:
        case ONL:
            return token.ibTokMac;

        case ERR:
        default:
            // scan text as a single token
            // If the editor uses token info for more than coloring
            //   (e.g. extended selections), then this will need to
            //   return smaller chunks.
            if (*plxs & inSCRIPT)
                *plxs |= inScriptText;
            return FindText(pchLine, cbLen, cbCur, token);
            break;
        }

        return cbCur;
    }

    ASSERT(*plxs & INTAG); // must be in a tag here

    BOOL bError = FALSE;
    hint = GetHint(pchLine, cbLen, cbCur, plxs, token);
    switch (hint)
    {
    case HTE:
        // Tag end: remove all tag state bits
        *plxs &= ~TAGMASK;
        cbCur++;
        token.tokClass = tokTag;
        token.tok = TokTag_CLOSE;
        token.ibTokMac = cbCur;
        break;

    case HNU:
#if 0  // lexing HTML instance, not a DTD!
        if (!IsNumber(pchLine, cbLen, cbCur, token))
            bError = TRUE;
        if (SetValueSeen(plxs))
            token.tokClass = tokValue;
        break;
#else
        // fall through
#endif

    case HRN: // reserved name start: #
#if 1  // lexing HTML instance, not a DTD!
        // simple nonwhitespace stream
        if (!(*plxs & inValue))
            bError = TRUE;
        FindValue(pchLine, cbLen, cbCur, plxs, token);
        if (bError)
        {
            token.tokClass = tokSpace;
            bError = FALSE; //"corrected" the error
        }
#else
        cbCur++;
        if (cbCur == cbLen)
            token.tokClass = tokOp;
        else
        {
            if (IsIdChar(pchLine[cbCur]))
            {
                cbCur++;
                while (cbCur < cbLen && IsIdChar(pchLine[cbCur]))
                    cbCur++;
                token.tokClass = tokResName;
            }
            else
                token.tokClass = tokOp;
        }
        token.ibTokMac = cbCur;
        if (SetValueSeen(plxs))
            token.tokClass = tokValue;
#endif
        break;

    case HEP: // parameter entity: %
#if 1  // lexing HTML instance, not a DTD!
        goto BadChar;
#else
        cbCur++;
        if (cbCur == cbLen)
        {
            token.tokClass = tokOp;
            token.ibTokMac = cbCur;
        }
        else
        {
            if (IsIdChar(pchLine[cbCur]))
            {
                token.ibTokMac = FindEndEntity(pchLine, cbLen, cbCur, plxs, token);
                token.tokClass = tokParEnt;
            }
            else
            {
                token.ibTokMac = cbCur;
                token.tokClass = tokOp;
            }
        }
        if (SetValueSeen(plxs))
            token.tokClass = tokValue;
#endif
        break;

    // ported HTMED change (walts) -- handle some chars as valid start char for attribute values.
    case HAV:
        {
        if (!(*plxs & inTag) || !SetValueSeen(plxs))
            goto BadChar;   // not in tag or attribute value.

        int iTokenLength = GetValueTokenLength(pchLine, cbLen, cbCur);
        token.ibTokMac = token.ibTokMin + iTokenLength;
        token.tokClass = tokValue;
        break;
        }
    // ported HTMED change (walts) -- handle some chars as valid start char for attribute values.

    case HKW:  // identifier
        {
            int iTokenLength = GetTokenLength(pchLine, cbLen, cbCur);
            token.ibTokMac = token.ibTokMin + iTokenLength;
            token.tokClass = tokName;
            //FUTURE: Don't scan attributes in an end tag
            if (*plxs & (inTag|inEndTag))
            {
                if (*plxs & inAttribute)
                {
                    IsAttributeName(pchLine, cbCur, iTokenLength, token);
                    // don't change attribute/value state here
                    // we only look for values after we've seen "=" in case OEQ below

                    // REVIEW(cgomes): what if more attributes follow
                    // the SPAN??
                    // if found STARTSPAN then pretend I am not in a tag
                    if(token.tok == TokAttrib_STARTSPAN)
                        *plxs &= ~(inTag | inAttribute);
                    // if found ENDSPAN then goback to comment state
                    else if(token.tok == TokAttrib_ENDSPAN)
                    {
                        *plxs &= ~(inTag | inAttribute);
                        *plxs |= inBangTag | inComment;
                    }
                }
                else if (SetValueSeen(plxs))
                {
                    // REVIEW (walts)
                    // Handle the client side script language detection here for the
                    // following case (language attribute value is NOT wrapped by quotes.)
                    // <SCRIPT LANGUAGE=VBScript>
                    if (*plxs & inSCRIPT)
                    {
                        SetScriptLanguage(&pchLine[cbCur], plxs);
                    }

                    //
                    // REVIEW(cgomes): It seems that any non-white space character
                    //      is valid for non-quoted attribute values.
                    //      Problem is that GetTokenLength is used to determine
                    //      the token length, which works great non-values,
                    //      but sucks egss for values.
                    //      I use GetValueTokenLength here to get the length
                    //      of value token.  GetValueTokenLength will not
                    //      stop till it hits a white space character.
                    //

                    iTokenLength = GetValueTokenLength(pchLine, cbLen, cbCur);
                    token.ibTokMac = token.ibTokMin + iTokenLength;
                    token.tokClass = tokName;

                    token.tokClass = tokValue;
                }
                else
                {
                    IsElementName(pchLine, cbCur, iTokenLength, token);
                    // look for attributes
                    *plxs |= inAttribute;
                    // set content state
                    if (*plxs & inTag)
                        *plxs |= TextStateFromElement(&pchLine[token.ibTokMin], iTokenLength);
                    else if ((*plxs & inEndTag) && (*plxs & TEXTMASK))
                        *plxs &= ~TextStateFromElement(&pchLine[token.ibTokMin], iTokenLength);
                    else if ((*plxs & inEndTag) && (*plxs & inSCRIPT))
                        *plxs &= ~(inSCRIPT | inScriptText | inServerASP/* | inVBScript | inJavaScript*/);
                }
            }
            else if (*plxs & inBangTag)
            {
                // FUTURE: other <!...> items like "HTML", "PUBLIC"? -- nice for DTDs
                //   Use a RW table for it if we do

                // recognize <!DOCTYPE ...>  as 'element'
                if ((iTokenLength == 7) &&
                    (0 == _tcsnicmp(&pchLine[cbCur], _T("doctype"), 7)))
                    token.tokClass = tokElem;
            }
            break;
        }

    case HST:  // string "..."
        *plxs |= inString;
        goto String;

    case HSL:  // string alternate '...'
        *plxs |= inStringA;
String:
        cbCur++;
        token.ibTokMac = FindEndString(pchLine, cbLen, cbCur, plxs, token);
        SetValueSeen(plxs);
        // Handle the client side script language detection here for the
        // following case (language attribute value is wrapped by quotes.)
        // <SCRIPT LANGUAGE="VBScript">
        if((*plxs & inSCRIPT) && (*plxs & inAttribute))
        {
            SetScriptLanguage(&pchLine[cbCur], plxs);
        }
        break;

    case HWS: // tag whitespace
        do
        {
            cbCur++;
        } while (cbCur < cbLen && IsWhiteSpace(pchLine[cbCur]));
        token.tokClass = tokSpace;
        token.ibTokMac = cbCur;
        break;

    case OEQ:
        // GetHint has set token info
        if (*plxs & inAttribute)
        {
            // start looking for values
            *plxs &= ~inAttribute;
            *plxs |= inValue;
        }
        else
            goto BadChar;
        break;

    case HTA:
        if (cbCur+1 < cbLen && '%' == pchLine[cbCur+1])
        {
            SetValueSeen(plxs);
            return FindTagOpen(pchLine, cbLen, cbCur, plxs, token);
        }
        // else fall through
    case ERR:
    case HEN:
BadChar:
        token.tokClass = tokSpace;

        // DS96# 10116 [CFlaat]: we can be in DBCS here, and so we need
        //     to make sure that our increment is double-byte aware
        cbCur += _tcsnbcnt(pchLine + cbCur, 1); // byte count for current char
        ASSERT(cbCur <= cbLen);
        token.ibTokMac = cbCur;
        break;

    // ported HTMED CHANGE (walts) - added this case to handle dbcs attribute values.
    case HDB:
        {
        // DBCS char.  Handle for attribute values within tag.
        if (!SetValueSeen(plxs))
            goto BadChar;

        int iTokenLength = GetValueTokenLength(pchLine, cbLen, cbCur);
        token.ibTokMac = token.ibTokMin + iTokenLength;
        token.tokClass = tokValue;
        }
        break;
    // ported HTMED CHANGE END

    default:
        // GetHint has set token info
        if (token.tokClass != tokComment && (*plxs & inValue))
            FindValue(pchLine, cbLen, cbCur, plxs, token);
        break;
    }
    if (bError)
        IsUnknownID(pchLine, cbLen, cbCur, token);
    return token.ibTokMac;
}

////////////////////////////////////////////////////////////////////
// GetTextHint()
// Like GetHint when scanning text -- look only for tags and entities
//
HINT GetTextHint(LPCTSTR pchLine, UINT /*cbLen*/, UINT cbCur, DWORD * /*plxs*/, TXTB & token)
{
    // if the character is bigger than 128 (dbcs) then return error
    if (pchLine[cbCur] & ~0x7F)
        return HDB;

    HINT hint = g_hintTable[pchLine[cbCur]];

    if (IsSingleOp(hint))
    {
        hint = ERR;
    }
    else if (hint == ONL || hint == EOS)
    {
        token.tokClass = tokOp;
        token.ibTokMac = cbCur + 1;
    }
    return hint;
}

////////////////////////////////////////////////////////////////////
// GetHint()
//      Use hint table to guess what the next token going to be
//      If it is a single operator, it will fill in the token info
//      as well
//
HINT GetHint(LPCTSTR pchLine, UINT cbLen, UINT cbCur, DWORD * plxs, TXTB & token)
{
    // if the character is bigger than 128 (dbcs) then return error
    if (pchLine[cbCur] & ~0x7F)
        return HDB;

    HINT hint = g_hintTable[pchLine[cbCur]];

    // check if it is a single op, new line or end of stream
    if (IsSingleOp(hint) || hint == ONL || hint == EOS)
    {
        token.tokClass = hint;
        token.ibTokMac = cbCur + 1;
    }
    else if (hint == ODA)
    {
        if ((cbCur + 1 < cbLen) &&
            (g_hintTable[pchLine[cbCur + 1]] == ODA) &&
            (*plxs & inBangTag))
        {
            cbCur += 2;
            *plxs |= inComment;
            COMMENTTYPE ct = IfHackComment(pchLine, cbLen, cbCur, plxs, token);
            if (ct == 0)
            {
                token.tokClass = tokComment;
                token.ibTokMac = cbCur;
            }
            else if(ct == CT_METADATA)
                hint = HTA; // tag open
        }
        else
        {
            // single -
            token.tokClass = tokOp;
            token.ibTokMac = cbCur + 1;
        }
    }
    return hint;
}

///////////////////////////////////////////////////////////////////
// GetTokenLength ()
//  return the length of a token identifier/keyword
//
UINT GetTokenLength(LPCTSTR pchLine, UINT cbLen, UINT cbCur)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    UINT cb;
    UINT cbOld = cbCur;

    if (IsCharAlphaNumeric(*pCurrent))
    {
        while (cbCur < cbLen && IsIdChar(*pCurrent))
        {
            cb = _tclen(pCurrent);
            cbCur += cb;
            pCurrent += cb;
        }
    }
    return (int) max((cbCur - cbOld), 1);
}

/*

    UINT GetValueTokenLength

    Description:
        Gets the length of the token.
        This version will accept any non whitespace character
        in the token.

*/
UINT GetValueTokenLength(LPCTSTR pchLine, UINT cbLen, UINT cbCur)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    UINT cb;
    UINT cbOld = cbCur;

    while (cbCur < cbLen && !_istspace(*pCurrent) && IsValueChar(*pCurrent))
    {
        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }
    return (int) max((cbCur - cbOld), 1);
}


////////////////////////////////////////////////////////////////
// IsElementName ()
//  lookup the keyword table to determine if it is a keyword or not
//
BOOL IsElementName(LPCTSTR pchLine, UINT cbCur, int iTokenLength, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    int iFound = NOT_FOUND;

    if (NOT_FOUND != (iFound = g_pTable->FindElement(pCurrent, iTokenLength)))
    {
        token.tokClass = tokElem;
        token.ibTokMac = cbCur + iTokenLength;
        token.tok = iFound; // set token
    }
    return (iFound != NOT_FOUND);
}

int IndexFromElementName(LPCTSTR pszName)
{
    return g_pTable->FindElement(pszName, lstrlen(pszName));
}

////////////////////////////////////////////////////////////////
// IsAttributeName ()
// lookup the keyword table to determine if it is a keyword or not
//
BOOL IsAttributeName(LPCTSTR pchLine, UINT cbCur, int iTokenLength, TXTB & token)
{
    LPCTSTR pCurrent = &pchLine[cbCur];
    int iFound = NOT_FOUND;

    if (NOT_FOUND != (iFound = g_pTable->FindAttribute(pCurrent, iTokenLength)))
    {
        token.tokClass = tokAttr;
        // ENDSPAN__ is needed b/c the lexer does not recognize the
        // endspan-- as 2 seperate tokens.
        if(iFound == TokAttrib_ENDSPAN__)
        {
            // endspan-- found.  return TokAttrib_ENDSPAN
            // set ibTokMac to not include --.
            token.tok = TokAttrib_ENDSPAN;
            token.ibTokMac = cbCur + iTokenLength - 2;
        }
        else
        {
            token.ibTokMac = cbCur + iTokenLength;
            token.tok =  iFound; // set token
        }
    }
    return (iFound != NOT_FOUND);
}

//////////////////////////////////////////////////////////////////////////
// IsIdentifier()
// check if it is an identifier
//
BOOL IsIdentifier (int iTokenLength, TXTB & token)
{
    if (iTokenLength > 0)
    {
        token.tokClass = tokName;
        token.ibTokMac = token.ibTokMin + iTokenLength;
        return TRUE;
    }
    else
        return FALSE;
}

////////////////////////////////////////////////////////////////////
// IsUnknownID ()
//  Mark the next token as an ID
//
BOOL IsUnknownID (LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token)
{
    ASSERT(cbCur < cbLen);
    UINT cb;
    LPCTSTR pCurrent = &pchLine[cbCur];

    cb = _tclen(pCurrent);
    cbCur += cb;
    pCurrent += cb;

    while ((cbCur < cbLen) && IsIdChar(*pCurrent))
    {
        cb = _tclen(pCurrent);
        cbCur += cb;
        pCurrent += cb;
    }

    token.tokClass = tokSpace;
    token.ibTokMac = cbCur;
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////
// IsNumber()
//  Check whether the next token is an SGML NUMTOKEN
//
BOOL IsNumber(LPCTSTR pchLine, UINT cbLen, UINT cbCur, TXTB & token)
{
    if (cbCur >= cbLen)
        return FALSE;

    if (!_istdigit(pchLine[cbCur]))
        return FALSE;

    token.tokClass = tokNum;

    // assume all digits are one byte
    ASSERT(1 == _tclen(&pchLine[cbCur]));
    cbCur++;

    while (cbCur < cbLen && _istdigit(pchLine[cbCur]))
    {
        // assume all digits are one byte
        ASSERT(1 == _tclen(&pchLine[cbCur]));
        cbCur++;
    }

    token.ibTokMac = cbCur;
    return TRUE;
}



/* end of file */
