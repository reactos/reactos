#include <stdint.h>

#include "glheader.h"
#include "macros.h"

#include "nouveau_context.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"
#include "nouveau_drm.h"
#include "nouveau_shader.h"
#include "nouveau_object.h"
#include "nouveau_msg.h"
#include "nouveau_bufferobj.h"
#include "nv30_shader.h"

unsigned int NVFP_TX_AOP_COUNT = 64;
struct _op_xlat NVFP_TX_AOP[64];

/*******************************************************************************
 * Support routines
 */

static void
NV30FPUploadToHW(GLcontext *ctx, nouveauShader *nvs)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   nvsCardPriv *priv = &nvs->card_priv;
   uint32_t offset;

   if (!nvs->program_buffer)
      nvs->program_buffer = ctx->Driver.NewBufferObject(ctx, 0,
							GL_ARRAY_BUFFER_ARB);

   /* Should use STATIC_DRAW_ARB if shader doesn't use changable params */
   nouveau_bo_init_storage(ctx, NOUVEAU_BO_VRAM_OK,
	 		  nvs->program_size * sizeof(uint32_t),
			  (const GLvoid *)nvs->program,
			  GL_DYNAMIC_DRAW_ARB,
			  nvs->program_buffer);

   offset = nouveau_bo_gpu_ref(ctx, nvs->program_buffer);

   /* Not using state cache here, updated programs at the same address don't
    * seem to take effect unless the ACTIVE_PROGRAM method is called again.
    * HW caches the program somewhere?
    */
   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_FP_ACTIVE_PROGRAM, 1);
   OUT_RING       (offset | 1);
   if (nmesa->screen->card->type == NV_30) {  
	   BEGIN_RING_SIZE(NvSub3D,
			   0x1d60 /*NV30_TCL_PRIMITIVE_3D_FP_CONTROL*/, 1);
	   OUT_RING       ((priv->NV30FP.uses_kil << 7));
	   BEGIN_RING_SIZE(NvSub3D, 0x1450, 1);
	   OUT_RING       (priv->NV30FP.num_regs << 16);
   } else {
	   BEGIN_RING_SIZE(NvSub3D,
			   0x1d60 /*NV30_TCL_PRIMITIVE_3D_FP_CONTROL*/, 1);
	   OUT_RING       ((priv->NV30FP.uses_kil <<  7) |
			   (priv->NV30FP.num_regs << 24));
   }
}

static void
NV30FPUpdateConst(GLcontext *ctx, nouveauShader *nvs, int id)
{
   uint32_t *new     = nvs->params[id].source_val ?
      (uint32_t*)nvs->params[id].source_val : (uint32_t*)nvs->params[id].val;
   uint32_t *current;
   int i;

   for (i=0; i<nvs->params[id].hw_index_cnt; i++) {
      current = nvs->program + nvs->params[id].hw_index[i];
      COPY_4V(current, new);
   }
   nvs->on_hardware = 0;
}

/*******************************************************************************
 * Assembly helpers
 */
static struct _op_xlat *
NV30FPGetOPTXFromSOP(nvsOpcode op, int *id)
{
   int i;

   for (i=0; i<NVFP_TX_AOP_COUNT; i++) {
      if (NVFP_TX_AOP[i].SOP == op) {
	 if (id) *id = 0;
	 return &NVFP_TX_AOP[i];
      }
   }

   return NULL;
}

static int
NV30FPSupportsOpcode(nvsFunc *shader, nvsOpcode op)
{
   if (shader->GetOPTXFromSOP(op, NULL))
      return 1;
   return 0;
}

static void
NV30FPSetOpcode(nvsFunc *shader, unsigned int opcode, int slot)
{
   if (opcode == NV30_FP_OP_OPCODE_KIL)
      shader->card_priv->NV30FP.uses_kil = GL_TRUE;
   shader->inst[0] &= ~NV30_FP_OP_OPCODE_MASK;
   shader->inst[0] |= (opcode << NV30_FP_OP_OPCODE_SHIFT);
}

static void
NV30FPSetCCUpdate(nvsFunc *shader)
{
   shader->inst[0] |= NV30_FP_OP_COND_WRITE_ENABLE;
}

