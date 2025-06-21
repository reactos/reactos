/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wined3d_private.h"
#define LIBVKD3D_SHADER_SOURCE
#include <vkd3d_shader.h>

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);
WINE_DECLARE_DEBUG_CHANNEL(d3d_bytecode);

#define WINED3D_SM4_INSTRUCTION_MODIFIER        (0x1u << 31)

#define WINED3D_SM4_MODIFIER_MASK               0x3fu

#define WINED3D_SM5_MODIFIER_DATA_TYPE_SHIFT    6
#define WINED3D_SM5_MODIFIER_DATA_TYPE_MASK     (0xffffu << WINED3D_SM5_MODIFIER_DATA_TYPE_SHIFT)

#define WINED3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT 6
#define WINED3D_SM5_MODIFIER_RESOURCE_TYPE_MASK (0xfu << WINED3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT)

#define WINED3D_SM4_AOFFIMMI_U_SHIFT            9
#define WINED3D_SM4_AOFFIMMI_U_MASK             (0xfu << WINED3D_SM4_AOFFIMMI_U_SHIFT)
#define WINED3D_SM4_AOFFIMMI_V_SHIFT            13
#define WINED3D_SM4_AOFFIMMI_V_MASK             (0xfu << WINED3D_SM4_AOFFIMMI_V_SHIFT)
#define WINED3D_SM4_AOFFIMMI_W_SHIFT            17
#define WINED3D_SM4_AOFFIMMI_W_MASK             (0xfu << WINED3D_SM4_AOFFIMMI_W_SHIFT)

#define WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT    24
#define WINED3D_SM4_INSTRUCTION_LENGTH_MASK     (0x1fu << WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT)

#define WINED3D_SM4_INSTRUCTION_FLAGS_SHIFT     11
#define WINED3D_SM4_INSTRUCTION_FLAGS_MASK      (0x7u << WINED3D_SM4_INSTRUCTION_FLAGS_SHIFT)

#define WINED3D_SM4_RESOURCE_TYPE_SHIFT         11
#define WINED3D_SM4_RESOURCE_TYPE_MASK          (0xfu << WINED3D_SM4_RESOURCE_TYPE_SHIFT)

#define WINED3D_SM4_RESOURCE_SAMPLE_COUNT_SHIFT 16
#define WINED3D_SM4_RESOURCE_SAMPLE_COUNT_MASK  (0xfu << WINED3D_SM4_RESOURCE_SAMPLE_COUNT_SHIFT)

#define WINED3D_SM4_PRIMITIVE_TYPE_SHIFT        11
#define WINED3D_SM4_PRIMITIVE_TYPE_MASK         (0x3fu << WINED3D_SM4_PRIMITIVE_TYPE_SHIFT)

#define WINED3D_SM4_INDEX_TYPE_SHIFT            11
#define WINED3D_SM4_INDEX_TYPE_MASK             (0x1u << WINED3D_SM4_INDEX_TYPE_SHIFT)

#define WINED3D_SM4_SAMPLER_MODE_SHIFT          11
#define WINED3D_SM4_SAMPLER_MODE_MASK           (0xfu << WINED3D_SM4_SAMPLER_MODE_SHIFT)

#define WINED3D_SM4_SHADER_DATA_TYPE_SHIFT      11
#define WINED3D_SM4_SHADER_DATA_TYPE_MASK       (0xfu << WINED3D_SM4_SHADER_DATA_TYPE_SHIFT)

#define WINED3D_SM4_INTERPOLATION_MODE_SHIFT    11
#define WINED3D_SM4_INTERPOLATION_MODE_MASK     (0xfu << WINED3D_SM4_INTERPOLATION_MODE_SHIFT)

#define WINED3D_SM4_GLOBAL_FLAGS_SHIFT          11
#define WINED3D_SM4_GLOBAL_FLAGS_MASK           (0xffu << WINED3D_SM4_GLOBAL_FLAGS_SHIFT)

#define WINED3D_SM5_PRECISE_SHIFT               19
#define WINED3D_SM5_PRECISE_MASK                (0xfu << WINED3D_SM5_PRECISE_SHIFT)

#define WINED3D_SM5_CONTROL_POINT_COUNT_SHIFT   11
#define WINED3D_SM5_CONTROL_POINT_COUNT_MASK    (0xffu << WINED3D_SM5_CONTROL_POINT_COUNT_SHIFT)

#define WINED3D_SM5_FP_ARRAY_SIZE_SHIFT         16
#define WINED3D_SM5_FP_TABLE_COUNT_MASK         0xffffu

#define WINED3D_SM5_UAV_FLAGS_SHIFT             15
#define WINED3D_SM5_UAV_FLAGS_MASK              (0x1ffu << WINED3D_SM5_UAV_FLAGS_SHIFT)

#define WINED3D_SM5_SYNC_FLAGS_SHIFT            11
#define WINED3D_SM5_SYNC_FLAGS_MASK             (0xffu << WINED3D_SM5_SYNC_FLAGS_SHIFT)

#define WINED3D_SM5_TESSELLATOR_SHIFT           11
#define WINED3D_SM5_TESSELLATOR_MASK            (0xfu << WINED3D_SM5_TESSELLATOR_SHIFT)

#define WINED3D_SM4_OPCODE_MASK                 0xff

#define WINED3D_SM4_REGISTER_MODIFIER           (0x1u << 31)

#define WINED3D_SM4_ADDRESSING_SHIFT1           25
#define WINED3D_SM4_ADDRESSING_MASK1            (0x3u << WINED3D_SM4_ADDRESSING_SHIFT1)

#define WINED3D_SM4_ADDRESSING_SHIFT0           22
#define WINED3D_SM4_ADDRESSING_MASK0            (0x3u << WINED3D_SM4_ADDRESSING_SHIFT0)

#define WINED3D_SM4_REGISTER_ORDER_SHIFT        20
#define WINED3D_SM4_REGISTER_ORDER_MASK         (0x3u << WINED3D_SM4_REGISTER_ORDER_SHIFT)

#define WINED3D_SM4_REGISTER_TYPE_SHIFT         12
#define WINED3D_SM4_REGISTER_TYPE_MASK          (0xffu << WINED3D_SM4_REGISTER_TYPE_SHIFT)

#define WINED3D_SM4_SWIZZLE_TYPE_SHIFT          2
#define WINED3D_SM4_SWIZZLE_TYPE_MASK           (0x3u << WINED3D_SM4_SWIZZLE_TYPE_SHIFT)

#define WINED3D_SM4_DIMENSION_SHIFT             0
#define WINED3D_SM4_DIMENSION_MASK              (0x3u << WINED3D_SM4_DIMENSION_SHIFT)

#define WINED3D_SM4_WRITEMASK_SHIFT             4
#define WINED3D_SM4_WRITEMASK_MASK              (0xfu << WINED3D_SM4_WRITEMASK_SHIFT)

#define WINED3D_SM4_SWIZZLE_SHIFT               4
#define WINED3D_SM4_SWIZZLE_MASK                (0xffu << WINED3D_SM4_SWIZZLE_SHIFT)

#define WINED3D_SM4_VERSION_MAJOR(version)      (((version) >> 4) & 0xf)
#define WINED3D_SM4_VERSION_MINOR(version)      (((version) >> 0) & 0xf)

#define WINED3D_SM4_ADDRESSING_RELATIVE         0x2
#define WINED3D_SM4_ADDRESSING_OFFSET           0x1

#define WINED3D_SM4_INSTRUCTION_FLAG_SATURATE   0x4

#define WINED3D_SM4_CONDITIONAL_NZ              (0x1u << 18)

