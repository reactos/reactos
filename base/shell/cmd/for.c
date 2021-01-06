/*
 *  FOR.C - for internal batch command.
 *
 *
 *  History:
 *
 *    16-Jul-1998 (Hans B Pufal)
 *        Started.
 *
 *    16-Jul-1998 (John P Price)
 *        Separated commands into individual files.
 *
 *    19-Jul-1998 (Hans B Pufal)
 *        Implementation of FOR.
 *
 *    27-Jul-1998 (John P Price <linux-guru@gcfl.net>)
 *        Added config.h include.
 *
 *    20-Jan-1999 (Eric Kohl)
 *        Unicode and redirection safe!
 *
 *    01-Sep-1999 (Eric Kohl)
 *        Added help text.
 *
 *    23-Feb-2001 (Carl Nettelblad <cnettel@hem.passagen.se>)
 *        Implemented preservation of echo flag. Some other for related
 *        code in other files fixed, too.
 *
 *    28-Apr-2005 (Magnus Olsen <magnus@greatlord.com>)
 *        Remove all hardcoded strings in En.rc
 */

#include "precomp.h"

/* Enable this define for "buggy" Windows' CMD FOR-command compatibility.
 * Currently, this enables the buggy behaviour of FOR /F token parsing. */
#define MSCMD_FOR_QUIRKS


/* FOR is a special command, so this function is only used for showing help now */
INT cmd_for(LPTSTR param)
{
    TRACE("cmd_for(\'%s\')\n", debugstr_aw(param));

    if (!_tcsncmp(param, _T("/?"), 2))
    {
        ConOutResPaging(TRUE, STRING_FOR_HELP1);
        return 0;
    }

    ParseErrorEx(param);
    return 1;
}

/* The stack of current FOR contexts.
 * NULL when no FOR command is active */
PFOR_CONTEXT fc = NULL;

/* Get the next element of the FOR's list */
static BOOL GetNextElement(TCHAR **pStart, TCHAR **pEnd)
{
    TCHAR *p = *pEnd;
    BOOL InQuotes = FALSE;
    while (_istspace(*p))
        p++;
    if (!*p)
        return FALSE;
    *pStart = p;
    while (*p && (InQuotes || !_istspace(*p)))
        InQuotes ^= (*p++ == _T('"'));
    *pEnd = p;
    return TRUE;
}

/* Execute a single instance of a FOR command */
/* Just run the command (variable expansion is done in DoDelayedExpansion) */
#define RunInstance(Cmd) \
    ExecuteCommandWithEcho((Cmd)->Subcommands)

/* Check if this FOR should be terminated early */
#define Exiting(Cmd) \
    /* Someone might have removed our context */ \
    (bCtrlBreak || (fc != (Cmd)->For.Context))
/* Take also GOTO jumps into account */
#define ExitingOrGoto(Cmd) \
    (Exiting(Cmd) || (bc && bc->current == NULL))

/* Read the contents of a text file into memory,
 * dynamically allocating enough space to hold it all */
static LPTSTR ReadFileContents(FILE *InputFile, TCHAR *Buffer)
{
    SIZE_T Len = 0;
    SIZE_T AllocLen = 1000;

    LPTSTR Contents = cmd_alloc(AllocLen * sizeof(TCHAR));
    if (!Contents)
    {
        WARN("Cannot allocate memory for Contents!\n");
        return NULL;
    }

    while (_fgetts(Buffer, CMDLINE_LENGTH, InputFile))
    {
        ULONG_PTR CharsRead = _tcslen(Buffer);
        while (Len + CharsRead >= AllocLen)
        {
            LPTSTR OldContents = Contents;
            Contents = cmd_realloc(Contents, (AllocLen *= 2) * sizeof(TCHAR));
            if (!Contents)
            {
                WARN("Cannot reallocate memory for Contents!\n");
                cmd_free(OldContents);
                return NULL;
            }
        }
        _tcscpy(&Contents[Len], Buffer);
        Len += CharsRead;
    }

    Contents[Len] = _T('\0');
    return Contents;
}

