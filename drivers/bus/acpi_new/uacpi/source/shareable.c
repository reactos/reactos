#include <uacpi/internal/shareable.h>
#include <uacpi/internal/stdlib.h>
#include <uacpi/platform/atomic.h>

#ifndef UACPI_BAREBONES_MODE

#define BUGGED_REFCOUNT 0xFFFFFFFF

void uacpi_shareable_init(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;
    shareable->reference_count = 1;
}

uacpi_bool uacpi_bugged_shareable(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;

    if (uacpi_unlikely(shareable->reference_count == 0))
        uacpi_make_shareable_bugged(shareable);

    return uacpi_atomic_load32(&shareable->reference_count) == BUGGED_REFCOUNT;
}

void uacpi_make_shareable_bugged(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;
    uacpi_atomic_store32(&shareable->reference_count, BUGGED_REFCOUNT);
}

uacpi_u32 uacpi_shareable_ref(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;

    if (uacpi_unlikely(uacpi_bugged_shareable(shareable)))
        return BUGGED_REFCOUNT;

    return uacpi_atomic_inc32(&shareable->reference_count) - 1;
}

uacpi_u32 uacpi_shareable_unref(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;

    if (uacpi_unlikely(uacpi_bugged_shareable(shareable)))
        return BUGGED_REFCOUNT;

    return uacpi_atomic_dec32(&shareable->reference_count) + 1;
}

void uacpi_shareable_unref_and_delete_if_last(
    uacpi_handle handle, void (*do_free)(uacpi_handle)
)
{
    if (handle == UACPI_NULL)
        return;

    if (uacpi_unlikely(uacpi_bugged_shareable(handle)))
        return;

    if (uacpi_shareable_unref(handle) == 1)
        do_free(handle);
}

uacpi_u32 uacpi_shareable_refcount(uacpi_handle handle)
{
    struct uacpi_shareable *shareable = handle;
    return uacpi_atomic_load32(&shareable->reference_count);
}

#endif // !UACPI_BAREBONES_MODE
