#include <uacpi/internal/opregion.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/helpers.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/io.h>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>

#ifndef UACPI_BAREBONES_MODE

#define PCI_ROOT_PNP_ID "PNP0A03"
#define PCI_EXPRESS_ROOT_PNP_ID "PNP0A08"

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
                "found a PCI root node %.4s controlling region %.4s\n",
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

static uacpi_status pci_region_attach(uacpi_region_attach_data *data)
{
    uacpi_namespace_node *node, *pci_root, *device;
    uacpi_pci_address address = { 0 };
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

    ret = uacpi_eval_simple_integer(device, "_ADR", &value);
    if (ret == UACPI_STATUS_OK) {
        address.function = (value >> 0)  & 0xFF;
        address.device   = (value >> 16) & 0xFF;
    }

    ret = uacpi_eval_simple_integer(pci_root, "_SEG", &value);
    if (ret == UACPI_STATUS_OK)
        address.segment = value;

    ret = uacpi_eval_simple_integer(pci_root, "_BBN", &value);
    if (ret == UACPI_STATUS_OK)
        address.bus = value;

    uacpi_trace(
        "detected PCI device %.4s@%04X:%02X:%02X:%01X\n",
        device->name.text, address.segment, address.bus,
        address.device, address.function
    );

    return uacpi_kernel_pci_device_open(address, &data->out_region_context);
}

static uacpi_status pci_region_detach(uacpi_region_detach_data *data)
{
    uacpi_kernel_pci_device_close(data->region_context);
    return UACPI_STATUS_OK;
}

static uacpi_status pci_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    uacpi_handle dev = data->region_context;
    uacpi_u8 width;
    uacpi_size offset;

    offset = data->offset;
    width = data->byte_width;

    return op == UACPI_REGION_OP_READ ?
        uacpi_pci_read(dev, offset, width, &data->value) :
        uacpi_pci_write(dev, offset, width, data->value);
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

    if (uacpi_unlikely(ctx->virt == UACPI_NULL)) {
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

static uacpi_status io_region_do_rw(
    uacpi_region_op op, uacpi_region_rw_data *data
)
{
    struct io_region_ctx *ctx = data->region_context;
    uacpi_u8 width;
    uacpi_size offset;

    offset = data->offset - ctx->base;
    width = data->byte_width;

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
