#include "msrating.h"
#include <npassert.h>
#include "array.h"
#include "msluglob.h"
#include "parselbl.h"
#include <convtime.h>
#include <wininet.h>

extern BOOL LoadWinINet();


COptionsBase::COptionsBase()
{
    m_cRef = 1;
    m_timeUntil = 0xffffffff;   /* as far in the future as possible */
    m_fdwFlags = 0;
    m_pszInvalidString = NULL;
    m_pszURL = NULL;
}


void COptionsBase::AddRef()
{
    m_cRef++;
}


void COptionsBase::Release()
{
    if (!--m_cRef)
        Delete();
}


void COptionsBase::Delete()
{
    /* default does nothing when deleting reference */
}


BOOL COptionsBase::CheckUntil(DWORD timeUntil)
{
    if (m_timeUntil <= timeUntil) {
        m_fdwFlags |= LBLOPT_EXPIRED;
        return FALSE;
    }
    return TRUE;
}


/* AppendSlash forces pszString to end in a single slash if it doesn't
 * already.  This may produce a technically invalid URL (for example,
 * "http://gregj/default.htm/", but we're only using the result for
 * comparisons against other paths similarly mangled.
 */
void AppendSlash(LPSTR pszString)
{
    LPSTR pszSlash = ::strrchrf(pszString, '/');

    if (pszSlash == NULL || *(pszSlash + 1) != '\0')
        ::strcatf(pszString, "/");
}


extern BOOL (WINAPI *pfnInternetCrackUrl)(
    IN LPCTSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS lpUrlComponents
    );
extern BOOL (WINAPI *pfnInternetCanonicalizeUrl)(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );


BOOL DoURLsMatch(LPCSTR pszBaseURL, LPCSTR pszCheckURL, BOOL fGeneric)
{
    /* Buffers to canonicalize URLs into */
    LPSTR pszBaseCanon = new char[INTERNET_MAX_URL_LENGTH + 1];
    LPSTR pszCheckCanon = new char[INTERNET_MAX_URL_LENGTH + 1];

    if (pszBaseCanon != NULL && pszCheckCanon != NULL) {
        BOOL fCanonOK = FALSE;
        DWORD cbBuffer = INTERNET_MAX_URL_LENGTH + 1;
        if (pfnInternetCanonicalizeUrl(pszBaseURL, pszBaseCanon, &cbBuffer, ICU_ENCODE_SPACES_ONLY)) {
            cbBuffer = INTERNET_MAX_URL_LENGTH + 1;
            if (pfnInternetCanonicalizeUrl(pszCheckURL, pszCheckCanon, &cbBuffer, ICU_ENCODE_SPACES_ONLY)) {
                fCanonOK = TRUE;
            }
        }
        if (!fCanonOK) {
            delete pszBaseCanon;
            delete pszCheckCanon;
            return FALSE;
        }
    }

    UINT cbBaseURL = strlenf(pszBaseCanon) + 1;

    LPSTR pszBaseUrlPath = new char[cbBaseURL];
    LPSTR pszBaseExtra = new char[cbBaseURL];

    CHAR szBaseHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    CHAR szBaseUrlScheme[20];   // reasonable limit

    UINT cbCheckURL = strlenf(pszCheckCanon) + 1;

    LPSTR pszCheckUrlPath = new char[cbCheckURL];
    LPSTR pszCheckExtra = new char[cbCheckURL];

    CHAR szCheckHostName[INTERNET_MAX_HOST_NAME_LENGTH];
    CHAR szCheckUrlScheme[20];   // reasonable limit

    BOOL fOK = FALSE;

    if (pszBaseUrlPath != NULL &&
        pszBaseExtra != NULL &&
        pszCheckUrlPath != NULL &&
        pszCheckExtra != NULL) {

        URL_COMPONENTS ucBase, ucCheck;

        memset(&ucBase, 0, sizeof(ucBase));
        ucBase.dwStructSize      = sizeof(ucBase);
        ucBase.lpszScheme        = szBaseUrlScheme;
        ucBase.dwSchemeLength    = sizeof(szBaseUrlScheme);
        ucBase.lpszHostName      = szBaseHostName;
        ucBase.dwHostNameLength  = sizeof(szBaseHostName);
        ucBase.lpszUrlPath       = pszBaseUrlPath;
        ucBase.dwUrlPathLength   = cbBaseURL;
        ucBase.lpszExtraInfo     = pszBaseExtra;
        ucBase.dwExtraInfoLength = cbBaseURL;

        memset(&ucCheck, 0, sizeof(ucCheck));
        ucCheck.dwStructSize      = sizeof(ucCheck);
        ucCheck.lpszScheme        = szCheckUrlScheme;
        ucCheck.dwSchemeLength    = sizeof(szCheckUrlScheme);
        ucCheck.lpszHostName      = szCheckHostName;
        ucCheck.dwHostNameLength  = sizeof(szCheckHostName);
        ucCheck.lpszUrlPath       = pszCheckUrlPath;
        ucCheck.dwUrlPathLength   = cbCheckURL;
        ucCheck.lpszExtraInfo     = pszCheckExtra;
        ucCheck.dwExtraInfoLength = cbCheckURL;

        if (pfnInternetCrackUrl(pszBaseCanon, 0, 0, &ucBase) &&
            pfnInternetCrackUrl(pszCheckCanon, 0, 0, &ucCheck)) {

            /* Scheme and host name must always match */
            if (!stricmpf(ucBase.lpszScheme, ucCheck.lpszScheme) &&
                !stricmpf(ucBase.lpszHostName, ucCheck.lpszHostName)) {

                /* For extra info, just has to match exactly, even for a generic URL. */
                if (!*ucBase.lpszExtraInfo ||
                    !stricmpf(ucBase.lpszExtraInfo, ucCheck.lpszExtraInfo)) {

                    AppendSlash(ucBase.lpszUrlPath);
                    AppendSlash(ucCheck.lpszUrlPath);

                    /* If not a generic label, path must match exactly too */
                    if (!fGeneric) {
                        if (!stricmpf(ucBase.lpszUrlPath, ucCheck.lpszUrlPath))
                            fOK = TRUE;
                    }
                    else {
                        UINT cbBasePath = strlenf(ucBase.lpszUrlPath);
                        if (!strnicmpf(ucBase.lpszUrlPath, ucCheck.lpszUrlPath, cbBasePath))
                            fOK = TRUE;
                    }
                }
            }
        }
    }

    delete pszBaseUrlPath;
    delete pszBaseExtra;

    delete pszCheckUrlPath;
    delete pszCheckExtra;

    delete pszBaseCanon;
    delete pszCheckCanon;

    return fOK;
}


