/*
 * jchuff.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy encoding routines.
 *
 * Much of the complexity here has to do with supporting output suspension.
 * If the data destination module demands suspension, we want to be able to
 * back up to the start of the current MCU.  To do this, we copy state
 * variables into local working storage, and update them back to the
 * permanent JPEG objects only upon successful completion of an MCU.
 */


// MMx Optimisation disabled 5/29/97 Gromit Bug 4375 -Tiling error. - ajais.


#pragma warning( disable : 4799 )
 
#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jchuff.h"		/* Declarations shared with jcphuff.c */

/* Expanded entropy encoder object for Huffman encoding.
 *
 * The savable_state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */
typedef struct 
{

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


typedef struct 
{
  
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

typedef struct 
{
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
  boolean mmx_cpu=1;


/* Forward declarations */
METHODDEF(boolean) encode_mcu_huff JPP((j_compress_ptr cinfo,
					JBLOCKROW *MCU_data));
METHODDEF(void) finish_pass_huff JPP((j_compress_ptr cinfo));

#ifdef ENTROPY_OPT_SUPPORTED
METHODDEF(boolean) encode_mcu_gather JPP((j_compress_ptr cinfo,
					  JBLOCKROW *MCU_data));
METHODDEF(void) finish_pass_gather JPP((j_compress_ptr cinfo));
#endif

void countZeros(int *indexBlock,short *coefBlock,short *outBlock,int *lastZeros,int *numElements);

boolean emit_bits_fast (working_state * state, unsigned int code, int bsize, int only1);
//extern boolean emit_bits (working_state * state, unsigned int code, int size);

boolean encode_one_block_fast (working_state * state, JCOEFPTR block, int last_dc_val,
		  c_derived_tbl *dctbl, c_derived_tbl *actbl);
//extern boolean encode_one_block (working_state * state, JCOEFPTR block, int last_dc_val,
//		  c_derived_tbl *dctbl, c_derived_tbl *actbl);

/*
 * Initialize for a Huffman-compressed scan.
 * If gather_statistics is TRUE, we do not output anything during the scan,
 * just count the Huffman symbols used and generate Huffman code tables.
 */

METHODDEF(void)
start_pass_huff (j_compress_ptr cinfo, boolean gather_statistics)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;

  if (gather_statistics) {
#ifdef ENTROPY_OPT_SUPPORTED
    entropy->pub.encode_mcu = encode_mcu_gather;
    entropy->pub.finish_pass = finish_pass_gather;
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    entropy->pub.encode_mcu = encode_mcu_huff;
    entropy->pub.finish_pass = finish_pass_huff;
  }

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    /* Make sure requested tables are present */
    /* (In gather mode, tables need not be allocated yet) */
    if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS ||
	(cinfo->dc_huff_tbl_ptrs[dctbl] == NULL && !gather_statistics))
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
    if (actbl < 0 || actbl >= NUM_HUFF_TBLS ||
	(cinfo->ac_huff_tbl_ptrs[actbl] == NULL && !gather_statistics))
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, actbl);
    if (gather_statistics) {
#ifdef ENTROPY_OPT_SUPPORTED
      /* Allocate and zero the statistics tables */
      /* Note that jpeg_gen_optimal_table expects 257 entries in each table! */
      if (entropy->dc_count_ptrs[dctbl] == NULL)
	entropy->dc_count_ptrs[dctbl] = (long *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->dc_count_ptrs[dctbl], 257 * SIZEOF(long));
      if (entropy->ac_count_ptrs[actbl] == NULL)
	entropy->ac_count_ptrs[actbl] = (long *)
	  (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				      257 * SIZEOF(long));
      MEMZERO(entropy->ac_count_ptrs[actbl], 257 * SIZEOF(long));
#endif
    } else {
      /* Compute derived values for Huffman tables */
      /* We may do this more than once for a table, but it's not expensive */
      jpeg_make_c_derived_tbl(cinfo, cinfo->dc_huff_tbl_ptrs[dctbl],
			      & entropy->dc_derived_tbls[dctbl]);
      jpeg_make_c_derived_tbl(cinfo, cinfo->ac_huff_tbl_ptrs[actbl],
			      & entropy->ac_derived_tbls[actbl]);
    }
    /* Initialize DC predictions to 0 */
    entropy->saved.last_dc_val[ci] = 0;
  }

  /* Initialize bit buffer to empty */
  entropy->saved.put_buffer_64 = 0;
  entropy->saved.put_buffer = 0;
  entropy->saved.put_bits = 0;

  /* Initialize restart stuff */
  entropy->restarts_to_go = cinfo->restart_interval;
  entropy->next_restart_num = 0;
}


/*
 * Compute the derived values for a Huffman table.
 * Note this is also used by jcphuff.c.
 */

GLOBAL(void)
jpeg_make_c_derived_tbl (j_compress_ptr cinfo, JHUFF_TBL * htbl,
			 c_derived_tbl ** pdtbl)
{
  c_derived_tbl *dtbl;
  int p, i, l, lastp, si;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  /* Allocate a workspace if we haven't already done so. */
  if (*pdtbl == NULL)
    *pdtbl = (c_derived_tbl *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(c_derived_tbl));
  dtbl = *pdtbl;
  
  /* Figure C.1: make table of Huffman code length for each symbol */
  /* Note that this is in code-length order. */

  p = 0;
  for (l = 1; l <= 16; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  lastp = p;
  
  /* Figure C.2: generate the codes themselves */
  /* Note that this is in code-length order. */
  
  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (((int) huffsize[p]) == si) {
      huffcode[p++] = code;
      code++;
    }
    code <<= 1;
    si++;
  }
  
  /* Figure C.3: generate encoding tables */
  /* These are code and size indexed by symbol value */

  /* Set any codeless symbols to have code length 0;
   * this allows emit_bits to detect any attempt to emit such symbols.
   */
  MEMZERO(dtbl->ehufsi, SIZEOF(dtbl->ehufsi));

  for (p = 0; p < lastp; p++) {
    dtbl->ehufco[htbl->huffval[p]] = huffcode[p];
    dtbl->ehufsi[htbl->huffval[p]] = huffsize[p];
  }
}


/* Outputting bytes to the file */

/* Emit a byte, taking 'action' if must suspend. */
#define emit_byte(state,val,action)  \
	{ *next_output_byte++ = (JOCTET) (val);  \
	  if (--free_in_buffer == 0)  \
	    if (! dump_buffer(state))  \
	      { action; } }