/* FOR /F: Parse the contents of each file */
static INT ForF(PARSED_COMMAND *Cmd, LPTSTR List, TCHAR *Buffer)
{
    LPTSTR Delims = _T(" \t");
    PTCHAR DelimsEndPtr = NULL;
    TCHAR  DelimsEndChr = _T('\0');
    TCHAR Eol = _T(';');
    INT SkipLines = 0;
    DWORD TokensMask = (1 << 1);
#ifdef MSCMD_FOR_QUIRKS
    DWORD NumTokens = 1;
    DWORD RemainderVar = 0;
#else
    DWORD NumTokens = 0;
#endif
    TCHAR StringQuote = _T('"');
    TCHAR CommandQuote = _T('\'');
    LPTSTR Variables[32];
    PTCHAR Start, End;
    INT Ret = 0;

    if (Cmd->For.Params)
    {
        TCHAR Quote = 0;
        PTCHAR Param = Cmd->For.Params;
        if (*Param == _T('"') || *Param == _T('\''))
            Quote = *Param++;

        while (*Param && *Param != Quote)
        {
            if (*Param <= _T(' '))
            {
                Param++;
            }
            else if (_tcsnicmp(Param, _T("delims="), 7) == 0)
            {
                Param += 7;
                /*
                 * delims=xxx: Specifies the list of characters that separate tokens.
                 * This option does not cumulate: only the latest 'delims=' specification
                 * is taken into account.
                 */
                Delims = Param;
                DelimsEndPtr = NULL;
                while (*Param && *Param != Quote)
                {
                    if (*Param == _T(' '))
                    {
                        PTCHAR FirstSpace = Param;
                        Param += _tcsspn(Param, _T(" "));
                        /* Exclude trailing spaces if this is not the last parameter */
                        if (*Param && *Param != Quote)
                        {
                            /* Save where the delimiters specification string ends */
                            DelimsEndPtr = FirstSpace;
                        }
                        break;
                    }
                    Param++;
                }
                if (*Param == Quote)
                {
                    /* Save where the delimiters specification string ends */
                    DelimsEndPtr = Param++;
                }
            }
            else if (_tcsnicmp(Param, _T("eol="), 4) == 0)
            {
                Param += 4;
                /* eol=c: Lines starting with this character (may be
                 * preceded by delimiters) are skipped. */
                Eol = *Param;
                if (Eol != _T('\0'))
                    Param++;
            }
            else if (_tcsnicmp(Param, _T("skip="), 5) == 0)
            {
                /* skip=n: Number of lines to skip at the beginning of each file */
                SkipLines = _tcstol(Param + 5, &Param, 0);
                if (SkipLines <= 0)
                    goto error;
            }
            else if (_tcsnicmp(Param, _T("tokens="), 7) == 0)
            {
#ifdef MSCMD_FOR_QUIRKS
                DWORD NumToksInSpec = 0; // Number of tokens in this specification.
#endif
                Param += 7;
                /*
                 * tokens=x,y,m-n: List of token numbers (must be between 1 and 31)
                 * that will be assigned into variables. This option does not cumulate:
                 * only the latest 'tokens=' specification is taken into account.
                 *
                 * NOTE: In MSCMD_FOR_QUIRKS mode, for Windows' CMD compatibility,
                 * not all the tokens-state is reset. This leads to subtle bugs.
                 */
                TokensMask = 0;
#ifdef MSCMD_FOR_QUIRKS
                NumToksInSpec = 0;
                // Windows' CMD compatibility: bug: the asterisk-token's position is not reset!
                // RemainderVar = 0;
#else
                NumTokens = 0;
#endif

                while (*Param && *Param != Quote && *Param != _T('*'))
                {
                    INT First = _tcstol(Param, &Param, 0);
                    INT Last = First;
#ifdef MSCMD_FOR_QUIRKS
                    if (First < 1)
#else
                    if ((First < 1) || (First > 31))
#endif
                        goto error;
                    if (*Param == _T('-'))
                    {
                        /* It's a range of tokens */
                        Last = _tcstol(Param + 1, &Param, 0);
#ifdef MSCMD_FOR_QUIRKS
                        /* Ignore the range if the endpoints are not in correct order */
                        if (Last < 1)
#else
                        if ((Last < First) || (Last > 31))
#endif
                            goto error;
                    }
#ifdef MSCMD_FOR_QUIRKS
                    /* Ignore the range if the endpoints are not in correct order */
                    if ((First <= Last) && (Last <= 31))
                    {
#endif
                        TokensMask |= (2 << Last) - (1 << First);
#ifdef MSCMD_FOR_QUIRKS
                        NumToksInSpec += (Last - First + 1);
                    }
#endif

                    if (*Param != _T(','))
                        break;
                    Param++;
                }
                /* With an asterisk at the end, an additional variable
                 * will be created to hold the remainder of the line
                 * (after the last specified token). */
                if (*Param == _T('*'))
                {
#ifdef MSCMD_FOR_QUIRKS
                    RemainderVar = ++NumToksInSpec;
#else
                    ++NumTokens;
#endif
                    Param++;
                }
#ifdef MSCMD_FOR_QUIRKS
                NumTokens = max(NumTokens, NumToksInSpec);
#endif
            }
            else if (_tcsnicmp(Param, _T("useback"), 7) == 0)
            {
                Param += 7;
                /* usebackq: Use alternate quote characters */
                StringQuote = _T('\'');
                CommandQuote = _T('`');
                /* Can be written as either "useback" or "usebackq" */
                if (_totlower(*Param) == _T('q'))
                    Param++;
            }
            else
            {
            error:
                error_syntax(Param);
                return 1;
            }
        }
    }

#ifdef MSCMD_FOR_QUIRKS
    /* Windows' CMD compatibility: use the wrongly evaluated number of tokens */
    fc->varcount = NumTokens;
    /* Allocate a large enough variables array if needed */
    if (NumTokens <= ARRAYSIZE(Variables))
    {
        fc->values = Variables;
    }
    else
    {
        fc->values = cmd_alloc(fc->varcount * sizeof(*fc->values));
        if (!fc->values)
        {
            error_out_of_memory();
            return 1;
        }
    }
#else
    /* Count how many variables will be set: one for each token,
     * plus maybe one for the remainder. */
    fc->varcount = NumTokens;
    for (NumTokens = 1; NumTokens < 32; ++NumTokens)
        fc->varcount += (TokensMask >> NumTokens) & 1;
    fc->values = Variables;
#endif

    if (*List == StringQuote || *List == CommandQuote)
    {
        /* Treat the entire "list" as one single element */
        Start = List;
        End = &List[_tcslen(List)];
        goto single_element;
    }

    /* Loop over each file */
    End = List;
    while (!ExitingOrGoto(Cmd) && GetNextElement(&Start, &End))
    {
        FILE* InputFile;
        LPTSTR FullInput, In, NextLine;
        INT Skip;
    single_element:

        if (*Start == StringQuote && End[-1] == StringQuote)
        {
            /* Input given directly as a string */
            End[-1] = _T('\0');
            FullInput = cmd_dup(Start + 1);
        }
        else if (*Start == CommandQuote && End[-1] == CommandQuote)
        {
            /*
             * Read input from a command. We let the CRT do the ANSI/UNICODE conversion.
             * NOTE: Should we do that, or instead read in binary mode and
             * do the conversion by ourselves, using *OUR* current codepage??
             */
            End[-1] = _T('\0');
            InputFile = _tpopen(Start + 1, _T("r"));
            if (!InputFile)
            {
                error_bad_command(Start + 1);
                Ret = 1;
                goto Quit;
            }
            FullInput = ReadFileContents(InputFile, Buffer);
            _pclose(InputFile);
        }
        else
        {
            /* Read input from a file */
            TCHAR Temp = *End;
            *End = _T('\0');
            StripQuotes(Start);
            InputFile = _tfopen(Start, _T("r"));
            *End = Temp;
            if (!InputFile)
            {
                error_sfile_not_found(Start);
                Ret = 1;
                goto Quit;
            }
            FullInput = ReadFileContents(InputFile, Buffer);
            fclose(InputFile);
        }

        if (!FullInput)
        {
            error_out_of_memory();
            Ret = 1;
            goto Quit;
        }

        /* Patch the delimiters string */
        if (DelimsEndPtr)
        {
            DelimsEndChr = *DelimsEndPtr;
            *DelimsEndPtr = _T('\0');
        }

        /* Loop over the input line by line */
        for (In = FullInput, Skip = SkipLines;
             !ExitingOrGoto(Cmd) && (In != NULL);
             In = NextLine)
        {
            DWORD RemainingTokens = TokensMask;
            LPTSTR* CurVar = fc->values;

            ZeroMemory(fc->values, fc->varcount * sizeof(*fc->values));
#ifdef MSCMD_FOR_QUIRKS
            NumTokens = fc->varcount;
#endif

            NextLine = _tcschr(In, _T('\n'));
            if (NextLine)
                *NextLine++ = _T('\0');

            if (--Skip >= 0)
                continue;

            /* Ignore lines where the first token starts with the eol character */
            In += _tcsspn(In, Delims);
            if (*In == Eol)
                continue;

            /* Loop as long as we have not reached the end of
             * the line, and that we have tokens available.
             * A maximum of 31 tokens will be enumerated. */
            while (*In && ((RemainingTokens >>= 1) != 0))
            {
                /* Save pointer to this token in a variable if requested */
                if (RemainingTokens & 1)
                {
#ifdef MSCMD_FOR_QUIRKS
                    --NumTokens;
#endif
                    *CurVar++ = In;
                }
                /* Find end of token */
                In += _tcscspn(In, Delims);
                /* NULL-terminate it and advance to next token */
                if (*In)
                {
                    *In++ = _T('\0');
                    In += _tcsspn(In, Delims);
                }
            }

            /* Save pointer to remainder of the line if we need to do so */
            if (*In)
#ifdef MSCMD_FOR_QUIRKS
            if (RemainderVar && (fc->varcount - NumTokens + 1 == RemainderVar))
#endif
            {
                /* NOTE: This sets fc->values[0] at least, if no tokens
                 * were initialized so far, since CurVar is initialized
                 * originally to point to fc->values. */
                *CurVar = In;
            }

            /* Don't run unless we have at least one variable filled */
            if (fc->values[0])
                Ret = RunInstance(Cmd);
        }

        /* Restore the delimiters string */
        if (DelimsEndPtr)
            *DelimsEndPtr = DelimsEndChr;

        cmd_free(FullInput);
    }

Quit:
#ifdef MSCMD_FOR_QUIRKS
    if (fc->values && (fc->values != Variables))
        cmd_free(fc->values);
#endif

    return Ret;
}

