
int Sfx86OpcodeExec_returns(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_returns(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_enterleave(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_enterleave(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
