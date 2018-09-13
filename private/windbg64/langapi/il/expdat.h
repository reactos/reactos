/* static char *SCCSID = "@(#)expdat.h:1.14"; */
/*
**
** IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
** if any changes are made to this file, the ILID_P1 constant in
** $(LANGAPI)\il\ilvers.h should be changed to reflect the date of the change
**
*/

#ifndef _VC_VER_INC
#include "..\include\vcver.h"
#endif

/*
 * The expdope vector is indexed by opcode and describes the attributes
 * of the node. The makefile ensures that changes to the expdope vector
 * will create the enumerations of the opcodes in opcode.h.
 * The expression IL attributes are defined as follows (P1 uses the numeric
 * encoding in parentheses, QC and CMERGE P2 use the letter coding - I wasn't
 * able to map all of the attributes, however -- markle 04/04/90):
 *
 *  a (05) - case value								(long)
 *  c (01) - copy between storage units             (char)
 *  i (02) - integer constant                       (long)
 *  f (03) - function/procedure call type           (char)
 *  n (04) - pragma number + other info             (short)<see pragma.c>
 *  o (20) - overflow check enable/disable          (char)
 *  p (06) - primitive data type                    (compacted short, <long>)
 *  d (07) - switch default enable/disable          (char)
 *  s (010)- reference to symbol stream             (key)
 *  S (011)- three symbol keys (fortran arith if)   (key)(key)(key)
 *  t (???)- interference type key                  (long)
 *  v (013)- constant type and value                (char)(short)<(long)OR(double)>
 *  x (014)- number of nodes in next tree (OPtrsize)    (short)
 *  w (015)- weighting factor                       (char)
 *  z (016)- rounding rules                         (char)
 *  X (017)- characteristics
 *  ? (020)-
 *    (021)- temporary key number (OPallotemp)      (key)
 *  ? (023)- sy file offset                         (long)
 *  ? (022)-
 *	  (024)- eh state attributes (OPpushstate etc.)	(long)
 *	  (025)- OPcatch attributes   					(key, key, short)
 *	  (026)- eh state count							(long)
 *	A (027)- OPargument align info (Alpha only) 	(char)
 *	B (030)- OPindex align info (Alpha only) 	(char)
 *	C (031)- inline asm char str   (Alpha only)     (char)
 */

#define L1 LEAF
#define U1 UNARY
#define B1 BINARY|HAS_RC
#define H1 HYBRID|HAS_RC
#define E1 ENDEXPR
#define CE CSEE
#define R CSER
#define F FOLD
#define S SORT
#define M COMM
#define T CSET
#define K KILL
#define C1 COLEAF
#define AS ASSIG
#define RE RELA
#define H HAS_RC
#define LV LVALUE

/*
* want to (someday) differentiate between side-effect (T2_NKILL) which
* effects rewrites and cse-killer (T2_NKCSE) which effects CSEs.  The
* former is 'dumb' and assumes a kill kills everything; the latter is
* 'smart' and tries to kill as little as possible.
*/
#define B2  T2_NBINARY
#define U2  T2_NUNARY
#define L2  T2_NLEAF
#define H2  T2_NHYBRID
#define KC  T2_NKCSE
#define KI  T2_NKILL
#define NC  T2_NOCSE
#define AG  T2_NASG
#define SK  T2_NSKIPCALC
#define CO  T2_NCODOPA
#define T2_NKCSE    0       /* M00TODO unused */
#define NP  T2_NOP2


#define QEXPDAT(a1, a2, a3, a4, a5)

#if VERSP_RISC && (! VERSP_ALPHA)
#define MEXPDAT EXPDAT
#else
#define MEXPDAT(a1, a2, a3, a4, a5)
#endif

#if VERSP_ALPHA
#define AEXPDAT EXPDAT
#else
#define AEXPDAT(a1, a2, a3, a4, a5)
#endif

#if (!(VERSP_QC2PASS || VERSP_C1) && !VERSP_ALPHA)
#define CEXPDAT EXPDAT
#else
#define CEXPDAT(a1, a2, a3, a4, a5)
#endif


/*  NAME      P1 NODE ATTRIBUTES    P1-IL ATTRIBUTES    */
EXPDAT(OPerror,     L1|E1,          "\017",         L2,             STR_X)
EXPDAT(OPnull,      L1|E1,          "\017",         L2|SK|NP,       STR_X)

