/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/interpreter.c
 * PURPOSE:         Reads the user input and then invokes the selected
 *                  command by the user.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"

BOOL exit_main(INT argc, LPWSTR *argv);
BOOL rem_main(INT argc, LPWSTR *argv);


COMMAND cmds[] =
{
    {L"active",      NULL,         NULL,        active_main,             IDS_HELP_CMD_ACTIVE,      IDS_HELP_CMD_DESC_ACTIVE},
    {L"add",         NULL,         NULL,        add_main,                IDS_HELP_CMD_ADD,         IDS_HELP_CMD_DESC_ADD},
    {L"assign",      NULL,         NULL,        assign_main,             IDS_HELP_CMD_ASSIGN,      IDS_HELP_CMD_DESC_ASSIGN},
    {L"attach",      NULL,         NULL,        attach_main,             IDS_HELP_CMD_ATTACH,      IDS_HELP_CMD_DESC_ATTACH},
    {L"attributes",  NULL,         NULL,        attributes_main,         IDS_HELP_CMD_ATTRIBUTES,  IDS_HELP_CMD_DESC_ATTRIBUTES},
    {L"automount",   NULL,         NULL,        automount_main,          IDS_HELP_CMD_AUTOMOUNT,   IDS_HELP_CMD_DESC_AUTOMOUNT},
    {L"break",       NULL,         NULL,        break_main,              IDS_HELP_CMD_BREAK,       IDS_HELP_CMD_DESC_BREAK},
    {L"clean",       NULL,         NULL,        clean_main,              IDS_HELP_CMD_CLEAN,       IDS_HELP_CMD_DESC_CLEAN},
    {L"compact",     NULL,         NULL,        compact_main,            IDS_HELP_CMD_COMPACT,     IDS_HELP_CMD_DESC_COMPACT},
    {L"convert",     NULL,         NULL,        convert_main,            IDS_HELP_CMD_CONVERT,     IDS_HELP_CMD_DESC_CONVERT},

    {L"create",      NULL,         NULL,        NULL,                    IDS_HELP_CMD_CREATE,                    IDS_HELP_CMD_DESC_CREATE},
    {L"create",      L"partition", NULL,        NULL,                    IDS_HELP_CMD_CREATE_PARTITION,          IDS_NONE},
    {L"create",      L"partition", L"extended", CreateExtendedPartition, IDS_HELP_CMD_CREATE_PARTITION_EXTENDED, IDS_NONE},
    {L"create",      L"partition", L"logical",  CreateLogicalPartition,  IDS_HELP_CMD_CREATE_PARTITION_LOGICAL,  IDS_NONE},
    {L"create",      L"partition", L"primary",  CreatePrimaryPartition,  IDS_HELP_CMD_CREATE_PARTITION_PRIMARY,  IDS_NONE},

    {L"delete",      NULL,         NULL,        delete_main,             IDS_HELP_CMD_DELETE,                    IDS_HELP_CMD_DESC_DELETE},

    {L"detail",      NULL,         NULL,        NULL,                    IDS_HELP_CMD_DETAIL,                    IDS_HELP_CMD_DESC_DETAIL},
    {L"detail",      L"disk",      NULL,        DetailDisk,              IDS_HELP_CMD_DETAIL_DISK,               IDS_NONE},
    {L"detail",      L"partition", NULL,        DetailPartition,         IDS_HELP_CMD_DETAIL_PARTITION,          IDS_NONE},
    {L"detail",      L"volume",    NULL,        DetailVolume,            IDS_HELP_CMD_DETAIL_VOLUME,             IDS_NONE},

    {L"detach",      NULL,         NULL,        detach_main,             IDS_HELP_CMD_DETACH,                    IDS_HELP_CMD_DESC_DETACH},
    {L"dump",        NULL,         NULL,        dump_main,               IDS_NONE,                               IDS_NONE},
    {L"exit",        NULL,         NULL,        NULL,                    IDS_NONE,                               IDS_HELP_CMD_DESC_EXIT},
    {L"expand",      NULL,         NULL,        expand_main,             IDS_HELP_CMD_EXPAND,                    IDS_HELP_CMD_DESC_EXPAND},
    {L"extend",      NULL,         NULL,        extend_main,             IDS_HELP_CMD_EXTEND,                    IDS_HELP_CMD_DESC_EXTEND},
    {L"filesystems", NULL,         NULL,        filesystems_main,        IDS_HELP_CMD_FILESYSTEMS,               IDS_HELP_CMD_DESC_FS},
    {L"format",      NULL,         NULL,        format_main,             IDS_HELP_CMD_FORMAT,                    IDS_HELP_CMD_DESC_FORMAT},
    {L"gpt",         NULL,         NULL,        gpt_main,                IDS_HELP_CMD_GPT,                       IDS_HELP_CMD_DESC_GPT},
    {L"help",        NULL,         NULL,        help_main,               IDS_HELP_CMD_HELP,                      IDS_HELP_CMD_DESC_HELP},
    {L"import",      NULL,         NULL,        import_main,             IDS_HELP_CMD_IMPORT,                    IDS_HELP_CMD_DESC_IMPORT},
    {L"inactive",    NULL,         NULL,        inactive_main,           IDS_HELP_CMD_INACTIVE,                  IDS_HELP_CMD_DESC_INACTIVE},

    {L"list",        NULL,         NULL,        NULL,                    IDS_HELP_CMD_LIST,                      IDS_HELP_CMD_DESC_LIST},
    {L"list",        L"disk",      NULL,        ListDisk,                IDS_HELP_CMD_LIST_DISK,                 IDS_NONE},
    {L"list",        L"partition", NULL,        ListPartition,           IDS_HELP_CMD_LIST_PARTITION,            IDS_NONE},
    {L"list",        L"volume",    NULL,        ListVolume,              IDS_HELP_CMD_LIST_VOLUME,               IDS_NONE},
    {L"list",        L"vdisk",     NULL,        ListVirtualDisk,         IDS_HELP_CMD_LIST_VDISK,                IDS_NONE},

    {L"merge",       NULL,         NULL,        merge_main,              IDS_HELP_CMD_MERGE,                     IDS_HELP_CMD_DESC_MERGE},
    {L"offline",     NULL,         NULL,        offline_main,            IDS_HELP_CMD_OFFLINE,                   IDS_HELP_CMD_DESC_OFFLINE},
    {L"online",      NULL,         NULL,        online_main,             IDS_HELP_CMD_ONLINE,                    IDS_HELP_CMD_DESC_ONLINE},
    {L"recover",     NULL,         NULL,        recover_main,            IDS_HELP_CMD_RECOVER,                   IDS_HELP_CMD_DESC_RECOVER},
    {L"rem",         NULL,         NULL,        NULL,                    IDS_NONE,                               IDS_HELP_CMD_DESC_REM},
    {L"remove",      NULL,         NULL,        remove_main,             IDS_HELP_CMD_REMOVE,                    IDS_HELP_CMD_DESC_REMOVE},
    {L"repair",      NULL,         NULL,        repair_main,             IDS_HELP_CMD_REPAIR,                    IDS_HELP_CMD_DESC_REPAIR},
    {L"rescan",      NULL,         NULL,        rescan_main,             IDS_HELP_CMD_RESCAN,                    IDS_HELP_CMD_DESC_RESCAN},
    {L"retain",      NULL,         NULL,        retain_main,             IDS_HELP_CMD_RETAIN,                    IDS_HELP_CMD_DESC_RETAIN},
    {L"san",         NULL,         NULL,        san_main,                IDS_HELP_CMD_SAN,                       IDS_HELP_CMD_DESC_SAN},

    {L"select",      NULL,         NULL,        NULL,                    IDS_HELP_CMD_SELECT,                    IDS_HELP_CMD_DESC_SELECT},
    {L"select",      L"disk",      NULL,        SelectDisk,              IDS_HELP_CMD_SELECT_DISK,               IDS_NONE},
    {L"select",      L"partition", NULL,        SelectPartition,         IDS_HELP_CMD_SELECT_PARTITION,          IDS_NONE},
    {L"select",      L"volume",    NULL,        SelectVolume,            IDS_HELP_CMD_SELECT_VOLUME,             IDS_NONE},
//    {L"select",      L"vdisk",     NULL,        SelectVirtualDisk,       IDS_HELP_CMD_SELECT_VDISK,              IDS_NONE},

    {L"setid",       NULL,         NULL,        setid_main,              IDS_HELP_CMD_SETID,                     IDS_HELP_CMD_DESC_SETID},
    {L"shrink",      NULL,         NULL,        shrink_main,             IDS_HELP_CMD_SHRINK,                    IDS_HELP_CMD_DESC_SHRINK},

    {L"uniqueid",    NULL,         NULL,        NULL,                    IDS_HELP_CMD_UNIQUEID,                  IDS_HELP_CMD_DESC_UNIQUEID},
    {L"uniqueid",    L"disk",      NULL,        UniqueIdDisk,            IDS_HELP_CMD_UNIQUEID_DISK,             IDS_NONE},

    {NULL,           NULL,         NULL,        NULL,                    IDS_NONE,                               IDS_NONE}
};


