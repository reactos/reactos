#include <uacpi/types.h>
#include <uacpi/internal/types.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/shareable.h>
#include <uacpi/internal/dynamic_array.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/tables.h>
#include <uacpi/kernel_api.h>

const uacpi_char *uacpi_address_space_to_string(
    enum uacpi_address_space space
)
{
    switch (space) {
    case UACPI_ADDRESS_SPACE_SYSTEM_MEMORY:
        return "SystemMemory";
    case UACPI_ADDRESS_SPACE_SYSTEM_IO:
        return "SystemIO";
    case UACPI_ADDRESS_SPACE_PCI_CONFIG:
        return "PCI_Config";
    case UACPI_ADDRESS_SPACE_EMBEDDED_CONTROLLER:
        return "EmbeddedControl";
    case UACPI_ADDRESS_SPACE_SMBUS:
        return "SMBus";
    case UACPI_ADDRESS_SPACE_SYSTEM_CMOS:
        return "SystemCMOS";
    case UACPI_ADDRESS_SPACE_PCI_BAR_TARGET:
        return "PciBarTarget";
    case UACPI_ADDRESS_SPACE_IPMI:
        return "IPMI";
    case UACPI_ADDRESS_SPACE_GENERAL_PURPOSE_IO:
        return "GeneralPurposeIO";
    case UACPI_ADDRESS_SPACE_GENERIC_SERIAL_BUS:
        return "GenericSerialBus";
    case UACPI_ADDRESS_SPACE_PCC:
        return "PCC";
    case UACPI_ADDRESS_SPACE_PRM:
        return "PlatformRtMechanism";
    case UACPI_ADDRESS_SPACE_FFIXEDHW:
        return "FFixedHW";
    case UACPI_ADDRESS_SPACE_TABLE_DATA:
        return "TableData";
    default:
        return "<vendor specific>";
    }
}

#ifndef UACPI_BAREBONES_MODE

const uacpi_char *uacpi_object_type_to_string(uacpi_object_type type)
{
    switch (type) {
    case UACPI_OBJECT_UNINITIALIZED:
        return "Uninitialized";
    case UACPI_OBJECT_INTEGER:
        return "Integer";
    case UACPI_OBJECT_STRING:
        return "String";
    case UACPI_OBJECT_BUFFER:
        return "Buffer";
    case UACPI_OBJECT_PACKAGE:
        return "Package";
    case UACPI_OBJECT_FIELD_UNIT:
        return "Field Unit";
    case UACPI_OBJECT_DEVICE:
        return "Device";
    case UACPI_OBJECT_EVENT:
        return "Event";
    case UACPI_OBJECT_REFERENCE:
        return "Reference";
    case UACPI_OBJECT_BUFFER_INDEX:
        return "Buffer Index";
    case UACPI_OBJECT_METHOD:
        return "Method";
    case UACPI_OBJECT_MUTEX:
        return "Mutex";
    case UACPI_OBJECT_OPERATION_REGION:
        return "Operation Region";
    case UACPI_OBJECT_POWER_RESOURCE:
        return "Power Resource";
    case UACPI_OBJECT_PROCESSOR:
        return "Processor";
    case UACPI_OBJECT_THERMAL_ZONE:
        return "Thermal Zone";
    case UACPI_OBJECT_BUFFER_FIELD:
        return "Buffer Field";
    case UACPI_OBJECT_DEBUG:
        return "Debug";
    default:
        return "<Invalid type>";
    }
}

