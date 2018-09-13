/************************************************************************/
/*                                                                      */
/* RCPP - Resource Compiler Pre-Processor for NT system                 */
/*                                                                      */
/* P0IO.C - Input/Output for Preprocessor                               */
/*                                                                      */
/* 27-Nov-90 w-BrianM  Update for NT from PM SDK RCPP                   */
/*                                                                      */
/************************************************************************/

#include "rc.h"


/************************************************************************/
/* Local Function Prototypes                                            */
/************************************************************************/
PWCHAR esc_sequence(PWCHAR, PWCHAR);


#define TEXT_TYPE ptext_t

/***  ASSUME : the trailing marker byte is only 1 character. ***/

#define PUSHBACK_BYTES  1

#define TRAILING_BYTES  1

#define EXTRA_BYTES             (PUSHBACK_BYTES + TRAILING_BYTES)
/*
**  here are some defines for the new handling of io buffers.
**  the buffer itself is 6k plus some extra bytes.
**  the main source file uses all 6k.
**  the first level of include files will use 4k starting 2k from the beginning.
**  the 2nd level - n level will use 2k starting 4k from the beginning.
**  this implies that some special handling may be necessary when we get
**  overlapping buffers. (unless the source file itself is < 2k
**  all the include files are < 2k and they do not nest more than 2 deep.)
**  first, the source file is read into the buffer (6k at a time).
**  at the first include file, (if the source from the parent file
**  is more than 2k chars) . . .
**              if the Current_char ptr is not pointing above the 2k boundary
**              (which is the beginning of the buffer for the include file)
**              then we pretend we've read in only 2k into the buffer and
**              place the terminator at the end of the parents 2k buffer.
**              else we pretend we've used up all chars in the parents buffer
**              so the next read for the parent will be the terminator, and
**              the buffer will get filled in the usual manner.
**  (if we're in a macro, the picture is slightly different in that we have
**  to update the 'real' source file pointer in the macro structure.)
**
**  the first nested include file is handled in a similar manner. (except
**  it starts up 4k away from the start.)
**
**  any further nesting will keep overlaying the upper 2k part.
*/
#define IO_BLOCK        (4 * 1024 + EXTRA_BYTES)

int vfCurrFileType = DFT_FILE_IS_UNKNOWN;   //- Added for 16-bit file support.

extern expansion_t Macro_expansion[];

typedef struct  s_filelist      filelist_t;
static struct s_filelist        {       /* list of input files (nested) */
    int         fl_bufsiz;      /* characters to read into the buffer */
    FILE *      fl_file;        /* FILE id */
    long        fl_lineno;      /* line number when file was pushed */
    PWCHAR      fl_name;        /* previous file text name */
    ptext_t     fl_currc;       /* ptr into our buffer for current c */
    TEXT_TYPE   fl_buffer;      /* type of buffer */
    int         fl_numread;     /* # of characters read from the file */
    int         fl_fFileType;   //- Added for 16-bit file support.
                                //- return from DetermineFileType.
    long        fl_seek;        //- Added for restart - contains seek
                                //  address of last read.
} Fstack[LIMIT_NESTED_INCLUDES];

static  FILE *Fp = NULL;
int           Findex = -1;


