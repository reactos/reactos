/***
**
**   Module: ttprog
**
**   Description:
**      This is a module of the T1 to TT font converter. This is a
**      sub-module of Hint module. This modules deals with the 
**      the font program fo the font.
**
**   Author: Michael Jansson
**
**   Created: 8/24/93
**
***/


/**** INCLUDES */
/* General types and definitions. */
#include <limits.h>

/* Special types and definitions. */
#include "titott.h"
#include "types.h"
#include "safemem.h"
#include "metrics.h"
#include "t1msg.h"

/* Module dependent types and prototypes. */
/*#include "hints.h"*/
#include "ttprog.h"



/***** MACROS */
/*-none-*/



/***** CONSTANTS */
#define MAXIP			100

#define CURVEPHASE      6

#define MAXTHINPNTS     512

#define UNDEF        -1

#define SMALL_LOWER     21L
#define SMALL_UPPER     50L
#define LARGE_LOWER     21L
#define LARGE_UPPER     21L

#define BUFSIZE         20

#define TTFUN_SET_ZONE           1 
#define TTFUN_COPY_ZONE          2 
#define TTFUN_STEM_SNAP_WIDTH    3 
#define TTFUN_STEM_STD_WIDTH     4 
#define TTFUN_SHIFT_BLUE_ZONE    5 
#define TTFUN_ALIGN_BLUE_ZONE    6 
#define TTFUN_COPY_FAMILY        7 
#define TTFUN_WRITE_STEM         8 
#define TTFUN_VERTICAL           9 
#define TTFUN_HORIZONTAL         10
#define TTFUN_VCENTER            11
#define TTFUN_HCENTER            12
#define TTFUN_RELATIVE1V         13
#define TTFUN_RELATIVE2V         14
#define TTFUN_RELATIVE1H         15
#define TTFUN_RELATIVE2H         16
#define TTFUN_SIDE1              17    
#define TTFUN_SIDE2              18
#define TTFUN_FLEX               19
#define TTFUN_SCALE3             20
#define TTFUN_SHIFT1             21
#define TTFUN_SHIFT2             22
#define TTFUN_IP1                23  
#define TTFUN_IP2                24
#define TTFUN_IPN                25
#define TTFUN_SHP1               26  
#define TTFUN_SHP2               27
#define TTFUN_SHPN               28
#define TTFUN_RANGE              29
#define TTFUN_OBLIQUE            30
#define TTFUN_NUM                31    /* 1..30 */


#define FDEF(name)            op_pushb1, name, op_fdef,
#define ENDF                  op_endf,
#define CALL(name)            op_pushb1, name, op_call
#define WCVT(name)            op_pushb1, name, op_swap, op_wcvtf
#define PUSH1(v)              op_pushb1, (v)
#define PUSH2(v1, v2)         op_pushb1+1, (v1), (v2)
#define PUSH3(v1, v2, v3)     op_pushb1+2, (v1), (v2), (v3)
#define PUSH4(v1, v2, v3, v4) op_pushb1+3, (v1), (v2), (v3), (v4)
#define PUSH5(v1,v2,v3,v4,v5) op_pushb1+4, (v1), (v2), (v3), (v4), (v5)


static const UBYTE FontProg[] = {


   
/******* SET ZONE FUNCTION
 *
 * Args: flat_pos
 *
 */
FDEF(TTFUN_SET_ZONE)
   PUSH1(TMPPNT),
   op_swap,
   op_miap,
   PUSH1(TMPPNT),
   op_mdap | SUBOP_R,
ENDF




/******* COPY ZONE FUNCTION
 *
 * Args: from_cvt, to_cvt
 *
 */
FDEF(TTFUN_COPY_ZONE)
   op_rcvt,
   op_round,
   op_wcvtp,
ENDF





/******* STEM SNAP WIDTH FUNCTION
 *
 * Args: std_ci, std_cvt, snap_ci, snap_cvt, width, storage
 *
 */
FDEF(TTFUN_STEM_SNAP_WIDTH)
   op_mppem,
   op_gteq,
   op_if,

      /* Use std */
      op_rcvt,
      op_round,
      PUSH1(ONEPIXEL/2),
      op_max,
      op_swap, op_pop, op_swap, op_pop, op_swap, op_pop,
      CALL(TTFUN_WRITE_STEM),
      
   op_else,
      op_pop,
      op_mppem,
      op_gteq,
      op_if,
   
         /* Use snap */
         op_rcvt,
         op_round,
         PUSH1(ONEPIXEL/2),
         op_max,
    op_swap,
         op_pop, 
         CALL(TTFUN_WRITE_STEM),
         
      /* Use real width. */
      op_else,
         op_pop,
         WCVT(TMPCVT),
         PUSH1(TMPCVT),
         op_rcvt,
         op_round,
         PUSH1(ONEPIXEL/2),
         op_max,
         CALL(TTFUN_WRITE_STEM),
      op_eif,
   
   op_eif,
ENDF



   

/******* STEM STD WIDTH FUNCTION
 *
 * Args: std_ci, std_cvt, width, storage
 *
 */
FDEF(TTFUN_STEM_STD_WIDTH)
   op_mppem,
   op_gteq,
   op_if,
   
      /* Use std */
      op_rcvt,
      op_round,
      PUSH1(ONEPIXEL/2),
      op_max,
      op_swap,
      op_pop,
      CALL(TTFUN_WRITE_STEM),
      
   /* Use real width. */
   op_else,
      op_pop,
      WCVT(TMPCVT),
      PUSH1(TMPCVT),
      op_rcvt,
      op_round,
      PUSH1(ONEPIXEL/2),
      op_max,
      CALL(TTFUN_WRITE_STEM),
   op_eif,
   
ENDF





/******* SHIFT BLUE ZONE FUNCTION
 *
 * Args: cvt
 *
 */
FDEF(TTFUN_SHIFT_BLUE_ZONE)
   PUSH5(TMPPNT1, TMPPNT1, TMPPNT, TMPPNT1, 5),
   op_cindex,
   op_miap,
   op_srp0,
   op_mdrp | SUBOP_mMRGR,
   op_gc,
   op_wcvtp,
ENDF





/******* ALIGN BLUE ZONE FUNCTION
 *
 * Args: cvt
 *
 */
FDEF(TTFUN_ALIGN_BLUE_ZONE)
   PUSH5(TMPPNT1, TMPPNT1, TMPPNT, TMPPNT1, 5),
   op_cindex,
   op_miap,
   op_srp0,
   op_mdrp | SUBOP_ROUND,
   op_gc,
   op_wcvtp,
ENDF





/******* COPY FAMILY FUNCTION
 *
 * Args: base_cvt
 *
 */
FDEF(TTFUN_COPY_FAMILY)
   op_dup,
   PUSH1(1),
   op_add,
   op_rcvt,
   op_wcvtp,
ENDF





/******* WRITE STEM FUNCTION
 *
 * Args: width, storage
 *
 */
FDEF(TTFUN_WRITE_STEM)
   op_dup,    /* -| width, width, storage */
   op_dup,    /* -| width, width, width, storage */
   op_add,    /* -| 2*width, width, storage, */
   op_odd,    /* -| odd/even, width, storage */
   PUSH2(1, 4),     /* -| 4, 1, odd/even, width, storage */
   op_cindex,     /* -| storage, 1, odd/even, width, storage */
   op_add,
   op_swap,   /* -| odd/even, storage+1, width, storage */
   op_ws,
   op_ws,
ENDF





/******* VERTICAL FUNCTION
 *
 * Args: -*none*-
 *
 */
FDEF(TTFUN_VERTICAL)
   op_svcta | SUBOP_X,
   PUSH1(TWILIGHT),
   op_szps,
ENDF





/******* HORIZONTAL FUNCTION
 *
 * Args: -*none*-
 *
 */
FDEF(TTFUN_HORIZONTAL)
   PUSH1(TWILIGHT),
   op_svcta,
   op_szps,
ENDF





/******* CENTER VSTEM FUNCTION
 *
 * Args: p1, p2, p3, p4, c, tz1, width
 *
 */
FDEF(TTFUN_VCENTER)

   /* Set rounding state for the center. */
   PUSH2(1, 8),
   op_cindex,
   op_add,
   op_rs,
   op_if,
       op_rthg,
   op_else,
       op_rtg,
   op_eif,

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 6),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 6),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 2, 5),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 3, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,


   /* Move/round center. */
   WCVT(TMPCVT),     /* c */
   PUSH2(TMPPNT, TMPCVT),
   op_miap| SUBOP_R,
   op_rtg,

   /* Align all points to the center. */
   op_dup, op_dup, PUSH1(1), op_add,
   op_alignrp, op_alignrp,   /* tz1, tz1+1 */


   /* Compute the width. */
   op_swap,
   op_rs,
   PUSH1(CURVEPHASE),
   op_sub,
   op_swap,


   /* -| tz1, width */
   op_dup,
   op_dup,
   op_dup,
   op_srp0,
   PUSH1(4), op_cindex,
   op_neg,     /* -| (-width/2), tz1, tz1, tz1, width */
   op_shpix,      
   PUSH1(2),
   op_add,
   op_alignrp,    /* -| tz1+2, tz1, width */


   /* Do the other side. */
   /* -| tz1, width */
   PUSH1(1),
   op_add,
   op_dup,
   op_dup,     /* -| tz1+1, tz1+1, tz1+1, width */
   op_srp0,
   op_roll,    /* -| width, tz1+1, tz1+1 */
   op_shpix,      
   PUSH1(2),
   op_add,
   op_alignrp,    /* -| tz1+3 */

   /* Done. */
