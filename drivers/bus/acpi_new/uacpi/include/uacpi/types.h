#pragma once

#include <uacpi/status.h>
#include <uacpi/platform/types.h>
#include <uacpi/platform/compiler.h>
#include <uacpi/platform/arch_helpers.h>
#include <uacpi/platform/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#if UACPI_POINTER_SIZE == 4 && defined(UACPI_PHYS_ADDR_IS_32BITS)
typedef uacpi_u32 uacpi_phys_addr;
typedef uacpi_u32 uacpi_io_addr;
#else
typedef uacpi_u64 uacpi_phys_addr;
typedef uacpi_u64 uacpi_io_addr;
#endif

typedef void *uacpi_handle;

typedef union uacpi_object_name {
    uacpi_char text[4];
    uacpi_u32 id;
} uacpi_object_name;

typedef enum uacpi_iteration_decision {
    UACPI_ITERATION_DECISION_CONTINUE = 0,
    UACPI_ITERATION_DECISION_BREAK,

    // Only applicable for uacpi_namespace_for_each_child
    UACPI_ITERATION_DECISION_NEXT_PEER,
} uacpi_iteration_decision;

typedef enum uacpi_address_space {
    UACPI_ADDRESS_SPACE_SYSTEM_MEMORY = 0,
    UACPI_ADDRESS_SPACE_SYSTEM_IO = 1,
    UACPI_ADDRESS_SPACE_PCI_CONFIG = 2,
    UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER = 3,
    UACPI_ADDRESS_SPACE_SMBUS = 4,
    UACPI_ADDRESS_SPACE_SYSTEM_CMOS = 5,
    UACPI_ADDRESS_SPACE_PCI_BAR_TARGET = 6,
    UACPI_ADDRESS_SPACE_IPMI = 7,
    UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO = 8,
    UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS = 9,
    UACPI_ADDRESS_SPACE_PCC = 0x0A,
    UACPI_ADDRESS_SPACE_PRM = 0x0B,
    UACPI_ADDRESS_SPACE_FFIXEDHW = 0x7F,

    // Internal type
    UACPI_ADDRESS_SPACE_TABLE_DATA = 0xDA1A,
} uacpi_address_space;
const uacpi_char *uacpi_address_space_to_string(uacpi_address_space space);

#ifndef UACPI_BAREBONES_MODE

typedef enum uacpi_init_level {
    // Reboot state, nothing is available
    UACPI_INIT_LEVEL_EARLY = 0,

    /*
     * State after a successfull call to uacpi_initialize. Table API and
     * other helpers that don't depend on the ACPI namespace may be used.
     */
    UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED = 1,

    /*
     * State after a successfull call to uacpi_namespace_load. Most API may be
     * used, namespace can be iterated, etc.
     */
    UACPI_INIT_LEVEL_NAMESPACE_LOADED = 2,

    /*
     * The final initialization stage, this is entered after the call to
     * uacpi_namespace_initialize. All API is available to use.
     */
    UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED = 3,
} uacpi_init_level;

typedef struct uacpi_pci_address {
    uacpi_u16 segment;
    uacpi_u8 bus;
    uacpi_u8 device;
    uacpi_u8 function;
} uacpi_pci_address;

typedef struct uacpi_data_view {
    union {
        uacpi_u8 *bytes;
        const uacpi_u8 *const_bytes;

        uacpi_char *text;
        const uacpi_char *const_text;

        void *data;
        const void *const_data;
    };
    uacpi_size length;
} uacpi_data_view;

typedef struct uacpi_namespace_node uacpi_namespace_node;

typedef enum uacpi_object_type {
    UACPI_OBJECT_UNINITIALIZED = 0,
    UACPI_OBJECT_INTEGER = 1,
    UACPI_OBJECT_STRING = 2,
    UACPI_OBJECT_BUFFER = 3,
    UACPI_OBJECT_PACKAGE = 4,
    UACPI_OBJECT_FIELD_UNIT = 5,
    UACPI_OBJECT_DEVICE = 6,
    UACPI_OBJECT_EVENT = 7,
    UACPI_OBJECT_METHOD = 8,
    UACPI_OBJECT_MUTEX = 9,
    UACPI_OBJECT_OPERATION_REGION = 10,
    UACPI_OBJECT_POWER_RESOURCE = 11,
    UACPI_OBJECT_PROCESSOR = 12,
    UACPI_OBJECT_THERMAL_ZONE = 13,
    UACPI_OBJECT_BUFFER_FIELD = 14,
    UACPI_OBJECT_DEBUG = 16,

    UACPI_OBJECT_REFERENCE = 20,
    UACPI_OBJECT_BUFFER_INDEX = 21,
    UACPI_OBJECT_MAX_TYPE_VALUE = UACPI_OBJECT_BUFFER_INDEX
} uacpi_object_type;

