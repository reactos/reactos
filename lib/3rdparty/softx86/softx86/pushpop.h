
int Sfx86OpcodeExec_push(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_push(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_pop(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_pop(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_ahf(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_ahf(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_pusha(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_pusha(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_popa(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_popa(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
