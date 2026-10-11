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
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wininet.h"
#include "winineti.h"
#include "winerror.h"
#include "winreg.h"
#include "winnls.h"
#include "wchar.h"

#include "wine/test.h"

static BOOL (WINAPI *pCreateUrlCacheContainerA)(DWORD, DWORD, DWORD, DWORD,
                                                DWORD, DWORD, DWORD, DWORD);
static BOOL (WINAPI *pCreateUrlCacheContainerW)(DWORD, DWORD, DWORD, DWORD,
                                                DWORD, DWORD, DWORD, DWORD);
static BOOL (WINAPI *pInternetTimeFromSystemTimeA)(const SYSTEMTIME *, DWORD, LPSTR, DWORD);
static BOOL (WINAPI *pInternetTimeFromSystemTimeW)(const SYSTEMTIME *, DWORD, LPWSTR, DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeA)(LPCSTR ,SYSTEMTIME *,DWORD);
static BOOL (WINAPI *pInternetTimeToSystemTimeW)(LPCWSTR ,SYSTEMTIME *,DWORD);
static BOOL (WINAPI *pIsDomainLegalCookieDomainW)(LPCWSTR, LPCWSTR);
static DWORD (WINAPI *pPrivacyGetZonePreferenceW)(DWORD, DWORD, LPDWORD, LPWSTR, LPDWORD);
static DWORD (WINAPI *pPrivacySetZonePreferenceW)(DWORD, DWORD, DWORD, LPCWSTR);
static BOOL (WINAPI *pInternetGetCookieExA)(LPCSTR,LPCSTR,LPSTR,LPDWORD,DWORD,LPVOID);
static BOOL (WINAPI *pInternetGetCookieExW)(LPCWSTR,LPCWSTR,LPWSTR,LPDWORD,DWORD,LPVOID);
static BOOL (WINAPI *pInternetGetConnectedStateExA)(LPDWORD,LPSTR,DWORD,DWORD);
static BOOL (WINAPI *pInternetGetConnectedStateExW)(LPDWORD,LPWSTR,DWORD,DWORD);

#ifndef ERROR_WINHTTP_AUTODETECTION_FAILED
#define ERROR_WINHTTP_AUTODETECTION_FAILED 12180
#endif


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
    urllen = strlen(url);

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = 1; /* Acrobat Updater use this size */
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (dwSize == (urllen+1)),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));


    /* buffer has no space for the terminating '\0' */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of needed bytes with the terminating '\0' */
    ok( !res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) && (dwSize == (urllen+1)),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));

    /* buffer has the required size */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of copied bytes without the terminating '\0' */
    ok( res && (dwSize == urllen) && (lstrcmpA(url, buffer) == 0),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));

    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = sizeof(buffer);
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA("file:///C:/Program%20Files/Atmel/AVR%20Tools/STK500/STK500.xml", buffer, &dwSize, ICU_DECODE | ICU_NO_ENCODE);
    ok(res, "InternetCanonicalizeUrlA failed %lu\n", GetLastError());
    ok(dwSize == strlen(buffer), "got %ld expected %Iu\n", dwSize, strlen(buffer));
    ok(!lstrcmpA("file://C:\\Program Files\\Atmel\\AVR Tools\\STK500\\STK500.xml", buffer),
       "got %s expected 'file://C:\\Program Files\\Atmel\\AVR Tools\\STK500\\STK500.xml'\n", buffer);

    /* buffer is larger as the required size */
    memset(buffer, '#', sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    dwSize = urllen+2;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, &dwSize, 0);
    /* dwSize is nr. of copied bytes without the terminating '\0' */
    ok( res && (dwSize == urllen) && (lstrcmpA(url, buffer) == 0),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));


    /* check NULL pointers */
    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(NULL, buffer, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));

    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, NULL, &dwSize, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));

    memset(buffer, '#', urllen + 4);
    buffer[urllen + 4] = '\0';
    dwSize = urllen+1;
    SetLastError(0xdeadbeef);
    res = InternetCanonicalizeUrlA(url, buffer, NULL, 0);
    ok( !res && (GetLastError() == ERROR_INVALID_PARAMETER),
        "got %lu and %lu with size %lu for '%s' (%Iu)\n",
        res, GetLastError(), dwSize, buffer, strlen(buffer));

    /* test with trailing space */
    dwSize = 256;
    res = InternetCanonicalizeUrlA("http://www.winehq.org/index.php?x= ", buffer, &dwSize, ICU_BROWSER_MODE);
    ok(res == 1, "InternetCanonicalizeUrlA failed\n");
    ok(!strcmp(buffer, "http://www.winehq.org/index.php?x="), "Trailing space should have been stripped even in ICU_BROWSER_MODE (%s)\n", buffer);

    res = InternetSetOptionA(NULL, 0xdeadbeef, buffer, sizeof(buffer));
    ok(!res, "InternetSetOptionA succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_INVALID_OPTION,
       "InternetSetOptionA failed %lu, expected ERROR_INTERNET_INVALID_OPTION\n", GetLastError());
}

/* ############################### */

