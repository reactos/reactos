#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/kernel_api.h>

#ifndef UACPI_BAREBONES_MODE

uacpi_bool uacpi_this_thread_owns_aml_mutex(uacpi_mutex*);

uacpi_status uacpi_acquire_aml_mutex(uacpi_mutex*, uacpi_u16 timeout);
uacpi_status uacpi_release_aml_mutex(uacpi_mutex*);

static inline uacpi_status uacpi_acquire_native_mutex(uacpi_handle mtx)
{
    if (uacpi_unlikely(mtx == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return uacpi_kernel_acquire_mutex(mtx, 0xFFFF);
}

uacpi_status uacpi_acquire_native_mutex_with_timeout(
    uacpi_handle mtx, uacpi_u16 timeout
);

static inline uacpi_status uacpi_release_native_mutex(uacpi_handle mtx)
{
    if (uacpi_unlikely(mtx == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    uacpi_kernel_release_mutex(mtx);
    return UACPI_STATUS_OK;
}

static inline uacpi_status uacpi_acquire_native_mutex_may_be_null(
    uacpi_handle mtx
)
{
    if (mtx == UACPI_NULL)
        return UACPI_STATUS_OK;

    return uacpi_kernel_acquire_mutex(mtx, 0xFFFF);
}

static inline uacpi_status uacpi_release_native_mutex_may_be_null(
    uacpi_handle mtx
)
{
    if (mtx == UACPI_NULL)
        return UACPI_STATUS_OK;

    uacpi_kernel_release_mutex(mtx);
    return UACPI_STATUS_OK;
}

struct uacpi_recursive_lock {
    uacpi_handle mutex;
    uacpi_size depth;
    uacpi_thread_id owner;
};

uacpi_status uacpi_recursive_lock_init(struct uacpi_recursive_lock *lock);
uacpi_status uacpi_recursive_lock_deinit(struct uacpi_recursive_lock *lock);

uacpi_status uacpi_recursive_lock_acquire(struct uacpi_recursive_lock *lock);
uacpi_status uacpi_recursive_lock_release(struct uacpi_recursive_lock *lock);

struct uacpi_rw_lock {
    uacpi_handle read_mutex;
    uacpi_handle write_mutex;
    uacpi_size num_readers;
};

uacpi_status uacpi_rw_lock_init(struct uacpi_rw_lock *lock);
uacpi_status uacpi_rw_lock_deinit(struct uacpi_rw_lock *lock);

uacpi_status uacpi_rw_lock_read(struct uacpi_rw_lock *lock);
uacpi_status uacpi_rw_unlock_read(struct uacpi_rw_lock *lock);

uacpi_status uacpi_rw_lock_write(struct uacpi_rw_lock *lock);
uacpi_status uacpi_rw_unlock_write(struct uacpi_rw_lock *lock);

#endif // !UACPI_BAREBONES_MODE
