/*
 *  CMDTABLE.C - table of internal commands.
 *
 *
 *  History:
 *
 *    16 Jul 1998 (Hans B Pufal)
 *        started.
 *        New file to keep the internal command table. I plan on
 *        getting rid of the table real soon now and replacing it
 *        with a dynamic mechnism.
 *
 *    27 Jul 1998  John P. Price
 *        added config.h include
 *
 *    21-Jan-1999 (Eric Kohl <ekohl@abo.rhein-zeitung.de>)
 *        Unicode ready!
 */

#include "config.h"

#include <windows.h>
#include <tchar.h>

#include "cmd.h"

/* a list of all the internal commands, associating their command names */
/* to the functions to process them                                     */


COMMAND cmds[] =
{
	{_T("?"), 0, CommandShowCommands},

#ifdef INCLUDE_CMD_ACTIVATE
	{_T("activate"),   0, CommandActivate},
#endif

#ifdef FEATURE_ALIASES
	{_T("alias"), 0, CommandAlias},
#endif

#ifdef INCLUDE_CMD_ATTRIB
	{_T("attrib"),   0, CommandAttrib},
#endif

#ifdef INCLUDE_CMD_BEEP
	{_T("beep"),     0, cmd_beep},
#endif

	{_T("call"), CMD_BATCHONLY, cmd_call},

#ifdef INCLUDE_CMD_CHDIR
	{_T("cd"), CMD_SPECIAL, cmd_chdir},
	{_T("chdir"), CMD_SPECIAL, cmd_chdir},
#endif

#ifdef INCLUDE_CMD_CHCP
	{_T("chcp"), 0, CommandChcp},
#endif

#ifdef INCLUDE_CMD_CHOICE
	{_T("choice"), 0, CommandChoice},
#endif


#ifdef INCLUDE_CMD_CLS
	{_T("cls"), 0, cmd_cls},
#endif

#ifdef INCLUDE_CMD_COLOR
	{_T("color"),    0, cmd_color},
#endif

#ifdef INCLUDE_CMD_COPY
	{_T("copy"),     0, cmd_copy},
#endif

#ifdef INCLUDE_CMD_DATE
	{_T("date"),     0, cmd_date},
#endif

#ifdef INCLUDE_CMD_DEL
	{_T("del"),      0, CommandDelete},
	{_T("delete"),   0, CommandDelete},
#endif

#ifdef INCLUDE_CMD_DELAY
	{_T("delay"), 0, CommandDelay},
#endif

#ifdef INCLUDE_CMD_DIR
	{_T("dir"), CMD_SPECIAL, cmd_dir},
#endif

#ifdef FEATURE_DIRECTORY_STACK
        {_T("dirs"), 0, CommandDirs},
#endif

	{_T("echo"), 0, CommandEcho},
	{_T("echos"), 0, CommandEchos},
	{_T("echoerr"), 0, CommandEchoerr},
	{_T("echoserr"), 0, CommandEchoserr},

#ifdef INCLUDE_CMD_DEL
	{_T("erase"), 0, CommandDelete},
#endif

	{_T("exit"), 0, CommandExit},

	{_T("for"), 0, cmd_for},

#ifdef INCLUDE_CMD_FREE
	{_T("free"), 0, CommandFree},
#endif

	{_T("goto"), CMD_BATCHONLY, cmd_goto},

	{_T("if"), 0, cmd_if},

#ifdef INCLUDE_CMD_LABEL
	{_T("label"), 0, cmd_label},
#endif

#ifdef INCLUDE_CMD_MEMORY
	{_T("memory"), 0, CommandMemory},
#endif

#ifdef INCLUDE_CMD_MKDIR
	{_T("md"), CMD_SPECIAL, cmd_mkdir},
	{_T("mkdir"), CMD_SPECIAL, cmd_mkdir},
#endif

#ifdef INCLUDE_CMD_MOVE
	{_T("move"), 0, cmd_move},
#endif

#ifdef INCLUDE_CMD_MSGBOX
	{_T("msgbox"), 0, CommandMsgbox},
#endif

#ifdef INCLUDE_CMD_PATH
	{_T("path"), 0, cmd_path},
#endif

#ifdef INCLUDE_CMD_PAUSE
	{_T("pause"), 0, cmd_pause},
#endif

#ifdef FEATURE_DIRECTORY_STACK
	{_T("popd"), 0, CommandPopd},
#endif

#ifdef INCLUDE_CMD_PROMPT
	{_T("prompt"), 0, cmd_prompt},
#endif

#ifdef FEATURE_DIRECTORY_STACK
	{_T("pushd"), 0, CommandPushd},
#endif

#ifdef INCLUDE_CMD_RMDIR
	{_T("rd"), CMD_SPECIAL, cmd_rmdir},
#endif

#ifdef INCLUDE_CMD_REM
	{_T("rem"), 0, CommandRem},
#endif

#ifdef INCLUDE_CMD_RENAME
	{_T("ren"), 0, cmd_rename},
	{_T("rename"), 0, cmd_rename},
#endif

#ifdef INCLUDE_CMD_RMDIR
	{_T("rmdir"), CMD_SPECIAL, cmd_rmdir},
#endif

#ifdef INCLUDE_CMD_SCREEN
	{_T("screen"), 0, CommandScreen},
#endif

#ifdef INCLUDE_CMD_SET
	{_T("set"), 0, cmd_set},
#endif

	{_T("shift"), CMD_BATCHONLY, cmd_shift},

#ifdef INCLUDE_CMD_START
	{_T("start"), 0, cmd_start},
#endif

#ifdef INCLUDE_CMD_TIME
	{_T("time"), 0, cmd_time},
#endif

#ifdef INCLUDE_CMD_TIMER
	{_T("timer"), 0, CommandTimer},
#endif

#ifdef INCLUDE_CMD_TITLE
	{_T("title"), 0, cmd_title},
#endif

#ifdef INCLUDE_CMD_TYPE
	{_T("type"), 0, cmd_type},
#endif

#ifdef INCLUDE_CMD_VER
	{_T("ver"), 0, cmd_ver},
#endif

#ifdef INCLUDE_CMD_VERIFY
	{_T("verify"), 0, cmd_verify},
#endif

#ifdef INCLUDE_CMD_VOL
	{_T("vol"), 0, cmd_vol},
#endif

#ifdef INCLUDE_CMD_WINDOW
	{_T("window"), 0, CommandWindow},
#endif

	{NULL, 0, NULL}
};

/* EOF */
