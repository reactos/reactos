typedef struct {
  __int64 put_buffer_64; //mmx bit-accumulation buffer
  INT32 put_buffer;		/* current bit-accumulation buffer */
  int put_bits;			/* # of bits now in it */
  int last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each component */
} savable_state;

/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX_COMPS_IN_SCAN.
 */

//#ifndef NO_STRUCT_ASSIGN
//#define ASSIGN_STATE(dest,src)  ((dest) = (src))
//#else
// pull out the assignments to put_buffer and put_bits since they are implentation dependent
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
	/*((dest).put_buffer = (src).put_buffer, \
	 (dest).put_bits = (src).put_bits, */ 
#endif
//#endif


typedef struct {
  struct jpeg_entropy_encoder pub; /* public fields */

  savable_state saved;		/* Bit buffer & DC state at start of MCU */

  /* These fields are NOT loaded into local working state. */
  unsigned int restarts_to_go;	/* MCUs left in this restart interval */
  int next_restart_num;		/* next restart number to write (0-7) */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  c_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
  c_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];

#ifdef ENTROPY_OPT_SUPPORTED	/* Statistics tables for optimization */
  long * dc_count_ptrs[NUM_HUFF_TBLS];
  long * ac_count_ptrs[NUM_HUFF_TBLS];
#endif
} huff_entropy_encoder;

typedef huff_entropy_encoder * huff_entropy_ptr;

/* Working state while writing an MCU.
 * This struct contains all the fields that are needed by subroutines.
 */

typedef struct {
//  make the next two variables global for easy access in mmx version
//  JOCTET * next_output_byte;	/* => next byte to write in buffer */
//  size_t free_in_buffer;	/* # of byte spaces remaining in buffer */
//  savable_state cur;		/* Current bit buffer & DC state */
// flatten (instantiate) savable state here
  __int64 put_buffer_64; // mmx bit accumulation buffer
  INT32 put_buffer;		/* current bit-accumulation buffer */
  int put_bits;			/* # of bits now in it */
  int last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each component */
  j_compress_ptr cinfo;		/* dump_buffer needs access to this */
} working_state;
//global vaiables
  __int64 put_buffer_64;  
//  INT32 put_buffer;
  int put_bits;
  JOCTET * next_output_byte;	/* => next byte to write in buffer */
  size_t free_in_buffer;	/* # of byte spaces remaining in buffer */
  //boolean mmx_cpu=1;

