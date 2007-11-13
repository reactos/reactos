#ifndef __NV40_SHADER_H__
#define __NV40_SHADER_H__

/* Vertex programs instruction set
 *
 * The NV40 instruction set is very similar to NV30.  Most fields are in
 * a slightly different position in the instruction however.
 *
 * Merged instructions
 *     In some cases it is possible to put two instructions into one opcode
 *     slot.  The rules for when this is OK is not entirely clear to me yet.
 *
 *     There are separate writemasks and dest temp register fields for each
 *     grouping of instructions.  There is however only one field with the
 *     ID of a result register.  Writing to temp/result regs is selected by
 *     setting VEC_RESULT/SCA_RESULT.
 *
 * Temporary registers
 *     The source/dest temp register fields have been extended by 1 bit, to
 *     give a total of 32 temporary registers.
 *
 * Relative Addressing
 *     NV40 can use an address register to index into vertex attribute regs.
 *     This is done by putting the offset value into INPUT_SRC and setting
 *     the INDEX_INPUT flag.
 *
 * Conditional execution (see NV_vertex_program{2,3} for details)
 *     There is a second condition code register on NV40, it's use is enabled
 *     by setting the COND_REG_SELECT_1 flag.
 *
 * Texture lookup
 *     TODO
 */

/* ---- OPCODE BITS 127:96 / data DWORD 0 --- */
#define NV40_VP_INST_VEC_RESULT                                        (1 << 30)
/* uncertain.. */
#define NV40_VP_INST_COND_UPDATE_ENABLE                        ((1 << 14)|1<<29)
/* use address reg as index into attribs */
#define NV40_VP_INST_INDEX_INPUT                                       (1 << 27)
#define NV40_VP_INST_COND_REG_SELECT_1                                 (1 << 25)
#define NV40_VP_INST_ADDR_REG_SELECT_1                                 (1 << 24)
#define NV40_VP_INST_SRC2_ABS                                          (1 << 23)
#define NV40_VP_INST_SRC1_ABS                                          (1 << 22)
#define NV40_VP_INST_SRC0_ABS                                          (1 << 21)
#define NV40_VP_INST_VEC_DEST_TEMP_SHIFT                                      15
#define NV40_VP_INST_VEC_DEST_TEMP_MASK                             (0x1F << 15)
#define NV40_VP_INST_COND_TEST_ENABLE                                  (1 << 13)
#define NV40_VP_INST_COND_SHIFT                                               10
#define NV40_VP_INST_COND_MASK                                       (0x7 << 10)
#    define NV40_VP_INST_COND_FL                                               0
#    define NV40_VP_INST_COND_LT                                               1
#    define NV40_VP_INST_COND_EQ                                               2
#    define NV40_VP_INST_COND_LE                                               3
#    define NV40_VP_INST_COND_GT                                               4
#    define NV40_VP_INST_COND_NE                                               5
#    define NV40_VP_INST_COND_GE                                               6
#    define NV40_VP_INST_COND_TR                                               7
#define NV40_VP_INST_COND_SWZ_X_SHIFT                                          8
#define NV40_VP_INST_COND_SWZ_X_MASK                                    (3 << 8)
#define NV40_VP_INST_COND_SWZ_Y_SHIFT                                          6
#define NV40_VP_INST_COND_SWZ_Y_MASK                                    (3 << 6)
#define NV40_VP_INST_COND_SWZ_Z_SHIFT                                          4
#define NV40_VP_INST_COND_SWZ_Z_MASK                                    (3 << 4)
#define NV40_VP_INST_COND_SWZ_W_SHIFT                                          2
#define NV40_VP_INST_COND_SWZ_W_MASK                                    (3 << 2)
#define NV40_VP_INST_COND_SWZ_ALL_SHIFT                                        2
#define NV40_VP_INST_COND_SWZ_ALL_MASK                               (0xFF << 2)
#define NV40_VP_INST_ADDR_SWZ_SHIFT                                            0
#define NV40_VP_INST_ADDR_SWZ_MASK                                   (0x03 << 0)
#define NV40_VP_INST0_KNOWN ( \
                NV40_VP_INST_INDEX_INPUT | \
                NV40_VP_INST_COND_REG_SELECT_1 | \
                NV40_VP_INST_ADDR_REG_SELECT_1 | \
                NV40_VP_INST_SRC2_ABS | \
                NV40_VP_INST_SRC1_ABS | \
                NV40_VP_INST_SRC0_ABS | \
                NV40_VP_INST_VEC_DEST_TEMP_MASK | \
                NV40_VP_INST_COND_TEST_ENABLE | \
                NV40_VP_INST_COND_MASK | \
                NV40_VP_INST_COND_SWZ_ALL_MASK | \
                NV40_VP_INST_ADDR_SWZ_MASK)

