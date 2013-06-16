
int Sfx86OpcodeExec_inc(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_inc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);

sx86_ubyte op_dec8(softx86_ctx* ctx,sx86_ubyte src);
sx86_uword op_dec16(softx86_ctx* ctx,sx86_uword src);
sx86_udword op_dec32(softx86_ctx* ctx,sx86_udword src);
sx86_ubyte op_inc8(softx86_ctx* ctx,sx86_ubyte src);
sx86_uword op_inc16(softx86_ctx* ctx,sx86_uword src);
sx86_udword op_inc32(softx86_ctx* ctx,sx86_udword src);
