/* $Id: mksystab.c,v 1.1 2003/01/05 18:08:11 robd Exp $
 *
 * PROJECT    : ReactOS / POSIX+ Subsystem
 * DESCRIPTION: Build the system calls table for
 * DESCRIPTION: the POSIX+ LPC server process.
 * NOTE       : this code is supposed to be portable.
 * AUTHOR     : Emanuele Aliberti
 * DATE       : 2001-05-26
 * REVISIONS
 *    2002-03-19 EA added stub file generation
 *    2002-04-06 EA added to the CVS repository
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PARSER_CONTEXT_LINE_SIZE      1024
#define PARSER_CONTEXT_INTERFACE_SIZE 64

const char * myname = "mksystab";

const char * syscall_name_prefix = "syscall_";
const char * proxy_name_prefix = "psxss_";

typedef enum {
    METHOD_SUCCESS,
    METHOD_EOF,
    METHOD_FAILURE
} METHOD_TYPE;

typedef struct _PARSER_CONTEXT
{
    int line_number;
    int id;
    char line [PARSER_CONTEXT_LINE_SIZE];
    char status;
    char interface [PARSER_CONTEXT_INTERFACE_SIZE];
    int argc;

} PARSER_CONTEXT, * PPARSER_CONTEXT;

typedef struct _MFILE
{
    char * name;
    FILE * fp;
    char * fopen_mode;
    METHOD_TYPE (*prologue)(int,PPARSER_CONTEXT);
    METHOD_TYPE (*iter)(int,PPARSER_CONTEXT);    
    METHOD_TYPE (*epilog)(int,PPARSER_CONTEXT);    

} MFILE, * PMFILE;

/* MFILE file table */

METHOD_TYPE db_prologue (int self, PPARSER_CONTEXT context);
METHOD_TYPE db_iter (int self, PPARSER_CONTEXT context);
METHOD_TYPE db_epilog (int self, PPARSER_CONTEXT context);

METHOD_TYPE systab_prologue (int self, PPARSER_CONTEXT context);
METHOD_TYPE systab_iter (int self, PPARSER_CONTEXT context);
METHOD_TYPE systab_epilog (int self, PPARSER_CONTEXT context);

METHOD_TYPE psx_include_prologue (int self, PPARSER_CONTEXT context);
METHOD_TYPE psx_include_iter (int self, PPARSER_CONTEXT context);
METHOD_TYPE psx_include_epilog (int self, PPARSER_CONTEXT context);

METHOD_TYPE server_include_prologue (int self, PPARSER_CONTEXT context);
METHOD_TYPE server_include_iter (int self, PPARSER_CONTEXT context);
METHOD_TYPE server_include_epilog (int self, PPARSER_CONTEXT context);

METHOD_TYPE stubs_prologue (int self, PPARSER_CONTEXT context);
METHOD_TYPE stubs_iter (int self, PPARSER_CONTEXT context);
METHOD_TYPE stubs_epilog (int self, PPARSER_CONTEXT context);


MFILE mf [] =
{ 
  { NULL, NULL, "r", db_prologue,             db_iter,             db_epilog }, /* it must be 1st */
  { NULL, NULL, "w", systab_prologue,         systab_iter,         systab_epilog },
  { NULL, NULL, "w", server_include_prologue, server_include_iter, server_include_epilog },
  { NULL, NULL, "w", psx_include_prologue,    psx_include_iter,    psx_include_epilog },
  { NULL, NULL, "w", stubs_prologue,          stubs_iter,          stubs_epilog }
};


/* mf objects methods */

int mf_open (int index)
{
    mf [index].fp = fopen (mf [index].name, mf [index].fopen_mode);
    if (NULL == mf [index].fp)
    {
        fprintf (stderr, "%s: error %d while opening \"%s\".", myname, errno, mf [index].name);
        return METHOD_FAILURE;
    }
    return METHOD_SUCCESS;
}

void mf_close (int index)
{
    fclose (mf[index].fp);
}

/* db file methods */

METHOD_TYPE db_prologue (int self, PPARSER_CONTEXT context)
{
    if (METHOD_FAILURE == mf_open (self))
    {
        return METHOD_FAILURE;
    }
    fprintf (stderr, "Processing \"%s\"...\n", mf [self].name);
    return METHOD_SUCCESS;
}