static void
NV30FPSetCondition(nvsFunc *shader, int on, nvsCond cond, int reg,
      		  nvsSwzComp *swz)
{
   nvsSwzComp default_swz[4] = { NVS_SWZ_X, NVS_SWZ_Y, NVS_SWZ_Z, NVS_SWZ_W };
   unsigned int hwcond;

   /* cond masking is always enabled */
   if (!on) {
      cond = NVS_COND_TR;
      reg  = 0;
      swz  = default_swz;
   }

   switch (cond) {
   case NVS_COND_TR: hwcond = NV30_FP_OP_COND_TR; break;
   case NVS_COND_FL: hwcond = NV30_FP_OP_COND_FL; break;
   case NVS_COND_LT: hwcond = NV30_FP_OP_COND_LT; break;
   case NVS_COND_GT: hwcond = NV30_FP_OP_COND_GT; break;
   case NVS_COND_LE: hwcond = NV30_FP_OP_COND_LE; break;
   case NVS_COND_GE: hwcond = NV30_FP_OP_COND_GE; break;
   case NVS_COND_EQ: hwcond = NV30_FP_OP_COND_EQ; break;
   case NVS_COND_NE: hwcond = NV30_FP_OP_COND_NE; break;
   default:
	WARN_ONCE("unknown fp condmask=%d\n", cond);
	hwcond = NV30_FP_OP_COND_TR;
	break;
   }

   shader->inst[1] &= ~NV30_FP_OP_COND_MASK;
   shader->inst[1] |= (hwcond << NV30_FP_OP_COND_SHIFT);

   shader->inst[1] &= ~NV30_FP_OP_COND_SWZ_ALL_MASK;
   shader->inst[1] |= (swz[NVS_SWZ_X] << NV30_FP_OP_COND_SWZ_X_SHIFT);
   shader->inst[1] |= (swz[NVS_SWZ_Y] << NV30_FP_OP_COND_SWZ_Y_SHIFT);
   shader->inst[1] |= (swz[NVS_SWZ_Z] << NV30_FP_OP_COND_SWZ_Z_SHIFT);
   shader->inst[1] |= (swz[NVS_SWZ_W] << NV30_FP_OP_COND_SWZ_W_SHIFT);
}

static void
NV30FPSetHighReg(nvsFunc *shader, int id)
{
   if (shader->card_priv->NV30FP.num_regs < (id+1)) {
      if (id == 0)
	 id = 1; /* necessary? */
      shader->card_priv->NV30FP.num_regs = (id+1);
   }
}

static void
NV30FPSetResult(nvsFunc *shader, nvsRegister *reg, unsigned int mask, int slot)
{
   unsigned int hwreg;

   if (mask & SMASK_X) shader->inst[0] |= NV30_FP_OP_OUT_X;
   if (mask & SMASK_Y) shader->inst[0] |= NV30_FP_OP_OUT_Y;
   if (mask & SMASK_Z) shader->inst[0] |= NV30_FP_OP_OUT_Z;
   if (mask & SMASK_W) shader->inst[0] |= NV30_FP_OP_OUT_W;

   if (reg->file == NVS_FILE_RESULT) {
      hwreg = 0; /* FIXME: this is only fragment.color */
      /* This is *not* correct, I have no idea what it is either */
      shader->inst[0] |= NV30_FP_OP_UNK0_7;
   } else {
      shader->inst[0] &= ~NV30_FP_OP_UNK0_7;
      hwreg = reg->index;
   }
   NV30FPSetHighReg(shader, hwreg);
   shader->inst[0] &= ~NV30_FP_OP_OUT_REG_SHIFT;
   shader->inst[0] |= (hwreg  << NV30_FP_OP_OUT_REG_SHIFT);
}

