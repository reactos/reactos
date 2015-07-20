/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/interpreter.c
 * PURPOSE:         Reads the user input and then envokes the selected
 *                  command by the user.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL exit_main(INT argc, LPWSTR *argv);
BOOL rem_main(INT argc, LPWSTR *argv);


COMMAND cmds[] =
{
    {L"active",      active_main,      IDS_HELP_CMD_ACTIVE,      IDS_HELP_CMD_DESC_ACTIVE},
    {L"add",         add_main,         IDS_HELP_CMD_ADD,         IDS_HELP_CMD_DESC_ADD},
    {L"assign",      assign_main,      IDS_HELP_CMD_ASSIGN,      IDS_HELP_CMD_DESC_ASSIGN},
    {L"attach",      attach_main,      IDS_HELP_CMD_ATTACH,      IDS_HELP_CMD_DESC_ATTACH},
    {L"attributes",  attributes_main,  IDS_HELP_CMD_ATTRIBUTES,  IDS_HELP_CMD_DESC_ATTRIBUTES},
    {L"automount",   automount_main,   IDS_HELP_CMD_AUTOMOUNT,   IDS_HELP_CMD_DESC_AUTOMOUNT},
    {L"break",       break_main,       IDS_HELP_CMD_BREAK,       IDS_HELP_CMD_DESC_BREAK},
    {L"clean",       clean_main,       IDS_HELP_CMD_CLEAN,       IDS_HELP_CMD_DESC_CLEAN},
    {L"compact",     compact_main,     IDS_HELP_CMD_COMPACT,     IDS_HELP_CMD_DESC_COMPACT},
    {L"convert",     convert_main,     IDS_HELP_CMD_CONVERT,     IDS_HELP_CMD_DESC_CONVERT},
    {L"create",      create_main,      IDS_HELP_CMD_CREATE,      IDS_HELP_CMD_DESC_CREATE},
    {L"delete",      delete_main,      IDS_HELP_CMD_DELETE,      IDS_HELP_CMD_DESC_DELETE},
    {L"detail",      detail_main,      IDS_HELP_CMD_DETAIL,      IDS_HELP_CMD_DESC_DETAIL},
    {L"detach",      detach_main,      IDS_HELP_CMD_DETACH,      IDS_HELP_CMD_DESC_DETACH},
    {L"exit",        NULL,             IDS_NONE,                 IDS_HELP_CMD_DESC_EXIT},
    {L"expand",      expand_main,      IDS_HELP_CMD_EXPAND,      IDS_HELP_CMD_DESC_EXPAND},
    {L"extend",      extend_main,      IDS_HELP_CMD_EXTEND,      IDS_HELP_CMD_DESC_EXTEND},
    {L"filesystems", filesystems_main, IDS_HELP_CMD_FILESYSTEMS, IDS_HELP_CMD_DESC_FS},
    {L"format",      format_main,      IDS_HELP_CMD_FORMAT,      IDS_HELP_CMD_DESC_FORMAT},
    {L"gpt",         gpt_main,         IDS_HELP_CMD_GPT,         IDS_HELP_CMD_DESC_GPT},
    {L"help",        help_main,        IDS_HELP_CMD_HELP,        IDS_HELP_CMD_DESC_HELP},
    {L"import",      import_main,      IDS_HELP_CMD_IMPORT,      IDS_HELP_CMD_DESC_IMPORT},
    {L"inactive",    inactive_main,    IDS_HELP_CMD_INACTIVE,    IDS_HELP_CMD_DESC_INACTIVE},
    {L"list",        list_main,        IDS_HELP_CMD_LIST,        IDS_HELP_CMD_DESC_LIST},
    {L"merge",       merge_main,       IDS_HELP_CMD_MERGE,       IDS_HELP_CMD_DESC_MERGE},
    {L"offline",     offline_main,     IDS_HELP_CMD_OFFLINE,     IDS_HELP_CMD_DESC_OFFLINE},
    {L"online",      online_main,      IDS_HELP_CMD_ONLINE,      IDS_HELP_CMD_DESC_ONLINE},
    {L"recover",     recover_main,     IDS_HELP_CMD_RECOVER,     IDS_HELP_CMD_DESC_RECOVER},
    {L"rem",         NULL,             IDS_NONE,                 IDS_HELP_CMD_DESC_REM},
    {L"remove",      remove_main,      IDS_HELP_CMD_REMOVE,      IDS_HELP_CMD_DESC_REMOVE},
    {L"repair",      repair_main,      IDS_HELP_CMD_REPAIR,      IDS_HELP_CMD_DESC_REPAIR},
    {L"rescan",      rescan_main,      IDS_HELP_CMD_RESCAN,      IDS_HELP_CMD_DESC_RESCAN},
    {L"retain",      retain_main,      IDS_HELP_CMD_RETAIN,      IDS_HELP_CMD_DESC_RETAIN},
    {L"san",         san_main,         IDS_HELP_CMD_SAN,         IDS_HELP_CMD_DESC_SAN},
    {L"select",      select_main,      IDS_HELP_CMD_SELECT,      IDS_HELP_CMD_DESC_SELECT},
    {L"setid",       setid_main,       IDS_HELP_CMD_SETID,       IDS_HELP_CMD_DESC_SETID},
    {L"shrink",      shrink_main,      IDS_HELP_CMD_SHRINK,      IDS_HELP_CMD_DESC_SHRINK},
    {L"uniqueid",    uniqueid_main,    IDS_HELP_CMD_UNIQUEID,    IDS_HELP_CMD_DESC_UNIQUEID},
    {NULL,           NULL,             IDS_NONE,                 IDS_NONE}
};


/* FUNCTIONS *****************************************************************/

/*
 * InterpretCmd(char *cmd_line, char *arg_line):
 * compares the command name to a list of available commands, and
 * determines which function to envoke.
 */
BOOL
InterpretCmd(int argc, LPWSTR *argv)
{
    PCOMMAND cmdptr;

    /* First, determine if the user wants to exit
       or to use a comment */
    if(wcsicmp(argv[0], L"exit") == 0)
        return FALSE;

    if(wcsicmp(argv[0], L"rem") == 0)
        return TRUE;

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->name; cmdptr++)
    {
        if (wcsicmp(argv[0], cmdptr->name) == 0)
            return cmdptr->func(argc, argv);
    }

    help_cmdlist();

    return TRUE;
}


/*
 * InterpretScript(char *line):
 * The main function used for when reading commands from scripts.
 */
BOOL
InterpretScript(LPWSTR input_line)
{
    LPWSTR args_vector[MAX_ARGS_COUNT];
    INT args_count = 0;
    BOOL bWhiteSpace = TRUE;
    LPWSTR ptr;

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
    LPWSTR args_vector[MAX_ARGS_COUNT];
    INT args_count = 0;
    BOOL bWhiteSpace = TRUE;
    BOOL bRun = TRUE;
    LPWSTR ptr;

    while (bRun == TRUE)
    {
        args_count = 0;
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
