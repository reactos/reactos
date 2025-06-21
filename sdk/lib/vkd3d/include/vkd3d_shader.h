/*
 * Copyright 2017-2019 Józef Kucia for CodeWeavers
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

#ifndef __VKD3D_SHADER_H
#define __VKD3D_SHADER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <vkd3d_types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * \file vkd3d_shader.h
 *
 * This file contains definitions for the vkd3d-shader library.
 *
 * The vkd3d-shader library provides multiple utilities related to the
 * compilation, transformation, and reflection of GPU shaders.
 *
 * \since 1.2
 */

/** \since 1.3 */
enum vkd3d_shader_api_version
{
    VKD3D_SHADER_API_VERSION_1_0,
    VKD3D_SHADER_API_VERSION_1_1,
    VKD3D_SHADER_API_VERSION_1_2,
    VKD3D_SHADER_API_VERSION_1_3,
    VKD3D_SHADER_API_VERSION_1_4,
    VKD3D_SHADER_API_VERSION_1_5,
    VKD3D_SHADER_API_VERSION_1_6,
    VKD3D_SHADER_API_VERSION_1_7,
    VKD3D_SHADER_API_VERSION_1_8,
    VKD3D_SHADER_API_VERSION_1_9,
    VKD3D_SHADER_API_VERSION_1_10,
    VKD3D_SHADER_API_VERSION_1_11,
    VKD3D_SHADER_API_VERSION_1_12,
    VKD3D_SHADER_API_VERSION_1_13,
    VKD3D_SHADER_API_VERSION_1_14,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_API_VERSION),
};

/** The type of a chained structure. */
enum vkd3d_shader_structure_type
{
    /** The structure is a vkd3d_shader_compile_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO,
    /** The structure is a vkd3d_shader_interface_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO,
    /** The structure is a vkd3d_shader_scan_descriptor_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO,
    /** The structure is a vkd3d_shader_spirv_domain_shader_target_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_DOMAIN_SHADER_TARGET_INFO,
    /** The structure is a vkd3d_shader_spirv_target_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO,
    /** The structure is a vkd3d_shader_transform_feedback_info structure. */
    VKD3D_SHADER_STRUCTURE_TYPE_TRANSFORM_FEEDBACK_INFO,

    /**
     * The structure is a vkd3d_shader_hlsl_source_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO,
    /**
     * The structure is a vkd3d_shader_preprocess_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO,
    /**
     * The structure is a vkd3d_shader_descriptor_offset_info structure.
     * \since 1.3
     */
    VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO,
    /**
     * The structure is a vkd3d_shader_scan_signature_info structure.
     * \since 1.9
     */
    VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO,
    /**
     * The structure is a vkd3d_shader_varying_map_info structure.
     * \since 1.9
     */
    VKD3D_SHADER_STRUCTURE_TYPE_VARYING_MAP_INFO,
    /**
     * The structure is a vkd3d_shader_scan_combined_resource_sampler_info structure.
     * \since 1.10
     */
    VKD3D_SHADER_STRUCTURE_TYPE_SCAN_COMBINED_RESOURCE_SAMPLER_INFO,
    /**
     * The structure is a vkd3d_shader_parameter_info structure.
     * \since 1.13
     */
    VKD3D_SHADER_STRUCTURE_TYPE_PARAMETER_INFO,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_STRUCTURE_TYPE),
};

/**
 * Determines how buffer UAVs are stored.
 *
 * This also affects UAV counters in Vulkan environments. In OpenGL
 * environments, atomic counter buffers are always used for UAV counters.
 */
enum vkd3d_shader_compile_option_buffer_uav
{
    /** Use buffer textures for buffer UAVs. This is the default value. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_TEXEL_BUFFER = 0x00000000,
    /** Use storage buffers for buffer UAVs. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV_STORAGE_BUFFER       = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV),
};

/**
 * Determines how typed UAVs are declared.
 * \since 1.5
 */
enum vkd3d_shader_compile_option_typed_uav
{
    /** Use R32(u)i/R32f format for UAVs which are read from. This is the default value. */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_R32     = 0x00000000,
    /**
     * Use Unknown format for UAVs which are read from. This should only be set if
     * shaderStorageImageReadWithoutFormat is enabled in the target environment.
     */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV_READ_FORMAT_UNKNOWN = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV),
};

enum vkd3d_shader_compile_option_formatting_flags
{
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_NONE    = 0x00000000,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_COLOUR  = 0x00000001,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_INDENT  = 0x00000002,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_OFFSETS = 0x00000004,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_HEADER  = 0x00000008,
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_RAW_IDS = 0x00000010,
    /**
     * Emit the signatures when disassembling a shader.
     *
     * \since 1.12
     */
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING_IO_SIGNATURES = 0x00000020,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_FORMATTING_FLAGS),
};

/** Determines how matrices are stored. \since 1.9 */
enum vkd3d_shader_compile_option_pack_matrix_order
{
    VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ROW_MAJOR    = 0x00000001,
    VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_COLUMN_MAJOR = 0x00000002,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ORDER),
};

/** Individual options to enable various backward compatibility features. \since 1.10 */
enum vkd3d_shader_compile_option_backward_compatibility
{
    /**
     *  Causes compiler to convert SM1-3 semantics to corresponding System Value semantics,
     *  when compiling HLSL sources for SM4+ targets.
     *
     *  This option does the following conversions:
     *
     *  - POSITION to SV_Position for vertex shader outputs, pixel shader inputs,
     *    and geometry shader inputs and outputs;
     *  - COLORN to SV_TargetN for pixel shader outputs;
     *  - DEPTH to SV_Depth for pixel shader outputs.
     */
    VKD3D_SHADER_COMPILE_OPTION_BACKCOMPAT_MAP_SEMANTIC_NAMES = 0x00000001,
    /**
     *  Causes 'double' to behave as an alias for 'float'. This option only
     *  applies to HLSL sources with shader model 1-3 target profiles. Without
     *  this option using the 'double' type produces compilation errors in
     *  these target profiles.
     *
     *  This option is disabled by default.
     *
     *  \since 1.14
     */
    VKD3D_SHADER_COMPILE_OPTION_DOUBLE_AS_FLOAT_ALIAS = 0x00000002,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_BACKWARD_COMPATIBILITY),
};

/**
 * Determines the origin of fragment coordinates.
 *
 * \since 1.10
 */
enum vkd3d_shader_compile_option_fragment_coordinate_origin
{
    /** Fragment coordinates originate from the upper-left. This is the
     * default; it's also the only value supported by Vulkan environments. */
    VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN_UPPER_LEFT = 0x00000000,
    /** Fragment coordinates originate from the lower-left. This matches the
     * traditional behaviour of OpenGL environments. */
    VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN_LOWER_LEFT = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN),
};

/** Advertises feature availability. \since 1.11 */
enum vkd3d_shader_compile_option_feature_flags
{
    /** The SPIR-V target environment supports 64-bit integer types. This
     * corresponds to the "shaderInt64" feature in the Vulkan API, and the
     * "GL_ARB_gpu_shader_int64" extension in the OpenGL API. */
    VKD3D_SHADER_COMPILE_OPTION_FEATURE_INT64         = 0x00000001,
    /** The SPIR-V target environment supports 64-bit floating-point types.
     * This corresponds to the "shaderFloat64" feature in the Vulkan API, and
     * the "GL_ARB_gpu_shader_fp64" extension in the OpenGL API. */
    VKD3D_SHADER_COMPILE_OPTION_FEATURE_FLOAT64       = 0x00000002,
    /** The SPIR-V target environment supports wave operations.
     * This flag is valid only in VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1
     * or greater, and corresponds to the following minimum requirements in
     * VkPhysicalDeviceSubgroupProperties:
     * - subgroupSize >= 4.
     * - supportedOperations has BASIC, VOTE, ARITHMETIC, BALLOT, SHUFFLE and
     *       QUAD bits set.
     * - supportedStages include COMPUTE and FRAGMENT. \since 1.12 */
    VKD3D_SHADER_COMPILE_OPTION_FEATURE_WAVE_OPS      = 0x00000004,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_FEATURE_FLAGS),
};

/**
 * Flags for vkd3d_shader_parse_dxbc().
 *
 * \since 1.12
 */
enum vkd3d_shader_parse_dxbc_flags
{
    /** Ignore the checksum and continue parsing even if it is
     * incorrect. */
    VKD3D_SHADER_PARSE_DXBC_IGNORE_CHECKSUM           = 0x00000001,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARSE_DXBC_FLAGS),
};

enum vkd3d_shader_compile_option_name
{
    /**
     * If \a value is nonzero, do not include debug information in the
     * compiled shader. The default value is zero.
     *
     * This option is supported by vkd3d_shader_compile(). However, not all
     * compilers support generating debug information.
     */
    VKD3D_SHADER_COMPILE_OPTION_STRIP_DEBUG = 0x00000001,
    /** \a value is a member of enum vkd3d_shader_compile_option_buffer_uav. */
    VKD3D_SHADER_COMPILE_OPTION_BUFFER_UAV  = 0x00000002,
    /** \a value is a member of enum vkd3d_shader_compile_option_formatting_flags. */
    VKD3D_SHADER_COMPILE_OPTION_FORMATTING  = 0x00000003,
    /** \a value is a member of enum vkd3d_shader_api_version. \since 1.3 */
    VKD3D_SHADER_COMPILE_OPTION_API_VERSION = 0x00000004,
    /** \a value is a member of enum vkd3d_shader_compile_option_typed_uav. \since 1.5 */
    VKD3D_SHADER_COMPILE_OPTION_TYPED_UAV   = 0x00000005,
    /**
     * If \a value is nonzero, write the point size for Vulkan tessellation and
     * geometry shaders. This option should be enabled if and only if the
     * shaderTessellationAndGeometryPointSize feature is enabled. The default
     * value is nonzero, i.e. write the point size.
     *
     * This option is supported by vkd3d_shader_compile() for the SPIR-V target
     * type and Vulkan targets; it should not be enabled otherwise.
     *
     * \since 1.7
     */
    VKD3D_SHADER_COMPILE_OPTION_WRITE_TESS_GEOM_POINT_SIZE = 0x00000006,
    /**
     * This option specifies default matrix packing order for HLSL sources.
     * Explicit variable modifiers or pragmas will take precedence.
     *
     * \a value is a member of enum vkd3d_shader_compile_option_pack_matrix_order.
     *
     * \since 1.9
     */
    VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ORDER = 0x00000007,
    /**
     * This option is used to enable various backward compatibility features.
     *
     * \a value is a mask of values from enum vkd3d_shader_compile_option_backward_compatibility.
     *
     * \since 1.10
     */
    VKD3D_SHADER_COMPILE_OPTION_BACKWARD_COMPATIBILITY = 0x00000008,
    /**
     * This option specifies the origin of fragment coordinates for SPIR-V
     * targets.
     *
     * \a value is a member of enum
     * vkd3d_shader_compile_option_fragment_coordinate_origin.
     *
     * \since 1.10
     */
    VKD3D_SHADER_COMPILE_OPTION_FRAGMENT_COORDINATE_ORIGIN = 0x00000009,
    /**
     * This option specifies the shader features available in the target
     * environment. These are not extensions, i.e. they are always supported
     * by the driver, but may not be supported by the available hardware.
     *
     * \a value is a member of enum vkd3d_shader_compile_option_feature_flags.
     *
     * \since 1.11
     */
    VKD3D_SHADER_COMPILE_OPTION_FEATURE = 0x0000000a,
    /**
     * If \a value is non-zero compilation will produce a child effect using
     * shared object descriptions, as instructed by the "shared" modifier.
     * Child effects are supported with fx_4_0, and fx_4_1 profiles. This option
     * and "shared" modifiers are ignored for the fx_5_0 profile and non-fx profiles.
     * The fx_2_0 profile does not have a separate concept of child effects, variables
     * marked with "shared" modifier will be marked as such in a binary.
     *
     * \since 1.12
     */
    VKD3D_SHADER_COMPILE_OPTION_CHILD_EFFECT = 0x0000000b,
    /**
     * If \a value is nonzero, emit a compile warning warn when vectors or
     * matrices are truncated in an implicit conversion.
     * If warnings are disabled, this option has no effect.
     * This option has no effects for targets other than HLSL.
     *
     * The default value is nonzero, i.e. enable implicit truncation warnings.
     *
     * \since 1.12
     */
    VKD3D_SHADER_COMPILE_OPTION_WARN_IMPLICIT_TRUNCATION = 0x0000000c,
    /**
     * If \a value is nonzero, empty constant buffers descriptions are
     * written out in the output effect binary. This option applies only
     * to fx_4_0 and fx_4_1 profiles and is otherwise ignored.
     *
     * \since 1.12
     */
    VKD3D_SHADER_COMPILE_OPTION_INCLUDE_EMPTY_BUFFERS_IN_EFFECTS = 0x0000000d,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPILE_OPTION_NAME),
};