/************************************************************************
 * NEWINPUT - A new input file is to be opened, saving the old.
 *
 * ARGUMENTS
 *      WCHAR *newname - the name of the file
 *
 * RETURNS  - none
 *
 * SIDE EFFECTS
 *      - causes input stream to be switched
 *      - Linenumber is reset to 1
 *      - storage is allocated for the newname
 *      - Filename is set to the new name
 *
 * DESCRIPTION
 *      The file is opened, and if successful, the current input stream is saved
 *      and the stream is switched to the new file. If the newname is NULL,
 *      then stdin is taken as the new input.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
int
newinput (
    WCHAR *newname,
    int m_open
    )
{
    filelist_t *pF;
    WCHAR      *p;

    if( newname == NULL ) {
        Fp = stdin;
    }
    else {
        // Note: Always use the Ansi codepage here.  uiCodePage may have been
        // modified by a codepage pragma in the source file.

        WideCharToMultiByte (GetACP(), 0, newname, -1, chBuf, sizeof(chBuf), NULL, NULL);
        if((Fp = fopen(chBuf, "rb")) == NULL) {
           if(m_open == MUST_OPEN) {
               Msg_Temp = GET_MSG (1005);
               SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, chBuf);
               fatal(1005);
           }
           return(FALSE);
        }
    }

    /* now push it onto the file stack */
    ++Findex;
    if(Findex >= LIMIT_NESTED_INCLUDES) {
        Msg_Temp = GET_MSG (1014);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, LIMIT_NESTED_INCLUDES);
        fatal(1014);
    }
    pF = &Fstack[Findex];
    p = (WCHAR *) MyAlloc((IO_BLOCK + PUSHBACK_BYTES) * sizeof(WCHAR));
    if (!p) {
        strcpy (Msg_Text, GET_MSG (1002));
        fatal(1002);                  /* no memory */
        return 0;
    }
    pF->fl_bufsiz = IO_BLOCK;

    pF->fl_currc = Current_char;     /*  previous file's current char */
    pF->fl_lineno = Linenumber;      /*  previous file's line number  */
    pF->fl_file = Fp;                /*  the new file descriptor      */
    pF->fl_buffer = p;
    pF->fl_numread = 0;
    pF->fl_seek = 0;

    pF->fl_fFileType = DetermineFileType (Fp);

    if (pF->fl_fFileType == DFT_FILE_IS_UNKNOWN) {
        Msg_Temp = GET_MSG (4413);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, newname);
        warning (4413);
        pF->fl_fFileType = DFT_FILE_IS_8_BIT;
    }

    vfCurrFileType = pF->fl_fFileType;

    Current_char = p;
    io_eob();                                   /*  fill the buffer  */
    /*
        * Note that include filenames will live the entire compiland. This
        * puts the burden on the user with MANY include files.  Any other
        * scheme takes space out of static data.
        * Notice also, that we save the previous filename in the new file's
        * fl_name.
        */
    pF->fl_name = pstrdup(Filename);
    wcsncpy(Filebuff, newname, sizeof(Filebuff) / sizeof(WCHAR));
    Linenumber = 0;     /*  do_newline() will increment to the first line */
    if(Eflag) {
        emit_line();
        // must manually write '\r' with '\n' when writing 16-bit strings
        myfwrite(L"\r\n", 2 * sizeof(WCHAR), 1, OUTPUTFILE);  /* this line is inserted */
    }

    {
        defn_t d;
        int old_line = Linenumber;
        Linenumber = Findex;

        DEFN_IDENT(&d) = L"!";
        DEFN_TEXT(&d) = Reuse_Include;
        DEFN_NEXT(&d) = NULL;
        DEFN_NFORMALS(&d) = 0;
        DEFN_EXPANDING(&d) = FALSE;
        AfxOutputMacroDefn(&d);

        if (Findex > 0) {
            DEFN_IDENT(&d) = L"$";
            DEFN_TEXT(&d) = Filename;
            DEFN_NEXT(&d) = NULL;
            DEFN_NFORMALS(&d) = 0;
            DEFN_EXPANDING(&d) = FALSE;
            AfxOutputMacroDefn(&d);
        }

        Linenumber = old_line;
    }

    do_newline();       /*  a new file may have preproc cmd as first line  */
    return(TRUE);
}


/************************************************************************
 * FPOP - pop to a previous level of input stream
 *
 * ARGUMENTS - none
 *
 * RETURNS
 *      TRUE if successful, FALSE if the stack is empty
 *
 * SIDE EFFECTS
 *      - Linenumber is restored to the old files line number
 *      - Filename is reset to the old filename
 *  - frees storage allocated for filename
 *
 * DESCRIPTION
 *      Pop the top of the file stack, restoring the previous input stream.
 *
 * AUTHOR - Ralph Ryan, Sept. 9, 1982
 *
 * MODIFICATIONS - none
 *
 ************************************************************************/
WCHAR
fpop(
    void
    )
{
    int     OldLine;
    defn_t  DefType;

    if(Findex == -1) {          /* no files left */
        return(FALSE);
    }

    if (Fp)
        fclose(Fp);

    OldLine = Linenumber;

    --Findex;
    Linenumber = Findex;

    DEFN_IDENT(&DefType) = L"!";
    DEFN_TEXT(&DefType) = L"";
    DEFN_NEXT(&DefType) = NULL;
    DEFN_NFORMALS(&DefType) = 0;
    DEFN_EXPANDING(&DefType) = FALSE;
    AfxOutputMacroDefn(&DefType);
    Findex++;
    Linenumber = OldLine;

    strappend(Filebuff,Fstack[Findex].fl_name);
    OldLine = Linenumber;
    Linenumber = (int)Fstack[Findex].fl_lineno;
    Current_char = Fstack[Findex].fl_currc;
    MyFree(Fstack[Findex].fl_buffer);
    if(--Findex < 0) {                  /* popped the last file */
        Linenumber = OldLine;
        return(FALSE);
    }
    Fp = Fstack[Findex].fl_file;
    vfCurrFileType = Fstack[Findex].fl_fFileType;
    if(Eflag) {
        // If the last file didn't end in a \r\n, the #line from emit_line could
        // end up in whatever data structure it ended in... Emit a dummy newline
        // just in case.
        myfwrite(L"\r\n", 2 * sizeof(WCHAR), 1, OUTPUTFILE);  /* this line is inserted */
        emit_line();
    }
    return(TRUE);
}


