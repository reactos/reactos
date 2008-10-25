#include "nouveau_context.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"

#include "nouveau_shader.h"
#include "nv30_shader.h"

/*****************************************************************************
 * Support routines
 */
static void
NV30VPUploadToHW(GLcontext *ctx, nouveauShader *nvs)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   int i;

   /* We can do better here and keep more than one VP on the hardware, and
    * switch between them with PROGRAM_START_ID..
    */
   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_UPLOAD_FROM_ID, 1);
   OUT_RING(0);
   for (i=0; i<nvs->program_size; i+=4) {
      BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_UPLOAD_INST0, 4);
      OUT_RING(nvs->program[i + 0]);
      OUT_RING(nvs->program[i + 1]);
      OUT_RING(nvs->program[i + 2]);
      OUT_RING(nvs->program[i + 3]);
   }
   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_PROGRAM_START_ID, 1);
   OUT_RING(0);

   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_IN_REG, 2);
   OUT_RING(nvs->card_priv.NV30VP.vp_in_reg);
   OUT_RING(nvs->card_priv.NV30VP.vp_out_reg);

   BEGIN_RING_CACHE(NvSub3D, NV30_TCL_PRIMITIVE_3D_SET_CLIPPING_PLANES, 1);
   OUT_RING_CACHE  (nvs->card_priv.NV30VP.clip_enables);
}

static void
NV30VPUpdateConst(GLcontext *ctx, nouveauShader *nvs, int id)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   GLfloat *val;

   val = nvs->params[id].source_val ?
      nvs->params[id].source_val : nvs->params[id].val;

   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 5);
   OUT_RING (id);
   OUT_RINGp(val, 4);
}

/*****************************************************************************
 * Assembly routines
 */
static void
NV30VPSetBranchTarget(nvsFunc *shader, int addr)
{
	shader->inst[2] &= ~NV30_VP_INST_IADDR_MASK;
	shader->inst[2] |= (addr << NV30_VP_INST_IADDR_SHIFT);
}

/*****************************************************************************
 * Disassembly routines
 */
static unsigned int
NV30VPGetOpcodeHW(nvsFunc * shader, int slot)
{
   int op;

   if (slot) {
      op = (shader->inst[1] & NV30_VP_INST_SCA_OPCODEL_MASK)
	 >> NV30_VP_INST_SCA_OPCODEL_SHIFT;
      op |= ((shader->inst[0] & NV30_VP_INST_SCA_OPCODEH_MASK)
	     >> NV30_VP_INST_SCA_OPCODEH_SHIFT) << 4;
   }
   else {
      op = (shader->inst[1] & NV30_VP_INST_VEC_OPCODE_MASK)
	 >> NV30_VP_INST_VEC_OPCODE_SHIFT;
   }

   return op;
}

static nvsRegFile
NV30VPGetDestFile(nvsFunc * shader, int merged)
{
   switch (shader->GetOpcode(shader, merged)) {
   case NVS_OP_ARL:
   case NVS_OP_ARR:
   case NVS_OP_ARA:
      return NVS_FILE_ADDRESS;
   default:
      /*FIXME: This probably isn't correct.. */
      if ((shader->inst[3] & NV30_VP_INST_VDEST_WRITEMASK_MASK) != 0)
	 return NVS_FILE_RESULT;
      if ((shader->inst[3] & NV30_VP_INST_SDEST_WRITEMASK_MASK) != 0)
	 return NVS_FILE_RESULT;
      return NVS_FILE_TEMP;
   }
}

static unsigned int
NV30VPGetDestID(nvsFunc * shader, int merged)
{
   int id;

   switch (shader->GetDestFile(shader, merged)) {
   case NVS_FILE_RESULT:
      id = ((shader->inst[3] & NV30_VP_INST_DEST_ID_MASK)
	    >> NV30_VP_INST_DEST_ID_SHIFT);
      switch (id) {
      case NV30_VP_INST_DEST_POS  : return NVS_FR_POSITION;
      case NV30_VP_INST_DEST_COL0 : return NVS_FR_COL0;
      case NV30_VP_INST_DEST_COL1 : return NVS_FR_COL1;
      case NV30_VP_INST_DEST_TC(0): return NVS_FR_TEXCOORD0;
      case NV30_VP_INST_DEST_TC(1): return NVS_FR_TEXCOORD1;
      case NV30_VP_INST_DEST_TC(2): return NVS_FR_TEXCOORD2;
      case NV30_VP_INST_DEST_TC(3): return NVS_FR_TEXCOORD3;
      case NV30_VP_INST_DEST_TC(4): return NVS_FR_TEXCOORD4;
      case NV30_VP_INST_DEST_TC(5): return NVS_FR_TEXCOORD5;
      case NV30_VP_INST_DEST_TC(6): return NVS_FR_TEXCOORD6;
      case NV30_VP_INST_DEST_TC(7): return NVS_FR_TEXCOORD7;
      default:
	 return -1;
      }
   case NVS_FILE_ADDRESS:
   case NVS_FILE_TEMP:
      return (shader->inst[0] & NV30_VP_INST_DEST_TEMP_ID_MASK)
	 >> NV30_VP_INST_DEST_TEMP_ID_SHIFT;
   default:
      return -1;
   }
}

