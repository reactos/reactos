#ifndef __NV30_SHADER_H__
#define __NV30_SHADER_H__

/* Vertex programs instruction set
 *
 * 128bit opcodes, split into 4 32-bit ones for ease of use.
 *
 * Non-native instructions
 *	 ABS - MOV + NV40_VP_INST0_DEST_ABS
 *	 POW - EX2 + MUL + LG2
 *	 SUB - ADD, second source negated
 *	 SWZ - MOV
 *	 XPD -  
 *
 * Register access
 *	 - Only one INPUT can be accessed per-instruction (move extras into TEMPs)
 *	 - Only one CONST can be accessed per-instruction (move extras into TEMPs)
 *
 * Relative Addressing
 *	 According to the value returned for MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB
 *	 there are only two address registers available.  The destination in the ARL
 *	 instruction is set to TEMP <n> (The temp isn't actually written).
 *
 *	 When using vanilla ARB_v_p, the proprietary driver will squish both the available
 *	 ADDRESS regs into the first hardware reg in the X and Y components.
 *
 *	 To use an address reg as an index into consts, the CONST_SRC is set to
 *	 (const_base + offset) and INDEX_CONST is set.
 *
 *	 To access the second address reg use ADDR_REG_SELECT_1. A particular component
 *	 of the address regs is selected with ADDR_SWZ.
 *
 *	 Only one address register can be accessed per instruction.
 *
 * Conditional execution (see NV_vertex_program{2,3} for details)
 *	 Conditional execution of an instruction is enabled by setting COND_TEST_ENABLE, and
 *	 selecting the condition which will allow the test to pass with COND_{FL,LT,...}.
 *	 It is possible to swizzle the values in the condition register, which allows for
 *	 testing against an individual component.
 *
 * Branching
 *	 The BRA/CAL instructions seem to follow a slightly different opcode layout.  The
 *	 destination instruction ID (IADDR) overlaps a source field.  Instruction ID's seem to
 *	 be numbered based on the UPLOAD_FROM_ID FIFO command, and is incremented automatically
 *	 on each UPLOAD_INST FIFO command.
 *
 *	 Conditional branching is achieved by using the condition tests described above.
 *	 There doesn't appear to be dedicated looping instructions, but this can be done
 *	 using a temp reg + conditional branching.
 *
 *	 Subroutines may be uploaded before the main program itself, but the first executed
 *	 instruction is determined by the PROGRAM_START_ID FIFO command.
 *
 */

/* DWORD 0 */
#define NV30_VP_INST_ADDR_REG_SELECT_1				(1 << 24)
#define NV30_VP_INST_SRC2_ABS			 		(1 << 23) /* guess */
#define NV30_VP_INST_SRC1_ABS			 		(1 << 22) /* guess */
#define NV30_VP_INST_SRC0_ABS			 		(1 << 21) /* guess */
#define NV30_VP_INST_OUT_RESULT					(1 << 20)
#define NV30_VP_INST_DEST_TEMP_ID_SHIFT				16
#define NV30_VP_INST_DEST_TEMP_ID_MASK				(0x0F << 16)
#define NV30_VP_INST_COND_UPDATE_ENABLE				(1<<15)
#define NV30_VP_INST_COND_TEST_ENABLE				(1<<14)
#define NV30_VP_INST_COND_SHIFT					11
#define NV30_VP_INST_COND_MASK					(0x07 << 11)
#	define NV30_VP_INST_COND_FL	0 /* guess */	
#	define NV30_VP_INST_COND_LT	1	
#	define NV30_VP_INST_COND_EQ	2
#	define NV30_VP_INST_COND_LE	3
#	define NV30_VP_INST_COND_GT	4
#	define NV30_VP_INST_COND_NE	5
#	define NV30_VP_INST_COND_GE	6
#	define NV30_VP_INST_COND_TR	7 /* guess */
#define NV30_VP_INST_COND_SWZ_X_SHIFT				9
#define NV30_VP_INST_COND_SWZ_X_MASK				(0x03 <<  9)
#define NV30_VP_INST_COND_SWZ_Y_SHIFT				7
#define NV30_VP_INST_COND_SWZ_Y_MASK				(0x03 <<  7)
#define NV30_VP_INST_COND_SWZ_Z_SHIFT				5
#define NV30_VP_INST_COND_SWZ_Z_MASK				(0x03 <<  5)
#define NV30_VP_INST_COND_SWZ_W_SHIFT				3
#define NV30_VP_INST_COND_SWZ_W_MASK				(0x03 <<  3)
#define NV30_VP_INST_COND_SWZ_ALL_SHIFT				3
#define NV30_VP_INST_COND_SWZ_ALL_MASK				(0xFF <<  3)
#define NV30_VP_INST_ADDR_SWZ_SHIFT				1
#define NV30_VP_INST_ADDR_SWZ_MASK				(0x03 <<  1)
#define NV30_VP_INST_SCA_OPCODEH_SHIFT				0
#define NV30_VP_INST_SCA_OPCODEH_MASK				(0x01 <<  0)

