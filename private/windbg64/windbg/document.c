/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    document.c

Abstract:

    This file contains the code which deals with the underlying document
    file.

Author:

    Jim Schaad (jimsch)
    Griffith Wm. Kadnier (v-griffk) 1-Nov-1992

Environment:

    Win32 - User

--*/



#include "precomp.h"
#pragma hdrstop

#define ROREGION(d,X,Y) ((d)->RORegionSet && ((Y)<(d)->RoY2 || ((Y)==(d)->RoY2 && (X)<(d)->RoX2)))


//Prototypes

void NEAR PASCAL ReadOnlyBeep (void);
LPBLOCKDEF NEAR PASCAL FreeBlock (int doc, LPBLOCKDEF pCurBlock);
void NEAR PASCAL ModifiedDoc (int doc);
BOOL NEAR PASCAL FindInsertSpace (int doc, LPBLOCKDEF *pCurBlock, LPLINEREC *pCurLine, LPSTR pBlockEnd, int sizeExpansion);
BOOL NEAR PASCAL CancelDelete (int doc);
BOOL NEAR PASCAL PartialLineDelete (int doc, LPBLOCKDEF *pCurBlock, LPLINEREC *pCurLine, int *totalSize, int col1, int col2, BOOL moveToNextLine);
BOOL NEAR PASCAL FullLineDelete (int doc, LPBLOCKDEF *pCurBlock, LPLINEREC *pCurLine, int *totalSize);
BOOL NEAR PASCAL JoinLines (int doc, LPBLOCKDEF pCurBlock, LPLINEREC pCurLine, int *totalSize, int lineNb, int col1);


/****************************************************************************
                     Document Structure in memory
                     ----------------------------
              (Use TABULATION = 3 if drawing looks bad)
- Blocks are allocated using malloc

                                                 Line status (tags,
              ----------> error, breakpoint, <-
              |           in C comment, ...)  |
              |                               |
              |           Previous line       | ---------> Previous line
              | --------> length (in previous | |          length
              | |         Block for Line 0)   | |
              | |                             | |
              | | ------> Line Length         | | -------> Line length
              | | |                           | | |
              | | |   --> Chars  |            | | |  ----> Chars
              | | |   |          |            | | |  |
              | | |   |          v            | | |  |
  |  | | | |                         ...                            |  | | |
  |  | | | | |_|_|_|_________|  |_|_|...__|  |_|_|_|____| <-Unused->|  | | |
  |  | | | |   Line 0 Block n    Line...1   Line m Block n  space   |  | | |
__|  |_|_|_|_________________________...____________________________|  |_|_|__

 |    | | |                         Block n   ^                         |
 |    | | |                                   |                         |
 v    | | ---> Offset of Last line in block ---                         v
      | |
Block | |                                                             Block
n - 1 | -----> Address of Block n + 1 (0 if last block)               n + 1
      |
      |
      -------> Address of Block n - 1 (0 if first block)

****************************************************************************/

static BOOL fReadOnlyErrorStatus = FALSE;
static BOOL fReadOnlyBeepEnabled = TRUE;


void NEAR PASCAL
ReadOnlyBeep(
    void
    )
/*++

Routine Description:

    Report a readonly error condition; set flag and
    beep if the beep flag is set.

Arguments:

    none

Return Value:

    none

--*/
{
    fReadOnlyErrorStatus = TRUE;
    if (fReadOnlyBeepEnabled) {
        MessageBeep(0);
    }
}

void FAR PASCAL
EnableReadOnlyBeep(
    BOOL f
    )
/*++

Routine Description:

    Set or clear the beep flag.  Always clears the error status flag.

Arguments:

    f - supplies new flag value

Return Value:

    none

--*/
{
    fReadOnlyBeepEnabled = (f != 0);
    fReadOnlyErrorStatus = FALSE;
}

BOOL FAR PASCAL
QueryReadOnlyError(
    void
    )
/*++

Routine Description:

    Examine the read only error status flag.  Status flag is cleared
    after examining the value.

Arguments:

    None

Return Value:

    Value of readonly error status - TRUE or FALSE.

--*/
{
    BOOL f = fReadOnlyErrorStatus;
    fReadOnlyErrorStatus = FALSE;
    return f;
}

LPSTR FAR PASCAL
DocAlloc(
    WORD bytes
    )
{
    PSTR psz = (PSTR) malloc(bytes);
    if (psz) {
        ZeroMemory(psz, bytes);
    }
    return psz;
}

BOOL FAR PASCAL
DocFree(
    LPVOID lpv
    )
{
    free(lpv);
    return TRUE;
}

//Allocates and initialize a block, update doc pointers
BOOL FAR PASCAL
AllocateBlock(
    LPBLOCKDEF pPrevBlock,
    LPBLOCKDEF pNextBlock,
    LPBLOCKDEF *pNewBlock
    )
{
    //Allocate memory
    *pNewBlock = (LPBLOCKDEF)DocAlloc(sizeof(BLOCKDEF));
    if (*pNewBlock == NULL) {
        return ErrorBox(SYS_Allocate_Memory);
    }

    //Do initialization work
    (*pNewBlock)->NextBlock = pNextBlock;
    (*pNewBlock)->PrevBlock = pPrevBlock;
    (*pNewBlock)->LastLineOffset = 0;

    return TRUE;
}

//Free current block, link previous and next blocks, update doc pointers
LPBLOCKDEF NEAR PASCAL
FreeBlock(
    int doc,
    LPBLOCKDEF pCurBlock
    )
{
    LPBLOCKDEF nextBlock = pCurBlock->NextBlock;

    //Link previous block to next block
    if (pCurBlock->PrevBlock == NULL) {
        Docs[doc].FirstBlock = nextBlock;
    } else {
        pCurBlock->PrevBlock->NextBlock = nextBlock;
    }
    if (nextBlock == NULL) {
        Docs[doc].LastBlock = pCurBlock->PrevBlock;
    } else {
        pCurBlock->NextBlock->PrevBlock = pCurBlock->PrevBlock;
    }

    //Release memory
    if (!DocFree((LPSTR)pCurBlock)) {
        InternalErrorBox(SYS_Free_Memory);
    }

    return nextBlock;
}

/***    InitializeDocument
**
**  Synopsis:
**      void = InitializeDocument(void)
**
**  Entry:
**      none
**
**  Returns:
**      Nothing
**
**  Description:
**      Initialize global variables used by the module
*/

void
InitializeDocument(
    void
    )
{
    UINT i;
    for (i = 0; i < MAX_DOCUMENTS; i++) {
        Docs[i].FirstView = -1;
    }
    for (i = 0; i < MAX_VIEWS; i++) {
        Views[i].Doc = -1;
    }
}                                       /* InitializeDocument() */

/***    FirstLine
**
**  Synopsis:
**      bool = FirstLine(doc, pl, lineNb, pb)
**
**  Entry:
**      doc     - Current document index
**      pl      - Place where the line is to be stored
**      lineNb  - pointer to line to be searched (incremented after search)
**      pb      - pointer to block pointer to be returned
**
**  Returns:
**
**  Description:
**
*/

