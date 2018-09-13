//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       nfparse.cxx
//
//  Contents:   Tool to build .nfh files from .nfi files.
//
//----------------------------------------------------------------------------

#ifndef X_STDLIB_H_
#define X_STDLIB_H_
#include <stdlib.h>
#endif

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_STDDEF_H_
#define X_STDDEF_H_
#include <stddef.h>
#endif

#ifndef X_SEARCH_H_
#define X_SEARCH_H_
#include <search.h>
#endif

#ifndef X_STRING_H_
#define X_STRING_H_
#include <string.h>
#endif

#ifndef X_CTYPE_H_
#define X_CTYPE_H_
#include <ctype.h>
#endif

#ifndef X_LIMITS_H_
#define X_LIMITS_H_
#include <limits.h>
#endif

#ifndef X_STDARG_H_
#define X_STDARG_H_
#include <stdarg.h>
#endif

//  Allow assignment within conditional
#pragma warning ( disable : 4706 )


#define MAX_WORD 256
#define MAX_LINE 4096
#define TRUE     1
#define FALSE    0
#define BOOL     int
#define ERROR    int
#define E_FAIL   1
#define S_OK     0

char achBlanks[] = "                                                                                    "
                   "                                                                                    "
                   "                                                                                    "
                   "                                                                                    "
                   "                                                                                    ";

class CNotificationParser
{
public:
    CNotificationParser() { memset(this, 0, sizeof(*this)); }
    
    class CNotification {
    public:
        CNotification() { memset(this, 0, sizeof(*this)); }
        void MakeSC( CNotification *pN );
        void MarkFirstChance();

        CNotification * _pNextN;

        char            _achType[MAX_WORD];

        //
        //  Targets
        //

        unsigned        _fSelf:1;
        unsigned        _fAncestors:1;
        unsigned        _fDescendents:1;
        unsigned        _fTree:1;

        //
        //  Categories
        //

        unsigned        _fTextChange:1;
        unsigned        _fTreeChange:1;
        unsigned        _fLayoutChange:1;
        unsigned        _fActiveX:1;
        unsigned        _fLayoutElements:1;
        unsigned        _fWindowedElements:1;
        unsigned        _fPositionedElements:1;
        unsigned        _fAllElements:1;

        //
        //  Properties
        //

        unsigned        _fSendUntilHandled:1;
        unsigned        _fLazyRange:1;
        unsigned        _fCleanChange:1;
        unsigned        _fSuperLast:1;
        unsigned        _fSynchronousOnly:1;
        unsigned        _fDoNotBlock:1;
        unsigned        _fClearFormatSelf:1;
        unsigned        _fClearFormat:1;
        unsigned        _fAutoOnly:1;
        unsigned        _fZParentsOnly:1;
        unsigned        _fSecondChanceAvail:1;
        unsigned        _fSecondChance:1;
        
        //
        //  Arguments
        //

        unsigned        _fElement:1;
        unsigned        _fSI:1;
        unsigned        _fCElements:1;
        unsigned        _fCp:1;
        unsigned        _fCch:1;
        unsigned        _fTreeNode:1;
        unsigned        _fData:1;
        unsigned        _fFlags:1;
    };

    char *  _pchInput;
    char *  _pchOutput;

    FILE *  _fpOutput;
    FILE *  _fpInput;
    FILE *  _fpLog;

    ERROR  Parse();
    BOOL   Validate(CNotification * pN);
    BOOL   ReadLine(char *pchBuf, int cchBuf, int *pcchRead);
    void   SkipSpace(char **ppch);
    void   SkipNonspace(char **ppch);
    void   ChopComment(char *pch);
    BOOL   GetWord(char **ppch, char **ppchWord);
    char * NameOf(char * pchInput, char * pchOutput);
    void   WriteArg(unsigned fFlag, char * pchArg, char * pchPad, int * pcArgs);
    void   WriteFlag(unsigned fFlag, char * pchFlag, int * pcFlags);
    void   ReportError(const char * pchError, ...);
};


ERROR __cdecl
main  ( int argc, char *argv[] )
{
    CNotificationParser np;
    ERROR               err = E_FAIL;
    
    if (argc != 3)
    {
        printf("nfparse: Usage <Notification File> <HeaderFile>\n");
        goto Cleanup;
    }


    np._pchInput  = argv[1];
    np._pchOutput = argv[2];

    err = np.Parse();

Cleanup:

    exit(err);
    return err;
}


ERROR
CNotificationParser::Parse()
{
    char                achBuf[MAX_LINE];
    char                achTemp[MAX_WORD];
    char *              pch;
    char *              pchWord  = NULL;
    CNotification *     pFirstN  = NULL;
    CNotification **    ppN      = &pFirstN;
    CNotification *     pN       = NULL;
    int                 i;
    int                 cch;
    ERROR               err;

    // open input file
    _fpInput = fopen(_pchInput, "r");
    if (!_fpInput)
    {
        err = E_FAIL;
        goto Cleanup;
    }

    // open output file
    _fpOutput = fopen(_pchOutput, "w");
    if (!_fpOutput)
    {
        err = E_FAIL;
        goto Cleanup;
    }

    // open log file
    strcpy(achBuf, _pchOutput);
    strcat(achBuf, "LOG");
    _fpLog = fopen(achBuf, "w");
    if (!_fpLog)
    {
        err = E_FAIL;
        goto Cleanup;
    }

    // phase 1: read the file
    err = E_FAIL;

    while (ReadLine(achBuf, MAX_LINE, NULL))
    {
        pch = achBuf;

        ChopComment(pch);

        while (GetWord(&pch, &pchWord))
        {
            if (!strcmp(pchWord, "type"))
            {
                if (*ppN == NULL)
                {
                    *ppN = new CNotification();
                    if (!*ppN)
                        goto Cleanup;
                    pN  = *ppN;
                    ppN = &pN->_pNextN;
                }

                if (!GetWord(&pch, &pchWord))
                {
                    ReportError("Missing notification type");
                    goto Cleanup;
                }
                strncpy(pN->_achType, pchWord, MAX_WORD-1);
            }

            else
            if (!strcmp(pchWord, "targets"))
            {
                if (!pN)
                {
                    ReportError("Notification does not start with 'type:'");
                    goto Cleanup;
                }

                while (GetWord(&pch, &pchWord))
                {
                    if (!strcmp(pchWord, "self"))
                    {
                        pN->_fSelf = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "ancestors"))
                    {
                        pN->_fAncestors = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "descendents"))
                    {
                        pN->_fDescendents = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "tree-level"))
                    {
                        pN->_fTree = TRUE;
                    }
                }
            }

            else
            if (!strcmp(pchWord, "categories"))
            {
                if (!pN)
                {
                    ReportError("Notification does not start with 'type:'");
                    goto Cleanup;
                }

                while (GetWord(&pch, &pchWord))
                {
                    if (!strcmp(pchWord, "text-change"))
                    {
                        pN->_fTextChange = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "tree-change"))
                    {
                        pN->_fTreeChange = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "layout-change"))
                    {
                        pN->_fLayoutChange = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "activex"))
                    {
                        pN->_fActiveX = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "layout-elements"))
                    {
                        pN->_fLayoutElements = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "windowed-elements"))
                    {
                        pN->_fWindowedElements = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "positioned-elements"))
                    {
                        pN->_fPositionedElements = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "all-elements"))
                    {
                        pN->_fAllElements = TRUE;
                    }
                }
            }

            else
            if (!strcmp(pchWord, "properties"))
            {
                if (!pN)
                {
                    ReportError("Notification does not start with 'type:'");
                    goto Cleanup;
                }

                while (GetWord(&pch, &pchWord))
                {
                    if (!strcmp(pchWord, "send-until-handled"))
                    {
                        pN->_fSendUntilHandled = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "lazy-range"))
                    {
                        pN->_fLazyRange = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "super-last"))
                    {
                        pN->_fSuperLast = TRUE;
                    }                    
                    else
                    if (!strcmp(pchWord, "clean-change"))
                    {
                        pN->_fCleanChange = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "synchronous-only"))
                    {
                        pN->_fSynchronousOnly = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "dont-block"))
                    {
                        pN->_fDoNotBlock = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "synchronous-only"))
                    {
                        pN->_fSynchronousOnly = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "clear-format-self"))
                    {
                        pN->_fClearFormatSelf = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "clear-format"))
                    {
                        pN->_fClearFormat = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "auto-only"))
                    {
                        pN->_fAutoOnly = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "zparents-only"))
                    {
                        pN->_fZParentsOnly = TRUE;
                    }
                    else
                    if (!strcmp(pchWord, "second-chance"))
                    {
                        pN->_fSecondChanceAvail = TRUE;
                    }
                }
            }

            else
            if (!strcmp(pchWord, "arguments"))
            {
                if (!pN)
                {
                    ReportError("Notification does not start with 'type:'");
                    goto Cleanup;
                }

                while (GetWord(&pch, &pchWord))
                {
                    if (!strcmp(pchWord, "element"))
                        pN->_fElement = 1;
                    else
                    if (!strcmp(pchWord, "si"))
                        pN->_fSI = 1;
                    else
                    if (!strcmp(pchWord, "celements"))
                        pN->_fCElements = 1;
                    else
                    if (!strcmp(pchWord, "cp"))
                        pN->_fCp = 1;
                    else
                    if (!strcmp(pchWord, "cch"))
                        pN->_fCch = 1;
                    else
                    if (!strcmp(pchWord, "tree-node"))
                        pN->_fTreeNode = 1;
                    else
                    if (!strcmp(pchWord, "data"))
                        pN->_fData = 1;
                    else
                    if (!strcmp(pchWord, "flags"))
                        pN->_fFlags = 1;
                }
            }

            else
            {
                ReportError("Unknown keyword (%s)", pchWord);
                goto Cleanup;
            }
        }
    }


    // phase 2: validate the notifications
    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        if (!Validate(pN))
        {
            ReportError("Notification %s is invalid", pN->_achType);
            goto Cleanup;
        }
    }

    // phase 3: expand second chance notifications
    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        if (pN->_fSecondChanceAvail)
        {
            CNotification *     pNNew = new CNotification();
            if (!pNNew)
                goto Cleanup;

            pNNew->MakeSC( pN );

            pN->MarkFirstChance();

            pNNew->_pNextN = pN->_pNextN;
            pN->_pNextN = pNNew;
            pN = pNNew;
        }
    }
    
            
    // phase 4: output the file
    fprintf(_fpOutput, "//-----------------------------------------------------------------------------\n");
    fprintf(_fpOutput, "//\n");
    fprintf(_fpOutput, "// WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING\n");
    fprintf(_fpOutput, "//\n");
    fprintf(_fpOutput, "// %s\n", _pchOutput);
    fprintf(_fpOutput, "// Generated by NFPARSE.EXE from %s\n", _pchInput);
    fprintf(_fpOutput, "// DO NOT MODIFY BY HAND\n");
    fprintf(_fpOutput, "//\n");
    fprintf(_fpOutput, "// WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING - WARNING\n");
    fprintf(_fpOutput, "//\n");
    fprintf(_fpOutput, "//-----------------------------------------------------------------------------\n\n");

    // generate the enums
    fprintf(_fpOutput, "#if defined(_NOTIFYTYPE_ENUM_)\n\n");

    for (i=0, pN = pFirstN; pN; pN = pN->_pNextN, i++)
    {
        sprintf(achTemp, "    NTYPE_%s", _strupr(pN->_achType));
        cch = 32 - strlen(achTemp);
        if (cch < 0)
            cch = 0;

        fprintf(_fpOutput, "%s", achTemp);

        strncpy(achTemp, achBlanks, cch);
        achTemp[cch] = '\0';

        fprintf(_fpOutput, "%s = 0x%08X,\n", achTemp, (long)i);
    }

    fprintf(_fpOutput, "#undef _NOTIFYTYPE_ENUM_\n\n");

    fprintf(_fpOutput, "\n");

    // generate the flag table
    fprintf(_fpOutput, "#elif defined(_NOTIFYTYPE_TABLE_)\n\n");

    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        int cFlags = 0;

        sprintf(achTemp, "/* NTYPE_%s", _strupr(pN->_achType));
        cch = 32 - strlen(achTemp);
        if (cch < 0)
            cch = 0;

        fprintf(_fpOutput, "%s", achTemp);

        strncpy(achTemp, achBlanks, cch);
        achTemp[cch] = '\0';

        fprintf(_fpOutput, "%s*/", achTemp);

        WriteFlag(pN->_fSelf,               "NFLAGS_SELF",              &cFlags);
        WriteFlag(pN->_fAncestors,          "NFLAGS_ANCESTORS",         &cFlags);
        WriteFlag(pN->_fDescendents,        "NFLAGS_DESCENDENTS",       &cFlags);
        WriteFlag(pN->_fTree,               "NFLAGS_TREELEVEL",         &cFlags);

        WriteFlag(pN->_fTextChange,         "NFLAGS_TEXTCHANGE",        &cFlags);
        WriteFlag(pN->_fTreeChange,         "NFLAGS_TREECHANGE",        &cFlags);
        WriteFlag(pN->_fLayoutChange,       "NFLAGS_LAYOUTCHANGE",      &cFlags);
        WriteFlag(pN->_fActiveX,            "NFLAGS_FOR_ACTIVEX",       &cFlags);
        WriteFlag(pN->_fLayoutElements,     "NFLAGS_FOR_LAYOUTS",       &cFlags);
        WriteFlag(pN->_fWindowedElements,   "NFLAGS_FOR_WINDOWED",      &cFlags);
        WriteFlag(pN->_fPositionedElements, "NFLAGS_FOR_POSITIONED",    &cFlags);
        WriteFlag(pN->_fAllElements,        "NFLAGS_FOR_ALLELEMENTS",   &cFlags);
        
        WriteFlag(pN->_fSendUntilHandled,   "NFLAGS_SENDUNTILHANDLED",  &cFlags);
        WriteFlag(pN->_fLazyRange,          "NFLAGS_LAZYRANGE",         &cFlags);
        WriteFlag(pN->_fCleanChange,        "NFLAGS_CLEANCHANGE",       &cFlags);
        WriteFlag(pN->_fSuperLast,          "NFLAGS_CALLSUPERLAST",     &cFlags);
        WriteFlag(pN->_fSynchronousOnly,    "NFLAGS_SYNCHRONOUSONLY",   &cFlags);
        WriteFlag(pN->_fDoNotBlock,         "NFLAGS_DONOTBLOCK",        &cFlags);
        WriteFlag(pN->_fClearFormatSelf,    "NFLAGS_CLEARFORMATSELF",   &cFlags);
        WriteFlag(pN->_fClearFormat,        "NFLAGS_CLEARFORMAT",       &cFlags);
        WriteFlag(pN->_fAutoOnly,           "NFLAGS_AUTOONLY",          &cFlags);
        WriteFlag(pN->_fZParentsOnly,       "NFLAGS_ZPARENTSONLY",      &cFlags);
        WriteFlag(pN->_fSecondChance,       "NFLAGS_SC",                &cFlags);
        WriteFlag(pN->_fSecondChanceAvail,  "NFLAGS_SC_AVAILABLE",      &cFlags);

        fprintf(_fpOutput, ",\n");
    }

    fprintf(_fpOutput, "#undef _NOTIFYTYPE_TABLE_\n\n");

    fprintf(_fpOutput, "\n");

    // generate the names
    fprintf(_fpOutput, "#elif defined(_NOTIFYTYPE_NAMES_)\n\n");

    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        fprintf(_fpOutput, "        case NTYPE_%s:\n", _strupr(pN->_achType));
        fprintf(_fpOutput, "            pch = _T(\"%s\");\n", _strupr(pN->_achType));
        fprintf(_fpOutput, "            break;\n");
    }

    fprintf(_fpOutput, "#undef _NOTIFYTYPE_NAMES_\n\n");

    fprintf(_fpOutput, "\n");

    // generate the prototypes
    fprintf(_fpOutput, "#elif defined(_NOTIFYTYPE_PROTO_)\n\n");

    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        int cArgs = 0;

        fprintf(_fpOutput, "        void %s(\n", NameOf(pN->_achType, achTemp));

        WriteArg(pN->_fElement,   "CElement *  pElement",      "             ", &cArgs);
        WriteArg(pN->_fSI,        "long        siElement",     "             ", &cArgs);
        WriteArg(pN->_fCElements, "long        cElements",     "             ", &cArgs);
        WriteArg(pN->_fCp,        "long        cp",            "             ", &cArgs);
        WriteArg(pN->_fCch,       "long        cch",           "             ", &cArgs);
        WriteArg(pN->_fTreeNode,  "CTreeNode * pNode  = NULL", "             ", &cArgs);
        WriteArg(pN->_fData,      "void *      pvData = NULL", "             ", &cArgs);
        WriteArg(pN->_fFlags,     "DWORD       grfFlags = 0",  "             ", &cArgs);

        fprintf(_fpOutput, ");\n\n");
    }

    fprintf(_fpOutput, "#undef _NOTIFYTYPE_PROTO_\n\n");

    fprintf(_fpOutput, "\n");

    // generate the inlines
    fprintf(_fpOutput, "#elif defined(_NOTIFYTYPE_INLINE_)\n\n");

    for (pN = pFirstN; pN; pN = pN->_pNextN)
    {
        int cArgs = 0;

        fprintf(_fpOutput, "inline void\nCNotification::%s(\n", NameOf(pN->_achType, achTemp));
        WriteArg(pN->_fElement,   "CElement *  pElement",  "    ", &cArgs);
        WriteArg(pN->_fSI,        "long        siElement", "    ", &cArgs);
        WriteArg(pN->_fCElements, "long        cElements", "    ", &cArgs);
        WriteArg(pN->_fCp,        "long        cp",        "    ", &cArgs);
        WriteArg(pN->_fCch,       "long        cch",       "    ", &cArgs);
        WriteArg(pN->_fTreeNode,  "CTreeNode * pNode",     "    ", &cArgs);
        WriteArg(pN->_fData,      "void *      pvData",    "    ", &cArgs);
        WriteArg(pN->_fFlags,     "DWORD       grfFlags",  "    ", &cArgs);
        fprintf(_fpOutput, ")\n{\n");

        if (    pN->_fElement
            &&  !pN->_fSI
            &&  !pN->_fCp
            &&  !pN->_fTreeChange)
            fprintf(_fpOutput, "    Assert(pElement);\n");

        cArgs = 0;

        fprintf(_fpOutput, "    Initialize%s%s(NTYPE_%s,\n",
                            pN->_fSI
                                ? "Si"
                                : "",
                            pN->_fCp
                                ? "Cp"
                                : "",
                            _strupr(pN->_achType));

        WriteArg(pN->_fElement, "pElement", "               ", &cArgs);

        if (pN->_fSI)
        {
            WriteArg(pN->_fSI,        "siElement", "               ", &cArgs);
            WriteArg(pN->_fCElements, "cElements", "               ", &cArgs);
        }

        if (pN->_fCp)
        {
            WriteArg(1, (pN->_fCp
                              ? "cp"
                              : "-1"),      "               ", &cArgs);
            WriteArg(1, (pN->_fCch
                              ? "cch"
                              : "-1"),      "               ", &cArgs);
        }

        WriteArg(1, (pN->_fTreeNode
                          ? "pNode"
                          : "NULL"),   "               ", &cArgs);
        WriteArg(1, (pN->_fData
                          ? "pvData"
                          : "NULL"),   "               ", &cArgs);
        WriteArg(1, (pN->_fFlags
                          ? "grfFlags"
                          : "0"),      "               ", &cArgs);

        fprintf(_fpOutput, ");\n");
        fprintf(_fpOutput, "}\n\n");
    }

    fprintf(_fpOutput, "#undef _NOTIFYTYPE_INLINE_\n\n");

    fprintf(_fpOutput, "\n");

    fprintf(_fpOutput, "#endif\n\n");

    err = S_OK;

