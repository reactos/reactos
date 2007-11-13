#include "nouveau_shader.h"
#include "nouveau_msg.h"
#include "nv40_shader.h"

/*****************************************************************************
 * Assembly routines
 */
static int
NV40VPSupportsOpcode(nvsFunc * shader, nvsOpcode op)
{
   if (shader->GetOPTXFromSOP(op, NULL))
      return 1;
   return 0;
}

static void
NV40VPSetOpcode(nvsFunc *shader, unsigned int opcode, int slot)
{
   if (slot) {
      shader->inst[1] &= ~NV40_VP_INST_SCA_OPCODE_MASK;
      shader->inst[1] |= (opcode << NV40_VP_INST_SCA_OPCODE_SHIFT);
   } else {
      shader->inst[1] &= ~NV40_VP_INST_VEC_OPCODE_MASK;
      shader->inst[1] |= (opcode << NV40_VP_INST_VEC_OPCODE_SHIFT);
   }
}

static void
NV40VPSetCCUpdate(nvsFunc *shader)
{
   shader->inst[0] |= NV40_VP_INST_COND_UPDATE_ENABLE;
}

static void
NV40VPSetCondition(nvsFunc *shader, int on, nvsCond cond, int reg,
      		   nvsSwzComp *swizzle)
{
   unsigned int hwcond;

   if (on ) shader->inst[0] |= NV40_VP_INST_COND_TEST_ENABLE;
   else     shader->inst[0] &= ~NV40_VP_INST_COND_TEST_ENABLE;
   if (reg) shader->inst[0] |= NV40_VP_INST_COND_REG_SELECT_1;
   else     shader->inst[0] &= ~NV40_VP_INST_COND_REG_SELECT_1;

   switch (cond) {
   case NVS_COND_TR: hwcond = NV40_VP_INST_COND_TR; break;
   case NVS_COND_FL: hwcond = NV40_VP_INST_COND_FL; break;
   case NVS_COND_LT: hwcond = NV40_VP_INST_COND_LT; break;
   case NVS_COND_GT: hwcond = NV40_VP_INST_COND_GT; break;
   case NVS_COND_NE: hwcond = NV40_VP_INST_COND_NE; break;
   case NVS_COND_EQ: hwcond = NV40_VP_INST_COND_EQ; break;
   case NVS_COND_GE: hwcond = NV40_VP_INST_COND_GE; break;
   case NVS_COND_LE: hwcond = NV40_VP_INST_COND_LE; break;
   default:
	WARN_ONCE("unknown vp cond %d\n", cond);
	hwcond = NV40_VP_INST_COND_TR;
	break;
   }
   shader->inst[0] &= ~NV40_VP_INST_COND_MASK;
   shader->inst[0] |= (hwcond << NV40_VP_INST_COND_SHIFT);

   shader->inst[0] &= ~NV40_VP_INST_COND_SWZ_ALL_MASK;
   shader->inst[0] |= (swizzle[NVS_SWZ_X] << NV40_VP_INST_COND_SWZ_X_SHIFT);
   shader->inst[0] |= (swizzle[NVS_SWZ_Y] << NV40_VP_INST_COND_SWZ_Y_SHIFT);
   shader->inst[0] |= (swizzle[NVS_SWZ_Z] << NV40_VP_INST_COND_SWZ_Z_SHIFT);
   shader->inst[0] |= (swizzle[NVS_SWZ_W] << NV40_VP_INST_COND_SWZ_W_SHIFT);
}

/* these just exist here until nouveau_reg.h has them. */
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_COL0	(1<<0)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_COL1	(1<<1)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_BFC0	(1<<2)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_BFC1	(1<<3)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_FOGC	(1<<4)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_PSZ	(1<<5)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP0	(1<<6)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP1	(1<<7)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP2	(1<<8)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP3	(1<<9)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP4	(1<<10)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP5	(1<<11)
#define NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_TEX0   (1<<14)

