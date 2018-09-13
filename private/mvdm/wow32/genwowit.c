/* generate wowit.h and wowit.c from wow.it
 *
 *   20-Feb-1997 DaveHart created
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <sys\stat.h>
#include <fcntl.h>
#include <windows.h>


VOID ErrorAbort(PSZ pszMsg);
BYTE GetReturnOpcode(PSZ *ppsz);
PSZ GetApiName(PSZ *ppsz);
PSZ GetApi32Name(PSZ *ppsz, PSZ pszApi16);
BYTE GetArgOpcode(PSZ *ppsz);
PSZ GetOpcodeName(BYTE bInstr);
PSZ GetLine(PSZ pszBuf, int cbBuf, FILE *fp);
VOID ReadTypeNames(FILE *fIn, PSZ szTypesPrefix, PSZ *OpcodeNamesArray, int *pnOpcodeNames);
PSZ DateTimeString(VOID);

#define IS_RET_OPCODE(b) (b & 0x80)

#define MAX_IT_INSTR 16

typedef struct tagITINSTR {
    int  cbInstr;
    int  offSwamp;
    BYTE Instr[MAX_IT_INSTR];
} ITINSTR;

#define MAX_INSTR_TABLE_SIZE 512
ITINSTR InstrTable[MAX_INSTR_TABLE_SIZE];
int     iNextInstrSlot = 0;

typedef struct tagTHUNKTABLESLOT {
    PSZ   pszAPI;
    PSZ   pszAPI32;                 // if Win32 routine name doesn't match pszAPI
    int   iInstrSlot;
    int   cbInstr;                  // how much of this slot we're using
} THUNKTABLESLOT;

#define MAX_THUNK_TABLE_SIZE 1024
THUNKTABLESLOT ThunkTable[MAX_THUNK_TABLE_SIZE];
int iNextThunkSlot = 0;

#define MAX_ARG_OPCODE_NAMES 32
PSZ ArgOpcodeNames[MAX_ARG_OPCODE_NAMES];
int nArgOpcodeNames;

#define MAX_RET_OPCODE_NAMES 32
PSZ RetOpcodeNames[MAX_RET_OPCODE_NAMES];
int nRetOpcodeNames;

static char szArgumentTypes[] = "Argument Types:";
static char szReturnTypes[] = "Return Types:";

int __cdecl main(int argc, char **argv)
{
    FILE *fIn, *fOutH, *fOutC;
    char szBuf[256], szOff1[32], szOff2[32];
    PSZ psz, pszAPI, pszAPI32;
    ITINSTR ThisInstr;
    BYTE bRetInstr;
    BYTE *pbInstr;
    int i, iSwampOffset;
    int iMaxArgs = 0;
    int cbDiff;

    if (argc != 2) {
        ErrorAbort("Usage:\n  genwowit <inputfile>\n");
    }

    if (!(fIn = fopen(argv[1], "rt"))) {
        ErrorAbort("Unable to open input file\n");
    }

    //
    // The input file (wow.it) uses # to begin comment lines.
    // Aside from comments, it must begin with two special lines
    // to define the available type names for arguments and
    // function return values.
    //
    // They look like:
    //
    // Argument Types: WORD, INT, DWORD, LPDWORD, PTR, PTRORATOM, HGDI, HUSER, COLOR, HINST, HICON, POINT, 16ONLY, 32ONLY;
    // Return Types: DWORD, WORD, INT, HGDI, HUSER, ZERO, HICON, ONE, HPRNDWP;
    //
    // Read these lines into the ArgOpcodeNames and RetOpcodeNames arrays.
    //

    ReadTypeNames(fIn, szArgumentTypes, ArgOpcodeNames, &nArgOpcodeNames);
    ReadTypeNames(fIn, szReturnTypes, RetOpcodeNames, &nRetOpcodeNames);

    //
    // Each input line in the main part has a very restricted syntax:
    //
    // RETTYPE Api16[=Api32](TYPE1, TYPE2, ... TYPEn);  # comment
    //
    // If Api32 isn't specified it's the same as Api16.
    // The types come from the set above only.
    //
    // Actually everything following the ) is ignored now.
    //

    while (GetLine(szBuf, sizeof szBuf, fIn)) {

        psz = szBuf;

        //
        // Pick up the return type, space-delimited
        //

        bRetInstr = GetReturnOpcode(&psz);

        //
        // Pick up the API name, leaving psz pointing past the open-paren
        //

        pszAPI = GetApiName(&psz);

        //
        // Pick up the 32-bit name if it exists
        //

        pszAPI32 = GetApi32Name(&psz, pszAPI);

        //
        // Pick up the arg types into Instr array
        //

        memset(&ThisInstr, 0, sizeof ThisInstr);
        pbInstr = ThisInstr.Instr;

        while (*psz && *psz != ')') {
            *pbInstr++ = GetArgOpcode(&psz);
        }

        //
        // Keep track of the max used args
        //

        iMaxArgs = max(iMaxArgs, (pbInstr - ThisInstr.Instr));

        //
        // Tack on the return opcode
        //

        *pbInstr++ = bRetInstr;

        //
        // Record instruction bytes used for this one.
        //

        ThisInstr.cbInstr = (pbInstr - ThisInstr.Instr);

        //
        // Make sure we haven't overrun
        //

        if ( ThisInstr.cbInstr > MAX_IT_INSTR ) {
            printf("Thunk for %s too many args (%d) increase MAX_IT_INSTR beyond %d.\n",
                   pszAPI, ThisInstr.cbInstr, MAX_IT_INSTR);
            ErrorAbort("Increase MAX_IT_INSTR in intthunk.h\n");
        }

        //
        // Now we have a fully-formed opcode stream, see if we can pack it
        // in with any previously recorded ones.  Walk through the table
        // from the start looking for any entry which already contains this
        // opcode sequence (possibly as part of a longer sequence) or which
        // is itself contained by this opcode sequence.  If we find one,
        // change it to be the longer sequence if needed and use it.  We'll
        // distinguish later between the multiple uses using the cbInstr in
        // each thunk table entry.  The logic here assumes the matches will
        // always be at the end, since ret opcodes always have 0x80 bit set
        // and no others do, and each sequence ends with one.
        //

        for (i = 0; i < iNextInstrSlot; i++) {
            //if (0 == memcmp(Instr, InstrTable[i], sizeof Instr)) {
            //    break;
            //}

            //
            // Is ThisInstr a subsequence of this table entry?
            //

            if (ThisInstr.cbInstr <= InstrTable[i].cbInstr &&
                0 == memcmp(ThisInstr.Instr,
                            InstrTable[i].Instr + (InstrTable[i].cbInstr -
                                                   ThisInstr.cbInstr),
                            ThisInstr.cbInstr)) {

                break;
            }

            //
            // Is this table entry a subsequence of ThisInstr?
            //

            if (InstrTable[i].cbInstr < ThisInstr.cbInstr &&
                0 == memcmp(InstrTable[i].Instr,
                            ThisInstr.Instr + (ThisInstr.cbInstr -
                                               InstrTable[i].cbInstr),
                            InstrTable[i].cbInstr)) {

                //
                // Blast the longer ThisInstr over the existing shorter
                // instruction.
                //

                memcpy(&InstrTable[i], &ThisInstr, sizeof InstrTable[i]);
                break;
            }

            //
            // Check the next instruction table entry.
            //
        }

        //
        // If we didn't find a match, add to the end.
        //

        if (i == iNextInstrSlot) {
            memcpy(&InstrTable[i], &ThisInstr, sizeof InstrTable[i]);
            iNextInstrSlot++;

            if (iNextInstrSlot == MAX_INSTR_TABLE_SIZE) {
                ErrorAbort("Increase MAX_INSTR_TABLE_SIZE in genwowit.c\n");
            }
        }

        //
        // Add this one to the thunk table.
        //

        ThunkTable[iNextThunkSlot].pszAPI = pszAPI;
        ThunkTable[iNextThunkSlot].pszAPI32 = pszAPI32;
        ThunkTable[iNextThunkSlot].iInstrSlot = i;
        ThunkTable[iNextThunkSlot].cbInstr = ThisInstr.cbInstr;
        iNextThunkSlot++;

        if (iNextThunkSlot == MAX_THUNK_TABLE_SIZE) {
            ErrorAbort("Increase MAX_THUNK_TABLE_SIZE in genwowit.c\n");
        }
    }

    //
    // Now we're ready to output the results.
    //

    if (!(fOutH = fopen("wowit.h", "wt"))) {
        ErrorAbort("Cannot open wowit.h output file\n");
    }

    fprintf(fOutH,
            "//\n"
            "// DO NOT EDIT.\n"
            "//\n"
            "// wowit.h generated by genwowit.exe from wow.it on\n"
            "//\n"
            "//   %s\n"
            "//\n\n", DateTimeString());

    fprintf(fOutH, "#include \"intthunk.h\"\n\n");

    fprintf(fOutH, "#define MAX_IT_ARGS  %d\n\n", iMaxArgs);

    //
    // Spit out the two types of opcode manifests.
    //

    for (i = 0; i < nArgOpcodeNames; i++) {
        fprintf(fOutH, "#define IT_%-20s ( (UCHAR) 0x%x )\n", ArgOpcodeNames[i], i);
    }

    fprintf(fOutH, "\n#define IT_RETMASK              ( (UCHAR) 0x80 )\n");

    for (i = 0; i < nRetOpcodeNames; i++) {
        sprintf(szBuf, "%sRET", RetOpcodeNames[i]);
        fprintf(fOutH, "#define IT_%-20s ( IT_RETMASK | (UCHAR) 0x%x )\n", szBuf, i);
    }

    fprintf(fOutH, "\n");

    //
    // ITID_ manifests map an API name to its slot
    // in the thunk table.  Each one looks like:
    //
    // #define ITID_ApiName               0
    //

    for (i = 0; i < iNextThunkSlot; i++) {
        fprintf(fOutH, "#define ITID_%-40s %d\n", ThunkTable[i].pszAPI, i);
    }

    fprintf(fOutH, "\n#define ITID_MAX %d\n", i-1);

    fclose(fOutH);


    //
    // wowit.c has two tables, the instruction table and
    // the thunk table.
    //

    if (!(fOutC = fopen("wowit.c", "wt"))) {
        ErrorAbort("Cannot open wowit.c output file\n");
    }

    fprintf(fOutC,
            "//\n"
            "// DO NOT EDIT.\n"
            "//\n"
            "// wowit.c generated by genwowit.exe from wow.it on\n"
            "//\n"
            "//   %s\n"
            "//\n\n", DateTimeString());


    fprintf(fOutC, "#include \"precomp.h\"\n");
    fprintf(fOutC, "#pragma hdrstop\n");
    fprintf(fOutC, "#define WOWIT_C\n");
    fprintf(fOutC, "#include \"wowit.h\"\n\n");

    //
    // Spit out the instruction table, packing bytes in the process
    // and filling in the aoffInstrTable array with offsets for each
    // entry in this program's InstrTable.  Those offsets are used
    // in writing the final thunk table.
    //

    iSwampOffset = 0;

    fprintf(fOutC, "CONST BYTE InstrSwamp[] = {\n");

    for (i = 0; i < iNextInstrSlot; i++) {

        fprintf(fOutC, "    /* %3d  0x%-3x */ ", i, iSwampOffset);

        pbInstr = InstrTable[i].Instr;
        InstrTable[i].offSwamp = iSwampOffset;

        do {
            fprintf(fOutC, "%s, ", GetOpcodeName(*pbInstr));
            iSwampOffset++;
        } while (!IS_RET_OPCODE(*pbInstr++));

        fprintf(fOutC, "\n");

    }

    fprintf(fOutC, "};\n\n");

    fprintf(fOutC, "CONST INT_THUNK_TABLEENTRY IntThunkTable[] = {\n");

    for (i = 0; i < iNextThunkSlot; i++) {

        //
        // Concatenate the API name followed by a comma into
        // szBuf, so the combination can be left-justified in the output.
        //

        sprintf(szBuf, "%s,", ThunkTable[i].pszAPI32);

        //
        // cbDiff is the offset into the instruction stream where
        // this thunks instruction stream begins.
        //

        cbDiff = InstrTable[ ThunkTable[i].iInstrSlot ].cbInstr -
                 ThunkTable[i].cbInstr;

        //
        // Format the swamp offset so it can be left-justified in the output.
        //

        sprintf(szOff1, "%x",
                InstrTable[ ThunkTable[i].iInstrSlot ].offSwamp + cbDiff);

        //
        // If this thunk table entry will point past the start of
        // an instruction (because of sharing), format the offset
        // past the start of the instruction into szOff2
        //

        if (cbDiff) {
            sprintf(szOff2, "+ %d ", cbDiff);
        } else {
            szOff2[0] = '\0';
        }

        fprintf(fOutC,
                "    /* %3d */ { (FARPROC) %-32s InstrSwamp + 0x%-4s },  /* %d %s*/ \n",
                i,
                szBuf,
                szOff1,
                ThunkTable[i].iInstrSlot,
                szOff2);
    }

    fprintf(fOutC, "};\n\n");

    fclose(fOutC);

    printf("Generated wowit.h and wowit.c from wow.it\n"
           "%d thunks, %d unique instruction streams, %d instruction bytes, %d max args.\n",
           iNextThunkSlot, iNextInstrSlot, iSwampOffset, iMaxArgs);

    return 0;
}



