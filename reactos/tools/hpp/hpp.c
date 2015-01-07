/*
 * COPYRIGHT:             See COPYING in the top level directory
 * PROJECT:               Header preprocessor
 * PURPOSE:               Generates header files from other header files
 * PROGRAMMER;            Timo Kreuzer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

//#define DBG 1

#if DBG
#define trace printf
#else
#define trace if (0) printf
#endif

typedef struct _DEFINE
{
    struct _DEFINE *pNext;
    int val;
    char *pszName;
    unsigned int cchName;
    char *pszValue;
    unsigned int cchValue;
    char achBuffer[1];
} DEFINE, *PDEFINE;

DEFINE *gpDefines = 0;
int iLine;
const char *gpszCurFile;

char*
convert_path(const char* origpath)
{
    char* newpath;
    int i;

    newpath = strdup(origpath);

    i = 0;
    while (newpath[i] != 0)
    {
#ifdef UNIX_PATHS
        if (newpath[i] == '\\')
        {
            newpath[i] = '/';
        }
#else
#ifdef DOS_PATHS
        if (newpath[i] == '/')
        {
            newpath[i] = '\\';
        }
#endif
#endif
        i++;
    }
    return newpath;
}

char*
GetFolder(const char* pszFullPath)
{
    return ".";
}

void*
LoadFile(const char* pszFileName, size_t* pFileSize)
{
    FILE* file;
    void* pFileData = NULL;
    int iFileSize;

    trace("Loading file...");

    file = fopen(pszFileName, "rb");
    if (!file)
    {
        trace("Could not open file\n");
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    iFileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);
    *pFileSize = iFileSize;
    trace("ok. Size is %d\n", iFileSize);

    pFileData = malloc(iFileSize + 1);

    if (pFileData != NULL)
    {
        if (iFileSize != fread(pFileData, 1, iFileSize, file))
        {
            free(pFileData);
            pFileData = NULL;
        }
    }
    else
    {
        trace("Could not allocate memory for file\n");
    }

    fclose(file);

    return pFileData;
}


int
error(char *format, ...)
{
    va_list valist;
    int res;
    va_start(valist, format);
    res = vfprintf(stderr, format, valist);
    va_end(valist);
    return res;
}

char*
GetNextChar(const char *psz)
{
    while (*psz == ' ' || *psz == '\t') psz++;
    return (char*)psz;
}

char*
GetNextLine(char *pszLine)
{
    /* Walk to the end of the line */
    while (*pszLine != 13 && *pszLine != 10 && *pszLine != 0) pszLine++;

    /* Skip one CR/LF */
    if (pszLine[0] == 13 && pszLine[1] == 10)
        pszLine += 2;
    else if (pszLine[0] == 13 || pszLine[0] == 10)
        pszLine++;

    if (*pszLine == 0)
    {
        return 0;
    }

    return pszLine;
}

int
strxlen(const char *psz)
{
    int len = 0;
    while (isalnum(*psz) || *psz == '_')
    {
        psz++;
        len++;
    }
    return len;
}

PDEFINE
FindDefine(const char *p, char **pNext)
{
    PDEFINE pDefine;
    int cchName;

    cchName = strxlen(p);
    if (pNext)
        *pNext = (char*)p + cchName;

    /* search for the define in the global list */
    pDefine = gpDefines;
    while (pDefine != 0)
    {
        trace("found a define: %s\n", pDefine->pszName);
        if (pDefine->cchName == cchName)
        {
            if (strncmp(p, pDefine->pszName, cchName) == 0)
            {
                return pDefine;
            }
        }
        pDefine = pDefine->pNext;
    }
    return 0;
}

void
WriteLine(char *pchLine, FILE *fileOut)
{
    char *pch, *pchLineEnd, *pchVariable;
    int len;
    PDEFINE pDefine;

    pchLineEnd = strchr(pchLine, '\n');
    if (pchLineEnd == 0)
        return;

    len = pchLineEnd - pchLine + 1;

    pch = pchLine;
    while (len > 0)
    {
        /* Check if there is a $ variable in the line */
        pchVariable = strchr(pch, '$');
        if (pchVariable && (pchVariable < pchLineEnd))
        {
            /* Write all characters up to the $ */
            fwrite(pch, 1, pchVariable - pch, fileOut);

            /* Try to find the define */
            pDefine = FindDefine(pchVariable + 1, &pch);
            if (pDefine != 0)
            {
                /* We have a define, write the value */
                fwrite(pDefine->pszValue, 1, pDefine->cchValue, fileOut);
            }
            else
            {
                len = strxlen(pchVariable + 1) + 1;
                error("Could not find variable '%.*s'\n", len, pchVariable);
                fwrite(pchVariable, 1, pch - pchVariable, fileOut);
            }

            len = pchLineEnd - pch + 1;
        }
        else
        {
            fwrite(pch, 1, len, fileOut);
            break;
        }
    }
}