static unsigned int
NV40VPTranslateResultReg(nvsFunc *shader, nvsFixedReg result,
					  unsigned int *mask_ret)
{
	unsigned int *out_reg = &shader->card_priv->NV30VP.vp_out_reg;
	unsigned int *clip_en = &shader->card_priv->NV30VP.clip_enables;

	*mask_ret = 0xf;

	switch (result) {
	case NVS_FR_POSITION:
		/* out_reg POS implied */
		return NV40_VP_INST_DEST_POS;
	case NVS_FR_COL0:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_COL0;
		return NV40_VP_INST_DEST_COL0;
	case NVS_FR_COL1:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_COL1;
		return NV40_VP_INST_DEST_COL1;
	case NVS_FR_BFC0:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_BFC0;
		return NV40_VP_INST_DEST_BFC0;
	case NVS_FR_BFC1:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_BFC1;
		return NV40_VP_INST_DEST_BFC1;
	case NVS_FR_FOGCOORD:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_FOGC;
		*mask_ret = 0x8;
		return NV40_VP_INST_DEST_FOGC;
	case NVS_FR_CLIP0:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP0;
		(*clip_en) |= 0x00000002;
		*mask_ret = 0x4;
		return NV40_VP_INST_DEST_FOGC;
	case NVS_FR_CLIP1:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP1;
		(*clip_en) |= 0x00000020;
		*mask_ret = 0x2;
		return NV40_VP_INST_DEST_FOGC;
	case NVS_FR_CLIP2:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP2;
		(*clip_en) |= 0x00000200;
		*mask_ret = 0x1;
		return NV40_VP_INST_DEST_FOGC;
	case NVS_FR_POINTSZ:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_PSZ;
		*mask_ret = 0x8;
		return NV40_VP_INST_DEST_PSZ;
	case NVS_FR_CLIP3:
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP3;
		(*clip_en) |= 0x00002000;
		*mask_ret = 0x4;
		return NV40_VP_INST_DEST_PSZ;
	case NVS_FR_CLIP4:
		(*clip_en) |= 0x00020000;
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP4;
		*mask_ret = 0x2;
		return NV40_VP_INST_DEST_PSZ;
	case NVS_FR_CLIP5:
		(*clip_en) |= 0x00200000;
		(*out_reg) |= NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_CLP5;
		*mask_ret = 0x1;
		return NV40_VP_INST_DEST_PSZ;
	case NVS_FR_TEXCOORD0:
	case NVS_FR_TEXCOORD1:
	case NVS_FR_TEXCOORD2:
	case NVS_FR_TEXCOORD3:
	case NVS_FR_TEXCOORD4:
	case NVS_FR_TEXCOORD5:
	case NVS_FR_TEXCOORD6:
	case NVS_FR_TEXCOORD7:
	{
		int unit = result - NVS_FR_TEXCOORD0;
		(*out_reg) |= (NV30_TCL_PRIMITIVE_3D_VP_OUT_REG_TEX0 << unit);
		return NV40_VP_INST_DEST_TC(unit);
	}
	default:
		WARN_ONCE("unknown vp output %d\n", result);
		return NV40_VP_INST_DEST_POS;
	}
}

static void
NV40VPSetResult(nvsFunc *shader, nvsRegister * dest, unsigned int mask,
      		int slot)
{
   unsigned int hwmask = 0;

   if (mask & SMASK_X) hwmask |= (1 << 3);
   if (mask & SMASK_Y) hwmask |= (1 << 2);
   if (mask & SMASK_Z) hwmask |= (1 << 1);
   if (mask & SMASK_W) hwmask |= (1 << 0);

   if (dest->file == NVS_FILE_RESULT) {
      unsigned int valid_mask;
      int hwidx;

      hwidx = NV40VPTranslateResultReg(shader, dest->index, &valid_mask);
      if (hwmask & ~valid_mask)
	      WARN_ONCE("writing invalid components of result reg\n");
      hwmask &= valid_mask;

      shader->inst[3] &= ~NV40_VP_INST_DEST_MASK;
      shader->inst[3] |= (hwidx << NV40_VP_INST_DEST_SHIFT);

      if (slot) shader->inst[3] |= NV40_VP_INST_SCA_RESULT;
      else      shader->inst[0] |= NV40_VP_INST_VEC_RESULT;
   } else {
      /* NVS_FILE_TEMP || NVS_FILE_ADDRESS */
      if (slot) {
	 shader->inst[3] &= ~NV40_VP_INST_SCA_RESULT;
	 shader->inst[3] &= ~NV40_VP_INST_SCA_DEST_TEMP_MASK;
	 shader->inst[3] |= (dest->index << NV40_VP_INST_SCA_DEST_TEMP_SHIFT);
      } else {
	 shader->inst[0] &= ~NV40_VP_INST_VEC_RESULT;
	 shader->inst[0] &= ~(NV40_VP_INST_VEC_DEST_TEMP_MASK | (1<<20));
	 shader->inst[0] |= (dest->index << NV40_VP_INST_VEC_DEST_TEMP_SHIFT);
      }
   }

   if (slot) {
      shader->inst[3] &= ~NV40_VP_INST_SCA_WRITEMASK_MASK;
      shader->inst[3] |= (hwmask << NV40_VP_INST_SCA_WRITEMASK_SHIFT);
   } else {
      shader->inst[3] &= ~NV40_VP_INST_VEC_WRITEMASK_MASK;
      shader->inst[3] |= (hwmask << NV40_VP_INST_VEC_WRITEMASK_SHIFT);
   }
}

