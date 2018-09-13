/****************************************************************************/
/*                                                                          */
/*  rcx.C - AFX symbol info writer                                          */
/*                                                                          */
/*    Windows 3.5 Resource Compiler                                         */
/*                                                                          */
/****************************************************************************/

#include "rc.h"

/////////////////////////////////////////////////////////////////////////////

// Symbol information
static PFILE    fhFileMap;
static LONG     lFileMap;

static PFILE    fhResMap;
static LONG     lResMap;

static PFILE    fhRefMap;
static LONG     lRefMap;

static PFILE    fhSymList;
static LONG     lSymList;

static LONG     HdrOffset;

static CHAR     szEndOfResource[2] = {'$', '\000'};

static CHAR     szSymList[_MAX_PATH];
static CHAR     szFileMap[_MAX_PATH];
static CHAR     szRefMap[_MAX_PATH];
static CHAR     szResMap[_MAX_PATH];

static WCHAR    szName[] = L"HWB";


#define OPEN_FLAGS (_O_TRUNC | _O_BINARY | _O_CREAT | _O_RDWR)
#define PROT_FLAGS (S_IWRITE | S_IWRITE)

void
wtoa(
    WORD value,
    char* string,
    int radix
    )
{
    if (value == (WORD)-1)
        _itoa(-1, string, radix);
    else
        _itoa(value, string, radix);
}

int
ConvertAndWrite(
    PFILE fp,
    PWCHAR pwch
    )
{
    int  n;
    char szMultiByte[_MAX_PATH];        // assumes _MAX_PATH >= MAX_SYMBOL

    n = wcslen(pwch) + 1;
    n = WideCharToMultiByte(uiCodePage, 0,
                pwch, n,
                szMultiByte, MAX_PATH,
                NULL, NULL);
    return MyWrite(fp, (PVOID)szMultiByte, n);
}

VOID
WriteResHdr (
    FILE *fh,
    LONG size,
    WORD id
    )
{
    LONG     val;

    /* add data size and header size */
    MyWrite(fh, (PVOID)&size, sizeof(ULONG)); // will backpatch
    MyWrite(fh, (PVOID)&HdrOffset, sizeof(ULONG));

    /* add type and name */
    MyWrite(fh, (PVOID)szName, sizeof(szName));
    val = 0xFFFF;
    MyWrite(fh, (PVOID)&val, sizeof(WORD));
    MyWrite(fh, (PVOID)&id, sizeof(WORD));

    MyAlign(fh);

    /* add data struct version, flags, language, resource data version
    /*  and characteristics */
    val = 0;
    MyWrite(fh, (PVOID)&val, sizeof(ULONG));
    val = 0x0030;
    MyWrite(fh, (PVOID)&val, sizeof(WORD));
    MyWrite(fh, (PVOID)&language, sizeof(WORD));
    val = 2;
    MyWrite(fh, (PVOID)&val, sizeof(ULONG));
    MyWrite(fh, (PVOID)&characteristics, sizeof(ULONG));
}

BOOL
InitSymbolInfo(
    void
    )
{
    PCHAR   szTmp;

    if (!fAFXSymbols)
        return(TRUE);

    if ((szTmp = _tempnam(NULL, "RCX1")) != NULL) {
        strcpy(szSymList, szTmp);
        free(szTmp);
    } else {
        strcpy(szSymList, tmpnam(NULL));
    }

    if ((szTmp = _tempnam(NULL, "RCX2")) != NULL) {
        strcpy(szFileMap, szTmp);
        free(szTmp);
    } else {
        strcpy(szFileMap, tmpnam(NULL));
    }

    if ((szTmp = _tempnam(NULL, "RCX3")) != NULL) {
        strcpy(szRefMap, szTmp);
        free(szTmp);
    } else {
        strcpy(szRefMap, tmpnam(NULL));
    }

    if ((szTmp = _tempnam(NULL, "RCX4")) != NULL) {
        strcpy(szResMap, szTmp);
        free(szTmp);
    } else {
        strcpy(szResMap, tmpnam(NULL));
    }

    if (!(fhFileMap = fopen(szFileMap, "w+b")) ||
        !(fhSymList = fopen(szSymList, "w+b")) ||
        !(fhRefMap  = fopen(szRefMap,  "w+b")) ||
        !(fhResMap  = fopen(szResMap,  "w+b")))
        return FALSE;

    /* calculate header size */
    HdrOffset = sizeof(szName);
    HdrOffset += 2 * sizeof(WORD);
    if (HdrOffset % 4)
        HdrOffset += sizeof(WORD);        // could only be off by 2
    HdrOffset += sizeof(RESADDITIONAL);

    WriteResHdr(fhSymList, lSymList, 200);
    WriteResHdr(fhFileMap, lFileMap, 201);
    WriteResHdr(fhRefMap, lRefMap, 202);
    WriteResHdr(fhResMap, lResMap, 2);

    return TRUE;
}

