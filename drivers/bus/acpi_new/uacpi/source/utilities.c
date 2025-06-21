#include <uacpi/types.h>
#include <uacpi/status.h>
#include <uacpi/uacpi.h>

#include <uacpi/internal/context.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/namespace.h>

enum char_type {
    CHAR_TYPE_CONTROL = 1 << 0,
    CHAR_TYPE_SPACE = 1 << 1,
    CHAR_TYPE_BLANK = 1 << 2,
    CHAR_TYPE_PUNCTUATION = 1 << 3,
    CHAR_TYPE_LOWER = 1 << 4,
    CHAR_TYPE_UPPER = 1 << 5,
    CHAR_TYPE_DIGIT = 1 << 6,
    CHAR_TYPE_HEX_DIGIT  = 1 << 7,
    CHAR_TYPE_ALPHA = CHAR_TYPE_LOWER | CHAR_TYPE_UPPER,
    CHAR_TYPE_ALHEX = CHAR_TYPE_ALPHA | CHAR_TYPE_HEX_DIGIT,
    CHAR_TYPE_ALNUM = CHAR_TYPE_ALPHA | CHAR_TYPE_DIGIT,
};

static const uacpi_u8 ascii_map[256] = {
    CHAR_TYPE_CONTROL, // 0
    CHAR_TYPE_CONTROL, // 1
    CHAR_TYPE_CONTROL, // 2
    CHAR_TYPE_CONTROL, // 3
    CHAR_TYPE_CONTROL, // 4
    CHAR_TYPE_CONTROL, // 5
    CHAR_TYPE_CONTROL, // 6
    CHAR_TYPE_CONTROL, // 7
    CHAR_TYPE_CONTROL, // -> 8 control codes

    CHAR_TYPE_CONTROL | CHAR_TYPE_SPACE | CHAR_TYPE_BLANK, // 9 tab

    CHAR_TYPE_CONTROL | CHAR_TYPE_SPACE, // 10
    CHAR_TYPE_CONTROL | CHAR_TYPE_SPACE, // 11
    CHAR_TYPE_CONTROL | CHAR_TYPE_SPACE, // 12
    CHAR_TYPE_CONTROL | CHAR_TYPE_SPACE, // -> 13 whitespaces

    CHAR_TYPE_CONTROL, // 14
    CHAR_TYPE_CONTROL, // 15
    CHAR_TYPE_CONTROL, // 16
    CHAR_TYPE_CONTROL, // 17
    CHAR_TYPE_CONTROL, // 18
    CHAR_TYPE_CONTROL, // 19
    CHAR_TYPE_CONTROL, // 20
    CHAR_TYPE_CONTROL, // 21
    CHAR_TYPE_CONTROL, // 22
    CHAR_TYPE_CONTROL, // 23
    CHAR_TYPE_CONTROL, // 24
    CHAR_TYPE_CONTROL, // 25
    CHAR_TYPE_CONTROL, // 26
    CHAR_TYPE_CONTROL, // 27
    CHAR_TYPE_CONTROL, // 28
    CHAR_TYPE_CONTROL, // 29
    CHAR_TYPE_CONTROL, // 30
    CHAR_TYPE_CONTROL, // -> 31 control codes

    CHAR_TYPE_SPACE | CHAR_TYPE_BLANK, // 32 space

    CHAR_TYPE_PUNCTUATION, // 33
    CHAR_TYPE_PUNCTUATION, // 34
    CHAR_TYPE_PUNCTUATION, // 35
    CHAR_TYPE_PUNCTUATION, // 36
    CHAR_TYPE_PUNCTUATION, // 37
    CHAR_TYPE_PUNCTUATION, // 38
    CHAR_TYPE_PUNCTUATION, // 39
    CHAR_TYPE_PUNCTUATION, // 40
    CHAR_TYPE_PUNCTUATION, // 41
    CHAR_TYPE_PUNCTUATION, // 42
    CHAR_TYPE_PUNCTUATION, // 43
    CHAR_TYPE_PUNCTUATION, // 44
    CHAR_TYPE_PUNCTUATION, // 45
    CHAR_TYPE_PUNCTUATION, // 46
    CHAR_TYPE_PUNCTUATION, // -> 47 punctuation

    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 48
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 49
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 50
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 51
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 52
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 53
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 54
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 55
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // 56
    CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT, // -> 57 digits

    CHAR_TYPE_PUNCTUATION, // 58
    CHAR_TYPE_PUNCTUATION, // 59
    CHAR_TYPE_PUNCTUATION, // 60
    CHAR_TYPE_PUNCTUATION, // 61
    CHAR_TYPE_PUNCTUATION, // 62
    CHAR_TYPE_PUNCTUATION, // 63
    CHAR_TYPE_PUNCTUATION, // -> 64 punctuation

    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // 65
    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // 66
    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // 67
    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // 68
    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // 69
    CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT, // -> 70 ABCDEF

    CHAR_TYPE_UPPER, // 71
    CHAR_TYPE_UPPER, // 72
    CHAR_TYPE_UPPER, // 73
    CHAR_TYPE_UPPER, // 74
    CHAR_TYPE_UPPER, // 75
    CHAR_TYPE_UPPER, // 76
    CHAR_TYPE_UPPER, // 77
    CHAR_TYPE_UPPER, // 78
    CHAR_TYPE_UPPER, // 79
    CHAR_TYPE_UPPER, // 80
    CHAR_TYPE_UPPER, // 81
    CHAR_TYPE_UPPER, // 82
    CHAR_TYPE_UPPER, // 83
    CHAR_TYPE_UPPER, // 84
    CHAR_TYPE_UPPER, // 85
    CHAR_TYPE_UPPER, // 86
    CHAR_TYPE_UPPER, // 87
    CHAR_TYPE_UPPER, // 88
    CHAR_TYPE_UPPER, // 89
    CHAR_TYPE_UPPER, // -> 90 the rest of UPPERCASE alphabet

    CHAR_TYPE_PUNCTUATION, // 91
    CHAR_TYPE_PUNCTUATION, // 92
    CHAR_TYPE_PUNCTUATION, // 93
    CHAR_TYPE_PUNCTUATION, // 94
    CHAR_TYPE_PUNCTUATION, // 95
    CHAR_TYPE_PUNCTUATION, // -> 96 punctuation

    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // 97
    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // 98
    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // 99
    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // 100
    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // 101
    CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT, // -> 102 abcdef

    CHAR_TYPE_LOWER, // 103
    CHAR_TYPE_LOWER, // 104
    CHAR_TYPE_LOWER, // 105
    CHAR_TYPE_LOWER, // 106
    CHAR_TYPE_LOWER, // 107
    CHAR_TYPE_LOWER, // 108
    CHAR_TYPE_LOWER, // 109
    CHAR_TYPE_LOWER, // 110
    CHAR_TYPE_LOWER, // 111
    CHAR_TYPE_LOWER, // 112
    CHAR_TYPE_LOWER, // 113
    CHAR_TYPE_LOWER, // 114
    CHAR_TYPE_LOWER, // 115
    CHAR_TYPE_LOWER, // 116
    CHAR_TYPE_LOWER, // 117
    CHAR_TYPE_LOWER, // 118
    CHAR_TYPE_LOWER, // 119
    CHAR_TYPE_LOWER, // 120
    CHAR_TYPE_LOWER, // 121
    CHAR_TYPE_LOWER, // -> 122 the rest of UPPERCASE alphabet

    CHAR_TYPE_PUNCTUATION, // 123
    CHAR_TYPE_PUNCTUATION, // 124
    CHAR_TYPE_PUNCTUATION, // 125
    CHAR_TYPE_PUNCTUATION, // -> 126 punctuation

    CHAR_TYPE_CONTROL // 127 backspace
};

