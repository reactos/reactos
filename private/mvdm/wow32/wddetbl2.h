/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WDDETBL2.h
 *  WOW32 16-bit DDEML API tables
 *
 *  History:
 *  Created 26-Jan-1993 by Chandan Chauhan (ChandanC)
--*/


    {W32FUN(UNIMPLEMENTEDAPI,                 "DUMMYENTRY",               MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(WD32DdeInitialize,                "DDEINITIALIZE",            MOD_DDEML,   sizeof(DDEINITIALIZE16))},
    {W32FUN(WD32DdeUninitialize,              "DDEUNINITIALIZE",          MOD_DDEML,   sizeof(DDEUNINITIALIZE16))},
    {W32FUN(WD32DdeConnectList,               "DDECONNECTLIST",           MOD_DDEML,   sizeof(DDECONNECTLIST16))},
    {W32FUN(WD32DdeQueryNextServer,           "DDEQUERYNEXTSERVER",       MOD_DDEML,   sizeof(DDEQUERYNEXTSERVER16))},
    {W32FUN(WD32DdeDisconnectList,            "DDEDISCONNECTLIST",        MOD_DDEML,   sizeof(DDEDISCONNECTLIST16))},
    {W32FUN(WD32DdeConnect,                   "DDECONNECT",               MOD_DDEML,   sizeof(DDECONNECT16))},
    {W32FUN(WD32DdeDisconnect,                "DDECONNECT",               MOD_DDEML,   sizeof(DDEDISCONNECT16))},
    {W32FUN(WD32DdeQueryConvInfo,             "DDEQUERYCONVINFO",         MOD_DDEML,   sizeof(DDEQUERYCONVINFO16))},
    {W32FUN(WD32DdeSetUserHandle,              "DDESETUSERHANDLE",        MOD_DDEML,   sizeof(DDESETUSERHANDLE16))},

  /*** 0011 ***/
    {W32FUN(WD32DdeClientTransaction,         "DDECLIENTTRANSACTION",     MOD_DDEML,   sizeof(DDECLIENTTRANSACTION16))},
    {W32FUN(WD32DdeAbandonTransaction,        "DDEABANDONTRANSACTION",    MOD_DDEML,   sizeof(DDEABANDONTRANSACTION16))},
    {W32FUN(WD32DdePostAdvise,                "DDEPOSTADVISE",            MOD_DDEML,   sizeof(DDEPOSTADVISE16))},
    {W32FUN(WD32DdeCreateDataHandle,          "DDECREATEDATAHANDLE",      MOD_DDEML,   sizeof(DDECREATEDATAHANDLE16))},
    {W32FUN(WD32DdeAddData,                   "DDEADDDATA",               MOD_DDEML,   sizeof(DDEADDDATA16))},
    {W32FUN(WD32DdeGetData,                   "DDEGETDATA",               MOD_DDEML,   sizeof(DDEGETDATA16))},
    {W32FUN(WD32DdeAccessData,                "DDEACCESSDATA",            MOD_DDEML,   sizeof(DDEACCESSDATA16))},
    {W32FUN(WD32DdeUnaccessData,              "DDEUNACCESSDATA",          MOD_DDEML,   sizeof(DDEUNACCESSDATA16))},
    {W32FUN(WD32DdeFreeDataHandle,            "DDEFREEDATAHANDLE",        MOD_DDEML,   sizeof(DDEFREEDATAHANDLE16))},
    {W32FUN(WD32DdeGetLastError,              "DDEGETLASTERROR",          MOD_DDEML,   sizeof(DDEGETLASTERROR16))},

  /*** 0021 ***/
    {W32FUN(WD32DdeCreateStringHandle,        "DDECREATESTRINGHANDLE",    MOD_DDEML,   sizeof(DDECREATESTRINGHANDLE16))},
    {W32FUN(WD32DdeFreeStringHandle,          "DDEFREESTRINGHANDLE",      MOD_DDEML,   sizeof(DDEFREESTRINGHANDLE16))},
    {W32FUN(WD32DdeQueryString,               "DDEQUERYSTRING",           MOD_DDEML,   sizeof(DDEQUERYSTRING16))},
    {W32FUN(WD32DdeKeepStringHandle,          "DDEKEEPSTRINGHANDLE",      MOD_DDEML,   sizeof(DDEKEEPSTRINGHANDLE16))},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(WD32DdeEnableCallback,            "DDEENABLECALLBACK",        MOD_DDEML,   sizeof(DDEENABLECALLBACK16))},
    {W32FUN(WD32DdeNameService,               "DDENAMESERVICE",           MOD_DDEML,   sizeof(DDENAMESERVICE16))},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},

  /*** 0031 ***/
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(WD32DdeCmpStringHandles,          "DDECMPSTRINGHANDLES",      MOD_DDEML,   sizeof(DDECMPSTRINGHANDLES16))},
    {W32FUN(WD32DdeReconnect,                 "DDERECONNECT",             MOD_DDEML,   sizeof(DDERECONNECT16))},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},
    {W32FUN(UNIMPLEMENTEDAPI,                 "",                         MOD_DDEML,   0)},