static void
NV30FPSetSource(nvsFunc *shader, nvsRegister *reg, int pos)
{
   unsigned int hwsrc = 0;

   switch (reg->file) {
   case NVS_FILE_TEMP:
      hwsrc |= (NV30_FP_REG_TYPE_TEMP << NV30_FP_REG_TYPE_SHIFT);
      hwsrc |= (reg->index << NV30_FP_REG_SRC_SHIFT);
      NV30FPSetHighReg(shader, reg->index);
      break;
   case NVS_FILE_ATTRIB:
      {
	 unsigned int hwin;

	 switch (reg->index) {
	 case NVS_FR_POSITION : hwin = NV30_FP_OP_INPUT_SRC_POSITION; break;
	 case NVS_FR_COL0     : hwin = NV30_FP_OP_INPUT_SRC_COL0; break;
	 case NVS_FR_COL1     : hwin = NV30_FP_OP_INPUT_SRC_COL1; break;
	 case NVS_FR_FOGCOORD : hwin = NV30_FP_OP_INPUT_SRC_FOGC; break;
	 case NVS_FR_TEXCOORD0: hwin = NV30_FP_OP_INPUT_SRC_TC(0); break;
	 case NVS_FR_TEXCOORD1: hwin = NV30_FP_OP_INPUT_SRC_TC(1); break;
	 case NVS_FR_TEXCOORD2: hwin = NV30_FP_OP_INPUT_SRC_TC(2); break;
	 case NVS_FR_TEXCOORD3: hwin = NV30_FP_OP_INPUT_SRC_TC(3); break;
	 case NVS_FR_TEXCOORD4: hwin = NV30_FP_OP_INPUT_SRC_TC(4); break;
	 case NVS_FR_TEXCOORD5: hwin = NV30_FP_OP_INPUT_SRC_TC(5); break;
	 case NVS_FR_TEXCOORD6: hwin = NV30_FP_OP_INPUT_SRC_TC(6); break;
	 case NVS_FR_TEXCOORD7: hwin = NV30_FP_OP_INPUT_SRC_TC(7); break;
	 default:
		WARN_ONCE("unknown fp input %d\n", reg->index);
		hwin = NV30_FP_OP_INPUT_SRC_COL0;
		break;
	 }
	 shader->inst[0] &= ~NV30_FP_OP_INPUT_SRC_MASK;
	 shader->inst[0] |= (hwin << NV30_FP_OP_INPUT_SRC_SHIFT);
	 hwsrc |= (hwin << NV30_FP_REG_SRC_SHIFT);
      }
      hwsrc |= (NV30_FP_REG_TYPE_INPUT << NV30_FP_REG_TYPE_SHIFT);
      break;
   case NVS_FILE_CONST:
      /* consts are inlined after the inst */
      hwsrc |= (NV30_FP_REG_TYPE_CONST << NV30_FP_REG_TYPE_SHIFT);
      break;
   default:
      assert(0);
      break;
   }

   if (reg->negate)
      hwsrc |= NV30_FP_REG_NEGATE;
   if (reg->abs)
      shader->inst[1] |= (1 << (29+pos));
   hwsrc |= (reg->swizzle[NVS_SWZ_X] << NV30_FP_REG_SWZ_X_SHIFT);
   hwsrc |= (reg->swizzle[NVS_SWZ_Y] << NV30_FP_REG_SWZ_Y_SHIFT);
   hwsrc |= (reg->swizzle[NVS_SWZ_Z] << NV30_FP_REG_SWZ_Z_SHIFT);
   hwsrc |= (reg->swizzle[NVS_SWZ_W] << NV30_FP_REG_SWZ_W_SHIFT);

   shader->inst[pos+1] &= ~NV30_FP_REG_ALL_MASK;
   shader->inst[pos+1] |= hwsrc;
}

static void
NV30FPSetTexImageUnit(nvsFunc *shader, int unit)
{
   shader->inst[0] &= ~NV30_FP_OP_TEX_UNIT_SHIFT;
   shader->inst[0] |= (unit << NV30_FP_OP_TEX_UNIT_SHIFT);
}

static void
NV30FPSetSaturate(nvsFunc *shader)
{
   shader->inst[0] |= NV30_FP_OP_OUT_SAT;
}

static void
NV30FPInitInstruction(nvsFunc *shader)
{
   unsigned int hwsrc;

   shader->inst[0] = 0;

   hwsrc = (NV30_FP_REG_TYPE_INPUT << NV30_FP_REG_TYPE_SHIFT) |
      	   (NVS_SWZ_X << NV30_FP_REG_SWZ_X_SHIFT) |
	   (NVS_SWZ_Y << NV30_FP_REG_SWZ_Y_SHIFT) |
	   (NVS_SWZ_Z << NV30_FP_REG_SWZ_Z_SHIFT) |
	   (NVS_SWZ_W << NV30_FP_REG_SWZ_W_SHIFT);
   shader->inst[1] = hwsrc;
   shader->inst[2] = hwsrc;
   shader->inst[3] = hwsrc;
}

static void
NV30FPSetLastInst(nvsFunc *shader)
{
   shader->inst[0] |= 1; 
}