static void test_InternetQueryOptionA(void)
{
  HINTERNET hinet,hurl;
  DWORD len, val;
  INTERNET_PROXY_INFOA *pi;
  DWORD err;
  static const char useragent[] = {"Wininet Test"};
  char proxy[256];
  char *buffer;
  BOOL retval, res;

  SetLastError(0xdeadfeed);
  memset(proxy, 0, sizeof(proxy));
  res = DetectAutoProxyUrl(proxy, sizeof(proxy), PROXY_AUTO_DETECT_TYPE_DHCP);
  todo_wine ok((res && proxy[0]) ||
     (!res && GetLastError() == ERROR_WINHTTP_AUTODETECTION_FAILED && !proxy[0]),
     "unexpected DHCP proxy result: %d gle %lu proxy %s\n", res, GetLastError(), proxy);

  SetLastError(0xdeadfeed);
  memset(proxy, 0, sizeof(proxy));
  res = DetectAutoProxyUrl(proxy, sizeof(proxy), PROXY_AUTO_DETECT_TYPE_DNS_A);
  todo_wine ok((res && proxy[0]) ||
     (!res && GetLastError() == ERROR_WINHTTP_AUTODETECTION_FAILED && !proxy[0]) ||
     broken(!res && GetLastError() == ERROR_NOT_FOUND && !proxy[0]),
     "unexpected DNS proxy result: %d gle %lu proxy %s\n", res, GetLastError(), proxy);

  SetLastError(0xdeadbeef);
  len = 0xdeadbeef;
  retval = InternetQueryOptionA(NULL, INTERNET_OPTION_PROXY, NULL, &len);
  ok(!retval && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got wrong error %x(%lu)\n", retval, GetLastError());
  ok(len >= sizeof(INTERNET_PROXY_INFOA) && len != 0xdeadbeef,"len = %lu\n", len);

  pi = HeapAlloc(GetProcessHeap(), 0, len);
  retval = InternetQueryOptionA(NULL, INTERNET_OPTION_PROXY, pi, &len);
  ok(retval, "Failed (%lu)\n", GetLastError());
  ok(len >= sizeof(INTERNET_PROXY_INFOA) && len != 0xdeadbeef, "len = %lu\n", len);
  HeapFree(GetProcessHeap(), 0, pi);

  hinet = InternetOpenA(useragent,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  retval=InternetQueryOptionA(NULL,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code%ld\n",err);

  SetLastError(0xdeadbeef);
  len=strlen(useragent)+1;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == strlen(useragent)+1,"Got wrong user agent length %ld instead of %Iu\n",len,strlen(useragent));
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code %ld\n",err);

  len=strlen(useragent)+1;
  buffer=HeapAlloc(GetProcessHeap(),0,len);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  ok(retval == 1,"Got wrong return value %d\n",retval);
  if (retval)
  {
      ok(!strcmp(useragent,buffer),"Got wrong user agent string %s instead of %s\n",buffer,useragent);
      ok(len == strlen(useragent),"Got wrong user agent length %ld instead of %Iu\n",len,strlen(useragent));
  }
  HeapFree(GetProcessHeap(),0,buffer);

  SetLastError(0xdeadbeef);
  len=0;
  buffer=HeapAlloc(GetProcessHeap(),0,100);
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,buffer,&len);
  err=GetLastError();
  ok(len == strlen(useragent) + 1,"Got wrong user agent length %ld instead of %Iu\n", len, strlen(useragent) + 1);
  ok(!retval, "Got wrong return value %d\n", retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code %ld\n", err);
  HeapFree(GetProcessHeap(),0,buffer);

  hurl = InternetConnectA(hinet,"www.winehq.org",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

  SetLastError(0xdeadbeef);
  len=0;
  retval = InternetQueryOptionA(hurl,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 0,"Got wrong user agent length %ld instead of 0\n",len);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %ld\n",err);

  SetLastError(0xdeadbeef);
  len = sizeof(DWORD);
  retval = InternetQueryOptionA(hurl,INTERNET_OPTION_REQUEST_FLAGS,NULL,&len);
  err = GetLastError();
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %ld\n",err);
  ok(len == sizeof(DWORD), "len = %ld\n", len);

  SetLastError(0xdeadbeef);
  len = sizeof(DWORD);
  retval = InternetQueryOptionA(NULL,INTERNET_OPTION_REQUEST_FLAGS,NULL,&len);
  err = GetLastError();
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "Got wrong error code %ld\n",err);
  ok(!len, "len = %ld\n", len);

  InternetCloseHandle(hurl);
  InternetCloseHandle(hinet);

  hinet = InternetOpenA("",INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");

  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 1,"Got wrong user agent length %ld instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%ld\n",err);

  InternetCloseHandle(hinet);

  /* Connect timeout */
  val = 12345;
  res = InternetSetOptionA(NULL, INTERNET_OPTION_CONNECT_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(NULL, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 12345, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Receive Timeout */
  val = 54321;
  res = InternetSetOptionA(NULL, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(NULL, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 54321, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Send Timeout */
  val = 12345;
  res = InternetSetOptionA(NULL, INTERNET_OPTION_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(NULL, INTERNET_OPTION_SEND_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 12345, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Data Receive Timeout */
  val = 54321;
  res = InternetSetOptionA(NULL, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(NULL, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 54321, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Data Send Timeout */
  val = 12345;
  res = InternetSetOptionA(NULL, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  len = sizeof(val);
  res = InternetQueryOptionA(NULL, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 12345, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  hinet = InternetOpenA(NULL,INTERNET_OPEN_TYPE_DIRECT,NULL,NULL, 0);
  ok((hinet != 0x0),"InternetOpen Failed\n");
  SetLastError(0xdeadbeef);
  len=0;
  retval=InternetQueryOptionA(hinet,INTERNET_OPTION_USER_AGENT,NULL,&len);
  err=GetLastError();
  ok(len == 1,"Got wrong user agent length %ld instead of %d\n",len,1);
  ok(retval == 0,"Got wrong return value %d\n",retval);
  ok(err == ERROR_INSUFFICIENT_BUFFER, "Got wrong error code%ld\n",err);

  len = sizeof(val);
  val = 0xdeadbeef;
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, &len);
  ok(!res, "InternetQueryOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER) succeeded\n");
  ok(GetLastError() == ERROR_INTERNET_INVALID_OPERATION, "GetLastError() = %lu\n", GetLastError());

  /* Connect Timeout */
  val = 2;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, sizeof(val));
  ok(!res, "InternetSetOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER) succeeded\n");
  ok(GetLastError() == ERROR_INTERNET_INVALID_OPERATION, "GetLastError() = %lu\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 12345, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  val = 1;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 1, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Receive Timeout */
  val = 60;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 60, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Send Timeout */
  val = 120;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_SEND_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 120, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Data Receive Timeout */
  val = 60;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 60, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Data Send Timeout */
  val = 120;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  len = sizeof(val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, &len);
  ok(res, "InternetQueryOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed %ld)\n", GetLastError());
  ok(val == 120, "val = %ld\n", val);
  ok(len == sizeof(val), "len = %ld\n", len);

  /* Timeout inheritance */
  val = 15000;
  len = sizeof(val);
  res = InternetSetOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed (%lu)\n", GetLastError());
  res = InternetSetOptionA(hinet, INTERNET_OPTION_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed (%lu)\n", GetLastError());
  res = InternetSetOptionA(hinet, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed (%lu)\n", GetLastError());
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  hurl = InternetConnectA(hinet,"www.winehq.org",INTERNET_DEFAULT_HTTP_PORT,NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(val == 15000, "failed to inherit INTERNET_OPTION_CONNECT_TIMEOUT on child connection (found %ld) - Error: %ld)\n", val, GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_SEND_TIMEOUT, &val, &len);
  ok(val == 15000, "failed to inherit INTERNET_OPTION_SEND_TIMEOUT on child connection (found %ld) - Error: %ld)\n", val, GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 15000, "failed to inherit INTERNET_OPTION_RECEIVE_TIMEOUT on child connection (found %ld) - Error: %ld)\n", val, GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, &len);
  ok(val == 15000, "failed to inherit INTERNET_OPTION_DATA_SEND_TIMEOUTt on child connection (found %ld) - Error: %ld)\n", val, GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 15000, "failed to inherit INTERNET_OPTION_DATA_RECEIVE_TIMEOUT on child connection (found %ld) - Error: %ld)\n", val, GetLastError());

  val = 12345;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_CONNECT_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(val == 15000, "Connection handle inherited value (INTERNET_OPTION_CONNECT_TIMEOUT) as %ld\n", val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_CONNECT_TIMEOUT, &val, &len);
  ok(val == 12345, "Parent handle set from inherited value (INTERNET_OPTION_CONNECT_TIMEOUT) as %ld\n", val);

  val = 12345;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_SEND_TIMEOUT, &val, &len);
  ok(val == 15000, "Connection handle inherited value (INTERNET_OPTION_SEND_TIMEOUT) as %ld\n", val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_SEND_TIMEOUT, &val, &len);
  ok(val == 12345, "Parent handle set from inherited value (INTERNET_OPTION_SEND_TIMEOUT) as %ld\n", val);

  val = 12345;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 15000, "Connection handle inherited value (INTERNET_OPTION_RECEIVE_TIMEOUT) as %ld\n", val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 12345, "Parent handle set from inherited value (INTERNET_OPTION_RECEIVE_TIMEOUT) as %ld\n", val);

  val = 12345;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_SEND_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, &len);
  ok(val == 15000, "Connection handle inherited value (INTERNET_OPTION_DATA_SEND_TIMEOUT) as %ld\n", val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_DATA_SEND_TIMEOUT, &val, &len);
  ok(val == 12345, "Parent handle set from inherited value (INTERNET_OPTION_DATA_SEND_TIMEOUT) as %ld\n", val);

  val = 12345;
  res = InternetSetOptionA(hinet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, sizeof(val));
  ok(res, "InternetSetOptionA(INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) failed (%lu)\n", GetLastError());

  val = 0xdeadbeef;
  res = InternetQueryOptionA(hurl, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 15000, "Connection handle inherited value (INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) as %ld\n", val);
  res = InternetQueryOptionA(hinet, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &val, &len);
  ok(val == 12345, "Parent handle set from inherited value (INTERNET_OPTION_DATA_RECEIVE_TIMEOUT) as %ld\n", val);

  InternetCloseHandle(hurl);
  InternetCloseHandle(hinet);
}

static void test_max_conns(void)
{
    DWORD len, val;
    BOOL res;

    len = sizeof(val);
    val = 0xdeadbeef;
    res = InternetQueryOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, &len);
    ok(res,"Got wrong return value %x\n", res);
    ok(len == sizeof(val), "got %ld\n", len);
    trace("INTERNET_OPTION_MAX_CONNS_PER_SERVER: %ld\n", val);

    len = sizeof(val);
    val = 0xdeadbeef;
    res = InternetQueryOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER, &val, &len);
    ok(res,"Got wrong return value %x\n", res);
    ok(len == sizeof(val), "got %ld\n", len);
    trace("INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER: %ld\n", val);

    val = 3;
    res = InternetSetOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, sizeof(val));
    ok(res, "InternetSetOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER) failed: %x\n", res);

    len = sizeof(val);
    val = 0xdeadbeef;
    res = InternetQueryOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, &len);
    ok(res,"Got wrong return value %x\n", res);
    ok(len == sizeof(val), "got %ld\n", len);
    ok(val == 3, "got %ld\n", val);

    val = 0;
    res = InternetSetOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, sizeof(val));
    ok(!res || broken(res), /* <= w2k3 */
       "InternetSetOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER, 0) succeeded\n");
    if (!res) ok(GetLastError() == ERROR_BAD_ARGUMENTS, "GetLastError() = %lu\n", GetLastError());

    val = 2;
    res = InternetSetOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, sizeof(val)-1);
    ok(!res, "InternetSetOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_BAD_OPTION_LENGTH, "GetLastError() = %lu\n", GetLastError());

    val = 2;
    res = InternetSetOptionA(NULL, INTERNET_OPTION_MAX_CONNS_PER_SERVER, &val, sizeof(val)+1);
    ok(!res, "InternetSetOptionA(INTERNET_OPTION_MAX_CONNS_PER_SERVER) succeeded\n");
    ok(GetLastError() == ERROR_INTERNET_BAD_OPTION_LENGTH, "GetLastError() = %lu\n", GetLastError());
}

static void test_get_cookie(void)
{
  DWORD len;
  BOOL ret;

  len = 1024;
  SetLastError(0xdeadbeef);
  ret = InternetGetCookieA("http://www.example.com", NULL, NULL, &len);
  ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS,
    "InternetGetCookie should have failed with %s and error %ld\n",
    ret ? "TRUE" : "FALSE", GetLastError());
  ok(!len, "len = %lu\n", len);
}


static void test_complicated_cookie(void)
{
  DWORD len;
  BOOL ret;

  CHAR buffer[1024];
  WCHAR wbuf[1024];

  static const WCHAR testing_example_comW[] =
      {'h','t','t','p',':','/','/','t','e','s','t','i','n','g','.','e','x','a','m','p','l','e','.','c','o','m',0};

  ret = InternetSetCookieA("http://www.example.com/bar",NULL,"A=B; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com/bar",NULL,"C=D; domain=.example.com; path=/");
  ok(ret == TRUE,"InternetSetCookie failed\n");

  /* Technically illegal! domain should require 2 dots, but native wininet accepts it */
  ret = InternetSetCookieA("http://www.example.com",NULL,"E=F; domain=example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com",NULL,"G=H; domain=.example.com; invalid=attr; path=/foo");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com/bar.html",NULL,"I=J; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com/bar/",NULL,"K=L; domain=.example.com");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com/bar/",NULL,"M=N; domain=.example.com; path=/foo/");
  ok(ret == TRUE,"InternetSetCookie failed\n");
  ret = InternetSetCookieA("http://www.example.com/bar/",NULL,"O=P; secure; path=/bar");
  ok(ret == TRUE,"InternetSetCookie failed\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com", NULL, NULL, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(len == 19, "len = %lu\n", len);

  len = 1024;
  memset(buffer, 0xac, sizeof(buffer));
  ret = InternetGetCookieA("http://testing.example.com", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(len == 19, "len = %lu\n", len);
  ok(strlen(buffer) == 18, "strlen(buffer) = %Iu\n", strlen(buffer));
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 10;
  memset(buffer, 0xac, sizeof(buffer));
  ret = InternetGetCookieA("http://testing.example.com", NULL, buffer, &len);
  ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "InternetGetCookie returned: %x(%lu), expected ERROR_INSUFFICIENT_BUFFER\n", ret, GetLastError());
  ok(len == 19, "len = %lu\n", len);

  len = 1024;
  ret = InternetGetCookieW(testing_example_comW, NULL, NULL, &len);
  ok(ret == TRUE,"InternetGetCookieW failed\n");
  ok(len == 38, "len = %lu\n", len);

  len = 1024;
  memset(wbuf, 0xac, sizeof(wbuf));
  ret = InternetGetCookieW(testing_example_comW, NULL, wbuf, &len);
  ok(ret == TRUE,"InternetGetCookieW failed\n");
  ok(len == 19 || broken(len==18), "len = %lu\n", len);
  ok(lstrlenW(wbuf) == 18, "strlenW(wbuf) = %u\n", lstrlenW(wbuf));

  len = 10;
  memset(wbuf, 0xac, sizeof(wbuf));
  ret = InternetGetCookieW(testing_example_comW, NULL, wbuf, &len);
  ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "InternetGetCookieW returned: %x(%lu), expected ERROR_INSUFFICIENT_BUFFER\n", ret, GetLastError());
  ok(len == 38, "len = %lu\n", len);

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/foobar", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/foobar/", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")!=NULL,"G=H missing\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/foo/bar", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")!=NULL,"G=H missing\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")!=NULL,"M=N missing\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/barfoo", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/barfoo/", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")==NULL,"K=L present\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/bar/foo", NULL, buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(len == 24, "len = %lu\n", len);
  ok(strstr(buffer,"A=B")!=NULL,"A=B missing\n");
  ok(strstr(buffer,"C=D")!=NULL,"C=D missing\n");
  ok(strstr(buffer,"E=F")!=NULL,"E=F missing\n");
  ok(strstr(buffer,"G=H")==NULL,"G=H present\n");
  ok(strstr(buffer,"I=J")!=NULL,"I=J missing\n");
  ok(strstr(buffer,"K=L")!=NULL,"K=L missing\n");
  ok(strstr(buffer,"M=N")==NULL,"M=N present\n");
  ok(strstr(buffer,"O=P")==NULL,"O=P present\n");

  /* Cookie name argument is not implemented */
  len = 1024;
  ret = InternetGetCookieA("http://testing.example.com/bar/foo", "A", buffer, &len);
  ok(ret == TRUE,"InternetGetCookie failed\n");
  ok(len == 24, "len = %lu\n", len);

  /* test persistent cookies */
  ret = InternetSetCookieA("http://testing.example.com", NULL, "A=B; expires=Fri, 01-Jan-2038 00:00:00 GMT");
  ok(ret, "InternetSetCookie failed with error %ld\n", GetLastError());

  /* test invalid expires parameter */
  ret = InternetSetCookieA("http://testing.example.com", NULL, "Q=R; expires=");
  ok(ret, "InternetSetCookie failed %#lx.\n", GetLastError());
  len = 1024;
  memset(buffer, 0xac, sizeof(buffer));
  ret = InternetGetCookieA("http://testing.example.com/", NULL, buffer, &len);
  ok(ret, "InternetGetCookie failed %#lx.\n", GetLastError());
  ok(len == 29, "got len %lu.\n", len);
  ok(!!strstr(buffer, "Q=R"), "cookie is not present.\n");

  len = sizeof(buffer);
  ret = InternetGetCookieA("http://testing.example.com/foobar", NULL, buffer, &len);
  ok(ret, "got error %#lx\n", GetLastError());
  ok(len == 29, "got len %lu\n", len);
  ok(!!strstr(buffer, "A=B"), "cookie is not present\n");

  /* remove persistent cookie */
  ret = InternetSetCookieA("http://testing.example.com", NULL, "A=B");
  ok(ret, "InternetSetCookie failed with error %ld\n", GetLastError());

  /* try setting cookie for different domain */
  ret = InternetSetCookieA("http://www.aaa.example.com/bar",NULL,"E=F; domain=different.com");
  ok(!ret, "InternetSetCookie succeeded\n");
  ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %ld\n", GetLastError());
  ret = InternetSetCookieA("http://www.aaa.example.com.pl/bar",NULL,"E=F; domain=example.com.pl");
  ok(ret, "InternetSetCookie failed with error: %ld\n", GetLastError());
  ret = InternetSetCookieA("http://www.aaa.example.com.pl/bar",NULL,"E=F; domain=com.pl");
  todo_wine ok(!ret, "InternetSetCookie succeeded\n");
}

static void test_cookie_attrs(void)
{
    char buf[100];
    DWORD size, state;
    BOOL ret;

    if(!GetProcAddress(GetModuleHandleA("wininet.dll"), "DeleteWpadCacheForNetworks")) {
        win_skip("Skipping cookie attributes tests. Too old IE.\n");
        return;
    }

    ret = InternetSetCookieA("http://cookie.attrs.com/bar", NULL, "A=data; httponly");
    ok(!ret && GetLastError() == ERROR_INVALID_OPERATION, "InternetSetCookie returned: %x (%lu)\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    state = InternetSetCookieExA("http://cookie.attrs.com/bar", NULL, "A=data; httponly", 0, 0);
    ok(state == COOKIE_STATE_REJECT && GetLastError() == ERROR_INVALID_OPERATION,
       "InternetSetCookieEx returned: %x (%lu)\n", ret, GetLastError());

    size = sizeof(buf);
    ret = InternetGetCookieExA("http://cookie.attrs.com/", NULL, buf, &size, INTERNET_COOKIE_HTTPONLY, NULL);
    ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS, "InternetGetCookieEx returned: %x (%lu)\n", ret, GetLastError());

    state = InternetSetCookieExA("http://cookie.attrs.com/bar",NULL,"A=data; httponly", INTERNET_COOKIE_HTTPONLY, 0);
    ok(state == COOKIE_STATE_ACCEPT,"InternetSetCookieEx failed: %lu\n", GetLastError());

    size = sizeof(buf);
    ret = InternetGetCookieA("http://cookie.attrs.com/", NULL, buf, &size);
    ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS, "InternetGetCookie returned: %x (%lu)\n", ret, GetLastError());

    size = sizeof(buf);
    ret = InternetGetCookieExA("http://cookie.attrs.com/", NULL, buf, &size, 0, NULL);
    ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS, "InternetGetCookieEx returned: %x (%lu)\n", ret, GetLastError());

    size = sizeof(buf);
    ret = InternetGetCookieExA("http://cookie.attrs.com/", NULL, buf, &size, INTERNET_COOKIE_HTTPONLY, NULL);
    ok(ret, "InternetGetCookieEx failed: %lu\n", GetLastError());
    ok(!strcmp(buf, "A=data"), "data = %s\n", buf);

    /* Try to override httponly cookie with non-httponly one */
    ret = InternetSetCookieA("http://cookie.attrs.com/bar", NULL, "A=test");
    ok(!ret && GetLastError() == ERROR_INVALID_OPERATION, "InternetSetCookie returned: %x (%lu)\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    state = InternetSetCookieExA("http://cookie.attrs.com/bar", NULL, "A=data", 0, 0);
    ok(state == COOKIE_STATE_REJECT && GetLastError() == ERROR_INVALID_OPERATION,
       "InternetSetCookieEx returned: %x (%lu)\n", ret, GetLastError());

    size = sizeof(buf);
    ret = InternetGetCookieExA("http://cookie.attrs.com/", NULL, buf, &size, INTERNET_COOKIE_HTTPONLY, NULL);
    ok(ret, "InternetGetCookieEx failed: %lu\n", GetLastError());
    ok(!strcmp(buf, "A=data"), "data = %s\n", buf);

}