enum wined3d_sm4_opcode
{
    WINED3D_SM4_OP_ADD                              = 0x00,
    WINED3D_SM4_OP_AND                              = 0x01,
    WINED3D_SM4_OP_BREAK                            = 0x02,
    WINED3D_SM4_OP_BREAKC                           = 0x03,
    WINED3D_SM4_OP_CASE                             = 0x06,
    WINED3D_SM4_OP_CONTINUE                         = 0x07,
    WINED3D_SM4_OP_CONTINUEC                        = 0x08,
    WINED3D_SM4_OP_CUT                              = 0x09,
    WINED3D_SM4_OP_DEFAULT                          = 0x0a,
    WINED3D_SM4_OP_DERIV_RTX                        = 0x0b,
    WINED3D_SM4_OP_DERIV_RTY                        = 0x0c,
    WINED3D_SM4_OP_DISCARD                          = 0x0d,
    WINED3D_SM4_OP_DIV                              = 0x0e,
    WINED3D_SM4_OP_DP2                              = 0x0f,
    WINED3D_SM4_OP_DP3                              = 0x10,
    WINED3D_SM4_OP_DP4                              = 0x11,
    WINED3D_SM4_OP_ELSE                             = 0x12,
    WINED3D_SM4_OP_EMIT                             = 0x13,
    WINED3D_SM4_OP_ENDIF                            = 0x15,
    WINED3D_SM4_OP_ENDLOOP                          = 0x16,
    WINED3D_SM4_OP_ENDSWITCH                        = 0x17,
    WINED3D_SM4_OP_EQ                               = 0x18,
    WINED3D_SM4_OP_EXP                              = 0x19,
    WINED3D_SM4_OP_FRC                              = 0x1a,
    WINED3D_SM4_OP_FTOI                             = 0x1b,
    WINED3D_SM4_OP_FTOU                             = 0x1c,
    WINED3D_SM4_OP_GE                               = 0x1d,
    WINED3D_SM4_OP_IADD                             = 0x1e,
    WINED3D_SM4_OP_IF                               = 0x1f,
    WINED3D_SM4_OP_IEQ                              = 0x20,
    WINED3D_SM4_OP_IGE                              = 0x21,
    WINED3D_SM4_OP_ILT                              = 0x22,
    WINED3D_SM4_OP_IMAD                             = 0x23,
    WINED3D_SM4_OP_IMAX                             = 0x24,
    WINED3D_SM4_OP_IMIN                             = 0x25,
    WINED3D_SM4_OP_IMUL                             = 0x26,
    WINED3D_SM4_OP_INE                              = 0x27,
    WINED3D_SM4_OP_INEG                             = 0x28,
    WINED3D_SM4_OP_ISHL                             = 0x29,
    WINED3D_SM4_OP_ISHR                             = 0x2a,
    WINED3D_SM4_OP_ITOF                             = 0x2b,
    WINED3D_SM4_OP_LABEL                            = 0x2c,
    WINED3D_SM4_OP_LD                               = 0x2d,
    WINED3D_SM4_OP_LD2DMS                           = 0x2e,
    WINED3D_SM4_OP_LOG                              = 0x2f,
    WINED3D_SM4_OP_LOOP                             = 0x30,
    WINED3D_SM4_OP_LT                               = 0x31,
    WINED3D_SM4_OP_MAD                              = 0x32,
    WINED3D_SM4_OP_MIN                              = 0x33,
    WINED3D_SM4_OP_MAX                              = 0x34,
    WINED3D_SM4_OP_SHADER_DATA                      = 0x35,
    WINED3D_SM4_OP_MOV                              = 0x36,
    WINED3D_SM4_OP_MOVC                             = 0x37,
    WINED3D_SM4_OP_MUL                              = 0x38,
    WINED3D_SM4_OP_NE                               = 0x39,
    WINED3D_SM4_OP_NOP                              = 0x3a,
    WINED3D_SM4_OP_NOT                              = 0x3b,
    WINED3D_SM4_OP_OR                               = 0x3c,
    WINED3D_SM4_OP_RESINFO                          = 0x3d,
    WINED3D_SM4_OP_RET                              = 0x3e,
    WINED3D_SM4_OP_RETC                             = 0x3f,
    WINED3D_SM4_OP_ROUND_NE                         = 0x40,
    WINED3D_SM4_OP_ROUND_NI                         = 0x41,
    WINED3D_SM4_OP_ROUND_PI                         = 0x42,
    WINED3D_SM4_OP_ROUND_Z                          = 0x43,
    WINED3D_SM4_OP_RSQ                              = 0x44,
    WINED3D_SM4_OP_SAMPLE                           = 0x45,
    WINED3D_SM4_OP_SAMPLE_C                         = 0x46,
    WINED3D_SM4_OP_SAMPLE_C_LZ                      = 0x47,
    WINED3D_SM4_OP_SAMPLE_LOD                       = 0x48,
    WINED3D_SM4_OP_SAMPLE_GRAD                      = 0x49,
    WINED3D_SM4_OP_SAMPLE_B                         = 0x4a,
    WINED3D_SM4_OP_SQRT                             = 0x4b,
    WINED3D_SM4_OP_SWITCH                           = 0x4c,
    WINED3D_SM4_OP_SINCOS                           = 0x4d,
    WINED3D_SM4_OP_UDIV                             = 0x4e,
    WINED3D_SM4_OP_ULT                              = 0x4f,
    WINED3D_SM4_OP_UGE                              = 0x50,
    WINED3D_SM4_OP_UMUL                             = 0x51,
    WINED3D_SM4_OP_UMAX                             = 0x53,
    WINED3D_SM4_OP_UMIN                             = 0x54,
    WINED3D_SM4_OP_USHR                             = 0x55,
    WINED3D_SM4_OP_UTOF                             = 0x56,
    WINED3D_SM4_OP_XOR                              = 0x57,
    WINED3D_SM4_OP_DCL_RESOURCE                     = 0x58,
    WINED3D_SM4_OP_DCL_CONSTANT_BUFFER              = 0x59,
    WINED3D_SM4_OP_DCL_SAMPLER                      = 0x5a,
    WINED3D_SM4_OP_DCL_INDEX_RANGE                  = 0x5b,
    WINED3D_SM4_OP_DCL_OUTPUT_TOPOLOGY              = 0x5c,
    WINED3D_SM4_OP_DCL_INPUT_PRIMITIVE              = 0x5d,
    WINED3D_SM4_OP_DCL_VERTICES_OUT                 = 0x5e,
    WINED3D_SM4_OP_DCL_INPUT                        = 0x5f,
    WINED3D_SM4_OP_DCL_INPUT_SGV                    = 0x60,
    WINED3D_SM4_OP_DCL_INPUT_SIV                    = 0x61,
    WINED3D_SM4_OP_DCL_INPUT_PS                     = 0x62,
    WINED3D_SM4_OP_DCL_INPUT_PS_SGV                 = 0x63,
    WINED3D_SM4_OP_DCL_INPUT_PS_SIV                 = 0x64,
    WINED3D_SM4_OP_DCL_OUTPUT                       = 0x65,
    WINED3D_SM4_OP_DCL_OUTPUT_SIV                   = 0x67,
    WINED3D_SM4_OP_DCL_TEMPS                        = 0x68,
    WINED3D_SM4_OP_DCL_INDEXABLE_TEMP               = 0x69,
    WINED3D_SM4_OP_DCL_GLOBAL_FLAGS                 = 0x6a,
    WINED3D_SM4_OP_LOD                              = 0x6c,
    WINED3D_SM4_OP_GATHER4                          = 0x6d,
    WINED3D_SM4_OP_SAMPLE_POS                       = 0x6e,
    WINED3D_SM4_OP_SAMPLE_INFO                      = 0x6f,
    WINED3D_SM5_OP_HS_DECLS                         = 0x71,
    WINED3D_SM5_OP_HS_CONTROL_POINT_PHASE           = 0x72,
    WINED3D_SM5_OP_HS_FORK_PHASE                    = 0x73,
    WINED3D_SM5_OP_HS_JOIN_PHASE                    = 0x74,
    WINED3D_SM5_OP_EMIT_STREAM                      = 0x75,
    WINED3D_SM5_OP_CUT_STREAM                       = 0x76,
    WINED3D_SM5_OP_FCALL                            = 0x78,
    WINED3D_SM5_OP_BUFINFO                          = 0x79,
    WINED3D_SM5_OP_DERIV_RTX_COARSE                 = 0x7a,
    WINED3D_SM5_OP_DERIV_RTX_FINE                   = 0x7b,
    WINED3D_SM5_OP_DERIV_RTY_COARSE                 = 0x7c,
    WINED3D_SM5_OP_DERIV_RTY_FINE                   = 0x7d,
    WINED3D_SM5_OP_GATHER4_C                        = 0x7e,
    WINED3D_SM5_OP_GATHER4_PO                       = 0x7f,
    WINED3D_SM5_OP_GATHER4_PO_C                     = 0x80,
    WINED3D_SM5_OP_RCP                              = 0x81,
    WINED3D_SM5_OP_F32TOF16                         = 0x82,
    WINED3D_SM5_OP_F16TOF32                         = 0x83,
    WINED3D_SM5_OP_COUNTBITS                        = 0x86,
    WINED3D_SM5_OP_FIRSTBIT_HI                      = 0x87,
    WINED3D_SM5_OP_FIRSTBIT_LO                      = 0x88,
    WINED3D_SM5_OP_FIRSTBIT_SHI                     = 0x89,
    WINED3D_SM5_OP_UBFE                             = 0x8a,
    WINED3D_SM5_OP_IBFE                             = 0x8b,
    WINED3D_SM5_OP_BFI                              = 0x8c,
    WINED3D_SM5_OP_BFREV                            = 0x8d,
    WINED3D_SM5_OP_SWAPC                            = 0x8e,
    WINED3D_SM5_OP_DCL_STREAM                       = 0x8f,
    WINED3D_SM5_OP_DCL_FUNCTION_BODY                = 0x90,
    WINED3D_SM5_OP_DCL_FUNCTION_TABLE               = 0x91,
    WINED3D_SM5_OP_DCL_INTERFACE                    = 0x92,
    WINED3D_SM5_OP_DCL_INPUT_CONTROL_POINT_COUNT    = 0x93,
    WINED3D_SM5_OP_DCL_OUTPUT_CONTROL_POINT_COUNT   = 0x94,
    WINED3D_SM5_OP_DCL_TESSELLATOR_DOMAIN           = 0x95,
    WINED3D_SM5_OP_DCL_TESSELLATOR_PARTITIONING     = 0x96,
    WINED3D_SM5_OP_DCL_TESSELLATOR_OUTPUT_PRIMITIVE = 0x97,
    WINED3D_SM5_OP_DCL_HS_MAX_TESSFACTOR            = 0x98,
    WINED3D_SM5_OP_DCL_HS_FORK_PHASE_INSTANCE_COUNT = 0x99,
    WINED3D_SM5_OP_DCL_HS_JOIN_PHASE_INSTANCE_COUNT = 0x9a,
    WINED3D_SM5_OP_DCL_THREAD_GROUP                 = 0x9b,
    WINED3D_SM5_OP_DCL_UAV_TYPED                    = 0x9c,
    WINED3D_SM5_OP_DCL_UAV_RAW                      = 0x9d,
    WINED3D_SM5_OP_DCL_UAV_STRUCTURED               = 0x9e,
    WINED3D_SM5_OP_DCL_TGSM_RAW                     = 0x9f,
    WINED3D_SM5_OP_DCL_TGSM_STRUCTURED              = 0xa0,
    WINED3D_SM5_OP_DCL_RESOURCE_RAW                 = 0xa1,
    WINED3D_SM5_OP_DCL_RESOURCE_STRUCTURED          = 0xa2,
    WINED3D_SM5_OP_LD_UAV_TYPED                     = 0xa3,
    WINED3D_SM5_OP_STORE_UAV_TYPED                  = 0xa4,
    WINED3D_SM5_OP_LD_RAW                           = 0xa5,
    WINED3D_SM5_OP_STORE_RAW                        = 0xa6,
    WINED3D_SM5_OP_LD_STRUCTURED                    = 0xa7,
    WINED3D_SM5_OP_STORE_STRUCTURED                 = 0xa8,
    WINED3D_SM5_OP_ATOMIC_AND                       = 0xa9,
    WINED3D_SM5_OP_ATOMIC_OR                        = 0xaa,
    WINED3D_SM5_OP_ATOMIC_XOR                       = 0xab,
    WINED3D_SM5_OP_ATOMIC_CMP_STORE                 = 0xac,
    WINED3D_SM5_OP_ATOMIC_IADD                      = 0xad,
    WINED3D_SM5_OP_ATOMIC_IMAX                      = 0xae,
    WINED3D_SM5_OP_ATOMIC_IMIN                      = 0xaf,
    WINED3D_SM5_OP_ATOMIC_UMAX                      = 0xb0,
    WINED3D_SM5_OP_ATOMIC_UMIN                      = 0xb1,
    WINED3D_SM5_OP_IMM_ATOMIC_ALLOC                 = 0xb2,
    WINED3D_SM5_OP_IMM_ATOMIC_CONSUME               = 0xb3,
    WINED3D_SM5_OP_IMM_ATOMIC_IADD                  = 0xb4,
    WINED3D_SM5_OP_IMM_ATOMIC_AND                   = 0xb5,
    WINED3D_SM5_OP_IMM_ATOMIC_OR                    = 0xb6,
    WINED3D_SM5_OP_IMM_ATOMIC_XOR                   = 0xb7,
    WINED3D_SM5_OP_IMM_ATOMIC_EXCH                  = 0xb8,
    WINED3D_SM5_OP_IMM_ATOMIC_CMP_EXCH              = 0xb9,
    WINED3D_SM5_OP_IMM_ATOMIC_IMAX                  = 0xba,
    WINED3D_SM5_OP_IMM_ATOMIC_IMIN                  = 0xbb,
    WINED3D_SM5_OP_IMM_ATOMIC_UMAX                  = 0xbc,
    WINED3D_SM5_OP_IMM_ATOMIC_UMIN                  = 0xbd,
    WINED3D_SM5_OP_SYNC                             = 0xbe,
    WINED3D_SM5_OP_EVAL_SAMPLE_INDEX                = 0xcc,
    WINED3D_SM5_OP_EVAL_CENTROID                    = 0xcd,
    WINED3D_SM5_OP_DCL_GS_INSTANCES                 = 0xce,
};

enum wined3d_sm4_instruction_modifier
{
    WINED3D_SM4_MODIFIER_AOFFIMMI       = 0x1,
    WINED3D_SM5_MODIFIER_RESOURCE_TYPE  = 0x2,
    WINED3D_SM5_MODIFIER_DATA_TYPE      = 0x3,
};

enum wined3d_sm4_register_type
{
    WINED3D_SM4_RT_TEMP                    = 0x00,
    WINED3D_SM4_RT_INPUT                   = 0x01,
    WINED3D_SM4_RT_OUTPUT                  = 0x02,
    WINED3D_SM4_RT_INDEXABLE_TEMP          = 0x03,
    WINED3D_SM4_RT_IMMCONST                = 0x04,
    WINED3D_SM4_RT_SAMPLER                 = 0x06,
    WINED3D_SM4_RT_RESOURCE                = 0x07,
    WINED3D_SM4_RT_CONSTBUFFER             = 0x08,
    WINED3D_SM4_RT_IMMCONSTBUFFER          = 0x09,
    WINED3D_SM4_RT_PRIMID                  = 0x0b,
    WINED3D_SM4_RT_DEPTHOUT                = 0x0c,
    WINED3D_SM4_RT_NULL                    = 0x0d,
    WINED3D_SM4_RT_RASTERIZER              = 0x0e,
    WINED3D_SM4_RT_OMASK                   = 0x0f,
    WINED3D_SM5_RT_STREAM                  = 0x10,
    WINED3D_SM5_RT_FUNCTION_BODY           = 0x11,
    WINED3D_SM5_RT_FUNCTION_POINTER        = 0x13,
    WINED3D_SM5_RT_OUTPUT_CONTROL_POINT_ID = 0x16,
    WINED3D_SM5_RT_FORK_INSTANCE_ID        = 0x17,
    WINED3D_SM5_RT_JOIN_INSTANCE_ID        = 0x18,
    WINED3D_SM5_RT_INPUT_CONTROL_POINT     = 0x19,
    WINED3D_SM5_RT_OUTPUT_CONTROL_POINT    = 0x1a,
    WINED3D_SM5_RT_PATCH_CONSTANT_DATA     = 0x1b,
    WINED3D_SM5_RT_DOMAIN_LOCATION         = 0x1c,
    WINED3D_SM5_RT_UAV                     = 0x1e,
    WINED3D_SM5_RT_SHARED_MEMORY           = 0x1f,
    WINED3D_SM5_RT_THREAD_ID               = 0x20,
    WINED3D_SM5_RT_THREAD_GROUP_ID         = 0x21,
    WINED3D_SM5_RT_LOCAL_THREAD_ID         = 0x22,
    WINED3D_SM5_RT_COVERAGE                = 0x23,
    WINED3D_SM5_RT_LOCAL_THREAD_INDEX      = 0x24,
    WINED3D_SM5_RT_GS_INSTANCE_ID          = 0x25,
    WINED3D_SM5_RT_DEPTHOUT_GREATER_EQUAL  = 0x26,
    WINED3D_SM5_RT_DEPTHOUT_LESS_EQUAL     = 0x27,
    WINED3D_SM5_RT_OUTPUT_STENCIL_REF      = 0x29,
};

enum wined3d_sm4_output_primitive_type
{
    WINED3D_SM4_OUTPUT_PT_POINTLIST     = 0x1,
    WINED3D_SM4_OUTPUT_PT_LINESTRIP     = 0x3,
    WINED3D_SM4_OUTPUT_PT_TRIANGLESTRIP = 0x5,
};

enum wined3d_sm4_input_primitive_type
{
    WINED3D_SM4_INPUT_PT_POINT          = 0x01,
    WINED3D_SM4_INPUT_PT_LINE           = 0x02,
    WINED3D_SM4_INPUT_PT_TRIANGLE       = 0x03,
    WINED3D_SM4_INPUT_PT_LINEADJ        = 0x06,
    WINED3D_SM4_INPUT_PT_TRIANGLEADJ    = 0x07,
    WINED3D_SM5_INPUT_PT_PATCH1         = 0x08,
    WINED3D_SM5_INPUT_PT_PATCH2         = 0x09,
    WINED3D_SM5_INPUT_PT_PATCH3         = 0x0a,
    WINED3D_SM5_INPUT_PT_PATCH4         = 0x0b,
    WINED3D_SM5_INPUT_PT_PATCH5         = 0x0c,
    WINED3D_SM5_INPUT_PT_PATCH6         = 0x0d,
    WINED3D_SM5_INPUT_PT_PATCH7         = 0x0e,
    WINED3D_SM5_INPUT_PT_PATCH8         = 0x0f,
    WINED3D_SM5_INPUT_PT_PATCH9         = 0x10,
    WINED3D_SM5_INPUT_PT_PATCH10        = 0x11,
    WINED3D_SM5_INPUT_PT_PATCH11        = 0x12,
    WINED3D_SM5_INPUT_PT_PATCH12        = 0x13,
    WINED3D_SM5_INPUT_PT_PATCH13        = 0x14,
    WINED3D_SM5_INPUT_PT_PATCH14        = 0x15,
    WINED3D_SM5_INPUT_PT_PATCH15        = 0x16,
    WINED3D_SM5_INPUT_PT_PATCH16        = 0x17,
    WINED3D_SM5_INPUT_PT_PATCH17        = 0x18,
    WINED3D_SM5_INPUT_PT_PATCH18        = 0x19,
    WINED3D_SM5_INPUT_PT_PATCH19        = 0x1a,
    WINED3D_SM5_INPUT_PT_PATCH20        = 0x1b,
    WINED3D_SM5_INPUT_PT_PATCH21        = 0x1c,
    WINED3D_SM5_INPUT_PT_PATCH22        = 0x1d,
    WINED3D_SM5_INPUT_PT_PATCH23        = 0x1e,
    WINED3D_SM5_INPUT_PT_PATCH24        = 0x1f,
    WINED3D_SM5_INPUT_PT_PATCH25        = 0x20,
    WINED3D_SM5_INPUT_PT_PATCH26        = 0x21,
    WINED3D_SM5_INPUT_PT_PATCH27        = 0x22,
    WINED3D_SM5_INPUT_PT_PATCH28        = 0x23,
    WINED3D_SM5_INPUT_PT_PATCH29        = 0x24,
    WINED3D_SM5_INPUT_PT_PATCH30        = 0x25,
    WINED3D_SM5_INPUT_PT_PATCH31        = 0x26,
    WINED3D_SM5_INPUT_PT_PATCH32        = 0x27,
};

