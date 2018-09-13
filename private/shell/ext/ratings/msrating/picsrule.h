//*******************************************************************
//*
//* PICSRule.h
//*
//* Revision History:
//*     Created 7/98 - Mark Hammond (t-markh)
//*
//* Contains classes and function prototypes
//* used for processing PICSRules files.
//*
//*******************************************************************

//*******************************************************************
//*
//* Function Prototypes / class declarations
//*
//*******************************************************************

//Forward class declarations
class PICSRulesQuotedURL;
class PICSRulesQuotedEmail;
class PICSRulesQuotedDate;
class PICSRulesYesNo;
class PICSRulesPassFail;
class PICSRulesPolicyExpression;
class PICSRulesByURL;
class PICSRulesPolicy;
class PICSRulesName;
class PICSRulesSource;
class PICSRulesServiceInfo;
class PICSRulesOptExtension;
class PICSRulesReqExtension;
class PICSRulesFileParser;
class PICSRulesRatingSystem;
class CParsedLabelList;

//This function is called by ApprovedSitesDlgProc while processing
//WM_COMMOND with LOWORD(wParam)==IDC_PICSRULESOPEN in msludlg.cpp
//The argument lpszFileName is the name of the PICSRules file
//selected by the user to import.
//
//This begins the PICSRules Import process.
HRESULT PICSRulesImport(char *lpszFileName, PICSRulesRatingSystem **pprrsOut);

//For reading and saving processed PICSRules from the registry
HRESULT PICSRulesSaveToRegistry(DWORD dwSystemToSave, PICSRulesRatingSystem **ppPRRS);
HRESULT PICSRulesReadNameFromRegistry(DWORD dwSystemToRead, LPTSTR *ppszSystemName);
HRESULT PICSRulesReadFromRegistry(DWORD dwSystemToRead, PICSRulesRatingSystem **ppPRRS);
HRESULT PICSRulesDeleteSystem(DWORD dwSystemToDelete);
HRESULT PICSRulesSetNumSystems(DWORD dwNumSystems);
HRESULT PICSRulesGetNumSystems(DWORD * pdwNumSystems);
HRESULT PICSRulesCheckApprovedSitesAccess(LPCSTR lpszUrl,BOOL *fPassFail);
HRESULT PICSRulesCheckAccess(LPCSTR lpszUrl,LPCSTR lpszRatingInfo,BOOL *fPassFail,CParsedLabelList **ppParsed);
void PICSRulesOutOfMemory();

//The following are handler functions which parse the various
//kinds of content which can occur within a parenthesized object.
//
//ppszIn is always advanced to the next non-white space token
//ppszOut returns the processed data
HRESULT PICSRulesParseString(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);
HRESULT PICSRulesParseNumber(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);
HRESULT PICSRulesParseYesNo(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);
HRESULT PICSRulesParsePassFail(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);

BOOL IsURLValid(WCHAR wcszURL[]);

//*******************************************************************
//*
//* Definitions used by the PICSRules code
//*
//*******************************************************************
#define PR_QUOTE_DOUBLE     1
#define PR_QUOTE_SINGLE     0

#define BYURL_SCHEME    1
#define BYURL_USER      2
#define BYURL_HOST      4
#define BYURL_PORT      8
#define BYURL_PATH      16

#define PICSRULES_FIRSTSYSTEMINDEX  100
#define PICSRULES_MAXSYSTEM         1000000
#define PICSRULES_APPROVEDSITES     0

#define PICSRULES_ALWAYS            1
#define PICSRULES_NEVER             0

#define PICSRULES_PAGE              1
#define PICSRULES_SITE              0

#define PICS_LABEL_FROM_HEADER      0
#define PICS_LABEL_FROM_PAGE        1
#define PICS_LABEL_FROM_BUREAU      2

struct PICSRULES_VERSION
{
    int iPICSRulesVerMajor,iPICSRulesVerMinor;
};