/* DWORD 1 */
#define NV30_VP_INST_SCA_OPCODEL_SHIFT				28
#define NV30_VP_INST_SCA_OPCODEL_MASK				(0x0F << 28)
#	define NV30_VP_INST_OP_NOP	0x00
#	define NV30_VP_INST_OP_RCP	0x02
#	define NV30_VP_INST_OP_RCC	0x03
#	define NV30_VP_INST_OP_RSQ	0x04
#	define NV30_VP_INST_OP_EXP	0x05
#	define NV30_VP_INST_OP_LOG	0x06
#	define NV30_VP_INST_OP_LIT	0x07
#	define NV30_VP_INST_OP_BRA	0x09
#	define NV30_VP_INST_OP_CAL	0x0B
#	define NV30_VP_INST_OP_RET	0x0C
#	define NV30_VP_INST_OP_LG2	0x0D
#	define NV30_VP_INST_OP_EX2	0x0E
#	define NV30_VP_INST_OP_SIN	0x0F
#	define NV30_VP_INST_OP_COS	0x10
#define NV30_VP_INST_VEC_OPCODE_SHIFT				23
#define NV30_VP_INST_VEC_OPCODE_MASK				(0x1F << 23)
#	define NV30_VP_INST_OP_NOPV	0x00
#	define NV30_VP_INST_OP_MOV	0x01
#	define NV30_VP_INST_OP_MUL	0x02
#	define NV30_VP_INST_OP_ADD	0x03
#	define NV30_VP_INST_OP_MAD	0x04
#	define NV30_VP_INST_OP_DP3	0x05
#	define NV30_VP_INST_OP_DP4	0x07
#	define NV30_VP_INST_OP_DPH	0x06
#	define NV30_VP_INST_OP_DST	0x08
#	define NV30_VP_INST_OP_MIN	0x09
#	define NV30_VP_INST_OP_MAX	0x0A
#	define NV30_VP_INST_OP_SLT	0x0B
#	define NV30_VP_INST_OP_SGE	0x0C
#	define NV30_VP_INST_OP_ARL	0x0D
#	define NV30_VP_INST_OP_FRC	0x0E
#	define NV30_VP_INST_OP_FLR	0x0F
#	define NV30_VP_INST_OP_SEQ	0x10
#	define NV30_VP_INST_OP_SFL	0x11
#	define NV30_VP_INST_OP_SGT	0x12
#	define NV30_VP_INST_OP_SLE	0x13
#	define NV30_VP_INST_OP_SNE	0x14
#	define NV30_VP_INST_OP_STR	0x15
#	define NV30_VP_INST_OP_SSG	0x16
#	define NV30_VP_INST_OP_ARR	0x17
#	define NV30_VP_INST_OP_ARA	0x18
#define NV30_VP_INST_CONST_SRC_SHIFT				14
#define NV30_VP_INST_CONST_SRC_MASK				(0xFF << 14)
#define NV30_VP_INST_INPUT_SRC_SHIFT				9		/*NV20*/
#define NV30_VP_INST_INPUT_SRC_MASK				(0x0F <<  9)	/*NV20*/
#	define NV30_VP_INST_IN_POS	0	  /* These seem to match the bindings specified in */
#	define NV30_VP_INST_IN_WEIGHT	1	  /* the ARB_v_p spec (2.14.3.1) */
#	define NV30_VP_INST_IN_NORMAL	2	  
#	define NV30_VP_INST_IN_COL0	3	  /* Should probably confirm them all though */
#	define NV30_VP_INST_IN_COL1	4
#	define NV30_VP_INST_IN_FOGC	5
#	define NV30_VP_INST_IN_TC0	8
#	define NV30_VP_INST_IN_TC(n)	(8+n)
#define NV30_VP_INST_SRC0H_SHIFT				0		/*NV20*/
#define NV30_VP_INST_SRC0H_MASK					(0x1FF << 0)	/*NV20*/