/* arithmetic operators */
EXPDAT(OPplus,      B1|CE|R|S|M|F|T,NULL,           B2,             NULL)
EXPDAT(OPminus,     B1|CE|R|F|T,    NULL,           B2,             NULL)
EXPDAT(OPmult,      B1|CE|R|S|M|F,  NULL,           B2,             NULL)
EXPDAT(OPdiv,       B1|CE|R|F,      NULL,           B2|NC,          NULL)
EXPDAT(OPrem,       B1|CE|R|F,      NULL,           B2|NC,          NULL)
EXPDAT(OPexp,       B1|CE|R|F,      NULL,           B2,             NULL)
EXPDAT(OPneg,       U1|CE|R|F,      NULL,           U2,             NULL)
EXPDAT(OPlshift,    B1|CE|R|F,      NULL,           B2,             NULL)
EXPDAT(OPrshift,    B1|CE|R|F,      NULL,           B2,             NULL)
EXPDAT(OPband,      B1|CE|R|S|M|F,  NULL,           B2,             NULL)
EXPDAT(OPbor,       B1|CE|R|S|M|F,  NULL,           B2,             NULL)
EXPDAT(OPxor,       B1|CE|R|S|M|F,  NULL,           B2,             NULL)
EXPDAT(OPcompl,     U1|CE|R|F,      NULL,           U2,             NULL)

/* operators using opeq shorthand */
EXPDAT(OPpluseq,    LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPminuseq,   LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPmulteq,    LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPdiveq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPremeq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPexpeq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPlsheq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPrsheq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPandeq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPxoreq,     LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPoreq,      LV|B1|K|R|AS,   "\06",          B2|KI|KC|AG,    STR_p)

/* relational operators */
EXPDAT(OPnot,       U1|CE|R|F,      NULL,           U2,             NULL)
EXPDAT(OPorelse,    B1,             NULL,           B2|SK,          NULL)
EXPDAT(OPandif,     B1,             NULL,           B2|SK,          NULL)
EXPDAT(OPand,       B1|F|R,         NULL,           B2,             NULL)
EXPDAT(OPor,        B1|F|R,         NULL,           B2,             NULL)
EXPDAT(OPeq,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPne,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPle,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPlt,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPge,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPgt,        B1|RE|S|R|CE,   NULL,           B2|SK,          NULL)
EXPDAT(OPov,        U1|R|CE,        NULL,           U2|SK,          NULL)

/* names */
EXPDAT(OPname,      L1|CE|C1,       "\010",         L2,             STR_s)
EXPDAT(OPfield,     B1,             "\006",         B2,             STR_s)
#if VERSP_ALPHA
EXPDAT(OPindex, 	B1, 			"\010\030", 	B2, 			STR_s)
#else
EXPDAT(OPindex,     B1,             "\010",         B2,             STR_s)
#endif
EXPDAT(OPlabel,     L1|E1,          "\010\017",     L2|KI|KC|CO,    STR_sX)
EXPDAT(OPreg,       L1|E1,          "\010\06\015\01\017", L2,       "spwcX")
EXPDAT(OPdereg,     L1|E1,          "\010\01\017",  L2,             "scX")
EXPDAT(OPcast,      U1,             "\06\016",      U2,             STR_pz)
EXPDAT(OPparameter, L1|E1,          "\010\017",     L2,             STR_sX)
EXPDAT(OPfstring,   H1|R,           NULL,           H2,             NULL)
EXPDAT(OPfindex,    H1|R,           "\06",          H2,             STR_p)

/* data movement and conversion */
EXPDAT(OPextract,   U1|CE|R|LV,     "\06",          U2,             STR_p)
EXPDAT(OPunop,      U1,             "\06",          U2,             STR_p)
EXPDAT(OPassign,    B1|K|R|AS|LV,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPconstant,  L1|CE|R|C1,     "\013",         L2,             "v")
EXPDAT(OPconvert,   U1|CE|R|F,      "\06\016",      U2,             STR_pz)
EXPDAT(OPpostincr,  B1|K|R|AS|LV,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPpostdecr,  B1|K|R|AS|LV,   "\06",          B2|KI|KC|AG,    STR_p)
EXPDAT(OPbit,       B1|CE|R,        NULL,           B2,             NULL)

