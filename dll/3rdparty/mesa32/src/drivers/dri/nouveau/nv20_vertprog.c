#include "nouveau_context.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"

#include "nouveau_shader.h"
#include "nv20_shader.h"

unsigned int NVVP_TX_VOP_COUNT = 16;
unsigned int NVVP_TX_NVS_OP_COUNT = 16;
struct _op_xlat NVVP_TX_VOP[32];
struct _op_xlat NVVP_TX_SOP[32];

nvsSwzComp NV20VP_TX_SWIZZLE[4] = { NVS_SWZ_X, NVS_SWZ_Y, NVS_SWZ_Z, NVS_SWZ_W };

/*****************************************************************************
 * Support routines
 */
static void
NV20VPUploadToHW(GLcontext *ctx, nouveauShader *nvs)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
   int i;

   /* XXX: missing a way to say what insn we're uploading from, and possible
    *      the program start position (if NV20 has one) */
   for (i=0; i<nvs->program_size; i+=4) {
      BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_INST0, 4);
      OUT_RING(nvs->program[i + 0]);
      OUT_RING(nvs->program[i + 1]);
      OUT_RING(nvs->program[i + 2]);
      OUT_RING(nvs->program[i + 3]);
   }
}

static void
NV20VPUpdateConst(GLcontext *ctx, nouveauShader *nvs, int id)
{
   nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

   /* Worth checking if the value *actually* changed? Mesa doesn't tell us this
    * as far as I know..
    */
   BEGIN_RING_SIZE(NvSub3D, NV30_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_ID, 1);
   OUT_RING (id);
   BEGIN_RING_SIZE(NvSub3D, NV20_TCL_PRIMITIVE_3D_VP_UPLOAD_CONST_X, 4);
   OUT_RINGf(nvs->params[id].source_val[0]);
   OUT_RINGf(nvs->params[id].source_val[1]);
   OUT_RINGf(nvs->params[id].source_val[2]);
   OUT_RINGf(nvs->params[id].source_val[3]);
}

/*****************************************************************************
 * Assembly routines
 */

/*****************************************************************************
 * Disassembly routines
 */
void
NV20VPTXSwizzle(int hwswz, nvsSwzComp *swz)
{
   swz[NVS_SWZ_X] = NV20VP_TX_SWIZZLE[(hwswz & 0xC0) >> 6];
   swz[NVS_SWZ_Y] = NV20VP_TX_SWIZZLE[(hwswz & 0x30) >> 4];
   swz[NVS_SWZ_Z] = NV20VP_TX_SWIZZLE[(hwswz & 0x0C) >> 2];
   swz[NVS_SWZ_W] = NV20VP_TX_SWIZZLE[(hwswz & 0x03) >> 0];
}

static int
NV20VPHasMergedInst(nvsFunc * shader)
{
   if (shader->GetOpcodeHW(shader, 0) != NV20_VP_INST_OPCODE_NOP &&
       shader->GetOpcodeHW(shader, 1) != NV20_VP_INST_OPCODE_NOP)
      printf
	 ("\n\n*****both opcode fields have values - PLEASE REPORT*****\n");
   return 0;
}

static int
NV20VPIsLastInst(nvsFunc * shader)
{
   return ((shader->inst[3] & (1 << 0)) ? 1 : 0);
}

static int
NV20VPGetOffsetNext(nvsFunc * shader)
{
   return 4;
}

static struct _op_xlat *
NV20VPGetOPTXRec(nvsFunc * shader, int merged)
{
   struct _op_xlat *opr;
   int op;

   if (shader->GetOpcodeSlot(shader, merged)) {
      opr = NVVP_TX_SOP;
      op = shader->GetOpcodeHW(shader, 1);
      if (op >= NVVP_TX_NVS_OP_COUNT)
	 return NULL;
   }
   else {
      opr = NVVP_TX_VOP;
      op = shader->GetOpcodeHW(shader, 0);
      if (op >= NVVP_TX_VOP_COUNT)
	 return NULL;
   }

   if (opr[op].SOP == NVS_OP_UNKNOWN)
      return NULL;
   return &opr[op];
}