BOOL COptionsBase::CheckURL(LPCSTR pszURL)
{
    if (!(m_fdwFlags & LBLOPT_URLCHECKED)) {
        m_fdwFlags |= LBLOPT_URLCHECKED;

        BOOL fInvalid = FALSE;

        if (pszURL != NULL && m_pszURL != NULL) {
            if (LoadWinINet()) {
                fInvalid = !DoURLsMatch(m_pszURL, pszURL, m_fdwFlags & LBLOPT_GENERIC);
            }
        }

        if (fInvalid)
            m_fdwFlags |= LBLOPT_WRONGURL;
    }


    return !(m_fdwFlags & LBLOPT_WRONGURL);
}


void CDynamicOptions::Delete()
{
    delete this;
}


CParsedServiceInfo::CParsedServiceInfo()
{
    m_pNext = NULL;
    m_poptCurrent = &m_opt;
    m_poptList = NULL;
    m_pszServiceName = NULL;
    m_pszErrorString = NULL;
    m_fInstalled = TRUE;        /* assume the best */
    m_pszInvalidString = NULL;
}


void FreeOptionsList(CDynamicOptions *pList)
{
    while (pList != NULL) {
        CDynamicOptions *pNext = pList->m_pNext;
        delete pList;
        pList = pNext;
    }
}


CParsedServiceInfo::~CParsedServiceInfo()
{
    FreeOptionsList(m_poptList);
}


void CParsedServiceInfo::Append(CParsedServiceInfo *pNew)
{
    CParsedServiceInfo **ppNext = &m_pNext;

    while (*ppNext != NULL)
        ppNext = &((*ppNext)->m_pNext);

    *ppNext = pNew;
    pNew->m_pNext = NULL;
}


CParsedLabelList::CParsedLabelList()
{
    m_pszList = NULL;
    m_fRated = FALSE;
    m_pszInvalidString = NULL;
    m_pszURL = NULL;
    m_pszOriginalLabel = NULL;
}


CParsedLabelList::~CParsedLabelList()
{
    delete m_pszList;

    CParsedServiceInfo *pInfo = m_ServiceInfo.Next();

    while (pInfo != NULL) {
        CParsedServiceInfo *pNext = pInfo->Next();
        delete pInfo;
        pInfo = pNext;
    }

    delete m_pszURL;
    delete m_pszOriginalLabel;
}


/* SkipWhitespace(&pszString)
 *
 * advances pszString past whitespace characters
 */
void SkipWhitespace(LPSTR *ppsz)
{
    UINT cchWhitespace = ::strspnf(*ppsz, szWhitespace);

    *ppsz += cchWhitespace;
}


/* FindTokenEnd(pszStart)
 *
 * Returns a pointer to the end of a contiguous range of similarly-typed
 * characters (whitespace, quote mark, punctuation, or alphanumerics).
 */
LPSTR FindTokenEnd(LPSTR pszStart)
{
    LPSTR pszEnd = pszStart;

    if (*pszEnd == '\0') {
        return pszEnd;
    }
    else if (strchrf(szSingleCharTokens, *pszEnd)) {
        return ++pszEnd;
    }

    UINT cch;
    cch = ::strspnf(pszEnd, szWhitespace);
    if (cch > 0)
        return pszEnd + cch;

    cch = ::strspnf(pszEnd, szExtendedAlphaNum);
    if (cch > 0)
        return pszEnd + cch;

    return pszEnd;              /* unrecognized characters */
}


/* GetBool(LPSTR *ppszToken, BOOL *pfOut, PICSRulesBooleanSwitch PRBoolSwitch)
 *
 * t-markh 8/98 (
 * added default parameter PRBoolSwitch=PR_BOOLEAN_TRUEFALSE
 * this allows for no modification of existing code, and extension
 * of the GetBool function from true/false to include pass/fail and
 * yes/no.  The enumerated type PICSRulesBooleanSwitch is defined
 * in picsrule.h)
 *
 * Parses a boolean value at the given token and returns its value in *pfOut.
 * Legal values are 't', 'f', 'true', and 'false'.  If success, *ppszToken
 * is advanced past the boolean token and any following whitespace.  If failure,
 * *ppszToken is not modified.
 *
 * pfOut may be NULL if the caller just wants to eat the token and doesn't
 * care about its value.
 */