ENDF






/******* CENTER HSTEM FUNCTION
 *
 * Args: p1, p2, c, tz1, width
 *
 */
FDEF(TTFUN_HCENTER)

   /* Set,rounding state for the center. */
   PUSH2(1, 6),
   op_cindex,
   op_add,
   op_rs,
   op_if,
       op_rthg,
   op_else,
       op_rtg,
   op_eif,

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 4),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,


   /* Move/round center. */
   WCVT(TMPCVT),  /* c */
   PUSH2(TMPPNT, TMPCVT),
   op_miap| SUBOP_R,
   op_rtg,

   /* Align all points to the center. */
   op_dup, op_dup, PUSH1(1), op_add,
   op_alignrp, op_alignrp,   /* tz1, tz1+1 */


   /* Compute the width. */
   op_swap,
   op_rs,
   PUSH1(CURVEPHASE),
   op_sub,
   op_swap,


   /* -| tz1, width */
   op_dup,
   PUSH1(3), op_cindex,
   op_neg,     /* -| -width, tz1, tz1, width */
   op_shpix,      

   /* Do the other side. */
   /* -| tz1, width */
   PUSH1(1),
   op_add,
   op_swap,    /* -| width, tz1+1 */
   op_shpix,      

   /* Done. */
ENDF





/******* RELATIVE1V STEM FUNCTION
 *
 * Args: p1, p2, p3, p4, ref, tz1, width
 *
 */
FDEF(TTFUN_RELATIVE1V)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 6),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 6),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 2, 5),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 3, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   /* Move/round side. */
   op_srp0,
   op_dup,
   op_mdrp | SUBOP_MmRGR,

   /* Align points on the left side. */
   op_dup, PUSH1(1), op_add, op_dup, op_dup, op_dup,
   PUSH1(1), op_add,  /* -| tz1+2, tz1+1, tz1+1, tz+1, tz, width */
   op_alignrp,
   op_alignrp,

   /* Align right side */
   op_srp0,    /* -| tz1+1, tz1, width */
   op_roll,
   op_rs,
   op_dup,
   op_add,     /* -| width*2, tz1+1, tz1 */
   op_shpix,
   PUSH1(3),
   op_add,
   op_alignrp,    /* -| tz1+3 */

ENDF





/******* RELATIVE2V STEM FUNCTION
 *
 * Args: p1, p2, p3, p4, ref, tz1, width
 *
 */
FDEF(TTFUN_RELATIVE2V)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 6),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 6),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 2, 5),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 3, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,


   /* Move/round side. */
   op_srp0,
   op_dup,
   PUSH1(1), op_add,
   op_mdrp | SUBOP_MmRGR,

   /* Align points on the left side. */
   op_dup, op_dup, op_dup, op_dup,
   PUSH1(3), op_add,  /* -| tz1+3, tz1, tz1, tz1, tz1, width */
   op_alignrp,
   op_alignrp,


   /* Align left side */
   op_srp0,    /* -| tz1, tz1, width */
   op_roll,
   op_rs,
   op_dup,
   op_add,
   op_neg,
   op_shpix,      /* -| -2*width, tz1, tz1 */
   PUSH1(2), op_add,
   op_alignrp,    /* -| tz1+2 */

ENDF





/******* RELATIVE1H STEM FUNCTION
 *
 * Args: p1, p2, ref, tz1, width
 *
 */
FDEF(TTFUN_RELATIVE1H)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 4),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   /* Move/round side. */
   op_srp0,
   op_dup,
   op_mdrp | SUBOP_MmRGR,


   /* Align all point to the lower side. */
   PUSH1(1), op_add, op_dup,
   op_alignrp,

   /* Align right side */
   op_swap,
   op_rs,
   op_dup,
   op_add,
   op_shpix,

ENDF





/******* RELATIVE2H STEM FUNCTION
 *
 * Args: p1, p2, ref, tz1, width
 *
 */
FDEF(TTFUN_RELATIVE2H)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 4),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,


   /* Move/round side. */
   op_srp0,
   op_dup,
   PUSH1(1), op_add,
   op_mdrp | SUBOP_MmRGR,

   /* Align all points to the center. */
   op_dup, op_alignrp,

   /* Align left side */
   op_swap,
   op_rs,
   op_dup,
   op_add,
   op_neg,
   op_shpix,

ENDF





/******* SIDE1 STEM FUNCTION
 *
 * Args: p1, p2, zone, tz1, width
 *
 */
FDEF(TTFUN_SIDE1)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 4),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,

   /* Move/round side. */
   PUSH2(TMPPNT, TMPPNT),
   op_srp0, op_swap, op_miap | SUBOP_R,

   /* Align all points to the side. */
   op_dup, PUSH1(1), op_add, op_dup, op_roll,
   op_alignrp, op_alignrp,

   /* Align first side */
   op_swap,
   op_rs,
   op_dup,
   op_add,
   PUSH1(CURVEPHASE),
   op_sub,
   op_shpix,