/* FUNCTIONS *****************************************************************/

/*
 * InterpretCmd(char *cmd_line, char *arg_line):
 * compares the command name to a list of available commands, and
 * determines which function to invoke.
 */
BOOL
InterpretCmd(
    int argc,
    LPWSTR *argv)
{
    PCOMMAND cmdptr;
    PCOMMAND cmdptr1 = NULL;
    PCOMMAND cmdptr2 = NULL;
    PCOMMAND cmdptr3 = NULL;

    /* If no args provided */
    if (argc < 1)
        return TRUE;

    /* First, determine if the user wants to exit
       or to use a comment */
    if (wcsicmp(argv[0], L"exit") == 0)
        return FALSE;

    if (wcsicmp(argv[0], L"rem") == 0)
        return TRUE;

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
    {
        if ((cmdptr1 == NULL) &&
            (wcsicmp(argv[0], cmdptr->cmd1) == 0))
            cmdptr1 = cmdptr;

        if ((cmdptr2 == NULL) &&
            (argc >= 2) &&
            (wcsicmp(argv[0], cmdptr->cmd1) == 0) &&
            (wcsicmp(argv[1], cmdptr->cmd2) == 0))
            cmdptr2 = cmdptr;

        if ((cmdptr3 == NULL) &&
            (argc >= 3) &&
            (wcsicmp(argv[0], cmdptr->cmd1) == 0) &&
            (wcsicmp(argv[1], cmdptr->cmd2) == 0) &&
            (wcsicmp(argv[2], cmdptr->cmd3) == 0))
            cmdptr3 = cmdptr;
    }

    if (cmdptr3 != NULL)
    {
        if (cmdptr3->func == NULL)
            return HelpCommand(cmdptr3);
        else
            return cmdptr3->func(argc, argv);
    }
    else if (cmdptr2 != NULL)
    {
        if (cmdptr2->func == NULL)
            return HelpCommand(cmdptr2);
        else
            return cmdptr2->func(argc, argv);
    }
    else if (cmdptr1 != NULL)
    {
        if (cmdptr1->func == NULL)
            return HelpCommand(cmdptr1);
        else
            return cmdptr1->func(argc, argv);
    }

    HelpCommandList();

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
            if ((bWhiteSpace != FALSE) && (args_count < MAX_ARGS_COUNT))
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

    while (bRun != FALSE)
    {
        args_count = 0;
        memset(args_vector, 0, sizeof(args_vector));

        /* Shown just before the input where the user places commands */
        ConResPuts(StdOut, IDS_APP_PROMPT);

        /* Get input from the user. */
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
                if ((bWhiteSpace != FALSE) && (args_count < MAX_ARGS_COUNT))
                {
                    args_vector[args_count] = ptr;
                    args_count++;
                }
                bWhiteSpace = FALSE;
            }
            ptr++;
        }

        /* Send the string to find the command */
        bRun = InterpretCmd(args_count, args_vector);
    }
}
