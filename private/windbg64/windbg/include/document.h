/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
//Initialize document module
void InitializeDocument(void);

void FAR PASCAL EnableReadOnlyBeep( BOOL f );

BOOL FAR PASCAL QueryReadOnlyError( void );

BOOL FAR PASCAL FirstLine(int doc, LPLINEREC *pl, long *lineNb, LPBLOCKDEF *pb);

BOOL FAR PASCAL NextLine(int doc, LPLINEREC *pl, long *lineNb, LPBLOCKDEF *pb);

BOOL FAR PASCAL PreviousLine(int doc, LPLINEREC *pl, long lineNb, LPBLOCKDEF *pb);

void FAR PASCAL CloseLine(int doc, LPLINEREC *pl, long lineNb, LPBLOCKDEF *pb);

int FAR PASCAL FirstNonBlank(int doc, long line);

//Delete a block from Col1,Line1 (inclusive) to Col2,Line2 (exclusive)
BOOL FAR PASCAL DeleteBlock(
    int doc,
    int col1,
    long line1,
    int col2,
    long line2);

//Delete all text
BOOL FAR PASCAL DeleteAll(
    int doc);

//Insert a block at Col,Line
BOOL FAR PASCAL InsertBlock( int doc, int col, long line, int size, LPSTR buf);

//Replace a char in a block
BOOL FAR PASCAL ReplaceCharInBlock(
    int doc,
    int x,
    long y,
    int ch);

//Count chars in document
ULONG FAR PASCAL CountCharsInDocument(
    int doc);

//Compact document in mem
BOOL FAR PASCAL CompactDocument(
    int doc);

//Check document coherence
BOOL FAR PASCAL CheckDocument(
    int doc);

void FAR PASCAL DumpDocument(
    int doc);

//Destroy memory allocated for a document
BOOL FAR PASCAL DestroyDocument(
    int doc);

//Allocate a block for document data
BOOL FAR PASCAL AllocateBlock(
    LPBLOCKDEF pPrevBlock,
    LPBLOCKDEF pNextBlock,
    LPBLOCKDEF *pNewBlock);

//Retrieve chars on a line
BOOL FAR PASCAL GetTextAtLine(
    int doc,
    long line,
    int col1,
    int col2,
    LPSTR text);

//Change tabulations in a document
void FAR PASCAL SetTabs(
    int doc,
    int oldTab,
    int newTab);

BOOL FAR PASCAL CreateNewBlock(
    LPBLOCKDEF *pNewBlock);

LPSTR FAR PASCAL DocAlloc(
    WORD bytes);

BOOL FAR PASCAL DocFree(
    LPVOID lpv);

int FAR PASCAL AlignToTabs(
    int editCol,
    int len,
    LPSTR pc);

void FAR PASCAL ExpandTabs(
    LPLINEREC *pl);

void FAR PASCAL CheckSyntax(
    int doc);