Cleanup:

    if (err)
        ReportError("Could not build %s\n", _pchOutput);

    if (_fpInput)
        fclose(_fpInput);
    if (_fpOutput)
        fclose(_fpOutput);
    if (_fpLog)
        fclose(_fpLog);

    return err;
}

BOOL
CNotificationParser::Validate(CNotification * pN)
{
    ERROR err = E_FAIL;

    if (!*pN->_achType)
    {
        ReportError("Notification is missing 'type:'");
        goto Cleanup;
    }

    if (    !pN->_fSelf
        &&  !pN->_fAncestors
        &&  !pN->_fDescendents
        &&  !pN->_fTree)
    {
        ReportError("Notification is missing targets");
        goto Cleanup;
    }

    if (    pN->_fAncestors
        &&  pN->_fDescendents)
    {
        ReportError("Notification is sent to both ancestors and descendents");
        goto Cleanup;
    }

    if (    !pN->_fTextChange
        &&  !pN->_fTreeChange
        &&  !pN->_fLayoutChange
        &&  !pN->_fActiveX
        &&  !pN->_fLayoutElements
        &&  !pN->_fWindowedElements
        &&  !pN->_fPositionedElements
        &&  !pN->_fAllElements)
    {
        ReportError("Notification is missing categories");
        goto Cleanup;
    }

    if (    !pN->_fElement
        &&  !pN->_fSI
        &&  !pN->_fCp
        &&  !pN->_fTreeNode)
    {
        ReportError("Notification requires either 'element', 'si', 'cp', or 'tree-node'");
        goto Cleanup;
    }

    if (    pN->_fCElements
        &&  !pN->_fSI)
    {
        ReportError("Notification has 'celement' without 'si'");
        goto Cleanup;
    }

    if (    pN->_fCch
        &&  !pN->_fCp)
    {
        ReportError("Notification has 'cch' without 'cp'");
        goto Cleanup;
    }

    if (    pN->_fSecondChanceAvail
        &&  (   pN->_fSelf && strcmp( pN->_achType, "ELEMENT_EXITTREE" )
            ||  pN->_fAncestors
            ||  pN->_fTree))
    {
        ReportError("Second chance only implemented for descendents and ELEMENT_EXITTREE");
        goto Cleanup;
    }

    err = S_OK;

Cleanup:
    return (err == S_OK);
}

