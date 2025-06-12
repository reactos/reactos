#include <uacpi/internal/types.h>
#include <uacpi/internal/interpreter.h>
#include <uacpi/internal/dynamic_array.h>
#include <uacpi/internal/opcodes.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/context.h>
#include <uacpi/internal/shareable.h>
#include <uacpi/internal/tables.h>
#include <uacpi/internal/helpers.h>
#include <uacpi/kernel_api.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/opregion.h>
#include <uacpi/internal/io.h>
#include <uacpi/internal/notify.h>
#include <uacpi/internal/resources.h>
#include <uacpi/internal/event.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/osi.h>

#ifndef UACPI_BAREBONES_MODE

enum item_type {
    ITEM_NONE = 0,
    ITEM_NAMESPACE_NODE,
    ITEM_OBJECT,
    ITEM_EMPTY_OBJECT,
    ITEM_PACKAGE_LENGTH,
    ITEM_IMMEDIATE,
};

struct package_length {
    uacpi_u32 begin;
    uacpi_u32 end;
};

struct item {
    uacpi_u8 type;
    union {
        uacpi_handle handle;
        uacpi_object *obj;
        struct uacpi_namespace_node *node;
        struct package_length pkg;
        uacpi_u64 immediate;
        uacpi_u8 immediate_bytes[8];
    };
};

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(item_array, struct item, 8)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(item_array, struct item, static)

struct op_context {
    uacpi_u8 pc;
    uacpi_bool preempted;

    /*
     * == 0 -> none
     * >= 1 -> item[idx - 1]
     */
    uacpi_u8 tracked_pkg_idx;

    uacpi_aml_op switched_from;

    const struct uacpi_op_spec *op;
    struct item_array items;
};

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(op_context_array, struct op_context, 8)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    op_context_array, struct op_context, static
)

static struct op_context *op_context_array_one_before_last(
    struct op_context_array *arr
)
{
    uacpi_size size;

    size = op_context_array_size(arr);

    if (size < 2)
        return UACPI_NULL;

    return op_context_array_at(arr, size - 2);
}

enum code_block_type {
    CODE_BLOCK_IF = 1,
    CODE_BLOCK_ELSE = 2,
    CODE_BLOCK_WHILE = 3,
    CODE_BLOCK_SCOPE = 4,
};

struct code_block {
    enum code_block_type type;
    uacpi_u32 begin, end;
    union {
        struct uacpi_namespace_node *node;
        uacpi_u64 expiration_point;
    };
};

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(code_block_array, struct code_block, 8)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    code_block_array, struct code_block, static
)

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(held_mutexes_array, uacpi_mutex*, 8)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    held_mutexes_array, uacpi_mutex*, static
)

