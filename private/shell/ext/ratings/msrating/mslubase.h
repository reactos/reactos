/****************************************************************************\
 *
 *   PICS.H --Structures for holding pics information
 *
 *   Created:   Jason Thomas
 *   Updated:   Ann McCurdy
 *   
\****************************************************************************/

#ifndef _PICS_H_
#define _PICS_H_

/*Includes---------------------------------------------------------*/
#include <npassert.h>
#include "array.h"
#include "resource.h"
#include "msluglob.h"

extern char PicsDelimChar;

#define PICS_FILE_BUF_LEN   20000
#define PICS_STRING_BUF_LEN  1000

#define P_INFINITY           9999
#define N_INFINITY          -9999

/*Simple PICS types------------------------------------------------*/

#if 0
class ET{
    private:
        BOOL m_fInit;
    public:
        ET();
        void Init();
        void UnInit();
        BOOL fIsInit();
};
#endif

class ETN
{
    private:
        int r;
        BOOL m_fInit;
    public:
        ETN() { m_fInit = FALSE; }

        void Init() { m_fInit = TRUE; }
        void UnInit() { m_fInit = FALSE; }
        BOOL fIsInit() { return m_fInit; }

#ifdef DEBUG
        void  Set(int rIn);
        int Get();
#else
        void  Set(int rIn) { Init(); r = rIn; }
        int Get() { return r; }
#endif

        ETN*  Duplicate();
};

const UINT ETB_VALUE = 0x01;
const UINT ETB_ISINIT = 0x02;
class ETB
{
    private:
        UINT m_nFlags;
    public:
        ETB() { m_nFlags = 0; }

#ifdef DEBUG
        BOOL Get();
        void Set(BOOL b);
#else
        BOOL Get() { return m_nFlags & ETB_VALUE; }
        void Set(BOOL b) { m_nFlags = ETB_ISINIT | (b ? ETB_VALUE : 0); }
#endif

        ETB   *Duplicate();
        BOOL fIsInit() { return m_nFlags & ETB_ISINIT; }
};

class ETS
{
    private:
        char *pc;
    public:
        ETS() { pc = NULL; }
        ~ETS();
#ifdef DEBUG
        char* Get();
#else
        char *Get() { return pc; }
#endif
        void  Set(const char *pIn);
        ETS*  Duplicate();
        void  SetTo(char *pIn);

        BOOL fIsInit() { return pc != NULL; }
};

/*Complex PICS types-----------------------------------------------*/


enum RatObjectID
{
    ROID_INVALID,           /* dummy entry for terminating arrays */
    ROID_PICSDOCUMENT,      /* value representing the entire document (i.e., no token) */
    ROID_PICSVERSION,
    ROID_RATINGSYSTEM,
    ROID_RATINGSERVICE,
    ROID_RATINGBUREAU,
    ROID_BUREAUREQUIRED,
    ROID_CATEGORY,
    ROID_TRANSMITAS,
    ROID_LABEL,
    ROID_VALUE,
    ROID_DEFAULT,
    ROID_DESCRIPTION,
    ROID_EXTENSION,
    ROID_MANDATORY,
    ROID_OPTIONAL,
    ROID_ICON,
    ROID_INTEGER,
    ROID_LABELONLY,
    ROID_MAX,
    ROID_MIN,
    ROID_MULTIVALUE,
    ROID_NAME,
    ROID_UNORDERED
};

enum PICSRulesObjectID
{
    PROID_INVALID,                  /* dummy entry for terminating arrays */
    PROID_PICSVERSION,              /* for holding the PICSRules version ID */
    
    PROID_POLICY,                   /* for the Policy class */
        PROID_EXPLANATION,
        PROID_REJECTBYURL,
        PROID_ACCEPTBYURL,
        PROID_REJECTIF,
        PROID_ACCEPTIF,
        PROID_ACCEPTUNLESS,
        PROID_REJECTUNLESS,
    PROID_NAME,                     /* for the name class */
        PROID_RULENAME,
        PROID_DESCRIPTION,
    PROID_SOURCE,                   /* for the source class */
        PROID_SOURCEURL,
        PROID_CREATIONTOOL,
        PROID_AUTHOR,
        PROID_LASTMODIFIED,
    PROID_SERVICEINFO,              /* for the serviceinfo class */
        PROID_SINAME,
        PROID_SHORTNAME,
        PROID_BUREAUURL,
        PROID_USEEMBEDDED,
        PROID_RATFILE,
        PROID_BUREAUUNAVAILABLE,
    PROID_OPTEXTENSION,             /* for the optextension class */
        PROID_EXTENSIONNAME,
      //PROID_SHORTNAME,
    PROID_REQEXTENSION,
      //PROID_EXTENSIONNAME,
      //PROID_SHORTNAME,
    PROID_EXTENSION,