static void
NV40VPInsertSource(nvsFunc *shader, unsigned int hw, int pos)
{
   switch (pos) {
   case 0:
      shader->inst[1] &= ~NV40_VP_INST_SRC0H_MASK;
      shader->inst[2] &= ~NV40_VP_INST_SRC0L_MASK;
      shader->inst[1] |= ((hw & NV40_VP_SRC0_HIGH_MASK) >>
	    NV40_VP_SRC0_HIGH_SHIFT)
	 << NV40_VP_INST_SRC0H_SHIFT;
      shader->inst[2] |= (hw & NV40_VP_SRC0_LOW_MASK)
	 << NV40_VP_INST_SRC0L_SHIFT;
      break;
   case 1:
      shader->inst[2] &= ~NV40_VP_INST_SRC1_MASK;
      shader->inst[2] |= hw
	 << NV40_VP_INST_SRC1_SHIFT;
      break;
   case 2:
      shader->inst[2] &= ~NV40_VP_INST_SRC2H_MASK;
      shader->inst[3] &= ~NV40_VP_INST_SRC2L_MASK;
      shader->inst[2] |= ((hw & NV40_VP_SRC2_HIGH_MASK) >>
	    NV40_VP_SRC2_HIGH_SHIFT)
	 << NV40_VP_INST_SRC2H_SHIFT;
      shader->inst[3] |= (hw & NV40_VP_SRC2_LOW_MASK)
	 << NV40_VP_INST_SRC2L_SHIFT;
      break;
   default:
      assert(0);
      break;
   }
}

static void
NV40VPSetSource(nvsFunc *shader, nvsRegister * src, int pos)
{
   unsigned int hw = 0;

   switch (src->file) {
   case NVS_FILE_ADDRESS:
      break;
   case NVS_FILE_ATTRIB:
      hw |= (NV40_VP_SRC_REG_TYPE_INPUT << NV40_VP_SRC_REG_TYPE_SHIFT);

      shader->inst[1] &= ~NV40_VP_INST_INPUT_SRC_MASK;
      shader->inst[1] |= (src->index << NV40_VP_INST_INPUT_SRC_SHIFT);
      shader->card_priv->NV30VP.vp_in_reg |= (1 << src->index);
      if (src->indexed) {
	 shader->inst[0] |= NV40_VP_INST_INDEX_INPUT;
	 if (src->addr_reg)
	    shader->inst[0] |= NV40_VP_INST_ADDR_REG_SELECT_1;
	 else
	    shader->inst[0] &= ~NV40_VP_INST_ADDR_REG_SELECT_1;
	 shader->inst[0] &= ~NV40_VP_INST_ADDR_SWZ_SHIFT;
	 shader->inst[0] |= (src->addr_comp << NV40_VP_INST_ADDR_SWZ_SHIFT);
      } else
	 shader->inst[0] &= ~NV40_VP_INST_INDEX_INPUT;
      break;
   case NVS_FILE_CONST:
      hw |= (NV40_VP_SRC_REG_TYPE_CONST << NV40_VP_SRC_REG_TYPE_SHIFT);

      shader->inst[1] &= ~NV40_VP_INST_CONST_SRC_MASK;
      shader->inst[1] |= (src->index << NV40_VP_INST_CONST_SRC_SHIFT);
      if (src->indexed) {
	 shader->inst[3] |= NV40_VP_INST_INDEX_CONST;
	 if (src->addr_reg)
	    shader->inst[0] |= NV40_VP_INST_ADDR_REG_SELECT_1;
	 else
	    shader->inst[0] &= ~NV40_VP_INST_ADDR_REG_SELECT_1;
	 shader->inst[0] &= ~NV40_VP_INST_ADDR_SWZ_MASK;
	 shader->inst[0] |= (src->addr_comp << NV40_VP_INST_ADDR_SWZ_SHIFT);
      } else
	 shader->inst[3] &= ~NV40_VP_INST_INDEX_CONST;
      break;
   case NVS_FILE_TEMP:
      hw |= (NV40_VP_SRC_REG_TYPE_TEMP << NV40_VP_SRC_REG_TYPE_SHIFT);
      hw |= (src->index << NV40_VP_SRC_TEMP_SRC_SHIFT);
      break;
   default:
      fprintf(stderr, "unknown source file %d\n", src->file);
      assert(0);
      break;
   }

   if (src->file != NVS_FILE_ADDRESS) {
      if (src->negate)
	 hw |= NV40_VP_SRC_NEGATE;
      if (src->abs)
	 shader->inst[0] |= (1 << (21 + pos));
      else
	 shader->inst[0] &= ~(1 << (21 + pos));
      hw |= (src->swizzle[0] << NV40_VP_SRC_SWZ_X_SHIFT);
      hw |= (src->swizzle[1] << NV40_VP_SRC_SWZ_Y_SHIFT);
      hw |= (src->swizzle[2] << NV40_VP_SRC_SWZ_Z_SHIFT);
      hw |= (src->swizzle[3] << NV40_VP_SRC_SWZ_W_SHIFT);

      NV40VPInsertSource(shader, hw, pos);
   }
}

