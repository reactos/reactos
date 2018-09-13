/*
 * jdhuff.c
 *
 * Copyright (C) 1991-1996, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 *
 * Much of the complexity here has to do with supporting input suspension.
 * If the data source module demands suspension, we want to be able to back
 * up to the start of the current MCU.  To do this, we copy state variables
 * into local working storage, and update them back to the permanent
 * storage only upon successful completion of an MCU.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jdhuff.h"		/* Declarations shared with jdphuff.c */


/*
 * Expanded entropy decoder object for Huffman decoding.
 *
 * The savable_state subrecord contains fields that change within an MCU,
 * but must not be updated permanently until we complete the MCU.
 */

typedef struct {
  int last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each component */
} savable_state;

/* This macro is to work around compilers with missing or broken
 * structure assignment.  You'll need to fix this code if you have
 * such a compiler and you change MAX_COMPS_IN_SCAN.
 */

#ifndef NO_STRUCT_ASSIGN
#define ASSIGN_STATE(dest,src)  ((dest) = (src))
#else
#if MAX_COMPS_IN_SCAN == 4
#define ASSIGN_STATE(dest,src)  \
	((dest).last_dc_val[0] = (src).last_dc_val[0], \
	 (dest).last_dc_val[1] = (src).last_dc_val[1], \
	 (dest).last_dc_val[2] = (src).last_dc_val[2], \
	 (dest).last_dc_val[3] = (src).last_dc_val[3])
#endif
#endif


typedef struct {
  struct jpeg_entropy_decoder pub; /* public fields */

  /* These fields are loaded into local variables at start of each MCU.
   * In case of suspension, we exit WITHOUT updating them.
   */
  bitread_perm_state bitstate;	/* Bit buffer at start of MCU */
  savable_state saved;		/* Other state at start of MCU */

  /* These fields are NOT loaded into local working state. */
  unsigned int restarts_to_go;	/* MCUs left in this restart interval */

  /* Pointers to derived tables (these workspaces have image lifespan) */
  d_derived_tbl * dc_derived_tbls[NUM_HUFF_TBLS];
  d_derived_tbl * ac_derived_tbls[NUM_HUFF_TBLS];
} huff_entropy_decoder;

typedef huff_entropy_decoder * huff_entropy_ptr;


/*
 * Initialize for a Huffman-compressed scan.
 */

METHODDEF(void)
start_pass_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci, dctbl, actbl;
  jpeg_component_info * compptr;

  /* Check that the scan parameters Ss, Se, Ah/Al are OK for sequential JPEG.
   * This ought to be an error condition, but we make it a warning because
   * there are some baseline files out there with all zeroes in these bytes.
   */
  if (cinfo->Ss != 0 || cinfo->Se != DCTSIZE2-1 ||
      cinfo->Ah != 0 || cinfo->Al != 0)
    WARNMS(cinfo, JWRN_NOT_SEQUENTIAL);

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    dctbl = compptr->dc_tbl_no;
    actbl = compptr->ac_tbl_no;
    /* Make sure requested tables are present */
    if (dctbl < 0 || dctbl >= NUM_HUFF_TBLS ||
	cinfo->dc_huff_tbl_ptrs[dctbl] == NULL)
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, dctbl);
    if (actbl < 0 || actbl >= NUM_HUFF_TBLS ||
	cinfo->ac_huff_tbl_ptrs[actbl] == NULL)
      ERREXIT1(cinfo, JERR_NO_HUFF_TABLE, actbl);
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for a table, but it's not expensive */
    jpeg_make_d_derived_tbl(cinfo, cinfo->dc_huff_tbl_ptrs[dctbl],
			    & entropy->dc_derived_tbls[dctbl]);
    jpeg_make_d_derived_tbl(cinfo, cinfo->ac_huff_tbl_ptrs[actbl],
			    & entropy->ac_derived_tbls[actbl]);
    /* Initialize DC predictions to 0 */
    entropy->saved.last_dc_val[ci] = 0;
  }

  /* Initialize bitread state variables */
  entropy->bitstate.bits_left = 0;
  entropy->bitstate.get_buffer_64 = 0; 
  entropy->bitstate.get_buffer = 0; /* unnecessary, but keeps Purify quiet */
  entropy->bitstate.printed_eod = FALSE;

  /* Initialize restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;
}


/*
 * Compute the derived values for a Huffman table.
 * Note this is also used by jdphuff.c.
 */

GLOBAL(void)
jpeg_make_d_derived_tbl (j_decompress_ptr cinfo, JHUFF_TBL * htbl,
			 d_derived_tbl ** pdtbl)
{
  d_derived_tbl *dtbl;
  int p, i, l, si;
  int lookbits, ctr;
  char huffsize[257];
  unsigned int huffcode[257];
  unsigned int code;

  /* Allocate a workspace if we haven't already done so. */
  if (*pdtbl == NULL)
    *pdtbl = (d_derived_tbl *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(d_derived_tbl));
  dtbl = *pdtbl;
  dtbl->pub = htbl;		/* fill in back link */
  
  /* Figure C.1: make table of Huffman code length for each symbol */
  /* Note that this is in code-length order. */

  p = 0;
  for (l = 1; l <= 16; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++)
      huffsize[p++] = (char) l;
  }
  huffsize[p] = 0;
  
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

  /* Figure F.15: generate decoding tables for bit-sequential decoding */

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
      dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
      p += htbl->bits[l];
      dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
    } else {
      dtbl->maxcode[l] = -1;	/* -1 if no codes of this length */
    }
  }
  dtbl->maxcode[17] = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */

  /* Compute lookahead tables to speed up decoding.
   * First we set all the table entries to 0, indicating "too long";
   * then we iterate through the Huffman codes that are short enough and
   * fill in all the entries that correspond to bit sequences starting
   * with that code.
   */

  MEMZERO(dtbl->look_nbits, SIZEOF(dtbl->look_nbits));

  p = 0;
  for (l = 1; l <= HUFF_LOOKAHEAD; l++) {
    for (i = 1; i <= (int) htbl->bits[l]; i++, p++) {
      /* l = current code's length, p = its index in huffcode[] & huffval[]. */
      /* Generate left-justified code followed by all possible bit sequences */
      lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
      for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) {
	dtbl->look_nbits[lookbits] = l;
	dtbl->look_sym[lookbits] = htbl->huffval[p];
	lookbits++;
      }
    }
  }
}


