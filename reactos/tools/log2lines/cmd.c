/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Cli for escape commands
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"
#include "cmd.h"
#include "options.h"
#include "log2lines.h"
#include "help.h"

/* When you edit the cmd line and/or use the history instead of just typing,
 * a bunch of editing BS and space characters
 * is inserted, so the string looks right on the console but still
 * contains the original string, plus other garbage:
 */
static char
*backSpaceEdit(char *s)
{
    char c;
    char *edit = s;
    char *text = s;

    while (( c = *edit++ ))
    {
        switch (c)
        {
        case KDBG_BS_CHAR:
            if (text > s)
                text --;
            break;
        default:
            *text++ = c;
        }
    }
    *text = '\0';

    return s;
}

static int
handle_switch(FILE *outFile, int *sw, char *arg, char *desc)
{
    int changed =0;
    int x = 0;

    if (arg && (strcmp(arg,"") != 0))
    {
        x = atoi(arg);
        if (x != *sw)
        {
            *sw = x;
            changed = 1;
        }
    }
    if (desc)
    {
        esclog(outFile, "%s is %d (%s)\n", desc, *sw, changed ? "changed":"unchanged");
        if (!arg)
            esclog(outFile, "(readonly)\n");
    }

    return changed;
}

static int
handle_switch_str(FILE *outFile, char *sw, char *arg, char *desc)
{
    int changed =0;

    if (arg)
    {
        if (strcmp(arg,"") != 0)
        {
            if (strcmp(arg,KDBG_ESC_OFF) == 0)
            {
                if (*sw)
                    changed = 1;
                *sw = '\0';
            }
            else if (strcmp(arg, sw) != 0)
            {
                strcpy(sw, arg);
                changed = 1;
            }
        }
    }
    if (desc)
    {
        esclog(outFile, "%s is \"%s\" (%s)\n", desc, sw, changed ? "changed":"unchanged");
        if (!arg)
            esclog(outFile, "(readonly)\n");
    }

    return changed;
}

static int
handle_switch_pstr(FILE *outFile, char **psw, char *arg, char *desc)
{
    int changed =0;

    if (arg)
    {
        if (strcmp(arg,"") != 0)
        {
            if (strcmp(arg,KDBG_ESC_OFF) == 0)
            {
                if (*psw)
                    changed = 1;
                free(*psw);
                *psw = NULL;
            }
            else
            {
                if (!*psw)
                {
                    *psw = malloc(LINESIZE);
                    **psw = '\0';
                }

                if (strcmp(arg, *psw) != 0)
                {
                    strcpy(*psw, arg);
                    changed = 1;
                }
            }
        }
    }
    if (desc)
    {
        esclog(outFile, "%s is \"%s\" (%s)\n", desc, *psw, changed ? "changed":"unchanged");
        if (!arg)
            esclog(outFile, "(readonly)\n");
    }

    return changed;
}

static int
handle_address_cmd(FILE *outFile, char *arg)
{
    PLIST_MEMBER plm;
    char Image[NAMESIZE];
    DWORD Offset;
    int cnt;
    char *s;

    if(( s = strchr(arg, ':') ))
    {
        *s = ' ';
        if ( (cnt = sscanf(arg,"%20s %x", Image, &Offset)) == 2)
        {
            if (( plm = entry_lookup(&cache, Image) ))
            {
                if (plm->RelBase != INVALID_BASE)
                    esclog(outFile, "Address: 0x%lx\n", plm->RelBase + Offset)
                else
                    esclog(outFile, "Relocated base missing for '%s' ('mod' will update)\n", Image);
            }
            else
                esclog(outFile, "Image '%s' not found\n", Image);
        }
        else
            esclog(outFile, "usage: `a <Image>:<offset>\n");
    }
    else
        esclog(outFile, "':' expected\n");

    return 1;
}

