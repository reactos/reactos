/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    tproxy.c

Abstract:

    Test program for Wininet proxy settings

    Contents:

Author:

    Richard L Firth (rfirth) 23-Jul-1996

Revision History:

    23-Jul-1996 rfirth
        Created

--*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wininet.h>
#include <catlib.h>
#include <malloc.h>
#include <memory.h>

#ifndef _CRTAPI1
#define _CRTAPI1
#endif

#define IS_ARG(c)   (((c) == '-') || ((c) == '/'))

#define NEW_USER_AGENT  "and now for a completely different user-agent"

void _CRTAPI1 main(int, char**);
void usage(void);
void get_proxy_info(HINTERNET);
void set_proxy_info(HINTERNET, LPINTERNET_PROXY_INFO);
void dump_proxy_info(HINTERNET, LPINTERNET_PROXY_INFO);
void get_user_agent(HINTERNET, char*, LPDWORD);
void set_user_agent(HINTERNET, char*);
void refresh_handle(HINTERNET);

BOOL Verbose = FALSE;
DWORD Failures = 0;

void _CRTAPI1 main(int argc, char** argv) {

    HINTERNET hInternet1;
    HINTERNET hInternet2;
    HINTERNET hInternet3;
    HINTERNET hInternet4;
    INTERNET_PROXY_INFO proxyInfo;
    char proxyBuffer[512];
    DWORD length;
    BOOL ok;
    char uaBuf1[128];
    char uaBuf2[128];
    DWORD uaLen1;
    DWORD uaLen2;

    for (--argc, ++argv; argc; --argc, ++argv) {
        if (IS_ARG(**argv)) {
            switch (*++*argv) {
            case 'v':
                Verbose = TRUE;
                break;

            default:
                printf("error: unrecognized command line flag: '%c'\n", **argv);
                usage();
                break;
            }
        } else {
            printf("error: unrecognized command line argument: \"%s\"\n", *argv);
            usage();
        }
    }

    //
    // get global proxy info
    //

    get_proxy_info(NULL);

    //
    // get & remember it
    //

    length = sizeof(proxyBuffer);
    ok = InternetQueryOption(NULL, INTERNET_OPTION_PROXY, (LPVOID)proxyBuffer, &length);
    if (!ok) {
        print_error("tproxy()", "InternetQueryOption()");
        ++Failures;
    }

    //
    // create handles
    //

    //
    // 1. preconfig
    //

    hInternet1 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_PRECONFIG, "foo", "bar", 0);
    if (hInternet1 == NULL) {
        print_error("tproxy()", "InternetOpen(PRECONFIG)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(PRECONFIG) returns %#x\n", hInternet1);
    }
    get_proxy_info(hInternet1);

    //
    // 2. direct
    //

    hInternet2 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_DIRECT, "foo", "bar", 0);
    if (hInternet1 == NULL) {
        print_error("tproxy()", "InternetOpen(DIRECT)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(DIRECT) returns %#x\n", hInternet2);
    }
    get_proxy_info(hInternet2);

    //
    // 3. private proxy
    //

    hInternet3 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_PROXY, "foo", "bar", 0);
    if (hInternet1 == NULL) {
        print_error("tproxy()", "InternetOpen(PROXY)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(PROXY) returns %#x\n", hInternet3);
    }
    get_proxy_info(hInternet3);

    //
    // 4. another preconfig
    //

    hInternet4 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet1 == NULL) {
        print_error("tproxy()", "InternetOpen(PRECONFIG)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(PRECONFIG #2) returns %#x\n", hInternet4);
    }
    get_proxy_info(hInternet4);

    //
    // change global proxy
    //

    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = "modified.global.proxy";
    proxyInfo.lpszProxyBypass = "modified.global.proxy.bypass.list, *";
    set_proxy_info(NULL, &proxyInfo);

    //
    // make sure global, hInternet1 and hInternet4 all reference same proxy info
    //

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet4);

    //
    // reload global proxy info from registry
    //

    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
    proxyInfo.lpszProxy = "modified.global.proxy";
    proxyInfo.lpszProxyBypass = "modified.global.proxy.bypass.list, *";
    set_proxy_info(NULL, &proxyInfo);

    //
    // set hInternet2 to use private proxy
    //

    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = "my.test.proxy";
    proxyInfo.lpszProxyBypass = "www.foo.com www.bar.com";
    set_proxy_info(hInternet2, &proxyInfo);
    get_proxy_info(hInternet2);

    //
    // set hInternet3 to use direct
    //

    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
    proxyInfo.lpszProxy = "this.is.a.bogus.proxy";
    proxyInfo.lpszProxyBypass = "this.is.a.bogus.bypass.entry";
    set_proxy_info(hInternet3, &proxyInfo);
    get_proxy_info(hInternet3);

    //
    //
    //

    //
    // get the user-agent
    //

    uaLen1 = sizeof(uaBuf1);
    get_user_agent(hInternet1, uaBuf1, &uaLen1);

    //
    // set the user-agent
    //

    set_user_agent(hInternet1, NEW_USER_AGENT);

    //
    // get it again to make sure its the correct value
    //

    uaLen2 = sizeof(uaBuf2);
    get_user_agent(hInternet1, uaBuf2, &uaLen2);

    //
    // compare 'em
    //

    if (strcmp(uaBuf2, NEW_USER_AGENT)) {
        printf("error: tproxy(): set_user_agent() failed\n");
    }

    //
    // reset global proxy info
    //

    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
    proxyInfo.lpszProxy = NULL;
    proxyInfo.lpszProxyBypass = NULL;
    set_proxy_info(NULL, &proxyInfo);

    //
    // make sure global, hInternet1 and hInternet4 all reference same proxy info
    //

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet4);

    //
    // close all handles
    //

    InternetCloseHandle(hInternet1);
    InternetCloseHandle(hInternet2);
    InternetCloseHandle(hInternet3);
    InternetCloseHandle(hInternet4);

    //
    // do the AOL test
    //

    if (Verbose) {
        printf("\nThe AOL Test\n\n");
    }

    if (Verbose) {
        printf("Opening PRECONFIG Internet handle #1\n");
    }
    hInternet1 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet1 == NULL) {
        print_error("tproxy()", "InternetOpen(PRECONFIG)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(PRECONFIG) returns %#x\n", hInternet1);
    }
    get_proxy_info(hInternet1);

    if (Verbose) {
        printf("Changing global proxy info\n");
    }
    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = "my.test.proxy";
    proxyInfo.lpszProxyBypass = "www.foo.com www.bar.com";
    set_proxy_info(NULL, &proxyInfo);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);

    if (Verbose) {
        printf("Opening PRECONFIG Internet handle #2\n");
    }
    hInternet2 = InternetOpen("tproxy", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hInternet2 == NULL) {
        print_error("tproxy()", "InternetOpen(PRECONFIG)");
        ++Failures;
    } else if (Verbose) {
        printf("InternetOpen(PRECONFIG) returns %#x\n", hInternet2);
    }
    get_proxy_info(hInternet2);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet2);

    if (Verbose) {
        printf("Changing proxy info on Internet handle #2\n");
    }
    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PROXY;
    proxyInfo.lpszProxy = "THIS.IS.A.BOGUS.SERVER.LIST";
    proxyInfo.lpszProxyBypass = "THIS.IS.A.BOGUS.BYPASS.LIST";
    set_proxy_info(hInternet2, &proxyInfo);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet2);

    //
    // refresh handle 2 - shouldn't change
    //

    if (Verbose) {
        printf("Refreshing Internet handle #2\n");
    }
    refresh_handle(hInternet2);

    //
    // refresh the global handle - should refresh global handle & handle 1
    //

    if (Verbose) {
        printf("Refreshing global handle\n");
    }
    refresh_handle(NULL);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet2);

    //
    // change global proxy info back to registry. Global handle & handle 1
    // proxy info should change
    //

    if (Verbose) {
        printf("Changing global proxy info back to preconfig\n");
    }
    proxyInfo.dwAccessType = INTERNET_OPEN_TYPE_PRECONFIG;
    proxyInfo.lpszProxy = NULL;
    proxyInfo.lpszProxyBypass = NULL;
    set_proxy_info(NULL, &proxyInfo);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet2);

    //
    // refresh the global handle - should refresh global handle & handle 1
    //

    if (Verbose) {
        printf("Refreshing global handle\n");
    }
    refresh_handle(NULL);

    get_proxy_info(NULL);
    get_proxy_info(hInternet1);
    get_proxy_info(hInternet2);

    //
    // close all handles
    //

    InternetCloseHandle(hInternet1);
    InternetCloseHandle(hInternet2);

    //
    // pass or fail?
    //

    if (Verbose) {
        printf("\nDone.\n");
        if (Failures) {
            printf("Test failed\n");
        } else {
            printf("Test passed\n");
        }
    }
    exit(Failures);
}