/* DWORD 2 */
#define NV30_VP_INST_SRC0L_SHIFT				26		/*NV20*/
#define NV30_VP_INST_SRC0L_MASK					(0x3F  <<26)	/*NV20*/
#define NV30_VP_INST_SRC1_SHIFT					11		/*NV20*/
#define NV30_VP_INST_SRC1_MASK					(0x7FFF<<11)	/*NV20*/
#define NV30_VP_INST_SRC2H_SHIFT				0		/*NV20*/
#define NV30_VP_INST_SRC2H_MASK					(0x7FF << 0)	/*NV20*/
#define NV30_VP_INST_IADDR_SHIFT				2
#define NV30_VP_INST_IADDR_MASK					(0xFF <<  2) 	/* guess */

/* DWORD 3 */
#define NV30_VP_INST_SRC2L_SHIFT				28		/*NV20*/
#define NV30_VP_INST_SRC2L_MASK					(0x0F  <<28)	/*NV20*/
#define NV30_VP_INST_STEMP_WRITEMASK_SHIFT			24
#define NV30_VP_INST_STEMP_WRITEMASK_MASK			(0x0F << 24)
#define NV30_VP_INST_VTEMP_WRITEMASK_SHIFT			20
#define NV30_VP_INST_VTEMP_WRITEMASK_MASK			(0x0F << 20)
#define NV30_VP_INST_SDEST_WRITEMASK_SHIFT			16
#define NV30_VP_INST_SDEST_WRITEMASK_MASK			(0x0F << 16)
#define NV30_VP_INST_VDEST_WRITEMASK_SHIFT			12		/*NV20*/
#define NV30_VP_INST_VDEST_WRITEMASK_MASK			(0x0F << 12)	/*NV20*/
#define NV30_VP_INST_DEST_ID_SHIFT				2
#define NV30_VP_INST_DEST_ID_MASK				(0x0F <<  2)
#	define NV30_VP_INST_DEST_POS	0
#	define NV30_VP_INST_DEST_COL0	3
#	define NV30_VP_INST_DEST_COL1	4
#	define NV30_VP_INST_DEST_TC(n)	(8+n)