/* control flow */
EXPDAT(OPcbranch,   U1|R|E1,        "\010\017",     U2|CO|SK,       STR_sX)
EXPDAT(OPnbranch,   U1|R|E1,        "\010\017",     U2|SK,          STR_sX)
EXPDAT(OPgoto,      L1|E1,          "\010\017",     L2|KC|CO,       STR_sX)
EXPDAT(OPswexp,     U1|R|E1,        "\010\017",     U2|KC|CO,       STR_sX)
#if VERSP_MIPS
EXPDAT(OPswitch,    U1|E1,          "\06\010\017",  H2|KC|CO,       "pts")
#elif VERSP_ALPHA
// 01/10/96 Phil Hutchinson - Merged FE IL and BE IL for Alpha
// OPswitch is a HYBRID, for we associate with it a list
EXPDAT(OPswitch,    H1|E1,          "\06\010\017",  H2|KC|CO,       "pts")
#else
EXPDAT(OPswitch,    L1|E1,          "\06\010\017",  H2|KC|CO,       "pts")
#endif
// 12/17/93 Jan de Rie
// Eventually, remove the \05 from the attribute string.
// For now, we allow the case value to accomodate the dumpers
//
// 12/28/93 TGL
// The MIPS BE meeds U1 to read the IL properly. Furthermore, I don't think the
// 'else' part will work at all for anybody how depend on reading the IL based on
// this file. I know, the Intel BE don't care, they hard-code their own dope-vectors.
// I made this temporary change to be able to build the MIPS compiler and not breaking whoever
// was targeted by the 'else' part.
#if CC_C9IL && 0 || (VERSP_RISC && (! VERSP_ALPHA))
EXPDAT(OPcase,      U1|R|E1,        "\010\017",     L2|KC|CO,       "saX")
#elif VERSP_ALPHA
// Don't know if ALPHA could use the CSER as well....
EXPDAT(OPcase,      U1|E1,          "\010\017",     L2|KC|CO,       "saX")
#else
EXPDAT(OPcase,      L1|E1,          "\010\05\017",  L2|KC|CO,       "saX")
#endif
EXPDAT(OPfunction,  H1|K|R,         "\06\03",       H2|KI|KC|SK,    "pf")
EXPDAT(OPprocedure, H1|K|R,         "\03",          H2|KI|KC,       "f")
EXPDAT(OPintrinsic, H1|K|R,         "\06",          H2|KI|KC,       STR_p)
EXPDAT(OPreturn,    U1|R|E1,        "\06\017",      U2|SK,          STR_pX)
EXPDAT(OPquestion,  B1|R|CE,        NULL,           B2|SK,          NULL)
EXPDAT(OPcolon,     B1|R|CE,        NULL,           B2,             NULL)
EXPDAT(OPcomma,     B1,             NULL,           B2,             NULL)
EXPDAT(OPfaif,      U1|R|E1,        "\011\017",     U2,             "SX")

/* procedure/function definition */
EXPDAT(OPentry,     H1|E1,          "\017",         H2|SK,          STR_X)
EXPDAT(OPexit,      L1|E1,          "\017",         L2|SK,          STR_X)
EXPDAT(OPfentry,    H1|E1,          "\017",         H2,             STR_X)
EXPDAT(OPfexit,     L1|E1,          "\010\017",     L2,             STR_sX)
EXPDAT(OPfmap,      H1|E1,          "\017",         H2,             STR_X)

/* IL control */
EXPDAT(OPexpression,U1|E1,          "\017",         L2,             STR_X)
EXPDAT(OPeolist,    L1|E1|C1,       "\017",         L2,             STR_X)
EXPDAT(OPeofile,    L1|E1,          NULL,           L2,             NULL)
EXPDAT(OPeostring,  L1,             NULL,           L2,             NULL)
EXPDAT(OPpragma,    L1|E1,          "\04\017",      L2|NP,          "nX")
EXPDAT(OPmark,      U1|R,           "\02",          U2,             STR_i)
EXPDAT(OPuse,       L1,             "\02",          L2,             STR_i)
EXPDAT(OPrelease,   L1|E1,          "\02\017",      L2,             STR_iX)
EXPDAT(OPblock,     L1|E1,          "\017",         L2|NP,          STR_X)
EXPDAT(OPendblock,  L1|E1,          "\02\017",      L2|NP,          STR_iX)
#if VERSP_ALPHA
EXPDAT(OPargument,	U1|H|R|E1,		"\06\017\027",	U2, 			STR_pX)
#else
EXPDAT(OPargument,	U1|H|R|E1,		"\06\017",		U2, 			STR_pX)
#endif
EXPDAT(OPfile,      L1|E1,          "\017",         L2,             STR_X)
EXPDAT(OPlist,      U1|H|R|E1,      "\017",         U2,             STR_X)
EXPDAT(OPaddress,   U1,             "\06",          U2,             STR_p)
EXPDAT(OPparen,     U1|R|CE,        NULL,           U2,             NULL)

/* Misc. */
EXPDAT(OPcextract,  U1|CE|R|LV,     "\06",          U2,             STR_p)
EXPDAT(OPfseek,     L1,             "\02",          L2,             STR_i)

