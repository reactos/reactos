 /* Operand and instruction types */
#define OP_REG			0x100		/* register */
#define OP_IMM			0x200		/* immediate value */
#define OP_IND			0x300		/* indirect memory reference */
#define OP_BPTR		0x400		/* BYTE Pointer */
#define OP_WPTR		0x500		/* WORD Pointer */
#define OP_DPTR		0x600		/* DWORD Pointer */
#define OP_UNK			0x900		
//#define INS_INVALID	0x00	/* Not a valid instruction */
   /* Branch Instruction types */
#define INS_BRANCH	0x01	/* Unconditional branch */
#define INS_COND		0x02	/* Conditional branch */
#define INS_SUB		0x04	/* Jump to subroutine */
#define INS_RET		0x08	/* Return from subroutine */
   /* modify ( 'w' ) instructions */
#define INS_ARITH 	0x10 /* Arithmetic inst */
#define INS_LOGIC 	0x20 /* logical inst */
#define INS_FPU   	0x40 /* Floating Point inst */
#define INS_FLAG  	0x80 /* Modify flags */
   /* misc Instruction Types */
#define INS_MOVE		0x0100
#define INS_ARRAY    0x0200   /* String and XLAT ops */
#define INS_PTR      0x0400   /* Load EA/pointer */
#define INS_STACK 	0x1000	/* PUSH, POP, etc */
#define INS_FRAME	   0x2000	/* ENTER, LEAVE, etc */
#define INS_SYSTEM	0x4000	/* CPUID, WBINVD, etc */

/* Other info */
#define BIG_ENDIAN_ORDER 0
#define LITTLE_ENDIAN_ORDER 1

struct code {  /* size 100 */
    unsigned long    rva;
    unsigned short   flags;
    char    mnemonic[16];
    char    dest[32];
    char    src[32];
    char    aux[32];
    int     mnemType;
    int     destType;
    int     srcType;
    int     auxType;
};

/* struct used in Init routine */
struct CPU_TYPE{
	char vendor;
	char model[12];
};

#define cpu_80386      0x01
#define cpu_80486      0x02
#define cpu_PENTIUM    0x04
#define cpu_PENTMMX    0x08
#define cpu_PENTPRO    0x10
#define cpu_PENTIUM2   0x20
#define cpu_PENTIUM3   0x40
#define cpu_PENTIUM4   0x80

#define FLAGS_MODRM      0x00001  //contains mod r/m byte
#define FLAGS_8BIT       0x00002  //force 8-bit arguments
#define FLAGS_16BIT      0x00004  //force 16-bit arguments
#define FLAGS_32BIT      0x00008  //force 32-bit arguments
#define FLAGS_REAL       0x00010  //real mode only
#define FLAGS_PMODE      0x00020  //protected mode only
#define FLAGS_PREFIX     0x00040  //for lock and rep prefix
#define FLAGS_MMX        0x00080  //mmx instruction/registers
#define FLAGS_FPU        0x00100  //fpu instruction/registers
#define FLAGS_CJMP       0x00200  //codeflow - conditional jump
#define FLAGS_JMP        0x00400  //codeflow - jump
#define FLAGS_IJMP       0x00800  //codeflow - indexed jump
#define FLAGS_CALL       0x01000  //codeflow - call
#define FLAGS_ICALL      0x02000  //codeflow - indexed call
#define FLAGS_RET        0x04000  //codeflow - return
#define FLAGS_SEGPREFIX  0x08000  //segment prefix
#define FLAGS_OPERPREFIX 0x10000  //operand prefix
#define FLAGS_ADDRPREFIX 0x20000  //address prefix
#define FLAGS_OMODE16    0x40000  //16-bit operand mode only
#define FLAGS_OMODE32    0x80000  //32-bit operand mode only

