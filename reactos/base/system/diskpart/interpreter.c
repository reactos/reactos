/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/interpreter.c
 * PURPOSE:         Reads the user input and then envokes the selected
 *                  command by the user.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL exit_main(INT argc, WCHAR **argv);
BOOL rem_main(INT argc, WCHAR **argv);


COMMAND cmds[] =
{
    {L"active",      active_main,      help_active},
    {L"add",         add_main,         help_add},
    {L"assign",      assign_main,      help_assign},
    {L"attributes",  attributes_main,  help_attributes},
    {L"automount",   automount_main,   help_automount},
    {L"break",       break_main,       help_break},
    {L"clean",       clean_main,       help_clean},
    {L"compact",     compact_main,     help_compact},
    {L"convert",     convert_main,     help_convert},
    {L"create",      create_main,      help_create},
    {L"delete",      delete_main,      help_delete},
    {L"detail",      detail_main,      help_detail},
    {L"detach",      detach_main,      help_detach},
    {L"exit",        exit_main,        NULL},
    {L"expand",      expand_main,      help_expand},
    {L"extend",      extend_main,      help_extend},
    {L"filesystems", filesystems_main, help_filesystems},
    {L"format",      format_main,      help_format},
    {L"gpt",         gpt_main,         help_gpt},
    {L"help",        help_main,        help_help},
    {L"list",        list_main,        help_list},
    {L"import",      import_main,      help_import},
    {L"inactive",    inactive_main,    help_inactive},
    {L"merge",       merge_main,       help_merge},
    {L"offline",     offline_main,     help_offline},
    {L"online",      online_main,      help_online},
    {L"recover",     recover_main,     help_recover},
    {L"rem",         rem_main,         NULL},
    {L"remove",      remove_main,      help_remove},
    {L"repair",      repair_main,      help_repair},
    {L"rescan",      rescan_main,      help_rescan},
    {L"retain",      retain_main,      help_retain},
    {L"san",         san_main,         help_san},
    {L"select",      select_main,      help_select},
    {L"setid",       setid_main,       help_setid},
    {L"shrink",      shrink_main,      help_shrink},
    {L"uniqueid",    uniqueid_main,    help_uniqueid},
    {NULL,           NULL,             NULL}
};


/* FUNCTIONS *****************************************************************/

BOOL
exit_main(INT argc, WCHAR **argv)
{
    return FALSE;
}


BOOL
rem_main(INT argc, WCHAR **argv)
{
    return TRUE;
}


/*
 * InterpretCmd(char *cmd_line, char *arg_line):
 * compares the command name to a list of available commands, and
 * determines which function to envoke.
 */
BOOL
InterpretCmd(int argc, WCHAR **argv)
{
    PCOMMAND cmdptr;

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (wcsicmp(argv[0], cmdptr->name) == 0)
        {
            return cmdptr->func(argc, argv);
        }
    }

    help_cmdlist();

    return TRUE;
}


/*
 * InterpretScript(char *line):
 * The main function used for when reading commands from scripts.
 */
BOOL
InterpretScript(WCHAR *input_line)
{
    WCHAR *args_vector[MAX_ARGS_COUNT];
    INT args_count = 0;
    BOOL bWhiteSpace = TRUE;
    WCHAR *ptr;

    memset(args_vector, 0, sizeof(args_vector));

    ptr = input_line;
    while (*ptr != 0)
    {
        if (iswspace(*ptr) || *ptr == L'\n')
        {
            *ptr = 0;
            bWhiteSpace = TRUE;
        }
        else
        {
            if ((bWhiteSpace == TRUE) && (args_count < MAX_ARGS_COUNT))
            {
                args_vector[args_count] = ptr;
                args_count++;
            }

            bWhiteSpace = FALSE;
        }

        ptr++;
    }

    /* sends the string to find the command */
    return InterpretCmd(args_count, args_vector);
}


/*
 * InterpretMain():
 * Contents for the main program loop as it reads each line, and then
 * it sends the string to interpret_line, where it determines what
 * command to use.
 */
VOID
InterpretMain(VOID)
{
    WCHAR input_line[MAX_STRING_SIZE];
    WCHAR *args_vector[MAX_ARGS_COUNT];
    INT args_count = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bRun = TRUE;
    WCHAR *ptr;

    while (bRun == TRUE)
    {
        memset(args_vector, 0, sizeof(args_vector));

        /* shown just before the input where the user places commands */
        PrintResourceString(IDS_APP_PROMPT);

        /* gets input from the user. */
        fgetws(input_line, MAX_STRING_SIZE, stdin);

        ptr = input_line;
        while (*ptr != 0)
        {
            if (iswspace(*ptr) || *ptr == L'\n')
            {
                *ptr = 0;
                bWhiteSpace = TRUE;
            }
            else
            {
                if ((bWhiteSpace == TRUE) && (args_count < MAX_ARGS_COUNT))
                {
                    args_vector[args_count] = ptr;
                    args_count++;
                }

                bWhiteSpace = FALSE;
            }

            ptr++;
        }

        /* sends the string to find the command */
        bRun = InterpretCmd(args_count, args_vector);
    }
}