static struct _op_xlat *
NV20VPGetOPTXFromSOP(nvsOpcode sop, int *id)
{
   int i;

   for (i=0;i<NVVP_TX_VOP_COUNT;i++) {
      if (NVVP_TX_VOP[i].SOP == sop) {
	 if (id) *id = 0;
	 return &NVVP_TX_VOP[i];
      }
   }

   for (i=0;i<NVVP_TX_NVS_OP_COUNT;i++) {
      if (NVVP_TX_SOP[i].SOP == sop) {
	 if (id) *id = 1;
	 return &NVVP_TX_SOP[i];
      }
   }

   return NULL;
}

static int
NV20VPGetOpcodeSlot(nvsFunc * shader, int merged)
{
   if (shader->HasMergedInst(shader))
      return merged;
   if (shader->GetOpcodeHW(shader, 0) == NV20_VP_INST_OPCODE_NOP)
      return 1;
   return 0;
}

static nvsOpcode
NV20VPGetOpcode(nvsFunc * shader, int merged)
{
   struct _op_xlat *opr;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr)
      return NVS_OP_UNKNOWN;

   return opr->SOP;
}

static nvsOpcode
NV20VPGetOpcodeHW(nvsFunc * shader, int slot)
{
   if (slot)
      return (shader->inst[1] & NV20_VP_INST_SCA_OPCODE_MASK)
	 >> NV20_VP_INST_SCA_OPCODE_SHIFT;
   return (shader->inst[1] & NV20_VP_INST_VEC_OPCODE_MASK)
      >> NV20_VP_INST_VEC_OPCODE_SHIFT;
}

static nvsRegFile
NV20VPGetDestFile(nvsFunc * shader, int merged)
{
   switch (shader->GetOpcode(shader, merged)) {
   case NVS_OP_ARL:
      return NVS_FILE_ADDRESS;
   default:
      /*FIXME: This probably isn't correct.. */
      if ((shader->inst[3] & NV20_VP_INST_DEST_WRITEMASK_MASK) == 0)
	 return NVS_FILE_TEMP;
      return NVS_FILE_RESULT;
   }
}

static unsigned int
NV20VPGetDestID(nvsFunc * shader, int merged)
{
   int id;

   switch (shader->GetDestFile(shader, merged)) {
   case NVS_FILE_RESULT:
      id = ((shader->inst[3] & NV20_VP_INST_DEST_MASK)
	    >> NV20_VP_INST_DEST_SHIFT);
      switch (id) {
      case NV20_VP_INST_DEST_POS  : return NVS_FR_POSITION;
      case NV20_VP_INST_DEST_COL0 : return NVS_FR_COL0;
      case NV20_VP_INST_DEST_COL1 : return NVS_FR_COL1;
      case NV20_VP_INST_DEST_TC(0): return NVS_FR_TEXCOORD0;
      case NV20_VP_INST_DEST_TC(1): return NVS_FR_TEXCOORD1;
      case NV20_VP_INST_DEST_TC(2): return NVS_FR_TEXCOORD2;
      case NV20_VP_INST_DEST_TC(3): return NVS_FR_TEXCOORD3;
      default:
	 return -1;
      }
   case NVS_FILE_ADDRESS:
      return 0;
   case NVS_FILE_TEMP:
      id = ((shader->inst[3] & NV20_VP_INST_DEST_TEMP_ID_MASK)
	    >> NV20_VP_INST_DEST_TEMP_ID_SHIFT);
      return id;
   default:
      return -1;
   }
}

static unsigned int
NV20VPGetDestMask(nvsFunc * shader, int merged)
{
   int hwmask, mask = 0;

   /* Special handling for ARL - hardware only supports a
    * 1-component address reg
    */
   if (shader->GetOpcode(shader, merged) == NVS_OP_ARL)
      return SMASK_X;

   if (shader->GetDestFile(shader, merged) == NVS_FILE_RESULT)
      hwmask = (shader->inst[3] & NV20_VP_INST_DEST_WRITEMASK_MASK)
	 >> NV20_VP_INST_DEST_WRITEMASK_SHIFT;
   else if (shader->GetOpcodeSlot(shader, merged))
      hwmask = (shader->inst[3] & NV20_VP_INST_STEMP_WRITEMASK_MASK)
	 >> NV20_VP_INST_STEMP_WRITEMASK_SHIFT;
   else
      hwmask = (shader->inst[3] & NV20_VP_INST_VTEMP_WRITEMASK_MASK)
	 >> NV20_VP_INST_VTEMP_WRITEMASK_SHIFT;

   if (hwmask & (1 << 3)) mask |= SMASK_X;
   if (hwmask & (1 << 2)) mask |= SMASK_Y;
   if (hwmask & (1 << 1)) mask |= SMASK_Z;
   if (hwmask & (1 << 0)) mask |= SMASK_W;

   return mask;
}

static unsigned int
NV20VPGetSourceHW(nvsFunc * shader, int merged, int pos)
{
   struct _op_xlat *opr;
   unsigned int src;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr)
      return -1;

   switch (opr->srcpos[pos]) {
   case 0:
      src = ((shader->inst[1] & NV20_VP_INST_SRC0H_MASK)
	     >> NV20_VP_INST_SRC0H_SHIFT)
	 << NV20_VP_SRC0_HIGH_SHIFT;
      src |= ((shader->inst[2] & NV20_VP_INST_SRC0L_MASK)
	      >> NV20_VP_INST_SRC0L_SHIFT);
      break;
   case 1:
      src = ((shader->inst[2] & NV20_VP_INST_SRC1_MASK)
	     >> NV20_VP_INST_SRC1_SHIFT);
      break;
   case 2:
      src = ((shader->inst[2] & NV20_VP_INST_SRC2H_MASK)
	     >> NV20_VP_INST_SRC2H_SHIFT)
	 << NV20_VP_SRC2_HIGH_SHIFT;
      src |= ((shader->inst[3] & NV20_VP_INST_SRC2L_MASK)
	      >> NV20_VP_INST_SRC2L_SHIFT);
      break;
   default:
      src = -1;
   }

   return src;
}

static nvsRegFile
NV20VPGetSourceFile(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;
   struct _op_xlat *opr;
   int file;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1)
      return -1;

   switch (opr->srcpos[pos]) {
   case SPOS_ADDRESS:
      return NVS_FILE_ADDRESS;
   default:
      src = NV20VPGetSourceHW(shader, merged, pos);
      file = (src & NV20_VP_SRC_REG_TYPE_MASK) >> NV20_VP_SRC_REG_TYPE_SHIFT;

      switch (file) {
      case NV20_VP_SRC_REG_TYPE_TEMP : return NVS_FILE_TEMP;
      case NV20_VP_SRC_REG_TYPE_INPUT: return NVS_FILE_ATTRIB;
      case NV20_VP_SRC_REG_TYPE_CONST: return NVS_FILE_CONST;
      default:
	 return NVS_FILE_UNKNOWN;
      }
   }
}

static int
NV20VPGetSourceID(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;

   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_TEMP:
      src = shader->GetSourceHW(shader, merged, pos);
      return ((src & NV20_VP_SRC_REG_TEMP_ID_MASK) >>
	      NV20_VP_SRC_REG_TEMP_ID_SHIFT);
   case NVS_FILE_CONST:
      return ((shader->inst[1] & NV20_VP_INST_CONST_SRC_MASK)
	      >> NV20_VP_INST_CONST_SRC_SHIFT);
   case NVS_FILE_ATTRIB:
      src = ((shader->inst[1] & NV20_VP_INST_INPUT_SRC_MASK)
	     >> NV20_VP_INST_INPUT_SRC_SHIFT);
      switch (src) {
      case NV20_VP_INST_INPUT_SRC_POS  : return NVS_FR_POSITION;
      case NV20_VP_INST_INPUT_SRC_COL0 : return NVS_FR_COL0;
      case NV20_VP_INST_INPUT_SRC_COL1 : return NVS_FR_COL1;
      case NV20_VP_INST_INPUT_SRC_TC(0): return NVS_FR_TEXCOORD0;
      case NV20_VP_INST_INPUT_SRC_TC(1): return NVS_FR_TEXCOORD1;
      case NV20_VP_INST_INPUT_SRC_TC(2): return NVS_FR_TEXCOORD2;
      case NV20_VP_INST_INPUT_SRC_TC(3): return NVS_FR_TEXCOORD3;
      default:
	 return NVS_FR_UNKNOWN;
      }
   default:
      return -1;
   }
}

static int
NV20VPGetSourceNegate(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;

   src = shader->GetSourceHW(shader, merged, pos);

   return ((src & NV20_VP_SRC_REG_NEGATE) ? 1 : 0);
}

