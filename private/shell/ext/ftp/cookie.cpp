/*****************************************************************************
 *
 *      cookies.cpp - Take care of the status bar.
 *
 *****************************************************************************/

#include "priv.h"
#include "cookie.h"

int CCookieList::_FreeStringEnum(LPVOID pString, LPVOID pData)
{
    LPTSTR pszString = (LPTSTR) pString;
    Str_SetPtr(&pszString, NULL);

    return 1;
}

DWORD CCookieList::_Find(LPCTSTR pszString)
{
    DWORD dwCookie = -1;        // -1 means not found.
    DWORD dwIndex;
    DWORD dwSize = DPA_GetPtrCount(m_hdpa);

    for (dwIndex = 0; dwIndex < dwSize; dwIndex++)
    {
        LPCTSTR pszCurrent = (LPCTSTR) DPA_FastGetPtr(m_hdpa, dwIndex);
        if (pszCurrent && !StrCmp(pszCurrent, pszString))
        {
            dwCookie = dwIndex;
            break;          // Found, it's already in the list so recycle.
        }
    }

    return dwCookie;
}

DWORD CCookieList::GetCookie(LPCTSTR pszString)
{
    ENTERCRITICAL;
    DWORD dwCookie = -1;

    if (!EVAL(pszString))
        return -1;

    if (!m_hdpa)
        m_hdpa = DPA_Create(10);
    
    if (EVAL(m_hdpa))
    {
        dwCookie = _Find(pszString);
        // Did we not find it in the list?
        if (-1 == dwCookie)
        {
            LPTSTR pszCopy = NULL;

            dwCookie = DPA_GetPtrCount(m_hdpa);
            Str_SetPtr(&pszCopy, pszString);
            DPA_AppendPtr(m_hdpa, pszCopy);
        }
    }
    LEAVECRITICAL;

    return dwCookie;
}

HRESULT CCookieList::GetString(DWORD dwCookie, LPTSTR pszString, DWORD cchSize)
{
    ENTERCRITICAL;
    HRESULT hr = S_FALSE;

    if (m_hdpa &&
       (dwCookie < (DWORD)DPA_GetPtrCount(m_hdpa)))
    {
        LPCTSTR pszCurrent = (LPCTSTR) DPA_FastGetPtr(m_hdpa, dwCookie);

        StrCpyN(pszString, pszCurrent, cchSize);
        hr = S_OK;
    }

    LEAVECRITICAL;
    return hr;
}



/****************************************************\
    Constructor
\****************************************************/
CCookieList::CCookieList()
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hdpa);

    LEAK_ADDREF(LEAK_CCookieList);
}


/****************************************************\
    Destructor
\****************************************************/
CCookieList::~CCookieList(void)
{
    ENTERCRITICAL;
    if (m_hdpa)
        DPA_DestroyCallback(m_hdpa, _FreeStringEnum, NULL);
    LEAVECRITICAL;

    ASSERTNONCRITICAL;

    DllRelease();
    LEAK_DELREF(LEAK_CCookieList);
}