static unsigned int
NV30VPGetDestMask(nvsFunc * shader, int merged)
{
   int hwmask, mask = 0;

   if (shader->GetDestFile(shader, merged) == NVS_FILE_RESULT)
      if (shader->GetOpcodeSlot(shader, merged))
	 hwmask = (shader->inst[3] & NV30_VP_INST_SDEST_WRITEMASK_MASK)
	    >> NV30_VP_INST_SDEST_WRITEMASK_SHIFT;
      else
	 hwmask = (shader->inst[3] & NV30_VP_INST_VDEST_WRITEMASK_MASK)
	    >> NV30_VP_INST_VDEST_WRITEMASK_SHIFT;
   else if (shader->GetOpcodeSlot(shader, merged))
      hwmask = (shader->inst[3] & NV30_VP_INST_STEMP_WRITEMASK_MASK)
	 >> NV30_VP_INST_STEMP_WRITEMASK_SHIFT;
   else
      hwmask = (shader->inst[3] & NV30_VP_INST_VTEMP_WRITEMASK_MASK)
	 >> NV30_VP_INST_VTEMP_WRITEMASK_SHIFT;

   if (hwmask & (1 << 3)) mask |= SMASK_X;
   if (hwmask & (1 << 2)) mask |= SMASK_Y;
   if (hwmask & (1 << 1)) mask |= SMASK_Z;
   if (hwmask & (1 << 0)) mask |= SMASK_W;

   return mask;
}

static int
NV30VPGetSourceID(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;

   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_TEMP:
      src = shader->GetSourceHW(shader, merged, pos);
      return ((src & NV30_VP_SRC_REG_TEMP_ID_MASK) >>
	      NV30_VP_SRC_REG_TEMP_ID_SHIFT);
   case NVS_FILE_CONST:
      return ((shader->inst[1] & NV30_VP_INST_CONST_SRC_MASK)
	      >> NV30_VP_INST_CONST_SRC_SHIFT);
   case NVS_FILE_ATTRIB:
      src = ((shader->inst[1] & NV30_VP_INST_INPUT_SRC_MASK)
	     >> NV30_VP_INST_INPUT_SRC_SHIFT);
      switch (src) {
      case NV30_VP_INST_IN_POS  : return NVS_FR_POSITION;
      case NV30_VP_INST_IN_COL0 : return NVS_FR_COL0;
      case NV30_VP_INST_IN_COL1 : return NVS_FR_COL1;
      case NV30_VP_INST_IN_TC(0): return NVS_FR_TEXCOORD0;
      case NV30_VP_INST_IN_TC(1): return NVS_FR_TEXCOORD1;
      case NV30_VP_INST_IN_TC(2): return NVS_FR_TEXCOORD2;
      case NV30_VP_INST_IN_TC(3): return NVS_FR_TEXCOORD3;
      case NV30_VP_INST_IN_TC(4): return NVS_FR_TEXCOORD4;
      case NV30_VP_INST_IN_TC(5): return NVS_FR_TEXCOORD5;
      case NV30_VP_INST_IN_TC(6): return NVS_FR_TEXCOORD6;
      case NV30_VP_INST_IN_TC(7): return NVS_FR_TEXCOORD7;
      default:
	 return NVS_FR_UNKNOWN;
      }
   default:
      return -1;
   }
}

static int
NV30VPGetSourceAbs(nvsFunc * shader, int merged, int pos)
{
   struct _op_xlat *opr;
   static unsigned int abspos[3] = {
      NV30_VP_INST_SRC0_ABS,
      NV30_VP_INST_SRC1_ABS,
      NV30_VP_INST_SRC2_ABS,
   };

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1 || opr->srcpos[pos] > 2)
      return 0;

   return ((shader->inst[0] & abspos[opr->srcpos[pos]]) ? 1 : 0);
}

static int
NV30VPGetRelAddressRegID(nvsFunc * shader)
{
   return ((shader->inst[0] & NV30_VP_INST_ADDR_REG_SELECT_1) ? 1 : 0);
}

static nvsSwzComp
NV30VPGetRelAddressSwizzle(nvsFunc * shader)
{
   nvsSwzComp swz;

   swz = NV20VP_TX_SWIZZLE[(shader->inst[0] & NV30_VP_INST_ADDR_SWZ_MASK)
			   >> NV30_VP_INST_ADDR_SWZ_SHIFT];
   return swz;
}

static int
NV30VPSupportsConditional(nvsFunc * shader)
{
   /*FIXME: Is this true of all ops? */
   return 1;
}