// Type bits for API requiring a bit mask, e.g. uacpi_eval_typed
typedef enum uacpi_object_type_bits {
    UACPI_OBJECT_INTEGER_BIT = (1 << UACPI_OBJECT_INTEGER),
    UACPI_OBJECT_STRING_BIT = (1 << UACPI_OBJECT_STRING),
    UACPI_OBJECT_BUFFER_BIT = (1 << UACPI_OBJECT_BUFFER),
    UACPI_OBJECT_PACKAGE_BIT = (1 << UACPI_OBJECT_PACKAGE),
    UACPI_OBJECT_FIELD_UNIT_BIT = (1 << UACPI_OBJECT_FIELD_UNIT),
    UACPI_OBJECT_DEVICE_BIT = (1 << UACPI_OBJECT_DEVICE),
    UACPI_OBJECT_EVENT_BIT = (1 << UACPI_OBJECT_EVENT),
    UACPI_OBJECT_METHOD_BIT = (1 << UACPI_OBJECT_METHOD),
    UACPI_OBJECT_MUTEX_BIT = (1 << UACPI_OBJECT_MUTEX),
    UACPI_OBJECT_OPERATION_REGION_BIT = (1 << UACPI_OBJECT_OPERATION_REGION),
    UACPI_OBJECT_POWER_RESOURCE_BIT = (1 << UACPI_OBJECT_POWER_RESOURCE),
    UACPI_OBJECT_PROCESSOR_BIT = (1 << UACPI_OBJECT_PROCESSOR),
    UACPI_OBJECT_THERMAL_ZONE_BIT = (1 << UACPI_OBJECT_THERMAL_ZONE),
    UACPI_OBJECT_BUFFER_FIELD_BIT = (1 << UACPI_OBJECT_BUFFER_FIELD),
    UACPI_OBJECT_DEBUG_BIT = (1 << UACPI_OBJECT_DEBUG),
    UACPI_OBJECT_REFERENCE_BIT = (1 << UACPI_OBJECT_REFERENCE),
    UACPI_OBJECT_BUFFER_INDEX_BIT = (1 << UACPI_OBJECT_BUFFER_INDEX),
    UACPI_OBJECT_ANY_BIT = 0xFFFFFFFF,
} uacpi_object_type_bits;

typedef struct uacpi_object uacpi_object;

void uacpi_object_ref(uacpi_object *obj);
void uacpi_object_unref(uacpi_object *obj);

uacpi_object_type uacpi_object_get_type(uacpi_object*);
uacpi_object_type_bits uacpi_object_get_type_bit(uacpi_object*);

/*
 * Returns UACPI_TRUE if the provided object's type matches this type.
 */
uacpi_bool uacpi_object_is(uacpi_object*, uacpi_object_type);

/*
 * Returns UACPI_TRUE if the provided object's type is one of the values
 * specified in the 'type_mask' of UACPI_OBJECT_*_BIT.
 */
uacpi_bool uacpi_object_is_one_of(
    uacpi_object*, uacpi_object_type_bits type_mask
);

const uacpi_char *uacpi_object_type_to_string(uacpi_object_type);

/*
 * Create an uninitialized object. The object can be further overwritten via
 * uacpi_object_assign_* to anything.
 */
uacpi_object *uacpi_object_create_uninitialized(void);

/*
 * Create an integer object with the value provided.
 */
uacpi_object *uacpi_object_create_integer(uacpi_u64);

typedef enum uacpi_overflow_behavior {
    UACPI_OVERFLOW_ALLOW = 0,
    UACPI_OVERFLOW_TRUNCATE,
    UACPI_OVERFLOW_DISALLOW,
} uacpi_overflow_behavior;

