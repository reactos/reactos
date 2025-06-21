#include <uacpi/internal/io.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/opregion.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/namespace.h>

#ifndef UACPI_BAREBONES_MODE

uacpi_size uacpi_round_up_bits_to_bytes(uacpi_size bit_length)
{
    return UACPI_ALIGN_UP(bit_length, 8, uacpi_size) / 8;
}

static void cut_misaligned_tail(
    uacpi_u8 *data, uacpi_size offset, uacpi_u32 bit_length
)
{
    uacpi_u8 remainder = bit_length & 7;

    if (remainder == 0)
        return;

    data[offset] &= ((1ull << remainder) - 1);
}

struct bit_span
{
    union {
        uacpi_u8 *data;
        const uacpi_u8 *const_data;
    };
    uacpi_u64 index;
    uacpi_u64 length;
};

static uacpi_size bit_span_offset(struct bit_span *span, uacpi_size bits)
{
    uacpi_size delta = UACPI_MIN(span->length, bits);

    span->index += delta;
    span->length -= delta;

    return delta;
}

static void bit_copy(struct bit_span *dst, struct bit_span *src)
{
    uacpi_u8 src_shift, dst_shift, bits = 0;
    uacpi_u16 dst_mask;
    uacpi_u8 *dst_ptr, *src_ptr;
    uacpi_u64 dst_count, src_count;

    dst_ptr = dst->data + (dst->index / 8);
    src_ptr = src->data + (src->index / 8);

    dst_count = dst->length;
    dst_shift = dst->index & 7;

    src_count = src->length;
    src_shift = src->index & 7;

    while (dst_count)
    {
        bits = 0;

        if (src_count) {
            bits = *src_ptr >> src_shift;

            if (src_shift && src_count > (uacpi_u32)(8 - src_shift))
                bits |= *(src_ptr + 1) << (8 - src_shift);

            if (src_count < 8) {
                bits &= (1 << src_count) - 1;
                src_count = 0;
            } else {
                src_count -= 8;
                src_ptr++;
            }
        }

        dst_mask = (dst_count < 8 ? (1 << dst_count) - 1 : 0xFF) << dst_shift;
        *dst_ptr = (*dst_ptr & ~dst_mask) | ((bits << dst_shift) & dst_mask);

        if (dst_shift && dst_count > (uacpi_u32)(8 - dst_shift)) {
            dst_mask >>= 8;
            *(dst_ptr + 1) &= ~dst_mask;
            *(dst_ptr + 1) |= (bits >> (8 - dst_shift)) & dst_mask;
        }

        dst_count = dst_count > 8 ? dst_count - 8 : 0;
        ++dst_ptr;
    }
}

static void do_misaligned_buffer_read(
    const uacpi_buffer_field *field, uacpi_u8 *dst
)
{
    struct bit_span src_span = {
        .index = field->bit_index,
        .length = field->bit_length,
        .const_data = field->backing->data,
    };
    struct bit_span dst_span = {
        .data = dst,
    };

    dst_span.length = uacpi_round_up_bits_to_bytes(field->bit_length) * 8;
    bit_copy(&dst_span, &src_span);
}

void uacpi_read_buffer_field(
    const uacpi_buffer_field *field, void *dst
)
{
    if (!(field->bit_index & 7)) {
        uacpi_u8 *src = field->backing->data;
        uacpi_size count;

        count = uacpi_round_up_bits_to_bytes(field->bit_length);
        uacpi_memcpy(dst, src + (field->bit_index / 8), count);
        cut_misaligned_tail(dst, count - 1, field->bit_length);
        return;
    }

    do_misaligned_buffer_read(field, dst);
}

static void do_write_misaligned_buffer_field(
    uacpi_buffer_field *field,
    const void *src, uacpi_size size
)
{
    struct bit_span src_span = {
        .length = size * 8,
        .const_data = src,
    };
    struct bit_span dst_span = {
        .index = field->bit_index,
        .length = field->bit_length,
        .data = field->backing->data,
    };

    bit_copy(&dst_span, &src_span);
}