static void
NV40VPSetBranchTarget(nvsFunc *shader, int addr)
{
	shader->inst[2] &= ~NV40_VP_INST_IADDRH_MASK;
	shader->inst[2] |= ((addr & 0xf8) >> 3) << NV40_VP_INST_IADDRH_SHIFT;
	shader->inst[3] &= ~NV40_VP_INST_IADDRL_MASK;
	shader->inst[3] |= ((addr & 0x07) << NV40_VP_INST_IADDRL_SHIFT);
}

static void
NV40VPInitInstruction(nvsFunc *shader)
{
   unsigned int hwsrc = 0;

   shader->inst[0] = /*NV40_VP_INST_VEC_RESULT | */
      		     NV40_VP_INST_VEC_DEST_TEMP_MASK | (1<<20);
   shader->inst[1] = 0;
   shader->inst[2] = 0;
   shader->inst[3] = NV40_VP_INST_SCA_RESULT |
      		     NV40_VP_INST_SCA_DEST_TEMP_MASK |
		     NV40_VP_INST_DEST_MASK;
   
   hwsrc = (NV40_VP_SRC_REG_TYPE_INPUT << NV40_VP_SRC_REG_TYPE_SHIFT) |
      	   (NVS_SWZ_X << NV40_VP_SRC_SWZ_X_SHIFT) |
      	   (NVS_SWZ_Y << NV40_VP_SRC_SWZ_Y_SHIFT) |
      	   (NVS_SWZ_Z << NV40_VP_SRC_SWZ_Z_SHIFT) |
      	   (NVS_SWZ_W << NV40_VP_SRC_SWZ_W_SHIFT);
   NV40VPInsertSource(shader, hwsrc, 0);
   NV40VPInsertSource(shader, hwsrc, 1);
   NV40VPInsertSource(shader, hwsrc, 2);
}

static void
NV40VPSetLastInst(nvsFunc *shader)
{
   shader->inst[3] |= 1;
}

/*****************************************************************************
 * Disassembly routines
 */
static int
NV40VPHasMergedInst(nvsFunc * shader)
{
   if (shader->GetOpcodeHW(shader, 0) != NV40_VP_INST_OP_NOP &&
       shader->GetOpcodeHW(shader, 1) != NV40_VP_INST_OP_NOP)
      return 1;
   return 0;
}

