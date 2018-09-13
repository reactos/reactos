//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       printfu.hxx
//
//  Contents:   Unicode Console Printf's
//
//  History:    02-May-1997 xiaohs      created
//              06-May-1997 pberkman    converted to C++
//
//--------------------------------------------------------------------------

#ifndef PRINTFU_HXX
#define PRINTFU_HXX

class PrintfU_
{
    public:
        PrintfU_(DWORD ccMaxString = 256);
        virtual ~PrintfU_(void);

        void _cdecl     Display(DWORD idFmt, ...);

        WCHAR           *get_String(DWORD dwID, WCHAR *pwszRet = NULL, DWORD ccRet = 0);

    private:
        WCHAR   *pwszDispString;
        WCHAR   *pwszResString;
        DWORD   ccMax;
        HMODULE hModule;
};

#endif // PRINTFU_HXX