void uacpi_write_buffer_field(
    uacpi_buffer_field *field,
    const void *src, uacpi_size size
)
{
    if (!(field->bit_index & 7)) {
        uacpi_u8 *dst, last_byte, tail_shift;
        uacpi_size count;

        dst = field->backing->data;
        dst += field->bit_index / 8;
        count = uacpi_round_up_bits_to_bytes(field->bit_length);

        last_byte = dst[count - 1];
        tail_shift = field->bit_length & 7;

        uacpi_memcpy_zerout(dst, src, count, size);
        if (tail_shift) {
            uacpi_u8 last_shift = 8 - tail_shift;
            dst[count - 1] = dst[count - 1] << last_shift;
            dst[count - 1] >>= last_shift;
            dst[count - 1] |= (last_byte >> tail_shift) << tail_shift;
        }

        return;
    }

    do_write_misaligned_buffer_field(field, src, size);
}

static uacpi_status access_field_unit(
    uacpi_field_unit *field, uacpi_u32 offset, uacpi_region_op op,
    union uacpi_opregion_io_data data
)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (field->lock_rule) {
        ret = uacpi_acquire_aml_mutex(
            g_uacpi_rt_ctx.global_lock_mutex, 0xFFFF
        );
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    switch (field->kind) {
    case UACPI_FIELD_UNIT_KIND_BANK:
        ret = uacpi_write_field_unit(
            field->bank_selection, &field->bank_value, sizeof(field->bank_value),
            UACPI_NULL
        );
        break;
    case UACPI_FIELD_UNIT_KIND_NORMAL:
        break;
    case UACPI_FIELD_UNIT_KIND_INDEX:
        ret = uacpi_write_field_unit(
            field->index, &offset, sizeof(offset),
            UACPI_NULL
        );
        if (uacpi_unlikely_error(ret))
            goto out;

        switch (op) {
        case UACPI_REGION_OP_READ:
            ret = uacpi_read_field_unit(
                field->data, data.integer, field->access_width_bytes,
                UACPI_NULL
            );
            break;
        case UACPI_REGION_OP_WRITE:
            ret = uacpi_write_field_unit(
                field->data, data.integer, field->access_width_bytes,
                UACPI_NULL
            );
            break;
        default:
            ret = UACPI_STATUS_INVALID_ARGUMENT;
            break;
        }

        goto out;

    default:
        uacpi_error("invalid field unit kind %d\n", field->kind);
        ret = UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (uacpi_unlikely_error(ret))
        goto out;

    ret = uacpi_dispatch_opregion_io(field, offset, op, data);

out:
    if (field->lock_rule)
        uacpi_release_aml_mutex(g_uacpi_rt_ctx.global_lock_mutex);
    return ret;
}

#define OPREGION_IO_U64(x) (union uacpi_opregion_io_data) { .integer = x }

#define SERIAL_HEADER_SIZE 2
#define IPMI_DATA_SIZE 64

static uacpi_status wtr_buffer_size(
    uacpi_field_unit *field, uacpi_address_space space,
    uacpi_size *out_size
)
{
    switch (space) {
    case UACPI_ADDRESS_SPACE_IPMI:
        *out_size = SERIAL_HEADER_SIZE + IPMI_DATA_SIZE;
        break;
    case UACPI_ADDRESS_SPACE_PRM:
        *out_size = 26;
        break;
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
        *out_size = 256;
        break;
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
    case UACPI_ADDRESS_SPACE_SMBUS: {
        uacpi_size size_for_protocol = SERIAL_HEADER_SIZE;

        switch (field->attributes) {
        case UACPI_ACCESS_ATTRIBUTE_QUICK:
            break; // + 0
        case UACPI_ACCESS_ATTRIBUTE_SEND_RECEIVE:
        case UACPI_ACCESS_ATTRIBUTE_BYTE:
            size_for_protocol += 1;
            break;

        case UACPI_ACCESS_ATTRIBUTE_WORD:
        case UACPI_ACCESS_ATTRIBUTE_PROCESS_CALL:
            size_for_protocol += 2;
            break;

        case UACPI_ACCESS_ATTRIBUTE_BYTES:
            size_for_protocol += field->access_length;
            break;

        case UACPI_ACCESS_ATTRIBUTE_BLOCK:
        case UACPI_ACCESS_ATTRIBUTE_BLOCK_PROCESS_CALL:
        case UACPI_ACCESS_ATTRIBUTE_RAW_BYTES:
        case UACPI_ACCESS_ATTRIBUTE_RAW_PROCESS_BYTES:
            size_for_protocol += 255;
            break;

        default:
            uacpi_error(
                "unsupported field@%p access attribute %d\n",
                field, field->attributes
            );
            return UACPI_STATUS_UNIMPLEMENTED;
        }

        *out_size = size_for_protocol;
        break;
    }
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_special_field(
    uacpi_field_unit *field, uacpi_data_view buf,
    uacpi_region_op op, uacpi_data_view *wtr_response,
    uacpi_bool *did_handle
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_object *obj;
    uacpi_operation_region *region;
    uacpi_u64 in_out;
    uacpi_data_view wtr_buffer;

    *did_handle = UACPI_FALSE;

    if (field->kind == UACPI_FIELD_UNIT_KIND_INDEX)
        return ret;

    obj = uacpi_namespace_node_get_object_typed(
        field->region, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL)) {
        ret = UACPI_STATUS_INVALID_ARGUMENT;
        uacpi_trace_region_error(
            field->region, "attempted access to deleted", ret
        );
        goto out_handled;
    }
    region = obj->op_region;

    switch (region->space) {
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        if (op == UACPI_REGION_OP_WRITE) {
            uacpi_memcpy_zerout(
                &in_out, buf.const_data, sizeof(in_out), buf.length
            );
        }

        ret = access_field_unit(field, 0, op, OPREGION_IO_U64(&in_out));
        if (uacpi_unlikely_error(ret))
            goto out_handled;

        if (op == UACPI_REGION_OP_READ)
            uacpi_memcpy_zerout(buf.data, &in_out, buf.length, sizeof(in_out));
        goto out_handled;
    case UACPI_ADDRESS_SPACE_IPMI:
    case UACPI_ADDRESS_SPACE_PRM:
        if (uacpi_unlikely(op == UACPI_REGION_OP_READ)) {
            ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
            uacpi_trace_region_error(
                field->region, "attempted to read from a write-only", ret
            );
            goto out_handled;
        }
        UACPI_FALLTHROUGH;
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
    case UACPI_ADDRESS_SPACE_SMBUS:
        goto do_wtr;
    default:
        return ret;
    }

do_wtr:
    ret = wtr_buffer_size(field, region->space, &wtr_buffer.length);
    if (uacpi_unlikely_error(ret))
        goto out_handled;

    wtr_buffer.data = uacpi_kernel_alloc(wtr_buffer.length);
    if (uacpi_unlikely(wtr_buffer.data == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out_handled;
    }

    uacpi_memcpy_zerout(
        wtr_buffer.data, buf.const_data, wtr_buffer.length, buf.length
    );
    ret = access_field_unit(
        field, field->byte_offset,
        op, (union uacpi_opregion_io_data) {
            .buffer = wtr_buffer,
        }
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_free(wtr_buffer.data, wtr_buffer.length);
        goto out_handled;
    }

    if (wtr_response != UACPI_NULL)
        *wtr_response = wtr_buffer;

out_handled:
    *did_handle = UACPI_TRUE;
    return ret;
}

static uacpi_status do_read_misaligned_field_unit(
    uacpi_field_unit *field, uacpi_u8 *dst, uacpi_size size
)
{
    uacpi_status ret;
    uacpi_size reads_to_do;
    uacpi_u64 out;
    uacpi_u32 byte_offset = field->byte_offset;
    uacpi_u32 bits_left = field->bit_length;
    uacpi_u8 width_access_bits = field->access_width_bytes * 8;

    struct bit_span src_span = {
        .data = (uacpi_u8*)&out,
        .index = field->bit_offset_within_first_byte,
    };
    struct bit_span dst_span = {
        .data = dst,
        .index = 0,
        .length = size * 8
    };

    reads_to_do = UACPI_ALIGN_UP(
        field->bit_offset_within_first_byte + field->bit_length,
        width_access_bits,
        uacpi_u32
    );
    reads_to_do /= width_access_bits;

    while (reads_to_do-- > 0) {
        src_span.length = UACPI_MIN(
            bits_left, width_access_bits - src_span.index
        );

        ret = access_field_unit(
            field, byte_offset, UACPI_REGION_OP_READ,
            OPREGION_IO_U64(&out)
        );
        if (uacpi_unlikely_error(ret))
            return ret;

        bit_copy(&dst_span, &src_span);
        bits_left -= src_span.length;
        src_span.index = 0;

        bit_span_offset(&dst_span, src_span.length);
        byte_offset += field->access_width_bytes;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_read_field_unit(
    uacpi_field_unit *field, void *dst, uacpi_size size,
    uacpi_data_view *wtr_response
)
{
    uacpi_status ret;
    uacpi_u32 field_byte_length;
    uacpi_bool did_handle;

    ret = handle_special_field(
        field, (uacpi_data_view) {
            .data = dst,
            .length = size,
        }, UACPI_REGION_OP_READ,
        wtr_response, &did_handle
    );
    if (did_handle)
        return ret;

    field_byte_length = uacpi_round_up_bits_to_bytes(field->bit_length);

    /*
     * Very simple fast case:
     * - Bit offset within first byte is 0
     * AND
     * - Field size is <= access width
     */
    if (field->bit_offset_within_first_byte == 0 &&
        field_byte_length <= field->access_width_bytes)
    {
        uacpi_u64 out;

        ret = access_field_unit(
            field, field->byte_offset, UACPI_REGION_OP_READ,
            OPREGION_IO_U64(&out)
        );
        if (uacpi_unlikely_error(ret))
            return ret;

        uacpi_memcpy_zerout(dst, &out, size, field_byte_length);
        if (size >= field_byte_length)
            cut_misaligned_tail(dst, field_byte_length - 1, field->bit_length);

        return UACPI_STATUS_OK;
    }

    // Slow case
    return do_read_misaligned_field_unit(field, dst, size);
}

static uacpi_status write_generic_field_unit(
    uacpi_field_unit *field, const void *src, uacpi_size size
)
{
    uacpi_status ret;
    uacpi_u32 bits_left, byte_offset = field->byte_offset;
    uacpi_u8 width_access_bits = field->access_width_bytes * 8;
    uacpi_u64 in;

    struct bit_span src_span = {
        .const_data = src,
        .index = 0,
        .length = size * 8
    };
    struct bit_span dst_span = {
        .data = (uacpi_u8*)&in,
        .index = field->bit_offset_within_first_byte,
    };

    bits_left = field->bit_length;

    while (bits_left) {
        in = 0;
        dst_span.length = UACPI_MIN(
            width_access_bits - dst_span.index, bits_left
        );

        if (dst_span.index != 0 || dst_span.length < width_access_bits) {
            switch (field->update_rule) {
            case UACPI_UPDATE_RULE_PRESERVE:
                ret = access_field_unit(
                    field, byte_offset, UACPI_REGION_OP_READ,
                    OPREGION_IO_U64(&in)
                );
                if (uacpi_unlikely_error(ret))
                    return ret;
                break;
            case UACPI_UPDATE_RULE_WRITE_AS_ONES:
                in = ~in;
                break;
            case UACPI_UPDATE_RULE_WRITE_AS_ZEROES:
                break;
            default:
                uacpi_error("invalid field@%p update rule %d\n",
                            field, field->update_rule);
                return UACPI_STATUS_INVALID_ARGUMENT;
            }
        }

        bit_copy(&dst_span, &src_span);
        bit_span_offset(&src_span, dst_span.length);

        ret = access_field_unit(
            field, byte_offset, UACPI_REGION_OP_WRITE,
            OPREGION_IO_U64(&in)
        );
        if (uacpi_unlikely_error(ret))
            return ret;

        bits_left -= dst_span.length;
        dst_span.index = 0;
        byte_offset += field->access_width_bytes;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_write_field_unit(
    uacpi_field_unit *field, const void *src, uacpi_size size,
    uacpi_data_view *wtr_response
)
{
    uacpi_status ret;
    uacpi_bool did_handle;

    ret = handle_special_field(
        field, (uacpi_data_view) {
            .const_data = src,
            .length = size,
        }, UACPI_REGION_OP_WRITE,
        wtr_response, &did_handle
    );
    if (did_handle)
        return ret;

    return write_generic_field_unit(field, src, size);
}

uacpi_status uacpi_field_unit_get_read_type(
    struct uacpi_field_unit *field, uacpi_object_type *out_type
)
{
    uacpi_object *obj;

    if (field->kind == UACPI_FIELD_UNIT_KIND_INDEX)
        goto out_basic_field;

    obj = uacpi_namespace_node_get_object_typed(
        field->region, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (uacpi_is_buffer_access_address_space(obj->op_region->space)) {
        *out_type = UACPI_OBJECT_BUFFER;
        return UACPI_STATUS_OK;
    }

out_basic_field:
    if (field->bit_length > (g_uacpi_rt_ctx.is_rev1 ? 32u : 64u))
        *out_type = UACPI_OBJECT_BUFFER;
    else
        *out_type = UACPI_OBJECT_INTEGER;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_field_unit_get_bit_length(
    struct uacpi_field_unit *field, uacpi_size *out_length
)
{
    uacpi_object *obj;

    if (field->kind == UACPI_FIELD_UNIT_KIND_INDEX)
        goto out_basic_field;

    obj = uacpi_namespace_node_get_object_typed(
        field->region, UACPI_OBJECT_OPERATION_REGION_BIT
    );
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (uacpi_is_buffer_access_address_space(obj->op_region->space)) {
        /*
         * Bit length is protocol specific, the data will be returned
         * via the write-then-read response buffer.
         */
        *out_length = 0;
        return UACPI_STATUS_OK;
    }

out_basic_field:
    *out_length = field->bit_length;
    return UACPI_STATUS_OK;
}

static uacpi_u8 gas_get_access_bit_width(const struct acpi_gas *gas)
{
    /*
     * Same algorithm as ACPICA.
     *
     * The reason we do this is apparently GAS bit offset being non-zero means
     * that it's an APEI register, as opposed to FADT, which needs special
     * handling. In the case of a FADT register we want to ignore the specified
     * access size.
     */
    uacpi_u8 access_bit_width;

    if (gas->register_bit_offset == 0 &&
        UACPI_IS_POWER_OF_TWO(gas->register_bit_width, uacpi_u8) &&
        UACPI_IS_ALIGNED(gas->register_bit_width, 8, uacpi_u8)) {
        access_bit_width = gas->register_bit_width;
    } else if (gas->access_size) {
        access_bit_width = gas->access_size * 8;
    } else {
        uacpi_u8 msb;

        msb = uacpi_bit_scan_backward(
            (gas->register_bit_offset + gas->register_bit_width) - 1
        );
        access_bit_width = 1 << msb;

        if (access_bit_width <= 8) {
            access_bit_width = 8;
        } else {
            /*
             * Keep backing off to previous power of two until we find one
             * that is aligned to the address specified in GAS.
             */
            while (!UACPI_IS_ALIGNED(
                gas->address, access_bit_width / 8, uacpi_u64
            ))
                access_bit_width /= 2;
        }
    }

    return UACPI_MIN(
        access_bit_width,
        gas->address_space_id == UACPI_ADDRESS_SPACE_SYSTEM_IO ? 32 : 64
    );
}

static uacpi_status gas_validate(
    const struct acpi_gas *gas, uacpi_u8 *access_bit_width,
    uacpi_u8 *bit_width
)
{
    uacpi_size total_width, aligned_width;

    if (uacpi_unlikely(gas == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (!gas->address)
        return UACPI_STATUS_NOT_FOUND;

    if (gas->address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_IO &&
        gas->address_space_id != UACPI_ADDRESS_SPACE_SYSTEM_MEMORY) {
        uacpi_warn("unsupported GAS address space '%s' (%d)\n",
                   uacpi_address_space_to_string(gas->address_space_id),
                   gas->address_space_id);
        return UACPI_STATUS_UNIMPLEMENTED;
    }

    if (gas->access_size > 4) {
        uacpi_warn("unsupported GAS access size %d\n",
                   gas->access_size);
        return UACPI_STATUS_UNIMPLEMENTED;
    }

    *access_bit_width = gas_get_access_bit_width(gas);

    total_width = gas->register_bit_offset + gas->register_bit_width;
    aligned_width = UACPI_ALIGN_UP(total_width, *access_bit_width, uacpi_size);

    if (uacpi_unlikely(aligned_width > 64)) {
        uacpi_warn(
            "GAS register total width is too large: %zu\n", total_width
        );
        return UACPI_STATUS_UNIMPLEMENTED;
    }

    *bit_width = total_width;
    return UACPI_STATUS_OK;
}

/*
 * Apparently both reading and writing GAS works differently from operation
 * region in that bit offsets are not respected when writing the data.
 *
 * Let's follow ACPICA's approach here so that we don't accidentally
 * break any quirky hardware.
 */
uacpi_status uacpi_gas_read_mapped(
    const uacpi_mapped_gas *gas, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_u8 access_byte_width;
    uacpi_u8 bit_offset, bits_left, index = 0;
    uacpi_u64 data, mask = 0xFFFFFFFFFFFFFFFF;
    uacpi_size offset = 0;

    bit_offset = gas->bit_offset;
    bits_left = gas->total_bit_width;

    access_byte_width = gas->access_bit_width / 8;

    if (access_byte_width < 8)
        mask = ~(mask << gas->access_bit_width);

    *out_value = 0;

    while (bits_left) {
        if (bit_offset >= gas->access_bit_width) {
            data = 0;
            bit_offset -= gas->access_bit_width;
        } else {
            ret = gas->read(gas->mapping, offset, access_byte_width, &data);
            if (uacpi_unlikely_error(ret))
                return ret;
        }

        *out_value |= (data & mask) << (index * gas->access_bit_width);
        bits_left -= UACPI_MIN(bits_left, gas->access_bit_width);
        ++index;
        offset += access_byte_width;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_gas_write_mapped(
    const uacpi_mapped_gas *gas, uacpi_u64 in_value
)
{
    uacpi_status ret;
    uacpi_u8 access_byte_width;
    uacpi_u8 bit_offset, bits_left, index = 0;
    uacpi_u64 data, mask = 0xFFFFFFFFFFFFFFFF;
    uacpi_size offset = 0;

    bit_offset = gas->bit_offset;
    bits_left = gas->total_bit_width;
    access_byte_width = gas->access_bit_width / 8;

    if (access_byte_width < 8)
        mask = ~(mask << gas->access_bit_width);

    while (bits_left) {
        data = (in_value >> (index * gas->access_bit_width)) & mask;

        if (bit_offset >= gas->access_bit_width) {
            bit_offset -= gas->access_bit_width;
        } else {
            ret = gas->write(gas->mapping, offset, access_byte_width, data);
            if (uacpi_unlikely_error(ret))
                return ret;
        }

        bits_left -= UACPI_MIN(bits_left, gas->access_bit_width);
        ++index;
        offset += access_byte_width;
    }

    return UACPI_STATUS_OK;
}

static void unmap_gas_io(uacpi_handle io_handle, uacpi_size size)
{
    UACPI_UNUSED(size);
    uacpi_kernel_io_unmap(io_handle);
}

uacpi_status uacpi_map_gas_noalloc(
    const struct acpi_gas *gas, uacpi_mapped_gas *out_mapped
)
{
    uacpi_status ret;
    uacpi_u8 access_bit_width, total_width;

    ret = gas_validate(gas, &access_bit_width, &total_width);
    if (ret != UACPI_STATUS_OK)
        return ret;

    if (gas->address_space_id == UACPI_ADDRESS_SPACE_SYSTEM_MEMORY) {
        out_mapped->mapping = uacpi_kernel_map(gas->address, total_width / 8);
        if (uacpi_unlikely(out_mapped->mapping == UACPI_NULL))
            return UACPI_STATUS_MAPPING_FAILED;

        out_mapped->read = uacpi_system_memory_read;
        out_mapped->write = uacpi_system_memory_write;
        out_mapped->unmap = uacpi_kernel_unmap;
    } else { // IO, validated by gas_validate above
        ret = uacpi_kernel_io_map(gas->address, total_width / 8, &out_mapped->mapping);
        if (uacpi_unlikely_error(ret))
            return ret;

        out_mapped->read = uacpi_system_io_read;
        out_mapped->write = uacpi_system_io_write;
        out_mapped->unmap = unmap_gas_io;
    }

    out_mapped->access_bit_width = access_bit_width;
    out_mapped->total_bit_width = total_width;
    out_mapped->bit_offset = gas->register_bit_offset;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_map_gas(
    const struct acpi_gas *gas, uacpi_mapped_gas **out_mapped
)
{
    uacpi_status ret;
    uacpi_mapped_gas *mapping;

    mapping = uacpi_kernel_alloc(sizeof(*mapping));
    if (uacpi_unlikely(mapping == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ret = uacpi_map_gas_noalloc(gas, mapping);
    if (uacpi_unlikely_error(ret)) {
        uacpi_free(mapping, sizeof(*mapping));
        return ret;
    }

    *out_mapped = mapping;
    return ret;
}

void uacpi_unmap_gas_nofree(uacpi_mapped_gas *gas)
{
    gas->unmap(gas->mapping, gas->access_bit_width / 8);
}

void uacpi_unmap_gas(uacpi_mapped_gas *gas)
{
    uacpi_unmap_gas_nofree(gas);
    uacpi_free(gas, sizeof(*gas));
}

uacpi_status uacpi_gas_read(const struct acpi_gas *gas, uacpi_u64 *out_value)
{
    uacpi_status ret;
    uacpi_mapped_gas mapping;

    ret = uacpi_map_gas_noalloc(gas, &mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_gas_read_mapped(&mapping, out_value);
    uacpi_unmap_gas_nofree(&mapping);

    return ret;
}

uacpi_status uacpi_gas_write(const struct acpi_gas *gas, uacpi_u64 in_value)
{
    uacpi_status ret;
    uacpi_mapped_gas mapping;

    ret = uacpi_map_gas_noalloc(gas, &mapping);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_gas_write_mapped(&mapping, in_value);
    uacpi_unmap_gas_nofree(&mapping);

    return ret;
}

uacpi_status uacpi_system_memory_read(
    void *ptr, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
)
{
    ptr = UACPI_PTR_ADD(ptr, offset);

    switch (width) {
    case 1:
        *out = *(volatile uacpi_u8*)ptr;
        break;
    case 2:
        *out = *(volatile uacpi_u16*)ptr;
        break;
    case 4:
        *out = *(volatile uacpi_u32*)ptr;
        break;
    case 8:
        *out = *(volatile uacpi_u64*)ptr;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_system_memory_write(
    void *ptr, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
)
{
    ptr = UACPI_PTR_ADD(ptr, offset);

    switch (width) {
    case 1:
        *(volatile uacpi_u8*)ptr = in;
        break;
    case 2:
        *(volatile uacpi_u16*)ptr = in;
        break;
    case 4:
        *(volatile uacpi_u32*)ptr = in;
        break;
    case 8:
        *(volatile uacpi_u64*)ptr = in;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

union integer_data {
    uacpi_u8 byte;
    uacpi_u16 word;
    uacpi_u32 dword;
    uacpi_u64 qword;
};

uacpi_status uacpi_system_io_read(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
)
{
    uacpi_status ret;
    union integer_data data = {
        .qword = 0,
    };

    switch (width) {
    case 1:
        ret = uacpi_kernel_io_read8(handle, offset, &data.byte);
        break;
    case 2:
        ret = uacpi_kernel_io_read16(handle, offset, &data.word);
        break;
    case 4:
        ret = uacpi_kernel_io_read32(handle, offset, &data.dword);
        break;
    default:
        uacpi_error(
            "invalid SystemIO read %p@%zu width=%d\n",
            handle, offset, width
        );
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (uacpi_likely_success(ret))
        *out = data.qword;
    return ret;
}

uacpi_status uacpi_system_io_write(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
)
{
    uacpi_status ret;

    switch (width) {
    case 1:
        ret = uacpi_kernel_io_write8(handle, offset, in);
        break;
    case 2:
        ret = uacpi_kernel_io_write16(handle, offset, in);
        break;
    case 4:
        ret = uacpi_kernel_io_write32(handle, offset, in);
        break;
    default:
        uacpi_error(
            "invalid SystemIO write %p@%zu width=%d\n",
            handle, offset, width
        );
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return ret;
}

uacpi_status uacpi_pci_read(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 *out
)
{
    uacpi_status ret;
    union integer_data data = {
        .qword = 0,
    };

    switch (width) {
    case 1:
        ret = uacpi_kernel_pci_read8(handle, offset, &data.byte);
        break;
    case 2:
        ret = uacpi_kernel_pci_read16(handle, offset, &data.word);
        break;
    case 4:
        ret = uacpi_kernel_pci_read32(handle, offset, &data.dword);
        break;
    default:
        uacpi_error(
            "invalid PCI_Config read %p@%zu width=%d\n",
            handle, offset, width
        );
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (uacpi_likely_success(ret))
        *out = data.qword;
    return ret;
}

uacpi_status uacpi_pci_write(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 width, uacpi_u64 in
)
{
    uacpi_status ret;

    switch (width) {
    case 1:
        ret = uacpi_kernel_pci_write8(handle, offset, in);
        break;
    case 2:
        ret = uacpi_kernel_pci_write16(handle, offset, in);
        break;
    case 4:
        ret = uacpi_kernel_pci_write32(handle, offset, in);
        break;
    default:
        uacpi_error(
            "invalid PCI_Config write %p@%zu width=%d\n",
            handle, offset, width
        );
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return ret;
}

#endif // !UACPI_BAREBONES_MODE