enum wined3d_sm4_swizzle_type
{
    WINED3D_SM4_SWIZZLE_NONE            = 0x0,
    WINED3D_SM4_SWIZZLE_VEC4            = 0x1,
    WINED3D_SM4_SWIZZLE_SCALAR          = 0x2,
};

enum wined3d_sm4_dimension
{
    WINED3D_SM4_DIMENSION_SCALAR    = 0x1,
    WINED3D_SM4_DIMENSION_VEC4      = 0x2,
};

enum wined3d_sm4_resource_type
{
    WINED3D_SM4_RESOURCE_BUFFER             = 0x1,
    WINED3D_SM4_RESOURCE_TEXTURE_1D         = 0x2,
    WINED3D_SM4_RESOURCE_TEXTURE_2D         = 0x3,
    WINED3D_SM4_RESOURCE_TEXTURE_2DMS       = 0x4,
    WINED3D_SM4_RESOURCE_TEXTURE_3D         = 0x5,
    WINED3D_SM4_RESOURCE_TEXTURE_CUBE       = 0x6,
    WINED3D_SM4_RESOURCE_TEXTURE_1DARRAY    = 0x7,
    WINED3D_SM4_RESOURCE_TEXTURE_2DARRAY    = 0x8,
    WINED3D_SM4_RESOURCE_TEXTURE_2DMSARRAY  = 0x9,
    WINED3D_SM4_RESOURCE_TEXTURE_CUBEARRAY  = 0xa,
};

enum wined3d_sm4_data_type
{
    WINED3D_SM4_DATA_UNORM  = 0x1,
    WINED3D_SM4_DATA_SNORM  = 0x2,
    WINED3D_SM4_DATA_INT    = 0x3,
    WINED3D_SM4_DATA_UINT   = 0x4,
    WINED3D_SM4_DATA_FLOAT  = 0x5,
};

enum wined3d_sm4_sampler_mode
{
    WINED3D_SM4_SAMPLER_DEFAULT    = 0x0,
    WINED3D_SM4_SAMPLER_COMPARISON = 0x1,
};

enum wined3d_sm4_shader_data_type
{
    WINED3D_SM4_SHADER_DATA_IMMEDIATE_CONSTANT_BUFFER = 0x3,
    WINED3D_SM4_SHADER_DATA_MESSAGE                   = 0x4,
};

struct wined3d_shader_src_param_entry
{
    struct list entry;
    struct wined3d_shader_src_param param;
};

struct wined3d_sm4_data
{
    struct wined3d_shader_version shader_version;
    const DWORD *start, *end;

    unsigned int output_map[MAX_REG_OUTPUT];

    struct wined3d_shader_src_param src_param[5];
    struct wined3d_shader_dst_param dst_param[2];
    struct list src_free;
    struct list src;
    struct wined3d_shader_immediate_constant_buffer icb;
};

struct wined3d_sm4_opcode_info
{
    enum wined3d_sm4_opcode opcode;
    enum WINED3D_SHADER_INSTRUCTION_HANDLER handler_idx;
    const char *dst_info;
    const char *src_info;
    void (*read_opcode_func)(struct wined3d_shader_instruction *ins,
            DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
            struct wined3d_sm4_data *priv);
};

static const enum wined3d_primitive_type output_primitive_type_table[] =
{
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_POINTLIST */       WINED3D_PT_POINTLIST,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_LINESTRIP */       WINED3D_PT_LINESTRIP,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_OUTPUT_PT_TRIANGLESTRIP */   WINED3D_PT_TRIANGLESTRIP,
};

static const enum wined3d_primitive_type input_primitive_type_table[] =
{
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_INPUT_PT_POINT */            WINED3D_PT_POINTLIST,
    /* WINED3D_SM4_INPUT_PT_LINE */             WINED3D_PT_LINELIST,
    /* WINED3D_SM4_INPUT_PT_TRIANGLE */         WINED3D_PT_TRIANGLELIST,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* UNKNOWN */                               WINED3D_PT_UNDEFINED,
    /* WINED3D_SM4_INPUT_PT_LINEADJ */          WINED3D_PT_LINELIST_ADJ,
    /* WINED3D_SM4_INPUT_PT_TRIANGLEADJ */      WINED3D_PT_TRIANGLELIST_ADJ,
};

static const enum wined3d_shader_resource_type resource_type_table[] =
{
    /* 0 */                                         WINED3D_SHADER_RESOURCE_NONE,
    /* WINED3D_SM4_RESOURCE_BUFFER */               WINED3D_SHADER_RESOURCE_BUFFER,
    /* WINED3D_SM4_RESOURCE_TEXTURE_1D */           WINED3D_SHADER_RESOURCE_TEXTURE_1D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2D */           WINED3D_SHADER_RESOURCE_TEXTURE_2D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DMS */         WINED3D_SHADER_RESOURCE_TEXTURE_2DMS,
    /* WINED3D_SM4_RESOURCE_TEXTURE_3D */           WINED3D_SHADER_RESOURCE_TEXTURE_3D,
    /* WINED3D_SM4_RESOURCE_TEXTURE_CUBE */         WINED3D_SHADER_RESOURCE_TEXTURE_CUBE,
    /* WINED3D_SM4_RESOURCE_TEXTURE_1DARRAY */      WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DARRAY */      WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY,
    /* WINED3D_SM4_RESOURCE_TEXTURE_2DMSARRAY */    WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY,
    /* WINED3D_SM4_RESOURCE_TEXTURE_CUBEARRAY */    WINED3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY,
};

static const enum wined3d_data_type data_type_table[] =
{
    /* 0 */                         WINED3D_DATA_FLOAT,
    /* WINED3D_SM4_DATA_UNORM */    WINED3D_DATA_UNORM,
    /* WINED3D_SM4_DATA_SNORM */    WINED3D_DATA_SNORM,
    /* WINED3D_SM4_DATA_INT */      WINED3D_DATA_INT,
    /* WINED3D_SM4_DATA_UINT */     WINED3D_DATA_UINT,
    /* WINED3D_SM4_DATA_FLOAT */    WINED3D_DATA_FLOAT,
};

static BOOL shader_sm4_read_src_param(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        enum wined3d_data_type data_type, struct wined3d_shader_src_param *src_param);
static BOOL shader_sm4_read_dst_param(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        enum wined3d_data_type data_type, struct wined3d_shader_dst_param *dst_param);

static void shader_sm4_read_conditional_op(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_src_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_UINT, &priv->src_param[0]);
    ins->flags = (opcode_token & WINED3D_SM4_CONDITIONAL_NZ) ?
            WINED3D_SHADER_CONDITIONAL_OP_NZ : WINED3D_SHADER_CONDITIONAL_OP_Z;
}

static void shader_sm4_read_shader_data(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    enum wined3d_sm4_shader_data_type type;
    unsigned int icb_size;

    type = (opcode_token & WINED3D_SM4_SHADER_DATA_TYPE_MASK) >> WINED3D_SM4_SHADER_DATA_TYPE_SHIFT;
    if (type != WINED3D_SM4_SHADER_DATA_IMMEDIATE_CONSTANT_BUFFER)
    {
        FIXME("Ignoring shader data type %#x.\n", type);
        ins->handler_idx = WINED3DSIH_NOP;
        return;
    }

    ++tokens;
    icb_size = token_count - 1;
    if (icb_size % 4 || icb_size > MAX_IMMEDIATE_CONSTANT_BUFFER_SIZE)
    {
        FIXME("Unexpected immediate constant buffer size %u.\n", icb_size);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        return;
    }

    priv->icb.vec4_count = icb_size / 4;
    memcpy(priv->icb.data, tokens, sizeof(*tokens) * icb_size);
    ins->declaration.icb = &priv->icb;
}

static void shader_sm4_read_dcl_resource(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    enum wined3d_sm4_resource_type resource_type;
    enum wined3d_sm4_data_type data_type;
    enum wined3d_data_type reg_data_type;
    uint32_t components;

    resource_type = (opcode_token & WINED3D_SM4_RESOURCE_TYPE_MASK) >> WINED3D_SM4_RESOURCE_TYPE_SHIFT;
    if (!resource_type || (resource_type >= ARRAY_SIZE(resource_type_table)))
    {
        FIXME("Unhandled resource type %#x.\n", resource_type);
        ins->declaration.semantic.resource_type = WINED3D_SHADER_RESOURCE_NONE;
    }
    else
    {
        ins->declaration.semantic.resource_type = resource_type_table[resource_type];
    }

    if (ins->declaration.semantic.resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2DMS
            || ins->declaration.semantic.resource_type == WINED3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY)
    {
        ins->declaration.semantic.sample_count = (opcode_token & WINED3D_SM4_RESOURCE_SAMPLE_COUNT_MASK) >> WINED3D_SM4_RESOURCE_SAMPLE_COUNT_SHIFT;
    }

    reg_data_type = opcode == WINED3D_SM4_OP_DCL_RESOURCE ? WINED3D_DATA_RESOURCE : WINED3D_DATA_UAV;
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], reg_data_type, &ins->declaration.semantic.reg);

    components = *tokens++;
    if ((components & 0xfff0) != (components & 0xf) * 0x1110)
        FIXME("Components (%#x) have different data types.\n", components);
    data_type = components & 0xf;

    if (!data_type || (data_type >= ARRAY_SIZE(data_type_table)))
    {
        FIXME("Unhandled data type %#x.\n", data_type);
        ins->declaration.semantic.resource_data_type = WINED3D_DATA_FLOAT;
    }
    else
    {
        ins->declaration.semantic.resource_data_type = data_type_table[data_type];
    }

    if (reg_data_type == WINED3D_DATA_UAV)
        ins->flags = (opcode_token & WINED3D_SM5_UAV_FLAGS_MASK) >> WINED3D_SM5_UAV_FLAGS_SHIFT;
}

static void shader_sm4_read_dcl_constant_buffer(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_src_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT, &ins->declaration.src);
    if (opcode_token & WINED3D_SM4_INDEX_TYPE_MASK)
        ins->flags |= WINED3DSI_INDEXED_DYNAMIC;
}

static void shader_sm4_read_dcl_sampler(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->flags = (opcode_token & WINED3D_SM4_SAMPLER_MODE_MASK) >> WINED3D_SM4_SAMPLER_MODE_SHIFT;
    if (ins->flags & ~WINED3D_SM4_SAMPLER_COMPARISON)
        FIXME("Unhandled sampler mode %#x.\n", ins->flags);
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_SAMPLER, &ins->declaration.dst);
}

static void shader_sm4_read_dcl_index_range(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_OPAQUE,
            &ins->declaration.index_range.first_register);
    ins->declaration.index_range.last_register = *tokens;
}

static void shader_sm4_read_dcl_output_topology(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    enum wined3d_sm4_output_primitive_type primitive_type;

    primitive_type = (opcode_token & WINED3D_SM4_PRIMITIVE_TYPE_MASK) >> WINED3D_SM4_PRIMITIVE_TYPE_SHIFT;
    if (primitive_type >= ARRAY_SIZE(output_primitive_type_table))
        ins->declaration.primitive_type.type = WINED3D_PT_UNDEFINED;
    else
        ins->declaration.primitive_type.type = output_primitive_type_table[primitive_type];

    if (ins->declaration.primitive_type.type == WINED3D_PT_UNDEFINED)
        FIXME("Unhandled output primitive type %#x.\n", primitive_type);
}

static void shader_sm4_read_dcl_input_primitive(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    enum wined3d_sm4_input_primitive_type primitive_type;

    primitive_type = (opcode_token & WINED3D_SM4_PRIMITIVE_TYPE_MASK) >> WINED3D_SM4_PRIMITIVE_TYPE_SHIFT;
    if (WINED3D_SM5_INPUT_PT_PATCH1 <= primitive_type && primitive_type <= WINED3D_SM5_INPUT_PT_PATCH32)
    {
        ins->declaration.primitive_type.type = WINED3D_PT_PATCH;
        ins->declaration.primitive_type.patch_vertex_count = primitive_type - WINED3D_SM5_INPUT_PT_PATCH1 + 1;
    }
    else if (primitive_type >= ARRAY_SIZE(input_primitive_type_table))
    {
        ins->declaration.primitive_type.type = WINED3D_PT_UNDEFINED;
    }
    else
    {
        ins->declaration.primitive_type.type = input_primitive_type_table[primitive_type];
    }

    if (ins->declaration.primitive_type.type == WINED3D_PT_UNDEFINED)
        FIXME("Unhandled input primitive type %#x.\n", primitive_type);
}