/* ---- OPCODE BITS 95:64 / data DWORD 1 --- */
#define NV40_VP_INST_VEC_OPCODE_SHIFT                                         22
#define NV40_VP_INST_VEC_OPCODE_MASK                                (0x1F << 22)
#    define NV40_VP_INST_OP_NOP                                             0x00
#    define NV40_VP_INST_OP_MOV                                             0x01
#    define NV40_VP_INST_OP_MUL                                             0x02
#    define NV40_VP_INST_OP_ADD                                             0x03
#    define NV40_VP_INST_OP_MAD                                             0x04
#    define NV40_VP_INST_OP_DP3                                             0x05
#    define NV40_VP_INST_OP_DP4                                             0x07
#    define NV40_VP_INST_OP_DPH                                             0x06
#    define NV40_VP_INST_OP_DST                                             0x08
#    define NV40_VP_INST_OP_MIN                                             0x09
#    define NV40_VP_INST_OP_MAX                                             0x0A
#    define NV40_VP_INST_OP_SLT                                             0x0B
#    define NV40_VP_INST_OP_SGE                                             0x0C
#    define NV40_VP_INST_OP_ARL                                             0x0D
#    define NV40_VP_INST_OP_FRC                                             0x0E
#    define NV40_VP_INST_OP_FLR                                             0x0F
#    define NV40_VP_INST_OP_SEQ                                             0x10
#    define NV40_VP_INST_OP_SFL                                             0x11
#    define NV40_VP_INST_OP_SGT                                             0x12
#    define NV40_VP_INST_OP_SLE                                             0x13
#    define NV40_VP_INST_OP_SNE                                             0x14
#    define NV40_VP_INST_OP_STR                                             0x15
#    define NV40_VP_INST_OP_SSG                                             0x16
#    define NV40_VP_INST_OP_ARR                                             0x17
#    define NV40_VP_INST_OP_ARA                                             0x18
#    define NV40_VP_INST_OP_TXWHAT                                          0x19
#define NV40_VP_INST_SCA_OPCODE_SHIFT                                         27
#define NV40_VP_INST_SCA_OPCODE_MASK                                (0x1F << 27)
#    define NV40_VP_INST_OP_RCP                                             0x02
#    define NV40_VP_INST_OP_RCC                                             0x03
#    define NV40_VP_INST_OP_RSQ                                             0x04
#    define NV40_VP_INST_OP_EXP                                             0x05
#    define NV40_VP_INST_OP_LOG                                             0x06
#    define NV40_VP_INST_OP_LIT                                             0x07
#    define NV40_VP_INST_OP_BRA                                             0x09
#    define NV40_VP_INST_OP_CAL                                             0x0B
#    define NV40_VP_INST_OP_RET                                             0x0C
#    define NV40_VP_INST_OP_LG2                                             0x0D
#    define NV40_VP_INST_OP_EX2                                             0x0E
#    define NV40_VP_INST_OP_SIN                                             0x0F
#    define NV40_VP_INST_OP_COS                                             0x10
#    define NV40_VP_INST_OP_PUSHA                                           0x13
#    define NV40_VP_INST_OP_POPA                                            0x14
#define NV40_VP_INST_CONST_SRC_SHIFT                                          12
#define NV40_VP_INST_CONST_SRC_MASK                                 (0xFF << 12)
#define NV40_VP_INST_INPUT_SRC_SHIFT                                           8
#define NV40_VP_INST_INPUT_SRC_MASK                                  (0x0F << 8)
#    define NV40_VP_INST_IN_POS                                                0
#    define NV40_VP_INST_IN_WEIGHT                                             1
#    define NV40_VP_INST_IN_NORMAL                                             2
#    define NV40_VP_INST_IN_COL0                                               3
#    define NV40_VP_INST_IN_COL1                                               4
#    define NV40_VP_INST_IN_FOGC                                               5
#    define NV40_VP_INST_IN_TC0                                                8
#    define NV40_VP_INST_IN_TC(n)                                          (8+n)
#define NV40_VP_INST_SRC0H_SHIFT                                               0
#define NV40_VP_INST_SRC0H_MASK                                      (0xFF << 0)
#define NV40_VP_INST1_KNOWN ( \
                NV40_VP_INST_VEC_OPCODE_MASK | \
                NV40_VP_INST_SCA_OPCODE_MASK | \
                NV40_VP_INST_CONST_SRC_MASK  | \
                NV40_VP_INST_INPUT_SRC_MASK  | \
                NV40_VP_INST_SRC0H_MASK \
                )