BOOL FAR PASCAL
FirstLine (
    int doc,
    LPLINEREC *pl,
    long *lineNb,
    LPBLOCKDEF *pb
    )
{
    LPLINEREC stop;
    int l;
    LPDOCREC d = &Docs[doc];

    // kcarlos
    // BUGBUG
    // This simple solves the symptom, but not the cause.
    // Assert(*lineNb < d->NbLines);
    if (*lineNb >= d->NbLines) {
        *lineNb = d->NbLines - 1;
    }

    //Exit if line was the last loaded
    if (*lineNb == d->CurrentLine) {
        *pb = d->CurrentBlock;
        *pl = (LPLINEREC)((LPSTR)(*pb)->Data + d->CurrentLineOffset);
        (*lineNb)++;

        //Expand line
        ExpandTabs(pl);
        pcl = *pl;
        pcb = *pb;

        return TRUE;
    }

    //First evaluate from which block do we start
    if (*lineNb <= (d->CurrentLine >> 1)) {

        //Go up from first line
        *pb = d->FirstBlock;

        l = *lineNb;
        *pl = (LPLINEREC)((*pb)->Data);
        stop = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);

        while (l--)     {

            //Get next line
            if (*pl < stop) {

                //We are below the last line of the block,
                // get just the next line
                *pl = (LPLINEREC) ((LPSTR)*pl + (*pl)->Length);

            } else {

                //We are at the last line of the block. Load the next block
                //and initialize line
                Assert(*pl == stop);
                Assert((*pb)->NextBlock != NULL);
                *pb = (*pb)->NextBlock;
                *pl = (LPLINEREC)((*pb)->Data);
                stop = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);
            }
        }
    } else if (*lineNb <= d->CurrentLine) {

        //Go down from current line
        *pb = d->CurrentBlock;

        l = d->CurrentLine - *lineNb;
        *pl = (LPLINEREC)((*pb)->Data + d->CurrentLineOffset);
        stop = (LPLINEREC)((*pb)->Data);

        while (l--)     {
            if (*pl > stop) {
                *pl = (LPLINEREC) ((LPSTR)*pl - (*pl)->PrevLength);
            } else {
                Assert(*pl == stop);
                Assert((*pb)->PrevBlock != NULL);
                *pb = (*pb)->PrevBlock;
                *pl = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);
                stop = (LPLINEREC)((*pb)->Data);
            }
        }
    // Kim Peterson
    // BUGBUG: Original expression without parens was always true
    // may be related to previous BUG
    } else if (*lineNb <= (d->CurrentLine + ((d->NbLines-d->CurrentLine) >> 1))) {

        //Go up from current line
        *pb = d->CurrentBlock;

        l = *lineNb - d->CurrentLine;
        *pl = (LPLINEREC)((*pb)->Data + d->CurrentLineOffset);
        stop = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);

        while (l--)     {
            if (*pl < stop) {
                *pl = (LPLINEREC) ((LPSTR)*pl + (*pl)->Length);
            } else {
                Assert(*pl == stop);
                Assert((*pb)->NextBlock != NULL);
                *pb = (*pb)->NextBlock;
                *pl = (LPLINEREC)((*pb)->Data);
                stop = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);
            }
        }
    }
    else {

        // Go down from last line
        *pb = d->LastBlock;
        l = d->NbLines - *lineNb -1;
        *pl = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);
        stop = (LPLINEREC)((*pb)->Data);

        while (l--)     {
            if (*pl > stop) {
                *pl = (LPLINEREC) ((LPSTR)*pl - (*pl)->PrevLength);
             } else {
                Assert(*pl == stop);
                Assert((*pb)->PrevBlock != NULL);
                *pb = (*pb)->PrevBlock;
                *pl = (LPLINEREC)((*pb)->Data + (*pb)->LastLineOffset);
                stop = (LPLINEREC)((*pb)->Data);
            }
        }
    }

    //Increment line count
    (*lineNb)++;

    //Expand line
    ExpandTabs(pl);
    pcl = *pl;
    pcb = *pb;

    return TRUE;
}                                       /* FirstLine() */

/***    NextLine
**
**  Synopsis:
**      bool = NextLine(doc, pl, lineno, bl, pb)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL FAR PASCAL
NextLine(
    int doc,
    LPLINEREC *pl,
    long *lineNb,
    LPBLOCKDEF *pb
    )
{
    LPDOCREC d = &Docs[doc];

    if (*lineNb >= d->NbLines) {
        *lineNb = LAST_LINE;
        return TRUE;
    }
    Assert(*lineNb >= 0);
    Assert(((LPSTR)*pl - (LPSTR)(*pb)->Data) <= (*pb)->LastLineOffset);

    //Return next line
    if ((LPSTR)*pl == (LPSTR)((*pb)->Data + (*pb)->LastLineOffset)) {

        //We are at the last line  of the block. Load next block

        Assert((*pb)->NextBlock != NULL);
        *pb = (*pb)->NextBlock;

        //First line of next block

        *pl = (LPLINEREC)((*pb)->Data);

        AssertAligned( *pl );
    }
    else {
        int i;

        //Get pointer to next line

        i = (*pl)->Length;
#ifdef ALIGN
        i = (i + 3) & ~3;
#endif

        *pl = (LPLINEREC)((LPSTR)*pl + i);

        AssertAligned( *pl );
    }

    (*lineNb)++;

    //Expand line
    ExpandTabs(pl);
    pcl = *pl;
    pcb = *pb;

    return TRUE;
}                                       /* NextLine() */

/***    PreviousLine
**
*/

BOOL FAR PASCAL
PreviousLine(
    int doc,
    LPLINEREC *pl,
    long lineNb,
    LPBLOCKDEF *pb
    )
{
    LPDOCREC d = &Docs[doc];

    if (lineNb <= 0) {
        return TRUE;
    }

    Assert(lineNb < d->NbLines);
    Assert(((LPSTR)(*pl)-(LPSTR)(*pb)->Data) <= (*pb)->LastLineOffset);

    //Return previous line
    if ((LPSTR)(*pl) == (LPSTR)((*pb)->Data))       {

        //We are at the first line  of the block. Load previous block
        Assert((*pb)->PrevBlock != NULL);
        *pb = (*pb)->PrevBlock;

        //Last line of previous block
        *pl = (LPLINEREC)((LPSTR)(*pb)->Data + (*pb)->LastLineOffset);
    } else {

        //Get pointer to previous line
        *pl = (LPLINEREC)((LPSTR)(*pl) - (*pl)->PrevLength);
    }

    //Expand line
    ExpandTabs(pl);
    pcl = *pl;
    pcb = *pb;

    return TRUE;
}                                       /* PreviousLine() */

/***    CloseLine
**
**  Synopsis:
**      void = CloseLine()
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

void FAR PASCAL
CloseLine (
    int doc,
    LPLINEREC *pl,
    long lineNb,
    LPBLOCKDEF *pb
    )
{
    LPDOCREC d = &Docs[doc];

    //Save current line infos

    d->CurrentBlock = *pb;
    d->CurrentLine = lineNb - 1;
    d->CurrentLineOffset = (int) ((LPSTR)(*pl) - (LPSTR)(*pb)->Data);

    return;
}                                       /* CloseLine() */

/***    FirstNonBlank
**
*/

int FAR PASCAL
FirstNonBlank(
    int doc,
    long line
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    register int x;

    if (!FirstLine(doc, &pl, &line, &pb)) {
        return FALSE;
    }
    CloseLine(doc, &pl, line, &pb);

    x = 0;
    while (x < elLen && el[x] == ' ') {
        x++;
    }
    if (x >= (elLen)) {
        x = 0;
    }

    return x;
}                                       /* FirstNonBlank() */

