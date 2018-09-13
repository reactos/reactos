/*
 * optenc.h
 *
 * Defines for the optimal encoder
 */


// lookahead
#define LOOK			1024

// don't-care threshold for tree structure
#define BREAK_LENGTH	50

#define NUM_DIRECT_LOOKUP_TABLE_ELEMENTS    65536

// see stdenc.h for comments on these values
#define OPT_ENCODER_LIT_DIST_BUFFER_SIZE    65536
#define OPT_ENCODER_MAX_ITEMS				65534


//
// For the optimal parser
//
typedef unsigned long numbits_t;

typedef struct
{
	ULONG		link;
	ULONG		path;
	numbits_t	numbits;
} t_decision_node;


//
// Optimal encoder context
//
typedef struct optimal_encoder
{
	BYTE 					window[2*WINDOW_SIZE + MAX_MATCH + 4];
	t_decision_node 		decision_node[LOOK+MAX_MATCH+16];
	t_match_pos				matchpos_table[MAX_MATCH+1];
	t_search_node			search_left[2*WINDOW_SIZE];
	t_search_node			search_right[2*WINDOW_SIZE];
	t_search_node			search_tree_root[65536];

	// recording buffer for recording literals and distances
	BYTE					lit_dist_buffer[OPT_ENCODER_LIT_DIST_BUFFER_SIZE];
	unsigned long			recording_bitbuf;
	unsigned long			recording_bitcount;
    BYTE *                  recording_bufptr;

	unsigned int			next_tree_update;

	short					recording_dist_tree_table[REC_DISTANCES_DECODING_TABLE_SIZE];
	short					recording_dist_tree_left[2*MAX_DIST_TREE_ELEMENTS];
	short					recording_dist_tree_right[2*MAX_DIST_TREE_ELEMENTS];
    BYTE					recording_dist_tree_len[MAX_DIST_TREE_ELEMENTS];
    USHORT                  recording_dist_tree_code[MAX_DIST_TREE_ELEMENTS];

	short					recording_literal_tree_table[REC_LITERALS_DECODING_TABLE_SIZE];
	short					recording_literal_tree_left[2*MAX_LITERAL_TREE_ELEMENTS];
	short					recording_literal_tree_right[2*MAX_LITERAL_TREE_ELEMENTS];
	BYTE					recording_literal_tree_len[MAX_LITERAL_TREE_ELEMENTS];
	USHORT                  recording_literal_tree_code[MAX_LITERAL_TREE_ELEMENTS];

	// literal trees
    unsigned short          literal_tree_freq[2*MAX_LITERAL_TREE_ELEMENTS];
	unsigned short			literal_tree_code[MAX_LITERAL_TREE_ELEMENTS];
	BYTE					literal_tree_len[MAX_LITERAL_TREE_ELEMENTS];
	
	// dist trees
    unsigned short          dist_tree_freq[2*MAX_DIST_TREE_ELEMENTS];
	unsigned short			dist_tree_code[MAX_DIST_TREE_ELEMENTS];
	BYTE					dist_tree_len[MAX_DIST_TREE_ELEMENTS];

} t_optimal_encoder;