GLOBAL(boolean)
dump_buffer (working_state * state)
/* Empty the output buffer; return TRUE if successful, FALSE if must suspend */
{
  struct jpeg_destination_mgr * dest = state->cinfo->dest;

  if (! (*dest->empty_output_buffer) (state->cinfo))
    return FALSE;
  /* After a successful buffer dump, must reset buffer pointers */
  next_output_byte = dest->next_output_byte;
  free_in_buffer = dest->free_in_buffer;
  return TRUE;
}


/* Outputting bits to the file */

/* Only the right 24 bits of put_buffer are used; the valid bits are
 * left-justified in this part.  At most 16 bits can be passed to emit_bits
 * in one call, and we never retain more than 7 bits in put_buffer
 * between calls, so 24 bits are sufficient.
 */

//INLINE
LOCAL(boolean)
emit_bits (working_state * state, unsigned int code, int size)
/* Emit some bits; return TRUE if successful, FALSE if must suspend */
{
  /* This routine is heavily used, so it's worth coding tightly. */
  register INT32 put_buffer = (INT32) code;
  register int put_bits = state->put_bits;

  /* if size is 0, caller used an invalid Huffman table entry */
  if (size == 0)
    ERREXIT(state->cinfo, JERR_HUFF_MISSING_CODE);

  put_buffer &= (((INT32) 1)<<size) - 1; /* mask off any extra bits in code */
  
  put_bits += size;		/* new number of bits in buffer */
  
  put_buffer <<= 24 - put_bits; /* align incoming bits */

  put_buffer |= state->put_buffer; /* and merge with old buffer contents */
  
  while (put_bits >= 8) {
    int c = (int) ((put_buffer >> 16) & 0xFF);
    
    emit_byte(state, c, return FALSE);
    if (c == 0xFF) {		/* need to stuff a zero byte? */
      emit_byte(state, 0, return FALSE);
    }
    put_buffer <<= 8;
    put_bits -= 8;
  }

  state->put_buffer = put_buffer; /* update state variables */
  state->put_bits = put_bits;

  return TRUE;
}


//This is a routine to dump whatever is in put_buffer out - I salvaged it from another
//routine so there is some dead-code as-used.

//As flush-bits is not called frequently, there should not be much overhead to this code . . .
//MJB

//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
void flush_bit_buffer_64()
{
	// byte-align previous bits if any 
__asm{

	mov ebx,[put_bits]

	mov eax,[next_output_byte]

	test ebx,ebx
	je no_ser_buf_data
	
	
			movq mm0,[put_buffer_64]
			pxor mm2,mm2

dump_loop:	movq mm1,mm0
			psrlq mm0,56
			
			movd ecx,mm0
			movq mm0,mm1

			mov (byte ptr[eax]),cl
			inc eax

			cmp ecx,0xFF
			jne not_ff

			mov (byte ptr[eax]),0x00
			dec [free_in_buffer]

			inc eax
			nop
not_ff:
			dec [free_in_buffer]
			psllq mm0,8

			sub ebx,8
			jg dump_loop

			mov [put_bits],0
			//mov [eb_ptr],eax
			
			movq [put_buffer_64],mm2
			//emms

no_ser_buf_data:

			mov [next_output_byte],eax

	}
}
#endif // #ifdef (_X86_)

LOCAL(boolean)
flush_bits (working_state * state)
{
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
  if (0)//vfMMXMachine)
  {
	//if (! emit_bits(state, 0x7F, 7)) /* fill any partial byte with ones */
	if (! emit_bits_fast(state, 0x7F, 7, 1)) /* fill any partial byte with ones */
		return FALSE;
    if (put_bits)
	{
		flush_bit_buffer_64();		// New stuff to write the last, few bits . . .
	}

  }
  else
#endif
  {
	if (! emit_bits(state, 0x7F, 7)) /* fill any partial byte with ones */
		return FALSE;
  }

  state->put_buffer_64 = 0;	/* and reset bit-buffer to empty */
  state->put_buffer = 0;	/* and reset bit-buffer to empty */
  state->put_bits = 0;
  return TRUE;
}


/* Encode a single block's worth of coefficients */

LOCAL(boolean)
encode_one_block (working_state * state, JCOEFPTR block, int last_dc_val,
		  c_derived_tbl *dctbl, c_derived_tbl *actbl)
{
  register int temp, temp2;
  register int nbits;
  register int k, r, i;
  
  /* Encode the DC coefficient difference per section F.1.2.1 */
  
  temp = temp2 = block[0] - last_dc_val;

  if (temp < 0) 
  {
    temp = -temp;		/* temp is abs value of input */
    /* For a negative input, want temp2 = bitwise complement of abs(input) */
    /* This code assumes we are on a two's complement machine */
    temp2--;
  }
  
  /* Find the number of bits needed for the magnitude of the coefficient */
  nbits = 0;
  while (temp) 
  {
    nbits++;
    temp >>= 1;
  }
  
  /* Emit the Huffman-coded symbol for the number of bits */
  if (! emit_bits(state, dctbl->ehufco[nbits], dctbl->ehufsi[nbits]))
    return FALSE;

  /* Emit that number of bits of the value, if positive, */
  /* or the complement of its magnitude, if negative. */
  if (nbits)			/* emit_bits rejects calls with size 0 */
    if (! emit_bits(state, (unsigned int) temp2, nbits))
      return FALSE;

  /* Encode the AC coefficients per section F.1.2.2 */
  
  r = 0;			/* r = run length of zeros */
  
  for (k = 1; k < DCTSIZE2; k++) 
  {
    if ((temp = block[jpeg_natural_order[k]]) == 0) 
	{
      r++;
    } 
	else 
	{
      /* if run length > 15, must emit special run-length-16 codes (0xF0) */
      while (r > 15) 
	  {
		if (! emit_bits(state, actbl->ehufco[0xF0], actbl->ehufsi[0xF0]))
			return FALSE;
		r -= 16;
      }

      temp2 = temp;
      
	  if (temp < 0) 
	  {
		  temp = -temp;		/* temp is abs value of input */
	/* This code assumes we are on a two's complement machine */
		  temp2--;
      }
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      nbits = 1;		/* there must be at least one 1 bit */
      while ((temp >>= 1))
		  nbits++;
      
      /* Emit Huffman symbol for run length / number of bits */
      i = (r << 4) + nbits;
      if (! emit_bits(state, actbl->ehufco[i], actbl->ehufsi[i]))
		  return FALSE;

      /* Emit that number of bits of the value, if positive, */
      /* or the complement of its magnitude, if negative. */
      if (! emit_bits(state, (unsigned int) temp2, nbits))
		  return FALSE;
      
	  r = 0;
	}
  }

  /* If the last coef(s) were zero, emit an end-of-block code */
  if (r > 0)
    if (! emit_bits(state, actbl->ehufco[0], actbl->ehufsi[0]))
      return FALSE;

  return TRUE;
}