void usage() {
    printf("usage: tproxy [-v]\n"
           "where: -v = Verbose mode\n"
           );
    exit(1);
}

void get_proxy_info(HINTERNET hInternet) {

    DWORD length;
    BOOL ok;

    length = 0;
    ok = InternetQueryOption(hInternet, INTERNET_OPTION_PROXY, NULL, &length);
    if (ok) {
        printf("error: get_proxy_info(%#x): InternetQueryOption() w/ no buffer succeeds\n", hInternet);
        ++Failures;
    } else {

        LPVOID buf = malloc(length);

        if (Verbose) {
            printf("get_proxy_info(%#x): %d bytes required for proxy info buffer\n", hInternet, length);
        }
        memset(buf, 0x99, length);
        ok = InternetQueryOption(hInternet, INTERNET_OPTION_PROXY, buf, &length);
        if (!ok) {
            print_error("get_proxy_info()", "InternetQueryOption()");
            ++Failures;
        } else if (Verbose) {
            dump_proxy_info(hInternet, (LPINTERNET_PROXY_INFO)buf);
        }
        free(buf);
    }
}

void set_proxy_info(HINTERNET hInternet, LPINTERNET_PROXY_INFO ProxyInfo) {

    BOOL ok;

    ok = InternetSetOption(hInternet, INTERNET_OPTION_PROXY, (LPVOID)ProxyInfo, sizeof(*ProxyInfo));
    if (!ok) {
        print_error("set_proxy_info()", "InternetSetOption()");
        ++Failures;
    }
}

