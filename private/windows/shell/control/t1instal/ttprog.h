/**
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


#ifndef _ARGS
#  define IN  const
#  define OUT
#  define INOUT
#  define _ARGS(arg) arg
#endif

#define HIBYTE(v)	(UBYTE)((USHORT)(v)>>8)
#define LOBYTE(v)	(UBYTE)((USHORT)(v)&0xff)

#define TWILIGHT  0
#define GLYPHZONE 1

#define TMPCVT 0L
#define TMPPNT 0L
#define TMPPNT1 1L

#define STORAGE_DIAG 3

#define ONEPIXEL    64L

#define INFINITY	   (USHORT)SHRT_MAX


#define MAXPTS 10         /* Max num of pts on a stem hint side. */


/* IP bucket - Used for collecting points that should be
interpolated within the zones defined by the stem hints. */
typedef struct Extremas {
   short rp1;
   short rp2;
   short n;
   short pts[MAXPTS];
} Extremas;



enum aligntype  {
   at_side1,
   at_side2,
   at_relative1,
   at_relative2,
   at_centered
};



/***
**
**   Description:
**      This file contains op-codes for a sub-set of the
**      TrueType instruction set.
**
**   Author: Michael Jansson
**
**   Created: 10/14/93
**
***/


/* TrueType sub-op codes. */
#define SUBOP_Y         0x00
#define SUBOP_X         0x01

#define SUBOP_R         0x01

#define SUBOP_MOVE      0x10
#define SUBOP_MINDIST   0x08
#define SUBOP_ROUND     0x04
#define SUBOP_GRAY      0x00

#define SUBOP_mMRGR  SUBOP_MINDIST | SUBOP_ROUND | SUBOP_GRAY
#define SUBOP_MmRGR  SUBOP_MOVE | SUBOP_ROUND | SUBOP_GRAY


/* TrueType op codes. */
enum {        
   op_mps = 0x4c,
   op_spvtl = 0x07,
   op_roll = 0x8a,
   op_gteq = 0x53,
   op_cindex =  0x25,
   op_rtdg = 0x3d,
   op_clear = 0x22,
   op_szp0 = 0x13,
   op_szp1 = 0x14,
   op_szp2 = 0x15,
   op_szps = 0x16,
   op_loopcall = 0x2a,
   op_shz = 0x36,
   op_smd = 0x1a,
   op_rutg = 0x7c,
   op_rdtg = 0x7d,
   op_pop = 0x21,
   op_abs = 0x64,
   op_scvtci = 0x1d,
   op_rs = 0x43,
   op_spvfs = 0x0a,
   op_shp = 0x33,
   op_roff = 0x7a,
   op_md = 0x49,
   op_ssw = 0x1f,
   op_mul = 0x63,
   op_odd = 0x56,
   op_gc = 0x46,
   op_dup = 0x20,
   op_min = 0x8c,
   op_max = 0x8b,
   op_neg = 0x65,
   op_sfvtl = 0x08,
   op_spvtca =  0x06,
   op_swap = 0x23,
   op_mdrp = 0xc0,
   op_mdap = 0x2e,
   op_miap = 0x3e,
   op_mirp = 0xe0,
   op_alignrp = 0x3c,
   op_iup = 0x30,
   op_svcta = 0x00,
   op_sloop = 0x17,
   op_npushb = 0x40,
   op_npushw = 0x41,
   op_mppem = 0x4b,
   op_lt = 0x50,
   op_gt = 0x52,
   op_if = 0x58,
   op_scfs = 0x48,
   op_else = 0x1b,
   op_wcvtf = 0x70,
   op_wcvtp = 0x44,
   op_pushw1 = 0xb8,
   op_pushb1 = 0xb0,
   op_eif = 0x59,
   op_shpix = 0x38,
   op_srp0 = 0x10,
   op_srp1 = 0x11,
   op_srp2 = 0x12,
   op_ip = 0x39,
   op_rcvt = 0x45,
   op_round = 0x68,
   op_rtg = 0x18,
   op_rthg = 0x19,
   op_add = 0x60,
   op_div = 0x62,
   op_scanctrl = 0x85,
   op_ws = 0x42,
   op_sswci = 0x1e,
   op_scantype = 0x8d,
   op_sub = 0x61,
   op_fdef = 0x2c,
   op_endf = 0x2d,
   op_call = 0x2b,
   op_getinfo = 0x88
};