ENDF





/******* SIDE2 STEM FUNCTION
 *
 * Args:  p1, p2, zone, tz1, width
 *
 */
FDEF(TTFUN_SIDE2)

   /* Create the stem in the twilight zone. */
   WCVT(TMPCVT),
   PUSH2(TMPCVT, 4),
   op_cindex,
   op_swap,
   op_miap,

   WCVT(TMPCVT),
   PUSH3(TMPCVT, 1, 4),
   op_cindex,
   op_add,
   op_swap,
   op_miap,


   /* Move/round side. */
   PUSH2(TMPPNT, TMPPNT),
   op_srp0, op_swap, op_miap | SUBOP_R,

   /* Align all points to the side. */
   op_dup, op_dup, PUSH1(1), op_add, 
   op_alignrp, op_alignrp,

   /* Align second side */
   op_swap,
   op_rs,
   op_dup,
   op_add,
   PUSH1(CURVEPHASE),
   op_sub,
   op_neg,
   op_shpix,

ENDF


    


/******* FLEX FUNCTION
 *
 * Args on the stack: pnt_start, pnt_mid, ref_pos, pnt_mid,
 *       pnt_start, pnt_mid, cnt, p1, p2, ....
 *
 */
FDEF(TTFUN_FLEX)
   op_srp0,
   op_alignrp,
   op_wcvtf,
   op_rcvt,
   op_shpix,
   op_srp1,
   op_srp2,
   op_sloop,
   op_ip,
ENDF





/******* SCALE3 FUNCTION
 *
 * Args: cnt, p1, p2, ... 
 *
 */
FDEF(TTFUN_SCALE3)
   PUSH4(GLYPHZONE, TMPPNT1, TMPPNT, TMPPNT1),
   op_pushw1, HIBYTE(-31), LOBYTE(-31),
   PUSH3(TMPPNT, 0, TMPPNT1),
   op_pushw1, HIBYTE(1000), LOBYTE(1000),
   op_scfs,
   op_scfs,
   op_shpix,
   op_srp1,
   op_srp2,
   op_szp2,
   op_sloop,
   op_ip,
ENDF





/******* SHIFT1 FUNCTION
 *
 * Args: cnt reduction p1 p2 ...
 *
 */
FDEF(TTFUN_SHIFT1)
   op_sloop,
   op_rs,
   op_neg,
   op_shpix,
ENDF




   
/******* SHIFT2 FUNCTION
 *
 * Args: cnt reduction p1 p2 ...
 *
 */
FDEF(TTFUN_SHIFT2)
   op_sloop,
   op_rs,
   op_shpix,
ENDF





/******* IP1 FUNCTION
 *
 * Args: rp1, rp2, p1
 *
 */
FDEF(TTFUN_IP1)
   op_srp1,
   op_srp2,
   op_ip,
ENDF





/******* IP2 FUNCTION
 *
 * Args: rp1, rp2, p1, p2
 *
 */
FDEF(TTFUN_IP2)
   op_srp1,
   op_srp2,
   op_ip,
   op_ip,
ENDF





/******* IPN FUNCTION
 *
 * Args: rp1, rp2, cnt, p1, p2
 *
 */
FDEF(TTFUN_IPN)
   op_srp1,
   op_srp2,
   op_sloop,
   op_ip,
ENDF





/******* SHP1 FUNCTION
 *
 * Args: rp, p
 *
 */
FDEF(TTFUN_SHP1)
   op_srp1,
   op_shp,
ENDF





/******* SHP2 FUNCTION
 *
 * Args: rp, p1, p2
 *
 */
FDEF(TTFUN_SHP2)
   op_srp1,
   op_shp,
   op_shp,
ENDF


    

    
/******* SHPN FUNCTION
 *
 * Args: rp, cnt, p1, p2
 *
 */
FDEF(TTFUN_SHPN)
   op_srp1,
   op_sloop,
   op_shp,
ENDF





/******* RANGE FUNCTION
 *
 * Args: p
 *
 */
FDEF(TTFUN_RANGE)
   op_dup,
   PUSH1(1),
   op_add,
ENDF




/******* RANGE FUNCTION
 *
 * Args: pos_x, pos_y,
 *
 */
FDEF(TTFUN_OBLIQUE)
   op_svcta | SUBOP_Y,
   PUSH1(TMPPNT1),
   op_swap,
   op_scfs,
   PUSH2(TMPPNT, 0),
   op_scfs,
   op_svcta | SUBOP_X,
   PUSH1(TMPPNT1),
   op_swap,
   op_scfs,
   PUSH2(TMPPNT, 0),
   op_scfs,
   PUSH2(TMPPNT, TMPPNT1),
   op_spvtl,
ENDF

};


/***** LOCAL TYPES */


/***** STATIC FUNCTIONS */

/***
** Function: GetVStemWidth
**
** Description:
**   This function allocates a storage entry for the 
**   width of a vertical stem;
***/
static short GetVStemWidth(WeightControl *weight, const funit width)
{
   StemWidth *newwidths = NULL;
   short entry = 0;
   USHORT j;

   if (weight->cnt_vw >= weight->max_vw) {
      newwidths = Realloc(weight->vwidths,
                          sizeof(StemWidth)*(weight->max_vw+BUFSIZE));
      if (newwidths == NULL) {
         entry = NOMEM;
      } else {
         weight->vwidths = newwidths;
         weight->max_vw += BUFSIZE;
      }
   }

   if (entry != NOMEM) {
      for (j=0; j<weight->cnt_vw; j++) {
         if (weight->vwidths[j].width==width) {
            entry = (short)weight->vwidths[j].storage;
            break;
         }
      }

      if (j==weight->cnt_vw) {
         weight->vwidths[weight->cnt_vw].storage = weight->storage;
         weight->vwidths[weight->cnt_vw].width = width;
         entry = (short)weight->storage;
         weight->storage += 2;
         weight->cnt_vw++;
      }
   }

   return entry;
}


/***
** Function: GetHStemWidth
**
** Description:
**   This function allocates a storage entry for the 
**   width of a vertical stem;
***/
static short GetHStemWidth(WeightControl *weight, const funit width)
{
   StemWidth *newwidths = NULL;
   short entry = 0;
   USHORT j;

   if (weight->cnt_hw >= weight->max_hw) {
      newwidths = Realloc(weight->hwidths,
                          sizeof(StemWidth)*(weight->max_hw+BUFSIZE));
      if (newwidths == NULL) {
         entry = NOMEM;
      } else {
         weight->hwidths = newwidths;
         weight->max_hw += BUFSIZE;
      }
   }

   if (entry != NOMEM) {
      for (j=0; j<weight->cnt_hw; j++) {
         if (weight->hwidths[j].width==width) {
            entry = (short)weight->hwidths[j].storage;
            break;
         }
      }

      if (j==weight->cnt_hw) {
         weight->hwidths[weight->cnt_hw].storage = weight->storage;
         weight->hwidths[weight->cnt_hw].width = width;
         entry = (short)weight->storage;
         weight->storage += 2;
         weight->cnt_hw++;
      }
   }

   return entry;
}




/***** GLOBAL FUNCTIONS */