static int
NV20VPGetSourceAbs(nvsFunc * shader, int merged, int pos)
{
   /* NV20 can't do ABS on sources?  Appears to be emulated with
    *      MAX reg, reg, -reg
    */
   return 0;
}

static void
NV20VPGetSourceSwizzle(nvsFunc * shader, int merged, int pos, nvsSwzComp *swz)
{
   unsigned int src;
   int swzbits;

   src = shader->GetSourceHW(shader, merged, pos);
   swzbits  =
      (src & NV20_VP_SRC_REG_SWZ_ALL_MASK) >> NV20_VP_SRC_REG_SWZ_ALL_SHIFT;
   return NV20VPTXSwizzle(swzbits, swz);
}

static int
NV20VPGetSourceIndexed(nvsFunc * shader, int merged, int pos)
{
   /* I don't think NV20 can index into attribs, at least no GL
    * extension is exposed that will allow it.
    */
   if (shader->GetSourceFile(shader, merged, pos) != NVS_FILE_CONST)
      return 0;
   if (shader->inst[3] & NV20_VP_INST_INDEX_CONST)
      return 1;
   return 0;
}

static int
NV20VPGetAddressRegID(nvsFunc * shader)
{
   /* Only 1 address reg */
   return 0;
}

static nvsSwzComp
NV20VPGetAddressRegSwizzle(nvsFunc * shader)
{
   /* Only A0.x available */
   return NVS_SWZ_X;
}

void
NV20VPInitShaderFuncs(nvsFunc * shader)
{
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_NOP, NVS_OP_NOP, -1, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_MOV, NVS_OP_MOV,  0, -1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_MUL, NVS_OP_MUL,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_ADD, NVS_OP_ADD,  0,  2, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_MAD, NVS_OP_MAD,  0,  1,  2);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_DP3, NVS_OP_DP3,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_DPH, NVS_OP_DPH,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_DP4, NVS_OP_DP4,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_DST, NVS_OP_DST,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_MIN, NVS_OP_MIN,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_MAX, NVS_OP_MAX,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_SLT, NVS_OP_SLT,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_SGE, NVS_OP_SGE,  0,  1, -1);
   MOD_OPCODE(NVVP_TX_VOP, NV20_VP_INST_OPCODE_ARL, NVS_OP_ARL,  0, -1, -1);

   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_NOP, NVS_OP_NOP, -1, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_RCP, NVS_OP_RCP,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_RCC, NVS_OP_RCC,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_RSQ, NVS_OP_RSQ,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_EXP, NVS_OP_EXP,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_LOG, NVS_OP_LOG,  2, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV20_VP_INST_OPCODE_LIT, NVS_OP_LIT,  2, -1, -1);

   shader->UploadToHW		= NV20VPUploadToHW;
   shader->UpdateConst		= NV20VPUpdateConst;

   shader->GetOPTXRec		= NV20VPGetOPTXRec;
   shader->GetOPTXFromSOP	= NV20VPGetOPTXFromSOP;

   shader->HasMergedInst	= NV20VPHasMergedInst;
   shader->IsLastInst		= NV20VPIsLastInst;
   shader->GetOffsetNext	= NV20VPGetOffsetNext;
   shader->GetOpcodeSlot	= NV20VPGetOpcodeSlot;
   shader->GetOpcode		= NV20VPGetOpcode;
   shader->GetOpcodeHW		= NV20VPGetOpcodeHW;
   shader->GetDestFile		= NV20VPGetDestFile;
   shader->GetDestID		= NV20VPGetDestID;
   shader->GetDestMask		= NV20VPGetDestMask;
   shader->GetSourceHW		= NV20VPGetSourceHW;
   shader->GetSourceFile	= NV20VPGetSourceFile;
   shader->GetSourceID		= NV20VPGetSourceID;
   shader->GetSourceNegate	= NV20VPGetSourceNegate;
   shader->GetSourceAbs		= NV20VPGetSourceAbs;
   shader->GetSourceSwizzle	= NV20VPGetSourceSwizzle;
   shader->GetSourceIndexed	= NV20VPGetSourceIndexed;
   shader->GetRelAddressRegID	= NV20VPGetAddressRegID;
   shader->GetRelAddressSwizzle	= NV20VPGetAddressRegSwizzle;
}
