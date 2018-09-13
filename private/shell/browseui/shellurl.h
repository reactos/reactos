/**************************************************************\
    FILE: shellurl.h

    DESCRIPTION:
    Handle dealing with Shell Urls.  Include: Generating from PIDL, 
    Generating from preparsed Url String, and parsing from a user
    entered Url String.
\**************************************************************/

#ifndef _SHELLURL_H
#define _SHELLURL_H

#define STR_REGKEY_APPPATH   TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths")


// Parameters for ::SetUrl()'s dwGenType parameter
#define GENTYPE_FROMPATH    0x00000001
#define GENTYPE_FROMURL     0x00000002

class CShellUrl;

BOOL IsShellUrl(LPCTSTR pcszUrl, BOOL fIncludeFileUrls);
BOOL IsSpecialFolderChild(LPCITEMIDLIST pidlToTest, int nFolder, BOOL fImmediate);
HRESULT SetDefaultShellPath(CShellUrl * psu);


/**************************************************************\
    CLASS: CShellUrl

    DESCRIPTION:
        This object was created to keep track of a FULL SHELL URL.
    This includes any object in the shell name space and how
    to interact with that object.  Objects can be specified as
    PIDLs and then will be handled appropriately when executed,
    which means it will Navigate to the object if the object
    supports navigation or "browse in place" (registered DocHost, 
    Shell Folder, Internet URL)
    otherwise the Shell Url will be executed (files, shell items).

         If a string is inputed by the user, the ::ParseFromOutsideSource()
    method should be called.  This will take all of the environment
    information into consideration (CurrWorkDir & Path) and parse
    the URL.  If the string needs to be shell executed, it will
    generate the command line arguments and current working directory
    string.  If the object is to be navigated to, it will determine
    the navigation flags (for AutoSearch and similar things).

    PERF:
        This object was build so that if you specify a PIDL or String,
    it should be able to hold that information without a perf hit.
    If you want to take advantage of ::ParseFromOutsideSource() or
    ::Execute(), it will require a little more CPU time to do the
    full functionality.

    GOAL:
        The goal of the parsing is to make it into a normal internet
    URL unless the algorithm can with certainty assume that the string
    entered is specifying a Shell Name Space item or action.  This means
    that CShellUrl will only assume the entered text is a Shell Item to be
    Navigated to if it can successfully bind to the destination pidl.
    It will assume it's an item to be executed and use the end of the
    string as Command Line Arguments if: 1) It can bind to the end PIDL,
    and 2) The end pidl is not "Browsable" or "Navigatible", and 3)
    the char after the string specifying the PIDL is a Space.
\**************************************************************/
class CShellUrl 
{
public:
    // Constructor / Destructor
    CShellUrl();
    ~CShellUrl(void);  

    HRESULT ParseFromOutsideSource(LPCTSTR pcszUrlIn, DWORD dwParseFlags, PBOOL pfWasCorrected = NULL);
    HRESULT Execute(IBandProxy * pbp, BOOL * pfDidShellExec, DWORD dwExecFlags);

    HRESULT GetUrl(LPTSTR pszUrlOut, DWORD cchUrlOutSize);
    HRESULT SetUrl(LPCTSTR pcszUrlIn, DWORD dwGenType);       // Reset CShellUrl to this URL
    HRESULT GetDisplayName(LPTSTR pszUrlOut, DWORD cchUrlOutSize);
    HRESULT GetPidl(LPITEMIDLIST * ppidl);
    HRESULT SetPidl(LPCITEMIDLIST pidl);       // Reset CShellUrl to this PIDL
    HRESULT GetArgs(LPTSTR pszArgsOut, DWORD cchArgsOutSize);
    BOOL IsWebUrl(void);

    HRESULT SetCurrentWorkingDir(LPCITEMIDLIST pidlCWD);
    HRESULT AddPath(LPCITEMIDLIST pidl);
    HRESULT Reset(void);
    void SetMessageBoxParent(HWND hwnd);

#ifdef UNICODE
    HRESULT ParseFromOutsideSource(LPCSTR pcszUrlIn, DWORD dwParseFlags, PBOOL pfWasCorrected);
#endif // UNICODE

