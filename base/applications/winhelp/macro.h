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

#ifndef __MACRO_H__
#define __MACRO_H__

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

BOOL MACRO_ExecuteMacro(LPCSTR);
int  MACRO_Lookup(const char* name, struct lexret* lr);

enum token_types {EMPTY, VOID_FUNCTION, BOOL_FUNCTION, INTEGER, STRING, IDENTIFIER};
void CALLBACK MACRO_About(void);
void CALLBACK MACRO_AddAccelerator(LONG, LONG, LPCSTR);
void CALLBACK MACRO_ALink(LPCSTR, LONG, LPCSTR);
void CALLBACK MACRO_Annotate(void);
void CALLBACK MACRO_AppendItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_Back(void);
void CALLBACK MACRO_BackFlush(void);
void CALLBACK MACRO_BookmarkDefine(void);
void CALLBACK MACRO_BookmarkMore(void);
void CALLBACK MACRO_BrowseButtons(void);
void CALLBACK MACRO_ChangeButtonBinding(LPCSTR, LPCSTR);
void CALLBACK MACRO_ChangeEnable(LPCSTR, LPCSTR);
void CALLBACK MACRO_ChangeItemBinding(LPCSTR, LPCSTR);
void CALLBACK MACRO_CheckItem(LPCSTR);
void CALLBACK MACRO_CloseSecondarys(void);
void CALLBACK MACRO_CloseWindow(LPCSTR);
void CALLBACK MACRO_Compare(LPCSTR);
void CALLBACK MACRO_Contents(void);
void CALLBACK MACRO_ControlPanel(LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_CopyDialog(void);
void CALLBACK MACRO_CopyTopic(void);
void CALLBACK MACRO_CreateButton(LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_DeleteItem(LPCSTR);
void CALLBACK MACRO_DeleteMark(LPCSTR);
void CALLBACK MACRO_DestroyButton(LPCSTR);
void CALLBACK MACRO_DisableButton(LPCSTR);
void CALLBACK MACRO_DisableItem(LPCSTR);
void CALLBACK MACRO_EnableButton(LPCSTR);
void CALLBACK MACRO_EnableItem(LPCSTR);
void CALLBACK MACRO_EndMPrint(void);
void CALLBACK MACRO_ExecFile(LPCSTR, LPCSTR, LONG, LPCSTR);
void CALLBACK MACRO_ExecProgram(LPCSTR, LONG);
void CALLBACK MACRO_Exit(void);
void CALLBACK MACRO_ExtAbleItem(LPCSTR, LONG);
void CALLBACK MACRO_ExtInsertItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR, LONG, LONG);
void CALLBACK MACRO_ExtInsertMenu(LPCSTR, LPCSTR, LPCSTR, LONG, LONG);
BOOL CALLBACK MACRO_FileExist(LPCSTR);
void CALLBACK MACRO_FileOpen(void);
void CALLBACK MACRO_Find(void);
void CALLBACK MACRO_Finder(void);
void CALLBACK MACRO_FloatingMenu(void);
void CALLBACK MACRO_Flush(void);
void CALLBACK MACRO_FocusWindow(LPCSTR);
void CALLBACK MACRO_Generate(LPCSTR, LONG, LONG);
void CALLBACK MACRO_GotoMark(LPCSTR);
void CALLBACK MACRO_HelpOn(void);
void CALLBACK MACRO_HelpOnTop(void);
void CALLBACK MACRO_History(void);
void CALLBACK MACRO_IfThen(BOOL, LPCSTR);
void CALLBACK MACRO_IfThenElse(BOOL, LPCSTR, LPCSTR);
BOOL CALLBACK MACRO_InitMPrint(void);
void CALLBACK MACRO_InsertItem(LPCSTR, LPCSTR, LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_InsertMenu(LPCSTR, LPCSTR, LONG);
BOOL CALLBACK MACRO_IsBook(void);
BOOL CALLBACK MACRO_IsMark(LPCSTR);
BOOL CALLBACK MACRO_IsNotMark(LPCSTR);
void CALLBACK MACRO_JumpContents(LPCSTR, LPCSTR);
void CALLBACK MACRO_JumpContext(LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_JumpHash(LPCSTR, LPCSTR, LONG);
void CALLBACK MACRO_JumpHelpOn(void);
void CALLBACK MACRO_JumpID(LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_JumpKeyword(LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_KLink(LPCSTR, LONG, LPCSTR, LPCSTR);
void CALLBACK MACRO_Menu(void);
void CALLBACK MACRO_MPrintHash(LONG);
void CALLBACK MACRO_MPrintID(LPCSTR);
void CALLBACK MACRO_Next(void);
void CALLBACK MACRO_NoShow(void);
void CALLBACK MACRO_PopupContext(LPCSTR, LONG);
void CALLBACK MACRO_PopupHash(LPCSTR, LONG);
void CALLBACK MACRO_PopupId(LPCSTR, LPCSTR);
void CALLBACK MACRO_PositionWindow(LONG, LONG, LONG, LONG, LONG, LPCSTR);
void CALLBACK MACRO_Prev(void);
void CALLBACK MACRO_Print(void);
void CALLBACK MACRO_PrinterSetup(void);
void CALLBACK MACRO_RegisterRoutine(LPCSTR, LPCSTR, LPCSTR);
void CALLBACK MACRO_RemoveAccelerator(LONG, LONG);
void CALLBACK MACRO_ResetMenu(void);
void CALLBACK MACRO_SaveMark(LPCSTR);
void CALLBACK MACRO_Search(void);
void CALLBACK MACRO_SetContents(LPCSTR, LONG);
void CALLBACK MACRO_SetHelpOnFile(LPCSTR);
void CALLBACK MACRO_SetPopupColor(LONG, LONG, LONG);
void CALLBACK MACRO_ShellExecute(LPCSTR, LPCSTR, LONG, LONG, LPCSTR, LPCSTR);
void CALLBACK MACRO_ShortCut(LPCSTR, LPCSTR, LONG, LONG, LPCSTR);
void CALLBACK MACRO_TCard(LONG);
void CALLBACK MACRO_Test(LONG);
BOOL CALLBACK MACRO_TestALink(LPCSTR);
BOOL CALLBACK MACRO_TestKLink(LPCSTR);
void CALLBACK MACRO_UncheckItem(LPCSTR);
void CALLBACK MACRO_UpdateWindow(LPCSTR, LPCSTR);

/* Local Variables:    */
/* c-file-style: "GNU" */
/* End:                */

#endif /* __MACRO_H__ */