/*
 * Same as uacpi_object_create_integer, but introduces additional ways to
 * control what happens if the provided integer is larger than 32-bits, and the
 * AML code expects 32-bit integers.
 *
 * - UACPI_OVERFLOW_ALLOW -> do nothing, same as the vanilla helper
 * - UACPI_OVERFLOW_TRUNCATE -> truncate the integer to 32-bits if it happens to
 *                              be larger than allowed by the DSDT
 * - UACPI_OVERFLOW_DISALLOW -> fail object creation with
 *                              UACPI_STATUS_INVALID_ARGUMENT if the provided
 *                              value happens to be too large
 */
uacpi_status uacpi_object_create_integer_safe(
    uacpi_u64, uacpi_overflow_behavior, uacpi_object **out_obj
);

uacpi_status uacpi_object_assign_integer(uacpi_object*, uacpi_u64 value);
uacpi_status uacpi_object_get_integer(uacpi_object*, uacpi_u64 *out);

/*
 * Create a string/buffer object. Takes in a constant view of the data.
 *
 * NOTE: The data is copied to a separately allocated buffer and is not taken
 *       ownership of.
 */
uacpi_object *uacpi_object_create_string(uacpi_data_view);
uacpi_object *uacpi_object_create_cstring(const uacpi_char*);
uacpi_object *uacpi_object_create_buffer(uacpi_data_view);

/*
 * Returns a writable view of the data stored in the string or buffer type
 * object.
 */
uacpi_status uacpi_object_get_string_or_buffer(
    uacpi_object*, uacpi_data_view *out
);
uacpi_status uacpi_object_get_string(uacpi_object*, uacpi_data_view *out);
uacpi_status uacpi_object_get_buffer(uacpi_object*, uacpi_data_view *out);

/*
 * Returns UACPI_TRUE if the provided string object is actually an AML namepath.
 *
 * This can only be the case for package elements. If a package element is
 * specified as a path to an object in AML, it's not resolved by the interpreter
 * right away as it might not have been defined at that point yet, and is
 * instead stored as a special string object to be resolved by client code
 * when needed.
 *
 * Example usage:
 *     uacpi_namespace_node *target_node = UACPI_NULL;
 *
 *     uacpi_object *obj = UACPI_NULL;
 *     uacpi_eval(scope, path, UACPI_NULL, &obj);
 *
 *     uacpi_object_array arr;
 *     uacpi_object_get_package(obj, &arr);
 *
 *     if (uacpi_object_is_aml_namepath(arr.objects[0])) {
 *         uacpi_object_resolve_as_aml_namepath(
 *             arr.objects[0], scope, &target_node
 *         );
 *     }
 */
uacpi_bool uacpi_object_is_aml_namepath(uacpi_object*);

/*
 * Resolve an AML namepath contained in a string object.
 *
 * This is only applicable to objects that are package elements. See an
 * explanation of how this works in the comment above the declaration of
 * uacpi_object_is_aml_namepath.
 *
 * This is a shorthand for:
 *     uacpi_data_view view;
 *     uacpi_object_get_string(object, &view);
 *
 *     target_node = uacpi_namespace_node_resolve_from_aml_namepath(
 *         scope, view.text
 *     );
 */
uacpi_status uacpi_object_resolve_as_aml_namepath(
    uacpi_object*, uacpi_namespace_node *scope, uacpi_namespace_node **out_node
);

/*
 * Make the provided object a string/buffer.
 * Takes in a constant view of the data to be stored in the object.
 *
 * NOTE: The data is copied to a separately allocated buffer and is not taken
 *       ownership of.
 */
uacpi_status uacpi_object_assign_string(uacpi_object*, uacpi_data_view in);
uacpi_status uacpi_object_assign_buffer(uacpi_object*, uacpi_data_view in);

typedef struct uacpi_object_array {
    uacpi_object **objects;
    uacpi_size count;
} uacpi_object_array;

/*
 * Create a package object and store all of the objects in the array inside.
 * The array is allowed to be empty.
 *
 * NOTE: the reference count of each object is incremented before being stored
 *       in the object. Client code must remove all of the locally created
 *       references at its own discretion.
 */
uacpi_object *uacpi_object_create_package(uacpi_object_array in);

