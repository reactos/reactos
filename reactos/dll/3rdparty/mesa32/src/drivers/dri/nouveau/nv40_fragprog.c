#include "nouveau_shader.h"
#include "nv40_shader.h"

/* branching ops */
unsigned int NVFP_TX_BOP_COUNT = 5;
struct _op_xlat NVFP_TX_BOP[64];


/*****************************************************************************
 * Assembly routines
 * 	- These extend the NV30 routines, which are almost identical.  NV40
 * 	  just has branching hacked into the instruction set.
 */
static int
NV40FPSupportsResultScale(nvsFunc *shader, nvsScale scale)
{
	switch (scale) {
	case NVS_SCALE_1X:
	case NVS_SCALE_2X:
	case NVS_SCALE_4X:
	case NVS_SCALE_8X:
	case NVS_SCALE_INV_2X:
	case NVS_SCALE_INV_4X:
	case NVS_SCALE_INV_8X:
		return 1;
	default:
		return 0;
	}
}

static void
NV40FPSetResultScale(nvsFunc *shader, nvsScale scale)
{
	shader->inst[2] &= ~NV40_FP_OP_DST_SCALE_MASK;
	shader->inst[2] |= ((unsigned int)scale << NV40_FP_OP_DST_SCALE_SHIFT);
}

static void
NV40FPSetBranchTarget(nvsFunc *shader, int addr)
{
	shader->inst[2] &= ~NV40_FP_OP_IADDR_MASK;
	shader->inst[2] |= (addr << NV40_FP_OP_IADDR_SHIFT);
}

static void
NV40FPSetBranchElse(nvsFunc *shader, int addr)
{
	shader->inst[2] &= ~NV40_FP_OP_ELSE_ID_MASK;
	shader->inst[2] |= (addr << NV40_FP_OP_ELSE_ID_SHIFT);
}

static void
NV40FPSetBranchEnd(nvsFunc *shader, int addr)
{
	shader->inst[3] &= ~NV40_FP_OP_END_ID_MASK;
	shader->inst[3] |= (addr << NV40_FP_OP_END_ID_SHIFT);
}

static void
NV40FPSetLoopParams(nvsFunc *shader, int count, int initial, int increment)
{
	shader->inst[2] &= ~(NV40_FP_OP_LOOP_COUNT_MASK |
			     NV40_FP_OP_LOOP_INDEX_MASK |
			     NV40_FP_OP_LOOP_INCR_MASK);
	shader->inst[2] |= ((count     << NV40_FP_OP_LOOP_COUNT_SHIFT) |
			    (initial   << NV40_FP_OP_LOOP_INDEX_SHIFT) |
			    (increment << NV40_FP_OP_LOOP_INCR_SHIFT));
}

/*****************************************************************************
 * Disassembly routines
 */
static struct _op_xlat *
NV40FPGetOPTXRec(nvsFunc * shader, int merged)
{
   struct _op_xlat *opr;
   int op;

   op = shader->GetOpcodeHW(shader, 0);
   if (shader->inst[2] & NV40_FP_OP_OPCODE_IS_BRANCH) {
      opr = NVFP_TX_BOP;
      op &= ~NV40_FP_OP_OPCODE_IS_BRANCH;
      if (op > NVFP_TX_BOP_COUNT)
	 return NULL;
   }
   else {
      opr = NVFP_TX_AOP;
      if (op > NVFP_TX_AOP_COUNT)
	 return NULL;
   }

   if (opr[op].SOP == NVS_OP_UNKNOWN)
      return NULL;
   return &opr[op];
}

static int
NV40FPGetSourceID(nvsFunc * shader, int merged, int pos)
{
   switch (shader->GetSourceFile(shader, merged, pos)) {
   case NVS_FILE_ATTRIB:
      switch ((shader->inst[0] & NV40_FP_OP_INPUT_SRC_MASK)
	      >> NV40_FP_OP_INPUT_SRC_SHIFT) {
      case NV40_FP_OP_INPUT_SRC_POSITION: return NVS_FR_POSITION;
      case NV40_FP_OP_INPUT_SRC_COL0    : return NVS_FR_COL0;
      case NV40_FP_OP_INPUT_SRC_COL1    : return NVS_FR_COL1;
      case NV40_FP_OP_INPUT_SRC_FOGC    : return NVS_FR_FOGCOORD;
      case NV40_FP_OP_INPUT_SRC_TC(0)   : return NVS_FR_TEXCOORD0;
      case NV40_FP_OP_INPUT_SRC_TC(1)   : return NVS_FR_TEXCOORD1;
      case NV40_FP_OP_INPUT_SRC_TC(2)   : return NVS_FR_TEXCOORD2;
      case NV40_FP_OP_INPUT_SRC_TC(3)   : return NVS_FR_TEXCOORD3;
      case NV40_FP_OP_INPUT_SRC_TC(4)   : return NVS_FR_TEXCOORD4;
      case NV40_FP_OP_INPUT_SRC_TC(5)   : return NVS_FR_TEXCOORD5;
      case NV40_FP_OP_INPUT_SRC_TC(6)   : return NVS_FR_TEXCOORD6;
      case NV40_FP_OP_INPUT_SRC_TC(7)   : return NVS_FR_TEXCOORD7;
      case NV40_FP_OP_INPUT_SRC_FACING  : return NVS_FR_FACING;
      default:
	 return -1;
      }
      break;
   case NVS_FILE_TEMP:
      {
	 unsigned int src;

	 src = shader->GetSourceHW(shader, merged, pos);
	 return ((src & NV40_FP_REG_SRC_MASK) >> NV40_FP_REG_SRC_SHIFT);
      }
   case NVS_FILE_CONST:		/* inlined into fragprog */
   default:
      return -1;
   }
}

