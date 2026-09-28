#include <uacpi/internal/opregion.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/helpers.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/io.h>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>
#include <uacpi/osi.h>

#ifndef UACPI_BAREBONES_MODE

#define PCI_ROOT_PNP_ID "PNP0A03"
#define PCI_EXPRESS_ROOT_PNP_ID "PNP0A08"

#define PCI_HEADER_SIZE 0x40
#define PCI_HEADER_TYPE_REG 0x0E
#define PCI_HEADER_TYPE_MASK 0x7F
#define PCI_HEADER_TYPE_NORMAL 0x00
#define PCI_HEADER_TYPE_PCI_BRIDGE 0x01
#define PCI_HEADER_TYPE_CARDBUS_BRIDGE 0x02
#define PCI_HEADER_TYPE_NO_DEVICE 0xFF
#define PCI_SECONDARY_BUS_REG 0x19
#define PCI_SUBSYSTEM_VENDOR_ID_REG 0x2C
#define PCI_EXPANSION_ROM_REG 0x30
#define PCI_INTERRUPT_LINE_REG 0x3C

static uacpi_namespace_node *find_pci_root(uacpi_namespace_node *node)
{
    static const uacpi_char *pci_root_ids[] = {
        PCI_ROOT_PNP_ID,
        PCI_EXPRESS_ROOT_PNP_ID,
        UACPI_NULL
    };
    uacpi_namespace_node *parent = node->parent;

    while (parent != uacpi_namespace_root()) {
        if (uacpi_device_matches_pnp_id(parent, pci_root_ids)) {
            uacpi_trace(
                "found a PCI root node %.4s controlling region %.4s",
                parent->name.text, node->name.text
            );
            return parent;
        }

        parent = parent->parent;
    }

    uacpi_trace_region_error(
        node, "unable to find PCI root controlling",
        UACPI_STATUS_NOT_FOUND
    );
    return node;
}

enum pci_region_state {
    PCI_REGION_STATE_UNKNOWN = 0,
    PCI_REGION_STATE_OPEN,
    PCI_REGION_STATE_NOT_FOUND,
    PCI_REGION_STATE_UNREACHABLE,
};

// One of the PCI devices found on the way from the root to a region
struct pci_region_device {
    uacpi_u8 device;
    uacpi_u8 function;

    // Only valid if is_open is set
    uacpi_pci_address address;
    uacpi_handle handle;
    uacpi_bool is_open;
};

struct pci_region_ctx {
    // The device the region was declared in, only used for logging
    uacpi_namespace_node *node;

    // Segment & bus of the PCI root
    uacpi_pci_address root_address;

    /*
     * Everything between the PCI root and the region that might be a PCI
     * device, top-down. This is the root itself if there's nothing else, or
     * nothing at all if the root has no _ADR.
     */
    struct pci_region_device *devices;
    uacpi_size num_devices;

    // The outcome of the last access
    uacpi_pci_address address;
    uacpi_u8 state;
};

enum pci_device_kind {
    // There's no device at this address
    PCI_DEVICE_KIND_NONE,

    PCI_DEVICE_KIND_NORMAL,
    PCI_DEVICE_KIND_BRIDGE,

    // A bridge that has no secondary bus assigned
    PCI_DEVICE_KIND_UNCONFIGURED_BRIDGE,
};

static uacpi_bool pci_address_equal(
    const uacpi_pci_address *lhs, const uacpi_pci_address *rhs
)
{
    return lhs->segment == rhs->segment && lhs->bus == rhs->bus &&
           lhs->device == rhs->device && lhs->function == rhs->function;
}

static void pci_region_device_close(struct pci_region_device *dev)
{
    if (!dev->is_open)
        return;

    uacpi_kernel_pci_device_close(dev->handle);
    dev->is_open = UACPI_FALSE;
}

// The handle is kept around for as long as the device stays where it was
static uacpi_bool pci_region_device_open(
    struct pci_region_device *dev, const uacpi_pci_address *address
)
{
    uacpi_status ret;

    if (dev->is_open) {
        if (pci_address_equal(&dev->address, address))
            return UACPI_TRUE;

        pci_region_device_close(dev);
    }

    ret = uacpi_kernel_pci_device_open(*address, &dev->handle);
    if (ret != UACPI_STATUS_OK)
        return UACPI_FALSE;

    dev->address = *address;
    dev->is_open = UACPI_TRUE;
    return UACPI_TRUE;
}