BYTE GetReturnOpcode(PSZ *ppsz)
{
    int i;
    char szBuf[32];
    PSZ psz;

    //
    // Copy the name up to the first space to szBuf,
    // then skip any remaining spaces leaving caller's
    // pointer pointing at API name.
    //

    psz = szBuf;
    while (**ppsz != ' ') {
        *psz++ = *((*ppsz)++);
    };

    *psz = 0;

    while (**ppsz == ' ') {
        (*ppsz)++;
    };

    i = 0;
    while (i < nRetOpcodeNames &&
           strcmp(szBuf, RetOpcodeNames[i])) {
        i++;
    }

    if (i == nRetOpcodeNames) {
        printf("%s is not a valid return type.\n", szBuf);
        ErrorAbort("Invalid return type.\n");
    }

    return (BYTE)i | 0x80;
}



PSZ GetApiName(PSZ *ppsz)
{
    char szBuf[128];
    PSZ psz;

    //
    // Copy the name up to the first space or open-paren or equals sign
    // to szBuf, then skip any remaining spaces and open-parens leaving caller's
    // pointer pointing at first arg type or equals sign
    //

    psz = szBuf;
    while (**ppsz != ' ' && **ppsz != '(' && **ppsz != '=') {
        *psz++ = *((*ppsz)++);
    };

    *psz = 0;

    while (**ppsz == ' ' || **ppsz == '(') {
        (*ppsz)++;
    };

    if (!strlen(szBuf)) {
        ErrorAbort("Empty API name\n");
    }

    return _strdup(szBuf);
}