/*******************************************************************************
 * Disassembly helpers
 */
static struct _op_xlat *
NV30FPGetOPTXRec(nvsFunc * shader, int merged)
{
   int op;

   op = shader->GetOpcodeHW(shader, 0);
   if (op > NVFP_TX_AOP_COUNT)
      return NULL;
   if (NVFP_TX_AOP[op].SOP == NVS_OP_UNKNOWN)
      return NULL;
   return &NVFP_TX_AOP[op];
}

static int
NV30FPHasMergedInst(nvsFunc * shader)
{
   return 0;
}

static int
NV30FPIsLastInst(nvsFunc * shader)
{
   return ((shader->inst[0] & NV30_FP_OP_PROGRAM_END) ? 1 : 0);
}

static int
NV30FPGetOffsetNext(nvsFunc * shader)
{
   int i;

   for (i = 0; i < 3; i++)
      if (shader->GetSourceFile(shader, 0, i) == NVS_FILE_CONST)
	 return 8;
   return 4;
}

static nvsOpcode
NV30FPGetOpcode(nvsFunc * shader, int merged)
{
   struct _op_xlat *opr;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr)
      return NVS_OP_UNKNOWN;

   return opr->SOP;
}

static unsigned int
NV30FPGetOpcodeHW(nvsFunc * shader, int slot)
{
   int op;

   op = (shader->inst[0] & NV30_FP_OP_OPCODE_MASK) >> NV30_FP_OP_OPCODE_SHIFT;

   return op;
}

static nvsRegFile
NV30FPGetDestFile(nvsFunc * shader, int merged)
{
   /* Result regs overlap temporary regs */
   return NVS_FILE_TEMP;
}

static unsigned int
NV30FPGetDestID(nvsFunc * shader, int merged)
{
   int id;

   switch (shader->GetDestFile(shader, merged)) {
   case NVS_FILE_TEMP:
      id = ((shader->inst[0] & NV30_FP_OP_OUT_REG_MASK)
	    >> NV30_FP_OP_OUT_REG_SHIFT);
      return id;
   default:
      return -1;
   }
}

static unsigned int
NV30FPGetDestMask(nvsFunc * shader, int merged)
{
   unsigned int mask = 0;

   if (shader->inst[0] & NV30_FP_OP_OUT_X) mask |= SMASK_X;
   if (shader->inst[0] & NV30_FP_OP_OUT_Y) mask |= SMASK_Y;
   if (shader->inst[0] & NV30_FP_OP_OUT_Z) mask |= SMASK_Z;
   if (shader->inst[0] & NV30_FP_OP_OUT_W) mask |= SMASK_W;

   return mask;
}

static unsigned int
NV30FPGetSourceHW(nvsFunc * shader, int merged, int pos)
{
   struct _op_xlat *opr;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1)
      return -1;

   return shader->inst[opr->srcpos[pos] + 1];
}

static nvsRegFile
NV30FPGetSourceFile(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;
   struct _op_xlat *opr;
   int file;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1)
      return NVS_FILE_UNKNOWN;

   switch (opr->srcpos[pos]) {
   case SPOS_ADDRESS: return NVS_FILE_ADDRESS;
   default:
      src = shader->GetSourceHW(shader, merged, pos);
      file = (src & NV30_FP_REG_TYPE_MASK) >> NV30_FP_REG_TYPE_SHIFT;

      switch (file) {
      case NV30_FP_REG_TYPE_TEMP : return NVS_FILE_TEMP;
      case NV30_FP_REG_TYPE_INPUT: return NVS_FILE_ATTRIB;
      case NV30_FP_REG_TYPE_CONST: return NVS_FILE_CONST;
      default:
	 return NVS_FILE_UNKNOWN;
      }
   }
}