static void test_cookie_url(void)
{
    char long_url[5000] = "http://long.url.test.com/", *p;
    WCHAR bufw[512];
    char buf[512];
    DWORD len;
    BOOL res;

    static const WCHAR about_blankW[] = {'a','b','o','u','t',':','b','l','a','n','k',0};

    len = sizeof(buf);
    res = InternetGetCookieA("about:blank", NULL, buf, &len);
    ok(!res && GetLastError() == ERROR_INVALID_PARAMETER,
       "InternetGetCookeA failed: %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    len = ARRAY_SIZE(bufw);
    res = InternetGetCookieW(about_blankW, NULL, bufw, &len);
    ok(!res && GetLastError() == ERROR_INVALID_PARAMETER,
       "InternetGetCookeW failed: %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    len = sizeof(buf);
    res = pInternetGetCookieExA("about:blank", NULL, buf, &len, 0, NULL);
    ok(!res && GetLastError() == ERROR_INVALID_PARAMETER,
       "InternetGetCookeExA failed: %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    len = ARRAY_SIZE(bufw);
    res = pInternetGetCookieExW(about_blankW, NULL, bufw, &len, 0, NULL);
    ok(!res && GetLastError() == ERROR_INVALID_PARAMETER,
       "InternetGetCookeExW failed: %lu, expected ERROR_INVALID_PARAMETER\n", GetLastError());

    p = long_url + strlen(long_url);
    memset(p, 'x', long_url+sizeof(long_url)-p);
    p += (long_url+sizeof(long_url)-p) - 3;
    p[0] = '/';
    p[2] = 0;
    res = InternetSetCookieA(long_url, NULL, "A=B");
    ok(res, "InternetSetCookieA failed: %lu\n", GetLastError());

    len = sizeof(buf);
    res = InternetGetCookieA(long_url, NULL, buf, &len);
    ok(res, "InternetGetCookieA failed: %lu\n", GetLastError());
    ok(!strcmp(buf, "A=B"), "buf = %s\n", buf);

    len = sizeof(buf);
    res = InternetGetCookieA("http://long.url.test.com/", NULL, buf, &len);
    ok(!res && GetLastError() == ERROR_NO_MORE_ITEMS, "InternetGetCookieA failed: %lu\n", GetLastError());
}

static void test_null(void)
{
  HINTERNET hi, hc;
  static const WCHAR szServer[] = { 's','e','r','v','e','r',0 };
  static const WCHAR szServer2[] = { 's','e','r','v','e','r','=',0 };
  static const WCHAR szEmpty[] = { 0 };
  static const WCHAR szUrl[] = { 'h','t','t','p',':','/','/','a','.','b','.','c',0 };
  static const WCHAR szUrlEmpty[] = { 'h','t','t','p',':','/','/',0 };
  static const WCHAR szExpect[] = { 's','e','r','v','e','r',';',' ','s','e','r','v','e','r',0 };
  WCHAR buffer[0x20];
  BOOL r;
  DWORD sz;

  SetLastError(0xdeadbeef);
  hi = InternetOpenW(NULL, 0, NULL, NULL, 0);
  if (hi == NULL && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
  {
    win_skip("Internet*W functions are not implemented\n");
    return;
  }
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
  ok(GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME, "wrong error\n");
  ok(r == FALSE, "return wrong\n");

  sz = 0;
  r = InternetGetCookieW(NULL, NULL, NULL, &sz);
  ok(GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INTERNET_UNRECOGNIZED_SCHEME,
     "wrong error %lu\n", GetLastError());
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

  /* sz is 14 on XP SP2 and beyond, 30 on XP SP1 and before, 16 on IE11 */
  ok( sz == 14 || sz == 16 || sz == 30, "sz wrong, got %lu, expected 14, 16 or 30\n", sz);

  sz = 0x20;
  memset(buffer, 0, sizeof buffer);
  r = InternetGetCookieW(szUrl, szServer, buffer, &sz);
  ok( r == TRUE, "return wrong\n");

  /* sz == lstrlenW(buffer) only in XP SP1 */
  ok( sz == 1 + lstrlenW(buffer) || sz == lstrlenW(buffer), "sz wrong %ld\n", sz);

  /* before XP SP2, buffer is "server; server" */
  ok( !lstrcmpW(szExpect, buffer) || !lstrcmpW(szServer, buffer) || !lstrcmpW(szServer2, buffer),
      "cookie data wrong %s\n", wine_dbgstr_w(buffer));

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
    ok(res, "Could not get version: %lu\n", GetLastError());
    ok(version.dwMajorVersion == 1, "dwMajorVersion=%ld, expected 1\n", version.dwMajorVersion);
    ok(version.dwMinorVersion == 2, "dwMinorVersion=%ld, expected 2\n", version.dwMinorVersion);
}

static void InternetTimeFromSystemTimeA_test(void)
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    char string[INTERNET_RFC1123_BUFSIZE];
    static const char expect[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    DWORD error;

    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeA failed (%lu)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeA failed (%lu)\n", GetLastError() );

    /* test NULL time parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( NULL, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test NULL string parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, NULL, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test invalid format parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT + 1, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeA failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test too small buffer size */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, 0 );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeA should have returned FALSE\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER,
        "InternetTimeFromSystemTimeA failed with ERROR_INSUFFICIENT_BUFFER instead of %lu\n",
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
    ok( ret, "InternetTimeFromSystemTimeW failed (%lu)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeW failed (%lu)\n", GetLastError() );

    /* test NULL time parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( NULL, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test NULL string parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, NULL, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test invalid format parameter */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT + 1, string, sizeof(string) );
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INVALID_PARAMETER,
        "InternetTimeFromSystemTimeW failed with ERROR_INVALID_PARAMETER instead of %lu\n",
        error );

    /* test too small buffer size */
    SetLastError(0xdeadbeef);
    ret = pInternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, ARRAY_SIZE(string));
    error = GetLastError();
    ok( !ret, "InternetTimeFromSystemTimeW should have returned FALSE\n" );
    ok( error == ERROR_INSUFFICIENT_BUFFER,
        "InternetTimeFromSystemTimeW failed with ERROR_INSUFFICIENT_BUFFER instead of %lu\n",
        error );
}