BOOL
TermSymbolInfo(
    PFILE fhResFile
    )
{
    long        lStart;
    PTYPEINFO   pType;
    RESINFO     r;

    if (!fAFXSymbols)
        return(TRUE);

    if (fhResFile == NULL_FILE)
        goto termCloseOnly;

    WriteSymbolDef(L"", L"", L"", 0, (char)0);
    MySeek(fhSymList, 0L, SEEK_SET);
    MyWrite(fhSymList, (PVOID)&lSymList, sizeof(lSymList));

    MySeek(fhFileMap, 0L, SEEK_SET);
    MyWrite(fhFileMap, (PVOID)&lFileMap, sizeof(lFileMap));

    WriteResInfo(NULL, NULL, FALSE);
    MySeek(fhRefMap, 0L, SEEK_SET);
    MyWrite(fhRefMap, (PVOID)&lRefMap, sizeof(lRefMap));

    // now append these to .res
    pType = AddResType(L"HWB", 0);
    r.flags = 0x0030;
    r.name = NULL;
    r.next = NULL;
    r.language = language;
    r.version = version;
    r.characteristics = characteristics;

    MySeek(fhSymList, 0L, SEEK_SET);
    MyAlign(fhResFile);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + HdrOffset;
    r.size = lSymList;
    r.nameord = 200;
    WriteResInfo(&r, pType, TRUE);
    MyCopyAll(fhSymList, fhResFile);

    MySeek(fhFileMap, 0L, SEEK_SET);
    MyAlign(fhResFile);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + HdrOffset;
    r.size = lFileMap;
    r.nameord = 201;
    WriteResInfo(&r, pType, TRUE);
    MyCopyAll(fhFileMap, fhResFile);

    MySeek(fhRefMap, 0L, SEEK_SET);
    MyAlign(fhResFile);
    r.BinOffset = MySeek(fhResFile, 0L, SEEK_END) + HdrOffset;
    r.size = lRefMap;
    r.nameord = 202;
    WriteResInfo(&r, pType, TRUE);
    MyCopyAll(fhRefMap, fhResFile);

    MyAlign(fhResFile);
    lStart = MySeek(fhResFile, 0L, SEEK_CUR);
    MySeek(fhResMap, 0L, SEEK_SET);
    MyWrite(fhResMap, (PVOID)&lResMap, sizeof(lResMap));
    MySeek(fhResMap, 0L, SEEK_SET);
    MyCopyAll(fhResMap, fhResFile);

    // patch the HWB:1 resource with HWB:2's starting point
    MySeek(fhResFile, lOffIndex, SEEK_SET);
    MyWrite(fhResFile, (PVOID)&lStart, sizeof(lStart));

    MySeek(fhResFile, 0L, SEEK_END);

termCloseOnly:;

    if (fhFileMap) {
        fclose(fhFileMap);
        remove(szFileMap);
    }

    if (fhRefMap) {
        fclose(fhRefMap);
        remove(szRefMap);
    }

    if (fhSymList) {
        fclose(fhSymList);
        remove(szSymList);
    }

    if (fhResMap) {
        fclose(fhResMap);
        remove(szResMap);
    }

    return TRUE;
}


void
WriteSymbolUse(
    PSYMINFO pSym
    )
{
    if (!fAFXSymbols)
        return;

    if (pSym == NULL) {
        WORD nID = (WORD)-1;

        lRefMap += MyWrite(fhRefMap, (PVOID)&szEndOfResource, sizeof(szEndOfResource));
        lRefMap += MyWrite(fhRefMap, (PVOID)&nID, sizeof(nID));
    } else {
        lRefMap += ConvertAndWrite(fhRefMap, pSym->name);
        lRefMap += MyWrite(fhRefMap, (PVOID)&pSym->nID, sizeof(pSym->nID));
    }
}


