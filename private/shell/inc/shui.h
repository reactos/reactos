#ifndef _SHUI_H_
#define _SHUI_H_

/* Declaration needed for shdocvw and browseui to work together and 
for whatever reason they cannot go into shdocvw.h or browseui.h
*/
#include <mshtml.h>

STDAPI SearchForElementInHead
(
    IHTMLDocument2* pHTMLDocument,  // [in] document to search
    LPOLESTR        pszAttribName,  // [in] attribute to check for
    LPOLESTR        pszAttrib,      // [in] value the attribute must have
    REFIID          iidDesired,     // [in] element interface to return
    IUnknown**      ppunkDesired    // [out] returned interface

);


typedef HRESULT (*PFNSEARCHFORELEMENTINHEAD)(
    IHTMLDocument2* pHTMLDocument,  // [in] document to search
    LPOLESTR        pszAttribName,  // [in] attribute to check for
    LPOLESTR        pszAttrib,      // [in] value the attribute must have
    REFIID          iidDesired,     // [in] element interface to return
    IUnknown**      ppunkDesired    // [out] returned interface
    );


#define SEARCHFORELEMENTINHEAD_ORD 208

typedef struct _internet_shortcut_params
{
    LPCITEMIDLIST pidlTarget;
    LPCTSTR  pszTitle;           // BUGBUG -- all of these can be made TCHAR 
                              // when shdocvw is made unicode - then the use of this struct in browseui
                              // will have to be changed to reflect that these are TCHAR
    LPCTSTR pszDir;
    LPTSTR  pszOut;
    int     cchOut;
    BOOL    bUpdateProperties;
    BOOL    bUniqueName;
    BOOL    bUpdateIcon;
    IOleCommandTarget *pCommand;
    IHTMLDocument2 *pDoc;
} ISHCUT_PARAMS;


STDAPI
CreateShortcutInDirEx(ISHCUT_PARAMS *pIShCutParams);


typedef HRESULT (*PFNDOWNLOADICONFORSHORTCUT)(WCHAR *pwszFileName, WCHAR *pwszShortcutUrl, WCHAR *pwszIconUrl);
#define DOWNLOADICONFORSHORTCUT_ORD 207   
STDAPI
DownloadIconForShortcut(
    WCHAR *pwszFileName,            // [in] Optional, File name of shortcut - full path
    WCHAR *pwszShortcutUrl,         // [in] Url of Shortcut
    IHTMLDocument2* pHTMLDocument  // [in] document to search for icon URL 
);

#endif