int
EvaluateConstant(const char *p, char **pNext)
{
    PDEFINE pDefine;

    pDefine = FindDefine(p, pNext);
    if (!pDefine)
        return 0;

    return pDefine->val;
}

int
EvaluateExpression(char *pExpression, char **pNext)
{
    char *p, *pstart;
    int inv, thisval, val = 0;

    trace("evaluating expression\n");

    pstart = GetNextChar(pExpression);
    if (*pstart != '(')
    {
        error("Parse error: expected '(' \n");
        return -1;
    }

    while (1)
    {
        /* Get the start of the real expression */
        p = pstart;
        if ((p[0] == '&' && p[1] == '&') ||
            (p[0] == '|' && p[1] == '|'))
        {
            p++;
        }
        p = GetNextChar(p + 1);

        /* Check for inversion modifier */
        if (*p == '!')
        {
            inv = 1;
            p = GetNextChar(p + 1);
        }
        else
            inv = 0;

        /* Beginning of a new subexpression? */
        if (*p == '(')
        {
            /* Evaluate subexpression */
            thisval = EvaluateExpression(p, &p);
        }
        else if (isdigit(*p))
        {
            thisval = strtod(p, &p);
            trace("found a num: %d\n", thisval);
        }
        else if (isalpha(*p) || *p == '_')
        {
            thisval = EvaluateConstant(p, &p);
        }
        else
        {
            error("..Parse error, expected '(' or constant in line %d\n",
                  iLine);
            return -1;
        }

        if (inv)
            thisval = !thisval;

        /* Check how to combine the current value */
        if (pstart[0] == '(')
        {
            val = thisval;
        }
        else if (pstart[0] == '&' && pstart[1] == '&')
        {
            val = val && thisval;
        }
        else if (pstart[0] == '&' && pstart[1] != '&')
        {
            val = val & thisval;
        }
        else if (pstart[0] == '|' && pstart[1] == '|')
        {
            trace("found || val = %d, thisval = %d\n", val, thisval);
            val = val || thisval;
        }
        else if (pstart[0] == '|' && pstart[1] != '|')
        {
            val = val | thisval;
        }
        else if (pstart[0] == '+')
        {
            val = val + thisval;
        }
        else
        {
            error("+Parse error: expected '(' or operator in Line %d, got %c\n",
                  iLine, pstart[0]);
            return -1;
        }

        p = GetNextChar(p);

        /* End of current subexpression? */
        if (*p == ')')
        {
            if (pNext)
            {
                *pNext = p + 1;
            }
            return val;
        }

        /* Continue with a new start position */
        pstart = p;
    }

    return val;
}