/*
 * Emit a restart marker & resynchronize predictions.
 */

LOCAL(boolean)
emit_restart (working_state * state, int restart_num)
{
  int ci;

  if (! flush_bits(state))
    return FALSE;

  emit_byte(state, 0xFF, return FALSE);
  emit_byte(state, JPEG_RST0 + restart_num, return FALSE);

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < state->cinfo->comps_in_scan; ci++)
    state->last_dc_val[ci] = 0;

  /* The restart counter is not updated until we successfully write the MCU. */

  return TRUE;
}


/*
 * Encode and output one MCU's worth of Huffman-compressed coefficients.
 */

METHODDEF(boolean)
encode_mcu_huff (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  working_state state;
  int blkn, ci;
  jpeg_component_info * compptr;

  /* Load up working state */
  next_output_byte = cinfo->dest->next_output_byte;
  free_in_buffer = cinfo->dest->free_in_buffer;
  if (0)//vfMMXMachine)
  {
	  state.put_buffer_64=entropy->saved.put_buffer_64;
  }
  else
  {
	  state.put_buffer=entropy->saved.put_buffer;
  }
  state.put_bits=entropy->saved.put_bits;
  ASSIGN_STATE(state, entropy->saved);
  state.cinfo = cinfo;

  /* Emit restart marker if needed */
  if (cinfo->restart_interval) 
  {
	  if (entropy->restarts_to_go == 0)
		  if (! emit_restart(&state, entropy->next_restart_num))
			  return FALSE;
  }

  /* Encode the MCU data blocks */
  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) 
  {
	  ci = cinfo->MCU_membership[blkn];
	  compptr = cinfo->cur_comp_info[ci];
	
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
    	  if (0)//vfMMXMachine)
	  {
		if (! encode_one_block_fast(&state,
			   MCU_data[blkn][0], state.last_dc_val[ci],
			   entropy->dc_derived_tbls[compptr->dc_tbl_no],
			   entropy->ac_derived_tbls[compptr->ac_tbl_no]))
			   return FALSE;
	  }
	  else
#endif	  
          {
		  if (! encode_one_block(&state,
			   MCU_data[blkn][0], state.last_dc_val[ci],
			   entropy->dc_derived_tbls[compptr->dc_tbl_no],
			   entropy->ac_derived_tbls[compptr->ac_tbl_no]))
			   return FALSE;
	  }
    /* Update last_dc_val */
    state.last_dc_val[ci] = MCU_data[blkn][0][0];
  }

  /* Completed MCU, so update state */
  cinfo->dest->next_output_byte = next_output_byte;
  cinfo->dest->free_in_buffer = free_in_buffer;

  if (0)//vfMMXMachine)
  {
	  entropy->saved.put_buffer_64=state.put_buffer_64;
  }
  else
  {
	  entropy->saved.put_buffer=state.put_buffer;
  }
  
  entropy->saved.put_bits=state.put_bits;
  
  ASSIGN_STATE(entropy->saved, state);

  /* Update restart-interval state too */
  if (cinfo->restart_interval) 
  {
	  if (entropy->restarts_to_go == 0) 
	  {
		  entropy->restarts_to_go = cinfo->restart_interval;
		  entropy->next_restart_num++;
		  entropy->next_restart_num &= 7;
	  }
	  entropy->restarts_to_go--;
  }
  
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)
  if (0)//vfMMXMachine)
  {
	  __asm emms
  }
#endif

  return TRUE;
}


/*
 * Finish up at the end of a Huffman-compressed scan.
 */

METHODDEF(void)
finish_pass_huff (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  working_state state;

  /* Load up working state ... flush_bits needs it */
  next_output_byte = cinfo->dest->next_output_byte;
  free_in_buffer = cinfo->dest->free_in_buffer;

  if (0)//vfMMXMachine)
  {
	  state.put_buffer_64=entropy->saved.put_buffer_64;
  }
  else
  {
	  state.put_buffer=entropy->saved.put_buffer;
  }
  
  state.put_bits=entropy->saved.put_bits;
  ASSIGN_STATE(state, entropy->saved);
  state.cinfo = cinfo;

  /* Flush out the last data */
  if (!flush_bits(&state))
	  ERREXIT(cinfo, JERR_CANT_SUSPEND);

  /* Update state */
  cinfo->dest->next_output_byte = next_output_byte;
  cinfo->dest->free_in_buffer = free_in_buffer;
  
  if (0)//vfMMXMachine)
  {
	  entropy->saved.put_buffer_64=state.put_buffer_64;
  }
  else
  {
	  entropy->saved.put_buffer=state.put_buffer;
  }
  entropy->saved.put_bits=state.put_bits;
  ASSIGN_STATE(entropy->saved, state);
  
//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)  
  if (0)//vfMMXMachine)
  {
	  __asm emms
  }
#endif
}


/*
 * Huffman coding optimization.
 *
 * This actually is optimization, in the sense that we find the best possible
 * Huffman table(s) for the given data.  We first scan the supplied data and
 * count the number of uses of each symbol that is to be Huffman-coded.
 * (This process must agree with the code above.)  Then we build an
 * optimal Huffman coding tree for the observed counts.
 *
 * The JPEG standard requires Huffman codes to be no more than 16 bits long.
 * If some symbols have a very small but nonzero probability, the Huffman tree
 * must be adjusted to meet the code length restriction.  We currently use
 * the adjustment method suggested in the JPEG spec.  This method is *not*
 * optimal; it may not choose the best possible limited-length code.  But
 * since the symbols involved are infrequently used, it's not clear that
 * going to extra trouble is worthwhile.
 */

#ifdef ENTROPY_OPT_SUPPORTED


