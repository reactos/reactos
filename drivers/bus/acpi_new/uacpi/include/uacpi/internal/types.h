#pragma once

#include <uacpi/status.h>
#include <uacpi/types.h>
#include <uacpi/internal/shareable.h>

#ifndef UACPI_BAREBONES_MODE

// object->flags field if object->type == UACPI_OBJECT_REFERENCE
enum uacpi_reference_kind {
    UACPI_REFERENCE_KIND_REFOF = 0,
    UACPI_REFERENCE_KIND_LOCAL = 1,
    UACPI_REFERENCE_KIND_ARG = 2,
    UACPI_REFERENCE_KIND_NAMED = 3,
    UACPI_REFERENCE_KIND_PKG_INDEX = 4,
};

// object->flags field if object->type == UACPI_OBJECT_STRING
enum uacpi_string_kind {
    UACPI_STRING_KIND_NORMAL = 0,
    UACPI_STRING_KIND_PATH,
};

typedef struct uacpi_buffer {
    struct uacpi_shareable shareable;
    union {
        void *data;
        uacpi_u8 *byte_data;
        uacpi_char *text;
    };
    uacpi_size size;
} uacpi_buffer;

typedef struct uacpi_package {
    struct uacpi_shareable shareable;
    uacpi_object **objects;
    uacpi_size count;
} uacpi_package;

typedef struct uacpi_buffer_field {
    uacpi_buffer *backing;
    uacpi_size bit_index;
    uacpi_u32 bit_length;
    uacpi_bool force_buffer;
} uacpi_buffer_field;

typedef struct uacpi_buffer_index {
    uacpi_size idx;
    uacpi_buffer *buffer;
} uacpi_buffer_index;

typedef struct uacpi_mutex {
    struct uacpi_shareable shareable;
    uacpi_handle handle;
    uacpi_thread_id owner;
    uacpi_u16 depth;
    uacpi_u8 sync_level;
} uacpi_mutex;

typedef struct uacpi_event {
    struct uacpi_shareable shareable;
    uacpi_handle handle;
} uacpi_event;

typedef struct uacpi_address_space_handler {
    struct uacpi_shareable shareable;
    uacpi_region_handler callback;
    uacpi_handle user_context;
    struct uacpi_address_space_handler *next;
    struct uacpi_operation_region *regions;
    uacpi_u16 space;

#define UACPI_ADDRESS_SPACE_HANDLER_DEFAULT (1 << 0)
    uacpi_u16 flags;
} uacpi_address_space_handler;

/*
 * NOTE: These are common object headers.
 * Any changes to these structs must be propagated to all objects.
 * ==============================================================
 * Common for the following objects:
 * - UACPI_OBJECT_OPERATION_REGION
 * - UACPI_OBJECT_PROCESSOR
 * - UACPI_OBJECT_DEVICE
 * - UACPI_OBJECT_THERMAL_ZONE
 */
typedef struct uacpi_address_space_handlers {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *head;
} uacpi_address_space_handlers;

typedef struct uacpi_device_notify_handler {
    uacpi_notify_handler callback;
    uacpi_handle user_context;
    struct uacpi_device_notify_handler *next;
} uacpi_device_notify_handler;

/*
 * Common for the following objects:
 * - UACPI_OBJECT_PROCESSOR
 * - UACPI_OBJECT_DEVICE
 * - UACPI_OBJECT_THERMAL_ZONE
 */
typedef struct uacpi_handlers {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *address_space_head;
    uacpi_device_notify_handler *notify_head;
} uacpi_handlers;

// This region has a corresponding _REG method that was succesfully executed
#define UACPI_OP_REGION_STATE_REG_EXECUTED (1 << 0)

// This region was successfully attached to a handler
#define UACPI_OP_REGION_STATE_ATTACHED (1 << 1)

typedef struct uacpi_operation_region {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *handler;
    uacpi_handle user_context;
    uacpi_u16 space;
    uacpi_u8 state_flags;
    uacpi_u64 offset;
    uacpi_u64 length;

    union {
        // If space == TABLE_DATA
        uacpi_u64 table_idx;

        // If space == PCC
        uacpi_u8 *internal_buffer;
    };

    // Used to link regions sharing the same handler
    struct uacpi_operation_region *next;
} uacpi_operation_region;

typedef struct uacpi_device {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *address_space_handlers;
    uacpi_device_notify_handler *notify_handlers;
} uacpi_device;

typedef struct uacpi_processor {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *address_space_handlers;
    uacpi_device_notify_handler *notify_handlers;
    uacpi_u8 id;
    uacpi_u32 block_address;
    uacpi_u8 block_length;
} uacpi_processor;

typedef struct uacpi_thermal_zone {
    struct uacpi_shareable shareable;
    uacpi_address_space_handler *address_space_handlers;
    uacpi_device_notify_handler *notify_handlers;
} uacpi_thermal_zone;

typedef struct uacpi_power_resource {
    uacpi_u8 system_level;
    uacpi_u16 resource_order;
} uacpi_power_resource;

typedef uacpi_status (*uacpi_native_call_handler)(
    uacpi_handle ctx, uacpi_object *retval
);

