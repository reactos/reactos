#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wininet.h"

#include "wine/test.h"

#define TEST_URL "http://www.winehq.org/site/about"
#define TEST_URL_PATH "/site/about"
#define TEST_URL2 "http://www.myserver.com/myscript.php?arg=1"
#define TEST_URL2_SERVER "www.myserver.com"
#define TEST_URL2_PATH "/myscript.php"
#define TEST_URL2_PATHEXTRA "/myscript.php?arg=1"
#define TEST_URL2_EXTRA "?arg=1"

int goon = 0;

VOID WINAPI callback(
     HINTERNET hInternet,
     DWORD dwContext,
     DWORD dwInternetStatus,
     LPVOID lpvStatusInformation,
     DWORD dwStatusInformationLength
)
{
    char name[124];

    switch (dwInternetStatus)
    {
        case INTERNET_STATUS_RESOLVING_NAME:
            strcpy(name,"INTERNET_STATUS_RESOLVING_NAME");
            break;
        case INTERNET_STATUS_NAME_RESOLVED:
            strcpy(name,"INTERNET_STATUS_NAME_RESOLVED");
            break;
        case INTERNET_STATUS_CONNECTING_TO_SERVER:
            strcpy(name,"INTERNET_STATUS_CONNECTING_TO_SERVER");
            break;
        case INTERNET_STATUS_CONNECTED_TO_SERVER:
            strcpy(name,"INTERNET_STATUS_CONNECTED_TO_SERVER");
            break;
        case INTERNET_STATUS_SENDING_REQUEST:
            strcpy(name,"INTERNET_STATUS_SENDING_REQUEST");
            break;
        case INTERNET_STATUS_REQUEST_SENT:
            strcpy(name,"INTERNET_STATUS_REQUEST_SENT");
            break;
        case INTERNET_STATUS_RECEIVING_RESPONSE:
            strcpy(name,"INTERNET_STATUS_RECEIVING_RESPONSE");
            break;
        case INTERNET_STATUS_RESPONSE_RECEIVED:
            strcpy(name,"INTERNET_STATUS_RESPONSE_RECEIVED");
            break;
        case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
            strcpy(name,"INTERNET_STATUS_CTL_RESPONSE_RECEIVED");
            break;
        case INTERNET_STATUS_PREFETCH:
            strcpy(name,"INTERNET_STATUS_PREFETCH");
            break;
        case INTERNET_STATUS_CLOSING_CONNECTION:
            strcpy(name,"INTERNET_STATUS_CLOSING_CONNECTION");
            break;
        case INTERNET_STATUS_CONNECTION_CLOSED:
            strcpy(name,"INTERNET_STATUS_CONNECTION_CLOSED");
            break;
        case INTERNET_STATUS_HANDLE_CREATED:
            strcpy(name,"INTERNET_STATUS_HANDLE_CREATED");
            break;
        case INTERNET_STATUS_HANDLE_CLOSING:
            strcpy(name,"INTERNET_STATUS_HANDLE_CLOSING");
            break;
        case INTERNET_STATUS_REQUEST_COMPLETE:
            strcpy(name,"INTERNET_STATUS_REQUEST_COMPLETE");
            goon = 1;
            break;
        case INTERNET_STATUS_REDIRECT:
            strcpy(name,"INTERNET_STATUS_REDIRECT");
            break;
        case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
            strcpy(name,"INTERNET_STATUS_INTERMEDIATE_RESPONSE");
            break;
    }

    trace("Callback %p 0x%lx %s(%li) %p %ld\n",hInternet,dwContext,name,dwInternetStatus,lpvStatusInformation,dwStatusInformationLength);
}