/* Process a single block's worth of coefficients */

LOCAL(void)
htest_one_block (JCOEFPTR block, int last_dc_val,
		 long dc_counts[], long ac_counts[])
{
  register int temp;
  register int nbits;
  register int k, r;
  
  /* Encode the DC coefficient difference per section F.1.2.1 */
  
  temp = block[0] - last_dc_val;
  if (temp < 0)
    temp = -temp;
  
  /* Find the number of bits needed for the magnitude of the coefficient */
  nbits = 0;
  while (temp) 
  {
    nbits++;
    temp >>= 1;
  }

  /* Count the Huffman symbol for the number of bits */
  dc_counts[nbits]++;
  
  /* Encode the AC coefficients per section F.1.2.2 */
  
  r = 0;			/* r = run length of zeros */
  
  for (k = 1; k < DCTSIZE2; k++) 
  {
    if ((temp = block[jpeg_natural_order[k]]) == 0) 
	{
      r++;
    } 
	else 
	{
      /* if run length > 15, must emit special run-length-16 codes (0xF0) */
      while (r > 15) 
	  {
		ac_counts[0xF0]++;
		r -= 16;
      }
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      if (temp < 0) temp = -temp;
      
      /* Find the number of bits needed for the magnitude of the coefficient */
      nbits = 1;		/* there must be at least one 1 bit */
      while ((temp >>= 1)) nbits++;
      
      /* Count Huffman symbol for run length / number of bits */
      ac_counts[(r << 4) + nbits]++;
      
      r = 0;
    }
  }

  /* If the last coef(s) were zero, emit an end-of-block code */
  if (r > 0)
    ac_counts[0]++;
}


/*
 * Trial-encode one MCU's worth of Huffman-compressed coefficients.
 * No data is actually output, so no suspension return is possible.
 */

METHODDEF(boolean)
encode_mcu_gather (j_compress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int blkn, ci;
  jpeg_component_info * compptr;

  /* Take care of restart intervals if needed */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0) {
      /* Re-initialize DC predictions to 0 */
      for (ci = 0; ci < cinfo->comps_in_scan; ci++)
	entropy->saved.last_dc_val[ci] = 0;
      /* Update restart state */
      entropy->restarts_to_go = cinfo->restart_interval;
    }
    entropy->restarts_to_go--;
  }

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    htest_one_block(MCU_data[blkn][0], entropy->saved.last_dc_val[ci],
		    entropy->dc_count_ptrs[compptr->dc_tbl_no],
		    entropy->ac_count_ptrs[compptr->ac_tbl_no]);
    entropy->saved.last_dc_val[ci] = MCU_data[blkn][0][0];
  }

  return TRUE;
}


/*
 * Generate the optimal coding for the given counts, fill htbl.
 * Note this is also used by jcphuff.c.
 */

GLOBAL(void)
jpeg_gen_optimal_table (j_compress_ptr cinfo, JHUFF_TBL * htbl, long freq[])
{
#define MAX_CLEN 32		/* assumed maximum initial code length */
  UINT8 bits[MAX_CLEN+1];	/* bits[k] = # of symbols with code length k */
  int codesize[257];		/* codesize[k] = code length of symbol k */
  int others[257];		/* next symbol in current branch of tree */
  int c1, c2;
  int p, i, j;
  long v;

  /* This algorithm is explained in section K.2 of the JPEG standard */

  MEMZERO(bits, SIZEOF(bits));
  MEMZERO(codesize, SIZEOF(codesize));
  for (i = 0; i < 257; i++)
    others[i] = -1;		/* init links to empty */
  
  freq[256] = 1;		/* make sure there is a nonzero count */
  /* Including the pseudo-symbol 256 in the Huffman procedure guarantees
   * that no real symbol is given code-value of all ones, because 256
   * will be placed in the largest codeword category.
   */

  /* Huffman's basic algorithm to assign optimal code lengths to symbols */

  for (;;) {
    /* Find the smallest nonzero frequency, set c1 = its symbol */
    /* In case of ties, take the larger symbol number */
    c1 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v) {
	v = freq[i];
	c1 = i;
      }
    }

    /* Find the next smallest nonzero frequency, set c2 = its symbol */
    /* In case of ties, take the larger symbol number */
    c2 = -1;
    v = 1000000000L;
    for (i = 0; i <= 256; i++) {
      if (freq[i] && freq[i] <= v && i != c1) {
	v = freq[i];
	c2 = i;
      }
    }

    /* Done if we've merged everything into one frequency */
    if (c2 < 0)
      break;
    
    /* Else merge the two counts/trees */
    freq[c1] += freq[c2];
    freq[c2] = 0;

    /* Increment the codesize of everything in c1's tree branch */
    codesize[c1]++;
    while (others[c1] >= 0) {
      c1 = others[c1];
      codesize[c1]++;
    }
    
    others[c1] = c2;		/* chain c2 onto c1's tree branch */
    
    /* Increment the codesize of everything in c2's tree branch */
    codesize[c2]++;
    while (others[c2] >= 0) {
      c2 = others[c2];
      codesize[c2]++;
    }
  }

  /* Now count the number of symbols of each code length */
  for (i = 0; i <= 256; i++) {
    if (codesize[i]) {
      /* The JPEG standard seems to think that this can't happen, */
      /* but I'm paranoid... */
      if (codesize[i] > MAX_CLEN)
	ERREXIT(cinfo, JERR_HUFF_CLEN_OVERFLOW);

      bits[codesize[i]]++;
    }
  }

  /* JPEG doesn't allow symbols with code lengths over 16 bits, so if the pure
   * Huffman procedure assigned any such lengths, we must adjust the coding.
   * Here is what the JPEG spec says about how this next bit works:
   * Since symbols are paired for the longest Huffman code, the symbols are
   * removed from this length category two at a time.  The prefix for the pair
   * (which is one bit shorter) is allocated to one of the pair; then,
   * skipping the BITS entry for that prefix length, a code word from the next
   * shortest nonzero BITS entry is converted into a prefix for two code words
   * one bit longer.
   */
  
  for (i = MAX_CLEN; i > 16; i--) {
    while (bits[i] > 0) {
      j = i - 2;		/* find length of new prefix to be used */
      while (bits[j] == 0)
	j--;
      
      bits[i] -= 2;		/* remove two symbols */
      bits[i-1]++;		/* one goes in this length */
      bits[j+1] += 2;		/* two new symbols in this length */
      bits[j]--;		/* symbol of this length is now a prefix */
    }
  }

  /* Remove the count for the pseudo-symbol 256 from the largest codelength */
  while (bits[i] == 0)		/* find largest codelength still in use */
    i--;
  bits[i]--;
  
  /* Return final symbol counts (only for lengths 0..16) */
  MEMCOPY(htbl->bits, bits, SIZEOF(htbl->bits));
  
  /* Return a list of the symbols sorted by code length */
  /* It's not real clear to me why we don't need to consider the codelength
   * changes made above, but the JPEG spec seems to think this works.
   */
  p = 0;
  for (i = 1; i <= MAX_CLEN; i++) {
    for (j = 0; j <= 255; j++) {
      if (codesize[j] == i) {
	htbl->huffval[p] = (UINT8) j;
	p++;
      }
    }
  }

  /* Set sent_table FALSE so updated table will be written to JPEG file. */
  htbl->sent_table = FALSE;
}


