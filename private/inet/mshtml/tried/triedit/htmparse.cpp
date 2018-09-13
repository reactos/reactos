// HtmParse.cpp : Implementation of CHtmParse
// Copyright (c)1997-1999 Microsoft Corporation, All Rights Reserved
#include "stdafx.h"

#include <designer.h>
#include <time.h> // for random number generation

#include "triedit.h"
#include "HtmParse.h"
#include "table.h"
#include "lexer.h"
#include "guids.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CTriEditParse
#undef ASSERT
#define ASSERT(b) _ASSERTE(b)

#ifdef NEEDED
inline int 
indexPrevtokTagStart(int index, TOKSTRUCT *pTokArray)
{
    while (    (index >= 0)
            && (pTokArray[index].token.tokClass != tokTag)
            && (pTokArray[index].token.tok != TokTag_START)
            )
    {
            index--;
    }
    return(index);
}

inline int
indexPrevTokElem(int index, TOKSTRUCT *pTokArray)
{
    while (    (index >= 0)
            && (pTokArray[index].token.tokClass != tokElem)
            )
    {
            index--;
    }
    return(index);
}
#endif //NEEDED

BOOL
FIsWhiteSpaceToken(WCHAR *pwOld, int indexStart, int indexEnd)
{
    BOOL fWhiteSpace = TRUE;
    int index;

    for (index = indexStart; index < indexEnd; index++)
    {
        if (   pwOld[index] != ' '
            && pwOld[index] != '\t'
            && pwOld[index] != '\r'
            && pwOld[index] != '\n'
            )
        {
            fWhiteSpace = FALSE;
            break;
        }
    }
    return (fWhiteSpace);
} /* FIsWhiteSpaceToken() */

inline void GlobalUnlockFreeNull(HGLOBAL *phg)
{
    GlobalUnlock(*phg); // do we need to check if this was already Unlocked?
    GlobalFree(*phg);
    *phg = NULL;
}

BOOL
FIsAbsURL(LPOLESTR pstr)
{
    LPCWSTR szHttp[] = {L"http:"};
    LPCWSTR szFile[] = {L"file:"};
    BOOL fRet = FALSE;

    if (pstr == NULL)
        goto LRet;

    if (   0 == _wcsnicmp(szHttp[0], pstr, wcslen(szHttp[0]))
        || 0 == _wcsnicmp(szFile[0], pstr, wcslen(szFile[0]))
        )
    {
        fRet = TRUE;
        goto LRet;
    }
LRet:
    return(fRet);
}

BOOL
FURLNeedSpecialHandling(TOKSTRUCT *pTokArray, int iArray, LPWSTR pwOld, int cMaxToken, int *pichURL, int *pcchURL)
{
    int index = iArray+1;
    int iHref = -1;
    int iURL = -1;
    BOOL fRet = FALSE;
    BOOL fCodeBase = FALSE;

    while (    index < cMaxToken
            && pTokArray[index].token.tok != TokTag_CLOSE
            && pTokArray[index].token.tokClass != tokTag
            ) // look for TokAttrib_HREF
    {
        if (   iHref == -1
            && (   pTokArray[index].token.tok == TokAttrib_HREF
                || pTokArray[index].token.tok == TokAttrib_SRC
                || pTokArray[index].token.tok == TokAttrib_CODEBASE
                )
            && pTokArray[index].token.tokClass == tokAttr
            )
        {
            iHref = index;
            // special case - if we have CODEBASE attribute, we always want special processing
            if (pTokArray[index].token.tok == TokAttrib_CODEBASE)
                fCodeBase = TRUE;
        }
        if (   iHref != -1
            && pTokArray[index].token.tok == 0
            && (pTokArray[index].token.tokClass == tokString || pTokArray[index].token.tokClass == tokValue)
            )
        {
            iURL = index;
            break;
        }
        index++;
    }
    if (iURL != -1) // its set properly
    {
        int cchURL;
        WCHAR *pszURL;
        BOOL fQuote = (pwOld[pTokArray[iURL].token.ibTokMin] == '"');

        cchURL = (fQuote)
                ? pTokArray[iURL].token.ibTokMac-pTokArray[iURL].token.ibTokMin-2
                : pTokArray[iURL].token.ibTokMac-pTokArray[iURL].token.ibTokMin;
        *pichURL = (fQuote)
                ? pTokArray[iURL].token.ibTokMin+1
                : pTokArray[iURL].token.ibTokMin;
        // special case - if the quoted value happens to be a serverside script,
        // we can ignore it here and decalre that we don't need to do any special 
        // processing.
        if (   ((pTokArray[iURL].token.ibTokMac-pTokArray[iURL].token.ibTokMin) == 1)
            || (cchURL < 0)
            )
        {
            *pcchURL = 0;
            goto LRet;
        }
        *pcchURL = cchURL;
        // special case - if we have CODEBASE attribute, we always want special processing
        // we don't need to see if its URL is absolute or not...
        if (fCodeBase)
        {
            fRet = TRUE;
            goto LRet;
        }

        pszURL = new WCHAR [cchURL+1];

        ASSERT(pszURL != NULL);
        memcpy( (BYTE *)pszURL,
                (BYTE *)&pwOld[pTokArray[iURL].token.ibTokMin + ((fQuote)? 1 : 0)],
                (cchURL)*sizeof(WCHAR));
        pszURL[cchURL] = '\0';
        if (!FIsAbsURL((LPOLESTR)pszURL))
            fRet = TRUE;
        delete pszURL;
    } // if (iURL != -1)

LRet:
    return(fRet);
}


// Copied from hu_url.cpp 
//-----------------------------------------------------------------------------
// Useful directory separator check
//-----------------------------------------------------------------------------
inline BOOL IsDirSep(CHAR ch)
{
    return ('\\' == ch || '/' == ch);
}

inline BOOL IsDirSep(WCHAR ch)
{
    return (L'\\' == ch || L'/' == ch);
}



//-----------------------------------------------------------------------------
//  UtilConvertToRelativeURL
//
//  Returns an item-relative URL.
//      The URL is returned identical if
//          the projects don't match
//          the protocols don't match
//
//  Assumes that protocol-less URLs are "http:". Must specify "file:" explicitly
//  to play with file URLs. 
//-----------------------------------------------------------------------------

static LPOLESTR
SkipServer(LPOLESTR pstr)
{
    pstr = wcschr(pstr, L'/');
    if (pstr == NULL)
        return NULL;
    pstr = wcschr(pstr+1, L'/');
    if (pstr == NULL)
        return NULL;
    pstr = wcschr(pstr+1, L'/');

    return pstr;            // positioned on the slash if there was one.
}

static LPOLESTR
SkipFile(LPOLESTR pstr)
{
    LPOLESTR    pstrT;

    pstrT = wcspbrk(pstr, L":\\/");
    if (pstr == NULL)
        return pstr;

    // Guard against the case "//\\".

    if (pstrT == pstr &&
            IsDirSep(pstr[0]) &&
            IsDirSep(pstr[1]))
    {
        if (IsDirSep(pstr[2]) && IsDirSep(pstr[3]))
        {
            pstrT = pstr + 2;           // saw a "//\\"
        }
        else if (pstr[2] != L'\0'  && pstr[3] == L':')
        {
            pstrT = pstr + 3;           // saw a "//c:"
        }
    }

    ASSERT(!wcschr(pstrT + 1, ':')); // better not be more colons!
    if (*pstrT == ':')  // drive letter possibility
    {
        return pstrT + 1;               // point at the character after the colon
    }
    if (pstrT[0] == pstrT[1])           // double slash?
    {
        // Skip server part. 

        pstrT = wcspbrk(pstrT + 2, L"\\/");
        if (pstrT == NULL)
            return pstr;                // malformed!

        // Skip share part.

        pstrT = wcspbrk(pstrT + 1, L"\\/");
        if (pstrT == NULL)
            return pstr;                // malformed!

        return pstrT;
    }

    return pstr;
}

static LPOLESTR
FindLastSlash(LPOLESTR pstr)
{
    LPOLESTR    pstrSlash;      // '/'
    LPOLESTR    pstrWhack;      // '\'

    pstrSlash = wcsrchr(pstr, L'/');
    pstrWhack = wcsrchr(pstr, L'\\');

    return pstrSlash > pstrWhack
            ? pstrSlash
            : pstrWhack;
}

HRESULT
UtilConvertToRelativeURL(
    LPOLESTR    pstrDestURL,        // URL to 'relativize'
    LPOLESTR    pstrDestFolder,     // URL to be relative to.
    LPOLESTR    pstrDestProject,    // Project to be relative to.
    BSTR *      pbstrRelativeURL)
{
    HRESULT     hr = S_OK;
    LPOLESTR    pstrFolder;
    LPOLESTR    pstrURL;
    LPOLESTR    pchLastSlash;
    CComBSTR    strWork;
    int         cch;
    int         cchFolder;
    int         i;
    int         ichLastSlash;
    bool        fAbsoluteURL = false;
    bool        fAbsoluteFolder = false;
    bool        fFileURL = false;

    // If there's a ':' in the URL we're relativizing, it's assumed
    // to contain a protocol. If the protocol isn't "http:", to hell with it.

    if (!FIsAbsURL(pstrDestURL)) // VID6 - bug 22895
        goto Copy;

    pstrURL = pstrDestURL;
    if (wcschr(pstrDestURL, L':'))
    {
        // Check the protocol against the two we understand. If it is some other thing,
        // we punt.

        if (wcsncmp(pstrDestURL, L"http:", 5) != 0)
        {
            if (wcsncmp(pstrDestURL, L"file:", 5) != 0)
                goto Copy;

            // File URLs are normalized by skipping any '\\server\share' part.

            fFileURL = true;
            pstrURL = SkipFile(pstrDestURL + 5); // 5 skips the 'file:' prefix
        }
        else if (pstrDestProject != NULL)
        {
            // Project-relative URLs had better match the project prefix.

            cch = wcslen(pstrDestProject);
            if (_wcsnicmp(pstrDestURL, pstrDestProject, cch) != 0)
                goto Copy;

            // Project-relative URLs are normalized by skipping the project prefix.

            pstrURL = pstrDestURL + cch - 1;
            ASSERT(*pstrURL == '/');
        }
        else
        {
            // Generic 'http:' URLs skip the server part only.

            pstrURL = SkipServer(pstrDestURL);
            ASSERT(*pstrURL == '/');
        }

        if (!pstrURL)
            goto Copy;
        fAbsoluteURL = true;
    }

    // If the folder contains an 'http:' prefix, then find the server and skip that part.
    // otherwise it's assumed the folder is already in a project-relative format.

    pstrFolder = pstrDestFolder;
    if (wcsncmp(pstrDestFolder, L"file://", 7) == 0)
    {
        if (!fFileURL)
            goto Copy;

        pstrFolder = SkipFile(pstrDestFolder + 5);
        fAbsoluteFolder = true;
    }
    else if (wcsncmp(pstrDestFolder, L"http://", 7) == 0)
    {
        if (pstrDestProject != NULL)
        {
            // If a project was passed in, make sure the place we're relativizing to has the same path.
            // If they don't match, we're in trouble.

            cch = wcslen(pstrDestProject);
            if (_wcsnicmp(pstrDestFolder, pstrDestProject, cch) != 0)
                goto Copy;
            pstrFolder = pstrDestFolder + cch - 1;
        }
        else
        {
            pstrFolder = SkipServer(pstrDestFolder);
        }
        ASSERT(pstrFolder);
        ASSERT(*pstrFolder == '/');
        fAbsoluteFolder = true;
    }

    // If both the URL and the folder had absolute paths, we need to ensure
    // that the servers are the same.

    if (fAbsoluteFolder && fAbsoluteURL)
    {
        if (pstrURL - pstrDestURL != pstrFolder - pstrDestFolder ||
                _wcsnicmp(pstrDestURL, pstrDestFolder, SAFE_PTR_DIFF_TO_INT(pstrURL - pstrDestURL)) != 0)
            goto Copy;
    }

    // From now on, ignore the item at the end of pstrFolder

    pchLastSlash = FindLastSlash(pstrFolder);
    ASSERT(pchLastSlash);
    cchFolder = 1 + SAFE_PTR_DIFF_TO_INT(pchLastSlash - pstrFolder);

    // Both folder and item are now relative to the server root.

    // Locate the last slash in the URL. 

    pchLastSlash = FindLastSlash(pstrURL);

    if (pchLastSlash == NULL)
        ichLastSlash = 0;
    else
        ichLastSlash = 1 + SAFE_PTR_DIFF_TO_INT(pchLastSlash - pstrURL);

    // Find any common directories. 

    cch = min(cchFolder, ichLastSlash);
    ichLastSlash = -1;
    for (i = 0; i < cch && pstrFolder[i] == pstrURL[i]; ++i)
    {
        if (IsDirSep(pstrFolder[i]))
            ichLastSlash = i;
    }

    // ichLastSlash should point beyond at last slash of the last common folder.

    // For each remaining slash, append a ../ to the path.

    for (; i < cchFolder; ++i)
    {
        if (IsDirSep(pstrFolder[i]))
        {
            strWork += (fFileURL ? L"..\\" : L"../");
        }
    }

    if (-1 == ichLastSlash)
    {   // no common parts, append all of the destination
        strWork += pstrURL;
    }
    else
    {   // append only the non-match part of the destination

        strWork += (pstrURL + ichLastSlash + 1);
    }


Cleanup:
    *pbstrRelativeURL = strWork.Copy();
    if (!*pbstrRelativeURL && ::wcslen(strWork) > 0)
        hr = E_OUTOFMEMORY;

    return hr;

Copy:
    strWork = pstrDestURL;
    goto Cleanup;
}




long CTriEditParse::m_bInit = 0;

CTriEditParse::CTriEditParse()
{
    m_rgSublang = 0;
    m_fHasTitleIn = FALSE;
    m_hgPTDTC = NULL;
    m_cchPTDTC = 0;
    m_ichBeginHeadTagIn = -1;
    m_ispInfoBase = 0;

    if(0 == m_bInit++)
        InitSublanguages();
}

CTriEditParse::~CTriEditParse()
{
    // save last variant as default if it's not ASP
    if (NULL != m_rgSublang)
    {
        for( int i= 0; NULL != m_rgSublang[i].szSubLang; i++)
        {
            delete [] (LPTSTR)(m_rgSublang[i].szSubLang);
        }
        delete [] m_rgSublang;
    }
    ASSERT(0 != m_bInit);

    if(0 == --m_bInit)
    {
        ATLTRACE(_T("Releasing tables\n"));

        // delete dynamically allocated tables
        for (int i = 0; NULL != g_arpTables[i]; i++)
            delete g_arpTables[i];
        delete g_pTabDefault;

        m_bInit = 0;
    }

}


// copied from CColorHtml::NextToken
STDMETHODIMP CTriEditParse::NextToken
(
    LPCWSTR pszText,
    UINT    cbText,
    UINT*   pcbCur,
    DWORD*  pLXS,
    TXTB*   pToken
)
{
    ASSERT(pszText != NULL);
    ASSERT(pcbCur != NULL);
    ASSERT(pLXS != NULL);
    ASSERT(pToken != NULL);
    USES_CONVERSION;

    if(pszText == NULL || pcbCur == NULL || pLXS == NULL || pToken == NULL)
        return E_INVALIDARG;

    if(0 == cbText)
        return S_FALSE;

    SetTable(*pLXS); // set g_pTable according to state

#ifdef _UNICODE
    *pcbCur = GetToken(pszText, cbText, *pcbCur, pLXS, *pToken);
#else   // _UNICODE
    int     cch;
    LPTSTR  pszTemp;

    // get the converted length
    cch = WideCharToMultiByte(CP_ACP, 0, pszText, cbText,
        NULL, 0, NULL, NULL);
    pszTemp = new char[cch + 1];

    ZeroMemory(pszTemp, cch + 1);
    // copy the wide char to multibyte
    WideCharToMultiByte(CP_ACP, 0, pszText, cbText, pszTemp, cch,
        NULL, NULL);

    *pcbCur = GetToken(pszTemp, cch, *pcbCur, pLXS, *pToken);

    delete [] pszTemp;
#endif  // _UNICODE

    return (*pcbCur < cbText) ? NOERROR : S_FALSE;
}



// set g_pTable according to state
void CTriEditParse::SetTable(DWORD lxs)
{
    ASSERT(SubLangIndexFromLxs(lxs) < sizeof g_arpTables/sizeof g_arpTables[0]);
    g_pTable = g_arpTables[SubLangIndexFromLxs(lxs)];

    ASSERT(g_pTable != NULL);
}

void CTriEditParse::InitSublanguages()
{
    g_pTabDefault = new CStaticTableSet(ALL, IDS_HTML);
    int cl = CV_FIXED;
    CTableSet * rgpts[CV_MAX +1];
    memset(rgpts, 0, sizeof rgpts);

    CTableSet* ptabset; // current
    CTableSet* ptabBackup; // backup default

    memset(g_arpTables, 0, sizeof g_arpTables);

    m_rgSublang = new SUBLANG[cl+2]; // 0th + list + empty terminator
    ASSERT(NULL != m_rgSublang);
    memset(m_rgSublang, 0, (cl+2)*sizeof SUBLANG);

    UINT iLang = 1;
    TCHAR strDefault[2048];

    // Microsoft browsers
    // Internet Explorer 3
    ptabset = MakeTableSet(rgpts, IEXP3, IDS_IEXP3);
    SetLanguage( strDefault, m_rgSublang, ptabset, iLang, IDR_HTML, CLSID_NULL );

    // Set backup default as IE 3
    ptabBackup = ptabset;
    if (lstrlen(strDefault) == 0)
    {
        ASSERT(lstrlen(ptabBackup->Name()) != 0);
        lstrcpy(strDefault, ptabBackup->Name());
    }

    // User's additions

    for (int n = 0; rgpts[n]; n++)
    {
        ptabset = rgpts[n];
        SetLanguage( strDefault, m_rgSublang, ptabset, iLang, 0, CLSID_NULL );
        ptabBackup = ptabset;
    }

    // HTML 2.0 base (if not overridden)
    {
        TCHAR strHTML2[2048];
        ::LoadString(   _Module.GetModuleInstance(),
                        IDS_RFC1866,
                        strHTML2,
                        sizeof(strHTML2)
                        );
        if (!FindTable(rgpts,strHTML2))
        {
            ptabset = new CStaticTableSet(HTML2, IDS_RFC1866);
            SetLanguage( strDefault, m_rgSublang, ptabset, iLang, 0, CLSID_NULL);
        }
    }

    if (NULL == g_arpTables[0])
    {
        ASSERT(NULL != ptabBackup); // error: didn't get a default!

        //Find the backup in the tables
        int i;
        for (i = 1; NULL != g_arpTables[i]; i++)
        {
            if (g_arpTables[i] == ptabBackup)
                break;
        }

        ASSERT(NULL != g_arpTables[i]); // must be in table

        // Set default
        g_arpTables[0] = g_pTable = g_arpTables[i];
        m_rgSublang[0] = m_rgSublang[i];
        m_rgSublang[0].lxsInitial = LxsFromSubLangIndex(0);

        // Move the rest down to fill the hole
        for (; g_arpTables[i]; i++)
        {
            g_arpTables[i] = g_arpTables[i+1];
            m_rgSublang[i] = m_rgSublang[i+1];
            m_rgSublang[i].lxsInitial = LxsFromSubLangIndex(i);
        }
    }
    ASSERT(NULL != g_arpTables[0]);

    // set global ASP sublang ptr
    // start at 1, since the default is at 0, and should never be ASP
    for (int i = 1; NULL != m_rgSublang[i].szSubLang; i++)
    {
        if (m_rgSublang[i].nIdTemplate == IDR_ASP)
        {
            g_psublangASP = &m_rgSublang[i];
            break;
        }
    }
}

// Reallocs are expensive, so when we Realloc, should we add some more pad so that 
// we wont have to call Realloc very often?

HRESULT
ReallocBuffer(HGLOBAL *phg, DWORD cbNew, UINT uFlags)
{
    HRESULT hr = S_OK;

    ASSERT(*phg != NULL);
    ASSERT(cbNew != 0); // will we ever get this?
    GlobalUnlock(*phg);
    *phg = GlobalReAlloc(*phg, cbNew, uFlags);
    if (*phg == NULL)
    {
#ifdef DEBUG
        hr = GetLastError();
#endif // DEBUG
        hr = E_OUTOFMEMORY;
    }

    return(hr);
} /* ReallocBuffer() */

HRESULT
ReallocIfNeeded(HGLOBAL *phg, WCHAR **ppwNew, UINT cbNeed, UINT uFlags)
{
    HRESULT hr = S_OK;

    ASSERT(*phg != NULL);
    if (GlobalSize(*phg) < cbNeed)
    {
        hr = ReallocBuffer(phg, cbNeed, uFlags);
        if (hr == E_OUTOFMEMORY)
            goto LRet;
        ASSERT(*phg != NULL);
        *ppwNew = (WCHAR *)GlobalLock(*phg);
    }
LRet:
    return(hr);

} /* ReallocIfNeeded() */

void
CTriEditParse::fnRestoreSSS(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
             TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft, 
             INT *pcSSSOut, UINT *pichNewCur, UINT *pichBeginCopy,
             DWORD /*dwFlags*/)
{
    // Server Side Script case
    // This occurs inside <%  %>. we assume simple SSS
    // remove the added <SCRIPT LANGUAGE=SERVERASP> & </SCRIPT> text around it
    UINT iArray = *piArrayStart;
    INT i;
    UINT ichScrStart, ichScrEnd, indexScrStart, indexScrEnd;
    UINT ichSSSStart, ichSSSEnd;
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT cSSSOut = *pcSSSOut;
    LPCWSTR szSSS[] = {L"SERVERASP", L"\"SERVERASP\""};
    LPCWSTR szSSSSp[] = {L"SERVERASPSP"};
    BOOL fSpecialSSS = FALSE;
    LPWSTR pwNew = *ppwNew;
    INT iMatchMax;
    UINT cbNeed;
    UINT ichScrWspBegin, ichScrWspEnd, ichSp;

    ASSERT(cSSSOut >= 0); // make sure that this was initilized
    if (cSSSOut == 0)
        goto LRetOnly;

    //{TokTag_START, TokElem_SCRIPT, TokTag_CLOSE, TokElem_SCRIPT, fnRestoreSSS}
    ichScrStart = ichScrEnd = indexScrStart = indexScrEnd = ichSSSStart = ichSSSEnd = 0;
    ichScrWspBegin = ichScrWspEnd = 0;
    while (cSSSOut > 0)
    {
        // start at iArray of pTokArray and look for TokElem_SCRIPT
        //while (pTokArray[iArray].token.tok != ft.tokBegin2)
        //  iArray++;
        ASSERT(iArray < ptep->m_cMaxToken);
        if (pTokArray[iArray].token.tok != TokElem_SCRIPT)
            goto LRet;

        // Here's the deal - we have to ignore all SSS that appear
        // as values inside client scripts or insize objects/dtcs
        // so, we need to skip this TokElem_SCRIPT tag if we found '</' before TokElem_SCRIPT
        if (   pTokArray[iArray].token.tok == TokElem_SCRIPT
            && pTokArray[iArray-1].token.tok != TokTag_START
            )
        {
            ASSERT(pTokArray[iArray].token.tokClass == tokElem);
            iArray++; // so that we don't come here again with the same iArray
            ptep->m_fDontDeccItem = TRUE; // we can do things differently here next time around
            ptep->m_cSSSOut++;
            goto LRet;
        }

        //ASSERT(pTokArray[iArray].token.tok == TokElem_SCRIPT);
        i = iArray; // the position at which we found ft.tokBegin2
        // look for the special LANGUAGE arrtibute that we had set.
        // if that doesn't exist, this is not the SSS we want
        // we don't really need to look for this till ptep->m_cMaxToken,
        // but this will cover boundary cases
        iMatchMax = (pTokArray[iArray].iNextprev == -1)? ptep->m_cMaxToken : pTokArray[iArray].iNextprev;
        while (i < iMatchMax)
        {
            if (pTokArray[i].token.tok == TokAttrib_LANGUAGE)
            {
                ASSERT(pTokArray[i].token.tokClass == tokAttr);
                break;
            }
            i++;
        }
        if (i < iMatchMax)
        {
            // make sure that the next one is tokOpEqual
            ASSERT(pTokArray[i+1].token.tokClass == tokOp);
            //ASSERT(((pwOld+pTokArray[i+1].token.ibTokMin)*sizeof(WCHAR)) == '=');
            // get the next value and compare it with szSSS[]
            // note that this may also match with szSSSSp[]
            if (   0 != _wcsnicmp(szSSS[0], &pwOld[pTokArray[i+2].token.ibTokMin], wcslen(szSSS[0]))
                && 0 != _wcsnicmp(szSSS[1], &pwOld[pTokArray[i+2].token.ibTokMin], wcslen(szSSS[1]))
                )
            {
                iArray = i;
                goto LNextSSS; // not this one
            }
        }
        else // error case
        {
            iArray++;
            goto LRet;
        }
        // compare with szSSSSp[] and set fSpecialSSS
        if (0 == _wcsnicmp(szSSSSp[0], &pwOld[pTokArray[i+2].token.ibTokMin], wcslen(szSSSSp[0])))
            fSpecialSSS = TRUE;
        i = iArray; // we are OK, so lets look for < before SCRIPT tag
        while (i >= 0)
        {
            // do we need to do anything else here?
            if (pTokArray[i].token.tok == ft.tokBegin)
            {
                ASSERT(pTokArray[i].token.tok == TokTag_START);
                ASSERT(pTokArray[i].token.tokClass == tokTag);
                break;
            }
            i--;
        }
        if (i >= 0) // found TokTag_START token
        {
            ichScrStart = pTokArray[i].token.ibTokMin;
            indexScrStart = i;
        }
        else // error case
        {
            // we found SCRIPT, but didn't find < of <SCRIPT
            // we can't process this SSS, so quit
            goto LRet;
        }

        // now lets look for <! that would be after <SCRIPT LANGUAGE = SERVERASP>
        while (i < (int)ptep->m_cMaxToken)
        {
            if (   pTokArray[i].token.tok == TokTag_CLOSE
                && pTokArray[i].token.tokClass == tokTag
                )
                ichScrWspBegin = pTokArray[i].token.ibTokMac; // if we had saved white space, it would begin here

            if (pTokArray[i].token.tok == TokTag_BANG)
            {
                ASSERT(pTokArray[i].token.tokClass == tokTag);
                ASSERT(pTokArray[i+1].token.tokClass == tokComment);
                //we can assert for next 2 chars as --
                ichSSSStart = pTokArray[i].token.ibTokMin;
                break;
            }
            i++;
        }
        if (i >= (int)ptep->m_cMaxToken) // didn't find <!
        {
            goto LRet;
        }
        // look for ending -->
        while (i < (int)ptep->m_cMaxToken)
        {
            if (pTokArray[i].token.tok == TokTag_CLOSE && pTokArray[i].token.tokClass == tokTag)
            {
                //we can assert for next 2 chars as --
                ASSERT(*(pwOld+pTokArray[i].token.ibTokMin-1) == '-');
                ASSERT(*(pwOld+pTokArray[i].token.ibTokMin-2) == '-');
                ichSSSEnd = pTokArray[i].token.ibTokMac;
                break;
            }
            i++;
        }
        if (i >= (int)ptep->m_cMaxToken) // didn't find >
        {
            goto LRet;
        }

        // now look for ft.tokEnd2 & ft.tokEnd (i.e. TokElem_SCRIPT & >)
        while (pTokArray[i].token.tok != ft.tokEnd2)
        {
            if (pTokArray[i].token.tok == TokTag_END && pTokArray[i].token.tokClass == tokTag)
                ichScrWspEnd = pTokArray[i].token.ibTokMin; // past the last white space
            i++;
        }
        ASSERT(i < (int)ptep->m_cMaxToken);
        ASSERT(pTokArray[i].token.tok == TokElem_SCRIPT);
        ASSERT(pTokArray[i].token.tokClass == tokElem);
        // go forward and look for > of SCRIPT>
        // as additional check, we can also check that previous token is </
        while (i < (int)ptep->m_cMaxToken)
        {
            if (pTokArray[i].token.tok == ft.tokEnd)
            {
                ASSERT(pTokArray[i].token.tok == TokTag_CLOSE);
                ASSERT(pTokArray[i].token.tokClass == tokTag);
                break;
            }
            i++;
        }
        if (i < (int)ptep->m_cMaxToken) // found TokTag_CLOSE
        {
            ichScrEnd = pTokArray[i].token.ibTokMac;
            indexScrEnd = i;
        }
        else // error case
        {
            // we found SCRIPT, but didn't find > of SCRIPT>
            // we can't process this SSS, so quit
            goto LRet;
        }
        iArray = i+1; // set it for next run

        cbNeed = (ichNewCur+(ichScrStart-ichBeginCopy)+(ichSSSEnd-ichSSSStart))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        // do the Blts
        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew
        // copy from ichBeginCopy to begining of SSS
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichScrStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichScrStart-ichBeginCopy);
        ichBeginCopy = ichScrEnd; // make it ready for next copy

        if (fSpecialSSS)
        {
            // in special case, we need to make space for the <%@...%> at the begining of pwNew
            // so, we move all the above stuff (ichNewCur chars) by (ichSSSEnd-ichSSSStart-3).
            memmove((BYTE *)(&pwNew[ichSSSEnd-ichSSSStart-3]),
                    (BYTE *)pwNew,
                    (ichNewCur)*sizeof(WCHAR)
                    );
            // we now copy <%@...%> at the begining of the doc instead of at ichNewCur
            // now skip <SCRIPT LANGUAGE=SERVERASP> & only copy <% ....%>
            // note that we have to get rid of 3 extra chars we had added when we converted going in Trident
            memcpy( (BYTE *)(pwNew),
                    (BYTE *)&pwOld[ichSSSStart+2],/*get rid of 2 extra chars we had added at the begining*/
                    (ichSSSEnd-ichSSSStart-3)*sizeof(WCHAR));
            pwNew[0] = '<'; pwNew[1] = '%'; // note that we have moved the SSS to the begining of the doc
            ichNewCur += ichSSSEnd-ichSSSStart-3; // here we got rid of 1 extra char that was added
            pwNew[(ichSSSEnd-ichSSSStart-3)-2] = '%';
            pwNew[(ichSSSEnd-ichSSSStart-3)-1] = '>';
            // change <!-- to <% and --> to %>
        }
        else
        {
            // in pwNew get rid of white space characters from ichNewCur backwards
            ichSp = ichNewCur-1;
            while (    (ichSp >= 0)
                    && (   pwNew[ichSp] == ' '  || pwNew[ichSp] == '\r' 
                        || pwNew[ichSp] == '\n' || pwNew[ichSp] == '\t'
                        )
                    )
            {
                ichSp--;
            }
            ichSp++; // compensate for the last decrement, ichSp points to the 1st white-space character
            ichNewCur = ichSp;
            // copy pre-script white space
            if (ichScrWspBegin > 0 && ichSSSStart > ichScrWspBegin) // has been set
            {
                memcpy( (BYTE *)&pwNew[ichNewCur], 
                        (BYTE *)&pwOld[ichScrWspBegin],
                        (ichSSSStart-ichScrWspBegin)*sizeof(WCHAR));
                ichNewCur += ichSSSStart-ichScrWspBegin;
            }
            // now skip <SCRIPT LANGUAGE=SERVERASP> & only copy <% ....%>
            // note that we have to get rid of 3 extra chars we had added when we converted going in Trident
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[ichSSSStart+2]),/*get rid of 2 extra chars we had added at the begining*/
                    (ichSSSEnd-ichSSSStart-3)*sizeof(WCHAR));
            pwNew[ichNewCur] = '<';
            pwNew[ichNewCur+1] = '%'; 
            ichNewCur += ichSSSEnd-ichSSSStart-3; // here we got rid of 1 extra char that was added
            pwNew[ichNewCur-2] = '%';
            pwNew[ichNewCur-1] = '>';
            // copy post-script white space
            if (ichScrWspEnd > 0 && ichScrWspEnd > ichSSSEnd) // has been set
            {
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[ichSSSEnd],
                        (ichScrWspEnd-ichSSSEnd)*sizeof(WCHAR));
                ichNewCur += ichScrWspEnd-ichSSSEnd;
            }

            // increment iArray & ichBeginCopy till the next non-whitespace token
            while (iArray < (int)ptep->m_cMaxToken)
            {
                UINT ich;
                BOOL fNonWspToken = FALSE; // assume the next token to be whitespace
                // scan entire token and see if it has all white-space characters
                for (ich = pTokArray[iArray].token.ibTokMin; ich < pTokArray[iArray].token.ibTokMac; ich++)
                {
                    if (   pwOld[ich] != ' '    && pwOld[ich] != '\t'
                        && pwOld[ich] != '\r'   && pwOld[ich] != '\n'
                        )
                    {
                        fNonWspToken = TRUE;
                        break;
                    }
                }
                if (fNonWspToken)
                {
                    ichBeginCopy = pTokArray[iArray].token.ibTokMin;
                    break;
                }
                iArray++;
            }
        }

        cSSSOut--;
    } // while (cSSSOut > 0)

LNextSSS:
LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;
LRetOnly:
    return;

} /* fnRestoreSSS() */

void
CTriEditParse::fnSaveSSS(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
             TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft, 
             INT *pcSSSIn, UINT *pichNewCur, UINT *pichBeginCopy,
             DWORD /*dwFlags*/)
{
    // Server Side Script case
    // This occur inside <%  %>. We assume simple SSS
    // add <SCRIPT LANGUAGE=SERVERASP> & </SCRIPT> around it
    // tag used for saving the SSS.
    /* 2 spaces at the end of 1st element are important */
    LPCWSTR rgSSSTags[] =
    {
        L"\r\n<SCRIPT LANGUAGE=\"SERVERASP\">",
        L"\r\n<SCRIPT LANGUAGE=\"SERVERASPSP\">",
        L"</SCRIPT>\r\n"
    };
    UINT iArray = *piArrayStart;
    UINT i;
    UINT ichSSSStart, ichSSSEnd, indexSSSStart, indexSSSEnd;
    HGLOBAL hgSSS = NULL;
    WCHAR *pSSS;
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT cSSSIn = *pcSSSIn;
    LPWSTR pwNew = *ppwNew;
    int indexSSSTag;
    UINT cbNeed;
    UINT ichSp;

    ASSERT(cSSSIn >= 0); // make sure that this was initilized
    if (cSSSIn == 0)
        goto LRetOnly;
    
    ichSSSStart = ichSSSEnd = indexSSSStart = indexSSSEnd = 0;

    while (cSSSIn > 0)
    {
        INT cbMin = 0x4fff; // init & increment size of hgSSS
        INT cchCurSSS = 0;
        int index;

        // handle special case here - if the script is inside <xmp> tag, we shouldn't convert the script
        // NOTE that we are only handling <xmp> <%...%> </xmp> case here
        // we don't have to worry about nested xmp's because its not valid html.
        // such invalid cases are <xmp>...<xmp> </xmp> <% %> </xmp> OR <xmp>...<xmp> <% %> </xmp> </xmp>
        // handle TokElem_PLAINTEXT as well
        index = iArray;
        while (index >= 0)
        {
            if (   (pTokArray[index].token.tok == TokElem_XMP || pTokArray[index].token.tok == TokElem_PLAINTEXT)
                && pTokArray[index].token.tokClass == tokElem
                && pTokArray[index].iNextprev > iArray
                )
            {
                iArray++;
                goto LRet;
            }
            index--;
        }

        // start at the begining of pTokArray and look for first <%
        ASSERT(ft.tokBegin2 == -1);
        ASSERT(ft.tokEnd2 == -1);
        // Here both supporting tokens are -1, so we simply look for main tokens.
        i = iArray;
        while (i < ptep->m_cMaxToken)
        {
            // do we need to do anything else here?
            if (pTokArray[i].token.tok == ft.tokBegin)
            {
                ASSERT(pTokArray[i].token.tok == TokTag_SSSOPEN);
                ASSERT(pTokArray[i].token.tokClass == tokSSS);
                break;
            }
            i++;
        }
        if (i < ptep->m_cMaxToken) // found TokTag_SSSOPEN token
        {
            ichSSSStart = pTokArray[i].token.ibTokMin;
            indexSSSStart = i;
        }

        // look for ft.tokEnd
        if (pTokArray[i].iNextprev != -1)
        {
            // NOTE that this will give us topmost nested level of the SSS
            indexSSSEnd = pTokArray[i].iNextprev;
            ichSSSEnd = pTokArray[indexSSSEnd].token.ibTokMac;
            ASSERT(indexSSSEnd < ptep->m_cMaxToken);
            // this will be a wierd case where the iNextprev is incorrectly pointing to another token
            // but lets handle that case.
            if (pTokArray[indexSSSEnd].token.tok != TokTag_SSSCLOSE)
                goto LFindSSSClose; // find it by looking at each token
        }
        else // actually, this is an error case, but rather than just giving assert, try to find the token
        {
LFindSSSClose:
            while (i < ptep->m_cMaxToken)
            {
                if (pTokArray[i].token.tok == ft.tokEnd)
                {
                    ASSERT(pTokArray[i].token.tok == TokTag_SSSCLOSE);
                    ASSERT(pTokArray[i].token.tokClass == tokSSS);
                    break;
                }
                i++;
            }
            if (i < ptep->m_cMaxToken) // found TokTag_SSSCLOSE token
            {
                ichSSSEnd = pTokArray[i].token.ibTokMac;
                indexSSSEnd = i;
            }
            else // error case 
            {
                goto LRet; // didn't find %>, but exhausted the token array
            }
        }
        iArray = indexSSSEnd; // set for for next SSS

        // now insert text from rgSSSTags[] into the source
        // 0. Allocate a local buffer
		cbNeed =	wcslen(rgSSSTags[0]) + wcslen(rgSSSTags[0]) + wcslen(rgSSSTags[2])
					+ (ichSSSEnd-ichSSSStart) + cbMin;
		hgSSS = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbNeed*sizeof(WCHAR));
        if (hgSSS == NULL)
            goto LErrorRet;
        pSSS = (WCHAR *) GlobalLock(hgSSS);
        ASSERT(pSSS != NULL);

        // NOTE - This flag would have been set to TRUE only if, 
        // we have found <%@ as the 1st SSS in the document
        indexSSSTag = 0;
        if (ptep->m_fSpecialSSS)
        {
            ptep->m_fSpecialSSS = FALSE;
            indexSSSTag = 1;
        }

        //-------------------------------------------------------------------------------
        // ASSUMPTION - The big assumption we are making is that IE5 doesn't change 
        // anything inside the client sctipt. So far we have seen that.
        // In the worst case, if they start mucking with the contents of client script, 
        // we will loose the spacing, but there will be NO DATA LOSS.
        // 
        // Based on this assumption, we simply save the pre-post script spacing as is
        // and expect to restore it on the way out.
        //-------------------------------------------------------------------------------

        // 1. Insert <SCRIPT> from rgSSSTags[indexSSSTag]
        wcscpy(&pSSS[cchCurSSS], rgSSSTags[indexSSSTag]);
        cchCurSSS += wcslen(rgSSSTags[indexSSSTag]);
        // insert the white space as it occurs in pwOld, ichSSSStart is '<' of '<%', walk backwards
        ichSp = ichSSSStart-1;
        while (    (ichSp >= 0)
                && (   pwOld[ichSp] == ' '  || pwOld[ichSp] == '\r' 
                    || pwOld[ichSp] == '\n' || pwOld[ichSp] == '\t'
                    )
                )
        {
            ichSp--;
        }
        ichSp++; // compensate for the last decrement
        if ((int)(ichSSSStart-ichSp) > 0)
        {
            wcsncpy(&pSSS[cchCurSSS], &pwOld[ichSp], ichSSSStart-ichSp);
            cchCurSSS += ichSSSStart-ichSp;
        }
        // now add TokTag_BANG '<!'
        pSSS[cchCurSSS++] = '<';
        pSSS[cchCurSSS++] = '!';
        // 2. copy the script from pwOld
        wcsncpy(&pSSS[cchCurSSS], &pwOld[ichSSSStart], ichSSSEnd-ichSSSStart);
        pSSS[cchCurSSS] = '-';
        pSSS[cchCurSSS+1] = '-';
        cchCurSSS += (ichSSSEnd-ichSSSStart);
        pSSS[cchCurSSS] = pSSS[cchCurSSS-1]; //note : -1 is '>'
        pSSS[cchCurSSS-2] = '-';
        pSSS[cchCurSSS-1] = '-';
        cchCurSSS++; // we are adding one extra character
        // insert the white space as it occurs in pwOld, ichSSSEnd is past '%>', walk forward
        ichSp = ichSSSEnd;
        while (    (ichSp < pTokArray[ptep->m_cMaxToken-1].token.ibTokMac-1)
                && (   pwOld[ichSp] == ' '  || pwOld[ichSp] == '\r' 
                    || pwOld[ichSp] == '\n' || pwOld[ichSp] == '\t'
                    )
                )
        {
            ichSp++;
        }
        if ((int)(ichSp-ichSSSEnd) > 0)
        {
            wcsncpy(&pSSS[cchCurSSS], &pwOld[ichSSSEnd], ichSp-ichSSSEnd);
            cchCurSSS += ichSp-ichSSSEnd;
        }
        // 3. Insert </SCRIPT> from rgSSSTags[2]
        wcscpy(&pSSS[cchCurSSS], rgSSSTags[2]);
        cchCurSSS += wcslen(rgSSSTags[2]);



        /* REALLOCATE pwNew IF NEEDED here, use cache value for GlobalSize(*phgNew) and don't forget to update it too */
        cbNeed = (ichNewCur+(ichSSSStart-ichBeginCopy)+(cchCurSSS))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LErrorRet;

                    
        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew

        if ((int)(ichSSSStart-ichBeginCopy) >= 0)
        {
            // copy till begining of the <%
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[ichBeginCopy]),
                    (ichSSSStart-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += ichSSSStart-ichBeginCopy;
            ichBeginCopy = ichSSSEnd; // set it for next script

            // copy the converted SSS
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(pSSS),
                    cchCurSSS*sizeof(WCHAR));
            ichNewCur += cchCurSSS;
        }

        if (hgSSS != NULL)
            GlobalUnlockFreeNull(&hgSSS);

        cSSSIn--;
    } // while(cSSSIn > 0)

LErrorRet:
    if (hgSSS != NULL)
        GlobalUnlockFreeNull(&hgSSS);
LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

LRetOnly:   
    return;

} /* fnSaveSSS() */

void
CTriEditParse::fnRestoreDTC(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
             TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft, 
             INT *piObj, UINT *pichNewCur, UINT *pichBeginCopy,
             DWORD dwFlags)
{
    // OBJECTS case - (These were converted from DTCs in modeInput
    // if we get OBJECT, search backwards (carefully) for tokTag/TokTag_START (<) in pTokArray
    // once we find that, remember the ibTokMin for Object conversion
    // look for the /OBJECT (i.e. look for OBJECT and look for previous /) and
    // once we get those two next to each other, wait for upcoming toktag_CLOSE which will end that Object
    // remember ibTokMac at that position. This is the OBJECT range.
    // First, insert the startspan text
    // Then generate and insert the endspan text (note that we may have to extend our
    // buffer becausethe generated text mey not fit.
    // Do the appropriate Blts to adjust the buffer.

    UINT cchObjStart, indexObjStart, cchObjEnd, indexObjEnd;
    HGLOBAL hgDTC = NULL;
    WCHAR *pDTC;
    UINT iArray = *piArrayStart;
    INT i;
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    HRESULT hr;
    LPWSTR pwNew = *ppwNew;
    UINT cbNeed;

    long iControlMac;
    CComPtr<IHTMLDocument2> pHTMLDoc;
    CComPtr<IHTMLElementCollection> pHTMLColl;
    CComPtr<IDispatch> pDispControl;
    CComPtr<IActiveDesigner> pActiveDesigner;
    VARIANT vaName, vaIndex;
    // DTC tag used for saving the DTC.
    LPCWSTR rgDTCTags[] =
    {
        L"<!--METADATA TYPE=\"DesignerControl\" startspan\r\n",
        L"\r\n-->\r\n",
        L"\r\n<!--METADATA TYPE=\"DesignerControl\" endspan-->"
    };
    LPCWSTR rgCommentRT[] =
    {
        L"DTCRUNTIME",
        L"--DTCRUNTIME ",
        L" DTCRUNTIME--",
    };
    int ichRT, cchRT, ichRTComment, cchRTComment, indexRTComment;

    ichRTComment = ichRT = -1;
    indexRTComment = -1;
    cchRT = cchRTComment = 0;

    cchObjStart = indexObjStart = cchObjEnd = indexObjEnd = 0;

    // start at the begining of pTokArray and look for first OBJECT
    //while (pTokArray[iArray].token.tok != ft.tokBegin2)
    //  iArray++;
    ASSERT(iArray < ptep->m_cMaxToken);

    if (pTokArray[iArray].token.tok != TokElem_OBJECT)
        goto LRet;

    //ASSERT(pTokArray[iArray].token.tok == TokElem_OBJECT);
    i = iArray; // the position at which we found ft.tokBegin2
    while (i >=0)
    {
        // do we need to do anything else here?
        if (pTokArray[i].token.tok == ft.tokBegin)
        {
            ASSERT(pTokArray[i].token.tok == TokTag_START);
            ASSERT(pTokArray[i].token.tokClass == tokTag);
            break;
        }
        i--;
    }
    if (i >= 0) // found TokTag_START token
    {
        cchObjStart = pTokArray[i].token.ibTokMin;
        indexObjStart = i;
    }
    i = pTokArray[iArray].iNextprev;
    if (i == -1) // no matching end, skip this <OBJECT>
        goto LRet;
    ASSERT(pTokArray[pTokArray[iArray].iNextprev].token.tok == TokElem_OBJECT);
    ASSERT(pTokArray[pTokArray[iArray].iNextprev].token.tokClass == tokElem);
    ASSERT(pTokArray[i-1].token.tok == TokTag_END);
    // from this ith position, look for ft.tokEnd
    while (i < (int)ptep->m_cMaxToken)
    {
        if (pTokArray[i].token.tok == ft.tokEnd)
        {
            ASSERT(pTokArray[i].token.tok == TokTag_CLOSE);
            ASSERT(pTokArray[i].token.tokClass == tokTag);
            break;
        }
        i++;
    }
    if (i < (int)ptep->m_cMaxToken) // found TokTag_CLOSE token
    {
        cchObjEnd = pTokArray[i].token.ibTokMac;
        indexObjEnd = i;
    }
    
    // look for the special comment that has the runtime text saved
    // we will need it if SaveRuntimeText() failed
    i = indexObjStart;
    while (i < (int)indexObjEnd)
    {
        if (   pTokArray[i].token.tok == TokTag_BANG
            && pTokArray[i].token.tokClass == tokTag)
        {
            // found the comment, now make sure that this is the comment with DTCRUNTIME
            if (   (pwOld[pTokArray[i+1].token.ibTokMin] == '-')
                && (pwOld[pTokArray[i+1].token.ibTokMin+1] == '-')
                && (0 == _wcsnicmp(rgCommentRT[0], &pwOld[pTokArray[i+1].token.ibTokMin+2], wcslen(rgCommentRT[0])))
                && (pwOld[pTokArray[i+1].token.ibTokMac-1] == '-')
                && (pwOld[pTokArray[i+1].token.ibTokMac-2] == '-')
                && (0 == _wcsnicmp(rgCommentRT[0], &pwOld[pTokArray[i+1].token.ibTokMac-2-wcslen(rgCommentRT[0])], wcslen(rgCommentRT[0])))
                )
            {
                ichRT = pTokArray[i+1].token.ibTokMin + wcslen(rgCommentRT[1]);
                cchRT = pTokArray[i+1].token.ibTokMac-pTokArray[i+1].token.ibTokMin - wcslen(rgCommentRT[2]) - wcslen(rgCommentRT[1]);
                indexRTComment = i;
                ichRTComment = pTokArray[i].token.ibTokMin;
                cchRTComment = pTokArray[i+2].token.ibTokMac-pTokArray[i].token.ibTokMin;
                break;
            }
        }
        i++;
    }

    iArray = indexObjEnd; // set it for the next Object

    // now, replace the OBJECT - Insert startspan and endspan stuff
    pHTMLDoc = NULL;
    hr = ptep->m_pUnkTrident->QueryInterface(IID_IHTMLDocument2, (void **) &pHTMLDoc);
    if (hr != S_OK)
        goto LErrorRet;

    pHTMLColl = NULL;
    hr = pHTMLDoc->get_applets(&pHTMLColl);
    if (hr != S_OK)
    {
        goto LErrorRet;
    }

    pHTMLColl->get_length(&iControlMac);
    ASSERT(*piObj <= iControlMac);

    hr = S_FALSE;
    VariantInit(&vaName);
    VariantInit(&vaIndex);

    V_VT(&vaName) = VT_ERROR;
    V_ERROR(&vaName) = DISP_E_PARAMNOTFOUND;

    V_VT(&vaIndex) = VT_I4;
    V_I4(&vaIndex) = *piObj;
    *piObj += 1; // get it ready for the next control
    ptep->m_iControl = *piObj; // get it ready for the next control

    pDispControl = NULL;
    hr = pHTMLColl->item(vaIndex, vaName, &pDispControl);
    // Trident has a bug that if the object was nested inside <scripts> tags,
    // it returns S_OK with pDispControl as NULL. (See VID BUG 11303)
    if (hr != S_OK || pDispControl == NULL)
    {
        goto LErrorRet;
    }
    pActiveDesigner = NULL;
    hr = pDispControl->QueryInterface(IID_IActiveDesigner, (void **) &pActiveDesigner);
    if (hr != S_OK) // release pActiveDesigner
    {
        pActiveDesigner.Release();
        pDispControl.Release();
    }

    if (hr == S_OK) // Found the control!
    {        
        // This is a DTC
        IStream *pStm;
        HGLOBAL hg = NULL;
        INT cbMin = 0x8fff; // init & increment size of hgDTC
        INT cchCurDTC = 0;

#ifdef DEBUG
        CComPtr<IHTMLElement> pHTMLElem = NULL;

        hr = pDispControl->QueryInterface(IID_IHTMLElement, (void **) &pHTMLElem);
        if (hr != S_OK)
        {   
            goto LErrorRet;
        }

        // get the index for TokAttrib_ID from pTokArray
        // from here get the actual value for future comparison

        i = indexObjStart;
        // actually, this has to exist before TokElem_PARAM,
        // but this takes care of boundary cases
        while (i < (int)indexObjEnd)
        {
            if (pTokArray[i].token.tok == TokAttrib_CLASSID)
            {
                ASSERT(pTokArray[i].token.tokClass == tokAttr);
                break;
            }
            i++;
        }

        if (i < (int)indexObjEnd -1) // found TokAttrib_CLASSID
        {
            CComPtr<IPersistPropertyBag> pPersistPropBag;
            INT ichClsid;

            // make sure that the next one is tokOpEqual
            ASSERT(pTokArray[i+1].token.tokClass == tokOp);
            // make sure that the next one is the id and get that value
            //ASSERT(pTokArray[i].token.tok == );

            // Is there any other way to skip "clsid:" string that appears before the clsid?
            ichClsid = pTokArray[i+2].token.ibTokMin + strlen("clsid:");

            pPersistPropBag = NULL;
            hr = pDispControl->QueryInterface(IID_IPersistPropertyBag, (void **) &pPersistPropBag);
            if (hr == S_OK)
            {
                CLSID clsid;
                LPOLESTR szClsid;

                if (S_OK == pPersistPropBag->GetClassID(&clsid))
                {
                    if (S_OK == StringFromCLSID(clsid, &szClsid))
                        ASSERT(0 == _wcsnicmp(szClsid+1/* for {*/, &pwOld[ichClsid], sizeof(CLSID)));
                    ::CoTaskMemFree(szClsid);
                }
            }

        }
#endif // DEBUG

        ASSERT(*piObj <= iControlMac);
        // Do the Blts. 
        // 0. Allocate a local buffer
        hgDTC = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, ((cchObjEnd-cchObjStart)+cbMin)*sizeof(WCHAR)); // stack
        if (hgDTC == NULL)
            goto LErrorRet;
        pDTC= (WCHAR *) GlobalLock(hgDTC);
        ASSERT(pDTC != NULL);

        if (!(dwFlags & dwFilterDTCsWithoutMetaTags))
        {
            INT indexTokOp = -1;
            INT indexClsId = -1;

            // 1. Insert MetaData1 tag from rgDTCTags[0]
            wcscpy(&pDTC[cchCurDTC], rgDTCTags[0]);
            cchCurDTC += wcslen(rgDTCTags[0]);

            // 2. copy the <OBJECT> </OBJECT> from pwOld

            // Split the copy into 3 parts...
            // part 1 - copy from cchObjStart till = following the ClassId
            // part 2 - add a quote around the classId value (if needed) and copy the value
            // part 3 - copy rest of the object till cchObjEnd

            // VID98-BUG 5649 - Fix DaVinci bug by adding quote around classId's.
            // NOTE - we want to make sure that the classId value is inside quotes,
            // if there is one for this <OBJECT> tag,

            // we actually don't need to go this far, but thats the indexObjEnd is the 
            // only index know
            for (i = indexObjStart; i < (INT)indexObjEnd; i++)
            {
                if (   pTokArray[i].token.tok == TokAttrib_CLASSID
                    && pTokArray[i].token.tokClass == tokAttr)
                {
                    indexClsId = i;
                }
                if (   pwOld[pTokArray[i].token.ibTokMin] == '='
                    && pTokArray[i].token.tokClass == tokOp
                    && indexTokOp == -1)
                {
                    indexTokOp = i;
                }
            } // for ()
            // following are simply error cases, we won't run into them unless we have
            // incomplete HTML
            if (   indexClsId == -1 /* we didn't have clsid for this <OBJECT> */
                || indexTokOp == -1 /* rare but possible error case of incomplete HTML */
                )
            {
                if (ichRTComment == -1)
                {
                    wcsncpy(&pDTC[cchCurDTC], &pwOld[cchObjStart], cchObjEnd - cchObjStart);
                    cchCurDTC += cchObjEnd - cchObjStart;
                }
                else
                {
                    wcsncpy(&pDTC[cchCurDTC], &pwOld[cchObjStart], ichRTComment - cchObjStart);
                    cchCurDTC += ichRTComment - cchObjStart;

                    wcsncpy(&pDTC[cchCurDTC], &pwOld[ichRTComment+cchRTComment], cchObjEnd - (ichRTComment+cchRTComment));
                    cchCurDTC += cchObjEnd - (ichRTComment+cchRTComment);
                }
            }
            else
            {
                LPCWSTR szClsId[] =
                {
                    L"clsid:",
                };
    
                ASSERT(indexTokOp != -1);
                // copy till '=' of 'classid=clsid:XXXX'
                memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                        (BYTE *)(&pwOld[cchObjStart]),
                        (pTokArray[indexTokOp].token.ibTokMac-cchObjStart)*sizeof(WCHAR));
                cchCurDTC += (pTokArray[indexTokOp].token.ibTokMac-cchObjStart);

                if (0 == _wcsnicmp(szClsId[0], &pwOld[pTokArray[indexTokOp+1].token.ibTokMin], wcslen(szClsId[0])))
                {
                    ASSERT(pwOld[pTokArray[indexTokOp+1].token.ibTokMin] != '"');
                    pDTC[cchCurDTC] = '"';
                    cchCurDTC++;

                    memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                            (BYTE *)(&pwOld[pTokArray[indexTokOp+1].token.ibTokMin]),
                            (pTokArray[indexTokOp+1].token.ibTokMac - pTokArray[indexTokOp+1].token.ibTokMin)*sizeof(WCHAR));
                    cchCurDTC += pTokArray[indexTokOp+1].token.ibTokMac - pTokArray[indexTokOp+1].token.ibTokMin;

                    pDTC[cchCurDTC] = '"';
                    cchCurDTC++;

                    if (ichRTComment == -1)
                    {
                        ASSERT((int)(cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMac) >= 0);
                        memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                (BYTE *)(&pwOld[pTokArray[indexTokOp+1].token.ibTokMac]),
                                (cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMac)*sizeof(WCHAR));
                        cchCurDTC += (cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMac);
                    }
                    else
                    {
                        if (indexRTComment == -1)
                        {
                            ASSERT((int)(ichRTComment-pTokArray[indexTokOp+1].token.ibTokMac) >= 0);
                            memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                    (BYTE *)(&pwOld[pTokArray[indexTokOp+1].token.ibTokMac]),
                                    (ichRTComment-pTokArray[indexTokOp+1].token.ibTokMac)*sizeof(WCHAR));
                            cchCurDTC += (ichRTComment-pTokArray[indexTokOp+1].token.ibTokMac);

                            ASSERT((int)(cchObjEnd-(ichRTComment+cchRTComment)) >= 0);
                            memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                    (BYTE *)(&pwOld[ichRTComment+cchRTComment]),
                                    (cchObjEnd-(ichRTComment+cchRTComment))*sizeof(WCHAR));
                            cchCurDTC += (cchObjEnd-(ichRTComment+cchRTComment));
                        }
                        else
                        {
                            // format and copy from indexTokOp+2 till indexRTComment
                            for (i = indexTokOp+2; i < indexRTComment; i++)
                            {
                                memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                        (BYTE *)(&pwOld[pTokArray[i].token.ibTokMin]),
                                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                                cchCurDTC += pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin;
                                if (pTokArray[i].token.tok == TokTag_CLOSE && pTokArray[i].token.tokClass == tokTag)
                                {
                                    // Don't bother checking for existing EOLs...
                                    // add \r\n
                                    pDTC[cchCurDTC++] = '\r';
                                    pDTC[cchCurDTC++] = '\n';
                                    pDTC[cchCurDTC++] = '\t';
                                }

                            }

                            // copy from end of the comment till </object>
                            ASSERT((int)(cchObjEnd-(ichRTComment+cchRTComment)) >= 0);
                            memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                    (BYTE *)(&pwOld[ichRTComment+cchRTComment]),
                                    (cchObjEnd-(ichRTComment+cchRTComment))*sizeof(WCHAR));
                            cchCurDTC += (cchObjEnd-(ichRTComment+cchRTComment));
                        }
                    }
                }
                else
                {
                    if (ichRTComment == -1)
                    {
                        ASSERT((int)(cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMin) >= 0);
                        memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                (BYTE *)(&pwOld[pTokArray[indexTokOp+1].token.ibTokMin]),
                                (cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMin)*sizeof(WCHAR));
                        cchCurDTC += (cchObjEnd-pTokArray[indexTokOp+1].token.ibTokMin);
                    }
                    else
                    {
                        ASSERT((int)(ichRTComment-pTokArray[indexTokOp+1].token.ibTokMin) >= 0);
                        memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                (BYTE *)(&pwOld[pTokArray[indexTokOp+1].token.ibTokMin]),
                                (ichRTComment-pTokArray[indexTokOp+1].token.ibTokMin)*sizeof(WCHAR));
                        cchCurDTC += (ichRTComment-pTokArray[indexTokOp+1].token.ibTokMin);

                        ASSERT((int)(cchObjEnd-(ichRTComment+cchRTComment)) >= 0);
                        memcpy( (BYTE *)(&pDTC[cchCurDTC]),
                                (BYTE *)(&pwOld[ichRTComment+cchRTComment]),
                                (cchObjEnd-(ichRTComment+cchRTComment))*sizeof(WCHAR));
                        cchCurDTC += (cchObjEnd-(ichRTComment+cchRTComment));
                    }
                }
            }

            // 3. Insert MetaData2 tag from rgDTCtags[1]
            wcscpy(&pDTC[cchCurDTC], rgDTCTags[1]);
            cchCurDTC += wcslen(rgDTCTags[1]);
        }

        // 4. Add runtime text (copy code from old stuff)
        if ((hr = CreateStreamOnHGlobal(NULL, TRUE, &pStm)) != S_OK)
            goto LErrorRet;
    
        ASSERT(pActiveDesigner != NULL);
        if ((hr = pActiveDesigner->SaveRuntimeState(IID_IPersistTextStream, IID_IStream, pStm)) == S_OK)
        {
            if ((hr = GetHGlobalFromStream(pStm, &hg)) != S_OK)
                goto LErrorRet;

            STATSTG stat;
            if ((hr = pStm->Stat(&stat, STATFLAG_NONAME)) != S_OK)
                goto LErrorRet;
        
            int cch = stat.cbSize.LowPart / sizeof(WCHAR);

            // before we put stuff from hg into pDTC, 
            // lets make sure that its big enough
            cbNeed = (cchCurDTC+cch)*sizeof(WCHAR)+cbBufPadding;
            if (GlobalSize(hgDTC) < cbNeed)
            {
                hr = ReallocBuffer( &hgDTC, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT);
                if (hr == E_OUTOFMEMORY)
                    goto LErrorRet;
                ASSERT(hgDTC != NULL);
                pDTC = (WCHAR *)GlobalLock(hgDTC);
            }

            wcsncpy(&pDTC[cchCurDTC], (LPCWSTR) GlobalLock(hg), cch);
            cchCurDTC += cch;
            
            // HACK - BUG fix 9844
            // Some DTCs add a NULL at the end of their runtime text
            if (pDTC[cchCurDTC-1] == '\0')
                cchCurDTC--;

            GlobalUnlock(hg);
        }
        else if (hr == S_FALSE)
        {
            // copy the commented runtime text into pDTC & incremtn cchCurDTC
            if (ichRTComment != -1 && ichRT != -1) // we have the runtime text
            {
                ASSERT(cchRT >= 0);
                cbNeed = (cchCurDTC+cchRT)*sizeof(WCHAR)+cbBufPadding;
                if (GlobalSize(hgDTC) < cbNeed)
                {
                    hr = ReallocBuffer( &hgDTC, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT);
                    if (hr == E_OUTOFMEMORY)
                        goto LErrorRet;
                    ASSERT(hgDTC != NULL);
                    pDTC = (WCHAR *)GlobalLock(hgDTC);
                }
                wcsncpy(&pDTC[cchCurDTC], &pwOld[ichRT], cchRT);
                cchCurDTC += cchRT;
            }
        }

        if (!(dwFlags & dwFilterDTCsWithoutMetaTags))
        {
            // 5. Insert MetaData2 tag from rgDTCtags[2]
            wcscpy(&pDTC[cchCurDTC], rgDTCTags[2]);
            cchCurDTC += wcslen(rgDTCTags[2]);
        }
        
        // now insert/replace contents of pDTC into pwNew
        // we are insert/replacing (cchObjEnd-cchObjStart) wchars
        // by cchCurDTC wchars, so realloc pwNew first

        
        
        /* Reallocate pwNew IF NEEDED here use cache value for GlobalSize(*phgNew) and don't forget to update it too */
        cbNeed = (ichNewCur+(cchObjStart-ichBeginCopy)+(cchCurDTC))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LErrorRet;

        
        // cchObjStart/End are actually ich's
        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew

        // copy till begining of the <OBJECT>
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (cchObjStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += cchObjStart-ichBeginCopy;
        ichBeginCopy = cchObjEnd; // set it for next object

        CComPtr<IPersistPropertyBag> pPersistPropBag = NULL;

        hr = pDispControl->QueryInterface(IID_IPersistPropertyBag, (void **) &pPersistPropBag);
        if (hr == S_OK)
        {
            CLSID clsid;

            if (S_OK == pPersistPropBag->GetClassID(&clsid))
            {
                if (IsEqualCLSID(clsid, CLSID_PageTr))
                {
                    if (ptep->m_cchPTDTC != 0)
                    {
                        // Note that there is no need to realloc here since our buffer will already be bigger than we need it to be.
                        if (cchCurDTC != ptep->m_cchPTDTC)
                        {
                            memmove((BYTE *)(pwNew+ptep->m_ichPTDTC+cchCurDTC),
                                    (BYTE *)(pwNew+ptep->m_ichPTDTC+ptep->m_cchPTDTC),
                                    (ichNewCur-ptep->m_ichPTDTC-ptep->m_cchPTDTC)*sizeof(WCHAR));

                            ichNewCur += cchCurDTC-ptep->m_cchPTDTC;
                        }

                        memcpy( (BYTE *)(pwNew+ptep->m_ichPTDTC),
                                (BYTE *)(pDTC),
                                cchCurDTC*sizeof(WCHAR));
                
                        ptep->m_cchPTDTC = 0; 
                        ptep->m_ichBeginHeadTagIn = 0;  // reset, so that if we had multiple PTDTCs, 
                                                        //we won't try to stuff them inside HEAD
                        goto LSkipDTC;
                    }
                    else // this is the case where the PTDTC didn't exist before going to Trident
                    {
                        // we need to move this between <head> </head> tags if they exist
                        if (ptep->m_ichBeginHeadTagIn > 0) // we had HEAD tag in Source view
                        {
                            int ichInsertPTDTC = ptep->m_ichBeginHeadTagIn;

                            // insert the control immediately after the <HEAD> tag
                            //in pwNew look for '>' after ichInsertPTDTC
                            while (pwNew[ichInsertPTDTC] != '>')
                                ichInsertPTDTC++;
                            ichInsertPTDTC++; // skip '>'

                            ASSERT(ichInsertPTDTC < (INT)ichNewCur);
                            memmove((BYTE *)(pwNew+ichInsertPTDTC+cchCurDTC),
                                    (BYTE *)(pwNew+ichInsertPTDTC),
                                    (ichNewCur-ichInsertPTDTC)*sizeof(WCHAR));
                            ichNewCur += cchCurDTC;
                            memcpy( (BYTE *)(pwNew+ichInsertPTDTC),
                                    (BYTE *)(pDTC),
                                    cchCurDTC*sizeof(WCHAR));

                            ptep->m_ichBeginHeadTagIn = 0;
                            goto LSkipDTC;
                        }
                    }

                } // else if (IsEqualCLSID(clsid, CLSID_PageTr))
            } // if (S_OK == pPersistPropBag->GetClassID(&clsid))
        } // if (hr == S_OK)

        // copy the converted DTC
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(pDTC),
                cchCurDTC*sizeof(WCHAR));
        ichNewCur += cchCurDTC;

LSkipDTC:

        if (hgDTC != NULL)
            GlobalUnlockFreeNull(&hgDTC);

    } // if (hr == S_OK)
    else // this object was not a DTC
    {
        // we don't need to do the same for DTC's, but lets visit this in next release
        LPCWSTR rgComment[] =
        {
            L"ERRORPARAM",
            L"--ERRORPARAM ",
            L" ERRORPARAM--",
        };
        BOOL fFoundParam = FALSE;
        INT iParam = -1;
        INT ichObjStartEnd, iCommentStart, iCommentEnd;
        UINT iObjTagEnd;
        INT cComment, iFirstComment, iComment;

        iCommentStart = iCommentEnd = iComment = -1;
        // loop through indexObjStart till indexObjEnd to see if we have any <PARAM> tags
        for (i = indexObjStart; i < (INT)indexObjEnd; i++)
        {
            if (   pTokArray[i].token.tok == TokElem_PARAM
                && pTokArray[i].token.tokClass == tokElem)
            {
                fFoundParam = TRUE;
                iParam = i;
                break;
            }
        } // for ()
        if (fFoundParam)
            ASSERT(iParam != -1);

        // We need to copy till end of <OBJECT...> irrespective of if we find <PARAM>s or not.
        // copy till end of <OBJECT...> tag and set ichBeginCopy to be after the commented <PARAM> tags
        // calculate ichObjStartEnd
        iObjTagEnd = indexObjStart;
        while (iObjTagEnd < indexObjEnd)
        {
            if (   pTokArray[iObjTagEnd].token.tok == TokTag_CLOSE
                && pTokArray[iObjTagEnd].token.tokClass == tokTag)
                break;
            iObjTagEnd++;
        }
        if (iObjTagEnd >= indexObjEnd) // error case
            goto LErrorRet;
        ichObjStartEnd = pTokArray[iObjTagEnd].token.ibTokMac;
        
        cbNeed = (ichNewCur+ichObjStartEnd-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        ASSERT((INT)(ichObjStartEnd-ichBeginCopy) >= 0);
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichObjStartEnd-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichObjStartEnd-ichBeginCopy);
        ichBeginCopy = ichObjStartEnd;
        iArray = iObjTagEnd + 1;

        // generally, we don't expect Trident to move the comment from where it was put
        // but if it does, be prepared.
        // NOTE - Lets not worry about the following case for this release becasue prior assumption
        // Also, should we look for more comments if the first one wasn't the magic one?
        // Would Trident move it form where it originally inserted? 
        
        // ASSUMPTION - that Trident doesn't muck with the contents inside a comment block
        // if rgComment[0] matches and rgComment[1] does not, Trident may have mucked with the 
        // comment contents. This invalidates our original assumption.
        // NOTE - We can get away with ignoring this case for thie release
        i = iObjTagEnd;
        cComment = 0;
        iFirstComment = -1;
        while ((UINT)i < indexObjEnd)
        {
            if (   pTokArray[i].token.tok == TokTag_BANG
                && pTokArray[i].token.tokClass == tokTag)
            {
                cComment++;
                if (iFirstComment == -1)
                    iFirstComment = i;
            }
            i++;
        }
        if (cComment == 0) // error, didn't find the comment
            goto LErrorRet;

        // early return cases
        // 1. see if these are comments or not.They could be anything that start with '<!'
        // e.g. <!DOCTYPE
        i = iFirstComment;
        while (i < (INT)indexObjEnd)
        {
            if (   (i < (INT)ptep->m_cMaxToken)
                && (pwOld[pTokArray[i].token.ibTokMin] == '-')
                && (pwOld[pTokArray[i].token.ibTokMin+1] == '-')
                && (0 == _wcsnicmp(rgComment[0], &pwOld[pTokArray[i].token.ibTokMin+2], wcslen(rgComment[0])))
                )
            {
                ASSERT(i-1 >= 0);
                iCommentStart = i-1; // this is a comment we are interested in
            }
            else
                goto LNextComment;

            // The first part matched, look at the end of the comment
            if (   (pwOld[pTokArray[i].token.ibTokMac-1] == '-')
                && (pwOld[pTokArray[i].token.ibTokMac-2] == '-')
                && (0 == _wcsnicmp( rgComment[0], 
                                    &pwOld[pTokArray[i].token.ibTokMac-(wcslen(rgComment[0])+2)], 
                                    wcslen(rgComment[0])
                                    )
                                )
                )
            {
                iCommentEnd = i + 1;
                iComment = i;
                ASSERT(iCommentEnd < (INT)ptep->m_cMaxToken);
                break;
            }
            else // error case (our assumption was not valid). ignore and return with iArraySav+1
                goto LNextComment;
LNextComment:
            i++;
        } // while ()


        // HANDLE THIS CASE - WHAT IF WE DIDN'T FIND A SINGLE COMMENT????


        if (fFoundParam)
        {
            if (iCommentStart != -1 && iCommentEnd != -1)
            {
                cbNeed = (ichNewCur+(pTokArray[iCommentEnd].token.ibTokMac-pTokArray[iObjTagEnd].token.ibTokMin)+(iCommentStart-iObjTagEnd)*3/*for eol,tab*/+(pTokArray[iObjTagEnd].token.ibTokMac-ichBeginCopy))*sizeof(WCHAR)+cbBufPadding;
                if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                    goto LErrorRet;

                // we need to format the param tags because trident puts them on one line
                // copy till the first param tag
                memcpy( (BYTE *)(&pwNew[ichNewCur]),
                        (BYTE *)(&pwOld[ichBeginCopy]),
                        (pTokArray[iObjTagEnd].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
                ichNewCur += (pTokArray[iObjTagEnd].token.ibTokMac-ichBeginCopy);
                // From here, copy each param tag and insert an EOL after each. 
                // Stop at iCommentStart
                for (i = iObjTagEnd+1; i < iCommentStart; i++)
                {
                    // if its TokTag_START, insert EOL
                    if (   pTokArray[i].token.tok == TokTag_START
                        && pTokArray[i].token.tokClass == tokTag
                        )
                    {
                        pwNew[ichNewCur] = '\r';
                        ichNewCur++;
                        pwNew[ichNewCur] = '\n';
                        ichNewCur++;
                        pwNew[ichNewCur] = '\t'; // replace this with appropriate alignment
                        ichNewCur++;
                    }
                    // copy the tag
                    memcpy( (BYTE *)(&pwNew[ichNewCur]),
                            (BYTE *)(&pwOld[pTokArray[i].token.ibTokMin]),
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                } // for ()

                // from here, look for extra spaces/tabs/eols that trident has accumulated
                // at the end of the PARAM tags and remove them.
                for (i = iCommentEnd+1; i <= (int)indexObjEnd; i++)
                {
                    if (   (pTokArray[i].token.tokClass == tokIDENTIFIER && pTokArray[i].token.tok == 0)
                        || (   pTokArray[i].token.tokClass == tokOp 
                            && pTokArray[i].token.tok == 0 
                            && pwOld[pTokArray[i].token.ibTokMin] == 0x0a
                            && pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin == 1
                            )
                        )
                    {
                        int iChar;
                        BOOL fCopy = FALSE;

                        // see if all the characters in this token are spaces/tabs/eols
                        for (iChar = pTokArray[i].token.ibTokMin; iChar < (int)pTokArray[i].token.ibTokMac; iChar++)
                        {
                            if (   pwOld[iChar] != ' '
                                && pwOld[iChar] != '\r'
                                && pwOld[iChar] != '\n'
                                && pwOld[iChar] != '\t'
                                )
                            {
                                // we need to copy this token
                                fCopy = TRUE;
                                break;
                            }
                        } // for (iChar)
                        if (fCopy)
                        {
                            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                                    (BYTE *)(&pwOld[pTokArray[i].token.ibTokMin]),
                                    (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                            ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                        }
                    }
                    else
                    {
                        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                                (BYTE *)(&pwOld[pTokArray[i].token.ibTokMin]),
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                        ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                        if (pTokArray[i].token.tok == TokTag_CLOSE && pTokArray[i].token.tokClass == tokTag)
                        {
                            pwNew[ichNewCur++] = '\r';
                            pwNew[ichNewCur++] = '\n';
                        }
                    }
                } // for ()
                ichBeginCopy = pTokArray[indexObjEnd].token.ibTokMac;
                iArray = indexObjEnd + 1;
            }
        }
        else
        {
            if (iCommentStart != -1 && iCommentEnd != -1 && iComment != -1)
            {
                INT cchComment1, cchComment2;
                INT ichCommentStart, ichParamStart, cchCommentToken;

                // We didn't have any <PARAM> for this object. It means one of the following
                // (a)Trident deleted those or (b)it didn't have any before going to Trident
                // If Trident deleted those, we should have them in form of a comment.
                // If we didn't have those  before doing to Trident, we won't have that magic comment
                // BUT by the time we come here, we are sure that we have found the magic comment

                // ASSUME that trident won't move the comment from its original place
                // NOTE - In this release, we don't need to handle the case of Trident moving the comment location
                // which was originally placed just after <OBJECT ...>

                // remove the comment tokens surrounding the <PARAM>s.
                cchComment1 = wcslen(rgComment[1]);
                cchComment2 = wcslen(rgComment[2]);
                // remove cchComment1 chars from begining of pwOld[pTokArray[i+1].token.ibTokMin
                // remove cchComment2 chars from the end of pwOld[pTokArray[i+1].token.ibTokMac
                // and copy the rest into pwNew

                ichCommentStart = pTokArray[iCommentStart].token.ibTokMin;
                ichParamStart = pTokArray[iCommentStart+1].token.ibTokMin+cchComment1;
                ASSERT((INT)(ichCommentStart-ichBeginCopy) >= 0);
                cbNeed = (ichNewCur+ichCommentStart-ichBeginCopy+pTokArray[iComment].token.ibTokMac-pTokArray[iComment].token.ibTokMin)*sizeof(WCHAR)+cbBufPadding;
                if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                    goto LRet;
                // copy till begining of the comment
                memcpy( (BYTE *)(pwNew+ichNewCur),
                        (BYTE *)(pwOld+ichBeginCopy),
                        (ichCommentStart-ichBeginCopy)*sizeof(WCHAR));
                ichNewCur += ichCommentStart-ichBeginCopy;
                ichBeginCopy = pTokArray[iCommentEnd].token.ibTokMac;

                cchCommentToken = pTokArray[iComment].token.ibTokMac-pTokArray[iComment].token.ibTokMin;
                ASSERT((INT)(cchCommentToken-cchComment1-cchComment2) >= 0);
                memcpy( (BYTE *)(&pwNew[ichNewCur]),
                        (BYTE *)&(pwOld[ichParamStart]),
                        (cchCommentToken-cchComment1-cchComment2)*sizeof(WCHAR));
                ichNewCur += pTokArray[iComment].token.ibTokMac-pTokArray[iComment].token.ibTokMin-cchComment1-cchComment2;
                iArray = iCommentEnd + 1;
            }
        } // if (!fFoundParam)
    } // else of if (hr == S_OK)

LErrorRet:
    //free hgDTC if its not NULL
    if (hgDTC != NULL)
        GlobalUnlockFreeNull(&hgDTC);

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

//LRetOnly:   
    return;

} /* fnRestoreDTC() */

void
CTriEditParse::fnSaveDTC(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
          INT *pcDTC, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{
    // DTC case -
    // if we get STARTSPAN, search backwords (carefully) for tokTag_BANG in pTokArray
    // once we find that, remember the ibTokMin for DTC replacement
    // once we get a ENDSPAN tagID, wait for upcoming toktag_CLOSE which will end DTC
    // remember ibTokMac at that position. This is the DTC range.
    // In pTokArray, start at METADATA and look for matching OBJECT & /OBJECT tokIDs
    // Blt the OBJECT block over to ibTokMin and NULL remaining area in DEBUG build

    UINT indexDTCStart, indexDTCEnd, cchDTCStart, cchDTCEnd;
    UINT indexObjectStart, indexObjectEnd, cchObjectStart, cchObjectEnd;
    BOOL fFindFirstObj;
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    INT cDTC = *pcDTC;
    INT i;
    INT ichClsid = 0; // init
    LPOLESTR szClsid;
    UINT iStartSpan;
    LPWSTR pwNew = *ppwNew;
    LPCWSTR rgCommentRT[] =
    {
        L" <!--DTCRUNTIME ",
        L" DTCRUNTIME--> ",
        L"-->",
    };
    LPCWSTR szDesignerControl[] =
    {
        L"\"DesignerControl\"",
        L"DesignerControl",
    };
    BOOL fDesignerControlFound;
    UINT iArraySav = iArray;

    UINT ichObjectEndBegin, indexRTMac, indexRTStart;
    BOOL fFirstDash;
    UINT cbNeed;

    indexDTCStart = indexDTCEnd = cchDTCStart = cchDTCEnd = 0;
    indexObjectStart = indexObjectEnd = cchObjectStart = cchObjectEnd = 0;

    ASSERT(cDTC >= 0); // make sure that this was initilized
    if (cDTC == 0)
        goto LRetOnly;
    while (cDTC > 0)
    {
        // start at iArray of pTokArray and look for STARTSPAN
        //while (pTokArray[iArray].token.tok != ft.tokBegin2)
        //  iArray++;
        ASSERT(iArray < ptep->m_cMaxToken);
        
        if (pTokArray[iArray].token.tok != TokAttrib_STARTSPAN)
            goto LRet; // something is wrong

        iStartSpan = iArray;
        ASSERT(pTokArray[iArray].token.tok == TokAttrib_STARTSPAN);
        ASSERT(pTokArray[iArray].token.tokClass == tokAttr);
        i = iArray; // the position at which we found ft.tokBegin2
        fDesignerControlFound = FALSE;
        while (i >= 0)
        {
            // do we need to do anything else here?
            if (pTokArray[i].token.tok == ft.tokBegin)
            {
                ASSERT(pTokArray[i].token.tok == TokTag_BANG);
                ASSERT(pTokArray[i].token.tokClass == tokTag);
                break;
            }
            if (   (   pTokArray[i].token.tokClass == tokString
                    && 0 == _wcsnicmp(szDesignerControl[0], &pwOld[pTokArray[i].token.ibTokMin], wcslen(szDesignerControl[0]))
                    )
                || (   pTokArray[i].token.tokClass == tokValue
                    && 0 == _wcsnicmp(szDesignerControl[1], &pwOld[pTokArray[i].token.ibTokMin], wcslen(szDesignerControl[1]))
                    )
                )
            {
                fDesignerControlFound = TRUE;
            }

            i--;
        }
        if (i >= 0) // found TokTag_BANG token
        {
            cchDTCStart = pTokArray[i].token.ibTokMin;
            indexDTCStart = i;
        }
        else // error case 
        {
            // we found STARTSPAN, but didn't find <! of <!--METADATA
            // we can't process this DTC, so quit
            goto LRet;
        }
        if (!fDesignerControlFound)
        {
            // we didn't find DesignerControl for the DTC, which means this is not the DTC we care about
            // we can't process this DTC, so quit
            iArray = iArraySav + 1;
            goto LRet;
        }

        // now, look for ft.tokEnd2 i.e. TokAttrib_ENDSPAN
        if (   pTokArray[iStartSpan].iNextprev != -1 /* validate */
            && pTokArray[pTokArray[iStartSpan].iNextprev].token.tok == ft.tokEnd2)
        {
            ASSERT(pTokArray[pTokArray[iStartSpan].iNextprev].token.tokClass == tokAttr);
            i = iStartSpan;
            while (i < (int)ptep->m_cMaxToken && pTokArray[i].token.tok != TokElem_OBJECT)
                i++;
            if (i < (int)ptep->m_cMaxToken) // found the first <OBJECT> tag
                indexObjectStart = i;
            i = pTokArray[iStartSpan].iNextprev;
        }
        else // actually, we should have found ft.tokEnd2 in the if case, but if stack unwinding didn't happen correctly...
        {
            // on the way, look for 1st <OBJECT> tag
            fFindFirstObj = TRUE;
            i = iArray;
            while (pTokArray[i].token.tok != ft.tokEnd2)
            {
                if (fFindFirstObj && pTokArray[i].token.tok == TokElem_OBJECT)
                {
                    ASSERT(pTokArray[i].token.tokClass == tokElem);
                    indexObjectStart = i;
                    fFindFirstObj = FALSE;
                }
                i++;
                if (i >= (int)ptep->m_cMaxToken)
                    break;
            }
            if (i >= (int)ptep->m_cMaxToken)
            {
                // we didn't find ENDSPAN before hitting ptep->m_cMaxToken
                // we can't process this DTC, so quit
                goto LRet;
            }
        }
        ASSERT(pTokArray[i].token.tok == TokAttrib_ENDSPAN);
        ASSERT(pTokArray[i].token.tokClass == tokAttr);

        // from this i'th  position, look backwards to find '<!' of '<!--METADATA ...endspan...'
        indexRTMac = i;
        while (indexRTMac > indexObjectStart)
        {
            if (   pTokArray[indexRTMac].token.tok == TokTag_BANG
                && pTokArray[indexRTMac].token.tokClass == tokTag
                )
            {
                break;
            }
            indexRTMac--;
        }
        if (indexRTMac <= indexObjectStart) // error case
            goto LRet;
        
        // save this ith position to find last </OBJECT> tag
        indexObjectEnd = indexObjectStart;
        // from this ith poistion, look for ft.tokEnd
        while (i < (int)ptep->m_cMaxToken)
        {
            if (pTokArray[i].token.tok == ft.tokEnd)
            {
                ASSERT(pTokArray[i].token.tok == TokTag_CLOSE);
                ASSERT(pTokArray[i].token.tokClass == tokTag);
                break;
            }
            i++;
        }
        if (i < (int)ptep->m_cMaxToken) // found TokTag_CLOSE token
        {
            cchDTCEnd = pTokArray[i].token.ibTokMac;
            indexDTCEnd = i;
        }
        else
        {
            // we didn't find TokTag_CLOSE after ENDSPAN,
            // we can't process this DTC, so quit
            goto LRet;
        }
        // look forward from indexObjectEnd for the </OBJECT> tag
        while (indexObjectEnd < ptep->m_cMaxToken)
        {
            if (   pTokArray[indexObjectEnd].token.tok == TokElem_OBJECT
                && pTokArray[indexObjectEnd].token.tokClass == tokElem
                && pTokArray[indexObjectEnd-1].token.tok == TokTag_END /* </ */
                )
                break;
            indexObjectEnd++;
        }
        if (indexObjectEnd >= ptep->m_cMaxToken) // didn't find </OBJECT>, error case
        {
            goto LRet;
        }
        if (indexObjectEnd > indexObjectStart) // </OBJECT> found
        {
            // get ibTokMin of the previous < tag for indexObjectStart
            i = indexObjectStart;
            // generally, the previous tag should be the one we want, 
            // but this covers the boundary cases
            while (i > (int)indexDTCStart) 
            {
                if (pTokArray[i].token.tok == TokTag_START)
                {
                    ASSERT(pTokArray[i].token.tokClass == tokTag);
                    break;
                }
                i--;
            }
            //ASSERT(i > (int)indexDTCStart+1); // atleast
            cchObjectStart = pTokArray[i].token.ibTokMin;
            // get ibTokMac of the next > tag for indexObjectEnd
            i = indexObjectEnd;
            // generally, the next tag should be the one we want, 
            // but this covers the boundary cases
            while (i < (int)indexDTCEnd)
            {
                if (pTokArray[i].token.tok == TokTag_CLOSE)
                {
                    ASSERT(pTokArray[i].token.tokClass == tokTag);
                    break;
                }
                i++;
            }
            ASSERT(i < (int)indexDTCEnd -1); // atleast
            cchObjectEnd = pTokArray[i].token.ibTokMac; // do we need -1 here?
        }
        else
            goto LRet;

        // from indexObjectEnd look backwards to get tokTag_END
        indexRTStart = i+1;
        i = indexObjectEnd;
        while (i > (int)indexObjectStart) // we don't have to go this far
        {
            if (   pTokArray[i].token.tok == TokTag_END
                && pTokArray[i].token.tokClass == tokTag
                )
            {
                break;
            }
            i--;
        }
        if (i <= (int)indexObjectStart) // error case, do we care?
            goto LRet;
        ichObjectEndBegin = pTokArray[i].token.ibTokMin;

        iArray = indexDTCEnd; // set it for next DTC entry
        
        // now Replace the DTC

        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew
        // copy from ichBeginCopy to begining of DTC
        if ((int)(cchDTCStart-ichBeginCopy) >= 0)
        {
            cbNeed = (ichNewCur+cchDTCStart-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
            if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                goto LSkipCopy;
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[ichBeginCopy]),
                    (cchDTCStart-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (cchDTCStart-ichBeginCopy);
            ichBeginCopy = cchDTCEnd; // make it ready for next copy
        }

        i = indexObjectStart;

        while (i < (int)indexObjectEnd)
        {
            if (pTokArray[i].token.tok == TokAttrib_CLASSID)
            {
                ASSERT(pTokArray[i].token.tokClass == tokAttr);
                break;
            }
            i++;
        }

        if (i < (int)indexObjectEnd -1) // found TokAttrib_CLASSID
        {
            // make sure that the next one is tokOpEqual
            ASSERT(pTokArray[i+1].token.tokClass == tokOp);
            // make sure that the next one is the id and get that value
            //ASSERT(pTokArray[i].token.tok == );

            // Is there any other way to skip "clsid:" string that appears before the clsid?
            ichClsid = pTokArray[i+2].token.ibTokMin + strlen("clsid:");
            // This is a HACK to fix DaVinci's bug, where they can't handle non-quoted
            // classId
            if (pwOld[pTokArray[i+2].token.ibTokMin] == '"')
                ichClsid++;
        }

        if (ptep->m_fInHdrIn)
        {
            if (       (S_OK == StringFromCLSID(CLSID_PageTr, &szClsid))
                        && (0 == _wcsnicmp(szClsid+1/* for {*/, &pwOld[ichClsid], sizeof(CLSID)))
                        )
            {
                // copy the object part of the DTC into m_pPTDTC
                if (ptep->m_pPTDTC != NULL) // means that we have more than one PTDTC on the page
                    goto LMultPTDTC;

                ptep->m_hgPTDTC = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (cchObjectEnd-cchObjectStart)*sizeof(WCHAR));
                // if the allocation failed, just don't copy into ptep->m_hgPTDTC
                if (ptep->m_hgPTDTC != NULL)
                {
                    ptep->m_pPTDTC = (WORD *) GlobalLock(ptep->m_hgPTDTC);
                    ASSERT(ptep->m_pPTDTC != NULL);
                    memcpy( (BYTE *)(ptep->m_pPTDTC),
                            (BYTE *)(&pwOld[cchObjectStart]),
                            (cchObjectEnd-cchObjectStart)*sizeof(WCHAR));
                    ptep->m_cchPTDTCObj = cchObjectEnd-cchObjectStart;
                    ptep->m_ichPTDTC = cchDTCStart; // with respect to the saved header
                    ptep->m_cchPTDTC = cchDTCEnd - cchDTCStart;

                    ::CoTaskMemFree(szClsid);
                    goto LSkipCopy;
                }
            }
LMultPTDTC:
            ::CoTaskMemFree(szClsid);
        }

        cbNeed = (ichNewCur+(cchObjectEnd-cchObjectStart)+(pTokArray[indexRTMac].token.ibTokMin-cchObjectEnd)+wcslen(rgCommentRT[0])+wcslen(rgCommentRT[1]))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LSkipCopy;
        // STEP 1 - copy till the begining of </OBJECT>
        ASSERT((int)(ichObjectEndBegin-cchObjectStart) >= 0);
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[cchObjectStart]),
                (ichObjectEndBegin-cchObjectStart)*sizeof(WCHAR));
        ichNewCur += ichObjectEndBegin-cchObjectStart;

        // STEP 2 - Insert the runtime text as a comment
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(rgCommentRT[0]),
                wcslen(rgCommentRT[0])*sizeof(WCHAR));
        ichNewCur += wcslen(rgCommentRT[0]);

        // we need to loop thr indexRTStart & indexRTMac and copy token by token
        // and modify TokTag_BANG on the way
        fFirstDash = TRUE;
        while (indexRTStart < indexRTMac)
        {
            // (4/14/98)
            // VID-BUG 17453 Fotm Manager DTC puts in 0x0d (\r) as an end of line instead of
            // putting 0x0d 0xa (\r\n) as an end of line. 
            // In this case, the token thats generated is tokIdentifier => "0x0d - - >" 
            // instead of getting 3 separate tokens for the normal case as "0x0d 0x0a"
            // & "- -" & ">".
            // Two ways to fix this problem ...
            // 1. Handle this would be in our tokenizer that treats "0x0d" as an
            //    end of line as well. But at this time, its not a safe change to do.
            // 2. In the below if condition, add the fact that we may have "0x0d" followed
            //    by "-->" for end of metadata comment.
            if (   fFirstDash
                && (   (0 == _wcsnicmp(rgCommentRT[2], &pwOld[pTokArray[indexRTStart].token.ibTokMin], wcslen(rgCommentRT[2])))
                    || (   (0 == _wcsnicmp(rgCommentRT[2], &pwOld[pTokArray[indexRTStart].token.ibTokMin+1], wcslen(rgCommentRT[2])))
                        && (pwOld[pTokArray[indexRTStart].token.ibTokMin] == 0x0d)
                        )
                    )
                )
            {
                indexRTStart++;
                fFirstDash = FALSE;
                continue;
            }

            memcpy( (BYTE *)&pwNew[ichNewCur],
                    (BYTE *)&pwOld[pTokArray[indexRTStart].token.ibTokMin],
                    (pTokArray[indexRTStart].token.ibTokMac-pTokArray[indexRTStart].token.ibTokMin)*sizeof(WCHAR)
                    );
            ichNewCur += pTokArray[indexRTStart].token.ibTokMac-pTokArray[indexRTStart].token.ibTokMin;
            if (   pTokArray[indexRTStart].token.tok == TokTag_BANG 
                && pTokArray[indexRTStart].token.tokClass == tokTag
                )
            {
                pwNew[ichNewCur-2] = '?';
            }
            if (   pTokArray[indexRTStart].token.tok == TokTag_CLOSE 
                && pTokArray[indexRTStart].token.tokClass == tokTag
                && pwOld[pTokArray[indexRTStart-1].token.ibTokMac-1] == '-'
                && pwOld[pTokArray[indexRTStart-1].token.ibTokMac-2] == '-'
                )
            {
                pwNew[ichNewCur-1] = '?';
            }
            // following is a hack for the NavBar DTC
            if (   pTokArray[indexRTStart].token.tok == TokElem_METADATA
                && pTokArray[indexRTStart].token.tokClass == tokElem
                )
            {
                pwNew[ichNewCur-1] = '?';
            }
            indexRTStart++;
        } // while ()

        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(rgCommentRT[1]),
                wcslen(rgCommentRT[1])*sizeof(WCHAR));
        ichNewCur += wcslen(rgCommentRT[1]);

        // STEP 3 - copy the rest of the object, i.e. the </OBJECT> tag
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichObjectEndBegin]),
                (cchObjectEnd-ichObjectEndBegin)*sizeof(WCHAR));
        ichNewCur += cchObjectEnd-ichObjectEndBegin;

LSkipCopy:
        cDTC--;
    } // while (cDTC > 0)

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;
LRetOnly:
    return;
} /* fnSaveDTC() */

void
CTriEditParse::fnSaveHtmlTag(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{
    BOOL fFoundTag, fFoundHtmlBegin;
    INT i;
    UINT cchHtml, iHtmlBegin, iHtmlEnd;
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT cbNeed;

    // assert that iArray'th element in pTokArry is TokTag_HTML
    // Look for any non -1 tags before iArray
    // if we find any, it indicates that we have some stuff before <HTML> that trident doesn't like
    // in pwNew, move all ichNewCur bytes (copied so far) to make space for <HTML> at the begining
    // copy from pwOld <HTML location=> tag
    // adjust ichNewCur and ichBeginCopy

    // **** don't bother about maintaning info about <HTML> tag's location for Restore
    ASSERT(pTokArray[iArray].token.tok == TokElem_HTML);
    iHtmlBegin = i = iArray-1; // init
    fFoundTag = fFoundHtmlBegin = FALSE;
    while (i >= 0)
    {
        if (pTokArray[i].token.tokClass == tokElem || pTokArray[i].token.tokClass == tokSSS)
        {
            fFoundTag = TRUE;
            break;
        }
        if (!fFoundHtmlBegin && pTokArray[i].token.tok == ft.tokBegin) // look for < of <HTML>
        {
            fFoundHtmlBegin = TRUE;
            iHtmlBegin = i; // generally, this should be the right before TokElem_HTML
        }
        i--;
    }
    if (!fFoundHtmlBegin) // we didn't find < for <HTML>, so we are in deep trouble, lets quit here
    {
        goto LRet;
    }
    if (!fFoundTag) // we didn't find any tag before TokElem_HTML, so we don't need to do anything, quit
    {
        goto LRet;
    }

    // move <HTML> tag at the begining of pwNew
    i = iHtmlBegin; // iArray;
    ASSERT(pTokArray[i].token.tok == TokTag_START);
    ASSERT(pTokArray[i].token.tokClass == tokTag);
    
    // look for > of <HTML>
    while (i < (int)ptep->m_cMaxToken) // generally, this will be the very next tag, but this covers boundary cases
    {
        if (pTokArray[i].token.tok == ft.tokEnd)
            break;
        i++;
    }
    if (i >= (int)ptep->m_cMaxToken) // error case, didn't find > of <HTML>, so quit
    {
        iArray++; // so that we won't come back here for the same token
        goto LRet;
    }
    iHtmlEnd = i; // found > of <HTML>
    iArray = i; // set it after > of <HTML>

    cbNeed = (ichNewCur+pTokArray[iHtmlBegin].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;
    // copy till begining of the <HTML>
    memcpy( (BYTE *)(&pwNew[ichNewCur]),
            (BYTE *)(&pwOld[ichBeginCopy]),
            (pTokArray[iHtmlBegin].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += pTokArray[iHtmlBegin].token.ibTokMin-ichBeginCopy;
    ichBeginCopy = pTokArray[iHtmlEnd].token.ibTokMac; // set it for next thing

    // move all the stuff from pwNew+0 till pwNew+ichNewCur by cchHtml (make space for <HTML>)
    cchHtml = pTokArray[iHtmlEnd].token.ibTokMac-pTokArray[iHtmlBegin].token.ibTokMin;
    memmove((BYTE *)(&pwNew[cchHtml]),
            (BYTE *)pwNew,
            ichNewCur*sizeof(WCHAR));
    ichNewCur += cchHtml;

    // copy <HTML>
    memcpy( (BYTE *)pwNew,
            (BYTE *)(&pwOld[pTokArray[iHtmlBegin].token.ibTokMin]), 
            cchHtml*sizeof(WCHAR));

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

} /* fnSaveHtmlTag() */

void
CTriEditParse::fnRestoreHtmlTag(CTriEditParse* /*ptep*/, LPWSTR /*pwOld*/,
          LPWSTR* /*ppwNew*/, UINT* /*pcchNew*/, HGLOBAL* /*phgNew*/, 
          TOKSTRUCT* /*pTokArray*/, UINT* /*piArrayStart*/, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT* /*pichNewCur*/, UINT* /*pichBeginCopy*/,
          DWORD /*dwFlags*/)
{
    // **** 
    // because we didn't save any info about <HTML> tag's location for Restore, we just return
    return;

} /* fnRestoreHtmlTag() */

void
CTriEditParse::fnSaveNBSP(CTriEditParse* /*ptep*/, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    LPCWSTR szNBSP[] = {L"&NBSP"};
    LPCWSTR szNBSPlower[] = {L"&nbsp;"};
    INT ichNbspStart, ichNbspEnd;
    UINT cbNeed;

    // see if pwOld[pTokArray->token.ibtokMin] matches with "&nbsp", 
    // and convert it to lower case
    ASSERT(pTokArray[iArray].token.tokClass == tokEntity);
    if (0 == _wcsnicmp(szNBSP[0], &pwOld[pTokArray[iArray].token.ibTokMin], wcslen(szNBSP[0])))
    {
        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew
        // copy from ichBeginCopy to begining of &nbsp

        // check if we have enough memory - If not, realloc
        ichNbspStart = pTokArray[iArray].token.ibTokMin;
        ichNbspEnd = pTokArray[iArray].token.ibTokMac;
        cbNeed = (ichNewCur+ichNbspStart-ichBeginCopy+wcslen(szNBSPlower[0]))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LErrorRet;

        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichNbspStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichNbspStart-ichBeginCopy);
        ichBeginCopy = ichNbspEnd; // make it ready for next copy
        
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(szNBSPlower[0]),
                (wcslen(szNBSPlower[0]))*sizeof(WCHAR));
        ichNewCur += wcslen(szNBSPlower[0]);
    }
LErrorRet:
    iArray++; // so that we won't look at the same token again

//LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

} /* fnSaveNBSP() */

void
CTriEditParse::fnRestoreNBSP(CTriEditParse* /*ptep*/, LPWSTR /*pwOld*/,
          LPWSTR* /*ppwNew*/, UINT* /*pcchNew*/, HGLOBAL* /*phgNew*/, 
          TOKSTRUCT* /*pTokArray*/, UINT* /*piArrayStart*/, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT* /*pichNewCur*/, UINT* /*pichBeginCopy*/,
          DWORD /*dwFlags*/)
{
    return;
} /* fnRestoreNBSP() */


BOOL
FIsSpecialTag(TOKSTRUCT *pTokArray, int iTag, WCHAR* /*pwOld*/)
{
    BOOL fRet = FALSE;

    if (   (   pTokArray[iTag].token.tokClass == tokSpace
			|| pTokArray[iTag].token.tokClass == tokComment)
        && pTokArray[iTag].token.tok == 0
        && iTag > 0
        && (   pTokArray[iTag-1].token.tok == TokTag_START 
            || pTokArray[iTag-1].token.tok == TokTag_PI
			|| (   pTokArray[iTag-1].token.tok == TokTag_BANG
				&& pTokArray[iTag+1].token.tok == TokTag_CLOSE
				&& pTokArray[iTag+1].token.tokClass == tokTag
				)
			)
        && pTokArray[iTag-1].token.tokClass == tokTag
        )
    {
        fRet = TRUE;
#ifdef WFC_FIX
        int cch = pTokArray[iTag].token.ibTokMac-pTokArray[iTag].token.ibTokMin;
        WCHAR *pStr = new WCHAR[cch+1];
        WCHAR *pFound = NULL;

        // see if this is xml tag
        // for now we will check tags that have a ':' in them.
        // NOTE - This will get changed when parser change to recognise xml tags is made
        if (pStr != NULL)
        {
            memcpy( (BYTE *)pStr, 
                    (BYTE *)&pwOld[pTokArray[iTag].token.ibTokMin],
                    cch*sizeof(WCHAR));
            pStr[cch] = '\0';
            pFound = wcschr(pStr, ':');
            if (pFound)
                fRet = TRUE;

            delete pStr;
        }
#endif //WFC_FIX
    }
    return(fRet);
}

void
GetTagRange(TOKSTRUCT *pTokArray, int iArrayLast, int *piTag, int *pichTokTagClose, BOOL fMatch)
{
    int index = *piTag;
    int iTokTagClose = -1;

    if (fMatch) // we should look fot pTokArray[iTag].iNextprev
    {
        if (pTokArray[*piTag].iNextprev == -1)
            goto LRet;
        index = pTokArray[*piTag].iNextprev; // that way, we will look for '>' after matching end
    }
    // look for TokTag_CLOSE, from iTag onwards
    while (index < iArrayLast)
    {
        if (   pTokArray[index].token.tokClass == tokTag
            && pTokArray[index].token.tok == TokTag_CLOSE)
        {
            iTokTagClose = index;
            break;
        }
        index++;
    }
    if (iTokTagClose != -1) // we found it
    {
        *pichTokTagClose = pTokArray[iTokTagClose].token.ibTokMac;
        *piTag = iTokTagClose + 1;
    }
LRet:
    return;
} /* GetTagRange() */


void CTriEditParse::fnSaveHdr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT cchBeforeBody = 0;
    UINT i, iFound;
    WCHAR *pHdr;
    UINT cbNeed;

    if (ptep->m_hgDocRestore == NULL)
        goto LRetOnly;

    // lock
    pHdr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore);
    ASSERT(pHdr != NULL);

    // look forward to make sure that we don't have multiple <BODY> tags
    // this may be a result of a typo in user's document or trident inserting it
    i = iArray+1;
    iFound = iArray;
    while (i < ptep->m_cMaxToken)
    {
        if (   (pTokArray[i].token.tok == TokElem_BODY)
            && (pTokArray[i].token.tokClass == tokElem)
            && (pTokArray[i-1].token.tok == TokTag_START)
            && (pTokArray[i-1].token.tokClass == tokTag)
            )
        {
            iFound = i;
            break;
        }
        i++;
    }
    if (iFound > iArray) // this means that we found the last <BODY> tag Trident inserted
        iArray = iFound;

    ASSERT(pTokArray[iArray].token.tok == TokElem_BODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    // what if we DON'T have a <BODY> tag at all. We would have found </BODY> here.
    // If thats the case, we just don't save anything
    ASSERT(iArray-1 >= 0);
    if (pTokArray[iArray-1].token.tok != TokTag_START)
        cchBeforeBody = 0;
    else
        cchBeforeBody = pTokArray[iArray].token.ibTokMin;

    // realloc if needed
    if (cchBeforeBody*sizeof(WCHAR)+sizeof(int) > GlobalSize(ptep->m_hgDocRestore))
    {
        GlobalUnlock(ptep->m_hgDocRestore);
        ptep->m_hgDocRestore = GlobalReAlloc(ptep->m_hgDocRestore, cchBeforeBody*sizeof(WCHAR)+sizeof(int), GMEM_MOVEABLE|GMEM_ZEROINIT);
        // if this alloc failed, we may still want to continue
        if (ptep->m_hgDocRestore == NULL)
            goto LRet;
        else
        {
            pHdr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore); // do we need to unlock this first?
            ASSERT(pHdr != NULL);
        }
    }

    // copy from pwOld
    memcpy( (BYTE *)pHdr,
            (BYTE *)&cchBeforeBody,
            sizeof(INT));
    memcpy( (BYTE *)(pHdr)+sizeof(INT),
            (BYTE *)pwOld,
            cchBeforeBody*sizeof(WCHAR));

    // reconstruct the pre_BODY part of the document
    // NOTE  - for next time around ...
    // If we get the title & body tags from pwNew instead of pwOld, we won't
    // loose the DESIGNTIMESPs for those 2 tags
    if (cchBeforeBody > 0)
    {
        int iTag = 0;
        int ichTokTagClose = -1;
        BOOL fMatch = FALSE;
        LPCWSTR rgSpaceTags[] =
        {
            L" DESIGNTIMESP=",
            L" designtimesp=",
        };
        WCHAR szIndex[cchspBlockMax]; // will we have more than 20 digit numbers as number of DESIGNTIMESPx?

        int index = iArray;
        int ichBodyTokenStart, ichBodyTokenEnd;
        LPCWSTR rgPreBody[] = {L"<BODY",};

        memset((BYTE *)pwNew, 0, ichNewCur*sizeof(WCHAR));
        // if we have a unicode stream, we should preserve 0xff,0xfe that occurs at the
        // beginning of the file
        ichNewCur = 0;
        if (ptep->m_fUnicodeFile)
        {
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, sizeof(WCHAR));
            ichNewCur = 1;
        }

        // loop through all tags starting from index of '<' of <html> till iArray
        // if the tag we see is one of the following, then copy that tag into pwNew
        // ------------------------------------------------------------------------
        // <HTML>, <HEAD>..</HEAD>, <TITLE>..</TITLE>, <STYLE>..</STYLE>, 
        // <LINK>, <BASE>, <BASEFONT>
        // ------------------------------------------------------------------------
        iTag = 0;
        ichTokTagClose = -1;
        while (iTag < (int)iArray)
        {
            if (   pTokArray[iTag].token.tokClass == tokAttr
                && pTokArray[iTag].token.tok == TokAttrib_STARTSPAN)
            {
                GetTagRange(pTokArray, iArray, &iTag, &ichTokTagClose, TRUE);
            }
            else if (   (   (pTokArray[iTag].token.tokClass == tokElem)
                    && (   pTokArray[iTag].token.tok == TokElem_HTML
                        || pTokArray[iTag].token.tok == TokElem_HEAD
                        || pTokArray[iTag].token.tok == TokElem_META
                        || pTokArray[iTag].token.tok == TokElem_LINK
                        || pTokArray[iTag].token.tok == TokElem_BASE
                        || pTokArray[iTag].token.tok == TokElem_BASEFONT
                        || pTokArray[iTag].token.tok == TokElem_TITLE
                        || pTokArray[iTag].token.tok == TokElem_STYLE
                        || pTokArray[iTag].token.tok == TokElem_OBJECT
                        )
                    )
                || (FIsSpecialTag(pTokArray, iTag, pwOld))
                )
            {
                int iTagSav = iTag;

                fMatch = FALSE;
                ichTokTagClose = -1;
                if (   pTokArray[iTag].token.tok == TokElem_TITLE
                    || pTokArray[iTag].token.tok == TokElem_STYLE
                    || pTokArray[iTag].token.tok == TokElem_OBJECT
                    )
                    fMatch = TRUE;
                GetTagRange(pTokArray, iArray, &iTag, &ichTokTagClose, fMatch);
                if (ichTokTagClose != -1)
                {
                    // copy the stuff into pwNew
                    pwNew[ichNewCur++] = '<';
                    if (   pTokArray[iTagSav-1].token.tok == TokTag_END
                        && pTokArray[iTagSav-1].token.tokClass == tokTag)
                    {
                        pwNew[ichNewCur++] = '/';
                    }
					else if (	   pTokArray[iTagSav-1].token.tok == TokTag_PI
								&& pTokArray[iTagSav-1].token.tokClass == tokTag)
					{
						pwNew[ichNewCur++] = '?';
					}
					else if (	   pTokArray[iTagSav-1].token.tok == TokTag_BANG
								&& pTokArray[iTagSav-1].token.tokClass == tokTag)
					{
						pwNew[ichNewCur++] = '!';
					}
                    memcpy( (BYTE *)&pwNew[ichNewCur], 
                            (BYTE *)&pwOld[pTokArray[iTagSav].token.ibTokMin],
                            (ichTokTagClose-pTokArray[iTagSav].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += ichTokTagClose-pTokArray[iTagSav].token.ibTokMin;
                    // do we want to add \r\n after each tag we copy?
                }
                else
                    goto LNext;
            }
            else
            {
LNext:
                iTag++;
            }
        } // while (iTag < (int)iArray)


        // we know that iArray is currently pointing to tokElem_BODY
        // go backwards and look for '<', so that we can copy from that point
        ASSERT(pTokArray[iArray].token.tok == TokElem_BODY);
        ASSERT(pTokArray[iArray].token.tokClass == tokElem);
        index = iArray;
        while (index >= 0)
        {
            if (   pTokArray[index].token.tok == TokTag_START
                && pTokArray[index].token.tokClass == tokTag)
            {
                break;
            }
            index--;
        }
        if (index < 0) // error case, we didn't find '<' before BODY
            goto LSkipBody;
        ichBodyTokenStart = pTokArray[index].token.ibTokMin;

        // now go forward till we get the '>' of <BODY>, we don't have to go this far, 
        // but this covers boundary cases
        index = iArray;
        while (index < (int)ptep->m_cMaxToken)
        {
            if (   pTokArray[index].token.tok == TokTag_CLOSE
                && pTokArray[index].token.tokClass == tokTag)
            {
                break;
            }
            index++;
        }
        if (index > (int)ptep->m_cMaxToken) // error case, we didn't find '>' before BODY
            goto LSkipBody;
        ichBodyTokenEnd = pTokArray[index-1].token.ibTokMac; // BUG 15391 - don't copy TokTag_CLOSE here, it gets added later
    
        // blt part of the <BODY> tag into pwNew. (BUG 15391 - excluding the ending >)
        ASSERT(ichBodyTokenEnd-ichBodyTokenStart >= 0);
        memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)&pwOld[ichBodyTokenStart], (ichBodyTokenEnd-ichBodyTokenStart)*sizeof(WCHAR));
        ichNewCur += (ichBodyTokenEnd-ichBodyTokenStart); 

        // only if spacing flag is set
        if (dwFlags & dwPreserveSourceCode)
        {
            // BUG 15391 - insert DESIGNTIMESP with (ptep->m_ispInfoBlock+ptep->m_ispInfoBase-1) & add '>' at the end
            ASSERT(wcslen(rgSpaceTags[1]) == wcslen(rgSpaceTags[0]));
            if (iswupper(pwOld[pTokArray[iArray].token.ibTokMin]) != 0) // upper case  - BUG 15389
            {
                memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgSpaceTags[0], wcslen(rgSpaceTags[0])*sizeof(WCHAR));
                ichNewCur += wcslen(rgSpaceTags[0]);
            }
            else
            {
                memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgSpaceTags[1], wcslen(rgSpaceTags[1])*sizeof(WCHAR));
                ichNewCur += wcslen(rgSpaceTags[1]);
            }
            (WCHAR)_itow(ptep->m_ispInfoBlock+ptep->m_ispInfoBase-1, szIndex, 10);
            ASSERT(wcslen(szIndex) < sizeof(szIndex));
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(szIndex),
                    wcslen(szIndex)*sizeof(WCHAR));
            ichNewCur += wcslen(szIndex);
        }
        goto LBodyCopyDone;

LSkipBody:
        // if we skipped copying <BODY> tag, we must put in a dummy <BODY> at ichNewCur
        memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgPreBody[0], wcslen(rgPreBody[0])*sizeof(WCHAR));
        ichNewCur = wcslen(rgPreBody[0]);

LBodyCopyDone:
        pwNew[ichNewCur++] = '>'; //ending '>' that we skipped copying before
        // set ichBeginCopy and iArray appropriately
        iArray = index+1;
        ichBeginCopy = pTokArray[iArray].token.ibTokMin;
    }

    // Copy everything upto and including <BODY>

//LSkipCopy:

    if (ptep->m_pPTDTC != NULL) // we had saved PageTransitionDTC in a temporary
    {
        ASSERT(ptep->m_cchPTDTCObj >= 0);
        cbNeed = (ichNewCur+ptep->m_cchPTDTCObj)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        memcpy( (BYTE *)&pwNew[ichNewCur],
                (BYTE *)ptep->m_pPTDTC,
                ptep->m_cchPTDTCObj*sizeof(WCHAR));
        ichNewCur += ptep->m_cchPTDTCObj;
        GlobalUnlockFreeNull(&(ptep->m_hgPTDTC));
    }

    ptep->m_fInHdrIn = FALSE;

LRet:
    // unlock
    GlobalUnlock(ptep->m_hgDocRestore);

    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

LRetOnly:
    return;

} /* fnSaveHdr() */

void 
CTriEditParse::fnRestoreHdr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT cchBeforeBody = 0;
    WCHAR *pHdr;
    INT ichBodyStart, ichBodyEnd;
    UINT i, iFound;
    UINT cbNeed;

    if (ptep->m_hgDocRestore == NULL)
        goto LRetOnly;

    // lock, copy, unlock
    pHdr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore);
    ASSERT(pHdr != NULL);

    ASSERT(pTokArray[iArray].token.tok == TokElem_BODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    
    // HACK to fix a TRIDENT misbehaviour
    // If we had any text before <BODY> tag going into Trident, it will add 2nd <BODY>
    // tag before this text comming out of Trident without looking forward and 
    // recognizing that a <BODY> tag already exists. Ideally, Trident should move teh
    // <BODY> tag at appropriate place rather than inserting a 2nd one.
    // Lets assume that Trident will insert only one extra <BODY> tag.
    i = iArray + 1; // we know iArray is the 1st <BODY> tag
    iFound = iArray;
    while (i < ptep->m_cMaxToken)
    {
        if (   (pTokArray[i].token.tok == ft.tokBegin2) /*TokElem_BODY*/
            && (pTokArray[i-1].token.tok == TokTag_START)
            )
        {
            iFound = i;
            break;
        }
        i++;
    }
    if (iFound > iArray) // this means that we found the last <BODY> tag Trident inserted
        iArray = iFound;

    memcpy((BYTE *)&cchBeforeBody, (BYTE *)pHdr, sizeof(INT));

    // realloc if needed
    ichBodyStart = pTokArray[iArray].token.ibTokMin;
    ichBodyEnd = pTokArray[iArray].token.ibTokMac;
    cbNeed = (ichNewCur+cchBeforeBody+ichBodyEnd-ichBodyStart)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LErrorRet;

    if (cchBeforeBody > 0)
    {
        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew
        // copy from ichBeginCopy to begining of &nbsp
        memcpy( (BYTE *)(pwNew),
                (BYTE *)(pHdr)+sizeof(INT),
                cchBeforeBody*sizeof(WCHAR));
        
        // fill 0s from pwNew+cchBeforeBody till pwNew+ichNewCur-1 (inclusive)
        if ((int)ichNewCur-cchBeforeBody > 0)
            memset((BYTE *)(pwNew+cchBeforeBody), 0, (ichNewCur-cchBeforeBody)*sizeof(WCHAR));

        ichNewCur = cchBeforeBody; // note that we are initializing ichNewCur here ***
        ichBeginCopy = ichBodyEnd; // make it ready for next copy
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBodyStart]),
                (ichBodyEnd-ichBodyStart)*sizeof(WCHAR));
        ichNewCur += (ichBodyEnd-ichBodyStart);  
    }
    else // if we didn't save anything, it means that we had no pre-BODY stuff in the doc (bug 15393)
    {
        if (ptep->m_fUnicodeFile && ichNewCur == 0)
        {
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, sizeof(WCHAR));
            ichNewCur = ichBeginCopy = 1;
        }
        // actually, we should get the '>' of <body> tag instead of using iArray+1
        if (dwFlags & dwFilterSourceCode)
            ichBeginCopy = pTokArray[iArray+1].token.ibTokMac; // '>' of <BODY> tag
        else
        {
#ifdef NEEDED // VID6 - bug 22781 (This is going to generate some debate, so #ifdef instead of removing.
            LPCWSTR rgPreBody[] =
            {
                L"<HTML>\r\n<HEAD><TITLE></TITLE></HEAD>\r\n",
            };
            ASSERT(ichNewCur >= 0); // make sure its not invalid
            memcpy( (BYTE *)&pwNew[ichNewCur], (BYTE *)rgPreBody[0], wcslen(rgPreBody[0])*sizeof(WCHAR));
            ichNewCur += wcslen(rgPreBody[0]);
#endif //NEEDED
            // Note that we had not saved any thing before going to design view because there was
            // no <BODY> tag. we should now copy from current pwOld[ichBeginCopy] till 
            // the new pwOld[ichBeginCopy] into pwNew[ichNewCur] and then set ichBeginCopy.
            if (pTokArray[iArray-1].token.ibTokMin > ichBeginCopy)
            {
                memcpy( (BYTE *)&pwNew[ichNewCur], 
                        (BYTE *)&pwOld[ichBeginCopy], 
                        (pTokArray[iArray-1].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
                ichNewCur += pTokArray[iArray-1].token.ibTokMin-ichBeginCopy;
            }
            ichBeginCopy = pTokArray[iArray-1].token.ibTokMin; // '<' of <BODY> tag
        }
    }

LErrorRet:
    // unlock
    GlobalUnlock(ptep->m_hgDocRestore);

    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;
LRetOnly:
    return;

} /* fnRestoreHdr() */


void CTriEditParse::fnSaveFtr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD /*dwFlags*/)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT cchAfterBody = 0;
    INT cchBeforeBody = 0;
    INT cchPreEndBody = 0;
    WCHAR *pFtr;
    INT ichStart, ichEnd;
    UINT iArraySav = iArray;
    UINT cbNeed;

    if (ptep->m_hgDocRestore == NULL)
        goto LRetOnly;

    // lock
    pFtr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore);
    ASSERT(pFtr != NULL);
    ichStart = pTokArray[iArray-1].token.ibTokMin; // init
    ASSERT(pTokArray[iArray].token.tok == TokElem_BODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    ASSERT(pTokArray[iArray-1].token.tok == TokTag_END);
    // what if we DON'T have a </BODY> tag at all. Lets handle the error case here
    // If thats the case, we just don't save anything
    ASSERT(iArray-1 >= 0);
    if (pTokArray[iArray-1].token.tok != TokTag_END)
    {
        cchAfterBody = 0;
        cchPreEndBody = 0;
    }
    else
    {
        // following was added for Bug fix for 7542
        cchAfterBody = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac-pTokArray[iArray].token.ibTokMac;
        
        // now calculate the space required to save stuff from before </BODY> 
        // till the previous meaningful token
        ichStart = ichEnd = pTokArray[iArray-1].token.ibTokMin;
        ichStart--; // now ichStart is pointing to a character before </BODY>
        while (    (ichStart >= 0)
                && (   pwOld[ichStart] == ' '
                    || pwOld[ichStart] == '\r'
                    || pwOld[ichStart] == '\n'
                    || pwOld[ichStart] == '\t'
                    )
                )
        {
            ichStart--;
        }
        ichStart++; // the current char is not one of the above, so increment
        if (ichStart == ichEnd) // we didn't have anyspace, eol, tab between </BODY> & previous token
        {
            cchPreEndBody = 0;
        }
        else
        {
            ASSERT(ichEnd - ichStart > 0);
            cchPreEndBody = ichEnd - ichStart;
        }
    }

    // get cchBeforeBody if pre-BODY part was saved, and adjust pFtr for saving
    memcpy((BYTE *)&cchBeforeBody, (BYTE *)pFtr, sizeof(INT));
    pFtr += cchBeforeBody + sizeof(INT)/sizeof(WCHAR);

    // realloc if needed
    if ((cchPreEndBody+cchAfterBody+cchBeforeBody)*sizeof(WCHAR)+3*sizeof(int) > GlobalSize(ptep->m_hgDocRestore))
    {
        GlobalUnlock(ptep->m_hgDocRestore);
        ptep->m_hgDocRestore = GlobalReAlloc(ptep->m_hgDocRestore, (cchPreEndBody+cchAfterBody+cchBeforeBody)*sizeof(WCHAR)+3*sizeof(int), GMEM_MOVEABLE|GMEM_ZEROINIT);
        // if this alloc failed, we may still want to continue
        if (ptep->m_hgDocRestore == NULL)
            goto LRet;
        else
        {
            pFtr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore); // do we need to unlock this first?
            ASSERT(pFtr != NULL);
            // remember to set pFtr to be after cchBeforeBody
            pFtr += cchBeforeBody + sizeof(INT)/sizeof(WCHAR);
        }
    }

    // copy from pwOld
    memcpy( (BYTE *)pFtr,
            (BYTE *)&cchAfterBody,
            sizeof(INT));
    memcpy( (BYTE *)(pFtr)+sizeof(INT),
            (BYTE *)(pwOld+pTokArray[iArray].token.ibTokMac),
            cchAfterBody*sizeof(WCHAR));
    pFtr += cchAfterBody + sizeof(INT)/sizeof(WCHAR);

    memcpy( (BYTE *)pFtr,
            (BYTE *)&cchPreEndBody,
            sizeof(INT));
    memcpy( (BYTE *)(pFtr)+sizeof(INT),
            (BYTE *)&(pwOld[ichStart]),
            cchPreEndBody*sizeof(WCHAR));

    // the very next token from TokElem_BODY will be TokTag_CLOSE in most cases, but just in case...
    while (iArray < ptep->m_cMaxToken)
    {
        if (pTokArray[iArray].token.tok == TokTag_CLOSE && pTokArray[iArray].token.tokClass == tokTag)
            break;
        iArray++;
    }
    if (iArray >= ptep->m_cMaxToken)
    {
        iArray = iArraySav+1; // atleast copy till that point
        goto LRet;
    }

    // copy till '>' of </BODY> from pwOld into pwNew
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);

    cbNeed = (ichNewCur+pTokArray[iArray].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;

    memcpy( (BYTE *)&(pwNew[ichNewCur]),
            (BYTE *)&(pwOld[ichBeginCopy]),
            (pTokArray[iArray].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += pTokArray[iArray].token.ibTokMac-ichBeginCopy;
    ichBeginCopy = pTokArray[iArray].token.ibTokMac;

    iArray = ptep->m_cMaxToken - 1;
    ichBeginCopy = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac; // we don't want to copy anything after this

LRet:
    // unlock
    GlobalUnlock(ptep->m_hgDocRestore);

    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

LRetOnly:
    return;

} /* fnSaveFtr() */

void CTriEditParse::fnRestoreFtr(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT cchAfterBody = 0;
    INT cchBeforeBody = 0;
    WCHAR *pFtr;
    INT ichBodyEnd;
    UINT i, iFound;
    INT ichInsEOL = -1; // initilize
    UINT cbNeed;

    if (ptep->m_hgDocRestore == NULL)
        goto LRetOnly;

    // lock, copy, unlock
    pFtr = (WCHAR *)GlobalLock(ptep->m_hgDocRestore);
    ASSERT(pFtr != NULL);

    ASSERT(pTokArray[iArray].token.tok == TokElem_BODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    
    // HACK to fix a TRIDENT misbehaviour
    // If we had any text before <BODY> tag going into Trident, it will add 2nd <BODY>
    // tag before this text comming out of Trident without looking forward and 
    // recognizing that a <BODY> tag already exists. Ideally, Trident should move teh
    // <BODY> tag at appropriate place rather than inserting a 2nd one.
    // Lets assume that Trident will insert only one extra <\BODY> tag.
    i = iArray + 1; // we know iArray is the 1st <\BODY> tag
    iFound = iArray;
    while (i < ptep->m_cMaxToken)
    {
        if (   (pTokArray[i].token.tok == ft.tokBegin2) /*TokElem_BODY*/
            && (pTokArray[i-1].token.tok == TokTag_END)
            )
        {
            iFound = i;
            break;
        }
        i++;
    }
    if (iFound > iArray) // this means that we found the last <BODY> tag Trident inserted
        iArray = iFound;

    memcpy((BYTE *)&cchBeforeBody, (BYTE *)pFtr, sizeof(INT));
    pFtr += cchBeforeBody + sizeof(INT)/sizeof(WCHAR);
    memcpy((BYTE *)&cchAfterBody, (BYTE *)pFtr, sizeof(INT));
    pFtr += sizeof(INT)/sizeof(WCHAR);
    ichBodyEnd = pTokArray[iArray].token.ibTokMac;
    // if (cchAfterBody == 0) // get the size of our own header

    // realloc if needed
    cbNeed = (ichNewCur+cchAfterBody+(ichBodyEnd-ichBeginCopy)+2/* for EOL*/)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LErrorRet;

    if (cchAfterBody > 0)
    {
        LPCWSTR rgSpaceTags[] = {L"DESIGNTIMESP"};
        int cchTag, index, indexDSP;

        // ichBeginCopy is a position in pwOld and
        // ichNewCur is a position in pwNew
        // copy from ichBeginCopy to end of HTML document
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichBodyEnd-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichBodyEnd-ichBeginCopy);
        ichBeginCopy = ichBodyEnd;

        // now that we have copied 'BODY' of </BODY> tag, lets make sure its of correct case (bug 18248)
        indexDSP = -1;
        index = pTokArray[iArray].iNextprev;
        cchTag = wcslen(rgSpaceTags[0]);
        if (index != -1 && index < (int)iArray) // we have matching <BODY> tag prior to this one
        {
            // get the designtimesp attribute
            while (index < (int)iArray) // we will never come this far, but thats the only known position at this point
            {
                if (pTokArray[index].token.tok == TokTag_CLOSE)
                    break;
                if (   (pTokArray[index].token.tok == 0)
                    && (pTokArray[index].token.tokClass == tokSpace)
                    && (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[index].token.ibTokMin], cchTag))
                    )
                {
                    indexDSP = index;
                    break;
                }
                index++;
            } // while
            if (indexDSP != -1) // we found DESIGNTIMESP attribute
            {
                // look for the case of designtimesp
                if (iswupper(pwOld[pTokArray[indexDSP].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
                    _wcsupr(&pwNew[ichNewCur-4]); // length of BODY tag name
                else
                    _wcslwr(&pwNew[ichNewCur-4]); // length of BODY tag name
            }
        }

        // we know that the following condition will be met most of the times, but just to cover
        // incomplete HTML cases...
        if (   (pTokArray[iArray].token.tok == ft.tokBegin2) /*TokElem_BODY*/
            && (pTokArray[iArray-1].token.tok == TokTag_END)
            )
        {
            ichInsEOL = ichNewCur - (pTokArray[iArray].token.ibTokMac - pTokArray[iArray-1].token.ibTokMin);
        }

        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)pFtr,
                (cchAfterBody)*sizeof(WCHAR));
        ichNewCur += (cchAfterBody);

        // we had saved spacing info before </BODY>
        if (ichInsEOL != -1)
        {
            INT cchPreEndBody = 0;

            pFtr += cchAfterBody;
            cchPreEndBody = *(int *)pFtr;
            if (cchPreEndBody > 0)
            {
                INT ichT = ichInsEOL-1;
                WCHAR *pw = NULL;
                INT cchSubStr = 0;
                WCHAR *pwStr = NULL;
                WCHAR *pwSubStr = NULL;

                pFtr += sizeof(INT)/sizeof(WCHAR); // pFtr now points to Pre </BODY> stuff
                // This is kind of hacky - but I don't see a way out, atleast 
                // If the contents in pFtr at cchPreEndBody are subset of the
                // contents before </BODY> and after any previous text/tokens,
                // then we shouldn't do the following memcpy()
                while (    ichT >= 0 /* validation */
                        && (       pwNew[ichT] == ' '
                                || pwNew[ichT] == '\n'
                                || pwNew[ichT] == '\r'
                                || pwNew[ichT] == '\t'
                                )
                            )
                {
                    ichT--;
                    cchSubStr++;
                }
                ichT++; // compensate the last decrement
                if (cchSubStr > 0)
                {
                    ASSERT(ichT >= 0);
                    pwStr = new WCHAR [cchSubStr+1];
                    memcpy((BYTE *)pwStr, (BYTE *)(&pwNew[ichT]), cchSubStr*sizeof(WCHAR));
                    pwStr[cchSubStr] = '\0';
                    pwSubStr = new WCHAR [cchPreEndBody+1];
                    memcpy((BYTE *)pwSubStr, (BYTE *)pFtr, cchPreEndBody*sizeof(WCHAR));
                    pwSubStr[cchPreEndBody] = '\0';
                    pw = wcsstr(pwStr, pwSubStr);
                }
                if (pw == NULL) // means that the substring wasn't found
                {
                    // allocate more memory if needed
                    cbNeed = (ichNewCur+cchPreEndBody)*sizeof(WCHAR)+cbBufPadding;
                    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                        goto LErrorRet;


                    memmove((BYTE *)(&pwNew[ichInsEOL+cchPreEndBody]),
                            (BYTE *)(&pwNew[ichInsEOL]),
                            (ichNewCur-ichInsEOL)*sizeof(WCHAR));
                    memcpy( (BYTE *)(&pwNew[ichInsEOL]),
                            (BYTE *)(pFtr),
                            (cchPreEndBody)*sizeof(WCHAR));
                    ichNewCur += cchPreEndBody;
                }
                if (pwStr != NULL)
                    delete pwStr;
                if (pwSubStr != NULL)
                    delete pwSubStr;
            } // if (cchPreEndBody > 0)
        } // if (ichInsEOL != -1)

        ichBeginCopy = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac; // we don't want to copy anything after this
        iArray = ptep->m_cMaxToken - 1;

        // WISH LIST Item for space preservation        
        // we know that ptep->m_ispInfoBlock was the last spacing block that was recovered.
        // This block (like all others) has 4 parts (1)pre '<' (2)between '<>' & order info
        // (3)post '>' (4)pre matching '</'
        // At this point we care about (3) & (4)
        // first of all, get ichBeginNext (ich past '>') & ichBeginMatch (ich before '</')
        // apply the saved spacing info to the contents of pwNew

        // The difficult part is to get these ich's without parsing pwNew.
    }
    else
    {
        // copy our own Footer
        if (dwFlags & dwFilterSourceCode)
        {
            int ichBodyStart, index, ichBodyTagEnd;

            // get the '</' of </body>
            index = iArray;
            while (index >= 0) // we won't go this far, but just in case we have invalid html
            {
                if (   pTokArray[index].token.tok == TokTag_END
                    && pTokArray[index].token.tokClass == tokTag
                    )
                {
                    break;
                }
                index--;
            }
            if (index >= 0)
            {
                ichBodyStart = pTokArray[index].token.ibTokMin;
                // copy till the current token's begining, see if we have enough space
                if (ichBodyStart > (int)ichBeginCopy)
                {
                    cbNeed = (ichNewCur+ichBodyStart-ichBeginCopy+1/*for null at the end*/)*sizeof(WCHAR)+cbBufPadding;
                    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                        goto LErrorRet;
                    memcpy( (BYTE *)(&pwNew[ichNewCur]),
                            (BYTE *)(&pwOld[ichBeginCopy]),
                            (ichBodyStart-ichBeginCopy)*sizeof(WCHAR));
                    ichNewCur += (ichBodyStart-ichBeginCopy);
                    ichBeginCopy = ichBodyStart; // setting this is redundant, but it makes the code readable.
                }
                else if (ichBodyEnd > (int)ichBeginCopy)
                {
                    index = iArray;
                    while (index <= (int)ptep->m_cMaxToken) // we won't go this far, but just in case we have invalid html
                    {
                        if (   pTokArray[index].token.tok == TokTag_CLOSE
                            && pTokArray[index].token.tokClass == tokTag
                            )
                        {
                            break;
                        }
                        index++;
                    }
                    if (index < (int)ptep->m_cMaxToken)
                    {
                        ichBodyTagEnd = pTokArray[index].token.ibTokMac;
                        cbNeed = (ichNewCur+ichBodyTagEnd-ichBeginCopy+1/*for null at the end*/)*sizeof(WCHAR)+cbBufPadding;
                        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                            goto LErrorRet;
                        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                                (BYTE *)(&pwOld[ichBeginCopy]),
                                (ichBodyTagEnd-ichBeginCopy)*sizeof(WCHAR));
                        ichNewCur += (ichBodyTagEnd-ichBeginCopy);
                        ichBeginCopy = ichBodyTagEnd; // setting this is redundant, but it makes the code readable.
                    }
                }

                // add a null at the end 
                // to keep the code in ssync with the if (cchAfterBody > 0) case
                pwNew[ichNewCur++] = '\0';

                ichBeginCopy = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac; // we don't want to copy anything after this
                iArray = ptep->m_cMaxToken - 1;
            } // if (index >= 0)
        } // if (dwFlags & dwFilterSourceCode)
    }


    if (ptep->m_cchPTDTC != 0)
    {
        // this means that we didn't encounter the DTC on way out from Trident
        // but they were there when we went to Trident. The user must have deleted
        // the DTCs while in Design view
        ASSERT(ptep->m_ichPTDTC != 0);
        // remove m_cchPTDTC WCHARS from m_ichPTDTC
        memset( (BYTE *)&pwNew[ptep->m_ichPTDTC],
                0,
                ptep->m_cchPTDTC*sizeof(WCHAR)
                );
        memmove((BYTE *)&pwNew[ptep->m_ichPTDTC],
                (BYTE *)&pwNew[ptep->m_ichPTDTC+ptep->m_cchPTDTC],
                (ichNewCur-(ptep->m_ichPTDTC+ptep->m_cchPTDTC))*sizeof(WCHAR)
                );
        ichNewCur -= ptep->m_cchPTDTC;
        ptep->m_cchPTDTC = 0;
    }

LErrorRet:
    // unlock
    GlobalUnlock(ptep->m_hgDocRestore);

    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;
LRetOnly:
    return;

} /* fnRestoreFtr() */


void CTriEditParse::fnSaveObject(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD /*dwFlags*/)
{
    // scan till the end of the object. 
    // If we find '<% %>' blocks inside, put a comment with a special tag around it,
    // else simply copy that object as is and exit
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT ichObjectStart, ichObjectEnd, iObjectStart, iObjectEnd, i;
    BOOL fSSSFound = FALSE;
    UINT iArraySav = iArray;
    UINT cbNeed;

    ichObjectStart = ichObjectEnd = iObjectStart = iObjectEnd = 0;
    
    if (       (iArray-1 >= 0)
            && pTokArray[iArray-1].token.tok == TokTag_END
            && pTokArray[iArray-1].token.tokClass == tokTag
            )
    {
        iArray++;
        goto LRet;
    }
    ASSERT(pTokArray[iArray].token.tok == TokElem_OBJECT); // we should be at the object tag
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    iObjectStart = iArray;

    if (pTokArray[iArray].iNextprev != -1)
    {
        // NOTE that this will give us topmost nested level of the OBJECT, if we had nested objects
        iObjectEnd = pTokArray[iArray].iNextprev;
        ASSERT(iObjectEnd < (INT)ptep->m_cMaxToken);
        ASSERT((iObjectEnd-1 >= 0) && pTokArray[iObjectEnd-1].token.tok == TokTag_END);

        // this will be a wierd case where the iNextprev is incorrectly pointing to another token
        // but lets handle that case.
        if (pTokArray[iObjectEnd].token.tok != TokElem_OBJECT)
            goto LFindObjectClose; // find it by looking at each token
    }
    else // actually, this is an error case, but rather than just giving assert, try to find the token
    {
LFindObjectClose:
        i = iObjectStart+1;
        while (i < (INT)ptep->m_cMaxToken)
        {
            // this may not give us the correct matching </OBJECT> if we had nested objects.
            // but we don't have that knowledge at this point any way.
            if (   pTokArray[i].token.tok == TokElem_OBJECT
                && pTokArray[i].token.tokClass == tokElem
                && (i-1 >= 0) /* validation */
                && pTokArray[i-1].token.tok == TokTag_END
                )
            {
                break;
            }
            i++;
        }
        if (i < (INT)ptep->m_cMaxToken) // found TokElem_OBJECT token
            iObjectEnd = i;
        else // error case 
            goto LRet; // didn't find OBJECT, but exhausted the token array
    }
    // at this point iObjectStart & iObjectEnd point to OBJECT of <OBJECT> and iObjectEnd respectively
    // look for '<' in <OBJECT> & and '>' in </OBJECT>
    i = iObjectStart;
    while (i >= 0)
    {
        if (   pTokArray[i].token.tok == TokTag_START
            && pTokArray[i].token.tokClass == tokTag
            )
            break;
        i--;
    }
    if (i < 0) // error case
        goto LRet;
    iObjectStart = i;
    ichObjectStart = pTokArray[iObjectStart].token.ibTokMin;

    i = iObjectEnd;
    while (i <= (INT)ptep->m_cMaxToken)
    {
        if (   pTokArray[i].token.tok == TokTag_CLOSE
            && pTokArray[i].token.tokClass == tokTag
            )
            break;
        i++;
    }
    if (i >= (INT)ptep->m_cMaxToken) // error case
        goto LRet;
    iObjectEnd = i;
    ichObjectEnd = pTokArray[iObjectEnd].token.ibTokMac;
    ASSERT(ichObjectEnd > ichObjectStart);

    // look for <% %> between iObjectStart & iObjectEnd
    for (i = iObjectStart; i <= iObjectEnd; i++)
    {
        if (   pTokArray[i].token.tok == TokTag_SSSOPEN
            && pTokArray[i].token.tokClass == tokSSS
            )
        {
            fSSSFound = TRUE;
            break;
        }
    }
    if (fSSSFound) // this object can't be displayed in Trident, so convert it
    {
        LPCWSTR rgComment[] =
        {
            L"<!--ERROROBJECT ",
            L" ERROROBJECT-->",
        };

        //if (dwFlags & dwPreserveSourceCode)
        //{
            // in this case, we would have already copied <OBJECT ... DESIGNTIMESP=x>
            // and ichNewCur is adjusted accordingly
            // get ich that points after <OBJECT> in pwOld

            // I don't like this, but don't see a way out...
            // look back in pwNew and get ich that points to '<' of <OBJECT ... DESIGNTIMESP=x>
            // insert the comment there
        //}
        //else
        //{
            ASSERT((INT)(ichObjectStart-ichBeginCopy) > 0);
            cbNeed = (ichNewCur+ichObjectEnd-ichBeginCopy+wcslen(rgComment[0])+wcslen(rgComment[1]))*sizeof(WCHAR)+cbBufPadding;
            if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                goto LNoCopy;

            // copy till begining of <OBJECT>
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[ichBeginCopy]),
                    (ichObjectStart-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += ichObjectStart-ichBeginCopy;

            // copy the comment begining
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(rgComment[0]),
                    wcslen(rgComment[0])*sizeof(WCHAR));
            ichNewCur += wcslen(rgComment[0]);
            
            // copy from <OBJECT> to </OBJECT>
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[ichObjectStart]),
                    (ichObjectEnd-ichObjectStart)*sizeof(WCHAR));
            ichNewCur += ichObjectEnd-ichObjectStart;
            
            // copy the comment end
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(rgComment[1]),
                    wcslen(rgComment[1])*sizeof(WCHAR));
            ichNewCur += wcslen(rgComment[1]);
        //}
    }
    else
    {
        // We always save its contents into our buffer and replace it on the way back if need be
        // save cchClsId, clsId, cchParam, PARAM_Tags
        INT cchParam, ichParam, iParamStart, iParamEnd;
        INT ichObjStartEnd; // ich at the end of <OBJECT .....>
        LPCWSTR rgComment[] =
        {
            L"<!--ERRORPARAM ",
            L" ERRORPARAM-->",
        };
        INT iObjTagEnd = -1;

        iParamStart = iObjectStart;
        while (iParamStart < iObjectEnd)
        {
            //if (   pTokArray[iParamStart].token.tok == TokAttrib_CLASSID
            //  && pTokArray[iParamStart].token.tokClass == tokAttr)
            //  iClsId = iParamStart;
            if (   pTokArray[iParamStart].token.tok == TokElem_PARAM
                && pTokArray[iParamStart].token.tokClass == tokElem)
                break;
            iParamStart++;
        }
        if (iParamStart >= iObjectEnd) // don't see any <PARAM> tags, so don't save
            goto LSkipSave;

        while (iParamStart > iObjectStart) // generally this will the previous token, but cover all cases
        {
            if (   pTokArray[iParamStart].token.tok == TokTag_START
                && pTokArray[iParamStart].token.tokClass == tokTag)
                break;
            iParamStart--;
        }
        if (iParamStart <= iObjectStart) // error
            goto LSkipSave;
        ichParam = pTokArray[iParamStart].token.ibTokMin;

        iParamEnd = iObjectEnd;
        while (iParamEnd > iObjectStart)
        {
            if (   pTokArray[iParamEnd].token.tok == TokElem_PARAM
                && pTokArray[iParamEnd].token.tokClass == tokElem)
                break;
            iParamEnd--;
        }
        while (iParamEnd < iObjectEnd) // generally this will the previous token, but cover all cases
        {
            if (   pTokArray[iParamEnd].token.tok == TokTag_CLOSE
                && pTokArray[iParamEnd].token.tokClass == tokTag)
                break;
            iParamEnd++;
        }
        if (iParamEnd >= iObjectEnd) // error
            goto LSkipSave;
        cchParam = pTokArray[iParamEnd].token.ibTokMac - ichParam;
        ASSERT(cchParam > 0);

        // calculate ichObjStartEnd
        iObjTagEnd = iObjectStart;
        while (iObjTagEnd < iParamStart)
        {
            if (   pTokArray[iObjTagEnd].token.tok == TokTag_CLOSE
                && pTokArray[iObjTagEnd].token.tokClass == tokTag)
                break;
            iObjTagEnd++;
        }
        if (iObjTagEnd >= iParamStart) // error case
            goto LSkipSave;
        ichObjStartEnd = pTokArray[iObjTagEnd].token.ibTokMac;

        // realloc if needed
        cbNeed = (ichNewCur+cchParam+(ichObjStartEnd-ichBeginCopy)+wcslen(rgComment[0])+wcslen(rgComment[1]))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LSkipSave;

        // 1. copy <OBJECT ...> tag into pwNew
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichObjStartEnd-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichObjStartEnd-ichBeginCopy);
        ichBeginCopy = ichObjStartEnd;
#ifdef ERROR_PARAM
        // 2. now insert the <PARAM> tags as a comment at pwNew[ichNewCur]
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(rgComment[0]),
                wcslen(rgComment[0])*sizeof(WCHAR));
        ichNewCur += wcslen(rgComment[0]);

        // we should copy <PARAM> tags ONLY. We may have things other than the tags 
        // in between. e.g. comments
        // Look for TokElem_PARAM between iParamStart & iParamEnd
        ASSERT(pTokArray[iParamStart].token.tok == TokTag_START);
        ASSERT(pTokArray[iParamEnd].token.tok == TokTag_CLOSE);
        // Find PARAM tag, get the '<' & '>' for that PARAM and copy that to pwNew
        // repeat
        index = iParamStart;
        iPrev = iParamStart;
        while (index <= iParamEnd)
        {
            INT iStart, iEnd;

            iStart = iEnd = -1; // that way, its easy to make sure that this is initilized
            // get PARAM
            while (    (       pTokArray[index].token.tok != TokElem_PARAM
                            || pTokArray[index].token.tokClass != tokElem)
                    && (index <= iParamEnd)
                    )
                    index++;
            if (index > iParamEnd)
                goto LDoneCopy;
            // get '<' before the PARAM
            while (    (       pTokArray[index].token.tok != TokTag_START
                            || pTokArray[index].token.tokClass != tokTag)
                    && (index >= iPrev)
                    )
                    index--;
            if (index < iPrev)
                goto LDoneCopy;
            iStart = index;

            // get matching '>'
            while (    (       pTokArray[index].token.tok != TokTag_CLOSE
                            || pTokArray[index].token.tokClass != tokTag)
                    && (index <= iParamEnd)
                    )
                index++;

            if (index > iParamEnd)
                goto LDoneCopy;
            iEnd = index;
            ASSERT(iEnd > iStart);
            ASSERT(iStart != -1);
            ASSERT(iEnd != -1);
            memcpy( (BYTE *)(&pwNew[ichNewCur]),
                    (BYTE *)(&pwOld[pTokArray[iStart].token.ibTokMin]),
                    (pTokArray[iEnd].token.ibTokMac-pTokArray[iStart].token.ibTokMin)*sizeof(WCHAR));
            ichNewCur += (pTokArray[iEnd].token.ibTokMac-pTokArray[iStart].token.ibTokMin);
            iPrev = iEnd + 1;
        }
LDoneCopy:

        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(rgComment[1]),
                wcslen(rgComment[1])*sizeof(WCHAR));
        ichNewCur += wcslen(rgComment[1]);
#endif //ERROR_PARAM

        // fake iArraySav to be iObjTagEnd, that way we will st iArray correctly before we leave
        ASSERT(iObjTagEnd != -1);
        iArraySav = (UINT)iObjTagEnd;

LSkipSave:
        iArray = iArraySav + 1;
        goto LRet;
    }

LNoCopy:
    ichBeginCopy = ichObjectEnd; // set it for next copy
    iArray = iObjectEnd+1; // set it after </OBJECT>

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

//LRetOnly:
    return;

} /* fnSaveObject() */

void 
CTriEditParse::fnRestoreObject(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD /*dwFlags*/)
{
    // look for the special tag after the '<!--'
    // if we find it, this was an object, remove the comments around it
    // else simply copy the comment and return
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT iArraySav = iArray;
    INT ichCommentStart, ichCommentEnd, iCommentStart, iCommentEnd, cchComment1, cchComment2;
    INT ichObjectStart;
    LPCWSTR rgComment[] =
    {
        L"ERROROBJECT",
        L"--ERROROBJECT ",
        L" ERROROBJECT--",
        L"TRIEDITCOMMENT-",
        L"TRIEDITCOMMENTEND-",
        L"TRIEDITPRECOMMENT-",
    };
    BOOL fSimpleComment = FALSE;
    UINT cbNeed;

    ichCommentStart = ichCommentEnd = iCommentStart = iCommentEnd = 0;
    ASSERT(pTokArray[iArray].token.tok == TokTag_BANG); // we should be at the comment
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);

    // ASSUMPTION - that Trident doesn't muck with the contents inside a comment block

    // if rgComment[0] matches and rgComment[1] does not, Trident may have mucked with the 
    // comment contents. This invalidates our original assumption.
    // NOTE - In this version, we can get away by assuming that trident doesn't muck with the comments

    // early return cases
    // 1. see if this is a comment or not. It could be anything that starts with '<!'
    // e.g. <!DOCTYPE
    if (   (iArray+1 < (INT)ptep->m_cMaxToken)
        && (pwOld[pTokArray[iArray+1].token.ibTokMin] == '-')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+1] == '-')
        && (0 == _wcsnicmp(rgComment[0], &pwOld[pTokArray[iArray+1].token.ibTokMin+2], wcslen(rgComment[0])))
        )
    {
        iCommentStart = iArray; // this is a comment we are interested in
    }
    else if (      (iArray+1 < (INT)ptep->m_cMaxToken)
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] == '-')
                && (pwOld[pTokArray[iArray+1].token.ibTokMin+1] == '-')
                && (0 == _wcsnicmp(rgComment[3], &pwOld[pTokArray[iArray+1].token.ibTokMin+2], wcslen(rgComment[3])))
                )
    {
        fSimpleComment = TRUE; // BUG 14056 - Instead of going to LRet, process the comment for space preservation. We will save 3 strings that look similar to text run
    }
    else
    {
        iArray = iArraySav + 1; // not this one
        goto LRet;
    }
    // The first part matched, look at the end of the comment
    if (   (pwOld[pTokArray[iArray+1].token.ibTokMac-1] == '-')
        && (pwOld[pTokArray[iArray+1].token.ibTokMac-2] == '-')
        && (0 == _wcsnicmp( rgComment[0], 
                            &pwOld[pTokArray[iArray+1].token.ibTokMac-(wcslen(rgComment[0])+2)], 
                            wcslen(rgComment[0])
                            )
                        )
        )
    {
        iCommentEnd = iArray + 2;
        ASSERT(iCommentEnd < (INT)ptep->m_cMaxToken);
    }
    else // error case (our assumption was not valid). ignore and return with iArraySav+1
    {
        if (!fSimpleComment)
        {
            iArray = iArraySav + 1; // not this one
            goto LRet;
        }
    }

    if (!fSimpleComment)
    {
        // found the correct one
        cchComment1 = wcslen(rgComment[1]);
        cchComment2 = wcslen(rgComment[2]);
        // remove cchComment1 chars from begining of pwOld[pTokArray[iArray+1].token.ibTokMin
        // remove cchComment2 chars from the end of pwOld[pTokArray[iArray+1].token.ibTokMac
        // and copy the rest into pwNew

        // copy till begining of the comment
        ichCommentStart = pTokArray[iCommentStart].token.ibTokMin;
        ichObjectStart = pTokArray[iCommentStart+1].token.ibTokMin+cchComment1;
        ASSERT((INT)ichCommentStart-ichBeginCopy >= 0);

        cbNeed = (ichNewCur+(ichCommentStart-ichBeginCopy)+(pTokArray[iArray+1].token.ibTokMac-pTokArray[iArray+1].token.ibTokMin))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;

        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(pwOld+ichBeginCopy),
                (ichCommentStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += ichCommentStart-ichBeginCopy;
        ichBeginCopy = pTokArray[iCommentEnd].token.ibTokMac;

        ASSERT((INT)(pTokArray[iArray+1].token.ibTokMac-pTokArray[iArray+1].token.ibTokMin-cchComment1-cchComment2) >= 0);
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)&(pwOld[ichObjectStart]),
                (pTokArray[iArray+1].token.ibTokMac-pTokArray[iArray+1].token.ibTokMin-cchComment1-cchComment2)*sizeof(WCHAR));
        ichNewCur += pTokArray[iArray+1].token.ibTokMac-pTokArray[iArray+1].token.ibTokMin-cchComment1-cchComment2;
        iArray = iCommentEnd + 1;
    }
    else
    {
        int ichspBegin, ichspEnd, ichCopy;
        WCHAR *pwstr = NULL;

        // part 1 - copy till begining of the comment & apply spacing
        iCommentStart = iArraySav;
        ASSERT(pTokArray[iArraySav].token.tok == TokTag_BANG);
        ASSERT(pTokArray[iArraySav].token.tokClass == tokTag);

        iCommentEnd = iCommentStart + 2;
        ASSERT(pTokArray[iCommentEnd].token.tok == TokTag_CLOSE);
        ASSERT(pTokArray[iCommentEnd].token.tokClass == tokTag);

        ichCommentStart = pTokArray[iCommentStart].token.ibTokMin;
        ASSERT((INT)ichCommentStart-ichBeginCopy >= 0);
        cbNeed = (ichNewCur+ichCommentStart-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        memcpy( (BYTE *)&pwNew[ichNewCur],
                (BYTE *)&pwOld[ichBeginCopy],
                (ichCommentStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += ichCommentStart-ichBeginCopy;

        // make sure that we have enough space
        // to make this calculation simple, we assume the extreme case where every
        // character in the comment had end of line after it. i.e. we will insert
        // 2 characters ('\r\n') after each character in the comment when we restore 
        // the spacing. That means, as long as we have enough space for
        // (pTokArray[iCommentEnd].token.ibTokMac-pTokArray[iCommentStart].token.ibTokMin)*3
        // we are fine
        cbNeed = (ichNewCur+3*(pTokArray[iCommentEnd].token.ibTokMac-pTokArray[iCommentStart].token.ibTokMin))*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;

        
        // apply spacing for pre comment part
        // remove extsting spacing before the comment & add the saved spacing
        // note that we already have copied till the begining of the comment
        ichNewCur--;
        while (    (ichNewCur >= 0)
                && (   pwNew[ichNewCur] == ' '  || pwNew[ichNewCur] == '\t'
                    || pwNew[ichNewCur] == '\r' || pwNew[ichNewCur] == '\n'
                    )
                )
        {
            ichNewCur--;
        }
        ichNewCur++; // compensate, ichNewCur points to non-white space characher
        // now, start writing out the saved spacing
        // look for rgComment[4] & rgComment[5]
        ichspBegin = pTokArray[iCommentStart+1].token.ibTokMin + 2/*for --*/ + wcslen(rgComment[3]);
        pwstr = wcsstr(&pwOld[ichspBegin], rgComment[4]);// pwstr points just after the spacing info block
        if (pwstr == NULL) // didn't find the substring
        {
            // copy the entire comment as is
            memcpy( (BYTE *)&pwNew[ichNewCur],
                    (BYTE *)&pwOld[pTokArray[iCommentStart+1].token.ibTokMin],
                    (pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2)*sizeof(WCHAR));
            ichNewCur += pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2;
            goto LCommentEnd;
        }
        ichspBegin = SAFE_PTR_DIFF_TO_INT(pwstr+wcslen(rgComment[4])-pwOld);
        pwstr = wcsstr(&pwOld[ichspBegin], rgComment[5]);// pwstr points just after the spacing info block
        if (pwstr == NULL) // didn't find the substring
        {
            // copy the entire comment as is
            memcpy( (BYTE *)&pwNew[ichNewCur],
                    (BYTE *)&pwOld[pTokArray[iCommentStart+1].token.ibTokMin],
                    (pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2)*sizeof(WCHAR));
            ichNewCur += pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2;
            goto LCommentEnd;
        }
        ichCopy = SAFE_PTR_DIFF_TO_INT(pwstr-pwOld) + wcslen(rgComment[5]); // actual comment begins at ichCopy
        ichspEnd = SAFE_PTR_DIFF_TO_INT(pwstr-pwOld);
        ASSERT(ichspEnd >= ichspBegin);
        while (ichspBegin < ichspEnd)
        {
            switch(pwOld[ichspBegin])
            {
            case chCommentSp:
                pwNew[ichNewCur++] = ' ';
                break;
            case chCommentTab:
                pwNew[ichNewCur++] = '\t';
                break;
            case chCommentEOL:
                pwNew[ichNewCur++] = '\r';
                pwNew[ichNewCur++] = '\n';
                break;
            case ',':
                ASSERT(FALSE);
                break;
            }
            ichspBegin++;
        }
        // now pre comment spacing is restored
        

        pwNew[ichNewCur++] = '<';
        pwNew[ichNewCur++] = '!';
        pwNew[ichNewCur++] = '-';
        pwNew[ichNewCur++] = '-';

        // part 2 - copy the comment and apply spacing
        // from pTokArray[iCommentStart+1].token,ibTokMIn, look for rgComment[4]
        // thats where we keep our spacing info. Exclude this stuff while copying the comment
        ichspBegin = pTokArray[iCommentStart+1].token.ibTokMin + 2/*for --*/ + wcslen(rgComment[3]);
        // locate rgComment[4] that will be somewhere in iCommentStart'th token
        pwstr = wcsstr(&pwOld[ichspBegin], rgComment[4]);// pwstr points just after the spacing info block
        if (pwstr == NULL) // didn't find the substring
        {
            // copy the entire comment as is
            memcpy( (BYTE *)&pwNew[ichNewCur],
                    (BYTE *)&pwOld[pTokArray[iCommentStart+1].token.ibTokMin],
                    (pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2)*sizeof(WCHAR));
            ichNewCur += pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2;
            goto LCommentEnd;
        }
        ichspEnd = SAFE_PTR_DIFF_TO_INT(pwstr - pwOld);
        ASSERT(ichspEnd >= ichspBegin);

        while (ichspBegin < ichspEnd)
        {
            switch(pwOld[ichspBegin])
            {
            case chCommentSp:
                pwNew[ichNewCur++] = ' ';
                break;
            case chCommentTab:
                pwNew[ichNewCur++] = '\t';
                break;
            case chCommentEOL:
                pwNew[ichNewCur++] = '\r';
                pwNew[ichNewCur++] = '\n';
                break;
            case ',':
                while (    pwOld[ichCopy] == ' '    || pwOld[ichCopy] == '\t'
                        || pwOld[ichCopy] == '\r'   || pwOld[ichCopy] == '\n'
                        )
                {
                    if (ichCopy >= (int)(pTokArray[iCommentStart+1].token.ibTokMac-2)) // we are done with copying
                        goto LCommentEnd;
                    ichCopy++;
                }
                while (    pwOld[ichCopy] != ' '    && pwOld[ichCopy] != '\t'
                        && pwOld[ichCopy] != '\r'   && pwOld[ichCopy] != '\n'
                        )
                {
                    if (ichCopy >= (int)(pTokArray[iCommentStart+1].token.ibTokMac-2)) // we are done with copying
                        goto LCommentEnd;
                    pwNew[ichNewCur++] = pwOld[ichCopy++];
                }
                break;
            }
            ichspBegin++;
        }

LCommentEnd:
        // part 3 - copy the end of comment
        pwNew[ichNewCur++] = '-';
        pwNew[ichNewCur++] = '-';
        pwNew[ichNewCur++] = '>';

        // set iArray & ichBeginCopy for next run
        ichBeginCopy = pTokArray[iCommentEnd].token.ibTokMac;
        iArray = iCommentEnd + 1;
    }

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

//LRetOnly:
    return;

} /* fnRestoreObject()*/


void 
CTriEditParse::fnSaveSpace(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD /*dwFlags*/)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT iArray = (INT)*piArrayStart;
    INT ichEnd, ichBegin;
    LPWSTR pwNew = *ppwNew;
    INT iArraySav = iArray;
    LPCWSTR rgSpaceTags[] =
    {
        L" DESIGNTIMESP=",
        L" DESIGNTIMESP1=",
        L" designtimesp=",
    };
    INT iArrayElem = -1;
    INT iArrayMatch, iArrayPrevTag;
    INT ichEndMatch, ichBeginMatch, ichEndPrev, ichBeginPrev, ichEndNext, ichBeginNext, ichEndTag, ichBeginTag;
    WCHAR szIndex[cchspBlockMax]; // will we have more than 20 digit numbers as number of DESIGNTIMESPx?
    UINT cbNeed;
    int cchURL = 0;
    int ichURL = 0;

    //  {-1, TokTag_START, tokTag, TokTag_CLOSE, -1, tokClsIgnore, fnSaveSpace},

    ASSERT(dwFlags &dwPreserveSourceCode);

    // special cases where we don't need to save spacing, because Trident doesn't muck with 
    // the spacing in these cases. If this changes in future, remove these cases.
    // If this case is removed, then make sure that fnSaveObject() changes accordingly
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (pTokArray[iArray+1].token.tok == TokElem_OBJECT)
        && (pTokArray[iArray+1].token.tokClass == tokElem)
        )
    {
        // (iArray+1)th token is an OBJECT tag
        iArray = iArraySav + 1;
        goto LRet;
    }
    // trident munges custom attributes inside STYLE tag, so DESIGNTIMESP gets out of place
    // so lets not save any spacing info for TokElem_STYLE
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (pTokArray[iArray+1].token.tok == TokElem_STYLE)
        && (pTokArray[iArray+1].token.tokClass == tokElem)
        )
    {
        // (iArray+1)th token is an STYLE tag
        iArray = iArraySav + 1;
        goto LRet;
    }
    // trident overwrites PARAM tags, so we can skip saving spacing info
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (pTokArray[iArray+1].token.tok == TokElem_PARAM)
        && (pTokArray[iArray+1].token.tokClass == tokElem)
        )
    {
        // (iArray+1)th token is an PARAM tag
        iArray = iArraySav + 1;
        goto LRet;
    }

    // we should skip saving for <applet>
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (   pTokArray[iArray+1].token.tok == TokElem_APPLET
            )
            && (pTokArray[iArray+1].token.tokClass == tokElem)
        )
    {
        // (iArray+1)th token is an APPLET tag
        iArray = iArraySav + 1;
        goto LRet;
    }
    // we special case textarea tags, so we should skip saving spacing info
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (pTokArray[iArray+1].token.tok == TokElem_TEXTAREA)
        && (pTokArray[iArray+1].token.tokClass == tokElem)
        )
    {
        // (iArray+1)th token is TEXTAREA tag
        iArray = iArraySav + 1;
        goto LRet;
    }
    // we special case A/IMG/LINK tags with Relative URLs ONLY, so we should skip saving spacing info
    if (   (iArray+1 < (INT)ptep->m_cMaxToken) /* validation */
        && (   pTokArray[iArray+1].token.tok == TokElem_A
            || pTokArray[iArray+1].token.tok == TokElem_IMG
            || pTokArray[iArray+1].token.tok == TokElem_LINK
            )
        && (pTokArray[iArray+1].token.tokClass == tokElem)
        && (FURLNeedSpecialHandling(pTokArray, iArray, pwOld, (int)ptep->m_cMaxToken, &ichURL, &cchURL))
        )
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // step 1
    // look for > that matches with <. we already are at ft.tokBegin2 i.e. <
    ASSERT(pTokArray[iArray].token.tok == TokTag_START);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    ichBeginTag = pTokArray[iArray].token.ibTokMac;
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (pTokArray[iArray].token.tok == ft.tokEnd && pTokArray[iArray].token.tokClass == tokTag) // ft.tokEnd2 is -1
            break;
        if (pTokArray[iArray].token.tokClass == tokElem)
            iArrayElem = iArray;
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // didn't find >
    {
        goto LRet;
    }
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE); // found >
    ASSERT(pTokArray[iArray].token.tokClass == tokTag); // found >
    ichEndTag = ichBegin = pTokArray[iArray].token.ibTokMin;
    ichEnd = pTokArray[iArray].token.ibTokMac;

    // step 2
    // look for > before iArraySav. Boundary case will be for the first < in the document
    // save the spacing info
    ASSERT(pTokArray[iArraySav].token.tok == TokTag_START);
    ASSERT(pTokArray[iArraySav].token.tokClass == tokTag);
    ichEndPrev = pTokArray[iArraySav].token.ibTokMin;
    ichBeginPrev = ichEndPrev-1;
    // look for previous TokTag_CLOSE
    // if the tag ending tag, ichBeginPrev becomes ibTokMac of '>' tag
    // if the tag is starting tag, ichBeginPrev becomes ibTokMac+(white space just after that tag)
    iArrayPrevTag = iArraySav; // this is TokTag_START
    while (iArrayPrevTag >= 0)
    {
        if (       (   pTokArray[iArrayPrevTag].token.tokClass == tokTag 
                    && pTokArray[iArrayPrevTag].token.tok == TokTag_CLOSE
                    )
                || (   pTokArray[iArrayPrevTag].token.tokClass == tokSSS 
                    && pTokArray[iArrayPrevTag].token.tok == TokTag_SSSCLOSE
                    )/* VID6 - bug 22787 */
                )
        {
            break;
        }
        iArrayPrevTag--;
    }
    if (iArrayPrevTag < 0) // handle error case
    {
        // leave the old behaviour as is for V1
        while (ichBeginPrev >= 0)
        {
            if (   pwOld[ichBeginPrev] != ' '
                && pwOld[ichBeginPrev] != '\r'
                && pwOld[ichBeginPrev] != '\n'
                && pwOld[ichBeginPrev] != '\t'
                )
                break;
            ichBeginPrev--;
        }
        goto LGotEndNext;
    }
    ichBeginPrev = pTokArray[iArrayPrevTag].token.ibTokMac - 1;

LGotEndNext:
    if (ichBeginPrev < 0)
        ichBeginPrev = 0;
    else
        ichBeginPrev++;


    // step 3
    // look for TokTag_START after iArray(which currently is TokTag_CLOSE)
    // save spacing info
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    //iArrayNextStart = iArray;
    ichBeginNext = pTokArray[iArray].token.ibTokMac;
    ASSERT(ichBeginNext == ichEnd);
    ichEndNext = ichBeginNext;
    while (ichEndNext < (INT)pTokArray[ptep->m_cMaxToken-1].token.ibTokMac)
    {
        if (   pwOld[ichEndNext] != ' '
            && pwOld[ichEndNext] != '\r'
            && pwOld[ichEndNext] != '\n'
            && pwOld[ichEndNext] != '\t'
            )
            break;
        ichEndNext++;
    }

    if (ichEndNext >= (INT)pTokArray[ptep->m_cMaxToken-1].token.ibTokMac)
        ichEndNext = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac;

    // step 4
    // if iArrayElem != -1, look for pTokArray[iArrayElem].iNextprev. If its not -1, set iArrayMatch
    // look for previous TokTag_START/TokTag_END. look for previous TokTag_CLOSE
    // save spacing info
    if (iArrayElem == -1) // this can happen if we have incomplete HTML
    {
        ichEndMatch = ichBeginMatch = 0;
        goto LSkipMatchCalc;
    }
    iArrayMatch = pTokArray[iArrayElem].iNextprev;
    if (iArrayMatch != -1) // match was set while tokenizing
    {
        ichBeginMatch = ichEndMatch = 0; //init
        ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
        ASSERT(pTokArray[iArray].token.tokClass == tokTag);
        while (iArrayMatch >= iArray) // iArray is TokTag_CLOSE of the current tag (i.e. '>')
        {
            if (   pTokArray[iArrayMatch].token.tokClass == tokTag
                && (   pTokArray[iArrayMatch].token.tok == TokTag_START
                    || pTokArray[iArrayMatch].token.tok == TokTag_END
                    )
                )
                break;
            iArrayMatch--;
        }
        if (iArrayMatch > iArray) // did find '</' or '<' after the current tag
        {
            ichEndMatch = pTokArray[iArrayMatch].token.ibTokMin;
            ichBeginMatch = ichEndMatch; // init
            // look for '>' and set ichBeginMatch
            while (iArrayMatch >= iArray) // iArray is TokTag_CLOSE of the current tag (i.e. '>')
            {
                if (   (   pTokArray[iArrayMatch].token.tokClass == tokTag
                        && pTokArray[iArrayMatch].token.tok == TokTag_CLOSE
                        )
                    || (   pTokArray[iArrayMatch].token.tokClass == tokSSS
                        && pTokArray[iArrayMatch].token.tok == TokTag_SSSCLOSE
                        )/* VID6 - bug 22787 */
                    )
                    break;
                iArrayMatch--;
            }
            if (iArrayMatch >= iArray) // they may very well be the same
            {
                ichBeginMatch = pTokArray[iArrayMatch].token.ibTokMac;
                ASSERT(ichBeginMatch <= ichEndMatch);
                ASSERT(ichBeginMatch >= ichEnd);
            }
        }
    }
    else
    {
        // don't bother saving any info from here
        ichEndMatch = ichBeginMatch = 0;
    }
LSkipMatchCalc:
    if (ichEndPrev > ichBeginPrev)
        ptep->hrMarkSpacing(pwOld, ichEndPrev, &ichBeginPrev);
    else
        ptep->hrMarkSpacing(pwOld, ichEndPrev, &ichEndPrev);

    if (ichEndTag > ichBeginTag)
    {
        INT ichBeginTagSav = ichBeginTag;

        ptep->hrMarkSpacing(pwOld, ichEndTag, &ichBeginTag);
        // iArray'th token is TokTag_CLOSE & iArraySav is TokTag_START
        ptep->hrMarkOrdering(pwOld, pTokArray, iArraySav, iArray, ichEndTag, &ichBeginTagSav);
    }
    else
    {
        INT ichEndTagSav = ichEndTag;

        ptep->hrMarkSpacing(pwOld, ichEndTag, &ichEndTag);
        // iArray'th token is TokTag_CLOSE & iArraySav is TokTag_START
        ptep->hrMarkOrdering(pwOld, pTokArray, iArraySav, iArray, ichEndTagSav, &ichEndTagSav);
    }

    if (ichEndNext > ichBeginNext)
        ptep->hrMarkSpacing(pwOld, ichEndNext, &ichBeginNext);
    else
        ptep->hrMarkSpacing(pwOld, ichEndNext, &ichEndNext);

    if (ichEndMatch > ichBeginMatch)
        ptep->hrMarkSpacing(pwOld, ichEndMatch, &ichBeginMatch);
    else
        ptep->hrMarkSpacing(pwOld, ichEndMatch, &ichEndMatch);



    // realloc if needed
    cbNeed = (ichNewCur+ichBegin-ichBeginCopy+3*wcslen(rgSpaceTags[0])+(ichEnd-ichBegin))*sizeof(WCHAR);
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LErrorRet;
    // ichBeginCopy is a position in pwOld and
    // ichNewCur is a position in pwNew
    // copy from ichBeginCopy to >
    ASSERT((INT)(ichBegin-ichBeginCopy) >= 0);
    if ((INT)(ichBegin-ichBeginCopy) > 0)
    {
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(pwOld+ichBeginCopy),
                (ichBegin-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichBegin-ichBeginCopy);
    }
    ichBeginCopy = ichEnd; // make it ready for next copy

    // BUG 15389 - Ideal fix will be to save the exact tag and restore it when we switch back,
    // but it will be a bigger change at this point, So we simply look at first character of the tag.
    // If it is uppercase, write DESIGNTIMESP, else write designtimesp
    // ASSUMPTION is that Trident doesn't change the case of unknown attribute & so far its TRUE.
    // ASSUMPTION is that we don't have extra spaces between '<' & the tag name.
    ASSERT(wcslen(rgSpaceTags[0]) == wcslen(rgSpaceTags[2]));
    if (iswupper(pwOld[pTokArray[iArraySav+1].token.ibTokMin]) != 0) // upper case
    {
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(rgSpaceTags[0]),
                (wcslen(rgSpaceTags[0]))*sizeof(WCHAR));
        ichNewCur += wcslen(rgSpaceTags[0]);
    }
    else
    {
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(rgSpaceTags[2]),
                (wcslen(rgSpaceTags[2]))*sizeof(WCHAR));
        ichNewCur += wcslen(rgSpaceTags[2]);
    }

    (WCHAR)_itow(ptep->m_ispInfoBlock+ptep->m_ispInfoBase, szIndex, 10);
    ptep->m_ispInfoBlock++;

    ASSERT(wcslen(szIndex) < sizeof(szIndex));
    ASSERT(sizeof(szIndex) == cchspBlockMax*sizeof(WCHAR));
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(szIndex),
            wcslen(szIndex)*sizeof(WCHAR));
    ichNewCur += wcslen(szIndex);


    // if (m_ispInfoIn == 0), then we have the last block of SPINFO, lets save it here
    if (ptep->m_ispInfoIn == 0)
    {
        ASSERT(FALSE);
        // realloc if needed
        cbNeed = (ichNewCur+ichBegin-ichBeginCopy+2*wcslen(rgSpaceTags[1]))*sizeof(WCHAR);
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LErrorRet;
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(rgSpaceTags[1]),
                (wcslen(rgSpaceTags[1]))*sizeof(WCHAR));
        ichNewCur += wcslen(rgSpaceTags[1]);

        *(WCHAR *)(pwNew+ichNewCur) = 'Z'; // ptep->m_ispInfoIn;
        ichNewCur++;
    }

    ASSERT((INT)(ichEnd-ichBegin) > 0);
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(pwOld+ichBegin),
            (ichEnd-ichBegin)*sizeof(WCHAR));
    ichNewCur += (ichEnd-ichBegin);

    // restore iArray
    iArray = iArraySav+1;

LErrorRet:
LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = (UINT)iArray;
} /* fnSaveSpace() */


void
CTriEditParse::fnRestoreSpace(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT *pcHtml, UINT *pichNewCur, UINT *pichBeginCopy, DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT iArray = (INT)*piArrayStart;
    UINT ichBegin, ichspInfoEndtagEnd;
    LPWSTR pwNew = *ppwNew;
    LPCWSTR rgSpaceTags[] =
    {
        L"DESIGNTIMESP",
        L"DESIGNTIMESP1",
    };
    INT iArraySav = iArray;
    WORD *pspInfoEnd, *pspInfoOrder;
    INT cchwspInfo; // spInfo block size in wide chars
    INT cchRange; // number of char for which this spInfo was saved
    BOOL fMatch = FALSE;
    BOOL fMatchLast = FALSE;
    INT cchtok, cchtag, itoktagStart, ichtoktagStart, iArrayValue, index;
    WCHAR szIndex[cchspBlockMax];
    INT cwOrderInfo = 0;
    UINT cbNeed;
    INT ichNewCurSav = -1; // init to -1 so that we will know when its set.
    int ichNewCurAtIndex0 = -1; // we need to adjust the saved ichNewCur because it gets invalidated
                                // as soon as the tag moves as a result of restoring pre-tag spaces.
    
    ASSERT(dwFlags & dwPreserveSourceCode);

    // take care of the matching end token's spacing
    if (       pTokArray[iArray].token.tok == ft.tokBegin2
            && pTokArray[iArray].token.tokClass == tokTag
            )
    {
        ASSERT(ft.tokBegin2 == TokTag_END);
        fnRestoreSpaceEnd(  ptep, pwOld, ppwNew, pcchNew, phgNew, pTokArray, piArrayStart, 
                            ft, pcHtml, pichNewCur, pichBeginCopy, dwFlags);
        goto LRetOnly;

    }

    // we already are at (token.tok == tokSpace), which may be DESIGNTIMESPx
    ASSERT(pTokArray[iArray].token.tok == 0);
    ASSERT(pTokArray[iArray].token.tokClass == tokSpace);
    cchtok = pTokArray[iArray].token.ibTokMac - pTokArray[iArray].token.ibTokMin;
    cchtag = wcslen(rgSpaceTags[0]);
    if (cchtag == cchtok)
    {
        if (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[iArray].token.ibTokMin], cchtag))
        {
            fMatch = TRUE;// match
        }
        else
            goto LNoMatch;
    }
    else if (cchtag+1 == cchtok)
    {
        if (0 == _wcsnicmp(rgSpaceTags[1], &pwOld[pTokArray[iArray].token.ibTokMin], cchtag+1))
        {
            fMatchLast = TRUE;// match
        }
        else
            goto LNoMatch;
    }
    else
    {
LNoMatch:
        iArray = iArraySav + 1;
        goto LRet;
    }

    ASSERT(fMatch || fMatchLast); // one of them has to be TRUE
    // found DESIGNTIMESPx. Now, go backwords and look for ft.tokBegin
    itoktagStart = iArray;
    ASSERT(ft.tokBegin == TokTag_START);
    while (itoktagStart >= 0)
    {
        if (       pTokArray[itoktagStart].token.tok == ft.tokBegin
                && pTokArray[itoktagStart].token.tokClass == tokTag
                )
        {
            break;
        }
        itoktagStart--;
    }
    if (itoktagStart < 0) // didn't find '<' before DESIGNTIMESPx
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // found '<' before DESIGNTIMESPx
    // the spacing info saved was for the portion of the document before the '<'
    ASSERT(pTokArray[itoktagStart].token.tok == TokTag_START);
    ASSERT(pTokArray[itoktagStart].token.tokClass == tokTag);
    // we already know that iArray'th token is DESIGNTIMESPx, so get past the '=' that follows it
    // ASSUMPTION - the value of attribute DESIGNTIMESPx will NOT get munged by Trident.
    // NOTE - the above assumption is correct for this release of Trident
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '=')
        {
            ASSERT(pTokArray[iArray].token.tokClass == tokOp);
            break;
        }
        else if (*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '>') // gone too far
            goto LSkip1;
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // didn't find = after DESIGNTIMESPx
    {
LSkip1:
        iArray = iArraySav + 1;
        goto LRet;
    }
    iArrayValue = -1;
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (   (iArrayValue == -1)
            && (   (pTokArray[iArray].token.tokClass == tokValue)
                || (pTokArray[iArray].token.tokClass == tokString)
                )
            )
            iArrayValue = iArray;
        else if (      pTokArray[iArray].token.tok == TokTag_CLOSE
                    && pTokArray[iArray].token.tokClass == tokTag
                    )
        {
            ASSERT(*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '>');
            break;
        }
        iArray++;
    }
    if (iArrayValue == -1 || iArray >= (int)ptep->m_cMaxToken) // didn't find tokValue after DESIGNTIMESPx
    {
        // BUG 9040
        //if (iArray >= (int)ptep->m_cMaxToken && iArrayValue != -1)
        //{
            // SOLUTION 1
            // overwrite the stuff from pwOld[pTokArray[iArraySav].token.ibTokMin]
            // to pwOld[pTokArray[iArrayValue].token.ibTokMac - 1]
            // SOLUTION 2
            // look for DESIGNTIMESP from pwOld[pTokArray[itokTagStart].token.ibTokMac - 1]
            // to pwOld[pTokArray[iArray].token.ibTokMac - 1] and overwrite all of those 
            // strings with spaces. We could NULL those and do the blts, but why bother
            // when the html isn't valid! 

            // make sure that all DESIGNTIMESPs are stripped off if we encountered this error case
        //}
        iArray = iArraySav + 1;
        goto LRet;
    }

    // we know that 4 blocks of info was saved for each DESIGNTIMESPx attribute
    // before tag, within tag, after tag, before matching end-tag
    // even if no info was saved, the block will still exist with 2 words (size,# of char)
    ichspInfoEndtagEnd = pTokArray[iArray].token.ibTokMac;

    // first copy the document till DESIGNTIMESPx
    // skip DESIGNTIMESPx and its value and set ichBeginCopy to be after that

    // NOTE - token before iArraySav'th one should be tokSpace with lenght 1 
    // and with a value of chSpace (unless Trident has modified it). If thats TRUE,
    // we should skip that too, because we added it when we put in DESIGNTIMESPx.
    
    // fix Trident's behaviour - If Trident sees unknown tag(s) it puts it(them) at the end 
    // and inserts EOL before those. In this case, we would have inserted a space before DESIGNTIMESP
    // and Trident would have inserted EOL. If thats not the case, we will ignore it.
    if (   (iArraySav-1 > 0) /* validation */
        && (    (      (pTokArray[iArraySav-1].token.ibTokMac - pTokArray[iArraySav-1].token.ibTokMin == 1)
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin] == ' ')
                    )
            ||  (      (pTokArray[iArraySav-1].token.ibTokMac - pTokArray[iArraySav-1].token.ibTokMin == 3)
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin] == ' ')
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin+1] == '\r')
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin+2] == '\n')
                    )
                )
        )
    {
        ichBegin = pTokArray[iArraySav-1].token.ibTokMin;
    }
    else
        ichBegin = pTokArray[iArraySav].token.ibTokMin;
    ASSERT(ichBegin >= ichBeginCopy);

    cbNeed = (ichNewCur+(ichBegin-ichBeginCopy))*sizeof(WCHAR) + cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    // BUG 15389 - look at the case of DESIGNTIMESP & convert the tag into upper/lower case...
    //memcpy(   (BYTE *)(pwNew+ichNewCur),
    //      (BYTE *)(pwOld+ichBeginCopy),
    //      (ichBegin-ichBeginCopy)*sizeof(WCHAR));
    //ichNewCur += (ichBegin-ichBeginCopy);
    if (ichBegin-ichBeginCopy >= 0)
    {
        // step 1 - copy from ichBeginCopy to '<' of the current tag
        if ((int)(pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy) > 0)
        {
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+ichBeginCopy),
                    (pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy);
            ichNewCurSav = ichNewCur+1; // used as a peg to get preceding tokTag_START i.e. '<'
        }
        // step 2 - convert current tag into upper/lower case & copy it
        if (ichBeginCopy < pTokArray[itoktagStart+1].token.ibTokMin)
        {
            ASSERT((int)(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin) > 0);
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+pTokArray[itoktagStart+1].token.ibTokMin),
                    (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)*sizeof(WCHAR));
            if (iswupper(pwOld[pTokArray[iArraySav].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
            {
                // convert the tag into upper case. ASSUME that the tag is at itoktagStart+1
                _wcsupr(&pwNew[ichNewCur]);
            }
            else
            {
                // convert the tag into lower case. ASSUME that the tag is at itoktagStart+1
                _wcslwr(&pwNew[ichNewCur]);
            }
            ichNewCur += (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin);
        }
        else // this tag is alreay been copied
        {
            // hack
            if (pTokArray[itoktagStart+1].token.ibTokMac == ichBeginCopy) // means we are just past the current tag
            {
                if (iswupper(pwOld[pTokArray[iArraySav].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
                {
                    ASSERT(ichNewCur >= (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin));
                    // convert the tag into upper case. ASSUME that the tag is at itoktagStart+1
                    _wcsupr(&pwNew[ichNewCur-(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)]);
                }
                else
                {
                    ASSERT(ichNewCur >= (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin));
                    // convert the tag into lower case. ASSUME that the tag is at itoktagStart+1
                    _wcslwr(&pwNew[ichNewCur-(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)]);
                }
            }
        }
        // step 3 - copy from after the tag (which is at ichtoktagStart+1) to ichBegin
        if ((int)(ichBegin-pTokArray[itoktagStart+1].token.ibTokMac) > 0)
        {
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+pTokArray[itoktagStart+1].token.ibTokMac),
                    (ichBegin-pTokArray[itoktagStart+1].token.ibTokMac)*sizeof(WCHAR));
            ichNewCur += (ichBegin-pTokArray[itoktagStart+1].token.ibTokMac);
        }
    }
    // set ichBeginCopy
    ichBeginCopy = ichspInfoEndtagEnd; // make it ready for next copy

    // copy the rest of the tag (skipping DESIGNTIMESPx = value)
    ASSERT((INT)(ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac) >= 0);
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMac),
            (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac)*sizeof(WCHAR));
    ichNewCur += (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac);
    
    memset((BYTE *)szIndex, 0, sizeof(szIndex));
    // check if the value has quotes around it and don't copy them to szIndex
    if (   pwOld[pTokArray[iArrayValue].token.ibTokMin] == '"'
        && pwOld[pTokArray[iArrayValue].token.ibTokMac-1] == '"'
        )
    {
        memcpy( (BYTE *)szIndex,
                (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin+1),
                (pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin-2)*sizeof(WCHAR));
    }
    else
    {
        memcpy( (BYTE *)szIndex,
                (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin),
                (pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin)*sizeof(WCHAR));
    }
    ptep->m_ispInfoBlock = _wtoi(szIndex);
    ptep->m_ispInfoBlock -= ptep->m_ispInfoBase;
    if (ptep->m_ispInfoBlock < 0)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // NOTE - we can cache this info in a link list at the begining
    // get to the ptep->m_ispInfoBlock'th block from ptep->m_pspInfoOutStart
    ASSERT(ptep->m_cchspInfoTotal >= 0);
    pspInfoEnd = ptep->m_pspInfoOutStart + ptep->m_cchspInfoTotal;
    ptep->m_pspInfoOut = ptep->m_pspInfoOutStart;
    for (index = 0; index < ptep->m_ispInfoBlock; index++)
    {
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before <
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // between <>
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // Order Info
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // after >
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before matching </

        // we somehow have gone beyond the data that was saved for spacing
        if (ptep->m_pspInfoOut >= pspInfoEnd)
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
    }

    // get the Order Info
    pspInfoOrder = ptep->m_pspInfoOut;
    pspInfoOrder += *(WORD *)pspInfoOrder; // skip info saved for spacing before '<'
    pspInfoOrder += *(WORD *)pspInfoOrder; // skip info saved for spacing between '<>'
    // now pspInfoOrder is at correct place
    cwOrderInfo = *(WORD *)pspInfoOrder++;
    ASSERT(cwOrderInfo >= 1);
    // process this info
    if (cwOrderInfo > 1) // means that we saved some info
    {
        INT cchNewCopy;

        cchNewCopy = (ichBegin-pTokArray[itoktagStart].token.ibTokMin) + (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac);
        ptep->FRestoreOrder(pwNew, pwOld, pspInfoOrder, &ichNewCur, cwOrderInfo, pTokArray, itoktagStart, iArray, iArraySav, iArrayValue, cchNewCopy, phgNew);
    }
    ichtoktagStart = ichNewCur; // init
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    for (index = 0; index < 4; index++)
    {
        BOOL fLookback = FALSE;

        cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
        cchRange = *(WORD *)ptep->m_pspInfoOut++;
        if (cchwspInfo == 2) // we didn't save any spacing info
        {
            if (index == 0) // special case BUG 8741
            {
                // Note that we didn't save anything before this tag. which means that
                // we had '>' or some text immediately before the < tag. 
                ichtoktagStart = ichNewCur;
                while (ichtoktagStart >= 0)
                {
                    if (pwNew[ichtoktagStart] == '<')
                    {
                        ichtoktagStart--;
                        break;
                    }
                    ichtoktagStart--;
                }
                if (ichtoktagStart >= 0)
                {
                    int cws = 0;
                    int ichtagStart = ichtoktagStart;

                    // remove any such white space trident inserts.
                    while (    pwNew[ichtoktagStart] == ' '
                            || pwNew[ichtoktagStart] == '\r'
                            || pwNew[ichtoktagStart] == '\n'
                            || pwNew[ichtoktagStart] == '\t')
                    {
                        cws++;
                        ichtoktagStart--;
                    }
                    if (cws > 0)
                    {
                        ASSERT((int)(ichNewCur-ichtagStart-1) >= 0);
                        //ichtokTagStart now points to either '>' or a non-whitespace char
                        memmove((BYTE*)&pwNew[ichtoktagStart+1],
                                (BYTE*)&pwNew[ichtoktagStart+1+cws],
                                (ichNewCur-ichtagStart-1)*sizeof(WCHAR));
                        ichNewCur -= cws;
                    }
                } // if (ichtoktagStart >= 0)
            } // if (index == 0)
            goto LNext;
        }

        // note that ichtoktagStart is a position in pwNew
        switch (index)
        {
        case 0: // before < of the tag
            fLookback = TRUE;
            ichtoktagStart = (ichNewCurSav == -1)?ichNewCur:ichNewCurSav;// handle < ... <%..%>...> case correctly
            ichNewCurAtIndex0 = ichNewCur; // lets save the ichNewCur before we restore pre-tag spacing
            while (ichtoktagStart >= 0)
            {
                if (pwNew[ichtoktagStart] == '<' && pwNew[ichtoktagStart+1] != '%')
                {
                    ichtoktagStart--;
                    break;
                }
                ichtoktagStart--;
            }
            if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
            {
                ptep->m_pspInfoOut += cchwspInfo-2;
                continue;
            }
            break;
        case 1: // between <> of the tag
            fLookback = FALSE;
            // NOTE - we can assume that in 'case 0' we had put ichtoktagStart is just before '<'
            // so that we can avoid this while loop. but what if we skipped case '0'?

            // adjust ichNewCurSav to reflect the pre-tag spacing so that it doesn't become invalid
            // we may need to adjust it in ichNewCur-ichNewCurAtIndex0 < 0 case as well, but lets not
            // add code at this stage that we don't have to. (4/30/98)
            if (ichNewCurAtIndex0 != -1 && ichNewCurSav != -1 && ichNewCur-ichNewCurAtIndex0 > 0)
                ichNewCurSav = ichNewCurSav + (ichNewCur-ichNewCurAtIndex0);
            ichtoktagStart = (ichNewCurSav == -1)?ichNewCur:ichNewCurSav;// handle < ... <%..%>...> case correctly
            while (ichtoktagStart >= 0)
            {
                if (pwNew[ichtoktagStart] == '<' && pwNew[ichtoktagStart+1] != '%')
                {
                    ichtoktagStart++;
                    break;
                }
                ichtoktagStart--;
            }
            if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
            {
                ptep->m_pspInfoOut += cchwspInfo-2; // for spacing info
                ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // for Order Info
                continue;
            }
            break;
        case 2: // after > of the tag
            // Observation - Trident messes up the document in following way - 
            //    If we had an EOL after '>' which is followed by HTML text, 
            //    trident eats that EOL
            // BUT
            //    If we had a space/tab before that EOL trident doesn't eat it!!!
            // so I have added the conditions
            // && (pwOld[pTokArray[iArray+1].token.ibTokMin] != ' ')
            // && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\t')

            // here is the deal - If the next tone happens to be plain text, there is no danger
            // of applying the same format twice.( i.e. once for after '>' and the next time for
            // before the next '<')
            if (   (iArray+1 < (INT)ptep->m_cMaxToken) /*validation*/
                && pTokArray[iArray+1].token.tok == 0
                && pTokArray[iArray+1].token.tokClass == tokIDENTIFIER
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\r')
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != ' ')
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\t')
                )
            {
                fLookback = FALSE;
                ichtoktagStart = ichNewCur;
                while (ichtoktagStart >= 0)
                {
                    if (pwNew[ichtoktagStart] == '>')
                    {
                        ichtoktagStart++;
                        break;
                    }
                    ichtoktagStart--;
                }
                if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
                {
                    ptep->m_pspInfoOut += cchwspInfo-2;
                    continue;
                }
            }
            else
            {
                ptep->m_pspInfoOut += cchwspInfo-2; // we ignore this info
                continue;
            }
            break;
        case 3: // before matching end tag
            ptep->m_pspInfoOut += cchwspInfo-2; // we ignore this info
            continue;
            //fLookback = TRUE;
            //ichtoktagStart = 0; // we ignore this info
            break;
        }

        if (index == 3) // skip this info, because we have not reached matching end tag yet
            ptep->m_pspInfoOut += cchwspInfo-2;
        //else if (index == 0)
        //  ptep->FRestoreSpacingInHTML(pwNew, pwOld, &ichNewCur, &cchwspInfo, cchRange, ichtoktagStart, fLookback, index);
        else
            ptep->FRestoreSpacing(pwNew, pwOld, &ichNewCur, &cchwspInfo, cchRange, ichtoktagStart, fLookback, index);

LNext:
        if (index == 1) // we have already processed this info, just move the pointer ahead
            ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut;

    } // for ()

    iArray++; // go part > of this tag for the next round

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = (UINT)iArray;

LRetOnly:
    return;

} /* fnRestoreSpace() */




void
CTriEditParse::fnRestoreSpaceEnd(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
              TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok ft,
              INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy, DWORD /*dwFlags*/)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT iArray = (INT)*piArrayStart;
    LPWSTR pwNew = *ppwNew;
    LPCWSTR rgSpaceTags[] =
    {
        L"DESIGNTIMESP",
    };
    INT iArraySav = iArray;
    INT iArrayMatch, i, itoktagStart;
    BOOL fMatch = FALSE;
    INT cchtag;
    WORD *pspInfoEnd;
    INT cchwspInfo; // spInfo block size in wide chars
    INT cchRange; // number of char for which this spInfo was saved
    INT ichtoktagStart, iArrayValue, index;
    WCHAR szIndex[cchspBlockMax];
    int iDSP = -1;
    UINT cbNeed;
    
    ASSERT(dwFlags & dwPreserveSourceCode);

    // take care of the matching end token's spacing
    ASSERT(pTokArray[iArray].token.tok == ft.tokBegin2);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    ASSERT(ft.tokBegin2 == TokTag_END);

    // We already are at (token.tok == TokTag_END)
    // Get the tokElem after the current token and find its matching begin token
    // If we don't find the begin token, we don't have spacing for this end token, return
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (pTokArray[iArray].token.tokClass == tokElem) // generally this will be the next token
            break;
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    
    if (pTokArray[iArray].iNextprev == -1)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    iArrayMatch = pTokArray[iArray].iNextprev;
    // look for 'DESIGNTIMESP' from iArrayMatch till the next '>'
    // If we don't find 'DESIGNTIMESP', this is an error case, return.
    i = iArrayMatch;
    cchtag = wcslen(rgSpaceTags[0]);
    while (    i < iArraySav /* boundary case */
            && (   pTokArray[i].token.tokClass != tokTag
                || pTokArray[i].token.tok != TokTag_CLOSE
                )
            )
    {

        if (   pTokArray[i].token.tokClass == tokSpace
            && cchtag == (int)(pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin)
            && (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[i].token.ibTokMin], cchtag))
            )
        {
            fMatch = TRUE;
            break;
        }
        i++;
    }
    if (!fMatch)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    // at this point pTokArray[i] is 'DESIGNTIMESP'
    iDSP = i; // save for later use when we convert the tokElem to upper/lower case
    itoktagStart = i;
    while (itoktagStart >= 0)
    {
        if (       pTokArray[itoktagStart].token.tok == ft.tokBegin
                && pTokArray[itoktagStart].token.tokClass == tokTag
                )
        {
            break;
        }
        itoktagStart--;
    }
    if (itoktagStart < 0) // didn't find '<' before DESIGNTIMESPx
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // found '<' before DESIGNTIMESPx
    ASSERT(pTokArray[itoktagStart].token.tok == TokTag_START);
    ASSERT(pTokArray[itoktagStart].token.tokClass == tokTag);
    // we already know that i'th token is DESIGNTIMESPx, so get past the '=' that follows it
    // ASSUMPTION - the value of attribute DESIGNTIMESPx will NOT get munged by Trident.
    // NOTE - The above assumption is correct for this Trident release.
    while (i < iArraySav)
    {
        if (*(WORD *)(pwOld+pTokArray[i].token.ibTokMin) == '=')
        {
            ASSERT(pTokArray[i].token.tokClass == tokOp);
            break;
        }
        else if (*(WORD *)(pwOld+pTokArray[i].token.ibTokMin) == '>') // gone too far
            goto LSkip1;
        i++;
    }
    if (i >= iArraySav) // didn't find = after DESIGNTIMESPx
    {
LSkip1:
        iArray = iArraySav + 1;
        goto LRet;
    }
    iArrayValue = -1;
    while (i < iArraySav)
    {
		if (   (iArrayValue == -1)
			&& (   pTokArray[i].token.tokClass == tokValue 
				|| pTokArray[i].token.tokClass == tokString)
			)
            iArrayValue = i;
        else if (      pTokArray[i].token.tok == TokTag_CLOSE
                    && pTokArray[i].token.tokClass == tokTag
                    )
        {
            ASSERT(*(WORD *)(pwOld+pTokArray[i].token.ibTokMin) == '>');
            break;
        }
        i++;
    }
    if (iArrayValue == -1)/*BUG 7951 || i >= iArraySav)*/ // didn't find tokValue after DESIGNTIMESPx
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // we know that iArraySav'th token is '</', copy till that token and apply spacing
    cbNeed = (ichNewCur+pTokArray[iArraySav].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;
    ASSERT(pTokArray[iArraySav].token.ibTokMin >= ichBeginCopy);
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(pwOld+ichBeginCopy),
            (pTokArray[iArraySav].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += (pTokArray[iArraySav].token.ibTokMin-ichBeginCopy);
    ichtoktagStart = ichNewCur-1;
    
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(pwOld+pTokArray[iArraySav].token.ibTokMin),
            (pTokArray[iArraySav].token.ibTokMac-pTokArray[iArraySav].token.ibTokMin)*sizeof(WCHAR));
    ichNewCur += (pTokArray[iArraySav].token.ibTokMac-pTokArray[iArraySav].token.ibTokMin);
    ichBeginCopy = pTokArray[iArraySav].token.ibTokMac; // make it ready for next copy

    // we know that 4 blocks of info was saved for each DESIGNTIMESPx attribute
    // before tag, within tag, after tag, before matching end-tag
    // even if no info was saved, the block will still exist with 2 words (size,# of char)
    memset((BYTE *)szIndex, 0, sizeof(szIndex));
	// check if the value has quotes around it and don't copy them to szIndex
	if (   pwOld[pTokArray[iArrayValue].token.ibTokMin] == '"'
		&& pwOld[pTokArray[iArrayValue].token.ibTokMac-1] == '"'
		)
	{
		memcpy( (BYTE *)szIndex,
				(BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin+1),
				(pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin-2)*sizeof(WCHAR));
	}
	else
	{
		memcpy( (BYTE *)szIndex,
				(BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin),
				(pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin)*sizeof(WCHAR));
	}
    ptep->m_ispInfoBlock = _wtoi(szIndex);
    ptep->m_ispInfoBlock -= ptep->m_ispInfoBase;
    if (ptep->m_ispInfoBlock < 0)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // NOTE - we can cache this info in a link list at the begining
    // get to the ptep->m_ispInfoBlock'th block from ptep->m_pspInfoOutStart
    ASSERT(ptep->m_cchspInfoTotal >= 0);
    pspInfoEnd = ptep->m_pspInfoOutStart + ptep->m_cchspInfoTotal;
    ptep->m_pspInfoOut = ptep->m_pspInfoOutStart;
    for (index = 0; index < ptep->m_ispInfoBlock; index++)
    {
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before <
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // between <>
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // Order Info
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // after >
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before matching </

        // we somehow have gone beyond the data that was saved for spacing
        if (ptep->m_pspInfoOut >= pspInfoEnd)
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
    }

    // skip pre '<' data
    cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
    cchRange = *(WORD *)ptep->m_pspInfoOut++;
    ptep->m_pspInfoOut += cchwspInfo - 2;
    // skip '<...>' data
    cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
    cchRange = *(WORD *)ptep->m_pspInfoOut++;
    ptep->m_pspInfoOut += cchwspInfo - 2;
    ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // for Order Info
    // skip post '>' data
    cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
    cchRange = *(WORD *)ptep->m_pspInfoOut++;
    ptep->m_pspInfoOut += cchwspInfo - 2;
    // now we are at matching </...> of the token
    cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
    cchRange = *(WORD *)ptep->m_pspInfoOut++;
    if (cchwspInfo == 2) // we didn't save any spacing info
    {
        // here is a little story. If we didn't save any spacing information for this end
        // tag, that means we didn't have any white-space before it. Lets go back from
        // pwNew[ichNewCur-1] and remove the white-space.
        // NOTE - Ideally, this needs to get folded into FRestoreSpacing, but 
        // FRestorespacing gets called from other places too so this is late in
        // the game to that kind of change.
        // we know that pwNew[ichNewCur-1] is '/' & pwNew[ichNewCur-2] is '<'
        if ((int)(ichNewCur-2) >= 0 && pwNew[ichNewCur-1] == '/' && pwNew[ichNewCur-2] == '<')
        {
            ichNewCur = ichNewCur - 3;
            while (    (ichNewCur >= 0)
                    && (   pwNew[ichNewCur] == ' '  || pwNew[ichNewCur] == '\t'
                        || pwNew[ichNewCur] == '\r' || pwNew[ichNewCur] == '\n'
                        )
                    )
            {
                ichNewCur--;
            }
            ichNewCur++; // compensate, ichNewCur points to non-white space characher
            pwNew[ichNewCur++] = '<';
            pwNew[ichNewCur++] = '/';
        }

        iArray = iArraySav + 1;
        goto LRestoreCaseAndRet;
    }
    ptep->FRestoreSpacing(pwNew, pwOld, &ichNewCur, &cchwspInfo, cchRange, ichtoktagStart, /*fLookback*/TRUE, /*index*/3);

    iArray = iArraySav + 1; // go past '</', we have already copied the doc till this point
    
LRestoreCaseAndRet:
    // BUG 15389 - we need to start copying the tokElem as well with proper upper/lower case
    // we should combine this memcpy with the above ones, but I want to keep the
    // code separate
    if (pTokArray[iArray].token.tokClass == tokElem && iDSP != -1)
    {
        // except for </BODY> tag because we need to restore post-end-BODY stuff in fnRestoreFtr()
        if (pTokArray[iArray].token.tok != TokElem_BODY && pTokArray[iArray].token.tok != tokElem)
        {
        cbNeed = (ichNewCur+pTokArray[iArray].token.ibTokMac-pTokArray[iArray].token.ibTokMin)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(pwOld+pTokArray[iArray].token.ibTokMin),
                (pTokArray[iArray].token.ibTokMac-pTokArray[iArray].token.ibTokMin)*sizeof(WCHAR));
        // convert into upper/lower case appropriately to match the opening tag's case
        if (iswupper(pwOld[pTokArray[iDSP].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
        {
            _wcsupr(&pwNew[ichNewCur]);
        }
        else
        {
            _wcslwr(&pwNew[ichNewCur]);
        }
        ichNewCur += (pTokArray[iArray].token.ibTokMac-pTokArray[iArray].token.ibTokMin);

        // set ichBeginCopy & iArray for next run
        ichBeginCopy = pTokArray[iArray].token.ibTokMac;
        iArray++;
        }
    }




LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = (UINT)iArray;

//LRetOnly:
    return;

} /* fnRestoreSpaceEnd() */



void
CTriEditParse::fnSaveTbody(CTriEditParse* /*ptep*/,
          LPWSTR /*pwOld*/, LPWSTR* /*ppwNew*/, UINT* /*pcchNew*/, HGLOBAL* /*phgNew*/, 
          TOKSTRUCT* /*pTokArray*/, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT* /*pichNewCur*/, UINT* /*hBeginCopy*/,
          DWORD /*dwFlags*/)
{
    UINT iArray = *piArrayStart;

    ASSERT(pTokArray[iArray].token.tok == TokElem_TBODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    iArray++;

    *piArrayStart = iArray;
    return;

} /* fnSaveTbody() */

void
CTriEditParse::fnRestoreTbody(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{
    // see if we have DESIGNTIMESP as an attribute for <TBODY>. If we do, ignore this one because
    // we know it existed before going to trident. Else, remove this one because trident inserted 
    // it.
    // NOTE - If Trident inserted it, we also have to remove the matching </TBODY>

    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    INT iArray = (INT)*piArrayStart;
    LPWSTR pwNew = *ppwNew;
    INT ichTBodyStart, ichTBodyEnd;
    BOOL fFoundDSP = FALSE;
    INT iArraySav = iArray;
    INT cchtag;
    LPCWSTR rgSpaceTags[] =
    {
        L"DESIGNTIMESP",
    };
    BOOL fBeginTBody = FALSE;
    UINT cbNeed;

    ichTBodyStart = pTokArray[iArray].token.ibTokMin; // init
    ASSERT(pTokArray[iArray].token.tok == TokElem_TBODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    // look for '<' or '</' before TBODY
    while (iArray >= 0) // generally, it will be the previous token, but just in case...
    {
        if (   (pTokArray[iArray].token.tok == TokTag_START)
            && (pTokArray[iArray].token.tokClass == tokTag)
            )
        {
            fBeginTBody = TRUE;
            ichTBodyStart = pTokArray[iArray].token.ibTokMin;
            break;
        }
        else if (      (pTokArray[iArray].token.tok == TokTag_END)
                    && (pTokArray[iArray].token.tokClass == tokTag)
                    )
        {
            if (ptep->m_iTBodyMax > 0) // we have atleast one saved <TBODY>
            {
                ASSERT(ptep->m_pTBodyStack != NULL);
                if (ptep->m_pTBodyStack[ptep->m_iTBodyMax-1] == (UINT)iArraySav) // this was the matching </TBODY>
                {
                    // we want to remove it
                    ichTBodyStart = pTokArray[iArray].token.ibTokMin;
                    break;
                }
                else // this one doesn't match with the saved one, so quit
                {
                    iArray = iArraySav + 1;
                    goto LRet;
                }
            }
            else // we don't have any saved <TBODY>, so quit
            {
                iArray = iArraySav + 1;
                goto LRet;
            }
        }
        iArray--;
    } // while ()
    if (iArray < 0) // this can happen only if we have incomplete HTML. Handle error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    ichTBodyEnd = pTokArray[iArraySav].token.ibTokMac; // init
    iArray = iArraySav;
    ASSERT(pTokArray[iArray].token.tok == TokElem_TBODY);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    cchtag = wcslen(rgSpaceTags[0]);
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (   (pTokArray[iArray].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[iArray].token.tokClass == tokTag)
            )
        {
            ichTBodyEnd = pTokArray[iArray].token.ibTokMac;
            break;
        }
        // look for DESIGNTIMESP
        if (   (pTokArray[iArray].token.tok == 0)
            && (pTokArray[iArray].token.tokClass == tokSpace)
            && (cchtag == (INT)(pTokArray[iArray].token.ibTokMac - pTokArray[iArray].token.ibTokMin))
            && (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[iArray].token.ibTokMin], cchtag))
            )
        {
            fFoundDSP = TRUE; 
            break;
        }
        else if (pTokArray[iArray].token.tokClass == tokAttr)
        {
            // look for any attribute before '>'
            // Even if Trident inserted this <TBODY>, the user may have set some TBODY properties
            // If thats the case, we don't want to remove this <TBODY>
            fFoundDSP = TRUE; // fake it to be fFoundDSP
            break;
        }

        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    if (fFoundDSP)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // we found '>', but didn't find DESIGNTIMESP
    // At this point we are sure that this was added by trident
    ASSERT(iArray < (int)ptep->m_cMaxToken);
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    ASSERT(!fFoundDSP);

    if (fBeginTBody)
    {
        // copy till ichTBodyStart, skip from ichTBodyStart till ichTBodyEnd, set ichBeginCopy accordingly
        // get the iArray for the matching </TBODY> and save it on stack
        
        if (ptep->m_pTBodyStack == NULL) // first time, so allocate it
        {
            ASSERT(ptep->m_hgTBodyStack == NULL);
            ptep->m_hgTBodyStack = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cTBodyInit*sizeof(UINT));
            if (ptep->m_hgTBodyStack == NULL)
            {
                // not enough memory, so lets keep all <TBODY> elements
                goto LRet;
            }
            ptep->m_pTBodyStack = (UINT *)GlobalLock(ptep->m_hgTBodyStack);
            ASSERT(ptep->m_pTBodyStack != NULL);
            ptep->m_iMaxTBody = cTBodyInit;
            ptep->m_iTBodyMax = 0;
        }
        else
        {
            ASSERT(ptep->m_hgTBodyStack != NULL);
            // see if we need to realloc it
            if (ptep->m_iTBodyMax+1 >= ptep->m_iMaxTBody)
            {
                HRESULT hrRet;

                hrRet = ReallocBuffer(  &ptep->m_hgTBodyStack,
                                        (ptep->m_iMaxTBody+cTBodyInit)*sizeof(UINT),
                                        GMEM_MOVEABLE|GMEM_ZEROINIT);
                if (hrRet == E_OUTOFMEMORY)
                    goto LRet;
                ptep->m_iMaxTBody += cTBodyInit;
                ptep->m_pTBodyStack = (UINT *)GlobalLock(ptep->m_hgTBodyStack);
                ASSERT(ptep->m_pTBodyStack != NULL);
            }
        }
        if (pTokArray[iArraySav].iNextprev != -1) // handle error case
        {
            ptep->m_pTBodyStack[ptep->m_iTBodyMax] = pTokArray[iArraySav].iNextprev;
            ptep->m_iTBodyMax++;
        }
        else
        {
            // don't delete this <TBODY> and its matching </TBODY>
            goto LRet;
        }
    }
    else
    {
        // if this was a matching </TBODY> for the one trident inserted, we don't copy it to pwNew
        ASSERT(ptep->m_iTBodyMax > 0);
        // look in ptep->m_pTBodyStack and see if you find this iArray
        ASSERT(ptep->m_pTBodyStack[ptep->m_iTBodyMax-1] == (UINT)iArraySav);
        // assume that we never can have tangled TBODY's
        ptep->m_pTBodyStack[ptep->m_iTBodyMax-1] = 0;
        ptep->m_iTBodyMax--;
    }
    // now do the actual skipping
    cbNeed = (ichNewCur+ichTBodyStart-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;
    if ((INT)(ichTBodyStart-ichBeginCopy) > 0)
    {
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[ichBeginCopy]),
                (ichTBodyStart-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (ichTBodyStart-ichBeginCopy);
    }
    ichBeginCopy = ichTBodyEnd;
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    iArray++; // iArray was at '>' of TBODY, so set it to be the next one. 


LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnRestoreTbody() */

void
CTriEditParse::fnSaveApplet(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT iArraySav = iArray;
    int indexAppletEnd, ichAppletEnd, indexAppletTagClose, index;
    UINT cbNeed;
    int cchURL = 0;
    int ichURL = 0;
    LPCWSTR rgDspURL[] = 
    {
        L" DESIGNTIMEURL=",
        L" DESIGNTIMEURL2=",
    };

    indexAppletTagClose = -1;
    ASSERT(pTokArray[iArray].token.tok == TokElem_APPLET);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);

    // get the ending '>' of the </applet>
    indexAppletEnd = pTokArray[iArraySav].iNextprev;
    if (indexAppletEnd == -1) // error case, we don't have matching </applet> tag
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    // validity check - the matching tag is not '</applet>'
    if (indexAppletEnd-1 >= 0 && pTokArray[indexAppletEnd-1].token.tok != TokTag_END)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    // get ending '>' of the <applet ...>
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (   (pTokArray[iArray].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[iArray].token.tokClass == tokTag)
            )
        {
            indexAppletTagClose = iArray;
            break;
        }
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // invalid case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    iArray = indexAppletEnd;
    while (iArray < (int)ptep->m_cMaxToken) // generally, it will be the next token, but just in case...
    {
        if (   (pTokArray[iArray].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[iArray].token.tokClass == tokTag)
            )
        {
            break;
        }
        iArray++;
    }
    indexAppletEnd = iArray;
    ichAppletEnd = pTokArray[indexAppletEnd].token.ibTokMac;

    // step 1 - if the applet needs special URL processing, act on it.
    if (!FURLNeedSpecialHandling(pTokArray, iArraySav, pwOld, (int)ptep->m_cMaxToken, &ichURL, &cchURL))
        goto LStep2;
    else // save the URL as an attribute value of DESIGNTIMEURL
    {
        // make sure we have enough space in pwNew.
        // copy from ichBeginCopy till current token's ending '>'.
        // index points to APPLET
        index = indexAppletTagClose;
        cbNeed = (ichNewCur+pTokArray[index].token.ibTokMin-ichBeginCopy+wcslen(rgDspURL[0])+cchURL+3/*eq,quotes*/)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
        // index points to '>'
        if ((int) (pTokArray[index].token.ibTokMin-ichBeginCopy) > 0)
        {
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichBeginCopy], 
                    (pTokArray[index].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[index].token.ibTokMin-ichBeginCopy);
        }

        if (cchURL != 0)
        {
            // add 'DESIGNTIMEURL=' followed by the current URL as quoted value
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)rgDspURL[0], 
                    wcslen(rgDspURL[0])*sizeof(WCHAR));
            ichNewCur += wcslen(rgDspURL[0]);

            pwNew[ichNewCur++] = '"';
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichURL], 
                    cchURL*sizeof(WCHAR));
            ichNewCur += cchURL;
            pwNew[ichNewCur++] = '"';
        }

        if (dwFlags & dwPreserveSourceCode)
            ptep->SaveSpacingSpecial(ptep, pwOld, &pwNew, phgNew, pTokArray, iArraySav-1, &ichNewCur);

        // add ending '>' and set ichBeginCopy, iArray, ichNewCur appropriately
        memcpy( (BYTE *)&pwNew[ichNewCur], 
                (BYTE *)&pwOld[pTokArray[index].token.ibTokMin], 
                (pTokArray[index].token.ibTokMac-pTokArray[index].token.ibTokMin)*sizeof(WCHAR));
        ichNewCur += (pTokArray[index].token.ibTokMac-pTokArray[index].token.ibTokMin);

        iArray = index+1; // redundant, but makes code more understandable
        ichBeginCopy = pTokArray[index].token.ibTokMac;
    }

    // step2 - copy the applet
LStep2:
    cbNeed = (ichNewCur+ichAppletEnd-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;

    memcpy( (BYTE *)(&pwNew[ichNewCur]),
            (BYTE *)(&pwOld[ichBeginCopy]),
            (ichAppletEnd-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += (ichAppletEnd-ichBeginCopy);
    ichBeginCopy = ichAppletEnd;
    iArray = indexAppletEnd;

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnSaveApplet() */


void
CTriEditParse::fnRestoreApplet(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT iArraySav = iArray;
    LPCWSTR rgSpaceTags[] =
    {
        L"DESIGNTIMESP",
        L"DESIGNTIMEURL",
    };
    int indexAppletStart, ichAppletStart, indexAppletEnd, i, indexAppletTagClose;
    UINT cchtag, cbNeed, cchURL;
    int indexDSU = -1; // init
    int indexDSUEnd = -1; // init
    int indexDSP = -1; // init
    int indexDSPEnd = -1; // init
    int indexCB = -1; // init (CODEBASE index)
    int indexCBEnd = -1; // init (CODEBASE index)
    BOOL fCodeBaseFound = FALSE;

    ASSERT(pTokArray[iArray].token.tok == TokElem_APPLET);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    indexAppletTagClose = iArraySav;
    // get the matching </applet> tag
    indexAppletEnd = pTokArray[iArraySav].iNextprev;
    if (indexAppletEnd == -1) // error case, we don't have matching </applet> tag
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // get ending '>' of the <applet ...>
    i = iArraySav;
    while (i < (int)ptep->m_cMaxToken)
    {
        if (   (pTokArray[i].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[i].token.tokClass == tokTag)
            )
        {
            indexAppletTagClose = i;
            break;
        }
        i++;
    }
    if (i >= (int)ptep->m_cMaxToken) // invalid case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // look for DESIGNTIMESP & DESIGNTIMEURL inside the <applet> tag
    cchtag = wcslen(rgSpaceTags[0]);
    cchURL = wcslen(rgSpaceTags[1]);
    for (i = iArraySav; i < indexAppletTagClose; i++)
    {
        if (       pTokArray[i].token.tok == 0
                && pTokArray[i].token.tokClass == tokSpace
                && cchtag == pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin
                && (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[i].token.ibTokMin], cchtag))
                )
        {
            indexDSP = i;
        }
        else if (      pTokArray[i].token.tok == 0
                    && pTokArray[i].token.tokClass == tokSpace
                    && cchURL == pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin
                    && (0 == _wcsnicmp(rgSpaceTags[1], &pwOld[pTokArray[i].token.ibTokMin], cchURL))
                    )
        {
            indexDSU = i;
        }
        else if (      pTokArray[i].token.tok == TokAttrib_CODEBASE
                    && pTokArray[i].token.tokClass == tokAttr
                    )
        {
            indexCB = i;
        }
    } // for ()

    // look for '<' before APPLET
    i = iArraySav;
    while (i >= 0) // generally, it will be the previous token, but just in case...
    {
        if (   (pTokArray[i].token.tok == TokTag_START)
            && (pTokArray[i].token.tokClass == tokTag)
            )
        {
            break;
        }
        i--;
    } // while ()
    if (i < 0) // this can happen only if we have incomplete HTML. Handle error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    indexAppletStart = i;
    ichAppletStart = pTokArray[indexAppletStart].token.ibTokMin;

    // look for '>' of </applet>
    i = indexAppletEnd;
    while (i < (int)ptep->m_cMaxToken) // generally, it will be the next token, but just in case...
    {
        if (   (pTokArray[i].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[i].token.tokClass == tokTag)
            )
        {
            break;
        }
        i++;
    }
    if (i >= (int)ptep->m_cMaxToken) // this can happen only if we have incomplete HTML. Handle error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    indexAppletEnd = i;

    // step 1 - copy till indexAppletStart
    cbNeed = (ichNewCur+ichAppletStart-ichBeginCopy+3*(indexAppletEnd-indexAppletStart))*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;

    memcpy( (BYTE *)(&pwNew[ichNewCur]),
            (BYTE *)(&pwOld[ichBeginCopy]),
            (ichAppletStart-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += (ichAppletStart-ichBeginCopy);

    // step 2 - if (indexDSU != -1), we need to go and restore the CODEBASE atrtribute
    // if (indexDSU == -1), we need to remove CODEBASE attribute
    ASSERT(indexAppletTagClose != -1);

    // get indexDSUEnd
    if (indexDSU != -1)
    {
        i = indexDSU;
        while (i < indexAppletTagClose)
        {
            if (   pTokArray[i].token.tok == 0 
                && (pTokArray[i].token.tokClass == tokValue || pTokArray[i].token.tokClass == tokString)
                )
            {
                indexDSUEnd = i;
                break;
            }
            i++;
        }
        if (indexDSUEnd == -1) // we have malformed html
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
    } /* if (indexDSU != -1)*/
    
    // get indexDSPEnd
    if (indexDSP != -1)
    {
        i = indexDSP;
        indexDSPEnd = -1;
        while (i < indexAppletTagClose)
        {
            if (   pTokArray[i].token.tok == 0 
                && (pTokArray[i].token.tokClass == tokValue || pTokArray[i].token.tokClass == tokString)
                )
            {
                indexDSPEnd = i;
                break;
            }
            i++;
        }
        if (indexDSPEnd == -1) // we have malformed html
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
    } /* if (indexDSP != -1) */

    // get indexCBEnd
    if (indexCB != -1)
    {
        i = indexCB;
        while (i < indexAppletTagClose)
        {
            if (   pTokArray[i].token.tok == 0 
                && (pTokArray[i].token.tokClass == tokValue || pTokArray[i].token.tokClass == tokString)
                )
            {
                indexCBEnd = i;
                break;
            }
            i++;
        }
        if (indexCBEnd == -1) // we have malformed html
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
    } /* if (indexCB != -1) */

    // if we didn't find DESIGNTIMEURL attribute, that means CODEBASE attribute
    // should be removed because it didn't exist in source view
    i = indexAppletStart;
    while (i <= indexAppletTagClose)
    {
        if (   (indexDSU != -1)
            && (i >= indexDSU && i <= indexDSUEnd)
            )
        {
            i++; // don't copy this token
        }
        else if (      (indexDSP != -1)
                    && (i >= indexDSP && i <= indexDSPEnd)
                    )
        {
            i++; // don't copy this token
        }
        else if (      pTokArray[i].token.tok == TokAttrib_CODEBASE
                    && pTokArray[i].token.tokClass == tokAttr
                    && !fCodeBaseFound
                    )
        {
            if (indexDSU == -1) // DESIGNTIMEURL not found, so skip CODEBASE
            {
                ASSERT(i == indexCB);
                i = indexCBEnd+1;
            }
            else
            {
                fCodeBaseFound = TRUE;
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                i++;
            }
        }
        else if (      pTokArray[i].token.tok == 0 
                    && pTokArray[i].token.tokClass == tokString || pTokArray[i].token.tokClass == tokValue
                    && fCodeBaseFound
                    )
        {
            int ichURL, ichURLEnd, ichDSURL, ichDSURLEnd;
            // if the url is now absloute and is just an absolute version of 
            // the one at indexDSUEnd, we need to replace it.
            ichURL = (pwOld[pTokArray[i].token.ibTokMin] == '"')
                    ? pTokArray[i].token.ibTokMin+1
                    : pTokArray[i].token.ibTokMin;
            ichURLEnd = (pwOld[pTokArray[i].token.ibTokMac-1] == '"')
                    ? pTokArray[i].token.ibTokMac-1
                    : pTokArray[i].token.ibTokMac;
            if (FIsAbsURL((LPOLESTR)&pwOld[ichURL]))
            {
                WCHAR *pszURL1 = NULL;
                WCHAR *pszURL2 = NULL;
                int ich;

                ichDSURL = (pwOld[pTokArray[indexDSUEnd].token.ibTokMin] == '"')
                        ? pTokArray[indexDSUEnd].token.ibTokMin+1
                        : pTokArray[indexDSUEnd].token.ibTokMin;
                ichDSURLEnd = (pwOld[pTokArray[indexDSUEnd].token.ibTokMac-1] == '"')
                        ? pTokArray[indexDSUEnd].token.ibTokMac-1
                        : pTokArray[indexDSUEnd].token.ibTokMac;

                // just for comparison purposes, don't look at '/' or '\' separators
                // between filenames & directories...
                pszURL1 = new WCHAR[ichDSURLEnd-ichDSURL + 1];
                pszURL2 = new WCHAR[ichDSURLEnd-ichDSURL + 1];
                if (pszURL1 == NULL || pszURL2 == NULL)
                    goto LResumeCopy;
                memcpy((BYTE *)pszURL1, (BYTE *)&pwOld[ichDSURL], (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                memcpy((BYTE *)pszURL2, (BYTE *)&pwOld[ichURLEnd-(ichDSURLEnd-ichDSURL)], (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                pszURL1[ichDSURLEnd-ichDSURL] = '\0';
                pszURL2[ichDSURLEnd-ichDSURL] = '\0';
                for (ich = 0; ich < ichDSURLEnd-ichDSURL; ich++)
                {
                    if (pszURL1[ich] == '/')
                        pszURL1[ich] = '\\';
                    if (pszURL2[ich] == '/')
                        pszURL2[ich] = '\\';
                }

                if (0 == _wcsnicmp(pszURL1, pszURL2, ichDSURLEnd-ichDSURL))
                {
                    pwNew[ichNewCur++] = '"';
                    memcpy( (BYTE *)&pwNew[ichNewCur],
                            (BYTE *)&pwOld[ichDSURL],
                            (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                    ichNewCur += (ichDSURLEnd-ichDSURL);
                    pwNew[ichNewCur++] = '"';
                }
                else // copy it as it is
                {
LResumeCopy:
                    memcpy( (BYTE *)&pwNew[ichNewCur],
                            (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
                if (pszURL1 != NULL)
                    delete pszURL1;
                if (pszURL2 != NULL)
                    delete pszURL2;
            }
            else // its realtive, simply copy it
            {
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
            }
            i++;
        }
        else // all other tokens
        {
            // ****NOTE - we can actually do pretty printing here 
            // instead of fixing the special cases****

            // fix Trident's behaviour - If Trident sees unknown tag(s) it puts it(them) at the end 
            // and inserts EOL before those. In this case, we would have inserted a space before DESIGNTIMESP
            // and Trident would have inserted EOL. If thats not the case, we will ignore it.
            if (   (pTokArray[i].token.tokClass == tokSpace)
                && (pTokArray[i].token.tok == 0)
                && (FIsWhiteSpaceToken(pwOld, pTokArray[i].token.ibTokMin, pTokArray[i].token.ibTokMac))
                )
            {
                if (i != indexDSU-1) // else skip the copy
                    pwNew[ichNewCur++] = ' '; // convert space+\r+\n into space
                i++;
            }
            else
            {
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                i++;
            }
        }
    } // while ()

    // we have spacing save dfor this tag, lets restore it
    if (   (indexDSP != -1)
        && (dwFlags & dwPreserveSourceCode)
        ) 
        ptep->RestoreSpacingSpecial(ptep, pwOld, &pwNew, phgNew, pTokArray, indexDSP, &ichNewCur);

    // step 3 - format all stuff between <applet> ... </applet>
    pwNew[ichNewCur] = '\r';
    ichNewCur++;
    pwNew[ichNewCur] = '\n';
    ichNewCur++;
    pwNew[ichNewCur] = '\t'; // replace this with appropriate alignment
    ichNewCur++;
    for (i = indexAppletTagClose+1; i <= indexAppletEnd; i++)
    {
        // copy the tag
        memcpy( (BYTE *)(&pwNew[ichNewCur]),
                (BYTE *)(&pwOld[pTokArray[i].token.ibTokMin]),
                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
        ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
        // if its was TokTag_CLOSE, insert EOL
        if (   pTokArray[i].token.tok == TokTag_CLOSE
            && pTokArray[i].token.tokClass == tokTag)
        {
            pwNew[ichNewCur] = '\r';
            ichNewCur++;
            pwNew[ichNewCur] = '\n';
            ichNewCur++;
            pwNew[ichNewCur] = '\t'; // replace this with appropriate alignment
            ichNewCur++;
        }
    } // for ()

    // remember to set iArray appropriately
    iArray = indexAppletEnd + 1;
    ichBeginCopy = pTokArray[indexAppletEnd].token.ibTokMac;

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnRestoreApplet() */

void
CTriEditParse::RestoreSpacingSpecial(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR *ppwNew, HGLOBAL* /*phgNew*/,
            TOKSTRUCT *pTokArray, UINT iArray, UINT *pichNewCur)
{
    LPWSTR pwNew = *ppwNew;
    UINT ichNewCur = *pichNewCur;
    UINT iArraySav = iArray;
    UINT ichspInfoEndtagEnd, ichBegin;
    WCHAR szIndex[cchspBlockMax]; // will we have more than 20 digit numbers as number of DESIGNTIMESPx?
    WORD *pspInfoEnd, *pspInfoOrder;
    INT cwOrderInfo = 0;
    UINT ichNewCurSav = 0xFFFFFFFF;
    INT cchwspInfo; // spInfo block size in wide chars
    INT cchRange; // number of char for which this spInfo was saved
    INT ichtoktagStart, iArrayValue, index, itoktagStart;
    int ichNewCurAtIndex0 = -1; // we need to adjust the saved ichNewCur because it gets invalidated
                                // as soon as the tag moves as a result of restoring pre-tag spaces.

    itoktagStart = iArray; // init
    // found DESIGNTIMESPx. Now, go backwords and look for TokTag_START
    while (itoktagStart >= 0)
    {
        if (       pTokArray[itoktagStart].token.tok == TokTag_START
                && pTokArray[itoktagStart].token.tokClass == tokTag
                )
        {
            break;
        }
        itoktagStart--;
    }
    if (itoktagStart < 0) // didn't find '<' before DESIGNTIMESPx
        goto LRet;

    // found '<' before DESIGNTIMESPx
    // the spacing info saved was for the portion of the document before the '<'
    ASSERT(pTokArray[itoktagStart].token.tok == TokTag_START);
    ASSERT(pTokArray[itoktagStart].token.tokClass == tokTag);
    // we already know that iArray'th token is DESIGNTIMESPx, so get past the '=' that follows it
    // ASSUMPTION - the value of attribute DESIGNTIMESPx will NOT get munged by Trident.
    // NOTE - the above assumption is correct for this release of Trident
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '=')
        {
            ASSERT(pTokArray[iArray].token.tokClass == tokOp);
            break;
        }
        else if (*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '>') // gone too far
            goto LSkip1;
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // didn't find = after DESIGNTIMESPx
    {
LSkip1:
        goto LRet;
    }
    iArrayValue = -1; // init
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (   (iArrayValue == -1)
            && (   (pTokArray[iArray].token.tokClass == tokValue)
                || (pTokArray[iArray].token.tokClass == tokString)
                )
            )
            iArrayValue = iArray;
        else if (      pTokArray[iArray].token.tok == TokTag_CLOSE
                    && pTokArray[iArray].token.tokClass == tokTag
                    )
        {
            ASSERT(*(WORD *)(pwOld+pTokArray[iArray].token.ibTokMin) == '>');
            break;
        }
        iArray++;
    }
    if (iArrayValue == -1 || iArray >= (int)ptep->m_cMaxToken) // didn't find tokValue after DESIGNTIMESPx
    {
        // BUG 9040
        //if (iArray >= (int)ptep->m_cMaxToken && iArrayValue != -1)
        //{
            // SOLUTION 1
            // overwrite the stuff from pwOld[pTokArray[iArraySav].token.ibTokMin]
            // to pwOld[pTokArray[iArrayValue].token.ibTokMac - 1]
            // SOLUTION 2
            // look for DESIGNTIMESP from pwOld[pTokArray[itokTagStart].token.ibTokMac - 1]
            // to pwOld[pTokArray[iArray].token.ibTokMac - 1] and overwrite all of those 
            // strings with spaces. We could NULL those and do the blts, but why bother
            // when the html isn't valid! 

            // make sure that all DESIGNTIMESPs are stripped off if we encountered this error case
        //}
        goto LRet;
    }

    // we know that 4 blocks of info was saved for each DESIGNTIMESPx attribute
    // before tag, within tag, after tag, before matching end-tag
    // even if no info was saved, the block will still exist with 2 words (size,# of char)
    ichspInfoEndtagEnd = pTokArray[iArray].token.ibTokMac;

    // first copy the document till DESIGNTIMESPx
    // skip DESIGNTIMESPx and its value and set ichBeginCopy to be after that

    // NOTE - token before iArraySav'th one should be tokSpace with lenght 1 
    // and with a value of chSpace (unless Trident has modified it). If thats TRUE,
    // we should skip that too, because we added it when we put in DESIGNTIMESPx.
    
    // fix Trident's behaviour - If Trident sees unknown tag(s) it puts it(them) at the end 
    // and inserts EOL before those. In this case, we would have inserted a space before DESIGNTIMESP
    // and Trident would have inserted EOL. If thats not the case, we will ignore it.
    if (   (iArraySav-1 > 0) /* validation */
        && (    (      (pTokArray[iArraySav-1].token.ibTokMac - pTokArray[iArraySav-1].token.ibTokMin == 1)
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin] == ' ')
                    )
            ||  (      (pTokArray[iArraySav-1].token.ibTokMac - pTokArray[iArraySav-1].token.ibTokMin == 3)
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin] == ' ')
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin+1] == '\r')
                    && (pwOld[pTokArray[iArraySav-1].token.ibTokMin+2] == '\n')
                    )
                )
        )
    {
        ichBegin = pTokArray[iArraySav-1].token.ibTokMin;
    }
    else
        ichBegin = pTokArray[iArraySav].token.ibTokMin;

#ifdef NEEDED
    ASSERT(ichBegin >= ichBeginCopy);

    cbNeed = (ichNewCur+(ichBegin-ichBeginCopy))*sizeof(WCHAR) + cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
    {
        iArray = iArraySav + 1;
        goto LRet;
    }
    // BUG 15389 - look at the case of DESIGNTIMESP & convert the tag into upper/lower case...
    //memcpy(   (BYTE *)(pwNew+ichNewCur),
    //      (BYTE *)(pwOld+ichBeginCopy),
    //      (ichBegin-ichBeginCopy)*sizeof(WCHAR));
    //ichNewCur += (ichBegin-ichBeginCopy);
    if (ichBegin-ichBeginCopy >= 0)
    {
        // step 1 - copy from ichBeginCopy to '<' of the current tag
        if ((int)(pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy) > 0)
        {
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+ichBeginCopy),
                    (pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[itoktagStart].token.ibTokMac-ichBeginCopy);
            ichNewCurSav = ichNewCur+1; // used as a peg to get preceding tokTag_START i.e. '<'
        }
        // step 2 - convert current tag into upper/lower case & copy it
        if (ichBeginCopy < pTokArray[itoktagStart+1].token.ibTokMin)
        {
            ASSERT((int)(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin) > 0);
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+pTokArray[itoktagStart+1].token.ibTokMin),
                    (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)*sizeof(WCHAR));
            if (iswupper(pwOld[pTokArray[iArraySav].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
            {
                // convert the tag into upper case. ASSUME that the tag is at itoktagStart+1
                _wcsupr(&pwNew[ichNewCur]);
            }
            else
            {
                // convert the tag into lower case. ASSUME that the tag is at itoktagStart+1
                _wcslwr(&pwNew[ichNewCur]);
            }
            ichNewCur += (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin);
        }
        else // this tag is alreay been copied
        {
            // hack
            if (pTokArray[itoktagStart+1].token.ibTokMac == ichBeginCopy) // means we are just past the current tag
            {
                if (iswupper(pwOld[pTokArray[iArraySav].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
                {
                    ASSERT(ichNewCur >= (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin));
                    // convert the tag into upper case. ASSUME that the tag is at itoktagStart+1
                    _wcsupr(&pwNew[ichNewCur-(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)]);
                }
                else
                {
                    ASSERT(ichNewCur >= (pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin));
                    // convert the tag into lower case. ASSUME that the tag is at itoktagStart+1
                    _wcslwr(&pwNew[ichNewCur-(pTokArray[itoktagStart+1].token.ibTokMac-pTokArray[itoktagStart+1].token.ibTokMin)]);
                }
            }
        }
        // step 3 - copy from after the tag (which is at ichtoktagStart+1) to ichBegin
        if ((int)(ichBegin-pTokArray[itoktagStart+1].token.ibTokMac) > 0)
        {
            memcpy( (BYTE *)(pwNew+ichNewCur),
                    (BYTE *)(pwOld+pTokArray[itoktagStart+1].token.ibTokMac),
                    (ichBegin-pTokArray[itoktagStart+1].token.ibTokMac)*sizeof(WCHAR));
            ichNewCur += (ichBegin-pTokArray[itoktagStart+1].token.ibTokMac);
        }
    }
    // set ichBeginCopy
    ichBeginCopy = ichspInfoEndtagEnd; // make it ready for next copy

    // copy the rest of the tag (skipping DESIGNTIMESPx = value)
    ASSERT((INT)(ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac) >= 0);
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMac),
            (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac)*sizeof(WCHAR));
    ichNewCur += (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac);
#endif //NEEDED 

    memset((BYTE *)szIndex, 0, sizeof(szIndex));
    // check if the value has quotes around it and don't copy them to szIndex
    if (   pwOld[pTokArray[iArrayValue].token.ibTokMin] == '"'
        && pwOld[pTokArray[iArrayValue].token.ibTokMac-1] == '"'
        )
    {
        memcpy( (BYTE *)szIndex,
                (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin+1),
                (pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin-2)*sizeof(WCHAR));
    }
    else
    {
        memcpy( (BYTE *)szIndex,
                (BYTE *)(pwOld+pTokArray[iArrayValue].token.ibTokMin),
                (pTokArray[iArrayValue].token.ibTokMac-pTokArray[iArrayValue].token.ibTokMin)*sizeof(WCHAR));
    }
    ptep->m_ispInfoBlock = _wtoi(szIndex);
    ptep->m_ispInfoBlock -= ptep->m_ispInfoBase;
    if (ptep->m_ispInfoBlock < 0)
        goto LRet;

    // NOTE - we can cache this info in a link list at the begining
    // get to the ptep->m_ispInfoBlock'th block from ptep->m_pspInfoOutStart
    ASSERT(ptep->m_cchspInfoTotal >= 0);
    pspInfoEnd = ptep->m_pspInfoOutStart + ptep->m_cchspInfoTotal;
    ptep->m_pspInfoOut = ptep->m_pspInfoOutStart;
    for (index = 0; index < ptep->m_ispInfoBlock; index++)
    {
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before <
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // between <>
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // Order Info
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // after >
        ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // before matching </

        // we somehow have gone beyond the data that was saved for spacing
        if (ptep->m_pspInfoOut >= pspInfoEnd)
        {
            goto LRet;
        }
    }

    // get the Order Info
    pspInfoOrder = ptep->m_pspInfoOut;
    pspInfoOrder += *(WORD *)pspInfoOrder; // skip info saved for spacing before '<'
    pspInfoOrder += *(WORD *)pspInfoOrder; // skip info saved for spacing between '<>'
    // now pspInfoOrder is at correct place
    cwOrderInfo = *(WORD *)pspInfoOrder++;
    ASSERT(cwOrderInfo >= 1);
    // process this info
#ifdef NEEDED
    if (cwOrderInfo > 1) // means that we saved some info
    {
        INT cchNewCopy;

        cchNewCopy = (ichBegin-pTokArray[itoktagStart].token.ibTokMin) + (ichspInfoEndtagEnd-pTokArray[iArrayValue].token.ibTokMac);
        ptep->FRestoreOrder(pwNew, pwOld, pspInfoOrder, &ichNewCur, cwOrderInfo, pTokArray, itoktagStart, iArray, iArraySav, iArrayValue, cchNewCopy, phgNew);
    }
#endif //NEEDED
    ichtoktagStart = ichNewCur; // init
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    for (index = 0; index < 4; index++)
    {
        BOOL fLookback = FALSE;

        cchwspInfo = *(WORD *)ptep->m_pspInfoOut++;
        cchRange = *(WORD *)ptep->m_pspInfoOut++;
        if (cchwspInfo == 2) // we didn't save any spacing info
        {
            if (index == 0) // special case BUG 8741
            {
                // Note that we didn't save anything before this tag. which means that
                // we had '>' or some text immediately before the < tag. 
                ichtoktagStart = ichNewCur;
                while (ichtoktagStart >= 0)
                {
                    if (pwNew[ichtoktagStart] == '<')
                    {
                        ichtoktagStart--;
                        break;
                    }
                    ichtoktagStart--;
                }
                if (ichtoktagStart >= 0)
                {
                    int cws = 0;
                    int ichtagStart = ichtoktagStart;

                    // remove any such white space trident inserts.
                    while (    pwNew[ichtoktagStart] == ' '
                            || pwNew[ichtoktagStart] == '\r'
                            || pwNew[ichtoktagStart] == '\n'
                            || pwNew[ichtoktagStart] == '\t')
                    {
                        cws++;
                        ichtoktagStart--;
                    }
                    if (cws > 0)
                    {
                        ASSERT((int)(ichNewCur-ichtagStart-1) >= 0);
                        //ichtokTagStart now points to either '>' or a non-whitespace char
                        memmove((BYTE*)&pwNew[ichtoktagStart+1],
                                (BYTE*)&pwNew[ichtoktagStart+1+cws],
                                (ichNewCur-ichtagStart-1)*sizeof(WCHAR));
                        ichNewCur -= cws;
                    }
                } // if (ichtoktagStart >= 0)
            } // if (index == 0)
            goto LNext;
        }

        // note that ichtoktagStart is a position in pwNew
        switch (index)
        {
        case 0: // before < of the tag
            fLookback = TRUE;
            ichtoktagStart = (ichNewCurSav == -1)?ichNewCur:ichNewCurSav;// handle < ... <%..%>...> case correctly
            ichNewCurAtIndex0 = ichNewCur; // lets save the ichNewCur before we restore pre-tag spacing
            while (ichtoktagStart >= 0)
            {
                if (pwNew[ichtoktagStart] == '<' && pwNew[ichtoktagStart+1] != '%')
                {
                    ichtoktagStart--;
                    break;
                }
                ichtoktagStart--;
            }
            if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
            {
                ptep->m_pspInfoOut += cchwspInfo-2;
                continue;
            }
            break;
        case 1: // between <> of the tag
            fLookback = FALSE;
            // NOTE - we can assume that in 'case 0' we had put ichtoktagStart is just before '<'
            // so that we can avoid this while loop. but what if we skipped case '0'?

            // adjust ichNewCurSav to reflect the pre-tag spacing so that it doesn't become invalid
            // we may need to adjust it in ichNewCur-ichNewCurAtIndex0 < 0 case as well, but lets not
            // add code at this stage that we don't have to. (4/30/98)
            if (ichNewCurAtIndex0 != -1 && ichNewCurSav != -1 && ichNewCur-ichNewCurAtIndex0 > 0)
                ichNewCurSav = ichNewCurSav + (ichNewCur-ichNewCurAtIndex0);
            ichtoktagStart = (ichNewCurSav == -1)?ichNewCur:ichNewCurSav;// handle < ... <%..%>...> case correctly
            while (ichtoktagStart >= 0)
            {
                if (pwNew[ichtoktagStart] == '<' && pwNew[ichtoktagStart+1] != '%')
                {
                    ichtoktagStart++;
                    break;
                }
                ichtoktagStart--;
            }
            if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
            {
                ptep->m_pspInfoOut += cchwspInfo-2; // for spacing info
                ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut; // for Order Info
                continue;
            }
            break;
        case 2: // after > of the tag
            // Observation - Trident messes up the document in following way - 
            //    If we had an EOL after '>' which is followed by HTML text, 
            //    trident eats that EOL
            // BUT
            //    If we had a space/tab before that EOL trident doesn't eat it!!!
            // so I have added the conditions
            // && (pwOld[pTokArray[iArray+1].token.ibTokMin] != ' ')
            // && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\t')

            // here is the deal - If the next tone happens to be plain text, there is no danger
            // of applying the same format twice.( i.e. once for after '>' and the next time for
            // before the next '<')
            if (   (iArray+1 < (INT)ptep->m_cMaxToken) /*validation*/
                && pTokArray[iArray+1].token.tok == 0
                && pTokArray[iArray+1].token.tokClass == tokIDENTIFIER
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\r')
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != ' ')
                && (pwOld[pTokArray[iArray+1].token.ibTokMin] != '\t')
                )
            {
                fLookback = FALSE;
                ichtoktagStart = ichNewCur;
                while (ichtoktagStart >= 0)
                {
                    if (pwNew[ichtoktagStart] == '>')
                    {
                        ichtoktagStart++;
                        break;
                    }
                    ichtoktagStart--;
                }
                if (ichtoktagStart < 0) // looks to be an error, don't try to restore the spacing
                {
                    ptep->m_pspInfoOut += cchwspInfo-2;
                    continue;
                }
            }
            else
            {
                ptep->m_pspInfoOut += cchwspInfo-2; // we ignore this info
                continue;
            }
            break;
        case 3: // before matching end tag
            ptep->m_pspInfoOut += cchwspInfo-2; // we ignore this info
            continue;
            //fLookback = TRUE;
            //ichtoktagStart = 0; // we ignore this info
            break;
        }

        if (index == 3) // skip this info, because we have not reached matching end tag yet
            ptep->m_pspInfoOut += cchwspInfo-2;
        //else if (index == 0)
        //  ptep->FRestoreSpacingInHTML(pwNew, pwOld, &ichNewCur, &cchwspInfo, cchRange, ichtoktagStart, fLookback, index);
        else
            ptep->FRestoreSpacing(pwNew, pwOld, &ichNewCur, &cchwspInfo, cchRange, ichtoktagStart, fLookback, index);

LNext:
        if (index == 1) // we have already processed this info, just move the pointer ahead
            ptep->m_pspInfoOut += *(WORD *)ptep->m_pspInfoOut;

    } // for ()

LRet:
    *ppwNew = pwNew; // in case this changed
    *pichNewCur = ichNewCur;

} /* RestoreSpacingSpecial() */

void
CTriEditParse::SaveSpacingSpecial(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR *ppwNew, HGLOBAL *phgNew,
            TOKSTRUCT *pTokArray, INT iArray, UINT *pichNewCur)
{
    UINT ichNewCur = *pichNewCur;
    LPWSTR pwNew = *ppwNew;
    int iArrayPrevTag, iArrayMatch;
    UINT iArrayElem, iArrayTagStart;
    INT ichEndMatch, ichBeginMatch, ichEndPrev, ichBeginPrev, ichEndNext, ichBeginNext, ichEndTag, ichBeginTag, ichBegin, ichEnd;
    UINT cbNeed;
    WCHAR szIndex[cchspBlockMax]; // will we have more than 20 digit numbers as number of DESIGNTIMESPx?
    LPCWSTR rgSpaceTags[] =
    {
        L" DESIGNTIMESP=",
        L" DESIGNTIMESP1=",
        L" designtimesp=",
    };

    iArrayElem = 0xFFFFFFFF; // init
    //
    // look for TokTag_START
    while (iArray >= 0)
    {
        if (   pTokArray[iArray].token.tokClass == tokTag 
            && pTokArray[iArray].token.tok == TokTag_START
            )
        {
            break;
        }
        iArray--;
    }
    if (iArray < 0) // error case
        goto LRet;
    iArrayTagStart = iArray;
    //

    // step 1
    // look for > that matches with <. we already are at ft.tokBegin2 i.e. <
    ASSERT(pTokArray[iArray].token.tok == TokTag_START);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    ichBeginTag = pTokArray[iArray].token.ibTokMac;
    while (iArray < (int)ptep->m_cMaxToken)
    {
        if (   pTokArray[iArray].token.tok == TokTag_CLOSE 
            && pTokArray[iArray].token.tokClass == tokTag) // ft.tokEnd2 is -1
            break;
        if (pTokArray[iArray].token.tokClass == tokElem)
            iArrayElem = iArray;
        iArray++;
    }
    if (iArray >= (int)ptep->m_cMaxToken) // didn't find >
    {
        goto LRet;
    }
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE); // found >
    ASSERT(pTokArray[iArray].token.tokClass == tokTag); // found >
    ichEndTag = ichBegin = pTokArray[iArray].token.ibTokMin;
    ichEnd = pTokArray[iArray].token.ibTokMac;

    // step 2
    // look for > before iArrayTagStart. Boundary case will be for the first < in the document
    // save the spacing info
    ASSERT(pTokArray[iArrayTagStart].token.tok == TokTag_START);
    ASSERT(pTokArray[iArrayTagStart].token.tokClass == tokTag);
    ichEndPrev = pTokArray[iArrayTagStart].token.ibTokMin;
    ichBeginPrev = ichEndPrev-1;
    // look for previous TokTag_CLOSE
    // if the tag ending tag, ichBeginPrev becomes ibTokMac of '>' tag
    // if the tag is starting tag, ichBeginPrev becomes ibTokMac+(white space just after that tag)
    iArrayPrevTag = iArrayTagStart; // this is TokTag_START
    while (iArrayPrevTag >= 0)
    {
        if (       (   pTokArray[iArrayPrevTag].token.tokClass == tokTag 
                    && pTokArray[iArrayPrevTag].token.tok == TokTag_CLOSE
                    )
                || (   pTokArray[iArrayPrevTag].token.tokClass == tokSSS 
                    && pTokArray[iArrayPrevTag].token.tok == TokTag_SSSCLOSE
                    )/* VID6 - bug 22787 */
                )
        {
            break;
        }
        iArrayPrevTag--;
    }
    if (iArrayPrevTag < 0) // handle error case
    {
        // leave the old behaviour as is for V1
        while (ichBeginPrev >= 0)
        {
            if (   pwOld[ichBeginPrev] != ' '
                && pwOld[ichBeginPrev] != '\r'
                && pwOld[ichBeginPrev] != '\n'
                && pwOld[ichBeginPrev] != '\t'
                )
                break;
            ichBeginPrev--;
        }
        goto LGotEndNext;
    }
    ichBeginPrev = pTokArray[iArrayPrevTag].token.ibTokMac - 1;

LGotEndNext:
    if (ichBeginPrev < 0)
        ichBeginPrev = 0;
    else
        ichBeginPrev++;


    // step 3
    // look for TokTag_START after iArray(which currently is TokTag_CLOSE)
    // save spacing info
    ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    //iArrayNextStart = iArray;
    ichBeginNext = pTokArray[iArray].token.ibTokMac;
    ASSERT(ichBeginNext == ichEnd);
    ichEndNext = ichBeginNext;
    while (ichEndNext < (INT)pTokArray[ptep->m_cMaxToken-1].token.ibTokMac)
    {
        if (   pwOld[ichEndNext] != ' '
            && pwOld[ichEndNext] != '\r'
            && pwOld[ichEndNext] != '\n'
            && pwOld[ichEndNext] != '\t'
            )
            break;
        ichEndNext++;
    }

    if (ichEndNext >= (INT)pTokArray[ptep->m_cMaxToken-1].token.ibTokMac)
        ichEndNext = pTokArray[ptep->m_cMaxToken-1].token.ibTokMac;

    // step 4
    // if iArrayElem != -1, look for pTokArray[iArrayElem].iNextprev. If its not -1, set iArrayMatch
    // look for previous TokTag_START/TokTag_END. look for previous TokTag_CLOSE
    // save spacing info
    if (iArrayElem == -1) // this can happen if we have incomplete HTML
    {
        ichEndMatch = ichBeginMatch = 0;
        goto LSkipMatchCalc;
    }
    iArrayMatch = pTokArray[iArrayElem].iNextprev;
    if (iArrayMatch != -1) // match was set while tokenizing
    {
        ichBeginMatch = ichEndMatch = 0; //init
        ASSERT(pTokArray[iArray].token.tok == TokTag_CLOSE);
        ASSERT(pTokArray[iArray].token.tokClass == tokTag);
        while (iArrayMatch >= iArray) // iArray is TokTag_CLOSE of the current tag (i.e. '>')
        {
            if (   pTokArray[iArrayMatch].token.tokClass == tokTag
                && (   pTokArray[iArrayMatch].token.tok == TokTag_START
                    || pTokArray[iArrayMatch].token.tok == TokTag_END
                    )
                )
                break;
            iArrayMatch--;
        }
        if (iArrayMatch > iArray) // did find '</' or '<' after the current tag
        {
            ichEndMatch = pTokArray[iArrayMatch].token.ibTokMin;
            ichBeginMatch = ichEndMatch; // init
            // look for '>' and set ichBeginMatch
            while (iArrayMatch >= iArray) // iArray is TokTag_CLOSE of the current tag (i.e. '>')
            {
                if (   (   pTokArray[iArrayMatch].token.tokClass == tokTag
                        && pTokArray[iArrayMatch].token.tok == TokTag_CLOSE
                        )
                    || (   pTokArray[iArrayMatch].token.tokClass == tokSSS
                        && pTokArray[iArrayMatch].token.tok == TokTag_SSSCLOSE
                        )/* VID6 - bug 22787 */
                    )
                    break;
                iArrayMatch--;
            }
            if (iArrayMatch >= iArray) // they may very well be the same
            {
                ichBeginMatch = pTokArray[iArrayMatch].token.ibTokMac;
                ASSERT(ichBeginMatch <= ichEndMatch);
                ASSERT(ichBeginMatch >= ichEnd);
            }
        }
    }
    else
    {
        // don't bother saving any info from here
        ichEndMatch = ichBeginMatch = 0;
    }
LSkipMatchCalc:
    if (ichEndPrev > ichBeginPrev)
        ptep->hrMarkSpacing(pwOld, ichEndPrev, &ichBeginPrev);
    else
        ptep->hrMarkSpacing(pwOld, ichEndPrev, &ichEndPrev);

    if (ichEndTag > ichBeginTag)
    {
        INT ichBeginTagSav = ichBeginTag;

        ptep->hrMarkSpacing(pwOld, ichEndTag, &ichBeginTag);
        // iArray'th token is TokTag_CLOSE & iArrayTagStart is TokTag_START
        ptep->hrMarkOrdering(pwOld, pTokArray, iArrayTagStart, iArray, ichEndTag, &ichBeginTagSav);
    }
    else
    {
        INT ichEndTagSav = ichEndTag;

        ptep->hrMarkSpacing(pwOld, ichEndTag, &ichEndTag);
        // iArray'th token is TokTag_CLOSE & iArrayTagStart is TokTag_START
        ptep->hrMarkOrdering(pwOld, pTokArray, iArrayTagStart, iArray, ichEndTagSav, &ichEndTagSav);
    }

    if (ichEndNext > ichBeginNext)
        ptep->hrMarkSpacing(pwOld, ichEndNext, &ichBeginNext);
    else
        ptep->hrMarkSpacing(pwOld, ichEndNext, &ichEndNext);

    if (ichEndMatch > ichBeginMatch)
        ptep->hrMarkSpacing(pwOld, ichEndMatch, &ichBeginMatch);
    else
        ptep->hrMarkSpacing(pwOld, ichEndMatch, &ichEndMatch);

    // realloc if needed
    cbNeed = (ichNewCur+3*wcslen(rgSpaceTags[0])+(ichEnd-ichBegin))*sizeof(WCHAR);
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;

    if (iswupper(pwOld[pTokArray[iArrayTagStart+1].token.ibTokMin]) != 0) // upper case
    {
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(rgSpaceTags[0]),
                (wcslen(rgSpaceTags[0]))*sizeof(WCHAR));
        ichNewCur += wcslen(rgSpaceTags[0]);
    }
    else
    {
        memcpy( (BYTE *)(pwNew+ichNewCur),
                (BYTE *)(rgSpaceTags[2]),
                (wcslen(rgSpaceTags[2]))*sizeof(WCHAR));
        ichNewCur += wcslen(rgSpaceTags[2]);
    }

    (WCHAR)_itow(ptep->m_ispInfoBlock+ptep->m_ispInfoBase, szIndex, 10);
    ptep->m_ispInfoBlock++;

    ASSERT(wcslen(szIndex) < sizeof(szIndex));
    ASSERT(sizeof(szIndex) == cchspBlockMax*sizeof(WCHAR));
    memcpy( (BYTE *)(pwNew+ichNewCur),
            (BYTE *)(szIndex),
            wcslen(szIndex)*sizeof(WCHAR));
    ichNewCur += wcslen(szIndex);



LRet:
    //*pcchNew = ichNewCur;
    *ppwNew = pwNew;
    *pichNewCur = ichNewCur;

    return;
} /* SaveSpacingSpecial() */


void
CTriEditParse::fnSaveAImgLink(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT cbNeed;

    int cchURL = 0;
    int ichURL = 0;
    int index = iArray;
    LPCWSTR rgDspURL[] = 
    {
        L" DESIGNTIMEURL=",
    };

    ASSERT(    pTokArray[iArray].token.tok == TokElem_A
            || pTokArray[iArray].token.tok == TokElem_IMG
            || pTokArray[iArray].token.tok == TokElem_LINK);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);

    if (!FURLNeedSpecialHandling(pTokArray, iArray, pwOld, (int)ptep->m_cMaxToken, &ichURL, &cchURL))
        iArray++;
    else // save the URL as an attribute value of DESIGNTIMEURL
    {
        // make sure we have enough space in pwNew.
        // copy from ichBeginCopy till current token's ending '>'.
        // index points to A/IMG/LINK
        while (index < (int)ptep->m_cMaxToken)
        {
            if (   pTokArray[index].token.tok == TokTag_CLOSE
                && pTokArray[index].token.tokClass == tokTag
                )
                break;
            index++;
        }
        if (index >= (int)ptep->m_cMaxToken) // invalid HTML, we didn't find '>'
        {
            iArray++;
            goto LRet;
        }
        cbNeed = (ichNewCur+pTokArray[index].token.ibTokMin-ichBeginCopy+wcslen(rgDspURL[0])+cchURL+3/*eq,quotes*/)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        {
            iArray++;
            goto LRet;
        }
        // index points to '>'
        if ((int) (pTokArray[index].token.ibTokMin-ichBeginCopy) > 0)
        {
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichBeginCopy], 
                    (pTokArray[index].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[index].token.ibTokMin-ichBeginCopy);
        }

        if (cchURL != 0)
        {
            // add 'DESIGNTIMEURL=' followed by the current URL as quoted value
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)rgDspURL[0], 
                    wcslen(rgDspURL[0])*sizeof(WCHAR));
            ichNewCur += wcslen(rgDspURL[0]);

            pwNew[ichNewCur++] = '"';
            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichURL], 
                    cchURL*sizeof(WCHAR));
            ichNewCur += cchURL;
            pwNew[ichNewCur++] = '"';
        }

        if (dwFlags & dwPreserveSourceCode)
            ptep->SaveSpacingSpecial(ptep, pwOld, &pwNew, phgNew, pTokArray, iArray-1, &ichNewCur);

        // add ending '>' and set ichBeginCopy, iArray, ichNewCur appropriately
        memcpy( (BYTE *)&pwNew[ichNewCur], 
                (BYTE *)&pwOld[pTokArray[index].token.ibTokMin], 
                (pTokArray[index].token.ibTokMac-pTokArray[index].token.ibTokMin)*sizeof(WCHAR));
        ichNewCur += (pTokArray[index].token.ibTokMac-pTokArray[index].token.ibTokMin);

        iArray = index+1;
        ichBeginCopy = pTokArray[index].token.ibTokMac;
    }

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;
    return;

} /* fnSaveAImgLink() */

void
CTriEditParse::fnRestoreAImgLink(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD dwFlags)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT iArraySav = iArray;
    LPCWSTR rgTags[] =
    {
        L"DESIGNTIMESP",
        L"DESIGNTIMEREF",
        L"DESIGNTIMEURL",
    };
    int indexStart, indexEnd, i, indexDSR, indexDSP, indexDSU;
    UINT cchsptag, cchhreftag, cchdsurltag;
    CComBSTR bstrRelativeURL;
    BOOL fHrefSrcFound = FALSE;
    UINT cbNeed;

    // we know that DESIGNTIMESP is not saved for these tags, but check it just to be sure.
    // if we find DESIGNTIMEREF, it means that the HREF was dragged on the page while in design view.
    ASSERT(    pTokArray[iArray].token.tok == TokElem_A
            || pTokArray[iArray].token.tok == TokElem_IMG
            || pTokArray[iArray].token.tok == TokElem_LINK);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);

    indexDSP = indexDSR = indexDSU = -1;
    //get the start tag
    indexStart = iArray;
    while (indexStart >= 0) // generally, it will be the previous token, but just in case...
    {
        if (   (pTokArray[indexStart].token.tok == TokTag_START)
            && (pTokArray[indexStart].token.tokClass == tokTag)
            )
        {
            break;
        }
        indexStart--;
    } // while ()
    if (indexStart < 0) // this can happen only if we have incomplete HTML. Handle error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    indexEnd = iArray;
    while (indexEnd < (int)ptep->m_cMaxToken) // generally, it will be the next token, but just in case...
    {
        if (   (pTokArray[indexEnd].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[indexEnd].token.tokClass == tokTag)
            )
        {
            break;
        }
        indexEnd++;
    }
    if (indexEnd >= (int)ptep->m_cMaxToken) // error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // look for DESIGNTIMEREF inside the tags
    cchsptag = wcslen(rgTags[0]);
    cchhreftag = wcslen(rgTags[1]);
    cchdsurltag = wcslen(rgTags[2]);
    for (i = iArray; i < indexEnd; i++)
    {
        if (       pTokArray[i].token.tok == 0
                && pTokArray[i].token.tokClass == tokSpace
                && cchsptag == pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin
                && (0 == _wcsnicmp(rgTags[0], &pwOld[pTokArray[i].token.ibTokMin], cchsptag))
                )
        {
            indexDSP = i;
            if (indexDSR != -1 && indexDSU != -1) // already initilized
                break;
        }
        else if (  pTokArray[i].token.tok == 0
                && pTokArray[i].token.tokClass == tokSpace
                && cchhreftag == pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin
                && (0 == _wcsnicmp(rgTags[1], &pwOld[pTokArray[i].token.ibTokMin], cchhreftag))
                )
        {
            indexDSR = i;
            if (indexDSP != -1 && indexDSU != -1) // already initilized
                break;
        }
        else if (  pTokArray[i].token.tok == 0
                && pTokArray[i].token.tokClass == tokSpace
                && cchhreftag == pTokArray[i].token.ibTokMac - pTokArray[i].token.ibTokMin
                && (0 == _wcsnicmp(rgTags[2], &pwOld[pTokArray[i].token.ibTokMin], cchdsurltag))
                )
        {
            indexDSU = i;
            if (indexDSP != -1 && indexDSR != -1) // already initilized
                break;
        }
    } // for ()

    // Here is the deal - If we found DESIGNTIMESP, it means that this A/Img/Link existed
    // while in source view. And in that case, we shouldn't find DESIGNTIMEREF. With the
    // same token, if we found DESINTIMEREF, it means that this A/Img/Link was dropped
    // while in design view, so DESIGNTIMESP shouldn't be there. They are mutually exclusive.
    
    // Also, DESIGNTIMEURL can exist only if the href was there while in source view
    // and its value was relative. This can coexist with DESIGNTIMESP, 
    // but not with DESIGNTIMEREF.
    if (indexDSP != -1 && indexDSU == -1) // we found DESIGNTIMESP, but not DESIGNTIMEURL
    {
        ASSERT(indexDSR == -1); // based on above statement, this better be true
        iArray = iArraySav + 1;
        goto LRet;
    }
    if (indexDSR == -1 && indexDSU == -1)
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    if (indexDSR != -1)
    {
        ASSERT(indexDSU == -1); // this better be TRUE, because the 2 are mutually exclusive
        // at this point we know that we have DESIGNTIMEREF (that was put in as part 
        // of drag-drop operation while in design view)
        // modify the href and copy the tag.
        if ((int) (pTokArray[indexStart].token.ibTokMin-ichBeginCopy) > 0)
        {
            cbNeed = (ichNewCur+pTokArray[indexStart].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
            if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                goto LRet;

            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichBeginCopy], 
                    (pTokArray[indexStart].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[indexStart].token.ibTokMin-ichBeginCopy);
        }

        cbNeed = (ichNewCur+pTokArray[indexEnd].token.ibTokMac-pTokArray[indexStart].token.ibTokMin)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;

        // trident mucks with the spacing of these tags and we didn't save any spacing info
        // soput endofline at the end of the tag.
        //pwNew[ichNewCur++] = '\r';
        //pwNew[ichNewCur++] = '\n';
        i = indexStart;

        while (i <= indexEnd)
        {
            if (i == indexDSR)
                i++; // don't copy this token
            else if (      (   pTokArray[i].token.tok == TokAttrib_HREF 
                            || pTokArray[i].token.tok == TokAttrib_SRC
                            )
                        && pTokArray[i].token.tokClass == tokAttr
                        && !fHrefSrcFound
                        )
            {
                fHrefSrcFound = TRUE;
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                i++;
            }
            else if (      pTokArray[i].token.tok == 0 
                        && pTokArray[i].token.tokClass == tokString
                        && fHrefSrcFound
                        )
            {
                HRESULT hr;
                int cchURL;
                WCHAR *pszURL;
                BOOL fQuote = (pwOld[pTokArray[i].token.ibTokMin] == '"');

                cchURL = (fQuote)
                        ? pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin-2
                        : pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin;
                pszURL = new WCHAR [cchURL+1];

                fHrefSrcFound = FALSE;
                if (ptep->m_bstrBaseURL != NULL) // get the relative URL
                {
                    // get the URL string from pwOld and pass it in to relativise
                    memcpy( (BYTE *)pszURL,
                            (BYTE *)&pwOld[pTokArray[i].token.ibTokMin + ((fQuote)? 1 : 0)],
                            (cchURL)*sizeof(WCHAR));
                    pszURL[cchURL] = '\0';
                    hr = UtilConvertToRelativeURL((LPOLESTR)pszURL, ptep->m_bstrBaseURL, NULL, &bstrRelativeURL);
                    if (SUCCEEDED(hr))
                    {
                        // can we assume that bstrRelativeURL is NULL terminated?
                        LPWSTR pszRelativeURL = bstrRelativeURL;
                        if (wcslen(pszRelativeURL) == 0)
                        {
                            memcpy( (BYTE *)&pwNew[ichNewCur],
                                    (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                                    (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                            ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                        }
                        else
                        {
                            pwNew[ichNewCur++] = '"';
                            memcpy( (BYTE *)&pwNew[ichNewCur],
                                    (BYTE *)pszRelativeURL,
                                    wcslen(pszRelativeURL)*sizeof(WCHAR));
                            ichNewCur += wcslen(pszRelativeURL);
                            pwNew[ichNewCur++] = '"';
                        }
                    }
                    else
                    {
                        memcpy( (BYTE *)&pwNew[ichNewCur],
                                (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                        ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                    }
                }
                else
                {
                    memcpy( (BYTE *)&pwNew[ichNewCur],
                            (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
                delete pszURL;
                i++;
            }
            else // all other tokens
            {
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                i++;
            }
        }
        // trident mucks with the spacing of these tags and we didn't save any spacing info
        // so put endofline at the end of the tag.
        //pwNew[ichNewCur++] = '\r';
        //pwNew[ichNewCur++] = '\n';
    }
    else // DESIGNTIMEURL case
    {
        int indexDSUEnd, indexDSPEnd;
        // we found DESIGNTIMEURL. It means, we had this URL while in source view and it was
        // a relative URL then.
        // Check if trident has made it absolute. If it has and the filename is same, 
        // we need to restore it. In all other cases, simply copy the URL and return.
        ASSERT(indexDSR == -1); // this better be TRUE, because the 2 are mutually exclusive
        if ((int) (pTokArray[indexStart].token.ibTokMin-ichBeginCopy) > 0)
        {
            cbNeed = (ichNewCur+pTokArray[indexStart].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
            if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
                goto LRet;

            memcpy( (BYTE *)&pwNew[ichNewCur], 
                    (BYTE *)&pwOld[ichBeginCopy], 
                    (pTokArray[indexStart].token.ibTokMin-ichBeginCopy)*sizeof(WCHAR));
            ichNewCur += (pTokArray[indexStart].token.ibTokMin-ichBeginCopy);
        }

        cbNeed = (ichNewCur+pTokArray[indexEnd].token.ibTokMac-pTokArray[indexStart].token.ibTokMin)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;
        // get indexDSUEnd
        i = indexDSU;
        indexDSUEnd = -1;
        while (i < indexEnd)
        {
            if (   pTokArray[i].token.tok == 0 
                && (pTokArray[i].token.tokClass == tokValue || pTokArray[i].token.tokClass == tokString)
                )
            {
                indexDSUEnd = i;
                break;
            }
            i++;
        }
        if (indexDSUEnd == -1) // we have malformed html
        {
            iArray = iArraySav + 1;
            goto LRet;
        }
        
        // get indexDSPEnd
        i = indexDSP;
        indexDSPEnd = -1;
        while (i < indexEnd)
        {
            if (   pTokArray[i].token.tok == 0 
                && (pTokArray[i].token.tokClass == tokValue || pTokArray[i].token.tokClass == tokString)
                )
            {
                indexDSPEnd = i;
                break;
            }
            i++;
        }
        if (indexDSPEnd == -1) // we have malformed html
        {
            iArray = iArraySav + 1;
            goto LRet;
        }

        i = indexStart;
        while (i <= indexEnd)
        {
            if (   (i >= indexDSU && i <= indexDSUEnd)
                || (i >= indexDSP && i <= indexDSPEnd)
                )
                i++; // don't copy this token
            else if (      (   pTokArray[i].token.tok == TokAttrib_HREF 
                            || pTokArray[i].token.tok == TokAttrib_SRC
                            )
                        && pTokArray[i].token.tokClass == tokAttr
                        && !fHrefSrcFound
                        )
            {
                fHrefSrcFound = TRUE;
                memcpy( (BYTE *)&pwNew[ichNewCur],
                        (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                i++;
            }
            else if (      pTokArray[i].token.tok == 0 
                        && pTokArray[i].token.tokClass == tokString
                        && fHrefSrcFound
                        )
            {
                int ichURL, ichURLEnd, ichDSURL, ichDSURLEnd;
                // if the url is now absloute and is just an absolute version of 
                // the one at indexDSUEnd, we need to replace it.
                ichURL = (pwOld[pTokArray[i].token.ibTokMin] == '"')
                        ? pTokArray[i].token.ibTokMin+1
                        : pTokArray[i].token.ibTokMin;
                ichURLEnd = (pwOld[pTokArray[i].token.ibTokMac-1] == '"')
                        ? pTokArray[i].token.ibTokMac-1
                        : pTokArray[i].token.ibTokMac;
                if (FIsAbsURL((LPOLESTR)&pwOld[ichURL]))
                {
                    WCHAR *pszURL1 = NULL;
                    WCHAR *pszURL2 = NULL;
                    int ich;

                    ichDSURL = (pwOld[pTokArray[indexDSUEnd].token.ibTokMin] == '"')
                            ? pTokArray[indexDSUEnd].token.ibTokMin+1
                            : pTokArray[indexDSUEnd].token.ibTokMin;
                    ichDSURLEnd = (pwOld[pTokArray[indexDSUEnd].token.ibTokMac-1] == '"')
                            ? pTokArray[indexDSUEnd].token.ibTokMac-1
                            : pTokArray[indexDSUEnd].token.ibTokMac;

                    // just for comparison purposes, don't look at '/' or '\' separators
                    // between filenames & directories...
                    pszURL1 = new WCHAR[ichDSURLEnd-ichDSURL + 1];
                    pszURL2 = new WCHAR[ichDSURLEnd-ichDSURL + 1];
                    if (pszURL1 == NULL || pszURL2 == NULL)
                        goto LResumeCopy;
                    memcpy((BYTE *)pszURL1, (BYTE *)&pwOld[ichDSURL], (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                    memcpy((BYTE *)pszURL2, (BYTE *)&pwOld[ichURLEnd-(ichDSURLEnd-ichDSURL)], (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                    pszURL1[ichDSURLEnd-ichDSURL] = '\0';
                    pszURL2[ichDSURLEnd-ichDSURL] = '\0';
                    for (ich = 0; ich < ichDSURLEnd-ichDSURL; ich++)
                    {
                        if (pszURL1[ich] == '/')
                            pszURL1[ich] = '\\';
                        if (pszURL2[ich] == '/')
                            pszURL2[ich] = '\\';
                    }

                    if (0 == _wcsnicmp(pszURL1, pszURL2, ichDSURLEnd-ichDSURL))
                    {
                        pwNew[ichNewCur++] = '"';
                        memcpy( (BYTE *)&pwNew[ichNewCur],
                                (BYTE *)&pwOld[ichDSURL],
                                (ichDSURLEnd-ichDSURL)*sizeof(WCHAR));
                        ichNewCur += (ichDSURLEnd-ichDSURL);
                        pwNew[ichNewCur++] = '"';
                    }
                    else // copy it as it is
                    {
LResumeCopy:
                        memcpy( (BYTE *)&pwNew[ichNewCur],
                                (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                        ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                    }
                    if (pszURL1 != NULL)
                        delete pszURL1;
                    if (pszURL2 != NULL)
                        delete pszURL2;
                }
                else // its realtive, simply copy it
                {
                    memcpy( (BYTE *)&pwNew[ichNewCur],
                            (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
                i++;
            }
            else // all other tokens
            {
                // ****NOTE - we can actually do pretty printing here 
                // instead of fixing the special cases****

                // fix Trident's behaviour - If Trident sees unknown tag(s) it puts it(them) at the end 
                // and inserts EOL before those. In this case, we would have inserted a space before DESIGNTIMESP
                // and Trident would have inserted EOL. If thats not the case, we will ignore it.
                if (   (pTokArray[i].token.tokClass == tokSpace)
                    && (pTokArray[i].token.tok == 0)
                    && (FIsWhiteSpaceToken(pwOld, pTokArray[i].token.ibTokMin, pTokArray[i].token.ibTokMac))
                    )
                {
                    if (i != indexDSU-1) // else skip the copy
                        pwNew[ichNewCur++] = ' '; // convert space+\r+\n into space
                    i++;
                }
                else
                {
                    memcpy( (BYTE *)&pwNew[ichNewCur],
                            (BYTE *)&pwOld[pTokArray[i].token.ibTokMin],
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    ichNewCur += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                    i++;
                }
            }
        } // while (i <= indexEnd)
    } // end of DESIGNTIMEURL case

    // we have spacing save dfor this tag, lets restore it
    if (   (indexDSP != -1)
        && (dwFlags & dwPreserveSourceCode)
        ) 
        ptep->RestoreSpacingSpecial(ptep, pwOld, &pwNew, phgNew, pTokArray, indexDSP, &ichNewCur);


    // remember to set iArray appropriately
    iArray = indexEnd + 1;
    ichBeginCopy = pTokArray[indexEnd].token.ibTokMac;

LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnRestoreAImgLink() */



void
CTriEditParse::fnSaveComment(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{
    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT iArraySav = iArray;
    UINT iCommentStart, iCommentEnd;
    LPCWSTR rgComment[] =
    {
        L"TRIEDITCOMMENT-",
        L"TRIEDITCOMMENTEND-",
        L"TRIEDITPRECOMMENT-",
    };
    int ichSp, cchComment;
    UINT cbNeed;

    // REMOVE METADATA from here, we don't need it because we are checking for end
    // of comment too.

    ASSERT(pTokArray[iArray].token.tok == TokTag_BANG);
    ASSERT(pTokArray[iArray].token.tokClass == tokTag);
    // early return cases
    // 1. see if this is a comment or not. It could be anything that starts with '<!'
    // e.g. <!DOCTYPE
    if (   (iArray+1 < (INT)ptep->m_cMaxToken)
        && (pwOld[pTokArray[iArray+1].token.ibTokMin] == '-')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+1] == '-')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+2] == '[')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+3] == 'i')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+3] == 'I')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+4] == 'f')
        && (pwOld[pTokArray[iArray+1].token.ibTokMin+4] == 'F')
        )
    {
        iCommentStart = iArray; // this is a comment we are interested in
    }
    else
    {
        iArray = iArraySav + 1; // not this one
        goto LRet;
    }
    iCommentEnd = iArray + 2;
    ASSERT(iCommentEnd < (INT)ptep->m_cMaxToken);
    if (   pTokArray[iCommentEnd].token.tok != TokTag_CLOSE 
        && pTokArray[iCommentEnd].token.tokClass != tokTag)
    {
        // we have found something that looks like a comment to begin with, but its
        // something else like a DTC, webbot stuff or some thing else...
        iArray = iArraySav + 1; // not this one
        goto LRet;
    }

    // write the spacing info, reallocate pwNew if needed
    cchComment = pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin;
    cbNeed = (ichNewCur+2*cchComment+wcslen(rgComment[0])+wcslen(rgComment[1])+(pTokArray[iCommentStart].token.ibTokMac-ichBeginCopy+2))*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;

    // write till '<!--' part of the comment
    memcpy( (BYTE *)&pwNew[ichNewCur],
            (BYTE *)&pwOld[ichBeginCopy],
            (pTokArray[iCommentStart].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
    ichNewCur += pTokArray[iCommentStart].token.ibTokMac-ichBeginCopy;
    pwNew[ichNewCur++] = '-';
    pwNew[ichNewCur++] = '-';
    
    // write the spacing info keyword
    memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgComment[0], wcslen(rgComment[0])*sizeof(WCHAR));
    ichNewCur += wcslen(rgComment[0]);
    //write spacing block
    ichSp = pTokArray[iCommentStart+1].token.ibTokMin+2; // exclude -- from <!--comment
    while (ichSp < (int)(pTokArray[iCommentStart+1].token.ibTokMac-2))// exclude -- from comment-->
    {
        switch (pwOld[ichSp++])
        {
        case ' ':
            pwNew[ichNewCur++] = chCommentSp;
            break;
        case '\t':
            pwNew[ichNewCur++] = chCommentTab;
            break;
        case '\r':
            pwNew[ichNewCur++] = chCommentEOL;
            break;
        case '\n':
            break;
        default:
            if (pwNew[ichNewCur-1] != ',')
                pwNew[ichNewCur++] = ',';
            break;
        } // switch()
    }

    // write the spacing info keyword
    memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgComment[1], wcslen(rgComment[1])*sizeof(WCHAR));
    ichNewCur += wcslen(rgComment[1]);

    //write spacing block for pre comment
    // go back from pwOld[ichSp] and see where we have the last non-white space
    ichSp = pTokArray[iCommentStart].token.ibTokMin-1;
    while (    (ichSp >= 0)
            && (   pwOld[ichSp] == ' '  || pwOld[ichSp] == '\t'
                || pwOld[ichSp] == '\r' || pwOld[ichSp] == '\n'
                )
            )
    {
        ichSp--;
    }
    ichSp++; // compensate because ichSp points to non-white space character at this point
    ASSERT(pTokArray[iCommentStart].token.ibTokMin >= (UINT)ichSp);
    cbNeed = (ichNewCur+2*(pTokArray[iCommentStart].token.ibTokMin-ichSp)+wcslen(rgComment[2]))*sizeof(WCHAR)+cbBufPadding;
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
        goto LRet;
    while (ichSp < (int)(pTokArray[iCommentStart].token.ibTokMin))
    {
        switch (pwOld[ichSp++])
        {
        case ' ':
            pwNew[ichNewCur++] = chCommentSp;
            break;
        case '\t':
            pwNew[ichNewCur++] = chCommentTab;
            break;
        case '\r':
            pwNew[ichNewCur++] = chCommentEOL;
            break;
        case '\n':
            break;
        default:
            if (pwNew[ichNewCur-1] != ',')
                pwNew[ichNewCur++] = ',';
            break;
        } // switch()
    }
    // write the spacing info keyword
    memcpy((BYTE *)&pwNew[ichNewCur], (BYTE *)rgComment[2], wcslen(rgComment[2])*sizeof(WCHAR));
    ichNewCur += wcslen(rgComment[2]);
    
    // write the comment
    memcpy( (BYTE *)&pwNew[ichNewCur],
            (BYTE *)&pwOld[pTokArray[iCommentStart+1].token.ibTokMin+2], 
            (pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2)*sizeof(WCHAR));
    ichNewCur += pTokArray[iCommentStart+1].token.ibTokMac-pTokArray[iCommentStart+1].token.ibTokMin-2;

    // write the ending '>'
    pwNew[ichNewCur++] = '>'; // alternatively, we could write iCommentEnd'th token

    // set iArray & ichBeginCopy
    iArray = iCommentEnd+1;
    ichBeginCopy = pTokArray[iCommentEnd].token.ibTokMac;
LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnSaveComment() */

void
CTriEditParse::fnRestoreComment(CTriEditParse* /*ptep*/,
          LPWSTR /*pwOld*/, LPWSTR* /*ppwNew*/, UINT* /*pcchNew*/, HGLOBAL* /*phgNew*/, 
          TOKSTRUCT* /*pTokArray*/, UINT* /*piArrayStart*/, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT* /*pichNewCur*/, UINT* /*pichBeginCopy*/,
          DWORD /*dwFlags*/)
{
    ASSERT(FALSE); // this case is handled by fnRestoreObject(), so we shouldn't reach here
    return;

} /* fnRestoreComment() */

void
CTriEditParse::fnSaveTextArea(CTriEditParse *ptep, LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew, HGLOBAL *phgNew, 
          TOKSTRUCT *pTokArray, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT *pichNewCur, UINT *pichBeginCopy,
          DWORD /*dwFlags*/)
{

    UINT ichNewCur = *pichNewCur;
    UINT ichBeginCopy = *pichBeginCopy;
    UINT iArray = *piArrayStart;
    LPWSTR pwNew = *ppwNew;
    UINT iArraySav = iArray;
    UINT cbNeed;
    UINT iTextAreaEnd;

    // look for TEXTAREA block and simply copy it into pwNew. Thereby avoiding the
    // space preservation & stuff.

    ASSERT(pTokArray[iArray].token.tok == TokElem_TEXTAREA);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);
    iTextAreaEnd = pTokArray[iArray].iNextprev;
    if (iTextAreaEnd == -1) // we don't have matching </textarea>
    {
        // ignore this case
        iArray = iArraySav + 1;
        goto LRet;
    }

    // NOTE that we don't even need to get get the '<' before the textarea here because we are
    // not doing anything special with them. We simply are going to copy everything inside the
    // textarea to pwNew. So, we start copying from ichBeginCopy and copy till end of the 
    // textarea block.

    // get the '>' after the matching end textarea, generally this will be right after iTextAreaEnd
    while (iTextAreaEnd < (int)ptep->m_cMaxToken)
    {
        if (   (pTokArray[iTextAreaEnd].token.tok == TokTag_CLOSE) /* > */
            && (pTokArray[iTextAreaEnd].token.tokClass == tokTag)
            )
        {
            break;
        }
        iTextAreaEnd++;
    }
    if (iTextAreaEnd >= (int)ptep->m_cMaxToken) // error case
    {
        iArray = iArraySav + 1;
        goto LRet;
    }

    // copy the textarea block into pwNew. Make sure that we have enough space in pwNew
    // NOTE - pTokArray[iTextAreaEnd].token.ibTokMac should be larger than ichBeginCopy,
    // but at this point in the game the assert is of no use, because no one is using 
    // debug builds (6/10/98)
    if ((int) (pTokArray[iTextAreaEnd].token.ibTokMac-ichBeginCopy) > 0)
    {
        cbNeed = (ichNewCur+pTokArray[iTextAreaEnd].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR)+cbBufPadding;
        if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
            goto LRet;

        memcpy( (BYTE *)&pwNew[ichNewCur], 
                (BYTE *)&pwOld[ichBeginCopy], 
                (pTokArray[iTextAreaEnd].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (pTokArray[iTextAreaEnd].token.ibTokMac-ichBeginCopy);
    }

    // set iArray & ichBeginCopy
    iArray = iTextAreaEnd+1;
    ichBeginCopy = pTokArray[iTextAreaEnd].token.ibTokMac;
LRet:
    *pcchNew = ichNewCur;
    *ppwNew = pwNew;

    *pichNewCur = ichNewCur;
    *pichBeginCopy = ichBeginCopy;
    *piArrayStart = iArray;

    return;

} /* fnSaveTextArea() */

void
CTriEditParse::fnRestoreTextArea(CTriEditParse* /*ptep*/,
          LPWSTR /*pwOld*/, LPWSTR* /*ppwNew*/, UINT* /*pcchNew*/, HGLOBAL* /*phgNew*/, 
          TOKSTRUCT* /*pTokArray*/, UINT *piArrayStart, FilterTok /*ft*/,
          INT* /*pcHtml*/, UINT* /*pichNewCur*/, UINT* /*pichBeginCopy*/,
          DWORD /*dwFlags*/)
{
    UINT iArray = *piArrayStart;

    // ideally, (for next version) we should restore the trident-converted &gt's & stuff
    // for now, we are simply going to ignore this tag on the way back from trident
    // NOTE that we never put in designtimesp's in this block, so we souldn't have to look
    // for them here.

    ASSERT(pTokArray[iArray].token.tok == TokElem_TEXTAREA);
    ASSERT(pTokArray[iArray].token.tokClass == tokElem);

    iArray++; // skip this textarea tag

    *piArrayStart = iArray;
    return;

} /* fnRestoreTextArea() */

void
CTriEditParse::FilterHtml(LPWSTR pwOld, LPWSTR* ppwNew, UINT *pcchNew,
                          HGLOBAL *phgNew, TOKSTRUCT *pTokArray, 
                          FilterMode mode, DWORD dwFlags)
{
    UINT iArray = 0;
    UINT ichNewCur = 0;
    UINT ichBeginCopy = 0;
    HRESULT hr;
    INT index = 0;
    INT iItem;
    INT cItems = 0;
    INT cRuleMid = cRuleMax / 2; // ASSUME that cRuleMax is an even number

    FilterRule fr[cRuleMax] =
    {
    // make sure that modeInput and modeOutput have the matching entries.
    // modeInput entries
    {TokTag_BANG, TokAttrib_STARTSPAN, tokClsIgnore, TokTag_CLOSE, TokAttrib_ENDSPAN, tokClsIgnore, fnSaveDTC},
    {TokTag_SSSOPEN, -1, tokClsIgnore, TokTag_SSSCLOSE, -1, tokClsIgnore, fnSaveSSS},
    {TokTag_START, TokElem_HTML, tokClsIgnore, TokTag_CLOSE, TokElem_HTML, tokClsIgnore, fnSaveHtmlTag},
    {-1, -1, tokEntity, -1, -1, tokEntity, fnSaveNBSP},
    {-1, TokElem_BODY, tokElem, -1, -1, tokClsIgnore, fnSaveHdr},
    {TokTag_END, TokElem_BODY, tokElem, -1, -1, tokClsIgnore, fnSaveFtr},
    {-1, TokTag_START, tokTag, TokTag_CLOSE, -1, tokClsIgnore, fnSaveSpace},
    {TokTag_START, TokElem_OBJECT, tokElem, TokTag_CLOSE, TokElem_OBJECT, tokElem, fnSaveObject},
    {TokTag_START, TokElem_TBODY, tokElem, TokTag_CLOSE, -1, tokTag, fnSaveTbody},
    {-1, TokElem_APPLET, tokElem, -1, -1, -1, fnSaveApplet},
    {TokTag_START, TokElem_A, tokElem, TokTag_CLOSE, TokAttrib_HREF, tokTag, fnSaveAImgLink},
    {-1, TokTag_BANG, tokTag, -1, -1, tokClsIgnore, fnSaveComment},
    {TokTag_START, TokElem_TEXTAREA, tokElem, TokTag_CLOSE, TokElem_TEXTAREA, tokClsIgnore, fnSaveTextArea},

    // modeOutput entries
    {TokTag_START, TokElem_OBJECT, tokClsIgnore, TokTag_CLOSE, TokElem_OBJECT, tokClsIgnore, fnRestoreDTC},
    {TokTag_START, TokElem_SCRIPT, tokClsIgnore, TokTag_CLOSE, TokElem_SCRIPT, tokClsIgnore, fnRestoreSSS},
    {-1, -1, tokClsIgnore, -1, -1, tokClsIgnore, fnRestoreHtmlTag},
    {-1, -1, tokEntity, -1, -1, tokEntity, fnRestoreNBSP},
    {-1, TokElem_BODY, tokElem, -1, -1, tokClsIgnore, fnRestoreHdr},
    {TokTag_END, TokElem_BODY, tokElem, -1, -1, tokClsIgnore, fnRestoreFtr},
    {TokTag_START, TokTag_END, tokSpace, TokTag_CLOSE, -1, tokClsIgnore, fnRestoreSpace},
    {-1, TokTag_BANG, tokTag, TokTag_CLOSE, -1, tokTag, fnRestoreObject},
    {TokTag_START, TokElem_TBODY, tokElem, TokTag_CLOSE, -1, tokTag, fnRestoreTbody},
    {TokTag_START, TokElem_APPLET, tokElem, TokTag_CLOSE, -1, tokTag, fnRestoreApplet},
    {TokTag_START, TokElem_A, tokElem, TokTag_CLOSE, TokAttrib_HREF, tokTag, fnRestoreAImgLink},
    {-1, TokTag_BANG, tokTag, TokTag_CLOSE, -1, tokTag, fnRestoreObject},
    {TokTag_START, TokElem_TEXTAREA, tokElem, TokTag_CLOSE, TokElem_TEXTAREA, tokClsIgnore, fnRestoreTextArea},
    };
    
    memcpy(m_FilterRule, fr, sizeof(FilterRule)*cRuleMax);
    ASSERT(pwOld != NULL);
    ASSERT(*ppwNew != NULL);

    if (mode == modeInput)
    {
        cItems = m_cDTC + m_cSSSIn + m_cHtml + m_cNbsp + m_cHdr + m_cFtr + m_cObjIn + m_ispInfoIn + m_cAppletIn + m_cAImgLink;
        while (cItems > 0)
        {
            if (iArray >= m_cMaxToken) // this will catch error cases
                break;

            while (iArray < m_cMaxToken)
            {   
                // its OK to enumerate the comparison rules, but once we have
                // a lot of rules, this needs to be made into a function
                if (pTokArray[iArray].token.tok == m_FilterRule[0].ft.tokBegin2 && m_cDTC > 0)
                {
                    m_cDTC--;
                    iItem = 1;
                    index = 0;
                    break;
                }
                else if (     (m_FilterRule[1].ft.tokBegin2 != -1)
                            ? (pTokArray[iArray].token.tok == m_FilterRule[1].ft.tokBegin2 && m_cSSSIn > 0)
                            : (pTokArray[iArray].token.tok == m_FilterRule[1].ft.tokBegin && m_cSSSIn > 0)
                            )
                {
                    m_cSSSIn--;
                    iItem = 1;
                    index = 1;
                    break;
                }
                else if (pTokArray[iArray].token.tok == m_FilterRule[2].ft.tokBegin2 && m_cHtml > 0)
                {
                    m_cHtml--;
                    iItem = 1;
                    index = 2;
                    break;
                }
                else if (      m_FilterRule[3].ft.tokBegin == -1 
                            && m_FilterRule[3].ft.tokBegin2 == -1
                            && m_FilterRule[3].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && m_cNbsp > 0)
                {
                    m_cNbsp--;
                    iItem = 1;
                    index = 3;
                    break;
                }
                else if (pTokArray[iArray].token.tok == m_FilterRule[4].ft.tokBegin2 && m_cHdr > 0)
                {
                    m_cHdr--;
                    iItem = 1;
                    index = 4;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[5].ft.tokBegin2
                            && (iArray-1 >= 0)
                            && pTokArray[iArray-1].token.tok == m_FilterRule[5].ft.tokBegin
                            && m_cFtr > 0
                            )
                {
                    m_cFtr--;
                    iItem = 1;
                    index = 5;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[6].ft.tokBegin2 
                            && m_FilterRule[6].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && m_ispInfoIn > 0
                            && (dwFlags & dwPreserveSourceCode)
                            )
                {
                    cItems++; // to compensate for cItems-- after the pfn() call
                    index = 6;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[7].ft.tokBegin2 
                            && pTokArray[iArray].token.tokClass == m_FilterRule[7].ft.tokClsBegin
                            && pTokArray[iArray-1].token.tok == TokTag_START
                            && pTokArray[iArray-1].token.tokClass == tokTag
                            && m_cObjIn > 0
                            )
                {
                    m_cObjIn--;
                    iItem = 1;
                    index = 7;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[8].ft.tokBegin2
                            && pTokArray[iArray].token.tokClass == m_FilterRule[8].ft.tokClsBegin
                            && (iArray-1 >= 0) /* validation*/
                            && pTokArray[iArray-1].token.tok == TokTag_START
                            && pTokArray[iArray-1].token.tokClass == tokTag
                            && (dwFlags & dwPreserveSourceCode)
                            )
                {
                    cItems++; //to compensate for cItems-- after the pfn() call
                    iItem = 1;
                    index = 8;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[9].ft.tokBegin2 
                            && m_FilterRule[9].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && m_cAppletIn > 0
                            )
                {
                    cItems++; //to compensate for cItems-- after the pfn() call
                    m_cAppletIn--;
                    index = 9;
                    break;
                }
                else if (      (   pTokArray[iArray].token.tok == m_FilterRule[10].ft.tokBegin2
                                || pTokArray[iArray].token.tok == TokElem_IMG
                                || pTokArray[iArray].token.tok == TokElem_LINK)
                            && m_FilterRule[10].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && m_cAImgLink > 0
                            && (iArray-1 >= 0)
                            && (pTokArray[iArray-1].token.tok == m_FilterRule[10].ft.tokBegin)
                            )
                {
                    cItems++; // to compensate for cItems-- after the pfn() call
                    index = 10;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[11].ft.tokBegin2 
                            && m_FilterRule[11].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            )
                {
                    cItems++; // to compensate for cItems-- after the pfn() call
                    index = 11;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[12].ft.tokBegin2 
                            && m_FilterRule[12].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && pTokArray[iArray-1].token.tok == m_FilterRule[12].ft.tokBegin
                            && pTokArray[iArray-1].token.tokClass == tokTag
                            )
                {
                    cItems++; // to compensate for cItems-- after the pfn() call
                    index = 12;
                    break;
                }

                iArray++;
            }
            if (iArray < m_cMaxToken) // we found a match
            {
                // call that function
                m_FilterRule[index].pfn(    this, pwOld, ppwNew, pcchNew, phgNew, pTokArray, 
                                            &iArray, m_FilterRule[index].ft, &iItem, 
                                            &ichNewCur, &ichBeginCopy,
                                            dwFlags);
            }

            cItems--;
        } // while (cItems > 0)
    }
    else if (mode == modeOutput)
    {
        cItems = m_cObj + m_cSSSOut + m_cHtml + m_cNbsp + m_cHdr + m_cFtr + m_cComment + m_ispInfoOut + m_cAppletOut + m_cAImgLink;
        while (cItems > 0)
        {
            if (iArray >= m_cMaxToken) // this will catch error cases
                break;

            while (iArray < m_cMaxToken)
            {   
                // its OK to enumerate the comparison rules, but once we have
                // a lot of rules, this needs to be made into a function
                if (   pTokArray[iArray].token.tok == m_FilterRule[cRuleMid].ft.tokBegin2
                    && pTokArray[iArray-1].token.tok == TokTag_START
                    && m_cObj > 0
                        )
                {
                    m_cObj--;
                    index = cRuleMid;
                    iItem = m_iControl;
                    break;
                }
                else if (pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+1].ft.tokBegin2 && m_cSSSOut > 0)
                {
                    m_cSSSOut--;
                    iItem = 1;
                    index = cRuleMid+1;
                    break;
                }
                else if (pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+2].ft.tokBegin2 && m_cHtml > 0)
                {
                    m_cHtml--;
                    iItem = 1;
                    index = cRuleMid+2;
                    break;
                }
                else if (      m_FilterRule[cRuleMid+3].ft.tokBegin == -1 
                            && m_FilterRule[cRuleMid+3].ft.tokBegin2 == -1
                            && m_FilterRule[cRuleMid+3].ft.tokClsBegin == tokEntity
                            && m_cNbsp > 0)
                {
                    m_cNbsp--;
                    iItem = 1;
                    index = cRuleMid+3;
                    break;
                }
                else if (pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+4].ft.tokBegin2 && m_cHdr > 0)
                {
                    m_cHdr--;
                    iItem = 1;
                    index = cRuleMid+4;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+5].ft.tokBegin2 
                            && (iArray-1 >= 0)
                            && pTokArray[iArray-1].token.tok == m_FilterRule[cRuleMid+5].ft.tokBegin
                            && m_cFtr > 0)
                {
                    m_cFtr--;
                    iItem = 1;
                    index = cRuleMid+5;
                    break;
                }
                else if (      (       pTokArray[iArray].token.tokClass == m_FilterRule[cRuleMid+6].ft.tokClsBegin
                                    || (       pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+6].ft.tokBegin2
                                            && pTokArray[iArray].token.tokClass == tokTag
                                            )
                                    )
                            && (dwFlags & dwPreserveSourceCode)
                            )
                {
                    index = cRuleMid+6;
                    cItems++; // to compensate for cItems-- after the pfn() call
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+7].ft.tokBegin2 
                            && pTokArray[iArray].token.tokClass == m_FilterRule[cRuleMid+7].ft.tokClsBegin
                            && m_cComment > 0
                            )
                {
                    m_cComment--;
                    iItem = 1;
                    index = cRuleMid+7;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+8].ft.tokBegin2 
                            && pTokArray[iArray].token.tokClass == m_FilterRule[cRuleMid+8].ft.tokClsBegin
                            && (dwFlags & dwPreserveSourceCode)
                            )
                {
                    // Note that TBody filtering is tied in with space preservation.
                    // In ideal world it shouldn't be, but thats acceptable to the most.
                    // If this view changes, we need to add some other designtime attribute 
                    // along with spacing attributes. This will be somewhat big change than 
                    // simply adding an attribute because then we need to change the code to 
                    // start going backwards in the token array in the main loop.
                    iItem = 1;
                    index = cRuleMid+8;
                    cItems++; //  to compensate for cItems-- after the pfn() call
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+9].ft.tokBegin2
                            && pTokArray[iArray].token.tokClass == m_FilterRule[cRuleMid+9].ft.tokClsBegin
                            && pTokArray[iArray-1].token.tok == m_FilterRule[cRuleMid+9].ft.tokBegin
                            && m_cAppletOut > 0
                            )
                {
                    cItems++; //  to compensate for cItems-- after the pfn() call
                    m_cAppletOut--;
                    index = cRuleMid+9;
                    break;
                }
                else if (      (   pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+10].ft.tokBegin2
                                || pTokArray[iArray].token.tok == TokElem_IMG
                                || pTokArray[iArray].token.tok == TokElem_LINK)
                            && m_FilterRule[cRuleMid+10].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && m_cAImgLink > 0
                            && (iArray-1 >= 0)
                            && (pTokArray[iArray-1].token.tok == m_FilterRule[cRuleMid+10].ft.tokBegin)
                            )
                {
                    index = cRuleMid+10;
                    cItems++; //  to compensate for cItems-- after the pfn() call
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+11].ft.tokBegin2 
                            && pTokArray[iArray].token.tokClass == m_FilterRule[cRuleMid+11].ft.tokClsBegin
                            )
                {
                    // actually, we won't reach here - just a dummy
                    cItems++; //  to compensate for cItems-- after the pfn() call
                    index = cRuleMid+11;
                    break;
                }
                else if (      pTokArray[iArray].token.tok == m_FilterRule[cRuleMid+12].ft.tokBegin2 
                            && m_FilterRule[cRuleMid+12].ft.tokClsBegin == pTokArray[iArray].token.tokClass
                            && pTokArray[iArray-1].token.tok == m_FilterRule[cRuleMid+12].ft.tokBegin
                            && pTokArray[iArray-1].token.tokClass == tokTag
                            )
                {
                    cItems++; // to compensate for cItems-- after the pfn() call
                    index = cRuleMid+12;
                    break;
                }


                iArray++;
            }
            if (iArray < m_cMaxToken) // we found a match
            {
                // call that function
                m_FilterRule[index].pfn(    this, pwOld, ppwNew, pcchNew, phgNew, pTokArray, 
                                            &iArray, m_FilterRule[index].ft, &iItem, 
                                            &ichNewCur, &ichBeginCopy,
                                            dwFlags);
            }

            if (m_fDontDeccItem) // we can do things differently next time
            {
                m_fDontDeccItem = FALSE;
                cItems++;
            }
            cItems--;
        } // while (cItems > 0)
    }
    else
        ASSERT(FALSE);


    if (cItems == 0) // everything ok, copy rest of the doc
    {
LIncorrectcItems:
        // copy rest of the stuff into pwNew
        /* REALLOCATE pwNew IF NEEDED here use cache value for GlobalSize(*phgNew) and don't forget to update it too */
        if (GlobalSize(*phgNew) < (ichNewCur+pTokArray[m_cMaxToken-1].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR))
        {
            hr = ReallocBuffer( phgNew,
                                (ichNewCur+pTokArray[m_cMaxToken-1].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR),
                                GMEM_MOVEABLE|GMEM_ZEROINIT);
            if (hr == E_OUTOFMEMORY)
                goto LCopyAndRet;
            ASSERT(*phgNew != NULL);
            *ppwNew = (WCHAR *)GlobalLock(*phgNew);
        }
        memcpy( (BYTE *)(*ppwNew+ichNewCur),
                (BYTE *)(pwOld+ichBeginCopy),
                (pTokArray[m_cMaxToken-1].token.ibTokMac-ichBeginCopy)*sizeof(WCHAR));
        ichNewCur += (pTokArray[m_cMaxToken-1].token.ibTokMac-ichBeginCopy);
        *pcchNew = ichNewCur;
    }
    else
    {
        // this means that we calculated one of m_c's incorrectly. We need to fix that
        // case in M4
        goto LIncorrectcItems;

LCopyAndRet:
        memcpy( (BYTE *)*ppwNew,
                (BYTE *)pwOld,
                (pTokArray[m_cMaxToken-1].token.ibTokMac)*sizeof(WCHAR));
        *pcchNew = pTokArray[m_cMaxToken-1].token.ibTokMac;
    }

} /* CTriEditParse::FilterHtml() */

int
CTriEditParse::ValidateTag(LPWSTR pszText)
{
    int len = 0;

    if (pszText == NULL)
        return(0);
    // check for the first non Alpha in the pszText and return it. Add '\0' at the end
    while (    (*(pszText+len) >= _T('A') && *(pszText+len) <= _T('Z'))
            || (*(pszText+len) >= _T('a') && *(pszText+len) <= _T('z'))
            || (*(pszText+len) >= _T('0') && *(pszText+len) <= _T('9'))
            )
    {
        len++;
    }

    return(len);
}

INT
CTriEditParse::GetTagID(LPWSTR pszText, TXTB token)
{
    WCHAR szTag[MAX_TOKIDLEN+1];
    int len;
    int tagID;

    len = ValidateTag(pszText+token.ibTokMin);
    if (len == 0 || len != (int)(token.ibTokMac-token.ibTokMin))
        tagID = -1;
    else
    {
		if (token.tok == 0 && token.tokClass == tokIDENTIFIER)
			tagID = -1;
		else
		{
			memcpy((BYTE *)szTag, (BYTE *)(pszText+token.ibTokMin), (min(len, MAX_TOKIDLEN))*sizeof(WCHAR));
			szTag[min(len, MAX_TOKIDLEN)] = '\0';
			tagID = IndexFromElementName((LPCTSTR) szTag);
		}
    }
    return(tagID);
}
void
CTriEditParse::PreProcessToken(TOKSTRUCT *pTokArray, INT *pitokCur, LPWSTR /*pszText*/, 
                               UINT /*cbCur*/, TXTB token, DWORD lxs, INT tagID, FilterMode mode)
{
    TOKSTRUCT *pTokT = pTokArray + *pitokCur;

    if (*pitokCur == -1) // the buffer reallocation must have failed
        goto LSkipArrayOp;

    // if (lxs & inTag) then we can ASSERT(token.tok == TokTag_START)
    //put the new token into pTokArray at *pitokCur position
    pTokT->token = token;
    pTokT->fStart = (lxs & inEndTag)?FALSE:TRUE;
    pTokT->ichStart = token.ibTokMin;
    pTokT->iNextprev = 0xFFFFFFFF; // init value
    pTokT->iNextPrevAlternate = 0xFFFFFFFF; // init value
    pTokT->tagID = tagID;

    if (mode == modeInput)
    {
        if (   pTokT->token.tok == TokTag_SSSOPEN
            && pTokT->token.tokClass == tokSSS
            && ((lxs & inSCRIPT) || (lxs & inAttribute))
            )
        {
            pTokT->token.tok = TokTag_SSSOPEN_TRIEDIT;
        }
        if (   pTokT->token.tok == TokTag_SSSCLOSE
            && pTokT->token.tokClass == tokSSS
            && ((lxs & inSCRIPT) || (lxs & inAttribute))
            )
        {
            pTokT->token.tok = TokTag_SSSCLOSE_TRIEDIT;
        }
    }

    *pitokCur += 1;

LSkipArrayOp:
    return;

} /* CTriEditParse::PreProcessToken() */


// Handle special cases of replacing things and saving the replaced contents
void
CTriEditParse::PostProcessToken(OLECHAR* /*pwOld*/, OLECHAR* /*pwNew*/, UINT* /*pcbNew*/, 
                                UINT /*cbCur*/, UINT /*cbCurSav*/, TXTB token, 
                                FilterMode mode, DWORD lxs, DWORD dwFlags)
{
    // handle special cases of replacing the DTCs, ServerSideScripts etc.
    // save the contents into a buffer if (mode == modeInput)
    // put the contents back into buffer if (mode == modeOutput)

    if (mode == modeInput)
    {
        if (   token.tok == TokAttrib_ENDSPAN
            && token.tokClass == tokAttr
            && (dwFlags & (dwFilterDTCs | dwFilterDTCsWithoutMetaTags))
            )
        {
            m_cDTC++;
        }
        if (   token.tok == TokTag_SSSCLOSE
            && token.tokClass == tokSSS
            && !(lxs & inAttribute) // !(lxs & inValue && lxs & inTag)
            && !(lxs & inSCRIPT)
            && (dwFlags & dwFilterServerSideScripts)
            )
        {
            m_cSSSIn++;
        }
        if (   token.tokClass == tokEntity
            && dwFlags != dwFilterNone
            )
        {
            m_cNbsp++;
        }
        if (   (token.tok == TokElem_OBJECT)
            && (token.tokClass == tokElem)
            && (lxs & inEndTag)
            && (dwFlags != dwFilterNone)
            )
        {
            m_cObjIn++;
        }
        if (   token.tok == TokElem_APPLET
            && token.tokClass == tokElem
            && (lxs & inEndTag)
            && (dwFlags != dwFilterNone)
            )
        {
            m_cAppletIn++;
        }
    }
    else if (mode == modeOutput)
    {
        if (   token.tok == TokElem_OBJECT
            && token.tokClass == tokElem
            && (lxs & inTag && !(lxs & inEndTag))
            && (dwFlags & (dwFilterDTCs | dwFilterDTCsWithoutMetaTags))
            )
        {
            m_cObj++;
        }
        if (   token.tok == TokElem_SCRIPT
            && token.tokClass == tokElem
            && (lxs & inEndTag)
            && (dwFlags & dwFilterServerSideScripts)
            )
        {
            m_cSSSOut++;
        }
        if (   token.tok == TokTag_BANG
            && token.tokClass == tokTag
            )
        {
            m_cComment++;
        }
        if (   token.tok == TokElem_APPLET
            && token.tokClass == tokElem
            && (lxs & inEndTag)
            && (dwFlags != dwFilterNone)
            )
        {
            m_cAppletOut++;
        }
    }

} /* CTriEditParse::PostProcessToken() */

HRESULT 
CTriEditParse::ProcessToken(DWORD &lxs, TXTB &tok, LPWSTR pszText, 
                            UINT /*cbCur*/, TOKSTACK *pTokStack, INT *pitokTop, 
                            TOKSTRUCT *pTokArray, INT iArrayPos, INT tagID)
{
    TXTB token = tok;

    if (*pitokTop == -1) // the buffer reallocation must have failed
        goto LSkipStackOp;

    if (lxs & inEndTag) // end tag begins, set m_fEndTagFound
        m_fEndTagFound = TRUE;

    if (tagID == -1) // we need to put only the IDENTIFIERS on the stack
    {
        // special cases (1)<%, (2)%>, (3)startspan, (4)endspan
        if (token.tok == TokTag_SSSOPEN && token.tokClass == tokSSS /*&& !(lxs & inAttribute)*/) // <%
        {
            token.tok = TokTag_SSSCLOSE; // fake it so that we can use the same code for matching %>
            goto LSpecialCase;
        }
        else if (token.tok == TokTag_SSSCLOSE && token.tokClass == tokSSS /*&& !(lxs & inAttribute)*/) // %>
        {
            m_fEndTagFound = TRUE; // lxs is not inEndTag when we get TokTag_SSSCLOSE
            goto LSpecialCase;
        }
        else if (token.tok == TokAttrib_STARTSPAN && token.tokClass == tokAttr) // startspan
        {
            token.tok = TokAttrib_ENDSPAN; // fake it so that we can use the same code for matching endspan
            goto LSpecialCase;
        }
        else if (token.tok == TokAttrib_ENDSPAN && token.tokClass == tokAttr) // endspan
        {
            LPCWSTR szDesignerControl[] =
            {
                L"\"DesignerControl\"",
                L"DesignerControl",
            };
            
            // HACK to fix FrontPage BUG - DaVinci puts a dummy endspan & startspan between
            // the "DESIGNERCONTROL" startspan-endspan pair. We want to make sure that
            // our pTokArray has correct matching iNextprev for the TokAttrib_STARTSPAN
            // Refer VID bug 3991
            if (       (iArrayPos-3 >= 0) /* validation */
                    && (   0 == _wcsnicmp(szDesignerControl[0], &pszText[pTokArray[iArrayPos-3].token.ibTokMin], wcslen(szDesignerControl[0]))
                        || 0 == _wcsnicmp(szDesignerControl[1], &pszText[pTokArray[iArrayPos-3].token.ibTokMin], wcslen(szDesignerControl[1]))
                        )
                    )
            {
                m_fEndTagFound = TRUE; // lxs is not inEndTag when we get TokAttrib_ENDSPAN
                goto LSpecialCase;
            }
            else
                goto LSkipStackOp;
        }
        else
        {
            if (m_fEndTagFound)
                m_fEndTagFound = FALSE;
            goto LSkipStackOp;
        }
    }

LSpecialCase:   
    if (m_fEndTagFound) // end tag was found previously, means pop from the stack
    {
        TOKSTACK *pTokT;

        if (*pitokTop == 0) // we don't have anything on stack, we can't delete it
            goto LSkipStackOp;

        pTokT = pTokStack + *pitokTop - 1;
        m_fEndTagFound = FALSE; // reset

        // if we get an end tag, in ideal case, the top of the stack should
        // match with what we got
        if (tagID == pTokT->tagID)
        {
            if (tagID == -1) // special case, match token.tok & token.tokClass
            {
                if (   (pTokT->token.tok == TokTag_SSSCLOSE) /* faked token for <% */
                    && (pTokT->token.tokClass == tokSSS)
                    )
                {
                    ASSERT(token.tok == TokTag_SSSCLOSE);
                    goto LMatch;
                }
                else if (   (pTokT->token.tok == TokAttrib_ENDSPAN) /* faked token for startspan */
                    && (pTokT->token.tokClass == tokAttr)
                    )
                {
                    ASSERT(token.tok == TokAttrib_ENDSPAN);
                    goto LMatch;
                }
                else // we may have found another special case
                {
                    goto LNoMatch;
                }
            }
LMatch:
            ASSERT(iArrayPos - 1 >= 0);
            // put iNextPrev or INextPrevAlternate for the matching start token in pTokArray
            pTokArray[pTokT->iMatch].iNextprev = iArrayPos - 1;
            ASSERT(pTokArray[pTokT->iMatch].fStart == TRUE);
            ASSERT(pTokT->ichStart == pTokArray[pTokT->iMatch].token.ibTokMin);
            pTokArray[iArrayPos-1].iNextprev = pTokT->iMatch;
            
            ASSERT(*pitokTop >= 0);
            *pitokTop -= 1; // pop the stack
        }
        else
        {
LNoMatch:
            int index;

            // look for the first entry down the array that matches
            index = *pitokTop - 1;
            while (index >= 0)
            {
                if (tagID == (pTokStack+index)->tagID)
                {
                    if (tagID == -1) // special case
                    {
                        if (       (   ((pTokStack+index)->token.tok == TokTag_SSSCLOSE) /* faked token for <% */
                                    && ((pTokStack+index)->token.tokClass == tokSSS)
                                    && (token.tok == TokTag_SSSCLOSE)
                                    && (token.tokClass == tokSSS)
                                    )
                                || (   ((pTokStack+index)->token.tok == TokAttrib_ENDSPAN) /* faked token for startspan */
                                    && ((pTokStack+index)->token.tokClass == tokAttr)
                                    && (token.tok == TokAttrib_ENDSPAN)
                                    && (token.tokClass == tokAttr)
                                    )
                                )
                            break;
                        //else actually, this means error case.
                    }
                    else
                        break;
                }
                index--;
            }

            if (index != -1) // match was found at index'th position on the stack
            {
                int i;
                TOKSTACK *pTokIndex = pTokStack + index;

                ASSERT(index >= 0);
                ASSERT(iArrayPos - 1 >= 0);
                
                if (tagID == -1) // special case, match token.tok & token.tokClass
                {
                    ASSERT(    (   (pTokIndex->token.tok == TokTag_SSSCLOSE) /* faked token for <% */
                                && (pTokIndex->token.tokClass == tokSSS)
                                && (token.tok == TokTag_SSSCLOSE)
                                && (token.tokClass == tokSSS)
                                )
                            || (   ((pTokStack+index)->token.tok == TokAttrib_ENDSPAN) /* faked token for startspan */
                                && ((pTokStack+index)->token.tokClass == tokAttr)
                                && (token.tok == TokAttrib_ENDSPAN)
                                && (token.tokClass == tokAttr)
                                )
                            );
                }
                // first of all fill in appropriate iNextprev
                pTokArray[pTokIndex->iMatch].iNextprev = iArrayPos - 1;
                ASSERT(pTokArray[pTokIndex->iMatch].fStart == TRUE);
                pTokArray[iArrayPos-1].iNextprev = pTokIndex->iMatch;

                // now fill in iNextPrevAlternate for all elements from index to *pitokTop - 1
                for (i = index+1; i <= *pitokTop - 1; i++)
                {
                    TOKSTACK *pTokSkip = pTokStack + i;

                    pTokArray[pTokSkip->iMatch].iNextPrevAlternate = iArrayPos - 1;
                    ASSERT(pTokArray[pTokSkip->iMatch].fStart == TRUE);
                    ASSERT(pTokArray[pTokSkip->iMatch].iNextprev == -1);
                } // for ()
                // decrement the stack appropriately
                *pitokTop = index;
            } // else

        } // of if (tagID == pTokT->tagID)
    } // end of if (lxs & inEndTag)
    else // push the token info on the stack
    {
        TOKSTACK *pTokT = pTokStack + *pitokTop;

        ASSERT(iArrayPos - 1 >= 0);
        //push the new token into pTokArray at *pitokCur position
        pTokT->iMatch = iArrayPos - 1;
        pTokT->tagID = tagID;
        pTokT->ichStart = token.ibTokMin;
        pTokT->token = token; // note that this isused ONLY in special cases where tagID is -1

        *pitokTop += 1;
    } //end of else case of if (lxs & inEndTag)

LSkipStackOp:

    return NOERROR;
}



// This function does following
//      (a) reads the stream 
//      (b) generates tokens
//      (c) allocates a buffer that holds replaced elements like DTCs
//      (d) does the parsing of the tokens to build a not-so-tree tree of tokens
//      (e) returns the not-so-tree tree of tokens
// VK 5/19/99: Replaced dwReserved with dwSpecialize.
// This can currently take PARSE_SPECIAL_HEAD_ONLY to terminate parsing at the <BODY>
HRESULT CTriEditParse::hrTokenizeAndParse(HGLOBAL hOld, HGLOBAL *phNew, IStream *pStmNew,
                        DWORD dwFlags, FilterMode mode, 
                        int cbSizeIn, UINT *pcbSizeOut, IUnknown *pUnkTrident, 
                        HGLOBAL *phgTokArray, UINT *pcMaxToken,
                        HGLOBAL *phgDocRestore, BSTR bstrBaseURL, DWORD dwSpecialize)
{
    // FilterRule structure initilization - move this at apporpriate place
    LPSTR pOld, pNew;
    UINT cbOld = 0;
    UINT cbwOld, cchwOld; // number of bytes & chars in the converted unicode string
    UINT cchNew = 0; // number of unicode chars in the new (after filtering) buffer
    HRESULT hrRet = S_OK;
    HGLOBAL hgNew, hgOld, hgTokStack;
    WCHAR *pwOld, *pwNew;
    UINT cbCur = 0; // This is actually the current character position
    TOKSTRUCT *pTokArray;
    TOKSTACK *pTokStack;
    INT itokTop = 0;
    INT itokCur = 0;
    TXTB token;
    INT cStackMax, cArrayMax;
    DWORD lxs = 0; 
    INT tagID;
    BOOL fAllocDocRestore = FALSE; // did we allocate *phgDocRestore locally? (Y/N)
    BOOL fUsePstmNew = (dwFlags & dwFilterUsePstmNew);
    HGLOBAL hgPstm = NULL;
    ULARGE_INTEGER li;
    UINT cbT = 0;
    BOOL fBeginTokSelect; // used by special case code that detects server side scripts inside a SELECT block
    BOOL fBeginTokTextarea; // used by special case code that detects server side scripts inside a TEXTAREA block
    BOOL fBeginTokLabel; // used by special case code that detects server side scripts inside a LABEL block
    BOOL fBeginTokListing; // used by special case code that detects server side scripts inside a LISTING block
    BOOL fInDTCOutput, fInDTC;

#ifdef DEBUG
    DWORD dwErr;
#endif // DEBUG

    ASSERT((PARSE_SPECIAL_NONE == dwSpecialize) || (PARSE_SPECIAL_HEAD_ONLY == dwSpecialize));

	if ( PARSE_SPECIAL_HEAD_ONLY & dwSpecialize )
		ASSERT ( dwFlags == dwFilterNone );

    // NOTE
    // this could be done another way. We can make m_pUnkTrident public member and set its value
    // at the point where the CTriEditParse object is created. But this looks fine too.
    m_pUnkTrident = pUnkTrident; // we cache this for our use.
    m_fUnicodeFile = FALSE;
    m_bstrBaseURL = bstrBaseURL;
    li.LowPart = li.HighPart = 0;
    // Initialize PTDTC related members
    if (mode == modeInput)
    {
        m_fInHdrIn = TRUE;
    }

    if (fUsePstmNew)
        li.LowPart = li.HighPart = 0;

    // initialize <TBODY> related members
    m_hgTBodyStack = NULL;
    m_pTBodyStack = NULL;
    m_iMaxTBody = m_iTBodyMax = 0;

    // initilize members used by PageTransitionDTC
    if (mode == modeInput)
    {
        m_ichPTDTC = m_cchPTDTCObj = m_cchPTDTC = 0;
        m_indexBeginBody = m_indexEndBody = 0;
        m_hgPTDTC = m_pPTDTC = NULL;
    }
    else
    {
        ASSERT(m_hgPTDTC == NULL); // make sure that it was freed (if we allocated it in modeInput case)
    }

    if (mode == modeInput)
    {
        m_fHasTitleIn = FALSE;
        m_indexTitleIn = -1;
        m_ichTitleIn = -1;
        m_cchTitleIn = -1;
        m_ichBeginBodyTagIn = -1;
        m_ichBeginHeadTagIn = -1;
        m_indexHttpEquivIn = -1;
    }
    //initilize fBeginTokSelect (used by special case code that 
    // detects server side scripts inside a SELECT block)
    fBeginTokSelect = fBeginTokTextarea = fBeginTokLabel = fBeginTokListing = FALSE;
    fInDTCOutput = fInDTC = FALSE;

    pOld = (LPSTR) GlobalLock(hOld);
    if (cbSizeIn == -1)
        cbOld = SAFE_INT64_TO_DWORD(GlobalSize(hOld));
    else
        cbOld = cbSizeIn;
    if (cbOld == 0) // zero sized file
    {
        if (pcbSizeOut)
            *pcbSizeOut = 0;
        hrRet = E_OUTOFMEMORY;
        *pcMaxToken = 0;
        if (fUsePstmNew)
            pStmNew->SetSize(li);
        else
            *phNew = NULL;
        *phgTokArray = NULL;
        goto LRetOnly;
    }
    hgNew = hgOld = hgTokStack = NULL;
    if (*((BYTE *)pOld) == 0xff && *((BYTE *)pOld+1) == 0xfe)
    {
        m_fUnicodeFile = TRUE;
        if (dwFlags & dwFilterMultiByteStream)
            dwFlags &= ~dwFilterMultiByteStream;
    }

    // allocate a buffer that will hold token structs. This is returned
    *phgTokArray = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, MIN_TOK*sizeof(TOKSTRUCT)); // stack
    if (*phgTokArray == NULL)
    {
        hrRet = E_OUTOFMEMORY;
        goto LOOM;
    }
    pTokArray = (TOKSTRUCT *) GlobalLock(*phgTokArray);
    ASSERT(pTokArray != NULL);
    cArrayMax = MIN_TOK;

    // allocate temporary buffers that for the current & filtered html documents
    hgTokStack = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, MIN_TOK*sizeof(TOKSTRUCT)); // stack
    if (hgTokStack == NULL)
    {
        hrRet = E_OUTOFMEMORY;
        goto LOOM;
    }
    pTokStack = (TOKSTACK *) GlobalLock(hgTokStack);
    ASSERT(pTokStack != NULL);
    cStackMax = MIN_TOK;

    // In most cases for NON-UNICODE streams, 
    // (cbOld+1/*for NULL*/)*sizeof(WCHAR)  will endup being lot more than what we need
    hgOld = GlobalAlloc(GMEM_ZEROINIT, (dwFlags & dwFilterMultiByteStream) 
                                        ? (cbOld+1/*for NULL*/)*sizeof(WCHAR) 
                                        : (cbOld+2/*for NULL*/));
    if (hgOld == NULL)
    {
        hrRet = E_OUTOFMEMORY;
        goto LOOM;
    }
    pwOld = (WCHAR *) GlobalLock(hgOld);
    ASSERT(pwOld != NULL);

    // we could just allocate cbOld bytes in modeInput and modeOutput. 
    // But reallocs are expensive and in both cases, we will grow by some bytes
    // if we have DTCs and/or SSSs.
    if (dwFlags & dwFilterNone) // the caller has called this function only for tokenizing
    {
        if (dwFlags & dwFilterMultiByteStream)
            cbT = (cbOld+1/*for NULL*/)*sizeof(WCHAR); // this will be bigger than what we need.
        else
            cbT = cbOld + sizeof(WCHAR); // for NULL
    }
    else
    {
        if (dwFlags & dwFilterMultiByteStream)
            cbT = (cbOld+1)*sizeof(WCHAR) + cbBufPadding;
        else
            cbT = cbOld + cbBufPadding; // no need to add +2
    }
    hgNew = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbT);
    if (hgNew == NULL)
    {
        hrRet = E_OUTOFMEMORY;
        goto LOOM;
    }
    pwNew = (WCHAR *) GlobalLock(hgNew);
    ASSERT(pwNew != NULL);

    // buffer to save all contents before/after <BODY> tag
    m_hgDocRestore = phgDocRestore ? *phgDocRestore : NULL;
    if (m_hgDocRestore == NULL)
    {
        fAllocDocRestore = TRUE;
        m_hgDocRestore = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbHeader);
        if (m_hgDocRestore == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            goto LOOM;
        }
    }
    // at this point we know that m_hgDocRestore is not going to be null, but lets be cautious
    // we call FilterIn only once when we load the document. (bug 15393)
    if (m_hgDocRestore != NULL && mode == modeInput)
    {
        WCHAR *pwDocRestore;
        DWORD cbDocRestore;

        // lock
        pwDocRestore = (WCHAR *) GlobalLock(m_hgDocRestore);
        // fill with zeros
        cbDocRestore = SAFE_INT64_TO_DWORD(GlobalSize(m_hgDocRestore));
        memset((BYTE *)pwDocRestore, 0, cbDocRestore);
        // unlock
        GlobalUnlock(m_hgDocRestore);
    }

    m_fEndTagFound = FALSE; // initialize

    m_cMaxToken = m_cDTC = m_cObj = m_cSSSIn = m_cSSSOut = m_cNbsp = m_iControl = m_cComment = m_cObjIn = 0;
    m_cAppletIn = m_cAppletOut = 0;
    m_fSpecialSSS = FALSE;
    m_cHtml = (mode == modeInput)? 0 : 0; // assume that we atleast have one <HTML> tag in modeInput case
    m_cHdr = m_cFtr = m_cAImgLink = 1;
    m_pspInfoCur = m_pspInfo = m_pspInfoOut = m_pspInfoOutStart = NULL;
    m_hgspInfo = NULL;
    m_ichStartSP = 0;
    if (dwFlags & dwPreserveSourceCode)
    {
        m_ispInfoIn = (mode == modeInput)? 1 : 0;
        m_ispInfoOut = (mode == modeOutput)? 1 : 0;
        if (mode == modeInput)
        {
            srand((unsigned)time(NULL));
            m_ispInfoBase = rand();
            if (0x0fffffff-m_ispInfoBase < 0x000fffff)
                m_ispInfoBase  = 0;
        }
    }
    else
    {
        m_ispInfoIn = 0;
        m_ispInfoOut = 0;
    }

    m_iArrayspLast = 0;
    m_ispInfoBlock = 0; // index of the block. stored as value of DESIGNTIMESPx tag
    m_cchspInfoTotal = 0;
    m_fDontDeccItem = FALSE; // we can do this differently next time

    // if we have multiple of these tags, we need to warn the user before going to design view
    // and not let the user switch views (bug 18474)
    m_cBodyTags = m_cHtmlTags = m_cTitleTags = m_cHeadTags = 0;

    if (dwFlags & dwFilterMultiByteStream)
    {
        // note that cbOld is actually number of characters in single byte world
        cchwOld = MultiByteToWideChar(CP_ACP, 0, pOld, (cbSizeIn==-1)?-1:cbOld, NULL, 0);
        MultiByteToWideChar(CP_ACP, 0, pOld, (cbSizeIn==-1)?-1:cbOld, pwOld, cchwOld);
    }
    else
    {
        memcpy((BYTE *)pwOld, (BYTE *)pOld, cbOld); // we are already UNICODE
        // Assume that in UNICODE world we can simply divide cbOld by sizeof(WCHAR)
        cchwOld = cbOld/sizeof(WCHAR);
    }
    *(pwOld+cchwOld) = '\0';

    // get the token & save it into a buffer
    cbwOld = cchwOld * sizeof(WCHAR);
    while (cbCur < cchwOld)
    {
        UINT cbCurSav = cbCur;

        NextToken(pwOld, cchwOld, &cbCur, &lxs, &token);
        tagID = GetTagID(pwOld, token); // only if inAttribute & inTag ????

        // if we have more of any of these tags, Trident removes them, so lets warn the user and
        // not the user go to design view (bug 18474)
        if (   (mode == modeInput)
            && (token.tokClass == tokElem)
            && (lxs & inTag)
            && !(lxs & inEndTag) /* this may be redundant, but having it does no harm */
            )
        {
            switch (token.tok)
            {
            case TokElem_BODY:
                m_cBodyTags++;
                break;
            case TokElem_HTML:
                m_cHtmlTags++;
                break;
            case TokElem_TITLE:
                m_cTitleTags++;
                break;
            case TokElem_HEAD:
                m_cHeadTags++;
                break;
            };
            if (m_cBodyTags > 1 || m_cHtmlTags > 1 || m_cTitleTags > 1 || m_cHeadTags > 1)
            {
                // skip tokenizing. we can't let this go to Trident
                memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
                cchNew = cchwOld;
                hrRet = E_FILTER_MULTIPLETAGS;
                goto LSkipTokFilter;
            }
        }

        if (   (token.tokClass == tokElem)
            && (   (token.tok == TokElem_FRAME)
                || (token.tok == TokElem_FRAMESET)
                )
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_FRAMESET;
            goto LSkipTokFilter;
        }
        if (   (token.tok == TokTag_SSSOPEN)
            && (token.tokClass == tokSSS)
            && (lxs & inAttribute)
            && !(lxs & inString)
            && !(lxs & inStringA)
            && !(fInDTCOutput)
            && (mode == modeInput)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SERVERSCRIPT;
            goto LSkipTokFilter;
        }
        if (   (token.tok == 0)
            && (token.tokClass == tokSSS)
            && (lxs & inTag)
            && (lxs & inHTXTag)
            && (lxs & inAttribute)
            && (lxs & inString || lxs & inStringA)
            && (lxs & inNestedQuoteinSSS)
            && !(fInDTCOutput)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SERVERSCRIPT;
            goto LSkipTokFilter;
        }

        // REVIEW TODO LATER - For all following special cases, we need to add !fInDTCOutput
        if (   (fBeginTokSelect)
            && (token.tok == TokTag_SSSOPEN || token.tok == TokElem_SCRIPT)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SCRIPTSELECT;
            goto LSkipTokFilter;
        }

        if (   (fBeginTokTextarea)
            && (token.tok == TokTag_SSSOPEN && token.tokClass == tokSSS)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SCRIPTTEXTAREA;
            goto LSkipTokFilter;
        }
        if (   (fBeginTokLabel)
            && (token.tok == TokTag_SSSOPEN && token.tokClass == tokSSS)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SCRIPTLABEL;
            goto LSkipTokFilter;
        }
        if (   (fBeginTokListing)
            && (token.tok == TokTag_SSSOPEN && token.tokClass == tokSSS)
            )
        {
            // skip tokenizing. we can't let this go to Trident
            memcpy((BYTE *)pwNew, (BYTE *)pwOld, cchwOld*sizeof(WCHAR));
            cchNew = cchwOld;
            hrRet = E_FILTER_SCRIPTLISTING;
            goto LSkipTokFilter;
        }

        // Special cases Begin

        // special case - check if the document has <!DOCTYPE before going to Design view.
        // If it does, set m_fHasDocType flag. Trident always inserts this flag and we
        // want to remove it on the way out from Design view.
        if (   (token.tok == TokElem_TITLE)
            && (token.tokClass == tokElem)
            )
        {
            if (mode == modeInput)
            {
                m_fHasTitleIn = TRUE;
                if (m_indexTitleIn == -1)
                    m_indexTitleIn = itokCur;
            }
        }

        if (   (token.tok == TokElem_BODY)
            && (token.tokClass == tokElem)
            && (m_ichBeginBodyTagIn == -1)
            )
		{
			if ( PARSE_SPECIAL_HEAD_ONLY & dwSpecialize )
				break;
			if (mode == modeInput)
				m_ichBeginBodyTagIn = token.ibTokMin;
		}

        if (   (token.tok == TokAttrib_HTTPEQUIV || token.tok == TokAttrib_HTTP_EQUIV)
            && (token.tokClass == tokAttr)
            && (mode == modeInput)
            )
        {
            if (m_indexHttpEquivIn == -1)
                m_indexHttpEquivIn = itokCur;
        }
        if (   (token.tok == TokElem_HEAD)
            && (token.tokClass == tokElem)
            && (m_ichBeginHeadTagIn == -1)
            && (mode == modeInput)
            )
        {
            m_ichBeginHeadTagIn = token.ibTokMin;
        }
        if (   (token.tok == TokElem_SELECT)
            && (token.tokClass == tokElem)
            && (mode == modeInput)
            && !(lxs & inSCRIPT)
            )
        {
            if (   (pTokArray[itokCur-1].token.tok == TokTag_START)
                && (pTokArray[itokCur-1].token.tokClass == tokTag)
                )
                fBeginTokSelect = TRUE;
            else if (      (pTokArray[itokCur-1].token.tok == TokTag_END)
                        && (pTokArray[itokCur-1].token.tokClass == tokTag)
                        )
                fBeginTokSelect = FALSE;
        }
        if (   (token.tok == TokElem_TEXTAREA)
            && (token.tokClass == tokElem)
            && (mode == modeInput)
            )
        {
            if (   (pTokArray[itokCur-1].token.tok == TokTag_START)
                && (pTokArray[itokCur-1].token.tokClass == tokTag)
                )
                fBeginTokTextarea = TRUE;
            else if (      (pTokArray[itokCur-1].token.tok == TokTag_END)
                        && (pTokArray[itokCur-1].token.tokClass == tokTag)
                        )
                fBeginTokTextarea = FALSE;
        }
        if (   (token.tok == TokElem_LABEL)
            && (token.tokClass == tokElem)
            && (mode == modeInput)
            )
        {
            if (   (pTokArray[itokCur-1].token.tok == TokTag_START)
                && (pTokArray[itokCur-1].token.tokClass == tokTag)
                )
                fBeginTokLabel = TRUE;
            else if (      (pTokArray[itokCur-1].token.tok == TokTag_END)
                        && (pTokArray[itokCur-1].token.tokClass == tokTag)
                        )
                fBeginTokLabel = FALSE;
        }
        if (   (token.tok == TokElem_LISTING)
            && (token.tokClass == tokElem)
            && (mode == modeInput)
            )
        {
            if (   (pTokArray[itokCur-1].token.tok == TokTag_START)
                && (pTokArray[itokCur-1].token.tokClass == tokTag)
                )
                fBeginTokListing = TRUE;
            else if (      (pTokArray[itokCur-1].token.tok == TokTag_END)
                        && (pTokArray[itokCur-1].token.tokClass == tokTag)
                        )
                fBeginTokListing = FALSE;
        }

        if (   (token.tok == TokAttrib_STARTSPAN)
            && (token.tokClass == tokAttr)
            && (mode == modeInput)
            )
            fInDTC = TRUE;
        if (   (token.tok == TokElem_OBJECT)
            && (token.tokClass == tokElem)
            && (lxs & inEndTag)
            && (fInDTC)
            && (mode == modeInput)
            )
            fInDTCOutput = TRUE;
        if (   (token.tok == TokAttrib_ENDSPAN)
            && (token.tokClass == tokAttr)
            && (mode == modeInput)
            )
        {
            fInDTCOutput = FALSE;
            fInDTC = FALSE;
        }
        // Special cases End


        if (itokCur == cArrayMax - 1) //allocate more memory for the array
        {
            GlobalUnlock(*phgTokArray);
            *phgTokArray = GlobalReAlloc(*phgTokArray, (cArrayMax+MIN_TOK)*sizeof(TOKSTRUCT), GMEM_MOVEABLE|GMEM_ZEROINIT);
            // if this alloc failed, we may still want to continue
            if (*phgTokArray == NULL)
            {
                hrRet = E_OUTOFMEMORY;
                *pcMaxToken = itokCur;
                if (fUsePstmNew)
                    pStmNew->SetSize(li);
                else
                    *phNew = NULL;
                goto LOOM;
            }
            else
            {
                pTokArray = (TOKSTRUCT *)GlobalLock(*phgTokArray); // do we need to unlock this first?
                ASSERT(pTokArray != NULL);
                cArrayMax += MIN_TOK;
            }
        }
        ASSERT(itokCur < cArrayMax);
        PreProcessToken(pTokArray, &itokCur, pwOld, cbCur, token, lxs, tagID, mode); //saves the token into the buffer


        if (itokTop == cStackMax - 1) //allocate more memory for the stack
        {
            GlobalUnlock(hgTokStack);
            hgTokStack = GlobalReAlloc(hgTokStack, (cStackMax+MIN_TOK)*sizeof(TOKSTACK), GMEM_MOVEABLE|GMEM_ZEROINIT);
            // if this alloc failed, we may still want to continue
            if (hgTokStack == NULL)
            {
                hrRet = E_OUTOFMEMORY;
                *pcMaxToken = itokCur;
                if (fUsePstmNew)
                    pStmNew->SetSize(li);
                else
                    *phNew = NULL;
                goto LOOM;
            }
            else
            {
                pTokStack = (TOKSTACK *)GlobalLock(hgTokStack); // do we need to unlock this first?
                ASSERT(pTokStack != NULL);
                cStackMax += MIN_TOK;
            }
        }
        ASSERT(itokTop < cStackMax);
        ProcessToken(lxs, token, pwOld, cbCur, pTokStack, &itokTop, pTokArray, itokCur, tagID); //push/pop stack, determine error states

        PostProcessToken(pwOld, pwNew, &cchNew, cbCur, cbCurSav, token, mode, lxs, dwFlags); // handle special cases of replacement 
    } // while (cbCur < cchwOld)
    *pcMaxToken = m_cMaxToken = itokCur;
    ASSERT(cchNew < GlobalSize(hgNew)); // or compare the cached value

    ASSERT(dwFlags != dwFilterDefaults);
    if (       dwFlags & dwFilterDTCs
            || dwFlags & dwFilterDTCsWithoutMetaTags
            || dwFlags & dwFilterServerSideScripts
            || dwFlags & dwPreserveSourceCode
            )
    {
        ASSERT(!(dwFlags & dwFilterNone));


        
        // check dwSpacing flag here
        if ((mode == modeOutput) && (dwFlags & dwPreserveSourceCode))
        {
            INT cchBeforeBody = 0;
            INT cchAfterBody = 0;
            INT cchPreEndBody = 0;

            ASSERT(m_pspInfoOut == NULL);
            ASSERT(m_hgDocRestore != NULL);
            m_pspInfoOut = (WORD *)GlobalLock(m_hgDocRestore);
            cchBeforeBody = (int)*m_pspInfoOut; // we are assuming that cchBeforeBody exists in this block
            m_pspInfoOut += cchBeforeBody + (sizeof(INT))/sizeof(WCHAR); // for cchBeforeBody
            cchAfterBody = (int)*m_pspInfoOut;
            m_pspInfoOut += cchAfterBody + (sizeof(INT))/sizeof(WCHAR); // for cchAfterBody
            cchPreEndBody = (int)*m_pspInfoOut;
            m_pspInfoOut += cchPreEndBody + (sizeof(INT))/sizeof(WCHAR); // for cchPreEndBody
            m_cchspInfoTotal = (int)*m_pspInfoOut;
            m_pspInfoOut += sizeof(INT)/sizeof(WCHAR);
            m_pspInfoOutStart = m_pspInfoOut;
        }

        
        ASSERT(pTokArray != NULL);
        FilterHtml( pwOld, &pwNew, &cchNew, &hgNew, pTokArray, 
                    mode, dwFlags);
        
        // check dwSpacing flag here
        if ((mode == modeOutput) && (dwFlags & dwPreserveSourceCode))
        {
            if (m_pspInfoOut != NULL)
            {
                ASSERT(m_hgDocRestore != NULL);
                GlobalUnlock(m_hgDocRestore);
            }
        }

    }

LSkipTokFilter:

    if (fUsePstmNew)
    {
        if (dwFlags & dwFilterMultiByteStream)
            li.LowPart = WideCharToMultiByte(CP_ACP, 0, pwNew, -1, NULL, 0, NULL, NULL) - 1; // to compensate for NULL character at end
        else
            li.LowPart = (cchNew)*sizeof(WCHAR);
        li.HighPart = 0;
        if (S_OK != pStmNew->SetSize(li))
        {
            hrRet = E_OUTOFMEMORY;
            goto LOOM;
        }
        if (S_OK != GetHGlobalFromStream(pStmNew, &hgPstm))
        {
            hrRet = E_INVALIDARG;
            goto LOOM;
        }
        pNew = (LPSTR) GlobalLock(hgPstm);
    }
    else
    {
        // cchNew is # of unicode characters in pwNew
        // If we want to convert this UNICODE string into MultiByte string, 
        // we will need anywhere between cchNew bytes & cchNew*sizeof(WCHAR) bytes.
        // and we don't know it at this point, so lets leave the max size for allocation.
        *phNew = GlobalAlloc(GMEM_ZEROINIT, (cchNew+1)*sizeof(WCHAR));
        if (*phNew == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            goto LOOM;
        }
        pNew = (LPSTR) GlobalLock(*phNew);
    }

    if (dwFlags & dwFilterMultiByteStream)
    {
        INT cbSize;

        cbSize = WideCharToMultiByte(CP_ACP, 0, pwNew, -1, NULL, 0, NULL, NULL) - 1; // to compensate for NULL character at end
        if (pcbSizeOut)
            *pcbSizeOut = cbSize;
        // we assume that number of characters will be the same in UNICODE or MBCS world
        // what changes is the number of bytes they need.
        WideCharToMultiByte(CP_ACP, 0, pwNew, -1, pNew, cbSize, NULL, NULL);
    }
    else
    {
        // NOTE - that we always set *pcbSizeOut to the number of BYTES in the new buffer
        if (pcbSizeOut)
            *pcbSizeOut = cchNew*sizeof(WCHAR);
        memcpy((BYTE *)pNew, (BYTE *)pwNew, cchNew*sizeof(WCHAR)); // we want to remain UNICODE
    }

#ifdef DEBUG
    dwErr = GetLastError();
#endif // DEBUG
    
    if (fUsePstmNew)
        GlobalUnlock(hgPstm);
    else
        GlobalUnlock(*phNew);

LOOM:
    // assume that the caller will free *phgTokArray
    if (*phgTokArray != NULL)
        GlobalUnlock(*phgTokArray); // do we need to check if this was already Unlocked?
    
    // assume that the caller will free *phgDocRestore if the caller allocated it
    if (fAllocDocRestore && m_hgDocRestore != NULL) // we allocated it here, so the caller doesn't need it
        GlobalUnlockFreeNull(&m_hgDocRestore);

    if (phgDocRestore)
        *phgDocRestore = m_hgDocRestore; // in case of a realloc, this may have changed.

    if (hgTokStack != NULL)
        GlobalUnlockFreeNull(&hgTokStack);
    if (hgNew != NULL)
        GlobalUnlockFreeNull(&hgNew);
    if (hgOld != NULL)
        GlobalUnlockFreeNull(&hgOld);
    if (m_hgTBodyStack != NULL)
        GlobalUnlockFreeNull(&m_hgTBodyStack);

    // check dwSpacing flag here
    if ((m_hgspInfo != NULL) && (dwFlags & dwPreserveSourceCode))
    {
        if (mode == modeInput && phgDocRestore)
        {
            WCHAR *pHdr, *pHdrSav;
            INT cchBeforeBody, cchAfterBody, cchPreEndBody;

            pHdr = (WCHAR *)GlobalLock(*phgDocRestore);
            ASSERT(pHdr != NULL);
            pHdrSav = pHdr;
            memcpy((BYTE *)&cchBeforeBody, (BYTE *)pHdr, sizeof(INT));
            pHdr += cchBeforeBody + sizeof(INT)/sizeof(WCHAR);

            memcpy((BYTE *)&cchAfterBody, (BYTE *)pHdr, sizeof(INT));
            pHdr += cchAfterBody + sizeof(INT)/sizeof(WCHAR);

            memcpy((BYTE *)&cchPreEndBody, (BYTE *)pHdr, sizeof(INT));
            pHdr += cchPreEndBody + sizeof(INT)/sizeof(WCHAR);


            if (GlobalSize(*phgDocRestore) < SAFE_PTR_DIFF_TO_INT(pHdr - pHdrSav)*sizeof(WCHAR) + SAFE_PTR_DIFF_TO_INT(m_pspInfoCur-m_pspInfo)*sizeof(WORD)+sizeof(int))
            {
                INT cdwSize = SAFE_PTR_DIFF_TO_INT(pHdr - pHdrSav);

                ASSERT(cdwSize >= 0); // validation
                hrRet = ReallocBuffer(  phgDocRestore,
                                        SAFE_INT64_TO_DWORD(pHdr - pHdrSav)*sizeof(WCHAR) + SAFE_INT64_TO_DWORD(m_pspInfoCur-m_pspInfo)*sizeof(WORD)+sizeof(int),
                                        GMEM_MOVEABLE|GMEM_ZEROINIT);
                if (hrRet == E_OUTOFMEMORY)
                    goto LRet;
                ASSERT(*phgDocRestore != NULL);
                pHdr = (WORD *)GlobalLock(*phgDocRestore);
                pHdr += cdwSize;
            }
            
            *(int*)pHdr = SAFE_PTR_DIFF_TO_INT(m_pspInfoCur-m_pspInfo);
            pHdr += sizeof(INT)/sizeof(WCHAR);

            memcpy( (BYTE *)pHdr,
                    (BYTE *)m_pspInfo,
                    SAFE_PTR_DIFF_TO_INT(m_pspInfoCur-m_pspInfo)*sizeof(WORD));
LRet:
            GlobalUnlock(*phgDocRestore);
        }
        GlobalUnlockFreeNull(&m_hgspInfo);
    }


    GlobalUnlock(hOld);
LRetOnly:
    return(hrRet);

}

void 
CTriEditParse::SetSPInfoState(WORD inState, WORD *pdwState, WORD *pdwStatePrev, BOOL *pfSave)
{
    *pfSave = TRUE;
    *pdwStatePrev = *pdwState;
    *pdwState = inState;
}

HRESULT
CTriEditParse::hrMarkOrdering(WCHAR *pwOld, TOKSTRUCT *pTokArray, INT iArrayStart, int iArrayEnd, 
                              UINT cbCur, INT *pichStartOR)
{

    HRESULT hr = S_OK;
    WORD *pspInfoSize;
    WORD cAttr = 0;

    ASSERT(m_pspInfo != NULL);
    if (m_pspInfo == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto LRetOnly;
    }

    pspInfoSize = m_pspInfoCur; // placeholder to save run size in BYTEs, size includes this DWORD
    *m_pspInfoCur++ = 0xFFFF; // placeholder to save run size in BYTEs, size includes this WORD
    *m_pspInfoCur++ = 0xFFFF; // placeholder to save number of Attr

    // handle the simplest case where we know that there is nothing to save
    if (cbCur == (UINT)*pichStartOR)
        goto LRet;
    // find out the number ot attributes insize this tag
    while (iArrayStart < iArrayEnd)
    {
        if (pTokArray[iArrayStart].token.tokClass == tokAttr)
        {
            INT ichStart, ichEnd;
            INT iArrayQuote = iArrayStart+1;
            INT iArrayEq = -1;

            cAttr++;
            ichStart = pTokArray[iArrayStart].token.ibTokMin;
            ichEnd = pTokArray[iArrayStart].token.ibTokMac;
            ASSERT(ichEnd > ichStart);
            
            while (iArrayQuote < iArrayEnd) // handle the case of white space before the quotes
            {
                if (pTokArray[iArrayQuote].token.tokClass == tokAttr) // gone too far, found next attr
                    break;
                if (   (   pTokArray[iArrayQuote].token.tokClass == tokValue
                        || pTokArray[iArrayQuote].token.tokClass == tokString
                        )
                    && (   pwOld[pTokArray[iArrayQuote].token.ibTokMin] == '"'
                        || pwOld[pTokArray[iArrayQuote].token.ibTokMin] == '\''
                        )
                    )
                    break;
                if (pwOld[pTokArray[iArrayQuote].token.ibTokMin] == '=')
                    iArrayEq = iArrayQuote;
                iArrayQuote++;
            }

            if (   iArrayEq != -1
                && pTokArray[iArrayEq].token.tokClass == tokOp
                && pwOld[pTokArray[iArrayEq].token.ibTokMin] == '='
                && (   pTokArray[iArrayQuote].token.tokClass == tokValue
                    || pTokArray[iArrayQuote].token.tokClass == tokString
                    )
                && pwOld[pTokArray[iArrayQuote].token.ibTokMin] == '"'
                )
            {
                *m_pspInfoCur++ = 1;
            }
            else if (   iArrayEq != -1
                && pTokArray[iArrayEq].token.tokClass == tokOp
                && pwOld[pTokArray[iArrayEq].token.ibTokMin] == '='
                && (   pTokArray[iArrayQuote].token.tokClass == tokValue
                    || pTokArray[iArrayQuote].token.tokClass == tokString
                    )
                && pwOld[pTokArray[iArrayQuote].token.ibTokMin] == '\''
                )
            {
                *m_pspInfoCur++ = 2;
            }
            else
            {
                *m_pspInfoCur++ = 0;
            }
            *m_pspInfoCur++ = (WORD)(ichEnd-ichStart);
            memcpy((BYTE *)m_pspInfoCur, (BYTE *)&(pwOld[ichStart]), (ichEnd-ichStart)*sizeof(WCHAR));
            m_pspInfoCur += (ichEnd-ichStart);
        }
        iArrayStart++;
    }

LRet:
    *pspInfoSize++ = SAFE_PTR_DIFF_TO_WORD(m_pspInfoCur - pspInfoSize);
    *pspInfoSize = cAttr;

    *pichStartOR = cbCur; // set for next run
LRetOnly:
    return(hr);

} /* hrMarkOrdering() */

BOOL
CTriEditParse::FRestoreOrder(WCHAR *pwNew, WCHAR *pwOld, WORD *pspInfoOrder, UINT *pichNewCur, 
                             INT /*cwOrderInfo*/, TOKSTRUCT *pTokArray, INT iArrayStart, INT iArrayEnd, 
                             INT iArrayDSPStart, INT iArrayDSPEnd, INT cchNewCopy, HGLOBAL *phgNew)
{
    // iArrayStart points to '<' & iArrayEnd points to '>'. (These refer to pwOld)
    // look at the attributes between iArrayStart & iArrayEnd and compare them with the attributes
    // saved in pspInfoOrder (which already points to the data saved, i.e. past cwOrderInfo)
    // If we find a matching attribute, move it to appropriate position.
    // DON'T touch extra attributes and IGNORE missing attributes because those represent user action

    HGLOBAL hgNewAttr = NULL;
    HGLOBAL hgTokList = NULL;
    BOOL *pTokList, *pTokListSav;
    WCHAR *pNewAttr, *pNewAttrSav;
    INT i, ichStart, ichEnd, iStart, iEnd, cAttr, cchTag, iStartSav, cchNew;
    BOOL fRet = TRUE;
    LPCWSTR rgSpaceTags[] =
    {
        L"DESIGNTIMESP",
    };
    UINT cbNeed;
    
    ASSERT(pspInfoOrder != NULL);
    cAttr = *(WORD *)pspInfoOrder++;
    ASSERT(cAttr >= 0); // make sure that it was filled in
    if (cAttr == 0)/* || cAttr == 1)*/
        goto LRet;

    hgTokList = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (iArrayEnd-iArrayStart+1)*(sizeof(BOOL)));
    if (hgTokList == NULL) // don't reorder the attributes
    {
        fRet = FALSE;
        goto LRet;
    }
    pTokList = (BOOL *) GlobalLock(hgTokList);
    pTokListSav = pTokList;

    ichStart = pTokArray[iArrayStart].token.ibTokMin;
    ichEnd = pTokArray[iArrayEnd].token.ibTokMac;
    // cAttr*2 becase we may need to add quotes around each attr value
    hgNewAttr = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (ichEnd-ichStart+cAttr*2)*(sizeof(WCHAR)));
    if (hgNewAttr == NULL) // don't reorder the attributes
    {
        fRet = FALSE;
        goto LRet;
    }
    pNewAttr = (WCHAR *) GlobalLock(hgNewAttr);
    pNewAttrSav = pNewAttr;

    for (i = iArrayStart; i <= iArrayEnd; i++)
        *pTokList++ = FALSE;

    ASSERT(iArrayDSPEnd > iArrayDSPStart);
    for (i = iArrayDSPStart; i <= iArrayDSPEnd; i++)
    {
        ASSERT(*(pTokListSav+i-iArrayStart) == FALSE);
        *(pTokListSav+i-iArrayStart) = TRUE;
    }
    if (pwOld[pTokArray[iArrayDSPEnd+1].token.ibTokMin] == ' ')
    {
        ASSERT(*(pTokListSav+iArrayDSPEnd+1-iArrayStart) == FALSE);
        *(pTokListSav+iArrayDSPEnd+1-iArrayStart) = TRUE;
    }
    // copy contents from pwOld into pNewAttr till we find the first tokAttr/tokSpace
    iStart = iEnd = iArrayStart;
    cchTag = wcslen(rgSpaceTags[0]);
    while (iEnd < iArrayEnd)
    {
        if (   (pTokArray[iEnd].token.tokClass == tokAttr)
            /*|| (   (pTokArray[iEnd].token.tokClass == tokSpace)
                && (0 != _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[iEnd].token.ibTokMin], cchTag))
                )*/
            )
        {
            break;
        }
        iEnd++;
    }
    if (iEnd >= iArrayEnd) // error
    {
        fRet = FALSE;
        goto LRet;
    }

    for (i = iStart; i < iEnd; i++)
    {
        if (*(pTokListSav+i-iArrayStart) != TRUE) // if not already copied
        {
            if (       (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin == 3)
                    && pwOld[pTokArray[i].token.ibTokMin] == ' '
                    && pwOld[pTokArray[i].token.ibTokMin+1] == '\r'
                    && pwOld[pTokArray[i].token.ibTokMin+2] == '\n'
                    )
            {
                memcpy( (BYTE *)pNewAttr,
                        (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                        (1)*sizeof(WCHAR));
                pNewAttr++;
            }
            else
            {
                if (pTokArray[i].token.tokClass == tokElem)
                {
                    memcpy( (BYTE *)pNewAttr,
                            (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    // BUG 15389 - restore proper case here
                    if (iswupper(pwOld[pTokArray[iArrayDSPStart].token.ibTokMin]) != 0) // DESIGNTIMESP is upper case
                    {
                        _wcsupr(pNewAttr);
                    }
                    else
                    {
                        _wcslwr(pNewAttr);
                    }
                    pNewAttr += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
                else
                {
                    memcpy( (BYTE *)pNewAttr,
                            (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                            (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    pNewAttr += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
            }
            *(pTokListSav+i-iArrayStart) = TRUE;
        }
    }

    iStartSav = iStart = iEnd;
    while (cAttr > 0)
    {
        INT cchAttr;
        BOOL fAddSpace;
        WORD isQuote;

        isQuote = *(WORD *)pspInfoOrder++;
        cchAttr = *(WORD *)pspInfoOrder++;
        ASSERT(cchAttr > 0); // make sure that it was filled in
        
        while (iStart <= iArrayEnd) //for (i = iStart; i <= iArrayEnd; i++)
        {
            if (   (pTokArray[iStart].token.tokClass == tokAttr)
                && (pTokArray[iStart].token.ibTokMac-pTokArray[iStart].token.ibTokMin == (UINT)cchAttr)
                && (0 == _wcsnicmp(pspInfoOrder, &pwOld[pTokArray[iStart].token.ibTokMin], cchAttr))
                )
            {
                break; // found the match, so copy from ith token to the next tokAttr
            }
            iStart++;
        } // while ()
        if (iStart >= iArrayEnd) // we know that iArrayEnd is actually '>'
            goto LNoMatch;

        // now from iStart go forward till we get the next tokAttr or '>'
        iEnd = iStart+1;
        fAddSpace = FALSE;
        while (iEnd < iArrayEnd)
        {
            if (       (pTokArray[iEnd].token.tokClass == tokAttr)
                    || (       (pTokArray[iEnd].token.tokClass == tokSpace)
                            && (0 == _wcsnicmp(rgSpaceTags[0], &pwOld[pTokArray[iEnd].token.ibTokMin], cchTag))
                            )
                    )
                break; // found the next attribute
            iEnd++;
        }
        if (iEnd == iArrayEnd)
            fAddSpace = TRUE;
        iEnd--; // iEnd will be pointing to '>' or the next Attribute, so decrement it

        for (i = iStart; i <= iEnd; i++)
        {
            if (*(pTokListSav+i-iArrayStart) != TRUE) // we didn't copy this token
            {
                if (       (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin == 3)
                        && pwOld[pTokArray[i].token.ibTokMin] == ' '
                        && pwOld[pTokArray[i].token.ibTokMin+1] == '\r'
                        && pwOld[pTokArray[i].token.ibTokMin+2] == '\n'
                        )
                {
                    memcpy( (BYTE *)pNewAttr,
                            (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                            (1)*sizeof(WCHAR));
                    pNewAttr++;
                }
                else
                {
                    if (pTokArray[i].token.tokClass == tokAttr)
                    {
                        ASSERT(i == iStart);
                        ASSERT((INT)(pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin) == cchAttr);
                        ASSERT(0 == _wcsnicmp(pspInfoOrder, &pwOld[pTokArray[i].token.ibTokMin], cchAttr));
                        memcpy( (BYTE *)pNewAttr,
                                (BYTE *)pspInfoOrder,
                                (cchAttr)*sizeof(WCHAR));
                    }
                    else if (      (isQuote == 1)
                                && (   pTokArray[i].token.tokClass == tokValue
                                    || pTokArray[i].token.tokClass == tokString
                                    )
								&& (pwOld[pTokArray[i-1].token.ibTokMin] != '@') /*hack alert - VID BUG 23597*/
                                )
                    {
                        isQuote = 0; // the quote restoring has been taken care of for this attribute's value
                        if (pwOld[pTokArray[i].token.ibTokMin] != '"')
                            *pNewAttr++ = '"';
                        memcpy( (BYTE *)pNewAttr,
                                (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                        if (pwOld[pTokArray[i].token.ibTokMin] != '"')
                        {
                            *(pNewAttr+pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin) = '"';
                            pNewAttr++;
                        }
                    }
                    else if (      (isQuote == 2)
                                && (   pTokArray[i].token.tokClass == tokValue
                                    || pTokArray[i].token.tokClass == tokString
                                    )
								&& (pwOld[pTokArray[i-1].token.ibTokMin] != '@') /*hack alert - VID BUG 23597*/
                                )
                    {
                        isQuote = 0; // the quote restoring has been taken care of for this attribute's value
                        // if we already have double quote, don't insert another single quote.
                        // ideally, we want to replace the double quote, but lets not do it now, because
                        // we believe that trident would have inserted double quotes to make it valid html!
                        if (pwOld[pTokArray[i].token.ibTokMin] != '\'' && pwOld[pTokArray[i].token.ibTokMin] != '"')
                            *pNewAttr++ = '\'';
                        memcpy( (BYTE *)pNewAttr,
                                (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                        if (pwOld[pTokArray[i].token.ibTokMin] != '\'' && pwOld[pTokArray[i].token.ibTokMin] != '"')
                        {
                            *(pNewAttr+pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin) = '\'';
                            pNewAttr++;
                        }
                    }
                    else
                    {
                        memcpy( (BYTE *)pNewAttr,
                                (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                                (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                    }
                    pNewAttr += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
                }
                *(pTokListSav+i-iArrayStart) = TRUE;
            }
        }
        if (fAddSpace)
            *pNewAttr++ = ' ';

LNoMatch:
        iStart = iStartSav;
        pspInfoOrder += cchAttr;
        cAttr--;
    } // while (cAttr > 0)

    // do we want to insert an extra space into pNewAttr here?

    // all the saved attributes are accounted for, lets copy remaining stuff
    for (i = iStartSav; i <= iArrayEnd; i++)
    {
        if (*(pTokListSav+i-iArrayStart) != TRUE) // we didn't copy this token
        {
            if (       (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin == 3)
                    && pwOld[pTokArray[i].token.ibTokMin] == ' '
                    && pwOld[pTokArray[i].token.ibTokMin+1] == '\r'
                    && pwOld[pTokArray[i].token.ibTokMin+2] == '\n'
                    )
            {
                memcpy( (BYTE *)pNewAttr,
                        (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                        (1)*sizeof(WCHAR));
                pNewAttr++;
            }
            else
            {
                memcpy( (BYTE *)pNewAttr,
                        (BYTE *)(pwOld+pTokArray[i].token.ibTokMin),
                        (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin)*sizeof(WCHAR));
                pNewAttr += (pTokArray[i].token.ibTokMac-pTokArray[i].token.ibTokMin);
            }
            *(pTokListSav+i-iArrayStart) = TRUE;
        }
    } // for ()
    cchNew = SAFE_PTR_DIFF_TO_INT(pNewAttr - pNewAttrSav);

    cbNeed = *pichNewCur+cchNew-cchNewCopy;
    ASSERT(cbNeed*sizeof(WCHAR) <= GlobalSize(*phgNew));
    if (S_OK != ReallocIfNeeded(phgNew, &pwNew, cbNeed, GMEM_MOVEABLE|GMEM_ZEROINIT))
    {
        fRet = FALSE;
        goto LRet;
    }

    memcpy( (BYTE *)(pwNew+*pichNewCur-cchNewCopy),
            (BYTE *)(pNewAttrSav),
            cchNew*sizeof(WCHAR));
    *pichNewCur += (cchNew-cchNewCopy);

    // NOTE - Find a better way to account for the extra space added when we moved
    // the attributes. We can't avoid adding space because when we move the last attribute, 
    // there may not be a space between that and the '>'. 
    if (       /*(cchNew > cchNewCopy)
            &&*/ (pwNew[*pichNewCur-1] == '>' && pwNew[*pichNewCur-2] == ' ')
            )
    {
        pwNew[*pichNewCur-2] = pwNew[*pichNewCur-1];
        pwNew[*pichNewCur-1] = '\0';
        *pichNewCur -= 1;
    }

LRet:
    if (hgNewAttr != NULL)
        GlobalUnlockFreeNull(&hgNewAttr);
    if (hgTokList != NULL)
        GlobalUnlockFreeNull(&hgTokList);

    return(fRet);

} /* FRestoreOrder() */

HRESULT
CTriEditParse::hrMarkSpacing(WCHAR *pwOld, UINT cbCur, INT *pichStartSP)
{

    HRESULT hrRet = S_OK;
    UINT i;
    WORD cSpace, cEOL, cTab, cChar, cTagOpen, cTagClose, cTagEq;
    WORD dwState = initState;
    WORD dwStatePrev = initState;
    BOOL fSave = FALSE;
    WORD *pspInfoSize;
    
    if (m_pspInfo == NULL) // allocate it
    {
        m_hgspInfo = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, cbHeader*sizeof(WORD));
        if (m_hgspInfo == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            goto LRet;
        }
        m_pspInfo = (WORD *) GlobalLock(m_hgspInfo);
        ASSERT(m_pspInfo != NULL);
        m_pspInfoCur = m_pspInfo;
        //ASSERT(m_ispInfoIn == 0);
    }
    else // reallocate if needed
    {
        // assumption here is that we can't have more runs than the number of characters we have to scan
        // we use *2 to reduce future reallocations
        if (GlobalSize(m_hgspInfo) < SAFE_PTR_DIFF_TO_INT(m_pspInfoCur-m_pspInfo)*sizeof(WORD) + (cbCur-*pichStartSP)*2*sizeof(WORD) + cbBufPadding)
        {
            int cdwSize = SAFE_PTR_DIFF_TO_INT(m_pspInfoCur-m_pspInfo); // size in DWORDs

            hrRet = ReallocBuffer(  &m_hgspInfo,
                                    SAFE_INT64_TO_DWORD(m_pspInfoCur-m_pspInfo)*sizeof(WORD) + (cbCur-*pichStartSP)*2*sizeof(WORD) + cbBufPadding,
                                    GMEM_MOVEABLE|GMEM_ZEROINIT);
            if (hrRet == E_OUTOFMEMORY)
                goto LRet;
            ASSERT(m_hgspInfo != NULL);
            m_pspInfo = (WORD *)GlobalLock(m_hgspInfo);
            m_pspInfoCur = m_pspInfo + cdwSize;
        }
    }

    //m_ispInfoIn++;
    pspInfoSize = m_pspInfoCur; // placeholder to save run size in BYTEs, size includes this DWORD
    *m_pspInfoCur++ = 0xFFFF; // placeholder to save run size in BYTEs, size includes this WORD
    *m_pspInfoCur++ = SAFE_INT_DIFF_TO_WORD(cbCur-*pichStartSP);
    cSpace = cEOL = cTab = cChar = cTagOpen = cTagClose = cTagEq = 0;
    
    //scan from ichStartSP till cbCur for space, tab, eol
    // NOTE - Optimization note
    // part of this info is already in pTokArray. We should use it
    // to reduce the time in this function
    for (i = *pichStartSP; i < cbCur; i++)
    {
        switch (pwOld[i])
        {
        case ' ':
            if (dwState != inSpace)
            {
                SetSPInfoState(inSpace, &dwState, &dwStatePrev, &fSave);
                ASSERT(cSpace == 0);
            }
            cSpace++;
            break;
        case '\r':
        case '\n':
            if (dwState != inEOL)
            {
                SetSPInfoState(inEOL, &dwState, &dwStatePrev, &fSave);
                ASSERT(cEOL == 0);
            }
            if (pwOld[i] == '\n')
                cEOL++;
            break;
        case '\t':
            if (dwState != inTab)
            {
                SetSPInfoState(inTab, &dwState, &dwStatePrev, &fSave);
                ASSERT(cTab == 0);
            }
            cTab++;
            break;
        case '<':
            if (dwState != inTagOpen)
            {
                SetSPInfoState(inTagOpen, &dwState, &dwStatePrev, &fSave);
                ASSERT(cTagOpen == 0);
            }
            cTagOpen++;
            break;
        case '>':
            if (dwState != inTagClose)
            {
                SetSPInfoState(inTagClose, &dwState, &dwStatePrev, &fSave);
                ASSERT(cTagClose == 0);
            }
            cTagClose++;
            break;
        case '=':
            if (dwState != inTagEq)
            {
                SetSPInfoState(inTagEq, &dwState, &dwStatePrev, &fSave);
                ASSERT(cTagEq == 0);
            }
            cTagEq++;
            break;
        default:
            if (dwState != inChar)
            {
                SetSPInfoState(inChar, &dwState, &dwStatePrev, &fSave);
                ASSERT(cChar == 0);
            }
            cChar++;
            break;
        } /* switch */

        if (fSave) // save previous run
        {
            if (dwStatePrev != initState)
            {
                switch (dwStatePrev)
                {
                case inSpace:
                    *m_pspInfoCur++ = inSpace;
                    *m_pspInfoCur++ = cSpace;
                    cSpace = 0;
                    break;
                case inEOL:
                    *m_pspInfoCur++ = inEOL;
                    *m_pspInfoCur++ = cEOL;
                    cEOL = 0;
                    break;
                case inTab:
                    *m_pspInfoCur++ = inTab;
                    *m_pspInfoCur++ = cTab;
                    cTab = 0;
                    break;
                case inTagOpen:
                    *m_pspInfoCur++ = inTagOpen;
                    *m_pspInfoCur++ = cTagOpen;
                    cTagOpen = 0;
                    break;
                case inTagClose:
                    *m_pspInfoCur++ = inTagClose;
                    *m_pspInfoCur++ = cTagClose;
                    cTagClose = 0;
                    break;
                case inTagEq:
                    *m_pspInfoCur++ = inTagEq;
                    *m_pspInfoCur++ = cTagEq;
                    cTagEq = 0;
                    break;
                case inChar:
                    *m_pspInfoCur++ = inChar;
                    *m_pspInfoCur++ = cChar;
                    cChar = 0;
                    break;
                }
            }
            fSave = FALSE;

        } // if (fSave)

    } // for ()
    
    *pichStartSP = cbCur; // set for next run

    //if (pwOld[i] == '\0') // end of file and we wouldn't have saved the last run
    //{
        if (cSpace > 0)
            dwStatePrev = inSpace;
        else if (cEOL > 0)
            dwStatePrev = inEOL;
        else if (cTab > 0)
            dwStatePrev = inTab;
        else if (cTagOpen > 0)
            dwStatePrev = inTagOpen;
        else if (cTagClose > 0)
            dwStatePrev = inTagClose;
        else if (cTagEq > 0)
            dwStatePrev = inTagEq;
        else if (cChar > 0)
            dwStatePrev = inChar;
        else
            dwStatePrev = initState; // handle error case

        switch (dwStatePrev) // repeat of above, make this into a function
        {
        case inSpace:
            *m_pspInfoCur++ = inSpace;
            *m_pspInfoCur++ = cSpace;
            cSpace = 0;
            break;
        case inEOL:
            *m_pspInfoCur++ = inEOL;
            *m_pspInfoCur++ = cEOL;
            cEOL = 0;
            break;
        case inTab:
            *m_pspInfoCur++ = inTab;
            *m_pspInfoCur++ = cTab;
            cTab = 0;
            break;
        case inTagOpen:
            *m_pspInfoCur++ = inTagOpen;
            *m_pspInfoCur++ = cTagOpen;
            cTagOpen = 0;
            break;
        case inTagClose:
            *m_pspInfoCur++ = inTagClose;
            *m_pspInfoCur++ = cTagClose;
            cTagClose = 0;
            break;
        case inTagEq:
            *m_pspInfoCur++ = inTagEq;
            *m_pspInfoCur++ = cTagEq;
            cTagEq = 0;
            break;
        case inChar:
            *m_pspInfoCur++ = inChar;
            *m_pspInfoCur++ = cChar;
            cChar = 0;
            break;
        } // switch()
    //} // if ()

    *pspInfoSize = SAFE_PTR_DIFF_TO_WORD(m_pspInfoCur - pspInfoSize);

LRet:
    return(hrRet);

} /* hrMarkSpacing() */


BOOL
CTriEditParse::FRestoreSpacing(LPWSTR pwNew, LPWSTR /*pwOld*/, UINT *pichNewCur, INT *pcchwspInfo,
                               INT cchRange, INT ichtoktagStart, BOOL fLookback, INT index)
{
    BOOL fRet = TRUE;
    INT ichNewCur = (INT)*pichNewCur;
    INT cchwspInfo = *pcchwspInfo;
    WORD *pspInfoCur;
    INT cchwspInfoSav, cspInfopair, cchIncDec;
    BOOL fInValue = FALSE;

    cchwspInfo -= 2; // skip the cch & cchRange
    cchwspInfoSav = cchwspInfo;
    if (fLookback)
        pspInfoCur = m_pspInfoOut + cchwspInfo-1; // cch is actual number of char, so its 1 based
    else
        pspInfoCur = m_pspInfoOut;
    cspInfopair = cchwspInfo / 2; // we assume that cchwspInfo will be even number
    ASSERT(cchwspInfo % 2 == 0);
    cchIncDec = (fLookback)? -1 : 1;

    while (cspInfopair > 0)//(pspInfoCur >= m_pspInfoOut)
    {
        WORD dwState, count;

        cspInfopair--; // ready to get next cch & its type
        if (fLookback)
        {
            count = *(WORD *)pspInfoCur--;
            dwState = *(WORD *)pspInfoCur--;
        }
        else
        {
            dwState = *(WORD *)pspInfoCur++;
            count = *(WORD *)pspInfoCur++;
        }
        cchwspInfo -= 2; // previous pair of cch and its type

        switch (dwState)
        {
        case inChar:
            ASSERT(index == 1 || index == 0 || index == 3);
            if (index == 0 || index == 3)
            {
                int countws = 0; // count of white space chars

                while (    pwNew[ichtoktagStart-countws] == ' '
                        || pwNew[ichtoktagStart-countws] == '\t'
                        || pwNew[ichtoktagStart-countws] == '\r'
                        || pwNew[ichtoktagStart-countws] == '\n'
                        )
                {
                    // skip these white space chars. They shouldn't be here
                    countws++;
                    if (ichtoktagStart-countws <= 0)
                        break;
                }
                if (countws > 0)
                {
                    if (ichtoktagStart-countws >= 0)
                    {
                        memcpy((BYTE*)&pwNew[ichtoktagStart-countws+1], (BYTE *)&pwNew[ichtoktagStart+1], (ichNewCur-ichtoktagStart-1)*sizeof(WCHAR));
                        ichNewCur -= countws;
                        ichtoktagStart -= countws;
                    }
                }
            } // if (index == 0 || index == 3)

            while (    pwNew[ichtoktagStart] != ' '
                    && pwNew[ichtoktagStart] != '\t'
                    && pwNew[ichtoktagStart] != '\n'
                    && pwNew[ichtoktagStart] != '\r'
                    && pwNew[ichtoktagStart] != '<'
                    && pwNew[ichtoktagStart] != '>'
                    && pwNew[ichtoktagStart] != '='
                    && (ichNewCur > ichtoktagStart)
                    && count > 0
                    )
            {
                count--;
                ichtoktagStart += cchIncDec;
                cchRange--;
                if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                {
                    fRet = FALSE;
                    goto LRet;
                }
            }
            if (count == 0) // we match the exact chars, we may have more contiguous chars in pwNew
            {
                while (    pwNew[ichtoktagStart] != ' '
                        && pwNew[ichtoktagStart] != '\t'
                        && pwNew[ichtoktagStart] != '\n'
                        && pwNew[ichtoktagStart] != '\r'
                        && pwNew[ichtoktagStart] != '<'
                        && pwNew[ichtoktagStart] != '>'
                        && (pwNew[ichtoktagStart] != '=' || (fInValue /*&& index == 1*/))
                        && (ichNewCur > ichtoktagStart)
                        )
                {
                    ichtoktagStart += cchIncDec;
                    cchRange--;
                    if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                    {
                        fRet = FALSE;
                        goto LRet;
                    }
                }
            }
            break;
        case inTagOpen:
        case inTagClose:
        case inTagEq:
            // make sure that we have atleast count number of spaces at 
            // pwNew[ichtoktagStart-count]
            if (pwNew[ichtoktagStart] == '=' /* && index == 1*/)
                fInValue = TRUE;
            else
                fInValue = FALSE;
            while (    (pwNew[ichtoktagStart] == '<' || pwNew[ichtoktagStart] == '>' || pwNew[ichtoktagStart] == '=')
                    && count > 0
                    )
            {
                count--;
                ichtoktagStart += cchIncDec;
                cchRange--;
                if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                {
                    fRet = FALSE;
                    goto LRet;
                }
            }
            break;

        case inSpace:
            // make sure that we have atleast count number of spaces at 
            // pwNew[ichtoktagStart-count]
            fInValue = FALSE;
            while (pwNew[ichtoktagStart] == ' ' && count > 0)
            {
                count--;
                ichtoktagStart += cchIncDec;
                cchRange--;
                if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                {
                    fRet = FALSE;
                    goto LRet;
                }
            }
            if (count == 0) // we matched exact spaces, we may have more spaces in pwNew
            {
                if (fLookback)
                {
                    INT countT = 0;
                    //INT ichtoktagStartSav = ichtoktagStart;

                    if (cspInfopair == 0)
                        break;

                    ASSERT(index == 0 || index == 3);
                    // REMOVE EXTRA SPACES here.
                    while (pwNew[ichtoktagStart-countT] == ' ')
                        countT++;
                    if (countT > 0)
                    {
                        if (ichNewCur-(ichtoktagStart) > 0)
                        {
                            memmove((BYTE *)(pwNew+ichtoktagStart-countT+1),
                                    (BYTE *)(pwNew+ichtoktagStart),
                                    (ichNewCur-(ichtoktagStart))*sizeof(WCHAR));
                            ichNewCur -= (countT-1);
                            ichtoktagStart -= (countT-1);
                            while (countT > 1)
                            {
                                pwNew[ichNewCur+countT-2] = '\0';
                                countT--;
                            }
                        }
                    }
                }
                else if (!fLookback)
                {
                    INT countT = -1;

                    ASSERT(index == 1 || index == 2);
                    // look ahead into pspInfoCur to see what the next parameters should be
                    if ((index == 1) && (*(WORD *)pspInfoCur == inChar))
                    {
                        while (    pwNew[ichtoktagStart] == ' '
                                || pwNew[ichtoktagStart] == '\r'
                                || pwNew[ichtoktagStart] == '\n'
                                || pwNew[ichtoktagStart] == '\t'
                                )
                        {
                            countT++;
                            ichtoktagStart += cchIncDec;
                        }
                    }
                    else
                    {
                        while (pwNew[ichtoktagStart] == ' ')
                        {
                            countT++;
                            ichtoktagStart += cchIncDec;
                        }
                    }
                    if (countT > 0)
                    {
                        if (ichNewCur-(ichtoktagStart+1) > 0)
                        {
                            memmove((BYTE *)(pwNew+ichtoktagStart-countT-1),
                                    (BYTE *)(pwNew+ichtoktagStart),
                                    (ichNewCur-ichtoktagStart)*sizeof(WCHAR));
                            ichNewCur -= (countT+1);
                            ichtoktagStart -= (countT+1);
                            while (countT >= 0)
                            {
                                pwNew[ichNewCur+countT] = '\0';
                                countT--;
                            }
                        }
                    }
                }
            }
            else
            {
                if (fLookback)
                {
                    ASSERT(index == 0 || index == 3);
                    if ((int)(ichNewCur-ichtoktagStart-1) >= 0)
                    {
                        // insert spaces after ichtoktagStart
                        memmove((BYTE *)&pwNew[ichtoktagStart+1+count],
                                (BYTE *)&pwNew[ichtoktagStart+1],
                                (ichNewCur-ichtoktagStart-1)*sizeof(WCHAR));
                        ichNewCur += count;
                        //ichtoktagStart++;
                        while (count > 0)
                        {
                            pwNew[ichtoktagStart+count] = ' ';
                            count--;
                        }
                        //ichtoktagStart--; // compensate
                    }
                }
                else 
                {
                    ASSERT(index == 1 || index == 2);
                    if ((int)(ichNewCur-ichtoktagStart) >= 0)
                    {
                        int countT = count;

                        // insert spaces at ichtoktagStart and set ichtoktagStart after last space
                        memmove((BYTE *)&pwNew[ichtoktagStart+count],
                                (BYTE *)&pwNew[ichtoktagStart],
                                (ichNewCur-ichtoktagStart)*sizeof(WCHAR));
                        ichNewCur += count;
                        while (count > 0)
                        {
                            ASSERT((INT)(ichtoktagStart+count-1) >= 0);
                            pwNew[ichtoktagStart+count-1] = ' ';
                            count--;
                        }
                        ichtoktagStart += countT;
                    }
                }
            }
            break;
        case inEOL:
            // make sure that we have atleast count number of EOLs at 
            // pwNew[ichtoktagStart-count]
            // if fLookback, then we get '\n', else we get '\r'
            fInValue = FALSE;
            while ((pwNew[ichtoktagStart] == '\n' || pwNew[ichtoktagStart] == '\r') && count > 0)
            {
                count--;
                cchRange -= 2;
                ichtoktagStart += cchIncDec; // assume '\r' or '\n'
                ichtoktagStart += cchIncDec; // assume '\r' or '\n'
                if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                {
                    fRet = FALSE;
                    goto LRet;
                }

            }
            if (count == 0) // we matched exact EOLs, we may have more EOLs in pwNew
            {
                if (fLookback)
                {
                    INT countT = 0;

                    ASSERT(index == 0 || index == 3);
                    // REMOVE EXTRA EOLs here.
                    while (    pwNew[ichtoktagStart-countT] == '\r'
                            || pwNew[ichtoktagStart-countT] == '\n'
                            )
                        countT++;
                    if (countT > 0)
                    {
                        if (ichNewCur-(ichtoktagStart) > 0)
                        {
                            memmove((BYTE *)(pwNew+ichtoktagStart-countT+1),
                                    (BYTE *)(pwNew+ichtoktagStart),
                                    (ichNewCur-(ichtoktagStart))*sizeof(WCHAR));
                            ichNewCur -= (countT-1);
                            ichtoktagStart -= (countT-1);
                            while (countT > 1)
                            {
                                pwNew[ichNewCur+countT-2] = '\0';
                                countT--;
                            }
                        }
                    }
                }
                else if (!fLookback)
                {
                    INT countT = 0;

                    ASSERT(index == 1 || index == 2);
                    // REMOVE EXTRA EOLS here.

                    // look ahead into pspInfoCur to see what the next parameters should be
                    if ((index == 1) && (*(WORD *)pspInfoCur == inChar))
                    {
                        while (    pwNew[ichtoktagStart] == ' '
                                || pwNew[ichtoktagStart] == '\r'
                                || pwNew[ichtoktagStart] == '\n'
                                || pwNew[ichtoktagStart] == '\t'
                                )
                        {
                            countT++;
                            ichtoktagStart += cchIncDec;
                        }
                    }
                    else
                    {
                        while (    pwNew[ichtoktagStart] == '\r'
                                || pwNew[ichtoktagStart] == '\n'
                                )
                        {
                            countT++;
                            ichtoktagStart += cchIncDec;
                        }
                    }
                    
                    //ASSERT(countT % 2 == 0); // assert that countT is an even number, because we should find \r & \n always in pair
                    if (countT > 0)
                    {
                        if (ichNewCur-(ichtoktagStart+1) > 0)
                        {
                            memmove((BYTE *)(pwNew+ichtoktagStart-countT),
                                    (BYTE *)(pwNew+ichtoktagStart),
                                    (ichNewCur-ichtoktagStart)*sizeof(WCHAR));
                            ichNewCur -= (countT);
                            ichtoktagStart -= (countT);
                            while (countT >= 0)
                            {
                                pwNew[ichNewCur+countT] = '\0';
                                countT--;
                            }
                        }
                    }
                }
            }
            else
            {
                if (fLookback)
                {
                    INT i;

                    ASSERT(index == 0 || index == 3);
                    if ((int)(ichNewCur-ichtoktagStart-1) >= 0)
                    {
                        // insert EOLs after ichtoktagStart
                        memmove((BYTE *)&pwNew[ichtoktagStart+1+count*2],
                                (BYTE *)&pwNew[ichtoktagStart+1],
                                (ichNewCur-ichtoktagStart-1)*sizeof(WCHAR));
                        ichNewCur += count*2;
                        count *= 2;
                        ichtoktagStart++;
                        for (i = 0; i < count; i+=2)
                        {
                            pwNew[ichtoktagStart+i] = '\r';
                            pwNew[ichtoktagStart+i+1] = '\n';
                        }
                        ichtoktagStart--; // compensate for prior increment
                    }
                }
                else 
                {
                    INT i;

                    ASSERT(index == 1 || index == 2);
                    // insert spaces at ichtoktagStart and set ichtoktagStart after last space
                    if ((int)(ichNewCur-ichtoktagStart) >= 0)
                    {
                        memmove((BYTE *)&pwNew[ichtoktagStart+count*2],
                                (BYTE *)&pwNew[ichtoktagStart],
                                (ichNewCur-ichtoktagStart)*sizeof(WCHAR));
                        ichNewCur += count*2;
                        count *= 2;
                        for (i=0; i < count; i+=2)
                        {
                            pwNew[ichtoktagStart+i] = '\r';
                            pwNew[ichtoktagStart+i+1] = '\n';
                        }
                        ichtoktagStart += count;
                    }
                }
            }

            break;
        case inTab:
            // make sure that we have atleast count number of spaces at 
            // pwNew[ichtoktagStart-count]
            fInValue = FALSE;
            while (pwNew[ichtoktagStart] == '\t' && count > 0)
            {
                count--;
                ichtoktagStart += cchIncDec;
                cchRange--;
                if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                {
                    fRet = FALSE;
                    goto LRet;
                }
            }
            if (count == 0) // we matched exact spaces, we may have more tabs in pwNew
            {
                // skip extra spaces in pwNew, if we had more spaces in pwNew than count
                while (pwNew[ichtoktagStart] == '\t')
                {
                    ichtoktagStart += cchIncDec;
                    cchRange--;
                    if (ichtoktagStart < 0 || cchRange < 0) // boundary condition
                    {
                        fRet = FALSE;
                        goto LRet;
                    }
                }

            }
            else
            {
                INT ichSav = ichtoktagStart;
                INT i;

                ASSERT(count > 0);
                // insert these many extra tabs at pwNew[ichtoktagStart] and increment ichNewCur
                if (fLookback)
                    ichtoktagStart++;
                if (ichNewCur-ichtoktagStart > 0)
                {
                    memmove((BYTE *)(pwNew+ichtoktagStart+count), 
                            (BYTE *)(pwNew+ichtoktagStart),
                            (ichNewCur-ichtoktagStart)*sizeof(WCHAR));
                }
                for (i = 0; i < count; i++)
                    pwNew[ichtoktagStart+i] = '\t';

                ichNewCur += count;
                if (fLookback)
                    ichtoktagStart = ichSav;
                else
                    ichtoktagStart += count;
            }
            break;
        } // switch (dwState)

    } // while ()
    if (   cspInfopair == 0
        && pwNew[ichNewCur-1] == '>'
        && ichNewCur > ichtoktagStart
        && !fLookback
        && index == 1)
    {
        INT countT = 0;

        ASSERT(cchIncDec == 1);
        // This means that we may have extra spaces & EOLs from ichtoktagStart to '>'
        // REMOVE EXTRA SPACES EOLS here.
        while (    pwNew[ichtoktagStart+countT] == ' '
                || pwNew[ichtoktagStart+countT] == '\r'
                || pwNew[ichtoktagStart+countT] == '\n'
                || pwNew[ichtoktagStart+countT] == '\t'
                )
        {
            countT++;
        }
        if (countT > 0 && pwNew[ichtoktagStart+countT] == '>')
        {
            if (ichNewCur-(ichtoktagStart+1) > 0)
            {
                memmove((BYTE *)(pwNew+ichtoktagStart),
                        (BYTE *)(pwNew+ichtoktagStart+countT),
                        (ichNewCur-(ichtoktagStart+countT))*sizeof(WCHAR));
                ichNewCur -= (countT);
                ichtoktagStart -= (countT);
                while (countT > 0)
                {
                    pwNew[ichNewCur+countT-1] = '\0';
                    countT--;
                }
            }
        }

        // Next time around - we can do the following...
        // look back from ichtoktagStart and check if we have any spaces/eols.
        // if we do, there is a likelihood that these shouldn't have been there.
        // Here is how they get there - If we had spaces between the parameter and
        // the '=' and its value, those spacves are removed by Trident. We then go
        // in and add those spaces at the end rather than at proper place because 
        // we don't break up the text. e.g. "width = 23" --> "width=23". 
        // Now, because we don't break that text, we end up inserting these spaces
        // at the end. Lets remove them.
    }
    else if (      cspInfopair == 0
                && fLookback
                && (index == 0 || index == 3)) /* VID6 - bug 18207 */
    {
        INT countT = 0;

        ASSERT(cchIncDec == -1);
        // This means that we may have extra spaces & EOLs before ichtoktagStart to '>'
        // REMOVE EXTRA SPACES EOLS here.
        while (    pwNew[ichtoktagStart-countT] == ' '
                || pwNew[ichtoktagStart-countT] == '\r'
                || pwNew[ichtoktagStart-countT] == '\n'
                || pwNew[ichtoktagStart-countT] == '\t'
                )
        {
            countT++;
        }
        if (countT > 0 && pwNew[ichtoktagStart-countT] == '>')
        {
            if (ichNewCur-(ichtoktagStart+1) > 0)
            {
                memmove((BYTE *)(pwNew+ichtoktagStart-countT+1),
                        (BYTE *)(pwNew+ichtoktagStart+1),
                        (ichNewCur-(ichtoktagStart+1))*sizeof(WCHAR));
                ichNewCur -= countT;
                ichtoktagStart -= (countT); // this doesn't matter because we will exit after this
                while (countT > 0)
                {
                    pwNew[ichNewCur+countT-1] = '\0';
                    countT--;
                }
            }
        }
    }
LRet:
    m_pspInfoOut = m_pspInfoOut + cchwspInfoSav;
    *pcchwspInfo = cchwspInfo;
    *pichNewCur = ichNewCur;
    return(fRet);
}
