/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    environ.c

Abstract:

    This file contains the code to deal with Options.Environment dialog
    box.

Author:

    Jim Schaad (jimsch)

Environment:

    Win32 - User

--*/

#include "precomp.h"
#pragma hdrstop

int NEAR PASCAL ChangeTabs(int doc, int newTabSize);


/***    ChangeTabs
**
**  Synopsis:
**      int = ChangeTabs(doc, newTabSize)
**
**  Entry:
**      doc     - Document to change tabs in
**      newTabSize - New size to set the tabs to
**
**  Returns:
**      0 if tab width is ok else the line number for which the
**      line size is too great
**
**  Description:
**
*/

int NEAR PASCAL ChangeTabs(int doc, int newTabSize)
{
    LPLINEREC    pl;
    LPBLOCKDEF   pb;
    long         y;
    register int i;
    register int j;
    int          len;

    y = 0;

    //
    //  Check expanded len of all tabs
    //
    if (!FirstLine(doc, &pl, &y, &pb)) {
        return 0;
    }

    while (TRUE) {

        len = pl->Length - LHD;

        if (len > 0) {

            i = 0;
            j = 0;

            //
            //  Compute len of line expanded with tabs
            //
            while (i < len) {
                if (pl->Text[i] == TAB) {
                    j += newTabSize - (j % newTabSize);
                } else {
                    j++;
                }
                i++;
            }

            if (j > MAX_USER_LINE) {
                CloseLine(doc, &pl, y, &pb);
                return y;
            }
        }

        if (y >= Docs[doc].NbLines) {
            break;
        } else {
            if (!NextLine(doc, &pl, &y, &pb)) {
                return 0;
            }
        }
    }

    CloseLine(doc, &pl, y, &pb);
    return 0;
}