// The secondary bus is only returned for PCI_DEVICE_KIND_BRIDGE
static enum pci_device_kind pci_region_device_probe(
    struct pci_region_device *dev, const uacpi_pci_address *address,
    uacpi_u8 *out_secondary_bus
)
{
    uacpi_status ret;
    uacpi_u8 header_type;

    if (!pci_region_device_open(dev, address))
        return PCI_DEVICE_KIND_NONE;

    ret = uacpi_kernel_pci_read8(
        dev->handle, PCI_HEADER_TYPE_REG, &header_type
    );
    if (ret != UACPI_STATUS_OK || header_type == PCI_HEADER_TYPE_NO_DEVICE)
        return PCI_DEVICE_KIND_NONE;

    switch (header_type & PCI_HEADER_TYPE_MASK) {
    case PCI_HEADER_TYPE_PCI_BRIDGE:
    case PCI_HEADER_TYPE_CARDBUS_BRIDGE:
        break;
    default:
        return PCI_DEVICE_KIND_NORMAL;
    }

    ret = uacpi_kernel_pci_read8(
        dev->handle, PCI_SECONDARY_BUS_REG, out_secondary_bus
    );
    if (ret != UACPI_STATUS_OK || *out_secondary_bus == 0)
        return PCI_DEVICE_KIND_UNCONFIGURED_BRIDGE;

    return PCI_DEVICE_KIND_BRIDGE;
}

static const uacpi_char *pci_region_state_to_string(uacpi_u8 state)
{
    switch (state) {
    case PCI_REGION_STATE_OPEN:
        return "detected";
    case PCI_REGION_STATE_NOT_FOUND:
        return "is not present or powered off";
    default:
        return "is unreachable due to a missing or unconfigured bridge";
    }
}

/*
 * Figure out the PCI device that a region belongs to at the moment, and open
 * it.
 *
 * A device is only a PCI device if it has an _ADR, and its direct parent is
 * either the PCI root or a PCI device that is a bridge, in which case it lives
 * on the secondary bus of that bridge. This means that e.g. a USB port below
 * an XHCI controller is not a PCI device even though it does have an _ADR.
 * The region then belongs to the closest PCI device at or above the device it
 * was declared in, or to the root itself if there's none.
 *
 * The namespace side of this never changes, so it's only looked at once when
 * the region is attached. The bus numbers on the other hand can change at any
 * time since they're host writable, which is why those are read for every
 * access.
 *
 * Returns the device that the access must be forwarded to, or NULL if there's
 * no such device at the moment.
 */
static struct pci_region_device *pci_region_resolve(struct pci_region_ctx *ctx)
{
    struct pci_region_device *dev;
    uacpi_pci_address address = ctx->root_address;
    uacpi_u8 state = PCI_REGION_STATE_OPEN;
    enum pci_device_kind kind;
    uacpi_size i;
    uacpi_u8 bus;

    // The region doesn't belong to any device, see pci_region_find_devices()
    if (ctx->num_devices == 0)
        return UACPI_NULL;

    for (i = 0;; ++i) {
        dev = &ctx->devices[i];

        address.device = dev->device;
        address.function = dev->function;

        // Nothing below us, so this is the one
        if ((i + 1) == ctx->num_devices)
            break;

        kind = pci_region_device_probe(dev, &address, &bus);
        if (kind == PCI_DEVICE_KIND_BRIDGE) {
            address.bus = bus;
            continue;
        }

        // Whatever is below a normal device belongs to it, so we're done
        if (kind != PCI_DEVICE_KIND_NORMAL)
            state = PCI_REGION_STATE_UNREACHABLE;
        break;
    }

    // Whatever is below the device we stopped at is of no use anymore
    while (++i < ctx->num_devices)
        pci_region_device_close(&ctx->devices[i]);

    if (state == PCI_REGION_STATE_OPEN &&
        !pci_region_device_open(dev, &address))
        state = PCI_REGION_STATE_NOT_FOUND;

    if (state != ctx->state || !pci_address_equal(&address, &ctx->address)) {
        uacpi_trace(
            "PCI device %.4s %s at %04X:%02X:%02X:%01X", ctx->node->name.text,
            pci_region_state_to_string(state), address.segment, address.bus,
            address.device, address.function
        );

        ctx->address = address;
        ctx->state = state;
    }