static unsigned int
NV40VPGetOpcodeHW(nvsFunc * shader, int slot)
{
   int op;

   if (slot)
      op = (shader->inst[1] & NV40_VP_INST_SCA_OPCODE_MASK)
	 >> NV40_VP_INST_SCA_OPCODE_SHIFT;
   else
      op = (shader->inst[1] & NV40_VP_INST_VEC_OPCODE_MASK)
	 >> NV40_VP_INST_VEC_OPCODE_SHIFT;

   return op;
}

static nvsRegFile
NV40VPGetDestFile(nvsFunc * shader, int merged)
{
   nvsOpcode op;

   op = shader->GetOpcode(shader, merged);
   switch (op) {
   case NVS_OP_ARL:
   case NVS_OP_ARR:
   case NVS_OP_ARA:
   case NVS_OP_POPA:
      return NVS_FILE_ADDRESS;
   default:
      if (shader->GetOpcodeSlot(shader, merged)) {
	 if (shader->inst[3] & NV40_VP_INST_SCA_RESULT)
	    return NVS_FILE_RESULT;
      }
      else {
	 if (shader->inst[0] & NV40_VP_INST_VEC_RESULT)
	    return NVS_FILE_RESULT;
      }
      return NVS_FILE_TEMP;
   }

}

static unsigned int
NV40VPGetDestID(nvsFunc * shader, int merged)
{
   int id;

   switch (shader->GetDestFile(shader, merged)) {
   case NVS_FILE_RESULT:
      id = ((shader->inst[3] & NV40_VP_INST_DEST_MASK)
	    >> NV40_VP_INST_DEST_SHIFT);
      switch (id) {
      case NV40_VP_INST_DEST_POS : return NVS_FR_POSITION;
      case NV40_VP_INST_DEST_COL0: return NVS_FR_COL0;
      case NV40_VP_INST_DEST_COL1: return NVS_FR_COL1;
      case NV40_VP_INST_DEST_BFC0: return NVS_FR_BFC0;
      case NV40_VP_INST_DEST_BFC1: return NVS_FR_BFC1;
      case NV40_VP_INST_DEST_FOGC: {
	    int mask = shader->GetDestMask(shader, merged);
	    switch (mask) {
	    case SMASK_X: return NVS_FR_FOGCOORD;
	    case SMASK_Y: return NVS_FR_CLIP0;
	    case SMASK_Z: return NVS_FR_CLIP1;
	    case SMASK_W: return NVS_FR_CLIP2;
	    default:
	       printf("more than 1 mask component set in FOGC writemask!\n");
	       return NVS_FR_UNKNOWN;
	    }
	 }
      case NV40_VP_INST_DEST_PSZ:
	 {
	    int mask = shader->GetDestMask(shader, merged);
	    switch (mask) {
	    case SMASK_X: return NVS_FR_POINTSZ;
	    case SMASK_Y: return NVS_FR_CLIP3;
	    case SMASK_Z: return NVS_FR_CLIP4;
	    case SMASK_W: return NVS_FR_CLIP5;
	    default:
	       printf("more than 1 mask component set in PSZ writemask!\n");
	       return NVS_FR_UNKNOWN;
	    }
	 }
      case NV40_VP_INST_DEST_TC(0): return NVS_FR_TEXCOORD0;
      case NV40_VP_INST_DEST_TC(1): return NVS_FR_TEXCOORD1;
      case NV40_VP_INST_DEST_TC(2): return NVS_FR_TEXCOORD2;
      case NV40_VP_INST_DEST_TC(3): return NVS_FR_TEXCOORD3;
      case NV40_VP_INST_DEST_TC(4): return NVS_FR_TEXCOORD4;
      case NV40_VP_INST_DEST_TC(5): return NVS_FR_TEXCOORD5;
      case NV40_VP_INST_DEST_TC(6): return NVS_FR_TEXCOORD6;
      case NV40_VP_INST_DEST_TC(7): return NVS_FR_TEXCOORD7;
      default:
	 return -1;
      }
   case NVS_FILE_ADDRESS:
      /* Instructions that write address regs are encoded as if
       * they would write temps.
       */
   case NVS_FILE_TEMP:
      if (shader->GetOpcodeSlot(shader, merged))
	 id = ((shader->inst[3] & NV40_VP_INST_SCA_DEST_TEMP_MASK)
	       >> NV40_VP_INST_SCA_DEST_TEMP_SHIFT);
      else
	 id = ((shader->inst[0] & NV40_VP_INST_VEC_DEST_TEMP_MASK)
	       >> NV40_VP_INST_VEC_DEST_TEMP_SHIFT);
      return id;
   default:
      return -1;
   }
}

