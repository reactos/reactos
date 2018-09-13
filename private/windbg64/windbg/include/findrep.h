/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
void FAR PASCAL SetStopLimit();

BOOL FindNext(
             HWND hwndParent,
             long startingLine,
             int startingCol,
             BOOL startFromSelection,
             BOOL selectFoundText,
             BOOL errorIfNotFound);

void FAR PASCAL Find();

void FAR PASCAL ReplaceOne();

void FAR PASCAL ReplaceAll(HWND);

void FAR PASCAL Replace(HWND);

BOOL FAR PASCAL InsertInPickList(WORD type);

void FAR PASCAL RemoveFromPick(WORD type);

void FAR PASCAL TagAll(HWND, int y);