/*
 * Out-of-line code for bit fetching (shared with jdphuff.c).
 * See jdhuff.h for info about usage.
 * Note: current values of get_buffer and bits_left are passed as parameters,
 * but are returned in the corresponding fields of the state struct.
 *
 * On most machines MIN_GET_BITS should be 25 to allow the full 32-bit width
 * of get_buffer to be used.  (On machines with wider words, an even larger
 * buffer could be used.)  However, on some machines 32-bit shifts are
 * quite slow and take time proportional to the number of places shifted.
 * (This is true with most PC compilers, for instance.)  In this case it may
 * be a win to set MIN_GET_BITS to the minimum value of 15.  This reduces the
 * average shift distance at the cost of more calls to jpeg_fill_bit_buffer.
 */

#ifdef SLOW_SHIFT_32
#define MIN_GET_BITS  15	/* minimum allowable value */
#else
#define MIN_GET_BITS  (BIT_BUF_SIZE-7)
#endif

// not used in MMX version
GLOBAL(boolean)
jpeg_fill_bit_buffer (bitread_working_state * state,
		      register bit_buf_type get_buffer, register int bits_left,
		      int nbits)
/* Load up the bit buffer to a depth of at least nbits */
{
  /* Copy heavily used state fields into locals (hopefully registers) */
  register const JOCTET * next_input_byte = state->next_input_byte;
  register size_t bytes_in_buffer = state->bytes_in_buffer;
  register int c;

  /* Attempt to load at least MIN_GET_BITS bits into get_buffer. */
  /* (It is assumed that no request will be for more than that many bits.) */

  while (bits_left < MIN_GET_BITS) {
    /* Attempt to read a byte */
    if (state->unread_marker != 0)
      goto no_more_data;	/* can't advance past a marker */

    if (bytes_in_buffer == 0) {
      if (! (*state->cinfo->src->fill_input_buffer) (state->cinfo))
	return FALSE;
      next_input_byte = state->cinfo->src->next_input_byte;
      bytes_in_buffer = state->cinfo->src->bytes_in_buffer;
    }
    bytes_in_buffer--;
    c = GETJOCTET(*next_input_byte++);

    /* If it's 0xFF, check and discard stuffed zero byte */
    if (c == 0xFF) 
	{
      do 
	  {
		  if (bytes_in_buffer == 0) 
		  {
			  if (! (*state->cinfo->src->fill_input_buffer) (state->cinfo))
				  return FALSE;
			  next_input_byte = state->cinfo->src->next_input_byte;
			  bytes_in_buffer = state->cinfo->src->bytes_in_buffer;
		  }
		  bytes_in_buffer--;
		  c = GETJOCTET(*next_input_byte++);
	  } while (c == 0xFF);

      if (c == 0) 
	  {
		  // Found FF/00, which represents an FF data byte 
		  c = 0xFF;
      } 
	  else 
	  {
		  // Oops, it's actually a marker indicating end of compressed data. 
		  // Better put it back for use later 
		  state->unread_marker = c;

no_more_data:
		  // There should be enough bits still left in the data segment; 
		  // if so, just break out of the outer while loop. 
		  if (bits_left >= nbits)
			  break;
			/* Uh-oh.  Report corrupted data to user and stuff zeroes into
			 * the data stream, so that we can produce some kind of image.
			 * Note that this code will be repeated for each byte demanded
			 * for the rest of the segment.  We use a nonvolatile flag to ensure
			 * that only one warning message appears.
			 */
		  if (! *(state->printed_eod_ptr)) 
		  {
			  WARNMS(state->cinfo, JWRN_HIT_MARKER);
			  *(state->printed_eod_ptr) = TRUE;
		  }
		  c = 0;			// insert a zero byte into bit buffer 
      }
    }

    /* OK, load c into get_buffer */
    get_buffer = (get_buffer << 8) | c;
    bits_left += 8;
  }

  /* Unload the local registers */
  state->next_input_byte = next_input_byte;
  state->bytes_in_buffer = bytes_in_buffer;
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  return TRUE;
}


/*
 * Out-of-line code for Huffman code decoding.
 * See jdhuff.h for info about usage.
 */

GLOBAL(int)
jpeg_huff_decode (bitread_working_state * state,
		  register bit_buf_type get_buffer, register int bits_left,
		  d_derived_tbl * htbl, int min_bits)
{
  register int l = min_bits;
  register INT32 code;

  /* HUFF_DECODE has determined that the code is at least min_bits */
  /* bits long, so fetch that many bits in one swoop. */

  CHECK_BIT_BUFFER(*state, l, return -1);
  code = GET_BITS(l);

  /* Collect the rest of the Huffman code one bit at a time. */
  /* This is per Figure F.16 in the JPEG spec. */

  while (code > htbl->maxcode[l]) {
    code <<= 1;
    CHECK_BIT_BUFFER(*state, 1, return -1);
    code |= GET_BITS(1);
    l++;
  }

  /* Unload the local registers */
  state->get_buffer = get_buffer;
  state->bits_left = bits_left;

  /* With garbage input we may reach the sentinel value l = 17. */

  if (l > 16) {
    WARNMS(state->cinfo, JWRN_HUFF_BAD_CODE);
    return 0;			/* fake a zero as the safest result */
  }

  return htbl->pub->huffval[ htbl->valptr[l] +
			    ((int) (code - htbl->mincode[l])) ];
}


/*
 * Figure F.12: extend sign bit.
 * On some machines, a shift and add will be faster than a table lookup.
 */

#ifdef AVOID_TABLES

#define HUFF_EXTEND(x,s)  ((x) < (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

#else

#define HUFF_EXTEND(x,s)  ((x) < extend_test[s] ? (x) + extend_offset[s] : (x))

static const int extend_test[16] =   /* entry n is 2**(n-1) */
  { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };

static const int extend_offset[16] = /* entry n is (-1 << n) + 1 */
  { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
    ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
    ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
    ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

#endif /* AVOID_TABLES */


/*
 * Check for a restart marker & resynchronize decoder.
 * Returns FALSE if must suspend.
 */

LOCAL(boolean)
process_restart (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  int ci;

  /* Throw away any unused bits remaining in bit buffer; */
  /* include any full bytes in next_marker's count of discarded bytes */
  cinfo->marker->discarded_bytes += entropy->bitstate.bits_left / 8;
  entropy->bitstate.bits_left = 0;

  /* Advance past the RSTn marker */
  if (! (*cinfo->marker->read_restart_marker) (cinfo))
    return FALSE;

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    entropy->saved.last_dc_val[ci] = 0;

  /* Reset restart counter */
  entropy->restarts_to_go = cinfo->restart_interval;

  /* Next segment can get another out-of-data warning */
  entropy->bitstate.printed_eod = FALSE;

  return TRUE;
}


/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 *
 * Returns FALSE if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * this module, since we'll just re-assign them on the next call.)
 */

METHODDEF(boolean)
__cdecl decode_mcu (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
  huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
  register int s, k, r;
  int blkn, ci;
  JBLOCKROW block;
  BITREAD_STATE_VARS;
  savable_state state;
  d_derived_tbl * dctbl;
  d_derived_tbl * actbl;
  jpeg_component_info * compptr;

  /* Process restart marker if needed; may have to suspend */
  if (cinfo->restart_interval) {
    if (entropy->restarts_to_go == 0)
      if (! process_restart(cinfo))
	return FALSE;
  }

  /* Load up working state */
  BITREAD_LOAD_STATE(cinfo,entropy->bitstate);
  ASSIGN_STATE(state, entropy->saved);

  /* Outer loop handles each block in the MCU */

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    block = MCU_data[blkn];
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    dctbl = entropy->dc_derived_tbls[compptr->dc_tbl_no];
    actbl = entropy->ac_derived_tbls[compptr->ac_tbl_no];

    /* Decode a single block's worth of coefficients */

    /* Section F.2.2.1: decode the DC coefficient difference */
    HUFF_DECODE(s, br_state, dctbl, return FALSE, label1);
    if (s) {
      CHECK_BIT_BUFFER(br_state, s, return FALSE);
      r = GET_BITS(s);
      s = HUFF_EXTEND(r, s);
    }

    /* Shortcut if component's values are not interesting */
    if (! compptr->component_needed)
      goto skip_ACs;

    /* Convert DC difference to actual value, update last_dc_val */
    s += state.last_dc_val[ci];
    state.last_dc_val[ci] = s;
    /* Output the DC coefficient (assumes jpeg_natural_order[0] = 0) */
    (*block)[0] = (JCOEF) s;

    /* Do we need to decode the AC coefficients for this component? */
    if (compptr->DCT_scaled_size > 1) {

      /* Section F.2.2.2: decode the AC coefficients */
      /* Since zeroes are skipped, output area must be cleared beforehand */
      for (k = 1; k < DCTSIZE2; k++) {
	HUFF_DECODE(s, br_state, actbl, return FALSE, label2);
      
	r = s >> 4;
	s &= 15;

      
	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  r = GET_BITS(s);
	  s = HUFF_EXTEND(r, s);
	  /* Output coefficient in natural (dezigzagged) order.
	   * Note: the extra entries in jpeg_natural_order[] will save us
	   * if k >= DCTSIZE2, which could happen if the data is corrupted.
	   */
	  (*block)[jpeg_natural_order[k]] = (JCOEF) s;
	} else {
	  if (r != 15)
	    break;
	  k += 15;
	}
      }

    } else {
skip_ACs:

      /* Section F.2.2.2: decode the AC coefficients */
      /* In this path we just discard the values */
      for (k = 1; k < DCTSIZE2; k++) {
	HUFF_DECODE(s, br_state, actbl, return FALSE, label3);
      
	r = s >> 4;
	s &= 15;
      
	if (s) {
	  k += r;
	  CHECK_BIT_BUFFER(br_state, s, return FALSE);
	  DROP_BITS(s);
	} else {
	  if (r != 15)
	    break;
	  k += 15;
	}
      }

    }
  }

  /* Completed MCU, so update state */
  BITREAD_SAVE_STATE(cinfo,entropy->bitstate);
  ASSIGN_STATE(entropy->saved, state);

  /* Account for restart interval (no-op if not using restarts) */
  entropy->restarts_to_go--;

  return TRUE;
}

//MMX routines

//new Typedefs necessary for the new decode_mcu_fast to work.
typedef struct jpeg_source_mgr * j_csrc_ptr;
//typedef struct jpeg_err_mgr * j_cerr_ptr;
typedef struct jpeg_error_mgr * j_cerr_ptr;