static unsigned int
NV40VPGetDestMask(nvsFunc * shader, int merged)
{
   unsigned int mask = 0;

   if (shader->GetOpcodeSlot(shader, merged)) {
      if (shader->inst[3] & NV40_VP_INST_SCA_WRITEMASK_X) mask |= SMASK_X;
      if (shader->inst[3] & NV40_VP_INST_SCA_WRITEMASK_Y) mask |= SMASK_Y;
      if (shader->inst[3] & NV40_VP_INST_SCA_WRITEMASK_Z) mask |= SMASK_Z;
      if (shader->inst[3] & NV40_VP_INST_SCA_WRITEMASK_W) mask |= SMASK_W;
   } else {
      if (shader->inst[3] & NV40_VP_INST_VEC_WRITEMASK_X) mask |= SMASK_X;
      if (shader->inst[3] & NV40_VP_INST_VEC_WRITEMASK_Y) mask |= SMASK_Y;
      if (shader->inst[3] & NV40_VP_INST_VEC_WRITEMASK_Z) mask |= SMASK_Z;
      if (shader->inst[3] & NV40_VP_INST_VEC_WRITEMASK_W) mask |= SMASK_W;
   }

   return mask;
}

static unsigned int
NV40VPGetSourceHW(nvsFunc * shader, int merged, int pos)
{
   struct _op_xlat *opr;
   unsigned int src;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr)
      return -1;

   switch (opr->srcpos[pos]) {
   case 0:
      src = ((shader->inst[1] & NV40_VP_INST_SRC0H_MASK)
	     >> NV40_VP_INST_SRC0H_SHIFT)
	 << NV40_VP_SRC0_HIGH_SHIFT;
      src |= ((shader->inst[2] & NV40_VP_INST_SRC0L_MASK)
	      >> NV40_VP_INST_SRC0L_SHIFT);
      break;
   case 1:
      src = ((shader->inst[2] & NV40_VP_INST_SRC1_MASK)
	     >> NV40_VP_INST_SRC1_SHIFT);
      break;
   case 2:
      src = ((shader->inst[2] & NV40_VP_INST_SRC2H_MASK)
	     >> NV40_VP_INST_SRC2H_SHIFT)
	 << NV40_VP_SRC2_HIGH_SHIFT;
      src |= ((shader->inst[3] & NV40_VP_INST_SRC2L_MASK)
	      >> NV40_VP_INST_SRC2L_SHIFT);
      break;
   default:
      src = -1;
   }

   return src;
}

static nvsRegFile
NV40VPGetSourceFile(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;
   struct _op_xlat *opr;
   int file;

   opr = shader->GetOPTXRec(shader, merged);
   if (!opr || opr->srcpos[pos] == -1)
      return -1;

   switch (opr->srcpos[pos]) {
   case SPOS_ADDRESS: return NVS_FILE_ADDRESS;
   default:
      src = shader->GetSourceHW(shader, merged, pos);
      file = (src & NV40_VP_SRC_REG_TYPE_MASK) >> NV40_VP_SRC_REG_TYPE_SHIFT;

      switch (file) {
      case NV40_VP_SRC_REG_TYPE_TEMP : return NVS_FILE_TEMP;
      case NV40_VP_SRC_REG_TYPE_INPUT: return NVS_FILE_ATTRIB;
      case NV40_VP_SRC_REG_TYPE_CONST: return NVS_FILE_CONST;
      default:
	 return NVS_FILE_UNKNOWN;
      }
   }
}