void FAR PASCAL
CheckSyntax(
    int doc
    )
{
    LPDOCREC    d = &Docs[doc];
    LPBLOCKDEF  pCurBlock;
    LPLINEREC   pCurLine;
    char        prevCh;
    char        ch;
    int         i;
    long        line = d->lineTop;
    int         savedLineBottom;
    int         curStatus, prevStatus;
    int         curLevel, prevLevel;

    if (d->lineBottom >= d->NbLines) {
        d->lineBottom--;
    }
    Assert(d->lineBottom < d->NbLines);
    savedLineBottom = d->lineBottom;

    //Load previous line to get previous line status

    if (line > 0) {
        line--;
        if (!FirstLine(doc, &pCurLine, &line, &pCurBlock)) {
            return;
        }

        //Compute previous line comment level and status

        prevStatus = pCurLine->Status & COMMENT_LINE;
        if (prevStatus == COMMENT_LINE) {
              prevLevel = 1;
        } else {

            for (i = 0, prevCh = ' ', prevLevel = 0;
                                             i < pCurLine->Length - LHD; i++) {
                if (IsDBCSLeadByte(pCurLine->Text[i]) && pCurLine->Text[i+1]) {
                      prevCh = ' ';
                      i++;
                      continue;
                }
                ch = pCurLine->Text[i];
                if (prevCh == '/' && ch == '*') {
                    prevLevel++;
                }
                if (prevCh == '*' && ch == '/') {
                    prevLevel = -1;
                }
                prevCh = ch;
            }
        }
        if (!NextLine(doc, &pCurLine, &line, &pCurBlock)) {
            return;
        }

    } else {
        Assert(line == 0);
        prevStatus = prevLevel = 0;
        if (!FirstLine(doc, &pCurLine, &line, &pCurBlock)) {
            return;
        }
    }

    //Scan now every line between lineTop and lineBottom and possibly
    //extend lineBottom if needed

    while (TRUE) {

        //Compute current line comment level and status

        curStatus = pCurLine->Status & COMMENT_LINE;
        for (i = 0, prevCh = ' ', curLevel = 0; i < pCurLine->Length - LHD;
                                                                         i++) {
            if (IsDBCSLeadByte(pCurLine->Text[i]) && pCurLine->Text[i+1]) {
                prevCh = ' ';
                i++;
                continue;
            }
            ch = pCurLine->Text[i];

            if (prevCh == '/' && ch == '*') {
                if (i <= 1 || pCurLine->Text[i-2] != '/') {
                    curLevel++;
                }
            }

            if (prevCh == '*' && ch == '/') {
                curLevel = -1;
            }
            prevCh = ch;
        }

        //Impossible combination

        Assert(prevStatus == 0 || prevLevel >= 0);

        //Set or reset multiline status

        if (prevStatus == 0 && prevLevel <= 0) {
            if (curLevel > 0) {
                SET(pCurLine->Status, COMMENT_LINE);
            } else {
                RESET(pCurLine->Status, COMMENT_LINE);
            }

        } else if (curLevel >= 0) {

            SET(pCurLine->Status, COMMENT_LINE);

        } else {

            RESET(pCurLine->Status, COMMENT_LINE);

        }

        //Current line info is now previous

        prevStatus = pCurLine->Status & COMMENT_LINE;

        //Check stop conditions

        if (line > d->lineBottom) {
            if (curStatus == prevStatus) {
                break;
            } else {
                d->lineBottom++;
            }
        }

        //Current level is previous one

        prevLevel = curLevel;

        //Stop if at end of doc

        if (line >= d->NbLines) {
            break;
        }

        //Return next line

        if ((LPSTR)pCurLine ==
                        (LPSTR)(pCurBlock->Data + pCurBlock->LastLineOffset)) {
            pCurBlock = pCurBlock->NextBlock;
            pCurLine = (LPLINEREC)(pCurBlock->Data);
        }
        else {
            pCurLine = (LPLINEREC)((LPSTR)pCurLine + pCurLine->Length);
        }
        line++;

    }

    if (!syntaxColors) {
        d->lineBottom = savedLineBottom;
    }

    CloseLine(doc, &pCurLine, line, &pCurBlock);

    return;
}                                       /* CheckSyntax() */

/***    ModifiedDoc
**
**  Synopsis:
**      void = ModifiedDoc(doc)
**
**  Entry:
**      doc     - index of document which has been modified
**
**  Returns:
**      Nothing:
**
**  Description:
**      Marks the document as having been modified and updates the
**      title if it has not previously been modified
**
*/

void NEAR PASCAL
ModifiedDoc(
    int doc
    )
{

    //Document is modified
    if (!Docs[doc].ismodified) {
        Docs[doc].ismodified = TRUE;
        RefreshWindowsTitle(doc);
    }
    return;
}                                       /* ModifiedDoc() */

void FAR PASCAL
ExpandTabs(
    LPLINEREC *pl
    )
{
    register int i = 0;
    register int j = 0;
    int len = (*pl)->Length - LHD;
    LPSTR pc = (*pl)->Text;

    memset(el, ' ', MAX_USER_LINE * 2);
    while (i < len) {
        if (IsDBCSLeadByte((BYTE)pc[i])) {
            el[j++] = pc[i++];
            el[j++] = pc[i++];
        } else
        if (pc[i] == TAB) {
            j += g_contGlobalPreferences_WkSp.m_nTabSize - (j % g_contGlobalPreferences_WkSp.m_nTabSize);
            i++;
        }
        else {
            el[j++] = pc[i++];
        }
    }

    Assert(j <= MAX_USER_LINE);
    elLen = j;
}

int FAR PASCAL
AlignToTabs(
    int editCol,
    int len,
    LPSTR pc
    )
{
    register int col, realCol;

    col = realCol = 0;
    while (realCol < editCol) {

        if (col >= len) {
            col += (editCol - realCol);
            break;
        }
        if (IsDBCSLeadByte(pc[col])) {
              realCol += 2;
        } else if (pc[col] == TAB) {
            realCol += g_contGlobalPreferences_WkSp.m_nTabSize - (realCol % g_contGlobalPreferences_WkSp.m_nTabSize);
        } else {
            realCol++;
        }

        if (realCol <= editCol) {
            if (IsDBCSLeadByte(pc[col])) {
                col += 2;
            } else {
                col++;
            }
        }
    }

    return col;
}                                       /* AlignToTabs() */

BOOL NEAR PASCAL
FindInsertSpace(
    int doc,
    LPBLOCKDEF *pCurBlock,
    LPLINEREC *pCurLine,
    LPSTR pBlockEnd,
    int sizeExpansion
    )
{
    LPBLOCKDEF  pNextBlock;
    LPLINEREC   pNextLineEnd, pTmpLine = *pCurLine;
    int         moveSize;
    BOOL        newBlockCreated = FALSE;
    int         curLineOffset;

    //Check not to move first line of current block in next block

    if ((LPSTR)*pCurLine - (LPSTR)(*pCurBlock)->Data == 0) {
        pTmpLine = (LPLINEREC)((LPSTR) pTmpLine + pTmpLine->Length);
    }

    moveSize = (int) (pBlockEnd - (LPSTR)pTmpLine);

    if ((*pCurBlock)->NextBlock == NULL) {

        //Create a new block

        if (!AllocateBlock(*pCurBlock, NULL, &pNextBlock)) {
            return FALSE;
        }
        newBlockCreated = TRUE;
        Docs[doc].LastBlock = pNextBlock;

        pNextLineEnd = (LPLINEREC)pNextBlock->Data;
    }
    else {

        //Before allocating a new block, look into next block
        //to see if we have space to insert chars and to move in
        //current line and lines after

        pNextBlock = (*pCurBlock)->NextBlock;

        pNextLineEnd = (LPLINEREC)(pNextBlock->Data
              + pNextBlock->LastLineOffset);
        pNextLineEnd = (LPLINEREC)((LPSTR)pNextLineEnd
              + pNextLineEnd->Length);

        if (moveSize + sizeExpansion > (int)(BLOCK_SIZE
                - ((LPSTR)pNextLineEnd - (LPSTR)pNextBlock->Data))) {

            LPBLOCKDEF pNewBlock;

            //We have not enough space in next block, so insert
            //a new created block

            if (!AllocateBlock(*pCurBlock, pNextBlock, &pNewBlock)) {
                return FALSE;
            }

            newBlockCreated = TRUE;
            pNextBlock->PrevBlock = pNewBlock;

            //Next block is new block

            pNextBlock = pNewBlock;
            pNextLineEnd = (LPLINEREC)pNextBlock->Data;
        }
    }

    curLineOffset = (int) ((LPSTR)pTmpLine - (LPSTR)(*pCurBlock)->Data);

    //Link current block to next
          (*pCurBlock)->NextBlock = pNextBlock;

    //Shift right next blocks chars

    memmove(pNextBlock->Data + moveSize,
          pNextBlock->Data,
          (size_t) ((LPSTR)pNextLineEnd - (LPSTR)pNextBlock->Data));

    //And copy chars from current block to next block

    memmove(pNextBlock->Data, (LPSTR)pTmpLine, moveSize);

    Assert(moveSize >= LHD);

    //Rework pointers

    if (newBlockCreated) {
        pNextBlock->LastLineOffset = (*pCurBlock)->LastLineOffset -
                                                                 curLineOffset;
    } else {
        pNextBlock->LastLineOffset += moveSize;
    }


    (*pCurBlock)->LastLineOffset = curLineOffset - pTmpLine->PrevLength;

    //Current block is next block if current line is not block first line

    if ((LPSTR)*pCurLine != (LPSTR)(*pCurBlock)->Data) {

        //Cur line is now next block first line

        *pCurLine = (LPLINEREC)pNextBlock->Data;

        //Next block becomes current block

        *pCurBlock = pNextBlock;

    }

    return TRUE;
}                                       /* FindInsertSpace() */

