/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/interpreter.c
 * PURPOSE:         Reads the user input and then invokes the selected
 *                  command by the user.
 * PROGRAMMERS:     Lee Schroeder
 */

#include "diskpart.h"
#include "diskpart_msg.h"

BOOL exit_main(INT argc, LPWSTR *argv);
BOOL rem_main(INT argc, LPWSTR *argv);


COMMAND cmds[] =
{
    {L"ACTIVE",      NULL,         NULL,        active_main,             IDS_HELP_ACTIVE,                    MSG_COMMAND_ACTIVE},
    {L"ADD",         NULL,         NULL,        add_main,                IDS_HELP_ADD,                       MSG_COMMAND_ADD},
    {L"ASSIGN",      NULL,         NULL,        assign_main,             IDS_HELP_ASSIGN,                    MSG_COMMAND_ASSIGN},
    {L"ATTACH",      NULL,         NULL,        attach_main,             IDS_HELP_ATTACH,                    MSG_COMMAND_ATTACH},
    {L"ATTRIBUTES",  NULL,         NULL,        attributes_main,         IDS_HELP_ATTRIBUTES,                MSG_COMMAND_ATTRIBUTES},
    {L"AUTOMOUNT",   NULL,         NULL,        automount_main,          IDS_HELP_AUTOMOUNT,                 MSG_COMMAND_AUTOMOUNT},
    {L"BREAK",       NULL,         NULL,        break_main,              IDS_HELP_BREAK,                     MSG_COMMAND_BREAK},
    {L"CLEAN",       NULL,         NULL,        clean_main,              IDS_HELP_CLEAN,                     MSG_COMMAND_CLEAN},
    {L"COMPACT",     NULL,         NULL,        compact_main,            IDS_HELP_COMPACT,                   MSG_COMMAND_COMPACT},
    {L"CONVERT",     NULL,         NULL,        convert_main,            IDS_HELP_CONVERT,                   MSG_COMMAND_CONVERT},

    {L"CREATE",      NULL,         NULL,        NULL,                    IDS_HELP_CREATE,                    MSG_NONE},
    {L"CREATE",      L"PARTITION", NULL,        NULL,                    IDS_HELP_CREATE_PARTITION,          MSG_NONE},
//    {L"CREATE",      L"PARTITION", L"EFI",      CreateEfiPartition,      IDS_HELP_CREATE_PARTITION_EFI,      MSG_COMMAND_CREATE_PARTITION_EFI},
    {L"CREATE",      L"PARTITION", L"EXTENDED", CreateExtendedPartition, IDS_HELP_CREATE_PARTITION_EXTENDED, MSG_COMMAND_CREATE_PARTITION_EXTENDED},
    {L"CREATE",      L"PARTITION", L"LOGICAL",  CreateLogicalPartition,  IDS_HELP_CREATE_PARTITION_LOGICAL,  MSG_COMMAND_CREATE_PARTITION_LOGICAL},
//    {L"CREATE",      L"PARTITION", L"MSR",      CreateMsrPartition,      IDS_HELP_CREATE_PARTITION_MSR,      MSG_COMMAND_CREATE_PARTITION_MSR},
    {L"CREATE",      L"PARTITION", L"PRIMARY",  CreatePrimaryPartition,  IDS_HELP_CREATE_PARTITION_PRIMARY,  MSG_COMMAND_CREATE_PARTITION_PRIMARY},
    {L"CREATE",      L"VOLUME",    NULL,        NULL,                    IDS_HELP_CREATE_VOLUME,             MSG_NONE},
    {L"CREATE",      L"VDISK",     NULL,        NULL,                    IDS_HELP_CREATE_VDISK,              MSG_NONE},

    {L"DELETE",      NULL,         NULL,        NULL,                    IDS_HELP_DELETE,                    MSG_NONE},
    {L"DELETE",      L"DISK",      NULL,        DeleteDisk,              IDS_HELP_DELETE_DISK,               MSG_COMMAND_DELETE_DISK},
    {L"DELETE",      L"PARTITION", NULL,        DeletePartition,         IDS_HELP_DELETE_PARTITION,          MSG_COMMAND_DELETE_PARTITION},
    {L"DELETE",      L"VOLUME",    NULL,        DeleteVolume,            IDS_HELP_DELETE_VOLUME,             MSG_COMMAND_DELETE_VOLUME},

    {L"DETACH",      NULL,         NULL,        detach_main,             IDS_HELP_DETACH,                    MSG_COMMAND_DETACH},

    {L"DETAIL",      NULL,         NULL,        NULL,                    IDS_HELP_DETAIL,                    MSG_NONE},
    {L"DETAIL",      L"DISK",      NULL,        DetailDisk,              IDS_HELP_DETAIL_DISK,               MSG_COMMAND_DETAIL_DISK},
    {L"DETAIL",      L"PARTITION", NULL,        DetailPartition,         IDS_HELP_DETAIL_PARTITION,          MSG_COMMAND_DETAIL_PARTITION},
    {L"DETAIL",      L"VOLUME",    NULL,        DetailVolume,            IDS_HELP_DETAIL_VOLUME,             MSG_COMMAND_DETAIL_VOLUME},

    {L"DUMP",        NULL,         NULL,        NULL,                    IDS_NONE,                           MSG_NONE},
    {L"DUMP",        L"DISK",      NULL,        DumpDisk,                IDS_NONE,                           MSG_NONE},
    {L"DUMP",        L"PARTITION", NULL,        DumpPartition,           IDS_NONE,                           MSG_NONE},
    {L"EXIT",        NULL,         NULL,        NULL,                    IDS_HELP_EXIT,                      MSG_COMMAND_EXIT},

    {L"EXPAND",      NULL,         NULL,        expand_main,             IDS_HELP_EXPAND,                    MSG_COMMAND_EXPAND},
    {L"EXTEND",      NULL,         NULL,        extend_main,             IDS_HELP_EXTEND,                    MSG_COMMAND_EXTEND},
    {L"FILESYSTEMS", NULL,         NULL,        filesystems_main,        IDS_HELP_FILESYSTEMS,               MSG_COMMAND_FILESYSTEMS},
    {L"FORMAT",      NULL,         NULL,        format_main,             IDS_HELP_FORMAT,                    MSG_COMMAND_FORMAT},
    {L"GPT",         NULL,         NULL,        gpt_main,                IDS_HELP_GPT,                       MSG_COMMAND_GPT},
    {L"HELP",        NULL,         NULL,        help_main,               IDS_HELP_HELP,                      MSG_COMMAND_HELP},
    {L"IMPORT",      NULL,         NULL,        import_main,             IDS_HELP_IMPORT,                    MSG_COMMAND_IMPORT},
    {L"INACTIVE",    NULL,         NULL,        inactive_main,           IDS_HELP_INACTIVE,                  MSG_COMMAND_INACTIVE},

    {L"LIST",        NULL,         NULL,        NULL,                    IDS_HELP_LIST,                      MSG_NONE},
    {L"LIST",        L"DISK",      NULL,        ListDisk,                IDS_HELP_LIST_DISK,                 MSG_COMMAND_LIST_DISK},
    {L"LIST",        L"PARTITION", NULL,        ListPartition,           IDS_HELP_LIST_PARTITION,            MSG_COMMAND_LIST_PARTITION},
    {L"LIST",        L"VOLUME",    NULL,        ListVolume,              IDS_HELP_LIST_VOLUME,               MSG_COMMAND_LIST_VOLUME},
    {L"LIST",        L"VDISK",     NULL,        ListVirtualDisk,         IDS_HELP_LIST_VDISK,                MSG_COMMAND_LIST_VDISK},

    {L"MERGE",       NULL,         NULL,        merge_main,              IDS_HELP_MERGE,                     MSG_COMMAND_MERGE},
    {L"OFFLINE",     NULL,         NULL,        offline_main,            IDS_HELP_OFFLINE,                   MSG_COMMAND_OFFLINE},
    {L"ONLINE",      NULL,         NULL,        online_main,             IDS_HELP_ONLINE,                    MSG_COMMAND_ONLINE},
    {L"RECOVER",     NULL,         NULL,        recover_main,            IDS_HELP_RECOVER,                   MSG_COMMAND_RECOVER},
    {L"REM",         NULL,         NULL,        NULL,                    IDS_HELP_REM,                       MSG_COMMAND_REM},
    {L"REMOVE",      NULL,         NULL,        remove_main,             IDS_HELP_REMOVE,                    MSG_COMMAND_REMOVE},
    {L"REPAIR",      NULL,         NULL,        repair_main,             IDS_HELP_REPAIR,                    MSG_COMMAND_REPAIR},
    {L"RESCAN",      NULL,         NULL,        rescan_main,             IDS_HELP_RESCAN,                    MSG_COMMAND_RESCAN},
    {L"RETAIN",      NULL,         NULL,        retain_main,             IDS_HELP_RETAIN,                    MSG_COMMAND_RETAIN},
    {L"SAN",         NULL,         NULL,        san_main,                IDS_HELP_SAN,                       MSG_COMMAND_SAN},

    {L"SELECT",      NULL,         NULL,        NULL,                    IDS_HELP_SELECT,                    MSG_NONE},
    {L"SELECT",      L"DISK",      NULL,        SelectDisk,              IDS_HELP_SELECT_DISK,               MSG_COMMAND_SELECT_DISK},
    {L"SELECT",      L"PARTITION", NULL,        SelectPartition,         IDS_HELP_SELECT_PARTITION,          MSG_COMMAND_SELECT_PARTITION},
    {L"SELECT",      L"VOLUME",    NULL,        SelectVolume,            IDS_HELP_SELECT_VOLUME,             MSG_COMMAND_SELECT_VOLUME},
//    {L"SELECT",      L"VDISK",     NULL,        SelectVirtualDisk,       IDS_HELP_SELECT_VDISK,              MSG_COMMAND_SELECT_VDISK},

    {L"SET",         NULL,         NULL,        setid_main,              IDS_HELP_SETID,                     MSG_COMMAND_SETID},
    {L"SETID",       NULL,         NULL,        setid_main,              IDS_HELP_SETID,                     MSG_COMMAND_SETID},
    {L"SHRINK",      NULL,         NULL,        shrink_main,             IDS_HELP_SHRINK,                    MSG_COMMAND_SHRINK},

    {L"UNIQUEID",    NULL,         NULL,        NULL,                    IDS_HELP_UNIQUEID,                  MSG_NONE},
    {L"UNIQUEID",    L"DISK",      NULL,        UniqueIdDisk,            IDS_HELP_UNIQUEID_DISK,             MSG_COMMAND_UNIQUEID_DISK},

    {NULL,           NULL,         NULL,        NULL,                    IDS_NONE,                           MSG_NONE}
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
    if (_wcsicmp(argv[0], L"exit") == 0)
        return FALSE;

    if (_wcsicmp(argv[0], L"rem") == 0)
        return TRUE;

    /* Scan internal command table */
    for (cmdptr = cmds; cmdptr->cmd1; cmdptr++)
    {
        if ((cmdptr1 == NULL) &&
            (cmdptr->cmd1 != NULL) && (_wcsicmp(argv[0], cmdptr->cmd1) == 0))
            cmdptr1 = cmdptr;

        if ((cmdptr2 == NULL) &&
            (argc >= 2) &&
            (cmdptr->cmd1 != NULL) && (_wcsicmp(argv[0], cmdptr->cmd1) == 0) &&
            (cmdptr->cmd2 != NULL) && (_wcsicmp(argv[1], cmdptr->cmd2) == 0))
            cmdptr2 = cmdptr;

        if ((cmdptr3 == NULL) &&
            (argc >= 3) &&
            (cmdptr->cmd1 != NULL) && (_wcsicmp(argv[0], cmdptr->cmd1) == 0) &&
            (cmdptr->cmd2 != NULL) && (_wcsicmp(argv[1], cmdptr->cmd2) == 0) &&
            (cmdptr->cmd3 != NULL) && (_wcsicmp(argv[2], cmdptr->cmd3) == 0))
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
    BOOL bQuote = FALSE;
    LPWSTR ptr;

    memset(args_vector, 0, sizeof(args_vector));

    bQuote = FALSE;
    ptr = input_line;
    while (*ptr != 0)
    {
        if (*ptr == L'"')
            bQuote = !bQuote;

        if ((iswspace(*ptr) && (bQuote == FALSE))|| *ptr == L'\n')
        {
            *ptr = 0;
            bWhiteSpace = TRUE;
        }
        else
        {
            if ((bWhiteSpace != FALSE) && (bQuote == FALSE) && (args_count < MAX_ARGS_COUNT))
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
    BOOL bQuote = FALSE;
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

        bQuote = FALSE;
        ptr = input_line;
        while (*ptr != 0)
        {
            if (*ptr == L'"')
                bQuote = !bQuote;

            if ((iswspace(*ptr) && (bQuote == FALSE))|| *ptr == L'\n')
            {
                *ptr = 0;
                bWhiteSpace = TRUE;
            }
            else
            {
                if ((bWhiteSpace != FALSE) && (bQuote == FALSE) && (args_count < MAX_ARGS_COUNT))
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
