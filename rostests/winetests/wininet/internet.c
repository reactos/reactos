/*
 * Wininet - internet tests
 *
 * Copyright 2005 Vijay Kiran Kamuju
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wininet.h"
#include "winerror.h"

#include "wine/test.h"

static BOOL (WINAPI *pInternetTimeFromSystemTimeA)(CONST SYSTEMTIME *,DWORD ,LPSTR ,DWORD);
static BOOL (WINAPI *pInternetTimeFromSystemTimeW)(CONST SYSTEMTIME *,DWORD ,LPWSTR ,DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeA)(LPCSTR ,SYSTEMTIME *,DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeW)(LPCWSTR ,SYSTEMTIME *,DWORD);
static BOOL (WINAPI *pIsDomainLegalCookieDomainW)(LPCWSTR, LPCWSTR);

/* ############################### */

static void test_InternetCanonicalizeUrlA(void)
{
    CHAR    buffer[256];
    LPCSTR  url;
    DWORD   urllen;
    DWORD   dwSize;
    DWORD   res;

    /* Acrobat Updater 5 calls this for Adobe Reader 8.1 */
    url = "http://swupmf.adobe.com/manifest/50/win/AdobeUpdater.upd";
    urllen = lstrlenA(url);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = 1; /* Acrobat Updater use this size */
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (dwSize == (urllen+1)),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));


    /* buffer has no space for the terminating '\0' */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of needed bytes with the terminating '\0' */
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (dwSize == (urllen+1)),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));

    /* buffer has the required size */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of copied bytes without the terminating '\0' */
    ok( res && (dwSize == urllen) && (lstrcmpA(url, buffer) == 0),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = sizeof(buffer);
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA("file:///C:/Program%20Files/Atmel/AVR%20Tools/STK500/STK500.xml", buffer, &dwSize, ICU_DECODE | ICU_NO_ENCODE);
    ok(res, "InternetCanonicalizeUrlA failed %u\n", GetLastError());
    ok(dwSize == lstrlenA(buffer), "got %d expected %d\n", dwSize, lstrlenA(buffer));
    todo_wine ok(!lstrcmpA("file://C:\\Program Files\\Atmel\\AVR Tools\\STK500\\STK500.xml", buffer),
       "got %s expected 'file://C:\\Program Files\\Atmel\\AVR Tools\\STK500\\STK500.xml'\n", buffer);

    /* buffer is larger as the required size */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen+2;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of copied bytes without the terminating '\0' */
    ok( res && (dwSize == urllen) && (lstrcmpA(url, buffer) == 0),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));


    /* check NULL pointers */
    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(NULL, buffer, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));

    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, NULL, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));

    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, NULL, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %u and %u with size %u for '%s' (%d)\n",
        res, GetLastError(), dwSize, buffer, lstrlenA(buffer));

    /* test with trailing space */
    dwSize = 256;
    res = InternetCanonicalizeUrlA("http://www.winehq.org/index.php?x= ", buffer, &dwSize, ICU_BROWSER_MODE);
    ok(res == 1, "InternetCanonicalizeUrlA failed\n");
    ok(!strcmp(buffer, "http://www.winehq.org/index.php?x="), "Trailing space should have been stripped even in ICU_BROWSER_MODE (%s)\n", buffer);

    res = InternetSetOptionA(NULL, 0xdeadbeef, buffer, sizeof(buffer));
    ok(!res, "InternetSetOptionA succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_INVALID_OPTION,
       "InternetSetOptionA failed %u, expected ERROR_INTERNET_INVALID_OPTION\n", GetLastError());
}

/* ############################### */