HRESULT GetBool(LPSTR *ppszToken, BOOL *pfOut, PICSRulesBooleanSwitch PRBoolSwitch)
{
    BOOL bValue;

    LPSTR pszTokenEnd = FindTokenEnd(*ppszToken);

    switch(PRBoolSwitch)
    {
        case PR_BOOLEAN_TRUEFALSE:
        {
            if (IsEqualToken(*ppszToken, pszTokenEnd, szShortTrue) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szTrue))
            {
                bValue = TRUE;
            }
            else if (IsEqualToken(*ppszToken, pszTokenEnd, szShortFalse) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szFalse))
            {
                bValue = FALSE;
            }
            else
            {
                return ResultFromScode(MK_E_SYNTAX);
            }

            break;
        }

        case PR_BOOLEAN_PASSFAIL:
        {
            //szPRShortPass and szPRShortfail are not supported in the
            //official PICSRules spec, but we'll catch them anyway

            if (IsEqualToken(*ppszToken, pszTokenEnd, szPRShortPass) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szPRPass))
            {
                bValue = PR_PASSFAIL_PASS;
            }
            else if (IsEqualToken(*ppszToken, pszTokenEnd, szPRShortFail) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szPRFail))
            {
                bValue = PR_PASSFAIL_FAIL;
            }
            else
            {
                return ResultFromScode(MK_E_SYNTAX);
            }

            break;
        }

        case PR_BOOLEAN_YESNO:
        {
            if (IsEqualToken(*ppszToken, pszTokenEnd, szPRShortYes) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szPRYes))
            {
                bValue = PR_YESNO_YES;
            }
            else if (IsEqualToken(*ppszToken, pszTokenEnd, szPRShortNo) ||
                IsEqualToken(*ppszToken, pszTokenEnd, szPRNo))
            {
                bValue = PR_YESNO_NO;
            }
            else
            {
                return ResultFromScode(MK_E_SYNTAX);
            }

            break;
        }

        default:
        {
            return(MK_E_UNAVAILABLE);
        }
    }

    if (pfOut != NULL)
        *pfOut = bValue;

    *ppszToken = pszTokenEnd;
    SkipWhitespace(ppszToken);

    return NOERROR;
}


/* GetQuotedToken(&pszThisToken, &pszQuotedToken)
 *
 * Sets pszQuotedToken to point to the contents of the doublequotes.
 * pszQuotedToken may be NULL if the caller just wants to eat the token.
 * Sets pszThisToken to point to the first character after the closing
 *   doublequote.
 * Fails if pszThisToken doesn't start with a doublequote or doesn't
 *   contain a closing doublequote.
 * The closing doublequote is replaced with a null terminator, iff the
 *   function does not fail.
 */
HRESULT GetQuotedToken(LPSTR *ppszThisToken, LPSTR *ppszQuotedToken)
{
    HRESULT hres = ResultFromScode(MK_E_SYNTAX);

    LPSTR pszStart = *ppszThisToken;
    if (*pszStart != '\"')
        return hres;

    pszStart++;
    LPSTR pszEndQuote = strchrf(pszStart, '\"');
    if (pszEndQuote == NULL)
        return hres;

    *pszEndQuote = '\0';
    if (ppszQuotedToken != NULL)
        *ppszQuotedToken = pszStart;
    *ppszThisToken = pszEndQuote+1;

    return NOERROR;
}


BOOL IsEqualToken(LPCSTR pszTokenStart, LPCSTR pszTokenEnd, LPCSTR pszTokenToMatch)
{
    UINT cbToken = strlenf(pszTokenToMatch);

    if (cbToken != (UINT)(pszTokenEnd - pszTokenStart) || strnicmpf(pszTokenStart, pszTokenToMatch, cbToken))
        return FALSE;

    return TRUE;
}


/* ParseLiteralToken(ppsz, pszToken) tries to match *ppsz against pszToken.
 * If they don't match, an error is returned.  If they do match, then *ppsz
 * is advanced past the token and any following whitespace.
 *
 * If ppszInvalid is NULL, then the function is non-destructive in the error
 * path, so it's OK to call ParseLiteralToken just to see if a possible literal
 * token is what's next; if the token isn't found, whatever was there didn't
 * get eaten or anything.
 *
 * If ppszInvalid is not NULL, then if the token doesn't match, *ppszInvalid
 * will be set to *ppsz.
 */
HRESULT ParseLiteralToken(LPSTR *ppsz, LPCSTR pszToken, LPCSTR *ppszInvalid)
{
    LPSTR pszTokenEnd = FindTokenEnd(*ppsz);

    if (!IsEqualToken(*ppsz, pszTokenEnd, pszToken)) {
        if (ppszInvalid != NULL)
            *ppszInvalid = *ppsz;
        return ResultFromScode(MK_E_SYNTAX);
    }

    *ppsz = pszTokenEnd;

    SkipWhitespace(ppsz);

    return NOERROR;
}


/* ParseServiceError parses a service-error construct, once it's been
 * determined that such is the case.  m_pszCurrent has been advanced past
 * the 'error' keyword that indicates a service-error.
 *
 * We're pretty flexible about the contents of this stuff.  We basically
 * accept anything of the form:
 *
 * 'error' '(' <error string> [quoted explanations] ')'     - or -
 * 'error' <error string>
 *
 * without caring too much about what the error string actually is.
 *
 * A format with quoted explanations but without the parens would not be
 * legal, we wouldn't be able to distinguish the explanations from the
 * serviceID of the next service-info.
 */