//When your file has an Undo buffer, be sure to set the undo/redo
//engine to UNDO before inserting chars, otherwise they will be stored
//in the REDO buffer

BOOL FAR PASCAL
InsertBlock(
    int doc,
    int col,
    long line,
    int size,
    LPSTR buf
    )
{
    LPLINEREC   pCurLine = NULL, pLastLine = NULL, pNextLine = NULL;
    LPBLOCKDEF  pCurBlock, pNextBlock;
    LPSTR       charsBegin = NULL;
    LPSTR       charsEnd = NULL;
    LPSTR       pBlockEnd = NULL;
    LPSTR       start = NULL;
    int         sizeExpansion, newLineLen, charsSize;
    int         extraBlanks, lineLen;
    int         nextLineLength;
    int         freeBlockSize;
    BOOL        addOneLine;
    LPDOCREC    d = &Docs[doc];
    UNDOREDOREC pUndoRedo;
    int         startLine = line;
    BOOL        firstLine = TRUE;
    int         nbLines = d->NbLines;
    char        lineBuf[MAX_LINE_SIZE];
    int         expandedLen;

    //Beep and exit if file is ReadOnly

    if (d->readOnly || ROREGION(d,col,line)) {
        ReadOnlyBeep();
        return FALSE;
    } else {
        // Clear error status
        QueryReadOnlyError();
    }

    if (size == 0) {
        return TRUE;
    }

    editorIsCritical = TRUE;
    Assert(line < d->NbLines);
    col = min(col, MAX_USER_LINE);

    d->lineTop = d->lineBottom = line;

    //Get information from line where we insert

    if (!FirstLine(doc, &pCurLine, &line, &pCurBlock)) {
        goto errRet;
    }

    //Align column respecting tabs

    if (playingRecords) {
          expandedLen = ConvertPosX(col);
    } else {
        expandedLen = col;
        col = AlignToTabs(expandedLen, pCurLine->Length - LHD,
                                                       (LPSTR)pCurLine->Text);
    }
    Assert(col >= 0);

    //Open an edit record

    if (!OpenRec(doc, DELETESTREAM, col, line - 1)) {
        goto errRet;
    }

    //Initialize rec buffer ptr if we will read chars from undo/redo record

    if (buf) {
        charsBegin = charsEnd = buf;
    } else if (d->recType == REC_UNDO) {
        pUndoRedo = d->redo;
    } else if (d->recType == REC_REDO) {
        pUndoRedo = d->undo;
    } else {
        Assert(FALSE);
        return FALSE;
    }

    //Insert until buffer empty

    while (size) {

        if (buf != NULL) {

            //Extract chars from supplied buf until EOF, EOL or end of buffer

            while (size && *charsEnd != CR) {
                if (*charsEnd == TAB) {
                    expandedLen += g_contGlobalPreferences_WkSp.m_nTabSize - (expandedLen % g_contGlobalPreferences_WkSp.m_nTabSize);
                } else {
                    expandedLen++;
                }
                charsEnd++;
                size--;
            }
        } else {

            //Extract from undo/redo records buffer a line, or the text if
            //no CR+LF found (no overflow protection needed, by definition
            //lines in undo/redo buf are never beyond maximum)

            charsBegin = (LPSTR)lineBuf;
            ReadLineFromBuf(&pUndoRedo, lineBuf, &size, &expandedLen,&charsEnd);
        }

        if (size == 0) {
            addOneLine = FALSE;
        } else if ( (addOneLine = (*charsEnd == CR)) &&
                                          d->NbLines >= MAX_LINE_NUMBER) {
            ErrorBox(ERR_Too_Many_Lines);
            goto errRet;
        }

        lineLen = pCurLine->Length - LHD;

        //We are in a line, check if we are after last char

        extraBlanks = max (col - lineLen, 0);

        //Compute new line len whether chars end with a CR or not

        newLineLen = 
        sizeExpansion = 
        charsSize = (int) (charsEnd - charsBegin);
        if (addOneLine) {
            newLineLen += col + LHD;
            sizeExpansion += LHD;

            //Adjust startline for breakpoints and error handling

            if (firstLine && col > 0) {
                startLine++;
            }
        } else {

            int newCol = col;

            newLineLen += (pCurLine->Length + extraBlanks);

            //Now see if what we insert in current line will not expand
            //line len beyond maximum

            if (extraBlanks == 0) {

                while (newCol < lineLen) {
                    if (pCurLine->Text[newCol] == TAB) {
                        expandedLen += g_contGlobalPreferences_WkSp.m_nTabSize - (expandedLen % g_contGlobalPreferences_WkSp.m_nTabSize);
                    } else {
                        expandedLen++;
                    }
                    newCol++;
                }

            }
        }

        //Check new line length

        if (expandedLen > MAX_USER_LINE) {
            ErrorBox(ERR_Line_Too_Long);
            goto errRet;
        }

        firstLine = FALSE;

        //Possibly add extraBlanks to buffer size expansion

        sizeExpansion += extraBlanks;

        //Sorry but recursivity eats stack

      reDoIt:

        //Keep a pointer on last line of block

        pLastLine = (LPLINEREC)((LPSTR)pCurBlock->Data +
                                                   pCurBlock->LastLineOffset);

        //Check block size

        freeBlockSize = BLOCK_SIZE - pCurBlock->LastLineOffset -
                                                             pLastLine->Length;

        //Compute very end of block

        pBlockEnd = (LPSTR)pLastLine + pLastLine->Length;
        Assert(pBlockEnd <= (LPSTR)(pCurBlock)->Data + BLOCK_SIZE);

        //Do we have room in current block

        if (freeBlockSize >= sizeExpansion) {

            //We have space to insert chars in current block

            //If we add a line, first update next line length

            if (addOneLine) {
                nextLineLength = pCurLine->Length - (BYTE)newLineLen
                        + (BYTE)sizeExpansion;
                pNextLine = (LPLINEREC)((LPSTR)pCurLine + pCurLine->Length);

                //Write future line length in next line prev length field,
                //if line is the last of block, load next block

                if (((LPSTR)pNextLine - (LPSTR)pCurBlock->Data)
                        <= pCurBlock->LastLineOffset) {
                    pNextLine->PrevLength = (BYTE)nextLineLength;
                } else {

                    //Write header only if there is a line after

                    pNextBlock = pCurBlock->NextBlock;
                    if (pNextBlock != NULL) {

                        //Set next line pointer in first line of Next Block

                        pNextLine = (LPLINEREC)pNextBlock->Data;
                        pNextLine->PrevLength = (BYTE)nextLineLength;
                    }
                }
            }

            //Compute the position where to insert

            start = (LPSTR)(pCurLine) + min(col + LHD, (int)pCurLine->Length);

            //Shift right part

            memmove(start + sizeExpansion, start, (size_t) (pBlockEnd - start));

            //First insert the extraBlanks if any

            memset(start, ' ', extraBlanks);
            start += extraBlanks;

            //Then insert chars

            memmove(start, charsBegin, charsSize);

            //Possibly adjust block last Line Offset
            // - We inserted before last line, propagate line extension
            // - We inserted at last line, propagate line extension
            //    only if a new line is inserted

            if (pCurLine < pLastLine) {
                  pCurBlock->LastLineOffset +=sizeExpansion;
            } else if (pCurLine == pLastLine) {
                if (addOneLine) {
                    pCurBlock->LastLineOffset +=newLineLen;
                }
            } else {
                Assert(FALSE);
                return FALSE;
            }

            pNextLine = (LPLINEREC)((LPSTR)pCurLine + newLineLen);
            Assert((LPSTR)pNextLine - (LPSTR)pCurBlock->Data <= BLOCK_SIZE);

            //Write next line header, in block or in next block

            if (((LPSTR)pCurLine - (LPSTR)pCurBlock->Data)
                == pCurBlock->LastLineOffset) {

                //Write header only if there is a line after

                pNextBlock = pCurBlock->NextBlock;
                if (pNextBlock != NULL) {

                    //Set next line pointer to first line of Next Block

                    pNextLine = (LPLINEREC)pNextBlock->Data;
                    pNextLine->PrevLength = (BYTE)newLineLen;

                }

            } else

                //Not at last line of block, Set next line previous line

                pNextLine->PrevLength = (BYTE)newLineLen;

            //If we insert a new line : Change next line length and
            //set current line status to 0 if it's a brand new line

            if (addOneLine) {
                pNextLine->Length = (BYTE)nextLineLength;
                if (col == 0) {

                    //Propagate status to next line

                    pNextLine->Status = pCurLine->Status;
                    pCurLine->Status = 0;
                } else {

                    //Set next line status to 0

                    AssertAligned(pNextLine);
                    pNextLine->Status = 0;
                }
            }

            //Set cur line length

            pCurLine->Length = (BYTE)newLineLen;

        } else {

            if (!FindInsertSpace(doc, &pCurBlock, &pCurLine, pBlockEnd,
                                                             sizeExpansion)) {
                goto errRet;
            }

            //Insert chars in next block

            goto reDoIt;

        }

        if (addOneLine) {
            charsEnd += 2;
            size -=2;
            d->lineBottom++;
            d->NbLines++;
            expandedLen = 0;
        }

        charsBegin = charsEnd;
        col = 0;
        if (size || addOneLine) {
            pCurLine = pNextLine;
        }

    }

    //Close undo/redo record

    CloseRec(doc, max(0, (int)((start - (LPSTR)pCurLine->Text) + charsSize)),
          d->lineBottom, TRUE);

    //Inform breakpoints and errors handlers of what we did in the text
    //(only if it's a real insertion)

    if (d->recType !=  REC_STOPPED && d->NbLines > nbLines) {
        AdjustDebugLines(doc, startLine, d->NbLines - nbLines, TRUE);
    }

    CloseLine(doc, &pCurLine, d->lineBottom + 1, &pCurBlock);

    //Document is modified

    ModifiedDoc(doc);

    //If there was nothing to record, tells the system that now we have
    //something in buffer

    if (d->playCount == REC_CANNOTUNDO) {
        d->playCount = 0;
    }

    if (d->language != NO_LANGUAGE) {
        CheckSyntax(doc);
    }

    editorIsCritical = FALSE;

    return TRUE;

  errRet: {

        //Cancel recording

        CloseRec(doc, 0, 0, FALSE);

        //Try to resynchronize system on first line of document

        pCurLine = (LPLINEREC)d->FirstBlock->Data;
        CloseLine(doc, &pCurLine, 1, &d->FirstBlock);

        editorIsCritical = FALSE;
        return FALSE;
    }

}                                       /* InsertBlock() */