/**
 * Various settings which may affect shader compilation or scanning, passed as
 * part of struct vkd3d_shader_compile_info. For more details, see the
 * documentation for individual options.
 */
struct vkd3d_shader_compile_option
{
    /** Name of the option. */
    enum vkd3d_shader_compile_option_name name;
    /**
     * A value associated with the option. The type and interpretation of the
     * value depends on the option in question.
     */
    unsigned int value;
};

/** Describes which shader stages a resource is visible to. */
enum vkd3d_shader_visibility
{
    /** The resource is visible to all shader stages. */
    VKD3D_SHADER_VISIBILITY_ALL = 0,
    /** The resource is visible only to the vertex shader. */
    VKD3D_SHADER_VISIBILITY_VERTEX = 1,
    /** The resource is visible only to the hull shader. */
    VKD3D_SHADER_VISIBILITY_HULL = 2,
    /** The resource is visible only to the domain shader. */
    VKD3D_SHADER_VISIBILITY_DOMAIN = 3,
    /** The resource is visible only to the geometry shader. */
    VKD3D_SHADER_VISIBILITY_GEOMETRY = 4,
    /** The resource is visible only to the pixel shader. */
    VKD3D_SHADER_VISIBILITY_PIXEL = 5,

    /** The resource is visible only to the compute shader. */
    VKD3D_SHADER_VISIBILITY_COMPUTE = 1000000000,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_VISIBILITY),
};

/** A generic structure containing a GPU shader, in text or byte-code format. */
struct vkd3d_shader_code
{
    /**
     * Pointer to the code. Note that textual formats are not null-terminated.
     * Therefore \a size should not include a null terminator, when this
     * structure is passed as input to a vkd3d-shader function, and the
     * allocated string will not include a null terminator when this structure
     * is used as output.
     */
    const void *code;
    /** Size of \a code, in bytes. */
    size_t size;
};

/** The type of a shader resource descriptor. */
enum vkd3d_shader_descriptor_type
{
    /**
     * The descriptor is a shader resource view. In Direct3D assembly, this is
     * bound to a t# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_SRV     = 0x0,
    /**
     * The descriptor is an unordered access view. In Direct3D assembly, this is
     * bound to a u# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_UAV     = 0x1,
    /**
     * The descriptor is a constant buffer view. In Direct3D assembly, this is
     * bound to a cb# register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_CBV     = 0x2,
    /**
     * The descriptor is a sampler. In Direct3D assembly, this is bound to an s#
     * register.
     */
    VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER = 0x3,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_TYPE),
};

/**
 * A common structure describing the bind point of a descriptor or descriptor
 * array in the target environment.
 */
struct vkd3d_shader_descriptor_binding
{
    /**
     * The set of the descriptor. If the target environment does not support
     * descriptor sets, this value must be set to 0.
     */
    unsigned int set;
    /** The binding index of the descriptor. */
    unsigned int binding;
    /**
     * The size of this descriptor array. If an offset is specified for this
     * binding by the vkd3d_shader_descriptor_offset_info structure, counting
     * starts at that offset.
     */
    unsigned int count;
};

enum vkd3d_shader_binding_flag
{
    VKD3D_SHADER_BINDING_FLAG_BUFFER = 0x00000001,
    VKD3D_SHADER_BINDING_FLAG_IMAGE  = 0x00000002,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_BINDING_FLAG),
};

/**
 * The manner in which a parameter value is provided to the shader, used in
 * struct vkd3d_shader_parameter and struct vkd3d_shader_parameter1.
 */
enum vkd3d_shader_parameter_type
{
    VKD3D_SHADER_PARAMETER_TYPE_UNKNOWN,
    /** The parameter value is embedded directly in the shader. */
    VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT,
    /**
     * The parameter value is provided to the shader via specialization
     * constants. This value is only supported for the SPIR-V target type.
     */
    VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT,
    /**
     * The parameter value is provided to the shader as part of a uniform
     * buffer.
     *
     * \since 1.13
     */
    VKD3D_SHADER_PARAMETER_TYPE_BUFFER,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_TYPE),
};

/**
 * The format of data provided to the shader, used in
 * struct vkd3d_shader_parameter and struct vkd3d_shader_parameter1.
 */
enum vkd3d_shader_parameter_data_type
{
    VKD3D_SHADER_PARAMETER_DATA_TYPE_UNKNOWN,
    /** The parameter is provided as a 32-bit unsigned integer. */
    VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32,
    /** The parameter is provided as a 32-bit float. \since 1.13 */
    VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32,
    /**
     * The parameter is provided as a 4-dimensional vector of 32-bit floats.
     * This parameter must be used with struct vkd3d_shader_parameter1;
     * it cannot be used with struct vkd3d_shader_parameter.
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_DATA_TYPE),
};

/**
 * Names a specific shader parameter, used in
 * struct vkd3d_shader_parameter and struct vkd3d_shader_parameter1.
 */
enum vkd3d_shader_parameter_name
{
    VKD3D_SHADER_PARAMETER_NAME_UNKNOWN,
    /**
     * The sample count of the framebuffer, as returned by the HLSL function
     * GetRenderTargetSampleCount() or the GLSL builtin gl_NumSamples.
     *
     * This parameter should be specified when compiling to SPIR-V, which
     * provides no builtin ability to query this information from the shader.
     *
     * The default value is 1.
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
     */
    VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT,
    /**
     * Alpha test comparison function. When this parameter is provided, if the
     * alpha component of the pixel shader colour output at location 0 fails the
     * test, as defined by this function and the reference value provided by
     * VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF, the fragment will be
     * discarded.
     *
     * This parameter, along with VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF,
     * can be used to implement fixed function alpha test, as present in
     * Direct3D versions up to 9, if the target environment does not support
     * alpha test as part of its own fixed-function API (as Vulkan and core
     * OpenGL).
     *
     * The default value is VKD3D_SHADER_COMPARISON_FUNC_ALWAYS.
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32. The value specified must be
     * a member of enum vkd3d_shader_comparison_func.
     *
     * Only VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT is supported in this
     * version of vkd3d-shader.
     *
     * \since 1.13
     */
    VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_FUNC,
    /**
     * Alpha test reference value.
     * See VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_FUNC for documentation of
     * alpha test.
     *
     * The default value is zero.
     *
     * \since 1.13
     */
    VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF,
    /**
     * Whether to use flat interpolation for fragment shader colour inputs.
     * If the value is nonzero, inputs whose semantic usage is COLOR will use
     * flat interpolation instead of linear.
     * This parameter is ignored if the shader model is 4 or greater, since only
     * shader model 3 and below do not specify the interpolation mode in the
     * shader bytecode.
     *
     * This parameter can be used to implement fixed function shade mode, as
     * present in Direct3D versions up to 9, if the target environment does not
     * support shade mode as part of its own fixed-function API (as Vulkan and
     * core OpenGL).
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
     *
     * The default value is zero, i.e. use linear interpolation.
     *
     * Only VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT is supported in this
     * version of vkd3d-shader.
     *
     * \since 1.13
     */
    VKD3D_SHADER_PARAMETER_NAME_FLAT_INTERPOLATION,
    /**
     * A mask of enabled clip planes.
     *
     * When this parameter is provided to a vertex shader, for each nonzero bit
     * of this mask, a user clip distance will be generated from vertex position
     * in clip space, and the clip plane defined by the indexed vector, taken
     * from the VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_# parameter.
     *
     * Regardless of the specific clip planes which are enabled, the clip
     * distances which are output are a contiguous array starting from clip
     * distance 0. This affects the interface of OpenGL. For example, if only
     * clip planes 1 and 3 are enabled (and so the value of the mask is 0xa),
     * the user should enable only GL_CLIP_DISTANCE0 and GL_CLIP_DISTANCE1.
     *
     * The default value is zero, i.e. do not enable any clip planes.
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
     *
     * Only VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT is supported in this
     * version of vkd3d-shader.
     *
     * If the source shader writes clip distances and this parameter is nonzero,
     * compilation fails.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_MASK,
    /**
     * Clip plane values.
     * See VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_MASK for documentation of
     * clip planes.
     *
     * These enum values are contiguous and arithmetic may safely be performed
     * on them. That is, VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_[n] is
     * VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_0 plus n.
     *
     * The data type for each parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4.
     *
     * The default value for each plane is a (0, 0, 0, 0) vector.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_0,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_1,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_2,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_3,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_4,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_5,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_6,
    VKD3D_SHADER_PARAMETER_NAME_CLIP_PLANE_7,
    /**
     * Point size.
     *
     * When this parameter is provided to a vertex, tessellation, or geometry
     * shader, and the source shader does not write point size, it specifies a
     * uniform value which will be written to point size.
     * If the source shader writes point size, this parameter is ignored.
     *
     * This parameter can be used to implement fixed function point size, as
     * present in Direct3D versions 8 and 9, if the target environment does not
     * support point size as part of its own fixed-function API (as Vulkan and
     * core OpenGL).
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE,
    /**
     * Minimum point size.
     *
     * When this parameter is provided to a vertex, tessellation, or geometry
     * shader, and the source shader writes point size or uses the
     * VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE parameter, the point size will
     * be clamped to the provided minimum value.
     * If point size is not written in one of these ways,
     * this parameter is ignored.
     * If this parameter is not provided, the point size will not be clamped
     * to a minimum size by vkd3d-shader.
     *
     * This parameter can be used to implement fixed function point size, as
     * present in Direct3D versions 8 and 9, if the target environment does not
     * support point size as part of its own fixed-function API (as Vulkan and
     * core OpenGL).
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MIN,
    /**
     * Maximum point size.
     *
     * This parameter has identical behaviour to
     * VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MIN, except that it provides
     * the maximum size rather than the minimum.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_POINT_SIZE_MAX,
    /**
     * Whether texture coordinate inputs should take their values from the
     * point coordinate.
     *
     * When this parameter is provided to a pixel shader, and the value is
     * nonzero, any fragment shader input with the semantic name "TEXCOORD"
     * takes its value from the point coordinates instead of from the previous
     * shader. The point coordinates here are defined as a four-component vector
     * whose X and Y components are the X and Y coordinates of the fragment
     * within a point being rasterized, and whose Z and W components are zero.
     *
     * In GLSL, the X and Y components are drawn from gl_PointCoord; in SPIR-V,
     * they are drawn from a variable with the BuiltinPointCoord decoration.
     *
     * This includes t# fragment shader inputs in shader model 2 shaders,
     * as well as texture sampling in shader model 1 shaders.
     *
     * This parameter can be used to implement fixed function point sprite, as
     * present in Direct3D versions 8 and 9, if the target environment does not
     * support point sprite as part of its own fixed-function API (as Vulkan and
     * core OpenGL).
     *
     * The data type for this parameter must be
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
     *
     * The default value is zero, i.e. use the original varyings.
     *
     * Only VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT is supported in this
     * version of vkd3d-shader.
     *
     * \since 1.14
     */
    VKD3D_SHADER_PARAMETER_NAME_POINT_SPRITE,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_PARAMETER_NAME),
};

/**
 * The value of an immediate constant parameter, used in
 * struct vkd3d_shader_parameter.
 */
struct vkd3d_shader_parameter_immediate_constant
{
    union
    {
        /**
         * The value if the parameter's data type is
         * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
         */
        uint32_t u32;
        /**
         * The value if the parameter's data type is
         * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32.
         *
         * \since 1.13
         */
        float f32;
    } u;
};

/**
 * The value of an immediate constant parameter, used in
 * struct vkd3d_shader_parameter1.
 *
 * \since 1.13
 */
struct vkd3d_shader_parameter_immediate_constant1
{
    union
    {
        /**
         * The value if the parameter's data type is
         * VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32.
         */
        uint32_t u32;
        /**
         * The value if the parameter's data type is
         * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32.
         */
        float f32;
        /**
         * A pointer to the value if the parameter's data type is
         * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4.
         *
         * \since 1.14
         */
        float f32_vec4[4];
        void *_pointer_pad;
        uint32_t _pad[4];
    } u;
};

/**
 * The linkage of a specialization constant parameter, used in
 * struct vkd3d_shader_parameter and struct vkd3d_shader_parameter1.
 */
struct vkd3d_shader_parameter_specialization_constant
{
    /**
     * The ID of the specialization constant.
     * If the type comprises more than one constant, such as
     * VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32_VEC4, then a contiguous
     * array of specialization constants should be used, one for each component,
     * and this ID should point to the first component.
     */
    uint32_t id;
};

/**
 * The linkage of a parameter specified through a uniform buffer, used in
 * struct vkd3d_shader_parameter1.
 */
struct vkd3d_shader_parameter_buffer
{
    /**
     * The set of the uniform buffer descriptor. If the target environment does
     * not support descriptor sets, this value must be set to 0.
     */
    unsigned int set;
    /** The binding index of the uniform buffer descriptor. */
    unsigned int binding;
    /** The byte offset of the parameter within the buffer. */
    uint32_t offset;
};

/**
 * An individual shader parameter.
 *
 * This structure is an earlier version of struct vkd3d_shader_parameter1
 * which supports fewer parameter types;
 * refer to that structure for usage information.
 *
 * Only the following types may be used with this structure:
 *
 * - VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT
 * - VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT
 */