/***
** Function: SetZone
**
** Description:
**   This function initiate an alignment zone
**   by creating an appropriate point in the
**   twilight zone.
***/
USHORT SetZone(UBYTE *prep, USHORT tp, const short cvt)
{
   /* Set up the zone. */
   if (cvt>255) {
      prep[tp++] = op_pushw1;
      prep[tp++] = HIBYTE(cvt);
      prep[tp++] = LOBYTE(cvt);
      prep[tp++] = op_pushb1;
      prep[tp++] = TTFUN_SET_ZONE;
   } else {
      prep[tp++] = op_pushb1 + 1;
      prep[tp++] = (UBYTE)cvt;
      prep[tp++] = TTFUN_SET_ZONE;
   }
   prep[tp++] = op_call;

   return tp;
}



/***
** Function: CopyZone
**
** Description:
**   This function copies a cvt entry, representing an
**   alignment zone, to the cvt used for a particular hstem.
***/
USHORT CopyZone(UBYTE *prep, short tp, short *args, const short ta)
{
   args[0] = TTFUN_COPY_ZONE;
   args[1] = (short)((ta-2)/2);
   AssembleArgs(args, ta, prep, &tp);
   prep[tp++] = op_loopcall;

   return (USHORT)tp;
}



/***
** Function: CopyFamilyBlue
**
** Description:
**   This function copies a cvt entry, representing a
**   family blue zone, to the cvt used for a particular hstem.
***/
USHORT CopyFamilyBlue(UBYTE *prep, short tp, short *args, const short ta)
{
   args[0] = TTFUN_COPY_FAMILY;
   args[1] = (short)(ta-2);
   AssembleArgs(args, ta, prep, &tp);
   prep[tp++] = op_loopcall;

   return (USHORT)tp;
}



/***
** Function: AlignFlat
**
** Description:
**   This function creates a cvt entry for
**   a particular hstem.
***/
USHORT AlignFlat(UBYTE *prep, short tp, short *args, const short ta)
{
   args[0] = TTFUN_ALIGN_BLUE_ZONE;
   args[1] = (short)(ta-2);
   AssembleArgs(args, ta, prep, &tp);
   prep[tp++] = op_loopcall;

   return (USHORT)tp;
}



/***
** Function: AlignOvershoot
**
** Description:
**   This function creates a cvt entry for
**   a particular hstem.
***/
USHORT AlignOvershoot(UBYTE *prep, short tp, short *args, const short ta)
{
   args[0] = TTFUN_SHIFT_BLUE_ZONE;
   args[1] = (short)(ta-2);
   AssembleArgs(args, ta, prep, &tp);
   prep[tp++] = op_loopcall;

   return (USHORT)tp;
}


/***
** Function: GetTopPos
**
** Description:
**   This function allocates a cvt entry for the 
**   top side of a horizontal stem;
***/
short GetTopPos(const Blues *blues,
                AlignmentControl *align,
                const funit pos)
{
   short entry = UNDEF;
   const funit *bluevals;
   short fuzz;
   USHORT i, j;

   bluevals = &(blues->bluevalues[0]);
   fuzz = blues->blueFuzz;

   /* Check if it is within a zone. */
   for (i=0; i<blues->blue_cnt; i+=2) {
      if (((bluevals[i]-fuzz)<=pos) && ((bluevals[i+1]+fuzz)>=pos))
         break;
   }

   /* Record the position? */
   if (i!=blues->blue_cnt) {
      i /= 2;

      /* Is the position already mapped to a cvt entry? */
      for (j=0; j<align->top[i].cnt; j++) {
         if (align->top[i].pos[j].y==pos) {
            entry = (short)align->top[i].pos[j].cvt;
            break;
         }
      }

      if (j==align->top[i].cnt) {

         /* Allocate the BlueZone cvt's */
         if (align->top[i].cnt==0) {
            align->top[i].blue_cvt = align->cvt;
            align->cvt +=2;
         }

         align->top[i].pos[align->top[i].cnt].cvt = align->cvt;
         align->top[i].pos[align->top[i].cnt].y = pos;
         entry = (short)align->cvt;
         align->cvt+=2;
         align->top[i].cnt++;
      }
   }

   return entry;
}


/***
** Function: GetBottomPos
**
** Description:
**   This function allocates a cvt entry for the 
**   top side of a horizontal stem;
***/
short GetBottomPos(const Blues *blues,
                   AlignmentControl *align,
                   const funit pos)
{
   short entry = UNDEF;
   const funit *bluevals;
   short fuzz;
   USHORT i, j;

   bluevals = &(blues->otherblues[0]);
   fuzz = blues->blueFuzz;

   /* Check if it is within a zone. */
   for (i=0; i<blues->oblue_cnt; i+=2) {
      if (((bluevals[i]-fuzz)<=pos) && ((bluevals[i+1]+fuzz)>=pos))
         break;
   }


   /* Record the position? */
   if (i!=blues->oblue_cnt) {
      i /= 2;

      /* Is the position already mapped to a cvt entry? */
      for (j=0; j<align->bottom[i].cnt; j++) {
         if (align->bottom[i].pos[j].y==pos) {
            entry = (short)align->bottom[i].pos[j].cvt;
            break;
         }
      }

      if (j==align->bottom[i].cnt) {

         /* Allocate the BlueZone and FamilyBlue cvt's */
         if (align->bottom[i].cnt==0) {
            align->bottom[i].blue_cvt = align->cvt++;
         }

         align->bottom[i].pos[align->bottom[i].cnt].cvt = align->cvt;
         align->bottom[i].pos[align->bottom[i].cnt].y = pos;
         entry = (short)align->cvt;
         align->cvt+=2;
         align->bottom[i].cnt++;
      }
   }

   return entry;
}


/***
** Function: CutInSize
**
** Description:
**   This function computes the cut in size
**   of a stem, given a master width and the
**   width of the stem. This is done with the
**   StdVW==2.0 pixel treshold and the thinn
**   and wide cut in values.
***/
USHORT CutInSize(const funit width,
                 const funit master,
                 const USHORT tresh,
                 const funit upem)
{
   USHORT cis, ci1, ci2;

   /*lint -e776 */
   if (width > master) {
      ci1 = (USHORT)((long)upem * SMALL_UPPER / ONEPIXEL /
                     (long)(width - master));
      ci2 = (USHORT)((long)upem * LARGE_UPPER / ONEPIXEL /
                     (long)(width - master));
   } else if (width < master) {
      ci1 = (USHORT)((long)upem * SMALL_LOWER / ONEPIXEL /
                     (long)(master - width));
      ci2 = (USHORT)((long)upem * LARGE_LOWER / ONEPIXEL /
                     (long)(master - width));
   } else {
      ci1 = INFINITY;  
      ci2 = INFINITY;
   }
   /*lint +e776 */

   if (ci1 < tresh) {
      cis = ci1;
   } else if (ci2 < tresh) {
      cis = tresh;
   } else {
      cis = ci2;
   }

   return cis;
}


/***
** Function: SnapStemArgs
**
** Description:
**   
***/
USHORT SnapStemArgs(short *args, USHORT ta,
                    const funit width,
                    const USHORT std_cvt,
                    const USHORT snap_cvt,
                    const USHORT std_ci,
                    const USHORT snap_ci,
                    const USHORT storage)
{
   args[ta++] = (short)std_ci;
   args[ta++] = (short)std_cvt;
   args[ta++] = (short)snap_ci;
   args[ta++] = (short)snap_cvt;
   args[ta++] = (short)(width/2);
   args[ta++] = (short)storage;

   return ta;
}



/***
** Function: StdStemArgs
**
** Description:
**   
***/
USHORT StdStemArgs(short *args, USHORT ta,
                   const funit width,
                   const USHORT std_cvt,
                   const USHORT std_ci,
                   const USHORT storage)
{
   args[ta++] = (short)std_ci;
   args[ta++] = (short)std_cvt;
   args[ta++] = (short)(width/2);
   args[ta++] = (short)storage;

   return ta;
}



/***
** Function: CreateStdStems
**
** Description:
**   
***/
USHORT CreateStdStems(UBYTE *prep, USHORT tp, const short cnt)
{
   if (cnt>255) {
      prep[tp++] = op_pushw1;
      prep[tp++] = HIBYTE(cnt);
      prep[tp++] = LOBYTE(cnt);
      prep[tp++] = op_pushb1;
      prep[tp++] = TTFUN_STEM_STD_WIDTH;
   } else {
      prep[tp++] = op_pushb1 + 1;
      prep[tp++] = (UBYTE)cnt;
      prep[tp++] = TTFUN_STEM_STD_WIDTH;
   }

   prep[tp++] = op_loopcall;

   return tp;
}



/***
** Function: CreateSnapStems
**
** Description:
**   
***/
USHORT CreateSnapStems(UBYTE *prep, USHORT tp, const short cnt)
{
   if (cnt>255) {
      prep[tp++] = op_pushw1;
      prep[tp++] = HIBYTE(cnt);
      prep[tp++] = LOBYTE(cnt);
      prep[tp++] = op_pushb1;
      prep[tp++] = TTFUN_STEM_SNAP_WIDTH;
   } else {
      prep[tp++] = op_pushb1 + 1;
      prep[tp++] = (UBYTE)cnt;
      prep[tp++] = TTFUN_STEM_SNAP_WIDTH;
   }

   prep[tp++] = op_loopcall;

   return tp;
}




/***
** Function: tt_GetFontProg
**
** Description:
**   This function returns the static font
**   font program.
***/
const UBYTE *tt_GetFontProg(void)
{
   return FontProg;
}




/***
** Function: tt_GetFontProgSize
**
** Description:
**   This function returns the size of the
**   static font program.
***/
USHORT tt_GetFontProgSize(void)
{
   return (USHORT)sizeof(FontProg);
}




/***
** Function: tt_GetNumFuns
**
** Description:
**   This function returns the number of functions
**   defined in the static font program.
***/
USHORT tt_GetNumFuns(void)
{
   return (USHORT)TTFUN_NUM;
}



/***
** Function: EmitFlex
**
** Description:
**   Convert a T1 flex hint into a TrueType IP[] 
**   intruction sequence that will reduce a flex
**   that is flatter than a given height.
***/
errcode EmitFlex(short *args,
                 short *pcd,
                 const funit height,
                 const short start,
                 const short mid,
                 const short last)
{
   errcode status = SUCCESS;
   int i;

   /* Enough space for the instructions? */
   args[(*pcd)++] = TTFUN_FLEX;
   args[(*pcd)++] = start;
   args[(*pcd)++] = mid;
   args[(*pcd)++] = (short)height;
   args[(*pcd)++] = TMPCVT;
   args[(*pcd)++] = TMPCVT;
   args[(*pcd)++] = mid;
   args[(*pcd)++] = start;
   args[(*pcd)++] = mid;

   /* Push the flex points onto the stack. */
   args[(*pcd)++] = (short)(last-start-2);
   for (i=start+(short)1; i<last; i++)
      if (i!=mid)
         args[(*pcd)++] = (short)i;

   return status;
}




