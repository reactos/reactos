/*--Author:

    Griffith Wm. Kadnier (v-griffk) 01-Aug-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


//Prototypes

void PASCAL LowLevelReadBuf (LPSTR source, long offset, LPSTR dest, WORD size, long limit);
void PASCAL LowLevelWriteBuf (LPSTR dest, long offset, LPSTR source, WORD size, long limit);
void PASCAL ReadFromBuf (LPUNDOREDOREC p, LPSTR dest, WORD size);
char PASCAL ReadCharFromStreamBuf (LPUNDOREDOREC p, WORD disp);
BOOL PASCAL WriteToBuf (int doc, LPUNDOREDOREC p, WORD disp, LPSTR source, WORD size, BOOL expanding);
BOOL PASCAL InitRecBuf (LPUNDOREDOREC p, long size, BOOL allocateMem);


/***    LowLevelReadBuf
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

void PASCAL LowLevelReadBuf(LPSTR source, long offset, LPSTR dest, WORD size, long limit)
{
    Assert(offset + size <= limit);
    memmove(dest, source + offset, size);
    return;
}                                       /* LowLevelReadBuf() */

/***    LowLevelWriteBuf
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

void PASCAL LowLevelWriteBuf(LPSTR dest, long offset, LPSTR source, WORD size, long limit)
{
    Assert(offset + size <= limit);
    memmove(dest + offset, source, size);
    return;
}                                       /* LowLevelWriteBuf() */

/***    ReadFileFromBuf
*/

void PASCAL ReadFromBuf(LPUNDOREDOREC p, LPSTR dest, WORD size)
{
    LPSTR source =(LPSTR)p->pRec + p->offset;

    //If read goes beyond buffer end, split in two moves
    Assert(p->offset + size <= p->bufferSize);
    memmove(dest, source, size);

    return;
}                                       /* ReadFromBuf() */


/***    ReadLineFromBuf
*/

void FAR PASCAL ReadLineFromBuf(LPUNDOREDOREC p, LPSTR dest, int *size,
                                 int *expandedLen, LPSTR *charsEnd)
{
    STREAMREC st;
    long pos;

    //Read this record header using a safe way (header could spread
    //over the end and the start of the buf)
    LowLevelReadBuf((LPSTR)p->pRec, p->offset,
            (LPSTR)&st, HDR_INSERTSTREAM_SIZE, p->bufferSize);
    Assert((st.action & ACTIONMASK) == INSERTSTREAM);

    pos = p->offset + HDR_INSERTSTREAM_SIZE + st.x.s.len - (*size);

    //Read sequentially buffer until size == 0 or char is a CR, rewind
          //to beginning of buffer if we reach end of buffer
          *charsEnd = dest;
    while (*size && *((LPSTR)p->pRec + pos) != CR) {
        if (*((LPSTR)p->pRec + pos) == TAB)
            *expandedLen += (g_contGlobalPreferences_WkSp.m_nTabSize - (*expandedLen % g_contGlobalPreferences_WkSp.m_nTabSize));
        else
            (*expandedLen)++;
        *(*charsEnd)++ = *((LPSTR)p->pRec + pos);
        pos++;
        (*size)--;
    }

    //Append a CR + LF in dest if there is a line
    if (*size && *((LPSTR)p->pRec + pos) == CR) {
        (*charsEnd)[0] = CR;
        (*charsEnd)[1] = LF;
    }

    return;
}                                       /* ReadLineFromBuf() */

/***    ReadCharFromStreamBuf
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


char PASCAL ReadCharFromStreamBuf(LPUNDOREDOREC p, WORD disp)
{
    return *((LPSTR)p->pRec + p->offset + HDR_INSERTSTREAM_SIZE + disp);
}                                       /* ReadCharFromStreamBuf() */

