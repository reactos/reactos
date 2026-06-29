#include <uacpi/platform/atomic.h>
#include <uacpi/internal/mutex.h>
#include <uacpi/internal/log.h>
#include <uacpi/internal/registers.h>
#include <uacpi/internal/context.h>
#include <uacpi/kernel_api.h>
#include <uacpi/internal/namespace.h>

#ifndef UACPI_BAREBONES_MODE

#ifndef UACPI_REDUCED_HARDWARE

#define GLOBAL_LOCK_PENDING (1 << 0)

#define GLOBAL_LOCK_OWNED_BIT 1
#define GLOBAL_LOCK_OWNED (1 << GLOBAL_LOCK_OWNED_BIT)

#define GLOBAL_LOCK_MASK 0b11u

static uacpi_bool try_acquire_global_lock_from_firmware(uacpi_u32 *lock)
{
    uacpi_u32 value, new_value;
    uacpi_bool was_owned;

    value = *(volatile uacpi_u32*)lock;
    do {
        was_owned = (value & GLOBAL_LOCK_OWNED) >> GLOBAL_LOCK_OWNED_BIT;

        // Clear both owned & pending bits.
        new_value = value & ~GLOBAL_LOCK_MASK;

        // Set owned unconditionally
        new_value |= GLOBAL_LOCK_OWNED;

        // Set pending iff the lock was owned at the time of reading
        if (was_owned)
            new_value |= GLOBAL_LOCK_PENDING;
    } while (!uacpi_atomic_cmpxchg32(lock, &value, new_value));

    return !was_owned;
}

static uacpi_bool do_release_global_lock_to_firmware(uacpi_u32 *lock)
{
    uacpi_u32 value, new_value;

    value = *(volatile uacpi_u32*)lock;
    do {
        new_value = value & ~GLOBAL_LOCK_MASK;
    } while (!uacpi_atomic_cmpxchg32(lock, &value, new_value));

    return value & GLOBAL_LOCK_PENDING;
}

static uacpi_status uacpi_acquire_global_lock_from_firmware(void)
{
    uacpi_cpu_flags flags;
    uacpi_u16 spins = 0;
    uacpi_bool success;

    if (!g_uacpi_rt_ctx.has_global_lock)
        return UACPI_STATUS_OK;

    flags = uacpi_kernel_lock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock);
    for (;;) {
        spins++;
        uacpi_trace(
            "trying to acquire the global lock from firmware... (attempt %u)\n",
            spins
        );

        success = try_acquire_global_lock_from_firmware(
            &g_uacpi_rt_ctx.facs->global_lock
        );
        if (success)
            break;

        if (uacpi_unlikely(spins == 0xFFFF))
            break;

        g_uacpi_rt_ctx.global_lock_pending = UACPI_TRUE;
        uacpi_trace(
            "global lock is owned by firmware, waiting for a release "
            "notification...\n"
        );
        uacpi_kernel_unlock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock, flags);

        uacpi_kernel_wait_for_event(g_uacpi_rt_ctx.global_lock_event, 0xFFFF);
        flags = uacpi_kernel_lock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock);
    }

    g_uacpi_rt_ctx.global_lock_pending = UACPI_FALSE;
    uacpi_kernel_unlock_spinlock(g_uacpi_rt_ctx.global_lock_spinlock, flags);

    if (uacpi_unlikely(!success)) {
        uacpi_error("unable to acquire global lock after %u attempts\n", spins);
        return UACPI_STATUS_HARDWARE_TIMEOUT;
    }

    uacpi_trace("global lock successfully acquired after %u attempt%s\n",
                spins, spins > 1 ? "s" : "");
    return UACPI_STATUS_OK;
}

static void uacpi_release_global_lock_to_firmware(void)
{
    if (!g_uacpi_rt_ctx.has_global_lock)
        return;

    uacpi_trace("releasing the global lock to firmware...\n");
    if (do_release_global_lock_to_firmware(&g_uacpi_rt_ctx.facs->global_lock)) {
        uacpi_trace("notifying firmware of the global lock release since the "
                    "pending bit was set\n");
        uacpi_write_register_field(UACPI_REGISTER_FIELD_GBL_RLS, 1);
    }
}
#endif

UACPI_ALWAYS_OK_FOR_REDUCED_HARDWARE(
    uacpi_status uacpi_acquire_global_lock_from_firmware(void)
)
UACPI_STUB_IF_REDUCED_HARDWARE(
    void uacpi_release_global_lock_to_firmware(void)
)

