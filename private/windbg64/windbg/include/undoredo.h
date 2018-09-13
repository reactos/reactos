/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
BOOL
PASCAL
CreateRecBuf(
    int doc,
    BYTE recType,
    long bytes
    );

BOOL
PASCAL
DestroyRecBuf(
    int doc,
    WORD recType
    );

BOOL
PASCAL
ResizeRecBuf(
    int doc,
    long bytes
    );

BOOL
PASCAL
OpenRec(
    int doc,
    BYTE action,
    int col,
    int line
    );

void
PASCAL
CloseRec(
    int doc,
    int col,
    int line,
    BOOL keepRec
    );

BOOL
PASCAL
AppendToRec(
    int doc,
    LPSTR chars,
    int size,
    BOOL isLine,
    int *totalSize
    );

BOOL
PASCAL
PlayRec(
    int doc,
    WORD recType,
    BOOL untilUserMark,
    BOOL prompt
    );

void
PASCAL
ReadLineFromBuf(
    LPUNDOREDOREC p,
    LPSTR dest,
    int *size,
    int *expandedLen,
    LPSTR *charsEnd
    );

BOOL
PASCAL
CheckRecBuf(
    int doc,
    WORD recType
    );

void
PASCAL
DumpRec(
    int doc,
    WORD recType
    );