static uacpi_bool is_char(uacpi_char c, enum char_type type)
{
    return (ascii_map[(uacpi_u8)c] & type) == type;
}

static uacpi_char to_lower(uacpi_char c)
{
    if (is_char(c, CHAR_TYPE_UPPER))
        return c + ('a' - 'A');

    return c;
}

static uacpi_bool peek_one(
    const uacpi_char **str, const uacpi_size *size, uacpi_char *out_char
)
{
    if (*size == 0)
        return UACPI_FALSE;

    *out_char = **str;
    return UACPI_TRUE;
}

static uacpi_bool consume_one(
    const uacpi_char **str, uacpi_size *size, uacpi_char *out_char
)
{
    if (!peek_one(str, size, out_char))
        return UACPI_FALSE;

    *str += 1;
    *size -= 1;
    return UACPI_TRUE;
}

static uacpi_bool consume_if(
    const uacpi_char **str, uacpi_size *size, enum char_type type
)
{
    uacpi_char c;

    if (!peek_one(str, size, &c) || !is_char(c, type))
        return UACPI_FALSE;

    *str += 1;
    *size -= 1;
    return UACPI_TRUE;
}

static uacpi_bool consume_if_equals(
    const uacpi_char **str, uacpi_size *size, uacpi_char c
)
{
    uacpi_char c1;

    if (!peek_one(str, size, &c1) || to_lower(c1) != c)
        return UACPI_FALSE;

    *str += 1;
    *size -= 1;
    return UACPI_TRUE;
}

