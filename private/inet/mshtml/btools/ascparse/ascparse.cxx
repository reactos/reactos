//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       ascparse.cxx
//
//  Contents:   Tool to build .hsc files from .asc files.
//
//              Does the work of precomputing hash tables for associative
//              arrays of strings.
//
//----------------------------------------------------------------------------

#define INCMSG(x)
#include "headers.hxx"

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>
#endif

#ifndef X_PLATFORM_H_
#define X_PLATFORM_H_
#include <platform.h>
#endif

#ifndef X_MSHTMDBG_H_
#define X_MSHTMDBG_H_
#undef PERFMETER
#include <mshtmdbg.h>
#endif

#define ASCPARSE

// The following macro definitions allow us to use assoc.cxx
// without bringing in the whole CORE directory

#ifndef X_TCHAR_H_
#define X_TCHAR_H_
#include "tchar.h"
#endif

#define THR(x) (x)
#define RRETURN(x) return(x)
#define _MemAlloc(cb) malloc(cb)
#define _MemAllocClear(cb) calloc(1,cb)
#define _MemFree(x) free(x)
#define MemAlloc(mt,cb) _MemAlloc(cb)
#define MemAllocClear(mt,cb) _MemAllocClear(cb)
#define MemFree(x) _MemFree(x)
#define MemRealloc(mt, ppv, cb) _MemRealloc(ppv, cb)
#define _tcsequal(x,y) (!_tcscmp(x,y))
#define Assert(x) if (!(x)) { fprintf(stderr, "%s", #x); exit(1); }
#define Verify(x) if (!(x)) { fprintf(stderr, "%s", #x); exit(1); }
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(x[0]))

HRESULT 
_MemRealloc(void **ppv, size_t cb)
{
    void *pv;

    if (*ppv == NULL)
    {
        *ppv = _MemAlloc(cb);
        if (*ppv == NULL)
            return E_OUTOFMEMORY;
    }
    else
    {       
        pv = realloc(*ppv, cb);
        if (pv == NULL)
            return E_OUTOFMEMORY;
        *ppv = pv;
    }
    return S_OK;
};

void GetSuffix(LPCSTR pAssocString, LPSTR pSuffix, int nNumber)
{
    pSuffix[0] = '\0'; // NULL terminate the suffix
    // check if the First Character is a Captial letter.
    if ( islower(pAssocString[0]) )
        return;

    _itoa(nNumber, pSuffix, 10);
}

#include "assoc.cxx"

// end of stubs


#define MAX_WORD 64
#define MAX_LINE 4096

class CAscParser
{
public:
    CAscParser() { memset(this, 0, sizeof(*this)); }
    
    class CAscEntry {
    public:
        CAscEntry() { memset(this, 0, sizeof(*this)); }
        CAscEntry *_pEntryNext;
        char _achString[MAX_WORD];
        int  _number;
        char _achNumber[MAX_WORD];
        char _achStringName[MAX_WORD];
        char _achAssoc[MAX_WORD];
        char _achEnum[MAX_WORD];
        BOOL _fNoassoc;
        BOOL _fNostring;
        BOOL _fNoenum;
        CAssoc *_pAssoc;
    };

    HRESULT ProcessAscFile(char *pchInputFile, char *pchOutputFile);
    char _achAssocArray[MAX_WORD];
    char _achAssocPrefix[MAX_WORD];
    char _achEnumType[MAX_WORD];
    char _achEnumPrefix[MAX_WORD];
    char _achStringNamePrefix[MAX_WORD];
    BOOL _fInsensitive;
    BOOL _fReversible;
    CAscEntry *_pEntryFirst;
    CAscEntry *_pEntryLast;
};
    
static BOOL ReadLine(FILE *fp, char *pchBuf, int cchBuf, int *pcchRead);
static void SkipSpace(char **ppch);
static void SkipNonspace(char **ppch);
static void ChopComment(char *pch);
static void GetWord(char **ppch, char **ppchWord);