static uacpi_bool buffer_alloc(uacpi_object *obj, uacpi_size initial_size)
{
    uacpi_buffer *buf;

    buf = uacpi_kernel_alloc_zeroed(sizeof(uacpi_buffer));
    if (uacpi_unlikely(buf == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(buf);

    if (initial_size) {
        buf->data = uacpi_kernel_alloc(initial_size);
        if (uacpi_unlikely(buf->data == UACPI_NULL)) {
            uacpi_free(buf, sizeof(*buf));
            return UACPI_FALSE;
        }

        buf->size = initial_size;
    }

    obj->buffer = buf;
    return UACPI_TRUE;
}

static uacpi_bool empty_buffer_or_string_alloc(uacpi_object *object)
{
    return buffer_alloc(object, 0);
}

uacpi_bool uacpi_package_fill(
    uacpi_package *pkg, uacpi_size num_elements,
    enum uacpi_prealloc_objects prealloc_objects
)
{
    uacpi_size i;

    if (uacpi_unlikely(num_elements == 0))
        return UACPI_TRUE;

    pkg->objects = uacpi_kernel_alloc_zeroed(
        num_elements * sizeof(uacpi_handle)
    );
    if (uacpi_unlikely(pkg->objects == UACPI_NULL))
        return UACPI_FALSE;

    pkg->count = num_elements;

    if (prealloc_objects == UACPI_PREALLOC_OBJECTS_YES) {
        for (i = 0; i < num_elements; ++i) {
            pkg->objects[i] = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);

            if (uacpi_unlikely(pkg->objects[i] == UACPI_NULL))
                return UACPI_FALSE;
        }
    }

    return UACPI_TRUE;
}

static uacpi_bool package_alloc(
    uacpi_object *obj, uacpi_size initial_size,
    enum uacpi_prealloc_objects prealloc
)
{
    uacpi_package *pkg;

    pkg = uacpi_kernel_alloc_zeroed(sizeof(uacpi_package));
    if (uacpi_unlikely(pkg == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(pkg);

    if (uacpi_unlikely(!uacpi_package_fill(pkg, initial_size, prealloc))) {
        uacpi_free(pkg, sizeof(*pkg));
        return UACPI_FALSE;
    }

    obj->package = pkg;
    return UACPI_TRUE;
}

static uacpi_bool empty_package_alloc(uacpi_object *object)
{
    return package_alloc(object, 0, UACPI_PREALLOC_OBJECTS_NO);
}

uacpi_mutex *uacpi_create_mutex(void)
{
    uacpi_mutex *mutex;

    mutex = uacpi_kernel_alloc_zeroed(sizeof(uacpi_mutex));
    if (uacpi_unlikely(mutex == UACPI_NULL))
        return UACPI_NULL;

    mutex->owner = UACPI_THREAD_ID_NONE;

    mutex->handle = uacpi_kernel_create_mutex();
    if (mutex->handle == UACPI_NULL) {
        uacpi_free(mutex, sizeof(*mutex));
        return UACPI_NULL;
    }

    uacpi_shareable_init(mutex);
    return mutex;
}

static uacpi_bool mutex_alloc(uacpi_object *obj)
{
    obj->mutex = uacpi_create_mutex();
    return obj->mutex != UACPI_NULL;
}

static uacpi_bool event_alloc(uacpi_object *obj)
{
    uacpi_event *event;

    event = uacpi_kernel_alloc_zeroed(sizeof(uacpi_event));
    if (uacpi_unlikely(event == UACPI_NULL))
        return UACPI_FALSE;

    event->handle = uacpi_kernel_create_event();
    if (event->handle == UACPI_NULL) {
        uacpi_free(event, sizeof(*event));
        return UACPI_FALSE;
    }

    uacpi_shareable_init(event);
    obj->event = event;

    return UACPI_TRUE;
}

static uacpi_bool method_alloc(uacpi_object *obj)
{
    uacpi_control_method *method;

    method = uacpi_kernel_alloc_zeroed(sizeof(*method));
    if (uacpi_unlikely(method == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(method);
    obj->method = method;

    return UACPI_TRUE;
}

static uacpi_bool op_region_alloc(uacpi_object *obj)
{
    uacpi_operation_region *op_region;

    op_region = uacpi_kernel_alloc_zeroed(sizeof(*op_region));
    if (uacpi_unlikely(op_region == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(op_region);
    obj->op_region = op_region;

    return UACPI_TRUE;
}

static uacpi_bool field_unit_alloc(uacpi_object *obj)
{
    uacpi_field_unit *field_unit;

    field_unit = uacpi_kernel_alloc_zeroed(sizeof(*field_unit));
    if (uacpi_unlikely(field_unit == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(field_unit);
    obj->field_unit = field_unit;

    return UACPI_TRUE;
}

static uacpi_bool processor_alloc(uacpi_object *obj)
{
    uacpi_processor *processor;

    processor = uacpi_kernel_alloc_zeroed(sizeof(*processor));
    if (uacpi_unlikely(processor == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(processor);
    obj->processor = processor;

    return UACPI_TRUE;
}

static uacpi_bool device_alloc(uacpi_object *obj)
{
    uacpi_device *device;

    device = uacpi_kernel_alloc_zeroed(sizeof(*device));
    if (uacpi_unlikely(device == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(device);
    obj->device = device;

    return UACPI_TRUE;
}

static uacpi_bool thermal_zone_alloc(uacpi_object *obj)
{
    uacpi_thermal_zone *thermal_zone;

    thermal_zone = uacpi_kernel_alloc_zeroed(sizeof(*thermal_zone));
    if (uacpi_unlikely(thermal_zone == UACPI_NULL))
        return UACPI_FALSE;

    uacpi_shareable_init(thermal_zone);
    obj->thermal_zone = thermal_zone;

    return UACPI_TRUE;
}

typedef uacpi_bool (*object_ctor)(uacpi_object *obj);

static object_ctor object_constructor_table[UACPI_OBJECT_MAX_TYPE_VALUE + 1] = {
    [UACPI_OBJECT_STRING] = empty_buffer_or_string_alloc,
    [UACPI_OBJECT_BUFFER] = empty_buffer_or_string_alloc,
    [UACPI_OBJECT_PACKAGE] = empty_package_alloc,
    [UACPI_OBJECT_FIELD_UNIT] = field_unit_alloc,
    [UACPI_OBJECT_MUTEX] = mutex_alloc,
    [UACPI_OBJECT_EVENT] = event_alloc,
    [UACPI_OBJECT_OPERATION_REGION] = op_region_alloc,
    [UACPI_OBJECT_METHOD] = method_alloc,
    [UACPI_OBJECT_PROCESSOR] = processor_alloc,
    [UACPI_OBJECT_DEVICE] = device_alloc,
    [UACPI_OBJECT_THERMAL_ZONE] = thermal_zone_alloc,
};

uacpi_object *uacpi_create_object(uacpi_object_type type)
{
    uacpi_object *ret;
    object_ctor ctor;

    ret = uacpi_kernel_alloc_zeroed(sizeof(*ret));
    if (uacpi_unlikely(ret == UACPI_NULL))
        return ret;

    uacpi_shareable_init(ret);
    ret->type = type;

    ctor = object_constructor_table[type];
    if (ctor == UACPI_NULL)
        return ret;

    if (uacpi_unlikely(!ctor(ret))) {
        uacpi_free(ret, sizeof(*ret));
        return UACPI_NULL;
    }

    return ret;
}

static void free_buffer(uacpi_handle handle)
{
    uacpi_buffer *buf = handle;

    if (buf->data != UACPI_NULL)
        /*
         * If buffer has a size of 0 but a valid data pointer it's probably an
         * "empty" buffer allocated by the interpreter in make_null_buffer
         * and its real size is actually 1.
         */
        uacpi_free(buf->data, UACPI_MAX(buf->size, 1));

    uacpi_free(buf, sizeof(*buf));
}

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(free_queue, uacpi_package*, 4)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(free_queue, uacpi_package*, static)

static uacpi_bool free_queue_push(struct free_queue *queue, uacpi_package *pkg)
{
    uacpi_package **slot;

    slot = free_queue_alloc(queue);
    if (uacpi_unlikely(slot == UACPI_NULL))
        return UACPI_FALSE;

    *slot = pkg;
    return UACPI_TRUE;
}

static void free_object(uacpi_object *obj);

// No references allowed here, only plain objects
static void free_plain_no_recurse(uacpi_object *obj, struct free_queue *queue)
{
    switch (obj->type) {
    case UACPI_OBJECT_PACKAGE:
        if (uacpi_shareable_unref(obj->package) > 1)
            break;

        if (uacpi_unlikely(!free_queue_push(queue,
                                            obj->package))) {
            uacpi_warn(
                "unable to free nested package @%p: not enough memory\n",
                obj->package
            );
        }

        // Don't call free_object here as that will recurse
        uacpi_free(obj, sizeof(*obj));
        break;
    default:
        /*
         * This call is guaranteed to not recurse further as we handle
         * recursive cases elsewhere explicitly.
         */
        free_object(obj);
    }
}

static void unref_plain_no_recurse(uacpi_object *obj, struct free_queue *queue)
{
    if (uacpi_shareable_unref(obj) > 1)
        return;

    free_plain_no_recurse(obj, queue);
}

static void unref_chain_no_recurse(uacpi_object *obj, struct free_queue *queue)
{
    uacpi_object *next_obj = UACPI_NULL;

    while (obj) {
        if (obj->type == UACPI_OBJECT_REFERENCE)
            next_obj = obj->inner_object;

        if (uacpi_shareable_unref(obj) > 1)
            goto do_next;

        if (obj->type == UACPI_OBJECT_REFERENCE) {
            uacpi_free(obj, sizeof(*obj));
        } else {
            free_plain_no_recurse(obj, queue);
        }

    do_next:
        obj = next_obj;
        next_obj = UACPI_NULL;
    }
}

static void unref_object_no_recurse(uacpi_object *obj, struct free_queue *queue)
{
    if (obj->type == UACPI_OBJECT_REFERENCE) {
        unref_chain_no_recurse(obj, queue);
        return;
    }

    unref_plain_no_recurse(obj, queue);
}

static void free_package(uacpi_handle handle)
{
    struct free_queue queue = { 0 };
    uacpi_package *pkg = handle;
    uacpi_object *obj;
    uacpi_size i;

    free_queue_push(&queue, pkg);

    while (free_queue_size(&queue) != 0) {
        pkg = *free_queue_last(&queue);
        free_queue_pop(&queue);

        /*
         * 1. Unref/free every object in the package. Note that this might add
         *    even more packages into the free queue.
         */
        for (i = 0; i < pkg->count; ++i) {
            obj = pkg->objects[i];
            unref_object_no_recurse(obj, &queue);
        }

        // 2. Release the object array
        uacpi_free(pkg->objects, sizeof(*pkg->objects) * pkg->count);

        // 3. Release the package itself
        uacpi_free(pkg, sizeof(*pkg));
    }

    free_queue_clear(&queue);
}

static void free_mutex(uacpi_handle handle)
{
    uacpi_mutex *mutex = handle;

    uacpi_kernel_free_mutex(mutex->handle);
    uacpi_free(mutex, sizeof(*mutex));
}

void uacpi_mutex_unref(uacpi_mutex *mutex)
{
    if (mutex == UACPI_NULL)
        return;

    uacpi_shareable_unref_and_delete_if_last(mutex, free_mutex);
}

static void free_event(uacpi_handle handle)
{
    uacpi_event *event = handle;

    uacpi_kernel_free_event(event->handle);
    uacpi_free(event, sizeof(*event));
}

static void free_address_space_handler(uacpi_handle handle)
{
    uacpi_address_space_handler *handler = handle;
    uacpi_free(handler, sizeof(*handler));
}

static void free_address_space_handlers(
    uacpi_address_space_handler *handler
)
{
    uacpi_address_space_handler *next_handler;

    while (handler) {
        next_handler = handler->next;
        uacpi_shareable_unref_and_delete_if_last(
            handler, free_address_space_handler
        );
        handler = next_handler;
    }
}

static void free_device_notify_handlers(uacpi_device_notify_handler *handler)
{
    uacpi_device_notify_handler *next_handler;

    while (handler) {
        next_handler = handler->next;
        uacpi_free(handler, sizeof(*handler));
        handler = next_handler;
    }
}

static void free_handlers(uacpi_handle handle)
{
    uacpi_handlers *handlers = handle;

    free_address_space_handlers(handlers->address_space_head);
    free_device_notify_handlers(handlers->notify_head);
}

void uacpi_address_space_handler_unref(uacpi_address_space_handler *handler)
{
    uacpi_shareable_unref_and_delete_if_last(
        handler, free_address_space_handler
    );
}

static void free_op_region(uacpi_handle handle)
{
    uacpi_operation_region *op_region = handle;

    if (uacpi_unlikely(op_region->handler != UACPI_NULL)) {
        uacpi_warn(
            "BUG: attempting to free an opregion@%p with a handler attached\n",
            op_region
        );
    }

    switch (op_region->space) {
    case UACPI_ADDRESS_SPACE_PCC:
        uacpi_free(op_region->internal_buffer, op_region->length);
        break;
    case UACPI_ADDRESS_SPACE_TABLE_DATA:
        uacpi_table_unref(
            &(struct uacpi_table) { .index = op_region->table_idx }
        );
        break;
    default:
        break;
    }

    uacpi_free(op_region, sizeof(*op_region));
}

static void free_device(uacpi_handle handle)
{
    uacpi_device *device = handle;
    free_handlers(device);
    uacpi_free(device, sizeof(*device));
}

static void free_processor(uacpi_handle handle)
{
    uacpi_processor *processor = handle;
    free_handlers(processor);
    uacpi_free(processor, sizeof(*processor));
}

static void free_thermal_zone(uacpi_handle handle)
{
    uacpi_thermal_zone *thermal_zone = handle;
    free_handlers(thermal_zone);
    uacpi_free(thermal_zone, sizeof(*thermal_zone));
}

static void free_field_unit(uacpi_handle handle)
{
    uacpi_field_unit *field_unit = handle;

    if (field_unit->connection)
        uacpi_object_unref(field_unit->connection);

    switch (field_unit->kind) {
    case UACPI_FIELD_UNIT_KIND_NORMAL:
        uacpi_namespace_node_unref(field_unit->region);
        break;
    case UACPI_FIELD_UNIT_KIND_BANK:
        uacpi_namespace_node_unref(field_unit->bank_region);
        uacpi_shareable_unref_and_delete_if_last(
            field_unit->bank_selection, free_field_unit
        );
        break;
    case UACPI_FIELD_UNIT_KIND_INDEX:
        uacpi_shareable_unref_and_delete_if_last(
            field_unit->index, free_field_unit
        );
        uacpi_shareable_unref_and_delete_if_last(
            field_unit->data, free_field_unit
        );
        break;
    default:
        break;
    }

    uacpi_free(field_unit, sizeof(*field_unit));
}

static void free_method(uacpi_handle handle)
{
    uacpi_control_method *method = handle;

    uacpi_shareable_unref_and_delete_if_last(
        method->mutex, free_mutex
    );

    if (!method->native_call && method->owns_code)
       uacpi_free(method->code, method->size);
    uacpi_free(method, sizeof(*method));
}

void uacpi_method_unref(uacpi_control_method *method)
{
    uacpi_shareable_unref_and_delete_if_last(method, free_method);
}

static void free_object_storage(uacpi_object *obj)
{
    switch (obj->type) {
    case UACPI_OBJECT_STRING:
    case UACPI_OBJECT_BUFFER:
        uacpi_shareable_unref_and_delete_if_last(obj->buffer, free_buffer);
        break;
    case UACPI_OBJECT_BUFFER_FIELD:
        uacpi_shareable_unref_and_delete_if_last(obj->buffer_field.backing,
                                                 free_buffer);
        break;
    case UACPI_OBJECT_BUFFER_INDEX:
        uacpi_shareable_unref_and_delete_if_last(obj->buffer_index.buffer,
                                                 free_buffer);
        break;
    case UACPI_OBJECT_METHOD:
        uacpi_method_unref(obj->method);
        break;
    case UACPI_OBJECT_PACKAGE:
        uacpi_shareable_unref_and_delete_if_last(obj->package,
                                                 free_package);
        break;
    case UACPI_OBJECT_FIELD_UNIT:
        uacpi_shareable_unref_and_delete_if_last(obj->field_unit,
                                                 free_field_unit);
        break;
    case UACPI_OBJECT_MUTEX:
        uacpi_mutex_unref(obj->mutex);
        break;
    case UACPI_OBJECT_EVENT:
        uacpi_shareable_unref_and_delete_if_last(obj->event,
                                                 free_event);
        break;
    case UACPI_OBJECT_OPERATION_REGION:
        uacpi_shareable_unref_and_delete_if_last(obj->op_region,
                                                 free_op_region);
        break;
    case UACPI_OBJECT_PROCESSOR:
        uacpi_shareable_unref_and_delete_if_last(obj->processor,
                                                 free_processor);
        break;
    case UACPI_OBJECT_DEVICE:
        uacpi_shareable_unref_and_delete_if_last(obj->device,
                                                 free_device);
        break;
    case UACPI_OBJECT_THERMAL_ZONE:
        uacpi_shareable_unref_and_delete_if_last(obj->thermal_zone,
                                                 free_thermal_zone);
        break;
    default:
        break;
    }
}

static void free_object(uacpi_object *obj)
{
    free_object_storage(obj);
    uacpi_free(obj, sizeof(*obj));
}

static void make_chain_bugged(uacpi_object *obj)
{
    uacpi_warn("object refcount bug, marking chain @%p as bugged\n", obj);

    while (obj) {
        uacpi_make_shareable_bugged(obj);

        if (obj->type == UACPI_OBJECT_REFERENCE)
            obj = obj->inner_object;
        else
            obj = UACPI_NULL;
    }
}

void uacpi_object_ref(uacpi_object *obj)
{
    while (obj) {
        uacpi_shareable_ref(obj);

        if (obj->type == UACPI_OBJECT_REFERENCE)
            obj = obj->inner_object;
        else
            obj = UACPI_NULL;
    }
}

static void free_chain(uacpi_object *obj)
{
    uacpi_object *next_obj = UACPI_NULL;

    while (obj) {
        if (obj->type == UACPI_OBJECT_REFERENCE)
            next_obj = obj->inner_object;

        if (uacpi_shareable_refcount(obj) == 0)
            free_object(obj);

        obj = next_obj;
        next_obj = UACPI_NULL;
    }
}

void uacpi_object_unref(uacpi_object *obj)
{
    uacpi_object *this_obj = obj;

    if (!obj)
        return;

    while (obj) {
        if (uacpi_unlikely(uacpi_bugged_shareable(obj)))
            return;

        uacpi_shareable_unref(obj);

        if (obj->type == UACPI_OBJECT_REFERENCE) {
            obj = obj->inner_object;
        } else {
            obj = UACPI_NULL;
        }
    }

    if (uacpi_shareable_refcount(this_obj) == 0)
        free_chain(this_obj);
}

static uacpi_status buffer_alloc_and_store(
    uacpi_object *obj, uacpi_size buf_size,
    const void *src, uacpi_size src_size
)
{
    if (uacpi_unlikely(!buffer_alloc(obj, buf_size)))
        return UACPI_STATUS_OUT_OF_MEMORY;

    uacpi_memcpy_zerout(obj->buffer->data, src, buf_size, src_size);
    return UACPI_STATUS_OK;
}

static uacpi_status assign_buffer(uacpi_object *dst, uacpi_object *src,
                                  enum uacpi_assign_behavior behavior)
{
    if (behavior == UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY) {
        dst->buffer = src->buffer;
        uacpi_shareable_ref(dst->buffer);
        return UACPI_STATUS_OK;
    }

    return buffer_alloc_and_store(dst, src->buffer->size,
                                  src->buffer->data, src->buffer->size);
}

struct pkg_copy_req {
    uacpi_object *dst;
    uacpi_package *src;
};

DYNAMIC_ARRAY_WITH_INLINE_STORAGE(pkg_copy_reqs, struct pkg_copy_req, 2)
DYNAMIC_ARRAY_WITH_INLINE_STORAGE_IMPL(
    pkg_copy_reqs, struct pkg_copy_req, static
)

static uacpi_bool pkg_copy_reqs_push(
    struct pkg_copy_reqs *reqs,
    uacpi_object *dst, uacpi_package *pkg
)
{
    struct pkg_copy_req *req;

    req = pkg_copy_reqs_alloc(reqs);
    if (uacpi_unlikely(req == UACPI_NULL))
        return UACPI_FALSE;

    req->dst = dst;
    req->src = pkg;

    return UACPI_TRUE;
}

static uacpi_status deep_copy_package_no_recurse(
    uacpi_object *dst, uacpi_package *src,
    struct pkg_copy_reqs *reqs
)
{
    uacpi_size i;
    uacpi_package *dst_package;

    if (uacpi_unlikely(!package_alloc(dst, src->count,
                                      UACPI_PREALLOC_OBJECTS_YES)))
        return UACPI_STATUS_OUT_OF_MEMORY;

    dst->type = UACPI_OBJECT_PACKAGE;
    dst_package = dst->package;

    for (i = 0; i < src->count; ++i) {
        uacpi_status st;
        uacpi_object *src_obj = src->objects[i];
        uacpi_object *dst_obj = dst_package->objects[i];

        // Don't copy the internal package index reference
        if (src_obj->type == UACPI_OBJECT_REFERENCE &&
            src_obj->flags == UACPI_REFERENCE_KIND_PKG_INDEX)
            src_obj = src_obj->inner_object;

        if (src_obj->type == UACPI_OBJECT_PACKAGE) {
            uacpi_bool ret;

            ret = pkg_copy_reqs_push(reqs, dst_obj, src_obj->package);
            if (uacpi_unlikely(!ret))
                return UACPI_STATUS_OUT_OF_MEMORY;

            continue;
        }

        st = uacpi_object_assign(dst_obj, src_obj,
                                 UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
        if (uacpi_unlikely_error(st))
            return st;
    }

    return UACPI_STATUS_OK;
}

static uacpi_status deep_copy_package(uacpi_object *dst, uacpi_object *src)
{
    uacpi_status ret = UACPI_STATUS_OK;
    struct pkg_copy_reqs reqs = { 0 };

    pkg_copy_reqs_push(&reqs, dst, src->package);

    while (pkg_copy_reqs_size(&reqs) != 0) {
        struct pkg_copy_req req;

        req = *pkg_copy_reqs_last(&reqs);
        pkg_copy_reqs_pop(&reqs);

        ret = deep_copy_package_no_recurse(req.dst, req.src, &reqs);
        if (uacpi_unlikely_error(ret))
            break;
    }

    pkg_copy_reqs_clear(&reqs);
    return ret;
}

static uacpi_status assign_mutex(uacpi_object *dst, uacpi_object *src,
                                 enum uacpi_assign_behavior behavior)
{
    if (behavior == UACPI_ASSIGN_BEHAVIOR_DEEP_COPY) {
        if (uacpi_likely(mutex_alloc(dst))) {
            dst->mutex->sync_level = src->mutex->sync_level;
            return UACPI_STATUS_OK;
        }

        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    dst->mutex = src->mutex;
    uacpi_shareable_ref(dst->mutex);

    return UACPI_STATUS_OK;
}

static uacpi_status assign_event(uacpi_object *dst, uacpi_object *src,
                                 enum uacpi_assign_behavior behavior)
{
    if (behavior == UACPI_ASSIGN_BEHAVIOR_DEEP_COPY) {
        if (uacpi_likely(event_alloc(dst)))
            return UACPI_STATUS_OK;

        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    dst->event = src->event;
    uacpi_shareable_ref(dst->event);

    return UACPI_STATUS_OK;
}

static uacpi_status assign_package(uacpi_object *dst, uacpi_object *src,
                                   enum uacpi_assign_behavior behavior)
{
    if (behavior == UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY) {
        dst->package = src->package;
        uacpi_shareable_ref(dst->package);
        return UACPI_STATUS_OK;
    }

    return deep_copy_package(dst, src);
}

void uacpi_object_attach_child(uacpi_object *parent, uacpi_object *child)
{
    uacpi_u32 refs_to_add;

    parent->inner_object = child;

    if (uacpi_unlikely(uacpi_bugged_shareable(parent))) {
        make_chain_bugged(child);
        return;
    }

    refs_to_add = uacpi_shareable_refcount(parent);
    while (refs_to_add--)
        uacpi_object_ref(child);
}

void uacpi_object_detach_child(uacpi_object *parent)
{
    uacpi_u32 refs_to_remove;
    uacpi_object *child;

    child = parent->inner_object;
    parent->inner_object = UACPI_NULL;

    if (uacpi_unlikely(uacpi_bugged_shareable(parent)))
        return;

    refs_to_remove = uacpi_shareable_refcount(parent);
    while (refs_to_remove--)
        uacpi_object_unref(child);
}

uacpi_object_type uacpi_object_get_type(uacpi_object *obj)
{
    return obj->type;
}

uacpi_object_type_bits uacpi_object_get_type_bit(uacpi_object *obj)
{
    return (1u << obj->type);
}

uacpi_bool uacpi_object_is(uacpi_object *obj, uacpi_object_type type)
{
    return obj->type == type;
}

uacpi_bool uacpi_object_is_one_of(
    uacpi_object *obj, uacpi_object_type_bits type_mask
)
{
    return (uacpi_object_get_type_bit(obj) & type_mask) != 0;
}

#define TYPE_CHECK_USER_OBJ_RET(obj, type_bits, ret)                 \
    do {                                                             \
        if (uacpi_unlikely(obj == UACPI_NULL ||                      \
                           !uacpi_object_is_one_of(obj, type_bits))) \
            return ret;                                              \
    } while (0)

#define TYPE_CHECK_USER_OBJ(obj, type_bits)                               \
    TYPE_CHECK_USER_OBJ_RET(obj, type_bits, UACPI_STATUS_INVALID_ARGUMENT)

#define ENSURE_VALID_USER_OBJ_RET(obj, ret)     \
    do {                                        \
        if (uacpi_unlikely(obj == UACPI_NULL))  \
            return ret;                         \
    } while (0)

#define ENSURE_VALID_USER_OBJ(obj)                               \
    ENSURE_VALID_USER_OBJ_RET(obj, UACPI_STATUS_INVALID_ARGUMENT)

uacpi_status uacpi_object_get_integer(uacpi_object *obj, uacpi_u64 *out)
{
    TYPE_CHECK_USER_OBJ(obj, UACPI_OBJECT_INTEGER_BIT);

    *out = obj->integer;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_assign_integer(uacpi_object *obj, uacpi_u64 value)
{
    ENSURE_VALID_USER_OBJ(obj);

    return uacpi_object_assign(obj, &(uacpi_object) {
        .type = UACPI_OBJECT_INTEGER,
        .integer = value,
    }, UACPI_ASSIGN_BEHAVIOR_DEEP_COPY);
}

void uacpi_buffer_to_view(uacpi_buffer *buf, uacpi_data_view *out_view)
{
    out_view->bytes = buf->byte_data;
    out_view->length = buf->size;
}

static uacpi_status uacpi_object_do_get_string_or_buffer(
    uacpi_object *obj, uacpi_data_view *out, uacpi_u32 mask
)
{
    TYPE_CHECK_USER_OBJ(obj, mask);

    uacpi_buffer_to_view(obj->buffer, out);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_get_string_or_buffer(
    uacpi_object *obj, uacpi_data_view *out
)
{
    return uacpi_object_do_get_string_or_buffer(
        obj, out, UACPI_OBJECT_STRING_BIT | UACPI_OBJECT_BUFFER_BIT
    );
}

uacpi_status uacpi_object_get_string(uacpi_object *obj, uacpi_data_view *out)
{
    return uacpi_object_do_get_string_or_buffer(
        obj, out, UACPI_OBJECT_STRING_BIT
    );
}

uacpi_status uacpi_object_get_buffer(uacpi_object *obj, uacpi_data_view *out)
{
    return uacpi_object_do_get_string_or_buffer(
        obj, out, UACPI_OBJECT_BUFFER_BIT
    );
}

uacpi_bool uacpi_object_is_aml_namepath(uacpi_object *obj)
{
    TYPE_CHECK_USER_OBJ_RET(obj, UACPI_OBJECT_STRING_BIT, UACPI_FALSE);
    return obj->flags == UACPI_STRING_KIND_PATH;
}

uacpi_status uacpi_object_resolve_as_aml_namepath(
    uacpi_object *obj, uacpi_namespace_node *scope,
    uacpi_namespace_node **out_node
)
{
    uacpi_status ret;
    uacpi_namespace_node *node;

    if (!uacpi_object_is_aml_namepath(obj))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_namespace_node_resolve_from_aml_namepath(
        scope, obj->buffer->text, &node
    );
    if (uacpi_likely_success(ret))
        *out_node = node;
    return ret;
}

static uacpi_status uacpi_object_do_assign_buffer(
    uacpi_object *obj, uacpi_data_view in, uacpi_object_type type
)
{
    uacpi_status ret;
    uacpi_object tmp_obj = {
        .type = type,
    };
    uacpi_size dst_buf_size = in.length;

    ENSURE_VALID_USER_OBJ(obj);

    if (type == UACPI_OBJECT_STRING && (in.length == 0 ||
        in.const_bytes[in.length - 1] != 0x00))
        dst_buf_size++;

    ret = buffer_alloc_and_store(
        &tmp_obj, dst_buf_size, in.const_bytes, in.length
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_object_assign(
        obj, &tmp_obj, UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY
    );
    uacpi_shareable_unref_and_delete_if_last(tmp_obj.buffer, free_buffer);

    return ret;
}

uacpi_status uacpi_object_assign_string(uacpi_object *obj, uacpi_data_view in)
{
    return uacpi_object_do_assign_buffer(obj, in, UACPI_OBJECT_STRING);
}

uacpi_status uacpi_object_assign_buffer(uacpi_object *obj, uacpi_data_view in)
{
    return uacpi_object_do_assign_buffer(obj, in, UACPI_OBJECT_BUFFER);
}

uacpi_object *uacpi_object_create_uninitialized(void)
{
    return uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
}

uacpi_status uacpi_object_create_integer_safe(
    uacpi_u64 value, uacpi_overflow_behavior behavior, uacpi_object **out_obj
)
{
    uacpi_status ret;
    uacpi_u8 bitness;
    uacpi_object *obj;

    ret = uacpi_get_aml_bitness(&bitness);
    if (uacpi_unlikely_error(ret))
        return ret;

    switch (behavior) {
    case UACPI_OVERFLOW_TRUNCATE:
    case UACPI_OVERFLOW_DISALLOW:
        if (bitness == 32 && value > 0xFFFFFFFF) {
            if (behavior == UACPI_OVERFLOW_DISALLOW)
                return UACPI_STATUS_INVALID_ARGUMENT;

            value &= 0xFFFFFFFF;
        }
        UACPI_FALLTHROUGH;
    case UACPI_OVERFLOW_ALLOW:
        obj = uacpi_object_create_integer(value);
        if (uacpi_unlikely(obj == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        *out_obj = obj;
        return ret;
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
}

uacpi_object *uacpi_object_create_integer(uacpi_u64 value)
{
    uacpi_object *obj;

    obj = uacpi_create_object(UACPI_OBJECT_INTEGER);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return obj;

    obj->integer = value;
    return obj;
}

static uacpi_object *uacpi_object_do_create_string_or_buffer(
    uacpi_data_view view, uacpi_object_type type
)
{
    uacpi_status ret;
    uacpi_object *obj;

    obj = uacpi_create_object(UACPI_OBJECT_UNINITIALIZED);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_NULL;

    ret = uacpi_object_do_assign_buffer(obj, view, type);
    if (uacpi_unlikely_error(ret)) {
        uacpi_object_unref(obj);
        return UACPI_NULL;
    }

    return obj;
}

uacpi_object *uacpi_object_create_string(uacpi_data_view view)
{
    return uacpi_object_do_create_string_or_buffer(view, UACPI_OBJECT_STRING);
}

uacpi_object *uacpi_object_create_buffer(uacpi_data_view view)
{
    return uacpi_object_do_create_string_or_buffer(view, UACPI_OBJECT_BUFFER);
}

uacpi_object *uacpi_object_create_cstring(const uacpi_char *str)
{
    return uacpi_object_create_string((uacpi_data_view) {
        .const_text = str,
        .length = uacpi_strlen(str) + 1,
    });
}

uacpi_status uacpi_object_get_package(
    uacpi_object *obj, uacpi_object_array *out
)
{
    TYPE_CHECK_USER_OBJ(obj, UACPI_OBJECT_PACKAGE_BIT);

    out->objects = obj->package->objects;
    out->count = obj->package->count;
    return UACPI_STATUS_OK;
}

uacpi_object *uacpi_object_create_reference(uacpi_object *child)
{
    uacpi_object *obj;

    ENSURE_VALID_USER_OBJ_RET(child, UACPI_NULL);

    obj = uacpi_create_object(UACPI_OBJECT_REFERENCE);
    if (uacpi_unlikely(obj == UACPI_NULL))
        return UACPI_NULL;

    uacpi_object_attach_child(obj, child);
    obj->flags = UACPI_REFERENCE_KIND_ARG;

    return obj;
}

uacpi_status uacpi_object_assign_reference(
    uacpi_object *obj, uacpi_object *child
)
{
    uacpi_status ret;

    ENSURE_VALID_USER_OBJ(obj);
    ENSURE_VALID_USER_OBJ(child);

    // First clear out the object
    ret = uacpi_object_assign(
        obj, &(uacpi_object) { .type = UACPI_OBJECT_UNINITIALIZED },
        UACPI_ASSIGN_BEHAVIOR_DEEP_COPY
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    obj->type = UACPI_OBJECT_REFERENCE;
    uacpi_object_attach_child(obj, child);
    obj->flags = UACPI_REFERENCE_KIND_ARG;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_get_dereferenced(
    uacpi_object *obj, uacpi_object **out
)
{
    TYPE_CHECK_USER_OBJ(obj, UACPI_OBJECT_REFERENCE_BIT);

    *out = obj->inner_object;
    uacpi_shareable_ref(*out);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_get_processor_info(
    uacpi_object *obj, uacpi_processor_info *out
)
{
    TYPE_CHECK_USER_OBJ(obj, UACPI_OBJECT_PROCESSOR_BIT);

    out->id = obj->processor->id;
    out->block_address = obj->processor->block_address;
    out->block_length = obj->processor->block_length;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_get_power_resource_info(
    uacpi_object *obj, uacpi_power_resource_info *out
)
{
    TYPE_CHECK_USER_OBJ(obj, UACPI_OBJECT_POWER_RESOURCE_BIT);

    out->system_level = obj->power_resource.system_level;
    out->resource_order = obj->power_resource.resource_order;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_object_assign_package(
    uacpi_object *obj, uacpi_object_array in
)
{
    uacpi_status ret;
    uacpi_size i;
    uacpi_object tmp_obj = {
        .type = UACPI_OBJECT_PACKAGE,
    };

    ENSURE_VALID_USER_OBJ(obj);

    if (uacpi_unlikely(!package_alloc(&tmp_obj, in.count,
                                      UACPI_PREALLOC_OBJECTS_NO)))
        return UACPI_STATUS_OUT_OF_MEMORY;

    obj->type = UACPI_OBJECT_PACKAGE;

    for (i = 0; i < in.count; ++i) {
        tmp_obj.package->objects[i] = in.objects[i];
        uacpi_object_ref(tmp_obj.package->objects[i]);
    }

    ret = uacpi_object_assign(obj, &tmp_obj, UACPI_ASSIGN_BEHAVIOR_SHALLOW_COPY);
    uacpi_shareable_unref_and_delete_if_last(tmp_obj.package, free_package);

    return ret;
}

uacpi_object *uacpi_object_create_package(uacpi_object_array in)
{
    uacpi_status ret;
    uacpi_object *obj;

    obj = uacpi_object_create_uninitialized();
    if (uacpi_unlikely(obj == UACPI_NULL))
        return obj;

    ret = uacpi_object_assign_package(obj, in);
    if (uacpi_unlikely_error(ret)) {
        uacpi_object_unref(obj);
        return UACPI_NULL;
    }

    return obj;
}

uacpi_status uacpi_object_assign(uacpi_object *dst, uacpi_object *src,
                                 enum uacpi_assign_behavior behavior)
{
    uacpi_status ret = UACPI_STATUS_OK;

    if (src == dst)
        return ret;

    switch (dst->type) {
    case UACPI_OBJECT_REFERENCE:
        uacpi_object_detach_child(dst);
        break;
    case UACPI_OBJECT_STRING:
    case UACPI_OBJECT_BUFFER:
    case UACPI_OBJECT_METHOD:
    case UACPI_OBJECT_PACKAGE:
    case UACPI_OBJECT_MUTEX:
    case UACPI_OBJECT_EVENT:
    case UACPI_OBJECT_PROCESSOR:
    case UACPI_OBJECT_DEVICE:
    case UACPI_OBJECT_THERMAL_ZONE:
        free_object_storage(dst);
        break;
    default:
        break;
    }

    switch (src->type) {
    case UACPI_OBJECT_UNINITIALIZED:
    case UACPI_OBJECT_DEBUG:
        break;
    case UACPI_OBJECT_BUFFER:
    case UACPI_OBJECT_STRING:
        dst->flags = src->flags;
        ret = assign_buffer(dst, src, behavior);
        break;
    case UACPI_OBJECT_BUFFER_FIELD:
        dst->buffer_field = src->buffer_field;
        uacpi_shareable_ref(dst->buffer_field.backing);
        break;
    case UACPI_OBJECT_BUFFER_INDEX:
        dst->buffer_index = src->buffer_index;
        uacpi_shareable_ref(dst->buffer_index.buffer);
        break;
    case UACPI_OBJECT_INTEGER:
        dst->integer = src->integer;
        break;
    case UACPI_OBJECT_METHOD:
        dst->method = src->method;
        uacpi_shareable_ref(dst->method);
        break;
    case UACPI_OBJECT_MUTEX:
        ret = assign_mutex(dst, src, behavior);
        break;
    case UACPI_OBJECT_EVENT:
        ret = assign_event(dst, src, behavior);
        break;
    case UACPI_OBJECT_OPERATION_REGION:
        dst->op_region = src->op_region;
        uacpi_shareable_ref(dst->op_region);
        break;
    case UACPI_OBJECT_PACKAGE:
        ret = assign_package(dst, src, behavior);
        break;
    case UACPI_OBJECT_FIELD_UNIT:
        dst->field_unit = src->field_unit;
        uacpi_shareable_ref(dst->field_unit);
        break;
    case UACPI_OBJECT_REFERENCE:
        uacpi_object_attach_child(dst, src->inner_object);
        break;
    case UACPI_OBJECT_PROCESSOR:
        dst->processor = src->processor;
        uacpi_shareable_ref(dst->processor);
        break;
    case UACPI_OBJECT_DEVICE:
        dst->device = src->device;
        uacpi_shareable_ref(dst->device);
        break;
    case UACPI_OBJECT_THERMAL_ZONE:
        dst->thermal_zone = src->thermal_zone;
        uacpi_shareable_ref(dst->thermal_zone);
        break;
    default:
        ret = UACPI_STATUS_UNIMPLEMENTED;
    }

    if (ret == UACPI_STATUS_OK)
        dst->type = src->type;

    return ret;
}

struct uacpi_object *uacpi_create_internal_reference(
    enum uacpi_reference_kind kind, uacpi_object *child
)
{
    uacpi_object *ret;

    ret = uacpi_create_object(UACPI_OBJECT_REFERENCE);
    if (uacpi_unlikely(ret == UACPI_NULL))
        return ret;

    ret->flags = kind;
    uacpi_object_attach_child(ret, child);
    return ret;
}

uacpi_object *uacpi_unwrap_internal_reference(uacpi_object *object)
{
    for (;;) {
        if (object->type != UACPI_OBJECT_REFERENCE ||
            (object->flags == UACPI_REFERENCE_KIND_REFOF ||
             object->flags == UACPI_REFERENCE_KIND_PKG_INDEX))
            return object;

        object = object->inner_object;
    }
}

#endif // !UACPI_BAREBONES_MODE