METHOD_TYPE db_iter (int self, PPARSER_CONTEXT context)
{
    char * eol;

    do 
    {
        if (feof(mf [self].fp))
        {
            return METHOD_EOF;
        }
        if (NULL == fgets (context->line, PARSER_CONTEXT_LINE_SIZE, mf [self].fp))
        {
            return METHOD_EOF;
        }
        ++ context->line_number;
        eol = strchr(context->line, '\n');
        if (eol)
        {
            *eol = '\0';
        }
        /* Is line empty or a comment? */
    } while (0 == strlen (context->line) || context->line[0] == '#');
    /* Line is not a comment nor an empty line */
    if (3 != sscanf (context->line, "%c%s%d", & context->status, context->interface, & context->argc))
    {
        fprintf (stderr, "Syntax error at line %d.\n", context->line_number);
        return METHOD_FAILURE;
    }
    return METHOD_SUCCESS;
}

METHOD_TYPE db_epilog (int self, PPARSER_CONTEXT context)
{
    mf_close (self);
    return METHOD_SUCCESS;
}

/* systab file methods */

METHOD_TYPE systab_prologue (int self, PPARSER_CONTEXT context)
{
    if (METHOD_FAILURE == mf_open (self))
    {
        return METHOD_FAILURE;
    }
    fprintf (mf[self].fp, "/* POSIX+ system calls (machine generated: do not edit!) */\n");
    fprintf (mf[self].fp, "#include <psxss.h>\n");
    fprintf (mf[self].fp, "#include <syscall.h>\n");
    fprintf (mf[self].fp, "PSX_SYSTEM_CALL SystemCall [] =\n");
    fprintf (mf[self].fp, "{\n");
    return METHOD_SUCCESS;
}

METHOD_TYPE systab_iter (int self, PPARSER_CONTEXT context)
{
    switch (context->status)
    {
    case '+':
    case '-': /* unimplemented interface */
        fprintf (mf[self].fp, "(void*)%s%s,\n", syscall_name_prefix, context->interface);
        break;
    default:
        fprintf (stderr, "%s: unknown interface status \"%c\" at line %d.\n",
            myname, context->status, context->line_number);
        return METHOD_FAILURE;
    }
    return METHOD_SUCCESS;
}

METHOD_TYPE systab_epilog (int self, PPARSER_CONTEXT context)
{
    fprintf (mf[self].fp,  "0\n};\n");
    fputs ("/* EOF */", mf[self].fp);
    return METHOD_SUCCESS;
}


/* server/include file methods */

METHOD_TYPE server_include_prologue (int self, PPARSER_CONTEXT context)
{
    if (METHOD_FAILURE == mf_open (self))
    {
        return METHOD_FAILURE;
    }
    fprintf (mf[self].fp, "/* POSIX+ system calls (machine generated: do not edit!) */\n");
    fprintf (mf[self].fp, "#ifndef _SERVER_SYSCALL_H\n");
    fprintf (mf[self].fp, "#define _SERVER_SYSCALL_H\n");
    return METHOD_SUCCESS;
}

METHOD_TYPE server_include_iter (int self, PPARSER_CONTEXT context)
{
    char interface [PARSER_CONTEXT_INTERFACE_SIZE*2];

    sprintf (interface, "%s%s", syscall_name_prefix, context->interface);
    fprintf (mf[self].fp, "NTSTATUS STDCALL %s (PPSX_MAX_MESSAGE);\n", interface);

    return METHOD_SUCCESS;
}

METHOD_TYPE server_include_epilog (int self, PPARSER_CONTEXT context)
{
    fprintf (mf[self].fp, "#endif /* ndef _SERVER_SYSCALL_H */\n");
    fputs ("/* EOF */", mf[self].fp);
    return METHOD_SUCCESS;
}


/* psx/include file methods */

METHOD_TYPE psx_include_prologue (int self, PPARSER_CONTEXT context)
{
    if (METHOD_FAILURE == mf_open (self))
    {
        return METHOD_FAILURE;
    }
    fprintf (mf[self].fp, "/* POSIX+ system calls (machine generated: do not edit!) */\n");
    fprintf (mf[self].fp, "#ifndef _PSX_SYSCALL_H\n");
    fprintf (mf[self].fp, "#define _PSX_SYSCALL_H\n");
    return METHOD_SUCCESS;
}