void
WriteSymbolDef(
    PWCHAR name,
    PWCHAR value,
    PWCHAR file,
    WORD line,
    char flags
    )
{
    if (!fAFXSymbols)
        return;

    if (name[0] == L'$' && value[0] != L'\0') {
        RESINFO     res;
        TYPEINFO    typ;

        res.nameord  = (USHORT) -1;
        res.language = language;
        typ.typeord  = (USHORT) -1;
        WriteFileInfo(&res, &typ, value);
        return;
    }

    lSymList += ConvertAndWrite(fhSymList, name);
    lSymList += ConvertAndWrite(fhSymList, value);

    lSymList += MyWrite(fhSymList, (PVOID)&line, sizeof(line));
    lSymList += MyWrite(fhSymList, (PVOID)&flags, sizeof(flags));
}


void
WriteFileInfo(
    PRESINFO pRes,
    PTYPEINFO pType,
    PWCHAR szFileName
    )
{
    WORD n1 = 0xFFFF;

    if (!fAFXSymbols)
        return;

    if (pType->typeord == 0) {
        lFileMap += MyWrite(fhFileMap, (PVOID)pType->type,
                            (wcslen(pType->type) + 1) * sizeof(WCHAR));
    } else {
        WORD n2 = pType->typeord;

        if (n2 == (WORD)RT_MENUEX)
            n2 = (WORD)RT_MENU;
        else if (n2 == (WORD)RT_DIALOGEX)
            n2 = (WORD)RT_DIALOG;
        lFileMap += MyWrite(fhFileMap, (PVOID)&n1, sizeof(WORD));
        lFileMap += MyWrite(fhFileMap, (PVOID)&n2, sizeof(WORD));
    }

    if (pRes->nameord == 0) {
        lFileMap += MyWrite(fhFileMap, (PVOID)pRes->name,
                            (wcslen(pRes->name) + 1) * sizeof(WCHAR));
    } else {
        lFileMap += MyWrite(fhFileMap, (PVOID)&n1, sizeof(WORD));
        lFileMap += MyWrite(fhFileMap, (PVOID)&pRes->nameord, sizeof(WORD));
    }

    lFileMap += MyWrite(fhFileMap, (PVOID)&pRes->language, sizeof(WORD));
    lFileMap += MyWrite(fhFileMap, (PVOID)szFileName,
                        (wcslen(szFileName) + 1) * sizeof(WCHAR));
}