HRESULT CParsedServiceInfo::ParseServiceError()
{
    BOOL fParen = FALSE;
    HRESULT hres = NOERROR;

    if (SUCCEEDED(ParseLiteralToken(&m_pszCurrent, szLeftParen, NULL))) {
        fParen = TRUE;
    }

    LPSTR pszErrorEnd = FindTokenEnd(m_pszCurrent);     /* find end of error string */

    m_pszErrorString = m_pszCurrent;                /* remember start of error string */
    if (fParen) {                           /* need to eat explanations */
        m_pszCurrent = pszErrorEnd;         /* skip error string to get to explanations */
        SkipWhitespace();
        while (SUCCEEDED(hres)) {
            hres = GetQuotedToken(&m_pszCurrent, NULL);
            SkipWhitespace();
        }
    }

    if (fParen)
        hres = ParseLiteralToken(&m_pszCurrent, szRightParen, &m_pszInvalidString);
    else
        hres = NOERROR;

    if (SUCCEEDED(hres))
        *pszErrorEnd = '\0';            /* null-terminate the error string */

    return hres;
}


/* ParseNumber parses a numeric token at the specified position.  If the
 * number makes sense, the pointer is advanced to the end of the number
 * and past any following whitespace, and the numeric value is returned
 * in *pnOut.  Any non-numeric characters are considered to terminate the
 * number without error;  it is assumed that higher-level parsing code
 * will eventually reject such characters if they're not supposed to be
 * there.
 *
 * pnOut may be NULL if the caller doesn't care about the number being
 * returned and just wants to eat it.
 *
 * Floating point numbers of the form nnn.nnn are rounded to the next
 * higher integer and returned as such.
 */
//t-markh 8/98 - added fPICSRules for line counting support in PICSRules
HRESULT ParseNumber(LPSTR *ppszNumber, INT *pnOut,BOOL fPICSRules)
{
    HRESULT hres = ResultFromScode(MK_E_SYNTAX);
    BOOL fNegative = FALSE;
    INT nAccum = 0;
    BOOL fNonZeroDecimal = FALSE;
    BOOL fInDecimal = FALSE;
    BOOL fFoundDigits = FALSE;

    LPSTR pszCurrent = *ppszNumber;

    /* Handle one sign character. */
    if (*pszCurrent == '+') {
        pszCurrent++;
    }
    else if (*pszCurrent == '-') {
        pszCurrent++;
        fNegative = TRUE;
    }

    for (;;) {
        if (*pszCurrent == '.') {
            fInDecimal = TRUE;
        }
        else if (*pszCurrent >= '0' && *pszCurrent <= '9') {
            fFoundDigits = TRUE;
            if (fInDecimal) {
                if (*pszCurrent > '0') {
                    fNonZeroDecimal = TRUE;
                }
            }
            else {
                nAccum = nAccum * 10 + (*pszCurrent - '0');
            }
        }
        else
            break;

        pszCurrent++;
    }

    if (fFoundDigits) {
        hres = NOERROR;
        if (fNonZeroDecimal)
            nAccum++;           /* round away from zero if decimal present */
        if (fNegative)
            nAccum = -nAccum;
    }

    if (SUCCEEDED(hres)) {
        if (pnOut != NULL)
            *pnOut = nAccum;
        *ppszNumber = pszCurrent;
        if(fPICSRules==FALSE) {
        SkipWhitespace(ppszNumber);
        }
    }

    return hres;
}


/* ParseExtensionData just needs to get past whatever data was supplied
 * for an extension.  The PICS spec implies that it can be recursive, which
 * complicates matters a bit:
 *
 * data :: quoted-ISO-date | quotedURL | number | quotedname | '(' data* ')'
 *
 * Use of recursion here is probably OK, we don't really expect complicated
 * nested extensions all that often, and this function doesn't use a lot of
 * stack or other resources...
 */
HRESULT CParsedServiceInfo::ParseExtensionData(COptionsBase *pOpt)
{
    HRESULT hres;

    if (SUCCEEDED(ParseLiteralToken(&m_pszCurrent, szLeftParen, NULL))) {
        hres = ParseExtensionData(pOpt);
        if (FAILED(hres))
            return hres;
        return ParseLiteralToken(&m_pszCurrent, szRightParen, &m_pszInvalidString);
    }

    if (SUCCEEDED(GetQuotedToken(&m_pszCurrent, NULL))) {
        SkipWhitespace();
        return NOERROR;
    }

    hres = ParseNumber(&m_pszCurrent, NULL);
    if (FAILED(hres))
        m_pszInvalidString = m_pszCurrent;

    return hres;
}


/* ParseExtension parses an extension option.  Syntax is:
 *
 * extension ( mandatory|optional "identifyingURL" data )
 *
 * Currently all extensions are parsed but ignored, although a mandatory
 * extension causes the entire options structure and anything dependent
 * on it to be invalidated.
 */