/*
 * Finish up a statistics-gathering pass and create the new Huffman tables.
 */

METHODDEF(void)
finish_pass_gather (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;
  JHUFF_TBL **htblptr;
  boolean did_dc[NUM_HUFF_TBLS];
  boolean did_ac[NUM_HUFF_TBLS];

  /* It's important not to apply jpeg_gen_optimal_table more than once
   * per table, because it clobbers the input frequency counts!
   */
  MEMZERO(did_dc, SIZEOF(did_dc));
  MEMZERO(did_ac, SIZEOF(did_ac));

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    if (! did_dc[dctbl]) {
      htblptr = & cinfo->dc_huff_tbl_ptrs[dctbl];
      if (*htblptr == NULL)
	*htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
      jpeg_gen_optimal_table(cinfo, *htblptr, entropy->dc_count_ptrs[dctbl]);
      did_dc[dctbl] = TRUE;
    }
    if (! did_ac[actbl]) {
      htblptr = & cinfo->ac_huff_tbl_ptrs[actbl];
      if (*htblptr == NULL)
	*htblptr = jpeg_alloc_huff_table((j_common_ptr) cinfo);
      jpeg_gen_optimal_table(cinfo, *htblptr, entropy->ac_count_ptrs[actbl]);
      did_ac[actbl] = TRUE;
    }
  }
}


#endif /* ENTROPY_OPT_SUPPORTED */


/*
 * Module initialization routine for Huffman entropy encoding.
 */

GLOBAL(void)
jinit_huff_encoder (j_compress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_encoder));
  cinfo->entropy = (struct jpeg_entropy_encoder *) entropy;
  entropy->pub.start_pass = start_pass_huff;

  /* Mark tables unallocated */
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
#ifdef ENTROPY_OPT_SUPPORTED
    entropy->dc_count_ptrs[i] = entropy->ac_count_ptrs[i] = NULL;
#endif
  }
}

//mark buxton's new emit_bits:

static unsigned int onlynbits[] = {

	0x00000000,	0x00000001,	0x00000003,	0x00000007,	0x000000F,	0x0000001F,
	0x0000003F,	0x0000007F,	0x000000FF,	0x000001FF,	0x000003FF,	0x000007FF,
	0x00000FFF,	0x00001FFF,	0x00003FFF,	0x00007FFF,	0x0000FFFF,	0x0001FFFF,
	0x0003FFFF,	0x0007FFFF,	0x000FFFFF,	0x001FFFFF,	0x003FFFFF,	0x007FFFFF,
	0x00FFFFFF,	0x01FFFFFF,	0x03FFFFFF,	0x07FFFFFF,	0x0FFFFFFF,	0x1FFFFFFF,
	0x3FFFFFFF,	0x7FFFFFFF,	0xFFFFFFFF
};


//
// Need to add #ifdef for Alpha port
//
#if defined (_X86_)

GLOBAL(boolean)
emit_bits_fast (working_state * state, unsigned int code, int bsize, int only1){
// Emit some bits; return TRUE if successful, FALSE if must suspend 
  // This routine is heavily used, so it's worth coding tightly. 
  //unsigned int put_buffer = code;

//  int put_bits = state->put_bits;
  unsigned c;

  __asm{	  
	  mov edx,64
	  mov esi,[put_bits]

	  add esi,dword ptr[bsize]
      
	  mov [put_bits],esi
	  sub  edx,esi

	  mov ebx,[free_in_buffer]
      
	  movd mm3,edx
	  mov edx,[next_output_byte]
	  

	  movd mm0,[code]
	  
	  movq mm7,[put_buffer_64]
	  psllq mm0,mm3		//put_buffer <<= 64-put_bits;
  	  	  
	  por mm7,mm0
   	  cmp [only1],0

	  movq mm0,mm7 
	  jne got_FF

	  cmp ebx,8
	  jng got_FF

	  cmp esi,32
	  jng buffer_not_full
	  
//	  test [next_output_byte],0x3
//	  jnz byte_write
	  //test to see if the data is on a 4-byte boundary.  If not, don't use the 
	  //integer write.  
//integer_write:  //Write 32 bits.
	  
	 movq mm1,mm5
	 psrlq mm0,32

	 pcmpeqb mm1,mm0
	 sub ebx,4

	 movd eax,mm1
	 movq mm2,mm0											//  | - | - | - | - | D | C | B | A | MM2
	 
	 test eax,eax
	 jne got_FF
 //!%$@ Stupid big-endian data %$^@#$%@
	 psrlq mm0,8											//  | - | - | - | - | - | D | C | B | MM0
	 mov [free_in_buffer],ebx

	 punpcklbw mm0,mm2										//  | - | - | C | D | B | C | A | B | MM0
	 add edx,4	

	 pslld mm0,16											//  | C | D | B | C | A | B | - | - | MM0
	 sub esi,32

	 psrad mm0,16	//have to use this pair because packssdw expects 16-byte _signed_ data.
 	 psllq mm7,32
	 
	 packssdw mm0,mm0										//  | - | - | - | - | C | D | A | B | MM0
	 mov [put_bits],esi

	 movq mm2,mm0											//  | - | - | - | - | C | D | A | B | MM2
	 movq [put_buffer_64],mm7

	 psrlq mm0,16											//  | - | - | - | - | - | - | C | D | MM0
     mov [next_output_byte],edx

	 punpcklwd mm0,mm2										//  | - | - | - | - | C | B | C | D | MM0

	 movd [edx-4],mm0
	 nop

	 }
	return TRUE;

got_FF:	//only if an FF was returned.
	while (put_bits >=8) {

		__asm{
			movq mm4,mm7
			psrlq mm7,56

			movd [c],mm7
			psllq mm4,8
			
			movq mm7,mm4
			sub [put_bits],8
		}

		emit_byte(state, c, return FALSE);
		//emit_byte_fast(c);
		if (c == 0xFF) emit_byte(state, 0, return FALSE);  
		//if (c==0xFF) emit_byte_fast(0);
		//put_bits -= 8;
	}

buffer_not_full:
  __asm  movq [put_buffer_64],mm7
  
  return TRUE;

}