/* Source-register definition - matches NV20 exactly */
#define NV30_VP_SRC_REG_NEGATE					(1<<14)
#define NV30_VP_SRC_REG_SWZ_X_SHIFT				12
#define NV30_VP_SRC_REG_SWZ_X_MASK				(0x03  <<12)
#define NV30_VP_SRC_REG_SWZ_Y_SHIFT				10
#define NV30_VP_SRC_REG_SWZ_Y_MASK				(0x03  <<10)
#define NV30_VP_SRC_REG_SWZ_Z_SHIFT				8
#define NV30_VP_SRC_REG_SWZ_Z_MASK				(0x03  << 8)
#define NV30_VP_SRC_REG_SWZ_W_SHIFT				6
#define NV30_VP_SRC_REG_SWZ_W_MASK				(0x03  << 6)
#define NV30_VP_SRC_REG_SWZ_ALL_SHIFT				6
#define NV30_VP_SRC_REG_SWZ_ALL_MASK				(0xFF  << 6)
#define NV30_VP_SRC_REG_TEMP_ID_SHIFT				2
#define NV30_VP_SRC_REG_TEMP_ID_MASK				(0x0F  << 0)
#define NV30_VP_SRC_REG_TYPE_SHIFT				0
#define NV30_VP_SRC_REG_TYPE_MASK				(0x03  << 0)
#define NV30_VP_SRC_REG_TYPE_TEMP	1
#define NV30_VP_SRC_REG_TYPE_INPUT	2
#define NV30_VP_SRC_REG_TYPE_CONST	3 /* guess */

/*
 * Each fragment program opcode appears to be comprised of 4 32-bit values.
 *
 *	 0 - Opcode, output reg/mask, ATTRIB source
 *	 1 - Source 0
 *	 2 - Source 1
 *	 3 - Source 2
 *
 * There appears to be no special difference between result regs and temp regs.
 * 		result.color == R0.xyzw
 * 		result.depth == R1.z
 * When the fragprog contains instructions to write depth, NV30_TCL_PRIMITIVE_3D_UNK1D78=0
 * otherwise it is set to 1.
 *
 * Constants are inserted directly after the instruction that uses them.
 * 
 * It appears that it's not possible to use two input registers in one
 * instruction as the input sourcing is done in the instruction dword
 * and not the source selection dwords.  As such instructions such as:
 * 
 *		 ADD result.color, fragment.color, fragment.texcoord[0];
 *
 * must be split into two MOV's and then an ADD (nvidia does this) but
 * I'm not sure why it's not just one MOV and then source the second input
 * in the ADD instruction..
 *
 * Negation of the full source is done with NV30_FP_REG_NEGATE, arbitrary
 * negation requires multiplication with a const.
 *
 * Arbitrary swizzling is supported with the exception of SWIZZLE_ZERO/SWIZZLE_ONE
 * The temp/result regs appear to be initialised to (0.0, 0.0, 0.0, 0.0) as SWIZZLE_ZERO
 * is implemented simply by not writing to the relevant components of the destination.
 *
 * Conditional execution
 *   TODO
 * 
 * Non-native instructions:
 *	 LIT
 *	 LRP - MAD+MAD
 *	 SUB - ADD, negate second source
 *	 RSQ - LG2 + EX2
 *	 POW - LG2 + MUL + EX2
 *	 SCS - COS + SIN
 *	 XPD
 */

//== Opcode / Destination selection ==
#define NV30_FP_OP_PROGRAM_END					(1 << 0)
#define NV30_FP_OP_OUT_REG_SHIFT				1
#define NV30_FP_OP_OUT_REG_MASK					(31 << 1)	/* uncertain */
/* Needs to be set when writing outputs to get expected result.. */
#define NV30_FP_OP_UNK0_7					(1 << 7)
#define NV30_FP_OP_COND_WRITE_ENABLE				(1 << 8)
#define NV30_FP_OP_OUTMASK_SHIFT				9
#define NV30_FP_OP_OUTMASK_MASK					(0xF << 9)
#	define NV30_FP_OP_OUT_X	(1<<9)
#	define NV30_FP_OP_OUT_Y	(1<<10)
#	define NV30_FP_OP_OUT_Z	(1<<11)
#	define NV30_FP_OP_OUT_W	(1<<12)
/* Uncertain about these, especially the input_src values.. it's possible that
 * they can be dynamically changed.
 */