PSZ GetApi32Name(PSZ *ppsz, PSZ pszApi16)
{
    char szBuf[128];
    PSZ psz;

    if (**ppsz != '=') {
        return pszApi16;
    }

    (*ppsz)++;  // skip =

    //
    // Copy the name up to the first space or open-paren
    // to szBuf, then skip any remaining spaces and open-parens leaving caller's
    // pointer pointing at first arg type
    //

    psz = szBuf;
    while (**ppsz != ' ' && **ppsz != '(') {
        *psz++ = *((*ppsz)++);
    };

    *psz = 0;

    while (**ppsz == ' ' || **ppsz == '(') {
        (*ppsz)++;
    };

    if (!strlen(szBuf)) {
        ErrorAbort("Empty API32 name\n");
    }

    return _strdup(szBuf);
}




BYTE GetArgOpcode(PSZ *ppsz)
{
    char szBuf[32];
    PSZ psz;
    int i;

    //
    // Copy the name up to the first space or comma close-paren
    // to szBuf, then skip any remaining spaces and commas,
    // leaving caller's pointer pointing at next arg type
    // or close-paren.
    //

    psz = szBuf;
    while (**ppsz != ' ' && **ppsz != ',' && **ppsz != ')') {
        *psz++ = *((*ppsz)++);
    };

    *psz = 0;

    while (**ppsz == ' ' || **ppsz == ',') {
        (*ppsz)++;
    };

    //
    // szBuf has the type name, find it in the table.
    //

    i = 0;
    while (i < nArgOpcodeNames &&
           strcmp(szBuf, ArgOpcodeNames[i])) {
        i++;
    }

    if (i == nArgOpcodeNames) {
        printf("%s is not a valid arg type.\n", szBuf);
        ErrorAbort("Invalid arg type.\n");
    }

    return (BYTE)i;
}