int __cdecl
main  ( int argc, char *argv[] )
{
    HRESULT hr = E_FAIL;
    CAscParser np;
    
    if (argc != 3)
        goto Cleanup;

    hr = np.ProcessAscFile(argv[1], argv[2]);

Cleanup:

    if (hr)
        printf ( "Error %lx building ASC file\n", hr);
    exit(hr);
    return hr;
}

HRESULT
CAscParser::ProcessAscFile(char *pchInputFile, char *pchOutputFile)
{
    HRESULT hr;
    FILE   *fpInput = NULL;
    FILE   *fpOutput = NULL;
    char    achBuf[MAX_LINE];
    char   *pch;
    char   *pchWord;
    CAscEntry *pEntryNew;
    CAscEntry *pEntry;
    CAssocArray nt;

    nt.Init();
    _fReversible = FALSE;

    // open input file
    fpInput = fopen(pchInputFile, "r");
    if (!fpInput)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // open output file
    fpOutput = fopen(pchOutputFile, "w");
    if (!fpOutput)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // phase 1: read the header section
    hr = E_FAIL;
    
    for (;;)
    {
        if (!ReadLine(fpInput, achBuf, MAX_LINE, NULL))
            goto Cleanup;
        pch = achBuf;
        ChopComment(pch);
        GetWord(&pch, &pchWord);
        if (!*pchWord)
            continue;
        if (!strcmp(pchWord, "assocarray"))
        {
            GetWord(&pch, &pchWord);
            strcpy(_achAssocArray, pchWord);
            GetWord(&pch, &pchWord);
            strcpy(_achAssocPrefix, pchWord);
        }
        else
        if (!strcmp(pchWord, "enum"))
        {
            GetWord(&pch, &pchWord);
            strcpy(_achEnumType, pchWord);
            GetWord(&pch, &pchWord);
            strcpy(_achEnumPrefix, pchWord);
        }
        else
        if (!strcmp(pchWord, "string"))
        {
            GetWord(&pch, &pchWord);
            strcpy(_achStringNamePrefix, pchWord);
        }
        else
        if (!strcmp(pchWord, "case-insensitive"))
        {
            _fInsensitive = TRUE;
        }
        else
        if (!strcmp(pchWord, "case-sensitive"))
        {
            _fInsensitive = FALSE;
        }
        else
        if (!strcmp(pchWord, "reversible"))
        {
            _fReversible = TRUE;
        }
        else
        if (!strcmp(pchWord, "start"))
            break;
    }
    
    // phase 2: read the assoc table section
    hr = S_OK;
    
    while (ReadLine(fpInput, achBuf, MAX_LINE, NULL))
    {
        pch = achBuf;
        
        ChopComment(pch);
        GetWord(&pch, &pchWord);
        if (!*pchWord)
            continue;

        // allocate
        pEntryNew = new CAscEntry;
        if (!pEntryNew)
            return E_OUTOFMEMORY;

        // link up
        if (!_pEntryLast)
        {
            pEntryNew->_number = 0;
            _pEntryLast = _pEntryFirst = pEntryNew;
        }
        else
        {
            pEntryNew->_number = _pEntryLast->_number+1;
            _pEntryLast->_pEntryNext = pEntryNew;
            _pEntryLast = pEntryNew;
        }

        // fill in assoc
        strcpy(pEntryNew->_achString, pchWord);

        // fill in other fields
        for (;;)
        {
            GetWord(&pch, &pchWord);
            if (!*pchWord)
                break;

            if (!strcmp(pchWord, "number"))
            {
                GetWord(&pch, &pchWord);
                if (*pchWord == '=')
                {
                    for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
                    {
                        if (!strcmp(pchWord+1, pEntry->_achString))
                        {
                            break;
                        }
                    }
                    
                    if (!pEntry)
                    {
                        hr = E_FAIL;
                        goto Cleanup;
                    }
                    
                    pEntryNew->_number = pEntry->_number;
                    strcpy(pEntryNew->_achNumber, pEntry->_achNumber);
                }
                else if (*pchWord >= '0' && *pchWord <= '9' || *pchWord == '-')
                {
                    pEntryNew->_number = atol(pchWord);
                    *pEntryNew->_achNumber = '\0';
                }
                else
                {
                    pEntryNew->_number = 0;
                    strcpy(pEntryNew->_achNumber, pchWord);
                }
            }
            else
            if (!strcmp(pchWord, "string"))
            {
                GetWord(&pch, &pchWord);
                strcpy(pEntryNew->_achStringName, pchWord);
            }
            else
            if (!strcmp(pchWord, "enum"))
            {
                GetWord(&pch, &pchWord);
                strcpy(pEntryNew->_achEnum, pchWord);
            }
            else
            if (!strcmp(pchWord, "assoc"))
            {
                GetWord(&pch, &pchWord);
                strcpy(pEntryNew->_achAssoc, pchWord);
            }
            else
            if (!strcmp(pchWord, "noassoc"))
            {
                pEntryNew->_fNoassoc = TRUE;
            }
            else
            if (!strcmp(pchWord, "nostring"))
            {
                pEntryNew->_fNostring = TRUE;
            }
            else
            if (!strcmp(pchWord, "noenum"))
            {
                pEntryNew->_fNoenum = TRUE;
            }
        }
    }

    // compute assocs
    for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
    {
        if (!pEntry->_fNoassoc)
        {
            WCHAR awch[MAX_WORD];
            WCHAR *pwch = awch;
            char *pch = pEntry->_achString;
            DWORD len;
            DWORD hash;
            CAssoc *passoc;
            
            do { *pwch++ = *pch; } while (*pch++);

            len = _tcslen(awch);
            
            if (_fInsensitive)
                hash = HashStringCi(awch, len, 0);
            else
                hash = HashString(awch, len, 0);

            passoc = nt.AddAssoc((DWORD_PTR)pEntry, awch, len, hash);
            if (!passoc)
            {
                hr = E_FAIL;
                goto Cleanup;
            }
            pEntry->_pAssoc = passoc;
        }
    }
            
    // phase 3: output header decls enums
    fprintf(fpOutput, "// %s\n", pchOutputFile);
    fprintf(fpOutput, "// Generated by ascparse.exe from %s\n", pchInputFile);
    fprintf(fpOutput, "// Do not modify by hand!\n");
    fprintf(fpOutput, "\n#ifndef _cxx_\n\n");

    // assocarray and hash
    fprintf(fpOutput, "class CAssoc;\n");
    fprintf(fpOutput, "class CAssocArray;\n\n");
    
    fprintf(fpOutput, "extern CAssoc *%s_HashTable[%d];\n", _achAssocArray, nt._mHash);
    if (_fReversible)
    {
        fprintf(fpOutput, "extern CAssoc *%s_RevSearch[];\n", _achAssocArray);
    }
    fprintf(fpOutput, "extern CAssocArray %s;\n\n", _achAssocArray);
    
    // enums
    if (*_achEnumType)
    {
        fprintf(fpOutput, "enum %s\n{\n", _achEnumType);
        for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
        {
            if (!pEntry->_fNoenum)
            {
                if (*pEntry->_achEnum)
                {
                    fprintf(fpOutput, "    %s = %d,\n", pEntry->_achEnum, pEntry->_number);
                }
                else
                {
                    fprintf(fpOutput, "    %s%s = %d,\n", _achEnumPrefix, pEntry->_achString, pEntry->_number);
                }
            }
        }
        fprintf(fpOutput, "    %s_FORCE_LONG = LONG_MAX\n", _achEnumType);
        fprintf(fpOutput, "};\n\n");
    }
    
    // assocs
    for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
    {
        if (!pEntry->_fNoassoc)
        {
            if (!*pEntry->_achAssoc)
            {
                char suffix[32];
                GetSuffix(pEntry->_achString, suffix, pEntry->_number); 
                fprintf(fpOutput, "extern CAssoc %s%s%s;\n",
                    _achAssocPrefix, pEntry->_achString, suffix);
            }
            else
            {
                fprintf(fpOutput, "extern CAssoc %s;\n",
                    pEntry->_achAssoc);
            }
        }
    }
    
    fprintf(fpOutput, "\n\n");

    // strings
    for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
    {
        if (!pEntry->_fNostring)
        {
            if (!pEntry->_fNoassoc && (*_achStringNamePrefix || *pEntry->_achStringName))
            {
                if (*pEntry->_achStringName)
                {
                    fprintf(fpOutput, "#define %s ",
                        pEntry->_achStringName);
                }
                else
                {
                    fprintf(fpOutput, "#define %s%s ",
                        _achStringNamePrefix, pEntry->_achString);
                }
                
                if (!*pEntry->_achAssoc)
                {
                    char suffix[32];
                    GetSuffix(pEntry->_achString, suffix, pEntry->_number); 
                    fprintf(fpOutput, "(%s%s%s._ach)\n",
                        _achAssocPrefix, pEntry->_achString, suffix);
                }
                else
                {
                    fprintf(fpOutput, "(%s._ach)\n",
                        pEntry->_achAssoc);
                }
            }
            else
            {
                if (*pEntry->_achStringName)
                {
                    fprintf(fpOutput, "#define %s (_T(\"%s\"))\n",
                        pEntry->_achStringName, pEntry->_achString);
                }
                else if (*_achStringNamePrefix)
                {
                    fprintf(fpOutput, "#define *%s%s (_T(\"%s\"))\n",
                        _achStringNamePrefix, pEntry->_achString, pEntry->_achString);
                }
            }
        }
    }

    // end of header section; start of cxx section
    fprintf(fpOutput, "\n#else _cxx_\n\n");
    fprintf(fpOutput, "\n#undef _cxx_\n\n");

    
    // phase 4: output assocs
    for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
    {
        if (!pEntry->_fNoassoc)
        {
            if (!*pEntry->_achAssoc)
            {
                char suffix[32];
                GetSuffix(pEntry->_achString, suffix, pEntry->_number); 
                fprintf(fpOutput, "CAssoc %s%s%s\t\t\t= ",
                    _achAssocPrefix, pEntry->_achString, suffix);
            }
            else
            {
                fprintf(fpOutput, "CAssoc %s = ",
                    pEntry->_achAssoc);
            }
            if (*pEntry->_achNumber)
            {
                fprintf(fpOutput, "{ %12s, 0x%08x, _T(\"%s\") };\n",
                    pEntry->_achNumber, pEntry->_pAssoc->_hash, pEntry->_achString);
            }
            else
            {
                fprintf(fpOutput, "{ %5d, 0x%08x, _T(\"%s\") };\n",
                    pEntry->_number, pEntry->_pAssoc->_hash, pEntry->_achString);
            }
        }
    }

    // phase 5: output table
    // output hash table
    fprintf(fpOutput, "\n\nCAssoc *%s_HashTable[%d] =\n{\n", _achAssocArray, nt._mHash);
    {
        int i;
        int c;
        int s;
        int d;
        
        CAssoc **ppAssoc;
        for (ppAssoc = nt._pHashTable, c=nt._mHash; c; ppAssoc++, c--)
        {
            if (!*ppAssoc)
            {
                fprintf(fpOutput, "        NULL,\n");
            }
            else
            {
                i = (*ppAssoc)->Hash() % nt._mHash;
                s = ((*ppAssoc)->Hash() & nt._sHash) + 1;
                d = 0;
                while (nt._pHashTable + i != ppAssoc)
                {
                    if (i < s)
                        i += nt._mHash;
                    i -= s;
                    d++;
                }
                    
                fprintf(fpOutput, "/*%2d */ ",d);
                
                pEntry = (CAscEntry*)(*ppAssoc)->Number();
                
                if (!*pEntry->_achAssoc)
                {
                    char suffix[32];
                    GetSuffix(pEntry->_achString, suffix, pEntry->_number); 
                    fprintf(fpOutput, "&%s%s%s,\n",
                        _achAssocPrefix, pEntry->_achString, suffix);
                }
                else
                {
                    char suffix[32];
                    GetSuffix(pEntry->_achAssoc, suffix, pEntry->_number); 
                    fprintf(fpOutput, "&%s%s,\n",
                        pEntry->_achAssoc, suffix);
                }

            }
        }
    }

    fprintf(fpOutput, "};\n\n");

    // phase 6: output table for reverse search (if requested)
    if (_fReversible)
    {
        CAscParser::CAscEntry *    pFound;
        CAscParser::CAscEntry *    pEntry;
        int nCurrMin;
        int nPrevMin;
        int nCurrVal;

        fprintf(fpOutput, "CAssoc *%s_RevSearch[] =\n{\n", _achAssocArray);

        // find and print the entries in order from numeric min to max
        nPrevMin = 0;
        for(;;)
        {
            // find the next entry

            nCurrMin = INT_MAX;
            pFound = NULL;

            for (pEntry = _pEntryFirst; pEntry; pEntry = pEntry->_pEntryNext)
            {
                if (*pEntry->_achNumber)
                {
                    nCurrVal = atol(pEntry->_achNumber);
                }
                else
                {
                    nCurrVal = pEntry->_number;
                }

                if (nCurrVal > nPrevMin && nCurrVal < nCurrMin)
                {
                    nCurrMin = nCurrVal;
                    pFound = pEntry;
                }
            }

            // break out once we've done everything
            if (pFound == NULL)
            {
                break;
            }

            // output a pointer to the assoc
            if (!*pFound->_achAssoc)
            {
                char suffix[32];
                GetSuffix(pFound->_achString, suffix, pFound->_number); 
                fprintf(fpOutput, "\t&%s%s%s,\n",
                    _achAssocPrefix, pFound->_achString, suffix);
            }
            else
            {
                fprintf(fpOutput, "\t%s,\n", pFound->_achAssoc);
            }

            nPrevMin = nCurrMin;
        }

        fprintf(fpOutput, "};\n\n");
    }

    // output assoc table struct itself
    fprintf(fpOutput, "CAssocArray %s = {\n", _achAssocArray);
    fprintf(fpOutput, "    %s_HashTable,\n", _achAssocArray);
    fprintf(fpOutput, "    %d,\n", nt._cHash);
    fprintf(fpOutput, "    %d,\n", nt._mHash);
    fprintf(fpOutput, "    %d,\n", nt._sHash);
    fprintf(fpOutput, "    %d,\n", nt._cHash);
    fprintf(fpOutput, "    %d,\n", nt._iSize);
    fprintf(fpOutput, "    TRUE,\n");
    fprintf(fpOutput, "};\n\n");
        
    fprintf(fpOutput, "\n#endif _cxx_\n\n");
    
    
Cleanup:
    nt.Deinit();
    
    if (fpInput)
        fclose(fpInput);
    if (fpOutput)
        fclose(fpOutput);

    return hr;
}

