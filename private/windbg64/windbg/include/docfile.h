/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
//Create or open a new document
int FAR PASCAL OpenDocument(WORD mode, 
                            WORD type, 
                            int doc, 
                            LPSTR FileName, 
                            int docView, 
                            int Preference,
                            BOOL bUserActivated);

// BUGBUG - dead code - kcarlos
#if 0
//Save a document with a specific name
BOOL FAR PASCAL SaveDocument(
    int Doc,
    LPSTR FileName);
#endif

// BUGBUG - dead code - kcarlos
#if 0
BOOL FAR PASCAL MergeFile(
    LPSTR   FileName,
    int view);
#endif