char *
CNotificationParser::NameOf(char * pchInput, char * pchOutput)
{
    char *  pch = pchOutput;
    int     fMakeUpper = 1;

    while (*pchInput)
    {
        if (*pchInput == '_')
        {
            fMakeUpper = 1;
        }
        else
        {
            *pchOutput++ = (fMakeUpper
                                ? (char)toupper(*pchInput)
                                : (char)tolower(*pchInput));
            fMakeUpper = 0;
        }

        pchInput++;
    }

    *pchOutput = '\0';
    return pch;
}

void
CNotificationParser::WriteArg(unsigned fFlag, char * pchArg, char * pchPad, int * pcArgs)
{
    if (fFlag)
    {
        if (*pcArgs)
            fprintf(_fpOutput, ",\n");

        fprintf(_fpOutput, "%s", pchPad);

        (*pcArgs)++;

        fprintf(_fpOutput, "%s", pchArg);
    }
}

void
CNotificationParser::WriteFlag(unsigned fFlag, char * pchFlag, int * pcFlags)
{
    char    ach[MAX_WORD];
    int     cch = strlen(pchFlag);

    if (fFlag)
    {
        fprintf(_fpOutput, (*pcFlags
                                ? " | "
                                : "   "));

        fprintf(_fpOutput, "%s", pchFlag);
        (*pcFlags)++;
    }
    else
    {
        strncpy(ach, achBlanks, cch);
        ach[cch] = '\0';
        fprintf(_fpOutput, "   ");
        fprintf(_fpOutput, "%s", ach);
    }
}