/***
** Function: ReduceDiagonals
**
** Description:
**   This function generates the TT instructions
**   that will shrink the outline, in order to
**   control the width of diagonals. This implementation
**   can probably be improved.
***/
short ReduceDiagonals(const Outline *paths,
                      UBYTE *pgm, short *pc,
                      short *args,  short *pcd)
{
   short cw[MAXTHINPNTS];
   short ccw[MAXTHINPNTS];
   short targ[MAXTHINPNTS];
   const Outline *path;
   Point *pts;
   short i,j;
   short cwi = 0, ccwi = 0;
   short prev;
   short n,m;
   short prev_cw, prev_ccw;
   short ta;



   /* Collect points on left and right side that are diagonals. */
   i = 0;
   for (path = paths; path && ccwi<MAXTHINPNTS && cwi<MAXTHINPNTS;
   path=path->next) {

      pts = &path->pts[0];
      prev_cw = FALSE;
      prev_ccw = FALSE;

      /* Are the first and last point coinciding? */
      if (pts[path->count-1].x!=pts[0].x ||
          pts[path->count-1].y!=pts[0].y)
         prev = (short)(path->count-(short)1);
      else
         prev = (short)(path->count-(short)2);

      /* Special case the first point. */
      if (!OnCurve(path->onoff, prev) ||
          (pts[0].x != pts[prev].x &&
           ABS(pts[0].x - pts[prev].x) < ABS(pts[0].y - pts[prev].y)*8)) {
         if (pts[0].y>pts[prev].y+20) {
            if (pts[prev].y<=pts[prev-1].y)
               cw[cwi++] = (short)(i+(short)path->count-1);
            cw[cwi++] = i;
            prev_cw = TRUE;
            prev_ccw = FALSE;
         } else if (pts[0].y<pts[prev].y-20) {
            if (pts[prev].y>=pts[prev-1].y)
               ccw[ccwi++] = (short)(i+(short)path->count-1); 
            ccw[ccwi++] = i;
            prev_cw = FALSE;
            prev_ccw = TRUE;
         }
      }


      for (j=1; j<(short)path->count &&
             ccwi<MAXTHINPNTS && cwi<MAXTHINPNTS; j++) {
         i++;
         if (!OnCurve(path->onoff, j-1) ||
             (pts[j].x != pts[j-1].x &&
              ABS(pts[j].x - pts[j-1].x) < ABS(pts[j].y - pts[j-1].y)*8)) {
            if (pts[j].y>pts[j-1].y+20) {
               if (!prev_cw)
                  cw[cwi++] = (short)(i-1);
               cw[cwi++] = i;
               prev_cw = TRUE; 
               prev_ccw = FALSE;
            } else if (pts[j].y<pts[j-1].y-20) {
               if (!prev_ccw)
                  ccw[ccwi++] = (short)(i-1);
               ccw[ccwi++] = i;
               prev_cw = FALSE;
               prev_ccw = TRUE;
            } else {
               prev_cw = FALSE;
               prev_ccw = FALSE;
            }
         } else {
            prev_cw = FALSE;
            prev_ccw = FALSE;
         }
      }
      i++;
   }


   /* Did we get all points? */
   if (ccwi>=MAXTHINPNTS || cwi>=MAXTHINPNTS) {
      LogError(MSG_WARNING, MSG_DIAG, NULL);
   }


   /* Any points to shift? */
   if (cwi || ccwi) {
      args[(*pcd)++] = STORAGE_DIAG;
      pgm[(*pc)++] = op_rs;
      pgm[(*pc)++] = op_if;
      pgm[(*pc)++] = op_svcta + SUBOP_X;

      /* Switch over to GLYPHZONE */
      pgm[(*pc)++] = op_szp2;
      args[(*pcd)++] = 1;


      ta = 3;

      /* Disable "cw[m] may not have been initialized".*/ /*lint -e644 */
      for (n=0; n<cwi; n=m) {
         for (m=(short)(n+1); m<cwi && cw[m]==cw[m-1]+1; m++); /*lint +e644 */
         if (m-n<=4) {
            for (i=n; i<m; i++)
               targ[ta++] = cw[i];
         } else {
            targ[0] = TTFUN_RANGE;
            targ[1] = (short)(m-n-1);
            targ[2] = cw[n];
            AssembleArgs(targ, ta, pgm, pc);
            pgm[(*pc)++] = op_loopcall;
            ta = 3;
         }
      }
      targ[0] = TTFUN_SHIFT1;
      targ[1] = cwi;
      targ[2] = STORAGE_DIAG;
      AssembleArgs(targ, ta, pgm, pc);
      pgm[(*pc)++] = op_call;


      /************ Shift back the left side of the glyph. */

      ta = 3;

      /* Disable "ccw[m] may not have been initialized".*/ /*lint -e644 */
      for (n=0; n<ccwi; n=m) {
         for (m=(short)(n+1); m<ccwi && ccw[m]==ccw[m-1]+1; m++); /*lint +e644 */
         if (m-n<=4) {
            for (i=n; i<m; i++)
               targ[ta++] = ccw[i];
         } else {
            targ[0] = TTFUN_RANGE;
            targ[1] = (short)(m-n-1);
            targ[2] = ccw[n];
            AssembleArgs(targ, ta, pgm, pc);
            pgm[(*pc)++] = op_loopcall;
            ta = 3;
         }
      }
      targ[0] = TTFUN_SHIFT2;
      targ[1] = ccwi;
      targ[2] = STORAGE_DIAG;
      AssembleArgs(targ, ta, pgm, pc);
      pgm[(*pc)++] = op_call;


#ifdef SYMETRICAL_REDUCTION

      /* The amount that the outline is shrunk is computed once at
      each size, in the pre-program. The outline is shrunk
      symetrically by the amount: 1/16 + (12 Funits)*size/UPEm.

      This approach yields more symmetrical results than shrinking
      the outline horizontally alone (see separate papers on the topic). */


      /* Same thing for the height... */
      i = 0;
      cwi = 0;
      ccwi = 0;
      for (path = paths; path && ccwi<MAXTHINPNTS && cwi<MAXTHINPNTS;
      path=path->next) {

         pts = &path->pts[0];

         /* Are the first and last point coinciding? */
         if (pts[path->count-1].y!=pts[0].y ||
             pts[path->count-1].x!=pts[0].x)
            prev = path->count-1;
         else
            prev = path->count-2;

         if (!OnCurve(path->onoff, prev) ||
             (pts[0].y != pts[prev].y &&
              ABS(pts[0].y - pts[prev].y) < ABS(pts[0].x - pts[prev].x)*8)) {
            if (pts[0].x>pts[prev].x+20) {
               if (pts[prev].x<=pts[prev-1].x)
                  cw[cwi++] = i+path->count-1;
               cw[cwi++] = i;
            } else if (pts[0].x<pts[prev].x-20) {
               if (pts[prev].x>=pts[prev-1].x)
                  ccw[ccwi++] = i+path->count-1; 
               ccw[ccwi++] = i;
            }
         }


         for (j=1; j<path->count && ccwi<MAXTHINPNTS && cwi<MAXTHINPNTS; j++) {
            i++;
            if (!OnCurve(path->onoff, j-1) ||
                (pts[j].y != pts[j-1].y &&
                 ABS(pts[j].y - pts[j-1].y) < ABS(pts[j].x - pts[j-1].x)*8)) {
               if (pts[j].x>pts[j-1].x+20) {
                  if (!cwi || cw[cwi-1]!=i-1)
                     cw[cwi++] = i-1;
                  cw[cwi++] = i;
               } else if (pts[j].x<pts[j-1].x-20) {
                  if (!ccwi || ccw[ccwi-1]!=i-1)
                     ccw[ccwi++] = i-1;
                  ccw[ccwi++] = i;
               }
            }
         }
         i++;
      }


      if (ccwi>=MAXTHINPNTS || cwi>=MAXTHINPNTS) {
         LogError(MSG_WARNING, MSG_DIAG, NULL);
      }


      /* Any points to shift? */
      if (cwi || ccwi) {
         pgm[(*pc)++] = op_svcta + SUBOP_Y;


         for (n=0; n<cwi; n=m) {
            for (m=n+1; m<cwi && cw[m]==cw[m-1]+1; m++);
            pgm[(*pc)++] = op_pushb1 + 2;
            pgm[(*pc)++] = cw[n];
            pgm[(*pc)++] = (UBYTE)(m-n-1);
            pgm[(*pc)++] = TTFUN_RANGE;
            pgm[(*pc)++] = op_loopcall;
         }
         pgm[(*pc)++] = op_pushb1+2;
         pgm[(*pc)++] = STORAGE_DIAG;
         pgm[(*pc)++] = cwi;
         pgm[(*pc)++] = TTFUN_SHIFT2;
         pgm[(*pc)++] = op_call;



         /************ Shift back the left side of the glyph. */


         for (n=0; n<ccwi; n=m) {
            for (m=n+1; m<ccwi && ccw[m]==ccw[m-1]+1; m++);
            pgm[(*pc)++] = op_pushb1 + 2;
            pgm[(*pc)++] = (UBYTE)ccw[n];
            pgm[(*pc)++] = (UBYTE)(m-n-1);
            pgm[(*pc)++] = TTFUN_RANGE;
            pgm[(*pc)++] = op_loopcall;
         }
         pgm[(*pc)++] = op_pushb1+2;
         pgm[(*pc)++] = STORAGE_DIAG;
         pgm[(*pc)++] = (UBYTE)ccwi;
         pgm[(*pc)++] = TTFUN_SHIFT1;
         pgm[(*pc)++] = op_call;
      }
#endif

      pgm[(*pc)++] = op_eif;
   }

   /* Args + num of args + function number. */
   return (short)(MAX(cwi, ccwi)+2); 
}




/***
** Function: ScaleDown3
**
** Description:
**   This function generates the TT instructions
**   that will scale down points 3%.
***/
void ScaleDown3(const Extremas *extr, const short xcnt, 
                UBYTE *pgm, short *pc,
                short *args, short *pcd)
{
   short i,j,offset, opc, opcd;

   /* Remember the state of the stacks. */
   opc = (*pc);
   opcd = (*pcd);

   args[(*pcd)++] = TTFUN_SCALE3;

   offset = (*pcd)++;
   args[offset] = 0;
   for (i=0; i<xcnt; i++) {
      if ((extr[i].rp1==UNDEF || extr[i].rp2==UNDEF)) {
         for (j=0; j<extr[i].n; j++) {
            args[(*pcd)++] = extr[i].pts[j];
         }
         args[offset] = (short)(args[offset] + extr[i].n);
      }
   }
   if (args[offset]>0) {
      pgm[(*pc)++] = op_call;
   } else {
      /* Back track. */
      (*pc) = opc;
      (*pcd) = opcd;
   }
}