PSZ GetOpcodeName(BYTE bInstr)
{
    char szBuf[64];

    if (!IS_RET_OPCODE(bInstr)) {
        sprintf(szBuf, "IT_%s", ArgOpcodeNames[bInstr]);
    } else {
        sprintf(szBuf, "IT_%sRET", RetOpcodeNames[bInstr & 0x7f]);
    }

    return _strdup(szBuf);
}



VOID ErrorAbort(PSZ pszMsg)
{
    printf("GENWOWIT : fatal error GWI0001: Unable to process wow.it: %s\n", pszMsg);
    exit(1);
}


//
// Read a line from the input file skipping
// comment lines with '#' in the first column.
//

PSZ GetLine(PSZ pszBuf, int cbBuf, FILE *fp)
{
    do {

        pszBuf = fgets(pszBuf, cbBuf, fp);

    } while (pszBuf && '#' == *pszBuf);

    return pszBuf;
}


//
// Read one of the two special lines at the start that
// define the available types.
//


VOID ReadTypeNames(FILE *fIn, PSZ pszTypesPrefix, PSZ *OpcodeNamesArray, int *pnOpcodeNames)
{
    char chSave, szBuf[512];
    PSZ psz, pszType;

    if ( ! GetLine(szBuf, sizeof szBuf, fIn) ||
         _memicmp(szBuf, pszTypesPrefix, strlen(pszTypesPrefix)) ) {

        ErrorAbort("First line of input file must be 'Argument Types:', second 'Return Types:' ...\n");
    }

    psz = szBuf + strlen(pszTypesPrefix);

    //
    // Skip whitespace and commas
    //

    while (' ' == *psz || '\t' == *psz) {
        psz++;
    }

    if ( ! *psz) {
        ErrorAbort("No types found.\n");
    }

    do {
        //
        // Now we're looking at the first character of the type name.
        //

        pszType = psz;

        //
        // Find next whitespace, comma, semi, or null and turn it into a null.
        // This turns this type name into a zero-terminated string.
        //

        while (*psz && ' ' != *psz && '\t' != *psz && ',' != *psz && ';' != *psz) {
            psz++;
        }

        chSave = *psz;
        *psz = 0;

        OpcodeNamesArray[*pnOpcodeNames] = _strdup(pszType);
        (*pnOpcodeNames)++;

        *psz = chSave;

        //
        // Skip whitespace and commas
        //

        while (' ' == *psz || '\t' == *psz || ',' == *psz) {
            psz++;
        }

    } while (*psz && ';' != *psz);

    if ( ! *pnOpcodeNames) {
        ErrorAbort("No types found.\n");
    }
}

//
// Return a formatted date/time string for now.
// Only checks system time once so that wowit.c and wowit.h
// will have same date/time string.
//

PSZ DateTimeString(VOID)
{
    static char sz[256];
    static int fSetupAlready;

    if (!fSetupAlready) {
        time_t UnixTimeNow;
        struct tm *ptmNow;

        fSetupAlready = TRUE;

        _tzset();

        time(&UnixTimeNow);

        ptmNow = localtime(&UnixTimeNow);

        strftime(sz, sizeof sz, "%#c", ptmNow);

        strcat(sz, " (");
        strcat(sz, _strupr(_tzname[0]));   // naughty me
        strcat(sz, ")");
    }

    return sz;
}