BOOL
CNotificationParser::ReadLine(char *pchBuf, int cchBuf, int *pcchRead)
{
    int cchRead;
    
    if (!fgets(pchBuf, cchBuf, _fpInput))
        return FALSE;

    cchRead = strlen(pchBuf);
    if (!cchRead)
        return FALSE;

    if (pcchRead)
        *pcchRead = cchRead;
        
    return TRUE;
}

inline BOOL
IsDelim(char ch)
{
    return (    ch == ' '
            ||  ch == '\t'
            ||  ch == '\r'
            ||  ch == '\n'
            ||  ch == ','
            ||  ch == ':'
            ||  ch == ';');
}

void
CNotificationParser::SkipSpace(char **ppch)
{
    char *pch = *ppch;
    while (*pch && IsDelim(*pch))
        pch++;
    *ppch = pch;
}


void
CNotificationParser::SkipNonspace(char **ppch)
{
    char *pch = *ppch;
    while (*pch && !IsDelim(*pch))
        pch++;
    *ppch = pch;
}

void
CNotificationParser::ChopComment(char *pch)
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

BOOL
CNotificationParser::GetWord(char **ppch, char **ppchWord)
{
    SkipSpace(ppch);
    *ppchWord = *ppch;
    SkipNonspace(ppch);
    if (**ppch)
    {
        if (*ppch - *ppchWord > MAX_WORD)
            *ppch = *ppchWord + MAX_WORD - 1;
        **ppch = '\0';
        (*ppch)++;
    }

    return **ppchWord;
}

void
CNotificationParser::ReportError(const char * pchError, ...)
{
    char    ach[MAX_LINE];
    va_list ap;

    va_start(ap, pchError);
    vsprintf(ach, pchError, ap);
    va_end(ap);

    printf("%s(0) : error NF0000: %s\n", _pchInput, ach);
    if (_fpLog)
        fprintf(_fpLog, "%s(0) : error NF0000: %s\n", _pchInput, ach);
}

void 
CNotificationParser::CNotification::MakeSC( CNotification *pN ) 
{ 
    memcpy( this, pN, sizeof(CNotification) );

    strncat(_achType, "_2", MAX_WORD-1);

    _fSecondChanceAvail = FALSE;
    _fSecondChance = TRUE;
}

void 
CNotificationParser::CNotification::MarkFirstChance() 
{ 
    strncat(_achType, "_1", MAX_WORD-1);
}