static int
NV30FPGetSourceID(nvsFunc * shader, int merged, int pos)
{
   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_ATTRIB:
      switch ((shader->inst[0] & NV30_FP_OP_INPUT_SRC_MASK)
	      >> NV30_FP_OP_INPUT_SRC_SHIFT) {
      case NV30_FP_OP_INPUT_SRC_POSITION: return NVS_FR_POSITION;
      case NV30_FP_OP_INPUT_SRC_COL0    : return NVS_FR_COL0;
      case NV30_FP_OP_INPUT_SRC_COL1    : return NVS_FR_COL1;
      case NV30_FP_OP_INPUT_SRC_FOGC    : return NVS_FR_FOGCOORD;
      case NV30_FP_OP_INPUT_SRC_TC(0)   : return NVS_FR_TEXCOORD0;
      case NV30_FP_OP_INPUT_SRC_TC(1)   : return NVS_FR_TEXCOORD1;
      case NV30_FP_OP_INPUT_SRC_TC(2)   : return NVS_FR_TEXCOORD2;
      case NV30_FP_OP_INPUT_SRC_TC(3)   : return NVS_FR_TEXCOORD3;
      case NV30_FP_OP_INPUT_SRC_TC(4)   : return NVS_FR_TEXCOORD4;
      case NV30_FP_OP_INPUT_SRC_TC(5)   : return NVS_FR_TEXCOORD5;
      case NV30_FP_OP_INPUT_SRC_TC(6)   : return NVS_FR_TEXCOORD6;
      case NV30_FP_OP_INPUT_SRC_TC(7)   : return NVS_FR_TEXCOORD7;
      default:
	 return -1;
      }
      break;
   case NVS_FILE_TEMP:
      {
	 unsigned int src;

	 src = shader->GetSourceHW(shader, merged, pos);
	 return ((src & NV30_FP_REG_SRC_MASK) >> NV30_FP_REG_SRC_SHIFT);
      }
   case NVS_FILE_CONST:		/* inlined into fragprog */
   default:
      return -1;
   }
}

static int
NV30FPGetTexImageUnit(nvsFunc *shader)
{
      return ((shader->inst[0] & NV30_FP_OP_TEX_UNIT_MASK)
	      >> NV30_FP_OP_TEX_UNIT_SHIFT);
}

static int
NV30FPGetSourceNegate(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;

   src = shader->GetSourceHW(shader, merged, pos);

   if (src == -1)
      return -1;
   return ((src & NV30_FP_REG_NEGATE) ? 1 : 0);
}

static int
NV30FPGetSourceAbs(nvsFunc * shader, int merged, int pos)
{
   struct _op_xlat *opr;
   static unsigned int abspos[3] = {
      NV30_FP_OP_OUT_ABS,
      (1 << 30),		/* guess */
      (1 << 31)			/* guess */
   };

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1)
      return -1;

   return ((shader->inst[1] & abspos[opr->srcpos[pos]]) ? 1 : 0);
}

nvsSwzComp NV30FP_TX_SWIZZLE[4] = {NVS_SWZ_X, NVS_SWZ_Y, NVS_SWZ_Z, NVS_SWZ_W };

static void
NV30FPTXSwizzle(int hwswz, nvsSwzComp *swz)
{
   swz[NVS_SWZ_W] = NV30FP_TX_SWIZZLE[(hwswz & 0xC0) >> 6];
   swz[NVS_SWZ_Z] = NV30FP_TX_SWIZZLE[(hwswz & 0x30) >> 4];
   swz[NVS_SWZ_Y] = NV30FP_TX_SWIZZLE[(hwswz & 0x0C) >> 2];
   swz[NVS_SWZ_X] = NV30FP_TX_SWIZZLE[(hwswz & 0x03) >> 0];
}

static void
NV30FPGetSourceSwizzle(nvsFunc * shader, int merged, int pos, nvsSwzComp *swz)
{
   unsigned int src;
   int swzbits;

   src = shader->GetSourceHW(shader, merged, pos);
   swzbits = (src & NV30_FP_REG_SWZ_ALL_MASK) >> NV30_FP_REG_SWZ_ALL_SHIFT;
   NV30FPTXSwizzle(swzbits, swz);
}

static int
NV30FPGetSourceIndexed(nvsFunc * shader, int merged, int pos)
{
   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_ATTRIB:
      return ((shader->inst[3] & NV30_FP_OP_INDEX_INPUT) ? 1 : 0);
   default:
      return 0;
   }
}

static void
NV30FPGetSourceConstVal(nvsFunc * shader, int merged, int pos, float *val)
{
   val[0] = *(float *) &(shader->inst[4]);
   val[1] = *(float *) &(shader->inst[5]);
   val[2] = *(float *) &(shader->inst[6]);
   val[3] = *(float *) &(shader->inst[7]);
}

