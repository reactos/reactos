#include <uacpi/types.h>
#include <uacpi/acpi.h>
#include <uacpi/internal/resources.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/utilities.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/namespace.h>
#include <uacpi/uacpi.h>

#ifndef UACPI_BAREBONES_MODE

#define LARGE_RESOURCE_BASE (ACPI_RESOURCE_END_TAG + 1)
#define L(x) (x + LARGE_RESOURCE_BASE)

/*
 * Map raw AML resource types to the internal enum, this also takes care of type
 * sanitization by returning UACPI_AML_RESOURCE_INVALID for any unknown type.
 */
static const uacpi_u8 aml_resource_to_type[256] = {
    // Small items
    [ACPI_RESOURCE_IRQ] = UACPI_AML_RESOURCE_IRQ,
    [ACPI_RESOURCE_DMA] = UACPI_AML_RESOURCE_DMA,
    [ACPI_RESOURCE_START_DEPENDENT] = UACPI_AML_RESOURCE_START_DEPENDENT,
    [ACPI_RESOURCE_END_DEPENDENT] = UACPI_AML_RESOURCE_END_DEPENDENT,
    [ACPI_RESOURCE_IO] = UACPI_AML_RESOURCE_IO,
    [ACPI_RESOURCE_FIXED_IO] = UACPI_AML_RESOURCE_FIXED_IO,
    [ACPI_RESOURCE_FIXED_DMA] = UACPI_AML_RESOURCE_FIXED_DMA,
    [ACPI_RESOURCE_VENDOR_TYPE0] = UACPI_AML_RESOURCE_VENDOR_TYPE0,
    [ACPI_RESOURCE_END_TAG] = UACPI_AML_RESOURCE_END_TAG,

    // Large items
    [L(ACPI_RESOURCE_MEMORY24)] = UACPI_AML_RESOURCE_MEMORY24,
    [L(ACPI_RESOURCE_GENERIC_REGISTER)] = UACPI_AML_RESOURCE_GENERIC_REGISTER,
    [L(ACPI_RESOURCE_VENDOR_TYPE1)] = UACPI_AML_RESOURCE_VENDOR_TYPE1,
    [L(ACPI_RESOURCE_MEMORY32)] = UACPI_AML_RESOURCE_MEMORY32,
    [L(ACPI_RESOURCE_FIXED_MEMORY32)] = UACPI_AML_RESOURCE_FIXED_MEMORY32,
    [L(ACPI_RESOURCE_ADDRESS32)] = UACPI_AML_RESOURCE_ADDRESS32,
    [L(ACPI_RESOURCE_ADDRESS16)] = UACPI_AML_RESOURCE_ADDRESS16,
    [L(ACPI_RESOURCE_EXTENDED_IRQ)] = UACPI_AML_RESOURCE_EXTENDED_IRQ,
    [L(ACPI_RESOURCE_ADDRESS64_EXTENDED)] = UACPI_AML_RESOURCE_ADDRESS64_EXTENDED,
    [L(ACPI_RESOURCE_ADDRESS64)] = UACPI_AML_RESOURCE_ADDRESS64,
    [L(ACPI_RESOURCE_GPIO_CONNECTION)] = UACPI_AML_RESOURCE_GPIO_CONNECTION,
    [L(ACPI_RESOURCE_PIN_FUNCTION)] = UACPI_AML_RESOURCE_PIN_FUNCTION,
    [L(ACPI_RESOURCE_SERIAL_CONNECTION)] = UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    [L(ACPI_RESOURCE_PIN_CONFIGURATION)] = UACPI_AML_RESOURCE_PIN_CONFIGURATION,
    [L(ACPI_RESOURCE_PIN_GROUP)] = UACPI_AML_RESOURCE_PIN_GROUP,
    [L(ACPI_RESOURCE_PIN_GROUP_FUNCTION)] = UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION,
    [L(ACPI_RESOURCE_PIN_GROUP_CONFIGURATION)] = UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION,
    [L(ACPI_RESOURCE_CLOCK_INPUT)] = UACPI_AML_RESOURCE_CLOCK_INPUT,
};

static const uacpi_u8 type_to_aml_resource[] = {
    [UACPI_AML_RESOURCE_IRQ] = ACPI_RESOURCE_IRQ,
    [UACPI_AML_RESOURCE_DMA] = ACPI_RESOURCE_DMA,
    [UACPI_AML_RESOURCE_START_DEPENDENT] = ACPI_RESOURCE_START_DEPENDENT,
    [UACPI_AML_RESOURCE_END_DEPENDENT] = ACPI_RESOURCE_END_DEPENDENT,
    [UACPI_AML_RESOURCE_IO] = ACPI_RESOURCE_IO,
    [UACPI_AML_RESOURCE_FIXED_IO] = ACPI_RESOURCE_FIXED_IO,
    [UACPI_AML_RESOURCE_FIXED_DMA] = ACPI_RESOURCE_FIXED_DMA,
    [UACPI_AML_RESOURCE_VENDOR_TYPE0] = ACPI_RESOURCE_VENDOR_TYPE0,
    [UACPI_AML_RESOURCE_END_TAG] = ACPI_RESOURCE_END_TAG,

    // Large items
    [UACPI_AML_RESOURCE_MEMORY24] = ACPI_RESOURCE_MEMORY24,
    [UACPI_AML_RESOURCE_GENERIC_REGISTER] = ACPI_RESOURCE_GENERIC_REGISTER,
    [UACPI_AML_RESOURCE_VENDOR_TYPE1] = ACPI_RESOURCE_VENDOR_TYPE1,
    [UACPI_AML_RESOURCE_MEMORY32] = ACPI_RESOURCE_MEMORY32,
    [UACPI_AML_RESOURCE_FIXED_MEMORY32] = ACPI_RESOURCE_FIXED_MEMORY32,
    [UACPI_AML_RESOURCE_ADDRESS32] = ACPI_RESOURCE_ADDRESS32,
    [UACPI_AML_RESOURCE_ADDRESS16] = ACPI_RESOURCE_ADDRESS16,
    [UACPI_AML_RESOURCE_EXTENDED_IRQ] = ACPI_RESOURCE_EXTENDED_IRQ,
    [UACPI_AML_RESOURCE_ADDRESS64_EXTENDED] = ACPI_RESOURCE_ADDRESS64_EXTENDED,
    [UACPI_AML_RESOURCE_ADDRESS64] = ACPI_RESOURCE_ADDRESS64,
    [UACPI_AML_RESOURCE_GPIO_CONNECTION] = ACPI_RESOURCE_GPIO_CONNECTION,
    [UACPI_AML_RESOURCE_PIN_FUNCTION] = ACPI_RESOURCE_PIN_FUNCTION,
    [UACPI_AML_RESOURCE_SERIAL_CONNECTION] = ACPI_RESOURCE_SERIAL_CONNECTION,
    [UACPI_AML_RESOURCE_PIN_CONFIGURATION] = ACPI_RESOURCE_PIN_CONFIGURATION,
    [UACPI_AML_RESOURCE_PIN_GROUP] = ACPI_RESOURCE_PIN_GROUP,
    [UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION] = ACPI_RESOURCE_PIN_GROUP_FUNCTION,
    [UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION] = ACPI_RESOURCE_PIN_GROUP_CONFIGURATION,
    [UACPI_AML_RESOURCE_CLOCK_INPUT] = ACPI_RESOURCE_CLOCK_INPUT,
};

static const uacpi_u8 native_resource_to_type[UACPI_RESOURCE_TYPE_MAX + 1] = {
    [UACPI_RESOURCE_TYPE_IRQ] = UACPI_AML_RESOURCE_IRQ,
    [UACPI_RESOURCE_TYPE_EXTENDED_IRQ] = UACPI_AML_RESOURCE_EXTENDED_IRQ,
    [UACPI_RESOURCE_TYPE_DMA] = UACPI_AML_RESOURCE_DMA,
    [UACPI_RESOURCE_TYPE_FIXED_DMA] = UACPI_AML_RESOURCE_FIXED_DMA,
    [UACPI_RESOURCE_TYPE_IO] = UACPI_AML_RESOURCE_IO,
    [UACPI_RESOURCE_TYPE_FIXED_IO] = UACPI_AML_RESOURCE_FIXED_IO,
    [UACPI_RESOURCE_TYPE_ADDRESS16] = UACPI_AML_RESOURCE_ADDRESS16,
    [UACPI_RESOURCE_TYPE_ADDRESS32] = UACPI_AML_RESOURCE_ADDRESS32,
    [UACPI_RESOURCE_TYPE_ADDRESS64] = UACPI_AML_RESOURCE_ADDRESS64,
    [UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED] = UACPI_AML_RESOURCE_ADDRESS64_EXTENDED,
    [UACPI_RESOURCE_TYPE_MEMORY24] = UACPI_AML_RESOURCE_MEMORY24,
    [UACPI_RESOURCE_TYPE_MEMORY32] = UACPI_AML_RESOURCE_MEMORY32,
    [UACPI_RESOURCE_TYPE_FIXED_MEMORY32] = UACPI_AML_RESOURCE_FIXED_MEMORY32,
    [UACPI_RESOURCE_TYPE_START_DEPENDENT] = UACPI_AML_RESOURCE_START_DEPENDENT,
    [UACPI_RESOURCE_TYPE_END_DEPENDENT] = UACPI_AML_RESOURCE_END_DEPENDENT,
    [UACPI_RESOURCE_TYPE_VENDOR_SMALL] = UACPI_AML_RESOURCE_VENDOR_TYPE0,
    [UACPI_RESOURCE_TYPE_VENDOR_LARGE] = UACPI_AML_RESOURCE_VENDOR_TYPE1,
    [UACPI_RESOURCE_TYPE_GENERIC_REGISTER] = UACPI_AML_RESOURCE_GENERIC_REGISTER,
    [UACPI_RESOURCE_TYPE_GPIO_CONNECTION] = UACPI_AML_RESOURCE_GPIO_CONNECTION,
    [UACPI_RESOURCE_TYPE_SERIAL_I2C_CONNECTION] = UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    [UACPI_RESOURCE_TYPE_SERIAL_SPI_CONNECTION] = UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    [UACPI_RESOURCE_TYPE_SERIAL_UART_CONNECTION] = UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    [UACPI_RESOURCE_TYPE_SERIAL_CSI2_CONNECTION] = UACPI_AML_RESOURCE_SERIAL_CONNECTION,
    [UACPI_RESOURCE_TYPE_PIN_FUNCTION] = UACPI_AML_RESOURCE_PIN_FUNCTION,
    [UACPI_RESOURCE_TYPE_PIN_CONFIGURATION] = UACPI_AML_RESOURCE_PIN_CONFIGURATION,
    [UACPI_RESOURCE_TYPE_PIN_GROUP] = UACPI_AML_RESOURCE_PIN_GROUP,
    [UACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION] = UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION,
    [UACPI_RESOURCE_TYPE_PIN_GROUP_CONFIGURATION] = UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION,
    [UACPI_RESOURCE_TYPE_CLOCK_INPUT] = UACPI_AML_RESOURCE_CLOCK_INPUT,
    [UACPI_RESOURCE_TYPE_END_TAG] = UACPI_AML_RESOURCE_END_TAG,
};

#define SMALL_ITEM_HEADER_SIZE sizeof(struct acpi_small_item)
#define LARGE_ITEM_HEADER_SIZE sizeof(struct acpi_large_item)

static const uacpi_u8 aml_resource_kind_to_header_size[2] = {
    [UACPI_AML_RESOURCE_KIND_SMALL] = SMALL_ITEM_HEADER_SIZE,
    [UACPI_AML_RESOURCE_KIND_LARGE] = LARGE_ITEM_HEADER_SIZE,
};

static uacpi_size aml_size_with_header(const struct uacpi_resource_spec *spec)
{
    return spec->aml_size +
           aml_resource_kind_to_header_size[spec->resource_kind];
}

static uacpi_size extra_size_for_native_irq_or_dma(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    UACPI_UNUSED(size);
    uacpi_u16 mask;
    uacpi_u8 i, total_bits, num_bits = 0;

    if (spec->type == UACPI_AML_RESOURCE_IRQ) {
        struct acpi_resource_irq *irq = data;
        mask = irq->irq_mask;
        total_bits = 16;
    } else {
        struct acpi_resource_dma *dma = data;
        mask = dma->channel_mask;
        total_bits = 8;
    }

    for (i = 0; i < total_bits; ++i)
        num_bits += !!(mask & (1 << i));

    return num_bits;
}

