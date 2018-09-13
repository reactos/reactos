//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
//     without Intel's express permission
//
/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//
// This module is only required for IA64
// Remove this module when IA64 support is in MSDIS.LIB
//

#if defined(_IA64_)

#define WINNT  1    // enable WIN_CDECL define in "disem.h"      09/03/96

#include "emdp.h"
#include "ia64dis.h"
#include "disem.h"

/*****************************************************************************/
// Temporary variables for IEL library

unsigned long IEL_t1, IEL_t2, IEL_t3, IEL_t4;
U64  IEL_et1, IEL_et2;
U128 IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
S128 IEL_ts1, IEL_ts2;

/*****************************************************************************/

ULONG    baseDefault = 16;        /* current display radix */

typedef LPCH FAR *LPLPCH;

#define MAXL     20

char    lhexdigit[] = { '0', '1', '2', '3', '4', '5', '6', '7',
               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
char    uhexdigit[] = { '0', '1', '2', '3', '4', '5', '6', '7',
               '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
char   *hexdigit = &uhexdigit[0];
static int fUpper = TRUE;

/* current position in instruction */
//static unsigned char FAR* pMem = (unsigned char *)NULL;

static int      EAsize  [2] = {0};  //  size of effective address item
static long     EAaddr  [2] = {0};  //  offset of effective address

int DumpAddress ( LPADDR, LPCH, int );
int DumpGeneric ( LSZ, LPCH, int );
int DumpComment ( LSZ, LPCH, int );
int DumpEA      ( HPID, HTID, LPADDR, LPCH, int );

void OutputAddr(LPLPCH, LPADDR, int );
void OutputHexString(LPLPCH, LPCH, int);
void OutputHexCode(LPLPCH, LPCH, int);



static CHAR    * PBuf;
static int      CchBuf;

#define DECODER_NAME "disem.X9.3.dll"
HINSTANCE hFalcon;

typedef
EM_Dis_Err
(*PEM_CLIENT_GEN_SYM) (
    const U64 address,
    char *sym_buf,
    unsigned int *sym_buf_length,
    U64 *offset
    );

typedef
EM_Dis_Err
(WIN_CDECL *PPFN_EM_DIS_SETUP) (
    const EM_Decoder_Machine_Type type,
    const EM_Decoder_Machine_Mode mode,
    const unsigned long print_flags,
    const EM_Dis_Radix radix,
    const EM_Dis_Style style,
    PEM_CLIENT_GEN_SYM clientgensym
    );

PPFN_EM_DIS_SETUP pfn_em_dis_setup;

typedef
EM_Dis_Err
(WIN_CDECL *PPFN_EM_DIS_INST) (
    const U64 *syl_location,
    const EM_Decoder_Machine_Mode mode,
    const unsigned char *bin_inst_buf,
    const unsigned int bin_inst_buf_length,
    unsigned int *actual_inst_length,
    char *ascii_inst_buf,
    unsigned int *ascii_inst_buf_length,
    EM_Dis_Fields *ascii_inst_fields
    );

PPFN_EM_DIS_INST pfn_em_dis_inst;

typedef
const char*
(WIN_CDECL *PPFN_EM_DIS_ERR_STR) (
    EM_Dis_Err error
    );

PPFN_EM_DIS_ERR_STR pfn_em_dis_err_str;

void CalcMain (HPID,HTID,DOP,ADDR,LPBYTE,int,int*,LPCH,int,LPCH,int,LPCH,int,LPCH,int);

/****disasm - disassemble an IA64 instruction
*
*  Input:
*   pOffset = pointer to offset to start disassembly
*   fEAout = if set, include EA (effective address)
*
*  Output:
*   pOffset = pointer to offset of next instruction
*   pchDst = pointer to result string
*
***************************************************************************/

#define CCHMAX 256
static char rgchDisasm [ CCHMAX ];
static HPID hpidLocal;
static HTID htidLocal;

XOSD
IA64Disasm (
    HPID   hpid,
    HTID   htid,
    LPSDI  lpsdi
    )
{
    XOSD xosd      = xosdNone;
    int  cchMax    = CCHMAX;
    DOP  dop       = lpsdi->dop;
    LPCH lpchOut   = rgchDisasm;
    int  ichCur    = 0;
    ADDR addrStart = lpsdi->addr;
    int  cch = 0;
    DWORD  cb;
    int  cbUsed=0;
    BYTE rgb [ MAXL ];

    char rgchRaw      [ MAXL * 2 + 1 ];
    char rgchPreg     [ 40 ];
    char rgchOpcode   [ 80 ];    // IA64 predicate
    char rgchOperands [ 80 ];
    char rgchEA       [ 44 ];
    char rgchComment  [ 80 ];

    hpidLocal = hpid;
    htidLocal = htid;
    _fmemset ( rgchRaw, 0, sizeof ( rgchRaw ) );
    _fmemset ( rgchPreg, 0, sizeof ( rgchPreg ) );    // zero out IA64 predicate buffer
    _fmemset ( rgchOpcode, 0, sizeof ( rgchOpcode ) );
    _fmemset ( rgchOperands, 0, sizeof ( rgchOperands ) );
    _fmemset ( rgchComment, 0, sizeof ( rgchComment ) );
    _fmemset ( rgchEA, 0, sizeof ( rgchEA ) );

    lpsdi->ichAddr      = -1;
    lpsdi->ichBytes     = -1;
    lpsdi->ichPreg      = -1;    // IA64 predicate, -1 means does not exist
    lpsdi->ichOpcode    = -1;
    lpsdi->ichOperands  = -1;
    lpsdi->ichComment   = -1;
    lpsdi->ichEA0       = -1;
    lpsdi->ichEA1       = -1;
    lpsdi->ichEA2       = -1;

    lpsdi->cbEA0        =  0;
    lpsdi->cbEA1        =  0;
    lpsdi->cbEA2        =  0;

    lpsdi->fAssocNext   =  0;

    lpsdi->lpch         = rgchDisasm;

    // Set up for upper or lower case

    fUpper = ( dop & dopUpper ) == dopUpper;
    if ( fUpper ) {
        hexdigit = uhexdigit;
    }
    else {
        hexdigit = lhexdigit;
    }

    ADDR_IS_FLAT( addrStart ) = TRUE;

    // Output the address if it is requested

    if ( ( dop & dopAddr ) == dopAddr ) {
        cch = DumpAddress ( &addrStart, lpchOut, cchMax );

        lpsdi->ichAddr = 0;
        cchMax        -= cch;
        lpchOut       += cch;
        ichCur        += cch;
    }

    // EM instruction can be disassembled only on a bundle basis. rgb therefore must contain the
    // binary of a whole bundle, not just an instruction.
    GetAddrOff(addrStart) &= ~0xf;

#ifdef OSDEBUG4
    xosd = ReadBuffer(hpid, htid, &addrStart, MAXL, rgb, &cb);
    if (xosd != xosdNone) {
        cb = 0;
    }
#else
    EMFunc ( emfSetAddr, hpid, htid, adrCurrent, (LONG) &addrStart );
    cb = EMFunc ( emfReadBuf, hpid, htid, MAXL, (LONG) (LPV) rgb );
#endif

    if ( cb <= 0 ) {

        _fmemcpy ( rgchRaw, " ??", 4 );
        _fmemcpy ( rgchOpcode, "???", 4 );
        switch (GetAddrOff(lpsdi->addr) & 0xF)
        {
            case 0:
            case 4:
                lpsdi->addr.addr.off += 4;
                break;
            case 8:
                lpsdi->addr.addr.off += 8;
                break;
            default:
                lpsdi->addr.addr.off += 1; //v-vadimp - bogus should assert
                break;
        }

    }
    else {

        CalcMain (
            hpid,
            htid,
            lpsdi->dop,
            lpsdi->addr,
            rgb,
            cb,
            &cbUsed,
            rgchPreg, sizeof(rgchPreg),
            rgchOpcode, sizeof(rgchOpcode),
            rgchOperands, sizeof(rgchOperands),
            rgchComment, sizeof(rgchComment)
        );

    if ( GetAddrOff(lpsdi->addr) > (DWORDLONG)(0xFFFFFFFFFFFFFFFF - cbUsed) ) {
            return xosdBadAddress;
    }

    if ( dop & dopRaw ) {
        LPCH lpchT = rgchRaw;
        if ((GetAddrOff(lpsdi->addr) & 0xF) == 0) {
                OutputHexCode ( &lpchT, (LPCH)rgb, 16); // rgb contains complete binary bundle
                //OutputHexCode ( &lpchT, (LPCH)rgb+4, cbUsed );
                //OutputHexCode ( &lpchT, (LPCH)rgb+8, cbUsed);
				*lpchT = '\0';
		} else {
				DumpGeneric ( "                                 ", lpchT, 33 );
		}
		}
	}

    if ( ( dop & dopRaw ) && ( cchMax > 0 ) ) {
        cch = DumpGeneric ( rgchRaw, lpchOut, cchMax );

        lpsdi->ichBytes = ichCur;
        cchMax         -= cch;
        lpchOut        += cch;
        ichCur         += cch;
    }

    if ( ( dop & dopOperands ) && ( cchMax > 0 ) && ( rgchPreg [ 0 ] != '\0' ) ) {
        cch = DumpGeneric ( rgchPreg, lpchOut, cchMax );

        lpsdi->ichPreg  = ichCur;
        cchMax         -= cch;
        lpchOut        += cch;
        ichCur         += cch;
    }

    if ( ( dop & dopOpcode ) && ( cchMax > 0 ) ) {
        cch = DumpGeneric ( rgchOpcode, lpchOut, cchMax );

        lpsdi->ichOpcode = ichCur;
        cchMax          -= cch;
        lpchOut         += cch;
        ichCur          += cch;
    }

    if ( ( dop & dopOperands ) && ( cchMax > 0 ) && ( rgchOperands [ 0 ] != '\0' ) ) {
        cch = DumpGeneric ( rgchOperands, lpchOut, cchMax );

        lpsdi->ichOperands = ichCur;
        cchMax            -= cch;
        lpchOut           += cch;
        ichCur            += cch;
    }

    if ( ( dop & dopOperands ) && ( cchMax > 0 ) && ( rgchComment [ 0 ] != '\0' ) ) {
        cch = DumpComment ( rgchComment, lpchOut, cchMax );

        lpsdi->ichComment  = ichCur;
        cchMax            -= cch;
        lpchOut           += cch;
        ichCur            += cch;
    }

    if ( dop & dopEA ) {
        cch = DumpEA ( hpid, htid, &lpsdi->addrEA0, lpchOut, cchMax );

        if ( cchMax > 0 && cch > 0 ) {
            lpsdi->ichEA0      = ichCur;
            cchMax            -= cch;
            lpchOut           += cch;
            ichCur            += cch;
        }
    }

    GetAddrOff(lpsdi->addr) += cbUsed;

    return xosd;
}



/*** GetSymbolExportRoutine - get symbol name from offset specified
*
* Purpose:
*       Wrapper for Client_gen_sym().
*       With the specified offset, return the nearest symbol string
*       previous or equal to the offset and its displacement
*
* Input:
*       offset - input offset to search
*
* Output:
*       pchBuffer - pointer to buffer to fill with string
*       pDisplacement - pointer to offset displacement
*
* Notes:
*       if offset if less than any defined symbol, the NULL value
*       is returned and the displacement is set to the offset
*
*************************************************************************/

EM_Dis_Err WIN_CDECL GetSymbolExportRoutine (const U64 offsetU64,
                                char * pchBuffer,
                                unsigned int * sym_buf_length,
                                U64 * pDisplacement)
{
    char * pchBuf = pchBuffer;
    UOFFSET offset = IEL_GETDW0(offsetU64);

    LPCH    lpchSymbol;
    ADDR    addrT={0}, addr={0};
    ODR     odr;

    if ( pchBuffer == NULL || sym_buf_length == NULL || pDisplacement == NULL )
        return EM_DIS_NULL_PTR;

    IEL_ZERO(*pDisplacement);
    odr.lszName = pchBuf;

    GetAddrOff(addr) = GetAddrOff(addrT) = offset;
    MODE_IS_FLAT(modeAddr(addr)) = TRUE;
    MODE_IS_FLAT(modeAddr(addrT)) = TRUE;

    lpchSymbol = SHGetSymbol (&addrT, &addr, sopNone, &odr);

    if (odr.dwDeltaOff == -1) {        // symbol not found
                    //write offset in hex to symbol buffer instead.
        if (fUpper)
            sprintf(pchBuf, "%8lXH", offset);
        else
            sprintf(pchBuf, "%8lxh", offset);
        *sym_buf_length = strlen(pchBuffer);
        return EM_DIS_NO_SYMBOL;
    }
    else {                          // Symbol found
        // Convert ULONG Disp back to Struct U64 Displacement

        IEL_ASSIGNU(*pDisplacement, IEL64( odr.dwDeltaOff ));
        *sym_buf_length = strlen(pchBuffer);
        return EM_DIS_NO_ERROR;
    }
}



/*++

Routine Description:

    Main disassembler routine based on Falcon EM disassembler library (DISEM.DLL)

Arguments:

    hpid        - Supplies the process handle
    hthd        - Supplies a thread handle
    dop         - Supplies the set of disassembly options
    lpaddr      - Supplies the address to be disassembled
    rgb         - Supplies the buffer to dissassemble into
    cbMax       - Supplies the size of rgb (Unused)
    lpcbUsed    - Returns the acutal size used (Unused)
    rgchPreg    - Supplies location to place qualifying predicate register
    rgchOpcode  - Supplies location to place opcode
    rgchOperands- Supplies location to place operands
    rgchComment - Supplies location to place comment

Return Value:

    None.

--*/

void
CalcMain (
          HPID     hpid,
          HTID     htid,
          DOP      dop,
          ADDR     addr,
          LPBYTE   rgb,
          int      cbMax,
          int FAR *lpcbUsed,
          LPCH     rgchPreg,
          int      cchPreg,
          LPCH     rgchOpcode,
          int      cchOpcode,
          LPCH     rgchOperands,
          int      cchOperands,
          LPCH     rgchComment,
          int      cchComment
          )

{
    UOFFSET     offset = GetAddrOff(addr);
    BOOL        fEAout  = TRUE;
    UOFFSET     gb_offset;

    static    EM_Decoder_Machine_Type    machineType = EM_DECODER_CPU_P7;
    static    EM_Decoder_Machine_Mode machineMode = EM_DECODER_MODE_EM;
    static    unsigned long            printFlags;
    static    EM_Dis_Radix            disRadix     = EM_DIS_RADIX_HEX;
    static    EM_Dis_Style            disStyle     = EM_DIS_STYLE_MASM;

    U64     location;
    UINT    actual_inst_length;
    CHAR    ascii_inst_buf[120];
    UINT    ascii_inst_buf_length;
    UINT    bin_inst_buf_length = sizeof(U128);
    EM_Dis_Field    dis_fields[120];
    INT     i;

    EM_Dis_Err    err;

try_loading:
    if (hFalcon == NULL) {
        hFalcon = LoadLibrary(DECODER_NAME);

        if( hFalcon == NULL) {
			switch (MessageBox(NULL, "IA64 Disassembler DLL " DECODER_NAME " could not be loaded. Press OK to exit windbg.", NULL, MB_ABORTRETRYIGNORE)) {
				case IDOK:
					exit(0);
					break;

				case IDIGNORE:
					return;
					break;

				case IDRETRY:
					goto try_loading;
					break;
			}
        }

        pfn_em_dis_setup = (PPFN_EM_DIS_SETUP)GetProcAddress( hFalcon, "em_dis_setup" );
        pfn_em_dis_inst = (PPFN_EM_DIS_INST)GetProcAddress( hFalcon, "em_dis_inst" );
        pfn_em_dis_err_str = (PPFN_EM_DIS_ERR_STR) GetProcAddress( hFalcon, "em_dis_err_str" );
        if (pfn_em_dis_setup == NULL || pfn_em_dis_inst == NULL || pfn_em_dis_err_str == NULL) {
            FreeLibrary(hFalcon);
			switch (MessageBox(NULL, "Disassembler routines could be loaded from not IA64 Disassembler DLL " DECODER_NAME ". Press OK to exit windbg.", NULL, MB_ABORTRETRYIGNORE)) {
				case IDOK:
					exit(0);
					break;

				case IDIGNORE:
					return;
					break;

				case IDRETRY:
					goto try_loading;
					break;
			}
        }
    }

    Unreferenced(cbMax);

    *rgchComment = *rgchOperands = *rgchPreg = 0;
    EAsize[0] = EAsize[1] = 0;

    PBuf = rgchComment;

    /* re-run dis_setup if baseDefault has been changed since last initialization */

        if ((( baseDefault == 2 ) && ( disRadix != EM_DIS_RADIX_BINARY ))   ||
            (( baseDefault == 8 ) && ( disRadix != EM_DIS_RADIX_OCTAL ))    ||
            (( baseDefault == 10 ) && ( disRadix != EM_DIS_RADIX_DECIMAL )) ||
            (( baseDefault == 16 ) && ( disRadix != EM_DIS_RADIX_HEX ))     ||
            ((dop & dopSym) && (printFlags != (EM_DIS_FLAG_DEFAULT | EM_DIS_FLAG_EXCLUDE_ADDRESS))))
		{

             printFlags = EM_DIS_FLAG_DEFAULT | EM_DIS_FLAG_EXCLUDE_ADDRESS;
            if (!(dop & dopSym))
                printFlags |= EM_DIS_FLAG_EXCLUDE_ADDRESS_CALLBACK;

            if ((err = (*pfn_em_dis_setup)( machineType,machineMode,printFlags,disRadix,disStyle,NULL/*GetSymbolExportRoutine*/))!= EM_DIS_NO_ERROR)
			{
                 sprintf(PBuf,"em_dis_setup: %s\n", (*pfn_em_dis_err_str)((EM_Dis_Err) err));
                return;
            }
        }

        IEL_ZERO(location);
        // convert EM address to Gambit internal address. i.e., move slot number from bit(2,3) to bit(0,1)
        gb_offset = (offset & ~0xf) | ((offset & 0xf) >> 2);
        IEL_ASSIGNU(location, IEL64(gb_offset));

        if ((err = (*pfn_em_dis_inst)(&location,machineMode,rgb,bin_inst_buf_length,&actual_inst_length,ascii_inst_buf,&ascii_inst_buf_length,(EM_Dis_Fields *)&dis_fields)) != EM_DIS_NO_ERROR )
		{
            sprintf(PBuf,"Falcon error %s at location %I64x", 
                    (*pfn_em_dis_err_str)((EM_Dis_Err) err),
                    offset
                    );
            EM_DIS_EM_NEXT(location, 3);         // IA64 EAS2.1 SlotSize=3, SepBr=0    */

            gb_offset = IEL_GETDW0(location);
            // convert Gambit internal address to EM address
            gb_offset =  (gb_offset & (~0xf)) | ((gb_offset & 0xf) << 2);
            *lpcbUsed = (int)(gb_offset - offset);
               return;
            }
        /*
         * determine the offset(cbUsed) to the next instruction
         */

        // fixup offset to next syllable/instruction
        EM_DIS_EM_NEXT(location, 3);         // IA64 EAS2.1 SlotSize=3, SepBr=0    */

        i = 0;
		while (dis_fields[i].type != EM_DIS_FIELD_NONE)
		{
            if (dis_fields[i].type == EM_DIS_FIELD_MNEM && strncmp(dis_fields[i].first, "movl", 4) == 0)
			{
                          EM_DIS_EM_NEXT(location, 3);         // movl instruction takes two slots
            }
            i++;
        }

        gb_offset = IEL_GETDW0(location);
        // convert Gambit internal address to EM address
        gb_offset =  (gb_offset & ~0xf) | ((gb_offset & 0xf) << 2);
        *lpcbUsed = (int)(gb_offset - offset);

        assert(*lpcbUsed <= 16); //v-vadimp - bundle size

        /*
         * write disassembly output to buffers
         */

        // em_dis_inst() places raw disassembled ascii output in asci_inst_buf[] and structured output in dis_fields[]
        // To comply with SDI format(see osdebug\include\od.h), we have to convert dis_fields output into four
        // SDI fields: ichPreg(new for IA64 ONLY), ichOpcode, ichOperands, and ichComment.

        // search for bundle start and qualifying predicate, skip search if mnemonics is found

        PBuf = rgchPreg;
        strncpy(PBuf, " ", 1);    // force predicate field to be non-NULL. Otherwise, opcode field will not line up.
        i = 0;

        while(dis_fields[i].type != EM_DIS_FIELD_NONE)
        {
            switch(dis_fields[i].type)
            {
                case EM_DIS_FIELD_ADDR_SYM:
                case EM_DIS_FIELD_ADDR_PLUS:
                case EM_DIS_FIELD_ADDR_OFFSET:
                case EM_DIS_FIELD_ADDR_COLON:
                    i++;        // skip address symbol fields
                    break;

                case EM_DIS_FIELD_BUNDLE_START:
                case EM_DIS_FIELD_OPER_LPARENT:
                case EM_DIS_FIELD_PREG_LPARENT:
                case EM_DIS_FIELD_PREG_REG:
                case EM_DIS_FIELD_OPER_RPARENT:
                case EM_DIS_FIELD_PREG_RPARENT:
                    // these field types should occur before MNEM. PBuf is initialized to rgchPreg.
                      strncpy(PBuf, dis_fields[i].first, dis_fields[i].length);
                      PBuf += dis_fields[i].length;
                    i++;
                    break;

                case EM_DIS_FIELD_MNEM:
                    PBuf = rgchOpcode;            // skip preg search if mnemonic is found
                      strncpy(PBuf, dis_fields[i].first, dis_fields[i].length);
                      PBuf += dis_fields[i].length;

                    /* If option is set, convert operator to upper case */
                    if ( fUpper )
                            for(PBuf = rgchOpcode; *PBuf != '\0'; PBuf++)
                                    *PBuf = (CHAR)toupper(*PBuf);

                    PBuf = rgchOperands;        // assume everything after mnemonics are operands
                    i++;
                    break;


                case EM_DIS_FIELD_BUNDLE_END:
                    PBuf += sprintf(PBuf, "     ");
                default:
                      strncpy(PBuf, dis_fields[i].first, dis_fields[i].length);
                      PBuf += dis_fields[i].length;
                    i++;
                    break;
            }
        }

        if (*rgchOpcode == 0) {
                PBuf = rgchOpcode;
                PBuf += sprintf(PBuf, "\tUnknown opcode or extended opcode");
        }
        return;
}



int DumpAddress ( LPADDR lpaddr, LPCH lpch, int cchMax ) {
    LPCH lpchT = lpch;

    // v-vadimp - Don't try to display only bundle addresses, windbg assumes the first
    // characters in disasm contain an address that's used to set BPs in disasm
    Unreferenced(cchMax);
	OutputAddr ( &lpch, lpaddr, (ADDR_IS_FLAT(*lpaddr) + 1) * 4 );
    return (int)(lpch - lpchT + 1);
}


int
DumpGeneric (
    LSZ lsz,
    LPCH lpch,
    int cchMax
    )
{
    int ich = 0;

    while ( *(lsz + ich) != 0 && ich < cchMax - 1 ) {
        *(lpch+ich) = *(lsz+ich);
        ich += 1;
    }
    *( lpch + ich ) = '\0';

    return ich + 1;
}


int
DumpComment (
    LSZ lsz,
    LPCH lpch,
    int cchMax
    )
{
    *(lpch) = ';';
    return DumpGeneric ( lsz, lpch + 1, cchMax - 1 ) + 1;
}

int
DumpEA (
    HPID hpid,
    HTID htid,
    LPADDR lpaddr,
    LPCH lpch,
    int cchMax
    )
{
    LPCH lpchT = lpch;
    BYTE rgb [ MAXL ];
    int  indx;
    DWORD cb;
#ifdef OSDEBUG4
    XOSD xosd;
#endif

    Unreferenced(cchMax);

    for ( indx = 0; indx < 2; indx++ ) {

        if ( EAsize [ indx ] ) {
            ADDR addr = {0};

            OutputHexString ( &lpchT, (LPCH) &EAaddr [ indx ], 4 );

            *lpchT++ = '=';

            addr.addr.off = (UOFFSET) EAaddr [ indx ];
            addr.addr.seg = (SEGMENT) 0;

            *lpaddr = addr;

#ifdef OSDEBUG4
            xosd = ReadBuffer(hpid, htid, &addr, EAsize[indx], rgb, &cb);
            if (xosd != xosdNone) {
                cb = 0;
            }
#else
            EMFunc (
                emfSetAddr,
                hpid,
                htid,
                adrCurrent,
                (LONG) (LPADDR) &addr
            );

            cb = EMFunc (
                emfReadBuf,
                hpid,
                htid,
                EAsize [ indx ],
                (LONG) (LPV) rgb
            );
#endif

            if ( cb == (DWORD)EAsize [ indx ] ) {
                OutputHexString ( &lpchT, (LPCH) rgb, EAsize [ indx ] );
            }
            else {
                while ( EAsize [ indx ]-- ) {
                    *lpchT++ = '?';
                    *lpchT++ = '?';
                }
            }
            *lpchT++ = '\0';
        }
    }

    return (int)(lpchT - lpch);
}



/*** OutputHexString - output hex string
*
*   Purpose:
*   Output the value pointed by *lplpchMemBuf of the specified
*   length.  The value is treated as unsigned and leading
*   zeroes are printed.
*
*   Input:
*   *lplpchBuf - pointer to text buffer to fill
*   *pchValue - pointer to memory buffer to extract value
*   length - length in bytes of value
*
*   Output:
*   *lplpchBuf - pointer updated to next text character
*   *lplpchMemBuf - pointer update to next memory byte
*
*************************************************************************/

void OutputHexString (LPLPCH lplpchBuf, PCH pchValue, int length)
{
    unsigned char    chMem;

    pchValue += length;
    while ( length-- ) {
        chMem = *--pchValue;
        *(*lplpchBuf)++ = hexdigit[chMem >> 4];
        *(*lplpchBuf)++ = hexdigit[chMem & 0x0f];
    }
}


/*** OutputAddr - output address package
*
*   Purpose:
*   Output the address pointed to by lpaddr.
*
*   Input:
*   *lplpchBuf - pointer to text buffer to fill
*   lpaddr - Standard address package.
*
*   Output:
*   *lplpchBuf - pointer updated to next text character
*
*************************************************************************/

VOID
OutputAddr (
    LPLPCH lplpchBuf,
    LPADDR lpaddr,
    int alen
    )
{
    ADDR addr = *lpaddr;

    *(*lplpchBuf)++ = '0';
    *(*lplpchBuf)++ = (fUpper) ? 'X' : 'x';
    OutputHexString ( lplpchBuf, (LPCH) &addr.addr.off, alen );

    return;
}                               /* OutputAddr() */


/*** OutputHexCode - output hex code
*
*   Purpose:
*   Output the code pointed by lpchMemBuf of the specified
*   length.  The value is treated as unsigned and leading
*   zeroes are printed.  This differs from OutputHexString
*   in that bytes are printed from low to high addresses.
*
*   Input:
*   *lplpchBuf - pointer to text buffer to fill
*   lpchMemBuf -  - pointer to memory buffer to extract value
*   length - length in bytes of value
*
*   Output:
*   *lplpchBuf - pointer updated to next text character
*
*************************************************************************/

void OutputHexCode (LPLPCH lplpchBuf, LPCH lpchMemBuf, int length)
{
    unsigned char    chMem;

    while (length--) {
        chMem = lpchMemBuf[length];
        *(*lplpchBuf)++ = hexdigit[chMem >> 4];
        *(*lplpchBuf)++ = hexdigit[chMem & 0x0f];
    }
}




void OutputHex (ULONG outvalue, ULONG length, BOOL fSigned)
{
    UCHAR   digit[8];
    LONG    index = 0;

    if (fSigned && (long)outvalue < 0) {
        if (CchBuf > 1) {
            *PBuf++ = '-';
            CchBuf -= 1;
        }
        outvalue = (ULONG) (- (LONG) outvalue);
    }

    if (CchBuf > 2) {
        *PBuf++ = '0';
        *PBuf++ = (fUpper) ? 'X' :'x';
        CchBuf -= 2;
    }

    do {
        digit[index++] = hexdigit[outvalue & 0xf];
        outvalue >>= 4;
    }
    while (outvalue || (!fSigned && index < (LONG)length));

    if (CchBuf > index) {
        CchBuf -= index;
        while (--index >= 0) {
            *PBuf++ = digit[index];
        }
    }
    return;
}

extern "C" void  CleanupDisassembler(void)
{ }

extern "C" int WINAPI SimplyDisassemble(
    PBYTE           pb,
    const size_t    cbMax,
    const DWORD64   Address,
    const int       Architecture,
    PVOID Sdis,
    PVOID pfnCchAddr,
    PVOID pfnCchFixup,
    PVOID pfnCchRegrel,
    PVOID pfnQwGetreg,
    const PVOID     pv
    )
{
    assert(!"Should never be called");
    return 0;
}

#endif // defined(_IA64_)