enum argtype {
  ARG_REG=1,ARG_IMM,ARG_NONE,ARG_MODRM,ARG_REG_AX,
  ARG_REG_ES,ARG_REG_CS,ARG_REG_SS,ARG_REG_DS,ARG_REG_FS,ARG_REG_GS,ARG_REG_BX,
  ARG_REG_CX,ARG_REG_DX,
  ARG_REG_SP,ARG_REG_BP,ARG_REG_SI,ARG_REG_DI,ARG_IMM8,ARG_RELIMM8,ARG_FADDR,ARG_REG_AL,
  ARG_MEMLOC,ARG_SREG,ARG_RELIMM,ARG_16REG_DX,ARG_REG_CL,ARG_REG_DL,ARG_REG_BL,ARG_REG_AH,
  ARG_REG_CH,ARG_REG_DH,ARG_REG_BH,ARG_MODREG,ARG_CREG,ARG_DREG,ARG_TREG_67,ARG_TREG,
  ARG_MREG,ARG_MMXMODRM,ARG_MODRM8,ARG_IMM_1,ARG_MODRM_FPTR,ARG_MODRM_S,ARG_MODRMM512,
  ARG_MODRMQ,ARG_MODRM_SREAL,ARG_REG_ST0,ARG_FREG,ARG_MODRM_PTR,ARG_MODRM_WORD,ARG_MODRM_SINT,
  ARG_MODRM_EREAL,ARG_MODRM_DREAL,ARG_MODRM_WINT,ARG_MODRM_LINT,ARG_REG_BC,ARG_REG_DE,
  ARG_REG_HL,ARG_REG_DE_IND,ARG_REG_HL_IND,ARG_REG_BC_IND,ARG_REG_SP_IND,ARG_REG_A,
  ARG_REG_B,ARG_REG_C,ARG_REG_D,ARG_REG_E,ARG_REG_H,ARG_REG_L,ARG_IMM16,ARG_REG_AF,
  ARG_REG_AF2,ARG_MEMLOC16,ARG_IMM8_IND,ARG_BIT,ARG_REG_IX,ARG_REG_IX_IND,ARG_REG_IY,
  ARG_REG_IY_IND,ARG_REG_C_IND,ARG_REG_I,ARG_REG_R,ARG_IMM16_A,ARG_MODRM16,ARG_SIMM8,
  ARG_IMM32,ARG_STRING,ARG_MODRM_BCD,ARG_PSTRING,ARG_DOSSTRING,ARG_CUNICODESTRING,
  ARG_PUNICODESTRING,ARG_NONEBYTE,ARG_XREG,ARG_XMMMODRM};
  
typedef struct x86_inst {
	int flags;
	int destType, srcType, auxType;
	int cpu_type;
	int inst_type;
	char *mnem;
	char *dest, *src, *aux;
} instr;


#define GENREG_8      0x0001
#define GENREG_16     0x0002
#define GENREG_32     0x0004
#define SEGREG        0x0008
#define MMXREG        0x0010
#define SIMDREG       0x0020
#define DEBUGREG      0x0040
#define CONTROLREG    0x0080
#define TESTREG       0x0100

#define NO_REG     0x100
#define DIRECT_REG 0x200
#define NO_BASE    0x400
#define NO_INDEX   0x800
#define DISP8     0x1000
#define DISP32    0x2000
#define HAS_SIB   0x4000
#define HAS_MODRM 0x8000 

struct OPERAND {    	//arg1, arg2, arg3
   char * str;			//temporary buffer for building arg text
   int    type;		//argument type
   int *  flag;		//pointer to CODE arg flags
   char * text;		//pointer to CODE arg text
};

struct EA {		//effective address [SIB/disp]
   int mode, flags;
   int mod, rm, reg;
   long disp;
   char sib[32];
};

struct modRM_byte {	
   unsigned int mod : 2;
   unsigned int reg : 3;
   unsigned int rm  : 3;
};

struct SIB_byte {
   unsigned int scale : 2;
   unsigned int index : 3;
   unsigned int base  : 3;
};

typedef struct x86_table {             //Assembly instruction tables
  instr *table;      //Pointer to table of instruction encodings
  char divisor;            // number to divide by for look up
  char mask;               // bit mask for look up
  char minlim,maxlim;      // limits on min/max entries.
  char modrmpos;           // modrm byte position plus
} asmtable;