static void shader_sm4_read_declaration_count(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.count = *tokens;
}

static void shader_sm4_read_declaration_dst(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT, &ins->declaration.dst);
}

static void shader_sm4_read_declaration_register_semantic(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT,
            &ins->declaration.register_semantic.reg);
    ins->declaration.register_semantic.sysval_semantic = *tokens;
}

static void shader_sm4_read_dcl_input_ps(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->flags = (opcode_token & WINED3D_SM4_INTERPOLATION_MODE_MASK) >> WINED3D_SM4_INTERPOLATION_MODE_SHIFT;
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT, &ins->declaration.dst);
}

static void shader_sm4_read_dcl_input_ps_siv(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->flags = (opcode_token & WINED3D_SM4_INTERPOLATION_MODE_MASK) >> WINED3D_SM4_INTERPOLATION_MODE_SHIFT;
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT,
            &ins->declaration.register_semantic.reg);
    ins->declaration.register_semantic.sysval_semantic = *tokens;
}

static void shader_sm4_read_dcl_indexable_temp(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.indexable_temp.register_idx = *tokens++;
    ins->declaration.indexable_temp.register_size = *tokens++;
    ins->declaration.indexable_temp.component_count = *tokens;
}

static void shader_sm4_read_dcl_global_flags(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->flags = (opcode_token & WINED3D_SM4_GLOBAL_FLAGS_MASK) >> WINED3D_SM4_GLOBAL_FLAGS_SHIFT;
}

static void shader_sm5_read_fcall(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    priv->src_param[0].reg.u.fp_body_idx = *tokens++;
    shader_sm4_read_src_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_OPAQUE, &priv->src_param[0]);
}

static void shader_sm5_read_dcl_function_body(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.index = *tokens;
}

static void shader_sm5_read_dcl_function_table(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.index = *tokens++;
    FIXME("Ignoring set of function bodies (count %lu).\n", *tokens);
}

static void shader_sm5_read_dcl_interface(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.fp.index = *tokens++;
    ins->declaration.fp.body_count = *tokens++;
    ins->declaration.fp.array_size = *tokens >> WINED3D_SM5_FP_ARRAY_SIZE_SHIFT;
    ins->declaration.fp.table_count = *tokens++ & WINED3D_SM5_FP_TABLE_COUNT_MASK;
    FIXME("Ignoring set of function tables (count %u).\n", ins->declaration.fp.table_count);
}

static void shader_sm5_read_control_point_count(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.count = (opcode_token & WINED3D_SM5_CONTROL_POINT_COUNT_MASK)
            >> WINED3D_SM5_CONTROL_POINT_COUNT_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_domain(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.tessellator_domain = (opcode_token & WINED3D_SM5_TESSELLATOR_MASK)
        >> WINED3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_partitioning(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.tessellator_partitioning = (opcode_token & WINED3D_SM5_TESSELLATOR_MASK)
            >> WINED3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_tessellator_output_primitive(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.tessellator_output_primitive = (opcode_token & WINED3D_SM5_TESSELLATOR_MASK)
            >> WINED3D_SM5_TESSELLATOR_SHIFT;
}

static void shader_sm5_read_dcl_hs_max_tessfactor(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.max_tessellation_factor = *(float *)tokens;
}

static void shader_sm5_read_dcl_thread_group(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->declaration.thread_group_size.x = *tokens++;
    ins->declaration.thread_group_size.y = *tokens++;
    ins->declaration.thread_group_size.z = *tokens++;
}

static void shader_sm5_read_dcl_uav_raw(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_UAV, &ins->declaration.dst);
    ins->flags = (opcode_token & WINED3D_SM5_UAV_FLAGS_MASK) >> WINED3D_SM5_UAV_FLAGS_SHIFT;
}

static void shader_sm5_read_dcl_uav_structured(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_UAV,
            &ins->declaration.structured_resource.reg);
    ins->flags = (opcode_token & WINED3D_SM5_UAV_FLAGS_MASK) >> WINED3D_SM5_UAV_FLAGS_SHIFT;
    ins->declaration.structured_resource.byte_stride = *tokens;
    if (ins->declaration.structured_resource.byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", ins->declaration.structured_resource.byte_stride);
}

static void shader_sm5_read_dcl_tgsm_raw(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT, &ins->declaration.tgsm_raw.reg);
    ins->declaration.tgsm_raw.byte_count = *tokens;
    if (ins->declaration.tgsm_raw.byte_count % 4)
        FIXME("Byte count %u is not multiple of 4.\n", ins->declaration.tgsm_raw.byte_count);
}

static void shader_sm5_read_dcl_tgsm_structured(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_FLOAT,
            &ins->declaration.tgsm_structured.reg);
    ins->declaration.tgsm_structured.byte_stride = *tokens++;
    ins->declaration.tgsm_structured.structure_count = *tokens;
    if (ins->declaration.tgsm_structured.byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", ins->declaration.tgsm_structured.byte_stride);
}

static void shader_sm5_read_dcl_resource_structured(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_RESOURCE,
            &ins->declaration.structured_resource.reg);
    ins->declaration.structured_resource.byte_stride = *tokens;
    if (ins->declaration.structured_resource.byte_stride % 4)
        FIXME("Byte stride %u is not multiple of 4.\n", ins->declaration.structured_resource.byte_stride);
}

static void shader_sm5_read_dcl_resource_raw(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    shader_sm4_read_dst_param(priv, &tokens, &tokens[token_count], WINED3D_DATA_RESOURCE, &ins->declaration.dst);
}

static void shader_sm5_read_sync(struct wined3d_shader_instruction *ins,
        DWORD opcode, DWORD opcode_token, const DWORD *tokens, unsigned int token_count,
        struct wined3d_sm4_data *priv)
{
    ins->flags = (opcode_token & WINED3D_SM5_SYNC_FLAGS_MASK) >> WINED3D_SM5_SYNC_FLAGS_SHIFT;
}

/*
 * f -> WINED3D_DATA_FLOAT
 * i -> WINED3D_DATA_INT
 * u -> WINED3D_DATA_UINT
 * O -> WINED3D_DATA_OPAQUE
 * R -> WINED3D_DATA_RESOURCE
 * S -> WINED3D_DATA_SAMPLER
 * U -> WINED3D_DATA_UAV
 */