void winapi_test(int flags)
{
    DWORD rc;
    CHAR buffer[4000];
    DWORD length;
    DWORD out;
    const char *types[2] = { "*", NULL };
    HINTERNET hi, hic = 0, hor = 0;

    trace("Starting with flags 0x%x\n",flags);

    trace("InternetOpenA <--\n");
    hi = InternetOpenA("",0x0,0x0,0x0,flags);
    ok((hi != 0x0),"InternetOpen Failed\n");
    trace("InternetOpenA -->\n");

    if (hi == 0x0) goto abort;

    InternetSetStatusCallback(hi,&callback);

    trace("InternetConnectA <--\n");
    hic=InternetConnectA(hi,"www.winehq.org",0x0,0x0,0x0,0x3,0x0,0xdeadbeef);
    ok((hic != 0x0),"InternetConnect Failed\n");
    trace("InternetConnectA -->\n");

    if (hic == 0x0) goto abort;

    trace("HttpOpenRequestA <--\n");
    hor = HttpOpenRequestA(hic, "GET",
                          "/about/",
                          0x0,0x0,types,0x00400800,0xdeadbead);
    if (hor == 0x0 && GetLastError() == 12007 /* ERROR_INTERNET_NAME_NOT_RESOLVED */) {
        /*
         * If the internet name can't be resolved we are probably behind
         * a firewall or in some other way not directly connected to the
         * Internet. Not enough reason to fail the test. Just ignore and
         * abort.
         */
    } else  {
        ok((hor != 0x0),"HttpOpenRequest Failed\n");
    }
    trace("HttpOpenRequestA -->\n");

    if (hor == 0x0) goto abort;

    trace("HttpSendRequestA -->\n");
    SetLastError(0);
    rc = HttpSendRequestA(hor, "", 0xffffffff,0x0,0x0);
    if (flags)
        ok(((rc == 0)&&(GetLastError()==997)),
            "Asynchronous HttpSendRequest NOT returning 0 with error 997\n");
    else
        ok((rc != 0) || GetLastError() == 12007, /* 12007 == XP */
           "Synchronous HttpSendRequest returning 0, error %ld\n", GetLastError());
    trace("HttpSendRequestA <--\n");

    while ((flags)&&(!goon))
        Sleep(100);

    length = 4;
    rc = InternetQueryOptionA(hor,0x17,&out,&length);
    trace("Option 0x17 -> %li  %li\n",rc,out);

    length = 100;
    rc = InternetQueryOptionA(hor,0x22,buffer,&length);
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = HttpQueryInfoA(hor,0x16,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x16 -> %li  %s\n",rc,buffer);

    length = 4000;
    rc = InternetQueryOptionA(hor,0x22,buffer,&length);
    buffer[length]=0;
    trace("Option 0x22 -> %li  %s\n",rc,buffer);

    length = 16;
    rc = HttpQueryInfoA(hor,0x5,&buffer,&length,0x0);
    trace("Option 0x5 -> %li  %s  (%li)\n",rc,buffer,GetLastError());

    length = 100;
    rc = HttpQueryInfoA(hor,0x1,buffer,&length,0x0);
    buffer[length]=0;
    trace("Option 0x1 -> %li  %s\n",rc,buffer);

    length = 100;
    trace("Entering Query loop\n");

    while (length)
    {

        rc = InternetQueryDataAvailable(hor,&length,0x0,0x0);
        ok(!(rc == 0 && length != 0),"InternetQueryDataAvailable failed\n");

        if (length)
        {
            char *buffer;
            buffer = HeapAlloc(GetProcessHeap(),0,length+1);

            rc = InternetReadFile(hor,buffer,length,&length);

            buffer[length]=0;

            trace("ReadFile -> %li %li\n",rc,length);

            HeapFree(GetProcessHeap(),0,buffer);
        }
    }
abort:
    if (hor != 0x0) {
        rc = InternetCloseHandle(hor);
        ok ((rc != 0), "InternetCloseHandle of handle opened by HttpOpenRequestA failed\n");
        rc = InternetCloseHandle(hor);
        ok ((rc == 0), "Double close of handle opened by HttpOpenRequestA succeeded\n");
    }
    if (hic != 0x0) {
        rc = InternetCloseHandle(hic);
        ok ((rc != 0), "InternetCloseHandle of handle opened by InternetConnectA failed\n");
    }
    if (hi != 0x0) {
      rc = InternetCloseHandle(hi);
      ok ((rc != 0), "InternetCloseHandle of handle opened by InternetOpenA failed\n");
      if (flags)
          Sleep(100);
    }
}

void InternetOpenUrlA_test(void)
{
  HINTERNET myhinternet, myhttp;
  char buffer[0x400];
  URL_COMPONENTSA urlComponents;
  char protocol[32], hostName[1024], userName[1024];
  char password[1024], extra[1024], path[1024];
  DWORD size, readbytes, totalbytes=0;
  BOOL ret;
  
  myhinternet = InternetOpen("Winetest",0,NULL,NULL,INTERNET_FLAG_NO_CACHE_WRITE);
  ok((myhinternet != 0), "InternetOpen failed, error %lx\n",GetLastError());
  size = 0x400;
  ret = InternetCanonicalizeUrl(TEST_URL, buffer, &size,ICU_BROWSER_MODE);
  ok( ret, "InternetCanonicalizeUrl failed, error %lx\n",GetLastError());
  
  urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
  urlComponents.lpszScheme = protocol;
  urlComponents.dwSchemeLength = 32;
  urlComponents.lpszHostName = hostName;
  urlComponents.dwHostNameLength = 1024;
  urlComponents.lpszUserName = userName;
  urlComponents.dwUserNameLength = 1024;
  urlComponents.lpszPassword = password;
  urlComponents.dwPasswordLength = 1024;
  urlComponents.lpszUrlPath = path;
  urlComponents.dwUrlPathLength = 2048;
  urlComponents.lpszExtraInfo = extra;
  urlComponents.dwExtraInfoLength = 1024;
  ret = InternetCrackUrl(TEST_URL, 0,0,&urlComponents);
  ok( ret, "InternetCrackUrl failed, error %lx\n",GetLastError());
  SetLastError(0);
  myhttp = InternetOpenUrl(myhinternet, TEST_URL, 0, 0,
			   INTERNET_FLAG_RELOAD|INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_TRANSFER_BINARY,0);
  if (GetLastError() == 12007)
    return; /* WinXP returns this when not connected to the net */
  ok((myhttp != 0),"InternetOpenUrl failed, error %lx\n",GetLastError());
  ret = InternetReadFile(myhttp, buffer,0x400,&readbytes);
  ok( ret, "InternetReadFile failed, error %lx\n",GetLastError());
  totalbytes += readbytes;
  while (readbytes && InternetReadFile(myhttp, buffer,0x400,&readbytes))
    totalbytes += readbytes;
  trace("read 0x%08lx bytes\n",totalbytes);
}
  
void InternetCrackUrl_test(void)
{
  URL_COMPONENTSA urlComponents;
  char protocol[32], hostName[1024], userName[1024];
  char password[1024], extra[1024], path[1024];
  BOOL ret;

  urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);
  urlComponents.lpszScheme = protocol;
  urlComponents.dwSchemeLength = 32;
  urlComponents.lpszHostName = hostName;
  urlComponents.dwHostNameLength = 1024;
  urlComponents.lpszUserName = userName;
  urlComponents.dwUserNameLength = 1024;
  urlComponents.lpszPassword = password;
  urlComponents.dwPasswordLength = 1024;
  urlComponents.lpszUrlPath = path;
  urlComponents.dwUrlPathLength = 2048;
  urlComponents.lpszExtraInfo = extra;
  urlComponents.dwExtraInfoLength = 1024;
  ret = InternetCrackUrl(TEST_URL, 0,0,&urlComponents);
  ok( ret, "InternetCrackUrl failed, error %lx\n",GetLastError());
  ok((strcmp(TEST_URL_PATH,path) == 0),"path cracked wrong\n");

  /* Bug 1805: Confirm the returned lengths are correct:                     */
  /* 1. When extra info split out explicitly */
  ZeroMemory(&urlComponents, sizeof(urlComponents));
  urlComponents.dwStructSize = sizeof(urlComponents);
  urlComponents.dwHostNameLength = 1;
  urlComponents.dwUrlPathLength  = 1;
  urlComponents.dwExtraInfoLength = 1;
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed, error 0x%lx\n", GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATH),".dwUrlPathLength should be %d, but is %ld\n", strlen(TEST_URL2_PATH), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATH,strlen(TEST_URL2_PATH)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATH, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %d, but is %ld\n", strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);
  ok(urlComponents.dwExtraInfoLength == strlen(TEST_URL2_EXTRA),".dwExtraInfoLength should be %d, but is %ld\n", strlen(TEST_URL2_EXTRA), urlComponents.dwExtraInfoLength);
  ok(!strncmp(urlComponents.lpszExtraInfo,TEST_URL2_EXTRA,strlen(TEST_URL2_EXTRA)),"lpszExtraInfo should be %s but is %s\n", TEST_URL2_EXTRA, urlComponents.lpszHostName);

  /* 2. When extra info is not split out explicitly and is in url path */
  ZeroMemory(&urlComponents, sizeof(urlComponents));
  urlComponents.dwStructSize = sizeof(urlComponents);
  urlComponents.dwHostNameLength = 1;
  urlComponents.dwUrlPathLength  = 1;
  ok(InternetCrackUrlA(TEST_URL2, 0, 0, &urlComponents),"InternetCrackUrl failed with GLE 0x%lx\n",GetLastError());
  ok(urlComponents.dwUrlPathLength == strlen(TEST_URL2_PATHEXTRA),".dwUrlPathLength should be %d, but is %ld\n", strlen(TEST_URL2_PATHEXTRA), urlComponents.dwUrlPathLength);
  ok(!strncmp(urlComponents.lpszUrlPath,TEST_URL2_PATHEXTRA,strlen(TEST_URL2_PATHEXTRA)),"lpszUrlPath should be %s but is %s\n", TEST_URL2_PATHEXTRA, urlComponents.lpszUrlPath);
  ok(urlComponents.dwHostNameLength == strlen(TEST_URL2_SERVER),".dwHostNameLength should be %d, but is %ld\n", strlen(TEST_URL2_SERVER), urlComponents.dwHostNameLength);
  ok(!strncmp(urlComponents.lpszHostName,TEST_URL2_SERVER,strlen(TEST_URL2_SERVER)),"lpszHostName should be %s but is %s\n", TEST_URL2_SERVER, urlComponents.lpszHostName);



}