    return state == PCI_REGION_STATE_OPEN ? dev : UACPI_NULL;
}

static void pci_region_ctx_free(struct pci_region_ctx *ctx)
{
    uacpi_size i;

    for (i = 0; i < ctx->num_devices; ++i)
        pci_region_device_close(&ctx->devices[i]);

    if (ctx->devices != UACPI_NULL)
        uacpi_free(ctx->devices, ctx->num_devices * sizeof(*ctx->devices));

    uacpi_free(ctx, sizeof(*ctx));
}

/*
 * Collect the devices below 'pci_root' that lead to 'device' for as long as
 * they have an _ADR, see pci_region_resolve() for why. If 'out' is NULL the
 * devices are only counted.
 */
static uacpi_size pci_region_collect_devices(
    uacpi_namespace_node *pci_root, uacpi_namespace_node *device,
    struct pci_region_device *out
)
{
    uacpi_namespace_node *node;
    uacpi_size depth = 0, count = 0, i;
    uacpi_object_type type;
    uacpi_status ret;
    uacpi_u64 adr;

    for (node = device; node != UACPI_NULL && node != pci_root;
         node = node->parent)
        depth++;

    // There's no root above the device, the best we can do is use its _ADR
    if (node == UACPI_NULL)
        depth = 1;

    while (depth-- != 0) {
        node = device;
        for (i = 0; i < depth; ++i)
            node = node->parent;

        ret = uacpi_namespace_node_type(node, &type);
        if (ret != UACPI_STATUS_OK || type != UACPI_OBJECT_DEVICE)
            break;

        ret = uacpi_eval_simple_integer(node, "_ADR", &adr);
        if (ret != UACPI_STATUS_OK)
            break;

        if (out != UACPI_NULL) {
            out[count].function = (adr >> 0)  & 0xFF;
            out[count].device   = (adr >> 16) & 0xFF;
        }
        count++;
    }

    return count;
}