uacpi_status uacpi_string_to_integer(
    const uacpi_char *str, uacpi_size max_chars, enum uacpi_base base,
    uacpi_u64 *out_value
)
{
    uacpi_status ret = UACPI_STATUS_INVALID_ARGUMENT;
    uacpi_bool negative = UACPI_FALSE;
    uacpi_u64 next, value = 0;
    uacpi_char c = '\0';

    while (consume_if(&str, &max_chars, CHAR_TYPE_SPACE));

    if (consume_if_equals(&str, &max_chars, '-'))
        negative = UACPI_TRUE;
    else
        consume_if_equals(&str, &max_chars, '+');

    if (base == UACPI_BASE_AUTO) {
        base = UACPI_BASE_DEC;

        if (consume_if_equals(&str, &max_chars, '0')) {
            base = UACPI_BASE_OCT;
            if (consume_if_equals(&str, &max_chars, 'x'))
                base = UACPI_BASE_HEX;
        }
    }

    while (consume_one(&str, &max_chars, &c)) {
        switch (ascii_map[(uacpi_u8)c] & (CHAR_TYPE_DIGIT | CHAR_TYPE_ALHEX)) {
        case CHAR_TYPE_DIGIT | CHAR_TYPE_HEX_DIGIT:
            next = c - '0';
            if (base == UACPI_BASE_OCT && next > 7)
                goto out;
            break;
        case CHAR_TYPE_LOWER | CHAR_TYPE_HEX_DIGIT:
        case CHAR_TYPE_UPPER | CHAR_TYPE_HEX_DIGIT:
            if (base != UACPI_BASE_HEX)
                goto out;
            next = 10 + (to_lower(c) - 'a');
            break;
        default:
            goto out;
        }

        next = (value * base) + next;
        if ((next / base) != value) {
            value = 0xFFFFFFFFFFFFFFFF;
            goto out;
        }

        value = next;
    }

out:
    if (negative)
        value = -((uacpi_i64)value);

    *out_value = value;
    if (max_chars == 0 || c == '\0')
        ret = UACPI_STATUS_OK;

    return ret;
}

#ifndef UACPI_BAREBONES_MODE

static inline uacpi_bool is_valid_name_byte(uacpi_u8 c)
{
    // ‘_’ := 0x5F
    if (c == 0x5F)
        return UACPI_TRUE;

    /*
     * LeadNameChar := ‘A’-‘Z’ | ‘_’
     * DigitChar := ‘0’ - ‘9’
     * NameChar := DigitChar | LeadNameChar
     * ‘A’-‘Z’ := 0x41 - 0x5A
     * ‘0’-‘9’ := 0x30 - 0x39
     */
    return (ascii_map[c] & (CHAR_TYPE_DIGIT | CHAR_TYPE_UPPER)) != 0;
}

uacpi_bool uacpi_is_valid_nameseg(uacpi_u8 *nameseg)
{
    return is_valid_name_byte(nameseg[0]) &&
           is_valid_name_byte(nameseg[1]) &&
           is_valid_name_byte(nameseg[2]) &&
           is_valid_name_byte(nameseg[3]);
}

void uacpi_eisa_id_to_string(uacpi_u32 id, uacpi_char *out_string)
{
    static uacpi_char hex_to_ascii[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'A', 'B', 'C', 'D', 'E', 'F'
    };

    /*
     * For whatever reason bits are encoded upper to lower here, swap
     * them around so that we don't have to do ridiculous bit shifts
     * everywhere.
     */
    union {
        uacpi_u8 bytes[4];
        uacpi_u32 dword;
    } orig, swapped;

    orig.dword = id;
    swapped.bytes[0] = orig.bytes[3];
    swapped.bytes[1] = orig.bytes[2];
    swapped.bytes[2] = orig.bytes[1];
    swapped.bytes[3] = orig.bytes[0];

    /*
     * Bit 16 - 20: 3rd character (- 0x40) of mfg code
     * Bit 21 - 25: 2nd character (- 0x40) of mfg code
     * Bit 26 - 30: 1st character (- 0x40) of mfg code
     */
    out_string[0] = (uacpi_char)(0x40 + ((swapped.dword >> 26) & 0x1F));
    out_string[1] = (uacpi_char)(0x40 + ((swapped.dword >> 21) & 0x1F));
    out_string[2] = (uacpi_char)(0x40 + ((swapped.dword >> 16) & 0x1F));

    /*
     * Bit 0  - 3 : 4th hex digit of product number
     * Bit 4  - 7 : 3rd hex digit of product number
     * Bit 8  - 11: 2nd hex digit of product number
     * Bit 12 - 15: 1st hex digit of product number
     */
    out_string[3] = hex_to_ascii[(swapped.dword >> 12) & 0x0F];
    out_string[4] = hex_to_ascii[(swapped.dword >> 8 ) & 0x0F];
    out_string[5] = hex_to_ascii[(swapped.dword >> 4 ) & 0x0F];
    out_string[6] = hex_to_ascii[(swapped.dword >> 0 ) & 0x0F];

    out_string[7] = '\0';
}

#define PNP_ID_LENGTH 8

