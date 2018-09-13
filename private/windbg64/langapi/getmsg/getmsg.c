// getmsg.c

#include <io.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <mbctype.h>
#include <string.h>
#include <getmsg.h>

// Local helper functions
static char * get_msg(int num, char *pbuf, int len);
static char * get_message(register char *p);
static char * get_int(register char *, int *);
static long nearest(int num);
static void mark_pos(long pos, int num);
void __cdecl CleanUp(void);


// TODO: Make these local?
static FILE *phErrorFile = NULL;
static char *pErrorFilename = NULL;

char  *
get_err(int msg_num)
{
    static char  Errbuff[1024];

    if (!phErrorFile)
    {
        /* try to open the error file */
        if (!pErrorFilename || (phErrorFile = fopen(pErrorFilename, "r")) == NULL)
        {
            phErrorFile = (FILE *)-1;

            return("");
        }
        else
        {
            // File Opened OK, install the clean up routine to
            // close file & deallocate memory used by the filename
            atexit(CleanUp);
        }
    }
    else if (phErrorFile == (FILE *)-1)
    {
        return("");
    }
    fseek(phErrorFile, (long)nearest(msg_num), 0);
    return(get_msg(msg_num, Errbuff, sizeof(Errbuff)));
} // get_err

//
// get_msg
//
static char *
get_msg(int num, char *pbuf, int len)
{
    int val;
    long pos;
    register char *p;

    for (;;)
    {
        pos = ftell(phErrorFile);
        p = pbuf;
        if (fgets(p, len, phErrorFile) == NULL)
        {
            // Reaches end-of-file, can't find the message, so just return
            // an empty string.
            return "";
        }
        p = get_int(p, &val);
        if (val == num || ((val % 1000) == 999))
        {
            // if val % 1000 == 999, then we have reached the unknown error
            // message, don't put it in the error message table in this case
            if (val == num)
            {
                mark_pos(pos, num);
            }
            else if ((val / 1000) != (num / 1000))
            {
                // found UNKNOWN msg, but wrong error class - continue
                continue;
            }
            return(get_message(p));
        } // if (val == num ...)
    } // for

    // When all else failed, returns an empty string
    return "";
} // get_msg()

//
// get_message
//
static char *
get_message(register char *p)
{
    char *retp;
    register char *cp;

    while (*p++ != '"')
        ;
    retp = cp = p;
    while (*p != '"')
    {
        if (_ismbblead(*p))
        {
            *cp++ = *p++;
        }
        else if (*p == '\\')
        {
            p++;
            if (*p == 'n')
                *p = '\n';
            else if (*p == 't')
                *p = '\t';
            else if (*p == '\"')
                *p = '\"';
            else
                *cp++ = '\\';
        } // else
        *cp++ = *p++;
    } // while 
    *cp = '\0';
    return(retp);
}

//
// get_int
//
static char *
get_int(register char *p, int *pval)
{
    register int val;

    val = 0;
    while (*p && isdigit(*p))
    {
        val = val * 10 + (*p - '0');
        p++;
    } // while
    *pval = val;
    return(p);
} // get_int()

//
// SetErrorFile
//
int
SetErrorFile(char *pFilename, char *pExeName, int bSearchExePath)
{
    char szPath[_MAX_PATH];
    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFilename[_MAX_FNAME];
    char szExtension[_MAX_EXT];

    if (!bSearchExePath)
    {
        if (0 == _access(pFilename, 0))
        {
            pErrorFilename = _strdup(pFilename);
            return 0;
        }
        return 1;
    }

    // Find in the executable's directory
    _splitpath(pExeName, szDrive, szDir, NULL, NULL);
    _splitpath(pFilename, NULL, NULL,  szFilename, szExtension);
    _makepath(szPath, szDrive, szDir, szFilename, szExtension);

    if (0 == _access(szPath, 0))
    {
        pErrorFilename = _strdup(szPath);
        return 0;
    }

    // Still not found, find in current directory
    _makepath(szPath, NULL, NULL, szFilename, szExtension);
    if (0 == _access(szPath, 0))
    {
        pErrorFilename = _strdup(szPath);
        return 0;
    }

    // We can't find the error message file, so display an error
    // message in English.  This error message must be embedded
    // in the .EXE, otherwise, what's the point?
    fprintf(stderr,
        "WARNING:  missing %s; "
        "displaying error numbers without messages.\n",
        pFilename);

    return 1;
}

//
// CleanUp
//
static void __cdecl
CleanUp(void)
{
    if (phErrorFile)
        fclose(phErrorFile);
    if (pErrorFilename)
        free(pErrorFilename);
}

//
// Error message cache
//
#define MAX_MSGLOCS 20

static struct ErrInfo
{
    int num;        // Message number
    long pos;       // File position
} Err_Info[MAX_MSGLOCS] = {0};

//
// mark_pos
//
static void
mark_pos(long pos, int num)
{
    register int i;

    for (i = 0; i < MAX_MSGLOCS && Err_Info[i].num != 0; i++)
    {
        if (Err_Info[i].num == num)
            return;     /* already in table */
    } // for
    if (i < MAX_MSGLOCS)
    {
        /* save location of this message */
        Err_Info[i].num = num;
        Err_Info[i].pos = pos;
    } // if
    return;
} // mark_pos()

/*
** Try to position the error file close to previously located
** messages.  This assumes that the message numbers in the error
** file are in ascending order within each category.
*/
static long
nearest(int num)
{
    register int i,n;
    int diff, ix;

    diff = 32767;
    ix = -1;
    for (i = 0; ((i < MAX_MSGLOCS) && ((n = Err_Info[i].num) != 0)); i++)
    {
        n = num - n;
        if (n == 0)
        {
            ix = i;
            break;
        }
        else if ((n > 0) && (n < diff) && (num / 1000) == (Err_Info[i].num / 1000))
        {
            diff = n;
            ix = i;
        }
    } // for
    return((ix >= 0) ? Err_Info[ix].pos : 0L);
} // nearest()

#ifdef TEST

int
main(int argc, char *argv[])
{
    char *pMessage;
    int nErrorNumber;

    if (argc < 3)
    {
        fprintf(stderr, "USAGE: GETMSG <MsgFilename> <MsgNumber>...\n\n");
        return 1;
    }

    SetErrorFile(*(argv+1), *argv, 1);
    argc--; argv++;
    while (--argc)
    {
        nErrorNumber = atoi(*++argv);
        pMessage = get_err(nErrorNumber);
        printf("%d: [%s]\n", nErrorNumber, pMessage);
    }
    return 0;
} // main()

#endif // TEST