static uacpi_status pci_region_find_devices(
    struct pci_region_ctx *ctx, uacpi_namespace_node *pci_root,
    uacpi_namespace_node *device
)
{
    struct pci_region_device *devices;
    uacpi_size count;
    uacpi_status ret;
    uacpi_u64 adr;

    count = pci_region_collect_devices(pci_root, device, UACPI_NULL);
    if (count != 0) {
        devices = uacpi_kernel_alloc_zeroed(count * sizeof(*devices));
        if (uacpi_unlikely(devices == UACPI_NULL))
            return UACPI_STATUS_OUT_OF_MEMORY;

        ctx->devices = devices;
        ctx->num_devices = pci_region_collect_devices(
            pci_root, device, devices
        );
        return UACPI_STATUS_OK;
    }

    ret = uacpi_eval_simple_integer(pci_root, "_ADR", &adr);
    if (ret != UACPI_STATUS_OK) {
        uacpi_trace(
            "unable to determine the PCI address of %.4s",
            device->name.text
        );
        return UACPI_STATUS_OK;
    }

    devices = uacpi_kernel_alloc_zeroed(sizeof(*devices));
    if (uacpi_unlikely(devices == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    devices->function = (adr >> 0)  & 0xFF;
    devices->device   = (adr >> 16) & 0xFF;

    ctx->devices = devices;
    ctx->num_devices = 1;
    return UACPI_STATUS_OK;
}

static uacpi_status pci_region_attach(uacpi_region_attach_data *data)
{
    uacpi_namespace_node *node, *pci_root, *device;
    struct pci_region_ctx *ctx;
    uacpi_u64 value;
    uacpi_status ret;

    node = data->region_node;
    pci_root = find_pci_root(node);

    /*
     * Find the actual device object that is supposed to be controlling
     * this operation region.
     */
    device = node;
    while (device) {
        uacpi_object_type type;

        ret = uacpi_namespace_node_type(device, &type);
        if (uacpi_unlikely_error(ret))
            return ret;

        if (type == UACPI_OBJECT_DEVICE)
            break;

        device = device->parent;
    }

    if (uacpi_unlikely(device == UACPI_NULL)) {
        ret = UACPI_STATUS_NOT_FOUND;
        uacpi_trace_region_error(
            node, "unable to find device responsible for", ret
        );
        return ret;
    }

    ctx = uacpi_kernel_alloc_zeroed(sizeof(*ctx));
    if (uacpi_unlikely(ctx == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ctx->node = device;

    ret = uacpi_eval_simple_integer(pci_root, "_SEG", &value);
    if (ret == UACPI_STATUS_OK)
        ctx->root_address.segment = value;

    ret = uacpi_eval_simple_integer(pci_root, "_BBN", &value);
    if (ret == UACPI_STATUS_OK)
        ctx->root_address.bus = value;

    ret = pci_region_find_devices(ctx, pci_root, device);
    if (uacpi_unlikely_error(ret)) {
        uacpi_free(ctx, sizeof(*ctx));
        return ret;
    }

    data->out_region_context = ctx;
    return UACPI_STATUS_OK;
}

static uacpi_status pci_region_detach(uacpi_region_detach_data *data)
{
    pci_region_ctx_free(data->region_context);
    return UACPI_STATUS_OK;
}

// We intentionally only check the first port of an access, same as NT
static uacpi_bool pci_write_is_protected(
    struct pci_region_device *dev, uacpi_size offset
)
{
    uacpi_status ret;
    uacpi_u8 header_type;

    if (offset >= PCI_HEADER_SIZE)
        return UACPI_FALSE;

    if (offset < PCI_SUBSYSTEM_VENDOR_ID_REG)
        return UACPI_TRUE;
    if (offset >= PCI_EXPANSION_ROM_REG && offset < PCI_INTERRUPT_LINE_REG)
        return UACPI_TRUE;

    ret = uacpi_kernel_pci_read8(
        dev->handle, PCI_HEADER_TYPE_REG, &header_type
    );
    if (uacpi_unlikely_error(ret))
        return UACPI_TRUE;

    return (header_type & PCI_HEADER_TYPE_MASK) != PCI_HEADER_TYPE_NORMAL;
}

static uacpi_status pci_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    uacpi_u8 width;
    uacpi_size offset;
    struct pci_region_device *dev;

    offset = data->offset;
    width = data->byte_width;

    dev = pci_region_resolve(data->region_context);
    if (dev == UACPI_NULL) {
        uacpi_trace(
            "faking a PCI device %s access",
            op == UACPI_REGION_OP_READ ? "read" : "write"
        );

        if (op == UACPI_REGION_OP_READ) {
            static uacpi_u64 ffs = 0xFFFFFFFFFFFFFFFF;
            uacpi_memcpy(&data->value, &ffs, width);
        }

        return UACPI_STATUS_OK;
    }

    if (op == UACPI_REGION_OP_READ)
        return uacpi_pci_read(dev->handle, offset, width, &data->value);

    if (pci_write_is_protected(dev, offset)) {
        uacpi_trace(
            "denied AML write access to protected offset 0x%02zX of PCI "
            "device %04X:%02X:%02X:%01X", offset, dev->address.segment,
            dev->address.bus, dev->address.device, dev->address.function
        );
        return UACPI_STATUS_OK;
    }

    return uacpi_pci_write(dev->handle, offset, width, data->value);
}

static uacpi_status handle_pci_region(uacpi_region_op op, uacpi_handle op_data)
{
    switch (op) {
    case UACPI_REGION_OP_ATTACH:
        return pci_region_attach(op_data);
    case UACPI_REGION_OP_DETACH:
        return pci_region_detach(op_data);
    case UACPI_REGION_OP_READ:
    case UACPI_REGION_OP_WRITE:
        return pci_region_do_rw(op, op_data);
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
}

struct memory_region_ctx {
    uacpi_phys_addr phys;
    uacpi_u8 *virt;
    uacpi_size size;
};

static uacpi_status memory_region_attach(uacpi_region_attach_data *data)
{
    struct memory_region_ctx *ctx;
    uacpi_status ret = UACPI_STATUS_OK;

    ctx = uacpi_kernel_alloc(sizeof(*ctx));
    if (ctx == UACPI_NULL)
        return UACPI_STATUS_OUT_OF_MEMORY;

    ctx->size = data->generic_info.length;

    // FIXME: this really shouldn't try to map everything at once
    ctx->phys = data->generic_info.base;
    ctx->virt = uacpi_kernel_map(ctx->phys, ctx->size);

    if (uacpi_unlikely(ctx->virt == UACPI_MAP_FAILED)) {
        ret = UACPI_STATUS_MAPPING_FAILED;
        uacpi_trace_region_error(data->region_node, "unable to map", ret);
        uacpi_free(ctx, sizeof(*ctx));
        goto out;
    }

    data->out_region_context = ctx;
out:
    return ret;
}

static uacpi_status memory_region_detach(uacpi_region_detach_data *data)
{
    struct memory_region_ctx *ctx = data->region_context;

    uacpi_kernel_unmap(ctx->virt, ctx->size);
    uacpi_free(ctx, sizeof(*ctx));
    return UACPI_STATUS_OK;
}

struct io_region_ctx {
    uacpi_io_addr base;
    uacpi_handle handle;
};

static uacpi_status io_region_attach(uacpi_region_attach_data *data)
{
    struct io_region_ctx *ctx;
    uacpi_generic_region_info *info = &data->generic_info;
    uacpi_status ret;

    ctx = uacpi_kernel_alloc(sizeof(*ctx));
    if (ctx == UACPI_NULL)
        return UACPI_STATUS_OUT_OF_MEMORY;

    ctx->base = info->base;

    ret = uacpi_kernel_io_map(ctx->base, info->length, &ctx->handle);
    if (uacpi_unlikely_error(ret)) {
        uacpi_trace_region_error(
            data->region_node, "unable to map an IO", ret
        );
        uacpi_free(ctx, sizeof(*ctx));
        return ret;
    }

    data->out_region_context = ctx;
    return ret;
}

static uacpi_status io_region_detach(uacpi_region_detach_data *data)
{
    struct io_region_ctx *ctx = data->region_context;

    uacpi_kernel_io_unmap(ctx->handle);
    uacpi_free(ctx, sizeof(*ctx));
    return UACPI_STATUS_OK;
}

static uacpi_status memory_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    struct memory_region_ctx *ctx = data->region_context;
    uacpi_size offset;

    offset = data->address - ctx->phys;

    return op == UACPI_REGION_OP_READ ?
        uacpi_system_memory_read(ctx->virt, offset, data->byte_width, &data->value) :
        uacpi_system_memory_write(ctx->virt, offset, data->byte_width, data->value);
}

static uacpi_status handle_memory_region(uacpi_region_op op, uacpi_handle op_data)
{
    switch (op) {
    case UACPI_REGION_OP_ATTACH:
        return memory_region_attach(op_data);
    case UACPI_REGION_OP_DETACH:
        return memory_region_detach(op_data);
    case UACPI_REGION_OP_READ:
    case UACPI_REGION_OP_WRITE:
        return memory_region_do_rw(op, op_data);
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
}

static uacpi_status table_data_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    void *addr = UACPI_VIRT_ADDR_TO_PTR((uacpi_virt_addr)data->offset);

    return op == UACPI_REGION_OP_READ ?
       uacpi_system_memory_read(addr, 0, data->byte_width, &data->value) :
       uacpi_system_memory_write(addr, 0, data->byte_width, data->value);
}

static uacpi_status handle_table_data_region(uacpi_region_op op, uacpi_handle op_data)
{
    switch (op) {
    case UACPI_REGION_OP_ATTACH:
    case UACPI_REGION_OP_DETACH:
        return UACPI_STATUS_OK;
    case UACPI_REGION_OP_READ:
    case UACPI_REGION_OP_WRITE:
        return table_data_region_do_rw(op, op_data);
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
}

enum io_protection_kind {
    IO_PROTECTION_KIND_ALWAYS,

    // Only after AML has queried _OSI for Windows XP or anything newer
    IO_PROTECTION_KIND_XP_AND_ABOVE,
};

struct protected_io_range {
    uacpi_u16 first;
    uacpi_u16 last;
    uacpi_u8 kind;
};

/*
 * Ports that AML can't touch. These ranges match the NT ACPI driver 1:1,
 * verified via a traced QEMU run with injected AML. Access to these returns
 * 0 on reads and writes are simply dropped.
 */
static const struct protected_io_range protected_io_ranges[] = {
    // DMA
    { 0x0000, 0x000F, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // PIC
    { 0x0020, 0x0021, IO_PROTECTION_KIND_ALWAYS },
    // PIT
    { 0x0040, 0x0043, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // PIT (EISA)
    { 0x0048, 0x004B, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // RTC/CMOS
    { 0x0070, 0x0071, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // Extended CMOS
    { 0x0074, 0x0076, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // DMA page registers
    { 0x0081, 0x0083, IO_PROTECTION_KIND_XP_AND_ABOVE },
    { 0x0087, 0x0087, IO_PROTECTION_KIND_XP_AND_ABOVE },
    { 0x0089, 0x008B, IO_PROTECTION_KIND_XP_AND_ABOVE },
    { 0x008F, 0x008F, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // Arbitration control
    { 0x0090, 0x0091, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // System board setup
    { 0x0093, 0x0094, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // POS channel select
    { 0x0096, 0x0097, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // Cascaded PIC
    { 0x00A0, 0x00A1, IO_PROTECTION_KIND_ALWAYS },
    // ISA DMA
    { 0x00C0, 0x00DF, IO_PROTECTION_KIND_XP_AND_ABOVE },
    // ELCR
    { 0x04D0, 0x04D1, IO_PROTECTION_KIND_ALWAYS },
    /*
     * PCI configuration mechanism #1. NT emulates this one via its own PCI
     * accessors until Windows XP is queried. We can't afford that because
     * we could race against the host kernel PCI configuration accesses, and
     * rolling emulation on top of the kernel API for PCI doesn't seem worth it
     * as this doesn't seem to be actualy used by AML from inspecting real
     * hardware AML dumps.
     */
    { 0x0CF8, 0x0CFF, IO_PROTECTION_KIND_ALWAYS },
};

// We intentionally only check the first port of an access, same as NT
static uacpi_bool io_port_is_protected(uacpi_u64 port)
{
    const struct protected_io_range *range;
    uacpi_size i;

    for (i = 0; i < UACPI_ARRAY_SIZE(protected_io_ranges); ++i) {
        range = &protected_io_ranges[i];

        if (port < range->first || port > range->last)
            continue;
        if (range->kind == IO_PROTECTION_KIND_ALWAYS)
            return UACPI_TRUE;

        return uacpi_latest_queried_vendor_interface() >=
               UACPI_VENDOR_INTERFACE_WINDOWS_XP;
    }

    return UACPI_FALSE;
}

static uacpi_status io_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    struct io_region_ctx *ctx = data->region_context;
    uacpi_u8 width;
    uacpi_size offset;

    offset = data->offset - ctx->base;
    width = data->byte_width;

    if (io_port_is_protected(data->offset)) {
        uacpi_trace(
            "denied AML %s access to protected port 0x%04"UACPI_PRIX64,
            op == UACPI_REGION_OP_READ ? "read" : "write",
            UACPI_FMT64(data->offset)
        );

        if (op == UACPI_REGION_OP_READ)
            data->value = 0;

        return UACPI_STATUS_OK;
    }

    return op == UACPI_REGION_OP_READ ?
        uacpi_system_io_read(ctx->handle, offset, width, &data->value) :
        uacpi_system_io_write(ctx->handle, offset, width, data->value);
}

static uacpi_status handle_io_region(uacpi_region_op op, uacpi_handle op_data)
{
    switch (op) {
    case UACPI_REGION_OP_ATTACH:
        return io_region_attach(op_data);
    case UACPI_REGION_OP_DETACH:
        return io_region_detach(op_data);
    case UACPI_REGION_OP_READ:
    case UACPI_REGION_OP_WRITE:
        return io_region_do_rw(op, op_data);
    default:
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
}

void uacpi_install_default_address_space_handlers(void)
{
    uacpi_namespace_node *root;

    root = uacpi_namespace_root();

    uacpi_install_address_space_handler_with_flags(
        root, UACPI_ADDRESS_SPACE_SYSTEM_MEMORY,
        handle_memory_region, UACPI_NULL,
        UACPI_ADDRESS_SPACE_HANDLER_DEFAULT
    );

    uacpi_install_address_space_handler_with_flags(
        root, UACPI_ADDRESS_SPACE_SYSTEM_IO,
        handle_io_region, UACPI_NULL,
        UACPI_ADDRESS_SPACE_HANDLER_DEFAULT
    );

    uacpi_install_address_space_handler_with_flags(
        root, UACPI_ADDRESS_SPACE_PCI_CONFIG,
        handle_pci_region, UACPI_NULL,
        UACPI_ADDRESS_SPACE_HANDLER_DEFAULT
    );

    uacpi_install_address_space_handler_with_flags(
        root, UACPI_ADDRESS_SPACE_TABLE_DATA,
        handle_table_data_region, UACPI_NULL,
        UACPI_ADDRESS_SPACE_HANDLER_DEFAULT
    );
}

#endif // !UACPI_BAREBONES_MODE