    PROID_POLICYDEFAULT,
    PROID_NAMEDEFAULT,
    PROID_SOURCEDEFAULT,
    PROID_SERVICEINFODEFAULT,
    PROID_OPTEXTENSIONDEFAULT,
    PROID_REQEXTENSIONDEFAULT
};

/* define some error codes */
const HRESULT RAT_E_BASE = 0x80050000;                  /* just a guess at a free area for internal use */
const HRESULT RAT_E_EXPECTEDLEFT    = RAT_E_BASE + 1;   /* expected left paren */
const HRESULT RAT_E_EXPECTEDRIGHT   = RAT_E_BASE + 2;   /* expected right paren */
const HRESULT RAT_E_EXPECTEDTOKEN   = RAT_E_BASE + 3;   /* expected unquoted token */
const HRESULT RAT_E_EXPECTEDSTRING  = RAT_E_BASE + 4;   /* expected quoted string */
const HRESULT RAT_E_EXPECTEDNUMBER  = RAT_E_BASE + 5;   /* expected number */
const HRESULT RAT_E_EXPECTEDBOOL    = RAT_E_BASE + 6;   /* expected boolean */
const HRESULT RAT_E_DUPLICATEITEM   = RAT_E_BASE + 7;   /* AO_SINGLE item appeared twice */
const HRESULT RAT_E_MISSINGITEM     = RAT_E_BASE + 8;   /* AO_MANDATORY item not found */
const HRESULT RAT_E_UNKNOWNITEM     = RAT_E_BASE + 9;   /* unrecognized token */
const HRESULT RAT_E_UNKNOWNMANDATORY= RAT_E_BASE + 10;  /* unrecognized mandatory extension */
const HRESULT RAT_E_EXPECTEDEND     = RAT_E_BASE + 11;  /* expected end of file */

/* echo for PICSRules with different names for clarity */
const HRESULT PICSRULES_E_BASE              = 0x80050000;       /* just a guess at a free area for internal use */
const HRESULT PICSRULES_E_EXPECTEDLEFT      = RAT_E_BASE + 1;   /* expected left paren */
const HRESULT PICSRULES_E_EXPECTEDRIGHT     = RAT_E_BASE + 2;   /* expected right paren */
const HRESULT PICSRULES_E_EXPECTEDTOKEN     = RAT_E_BASE + 3;   /* expected unquoted token */
const HRESULT PICSRULES_E_EXPECTEDSTRING    = RAT_E_BASE + 4;   /* expected quoted string */
const HRESULT PICSRULES_E_EXPECTEDNUMBER    = RAT_E_BASE + 5;   /* expected number */
const HRESULT PICSRULES_E_EXPECTEDBOOL      = RAT_E_BASE + 6;   /* expected boolean */
const HRESULT PICSRULES_E_DUPLICATEITEM     = RAT_E_BASE + 7;   /* AO_SINGLE item appeared twice */
const HRESULT PICSRULES_E_MISSINGITEM       = RAT_E_BASE + 8;   /* AO_MANDATORY item not found */
const HRESULT PICSRULES_E_UNKNOWNITEM       = RAT_E_BASE + 9;   /* unrecognized token */
const HRESULT PICSRULES_E_UNKNOWNMANDATORY  = RAT_E_BASE + 10;  /* unrecognized mandatory extension */
const HRESULT PICSRULES_E_EXPECTEDEND       = RAT_E_BASE + 11;  /* expected end of file */
const HRESULT PICSRULES_E_SERVICEUNDEFINED  = RAT_E_BASE + 12;  /* a service referenced is undefined */
const HRESULT PICSRULES_E_REQEXTENSIONUSED  = RAT_E_BASE + 13;  /* an unknown required extension was used */
const HRESULT PICSRULES_E_VERSION           = RAT_E_BASE + 14;  /* a non-support version was presented */