/*
 * Returns the list of objects stored in a package object.
 *
 * NOTE: the reference count of the objects stored inside is not incremented,
 *       which means destorying/overwriting the object also potentially destroys
 *       all of the objects stored inside unless the reference count is
 *       incremented by the client via uacpi_object_ref.
 */
uacpi_status uacpi_object_get_package(uacpi_object*, uacpi_object_array *out);

/*
 * Make the provided object a package and store all of the objects in the array
 * inside. The array is allowed to be empty.
 *
 * NOTE: the reference count of each object is incremented before being stored
 *       in the object. Client code must remove all of the locally created
 *       references at its own discretion.
 */
uacpi_status uacpi_object_assign_package(uacpi_object*, uacpi_object_array in);

/*
 * Create a reference object and make it point to 'child'.
 *
 * NOTE: child's reference count is incremented by one. Client code must remove
 *       all of the locally created references at its own discretion.
 */
uacpi_object *uacpi_object_create_reference(uacpi_object *child);

/*
 * Make the provided object a reference and make it point to 'child'.
 *
 * NOTE: child's reference count is incremented by one. Client code must remove
 *       all of the locally created references at its own discretion.
 */
uacpi_status uacpi_object_assign_reference(uacpi_object*, uacpi_object *child);

/*
 * Retrieve the object pointed to by a reference object.
 *
 * NOTE: the reference count of the returned object is incremented by one and
 *       must be uacpi_object_unref'ed by the client when no longer needed.
 */
uacpi_status uacpi_object_get_dereferenced(uacpi_object*, uacpi_object **out);

typedef struct uacpi_processor_info {
    uacpi_u8 id;
    uacpi_u32 block_address;
    uacpi_u8 block_length;
} uacpi_processor_info;

/*
 * Returns the information about the provided processor object.
 */
uacpi_status uacpi_object_get_processor_info(
    uacpi_object*, uacpi_processor_info *out
);

typedef struct uacpi_power_resource_info {
    uacpi_u8 system_level;
    uacpi_u16 resource_order;
} uacpi_power_resource_info;

/*
 * Returns the information about the provided power resource object.
 */
uacpi_status uacpi_object_get_power_resource_info(
    uacpi_object*, uacpi_power_resource_info *out
);

typedef enum uacpi_region_op {
    // data => uacpi_region_attach_data
    UACPI_REGION_OP_ATTACH = 0,
    // data => uacpi_region_detach_data
    UACPI_REGION_OP_DETACH,

    // data => uacpi_region_rw_data
    UACPI_REGION_OP_READ,
    UACPI_REGION_OP_WRITE,

    // data => uacpi_region_pcc_send_data
    UACPI_REGION_OP_PCC_SEND,

    // data => uacpi_region_gpio_rw_data
    UACPI_REGION_OP_GPIO_READ,
    UACPI_REGION_OP_GPIO_WRITE,

    // data => uacpi_region_ipmi_rw_data
    UACPI_REGION_OP_IPMI_COMMAND,

    // data => uacpi_region_ffixedhw_rw_data
    UACPI_REGION_OP_FFIXEDHW_COMMAND,

    // data => uacpi_region_prm_rw_data
    UACPI_REGION_OP_PRM_COMMAND,

    // data => uacpi_region_serial_rw_data
    UACPI_REGION_OP_SERIAL_READ,
    UACPI_REGION_OP_SERIAL_WRITE,
} uacpi_region_op;

typedef struct uacpi_generic_region_info {
    uacpi_u64 base;
    uacpi_u64 length;
} uacpi_generic_region_info;

typedef struct uacpi_pcc_region_info {
    uacpi_data_view buffer;
    uacpi_u8 subspace_id;
} uacpi_pcc_region_info;

typedef struct uacpi_gpio_region_info
{
    uacpi_u64 num_pins;
} uacpi_gpio_region_info;

typedef struct uacpi_region_attach_data {
    void *handler_context;
    uacpi_namespace_node *region_node;
    union {
        uacpi_generic_region_info generic_info;
        uacpi_pcc_region_info pcc_info;
        uacpi_gpio_region_info gpio_info;
    };
    void *out_region_context;
} uacpi_region_attach_data;

