#ifndef _COOKIES_HXX_
#define _COOKIES_HXX_

/*-----------------------------------------------------------------------------
Copyright (c) 1996  Microsoft Corporation

Module Name:  cookies.hxx

Abstract:

  Cookie upgrade object header
  
  Upgrades cookies to new format by parsing existing cookie
  files and adding them to the newly created cookie cache index. 

  Currently upgrades v3.2 to v4.0.

    
Author:
    Adriaan Canter (adriaanc) 01-Nov-1996.
    
-------------------------------------------------------------------------------*/




/*-----------------------------------------------------------------------------
    class CCookieLoader
  ----------------------------------------------------------------------------*/
class CCookieLoader
{
private:
    DWORD GetHKLMCookiesDirectory(CHAR*);
    CHAR* ParseNextCookie(CHAR*, CHAR**, FILETIME*, FILETIME*);
    
public:
    DWORD LoadCookies(URL_CONTAINER *);
};

#endif // _COOKIES_HXX