static const struct wined3d_sm4_opcode_info opcode_table[] =
{
    {WINED3D_SM4_OP_ADD,                              WINED3DSIH_ADD,                              "f",    "ff"},
    {WINED3D_SM4_OP_AND,                              WINED3DSIH_AND,                              "u",    "uu"},
    {WINED3D_SM4_OP_BREAK,                            WINED3DSIH_BREAK,                            "",     ""},
    {WINED3D_SM4_OP_BREAKC,                           WINED3DSIH_BREAKP,                           "",     "u",
            shader_sm4_read_conditional_op},
    {WINED3D_SM4_OP_CASE,                             WINED3DSIH_CASE,                             "",     "u"},
    {WINED3D_SM4_OP_CONTINUE,                         WINED3DSIH_CONTINUE,                         "",     ""},
    {WINED3D_SM4_OP_CONTINUEC,                        WINED3DSIH_CONTINUEP,                        "",     "u",
            shader_sm4_read_conditional_op},
    {WINED3D_SM4_OP_CUT,                              WINED3DSIH_CUT,                              "",     ""},
    {WINED3D_SM4_OP_DEFAULT,                          WINED3DSIH_DEFAULT,                          "",     ""},
    {WINED3D_SM4_OP_DERIV_RTX,                        WINED3DSIH_DSX,                              "f",    "f"},
    {WINED3D_SM4_OP_DERIV_RTY,                        WINED3DSIH_DSY,                              "f",    "f"},
    {WINED3D_SM4_OP_DISCARD,                          WINED3DSIH_TEXKILL,                          "",     "u",
            shader_sm4_read_conditional_op},
    {WINED3D_SM4_OP_DIV,                              WINED3DSIH_DIV,                              "f",    "ff"},
    {WINED3D_SM4_OP_DP2,                              WINED3DSIH_DP2,                              "f",    "ff"},
    {WINED3D_SM4_OP_DP3,                              WINED3DSIH_DP3,                              "f",    "ff"},
    {WINED3D_SM4_OP_DP4,                              WINED3DSIH_DP4,                              "f",    "ff"},
    {WINED3D_SM4_OP_ELSE,                             WINED3DSIH_ELSE,                             "",     ""},
    {WINED3D_SM4_OP_EMIT,                             WINED3DSIH_EMIT,                             "",     ""},
    {WINED3D_SM4_OP_ENDIF,                            WINED3DSIH_ENDIF,                            "",     ""},
    {WINED3D_SM4_OP_ENDLOOP,                          WINED3DSIH_ENDLOOP,                          "",     ""},
    {WINED3D_SM4_OP_ENDSWITCH,                        WINED3DSIH_ENDSWITCH,                        "",     ""},
    {WINED3D_SM4_OP_EQ,                               WINED3DSIH_EQ,                               "u",    "ff"},
    {WINED3D_SM4_OP_EXP,                              WINED3DSIH_EXP,                              "f",    "f"},
    {WINED3D_SM4_OP_FRC,                              WINED3DSIH_FRC,                              "f",    "f"},
    {WINED3D_SM4_OP_FTOI,                             WINED3DSIH_FTOI,                             "i",    "f"},
    {WINED3D_SM4_OP_FTOU,                             WINED3DSIH_FTOU,                             "u",    "f"},
    {WINED3D_SM4_OP_GE,                               WINED3DSIH_GE,                               "u",    "ff"},
    {WINED3D_SM4_OP_IADD,                             WINED3DSIH_IADD,                             "i",    "ii"},
    {WINED3D_SM4_OP_IF,                               WINED3DSIH_IF,                               "",     "u",
            shader_sm4_read_conditional_op},
    {WINED3D_SM4_OP_IEQ,                              WINED3DSIH_IEQ,                              "u",    "ii"},
    {WINED3D_SM4_OP_IGE,                              WINED3DSIH_IGE,                              "u",    "ii"},
    {WINED3D_SM4_OP_ILT,                              WINED3DSIH_ILT,                              "u",    "ii"},
    {WINED3D_SM4_OP_IMAD,                             WINED3DSIH_IMAD,                             "i",    "iii"},
    {WINED3D_SM4_OP_IMAX,                             WINED3DSIH_IMAX,                             "i",    "ii"},
    {WINED3D_SM4_OP_IMIN,                             WINED3DSIH_IMIN,                             "i",    "ii"},
    {WINED3D_SM4_OP_IMUL,                             WINED3DSIH_IMUL,                             "ii",   "ii"},
    {WINED3D_SM4_OP_INE,                              WINED3DSIH_INE,                              "u",    "ii"},
    {WINED3D_SM4_OP_INEG,                             WINED3DSIH_INEG,                             "i",    "i"},
    {WINED3D_SM4_OP_ISHL,                             WINED3DSIH_ISHL,                             "i",    "ii"},
    {WINED3D_SM4_OP_ISHR,                             WINED3DSIH_ISHR,                             "i",    "ii"},
    {WINED3D_SM4_OP_ITOF,                             WINED3DSIH_ITOF,                             "f",    "i"},
    {WINED3D_SM4_OP_LABEL,                            WINED3DSIH_LABEL,                            "",     "O"},
    {WINED3D_SM4_OP_LD,                               WINED3DSIH_LD,                               "u",    "iR"},
    {WINED3D_SM4_OP_LD2DMS,                           WINED3DSIH_LD2DMS,                           "u",    "iRi"},
    {WINED3D_SM4_OP_LOG,                              WINED3DSIH_LOG,                              "f",    "f"},
    {WINED3D_SM4_OP_LOOP,                             WINED3DSIH_LOOP,                             "",     ""},
    {WINED3D_SM4_OP_LT,                               WINED3DSIH_LT,                               "u",    "ff"},
    {WINED3D_SM4_OP_MAD,                              WINED3DSIH_MAD,                              "f",    "fff"},
    {WINED3D_SM4_OP_MIN,                              WINED3DSIH_MIN,                              "f",    "ff"},
    {WINED3D_SM4_OP_MAX,                              WINED3DSIH_MAX,                              "f",    "ff"},
    {WINED3D_SM4_OP_SHADER_DATA,                      WINED3DSIH_DCL_IMMEDIATE_CONSTANT_BUFFER,    "",     "",
            shader_sm4_read_shader_data},
    {WINED3D_SM4_OP_MOV,                              WINED3DSIH_MOV,                              "f",    "f"},
    {WINED3D_SM4_OP_MOVC,                             WINED3DSIH_MOVC,                             "f",    "uff"},
    {WINED3D_SM4_OP_MUL,                              WINED3DSIH_MUL,                              "f",    "ff"},
    {WINED3D_SM4_OP_NE,                               WINED3DSIH_NE,                               "u",    "ff"},
    {WINED3D_SM4_OP_NOP,                              WINED3DSIH_NOP,                              "",     ""},
    {WINED3D_SM4_OP_NOT,                              WINED3DSIH_NOT,                              "u",    "u"},
    {WINED3D_SM4_OP_OR,                               WINED3DSIH_OR,                               "u",    "uu"},
    {WINED3D_SM4_OP_RESINFO,                          WINED3DSIH_RESINFO,                          "f",    "iR"},
    {WINED3D_SM4_OP_RET,                              WINED3DSIH_RET,                              "",     ""},
    {WINED3D_SM4_OP_RETC,                             WINED3DSIH_RETP,                             "",     "u",
            shader_sm4_read_conditional_op},
    {WINED3D_SM4_OP_ROUND_NE,                         WINED3DSIH_ROUND_NE,                         "f",    "f"},
    {WINED3D_SM4_OP_ROUND_NI,                         WINED3DSIH_ROUND_NI,                         "f",    "f"},
    {WINED3D_SM4_OP_ROUND_PI,                         WINED3DSIH_ROUND_PI,                         "f",    "f"},
    {WINED3D_SM4_OP_ROUND_Z,                          WINED3DSIH_ROUND_Z,                          "f",    "f"},
    {WINED3D_SM4_OP_RSQ,                              WINED3DSIH_RSQ,                              "f",    "f"},
    {WINED3D_SM4_OP_SAMPLE,                           WINED3DSIH_SAMPLE,                           "u",    "fRS"},
    {WINED3D_SM4_OP_SAMPLE_C,                         WINED3DSIH_SAMPLE_C,                         "f",    "fRSf"},
    {WINED3D_SM4_OP_SAMPLE_C_LZ,                      WINED3DSIH_SAMPLE_C_LZ,                      "f",    "fRSf"},
    {WINED3D_SM4_OP_SAMPLE_LOD,                       WINED3DSIH_SAMPLE_LOD,                       "u",    "fRSf"},
    {WINED3D_SM4_OP_SAMPLE_GRAD,                      WINED3DSIH_SAMPLE_GRAD,                      "u",    "fRSff"},
    {WINED3D_SM4_OP_SAMPLE_B,                         WINED3DSIH_SAMPLE_B,                         "u",    "fRSf"},
    {WINED3D_SM4_OP_SQRT,                             WINED3DSIH_SQRT,                             "f",    "f"},
    {WINED3D_SM4_OP_SWITCH,                           WINED3DSIH_SWITCH,                           "",     "u"},
    {WINED3D_SM4_OP_SINCOS,                           WINED3DSIH_SINCOS,                           "ff",   "f"},
    {WINED3D_SM4_OP_UDIV,                             WINED3DSIH_UDIV,                             "uu",   "uu"},
    {WINED3D_SM4_OP_ULT,                              WINED3DSIH_ULT,                              "u",    "uu"},
    {WINED3D_SM4_OP_UGE,                              WINED3DSIH_UGE,                              "u",    "uu"},
    {WINED3D_SM4_OP_UMUL,                             WINED3DSIH_UMUL,                             "uu",   "uu"},
    {WINED3D_SM4_OP_UMAX,                             WINED3DSIH_UMAX,                             "u",    "uu"},
    {WINED3D_SM4_OP_UMIN,                             WINED3DSIH_UMIN,                             "u",    "uu"},
    {WINED3D_SM4_OP_USHR,                             WINED3DSIH_USHR,                             "u",    "uu"},
    {WINED3D_SM4_OP_UTOF,                             WINED3DSIH_UTOF,                             "f",    "u"},
    {WINED3D_SM4_OP_XOR,                              WINED3DSIH_XOR,                              "u",    "uu"},
    {WINED3D_SM4_OP_DCL_RESOURCE,                     WINED3DSIH_DCL,                              "R",    "",
            shader_sm4_read_dcl_resource},
    {WINED3D_SM4_OP_DCL_CONSTANT_BUFFER,              WINED3DSIH_DCL_CONSTANT_BUFFER,              "",     "",
            shader_sm4_read_dcl_constant_buffer},
    {WINED3D_SM4_OP_DCL_SAMPLER,                      WINED3DSIH_DCL_SAMPLER,                      "",     "",
            shader_sm4_read_dcl_sampler},
    {WINED3D_SM4_OP_DCL_INDEX_RANGE,                  WINED3DSIH_DCL_INDEX_RANGE,                  "",     "",
            shader_sm4_read_dcl_index_range},
    {WINED3D_SM4_OP_DCL_OUTPUT_TOPOLOGY,              WINED3DSIH_DCL_OUTPUT_TOPOLOGY,              "",     "",
            shader_sm4_read_dcl_output_topology},
    {WINED3D_SM4_OP_DCL_INPUT_PRIMITIVE,              WINED3DSIH_DCL_INPUT_PRIMITIVE,              "",     "",
            shader_sm4_read_dcl_input_primitive},
    {WINED3D_SM4_OP_DCL_VERTICES_OUT,                 WINED3DSIH_DCL_VERTICES_OUT,                 "",     "",
            shader_sm4_read_declaration_count},
    {WINED3D_SM4_OP_DCL_INPUT,                        WINED3DSIH_DCL_INPUT,                        "",     "",
            shader_sm4_read_declaration_dst},
    {WINED3D_SM4_OP_DCL_INPUT_SGV,                    WINED3DSIH_DCL_INPUT_SGV,                    "",     "",
            shader_sm4_read_declaration_register_semantic},
    {WINED3D_SM4_OP_DCL_INPUT_SIV,                    WINED3DSIH_DCL_INPUT_SIV,                    "",     "",
            shader_sm4_read_declaration_register_semantic},
    {WINED3D_SM4_OP_DCL_INPUT_PS,                     WINED3DSIH_DCL_INPUT_PS,                     "",     "",
            shader_sm4_read_dcl_input_ps},
    {WINED3D_SM4_OP_DCL_INPUT_PS_SGV,                 WINED3DSIH_DCL_INPUT_PS_SGV,                 "",     "",
            shader_sm4_read_declaration_register_semantic},
    {WINED3D_SM4_OP_DCL_INPUT_PS_SIV,                 WINED3DSIH_DCL_INPUT_PS_SIV,                 "",     "",
            shader_sm4_read_dcl_input_ps_siv},
    {WINED3D_SM4_OP_DCL_OUTPUT,                       WINED3DSIH_DCL_OUTPUT,                       "",     "",
            shader_sm4_read_declaration_dst},
    {WINED3D_SM4_OP_DCL_OUTPUT_SIV,                   WINED3DSIH_DCL_OUTPUT_SIV,                   "",     "",
            shader_sm4_read_declaration_register_semantic},
    {WINED3D_SM4_OP_DCL_TEMPS,                        WINED3DSIH_DCL_TEMPS,                        "",     "",
            shader_sm4_read_declaration_count},
    {WINED3D_SM4_OP_DCL_INDEXABLE_TEMP,               WINED3DSIH_DCL_INDEXABLE_TEMP,               "",     "",
            shader_sm4_read_dcl_indexable_temp},
    {WINED3D_SM4_OP_DCL_GLOBAL_FLAGS,                 WINED3DSIH_DCL_GLOBAL_FLAGS,                 "",     "",
            shader_sm4_read_dcl_global_flags},
    {WINED3D_SM4_OP_LOD,                              WINED3DSIH_LOD,                              "f",    "fRS"},
    {WINED3D_SM4_OP_GATHER4,                          WINED3DSIH_GATHER4,                          "u",    "fRS"},
    {WINED3D_SM4_OP_SAMPLE_POS,                       WINED3DSIH_SAMPLE_POS,                       "f",    "Ru"},
    {WINED3D_SM4_OP_SAMPLE_INFO,                      WINED3DSIH_SAMPLE_INFO,                      "f",    "R"},
    {WINED3D_SM5_OP_HS_DECLS,                         WINED3DSIH_HS_DECLS,                         "",     ""},
    {WINED3D_SM5_OP_HS_CONTROL_POINT_PHASE,           WINED3DSIH_HS_CONTROL_POINT_PHASE,           "",     ""},
    {WINED3D_SM5_OP_HS_FORK_PHASE,                    WINED3DSIH_HS_FORK_PHASE,                    "",     ""},
    {WINED3D_SM5_OP_HS_JOIN_PHASE,                    WINED3DSIH_HS_JOIN_PHASE,                    "",     ""},
    {WINED3D_SM5_OP_EMIT_STREAM,                      WINED3DSIH_EMIT_STREAM,                      "",     "f"},
    {WINED3D_SM5_OP_CUT_STREAM,                       WINED3DSIH_CUT_STREAM,                       "",     "f"},
    {WINED3D_SM5_OP_FCALL,                            WINED3DSIH_FCALL,                            "",     "O",
            shader_sm5_read_fcall},
    {WINED3D_SM5_OP_BUFINFO,                          WINED3DSIH_BUFINFO,                          "i",    "U"},
    {WINED3D_SM5_OP_DERIV_RTX_COARSE,                 WINED3DSIH_DSX_COARSE,                       "f",    "f"},
    {WINED3D_SM5_OP_DERIV_RTX_FINE,                   WINED3DSIH_DSX_FINE,                         "f",    "f"},
    {WINED3D_SM5_OP_DERIV_RTY_COARSE,                 WINED3DSIH_DSY_COARSE,                       "f",    "f"},
    {WINED3D_SM5_OP_DERIV_RTY_FINE,                   WINED3DSIH_DSY_FINE,                         "f",    "f"},
    {WINED3D_SM5_OP_GATHER4_C,                        WINED3DSIH_GATHER4_C,                        "f",    "fRSf"},
    {WINED3D_SM5_OP_GATHER4_PO,                       WINED3DSIH_GATHER4_PO,                       "f",    "fiRS"},
    {WINED3D_SM5_OP_GATHER4_PO_C,                     WINED3DSIH_GATHER4_PO_C,                     "f",    "fiRSf"},
    {WINED3D_SM5_OP_RCP,                              WINED3DSIH_RCP,                              "f",    "f"},
    {WINED3D_SM5_OP_F32TOF16,                         WINED3DSIH_F32TOF16,                         "u",    "f"},
    {WINED3D_SM5_OP_F16TOF32,                         WINED3DSIH_F16TOF32,                         "f",    "u"},
    {WINED3D_SM5_OP_COUNTBITS,                        WINED3DSIH_COUNTBITS,                        "u",    "u"},
    {WINED3D_SM5_OP_FIRSTBIT_HI,                      WINED3DSIH_FIRSTBIT_HI,                      "u",    "u"},
    {WINED3D_SM5_OP_FIRSTBIT_LO,                      WINED3DSIH_FIRSTBIT_LO,                      "u",    "u"},
    {WINED3D_SM5_OP_FIRSTBIT_SHI,                     WINED3DSIH_FIRSTBIT_SHI,                     "u",    "i"},
    {WINED3D_SM5_OP_UBFE,                             WINED3DSIH_UBFE,                             "u",    "iiu"},
    {WINED3D_SM5_OP_IBFE,                             WINED3DSIH_IBFE,                             "i",    "iii"},
    {WINED3D_SM5_OP_BFI,                              WINED3DSIH_BFI,                              "u",    "iiuu"},
    {WINED3D_SM5_OP_BFREV,                            WINED3DSIH_BFREV,                            "u",    "u"},
    {WINED3D_SM5_OP_SWAPC,                            WINED3DSIH_SWAPC,                            "ff",   "uff"},
    {WINED3D_SM5_OP_DCL_STREAM,                       WINED3DSIH_DCL_STREAM,                       "",     "O"},
    {WINED3D_SM5_OP_DCL_FUNCTION_BODY,                WINED3DSIH_DCL_FUNCTION_BODY,                "",     "",
            shader_sm5_read_dcl_function_body},
    {WINED3D_SM5_OP_DCL_FUNCTION_TABLE,               WINED3DSIH_DCL_FUNCTION_TABLE,               "",     "",
            shader_sm5_read_dcl_function_table},
    {WINED3D_SM5_OP_DCL_INTERFACE,                    WINED3DSIH_DCL_INTERFACE,                    "",     "",
            shader_sm5_read_dcl_interface},
    {WINED3D_SM5_OP_DCL_INPUT_CONTROL_POINT_COUNT,    WINED3DSIH_DCL_INPUT_CONTROL_POINT_COUNT,    "",     "",
            shader_sm5_read_control_point_count},
    {WINED3D_SM5_OP_DCL_OUTPUT_CONTROL_POINT_COUNT,   WINED3DSIH_DCL_OUTPUT_CONTROL_POINT_COUNT,   "",     "",
            shader_sm5_read_control_point_count},
    {WINED3D_SM5_OP_DCL_TESSELLATOR_DOMAIN,           WINED3DSIH_DCL_TESSELLATOR_DOMAIN,           "",     "",
            shader_sm5_read_dcl_tessellator_domain},
    {WINED3D_SM5_OP_DCL_TESSELLATOR_PARTITIONING,     WINED3DSIH_DCL_TESSELLATOR_PARTITIONING,     "",     "",
            shader_sm5_read_dcl_tessellator_partitioning},
    {WINED3D_SM5_OP_DCL_TESSELLATOR_OUTPUT_PRIMITIVE, WINED3DSIH_DCL_TESSELLATOR_OUTPUT_PRIMITIVE, "",     "",
            shader_sm5_read_dcl_tessellator_output_primitive},
    {WINED3D_SM5_OP_DCL_HS_MAX_TESSFACTOR,            WINED3DSIH_DCL_HS_MAX_TESSFACTOR,            "",     "",
            shader_sm5_read_dcl_hs_max_tessfactor},
    {WINED3D_SM5_OP_DCL_HS_FORK_PHASE_INSTANCE_COUNT, WINED3DSIH_DCL_HS_FORK_PHASE_INSTANCE_COUNT, "",     "",
            shader_sm4_read_declaration_count},
    {WINED3D_SM5_OP_DCL_HS_JOIN_PHASE_INSTANCE_COUNT, WINED3DSIH_DCL_HS_JOIN_PHASE_INSTANCE_COUNT, "",     "",
            shader_sm4_read_declaration_count},
    {WINED3D_SM5_OP_DCL_THREAD_GROUP,                 WINED3DSIH_DCL_THREAD_GROUP,                 "",     "",
            shader_sm5_read_dcl_thread_group},
    {WINED3D_SM5_OP_DCL_UAV_TYPED,                    WINED3DSIH_DCL_UAV_TYPED,                    "",     "",
            shader_sm4_read_dcl_resource},
    {WINED3D_SM5_OP_DCL_UAV_RAW,                      WINED3DSIH_DCL_UAV_RAW,                      "",     "",
            shader_sm5_read_dcl_uav_raw},
    {WINED3D_SM5_OP_DCL_UAV_STRUCTURED,               WINED3DSIH_DCL_UAV_STRUCTURED,               "",     "",
            shader_sm5_read_dcl_uav_structured},
    {WINED3D_SM5_OP_DCL_TGSM_RAW,                     WINED3DSIH_DCL_TGSM_RAW,                     "",     "",
            shader_sm5_read_dcl_tgsm_raw},
    {WINED3D_SM5_OP_DCL_TGSM_STRUCTURED,              WINED3DSIH_DCL_TGSM_STRUCTURED,              "",     "",
            shader_sm5_read_dcl_tgsm_structured},
    {WINED3D_SM5_OP_DCL_RESOURCE_RAW,                 WINED3DSIH_DCL_RESOURCE_RAW,                 "",     "",
            shader_sm5_read_dcl_resource_raw},
    {WINED3D_SM5_OP_DCL_RESOURCE_STRUCTURED,          WINED3DSIH_DCL_RESOURCE_STRUCTURED,          "",     "",
            shader_sm5_read_dcl_resource_structured},
    {WINED3D_SM5_OP_LD_UAV_TYPED,                     WINED3DSIH_LD_UAV_TYPED,                     "u",    "iU"},
    {WINED3D_SM5_OP_STORE_UAV_TYPED,                  WINED3DSIH_STORE_UAV_TYPED,                  "U",    "iu"},
    {WINED3D_SM5_OP_LD_RAW,                           WINED3DSIH_LD_RAW,                           "u",    "iU"},
    {WINED3D_SM5_OP_STORE_RAW,                        WINED3DSIH_STORE_RAW,                        "U",    "iu"},
    {WINED3D_SM5_OP_LD_STRUCTURED,                    WINED3DSIH_LD_STRUCTURED,                    "u",    "iiR"},
    {WINED3D_SM5_OP_STORE_STRUCTURED,                 WINED3DSIH_STORE_STRUCTURED,                 "U",    "iiu"},
    {WINED3D_SM5_OP_ATOMIC_AND,                       WINED3DSIH_ATOMIC_AND,                       "U",    "iu"},
    {WINED3D_SM5_OP_ATOMIC_OR,                        WINED3DSIH_ATOMIC_OR,                        "U",    "iu"},
    {WINED3D_SM5_OP_ATOMIC_XOR,                       WINED3DSIH_ATOMIC_XOR,                       "U",    "iu"},
    {WINED3D_SM5_OP_ATOMIC_CMP_STORE,                 WINED3DSIH_ATOMIC_CMP_STORE,                 "U",    "iuu"},
    {WINED3D_SM5_OP_ATOMIC_IADD,                      WINED3DSIH_ATOMIC_IADD,                      "U",    "ii"},
    {WINED3D_SM5_OP_ATOMIC_IMAX,                      WINED3DSIH_ATOMIC_IMAX,                      "U",    "ii"},
    {WINED3D_SM5_OP_ATOMIC_IMIN,                      WINED3DSIH_ATOMIC_IMIN,                      "U",    "ii"},
    {WINED3D_SM5_OP_ATOMIC_UMAX,                      WINED3DSIH_ATOMIC_UMAX,                      "U",    "iu"},
    {WINED3D_SM5_OP_ATOMIC_UMIN,                      WINED3DSIH_ATOMIC_UMIN,                      "U",    "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_ALLOC,                 WINED3DSIH_IMM_ATOMIC_ALLOC,                 "u",    "U"},
    {WINED3D_SM5_OP_IMM_ATOMIC_CONSUME,               WINED3DSIH_IMM_ATOMIC_CONSUME,               "u",    "U"},
    {WINED3D_SM5_OP_IMM_ATOMIC_IADD,                  WINED3DSIH_IMM_ATOMIC_IADD,                  "uU",   "ii"},
    {WINED3D_SM5_OP_IMM_ATOMIC_AND,                   WINED3DSIH_IMM_ATOMIC_AND,                   "uU",   "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_OR,                    WINED3DSIH_IMM_ATOMIC_OR,                    "uU",   "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_XOR,                   WINED3DSIH_IMM_ATOMIC_XOR,                   "uU",   "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_EXCH,                  WINED3DSIH_IMM_ATOMIC_EXCH,                  "uU",   "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_CMP_EXCH,              WINED3DSIH_IMM_ATOMIC_CMP_EXCH,              "uU",   "iuu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_IMAX,                  WINED3DSIH_IMM_ATOMIC_IMAX,                  "iU",   "ii"},
    {WINED3D_SM5_OP_IMM_ATOMIC_IMIN,                  WINED3DSIH_IMM_ATOMIC_IMIN,                  "iU",   "ii"},
    {WINED3D_SM5_OP_IMM_ATOMIC_UMAX,                  WINED3DSIH_IMM_ATOMIC_UMAX,                  "uU",   "iu"},
    {WINED3D_SM5_OP_IMM_ATOMIC_UMIN,                  WINED3DSIH_IMM_ATOMIC_UMIN,                  "uU",   "iu"},
    {WINED3D_SM5_OP_SYNC,                             WINED3DSIH_SYNC,                             "",     "",
            shader_sm5_read_sync},
    {WINED3D_SM5_OP_EVAL_SAMPLE_INDEX,                WINED3DSIH_EVAL_SAMPLE_INDEX,                "f",    "fi"},
    {WINED3D_SM5_OP_EVAL_CENTROID,                    WINED3DSIH_EVAL_CENTROID,                    "f",    "f"},
    {WINED3D_SM5_OP_DCL_GS_INSTANCES,                 WINED3DSIH_DCL_GS_INSTANCES,                 "",     "",
            shader_sm4_read_declaration_count},
};

static const enum wined3d_shader_register_type register_type_table[] =
{
    /* WINED3D_SM4_RT_TEMP */                    WINED3DSPR_TEMP,
    /* WINED3D_SM4_RT_INPUT */                   WINED3DSPR_INPUT,
    /* WINED3D_SM4_RT_OUTPUT */                  WINED3DSPR_OUTPUT,
    /* WINED3D_SM4_RT_INDEXABLE_TEMP */          WINED3DSPR_IDXTEMP,
    /* WINED3D_SM4_RT_IMMCONST */                WINED3DSPR_IMMCONST,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM4_RT_SAMPLER */                 WINED3DSPR_SAMPLER,
    /* WINED3D_SM4_RT_RESOURCE */                WINED3DSPR_RESOURCE,
    /* WINED3D_SM4_RT_CONSTBUFFER */             WINED3DSPR_CONSTBUFFER,
    /* WINED3D_SM4_RT_IMMCONSTBUFFER */          WINED3DSPR_IMMCONSTBUFFER,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM4_RT_PRIMID */                  WINED3DSPR_PRIMID,
    /* WINED3D_SM4_RT_DEPTHOUT */                WINED3DSPR_DEPTHOUT,
    /* WINED3D_SM4_RT_NULL */                    WINED3DSPR_NULL,
    /* WINED3D_SM4_RT_RASTERIZER */              WINED3DSPR_RASTERIZER,
    /* WINED3D_SM4_RT_OMASK */                   WINED3DSPR_SAMPLEMASK,
    /* WINED3D_SM5_RT_STREAM */                  WINED3DSPR_STREAM,
    /* WINED3D_SM5_RT_FUNCTION_BODY */           WINED3DSPR_FUNCTIONBODY,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM5_RT_FUNCTION_POINTER */        WINED3DSPR_FUNCTIONPOINTER,
    /* UNKNOWN */                                ~0u,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM5_RT_OUTPUT_CONTROL_POINT_ID */ WINED3DSPR_OUTPOINTID,
    /* WINED3D_SM5_RT_FORK_INSTANCE_ID */        WINED3DSPR_FORKINSTID,
    /* WINED3D_SM5_RT_JOIN_INSTANCE_ID */        WINED3DSPR_JOININSTID,
    /* WINED3D_SM5_RT_INPUT_CONTROL_POINT */     WINED3DSPR_INCONTROLPOINT,
    /* WINED3D_SM5_RT_OUTPUT_CONTROL_POINT */    WINED3DSPR_OUTCONTROLPOINT,
    /* WINED3D_SM5_RT_PATCH_CONSTANT_DATA */     WINED3DSPR_PATCHCONST,
    /* WINED3D_SM5_RT_DOMAIN_LOCATION */         WINED3DSPR_TESSCOORD,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM5_RT_UAV */                     WINED3DSPR_UAV,
    /* WINED3D_SM5_RT_SHARED_MEMORY */           WINED3DSPR_GROUPSHAREDMEM,
    /* WINED3D_SM5_RT_THREAD_ID */               WINED3DSPR_THREADID,
    /* WINED3D_SM5_RT_THREAD_GROUP_ID */         WINED3DSPR_THREADGROUPID,
    /* WINED3D_SM5_RT_LOCAL_THREAD_ID */         WINED3DSPR_LOCALTHREADID,
    /* WINED3D_SM5_RT_COVERAGE */                WINED3DSPR_COVERAGE,
    /* WINED3D_SM5_RT_LOCAL_THREAD_INDEX */      WINED3DSPR_LOCALTHREADINDEX,
    /* WINED3D_SM5_RT_GS_INSTANCE_ID */          WINED3DSPR_GSINSTID,
    /* WINED3D_SM5_RT_DEPTHOUT_GREATER_EQUAL */  WINED3DSPR_DEPTHOUTGE,
    /* WINED3D_SM5_RT_DEPTHOUT_LESS_EQUAL */     WINED3DSPR_DEPTHOUTLE,
    /* UNKNOWN */                                ~0u,
    /* WINED3D_SM5_RT_OUTPUT_STENCIL_REF */      WINED3DSPR_STENCILREF,
};

static const struct wined3d_sm4_opcode_info *get_opcode_info(enum wined3d_sm4_opcode opcode)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(opcode_table); ++i)
    {
        if (opcode == opcode_table[i].opcode) return &opcode_table[i];
    }

    return NULL;
}

