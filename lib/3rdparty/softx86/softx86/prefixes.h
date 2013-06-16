
int Sfx86OpcodeExec_segover(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_segover(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_repetition(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_repetition(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_lock(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_lock(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