/* ---- OPCODE BITS 63:32 / data DWORD 2 --- */
#define NV40_VP_INST_SRC0L_SHIFT                                              23
#define NV40_VP_INST_SRC0L_MASK                                    (0x1FF << 23)
#define NV40_VP_INST_SRC1_SHIFT                                                6
#define NV40_VP_INST_SRC1_MASK                                    (0x1FFFF << 6)
#define NV40_VP_INST_SRC2H_SHIFT                                               0
#define NV40_VP_INST_SRC2H_MASK                                      (0x3F << 0)
#define NV40_VP_INST_IADDRH_SHIFT                                              0
#define NV40_VP_INST_IADDRH_MASK                                     (0x1F << 0)

/* ---- OPCODE BITS 31:0 / data DWORD 3 --- */
#define NV40_VP_INST_IADDRL_SHIFT                                             29
#define NV40_VP_INST_IADDRL_MASK                                       (7 << 29)
#define NV40_VP_INST_SRC2L_SHIFT                                              21
#define NV40_VP_INST_SRC2L_MASK                                    (0x7FF << 21)
#define NV40_VP_INST_SCA_WRITEMASK_SHIFT                                      17
#define NV40_VP_INST_SCA_WRITEMASK_MASK                              (0xF << 17)
#    define NV40_VP_INST_SCA_WRITEMASK_X                               (1 << 20)
#    define NV40_VP_INST_SCA_WRITEMASK_Y                               (1 << 19)
#    define NV40_VP_INST_SCA_WRITEMASK_Z                               (1 << 18)
#    define NV40_VP_INST_SCA_WRITEMASK_W                               (1 << 17)
#define NV40_VP_INST_VEC_WRITEMASK_SHIFT                                      13
#define NV40_VP_INST_VEC_WRITEMASK_MASK                              (0xF << 13)
#    define NV40_VP_INST_VEC_WRITEMASK_X                               (1 << 16)
#    define NV40_VP_INST_VEC_WRITEMASK_Y                               (1 << 15)
#    define NV40_VP_INST_VEC_WRITEMASK_Z                               (1 << 14)
#    define NV40_VP_INST_VEC_WRITEMASK_W                               (1 << 13)
#define NV40_VP_INST_SCA_RESULT                                        (1 << 12)
#define NV40_VP_INST_SCA_DEST_TEMP_SHIFT                                       7
#define NV40_VP_INST_SCA_DEST_TEMP_MASK                              (0x1F << 7)
#define NV40_VP_INST_DEST_SHIFT                                                2
#define NV40_VP_INST_DEST_MASK                                         (31 << 2)
#    define NV40_VP_INST_DEST_POS                                              0
#    define NV40_VP_INST_DEST_COL0                                             1
#    define NV40_VP_INST_DEST_COL1                                             2
#    define NV40_VP_INST_DEST_BFC0                                             3
#    define NV40_VP_INST_DEST_BFC1                                             4
#    define NV40_VP_INST_DEST_FOGC                                             5
#    define NV40_VP_INST_DEST_PSZ                                              6
#    define NV40_VP_INST_DEST_TC0                                              7
#    define NV40_VP_INST_DEST_TC(n)                                        (7+n)
#    define NV40_VP_INST_DEST_TEMP                                          0x1F
#define NV40_VP_INST_INDEX_CONST                                        (1 << 1)
#define NV40_VP_INST_LAST                                               (1 << 0)
#define NV40_VP_INST3_KNOWN ( \
                NV40_VP_INST_SRC2L_MASK |\
                NV40_VP_INST_SCA_WRITEMASK_MASK |\
                NV40_VP_INST_VEC_WRITEMASK_MASK |\
                NV40_VP_INST_SCA_DEST_TEMP_MASK |\
                NV40_VP_INST_DEST_MASK |\
                NV40_VP_INST_INDEX_CONST)

