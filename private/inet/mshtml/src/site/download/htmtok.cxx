//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       htmtok.cxx
//
//  Contents:   CHtmPre::Tokenize
//
//              Split out from htmpre.cxx for better code generation
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTM_HXX_
#define X_HTM_HXX_
#include "htm.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_ASSOC_HXX_
#define X_ASSOC_HXX_
#include "assoc.hxx"
#endif

#ifndef X_ENTITY_H_
#define X_ENTITY_H_
#include "entity.h"
#endif

#ifndef X_HTMVER_HXX_
#define X_HTMVER_HXX_
#include "htmver.hxx"
#endif

#ifdef WIN16
#ifndef X_URLMKI_H_
#define X_URLMKI_H_
#include "urlmki.h"
#endif
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_HTMTOK_HXX_
#define X_HTMTOK_HXX_
#include "htmtok.hxx"
#endif

#define HTMPRE_BLOCK_SIZE      4096 // assumed to be power of 2

#define CCH_LARGE_TOKEN       16384 // after text buf grows to 16K, use exponential allocation
#define CCH_OVERFLOW_TOKEN  1048576 // stop growing after a megabyte
#define MAX_ATTR_COUNT        16383 // Allow at most 16K-1 attribute-value pairs

ExternTag(tagPalette);
ExternTag(tagToken);

PerfDbgExtern(tagHtmPre);
PerfDbgExtern(tagHtmPreOneCharText);
PerfDbgExtern(tagPerfWatch);

DWORD HashString(const TCHAR *pch, DWORD len, DWORD hash);
extern HRESULT SetUrlDefaultScheme(const TCHAR *pchHref, CStr *pStr);
extern BOOL _7csnziequal(const TCHAR *string1, DWORD cch, const TCHAR *string2);

#if DBG != 1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

//+------------------------------------------------------------------------
//
//  Member:     CHtmPre::Tokenize
//
//  Synopsis:   The main method of the tokenizer.
//
//              Advances _pch up to _pchEnd, producing tag/text output
//              and saving state as required.
//
//-------------------------------------------------------------------------
//
// Description:
//
// The tokenizer is designed to mimic Netscape 3.0 parsing rules.
//
// * space is ascii 9-13 and 32.
// * markup begins with < and &
// * there are three quote chars, ' " and `, and they only self-match
// * tags begin with <alpha, <!, <?, or </
// * tags end with a > which does not lie inside a quote
// * tag names can contain any char except a space or a >
// * attribute names can be quoted (closing quote does not terminate name)
// * unquoted attribute names contain any char (incl quote) except sp, >, or =
// * attribute values can be quoted (closing quote does terminate value)
// * unquoted values contain any char (incl quote) except sp or >
// * EOF in the middle of a tag causes initial < to be treated as text
// * comments begin with <!-- and close with -->
// * in addition, comments can have fewer than 4 dashes, e.g., <!-->
// * EOF in the middle of a comment causes comment to be treated as <! tag
// * all markup in a comment is ignored, even inside literal tags
// * comments inside literal tags are treated as text
// * literal tags can suppress either entities or tags or both
// * plain text mode ignores all markup, including </PLAINTEXT>
// * numbered entities can have an arbitrary number of digits
// * &#0 is not a legal numbered entity (treated as text)
// * a semicolon after an entity is eaten, but a space is not
// * CRLF collapses to CR, but LFCR does not (it becomes CRCR)
// * document.write output appears directly after </SCRIPT>
//
// Following are regular expressions representing the tokenizer:
// (n = not, s = space, q = quote, a = alphabetic, d = digit)
//
// Comment:
// <!--((-)*|(.)*--)>
//
// Begin tag:
// <(a)(ns>)*((s)*[(q)(nq)*(q)][(ns=>)*][=][(q)(nq)*(q)|(nsq>)(ns>)*])*(s)*>
//
// End tag:
// </(s)*[(q)(nq)*(q)](ns>)*((s)*[(q)(nq)*(q)][(ns=>)*][=][(q)(nq)*(q)|(nsq>)(ns>)*])*(s)*>
//
// Decl tag:
// <!(ns>)*((s)*[(q)(nq)*(q)][(ns=>)*][=][(q)(nq)*(q)|(nsq>)(ns>)*])*(s)*>
//
// Entity:
// &(entity|(#d(d)*))[;]
//
// Everything else is text.
//
//-------------------------------------------------------------------------

#if DBG == 1
#define TraceState(state) if (IsTagEnabled(tagToken)) {TraceStateImpl(#state, pchStart, pch, pchEnd);}

#define PRETTY_CHAR(ch) ((ch) < 32 ? _T('~') : (ch) > 127 ? _T('@') : (ch))