struct vkd3d_shader_parameter
{
    enum vkd3d_shader_parameter_name name;
    enum vkd3d_shader_parameter_type type;
    enum vkd3d_shader_parameter_data_type data_type;
    union
    {
        struct vkd3d_shader_parameter_immediate_constant immediate_constant;
        struct vkd3d_shader_parameter_specialization_constant specialization_constant;
    } u;
};

/**
 * An individual shader parameter.
 *
 * This structure is used in struct vkd3d_shader_parameter_info; see there for
 * explanation of shader parameters.
 *
 * For example, to specify the rasterizer sample count to the shader via an
 * unsigned integer specialization constant with ID 3,
 * set the following members:
 *
 * - \a name = VKD3D_SHADER_PARAMETER_NAME_RASTERIZER_SAMPLE_COUNT
 * - \a type = VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT
 * - \a data_type = VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32
 * - \a u.specialization_constant.id = 3
 *
 * This structure is an extended version of struct vkd3d_shader_parameter.
 */
struct vkd3d_shader_parameter1
{
    /** The builtin parameter to be mapped. */
    enum vkd3d_shader_parameter_name name;
    /** How the parameter will be provided to the shader. */
    enum vkd3d_shader_parameter_type type;
    /**
     * The data type of the supplied parameter, which determines how it is to
     * be interpreted.
     */
    enum vkd3d_shader_parameter_data_type data_type;
    union
    {
        /**
         * Additional information if \a type is
         * VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT.
         */
        struct vkd3d_shader_parameter_immediate_constant1 immediate_constant;
        /**
         * Additional information if \a type is
         * VKD3D_SHADER_PARAMETER_TYPE_SPECIALIZATION_CONSTANT.
         */
        struct vkd3d_shader_parameter_specialization_constant specialization_constant;
        /**
         * Additional information if \a type is
         * VKD3D_SHADER_PARAMETER_TYPE_BUFFER.
         */
        struct vkd3d_shader_parameter_buffer buffer;
        void *_pointer_pad;
        uint32_t _pad[4];
    } u;
};

/**
 * Symbolic register indices for mapping uniform constant register sets in
 * legacy Direct3D bytecode to constant buffer views in the target environment.
 *
 * Members of this enumeration are used in
 * \ref vkd3d_shader_resource_binding.register_index.
 *
 * \since 1.9
 */
enum vkd3d_shader_d3dbc_constant_register
{
    /** The float constant register set, c# in Direct3D assembly. */
    VKD3D_SHADER_D3DBC_FLOAT_CONSTANT_REGISTER  = 0x0,
    /** The integer constant register set, i# in Direct3D assembly. */
    VKD3D_SHADER_D3DBC_INT_CONSTANT_REGISTER    = 0x1,
    /** The boolean constant register set, b# in Direct3D assembly. */
    VKD3D_SHADER_D3DBC_BOOL_CONSTANT_REGISTER   = 0x2,
};

/**
 * Describes the mapping of a single resource or resource array to its binding
 * point in the target environment.
 *
 * For example, to map a Direct3D SRV with register space 2, register "t3" to
 * a Vulkan descriptor in set 4 and with binding 5, set the following members:
 * - \a type = VKD3D_SHADER_DESCRIPTOR_TYPE_SRV
 * - \a register_space = 2
 * - \a register_index = 3
 * - \a binding.set = 4
 * - \a binding.binding = 5
 * - \a binding.count = 1
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_resource_binding
{
    /** The type of this descriptor. */
    enum vkd3d_shader_descriptor_type type;
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /**
     * Register index of the Direct3D resource.
     *
     * For legacy Direct3D shaders, vkd3d-shader maps each constant register
     * set to a single constant buffer view. This parameter names the register
     * set to map, and must be a member of
     * enum vkd3d_shader_d3dbc_constant_register.
     */
    unsigned int register_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;
    /** A combination of zero or more elements of vkd3d_shader_binding_flag. */
    unsigned int flags;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
};

#define VKD3D_SHADER_DUMMY_SAMPLER_INDEX ~0u

/**
 * Describes the mapping of a Direct3D resource-sampler pair to a combined
 * sampler (i.e. sampled image).
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_combined_resource_sampler
{
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int resource_space;
    /** Register index of the Direct3D resource. */
    unsigned int resource_index;
    /**
     * Register space of the Direct3D sampler. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int sampler_space;
    /** Register index of the Direct3D sampler. */
    unsigned int sampler_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;
    /** A combination of zero or more elements of vkd3d_shader_binding_flag. */
    unsigned int flags;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
};

/**
 * Describes the mapping of a single Direct3D UAV counter.
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_uav_counter_binding
{
    /**
     * Register space of the Direct3D UAV descriptor. If the source format does
     * not support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /** Register index of the Direct3D UAV descriptor. */
    unsigned int register_index;
    /** Shader stage(s) to which the UAV counter is visible. */
    enum vkd3d_shader_visibility shader_visibility;

    /** The binding in the target environment. */
    struct vkd3d_shader_descriptor_binding binding;
    unsigned int offset;
};

/**
 * Describes the mapping of a Direct3D constant buffer to a range of push
 * constants in the target environment.
 *
 * This structure is used in struct vkd3d_shader_interface_info.
 */
struct vkd3d_shader_push_constant_buffer
{
    /**
     * Register space of the Direct3D resource. If the source format does not
     * support multiple register spaces, this parameter must be set to 0.
     */
    unsigned int register_space;
    /** Register index of the Direct3D resource. */
    unsigned int register_index;
    /** Shader stage(s) to which the resource is visible. */
    enum vkd3d_shader_visibility shader_visibility;

    /** Offset, in bytes, of the target push constants. */
    unsigned int offset;
    /** Size, in bytes, of the target push constants. */
    unsigned int size;
};

/**
 * A chained structure describing the interface between a compiled shader and
 * the target environment.
 *
 * For example, when compiling Direct3D shader byte code to SPIR-V, this
 * structure contains mappings from Direct3D descriptor registers to SPIR-V
 * descriptor bindings.
 *
 * This structure is optional. If omitted, vkd3d_shader_compile() will use a
 * default mapping, in which resources are mapped to sequential bindings in
 * register set 0.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 */
struct vkd3d_shader_interface_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /** Pointer to an array of bindings for shader resource descriptors. */
    const struct vkd3d_shader_resource_binding *bindings;
    /** Size, in elements, of \ref bindings. */
    unsigned int binding_count;

    /** Pointer to an array of bindings for push constant buffers. */
    const struct vkd3d_shader_push_constant_buffer *push_constant_buffers;
    /** Size, in elements, of \ref push_constant_buffers. */
    unsigned int push_constant_buffer_count;

    /** Pointer to an array of bindings for combined samplers. */
    const struct vkd3d_shader_combined_resource_sampler *combined_samplers;
    /** Size, in elements, of \ref combined_samplers. */
    unsigned int combined_sampler_count;

    /** Pointer to an array of bindings for UAV counters. */
    const struct vkd3d_shader_uav_counter_binding *uav_counters;
    /** Size, in elements, of \ref uav_counters. */
    unsigned int uav_counter_count;
};

struct vkd3d_shader_transform_feedback_element
{
    unsigned int stream_index;
    const char *semantic_name;
    unsigned int semantic_index;
    uint8_t component_index;
    uint8_t component_count;
    uint8_t output_slot;
};

/* Extends vkd3d_shader_interface_info. */
struct vkd3d_shader_transform_feedback_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    const struct vkd3d_shader_transform_feedback_element *elements;
    unsigned int element_count;
    const unsigned int *buffer_strides;
    unsigned int buffer_stride_count;
};

struct vkd3d_shader_descriptor_offset
{
    unsigned int static_offset;
    unsigned int dynamic_offset_index;
};

/**
 * A chained structure containing descriptor offsets.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_interface_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_descriptor_offset_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_DESCRIPTOR_OFFSET_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Byte offset within the push constants of an array of 32-bit
     * descriptor array offsets. See the description of 'binding_offsets'
     * below.
     */
    unsigned int descriptor_table_offset;
    /** Size, in elements, of the descriptor table push constant array. */
    unsigned int descriptor_table_count;

    /**
     * Pointer to an array of struct vkd3d_shader_descriptor_offset objects.
     * The 'static_offset' field contains an offset into the descriptor arrays
     * referenced by the 'bindings' array in struct vkd3d_shader_interface_info.
     * This allows mapping multiple shader resource arrays to a single binding
     * point in the target environment.
     *
     * 'dynamic_offset_index' in struct vkd3d_shader_descriptor_offset allows
     * offsets to be set at runtime. The 32-bit descriptor table push constant
     * at this index will be added to 'static_offset' to calculate the final
     * binding offset.
     *
     * If runtime offsets are not required, set all 'dynamic_offset_index'
     * values to \c ~0u and 'descriptor_table_count' to zero.
     *
     * For example, to map Direct3D constant buffer registers 'cb0[0:3]' and
     * 'cb1[6:7]' to descriptors 8-12 and 4-5 in the Vulkan descriptor array in
     * descriptor set 3 and with binding 2, set the following values in the
     * 'bindings' array in struct vkd3d_shader_interface_info:
     *
     * \code
     * type = VKD3D_SHADER_DESCRIPTOR_TYPE_CBV
     * register_space = 0
     * register_index = 0
     * binding.set = 3
     * binding.binding = 2
     * binding.count = 4
     *
     * type = VKD3D_SHADER_DESCRIPTOR_TYPE_CBV
     * register_space = 0
     * register_index = 6
     * binding.set = 3
     * binding.binding = 2
     * binding.count = 2
     * \endcode
     *
     * and then pass \c {8, \c 4} as static binding offsets here.
     *
     * This field may be NULL, in which case the corresponding offsets are
     * specified to be 0.
     */
    const struct vkd3d_shader_descriptor_offset *binding_offsets;

    /**
     * Pointer to an array of offsets into the descriptor arrays referenced by
     * the 'uav_counters' array in struct vkd3d_shader_interface_info. This
     * works the same way as \ref binding_offsets above.
     */
    const struct vkd3d_shader_descriptor_offset *uav_counter_offsets;
};

/** The format of a shader to be compiled or scanned. */
enum vkd3d_shader_source_type
{
    /**
     * The shader has no type or is to be ignored. This is not a valid value
     * for vkd3d_shader_compile() or vkd3d_shader_scan().
     */
    VKD3D_SHADER_SOURCE_NONE,
    /**
     * A 'Tokenized Program Format' shader embedded in a DXBC container. This is
     * the format used for Direct3D shader model 4 and 5 shaders.
     */
    VKD3D_SHADER_SOURCE_DXBC_TPF,
    /** High-Level Shader Language source code. \since 1.3 */
    VKD3D_SHADER_SOURCE_HLSL,
    /**
     * Legacy Direct3D byte-code. This is the format used for Direct3D shader
     * model 1, 2, and 3 shaders. \since 1.3
     */
    VKD3D_SHADER_SOURCE_D3D_BYTECODE,
    /**
     * A 'DirectX Intermediate Language' shader embedded in a DXBC container. This is
     * the format used for Direct3D shader model 6 shaders. \since 1.9
     */
    VKD3D_SHADER_SOURCE_DXBC_DXIL,
    /**
     * Binary format used by Direct3D 9/10.x/11 effects.
     * Input is a raw FX section without container. \since 1.14
     */
    VKD3D_SHADER_SOURCE_FX,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SOURCE_TYPE),
};

/** The output format of a compiled shader. */
enum vkd3d_shader_target_type
{
    /**
     * The shader has no type or is to be ignored. This is not a valid value
     * for vkd3d_shader_compile().
     */
    VKD3D_SHADER_TARGET_NONE,
    /**
     * A SPIR-V shader in binary form. This is the format used for Vulkan
     * shaders.
     */
    VKD3D_SHADER_TARGET_SPIRV_BINARY,
    VKD3D_SHADER_TARGET_SPIRV_TEXT,
    /**
     * Direct3D shader assembly. \since 1.3
     */
    VKD3D_SHADER_TARGET_D3D_ASM,
    /**
     * Legacy Direct3D byte-code. This is the format used for Direct3D shader
     * model 1, 2, and 3 shaders. \since 1.3
     */
    VKD3D_SHADER_TARGET_D3D_BYTECODE,
    /**
     * A 'Tokenized Program Format' shader embedded in a DXBC container. This is
     * the format used for Direct3D shader model 4 and 5 shaders. \since 1.3
     */
    VKD3D_SHADER_TARGET_DXBC_TPF,
    /**
     * An 'OpenGL Shading Language' shader. \since 1.3
     */
    VKD3D_SHADER_TARGET_GLSL,
    /**
     * Binary format used by Direct3D 9/10.x/11 effects profiles.
     * Output is a raw FX section without container. \since 1.11
     */
    VKD3D_SHADER_TARGET_FX,
    /**
     * A 'Metal Shading Language' shader. \since 1.14
     */
    VKD3D_SHADER_TARGET_MSL,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TARGET_TYPE),
};

