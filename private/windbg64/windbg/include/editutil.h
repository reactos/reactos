/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/***********************************************************************

    EDITUTIL.H

    Interface routines to the editor.

***********************************************************************/

// prototypes
void PASCAL InsertEditLine(
    int doc,
    LPSTR EditText,
    int y,
    BOOL VisualUpdate);

void PASCAL DeleteEditLine(
    int doc,
    int y,
    BOOL VisualUpdate);

void PASCAL AddEditLine(
    int doc,
    LPSTR EditText,
    BOOL VisualUpdate);

void PASCAL ReplaceEditLine(
    int doc,
    LPSTR EditText,
    int y,
    BOOL VisualUpdate);

void PASCAL SetDocLines(
    int doc,
    int lines);