static int
NV30FPGetSourceScale(nvsFunc * shader, int merged, int pos)
{
/*FIXME: is this per-source, only for a specific source, or all sources??*/
   return (1 << ((shader->inst[2] & NV30_FP_OP_SRC_SCALE_MASK)
		 >> NV30_FP_OP_SRC_SCALE_SHIFT));
}

static int
NV30FPGetAddressRegID(nvsFunc * shader)
{
   return 0;
}

static nvsSwzComp
NV30FPGetAddressRegSwizzle(nvsFunc * shader)
{
   return NVS_SWZ_X;
}

static int
NV30FPSupportsConditional(nvsFunc * shader)
{
   /*FIXME: Is this true of all ops? */
   return 1;
}

static int
NV30FPGetConditionUpdate(nvsFunc * shader)
{
   return ((shader->inst[0] & NV30_FP_OP_COND_WRITE_ENABLE) ? 1 : 0);
}

static int
NV30FPGetConditionTest(nvsFunc * shader)
{
   /*FIXME: always? */
   return 1;
}

static nvsCond
NV30FPGetCondition(nvsFunc * shader)
{
   int cond;

   cond = ((shader->inst[1] & NV30_FP_OP_COND_MASK)
	   >> NV30_FP_OP_COND_SHIFT);

   switch (cond) {
   case NV30_FP_OP_COND_FL: return NVS_COND_FL;
   case NV30_FP_OP_COND_LT: return NVS_COND_LT;
   case NV30_FP_OP_COND_EQ: return NVS_COND_EQ;
   case NV30_FP_OP_COND_LE: return NVS_COND_LE;
   case NV30_FP_OP_COND_GT: return NVS_COND_GT;
   case NV30_FP_OP_COND_NE: return NVS_COND_NE;
   case NV30_FP_OP_COND_GE: return NVS_COND_GE;
   case NV30_FP_OP_COND_TR: return NVS_COND_TR;
   default:
      return NVS_COND_UNKNOWN;
   }
}

static void
NV30FPGetCondRegSwizzle(nvsFunc * shader, nvsSwzComp *swz)
{
   int swzbits;

   swzbits = (shader->inst[1] & NV30_FP_OP_COND_SWZ_ALL_MASK)
      >> NV30_FP_OP_COND_SWZ_ALL_SHIFT;
   NV30FPTXSwizzle(swzbits, swz);
}

static int
NV30FPGetCondRegID(nvsFunc * shader)
{
   return 0;
}

static nvsPrecision
NV30FPGetPrecision(nvsFunc * shader)
{
   int p;

   p = (shader->inst[0] & NV30_FP_OP_PRECISION_MASK)
      >> NV30_FP_OP_PRECISION_SHIFT;

   switch (p) {
   case NV30_FP_PRECISION_FP32: return NVS_PREC_FLOAT32;
   case NV30_FP_PRECISION_FP16: return NVS_PREC_FLOAT16;
   case NV30_FP_PRECISION_FX12: return NVS_PREC_FIXED12;
   default:
      return NVS_PREC_UNKNOWN;
   }
}

static int
NV30FPGetSaturate(nvsFunc * shader)
{
   return ((shader->inst[0] & NV30_FP_OP_OUT_SAT) ? 1 : 0);
}

/*******************************************************************************
 * Init
 */
