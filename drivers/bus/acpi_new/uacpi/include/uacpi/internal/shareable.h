#pragma once

#include <uacpi/types.h>

struct uacpi_shareable {
    uacpi_u32 reference_count;
};

void uacpi_shareable_init(uacpi_handle);

uacpi_bool uacpi_bugged_shareable(uacpi_handle);
void uacpi_make_shareable_bugged(uacpi_handle);

uacpi_u32 uacpi_shareable_ref(uacpi_handle);
uacpi_u32 uacpi_shareable_unref(uacpi_handle);

void uacpi_shareable_unref_and_delete_if_last(
    uacpi_handle, void (*do_free)(uacpi_handle)
);

uacpi_u32 uacpi_shareable_refcount(uacpi_handle);