/**
 * Describes the minimum severity of compilation messages returned by
 * vkd3d_shader_compile() and similar functions.
 */
enum vkd3d_shader_log_level
{
    /** No messages will be returned. */
    VKD3D_SHADER_LOG_NONE,
    /** Only fatal errors which prevent successful compilation will be returned. */
    VKD3D_SHADER_LOG_ERROR,
    /** Non-fatal warnings and fatal errors will be returned. */
    VKD3D_SHADER_LOG_WARNING,
    /**
     * All messages, including general informational messages, will be returned.
     */
    VKD3D_SHADER_LOG_INFO,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_LOG_LEVEL),
};

/**
 * A chained structure containing compilation parameters.
 */
struct vkd3d_shader_compile_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO. */
    enum vkd3d_shader_structure_type type;
    /**
     * Optional pointer to a structure containing further parameters. For a list
     * of valid structures, refer to the respective function documentation. If
     * no further parameters are needed, this field should be set to NULL.
     */
    const void *next;

    /** Input source code or byte code. */
    struct vkd3d_shader_code source;

    /** Format of the input code passed in \ref source. */
    enum vkd3d_shader_source_type source_type;
    /** Desired output format. */
    enum vkd3d_shader_target_type target_type;

    /**
     * Pointer to an array of compilation options. This field is ignored if
     * \ref option_count is zero, but must be valid otherwise.
     *
     * If the same option is specified multiple times, only the last value is
     * used.
     *
     * Options not relevant to or not supported by a particular shader compiler
     * or scanner will be ignored.
     */
    const struct vkd3d_shader_compile_option *options;
    /** Size, in elements, of \ref options. */
    unsigned int option_count;

    /** Minimum severity of messages returned from the shader function. */
    enum vkd3d_shader_log_level log_level;
    /**
     * Name of the initial source file, which may be used in error messages or
     * debug information. This parameter is optional and may be NULL.
     */
    const char *source_name;
};

enum vkd3d_shader_spirv_environment
{
    VKD3D_SHADER_SPIRV_ENVIRONMENT_NONE,
    VKD3D_SHADER_SPIRV_ENVIRONMENT_OPENGL_4_5,
    VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_0, /* default target */
    /** \since 1.12 */
    VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SPIRV_ENVIRONMENT),
};

enum vkd3d_shader_spirv_extension
{
    VKD3D_SHADER_SPIRV_EXTENSION_NONE,
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_DEMOTE_TO_HELPER_INVOCATION,
    /** \since 1.3 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_DESCRIPTOR_INDEXING,
    /** \since 1.3 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_STENCIL_EXPORT,
    /** \since 1.11 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_VIEWPORT_INDEX_LAYER,
    /** \since 1.12 */
    VKD3D_SHADER_SPIRV_EXTENSION_EXT_FRAGMENT_SHADER_INTERLOCK,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SPIRV_EXTENSION),
};

/* Extends vkd3d_shader_compile_info. */
struct vkd3d_shader_spirv_target_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    const char *entry_point; /* "main" if NULL. */

    enum vkd3d_shader_spirv_environment environment;

    const enum vkd3d_shader_spirv_extension *extensions;
    unsigned int extension_count;

    const struct vkd3d_shader_parameter *parameters;
    unsigned int parameter_count;

    bool dual_source_blending;
    const unsigned int *output_swizzles;
    unsigned int output_swizzle_count;
};

enum vkd3d_shader_tessellator_output_primitive
{
    VKD3D_SHADER_TESSELLATOR_OUTPUT_POINT        = 0x1,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_LINE         = 0x2,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CW  = 0x3,
    VKD3D_SHADER_TESSELLATOR_OUTPUT_TRIANGLE_CCW = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TESSELLATOR_OUTPUT_PRIMITIVE),
};

enum vkd3d_shader_tessellator_partitioning
{
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_INTEGER         = 0x1,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_POW2            = 0x2,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_ODD  = 0x3,
    VKD3D_SHADER_TESSELLATOR_PARTITIONING_FRACTIONAL_EVEN = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TESSELLATOR_PARTITIONING),
};

/* Extends vkd3d_shader_spirv_target_info. */
struct vkd3d_shader_spirv_domain_shader_target_info
{
    enum vkd3d_shader_structure_type type;
    const void *next;

    enum vkd3d_shader_tessellator_output_primitive output_primitive;
    enum vkd3d_shader_tessellator_partitioning partitioning;
};

/**
 * A single preprocessor macro, passed as part of struct
 * vkd3d_shader_preprocess_info.
 */
struct vkd3d_shader_macro
{
    /**
     * Pointer to a null-terminated string containing the name of a macro. This
     * macro must not be a parameterized (i.e. function-like) macro. If this
     * field is not a valid macro identifier, this macro will be ignored.
     */
    const char *name;
    /**
     * Optional pointer to a null-terminated string containing the expansion of
     * the macro. This field may be set to NULL, in which case the macro has an
     * empty expansion.
     */
    const char *value;
};

/**
 * Type of a callback function which will be used to open preprocessor includes.
 *
 * This callback function is passed as part of struct
 * vkd3d_shader_preprocess_info.
 *
 * If this function fails, vkd3d-shader will emit a compilation error, and the
 * \a pfn_close_include callback will not be called.
 *
 * \param filename Unquoted string used as an argument to the \#include
 * directive.
 *
 * \param local Whether the \#include directive is requesting a local (i.e.
 * double-quoted) or system (i.e. angle-bracketed) include.
 *
 * \param parent_data Unprocessed source code of the file in which this
 * \#include directive is evaluated. This parameter may be NULL.
 *
 * \param context The user-defined pointer passed to struct
 * vkd3d_shader_preprocess_info.
 *
 * \param out Output location for the full contents of the included file. The
 * code need not be allocated using standard vkd3d functions, but must remain
 * valid until the corresponding call to \a pfn_close_include. If this function
 * fails, the contents of this parameter are ignored.
 *
 * \return A member of \ref vkd3d_result.
 */
typedef int (*PFN_vkd3d_shader_open_include)(const char *filename, bool local,
        const char *parent_data, void *context, struct vkd3d_shader_code *out);
/**
 * Type of a callback function which will be used to close preprocessor
 * includes.
 *
 * This callback function is passed as part of struct
 * vkd3d_shader_preprocess_info.
 *
 * \param code Contents of the included file, which were allocated by the
 * vkd3d_shader_preprocess_info.pfn_open_include callback.
 * The user must free them.
 *
 * \param context The user-defined pointer passed to struct
 * vkd3d_shader_preprocess_info.
 */
typedef void (*PFN_vkd3d_shader_close_include)(const struct vkd3d_shader_code *code, void *context);

/**
 * A chained structure containing preprocessing parameters.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_preprocess_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Pointer to an array of predefined macros. Each macro in this array will
     * be expanded as if a corresponding \#define statement were prepended to
     * the source code.
     *
     * If the same macro is specified multiple times, only the last value is
     * used.
     */
    const struct vkd3d_shader_macro *macros;
    /** Size, in elements, of \ref macros. */
    unsigned int macro_count;

    /**
     * Optional pointer to a callback function, which will be called in order to
     * evaluate \#include directives. The function receives parameters
     * corresponding to the directive's arguments, and should return the
     * complete text of the included file.
     *
     * If this field is set to NULL, or if this structure is omitted,
     * vkd3d-shader will attempt to open included files using POSIX file APIs.
     *
     * If this field is set to NULL, the \ref pfn_close_include field must also
     * be set to NULL.
     */
    PFN_vkd3d_shader_open_include pfn_open_include;
    /**
     * Optional pointer to a callback function, which will be called whenever an
     * included file is closed. This function will be called exactly once for
     * each successful call to \ref pfn_open_include, and should be used to free
     * any resources allocated thereby.
     *
     * If this field is set to NULL, the \ref pfn_open_include field must also
     * be set to NULL.
     */
    PFN_vkd3d_shader_close_include pfn_close_include;
    /**
     * User-defined pointer which will be passed unmodified to the
     * \ref pfn_open_include and \ref pfn_close_include callbacks.
     */
    void *include_context;
};

/**
 * A chained structure containing HLSL compilation parameters.
 *
 * This structure is optional.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.3
 */
struct vkd3d_shader_hlsl_source_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * Optional pointer to a null-terminated string containing the shader entry
     * point.
     *
     * If this parameter is NULL, vkd3d-shader uses the entry point "main".
     */
    const char *entry_point;
    struct vkd3d_shader_code secondary_code;
    /**
     * Pointer to a null-terminated string containing the target shader
     * profile.
     */
    const char *profile;
};

/* root signature 1.0 */
enum vkd3d_shader_filter
{
    VKD3D_SHADER_FILTER_MIN_MAG_MIP_POINT                          = 0x000,
    VKD3D_SHADER_FILTER_MIN_MAG_POINT_MIP_LINEAR                   = 0x001,
    VKD3D_SHADER_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT             = 0x004,
    VKD3D_SHADER_FILTER_MIN_POINT_MAG_MIP_LINEAR                   = 0x005,
    VKD3D_SHADER_FILTER_MIN_LINEAR_MAG_MIP_POINT                   = 0x010,
    VKD3D_SHADER_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR            = 0x011,
    VKD3D_SHADER_FILTER_MIN_MAG_LINEAR_MIP_POINT                   = 0x014,
    VKD3D_SHADER_FILTER_MIN_MAG_MIP_LINEAR                         = 0x015,
    VKD3D_SHADER_FILTER_ANISOTROPIC                                = 0x055,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_MIP_POINT               = 0x080,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR        = 0x081,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x084,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR        = 0x085,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT        = 0x090,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x091,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT        = 0x094,
    VKD3D_SHADER_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR              = 0x095,
    VKD3D_SHADER_FILTER_COMPARISON_ANISOTROPIC                     = 0x0d5,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_MIP_POINT                  = 0x100,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x101,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x104,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x105,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x110,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x111,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x114,
    VKD3D_SHADER_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR                 = 0x115,
    VKD3D_SHADER_FILTER_MINIMUM_ANISOTROPIC                        = 0x155,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_MIP_POINT                  = 0x180,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x181,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x184,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x185,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x190,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x191,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x194,
    VKD3D_SHADER_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR                 = 0x195,
    VKD3D_SHADER_FILTER_MAXIMUM_ANISOTROPIC                        = 0x1d5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_FILTER),
};

enum vkd3d_shader_texture_address_mode
{
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_WRAP        = 0x1,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_MIRROR      = 0x2,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_CLAMP       = 0x3,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_BORDER      = 0x4,
    VKD3D_SHADER_TEXTURE_ADDRESS_MODE_MIRROR_ONCE = 0x5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_TEXTURE_ADDRESS_MODE),
};

enum vkd3d_shader_comparison_func
{
    VKD3D_SHADER_COMPARISON_FUNC_NEVER         = 0x1,
    VKD3D_SHADER_COMPARISON_FUNC_LESS          = 0x2,
    VKD3D_SHADER_COMPARISON_FUNC_EQUAL         = 0x3,
    VKD3D_SHADER_COMPARISON_FUNC_LESS_EQUAL    = 0x4,
    VKD3D_SHADER_COMPARISON_FUNC_GREATER       = 0x5,
    VKD3D_SHADER_COMPARISON_FUNC_NOT_EQUAL     = 0x6,
    VKD3D_SHADER_COMPARISON_FUNC_GREATER_EQUAL = 0x7,
    VKD3D_SHADER_COMPARISON_FUNC_ALWAYS        = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPARISON_FUNC),
};

enum vkd3d_shader_static_border_colour
{
    VKD3D_SHADER_STATIC_BORDER_COLOUR_TRANSPARENT_BLACK = 0x0,
    VKD3D_SHADER_STATIC_BORDER_COLOUR_OPAQUE_BLACK      = 0x1,
    VKD3D_SHADER_STATIC_BORDER_COLOUR_OPAQUE_WHITE      = 0x2,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_STATIC_BORDER_COLOUR),
};

struct vkd3d_shader_static_sampler_desc
{
    enum vkd3d_shader_filter filter;
    enum vkd3d_shader_texture_address_mode address_u;
    enum vkd3d_shader_texture_address_mode address_v;
    enum vkd3d_shader_texture_address_mode address_w;
    float mip_lod_bias;
    unsigned int max_anisotropy;
    enum vkd3d_shader_comparison_func comparison_func;
    enum vkd3d_shader_static_border_colour border_colour;
    float min_lod;
    float max_lod;
    unsigned int shader_register;
    unsigned int register_space;
    enum vkd3d_shader_visibility shader_visibility;
};

struct vkd3d_shader_descriptor_range
{
    enum vkd3d_shader_descriptor_type range_type;
    unsigned int descriptor_count;
    unsigned int base_shader_register;
    unsigned int register_space;
    unsigned int descriptor_table_offset;
};

struct vkd3d_shader_root_descriptor_table
{
    unsigned int descriptor_range_count;
    const struct vkd3d_shader_descriptor_range *descriptor_ranges;
};

