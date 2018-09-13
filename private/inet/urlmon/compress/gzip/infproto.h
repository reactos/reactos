//
// infproto.h
//

// comninit.c
void InitStaticBlock(void);

// infinit.c
VOID inflateInit(VOID);

// inflate.c
BOOL ensureBitsContext(t_decoder_context *context, int num_bits);
int	 getBits(t_decoder_context *context, int n);
void dumpBits(t_decoder_context *context, int n);

// infuncmp.c
BOOL decodeUncompressedBlock(t_decoder_context *context, BOOL *end_of_block);

// inftree.c
BOOL readDynamicBlockHeader(t_decoder_context *context);

// infinput.c
void dumpBits(t_decoder_context *context, int n);
int getBits(t_decoder_context *context, int n);
BOOL ensureBitsContext(t_decoder_context *context, int num_bits);
BOOL initBitBuffer(t_decoder_context *context);

// infdyna.c
BOOL DecodeDynamicBlock(t_decoder_context *context, BOOL *end_of_block_code_seen); 
BOOL FastDecodeDynamicBlock(t_decoder_context *context, BOOL *end_of_block_code_seen);

// infstatic.c
BOOL DecodeStaticBlock(t_decoder_context *context, BOOL *end_of_block_code_seen);
BOOL FastDecodeStaticBlock(t_decoder_context *context, BOOL *end_of_block_code_seen);
