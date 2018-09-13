/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    editutil.c

Abstract:

    This file contains the codes to interface into the edit manager

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop


/***    InsertEditLine
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
**      NB inserts above y
*/

void PASCAL InsertEditLine(int doc, LPSTR EditText, int y, BOOL VisualUpdate)
{
    int EditLen;

    EditLen = strlen(EditText);

    InsertBlock(doc, 0, y, EditLen, EditText);
    if (strcmp(EditText+EditLen-2, CrLf) != 0)
    {
        // Ensure CrlLf
        InsertBlock(doc, (int)EditLen, y, sizeof(CrLf), (LPSTR)CrLf);
    }

    SetVerticalScrollBar(Docs[doc].FirstView, FALSE);

    if (VisualUpdate)
    {
        InvalidateLines(Docs[doc].FirstView, y, LAST_LINE, FALSE);
    }
    return;
}                                       /* InsertEditLine() */

/***    DeleteEditLine
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

void PASCAL DeleteEditLine(int doc, int y, BOOL VisualUpdate)
{
    DeleteBlock(doc, 0, y, 0, y+1);

    SetVerticalScrollBar(Docs[doc].FirstView, FALSE);

    if (VisualUpdate)
    {
        InvalidateLines(Docs[doc].FirstView, y, LAST_LINE, FALSE);
    }
    return;
}                                       /* DeleteEditLine() */

/***    AddEditLine
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

void PASCAL AddEditLine(int doc, LPSTR EditText, BOOL VisualUpdate)
{
    char szBuffer[MAX_USER_LINE+1];
    int lastLine;
    int lastLineLen;

    // Is there any text on the last line?
    lastLine = Docs[doc].NbLines-1;
    Dbg(GetTextAtLine(doc, lastLine,    0, sizeof(szBuffer)-1, szBuffer));
    lastLineLen = strlen(szBuffer);
    if (lastLineLen != 0)
    {
        // We need to append a CrLf to the last line first
        InsertBlock(doc, lastLineLen, lastLine, sizeof(CrLf), (LPSTR)CrLf);
        // And then replace the text at the new last line
        InsertBlock(doc, 0, lastLine+1, strlen(EditText), EditText);
    }
    else
    {
        // We just insert above the last line
        InsertEditLine(doc, EditText, lastLine, VisualUpdate);
    }

    SetVerticalScrollBar(Docs[doc].FirstView, FALSE);
    return;
}                                       /* AddEditLine() */

/***    ReplaceEditLine
**
**  Synopsis:
**      void = ReplaceEditLine(doc, EditText, y, VisualUpdate)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

void PASCAL ReplaceEditLine(int doc, LPSTR EditText, int y, BOOL VisualUpdate)
{
    DeleteBlock(doc, 0, y, MAX_USER_LINE, y);
    InsertBlock(doc, 0, y, strlen(EditText), EditText);

    SetVerticalScrollBar(Docs[doc].FirstView, FALSE);

    if (VisualUpdate)
    {
        InvalidateLines(Docs[doc].FirstView, y, LAST_LINE, FALSE);
    }
    return;
}                                       /* ReplaceEditLine() */


/***    SetDocLines
**
**  Synopsis:
**
**  Entry:
**
**  Returns:
**
**  Description:
**      Sets the number of lines in a document accordingly
**      This zaps the current contents
**
*/

void PASCAL SetDocLines(int doc, int lines)
{
    int i;
    int curLines;

    Assert(lines > 0);

    curLines = Docs[doc].NbLines;

    if (lines > curLines)
    {
        // Add some at the end
        for (i = 0; i < lines-curLines; i++)
        {
            AddEditLine(doc, szNull, FALSE);
        }
    }
    else if (lines < curLines)
    {
        // Take them off from the end.

        DeleteBlock(doc, 0, lines-1, MAX_USER_LINE, curLines-1);
        SetVerticalScrollBar(Docs[doc].FirstView, FALSE);
    }
    return;
}                                       /* SetDocLines() */