/* Useful to split the source selection regs into their pieces */
#define NV40_VP_SRC0_HIGH_SHIFT                                                9
#define NV40_VP_SRC0_HIGH_MASK                                        0x0001FE00
#define NV40_VP_SRC0_LOW_MASK                                         0x000001FF
#define NV40_VP_SRC2_HIGH_SHIFT                                               11
#define NV40_VP_SRC2_HIGH_MASK                                        0x0001F800
#define NV40_VP_SRC2_LOW_MASK                                         0x000007FF

/* Source selection - these are the bits you fill NV40_VP_INST_SRCn with */
#define NV40_VP_SRC_NEGATE                                             (1 << 16)
#define NV40_VP_SRC_SWZ_X_SHIFT                                               14
#define NV40_VP_SRC_SWZ_X_MASK                                         (3 << 14)
#define NV40_VP_SRC_SWZ_Y_SHIFT                                               12
#define NV40_VP_SRC_SWZ_Y_MASK                                         (3 << 12)
#define NV40_VP_SRC_SWZ_Z_SHIFT                                               10
#define NV40_VP_SRC_SWZ_Z_MASK                                         (3 << 10)
#define NV40_VP_SRC_SWZ_W_SHIFT                                                8
#define NV40_VP_SRC_SWZ_W_MASK                                          (3 << 8)
#define NV40_VP_SRC_SWZ_ALL_SHIFT                                              8
#define NV40_VP_SRC_SWZ_ALL_MASK                                     (0xFF << 8)
#define NV40_VP_SRC_TEMP_SRC_SHIFT                                             2
#define NV40_VP_SRC_TEMP_SRC_MASK                                    (0x1F << 2)
#define NV40_VP_SRC_REG_TYPE_SHIFT                                             0
#define NV40_VP_SRC_REG_TYPE_MASK                                       (3 << 0)
#    define NV40_VP_SRC_REG_TYPE_UNK0                                          0
#    define NV40_VP_SRC_REG_TYPE_TEMP                                          1
#    define NV40_VP_SRC_REG_TYPE_INPUT                                         2
#    define NV40_VP_SRC_REG_TYPE_CONST                                         3


/*
 * Each fragment program opcode appears to be comprised of 4 32-bit values.
 *
 *         0 - Opcode, output reg/mask, ATTRIB source
 *         1 - Source 0
 *         2 - Source 1
 *         3 - Source 2
 *
 * There appears to be no special difference between result regs and temp regs.
 *                 result.color == R0.xyzw
 *                 result.depth == R1.z
 * When the fragprog contains instructions to write depth,
 * NV30_TCL_PRIMITIVE_3D_UNK1D78=0 otherwise it is set to 1.
 *
 * Constants are inserted directly after the instruction that uses them.
 * 
 * It appears that it's not possible to use two input registers in one
 * instruction as the input sourcing is done in the instruction dword
 * and not the source selection dwords.  As such instructions such as:
 * 
 *                 ADD result.color, fragment.color, fragment.texcoord[0];
 *
 * must be split into two MOV's and then an ADD (nvidia does this) but
 * I'm not sure why it's not just one MOV and then source the second input
 * in the ADD instruction..
 *
 * Negation of the full source is done with NV30_FP_REG_NEGATE, arbitrary
 * negation requires multiplication with a const.
 *
 * Arbitrary swizzling is supported with the exception of SWIZZLE_ZERO and
 * SWIZZLE_ONE.
 *
 * The temp/result regs appear to be initialised to (0.0, 0.0, 0.0, 0.0) as
 * SWIZZLE_ZERO is implemented simply by not writing to the relevant components
 * of the destination.
 *
 * Looping
 *   Loops appear to be fairly expensive on NV40 at least, the proprietary
 *   driver goes to a lot of effort to avoid using the native looping
 *   instructions.  If the total number of *executed* instructions between
 *   REP/ENDREP or LOOP/ENDLOOP is <=500, the driver will unroll the loop.
 *   The maximum loop count is 255.
 *
 * Conditional execution
 *   TODO
 * 
 * Non-native instructions:
 *         LIT
 *         LRP - MAD+MAD
 *         SUB - ADD, negate second source
 *         RSQ - LG2 + EX2
 *         POW - LG2 + MUL + EX2
 *         SCS - COS + SIN
 *         XPD
 *         DP2 - MUL + ADD
 *         NRM
 */