/* A RatObjectHandler parses the contents of a parenthesized object and
 * spits out a binary representation of that data, suitable for passing
 * to an object's AddItem function.  It does not consume the ')' which
 * closes the object.
 */
class RatFileParser;
typedef HRESULT (*RatObjectHandler)(LPSTR *ppszIn, LPVOID *ppOut, RatFileParser *pParser);

class PICSRulesFileParser;
typedef HRESULT (*PICSRulesObjectHandler)(LPSTR *ppszIn, LPVOID *ppOut, PICSRulesFileParser *pParser);

class PicsCategory;

class PicsObjectBase
{
public:
    virtual HRESULT AddItem(RatObjectID roid, LPVOID pData) = 0;
    virtual HRESULT InitializeMyDefaults(PicsCategory *pCategory) = 0;
};

class PICSRulesObjectBase
{
public:
    virtual HRESULT AddItem(PICSRulesObjectID proid, LPVOID pData) = 0;
    virtual HRESULT InitializeMyDefaults() = 0;
};

const DWORD AO_SINGLE = 0x01;
const DWORD AO_SEEN = 0x02;
const DWORD AO_MANDATORY = 0x04;

struct AllowableOption
{
    RatObjectID roid;
    DWORD fdwOptions;
};

struct PICSRulesAllowableOption
{
    PICSRulesObjectID roid;
    DWORD fdwOptions;
};

class PicsEnum : public PicsObjectBase
{
    private:
    public:
        ETS etstrName, etstrIcon, etstrDesc;
        ETN etnValue;

        PicsEnum();
        ~PicsEnum();
//        char* Parse(char *pStreamIn);

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);
};

class PicsRatingSystem;

class PicsCategory : public PicsObjectBase
{
    private:
    public:
        array<PicsCategory*> arrpPC;
        array<PicsEnum*>     arrpPE;
        ETS   etstrTransmitAs, etstrName, etstrIcon, etstrDesc;
        ETN   etnMin,   etnMax;
        ETB   etfMulti, etfInteger, etfLabelled, etfUnordered;
        PicsRatingSystem *pPRS;

        PicsCategory();
        ~PicsCategory();
//        char* Parse(char *pStreamIn, char *pBaseName, PicsCategory *pPCparent);
        void FixupLimits();
        void SetParents(PicsRatingSystem *pOwner);

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);
};


class PicsDefault : public PicsObjectBase
{
public:
    ETB etfInteger, etfLabelled, etfMulti, etfUnordered;
    ETN etnMax, etnMin;

    PicsDefault();
    ~PicsDefault();

    HRESULT AddItem(RatObjectID roid, LPVOID pData);
    HRESULT InitializeMyDefaults(PicsCategory *pCategory);
};


class PicsExtension : public PicsObjectBase
{
public:
    LPSTR m_pszRatingBureau;

    PicsExtension();
    ~PicsExtension();

    HRESULT AddItem(RatObjectID roid, LPVOID pData);
    HRESULT InitializeMyDefaults(PicsCategory *pCategory);
};


class PicsRatingSystem : public PicsObjectBase
{
    private:
    public:
        array<PicsCategory*> arrpPC;
        ETS                  etstrFile, etstrName, etstrIcon, etstrDesc, 
                             etstrRatingService, etstrRatingSystem, etstrRatingBureau;
        ETN                  etnPicsVersion;
        ETB                  etbBureauRequired;
        PicsDefault *        m_pDefaultOptions;
        DWORD                dwFlags;
        UINT                 nErrLine;

        PicsRatingSystem();
        ~PicsRatingSystem();
        HRESULT Parse(LPCSTR pszFile, LPSTR pStreamIn);

        HRESULT AddItem(RatObjectID roid, LPVOID pData);
        HRESULT InitializeMyDefaults(PicsCategory *pCategory);
        void ReportError(HRESULT hres);
};

/* bit values for PicsRatingSystem::dwFlags */
const DWORD PRS_ISVALID = 0x01;         /* this rating system was loaded successfully */
const DWORD PRS_WASINVALID = 0x02;      /* was invalid last time we tried to load it */

/* echo for PICSRules with different names for clarity */
#define PRRS_ISVALID        PRS_ISVALID;
#define PRRS_WASINVALID     PRS_WASINVALID;

class UserRating : public NLS_STR
{
public:
    INT m_nValue;
    UserRating *m_pNext;
    PicsCategory *m_pPC;