BOOL NEAR PASCAL
CancelDelete(
    int doc
    )
{

    if (QuestionBox(ERR_UndoRedoBufferTooSmall, MB_YESNO) == IDYES) {

        //Cancel this record and stop recording for this action
        CloseRec(doc, 0, 0, FALSE);
        Docs[doc].recType = REC_HADOVERFLOW;
        return FALSE;
    } else {
        return TRUE;
    }
}

//Delete characters on a single line, pointers are unaffected
BOOL NEAR PASCAL
PartialLineDelete(
    int doc,
    LPBLOCKDEF *pCurBlock,
    LPLINEREC *pCurLine,
    int * totalSize,
    int col1,
    int col2,
    BOOL moveToNextLine
    )
{
    int moveSize;
    LPLINEREC pNextLine  = NULL, pLastLine;
    LPBLOCKDEF pNextBlock = *pCurBlock;

    //Adjust col1 and col2 inside the line
    col1 = min(col1, (*pCurLine)->Length - LHD);
    col2 = min(col2, (*pCurLine)->Length - LHD);
    moveSize = col2 - col1;

    //Send deleted chars to record buffer and roll back
    //if buffer full
    if (!AppendToRec(doc, (*pCurLine)->Text + col1, moveSize, FALSE, totalSize))
        if (CancelDelete(doc))
            return FALSE;

    pLastLine = (LPLINEREC)((*pCurBlock)->Data + (*pCurBlock)->LastLineOffset);

    //Move chars
    if (moveSize > 0) {
        memmove((LPSTR)(*pCurLine)->Text + col1,
                 (LPSTR)(*pCurLine)->Text + col2,
                 (size_t) ((LPSTR)pLastLine + pLastLine->Length
                         - ((LPSTR)(*pCurLine)->Text + col2)));
    }

    //Update current line length
    (*pCurLine)->Length -= (BYTE) moveSize;

    //Update last line offset and next line previous length (in current
    //or in next block)
    if ((*pCurLine) == pLastLine) {

        //Next line is in next block
        pNextBlock = (*pCurBlock)->NextBlock;
        if (pNextBlock != NULL) {

            //Set next line pointer to first line of Next Block and
            //update previous length
            pNextLine =     (LPLINEREC)pNextBlock->Data;
            pNextLine->PrevLength -= (BYTE) moveSize;

        }

    }
    else {

        (*pCurBlock)->LastLineOffset -= moveSize;
        pNextLine = (LPLINEREC)((LPSTR)*pCurLine + (*pCurLine)->Length);
        pNextLine->PrevLength -= (BYTE) moveSize;
    }

    if (moveToNextLine) {
        *pCurLine = pNextLine;
        *pCurBlock = pNextBlock;
    }

    return TRUE;

}