/***    WriteToBuf
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

BOOL PASCAL WriteToBuf(int doc, LPUNDOREDOREC p, WORD disp, LPSTR source,
        WORD size, BOOL expanding)
{
    long oldPos = p->offset + disp;
    long newPos = p->offset + disp + size;
    STREAMREC st;
    BOOL more;

    //This should allways be true
          Assert((long)size <= p->bufferSize);

    if (expanding &&    newPos > p->bufferSize) {

        WORD k = 0, len = 0;

        recordBufferOverflow = 0;

        do {

            //Read the record header
                  LowLevelReadBuf((LPSTR)p->pRec, k,
                  (LPSTR)&st, sizeof(st), p->bufferSize);

            //Compute len of the first record
            switch(st.action & ACTIONMASK) {
              case DELETESTREAM:
                len += HDR_DELETESTREAM_SIZE;
                break;
              case DELETECHAR:
                len += HDR_DELETECHAR_SIZE;
                break;
              case INSERTSTREAM:
                len += (WORD) (HDR_INSERTSTREAM_SIZE + st.x.s.len);
                break;
              case INSERTCHAR:
              case REPLACECHAR:
                len += HDR_INSERTCHAR_SIZE;
                break;
              default:
                Assert(FALSE);
                return FALSE;
                break;
            }

            k = len;
            more = (newPos - len > p->bufferSize);

            if (Docs[doc].recType == REC_REDO && !(st.action & CANCELREC)) {

                //Increment the global record buffer overflow count
                      recordBufferOverflow++;

                return InternalErrorBox(SYS_RedoBufferOverflow);
            }

        } while (more);

        //Make the move
              memmove((LPSTR)p->pRec,
              (LPSTR)p->pRec + len,
              (WORD)p->offset + disp - len);
        oldPos -= len;
        p->offset -= len;

        //The current record is now the first one, set it's previous length
              //to 0 and write it back (we just write the first word, to avoid
              //erasing other fields). CAUTION : If prevLen is not first field
              //of STREAMREC. The guy changing it will have surprises....
              st.prevLen = 0;
        LowLevelWriteBuf((LPSTR)p->pRec, 0,
              (LPSTR)&st, sizeof(st.prevLen), p->bufferSize);

    }

    //Now we can safely write data in buffer
    LowLevelWriteBuf((LPSTR)p->pRec, oldPos,  source, size, p->bufferSize);

    return TRUE;
}                                       /* WriteToBuf() */

/***    InitRecBuf
**
**  Synopsis:
**      bool = InitRecBuf(p, size, allocateMem)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL PASCAL
InitRecBuf(
    LPUNDOREDOREC p,
    long size,
    BOOL allocateMem
    )
{
    WORD nbRecs;
    STREAMREC st;

    nbRecs = (WORD) max(2, (WORD)(size / HDR_DELETESTREAM_SIZE));
    p->bufferSize = nbRecs * HDR_DELETESTREAM_SIZE;

    if (allocateMem) {

        //Allocate record buffer, it is important to be zero inited !
        if (!(p->h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                             p->bufferSize))) {
            return ErrorBox(SYS_Allocate_Memory);
        }
        AuxPrintf(1, "%lu bytes allocated", p->bufferSize);

        //Lock buffer
        if (FailGlobalLock(p->h, (LPSTR *)&(p->pRec))) {
            return InternalErrorBox(SYS_Lock_Memory);
        }
    }

    //Put a DELETESTREAM + CANCEL in buffer so OpenRec won't try to
     //alloc a new rec, and to initiate the circular buffer process
    st.action = DELETESTREAM + CANCELREC;
    st.prevLen = 0;
    st.col = 0;
    st.line = 0;
    st.x.c.col = 1;
    st.x.c.line = 0;
    memmove((LPSTR)p->pRec, (LPSTR)&st, HDR_DELETESTREAM_SIZE);

    //First record
    p->offset = 0;

    return TRUE;
}


/***    CreateRecBuf
**
**  Synopsis:
**      bool = CreateRecBuf(doc, recType, bytes)
**
**  Entry:
**
**  Returns:
**
**  Description:
**
*/

