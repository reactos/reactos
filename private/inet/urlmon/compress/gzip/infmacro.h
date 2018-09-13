//
// infmacro.h
//

#define INPUT_EOF()	(context->input_curpos >= context->end_input_buffer)


// dump n bits from the bit buffer (n can be up to 16)
// in assertion: there must be at least n valid bits in the buffer
#define DUMPBITS(n) \
	bitbuf >>= n; \
	bitcount -= n; 


// return the next n bits in the bit buffer (n <= 16), then dump these bits
// in assertion: there must be at least n valid bits in the buffer
#define GETBITS(result, n) \
	bitcount -= n; \
	result = (bitbuf & g_BitMask[n]); \
	bitbuf >>= n; \


//
// Load bit buffer variables from context into local variables
//
#define LOAD_BITBUF_VARS() \
	bitbuf = context->bitbuf; \
	bitcount = context->bitcount; \
	input_ptr = context->input_curpos;


//
// Save bit buffer variables from local variables into context
//
#define SAVE_BITBUF_VARS() \
	context->bitbuf = bitbuf; \
	context->bitcount = bitcount; \
	context->input_curpos = input_ptr;