void InternetCrackUrlW_test(void)
{
    WCHAR url[] = {
        'h','t','t','p',':','/','/','1','9','2','.','1','6','8','.','0','.','2','2','/',
        'C','F','I','D','E','/','m','a','i','n','.','c','f','m','?','C','F','S','V','R',
        '=','I','D','E','&','A','C','T','I','O','N','=','I','D','E','_','D','E','F','A',
        'U','L','T', 0 };
    URL_COMPONENTSW comp;
    WCHAR scheme[20], host[20], user[20], pwd[20], urlpart[50], extra[50];
    BOOL r;

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszScheme = scheme;
    comp.dwSchemeLength = sizeof scheme;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUserName = user;
    comp.dwUserNameLength = sizeof user;
    comp.lpszPassword = pwd;
    comp.dwPasswordLength = sizeof pwd;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = extra;
    comp.dwExtraInfoLength = sizeof extra;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 4, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");
 
    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 44, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 0, "extra length wrong\n");

    urlpart[0]=0;
    scheme[0]=0;
    extra[0]=0;
    host[0]=0;
    user[0]=0;
    pwd[0]=0;
    memset(&comp, 0, sizeof comp);
    comp.dwStructSize = sizeof comp;
    comp.lpszHostName = host;
    comp.dwHostNameLength = sizeof host;
    comp.lpszUrlPath = urlpart;
    comp.dwUrlPathLength = sizeof urlpart;
    comp.lpszExtraInfo = NULL;
    comp.dwExtraInfoLength = sizeof extra;

    r = InternetCrackUrlW(url, 0, 0, &comp );
    ok( r, "failed to crack url\n");
    ok( comp.dwSchemeLength == 0, "scheme length wrong\n");
    ok( comp.dwHostNameLength == 12, "host length wrong\n");
    ok( comp.dwUserNameLength == 0, "user length wrong\n");
    ok( comp.dwPasswordLength == 0, "password length wrong\n");
    ok( comp.dwUrlPathLength == 15, "url length wrong\n");
    ok( comp.dwExtraInfoLength == 29, "extra length wrong\n");
}

