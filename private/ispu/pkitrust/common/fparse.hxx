//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       fparse.hxx
//
//  Contents:   File parsing api -- INI file types
//
//  History:    01-Oct-1997 pberkman    create
//
//--------------------------------------------------------------------------

#ifndef FPARSE_HXX
#define FPARSE_HXX


class fParse_
{
    public:
        fParse_(WCHAR *pwszFilename, DWORD dwMaxLine0 = MAX_PATH, 
                DWORD dwFileAccess = GENERIC_READ, DWORD dwFileSharing = FILE_SHARE_READ);
        virtual ~fParse_(void);

        void        Reset(void);

        WCHAR       *GetCurrentLine(void) { return(pwszCurrentLine); }

        DWORD       GetNextLine(void);

        BOOL        FindGroup(WCHAR *pwszGroup);
        BOOL        FindTagInCurrentGroup(WCHAR *pwszTag);
        BOOL        GetLineInCurrentGroup(void);

        BOOL        FindTagFromCurrentPos(WCHAR *pwszTag);

        BOOL        PositionAtLastGroup(void);
        BOOL        PositionAtLastTag(void);

        BOOL        AddTagToFile(WCHAR *pwszGroup, WCHAR *pwszTag, WCHAR *pwszValue);

        void        EOLRemove(void);

    private:
        HANDLE      hFile;
        WCHAR       *pwszFName;
        WCHAR       *pwszTempFName;
        WCHAR       *pwszCurrentLine;
        WCHAR       *pwszLastGroupTag;
        DWORD       dwMaxLine;
        DWORD       dwCurLineFilePos;
        DWORD       dwLastGroupFilePos;
        DWORD       dwLastTagFilePos;
        BOOL        fEOF;
};


#endif // FPARSE_HXX
