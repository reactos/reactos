/******************************************************************************
*                                                                             *
*   Module:     disassemblerdefines.h                                         *
*                                                                             *
*   Revision:   1.00                                                          *
*                                                                             *
*   Date:       3/17/2000                                                     *
*                                                                             *
*   Copyright (c) 2000 Goran Devic                                            *
*                                                                             *
*   Author:     Goran Devic                                                   *
*                                                                             *
*******************************************************************************

    Module Description:

        This is a header file containing the disassembler defines that are
        used in DisassemblerData.h

*******************************************************************************
*                                                                             *
*   Changes:                                                                  *
*                                                                             *
*   DATE     DESCRIPTION OF CHANGES                               AUTHOR      *
* --------   ---------------------------------------------------  ----------- *
* 4/28/2000  Original                                             Goran Devic *
* 11/4/2000  Modified for LinIce                                  Goran Devic *
* --------   ---------------------------------------------------  ----------- *
*******************************************************************************
*   Important Defines                                                         *
******************************************************************************/
#ifndef _DDEF_H_
#define _DDEF_H_

/******************************************************************************
*
*   Groups and special codes in place of name index
*
******************************************************************************/
#define _NDEF          0x00     // Udefined/reserved opcode
#define _2BESC         0x01     // 2 byte escape code
#define _S_ES          0x02     // Segment ES override | these defines
#define _S_CS          0x03     // Segment CS override | must have
#define _S_SS          0x04     // Segment SS override | consecutive
#define _S_DS          0x05     // Segment DS override | enumeration
#define _S_FS          0x06     // Segment FS override | numbers.
#define _S_GS          0x07     // Segment GS override |
#define _OPSIZ         0x08     // Operand size override
#define _ADSIZ         0x09     // Address size override
#define _REPNE         0x0A     // REPNE/REPNZ prefix
#define _REP           0x0B     // REP/REPE/REPZ prefix
#define _EscD8         0x0C     // Escape to coprocessor set: prefix D8
#define _EscD9         0x0D     // Escape to coprocessor set: prefix D9
#define _EscDA         0x0E     // Escape to coprocessor set: prefix DA
#define _EscDB         0x0F     // Escape to coprocessor set: prefix DB
#define _EscDC         0x10     // Escape to coprocessor set: prefix DC
#define _EscDD         0x11     // Escape to coprocessor set: prefix DD
#define _EscDE         0x12     // Escape to coprocessor set: prefix DE
#define _EscDF         0x13     // Escape to coprocessor set: prefix DF
#define _GRP1a         0x14     // Group 1a extended opcode
#define _GRP1b         0x15     // Group 1b extended opcode
#define _GRP1c         0x16     // Group 1c extended opcode
#define _GRP2a         0x17     // Group 2a extended opcode
#define _GRP2b         0x18     // Group 2b extended opcode
#define _GRP2c         0x19     // Group 2c extended opcode
#define _GRP2d         0x1A     // Group 2d extended opcode
#define _GRP2e         0x1B     // Group 2e extended opcode
#define _GRP2f         0x1C     // Group 2f extended opcode
#define _GRP3a         0x1D     // Group 3a extended opcode
#define _GRP3b         0x1E     // Group 3b extended opcode
#define _GRP4          0x1F     // Group 4 extended opcode
#define _GRP5          0x20     // Group 5 extended opcode
#define _GRP6          0x21     // Group 6 extended opcode
#define _GRP7          0x22     // Group 7 extended opcode
#define _GRP8          0x23     // Group 8 extended opcode
#define _GRP9          0x24     // Group 9 extended opcode

/******************************************************************************
*
*   Addressing modes argument definiton for the opcodes in a table
*
******************************************************************************/
#define _O             0x01