/***
** Function: GetTopPos
**
** Description:
**   This function allocates a cvt entry for the 
**   top side of a horizontal stem;
***/
short	       GetTopPos	    _ARGS((IN	   Blues *blues,
					   INOUT   AlignmentControl *align,
					   IN	   funit pos));
/***
** Function: GetBottomPos
**
** Description:
**   This function allocates a cvt entry for the 
**   top side of a horizontal stem;
***/
short	       GetBottomPos	    _ARGS((IN	   Blues *blues,
					   INOUT   AlignmentControl *align,
					   IN	   funit pos));
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
USHORT	       CutInSize	    _ARGS((IN	   funit width,
					   IN	   funit master,
					   IN	   USHORT tresh,
					   IN	   funit upem));

/***
** Function: SnapStemArgs
**
** Description:
**   
***/
USHORT	       SnapStemArgs	    _ARGS((OUT	   short *args,
					   INOUT   USHORT ta,
					   IN	   funit width,
					   IN	   USHORT std_cvt,
					   IN	   USHORT snap_cvt,
					   IN	   USHORT std_ci,
					   IN	   USHORT snap_ci,
					   IN	   USHORT storage));
/***
** Function: StdStemArgs
**
** Description:
**   
***/
USHORT	       StdStemArgs	    _ARGS((OUT	   short *args,
					   INOUT   USHORT ta,
					   IN	   funit width,
					   IN	   USHORT std_cvt,
					   IN	   USHORT std_ci,
					   IN	   USHORT storage));
/***
** Function: CreateStdStems
**
** Description:
**   
***/
USHORT	       CreateStdStems	    _ARGS((INOUT   UBYTE *prep,
					   INOUT   USHORT tp,
					   IN	   short cnt));
/***
** Function: CreateSnapStems
**
** Description:
**   
***/
USHORT	       CreateSnapStems	    _ARGS((INOUT   UBYTE *prep,
					   INOUT   USHORT tp,
					   IN	   short cnt));
/***
** Function: tt_GetFontProg
**
** Description:
**   This function returns the static font
**   font program.
***/
const UBYTE    *tt_GetFontProg	    _ARGS((void));


/***
** Function: tt_GetNumFuns
**
** Description:
**   This function returns the number of functions
**   defined in the static font program.
***/
USHORT	       tt_GetNumFuns	    _ARGS((void));


/***
** Function: tt_GetFontProgSize
**
** Description:
**   This function returns the size of the
**   static font program.
***/
USHORT	       tt_GetFontProgSize   _ARGS((void));


/***
** Function: SetZone
**
** Description:
**   This function initiate an alignment zone
**   by creating an appropriate point in the
**   twilight zone.
***/
USHORT	       SetZone		    _ARGS((INOUT   UBYTE *prep,
					   INOUT   USHORT tp,
					   IN	   short cvt));
/***
** Function: CopyZone
**
** Description:
**   This function copies a cvt entry, representing an
**   alignment zone, to the cvt used for a particular hstem.
***/
USHORT	       CopyZone		    _ARGS((INOUT   UBYTE *prep,
					   INOUT   short tp,
					   INOUT   short *args,
					   IN	   short ta));
/***
** Function: CopyFamilyBlue
**
** Description:
**   This function copies a cvt entry, representing a
**   family blue zone, to the cvt used for a particular hstem.
***/
USHORT	       CopyFamilyBlue	    _ARGS((INOUT   UBYTE *prep,
					   INOUT   short tp,
					   INOUT   short *args,
					   IN	   short ta));
/***
** Function: AlignFlat
**
** Description:
**   This function creates a cvt entry for
**   a particular hstem.
***/
USHORT	       AlignFlat	    _ARGS((INOUT   UBYTE *prep,
					   INOUT   short tp,
					   INOUT   short *args,
					   IN	   short ta));