METHOD_TYPE psx_include_iter (int self, PPARSER_CONTEXT context)
{
    char interface [PARSER_CONTEXT_INTERFACE_SIZE*2];

    sprintf (interface, "%s%s", proxy_name_prefix, context->interface);
    fprintf (mf[self].fp, "#define %s %d\n", strupr(interface), context->id ++);

    return METHOD_SUCCESS;
}

METHOD_TYPE psx_include_epilog (int self, PPARSER_CONTEXT context)
{
    fprintf (mf[self].fp, "#define PSX_SYSCALL_APIPORT_COUNT %d\n", context->id ++);
    fprintf (mf[self].fp, "#endif /* ndef _PSX_SYSCALL_H */\n");
    fputs ("/* EOF */", mf[self].fp);
    return METHOD_SUCCESS;
}


/* stubs file methods */

METHOD_TYPE stubs_prologue (int self, PPARSER_CONTEXT context)
{
    if (METHOD_FAILURE == mf_open (self))
    {
        return METHOD_FAILURE;
    }
    fprintf( mf[self].fp,
        "/* POSIX+ system calls not yet implemented */\n"
        "/* (machine generated: do not edit!) */\n"
        "#include <psxss.h>\n");
    return METHOD_SUCCESS;
}

METHOD_TYPE stubs_iter (int self, PPARSER_CONTEXT context)
{
    if ('-' == context->status)
    {
        fprintf (
            mf[self].fp,
            "NTSTATUS STDCALL %s%s(PPSX_MAX_MESSAGE Msg){Msg->PsxHeader.Status=STATUS_NOT_IMPLEMENTED;return(STATUS_SUCCESS);}\n",
            syscall_name_prefix,
            context->interface
            );
    }
    return METHOD_SUCCESS;
}

METHOD_TYPE stubs_epilog (int self, PPARSER_CONTEXT context)
{
    fputs ("/* EOF */", mf[self].fp);
    return METHOD_SUCCESS;
}


/* main loop */

METHOD_TYPE mksystab ()
{
    int index;
    int index_top = (sizeof mf / sizeof mf[0]);
    int iterate = 1;
    PARSER_CONTEXT context;
    METHOD_TYPE mt;

    /* initialize the parser's context */
    context.line_number = 0;
    context.id = 0;

    /* prologue */
    for (index = 0; index < index_top; index ++)
    {
        if (METHOD_FAILURE == mf[index].prologue (index, & context))
        {
            return METHOD_FAILURE;
        }
    }
    /* iter */
    while (iterate)
    {
        for (index = 0; index < index_top; index ++)
        {
            mt = mf[index].iter (index, & context);
            if (METHOD_EOF == mt)
            {
                if (0 == index) /* input MUST be 1st MFILE */
                {
                    iterate = 0;
                    break; /* input reached EOF */
                }
                return METHOD_FAILURE;
            }
            else if (METHOD_FAILURE == mt)
            {
                return METHOD_FAILURE;
            }
        }
    }
    /* epilog */
    for (index = 0; index < index_top; index ++)
    {
        if (METHOD_FAILURE == mf[index].epilog (index, & context))
        {
            return METHOD_FAILURE;
        }
    }

    /* done */
    return METHOD_SUCCESS;
}

/* entry point */

int main (int argc, char **argv)
{
    int status = 0;
    int index;
   
    /* Check user parameters */ 
    if ((1 + (sizeof mf / sizeof (MFILE))) != argc)
    {
        printf ("ReactOS Operating System - POSIX+ Environment Subsystem\n");
        printf ("Build the system calls table of the POSIX+ server.\n\n");
        printf ("usage: %s syscall.db syscall.c syscall.h syscall.h stubs.c\n", argv[0]);
        exit (METHOD_FAILURE);
    }
    /* initialize descriptors */
    for (index = 0; index < (sizeof mf / sizeof mf[0]); index ++)
    {      
        mf [index].name = argv [index + 1];
    }

    /* do process them */
    status = mksystab ();

    return (status);
}


/* EOF */