static void map_register(const struct wined3d_sm4_data *priv, struct wined3d_shader_register *reg)
{
    switch (priv->shader_version.type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            if (reg->type == WINED3DSPR_OUTPUT)
            {
                unsigned int reg_idx = reg->idx[0].offset;

                if (reg_idx >= ARRAY_SIZE(priv->output_map))
                {
                    ERR("Invalid output index %u.\n", reg_idx);
                    break;
                }

                reg->type = WINED3DSPR_COLOROUT;
                reg->idx[0].offset = priv->output_map[reg_idx];
            }
            break;

        default:
            break;
    }
}

static enum wined3d_data_type map_data_type(char t)
{
    switch (t)
    {
        case 'f':
            return WINED3D_DATA_FLOAT;
        case 'i':
            return WINED3D_DATA_INT;
        case 'u':
            return WINED3D_DATA_UINT;
        case 'O':
            return WINED3D_DATA_OPAQUE;
        case 'R':
            return WINED3D_DATA_RESOURCE;
        case 'S':
            return WINED3D_DATA_SAMPLER;
        case 'U':
            return WINED3D_DATA_UAV;
        default:
            ERR("Invalid data type '%c'.\n", t);
            return WINED3D_DATA_FLOAT;
    }
}

static enum wined3d_shader_type wined3d_get_sm4_shader_type(const DWORD *byte_code, size_t byte_code_size)
{
    unsigned int shader_type;

    if (byte_code_size / sizeof(*byte_code) < 1)
    {
        WARN("Invalid byte code size %lu.\n", (long)byte_code_size);
        return WINED3D_SHADER_TYPE_INVALID;
    }

    shader_type = byte_code[0] >> 16;
    switch (shader_type)
    {
        case WINED3D_SM4_PS:
            return WINED3D_SHADER_TYPE_PIXEL;
            break;
        case WINED3D_SM4_VS:
            return WINED3D_SHADER_TYPE_VERTEX;
            break;
        case WINED3D_SM4_GS:
            return WINED3D_SHADER_TYPE_GEOMETRY;
            break;
        case WINED3D_SM5_HS:
            return WINED3D_SHADER_TYPE_HULL;
            break;
        case WINED3D_SM5_DS:
            return WINED3D_SHADER_TYPE_DOMAIN;
            break;
        case WINED3D_SM5_CS:
            return WINED3D_SHADER_TYPE_COMPUTE;
            break;
        default:
            FIXME("Unrecognised shader type %#x.\n", shader_type);
            return WINED3D_SHADER_TYPE_INVALID;
    }
}

