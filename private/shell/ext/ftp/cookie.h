/*****************************************************************************
 *	cookies.h
 *****************************************************************************/

#ifndef _COOKIES_H
#define _COOKIES_H

class CCookieList;
CCookieList * CCookieList_Create(void);

/*****************************************************************************
 *
 *	CCookieList
 *
 *****************************************************************************/

class CCookieList
{
public:
    CCookieList();
    ~CCookieList(void);

    // Public Member Functions
    DWORD GetCookie(LPCTSTR pszString);
    HRESULT GetString(DWORD dwCookie, LPTSTR pszString, DWORD cchSize);

    friend CCookieList * CCookieList_Create(void) { return new CCookieList(); };

protected:
    // Private Member Variables
    HDPA                    m_hdpa;

    // Private Member Variables
    DWORD _Find(LPCTSTR pszString);
    static int _FreeStringEnum(LPVOID pString, LPVOID pData);
};

#endif // _COOKIES_H