static void test_InternetTimeToSystemTime(void)
{
    BOOL ret;
    unsigned int i;
    SYSTEMTIME time;
    WCHAR buffer[64];
    static const SYSTEMTIME expect1 = { 2005, 1, 5, 7,  12, 6,  35, 0 };
    static const SYSTEMTIME expect2 = { 2022, 1, 2, 11, 11, 13, 5,  0 };

    static const struct test_data
    {
        const char *string;
        const SYSTEMTIME *expect;
        BOOL match;
        BOOL todo;
    }
    test_data[] =
    {
        { "Fri, 07 Jan 2005 12:06:35 GMT", &expect1, TRUE },
        { "Fri, 07 Jan 2005 12:06:35 UTC", &expect1, TRUE },
        { " fri, 7 jan 2005 12 06 35",     &expect1, TRUE },
        { "Fri, 07-01-2005 12:06:35",      &expect1, TRUE },
        { "5, 07-01-2005 12:06:35 GMT",    &expect1, TRUE },
        { "5, 07-01-2005 12:06:35 UTC",    &expect1, TRUE },
        { "5, 07-01-2005 12:06:35 GMT;",   &expect1, TRUE },
        { "5, 07-01-2005 12:06:35 GMT123", &expect1, TRUE },
        { "2, 11 01 2022 11 13 05",        &expect2, TRUE },
        { "2, 11-01-2022 11#13^05",        &expect2, TRUE },
        { "2, 11*01/2022 11+13=05",        &expect2, TRUE },
        { "2, 11-Jan-2022 11:13:05",       &expect2, TRUE },
        { "Fr",                            NULL,     FALSE },
        { "Fri Jan 7 12:06:35 2005",       &expect1, TRUE, TRUE },
        { "Fri Jan 7 12:06:35 2005 GMT",   &expect1, TRUE, TRUE },
        { "Fri Jan 7 12:06:35 2005 UTC",   &expect1, TRUE, TRUE },
    };

    ret = pInternetTimeToSystemTimeA(NULL, NULL, 0);
    ok(!ret, "InternetTimeToSystemTimeA succeeded.\n");
    ret = pInternetTimeToSystemTimeA(NULL, &time, 0);
    ok(!ret, "InternetTimeToSystemTimeA succeeded.\n");
    ret = pInternetTimeToSystemTimeW(NULL, NULL, 0);
    ok(!ret, "InternetTimeToSystemTimeW succeeded.\n");
    ret = pInternetTimeToSystemTimeW(NULL, &time, 0);
    ok(!ret, "InternetTimeToSystemTimeW succeeded.\n");

    for (i = 0; i < ARRAY_SIZE(test_data); ++i)
    {
        const struct test_data *test = &test_data[i];
        winetest_push_context("Test %u", i);

        memset(&time, 0, sizeof(time));
        ret = pInternetTimeToSystemTimeA(test->string, NULL, 0);
        ok(!ret, "InternetTimeToSystemTimeA succeeded.\n");
        ret = pInternetTimeToSystemTimeA(test->string, &time, 0);
        ok(ret, "InternetTimeToSystemTimeA failed: %lu.\n", GetLastError());
        todo_wine_if(test->todo)
        ok(!test->match || !memcmp(&time, test->expect, sizeof(*test->expect)),
           "Got unexpected system time.\n");

        MultiByteToWideChar(CP_ACP, 0, test->string, -1, buffer, ARRAY_SIZE(buffer));

        memset(&time, 0, sizeof(time));
        ret = pInternetTimeToSystemTimeW(buffer, NULL, 0);
        ok(!ret, "InternetTimeToSystemTimeW succeeded.\n");
        ret = pInternetTimeToSystemTimeW(buffer, &time, 0);
        ok(ret, "InternetTimeToSystemTimeW failed: %lu.\n", GetLastError());
        todo_wine_if(test->todo)
        ok(!test->match || !memcmp(&time, test->expect, sizeof(*test->expect)),
           "Got unexpected system time.\n");

        winetest_pop_context();
    }
}