    UserRating();
    UserRating(UserRating *pCopyFrom);
    ~UserRating();

    UserRating *Duplicate(void);
    void SetName(LPCSTR pszName) { *(NLS_STR *)this = pszName; }
};


class UserRatingSystem : public NLS_STR
{
public:
    UserRating *m_pRatingList;
    UserRatingSystem *m_pNext;
    PicsRatingSystem *m_pPRS;

    UserRatingSystem();
    UserRatingSystem(UserRatingSystem *pCopyFrom);
    ~UserRatingSystem();

    UserRating *FindRating(LPCSTR pszTransmitName);
    UINT AddRating(UserRating *pRating);
    UINT ReadFromRegistry(HKEY hkeyProvider);
    UINT WriteToRegistry(HKEY hkey);

    UserRatingSystem *Duplicate(void);
    void SetName(LPCSTR pszName) { *(NLS_STR *)this = pszName; }
};


UserRatingSystem *DuplicateRatingSystemList(UserRatingSystem *pOld);
void DestroyRatingSystemList(UserRatingSystem *pList);
UserRatingSystem *FindRatingSystem(UserRatingSystem *pList, LPCSTR pszName);


class PicsUser{
private:
public:
    NLS_STR nlsUsername; 
    BOOL fAllowUnknowns, fPleaseMom, fEnabled;
#ifdef NASH        
    BOOL fMaster, fControlPanel, fNewApps;
    NLS_STR nlsLogFileName;
#endif
    UserRatingSystem *m_pRatingSystems;

    PicsUser();
    ~PicsUser();

    UserRatingSystem *FindRatingSystem(LPCSTR pszSystemName) { return ::FindRatingSystem(m_pRatingSystems, pszSystemName); }
    UINT AddRatingSystem(UserRatingSystem *pRatingSystem);
    BOOL NewInstall();
    UINT ReadFromRegistry(HKEY hkey, char *pszUserName);
    UINT WriteToRegistry(HKEY hkey);
};


PicsUser *GetUserObject(LPCSTR pszUsername=NULL);


extern long GetStateCounter();

class PicsRatingSystemInfo{
    private:
    public:
        array<PicsRatingSystem*> arrpPRS;
#ifdef NASH
        array<PicsUser*>         arrpPU;
#else
        PicsUser *pUserObject;
#endif
        BOOL                     fRatingInstalled;
        ETS                      etstrRatingBureau;
        long                     nInfoRev;
        BOOL                     fStoreInRegistry;
        BOOL                     fSettingsValid;
        PSTR                     lpszFileName;

        PicsRatingSystemInfo() { lpszFileName=NULL; nInfoRev = ::GetStateCounter(); };
        ~PicsRatingSystemInfo();

        BOOL Init();
        BOOL FreshInstall();
        void SaveRatingSystemInfo();
        BOOL LoadProviderFiles(HKEY hKey);
};


#endif
/*#define _PICS_H_*/

extern PicsRatingSystemInfo *gPRSI;

void CheckGlobalInfoRev(void);


char* EtStringParse(ETS &ets, char *pIn, const char *pKeyWord, BOOL fParen);
char* EtLabelParse(char *pIn, const char *pszLongName, const char *pszShortName);
char* EtRatingParse(ETN &etn, ETS &ets, char *pInStream);

extern "C" {
INT_PTR CALLBACK TestDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ChangePasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PasswordDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK IntroDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProviderDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

int  MyMessageBox(HWND hwnd, UINT uText, UINT uTitle, UINT uType);
int  MyMessageBox(HWND hwnd, UINT uText, UINT uText2, UINT uTitle, UINT uType);
int  MyMessageBox(HWND hwnd, LPCSTR pszText, UINT uTitle, UINT uType);
char *NonWhite(char *pIn);
BOOL MyAtoi(char *pIn, int *i);
LONG MyRegDeleteKey(HKEY hkey,LPCSTR pszSubkey);
HRESULT LoadRatingSystem(LPCSTR pszFilename, PicsRatingSystem **pprsOut);
INT_PTR DoProviderDialog(HWND hDlg, PicsRatingSystemInfo *pPRSI);

void DeleteUserSettings(PicsRatingSystem *pPRS);
void CheckUserSettings(PicsRatingSystem *pPRS);
