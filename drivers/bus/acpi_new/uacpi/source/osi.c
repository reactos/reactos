#include <uacpi/platform/atomic.h>
#include <uacpi/internal/osi.h>
#include <uacpi/internal/helpers.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/kernel_api.h>

#ifndef UACPI_BAREBONES_MODE

struct registered_interface {
    const uacpi_char *name;
    uacpi_u8 weight;
    uacpi_u8 kind;

    // Only applicable for predefined host interfaces
    uacpi_u8 host_type;

    // Only applicable for predefined interfaces
    uacpi_u8 disabled : 1;
    uacpi_u8 dynamic : 1;

    struct registered_interface *next;
};

static uacpi_handle interface_mutex;
static struct registered_interface *registered_interfaces;
static uacpi_interface_handler interface_handler;
static uacpi_u32 latest_queried_interface;

#define WINDOWS(string, interface)                            \
    {                                                         \
        .name = "Windows "string,                             \
        .weight = UACPI_VENDOR_INTERFACE_WINDOWS_##interface, \
        .kind = UACPI_INTERFACE_KIND_VENDOR,                  \
        .host_type = 0,                                       \
        .disabled = 0,                                        \
        .dynamic = 0,                                         \
        .next = UACPI_NULL                                    \
    }

#define HOST_FEATURE(string, type)                \
    {                                             \
        .name = string,                           \
        .weight = 0,                              \
        .kind = UACPI_INTERFACE_KIND_FEATURE,     \
        .host_type = UACPI_HOST_INTERFACE_##type, \
        .disabled = 1,                            \
        .dynamic = 0,                             \
        .next = UACPI_NULL,                       \
    }

static struct registered_interface predefined_interfaces[] = {
    // Vendor strings
    WINDOWS("2000", 2000),
    WINDOWS("2001", XP),
    WINDOWS("2001 SP1", XP_SP1),
    WINDOWS("2001.1", SERVER_2003),
    WINDOWS("2001 SP2", XP_SP2),
    WINDOWS("2001.1 SP1", SERVER_2003_SP1),
    WINDOWS("2006", VISTA),
    WINDOWS("2006.1", SERVER_2008),
    WINDOWS("2006 SP1", VISTA_SP1),
    WINDOWS("2006 SP2", VISTA_SP2),
    WINDOWS("2009", 7),
    WINDOWS("2012", 8),
    WINDOWS("2013", 8_1),
    WINDOWS("2015", 10),
    WINDOWS("2016", 10_RS1),
    WINDOWS("2017", 10_RS2),
    WINDOWS("2017.2", 10_RS3),
    WINDOWS("2018", 10_RS4),
    WINDOWS("2018.2", 10_RS5),
    WINDOWS("2019", 10_19H1),
    WINDOWS("2020", 10_20H1),
    WINDOWS("2021", 11),
    WINDOWS("2022", 11_22H2),

    // Feature strings
    HOST_FEATURE("Module Device", MODULE_DEVICE),
    HOST_FEATURE("Processor Device", PROCESSOR_DEVICE),
    HOST_FEATURE("3.0 Thermal Model", 3_0_THERMAL_MODEL),
    HOST_FEATURE("3.0 _SCP Extensions", 3_0_SCP_EXTENSIONS),
    HOST_FEATURE("Processor Aggregator Device", PROCESSOR_AGGREGATOR_DEVICE),

    // Interpreter features
    { .name = "Extended Address Space Descriptor" },
};