void
WriteResInfo(
    PRESINFO pRes,
    PTYPEINFO pType,
    BOOL bWriteMapEntry
    )
{
    if (!fAFXSymbols)
        return;

    if (pRes == NULL) {
        WORD nID = (WORD)-1;

        //assert(bWriteMapEntry == FALSE);
        lRefMap += MyWrite(fhRefMap, (PVOID)&szEndOfResource, sizeof(szEndOfResource));
        lRefMap += MyWrite(fhRefMap, (PVOID)&nID, sizeof(nID));

        return;
    }

    if (bWriteMapEntry) {
        WORD n1 = 0xFFFF;
        ULONG t0 = 0;

        /* add data size and data offset */
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->size, sizeof(ULONG));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->BinOffset, sizeof(ULONG));

        /* Is this an ordinal type? */
        if (pType->typeord) {
            WORD n2 = pType->typeord;

            if (n2 == (WORD)RT_MENUEX)
                n2 = (WORD)RT_MENU;
            else if (n2 == (WORD)RT_DIALOGEX)
                n2 = (WORD)RT_DIALOG;
            lResMap += MyWrite(fhResMap, (PVOID)&n1, sizeof(WORD));
            lResMap += MyWrite(fhResMap, (PVOID)&n2, sizeof(WORD));
        } else {
            lResMap += MyWrite(fhResMap, (PVOID)pType->type,
                               (wcslen(pType->type) + 1) * sizeof(WCHAR));
        }

        if (pRes->nameord) {
            lResMap += MyWrite(fhResMap, (PVOID)&n1, sizeof(WORD));
            lResMap += MyWrite(fhResMap, (PVOID)&pRes->nameord, sizeof(WORD));
        } else {
            lResMap += MyWrite(fhResMap, (PVOID)pRes->name,
                               (wcslen(pRes->name) + 1) * sizeof(WCHAR));
        }

        lResMap += MyAlign(fhResMap);

        /* add data struct version, flags, language, resource data version
        /*  and characteristics */
        lResMap += MyWrite(fhResMap, (PVOID)&t0, sizeof(ULONG));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->flags, sizeof(WORD));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->language, sizeof(WORD));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->version, sizeof(ULONG));
        lResMap += MyWrite(fhResMap, (PVOID)&pRes->characteristics, sizeof(ULONG));

        return;
    }

    if (pType->typeord == 0) {
        lRefMap += ConvertAndWrite(fhRefMap, pType->type);
    } else {
        char szID[33];
        WORD n2 = pType->typeord;

        if (n2 == (WORD)RT_MENUEX)
            n2 = (WORD)RT_MENU;
        else if (n2 == (WORD)RT_DIALOGEX)
            n2 = (WORD)RT_DIALOG;

        wtoa(n2, szID, 10);
        lRefMap += MyWrite(fhRefMap, (PVOID)szID, strlen(szID)+1);
    }

    if (pRes->nameord == 0) {
        lRefMap += ConvertAndWrite(fhRefMap, pRes->name);
    } else {
        char szID[33];

        wtoa(pRes->nameord, szID, 10);
        lRefMap += MyWrite(fhRefMap, (PVOID)szID, strlen(szID)+1);
    }

    lRefMap += ConvertAndWrite(fhRefMap, pRes->sym.name);
    lRefMap += ConvertAndWrite(fhRefMap, pRes->sym.file);
    lRefMap += MyWrite(fhRefMap,(PVOID)&pRes->sym.line,sizeof(pRes->sym.line));
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*      GetSymbolDef() - get a symbol def record and write out info          */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
GetSymbolDef(
    int fReportError,
    WCHAR curChar
    )
{
    SYMINFO sym;
    WCHAR   szDefn[_MAX_PATH];
    WCHAR   szLine[16];
    PWCHAR  p;
    CHAR    flags = 0;
    WCHAR   currentChar = curChar;

    if (!fAFXSymbols)
        return;

    currentChar = LitChar(); // get past SYMDEFSTART

    /* read the symbol name */
    p = sym.name;
    while ((*p++ = currentChar) != SYMDELIMIT)
        currentChar = LitChar();
    *--p = L'\0';
    if (p - sym.name > MAX_SYMBOL) {
        ParseError1(2247);
        return;
    }
    currentChar = LitChar(); /* read past the delimiter */

    p = szDefn;
    while ((*p++ = currentChar) != SYMDELIMIT)
        currentChar = LitChar();
    *--p = L'\0';
    currentChar = LitChar(); /* read past the delimiter */

    sym.file[0] = L'\0';

    p = szLine;
    while ((*p++ = currentChar) != SYMDELIMIT)
        currentChar = LitChar();
    *--p = L'\0';
    sym.line = (WORD)wcsatoi(szLine);
    currentChar = LitChar(); /* read past the delimiter */

    flags = (CHAR)currentChar;
    flags &= 0x7f; // clear the hi bit
    currentChar = LitChar(); /* read past the delimiter */

    /* leave positioned at last character (LitChar will bump) */
    if (currentChar != SYMDELIMIT) {
        ParseError1(2248);
    }

    WriteSymbolDef(sym.name, szDefn, sym.file, sym.line, flags);
}


/*---------------------------------------------------------------------------*/
/*                                                                           */
/* GetSymbol() - read a symbol and put id in the token if there              */
/*                                                                           */
/*---------------------------------------------------------------------------*/
void
GetSymbol(
    int fReportError,
    WCHAR curChar
    )
{
    WCHAR currentChar = curChar;

    token.sym.name[0] = L'\0';
    token.sym.file[0] = L'\0';
    token.sym.line = 0;

    if (!fAFXSymbols)
        return;

    /* skip whitespace */
    while (iswhite(currentChar))
        currentChar = LitChar();

    if (currentChar == SYMUSESTART) {
        WCHAR * p;
        int i = 0;
        WCHAR szLine[16];

        currentChar = LitChar(); // get past SYMUSESTART

        if (currentChar != L'\"') {
            ParseError1(2249);
            return;
        }
        currentChar = LitChar(); // get past the first \"

        /* read the symbol name */
        p = token.sym.name;
        while ((*p++ = currentChar) != SYMDELIMIT)
            currentChar = LitChar();
        *--p = L'\0';
        if (p - token.sym.name > MAX_SYMBOL) {
            ParseError1(2247);
            return;
        }
        currentChar = LitChar(); /* read past the delimiter */

        p = token.sym.file;
        while ((*p++ = currentChar) != SYMDELIMIT)
            currentChar = LitChar();
        *--p = L'\0';
        currentChar = LitChar(); /* read past the delimiter */

        p = szLine;
        while ((*p++ = currentChar) != L'\"')
            currentChar = LitChar();
        *--p = L'\0';
        token.sym.line = (WORD)wcsatoi(szLine);

        if (currentChar != L'\"') {
            ParseError1(2249);
            return;
        }

        currentChar = LitChar(); // get past SYMDELIMIT

        /* skip whitespace */
        while (iswhite(currentChar))
            currentChar = LitChar();
    }
}