struct vkd3d_shader_root_constants
{
    unsigned int shader_register;
    unsigned int register_space;
    unsigned int value_count;
};

struct vkd3d_shader_root_descriptor
{
    unsigned int shader_register;
    unsigned int register_space;
};

enum vkd3d_shader_root_parameter_type
{
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE = 0x0,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS  = 0x1,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_CBV              = 0x2,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_SRV              = 0x3,
    VKD3D_SHADER_ROOT_PARAMETER_TYPE_UAV              = 0x4,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_PARAMETER_TYPE),
};

struct vkd3d_shader_root_parameter
{
    enum vkd3d_shader_root_parameter_type parameter_type;
    union
    {
        struct vkd3d_shader_root_descriptor_table descriptor_table;
        struct vkd3d_shader_root_constants constants;
        struct vkd3d_shader_root_descriptor descriptor;
    } u;
    enum vkd3d_shader_visibility shader_visibility;
};

enum vkd3d_shader_root_signature_flags
{
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_NONE                               = 0x00,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT = 0x01,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS     = 0x02,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS       = 0x04,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS     = 0x08,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS   = 0x10,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS      = 0x20,
    VKD3D_SHADER_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT                = 0x40,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_SIGNATURE_FLAGS),
};

struct vkd3d_shader_root_signature_desc
{
    unsigned int parameter_count;
    const struct vkd3d_shader_root_parameter *parameters;
    unsigned int static_sampler_count;
    const struct vkd3d_shader_static_sampler_desc *static_samplers;
    enum vkd3d_shader_root_signature_flags flags;
};

/* root signature 1.1 */
enum vkd3d_shader_root_descriptor_flags
{
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_NONE                             = 0x0,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE                    = 0x2,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE = 0x4,
    VKD3D_SHADER_ROOT_DESCRIPTOR_FLAG_DATA_STATIC                      = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_DESCRIPTOR_FLAGS),
};

enum vkd3d_shader_descriptor_range_flags
{
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_NONE                             = 0x0,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE             = 0x1,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE                    = 0x2,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE = 0x4,
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DATA_STATIC                      = 0x8,
    /** \since 1.11 */
    VKD3D_SHADER_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_STATIC_KEEPING_BUFFER_BOUNDS_CHECKS = 0x10000,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_RANGE_FLAGS),
};

struct vkd3d_shader_descriptor_range1
{
    enum vkd3d_shader_descriptor_type range_type;
    unsigned int descriptor_count;
    unsigned int base_shader_register;
    unsigned int register_space;
    enum vkd3d_shader_descriptor_range_flags flags;
    unsigned int descriptor_table_offset;
};

struct vkd3d_shader_root_descriptor_table1
{
    unsigned int descriptor_range_count;
    const struct vkd3d_shader_descriptor_range1 *descriptor_ranges;
};

struct vkd3d_shader_root_descriptor1
{
    unsigned int shader_register;
    unsigned int register_space;
    enum vkd3d_shader_root_descriptor_flags flags;
};

struct vkd3d_shader_root_parameter1
{
    enum vkd3d_shader_root_parameter_type parameter_type;
    union
    {
        struct vkd3d_shader_root_descriptor_table1 descriptor_table;
        struct vkd3d_shader_root_constants constants;
        struct vkd3d_shader_root_descriptor1 descriptor;
    } u;
    enum vkd3d_shader_visibility shader_visibility;
};

struct vkd3d_shader_root_signature_desc1
{
    unsigned int parameter_count;
    const struct vkd3d_shader_root_parameter1 *parameters;
    unsigned int static_sampler_count;
    const struct vkd3d_shader_static_sampler_desc *static_samplers;
    enum vkd3d_shader_root_signature_flags flags;
};

enum vkd3d_shader_root_signature_version
{
    VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_0 = 0x1,
    VKD3D_SHADER_ROOT_SIGNATURE_VERSION_1_1 = 0x2,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_ROOT_SIGNATURE_VERSION),
};

struct vkd3d_shader_versioned_root_signature_desc
{
    enum vkd3d_shader_root_signature_version version;
    union
    {
        struct vkd3d_shader_root_signature_desc v_1_0;
        struct vkd3d_shader_root_signature_desc1 v_1_1;
    } u;
};

/**
 * The type of a shader resource, returned as part of struct
 * vkd3d_shader_descriptor_info.
 */
enum vkd3d_shader_resource_type
{
    /**
     * The type is invalid or not applicable for this descriptor. This value is
     * returned for samplers.
     */
    VKD3D_SHADER_RESOURCE_NONE              = 0x0,
    /** Dimensionless buffer. */
    VKD3D_SHADER_RESOURCE_BUFFER            = 0x1,
    /** 1-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_1D        = 0x2,
    /** 2-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2D        = 0x3,
    /** Multisampled 2-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DMS      = 0x4,
    /** 3-dimensional texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_3D        = 0x5,
    /** Cubemap texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_CUBE      = 0x6,
    /** 1-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_1DARRAY   = 0x7,
    /** 2-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DARRAY   = 0x8,
    /** Multisampled 2-dimensional array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_2DMSARRAY = 0x9,
    /** Cubemap array texture. */
    VKD3D_SHADER_RESOURCE_TEXTURE_CUBEARRAY = 0xa,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_RESOURCE_TYPE),
};

/**
 * The type of the data contained in a shader resource, returned as part of
 * struct vkd3d_shader_descriptor_info. All formats are 32-bit.
 */
enum vkd3d_shader_resource_data_type
{
    /** Unsigned normalized integer. */
    VKD3D_SHADER_RESOURCE_DATA_UNORM     = 0x1,
    /** Signed normalized integer. */
    VKD3D_SHADER_RESOURCE_DATA_SNORM     = 0x2,
    /** Signed integer. */
    VKD3D_SHADER_RESOURCE_DATA_INT       = 0x3,
    /** Unsigned integer. */
    VKD3D_SHADER_RESOURCE_DATA_UINT      = 0x4,
    /** IEEE single-precision floating-point. */
    VKD3D_SHADER_RESOURCE_DATA_FLOAT     = 0x5,
    /** Undefined/type-less. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_MIXED     = 0x6,
    /** IEEE double-precision floating-point. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_DOUBLE    = 0x7,
    /** Continuation of the previous component. For example, 64-bit
     * double-precision floating-point data may be returned as two 32-bit
     * components, with the first component (containing the LSB) specified as
     * VKD3D_SHADER_RESOURCE_DATA_DOUBLE, and the second component specified
     * as VKD3D_SHADER_RESOURCE_DATA_CONTINUED. \since 1.3 */
    VKD3D_SHADER_RESOURCE_DATA_CONTINUED = 0x8,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_RESOURCE_DATA_TYPE),
};

/**
 * Additional flags describing a shader descriptor, returned as part of struct
 * vkd3d_shader_descriptor_info.
 */
enum vkd3d_shader_descriptor_info_flag
{
    /**
     * The descriptor is a UAV resource, whose counter is read from or written
     * to by the shader.
     */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_COUNTER             = 0x00000001,
    /** The descriptor is a UAV resource, which is read from by the shader. */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_READ                = 0x00000002,
    /** The descriptor is a comparison sampler. */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_SAMPLER_COMPARISON_MODE = 0x00000004,
    /** The descriptor is a UAV resource, on which the shader performs
     *  atomic ops. \since 1.6 */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_UAV_ATOMICS             = 0x00000008,
    /** The descriptor is a raw (byte-addressed) buffer. \since 1.9 */
    VKD3D_SHADER_DESCRIPTOR_INFO_FLAG_RAW_BUFFER              = 0x00000010,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_DESCRIPTOR_INFO_FLAG),
};

/**
 * Describes a single shader descriptor; returned as part of
 * struct vkd3d_shader_scan_descriptor_info.
 */
struct vkd3d_shader_descriptor_info
{
    /** Type of the descriptor (for example, SRV, CBV, UAV, or sampler). */
    enum vkd3d_shader_descriptor_type type;
    /**
     * Register space of the resource, or 0 if the shader does not
     * support multiple register spaces.
     */
    unsigned int register_space;
    /** Register index of the descriptor. */
    unsigned int register_index;
    /** Resource type, if applicable, including its dimension. */
    enum vkd3d_shader_resource_type resource_type;
    /** Data type contained in the resource (for example, float or integer). */
    enum vkd3d_shader_resource_data_type resource_data_type;
    /**
     * Bitwise combination of zero or more members of
     * \ref vkd3d_shader_descriptor_info_flag.
     */
    unsigned int flags;
    /**
     *  Size of this descriptor array, or 1 if a single descriptor.
     *  For an unbounded array this value is ~0u.
     */
    unsigned int count;
};

/**
 * A chained structure enumerating the descriptors declared by a shader.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * When scanning a legacy Direct3D shader, vkd3d-shader enumerates descriptors
 * as follows:
 *
 * - Each constant register set used by the shader is scanned as a single
 *   constant buffer descriptor.
 *   There may therefore be up to three such descriptors, one for each register
 *   set used by the shader: float, integer, and boolean.
 *   The fields are set as follows:
 *   * The \ref vkd3d_shader_descriptor_info.type field is set to
 *     VKD3D_SHADER_DESCRIPTOR_TYPE_CBV.
 *   * The \ref vkd3d_shader_descriptor_info.register_space field is set to zero.
 *   * The \ref vkd3d_shader_descriptor_info.register_index field is set to a
 *     member of enum vkd3d_shader_d3dbc_constant_register denoting which set
 *     is used.
 *   * The \ref vkd3d_shader_descriptor_info.count field is set to one.
 * - Each sampler used by the shader is scanned as two separate descriptors,
 *   one representing the texture, and one representing the sampler state.
 *   If desired, these may be mapped back into a single combined sampler using
 *   struct vkd3d_shader_combined_resource_sampler.
 *   The fields are set as follows:
 *   * The \ref vkd3d_shader_descriptor_info.type field is set to
 *     VKD3D_SHADER_DESCRIPTOR_TYPE_SRV and VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER
 *     respectively.
 *   * The \ref vkd3d_shader_descriptor_info.register_space field is set to zero.
 *   * The \ref vkd3d_shader_descriptor_info.register_index field is set to the
 *     binding index of the original sampler, for both descriptors.
 *   * The \ref vkd3d_shader_descriptor_info.count field is set to one.
 */
struct vkd3d_shader_scan_descriptor_info
{
    /**
     * Input; must be set to VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO.
     */
    enum vkd3d_shader_structure_type type;
    /** Input; optional pointer to a structure containing further parameters. */
    const void *next;

    /** Output; returns a pointer to an array of descriptors. */
    struct vkd3d_shader_descriptor_info *descriptors;
    /** Output; size, in elements, of \ref descriptors. */
    unsigned int descriptor_count;
};

/**
 * This structure describes a single resource-sampler pair. It is returned as
 * part of struct vkd3d_shader_scan_combined_resource_sampler_info.
 *
 * \since 1.10
 */
struct vkd3d_shader_combined_resource_sampler_info
{
    unsigned int resource_space;
    unsigned int resource_index;
    unsigned int sampler_space;
    unsigned int sampler_index;
};

/**
 * A chained structure describing the resource-sampler pairs used by a shader.
 *
 * This structure extends vkd3d_shader_compile_info.
 *
 * The information returned in this structure can be used to populate the
 * \ref vkd3d_shader_interface_info.combined_samplers field. This is
 * particularly useful when targeting environments without separate binding
 * points for samplers and resources, like OpenGL.
 *
 * No resource-sampler pairs are returned for dynamic accesses to
 * resource/sampler descriptor arrays, as can occur in Direct3D shader model
 * 5.1 shaders.
 *
 * Members of this structure are allocated by vkd3d-shader and should be freed
 * with vkd3d_shader_free_scan_combined_resource_sampler_info() when no longer
 * needed.
 *
 * \since 1.10
 */
struct vkd3d_shader_scan_combined_resource_sampler_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_SCAN_COMBINED_RESOURCE_SAMPLER_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /** Pointer to an array of resource-sampler pairs. */
    struct vkd3d_shader_combined_resource_sampler_info *combined_samplers;
    /** The number of resource-sampler pairs in \ref combined_samplers. */
    unsigned int combined_sampler_count;
};

/**
 * Data type of a shader varying, returned as part of struct
 * vkd3d_shader_signature_element.
 */
enum vkd3d_shader_component_type
{
    /** The varying has no type. */
    VKD3D_SHADER_COMPONENT_VOID     = 0x0,
    /** 32-bit unsigned integer. */
    VKD3D_SHADER_COMPONENT_UINT     = 0x1,
    /** 32-bit signed integer. */
    VKD3D_SHADER_COMPONENT_INT      = 0x2,
    /** 32-bit IEEE floating-point. */
    VKD3D_SHADER_COMPONENT_FLOAT    = 0x3,
    /** Boolean. */
    VKD3D_SHADER_COMPONENT_BOOL     = 0x4,
    /** 64-bit IEEE floating-point. */
    VKD3D_SHADER_COMPONENT_DOUBLE   = 0x5,
    /** 64-bit unsigned integer. \since 1.11 */
    VKD3D_SHADER_COMPONENT_UINT64   = 0x6,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_COMPONENT_TYPE),
};