uacpi_status uacpi_initialize_interfaces(void)
{
    uacpi_size i;

    registered_interfaces = &predefined_interfaces[0];

    interface_mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(interface_mutex == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    for (i = 0; i < (UACPI_ARRAY_SIZE(predefined_interfaces) - 1); ++i)
        predefined_interfaces[i].next = &predefined_interfaces[i + 1];

    return UACPI_STATUS_OK;
}

void uacpi_deinitialize_interfaces(void)
{
    struct registered_interface *iface, *next_iface = registered_interfaces;

    while (next_iface) {
        iface = next_iface;
        next_iface = iface->next;

        iface->next = UACPI_NULL;

        if (iface->dynamic) {
            uacpi_free_dynamic_string(iface->name);
            uacpi_free(iface, sizeof(*iface));
            continue;
        }

        // Only features are disabled by default
        iface->disabled = iface->kind == UACPI_INTERFACE_KIND_FEATURE ?
                UACPI_TRUE : UACPI_FALSE;
    }

    if (interface_mutex)
        uacpi_kernel_free_mutex(interface_mutex);

    interface_mutex = UACPI_NULL;
    interface_handler = UACPI_NULL;
    latest_queried_interface = 0;
    registered_interfaces = UACPI_NULL;
}

uacpi_vendor_interface uacpi_latest_queried_vendor_interface(void)
{
    return uacpi_atomic_load32(&latest_queried_interface);
}

static struct registered_interface *find_interface_unlocked(
    const uacpi_char *name
)
{
    struct registered_interface *interface = registered_interfaces;

    while (interface) {
        if (uacpi_strcmp(interface->name, name) == 0)
            return interface;

        interface = interface->next;
    }

    return UACPI_NULL;
}

static struct registered_interface *find_host_interface_unlocked(
    uacpi_host_interface type
)
{
    struct registered_interface *interface = registered_interfaces;

    while (interface) {
        if (interface->host_type == type)
            return interface;

        interface = interface->next;
    }

    return UACPI_NULL;
}

uacpi_status uacpi_install_interface(
    const uacpi_char *name, uacpi_interface_kind kind
)
{
    struct registered_interface *interface;
    uacpi_status ret;
    uacpi_char *name_copy;
    uacpi_size name_size;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    interface = find_interface_unlocked(name);
    if (interface != UACPI_NULL) {
        if (interface->disabled)
            interface->disabled = UACPI_FALSE;

        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    interface = uacpi_kernel_alloc(sizeof(*interface));
    if (uacpi_unlikely(interface == UACPI_NULL)) {
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    name_size = uacpi_strlen(name) + 1;
    name_copy = uacpi_kernel_alloc(name_size);
    if (uacpi_unlikely(name_copy == UACPI_NULL)) {
        uacpi_free(interface, sizeof(*interface));
        ret = UACPI_STATUS_OUT_OF_MEMORY;
        goto out;
    }

    uacpi_memcpy(name_copy, name, name_size);
    interface->name = name_copy;
    interface->weight = 0;
    interface->kind = kind;
    interface->host_type = 0;
    interface->disabled = 0;
    interface->dynamic = 1;
    interface->next = registered_interfaces;
    registered_interfaces = interface;

out:
    uacpi_release_native_mutex(interface_mutex);
    return ret;
}

uacpi_status uacpi_uninstall_interface(const uacpi_char *name)
{
    struct registered_interface *cur, *prev;
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    cur = registered_interfaces;
    prev = cur;

    ret = UACPI_STATUS_NOT_FOUND;
    while (cur) {
        if (uacpi_strcmp(cur->name, name) != 0) {
            prev = cur;
            cur = cur->next;
            continue;
        }

        if (cur->dynamic) {
            if (prev == cur) {
                registered_interfaces = cur->next;
            } else {
                prev->next = cur->next;
            }

            uacpi_release_native_mutex(interface_mutex);
            uacpi_free_dynamic_string(cur->name);
            uacpi_free(cur, sizeof(*cur));
            return UACPI_STATUS_OK;
        }

        /*
         * If this interface was already disabled, pretend we didn't actually
         * find it and keep ret as UACPI_STATUS_NOT_FOUND. The fact that it's
         * still in the registered list is an implementation detail of
         * predefined interfaces.
         */
        if (!cur->disabled) {
            cur->disabled = UACPI_TRUE;
            ret = UACPI_STATUS_OK;
        }

        break;
    }

    uacpi_release_native_mutex(interface_mutex);
    return ret;
}

static uacpi_status configure_host_interface(
    uacpi_host_interface type, uacpi_bool enabled
)
{
    struct registered_interface *interface;
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    interface = find_host_interface_unlocked(type);
    if (interface == UACPI_NULL) {
        ret = UACPI_STATUS_NOT_FOUND;
        goto out;
    }

    interface->disabled = !enabled;
out:
    uacpi_release_native_mutex(interface_mutex);
    return ret;
}

uacpi_status uacpi_enable_host_interface(uacpi_host_interface type)
{
    return configure_host_interface(type, UACPI_TRUE);
}

uacpi_status uacpi_disable_host_interface(uacpi_host_interface type)
{
    return configure_host_interface(type, UACPI_FALSE);
}

uacpi_status uacpi_set_interface_query_handler(
    uacpi_interface_handler handler
)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (interface_handler != UACPI_NULL && handler != UACPI_NULL) {
        ret = UACPI_STATUS_ALREADY_EXISTS;
        goto out;
    }

    interface_handler = handler;
out:
    uacpi_release_native_mutex(interface_mutex);
    return ret;
}

uacpi_status uacpi_bulk_configure_interfaces(
    uacpi_interface_action action, uacpi_interface_kind kind
)
{
    uacpi_status ret;
    struct registered_interface *interface;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    interface = registered_interfaces;
    while (interface) {
        if (kind & interface->kind)
            interface->disabled = (action == UACPI_INTERFACE_ACTION_DISABLE);

        interface = interface->next;
    }

    uacpi_release_native_mutex(interface_mutex);
    return ret;
}

uacpi_status uacpi_handle_osi(const uacpi_char *string, uacpi_bool *out_value)
{
    uacpi_status ret;
    struct registered_interface *interface;
    uacpi_bool is_supported = UACPI_FALSE;

    ret = uacpi_acquire_native_mutex(interface_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    interface = find_interface_unlocked(string);
    if (interface == UACPI_NULL)
        goto out;

    if (interface->weight > latest_queried_interface)
        uacpi_atomic_store32(&latest_queried_interface, interface->weight);

    is_supported = !interface->disabled;
    if (interface_handler)
        is_supported = interface_handler(string, is_supported);
out:
    uacpi_release_native_mutex(interface_mutex);
    *out_value = is_supported;
    return UACPI_STATUS_OK;
}

#endif // !UACPI_BAREBONES_MODE