static uacpi_size size_for_aml_irq(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_resource_irq *irq = &resource->irq;
    uacpi_size size;

    size = aml_size_with_header(spec);

    switch (irq->length_kind) {
    case UACPI_RESOURCE_LENGTH_KIND_FULL:
        goto out_full;
    case UACPI_RESOURCE_LENGTH_KIND_ONE_LESS:
    case UACPI_RESOURCE_LENGTH_KIND_DONT_CARE:
        if (irq->triggering != UACPI_TRIGGERING_EDGE)
            goto out_full;
        if (irq->polarity != UACPI_POLARITY_ACTIVE_HIGH)
            goto out_full;
        if (irq->sharing != UACPI_EXCLUSIVE)
            goto out_full;

        return size - 1;
    }

out_full:
    if (uacpi_unlikely(irq->length_kind ==
                       UACPI_RESOURCE_LENGTH_KIND_ONE_LESS)) {
        uacpi_warn("requested IRQ resource length is "
                   "not compatible with specified flags, corrected\n");
    }

    return size;
}

static uacpi_size size_for_aml_start_dependent(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_resource_start_dependent *start_dep = &resource->start_dependent;
    uacpi_size size;

    size = aml_size_with_header(spec);
    switch (start_dep->length_kind) {
    case UACPI_RESOURCE_LENGTH_KIND_FULL:
        goto out_full;
    case UACPI_RESOURCE_LENGTH_KIND_ONE_LESS:
    case UACPI_RESOURCE_LENGTH_KIND_DONT_CARE:
        if (start_dep->compatibility != UACPI_ACCEPTABLE)
            goto out_full;
        if (start_dep->performance != UACPI_ACCEPTABLE)
            goto out_full;

        return size - 1;
    }

out_full:
    if (uacpi_unlikely(start_dep->length_kind ==
                       UACPI_RESOURCE_LENGTH_KIND_ONE_LESS)) {
        uacpi_warn("requested StartDependentFn resource length is "
                   "not compatible with specified flags, corrected\n");
    }

    return size;
}

static uacpi_size extra_size_for_native_vendor(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    UACPI_UNUSED(spec);
    UACPI_UNUSED(data);
    return size;
}

static uacpi_size size_for_aml_vendor(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    UACPI_UNUSED(spec);
    uacpi_size size = resource->vendor.length;

    if (size > 7 || resource->type == UACPI_RESOURCE_TYPE_VENDOR_LARGE) {
        size += aml_resource_kind_to_header_size[
            UACPI_AML_RESOURCE_KIND_LARGE
        ];

        if (uacpi_unlikely(resource->type != UACPI_RESOURCE_TYPE_VENDOR_LARGE)) {
            uacpi_warn("vendor data too large for small descriptor (%zu), "
                       "correcting to large\n", size);
            resource->type = UACPI_RESOURCE_TYPE_VENDOR_LARGE;
        }
    } else {
        size += aml_resource_kind_to_header_size[
            UACPI_AML_RESOURCE_KIND_SMALL
        ];
    }

    return size;
}

static uacpi_size extra_size_for_resource_source(
    uacpi_size base_size, uacpi_size reported_size
)
{
    uacpi_size string_length;

    if (reported_size <= base_size)
        return 0;

    /*
     * The remainder of the descriptor minus the resource index field
     */
    string_length = (reported_size - base_size) - 1;
    return UACPI_ALIGN_UP(string_length, sizeof(void*), uacpi_size);
}

static uacpi_size size_for_aml_resource_source(
    uacpi_resource_source *source, uacpi_bool with_index
)
{
    uacpi_size length = source->length;

    if (uacpi_unlikely(length && !source->index_present)) {
        uacpi_warn("resource declares no source index with non-empty "
                   "string (%zu bytes), corrected\n", length);
        source->index_present = UACPI_TRUE;
    }

    // If index is included in the dynamic resource source, add it to the length
    if (with_index)
        length += source->index_present;

    return length;
}

static uacpi_size extra_size_for_native_address_or_clock_input(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    UACPI_UNUSED(data);
    return extra_size_for_resource_source(spec->aml_size, size);
}

static uacpi_size size_for_aml_address_or_clock_input(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_resource_source *source;
    bool has_index = UACPI_TRUE;

    switch (resource->type) {
    case UACPI_RESOURCE_TYPE_ADDRESS16:
        source = &resource->address16.source;
        break;
    case UACPI_RESOURCE_TYPE_ADDRESS32:
        source = &resource->address32.source;
        break;
    case UACPI_RESOURCE_TYPE_ADDRESS64:
        source = &resource->address64.source;
        break;
    case UACPI_RESOURCE_TYPE_CLOCK_INPUT:
        source = &resource->clock_input.source;
        has_index = UACPI_FALSE;
        break;
    default:
        return 0;
    }

    return aml_size_with_header(spec) +
           size_for_aml_resource_source(source, has_index);
}

static uacpi_size extra_size_for_extended_irq(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    struct acpi_resource_extended_irq *irq = data;
    uacpi_size extra_size = 0;

    extra_size += irq->num_irqs * sizeof(uacpi_u32);
    extra_size += extra_size_for_resource_source(
        spec->aml_size, size - extra_size
    );

    return extra_size;
}

static uacpi_size size_for_aml_extended_irq(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_resource_extended_irq *irq = &resource->extended_irq;
    uacpi_size size;

    size = aml_size_with_header(spec);
    size += irq->num_irqs * 4;
    size += size_for_aml_resource_source(&irq->source, UACPI_TRUE);

    return size;
}