/* FOR /L: Do a numeric loop */
static INT ForLoop(PARSED_COMMAND *Cmd, LPTSTR List, TCHAR *Buffer)
{
    enum { START, STEP, END };
    INT params[3] = { 0, 0, 0 };
    INT i;
    INT Ret = 0;
    TCHAR *Start, *End = List;

    for (i = 0; i < 3 && GetNextElement(&Start, &End); ++i)
        params[i] = _tcstol(Start, NULL, 0);

    i = params[START];
    /*
     * Windows' CMD compatibility:
     * Contrary to the other FOR-loops, FOR /L does not check
     * whether a GOTO has been done, and will continue to loop.
     */
    while (!Exiting(Cmd) &&
           (params[STEP] >= 0 ? (i <= params[END]) : (i >= params[END])))
    {
        _itot(i, Buffer, 10);
        Ret = RunInstance(Cmd);
        i += params[STEP];
    }

    return Ret;
}

/* Process a FOR in one directory. Stored in Buffer (up to BufPos) is a
 * string which is prefixed to each element of the list. In a normal FOR
 * it will be empty, but in FOR /R it will be the directory name. */
static INT ForDir(PARSED_COMMAND *Cmd, LPTSTR List, TCHAR *Buffer, TCHAR *BufPos)
{
    INT Ret = 0;
    TCHAR *Start, *End = List;

    while (!ExitingOrGoto(Cmd) && GetNextElement(&Start, &End))
    {
        if (BufPos + (End - Start) > &Buffer[CMDLINE_LENGTH])
            continue;
        memcpy(BufPos, Start, (End - Start) * sizeof(TCHAR));
        BufPos[End - Start] = _T('\0');

        if (_tcschr(BufPos, _T('?')) || _tcschr(BufPos, _T('*')))
        {
            WIN32_FIND_DATA w32fd;
            HANDLE hFind;
            TCHAR *FilePart;

            StripQuotes(BufPos);
            hFind = FindFirstFile(Buffer, &w32fd);
            if (hFind == INVALID_HANDLE_VALUE)
                continue;
            FilePart = _tcsrchr(BufPos, _T('\\'));
            FilePart = FilePart ? FilePart + 1 : BufPos;
            do
            {
                if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                    continue;
                if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    != !(Cmd->For.Switches & FOR_DIRS))
                    continue;
                if (_tcscmp(w32fd.cFileName, _T(".")) == 0 ||
                    _tcscmp(w32fd.cFileName, _T("..")) == 0)
                    continue;
                _tcscpy(FilePart, w32fd.cFileName);
                Ret = RunInstance(Cmd);
            } while (!ExitingOrGoto(Cmd) && FindNextFile(hFind, &w32fd));
            FindClose(hFind);
        }
        else
        {
            Ret = RunInstance(Cmd);
        }
    }
    return Ret;
}