//Deletes the pCurLine in the pCurBlock and adjust them to the
//next line, if the line was the last line of the block, the block
//is deleted
BOOL NEAR PASCAL
FullLineDelete(
    int doc,
    LPBLOCKDEF *pCurBlock,
    LPLINEREC *pCurLine,
    int *totalSize
    )
{
    LPLINEREC pLastLine;
    BYTE prevLineLen = (*pCurLine)->PrevLength;

    //First send deleted chars to record buffer
    if (!AppendToRec(doc, (*pCurLine)->Text, (*pCurLine)->Length - LHD, TRUE,
                                                                 totalSize)) {
        if (CancelDelete(doc)) {
            return FALSE;
        }
    }

    //Compute lastLine and nextLine in block
    pLastLine = (LPLINEREC)((*pCurBlock)->Data + (*pCurBlock)->LastLineOffset);
    if (*pCurLine == pLastLine) {

        //If line is also the first line of block, delete the block
        if (*pCurLine == (LPLINEREC)(*pCurBlock)->Data) {

            Assert((*pCurBlock)->LastLineOffset == 0);

            //Document may be empty, reinitialize first block
            if ((*pCurBlock)->PrevBlock == NULL &&
                                             (*pCurBlock)->NextBlock == NULL) {
                (*pCurLine)->Length = LHD;
                (*pCurLine)->Status = 0;
                (*pCurBlock)->LastLineOffset = 0;
                Docs[doc].NbLines = 1;
                Assert((*pCurLine)->PrevLength == 0);
                Assert(Docs[doc].FirstBlock == *pCurBlock);
                Assert(Docs[doc].LastBlock == *pCurBlock);
                return TRUE;
            } else {

                //Delete block, and set current block to next block
                *pCurBlock = FreeBlock(doc, *pCurBlock);
            }

        } else {

            //Adjust the current block last offset
            (*pCurBlock)->LastLineOffset -= prevLineLen;
            Assert((*pCurBlock)->LastLineOffset >= 0);

            //Set curBlock to next block
            *pCurBlock = (*pCurBlock)->NextBlock;

        }

        //Set next line to first line of block
        if (*pCurBlock != NULL) {
            *pCurLine = (LPLINEREC)(*pCurBlock)->Data;
        }

    } else {

        LPLINEREC pNextLine = (LPLINEREC)((LPSTR)*pCurLine +
                                                         (*pCurLine)->Length);

        //Adjust block last offset
        (*pCurBlock)->LastLineOffset -= (*pCurLine)->Length;
        Assert((*pCurBlock)->LastLineOffset >= 0);

        //Now shift left rest of block chars
        memmove(*pCurLine,
                 (LPSTR)pNextLine,
                 (size_t) ((LPSTR)pLastLine + pLastLine->Length - (LPSTR)pNextLine));
    }

    //If next line exist, change it's header, otherwise resynchronise
    if (*pCurBlock != NULL) {
        (*pCurLine)->PrevLength = prevLineLen;
    } else {
        *pCurBlock = Docs[doc].LastBlock;
        *pCurLine = (LPLINEREC)((*pCurBlock)->Data +
                                                (*pCurBlock)->LastLineOffset);
    }

    Docs[doc].NbLines--;

    return TRUE;
}

//Join the current line to the preceding, pad blanks if necessary
BOOL NEAR PASCAL
JoinLines(
    int doc,
    LPBLOCKDEF pCurBlock,
    LPLINEREC pCurLine,
    int *totalSize,
    int lineNb,
    int col1
    )
{
    LPDOCREC d = &Docs[doc];
    WORD undoRedoState = d->recType;
    int limit;
    LPLINEREC pPrevLine;
    char tmpLine[MAX_USER_LINE + 2]; //2 = Possible CR+LF
    WORD status;
    BOOL language = d->language;

    //First compose the resulting new line
    if (pCurLine == (LPLINEREC)pCurBlock->Data) {
        pCurBlock = pCurBlock->PrevBlock;
        Assert(pCurBlock != NULL);
        pPrevLine = (LPLINEREC)((LPSTR)pCurBlock->Data +
                                                   pCurBlock->LastLineOffset);
    } else {
        pPrevLine = (LPLINEREC)((LPSTR)pCurLine - pCurLine->PrevLength);
    }

    //Save portion of first line and save status (unless all first line
    //is deleted)
    if (col1 >0) {
        status = pPrevLine->Status;
    } else {
        status = 0;
    }

    limit = min(col1, pPrevLine->Length - LHD);
    memmove((LPSTR)tmpLine, (LPSTR)pPrevLine->Text, limit);

    //We may have now to pad line with blanks
    for(; limit < MAX_USER_LINE && limit < col1; limit++) {
        tmpLine[limit] = ' ';
    }

    //Append now current line
    if (limit + pCurLine->Length - LHD <= MAX_USER_LINE) {
        memmove((LPSTR)tmpLine + limit, (LPSTR)pCurLine->Text,
                                         pCurLine->Length - LHD);
        limit += pCurLine->Length - LHD;
    }
    else {
        ErrorBox(ERR_Line_Too_Long);
        return FALSE;
    }

    //Check line overflow
    if (limit >= MAX_USER_LINE) {
        ErrorBox(ERR_Line_Too_Long);
        return FALSE;
    }

    //Append a  CR+LF
    memmove(tmpLine + limit, CrLf, 2);
    limit += 2;

    //Insert the new one
    CloseLine(doc, &pPrevLine, lineNb--, &pCurBlock);

    //Stop Recording
    d->recType = REC_STOPPED;

    d->language = NO_LANGUAGE; //To avoid syntax check
    if (!InsertBlock(doc, 0, lineNb++, limit, (LPSTR)tmpLine)) {
        return FALSE;
    }
    d->language = (WORD) language;

    //Resynchronize to next line
    pCurBlock = d->CurrentBlock;
    pCurLine = (LPLINEREC)((LPSTR)pCurBlock->Data + d->CurrentLineOffset);
    pCurLine->Status = 0;

    //Write saved status in previous line (the one just inserted)
    if (pCurLine == (LPLINEREC)pCurBlock->Data) {

        LPBLOCKDEF pPrevBlock = pCurBlock->PrevBlock;
        if (pPrevBlock != NULL) {
            ((LPLINEREC)((LPSTR)pPrevBlock->Data + pPrevBlock->LastLineOffset))                                                               ->Status = status;
        }
    } else {
        ((LPLINEREC)((LPSTR)pCurLine - pCurLine->PrevLength))->Status = status;
    }

    //Delete now the 2 lines
    if (!FullLineDelete(doc, &pCurBlock, &pCurLine, totalSize)) {
        return FALSE;
    }
    if (!FullLineDelete(doc, &pCurBlock, &pCurLine, totalSize)) {
        return FALSE;
    }
    d->recType = undoRedoState;
    CloseLine(doc, &pCurLine, min(d->NbLines, lineNb + 1), &pCurBlock);

    return TRUE;
}