static int
NV40VPGetSourceID(nvsFunc * shader, int merged, int pos)
{
   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_ATTRIB:
      switch ((shader->inst[1] & NV40_VP_INST_INPUT_SRC_MASK)
	      >> NV40_VP_INST_INPUT_SRC_SHIFT) {
      case NV40_VP_INST_IN_POS:		return NVS_FR_POSITION;
      case NV40_VP_INST_IN_WEIGHT:	return NVS_FR_WEIGHT;
      case NV40_VP_INST_IN_NORMAL:	return NVS_FR_NORMAL;
      case NV40_VP_INST_IN_COL0:	return NVS_FR_COL0;
      case NV40_VP_INST_IN_COL1:	return NVS_FR_COL1;
      case NV40_VP_INST_IN_FOGC:	return NVS_FR_FOGCOORD;
      case NV40_VP_INST_IN_TC(0):	return NVS_FR_TEXCOORD0;
      case NV40_VP_INST_IN_TC(1):	return NVS_FR_TEXCOORD1;
      case NV40_VP_INST_IN_TC(2):	return NVS_FR_TEXCOORD2;
      case NV40_VP_INST_IN_TC(3):	return NVS_FR_TEXCOORD3;
      case NV40_VP_INST_IN_TC(4):	return NVS_FR_TEXCOORD4;
      case NV40_VP_INST_IN_TC(5):	return NVS_FR_TEXCOORD5;
      case NV40_VP_INST_IN_TC(6):	return NVS_FR_TEXCOORD6;
      case NV40_VP_INST_IN_TC(7):	return NVS_FR_TEXCOORD7;
      default:
	 return -1;
      }
      break;
   case NVS_FILE_CONST:
      return ((shader->inst[1] & NV40_VP_INST_CONST_SRC_MASK)
	      >> NV40_VP_INST_CONST_SRC_SHIFT);
   case NVS_FILE_TEMP:
      {
	 unsigned int src;

	 src = shader->GetSourceHW(shader, merged, pos);
	 return ((src & NV40_VP_SRC_TEMP_SRC_MASK) >>
		 NV40_VP_SRC_TEMP_SRC_SHIFT);
      }
   default:
      return -1;
   }
}

static int
NV40VPGetSourceNegate(nvsFunc * shader, int merged, int pos)
{
   unsigned int src;

   src = shader->GetSourceHW(shader, merged, pos);

   if (src == -1)
      return -1;
   return ((src & NV40_VP_SRC_NEGATE) ? 1 : 0);
}

static void
NV40VPGetSourceSwizzle(nvsFunc * shader, int merged, int pos, nvsSwzComp *swz)
{
   unsigned int src;
   int swzbits;

   src = shader->GetSourceHW(shader, merged, pos);
   swzbits = (src & NV40_VP_SRC_SWZ_ALL_MASK) >> NV40_VP_SRC_SWZ_ALL_SHIFT;
   NV20VPTXSwizzle(swzbits, swz);
}

static int
NV40VPGetSourceIndexed(nvsFunc * shader, int merged, int pos)
{
   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_ATTRIB:
      return ((shader->inst[0] & NV40_VP_INST_INDEX_INPUT) ? 1 : 0);
   case NVS_FILE_CONST:
      return ((shader->inst[3] & NV40_VP_INST_INDEX_CONST) ? 1 : 0);
   default:
      return 0;
   }
}

static nvsSwzComp
NV40VPGetAddressRegSwizzle(nvsFunc * shader)
{
   nvsSwzComp swz;

   swz = NV20VP_TX_SWIZZLE[(shader->inst[0] & NV40_VP_INST_ADDR_SWZ_MASK)
			   >> NV40_VP_INST_ADDR_SWZ_SHIFT];
   return swz;
}

static int
NV40VPSupportsConditional(nvsFunc * shader)
{
   /*FIXME: Is this true of all ops? */
   return 1;
}

static int
NV40VPGetConditionUpdate(nvsFunc * shader)
{
   return ((shader->inst[0] & NV40_VP_INST_COND_UPDATE_ENABLE) ? 1 : 0);
}

static int
NV40VPGetConditionTest(nvsFunc * shader)
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
   case NV40_VP_INST_OP_BRA:
      return 1;
   default:
      return ((shader->inst[0] & NV40_VP_INST_COND_TEST_ENABLE) ? 1 : 0);
   }
}

static nvsCond
NV40VPGetCondition(nvsFunc * shader)
{
   int cond;

   cond = ((shader->inst[0] & NV40_VP_INST_COND_MASK)
	   >> NV40_VP_INST_COND_SHIFT);

   switch (cond) {
   case NV40_VP_INST_COND_FL: return NVS_COND_FL;
   case NV40_VP_INST_COND_LT: return NVS_COND_LT;
   case NV40_VP_INST_COND_EQ: return NVS_COND_EQ;
   case NV40_VP_INST_COND_LE: return NVS_COND_LE;
   case NV40_VP_INST_COND_GT: return NVS_COND_GT;
   case NV40_VP_INST_COND_NE: return NVS_COND_NE;
   case NV40_VP_INST_COND_GE: return NVS_COND_GE;
   case NV40_VP_INST_COND_TR: return NVS_COND_TR;
   default:
      return NVS_COND_UNKNOWN;
   }
}