/* FOR /R: Process a FOR in each directory of a tree, recursively */
static INT ForRecursive(PARSED_COMMAND *Cmd, LPTSTR List, TCHAR *Buffer, TCHAR *BufPos)
{
    INT Ret = 0;
    HANDLE hFind;
    WIN32_FIND_DATA w32fd;

    if (BufPos[-1] != _T('\\'))
    {
        *BufPos++ = _T('\\');
        *BufPos = _T('\0');
    }

    Ret = ForDir(Cmd, List, Buffer, BufPos);

    /* NOTE (We don't apply Windows' CMD compatibility here):
     * Windows' CMD does not check whether a GOTO has been done,
     * and will continue to loop. */
    if (ExitingOrGoto(Cmd))
        return Ret;

    _tcscpy(BufPos, _T("*"));
    hFind = FindFirstFile(Buffer, &w32fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return Ret;
    do
    {
        if (!(w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (_tcscmp(w32fd.cFileName, _T(".")) == 0 ||
            _tcscmp(w32fd.cFileName, _T("..")) == 0)
            continue;
        Ret = ForRecursive(Cmd, List, Buffer, _stpcpy(BufPos, w32fd.cFileName));

    /* NOTE (We don't apply Windows' CMD compatibility here):
     * Windows' CMD does not check whether a GOTO has been done,
     * and will continue to loop. */
    } while (!ExitingOrGoto(Cmd) && FindNextFile(hFind, &w32fd));
    FindClose(hFind);

    return Ret;
}

INT
ExecuteFor(PARSED_COMMAND *Cmd)
{
    INT Ret;
    LPTSTR List;
    PFOR_CONTEXT lpNew;
    TCHAR Buffer[CMDLINE_LENGTH]; /* Buffer to hold the variable value */
    LPTSTR BufferPtr = Buffer;

    List = DoDelayedExpansion(Cmd->For.List);
    if (!List)
        return 1;

    /* Create our FOR context */
    lpNew = cmd_alloc(sizeof(FOR_CONTEXT));
    if (!lpNew)
    {
        WARN("Cannot allocate memory for lpNew!\n");
        cmd_free(List);
        return 1;
    }
    lpNew->prev = fc;
    lpNew->firstvar = Cmd->For.Variable;
    lpNew->varcount = 1;
    lpNew->values = &BufferPtr;

    Cmd->For.Context = lpNew;
    fc = lpNew;

    /* Run the extended FOR syntax only if extensions are enabled */
    if (bEnableExtensions)
    {
        if (Cmd->For.Switches & FOR_F)
        {
            Ret = ForF(Cmd, List, Buffer);
        }
        else if (Cmd->For.Switches & FOR_LOOP)
        {
            Ret = ForLoop(Cmd, List, Buffer);
        }
        else if (Cmd->For.Switches & FOR_RECURSIVE)
        {
            DWORD Len = GetFullPathName(Cmd->For.Params ? Cmd->For.Params : _T("."),
                                        MAX_PATH, Buffer, NULL);
            Ret = ForRecursive(Cmd, List, Buffer, &Buffer[Len]);
        }
        else
        {
            Ret = ForDir(Cmd, List, Buffer, Buffer);
        }
    }
    else
    {
        Ret = ForDir(Cmd, List, Buffer, Buffer);
    }

    /* Remove our context, unless someone already did that */
    if (fc == lpNew)
        fc = lpNew->prev;

    cmd_free(lpNew);
    cmd_free(List);
    return Ret;
}

/* EOF */