//When your file has an Undo buffer, be sure to set the undo/redo
//engine to UNDO before deleting chars, otherwise they will be stored
//in the REDO buffer
BOOL FAR PASCAL
DeleteBlock(
    int doc,
    int col1,
    long line1,
    int col2,
    long line2
    )
{
    LPLINEREC   pCurLine;
    LPBLOCKDEF  pCurBlock;
    int         curLine = line1, startLine = line1;
    int         editCol1, editCol2;
    LPDOCREC    d = &Docs[doc];
    int         totalSize = 0;
    WORD        undoRedoState;
    int         totalNbLines = d->NbLines;

    //Beep and exit if file is ReadOnly
    if (d->readOnly || ROREGION(d,col1,line1)) {
        ReadOnlyBeep();
        return FALSE;
    } else {
        // Clear error status
        QueryReadOnlyError();
    }

    //Verifications and adjustements
    Assert(line1 < totalNbLines);
    Assert(line2 <= totalNbLines);
    Assert(line2 >= line1);
    editCol1 = min(col1, MAX_USER_LINE);
    editCol2 = min(col2, MAX_USER_LINE + 1);
    d->lineTop = d->lineBottom = line1;

    editorIsCritical = TRUE;

    //Get information about line we start delete
    if (!FirstLine (doc, &pCurLine, &line1, &pCurBlock)) {
        //Try to resynchronize system on first line of document
        pCurLine = (LPLINEREC)(d->FirstBlock->Data);
        CloseLine(doc, &pCurLine, 1, &d->FirstBlock);
        ClearSelection(curView);
        editorIsCritical = FALSE;
        return FALSE;
    }
    line1--;

    //Remember record action
    undoRedoState = d->recType;

    //Align column respecting tabs
    if (playingRecords) {
        col1 = editCol1;
    } else {
        col1 = AlignToTabs(editCol1, pCurLine->Length - LHD, (LPSTR)pCurLine->Text);
    }

    //Open an edit record, if buffer full, ask the user for action
    if (!OpenRec(doc, INSERTSTREAM, col1, curLine)) {
        if (CancelDelete(doc)) {
            goto rollBack;
        }
    }

    //Full line(s) delete
    if (col1 == 0  && col2 == 0) {

        for (curLine = line1; curLine < line2; curLine++) {
            if (!FullLineDelete(doc, &pCurBlock, &pCurLine, &totalSize)) {
                goto rollBack;
            }
        }
        CloseLine(doc, &pCurLine, line1+1, &pCurBlock);

    } else if (line1 == line2) {

        if (playingRecords) {
            col2 = editCol2;
        } else {
            col2 = AlignToTabs(editCol2, pCurLine->Length - LHD, (LPSTR)pCurLine->Text);
        }

        //The 2 columns could be in the same tab
        if (col1 == col2) {
            col2++;
        }

        if (!PartialLineDelete(doc, &pCurBlock, &pCurLine, &totalSize, col1, col2, FALSE)) {
            goto rollBack;
        }
        CloseLine(doc, &pCurLine, line1+1, &pCurBlock);

    } else {

        //Tricky case, deletion spreads over several lines
        Assert(curLine < line2);

        //Delete the right part of first line
        if (!PartialLineDelete(doc,
                               &pCurBlock,
                               &pCurLine,
                               &totalSize,
                               col1,
                               pCurLine->Length - LHD,
                               TRUE)) {
            goto rollBack;
        }

        //Insert a fake CrLf in Undo buffer,
        if (!AppendToRec(doc, (LPSTR)NULL, 0, TRUE, &totalSize)) {
            if (CancelDelete(doc)) {
                goto rollBack;
            }
        }

        //Delete lines in between
        for (curLine = line1+1; curLine < line2; curLine++) {
            if (!FullLineDelete(doc, &pCurBlock, &pCurLine, &totalSize)) {
                goto rollBack;
            }
        }

        if (playingRecords) {
            col2 = editCol2;
        } else {
            col2 = AlignToTabs(editCol2, pCurLine->Length - LHD, (LPSTR)pCurLine->Text);
        }

        //Delete the left part of last line
        if (!PartialLineDelete(doc,
                               &pCurBlock,
                               &pCurLine,
                               &totalSize,
                               0,
                               col2,
                               FALSE)) {
            goto rollBack;
        }

        //Rebuild resulting line
        if (!JoinLines(doc, pCurBlock, pCurLine, &totalSize, line1+1, col1)) {
            goto rollBack;
        }

        //Inform breakpoints and errors handler
        if (col1 > 0) {
            startLine++;
        }
    }

    //Document is modified
    ModifiedDoc(doc);

    //Inform breakpoints handler of what we did in the text
    if (d->NbLines < totalNbLines) {
        AdjustDebugLines(doc, startLine, totalNbLines - d->NbLines, FALSE);
    }

    //If the recording was canceled, this close will do nothing
    CloseRec(doc, 0, 0, TRUE);

    //If there was nothing to record, tells the system that now we have
    //something in buffer, if we had an overflow, restart Undo/Redo
    if (d->recType != REC_HADOVERFLOW) {
        d->recType = undoRedoState;
    } else {
        d->playCount = REC_CANNOTUNDO;
    }

    if (d->language != NO_LANGUAGE) {
        CheckSyntax(doc);
    }

    editorIsCritical = FALSE;
    return TRUE;


    //Record buffer full
    rollBack: {

        CloseRec(doc, 0, 0, TRUE);

        //Document is modified
        ModifiedDoc(doc);

        //Try to resynchronize system on first line of document
        pCurBlock = d->FirstBlock;
        pCurLine = (LPLINEREC)pCurBlock->Data;

        CloseLine(doc, &pCurLine, 1, &pCurBlock);
        ClearSelection(curView);
        editorIsCritical = FALSE;

        //Play the recorded information
        PlayRec(doc, d->recType, FALSE, FALSE);
        return FALSE;
    }
}

BOOL FAR PASCAL
DeleteAll(
    int doc
    )
{
    return DeleteBlock(doc, 0, 0, MAX_USER_LINE + 1, Docs[doc].NbLines - 1);
}

BOOL FAR PASCAL
ReplaceCharInBlock(
    int doc,
    int x,
    long y,
    int ch
    )
{
    LPLINEREC pCurLine;
    LPBLOCKDEF pCurBlock;
    LPDOCREC d = &Docs[doc];
    int realX;

    //Beep and exit if file is ReadOnly
    if (d->readOnly || ROREGION(d,x,y)) {
        ReadOnlyBeep();
        return TRUE;
    } else {
        // Clear error status
        QueryReadOnlyError();
    }

    //Get information from line we replace char
    if (!FirstLine (doc, &pCurLine, &y, &pCurBlock))
        return FALSE;
    CloseLine(doc, &pCurLine, y, &pCurBlock);

    if (playingRecords) {
        realX = x;
    } else {
        realX = AlignToTabs(x, pCurLine->Length - LHD, (LPSTR)pCurLine->Text);
    }

    if (realX >= pCurLine->Length - LHD) {

        //Character position is after line last char, insert char
        return InsertBlock(doc, x, y - 1, 1, (LPSTR)&ch);

    } else {

        d->lineTop = d->lineBottom = y - 1;

        //It's a real replace, open an edit record, append char and
        //close record
        if (!OpenRec(doc, REPLACECHAR, realX, y - 1)) {
            return FALSE;
        }
        if (!AppendToRec(doc, (LPSTR)pCurLine->Text + realX, 1, FALSE, NULL)) {
            return FALSE;
        }
        CloseRec(doc, 0, 0, TRUE);

        //Character is inside line, just replace it
        pCurLine->Text[realX] = (char)ch;

        //Document is modified
        ModifiedDoc(doc);

        if (d->language != NO_LANGUAGE) {
            CheckSyntax(doc);
        }
        return TRUE;
    }
}

ULONG FAR PASCAL CountCharsInDocument(
        int doc)
{
        LPLINEREC pl;
        LPBLOCKDEF pb;
        long y;
        ULONG nbChars;

        y = 0;
        //Load first line of document
        if (!FirstLine (doc, &pl, &y, &pb)) {
                Assert(FALSE);
                return 0L;
        }

        //Count chars in first line
        nbChars = pl->Length - LHD + 2; //2 = CR+LF

        //Count chars in middle lines
        while (y < Docs[doc].NbLines) {

                //Load next line of document
                if (!NextLine (doc, &pl, &y, &pb)) {
                        Assert(FALSE);
                        return 0;
                }

                //Count chars in current line
                nbChars += pl->Length - LHD + 2; //2 = CR+LF

        }

        //Refresh pointers
        CloseLine (doc, &pl, y, &pb);

        return nbChars;

}

//Scan all document and try to compress data by moving lines and
//filling blocks
BOOL FAR PASCAL
CompactDocument(
    int doc
    )
{
    LPLINEREC pCurLine, pNextLine;
    LPBLOCKDEF pCurBlock, pNextBlock;
    int freeBlockSize;
    BOOL more;
    LPDOCREC d = &Docs[doc];
    long spaceRecovered = 0;

    //Load last line of first block
    pCurBlock = d->FirstBlock;
    pNextBlock = pCurBlock->NextBlock;

    while (pNextBlock != NULL) {

        pNextLine = (LPLINEREC)(pNextBlock->Data);

        more = TRUE;
        while (more) {

            pCurLine = (LPLINEREC)(pCurBlock->Data + pCurBlock->LastLineOffset);
            freeBlockSize = BLOCK_SIZE - pCurBlock->LastLineOffset -
                                                             pCurLine->Length;

            //Have we space to move next line in current block ?
            if (freeBlockSize < (int)pNextLine->Length) {

                more = FALSE;

            } else {

                //Move first line from next block in current block
                pCurBlock->LastLineOffset += pCurLine->Length;
                memmove((LPSTR)pCurLine + pCurLine->Length,
                          pNextLine,
                          pNextLine->Length);

                //Remove line from next block
                pNextBlock->LastLineOffset -= pNextLine->Length;
                spaceRecovered += pNextLine->Length;
                memmove((LPSTR)pNextBlock->Data,
                         (LPSTR)pNextLine + pNextLine->Length,
                         BLOCK_SIZE - (size_t) (((LPSTR)pNextLine + pNextLine->Length -
                                                     (LPSTR)pNextBlock->Data)));

                //Free next block if empty
                if (pNextBlock->LastLineOffset < 0) {
                    pNextBlock = FreeBlock(doc, pNextBlock);
                    if (pNextBlock == NULL) {
                        more = FALSE;
                    } else {
                        pNextLine = (LPLINEREC)(pNextBlock->Data);
                    }
                }
            }
        }

        //Next block becomes current block
        pCurBlock = pCurBlock->NextBlock;
        if (pCurBlock != NULL) {
            pNextBlock = pCurBlock->NextBlock;
        }
    }

    //Resynchronize to first line of document
    d->CurrentLine = 0;
    d->CurrentBlock = d->FirstBlock;
    d->CurrentLineOffset = 0;
    return TRUE;
}