//Data types for the necessary logic in evaluating
//the rules.
enum PICSRulesOperators
{
    PR_OPERATOR_INVALID,
    PR_OPERATOR_GREATEROREQUAL,
    PR_OPERATOR_GREATER,
    PR_OPERATOR_EQUAL,
    PR_OPERATOR_LESSOREQUAL,
    PR_OPERATOR_LESS,
    PR_OPERATOR_DEGENERATE,
    PR_OPERATOR_SERVICEONLY,
    PR_OPERATOR_SERVICEANDCATEGORY,
    PR_OPERATOR_RESULT
};

enum PICSRulesEvaluation
{
    PR_EVALUATION_DOESAPPLY,
    PR_EVALUATION_DOESNOTAPPLY
};

//This indicates which member is valid in a PICSRulesPolicy
//Class
enum PICSRulesPolicyAttribute
{
    PR_POLICY_NONEVALID,
    PR_POLICY_REJECTBYURL,
    PR_POLICY_ACCEPTBYURL,
    PR_POLICY_REJECTIF,
    PR_POLICY_ACCEPTIF,
    PR_POLICY_REJECTUNLESS,
    PR_POLICY_ACCEPTUNLESS
};

//This indicates if a PolicyExpression is embedded in another
//PolicyExpression, and if so, what logic to use.
enum PICSRulesPolicyEmbedded
{
    PR_POLICYEMBEDDED_NONE,
    PR_POLICYEMBEDDED_OR,
    PR_POLICYEMBEDDED_AND
};

//*******************************************************************
//*
//* Classes to handle possible value types... all derive from
//* ETS (the encapsulated string type) since the are found as
//* strings during processing.
//*
//* The Set() and SetTo() member functions are overloaded on the
//* types which convert from a string.  We want to keep the original
//* string in case the data is invalid.
//*
//* Additional member functions are provided to assure that the
//* data is what it says it is, and to return the non-string type.
//*
//*******************************************************************
class PICSRulesByURLExpression
{
    public:
        PICSRulesByURLExpression();
        ~PICSRulesByURLExpression();

        BOOL    m_fInternetPattern;
        BYTE    m_bNonWild,m_bSpecified;
        ETS     m_etstrScheme,m_etstrUser,m_etstrHost,m_etstrPort,m_etstrPath,m_etstrURL;
};

class PICSRulesQuotedURL : public ETS
{
    public:
        PICSRulesQuotedURL();
        ~PICSRulesQuotedURL();

        BOOL IsURLValid();
        BOOL IsURLValid(char * lpszURL);
        BOOL IsURLValid(ETS etstrURL);
};

class PICSRulesQuotedEmail : public ETS
{
    public:
        PICSRulesQuotedEmail();
        ~PICSRulesQuotedEmail();

        BOOL IsEmailValid();
        BOOL IsEmailValid(char * lpszEmail);
        BOOL IsEmailValid(ETS etstrEmail);
};

class PICSRulesQuotedDate : public ETS
{
    public:
        PICSRulesQuotedDate();
        ~PICSRulesQuotedDate();

        BOOL IsDateValid();
        BOOL IsDateValid(char * lpszDate);
        BOOL IsDateValid(ETS etstrDate);

        HRESULT Set(const char *pIn);
        HRESULT SetTo(char *pIn);

        DWORD GetDate()
        {
            return m_dwDate;
        };

    private:
        DWORD m_dwDate;
};

class PICSRulesYesNo : public ETS
{
    public:
        PICSRulesYesNo();
        ~PICSRulesYesNo();

        void Set(const BOOL *pIn);
        void SetTo(BOOL *pIn);

        BOOL GetYesNo()
        {
            return m_fYesOrNo;
        };

    private:
        BOOL m_fYesOrNo;
};
        
class PICSRulesPassFail : public ETS
{
    public:
        PICSRulesPassFail();
        ~PICSRulesPassFail();