void dump_proxy_info(HINTERNET hInternet, LPINTERNET_PROXY_INFO ProxyInfo) {
    printf("INTERNET_PROXY_INFO for handle %#x:\n"
           "\tAccess Type   : %s\n"
           "\tProxy Server  : \"%s\"\n"
           "\tProxy Bypass  : \"%s\"\n"
           "\n",
           hInternet,
           (ProxyInfo->dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG)
                ? "PRECONFIG"
                : (ProxyInfo->dwAccessType == INTERNET_OPEN_TYPE_DIRECT)
                    ? "DIRECT"
                    : (ProxyInfo->dwAccessType == INTERNET_OPEN_TYPE_PROXY)
                        ? "PROXY"
                        : "?",
           (ProxyInfo->lpszProxy != NULL) ? ProxyInfo->lpszProxy : "",
           (ProxyInfo->lpszProxyBypass != NULL) ? ProxyInfo->lpszProxyBypass : ""
           );
}

void get_user_agent(HINTERNET hInternet, char* Buffer, LPDWORD lpdwLen) {

    DWORD length;
    BOOL ok;

    length = 0;
    ok = InternetQueryOption(hInternet, INTERNET_OPTION_USER_AGENT, NULL, &length);
    if (ok) {
        printf("error: get_user_agent(%#x): InternetQueryOption() w/ no buffer succeeds\n", hInternet);
        ++Failures;
    } else if (length <= *lpdwLen) {
        if (Verbose) {
            printf("get_user_agent(%#x): %d bytes required for proxy info buffer\n", hInternet, length);
        }
        memset(Buffer, 0x99, *lpdwLen);
        ok = InternetQueryOption(hInternet, INTERNET_OPTION_USER_AGENT, Buffer, &length);
        if (!ok) {
            print_error("get_proxy_info()", "InternetQueryOption()");
            ++Failures;
        } else {
            if (Buffer[length] != '\0') {
                printf("error: InternetQueryOption(USER_AGENT) returns incorrectly terminated string\n");
                ++Failures;
            }
            *lpdwLen = length;
            if (Verbose) {
                printf("User-Agent for %#x = \"%s\" (%d)\n", hInternet, Buffer, length);
            }
        }
    } else {
        printf("error: get_user_agent(%#x): not enough buffer (%d)\n", hInternet, *lpdwLen);
        ++Failures;
    }
}

void set_user_agent(HINTERNET hInternet, char* String) {

    BOOL ok;

    ok = InternetSetOption(hInternet, INTERNET_OPTION_USER_AGENT, (LPVOID)String, strlen(String) + 1);
    if (!ok) {
        print_error("set_user_agent()", "InternetSetOption()");
        ++Failures;
    }
}

void refresh_handle(HINTERNET hInternet) {

    BOOL ok;
    DWORD zero;

    zero = 0;
    ok = InternetSetOption(hInternet, INTERNET_OPTION_REFRESH, &zero, sizeof(zero));
    if (!ok) {
        print_error("refresh_handle()", "InternetSetOption()");
        ++Failures;
    }
}