static void test_IsDomainLegalCookieDomainW(void)
{
    BOOL ret;
    static const WCHAR empty[]          = {0};
    static const WCHAR dot[]            = {'.',0};
    static const WCHAR uk[]             = {'u','k',0};
    static const WCHAR com[]            = {'c','o','m',0};
    static const WCHAR dot_com[]        = {'.','c','o','m',0};
    static const WCHAR gmail_com[]      = {'g','m','a','i','l','.','c','o','m',0};
    static const WCHAR dot_gmail_com[]  = {'.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR www_gmail_com[]  = {'w','w','w','.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR www_mail_gmail_com[] = {'w','w','w','.','m','a','i','l','.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR mail_gmail_com[] = {'m','a','i','l','.','g','m','a','i','l','.','c','o','m',0};
    static const WCHAR gmail_co_uk[]    = {'g','m','a','i','l','.','c','o','.','u','k',0};
    static const WCHAR co_uk[]          = {'c','o','.','u','k',0};
    static const WCHAR dot_co_uk[]      = {'.','c','o','.','u','k',0};

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(NULL, NULL);
    if (!ret && (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED))
    {
        win_skip("IsDomainLegalCookieDomainW is not implemented\n");
        return;
    }
    ok(!ret ||
        broken(ret), /* IE6 */
        "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, NULL);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(NULL, gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(empty, gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, empty);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(gmail_com, dot);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot, gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, dot_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot_com, com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(com, gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(gmail_com, gmail_com);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    ret = pIsDomainLegalCookieDomainW(gmail_com, www_gmail_com);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    ret = pIsDomainLegalCookieDomainW(gmail_com, www_mail_gmail_com);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, co_uk);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(uk, co_uk);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, dot_co_uk);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(co_uk, gmail_co_uk);
    todo_wine ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    ret = pIsDomainLegalCookieDomainW(gmail_co_uk, gmail_co_uk);
    ok(ret, "IsDomainLegalCookieDomainW failed\n");

    ret = pIsDomainLegalCookieDomainW(gmail_com, com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

    SetLastError(0xdeadbeef);
    ret = pIsDomainLegalCookieDomainW(dot_gmail_com, mail_gmail_com);
    ok(!ret, "IsDomainLegalCookieDomainW succeeded\n");

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

static void test_PrivacyGetSetZonePreferenceW(void)
{
    DWORD ret, zone, type, template, old_template, pref_size = 0;
    WCHAR pref[256];

    zone = 3;
    type = 0;
    ret = pPrivacyGetZonePreferenceW(zone, type, NULL, NULL, NULL);
    ok(ret == 0, "expected ret == 0, got %lu\n", ret);

    old_template = 0;
    ret = pPrivacyGetZonePreferenceW(zone, type, &old_template, NULL, NULL);
    ok(ret == 0, "expected ret == 0, got %lu\n", ret);

    trace("template %lu\n", old_template);

    if(old_template == PRIVACY_TEMPLATE_ADVANCED) {
        pref_size = ARRAY_SIZE(pref);
        ret = pPrivacyGetZonePreferenceW(zone, type, &old_template, pref, &pref_size);
        ok(ret == 0, "expected ret == 0, got %lu\n", ret);
    }

    template = 5;
    ret = pPrivacySetZonePreferenceW(zone, type, template, NULL);
    ok(ret == 0, "expected ret == 0, got %lu\n", ret);

    template = 0;
    ret = pPrivacyGetZonePreferenceW(zone, type, &template, NULL, NULL);
    ok(ret == 0, "expected ret == 0, got %lu\n", ret);
    ok(template == 5, "expected template == 5, got %lu\n", template);

    template = 5;
    ret = pPrivacySetZonePreferenceW(zone, type, old_template, pref_size ? pref : NULL);
    ok(ret == 0, "expected ret == 0, got %lu\n", ret);
}

static void test_InternetSetOption(void)
{
    HINTERNET ses, con, req;
    ULONG ulArg;
    DWORD size;
    BOOL ret;

    ses = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != 0, "InternetOpen failed: 0x%08lx\n", GetLastError());
    con = InternetConnectA(ses, "www.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != 0, "InternetConnect failed: 0x%08lx\n", GetLastError());
    req = HttpOpenRequestA(con, "GET", "/", NULL, NULL, NULL, 0, 0);
    ok(req != 0, "HttpOpenRequest failed: 0x%08lx\n", GetLastError());

    /* INTERNET_OPTION_POLICY tests */
    SetLastError(0xdeadbeef);
    ret = InternetSetOptionW(ses, INTERNET_OPTION_POLICY, NULL, 0);
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError should've "
            "given ERROR_INVALID_PARAMETER, gave: 0x%08lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InternetQueryOptionW(ses, INTERNET_OPTION_POLICY, NULL, 0);
    ok(ret == FALSE, "InternetQueryOption should've failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError should've "
            "given ERROR_INVALID_PARAMETER, gave: 0x%08lx\n", GetLastError());

    /* INTERNET_OPTION_ERROR_MASK tests */
    SetLastError(0xdeadbeef);
    size = sizeof(ulArg);
    ret = InternetQueryOptionW(NULL, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, &size);
    ok(ret == FALSE, "InternetQueryOption should've failed\n");
    ok(GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "GetLastError() = %lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ulArg = 11;
    ret = InternetSetOptionA(NULL, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, sizeof(ULONG));
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "GetLastError() = %lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ulArg = 11;
    ret = InternetSetOptionA(req, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, 20);
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INTERNET_BAD_OPTION_LENGTH, "GetLastError() = %ld\n", GetLastError());

    ulArg = 11;
    ret = InternetSetOptionA(req, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, sizeof(ULONG));
    ok(ret == TRUE, "InternetSetOption should've succeeded\n");

    SetLastError(0xdeadbeef);
    ulArg = 4;
    ret = InternetSetOptionA(req, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, sizeof(ULONG));
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %lx\n", GetLastError());

    SetLastError(0xdeadbeef);
    ulArg = 16;
    ret = InternetSetOptionA(req, INTERNET_OPTION_ERROR_MASK, (void*)&ulArg, sizeof(ULONG));
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError() = %lx\n", GetLastError());

    ret = InternetSetOptionA(req, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(ret == TRUE, "InternetSetOption should've succeeded\n");

    ret = InternetSetOptionA(con, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(ret == TRUE, "InternetSetOption should've succeeded\n");

    ret = InternetSetOptionA(ses, INTERNET_OPTION_SETTINGS_CHANGED, NULL, 0);
    ok(ret == TRUE, "InternetSetOption should've succeeded\n");

    ret = InternetSetOptionA(ses, INTERNET_OPTION_REFRESH, NULL, 0);
    ok(ret == TRUE, "InternetSetOption should've succeeded\n");

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(req, INTERNET_OPTION_REFRESH, NULL, 0);
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "GetLastError() = %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = InternetSetOptionA(con, INTERNET_OPTION_REFRESH, NULL, 0);
    ok(ret == FALSE, "InternetSetOption should've failed\n");
    ok(GetLastError() == ERROR_INTERNET_INCORRECT_HANDLE_TYPE, "GetLastError() = %lu\n", GetLastError());

    ret = InternetCloseHandle(req);
    ok(ret == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
    ret = InternetCloseHandle(con);
    ok(ret == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
    ret = InternetCloseHandle(ses);
    ok(ret == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
}

static void test_end_browser_session(void)
{
    DWORD len;
    BOOL ret;

    ret = InternetSetCookieA("http://www.example.com/test_end", NULL, "A=B");
    ok(ret == TRUE, "InternetSetCookie failed\n");

    len = 1024;
    ret = InternetGetCookieA("http://www.example.com/test_end", NULL, NULL, &len);
    ok(ret == TRUE,"InternetGetCookie failed\n");
    ok(len != 0, "len = 0\n");

    ret = InternetSetOptionA(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);
    ok(ret, "InternetSetOption(INTERNET_OPTION_END_BROWSER_SESSION) failed: %lu\n", GetLastError());

    len = 1024;
    ret = InternetGetCookieA("http://www.example.com/test_end", NULL, NULL, &len);
    ok(!ret && GetLastError() == ERROR_NO_MORE_ITEMS, "InternetGetCookie returned %x (%lu)\n", ret, GetLastError());
    ok(!len, "len = %lu\n", len);
}

static void test_Option_PerConnectionOption(void)
{
    BOOL ret;
    DWORD size = sizeof(INTERNET_PER_CONN_OPTION_LISTW);
    INTERNET_PER_CONN_OPTION_LISTW list = {size};
    INTERNET_PER_CONN_OPTIONW orig_settings[2], options[2];
    static WCHAR proxy_srvW[] = L"proxy.example";
    HINTERNET ses;

    ses = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != 0, "InternetOpen failed: 0x%08lx\n", GetLastError());

    /* get the process IE proxy server info, to restore later */
    list.dwOptionCount = 2;
    list.pOptions = orig_settings;
    orig_settings[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    orig_settings[1].dwOption = INTERNET_PER_CONN_FLAGS;

    ret = InternetQueryOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");

    /* set the process IE proxy server */
    list.pOptions = options;
    options[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[0].Value.pszValue = proxy_srvW;
    options[1].dwOption = INTERNET_PER_CONN_FLAGS;
    options[1].Value.dwValue = PROXY_TYPE_PROXY;

    ret = InternetSetOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    /* get & verify the process IE proxy server */
    options[0].Value.pszValue = NULL;
    options[1].Value.dwValue = 0;

    ret = InternetQueryOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    ok(!lstrcmpW(options[0].Value.pszValue, proxy_srvW),
            "Retrieved proxy server should've been %s, was: %s\n",
            wine_dbgstr_w(proxy_srvW), wine_dbgstr_w(options[0].Value.pszValue));
    ok(options[1].Value.dwValue == PROXY_TYPE_PROXY,
            "Retrieved flags should've been PROXY_TYPE_PROXY, was: %ld\n",
            options[1].Value.dwValue);

    ret = HeapValidate(GetProcessHeap(), 0, options[0].Value.pszValue);
    ok(ret, "HeapValidate failed, last error %lu\n", GetLastError());
    GlobalFree(options[0].Value.pszValue);

    /* verify that global proxy settings were not changed */
    ret = InternetQueryOptionW(NULL, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    ok(lstrcmpW(options[0].Value.pszValue, proxy_srvW),
            "Retrieved proxy server should've been %s, was: %s\n",
            wine_dbgstr_w(proxy_srvW), wine_dbgstr_w(options[0].Value.pszValue));

    /* disable the proxy server */
    list.dwOptionCount = 1;
    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    options[0].Value.dwValue = PROXY_TYPE_DIRECT;

    ret = InternetSetOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    /* verify that the proxy is disabled */
    options[0].Value.dwValue = 0;

    ret = InternetQueryOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    ok(options[0].Value.dwValue == PROXY_TYPE_DIRECT,
            "Retrieved flags should've been PROXY_TYPE_DIRECT, was: %ld\n",
            options[0].Value.dwValue);

    /* set the proxy flags to 'invalid' value */
    options[0].dwOption = INTERNET_PER_CONN_FLAGS;
    options[0].Value.dwValue = PROXY_TYPE_PROXY | PROXY_TYPE_DIRECT;

    ret = InternetSetOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    /* verify that the proxy is enabled */
    options[0].Value.dwValue = 0;

    ret = InternetQueryOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    todo_wine ok(options[0].Value.dwValue == PROXY_TYPE_DIRECT,
            "Retrieved flags should've been PROXY_TYPE_DIRECT, was: %ld\n",
            options[0].Value.dwValue);

    /* restore original settings */
    list.dwOptionCount = 2;
    list.pOptions = orig_settings;

    ret = InternetSetOptionW(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    GlobalFree(orig_settings[0].Value.pszValue);
    InternetCloseHandle(ses);
}

static void test_Option_PerConnectionOptionA(void)
{
    BOOL ret;
    DWORD size = sizeof(INTERNET_PER_CONN_OPTION_LISTA);
    INTERNET_PER_CONN_OPTION_LISTA list = {size};
    INTERNET_PER_CONN_OPTIONA orig_settings[2], options[2];
    char proxy_srv[] = "proxy.example";
    HINTERNET ses;

    ses = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != 0, "InternetOpen failed: 0x%08lx\n", GetLastError());

    /* get the process IE proxy server info, to restore later */
    list.dwOptionCount = 2;
    list.pOptions = orig_settings;
    orig_settings[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    orig_settings[1].dwOption = INTERNET_PER_CONN_FLAGS;

    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION,
            &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");

    /* set the process IE proxy server */
    list.pOptions = options;
    options[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[0].Value.pszValue = proxy_srv;
    options[1].dwOption = INTERNET_PER_CONN_FLAGS;
    options[1].Value.dwValue = PROXY_TYPE_PROXY;

    ret = InternetSetOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION,
            &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    /* get & verify the process IE proxy server */
    options[0].Value.pszValue = NULL;
    options[1].Value.dwValue = 0;

    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION,
            &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    ok(!lstrcmpA(options[0].Value.pszValue, "proxy.example"),
            "Retrieved proxy server should've been \"%s\", was: \"%s\"\n",
            proxy_srv, options[0].Value.pszValue);
    ok(options[1].Value.dwValue == PROXY_TYPE_PROXY,
            "Retrieved flags should've been PROXY_TYPE_PROXY, was: %ld\n",
            options[1].Value.dwValue);

    GlobalFree(options[0].Value.pszValue);

    /* test with NULL as proxy server */
    list.dwOptionCount = 1;
    options[0].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    options[0].Value.pszValue = NULL;

    ret = InternetSetOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    /* get & verify the proxy server */
    options[0].Value.dwValue = 0xdeadbeef;

    ret = InternetQueryOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, &size);
    ok(ret, "InternetQueryOption should've succeeded\n");
    ok(!options[0].Value.pszValue,
            "Retrieved proxy server should've been NULL, was: \"%s\"\n",
            options[0].Value.pszValue);

    /* restore original settings */
    list.dwOptionCount = 2;
    list.pOptions = orig_settings;

    ret = InternetSetOptionA(ses, INTERNET_OPTION_PER_CONNECTION_OPTION, &list, size);
    ok(ret, "InternetSetOption should've succeeded\n");

    GlobalFree(orig_settings[0].Value.pszValue);
    InternetCloseHandle(ses);
}

#define FLAG_NEEDREQ  0x1
#define FLAG_UNIMPL   0x2

static void test_InternetErrorDlg(void)
{
    HINTERNET ses, con, req;
    DWORD res;
    HWND hwnd;
    ULONG i;
    static const struct {
        DWORD error;
        DWORD res;
        DWORD test_flags;
    } no_ui_res[] = {
        { ERROR_INTERNET_INCORRECT_PASSWORD     , ERROR_SUCCESS, FLAG_NEEDREQ },
        { ERROR_INTERNET_SEC_CERT_DATE_INVALID  , ERROR_CANCELLED, 0 },
        { ERROR_INTERNET_SEC_CERT_CN_INVALID    , ERROR_CANCELLED, 0 },
        { ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR , ERROR_SUCCESS, 0 },
        { ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR , ERROR_SUCCESS },
        { ERROR_INTERNET_MIXED_SECURITY         , ERROR_CANCELLED },
        { ERROR_INTERNET_CHG_POST_IS_NON_SECURE , ERROR_SUCCESS },
        { ERROR_INTERNET_POST_IS_NON_SECURE     , ERROR_SUCCESS, 0 },
        { ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED, ERROR_CANCELLED, FLAG_NEEDREQ },
        { ERROR_INTERNET_INVALID_CA             , ERROR_CANCELLED, 0 },
        { ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR, ERROR_CANCELLED },
        { ERROR_INTERNET_INSERT_CDROM           , ERROR_CANCELLED, FLAG_NEEDREQ|FLAG_UNIMPL },
        { ERROR_INTERNET_SEC_CERT_ERRORS        , ERROR_CANCELLED, 0 },
        { ERROR_INTERNET_SEC_CERT_REV_FAILED    , ERROR_CANCELLED, 0 },
        { ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION  , ERROR_HTTP_COOKIE_DECLINED },
        { ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT  , ERROR_CANCELLED },
        { ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT, ERROR_CANCELLED },
        { ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION, ERROR_CANCELLED },
        { ERROR_INTERNET_SEC_CERT_REVOKED       , ERROR_CANCELLED },
        { ERROR_INTERNET_SEC_CERT_WEAK_SIGNATURE, ERROR_CANCELLED },
    };

    res = InternetErrorDlg(NULL, NULL, ERROR_INTERNET_SEC_CERT_ERRORS, 0, NULL);
    ok(res == ERROR_INVALID_HANDLE, "Got %ld\n", res);

    ses = InternetOpenA(NULL, INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    ok(ses != 0, "InternetOpen failed: 0x%08lx\n", GetLastError());
    con = InternetConnectA(ses, "www.winehq.org", 80, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    ok(con != 0, "InternetConnect failed: 0x%08lx\n", GetLastError());
    req = HttpOpenRequestA(con, "GET", "/", NULL, NULL, NULL, 0, 0);
    ok(req != 0, "HttpOpenRequest failed: 0x%08lx\n", GetLastError());

    hwnd = GetDesktopWindow();
    ok(hwnd != NULL, "GetDesktopWindow failed (%ld)\n", GetLastError());

    for(i = INTERNET_ERROR_BASE; i < INTERNET_ERROR_LAST; i++)
    {
        DWORD expected = ERROR_CANCELLED, expected2 = ERROR_SUCCESS, broken2, test_flags = 0, j;

        for (j = 0; j < ARRAY_SIZE(no_ui_res); ++j)
        {
            if (no_ui_res[j].error == i)
            {
                test_flags = no_ui_res[j].test_flags;
                expected = no_ui_res[j].res;
                expected2 = no_ui_res[j].res;
            }
        }

        /* Try an invalid request handle */
        res = InternetErrorDlg(hwnd, (HANDLE)0xdeadbeef, i, FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
        if(res == ERROR_CALL_NOT_IMPLEMENTED)
        {
            ok(test_flags & FLAG_UNIMPL, "%li is unexpectedly unimplemented.\n", i);
            continue;
        }
        else
            ok(res == ERROR_INVALID_HANDLE, "Got %ld (%ld)\n", res, i);

        /* With a valid req */
        if(i == ERROR_INTERNET_NEED_UI)
            continue; /* Crashes on windows XP */

        if(i == ERROR_INTERNET_SEC_CERT_REVOKED)
            continue; /* Interactive (XP, Win7) */
        if (i == ERROR_INTERNET_PROXY_ALERT)
            continue; /* Interactive (Win10 1607+) */

        res = InternetErrorDlg(hwnd, req, i, FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);

        /* Handle some special cases */
        broken2 = expected2;
        switch(i)
        {
        case ERROR_INTERNET_CHG_POST_IS_NON_SECURE:
            if (broken(res == ERROR_CANCELLED)) /* before win10 returns ERROR_CANCELLED */
            {
                expected = ERROR_CANCELLED;
                expected2 = broken2 = ERROR_CANCELLED;
            }
            break;
        case ERROR_INTERNET_SEC_CERT_WEAK_SIGNATURE:
            if (res == ERROR_CANCELLED)
            {
                expected = ERROR_CANCELLED;
                expected2 = ERROR_CANCELLED;
                broken2 = ERROR_SUCCESS; /* win10 1607 */
                break;
            }
            /* fall through */
        default:
            if(broken(expected == ERROR_CANCELLED && res == ERROR_NOT_SUPPORTED)) /* XP, Win7, Win8 */
            {
                expected = ERROR_NOT_SUPPORTED;
                expected2 = broken2 = ERROR_SUCCESS;
            }
            break;
        }

        ok(res == expected, "Got %ld, expected %ld (%ld)\n", res, expected, i);

        /* Same thing with NULL hwnd */
        res = InternetErrorDlg(NULL, req, i, FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
        ok(res == expected, "Got %ld, expected %ld (%ld)\n", res, expected, i);

        res = InternetErrorDlg(NULL, req, i, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
        ok(res == expected2 || broken(res == broken2),
           "Got %ld, expected %ld (%ld)\n", res, expected2, i);

        /* With a null req */
        if(test_flags & FLAG_NEEDREQ)
        {
            expected = ERROR_INVALID_PARAMETER;
            expected2 = ERROR_INVALID_PARAMETER;
        }

        res = InternetErrorDlg(hwnd, NULL, i, FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
        ok(res == expected, "Got %ld, expected %ld (%ld)\n", res, expected, i);

        res = InternetErrorDlg(NULL, NULL, i, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
        ok(res == expected2 || broken(res == broken2),
           "Got %ld, expected %ld (%ld)\n", res, expected2, i);
    }

    res = InternetErrorDlg(NULL, req, 0xdeadbeef, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
    ok(res == ERROR_SUCCESS, "Got %ld, expected ERROR_SUCCESS\n", res);

    res = InternetErrorDlg(NULL, NULL, 0xdeadbeef, FLAGS_ERROR_UI_FILTER_FOR_ERRORS | FLAGS_ERROR_UI_FLAGS_NO_UI, NULL);
    ok(res == ERROR_SUCCESS, "Got %ld, expected ERROR_SUCCESS\n", res);

    res = InternetCloseHandle(req);
    ok(res == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
    res = InternetCloseHandle(con);
    ok(res == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
    res = InternetCloseHandle(ses);
    ok(res == TRUE, "InternetCloseHandle failed: 0x%08lx\n", GetLastError());
}

static void test_InternetGetConnectedStateExA(void)
{
    BOOL res;
    CHAR buffer[256];
    DWORD flags, sz;

    if(!pInternetGetConnectedStateExA) {
        win_skip("InternetGetConnectedStateExA is not supported\n");
        return;
    }

    flags = 0;
    buffer[0] = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, sizeof(buffer), 0);
    trace("Internet Connection: Flags 0x%02lx - Name '%s'\n", flags, buffer);
    ok (flags & INTERNET_RAS_INSTALLED, "Missing RAS flag\n");
    if(!res) {
        win_skip("InternetGetConnectedStateExA tests require a valid connection\n");
        return;
    }

    res = pInternetGetConnectedStateExA(NULL, NULL, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, NULL, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    if (flags & INTERNET_CONNECTION_CONFIGURED)
    {
        ok(flags & INTERNET_CONNECTION_MODEM, "Modem connection flag missing\n");
        ok(flags & ~INTERNET_CONNECTION_LAN, "Mixed Modem and LAN flags\n");
    }
    else
    {
        ok(flags & INTERNET_CONNECTION_LAN, "LAN connection flag missing\n");
        ok(flags & ~INTERNET_CONNECTION_MODEM, "Mixed Modem and LAN flags\n");
    }

    buffer[0] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(!buffer[0], "Buffer must not change, got %02X\n", buffer[0]);

    buffer[0] = 0;
    res = pInternetGetConnectedStateExA(NULL, buffer, sizeof(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(buffer[0], "Expected a connection name\n");

    buffer[0] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, sizeof(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(buffer[0], "Expected a connection name\n");
    sz = strlen(buffer);

    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, NULL, sizeof(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");

    /* no space for complete string this time, double-byte characters get truncated */
    memset(buffer, 'z', sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, sz, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(sz - 1 == strlen(buffer), "Expected len %lu, got %Iu: %s\n", sz - 1, strlen(buffer), wine_dbgstr_a(buffer));

    memset(buffer, 'z', sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, sz / 2, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(sz / 2 - 1 == strlen(buffer), "Expected len %lu, got %Iu: %s\n", sz / 2 - 1, strlen(buffer), wine_dbgstr_a(buffer));

    memset(buffer, 'z', sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, 2, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(strlen(buffer) == 1, "Expected len 1, got %Iu: %s\n", strlen(buffer), wine_dbgstr_a(buffer));

    memset(buffer, 'z', sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExA(&flags, buffer, 1, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(!buffer[0], "Expected len 0, got %Iu: %s\n", strlen(buffer), wine_dbgstr_a(buffer));
}

static void test_InternetGetConnectedStateExW(void)
{
    BOOL res;
    WCHAR buffer[256];
    DWORD flags, sz;

    if(!pInternetGetConnectedStateExW) {
        win_skip("InternetGetConnectedStateExW is not supported\n");
        return;
    }

    flags = 0;
    wcscpy(buffer, L"wine");
    SetLastError(0xdeadbeef);
    res = pInternetGetConnectedStateExW(&flags, buffer, ARRAY_SIZE(buffer), 0);
    trace("Internet Connection: Flags 0x%02lx - Name %s\n", flags, wine_dbgstr_w(buffer));
    ok (flags & INTERNET_RAS_INSTALLED, "Missing RAS flag\n");
    if(!res) {
        DWORD error = GetLastError();
        ok(error == ERROR_SUCCESS, "Last error = %#lx\n", error);
        ok(!buffer[0], "Expected empty connection name, got %s\n", wine_dbgstr_w(buffer));

        skip("InternetGetConnectedStateExW tests require a valid connection\n");
        return;
    }

    res = pInternetGetConnectedStateExW(NULL, NULL, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);

    SetLastError(0xdeadbeef);
    res = pInternetGetConnectedStateExW(NULL, NULL, 0, 1);
    ok(res == FALSE, "Expected TRUE, got %d\n", res);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Unexpected gle %lu\n", GetLastError());

    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, NULL, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    if (flags & INTERNET_CONNECTION_CONFIGURED)
    {
        ok(flags & INTERNET_CONNECTION_MODEM, "Modem connection flag missing\n");
        ok(flags & ~INTERNET_CONNECTION_LAN, "Mixed Modem and LAN flags\n");
    }
    else
    {
        ok(flags & INTERNET_CONNECTION_LAN, "LAN connection flag missing\n");
        ok(flags & ~INTERNET_CONNECTION_MODEM, "Mixed Modem and LAN flags\n");
    }

    buffer[0] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, 0, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(!buffer[0], "Buffer must not change, got %02X\n", buffer[0]);

    buffer[0] = 0;
    res = pInternetGetConnectedStateExW(NULL, buffer, ARRAY_SIZE(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(buffer[0], "Expected a connection name\n");

    buffer[0] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, ARRAY_SIZE(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(buffer[0], "Expected a connection name\n");
    sz = lstrlenW(buffer);

    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, NULL, ARRAY_SIZE(buffer), 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");

    /* no space for complete string this time */
    wmemset(buffer, 'z', ARRAY_SIZE(buffer) - 1);
    buffer[ARRAY_SIZE(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, sz, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    if (flags & INTERNET_CONNECTION_MODEM)
        ok(!buffer[0], "Expected len 0, got %u: %s\n", lstrlenW(buffer), wine_dbgstr_w(buffer));
    else
        ok(sz - 1 == lstrlenW(buffer), "Expected len %lu, got %u: %s\n", sz - 1, lstrlenW(buffer), wine_dbgstr_w(buffer));

    wmemset(buffer, 'z', ARRAY_SIZE(buffer) - 1);
    buffer[ARRAY_SIZE(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, sz / 2, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    if (flags & INTERNET_CONNECTION_MODEM)
        ok(!buffer[0], "Expected len 0, got %u: %s\n", lstrlenW(buffer), wine_dbgstr_w(buffer));
    else
        ok(sz / 2 - 1 == lstrlenW(buffer), "Expected len %lu, got %u: %s\n", sz / 2 - 1, lstrlenW(buffer), wine_dbgstr_w(buffer));

    wmemset(buffer, 'z', ARRAY_SIZE(buffer) - 1);
    buffer[ARRAY_SIZE(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, 2, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    if (flags & INTERNET_CONNECTION_MODEM)
        ok(!buffer[0], "Expected len 0, got %u: %s\n", lstrlenW(buffer), wine_dbgstr_w(buffer));
    else
        ok(lstrlenW(buffer) == 1, "Expected len 1, got %u: %s\n", lstrlenW(buffer), wine_dbgstr_w(buffer));

    wmemset(buffer, 'z', ARRAY_SIZE(buffer) - 1);
    buffer[ARRAY_SIZE(buffer) - 1] = 0;
    flags = 0;
    res = pInternetGetConnectedStateExW(&flags, buffer, 1, 0);
    ok(res == TRUE, "Expected TRUE, got %d\n", res);
    ok(flags, "Expected at least one flag set\n");
    ok(!buffer[0], "Expected len 0, got %u: %s\n", lstrlenW(buffer), wine_dbgstr_w(buffer));
}

static void test_format_message(HMODULE hdll)
{
    DWORD ret;
    CHAR out[0x100];

    /* These messages come from wininet and not the system. */
    ret = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM , NULL, ERROR_INTERNET_TIMEOUT,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret == 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_TIMEOUT,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_INTERNAL_ERROR,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_INVALID_URL,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_UNRECOGNIZED_SCHEME,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_NAME_NOT_RESOLVED,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_INVALID_OPERATION,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0 || broken(!ret) /* XP, w2k3 */, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_OPERATION_CANCELLED,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_ITEM_NOT_FOUND,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_CANNOT_CONNECT,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_CONNECTION_ABORTED,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_SEC_CERT_DATE_INVALID,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);

    ret = FormatMessageA(FORMAT_MESSAGE_FROM_HMODULE, hdll, ERROR_INTERNET_SEC_CERT_CN_INVALID,
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), out, sizeof(out), NULL);
    ok(ret != 0, "FormatMessageA returned %ld\n", ret);
}

/* ############################### */

START_TEST(internet)
{
    HMODULE hdll;
    hdll = GetModuleHandleA("wininet.dll");

    pCreateUrlCacheContainerA = (void*)GetProcAddress(hdll, "CreateUrlCacheContainerA");
    pCreateUrlCacheContainerW = (void*)GetProcAddress(hdll, "CreateUrlCacheContainerW");
    pInternetTimeFromSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeA");
    pInternetTimeFromSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeFromSystemTimeW");
    pInternetTimeToSystemTimeA = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeA");
    pInternetTimeToSystemTimeW = (void*)GetProcAddress(hdll, "InternetTimeToSystemTimeW");
    pIsDomainLegalCookieDomainW = (void*)GetProcAddress(hdll, (LPCSTR)117);
    pPrivacyGetZonePreferenceW = (void*)GetProcAddress(hdll, "PrivacyGetZonePreferenceW");
    pPrivacySetZonePreferenceW = (void*)GetProcAddress(hdll, "PrivacySetZonePreferenceW");
    pInternetGetCookieExA = (void*)GetProcAddress(hdll, "InternetGetCookieExA");
    pInternetGetCookieExW = (void*)GetProcAddress(hdll, "InternetGetCookieExW");
    pInternetGetConnectedStateExA = (void*)GetProcAddress(hdll, "InternetGetConnectedStateExA");
    pInternetGetConnectedStateExW = (void*)GetProcAddress(hdll, "InternetGetConnectedStateExW");

    if(!pInternetGetCookieExW) {
        win_skip("Too old IE (older than 6.0)\n");
        return;
    }

    test_InternetCanonicalizeUrlA();
    test_InternetQueryOptionA();
    test_InternetGetConnectedStateExA();
    test_InternetGetConnectedStateExW();
    test_get_cookie();
    test_complicated_cookie();
    test_cookie_url();
    test_cookie_attrs();
    test_version();
    test_null();
    test_Option_PerConnectionOption();
    test_Option_PerConnectionOptionA();
    test_InternetErrorDlg();
    test_max_conns();

    if (!pInternetTimeFromSystemTimeA)
        win_skip("skipping the InternetTime tests\n");
    else
    {
        InternetTimeFromSystemTimeA_test();
        InternetTimeFromSystemTimeW_test();
        test_InternetTimeToSystemTime();
    }
    if (pIsDomainLegalCookieDomainW &&
        ((void*)pIsDomainLegalCookieDomainW == (void*)pCreateUrlCacheContainerA ||
         (void*)pIsDomainLegalCookieDomainW == (void*)pCreateUrlCacheContainerW))
        win_skip("IsDomainLegalCookieDomainW is not available on systems with IE5\n");
    else if (!pIsDomainLegalCookieDomainW)
        win_skip("IsDomainLegalCookieDomainW (or ordinal 117) is not available\n");
    else
        test_IsDomainLegalCookieDomainW();

    if (pPrivacyGetZonePreferenceW && pPrivacySetZonePreferenceW)
        test_PrivacyGetSetZonePreferenceW();
    else
        win_skip("Privacy[SG]etZonePreferenceW are not available\n");

    test_InternetSetOption();
    test_end_browser_session();
    test_format_message(hdll);
}