        void Set(const BOOL *pIn);
        void SetTo(BOOL *pIn);

        BOOL GetPassFail()
        {
            return m_fPassOrFail;
        };

    private:
        BOOL m_fPassOrFail;
};

//*******************************************************************
//*
//* The PICSRulesPolicy class handles the "Policy" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//*     Policy (
//*         [Explanation]   quoted
//*         RejectByURL     URL | ( [patterns]  URL )
//*         AcceptByURL     URL | ( [patterns]  URL )
//*         RejectIf        PolicyExpression
//*         RejectUnless    PolicyExpression
//*         AcceptIf        PolicyExpression
//*         AcceptUnless    PolicyExpression
//*         *Extension* )
//*
//*******************************************************************

class PICSRulesPolicyExpression
{
    public:
        PICSRulesPolicyExpression       * m_pPRPolicyExpressionLeft,
                                        * m_pPRPolicyExpressionRight;
        ETS                             m_etstrServiceName,m_etstrCategoryName,
                                        m_etstrFullServiceName;
        ETN                             m_etnValue;
        PICSRulesYesNo                  m_prYesNoUseEmbedded;
        enum PICSRulesOperators         m_PROPolicyOperator;
        enum PICSRulesPolicyEmbedded    m_PRPEPolicyEmbedded;

        PICSRulesPolicyExpression();
        ~PICSRulesPolicyExpression();

        PICSRulesEvaluation EvaluateRule(CParsedLabelList *pParsed);
};

class PICSRulesByURL
{
    public:
        array<PICSRulesByURLExpression*> m_arrpPRByURL;

        PICSRulesByURL();
        ~PICSRulesByURL();

        PICSRulesEvaluation EvaluateRule(PICSRulesQuotedURL *pprurlComparisonURL);
};

class PICSRulesPolicy : public PICSRulesObjectBase
{
    public:
        ETS                                 m_etstrExplanation;
        PICSRulesPolicyExpression           * m_pPRRejectIf,* m_pPRRejectUnless,
                                            * m_pPRAcceptIf,* m_pPRAcceptUnless;
        PICSRulesByURL                      * m_pPRRejectByURL,* m_pPRAcceptByURL;
        PICSRulesPolicyAttribute            m_PRPolicyAttribute;
        
        PICSRulesPolicy();
        ~PICSRulesPolicy();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();
};

//*******************************************************************
//*
//* The PICSRulesName class handles the "name" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//*     name (
//*         [Rulename]  quoted
//*         Description quoted
//*         *Extension* )
//*
//*******************************************************************
class PICSRulesName : public PICSRulesObjectBase
{
    public:
        ETS m_etstrRuleName,m_etstrDescription;

        PICSRulesName();
        ~PICSRulesName();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();
};

//*******************************************************************
//*
//* The PICSRulesSource class handles the "source" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//*     source (
//*         [SourceURL]     URL
//*         CreationTool    quoted (has format toolname/version)
//*         author          email
//*         LastModified    ISO Date
//*         *Extension* )
//*
//*******************************************************************
class PICSRulesSource : public PICSRulesObjectBase
{
    public:
        PICSRulesQuotedURL      m_prURLSourceURL;
        ETS                     m_etstrCreationTool;
        PICSRulesQuotedEmail    m_prEmailAuthor;
        PICSRulesQuotedDate     m_prDateLastModified;
        
        PICSRulesSource();
        ~PICSRulesSource();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();

        char * GetToolName();
};

//*******************************************************************
//*
//* The PICSRulesServiceInfo class handles the "serviceinfo" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//* The Ratfile attribute is either an entire machine readable .RAT
//* file, or the URL where the .RAT file can be obtained.
//*
//*     serviceinfo (
//*         [Name]              URL
//*         shortname           quoted
//*         BureauURL           URL
//*         UseEmbedded         Y|N
//*         Ratfile             quoted
//*         BureauUnavailable   PASS|FAIL
//*         *Extension* )
//*
//*******************************************************************
class PICSRulesServiceInfo : public PICSRulesObjectBase
{
    public:
        PICSRulesQuotedURL      m_prURLName,m_prURLBureauURL;
        ETS                     m_etstrShortName,m_etstrRatfile;
        PICSRulesYesNo          m_prYesNoUseEmbedded;
        PICSRulesPassFail       m_prPassFailBureauUnavailable;