/***
** Function: EmitIP
**
** Description:
**   This function generates the TT instructions
**   that will interpolate points that are either
**   within or between stem sides.
***/
void EmitIP(const Extremas *extr, const short xcnt, 
            UBYTE *pgm, short *pc,
            short *args, short *pcd,
            const short scale3offset)
{
   short i,j,num;
   short ones[MAXIP], twoes[MAXIP], nths[MAXIP];
   short cnt1, cnt2, cntn;


   /*lint -e530 -e644 */
   /* Shift extrems. */
   cnt1 = 0; cnt2 = 0; cntn = 0; num = 0;
   for (i=0; i<xcnt; i++) {
      short rp;

      /* Skip interpolations. */
      if (extr[i].rp1!=UNDEF && extr[i].rp2!=UNDEF)
         continue;

      /* Set the reference points. */
      if (extr[i].rp1!=UNDEF) {
         rp = (short)(extr[i].rp1+scale3offset);
      }  else {
         rp = (short)(extr[i].rp2+scale3offset);
      }

      if (extr[i].n==1) {
         if ((cnt1+2)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_SHP1;
            args[(*pcd)++] = (short)(cnt1/2);
            for (j=0; j<cnt1; j++)
               args[(*pcd)++] = (short)ones[j];
            cnt1 = 0;
         }
         ones[cnt1++] = rp;
         ones[cnt1++] = extr[i].pts[0];
      } else if (extr[i].n==2) {
         if ((cnt2+3)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_SHP2;
            args[(*pcd)++] = (short)(cnt2/3);
            for (j=0; j<cnt2; j++)
               args[(*pcd)++] = (short)twoes[j];
            cnt2 = 0;
         }
         twoes[cnt2++] = rp;
         twoes[cnt2++] = extr[i].pts[0];
         twoes[cnt2++] = extr[i].pts[1];
      } else {
         if ((cntn+2+extr[i].n)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_SHPN;
            args[(*pcd)++] = num;
            for (j=0; j<cntn; j++)
               args[(*pcd)++] = (short)nths[j];
            cntn = 0;
            num = 0;
         }
         nths[cntn++] = rp;
         nths[cntn++] = extr[i].n;
         for (j=0; j<extr[i].n; j++) {
            nths[cntn++] = extr[i].pts[j];
         }
         num++;
      }
   }

   if (cnt1) {
      if (cnt1>2) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_SHP1;
         args[(*pcd)++] = (short)(cnt1/2);
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_SHP1;
      }
      for (i=0; i<cnt1; i++)
         args[(*pcd)++] = ones[i];
   }
   if (cnt2) {
      if (cnt2>3) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_SHP2;
         args[(*pcd)++] = (short)(cnt2/3);
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_SHP2;
      }
      for (i=0; i<cnt2; i++)
         args[(*pcd)++] = twoes[i];
   }
   if (cntn) {
      if (num>1) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_SHPN;
         args[(*pcd)++] = num;
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_SHPN;
      }
      for (i=0; i<cntn; i++)
         args[(*pcd)++] = (short)nths[i];
   }


   /* Interpolate the extrems. */
   cnt1 = 0; cnt2 = 0; cntn = 0; num = 0;
   for (i=0; i<xcnt; i++) {

      /* Skip interpolations. */
      if (extr[i].rp1==UNDEF || extr[i].rp2==UNDEF)
         continue;

      if (extr[i].n==1) {
         if ((cnt1+3)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_IP1;
            args[(*pcd)++] = (short)(cnt1/2);
            for (j=0; j<cnt1; j++)
               args[(*pcd)++] = (short)ones[j];
            cnt1 = 0;
         }
         ones[cnt1++] = extr[i].rp1;
         ones[cnt1++] = extr[i].rp2;
         ones[cnt1++] = extr[i].pts[0];
      } else if (extr[i].n==2) {
         if ((cnt2+4)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_IP2;
            args[(*pcd)++] = (short)(cnt2/3);
            for (j=0; j<cnt2; j++)
               args[(*pcd)++] = (short)twoes[j];
            cnt2 = 0;
         }
         twoes[cnt2++] = extr[i].rp1;
         twoes[cnt2++] = extr[i].rp2;
         twoes[cnt2++] = extr[i].pts[0];
         twoes[cnt2++] = extr[i].pts[1];
      } else {
         if ((cntn+3+extr[i].n)>=MAXIP) {
            pgm[(*pc)++] = op_loopcall;
            args[(*pcd)++] = TTFUN_IPN;
            args[(*pcd)++] = num;
            for (j=0; j<cntn; j++)
               args[(*pcd)++] = (short)nths[j];
            cntn = 0;
            num = 0;
         }
         nths[cntn++] = extr[i].rp1;
         nths[cntn++] = extr[i].rp2;
         nths[cntn++] = extr[i].n;
         for (j=0; j<extr[i].n; j++) {
            nths[cntn++] = extr[i].pts[j];
         }
         num++;
      }
   }

   if (cnt1) {
      if (cnt1>3) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_IP1;
         args[(*pcd)++] = (short)(cnt1/3);
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_IP1;
      }
      for (i=0; i<cnt1; i++)
         args[(*pcd)++] = (short)ones[i];
   }
   if (cnt2) {
      if (cnt2>4) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_IP2;
         args[(*pcd)++] = (short)(cnt2/4);
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_IP2;
      }
      for (i=0; i<cnt2; i++)
         args[(*pcd)++] = (short)twoes[i];
   }
   if (cntn) {
      if (num>1) {
         pgm[(*pc)++] = op_loopcall;
         args[(*pcd)++] = TTFUN_IPN;
         args[(*pcd)++] = num;
      } else {
         pgm[(*pc)++] = op_call;
         args[(*pcd)++] = TTFUN_IPN;
      }
      for (i=0; i<cntn; i++)
         args[(*pcd)++] = (short)nths[i];
   }
   /*lint +e530 +e644 */
}




/***
** Function: EmitVerticalStem
**
** Description:
**   This function generates the code that
**   will initiate the graphics state of the
**   TrueType interpreter for the grid fitting
**   of vertical stems.
***/
void EmitVerticalStems(UBYTE *pgm, short *pc, short *args, short *pcd)
{
   pgm[(*pc)++] = op_call;
   args[(*pcd)++] = TTFUN_VERTICAL;
}





/***
** Function: EmitHorizontalStem
**
** Description:
**   This function generates the code that
**   will initiate the graphics state of the
**   TrueType interpreter for the grid fitting
**   of vertical stems.
***/
void EmitHorizontalStems(UBYTE *pgm, short *pc, short *args, short *pcd)
{
   pgm[(*pc)++] = op_call;
   args[(*pcd)++] = TTFUN_HORIZONTAL;
}