HRESULT CParsedServiceInfo::ParseExtension(COptionsBase *pOpt)
{
    HRESULT hres;

    hres = ParseLiteralToken(&m_pszCurrent, szLeftParen, &m_pszInvalidString);
    if (FAILED(hres))
        return hres;

    hres = ParseLiteralToken(&m_pszCurrent, szOptional, &m_pszInvalidString);
    if (FAILED(hres)) {
        hres = ParseLiteralToken(&m_pszCurrent, szMandatory, &m_pszInvalidString);
        if (SUCCEEDED(hres))
            pOpt->m_fdwFlags |= LBLOPT_INVALID;
    }
    if (FAILED(hres))
        return hres;            /* BUGBUG - this causes us to lose our place -- OK? */

    hres = GetQuotedToken(&m_pszCurrent, NULL);
    if (FAILED(hres)) {
        m_pszInvalidString = m_pszCurrent;
        return hres;
    }

    SkipWhitespace();

    while (*m_pszCurrent != ')' && *m_pszCurrent != '\0') {
        hres = ParseExtensionData(pOpt);
        if (FAILED(hres))
            return hres;
    }
    if (*m_pszCurrent != ')') {
        m_pszInvalidString = m_pszCurrent;
        return ResultFromScode(MK_E_SYNTAX);
    }

    m_pszCurrent++;
    SkipWhitespace();

    return NOERROR;
}


/* ParseTime parses a "quoted-ISO-date" as found in a label.  This is required
 * to have the following form, as quoted from the PICS spec:
 *
 * quoted-ISO-date :: YYYY'.'MM'.'DD'T'hh':'mmStz
 *  YYYY :: four-digit year
 *  MM :: two-digit month (01=January, etc.)
 *  DD :: two-digit day of month (01-31)
 *  hh :: two digits of hour (00-23)
 *  mm :: two digits of minute (00-59)
 *  S :: sign of time zone offset from UTC (+ or -)
 *  tz :: four digit amount of offset from UTC (e.g., 1512 means 15 hours 12 minutes)
 *
 * Example: "1994.11.05T08:15-0500" means Nov. 5, 1994, 8:15am, US EST.
 *
 * Time is parsed into NET format -- seconds since 1970 (easiest to adjust for
 * time zones, and compare with).  Returns an error if string is invalid.
 */

/* Template describing the string format.  'n' means a digit, '+' means a
 * plus or minus sign, any other character must match that literal character.
 */
const char szTimeTemplate[] = "nnnn.nn.nnTnn:nn+nnnn";
const char szPICSRulesTimeTemplate[] = "nnnn-nn-nnTnn:nn+nnnn";

HRESULT ParseTime(LPSTR pszTime, DWORD *pOut, BOOL fPICSRules)
{
    /* Copy the time string into a temporary buffer, since we're going to
     * stomp on some separators.  We preserve the original in case it turns
     * out to be invalid and we have to show it to the user later.
     */
    LPCSTR pszCurrTemplate;
    
    char szTemp[sizeof(szTimeTemplate)];

    if (::strlenf(pszTime) >= sizeof(szTemp))
        return MK_E_SYNTAX;
    strcpyf(szTemp, pszTime);

    LPSTR pszCurrent = szTemp;

    if(fPICSRules)
    {
        pszCurrTemplate = szPICSRulesTimeTemplate;
    }
    else
    {
        pszCurrTemplate = szTimeTemplate;
    }

    /* First validate the format against the template.  If that succeeds, then
     * we get to make all sorts of assumptions later.
     *
     * We stomp all separators except the +/- for the timezone with spaces
     * so that ParseNumber will (a) skip them for us, and (b) not interpret
     * the '.' separators as decimal points.
     */
    BOOL fOK = TRUE;
    while (*pszCurrent && *pszCurrTemplate && fOK) {
        char chCurrent = *pszCurrent;

        switch (*pszCurrTemplate) {
        case 'n':
            if (chCurrent < '0' || chCurrent > '9')
                fOK = FALSE;
            break;
        case '+':
            if (chCurrent != '+' && chCurrent != '-')
                fOK = FALSE;
            break;
        default:
            if (chCurrent != *pszCurrTemplate)
                fOK = FALSE;
            else
                *pszCurrent = ' ';
            break;
        }
        pszCurrent++;
        pszCurrTemplate++;
    }

    /* If invalid character, or didn't reach the ends of both strings
     * simultaneously, fail.
     */
    if (!fOK || *pszCurrent || *pszCurrTemplate)
        return MK_E_SYNTAX;

    HRESULT hres;
    int n;
    SYSTEMTIME st;

    /* We parse into SYSTEMTIME structure because it has separate fields for
     * the different components.  We then convert to net time (seconds since
     * Jan 1 1970) to easily add the timezone bias and compare with other
     * times.
     *
     * The sense of the bias sign is inverted because it indicates the direction
     * of the bias FROM UTC.  We want to use it to convert the specified time
     * back TO UTC.
     */

    int nBiasSign = -1;
    int nBiasNumber;
    pszCurrent = szTemp;
    hres = ParseNumber(&pszCurrent, &n);
    if (SUCCEEDED(hres) && n >= 1980) {
        st.wYear = (WORD)n;
        hres = ParseNumber(&pszCurrent, &n);
        if (SUCCEEDED(hres) && n <= 12) {
            st.wMonth = (WORD)n;
            hres = ParseNumber(&pszCurrent, &n);
            if (SUCCEEDED(hres) && n < 32) {
                st.wDay = (WORD)n;
                hres = ParseNumber(&pszCurrent, &n);
                if (SUCCEEDED(hres) && n <= 23) {
                    st.wHour = (WORD)n;
                    hres = ParseNumber(&pszCurrent, &n);
                    if (SUCCEEDED(hres) && n <= 59) {
                        st.wMinute = (WORD)n;
                        if (*(pszCurrent++) == '-')
                            nBiasSign = 1;
                        hres = ParseNumber(&pszCurrent, &nBiasNumber);
                    }
                }
            }
        }
    }

    /* Seconds are used by the time converter, but are not specified in
     * the label.
     */
    st.wSecond = 0;

    /* Other fields (wDayOfWeek, wMilliseconds) are ignored when converting
     * to net time.
     */

    if (FAILED(hres))
        return hres;

    DWORD dwTime = SystemToNetDate(&st);

    /* The bias number is 4 digits, but hours and minutes.  Convert to
     * a number of seconds.
     */
    nBiasNumber = (((nBiasNumber / 100) * 60) + (nBiasNumber % 100)) * 60;

    /* Adjust the time by the timezone bias, and return to the caller. */
    *pOut = dwTime + (nBiasNumber * nBiasSign);

    return hres;
}