        PICSRulesServiceInfo();
        ~PICSRulesServiceInfo();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();
};

//*******************************************************************
//*
//* The PICSRulesOptExtension class handles the "optextension" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//*     optextension (
//*         [extension-name]    URL
//*         shortname           quoted
//*         *Extension* )
//*
//*******************************************************************
class PICSRulesOptExtension : public PICSRulesObjectBase
{
    public:
        PICSRulesQuotedURL      m_prURLExtensionName;
        ETS                     m_etstrShortName;

        PICSRulesOptExtension();
        ~PICSRulesOptExtension();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();
};

//*******************************************************************
//*
//* The PICSRulesReqExtension class handles the "reqextension" token
//* from a PICSRules stream, and its attributes (square brackets
//* denote the primary attribute):
//*
//*     reqextension (
//*         [extension-name]    URL
//*         shortname           quoted
//*         *Extension* )
//*
//*******************************************************************
class PICSRulesReqExtension : public PICSRulesObjectBase
{
    public:
        PICSRulesQuotedURL      m_prURLExtensionName;
        ETS                     m_etstrShortName;

        PICSRulesReqExtension();
        ~PICSRulesReqExtension();

        HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData);
        HRESULT InitializeMyDefaults();
};

//*******************************************************************
//*
//* The PICSRulesRatingSystem class encapsulates all of the
//* information from a give PICSRules source.  Multiple
//* instantiations are created in the PicsRatingSystemInfo class,
//* created at startup and stored in gPRSI.
//*
//*******************************************************************
class PICSRulesRatingSystem : public PICSRulesObjectBase
{
    public:
        array<PICSRulesPolicy*>         m_arrpPRPolicy;
        array<PICSRulesServiceInfo*>    m_arrpPRServiceInfo;
        array<PICSRulesOptExtension*>   m_arrpPROptExtension;
        array<PICSRulesReqExtension*>   m_arrpPRReqExtension;
        PICSRulesName                   * m_pPRName;
        PICSRulesSource                 * m_pPRSource;
        ETS                             m_etstrFile;
        ETN                             m_etnPRVerMajor,m_etnPRVerMinor;
        DWORD                           m_dwFlags;
        UINT                            m_nErrLine;

        PICSRulesRatingSystem();
        ~PICSRulesRatingSystem();
        HRESULT Parse(LPCSTR pszFile, LPSTR pStreamIn);

        HRESULT AddItem(PICSRulesObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults();
        void ReportError(HRESULT hres);
};

//*******************************************************************
//*
//* The PICSRulesFileParser class exists to provide a line number
//* shared by all the parsing routines.  This line number is updated
//* as the parser walks through the stream, and is frozen as soon as
//* an error is found.  This line number can later be reported to the
//* user to help localize errors.
//*
//*******************************************************************
class PICSRulesFileParser
{
public:
    UINT m_nLine;

    PICSRulesFileParser() { m_nLine = 1; }

    LPSTR EatQuotedString(LPSTR pIn,BOOL fQuote=PR_QUOTE_DOUBLE);
    HRESULT ParseToOpening(LPSTR *ppIn, PICSRulesAllowableOption  *paoExpected,
                           PICSRulesAllowableOption  **ppFound);
    HRESULT ParseParenthesizedObject(
        LPSTR *ppIn,                    //where we are in the text stream
        PICSRulesAllowableOption aao[], //allowable things inside this object
        PICSRulesObjectBase *pObject    //object to set parameters into
    );
    char* FindNonWhite(char *pc);
};