/** System value semantic, returned as part of struct vkd3d_shader_signature. */
enum vkd3d_shader_sysval_semantic
{
    /** No system value. */
    VKD3D_SHADER_SV_NONE                      = 0x00,
    /** Vertex position; SV_Position in Direct3D. */
    VKD3D_SHADER_SV_POSITION                  = 0x01,
    /** Clip distance; SV_ClipDistance in Direct3D. */
    VKD3D_SHADER_SV_CLIP_DISTANCE             = 0x02,
    /** Cull distance; SV_CullDistance in Direct3D. */
    VKD3D_SHADER_SV_CULL_DISTANCE             = 0x03,
    /** Render target layer; SV_RenderTargetArrayIndex in Direct3D. */
    VKD3D_SHADER_SV_RENDER_TARGET_ARRAY_INDEX = 0x04,
    /** Viewport index; SV_ViewportArrayIndex in Direct3D. */
    VKD3D_SHADER_SV_VIEWPORT_ARRAY_INDEX      = 0x05,
    /** Vertex ID; SV_VertexID in Direct3D. */
    VKD3D_SHADER_SV_VERTEX_ID                 = 0x06,
    /** Primitive ID; SV_PrimitiveID in Direct3D. */
    VKD3D_SHADER_SV_PRIMITIVE_ID              = 0x07,
    /** Instance ID; SV_InstanceID in Direct3D. */
    VKD3D_SHADER_SV_INSTANCE_ID               = 0x08,
    /** Whether the triangle is front-facing; SV_IsFrontFace in Direct3D. */
    VKD3D_SHADER_SV_IS_FRONT_FACE             = 0x09,
    /** Sample index; SV_SampleIndex in Direct3D. */
    VKD3D_SHADER_SV_SAMPLE_INDEX              = 0x0a,
    VKD3D_SHADER_SV_TESS_FACTOR_QUADEDGE      = 0x0b,
    VKD3D_SHADER_SV_TESS_FACTOR_QUADINT       = 0x0c,
    VKD3D_SHADER_SV_TESS_FACTOR_TRIEDGE       = 0x0d,
    VKD3D_SHADER_SV_TESS_FACTOR_TRIINT        = 0x0e,
    VKD3D_SHADER_SV_TESS_FACTOR_LINEDET       = 0x0f,
    VKD3D_SHADER_SV_TESS_FACTOR_LINEDEN       = 0x10,
    /** Render target; SV_Target in Direct3D. \since 1.9 */
    VKD3D_SHADER_SV_TARGET                    = 0x40,
    /** Depth; SV_Depth in Direct3D. \since 1.9 */
    VKD3D_SHADER_SV_DEPTH                     = 0x41,
    /** Sample mask; SV_Coverage in Direct3D. \since 1.9 */
    VKD3D_SHADER_SV_COVERAGE                  = 0x42,
    /**
     * Depth, which is guaranteed to be greater than or equal to the current
     * depth; SV_DepthGreaterEqual in Direct3D. \since 1.9
     */
    VKD3D_SHADER_SV_DEPTH_GREATER_EQUAL       = 0x43,
    /**
     * Depth, which is guaranteed to be less than or equal to the current
     * depth; SV_DepthLessEqual in Direct3D. \since 1.9
     */
    VKD3D_SHADER_SV_DEPTH_LESS_EQUAL          = 0x44,
    /** Stencil reference; SV_StencilRef in Direct3D. \since 1.9 */
    VKD3D_SHADER_SV_STENCIL_REF               = 0x45,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SYSVAL_SEMANTIC),
};

/**
 * Minimum interpolation precision of a shader varying, returned as part of
 * struct vkd3d_shader_signature_element.
 */
enum vkd3d_shader_minimum_precision
{
    VKD3D_SHADER_MINIMUM_PRECISION_NONE      = 0,
    /** 16-bit floating-point. */
    VKD3D_SHADER_MINIMUM_PRECISION_FLOAT_16  = 1,
    /** 10-bit fixed point (2 integer and 8 fractional bits). */
    VKD3D_SHADER_MINIMUM_PRECISION_FIXED_8_2 = 2,
    /** 16-bit signed integer. */
    VKD3D_SHADER_MINIMUM_PRECISION_INT_16    = 4,
    /** 16-bit unsigned integer. */
    VKD3D_SHADER_MINIMUM_PRECISION_UINT_16   = 5,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_MINIMUM_PRECISION),
};

/**
 * A single shader varying, returned as part of struct vkd3d_shader_signature.
 */
struct vkd3d_shader_signature_element
{
    /** Semantic name. */
    const char *semantic_name;
    /** Semantic index, or 0 if the semantic is not indexed. */
    unsigned int semantic_index;
    /**
     * Stream index of a geometry shader output semantic. If the signature is
     * not a geometry shader output signature, this field will be set to 0.
     */
    unsigned int stream_index;
    /**
     * System value semantic. If the varying is not a system value, this field
     * will be set to VKD3D_SHADER_SV_NONE.
     */
    enum vkd3d_shader_sysval_semantic sysval_semantic;
    /** Data type. */
    enum vkd3d_shader_component_type component_type;
    /** Register index. */
    unsigned int register_index;
    /** Mask of the register components allocated to this varying. */
    unsigned int mask;
    /**
     * Subset of \ref mask which the shader reads from or writes to. Unlike
     * Direct3D shader bytecode, the mask for output and tessellation signatures
     * is not inverted, i.e. bits set in this field denote components which are
     * written to.
     */
    unsigned int used_mask;
    /** Minimum interpolation precision. */
    enum vkd3d_shader_minimum_precision min_precision;
};

/**
 * Description of a shader input or output signature. This structure is
 * populated by vkd3d_shader_parse_input_signature().
 *
 * The helper function vkd3d_shader_find_signature_element() will look up a
 * varying element by its semantic name, semantic index, and stream index.
 */
struct vkd3d_shader_signature
{
    /** Pointer to an array of varyings. */
    struct vkd3d_shader_signature_element *elements;
    /** Size, in elements, of \ref elements. */
    unsigned int element_count;
};

/** Possible values for a single component of a vkd3d-shader swizzle. */
enum vkd3d_shader_swizzle_component
{
    VKD3D_SHADER_SWIZZLE_X = 0x0,
    VKD3D_SHADER_SWIZZLE_Y = 0x1,
    VKD3D_SHADER_SWIZZLE_Z = 0x2,
    VKD3D_SHADER_SWIZZLE_W = 0x3,

    VKD3D_FORCE_32_BIT_ENUM(VKD3D_SHADER_SWIZZLE_COMPONENT),
};

/**
 * A description of a DXBC section.
 *
 * \since 1.7
 */
struct vkd3d_shader_dxbc_section_desc
{
    /** The section tag. */
    uint32_t tag;
    /** The contents of the section. */
    struct vkd3d_shader_code data;
};

/**
 * A description of a DXBC blob, as returned by vkd3d_shader_parse_dxbc().
 *
 * \since 1.7
 */
struct vkd3d_shader_dxbc_desc
{
    /**
     * The DXBC tag. This will always be "DXBC" in structures returned by
     * this version of vkd3d-shader.
     */
    uint32_t tag;
    /** A checksum of the DXBC contents. */
    uint32_t checksum[4];
    /**
     * The DXBC version. This will always be 1 in structures returned by this
     * version of vkd3d-shader.
     */
    unsigned int version;
    /** The total size of the DXBC blob. */
    size_t size;
    /** The number of sections contained in the DXBC. */
    size_t section_count;
    /** Descriptions of the sections contained in the DXBC. */
    struct vkd3d_shader_dxbc_section_desc *sections;
};

/**
 * A mask selecting one component from a vkd3d-shader swizzle. The component has
 * type \ref vkd3d_shader_swizzle_component.
 */
#define VKD3D_SHADER_SWIZZLE_MASK (0xffu)
/** The offset, in bits, of the nth parameter of a vkd3d-shader swizzle. */
#define VKD3D_SHADER_SWIZZLE_SHIFT(idx) (8u * (idx))

/**
 * A helper macro which returns a vkd3d-shader swizzle with the given
 * components. The components are specified as the suffixes to members of
 * \ref vkd3d_shader_swizzle_component. For example, the swizzle ".xwyy" can be
 * represented as:
 * \code
 * VKD3D_SHADER_SWIZZLE(X, W, Y, Y)
 * \endcode
 */
