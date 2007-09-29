/* Fax G3/G4 tables */

/*
<raph> the first 2^(initialbits) entries map bit patterns to decodes
<raph> let's say initial_bits is 8 for the sake of example
<raph> and that the code is 1001
<raph> that means that entries 0x90 .. 0x9f have the entry { val, 4 }
<raph> because those are all the bytes that start with the code
<raph> and the 4 is the length of the code
... if (n_bits > initial_bits) ...
<raph> anyway, in that case, it basically points to a mini table
<raph> the n_bits is the maximum length of all codes beginning with that byte
<raph> so 2^(n_bits - initial_bits) is the size of the mini-table
<raph> peter came up with this, and it makes sense
*/

typedef struct cfd_node_s cfd_node;

struct cfd_node_s
{
	short val;
	short nbits;
};

enum
{
	cfd_white_initial_bits = 8,
	cfd_black_initial_bits = 7,
	cfd_2d_initial_bits = 7,
	cfd_uncompressed_initial_bits = 6	/* must be 6 */
};

/* non-run codes in tables */
enum
{
	ERROR = -1,
	ZEROS = -2, /* EOL follows, possibly with more padding first */
	UNCOMPRESSED = -3
};

/* semantic codes for cf_2d_decode */
enum
{
	P = -4,
	H = -5,
	VR3 = 0,
	VR2 = 1,
	VR1 = 2,
	V0 = 3,
	VL1 = 4,
	VL2 = 5,
	VL3 = 6
};

/* Decoding tables */

extern const cfd_node cf_white_decode[];
extern const cfd_node cf_black_decode[];
extern const cfd_node cf_2d_decode[];
extern const cfd_node cf_uncompressed_decode[];

