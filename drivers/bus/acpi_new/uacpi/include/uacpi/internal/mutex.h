#pragma once

#include <uacpi/internal/types.h>
#include <uacpi/kernel_api.h>

#ifndef UACPI_BAREBONES_MODE

uacpi_bool uacpi_this_thread_owns_aml_mutex(uacpi_mutex*);

uacpi_status uacpi_acquire_aml_mutex(uacpi_mutex*, uacpi_u16 timeout);
uacpi_status uacpi_release_aml_mutex(uacpi_mutex*);

/*
 * A native mutex is a semaphore that has exactly one unit: whoever takes it
 * holds the mutex until they give it back.
 *
 * Note that there's no such thing as an owner as far as the kernel is
 * concerned. Whenever we need one we keep track of it ourselves, and some of
 * our locks rely on being released by a thread other than the one that has
 * acquired them, see uacpi_rw_lock.
 */
static inline uacpi_handle uacpi_create_native_mutex(void)
{
    return uacpi_kernel_create_semaphore(1);
}

static inline void uacpi_free_native_mutex(uacpi_handle mtx)
{
    uacpi_kernel_free_semaphore(mtx);
}

static inline uacpi_status uacpi_acquire_native_mutex(uacpi_handle mtx)
{
    if (uacpi_unlikely(mtx == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    return uacpi_kernel_wait_for_semaphore(mtx, 0xFFFF);
}

uacpi_status uacpi_acquire_native_mutex_with_timeout(
    uacpi_handle mtx, uacpi_u16 timeout
);

static inline uacpi_status uacpi_release_native_mutex(uacpi_handle mtx)
{
    if (uacpi_unlikely(mtx == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    uacpi_kernel_signal_semaphore(mtx);
    return UACPI_STATUS_OK;
}

static inline uacpi_status uacpi_acquire_native_mutex_may_be_null(
    uacpi_handle mtx
)
{
    if (mtx == UACPI_NULL)
        return UACPI_STATUS_OK;

    return uacpi_kernel_wait_for_semaphore(mtx, 0xFFFF);
}

static inline uacpi_status uacpi_release_native_mutex_may_be_null(
    uacpi_handle mtx
)
{
    if (mtx == UACPI_NULL)
        return UACPI_STATUS_OK;

    uacpi_kernel_signal_semaphore(mtx);
    return UACPI_STATUS_OK;
}

/*
 * A native event is a semaphore that starts out with no units, and is given
 * one every time that it's signaled.
 */
static inline uacpi_handle uacpi_create_native_event(void)
{
    return uacpi_kernel_create_semaphore(0);
}

static inline void uacpi_free_native_event(uacpi_handle evt)
{
    uacpi_kernel_free_semaphore(evt);
}

static inline uacpi_bool uacpi_wait_for_native_event(
    uacpi_handle evt, uacpi_u16 timeout
)
{
    return uacpi_kernel_wait_for_semaphore(evt, timeout) == UACPI_STATUS_OK;
}

static inline void uacpi_signal_native_event(uacpi_handle evt)
{
    uacpi_kernel_signal_semaphore(evt);
}

/*
 * There's no such thing as resetting a semaphore, so take every unit that the
 * event has instead. Nothing is able to signal it in the meantime: the only
 * events that are ever reset are the AML ones, which are only signaled by AML,
 * and that can't be executed while the caller is holding the namespace lock.
 */
static inline void uacpi_reset_native_event(uacpi_handle evt)
{
    while (uacpi_kernel_wait_for_semaphore(evt, 0) == UACPI_STATUS_OK);
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
