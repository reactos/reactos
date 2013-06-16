
// return value:
// 0 = opcode not recognized
// 1 = opcode recognized, parsing complete
// 2 = opcode recognized, more parsing needed (i.e., prefix)
typedef int (*Sfx87OpcodeExec)(sx86_ubyte opcode,softx87_ctx* ctx);
typedef int (*Sfx87OpcodeDec)(sx86_ubyte opcode,softx87_ctx* ctx,char buf[128]);

typedef struct {
	Sfx87OpcodeExec			exec;
	Sfx87OpcodeDec			dec;
} Sfx87Opcode;

/* for sanity's sake we only look at the
   upper 3 bits of the 11-bit opcode.
   the opcode arrangement of FPU opcodes
   makes a lot less sense than the CPU
   opcode arrangement. */
typedef struct {
	Sfx87Opcode			table[8];
} Sfx87OpcodeTable;

extern Sfx87OpcodeTable			optab8087;

extern char				s87op1_tmp[32];
extern char				s87op2_tmp[32];