static int
NV30VPGetConditionUpdate(nvsFunc * shader)
{
   return ((shader->inst[0] & NV30_VP_INST_COND_UPDATE_ENABLE) ? 1 : 0);
}

static int
NV30VPGetConditionTest(nvsFunc * shader)
{
   int op;

   /* The condition test is unconditionally enabled on some
    * instructions. ie: the condition test bit does *NOT* have
    * to be set.
    *
    * FIXME: check other relevant ops for this situation.
    */
   op = shader->GetOpcodeHW(shader, 1);
   switch (op) {
   case NV30_VP_INST_OP_BRA:
      return 1;
   default:
      return ((shader->inst[0] & NV30_VP_INST_COND_TEST_ENABLE) ? 1 : 0);
   }
}

static nvsCond
NV30VPGetCondition(nvsFunc * shader)
{
   int cond;

   cond = ((shader->inst[0] & NV30_VP_INST_COND_MASK)
	   >> NV30_VP_INST_COND_SHIFT);

   switch (cond) {
   case NV30_VP_INST_COND_FL: return NVS_COND_FL;
   case NV30_VP_INST_COND_LT: return NVS_COND_LT;
   case NV30_VP_INST_COND_EQ: return NVS_COND_EQ;
   case NV30_VP_INST_COND_LE: return NVS_COND_LE;
   case NV30_VP_INST_COND_GT: return NVS_COND_GT;
   case NV30_VP_INST_COND_NE: return NVS_COND_NE;
   case NV30_VP_INST_COND_GE: return NVS_COND_GE;
   case NV30_VP_INST_COND_TR: return NVS_COND_TR;
   default:
      return NVS_COND_UNKNOWN;
   }
}

static void
NV30VPGetCondRegSwizzle(nvsFunc * shader, nvsSwzComp *swz)
{
   int swzbits;

   swzbits = (shader->inst[0] & NV30_VP_INST_COND_SWZ_ALL_MASK)
      >> NV30_VP_INST_COND_SWZ_ALL_SHIFT;
   NV20VPTXSwizzle(swzbits, swz);
}

static int
NV30VPGetCondRegID(nvsFunc * shader)
{
   return 0;
}


static int
NV30VPGetBranch(nvsFunc * shader)
{
   return ((shader->inst[2] & NV30_VP_INST_IADDR_MASK)
	   >> NV30_VP_INST_IADDR_SHIFT);
}

void
NV30VPInitShaderFuncs(nvsFunc * shader)
{
   /* Inherit NV20 code, a lot of it is the same */
   NV20VPInitShaderFuncs(shader);

   /* Increase max valid opcode ID, and add new instructions */
   NVVP_TX_VOP_COUNT = NVVP_TX_NVS_OP_COUNT = 32;

   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_FRC, NVS_OP_FRC,  0, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_FLR, NVS_OP_FLR,  0, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SEQ, NVS_OP_SEQ,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SFL, NVS_OP_SFL,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SGT, NVS_OP_SGT,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SLE, NVS_OP_SLE,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SNE, NVS_OP_SNE,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_STR, NVS_OP_STR,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_SSG, NVS_OP_SSG,  0, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_ARR, NVS_OP_ARR,  0, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV30_VP_INST_OP_ARA, NVS_OP_ARA,  3, -1, -1);

   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_BRA, NVS_OP_BRA, -1, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_CAL, NVS_OP_CAL, -1, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_RET, NVS_OP_RET, -1, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_LG2, NVS_OP_LG2,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_EX2, NVS_OP_EX2,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_SIN, NVS_OP_SIN,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV30_VP_INST_OP_COS, NVS_OP_COS,  2, -1, -1);

   shader->UploadToHW		= NV30VPUploadToHW;
   shader->UpdateConst		= NV30VPUpdateConst;

   shader->GetOpcodeHW		= NV30VPGetOpcodeHW;

   shader->GetDestFile		= NV30VPGetDestFile;
   shader->GetDestID		= NV30VPGetDestID;
   shader->GetDestMask		= NV30VPGetDestMask;

   shader->GetSourceID		= NV30VPGetSourceID;
   shader->GetSourceAbs		= NV30VPGetSourceAbs;

   shader->GetRelAddressRegID	= NV30VPGetRelAddressRegID;
   shader->GetRelAddressSwizzle	= NV30VPGetRelAddressSwizzle;

   shader->SupportsConditional	= NV30VPSupportsConditional;
   shader->GetConditionUpdate	= NV30VPGetConditionUpdate;
   shader->GetConditionTest	= NV30VPGetConditionTest;
   shader->GetCondition		= NV30VPGetCondition;
   shader->GetCondRegSwizzle	= NV30VPGetCondRegSwizzle;
   shader->GetCondRegID		= NV30VPGetCondRegID;

   shader->GetBranch		= NV30VPGetBranch;
   shader->SetBranchTarget	= NV30VPSetBranchTarget;
}