/* EH state and flow control nodes (may be moved around later -- 072393) */
EXPDAT(OPpushstate,	B1,				"\06\024",		B2,				STR_00)
EXPDAT(OPpopstate,	L1,				"\026\024",		L2,				STR_01)
EXPDAT(OPdtoraction,L1,				"\026\024",		L2,				STR_01)
EXPDAT(OPspare010,	L1,				NULL,			L2,				NULL)
EXPDAT(OPehtry,     L1|E1,          "\017",         L2,             STR_i)
EXPDAT(OPcatch,     L1|E1,          "\025",         L2,             NULL)
EXPDAT(OPehendtry,  L1|E1,          "\017",         L2,             NULL)
EXPDAT(OPcatchret,  L1|E1,          "\010",         L2,             NULL)

/* mostly spare nodes */
EXPDAT(OPargumentudt, U1|H|R|E1,    "\06\017",      U2,             STR_pX)
EXPDAT(OPparameterudt, L1|E1,       "\010\017",     L2,             STR_sX)
EXPDAT(OPspare013,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare014,  L1,             NULL,           L2,             NULL)
EXPDAT(OPlongaddr,  B1|CE|R,        NULL,           B2,             NULL)
EXPDAT(OPsegnum,    L1|CE|R|C1,     "\010",         L2,             "vs")
EXPDAT(OPspare015,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare016,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare017,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare018,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare019,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare020,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare021,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare022,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare023,  L1,             NULL,           L2,             NULL)
EXPDAT(OPspare024,  L1,             NULL,           L2,             NULL)

EXPDAT(OPlogshift,  B1|CE|R|F,      NULL,           B2,             NULL)
EXPDAT(OParshift,   B1|CE|R,        NULL,           B2,             NULL)
EXPDAT(OProtate,    B1|CE|R,        NULL,           B2,             NULL)
EXPDAT(OPptradd,    B1,             "\06",          B2,             STR_p)
EXPDAT(OPdoloop,    H1|K|R|E1,      "\06\07\017",   H2|KI|KC,       "pdX")
EXPDAT(OPdbcnv,     U1|R|F,         NULL,           U2,             NULL)
EXPDAT(OPmathintr,  H1|R|CE,        NULL,           H2,             NULL)
EXPDAT(OPrngge,     B1|R,           NULL,           B2,             NULL)
EXPDAT(OPrngle,     B1|R,           NULL,           B2,             NULL)
EXPDAT(OPfpassign,  B1|K|R|AS|LV,   "\06",          B2|KI|KC,       STR_p)
EXPDAT(OPrtcall,    H1|K|R,         "\06\03",       H2|KI|KC,       "pf")
EXPDAT(OPtrsize,    L1|E1,          "\014\017",     L2,             STR_X)
EXPDAT(OPnolength,  L1,             NULL,           L2,             NULL)
EXPDAT(OPzplus,     B1|CE|R|S|M|F|T,"\020",         B2,             NULL)
EXPDAT(OPzminus,    B1|CE|R|S|F|T,  "\020",         B2,             NULL)
EXPDAT(OPzmult,     B1|CE|R|S|M|F,  "\020",         B2,             NULL)
EXPDAT(OPabs,       U1|CE|R|F,      NULL,           U2,             NULL)
EXPDAT(OPself,      L1,             NULL,           L2,             NULL)
EXPDAT(OPseg,       U1,             NULL,           U2,             NULL)
EXPDAT(OPregopeq,   B1|K|R|AS|LV,   "p",            B2,             "p")
EXPDAT(OPfirstreg,  U1,             NULL,           U2,             NULL)
EXPDAT(OPlastreg,   U1,             NULL,           U2,             NULL)
EXPDAT(OPlongaddrx, B1|CE|R,        NULL,           B2,             NULL)
EXPDAT(OPbaseaddr,  B1|CE|R,        NULL,           B2,             NULL)
EXPDAT(OPspare025,  L1,             NULL,           U2,             NULL)
EXPDAT(OPexcept,    U1|R|E1,        "\06\017",      L2,             NULL)
EXPDAT(OPfinally,   L1|E1,          "\017",         L2,             NULL)
EXPDAT(OPtry,       L1|E1,          "\017",         L2,             NULL)
EXPDAT(OPtryend,    L1|E1,          "\017",         L2,             NULL)
/* QC specific operators */
EXPDAT(OPenloopL,   L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPdeloopU,   L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPenloopS,   L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPdeloopR,   L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPbrkloopU,  L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPbrkloopR,  L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPcontloopU, L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPcontloopR, L1|E1,          NULL,           L2|NP,          NULL)
EXPDAT(OPmfunc,     B1,             "\006",         B2,             STR_p)
EXPDAT(OPvfunc,     B1,             "\006",         B2,             STR_p)
EXPDAT(OPallotemp,  L1|C1,          "\006\021",     L2,             STR_p)
EXPDAT(OPltor,      B1,             "\006",         L2,             STR_p)
EXPDAT(OPthunkx,    H1|E1,          "\017",         H2,             NULL)