static void
NV40VPGetCondRegSwizzle(nvsFunc * shader, nvsSwzComp *swz)
{
   int swzbits;

   swzbits = (shader->inst[0] & NV40_VP_INST_COND_SWZ_ALL_MASK)
      >> NV40_VP_INST_COND_SWZ_ALL_SHIFT;
   NV20VPTXSwizzle(swzbits, swz);
}

static int
NV40VPGetCondRegID(nvsFunc * shader)
{
   return ((shader->inst[0] & NV40_VP_INST_COND_REG_SELECT_1) ? 1 : 0);
}

static int
NV40VPGetBranch(nvsFunc * shader)
{
   int addr;

   addr = ((shader->inst[2] & NV40_VP_INST_IADDRH_MASK)
	   >> NV40_VP_INST_IADDRH_SHIFT) << 3;
   addr |= ((shader->inst[3] & NV40_VP_INST_IADDRL_MASK)
	    >> NV40_VP_INST_IADDRL_SHIFT);
   return addr;
}

void
NV40VPInitShaderFuncs(nvsFunc * shader)
{
   /* Inherit NV30 VP code, we share some of it */
   NV30VPInitShaderFuncs(shader);

   /* Limits */
   shader->MaxInst	= 4096;
   shader->MaxAttrib	= 16;
   shader->MaxTemp	= 32;
   shader->MaxAddress	= 2;
   shader->MaxConst	= 256;
   shader->caps		= SCAP_SRC_ABS;

   /* Add extra opcodes for NV40+ */
//      MOD_OPCODE(NVVP_TX_VOP, NV40_VP_INST_OP_TXWHAT, NVS_OP_TEX  ,  0,  4, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV40_VP_INST_OP_PUSHA, NVS_OP_PUSHA,  3, -1, -1);
   MOD_OPCODE(NVVP_TX_SOP, NV40_VP_INST_OP_POPA , NVS_OP_POPA , -1, -1, -1);

   shader->InitInstruction	= NV40VPInitInstruction;
   shader->SupportsOpcode	= NV40VPSupportsOpcode;
   shader->SetOpcode		= NV40VPSetOpcode;
   shader->SetCCUpdate		= NV40VPSetCCUpdate;
   shader->SetCondition		= NV40VPSetCondition;
   shader->SetResult		= NV40VPSetResult;
   shader->SetSource		= NV40VPSetSource;
   shader->SetLastInst		= NV40VPSetLastInst;
   shader->SetBranchTarget	= NV40VPSetBranchTarget;

   shader->HasMergedInst	= NV40VPHasMergedInst;
   shader->GetOpcodeHW		= NV40VPGetOpcodeHW;

   shader->GetDestFile		= NV40VPGetDestFile;
   shader->GetDestID		= NV40VPGetDestID;
   shader->GetDestMask		= NV40VPGetDestMask;

   shader->GetSourceHW		= NV40VPGetSourceHW;
   shader->GetSourceFile	= NV40VPGetSourceFile;
   shader->GetSourceID		= NV40VPGetSourceID;
   shader->GetSourceNegate	= NV40VPGetSourceNegate;
   shader->GetSourceSwizzle	= NV40VPGetSourceSwizzle;
   shader->GetSourceIndexed	= NV40VPGetSourceIndexed;

   shader->GetRelAddressSwizzle	= NV40VPGetAddressRegSwizzle;

   shader->SupportsConditional	= NV40VPSupportsConditional;
   shader->GetConditionUpdate	= NV40VPGetConditionUpdate;
   shader->GetConditionTest	= NV40VPGetConditionTest;
   shader->GetCondition		= NV40VPGetCondition;
   shader->GetCondRegSwizzle	= NV40VPGetCondRegSwizzle;
   shader->GetCondRegID		= NV40VPGetCondRegID;

   shader->GetBranch		= NV40VPGetBranch;
}
