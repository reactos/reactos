#ifndef __SLOWFIND_H_
#define __SLOWFIND_H_

#include "appsize.h"

#define MAX_PROGFILES_SEARCH_DEPTH 1
#define MAX_STARTMENU_SEARCH_DEPTH 2

HRESULT GetShortcutTarget(LPCWSTR pszPath, LPTSTR pszTarget, UINT cch);
BOOL SlowFindAppFolder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder);

class CStartMenuAppFinder : public CAppFolderSize
{
    friend BOOL SlowFindAppFolder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder);
public:
    CStartMenuAppFinder(LPCTSTR pszFullName, LPCTSTR pszShortName, LPTSTR pszFolder);

    // *** IShellTreeWalkerCallBack methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszFolder, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    
    HRESULT SearchInFolder(LPCTSTR pszStart);

protected:
    BOOL _MatchSMLinkWithApp(LPCTSTR pszLnkFile);

    LPCTSTR _pszFullName;
    LPCTSTR _pszShortName;

    // The Result
    LPTSTR  _pszFolder;

    // Best match found
    int _iBest;
}; 


#endif // _SLOWFIND_H_