/***
** Function: AlignOvershoot
**
** Description:
**   This function creates a cvt entry for
**   a particular hstem.
***/
USHORT	       AlignOvershoot	    _ARGS((INOUT   UBYTE *prep,
					   INOUT   short tp,
					   INOUT   short *args,
					   IN	   short ta));
/***
** Function: EmitFlex
**
** Description:
**   Convert a T1 flex hint into a TrueType IP[] 
**   intruction sequence that will reduce a flex
**   that is flatter than a given height.
***/
errcode	       EmitFlex		    _ARGS((INOUT   short *args,
					   INOUT   short *pcd,
					   IN	   funit height,
					   IN	   short start,
					   IN	   short mid,
					   IN	   short last));
/***
** Function: ReduceDiagonals
**
** Description:
**   This function generates the TT instructions
**   that will shrink the outline, in order to
**   control the width of diagonals. This implementation
**   can probably be improved.
***/
short	       ReduceDiagonals	    _ARGS((IN	   Outline *paths,
					   INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd));
/***
** Function: ScaleDown3
**
** Description:
**   This function generates the TT instructions
**   that will scale down points 3%.
***/
void	       ScaleDown3	    _ARGS((IN	   Extremas *extr,
					   IN	   short xcnt, 
					   INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd));
/***
** Function: EmitIP
**
** Description:
**   This function generates the TT instructions
**   that will interpolate points that are either
**   within or between stem sides.
***/
void	       EmitIP		    _ARGS((IN	   Extremas *extr,
					   IN	   short xcnt, 
					   INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd,
					   IN	   short scale3offset));
/***
** Function: EmitVerticalStem
**
** Description:
**   This function generates the code that
**   will initiate the graphics state of the
**   TrueType interpreter for the grid fitting
**   of vertical stems.
***/
void	       EmitVerticalStems    _ARGS((INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd));
/***
** Function: EmitHorizontalStem
**
** Description:
**   This function generates the code that
**   will initiate the graphics state of the
**   TrueType interpreter for the grid fitting
**   of vertical stems.
***/
void	       EmitHorizontalStems  _ARGS((INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd));
/***
** Function: EmitVStem
**
** Description:
**   This function generates the code that
**   will create and grid fit points in the
**   twilight zone, corresponding to a vstem.
***/
errcode	       EmitVStem	    _ARGS((INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd,
					   INOUT   struct T1Metrics *t1m,
					   IN	   funit width,
					   IN      funit real_side1,
					   IN      funit real_side2,
					   IN      funit side1,
					   IN      funit side2,
					   IN      short rp,
					   IN      enum aligntype align,
					   IN      short ref));
/***
** Function: EmitHStem
**
** Description:
**   This function generates the code that
**   will create and grid fit points in the
**   twilight zone, corresponding to a hstem.
***/
errcode	       EmitHStem	    _ARGS((INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd,
					   INOUT   struct T1Metrics *t1m,
					   IN	   funit width,
					   IN      funit side1,
					   IN      funit side2,
					   IN      short rp,
					   IN      enum aligntype align,
					   IN      short ref));
/***
** Function: FamilyCutIn
**
** Description:
**   This function generates a branch in the
**   pre-program. 
***/
USHORT	       FamilyCutIn	    _ARGS((INOUT   UBYTE *pgm,
					   INOUT   USHORT tp,
					   IN	   short cis));
/***
** Function: SetProjection
**
** Description:
**   This function generates the TrueType code that
**   changes the projection vector in oblique typefaces.
***/
void	       SetProjection	    _ARGS((INOUT   UBYTE *pgm,
					   INOUT   short *pc,
					   INOUT   short *args,
					   INOUT   short *pcd,
					   IN	   funit x,
					   IN	   funit y));
/***
** Function: AssembleArgs
**
** Description:
**   This function takes a sequence of arguments and
**   assembles them into a sequence of PUSHB1[], PUSHW1[],
**   NPUSHB[] and NPUSHW[] instructions.
***/
void           AssembleArgs         _ARGS((INOUT   short *args,
                                           IN      short pcd,
                                           OUT     UBYTE *is,
                                           INOUT   short *cnt));