void TraceStateImpl(char *state, TCHAR *pchStart, TCHAR *pchAt, TCHAR *pchEnd)
{
    TCHAR achBefore[12 + 1];
    TCHAR achAfter[48 + 1];
    TCHAR *pch;
    TCHAR *pchTo;
    static c=0;

    achBefore[ARRAY_SIZE(achBefore) - 1] = _T('\0');
    achAfter[ARRAY_SIZE(achAfter) - 1] = _T('\0');

    pch = pchAt - (ARRAY_SIZE(achBefore) - 1);
    pchTo = achBefore;

    while (pch < pchStart)
    {
        *pchTo++ = _T('*');
        pch++;
    }

    while (pch < pchAt)
    {
        *pchTo = PRETTY_CHAR(*pch);
        pchTo++;
        pch++;
    }

    pchTo = achAfter + (ARRAY_SIZE(achAfter) - 1) - 1;
    pch = pchAt + (ARRAY_SIZE(achAfter) - 1);

    while (pch >= pchEnd && pch > pchAt)
    {
        *pchTo-- = _T('*');
        pch--;
    }

    while (pch > pchAt)
    {
        *pchTo = PRETTY_CHAR(*pch);
        pchTo--;
        pch--;
    }

    TraceTag((tagToken, "%8d %14s:  %ls{%lc}%ls", c++, state, achBefore, PRETTY_CHAR(*pchAt), achAfter));
}

#undef PRETTY_CHAR

#else
#define TraceState(state)
#endif

// CHtmPre keeps copies of a bunch of the instance variables on the stack to allow for
// compiler optimizations.  These macros save all the local variables back to the instance
// variables before calling functions that might rely / alter the instance variables.
#define SAVE_STACKVAR_STATE()  { _pch = pch;  _pchEnd = pchEnd;  _pchStart = pchStart;  _state = state; }
#define RESTORE_STACKVAR_STATE()  { pch = _pch; ch = *pch; pchEnd = _pchEnd; pchStart = _pchStart; state = _state; }