static void *shader_sm4_init(const DWORD *byte_code, size_t byte_code_size,
        const struct wined3d_shader_signature *output_signature)
{
    unsigned int version_token, token_count;
    struct wined3d_sm4_data *priv;
    unsigned int i;

    if (byte_code_size / sizeof(*byte_code) < 2)
    {
        WARN("Invalid byte code size %lu.\n", (long)byte_code_size);
        return NULL;
    }

    version_token = byte_code[0];
    TRACE("Version: 0x%08x.\n", version_token);
    token_count = byte_code[1];
    TRACE("Token count: %u.\n", token_count);

    if (token_count < 2 || byte_code_size / sizeof(*byte_code) < token_count)
    {
        WARN("Invalid token count %u.\n", token_count);
        return NULL;
    }

    if (!(priv = malloc(sizeof(*priv))))
    {
        ERR("Failed to allocate private data\n");
        return NULL;
    }

    priv->start = &byte_code[2];
    priv->end = &byte_code[token_count];

    priv->shader_version.type = wined3d_get_sm4_shader_type(byte_code, byte_code_size);
    if (priv->shader_version.type == WINED3D_SHADER_TYPE_INVALID)
    {
        free(priv);
        return NULL;
    }

    priv->shader_version.major = WINED3D_SM4_VERSION_MAJOR(version_token);
    priv->shader_version.minor = WINED3D_SM4_VERSION_MINOR(version_token);

    memset(priv->output_map, 0xff, sizeof(priv->output_map));
    for (i = 0; i < output_signature->element_count; ++i)
    {
        struct wined3d_shader_signature_element *e = &output_signature->elements[i];

        if (priv->shader_version.type == WINED3D_SHADER_TYPE_PIXEL
                && stricmp(e->semantic_name, "SV_TARGET"))
            continue;
        if (e->register_idx >= ARRAY_SIZE(priv->output_map))
        {
            WARN("Invalid output index %u.\n", e->register_idx);
            continue;
        }

        priv->output_map[e->register_idx] = e->semantic_idx;
    }

    list_init(&priv->src_free);
    list_init(&priv->src);

    return priv;
}

static void shader_sm4_free(void *data)
{
    struct wined3d_shader_src_param_entry *e1, *e2;
    struct wined3d_sm4_data *priv = data;

    list_move_head(&priv->src_free, &priv->src);
    LIST_FOR_EACH_ENTRY_SAFE(e1, e2, &priv->src_free, struct wined3d_shader_src_param_entry, entry)
    {
        free(e1);
    }
    free(priv);
}

static struct wined3d_shader_src_param *get_src_param(struct wined3d_sm4_data *priv)
{
    struct wined3d_shader_src_param_entry *e;
    struct list *elem;

    if (!list_empty(&priv->src_free))
    {
        elem = list_head(&priv->src_free);
        list_remove(elem);
    }
    else
    {
        if (!(e = malloc(sizeof(*e))))
            return NULL;
        elem = &e->entry;
    }

    list_add_tail(&priv->src, elem);
    e = LIST_ENTRY(elem, struct wined3d_shader_src_param_entry, entry);
    return &e->param;
}

static void shader_sm4_read_header(void *data, const DWORD **ptr, struct wined3d_shader_version *shader_version)
{
    struct wined3d_sm4_data *priv = data;

    *ptr = priv->start;
    *shader_version = priv->shader_version;
}

static BOOL shader_sm4_read_reg_idx(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        DWORD addressing, struct wined3d_shader_register_index *reg_idx)
{
    if (addressing & WINED3D_SM4_ADDRESSING_RELATIVE)
    {
        struct wined3d_shader_src_param *rel_addr = get_src_param(priv);

        if (!(reg_idx->rel_addr = rel_addr))
        {
            ERR("Failed to get src param for relative addressing.\n");
            return FALSE;
        }

        if (addressing & WINED3D_SM4_ADDRESSING_OFFSET)
            reg_idx->offset = *(*ptr)++;
        else
            reg_idx->offset = 0;
        shader_sm4_read_src_param(priv, ptr, end, WINED3D_DATA_INT, rel_addr);
    }
    else
    {
        reg_idx->rel_addr = NULL;
        reg_idx->offset = *(*ptr)++;
    }

    return TRUE;
}

static BOOL shader_sm4_read_param(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        enum wined3d_data_type data_type, struct wined3d_shader_register *param,
        enum wined3d_shader_src_modifier *modifier)
{
    enum wined3d_sm4_register_type register_type;
    uint32_t token, order;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return FALSE;
    }
    token = *(*ptr)++;

    register_type = (token & WINED3D_SM4_REGISTER_TYPE_MASK) >> WINED3D_SM4_REGISTER_TYPE_SHIFT;
    if (register_type >= ARRAY_SIZE(register_type_table)
            || register_type_table[register_type] == ~0u)
    {
        FIXME("Unhandled register type %#x.\n", register_type);
        param->type = WINED3DSPR_TEMP;
    }
    else
    {
        param->type = register_type_table[register_type];
    }
    param->data_type = data_type;

    if (token & WINED3D_SM4_REGISTER_MODIFIER)
    {
        unsigned int m;

        if (*ptr >= end)
        {
            WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
            return FALSE;
        }
        m = *(*ptr)++;

        switch (m)
        {
            case 0x41:
                *modifier = WINED3DSPSM_NEG;
                break;

            case 0x81:
                *modifier = WINED3DSPSM_ABS;
                break;

            case 0xc1:
                *modifier = WINED3DSPSM_ABSNEG;
                break;

            default:
                FIXME("Skipping modifier 0x%08x.\n", m);
            case 0x01:
                *modifier = WINED3DSPSM_NONE;
                break;
        }
    }
    else
    {
        *modifier = WINED3DSPSM_NONE;
    }

    order = (token & WINED3D_SM4_REGISTER_ORDER_MASK) >> WINED3D_SM4_REGISTER_ORDER_SHIFT;

    if (order < 1)
        param->idx[0].offset = ~0U;
    else
    {
        DWORD addressing = (token & WINED3D_SM4_ADDRESSING_MASK0) >> WINED3D_SM4_ADDRESSING_SHIFT0;
        if (!(shader_sm4_read_reg_idx(priv, ptr, end, addressing, &param->idx[0])))
        {
            ERR("Failed to read register index.\n");
            return FALSE;
        }
    }

    if (order < 2)
        param->idx[1].offset = ~0U;
    else
    {
        DWORD addressing = (token & WINED3D_SM4_ADDRESSING_MASK1) >> WINED3D_SM4_ADDRESSING_SHIFT1;
        if (!(shader_sm4_read_reg_idx(priv, ptr, end, addressing, &param->idx[1])))
        {
            ERR("Failed to read register index.\n");
            return FALSE;
        }
    }

    if (order > 2)
        FIXME("Unhandled order %u.\n", order);

    if (register_type == WINED3D_SM4_RT_IMMCONST)
    {
        enum wined3d_sm4_dimension dimension = (token & WINED3D_SM4_DIMENSION_MASK) >> WINED3D_SM4_DIMENSION_SHIFT;

        switch (dimension)
        {
            case WINED3D_SM4_DIMENSION_SCALAR:
                param->immconst_type = WINED3D_IMMCONST_SCALAR;
                if (end - *ptr < 1)
                {
                    WARN("Invalid ptr %p, end %p.\n", *ptr, end);
                    return FALSE;
                }
                memcpy(param->u.immconst_data, *ptr, 1 * sizeof(DWORD));
                *ptr += 1;
                break;

            case WINED3D_SM4_DIMENSION_VEC4:
                param->immconst_type = WINED3D_IMMCONST_VEC4;
                if (end - *ptr < 4)
                {
                    WARN("Invalid ptr %p, end %p.\n", *ptr, end);
                    return FALSE;
                }
                memcpy(param->u.immconst_data, *ptr, 4 * sizeof(DWORD));
                *ptr += 4;
                break;

            default:
                FIXME("Unhandled dimension %#x.\n", dimension);
                break;
        }
    }

    map_register(priv, param);

    return TRUE;
}

static BOOL shader_sm4_read_src_param(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        enum wined3d_data_type data_type, struct wined3d_shader_src_param *src_param)
{
    DWORD token;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return FALSE;
    }
    token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, end, data_type, &src_param->reg, &src_param->modifiers))
    {
        ERR("Failed to read parameter.\n");
        return FALSE;
    }

    if (src_param->reg.type == WINED3DSPR_IMMCONST)
    {
        src_param->swizzle = WINED3DSP_NOSWIZZLE;
    }
    else
    {
        enum wined3d_sm4_swizzle_type swizzle_type =
                (token & WINED3D_SM4_SWIZZLE_TYPE_MASK) >> WINED3D_SM4_SWIZZLE_TYPE_SHIFT;

        switch (swizzle_type)
        {
            case WINED3D_SM4_SWIZZLE_NONE:
                src_param->swizzle = WINED3DSP_NOSWIZZLE;
                break;

            case WINED3D_SM4_SWIZZLE_SCALAR:
                src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
                src_param->swizzle = (src_param->swizzle & 0x3) * 0x55;
                break;

            case WINED3D_SM4_SWIZZLE_VEC4:
                src_param->swizzle = (token & WINED3D_SM4_SWIZZLE_MASK) >> WINED3D_SM4_SWIZZLE_SHIFT;
                break;

            default:
                FIXME("Unhandled swizzle type %#x.\n", swizzle_type);
                break;
        }
    }

    return TRUE;
}

static BOOL shader_sm4_read_dst_param(struct wined3d_sm4_data *priv, const DWORD **ptr, const DWORD *end,
        enum wined3d_data_type data_type, struct wined3d_shader_dst_param *dst_param)
{
    enum wined3d_shader_src_modifier modifier;
    DWORD token;

    if (*ptr >= end)
    {
        WARN("Invalid ptr %p >= end %p.\n", *ptr, end);
        return FALSE;
    }
    token = **ptr;

    if (!shader_sm4_read_param(priv, ptr, end, data_type, &dst_param->reg, &modifier))
    {
        ERR("Failed to read parameter.\n");
        return FALSE;
    }

    if (modifier != WINED3DSPSM_NONE)
    {
        ERR("Invalid source modifier %#x on destination register.\n", modifier);
        return FALSE;
    }

    dst_param->write_mask = (token & WINED3D_SM4_WRITEMASK_MASK) >> WINED3D_SM4_WRITEMASK_SHIFT;
    dst_param->modifiers = 0;
    dst_param->shift = 0;

    return TRUE;
}

static void shader_sm4_read_instruction_modifier(uint32_t modifier, struct wined3d_shader_instruction *ins)
{
    enum wined3d_sm4_instruction_modifier modifier_type = modifier & WINED3D_SM4_MODIFIER_MASK;

    switch (modifier_type)
    {
        case WINED3D_SM4_MODIFIER_AOFFIMMI:
        {
            static const DWORD recognized_bits = WINED3D_SM4_INSTRUCTION_MODIFIER
                    | WINED3D_SM4_MODIFIER_MASK
                    | WINED3D_SM4_AOFFIMMI_U_MASK
                    | WINED3D_SM4_AOFFIMMI_V_MASK
                    | WINED3D_SM4_AOFFIMMI_W_MASK;

            /* Bit fields are used for sign extension. */
            struct
            {
                int u : 4;
                int v : 4;
                int w : 4;
            } aoffimmi;

            if (modifier & ~recognized_bits)
                FIXME("Unhandled instruction modifier %#x.\n", modifier);

            aoffimmi.u = (modifier & WINED3D_SM4_AOFFIMMI_U_MASK) >> WINED3D_SM4_AOFFIMMI_U_SHIFT;
            aoffimmi.v = (modifier & WINED3D_SM4_AOFFIMMI_V_MASK) >> WINED3D_SM4_AOFFIMMI_V_SHIFT;
            aoffimmi.w = (modifier & WINED3D_SM4_AOFFIMMI_W_MASK) >> WINED3D_SM4_AOFFIMMI_W_SHIFT;
            ins->texel_offset.u = aoffimmi.u;
            ins->texel_offset.v = aoffimmi.v;
            ins->texel_offset.w = aoffimmi.w;
            break;
        }

        case WINED3D_SM5_MODIFIER_DATA_TYPE:
        {
            uint32_t components = (modifier & WINED3D_SM5_MODIFIER_DATA_TYPE_MASK) >> WINED3D_SM5_MODIFIER_DATA_TYPE_SHIFT;
            enum wined3d_sm4_data_type data_type = components & 0xf;

            if ((components & 0xfff0) != (components & 0xf) * 0x1110)
                FIXME("Components (%#x) have different data types.\n", components);
            ins->resource_data_type = data_type_table[data_type];
            break;
        }

        case WINED3D_SM5_MODIFIER_RESOURCE_TYPE:
        {
            enum wined3d_sm4_resource_type resource_type
                    = (modifier & WINED3D_SM5_MODIFIER_RESOURCE_TYPE_MASK) >> WINED3D_SM5_MODIFIER_RESOURCE_TYPE_SHIFT;

            ins->resource_type = resource_type_table[resource_type];
            break;
        }

        default:
            FIXME("Unhandled instruction modifier %#x.\n", modifier);
    }
}