#define _Ib            0x03
#define _Iv            0x04
#define _Iw            0x05
#define _Yb            0x06
#define _Yv            0x07
#define _Xb            0x08
#define _Xv            0x09
#define _Jb            0x0A
#define _Jv            0x0B
#define _Ap            0x0C
#define _1             0x10
#define _3             0x11
#define _DX            0x12
#define _AL            0x13
#define _AH            0x14
#define _BL            0x15
#define _BH            0x16
#define _CL            0x17
#define _CH            0x18
#define _DL            0x19
#define _DH            0x1A
#define _CS            0x1B
#define _DS            0x1C
#define _ES            0x1D
#define _SS            0x1E
#define _FS            0x1F
#define _GS            0x20
#define _eAX           0x21
#define _eCX           0x22
#define _eDX           0x23
#define _eBX           0x24
#define _eSP           0x25
#define _eBP           0x26
#define _eSI           0x27
#define _eDI           0x28
#define _Eb            0x2F
#define _Ev            0x30
#define _Ew            0x31
#define _Ep            0x32
#define _Gb            0x33
#define _Gv            0x34
#define _M             0x35
#define _Ma            0x36
#define _Mp            0x37
#define _Ms            0x38
#define _Mq            0x39
#define _Rd            0x3A
#define _Rw            0x3B
#define _Sw            0x3C
#define _Cd            0x3D
#define _Dd            0x3E
#define _Td            0x3F
#define _ST            0x40
#define _ST0           0x41
#define _ST1           0x42
#define _ST2           0x43
#define _ST3           0x44
#define _ST4           0x45
#define _ST5           0x46
#define _ST6           0x47
#define _ST7           0x48
#define _AX            0x49

/******************************************************************************
*
*   Define holding structure for opcode
*
******************************************************************************/

typedef struct
{
    UCHAR    name;               // Index into the opcode name table
    UCHAR    args;               // Number of addressing codes that follow
    UCHAR    dest;               // Destination operand addressing code
    UCHAR    src;                // Source operand addressing code
    UCHAR    thrid;              // Third operand addressing code
    UCHAR    v_instruction;      // Virtual instruction index
    UCHAR    access;             // Instruction data access type
    UCHAR    flags;              // Miscellaneous flags
} TOpcodeData;

// `access' field:
// Data access flags are used with memory access instructions

#define INSTR_READ                  0x80      // Faulting instruction reads memory
#define INSTR_WRITE                 0x40      // Faulting instruction writes to memory
#define INSTR_READ_WRITE            0x20      // Faulting instruction is read-modify-write

// Low nibble contains the data length code - do not change these values as
// they represent the data width value as well

#define INSTR_BYTE                  0x01      // Byte access instruction
#define INSTR_WORD                  0x02      // Word access instruction
#define INSTR_WORD_DWORD            0x03      // Word or dword, depending on operand size
#define INSTR_DWORD                 0x04      // Dword access instruction

// `flags' field:
// Disassembler flags; bottom 4 bits are used by the scanner flags

#define DIS_SPECIAL                 0x80      // Special opcode
#define DIS_NAME_FLAG               0x40      // Name changes
#define   DIS_GETNAMEFLAG(flags)    (((flags)>>6)&1)
#define DIS_COPROC                  0x20      // Coprocessor instruction
#define DIS_MODRM                   0x10      // Use additional Mod R/M byte

// Scanner enums: 4 bits wide

#define SCAN_NATIVE                 0x0     // Native instruction are default 0
#define SCAN_JUMP                   0x1     // Evaluate new path
#define SCAN_COND_JUMP              0x2     // Evaluate both paths
#define SCAN_TERMINATING            0x3     // Terminating instruction needs virtualization
#define SCAN_TERM_PMODE             0x4     // Terminating instruction in protected mode only
#define SCAN_SINGLE_STEP            0x5     // Single-step instruction

// Define values stored in meta pages (bits [7:4])

#define META_NATIVE                 0x0     // Native instruction are default 0
#define META_UNDEF                  0x1     // Undefined/illegal instruction
#define META_TERMINATING            0x2     // Terminating instruction
#define META_SINGLE_STEP            0x3     // Execute natively single step

