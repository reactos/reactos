#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef DBG
#define trace printf
#else
#define trace if (0) printf
#endif

typedef struct _DEFINE
{
    struct _DEFINE *pNext;
    int len;
    int val;
    char szName[1];
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
	return(newpath);
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

	file = fopen(pszFileName, "rb");
	if (file != NULL)
	{
		fseek(file, 0L, SEEK_END);
		*pFileSize = ftell(file);
		fseek(file, 0L, SEEK_SET);
		pFileData = malloc(*pFileSize);
		if (pFileData != NULL)
		{
			if (*pFileSize != fread(pFileData, 1, *pFileSize, file))
			{
				free(pFileData);
				pFileData = NULL;
			}
		}
		fclose(file);
	}
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


void
WriteLine(char *pszLine, FILE *fileOut)
{
    char * pszEnd;

    pszEnd = strchr(pszLine, '\n');
    if (pszEnd)
    {
        int len = pszEnd - pszLine + 1;
        fwrite(pszLine, 1, len, fileOut);
    }
}

int
EvaluateConstant(const char *p, char **pNext)
{
    PDEFINE pDefine;
    int len;

    len = strxlen(p);
    if (pNext)
        *pNext = (char*)p + len;

    /* search for the define in the global list */
    pDefine = gpDefines;
    while (pDefine != 0)
    {
        trace("found a define: %s\n", pDefine->szName);
        if (pDefine->len == len)
        {
            if (strncmp(p, pDefine->szName, len) == 0)
            {
                return pDefine->val;
            }
        }
        pDefine = pDefine->pNext;
    }
    return 0;
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
    size_t cbInFileLenth, len;
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

            trace("found $define\n");
            p1 = GetNextChar(pCurrentLine + 7);
            if (*p1 != '(')
            {
                error("Parse error: expected '(' at %s:%d\n", 
                      pszInFile, iLine);
                return -1;
            }
            p1 = GetNextChar(p1 + 1);
            len = strxlen(p1);
            p2 = p1 + len;
            if (*p2 != ')')
            {
                error("Parse error: expected ')' at %s:%d\n",
                      pszInFile, iLine);
                return -1;
            }

            /* Insert the new define into the global list */
            pDefine = malloc(sizeof(DEFINE) + len);
            strncpy(pDefine->szName, p1, len);
            pDefine->szName[len] = 0;
            pDefine->len = len;
            pDefine->val = 1;
            pDefine->pNext = gpDefines;
            gpDefines = pDefine;
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

    return 0;
}


int
main(int argc, char* argv[])
{
    char *pszInFolder, *pszInFile, *pszOutFile;
    FILE* fileOut;
    int ret;

    if (argc != 3)
    {
        error("Usage: hc <inputfile> <outputfile>\n");
        exit(1);
    }

    pszInFile = convert_path(argv[1]);
    pszOutFile = convert_path(argv[2]);
    pszInFolder = GetFolder(pszInFile);

    fileOut = fopen(pszOutFile, "wb");
    if (fileOut == NULL)
    {
        error("Cannot open output file %s", pszOutFile);
        exit(1);
    }

    ret = ParseInputFile(pszInFile, fileOut);

    fclose(fileOut);

    return ret;
}