char
handle_escape_cmd(FILE *outFile, char *Line, char *path, char *LineOut)
{
    char cmd;
    char sep = '\n';
    char *arg;
    char *l = Line;
    int res = 1;
    int cnt = 0;
    int changed = 0;

    l = backSpaceEdit(l);
    if (l[1] != KDBG_ESC_CHAR)
        return l[1]; //for reprocessing as not escaped

    log(outFile, "\n");

    l += 2; //skip space+escape character
    if ( (cnt=sscanf(l,"%c%c",&cmd,&sep)) < 1)
    {
        esclog(outFile, "Command expected\n");
        res = 0;
    }

    if (res && cnt==2 && sep != ' ')
    {
        esclog(outFile, "' ' expected\n");
        res = 0;
    }
    l++; //skip cmd
    while ( *l == ' ')l++; //skip more spaces
    arg = l;
    opt_cli = 1;
    switch (cmd)
    {
    case 'a':
        handle_address_cmd(outFile, arg);
        break;
    case 'h':
        usage(1);
        break;
    case 'b':
        if (handle_switch(outFile, &opt_buffered, arg, "-b Logfile buffering"))
            set_LogFile(&logFile); //re-open same logfile
        break;
    case 'c':
        handle_switch(outFile, &opt_console, NULL, "-c Console option");
        break;
    case 'd':
        handle_switch_str(outFile, opt_dir, NULL, "-d Directory option");
        break;
    case 'l':
        if (handle_switch_str(outFile, opt_logFile, arg, "-l logfile") || (strcmp(opt_mod,"a")!=0))
        {
            opt_mod = "a";
            set_LogFile(&logFile); //open new logfile
        }
        break;
    case 'L':
        if (handle_switch_str(outFile, opt_logFile, arg, "-L logfile") || (strcmp(opt_mod,"w")!=0))
        {
            opt_mod = "w";
            set_LogFile(&logFile); //open new logfile
        }
        break;
    case 'm':
        handle_switch(outFile, &opt_Mark, arg, "-m mark (*)");
        break;
    case 'M':
        handle_switch(outFile, &opt_Mark, arg, "-M Mark (?)");
        break;
    case 'P':
        handle_switch_str(outFile, opt_Pipe, NULL, "-P Pipeline option");
        break;
    case 'q':
        opt_quit = 1;
        esclog(outFile, "Bye!\n");
        break;
    case 'r':
        handle_switch(outFile, &opt_raw, arg, "-r Raw");
        break;
    case 'R':
        changed = handle_switch_pstr(outFile, &opt_Revision, arg, NULL);
        opt_Revision_check = 0;
        if (opt_Revision)
        {
            opt_Revision_check = 1;
            if (strstr(opt_Revision, "check") == opt_Revision)
            {
                esclog(outFile, "-R is \"%s\" (%s)\n", opt_Revision, changed ? "changed":"unchanged");
            }
            else if (strstr(opt_Revision, "regscan") == opt_Revision)
            {
                char *s = strchr(opt_Revision, ',');

                revinfo.range = DEF_RANGE;
                if (s)
                {
                    *s++ = '\0';
                    revinfo.range = atoi(s);
                }
                regscan(outFile);
            }
            else if (strstr(opt_Revision, "regclear") == opt_Revision)
            {
                list_clear(&sources);
                summ.regfound = 0;
                esclog(outFile, "cleared regression scan results\n");
            }
        }
        break;
    case 's':
        if (strcmp(arg,"clear") == 0)
        {
            memset(&summ, 0, sizeof(SUMM));
            esclog(outFile, "Statistics cleared\n");
        }
        else
            stat_print(outFile, &summ);
        break;
    case 'S':
        cnt = sscanf(arg, "%d+%d", &opt_Source, &opt_SrcPlus);
        if (opt_Source)
        {
            handle_switch(outFile, &opt_undo, "1", "-u Undo");
            handle_switch(outFile, &opt_redo, "1", "-U Undo and reprocess");
            opt_Revision_check = 1;
        }
        esclog(outFile, "-S Sources option is %d+%d,\"%s\"\n", opt_Source, opt_SrcPlus, opt_SourcesPath);
        esclog(outFile, "(Setting source tree not implemented)\n");
        break;
    case 't':
        handle_switch(outFile, &opt_twice, arg, "-t Translate twice");
        break;
    case 'T':
        handle_switch(outFile, &opt_twice, arg, NULL);
        handle_switch(outFile, &opt_Twice, arg, "-T Translate for (address-1)");
        break;
    case 'u':
        handle_switch(outFile, &opt_undo, arg, "-u undo");
        break;
    case 'U':
        handle_switch(outFile, &opt_undo, arg, NULL);
        handle_switch(outFile, &opt_redo, arg, "-U Undo and reprocess");
        break;
    case 'v':
        handle_switch(outFile, &opt_verbose, arg, "-v Verbosity");
        break;
    case 'z':
        handle_switch_str(outFile, opt_7z, NULL, "-z 7z path");
        break;
    default:
        if (strchr(optchars, cmd))
            esclog(outFile, "Command not implemented in cli: %c %s\n",cmd, arg)
        else
            esclog(outFile, "Unknown command: %c %s\n",cmd, arg);
    }
    opt_cli = 0;

    memset(Line, '\0', LINESIZE);  // flushed

    return KDBG_ESC_CHAR; //handled escaped command
}

/* EOF */
