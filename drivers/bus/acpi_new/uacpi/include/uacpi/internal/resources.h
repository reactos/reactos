#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/resources.h>

#ifndef UACPI_BAREBONES_MODE

enum uacpi_aml_resource {
    UACPI_AML_RESOURCE_TYPE_INVALID = 0,

    // Small resources
    UACPI_AML_RESOURCE_IRQ,
    UACPI_AML_RESOURCE_DMA,
    UACPI_AML_RESOURCE_START_DEPENDENT,
    UACPI_AML_RESOURCE_END_DEPENDENT,
    UACPI_AML_RESOURCE_IO,
    UACPI_AML_RESOURCE_FIXED_IO,
    UACPI_AML_RESOURCE_FIXED_DMA,
    UACPI_AML_RESOURCE_VENDOR_TYPE0,
    UACPI_AML_RESOURCE_END_TAG,

    // Large resources
    UACPI_AML_RESOURCE_MEMORY24,
    UACPI_AML_RESOURCE_GENERIC_REGISTER,
    UACPI_AML_RESOURCE_VENDOR_TYPE1,
    UACPI_AML_RESOURCE_MEMORY32,
    UACPI_AML_RESOURCE_FIXED_MEMORY32,
    UACPI_AML_RESOURCE_ADDRESS32,
    UACPI_AML_RESOURCE_ADDRESS16,
    UACPI_AML_RESOURCE_EXTENDED_IRQ,
    UACPI_AML_RESOURCE_ADDRESS64,
    UACPI_AML_RESOURCE_ADDRESS64_EXTENDED,
    UACPI_AML_RESOURCE_GPIO_CONNECTION,
    UACPI_AML_RESOURCE_PIN_FUNCTION,
    UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    UACPI_AML_RESOURCE_PIN_CONFIGURATION,
    UACPI_AML_RESOURCE_PIN_GROUP,
    UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION,
    UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION,
    UACPI_AML_RESOURCE_CLOCK_INPUT,
    UACPI_AML_RESOURCE_MAX = UACPI_AML_RESOURCE_CLOCK_INPUT,
};

enum uacpi_aml_resource_size_kind {
    UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
    UACPI_AML_RESOURCE_SIZE_KIND_FIXED_OR_ONE_LESS,
    UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
};

enum uacpi_aml_resource_kind {
    UACPI_AML_RESOURCE_KIND_SMALL = 0,
    UACPI_AML_RESOURCE_KIND_LARGE,
};

enum uacpi_resource_convert_opcode {
    UACPI_RESOURCE_CONVERT_OPCODE_END = 0,

    /*
     * AML -> native:
     * Take the mask at 'aml_offset' and convert to an array of uacpi_u8
     * at 'native_offset' with the value corresponding to the bit index.
     * The array size is written to the byte at offset 'arg2'.
     *
     * native -> AML:
     * Walk each element of the array at 'native_offset' and set the
     * corresponding bit in the mask at 'aml_offset' to 1. The array size is
     * read from the byte at offset 'arg2'.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_8,
    UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_16,

    /*
     * AML -> native:
     * Grab the bits at the byte at 'aml_offset' + 'bit_index', and copy its
     * value into the byte at 'native_offset'.
     *
     * native -> AML:
     * Grab first N bits at 'native_offset' and copy to 'aml_offset' starting
     * at the 'bit_index'.
     *
     * NOTE:
     * These must be contiguous in this order.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_1,
    UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_2,
    UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_3,
    UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_6 =
        UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_3 + 3,

    /*
     * AML -> native:
     * Copy N bytes at 'aml_offset' to 'native_offset'.
     *
     * native -> AML:
     * Copy N bytes at 'native_offset' to 'aml_offset'.
     *
     * 'imm' is added to the accumulator.
     *
     * NOTE: These are affected by the current value in the accumulator. If it's
     *       set to 0 at the time of evalution, this is executed once, N times
     *       otherwise. 0xFF is considered a special value, which resets the
     *       accumulator to 0 unconditionally.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_FIELD_8,
    UACPI_RESOURCE_CONVERT_OPCODE_FIELD_16,
    UACPI_RESOURCE_CONVERT_OPCODE_FIELD_32,
    UACPI_RESOURCE_CONVERT_OPCODE_FIELD_64,

    /*
     * If the length of the current resource is less than 'arg0', then skip
     * 'imm' instructions.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_SKIP_IF_AML_SIZE_LESS_THAN,

    /*
     * Skip 'imm' instructions if 'arg0' is not equal to the value in the
     * accumulator.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_SKIP_IF_NOT_EQUALS,

    /*
     * AML -> native:
     * Set the byte at 'native_offset' to 'imm'.
     *
     * native -> AML:
     * Set the byte at 'aml_offset' to 'imm'.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_SET_TO_IMM,

    /*
     * AML -> native:
     * Load the AML resoruce length into the accumulator as well as the field at
     * 'native_offset' of width N.
     *
     * native -> AML:
     * Load the resource length into the accumulator.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_AML_SIZE_32,

    /*
     * AML -> native:
     * Load the 8 bit field at 'aml_offset' into the accumulator and store at
     * 'native_offset'.
     *
     * native -> AML:
     * Load the 8 bit field at 'native_offset' into the accumulator and store
     * at 'aml_offset'.
     *
     * The accumulator is multiplied by 'imm' unless it's set to zero.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_8_STORE,

    /*
     * Load the N bit field at 'native_offset' into the accumulator
     */
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_8_NATIVE,
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_16_NATIVE,