//== Opcode / Destination selection ==
#define NV40_FP_OP_PROGRAM_END                                          (1 << 0)
#define NV40_FP_OP_OUT_REG_SHIFT                                               1
#define NV40_FP_OP_OUT_REG_MASK                                        (31 << 1)
/* Needs to be set when writing outputs to get expected result.. */
#define NV40_FP_OP_UNK0_7                                               (1 << 7)
#define NV40_FP_OP_COND_WRITE_ENABLE                                    (1 << 8)
#define NV40_FP_OP_OUTMASK_SHIFT                                               9
#define NV40_FP_OP_OUTMASK_MASK                                       (0xF << 9)
#    define NV40_FP_OP_OUT_X                                            (1 << 9)
#    define NV40_FP_OP_OUT_Y                                            (1 <<10)
#    define NV40_FP_OP_OUT_Z                                            (1 <<11)
#    define NV40_FP_OP_OUT_W                                            (1 <<12)
/* Uncertain about these, especially the input_src values.. it's possible that
 * they can be dynamically changed.
 */
#define NV40_FP_OP_INPUT_SRC_SHIFT                                            13
#define NV40_FP_OP_INPUT_SRC_MASK                                     (15 << 13)
#    define NV40_FP_OP_INPUT_SRC_POSITION                                    0x0
#    define NV40_FP_OP_INPUT_SRC_COL0                                        0x1
#    define NV40_FP_OP_INPUT_SRC_COL1                                        0x2
#    define NV40_FP_OP_INPUT_SRC_FOGC                                        0x3
#    define NV40_FP_OP_INPUT_SRC_TC0                                         0x4
#    define NV40_FP_OP_INPUT_SRC_TC(n)                                 (0x4 + n)
#    define NV40_FP_OP_INPUT_SRC_FACING                                      0xE
#define NV40_FP_OP_TEX_UNIT_SHIFT                                             17
#define NV40_FP_OP_TEX_UNIT_MASK                                     (0xF << 17)
#define NV40_FP_OP_PRECISION_SHIFT                                            22
#define NV40_FP_OP_PRECISION_MASK                                      (3 << 22)
#   define NV40_FP_PRECISION_FP32                                              0
#   define NV40_FP_PRECISION_FP16                                              1
#   define NV40_FP_PRECISION_FX12                                              2
#define NV40_FP_OP_OPCODE_SHIFT                                               24
#define NV40_FP_OP_OPCODE_MASK                                      (0x3F << 24)
#        define NV40_FP_OP_OPCODE_NOP                                       0x00
#        define NV40_FP_OP_OPCODE_MOV                                       0x01
#        define NV40_FP_OP_OPCODE_MUL                                       0x02
#        define NV40_FP_OP_OPCODE_ADD                                       0x03
#        define NV40_FP_OP_OPCODE_MAD                                       0x04
#        define NV40_FP_OP_OPCODE_DP3                                       0x05
#        define NV40_FP_OP_OPCODE_DP4                                       0x06
#        define NV40_FP_OP_OPCODE_DST                                       0x07
#        define NV40_FP_OP_OPCODE_MIN                                       0x08
#        define NV40_FP_OP_OPCODE_MAX                                       0x09
#        define NV40_FP_OP_OPCODE_SLT                                       0x0A
#        define NV40_FP_OP_OPCODE_SGE                                       0x0B
#        define NV40_FP_OP_OPCODE_SLE                                       0x0C
#        define NV40_FP_OP_OPCODE_SGT                                       0x0D
#        define NV40_FP_OP_OPCODE_SNE                                       0x0E
#        define NV40_FP_OP_OPCODE_SEQ                                       0x0F
#        define NV40_FP_OP_OPCODE_FRC                                       0x10
#        define NV40_FP_OP_OPCODE_FLR                                       0x11
#        define NV40_FP_OP_OPCODE_KIL                                       0x12
#        define NV40_FP_OP_OPCODE_PK4B                                      0x13
#        define NV40_FP_OP_OPCODE_UP4B                                      0x14
/* DDX/DDY can only write to XY */
#        define NV40_FP_OP_OPCODE_DDX                                       0x15
#        define NV40_FP_OP_OPCODE_DDY                                       0x16
#        define NV40_FP_OP_OPCODE_TEX                                       0x17
#        define NV40_FP_OP_OPCODE_TXP                                       0x18
#        define NV40_FP_OP_OPCODE_TXD                                       0x19
#        define NV40_FP_OP_OPCODE_RCP                                       0x1A
#        define NV40_FP_OP_OPCODE_EX2                                       0x1C
#        define NV40_FP_OP_OPCODE_LG2                                       0x1D
#        define NV40_FP_OP_OPCODE_COS                                       0x22
#        define NV40_FP_OP_OPCODE_SIN                                       0x23
#        define NV40_FP_OP_OPCODE_PK2H                                      0x24
#        define NV40_FP_OP_OPCODE_UP2H                                      0x25
#        define NV40_FP_OP_OPCODE_PK4UB                                     0x27
#        define NV40_FP_OP_OPCODE_UP4UB                                     0x28
#        define NV40_FP_OP_OPCODE_PK2US                                     0x29
#        define NV40_FP_OP_OPCODE_UP2US                                     0x2A
#        define NV40_FP_OP_OPCODE_DP2A                                      0x2E
#        define NV40_FP_OP_OPCODE_TXL                                       0x2F
#        define NV40_FP_OP_OPCODE_TXB                                       0x31
#        define NV40_FP_OP_OPCODE_DIV                                       0x3A
/* The use of these instructions appears to be indicated by bit 31 of DWORD 2.*/
#        define NV40_FP_OP_BRA_OPCODE_BRK                                    0x0
#        define NV40_FP_OP_BRA_OPCODE_CAL                                    0x1
#        define NV40_FP_OP_BRA_OPCODE_IF                                     0x2
#        define NV40_FP_OP_BRA_OPCODE_LOOP                                   0x3
#        define NV40_FP_OP_BRA_OPCODE_REP                                    0x4
#        define NV40_FP_OP_BRA_OPCODE_RET                                    0x5
#define NV40_FP_OP_OUT_SAT                                             (1 << 31)