typedef struct uacpi_control_method {
    struct uacpi_shareable shareable;
    union {
        uacpi_u8 *code;
        uacpi_native_call_handler handler;
    };
    uacpi_mutex *mutex;
    uacpi_u32 size;
    uacpi_u8 sync_level : 4;
    uacpi_u8 args : 3;
    uacpi_u8 is_serialized : 1;
    uacpi_u8 named_objects_persist: 1;
    uacpi_u8 native_call : 1;
    uacpi_u8 owns_code : 1;
} uacpi_control_method;

typedef enum uacpi_access_type {
    UACPI_ACCESS_TYPE_ANY = 0,
    UACPI_ACCESS_TYPE_BYTE = 1,
    UACPI_ACCESS_TYPE_WORD = 2,
    UACPI_ACCESS_TYPE_DWORD = 3,
    UACPI_ACCESS_TYPE_QWORD = 4,
    UACPI_ACCESS_TYPE_BUFFER = 5,
} uacpi_access_type;

typedef enum uacpi_lock_rule {
    UACPI_LOCK_RULE_NO_LOCK = 0,
    UACPI_LOCK_RULE_LOCK = 1,
} uacpi_lock_rule;

typedef enum uacpi_update_rule {
    UACPI_UPDATE_RULE_PRESERVE = 0,
    UACPI_UPDATE_RULE_WRITE_AS_ONES = 1,
    UACPI_UPDATE_RULE_WRITE_AS_ZEROES = 2,
} uacpi_update_rule;

typedef enum uacpi_field_unit_kind {
    UACPI_FIELD_UNIT_KIND_NORMAL = 0,
    UACPI_FIELD_UNIT_KIND_INDEX = 1,
    UACPI_FIELD_UNIT_KIND_BANK = 2,
} uacpi_field_unit_kind;

typedef struct uacpi_field_unit {
    struct uacpi_shareable shareable;

    union {
        // UACPI_FIELD_UNIT_KIND_NORMAL
        struct {
            uacpi_namespace_node *region;
        };

        // UACPI_FIELD_UNIT_KIND_INDEX
        struct {
            struct uacpi_field_unit *index;
            struct uacpi_field_unit *data;
        };

        // UACPI_FIELD_UNIT_KIND_BANK
        struct {
            uacpi_namespace_node *bank_region;
            struct uacpi_field_unit *bank_selection;
            uacpi_u64 bank_value;
        };
    };

    uacpi_object *connection;

    uacpi_u32 byte_offset;
    uacpi_u32 bit_length;
    uacpi_u32 pin_offset;
    uacpi_u8 bit_offset_within_first_byte;
    uacpi_u8 access_width_bytes;
    uacpi_u8 access_length;

    uacpi_u8 attributes : 4;
    uacpi_u8 update_rule : 2;
    uacpi_u8 kind : 2;
    uacpi_u8 lock_rule : 1;
} uacpi_field_unit;

typedef struct uacpi_object {
    struct uacpi_shareable shareable;
    uacpi_u8 type;
    uacpi_u8 flags;

    union {
        uacpi_u64 integer;
        uacpi_package *package;
        uacpi_buffer_field buffer_field;
        uacpi_object *inner_object;
        uacpi_control_method *method;
        uacpi_buffer *buffer;
        uacpi_mutex *mutex;
        uacpi_event *event;
        uacpi_buffer_index buffer_index;
        uacpi_operation_region *op_region;
        uacpi_device *device;
        uacpi_processor *processor;
        uacpi_thermal_zone *thermal_zone;
        uacpi_address_space_handlers *address_space_handlers;
        uacpi_handlers *handlers;
        uacpi_power_resource power_resource;
        uacpi_field_unit *field_unit;
    };
} uacpi_object;

uacpi_object *uacpi_create_object(uacpi_object_type type);

enum uacpi_assign_behavior {
    UACPI_ASSIGN_BEHAVIOR_DEEP_COPY,
    UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY,
};

uacpi_status uacpi_object_assign(uacpi_object *dst, uacpi_object *src,
                                 enum uacpi_assign_behavior);

void uacpi_object_attach_child(uacpi_object *parent, uacpi_object *child);
void uacpi_object_detach_child(uacpi_object *parent);

struct uacpi_object *uacpi_create_internal_reference(
    enum uacpi_reference_kind kind, uacpi_object *child
);
uacpi_object *uacpi_unwrap_internal_reference(uacpi_object *object);

enum uacpi_prealloc_objects {
    UACPI_PREALLOC_OBJECTS_NO,
    UACPI_PREALLOC_OBJECTS_YES,
};

uacpi_bool uacpi_package_fill(
    uacpi_package *pkg, uacpi_size num_elements,
    enum uacpi_prealloc_objects prealloc_objects
);

uacpi_mutex *uacpi_create_mutex(void);
void uacpi_mutex_unref(uacpi_mutex*);

void uacpi_method_unref(uacpi_control_method*);

void uacpi_address_space_handler_unref(uacpi_address_space_handler *handler);

void uacpi_buffer_to_view(uacpi_buffer*, uacpi_data_view*);

#endif // !UACPI_BAREBONES_MODE