static uacpi_size extra_size_for_native_gpio_or_pins(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    uacpi_size pin_table_offset;

    /*
     * These resources pretend to have variable layout by declaring "offset"
     * fields, but the layout is hardcoded and mandated by the spec to be
     * very specific. We can use the offset numbers here to calculate the final
     * length.
     *
     * For example, the layout of GPIO connection _always_ looks as follows:
     * [0...22] -> fixed data
     * [23...<source name offset - 1>] -> pin table
     * [<source name offset>...<vendor data offset - 1>] -> source name
     * [<vendor data offset>...<data offset + data length>] -> vendor data
     */
    switch (spec->type) {
    case UACPI_AML_RESOURCE_GPIO_CONNECTION: {
        struct acpi_resource_gpio_connection *gpio = data;
        pin_table_offset = gpio->pin_table_offset;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_FUNCTION: {
        struct acpi_resource_pin_function *pin = data;
        pin_table_offset = pin->pin_table_offset;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_CONFIGURATION: {
        struct acpi_resource_pin_configuration *config = data;
        pin_table_offset = config->pin_table_offset;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_GROUP: {
        struct acpi_resource_pin_group *group = data;
        pin_table_offset = group->pin_table_offset;
        break;
    }

    default:
        return 0;
    }

    /*
     * The size we get passed here does not include the header size because
     * that's how resources are encoded. Subtract it here so that we get the
     * correct final length.
     */
    return size - (pin_table_offset - LARGE_ITEM_HEADER_SIZE);
}

static uacpi_size size_for_aml_gpio_or_pins(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_size source_length, vendor_length, pin_table_length, size;

    size = aml_size_with_header(spec);
    switch (spec->type) {
    case UACPI_AML_RESOURCE_GPIO_CONNECTION: {
        uacpi_resource_gpio_connection *res = &resource->gpio_connection;
        source_length = res->source.length;
        pin_table_length = res->pin_table_length;
        vendor_length = res->vendor_data_length;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_FUNCTION: {
        uacpi_resource_pin_function *res = &resource->pin_function;
        source_length = res->source.length;
        pin_table_length = res->pin_table_length;
        vendor_length = res->vendor_data_length;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_CONFIGURATION: {
        uacpi_resource_pin_configuration *res = &resource->pin_configuration;
        source_length = res->source.length;
        pin_table_length = res->pin_table_length;
        vendor_length = res->vendor_data_length;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_GROUP: {
        uacpi_resource_pin_group *res = &resource->pin_group;
        source_length = res->label.length;
        pin_table_length = res->pin_table_length;
        vendor_length = res->vendor_data_length;
        break;
    }

    default:
        return 0;
    }

    size += source_length;
    size += pin_table_length * 2;
    size += vendor_length;

    return size;
}

static uacpi_size extra_size_for_native_pin_group(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    uacpi_size source_offset;

    switch (spec->type) {
    case UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION: {
        struct acpi_resource_pin_group_function *func = data;
        source_offset = func->source_offset;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION: {
        struct acpi_resource_pin_group_configuration *config = data;
        source_offset = config->source_offset;
        break;
    }

    default:
        return 0;
    }

    // Same logic as extra_size_for_native_gpio_or_pins
    return size - (source_offset - LARGE_ITEM_HEADER_SIZE);
}

static uacpi_size size_for_aml_pin_group(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_size source_length, label_length, vendor_length, size;

    size = aml_size_with_header(spec);
    switch (spec->type) {
    case UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION: {
        uacpi_resource_pin_group_function *res = &resource->pin_group_function;
        source_length = res->source.length;
        label_length = res->label.length;
        vendor_length = res->vendor_data_length;
        break;
    }

    case UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION: {
        uacpi_resource_pin_group_configuration *res;
        res = &resource->pin_group_configuration;
        source_length = res->source.length;
        label_length = res->label.length;
        vendor_length = res->vendor_data_length;
        break;
    }

    default:
        return 0;
    }

    size += source_length;
    size += label_length;
    size += vendor_length;

    return size;
}

#define AML_SERIAL_RESOURCE_EXTRA_SIZE(type)    \
    (sizeof(struct acpi_resource_serial_##type) \
     - sizeof(struct acpi_resource_serial))

#define NATIVE_SERIAL_RESOURCE_EXTRA_SIZE(type) \
    (sizeof(uacpi_resource_##type##_connection)  \
     - sizeof(uacpi_resource_serial_bus_common))

static const uacpi_u8 aml_serial_resource_to_extra_aml_size
[ACPI_SERIAL_TYPE_MAX + 1] = {
    [ACPI_SERIAL_TYPE_I2C] = AML_SERIAL_RESOURCE_EXTRA_SIZE(i2c),
    [ACPI_SERIAL_TYPE_SPI] = AML_SERIAL_RESOURCE_EXTRA_SIZE(spi),
    [ACPI_SERIAL_TYPE_UART] = AML_SERIAL_RESOURCE_EXTRA_SIZE(uart),
    [ACPI_SERIAL_TYPE_CSI2] = AML_SERIAL_RESOURCE_EXTRA_SIZE(csi2),
};

static const uacpi_u8 aml_serial_resource_to_extra_native_size
[ACPI_SERIAL_TYPE_MAX + 1] = {
    [ACPI_SERIAL_TYPE_I2C] = NATIVE_SERIAL_RESOURCE_EXTRA_SIZE(i2c),
    [ACPI_SERIAL_TYPE_SPI] = NATIVE_SERIAL_RESOURCE_EXTRA_SIZE(spi),
    [ACPI_SERIAL_TYPE_UART] = NATIVE_SERIAL_RESOURCE_EXTRA_SIZE(uart),
    [ACPI_SERIAL_TYPE_CSI2] = NATIVE_SERIAL_RESOURCE_EXTRA_SIZE(csi2),
};

static uacpi_size extra_size_for_serial_connection(
    const struct uacpi_resource_spec *spec, void *data, uacpi_size size
)
{
    struct acpi_resource_serial *serial = data;
    uacpi_size extra_bytes = size;

    extra_bytes -= spec->aml_size;
    extra_bytes -= aml_serial_resource_to_extra_aml_size[serial->type];
    extra_bytes += aml_serial_resource_to_extra_native_size[serial->type];

    return extra_bytes;
}

static uacpi_size aml_size_for_serial_connection(
    const struct uacpi_resource_spec *spec, uacpi_resource *resource
)
{
    uacpi_size size;
    uacpi_resource_serial_bus_common *serial_bus = &resource->serial_bus_common;

    size = aml_size_with_header(spec);
    size += aml_serial_resource_to_extra_aml_size[serial_bus->type];
    size += serial_bus->vendor_data_length;
    size += serial_bus->source.length;

    return size;
}

#define OP(short_code, ...)                             \
{                                                       \
    .code = UACPI_RESOURCE_CONVERT_OPCODE_##short_code, \
    __VA_ARGS__                                         \
}

#define END() OP(END)

#define AML_O(short_aml_name, field) \
    uacpi_offsetof(struct acpi_resource_##short_aml_name, field)

#define AML_F(short_aml_name, field) \
    .aml_offset = AML_O(short_aml_name, field)

#define NATIVE_O(short_name, field) \
    uacpi_offsetof(uacpi_resource_##short_name, field)

#define NATIVE_F(short_native_name, field) \
    .native_offset = NATIVE_O(short_native_name, field)

#define IMM(value) .imm = value
#define ARG0(value) .arg0 = (value)
#define ARG1(value) .arg1 = (value)
#define ARG2(value) .arg2 = (value)


static const struct uacpi_resource_convert_instruction convert_irq_to_native[] = {
    OP(PACKED_ARRAY_16, AML_F(irq, irq_mask), NATIVE_F(irq, irqs),
       ARG2(NATIVE_O(irq, num_irqs))),
    OP(SKIP_IF_AML_SIZE_LESS_THAN, ARG0(3), IMM(6)),
        OP(SET_TO_IMM, NATIVE_F(irq, length_kind),
           IMM(UACPI_RESOURCE_LENGTH_KIND_FULL)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, triggering), IMM(0)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, polarity), IMM(3)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, sharing), IMM(4)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, wake_capability), IMM(5)),
        END(),
    OP(SET_TO_IMM, NATIVE_F(irq, length_kind),
       IMM(UACPI_RESOURCE_LENGTH_KIND_ONE_LESS)),
    OP(SET_TO_IMM, NATIVE_F(irq, triggering), IMM(UACPI_TRIGGERING_EDGE)),
    END(),
};

const struct uacpi_resource_convert_instruction convert_irq_to_aml[] = {
    OP(PACKED_ARRAY_16, AML_F(irq, irq_mask), NATIVE_F(irq, irqs),
       ARG2(NATIVE_O(irq, num_irqs))),
    OP(SKIP_IF_AML_SIZE_LESS_THAN, ARG0(3), IMM(4)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, triggering), IMM(0)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, polarity), IMM(3)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, sharing), IMM(4)),
        OP(BIT_FIELD_1, AML_F(irq, flags), NATIVE_F(irq, wake_capability), IMM(5)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_dma[] = {
    OP(PACKED_ARRAY_8, AML_F(dma, channel_mask), NATIVE_F(dma, channels),
       ARG2(NATIVE_O(dma, num_channels))),
    OP(BIT_FIELD_2, AML_F(dma, flags), NATIVE_F(dma, transfer_type), IMM(0)),
    OP(BIT_FIELD_1, AML_F(dma, flags), NATIVE_F(dma, bus_master_status), IMM(2)),
    OP(BIT_FIELD_2, AML_F(dma, flags), NATIVE_F(dma, channel_speed), IMM(5)),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_start_dependent_to_native[] = {
    OP(SKIP_IF_AML_SIZE_LESS_THAN, ARG0(1), IMM(4)),
        OP(SET_TO_IMM, NATIVE_F(start_dependent, length_kind),
           IMM(UACPI_RESOURCE_LENGTH_KIND_FULL)),
        OP(BIT_FIELD_2, AML_F(start_dependent, flags),
           NATIVE_F(start_dependent, compatibility), IMM(0)),
        OP(BIT_FIELD_2, AML_F(start_dependent, flags),
           NATIVE_F(start_dependent, performance), IMM(2)),
        END(),
    OP(SET_TO_IMM, NATIVE_F(start_dependent, length_kind),
       IMM(UACPI_RESOURCE_LENGTH_KIND_ONE_LESS)),
    OP(SET_TO_IMM, NATIVE_F(start_dependent, compatibility),
       IMM(UACPI_ACCEPTABLE)),
    OP(SET_TO_IMM, NATIVE_F(start_dependent, performance),
       IMM(UACPI_ACCEPTABLE)),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_start_dependent_to_aml[] = {
    OP(SKIP_IF_AML_SIZE_LESS_THAN, ARG0(1), IMM(1)),
        OP(BIT_FIELD_2, AML_F(start_dependent, flags),
           NATIVE_F(start_dependent, compatibility), IMM(0)),
        OP(BIT_FIELD_2, AML_F(start_dependent, flags),
           NATIVE_F(start_dependent, performance), IMM(2)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_io[] = {
    OP(BIT_FIELD_1, AML_F(io, information), NATIVE_F(io, decode_type)),
    OP(FIELD_16, AML_F(io, minimum), NATIVE_F(io, minimum)),
    OP(FIELD_16, AML_F(io, maximum), NATIVE_F(io, maximum)),
    OP(FIELD_8, AML_F(io, alignment), NATIVE_F(io, alignment)),
    OP(FIELD_8, AML_F(io, length), NATIVE_F(io, length)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_fixed_io[] = {
    OP(FIELD_16, AML_F(fixed_io, address), NATIVE_F(fixed_io, address)),
    OP(FIELD_8, AML_F(fixed_io, length), NATIVE_F(fixed_io, length)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_fixed_dma[] = {
    OP(FIELD_16, AML_F(fixed_dma, request_line),
                 NATIVE_F(fixed_dma, request_line)),
    OP(FIELD_16, AML_F(fixed_dma, channel), NATIVE_F(fixed_dma, channel)),
    OP(FIELD_8, AML_F(fixed_dma, transfer_width),
                NATIVE_F(fixed_dma, transfer_width)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_vendor_type0[] = {
    OP(LOAD_AML_SIZE_32, NATIVE_F(vendor, length)),
    OP(FIELD_8, AML_F(vendor_defined_type0, byte_data), NATIVE_F(vendor, data)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_vendor_type1[] = {
    OP(LOAD_AML_SIZE_32, NATIVE_F(vendor, length)),
    OP(FIELD_8, AML_F(vendor_defined_type1, byte_data), NATIVE_F(vendor, data)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_memory24[] = {
    OP(BIT_FIELD_1, AML_F(memory24, information),
                    NATIVE_F(memory24, write_status), IMM(0)),
    OP(FIELD_16, AML_F(memory24, minimum), NATIVE_F(memory24, minimum), IMM(4)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_memory32[] = {
    OP(BIT_FIELD_1, AML_F(memory32, information),
                    NATIVE_F(memory32, write_status), IMM(0)),
    OP(FIELD_32, AML_F(memory32, minimum), NATIVE_F(memory32, minimum), IMM(4)),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_fixed_memory32[] = {
    OP(BIT_FIELD_1, AML_F(fixed_memory32, information),
                    NATIVE_F(fixed_memory32, write_status), IMM(0)),
    OP(FIELD_32, AML_F(fixed_memory32, address),
                 NATIVE_F(fixed_memory32, address)),
    OP(FIELD_32, AML_F(fixed_memory32, length),
                 NATIVE_F(fixed_memory32, length)),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_generic_register[] = {
    OP(FIELD_8, AML_F(generic_register, address_space_id),
                NATIVE_F(generic_register, address_space_id), IMM(4)),
    OP(FIELD_64, AML_F(generic_register, address),
                NATIVE_F(generic_register, address)),
    END(),
};

#define CONVERT_TYPE_SPECIFIC_FLAGS(addr_type)                                 \
    OP(LOAD_8_STORE, AML_F(addr_type, common.type),                            \
       NATIVE_F(addr_type, common.type)),                                      \
    OP(SKIP_IF_NOT_EQUALS, ARG0(UACPI_RANGE_MEMORY), IMM(5)),                  \
        OP(BIT_FIELD_1,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.memory.write_status), IMM(0)), \
        OP(BIT_FIELD_2,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.memory.caching), IMM(1)),      \
        OP(BIT_FIELD_2,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.memory.range_type), IMM(3)),   \
        OP(BIT_FIELD_1,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.memory.translation), IMM(5)),  \
        END(),                                                                 \
    OP(SKIP_IF_NOT_EQUALS, ARG0(UACPI_RANGE_IO), IMM(4)),                      \
        OP(BIT_FIELD_2,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.io.range_type), IMM(0)),       \
        OP(BIT_FIELD_1,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.io.translation_type), IMM(4)), \
        OP(BIT_FIELD_1,                                                        \
           AML_F(addr_type, common.type_flags),                                \
           NATIVE_F(addr_type, common.attribute.io.translation), IMM(5)),      \
        END(),                                                                 \
    /* Memory type that we don't know, just copy the byte */                   \
    OP(FIELD_8, AML_F(addr_type, common.type_flags),                           \
       NATIVE_F(addr_type, common.attribute.type_specific), IMM(0xFF)),        \
    END()

#define CONVERT_GENERAL_ADDRESS_FLAGS(addr_type)               \
    OP(BIT_FIELD_1,                                            \
       AML_F(addr_type, common.flags),                         \
       NATIVE_F(addr_type, common.direction), IMM(0)),         \
    OP(BIT_FIELD_1,                                            \
       AML_F(addr_type, common.flags),                         \
       NATIVE_F(addr_type, common.decode_type), IMM(1)),       \
    OP(BIT_FIELD_1,                                            \
       AML_F(addr_type, common.flags),                         \
       NATIVE_F(addr_type, common.fixed_min_address), IMM(2)), \
    OP(BIT_FIELD_1,                                            \
      AML_F(addr_type, common.flags),                          \
      NATIVE_F(addr_type, common.fixed_max_address), IMM(3))   \

#define DEFINE_ADDRESS_CONVERSION(width)                              \
    static const struct uacpi_resource_convert_instruction            \
    convert_address##width[] = {                                      \
        CONVERT_GENERAL_ADDRESS_FLAGS(address##width),                \
        OP(FIELD_##width, AML_F(address##width, granularity),         \
           NATIVE_F(address##width, granularity), IMM(5)),            \
        OP(RESOURCE_SOURCE, NATIVE_F(address##width, source)),        \
        CONVERT_TYPE_SPECIFIC_FLAGS(address##width),                  \
    };

DEFINE_ADDRESS_CONVERSION(16)
DEFINE_ADDRESS_CONVERSION(32)
DEFINE_ADDRESS_CONVERSION(64)

static const struct uacpi_resource_convert_instruction
convert_address64_extended[] = {
    CONVERT_GENERAL_ADDRESS_FLAGS(address64_extended),
    OP(FIELD_8, AML_F(address64_extended, revision_id),
       NATIVE_F(address64_extended, revision_id)),
    OP(FIELD_64, AML_F(address64_extended, granularity),
       NATIVE_F(address64_extended, granularity), IMM(6)),
    CONVERT_TYPE_SPECIFIC_FLAGS(address64_extended),
};

static const struct uacpi_resource_convert_instruction
convert_extended_irq[] = {
    OP(BIT_FIELD_1, AML_F(extended_irq, flags),
                    NATIVE_F(extended_irq, direction), IMM(0)),
    OP(BIT_FIELD_1, AML_F(extended_irq, flags),
                    NATIVE_F(extended_irq, triggering), IMM(1)),
    OP(BIT_FIELD_1, AML_F(extended_irq, flags),
                    NATIVE_F(extended_irq, polarity), IMM(2)),
    OP(BIT_FIELD_1, AML_F(extended_irq, flags),
                    NATIVE_F(extended_irq, sharing), IMM(3)),
    OP(BIT_FIELD_1, AML_F(extended_irq, flags),
                    NATIVE_F(extended_irq, wake_capability), IMM(4)),
    OP(LOAD_8_STORE, AML_F(extended_irq, num_irqs),
       NATIVE_F(extended_irq, num_irqs), IMM(4)),
    OP(RESOURCE_SOURCE, NATIVE_F(extended_irq, source)),

    // Use FIELD_8 here since the accumulator has been multiplied by 4
    OP(FIELD_8, AML_F(extended_irq, irqs), NATIVE_F(extended_irq, irqs)),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_clock_input[] = {
    OP(FIELD_8, AML_F(clock_input, revision_id),
       NATIVE_F(clock_input, revision_id)),
    OP(BIT_FIELD_1, AML_F(clock_input, flags), NATIVE_F(clock_input, frequency),
       IMM(0)),
    OP(BIT_FIELD_2, AML_F(clock_input, flags), NATIVE_F(clock_input, scale),
       IMM(1)),
    OP(FIELD_16, AML_F(clock_input, divisor), NATIVE_F(clock_input, divisor)),
    OP(FIELD_32, AML_F(clock_input, numerator), NATIVE_F(clock_input, numerator)),
    OP(FIELD_8, AML_F(clock_input, source_index), NATIVE_F(clock_input, source.index)),
    OP(RESOURCE_SOURCE_NO_INDEX, NATIVE_F(clock_input, source)),
    END(),
};

#define DECODE_SOURCE_INDEX(short_aml_name)          \
    OP(FIELD_8, AML_F(short_aml_name, source_index), \
       NATIVE_F(short_aml_name, source.index))       \

#define DECODE_RES_PIN_TBL_AND_VENDOR_DATA(                            \
    short_aml_name, res_opcode, offset_field, res_field                \
)                                                                      \
    OP(LOAD_PIN_TABLE_LENGTH, AML_F(short_aml_name, offset_field),     \
       NATIVE_F(short_aml_name, pin_table_length)),                    \
    OP(RESOURCE_##res_opcode, NATIVE_F(short_aml_name, res_field),     \
       AML_F(short_aml_name, offset_field),                            \
       ARG2(AML_O(short_aml_name, vendor_data_offset))),               \
    OP(PIN_TABLE, AML_F(short_aml_name, pin_table_offset),             \
       NATIVE_F(short_aml_name, pin_table_length),                     \
       ARG2(NATIVE_O(short_aml_name, pin_table))),                     \
    OP(VENDOR_DATA, AML_F(short_aml_name, vendor_data_offset),         \
       NATIVE_F(short_aml_name, vendor_data_length),                   \
       ARG2(NATIVE_O(short_aml_name, vendor_data)))

static const struct uacpi_resource_convert_instruction
convert_gpio_connection[] = {
    OP(FIELD_8, AML_F(gpio_connection, revision_id),
       NATIVE_F(gpio_connection, revision_id)),
    OP(BIT_FIELD_1, AML_F(gpio_connection, general_flags),
       NATIVE_F(gpio_connection, direction)),
    OP(FIELD_8, AML_F(gpio_connection, pull_configuration),
       NATIVE_F(gpio_connection, pull_configuration)),
    OP(FIELD_16, AML_F(gpio_connection, drive_strength),
       NATIVE_F(gpio_connection, drive_strength), IMM(2)),
    DECODE_SOURCE_INDEX(gpio_connection),
    DECODE_RES_PIN_TBL_AND_VENDOR_DATA(
        gpio_connection, SOURCE_NO_INDEX, source_offset, source
    ),
    OP(LOAD_8_STORE, AML_F(gpio_connection, type), NATIVE_F(gpio_connection, type)),
    OP(SKIP_IF_NOT_EQUALS, ARG0(UACPI_GPIO_CONNECTION_INTERRUPT), IMM(5)),
        OP(BIT_FIELD_1, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, interrupt.triggering), IMM(0)),
        OP(BIT_FIELD_2, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, interrupt.polarity), IMM(1)),
        OP(BIT_FIELD_1, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, interrupt.sharing), IMM(3)),
        OP(BIT_FIELD_1, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, interrupt.wake_capability), IMM(4)),
        END(),
    OP(SKIP_IF_NOT_EQUALS, ARG0(UACPI_GPIO_CONNECTION_IO), IMM(3)),
        OP(BIT_FIELD_2, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, io.restriction), IMM(0)),
        OP(BIT_FIELD_1, AML_F(gpio_connection, connection_flags),
           NATIVE_F(gpio_connection, io.sharing), IMM(3)),
        END(),
    OP(FIELD_16, AML_F(gpio_connection, connection_flags),
       NATIVE_F(gpio_connection, type_specific), IMM(0xFF)),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_pin_function[] = {
    OP(FIELD_8, AML_F(pin_function, revision_id),
       NATIVE_F(pin_function, revision_id)),
    OP(BIT_FIELD_1, AML_F(pin_function, flags),
       NATIVE_F(pin_function, sharing), IMM(0)),
    OP(FIELD_8, AML_F(pin_function, pull_configuration),
       NATIVE_F(pin_function, pull_configuration)),
    OP(FIELD_16, AML_F(pin_function, function_number),
       NATIVE_F(pin_function, function_number)),
    DECODE_SOURCE_INDEX(pin_function),
    DECODE_RES_PIN_TBL_AND_VENDOR_DATA(
        pin_function, SOURCE_NO_INDEX, source_offset, source
    ),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_pin_configuration[] = {
    OP(FIELD_8, AML_F(pin_configuration, revision_id),
       NATIVE_F(pin_configuration, revision_id)),
    OP(BIT_FIELD_1, AML_F(pin_configuration, flags),
       NATIVE_F(pin_configuration, sharing), IMM(0)),
    OP(BIT_FIELD_1, AML_F(pin_configuration, flags),
       NATIVE_F(pin_configuration, direction), IMM(1)),
    OP(FIELD_8, AML_F(pin_configuration, type),
       NATIVE_F(pin_configuration, type)),
    OP(FIELD_32, AML_F(pin_configuration, value),
       NATIVE_F(pin_configuration, value)),
    DECODE_SOURCE_INDEX(pin_configuration),
    DECODE_RES_PIN_TBL_AND_VENDOR_DATA(
        pin_configuration, SOURCE_NO_INDEX, source_offset, source
    ),
    END(),
};

static const struct uacpi_resource_convert_instruction convert_pin_group[] = {
    OP(FIELD_8, AML_F(pin_group, revision_id),
       NATIVE_F(pin_group, revision_id)),
    OP(BIT_FIELD_1, AML_F(pin_group, flags),
       NATIVE_F(pin_group, direction), IMM(0)),
    DECODE_RES_PIN_TBL_AND_VENDOR_DATA(
        pin_group, LABEL, source_lable_offset, label
    ),
    END(),
};

#define DECODE_PIN_GROUP_RES_SOURCES(postfix)                           \
    DECODE_SOURCE_INDEX(pin_group_##postfix),                           \
    OP(RESOURCE_SOURCE_NO_INDEX, NATIVE_F(pin_group_##postfix, source), \
       AML_F(pin_group_##postfix, source_offset),                       \
       ARG2(AML_O(pin_group_##postfix, source_lable_offset))),          \
    OP(LOAD_16_NATIVE, NATIVE_F(pin_group_##postfix, source.length)),   \
    OP(RESOURCE_LABEL, NATIVE_F(pin_group_##postfix, label),            \
       AML_F(pin_group_##postfix, source_lable_offset),                 \
       ARG2(AML_O(pin_group_##postfix, vendor_data_offset))),           \
    OP(VENDOR_DATA, AML_F(pin_group_##postfix, vendor_data_offset),     \
       NATIVE_F(pin_group_##postfix, vendor_data_length),               \
       ARG2(NATIVE_O(pin_group_##postfix, vendor_data)))

static const struct uacpi_resource_convert_instruction
convert_pin_group_function[] = {
    OP(FIELD_8, AML_F(pin_group_function, revision_id),
       NATIVE_F(pin_group_function, revision_id)),
    OP(BIT_FIELD_1, AML_F(pin_group_function, flags),
       NATIVE_F(pin_group_function, sharing), IMM(0)),
    OP(BIT_FIELD_1, AML_F(pin_group_function, flags),
       NATIVE_F(pin_group_function, direction), IMM(1)),
    OP(FIELD_16, AML_F(pin_group_function, function),
       NATIVE_F(pin_group_function, function)),
    DECODE_PIN_GROUP_RES_SOURCES(function),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_pin_group_configuration[] = {
    OP(FIELD_8, AML_F(pin_group_configuration, revision_id),
       NATIVE_F(pin_group_configuration, revision_id)),
    OP(BIT_FIELD_1, AML_F(pin_group_configuration, flags),
       NATIVE_F(pin_group_configuration, sharing), IMM(0)),
    OP(BIT_FIELD_1, AML_F(pin_group_configuration, flags),
       NATIVE_F(pin_group_configuration, direction), IMM(1)),
    OP(FIELD_8, AML_F(pin_group_configuration, type),
       NATIVE_F(pin_group_configuration, type)),
    OP(FIELD_32, AML_F(pin_group_configuration, value),
       NATIVE_F(pin_group_configuration, value)),
    DECODE_PIN_GROUP_RES_SOURCES(configuration),
    END(),
};

static const struct uacpi_resource_convert_instruction
convert_generic_serial_bus[] = {
    OP(FIELD_8, AML_F(serial, revision_id),
       NATIVE_F(serial_bus_common, revision_id)),
    OP(FIELD_8, AML_F(serial, type_specific_revision_id),
       NATIVE_F(serial_bus_common, type_revision_id)),
    OP(FIELD_8, AML_F(serial, source_index),
       NATIVE_F(serial_bus_common, source.index)),
    OP(FIELD_16, AML_F(serial, type_data_length),
       NATIVE_F(serial_bus_common, type_data_length)),
    OP(BIT_FIELD_1, AML_F(serial, flags),
       NATIVE_F(serial_bus_common, mode), IMM(0)),
    OP(BIT_FIELD_1, AML_F(serial, flags),
       NATIVE_F(serial_bus_common, direction), IMM(1)),
    OP(BIT_FIELD_1, AML_F(serial, flags),
       NATIVE_F(serial_bus_common, sharing), IMM(2)),
    OP(SERIAL_TYPE_SPECIFIC, AML_F(serial, type),
       NATIVE_F(serial_bus_common, type)),
    OP(RESOURCE_SOURCE_NO_INDEX, NATIVE_F(serial_bus_common, source)),
    OP(LOAD_8_NATIVE, NATIVE_F(serial_bus_common, type)),
    OP(SKIP_IF_NOT_EQUALS, ARG0(ACPI_SERIAL_TYPE_I2C), IMM(4)),
        OP(BIT_FIELD_1, AML_F(serial, type_specific_flags),
           NATIVE_F(i2c_connection, addressing_mode), IMM(0)),
        OP(FIELD_32, AML_F(serial_i2c, connection_speed),
           NATIVE_F(i2c_connection, connection_speed), IMM(0xFF)),
        OP(FIELD_16, AML_F(serial_i2c, slave_address),
           NATIVE_F(i2c_connection, slave_address)),
        END(),
    OP(SKIP_IF_NOT_EQUALS, ARG0(ACPI_SERIAL_TYPE_SPI), IMM(5)),
        OP(BIT_FIELD_1, AML_F(serial, type_specific_flags),
           NATIVE_F(spi_connection, wire_mode), IMM(0)),
        OP(BIT_FIELD_1, AML_F(serial, type_specific_flags),
           NATIVE_F(spi_connection, device_polarity), IMM(1)),
        OP(FIELD_32, AML_F(serial_spi, connection_speed),
           NATIVE_F(spi_connection, connection_speed), IMM(0xFF)),
        OP(FIELD_8, AML_F(serial_spi, data_bit_length),
           NATIVE_F(spi_connection, data_bit_length), IMM(5)),
        END(),
    OP(SKIP_IF_NOT_EQUALS, ARG0(ACPI_SERIAL_TYPE_UART), IMM(8)),
        OP(BIT_FIELD_2, AML_F(serial, type_specific_flags),
           NATIVE_F(uart_connection, flow_control), IMM(0)),
        OP(BIT_FIELD_2, AML_F(serial, type_specific_flags),
           NATIVE_F(uart_connection, stop_bits), IMM(2)),
        OP(BIT_FIELD_3, AML_F(serial, type_specific_flags),
           NATIVE_F(uart_connection, data_bits), IMM(4)),
        OP(BIT_FIELD_1, AML_F(serial, type_specific_flags),
           NATIVE_F(uart_connection, endianness), IMM(7)),
        OP(FIELD_32, AML_F(serial_uart, baud_rate),
           NATIVE_F(uart_connection, baud_rate), IMM(0xFF)),
        OP(FIELD_16, AML_F(serial_uart, rx_fifo),
           NATIVE_F(uart_connection, rx_fifo), IMM(2)),
        OP(FIELD_8, AML_F(serial_uart, parity),
           NATIVE_F(uart_connection, parity), IMM(2)),
        END(),
    OP(SKIP_IF_NOT_EQUALS, ARG0(ACPI_SERIAL_TYPE_CSI2), IMM(3)),
        OP(BIT_FIELD_2, AML_F(serial, type_specific_flags),
           NATIVE_F(csi2_connection, phy_type), IMM(0)),
        OP(BIT_FIELD_6, AML_F(serial, type_specific_flags),
           NATIVE_F(csi2_connection, local_port), IMM(2)),
        END(),

    /*
     * Insert a trap to catch unimplemented types, this should be unreachable
     * because of validation earlier.
     */
    OP(UNREACHABLE),
};

#define NATIVE_RESOURCE_HEADER_SIZE 8

#define DEFINE_SMALL_AML_RESOURCE(aml_type_enum, native_type_enum,           \
                                  aml_struct, native_struct, ...)            \
    [aml_type_enum] = {                                                      \
        .type = aml_type_enum,                                               \
        .native_type = native_type_enum,                                     \
        .resource_kind = UACPI_AML_RESOURCE_KIND_SMALL,                      \
        .aml_size = sizeof(aml_struct) - SMALL_ITEM_HEADER_SIZE,             \
        .native_size = sizeof(native_struct) + NATIVE_RESOURCE_HEADER_SIZE,  \
        __VA_ARGS__                                                          \
    }

#define DEFINE_SMALL_AML_RESOURCE_NO_NATIVE_REPR(                \
    aml_type_enum, native_type_enum, aml_struct, ...             \
)                                                                \
    [aml_type_enum] = {                                          \
        .type = aml_type_enum,                                   \
        .native_type = native_type_enum,                         \
        .resource_kind = UACPI_AML_RESOURCE_KIND_SMALL,          \
        .aml_size = sizeof(aml_struct) - SMALL_ITEM_HEADER_SIZE, \
        .native_size = NATIVE_RESOURCE_HEADER_SIZE,              \
        __VA_ARGS__                                              \
    }

#define DEFINE_LARGE_AML_RESOURCE(aml_type_enum, native_type_enum,           \
                                  aml_struct, native_struct, ...)            \
    [aml_type_enum] = {                                                      \
        .type = aml_type_enum,                                               \
        .native_type = native_type_enum,                                     \
        .resource_kind = UACPI_AML_RESOURCE_KIND_LARGE,                      \
        .aml_size = sizeof(aml_struct) - LARGE_ITEM_HEADER_SIZE,             \
        .native_size = sizeof(native_struct) + NATIVE_RESOURCE_HEADER_SIZE,  \
        __VA_ARGS__                                                          \
    }

const struct uacpi_resource_spec aml_resources[UACPI_AML_RESOURCE_MAX + 1] = {
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_IRQ,
        UACPI_RESOURCE_TYPE_IRQ,
        struct acpi_resource_irq,
        uacpi_resource_irq,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED_OR_ONE_LESS,
        .extra_size_for_native = extra_size_for_native_irq_or_dma,
        .size_for_aml = size_for_aml_irq,
        .to_native = convert_irq_to_native,
        .to_aml = convert_irq_to_aml,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_DMA,
        UACPI_RESOURCE_TYPE_DMA,
        struct acpi_resource_dma,
        uacpi_resource_dma,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .extra_size_for_native = extra_size_for_native_irq_or_dma,
        .to_native = convert_dma,
        .to_aml = convert_dma,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_START_DEPENDENT,
        UACPI_RESOURCE_TYPE_START_DEPENDENT,
        struct acpi_resource_start_dependent,
        uacpi_resource_start_dependent,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED_OR_ONE_LESS,
        .size_for_aml = size_for_aml_start_dependent,
        .to_native = convert_start_dependent_to_native,
        .to_aml = convert_start_dependent_to_aml,
    ),
    DEFINE_SMALL_AML_RESOURCE_NO_NATIVE_REPR(
        UACPI_AML_RESOURCE_END_DEPENDENT,
        UACPI_RESOURCE_TYPE_END_DEPENDENT,
        struct acpi_resource_end_dependent,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_IO,
        UACPI_RESOURCE_TYPE_IO,
        struct acpi_resource_io,
        uacpi_resource_io,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_io,
        .to_aml = convert_io,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_FIXED_IO,
        UACPI_RESOURCE_TYPE_FIXED_IO,
        struct acpi_resource_fixed_io,
        uacpi_resource_fixed_io,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_fixed_io,
        .to_aml = convert_fixed_io,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_FIXED_DMA,
        UACPI_RESOURCE_TYPE_FIXED_DMA,
        struct acpi_resource_fixed_dma,
        uacpi_resource_fixed_dma,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_fixed_dma,
        .to_aml = convert_fixed_dma,
    ),
    DEFINE_SMALL_AML_RESOURCE(
        UACPI_AML_RESOURCE_VENDOR_TYPE0,
        UACPI_RESOURCE_TYPE_VENDOR_SMALL,
        struct acpi_resource_vendor_defined_type0,
        uacpi_resource_vendor,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .size_for_aml = size_for_aml_vendor,
        .extra_size_for_native = extra_size_for_native_vendor,
        .to_native = convert_vendor_type0,
        .to_aml = convert_vendor_type0,
    ),
    DEFINE_SMALL_AML_RESOURCE_NO_NATIVE_REPR(
        UACPI_AML_RESOURCE_END_TAG,
        UACPI_RESOURCE_TYPE_END_TAG,
        struct acpi_resource_end_tag,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_MEMORY24,
        UACPI_RESOURCE_TYPE_MEMORY24,
        struct acpi_resource_memory24,
        uacpi_resource_memory24,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_memory24,
        .to_aml = convert_memory24,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_GENERIC_REGISTER,
        UACPI_RESOURCE_TYPE_GENERIC_REGISTER,
        struct acpi_resource_generic_register,
        uacpi_resource_generic_register,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_generic_register,
        .to_aml = convert_generic_register,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_VENDOR_TYPE1,
        UACPI_RESOURCE_TYPE_VENDOR_LARGE,
        struct acpi_resource_vendor_defined_type1,
        uacpi_resource_vendor,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_vendor,
        .size_for_aml = size_for_aml_vendor,
        .to_native = convert_vendor_type1,
        .to_aml = convert_vendor_type1,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_MEMORY32,
        UACPI_RESOURCE_TYPE_MEMORY32,
        struct acpi_resource_memory32,
        uacpi_resource_memory32,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_memory32,
        .to_aml = convert_memory32,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_FIXED_MEMORY32,
        UACPI_RESOURCE_TYPE_FIXED_MEMORY32,
        struct acpi_resource_fixed_memory32,
        uacpi_resource_fixed_memory32,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_fixed_memory32,
        .to_aml = convert_fixed_memory32,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_ADDRESS32,
        UACPI_RESOURCE_TYPE_ADDRESS32,
        struct acpi_resource_address32,
        uacpi_resource_address32,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_address_or_clock_input,
        .size_for_aml = size_for_aml_address_or_clock_input,
        .to_native = convert_address32,
        .to_aml = convert_address32,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_ADDRESS16,
        UACPI_RESOURCE_TYPE_ADDRESS16,
        struct acpi_resource_address16,
        uacpi_resource_address16,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_address_or_clock_input,
        .size_for_aml = size_for_aml_address_or_clock_input,
        .to_native = convert_address16,
        .to_aml = convert_address16,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_EXTENDED_IRQ,
        UACPI_RESOURCE_TYPE_EXTENDED_IRQ,
        struct acpi_resource_extended_irq,
        uacpi_resource_extended_irq,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_extended_irq,
        .size_for_aml = size_for_aml_extended_irq,
        .to_native = convert_extended_irq,
        .to_aml = convert_extended_irq,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_ADDRESS64,
        UACPI_RESOURCE_TYPE_ADDRESS64,
        struct acpi_resource_address64,
        uacpi_resource_address64,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_address_or_clock_input,
        .size_for_aml = size_for_aml_address_or_clock_input,
        .to_native = convert_address64,
        .to_aml = convert_address64,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_ADDRESS64_EXTENDED,
        UACPI_RESOURCE_TYPE_ADDRESS64_EXTENDED,
        struct acpi_resource_address64_extended,
        uacpi_resource_address64_extended,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_FIXED,
        .to_native = convert_address64_extended,
        .to_aml = convert_address64_extended,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_GPIO_CONNECTION,
        UACPI_RESOURCE_TYPE_GPIO_CONNECTION,
        struct acpi_resource_gpio_connection,
        uacpi_resource_gpio_connection,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_gpio_or_pins,
        .size_for_aml = size_for_aml_gpio_or_pins,
        .to_aml = convert_gpio_connection,
        .to_native = convert_gpio_connection,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_PIN_FUNCTION,
        UACPI_RESOURCE_TYPE_PIN_FUNCTION,
        struct acpi_resource_pin_function,
        uacpi_resource_pin_function,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_gpio_or_pins,
        .size_for_aml = size_for_aml_gpio_or_pins,
        .to_aml = convert_pin_function,
        .to_native = convert_pin_function,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_SERIAL_CONNECTION,
        0, // the native type here is determined dynamically
        struct acpi_resource_serial,
        uacpi_resource_serial_bus_common,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_serial_connection,
        .size_for_aml = aml_size_for_serial_connection,
        .to_native = convert_generic_serial_bus,
        .to_aml = convert_generic_serial_bus,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_PIN_CONFIGURATION,
        UACPI_RESOURCE_TYPE_PIN_CONFIGURATION,
        struct acpi_resource_pin_configuration,
        uacpi_resource_pin_configuration,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_gpio_or_pins,
        .size_for_aml = size_for_aml_gpio_or_pins,
        .to_native = convert_pin_configuration,
        .to_aml = convert_pin_configuration,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_PIN_GROUP,
        UACPI_RESOURCE_TYPE_PIN_GROUP,
        struct acpi_resource_pin_group,
        uacpi_resource_pin_group,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_gpio_or_pins,
        .size_for_aml = size_for_aml_gpio_or_pins,
        .to_native = convert_pin_group,
        .to_aml = convert_pin_group,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_PIN_GROUP_FUNCTION,
        UACPI_RESOURCE_TYPE_PIN_GROUP_FUNCTION,
        struct acpi_resource_pin_group_function,
        uacpi_resource_pin_group_function,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_pin_group,
        .size_for_aml = size_for_aml_pin_group,
        .to_native = convert_pin_group_function,
        .to_aml = convert_pin_group_function,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_PIN_GROUP_CONFIGURATION,
        UACPI_RESOURCE_TYPE_PIN_GROUP_CONFIGURATION,
        struct acpi_resource_pin_group_configuration,
        uacpi_resource_pin_group_configuration,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_pin_group,
        .size_for_aml = size_for_aml_pin_group,
        .to_native = convert_pin_group_configuration,
        .to_aml = convert_pin_group_configuration,
    ),
    DEFINE_LARGE_AML_RESOURCE(
        UACPI_AML_RESOURCE_CLOCK_INPUT,
        UACPI_RESOURCE_TYPE_CLOCK_INPUT,
        struct acpi_resource_clock_input,
        uacpi_resource_clock_input,
        .size_kind =  UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE,
        .extra_size_for_native = extra_size_for_native_address_or_clock_input,
        .size_for_aml = size_for_aml_address_or_clock_input,
        .to_native = convert_clock_input,
        .to_aml = convert_clock_input,
    ),
};

static enum uacpi_aml_resource get_aml_resource_type(uacpi_u8 raw_byte)
{
    if (raw_byte & ACPI_LARGE_ITEM) {
        return aml_resource_to_type[
            LARGE_RESOURCE_BASE + (raw_byte & ACPI_LARGE_ITEM_NAME_MASK)
        ];
    }

    return aml_resource_to_type[
        (raw_byte >> ACPI_SMALL_ITEM_NAME_IDX) & ACPI_SMALL_ITEM_NAME_MASK
    ];
}

static uacpi_status get_aml_resource_size(
    uacpi_u8 *data, uacpi_size bytes_left, uacpi_u16 *out_size
)
{
    uacpi_u16 size;

    /*
     * Resource header is not included in size for both, so we subtract
     * the header size from bytes_left to validate it.
     */
    if (*data & ACPI_LARGE_ITEM) {
        if (uacpi_unlikely(bytes_left < 3))
            return UACPI_STATUS_AML_INVALID_RESOURCE;

        uacpi_memcpy(&size, data + 1, sizeof(size));
        bytes_left -= aml_resource_kind_to_header_size[
            UACPI_AML_RESOURCE_KIND_LARGE
        ];
    } else {
        size = *data & ACPI_SMALL_ITEM_LENGTH_MASK;
        bytes_left -= aml_resource_kind_to_header_size[
            UACPI_AML_RESOURCE_KIND_SMALL
        ];
    }

    if (uacpi_unlikely(size > bytes_left))
        return UACPI_STATUS_AML_INVALID_RESOURCE;

    *out_size = size;
    return UACPI_STATUS_OK;
}

static uacpi_status validate_aml_serial_type(uacpi_u8 type)
{
    if (uacpi_unlikely(type < ACPI_SERIAL_TYPE_I2C ||
                       type > ACPI_SERIAL_TYPE_CSI2)) {
        uacpi_error("invalid/unsupported serial connection type %d\n", type);
        return UACPI_STATUS_AML_INVALID_RESOURCE;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_for_each_aml_resource(
    uacpi_data_view buffer, uacpi_aml_resource_iteration_callback cb, void *user
)
{
    uacpi_status ret;
    uacpi_iteration_decision decision;
    uacpi_u8 *data;
    uacpi_size bytes_left;
    uacpi_u16 resource_size;
    enum uacpi_aml_resource type;
    const struct uacpi_resource_spec *spec;

    bytes_left = buffer.length;
    data = buffer.bytes;

    while (bytes_left) {
        type = get_aml_resource_type(*data);
        if (uacpi_unlikely(type == UACPI_AML_RESOURCE_TYPE_INVALID))
            return UACPI_STATUS_AML_INVALID_RESOURCE;

        ret = get_aml_resource_size(data, bytes_left, &resource_size);
        if (uacpi_unlikely_error(ret))
            return ret;

        spec = &aml_resources[type];
        switch (spec->size_kind) {
        case UACPI_AML_RESOURCE_SIZE_KIND_FIXED:
            if (resource_size != spec->aml_size)
                return UACPI_STATUS_AML_INVALID_RESOURCE;
            break;
        case UACPI_AML_RESOURCE_SIZE_KIND_VARIABLE:
            if (resource_size < spec->aml_size)
                return UACPI_STATUS_AML_INVALID_RESOURCE;
            break;
        case UACPI_AML_RESOURCE_SIZE_KIND_FIXED_OR_ONE_LESS:
            if (resource_size != spec->aml_size &&
                resource_size != (spec->aml_size - 1))
                return UACPI_STATUS_AML_INVALID_RESOURCE;
            break;
        default:
            return UACPI_STATUS_INTERNAL_ERROR;
        }

        if (spec->type == UACPI_AML_RESOURCE_SERIAL_CONNECTION) {
            struct acpi_resource_serial *serial;

            serial = (struct acpi_resource_serial*)data;

            ret = validate_aml_serial_type(serial->type);
            if (uacpi_unlikely_error(ret))
                return ret;
        }

        decision = cb(user, data, resource_size, spec);
        switch (decision) {
        case UACPI_ITERATION_DECISION_BREAK:
            return UACPI_STATUS_OK;
        case UACPI_ITERATION_DECISION_CONTINUE: {
            uacpi_size total_size = resource_size;

            total_size += aml_resource_kind_to_header_size[spec->resource_kind];
            data += total_size;
            bytes_left -= total_size;
            break;
        }
        default:
            return UACPI_STATUS_INTERNAL_ERROR;
        }

        if (type == UACPI_AML_RESOURCE_END_TAG)
            return UACPI_STATUS_OK;
    }

    return UACPI_STATUS_NO_RESOURCE_END_TAG;
}

static uacpi_iteration_decision find_end(
    void *opaque, uacpi_u8 *data, uacpi_u16 resource_size,
    const struct uacpi_resource_spec *spec
)
{
    uacpi_u8 **out_ptr = opaque;
    UACPI_UNUSED(resource_size);

    if (spec->type != UACPI_AML_RESOURCE_END_TAG)
        return UACPI_ITERATION_DECISION_CONTINUE;

    *out_ptr = data;
    return UACPI_ITERATION_DECISION_BREAK;
}

static uacpi_size native_size_for_aml_resource(
    uacpi_u8 *data, uacpi_u16 size, const struct uacpi_resource_spec *spec
)
{
    uacpi_size final_size = spec->native_size;

    if (spec->extra_size_for_native)
        final_size += spec->extra_size_for_native(spec, data, size);

    return UACPI_ALIGN_UP(final_size, sizeof(void*), uacpi_size);
}

uacpi_status uacpi_find_aml_resource_end_tag(
    uacpi_data_view buffer, uacpi_size *out_offset
)
{
    uacpi_u8 *end_tag_ptr = UACPI_NULL;
    uacpi_status ret;

    if (buffer.length == 0) {
        *out_offset = 0;
        return UACPI_STATUS_OK;
    }

    /*
     * This returning UACPI_STATUS_OK guarantees that end_tag_ptr is set to
     * a valid value because a missing end tag would produce a
     * UACPI_STATUS_NO_RESOURCE_END_TAG error.
     */
    ret = uacpi_for_each_aml_resource(buffer, find_end, &end_tag_ptr);
    if (uacpi_unlikely_error(ret))
        return ret;

    *out_offset = end_tag_ptr - buffer.bytes;
    return UACPI_STATUS_OK;
}

struct resource_conversion_ctx {
    union {
        void *buf;
        uacpi_u8 *byte_buf;
        uacpi_size size;
    };
    uacpi_status st;
    uacpi_bool just_one;
};

static uacpi_iteration_decision conditional_continue(
    struct resource_conversion_ctx *ctx
)
{
    return ctx->just_one ? UACPI_ITERATION_DECISION_BREAK :
                           UACPI_ITERATION_DECISION_CONTINUE;
}

// Opcodes that are the same for both AML->native and native->AML
#define CONVERSION_OPCODES_COMMON(native_buf)                                \
    case UACPI_RESOURCE_CONVERT_OPCODE_END:                                  \
        return conditional_continue(ctx);                                    \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_FIELD_8:                              \
    case UACPI_RESOURCE_CONVERT_OPCODE_FIELD_16:                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_FIELD_32:                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_FIELD_64: {                           \
        uacpi_u8 bytes;                                                      \
                                                                             \
        bytes = 1 << (insn->code - UACPI_RESOURCE_CONVERT_OPCODE_FIELD_8);   \
        accumulator = insn->imm == 0xFF ? 0 : accumulator + insn->imm;       \
                                                                             \
        uacpi_memcpy(dst, src, bytes * UACPI_MAX(1, accumulator));           \
        accumulator = 0;                                                     \
        break;                                                               \
    }                                                                        \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_SKIP_IF_AML_SIZE_LESS_THAN:           \
        if (aml_size < insn->arg0)                                           \
            pc += insn->imm;                                                 \
        break;                                                               \
    case UACPI_RESOURCE_CONVERT_OPCODE_SKIP_IF_NOT_EQUALS:                   \
        if (insn->arg0 != accumulator)                                       \
            pc += insn->imm;                                                 \
        break;                                                               \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_SET_TO_IMM:                           \
        uacpi_memcpy(dst, &insn->imm, sizeof(insn->imm));                    \
        break;                                                               \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_IMM:                             \
        accumulator = insn->imm;                                             \
        break;                                                               \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_8_STORE:                         \
        uacpi_memcpy_zerout(&accumulator, src, sizeof(accumulator), 1);      \
        uacpi_memcpy(dst, &accumulator, 1);                                  \
                                                                             \
        if (insn->imm)                                                       \
            accumulator *= insn->imm;                                        \
        break;                                                               \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_8_NATIVE:                        \
    case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_16_NATIVE: {                     \
        uacpi_u8 bytes;                                                      \
                                                                             \
        bytes =                                                              \
            1 << (insn->code - UACPI_RESOURCE_CONVERT_OPCODE_LOAD_8_NATIVE); \
        uacpi_memcpy_zerout(                                                 \
            &accumulator, native_buf, sizeof(accumulator), bytes             \
        );                                                                   \
        break;                                                               \
    }                                                                        \
                                                                             \
    case UACPI_RESOURCE_CONVERT_OPCODE_UNREACHABLE:                          \
    default:                                                                 \
        if (insn->code != UACPI_RESOURCE_CONVERT_OPCODE_UNREACHABLE) {       \
            uacpi_error("unhandled resource conversion opcode %d\n",         \
                       insn->code);                                          \
        } else {                                                             \
            uacpi_error("tried to execute unreachable conversion opcode\n"); \
        }                                                                    \
        ctx->st = UACPI_STATUS_INTERNAL_ERROR;                               \
        return UACPI_ITERATION_DECISION_BREAK;

#define PTR_AT(ptr, offset) (void*)((uacpi_u8*)(ptr) + (offset))

#define NATIVE_OFFSET(res, offset) \
    PTR_AT(res, (offset) + (sizeof(uacpi_u32) * 2))

#define NATIVE_FIELD(res, name, field) \
    NATIVE_OFFSET(res, NATIVE_O(name, field))

#define CHECK_AML_OOB(offset, prefix, what)                                  \
    if (uacpi_unlikely(offset > ((uacpi_u32)aml_size + header_size))) {      \
        uacpi_error(prefix what " is OOB: %zu > %u\n",                       \
                    (uacpi_size)offset, (uacpi_u32)aml_size + header_size);  \
        ctx->st = UACPI_STATUS_AML_INVALID_RESOURCE;                         \
        return UACPI_ITERATION_DECISION_BREAK;                               \
    }

#define CHECK_AML_OFFSET_BASE(offset, what)                             \
    if (uacpi_unlikely(offset < base_aml_size_with_header)) {           \
        uacpi_error(                                                    \
            "invalid " what " offset: %zu, expected at least %u\n",     \
            (uacpi_size)offset, base_aml_size_with_header);             \
        ctx->st = UACPI_STATUS_AML_INVALID_RESOURCE;                    \
        return UACPI_ITERATION_DECISION_BREAK;                          \
    }

#define CHECK_AML_OFFSET(offset, what)     \
    CHECK_AML_OOB(offset, "end of ", what) \
    CHECK_AML_OFFSET_BASE(offset, what)

static uacpi_resource_type aml_serial_to_native_type(
        uacpi_u8 type
)
{
    return (type - ACPI_SERIAL_TYPE_I2C) +
           UACPI_RESOURCE_TYPE_SERIAL_I2C_CONNECTION;
}

static uacpi_iteration_decision do_aml_resource_to_native(
    void *opaque, uacpi_u8 *data, uacpi_u16 aml_size,
    const struct uacpi_resource_spec *spec
)
{
    struct resource_conversion_ctx *ctx = opaque;
    uacpi_resource *resource = ctx->buf;
    const struct uacpi_resource_convert_instruction *insns, *insn;
    uacpi_u8 header_size, pc = 0;
    uacpi_u8 *src, *dst;
    void *resource_end;
    uacpi_u16 base_aml_size;
    uacpi_u32 base_aml_size_with_header, accumulator = 0;

    insns = spec->to_native;

    header_size = aml_resource_kind_to_header_size[spec->resource_kind];
    resource->type = spec->native_type;
    resource->length = native_size_for_aml_resource(data, aml_size, spec);
    resource_end = ctx->byte_buf + spec->native_size;
    ctx->byte_buf += resource->length;

    base_aml_size = base_aml_size_with_header = spec->aml_size;
    base_aml_size_with_header += header_size;

    if (insns == UACPI_NULL)
        return conditional_continue(ctx);

    for (;;) {
        insn = &insns[pc++];

        src = data + insn->aml_offset;
        dst = NATIVE_OFFSET(resource, insn->native_offset);

        switch (insn->code) {
        case UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_8:
        case UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_16: {
            uacpi_size i, j, max_bit;
            uacpi_u16 value;

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_16) {
                max_bit = 16;
                uacpi_memcpy(&value, src, sizeof(uacpi_u16));
            } else {
                max_bit = 8;
                uacpi_memcpy_zerout(
                    &value, src, sizeof(value), sizeof(uacpi_u8)
                );
            }

            for (i = 0, j = 0; i < max_bit; ++i) {
                if (!(value & (1 << i)))
                    continue;

                dst[j++] = i;
            }

            uacpi_memcpy(NATIVE_OFFSET(resource, insn->arg2), &j, 1);
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_1:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_2:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_3:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_6:{
            uacpi_u8 mask, value;

            mask = (insn->code - UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_1) + 1;
            mask = (1 << mask) - 1;

            value = (*src >> insn->imm) & mask;
            uacpi_memcpy(dst, &value, sizeof(value));
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_AML_SIZE_32:
            accumulator = aml_size;
            uacpi_memcpy(dst, &accumulator, 4);
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE:
        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE_NO_INDEX:
        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL: {
            uacpi_size offset = 0, max_offset, length = 0;
            uacpi_char *src_string, *dst_string;
            union {
                void *ptr;
                uacpi_resource_source *source;
                uacpi_resource_label *label;
            } dst_name = { .ptr = dst };

            /*
             * Check if the string is bounded by anything at the top. If not, we
             * just assume it ends at the end of the resource.
             */
            if (insn->arg2) {
                uacpi_memcpy_zerout(&max_offset, data + insn->arg2,
                                    sizeof(max_offset), sizeof(uacpi_u16));
                CHECK_AML_OFFSET(max_offset, "resource source");
            } else {
                max_offset = aml_size + header_size;
            }

            offset += base_aml_size_with_header;
            offset += accumulator;

            if (insn->code != UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL)
                dst_name.source->index_present = UACPI_TRUE;

            if (offset >= max_offset) {
                if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE)
                    dst_name.source->index_present = UACPI_FALSE;
                break;
            }

            src_string = PTR_AT(data, offset);

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE) {
                uacpi_memcpy(&dst_name.source->index, src_string++, 1);
                offset++;
            }

            if (offset == max_offset)
                break;

            while (offset++ < max_offset) {
                if (src_string[length++] == '\0')
                    break;
            }

            if (src_string[length - 1] != '\0') {
                uacpi_error("non-null-terminated resource source string\n");
                ctx->st = UACPI_STATUS_AML_INVALID_RESOURCE;
                return UACPI_ITERATION_DECISION_BREAK;
            }

            dst_string = PTR_AT(resource_end, accumulator);
            uacpi_memcpy(dst_string, src_string, length);

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL) {
                dst_name.label->length = length;
                dst_name.label->string = dst_string;
            } else {
                dst_name.source->length = length;
                dst_name.source->string = dst_string;
            }

            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_PIN_TABLE_LENGTH:
            uacpi_memcpy_zerout(&accumulator, src,
                                sizeof(accumulator), sizeof(uacpi_u16));
            CHECK_AML_OFFSET(accumulator, "pin table");

            accumulator -= base_aml_size_with_header;
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_PIN_TABLE: {
            uacpi_u16 entry_count = accumulator / 2;

            /*
             * Pin table is stored right at the end of the resource buffer,
             * copy the data there.
             */
            uacpi_memcpy(
                resource_end,
                data + base_aml_size_with_header,
                accumulator
            );

            // Set pin_table_length
            uacpi_memcpy(dst, &entry_count, sizeof(entry_count));

            // Set pin_table pointer
            uacpi_memcpy(NATIVE_OFFSET(resource, insn->arg2),
                         &resource_end, sizeof(void*));
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_VENDOR_DATA: {
            uacpi_size length;
            uacpi_u16 data_offset, offset_from_end;
            void *native_dst, *vendor_data;

            uacpi_memcpy(&data_offset, src, sizeof(data_offset));
            CHECK_AML_OFFSET(data_offset, "vendor data");

            vendor_data = data + data_offset;

            /*
             * Rebase the offset to cut off the header as it's not included
             * in the size fields.
             */
            data_offset -= header_size;

            length = aml_size - data_offset;
            if (length == 0)
                break;

            uacpi_memcpy(dst, &length, sizeof(uacpi_u16));

            offset_from_end = data_offset - base_aml_size;
            native_dst = PTR_AT(resource_end, offset_from_end);

            uacpi_memcpy(native_dst, vendor_data, length);
            uacpi_memcpy(NATIVE_OFFSET(resource, insn->arg2),
                         &native_dst, sizeof(void*));
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_SERIAL_TYPE_SPECIFIC: {
            uacpi_resource_serial_bus_common *serial_bus_common;
            uacpi_u8 serial_type, extra_size, type_length;

            serial_bus_common = &resource->serial_bus_common;
            serial_type = *src;
            serial_bus_common->type = serial_type;
            resource->type = aml_serial_to_native_type(serial_type);

            /*
             * Now that we know the serial type rebase the end pointers and
             * sizes.
             */
            resource_end = PTR_AT(
                resource_end,
                aml_serial_resource_to_extra_native_size[serial_type]
            );
            extra_size = aml_serial_resource_to_extra_aml_size[serial_type];
            base_aml_size += extra_size;
            base_aml_size_with_header += extra_size;

            type_length = serial_bus_common->type_data_length;
            if (uacpi_unlikely(type_length < extra_size)) {
                uacpi_error(
                    "invalid type-specific data length: %d, "
                    "expected at least %d\n", type_length, extra_size
                );
                ctx->st = UACPI_STATUS_AML_INVALID_RESOURCE;
                return UACPI_ITERATION_DECISION_BREAK;
            }

            /*
             * Calculate the length of the vendor data. All the extra data
             * beyond the end of type-specific size is considered vendor data.
             */
            accumulator = type_length - extra_size;
            if (accumulator == 0)
                break;

            serial_bus_common->vendor_data_length = accumulator;
            serial_bus_common->vendor_data = resource_end;
            uacpi_memcpy(
                resource_end,
                data + base_aml_size_with_header,
                accumulator
            );
            break;
        }

        CONVERSION_OPCODES_COMMON(dst)
        }
    }
}

static uacpi_iteration_decision accumulate_native_buffer_size(
    void *opaque, uacpi_u8 *data, uacpi_u16 resource_size,
    const struct uacpi_resource_spec *spec
)
{
    struct resource_conversion_ctx *ctx = opaque;
    uacpi_size size_for_this;

    size_for_this = native_size_for_aml_resource(data, resource_size, spec);
    if (size_for_this == 0 || (ctx->size + size_for_this) < ctx->size) {
        uacpi_error("invalid native size for aml resource: %zu\n",
                    size_for_this);
        ctx->st = UACPI_STATUS_AML_INVALID_RESOURCE;
        return UACPI_ITERATION_DECISION_BREAK;
    }

    ctx->size += size_for_this;
    return conditional_continue(ctx);
}

static uacpi_status eval_resource_helper(
    uacpi_namespace_node *node, const uacpi_char *method,
    uacpi_object **out_obj
)
{
    uacpi_status ret;
    uacpi_bool is_device;

    ret = uacpi_namespace_node_is(node, UACPI_OBJECT_DEVICE, &is_device);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely(!is_device))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return uacpi_eval_simple_buffer(
        node, method, out_obj
    );
}

uacpi_status uacpi_native_resources_from_aml(
    uacpi_data_view aml_buffer, uacpi_resources **out_resources
)
{
    uacpi_status ret;
    struct resource_conversion_ctx ctx = { 0 };
    uacpi_resources *resources;

    ret = uacpi_for_each_aml_resource(
        aml_buffer, accumulate_native_buffer_size, &ctx
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    if (uacpi_unlikely_error(ctx.st))
        return ctx.st;

    // Realistically any resource buffer bigger than this is probably a bug
    if (uacpi_unlikely(ctx.size > (5 * 1024u * 1024u))) {
        uacpi_error("bug: bogus native resource buffer size %zu\n", ctx.size);
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    resources = uacpi_kernel_alloc_zeroed(ctx.size + sizeof(uacpi_resources));
    if (uacpi_unlikely(resources == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;
    resources->length = ctx.size;
    resources->entries = UACPI_PTR_ADD(resources, sizeof(uacpi_resources));

    ctx = (struct resource_conversion_ctx) {
        .buf = resources->entries,
    };

    ret = uacpi_for_each_aml_resource(aml_buffer, do_aml_resource_to_native, &ctx);
    if (uacpi_unlikely_error(ret)) {
        uacpi_free_resources(resources);
        return ret;
    }

    *out_resources = resources;
    return ret;
}

uacpi_status uacpi_get_resource_from_buffer(
    uacpi_data_view aml_buffer, uacpi_resource **out_resource
)
{
    uacpi_status ret;
    struct resource_conversion_ctx ctx = {
        .just_one = UACPI_TRUE,
    };
    uacpi_resource *resource;

    ret = uacpi_for_each_aml_resource(
        aml_buffer, accumulate_native_buffer_size, &ctx
    );
    if (uacpi_unlikely_error(ret))
        return ret;

    resource = uacpi_kernel_alloc_zeroed(ctx.size);
    if (uacpi_unlikely(resource == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    ctx = (struct resource_conversion_ctx) {
        .buf = resource,
        .just_one = UACPI_TRUE,
    };

    ret = uacpi_for_each_aml_resource(aml_buffer, do_aml_resource_to_native, &ctx);
    if (uacpi_unlikely_error(ret)) {
        uacpi_free_resource(resource);
        return ret;
    }

    *out_resource = resource;
    return ret;
}

void uacpi_free_resources(uacpi_resources *resources)
{
    if (resources == UACPI_NULL)
        return;

    uacpi_free(resources, sizeof(uacpi_resources) + resources->length);
}

void uacpi_free_resource(uacpi_resource *resource)
{
    if (resource == UACPI_NULL)
        return;

    uacpi_free(resource, resource->length);
}

static uacpi_status extract_native_resources_from_method(
    uacpi_namespace_node *device, const uacpi_char *method,
    uacpi_resources **out_resources
)
{
    uacpi_status ret;
    uacpi_object *obj;
    uacpi_data_view buffer;

    ret = eval_resource_helper(device, method, &obj);
    if (uacpi_unlikely_error(ret))
        return ret;

    uacpi_buffer_to_view(obj->buffer, &buffer);

    ret = uacpi_native_resources_from_aml(buffer, out_resources);
    uacpi_object_unref(obj);

    return ret;
}

uacpi_status uacpi_get_current_resources(
    uacpi_namespace_node *device, uacpi_resources **out_resources
)
{
    return extract_native_resources_from_method(device, "_CRS", out_resources);
}

uacpi_status uacpi_get_possible_resources(
    uacpi_namespace_node *device, uacpi_resources **out_resources
)
{
    return extract_native_resources_from_method(device, "_PRS", out_resources);
}

uacpi_status uacpi_get_device_resources(
    uacpi_namespace_node *device, const uacpi_char *method,
    uacpi_resources **out_resources
)
{
    return extract_native_resources_from_method(device, method, out_resources);
}

uacpi_status uacpi_for_each_resource(
    uacpi_resources *resources, uacpi_resource_iteration_callback cb, void *user
)
{
    uacpi_size bytes_left = resources->length;
    uacpi_resource *current = resources->entries;
    uacpi_iteration_decision decision;

    while (bytes_left) {
        // At least the head bytes
        if (uacpi_unlikely(bytes_left < 4)) {
            uacpi_error("corrupted resource buffer %p length %zu\n",
                        resources, resources->length);
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        if (uacpi_unlikely(current->type > UACPI_RESOURCE_TYPE_MAX)) {
            uacpi_error("invalid resource type %d\n", current->type);
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        if (uacpi_unlikely(current->length > bytes_left)) {
            uacpi_error("corrupted resource@%p length %u (%zu bytes left)\n",
                        current, current->length, bytes_left);
            return UACPI_STATUS_INVALID_ARGUMENT;
        }

        decision = cb(user, current);

        if (decision == UACPI_ITERATION_DECISION_BREAK ||
            current->type == UACPI_RESOURCE_TYPE_END_TAG)
            return UACPI_STATUS_OK;

        bytes_left -= current->length;
        current = (uacpi_resource*)((uacpi_u8*)current + current->length);
    }

    return UACPI_STATUS_NO_RESOURCE_END_TAG;
}

uacpi_status uacpi_for_each_device_resource(
    uacpi_namespace_node *device, const uacpi_char *method,
    uacpi_resource_iteration_callback cb, void *user
)
{
    uacpi_status ret;
    uacpi_resources *resources;

    ret = extract_native_resources_from_method(device, method, &resources);
    if (uacpi_unlikely_error(ret))
        return ret;

    ret = uacpi_for_each_resource(resources, cb, user);
    uacpi_free_resources(resources);

    return ret;
}

static const struct uacpi_resource_spec *resource_spec_from_native(
        uacpi_resource *resource
)
{
    return &aml_resources[native_resource_to_type[resource->type]];
}

static uacpi_size aml_size_for_native_resource(
        uacpi_resource *resource, const struct uacpi_resource_spec *spec
)
{
    return spec->size_for_aml ?
           spec->size_for_aml(spec, resource) :
           aml_size_with_header(spec);
}

static uacpi_iteration_decision do_native_resource_to_aml(
    void *opaque, uacpi_resource *resource
)
{
    struct resource_conversion_ctx *ctx = opaque;
    const struct uacpi_resource_spec *spec;
    const struct uacpi_resource_convert_instruction *insns, *insn;
    uacpi_u8 pc = 0;
    uacpi_u8 *dst_base, *src, *dst;
    uacpi_u32 aml_size, base_aml_size_with_header, accumulator = 0;
    void *resource_end;

    spec = resource_spec_from_native(resource);
    aml_size = aml_size_for_native_resource(resource, spec);
    insns = spec->to_aml;

    dst_base = ctx->byte_buf;
    ctx->byte_buf += aml_size;
    aml_size -= aml_resource_kind_to_header_size[spec->resource_kind];

    base_aml_size_with_header = spec->aml_size;
    base_aml_size_with_header += aml_resource_kind_to_header_size[
        spec->resource_kind
    ];
    resource_end = PTR_AT(resource, spec->native_size);

    if (spec->resource_kind == UACPI_AML_RESOURCE_KIND_LARGE) {
        *dst_base = ACPI_LARGE_ITEM | type_to_aml_resource[spec->type];
        uacpi_memcpy(dst_base + 1, &aml_size, sizeof(uacpi_u16));
    } else {
        *dst_base = type_to_aml_resource[spec->type] << ACPI_SMALL_ITEM_NAME_IDX;
        *dst_base |= aml_size;
    }

    if (insns == UACPI_NULL)
        return UACPI_ITERATION_DECISION_CONTINUE;

    for (;;) {
        insn = &insns[pc++];

        src = NATIVE_OFFSET(resource, insn->native_offset);
        dst = dst_base + insn->aml_offset;

        switch (insn->code) {
        case UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_8:
        case UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_16: {
            uacpi_u8 i, *array_size, bytes = 1;
            uacpi_u16 mask = 0;

            array_size = NATIVE_OFFSET(resource, insn->arg2);
            for (i = 0; i < *array_size; ++i)
                mask |= 1 << src[i];

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_PACKED_ARRAY_16)
                bytes = 2;

            uacpi_memcpy(dst, &mask, bytes);
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_1:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_2:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_3:
        case UACPI_RESOURCE_CONVERT_OPCODE_BIT_FIELD_6:
            *dst |= *src << insn->imm;
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_AML_SIZE_32:
            accumulator = aml_size;
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE:
        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE_NO_INDEX:
        case UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL: {
            uacpi_size source_offset, length;
            uacpi_u8 *dst_string;
            const uacpi_char *src_string;
            union {
                void *ptr;
                uacpi_resource_source *source;
                uacpi_resource_label *label;
            } src_name = { .ptr = src };

            source_offset = base_aml_size_with_header + accumulator;
            dst_string = dst_base + source_offset;

            if (insn->aml_offset)
                uacpi_memcpy(dst, &source_offset, sizeof(uacpi_u16));

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_SOURCE &&
                src_name.source->index_present)
                uacpi_memcpy(dst_string++, &src_name.source->index, 1);

            if (insn->code == UACPI_RESOURCE_CONVERT_OPCODE_RESOURCE_LABEL) {
                length = src_name.label->length;
                src_string = src_name.label->string;
            } else {
                length = src_name.source->length;
                src_string = src_name.source->string;
            }

            if (length == 0)
                break;

            if (uacpi_unlikely(src_string == UACPI_NULL)) {
                uacpi_error(
                    "source string length is %zu but the pointer is NULL\n",
                    length
                );
                ctx->st = UACPI_STATUS_INVALID_ARGUMENT;
                return UACPI_ITERATION_DECISION_BREAK;
            }

            uacpi_memcpy(dst_string, src_string, length);
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_LOAD_PIN_TABLE_LENGTH:
            uacpi_memcpy_zerout(&accumulator, src,
                                sizeof(accumulator), sizeof(uacpi_u16));
            accumulator *= sizeof(uacpi_u16);
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_PIN_TABLE:
            /*
             * The pin table resides right at the end of the base resource,
             * set the offset to it in the AML we're encoding.
             */
            uacpi_memcpy(dst, &base_aml_size_with_header, sizeof(uacpi_u16));

            /*
             * Copy the actual data. It also resides right at the end of the
             * native base resource.
             */
            uacpi_memcpy(
                dst_base + base_aml_size_with_header,
                resource_end,
                accumulator
            );
            break;

        case UACPI_RESOURCE_CONVERT_OPCODE_VENDOR_DATA: {
            uacpi_u16 vendor_data_length, data_offset, vendor_data_offset;
            uacpi_u8 *vendor_data;

            // Read the vendor_data pointer
            uacpi_memcpy(&vendor_data, NATIVE_OFFSET(resource, insn->arg2),
                         sizeof(void*));
            uacpi_memcpy(&vendor_data_length, src, sizeof(uacpi_u16));

            if (vendor_data == UACPI_NULL) {
                uacpi_size full_aml_size;

                if (uacpi_unlikely(vendor_data_length != 0)) {
                    uacpi_error(
                        "vendor_data_length is %d, but pointer is NULL\n",
                        vendor_data_length
                    );
                    ctx->st = UACPI_STATUS_INVALID_ARGUMENT;
                    return UACPI_ITERATION_DECISION_BREAK;
                }

                /*
                 * There's no vendor data. The specification still mandates
                 * that we fill the vendor data offset field correctly, meaning
                 * we set it to the total length of the resource.
                 */
                full_aml_size = aml_size;
                full_aml_size += aml_resource_kind_to_header_size[
                    spec->resource_kind
                ];

                uacpi_memcpy(dst, &full_aml_size, sizeof(uacpi_u16));
                break;
            }

            /*
             * Calculate the offset of vendor data from the end of the native
             * resource and use it since it matches the offset from the end of
             * the AML resource.
             *
             * Non-zero value means there's a source string in between.
             */
            data_offset = vendor_data - (uacpi_u8*)resource_end;
            vendor_data_offset = data_offset + base_aml_size_with_header;

            // Write vendor_data_offset
            uacpi_memcpy(dst, &vendor_data_offset, sizeof(uacpi_u16));

            /*
             * Write vendor_data_length, this field is right after
             * vendor_data_offset, and is completely redundant, but it exists
             * nonetheless.
             */
            uacpi_memcpy(
                dst + sizeof(uacpi_u16),
                &vendor_data_length,
                sizeof(vendor_data_length)
            );

            // Finally write the data itself
            uacpi_memcpy(
                dst_base + vendor_data_offset,
                vendor_data,
                vendor_data_length
            );
            break;
        }

        case UACPI_RESOURCE_CONVERT_OPCODE_SERIAL_TYPE_SPECIFIC: {
            uacpi_u8 serial_type = *src;
            *dst = serial_type;

            ctx->st = validate_aml_serial_type(serial_type);
            if (uacpi_unlikely_error(ctx->st))
                return UACPI_ITERATION_DECISION_BREAK;

            if (uacpi_unlikely(resource->type !=
                               aml_serial_to_native_type(serial_type))) {
                uacpi_error(
                    "native serial resource type %d doesn't match expected %d\n",
                    resource->type, aml_serial_to_native_type(serial_type)
                );
                ctx->st = UACPI_STATUS_INVALID_ARGUMENT;
                return UACPI_ITERATION_DECISION_BREAK;
            }

            // Rebase the end pointer & size now that we know the serial type
            resource_end = PTR_AT(
                resource_end,
                aml_serial_resource_to_extra_native_size[serial_type]
            );
            base_aml_size_with_header += aml_serial_resource_to_extra_aml_size[
                serial_type
            ];

            accumulator = resource->serial_bus_common.vendor_data_length;
            if (accumulator == 0)
                break;

            // Copy vendor data
            uacpi_memcpy(
                dst_base + base_aml_size_with_header,
                resource_end,
                accumulator
            );
            break;
        }

        CONVERSION_OPCODES_COMMON(src)
        }
    }
}

#define INLINE_END_TAG &(uacpi_resource) { .type = UACPI_RESOURCE_TYPE_END_TAG }

static uacpi_status native_resources_to_aml(
    uacpi_resources *native_resources, void *aml_buffer
)
{
    uacpi_status ret;
    struct resource_conversion_ctx ctx = {
        .buf = aml_buffer,
    };

    ret = uacpi_for_each_resource(
        native_resources, do_native_resource_to_aml, &ctx
    );
    if (ret == UACPI_STATUS_NO_RESOURCE_END_TAG) {
        // An end tag is always included
        do_native_resource_to_aml(&ctx, INLINE_END_TAG);
        ret = UACPI_STATUS_OK;
    }
    if (uacpi_unlikely_error(ret))
        return ret;

    return ctx.st;
}

static uacpi_iteration_decision accumulate_aml_buffer_size(
    void *opaque, uacpi_resource *resource
)
{
    struct resource_conversion_ctx *ctx = opaque;
    const struct uacpi_resource_spec *spec;
    uacpi_size size_for_this;

    // resource->type is sanitized to be valid here by the iteration function
    spec = resource_spec_from_native(resource);

    size_for_this = aml_size_for_native_resource(resource, spec);
    if (size_for_this == 0 || (ctx->size + size_for_this) < ctx->size) {
        uacpi_error("invalid aml size for native resource: %zu\n",
                    size_for_this);
        ctx->st = UACPI_STATUS_INVALID_ARGUMENT;
        return UACPI_ITERATION_DECISION_BREAK;
    }

    ctx->size += size_for_this;
    return UACPI_ITERATION_DECISION_CONTINUE;
}

uacpi_status uacpi_native_resources_to_aml(
    uacpi_resources *resources, uacpi_object **out_template
)
{
    uacpi_status ret;
    uacpi_object *obj;
    void *buffer;
    struct resource_conversion_ctx ctx = { 0 };

    ret = uacpi_for_each_resource(
        resources, accumulate_aml_buffer_size, &ctx
    );
    if (ret == UACPI_STATUS_NO_RESOURCE_END_TAG) {
        // An end tag is always included
        accumulate_aml_buffer_size(&ctx, INLINE_END_TAG);
        ret = UACPI_STATUS_OK;
    }
    if (uacpi_unlikely_error(ret))
        return ret;
    if (uacpi_unlikely_error(ctx.st))
        return ctx.st;

    // Same reasoning as native_resource_from_aml
    if (uacpi_unlikely(ctx.size > (5 * 1024u * 1024u))) {
        uacpi_error("bug: bogus target aml resource buffer size %zu\n",
                    ctx.size);
        return UACPI_STATUS_INTERNAL_ERROR;
    }

    buffer = uacpi_kernel_alloc_zeroed(ctx.size);
    if (uacpi_unlikely(buffer == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    obj = uacpi_create_object(UACPI_OBJECT_BUFFER);
    if (uacpi_unlikely(obj == UACPI_NULL)) {
        uacpi_free(buffer, ctx.size);
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    obj->buffer->data = buffer;
    obj->buffer->size = ctx.size;

    ret = native_resources_to_aml(resources, buffer);
    if (uacpi_unlikely_error(ret))
        uacpi_object_unref(obj);

    if (ret == UACPI_STATUS_OK)
        *out_template = obj;

    return ret;
}

uacpi_status uacpi_set_resources(
    uacpi_namespace_node *device, uacpi_resources *resources
)
{
    uacpi_status ret;
    uacpi_object *res_template;
    uacpi_object_array args;

    ret = uacpi_native_resources_to_aml(resources, &res_template);
    if (uacpi_unlikely_error(ret))
        return ret;

    args.objects = &res_template;
    args.count = 1;
    ret = uacpi_eval(device, "_SRS", &args, UACPI_NULL);

    uacpi_object_unref(res_template);
    return ret;
}

#endif // !UACPI_BAREBONES_MODE
