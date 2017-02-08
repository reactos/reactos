/*
 * PROJECT:         ReactOS DiskPart
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/system/diskpart/diskpart.c
 * PURPOSE:         Manages all the partitions of the OS in
 *                  an interactive way
 * PROGRAMMERS:     Lee Schroeder
 */
#ifndef DISKPART_H
#define DISKPART_H

/* INCLUDES ******************************************************************/

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>

#include "resource.h"

/* DEFINES *******************************************************************/

typedef struct _COMMAND
{
    WCHAR *name;
    BOOL (*func)(INT, WCHAR**);
    VOID (*help)(INT, WCHAR**);
} COMMAND, *PCOMMAND;

extern COMMAND cmds[];

/* NOERR codes for the program */
#define ERROR_NONE      0
#define ERROR_FATAL     1
#define ERROR_CMD_ARG   2
#define ERROR_FILE      3
#define ERROR_SERVICE   4
#define ERROR_SYNTAX    5

#define DISKPART_VERSION     L"0.0.019"

#define MAX_STRING_SIZE 1024
#define MAX_ARGS_COUNT 256

/* PROTOTYPES *****************************************************************/

/* active.c */
BOOL active_main(INT argc, WCHAR **argv);
VOID help_active(INT argc, WCHAR **argv);

/* add.c */
BOOL add_main(INT argc, WCHAR **argv);
VOID help_add(INT argc, WCHAR **argv);

/* assign.c */
BOOL assign_main(INT argc, WCHAR **argv);
VOID help_assign(INT argc, WCHAR **argv);

/* attach.c */
BOOL attach_main(INT argc, WCHAR **argv);
VOID help_attach(INT argc, WCHAR **argv);

/* attributes.h */
VOID help_attributes(INT argc, WCHAR **argv);
BOOL attributes_main(INT argc, WCHAR **argv);

/* automount.c */
BOOL automount_main(INT argc, WCHAR **argv);
VOID help_automount(INT argc, WCHAR **argv);

/* break.c */
BOOL break_main(INT argc, WCHAR **argv);
VOID help_break(INT argc, WCHAR **argv);

/* clean.c */
BOOL clean_main(INT argc, WCHAR **argv);
VOID help_clean(INT argc, WCHAR **argv);

/* compact.c */
BOOL compact_main(INT argc, WCHAR **argv);
VOID help_compact(INT argc, WCHAR **argv);

/* convert.c */
BOOL convert_main(INT argc, WCHAR **argv);
VOID help_convert(INT argc, WCHAR **argv);

/* create.c */
BOOL create_main(INT argc, WCHAR **argv);
VOID help_create(INT argc, WCHAR **argv);

/* delete.c */
BOOL delete_main(INT argc, WCHAR **argv);
VOID help_delete(INT argc, WCHAR **argv);

/* detach.c */
BOOL detach_main(INT argc, WCHAR **argv);
VOID help_detach(INT argc, WCHAR **argv);

/* detail.c */
BOOL detail_main(INT argc, WCHAR **argv);
VOID help_detail(INT argc, WCHAR **argv);

/* diskpart.c */
VOID PrintResourceString(INT resID, ...);

/* expand.c */
BOOL expand_main(INT argc, WCHAR **argv);
VOID help_expand(INT argc, WCHAR **argv);

/* extend.c */
BOOL extend_main(INT argc, WCHAR **argv);
VOID help_extend(INT argc, WCHAR **argv);

/* filesystem.c */
BOOL filesystems_main(INT argc, WCHAR **argv);
VOID help_filesystems(INT argc, WCHAR **argv);

/* format.c */
BOOL format_main(INT argc, WCHAR **argv);
VOID help_format(INT argc, WCHAR **argv);

/* gpt.c */
BOOL gpt_main(INT argc, WCHAR **argv);
VOID help_gpt(INT argc, WCHAR **argv);

/* help.c */
BOOL help_main(INT argc, WCHAR **argv);
VOID help_help(INT argc, WCHAR **argv);
VOID help_cmdlist(VOID);
VOID help_print_noerr(VOID);

/* import. c */
BOOL import_main(INT argc, WCHAR **argv);
VOID help_import(INT argc, WCHAR **argv);

/* inactive.c */
BOOL inactive_main(INT argc, WCHAR **argv);
VOID help_inactive(INT argc, WCHAR **argv);

/* interpreter.c */
BOOL InterpretScript(WCHAR *line);
BOOL InterpretCmd(INT argc, WCHAR **argv);
VOID InterpretMain(VOID);

/* list.c */
BOOL list_main(INT argc, WCHAR **argv);
VOID help_list(INT argc, WCHAR **argv);

/* merge.c */
BOOL merge_main(INT argc, WCHAR **argv);
VOID help_merge(INT argc, WCHAR **argv);

/* offline.c */
BOOL offline_main(INT argc, WCHAR **argv);
VOID help_offline(INT argc, WCHAR **argv);

/* online.c */
BOOL online_main(INT argc, WCHAR **argv);
VOID help_online(INT argc, WCHAR **argv);

/* recover.c */
BOOL recover_main(INT argc, WCHAR **argv);
VOID help_recover(INT argc, WCHAR **argv);

/* remove.c */
BOOL remove_main(INT argc, WCHAR **argv);
VOID help_remove(INT argc, WCHAR **argv);

/* repair.c */
BOOL repair_main(INT argc, WCHAR **argv);
VOID help_repair(INT argc, WCHAR **argv);

/* rescan.c */
BOOL rescan_main(INT argc, WCHAR **argv);
VOID help_rescan(INT argc, WCHAR **argv);

/* retain.c */
BOOL retain_main(INT argc, WCHAR **argv);
VOID help_retain(INT argc, WCHAR **argv);

/* san.c */
BOOL san_main(INT argc, WCHAR **argv);
VOID help_san(INT argc, WCHAR **argv);

/* select.c */
BOOL select_main(INT argc, WCHAR **argv);
VOID help_select(INT argc, WCHAR **argv);

/* setid.c */
BOOL setid_main(INT argc, WCHAR **argv);
VOID help_setid(INT argc, WCHAR **argv);

/* shrink.c */
BOOL shrink_main(INT argc, WCHAR **argv);
VOID help_shrink(INT argc, WCHAR **argv);

/* uniqueid.c */
BOOL uniqueid_main(INT argc, WCHAR **argv);
VOID help_uniqueid(INT argc, WCHAR **argv);

#endif /* DISKPART_H */