/***
** Function: EmitVStem
**
** Description:
**   This function generates the code that
**   will create and grid fit points in the
**   twilight zone, corresponding to a vstem.
***/
errcode EmitVStem(UBYTE *pgm, short *pc,
                  short *args, short *pcd,
                  struct T1Metrics *t1m,
                  const funit width,
                  const funit real_side1,
                  const funit real_side2,
                  const funit side1,
                  const funit side2,
                  const short rp,
                  const enum aligntype align,
                  const short ref)
{
                     errcode status = SUCCESS;
                     short w_storage;

                     if ((w_storage = GetVStemWidth(GetWeight(t1m), width))==NOMEM) {
                        SetError(status = NOMEM);
                     } else {

                        pgm[(*pc)++] = op_call;
                        switch (align) {
                           case at_centered:
                              args[(*pcd)++] = TTFUN_VCENTER;
                              args[(*pcd)++] = (short)real_side1;
                              args[(*pcd)++] = (short)real_side2;
                              args[(*pcd)++] = (short)side1;
                              args[(*pcd)++] = (short)side2;
                              args[(*pcd)++] = (short)((side1+side2)/2);
                              args[(*pcd)++] = rp;
                              args[(*pcd)++] = w_storage;
                              break;

                           case at_relative1:
                              args[(*pcd)++] = TTFUN_RELATIVE1V;
                              args[(*pcd)++] = (short)real_side1;
                              args[(*pcd)++] = (short)real_side2;
                              args[(*pcd)++] = (short)side1;
                              args[(*pcd)++] = (short)side2;
                              args[(*pcd)++] = ref;
                              args[(*pcd)++] = rp;
                              args[(*pcd)++] = w_storage;
                              break;

                           case at_relative2:
                              args[(*pcd)++] = TTFUN_RELATIVE2V;
                              args[(*pcd)++] = (short)real_side1;
                              args[(*pcd)++] = (short)real_side2;
                              args[(*pcd)++] = (short)side1;
                              args[(*pcd)++] = (short)side2;
                              args[(*pcd)++] = ref;
                              args[(*pcd)++] = rp;
                              args[(*pcd)++] = w_storage;
                              break;

                           case at_side1:
                           case at_side2:
                              LogError(MSG_WARNING, MSG_ALIGN, NULL);
                              break;
                        }
                     }

                     return status;
} 




/***
** Function: EmitHStem
**
** Description:
**   This function generates the code that
**   will create and grid fit points in the
**   twilight zone, corresponding to a hstem.
***/
errcode EmitHStem(UBYTE *pgm, short *pc,
                  short *args, short *pcd,
                  struct T1Metrics *t1m,
                  const funit width,
                  const funit side1,
                  const funit side2,
                  const short rp,
                  const enum aligntype align,
                  const short ref)
{
   errcode status = SUCCESS;
   short w_storage;

   if ((w_storage = GetHStemWidth(GetWeight(t1m), width))==NOMEM) {
      SetError(status = NOMEM);
   } else {

      pgm[(*pc)++] = op_call;
      switch (align) {

         case at_side1:
            args[(*pcd)++] = TTFUN_SIDE1;
            args[(*pcd)++] = (short)side1;
            args[(*pcd)++] = (short)side2;
            args[(*pcd)++] = ref;
            args[(*pcd)++] = rp;
            args[(*pcd)++] = w_storage;
            break;

         case at_side2:
            args[(*pcd)++] = TTFUN_SIDE2;
            args[(*pcd)++] = (short)side1;
            args[(*pcd)++] = (short)side2;
            args[(*pcd)++] = ref;
            args[(*pcd)++] = rp;
            args[(*pcd)++] = w_storage;
            break;

         case at_relative1:
            args[(*pcd)++] = TTFUN_RELATIVE1H;
            args[(*pcd)++] = (short)side1;
            args[(*pcd)++] = (short)side2;
            args[(*pcd)++] = ref;
            args[(*pcd)++] = rp;
            args[(*pcd)++] = w_storage;
            break;

         case at_relative2:
            args[(*pcd)++] = TTFUN_RELATIVE2H;
            args[(*pcd)++] = (short)side1;
            args[(*pcd)++] = (short)side2;
            args[(*pcd)++] = ref;
            args[(*pcd)++] = rp;
            args[(*pcd)++] = w_storage;
            break;

         case at_centered:
         default:
            args[(*pcd)++] = TTFUN_HCENTER;
            args[(*pcd)++] = (short)side1;
            args[(*pcd)++] = (short)side2;
            args[(*pcd)++] = (short)((side1+side2)/2);
            args[(*pcd)++] = rp;
            args[(*pcd)++] = w_storage;
            break;
      }
   }

   return status;
}






/***
** Function: FamilyCutIn
**
** Description:
**   This function generates a branch in the
**   pre-program. 
***/
USHORT FamilyCutIn(UBYTE *prep,
                   USHORT tp,
                   const short cis)
{
   prep[tp++] = op_mppem;
   if (cis<256) {
      prep[tp++] = op_pushb1; prep[tp++] = (UBYTE)cis;
   } else {
      prep[tp++] = op_pushw1;
      prep[tp++] = HIBYTE(cis);
      prep[tp++] = LOBYTE(cis);
   }
   prep[tp++] = op_lt;
   prep[tp++] = op_if;

   return tp;
}




/***
** Function: SetProjection
**
** Description:
**   This function generates the TrueType code that
**   changes the projection vector in oblique typefaces.
***/
void SetProjection(UBYTE *pgm, short *pc,
                   short *args, short *pcd,
                   const funit x, const funit y)
{
   pgm[(*pc)++] = op_call;
   args[(*pcd)++] = TTFUN_OBLIQUE;
   args[(*pcd)++] = (short)y;
   args[(*pcd)++] = (short)x;
}


/***
** Function: AssembleArgs
**
** Description:
**   This function takes a sequence of arguments and
**   assembles them into a sequence of PUSHB1[], PUSHW1[],
**   NPUSHB[] and NPUSHW[] instructions.
***/
void AssembleArgs(short *args, const short pcd, UBYTE *is, short *cnt)
{
   short bytes;
   short i,j;


   if ((args[pcd-1] <= UCHAR_MAX && args[pcd-1]>=0)) {
      bytes = 1;
   } else {
      bytes = 0;
   }

   for (i=0, j=0; j<pcd; i++) {

      /* Pack a sequence of bytes? */
      if (bytes) {
         if ((i-j)>=255 || i==pcd ||
             (args[pcd-i-1]>UCHAR_MAX || args[pcd-i-1]<0)) {
            bytes = 0;
            if ((i-j)<=8) {
               is[(*cnt)++] = (UBYTE)(op_pushb1 + (i-j) - 1);
            } else {
               is[(*cnt)++] = op_npushb;
               is[(*cnt)++] = (UBYTE)(i-j);
            }
            while (j<i)
               is[(*cnt)++] = (UBYTE)args[pcd-1-j++];
         }

         /* Pack a sequence of words? */
      } else {
         if ((i-j)>=255 || i==pcd || 
             (args[pcd-i-1]<=UCHAR_MAX && args[pcd-i-1]>=0)) {
            bytes = 1;
            if ((i-j)<=8) {
               is[(*cnt)++] = (UBYTE)(op_pushw1 + (i-j) - 1);
            } else {
               is[(*cnt)++] = op_npushw;
               is[(*cnt)++] = (UBYTE)(i-j);
            }
            while (j<i) {
               is[(*cnt)++] = HIBYTE(args[pcd-j-1]);
               is[(*cnt)++] = LOBYTE(args[pcd-j-1]);
               j++;
            }
         }
      }
   }
}