/******************************************************************************
*                                                                             *
*   Define opcode values for the main table                                   *
*                                                                             *
******************************************************************************/
#define _aaa           0x001
#define _aad           0x002
#define _aam           0x003
#define _aas           0x004
#define _adc           0x005
#define _add           0x006
#define _and           0x007
#define _arpl          0x008
#define _bound         0x009
#define _bsf           0x00a
#define _bsr           0x00b
#define _bt            0x00c
#define _btc           0x00d
#define _btr           0x00e
#define _bts           0x00f
#define _call          0x010
#define _cbw           0x011
#define _cwde          0x012
#define _clc           0x013
#define _cld           0x014
#define _cli           0x015
#define _clts          0x016
#define _cmc           0x017
#define _cmp           0x018
#define _cmps          0x019
#define _cmpsb         0x01a
#define _cmpsw         0x01b
#define _cmpsd         0x01c
#define _cwd           0x01d
#define _cdq           0x01e
#define _daa           0x01f
#define _das           0x020
#define _dec           0x021
#define _div           0x022
#define _enter         0x023
#define _hlt           0x024
#define _idiv          0x025
#define _imul          0x026
#define _in            0x027
#define _inc           0x028
#define _ins           0x029
#define _insb          0x02a
#define _insw          0x02b
#define _insd          0x02c
#define _int           0x02d
#define _into          0x02e
#define _iret          0x02f
#define _iretd         0x030
#define _jo            0x031
#define _jno           0x032
#define _jb            0x033
#define _jnb           0x034
#define _jz            0x035
#define _jnz           0x036
#define _jbe           0x037
#define _jnbe          0x038
#define _js            0x039
#define _jns           0x03a
#define _jp            0x03b
#define _jnp           0x03c
#define _jl            0x03d
#define _jnl           0x03e
#define _jle           0x03f
#define _jnle          0x040
#define _jmp           0x041
#define _lahf          0x042
#define _lar           0x043
#define _lea           0x044
#define _leave         0x045
#define _lgdt          0x046
#define _lidt          0x047
#define _lgs           0x048
#define _lss           0x049
#define _lds           0x04a
#define _les           0x04b
#define _lfs           0x04c
#define _lldt          0x04d
#define _lmsw          0x04e
#define _lock          0x04f
#define _lods          0x050
#define _lodsb         0x051
#define _lodsw         0x052
#define _lodsd         0x053
#define _loop          0x054
#define _loope         0x055
#define _loopz         0x056
#define _loopne        0x057
#define _loopnz        0x058
#define _lsl           0x059
#define _ltr           0x05a
#define _mov           0x05b
#define _movs          0x05c
#define _movsb         0x05d
#define _movsw         0x05e
#define _movsd         0x05f
#define _movsx         0x060
#define _movzx         0x061
#define _mul           0x062
#define _neg           0x063
#define _nop           0x064
#define _not           0x065
#define _or            0x066
#define _out           0x067
#define _outs          0x068
#define _outsb         0x069
#define _outsw         0x06a
#define _outsd         0x06b
#define _pop           0x06c
#define _popa          0x06d
#define _popad         0x06e
#define _popf          0x06f
#define _popfd         0x070
#define _push          0x071
#define _pusha         0x072
#define _pushad        0x073
#define _pushf         0x074
#define _pushfd        0x075
#define _rcl           0x076
#define _rcr           0x077
#define _rol           0x078
#define _ror           0x079
#define _rep           0x07a
#define _repe          0x07b
#define _repz          0x07c
#define _repne         0x07d
#define _repnz         0x07e
#define _ret           0x07f
#define _sahf          0x080
#define _sal           0x081
#define _sar           0x082
#define _shl           0x083
#define _shr           0x084
#define _sbb           0x085
#define _scas          0x086
#define _scasb         0x087
#define _scasw         0x088
#define _scasd         0x089
#define _set           0x08a
#define _sgdt          0x08b
#define _sidt          0x08c
#define _shld          0x08d
#define _shrd          0x08e
#define _sldt          0x08f
#define _smsw          0x090
#define _stc           0x091
#define _std           0x092
#define _sti           0x093
#define _stos          0x094
#define _stosb         0x095
#define _stosw         0x096
#define _stosd         0x097
#define _str           0x098
#define _sub           0x099
#define _test          0x09a
#define _verr          0x09b
#define _verw          0x09c
#define _wait          0x09d
#define _xchg          0x09e
#define _xlat          0x09f
#define _xlatb         0x0a0
#define _xor           0x0a1
#define _jcxz          0x0a2
#define _loadall       0x0a3
#define _invd          0x0a4
#define _wbinv         0x0a5
#define _seto          0x0a6
#define _setno         0x0a7
#define _setb          0x0a8
#define _setnb         0x0a9
#define _setz          0x0aa
#define _setnz         0x0ab
#define _setbe         0x0ac
#define _setnbe        0x0ad
#define _sets          0x0ae
#define _setns         0x0af
#define _setp          0x0b0
#define _setnp         0x0b1
#define _setl          0x0b2
#define _setnl         0x0b3
#define _setle         0x0b4
#define _setnle        0x0b5
#define _wrmsr         0x0b6
#define _rdtsc         0x0b7
#define _rdmsr         0x0b8
#define _cpuid         0x0b9
#define _rsm           0x0ba
#define _cmpx          0x0bb
#define _xadd          0x0bc
#define _bswap         0x0bd
#define _invpg         0x0be
#define _cmpx8         0x0bf
#define _jmpf          0x0c0
#define _retf          0x0c1
#define _rdpmc         0x0c2

