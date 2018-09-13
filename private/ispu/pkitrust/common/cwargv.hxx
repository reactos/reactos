//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       cwargv.hxx
//
//  Contents:   argv parsing api
//
//  History:    02-Oct-1997 pberkman    create
//
//--------------------------------------------------------------------------

#ifndef CWARGV_HXX
#define CWARGV_HXX

#include    "stack.hxx"

#define     WARGV_VALUETYPE_BOOL            1   // just there or not
#define     WARGV_VALUETYPE_WCHAR           2   // -p file_name, etc.
#define     WARGV_VALUETYPE_DWORDH          3   // -c 99, etc. hex
#define     WARGV_VALUETYPE_DWORDD          4   // -c 99, etc. decimal

//
//  command file argument is xxxx.exe [options] [filename] [@commandfile]
//

class cWArgv_
{
    public:
        cWArgv_(HINSTANCE hInst0, BOOL fCheckForCommandFile = TRUE);
        virtual ~cWArgv_(void);

        BOOL    Add2List(DWORD dwsidOption, DWORD dwsidOptionHellp,
                         DWORD dwValueType, void *pvDefaultValue = NULL,
                         BOOL fInternalArg = FALSE);
        BOOL    Fill(int argc, WCHAR **wargv);

        void    *GetValue(DWORD dwsidOption);

        WCHAR   *GetOption(DWORD dwsidOption);
        WCHAR   *GetOptionHelp(DWORD dwsidOption);

        WCHAR   *GetFileName(DWORD *pdwidxLast = NULL);

                            // the format should be "usage" "[options]" 
                            //                      "[@commandfile]" "filename"
                            //                      "<input param>"
        void    AddUsageText(DWORD dwsidUsageWord, DWORD dwsidUsageOptionsText, 
                             DWORD dwsidUsageCmdFileText, DWORD dwsidUsageAddText,
                             DWORD dwsidOptionParamText);

        WCHAR   *GetUsageString(void);

        BOOL    IsSet(DWORD dwsidOption);

    private:
        HINSTANCE   hInst;
        BOOL        fChkCmdF;
        BOOL        fShowHiddenArgs;
        BOOL        fNonHiddenParamArgs;
        WCHAR       *pwszThisFilename;
        WCHAR       *pwszUsageWord;
        WCHAR       *pwszUsageOptionsText;
        WCHAR       *pwszUsageCmdFileText;
        WCHAR       *pwszOptionParamText;
        WCHAR       *pwszUsageAddText;
        WCHAR       *pwszUsageString;
        WCHAR       *pwszNonParamArgBlanks;
        DWORD       dwLongestArg;
        Stack_      *pArgs;

        BOOL        ProcessCommandFile(WCHAR *pwszFile);
        int         ProcessArg(int argc, WCHAR **wargv);
        BOOL        AddFile(WCHAR *pwszFile);
        void        StripQuotes(WCHAR *pwszIn);
};

#endif // CWARGV_HXX
