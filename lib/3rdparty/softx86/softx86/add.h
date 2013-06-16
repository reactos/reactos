
int Sfx86OpcodeExec_adc(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_adc(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_add(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_add(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_sbb(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_sbb(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_sub(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_sub(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);
int Sfx86OpcodeExec_cmp(sx86_ubyte opcode,softx86_ctx* ctx);
int Sfx86OpcodeDec_cmp(sx86_ubyte opcode,softx86_ctx* ctx,char buf[128]);

sx86_ubyte op_adc8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_adc16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
sx86_udword op_adc32(softx86_ctx* ctx,sx86_udword src,sx86_udword val);
sx86_ubyte op_add8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_add16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
sx86_udword op_add32(softx86_ctx* ctx,sx86_udword src,sx86_udword val);
sx86_ubyte op_sbb8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_sbb16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
sx86_udword op_sbb32(softx86_ctx* ctx,sx86_udword src,sx86_udword val);
sx86_ubyte op_sub8(softx86_ctx* ctx,sx86_ubyte src,sx86_ubyte val);
sx86_uword op_sub16(softx86_ctx* ctx,sx86_uword src,sx86_uword val);
sx86_udword op_sub32(softx86_ctx* ctx,sx86_udword src,sx86_udword val);