uacpi_status uacpi_eval_hid(uacpi_namespace_node *node, uacpi_id_string **out_id)
{
    uacpi_status ret;
    uacpi_object *hid_ret;
    uacpi_id_string *id = UACPI_NULL;
    uacpi_u32 size;

    ret = uacpi_eval_typed(
        node, "_HID", UACPI_NULL,
        UACPI_OBJECT_INTEGER_BIT | UACPI_OBJECT_STRING_BIT,
        &hid_ret
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    size = sizeof(uacpi_id_string);

    switch (hid_ret->type) {
    case UACPI_OBJECT_STRING: {
        uacpi_buffer *buf = hid_ret->buffer;

        size += buf->size;
        if (uacpi_unlikely(buf->size == 0 || size < buf->size)) {
            uacpi_error(
                "%.4s._HID: empty/invalid EISA ID string (%zu bytes)\n",
                uacpi_namespace_node_name(node).text, buf->size
            );
            ret = UACPI_STATUS_AML_BAD_ENCODING;
            break;
        }

        id = uacpi_kernel_alloc(size);
        if (uacpi_unlikely(id == UACPI_NULL)) {
            ret = UACPI_STATUS_OUT_OF_MEMORY;
            break;
        }
        id->size = buf->size;
        id->value = UACPI_PTR_ADD(id, sizeof(uacpi_id_string));

        uacpi_memcpy(id->value, buf->text, buf->size);
        id->value[buf->size - 1] = '\0';
        break;
    }

    case UACPI_OBJECT_INTEGER:
        size += PNP_ID_LENGTH;

        id = uacpi_kernel_alloc(size);
        if (uacpi_unlikely(id == UACPI_NULL)) {
            ret = UACPI_STATUS_OUT_OF_MEMORY;
            break;
        }
        id->size = PNP_ID_LENGTH;
        id->value = UACPI_PTR_ADD(id, sizeof(uacpi_id_string));

        uacpi_eisa_id_to_string(hid_ret->integer, id->value);
        break;
    }

    uacpi_object_unref(hid_ret);
    if (uacpi_likely_success(ret))
        *out_id = id;
    return ret;
}

void uacpi_free_id_string(uacpi_id_string *id)
{
    if (id == UACPI_NULL)
        return;

    uacpi_free(id, sizeof(uacpi_id_string) + id->size);
}

uacpi_status uacpi_eval_cid(
    uacpi_namespace_node *node, uacpi_pnp_id_list **out_list
)
{
    uacpi_status ret;
    uacpi_object *object, *cid_ret;
    uacpi_object **objects;
    uacpi_size num_ids, i;
    uacpi_u32 size;
    uacpi_id_string *id;
    uacpi_char *id_buffer;
    uacpi_pnp_id_list *list;

    ret = uacpi_eval_typed(
        node, "_CID", UACPI_NULL,
        UACPI_OBJECT_INTEGER_BIT | UACPI_OBJECT_STRING_BIT |
        UACPI_OBJECT_PACKAGE_BIT,
        &cid_ret
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    switch (cid_ret->type) {
    case UACPI_OBJECT_PACKAGE:
        objects = cid_ret->package->objects;
        num_ids = cid_ret->package->count;
        break;
    default:
        objects = &cid_ret;
        num_ids = 1;
        break;
    }

    size = sizeof(uacpi_pnp_id_list);
    size += num_ids * sizeof(uacpi_id_string);

    for (i = 0; i < num_ids; ++i) {
        object = objects[i];

        switch (object->type) {
        case UACPI_OBJECT_STRING: {
            uacpi_size buf_size = object->buffer->size;

            if (uacpi_unlikely(buf_size == 0)) {
                uacpi_error(
                    "%.4s._CID: empty EISA ID string (sub-object %zu)\n",
                    uacpi_namespace_node_name(node).text, i
                );
                return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
            }

            size += buf_size;
            if (uacpi_unlikely(size < buf_size)) {
                uacpi_error(
                    "%.4s._CID: buffer size overflow (+ %zu)\n",
                    uacpi_namespace_node_name(node).text, buf_size
                );
                return UACPI_STATUS_AML_BAD_ENCODING;
            }

            break;
        }

        case UACPI_OBJECT_INTEGER:
            size += PNP_ID_LENGTH;
            break;
        default:
            uacpi_error(
                "%.4s._CID: invalid package sub-object %zu type: %s\n",
                uacpi_namespace_node_name(node).text, i,
                uacpi_object_type_to_string(object->type)
            );
            return UACPI_STATUS_AML_INCOMPATIBLE_OBJECT_TYPE;
        }
    }

    list = uacpi_kernel_alloc(size);
    if (uacpi_unlikely(list == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;
    list->num_ids = num_ids;
    list->size = size - sizeof(uacpi_pnp_id_list);

    id_buffer = UACPI_PTR_ADD(list, sizeof(uacpi_pnp_id_list));
    id_buffer += num_ids * sizeof(uacpi_id_string);

    for (i = 0; i < num_ids; ++i) {
        object = objects[i];
        id = &list->ids[i];

        switch (object->type) {
        case UACPI_OBJECT_STRING: {
            uacpi_buffer *buf = object->buffer;

            id->size = buf->size;
            id->value = id_buffer;

            uacpi_memcpy(id->value, buf->text, id->size);
            id->value[id->size - 1] = '\0';
            break;
        }

        case UACPI_OBJECT_INTEGER:
            id->size = PNP_ID_LENGTH;
            id->value = id_buffer;
            uacpi_eisa_id_to_string(object->integer, id_buffer);
            break;
        }

        id_buffer += id->size;
    }

    uacpi_object_unref(cid_ret);
    *out_list = list;
    return ret;
}

void uacpi_free_pnp_id_list(uacpi_pnp_id_list *list)
{
    if (list == UACPI_NULL)
        return;

    uacpi_free(list, sizeof(uacpi_pnp_id_list) + list->size);
}

uacpi_status uacpi_eval_sta(uacpi_namespace_node *node, uacpi_u32 *flags)
{
    uacpi_status ret;
    uacpi_u64 value = 0;

    ret = uacpi_eval_integer(node, "_STA", UACPI_NULL, &value);

    /*
     * ACPI 6.5 specification:
     * If a device object (including the processor object) does not have
     * an _STA object, then OSPM assumes that all of the above bits are
     * set (i.e., the device is present, enabled, shown in the UI,
     * and functioning).
     */
    if (ret == UACPI_STATUS_NOT_FOUND) {
        value = 0xFFFFFFFF;
        ret = UACPI_STATUS_OK;
    }

    *flags = value;
    return ret;
}

uacpi_status uacpi_eval_adr(uacpi_namespace_node *node, uacpi_u64 *out)
{
    return uacpi_eval_integer(node, "_ADR", UACPI_NULL, out);
}

#define CLS_REPR_SIZE 7

static uacpi_u8 extract_package_byte_or_zero(uacpi_package *pkg, uacpi_size i)
{
    uacpi_object *obj;

    if (uacpi_unlikely(pkg->count <= i))
        return 0;

    obj = pkg->objects[i];
    if (uacpi_unlikely(obj->type != UACPI_OBJECT_INTEGER))
        return 0;

    return obj->integer;
}

uacpi_status uacpi_eval_cls(
    uacpi_namespace_node *node, uacpi_id_string **out_id
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_package *pkg;
    uacpi_u8 class_codes[3];
    uacpi_id_string *id_string;

    ret = uacpi_eval_typed(
        node, "_CLS", UACPI_NULL, UACPI_OBJECT_PACKAGE_BIT, &obj
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    pkg = obj->package;
    class_codes[0] = extract_package_byte_or_zero(pkg, 0);
    class_codes[1] = extract_package_byte_or_zero(pkg, 1);
    class_codes[2] = extract_package_byte_or_zero(pkg, 2);

    id_string = uacpi_kernel_alloc(sizeof(uacpi_id_string) + CLS_REPR_SIZE);
    if (uacpi_unlikely(id_string == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    id_string->size = CLS_REPR_SIZE;
    id_string->value = UACPI_PTR_ADD(id_string, sizeof(uacpi_id_string));

    uacpi_snprintf(
        id_string->value, CLS_REPR_SIZE, "%02X%02X%02X",
        class_codes[0], class_codes[1], class_codes[2]
    );

out:
    if (uacpi_likely_success(ret))
        *out_id = id_string;

    uacpi_object_unref(obj);
    return ret;
}

uacpi_status uacpi_eval_uid(
    uacpi_namespace_node *node, uacpi_id_string **out_uid
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_id_string *id_string;
    uacpi_u32 size;

    ret = uacpi_eval_typed(
        node, "_UID", UACPI_NULL,
        UACPI_OBJECT_INTEGER_BIT | UACPI_OBJECT_STRING_BIT,
        &obj
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    if (obj->type == UACPI_OBJECT_STRING) {
        size = obj->buffer->size;
        if (uacpi_unlikely(size == 0 || size > 0xE0000000)) {
            uacpi_error(
                "invalid %.4s._UID string size: %u\n",
                uacpi_namespace_node_name(node).text, size
            );
            ret = UACPI_STATUS_AML_BAD_ENCODING;
            goto out;
        }
    } else {
        size = uacpi_snprintf(
            UACPI_NULL, 0, "%"UACPI_PRIu64, UACPI_FMT64(obj->integer)
        ) + 1;
    }

    id_string = uacpi_kernel_alloc(sizeof(uacpi_id_string) + size);
    if (uacpi_unlikely(id_string == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    id_string->value = UACPI_PTR_ADD(id_string, sizeof(uacpi_id_string));
    id_string->size = size;

    if (obj->type == UACPI_OBJECT_STRING) {
        uacpi_memcpy(id_string->value, obj->buffer->text, size);
        id_string->value[size - 1] = '\0';
    } else {
        uacpi_snprintf(
            id_string->value, id_string->size, "%"UACPI_PRIu64,
            UACPI_FMT64(obj->integer)
        );
    }

out:
    if (uacpi_likely_success(ret))
        *out_uid = id_string;

    uacpi_object_unref(obj);
    return ret;
}

static uacpi_bool matches_any(
    uacpi_id_string *id, const uacpi_char *const *ids
)
{
    uacpi_size i;

    for (i = 0; ids[i]; ++i) {
        if (uacpi_strcmp(id->value, ids[i]) == 0)
            return UACPI_TRUE;
    }

    return UACPI_FALSE;
}

static uacpi_status uacpi_eval_dstate_method_template(
    uacpi_namespace_node *parent, uacpi_char *template, uacpi_u8 num_methods,
    uacpi_u8 *out_values
)
{
    uacpi_u8 i;
    uacpi_status ret = UACPI_STATUS_NOT_FOUND, eval_ret;
    uacpi_object *obj;

    // We expect either _SxD or _SxW, so increment template[2]
    for (i = 0; i < num_methods; ++i, template[2]++) {
        eval_ret = uacpi_eval_typed(
            parent, template, UACPI_NULL, UACPI_OBJECT_INTEGER_BIT, &obj
        );
        if (eval_ret == UACPI_STATUS_OK) {
            ret = UACPI_STATUS_OK;
            out_values[i] = obj->integer;
            uacpi_object_unref(obj);
            continue;
        }

        out_values[i] = 0xFF;
        if (uacpi_unlikely(eval_ret != UACPI_STATUS_NOT_FOUND)) {
            const char *path;

            path = uacpi_namespace_node_generate_absolute_path(parent);
            uacpi_warn(
                "failed to evaluate %s.%s: %s\n",
                path, template, uacpi_status_to_string(eval_ret)
            );
            uacpi_free_dynamic_string(path);
        }
    }

    return ret;
}

#define NODE_INFO_EVAL_ADD_ID(name)                          \
    if (uacpi_eval_##name(node, &name) == UACPI_STATUS_OK) { \
        size += name->size;                                  \
        if (uacpi_unlikely(size < name->size)) {             \
            ret = UACPI_STATUS_AML_BAD_ENCODING;             \
            goto out;                                        \
        }                                                    \
    }

#define NODE_INFO_COPY_ID(name, flag)                  \
    if (name != UACPI_NULL) {                          \
        flags |= UACPI_NS_NODE_INFO_HAS_##flag;        \
        info->name.value = cursor;                     \
        info->name.size = name->size;                  \
        uacpi_memcpy(cursor, name->value, name->size); \
        cursor += name->size;                          \
    } else {                                           \
        uacpi_memzero(&info->name, sizeof(*name));     \
    }                                                  \

uacpi_status uacpi_get_namespace_node_info(
    uacpi_namespace_node *node, uacpi_namespace_node_info **out_info
)
{
    uacpi_status ret = UACPI_STATUS_OK;
    uacpi_u32 size = sizeof(uacpi_namespace_node_info);
    uacpi_object *obj;
    uacpi_namespace_node_info *info;
    uacpi_id_string *hid = UACPI_NULL, *uid = UACPI_NULL, *cls = UACPI_NULL;
    uacpi_pnp_id_list *cid = UACPI_NULL;
    uacpi_char *cursor;
    uacpi_u64 adr = 0;
    uacpi_u8 flags = 0;
    uacpi_u8 sxd[4], sxw[5];

    obj = uacpi_namespace_node_get_object(node);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    if (obj->type == UACPI_OBJECT_DEVICE ||
        obj->type == UACPI_OBJECT_PROCESSOR) {
        char dstate_method_template[5] = { '_', 'S', '1', 'D', '\0' };

        NODE_INFO_EVAL_ADD_ID(hid)
        NODE_INFO_EVAL_ADD_ID(uid)
        NODE_INFO_EVAL_ADD_ID(cls)
        NODE_INFO_EVAL_ADD_ID(cid)

        if (uacpi_eval_adr(node, &adr) == UACPI_STATUS_OK)
            flags |= UACPI_NS_NODE_INFO_HAS_ADR;

        if (uacpi_eval_dstate_method_template(
                node, dstate_method_template, sizeof(sxd), sxd
            ) == UACPI_STATUS_OK)
            flags |= UACPI_NS_NODE_INFO_HAS_SXD;

        dstate_method_template[2] = '0';
        dstate_method_template[3] = 'W';

        if (uacpi_eval_dstate_method_template(
                node, dstate_method_template, sizeof(sxw), sxw
            ) == UACPI_STATUS_OK)
            flags |= UACPI_NS_NODE_INFO_HAS_SXW;
    }

    info = uacpi_kernel_alloc(size);
    if (uacpi_unlikely(info == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }
    info->size = size;
    cursor = UACPI_PTR_ADD(info, sizeof(uacpi_namespace_node_info));
    info->name = uacpi_namespace_node_name(node);
    info->type = obj->type;
    info->num_params = info->type == UACPI_OBJECT_METHOD ? obj->method->args : 0;

    info->adr = adr;
    if (flags & UACPI_NS_NODE_INFO_HAS_SXD)
        uacpi_memcpy(info->sxd, sxd, sizeof(sxd));
    else
        uacpi_memzero(info->sxd, sizeof(info->sxd));

    if (flags & UACPI_NS_NODE_INFO_HAS_SXW)
        uacpi_memcpy(info->sxw, sxw, sizeof(sxw));
    else
        uacpi_memzero(info->sxw, sizeof(info->sxw));

    if (cid != UACPI_NULL) {
        uacpi_u32 i;

        uacpi_memcpy(&info->cid, cid, cid->size + sizeof(*cid));
        cursor += cid->num_ids * sizeof(uacpi_id_string);

        for (i = 0; i < cid->num_ids; ++i) {
            info->cid.ids[i].value = cursor;
            cursor += info->cid.ids[i].size;
        }

        flags |= UACPI_NS_NODE_INFO_HAS_CID;
    } else {
        uacpi_memzero(&info->cid, sizeof(*cid));
    }

    NODE_INFO_COPY_ID(hid, HID)
    NODE_INFO_COPY_ID(uid, UID)
    NODE_INFO_COPY_ID(cls, CLS)

out:
    if (uacpi_likely_success(ret)) {
        info->flags = flags;
        *out_info = info;
    }

    uacpi_free_id_string(hid);
    uacpi_free_id_string(uid);
    uacpi_free_id_string(cls);
    uacpi_free_pnp_id_list(cid);
    return ret;
}

void uacpi_free_namespace_node_info(uacpi_namespace_node_info *info)
{
    if (info == UACPI_NULL)
        return;

    uacpi_free(info, info->size);
}

uacpi_bool uacpi_device_matches_pnp_id(
    uacpi_namespace_node *node, const uacpi_char *const *ids
)
{
    uacpi_status st;
    uacpi_bool ret = UACPI_FALSE;
    uacpi_id_string *id = UACPI_NULL;
    uacpi_pnp_id_list *id_list = UACPI_NULL;

    st = uacpi_eval_hid(node, &id);
    if (st == UACPI_STATUS_OK && matches_any(id, ids)) {
        ret = UACPI_TRUE;
        goto out;
    }

    st = uacpi_eval_cid(node, &id_list);
    if (st == UACPI_STATUS_OK) {
        uacpi_size i;

        for (i = 0; i < id_list->num_ids; ++i) {
            if (matches_any(&id_list->ids[i], ids)) {
                ret = UACPI_TRUE;
                goto out;
            }
        }
    }

out:
    uacpi_free_id_string(id);
    uacpi_free_pnp_id_list(id_list);
    return ret;
}

struct device_find_ctx {
    const uacpi_char *const *target_hids;
    void *user;
    uacpi_iteration_callback cb;
};

static uacpi_iteration_decision find_one_device(
    void *opaque, uacpi_namespace_node *node, uacpi_u32 depth
)
{
    struct device_find_ctx *ctx = opaque;
    uacpi_status ret;
    uacpi_u32 flags;

    if (!uacpi_device_matches_pnp_id(node, ctx->target_hids))
        return UACPI_ITERATION_DECISION_CONTINUE;

    ret = uacpi_eval_sta(node, &flags);
    if (uacpi_unlikely_error(ret))
        return UACPI_ITERATION_DECISION_NEXT_PEER;

    if (!(flags & ACPI_STA_RESULT_DEVICE_PRESENT) &&
        !(flags & ACPI_STA_RESULT_DEVICE_FUNCTIONING))
        return UACPI_ITERATION_DECISION_NEXT_PEER;

    return ctx->cb(ctx->user, node, depth);
}


uacpi_status uacpi_find_devices_at(
    uacpi_namespace_node *parent, const uacpi_char *const *hids,
    uacpi_iteration_callback cb, void *user
)
{
    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    struct device_find_ctx ctx = {
        .target_hids = hids,
        .user = user,
        .cb = cb,
    };

    return uacpi_namespace_for_each_child(
        parent, find_one_device, UACPI_NULL, UACPI_OBJECT_DEVICE_BIT,
        UACPI_MAX_DEPTH_ANY, &ctx
    );
}

uacpi_status uacpi_find_devices(
    const uacpi_char *hid, uacpi_iteration_callback cb, void *user
)
{
    const uacpi_char *hids[] = {
        hid, UACPI_NULL
    };

    return uacpi_find_devices_at(uacpi_namespace_root(), hids, cb, user);
}

uacpi_status uacpi_set_interrupt_model(uacpi_interrupt_model model)
{
    uacpi_status ret;
    uacpi_object *arg;
    uacpi_object_array args;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    arg = uacpi_create_object(UACPI_OBJECT_INTEGER);
    if (uacpi_unlikely(arg == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    arg->integer = model;
    args.objects = &arg;
    args.count = 1;

    ret = uacpi_eval(uacpi_namespace_root(), "_PIC", &args, UACPI_NULL);
    uacpi_object_unref(arg);

    if (ret == UACPI_STATUS_NOT_FOUND)
        ret = UACPI_STATUS_OK;

    return ret;
}

uacpi_status uacpi_get_pci_routing_table(
    uacpi_namespace_node *parent, uacpi_pci_routing_table **out_table
)
{
    uacpi_status ret;
    uacpi_object *obj, *entry_obj, *elem_obj;
    uacpi_package *table_pkg, *entry_pkg;
    uacpi_pci_routing_table_entry *entry;
    uacpi_pci_routing_table *table;
    uacpi_size size, i;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_NAMESPACE_LOADED);

    obj = uacpi_namespace_node_get_object(parent);
    if (uacpi_unlikely(obj == UACPI_NULL || obj->type != UACPI_OBJECT_DEVICE))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_eval_typed(
        parent, "_PRT", UACPI_NULL, UACPI_OBJECT_PACKAGE_BIT, &obj
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    table_pkg = obj->package;
    if (uacpi_unlikely(table_pkg->count == 0 || table_pkg->count > 1024)) {
        uacpi_error("invalid number of _PRT entries: %zu\n", table_pkg->count);
        uacpi_object_unref(obj);
        return UACPI_STATUS_AML_BAD_ENCODING;
    }

    size = table_pkg->count * sizeof(uacpi_pci_routing_table_entry);
    table = uacpi_kernel_alloc(sizeof(uacpi_pci_routing_table) + size);
    if (uacpi_unlikely(table == UACPI_NULL)) {
        uacpi_object_unref(obj);
        return UACPI_STATUS_OUT_OF_MEMORY;
    }
    table->num_entries = table_pkg->count;

    for (i = 0; i < table_pkg->count; ++i) {
        entry_obj = table_pkg->objects[i];

        if (uacpi_unlikely(entry_obj->type != UACPI_OBJECT_PACKAGE)) {
            uacpi_error("_PRT sub-object %zu is not a package: %s\n",
                        i, uacpi_object_type_to_string(entry_obj->type));
            goto out_bad_encoding;
        }

        entry_pkg = entry_obj->package;
        if (uacpi_unlikely(entry_pkg->count != 4)) {
            uacpi_error("invalid _PRT sub-package entry count %zu\n",
                        entry_pkg->count);
            goto out_bad_encoding;
        }

        entry = &table->entries[i];

        elem_obj = entry_pkg->objects[0];
        if (uacpi_unlikely(elem_obj->type != UACPI_OBJECT_INTEGER)) {
            uacpi_error("invalid _PRT sub-package %zu address type: %s\n",
                        i, uacpi_object_type_to_string(elem_obj->type));
            goto out_bad_encoding;
        }
        entry->address = elem_obj->integer;

        elem_obj = entry_pkg->objects[1];
        if (uacpi_unlikely(elem_obj->type != UACPI_OBJECT_INTEGER)) {
            uacpi_error("invalid _PRT sub-package %zu pin type: %s\n",
                        i, uacpi_object_type_to_string(elem_obj->type));
            goto out_bad_encoding;
        }
        entry->pin = elem_obj->integer;

        elem_obj = entry_pkg->objects[2];
        switch (elem_obj->type) {
        case UACPI_OBJECT_STRING:
            ret = uacpi_object_resolve_as_aml_namepath(
                elem_obj, parent, &entry->source
            );
            if (uacpi_unlikely_error(ret)) {
                uacpi_error("unable to lookup _PRT source %s: %s\n",
                            elem_obj->buffer->text, uacpi_status_to_string(ret));
                goto out_bad_encoding;
            }
            break;
        case UACPI_OBJECT_INTEGER:
            entry->source = UACPI_NULL;
            break;
        default:
            uacpi_error("invalid _PRT sub-package %zu source type: %s\n",
                        i, uacpi_object_type_to_string(elem_obj->type));
            goto out_bad_encoding;
        }

        elem_obj = entry_pkg->objects[3];
        if (uacpi_unlikely(elem_obj->type != UACPI_OBJECT_INTEGER)) {
            uacpi_error("invalid _PRT sub-package %zu source index type: %s\n",
                        i, uacpi_object_type_to_string(elem_obj->type));
            goto out_bad_encoding;
        }
        entry->index = elem_obj->integer;
    }

    uacpi_object_unref(obj);
    *out_table = table;
    return UACPI_STATUS_OK;

out_bad_encoding:
    uacpi_object_unref(obj);
    uacpi_free_pci_routing_table(table);
    return UACPI_STATUS_AML_BAD_ENCODING;
}

void uacpi_free_pci_routing_table(uacpi_pci_routing_table *table)
{
    if (table == UACPI_NULL)
        return;

    uacpi_free(
        table,
        sizeof(uacpi_pci_routing_table) +
        table->num_entries * sizeof(uacpi_pci_routing_table_entry)
    );
}

void uacpi_free_dynamic_string(const uacpi_char *str)
{
    if (str == UACPI_NULL)
        return;

    uacpi_free((void*)str, uacpi_strlen(str) + 1);
}

#endif // !UACPI_BAREBONES_MODE