#define NV30_FP_OP_INPUT_SRC_SHIFT				13
#define NV30_FP_OP_INPUT_SRC_MASK				(15 << 13)
#	define NV30_FP_OP_INPUT_SRC_POSITION	0x0
#	define NV30_FP_OP_INPUT_SRC_COL0	0x1
#	define NV30_FP_OP_INPUT_SRC_COL1	0x2
#	define NV30_FP_OP_INPUT_SRC_FOGC	0x3
#	define NV30_FP_OP_INPUT_SRC_TC0		0x4
#	define NV30_FP_OP_INPUT_SRC_TC(n)	(0x4 + n)
#define NV30_FP_OP_TEX_UNIT_SHIFT				17
#define NV30_FP_OP_TEX_UNIT_MASK				(0xF << 17) /* guess */
#define NV30_FP_OP_PRECISION_SHIFT				22
#define NV30_FP_OP_PRECISION_MASK				(3 << 22)
#   define NV30_FP_PRECISION_FP32	0
#   define NV30_FP_PRECISION_FP16	1
#   define NV30_FP_PRECISION_FX12	2
#define NV30_FP_OP_OPCODE_SHIFT					24
#define NV30_FP_OP_OPCODE_MASK					(0x3F << 24)
#	define NV30_FP_OP_OPCODE_NOP	0x00
#	define NV30_FP_OP_OPCODE_MOV	0x01
#	define NV30_FP_OP_OPCODE_MUL	0x02
#	define NV30_FP_OP_OPCODE_ADD	0x03
#	define NV30_FP_OP_OPCODE_MAD	0x04
#	define NV30_FP_OP_OPCODE_DP3	0x05
#	define NV30_FP_OP_OPCODE_DP4	0x06
#	define NV30_FP_OP_OPCODE_DST	0x07
#	define NV30_FP_OP_OPCODE_MIN	0x08
#	define NV30_FP_OP_OPCODE_MAX	0x09
#	define NV30_FP_OP_OPCODE_SLT	0x0A
#	define NV30_FP_OP_OPCODE_SGE	0x0B
#	define NV30_FP_OP_OPCODE_SLE	0x0C
#	define NV30_FP_OP_OPCODE_SGT	0x0D
#	define NV30_FP_OP_OPCODE_SNE	0x0E
#	define NV30_FP_OP_OPCODE_SEQ	0x0F
#	define NV30_FP_OP_OPCODE_FRC	0x10
#	define NV30_FP_OP_OPCODE_FLR	0x11
#	define NV30_FP_OP_OPCODE_KIL	0x12
#	define NV30_FP_OP_OPCODE_PK4B   0x13
#	define NV30_FP_OP_OPCODE_UP4B   0x14
#	define NV30_FP_OP_OPCODE_DDX	0x15 /* can only write XY */
#	define NV30_FP_OP_OPCODE_DDY	0x16 /* can only write XY */
#	define NV30_FP_OP_OPCODE_TEX	0x17
#	define NV30_FP_OP_OPCODE_TXP	0x18
#	define NV30_FP_OP_OPCODE_TXD	0x19
#	define NV30_FP_OP_OPCODE_RCP	0x1A
#	define NV30_FP_OP_OPCODE_RSQ	0x1B
#	define NV30_FP_OP_OPCODE_EX2	0x1C
#	define NV30_FP_OP_OPCODE_LG2	0x1D
#	define NV30_FP_OP_OPCODE_LIT	0x1E
#	define NV30_FP_OP_OPCODE_LRP	0x1F
#	define NV30_FP_OP_OPCODE_COS	0x22
#	define NV30_FP_OP_OPCODE_SIN	0x23
#	define NV30_FP_OP_OPCODE_PK2H   0x24
#	define NV30_FP_OP_OPCODE_UP2H   0x25
#	define NV30_FP_OP_OPCODE_POW	0x26
#	define NV30_FP_OP_OPCODE_PK4UB  0x27
#	define NV30_FP_OP_OPCODE_UP4UB  0x28
#	define NV30_FP_OP_OPCODE_PK2US  0x29
#	define NV30_FP_OP_OPCODE_UP2US  0x2A
#	define NV30_FP_OP_OPCODE_DP2A   0x2E
#	define NV30_FP_OP_OPCODE_TXB	0x31
#	define NV30_FP_OP_OPCODE_RFL	0x36
#define NV30_FP_OP_OUT_SAT					(1 << 31)