typedef struct uacpi_region_rw_data {
    void *handler_context;
    void *region_context;
    union {
        uacpi_phys_addr address;
        uacpi_u64 offset;
    };
    uacpi_u64 value;
    uacpi_u8 byte_width;
} uacpi_region_rw_data;

typedef struct uacpi_region_pcc_send_data {
    void *handler_context;
    void *region_context;
    uacpi_data_view buffer;
} uacpi_region_pcc_send_data;

typedef struct uacpi_region_gpio_rw_data
{
    void *handler_context;
    void *region_context;
    uacpi_data_view connection;
    uacpi_u32 pin_offset;
    uacpi_u32 num_pins;
    uacpi_u64 value;
} uacpi_region_gpio_rw_data;

typedef struct uacpi_region_ipmi_rw_data
{
    void *handler_context;
    void *region_context;
    uacpi_data_view in_out_message;
    uacpi_u64 command;
} uacpi_region_ipmi_rw_data;

typedef uacpi_region_ipmi_rw_data uacpi_region_ffixedhw_rw_data;

typedef struct uacpi_region_prm_rw_data
{
    void *handler_context;
    void *region_context;
    uacpi_data_view in_out_message;
} uacpi_region_prm_rw_data;

typedef enum uacpi_access_attribute {
    UACPI_ACCESS_ATTRIBUTE_QUICK = 0x02,
    UACPI_ACCESS_ATTRIBUTE_SEND_RECEIVE = 0x04,
    UACPI_ACCESS_ATTRIBUTE_BYTE = 0x06,
    UACPI_ACCESS_ATTRIBUTE_WORD = 0x08,
    UACPI_ACCESS_ATTRIBUTE_BLOCK = 0x0A,
    UACPI_ACCESS_ATTRIBUTE_BYTES = 0x0B,
    UACPI_ACCESS_ATTRIBUTE_PROCESS_CALL = 0x0C,
    UACPI_ACCESS_ATTRIBUTE_BLOCK_PROCESS_CALL = 0x0D,
    UACPI_ACCESS_ATTRIBUTE_RAW_BYTES = 0x0E,
    UACPI_ACCESS_ATTRIBUTE_RAW_PROCESS_BYTES = 0x0F,
} uacpi_access_attribute;

typedef struct uacpi_region_serial_rw_data {
    void *handler_context;
    void *region_context;
    uacpi_u64 command;
    uacpi_data_view connection;
    uacpi_data_view in_out_buffer;
    uacpi_access_attribute access_attribute;

    /*
     * Applicable if access_attribute is one of:
     * - UACPI_ACCESS_ATTRIBUTE_BYTES
     * - UACPI_ACCESS_ATTRIBUTE_RAW_BYTES
     * - UACPI_ACCESS_ATTRIBUTE_RAW_PROCESS_BYTES
     */
    uacpi_u8 access_length;
} uacpi_region_serial_rw_data;

typedef struct uacpi_region_detach_data {
    void *handler_context;
    void *region_context;
    uacpi_namespace_node *region_node;
} uacpi_region_detach_data;

typedef uacpi_status (*uacpi_region_handler)
    (uacpi_region_op op, uacpi_handle op_data);

typedef uacpi_status (*uacpi_notify_handler)
    (uacpi_handle context, uacpi_namespace_node *node, uacpi_u64 value);

typedef enum uacpi_firmware_request_type {
    UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT,
    UACPI_FIRMWARE_REQUEST_TYPE_FATAL,
} uacpi_firmware_request_type;

typedef struct uacpi_firmware_request {
    uacpi_u8 type;

    union {
        // UACPI_FIRMWARE_REQUEST_BREAKPOINT
        struct {
            // The context of the method currently being executed
            uacpi_handle ctx;
        } breakpoint;

        // UACPI_FIRMWARE_REQUEST_FATAL
        struct {
            uacpi_u8 type;
            uacpi_u32 code;
            uacpi_u64 arg;
        } fatal;
    };
} uacpi_firmware_request;

#define UACPI_INTERRUPT_NOT_HANDLED 0
#define UACPI_INTERRUPT_HANDLED 1
typedef uacpi_u32 uacpi_interrupt_ret;

typedef uacpi_interrupt_ret (*uacpi_interrupt_handler)(uacpi_handle);

#endif // !UACPI_BAREBONES_MODE

#ifdef __cplusplus
}
#endif
