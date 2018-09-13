/*
 * defctxt.h
 *
 * Deflate context
 */
typedef unsigned short	t_search_node;
typedef unsigned int	t_match_pos;


typedef enum
{
	STATE_NORMAL,
	STATE_OUTPUTTING_TREE_STRUCTURE,
	STATE_OUTPUTTING_BLOCK
} t_encoder_state;



struct fast_encoder;
struct optimal_encoder;
struct std_encoder;


//
// Context info common to all encoders
//
typedef struct
{
	t_encoder_state			state;

	unsigned long			outputting_block_bitbuf;
	int						outputting_block_bitcount;
	byte *					outputting_block_bufptr;
	unsigned int			outputting_block_current_literal;
	unsigned int			outputting_block_num_literals;

	long					bufpos;
	long					bufpos_end;

    // output buffer
	BYTE *					output_curpos;
	BYTE *					output_endpos;
	BYTE *					output_near_end_threshold;

	// bit buffer variables for outputting data
	unsigned long			bitbuf;
	int						bitcount;

    // varies; std/optimal encoders use the normal 32K window, while the fast
    // encoder uses a smaller window
    long                    window_size;

	struct std_encoder *	std_encoder;
	struct optimal_encoder *optimal_encoder;
    struct fast_encoder *   fast_encoder;

	BOOL					no_more_input;
	
	// have we output "bfinal=1"?
	BOOL					marked_final_block;

    // do we need to call ResetCompression() before we start compressing?
    BOOL                    fNeedToResetCompression;

    // if GZIP, have we output the GZIP header?
    BOOL                    using_gzip;
    BOOL                    gzip_fOutputGzipHeader;
    ULONG                   gzip_crc32;
    ULONG                   gzip_input_stream_size;
} t_encoder_context;