/*
** Pass 2 specific operators
** This area can be overlapped by CMERGE P2 and QC P2 and CENTAUR.
** As long as CMERGE P2 does not use this file, they must make sure
** their number of spare real IL opcodes matches the definitions above.
*/
QEXPDAT(OPtreg,     L1,             NULL,           L2,             NULL)
QEXPDAT(OPfrc,      U1,             NULL,           U2,             NULL)
QEXPDAT(OPfrcfunc,  U1,             NULL,           U2,             NULL)
QEXPDAT(OPfrcmem,   U1,             NULL,           U2,             NULL)
QEXPDAT(OPeff,      L1,             NULL,           L2,             NULL)
QEXPDAT(OPstkvar,   L1,             NULL,           L2,             NULL)
QEXPDAT(OPregister, L1|CE|R,        NULL,           L2,             NULL)
QEXPDAT(OPlregister,L1|CE|R|C1,     NULL,           L2,             NULL)
QEXPDAT(OP2extrac,  L1,             NULL,           L2,             NULL)
QEXPDAT(OP2extrad,  L1,             NULL,           L2,             NULL)

CEXPDAT(OPllongaddr,B1|CE|R,        NULL,           B2,             NULL)
CEXPDAT(OPthunk,    L1,             NULL,           L2,             NULL)
CEXPDAT(OPfar16,    L1,             NULL,           L2,             NULL)
CEXPDAT(OPregister, L1|CE|R,        NULL,           L2,             NULL)
CEXPDAT(OPlregister,L1|CE|R|C1,     NULL,           L2,             NULL)
CEXPDAT(OPinreg,    L1,             NULL,           L2,             NULL)
CEXPDAT(OPfpreg,    U1,             NULL,           L2,             NULL)
CEXPDAT(OPeffaddr,  L1,             NULL,           L2,             NULL)
CEXPDAT(OPrngone,   B1|R,           NULL,           B2,             NULL)
CEXPDAT(OPdup,      U1,             NULL,           U2,             NULL)
CEXPDAT(OPintemp,   L1,             NULL,           L2,             NULL)
CEXPDAT(OPonstack,  L1,             NULL,           L2,             NULL)
CEXPDAT(OPinvert,   U1|CE|R|F,      NULL,           U2,             NULL)
CEXPDAT(OPlgrshift, B1|CE|R|F,      NULL,           B2,             NULL)
CEXPDAT(OPrrotate,  B1|CE|R,        NULL,           B2,             NULL)
CEXPDAT(OPlrotate,  B1|CE|R,        NULL,           B2,             NULL)
CEXPDAT(OPmax,      B1|CE|R|F|S,    NULL,           B2,             NULL)
CEXPDAT(OPmin,      B1|CE|R|F|S,    NULL,           B2,             NULL)
CEXPDAT(OPkilled,   U1,             NULL,           U2,             NULL)
CEXPDAT(OPspare026, L1,             NULL,           U2,             NULL)
CEXPDAT(OPspare027, L1,             NULL,           U2,             NULL)
CEXPDAT(OPspare028, L1,             NULL,           U2,             NULL)

MEXPDAT(OPmsufunc,  U1,             NULL,           U2,             NULL)
MEXPDAT(OPmsuunwind,U1,             NULL,           U2,             NULL)
MEXPDAT(OPmsuexcode,L1,             NULL,           L2,             NULL)
MEXPDAT(OPmsulab,   L1,             NULL,           L2,             NULL)
MEXPDAT(OPmsuassign,U1,             NULL,           U2,             NULL)

// OPasmstr only used by VERSP_ALPHA, but referenced in tokdat.h, so it is in all versions
EXPDAT(OPasmstr,   L1,            "\031",          L2,             NULL)

EXPDAT(OPmaxopcode, L1,             NULL,           L2,             NULL)

#undef L1
#undef U1
#undef B1
#undef H1
#undef E1
#undef CE
#undef R
#undef F
#undef S
#undef M
#undef T
#undef K
#undef C1
#undef AS
#undef RE
#undef B2
#undef U2
#undef L2
#undef H2
#undef KC
#undef KI
#undef NC
#undef AG
#undef SK
#undef CO
#undef H
#undef LV
