//t-markh 8/98
//extends GetBool() to true/false, yes/no, pass/fail for
//PICSRules support
enum PICSRulesBooleanSwitch
{
    PR_BOOLEAN_TRUEFALSE,
    PR_BOOLEAN_PASSFAIL,
    PR_BOOLEAN_YESNO
};

//t-markh 8/98
//Definitions used by GetBool for PICSRules types
#define PR_YESNO_YES    1
#define PR_YESNO_NO     0

#define PR_PASSFAIL_PASS    1
#define PR_PASSFAIL_FAIL    0

void SkipWhitespace(LPSTR *ppsz);
BOOL IsEqualToken(LPCSTR pszTokenStart, LPCSTR pszTokenEnd, LPCSTR pszTokenToMatch);
LPSTR FindTokenEnd(LPSTR pszStart);
HRESULT GetBool(LPSTR *ppszToken, BOOL *pfOut, PICSRulesBooleanSwitch PRBoolSwitch=PR_BOOLEAN_TRUEFALSE);
HRESULT ParseNumber(LPSTR *ppszNumber, INT *pnOut, BOOL fPICSRules=FALSE);
HRESULT ParseTime(LPSTR pszTime, DWORD *pOut, BOOL fPICSRules=FALSE);

class COptionsBase
{
protected:
    COptionsBase();
    UINT m_cRef;

public:
    void AddRef();
    void Release();
    virtual void Delete();

    DWORD m_timeUntil;          /* 'until' time, in UTC net format (secs since 1/1/1970) */

    DWORD m_fdwFlags;           /* see LBLOPT_XXX below */

    LPSTR m_pszInvalidString;   /* ptr to invalid or unrecognized string */
    LPSTR m_pszURL;             /* value of "for" option, may be NULL */

    BOOL CheckUntil(DWORD timeCurrent); /* returns TRUE if 'until' option checks out */
    BOOL CheckURL(LPCSTR pszURL);       /* returns TRUE if 'for' option checks out */
};

const DWORD LBLOPT_GENERIC = 0x01;
const DWORD LBLOPT_INVALID = 0x02;
const DWORD LBLOPT_EXPIRED = 0x04;
const DWORD LBLOPT_WRONGURL = 0x08;
const DWORD LBLOPT_URLCHECKED = 0x10;


class CStaticOptions : public COptionsBase
{
public:
    CStaticOptions() { }
};


class CDynamicOptions : public COptionsBase
{
public:
    CDynamicOptions *m_pNext;

    CDynamicOptions() { m_pNext = NULL; }
    virtual void Delete();
};


class CParsedRating
{
public:
    LPSTR pszTransmitName;
    INT nValue;
    COptionsBase *pOptions;
    BOOL fFound;                /* TRUE if matches a rating in the installed system */
    BOOL fFailed;               /* TRUE if exceeded user's limit */
};


class CParsedServiceInfo
{
private:
    CParsedServiceInfo *m_pNext;
    COptionsBase *m_poptCurrent;
    CDynamicOptions *m_poptList;
    LPSTR m_pszCurrent;             /* for parsing routines */
    void SkipWhitespace() { ::SkipWhitespace(&m_pszCurrent); }

public:
    LPCSTR m_pszServiceName;        /* service name URL, may be NULL if not reported */
    LPCSTR m_pszErrorString;        /* points to error string if error reported by site */
    LPCSTR m_pszInvalidString;      /* pointer to invalid or unrecognized string found */
    BOOL m_fInstalled;              /* TRUE if this rating system is installed on this machine */
    CStaticOptions m_opt;
    array<CParsedRating> aRatings;

    CParsedServiceInfo();
    ~CParsedServiceInfo();

    CParsedServiceInfo *Next() { return m_pNext; }
    void Append(CParsedServiceInfo *pNew);
    HRESULT Parse(LPSTR *ppszServiceInfo);
    HRESULT ParseServiceError();
    HRESULT ParseOptions(LPSTR pszTokenEnd, COptionsBase *pOpt,
                     CDynamicOptions **ppOptOut, LPCSTR pszOptionEndToken);
    HRESULT ParseExtension(COptionsBase *pOpt);
    HRESULT ParseExtensionData(COptionsBase *pOpt);
    HRESULT ParseRating();
    HRESULT ParseSingleLabel();
    HRESULT ParseLabels();
};


class CParsedLabelList
{
private:
    LPSTR m_pszList;
    LPSTR m_pszCurrent;
    HRESULT ParseServiceInfo();
    void SkipWhitespace() { ::SkipWhitespace(&m_pszCurrent); }

public:
    CParsedServiceInfo m_ServiceInfo;
    LPCSTR m_pszInvalidString;      /* pointer to invalid or unrecognized string found */
    LPSTR m_pszURL;                 /* copy of URL we were originally given */
    LPSTR m_pszOriginalLabel;      /* copy of original, raw rating label */

    BOOL m_fRated;                      /* TRUE if site is considered rated */

    CParsedLabelList();
    ~CParsedLabelList();

    HRESULT Parse(LPSTR pszCopy);
};


extern "C" {
HRESULT ParseLabelList(LPCSTR pszList, CParsedLabelList **ppParsed);
void FreeParsedLabelList(CParsedLabelList *pList);
};  /* extern "C" */