HRESULT
CHtmPre::Tokenize()
{
    // nothing to tokenize
    if (!_pch)
        return S_OK;

    PerfDbgLog(tagHtmPre, this, "+CHtmPre::Tokenize");

    static const TCHAR achXmlDeclPrefix[] = XML_DECL; // _T("XML:namespace");

#ifdef VSTUDIO7
    static const TCHAR achFactory[]   = FACTORY_TAGNAME; // _T("?FACTORY");
    static const TCHAR achTagsource[] = TAGSOURCE_TAGNAME; // _T("?TAGSOURCE");
#endif //VSTUDIO7

    TCHAR * pch        = _pch;
    TCHAR * pchEnd     = _pchEnd;
    TCHAR * pchStart   = _pchStart;
    TCHAR * pchWord    = _pchWord;
    ULONG   state      = _state;
    TCHAR   chQuote    = _chQuote;
    TCHAR   ch         = *pch;
    XCHAR   chEnt;
    HRESULT hr;

    // scan every avaliable character
    while (ch)
    {
        Assert(pch < pchEnd);
        Assert(ch == *pch);

        switch (state)
        {

        case TS_PLAINTEXT:

            TraceState(PLAINTEXT);

            // pchStart: first text char not yet output

            // In plaintext, everything is text
            pch = pchEnd;
            ch  = *pch;

            // fallthrough

        case TS_TEXT:
        TEXT:

            if (_cOutputInsert)
            {
                hr = THR(OutputInserts());
                if (hr)
                    goto Cleanup;
            }

            // Optimization: deal with leading CRLF/LF
            if (ch == _T('\r'))
            {
                _nLine += _fCount;
                ch = *(++pch);
                
                if (ch == _T('\n'))
                    ch = *(++pch);
            }
            else if (ch == _T('\n') && pch == pchStart)
            {
                if (!_fEndCR)
                    _nLine += _fCount;
                    
                ch = *(++pch);
            }

        QUICKTEXT:

            TraceState(TEXT);

            // pchStart: first text char not yet output
            // pch:      first char not yet known to be text

            // skip to first special char
            while (ISTXTCH(ch))
                ch = *(++pch);

            // count line  (count \r or \n, but not \n immediately following \r)
            if (ch == _T('\r'))
            {
                _nLine += _fCount;
                ch = *(++pch);
                
                if (ch == _T('\n'))
                    ch = *(++pch);
                    
                goto QUICKTEXT;
            }

            if (ch == _T('\n'))
            {
                if (*(pch-1) != _T('\r'))
                    _nLine += _fCount;
                ch = *(++pch);
                goto QUICKTEXT;
            }

            if (pch > pchStart && !_fSuppressLeadingText)
            {
                BOOL fAscii = _pchAscii <= _pchStart;

                if (_etagEchoSourceEnd)
                {
                    hr = THR(SaveSource(pchStart, PTR_DIFF(pch, pchStart)));
                    if (hr)
                        goto Cleanup;
                }

                #if DBG==1 || defined(PERFTAGS)
                if (IsPerfDbgEnabled(tagHtmPreOneCharText))
                {
                    for (TCHAR * pchT = pchStart; pchT < pch; ++pchT)
                    {
                        hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXT, pchT, 1, fAscii));
                        if (hr)
                            goto Error;
                    }
                }
                else
                #endif
                {
                    TraceTag((tagToken, "   OutputText %d", PTR_DIFF(pch, pchStart)));
                    hr = THR(_pHtmTagStm->WriteTag(ETAG_RAW_TEXT, pchStart, PTR_DIFF(pch, pchStart), fAscii));
                    if (hr)
                        goto Error;
                }
            }

            _nLineStart = _nLine;
            _fCountStart = _fCount;
            if (pch > pchStart)
                _fEndCR = (*(pch-1) == _T('\r'));
            pchStart = pch;

            // end of available buffer
            if (!ch)
                break;

            // possible entity
            if (ch == _T('&'))
            {
                state = _fLiteralEnt ? TS_TEXT : TS_ENTOPEN;
                ch = *(++pch);      // at '&'+1
                break;
            }

            Assert(ch == _T('<'));

            // possible tag/comment/decl
            state = TS_TAGOPEN;
            ch = *(++pch);      // at '<'+1

            // end of available buffer
            if (!ch)
                break;

            // fallthrough

        case TS_TAGOPEN:

            TraceState(TAGOPEN);

            // pchStart: at '<'
            // pch:      at '<'+1

            // possibly not a tag
            if ((!ISALPHA(ch) || _etagLiteral) && ch != _T('/'))
            {
                // unusual markup: <!, <%, <?

                if (ch == _T('!'))
                {
                    state = TS_TAGBANG;
                    ch = *(++pch);
                    goto TAGBANG; // withstands \0
                }
                else if (ch == _T('%'))
                {
                    state = TS_TAGASP;
                    ch = *(++pch);
                    goto TAGASP;
                }
                else if (ch == _T('?'))
                {
                    state = TS_TAGSCAN;
                    ch = *(++pch);
                    goto TAGSCAN;
                }

                // not markup
                state = TS_TEXT;
                goto TEXT;
            }

            state = TS_TAGSCAN;
            ch = *(++pch);

            // fallthrough

        // For Netscape compatibility: when parsing a tag, begin by finding the
        // ending '>'. Any > hidden by quotes don't count so must match quotes
        // according to Netscape rules.

        // In this phase, Netscape defines a matchable quote as ', ", or `
        // preceded by a space or an =.

        TAGSCAN:
        case TS_TAGSCAN:
            // pchStart: at '<'
            // pch:      at next char in tag not in quote

            TraceState(TAGSCAN);

            while (ISTAGCH(ch))
                ch = *(++pch);

            // found a tag!
            if (ch == _T('>'))
            {
                ULONG ulChars;
                DWORD otCode;

                // consume >
                ch = *(++pch);

                // Treat <!... and <?... tags as comments, except <?XML:namespace ... >
#ifdef VSTUDIO7
                if (!_etagLiteral && (*(pchStart+1) == _T('!') ||
                    (*(pchStart+1) == _T('?')) &&
                     ((0 != _tcsnicmp(achXmlDeclPrefix, -1, pchStart+2, ARRAY_SIZE(achXmlDeclPrefix) - 1)) &&
                      (0 != _tcsnicmp(achFactory, -1, pchStart+1, ARRAY_SIZE(achFactory) - 1)) &&
                      (0 != _tcsnicmp(achTagsource, -1, pchStart+1, ARRAY_SIZE(achTagsource) - 1)))))
#else
                if (!_etagLiteral && (*(pchStart+1) == _T('!') ||
                    (*(pchStart+1) == _T('?')) &&
                    0 != _tcsnicmp(achXmlDeclPrefix, -1, pchStart+2, ARRAY_SIZE(achXmlDeclPrefix) - 1)))
#endif //VSTUDIO7
                {
                    hr = THR(OutputComment(pchStart, PTR_DIFF(pch, pchStart)));
                    if (hr)
                        goto Cleanup;
                        
                    _nLineStart = _nLine;
                    _fCountStart = _fCount;
                    if (pch > pchStart)
                        _fEndCR = (*(pch-1) == _T('\r'));
                    pchStart = pch;
                    state = TS_TEXT;
                    break;
                }

                // compute offset of start of tag
                if ((unsigned)(pchEnd - pchStart) > _ulCharsUncounted)
                    ulChars = _ulCharsEnd + _ulCharsUncounted - (pchEnd - pchStart);
                else
                    ulChars = _ulCharsEnd;

                SAVE_STACKVAR_STATE();
 
                // Tokenize the tag
                hr = THR(OutputTag(_nLineStart, ulChars, _fCountStart && _fCount, &otCode));
                if (hr)
                    goto Cleanup;
                    
                RESTORE_STACKVAR_STATE();

                if (otCode == OT_REJECT)
                {
                    // ROLLBACK for literal
                    _nLine = _nLineStart;
                    ch = *(pch = pchStart+1);
                    Assert(ch);
                    state = TS_TEXT;
                    goto TEXT;
                }
                Assert( ! otCode );

                TraceState(TAGOUT);

                state = _state;

                // We did output, so advance
                _nLineStart = _nLine;
                _fCountStart = _fCount;
                if (pch > pchStart)
                    _fEndCR = (*(pch-1) == _T('\r'));
                pchStart = pch;

                // after script tags we must suspend
                if (state == TS_SUSPEND)
                {
                    state = TS_TEXT;
                    hr = E_PENDING;
                    goto Cleanup;
                }

                else if (state == TS_NEWCODEPAGE)
                {
                    goto NEWCODEPAGE;
                }

                break;
            }

            // count line
            if (ch == _T('\r'))
            {
                _nLine += _fCount;
                ch = *(++pch);

                if (ch == _T('\n'))
                    ch = *(++pch);
                
                goto TAGSCAN;
            }

            if (ch == _T('\n'))
            {
                Assert(pch > pchStart); // Can back up by one
                
                if (*(pch-1) != _T('\r'))
                    _nLine += _fCount;
                    
                ch = *(++pch);
                goto TAGSCAN;
            }

            if (!ch)
                break;

            Assert(ISQUOTE(ch));

            // Netscape's rule: quote counts only if preceded by space or =
            ch = *(pch-1);

            if (ISNONSP(ch) && ch != _T('='))
            {
                ch = *(++pch);
                goto TAGSCAN;
            }

            chQuote = *pch;
            ch = *(++pch);
            state = TS_TAGSCANQ;
            goto TAGSCANQ;

        TAGSCANQ:
        case TS_TAGSCANQ:

            TraceState(TAGSCANQ);

            while (ch)
            {

                if (ch == chQuote)
                {
                    state = TS_TAGSCAN;
                    ch = *(++pch);
                    goto TAGSCAN;
                }
                ch = *(++pch);
            }

            // end of available buffer
            break;

        TAGASP:
        case TS_TAGASP:

            // pchStart: at '<%'
            // pch:      at next char inside <% ...

            TraceState(TAGASP);

            while (ISMRKCH(ch))
                ch = *(++pch);

            // count line
            if (ch == _T('\r') || (ch == _T('\n')))
            {
                if (ch == _T('\r') || !(pch == pchStart ? _fEndCR : *(pch-1) == _T('\r')))
                    _nLine += _fCount;
                ch = *(++pch);
                goto TAGASP;
            }

            if (!ch)
                break;

            Assert(ch == _T('>'));

            ch = *(++pch);  // past '>'

            if (pch[-2] != _T('%'))
                goto TAGASP;

            if (!_etagLiteral)
            {
                hr = THR(OutputComment(pchStart, PTR_DIFF(pch, pchStart)));
                if (hr)
                    goto Cleanup;

                _nLineStart = _nLine;
                _fCountStart = _fCount;
                if (pch > pchStart)
                    _fEndCR = (*(pch-1) == _T('\r'));
                pchStart = pch;
            }

            // literal comment output as text
            state = TS_TEXT;
            break;

        TAGBANG:
        case TS_TAGBANG:

            TraceState(TAGBANG);

            // pchStart: at '<'
            // pch:      at '<!'+1

            // skip to nondash
            while (ISDASHC(ch))
                ch = *(++pch);

            // end of available buffer
            if (!ch)
                break;

            // detect conditional idioms
            if (ch == _T('[') && !_etagLiteral)
            {
                if (pch - pchStart == 2) // <![
                {
                    ch = *(++pch);
                    pchWord = pch;
                    _fCondComment = FALSE;
                    state = TS_CONDSCAN;
                    goto CONDSCAN; // withstands \0
                }

                if (pch - pchStart == 4) // <!--[
                {
                    ch = *(++pch);
                    pchWord = pch;
                    _fCondComment = TRUE;
                    state = TS_CONDSCAN;
                    goto CONDSCAN; // withstands \0
                }
            }

            // two or more dashes: comment
            if ( PTR_DIFF(pch, pchStart) >= 4 )
            {
                // NS: handle short comments
                pchWord = pch-2; // at '--'
                state = TS_TAGCOMDASH;
                break;
            }

            // literal: output noncomment as text
            if (_etagLiteral)
            {
                _nLine = _nLineStart;
                ch = *(pch = pchStart+1);
                state = TS_TEXT;
                goto TEXT; // break;
            }

            // open tag: tagname begins with '!'
            _nLine = _nLineStart;
            ch = *(pch = pchStart+1);
            state = TS_TAGSCAN;
            goto TAGSCAN;

        TAGCOMMENT:
        case TS_TAGCOMMENT:

            TraceState(TAGCOMMENT);

            // pchStart: at '<'
            // pch:      at first possible dash

            // skip to first dash
            while (ISNDASH(ch))
                ch = *(++pch);

            // end of available buffer
            if (!ch)
                break;

            // count line
            if (ch == _T('\r') || (ch == _T('\n')))
            {
                if (ch == _T('\r') || !(pch == pchStart ? _fEndCR : *(pch-1) == _T('\r')))
                    _nLine += _fCount;
                ch = *(++pch);
                goto TAGCOMMENT;
            }

            // possible end of comment
            pchWord = pch;      // at '-'
            state = TS_TAGCOMDASH;

            // fallthrough

        case TS_TAGCOMDASH:

            TraceState(TAGCOMDASH);

            // pchStart: at '<'
            // pchWord:  at first '-'
            // pch:      at first possible nondash

            // skip to first nondash
            while (ISDASHC(ch))
            {
                ch = *(++pch);
            }

            // end of available buffer
            if (!ch)
                break;

            // two dashes and a > end comment
            if (ch == _T('>') && (PTR_DIFF(pch, pchWord) >= 2))
            {
                ch = *(++pch); // past '>'

                if (!_etagLiteral)
                {
                    long cch = PTR_DIFF(pch, pchStart);

                    hr = THR(OutputComment(pchStart, cch));
                    if (hr)
                        goto Cleanup;

                    _nLineStart = _nLine;
                     _fCountStart = _fCount;
                    if (pch > pchStart)
                        _fEndCR = (*(pch-1) == _T('\r'));
                    pchStart = pch;
                    state = TS_TEXT;

                }
                else
                {
                    // literal comment output as text
                    state = TS_TEXT;
                }
                break;
            }

            // comment did not end
            state = TS_TAGCOMMENT;
            goto TAGCOMMENT; // break;

        case TS_ENTOPEN:

            // pchStart: at '&'
            // pch:      at '&'+1

            // looks like a number or hex entity
            if (ch == _T('#'))
            {
                ch = *(++pch);
                // looks like a hex entity
                if ((ch == _T('X')) || (ch == _T('x')))
                {
                    ch = *(++pch);
                    pchWord = pch; // at '&#X'+1
                    state = TS_ENTHEX;
                    break;
                }
                else
                {
                    // looks like a number entity
                    pchWord = pch; // at '&#'+1
                    state = TS_ENTNUMBER;
                    break;
                }
            }

            // looks like a named entity
            pchWord = pch; // at '&'+1
            _hash = 0;
            _chEnt = 0;
            state = TS_ENTMATCH;

            // fallthrough

        ENTMATCH:
        case TS_ENTMATCH:

            TraceState(ENTMATCH);

            // pchStart: at '&'
            // pchWord:  beyond last matched entity
            // pch       at first possible last-char-of-entity

            // NOTE: In IE5, unlike IE4, we no longer allow entities like &ltfoo to match.
            // The entity must end on a non-alphanumeric char, like &lt,foo
            // (This follows Nav 4.04+ etc.) - dbau

            // ANOTHER NOTE: in IE5, we decided to act like IE4 after all, so that
            // entities like &ltfoo _do_ match like &lt;foo.
            
            while (ISENTYC(ch) && (PTR_DIFF(pch + 1, pchStart + 1) <= MAXENTYLEN))
            {
                // grab extra char into hash
                _hash = HashString(pch, 1, _hash);

                // advance pch beyond extra char
                ch = *(++pch);

                // lookup HTML 1.0 entity (';' optional)
                chEnt = EntityChFromName(pchStart + 1, PTR_DIFF(pch, pchStart + 1), _hash);
                if (chEnt && IS_HTML1_ENTITY_CHAR(chEnt))
                {
                    _chEnt = chEnt;
                    pchWord = pch;
                }
            }

            // end of available buffer (and not EOF)
            if (!ch && !_fEOF)
                break;

            // lookup HTML 3.x entity (';' required)
            if (ch == _T(';'))
            {
                chEnt = EntityChFromName(pchStart + 1, PTR_DIFF(pch, pchStart + 1), _hash);
                if (chEnt)
                {
                    _chEnt = chEnt;
                    pchWord = pch;
                }
            }

            // not entity char give up and roll back to start
            if (!_chEnt)
            {
                _nLine = _nLineStart;
                ch = *(pch = pchStart+1);
                state = TS_TEXT;
                goto TEXT; // break;
            }

            // complete entity; rollback to word
            ch = *(pch = pchWord);
            
            // output the thing
            hr = THR(OutputEntity(pchStart, PTR_DIFF(pch, pchStart), _chEnt));
            if (hr)
                goto Error;

            if (pch > pchStart)
                _fEndCR = (*(pch-1) == _T('\r'));
            pchStart = pch;
            state = TS_ENTCLOSE;
            break;

        case TS_ENTCLOSE:

            TraceState(ENTCLOSE);

            // pchStart: at '&'
            // pch:      at possible ';'

            if (ch == _T(';'))
            {
                ch = *(++pch);
            }

            _nLineStart = _nLine;
            _fCountStart = _fCount;
            if (pch > pchStart)
                _fEndCR = (*(pch-1) == _T('\r'));
            pchStart = pch;
            state = TS_TEXT;
            break;

        ENTNUMBER:
        case TS_ENTNUMBER:

            TraceState(ENTNUMBER);

            // pchStart: at '&'
            // pchWord:  at '&#'+1
            // pch:      at first possible nondigit

            // skip to first nondigit
            while (ISDIGIT(ch))
            {
                ch = *(++pch);
            }

            // end of available buffer (and not EOF)
            if (!ch && !_fEOF)
                break;

            // EOF or nondigit ; end of number entity
            chEnt = EntityChFromNumber(pchWord, PTR_DIFF(pch, pchWord));
            if (chEnt)
            {
                hr = THR(OutputEntity(pchStart, PTR_DIFF(pch, pchStart), chEnt));
                if (hr)
                    goto Error;

                Assert(pch > pchStart && *(pch-1) != _T('\r'));
                _fEndCR = FALSE;
                pchStart = pch;
                state = TS_ENTCLOSE;
                break;
            }

            // zero entity: output as text
            state = TS_TEXT;
            break;

        ENTHEX:
        case TS_ENTHEX:

            TraceState(ENTHEX);

            // pchStart: at '&'
            // pchWord:  at "&#X + 1
            // pch:      at first possible nonhex

            // skip to first nonhex
            while (ISHEX(ch))
            {
                ch = *(++pch);
            }

            // end of available buffer (and not EOF)
            if (!ch && !_fEOF)
                break;

            // EOF or nonhex ; end of hex entity
            if (ch == _T(';'))
            {
                chEnt = EntityChFromHex(pchWord, PTR_DIFF(pch, pchWord));
                if (chEnt)
                {
                    hr = THR(OutputEntity(pchStart, PTR_DIFF(pch, pchStart), chEnt));
                    if (hr)
                        goto Error;

                    Assert(pch > pchStart && *(pch-1) != _T('\r'));
                    _fEndCR = FALSE;
                    pchStart = pch;
                    state = TS_ENTCLOSE;
                    break;
                }
            }

            // zero entity or missing ';': output as text
            state = TS_TEXT;
            break;

    CONDSCAN:
        case TS_CONDSCAN:

            TraceState(CONDSCAN);

            // pchStart: at '<'
            // pchWord:  at '<!#'+1     !_fCondComment
            // pchWord:  at '<!--#'+1   _fCondComment
            // pch:      at first possible '>'

            while (ISMRKCH(ch))
                ch = *(++pch);

            // count line
            if (ch == _T('\r') || (ch == _T('\n')))
            {
                if (ch == _T('\r') || !(pch == pchStart ? _fEndCR : *(pch-1) == _T('\r')))
                    _nLine += _fCount;
                ch = *(++pch);
                goto CONDSCAN;
            }

            if (!ch)
                break;

            Assert(ch == _T('>'));

            ch = *(++pch);  // past '>'

            {
                CONDVAL result;
                BOOL fComment;
                fComment = (pch[-2] == _T('-'));

                if (!_pVersions ||
                    fComment ?
                    pch < pchWord + 4 || pch[-2] != _T('-') || pch[-3] != _T('-') || pch[-4] != _T(']'):
                    pch < pchWord + 2 || pch[-2] != _T(']'))
                {
                    // was not well-formed ']>' or ']-->': treat as ordinary tag or comment
                    _nLine = _nLineStart;
                    if (_fCondComment)
                    {
                        ch = *(pch = pchStart+4); // at [ past <!--
                        state = TS_TAGCOMMENT;
                        goto TAGCOMMENT;
                    }
                    else
                    {
                        ch = *(pch = pchStart+1);
                        state = TS_TAGSCAN;
                        goto TAGSCAN;
                    }
                }

                hr = THR(_pVersions->EvaluateConditional(&result, pchWord, pch - (fComment ? 4 : 2) - pchWord));
                if (hr)
                    goto Cleanup;

                if (result == COND_SYNTAX ||
                    _fCondComment && result != COND_IF_TRUE && result != COND_IF_FALSE ||
                    fComment && result != COND_ENDIF ||
                    !_cCondNestTrue && result == COND_ENDIF)
                {
                    if (_fCondComment && result == COND_NULL)
                    {
                        // ordinary comment if unrecognized construct
                        _nLine = _nLineWord;
                        ch = *(pch = pchWord);
                        state = TS_TAGCOMMENT;
                        goto TAGCOMMENT;
                    }
                    else
                    {
                        // textify illegal constructs
                        _nLine = _nLineStart;
                        ch = *(pch = pchStart+1);
                        state = TS_TEXT;
                        goto TEXT;
                    }
                }

                if (result == COND_ENDIF)
                {
                    _cCondNestTrue--;
                }

                if (result == COND_IF_TRUE)
                {
                    _cCondNestTrue++;
                }

                if (result == COND_IF_FALSE)
                {
                    state = TS_CONDITIONAL;
                    break;
                }
    
                // Only suppress contents if condition evaluated to FALSE
                // if condition was unrecognized, TRUE cond, or matched ENDIF,
                // treat it as a comment

                hr = THR(OutputConditional(pchStart, PTR_DIFF(pch, pchStart), result));
                if (hr)
                    goto Cleanup;
    
                _nLineStart = _nLine;
                _fCountStart = _fCount;
                if (pch > pchStart)
                    _fEndCR = (*(pch-1) == _T('\r'));
                pchStart = pch;
                state = TS_TEXT;

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
                // for include case, make sure preprocessor suspends correctly

                if (result == COND_INCLUDE) 
                {
                    hr = E_PENDING;
                    goto Cleanup;
                }
#endif
            }

            break;


    CONDITIONAL:
        case TS_CONDITIONAL:

            TraceState(CONDITIONAL);

            // pchStart: at start of '<!#if ...#>...'
            // pch at first possible '<' for <!#endif#>

            // skip to first special char
            while (ISTXTCH(ch))
                ch = *(++pch);

            // count line  (count \r or \n, but not \n immediately following \r)
            if (ch == _T('\r') || (ch == _T('\n')))
            {
                if (ch == _T('\r') || !(pch == pchStart ? _fEndCR : *(pch-1) == _T('\r')))
                    _nLine += _fCount;
                ch = *(++pch);
                goto CONDITIONAL;
            }

            if (!ch)
                break;

            if (ch != _T('<'))
            {
                ch = *(++pch);
                goto CONDITIONAL;
            }

            ch = *(++pch);

            state = TS_ENDCOND;
            if (!ch)
                break;

            // fallthrough

        case TS_ENDCOND:

            TraceState(ENDCOND);

            // pch:     at '<'+1

            if (ch != _T('!'))
            {
                state = TS_CONDITIONAL;
                goto CONDITIONAL;
            }

            ch = *(++pch);

            state = TS_ENDCONDBANG;
            if (!ch)
                break;

            // fallthrough

       case TS_ENDCONDBANG:

            // pch:     at '<!'+1

            if (ch !=_T('['))
            {
                state = TS_CONDITIONAL;
                goto CONDITIONAL;
            }

            ch = *(++pch);
            pchWord = pch;
            _nLineWord = _nLine;

            state = TS_ENDCONDSCAN;
            if (!ch)
                break;

            // fallthrough

   ENDCONDSCAN:

       case TS_ENDCONDSCAN:

            // pchWord: at '<!#'+1
            // pch:     at first possible '>'

            while (ISMRKCH(ch))
                ch = *(++pch);

            // count line
            if (ch == _T('\r') || (ch == _T('\n')))
            {
                if (ch == _T('\r') || !(pch == pchStart ? _fEndCR : *(pch-1) == _T('\r')))
                    _nLine += _fCount;
                ch = *(++pch);
                goto ENDCONDSCAN;
            }

            if (!ch)
                break;

            Assert(ch == _T('>'));

            ch = *(++pch);  // past '>'

            Assert(_pVersions);

            if (_fCondComment ?
                 pch < pchWord + 4 || pch[-2] != _T('-') || pch[-3] != _T('-') || pch[-4] != _T(']'):
                 pch < pchWord + 2 || pch[-2] != _T(']'))
            {
                // Not well-formed <!# .... #> or <!# .... #-->: rollback
                _nLine = _nLineWord;
                ch = *(pch = pchWord + 2); // at '<!#'+1
                state = TS_CONDITIONAL;
                goto CONDITIONAL;
            }

            {

                CONDVAL result;
                hr = THR(_pVersions->EvaluateConditional(&result, pchWord, pch - (_fCondComment ? 4 : 2) - pchWord));
                if (hr)
                    goto Cleanup;

                // gibberish: rollback
                if (result == COND_NULL || result == COND_SYNTAX)
                {
                    _nLine = _nLineWord;
                    ch = *(pch = pchWord + 2); // at '<!#'+1
                    state = TS_CONDITIONAL;
                    goto CONDITIONAL;
                }

                // handle nested <!#if#> : treat as gibberish inside condstyle; otherwise nest
                if (result == COND_IF_TRUE || result == COND_IF_FALSE)
                {
                    if (_fCondComment)
                    {
                        _nLine = _nLineWord;
                        ch = *(pch = pchWord + 2); // at '<!#'+1
                    }
                    else
                    {
                        _cCondNest++;
                    }
                    state = TS_CONDITIONAL;
                    goto CONDITIONAL;
                }

#ifdef CLIENT_SIDE_INCLUDE_FEATURE
                // <![include]> in an off conditional gets ignored
                if (result == COND_INCLUDE) 
                {
                    state = TS_CONDITIONAL;
                    goto CONDITIONAL;
                }
#endif

                Assert(result == COND_ENDIF);
                
                // <!#endif#-->

                if (_fCondComment)
                    _fCondComment = FALSE;

                // nested <!#endif#>

                if (_cCondNest)
                {
                    Assert(!_fCondComment);
                    _cCondNest--;
                    state = TS_CONDITIONAL;
                    goto CONDITIONAL;
                }

                // if last endif, output the whole conditional area as a single comment
                hr = THR(OutputConditional(pchStart, PTR_DIFF(pch, pchStart), result));
                if (hr)
                    goto Cleanup;

                _nLineStart = _nLine;
                _fCountStart = _fCount;
                if (pch > pchStart)
                    _fEndCR = (*(pch-1) == _T('\r'));
                pchStart = pch;
                state = TS_TEXT;
            }

            break;

    NEWCODEPAGE:
        case TS_NEWCODEPAGE:

            TraceState(NEWCODEPAGE);

            // pchStart: at '>'+1
            // pch:      at '>'+1

            _pch      = pch;
            _pchEnd   = pchEnd;
            _pchStart = pchStart;

            // cannot switch codepage inside script or if we've already switched once
            if (!_cSuspended && !_fRestarted && !_fMetaCharsetOverride)
            {
                BOOL fNeedRestart;
                
                // after switching codepage we must suspend
                if (DoSwitchCodePage(_cpNew, &fNeedRestart, TRUE) && fNeedRestart)
                {
                    state = TS_TEXT;
                    hr = E_PENDING;
                    goto Cleanup;
                }
            }

            state = TS_TEXT;
            pch = _pch;
            pchStart = _pchStart;
            pchEnd = _pchEnd;
            break;

        default:
            AssertSz(0,"Unknown state in tokenizer");
            _nLine = _nLineStart;
            pch = pchStart+1;
            state = TS_TEXT;
        }
    }

    // we have examined every avaliable character
    Assert(!ch && pch == pchEnd);


    if (_fEOF && !_cSuspended && pchStart != pchEnd)
    {
        // EOF in unmatched false conditional: commentize up to EOF
        if (state == TS_CONDITIONAL || state == TS_ENDCONDSCAN)
        {
            hr = THR(OutputComment(pchStart, PTR_DIFF(pch, pchStart)));
            if (hr)
                goto Cleanup;

            _nLineStart = _nLine;
            _fCountStart = _fCount;
            if (pch > pchStart)
                _fEndCR = (*(pch-1) == _T('\r'));
            pchStart = pch;
            state = TS_TEXT;
        }
        // EOF in incomplete comment or conditional: turn to tag
        else if (state == TS_TAGCOMMENT || state == TS_CONDSCAN)
        {
            _nLine = _nLineStart;
            ch = *(pch = pchStart+1);
            state = TS_TAGSCAN;
            goto TAGSCAN; // withstands ch=='\0'
        }
        else if (state == TS_ENTMATCH)
        {
            // withstands ch=='\0', knows how to roll back
            goto ENTMATCH;
        }
        else if (state == TS_ENTNUMBER)
        {
            // withstands ch=='\0', knows how to roll back
            goto ENTNUMBER;
        }
        else if (state == TS_ENTHEX)
        {
            // withstands ch=='\0', knows how to roll back
            goto ENTHEX;
        }
        // EOF in incomplete tag: turn < or & to text
        else
        {
            _nLine = _nLineStart;
            ch = *(pch = pchStart+1);
            state = TS_TEXT; // withstands ch=='\0'
            goto TEXT;
        }
    }

    hr = S_OK;

Cleanup:

    // suspend comes here

    // save state
    _pch        = pch;
    _pchEnd     = pchEnd;
    _pchStart   = pchStart;
    _pchWord    = pchWord;
    _state      = state;
    _chQuote    = chQuote;

    if (hr == E_PENDING)
    {
        Suspend();
    }

    goto Leave;

Error:

    // on error, behave like EOF
    _fEOF = TRUE;
    _pchStart = _pchEnd;

Leave:

    _pHtmTagStm->Signal();

    PerfDbgLog1(tagHtmPre, this, "-CHtmPre::Tokenize (hr=%lX)", hr);
    RRETURN1(hr, E_PENDING);
}

#if DBG != 1
#pragma optimize("", on)
#endif


