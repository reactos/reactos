
int Sfx86OpcodeExec_mov(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_mov(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_xlat(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_xlat(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_les(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_les(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);

int Sfx86OpcodeExec_lmsw286(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_lmsw286(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);

sx86_ubyte op_mov8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_mov16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
sx86_udword op_mov32(softx86_ctx* ctx,sx86_udword src,sx86_udword val);