BOOL FAR PASCAL
CheckDocument(
    int doc
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb, previousBlock;
    BOOL curBlockFound = FALSE, lastBlockFound = FALSE;
    BOOL curLineFound = FALSE;
    BYTE curLen;
    int nbLines = 0;
    LPDOCREC d = &Docs[doc];
    int nbBl = 0;
    DWORD dataSize = 0;
    DWORD totalSize;

    pb = d->FirstBlock;
    Assert(pb != NULL);
    previousBlock = NULL;
    curLen = 0;

    while (pb != NULL) {

        if (pb == d->CurrentBlock) {
            curBlockFound = TRUE;
        }
        if (pb == d->LastBlock) {
            lastBlockFound = TRUE;
        }
        if (pb->PrevBlock != previousBlock) {
            return AuxPrintf(1, "ChkDoc:Prev block pointer (Line %i)", nbLines);
        }

        if (pb->LastLineOffset > BLOCK_SIZE - LHD) {
            return AuxPrintf(1, "ChkDoc:Line last offset above (Line %i)",
                                                                     nbLines);
        }
        if (pb->LastLineOffset < 0) {
            return AuxPrintf(1, "ChkDoc:Line last offset below (Line %i)",
                                                                     nbLines);
        }

        pl = (LPLINEREC)(pb)->Data;

        while (pl <= (LPLINEREC)(pb->Data + pb->LastLineOffset)) {

            if (d->CurrentBlock == pb) {
                if (d->CurrentLineOffset == ((LPSTR)pl - (LPSTR)pb->Data)) {
                    curLineFound = TRUE;
                    if (nbLines != d->CurrentLine) {
                        return AuxPrintf(1, "ChkDoc:Bad current line (Line %i)",
                                                                     nbLines);
                    }
                }
            }

            if (pl->PrevLength != curLen) {
                return AuxPrintf(1, "ChkDoc:Bad prev length (Line %i)",
                                                                     nbLines);
            }
            curLen = pl->Length;
            dataSize += curLen;
            if (curLen < LHD) {
                return AuxPrintf(1, "ChkDoc:Bad length (Line %i)", nbLines);
            }
            if ((LPSTR)pl + curLen > (LPSTR)pb->Data + BLOCK_SIZE) {
                return AuxPrintf(1, "ChkDoc:Line above block (Line %i)",
                                                                     nbLines);
            }
            pl = (LPLINEREC) ((LPSTR)pl + curLen);
            nbLines++;
        }

        previousBlock = pb;
        pb = pb->NextBlock;
        if (pb == NULL) {
            if (d->LastBlock != previousBlock) {
                return AuxPrintf(1, "ChkDoc: Bad last block (Line %i)",
                                                                     nbLines);
            }
        }

        nbBl++;
    }

    totalSize = (DWORD)nbBl * sizeof(BLOCKDEF);
    Assert(totalSize);
    AuxPrintf(1,
        "Blocks = %i, Total Size = %lu, Data Size = %lu, Efficiency = %lu\\%",
         nbBl, totalSize, dataSize, (100 * dataSize) / totalSize);
    if (!curBlockFound) {
        return AuxPrintf(1, "ChkDoc:Lost cur block (Line %i)", nbLines);
    }
    if (!lastBlockFound) {
        return AuxPrintf(1, "ChkDoc:Lost last block (Line %i)", nbLines);
    }
    if (!curLineFound){
        return AuxPrintf(1, "ChkDoc:Lost cur line (Line %i)", nbLines);
    }
    if (nbLines != d->NbLines) {
        return AuxPrintf(1,
                     "ChkDoc:Wrong total of lines (Doc = %i, Count = %i)",
                     d->NbLines, nbLines);
    }

    return TRUE;
}

void FAR PASCAL
DumpDocument(
    int doc
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;
    int line = 0;
    char s[10], t[10];
    LPDOCREC d = &Docs[doc];
    int i;

    pb = d->FirstBlock;
    while (pb != NULL) {
        _itoa(HIWORD((DWORD_PTR)pb), t, 16);
        while ((int)strlen(t) <= 4) {
            strcat(t, " ");
        }
        strcat(t, ":");
        pl = (LPLINEREC)(pb)->Data;
        while (pl <= (LPLINEREC)(pb->Data + pb->LastLineOffset)) {

            s[0]='\0';
            if (d->CurrentBlock == pb) {
                if (d->CurrentLineOffset == ((LPSTR)pl - (LPSTR)pb->Data)) {
                    _itoa((int)d->CurrentLine, s, 10);
                }
            }

            while ((int)strlen(s) <= 2) {
                strcat(s, " ");
            }

            strncpy((LPSTR)szTmp, pl->Text, pl->Length - LHD);
            for (i = 0; i < (int)strlen(szTmp); i++) {
                if(IsDBCSLeadByte((BYTE)szTmp[i])) {
                    i++;
                }
                if (szTmp[i] == TAB) {
                    szTmp[i] = 126;
                }
            }
            szTmp[min(200, pl->Length - LHD)] = '\0';
            AuxPrintf(1, "%s%3u [%3i,%3i,%3i] %s '%s'",
                     (LPSTR)t, line++, pl->Status, pl->PrevLength,
                     pl->Length, (LPSTR)s, (LPSTR)szTmp);
            strcpy(t,"     :");
            pl = (LPLINEREC) ((LPSTR)pl + pl->Length);
        }
        pb = pb->NextBlock;
    }

    AuxPrintf(1, "");
}

BOOL FAR PASCAL
DestroyDocument(
    int doc
    )
{
    LPBLOCKDEF pb;
    LPDOCREC d = &Docs[doc];

    //Destroy undo and possibly redo buffers
    DestroyRecBuf(doc, REC_UNDO);
    if (d->playCount != 0 && d->playCount != REC_CANNOTUNDO) {
        DestroyRecBuf(doc, REC_REDO);
    }

    //And blocks of memory allocated for it
    while (d->FirstBlock != NULL) {

        pb = d->FirstBlock;

        Assert(pb->NextBlock != NULL || pb == d->LastBlock);

        d->FirstBlock = pb->NextBlock;
        if (!DocFree((LPSTR)pb)) {
            InternalErrorBox(SYS_Free_Memory);
        }
    }

    return TRUE;
}

//Retrieve chars on a line
BOOL FAR PASCAL
GetTextAtLine(
    int doc,
    long line,
    int col1,
    int col2,
    LPSTR text
    )
{
    LPLINEREC pl;
    LPBLOCKDEF pb;

    if (!FirstLine (doc, &pl, &line, &pb))
        return FALSE;
    CloseLine(doc, &pl, line, &pb);

    col2 = min(col2, elLen);
    col1 = min(col1, col2);
    memmove(text, el + col1, col2 - col1);
    text[col2 - col1] = '\0';
    return TRUE;
}