static void InternetTimeFromSystemTimeA_test()
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    char string[INTERNET_RFC1123_BUFSIZE];
    static const char expect[] = "Fri, 07 Jan 2005 12:06:35 GMT";

    ret = InternetTimeFromSystemTimeA( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeA failed (%ld)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeA failed (%ld)\n", GetLastError() );
}

static void InternetTimeFromSystemTimeW_test()
{
    BOOL ret;
    static const SYSTEMTIME time = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    WCHAR string[INTERNET_RFC1123_BUFSIZE + 1];
    static const WCHAR expect[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };

    ret = InternetTimeFromSystemTimeW( &time, INTERNET_RFC1123_FORMAT, string, sizeof(string) );
    ok( ret, "InternetTimeFromSystemTimeW failed (%ld)\n", GetLastError() );

    ok( !memcmp( string, expect, sizeof(expect) ),
        "InternetTimeFromSystemTimeW failed (%ld)\n", GetLastError() );
}

static void InternetTimeToSystemTimeA_test()
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const char string[] = "Fri, 07 Jan 2005 12:06:35 GMT";
    static const char string2[] = " fri 7 jan 2005 12 06 35";

    ret = InternetTimeToSystemTimeA( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeA( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeA failed (%ld)\n", GetLastError() );
}

static void InternetTimeToSystemTimeW_test()
{
    BOOL ret;
    SYSTEMTIME time;
    static const SYSTEMTIME expect = { 2005, 1, 5, 7, 12, 6, 35, 0 };
    static const WCHAR string[] = { 'F','r','i',',',' ','0','7',' ','J','a','n',' ','2','0','0','5',' ',
                                    '1','2',':','0','6',':','3','5',' ','G','M','T',0 };
    static const WCHAR string2[] = { ' ','f','r','i',' ','7',' ','j','a','n',' ','2','0','0','5',' ',
                                     '1','2',' ','0','6',' ','3','5',0 };
    static const WCHAR string3[] = { 'F','r',0 };

    ret = InternetTimeToSystemTimeW( NULL, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( NULL, &time, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, NULL, 0 );
    ok( !ret, "InternetTimeToSystemTimeW succeeded (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, &time, 1 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string2, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
    ok( !memcmp( &time, &expect, sizeof(expect) ),
        "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );

    ret = InternetTimeToSystemTimeW( string3, &time, 0 );
    ok( ret, "InternetTimeToSystemTimeW failed (%ld)\n", GetLastError() );
}

START_TEST(http)
{
    winapi_test(0x10000000);
    winapi_test(0x00000000);
    InternetCrackUrl_test();
    InternetOpenUrlA_test();
    InternetCrackUrlW_test();
    InternetTimeFromSystemTimeA_test();
    InternetTimeFromSystemTimeW_test();
    InternetTimeToSystemTimeA_test();
    InternetTimeToSystemTimeW_test();
}