static uacpi_status held_mutexes_array_push(
    struct held_mutexes_array *arr, uacpi_mutex *mutex
)
{
    uacpi_mutex **slot;

    slot = held_mutexes_array_alloc(arr);
    if (uacpi_unlikely(slot == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    *slot = mutex;
    uacpi_shareable_ref(mutex);
    return UACPI_STATUS_OK;
}

static void held_mutexes_array_remove_idx(
    struct held_mutexes_array *arr, uacpi_size i
)
{
    uacpi_size size;

    size = held_mutexes_array_inline_capacity(arr);

    // Only the dynamic array part is affected
    if (i >= size) {
        i -= size;
        size = arr->size_including_inline - size;
        size -= i + 1;

        uacpi_memmove(
            &arr->dynamic_storage[i], &arr->dynamic_storage[i + 1],
            size * sizeof(arr->inline_storage[0])
        );

        held_mutexes_array_pop(arr);
        return;
    }

    size = UACPI_MIN(held_mutexes_array_inline_capacity(arr),
                     arr->size_including_inline);
    size -= i + 1;
    uacpi_memmove(
        &arr->inline_storage[i], &arr->inline_storage[i + 1],
        size * sizeof(arr->inline_storage[0])
    );

    size = held_mutexes_array_size(arr);
    i = held_mutexes_array_inline_capacity(arr);

    /*
     * This array has dynamic storage as well, now we have to take the first
     * dynamic item, move it to the top of inline storage, and then shift all
     * dynamic items backward by 1 as well.
     */
    if (size > i) {
        arr->inline_storage[i - 1] = arr->dynamic_storage[0];
        size -= i + 1;

        uacpi_memmove(
            &arr->dynamic_storage[0], &arr->dynamic_storage[1],
            size * sizeof(arr->inline_storage[0])
        );
    }

    held_mutexes_array_pop(arr);
}

enum force_release {
    FORCE_RELEASE_NO,
    FORCE_RELEASE_YES,
};

static uacpi_status held_mutexes_array_remove_and_release(
    struct held_mutexes_array *arr, uacpi_mutex *mutex,
    enum force_release force
)
{
    uacpi_mutex *item;
    uacpi_size i;

    if (uacpi_unlikely(held_mutexes_array_size(arr) == 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    item = *held_mutexes_array_last(arr);

    if (uacpi_unlikely(item->sync_level != mutex->sync_level &&
                       force != FORCE_RELEASE_YES)) {
        uacpi_warn(
            "ignoring mutex @%p release due to sync level mismatch: %d vs %d\n",
            mutex, mutex->sync_level, item->sync_level
        );

        // We still return OK because we don't want to abort because of this
        return UACPI_STATUS_OK;
    }

    if (mutex->depth > 1 && force == FORCE_RELEASE_NO) {
        uacpi_release_aml_mutex(mutex);
        return UACPI_STATUS_OK;
    }

    // Fast path for well-behaved AML that releases mutexes in descending order
    if (uacpi_likely(item == mutex)) {
        held_mutexes_array_pop(arr);
        goto do_release;
    }

    /*
     * The mutex being released is not the last one acquired, although we did
     * verify that at least it has the same sync level. Anyway, now we have
     * to search for it and then remove it from the array while shifting
     * everything backwards.
     */
    i = held_mutexes_array_size(arr);
    for (;;) {
        item = *held_mutexes_array_at(arr, --i);
        if (item == mutex)
            break;

        if (uacpi_unlikely(i == 0))
            return UACPI_STATUS_INVALID_ARGUMENT;
    }

    held_mutexes_array_remove_idx(arr, i);

do_release:
    // This is either a force release, or depth was already 1 to begin with
    mutex->depth = 1;
    uacpi_release_aml_mutex(mutex);

    uacpi_mutex_unref(mutex);
    return UACPI_STATUS_OK;
}

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(
    temp_namespace_node_array, uacpi_namespace_node*, 8)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    temp_namespace_node_array, uacpi_namespace_node*, static
)

static uacpi_status temp_namespace_node_array_push(
    struct temp_namespace_node_array *arr, uacpi_namespace_node *node
)
{
    uacpi_namespace_node **slot;

    slot = temp_namespace_node_array_alloc(arr);
    if (uacpi_unlikely(slot == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    *slot = node;
    return UACPI_STATUS_OK;
}

struct call_frame {
    struct uacpi_control_method *method;

    uacpi_object *args[7];
    uacpi_object *locals[8];

    struct op_context_array pending_ops;
    struct code_block_array code_blocks;
    struct temp_namespace_node_array temp_nodes;
    struct code_block *last_while;
    uacpi_u64 prev_while_expiration;
    uacpi_u32 prev_while_code_offset;

    uacpi_u32 code_offset;

    struct uacpi_namespace_node *cur_scope;

    // Only used if the method is serialized
    uacpi_u8 prev_sync_level;
};

static void *call_frame_cursor(struct call_frame *frame)
{
    return frame->method->code + frame->code_offset;
}

static uacpi_size call_frame_code_bytes_left(struct call_frame *frame)
{
    return frame->method->size - frame->code_offset;
}

static uacpi_bool call_frame_has_code(struct call_frame *frame)
{
    return call_frame_code_bytes_left(frame) > 0;
}

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(call_frame_array, struct call_frame, 4)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    call_frame_array, struct call_frame, static
)

static struct call_frame *call_frame_array_one_before_last(
    struct call_frame_array *arr
)
{
    uacpi_size size;

    size = call_frame_array_size(arr);

    if (size < 2)
        return UACPI_NULL;

    return call_frame_array_at(arr, size - 2);
}

// NOTE: Try to keep size under 2 pages
struct execution_context {
    uacpi_object *ret;
    struct call_frame_array call_stack;
    struct held_mutexes_array held_mutexes;

    struct call_frame *cur_frame;
    struct code_block *cur_block;
    const struct uacpi_op_spec *cur_op;
    struct op_context *prev_op_ctx;
    struct op_context *cur_op_ctx;

    uacpi_u8 sync_level;
};

#define AML_READ(ptr, offset) (*(((uacpi_u8*)(ptr)) + offset))

static uacpi_status parse_nameseg(uacpi_u8 *cursor,
                                  uacpi_object_name *out_name)
{
    if (uacpi_unlikely(!uacpi_is_valid_nameseg(cursor)))
        return UACPI_STATUS_AML_INVALID_NAMESTRING;

    uacpi_memcpy(&out_name->id, cursor, 4);
    return UACPI_STATUS_OK;
}

/*
 * -------------------------------------------------------------
 * RootChar := ‘\’
 * ParentPrefixChar := ‘^’
 * ‘\’ := 0x5C
 * ‘^’ := 0x5E
 * ------------------------------------------------------------
 * NameSeg := <leadnamechar namechar namechar namechar>
 * NameString := <rootchar namepath> | <prefixpath namepath>
 * PrefixPath := Nothing | <’^’ prefixpath>
 * NamePath := NameSeg | DualNamePath | MultiNamePath | NullName
 * DualNamePath := DualNamePrefix NameSeg NameSeg
 * MultiNamePath := MultiNamePrefix SegCount NameSeg(SegCount)
 */

static uacpi_status name_string_to_path(
    struct call_frame *frame, uacpi_size offset,
    uacpi_char **out_string, uacpi_size *out_size
)
{
    uacpi_size bytes_left, prefix_bytes, nameseg_bytes = 0, namesegs;
    uacpi_char *base_cursor, *cursor;
    uacpi_char prev_char;

    bytes_left = frame->method->size - offset;
    cursor = (uacpi_char*)frame->method->code + offset;
    base_cursor = cursor;
    namesegs = 0;

    prefix_bytes = 0;
    for (;;) {
        if (uacpi_unlikely(bytes_left == 0))
            return UACPI_STATUS_AML_INVALID_NAMESTRING;

        prev_char = *cursor;

        switch (prev_char) {
        case '^':
        case '\\':
            prefix_bytes++;
            cursor++;
            bytes_left--;
            break;
        default:
            break;
        }

        if (prev_char != '^')
            break;
    }

    // At least a NullName byte is expected here
    if (uacpi_unlikely(bytes_left == 0))
        return UACPI_STATUS_AML_INVALID_NAMESTRING;

    namesegs = 0;
    bytes_left--;
    switch (*cursor++)
    {
    case UACPI_DUAL_NAME_PREFIX:
        namesegs = 2;
        break;
    case UACPI_MULTI_NAME_PREFIX:
        if (uacpi_unlikely(bytes_left == 0))
            return UACPI_STATUS_AML_INVALID_NAMESTRING;

        namesegs = *(uacpi_u8*)cursor;
        if (uacpi_unlikely(namesegs == 0)) {
            uacpi_error("MultiNamePrefix but SegCount is 0\n");
            return UACPI_STATUS_AML_INVALID_NAMESTRING;
        }

        cursor++;
        bytes_left--;
        break;
    case UACPI_NULL_NAME:
        break;
    default:
        /*
         * Might be an invalid byte, but assume single nameseg for now,
         * the code below will validate it for us.
         */
        cursor--;
        bytes_left++;
        namesegs = 1;
        break;
    }

    if (uacpi_unlikely((namesegs * 4) > bytes_left))
        return UACPI_STATUS_AML_INVALID_NAMESTRING;

    if (namesegs) {
        // 4 chars per nameseg
        nameseg_bytes = namesegs * 4;

        // dot separator for every nameseg
        nameseg_bytes += namesegs - 1;
    }

    *out_size = nameseg_bytes + prefix_bytes + 1;

    *out_string = uacpi_kernel_alloc(*out_size);
    if (*out_string == UACPI_NULL)
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_memcpy(*out_string, base_cursor, prefix_bytes);

    base_cursor = *out_string;
    base_cursor += prefix_bytes;

    while (namesegs-- > 0) {
        uacpi_memcpy(base_cursor, cursor, 4);
        cursor += 4;
        base_cursor += 4;

        if (namesegs)
            *base_cursor++ = '.';
    }

    *base_cursor = '\0';
    return UACPI_STATUS_OK;
}

enum resolve_behavior {
    RESOLVE_CREATE_LAST_NAMESEG_FAIL_IF_EXISTS,
    RESOLVE_FAIL_IF_DOESNT_EXIST,
};

static uacpi_status resolve_name_string(
    struct call_frame *frame,
    enum resolve_behavior behavior,
    struct uacpi_namespace_node **out_node
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_u8 *cursor;
    uacpi_size bytes_left, namesegs = 0;
    struct uacpi_namespace_node *parent, *cur_node = frame->cur_scope;
    uacpi_char prev_char = 0;
    uacpi_bool just_one_nameseg = UACPI_TRUE;

    bytes_left = call_frame_code_bytes_left(frame);
    cursor = call_frame_cursor(frame);

    for (;;) {
        if (uacpi_unlikely(bytes_left == 0))
            return UACPI_STATUS_AML_INVALID_NAMESTRING;

        switch (*cursor) {
        case '\\':
            if (prev_char == '^')
                return UACPI_STATUS_AML_INVALID_NAMESTRING;

            cur_node = uacpi_namespace_root();
            break;
        case '^':
            // Tried to go behind root
            if (uacpi_unlikely(cur_node == uacpi_namespace_root()))
                return UACPI_STATUS_AML_INVALID_NAMESTRING;

            cur_node = cur_node->parent;
            break;
        default:
            break;
        }

        prev_char = *cursor;

        switch (prev_char) {
        case '^':
        case '\\':
            just_one_nameseg = UACPI_FALSE;
            cursor++;
            bytes_left--;
            break;
        default:
            break;
        }

        if (prev_char != '^')
            break;
    }

    // At least a NullName byte is expected here
    if (uacpi_unlikely(bytes_left == 0))
        return UACPI_STATUS_AML_INVALID_NAMESTRING;

    bytes_left--;
    switch (*cursor++)
    {
    case UACPI_DUAL_NAME_PREFIX:
        namesegs = 2;
        just_one_nameseg = UACPI_FALSE;
        break;
    case UACPI_MULTI_NAME_PREFIX:
        if (uacpi_unlikely(bytes_left == 0))
            return UACPI_STATUS_AML_INVALID_NAMESTRING;

        namesegs = *cursor;
        if (uacpi_unlikely(namesegs == 0)) {
            uacpi_error("MultiNamePrefix but SegCount is 0\n");
            return UACPI_STATUS_AML_INVALID_NAMESTRING;
        }

        cursor++;
        bytes_left--;
        just_one_nameseg = UACPI_FALSE;
        break;
    case UACPI_NULL_NAME:
        if (behavior == RESOLVE_CREATE_LAST_NAMESEG_FAIL_IF_EXISTS ||
            just_one_nameseg)
            return UACPI_STATUS_AML_INVALID_NAMESTRING;

        goto out;
    default:
        /*
         * Might be an invalid byte, but assume single nameseg for now,
         * the code below will validate it for us.
         */
        cursor--;
        bytes_left++;
        namesegs = 1;
        break;
    }

    if (uacpi_unlikely((namesegs * 4) > bytes_left))
        return UACPI_STATUS_AML_INVALID_NAMESTRING;

    for (; namesegs; cursor += 4, namesegs--) {
        uacpi_object_name name;

        ret = parse_nameseg(cursor, &name);
        if (uacpi_unlikely_error(ret))
            return ret;

        parent = cur_node;
        cur_node = uacpi_namespace_node_find_sub_node(parent, name);

        switch (behavior) {
        case RESOLVE_CREATE_LAST_NAMESEG_FAIL_IF_EXISTS:
            if (namesegs == 1) {
                if (cur_node) {
                    cur_node = UACPI_NULL;
                    ret = UACPI_STATUS_AML_OBJECT_ALREADY_EXISTS;
                    goto out;
                }

                // Create the node and link to parent but don't install YET
                cur_node = uacpi_namespace_node_alloc(name);
                if (uacpi_unlikely(cur_node == UACPI_NULL)) {
                    ret = UACPI_STATUS_OUT_OF_MEMORY;
                    goto out;
                }

                cur_node->parent = parent;
            }
            break;
        case RESOLVE_FAIL_IF_DOESNT_EXIST:
            if (just_one_nameseg) {
                while (!cur_node && parent != uacpi_namespace_root()) {
                    cur_node = parent;
                    parent = cur_node->parent;

                    cur_node = uacpi_namespace_node_find_sub_node(parent, name);
                }
            }
            break;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        if (cur_node == UACPI_NULL) {
            ret = UACPI_STATUS_NOT_FOUND;
            break;
        }
    }

out:
    cursor += namesegs * 4;
    frame->code_offset = cursor - frame->method->code;

    if (uacpi_likely_success(ret) && behavior == RESOLVE_FAIL_IF_DOESNT_EXIST)
        uacpi_shareable_ref(cur_node);

    *out_node = cur_node;
    return ret;
}

static uacpi_status do_install_node_item(struct call_frame *frame,
                                         struct item *item)
{
    uacpi_status ret;

    ret = uacpi_namespace_node_install(item->node->parent, item->node);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (!frame->method->named_objects_persist)
        ret = temp_namespace_node_array_push(&frame->temp_nodes, item->node);

    if (uacpi_likely_success(ret))
        item->node = UACPI_NULL;

    return ret;
}

static uacpi_u8 peek_next_op(struct call_frame *frame, uacpi_aml_op *out_op)
{
    uacpi_aml_op op;
    uacpi_size bytes_left;
    uacpi_u8 length = 0;
    uacpi_u8 *cursor;
    struct code_block *block;

    block = code_block_array_last(&frame->code_blocks);
    bytes_left = block->end - frame->code_offset;
    if (bytes_left == 0)
        return 0;

    cursor = call_frame_cursor(frame);

    op = AML_READ(cursor, length++);
    if (op == UACPI_EXT_PREFIX) {
        if (uacpi_unlikely(bytes_left < 2))
            return 0;

        op <<= 8;
        op |= AML_READ(cursor, length++);
    }

    *out_op = op;
    return length;
}

static uacpi_status get_op(struct execution_context *ctx)
{
    uacpi_aml_op op;
    uacpi_u8 length;

    length = peek_next_op(ctx->cur_frame, &op);
    if (uacpi_unlikely(length == 0))
        return UACPI_STATUS_AML_BAD_ENCODING;

    ctx->cur_frame->code_offset += length;
    g_uacpi_rt_ctx.opcodes_executed++;

    ctx->cur_op = uacpi_get_op_spec(op);
    if (uacpi_unlikely(ctx->cur_op->properties & UACPI_OP_PROPERTY_RESERVED)) {
        uacpi_error(
            "invalid opcode '%s' encountered in bytestream\n",
            ctx->cur_op->name
        );
        return UACPI_STATUS_AML_INVALID_OPCODE;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_buffer(struct execution_context *ctx)
{
    struct package_length *pkg;
    uacpi_u8 *src;
    uacpi_object *dst, *declared_size;
    uacpi_u32 buffer_size, init_size, aml_offset;
    struct op_context *op_ctx = ctx->cur_op_ctx;

    aml_offset = item_array_at(&op_ctx->items, 2)->immediate;
    src = ctx->cur_frame->method->code;
    src += aml_offset;

    pkg = &item_array_at(&op_ctx->items, 0)->pkg;
    init_size = pkg->end - aml_offset;

    // TODO: do package bounds checking at parse time
    if (uacpi_unlikely(pkg->end > ctx->cur_frame->method->size))
        return UACPI_STATUS_AML_BAD_ENCODING;

    declared_size = item_array_at(&op_ctx->items, 1)->obj;

    if (uacpi_unlikely(declared_size->integer > 0xE0000000)) {
        uacpi_error(
            "buffer is too large (%"UACPI_PRIu64"), assuming corrupted "
            "bytestream\n", UACPI_FMT64(declared_size->integer)
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    if (uacpi_unlikely(declared_size->integer == 0)) {
        uacpi_error("attempted to create an empty buffer\n");
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    buffer_size = declared_size->integer;
    if (uacpi_unlikely(init_size > buffer_size)) {
        uacpi_error(
            "too many buffer initializers: %u (size is %u)\n",
            init_size, buffer_size
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    dst = item_array_at(&op_ctx->items, 3)->obj;
    dst->buffer->data = uacpi_kernel_alloc(buffer_size);
    if (uacpi_unlikely(dst->buffer->data == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;
    dst->buffer->size = buffer_size;

    uacpi_memcpy_zerout(dst->buffer->data, src, buffer_size, init_size);
    return UACPI_STATUS_OK;
}

static uacpi_status handle_string(struct execution_context *ctx)
{
    struct call_frame *frame = ctx->cur_frame;
    uacpi_object *obj;

    uacpi_char *string;
    uacpi_size length, max_bytes;

    obj = item_array_last(&ctx->cur_op_ctx->items)->obj;
    string = call_frame_cursor(frame);

    // TODO: sanitize string for valid UTF-8
    max_bytes = call_frame_code_bytes_left(frame);
    length = uacpi_strnlen(string, max_bytes);

    if (uacpi_unlikely((length == max_bytes) || (string[length++] != 0x00)))
        return UACPI_STATUS_AML_BAD_ENCODING;

    obj->buffer->text = uacpi_kernel_alloc(length);
    if (uacpi_unlikely(obj->buffer->text == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_memcpy(obj->buffer->text, string, length);
    obj->buffer->size = length;
    frame->code_offset += length;
    return UACPI_STATUS_OK;
}

static uacpi_status handle_package(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_package *package;
    uacpi_u32 num_elements, num_defined_elements, i;

    /*
     * Layout of items here:
     * [0] -> Package length, not interesting
     * [1] -> Immediate or integer object, depending on PackageOp/VarPackageOp
     * [2..N-2] -> AML pc+Package element pairs
     * [N-1] -> The resulting package object that we're constructing
     */
    package = item_array_last(&op_ctx->items)->obj->package;

    // 1. Detect how many elements we have, do sanity checking
    if (op_ctx->op->code == UACPI_AML_OP_VarPackageOp) {
        uacpi_object *var_num_elements;

        var_num_elements = item_array_at(&op_ctx->items, 1)->obj;
        if (uacpi_unlikely(var_num_elements->integer > 0xE0000000)) {
            uacpi_error(
                "package is too large (%"UACPI_PRIu64"), assuming "
                "corrupted bytestream\n", UACPI_FMT64(var_num_elements->integer)
            );
            return UACPI_STATUS_AML_BAD_ENCODING;
        }
        num_elements = var_num_elements->integer;
    } else {
        num_elements = item_array_at(&op_ctx->items, 1)->immediate;
    }

    num_defined_elements = (item_array_size(&op_ctx->items) - 3) / 2;
    if (uacpi_unlikely(num_defined_elements > num_elements)) {
        uacpi_warn(
            "too many package initializers: %u, truncating to %u\n",
            num_defined_elements, num_elements
        );

        num_defined_elements = num_elements;
    }

    // 2. Create every object in the package, start as uninitialized
    if (uacpi_unlikely(!uacpi_package_fill(package, num_elements,
                                           UACPI_PREALLOC_OBJECTS_YES)))
        return UACPI_STATUS_OUT_OF_MEMORY;

    // 3. Go through every defined object and copy it into the package
    for (i = 0; i < num_defined_elements; ++i) {
        uacpi_size base_pkg_index;
        uacpi_status ret;
        struct item *item;
        uacpi_object *obj;

        base_pkg_index = (i * 2) + 2;
        item = item_array_at(&op_ctx->items, base_pkg_index + 1);
        obj = item->obj;

        if (obj != UACPI_NULL && obj->type == UACPI_OBJECT_REFERENCE) {
            /*
             * For named objects we don't actually need the object itself, but
             * simply the path to it. Often times objects referenced by the
             * package are not defined until later so it's not possible to
             * resolve them. For uniformity and to follow the behavior of NT,
             * simply convert the name string to a path string object to be
             * resolved later when actually needed.
             */
            if (obj->flags == UACPI_REFERENCE_KIND_NAMED) {
                uacpi_object_unref(obj);
                item->obj = UACPI_NULL;
                obj = UACPI_NULL;
            } else {
                obj = uacpi_unwrap_internal_reference(obj);
            }
        }

        if (obj == UACPI_NULL) {
            uacpi_size length;
            uacpi_char *path;

            obj = uacpi_create_object(UACPI_OBJECT_STRING);
            if (uacpi_unlikely(obj == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;

            ret = name_string_to_path(
                ctx->cur_frame,
                item_array_at(&op_ctx->items, base_pkg_index)->immediate,
                &path, &length
            );
            if (uacpi_unlikely_error(ret))
                return ret;

            obj->flags = UACPI_STRING_KIND_PATH;
            obj->buffer->text = path;
            obj->buffer->size = length;

            item->obj = obj;
            item->type = ITEM_OBJECT;
        }

        ret = uacpi_object_assign(package->objects[i], obj,
                                  UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    return UACPI_STATUS_OK;
}

static uacpi_size sizeof_int(void)
{
    return g_uacpi_rt_ctx.is_rev1 ? 4 : 8;
}

static uacpi_status get_object_storage(
    uacpi_object *obj, uacpi_data_view *out_buf, uacpi_bool include_null
)
{
    switch (obj->type) {
    case UACPI_OBJECT_INTEGER:
        out_buf->length = sizeof_int();
        out_buf->data = &obj->integer;
        break;
    case UACPI_OBJECT_STRING:
        out_buf->length = obj->buffer->size;
        if (out_buf->length && !include_null)
            out_buf->length--;

        out_buf->text = obj->buffer->text;
        break;
    case UACPI_OBJECT_BUFFER:
        if (obj->buffer->size == 0) {
            out_buf->bytes = UACPI_NULL;
            out_buf->length = 0;
            break;
        }

        out_buf->length = obj->buffer->size;
        out_buf->bytes = obj->buffer->data;
        break;
    case UACPI_OBJECT_REFERENCE:
        return UACPI_STATUS_INVALID_ARGUMENT;
    default:
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return UACPI_STATUS_OK;
}

static uacpi_u8 *buffer_index_cursor(uacpi_buffer_index *buf_idx)
{
    uacpi_u8 *out_cursor;

    out_cursor = buf_idx->buffer->data;
    out_cursor += buf_idx->idx;

    return out_cursor;
}

static void write_buffer_index(uacpi_buffer_index *buf_idx,
                               uacpi_data_view *src_buf)
{
    uacpi_memcpy_zerout(buffer_index_cursor(buf_idx), src_buf->bytes,
                        1, src_buf->length);
}

/*
 * The word "implicit cast" here is only because it's called that in
 * the specification. In reality, we just copy one buffer to another
 * because that's what NT does.
 */
static uacpi_status object_assign_with_implicit_cast(
    uacpi_object *dst, uacpi_object *src, uacpi_data_view *wtr_response
)
{
    uacpi_status ret;
    uacpi_data_view src_buf;

    ret = get_object_storage(src, &src_buf, UACPI_FALSE);
    if (uacpi_unlikely_error(ret))
        goto out_bad_cast;

    switch (dst->type) {
    case UACPI_OBJECT_INTEGER:
    case UACPI_OBJECT_STRING:
    case UACPI_OBJECT_BUFFER: {
        uacpi_data_view dst_buf;

        ret = get_object_storage(dst, &dst_buf, UACPI_FALSE);
        if (uacpi_unlikely_error(ret))
            goto out_bad_cast;

        uacpi_memcpy_zerout(
            dst_buf.bytes, src_buf.bytes, dst_buf.length, src_buf.length
        );
        break;
    }

    case UACPI_OBJECT_BUFFER_FIELD:
        uacpi_write_buffer_field(
            &dst->buffer_field, src_buf.bytes, src_buf.length
        );
        break;

    case UACPI_OBJECT_FIELD_UNIT:
        return uacpi_write_field_unit(
            dst->field_unit, src_buf.bytes, src_buf.length,
            wtr_response
        );

    case UACPI_OBJECT_BUFFER_INDEX:
        write_buffer_index(&dst->buffer_index, &src_buf);
        break;

    default:
        ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
        goto out_bad_cast;
    }

    return ret;

out_bad_cast:
    uacpi_error(
        "attempted to perform an invalid implicit cast (%s -> %s)\n",
        uacpi_object_type_to_string(src->type),
        uacpi_object_type_to_string(dst->type)
    );
    return ret;
}

enum argx_or_localx {
    ARGX,
    LOCALX,
};

static uacpi_status handle_arg_or_local(
    struct execution_context *ctx,
    uacpi_size idx, enum argx_or_localx type
)
{
    uacpi_object **src;
    struct item *dst;
    enum uacpi_reference_kind kind;

    if (type == ARGX) {
        src = &ctx->cur_frame->args[idx];
        kind = UACPI_REFERENCE_KIND_ARG;
    } else {
        src = &ctx->cur_frame->locals[idx];
        kind = UACPI_REFERENCE_KIND_LOCAL;
    }

    if (*src == UACPI_NULL) {
        uacpi_object *default_value;

        default_value = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
        if (uacpi_unlikely(default_value == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        *src = uacpi_create_internal_reference(kind, default_value);
        if (uacpi_unlikely(*src == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        uacpi_object_unref(default_value);
    }

    dst = item_array_last(&ctx->cur_op_ctx->items);
    dst->obj = *src;
    dst->type = ITEM_OBJECT;
    uacpi_object_ref(dst->obj);

    return UACPI_STATUS_OK;
}

static uacpi_status handle_local(struct execution_context *ctx)
{
    uacpi_size idx;
    struct op_context *op_ctx = ctx->cur_op_ctx;

    idx = op_ctx->op->code - UACPI_AML_OP_Local0Op;
    return handle_arg_or_local(ctx, idx, LOCALX);
}

static uacpi_status handle_arg(struct execution_context *ctx)
{
    uacpi_size idx;
    struct op_context *op_ctx = ctx->cur_op_ctx;

    idx = op_ctx->op->code - UACPI_AML_OP_Arg0Op;
    return handle_arg_or_local(ctx, idx, ARGX);
}

static uacpi_status handle_named_object(struct execution_context *ctx)
{
    struct uacpi_namespace_node *src;
    struct item *dst;

    src = item_array_at(&ctx->cur_op_ctx->items, 0)->node;
    dst = item_array_at(&ctx->cur_op_ctx->items, 1);

    dst->obj = src->object;
    dst->type = ITEM_OBJECT;
    uacpi_object_ref(dst->obj);

    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_alias(struct execution_context *ctx)
{
    uacpi_namespace_node *src, *dst;

    src = item_array_at(&ctx->cur_op_ctx->items, 0)->node;
    dst = item_array_at(&ctx->cur_op_ctx->items, 1)->node;

    dst->object = src->object;
    dst->flags = UACPI_NAMESPACE_NODE_FLAG_ALIAS;
    uacpi_object_ref(dst->object);

    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_op_region(struct execution_context *ctx)
{
    uacpi_namespace_node *node;
    uacpi_object *obj;
    uacpi_operation_region *op_region;
    uacpi_u64 region_end;

    node = item_array_at(&ctx->cur_op_ctx->items, 0)->node;
    obj = item_array_at(&ctx->cur_op_ctx->items, 4)->obj;
    op_region = obj->op_region;

    op_region->space = item_array_at(&ctx->cur_op_ctx->items, 1)->immediate;
    op_region->offset = item_array_at(&ctx->cur_op_ctx->items, 2)->obj->integer;
    op_region->length = item_array_at(&ctx->cur_op_ctx->items, 3)->obj->integer;
    region_end = op_region->offset + op_region->length;

    if (uacpi_unlikely(op_region->length == 0)) {
        // Don't abort here, as long as it's never accessed we don't care
        uacpi_warn("unusable/empty operation region %.4s\n", node->name.text);
    } else if (uacpi_unlikely(op_region->offset > region_end)) {
        uacpi_error(
            "invalid operation region %.4s bounds: offset=0x%"UACPI_PRIX64
            " length=0x%"UACPI_PRIX64"\n", node->name.text,
            UACPI_FMT64(op_region->offset), UACPI_FMT64(op_region->length)
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    if (op_region->space == UACPI_ADDRESS_SPACE_PCC && op_region->offset > 255) {
        uacpi_warn(
            "invalid PCC operation region %.4s subspace %"UACPI_PRIX64"\n",
            node->name.text, UACPI_FMT64(op_region->offset)
        );
    }

    node->object = uacpi_create_internal_reference(
        UACPI_REFERENCE_KIND_NAMED, obj
    );
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_initialize_opregion_node(node);
    return UACPI_STATUS_OK;
}

static uacpi_status table_id_error(
    const uacpi_char *opcode, const uacpi_char *arg,
    uacpi_buffer *str
)
{
    uacpi_error("%s: invalid %s '%s'\n", opcode, arg, str->text);
    return UACPI_STATUS_AML_BAD_ENCODING;
}

static void report_table_id_find_error(
    const uacpi_char *opcode, struct uacpi_table_identifiers *id,
    uacpi_status ret
)
{
    uacpi_error(
        "%s: unable to find table '%.4s' (OEM ID '%.6s', "
        "OEM Table ID '%.8s'): %s\n",
        opcode, id->signature.text, id->oemid, id->oem_table_id,
        uacpi_status_to_string(ret)
    );
}

static uacpi_status build_table_id(
    const uacpi_char *opcode,
    struct uacpi_table_identifiers *out_id,
    uacpi_buffer *signature, uacpi_buffer *oem_id,
    uacpi_buffer *oem_table_id
)
{
    if (uacpi_unlikely(signature->size != (sizeof(uacpi_object_name) + 1)))
        return table_id_error(opcode, "SignatureString", signature);

    uacpi_memcpy(out_id->signature.text, signature->text,
                 sizeof(uacpi_object_name));

    if (uacpi_unlikely(oem_id->size > (sizeof(out_id->oemid) + 1)))
        return table_id_error(opcode, "OemIDString", oem_id);

    uacpi_memcpy_zerout(
        out_id->oemid, oem_id->text,
        sizeof(out_id->oemid), oem_id->size ? oem_id->size  - 1 : 0
    );

    if (uacpi_unlikely(oem_table_id->size > (sizeof(out_id->oem_table_id) + 1)))
        return table_id_error(opcode, "OemTableIDString", oem_table_id);

    uacpi_memcpy_zerout(
        out_id->oem_table_id, oem_table_id->text,
        sizeof(out_id->oem_table_id),
        oem_table_id->size ? oem_table_id->size - 1 : 0
    );

    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_data_region(struct execution_context *ctx)
{
    uacpi_status ret;
    struct item_array *items = &ctx->cur_op_ctx->items;
    struct uacpi_table_identifiers table_id;
    uacpi_table table;
    uacpi_namespace_node *node;
    uacpi_object *obj;
    uacpi_operation_region *op_region;

    node = item_array_at(items, 0)->node;

    ret = build_table_id(
        "DataTableRegion", &table_id,
        item_array_at(items, 1)->obj->buffer,
        item_array_at(items, 2)->obj->buffer,
        item_array_at(items, 3)->obj->buffer
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_table_find(&table_id, &table);
    if (uacpi_unlikely_error(ret)) {
        report_table_id_find_error("DataTableRegion", &table_id, ret);
        return ret;
    }

    obj = item_array_at(items, 4)->obj;
    op_region = obj->op_region;
    op_region->space = UACPI_ADDRESS_SPACE_TABLE_DATA;
    op_region->offset = table.virt_addr;
    op_region->length = table.hdr->length;
    op_region->table_idx = table.index;

    node->object = uacpi_create_internal_reference(
        UACPI_REFERENCE_KIND_NAMED, obj
    );
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_initialize_opregion_node(node);
    return UACPI_STATUS_OK;
}

static uacpi_bool is_dynamic_table_load(enum uacpi_table_load_cause cause)
{
    return cause != UACPI_TABLE_LOAD_CAUSE_INIT;
}

static void prepare_table_load(
    void *ptr, enum uacpi_table_load_cause cause, uacpi_control_method *in_method
)
{
    struct acpi_dsdt *dsdt = ptr;
    enum uacpi_log_level log_level = UACPI_LOG_TRACE;
    const uacpi_char *log_prefix = "load of";

    if (is_dynamic_table_load(cause)) {
        log_prefix = cause == UACPI_TABLE_LOAD_CAUSE_HOST ?
               "host-invoked load of" : "dynamic load of";
        log_level = UACPI_LOG_INFO;
    }

    uacpi_log_lvl(
        log_level, "%s "UACPI_PRI_TBL_HDR"\n",
        log_prefix, UACPI_FMT_TBL_HDR(&dsdt->hdr)
    );

    in_method->code = dsdt->definition_block;
    in_method->size = dsdt->hdr.length - sizeof(dsdt->hdr);
    in_method->named_objects_persist = UACPI_TRUE;
}

static uacpi_status do_load_table(
    uacpi_namespace_node *parent, struct acpi_sdt_hdr *tbl,
    enum uacpi_table_load_cause cause
)
{
    struct uacpi_control_method method = { 0 };
    uacpi_status ret;

    prepare_table_load(tbl, cause, &method);

    ret = uacpi_execute_control_method(parent, &method, UACPI_NULL, UACPI_NULL);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (is_dynamic_table_load(cause))
        uacpi_events_match_post_dynamic_table_load();

    return ret;
}

static uacpi_status handle_load_table(struct execution_context *ctx)
{
    uacpi_status ret;
    struct item_array *items = &ctx->cur_op_ctx->items;
    struct item *root_node_item;
    struct uacpi_table_identifiers table_id;
    uacpi_table table;
    uacpi_buffer *root_path, *param_path;
    uacpi_control_method *method;
    uacpi_namespace_node *root_node, *param_node = UACPI_NULL;

    /*
     * If we already have the last true/false object loaded, this is a second
     * invocation of this handler. For the second invocation we want to detect
     * new AML GPE handlers that might've been loaded, as well as potentially
     * remove the target.
     */
    if (item_array_size(items) == 12) {
        uacpi_size idx;

        idx = item_array_at(items, 2)->immediate;
        uacpi_table_unref(&(struct uacpi_table) { .index = idx });

        /*
         * If this load failed, remove the target that was provided via
         * ParameterPathString so that it doesn't get stored to.
         */
        if (uacpi_unlikely(item_array_at(items, 11)->obj->integer == 0)) {
            uacpi_object *target;

            target = item_array_at(items, 3)->obj;
            if (target != UACPI_NULL) {
                uacpi_object_unref(target);
                item_array_at(items, 3)->obj = UACPI_NULL;
            }

            return UACPI_STATUS_OK;
        }

        uacpi_events_match_post_dynamic_table_load();
        return UACPI_STATUS_OK;
    }

    ret = build_table_id(
        "LoadTable", &table_id,
        item_array_at(items, 5)->obj->buffer,
        item_array_at(items, 6)->obj->buffer,
        item_array_at(items, 7)->obj->buffer
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    root_path = item_array_at(items, 8)->obj->buffer;
    param_path = item_array_at(items, 9)->obj->buffer;
    root_node_item = item_array_at(items, 0);

    if (root_path->size > 1) {
        ret = uacpi_namespace_node_resolve(
            ctx->cur_frame->cur_scope, root_path->text, UACPI_SHOULD_LOCK_NO,
            UACPI_MAY_SEARCH_ABOVE_PARENT_YES, UACPI_PERMANENT_ONLY_NO,
            &root_node
        );
        if (uacpi_unlikely_error(ret)) {
            table_id_error("LoadTable", "RootPathString", root_path);
            if (ret == UACPI_STATUS_NOT_FOUND)
                ret = UACPI_STATUS_AML_UNDEFINED_REFERENCE;
            return ret;
        }
    } else {
        root_node = uacpi_namespace_root();
    }

    root_node_item->node = root_node;
    root_node_item->type = ITEM_NAMESPACE_NODE;
    uacpi_shareable_ref(root_node);

    if (param_path->size > 1) {
        struct item *param_item;

        ret = uacpi_namespace_node_resolve(
            root_node, param_path->text, UACPI_SHOULD_LOCK_NO,
            UACPI_MAY_SEARCH_ABOVE_PARENT_YES, UACPI_PERMANENT_ONLY_NO,
            &param_node
        );
        if (uacpi_unlikely_error(ret)) {
            table_id_error("LoadTable", "ParameterPathString", root_path);
            if (ret == UACPI_STATUS_NOT_FOUND)
                ret = UACPI_STATUS_AML_UNDEFINED_REFERENCE;
            return ret;
        }

        param_item = item_array_at(items, 3);
        param_item->obj = param_node->object;
        uacpi_object_ref(param_item->obj);
        param_item->type = ITEM_OBJECT;
    }

    ret = uacpi_table_find(&table_id, &table);
    if (uacpi_unlikely_error(ret)) {
        report_table_id_find_error("LoadTable", &table_id, ret);
        return ret;
    }
    uacpi_table_mark_as_loaded(table.index);

    item_array_at(items, 2)->immediate = table.index;
    method = item_array_at(items, 1)->obj->method;
    prepare_table_load(table.hdr, UACPI_TABLE_LOAD_CAUSE_LOAD_TABLE_OP, method);

    return UACPI_STATUS_OK;
}

static uacpi_status handle_load(struct execution_context *ctx)
{
    uacpi_status ret;
    struct item_array *items = &ctx->cur_op_ctx->items;
    uacpi_table table;
    uacpi_control_method *method;
    uacpi_object *src;
    struct acpi_sdt_hdr *src_table = UACPI_NULL;
    void *table_buffer;
    uacpi_size declared_size;
    uacpi_bool unmap_src = UACPI_FALSE;

    /*
     * If we already have the last true/false object loaded, this is a second
     * invocation of this handler. For the second invocation we simply want to
     * detect new AML GPE handlers that might've been loaded.
     * We do this only if table load was successful though.
     */
    if (item_array_size(items) == 5) {
        if (item_array_at(items, 4)->obj->integer != 0)
            uacpi_events_match_post_dynamic_table_load();
        return UACPI_STATUS_OK;
    }

    src = item_array_at(items, 2)->obj;

    switch (src->type) {
    case UACPI_OBJECT_OPERATION_REGION: {
        uacpi_operation_region *op_region;

        op_region = src->op_region;
        if (uacpi_unlikely(
            op_region->space != UACPI_ADDRESS_SPACE_SYSTEM_MEMORY
        )) {
            uacpi_error("Load: operation region is not SystemMemory\n");
            goto error_out;
        }

        if (uacpi_unlikely(op_region->length < sizeof(struct acpi_sdt_hdr))) {
            uacpi_error(
                "Load: operation region is too small: %"UACPI_PRIu64"\n",
                UACPI_FMT64(op_region->length)
            );
            goto error_out;
        }

        src_table = uacpi_kernel_map(op_region->offset, op_region->length);
        if (uacpi_unlikely(src_table == UACPI_NULL)) {
            uacpi_error(
                "Load: failed to map operation region "
                "0x%016"UACPI_PRIX64" -> 0x%016"UACPI_PRIX64"\n",
                UACPI_FMT64(op_region->offset),
                UACPI_FMT64(op_region->offset + op_region->length)
            );
            goto error_out;
        }

        unmap_src = UACPI_TRUE;
        declared_size = op_region->length;
        break;
    }

    case UACPI_OBJECT_BUFFER: {
        uacpi_buffer *buffer;

        buffer = src->buffer;
        if (buffer->size < sizeof(struct acpi_sdt_hdr)) {
            uacpi_error(
                "Load: buffer is too small: %zu\n",
                buffer->size
            );
            goto error_out;
        }

        src_table = buffer->data;
        declared_size = buffer->size;
        break;
    }

    default:
        uacpi_error(
            "Load: invalid argument '%s', expected "
            "Buffer/Field/OperationRegion\n",
            uacpi_object_type_to_string(src->type)
        );
        goto error_out;
    }

    if (uacpi_unlikely(src_table->length > declared_size)) {
        uacpi_error(
            "Load: table size %u is larger than the declared size %zu\n",
            src_table->length, declared_size
        );
        goto error_out;
    }

    if (uacpi_unlikely(src_table->length < sizeof(struct acpi_sdt_hdr))) {
        uacpi_error("Load: table size %u is too small\n", src_table->length);
        goto error_out;
    }

    table_buffer = uacpi_kernel_alloc(src_table->length);
    if (uacpi_unlikely(table_buffer == UACPI_NULL))
        goto error_out;

    uacpi_memcpy(table_buffer, src_table, src_table->length);

    if (unmap_src) {
        uacpi_kernel_unmap(src_table, declared_size);
        unmap_src = UACPI_FALSE;
    }

    ret = uacpi_table_install_with_origin(
        table_buffer, UACPI_TABLE_ORIGIN_FIRMWARE_VIRTUAL, &table
    );
    if (uacpi_unlikely_error(ret)) {
        uacpi_free(table_buffer, src_table->length);

        if (ret != UACPI_STATUS_OVERRIDDEN)
            goto error_out;
    }
    uacpi_table_mark_as_loaded(table.index);

    item_array_at(items, 0)->node = uacpi_namespace_root();

    method = item_array_at(items, 1)->obj->method;
    prepare_table_load(table.ptr, UACPI_TABLE_LOAD_CAUSE_LOAD_OP, method);

    return UACPI_STATUS_OK;

error_out:
    if (unmap_src && src_table)
        uacpi_kernel_unmap(src_table, declared_size);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_execute_table(void *tbl, enum uacpi_table_load_cause cause)
{
    uacpi_status ret;

    ret = uacpi_namespace_write_lock();
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = do_load_table(uacpi_namespace_root(), tbl, cause);

    uacpi_namespace_write_unlock();
    return ret;
}

static uacpi_u32 get_field_length(struct item *item)
{
    struct package_length *pkg = &item->pkg;
    return pkg->end - pkg->begin;
}

struct field_specific_data {
    uacpi_namespace_node *region;
    struct uacpi_field_unit *field0;
    struct uacpi_field_unit *field1;
    uacpi_u64 value;
};

static uacpi_status ensure_is_a_field_unit(uacpi_namespace_node *node,
                                           uacpi_field_unit **out_field)
{
    uacpi_object *obj;

    obj = uacpi_namespace_node_get_object(node);
    if (obj->type != UACPI_OBJECT_FIELD_UNIT) {
        uacpi_error(
            "invalid argument: '%.4s' is not a field unit (%s)\n",
            node->name.text, uacpi_object_type_to_string(obj->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    *out_field = obj->field_unit;
    return UACPI_STATUS_OK;
}

static uacpi_status ensure_is_an_op_region(uacpi_namespace_node *node,
                                           uacpi_namespace_node **out_node)
{
    uacpi_object *obj;

    obj = uacpi_namespace_node_get_object(node);
    if (obj->type != UACPI_OBJECT_OPERATION_REGION) {
        uacpi_error(
            "invalid argument: '%.4s' is not an operation region (%s)\n",
            node->name.text, uacpi_object_type_to_string(obj->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    *out_node = node;
    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_field(struct execution_context *ctx)
{
    uacpi_status ret;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_namespace_node *node;
    uacpi_object *obj, *connection_obj = UACPI_NULL;
    struct field_specific_data field_data = { 0 };
    uacpi_size i = 1, bit_offset = 0;
    uacpi_u32 length, pin_offset = 0;

    uacpi_u8 raw_value, access_type, lock_rule, update_rule;
    uacpi_u8 access_attrib = 0, access_length = 0;

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_FieldOp:
        node = item_array_at(&op_ctx->items, i++)->node;
        ret = ensure_is_an_op_region(node, &field_data.region);
        if (uacpi_unlikely_error(ret))
            return ret;
        break;

    case UACPI_AML_OP_BankFieldOp:
        node = item_array_at(&op_ctx->items, i++)->node;
        ret = ensure_is_an_op_region(node, &field_data.region);
        if (uacpi_unlikely_error(ret))
            return ret;

        node = item_array_at(&op_ctx->items, i++)->node;
        ret = ensure_is_a_field_unit(node, &field_data.field0);
        if (uacpi_unlikely_error(ret))
            return ret;

        field_data.value = item_array_at(&op_ctx->items, i++)->obj->integer;
        break;

    case UACPI_AML_OP_IndexFieldOp:
        node = item_array_at(&op_ctx->items, i++)->node;
        ret = ensure_is_a_field_unit(node, &field_data.field0);
        if (uacpi_unlikely_error(ret))
            return ret;

        node = item_array_at(&op_ctx->items, i++)->node;
        ret = ensure_is_a_field_unit(node, &field_data.field1);
        if (uacpi_unlikely_error(ret))
            return ret;
        break;

    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    /*
     * ByteData
     * bit 0-3: AccessType
     *     0 AnyAcc
     *     1 ByteAcc
     *     2 WordAcc
     *     3 DWordAcc
     *     4 QWordAcc
     *     5 BufferAcc
     *     6 Reserved
     *     7-15 Reserved
     * bit 4: LockRule
     *     0 NoLock
     *     1 Lock
     * bit 5-6: UpdateRule
     *     0 Preserve
     *     1 WriteAsOnes
     *     2 WriteAsZeros
     * bit 7: Reserved (must be 0)
     */
    raw_value = item_array_at(&op_ctx->items, i++)->immediate;
    access_type = (raw_value >> 0) & 0b1111;
    lock_rule   = (raw_value >> 4) & 0b1;
    update_rule = (raw_value >> 5) & 0b11;

    while (i < item_array_size(&op_ctx->items)) {
        struct item *item;
        item = item_array_at(&op_ctx->items, i++);

        // An actual field object
        if (item->type == ITEM_NAMESPACE_NODE) {
            uacpi_field_unit *field;

            length = get_field_length(item_array_at(&op_ctx->items, i++));
            node = item->node;

            obj = item_array_at(&op_ctx->items, i++)->obj;
            field = obj->field_unit;

            field->update_rule = update_rule;
            field->lock_rule = lock_rule;
            field->attributes = access_attrib;
            field->access_length = access_length;

            /*
             * 0 AnyAcc
             * 1 ByteAcc
             * 2 WordAcc
             * 3 DWordAcc
             * 4 QWordAcc
             * 5 BufferAcc
             * 6 Reserved
             * 7-15 Reserved
             */
            switch (access_type) {
            case 0:
                 // TODO: optimize to calculate best access strategy
                 UACPI_FALLTHROUGH;
            case 1:
            case 5:
                field->access_width_bytes = 1;
                break;
            case 2:
                field->access_width_bytes = 2;
                break;
            case 3:
                field->access_width_bytes = 4;
                break;
            case 4:
                field->access_width_bytes = 8;
                break;
            default:
                uacpi_error("invalid field '%.4s' access type %d\n",
                            node->name.text, access_type);
                return UACPI_STATUS_AML_BAD_ENCODING;
            }

            field->bit_length = length;
            field->pin_offset = pin_offset;

            // FIXME: overflow, OOB, etc checks
            field->byte_offset = UACPI_ALIGN_DOWN(
                bit_offset / 8,
                field->access_width_bytes,
                uacpi_u32
            );

            field->bit_offset_within_first_byte = bit_offset;
            field->bit_offset_within_first_byte =
                bit_offset & ((field->access_width_bytes * 8) - 1);

            switch (op_ctx->op->code) {
            case UACPI_AML_OP_FieldOp:
                field->region = field_data.region;
                uacpi_shareable_ref(field->region);

                field->kind = UACPI_FIELD_UNIT_KIND_NORMAL;
                break;

            case UACPI_AML_OP_BankFieldOp:
                field->bank_region = field_data.region;
                uacpi_shareable_ref(field->bank_region);

                field->bank_selection = field_data.field0;
                uacpi_shareable_ref(field->bank_selection);

                field->bank_value = field_data.value;
                field->kind = UACPI_FIELD_UNIT_KIND_BANK;
                break;

            case UACPI_AML_OP_IndexFieldOp:
                field->index = field_data.field0;
                uacpi_shareable_ref(field->index);

                field->data = field_data.field1;
                uacpi_shareable_ref(field->data);

                field->kind = UACPI_FIELD_UNIT_KIND_INDEX;
                break;

            default:
                return UACPI_STATUS_INVALID_ARGUMENT;
            }

            field->connection = connection_obj;
            if (field->connection)
                uacpi_object_ref(field->connection);

            node->object = uacpi_create_internal_reference(
                UACPI_REFERENCE_KIND_NAMED, obj
            );
            if (uacpi_unlikely(node->object == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;

            ret = do_install_node_item(ctx->cur_frame, item);
            if (uacpi_unlikely_error(ret))
                return ret;

            bit_offset += length;
            pin_offset += length;
            continue;
        }

        // All other stuff
        switch (item->immediate) {
        // ReservedField := 0x00 PkgLength
        case 0x00:
            length = get_field_length(item_array_at(&op_ctx->items, i++));
            bit_offset += length;
            pin_offset += length;
            break;

        // AccessField := 0x01 AccessType AccessAttrib
        // ExtendedAccessField := 0x03 AccessType ExtendedAccessAttrib AccessLength
        case 0x01:
        case 0x03:
            raw_value = item_array_at(&op_ctx->items, i++)->immediate;

            access_type = raw_value & 0b1111;
            access_attrib = (raw_value >> 6) & 0b11;

            raw_value = item_array_at(&op_ctx->items, i++)->immediate;

            /*
             * Bits 7:6
             *     0 = AccessAttrib = Normal Access Attributes
             *     1 = AccessAttrib = AttribBytes (x)
             *     2 = AccessAttrib = AttribRawBytes (x)
             *     3 = AccessAttrib = AttribRawProcessBytes (x)
             *     x is encoded as bits 0:7 of the AccessAttrib byte.
             */
            if (access_attrib) {
                switch (access_attrib) {
                case 1:
                    access_attrib = UACPI_ACCESS_ATTRIBUTE_BYTES;
                    break;
                case 2:
                    access_attrib = UACPI_ACCESS_ATTRIBUTE_RAW_BYTES;
                    break;
                case 3:
                    access_attrib = UACPI_ACCESS_ATTRIBUTE_RAW_PROCESS_BYTES;
                    break;
                }

                access_length = raw_value;
            } else { // Normal access attributes
                access_attrib = raw_value;
            }

            if (item->immediate == 3)
                access_length = item_array_at(&op_ctx->items, i++)->immediate;
            break;

        // ConnectField := <0x02 NameString> | <0x02 BufferData>
        case 0x02:
            connection_obj = item_array_at(&op_ctx->items, i++)->obj;
            pin_offset = 0;
            break;

        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }
    }

    return UACPI_STATUS_OK;
}

static void truncate_number_if_needed(uacpi_object *obj)
{
    if (!g_uacpi_rt_ctx.is_rev1)
        return;

    obj->integer &= 0xFFFFFFFF;
}

static uacpi_u64 ones(void)
{
    return g_uacpi_rt_ctx.is_rev1 ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF;
}

static uacpi_status method_get_ret_target(struct execution_context *ctx,
                                          uacpi_object **out_operand)
{
    uacpi_size depth;

    // Check if we're targeting the previous call frame
    depth = call_frame_array_size(&ctx->call_stack);
    if (depth > 1) {
        struct op_context *op_ctx;
        struct call_frame *frame;

        frame = call_frame_array_at(&ctx->call_stack, depth - 2);
        depth = op_context_array_size(&frame->pending_ops);

        // Ok, no one wants the return value at call site. Discard it.
        if (!depth) {
            *out_operand = UACPI_NULL;
            return UACPI_STATUS_OK;
        }

        op_ctx = op_context_array_at(&frame->pending_ops, depth - 1);

        /*
         * Prevent the table being dynamically loaded from attempting to return
         * a value to the caller. This is unlikely to be ever encountered in the
         * wild, but we should still guard against the possibility.
         */
        if (uacpi_unlikely(op_ctx->op->code == UACPI_AML_OP_LoadOp ||
                           op_ctx->op->code == UACPI_AML_OP_LoadTableOp)) {
            *out_operand = UACPI_NULL;
            return UACPI_STATUS_OK;
        }

        *out_operand = item_array_last(&op_ctx->items)->obj;
        return UACPI_STATUS_OK;
    }

    return UACPI_STATUS_NOT_FOUND;
}

static uacpi_status method_get_ret_object(struct execution_context *ctx,
                                          uacpi_object **out_obj)
{
    uacpi_status ret;

    ret = method_get_ret_target(ctx, out_obj);
    if (ret == UACPI_STATUS_NOT_FOUND) {
        *out_obj = ctx->ret;
        return UACPI_STATUS_OK;
    }
    if (ret != UACPI_STATUS_OK || *out_obj == UACPI_NULL)
        return ret;

    *out_obj = uacpi_unwrap_internal_reference(*out_obj);
    return UACPI_STATUS_OK;
}

static struct code_block *find_last_block(struct code_block_array *blocks,
                                          enum code_block_type type)
{
    uacpi_size i;

    i = code_block_array_size(blocks);
    while (i-- > 0) {
        struct code_block *block;

        block = code_block_array_at(blocks, i);
        if (block->type == type)
            return block;
    }

    return UACPI_NULL;
}

static void update_scope(struct call_frame *frame)
{
    struct code_block *block;

    block = find_last_block(&frame->code_blocks, CODE_BLOCK_SCOPE);
    if (block == UACPI_NULL) {
        frame->cur_scope = uacpi_namespace_root();
        return;
    }

    frame->cur_scope = block->node;
}

static uacpi_status begin_block_execution(struct execution_context *ctx)
{
    struct call_frame *cur_frame = ctx->cur_frame;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct package_length *pkg;
    struct code_block *block;

    block = code_block_array_alloc(&cur_frame->code_blocks);
    if (uacpi_unlikely(block == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    pkg = &item_array_at(&op_ctx->items, 0)->pkg;

    // Disarm the tracked package so that we don't skip the Scope
    op_ctx->tracked_pkg_idx = 0;

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_IfOp:
        block->type = CODE_BLOCK_IF;
        break;
    case UACPI_AML_OP_ElseOp:
        block->type = CODE_BLOCK_ELSE;
        break;
    case UACPI_AML_OP_WhileOp:
        block->type = CODE_BLOCK_WHILE;

        if (pkg->begin == cur_frame->prev_while_code_offset) {
            uacpi_u64 cur_ticks;

            cur_ticks = uacpi_kernel_get_nanoseconds_since_boot();

            if (uacpi_unlikely(cur_ticks > block->expiration_point)) {
                uacpi_error("loop time out after running for %u seconds\n",
                            g_uacpi_rt_ctx.loop_timeout_seconds);
                code_block_array_pop(&cur_frame->code_blocks);
                return UACPI_STATUS_AML_LOOP_TIMEOUT;
            }

            block->expiration_point = cur_frame->prev_while_expiration;
        } else {
            /*
             * Calculate the expiration point for this loop.
             * If a loop is executed past this point, it will get aborted.
             */
            block->expiration_point = uacpi_kernel_get_nanoseconds_since_boot();
            block->expiration_point +=
                g_uacpi_rt_ctx.loop_timeout_seconds * UACPI_NANOSECONDS_PER_SEC;
        }
        break;
    case UACPI_AML_OP_ScopeOp:
    case UACPI_AML_OP_DeviceOp:
    case UACPI_AML_OP_ProcessorOp:
    case UACPI_AML_OP_PowerResOp:
    case UACPI_AML_OP_ThermalZoneOp:
        block->type = CODE_BLOCK_SCOPE;
        block->node = item_array_at(&op_ctx->items, 1)->node;
        break;
    default:
        code_block_array_pop(&cur_frame->code_blocks);
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    // -1 because we want to re-evaluate at the start of the op next time
    block->begin = pkg->begin - 1;
    block->end = pkg->end;
    ctx->cur_block = block;

    cur_frame->last_while = find_last_block(&cur_frame->code_blocks,
                                            CODE_BLOCK_WHILE);
    update_scope(cur_frame);
    return UACPI_STATUS_OK;
}

static void frame_reset_post_end_block(struct execution_context *ctx,
                                       enum code_block_type type)
{
    struct call_frame *frame = ctx->cur_frame;

    if (type == CODE_BLOCK_WHILE) {
        struct code_block *block = ctx->cur_block;

        // + 1 here to skip the WhileOp and get to the PkgLength
        frame->prev_while_code_offset = block->begin + 1;
        frame->prev_while_expiration = block->expiration_point;
    }

    code_block_array_pop(&frame->code_blocks);
    ctx->cur_block = code_block_array_last(&frame->code_blocks);

    if (type == CODE_BLOCK_WHILE) {
        frame->last_while = find_last_block(&frame->code_blocks, type);
    } else if (type == CODE_BLOCK_SCOPE) {
        update_scope(frame);
    }
}

static void debug_store_no_recurse(const uacpi_char *prefix, uacpi_object *src)
{
    switch (src->type) {
    case UACPI_OBJECT_UNINITIALIZED:
        uacpi_trace("%s Uninitialized\n", prefix);
        break;
    case UACPI_OBJECT_STRING:
        uacpi_trace("%s String => \"%s\"\n", prefix, src->buffer->text);
        break;
    case UACPI_OBJECT_INTEGER:
        if (g_uacpi_rt_ctx.is_rev1) {
            uacpi_trace(
                "%s Integer => 0x%08X\n", prefix, (uacpi_u32)src->integer
            );
        } else {
            uacpi_trace(
                "%s Integer => 0x%016"UACPI_PRIX64"\n", prefix,
                UACPI_FMT64(src->integer)
            );
        }
        break;
    case UACPI_OBJECT_REFERENCE:
        uacpi_trace("%s Reference @%p => %p\n", prefix, src, src->inner_object);
        break;
    case UACPI_OBJECT_PACKAGE:
        uacpi_trace(
            "%s Package @%p (%p) (%zu elements)\n",
            prefix, src, src->package, src->package->count
        );
        break;
    case UACPI_OBJECT_BUFFER:
        uacpi_trace(
            "%s Buffer @%p (%p) (%zu bytes)\n",
            prefix, src, src->buffer, src->buffer->size
        );
        break;
    case UACPI_OBJECT_OPERATION_REGION:
        uacpi_trace(
            "%s OperationRegion (ASID %d) 0x%016"UACPI_PRIX64
            " -> 0x%016"UACPI_PRIX64"\n", prefix,
            src->op_region->space, UACPI_FMT64(src->op_region->offset),
            UACPI_FMT64(src->op_region->offset + src->op_region->length)
        );
        break;
    case UACPI_OBJECT_POWER_RESOURCE:
        uacpi_trace(
            "%s Power Resource %d %d\n",
            prefix, src->power_resource.system_level,
            src->power_resource.resource_order
        );
        break;
    case UACPI_OBJECT_PROCESSOR:
        uacpi_trace(
            "%s Processor[%d] 0x%08X (%d)\n",
            prefix, src->processor->id, src->processor->block_address,
            src->processor->block_length
        );
        break;
    case UACPI_OBJECT_BUFFER_INDEX:
        uacpi_trace(
            "%s Buffer Index %p[%zu] => 0x%02X\n",
            prefix, src->buffer_index.buffer->data, src->buffer_index.idx,
            *buffer_index_cursor(&src->buffer_index)
        );
        break;
    case UACPI_OBJECT_MUTEX:
        uacpi_trace(
            "%s Mutex @%p (%p => %p) sync level %d\n",
            prefix, src, src->mutex, src->mutex->handle,
            src->mutex->sync_level
        );
        break;
    case UACPI_OBJECT_METHOD:
        uacpi_trace("%s Method @%p (%p)\n", prefix, src, src->method);
        break;
    default:
        uacpi_trace(
            "%s %s @%p\n",
            prefix, uacpi_object_type_to_string(src->type), src
        );
    }
}

static uacpi_status debug_store(uacpi_object *src)
{
    /*
     * Don't bother running the body if current log level is not set to trace.
     * All DebugOp logging is done as TRACE exclusively.
     */
    if (!uacpi_should_log(UACPI_LOG_TRACE))
        return UACPI_STATUS_OK;

    src = uacpi_unwrap_internal_reference(src);

    debug_store_no_recurse("[AML DEBUG]", src);

    if (src->type == UACPI_OBJECT_PACKAGE) {
        uacpi_package *pkg = src->package;
        uacpi_size i;

        for (i = 0; i < pkg->count; ++i) {
            uacpi_object *obj = pkg->objects[i];
            if (obj->type == UACPI_OBJECT_REFERENCE &&
                obj->flags == UACPI_REFERENCE_KIND_PKG_INDEX)
                obj = obj->inner_object;

            debug_store_no_recurse("Element:", obj);
        }
    }

    return UACPI_STATUS_OK;
}

/*
 * NOTE: this function returns the parent object
 */
static uacpi_object *reference_unwind(uacpi_object *obj)
{
    uacpi_object *parent = obj;

    while (obj) {
        if (obj->type != UACPI_OBJECT_REFERENCE)
            return parent;

        parent = obj;
        obj = parent->inner_object;
    }

    // This should be unreachable
    return UACPI_NULL;
}

static uacpi_iteration_decision opregion_try_detach_from_parent(
    void *user, uacpi_namespace_node *node, uacpi_u32 node_depth
)
{
    uacpi_object *target_object = user;
    UACPI_UNUSED(node_depth);

    if (node->object == target_object) {
        uacpi_opregion_uninstall_handler(node);
        return UACPI_ITERATION_DECISION_BREAK;
    }

    return UACPI_ITERATION_DECISION_CONTINUE;
}

static void object_replace_child(uacpi_object *parent, uacpi_object *new_child)
{
    if (parent->flags == UACPI_REFERENCE_KIND_NAMED &&
        uacpi_object_is(parent->inner_object, UACPI_OBJECT_OPERATION_REGION)) {

        /*
         * We're doing a CopyObject or similar to a namespace node that is an
         * operation region. Try to find the parent node and manually detach
         * the handler.
         */
        opregion_try_detach_from_parent(parent, uacpi_namespace_root(), 0);
        uacpi_namespace_do_for_each_child(
            uacpi_namespace_root(), opregion_try_detach_from_parent, UACPI_NULL,
            UACPI_OBJECT_OPERATION_REGION_BIT, UACPI_MAX_DEPTH_ANY,
            UACPI_SHOULD_LOCK_NO, UACPI_PERMANENT_ONLY_NO, parent
        );
    }

    uacpi_object_detach_child(parent);
    uacpi_object_attach_child(parent, new_child);
}

/*
 * Breakdown of what happens here:
 *
 * CopyObject(..., Obj) where Obj is:
 * 1. LocalX -> Overwrite LocalX.
 * 2. NAME -> Overwrite NAME.
 * 3. ArgX -> Overwrite ArgX unless ArgX is a reference, in that case
 *            overwrite the referenced object.
 * 4. RefOf -> Not allowed here.
 * 5. Index -> Overwrite Object stored at the index.
 */
 static uacpi_status copy_object_to_reference(uacpi_object *dst,
                                              uacpi_object *src)
{
    uacpi_status ret;
    uacpi_object *src_obj, *new_obj;

    switch (dst->flags) {
    case UACPI_REFERENCE_KIND_ARG: {
        uacpi_object *referenced_obj;

        referenced_obj = uacpi_unwrap_internal_reference(dst);
        if (referenced_obj->type == UACPI_OBJECT_REFERENCE) {
            dst = reference_unwind(referenced_obj);
            break;
        }

        UACPI_FALLTHROUGH;
    }
    case UACPI_REFERENCE_KIND_LOCAL:
    case UACPI_REFERENCE_KIND_PKG_INDEX:
    case UACPI_REFERENCE_KIND_NAMED:
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    src_obj = uacpi_unwrap_internal_reference(src);

    new_obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
    if (uacpi_unlikely(new_obj == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ret = uacpi_object_assign(new_obj, src_obj,
                              UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
    if (uacpi_unlikely_error(ret))
        return ret;

    object_replace_child(dst, new_obj);
    uacpi_object_unref(new_obj);

    return UACPI_STATUS_OK;
}

/*
 * if Store(..., Obj) where Obj is:
 * 1. LocalX/Index -> OVERWRITE unless the object is a reference, in that
 *                    case store to the referenced object _with_ implicit
 *                    cast.
 * 2. ArgX -> OVERWRITE unless the object is a reference, in that
 *            case OVERWRITE the referenced object.
 * 3. NAME -> Store with implicit cast.
 * 4. RefOf -> Not allowed here.
 */
static uacpi_status store_to_reference(
    uacpi_object *dst, uacpi_object *src, uacpi_data_view *wtr_response
)
{
    uacpi_object *src_obj;
    uacpi_bool overwrite = UACPI_FALSE;

    switch (dst->flags) {
    case UACPI_REFERENCE_KIND_LOCAL:
    case UACPI_REFERENCE_KIND_ARG:
    case UACPI_REFERENCE_KIND_PKG_INDEX: {
        uacpi_object *referenced_obj;

        if (dst->flags == UACPI_REFERENCE_KIND_PKG_INDEX)
            referenced_obj = dst->inner_object;
        else
            referenced_obj = uacpi_unwrap_internal_reference(dst);

        if (referenced_obj->type == UACPI_OBJECT_REFERENCE) {
            overwrite = dst->flags == UACPI_REFERENCE_KIND_ARG;
            dst = reference_unwind(referenced_obj);
            break;
        }

        overwrite = UACPI_TRUE;
        break;
    }
    case UACPI_REFERENCE_KIND_NAMED:
        dst = reference_unwind(dst);
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    src_obj = uacpi_unwrap_internal_reference(src);
    overwrite |= dst->inner_object->type == UACPI_OBJECT_UNINITIALIZED;

    if (overwrite) {
        uacpi_status ret;
        uacpi_object *new_obj;

        new_obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
        if (uacpi_unlikely(new_obj == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        ret = uacpi_object_assign(new_obj, src_obj,
                                  UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
        if (uacpi_unlikely_error(ret)) {
            uacpi_object_unref(new_obj);
            return ret;
        }

        object_replace_child(dst, new_obj);
        uacpi_object_unref(new_obj);
        return UACPI_STATUS_OK;
    }

    return object_assign_with_implicit_cast(
        dst->inner_object, src_obj, wtr_response
    );
}

static uacpi_status handle_ref_or_deref_of(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *dst, *src;

    src = item_array_at(&op_ctx->items, 0)->obj;

    if (op_ctx->op->code == UACPI_AML_OP_CondRefOfOp)
        dst = item_array_at(&op_ctx->items, 2)->obj;
    else
        dst = item_array_at(&op_ctx->items, 1)->obj;

    if (op_ctx->op->code == UACPI_AML_OP_DerefOfOp) {
        uacpi_bool was_a_reference = UACPI_FALSE;

        if (src->type == UACPI_OBJECT_REFERENCE) {
            was_a_reference = UACPI_TRUE;

            /*
             * Explicit dereferencing [DerefOf] behavior:
             * Simply grabs the bottom-most object that is not a reference.
             * This mimics the behavior of NT Acpi.sys: any DerfOf fetches
             * the bottom-most reference. Note that this is different from
             * ACPICA where DerefOf dereferences one level.
             */
            src = reference_unwind(src)->inner_object;
        }

        if (src->type == UACPI_OBJECT_BUFFER_INDEX) {
            uacpi_buffer_index *buf_idx = &src->buffer_index;

            dst->type = UACPI_OBJECT_INTEGER;
            uacpi_memcpy_zerout(
                &dst->integer, buffer_index_cursor(buf_idx),
                sizeof(dst->integer), 1
            );
            return UACPI_STATUS_OK;
        }

        if (!was_a_reference) {
            uacpi_error(
                "invalid DerefOf argument: %s, expected a reference\n",
                uacpi_object_type_to_string(src->type)
            );
            return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
        }

        return uacpi_object_assign(dst, src,
                                   UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY);
    }

    dst->type = UACPI_OBJECT_REFERENCE;
    dst->inner_object = src;
    uacpi_object_ref(src);
    return UACPI_STATUS_OK;
}

static uacpi_status do_binary_math(
    uacpi_object *arg0, uacpi_object *arg1,
    uacpi_object *tgt0, uacpi_object *tgt1,
    uacpi_aml_op op
)
{
    uacpi_u64 lhs, rhs, res;
    uacpi_bool should_negate = UACPI_FALSE;

    lhs = arg0->integer;
    rhs = arg1->integer;

    switch (op)
    {
    case UACPI_AML_OP_AddOp:
        res = lhs + rhs;
        break;
    case UACPI_AML_OP_SubtractOp:
        res = lhs - rhs;
        break;
    case UACPI_AML_OP_MultiplyOp:
        res = lhs * rhs;
        break;
    case UACPI_AML_OP_ShiftLeftOp:
    case UACPI_AML_OP_ShiftRightOp:
        if (rhs <= (g_uacpi_rt_ctx.is_rev1 ? 31 : 63)) {
            if (op == UACPI_AML_OP_ShiftLeftOp)
                res = lhs << rhs;
            else
                res = lhs >> rhs;
        } else {
            res = 0;
        }
        break;
    case UACPI_AML_OP_NandOp:
        should_negate = UACPI_TRUE;
        UACPI_FALLTHROUGH;
    case UACPI_AML_OP_AndOp:
        res = rhs & lhs;
        break;
    case UACPI_AML_OP_NorOp:
        should_negate = UACPI_TRUE;
        UACPI_FALLTHROUGH;
    case UACPI_AML_OP_OrOp:
        res = rhs | lhs;
        break;
    case UACPI_AML_OP_XorOp:
        res = rhs ^ lhs;
        break;
    case UACPI_AML_OP_DivideOp:
        if (uacpi_unlikely(rhs == 0)) {
            uacpi_error("attempted to divide by zero\n");
            return UACPI_STATUS_AML_BAD_ENCODING;
        }
        tgt1->integer = lhs / rhs;
        res = lhs % rhs;
        break;
    case UACPI_AML_OP_ModOp:
        if (uacpi_unlikely(rhs == 0)) {
            uacpi_error("attempted to calculate modulo of zero\n");
            return UACPI_STATUS_AML_BAD_ENCODING;
        }
        res = lhs % rhs;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (should_negate)
        res = ~res;

    tgt0->integer = res;
    return UACPI_STATUS_OK;
}

static uacpi_status handle_binary_math(struct execution_context *ctx)
{
    uacpi_object *arg0, *arg1, *tgt0, *tgt1;
    struct item_array *items = &ctx->cur_op_ctx->items;
    uacpi_aml_op op = ctx->cur_op_ctx->op->code;

    arg0 = item_array_at(items, 0)->obj;
    arg1 = item_array_at(items, 1)->obj;

    if (op == UACPI_AML_OP_DivideOp) {
        tgt0 = item_array_at(items, 4)->obj;
        tgt1 = item_array_at(items, 5)->obj;
    } else {
        tgt0 = item_array_at(items, 3)->obj;
        tgt1 = UACPI_NULL;
    }

    return do_binary_math(arg0, arg1, tgt0, tgt1, op);
}

static uacpi_status handle_unary_math(struct execution_context *ctx)
{
    uacpi_object *arg, *tgt;
    struct item_array *items = &ctx->cur_op_ctx->items;
    uacpi_aml_op op = ctx->cur_op_ctx->op->code;

    arg = item_array_at(items, 0)->obj;
    tgt = item_array_at(items, 2)->obj;

    switch (op) {
    case UACPI_AML_OP_NotOp:
        tgt->integer = ~arg->integer;
        truncate_number_if_needed(tgt);
        break;
    case UACPI_AML_OP_FindSetRightBitOp:
        tgt->integer = uacpi_bit_scan_forward(arg->integer);
        break;
    case UACPI_AML_OP_FindSetLeftBitOp:
        tgt->integer = uacpi_bit_scan_backward(arg->integer);
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status ensure_valid_idx(uacpi_object *obj, uacpi_size idx,
                                     uacpi_size src_size)
{
    if (uacpi_likely(idx < src_size))
        return UACPI_STATUS_OK;

    uacpi_error(
        "invalid index %zu, %s@%p has %zu elements\n",
        idx, uacpi_object_type_to_string(obj->type), obj, src_size
    );
    return UACPI_STATUS_AML_OUT_OF_BOUNDS_INDEX;
}

static uacpi_status handle_index(struct execution_context *ctx)
{
    uacpi_status ret;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src;
    struct item *dst;
    uacpi_size idx;

    src = item_array_at(&op_ctx->items, 0)->obj;
    idx = item_array_at(&op_ctx->items, 1)->obj->integer;
    dst = item_array_at(&op_ctx->items, 3);

    switch (src->type) {
    case UACPI_OBJECT_BUFFER:
    case UACPI_OBJECT_STRING: {
        uacpi_buffer_index *buf_idx;
        uacpi_data_view buf;
        get_object_storage(src, &buf, UACPI_FALSE);

        ret = ensure_valid_idx(src, idx, buf.length);
        if (uacpi_unlikely_error(ret))
            return ret;

        dst->type = ITEM_OBJECT;
        dst->obj = uacpi_create_object(UACPI_OBJECT_BUFFER_INDEX);
        if (uacpi_unlikely(dst->obj == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        buf_idx = &dst->obj->buffer_index;
        buf_idx->idx = idx;
        buf_idx->buffer = src->buffer;
        uacpi_shareable_ref(buf_idx->buffer);

        break;
    }
    case UACPI_OBJECT_PACKAGE: {
        uacpi_package *pkg = src->package;
        uacpi_object *obj;

        ret = ensure_valid_idx(src, idx, pkg->count);
        if (uacpi_unlikely_error(ret))
            return ret;

        /*
         * Lazily transform the package element into an internal reference
         * to itself of type PKG_INDEX. This is needed to support stuff like
         * CopyObject(..., Index(pkg, X)) where the new object must be
         * propagated to anyone else with a currently alive index object.
         *
         * Sidenote: Yes, IndexOp is not a SimpleName, so technically it is
         *           illegal to CopyObject to it. However, yet again we fall
         *           victim to the NT ACPI driver implementation, which allows
         *           it just fine.
         */
        obj = pkg->objects[idx];
        if (obj->type != UACPI_OBJECT_REFERENCE ||
            obj->flags != UACPI_REFERENCE_KIND_PKG_INDEX) {

            obj = uacpi_create_internal_reference(
                UACPI_REFERENCE_KIND_PKG_INDEX, obj
            );
            if (uacpi_unlikely(obj == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;

            pkg->objects[idx] = obj;
            uacpi_object_unref(obj->inner_object);
        }

        dst->obj = obj;
        dst->type = ITEM_OBJECT;
        uacpi_object_ref(dst->obj);
        break;
    }
    default:
        uacpi_error(
            "invalid argument for Index: %s, "
            "expected String/Buffer/Package\n",
            uacpi_object_type_to_string(src->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return UACPI_STATUS_OK;
}

static uacpi_u64 object_to_integer(const uacpi_object *obj,
                                   uacpi_size max_buffer_bytes)
{
    uacpi_u64 dst;

    switch (obj->type) {
    case UACPI_OBJECT_INTEGER:
        dst = obj->integer;
        break;
    case UACPI_OBJECT_BUFFER: {
        uacpi_size bytes;
        bytes = UACPI_MIN(max_buffer_bytes, obj->buffer->size);
        uacpi_memcpy_zerout(&dst, obj->buffer->data, sizeof(dst), bytes);
        break;
    }
    case UACPI_OBJECT_STRING:
        uacpi_string_to_integer(
            obj->buffer->text, obj->buffer->size, UACPI_BASE_AUTO, &dst
        );
        break;
    default:
        dst = 0;
        break;
    }

    return dst;
}

static uacpi_status integer_to_string(
    uacpi_u64 integer, uacpi_buffer *str, uacpi_bool is_hex
)
{
    int repr_len;
    uacpi_char int_buf[21];
    uacpi_size final_size;

    repr_len = uacpi_snprintf(
        int_buf, sizeof(int_buf),
        is_hex ? "%"UACPI_PRIX64 : "%"UACPI_PRIu64,
        UACPI_FMT64(integer)
    );
    if (uacpi_unlikely(repr_len < 0))
        return UACPI_STATUS_INVALID_ARGUMENT;

    // 0x prefix + repr + \0
    final_size = (is_hex ? 2 : 0) + repr_len + 1;

    str->data = uacpi_kernel_alloc(final_size);
    if (uacpi_unlikely(str->data == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    if (is_hex) {
        str->text[0] = '0';
        str->text[1] = 'x';
    }
    uacpi_memcpy(str->text + (is_hex ? 2 : 0), int_buf, repr_len + 1);
    str->size = final_size;

    return UACPI_STATUS_OK;
}

static uacpi_status buffer_to_string(
    uacpi_buffer *buf, uacpi_buffer *str, uacpi_bool is_hex
)
{
    int repr_len;
    uacpi_char int_buf[5];
    uacpi_size i, final_size;
    uacpi_char *cursor;

    if (is_hex) {
        final_size = 4 * buf->size;
    } else {
        final_size = 0;

        for (i = 0; i < buf->size; ++i) {
            uacpi_u8 value = ((uacpi_u8*)buf->data)[i];

            if (value < 10)
                final_size += 1;
            else if (value < 100)
                final_size += 2;
            else
                final_size += 3;
        }
    }

    // Comma for every value but one
    final_size += buf->size - 1;

    // Null terminator
    final_size += 1;

    str->data = uacpi_kernel_alloc(final_size);
    if (uacpi_unlikely(str->data == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    cursor = str->data;

    for (i = 0; i < buf->size; ++i) {
        repr_len = uacpi_snprintf(
            int_buf, sizeof(int_buf),
            is_hex ? "0x%02X" : "%d",
            ((uacpi_u8*)buf->data)[i]
        );
        if (uacpi_unlikely(repr_len < 0)) {
            uacpi_free(str->data, final_size);
            str->data = UACPI_NULL;
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        uacpi_memcpy(cursor, int_buf, repr_len + 1);
        cursor += repr_len;

        if (i != buf->size - 1)
            *cursor++ = ',';
    }

    str->size = final_size;
    return UACPI_STATUS_OK;
}

static uacpi_status do_make_empty_object(uacpi_buffer *buf,
                                         uacpi_bool is_string)
{
    buf->text = uacpi_kernel_alloc_zeroed(sizeof(uacpi_char));
    if (uacpi_unlikely(buf->text == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    if (is_string)
        buf->size = sizeof(uacpi_char);

    return UACPI_STATUS_OK;
}

static uacpi_status make_null_string(uacpi_buffer *buf)
{
    return do_make_empty_object(buf, UACPI_TRUE);
}

static uacpi_status make_null_buffer(uacpi_buffer *buf)
{
    /*
     * Allocate at least 1 byte just to be safe,
     * even for empty buffers. We still set the
     * size to 0 though.
     */
    return do_make_empty_object(buf, UACPI_FALSE);
}

static uacpi_status handle_to(struct execution_context *ctx)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src, *dst;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 2)->obj;

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_ToIntegerOp:
        // NT always takes the first 8 bytes, even for revision 1
        dst->integer = object_to_integer(src, 8);
        break;

    case UACPI_AML_OP_ToHexStringOp:
    case UACPI_AML_OP_ToDecimalStringOp: {
        uacpi_bool is_hex = op_ctx->op->code == UACPI_AML_OP_ToHexStringOp;

        if (src->type == UACPI_OBJECT_INTEGER) {
            ret = integer_to_string(src->integer, dst->buffer, is_hex);
            break;
        } else if (src->type == UACPI_OBJECT_BUFFER) {
            if (uacpi_unlikely(src->buffer->size == 0))
                return make_null_string(dst->buffer);

            ret = buffer_to_string(src->buffer, dst->buffer, is_hex);
            break;
        }
        UACPI_FALLTHROUGH;
    }
    case UACPI_AML_OP_ToBufferOp: {
        uacpi_data_view buf;
        uacpi_u8 *dst_buf;

        ret = get_object_storage(src, &buf, UACPI_TRUE);
        if (uacpi_unlikely_error(ret))
            return ret;

        if (uacpi_unlikely(buf.length == 0))
            return make_null_buffer(dst->buffer);

        dst_buf = uacpi_kernel_alloc(buf.length);
        if (uacpi_unlikely(dst_buf == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        uacpi_memcpy(dst_buf, buf.bytes, buf.length);
        dst->buffer->data = dst_buf;
        dst->buffer->size = buf.length;
        break;
    }

    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return ret;
}

static uacpi_status handle_to_string(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_buffer *src_buf, *dst_buf;
    uacpi_size req_len, len;

    src_buf = item_array_at(&op_ctx->items, 0)->obj->buffer;
    req_len = item_array_at(&op_ctx->items, 1)->obj->integer;
    dst_buf = item_array_at(&op_ctx->items, 3)->obj->buffer;

    len = UACPI_MIN(req_len, src_buf->size);
    if (uacpi_unlikely(len == 0))
        return make_null_string(dst_buf);

    len = uacpi_strnlen(src_buf->text, len);

    dst_buf->text = uacpi_kernel_alloc(len + 1);
    if (uacpi_unlikely(dst_buf->text == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_memcpy(dst_buf->text, src_buf->data, len);
    dst_buf->text[len] = '\0';
    dst_buf->size = len + 1;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_mid(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src, *dst;
    uacpi_data_view src_buf;
    uacpi_buffer *dst_buf;
    uacpi_size idx, len;
    uacpi_bool is_string;

    src = item_array_at(&op_ctx->items, 0)->obj;
    if (uacpi_unlikely(src->type != UACPI_OBJECT_STRING &&
                       src->type != UACPI_OBJECT_BUFFER)) {
        uacpi_error(
            "invalid argument for Mid: %s, expected String/Buffer\n",
            uacpi_object_type_to_string(src->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    idx = item_array_at(&op_ctx->items, 1)->obj->integer;
    len = item_array_at(&op_ctx->items, 2)->obj->integer;
    dst = item_array_at(&op_ctx->items, 4)->obj;
    dst_buf = dst->buffer;

    is_string = src->type == UACPI_OBJECT_STRING;
    get_object_storage(src, &src_buf, UACPI_FALSE);

    if (uacpi_unlikely(src_buf.length == 0 || idx >= src_buf.length ||
                       len == 0)) {
        if (src->type == UACPI_OBJECT_STRING) {
            dst->type = UACPI_OBJECT_STRING;
            return make_null_string(dst_buf);
        }

        return make_null_buffer(dst_buf);
    }

    // Guaranteed to be at least 1 here
    len = UACPI_MIN(len, src_buf.length - idx);

    dst_buf->data = uacpi_kernel_alloc(len + is_string);
    if (uacpi_unlikely(dst_buf->data == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_memcpy(dst_buf->data, (uacpi_u8*)src_buf.bytes + idx, len);
    dst_buf->size = len;

    if (is_string) {
        dst_buf->text[dst_buf->size++] = '\0';
        dst->type = UACPI_OBJECT_STRING;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_concatenate(struct execution_context *ctx)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *arg0, *arg1, *dst;
    uacpi_u8 *dst_buf;
    uacpi_size buf_size = 0;

    arg0 = item_array_at(&op_ctx->items, 0)->obj;
    arg1 = item_array_at(&op_ctx->items, 1)->obj;
    dst = item_array_at(&op_ctx->items, 3)->obj;

    switch (arg0->type) {
    case UACPI_OBJECT_INTEGER: {
        uacpi_u64 arg1_as_int;
        uacpi_size int_size;

        int_size = sizeof_int();
        buf_size = int_size * 2;

        dst_buf = uacpi_kernel_alloc(buf_size);
        if (uacpi_unlikely(dst_buf == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        arg1_as_int = object_to_integer(arg1, 8);

        uacpi_memcpy(dst_buf, &arg0->integer, int_size);
        uacpi_memcpy(dst_buf+ int_size, &arg1_as_int, int_size);
        break;
    }
    case UACPI_OBJECT_BUFFER: {
        uacpi_buffer *arg0_buf = arg0->buffer;
        uacpi_data_view arg1_buf = { 0 };

        get_object_storage(arg1, &arg1_buf, UACPI_TRUE);
        buf_size = arg0_buf->size + arg1_buf.length;

        dst_buf = uacpi_kernel_alloc(buf_size);
        if (uacpi_unlikely(dst_buf == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        uacpi_memcpy(dst_buf, arg0_buf->data, arg0_buf->size);
        uacpi_memcpy(dst_buf + arg0_buf->size, arg1_buf.bytes, arg1_buf.length);
        break;
    }
    case UACPI_OBJECT_STRING: {
        uacpi_char int_buf[17];
        void *arg1_ptr;
        uacpi_size arg0_size, arg1_size;
        uacpi_buffer *arg0_buf = arg0->buffer;

        switch (arg1->type) {
        case UACPI_OBJECT_INTEGER: {
            int size;
            size = uacpi_snprintf(int_buf, sizeof(int_buf), "%"UACPI_PRIx64,
                                  UACPI_FMT64(arg1->integer));
            if (size < 0)
                return UACPI_STATUS_INVALID_ARGUMENT;

            arg1_ptr = int_buf;
            arg1_size = size + 1;
            break;
        }
        case UACPI_OBJECT_STRING:
            arg1_ptr = arg1->buffer->data;
            arg1_size = arg1->buffer->size;
            break;
        case UACPI_OBJECT_BUFFER: {
            uacpi_buffer tmp_buf;

            ret = buffer_to_string(arg1->buffer, &tmp_buf, UACPI_TRUE);
            if (uacpi_unlikely_error(ret))
                return ret;

            arg1_ptr = tmp_buf.data;
            arg1_size = tmp_buf.size;
            break;
        }
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        arg0_size = arg0_buf->size ? arg0_buf->size - 1 : arg0_buf->size;
        buf_size = arg0_size + arg1_size;

        dst_buf = uacpi_kernel_alloc(buf_size);
        if (uacpi_unlikely(dst_buf == UACPI_NULL)) {
            ret = UACPI_STATUS_OUT_OF_MEMORY;
            goto cleanup;
        }

        uacpi_memcpy(dst_buf, arg0_buf->data, arg0_size);
        uacpi_memcpy(dst_buf + arg0_size, arg1_ptr, arg1_size);
        dst->type = UACPI_OBJECT_STRING;

    cleanup:
        if (arg1->type == UACPI_OBJECT_BUFFER)
            uacpi_free(arg1_ptr, arg1_size);
        break;
    }
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (uacpi_likely_success(ret)) {
        dst->buffer->data = dst_buf;
        dst->buffer->size = buf_size;
    }
    return ret;
}

static uacpi_status handle_concatenate_res(struct execution_context *ctx)
{
    uacpi_status ret;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_data_view buffer;
    uacpi_object *arg0, *arg1, *dst;
    uacpi_u8 *dst_buf;
    uacpi_size dst_size, arg0_size, arg1_size;

    arg0 = item_array_at(&op_ctx->items, 0)->obj;
    arg1 = item_array_at(&op_ctx->items, 1)->obj;
    dst = item_array_at(&op_ctx->items, 3)->obj;

    uacpi_buffer_to_view(arg0->buffer, &buffer);
    ret = uacpi_find_aml_resource_end_tag(buffer, &arg0_size);
    if (uacpi_unlikely_error(ret))
        return ret;

    uacpi_buffer_to_view(arg1->buffer, &buffer);
    ret = uacpi_find_aml_resource_end_tag(buffer, &arg1_size);
    if (uacpi_unlikely_error(ret))
        return ret;

    dst_size = arg0_size + arg1_size + sizeof(struct acpi_resource_end_tag);

    dst_buf = uacpi_kernel_alloc(dst_size);
    if (uacpi_unlikely(dst_buf == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    dst->buffer->data = dst_buf;
    dst->buffer->size = dst_size;

    uacpi_memcpy(dst_buf, arg0->buffer->data, arg0_size);
    uacpi_memcpy(dst_buf + arg0_size, arg1->buffer->data, arg1_size);

    /*
     * Small item (0), End Tag (0x0F), length 1
     * Leave the checksum as 0
     */
    dst_buf[dst_size - 2] =
        (ACPI_RESOURCE_END_TAG << ACPI_SMALL_ITEM_NAME_IDX) |
        (sizeof(struct acpi_resource_end_tag) - 1);
    dst_buf[dst_size - 1] = 0;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_sizeof(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src, *dst;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 1)->obj;

    if (uacpi_likely(src->type == UACPI_OBJECT_REFERENCE))
        src = reference_unwind(src)->inner_object;

    switch (src->type) {
    case UACPI_OBJECT_STRING:
    case UACPI_OBJECT_BUFFER: {
        uacpi_data_view buf;
        get_object_storage(src, &buf, UACPI_FALSE);

        dst->integer = buf.length;
        break;
    }

    case UACPI_OBJECT_PACKAGE:
        dst->integer = src->package->count;
        break;

    default:
        uacpi_error(
            "invalid argument for Sizeof: %s, "
            "expected String/Buffer/Package\n",
            uacpi_object_type_to_string(src->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_object_type(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src, *dst;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 1)->obj;

    if (uacpi_likely(src->type == UACPI_OBJECT_REFERENCE))
        src = reference_unwind(src)->inner_object;

    dst->integer = src->type;
    if (dst->integer == UACPI_OBJECT_BUFFER_INDEX)
        dst->integer = UACPI_OBJECT_BUFFER_FIELD;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_timer(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *dst;

    dst = item_array_at(&op_ctx->items, 0)->obj;
    dst->integer = uacpi_kernel_get_nanoseconds_since_boot() / 100;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_stall_or_sleep(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_u64 time;

    time = item_array_at(&op_ctx->items, 0)->obj->integer;

    if (op_ctx->op->code == UACPI_AML_OP_SleepOp) {
        /*
         * ACPICA doesn't allow sleeps longer than 2 seconds,
         * so we shouldn't either.
         */
        if (time > 2000)
            time = 2000;

        uacpi_namespace_write_unlock();
        uacpi_kernel_sleep(time);
        uacpi_namespace_write_lock();
    } else {
        // Spec says this must evaluate to a ByteData
        if (time > 0xFF)
            time = 0xFF;
        uacpi_kernel_stall(time);
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_bcd(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_u64 src, dst = 0;
    uacpi_size i;
    uacpi_object *dst_obj;

    src = item_array_at(&op_ctx->items, 0)->obj->integer;
    dst_obj = item_array_at(&op_ctx->items, 2)->obj;
    i = 64;

    /*
     * NOTE: ACPICA just errors out for invalid BCD, but NT allows it just fine.
     *       FromBCD matches NT behavior 1:1 even for invalid BCD, but ToBCD
     *       produces different results when the input is too large.
     */
    if (op_ctx->op->code == UACPI_AML_OP_FromBCDOp) {
        do {
            i -= 4;
            dst *= 10;
            dst += (src >> i) & 0b1111;
        } while (i);
    } else {
        while (src != 0) {
            dst >>= 4;
            i -= 4;
            dst |= (src % 10) << 60;
            src /= 10;
        }

        dst >>= (i % 64);
    }

    dst_obj->integer = dst;
    return UACPI_STATUS_OK;
}

static uacpi_status handle_unload(struct execution_context *ctx)
{
    UACPI_UNUSED(ctx);

    /*
     * Technically this doesn't exist in the wild, from the dumps that I have
     * the only user of the Unload opcode is the Surface Pro 3, which triggers
     * an unload of some I2C-related table as a response to some event.
     *
     * This op has been long deprecated by the specification exactly because
     * it hasn't really been used by anyone and the fact that it introduces
     * an enormous layer of complexity, which no driver is really prepared to
     * deal with (aka namespace nodes disappearing under its feet).
     *
     * Just pretend we have actually unloaded whatever the AML asked for, if it
     * ever tries to re-load this table that will just skip opcodes that create
     * already existing objects, which should be good enough and mostly
     * transparent to the AML.
     */
    uacpi_warn("refusing to unload a table from AML\n");
    return UACPI_STATUS_OK;
}

static uacpi_status handle_logical_not(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *src, *dst;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 1)->obj;

    dst->type = UACPI_OBJECT_INTEGER;
    dst->integer = src->integer ? 0 : ones();

    return UACPI_STATUS_OK;
}

static uacpi_bool handle_logical_equality(uacpi_object *lhs, uacpi_object *rhs)
{
    uacpi_bool res = UACPI_FALSE;

    if (lhs->type == UACPI_OBJECT_STRING || lhs->type == UACPI_OBJECT_BUFFER) {
        res = lhs->buffer->size == rhs->buffer->size;

        if (res && lhs->buffer->size) {
            res = uacpi_memcmp(
                lhs->buffer->data,
                rhs->buffer->data,
                lhs->buffer->size
            ) == 0;
        }
    } else if (lhs->type == UACPI_OBJECT_INTEGER) {
        res = lhs->integer == rhs->integer;
    }

    return res;
}

static uacpi_bool handle_logical_less_or_greater(
    uacpi_aml_op op, uacpi_object *lhs, uacpi_object *rhs
)
{
    if (lhs->type == UACPI_OBJECT_STRING || lhs->type == UACPI_OBJECT_BUFFER) {
        int res;
        uacpi_buffer *lhs_buf, *rhs_buf;

        lhs_buf = lhs->buffer;
        rhs_buf = rhs->buffer;

        res = uacpi_memcmp(lhs_buf->data, rhs_buf->data,
                           UACPI_MIN(lhs_buf->size, rhs_buf->size));
        if (res == 0) {
            if (lhs_buf->size < rhs_buf->size)
                res = -1;
            else if (lhs_buf->size > rhs_buf->size)
                res = 1;
        }

        if (op == UACPI_AML_OP_LLessOp)
            return res < 0;

        return res > 0;
    }

    if (op == UACPI_AML_OP_LLessOp)
        return lhs->integer < rhs->integer;

    return lhs->integer > rhs->integer;
}

static uacpi_status handle_binary_logic(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_aml_op op = op_ctx->op->code;
    uacpi_object *lhs, *rhs, *dst;
    uacpi_bool res;

    lhs = item_array_at(&op_ctx->items, 0)->obj;
    rhs = item_array_at(&op_ctx->items, 1)->obj;
    dst = item_array_at(&op_ctx->items, 2)->obj;

    switch (op) {
    case UACPI_AML_OP_LEqualOp:
    case UACPI_AML_OP_LLessOp:
    case UACPI_AML_OP_LGreaterOp:
        // TODO: typecheck at parse time
        if (lhs->type != rhs->type) {
            uacpi_error(
                "don't know how to do a logical comparison of '%s' and '%s'\n",
                uacpi_object_type_to_string(lhs->type),
                uacpi_object_type_to_string(rhs->type)
            );
            return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
        }

        if (op == UACPI_AML_OP_LEqualOp)
            res = handle_logical_equality(lhs, rhs);
        else
            res = handle_logical_less_or_greater(op, lhs, rhs);
        break;
    default: {
        uacpi_u64 lhs_int, rhs_int;

        // NT only looks at the first 4 bytes of a buffer
        lhs_int = object_to_integer(lhs, 4);
        rhs_int = object_to_integer(rhs, 4);

        if (op == UACPI_AML_OP_LandOp)
            res = lhs_int && rhs_int;
        else
            res = lhs_int || rhs_int;
        break;
    }
    }

    dst->integer = res ? ones() : 0;
    return UACPI_STATUS_OK;
}

enum match_op {
    MTR = 0,
    MEQ = 1,
    MLE = 2,
    MLT = 3,
    MGE = 4,
    MGT = 5,
};

static uacpi_bool match_one(enum match_op op, uacpi_u64 lhs, uacpi_u64 rhs)
{
    switch (op) {
    case MTR:
        return UACPI_TRUE;
    case MEQ:
        return lhs == rhs;
    case MLE:
        return lhs <= rhs;
    case MLT:
        return lhs < rhs;
    case MGE:
        return lhs >= rhs;
    case MGT:
        return lhs > rhs;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_status handle_match(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_package *pkg;
    uacpi_u64 operand0, operand1, start_idx, i;
    enum match_op mop0, mop1;
    uacpi_object *dst;

    pkg = item_array_at(&op_ctx->items, 0)->obj->package;
    mop0 = item_array_at(&op_ctx->items, 1)->immediate;
    operand0 = item_array_at(&op_ctx->items, 2)->obj->integer;
    mop1 = item_array_at(&op_ctx->items, 3)->immediate;
    operand1 = item_array_at(&op_ctx->items, 4)->obj->integer;
    start_idx = item_array_at(&op_ctx->items, 5)->obj->integer;
    dst = item_array_at(&op_ctx->items, 6)->obj;

    for (i = start_idx; i < pkg->count; ++i) {
        uacpi_object *obj = pkg->objects[i];

        if (obj->type != UACPI_OBJECT_INTEGER)
            continue;

        if (match_one(mop0, obj->integer, operand0) &&
            match_one(mop1, obj->integer, operand1))
            break;
    }

    if (i < pkg->count)
        dst->integer = i;
    else
        dst->integer = ones();

    return UACPI_STATUS_OK;
}

/*
 * PkgLength :=
 *   PkgLeadByte |
 *   <pkgleadbyte bytedata> |
 *   <pkgleadbyte bytedata bytedata> | <pkgleadbyte bytedata bytedata bytedata>
 * PkgLeadByte :=
 *   <bit 7-6: bytedata count that follows (0-3)>
 *   <bit 5-4: only used if pkglength < 63>
 *   <bit 3-0: least significant package length nybble>
 */
static uacpi_status parse_package_length(struct call_frame *frame,
                                         struct package_length *out_pkg)
{
    uacpi_u32 left, size;
    uacpi_u8 *data, marker_length;

    out_pkg->begin = frame->code_offset;
    marker_length = 1;

    left = call_frame_code_bytes_left(frame);
    if (uacpi_unlikely(left < 1))
        return UACPI_STATUS_AML_BAD_ENCODING;

    data = call_frame_cursor(frame);
    marker_length += *data >> 6;

    if (uacpi_unlikely(left < marker_length))
        return UACPI_STATUS_AML_BAD_ENCODING;

    switch (marker_length) {
    case 1:
        size = *data & 0b111111;
        break;
    case 2:
    case 3:
    case 4: {
        uacpi_u32 temp_byte = 0;

        size = *data & 0b1111;
        uacpi_memcpy(&temp_byte, data + 1, marker_length - 1);

        // marker_length - 1 is at most 3, so this shift is safe
        size |= temp_byte << 4;
        break;
    }
    }

    frame->code_offset += marker_length;

    out_pkg->end = out_pkg->begin + size;
    if (uacpi_unlikely(out_pkg->end < out_pkg->begin)) {
        uacpi_error(
            "PkgLength overflow: start=%u, size=%u\n", out_pkg->begin, size
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    return UACPI_STATUS_OK;
}

/*
 * ByteData
 * // bit 0-2: ArgCount (0-7)
 * // bit 3: SerializeFlag
 * //   0 NotSerialized
 * //   1 Serialized
 * // bit 4-7: SyncLevel (0x00-0x0f)
 */
static void init_method_flags(uacpi_control_method *method, uacpi_u8 flags_byte)
{
    method->args = flags_byte & 0b111;
    method->is_serialized = (flags_byte >> 3) & 1;
    method->sync_level = flags_byte >> 4;
}

static uacpi_status handle_create_method(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct uacpi_control_method *this_method, *method;
    struct package_length *pkg;
    struct uacpi_namespace_node *node;
    struct uacpi_object *dst;
    uacpi_u32 method_begin_offset, method_size;

    this_method = ctx->cur_frame->method;
    pkg = &item_array_at(&op_ctx->items, 0)->pkg;
    node = item_array_at(&op_ctx->items, 1)->node;
    method_begin_offset = item_array_at(&op_ctx->items, 3)->immediate;

    if (uacpi_unlikely(pkg->end < pkg->begin ||
                       pkg->end < method_begin_offset ||
                       pkg->end > this_method->size)) {
        uacpi_error(
            "invalid method %.4s bounds [%u..%u] (parent size is %u)\n",
            node->name.text, method_begin_offset, pkg->end, this_method->size
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    dst = item_array_at(&op_ctx->items, 4)->obj;

    method = dst->method;
    method_size = pkg->end - method_begin_offset;

    if (method_size) {
        method->code = uacpi_kernel_alloc(method_size);
        if (uacpi_unlikely(method->code == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        uacpi_memcpy(
            method->code,
            ctx->cur_frame->method->code + method_begin_offset,
            method_size
        );
        method->size = method_size;
        method->owns_code = 1;
    }

    init_method_flags(method, item_array_at(&op_ctx->items, 2)->immediate);

    node->object = uacpi_create_internal_reference(UACPI_REFERENCE_KIND_NAMED,
                                                   dst);
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_mutex_or_event(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_namespace_node *node;
    uacpi_object *dst;

    node = item_array_at(&op_ctx->items, 0)->node;

    if (op_ctx->op->code == UACPI_AML_OP_MutexOp) {
        dst = item_array_at(&op_ctx->items, 2)->obj;

        // bits 0-3: SyncLevel (0x00-0x0f), bits 4-7: Reserved (must be 0)
        dst->mutex->sync_level = item_array_at(&op_ctx->items, 1)->immediate;
        dst->mutex->sync_level &= 0b1111;
    } else {
        dst = item_array_at(&op_ctx->items, 1)->obj;
    }

    node->object = uacpi_create_internal_reference(
        UACPI_REFERENCE_KIND_NAMED,
        dst
    );
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_event_ctl(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *obj;

    obj = uacpi_unwrap_internal_reference(
        item_array_at(&op_ctx->items, 0)->obj
    );
    if (uacpi_unlikely(obj->type != UACPI_OBJECT_EVENT)) {
        uacpi_error(
            "%s: invalid argument '%s', expected an Event object\n",
            op_ctx->op->name, uacpi_object_type_to_string(obj->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    switch (op_ctx->op->code)
    {
    case UACPI_AML_OP_SignalOp:
        uacpi_kernel_signal_event(obj->event->handle);
        break;
    case UACPI_AML_OP_ResetOp:
        uacpi_kernel_reset_event(obj->event->handle);
        break;
    case UACPI_AML_OP_WaitOp: {
        uacpi_u64 timeout;
        uacpi_bool ret;

        timeout = item_array_at(&op_ctx->items, 1)->obj->integer;
        if (timeout > 0xFFFF)
            timeout = 0xFFFF;

        uacpi_namespace_write_unlock();
        ret = uacpi_kernel_wait_for_event(obj->event->handle, timeout);
        uacpi_namespace_write_lock();

        /*
         * The return value here is inverted, we return 0 for success and Ones
         * for timeout and everything else.
         */
        if (ret)
            item_array_at(&op_ctx->items, 2)->obj->integer = 0;
        break;
    }
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_mutex_ctl(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_object *obj;

    obj = uacpi_unwrap_internal_reference(
        item_array_at(&op_ctx->items, 0)->obj
    );
    if (uacpi_unlikely(obj->type != UACPI_OBJECT_MUTEX)) {
        uacpi_error(
            "%s: invalid argument '%s', expected a Mutex object\n",
            op_ctx->op->name, uacpi_object_type_to_string(obj->type)
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    switch (op_ctx->op->code)
    {
    case UACPI_AML_OP_AcquireOp: {
        uacpi_u64 timeout;
        uacpi_u64 *return_value;
        uacpi_status ret;

        return_value = &item_array_at(&op_ctx->items, 2)->obj->integer;

        if (uacpi_unlikely(ctx->sync_level > obj->mutex->sync_level)) {
            uacpi_warn(
                "ignoring attempt to acquire mutex @%p with a lower sync level "
                "(%d < %d)\n", obj->mutex, obj->mutex->sync_level,
                ctx->sync_level
            );
            break;
        }

        timeout = item_array_at(&op_ctx->items, 1)->immediate;
        if (timeout > 0xFFFF)
            timeout = 0xFFFF;

        if (uacpi_this_thread_owns_aml_mutex(obj->mutex)) {
            ret = uacpi_acquire_aml_mutex(obj->mutex, timeout);
            if (uacpi_likely_success(ret))
                *return_value = 0;
            break;
        }

        ret = uacpi_acquire_aml_mutex(obj->mutex, timeout);
        if (uacpi_unlikely_error(ret))
            break;

        ret = held_mutexes_array_push(&ctx->held_mutexes, obj->mutex);
        if (uacpi_unlikely_error(ret)) {
            uacpi_release_aml_mutex(obj->mutex);
            return ret;
        }

        ctx->sync_level = obj->mutex->sync_level;
        *return_value = 0;
        break;
    }

    case UACPI_AML_OP_ReleaseOp: {
        uacpi_status ret;

        if (!uacpi_this_thread_owns_aml_mutex(obj->mutex)) {
            uacpi_warn(
                "attempted to release not-previously-acquired mutex object "
                "@%p (%p)\n", obj->mutex, obj->mutex->handle
            );
            break;
        }

        ret = held_mutexes_array_remove_and_release(
            &ctx->held_mutexes, obj->mutex,
            FORCE_RELEASE_NO
        );
        if (uacpi_likely_success(ret)) {
            uacpi_mutex **last_mutex;

            last_mutex = held_mutexes_array_last(&ctx->held_mutexes);
            if (last_mutex == UACPI_NULL) {
                ctx->sync_level = 0;
                break;
            }

            ctx->sync_level = (*last_mutex)->sync_level;
        }
        break;
    }

    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status handle_notify(struct execution_context *ctx)
{
    uacpi_status ret;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct uacpi_namespace_node *node;
    uacpi_u64 value;

    node = item_array_at(&op_ctx->items, 0)->node;
    value = item_array_at(&op_ctx->items, 1)->obj->integer;

    ret = uacpi_notify_all(node, value);
    if (uacpi_likely_success(ret))
        return ret;

    if (ret == UACPI_STATUS_NO_HANDLER) {
        const uacpi_char *path;

        path = uacpi_namespace_node_generate_absolute_path(node);
        uacpi_warn(
            "ignoring firmware Notify(%s, 0x%"UACPI_PRIX64") request, "
            "no listeners\n", path, UACPI_FMT64(value)
        );
        uacpi_free_dynamic_string(path);

        return UACPI_STATUS_OK;
    }

    if (ret == UACPI_STATUS_INVALID_ARGUMENT) {
        uacpi_error("Notify() called on an invalid object %.4s\n",
                    node->name.text);
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return ret;
}

static uacpi_status handle_firmware_request(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_firmware_request req = { 0 };

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_BreakPointOp:
        req.type = UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT;
        req.breakpoint.ctx = ctx;
        break;
    case UACPI_AML_OP_FatalOp:
        req.type = UACPI_FIRMWARE_REQUEST_TYPE_FATAL;
        req.fatal.type = item_array_at(&op_ctx->items, 0)->immediate;
        req.fatal.code = item_array_at(&op_ctx->items, 1)->immediate;
        req.fatal.arg = item_array_at(&op_ctx->items, 2)->obj->integer;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    uacpi_namespace_write_unlock();
    uacpi_kernel_handle_firmware_request(&req);
    uacpi_namespace_write_lock();

    return UACPI_STATUS_OK;
}

static uacpi_status handle_create_named(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct uacpi_namespace_node *node;
    uacpi_object *src;

    node = item_array_at(&op_ctx->items, 0)->node;
    src = item_array_at(&op_ctx->items, 1)->obj;

    node->object = uacpi_create_internal_reference(UACPI_REFERENCE_KIND_NAMED,
                                                   src);
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

static uacpi_object_type buffer_field_get_read_type(
    struct uacpi_buffer_field *field
)
{
    if (field->bit_length > (g_uacpi_rt_ctx.is_rev1 ? 32u : 64u) ||
        field->force_buffer)
        return UACPI_OBJECT_BUFFER;

    return UACPI_OBJECT_INTEGER;
}

static uacpi_status field_get_read_type(
    uacpi_object *obj, uacpi_object_type *out_type
)
{
    if (obj->type == UACPI_OBJECT_BUFFER_FIELD) {
        *out_type = buffer_field_get_read_type(&obj->buffer_field);
        return UACPI_STATUS_OK;
    }

    return uacpi_field_unit_get_read_type(obj->field_unit, out_type);
}

static uacpi_status field_byte_size(
    uacpi_object *obj, uacpi_size *out_size
)
{
    uacpi_size bit_length;

    if (obj->type == UACPI_OBJECT_BUFFER_FIELD) {
        bit_length = obj->buffer_field.bit_length;
    } else {
        uacpi_status ret;

        ret = uacpi_field_unit_get_bit_length(obj->field_unit, &bit_length);
        if (uacpi_unlikely_error(ret))
            return ret;
    }

    *out_size = uacpi_round_up_bits_to_bytes(bit_length);
    return UACPI_STATUS_OK;
}

static uacpi_status handle_field_read(struct execution_context *ctx)
{
    uacpi_status ret;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct uacpi_namespace_node *node;
    uacpi_object *src_obj, *dst_obj;
    uacpi_size dst_size;
    void *dst = UACPI_NULL;
    uacpi_data_view wtr_response = { 0 };

    node = item_array_at(&op_ctx->items, 0)->node;
    src_obj = uacpi_namespace_node_get_object(node);
    dst_obj = item_array_at(&op_ctx->items, 1)->obj;

    if (op_ctx->op->code == UACPI_AML_OP_InternalOpReadFieldAsBuffer) {
        uacpi_buffer *buf;

        ret = field_byte_size(src_obj, &dst_size);
        if (uacpi_unlikely_error(ret))
            return ret;

        if (dst_size != 0) {
            buf = dst_obj->buffer;

            dst = uacpi_kernel_alloc_zeroed(dst_size);
            if (dst == UACPI_NULL)
                return UACPI_STATUS_OUT_OF_MEMORY;

            buf->data = dst;
            buf->size = dst_size;
        }
    } else {
        dst = &dst_obj->integer;
        dst_size = sizeof(uacpi_u64);
    }

    if (src_obj->type == UACPI_OBJECT_BUFFER_FIELD) {
        uacpi_read_buffer_field(&src_obj->buffer_field, dst);
        return UACPI_STATUS_OK;
    }

    ret = uacpi_read_field_unit(
        src_obj->field_unit, dst, dst_size, &wtr_response
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    if (wtr_response.data != UACPI_NULL) {
        uacpi_buffer *buf;

        buf = dst_obj->buffer;
        buf->data = wtr_response.data;
        buf->size = wtr_response.length;
    }

    return ret;
}

static uacpi_status handle_create_buffer_field(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;
    struct uacpi_namespace_node *node;
    uacpi_buffer *src_buf;
    uacpi_object *field_obj;
    uacpi_buffer_field *field;

    /*
     * Layout of items here:
     * [0] -> Type checked source buffer object
     * [1] -> Byte/bit index integer object
     * [2] (  if     CreateField) -> bit length integer object
     * [3] (2 if not CreateField) -> the new namespace node
     * [4] (3 if not CreateField) -> the buffer field object we're creating here
     */
    src_buf = item_array_at(&op_ctx->items, 0)->obj->buffer;

    if (op_ctx->op->code == UACPI_AML_OP_CreateFieldOp) {
        uacpi_object *idx_obj, *len_obj;

        idx_obj = item_array_at(&op_ctx->items, 1)->obj;
        len_obj = item_array_at(&op_ctx->items, 2)->obj;
        node = item_array_at(&op_ctx->items, 3)->node;
        field_obj = item_array_at(&op_ctx->items, 4)->obj;
        field = &field_obj->buffer_field;

        field->bit_index = idx_obj->integer;

        if (uacpi_unlikely(!len_obj->integer ||
                            len_obj->integer > 0xFFFFFFFF)) {
            uacpi_error("invalid bit field length (%u)\n", field->bit_length);
            return UACPI_STATUS_AML_BAD_ENCODING;
        }

        field->bit_length = len_obj->integer;
        field->force_buffer = UACPI_TRUE;
    } else {
        uacpi_object *idx_obj;

        idx_obj = item_array_at(&op_ctx->items, 1)->obj;
        node = item_array_at(&op_ctx->items, 2)->node;
        field_obj = item_array_at(&op_ctx->items, 3)->obj;
        field = &field_obj->buffer_field;

        field->bit_index = idx_obj->integer;
        switch (op_ctx->op->code) {
        case UACPI_AML_OP_CreateBitFieldOp:
            field->bit_length = 1;
            break;
        case UACPI_AML_OP_CreateByteFieldOp:
            field->bit_length = 8;
            break;
        case UACPI_AML_OP_CreateWordFieldOp:
            field->bit_length = 16;
            break;
        case UACPI_AML_OP_CreateDWordFieldOp:
            field->bit_length = 32;
            break;
        case UACPI_AML_OP_CreateQWordFieldOp:
            field->bit_length = 64;
            break;
        default:
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        if (op_ctx->op->code != UACPI_AML_OP_CreateBitFieldOp)
            field->bit_index *= 8;
    }

    if (uacpi_unlikely((field->bit_index + field->bit_length) >
                       src_buf->size * 8)) {
        uacpi_error(
            "invalid buffer field: bits [%zu..%zu], buffer size is %zu bytes\n",
            field->bit_index, field->bit_index + field->bit_length,
            src_buf->size
        );
        return UACPI_STATUS_AML_OUT_OF_BOUNDS_INDEX;
    }

    field->backing = src_buf;
    uacpi_shareable_ref(field->backing);
    node->object = uacpi_create_internal_reference(UACPI_REFERENCE_KIND_NAMED,
                                                   field_obj);
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_control_flow(struct execution_context *ctx)
{
    struct call_frame *frame = ctx->cur_frame;
    struct op_context *op_ctx = ctx->cur_op_ctx;

    if (uacpi_unlikely(frame->last_while == UACPI_NULL)) {
        uacpi_error(
            "attempting to %s outside of a While block\n",
            op_ctx->op->code == UACPI_AML_OP_BreakOp ? "Break" : "Continue"
        );
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    for (;;) {
        if (ctx->cur_block != frame->last_while) {
            frame_reset_post_end_block(ctx, ctx->cur_block->type);
            continue;
        }

        if (op_ctx->op->code == UACPI_AML_OP_BreakOp)
            frame->code_offset = ctx->cur_block->end;
        else
            frame->code_offset = ctx->cur_block->begin;
        frame_reset_post_end_block(ctx, ctx->cur_block->type);
        break;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status create_named_scope(struct op_context *op_ctx)
{
    uacpi_namespace_node *node;
    uacpi_object *obj;

    node = item_array_at(&op_ctx->items, 1)->node;
    obj = item_array_last(&op_ctx->items)->obj;

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_ProcessorOp: {
        uacpi_processor *proc = obj->processor;
        proc->id = item_array_at(&op_ctx->items, 2)->immediate;
        proc->block_address = item_array_at(&op_ctx->items, 3)->immediate;
        proc->block_length = item_array_at(&op_ctx->items, 4)->immediate;
        break;
    }

    case UACPI_AML_OP_PowerResOp: {
        uacpi_power_resource *power_res = &obj->power_resource;
        power_res->system_level = item_array_at(&op_ctx->items, 2)->immediate;
        power_res->resource_order = item_array_at(&op_ctx->items, 3)->immediate;
        break;
    }

    default:
        break;
    }

    node->object = uacpi_create_internal_reference(UACPI_REFERENCE_KIND_NAMED,
                                                   obj);
    if (uacpi_unlikely(node->object == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    return UACPI_STATUS_OK;
}

static uacpi_status handle_code_block(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;

    switch (op_ctx->op->code) {
    case UACPI_AML_OP_ProcessorOp:
    case UACPI_AML_OP_PowerResOp:
    case UACPI_AML_OP_ThermalZoneOp:
    case UACPI_AML_OP_DeviceOp: {
        uacpi_status ret;

        ret = create_named_scope(op_ctx);
        if (uacpi_unlikely_error(ret))
            return ret;

        UACPI_FALLTHROUGH;
    }
    case UACPI_AML_OP_ScopeOp:
    case UACPI_AML_OP_IfOp:
    case UACPI_AML_OP_ElseOp:
    case UACPI_AML_OP_WhileOp: {
        break;
    }
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    return begin_block_execution(ctx);
}

static uacpi_status handle_return(struct execution_context *ctx)
{
    uacpi_status ret;
    uacpi_object *dst = UACPI_NULL;

    ctx->cur_frame->code_offset = ctx->cur_frame->method->size;
    ret = method_get_ret_object(ctx, &dst);

    if (uacpi_unlikely_error(ret))
        return ret;
    if (dst == UACPI_NULL)
        return UACPI_STATUS_OK;

    /*
     * Should be possible to move here if method returns a literal
     * like Return(Buffer { ... }), otherwise we have to copy just to
     * be safe.
     */
    return uacpi_object_assign(
        dst,
        item_array_at(&ctx->cur_op_ctx->items, 0)->obj,
        UACPI_ASSIGN_BEHAVIOR_DEEP_COPY
    );
}

static void refresh_ctx_pointers(struct execution_context *ctx)
{
    struct call_frame *frame = ctx->cur_frame;

    if (frame == UACPI_NULL) {
        ctx->cur_op_ctx = UACPI_NULL;
        ctx->prev_op_ctx = UACPI_NULL;
        ctx->cur_block = UACPI_NULL;
        return;
    }

    ctx->cur_op_ctx = op_context_array_last(&frame->pending_ops);
    ctx->prev_op_ctx = op_context_array_one_before_last(&frame->pending_ops);
    ctx->cur_block = code_block_array_last(&frame->code_blocks);
}

static uacpi_bool ctx_has_non_preempted_op(struct execution_context *ctx)
{
    return ctx->cur_op_ctx && !ctx->cur_op_ctx->preempted;
}

enum op_trace_action_type {
    OP_TRACE_ACTION_BEGIN,
    OP_TRACE_ACTION_RESUME,
    OP_TRACE_ACTION_END,
};

static const uacpi_char *const op_trace_action_types[3] = {
    [OP_TRACE_ACTION_BEGIN] = "BEGIN",
    [OP_TRACE_ACTION_RESUME] = "RESUME",
    [OP_TRACE_ACTION_END] = "END",
};

static inline void trace_op(
    const struct uacpi_op_spec *op, enum op_trace_action_type action
)
{
    uacpi_debug(
        "%s OP '%s' (0x%04X)\n",
        op_trace_action_types[action], op->name, op->code
    );
}

static inline void trace_pop(uacpi_u8 pop)
{
    uacpi_debug("    pOP: %s (0x%02X)\n", uacpi_parse_op_to_string(pop), pop);
}

static uacpi_status frame_push_args(struct call_frame *frame,
                                    struct op_context *op_ctx)
{
    uacpi_size i;

    /*
     * MethodCall items:
     * items[0] -> method namespace node
     * items[1] -> immediate that was used for parsing the arguments
     * items[2...nargs-1] -> method arguments
     * items[-1] -> return value object
     *
     * Here we only care about the arguments though.
     */
    for (i = 2; i < item_array_size(&op_ctx->items) - 1; i++) {
        uacpi_object *src, *dst;

        src = item_array_at(&op_ctx->items, i)->obj;

        dst = uacpi_create_internal_reference(UACPI_REFERENCE_KIND_ARG, src);
        if (uacpi_unlikely(dst == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        frame->args[i - 2] = dst;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status frame_setup_base_scope(struct call_frame *frame,
                                           uacpi_namespace_node *scope,
                                           uacpi_control_method *method)
{
    struct code_block *block;

    block = code_block_array_alloc(&frame->code_blocks);
    if (uacpi_unlikely(block == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    block->type = CODE_BLOCK_SCOPE;
    block->node = scope;
    block->begin = 0;
    block->end = method->size;
    frame->method = method;
    frame->cur_scope = scope;
    return UACPI_STATUS_OK;
}

static uacpi_status push_new_frame(struct execution_context *ctx,
                                   struct call_frame **out_frame)
{
    struct call_frame_array *call_stack = &ctx->call_stack;
    struct call_frame *prev_frame;

    *out_frame = call_frame_array_calloc(call_stack);
    if (uacpi_unlikely(*out_frame == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    /*
     * Allocating a new frame might have reallocated the dynamic buffer so our
     * execution_context members might now be pointing to freed memory.
     * Refresh them here.
     */
    prev_frame = call_frame_array_one_before_last(call_stack);
    ctx->cur_frame = prev_frame;
    refresh_ctx_pointers(ctx);

    return UACPI_STATUS_OK;
}

static uacpi_bool maybe_end_block(struct execution_context *ctx)
{
    struct code_block *block = ctx->cur_block;
    struct call_frame *cur_frame = ctx->cur_frame;

    if (!block)
        return UACPI_FALSE;
    if (cur_frame->code_offset != block->end)
        return UACPI_FALSE;

    if (block->type == CODE_BLOCK_WHILE)
        cur_frame->code_offset = block->begin;

    frame_reset_post_end_block(ctx, block->type);
    return UACPI_TRUE;
}

static uacpi_status store_to_target(
    uacpi_object *dst, uacpi_object *src, uacpi_data_view *wtr_response
)
{
    uacpi_status ret;

    switch (dst->type) {
    case UACPI_OBJECT_DEBUG:
        ret = debug_store(src);
        break;
    case UACPI_OBJECT_REFERENCE:
        ret = store_to_reference(dst, src, wtr_response);
        break;

    case UACPI_OBJECT_BUFFER_INDEX:
        src = uacpi_unwrap_internal_reference(src);
        ret = object_assign_with_implicit_cast(dst, src, wtr_response);
        break;

    case UACPI_OBJECT_INTEGER:
        // NULL target
        if (dst->integer == 0) {
            ret = UACPI_STATUS_OK;
            break;
        }
        UACPI_FALLTHROUGH;
    default:
        uacpi_error("attempted to store to an invalid target: %s\n",
                    uacpi_object_type_to_string(dst->type));
        ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return ret;
}

static uacpi_status handle_copy_object_or_store(struct execution_context *ctx)
{
    uacpi_object *src, *dst;
    struct op_context *op_ctx = ctx->cur_op_ctx;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 1)->obj;

    if (op_ctx->op->code == UACPI_AML_OP_StoreOp) {
        uacpi_status ret;
        uacpi_data_view wtr_response = { 0 };

        ret = store_to_target(dst, src, &wtr_response);
        if (uacpi_unlikely_error(ret))
            return ret;

        /*
         * This was a write-then-read field access since we got a response
         * buffer back from this store. Now we have to return this buffer
         * as a prvalue from the StoreOp so that it can be used by AML to
         * retrieve the response.
         */
        if (wtr_response.data != UACPI_NULL) {
            uacpi_object *wtr_response_obj;

            wtr_response_obj = uacpi_create_object(UACPI_OBJECT_BUFFER);
            if (uacpi_unlikely(wtr_response_obj == UACPI_NULL)) {
                uacpi_free(wtr_response.data, wtr_response.length);
                return UACPI_STATUS_OUT_OF_MEMORY;
            }

            wtr_response_obj->buffer->data = wtr_response.data;
            wtr_response_obj->buffer->size = wtr_response.length;

            uacpi_object_unref(src);
            item_array_at(&op_ctx->items, 0)->obj = wtr_response_obj;
        }

        return ret;
    }

    if (dst->type != UACPI_OBJECT_REFERENCE)
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;

    return copy_object_to_reference(dst, src);
}

static uacpi_status handle_inc_dec(struct execution_context *ctx)
{
    uacpi_object *src, *dst;
    struct op_context *op_ctx = ctx->cur_op_ctx;
    uacpi_bool field_allowed = UACPI_FALSE;
    uacpi_object_type true_src_type;
    uacpi_status ret;

    src = item_array_at(&op_ctx->items, 0)->obj;
    dst = item_array_at(&op_ctx->items, 1)->obj;

    if (src->type == UACPI_OBJECT_REFERENCE) {
        /*
         * Increment/Decrement are the only two operators that modify the value
         * in-place, thus we need very specific dereference rules here.
         *
         * Reading buffer fields & field units is only allowed if we were passed
         * a namestring directly as opposed to some nested reference chain
         * containing a field at the bottom.
         */
        if (src->flags == UACPI_REFERENCE_KIND_NAMED)
            field_allowed = src->inner_object->type != UACPI_OBJECT_REFERENCE;

        src = reference_unwind(src)->inner_object;
    } // else buffer index

    true_src_type = src->type;

    switch (true_src_type) {
    case UACPI_OBJECT_INTEGER:
        dst->integer = src->integer;
        break;
    case UACPI_OBJECT_FIELD_UNIT:
    case UACPI_OBJECT_BUFFER_FIELD:
        if (uacpi_unlikely(!field_allowed))
            goto out_bad_type;

        ret = field_get_read_type(src, &true_src_type);
        if (uacpi_unlikely_error(ret))
            goto out_bad_type;
        if (true_src_type != UACPI_OBJECT_INTEGER)
            goto out_bad_type;

        if (src->type == UACPI_OBJECT_FIELD_UNIT) {
            ret = uacpi_read_field_unit(
                src->field_unit, &dst->integer, sizeof_int(),
                UACPI_NULL
            );
            if (uacpi_unlikely_error(ret))
                return ret;
        } else {
            uacpi_read_buffer_field(&src->buffer_field, &dst->integer);
        }
        break;
    case UACPI_OBJECT_BUFFER_INDEX:
        dst->integer = *buffer_index_cursor(&src->buffer_index);
        break;
    default:
        goto out_bad_type;
    }

    if (op_ctx->op->code == UACPI_AML_OP_IncrementOp)
        dst->integer++;
    else
        dst->integer--;

    return UACPI_STATUS_OK;

out_bad_type:
    uacpi_error("Increment/Decrement: invalid object type '%s'\n",
                uacpi_object_type_to_string(true_src_type));
    return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
}

static uacpi_status enter_method(
    struct execution_context *ctx, struct call_frame *new_frame,
    uacpi_control_method *method
)
{
    uacpi_status ret = UACPI_STATUS_OK;

    uacpi_shareable_ref(method);

    if (!method->is_serialized)
        return ret;

    if (uacpi_unlikely(ctx->sync_level > method->sync_level)) {
        uacpi_error(
            "cannot invoke method @%p, sync level %d is too low "
            "(current is %d)\n",
            method, method->sync_level, ctx->sync_level
        );
        return UACPI_STATUS_AML_SYNC_LEVEL_TOO_HIGH;
    }

    if (method->mutex == UACPI_NULL) {
        method->mutex = uacpi_create_mutex();
        if (uacpi_unlikely(method->mutex == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;
        method->mutex->sync_level = method->sync_level;
    }

    if (!uacpi_this_thread_owns_aml_mutex(method->mutex)) {
        ret = uacpi_acquire_aml_mutex(method->mutex, 0xFFFF);
        if (uacpi_unlikely_error(ret))
            return ret;

        ret = held_mutexes_array_push(&ctx->held_mutexes, method->mutex);
        if (uacpi_unlikely_error(ret)) {
            uacpi_release_aml_mutex(method->mutex);
            return ret;
        }
    }

    new_frame->prev_sync_level = ctx->sync_level;
    ctx->sync_level = method->sync_level;
    return UACPI_STATUS_OK;
}

static uacpi_status push_op(struct execution_context *ctx)
{
    struct call_frame *frame = ctx->cur_frame;
    struct op_context *op_ctx;

    op_ctx = op_context_array_calloc(&frame->pending_ops);
    if (op_ctx == UACPI_NULL)
        return UACPI_STATUS_OUT_OF_MEMORY;

    op_ctx->op = ctx->cur_op;
    refresh_ctx_pointers(ctx);
    return UACPI_STATUS_OK;
}

static uacpi_bool pop_item(struct op_context *op_ctx)
{
    struct item *item;

    if (item_array_size(&op_ctx->items) == 0)
        return UACPI_FALSE;

    item = item_array_last(&op_ctx->items);

    if (item->type == ITEM_OBJECT)
        uacpi_object_unref(item->obj);

    if (item->type == ITEM_NAMESPACE_NODE)
        uacpi_namespace_node_unref(item->node);

    item_array_pop(&op_ctx->items);
    return UACPI_TRUE;
}

static void pop_op(struct execution_context *ctx)
{
    struct call_frame *frame = ctx->cur_frame;
    struct op_context *cur_op_ctx = ctx->cur_op_ctx;

    while (pop_item(cur_op_ctx));

    item_array_clear(&cur_op_ctx->items);
    op_context_array_pop(&frame->pending_ops);
    refresh_ctx_pointers(ctx);
}

static void call_frame_clear(struct call_frame *frame)
{
    uacpi_size i;
    op_context_array_clear(&frame->pending_ops);
    code_block_array_clear(&frame->code_blocks);

    while (temp_namespace_node_array_size(&frame->temp_nodes) != 0) {
        uacpi_namespace_node *node;

        node = *temp_namespace_node_array_last(&frame->temp_nodes);
        uacpi_namespace_node_uninstall(node);
        temp_namespace_node_array_pop(&frame->temp_nodes);
    }
    temp_namespace_node_array_clear(&frame->temp_nodes);

    for (i = 0; i < 7; ++i)
        uacpi_object_unref(frame->args[i]);
    for (i = 0; i < 8; ++i)
        uacpi_object_unref(frame->locals[i]);

    uacpi_method_unref(frame->method);
}

static uacpi_u8 parse_op_generates_item[0x100] = {
    [UACPI_PARSE_OP_SIMPLE_NAME] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_SUPERNAME] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_TERM_ARG] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_OPERAND] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_STRING] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_COMPUTATIONAL_DATA] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_TARGET] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_PKGLEN] = ITEM_PACKAGE_LENGTH,
    [UACPI_PARSE_OP_TRACKED_PKGLEN] = ITEM_PACKAGE_LENGTH,
    [UACPI_PARSE_OP_CREATE_NAMESTRING] = ITEM_NAMESPACE_NODE,
    [UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD] = ITEM_NAMESPACE_NODE,
    [UACPI_PARSE_OP_EXISTING_NAMESTRING] = ITEM_NAMESPACE_NODE,
    [UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL] = ITEM_NAMESPACE_NODE,
    [UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD] = ITEM_NAMESPACE_NODE,
    [UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT] = ITEM_OBJECT,
    [UACPI_PARSE_OP_LOAD_INLINE_IMM] = ITEM_IMMEDIATE,
    [UACPI_PARSE_OP_LOAD_ZERO_IMM] = ITEM_IMMEDIATE,
    [UACPI_PARSE_OP_LOAD_IMM] = ITEM_IMMEDIATE,
    [UACPI_PARSE_OP_LOAD_IMM_AS_OBJECT] = ITEM_OBJECT,
    [UACPI_PARSE_OP_LOAD_FALSE_OBJECT] = ITEM_OBJECT,
    [UACPI_PARSE_OP_LOAD_TRUE_OBJECT] = ITEM_OBJECT,
    [UACPI_PARSE_OP_OBJECT_ALLOC] = ITEM_OBJECT,
    [UACPI_PARSE_OP_OBJECT_ALLOC_TYPED] = ITEM_OBJECT,
    [UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC] = ITEM_EMPTY_OBJECT,
    [UACPI_PARSE_OP_OBJECT_CONVERT_TO_SHALLOW_COPY] = ITEM_OBJECT,
    [UACPI_PARSE_OP_OBJECT_CONVERT_TO_DEEP_COPY] = ITEM_OBJECT,
    [UACPI_PARSE_OP_RECORD_AML_PC] = ITEM_IMMEDIATE,
};

static const uacpi_u8 *op_decode_cursor(const struct op_context *ctx)
{
    const struct uacpi_op_spec *spec = ctx->op;

    if (spec->properties & UACPI_OP_PROPERTY_OUT_OF_LINE)
        return &spec->indirect_decode_ops[ctx->pc];

    return &spec->decode_ops[ctx->pc];
}

static uacpi_u8 op_decode_byte(struct op_context *ctx)
{
    uacpi_u8 byte;

    byte = *op_decode_cursor(ctx);
    ctx->pc++;

    return byte;
}

static uacpi_aml_op op_decode_aml_op(struct op_context *op_ctx)
{
    uacpi_aml_op op = 0;

    op |= op_decode_byte(op_ctx);
    op |= op_decode_byte(op_ctx) << 8;

    return op;
}

// MSVC doesn't support __VA_OPT__ so we do this weirdness
#define EXEC_OP_DO_LVL(lvl, reason, ...)                              \
    uacpi_##lvl("Op 0x%04X ('%s'): "reason"\n",                       \
                op_ctx->op->code, op_ctx->op->name __VA_ARGS__)

#define EXEC_OP_DO_ERR(reason, ...) EXEC_OP_DO_LVL(error, reason, __VA_ARGS__)
#define EXEC_OP_DO_WARN(reason, ...) EXEC_OP_DO_LVL(warn, reason, __VA_ARGS__)

#define EXEC_OP_ERR_2(reason, arg0, arg1) EXEC_OP_DO_ERR(reason, ,arg0, arg1)
#define EXEC_OP_ERR_1(reason, arg0) EXEC_OP_DO_ERR(reason, ,arg0)
#define EXEC_OP_ERR(reason) EXEC_OP_DO_ERR(reason)

#define EXEC_OP_WARN(reason) EXEC_OP_DO_WARN(reason)

#define SPEC_SIMPLE_NAME "SimpleName := NameString | ArgObj | LocalObj"
#define SPEC_SUPER_NAME \
    "SuperName := SimpleName | DebugObj | ReferenceTypeOpcode"
#define SPEC_TERM_ARG \
    "TermArg := ExpressionOpcode | DataObject | ArgObj | LocalObj"
#define SPEC_OPERAND "Operand := TermArg => Integer"
#define SPEC_STRING "String := TermArg => String"
#define SPEC_TARGET "Target := SuperName | NullName"

#define SPEC_COMPUTATIONAL_DATA                                             \
    "ComputationalData := ByteConst | WordConst | DWordConst | QWordConst " \
    "| String | ConstObj | RevisionOp | DefBuffer"

static uacpi_bool op_wants_supername(enum uacpi_parse_op op)
{
    switch (op) {
    case UACPI_PARSE_OP_SIMPLE_NAME:
    case UACPI_PARSE_OP_SUPERNAME:
    case UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED:
    case UACPI_PARSE_OP_TARGET:
        return UACPI_TRUE;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_bool op_wants_term_arg_or_operand(enum uacpi_parse_op op)
{
    switch (op) {
    case UACPI_PARSE_OP_TERM_ARG:
    case UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL:
    case UACPI_PARSE_OP_OPERAND:
    case UACPI_PARSE_OP_STRING:
    case UACPI_PARSE_OP_COMPUTATIONAL_DATA:
        return UACPI_TRUE;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_bool op_allows_unresolved(enum uacpi_parse_op op)
{
    switch (op) {
    case UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED:
    case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED:
    case UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL:
        return UACPI_TRUE;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_bool op_allows_unresolved_if_load(enum uacpi_parse_op op)
{
    switch (op) {
    case UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD:
    case UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD:
        return UACPI_TRUE;
    default:
        return UACPI_FALSE;
    }
}

static uacpi_status op_typecheck(const struct op_context *op_ctx,
                                 const struct op_context *cur_op_ctx)
{
    const uacpi_char *expected_type_str;
    uacpi_u8 ok_mask = 0;
    uacpi_u8 props = cur_op_ctx->op->properties;

    switch (*op_decode_cursor(op_ctx)) {
    // SimpleName := NameString | ArgObj | LocalObj
    case UACPI_PARSE_OP_SIMPLE_NAME:
        expected_type_str = SPEC_SIMPLE_NAME;
        ok_mask |= UACPI_OP_PROPERTY_SIMPLE_NAME;
        break;

    // Target := SuperName | NullName
    case UACPI_PARSE_OP_TARGET:
        expected_type_str = SPEC_TARGET;
        ok_mask |= UACPI_OP_PROPERTY_TARGET | UACPI_OP_PROPERTY_SUPERNAME;
        break;

    // SuperName := SimpleName | DebugObj | ReferenceTypeOpcode
    case UACPI_PARSE_OP_SUPERNAME:
    case UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED:
        expected_type_str = SPEC_SUPER_NAME;
        ok_mask |= UACPI_OP_PROPERTY_SUPERNAME;
        break;

    // TermArg := ExpressionOpcode | DataObject | ArgObj | LocalObj
    case UACPI_PARSE_OP_TERM_ARG:
    case UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL:
    case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT:
    case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED:
    case UACPI_PARSE_OP_OPERAND:
    case UACPI_PARSE_OP_STRING:
    case UACPI_PARSE_OP_COMPUTATIONAL_DATA:
        expected_type_str = SPEC_TERM_ARG;
        ok_mask |= UACPI_OP_PROPERTY_TERM_ARG;
        break;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }

    if (!(props & ok_mask)) {
        EXEC_OP_ERR_2("invalid argument: '%s', expected a %s",
                      cur_op_ctx->op->name, expected_type_str);
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status typecheck_obj(
    const struct op_context *op_ctx,
    const uacpi_object *obj,
    enum uacpi_object_type expected_type,
    const uacpi_char *spec_desc
)
{
    if (uacpi_likely(obj->type == expected_type))
        return UACPI_STATUS_OK;

    EXEC_OP_ERR_2("invalid argument type: %s, expected a %s",
                  uacpi_object_type_to_string(obj->type), spec_desc);
    return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
}

static uacpi_status typecheck_operand(
    const struct op_context *op_ctx,
    const uacpi_object *obj
)
{
    return typecheck_obj(op_ctx, obj, UACPI_OBJECT_INTEGER, SPEC_OPERAND);
}

static uacpi_status typecheck_string(
    const struct op_context *op_ctx,
    const uacpi_object *obj
)
{
    return typecheck_obj(op_ctx, obj, UACPI_OBJECT_STRING, SPEC_STRING);
}

static uacpi_status typecheck_computational_data(
    const struct op_context *op_ctx,
    const uacpi_object *obj
)
{
    switch (obj->type) {
    case UACPI_OBJECT_STRING:
    case UACPI_OBJECT_BUFFER:
    case UACPI_OBJECT_INTEGER:
        return UACPI_STATUS_OK;
    default:
        EXEC_OP_ERR_2(
            "invalid argument type: %s, expected a %s",
            uacpi_object_type_to_string(obj->type),
            SPEC_COMPUTATIONAL_DATA
        );
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }
}

static void emit_op_skip_warn(const struct op_context *op_ctx)
{
    EXEC_OP_WARN("skipping due to previous errors");
}

static void trace_named_object_lookup_or_creation_failure(
    struct call_frame *frame, uacpi_size offset, enum uacpi_parse_op op,
    uacpi_status ret, enum uacpi_log_level level
)
{
    static const uacpi_char *oom_prefix = "<...>";
    static const uacpi_char *empty_string = "";
    static const uacpi_char *unknown_path = "<unknown-path>";
    static const uacpi_char *invalid_path = "<invalid-path>";

    uacpi_status conv_ret;
    const uacpi_char *action;
    const uacpi_char *requested_path_to_print;
    const uacpi_char *middle_part = UACPI_NULL;
    const uacpi_char *prefix_path = UACPI_NULL;
    uacpi_char *requested_path = UACPI_NULL;
    uacpi_size length;
    uacpi_bool is_create;

    is_create = op == UACPI_PARSE_OP_CREATE_NAMESTRING ||
                op == UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD;

    if (is_create)
        action = "create";
    else
        action = "lookup";

    conv_ret = name_string_to_path(
        frame, offset, &requested_path, &length
    );
    if (uacpi_unlikely_error(conv_ret)) {
        if (conv_ret == UACPI_STATUS_OUT_OF_MEMORY)
            requested_path_to_print = unknown_path;
        else
            requested_path_to_print = invalid_path;
    } else {
        requested_path_to_print = requested_path;
    }

    if (requested_path && requested_path[0] != '\\') {
        prefix_path = uacpi_namespace_node_generate_absolute_path(
            frame->cur_scope
        );
        if (uacpi_unlikely(prefix_path == UACPI_NULL))
            prefix_path = oom_prefix;

        if (prefix_path[1] != '\0')
            middle_part = ".";
    } else {
        prefix_path = empty_string;
    }

    if (middle_part == UACPI_NULL)
        middle_part = empty_string;

    if (length == 5 && !is_create) {
        uacpi_log_lvl(
            level,
            "unable to %s named object '%s' within (or above) "
            "scope '%s': %s\n", action, requested_path_to_print,
            prefix_path, uacpi_status_to_string(ret)
        );
    } else {
        uacpi_log_lvl(
            level,
            "unable to %s named object '%s%s%s': %s\n",
            action, prefix_path, middle_part,
            requested_path_to_print, uacpi_status_to_string(ret)
        );
    }

    uacpi_free(requested_path, length);
    if (prefix_path != oom_prefix && prefix_path != empty_string)
        uacpi_free_dynamic_string(prefix_path);
}

static uacpi_status uninstalled_op_handler(struct execution_context *ctx)
{
    struct op_context *op_ctx = ctx->cur_op_ctx;

    EXEC_OP_ERR("no dedicated handler installed");
    return UACPI_STATUS_UNIMPLEMENTED;
}

enum op_handler {
    OP_HANDLER_UNINSTALLED = 0,
    OP_HANDLER_LOCAL,
    OP_HANDLER_ARG,
    OP_HANDLER_STRING,
    OP_HANDLER_BINARY_MATH,
    OP_HANDLER_CONTROL_FLOW,
    OP_HANDLER_CODE_BLOCK,
    OP_HANDLER_RETURN,
    OP_HANDLER_CREATE_METHOD,
    OP_HANDLER_COPY_OBJECT_OR_STORE,
    OP_HANDLER_INC_DEC,
    OP_HANDLER_REF_OR_DEREF_OF,
    OP_HANDLER_LOGICAL_NOT,
    OP_HANDLER_BINARY_LOGIC,
    OP_HANDLER_NAMED_OBJECT,
    OP_HANDLER_BUFFER,
    OP_HANDLER_PACKAGE,
    OP_HANDLER_CREATE_NAMED,
    OP_HANDLER_CREATE_BUFFER_FIELD,
    OP_HANDLER_READ_FIELD,
    OP_HANDLER_ALIAS,
    OP_HANDLER_CONCATENATE,
    OP_HANDLER_CONCATENATE_RES,
    OP_HANDLER_SIZEOF,
    OP_HANDLER_UNARY_MATH,
    OP_HANDLER_INDEX,
    OP_HANDLER_OBJECT_TYPE,
    OP_HANDLER_CREATE_OP_REGION,
    OP_HANDLER_CREATE_DATA_REGION,
    OP_HANDLER_CREATE_FIELD,
    OP_HANDLER_TO,
    OP_HANDLER_TO_STRING,
    OP_HANDLER_TIMER,
    OP_HANDLER_MID,
    OP_HANDLER_MATCH,
    OP_HANDLER_CREATE_MUTEX_OR_EVENT,
    OP_HANDLER_BCD,
    OP_HANDLER_UNLOAD,
    OP_HANDLER_LOAD_TABLE,
    OP_HANDLER_LOAD,
    OP_HANDLER_STALL_OR_SLEEP,
    OP_HANDLER_EVENT_CTL,
    OP_HANDLER_MUTEX_CTL,
    OP_HANDLER_NOTIFY,
    OP_HANDLER_FIRMWARE_REQUEST,
};

static uacpi_status (*op_handlers[])(struct execution_context *ctx) = {
    /*
     * All OPs that don't have a handler dispatch to here if
     * UACPI_PARSE_OP_INVOKE_HANDLER is reached.
     */
    [OP_HANDLER_UNINSTALLED] = uninstalled_op_handler,
    [OP_HANDLER_LOCAL] = handle_local,
    [OP_HANDLER_ARG] = handle_arg,
    [OP_HANDLER_NAMED_OBJECT] = handle_named_object,
    [OP_HANDLER_STRING] = handle_string,
    [OP_HANDLER_BINARY_MATH] = handle_binary_math,
    [OP_HANDLER_CONTROL_FLOW] = handle_control_flow,
    [OP_HANDLER_CODE_BLOCK] = handle_code_block,
    [OP_HANDLER_RETURN] = handle_return,
    [OP_HANDLER_CREATE_METHOD] = handle_create_method,
    [OP_HANDLER_CREATE_MUTEX_OR_EVENT] = handle_create_mutex_or_event,
    [OP_HANDLER_COPY_OBJECT_OR_STORE] = handle_copy_object_or_store,
    [OP_HANDLER_INC_DEC] = handle_inc_dec,
    [OP_HANDLER_REF_OR_DEREF_OF] = handle_ref_or_deref_of,
    [OP_HANDLER_LOGICAL_NOT] = handle_logical_not,
    [OP_HANDLER_BINARY_LOGIC] = handle_binary_logic,
    [OP_HANDLER_BUFFER] = handle_buffer,
    [OP_HANDLER_PACKAGE] = handle_package,
    [OP_HANDLER_CREATE_NAMED] = handle_create_named,
    [OP_HANDLER_CREATE_BUFFER_FIELD] = handle_create_buffer_field,
    [OP_HANDLER_READ_FIELD] = handle_field_read,
    [OP_HANDLER_TO] = handle_to,
    [OP_HANDLER_ALIAS] = handle_create_alias,
    [OP_HANDLER_CONCATENATE] = handle_concatenate,
    [OP_HANDLER_CONCATENATE_RES] = handle_concatenate_res,
    [OP_HANDLER_SIZEOF] = handle_sizeof,
    [OP_HANDLER_UNARY_MATH] = handle_unary_math,
    [OP_HANDLER_INDEX] = handle_index,
    [OP_HANDLER_OBJECT_TYPE] = handle_object_type,
    [OP_HANDLER_CREATE_OP_REGION] = handle_create_op_region,
    [OP_HANDLER_CREATE_DATA_REGION] = handle_create_data_region,
    [OP_HANDLER_CREATE_FIELD] = handle_create_field,
    [OP_HANDLER_TIMER] = handle_timer,
    [OP_HANDLER_TO_STRING] = handle_to_string,
    [OP_HANDLER_MID] = handle_mid,
    [OP_HANDLER_MATCH] = handle_match,
    [OP_HANDLER_BCD] = handle_bcd,
    [OP_HANDLER_UNLOAD] = handle_unload,
    [OP_HANDLER_LOAD_TABLE] = handle_load_table,
    [OP_HANDLER_LOAD] = handle_load,
    [OP_HANDLER_STALL_OR_SLEEP] = handle_stall_or_sleep,
    [OP_HANDLER_EVENT_CTL] = handle_event_ctl,
    [OP_HANDLER_MUTEX_CTL] = handle_mutex_ctl,
    [OP_HANDLER_NOTIFY] = handle_notify,
    [OP_HANDLER_FIRMWARE_REQUEST] = handle_firmware_request,
};

static uacpi_u8 handler_idx_of_op[0x100] = {
    [UACPI_AML_OP_Local0Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local1Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local2Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local3Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local4Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local5Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local6Op] = OP_HANDLER_LOCAL,
    [UACPI_AML_OP_Local7Op] = OP_HANDLER_LOCAL,

    [UACPI_AML_OP_Arg0Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg1Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg2Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg3Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg4Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg5Op] = OP_HANDLER_ARG,
    [UACPI_AML_OP_Arg6Op] = OP_HANDLER_ARG,

    [UACPI_AML_OP_StringPrefix] = OP_HANDLER_STRING,

    [UACPI_AML_OP_AddOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_SubtractOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_MultiplyOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_DivideOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_ShiftLeftOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_ShiftRightOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_AndOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_NandOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_OrOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_NorOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_XorOp] = OP_HANDLER_BINARY_MATH,
    [UACPI_AML_OP_ModOp] = OP_HANDLER_BINARY_MATH,

    [UACPI_AML_OP_IfOp] = OP_HANDLER_CODE_BLOCK,
    [UACPI_AML_OP_ElseOp] = OP_HANDLER_CODE_BLOCK,
    [UACPI_AML_OP_WhileOp] = OP_HANDLER_CODE_BLOCK,
    [UACPI_AML_OP_ScopeOp] = OP_HANDLER_CODE_BLOCK,

    [UACPI_AML_OP_ContinueOp] = OP_HANDLER_CONTROL_FLOW,
    [UACPI_AML_OP_BreakOp] = OP_HANDLER_CONTROL_FLOW,

    [UACPI_AML_OP_ReturnOp] = OP_HANDLER_RETURN,

    [UACPI_AML_OP_MethodOp] = OP_HANDLER_CREATE_METHOD,

    [UACPI_AML_OP_StoreOp] = OP_HANDLER_COPY_OBJECT_OR_STORE,
    [UACPI_AML_OP_CopyObjectOp] = OP_HANDLER_COPY_OBJECT_OR_STORE,

    [UACPI_AML_OP_IncrementOp] = OP_HANDLER_INC_DEC,
    [UACPI_AML_OP_DecrementOp] = OP_HANDLER_INC_DEC,

    [UACPI_AML_OP_RefOfOp] = OP_HANDLER_REF_OR_DEREF_OF,
    [UACPI_AML_OP_DerefOfOp] = OP_HANDLER_REF_OR_DEREF_OF,

    [UACPI_AML_OP_LnotOp] = OP_HANDLER_LOGICAL_NOT,

    [UACPI_AML_OP_LEqualOp] = OP_HANDLER_BINARY_LOGIC,
    [UACPI_AML_OP_LandOp] = OP_HANDLER_BINARY_LOGIC,
    [UACPI_AML_OP_LorOp] = OP_HANDLER_BINARY_LOGIC,
    [UACPI_AML_OP_LGreaterOp] = OP_HANDLER_BINARY_LOGIC,
    [UACPI_AML_OP_LLessOp] = OP_HANDLER_BINARY_LOGIC,

    [UACPI_AML_OP_InternalOpNamedObject] = OP_HANDLER_NAMED_OBJECT,

    [UACPI_AML_OP_BufferOp] = OP_HANDLER_BUFFER,

    [UACPI_AML_OP_PackageOp] = OP_HANDLER_PACKAGE,
    [UACPI_AML_OP_VarPackageOp] = OP_HANDLER_PACKAGE,

    [UACPI_AML_OP_NameOp] = OP_HANDLER_CREATE_NAMED,

    [UACPI_AML_OP_CreateBitFieldOp] = OP_HANDLER_CREATE_BUFFER_FIELD,
    [UACPI_AML_OP_CreateByteFieldOp] = OP_HANDLER_CREATE_BUFFER_FIELD,
    [UACPI_AML_OP_CreateWordFieldOp] = OP_HANDLER_CREATE_BUFFER_FIELD,
    [UACPI_AML_OP_CreateDWordFieldOp] = OP_HANDLER_CREATE_BUFFER_FIELD,
    [UACPI_AML_OP_CreateQWordFieldOp] = OP_HANDLER_CREATE_BUFFER_FIELD,

    [UACPI_AML_OP_InternalOpReadFieldAsBuffer] = OP_HANDLER_READ_FIELD,
    [UACPI_AML_OP_InternalOpReadFieldAsInteger] = OP_HANDLER_READ_FIELD,

    [UACPI_AML_OP_ToIntegerOp] = OP_HANDLER_TO,
    [UACPI_AML_OP_ToBufferOp] = OP_HANDLER_TO,
    [UACPI_AML_OP_ToDecimalStringOp] = OP_HANDLER_TO,
    [UACPI_AML_OP_ToHexStringOp] = OP_HANDLER_TO,
    [UACPI_AML_OP_ToStringOp] = OP_HANDLER_TO_STRING,

    [UACPI_AML_OP_AliasOp] = OP_HANDLER_ALIAS,

    [UACPI_AML_OP_ConcatOp] = OP_HANDLER_CONCATENATE,
    [UACPI_AML_OP_ConcatResOp] = OP_HANDLER_CONCATENATE_RES,

    [UACPI_AML_OP_SizeOfOp] = OP_HANDLER_SIZEOF,

    [UACPI_AML_OP_NotOp] = OP_HANDLER_UNARY_MATH,
    [UACPI_AML_OP_FindSetLeftBitOp] = OP_HANDLER_UNARY_MATH,
    [UACPI_AML_OP_FindSetRightBitOp] = OP_HANDLER_UNARY_MATH,

    [UACPI_AML_OP_IndexOp] = OP_HANDLER_INDEX,

    [UACPI_AML_OP_ObjectTypeOp] = OP_HANDLER_OBJECT_TYPE,

    [UACPI_AML_OP_MidOp] = OP_HANDLER_MID,

    [UACPI_AML_OP_MatchOp] = OP_HANDLER_MATCH,

    [UACPI_AML_OP_NotifyOp] = OP_HANDLER_NOTIFY,

    [UACPI_AML_OP_BreakPointOp] = OP_HANDLER_FIRMWARE_REQUEST,
};

#define EXT_OP_IDX(op) (op & 0xFF)

static uacpi_u8 handler_idx_of_ext_op[0x100] = {
    [EXT_OP_IDX(UACPI_AML_OP_CreateFieldOp)] = OP_HANDLER_CREATE_BUFFER_FIELD,
    [EXT_OP_IDX(UACPI_AML_OP_CondRefOfOp)] = OP_HANDLER_REF_OR_DEREF_OF,
    [EXT_OP_IDX(UACPI_AML_OP_OpRegionOp)] = OP_HANDLER_CREATE_OP_REGION,
    [EXT_OP_IDX(UACPI_AML_OP_DeviceOp)] = OP_HANDLER_CODE_BLOCK,
    [EXT_OP_IDX(UACPI_AML_OP_ProcessorOp)] = OP_HANDLER_CODE_BLOCK,
    [EXT_OP_IDX(UACPI_AML_OP_PowerResOp)] = OP_HANDLER_CODE_BLOCK,
    [EXT_OP_IDX(UACPI_AML_OP_ThermalZoneOp)] = OP_HANDLER_CODE_BLOCK,
    [EXT_OP_IDX(UACPI_AML_OP_TimerOp)] = OP_HANDLER_TIMER,
    [EXT_OP_IDX(UACPI_AML_OP_MutexOp)] = OP_HANDLER_CREATE_MUTEX_OR_EVENT,
    [EXT_OP_IDX(UACPI_AML_OP_EventOp)] = OP_HANDLER_CREATE_MUTEX_OR_EVENT,

    [EXT_OP_IDX(UACPI_AML_OP_FieldOp)] = OP_HANDLER_CREATE_FIELD,
    [EXT_OP_IDX(UACPI_AML_OP_IndexFieldOp)] = OP_HANDLER_CREATE_FIELD,
    [EXT_OP_IDX(UACPI_AML_OP_BankFieldOp)] = OP_HANDLER_CREATE_FIELD,

    [EXT_OP_IDX(UACPI_AML_OP_FromBCDOp)] = OP_HANDLER_BCD,
    [EXT_OP_IDX(UACPI_AML_OP_ToBCDOp)] = OP_HANDLER_BCD,

    [EXT_OP_IDX(UACPI_AML_OP_DataRegionOp)] = OP_HANDLER_CREATE_DATA_REGION,

    [EXT_OP_IDX(UACPI_AML_OP_LoadTableOp)] = OP_HANDLER_LOAD_TABLE,
    [EXT_OP_IDX(UACPI_AML_OP_LoadOp)] = OP_HANDLER_LOAD,
    [EXT_OP_IDX(UACPI_AML_OP_UnloadOp)] = OP_HANDLER_UNLOAD,

    [EXT_OP_IDX(UACPI_AML_OP_StallOp)] = OP_HANDLER_STALL_OR_SLEEP,
    [EXT_OP_IDX(UACPI_AML_OP_SleepOp)] = OP_HANDLER_STALL_OR_SLEEP,

    [EXT_OP_IDX(UACPI_AML_OP_SignalOp)] = OP_HANDLER_EVENT_CTL,
    [EXT_OP_IDX(UACPI_AML_OP_ResetOp)] = OP_HANDLER_EVENT_CTL,
    [EXT_OP_IDX(UACPI_AML_OP_WaitOp)] = OP_HANDLER_EVENT_CTL,

    [EXT_OP_IDX(UACPI_AML_OP_AcquireOp)] = OP_HANDLER_MUTEX_CTL,
    [EXT_OP_IDX(UACPI_AML_OP_ReleaseOp)] = OP_HANDLER_MUTEX_CTL,

    [EXT_OP_IDX(UACPI_AML_OP_FatalOp)] = OP_HANDLER_FIRMWARE_REQUEST,
};

enum method_call_type {
    METHOD_CALL_NATIVE,
    METHOD_CALL_AML,
    METHOD_CALL_TABLE_LOAD,
};

static uacpi_status prepare_method_call(
    struct execution_context *ctx, uacpi_namespace_node *node,
    uacpi_control_method *method, enum method_call_type type,
    const uacpi_object_array *args
)
{
    uacpi_status ret;
    struct call_frame *frame;

    if (uacpi_unlikely(call_frame_array_size(&ctx->call_stack) >=
                       g_uacpi_rt_ctx.max_call_stack_depth))
        return UACPI_STATUS_AML_CALL_STACK_DEPTH_LIMIT;

    ret = push_new_frame(ctx, &frame);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = enter_method(ctx, frame, method);
    if (uacpi_unlikely_error(ret))
        goto method_dispatch_error;

    if (type == METHOD_CALL_NATIVE) {
        uacpi_u8 arg_count;

        arg_count = args ? args->count : 0;
        if (uacpi_unlikely(arg_count != method->args)) {
            uacpi_error(
                "invalid number of arguments %zu to call %.4s, expected %d\n",
                args ? args->count : 0, node->name.text, method->args
            );

            ret = UACPI_STATUS_INVALID_ARGUMENT;
            goto method_dispatch_error;
        }

        if (args != UACPI_NULL) {
            uacpi_u8 i;

            for (i = 0; i < method->args; ++i) {
                frame->args[i] = args->objects[i];
                uacpi_object_ref(args->objects[i]);
            }
        }
    } else if (type == METHOD_CALL_AML) {
        ret = frame_push_args(frame, ctx->cur_op_ctx);
        if (uacpi_unlikely_error(ret))
            goto method_dispatch_error;
    }

    ret = frame_setup_base_scope(frame, node, method);
    if (uacpi_unlikely_error(ret))
        goto method_dispatch_error;

    ctx->cur_frame = frame;
    ctx->cur_op_ctx = UACPI_NULL;
    ctx->prev_op_ctx = UACPI_NULL;
    ctx->cur_block = code_block_array_last(&ctx->cur_frame->code_blocks);

    if (method->native_call) {
        uacpi_object *retval;

        ret = method_get_ret_object(ctx, &retval);
        if (uacpi_unlikely_error(ret))
            goto method_dispatch_error;

        return method->handler(ctx, retval);
    }

    return UACPI_STATUS_OK;

method_dispatch_error:
    call_frame_clear(frame);
    call_frame_array_pop(&ctx->call_stack);
    return ret;
}

static void apply_tracked_pkg(
    struct call_frame *frame, struct op_context *op_ctx
)
{
    struct item *item;

    if (op_ctx->tracked_pkg_idx == 0)
        return;

    item = item_array_at(&op_ctx->items, op_ctx->tracked_pkg_idx - 1);
    frame->code_offset = item->pkg.end;
}

static uacpi_status exec_op(struct execution_context *ctx)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct call_frame *frame = ctx->cur_frame;
    struct op_context *op_ctx;
    struct item *item = UACPI_NULL;
    enum uacpi_parse_op prev_op = 0, op;

    /*
     * Allocate a new op context if previous is preempted (looking for a
     * dynamic argument), or doesn't exist at all.
     */
    if (!ctx_has_non_preempted_op(ctx)) {
        ret = push_op(ctx);
        if (uacpi_unlikely_error(ret))
            return ret;
    } else {
        trace_op(ctx->cur_op_ctx->op, OP_TRACE_ACTION_RESUME);
    }

    if (ctx->prev_op_ctx)
        prev_op = *op_decode_cursor(ctx->prev_op_ctx);

    for (;;) {
        if (uacpi_unlikely_error(ret))
            return ret;

        op_ctx = ctx->cur_op_ctx;
        frame = ctx->cur_frame;

        if (op_ctx->pc == 0 && ctx->prev_op_ctx) {
            /*
             * Type check the current arg type against what is expected by the
             * preempted op. This check is able to catch most type violations
             * with the only exception being Operand as we only know whether
             * that evaluates to an integer after the fact.
             */
            ret = op_typecheck(ctx->prev_op_ctx, ctx->cur_op_ctx);
            if (uacpi_unlikely_error(ret))
                return ret;
        }

        op = op_decode_byte(op_ctx);
        trace_pop(op);

        if (parse_op_generates_item[op] != ITEM_NONE) {
            item = item_array_alloc(&op_ctx->items);
            if (uacpi_unlikely(item == UACPI_NULL))
                return UACPI_STATUS_OUT_OF_MEMORY;

            item->type = parse_op_generates_item[op];
            if (item->type == ITEM_OBJECT) {
                enum uacpi_object_type type = UACPI_OBJECT_UNINITIALIZED;

                if (op == UACPI_PARSE_OP_OBJECT_ALLOC_TYPED)
                    type = op_decode_byte(op_ctx);

                item->obj = uacpi_create_object(type);
                if (uacpi_unlikely(item->obj == UACPI_NULL))
                    return UACPI_STATUS_OUT_OF_MEMORY;
            } else {
                uacpi_memzero(&item->immediate, sizeof(item->immediate));
            }
        } else if (item == UACPI_NULL) {
            item = item_array_last(&op_ctx->items);
        }

        switch (op) {
        case UACPI_PARSE_OP_END:
        case UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL: {
            trace_op(ctx->cur_op_ctx->op, OP_TRACE_ACTION_END);

            if (op == UACPI_PARSE_OP_SKIP_WITH_WARN_IF_NULL) {
                uacpi_u8 idx;

                idx = op_decode_byte(op_ctx);
                if (item_array_at(&op_ctx->items, idx)->handle != UACPI_NULL)
                    break;

                emit_op_skip_warn(op_ctx);
            }

            apply_tracked_pkg(frame, op_ctx);

            pop_op(ctx);
            if (ctx->cur_op_ctx) {
                ctx->cur_op_ctx->preempted = UACPI_FALSE;
                ctx->cur_op_ctx->pc++;
            }

            return UACPI_STATUS_OK;
        }

        case UACPI_PARSE_OP_EMIT_SKIP_WARN:
            emit_op_skip_warn(op_ctx);
            break;

        case UACPI_PARSE_OP_SIMPLE_NAME:
        case UACPI_PARSE_OP_SUPERNAME:
        case UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED:
        case UACPI_PARSE_OP_TERM_ARG:
        case UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL:
        case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT:
        case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED:
        case UACPI_PARSE_OP_OPERAND:
        case UACPI_PARSE_OP_STRING:
        case UACPI_PARSE_OP_COMPUTATIONAL_DATA:
        case UACPI_PARSE_OP_TARGET:
            /*
             * Preempt this op parsing for now as we wait for the dynamic arg
             * to be parsed.
             */
            op_ctx->preempted = UACPI_TRUE;
            op_ctx->pc--;
            return UACPI_STATUS_OK;

        case UACPI_PARSE_OP_TRACKED_PKGLEN:
            op_ctx->tracked_pkg_idx = item_array_size(&op_ctx->items);
            UACPI_FALLTHROUGH;
        case UACPI_PARSE_OP_PKGLEN:
            ret = parse_package_length(frame, &item->pkg);
            break;

        case UACPI_PARSE_OP_LOAD_INLINE_IMM:
        case UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT: {
            void *dst;
            uacpi_u8 src_width;

            if (op == UACPI_PARSE_OP_LOAD_INLINE_IMM_AS_OBJECT) {
                item->obj->type = UACPI_OBJECT_INTEGER;
                dst = &item->obj->integer;
                src_width = 8;
            } else {
                dst = &item->immediate;
                src_width = op_decode_byte(op_ctx);
            }

            uacpi_memcpy_zerout(
                dst, op_decode_cursor(op_ctx),
                sizeof(uacpi_u64), src_width
            );
            op_ctx->pc += src_width;
            break;
        }

        case UACPI_PARSE_OP_LOAD_ZERO_IMM:
            break;

        case UACPI_PARSE_OP_LOAD_IMM:
        case UACPI_PARSE_OP_LOAD_IMM_AS_OBJECT: {
            uacpi_u8 width;
            void *dst;

            width = op_decode_byte(op_ctx);
            if (uacpi_unlikely(call_frame_code_bytes_left(frame) < width))
                return UACPI_STATUS_AML_BAD_ENCODING;

            if (op == UACPI_PARSE_OP_LOAD_IMM_AS_OBJECT) {
                item->obj->type = UACPI_OBJECT_INTEGER;
                item->obj->integer = 0;
                dst = &item->obj->integer;
            } else {
                dst = item->immediate_bytes;
            }

            uacpi_memcpy(dst, call_frame_cursor(frame), width);
            frame->code_offset += width;
            break;
        }

        case UACPI_PARSE_OP_LOAD_FALSE_OBJECT:
        case UACPI_PARSE_OP_LOAD_TRUE_OBJECT: {
            uacpi_object *obj = item->obj;
            obj->type = UACPI_OBJECT_INTEGER;
            obj->integer = op == UACPI_PARSE_OP_LOAD_FALSE_OBJECT ? 0 : ones();
            break;
        }

        case UACPI_PARSE_OP_RECORD_AML_PC:
            item->immediate = frame->code_offset;
            break;

        case UACPI_PARSE_OP_TRUNCATE_NUMBER:
            truncate_number_if_needed(item->obj);
            break;

        case UACPI_PARSE_OP_TYPECHECK: {
            enum uacpi_object_type expected_type;

            expected_type = op_decode_byte(op_ctx);

            if (uacpi_unlikely(item->obj->type != expected_type)) {
                EXEC_OP_ERR_2("bad object type: expected %s, got %s!",
                              uacpi_object_type_to_string(expected_type),
                              uacpi_object_type_to_string(item->obj->type));
                ret = UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
            }

            break;
        }

        case UACPI_PARSE_OP_BAD_OPCODE:
        case UACPI_PARSE_OP_UNREACHABLE:
            EXEC_OP_ERR("invalid/unexpected opcode");
            ret = UACPI_STATUS_AML_INVALID_OPCODE;
            break;

        case UACPI_PARSE_OP_AML_PC_DECREMENT:
            frame->code_offset--;
            break;

        case UACPI_PARSE_OP_IMM_DECREMENT:
            item_array_at(&op_ctx->items, op_decode_byte(op_ctx))->immediate--;
            break;

        case UACPI_PARSE_OP_ITEM_POP:
            pop_item(op_ctx);
            item = item_array_last(&op_ctx->items);
            break;

        case UACPI_PARSE_OP_IF_HAS_DATA: {
            uacpi_size pkg_idx = op_ctx->tracked_pkg_idx - 1;
            struct package_length *pkg;
            uacpi_u8 bytes_skip;

            bytes_skip = op_decode_byte(op_ctx);
            pkg = &item_array_at(&op_ctx->items, pkg_idx)->pkg;

            if (frame->code_offset >= pkg->end)
                op_ctx->pc += bytes_skip;

            break;
        }

        case UACPI_PARSE_OP_IF_NOT_NULL:
        case UACPI_PARSE_OP_IF_NULL:
        case UACPI_PARSE_OP_IF_LAST_NULL:
        case UACPI_PARSE_OP_IF_LAST_NOT_NULL: {
            uacpi_u8 idx, bytes_skip;
            uacpi_bool is_null, skip_if_null;

            if (op == UACPI_PARSE_OP_IF_LAST_NULL ||
                op == UACPI_PARSE_OP_IF_LAST_NOT_NULL) {
                is_null = item->handle == UACPI_NULL;
            } else {
                idx = op_decode_byte(op_ctx);
                is_null = item_array_at(&op_ctx->items, idx)->handle == UACPI_NULL;
            }

            bytes_skip = op_decode_byte(op_ctx);
            skip_if_null = op == UACPI_PARSE_OP_IF_NOT_NULL ||
                           op == UACPI_PARSE_OP_IF_LAST_NOT_NULL;

            if (is_null == skip_if_null)
                op_ctx->pc += bytes_skip;

            break;
        }

        case UACPI_PARSE_OP_IF_LAST_EQUALS: {
            uacpi_u8 value, bytes_skip;

            value = op_decode_byte(op_ctx);
            bytes_skip = op_decode_byte(op_ctx);

            if (item->immediate != value)
                op_ctx->pc += bytes_skip;

            break;
        }

        case UACPI_PARSE_OP_IF_LAST_FALSE:
        case UACPI_PARSE_OP_IF_LAST_TRUE: {
            uacpi_u8 bytes_skip;
            uacpi_bool is_false, skip_if_false;

            bytes_skip = op_decode_byte(op_ctx);
            is_false = item->obj->integer == 0;
            skip_if_false = op == UACPI_PARSE_OP_IF_LAST_TRUE;

            if (is_false == skip_if_false)
                op_ctx->pc += bytes_skip;

            break;
        }

        case UACPI_PARSE_OP_JMP: {
            op_ctx->pc = op_decode_byte(op_ctx);
            break;
        }

        case UACPI_PARSE_OP_CREATE_NAMESTRING:
        case UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD:
        case UACPI_PARSE_OP_EXISTING_NAMESTRING:
        case UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL:
        case UACPI_PARSE_OP_EXISTING_NAMESTRING_OR_NULL_IF_LOAD: {
            uacpi_size offset = frame->code_offset;
            enum resolve_behavior behavior;

            if (op == UACPI_PARSE_OP_CREATE_NAMESTRING ||
                op == UACPI_PARSE_OP_CREATE_NAMESTRING_OR_NULL_IF_LOAD)
                behavior = RESOLVE_CREATE_LAST_NAMESEG_FAIL_IF_EXISTS;
            else
                behavior = RESOLVE_FAIL_IF_DOESNT_EXIST;

            ret = resolve_name_string(frame, behavior, &item->node);

            if (ret == UACPI_STATUS_NOT_FOUND) {
                uacpi_bool is_ok;

                if (prev_op) {
                    is_ok = op_allows_unresolved(prev_op);
                    is_ok &= op_allows_unresolved(op);
                } else {
                    // This is the only standalone op where we allow unresolved
                    is_ok = op_ctx->op->code == UACPI_AML_OP_ExternalOp;
                }

                if (is_ok)
                    ret = UACPI_STATUS_OK;
            }

            if (uacpi_unlikely_error(ret)) {
                enum uacpi_log_level lvl = UACPI_LOG_ERROR;
                uacpi_status trace_ret = ret;
                uacpi_bool abort_whileif = UACPI_FALSE;

                if (frame->method->named_objects_persist &&
                    (ret == UACPI_STATUS_AML_OBJECT_ALREADY_EXISTS ||
                     ret == UACPI_STATUS_NOT_FOUND)) {
                    struct op_context *first_ctx;

                    first_ctx = op_context_array_at(&frame->pending_ops, 0);
                    abort_whileif = first_ctx->op->code == UACPI_AML_OP_WhileOp ||
                                    first_ctx->op->code == UACPI_AML_OP_IfOp;

                    if (op_allows_unresolved_if_load(op) || abort_whileif) {
                        lvl = UACPI_LOG_WARN;
                        ret = UACPI_STATUS_OK;
                    }
                }

                trace_named_object_lookup_or_creation_failure(
                    frame, offset, op, trace_ret, lvl
                );

                if (abort_whileif) {
                    while (op_context_array_size(&frame->pending_ops) != 1)
                        pop_op(ctx);

                    op_ctx = op_context_array_at(&frame->pending_ops, 0);
                    op_ctx->pc++;
                    op_ctx->preempted = UACPI_FALSE;
                    break;
                }

                if (ret == UACPI_STATUS_NOT_FOUND)
                    ret = UACPI_STATUS_AML_UNDEFINED_REFERENCE;
            }

            if (behavior == RESOLVE_CREATE_LAST_NAMESEG_FAIL_IF_EXISTS &&
                !frame->method->named_objects_persist)
                item->node->flags |= UACPI_NAMESPACE_NODE_FLAG_TEMPORARY;

            break;
        }

        case UACPI_PARSE_OP_INVOKE_HANDLER: {
            uacpi_aml_op code = op_ctx->op->code;
            uacpi_u8 idx;

            if (code <= 0xFF)
                idx = handler_idx_of_op[code];
            else
                idx = handler_idx_of_ext_op[EXT_OP_IDX(code)];

            ret = op_handlers[idx](ctx);
            break;
        }

        case UACPI_PARSE_OP_INSTALL_NAMESPACE_NODE:
            item = item_array_at(&op_ctx->items, op_decode_byte(op_ctx));
            ret = do_install_node_item(frame, item);
            break;

        case UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV:
        case UACPI_PARSE_OP_OBJECT_COPY_TO_PREV: {
            uacpi_object *src;
            struct item *dst;

            if (!ctx->prev_op_ctx)
                break;

            switch (prev_op) {
            case UACPI_PARSE_OP_TERM_ARG_UNWRAP_INTERNAL:
            case UACPI_PARSE_OP_COMPUTATIONAL_DATA:
            case UACPI_PARSE_OP_OPERAND:
            case UACPI_PARSE_OP_STRING:
                src = uacpi_unwrap_internal_reference(item->obj);

                if (prev_op == UACPI_PARSE_OP_OPERAND)
                    ret = typecheck_operand(ctx->prev_op_ctx, src);
                else if (prev_op == UACPI_PARSE_OP_STRING)
                    ret = typecheck_string(ctx->prev_op_ctx, src);
                else if (prev_op == UACPI_PARSE_OP_COMPUTATIONAL_DATA)
                    ret = typecheck_computational_data(ctx->prev_op_ctx, src);

                break;
            case UACPI_PARSE_OP_SUPERNAME:
            case UACPI_PARSE_OP_SUPERNAME_OR_UNRESOLVED:
                src = item->obj;
                break;

            case UACPI_PARSE_OP_SIMPLE_NAME:
            case UACPI_PARSE_OP_TERM_ARG:
            case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT:
            case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED:
            case UACPI_PARSE_OP_TARGET:
                src = item->obj;
                break;

            default:
                EXEC_OP_ERR_1("don't know how to copy/transfer object to %d",
                              prev_op);
                ret = UACPI_STATUS_INVALID_ARGUMENT;
                break;
            }

            if (uacpi_likely_success(ret)) {
                dst = item_array_last(&ctx->prev_op_ctx->items);
                dst->type = ITEM_OBJECT;

                if (op == UACPI_PARSE_OP_OBJECT_TRANSFER_TO_PREV) {
                    dst->obj = src;
                    uacpi_object_ref(dst->obj);
                } else {
                    dst->obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
                    if (uacpi_unlikely(dst->obj == UACPI_NULL)) {
                        ret = UACPI_STATUS_OUT_OF_MEMORY;
                        break;
                    }

                    ret = uacpi_object_assign(dst->obj, src,
                                              UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
                }
            }
            break;
        }

        case UACPI_PARSE_OP_STORE_TO_TARGET:
        case UACPI_PARSE_OP_STORE_TO_TARGET_INDIRECT: {
            uacpi_object *dst, *src;

            dst = item_array_at(&op_ctx->items, op_decode_byte(op_ctx))->obj;

            if (op == UACPI_PARSE_OP_STORE_TO_TARGET_INDIRECT) {
                src = item_array_at(&op_ctx->items,
                                    op_decode_byte(op_ctx))->obj;
            } else {
                src = item->obj;
            }

            ret = store_to_target(dst, src, UACPI_NULL);
            break;
        }

        // Nothing to do here, object is allocated automatically
        case UACPI_PARSE_OP_OBJECT_ALLOC:
        case UACPI_PARSE_OP_OBJECT_ALLOC_TYPED:
        case UACPI_PARSE_OP_EMPTY_OBJECT_ALLOC:
            break;

        case UACPI_PARSE_OP_OBJECT_CONVERT_TO_SHALLOW_COPY:
        case UACPI_PARSE_OP_OBJECT_CONVERT_TO_DEEP_COPY: {
            uacpi_object *temp = item->obj;
            enum uacpi_assign_behavior behavior;

            item_array_pop(&op_ctx->items);
            item = item_array_last(&op_ctx->items);

            if (op == UACPI_PARSE_OP_OBJECT_CONVERT_TO_SHALLOW_COPY)
                behavior = UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY;
            else
                behavior = UACPI_ASSIGN_BEHAVIOR_DEEP_COPY;

            ret = uacpi_object_assign(temp, item->obj, behavior);
            if (uacpi_unlikely_error(ret))
                break;

            uacpi_object_unref(item->obj);
            item->obj = temp;
            break;
        }

        case UACPI_PARSE_OP_DISPATCH_METHOD_CALL: {
            struct uacpi_namespace_node *node;
            struct uacpi_control_method *method;

            node = item_array_at(&op_ctx->items, 0)->node;
            method = uacpi_namespace_node_get_object(node)->method;

            ret = prepare_method_call(
                ctx, node, method, METHOD_CALL_AML, UACPI_NULL
            );
            return ret;
        }

        case UACPI_PARSE_OP_DISPATCH_TABLE_LOAD: {
            struct uacpi_namespace_node *node;
            struct uacpi_control_method *method;

            node = item_array_at(&op_ctx->items, 0)->node;
            method = item_array_at(&op_ctx->items, 1)->obj->method;

            ret = prepare_method_call(
                ctx, node, method, METHOD_CALL_TABLE_LOAD, UACPI_NULL
            );
            return ret;
        }

        case UACPI_PARSE_OP_CONVERT_NAMESTRING: {
            uacpi_aml_op new_op = UACPI_AML_OP_InternalOpNamedObject;
            uacpi_object *obj;

            if (item->node == UACPI_NULL) {
                if (!op_allows_unresolved(prev_op))
                    ret = UACPI_STATUS_NOT_FOUND;
                break;
            }

            obj = uacpi_namespace_node_get_object(item->node);

            switch (obj->type) {
            case UACPI_OBJECT_METHOD: {
                uacpi_bool should_invoke;

                switch (prev_op) {
                case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT:
                case UACPI_PARSE_OP_TERM_ARG_OR_NAMED_OBJECT_OR_UNRESOLVED:
                    should_invoke = UACPI_FALSE;
                    break;
                default:
                    should_invoke = !op_wants_supername(prev_op);
                }

                if (!should_invoke)
                    break;

                new_op = UACPI_AML_OP_InternalOpMethodCall0Args;
                new_op += obj->method->args;
                break;
            }

            case UACPI_OBJECT_BUFFER_FIELD:
            case UACPI_OBJECT_FIELD_UNIT: {
                uacpi_object_type type;

                if (!op_wants_term_arg_or_operand(prev_op))
                    break;

                ret = field_get_read_type(obj, &type);
                if (uacpi_unlikely_error(ret)) {
                    const uacpi_char *field_path;

                    field_path = uacpi_namespace_node_generate_absolute_path(
                        item->node
                    );

                    uacpi_error(
                        "unable to perform a read from field %s: "
                        "parent opregion gone\n", field_path
                    );
                    uacpi_free_absolute_path(field_path);
                }

                switch (type) {
                case UACPI_OBJECT_BUFFER:
                    new_op = UACPI_AML_OP_InternalOpReadFieldAsBuffer;
                    break;
                case UACPI_OBJECT_INTEGER:
                    new_op = UACPI_AML_OP_InternalOpReadFieldAsInteger;
                    break;
                default:
                    ret = UACPI_STATUS_INVALID_ARGUMENT;
                    continue;
                }
                break;
            }
            default:
                break;
            }

            op_ctx->pc = 0;
            op_ctx->op = uacpi_get_op_spec(new_op);
            break;
        }

        case UACPI_PARSE_OP_SWITCH_TO_NEXT_IF_EQUALS: {
            uacpi_aml_op op, target_op;
            uacpi_u32 cur_offset;
            uacpi_u8 op_length;

            cur_offset = frame->code_offset;
            apply_tracked_pkg(frame, op_ctx);
            op_length = peek_next_op(frame, &op);

            target_op = op_decode_aml_op(op_ctx);
            if (op_length == 0 || op != target_op) {
                // Revert tracked package
                frame->code_offset = cur_offset;
                break;
            }

            frame->code_offset += op_length;
            op_ctx->switched_from = op_ctx->op->code;
            op_ctx->op = uacpi_get_op_spec(target_op);
            op_ctx->pc = 0;
            break;
        }

        case UACPI_PARSE_OP_IF_SWITCHED_FROM: {
            uacpi_aml_op target_op;
            uacpi_u8 skip_bytes;

            target_op = op_decode_aml_op(op_ctx);
            skip_bytes = op_decode_byte(op_ctx);

            if (op_ctx->switched_from != target_op)
                op_ctx->pc += skip_bytes;
            break;
        }

        default:
            EXEC_OP_ERR_1("unhandled parser op '%d'", op);
            ret = UACPI_STATUS_UNIMPLEMENTED;
            break;
        }
    }
}

static void ctx_reload_post_ret(struct execution_context *ctx)
{
    uacpi_control_method *method = ctx->cur_frame->method;

    if (method->is_serialized) {
        held_mutexes_array_remove_and_release(
            &ctx->held_mutexes, method->mutex, FORCE_RELEASE_YES
        );
        ctx->sync_level = ctx->cur_frame->prev_sync_level;
    }

    call_frame_clear(ctx->cur_frame);
    call_frame_array_pop(&ctx->call_stack);

    ctx->cur_frame = call_frame_array_last(&ctx->call_stack);
    refresh_ctx_pointers(ctx);
}

static void trace_method_abort(struct code_block *block, uacpi_size depth)
{
    static const uacpi_char *unknown_path = "<unknown>";
    uacpi_char oom_absolute_path[9] = "<?>.";

    const uacpi_char *absolute_path;

    if (block != UACPI_NULL && block->type == CODE_BLOCK_SCOPE) {
        absolute_path = uacpi_namespace_node_generate_absolute_path(block->node);
        if (uacpi_unlikely(absolute_path == UACPI_NULL))
            uacpi_memcpy(oom_absolute_path + 4, block->node->name.text, 4);
    } else {
        absolute_path = unknown_path;
    }

    uacpi_error("    #%zu in %s()\n", depth, absolute_path);

    if (absolute_path != oom_absolute_path && absolute_path != unknown_path)
        uacpi_free_dynamic_string(absolute_path);
}

static void stack_unwind(struct execution_context *ctx)
{
    uacpi_size depth;
    uacpi_bool should_stop;

    /*
     * Non-empty call stack here means the execution was aborted at some point,
     * probably due to a bytecode error.
     */
    depth = call_frame_array_size(&ctx->call_stack);

    if (depth != 0) {
        uacpi_size idx = 0;
        uacpi_bool table_level_code;

        do {
            table_level_code = ctx->cur_frame->method->named_objects_persist;

            if (table_level_code && idx != 0)
                /*
                 * This isn't the first frame that we are aborting.
                 * If this is table-level code, we have just unwound a call
                 * chain that had triggered an abort. Stop here, no need to
                 * abort table load because of it.
                 */
                break;

            while (op_context_array_size(&ctx->cur_frame->pending_ops) != 0)
                pop_op(ctx);

            trace_method_abort(
                code_block_array_at(&ctx->cur_frame->code_blocks, 0), idx
            );

            should_stop = idx++ == 0 && table_level_code;
            ctx_reload_post_ret(ctx);
        } while (--depth && !should_stop);
    }
}

static void execution_context_release(struct execution_context *ctx)
{
    if (ctx->ret)
        uacpi_object_unref(ctx->ret);

    while (held_mutexes_array_size(&ctx->held_mutexes) != 0) {
        held_mutexes_array_remove_and_release(
            &ctx->held_mutexes,
            *held_mutexes_array_last(&ctx->held_mutexes),
            FORCE_RELEASE_YES
        );
    }

    call_frame_array_clear(&ctx->call_stack);
    held_mutexes_array_clear(&ctx->held_mutexes);
    uacpi_free(ctx, sizeof(*ctx));
}

uacpi_status uacpi_execute_control_method(
    uacpi_namespace_node *scope, uacpi_control_method *method,
    const uacpi_object_array *args, uacpi_object **out_obj
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct execution_context *ctx;

    ctx = uacpi_kernel_alloc_zeroed(sizeof(*ctx));
    if (uacpi_unlikely(ctx == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    if (out_obj != UACPI_NULL) {
        ctx->ret = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
        if (uacpi_unlikely(ctx->ret == UACPI_NULL)) {
            ret = UACPI_STATUS_OUT_OF_MEMORY;
            goto out;
        }
    }

    ret = prepare_method_call(ctx, scope, method, METHOD_CALL_NATIVE, args);
    if (uacpi_unlikely_error(ret))
        goto out;

    for (;;) {
        if (!ctx_has_non_preempted_op(ctx)) {
            if (ctx->cur_frame == UACPI_NULL)
                break;

            if (maybe_end_block(ctx))
                continue;

            if (!call_frame_has_code(ctx->cur_frame)) {
                ctx_reload_post_ret(ctx);
                continue;
            }

            ret = get_op(ctx);
            if (uacpi_unlikely_error(ret))
                goto handle_method_abort;

            trace_op(ctx->cur_op, OP_TRACE_ACTION_BEGIN);
        }

        ret = exec_op(ctx);
        if (uacpi_unlikely_error(ret))
            goto handle_method_abort;

        continue;

    handle_method_abort:
        uacpi_error("aborting %s due to previous error: %s\n",
                    ctx->cur_frame->method->named_objects_persist ?
                        "table load" : "method invocation",
                    uacpi_status_to_string(ret));
        stack_unwind(ctx);

        /*
         * Having a frame here implies that we just aborted a dynamic table
         * load. Signal to the caller that it failed by setting the return
         * value to false.
         */
        if (ctx->cur_frame) {
            struct item *it;

            it = item_array_last(&ctx->cur_op_ctx->items);
            if (it != UACPI_NULL && it->obj != UACPI_NULL)
                it->obj->integer = 0;
        }
    }

out:
    if (ctx->ret != UACPI_NULL) {
        uacpi_object *ret_obj = UACPI_NULL;

        if (ctx->ret->type != UACPI_OBJECT_UNINITIALIZED) {
            ret_obj = ctx->ret;
            uacpi_object_ref(ret_obj);
        }

        *out_obj = ret_obj;
    }

    execution_context_release(ctx);
    return ret;
}

uacpi_status uacpi_osi(uacpi_handle handle, uacpi_object *retval)
{
    struct execution_context *ctx = handle;
    uacpi_bool is_supported;
    uacpi_status ret;
    uacpi_object *arg;

    arg = uacpi_unwrap_internal_reference(ctx->cur_frame->args[0]);
    if (arg->type != UACPI_OBJECT_STRING) {
        uacpi_error("_OSI: invalid argument type %s, expected a String\n",
                    uacpi_object_type_to_string(arg->type));
        return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
    }

    if (retval == UACPI_NULL)
        return UACPI_STATUS_OK;

    retval->type = UACPI_OBJECT_INTEGER;

    ret = uacpi_handle_osi(arg->buffer->text, &is_supported);
    if (uacpi_unlikely_error(ret))
        return ret;

    retval->integer = is_supported ? ones() : 0;

    uacpi_trace("_OSI(%s) => reporting as %ssupported\n",
                arg->buffer->text, is_supported ? "" : "un");
    return UACPI_STATUS_OK;
}

#endif // !UACPI_BAREBONES_MODE