    /*
     * Load 'imm' into the accumulator.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_IMM,

    /*
     * AML -> native:
     * Load the resource source at offset = aml size + accumulator into the
     * uacpi_resource_source struct at 'native_offset'. The string bytes are
     * written to the offset at resource size + accumulator. The presence is
     * detected by comparing the length of the resource to the offset,
     * 'arg2' optionally specifies the offset to the upper bound of the string.
     *
     * native -> AML:
     * Load the resource source from the uacpi_resource_source struct at
     * 'native_offset' to aml_size + accumulator. aml_size + accumulator is
     * optionally written to 'aml_offset' if it's specified.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE,
    UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE_NO_INDEX,
    UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL,

    /*
     * AML -> native:
     * Load the pin table with upper bound specified at 'aml_offset'.
     * The table length is calculated by subtracting the upper bound from
     * aml_size and is written into the accumulator.
     *
     * native -> AML:
     * Load the pin table length from 'native_offset' and multiply by 2, store
     * the result in the accumulator.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_LOAD_PIN_TABLE_LENGTH,

    /*
     * AML -> native:
     * Store the accumulator divided by 2 at 'native_offset'.
     * The table is copied to the offset at resource size from offset at
     * aml_size with the pointer written to the offset at 'arg2'.
     *
     * native -> AML:
     * Read the pin table from resource size offset, write aml_size to
     * 'aml_offset'. Copy accumulator bytes to the offset at aml_size.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_PIN_TABLE,

    /*
     * AML -> native:
     * Load vendor data with offset stored at 'aml_offset'. The length is
     * calculated as aml_size - aml_offset and is written to 'native_offset'.
     * The data is written to offset - aml_size with the pointer written back
     * to the offset at 'arg2'.
     *
     * native -> AML:
     * Read vendor data from the pointer at offset 'arg2' and size at
     * 'native_offset', the offset to write to is calculated as the difference
     * between the data pointer and the native resource end pointer.
     * offset + aml_size is written to 'aml_offset' and the data is copied
     * there as well.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_VENDOR_DATA,

    /*
     * AML -> native:
     * Read the serial type from the byte at 'aml_offset' and write it to the
     * type field of the uacpi_resource_serial_bus_common structure. Convert
     * the serial type to native and set the resource type to it. Copy the
     * vendor data to the offset at native size, the length is calculated
     * as type_data_length - extra-type-specific-size, and is written to
     * vendor_data_length, as well as the accumulator. The data pointer is
     * written to vendor_data.
     *
     * native -> AML:
     * Set the serial type at 'aml_offset' to the value stored at
     * 'native_offset'. Load the vendor data to the offset at aml_size,
     * the length is read from 'vendor_data_length', and the data is copied from
     * 'vendor_data'.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_SERIAL_TYPE_SPECIFIC,

    /*
     * Produces an error if encountered in the instruction stream.
     * Used to trap invalid/unexpected code flow.
     */
    UACPI_RESOURCE_CONVERT_OPCODE_UNREACHABLE,
};

struct uacpi_resource_convert_instruction {
    uacpi_u8 code;

    union {
        uacpi_u8 aml_offset;
        uacpi_u8 arg0;
    };

    union {
        uacpi_u8 native_offset;
        uacpi_u8 arg1;
    };

    union {
        uacpi_u8 imm;
        uacpi_u8 bit_index;
        uacpi_u8 arg2;
    };
};

struct uacpi_resource_spec {
    uacpi_u8 type : 5;
    uacpi_u8 native_type : 5;
    uacpi_u8 resource_kind : 1;
    uacpi_u8 size_kind : 2;

    /*
     * Size of the resource as appears in the AML byte stream, for variable
     * length resources this is the minimum.
     */
    uacpi_u16 aml_size;

    /*
     * Size of the native human-readable uacpi resource, for variable length
     * resources this is the minimum. The final length is this field plus the
     * result of extra_size_for_native().
     */
    uacpi_u16 native_size;

    /*
     * Calculate the amount of extra bytes that must be allocated for a specific
     * native resource given the AML counterpart. This being NULL means no extra
     * bytes are needed, aka native resources is always the same size.
     */
    uacpi_size (*extra_size_for_native)(
        const struct uacpi_resource_spec*, void*, uacpi_size
    );

    /*
     * Calculate the number of bytes needed to represent a native resource as
     * AML. The 'aml_size' field is used if this is NULL.
     */
    uacpi_size (*size_for_aml)(
        const struct uacpi_resource_spec*, uacpi_resource*
    );

    const struct uacpi_resource_convert_instruction *to_native;
    const struct uacpi_resource_convert_instruction *to_aml;
};

typedef uacpi_iteration_decision (*uacpi_aml_resource_iteration_callback)(
    void*, uacpi_u8 *data, uacpi_u16 resource_size,
    const struct uacpi_resource_spec*
);

uacpi_status uacpi_for_each_aml_resource(
    uacpi_data_view, uacpi_aml_resource_iteration_callback cb, void *user
);

uacpi_status uacpi_find_aml_resource_end_tag(
    uacpi_data_view, uacpi_size *out_offset
);

uacpi_status uacpi_native_resources_from_aml(
    uacpi_data_view, uacpi_resources **out_resources
);

uacpi_status uacpi_native_resources_to_aml(
    uacpi_resources *resources, uacpi_object **out_template
);

#endif // !UACPI_BAREBONES_MODE