BOOL FAR PASCAL CreateRecBuf(int doc, BYTE recType, long bytes)
{
    LPDOCREC docs = &Docs[doc];
    UNDOREDOREC p;
    int tmp_int;

    if (docs->recType == REC_STOPPED)
          return TRUE;

    switch(recType) {
      case REC_UNDO :
        p = docs->undo;
        docs->playCount = REC_CANNOTUNDO; //Normal editing mode for now
        break;

      case REC_REDO :
        {
            SYSTEMTIME SystemTime;

            p = docs->redo;

            //Redo Buffer needs to be bigger than undo buf, to potentially
            //contain the characters in the existing text, so we allocate
            //the size of undo buffer + size of text.
            tmp_int = docs->undo.bufferSize + CountCharsInDocument(doc);
            bytes = min(tmp_int, UNDOREDO_MAX_SIZE);

            FileTimeToSystemTime(&docs->time, &SystemTime);
            mC = SystemTime.wMonth;
            dL = SystemTime.wDay - 1;
        }
        break;

      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    //Allocates and initialize buffer
          if (!InitRecBuf(&p, bytes, TRUE))
          return FALSE;

    DbgX(GlobalUnlock(p.h) == 0);

    //Set document record pointer to new record
    switch(recType) {
      case REC_UNDO :
        docs->undo = p;
        break;
      case REC_REDO :
        docs->redo = p;
        break;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    return TRUE;
}                                       /* CreateRecBuf() */

/***    DestroyRecBuf
**
*/

BOOL FAR PASCAL DestroyRecBuf(int doc, WORD recType)
{
    LPDOCREC docs = &Docs[doc];
    HANDLE h;

    if (docs->recType == REC_STOPPED)
          return TRUE;

    switch(recType) {
      case REC_UNDO :
        h = docs->undo.h;
        break;
      case REC_REDO :
        h = docs->redo.h;
        break;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    if ((h != 0) && (GlobalFree (h) != NULL))
          return InternalErrorBox(SYS_Free_Memory);

    switch(recType) {
      case REC_UNDO :
        docs->undo.h = 0;
        break;
      case REC_REDO :
        docs->redo.h = 0;
        docs->playCount = 0;
        docs->recType = REC_UNDO;
        break;
      case REC_STOPPED :
        return TRUE;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    return TRUE;
}                                       /* DestroyRecBuf() */


/***    ResizeRecBuf
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

BOOL FAR PASCAL ResizeRecBuf(int doc, long bytes)
{

    LPDOCREC docs = &Docs[doc];

    if (docs->recType == REC_STOPPED)
        return TRUE;

    //Destroy Redo Buffer if any
    if (docs->redo.h){
        DestroyRecBuf(doc, REC_REDO);
    }

    if (docs->undo.h) {

        //Destroy Undo Buffer
        DestroyRecBuf(doc, REC_UNDO);
        return CreateRecBuf(doc, REC_UNDO, bytes);
    }
    else {
        //Create Undo Buffer
        return CreateRecBuf(doc, REC_UNDO, bytes);
    }
}                                       /* ResizeRecBuf() */


/***    OpenRec
**
*/

BOOL FAR PASCAL OpenRec(int doc, BYTE action, int col, int line)
{
    LPDOCREC docs = &Docs[doc];
    UNDOREDOREC p;
    WORD hdrSize;
    STREAMREC st;

    switch(docs->recType) {
      case REC_UNDO :
        p = docs->undo;
        break;
      case REC_REDO :
        p = docs->redo;
        break;
      case REC_STOPPED :
      case REC_HADOVERFLOW :
        return TRUE;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    //Lock buffer, will stay used and locked until CloseRec
    if (FailGlobalLock(p.h, (LPSTR *)&(p.pRec)))
        return InternalErrorBox(SYS_Lock_Memory);

    //Get last rec to compute new rec address
          ReadFromBuf(&p, (LPSTR)&st, (WORD)sizeof(st));

    //First rec is or not a existing rec
    switch(st.action & ACTIONMASK) {
      case DELETESTREAM:
        hdrSize = HDR_DELETESTREAM_SIZE;
        break;
      case DELETECHAR:
        hdrSize = HDR_DELETECHAR_SIZE;
        break;
      case INSERTSTREAM:
        hdrSize = (WORD) (HDR_INSERTSTREAM_SIZE + st.x.s.len);
        break;
      case INSERTCHAR:
      case REPLACECHAR:
        hdrSize = HDR_INSERTCHAR_SIZE;
        break;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    //If type is CANCEL then don't change offset in buf
    if (!(st.action & CANCELREC)) {
        p.offset += hdrSize;
        st.prevLen = hdrSize;
    }

    st.action = (BYTE)action;
    if (stopMarkStatus == HAS_STOP)
          st.action += STOPMARK;
    else if (stopMarkStatus == NEXT_HAS_NO_STOP) {
        st.action += STOPMARK;
        stopMarkStatus = HAS_NO_STOP;
    }
    st.col = (BYTE)col;
    st.line = line;


    //Adjust buffer to chars start if we insert a block
          if ((action & ACTIONMASK) == INSERTSTREAM
          || (action & ACTIONMASK) == REPLACECHAR)
          st.x.s.len = 0;

    //Update last rec in buffer
          if (!WriteToBuf(doc, &p, 0, (LPSTR)&st, (WORD)sizeof(st), TRUE))
          return FALSE;

    //Set document record pointer to new record
    switch(docs->recType) {
      case REC_UNDO :
        docs->undo = p;
        break;
      case REC_REDO :
        docs->redo = p;
        break;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }
    return TRUE;
}                                       /* OpenRec() */

/***    CloseRec
**
*/

void FAR PASCAL CloseRec(int doc, int col, int line, BOOL keepRec)
{
    LPDOCREC docs = &Docs[doc];
    UNDOREDOREC p;
    STREAMREC st;

    switch(docs->recType) {
      case REC_UNDO :
        p = docs->undo;
        break;
      case REC_REDO :
        p = docs->redo;
        break;
      case REC_STOPPED :
      case REC_HADOVERFLOW :
        return;
      default:
        Assert(FALSE);
        return;
        break;
    }

    //Get last rec from buffer
          ReadFromBuf(&p, (LPSTR)&st, (WORD)sizeof(st));

    if (keepRec) {

        if ((st.action & ACTIONMASK) == DELETESTREAM) {

            //Cancel if we delete 0 chars and check if we delete 1 char
            if (col == st.col +1 && line == st.line)
                st.action += (DELETECHAR - DELETESTREAM);
            else if (col == (int)st.col && line == st.line)
                  st.action += CANCELREC;
            else {
                //Deleting a stream, save ending coordinates
                st.x.c.col = (BYTE)col;
                st.x.c.line = line;
            }
        }
        else {

            //Replacing char
            if ((st.action & ACTIONMASK) == REPLACECHAR)
                  st.x.ch = st.x.s.chars[0];
            else {

                //Inserting : If length of chars == 0 then cancel record, if
                //it's one char, put it in the special stucture
                if (st.x.s.len == 1) {
                    st.action += (INSERTCHAR - INSERTSTREAM);
                    st.x.ch = st.x.s.chars[0];
                }
                else if (st.x.s.len == 0)
                    st.action += CANCELREC;
            }
        }
    }
    else
        //Tells OpenRec to cancel this record
        st.action += CANCELREC;

    //Update last rec in buffer
          WriteToBuf(doc, &p, 0, (LPSTR)&st, (WORD)sizeof(st), FALSE);

    //%if (docs->recType == REC_UNDO)
          //%   AuxPrintf(1, "CloseRec REC_UNDO First = %lu", p.first);
    //%else
          //%   AuxPrintf(1, "CloseRec REC_REDO First = %lu", p.first);

    DbgX(GlobalUnlock(p.h) == 0);

    return;
}                                       /* CloseRec() */

/***    AppendToRec
**
*/

BOOL FAR PASCAL AppendToRec(int doc, LPSTR chars, int size, BOOL isLine, int *totalSize)
{
    LPDOCREC docs = &Docs[doc];
    UNDOREDOREC p;
    STREAMREC st;

    if (docs->recType == REC_STOPPED
          || docs->recType == REC_HADOVERFLOW)
          return TRUE;

    switch(docs->recType) {
      case REC_UNDO :
        p = docs->undo;
        break;
      case REC_REDO :
        p = docs->redo;
        break;
      case REC_STOPPED :
        return TRUE;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    //Get last rec from buffer
          ReadFromBuf(&p, (LPSTR)&st, (WORD)sizeof(st));

    Assert((st.action & ACTIONMASK) == INSERTSTREAM
          || (st.action & ACTIONMASK) == REPLACECHAR);

    //Updates total size
    if (totalSize) {
        *totalSize += size;
        if (isLine)
              *totalSize += 2;

        //Before to write data, check for a possible buffer overflow
              if ((long)(*totalSize + HDR_INSERTSTREAM_SIZE) > p.bufferSize)
              return FALSE;
    }

    if (size) {
        if (!WriteToBuf(doc, &p, (WORD)(HDR_INSERTSTREAM_SIZE + st.x.s.len), chars,
        (WORD) size, TRUE))
        return FALSE;
        st.x.s.len += (WORD) size;

        //Update last rec in buffer (but only the x.s.len field)
              WriteToBuf(doc, &p, HDR_INSERTSTREAM_SIZE - sizeof(STREAM),
              (LPSTR)&st.x.s, sizeof(WORD), FALSE);

    }

    if (isLine) {
        if (!WriteToBuf(doc, &p, (WORD)(HDR_INSERTSTREAM_SIZE + st.x.s.len),
        (LPSTR)CrLf, 2, TRUE))
        return FALSE;
        st.x.s.len += 2;

        //Update last rec in buffer (but only the x.s.len field)
              WriteToBuf(doc, &p, HDR_INSERTSTREAM_SIZE - sizeof(STREAM),
              (LPSTR)&st.x.s, sizeof(WORD), FALSE);
    }

    //Set document record pointer to new record
    switch(docs->recType) {
      case REC_UNDO :
        docs->undo = p;
        break;
      case REC_REDO :
        docs->redo = p;
        break;
      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    return TRUE;
}


/***    PlayRec
**
**
*/

BOOL FAR PASCAL PlayRec(int doc, WORD recType, BOOL untilUserMark, BOOL prompt)
{

    LPDOCREC docs = &Docs[doc];
    int j = 0;
    BOOL endOfRec = FALSE;
    UNDOREDOREC p;
    STREAMREC st;
    WORD action;
    int line;
    int col;
    BOOL more;
    BOOL pass1 = TRUE;

    if (docs->recType == REC_STOPPED) {
        return FALSE;
    }

    Assert(curView >= 0 && curView < MAX_VIEWS);

    switch(recType) {

      case REC_UNDO :
        //%AuxPrintf(1, "PLAY AN UNDO");
        docs->recType = REC_REDO;

        p = docs->undo;
        //If we start replay, we lock undo buffer and we create redo buffer
        if (docs->playCount == 0) {
            CreateRecBuf(doc, REC_REDO, p.bufferSize);
        }

        ClearSelection(curView);

        break;

      case REC_REDO :
        //%AuxPrintf(1, "PLAY A  REDO");

        //Were we at end of undos ?
              if (docs->playCount < 0)
              docs->playCount = -docs->playCount;

        p = docs->redo;
        docs->recType = REC_UNDO;
        ClearSelection(curView);
        break;

      default:
        Assert(FALSE);
        return FALSE;
        break;
    }

    if (FailGlobalLock(p.h, (LPSTR *)&(p.pRec))) {
        InternalErrorBox(SYS_Lock_Memory);
        return FALSE;
    }

    //Tells the editor that we are playing

    playingRecords = TRUE;

    //We will stop playing when find a record with required
    //stop condition, but at least, we play one
    more = TRUE;
    while (more) {
        STREAMREC *pstSave = (STREAMREC *)((LPSTR)p.pRec + p.offset);

        //Get rec from buffer
        ReadFromBuf(&p, (LPSTR)&st, (WORD)sizeof(st));

        action = st.action;

        //Test if we will stop after this replay
        if (untilUserMark) {
            more = !(action & USERMARK);
        } else {
            more = !(action & STOPMARK);
        }
        endOfRec = (st.prevLen == 0);
        if (endOfRec) {
            more = FALSE;
        }

        // If we are in a multiple replay, we insert the STOP in opposite
        //record buffer only if we replay the first one
        if (!pass1) {
            stopMarkStatus = HAS_NO_STOP;
        }

        if (!(action & CANCELREC)) {
            col = st.col;
            line = st.line;

            switch(action & ACTIONMASK) {

              case DELETESTREAM :
                //%AuxPrintf(1, "DELETESTREAM from (%i,%i) to (%i,%i)",
                //%                      col, line, st.x.c.col, st.x.c.line);
                //DeleteStream(doc, col, line,
                      //st.x.c.col, st.x.c.line, FALSE);
                DeleteBlock(doc, col, line,
                      st.x.c.col, st.x.c.line);
                break;

              case DELETECHAR :
                //%AuxPrintf(1, "DELETECHAR at (%i,%i)", col, line);
                //DeleteStream(doc, col, line, st.col + 1,
                      //st.line, FALSE);
                DeleteBlock(doc, col, line, st.col + 1, st.line);
                break;

              case INSERTSTREAM :
                //InsertStream(doc, col, line, st.x.s.len, NULL, FALSE);
                InsertBlock(doc, col, line, st.x.s.len, NULL);

                //If stream is a CR+LF, we need to reposition
                //cursor at begin of next line

                if (st.x.s.len == 2
                    && ReadCharFromStreamBuf(&p, 1) == LF) {
                    line++;
                    col = 0;
                }
                if (action & REPLACEDBCS) {
                    col += st.x.s.len;
                }
                break;

              case INSERTCHAR :
                //%AuxPrintf(1, "INSERTCHAR at (%i,%i) '%c'",
                //%                      col, line, st.x.ch);
                //InsertStream(doc, col, line, 1, (LPSTR)&st.x.ch, FALSE);
                InsertBlock(doc, col, line, 1, (LPSTR)&st.x.ch);

                //Reposition cursor after char
                col++;

                break;

              case REPLACECHAR :
                //%AuxPrintf(1, "REPLACECHAR at (%i,%i) '%c'",
                //%                      col, line, st.x.ch);
                //ReplaceChar(doc, col, line, st.x.ch, FALSE);
                ReplaceCharInBlock(doc, col, line, st.x.ch);
                col++;
                break;

              default:
                Assert(FALSE);
                return FALSE;
                break;
            }

            //Reposition Cursor taking care of tabs

            PosXY(curView, col, line, FALSE);

            //%AuxPrintf(1, "Cursor at (%i,%i)", col, line);

            //Update tape counter
            if (recType == REC_UNDO) {
                docs->playCount += 1 - recordBufferOverflow;
                if (prompt)
                      SetMessageText_StatusBar(STA_Undo, STATUS_INFOTEXT, abs(docs->playCount));
            }
            else {
                docs->playCount--;
                if (prompt)
                      SetMessageText_StatusBar(STA_Redo, STATUS_INFOTEXT, abs(docs->playCount));
            }
        }

        //Prepare next play for the next record (going backward in buffer)
        if (!endOfRec)
                p.offset -= st.prevLen;
        else {

            if (recType == REC_UNDO) {
                if (prompt)
                        SetMessageText_StatusBar(STA_End_Of_Undo, STATUS_INFOTEXT);
            } else {
                if (prompt)
                        SetMessageText_StatusBar(STA_End_Of_Redo, STATUS_INFOTEXT);
            }
        }

        // Restore "REPLACEDBCS" flag.
        // Because some action can modify this flag.
        pstSave = (STREAMREC *)((LPSTR)p.pRec + p.offset);
        if (action & REPLACEDBCS) {
            pstSave->action |= REPLACEDBCS;
        } else {
            pstSave->action &= ~(REPLACEDBCS);
        }
        pass1 = FALSE;
        if (recType == REC_UNDO)
              docs->undo = p;
        else
              docs->redo = p;
    }

    stopMarkStatus = HAS_STOP;
    playingRecords = FALSE;

    switch(recType) {

      case REC_UNDO :

        //See if we have not played the first record
        if (endOfRec) {

            //Convention to know we are at end of undos
                  docs->playCount = -docs->playCount;

            //Reinitialize undo buffer
                  InitRecBuf(&p, p.bufferSize, FALSE);
        }

        DbgX(GlobalUnlock(docs->undo.h) == 0);

        break;

      case REC_REDO :
        DbgX(GlobalUnlock(docs->redo.h) == 0);
        break;

      default:
        Assert(FALSE);
        return FALSE;
        break;
    }
    return TRUE;
}                                       /* PlayRec() */


/***    DumpRec
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

void FAR PASCAL DumpRec(int doc, WORD recType)
{

    LPDOCREC docs = &Docs[doc];
    char s[512], t[50];
    int j, i = 0;
    UNDOREDOREC p;
    STREAMREC st;
    BOOL endOfRec;
    WORD action;

    if (docs->recType == REC_STOPPED)
          return;

    switch(recType) {
      case REC_UNDO :
        AuxPrintf(1, "Dumping Undo Buffer");
        p = docs->undo;
        break;
      case REC_REDO :
        AuxPrintf(1, "Dumping Redo Buffer");
        p = docs->redo;
        break;
      default:
        Assert(FALSE);
        return;
        break;
    }

    if (p.h == 0) {
        AuxPrintf(1, "No buffer");
        return;
    }

    if (FailGlobalLock(p.h, (LPSTR *)&(p.pRec))) {
        InternalErrorBox(SYS_Lock_Memory);
        return;
    }

    endOfRec = FALSE;
    while (!endOfRec) {

        //Get rec from buffer
        ReadFromBuf(&p, (LPSTR)&st, (WORD)sizeof(st));

        action = st.action;

        t[0] = '\0';
        if (action & STOPMARK)
              strcat(t, "STOP ");
        if (action & USERMARK)
              strcat(t, "USER ");
        if (action & CANCELREC)
              strcat(t, "CANCEL ");

        switch(action & ACTIONMASK) {

          case DELETESTREAM :
            AuxPrintf(1, "%3i : DELETESTREAM from (%i,%i) to (%i,%i)  %s",
            i, st.col, st.line, st.x.c.col, st.x.c.line, (LPSTR)t);
            break;

          case DELETECHAR :
            AuxPrintf(1, "%3i : DELETECHAR   at   (%i,%i)  %s",
            i, st.col, st.line, (LPSTR)t);
            break;

          case INSERTSTREAM :
            if (st.x.s.len < 512) {
                if ((long) (p.offset + HDR_INSERTSTREAM_SIZE) >= p.bufferSize)
                    p.offset -= p.bufferSize;
                LowLevelReadBuf((LPSTR)p.pRec,
                      p.offset + HDR_INSERTSTREAM_SIZE,
                      (LPSTR)s, st.x.s.len, p.bufferSize);
                s[st.x.s.len] = '\0';
                for (j = 0; j < (int)strlen(s); j++) {
                    if (IsDBCSLeadByte(s[j]) && s[j+1]) {
                        j++;
                        continue;
                    }
                    if (s[j] == CR)
                        s[j] = 'Ä';
                    if (s[j] == LF)
                        s[j] = '¿';
                }
            }
            else
                strcpy(s,"too long");
            AuxPrintf(1, "%3i : INSERTSTREAM at   (%i,%i) '%s' [%i]  %s",
                  i, st.col, st.line, (LPSTR)s, strlen(s), (LPSTR)t);
            break;

          case INSERTCHAR :
            AuxPrintf(1, "%3i : INSERTCHAR   at   (%i,%i) '%c'  %s",
                i, st.col, st.line, st.x.ch, (LPSTR)t);
            break;

          case REPLACECHAR :
            AuxPrintf(1, "%3i : REPLACECHAR  at   (%i,%i) '%c'  %s",
            i, st.col, st.line, st.x.ch, (LPSTR)t);
            break;

          default:
            Assert(FALSE);
            return;
            break;
        }

        endOfRec = (st.prevLen == 0);
        if (!endOfRec)
              p.offset -= st.prevLen;

        i++;

    }

    DbgX(GlobalUnlock(p.h) == 0);

    return;
}                                       /* DumpRec() */
