/*
 * Help Viewer
 *
 * Copyright 1996 Ulrich Schmid
 * Copyright 2002 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"

struct lexret {
  LPCSTR        proto;
  BOOL          bool;
  LONG          integer;
  LPCSTR        string;
  FARPROC       function;
};

extern struct lexret yylval;
struct tagWinHelp;

BOOL            MACRO_ExecuteMacro(struct tagWinHelp*, LPCSTR);
int             MACRO_Lookup(const char* name, struct lexret* lr);
struct tagWinHelp* MACRO_CurrentWindow(void);

enum token_types {EMPTY, VOID_FUNCTION, BOOL_FUNCTION, INTEGER, STRING, IDENTIFIER};
void CALLBACK MACRO_About(void);
void CALLBACK MACRO_Annotate(void);
void CALLBACK MACRO_BookmarkDefine(void);
void CALLBACK MACRO_CopyDialog(void);
void CALLBACK MACRO_CreateButton(LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_DisableButton(LPCSTR);
void CALLBACK MACRO_Exit(void);
void CALLBACK MACRO_FileOpen(void);
void CALLBACK MACRO_HelpOn(void);
void CALLBACK MACRO_HelpOnTop(void);
void CALLBACK MACRO_History(void);
void CALLBACK MACRO_JumpContents(LPCSTR, LPCSTR);
void CALLBACK MACRO_JumpContext(LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_JumpHash(LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_PopupContext(LPCSTR, LONG);
void CALLBACK MACRO_Print(void);
void CALLBACK MACRO_PrinterSetup(void);
void CALLBACK MACRO_SetContents(LPCSTR, LONG);

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */
