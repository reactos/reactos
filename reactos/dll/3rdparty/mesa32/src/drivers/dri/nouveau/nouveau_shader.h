#ifndef __SHADER_COMMON_H__
#define __SHADER_COMMON_H__

#include "mtypes.h"
#include "bufferobj.h"

#define NVSDBG(fmt, args...) do {                             \
	if (NOUVEAU_DEBUG & DEBUG_SHADERS) {                  \
		fprintf(stderr, "%s: "fmt, __func__, ##args); \
	}                                                     \
} while(0)

typedef struct _nvsFunc nvsFunc;

#define NVS_MAX_TEMPS   32
#define NVS_MAX_ATTRIBS 16
#define NVS_MAX_CONSTS  256
#define NVS_MAX_ADDRESS 2
#define NVS_MAX_INSNS   4096

typedef struct _nvs_fragment_header {
   struct _nvs_fragment_header *parent;
   struct _nvs_fragment_header *prev;
   struct _nvs_fragment_header *next;
   enum {
      NVS_INSTRUCTION,
      NVS_BRANCH,
      NVS_LOOP,
      NVS_SUBROUTINE
   } type;
} nvsFragmentHeader;

typedef union {
	struct {
		GLboolean uses_kil;
		GLuint    num_regs;
	} NV30FP;
	struct {
		uint32_t vp_in_reg;
		uint32_t vp_out_reg;
		uint32_t clip_enables;
	} NV30VP;
} nvsCardPriv;

typedef struct _nouveauShader {
   union {
      struct gl_vertex_program vp;
      struct gl_fragment_program fp;
   } mesa;
   GLcontext *ctx;
   nvsFunc *func;

   /* State of the final program */
   GLboolean error;
   GLboolean translated;
   GLboolean on_hardware;
   unsigned int *program;
   unsigned int program_size;
   unsigned int program_alloc_size;
   unsigned int program_start_id;
   unsigned int program_current;
   struct gl_buffer_object *program_buffer;
   int		inst_count;

   nvsCardPriv card_priv;
   int		vp_attrib_map[NVS_MAX_ATTRIBS];

   struct {
      GLboolean in_use;

      GLfloat  *source_val;	/* NULL if invariant */
      float	val[4];
      /* Hardware-specific tracking, currently only nv30_fragprog
       * makes use of it.
       */
      int	*hw_index;
      int        hw_index_cnt;
   } params[NVS_MAX_CONSTS];
   int param_high;

   /* Pass-private data */
   void *pass_rec;

   nvsFragmentHeader *program_tree;
} nouveauShader, *nvsPtr;

typedef enum {
   NVS_FILE_NONE,
   NVS_FILE_TEMP,
   NVS_FILE_ATTRIB,
   NVS_FILE_CONST,
   NVS_FILE_RESULT,
   NVS_FILE_ADDRESS,
   NVS_FILE_UNKNOWN
} nvsRegFile;

typedef enum {
   NVS_OP_UNKNOWN = 0,
   NVS_OP_NOP,
   NVS_OP_ABS, NVS_OP_ADD, NVS_OP_ARA, NVS_OP_ARL, NVS_OP_ARR,
   NVS_OP_BRA, NVS_OP_BRK,
   NVS_OP_CAL, NVS_OP_CMP, NVS_OP_COS,
   NVS_OP_DDX, NVS_OP_DDY, NVS_OP_DIV, NVS_OP_DP2, NVS_OP_DP2A, NVS_OP_DP3,
   NVS_OP_DP4, NVS_OP_DPH, NVS_OP_DST,
   NVS_OP_EX2, NVS_OP_EXP,
   NVS_OP_FLR, NVS_OP_FRC,
   NVS_OP_IF,
   NVS_OP_KIL,
   NVS_OP_LG2, NVS_OP_LIT, NVS_OP_LOG, NVS_OP_LOOP, NVS_OP_LRP,
   NVS_OP_MAD, NVS_OP_MAX, NVS_OP_MIN, NVS_OP_MOV, NVS_OP_MUL,
   NVS_OP_NRM,
   NVS_OP_PK2H, NVS_OP_PK2US, NVS_OP_PK4B, NVS_OP_PK4UB, NVS_OP_POW,
   NVS_OP_POPA, NVS_OP_PUSHA,
   NVS_OP_RCC, NVS_OP_RCP, NVS_OP_REP, NVS_OP_RET, NVS_OP_RFL, NVS_OP_RSQ,
   NVS_OP_SCS, NVS_OP_SEQ, NVS_OP_SFL, NVS_OP_SGE, NVS_OP_SGT, NVS_OP_SIN,
   NVS_OP_SLE, NVS_OP_SLT, NVS_OP_SNE, NVS_OP_SSG, NVS_OP_STR, NVS_OP_SUB,
   NVS_OP_SWZ,
   NVS_OP_TEX, NVS_OP_TXB, NVS_OP_TXD, NVS_OP_TXL, NVS_OP_TXP,
   NVS_OP_UP2H, NVS_OP_UP2US, NVS_OP_UP4B, NVS_OP_UP4UB,
   NVS_OP_X2D, NVS_OP_XPD,
   NVS_OP_EMUL
} nvsOpcode;

typedef enum {
   NVS_PREC_FLOAT32,
   NVS_PREC_FLOAT16,
   NVS_PREC_FIXED12,
   NVS_PREC_UNKNOWN
} nvsPrecision;

typedef enum {
   NVS_SWZ_X = 0,
   NVS_SWZ_Y = 1,
   NVS_SWZ_Z = 2,
   NVS_SWZ_W = 3
} nvsSwzComp;

typedef enum {
   NVS_FR_POSITION	= 0,
   NVS_FR_WEIGHT	= 1,
   NVS_FR_NORMAL	= 2,
   NVS_FR_COL0		= 3,
   NVS_FR_COL1		= 4,
   NVS_FR_FOGCOORD	= 5,
   NVS_FR_TEXCOORD0	= 8,
   NVS_FR_TEXCOORD1	= 9,
   NVS_FR_TEXCOORD2	= 10,
   NVS_FR_TEXCOORD3	= 11,
   NVS_FR_TEXCOORD4	= 12,
   NVS_FR_TEXCOORD5	= 13,
   NVS_FR_TEXCOORD6	= 14,
   NVS_FR_TEXCOORD7	= 15,
   NVS_FR_BFC0		= 16,
   NVS_FR_BFC1		= 17,
   NVS_FR_POINTSZ	= 18,
   NVS_FR_FRAGDATA0	= 19,
   NVS_FR_FRAGDATA1	= 20,
   NVS_FR_FRAGDATA2	= 21,
   NVS_FR_FRAGDATA3	= 22,
   NVS_FR_CLIP0		= 23,
   NVS_FR_CLIP1		= 24,
   NVS_FR_CLIP2		= 25,
   NVS_FR_CLIP3		= 26,
   NVS_FR_CLIP4		= 27,
   NVS_FR_CLIP5		= 28,
   NVS_FR_CLIP6		= 29,
   NVS_FR_FACING	= 30,
   NVS_FR_UNKNOWN
} nvsFixedReg;

typedef enum {
   NVS_COND_FL, NVS_COND_LT, NVS_COND_EQ, NVS_COND_LE, NVS_COND_GT,
   NVS_COND_NE, NVS_COND_GE, NVS_COND_TR, NVS_COND_UN,
   NVS_COND_UNKNOWN
} nvsCond;

typedef struct {
   nvsRegFile	file;
   unsigned int		index;

   unsigned int		indexed;
   unsigned int		addr_reg;
   nvsSwzComp		addr_comp;

   nvsSwzComp		swizzle[4];
   int			negate;
   int			abs;
} nvsRegister;

static const nvsRegister nvr_unused = {
   .file	= NVS_FILE_ATTRIB,
   .index	= 0,
   .indexed	= 0,
   .addr_reg	= 0,
   .addr_comp	= NVS_SWZ_X,
   .swizzle	= {NVS_SWZ_X, NVS_SWZ_Y, NVS_SWZ_Z, NVS_SWZ_W},
   .negate	= 0,
   .abs		= 0,
};

typedef enum {
   NVS_TEX_TARGET_1D,
   NVS_TEX_TARGET_2D,
   NVS_TEX_TARGET_3D,
   NVS_TEX_TARGET_CUBE,
   NVS_TEX_TARGET_RECT,
   NVS_TEX_TARGET_UNKNOWN = 0
} nvsTexTarget;

typedef enum {
	NVS_SCALE_1X	 = 0,
	NVS_SCALE_2X	 = 1,
	NVS_SCALE_4X	 = 2,
	NVS_SCALE_8X	 = 3,
	NVS_SCALE_INV_2X = 5,
	NVS_SCALE_INV_4X = 6,
	NVS_SCALE_INV_8X = 7,
} nvsScale;

/* Arith/TEX instructions */
typedef struct nvs_instruction {
   nvsFragmentHeader header;

   nvsOpcode	op;
   unsigned int saturate;

   nvsRegister	dest;
   unsigned int	mask;
   nvsScale	dest_scale;

   nvsRegister	src[3];

   unsigned int tex_unit;
   nvsTexTarget tex_target;

   nvsCond	cond;
   nvsSwzComp	cond_swizzle[4];
   int		cond_reg;
   int		cond_test;
   int		cond_update;
} nvsInstruction;

/* BRA, CAL, IF */
typedef struct nvs_branch {
	nvsFragmentHeader  header;

	nvsOpcode	op;

	nvsCond		cond;
	nvsSwzComp	cond_swizzle[4];
	int		cond_test;

	nvsFragmentHeader *target_head;
	nvsFragmentHeader *target_tail;
	nvsFragmentHeader *else_head;
	nvsFragmentHeader *else_tail;
} nvsBranch;

/* LOOP+ENDLOOP */
typedef struct {
	nvsFragmentHeader  header;

	int                count;
	int                initial;
	int                increment;

	nvsFragmentHeader *insn_head;
	nvsFragmentHeader *insn_tail;
} nvsLoop;

/* label+following instructions */
typedef struct nvs_subroutine {
	nvsFragmentHeader  header;

	char *             label;
	nvsFragmentHeader *insn_head;
	nvsFragmentHeader *insn_tail;
} nvsSubroutine;

#define SMASK_X (1<<0)
#define SMASK_Y (1<<1)
#define SMASK_Z (1<<2)
#define SMASK_W (1<<3)
#define SMASK_ALL (SMASK_X|SMASK_Y|SMASK_Z|SMASK_W)

#define SPOS_ADDRESS 3
struct _op_xlat {
	unsigned int	NV;
	nvsOpcode	SOP;
	int		srcpos[3];
};
#define MOD_OPCODE(t,hw,sop,s0,s1,s2) do { \
	t[hw].NV = hw; \
	t[hw].SOP = sop; \
	t[hw].srcpos[0] = s0; \
	t[hw].srcpos[1] = s1; \
	t[hw].srcpos[2] = s2; \
} while(0)

extern unsigned int NVVP_TX_VOP_COUNT;
extern unsigned int NVVP_TX_NVS_OP_COUNT;
extern struct _op_xlat NVVP_TX_VOP[];
extern struct _op_xlat NVVP_TX_SOP[];

extern unsigned int NVFP_TX_AOP_COUNT;
extern unsigned int NVFP_TX_BOP_COUNT;
extern struct _op_xlat NVFP_TX_AOP[];
extern struct _op_xlat NVFP_TX_BOP[];

extern void NV20VPTXSwizzle(int hwswz, nvsSwzComp *swz);
extern nvsSwzComp NV20VP_TX_SWIZZLE[4];

#define SCAP_SRC_ABS	(1<<0)

struct _nvsFunc {
   nvsCardPriv *card_priv;

   unsigned int	MaxInst;
   unsigned int	MaxAttrib;
   unsigned int	MaxTemp;
   unsigned int	MaxAddress;
   unsigned int	MaxConst;
   unsigned int	caps;

   unsigned int *inst;
   void		(*UploadToHW)		(GLcontext *, nouveauShader *);
   void		(*UpdateConst)		(GLcontext *, nouveauShader *, int);

   struct _op_xlat*(*GetOPTXRec)	(nvsFunc *, int merged);
   struct _op_xlat*(*GetOPTXFromSOP)	(nvsOpcode, int *id);

   void		(*InitInstruction)	(nvsFunc *);
   int		(*SupportsOpcode)	(nvsFunc *, nvsOpcode);
   int		(*SupportsResultScale)	(nvsFunc *, nvsScale);
   void		(*SetOpcode)		(nvsFunc *, unsigned int opcode,
	 				 int slot);
   void		(*SetCCUpdate)		(nvsFunc *);
   void		(*SetCondition)		(nvsFunc *, int on, nvsCond, int reg,
	 				 nvsSwzComp *swizzle);
   void		(*SetResult)		(nvsFunc *, nvsRegister *,
	 				 unsigned int mask, int slot);
   void		(*SetResultScale)	(nvsFunc *, nvsScale);
   void		(*SetSource)		(nvsFunc *, nvsRegister *, int pos);
   void		(*SetTexImageUnit)	(nvsFunc *, int unit);
   void		(*SetSaturate)		(nvsFunc *);
   void		(*SetLastInst)		(nvsFunc *);

   void		(*SetBranchTarget)	(nvsFunc *, int addr);
   void		(*SetBranchElse)	(nvsFunc *, int addr);
   void		(*SetBranchEnd)		(nvsFunc *, int addr);
   void		(*SetLoopParams)	(nvsFunc *, int cnt, int init, int inc);

   int		(*HasMergedInst)	(nvsFunc *);
   int		(*IsLastInst)		(nvsFunc *);
   int		(*GetOffsetNext)	(nvsFunc *);

   int		(*GetOpcodeSlot)	(nvsFunc *, int merged);
   unsigned int	(*GetOpcodeHW)		(nvsFunc *, int slot);
   nvsOpcode	(*GetOpcode)		(nvsFunc *, int merged);

   nvsPrecision	(*GetPrecision)		(nvsFunc *);
   int		(*GetSaturate)		(nvsFunc *);

   nvsRegFile	(*GetDestFile)		(nvsFunc *, int merged);
   unsigned int	(*GetDestID)		(nvsFunc *, int merged);
   unsigned int	(*GetDestMask)		(nvsFunc *, int merged);

   unsigned int	(*GetSourceHW)		(nvsFunc *, int merged, int pos);
   nvsRegFile	(*GetSourceFile)	(nvsFunc *, int merged, int pos);
   int		(*GetSourceID)		(nvsFunc *, int merged, int pos);
   int		(*GetTexImageUnit)	(nvsFunc *);
   int		(*GetSourceNegate)	(nvsFunc *, int merged, int pos);
   int		(*GetSourceAbs)		(nvsFunc *, int merged, int pos);
   void		(*GetSourceSwizzle)	(nvsFunc *, int merged, int pos,
	 				 nvsSwzComp *swz);
   int		(*GetSourceIndexed)	(nvsFunc *, int merged, int pos);
   void		(*GetSourceConstVal)	(nvsFunc *, int merged, int pos,
	 				 float *val);
   int		(*GetSourceScale)	(nvsFunc *, int merged, int pos);

   int		(*GetRelAddressRegID)	(nvsFunc *);
   nvsSwzComp	(*GetRelAddressSwizzle)	(nvsFunc *);

   int		(*SupportsConditional)	(nvsFunc *);
   int		(*GetConditionUpdate)	(nvsFunc *);
   int		(*GetConditionTest)	(nvsFunc *);
   nvsCond	(*GetCondition)		(nvsFunc *);
   void		(*GetCondRegSwizzle)	(nvsFunc *, nvsSwzComp *swz);
   int		(*GetCondRegID)		(nvsFunc *);
   int		(*GetBranch)		(nvsFunc *);
   int		(*GetBranchElse)	(nvsFunc *);
   int		(*GetBranchEnd)		(nvsFunc *);

   int		(*GetLoopCount)		(nvsFunc *);
   int		(*GetLoopInitial)	(nvsFunc *);
   int		(*GetLoopIncrement)	(nvsFunc *);
};

static inline nvsRegister
nvsNegate(nvsRegister reg)
{
   reg.negate = !reg.negate;
   return reg;
}

static inline nvsRegister
nvsAbs(nvsRegister reg)
{
   reg.abs = 1;
   return reg;
}

static inline nvsRegister
nvsSwizzle(nvsRegister reg, nvsSwzComp x, nvsSwzComp y,
      	   nvsSwzComp z, nvsSwzComp w)
{
   nvsSwzComp sc[4] = { x, y, z, w };
   nvsSwzComp oc[4];
   int i;

   for (i=0;i<4;i++)
      oc[i] = reg.swizzle[i];
   for (i=0;i<4;i++)
      reg.swizzle[i] = oc[sc[i]];
   return reg;
}

#define nvsProgramError(nvs,fmt,args...) do {                           \
	fprintf(stderr, "nvsProgramError (%s): "fmt, __func__, ##args); \
	(nvs)->error = GL_TRUE;                                         \
	(nvs)->translated = GL_FALSE;                                   \
} while(0)

extern GLboolean nvsUpdateShader(GLcontext *ctx, nouveauShader *nvs);
extern void nvsDisasmHWShader(nvsPtr);
extern void nvsDumpFragmentList(nvsFragmentHeader *f, int lvl);
extern nouveauShader *nvsBuildTextShader(GLcontext *ctx, GLenum target,
      					 const char *text);

extern void NV20VPInitShaderFuncs(nvsFunc *);
extern void NV30VPInitShaderFuncs(nvsFunc *);
extern void NV40VPInitShaderFuncs(nvsFunc *);

extern void NV30FPInitShaderFuncs(nvsFunc *);
extern void NV40FPInitShaderFuncs(nvsFunc *);

extern void nouveauShaderInitFuncs(GLcontext *ctx);

extern GLboolean nouveau_shader_pass0(GLcontext *ctx, nouveauShader *nvs);
extern GLboolean nouveau_shader_pass1(nvsPtr nvs);
extern GLboolean nouveau_shader_pass2(nvsPtr nvs);

#endif

