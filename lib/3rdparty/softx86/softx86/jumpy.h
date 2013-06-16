
int Sfx86OpcodeExec_jc(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_jc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_call(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_call(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_jmp(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_jmp(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_loop(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_loop(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