/* ParseOptions parses through any label options that may be present at
 * m_pszCurrent.  pszTokenEnd initially points to the end of the token at
 * m_pszCurrent, a small perf win since the caller has already calculated
 * it.  If ParseOptions is filling in the static options structure embedded
 * in the serviceinfo, pOpt points to it and ppOptOut will be NULL.  If pOpt
 * is NULL, then ParseOptions will construct a new CDynamicOptions object
 * and return it in *ppOptOut, iff any new options are found at the current
 * token.  pszOptionEndToken indicates the token which ends the list of
 * options -- either "labels" or "ratings".  A token consisting of just the
 * first character of pszOptionEndToken will also terminate the list.
 *
 * ParseOptions fails iff it finds an option it doesn't recognize, or a
 * syntax error in an option it does recognize.  It succeeds if all options
 * are syntactically correct or if there are no options to parse.
 *
 * The token which terminates the list of options is also consumed.
 *
 * BUGBUG - how should we flag mandatory extensions, 'until' options that
 * give an expired date, etc.?  set a flag in the CParsedServiceInfo and
 * keep parsing?
 */

enum OptionID {
    OID_AT,
    OID_BY,
    OID_COMMENT,
    OID_FULL,
    OID_EXTENSION,
    OID_GENERIC,
    OID_FOR,
    OID_MIC,
    OID_ON,
    OID_SIG,
    OID_UNTIL
};

enum OptionContents {
    OC_QUOTED,
    OC_BOOL,
    OC_SPECIAL
};

const struct {
    LPCSTR pszToken;
    OptionID oid;
    OptionContents oc;
} aKnownOptions[] = {
    { szAtOption, OID_AT, OC_QUOTED },
    { szByOption, OID_BY, OC_QUOTED },
    { szCommentOption, OID_COMMENT, OC_QUOTED },
    { szCompleteLabelOption, OID_FULL, OC_QUOTED },
    { szFullOption, OID_FULL, OC_QUOTED },
    { szExtensionOption, OID_EXTENSION, OC_SPECIAL },
    { szGenericOption, OID_GENERIC, OC_BOOL },
    { szShortGenericOption, OID_GENERIC, OC_BOOL },
    { szForOption, OID_FOR, OC_QUOTED },
    { szMICOption, OID_MIC, OC_QUOTED },
    { szMD5Option, OID_MIC, OC_QUOTED },
    { szOnOption, OID_ON, OC_QUOTED },
    { szSigOption, OID_SIG, OC_QUOTED },
    { szUntilOption, OID_UNTIL, OC_QUOTED },
    { szExpOption, OID_UNTIL, OC_QUOTED }
};

const UINT cKnownOptions = sizeof(aKnownOptions) / sizeof(aKnownOptions[0]);
    

HRESULT CParsedServiceInfo::ParseOptions(LPSTR pszTokenEnd, COptionsBase *pOpt,
                             CDynamicOptions **ppOptOut, LPCSTR pszOptionEndToken)
{
    HRESULT hres = NOERROR;
    char szShortOptionEndToken[2];

    szShortOptionEndToken[0] = *pszOptionEndToken;
    szShortOptionEndToken[1] = '\0';

    if (pszTokenEnd == NULL)
        pszTokenEnd = FindTokenEnd(m_pszCurrent);

    do {
        /* Have we hit the token that signals the end of the options? */
        if (IsEqualToken(m_pszCurrent, pszTokenEnd, pszOptionEndToken) ||
            IsEqualToken(m_pszCurrent, pszTokenEnd, szShortOptionEndToken)) {
            m_pszCurrent = pszTokenEnd;
            SkipWhitespace();
            return NOERROR;
        }

        for (UINT i=0; i<cKnownOptions; i++) {
            if (IsEqualToken(m_pszCurrent, pszTokenEnd, aKnownOptions[i].pszToken)) {
                break;
            }
        }

        if (i == cKnownOptions) {
            m_pszInvalidString = m_pszCurrent;
            return ResultFromScode(MK_E_SYNTAX);    /* unrecognized option */
        }

        m_pszCurrent = pszTokenEnd;
        SkipWhitespace();

        /* Now parse the stuff that comes after the option token. */
        LPSTR pszQuotedString = NULL;
        BOOL fBoolOpt = FALSE;
        switch (aKnownOptions[i].oc) {
        case OC_QUOTED:
            hres = GetQuotedToken(&m_pszCurrent, &pszQuotedString);
            break;

        case OC_BOOL:
            hres = GetBool(&m_pszCurrent, &fBoolOpt);
            break;

        case OC_SPECIAL:
            break;          /* we'll handle this specially */
        }

        if (FAILED(hres)) { /* incorrect stuff after the option token */
            m_pszInvalidString = m_pszCurrent;
            return hres;
        }

        if (pOpt == NULL) {     /* need to allocate a new options structure */
            CDynamicOptions *pNew = new CDynamicOptions;
            if (pNew == NULL)
                return ResultFromScode(E_OUTOFMEMORY);
            pOpt = pNew;
            *ppOptOut = pNew;   /* return new structure to caller */
        }

        /* Now actually do useful stuff based on which option it is. */
        switch (aKnownOptions[i].oid) {
        case OID_UNTIL:
            hres = ParseTime(pszQuotedString, &pOpt->m_timeUntil);
            if (FAILED(hres))
                m_pszInvalidString = pszQuotedString;
            break;

        case OID_FOR:
            pOpt->m_pszURL = pszQuotedString;
            break;

        case OID_GENERIC:
            if (fBoolOpt)
                pOpt->m_fdwFlags |= LBLOPT_GENERIC;
            else
                pOpt->m_fdwFlags &= ~LBLOPT_GENERIC;
            break;

        case OID_EXTENSION:
            hres = ParseExtension(pOpt);
            break;
        }

        SkipWhitespace();

        pszTokenEnd = FindTokenEnd(m_pszCurrent);
    } while (SUCCEEDED(hres));

    return hres;
}