/* Encode a single block's worth of coefficients */

GLOBAL(boolean)
encode_one_block_fast (working_state * state, JCOEFPTR block, int last_dc_val,
		  c_derived_tbl *dctbl, c_derived_tbl *actbl)
{
  JCOEF temp ;
  int nbits;
  int k, i ,j;
  /*unsigned <-Buxton BUG??*/ int l;
  int	lastzeros = 0, numElements = 0 ;
  
  short dummy_outblock[192+4] ;	// 64 values, 64 corresponding zero & bit counts 
  short *outblock;
  outblock= (short *)(((unsigned int)dummy_outblock+7)&0xFFFFFFF8);


  // Encode the DC coefficient difference per section F.1.2.1 
  //starttimer();

  temp = block[0] ;
  block[0] = (JCOEF) (block[0] - last_dc_val) ;
  countZeros((int *)jpeg_natural_order, block, outblock, &lastzeros, &numElements) ;
  //formatting for frequently used MMX registers in emit_bits_fast
  __asm{
	  //provide two constants:		mm5 = 0xFFFF|FFFF|FFFF|FFFF
	  //							mm6 = 0x0000|FFFF|0000|FFFF
	  pcmpeqb mm5,mm5
	  pxor mm0,mm0  
	  movq mm6,mm5
	  punpcklwd mm6,mm0
	
  }
  if(block[0] == 0) {
     // Emit the Huffman-coded symbol for the number of bits 
	  //changed to only1 - dshade
     if (! emit_bits_fast(state, dctbl->ehufco[0], dctbl->ehufsi[0],1)) return FALSE;

	outblock[64] -- ;  
	k=0;
  } else {
	  nbits = outblock[128] ;
	  k=1;
	  //changed to only1 - dshade
	  if (!emit_bits_fast(state,dctbl->ehufco[nbits]<<nbits | ((unsigned int) outblock[0]&onlynbits[nbits]),nbits + dctbl->ehufsi[nbits],1)) return FALSE;
  }
  block[0] = temp ; // get the original value of block[0] back in. 
  
  if( (numElements == 0) || (numElements == 1) ) {
	  lastzeros = 63 ; // DC element handled outside of the AC loop below 
  }

  
  
  // Encode the AC coefficients per section F.1.2.2 
  
  for (; k < numElements; k++) {

	  l = outblock[64+k];//<<4;
	  //store frequently used lookups:
	  j = outblock[128+k];
      // if run length > 15, must emit special run-length-16 codes (0xF0) 
      while (l > 15/*240*/) {
		// changed to only1 - dshade
		  if (! emit_bits_fast(state, actbl->ehufco[0xF0], actbl->ehufsi[0xF0],1))
		    return FALSE;
		l -= 16; //256;
      }

//	  if (l	< 0) 	  // dshade
//		  l = 0;	  // dshade

	  l = l << 4;

	  i = l + j;

	  //hufandval = actbl->ehufco[i]<<j | ((unsigned int) outblock[k]&onlynbits[j]);
	  //hufandvallen = j + actbl->ehufsi[i];

	  //changed to only1 - dshade
	  if (!emit_bits_fast(state,actbl->ehufco[i]<<j | ((unsigned int) outblock[k]&onlynbits[j]),j + actbl->ehufsi[i],1)) return FALSE;

  }

  // If the last coef(s) were zero, emit an end-of-block code 
  if (lastzeros > 0)
  {	//changed to only1 - dshade
    if (! emit_bits_fast(state, actbl->ehufco[0], actbl->ehufsi[0],1))
		return FALSE;
  }

//cumulative_time += stoptimer();

  _asm emms    // dshade

  return TRUE;

}

//used above as:
//countZeros((int *)jpeg_natural_order, block, outblock, &lastzeros, &numElements) ;
__int32 jmpswitch;

