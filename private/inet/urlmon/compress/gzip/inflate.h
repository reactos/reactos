#include "common.h"
#include "api_int.h"


// decoding tables for dynamic blocks
#define LITERAL_TABLE_BITS		9
#define LITERAL_TABLE_MASK		((1 << LITERAL_TABLE_BITS)-1)

#define DISTANCE_TABLE_BITS		7
#define DISTANCE_TABLE_MASK		((1 << DISTANCE_TABLE_BITS)-1)

#define PRETREE_TABLE_BITS		7
#define PRETREE_TABLE_MASK		((1 << PRETREE_TABLE_BITS)-1)


// decoding tables for static blocks
#define STATIC_BLOCK_LITERAL_TABLE_BITS		9
#define STATIC_BLOCK_LITERAL_TABLE_MASK		((1 << STATIC_BLOCK_LITERAL_TABLE_BITS)-1)
#define STATIC_BLOCK_LITERAL_TABLE_SIZE		(1 << STATIC_BLOCK_LITERAL_TABLE_BITS)

#define STATIC_BLOCK_DISTANCE_TABLE_BITS    5
#define STATIC_BLOCK_DISTANCE_TABLE_MASK	((1 << STATIC_BLOCK_DISTANCE_TABLE_BITS)-1)
#define STATIC_BLOCK_DISTANCE_TABLE_SIZE	(1 << STATIC_BLOCK_DISTANCE_TABLE_BITS)


//
// Various possible states
//
typedef enum
{
    STATE_READING_GZIP_HEADER, // Only applies to GZIP
	STATE_READING_BFINAL_NEED_TO_INIT_BITBUF, // Start of block, need to init bit buffer
	STATE_READING_BFINAL,				// About to read bfinal bit
	STATE_READING_BTYPE,				// About to read btype bits
	STATE_READING_NUM_LIT_CODES,		// About to read # literal codes
	STATE_READING_NUM_DIST_CODES,		// About to read # dist codes
	STATE_READING_NUM_CODE_LENGTH_CODES,// About to read # code length codes
	STATE_READING_CODE_LENGTH_CODES,	// In the middle of reading the code length codes
	STATE_READING_TREE_CODES_BEFORE,	// In the middle of reading tree codes (loop top)
	STATE_READING_TREE_CODES_AFTER,		// In the middle of reading tree codes (extension; code > 15)
	STATE_DECODE_TOP,					// About to decode a literal (char/match) in a compressed block
	STATE_HAVE_INITIAL_LENGTH,			// Decoding a match, have the literal code (base length)
	STATE_HAVE_FULL_LENGTH,				// Ditto, now have the full match length (incl. extra length bits)
	STATE_HAVE_DIST_CODE,				// Ditto, now have the distance code also, need extra dist bits
	STATE_INTERRUPTED_MATCH,			// In the middle of a match, but output buffer filled up

	/* uncompressed blocks */
	STATE_UNCOMPRESSED_ALIGNING,
	STATE_UNCOMPRESSED_1,
	STATE_UNCOMPRESSED_2,
	STATE_UNCOMPRESSED_3,
	STATE_UNCOMPRESSED_4,
	STATE_DECODING_UNCOMPRESSED,

    // These three apply only to GZIP
    STATE_START_READING_GZIP_FOOTER, // (Initialisation for reading footer)
    STATE_READING_GZIP_FOOTER, 
    STATE_VERIFYING_GZIP_FOOTER,

    STATE_DONE // Finished

} t_decoder_state;


typedef struct
{
	byte				window[WINDOW_SIZE];

	// output buffer
	byte *				output_curpos;		// current output pos
	byte *				end_output_buffer;	// ptr to end of output buffer
	byte *				output_buffer;		// ptr to start of output buffer

	// input buffer
	const byte *		input_curpos;		// current input pos
	const byte *		end_input_buffer;	// ptr to end of input buffer

	int					num_literal_codes;
	int					num_dist_codes;
	int					num_code_length_codes;
	int					temp_code_array_size;
	byte				temp_code_list[MAX_LITERAL_TREE_ELEMENTS + MAX_DIST_TREE_ELEMENTS];

	// is this the last block?
	int					bfinal;

	// type of current block
	int					btype;

	// state information
	t_decoder_state		state;
	long				state_loop_counter;
	byte				state_code;
    BOOL                using_gzip;

    // gzip-specific stuff
    byte                gzip_header_substate;
    byte                gzip_header_flag;
    byte                gzip_header_xlen1_byte; // first byte of XLEN
    unsigned int        gzip_header_xlen; // xlen (0...65535)
    unsigned int        gzip_header_loop_counter;

    byte                gzip_footer_substate;
    unsigned int        gzip_footer_loop_counter;
    unsigned long       gzip_footer_crc32; // what we're supposed to end up with
    unsigned long       gzip_footer_output_stream_size; // what we're supposed to end up with

    unsigned long       gzip_crc32; // running counter
    unsigned long       gzip_output_stream_size; // running counter
    // end of gzip-specific stuff

	int					length;
	int					dist_code;
	long				offset;

	// bit buffer and # bits available in buffer
	unsigned long		bitbuf;
	int					bitcount;

	// position in the window
	long				bufpos;

	// for decoding the uncompressed block header
	byte				unc_buffer[4];

	// bit lengths of tree codes
	byte				literal_tree_code_length[MAX_LITERAL_TREE_ELEMENTS];
	byte				distance_tree_code_length[MAX_DIST_TREE_ELEMENTS];
	byte				pretree_code_length[NUM_PRETREE_ELEMENTS];

	// tree decoding tables
	short				distance_table[1 << DISTANCE_TABLE_BITS];
	short				literal_table[1 << LITERAL_TABLE_BITS];

	short 				literal_left[MAX_LITERAL_TREE_ELEMENTS*2];
	short 				literal_right[MAX_LITERAL_TREE_ELEMENTS*2];

	short 				distance_left[MAX_DIST_TREE_ELEMENTS*2];
	short 				distance_right[MAX_DIST_TREE_ELEMENTS*2];

	short				pretree_table[1 << PRETREE_TABLE_BITS];
	short				pretree_left[NUM_PRETREE_ELEMENTS*2];
	short				pretree_right[NUM_PRETREE_ELEMENTS*2];

} t_decoder_context;


#include "infproto.h"

#include "infdata.h"
#include "comndata.h"