static int
NV40FPGetBranch(nvsFunc * shader)
{
   return ((shader->inst[2] & NV40_FP_OP_IADDR_MASK)
	   >> NV40_FP_OP_IADDR_SHIFT);;
}

static int
NV40FPGetBranchElse(nvsFunc * shader)
{
   return ((shader->inst[2] & NV40_FP_OP_ELSE_ID_MASK)
	   >> NV40_FP_OP_ELSE_ID_SHIFT);
}

static int
NV40FPGetBranchEnd(nvsFunc * shader)
{
   return ((shader->inst[3] & NV40_FP_OP_END_ID_MASK)
	   >> NV40_FP_OP_END_ID_SHIFT);
}

static int
NV40FPGetLoopCount(nvsFunc * shader)
{
   return ((shader->inst[2] & NV40_FP_OP_LOOP_COUNT_MASK)
	   >> NV40_FP_OP_LOOP_COUNT_SHIFT);
}

static int
NV40FPGetLoopInitial(nvsFunc * shader)
{
   return ((shader->inst[2] & NV40_FP_OP_LOOP_INDEX_MASK)
	   >> NV40_FP_OP_LOOP_INDEX_SHIFT);
}

static int
NV40FPGetLoopIncrement(nvsFunc * shader)
{
   return ((shader->inst[2] & NV40_FP_OP_LOOP_INCR_MASK)
	   >> NV40_FP_OP_LOOP_INCR_SHIFT);
}

void
NV40FPInitShaderFuncs(nvsFunc * shader)
{
   /* Inherit NV30 FP code, it's mostly the same */
   NV30FPInitShaderFuncs(shader);

   /* Kill off opcodes seen on NV30, but not seen on NV40 - need to find
    * out if these actually work or not.
    *
    * update: either LIT/RSQ don't work on nv40, or I generate bad code for
    *         them.  haven't tested the others yet
    */
   MOD_OPCODE(NVFP_TX_AOP, 0x1B, NVS_OP_UNKNOWN, -1, -1, -1);	/* NV30 RSQ */
   MOD_OPCODE(NVFP_TX_AOP, 0x1E, NVS_OP_UNKNOWN, -1, -1, -1);	/* NV30 LIT */
   MOD_OPCODE(NVFP_TX_AOP, 0x1F, NVS_OP_UNKNOWN, -1, -1, -1);	/* NV30 LRP */
   MOD_OPCODE(NVFP_TX_AOP, 0x26, NVS_OP_UNKNOWN, -1, -1, -1);	/* NV30 POW */
   MOD_OPCODE(NVFP_TX_AOP, 0x36, NVS_OP_UNKNOWN, -1, -1, -1);	/* NV30 RFL */

   /* Extra opcodes supported on NV40 */
   MOD_OPCODE(NVFP_TX_AOP, NV40_FP_OP_OPCODE_DIV     , NVS_OP_DIV ,  0,  1, -1);
   MOD_OPCODE(NVFP_TX_AOP, NV40_FP_OP_OPCODE_DP2A    , NVS_OP_DP2A,  0,  1,  2);
   MOD_OPCODE(NVFP_TX_AOP, NV40_FP_OP_OPCODE_TXL     , NVS_OP_TXL ,  0, -1, -1);

   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_BRK , NVS_OP_BRK , -1, -1, -1);
   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_CAL , NVS_OP_CAL , -1, -1, -1);
   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_IF  , NVS_OP_IF  , -1, -1, -1);
   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_LOOP, NVS_OP_LOOP, -1, -1, -1);
   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_REP , NVS_OP_REP , -1, -1, -1);
   MOD_OPCODE(NVFP_TX_BOP, NV40_FP_OP_BRA_OPCODE_RET , NVS_OP_RET , -1, -1, -1);

   shader->SupportsResultScale	= NV40FPSupportsResultScale;
   shader->SetResultScale	= NV40FPSetResultScale;

   /* fragment.facing */
   shader->GetSourceID		= NV40FPGetSourceID;

   /* branching */
   shader->GetOPTXRec		= NV40FPGetOPTXRec;
   shader->GetBranch		= NV40FPGetBranch;
   shader->GetBranchElse	= NV40FPGetBranchElse;
   shader->GetBranchEnd		= NV40FPGetBranchEnd;
   shader->GetLoopCount		= NV40FPGetLoopCount;
   shader->GetLoopInitial	= NV40FPGetLoopInitial;
   shader->GetLoopIncrement	= NV40FPGetLoopIncrement;
   shader->SetBranchTarget	= NV40FPSetBranchTarget;
   shader->SetBranchElse	= NV40FPSetBranchElse;
   shader->SetBranchEnd		= NV40FPSetBranchEnd;
   shader->SetLoopParams	= NV40FPSetLoopParams;
}