int
ParseInputFile(const char *pszInFile, FILE *fileOut)
{
    char* pInputData, *pCurrentLine, *p1, *p2;
    size_t cbInFileLenth;
    int iIfLevel, iCopyLevel;

    trace("parsing input file: %s\n", pszInFile);

    /* Set the global file name */
    gpszCurFile = pszInFile;

    /* Load the input file into memory */
    pInputData = LoadFile(pszInFile, &cbInFileLenth);
    if (!pInputData)
    {
        error("Could not load input file %s\n", pszInFile);
        return -1;
    }

    /* Zero terminate the file */
    pInputData[cbInFileLenth] = 0;

    pCurrentLine = pInputData;
    iLine = 1;
    iCopyLevel = iIfLevel = 0;

    /* The main processing loop */
    do
    {
        trace("line %d: ", iLine);

        /* If this is a normal line ... */
        if (pCurrentLine[0] != '$')
        {
            /* Check if we are to copy this line */
            if (iCopyLevel == iIfLevel)
            {
                trace("copying\n");
                WriteLine(pCurrentLine, fileOut);
            }
            else
                trace("skipping\n");

            /* Continue with next line */
            continue;
        }

        /* Check for $endif */
        if (strncmp(pCurrentLine, "$endif", 6) == 0)
        {
            trace("found $endif\n");
            if (iIfLevel <= 0)
            {
                error("Parse error: $endif without $if in %s:%d\n", pszInFile, iLine);
                return -1;
            }
            if (iCopyLevel == iIfLevel)
            {
                iCopyLevel--;
            }
            iIfLevel--;

            /* Continue with next line */
            continue;
        }

        /* The rest is only parsed when we are in a true block */
        if (iCopyLevel < iIfLevel)
        {
            trace("skipping\n");

            /* Continue with next line */
            continue;
        }

        /* Check for $define */
        if (strncmp(pCurrentLine, "$define", 7) == 0)
        {
            PDEFINE pDefine;
            char *pchName, *pchValue;
            size_t cchName, cchValue;

            trace("found $define\n");
            p1 = GetNextChar(pCurrentLine + 7);
            if (*p1 != '(')
            {
                error("Parse error: expected '(' at %s:%d\n",
                      pszInFile, iLine);
                return -1;
            }

            pchName = GetNextChar(p1 + 1);
            cchName = strxlen(pchName);
            p1 = GetNextChar(pchName + cchName);

            /* Check for assignment */
            if (*p1 == '=')
            {
                trace("found $define with assignment\n");
                pchValue = GetNextChar(p1 + 1);
                cchValue = strxlen(pchValue);
                p1 = GetNextChar(pchValue + cchValue);
            }
            else
            {
                pchValue = 0;
                cchValue = 0;
            }

            /* Allocate a DEFINE structure */
            pDefine = malloc(sizeof(DEFINE) + cchName + cchValue + 2);
            if (pDefine == 0)
            {
                error("Failed to allocate %u bytes\n",
                      sizeof(DEFINE) + cchName + cchValue + 2);
                return -1;
            }

            pDefine->pszName = pDefine->achBuffer;
            strncpy(pDefine->pszName, pchName, cchName);
            pDefine->pszName[cchName] = 0;
            pDefine->cchName = cchName;
            pDefine->val = 1;

            if (pchValue != 0)
            {
                pDefine->pszValue = &pDefine->achBuffer[cchName + 1];
                strncpy(pDefine->pszValue, pchValue, cchValue);
                pDefine->pszValue[cchValue] = 0;
                pDefine->cchValue = cchValue;
            }
            else
            {
                pDefine->pszValue = 0;
                pDefine->cchValue = 0;
            }

            /* Insert the new define into the global list */
            pDefine->pNext = gpDefines;
            gpDefines = pDefine;

            /* Check for closing ')' */
            if (*p1 != ')')
            {
                error("Parse error: expected ')' at %s:%d\n",
                      pszInFile, iLine);
                return -1;
            }
        }

        /* Check for $if */
        else if (strncmp(pCurrentLine, "$if", 3) == 0)
        {
            int val;

            trace("found $if\n");
            /* Increase the if-level */
            iIfLevel++;

            /* Get beginning of the expression */
            p1 = GetNextChar(pCurrentLine + 3);

            /* evaluate the expression */
            val = EvaluateExpression(p1, 0);

            if (val)
            {
                iCopyLevel = iIfLevel;
            }
            else if (val == -1)
            {
                /* Parse error */
                return -1;
            }
        }

        /* Check for $include */
        else if (strncmp(pCurrentLine, "$include", 8) == 0)
        {
            int ret;

            trace("found $include\n");
            p1 = GetNextChar(pCurrentLine + 8);
            if (*p1 != '(')
            {
                error("Parse error: expected '(' at %s:%d, found '%c'\n",
                      pszInFile, iLine, *p1);
                return -1;
            }
            p1++;
            p2 = strchr(p1, ')');
            *p2 = 0;

            /* Parse the included file */
            ret = ParseInputFile(p1, fileOut);

            /* Restore the global file name */
            gpszCurFile = pszInFile;

            /* Restore the zeroed character */
            *p2 = ')';

            if (ret == -1)
            {
                return -1;
            }
        }

        /* Check for $$ comment */
        else if (strncmp(pCurrentLine, "$$", 2) == 0)
        {
            trace("$$ ignored\n");
            /* continue with next line */
            continue;
        }

        else
        {
            trace("wot:%s\n", pCurrentLine);
        }

        /* Continue with next line */
    }
    while (pCurrentLine = GetNextLine(pCurrentLine),
           iLine++,
           pCurrentLine != 0);

    /* Free the file data */
    free(pInputData);

    trace("Done with file.\n\n");

    return 0;
}


int
main(int argc, char* argv[])
{
    char *pszInFile, *pszOutFile;
    FILE* fileOut;
    int ret;

    if (argc != 3)
    {
        error("Usage: hpp <inputfile> <outputfile>\n");
        exit(1);
    }

    pszInFile = convert_path(argv[1]);
    pszOutFile = convert_path(argv[2]);

    fileOut = fopen(pszOutFile, "wb");
    if (fileOut == NULL)
    {
        error("Cannot open output file %s", pszOutFile);
        exit(1);
    }

    ret = ParseInputFile(pszInFile, fileOut);

    fclose(fileOut);
    free(pszInFile);
    free(pszOutFile);

    return ret;
}
