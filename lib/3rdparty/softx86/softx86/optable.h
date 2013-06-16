
// return value:
// 0 = opcode not recognized
// 1 = opcode recognized, parsing complete
// 2 = opcode recognized, more parsing needed (i.e., prefix)
typedef int (*Sfx86OpcodeExec)(sx86_ubyte opcode,softx86_ctx* ctx);
typedef int (*Sfx86OpcodeDec)(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);

typedef struct {
	Sfx86OpcodeExec			exec;
	Sfx86OpcodeDec			dec;
} Sfx86Opcode;

typedef struct {
	Sfx86Opcode			table[256];
} Sfx86OpcodeTable;

extern Sfx86OpcodeTable			optab8086;
extern Sfx86OpcodeTable			optab8086_0Fh;

extern char				op1_tmp[32];
extern char				op2_tmp[32];

extern char*				sx86_regs8[8];
extern char*				sx86_regs16[8];
extern char*				sx86_regs32[8];
extern char*				sx86_regsaddr16_16[8];
extern char*				sx86_segregs[8];