#define VKD3D_SHADER_SWIZZLE(x, y, z, w) \
        (VKD3D_SHADER_SWIZZLE_ ## x << VKD3D_SHADER_SWIZZLE_SHIFT(0) \
                | VKD3D_SHADER_SWIZZLE_ ## y << VKD3D_SHADER_SWIZZLE_SHIFT(1) \
                | VKD3D_SHADER_SWIZZLE_ ## z << VKD3D_SHADER_SWIZZLE_SHIFT(2) \
                | VKD3D_SHADER_SWIZZLE_ ## w << VKD3D_SHADER_SWIZZLE_SHIFT(3))

/** The identity swizzle ".xyzw". */
#define VKD3D_SHADER_NO_SWIZZLE VKD3D_SHADER_SWIZZLE(X, Y, Z, W)

/** Build a vkd3d-shader swizzle with the given components. */
static inline uint32_t vkd3d_shader_create_swizzle(enum vkd3d_shader_swizzle_component x,
        enum vkd3d_shader_swizzle_component y, enum vkd3d_shader_swizzle_component z,
        enum vkd3d_shader_swizzle_component w)
{
    return ((x & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(0))
            | ((y & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(1))
            | ((z & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(2))
            | ((w & VKD3D_SHADER_SWIZZLE_MASK) << VKD3D_SHADER_SWIZZLE_SHIFT(3));
}

/**
 * A chained structure containing descriptions of shader inputs and outputs.
 *
 * This structure is currently implemented only for DXBC and legacy D3D bytecode
 * source types.
 * For DXBC shaders, the returned information is parsed directly from the
 * signatures embedded in the DXBC shader.
 * For legacy D3D shaders, the returned information is synthesized based on
 * registers declared or used by shader instructions.
 * For all other shader types, the structure is zeroed.
 *
 * All members (except for \ref type and \ref next) are output-only.
 *
 * This structure is passed to vkd3d_shader_scan() and extends
 * vkd3d_shader_compile_info.
 *
 * Members of this structure are allocated by vkd3d-shader and should be freed
 * with vkd3d_shader_free_scan_signature_info() when no longer needed.
 *
 * All signatures may contain pointers into the input shader, and should only
 * be accessed while the input shader remains valid.
 *
 * Signature elements are synthesized from legacy Direct3D bytecode as follows:
 * - The \ref vkd3d_shader_signature_element.semantic_name field is set to an
 *   uppercase string corresponding to the HLSL name for the usage, e.g.
 *   "POSITION", "BLENDWEIGHT", "COLOR", "PSIZE", etc.
 * - The \ref vkd3d_shader_signature_element.semantic_index field is set to the
 *   usage index.
 * - The \ref vkd3d_shader_signature_element.stream_index is always 0.
 *
 * Signature elements are synthesized for any input or output register declared
 * or used in a legacy Direct3D bytecode shader, including the following:
 * - Shader model 1 and 2 colour and texture coordinate registers.
 * - The shader model 1 pixel shader output register.
 * - Shader model 1 and 2 vertex shader output registers (position, fog, and
 *   point size).
 * - Shader model 3 pixel shader system value input registers (pixel position
 *   and face).
 *
 * \since 1.9
 */
struct vkd3d_shader_scan_signature_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /** The shader input varyings. */
    struct vkd3d_shader_signature input;

    /** The shader output varyings. */
    struct vkd3d_shader_signature output;

    /** The shader patch constant varyings. */
    struct vkd3d_shader_signature patch_constant;
};

/**
 * Describes the mapping of a output varying register in a shader stage,
 * to an input varying register in the following shader stage.
 *
 * This structure is used in struct vkd3d_shader_varying_map_info.
 */
struct vkd3d_shader_varying_map
{
    /**
     * The signature index (in the output signature) of the output varying.
     * If greater than or equal to the number of elements in the output
     * signature, signifies that the varying is consumed by the next stage but
     * not written by this one.
     */
    unsigned int output_signature_index;
    /** The register index of the input varying to map this register to. */
    unsigned int input_register_index;
    /** The mask consumed by the destination register. */
    unsigned int input_mask;
};

/**
 * A chained structure which describes how output varyings in this shader stage
 * should be mapped to input varyings in the next stage.
 *
 * This structure is optional. It should not be provided if there is no shader
 * stage.
 * However, depending on the input and output formats, this structure may be
 * necessary in order to generate shaders which correctly match each other.
 *
 * If this structure is absent, vkd3d-shader will map varyings from one stage
 * to another based on their register index.
 * For Direct3D shader model 3.0, such a default mapping will be incorrect
 * unless the registers are allocated in the same order, and hence this
 * field is necessary to correctly match inter-stage varyings.
 * This mapping may also be necessary under other circumstances where the
 * varying interface does not match exactly.
 *
 * This structure is passed to vkd3d_shader_compile() and extends
 * vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.9
 */
struct vkd3d_shader_varying_map_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_VARYING_MAP_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /**
     * A mapping of output varyings in this shader stage to input varyings
     * in the next shader stage.
     *
     * This mapping should include exactly one element for each varying
     * consumed by the next shader stage.
     * If this shader stage outputs a varying that is not consumed by the next
     * shader stage, that varying should be absent from this array.
     *
     * This mapping may be constructed by vkd3d_shader_build_varying_map().
     */
    const struct vkd3d_shader_varying_map *varying_map;
    /** The number of registers provided in \ref varying_map. */
    unsigned int varying_count;
};

/**
 * Interface information regarding a builtin shader parameter.
 *
 * Like compile options specified with struct vkd3d_shader_compile_option,
 * parameters are used to specify certain values which are not part of the
 * source shader bytecode but which need to be specified in the shader bytecode
 * in the target format.
 * Unlike struct vkd3d_shader_compile_option, however, this structure allows
 * parameters to be specified in a variety of different ways, as described by
 * enum vkd3d_shader_parameter_type.
 *
 * This structure is an extended version of struct vkd3d_shader_parameter as
 * used in struct vkd3d_shader_spirv_target_info, which allows more parameter
 * types to be used, and also allows specifying parameters when compiling
 * shaders to target types other than SPIR-V. If this structure is chained
 * along with vkd3d_shader_spirv_target_info, any parameters specified in the
 * latter structure are ignored.
 *
 * This structure is passed to vkd3d_shader_compile() and extends
 * vkd3d_shader_compile_info.
 *
 * This structure contains only input parameters.
 *
 * \since 1.13
 */
struct vkd3d_shader_parameter_info
{
    /** Must be set to VKD3D_SHADER_STRUCTURE_TYPE_PARAMETER_INFO. */
    enum vkd3d_shader_structure_type type;
    /** Optional pointer to a structure containing further parameters. */
    const void *next;

    /** Pointer to an array of dynamic parameters for this shader instance. */
    const struct vkd3d_shader_parameter1 *parameters;
    /** Size, in elements, of \ref parameters. */
    unsigned int parameter_count;
};

#ifdef LIBVKD3D_SHADER_SOURCE
# define VKD3D_SHADER_API VKD3D_EXPORT
#else
# define VKD3D_SHADER_API VKD3D_IMPORT
#endif

#ifndef VKD3D_SHADER_NO_PROTOTYPES

/**
 * Returns the current version of this library.
 *
 * \param major Output location for the major version of this library.
 *
 * \param minor Output location for the minor version of this library.
 *
 * \return A human-readable string describing the library name and version. This
 * string is null-terminated and UTF-8 encoded. This may be a pointer to static
 * data in libvkd3d-shader; it should not be freed.
 */
VKD3D_SHADER_API const char *vkd3d_shader_get_version(unsigned int *major, unsigned int *minor);
/**
 * Returns the source types supported, with any target type, by
 * vkd3d_shader_compile(). Future versions of the library may introduce
 * additional source types; callers should ignore unrecognised source types.
 *
 * Use vkd3d_shader_get_supported_target_types() to determine which target types
 * are supported for each source type.
 *
 * \param count Output location for the size, in elements, of the returned
 * array.
 *
 * \return Pointer to an array of source types supported by this version of
 * vkd3d-shader. This array may be a pointer to static data in libvkd3d-shader;
 * it should not be freed.
 */
VKD3D_SHADER_API const enum vkd3d_shader_source_type *vkd3d_shader_get_supported_source_types(unsigned int *count);
/**
 * Returns the target types supported, with the given source type, by
 * vkd3d_shader_compile(). Future versions of the library may introduce
 * additional target types; callers should ignore unrecognised target types.
 *
 * \param source_type Source type for which to enumerate supported target types.
 *
 * \param count Output location for the size, in elements, of the returned
 * array.
 *
 * \return Pointer to an array of target types supported by this version of
 * vkd3d-shader. This array may be a pointer to static data in libvkd3d-shader;
 * it should not be freed.
 */
VKD3D_SHADER_API const enum vkd3d_shader_target_type *vkd3d_shader_get_supported_target_types(
        enum vkd3d_shader_source_type source_type, unsigned int *count);

/**
 * Transform a form of GPU shader source code or byte code into another form of
 * source code or byte code.
 *
 * This version of vkd3d-shader supports the following transformations:
 * - VKD3D_SHADER_SOURCE_DXBC_TPF to VKD3D_SHADER_TARGET_SPIRV_BINARY
 * - VKD3D_SHADER_SOURCE_DXBC_TPF to VKD3D_SHADER_TARGET_SPIRV_TEXT
 *   (if vkd3d was compiled with SPIRV-Tools)
 * - VKD3D_SHADER_SOURCE_DXBC_TPF to VKD3D_SHADER_TARGET_D3D_ASM
 * - VKD3D_SHADER_SOURCE_D3D_BYTECODE to VKD3D_SHADER_TARGET_SPIRV_BINARY
 * - VKD3D_SHADER_SOURCE_D3D_BYTECODE to VKD3D_SHADER_TARGET_SPIRV_TEXT
 *   (if vkd3d was compiled with SPIRV-Tools)
 * - VKD3D_SHADER_SOURCE_D3D_BYTECODE to VKD3D_SHADER_TARGET_D3D_ASM
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_SPIRV_BINARY
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_SPIRV_TEXT
 *   (if vkd3d was compiled with SPIRV-Tools)
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_D3D_ASM
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_D3D_BYTECODE
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_DXBC_TPF
 * - VKD3D_SHADER_SOURCE_HLSL to VKD3D_SHADER_TARGET_FX
 * - VKD3D_SHADER_SOURCE_FX to VKD3D_SHADER_TARGET_D3D_ASM
 *
 * Supported transformations can also be detected at runtime with the functions
 * vkd3d_shader_get_supported_source_types() and
 * vkd3d_shader_get_supported_target_types().
 *
 * Depending on the source and target types, this function may support the
 * following chained structures:
 * - vkd3d_shader_descriptor_offset_info
 * - vkd3d_shader_hlsl_source_info
 * - vkd3d_shader_interface_info
 * - vkd3d_shader_parameter_info
 * - vkd3d_shader_preprocess_info
 * - vkd3d_shader_scan_combined_resource_sampler_info
 * - vkd3d_shader_scan_descriptor_info
 * - vkd3d_shader_scan_signature_info
 * - vkd3d_shader_spirv_domain_shader_target_info
 * - vkd3d_shader_spirv_target_info
 * - vkd3d_shader_transform_feedback_info
 * - vkd3d_shader_varying_map_info
 *
 * \param compile_info A chained structure containing compilation parameters.
 *
 * \param out A pointer to a vkd3d_shader_code structure in which the compiled
 * code will be stored.
 * \n
 * The compiled shader is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the compiler.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * The messages returned can be regulated with the \a log_level member of struct
 * vkd3d_shader_compile_info. Regardless of the requested level, if this
 * parameter is NULL, no compilation messages will be returned.
 * \n
 * If no messages are produced by the compiler, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_compile(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);
/**
 * Free shader messages allocated by another vkd3d-shader function, such as
 * vkd3d_shader_compile().
 *
 * \param messages Messages to free. This pointer is optional and may be NULL,
 * in which case no action will be taken.
 */
VKD3D_SHADER_API void vkd3d_shader_free_messages(char *messages);
/**
 * Free shader code allocated by another vkd3d-shader function, such as
 * vkd3d_shader_compile().
 *
 * This function frees the \ref vkd3d_shader_code.code member, but does not free
 * the structure itself.
 *
 * \param code Code to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_shader_code(struct vkd3d_shader_code *code);

/**
 * Convert a byte code description of a shader root signature to a structural
 * description which can be easily parsed by C code.
 *
 * This function corresponds to
 * ID3D12VersionedRootSignatureDeserializer::GetUnconvertedRootSignatureDesc().
 *
 * This function performs the reverse transformation of
 * vkd3d_shader_serialize_root_signature().
 *
 * This function parses a standalone root signature, and should not be confused
 * with vkd3d_shader_parse_input_signature().
 *
 * \param dxbc Compiled byte code, in DXBC format.
 *
 * \param root_signature Output location in which the decompiled root signature
 * will be stored.
 * \n
 * Members of \a root_signature may be allocated by vkd3d-shader. The signature
 * should be freed with vkd3d_shader_free_root_signature() when no longer
 * needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the parser.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * If no messages are produced by the parser, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_parse_root_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages);
/**
 * Free a structural representation of a shader root signature allocated by
 * vkd3d_shader_convert_root_signature() or vkd3d_shader_parse_root_signature().
 *
 * This function may free members of struct
 * vkd3d_shader_versioned_root_signature_desc, but does not free the structure
 * itself.
 *
 * \param root_signature Signature description to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_root_signature(
        struct vkd3d_shader_versioned_root_signature_desc *root_signature);

/**
 * Convert a structural description of a shader root signature to a byte code
 * format capable of being read by ID3D12Device::CreateRootSignature. The
 * compiled signature is compatible with Microsoft D3D 12.
 *
 * This function corresponds to D3D12SerializeVersionedRootSignature().
 *
 * \param root_signature Description of the root signature.
 *
 * \param dxbc A pointer to a vkd3d_shader_code structure in which the compiled
 * code will be stored.
 * \n
 * The compiled signature is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the serializer.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * If no messages are produced by the serializer, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_serialize_root_signature(
        const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages);
/**
 * Convert a structural representation of a root signature to a different
 * version of structural representation.
 *
 * This function corresponds to
 * ID3D12VersionedRootSignatureDeserializer::GetRootSignatureDescAtVersion().
 *
 * \param dst A pointer to a vkd3d_shader_versioned_root_signature_desc
 * structure in which the converted signature will be stored.
 * \n
 * Members of \a dst may be allocated by vkd3d-shader. The signature should be
 * freed with vkd3d_shader_free_root_signature() when no longer needed.
 *
 * \param version The desired version to convert \a src to. This version must
 * not be equal to \a src->version.
 *
 * \param src Input root signature description.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_convert_root_signature(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src);

/**
 * Parse shader source code or byte code, returning various types of requested
 * information.
 *
 * The \a source_type member of \a compile_info must be set to the type of the
 * shader.
 *
 * The \a target_type member may be set to VKD3D_SHADER_TARGET_NONE, in which
 * case vkd3d_shader_scan() will return information about the shader in
 * isolation. Alternatively, it may be set to a valid compilation target for the
 * shader, in which case vkd3d_shader_scan() will return information that
 * reflects the interface for a shader as it will be compiled to that target.
 * In this case other chained structures may be appended to \a compile_info as
 * they would be passed to vkd3d_shader_compile(), and interpreted accordingly,
 * such as vkd3d_shader_spirv_target_info.
 *
 * (For a hypothetical example, suppose the source shader distinguishes float
 * and integer texture data, but the target environment does not support integer
 * textures. In this case vkd3d_shader_compile() might translate integer
 * operations to float. Accordingly using VKD3D_SHADER_TARGET_NONE would
 * accurately report whether the texture expects integer or float data, but
 * using the relevant specific target type would report
 * VKD3D_SHADER_RESOURCE_DATA_FLOAT.)
 *
 * Currently this function supports the following code types:
 * - VKD3D_SHADER_SOURCE_DXBC_TPF
 * - VKD3D_SHADER_SOURCE_D3D_BYTECODE
 *
 * \param compile_info A chained structure containing scan parameters.
 * \n
 * The scanner supports the following chained structures:
 * - vkd3d_shader_scan_descriptor_info
 * - vkd3d_shader_scan_signature_info
 * - vkd3d_shader_scan_combined_resource_sampler_info
 * \n
 * Although the \a compile_info parameter is read-only, chained structures
 * passed to this function need not be, and may serve as output parameters,
 * depending on their structure type.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the parser.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * The messages returned can be regulated with the \a log_level member of struct
 * vkd3d_shader_compile_info. Regardless of the requested level, if this
 * parameter is NULL, no compilation messages will be returned.
 * \n
 * If no messages are produced by the parser, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_scan(const struct vkd3d_shader_compile_info *compile_info, char **messages);
/**
 * Free members of struct vkd3d_shader_scan_descriptor_info() allocated by
 * vkd3d_shader_scan().
 *
 * This function may free members of vkd3d_shader_scan_descriptor_info, but
 * does not free the structure itself.
 *
 * \param scan_descriptor_info Descriptor information to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_scan_descriptor_info(
        struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info);

/**
 * Read the input signature of a compiled DXBC shader, returning a structural
 * description which can be easily parsed by C code.
 *
 * This function parses a compiled shader. To parse a standalone root signature,
 * use vkd3d_shader_parse_root_signature().
 *
 * This function only parses DXBC shaders, and only retrieves the input
 * signature. To retrieve signatures from other shader types, or other signature
 * types, use vkd3d_shader_scan() and struct vkd3d_shader_scan_signature_info.
 * This function returns the same input signature that is returned in
 * struct vkd3d_shader_scan_signature_info.
 *
 * \param dxbc Compiled byte code, in DXBC format.
 *
 * \param signature Output location in which the parsed root signature will be
 * stored.
 * \n
 * Members of \a signature may be allocated by vkd3d-shader. The signature
 * should be freed with vkd3d_shader_free_shader_signature() when no longer
 * needed.
 * \n
 * The signature may contain pointers into the input shader, and should only be
 * accessed while the input shader remains valid.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the parser.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * If no messages are produced by the parser, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 */
VKD3D_SHADER_API int vkd3d_shader_parse_input_signature(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages);
/**
 * Find a single element of a parsed input signature.
 *
 * \param signature The parsed input signature. This structure is normally
 * populated by vkd3d_shader_parse_input_signature().
 *
 * \param semantic_name Semantic name of the desired element. This function
 * performs a case-insensitive comparison with respect to the ASCII plane.
 *
 * \param semantic_index Semantic index of the desired element.
 *
 * \param stream_index Geometry shader stream index of the desired element. If
 * the signature is not a geometry shader output signature, this parameter must
 * be set to 0.
 *
 * \return A description of the element matching the requested parameters, or
 * NULL if no such element was found. If not NULL, the return value points into
 * the \a signature parameter and should not be explicitly freed.
 */
VKD3D_SHADER_API struct vkd3d_shader_signature_element *vkd3d_shader_find_signature_element(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index);
/**
 * Free a structural representation of a shader input signature allocated by
 * vkd3d_shader_parse_input_signature().
 *
 * This function may free members of struct vkd3d_shader_signature, but does not
 * free the structure itself.
 *
 * \param signature Signature description to free.
 */
VKD3D_SHADER_API void vkd3d_shader_free_shader_signature(struct vkd3d_shader_signature *signature);

/* 1.3 */

/**
 * Preprocess the given source code.
 *
 * This function supports the following chained structures:
 * - vkd3d_shader_preprocess_info
 *
 * \param compile_info A chained structure containing compilation parameters.
 *
 * \param out A pointer to a vkd3d_shader_code structure in which the
 * preprocessed code will be stored.
 * \n
 * The preprocessed shader is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the preprocessor.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * The messages returned can be regulated with the \a log_level member of struct
 * vkd3d_shader_compile_info. Regardless of the requested level, if this
 * parameter is NULL, no compilation messages will be returned.
 * \n
 * If no messages are produced by the preprocessor, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 *
 * \since 1.3
 */
VKD3D_SHADER_API int vkd3d_shader_preprocess(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);

/**
 * Set a callback to be called when vkd3d-shader outputs debug logging.
 *
 * If NULL, or if this function has not been called, libvkd3d-shader will print
 * all enabled log output to stderr.
 *
 * \param callback Callback function to set.
 *
 * \since 1.4
 */
VKD3D_SHADER_API void vkd3d_shader_set_log_callback(PFN_vkd3d_log callback);

/**
 * Free the contents of a vkd3d_shader_dxbc_desc structure allocated by
 * another vkd3d-shader function, such as vkd3d_shader_parse_dxbc().
 *
 * This function may free the \ref vkd3d_shader_dxbc_desc.sections member, but
 * does not free the structure itself.
 *
 * \param dxbc The vkd3d_shader_dxbc_desc structure to free.
 *
 * \since 1.7
 */
VKD3D_SHADER_API void vkd3d_shader_free_dxbc(struct vkd3d_shader_dxbc_desc *dxbc);

/**
 * Parse a DXBC blob contained in a vkd3d_shader_code structure.
 *
 * \param dxbc A vkd3d_shader_code structure containing the DXBC blob to parse.
 *
 * \param flags A combination of zero or more elements of enum
 * vkd3d_shader_parse_dxbc_flags.
 *
 * \param desc A vkd3d_shader_dxbc_desc structure describing the contents of
 * the DXBC blob. Its vkd3d_shader_dxbc_section_desc structures will contain
 * pointers into the input blob; its contents are only valid while the input
 * blob is valid. The contents of this structure should be freed with
 * vkd3d_shader_free_dxbc() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the parser.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * If no messages are produced by the parser, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 *
 * \since 1.7
 */
VKD3D_SHADER_API int vkd3d_shader_parse_dxbc(const struct vkd3d_shader_code *dxbc,
        uint32_t flags, struct vkd3d_shader_dxbc_desc *desc, char **messages);

/**
 * Serialize a DXBC description into a blob stored in a vkd3d_shader_code
 * structure.
 *
 * \param section_count The number of DXBC sections to serialize.
 *
 * \param sections An array of vkd3d_shader_dxbc_section_desc structures
 * to serialize.
 *
 * \param dxbc A pointer to a vkd3d_shader_code structure in which the
 * serialized blob will be stored.
 * \n
 * The output blob is allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_shader_code() when no longer needed.
 *
 * \param messages Optional output location for error or informational messages
 * produced by the serializer.
 * \n
 * This string is null-terminated and UTF-8 encoded.
 * \n
 * The messages are allocated by vkd3d-shader and should be freed with
 * vkd3d_shader_free_messages() when no longer needed.
 * \n
 * If no messages are produced by the serializer, this parameter may
 * receive NULL instead of a valid string pointer.
 *
 * \return A member of \ref vkd3d_result.
 *
 * \since 1.7
 */
VKD3D_SHADER_API int vkd3d_shader_serialize_dxbc(size_t section_count,
        const struct vkd3d_shader_dxbc_section_desc *sections, struct vkd3d_shader_code *dxbc, char **messages);

/**
 * Free members of struct vkd3d_shader_scan_signature_info allocated by
 * vkd3d_shader_scan().
 *
 * This function may free members of vkd3d_shader_scan_signature_info, but
 * does not free the structure itself.
 *
 * \param info Scan information to free.
 *
 * \since 1.9
 */
VKD3D_SHADER_API void vkd3d_shader_free_scan_signature_info(struct vkd3d_shader_scan_signature_info *info);

/**
 * Build a mapping of output varyings in a shader stage to input varyings in
 * the following shader stage.
 *
 * This mapping should be used in struct vkd3d_shader_varying_map_info to
 * compile the first shader.
 *
 * \param output_signature The output signature of the first shader.
 *
 * \param input_signature The input signature of the second shader.
 *
 * \param count On output, contains the number of entries written into
 * "varyings".
 *
 * \param varyings Pointer to an output array of varyings.
 * This must point to space for N varyings, where N is the number of elements
 * in the input signature.
 *
 * \remark Valid legacy Direct3D pixel shaders have at most 12 varying inputs:
 * 10 inter-stage varyings, face, and position.
 * Therefore, in practice, it is safe to call this function with a
 * pre-allocated array with a fixed size of 12.
 *
 * \since 1.9
 */
VKD3D_SHADER_API void vkd3d_shader_build_varying_map(const struct vkd3d_shader_signature *output_signature,
        const struct vkd3d_shader_signature *input_signature,
        unsigned int *count, struct vkd3d_shader_varying_map *varyings);

/**
 * Free members of struct vkd3d_shader_scan_combined_resource_sampler_info
 * allocated by vkd3d_shader_scan().
 *
 * This function may free members of
 * vkd3d_shader_scan_combined_resource_sampler_info, but does not free the
 * structure itself.
 *
 * \param info Combined resource-sampler information to free.
 *
 * \since 1.10
 */
VKD3D_SHADER_API void vkd3d_shader_free_scan_combined_resource_sampler_info(
        struct vkd3d_shader_scan_combined_resource_sampler_info *info);

#endif  /* VKD3D_SHADER_NO_PROTOTYPES */

/** Type of vkd3d_shader_get_version(). */
typedef const char *(*PFN_vkd3d_shader_get_version)(unsigned int *major, unsigned int *minor);
/** Type of vkd3d_shader_get_supported_source_types(). */
typedef const enum vkd3d_shader_source_type *(*PFN_vkd3d_shader_get_supported_source_types)(unsigned int *count);
/** Type of vkd3d_shader_get_supported_target_types(). */
typedef const enum vkd3d_shader_target_type *(*PFN_vkd3d_shader_get_supported_target_types)(
        enum vkd3d_shader_source_type source_type, unsigned int *count);

/** Type of vkd3d_shader_compile(). */
typedef int (*PFN_vkd3d_shader_compile)(const struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);
/** Type of vkd3d_shader_free_messages(). */
typedef void (*PFN_vkd3d_shader_free_messages)(char *messages);
/** Type of vkd3d_shader_free_shader_code(). */
typedef void (*PFN_vkd3d_shader_free_shader_code)(struct vkd3d_shader_code *code);

/** Type of vkd3d_shader_parse_root_signature(). */
typedef int (*PFN_vkd3d_shader_parse_root_signature)(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_versioned_root_signature_desc *root_signature, char **messages);
/** Type of vkd3d_shader_free_root_signature(). */
typedef void (*PFN_vkd3d_shader_free_root_signature)(struct vkd3d_shader_versioned_root_signature_desc *root_signature);

/** Type of vkd3d_shader_serialize_root_signature(). */
typedef int (*PFN_vkd3d_shader_serialize_root_signature)(
        const struct vkd3d_shader_versioned_root_signature_desc *root_signature,
        struct vkd3d_shader_code *dxbc, char **messages);

/** Type of vkd3d_shader_convert_root_signature(). */
typedef int (*PFN_vkd3d_shader_convert_root_signature)(struct vkd3d_shader_versioned_root_signature_desc *dst,
        enum vkd3d_shader_root_signature_version version, const struct vkd3d_shader_versioned_root_signature_desc *src);

/** Type of vkd3d_shader_scan(). */
typedef int (*PFN_vkd3d_shader_scan)(const struct vkd3d_shader_compile_info *compile_info, char **messages);
/** Type of vkd3d_shader_free_scan_descriptor_info(). */
typedef void (*PFN_vkd3d_shader_free_scan_descriptor_info)(
        struct vkd3d_shader_scan_descriptor_info *scan_descriptor_info);

/** Type of vkd3d_shader_parse_input_signature(). */
typedef int (*PFN_vkd3d_shader_parse_input_signature)(const struct vkd3d_shader_code *dxbc,
        struct vkd3d_shader_signature *signature, char **messages);
/** Type of vkd3d_shader_find_signature_element(). */
typedef struct vkd3d_shader_signature_element * (*PFN_vkd3d_shader_find_signature_element)(
        const struct vkd3d_shader_signature *signature, const char *semantic_name,
        unsigned int semantic_index, unsigned int stream_index);
/** Type of vkd3d_shader_free_shader_signature(). */
typedef void (*PFN_vkd3d_shader_free_shader_signature)(struct vkd3d_shader_signature *signature);

/** Type of vkd3d_shader_preprocess(). \since 1.3 */
typedef void (*PFN_vkd3d_shader_preprocess)(struct vkd3d_shader_compile_info *compile_info,
        struct vkd3d_shader_code *out, char **messages);

/** Type of vkd3d_shader_set_log_callback(). \since 1.4 */
typedef void (*PFN_vkd3d_shader_set_log_callback)(PFN_vkd3d_log callback);

/** Type of vkd3d_shader_free_dxbc(). \since 1.7 */
typedef void (*PFN_vkd3d_shader_free_dxbc)(struct vkd3d_shader_dxbc_desc *dxbc);
/** Type of vkd3d_shader_parse_dxbc(). \since 1.7 */
typedef int (*PFN_vkd3d_shader_parse_dxbc)(const struct vkd3d_shader_code *dxbc,
        uint32_t flags, struct vkd3d_shader_dxbc_desc *desc, char **messages);
/** Type of vkd3d_shader_serialize_dxbc(). \since 1.7 */
typedef int (*PFN_vkd3d_shader_serialize_dxbc)(size_t section_count,
        const struct vkd3d_shader_dxbc_section_desc *sections, struct vkd3d_shader_code *dxbc, char **messages);

/** Type of vkd3d_shader_build_varying_map(). \since 1.9 */
typedef void (*PFN_vkd3d_shader_build_varying_map)(const struct vkd3d_shader_signature *output_signature,
        const struct vkd3d_shader_signature *input_signature,
        unsigned int *count, struct vkd3d_shader_varying_map *varyings);
/** Type of vkd3d_shader_free_scan_signature_info(). \since 1.9 */
typedef void (*PFN_vkd3d_shader_free_scan_signature_info)(struct vkd3d_shader_scan_signature_info *info);

/** Type of vkd3d_shader_free_scan_combined_resource_sampler_info(). \since 1.10 */
typedef void (*PFN_vkd3d_shader_free_scan_combined_resource_sampler_info)(
        struct vkd3d_shader_scan_combined_resource_sampler_info *info);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __VKD3D_SHADER_H */