/* high order bits of SRC0 */
#define NV40_FP_OP_OUT_ABS                                             (1 << 29)
#define NV40_FP_OP_COND_SWZ_W_SHIFT                                           27
#define NV40_FP_OP_COND_SWZ_W_MASK                                     (3 << 27)
#define NV40_FP_OP_COND_SWZ_Z_SHIFT                                           25
#define NV40_FP_OP_COND_SWZ_Z_MASK                                     (3 << 25)
#define NV40_FP_OP_COND_SWZ_Y_SHIFT                                           23
#define NV40_FP_OP_COND_SWZ_Y_MASK                                     (3 << 23)
#define NV40_FP_OP_COND_SWZ_X_SHIFT                                           21
#define NV40_FP_OP_COND_SWZ_X_MASK                                     (3 << 21)
#define NV40_FP_OP_COND_SWZ_ALL_SHIFT                                         21
#define NV40_FP_OP_COND_SWZ_ALL_MASK                                (0xFF << 21)
#define NV40_FP_OP_COND_SHIFT                                                 18
#define NV40_FP_OP_COND_MASK                                        (0x07 << 18)
#        define NV40_FP_OP_COND_FL                                             0
#        define NV40_FP_OP_COND_LT                                             1
#        define NV40_FP_OP_COND_EQ                                             2
#        define NV40_FP_OP_COND_LE                                             3
#        define NV40_FP_OP_COND_GT                                             4
#        define NV40_FP_OP_COND_NE                                             5
#        define NV40_FP_OP_COND_GE                                             6
#        define NV40_FP_OP_COND_TR                                             7