void countZeros(
	int *dwindexBlock,
	short *dwcoefBlock,
	short *dwoutBlock,
	int *dwlastZeros, 
	int *dwnumElem
){

static __int64 const_1   = 0x0001000100010001;
static __int64 const_2   = 0x0002000200020002;
static __int64 const_3   = 0x0003000300030003;
static __int64 const_4   = 0x0004000400040004;
static __int64 const_8   = 0x0008000800080008;
static __int64 const_15  = 0x000f000f000f000f;
static __int64 const_255 = 0x00ff00ff00ff00ff;

//#define sizLOCALS 8

//_countZeros proc USES eax ebx ecx edx esi edi 

// Move all paramters to be based on esp
// Must REMEMBER NOT to use the stack for any push/pops

// the following are the parameters passed into the routine.
//#define dwindexBlock	dword ptr [esp+32+sizLOCALS]  // 32 bit elements
//#define dwcoefBlock		dword ptr [esp+36+sizLOCALS]  // 16 bit elements
//#define dwoutBlock		dword ptr [esp+40+sizLOCALS]  // 16 bit elements
//#define dwlastZeros		dword ptr [esp+44+sizLOCALS]  // address of a 32 bit element
//#define dwnumElem		dword ptr [esp+48+sizLOCALS]  // number of non-zero elements
// dwlastZeros stores the number of trailing zero values.

//;;;;; LOCALS  :;;;;;;;;;;;;;;;
__int32 locdwoutBlock;
__int32 locdwZeroCount;
__int32 loopctr;
// these are used as scratchpad registers.
// right now a new local has been added on an as needed
// basis. There's potential for reducing the number of locals.
//#define locdwoutBlock	dword ptr [esp+0]
//#define locdwZeroCount	dword ptr [esp+4]

//;;;;; END OF LOCALS  ;;;;;;;;;;;;;;;
__asm{
//sub	esp, sizLOCALS


mov	esi, dwindexBlock	; load the input array pointer
mov	edi, dwcoefBlock

mov	eax, dwoutBlock
nop		//************************;;

mov	locdwoutBlock, eax
nop		//************************;;

mov		dword ptr[loopctr], 10h	// loop count of 16 : four elements handled per loop
mov		locdwZeroCount, 0h	// initialize zero counter to 0

CountZeroLoop:
// align the zigzag elements of inblock into MMX register, four words 
// at a time.

// get index for next four elements in coeff array from the zigzag array
mov	eax, [esi]			
mov	ebx, [esi+4]

mov	ecx, [esi+8]
mov	edx, [esi+12]

// get the next four coeff. words

mov	eax, [edi+2*eax]
mov	ebx, [edi+2*ebx]

mov	ecx, [edi+2*ecx]
mov	edx, [edi+2*edx]

// pack first two words in eax (first word in LS 16 bits)
shl	ebx, 16
and	eax, 0ffffh

// pack next two words in ecx (third word in LS 16 bits)
shl	edx, 16
and	ecx, 0ffffh

or	eax, ebx
or	ecx, edx

mov	ebx, eax
or	eax, ecx	// check to see if all 4 elems. are zero

cmp	eax, 0h		
jz	caseAllZeros

movd	mm0, ebx	// move LS two words into mm0
pxor	mm2, mm2	// initialize mm2 to zero

movd	mm1, ecx	// move MS two words into mm1
nop		//************************;;

pcmpeqw	mm0, mm2
pcmpeqw	mm1, mm2

movq mm3,mm0
por	mm0,mm1

movd eax,mm0
pcmpeqw mm2,mm2

cmp eax,0h
jz  caseNoZeros

movd eax,mm3
pandn mm1,mm2

movd edx,mm1
not eax

and eax,0x00020001
and edx,0x00080004

or eax,edx
add esi,16

mov edx,eax
shr eax,16

or eax,edx
mov	edx, locdwZeroCount

and eax,0xFFFF
nop

dec eax

lea eax,[JmpTable+eax*8]
jmp eax


JmpTable:
jmp case0001
nop
nop
nop
jmp case0010
nop
nop
nop
jmp case0011
nop
nop
nop
jmp case0100
nop
nop
nop
jmp case0101
nop
nop
nop
jmp case0110
nop
nop
nop
jmp case0111
nop
nop
nop
jmp case1000
nop
nop
nop
jmp case1001
nop
nop
nop
jmp case1010
nop
nop
nop
jmp case1011
nop
nop
nop
jmp case1100
nop
nop
nop
jmp case1101
nop
nop
nop
jmp case1110
nop
nop
nop
jmp caseNoZeros


caseAllZeros:

add		locdwZeroCount, 4
add		esi, 16

dec		[loopctr]				// decrement loop counter
jnz		CountZeroLoop

jmp		AllDone



caseNoZeros:

mov		eax, locdwoutBlock
mov		edx, locdwZeroCount

mov		locdwZeroCount, 0h
add		esi, 16		// esi points to a 32 bit quantity

mov	[eax], ebx		// store the LS two words
mov	[eax+4], ecx	// store the MS two words

add		locdwoutBlock, 8
mov		[eax+128], edx 

mov		dword ptr [eax+132], 0
nop		//************************;;

dec		[loopctr]
jnz		CountZeroLoop

jmp		AllDone		


// case0000:
// this case is taken care of by caseAllZero

case0001:

mov		eax, locdwoutBlock
mov		locdwZeroCount, 3

mov		[eax], bx	// store the LS word
mov		[eax+128], dx	//; store the corresponding zero count 

add		locdwoutBlock, 2
nop		//************************;;

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		

case0010:

mov		eax, locdwoutBlock
mov		locdwZeroCount, 2

shr		ebx, 16		// get the MS word into LS 16 bits
add		edx, 1		// increment zero count

mov		[eax+128], dx	// store the corresponding zero count 
nop		//************************;;

mov		[eax], bx		// store the LS word
add		locdwoutBlock, 2

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		

case0011:

mov		eax, locdwoutBlock
mov		locdwZeroCount, 2

mov		[eax], ebx		// store the LS word
mov		[eax+128], edx	// store the corresponding zero count 

add		locdwoutBlock, 4
nop		//************************;;

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case0100:

mov		eax, locdwoutBlock
add		edx, 2

mov		[eax], cx		// store the LS word within MS DWORD
mov		[eax+128], dx	// store the corresponding zero count 

add		locdwoutBlock, 2
mov		locdwZeroCount, 1

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case0101:

mov		eax, locdwoutBlock
or		edx, 10000h		// zero count is 1 for second word

mov		[eax], bx		// store the LS word within LS DWORD
mov		[eax+2], cx		// store the LS word within MS DWORD

mov		[eax+128], edx	// store the corresponding zero count 
nop		//************************;;

add		locdwoutBlock, 4
mov		locdwZeroCount, 1

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case0110:

mov		eax, locdwoutBlock
add		edx, 1			// zero count is incremented for first word

shr	ebx, 16			// move the word to be written into LS 16 bits
mov		[eax], bx		// store the LS word within LS DWORD

mov		[eax+2], cx		// store the LS word within MS DWORD
add		locdwoutBlock, 4

mov		[eax+128], edx	// store the corresponding zero count 
mov		locdwZeroCount, 1

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case0111:

mov		eax, locdwoutBlock
nop

add		locdwoutBlock, 6
mov		locdwZeroCount, 1

mov		[eax], ebx		// store the LS word within LS DWORD
mov		[eax+4], cx		// store the LS word within MS DWORD

mov		[eax+128], edx	// store the corresponding zero count 
mov		word ptr [eax+132], 0	// zerocount of 0 for third word

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case1000:

mov		eax, locdwoutBlock
add		edx, 3


shr	ecx, 16
nop		//************************;;

mov		[eax], cx		// store the LS word within MS DWORD
mov		[eax+128], dx	// store the corresponding zero count 

add		locdwoutBlock, 2
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case1001:

mov		eax, locdwoutBlock
shr		ecx, 16		// word 3 into LS bits

or		edx, 00020000h	//// zero count of two for MS word
mov		[eax], bx		// store the LS word within MS DWORD

mov		[eax+2], cx		// store the LS word within MS DWORD
add		locdwoutBlock, 4

mov		[eax+128], edx	// store the corresponding zero count 
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case1010:

mov		eax, locdwoutBlock
nop

add		edx, 1		// increment zero count
shr	ecx, 16		// word 3 into LS bits

shr	ebx, 16		// word 2 into LS bits
or		edx, 00010000h	// zero count of two for MS word

mov		[eax], bx		// store the LS word within MS DWORD
mov		[eax+2], cx		// store the LS word within MS DWORD

mov		[eax+128], edx	// store the corresponding zero count 
//add		esi, 16		// esi points to a 32 bit quantity

add		locdwoutBlock, 4
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		



case1011:


mov		eax, locdwoutBlock
shr	ecx, 16		// word 3 into LS bits

mov		[eax], ebx		// store the LS DWORD
mov		[eax+4], cx		// store the LS word within MS DWORD

mov		[eax+128], edx	// store the corresponding zero count 
mov		word ptr [eax+132], 1

add		locdwoutBlock, 6
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		



case1100:

mov		eax, locdwoutBlock
add		edx, 2		// add 2 to zeroc count

mov		[eax], ecx		// store the LS DWORD
mov		[eax+128], edx	// store the corresponding zero count 

add		locdwoutBlock, 4
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case1101:

mov		eax, locdwoutBlock
nop		////************************////

mov		[eax], bx		// store the LS DWORD
mov		[eax+128], dx	// store the corresponding zero count 

mov		[eax+2], ecx
mov		dword ptr [eax+130], 1	// zero count of 1 for 2nd word

add		locdwoutBlock, 6
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		


case1110:

mov		eax, locdwoutBlock
shr	ebx, 16		// get word 1 in LS word

add		dx, 1		// add 1 to zerocount
nop		////************************////

mov		[eax], bx		// store the LS DWORD
mov		[eax+128], dx	// store the corresponding zero count 

mov		[eax+2], ecx
mov		dword ptr [eax+130], 0	// zero count of 0 for 2nd & 3rd word

add		locdwoutBlock, 6
mov		locdwZeroCount, 0

dec [loopctr]
jnz		CountZeroLoop

jmp		AllDone		



// case1111:
// this case is handled by caseNoZeros


////////////////////////////////////////////////////////////////////////////////////////////////////////////

AllDone:

// at this point all zero counting is done
// now get the number of non-zero elements and round to the
// nearest multiple of four in the +infinity direction

mov	eax, locdwoutBlock	

sub	eax, dwoutBlock	// how many non-zero elements did you write
mov	ebx, dwnumElem	// address to store number of non-zero elements

//*** small bug fix by dshade 4/8/97 to eliminate case where loop count went negative below
//*** all changes have //*** by them
mov	edx, eax		//*** make a copy before shifting

shr	eax, 1			// a/c for each element being 2 bytes

//// round the number of elements to nearest multiple of four
mov	[ebx], eax			
//add	eax, 4			
add	edx, 7				//*** add get an even multiple of 8

and	edx, 0x1f8			//*** edx holds the number of writes rounded up to 8
//and	eax, 0fch		// get the LS 2 bits to be zero
mov	esi, dwoutBlock

//// This loop should count the number of bits needed to represent
//// the input value. It handles four inputs in each iteration

CountBitsLoop:

movq	mm0, [esi]	// get the first four input data
pxor	mm7, mm7	// clear mm7

movq	mm1, mm0
pcmpgtw	mm0, mm7	// is input number positive ?

movq	mm3, mm1
pand	mm1, mm0	// original number if greater than 0, else 0

psubw	mm7, mm3	// 0 minus input number
movq	mm2, mm0

psubw	mm3, qword ptr [const_1]		// decrement input
pandn	mm0, mm7	// if number < 0 then (- input), else 0

por		mm0, mm1	// abs(input)
pandn	mm2, mm3	// input minus 1 if input < 0, else 0

por		mm2, mm1	// same as input, if input positive, else input minus 1. 
nop		////************************////

movq	mm3, mm0	
movq	mm1, mm0

pcmpgtw	mm1, qword ptr [const_255]	// split the 16bit value across bit 8 (256)
psrlw	mm0, 8		// get MS 8 bits into LS byte

movq	[esi], mm2	// store input (if it's +ve), else store 1s comp. of input
movq	mm2, mm1

pand	mm1, qword ptr [const_8]	// value > 255 implies need for 8 bits, else zero
pand	mm0, mm2	// sift the ones greater than 255, else zeros

pandn	mm2, mm3	// sift the ones less than 256, else zeros
movq	mm5, mm0

por	mm0, mm2		// get reqd. portions of data in LS 8 bits
por	mm5, mm2		// copy of above instruction

pcmpgtw	mm5, qword ptr [const_15]
movq	mm4, mm0

movq	mm3, mm5
psrlw	mm0, 4

pand	mm5, qword ptr [const_4]
pand	mm0, mm3

movq	mm2, mm0
pandn	mm3, mm4

por		mm0, mm3		
por		mm2, mm3

pcmpgtw	mm2, qword ptr [const_3]
movq	mm4, mm0

movq	mm3, mm2
psrlw	mm0, 2

pand	mm2, qword ptr [const_2]
pand	mm0, mm3

movq	mm6, mm0
pandn	mm3, mm4

por		mm0, mm3		
por		mm6, mm3

pcmpgtw	mm6, qword ptr [const_1]
movq	mm4, mm0

movq	mm3, mm6
psrlw	mm0, 1

pand	mm6, qword ptr [const_1]
pand	mm0, mm3

por		mm1, mm5					
pandn	mm3, mm4

por		mm6, mm2
por		mm0, mm3		

por		mm1, mm6
nop		////************************////

paddw	mm0, mm1
nop		////************************////

movq	[esi+256], mm0
nop		////************************////

add		esi, 8
nop		////************************////

//sub		eax, 4
sub	edx, 8		//*** decrement byte count
jg		CountBitsLoop	   //*** changed loop conditions to break out if not positive




mov	eax, locdwZeroCount
mov	ebx, dwlastZeros

//emms
//nop		////************************////

mov	[ebx], eax
//add	esp, sizLOCALS


}
return;
}

#endif // #define (_X86_)