/************************************************************************
**  nested_include : searches the parentage list of the currently
**              open files on the stack when a new include file is found.
**              Input : ptr to include file name.
**              Output : TRUE if the file was found, FALSE if not.
*************************************************************************/
int
nested_include(
    void
    )
{
    PWCHAR      p_tmp1;
    PWCHAR      p_file;
    PWCHAR      p_slash;
    int         tos;

    tos = Findex;
    p_file = Filename;          /* always start with the current file */
    for(;;) {
        p_tmp1 = p_file;
        p_slash = NULL;
        while(*p_tmp1) {        /* pt to end of filename, find trailing slash */
            if(wcschr(Path_chars, *p_tmp1)) {
                p_slash = p_tmp1;
            }
            p_tmp1++;
        }
        if(p_slash) {
            p_tmp1 = Reuse_W;
            while(p_file <= p_slash) {  /*  we want the trailing '/'  */
                *p_tmp1++ = *p_file++;  /*  copy the parent directory  */
            }
            p_file = yylval.yy_string.str_ptr;
            while((*p_tmp1++ = *p_file++)!=0) {  /*append include file name  */
                ;       /*  NULL  */
            }
        } else {
            wcscpy(Reuse_W, yylval.yy_string.str_ptr);
        }
        if(newinput(Reuse_W,MAY_OPEN)) {
            return(TRUE);
        }
        if(tos <= 0) {
            break;
        }
        p_file = Fstack[tos--].fl_name;
    }
    return(FALSE);
}


/************************************************************************/
/* esc_sequence()                                                       */
/************************************************************************/
PWCHAR
esc_sequence(
    PWCHAR dest,
    PWCHAR name
    )
{
    *dest = L'"';
    while((*++dest = *name) != 0) {
        if (CHARMAP(*name) == LX_EOS) {
            *++dest = L'\\';
        }
        name++;
    }
    *dest++ = L'"';              /* overwrite null */
    return( dest );
}


