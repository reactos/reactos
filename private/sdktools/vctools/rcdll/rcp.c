/****************************************************************************/
/*                                                                          */
/*  RCP.C -                                                                 */
/*                                                                          */
/*      Resource Compiler 3.00 - Top Level Parsing routines                 */
/*                                                                          */
/****************************************************************************/

#include "rc.h"


static BOOL fFontDirRead = FALSE;

BOOL    bExternParse = FALSE;

WORD    language = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
LONG    version = 0;
LONG    characteristics = 0;

static int rowError = 0;
static int colError = 0;
static int idError = 0;


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseError3() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void
ParseError3(
    int id
    )
{
    // Don't get caught giving the same error over and over and over...
    if ((Nerrors > 0) && (idError == id) && (rowError == token.row) && (colError == token.col))
        quit("\n");

    SendError("\n");
    SendError(Msg_Text);

    if (++Nerrors > 25)
        quit("\n");

    rowError = token.row;
    colError = token.col;
    idError = id;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseError2() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void
ParseError2(
    int id,
    PWCHAR arg
    )
{
    // Don't get caught giving the same error over and over and over...
    if ((Nerrors > 0) && (idError == id) && (rowError == token.row) && (colError == token.col))
        quit("\n");

    SendError("\n");
    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(id), curFile, token.row, arg);
    SendError(Msg_Text);

    if (++Nerrors > 25)
        quit("\n");

    rowError = token.row;
    colError = token.col;
    idError = id;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseError1() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

void
ParseError1(
    int id
    )
{
    // Don't get caught giving the same error over and over and over...
    if ((Nerrors > 0) && (idError == id) && (rowError == token.row) && (colError == token.col))
        quit("\n");

    SendError("\n");
    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(id), curFile, token.row);
    SendError(Msg_Text);

    if (++Nerrors > 25)
        quit("\n");

    rowError = token.row;
    colError = token.col;
    idError = id;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetFileName() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* Read in a filename from the RC file. */

LONG
GetFileName(
    VOID
    )
{
    PFILE fh;
    LONG size;
    CHAR szFilename[_MAX_PATH];
    CHAR buf[_MAX_PATH];

    // Note: Always use the Ansi codepage here.  uiCodePage may have been
    // modified by a codepage pragma in the source file.

    WideCharToMultiByte(GetACP(), 0, tokenbuf, -1, buf, _MAX_PATH, NULL, NULL);
    searchenv(buf, "INCLUDE", szFilename);
    if ( szFilename[0] && ((fh = fopen(szFilename, "rb")) != NULL)) {
        size = MySeek(fh, 0L, SEEK_END);                /* find size of file */
        MySeek(fh, 0L, SEEK_SET);                       /* return to start of file */
        CtlFile(fh);
        return(size);
    } else {
        ParseError2(2135, (PWCHAR)buf);
        return 0;
    }
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddStringToBin() -                                                      */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* adds ordinal if non-zero, otherwise adds string.  In either case, puts */
/* it in a field of NCHARS [16] */

VOID
AddStringToBin(
    USHORT ord,
    WCHAR *sz
    )
{
    USHORT      n1 = 0xFFFF;

    /* Is this an ordinal type? */
    if (ord) {
        MyWrite(fhBin, (PCHAR)&n1, sizeof(USHORT));     /* 0xFFFF */
        MyWrite(fhBin, (PCHAR)&ord, sizeof(USHORT));
    } else {
        MyWrite(fhBin, (PVOID)sz, (wcslen(sz)+1) * sizeof(WCHAR));
    }
}


PWCHAR   pTypeName[] =
{
    NULL,                //  0
    L"CURSOR",           //  1 RT_CURSOR
    L"BITMAP",           //  2 RT_BITMAP
    L"ICON",             //  3 RT_ICON
    L"MENU",             //  4 RT_MENU
    L"DIALOG",           //  5 RT_DIALOG
    L"STRING",           //  6 RT_STRING
    L"FONTDIR",          //  7 RT_FONTDIR
    L"FONT",             //  8 RT_FONT
    L"ACCELERATOR",      //  9 RT_ACCELERATOR
    L"RCDATA",           // 10 RT_RCDATA
    L"MESSAGETABLE",     // 11 RT_MESSAGETABLE
    L"GROUP_CURSOR",     // 12 RT_GROUP_CURSOR
    NULL,                // 13 RT_NEWBITMAP -- according to NT
    L"GROUP_ICON",       // 14 RT_GROUP_ICON
    NULL,                // 15 RT_NAMETABLE
    L"VERSION",          // 16 RT_VERSION
    L"DIALOGEX",         // 17 RT_DIALOGEX     ;internal
    L"DLGINCLUDE",       // 18 RT_DLGINCLUDE
    L"PLUGPLAY",         // 19 RT_PLUGPLAY
    L"VXD",              // 20 RT_VXD
    L"ANICURSOR",        // 21 RT_ANICURSOR    ;internal
    L"ANIICON",          // 22 RT_ANIICON      ;internal
    L"HTML"              // 23 RT_HTML
};

// Note: Don't forget to update the same table in rcdump.c

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddBinEntry() -                                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* FORMAT: type, name, flags, length, bytes */

VOID
AddBinEntry(
    PTYPEINFO pType,
    PRESINFO pRes,
    PCHAR Array,
    int ArrayCount,
    LONG FileCount
    )
{
    ULONG       hdrSize = sizeof(RESADDITIONAL);
    ULONG       t0 = 0;
    ULONG       cbPad=0;

    if (!pRes->size)
        pRes->size = ResourceSize();

    if (pType->typeord == 0) {
        hdrSize += (wcslen(pType->type) + 1) * sizeof(WCHAR);
        cbPad += (wcslen(pType->type) + 1) * sizeof(WCHAR);
    } else {
        hdrSize += 2 * sizeof(WORD);
    }

    if (pRes->nameord == 0) {
        hdrSize += (wcslen(pRes->name) + 1) * sizeof(WCHAR);
        cbPad += (wcslen(pRes->name) + 1) * sizeof(WCHAR);
    } else {
        hdrSize += 2 * sizeof(WORD);
    }

    if (cbPad % 4)
        hdrSize += sizeof(WORD);        // could only be off by 2

    if (fVerbose) {
        if (pType->typeord == 0) {
            if (pRes->nameord == 0)
                wsprintfA(Msg_Text, "\nWriting %ws:%ws,\tlang:0x%x,\tsize %d",
                        pType->type, pRes->name, pRes->language, pRes->size);
            else
                wsprintfA(Msg_Text, "\nWriting %ws:%d,\tlang:0x%x,\tsize %d",
                        pType->type, pRes->nameord, pRes->language, pRes->size);
        } else {
            if (pRes->nameord == 0) {
                if (pType->typeord <= (USHORT)(UINT_PTR)RT_LAST)
                    wsprintfA(Msg_Text, "\nWriting %ws:%ws,\tlang:0x%x,\tsize %d",
                              pTypeName[pType->typeord],
                              pRes->name, pRes->language, pRes->size);
                else
                    wsprintfA(Msg_Text, "\nWriting %d:%ws,\tlang:0x%x,\tsize %d",
                              pType->typeord,
                              pRes->name, pRes->language, pRes->size);
            } else {
                if (pType->typeord <= (USHORT)(UINT_PTR)RT_LAST)
                    wsprintfA(Msg_Text, "\nWriting %ws:%d,\tlang:0x%x,\tsize %d",
                              pTypeName[pType->typeord],
                              pRes->nameord, pRes->language, pRes->size);
                else
                    wsprintfA(Msg_Text, "\nWriting %d:%d,\tlang:0x%x,\tsize %d",
                              pType->typeord,
                              pRes->nameord, pRes->language, pRes->size);
            }
        }
        printf(Msg_Text);
    }

    if (fMacRsrcs) {
        /* record file location for the resource map and dump out
        resource's size */
        DWORD dwT;
        pRes->BinOffset = (long)MySeek(fhBin,0L,1) - MACDATAOFFSET;
        dwT = SwapLong(pRes->size);
        MyWrite(fhBin, &dwT, 4);
    } else {
        /* add type, name, flags, and resource length */
        MyWrite(fhBin, (PCHAR)&pRes->size, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&hdrSize, sizeof(ULONG));

        AddStringToBin(pType->typeord, pType->type);
        AddStringToBin(pRes->nameord , pRes->name);
        MyAlign(fhBin);

        MyWrite(fhBin, (PCHAR)&t0, sizeof(ULONG));  /* data version */
        MyWrite(fhBin, (PCHAR)&pRes->flags, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&pRes->language, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&pRes->version, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&pRes->characteristics, sizeof(ULONG));

        /* record file location for the .EXE construction */
        pRes->BinOffset = (LONG)MySeek(fhBin, 0L, SEEK_CUR);
    }

    /* write array plus contents of resource source file */
    WriteControl(fhBin, Array, ArrayCount, FileCount);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddResToResFile(pType, pRes, Array, ArrayCount, FileCount)              */
/*                                                                          */
/*  Parameters:                                                             */
/*      pType  : Pointer to Res Type                                        */
/*      pRes   : Pointer to resource                                        */
/*      Array  : Pointer to array from which some data is to be copied into */
/*               the .RES file.                                             */
/*               This is ignored if ArrayCount is zero.                     */
/*      ArrayCount : This is the number of bytes to be copied from "Array"  */
/*                   into the .RES file. This is zero if no copy is required*/
/*      FileCount  : This specifies the number of bytes to be copied from   */
/*                   fhCode into fhOut. If this is -1, the complete input   */
/*                   file is to be copied into fhOut.                       */
/*                                                                          */
/*------------------------------------------------------------------------*/

VOID
AddResToResFile(
    PTYPEINFO pType,
    PRESINFO pRes,
    PCHAR Array,
    int ArrayCount,
    LONG FileCount
    )
{
    PRESINFO p;

    p = pType->pres;

    /* add resource to end of resource list for this type */
    if (p) {
        while (p->next)
            p = p->next;

        p->next = pRes;
    } else {
        pType->pres = pRes;
    }


    /* add the resource to the .RES File */
    AddBinEntry(pType, pRes, Array, ArrayCount, FileCount);

    /* keep track of number of resources and types */
    pType->nres++;
    ResCount++;
    WriteResInfo(pRes, pType, TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddResType() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PTYPEINFO
AddResType(
    PWCHAR s,
    LPWSTR l
    )
{
    PTYPEINFO  pType;

    if ((pType = pTypInfo) != 0) {
        for (; ; ) {
            /* search for resource type, return if already exists */
            if ((s && pType->type && !wcscmp(s, pType->type)) ||
                (!s && l && pType->typeord == (USHORT)l))
                return(pType);
            else if (!pType->next)
                break;
            else
                pType = pType->next;
        }

        /* if not in list, add space for it */
        pType->next = (PTYPEINFO)MyAlloc(sizeof(TYPEINFO));
        pType = pType->next;
    } else {
        /* allocate space for resource list */
        pTypInfo = (PTYPEINFO)MyAlloc(sizeof(TYPEINFO));
        pType = pTypInfo;
    }

    /* fill allocated space with name and ordinal, and clear the resources
         of this type */
    pType->type = MyMakeStr(s);
    pType->typeord = (USHORT)l;
    pType->nres = 0;
    pType->pres = NULL;

    return(pType);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  DGetMemFlags() -                                                        */
/*                                                                          */
/*--------------------------------------------------------------------------*/

int
DGetMemFlags (
    PRESINFO pRes
    )
{
    if (token.type == NUMLIT)
        // this is a numeric value, not a mem flag -- this means we're done
        // processing memory flags
        return(FALSE);

    /* adjust memory flags of resource */
    switch (token.val) {
        case TKMOVEABLE:
            pRes->flags |= NSMOVE;
            break;

        case TKFIXED:
            pRes->flags &= ~(NSMOVE | NSDISCARD);
            break;

        case TKPURE :
            pRes->flags |= NSPURE;
            break;

        case TKIMPURE :
            pRes->flags &= ~(NSPURE | NSDISCARD);
            break;

        case TKPRELOAD:
            pRes->flags |= NSPRELOAD;
            break;

        case TKLOADONCALL:
            pRes->flags &= ~NSPRELOAD;
            break;

        case TKDISCARD:
            pRes->flags |= NSMOVE | NSPURE | NSDISCARD;
            break;

        case TKEXSTYLE:
            GetToken(FALSE);        /* ignore '=' */
            if (token.type != EQUAL)
                ParseError1(2136);
            GetTokenNoComma(TOKEN_NOEXPRESSION);
            GetFullExpression(&pRes->exstyleT, GFE_ZEROINIT);
            break;

            /* if current token not memory flag, return FALSE to indicate not
             to continue parsing flags */
        default:
            return(FALSE);
    }

    GetToken(FALSE);

    /* TRUE ==> found memory flag */
    return(TRUE);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddDefaultTypes() -                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
AddDefaultTypes(
    VOID
    )
{
    AddResType(L"CURSOR", RT_GROUP_CURSOR);
    AddResType(L"ICON", RT_GROUP_ICON);
    AddResType(L"BITMAP", RT_BITMAP);
    AddResType(L"MENU", RT_MENU);
    AddResType(L"DIALOG", RT_DIALOG);
    AddResType(L"STRINGTABLE", RT_STRING);
    AddResType(L"FONTDIR", RT_FONTDIR);
    AddResType(L"FONT", RT_FONT);
    AddResType(L"ACCELERATORS", RT_ACCELERATOR);
    AddResType(L"RCDATA", RT_RCDATA);
    AddResType(L"MESSAGETABLE", RT_MESSAGETABLE);
    AddResType(L"VERSIONINFO", RT_VERSION);
    AddResType(L"DLGINCLUDE", RT_DLGINCLUDE);
    AddResType(L"MENUEX", RT_MENUEX);
    AddResType(L"DIALOGEX", RT_DIALOGEX);
    AddResType(L"PLUGPLAY", RT_PLUGPLAY);
    AddResType(L"VXD", RT_VXD);

    // AFX resource types.
    AddResType(L"DLGINIT", RT_DLGINIT);
    AddResType(L"TOOLBAR", RT_TOOLBAR);

    AddResType(L"ANIICON",   RT_ANIICON);
    AddResType(L"ANICURSOR", RT_ANICURSOR);

    AddResType(L"HTML", RT_HTML);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  AddFontDir() -                                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

VOID
AddFontDir(
    VOID
    )
{
    PRESINFO   pRes;
    PTYPEINFO  pType;
    PFONTDIR   pFont;

    /* make new resource */
    pRes = (PRESINFO)MyAlloc(sizeof(RESINFO));
    pRes->language = language;
    pRes->version = version;
    pRes->characteristics = characteristics;
    pRes->name = MyMakeStr(L"FONTDIR");

    /* find or create the type list */
    pType = AddResType(NULL, RT_FONTDIR);

    CtlInit();

    WriteWord(nFontsRead);

    pFont = pFontList;

    while (pFont) {
        WriteWord(pFont->ordinal);
        WriteBuffer((PCHAR)(pFont + 1), pFont->nbyFont);
        pFont = pFont->next;
    }

    pRes->flags = NSMOVE | NSPRELOAD;

    /* write to the .RES file */
    SaveResFile(pType, pRes);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadRF() -                                                              */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/* top level parsing function: recognizes an RC script */

int
ReadRF(
    VOID
    )
{
    PRESINFO    pRes;
    PTYPEINFO   pType;
    ULONG       zero=0;
    WORD        ffff=0xffff;
    ULONG       hdrSize = sizeof(RESADDITIONAL) + 2 * (sizeof(WORD) * 2);

    ResCount = 0;
    nFontsRead = 0;

    /* Initialize data structures. */
    AddDefaultTypes();

    if (!fMacRsrcs) {
        /* write 32-bit header for empty resource/signature */
        MyWrite(fhBin, (PCHAR)&zero, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&hdrSize, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&ffff, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&ffff, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(WORD));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(ULONG));
        MyWrite(fhBin, (PCHAR)&zero, sizeof(ULONG));
    }

    CtlAlloc();

    if (fAFXSymbols) {
        char* pch = inname;
        // write out first HWB resource

        CtlInit();
        pRes = (PRESINFO)MyAlloc(sizeof(RESINFO));
        pRes->language = language;
        pRes->version = version;
        pRes->characteristics = characteristics;

        pRes->size = sizeof(DWORD);
        pRes->flags = 0;
        pRes->name = 0;
        pRes->nameord = 1;
        WriteLong(0);           /* space for file pointer */
        while (*pch) {
            WriteByte(*pch++);
            pRes->size++;
        }
        WriteByte(0);
        pRes->size++;

        pType = AddResType(L"HWB", 0);
        SaveResFile(pType, pRes);
        lOffIndex = pRes->BinOffset;
    }

    /* Process the RC file. */
    do {
        token.sym.name[0] = L'\0';
        token.sym.nID = 0;

        /* Find the beginning of the next resource. */
        if (!GetNameOrd())
            break;

        if (!wcscmp(tokenbuf, L"LANGUAGE")) {
            language = GetLanguage();
            continue;
        } else if (!wcscmp(tokenbuf, L"VERSION")) {
            GetToken(FALSE);
            if (token.type != NUMLIT)
                ParseError1(2139);
            version = token.longval;
            continue;
        } else if (!wcscmp(tokenbuf, L"CHARACTERISTICS")) {
            GetToken(FALSE);
            if (token.type != NUMLIT)
                ParseError1(2140);
            characteristics = token.longval;
            continue;
        }

        /* Print a dot for each resource processed. */
        if (fVerbose) {
            printf(".");
        }

        /* Allocate space for the new resources Info structure. */
        pRes = (PRESINFO)MyAlloc(sizeof(RESINFO));
        pRes->language = language;
        pRes->version = version;
        pRes->characteristics = characteristics;

        if (token.sym.name[0]) {
            /* token has a real symbol associated with it */
            memcpy(&pRes->sym, &token.sym, sizeof(SYMINFO));
        } else {
            pRes->sym.name[0] = L'\0';
        }

        if (!token.val) {
            if (wcslen(tokenbuf) > MAXTOKSTR-1) {
                SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4206), curFile, token.row);
                SendError(Msg_Text);
                tokenbuf[MAXTOKSTR-1] = L'\0';
                token.val = MAXTOKSTR-2;
            }
            pRes->name = MyMakeStr(tokenbuf);
        } else {
            pRes->nameord = token.val;
        }

      /* If not a string table, find out what kind of resource follows.
       * The StringTable is a special case since the Name field is the
       * string's ID number mod 16.
       */
        if ((pRes->name == NULL) || wcscmp(pRes->name, L"STRINGTABLE")) {
            if (!GetNameOrd())
                break;

            if (!token.val) {
                if (wcslen(tokenbuf) > MAXTOKSTR-1) {
                    SET_MSG(Msg_Text, sizeof(Msg_Text), GET_MSG(4207), curFile, token.row);
                    SendError(Msg_Text);
                    tokenbuf[MAXTOKSTR-1] = L'\0';
                    token.val = MAXTOKSTR-2;
                }
                if (!wcscmp(tokenbuf, L"STRINGTABLE")) {
                    // User attempted to create a named string table... Bail
                    ParseError1(2255);
                    pRes->name = MyMakeStr(tokenbuf);
                    goto ItsAStringTable;
                }

                pType = AddResType(tokenbuf, MAKEINTRESOURCE(0));
            }
            else
                pType = AddResType(NULL, MAKEINTRESOURCE(token.val));

            if (!pType)
                return(errorCount == 0);

            /* Parse any user specified memory flags. */
            GetToken(FALSE);

            switch ((INT_PTR)pType->typeord) {
                    /* Calculated resources default to discardable. */
                case (INT_PTR)RT_ICON:
                case (INT_PTR)RT_CURSOR:
                case (INT_PTR)RT_FONT:
                case (INT_PTR)RT_DIALOG:
                case (INT_PTR)RT_MENU:
                case (INT_PTR)RT_DLGINCLUDE:
                case (INT_PTR)RT_DIALOGEX:
                case (INT_PTR)RT_MENUEX:
                    pRes->flags = NSMOVE | NSPURE | NSDISCARD;
                    break;

                case (INT_PTR)RT_GROUP_ICON:
                case (INT_PTR)RT_GROUP_CURSOR:
                    pRes->flags = NSMOVE | NSDISCARD;
                    break;

                    /* All other resources default to moveable. */
                default:
                    pRes->flags = NSMOVE | NSPURE;
                    break;
            }

            /* adjust according to the user's specifications
           */
            while (DGetMemFlags(pRes))
                ;

            // write out start of new resource
            WriteResInfo(pRes, pType, FALSE);
        } else {

ItsAStringTable:

            /* Parse any user specified memory flags. */
            GetToken(FALSE);

            /* String and Error resources default to discardable. */
            pRes->flags = NSMOVE | NSPURE | NSDISCARD;
            while (DGetMemFlags(pRes))
                ;

            pType = NULL;
        }

        if (!pType) {
            /* parse the string table, if that's what it is */
            if ((pRes->name != NULL) && (!wcscmp(pRes->name, L"STRINGTABLE"))) {
                if (GetTable(pRes) == NULL)
                    break;
            } else {
                ParseError1(2141);
            }
        } else {
            CtlInit();
            pRes->size = 0L;

            /* call parsing and generating functions specific to the various
             resource types */
            switch ((INT_PTR)pType->typeord) {
                case (INT_PTR)RT_DIALOGEX:
                    /* allocate dialog memory */
                    pLocDlg = (PDLGHDR) MyAlloc(sizeof(DLGHDR));

                    /* parse dialog box */
                    GetDlg(pRes, pLocDlg, TRUE);

                    /* write dialog box */
                    SaveResFile(AddResType(L"DIALOG", 0), pRes);

                    /* free dialog memory */
                    MyFree(pLocDlg);
                    break;

                case (INT_PTR)RT_DIALOG:
                    /* allocate dialog memory */
                    pLocDlg = (PDLGHDR) MyAlloc(sizeof(DLGHDR));

                    /* parse dialog box */
                    GetDlg(pRes, pLocDlg, FALSE);

                    /* write dialog box */
                    SaveResFile(pType, pRes);

                    /* free dialog memory */
                    MyFree(pLocDlg);
                    break;

                case (INT_PTR)RT_ACCELERATOR:
                    GetAccelerators(pRes);
                    SaveResFile(pType, pRes);
                    break;

                case (INT_PTR)RT_MENUEX:
                    WriteWord(MENUITEMTEMPLATEVERSIONNUMBER);
                    WriteWord(MENUITEMTEMPLATEBYTESINHEADER);
                    ParseMenu(FALSE, pRes);
                    SaveResFile(AddResType(L"MENU", 0), pRes);
                    break;

                case (INT_PTR)RT_MENU:
                    WriteWord(OLDMENUITEMTEMPLATEVERSIONNUMBER);
                    WriteWord(OLDMENUITEMTEMPLATEBYTESINHEADER);
                    ParseOldMenu(FALSE, pRes);
                    SaveResFile(pType, pRes);
                    break;

                case (INT_PTR)RT_ICON:
                case (INT_PTR)RT_CURSOR:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        if (FileIsAnimated(pRes->size)) {
                            goto ani;
                        } else {
                            pRes->size = GetIcon(pRes->size);
                            SaveResFile(pType, pRes);
                        }
                    }
                    break;

                case (INT_PTR) RT_ANIICON:
                case (INT_PTR) RT_ANICURSOR:
ani:
                    {
                        USHORT iLastTypeOrd = pType->typeord;

                        // Strictly speaking, ANIICON and ANICURSOR are not allowed.  However,
                        // we'll keep them around for the time being.  BryanT 8/14/96.
                        if ((pType->typeord == (USHORT)(INT_PTR)RT_ICON) ||
                            (pType->typeord == (USHORT)(INT_PTR)RT_GROUP_ICON))
                        {
                            pType->typeord = (USHORT)(INT_PTR)RT_ANIICON;
                        } else
                        if ((pType->typeord == (USHORT)(INT_PTR) RT_CURSOR) ||
                            (pType->typeord == (USHORT)(INT_PTR) RT_GROUP_CURSOR))
                        {
                            pType->typeord = (USHORT)(INT_PTR)RT_ANICURSOR;
                        }
                        WriteFileInfo(pRes, pType, tokenbuf);
                        pRes->size = GetFileName();
                        if (pRes->size) {
                            pRes->size = GetAniIconsAniCursors(pRes->size);
                            SaveResFile(pType, pRes);
                        }

                        pType->typeord = iLastTypeOrd;
                    }
                    break;

                case (INT_PTR)RT_BITMAP:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        /* Bitmap in DIB format */
                        pRes ->size = GetNewBitmap();
                        SaveResFile(pType, pRes);
                    }
                    break;

                case (INT_PTR)RT_GROUP_ICON:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        if (FileIsAnimated(pRes->size)) {
                            goto ani;
                        } else {
                            if (fMacRsrcs)
                                GetMacIcon(pType, pRes);
                            else
                                GetNewIconsCursors(pType, pRes, RT_ICON);
                        }
                    }
                    break;

                case (INT_PTR)RT_GROUP_CURSOR:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        if (FileIsAnimated(pRes->size)) {
                            goto ani;
                        } else {
                            if (fMacRsrcs)
                                GetMacCursor(pType, pRes);
                            else
                                GetNewIconsCursors(pType, pRes, RT_CURSOR);
                        }
                    }
                    break;

                case (INT_PTR)RT_FONT:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    pRes->size = GetFileName();
                    if (pRes->name)
                        ParseError1(2143);
                    if (AddFontRes(pRes)) {
                        nFontsRead++;
                        SaveResFile(pType, pRes);
                    }
                    break;

                case (INT_PTR)RT_FONTDIR:
                    WriteFileInfo(pRes, pType, tokenbuf);
                    fFontDirRead = TRUE;
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        SaveResFile(pType, pRes);
                    }
                    break;

                case (INT_PTR)RT_MESSAGETABLE:
                    pRes->size = GetFileName();
                    if (pRes->size) {
                        SaveResFile(pType, pRes);
                    }
                    break;

                case (INT_PTR)RT_VERSION:
                    VersionParse();
                    SaveResFile(pType, pRes);
                    break;

                case (INT_PTR)RT_DLGINCLUDE:
                    DlgIncludeParse(pRes);
                    SaveResFile(pType, pRes);
                    break;

                case (INT_PTR)RT_TOOLBAR:
                    GetToolbar(pRes);
                    SaveResFile(pType, pRes);
                    break;

                case (INT_PTR)RT_RCDATA:
                case (INT_PTR)RT_DLGINIT:
                default:
                    if (token.type != BEGIN) {
                        pRes->size = GetFileName();
                        if (pRes->size) {
                            WriteFileInfo(pRes, pType, tokenbuf);
                        }
                    } else {
                        RESINFO_PARSE rip;

                        bExternParse = FALSE;

                        // Check to see if caller wants to parse this.
                        if (lpfnParseCallback != NULL) {
                            rip.size = 0L;
                            rip.type = pType->type;
                            rip.typeord = pType->typeord;
                            rip.name = pRes->name;
                            rip.nameord = pRes->nameord;
                            rip.flags = pRes->flags;
                            rip.language = pRes->language;
                            rip.version = pRes->version;
                            rip.characteristics = pRes->characteristics;

                            bExternParse = (*lpfnParseCallback)(&rip, NULL, NULL);
                        }

                        if (!bExternParse) {
                            GetRCData(pRes);
                        } else {
                            CONTEXTINFO_PARSE cip;
                            CHAR mbuff[512];    // REVIEW: Long filenames??  See error.c also.

                            extern PCHAR CodeArray;
                            extern int CodeSize;
                            extern int CCount;

                            int nBegins = 1;
                            int nCountSave; // CCount before END token reached

                            cip.hHeap = hHeap;
                            cip.hWndCaller = hWndCaller;
                            cip.lpfnMsg = lpfnMessageCallback;
                            cip.line = token.row;

                            wsprintfA(mbuff, "%s(%%d) : %%s", curFile);
                            cip.format = mbuff;

                            // Collect data for caller to parse.
                            while(nBegins > 0) {
                                nCountSave = CCount;
                                GetToken(FALSE);
                                if (token.type == BEGIN)
                                    nBegins++;
                                else if (token.type == END)
                                    nBegins--;
                            }

                            bExternParse = FALSE;

                            if ((rip.size = nCountSave) > 0 &&
                                !(*lpfnParseCallback)(&rip, (void **) &CodeArray, &cip)) {
                                // Assume caller gave error message, and quit.
                                quit("\n");
                            }

                        pRes->size = CCount = CodeSize = rip.size;
                        }
                    }

                    SaveResFile(pType, pRes);
                    break;
            }
            // write out end of new resource
            WriteResInfo(NULL, NULL, FALSE);
        }
    } while (token.type != EOFMARK);

    /* if we added fonts without a font directory, add one */
    if (!fFontDirRead && nFontsRead)
        AddFontDir();

    /* write string table */
    if (pResString != NULL)
        WriteTable(pResString);

    /* write Mac resource map */
    if (fMacRsrcs)
        WriteMacMap();

    CtlFree();

    if (fVerbose) {
        printf("\n");
    }
    return(errorCount == 0);
}


WORD
GetLanguage()
{
    WORD    language;

    GetToken(FALSE);
    if (token.type != NUMLIT) {
        ParseError1(2144);
        return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    if (token.flongval) {
        ParseError1(2145);
        return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    language = token.val;
    GetToken(FALSE);
    if (token.type != COMMA) {
        ParseError1(2146);
        return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    GetToken(FALSE);
    if (token.type != NUMLIT) {
        ParseError1(2147);
        return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }
    if (token.flongval) {
        ParseError1(2148);
        return MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);
    }

    return MAKELANGID(language, token.val);
}