    HRESULT GetPidlNoGenerate(LPITEMIDLIST * ppidl);

private:
    // Parsing Methods
    HRESULT _ParseRelativePidl(LPCTSTR pcszUrlIn, BOOL * pfPossibleWebUrl, DWORD dwFlags, LPCITEMIDLIST pidl, BOOL fAllowRelative, BOOL fQualifyDispName);
    HRESULT _ParseUNC(LPCTSTR pcszUrlIn, BOOL * pfPossibleWebUrl, DWORD dwFlags, BOOL fQualifyDispName);
    HRESULT _ParseSeparator(LPCITEMIDLIST pidlParent, LPCTSTR pcszSeg, BOOL * pfPossibleWebUrl, BOOL fAllowRelative, BOOL fQualifyDispName);
    HRESULT _ParseNextSegment(LPCITEMIDLIST pidlParent, LPCTSTR pcszStrToParse, BOOL * pfPossibleWebUrl, BOOL fAllowRelative, BOOL fQualifyDispName);
    HRESULT _CheckItem(IShellFolder * psfFolder, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlRelative, 
                       LPITEMIDLIST * ppidlChild, LPCTSTR pcszStrToParse, LPTSTR * ppszRemaining, DWORD dwFlags);
    HRESULT _GetNextPossibleSegment(LPCTSTR pcszFullPath, LPTSTR * ppszSegIterator, LPTSTR pszSegOut, DWORD cchSegOutSize);
    HRESULT _GetNextPossibleFullPath(LPCTSTR pcszFullPath, LPTSTR * ppszSegIterator, LPTSTR pszSegOut, DWORD cchSegOutSize, BOOL * pfContinue);
    HRESULT _QualifyFromPath(LPCTSTR pcszFilePathIn, DWORD dwFlags);
    HRESULT _QualifyFromDOSPath(LPCTSTR pcszFilePathIn, DWORD dwFlags);
    HRESULT _QualifyFromAppPath(LPCTSTR pcszFilePathIn, DWORD dwFlags);
    HRESULT _QuickParse(LPCITEMIDLIST pidlParent, LPTSTR pszParseChunk, LPTSTR pszNext, BOOL * pfPossibleWebUrl, BOOL fAllowRelative, BOOL fQualifyDispName);
    BOOL _CanUseAdvParsing(void);
    BOOL _ParseURLFromOutsideSource(LPCWSTR psz, LPWSTR pszOut, LPDWORD pcchOut, LPBOOL pbWasSearchURL, LPBOOL pbWasCorrected);
    HRESULT _TryQuickParse(LPCTSTR pszUrl, DWORD dwParseFlags);

    // Accessor Methods
    HRESULT _SetPidl(LPCITEMIDLIST pidl);       // Set PIDL w/o modifying URL
    HRESULT _GeneratePidl(LPCTSTR pcszUrl, DWORD dwGenType);
    HRESULT _SetUrl(LPCTSTR pcszUrlIn, DWORD dwGenType);       // Set URL w/o modifying PIDL
    HRESULT _GenerateUrl(LPCITEMIDLIST pidl);
    HRESULT _GenDispNameFromPidl(LPCITEMIDLIST pidl, LPCTSTR pcszArgs);

    // Other Methods
    HRESULT _PidlShellExec(LPCITEMIDLIST pidl, ULONG ulShellExecFMask);
    HRESULT _UrlShellExec(void);
    BOOL _IsFilePidl(LPCITEMIDLIST pidl);

    HWND _GetWindow(void) { return (IsFlagSet(m_dwFlags, SHURL_FLAGS_NOUI) ? NULL : GetDesktopWindow()); }

    //////////////////////////////////////////////////////
    //  Private Member Variables 
    //////////////////////////////////////////////////////

    LPTSTR          m_pszURL;
    LPTSTR          m_pszDisplayName;       // The nice display name of the entity.
    LPTSTR          m_pszArgs;
    LPTSTR          m_pstrRoot;
    LPITEMIDLIST    m_pidl;
    DWORD           m_dwGenType;
    DWORD           m_dwFlags;

    LPITEMIDLIST    m_pidlWorkingDir;
    HDPA            m_hdpaPath;             // DPA of PIDLs
    HWND            m_hwnd;                 // parent window for message boxes
};


#endif /* _SHELLURL_H */