/* high order bits of SRC1 */
#define NV40_FP_OP_OPCODE_IS_BRANCH                                      (1<<31)
#define NV40_FP_OP_DST_SCALE_SHIFT                                            28
#define NV40_FP_OP_DST_SCALE_MASK                                      (3 << 28)

/* SRC1 LOOP */
#define NV40_FP_OP_LOOP_INCR_SHIFT                                            19
#define NV40_FP_OP_LOOP_INCR_MASK                                   (0xFF << 19)
#define NV40_FP_OP_LOOP_INDEX_SHIFT                                           10
#define NV40_FP_OP_LOOP_INDEX_MASK                                  (0xFF << 10)
#define NV40_FP_OP_LOOP_COUNT_SHIFT                                            2
#define NV40_FP_OP_LOOP_COUNT_MASK                                   (0xFF << 2)

/* SRC1 IF */
#define NV40_FP_OP_ELSE_ID_SHIFT                                               2
#define NV40_FP_OP_ELSE_ID_MASK                                      (0xFF << 2)

/* SRC1 CAL */
#define NV40_FP_OP_IADDR_SHIFT                                                 2
#define NV40_FP_OP_IADDR_MASK                                        (0xFF << 2)

/* SRC1 REP
 *   I have no idea why there are 3 count values here..  but they
 *   have always been filled with the same value in my tests so
 *   far..
 */
#define NV40_FP_OP_REP_COUNT1_SHIFT                                            2
#define NV40_FP_OP_REP_COUNT1_MASK                                   (0xFF << 2)
#define NV40_FP_OP_REP_COUNT2_SHIFT                                           10
#define NV40_FP_OP_REP_COUNT2_MASK                                  (0xFF << 10)
#define NV40_FP_OP_REP_COUNT3_SHIFT                                           19
#define NV40_FP_OP_REP_COUNT3_MASK                                  (0xFF << 19)

/* SRC2 REP/IF */
#define NV40_FP_OP_END_ID_SHIFT                                                2
#define NV40_FP_OP_END_ID_MASK                                       (0xFF << 2)

// SRC2 high-order
#define NV40_FP_OP_INDEX_INPUT                                         (1 << 30)
#define NV40_FP_OP_ADDR_INDEX_SHIFT                                           19
#define NV40_FP_OP_ADDR_INDEX_MASK                                   (0xF << 19)

//== Register selection ==
#define NV40_FP_REG_TYPE_SHIFT                                                 0
#define NV40_FP_REG_TYPE_MASK                                           (3 << 0)
#        define NV40_FP_REG_TYPE_TEMP                                          0
#        define NV40_FP_REG_TYPE_INPUT                                         1
#        define NV40_FP_REG_TYPE_CONST                                         2
#define NV40_FP_REG_SRC_SHIFT                                                  2
#define NV40_FP_REG_SRC_MASK                                           (31 << 2)
#define NV40_FP_REG_UNK_0                                               (1 << 8)
#define NV40_FP_REG_SWZ_ALL_SHIFT                                              9
#define NV40_FP_REG_SWZ_ALL_MASK                                      (255 << 9)
#define NV40_FP_REG_SWZ_X_SHIFT                                                9
#define NV40_FP_REG_SWZ_X_MASK                                          (3 << 9)
#define NV40_FP_REG_SWZ_Y_SHIFT                                               11
#define NV40_FP_REG_SWZ_Y_MASK                                         (3 << 11)
#define NV40_FP_REG_SWZ_Z_SHIFT                                               13
#define NV40_FP_REG_SWZ_Z_MASK                                         (3 << 13)
#define NV40_FP_REG_SWZ_W_SHIFT                                               15
#define NV40_FP_REG_SWZ_W_MASK                                         (3 << 15)
#        define NV40_FP_SWIZZLE_X                                              0
#        define NV40_FP_SWIZZLE_Y                                              1
#        define NV40_FP_SWIZZLE_Z                                              2
#        define NV40_FP_SWIZZLE_W                                              3
#define NV40_FP_REG_NEGATE                                             (1 << 17)

#endif