typedef d_derived_tbl * h_pub_ptr;
/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 * The coefficients are reordered from zigzag order into natural array order,
 * but are not dequantized.
 *
 * The i'th block of the MCU is stored into the block pointed to by
 * MCU_data[i].  WE ASSUME THIS AREA HAS BEEN ZEROED BY THE CALLER.
 * (Wholesale zeroing is usually a little faster than retail...)
 *
 * Returns FALSE if data source requested suspension.  In that case no
 * changes have been made to permanent state.  (Exception: some output
 * coefficients may already have been assigned.  This is harmless for
 * this module, since we'll just re-assign them on the next call.)
 */

const int twoexpnminusone[13] = { 0, 1, 2, 4, 8,16,32,64,128,256,512,1024,2048};
const int oneminustwoexpn[13] = { 0,-1,-3,-7,-15,-31,-63,-127,-255,-511,-1023,-2047};

#ifdef _X86_

METHODDEF(boolean)
__cdecl decode_mcu_fast (j_decompress_ptr cinfo, JBLOCKROW *MCU_data)
{
//	return decode_mcu_inner(cinfo,MCU_data);
//***************************************************************************/
//*
//*                INTEL Corporation Proprietary Information  
//*
//*      
//*                  Copyright (c) 1996 Intel Corporation.
//*                         All rights reserved.
//*
//***************************************************************************/
//			AUTHOR:  Mark  Buxton
/***************************************************************************/
// MMX version of the "Huffman Decoder" within the IJG decompressor code.

// //	MMX Allocation:
//-------------------------------------------------------------
////				XXXX	XXXX  |  XXXX	XXXX
//
//		MM0:	------------     				
//		MM1:             						bit_buffer
//		MM2:             						temp buffer				
//		MM3:             						temp buffer				
//		MM4:    0000    0000     0000   0040		
//		MM5:	------------	  				dctbl
//		MM6:    ------------	  				actbl
//		MM7:    ------------      				temp_buffer
//
//  
//		edi	 -  bits left in the Bit Buffer

//				//routines to modify:  jpeg_huff_decode_fast
//				//					   fill_bit_buffer
//
//
//
// Other available storage locations:
//
//	ebp	 -  state



	//data declaration:

	unsigned char blkn;
	unsigned char nbits;
	JBLOCKROW block;
	huff_entropy_ptr entropy = (huff_entropy_ptr) cinfo->entropy;
	jpeg_component_info * compptr;
	bitread_working_state br_state;
	savable_state state;
	d_derived_tbl * dctbl;
	d_derived_tbl * actbl;
	d_derived_tbl * htbl;
	int ci,temp1;
	int code;
	int min_bits;
	
__asm {
//  // Process restart marker if needed// may have to suspend 
//  if (cinfo->restart_interval) {
		mov  eax,dword ptr [cinfo]
		cmp  (j_decompress_ptr [eax]).restart_interval,1
		jne   Skip_Restart
//if (entropy->restarts_to_go == 0)
		mov  eax,dword ptr [entropy]
		cmp  (dword ptr [eax]).restarts_to_go,0
		jne  Skip_Restart
//if (! process_restart(cinfo))
		mov  eax,dword ptr [cinfo]
		push eax
		call process_restart
		add  esp,4
		test eax,eax
		jne  Skip_Restart

		jmp  Return_Fail

Skip_Restart:

 // // Load up working state 
  
//  br_state.cinfo = cinfop// 
//	br_state.next_input_byte = cinfop->src->next_input_byte// 
//	br_state.bytes_in_buffer = cinfop->src->bytes_in_buffer// 
//	br_state.unread_marker = cinfop->unread_marker// 
//	get_buffer = entropy->bitstate.get_buffer// 
//	bits_left = entropy->bitstate.bits_left// 
//	br_state.printed_eod_ptr = & entropy->bitstate.printed_eod
	
	mov  eax,dword ptr [cinfo]
	mov  dword ptr [br_state.cinfo],eax
	
	
	mov  ebx,(j_decompress_ptr [eax]).unread_marker 
	mov  dword ptr [br_state.unread_marker],ebx 
	
	mov  eax,(j_decompress_ptr [eax]).src
	mov  ebx,(j_csrc_ptr [eax]).next_input_byte
	mov  dword ptr [br_state.next_input_byte],ebx 
	
	mov  ebx,(j_csrc_ptr [eax]).bytes_in_buffer
	mov  dword ptr [br_state.bytes_in_buffer],ebx 
	
	//pxor mm0,mm0
	mov eax,dword ptr[entropy]
	movq mm1,(qword ptr [eax]).bitstate.get_buffer_64
	mov edi,(dword ptr [eax]).bitstate.bits_left 
	
	lea  eax,dword ptr[eax].bitstate.printed_eod
	mov  dword ptr [br_state.printed_eod_ptr],eax
	
	

	mov  ebx,dword ptr [entropy]
	xor  eax,eax
	mov	 eax,(dword ptr [ebx]).saved.last_dc_val[0x00]
	mov  dword ptr [state.last_dc_val+0x00],eax
	mov	 eax,(dword ptr [ebx]).saved.last_dc_val[0x04]
	mov  dword ptr [state.last_dc_val+0x04],eax
	mov	 eax,(dword ptr [ebx]).saved.last_dc_val[0x08]
	mov  dword ptr [state.last_dc_val+0x08],eax
	mov	 eax,(dword ptr [ebx]).saved.last_dc_val[0x0C]
	mov  dword ptr [state.last_dc_val+0x0c],eax

//make sure all variables are initalized.
//see map in header for register usage


 // // Outer loop handles each block in the MCU 

	 //the address of each block is just MCU_data + blkn<<7 (this is MCU_data * 128, right?)
	//ci = cinfo->MCU_membership[blkn];
	//compptr = cinfo->cur_comp_info[ci];
    //dctbl = entropy->dc_derived_tbls[compptr->dc_tbl_no];
    //actbl = entropy->ac_derived_tbls[compptr->ac_tbl_no];
	
	mov  byte ptr [blkn],0
	pxor mm5,mm5
	pxor mm6,mm6
	pxor mm2,mm2
	pxor mm3,mm3
	pxor mm4,mm4
	mov eax,0x40
	movd mm4,eax


}
One_Block_Loop:
	block = MCU_data[blkn];
	ci = cinfo->MCU_membership[blkn];
	compptr = cinfo->cur_comp_info[ci];
	actbl = entropy->ac_derived_tbls[compptr->ac_tbl_no];
	dctbl = entropy->dc_derived_tbls[compptr->dc_tbl_no];
	__asm
	{

	movd mm5,[dctbl]
	movd mm6,[actbl]
	//// Decode a single block's worth of coefficients 

    //// Section F.2.2.1: decode the DC coefficient difference 

//---------------------------------------------------------------------------------
//DC loop section:     there are probably only ~6 to process.
//---------------------------------------------------------------------------------
	
	//set up the MMX registers:
			//move the dctbl pointer into MM6
			//pxor mm6,mm6
			//movd mm6,dword ptr [dctbl]
			//movd eax,mm0

			
			cmp edi,8
			jl Get_n_bits_DC
				//normal path
				//take a peek at the data in get_buffer.
Got_n_bits_DC:		
				movq mm3,mm1	//copy the Bit-Buffer
				psrlq mm1,56	//Extract the MS 8 bits from the Bit Buffer

				movd eax,mm5	//load the DC table pointer
				movd ecx,mm1	//lsb holds the 8 input bits

				movq mm1,mm3
				mov  ebx,(dword ptr[eax+4*ecx]).look_nbits  											
				/*get the number of bits required to represent
				 this Huffman Code (n) .  If the code is > 8 bits, 
				 the table entry is Zero*/
				
				test  ebx,ebx
				je Nineplus_Decode_DC//branch taken 3% of the time.  If code > 8 bits,
									 //get it via a slower metho
				 
				movd mm2,ebx
				sub edi,ebx			//invalidate n bits from the Bit counter

				xor ebx,ebx
				psllq mm1,mm2		//invalidate n bits from the Bit Buffer
				
				mov bl,(byte ptr[eax+ecx]).look_sym //read in the Run Lenth Code (rrrr|ssss); though for the DC coefct's rrrr=0000
				
Got_SymbolDC:							//return point from the slow Huffman decoder routine (for code length > 8 bits)
				cmp edi,ebx				//
				jl not_enough_bits_DC	//If Not enough bits left in the Bit Buffer, Get More

Got_enough_bits_DC:
					pxor mm2,mm2
					sub edi,ebx		//invalidate ssss bits from the Bit counter

					movd mm2,ebx
					movq mm3,mm4	//copy #64 into mm3
					
					psubd mm3,mm2	//now mm3 has 64-ssss
					movq mm0,mm1	//save a copy of the Bit Buffer

					psrlq mm0,mm3	//shift result right
					nop

					psllq mm1,mm2	//Invalidate ssss bits from the Bit Buffer
					movd ecx,mm0		
					

					mov eax,(dword ptr[twoexpnminusone+4*ebx])		//load 2^(ssss-1)
			
					cmp ecx,eax										//
					jge positiv_symDC								// If # < 2^(ssss-1), then # = #+(1-2^ssss)

 						add ecx,(dword ptr [oneminustwoexpn+4*ebx])	//
						nop											/****************************************/
positiv_symDC:
				
				mov eax,dword ptr [compptr] //If !(compptr->compoent_needed), skip AC and DC coefts
				mov edx,1					//initalize loop counter for AC coef't loop

				cmp (dword ptr [eax]).component_needed,0
				je skip_ACs
				//don't skip the AC coefficients.
	
    


		mov eax,[ci]
		mov ebx,[block]									//(*block)[0] = (JCOEF) s//

		add ecx,(dword ptr[state.last_dc_val+eax*4])	//s += state.last_dc_val[ci]//
		pxor mm7,mm7									//cleared for AC_coefficient calculations
		
		mov (dword ptr[state.last_dc_val+eax*4]),ecx	//state.last_dc_val[ci] = s//

		mov word ptr[ebx],cx							//store in (*block)
		mov eax,[compptr]
	
		cmp (dword ptr[eax]).DCT_scaled_size,1	//if (compptr->DCT_scaled_size > 1) {
		jle skip_ACs

		
		


// Section F.2.2.2: decode the AC coefficients 
// Since zeroes are skipped, output area must be cleared beforehand 
//---------------------------------------------------------------------------------
//AC loop section:  Active case.
//---------------------------------------------------------------------------------
Get_AC_DCT_loop:
	
			
			cmp edi,8
			jl Get_8_bits_ac
				//take a peek at the data in get_buffer.
Full_8_bits_AC:		
				movq mm3,mm1								//copy Bit Buffer
				psrlq mm1,56								//load msb from the Bit Buffer

				movd ecx,mm6								//load AC Huffman Table Pointer
				movd eax,mm1								//copy into integer reg. for address calculation
				
				movq mm1,mm3
				mov  ebx,(dword ptr[ecx+4*eax]).look_nbits	//If Huffman symbol is contained within 8 bits fetched,
															//return the actual length of the sequence.  If zero, len>8 bits
				test  ebx,ebx								
				je Nineplus_decode_AC

				sub  edi,ebx								//invalidate n bits from Bit Counter
				movd mm2,ebx				
				
				psllq mm1,mm2								//invalidate n bits from Bit Buffer	
				xor ebx,ebx

				mov bl,(byte ptr[eax+ecx]).look_sym			//load the Huffman Run Length code (rrrr|ssss) for this symbol
				 

Got_SymbolAC:	//return point from the slow Huffman routine
	
				mov eax,ebx	
		
				shr eax,4									//highest nibble is run-length of zeroes (rrrr)
				add edx,eax									//increment AC coefft counter by the # of zeroes.  Assume array is zeroed originally
	
				and ebx,0x000F								//isolate the lowest nibble, the bit-length of the actual coeff't (ssss)
				jz Special_SymbolAC							//a zero for the symbol bit-length indicates it is a special symbol.  Ex:  0xF0, 0x00

			//test to see if # available bits from bit_buffer are less than required to fill the Huffman symbol
			//if insufficient bits, load new bit_buffer through fill_bit_buffer
			
				cmp edi,ebx									//ssss in ebx
				jl Get_n_bits_ac
			
Got_n_bits_AC:
				
				sub edi,ebx									//invalidate ssss bits from the Bit counter
				movd mm2,ebx

				movq mm3,mm4								//copy #64 into mm3
				psubd mm3,mm2								//now mm3 has 64-ssss

				movq mm0,mm1								//save a copy of the Bit Buffer
				psllq mm1,mm2								//Invalidate ssss bits from the Bit Buffer

				psrlq mm0,mm3								//shift result right
				mov eax,(dword ptr[twoexpnminusone+4*ebx])  //load 2^(ssss-1)


				movd ecx,mm0					
				cmp ecx,eax									//
															//
				jge positiv_symAC							// If # < 2^(ssss-1), then # = #+(1-2^ssss)
 				add ecx,(dword ptr [oneminustwoexpn+4*ebx])	//

positiv_symAC:
					//don't modify mm3. It has the actual AC-DCT coefficient.
	  
	  // Output coefficient in natural (dezigzagged) order.
	  // Note: the extra entries in jpeg_natural_order[] will save us
	  //  if the AC coefct index >= DCTSIZE2 (64), which could happen if the data is corrupted.

				
		mov eax, dword ptr(jpeg_natural_order[4*edx])	//(*block)[jpeg_natural_order[k]]=s;
		mov ebx, dword ptr [block]

		mov word ptr([ebx+2*eax]),cx
ContinueAC:
	  inc edx			 //Ac coefct index ++
	  cmp edx,64		 //While (index) < 64
	  jl Get_AC_DCT_loop //imples we are doing the loop 63 times (DC was the first, for 64 total COEFF"s)

Continue_Next_Block_AC:
	  inc byte ptr[blkn] //process the next Coeff. block

	  xor eax,eax
	  mov al,byte ptr[blkn]
	  
	  mov edx,dword ptr[cinfo]
	  cmp eax,(j_decompress_ptr [edx]).blocks_in_MCU	//While [blkn]<= Max number of blocks in MCU:
	  jge COMPLETED_MCU
	  jmp One_Block_Loop

/***************************************************************************************/
/*     DC helper Code																   */
/***************************************************************************************/

Get_n_bits_DC:  xor ebx,ebx//pass nbits in the eax register 
			    call fill_bit_buffer
			  //if zero, it was probably suspended.  Therefore suspend the whole DECODE_MCU
			    test eax,eax
			    je Return_Fail
			  	cmp edi,8
			  	jge Got_n_bits_DC  //probable and predicted path is up.
					mov ebx,1
					jmp Slow_Decode_DC

not_enough_bits_DC:
				    call fill_bit_buffer
					xor ebx,ebx
					mov bl,byte ptr[nbits]
			  	
				    test eax,eax
                    jne Got_enough_bits_DC
					jmp Return_Fail

Nineplus_Decode_DC:
				mov ebx,9
Slow_Decode_DC:		//aka slow_label.  This is the _slow_ huff_decode.
				
				mov eax,[dctbl]
				mov [htbl],eax
				call jpeg_huff_decode_fast //assume ebx holds nbits
				test eax,eax
				jl Return_Fail
				mov ebx,eax
				jmp Got_SymbolDC
				
/***************************************************************************************/
/*     AC helper Code																   */
/***************************************************************************************/

Special_SymbolAC:
	  cmp al,0x0F
	  jne Continue_Next_Block_AC
	  jmp ContinueAC

Get_n_bits_ac:
	  call fill_bit_buffer
	  xor ebx,ebx
	  mov bl,byte ptr[nbits]
	  test eax,eax
      jne Got_n_bits_AC
	  jmp Return_Fail 

Get_8_bits_ac:
	  call fill_bit_buffer
	  test eax,eax
	  je Return_Fail
		
		cmp edi,8
		jge Full_8_bits_AC  //probable and predicted path is up.
			mov ebx,1
			jmp Slow_decode_AC

Nineplus_decode_AC:
				mov ebx,9
Slow_decode_AC:				//The slow Huffman Decode.  Used when the code length is > 8 bits
				mov eax,[actbl]
				mov [htbl],eax
				call jpeg_huff_decode_fast //assume ebx holds nbits
				test eax,eax
				jl Return_Fail
				mov ebx,eax
				jmp Got_SymbolAC


			 //Failure, return from the routine
Return_Fail:		//do not modify any permanent registers
				emms
}
			return FALSE;
__asm {
				




    //} else {

//---------------------------------------------------------------------------------
//AC loop section:  Ignore case.
//---------------------------------------------------------------------------------
skip_ACs:

	      // Section F.2.2.2: decode the AC coefficients 
      // In this path we just discard the values 

Ignore_AC_DCT_loop:

			cmp edi,8
			jl Get_8_bits_acs
				//take a peek at the data in get_buffer.
Full_8_bits_ACs:		
				movq mm3,mm1								//copy Bit Buffer
				psrlq mm1,56								//load msb from the Bit Buffer

				movd ecx,mm6								//load AC Huffman Table Pointer
				movd eax,mm1								//copy into integer reg. for address calculation
				
				movq mm1,mm3
				mov  ebx,(dword ptr[ecx+4*eax]).look_nbits	//If Huffman symbol is contained within 8 bits fetched,
															//return the actual length of the sequence.  If zero, len>8 bits
				test  ebx,ebx								
				je Nineplus_Decode_ACs						//If symbol > 8 bits, fetch the slow way.  Called 3% of the time

				sub  edi,ebx								//invalidate n bits from Bit Counter
				movd mm2,ebx				
				

				psllq mm1,mm2								//invalidate n bits from Bit Buffer	
				xor ebx,ebx

				mov bl,(byte ptr[eax+ecx]).look_sym			//load the Huffman Run Length code (rrrr|ssss) for this symbol

Got_SymbolACs:												//return point from the slow Huffman routine
	
				mov eax,ebx	
		
				shr eax,4				//highest nibble is run-length of zeroes (rrrr)
				add edx,eax				//increment AC coefft counter by the # of zeroes.  Assume array is zeroed originally
	
				and ebx,0x000F			//isolate the lowest nibble, the bit-length of the actual coeff't (ssss)
				jz Special_SymbolACs	//a zero for the symbol bit-length indicates it is a special symbol.  Ex:  0xF0, 0x00
	
				//test to see if # available bits from bit_buffer are less than required to fill the Huffman symbol
				//if insufficient bits, load new bit_buffer through fill_bit_buffer
			
				cmp edi,ebx				//ssss in ebx
				jl Get_n_bits_acs
			
Got_n_bits_acs:
				
					sub edi,ebx		//invalidate ssss bits from the Bit counter
					movd mm2,ebx
					psllq mm1,mm2	//Invalidate ssss bits from the Bit Buffer

Continue_ACs:
			inc edx					//Ac coefct index ++
			cmp edx,64				//While (index) < 64
			jl Ignore_AC_DCT_loop	//imples we are doing the loop 63 times (DC was the first, for 64 total COEFF"s)
			jmp Continue_Next_Block_AC

/***************************************************************************************/
/*     Skipped AC helper Code														   */
/***************************************************************************************/

Special_SymbolACs:
	  cmp al,0x0F
	  jne Continue_Next_Block_AC
	  jmp Continue_ACs

Get_8_bits_acs:
	  call fill_bit_buffer
	  test eax,eax
	  je Return_Fail
		
		cmp edi,8
		jge Full_8_bits_ACs  //probable and predicted path is up.
			mov ebx,1
			jmp Slow_Decode_ACs
Get_n_bits_acs:
	  call fill_bit_buffer
	  xor ebx,ebx
	  mov bl,byte ptr[nbits]
	  test eax,eax
      jne Got_n_bits_acs
	  jmp Return_Fail
			
Nineplus_Decode_ACs:
				mov ebx,9
Slow_Decode_ACs:	//The slow Huffman Decode.  Used when the code length is > 8 bits
				mov eax,[actbl]
				mov [htbl],eax
				call jpeg_huff_decode_fast //assume ebx holds nbits
				test eax,eax
				jl Return_Fail
				mov ebx,eax
				jmp Got_SymbolACs



	  
    //} else {


COMPLETED_MCU:
	  
  // Completed MCU, so update state 

//BITREAD_SAVE_STATE(cinfo,entropy->bitstate)//
//#define BITREAD_SAVE_STATE(cinfop,permstate)

//	cinfo->src->next_input_byte = br_state.next_input_byte
//	cinfo->src->bytes_in_buffer = br_state.bytes_in_buffer
//	cinfo->unread_marker = br_state.unread_marker
//	entropy->bitstate.get_buffer_64 = mm1
//	entropy->bitstate.bits_left = mm0

	mov  eax,dword ptr [br_state.unread_marker]
	mov  ebx,dword ptr [cinfo]
	mov  (j_decompress_ptr [ebx]).unread_marker,eax
	
	mov  eax,dword ptr [br_state.next_input_byte]
	mov  ebx,(j_decompress_ptr [ebx]).src
	mov  (j_csrc_ptr [ebx]).next_input_byte,eax

	mov  eax,dword ptr [br_state.bytes_in_buffer]
	mov  (j_csrc_ptr [ebx]).bytes_in_buffer,eax

	mov  eax,dword ptr [entropy]
	movq  (qword ptr [eax]).bitstate.get_buffer_64,mm1	
	mov  (dword ptr [eax]).bitstate.bits_left,edi


	mov  ebx,dword ptr [entropy]
	mov  eax,dword ptr [state.last_dc_val+0x00]
	mov  (dword ptr [ebx]).saved[0x00],eax
	mov  eax,dword ptr [state.last_dc_val+0x04]
	mov  (dword ptr [ebx]).saved[0x04],eax
	mov  eax,dword ptr [state.last_dc_val+0x08]
	mov  (dword ptr [ebx]).saved[0x08],eax
	mov  eax,dword ptr [state.last_dc_val+0x0C]
	mov  (dword ptr [ebx]).saved[0x0C],eax


  // Account for restart interval (no-op if not using restarts) 
	emms
}
	entropy->restarts_to_go--;
	return TRUE;

	//----------------------------------------------------------------------


/***************************************************************************
fill_bit_buffer:
	Assembly procedure to decode Huffman coefficients longer than 8 bits.
	Also called near the end of a data segment.


	Input Parameters
	al:  minimum number of bits to get

    various MMX registers and local variables must be defined; see 
	_decode_one_mcu_inner above

	This code is called very frequently
****************************************************************************/
__asm {
fill_bit_buffer:

  //use ecx to store bytes_in_buffer
  //use ebx to store next_input_byte
  //edi to store Bit Buffer length

//---------------------------------------------Main Looop----------
  mov dword ptr [temp1],edx
  mov byte ptr[nbits],bl	//number of bits to get
	  //format the bit buffer:  shift to the right by 
	  //64-nbits
  movd mm0,edi
  movq mm7,mm4
  

  mov ecx,dword ptr[br_state.bytes_in_buffer]
  psubd mm7,mm0

  
  psrlq mm1,mm7
  mov ebx,dword ptr[br_state.next_input_byte]
  
  
  //mov eax,8
  //movd mm4,eax
    // Attempt to read a byte */
	cmp [br_state.unread_marker],0
	jne no_more_data

	test ecx,ecx
	je call_load_more_bytes
    
	//determine if there are enough bytes in the i/o buffer
		
continue_reading:	
	//decrement bytes_in_buffer// 
	dec ecx
	js call_load_more_bytes
	//load new data

	xor eax,eax
	mov al,byte ptr[ebx]
	//update next_input_byte pointer
	inc ebx
	cmp eax,0xFF		//compare ebx to FF

	je got_FF

stuff_byte:
	
	psllq mm1,8
	movd mm7,eax

	add edi,8
	por mm1,mm7

	//determine if we've read enough bytes
	cmp edi,56
	jle continue_reading
done_loading:
	//were done loading data.  
	//stuff values for bytes_in_buffer, next_input_byte
	 mov [br_state.next_input_byte],ebx
	 mov [br_state.bytes_in_buffer],ecx
	//finish formatting the bit_register
		
		movd mm7,edi
		movq mm0,mm4

		psubd mm0,mm7
		mov eax,0xFF

		psllq mm1,mm0
		mov edx, dword ptr [temp1]

		ret 

call_load_more_bytes:
	call load_more_bytes
	jmp continue_reading
//---------------------------------------End Main Loop-----------

	got_FF:
	 //test to see if there are enough bytes in input_buffer
	 test ecx,ecx
	 jne continue_reading_2
	 call load_more_bytes
continue_reading_2:
	//decrement bytes_in_buffer// 
	 dec ecx
	//load new data
	 xor eax,eax
     mov al,[ebx]
 	//update next_input_byte pointer
	 inc ebx //do this twice?
	 cmp eax,0xff
	 je got_FF
	 test eax,eax
	 jne eod_marker
	 mov eax,0xFF
	 jmp stuff_byte	//stuff an 'FF'
eod_marker:	 //byte was an end-of-data marker
	 mov [br_state.unread_marker],eax
	 //if we have enough bits in the input buffer to cover the required bits, ok.
	 //otherwise, warn the sytem about corrupt data.

no_more_data:
	 xor eax, eax
	 //movd ebx,mm0   //dshade
	 //cmp bl,[nbits]
	 //jl corrupt_data
	 //ok, have enough data, 
	 jmp stuff_byte_corrupt

//corrupt_data:	 
	//this junk is the WARNMS macro
	
	mov eax,dword ptr [br_state.printed_eod_ptr]
	cmp dword ptr [eax],0x00
	jne continue_corrupt


	mov eax,dword ptr [cinfo]
	mov eax,(j_decompress_ptr [eax]).err		//the err struct is the first memer of state->cinfo
	mov (j_cerr_ptr [eax]).msg_code,JWRN_HIT_MARKER
	push 0xffffffff

	mov eax,dword ptr [cinfo] 
	push eax					

	
	mov eax,dword ptr[cinfo]  //the err struct is the first member of state->cinfo
	mov eax,(j_decompress_ptr [eax]).err
	call (j_cerr_ptr [eax]).emit_message
	//call dword ptr[eax]
	add esp,8
	mov eax, dword ptr[br_state.printed_eod_ptr]
	mov dword ptr [eax],1
continue_corrupt:
	xor eax,eax
	jmp stuff_byte_corrupt

stuff_byte_corrupt:
	psllq mm1,8
	movd mm7,eax
	add edi,8
	por mm1,mm7

	//determine if we've read enough bytes
	cmp edi,56
	jle stuff_byte_corrupt
	jmp done_loading

	

load_more_bytes:
	 movd mm0,edi
	 mov [br_state.next_input_byte],ebx
	 mov eax,[br_state.cinfo]
	 push eax
	 //mov eax,[br_state.cinfo]
	 mov eax,(j_decompress_ptr[eax]).src
	 //movd mm0,edi
	 call (j_csrc_ptr [eax]).fill_input_buffer
	 add esp,4
	 //eax has the return value.  If zero, bomb out
	 test eax,eax
	 je return_4
	 //update next_input_byte and bytes_in_buffer.
	 mov eax,[br_state.cinfo]
	 mov eax,(j_decompress_ptr[eax]).src
	 mov ebx,(j_csrc_ptr [eax]).next_input_byte;
	 mov ecx,(j_csrc_ptr [eax]).bytes_in_buffer;
	 movd edi,mm0
	 mov edx,dword ptr[temp1]
	 ret


return_4:
	 mov eax,0x40
	 movd mm4,eax
	 mov eax,0
	 mov edx,dword ptr[temp1]
	 emms
	 ret 

	
//End fill_bit_buffer--------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

/***************************************************************************
	Jpeg_huff_decode_fast.  
	Assembly procedure to decode Huffman coefficients longer than 8 bits.
	Also called near the end of a data segment.


	Input Parameters
	eax:  minimum number of bits for the next huffman code.

    various MMX registers and local variables must be defined; see 
	_decode_one_mcu_inner above

	This code is infrequently called
****************************************************************************/

jpeg_huff_decode_fast:
  /* HUFF_DECODE has determined that the code is at least min_bits */
  /* bits long, so fetch that many bits in one swoop. */
			push edx
			mov [min_bits],ebx

			cmp edi,ebx
			jl Fill_Input_Buffer
Filled_Up:		
			
			sub edi,ebx
			movq mm3,mm4

			movd mm7,ebx
			movq mm2,mm1
			
			psubd mm3,mm7
			psllq mm1,mm7
			
			psrlq mm2,mm3
			movd ecx,mm2
		
Continue_Tedious_1:			
//now mm7 holds the most recent code

  /* Collect the rest of the Huffman code one bit at a time. */
  /* This is per Figure F.16 in the JPEG spec. */
			mov  eax,dword ptr [min_bits]
			mov  edx,dword ptr [htbl]
			//mov  ecx,dword ptr [code]
			mov  ebx,dword ptr [edx+eax*4].maxcode
			cmp  ebx,ecx
			jge  Continue_Tedious_2b

  //while (code > htbl->maxcode[min_bits]) {
    
			//movd eax,mm0
			cmp edi,1
			jl Fill_Input_Buffer_2
Filled_Up_2:			

			dec edi
			movq mm3,mm1

			psrlq mm3,63
			
			movd mm7,ecx
			psllq mm1,1
			
			psllq mm7,1
			inc [min_bits]
			
			por mm7,mm3
			movd ecx,mm7
    
			jmp Continue_Tedious_1	

Fill_Input_Buffer:
	//al should hold the number of valid bits;
	//mov eax,ebx
	call fill_bit_buffer			
	//if it returned a zero, exit with a -1.
	test eax,eax
	je Suspend_Label
	//we were able to fill it with (some) data.  
	//jump back to the continuation of this loop:
	xor ebx,ebx
	mov ebx,[min_bits]
	jmp Filled_Up



Fill_Input_Buffer_2:
	
	mov ebx,1
	mov [code],ecx
	call fill_bit_buffer			
	//if it returned a zero, exit with a -1.
	test eax,eax
	je Suspend_Label
	//we were able to fill it with (some) data.  
	//jump back to the continuation of this loop:
	mov ecx,[code]
	jmp Filled_Up_2

Continue_Tedious_2b: 
push edi
  /* With garbage input we may reach the sentinel value l = 17. */
}
  if (min_bits > 16) {
    WARNMS(br_state.cinfo, JWRN_HUFF_BAD_CODE);
  __asm {
	    pop edi
		xor eax,eax
		pop edx
		ret
		}
  }
  
  /*code= htbl->pub->huffval[ htbl->valptr[min_bits] +
			    ((int) (code - htbl->mincode[min_bits])) ];*/
__asm{
pop edi
mov       eax,dword ptr [min_bits]
mov       ebx,dword ptr [htbl]
sub       ecx,(dword ptr [ebx+eax*4]).mincode
add       ecx,(dword ptr [ebx+eax*4]).valptr
mov       ebx,(h_pub_ptr [ebx]).pub
xor       eax,eax
mov       al,(byte ptr [ecx+ebx]).huffval
pop		  edx
ret

Suspend_Label:
  
	mov eax,1
	pop edx
	ret
  }
}

#endif

//End jpeg_huff_decode_fast-------------------------------------------------
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

/*
 * Module initialization routine for Huffman entropy decoding.
 */

GLOBAL(void)
jinit_huff_decoder (j_decompress_ptr cinfo)
{
  huff_entropy_ptr entropy;
  int i;

  entropy = (huff_entropy_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(huff_entropy_decoder));
  cinfo->entropy = (struct jpeg_entropy_decoder *) entropy;
  entropy->pub.start_pass = start_pass_huff_decoder;
#if 0
//#ifdef _X86_  
  if (vfMMXMachine)
  {
	  entropy->pub.decode_mcu = decode_mcu_fast;
  }
  else
  {
	  entropy->pub.decode_mcu = decode_mcu;
  }
#else
    entropy->pub.decode_mcu = decode_mcu;
#endif
  /* Mark tables unallocated */
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    entropy->dc_derived_tbls[i] = entropy->ac_derived_tbls[i] = NULL;
  }
}