#define _f2xm1         0x001
#define _fabs          0x002
#define _fadd          0x003
#define _faddp         0x004
#define _fbld          0x005
#define _fbstp         0x006
#define _fchs          0x007
#define _fclex         0x008
#define _fcom          0x009
#define _fcomp         0x00a
#define _fcompp        0x00b
#define _fcos          0x00c
#define _fdecstp       0x00d
#define _fdiv          0x00e
#define _fdivp         0x00f
#define _fdivr         0x010
#define _fdivrp        0x011
#define _ffree         0x012
#define _fiadd         0x013
#define _ficom         0x014
#define _ficomp        0x015
#define _fidiv         0x016
#define _fidivr        0x017
#define _fild          0x018
#define _fimul         0x019
#define _fincstp       0x01a
#define _finit         0x01b
#define _fist          0x01c
#define _fistp         0x01d
#define _fisub         0x01e
#define _fisubr        0x01f
#define _fld           0x020
#define _fld1          0x021
#define _fldcw         0x022
#define _fldenv        0x023
#define _fldl2e        0x024
#define _fldl2t        0x025
#define _fldlg2        0x026
#define _fldln2        0x027
#define _fldpi         0x028
#define _fldz          0x029
#define _fmul          0x02a
#define _fmulp         0x02b
#define _fnop          0x02c
#define _fpatan        0x02d
#define _fprem         0x02e
#define _fprem1        0x02f
#define _fptan         0x030
#define _frndint       0x031
#define _frstor        0x032
#define _fsave         0x033
#define _fscale        0x034
#define _fsin          0x035
#define _fsincos       0x036
#define _fsqrt         0x037
#define _fst           0x038
#define _fstcw         0x039
#define _fstenv        0x03a
#define _fstp          0x03b
#define _fstsw         0x03c
#define _fsub          0x03d
#define _fsubp         0x03e
#define _fsubr         0x03f
#define _fsubrp        0x040
#define _ftst          0x041
#define _fucom         0x042
#define _fucomp        0x043
#define _fucompp       0x044
#define _fxam          0x045
#define _fxch          0x046
#define _fxtract       0x047
#define _fyl2x         0x048
#define _fyl2xp1       0x049

/******************************************************************************
*
*   External data and strings
*
******************************************************************************/
extern char* sNames[];
extern char* sCoprocNames[];
extern TOpcodeData Op1[ 256 ];
extern TOpcodeData Op2[ 256 ];
extern TOpcodeData Groups[ 17 ][ 8 ];
extern TOpcodeData Coproc1[ 8 ][ 8 ];
extern TOpcodeData Coproc2[ 8 ][ 16 * 4 ];
extern char *sBytePtr;
extern char *sWordPtr;
extern char *sDwordPtr;
extern char *sFwordPtr;
extern char *sQwordPtr;
extern char *sGenReg16_32[ 2 ][ 8 ];
extern char *sSeg[ 8 ];
extern char *sSegOverride[ 8 ];
extern char *sSegOverrideDefaultES[ 8 ];
extern char *sSegOverrideDefaultDS[ 8 ];
extern char *sScale[ 4 ];
extern char *sAdr1[ 2 ][ 8 ];
extern char *sRegs1[ 2 ][ 2 ][ 8 ];
extern char *sRegs2[];
extern char *sControl[ 8 ];
extern char *sDebug[ 8 ];
extern char *sTest[ 8 ];
extern char *sYptr[ 2 ];
extern char *sXptr[ 2 ];
extern char *sRep[ 4 ];
extern char *sST[ 9 ];


#endif // _DDEF_H_