static
BOOL
ReadLine(FILE *fp, char *pchBuf, int cchBuf, int *pcchRead)
{
    int cchRead;
    
    if (!fgets(pchBuf, cchBuf, fp))
        return FALSE;

    cchRead = strlen(pchBuf);
    if (!cchRead)
        return FALSE;

    if (pcchRead)
        *pcchRead = cchRead;
        
    return TRUE;
}

static
void
SkipSpace(char **ppch)
{
    char *pch = *ppch;
    while (*pch && (*pch == ' ' || *pch == '\t' || *pch == '\r' || *pch == '\n'))
        pch++;
    *ppch = pch;
}


static
void
SkipNonspace(char **ppch)
{
    char *pch = *ppch;
    while (*pch && (*pch != ' ' && *pch != '\t' && *pch != '\r' && *pch != '\n'))
        pch++;
    *ppch = pch;
}

static
void
ChopComment(char *pch)
{
    while (*pch)
    {
        if (*pch == '/' && *(pch+1) == '/')
        {
            *pch = '\0';
            return;
        }
        pch++;
    }
}

static
void
GetWord(char **ppch, char **ppchWord)
{
    SkipSpace(ppch);
    *ppchWord = *ppch;
    SkipNonspace(ppch);
    if (**ppch)
    {
        **ppch = '\0';
        if (*ppch - *ppchWord > MAX_WORD)
            *(*ppchWord + MAX_WORD-1) = '\0';
        (*ppch)++;
    }
}