void
NV30FPInitShaderFuncs(nvsFunc * shader)
{
   /* These are probably bogus, I made them up... */
   shader->MaxInst	= 1024;
   shader->MaxAttrib	= 16;
   shader->MaxTemp	= 32;
   shader->MaxAddress	= 1;
   shader->MaxConst	= 256;
   shader->caps		= SCAP_SRC_ABS;

   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_MOV, NVS_OP_MOV, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_MUL, NVS_OP_MUL, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_ADD, NVS_OP_ADD, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_MAD, NVS_OP_MAD, 0, 1, 2);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_DP3, NVS_OP_DP3, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_DP4, NVS_OP_DP4, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_DST, NVS_OP_DST, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_MIN, NVS_OP_MIN, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_MAX, NVS_OP_MAX, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SLT, NVS_OP_SLT, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SGE, NVS_OP_SGE, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_FRC, NVS_OP_FRC, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_FLR, NVS_OP_FLR, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_TEX, NVS_OP_TEX, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_TXD, NVS_OP_TXD, 0, 1, 2);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_TXP, NVS_OP_TXP, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_TXB, NVS_OP_TXB, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SEQ, NVS_OP_SEQ, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SGT, NVS_OP_SGT, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SLE, NVS_OP_SLE, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SNE, NVS_OP_SNE, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_RCP, NVS_OP_RCP, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_LG2, NVS_OP_LG2, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_EX2, NVS_OP_EX2, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_COS, NVS_OP_COS, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_SIN, NVS_OP_SIN, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_NOP, NVS_OP_NOP, -1, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_DDX, NVS_OP_DDX, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_DDY, NVS_OP_DDY, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_KIL, NVS_OP_KIL, -1, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_PK4B, NVS_OP_PK4B, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_UP4B, NVS_OP_UP4B, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_PK2H, NVS_OP_PK2H, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_UP2H, NVS_OP_UP2H, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_PK4UB, NVS_OP_PK4UB, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_UP4UB, NVS_OP_UP4UB, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_PK2US, NVS_OP_PK2US, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_UP2US, NVS_OP_UP2US, 0, -1, -1);
   /*FIXME: Haven't confirmed the source positions for the below opcodes */
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_LIT, NVS_OP_LIT, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_LRP, NVS_OP_LRP, 0, 1, 2);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_POW, NVS_OP_POW, 0, 1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_RSQ, NVS_OP_RSQ, 0, -1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV30_FP_OP_OPCODE_RFL, NVS_OP_RFL, 0, 1, -1);

   shader->GetOPTXRec		= NV30FPGetOPTXRec;
   shader->GetOPTXFromSOP	= NV30FPGetOPTXFromSOP;

   shader->UploadToHW		= NV30FPUploadToHW;
   shader->UpdateConst		= NV30FPUpdateConst;

   shader->InitInstruction	= NV30FPInitInstruction;
   shader->SupportsOpcode	= NV30FPSupportsOpcode;
   shader->SetOpcode		= NV30FPSetOpcode;
   shader->SetCCUpdate		= NV30FPSetCCUpdate;
   shader->SetCondition		= NV30FPSetCondition;
   shader->SetResult		= NV30FPSetResult;
   shader->SetSource		= NV30FPSetSource;
   shader->SetTexImageUnit	= NV30FPSetTexImageUnit;
   shader->SetSaturate		= NV30FPSetSaturate;
   shader->SetLastInst		= NV30FPSetLastInst;

   shader->HasMergedInst	= NV30FPHasMergedInst;
   shader->IsLastInst		= NV30FPIsLastInst;
   shader->GetOffsetNext	= NV30FPGetOffsetNext;
   shader->GetOpcode		= NV30FPGetOpcode;
   shader->GetOpcodeHW		= NV30FPGetOpcodeHW;
   shader->GetDestFile		= NV30FPGetDestFile;
   shader->GetDestID		= NV30FPGetDestID;
   shader->GetDestMask		= NV30FPGetDestMask;
   shader->GetSourceHW		= NV30FPGetSourceHW;
   shader->GetSourceFile	= NV30FPGetSourceFile;
   shader->GetSourceID		= NV30FPGetSourceID;
   shader->GetTexImageUnit	= NV30FPGetTexImageUnit;
   shader->GetSourceNegate	= NV30FPGetSourceNegate;
   shader->GetSourceAbs		= NV30FPGetSourceAbs;
   shader->GetSourceSwizzle	= NV30FPGetSourceSwizzle;
   shader->GetSourceIndexed	= NV30FPGetSourceIndexed;
   shader->GetSourceConstVal	= NV30FPGetSourceConstVal;
   shader->GetSourceScale	= NV30FPGetSourceScale;
   shader->GetRelAddressRegID	= NV30FPGetAddressRegID;
   shader->GetRelAddressSwizzle = NV30FPGetAddressRegSwizzle;
   shader->GetPrecision		= NV30FPGetPrecision;
   shader->GetSaturate		= NV30FPGetSaturate;
   shader->SupportsConditional	= NV30FPSupportsConditional;
   shader->GetConditionUpdate	= NV30FPGetConditionUpdate;
   shader->GetConditionTest	= NV30FPGetConditionTest;
   shader->GetCondition		= NV30FPGetCondition;
   shader->GetCondRegSwizzle	= NV30FPGetCondRegSwizzle;
   shader->GetCondRegID		= NV30FPGetCondRegID;
}