static void test_InternetQueryOptionA(void)
{
  HINTERNET hinet,hurl;
  DWORD len;
  DWORD err;
  static const char useragent[] = {"Wininet Test"};
  char *buffer;
  int retval;

  hinet = InternetOpenA(useragent,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  retval=InternetQueryOptionA(NULL,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code%d\n",err);

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == strlen(useragent)+1,"Got wrong user agent length %d instead of %d\n",len,lstrlenA(useragent));
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code %d\n",err);

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  buffer=HeapAlloc(GetProcessHeap(),0,len);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  ok(retval == 1,"Got wrong return value %d\n",retval);
  if (retval)
  {
      ok(!strcmp(useragent,buffer),"Got wrong user agent string %s instead of %s\n",buffer,useragent);
      todo_wine ok(len == strlen(useragent),"Got wrong user agent length %d instead of %d\n",len,lstrlenA(useragent));
  }
  ok(err == 0xdeadbeef, "Got wrong error code %d\n",err);
  HeapFree(GetProcessHeap(),0,buffer);

  SetLastError(0xdeadbeef);
  len=0;
  buffer=HeapAlloc(GetProcessHeap(),0,100);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  ok(len == strlen(useragent) + 1,"Got wrong user agent length %d instead of %d\n", len, lstrlenA(useragent) + 1);
  ok(!retval, "Got wrong return value %d\n", retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code %d\n", err);
  HeapFree(GetProcessHeap(),0,buffer);

  hurl = InternetConnectA(hinet,"www.winehq.com",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

  SetLastError(0xdeadbeef);
  len=0;
  retval = InternetQueryOptionA(hurl,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 0,"Got wrong user agent length %d instead of 0\n",len);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %d\n",err);

  InternetCloseHandle(hurl);
  InternetCloseHandle(hinet);

  hinet = InternetOpenA("",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 1,"Got wrong user agent length %d instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%d\n",err);

  InternetCloseHandle(hinet);

  hinet = InternetOpenA(NULL,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");
  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  todo_wine ok(len == 1,"Got wrong user agent length %d instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%d\n",err);

  InternetCloseHandle(hinet);
}

static void test_get_cookie(void)
{
  DWORD len;
  BOOL ret;

  SetLastError(0xdeadbeef);
  ret = InternetGetCookie("http://www.example.com", NULL, NULL, &len);
  ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS,
    "InternetGetCookie should have failed with %s and error %d\n",
    ret ? "TRUE" : "FALSE", GetLastError());
}


static void test_complicated_cookie(void)
{
  DWORD len;
  BOOL ret;

  CHAR buffer[1024];

  ret = InternetSetCookie("http://www.example.com/bar",NULL,"A=B; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookie("http://www.example.com/bar",NULL,"C=D; domain=.example.com; path=/");
  ok(ret == TRUE,"InternetSetCookie failed\n");

  /* Technically illegal! domain should require 2 dots, but native wininet accepts it */
  ret = InternetSetCookie("http://www.example.com",NULL,"E=F; domain=example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookie("http://www.example.com",NULL,"G=H; domain=.example.com; path=/foo");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookie("http://www.example.com/bar.html",NULL,"I=J; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookie("http://www.example.com/bar/",NULL,"K=L; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookie("http://www.example.com/bar/",NULL,"M=N; domain=.example.com; path=/foo/");
  ok(ret == TRUE,"InternetSetCookie failed\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/foobar", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/foobar/", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")!=NULL,"G=H missing\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/foo/bar", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")!=NULL,"G=H missing\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")!=NULL,"M=N missing\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/barfoo", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/barfoo/", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");

  len = 1024;
  ret = InternetGetCookie("http://testing.example.com/bar/foo", NULL, buffer, &len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")!=NULL,"K=L missing\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
}

static void test_null(void)
{
  HINTERNET hi, hc;
  static const WCHAR szServer[] = { 's','e','r','v','e','r',0 };
  static const WCHAR szEmpty[] = { 0 };
  static const WCHAR szUrl[] = { 'h','t','t','p',':','/','/','a','.','b','.','c',0 };
  static const WCHAR szUrlEmpty[] = { 'h','t','t','p',':','/','/',0 };
  static const WCHAR szExpect[] = { 's','e','r','v','e','r',';',' ','s','e','r','v','e','r',0 };
  WCHAR buffer[0x20];
  BOOL r;
  DWORD sz;

  hi = InternetOpenW(NULL, 0, NULL, NULL, 0);
  ok(hi != NULL, "open failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(hi, NULL, 0, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetConnectW(NULL, szServer, 0, NULL, NULL, INTERNET_SERVICE_FTP, 0, 0);
  ok(GetLastError() == ERROR_INVALID_HANDLE, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetOpenUrlW(hi, NULL, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INVALID_PARAMETER ||
     GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  hc = InternetOpenUrlW(hi, szServer, NULL, 0, 0, 0);
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  ok(hc == NULL, "connect failed\n");

  InternetCloseHandle(hi);

  r = InternetSetCookieW(NULL, NULL, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szServer, NULL, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szUrl, szServer, NULL);
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szUrl, szServer, szServer);
  ok(r == TRUE, "return wrong\n");

  r = InternetSetCookieW(szUrl, NULL, szServer);
  ok(r == TRUE, "return wrong\n");

  r = InternetSetCookieW(szUrl, szServer, szEmpty);
  ok(r == TRUE, "return wrong\n");

  r = InternetSetCookieW(szUrlEmpty, szServer, szServer);
  ok(r == FALSE, "return wrong\n");

  r = InternetSetCookieW(szServer, NULL, szServer);
  todo_wine {
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  }
  ok(r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(NULL, NULL, NULL, &sz);
  ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME,
     "wrong error %u\n", GetLastError());
  ok( r == FALSE, "return wrong\n");

  r = InternetGetCookieW(szServer, NULL, NULL, &sz);
  todo_wine {
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  }
  ok( r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(szUrlEmpty, szServer, NULL, &sz);
  ok( r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(szUrl, szServer, NULL, &sz);
  ok( r == TRUE, "return wrong\n");

  /* sz is 14 on XP SP2 and beyond, 30 on XP SP1 and before */
  ok( sz == 14 || sz == 30, "sz wrong, got %u, expected 14 or 30\n", sz);

  sz = 0x20;
  memset(buffer, 0, sizeof buffer);
  r = InternetGetCookieW(szUrl, szServer, buffer, &sz);
  ok( r == TRUE, "return wrong\n");

  /* sz == lstrlenW(buffer) only in XP SP1 */
  ok( sz == 1 + lstrlenW(buffer) || sz == lstrlenW(buffer), "sz wrong %d\n", sz);

  /* before XP SP2, buffer is "server; server" */
  ok( !lstrcmpW(szExpect, buffer) || !lstrcmpW(szServer, buffer), "cookie data wrong\n");

  sz = sizeof(buffer);
  r = InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECTED_STATE, buffer, &sz);
  ok(r == TRUE, "ret %d\n", r);
}

static void test_version(void)
{
    INTERNET_VERSION_INFO version;
    DWORD size;
    BOOL res;

    size = sizeof(version);
    res = InternetQueryOptionA(NULL, INTERNET_OPTION_VERSION, &version, &size);
    ok(res, "Could not get version: %u\n", GetLastError());
    ok(version.dwMajorVersion == 1, "dwMajorVersion=%d, expected 1\n", version.dwMajorVersion);
    ok(version.dwMinorVersion == 2, "dwMinorVersion=%d, expected 2\n", version.dwMinorVersion);
}

static void InternetTimeFromSystemTimeA_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    char string[INTERNET_RFC1123_BUFSIZE];
    static const char expect[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    DWORD error;

    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeA failed (%u)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeA failed (%u)\n", GetLastError() );

    /* test NULL time parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( NULL, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test NULL string parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, NULL, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test invalid format parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT + 1, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test too small buffer size */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, 0 );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER,
        "InternetTimeFromSystemTimeA failed with ERROR_INSUFFICIENT_BUFFER instead of %u\n",
        error );
}

static void InternetTimeFromSystemTimeW_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    WCHAR string[INTERNET_RFC1123_BUFSIZE + 1];
    static const WCHAR expect[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };
    DWORD error;

    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeW failed (%u)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeW failed (%u)\n", GetLastError() );

    /* test NULL time parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( NULL, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test NULL string parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, NULL, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test invalid format parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT + 1, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %u\n",
        error );

    /* test too small buffer size */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string)/sizeof(string[0]) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER,
        "InternetTimeFromSystemTimeW failed with ERROR_INSUFFICIENT_BUFFER instead of %u\n",
        error );
}

static void InternetTimeToSystemTimeA_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const char string[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    static const char string2[] = " fri 7 jan 2005 12 06 35";

    ret = pInternetTimeToSystemTimeA( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeA( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%u)\n", GetLastError() );
}

static void InternetTimeToSystemTimeW_test(void)
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const WCHAR string[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };
    static const WCHAR string2[] = { ' ','f','r','i',' ','7',' ','j','a','n',' ','2','0','0','5',' ',
                                     '1','2',' ','0','6',' ','3','5',0 };
    static const WCHAR string3[] = { 'F','r',0 };

    ret = pInternetTimeToSystemTimeW( NULL, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( NULL, &time, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );

    ret = pInternetTimeToSystemTimeW( string3, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%u)\n", GetLastError() );
}

static void test_IsDomainLegalCookieDomainW(void)
{
    BOOL ret;
    DWORD error;
    static const WCHAR empty[]          = {0};
    static const WCHAR dot[]            = {'.',0};
    static const WCHAR uk[]             = {'u','k',0};
    static const WCHAR com[]            = {'c','o','m',0};
    static const WCHAR dot_com[]        = {'.','c','o','m',0};
    static const WCHAR gmail_com[]      = {'g','m','a','i','l','.','c','o','m',0};
    static const WCHAR dot_gmail_com[]  = {'.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR mail_gmail_com[] = {'m','a','i','l','.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR gmail_co_uk[]    = {'g','m','a','i','l','.','c','o','.','u','k',0};
    static const WCHAR co_uk[]          = {'c','o','.','u','k',0};
    static const WCHAR dot_co_uk[]      = {'.','c','o','.','u','k',0};

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(NULL, NULL);
    error = GetLastError();
    ok(!ret ||
        broken(ret), /* Win98, NT4, W2K, XP (some) */
        "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, NULL);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(NULL, gmail_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(empty, gmail_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == ERROR_INVALID_PARAMETER), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, empty);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == ERROR_INVALID_PARAMETER), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(gmail_com, dot);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == 0xdeadbeef), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot, gmail_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == 0xdeadbeef), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == 0xdeadbeef, "got %u expected 0xdeadbeef\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, dot_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == 0xdeadbeef), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot_com, com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == 0xdeadbeef), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, gmail_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == 0xdeadbeef, "got %u expected 0xdeadbeef\n", error);

    ret = pIsDomainLegalCookieDomainW(gmail_com, gmail_com);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, co_uk);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == 0xdeadbeef, "got %u expected 0xdeadbeef\n", error);

    ret = pIsDomainLegalCookieDomainW(uk, co_uk);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, dot_co_uk);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, gmail_co_uk);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    ret = pIsDomainLegalCookieDomainW(gmail_com, com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot_gmail_com, mail_gmail_com);
    error = GetLastError();
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
    ok(error == ERROR_INVALID_NAME ||
        broken(error == 0xdeadbeef), /* Win98, NT4, W2K, XP (some) */
        "got %u expected ERROR_INVALID_NAME\n", error);

    ret = pIsDomainLegalCookieDomainW(gmail_com, mail_gmail_com);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    ret = pIsDomainLegalCookieDomainW(mail_gmail_com, gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(mail_gmail_com, com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(dot_gmail_com, mail_gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(mail_gmail_com, dot_gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");
}

/* ############################### */

START_TEST(internet)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");
    pInternetTimeFromSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeA");
    pInternetTimeFromSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeW");
    pInternetTimeToSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeA");
    pInternetTimeToSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeW");
    pIsDomainLegalCookieDomainW = (void*)GetProcAddress(hdll, (LPCSTR)117);

    test_InternetCanonicalizeUrlA();
    test_InternetQueryOptionA();
    test_get_cookie();
    test_complicated_cookie();
    test_version();
    test_null();

    if (!pInternetTimeFromSystemTimeA)
        skip("skipping the InternetTime tests\n");
    else
    {
        InternetTimeFromSystemTimeA_test();
        InternetTimeFromSystemTimeW_test();
        InternetTimeToSystemTimeA_test();
        InternetTimeToSystemTimeW_test();
    }
    if (!pIsDomainLegalCookieDomainW)
        skip("skipping IsDomainLegalCookieDomainW tests\n");
    else
        test_IsDomainLegalCookieDomainW();
}