/* high order bits of SRC0 */
#define NV30_FP_OP_OUT_ABS					(1 << 29)
#define NV30_FP_OP_COND_SWZ_W_SHIFT				27
#define NV30_FP_OP_COND_SWZ_W_MASK				(3 << 27)
#define NV30_FP_OP_COND_SWZ_Z_SHIFT				25
#define NV30_FP_OP_COND_SWZ_Z_MASK				(3 << 25)
#define NV30_FP_OP_COND_SWZ_Y_SHIFT				23
#define NV30_FP_OP_COND_SWZ_Y_MASK				(3 << 23)
#define NV30_FP_OP_COND_SWZ_X_SHIFT				21
#define NV30_FP_OP_COND_SWZ_X_MASK				(3 << 21)
#define NV30_FP_OP_COND_SWZ_ALL_SHIFT				21
#define NV30_FP_OP_COND_SWZ_ALL_MASK				(0xFF << 21)
#define NV30_FP_OP_COND_SHIFT					18
#define NV30_FP_OP_COND_MASK					(0x07 << 18)
#	define NV30_FP_OP_COND_FL	0
#	define NV30_FP_OP_COND_LT	1
#	define NV30_FP_OP_COND_EQ	2
#	define NV30_FP_OP_COND_LE	3
#	define NV30_FP_OP_COND_GT	4
#	define NV30_FP_OP_COND_NE	5
#	define NV30_FP_OP_COND_GE	6
#	define NV30_FP_OP_COND_TR	7

/* high order bits of SRC1 */
#define NV30_FP_OP_SRC_SCALE_SHIFT				28
#define NV30_FP_OP_SRC_SCALE_MASK				(3 << 28)

/* high order bits of SRC2 */
#define NV30_FP_OP_INDEX_INPUT					(1 << 30)

//== Register selection ==
#define NV30_FP_REG_ALL_MASK					(0x1FFFF<<0)
#define NV30_FP_REG_TYPE_SHIFT					0
#define NV30_FP_REG_TYPE_MASK					(3 << 0)
#	define NV30_FP_REG_TYPE_TEMP	0
#	define NV30_FP_REG_TYPE_INPUT	1
#	define NV30_FP_REG_TYPE_CONST	2
#define NV30_FP_REG_SRC_SHIFT					2 /* uncertain */
#define NV30_FP_REG_SRC_MASK					(31 << 2)
#define NV30_FP_REG_UNK_0					(1 << 8)
#define NV30_FP_REG_SWZ_ALL_SHIFT				9
#define NV30_FP_REG_SWZ_ALL_MASK				(255 << 9)
#define NV30_FP_REG_SWZ_X_SHIFT					9
#define NV30_FP_REG_SWZ_X_MASK					(3 << 9)
#define NV30_FP_REG_SWZ_Y_SHIFT					11
#define NV30_FP_REG_SWZ_Y_MASK					(3 << 11)
#define NV30_FP_REG_SWZ_Z_SHIFT					13
#define NV30_FP_REG_SWZ_Z_MASK					(3 << 13)
#define NV30_FP_REG_SWZ_W_SHIFT					15
#define NV30_FP_REG_SWZ_W_MASK					(3 << 15)
#	define NV30_FP_SWIZZLE_X	0
#	define NV30_FP_SWIZZLE_Y	1
#	define NV30_FP_SWIZZLE_Z	2
#	define NV30_FP_SWIZZLE_W	3
#define NV30_FP_REG_NEGATE					(1 << 17)

#endif