static void shader_sm4_read_instruction(void *data, const DWORD **ptr, struct wined3d_shader_instruction *ins)
{
    const struct wined3d_sm4_opcode_info *opcode_info;
    uint32_t opcode_token, previous_token;
    struct wined3d_sm4_data *priv = data;
    unsigned int opcode;
    unsigned int i, len;
    SIZE_T remaining;
    const DWORD *p;
    DWORD precise;

    list_move_head(&priv->src_free, &priv->src);

    if (*ptr >= priv->end)
    {
        WARN("End of byte-code, failed to read opcode.\n");
        goto fail;
    }
    remaining = priv->end - *ptr;

    opcode_token = *(*ptr)++;
    opcode = opcode_token & WINED3D_SM4_OPCODE_MASK;

    len = ((opcode_token & WINED3D_SM4_INSTRUCTION_LENGTH_MASK) >> WINED3D_SM4_INSTRUCTION_LENGTH_SHIFT);
    if (!len)
    {
        if (remaining < 2)
        {
            WARN("End of byte-code, failed to read length token.\n");
            goto fail;
        }
        len = **ptr;
    }
    if (!len || remaining < len)
    {
        WARN("Read invalid length %u (remaining %Iu).\n", len, remaining);
        goto fail;
    }
    --len;

    if (TRACE_ON(d3d_bytecode))
    {
        TRACE_(d3d_bytecode)("[ %08x ", opcode_token);
        for (i = 0; i < len; ++i)
        {
            TRACE_(d3d_bytecode)("%08lx ", (*ptr)[i]);
        }
        TRACE_(d3d_bytecode)("]\n");
    }

    if (!(opcode_info = get_opcode_info(opcode)))
    {
        FIXME("Unrecognized opcode %#x, opcode_token 0x%08x.\n", opcode, opcode_token);
        ins->handler_idx = WINED3DSIH_TABLE_SIZE;
        *ptr += len;
        return;
    }

    ins->handler_idx = opcode_info->handler_idx;
    ins->flags = 0;
    ins->coissue = 0;
    ins->predicate = NULL;
    ins->dst_count = strlen(opcode_info->dst_info);
    ins->dst = priv->dst_param;
    ins->src_count = strlen(opcode_info->src_info);
    ins->src = priv->src_param;
    ins->resource_type = WINED3D_SHADER_RESOURCE_NONE;
    ins->resource_data_type = WINED3D_DATA_FLOAT;
    memset(&ins->texel_offset, 0, sizeof(ins->texel_offset));

    p = *ptr;
    *ptr += len;

    if (opcode_info->read_opcode_func)
    {
        opcode_info->read_opcode_func(ins, opcode, opcode_token, p, len, priv);
    }
    else
    {
        enum wined3d_shader_dst_modifier instruction_dst_modifier = WINED3DSPDM_NONE;

        previous_token = opcode_token;
        while (previous_token & WINED3D_SM4_INSTRUCTION_MODIFIER && p != *ptr)
            shader_sm4_read_instruction_modifier(previous_token = *p++, ins);

        ins->flags = (opcode_token & WINED3D_SM4_INSTRUCTION_FLAGS_MASK) >> WINED3D_SM4_INSTRUCTION_FLAGS_SHIFT;
        if (ins->flags & WINED3D_SM4_INSTRUCTION_FLAG_SATURATE)
        {
            ins->flags &= ~WINED3D_SM4_INSTRUCTION_FLAG_SATURATE;
            instruction_dst_modifier = WINED3DSPDM_SATURATE;
        }
        precise = (opcode_token & WINED3D_SM5_PRECISE_MASK) >> WINED3D_SM5_PRECISE_SHIFT;
        ins->flags |= precise << WINED3DSI_PRECISE_SHIFT;

        for (i = 0; i < ins->dst_count; ++i)
        {
            if (!(shader_sm4_read_dst_param(priv, &p, *ptr, map_data_type(opcode_info->dst_info[i]),
                    &priv->dst_param[i])))
            {
                ins->handler_idx = WINED3DSIH_TABLE_SIZE;
                return;
            }
            priv->dst_param[i].modifiers |= instruction_dst_modifier;
        }

        for (i = 0; i < ins->src_count; ++i)
        {
            if (!(shader_sm4_read_src_param(priv, &p, *ptr, map_data_type(opcode_info->src_info[i]),
                    &priv->src_param[i])))
            {
                ins->handler_idx = WINED3DSIH_TABLE_SIZE;
                return;
            }
        }
    }

    return;

fail:
    *ptr = priv->end;
    ins->handler_idx = WINED3DSIH_TABLE_SIZE;
    return;
}

static BOOL shader_sm4_is_end(void *data, const DWORD **ptr)
{
    struct wined3d_sm4_data *priv = data;
    return *ptr == priv->end;
}

const struct wined3d_shader_frontend sm4_shader_frontend =
{
    shader_sm4_init,
    shader_sm4_free,
    shader_sm4_read_header,
    shader_sm4_read_instruction,
    shader_sm4_is_end,
};

#define TAG_AON9 WINEMAKEFOURCC('A', 'o', 'n', '9')
#define TAG_ISG1 WINEMAKEFOURCC('I', 'S', 'G', '1')
#define TAG_ISGN WINEMAKEFOURCC('I', 'S', 'G', 'N')
#define TAG_OSG1 WINEMAKEFOURCC('O', 'S', 'G', '1')
#define TAG_OSG5 WINEMAKEFOURCC('O', 'S', 'G', '5')
#define TAG_OSGN WINEMAKEFOURCC('O', 'S', 'G', 'N')
#define TAG_PCSG WINEMAKEFOURCC('P', 'C', 'S', 'G')
#define TAG_PSG1 WINEMAKEFOURCC('P', 'S', 'G', '1')
#define TAG_SHDR WINEMAKEFOURCC('S', 'H', 'D', 'R')
#define TAG_SHEX WINEMAKEFOURCC('S', 'H', 'E', 'X')

struct aon9_header
{
    DWORD chunk_size;
    unsigned int shader_version;
    DWORD unknown;
    unsigned int byte_code_offset;
};

static unsigned int read_dword(const char **ptr)
{
    unsigned int ret;
    memcpy(&ret, *ptr, sizeof(ret));
    *ptr += sizeof(ret);
    return ret;
}

static BOOL require_space(size_t offset, size_t count, size_t size, size_t data_size)
{
    return !count || (data_size - offset) / count >= size;
}

static void skip_dword_unknown(const char **ptr, unsigned int count)
{
    unsigned int i;
    unsigned int d;

    WARN("Skipping %u unknown DWORDs:\n", count);
    for (i = 0; i < count; ++i)
    {
        d = read_dword(ptr);
        WARN("\t0x%08x\n", d);
    }
}

static const char *shader_get_string(const char *data, size_t data_size, unsigned int offset)
{
    if (offset >= data_size)
    {
        WARN("Invalid offset %#x (data size %#lx).\n", offset, (long)data_size);
        return NULL;
    }

    if (!memchr( data + offset, 0, data_size - offset ))
        return NULL;

    return data + offset;
}

static HRESULT shader_parse_signature(DWORD tag, const char *data, unsigned int data_size,
        struct wined3d_shader_signature *s)
{
    struct wined3d_shader_signature_element *e;
    bool has_stream_index, has_min_precision;
    const char *ptr = data;
    unsigned int i;
    unsigned int count;

    if (!require_space(0, 2, sizeof(DWORD), data_size))
    {
        WARN("Invalid data size %#x.\n", data_size);
        return E_INVALIDARG;
    }

    count = read_dword(&ptr);
    TRACE("%u elements.\n", count);

    skip_dword_unknown(&ptr, 1); /* It seems to always be 0x00000008. */

    if (!require_space(ptr - data, count, 6 * sizeof(DWORD), data_size))
    {
        WARN("Invalid count %#x (data size %#x).\n", count, data_size);
        return E_INVALIDARG;
    }

    if (!(e = calloc(count, sizeof(*e))))
    {
        ERR("Failed to allocate input signature memory.\n");
        return E_OUTOFMEMORY;
    }

    has_min_precision = tag == TAG_OSG1 || tag == TAG_PSG1 || tag == TAG_ISG1;
    has_stream_index = tag == TAG_OSG5 || has_min_precision;

    for (i = 0; i < count; ++i)
    {
        unsigned int name_offset;

        if (has_stream_index)
            e[i].stream_idx = read_dword(&ptr);
        else
            e[i].stream_idx = 0;
        name_offset = read_dword(&ptr);
        if (!(e[i].semantic_name = shader_get_string(data, data_size, name_offset)))
        {
            WARN("Invalid name offset %#x (data size %#x).\n", name_offset, data_size);
            free(e);
            return E_INVALIDARG;
        }
        e[i].semantic_idx = read_dword(&ptr);
        e[i].sysval_semantic = read_dword(&ptr);
        e[i].component_type = read_dword(&ptr);
        e[i].register_idx = read_dword(&ptr);
        e[i].mask = read_dword(&ptr);

        if (has_min_precision)
            e[i].min_precision = read_dword(&ptr);
        else
            e[i].min_precision = 0;

        TRACE("Stream: %u, semantic: %s, semantic idx: %u, sysval_semantic %#x, "
                "type %u, register idx: %u, use_mask %#x, input_mask %#x, min_precision %u.\n",
                e[i].stream_idx, debugstr_a(e[i].semantic_name), e[i].semantic_idx, e[i].sysval_semantic,
                e[i].component_type, e[i].register_idx, (e[i].mask >> 8) & 0xff, e[i].mask & 0xff, e[i].min_precision);
    }

    s->elements = e;
    s->element_count = count;

    return S_OK;
}

static HRESULT shader_dxbc_process_section(struct wined3d_shader *shader, unsigned int max_version,
        enum vkd3d_shader_source_type *source_type, const struct vkd3d_shader_dxbc_section_desc *section)
{
    unsigned int data_size = section->data.size;
    const void *data = section->data.code;
    uint32_t tag = section->tag;
    HRESULT hr;

    switch (tag)
    {
        case TAG_ISGN:
        case TAG_ISG1:
            if (max_version < 4)
            {
                TRACE("Skipping shader input signature.\n");
                break;
            }
            if (shader->input_signature.elements)
            {
                FIXME("Multiple input signatures.\n");
                break;
            }
            if (FAILED(hr = shader_parse_signature(tag, data, data_size, &shader->input_signature)))
                return hr;
            break;

        case TAG_OSGN:
        case TAG_OSG1:
        case TAG_OSG5:
            if (max_version < 4)
            {
                TRACE("Skipping shader output signature.\n");
                break;
            }
            if (shader->output_signature.elements)
            {
                FIXME("Multiple output signatures.\n");
                break;
            }
            if (FAILED(hr = shader_parse_signature(tag, data, data_size, &shader->output_signature)))
                return hr;
            break;

        case TAG_PCSG:
        case TAG_PSG1:
            if (shader->patch_constant_signature.elements)
            {
                FIXME("Multiple patch constant signatures.\n");
                break;
            }
            if (FAILED(hr = shader_parse_signature(tag, data, data_size, &shader->patch_constant_signature)))
                return hr;
            break;

        case TAG_SHDR:
        case TAG_SHEX:
            if (max_version < 4)
            {
                TRACE("Skipping SM4+ shader.\n");
                break;
            }
            if (shader->function)
                FIXME("Multiple shader code chunks.\n");
            shader->function = data;
            shader->functionLength = data_size;
            *source_type = VKD3D_SHADER_SOURCE_DXBC_TPF;
            break;

        case TAG_AON9:
            if (max_version < 4)
            {
                const struct aon9_header *header = data;
                unsigned int unknown_dword_count;
                const char *byte_code;

                if (data_size < sizeof(*header))
                {
                    WARN("Invalid Aon9 data size %#x.\n", data_size);
                    return E_FAIL;
                }
                byte_code = data;
                byte_code += header->byte_code_offset;
                unknown_dword_count = (header->byte_code_offset - sizeof(*header)) / sizeof(DWORD);

                if (data_size - 2 * sizeof(DWORD) < header->byte_code_offset)
                {
                    WARN("Invalid byte code offset %#x (size %#x).\n", header->byte_code_offset, data_size);
                    return E_FAIL;
                }
                FIXME("Skipping %u unknown DWORDs.\n", unknown_dword_count);

                if (shader->function)
                    FIXME("Multiple shader code chunks.\n");
                shader->function = (const DWORD *)byte_code;
                shader->functionLength = data_size - header->byte_code_offset;
                *source_type = VKD3D_SHADER_SOURCE_D3D_BYTECODE;
                TRACE("Feature level 9 shader version 0%08x, 0%08lx.\n",
                        header->shader_version, *shader->function);
            }
            else
            {
                TRACE("Skipping feature level 9 shader code.\n");
            }
            break;

        default:
            TRACE("Skipping chunk %s.\n", debugstr_fourcc(tag));
            break;
    }

    return S_OK;
}

HRESULT wined3d_shader_extract_from_dxbc(struct wined3d_shader *shader,
        unsigned int max_shader_version, enum vkd3d_shader_source_type *source_type)
{
    const struct vkd3d_shader_code dxbc = {.code = shader->byte_code, .size = shader->byte_code_size};
    struct vkd3d_shader_dxbc_desc dxbc_desc;
    HRESULT hr = WINED3D_OK;
    unsigned int i;
    int ret;

    if ((ret = vkd3d_shader_parse_dxbc(&dxbc, 0, &dxbc_desc, NULL)) < 0)
    {
        WARN("Failed to parse DXBC, ret %d.\n", ret);
        return E_INVALIDARG;
    }

    for (i = 0; i < dxbc_desc.section_count; ++i)
    {
        if (FAILED(hr = shader_dxbc_process_section(shader, max_shader_version, source_type, &dxbc_desc.sections[i])))
            break;
    }
    vkd3d_shader_free_dxbc(&dxbc_desc);

    if (!shader->function)
        hr = E_INVALIDARG;

    if (FAILED(hr))
        WARN("Failed to parse DXBC, hr %#lx.\n", hr);

    return hr;
}