/* CParsedServiceInfo::ParseRating parses a single rating -- a transmit-name
 * followed by either a number or a parenthesized list of multi-values.  The
 * corresponding rating is stored in the current list of ratings.
 */
HRESULT CParsedServiceInfo::ParseRating()
{
    LPSTR pszTokenEnd = FindTokenEnd(m_pszCurrent);
    if (*m_pszCurrent == '\0') {
        return ResultFromScode(MK_E_SYNTAX);
    }

    *(pszTokenEnd++) = '\0';

    CParsedRating r;

    r.pszTransmitName = m_pszCurrent;
    m_pszCurrent = pszTokenEnd;
    SkipWhitespace();

    HRESULT hres = ParseNumber(&m_pszCurrent, &r.nValue);
    if (FAILED(hres)) {
        m_pszInvalidString = m_pszCurrent;
        return hres;
    }

    r.pOptions = m_poptCurrent;
    r.fFound = FALSE;
    r.fFailed = FALSE;

    return (aRatings.Append(r) ? NOERROR : ResultFromScode(E_OUTOFMEMORY));
}


/* CParsedServiceInfo::ParseSingleLabel starts parsing where a single-label
 * should occur.  A single-label may contain options (in which case a new
 * options structure will be allocated), following by the keyword 'ratings'
 * (or 'r') and a parenthesized list of ratings.
 */
HRESULT CParsedServiceInfo::ParseSingleLabel()
{
    HRESULT hres;
    CDynamicOptions *pOpt = NULL;

    hres = ParseOptions(NULL, NULL, &pOpt, szRatings);
    if (FAILED(hres)) {
        if (pOpt != NULL)
            pOpt->Release();
        return hres;
    }
    if (pOpt != NULL) {
        pOpt->m_pNext = m_poptList;
        m_poptList = pOpt;
        m_poptCurrent = pOpt;
    }

    hres = ParseLiteralToken(&m_pszCurrent, szLeftParen, &m_pszInvalidString);
    if (FAILED(hres))
        return hres;

    do {
        hres = ParseRating();
    } while (SUCCEEDED(hres) && *m_pszCurrent != ')' && *m_pszCurrent != '\0');

    if (FAILED(hres))
        return hres;

    return ParseLiteralToken(&m_pszCurrent, szRightParen, &m_pszInvalidString);
}


/* CParsedServiceInfo::ParseLabels starts parsing just past the keyword
 * 'labels' (or 'l').  It needs to handle a label-error, a single-label,
 * or a parenthesized list of single-labels.
 */
HRESULT CParsedServiceInfo::ParseLabels()
{
    HRESULT hres;

    /* First deal with a label-error.  It begins with the keyword 'error'. */
    if (SUCCEEDED(ParseLiteralToken(&m_pszCurrent, szError, NULL))) {
        hres = ParseLiteralToken(&m_pszCurrent, szLeftParen, &m_pszInvalidString);
        if (FAILED(hres))
            return hres;
        LPSTR pszTokenEnd = FindTokenEnd(m_pszCurrent);
        m_pszErrorString = m_pszCurrent;
        m_pszCurrent = pszTokenEnd;
        SkipWhitespace();

        while (*m_pszCurrent != ')') {
            hres = GetQuotedToken(&m_pszCurrent, NULL);
            if (FAILED(hres)) {
                m_pszInvalidString = m_pszCurrent;
                return hres;
            }
        }
        return NOERROR;
    }

    BOOL fParenthesized = FALSE;

    /* If we see a left paren, it's a parenthesized list of single-labels,
     * which basically means we'll have to eat an extra parenthesis later.
     */
    if (SUCCEEDED(ParseLiteralToken(&m_pszCurrent, szLeftParen, NULL)))
        fParenthesized = TRUE;

    for (;;) {
        /* Things which signify the end of the label list:
         * - the close parenthesis checked for above
         * - a quoted string, indicating the next service-info
         * - the end of the string
         * - a service-info saying "error (no-ratings <explanation>)"
         *
         * Check the easy ones first.
         */
        if (*m_pszCurrent == ')' || *m_pszCurrent == '\"' || *m_pszCurrent == '\0')
            break;

        /* Now look for that tricky error-state service-info. */
        LPSTR pszTemp = m_pszCurrent;
        if (SUCCEEDED(ParseLiteralToken(&pszTemp, szError, NULL)) &&
            SUCCEEDED(ParseLiteralToken(&pszTemp, szLeftParen, NULL)) &&
            SUCCEEDED(ParseLiteralToken(&pszTemp, szNoRatings, NULL)))
            break;

        hres = ParseSingleLabel();
        if (FAILED(hres))
            return hres;
    }

    if (fParenthesized)
        return ParseLiteralToken(&m_pszCurrent, szRightParen, &m_pszInvalidString);

    return NOERROR;
}


