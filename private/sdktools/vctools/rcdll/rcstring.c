/****************************************************************************/
/*                                                                          */
/*  RCSTRING.C -                                                            */
/*                                                                          */
/*      StringTable and Accelerators Parsing Code                           */
/*                                                                          */
/****************************************************************************/

#include "rc.h"

PRESINFO pResString = NULL;      /* Used to add a stringtable     */
/* at the end of processing if a stringtable */
/* was found.                                */

static PRCSTRING pSTHeader;
/* Ptr to the start of the parsed STRINGTABLE. */


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  MyFAlloc() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PCHAR
MyFAlloc(
    UINT cb,
    PCHAR pb
    )
{
    PCHAR pbT;

    pbT = (PCHAR)MyAlloc(cb);
    if (!pbT)
        quit("RC : fatal error RW1025: Out of heap memory");
    if (pb) {
        memmove(pbT, pb, cb);
    } else {
        memset(pbT, 0, cb);
    }
    return pbT;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetTable() -                                                            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRESINFO
GetTable(
    PRESINFO pResTemp
    )
{
    PRCSTRING  pCurrent;
    PRCSTRING  pTrailer;
    CHAR       bDone = FALSE;
    USHORT     nStringID;
    PWCHAR     p;
    PSYMINFO   pCurrentSymbol;

    DWORD  TmpSize;
    PCHAR  TmpBuf;
    int    TmpRow;

    PreBeginParse(pResTemp, 2105);

    /* Does a string table already exist? */
    if (pResString == NULL) {
        /* No, start at the beginning - otherwise append. */
        pTrailer = (PRCSTRING)NULL;
        pSTHeader = (PRCSTRING)NULL;
    }

    do {
        pCurrent = pSTHeader;
        bDone = FALSE;
        if (token.type != NUMLIT)
            ParseError1(2149); //"Expected numeric constant in string table "

        nStringID = token.val;

        pCurrentSymbol = (SYMINFO*)MyFAlloc(sizeof(token.sym), (char*)&token.sym);

        if (!GetFullExpression(&nStringID, GFE_ZEROINIT | GFE_SHORT))
            ParseError1(2110); //"Expected numeric constant in v table "

        if (token.type == COMMA)
            GetToken(TOKEN_NOEXPRESSION);

        if (token.type != LSTRLIT)
            ParseError1(2150);

        tokenbuf[token.val + 1] = 0;

        TmpSize = sizeof(WCHAR) * (token.val + 2);
        TmpBuf = MyFAlloc(TmpSize, (PCHAR)tokenbuf);
        TmpRow = token.row;
        GetToken(TRUE);

//        printf("TmpSize: %d\tTmpBuf: %ws\tTmpRow: %d\n", TmpSize, TmpBuf, TmpRow);

        while ((token.row == TmpRow) && (token.type == LSTRLIT)) {
            ULONG NewSize = TmpSize + (sizeof(WCHAR) * (token.val));
            PCHAR NewBuf = (PCHAR)MyAlloc(NewSize);
            if (!NewBuf)
                quit("RC : fatal error RW1025: Out of heap memory");

            memmove(NewBuf, TmpBuf, TmpSize);
            memmove(NewBuf+TmpSize-4, tokenbuf, (token.val * sizeof(WCHAR)));

//            printf("NewSize: %d\tNewBuf: %ws\ttoken.row: %d\ttokenbuf: %ws\n", NewSize, NewBuf, token.row, tokenbuf);

            MyFree(TmpBuf);
            TmpSize = NewSize;
            TmpBuf = NewBuf;
            GetToken(TRUE);
        }

        while (!bDone && pCurrent) {
            if (pCurrent->language == pResTemp->language) {
                if (pCurrent->hibits == (USHORT)(nStringID / BLOCKSIZE)) {
                    bDone = TRUE;
                    if (!(pCurrent->rgsz[nStringID % BLOCKSIZE])) {
                        pCurrent->rgsz[nStringID % BLOCKSIZE] = (PWCHAR) TmpBuf;
                        pCurrent->rgsym[nStringID % BLOCKSIZE] = pCurrentSymbol;
                    }
                    else {
                        SET_MSG(Msg_Text, sizeof(Msg_Text),
                                GET_MSG(2151),
                                curFile,
                                TmpRow,
                                nStringID,
                                pCurrent->rgsz[nStringID % BLOCKSIZE],
                                TmpBuf
                                );

                        ParseError3(2151);
                        MyFree(TmpBuf);
                    }

                    TmpBuf = NULL; TmpSize = 0;
                }
            }
            pTrailer = pCurrent;
            pCurrent = pCurrent->next;
        }

        if (!bDone) {       /* and thus pCurrent == (PCHAR)NULL */
            pCurrent = (PRCSTRING)MyFAlloc(
                        sizeof(RCSTRING), (PCHAR)NULL);
            pCurrent->hibits = (short)(nStringID / BLOCKSIZE);
            pCurrent->flags  = pResTemp->flags;
            pCurrent->language = pResTemp->language;
            pCurrent->version = pResTemp->version;
            pCurrent->characteristics = pResTemp->characteristics;

            p = pCurrent->rgsz[nStringID%BLOCKSIZE] = (PWCHAR)TmpBuf;
            TmpBuf = NULL; TmpSize = 0;

            pCurrent->rgsym[nStringID%BLOCKSIZE] = pCurrentSymbol;

            if (pTrailer)
                pTrailer->next = pCurrent;

            if (!pSTHeader)
                pSTHeader = pCurrent;           /* First time only */
        }

//        GetToken(TRUE);
    } while (token.type != END);

    pResString = pResTemp;

    return pResString;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  WriteTable() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
WriteTable(
    PRESINFO pResOld
    )
{
    PRCSTRING   p;
    int         i;
    PRESINFO    pRes;
    PTYPEINFO   pType;
    int         n;
    PWCHAR      s;
    UINT        nBytesWritten;
    SYMINFO     symInfo;

    /* Start at the start of the proper table. */
    p = pSTHeader;

    while (p) {
        nBytesWritten = 0;

        CtlInit();

        // 'STR#' resource starts with a count of strings
        if (fMacRsrcs)
            WriteWord(BLOCKSIZE);

        /* Write out the next block. */
        for (i = 0; i < BLOCKSIZE; i++) {
            n = 0;
            s = p->rgsz[i];

            if (fMacRsrcs) {
                WriteMacString(s, TRUE, TRUE);
                continue;
            }

            if (s) {
                while (s[n] || s[n + 1])
                    n++; // szsz terminated

                if (fAppendNull)
                    n++;
            }

            nBytesWritten += sizeof(WCHAR) * (n + 1);

            WriteWord((WORD)n);
            while (n--)
                WriteWord(*s++);
        }

        pRes = (RESINFO * )MyAlloc(sizeof(RESINFO));
        pRes->language = p->language;
        pRes->version = p->version;
        pRes->characteristics = p->characteristics;

        pType = AddResType(NULL, RT_STRING);

        pRes->size = nBytesWritten;

        /* Mark the resource as Moveable and Discardable. */
        pRes->flags = p->flags;

        /*We're in an origin 1 world here*/
        pRes->nameord = (short)(p->hibits + 1);
        SaveResFile(pType, pRes);

        memset(&symInfo, 0, sizeof(symInfo));
        WriteResInfo(pRes, pType, FALSE);
        for (i=0; i < BLOCKSIZE; i++) {
            WriteSymbolUse(p->rgsym[i] != NULL  && p->rgsz[i][0] != '\0' ? p->rgsym[i] : &symInfo);
        }
        WriteResInfo(NULL, NULL, FALSE);

        /* Move on to the next block. */
        p = p->next;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetAccelerators() _                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int
GetAccelerators(
    PRESINFO pRes
    )
{
    int count = 0;
    int ntype;
    WCHAR                 c;
    int bTypeSpecified;
    RCACCEL Accel;

    PreBeginParse(pRes, 2106);

    do {
        if (token.type == END)
            continue;
        bTypeSpecified = FALSE;
        ntype = token.type;
        if (token.type == END) {
            MarkAccelFlagsByte();
            WriteWord(0);
            WriteWord(0);
            WriteWord(0);
            WriteWord(0);
            count++;
            continue;
        }
        else if (token.type == NUMLIT)
            Accel.ascii = token.val;
        else if (token.type == LSTRLIT) {
            if (tokenbuf[0] == L'^') {
                if (wcslen(tokenbuf) != 2)
                    ParseError1(2152);
                /* GetAccelerators() and support "^^" to put ^ */
                if (tokenbuf[1] == L'^')
                    Accel.ascii = L'^';
                else {
                    if (!iswalpha(c=towupper(tokenbuf[1])))
                        ParseError1(2154);

                    Accel.ascii = c - L'A' + 1;
                }
            }
            else if (wcslen(tokenbuf) == 2)
                Accel.ascii = (WCHAR)((tokenbuf[0] << 8) + tokenbuf[1]);
            else if (wcslen(tokenbuf) == 1)
                Accel.ascii = tokenbuf[0];
            else
                ParseError1(2155);
        }
        else
            ParseError1(2156);

        /* Get the trailing comma. */
        GetToken(TRUE);
        if (token.type != COMMA)
            ParseError1(2157);

        /* Get the next number. */
        GetToken(TRUE);
        if (token.type != NUMLIT)
            ParseError1(2107);

        Accel.id = token.val;

        WriteSymbolUse(&token.sym);

        if (!GetFullExpression(&Accel.id, GFE_ZEROINIT | GFE_SHORT))
            ParseError1(2107); //"Expected numeric command value"

        Accel.flags = 0;

        if (token.type == COMMA)
            do {
                GetToken(TRUE);
                switch (token.type) {
                    case TKVIRTKEY:
                        Accel.flags |= fVIRTKEY;
                        bTypeSpecified = TRUE;
                        break;
                    case TKASCII:
                        bTypeSpecified = TRUE;
                        break;  /* don't set the flag */
                    case TKNOINVERT:
                        Accel.flags |= fNOINVERT;
                        break;
                    case TKSHIFT:
                        Accel.flags |= fSHIFT;
                        break;
                    case TKCONTROL:
                        Accel.flags |= fCONTROL;
                        break;
                    case TKALT:
                        Accel.flags |= fALT;
                        break;
                    default:
                        ParseError1(2159);
                }
                GetToken(TRUE);
            } while (token.type == COMMA);

        if (ntype == NUMLIT && !bTypeSpecified)
            ParseError1(2163);

        if (!(Accel.flags & fVIRTKEY) && (Accel.flags & (fSHIFT | fCONTROL))) {
            SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4203), curFile, token.row);
            SendError(Msg_Text);
        }

        if (Accel.flags & fVIRTKEY && ntype == LSTRLIT) {
            if (!iswalnum(Accel.ascii = (WCHAR)towupper(Accel.ascii))) {
                SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4204), curFile, token.row);
                SendError(Msg_Text);
            }
        }

        MarkAccelFlagsByte();
        WriteWord(Accel.flags);
        WriteWord(Accel.ascii);
        if (fMacRsrcs) {
            WriteLong(Accel.id);
        } else {
            WriteWord(Accel.id);
            WriteWord(0);
        }

        count++;

    } while (token.type != END);

    PatchAccelEnd();

    return(5 * count);
}