uacpi_status uacpi_acquire_native_mutex_with_timeout(
    uacpi_handle mtx, uacpi_u16 timeout
)
{
    uacpi_status ret;

    if (uacpi_unlikely(mtx == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_kernel_acquire_mutex(mtx, timeout);
    if (uacpi_likely_success(ret))
        return ret;

    if (uacpi_unlikely(ret != UACPI_STATUS_TIMEOUT || timeout == 0xFFFF)) {
        uacpi_error(
            "unexpected status %08X (%s) while acquiring %p (timeout=%04X)\n",
            ret, uacpi_status_to_string(ret), mtx, timeout
        );
    }

    return ret;
}

uacpi_status uacpi_acquire_global_lock(uacpi_u16 timeout, uacpi_u32 *out_seq)
{
    uacpi_status ret;

    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(out_seq == UACPI_NULL))
        return UACPI_STATUS_INVALID_ARGUMENT;

    ret = uacpi_acquire_native_mutex_with_timeout(
        g_uacpi_rt_ctx.global_lock_mutex->handle, timeout
    );
    if (ret != UACPI_STATUS_OK)
        return ret;

    ret = uacpi_acquire_global_lock_from_firmware();
    if (uacpi_unlikely_error(ret)) {
        uacpi_release_native_mutex(g_uacpi_rt_ctx.global_lock_mutex->handle);
        return ret;
    }

    if (uacpi_unlikely(g_uacpi_rt_ctx.global_lock_seq_num == 0xFFFFFFFF))
        g_uacpi_rt_ctx.global_lock_seq_num = 0;

    *out_seq = g_uacpi_rt_ctx.global_lock_seq_num++;
    g_uacpi_rt_ctx.global_lock_acquired = UACPI_TRUE;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_release_global_lock(uacpi_u32 seq)
{
    UACPI_ENSURE_INIT_LEVEL_AT_LEAST(UACPI_INIT_LEVEL_SUBSYSTEM_INITIALIZED);

    if (uacpi_unlikely(!g_uacpi_rt_ctx.global_lock_acquired ||
                       seq != g_uacpi_rt_ctx.global_lock_seq_num))
        return UACPI_STATUS_INVALID_ARGUMENT;

    g_uacpi_rt_ctx.global_lock_acquired = UACPI_FALSE;
    uacpi_release_global_lock_to_firmware();
    uacpi_release_native_mutex(g_uacpi_rt_ctx.global_lock_mutex->handle);

    return UACPI_STATUS_OK;
}

uacpi_bool uacpi_this_thread_owns_aml_mutex(uacpi_mutex *mutex)
{
    uacpi_thread_id id;

    id = UACPI_ATOMIC_LOAD_THREAD_ID(&mutex->owner);
    return id == uacpi_kernel_get_thread_id();
}

uacpi_status uacpi_acquire_aml_mutex(uacpi_mutex *mutex, uacpi_u16 timeout)
{
    uacpi_thread_id this_id;
    uacpi_status ret = UACPI_STATUS_OK;

    this_id = uacpi_kernel_get_thread_id();
    if (UACPI_ATOMIC_LOAD_THREAD_ID(&mutex->owner) == this_id) {
        if (uacpi_unlikely(mutex->depth == 0xFFFF)) {
            uacpi_warn(
                "failing an attempt to acquire mutex @%p, too many recursive "
                "acquires\n", mutex
            );
            return UACPI_STATUS_DENIED;
        }

        mutex->depth++;
        return ret;
    }

    uacpi_namespace_write_unlock();
    ret = uacpi_acquire_native_mutex_with_timeout(mutex->handle, timeout);
    if (ret != UACPI_STATUS_OK)
        goto out;

    if (mutex->handle == g_uacpi_rt_ctx.global_lock_mutex->handle) {
        ret = uacpi_acquire_global_lock_from_firmware();
        if (uacpi_unlikely_error(ret)) {
            uacpi_release_native_mutex(mutex->handle);
            goto out;
        }
    }

    UACPI_ATOMIC_STORE_THREAD_ID(&mutex->owner, this_id);
    mutex->depth = 1;

out:
    uacpi_namespace_write_lock();
    return ret;
}

uacpi_status uacpi_release_aml_mutex(uacpi_mutex *mutex)
{
    if (mutex->depth-- > 1)
        return UACPI_STATUS_OK;

    if (mutex->handle == g_uacpi_rt_ctx.global_lock_mutex->handle)
        uacpi_release_global_lock_to_firmware();

    UACPI_ATOMIC_STORE_THREAD_ID(&mutex->owner, UACPI_THREAD_ID_NONE);
    uacpi_release_native_mutex(mutex->handle);

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_recursive_lock_init(struct uacpi_recursive_lock *lock)
{
    lock->mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(lock->mutex == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    lock->owner = UACPI_THREAD_ID_NONE;
    lock->depth = 0;

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_recursive_lock_deinit(struct uacpi_recursive_lock *lock)
{
    if (uacpi_unlikely(lock->depth)) {
        uacpi_warn(
            "de-initializing active recursive lock %p with depth=%zu\n",
            lock, lock->depth
        );
        lock->depth = 0;
    }

    lock->owner = UACPI_THREAD_ID_NONE;

    if (lock->mutex != UACPI_NULL) {
        uacpi_kernel_free_mutex(lock->mutex);
        lock->mutex = UACPI_NULL;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_recursive_lock_acquire(struct uacpi_recursive_lock *lock)
{
    uacpi_thread_id this_id;
    uacpi_status ret = UACPI_STATUS_OK;

    this_id = uacpi_kernel_get_thread_id();
    if (UACPI_ATOMIC_LOAD_THREAD_ID(&lock->owner) == this_id) {
        lock->depth++;
        return ret;
    }

    ret = uacpi_acquire_native_mutex(lock->mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    UACPI_ATOMIC_STORE_THREAD_ID(&lock->owner, this_id);
    lock->depth = 1;
    return ret;
}

uacpi_status uacpi_recursive_lock_release(struct uacpi_recursive_lock *lock)
{
    if (lock->depth-- > 1)
        return UACPI_STATUS_OK;

    UACPI_ATOMIC_STORE_THREAD_ID(&lock->owner, UACPI_THREAD_ID_NONE);
    return uacpi_release_native_mutex(lock->mutex);
}

uacpi_status uacpi_rw_lock_init(struct uacpi_rw_lock *lock)
{
    lock->read_mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(lock->read_mutex == UACPI_NULL))
        return UACPI_STATUS_OUT_OF_MEMORY;

    lock->write_mutex = uacpi_kernel_create_mutex();
    if (uacpi_unlikely(lock->write_mutex == UACPI_NULL)) {
        uacpi_kernel_free_mutex(lock->read_mutex);
        lock->read_mutex = UACPI_NULL;
        return UACPI_STATUS_OUT_OF_MEMORY;
    }

    lock->num_readers = 0;
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_rw_lock_deinit(struct uacpi_rw_lock *lock)
{
    if (uacpi_unlikely(lock->num_readers)) {
        uacpi_warn("de-initializing rw_lock %p with %zu active readers\n",
                   lock, lock->num_readers);
        lock->num_readers = 0;
    }

    if (lock->read_mutex != UACPI_NULL) {
        uacpi_kernel_free_mutex(lock->read_mutex);
        lock->read_mutex = UACPI_NULL;
    }
    if (lock->write_mutex != UACPI_NULL) {
        uacpi_kernel_free_mutex(lock->write_mutex);
        lock->write_mutex = UACPI_NULL;
    }

    return UACPI_STATUS_OK;
}

uacpi_status uacpi_rw_lock_read(struct uacpi_rw_lock *lock)
{
    uacpi_status ret;

    ret = uacpi_acquire_native_mutex(lock->read_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (lock->num_readers++ == 0) {
        ret = uacpi_acquire_native_mutex(lock->write_mutex);
        if (uacpi_unlikely_error(ret))
            lock->num_readers = 0;
    }

    uacpi_kernel_release_mutex(lock->read_mutex);
    return ret;
}

uacpi_status uacpi_rw_unlock_read(struct uacpi_rw_lock *lock)
{
    uacpi_status ret;

    ret = uacpi_acquire_native_mutex(lock->read_mutex);
    if (uacpi_unlikely_error(ret))
        return ret;

    if (lock->num_readers-- == 1)
        uacpi_release_native_mutex(lock->write_mutex);

    uacpi_kernel_release_mutex(lock->read_mutex);
    return ret;
}

uacpi_status uacpi_rw_lock_write(struct uacpi_rw_lock *lock)
{
    return uacpi_acquire_native_mutex(lock->write_mutex);
}

uacpi_status uacpi_rw_unlock_write(struct uacpi_rw_lock *lock)
{
    return uacpi_release_native_mutex(lock->write_mutex);
}

#endif // !UACPI_BAREBONES_MODE