/* Parse is passed a pointer to a pointer to something which should
 * be a service-info string (i.e., not the close paren for the labellist, and
 * not the end of the string).  The caller's string pointer is advanced to the
 * end of the service-info string.
 */
HRESULT CParsedServiceInfo::Parse(LPSTR *ppszServiceInfo)
{
    /* NOTE: Do not return out of this function without copying m_pszCurrent
     * back into *ppszServiceInfo!  Always store your return code in hres and
     * exit out the bottom of the function.
     */
    HRESULT hres;

    m_pszCurrent = *ppszServiceInfo;

    hres = ParseLiteralToken(&m_pszCurrent, szError, NULL);
    if (SUCCEEDED(hres)) {
        /* Keyword is 'error'.  Better be followed by '(', 'no-ratings',
         * explanations, and a close-paren.
         */
        hres = ParseLiteralToken(&m_pszCurrent, szLeftParen, &m_pszInvalidString);
        if (SUCCEEDED(hres))
            hres = ParseLiteralToken(&m_pszCurrent, szNoRatings, &m_pszInvalidString);
        if (SUCCEEDED(hres)) {
            m_pszErrorString = szNoRatings;

            while (*m_pszCurrent != ')' && *m_pszCurrent != '\0') {
                hres = GetQuotedToken(&m_pszCurrent, NULL);
                if (FAILED(hres)) {
                    m_pszInvalidString = m_pszCurrent;
                    break;
                }
                SkipWhitespace();
            }
            if (*m_pszCurrent == ')') {
                m_pszCurrent++;
                SkipWhitespace();
            }
        }
    }
    else {
        /* Keyword is not 'error'.  Better start with a serviceID --
         * a quoted URL.
         */
        LPSTR pszServiceID;
        hres = GetQuotedToken(&m_pszCurrent, &pszServiceID);
        if (SUCCEEDED(hres)) {
            m_pszServiceName = pszServiceID;

            SkipWhitespace();

            /* Past the serviceID.  Next either 'error' indicating a service-error,
             * or we start options and then a labelword.
             */

            LPSTR pszTokenEnd = FindTokenEnd(m_pszCurrent);

            if (IsEqualToken(m_pszCurrent, pszTokenEnd, szError)) {
                m_pszCurrent = pszTokenEnd;
                SkipWhitespace();
                hres = ParseServiceError();
            }
            else {
                hres = ParseOptions(pszTokenEnd, &m_opt, NULL, ::szLabelWord);
                if (SUCCEEDED(hres)) {
                    hres = ParseLabels();
                }
            }
        }
        else {
            m_pszInvalidString = m_pszCurrent;
        }
    }

    *ppszServiceInfo = m_pszCurrent;
    return hres;
}


const char szPicsVersionLabel[] = "PICS-";
const UINT cchLabel = (sizeof(szPicsVersionLabel)-1) / sizeof(szPicsVersionLabel[0]);

HRESULT CParsedLabelList::Parse(LPSTR pszCopy)
{
    m_pszList = pszCopy;                /* we own the label list string now */

    /* Make another copy, which we won't carve up during parsing, so that the
     * access-denied dialog can compare literal labels.
     */
    m_pszOriginalLabel = new char[::strlenf(pszCopy)+1];
    if (m_pszOriginalLabel != NULL)
        ::strcpyf(m_pszOriginalLabel, pszCopy);

    m_pszCurrent = m_pszList;

    SkipWhitespace();

    HRESULT hres;

    hres = ParseLiteralToken(&m_pszCurrent, szLeftParen, &m_pszInvalidString);
    if (FAILED(hres))
        return hres;

    if (strnicmpf(m_pszCurrent, szPicsVersionLabel, cchLabel))
        return MK_E_SYNTAX;

    m_pszCurrent += cchLabel;
    INT nVersion;
    hres = ParseNumber(&m_pszCurrent, &nVersion);
    if (FAILED(hres))
        return hres;

    CParsedServiceInfo *psi = &m_ServiceInfo;

    do {
        hres = psi->Parse(&m_pszCurrent);
        if (FAILED(hres))
            return hres;
        if (*m_pszCurrent != ')' && *m_pszCurrent != '\0') {
            CParsedServiceInfo *pNew = new CParsedServiceInfo;
            if (pNew == NULL)
                return ResultFromScode(E_OUTOFMEMORY);
            psi->Append(pNew);
            psi = pNew;
        }
    } while (*m_pszCurrent != ')' && *m_pszCurrent != '\0');

    return NOERROR;
}


HRESULT ParseLabelList(LPCSTR pszList, CParsedLabelList **ppParsed)
{
    LPSTR pszCopy = new char[strlenf(pszList)+1];
    if (pszCopy == NULL) {
        return ResultFromScode(E_OUTOFMEMORY);
    }
    ::strcpyf(pszCopy, pszList);

    *ppParsed = new CParsedLabelList;
    if (*ppParsed == NULL) {
        delete pszCopy;
        return ResultFromScode(E_OUTOFMEMORY);
    }

    return (*ppParsed)->Parse(pszCopy);
}


void FreeParsedLabelList(CParsedLabelList *pList)
{
    delete pList;
}