/************************************************************************/
/* emit_line()                                                          */
/************************************************************************/
void
emit_line(
    void
    )
{
    PWCHAR   p;

    swprintf(Reuse_W, L"#line %d ", Linenumber+1);
    myfwrite(Reuse_W, wcslen(Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);

    p = esc_sequence(Reuse_W, Filename);
    myfwrite(Reuse_W, (size_t)(p - Reuse_W) * sizeof(WCHAR), 1, OUTPUTFILE);
}

/************************************************************************
**  io_eob : handle getting the next block from a file.
**  return TRUE if this is the real end of the buffer, FALSE if we have
**  more to do.
************************************************************************/
int
io_eob(
    void
    )
{
    int         n;
    TEXT_TYPE   p;

    static int   dc;

    p = Fstack[Findex].fl_buffer;
    if((Current_char - (ptext_t)p) < Fstack[Findex].fl_numread) {
        /*
        **  haven't used all the chars from the buffer yet.
        **  (some clown has a null/cntl z embedded in his source file.)
        */
        if(PREVCH() == CONTROL_Z) {     /* imbedded control z, real eof */
            UNGETCH();
            return(TRUE);
        }
        return(FALSE);
    }
    Current_char = p;

    //-
    //- The following section was added to support 16-bit resource files.
    //- It will just convert them to 8-bit files that the Resource Compiler
    //- can read.  Here is the basic strategy used.  An 8-bit file is
    //- read into the normal buffer and should be processed the old way.
    //- A 16-bit file is read into a wide character buffer identical to the
    //- normal 8-bit one.  The entire contents are then copied to the 8-bit
    //- buffer and processed normally.  The one exception to this is when
    //- a string literal is encountered.  We then return to the 16-bit buffer
    //- to read the characters.  These characters are written as backslashed
    //- escape characters inside an 8-bit string.  (ex. "\x004c\x523f").
    //- I'll be the first person to admit that this is an ugly solution, but
    //- hey, we're Microsoft :-).  8-2-91 David Marsyla.
    //-
    if (Fstack[Findex].fl_fFileType == DFT_FILE_IS_8_BIT) {
        REG int     i;
        REG PUCHAR  lpb;
        PUCHAR      Buf;

        Buf = (PUCHAR) MyAlloc(Fstack[Findex].fl_bufsiz + 1);
        if (Buf == NULL) {
            strcpy (Msg_Text, GET_MSG (1002));
            fatal(1002);                  /* no memory */
        }
        Fstack[Findex].fl_seek = fseek(Fp, 0, SEEK_CUR);
        n = fread (Buf, sizeof(char), Fstack[Findex].fl_bufsiz, Fp);

        //-
        //- Determine if the last byte is a DBCS lead byte
        //-     if YES (i will be greater than n), backup one byte
        //-
        for (i = 0, lpb = Buf; i < n; i++, lpb++) {
            if (IsDBCSLeadByteEx(uiCodePage, *lpb)) {
                i++;
                lpb++;
            }
        }
        if (i > n) {
            fseek (Fp, -1, SEEK_CUR);
            n--;
            *(Buf + n) = 0;
        }

        //-
        //- Convert the 8-bit buffer to the 16-bit buffer.
        //-
        Fstack[Findex].fl_numread = MultiByteToWideChar (uiCodePage, MB_PRECOMPOSED,
                                          (LPCSTR) Buf, n, p, Fstack[Findex].fl_bufsiz);
        MyFree (Buf);
    } else {

        Fstack[Findex].fl_numread = n =
            fread (p, sizeof(WCHAR), Fstack[Findex].fl_bufsiz, Fp);

        //-
        //- If the file is in reversed format, swap the bytes.
        //-
        if (Fstack[Findex].fl_fFileType == DFT_FILE_IS_16_BIT_REV && n > 0) {
            WCHAR  *pT = p;
            BYTE  jLowNibble;
            BYTE  jHighNibble;
            INT   cNumWords = n;

            while (cNumWords--) {
                jLowNibble = (BYTE)(*pT & 0xFF);
                jHighNibble = (BYTE)((*pT >> 8) & 0xFF);

                *pT++ = (WCHAR)(jHighNibble | (jLowNibble << 8));
            }
        }
    }

    /*
    **  the total read counts the total read *and* used.
    */

    if (n != 0) {                               /* we read something */
        *(p + Fstack[Findex].fl_numread) = EOS_CHAR;    /* sentinal at the end */
        return(FALSE);                          /* more to do */
    }
    *p = EOS_CHAR;                              /* read no chars */
    return(TRUE);                               /* real end of buffer */
}


/************************************************************************
** io_restart : restarts the current file with a new codepage
**  Method: figure out where the current character came from
**      using WideCharToMultiByte(...cch up to current char...)
**      Note that this assumes that roundtrip conversion to/from
**      Unicode results in the same # of characters out as in.
**      fseek to the right place, then read a new buffer
**
**      Note that uiCodePage controls the seek, so it must
**      remain set to the value used to do the translation
**      from multi-byte to Unicode until after io_restart returns.
**
************************************************************************/
int
io_restart(
    unsigned long cp
    )
{
    int         n;
    TEXT_TYPE   p;

    // If it's a Unicode file, nothing to do, so just return.
    if (Fstack[Findex].fl_fFileType != DFT_FILE_IS_8_BIT)
        return TRUE;

    p = Fstack[Findex].fl_buffer;
    n = Fstack[Findex].fl_numread - (int)(Current_char - p);

    if (n != 0) {
        if (Fstack[Findex].fl_fFileType == DFT_FILE_IS_8_BIT) {
            n = WideCharToMultiByte(uiCodePage, 0, Current_char, n, NULL, 0, NULL, NULL);
            if (n == 0)
                return TRUE;
        } else
            n *= sizeof(WCHAR);

        fseek(Fp, -n, SEEK_CUR);
    }
    Fstack[Findex].fl_numread = 0;
    // io_eob will return true if we're at the end of the file.
    // this is an error for restart (it means there's nothing more
    // to do here (ie: #pragma codepage is the last line in the file).
    return !io_eob();
}


/************************************************************************
**  p0_init : inits for prepocessing.
**              Input : ptr to file name to use as input.
**                      ptr to LIST containing predefined values.
**                                       ( -D's from cmd line )
**
**  Note : if "newinput" cannot open the file,
**                it gives a fatal msg and exits.
**
************************************************************************/
void
p0_init(
    WCHAR *p_fname,
    WCHAR *p_outname,
    LIST *p_defns,
    LIST *p_undefns
    )
{
    REG WCHAR  *p_dstr;
    REG WCHAR  *p_eq;
    int         ntop;

    SETCHARMAP(LX_FORMALMARK, LX_MACFORMAL);
    SETCHARMAP(LX_FORMALSTR, LX_STRFORMAL);
    SETCHARMAP(LX_FORMALCHAR, LX_CHARFORMAL);
    SETCHARMAP(LX_NOEXPANDMARK, LX_NOEXPAND);
    if(EXTENSION) {
        /*
        **      '$' is an identifier character under extensions.
        */
        SETCHARMAP(L'$', LX_ID);
        SETCONTMAP(L'$', LXC_ID);
    }

    for(ntop = p_defns->li_top; ntop < MAXLIST; ++ntop) {
        p_dstr = p_defns->li_defns[ntop];
        p_eq = Reuse_W;
        while ((*p_eq = *p_dstr++) != 0)  {  /* copy the name to Reuse_W */
            if(*p_eq == L'=') {     /* we're told what the value is */
                break;
            }
            p_eq++;
        }
        if(*p_eq == L'=') {
            WCHAR      *p_tmp;
            WCHAR      *last_space = NULL;

            *p_eq = L'\0';               /* null the = */
            for(p_tmp = p_dstr; *p_tmp; p_tmp++) {      /* find the end of it */
                if(iswspace(*p_tmp)) {
                    if(last_space == NULL) {
                        last_space = p_tmp;
                    }
                } else {
                    last_space = NULL;
                }
            }
            if(last_space != NULL) {
                *last_space = L'\0';
            }
            Reuse_W_hash = local_c_hash(Reuse_W);
            Reuse_W_length = wcslen(Reuse_W) + 1;
            if( *p_dstr ) {     /* non-empty string */
                definstall(p_dstr, (wcslen(p_dstr) + 2), FROM_COMMAND);
            } else {
                definstall((WCHAR *)0, 0, 0);
            }
        } else {
            Reuse_W_hash = local_c_hash(Reuse_W);
            Reuse_W_length = wcslen(Reuse_W) + 1;
            definstall(L"1\000", 3, FROM_COMMAND);   /* value of string is 1 */
        }
    }

    /* undefine */
    for(ntop = p_undefns->li_top; ntop < MAXLIST; ++ntop) {
        p_dstr = p_undefns->li_defns[ntop];
        p_eq = Reuse_W;
        while ((*p_eq = *p_dstr++) != 0)  {  /* copy the name to Reuse_W */
            if(*p_eq == L'=') {     /* we're told what the value is */
                break;
            }
            p_eq++;
        }
        if(*p_eq == L'=') {
            WCHAR      *p_tmp;
            WCHAR      *last_space = NULL;

            *p_eq = L'\0';               /* null the = */
            for(p_tmp = p_dstr; *p_tmp; p_tmp++) {      /* find the end of it */
                if(iswspace(*p_tmp)) {
                    if(last_space == NULL) {
                        last_space = p_tmp;
                    }
                } else {
                    last_space = NULL;
                }
            }
            if(last_space != NULL) {
                *last_space = L'\0';
            }
            Reuse_W_hash = local_c_hash(Reuse_W);
            Reuse_W_length = wcslen(Reuse_W) + 1;
            if( *p_dstr ) {     /* non-empty string */
                undefine();
            } else {
                undefine();
            }
        } else {
            Reuse_W_hash = local_c_hash(Reuse_W);
            Reuse_W_length = wcslen(Reuse_W) + 1;
            undefine();   /* value of string is 1 */
        }
    }

    // Note: Always use the Ansi codepage here.  uiCodePage may have been
    // modified by a codepage pragma in the source file.

    WideCharToMultiByte (GetACP(), 0, p_outname, -1, chBuf, sizeof(chBuf), NULL, NULL);
    if ((OUTPUTFILE = fopen (chBuf, "w+b")) == NULL) {
        Msg_Temp = GET_MSG (1023);
        SET_MSG (Msg_Text, sizeof(Msg_Text), Msg_Temp, chBuf);
        fatal (1023);
    }

    newinput(p_fname,MUST_OPEN);
}

/************************************************************************
**  p0_terminate : terminates prepocessing.
**
**
************************************************************************/
void
p0_terminate(
    void
    )
{
    for ( ;fpop(); )
        ;
    if (OUTPUTFILE)
        fclose(OUTPUTFILE);
}
