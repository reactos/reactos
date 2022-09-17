/*

Required functions

*/
#include <stdint.h>

#define KASAN_SHADOW_SCALE_SHIFT 3
#define KASAN_SHADOW_OFFSET 0x80000000

int __asan_option_detect_stack_use_after_return;
typedef unsigned long long uptr;

extern int __asan_option_detect_stack_use_after_return;

struct kasan_access_info
{
    const void *access_addr;
    const void *first_bad_addr;
    size_t access_size;
    bool is_write;
    unsigned long ip;
};

static inline const void *
kasan_shadow_to_mem(const void *shadow_addr)
{
    return (void *)(((unsigned long)shadow_addr - KASAN_SHADOW_OFFSET) << KASAN_SHADOW_SCALE_SHIFT);
}

void
__asan_memcpy()
{
}

void
__asan_memmove()
{
}

// static DEFINE_SPINLOCK(report_lock);

void
kasan_report_error(struct kasan_access_info *info)
{
    // TODO: debug output

    /*
    unsigned long flags;
    spin_lock_irqsave(&report_lock, flags);
    pr_err("================================="
           "=================================\n");
    print_error_description(info);
    print_address_description(info);
    print_shadow_for_address(info->first_bad_addr);
    pr_err("================================="
           "=================================\n");
    spin_unlock_irqrestore(&report_lock, flags);
    */
}

void
kasan_report_user_access(struct kasan_access_info *info)
{
    // TODO: debug output

    /*
    unsigned long flags;
    spin_lock_irqsave(&report_lock, flags);
    pr_err("================================="
           "=================================\n");
    pr_err("BUG: KASan: user-memory-access on address %p\n", info->access_addr);
    pr_err(
        "%s of size %zu by task %s/%d\n", info->is_write ? "Write" : "Read", info->access_size, current->comm,
        task_pid_nr(current));
    dump_stack();
    pr_err("================================="
           "=================================\n");
    spin_unlock_irqrestore(&report_lock, flags);
    */
}

void
kasan_report(uintptr_t p, size_t size, bool is_write, unsigned long ip)
{
    struct kasan_access_info info;
    info.access_addr = (void *)p;
    info.access_size = size;
    info.is_write = is_write;
    info.ip = ip;
    // kasan_report_error(&info);
}

void
__asan_report_load1(uintptr_t p) {
    kasan_report(p, 1, false, _RET_IP_);
}

void
__asan_report_load2(uintptr_t p) {
    kasan_report(p, 2, false, _RET_IP_);
}

void
__asan_report_load4(uintptr_t p) {
    kasan_report(p, 4, false, _RET_IP_);
}

void
__asan_report_load8(uintptr_t p) {
    kasan_report(p, 8, false, _RET_IP_);
}

void
__asan_report_load16(uintptr_t p) {
    kasan_report(p, 16, false, _RET_IP_);
}

void
__asan_report_store1(uintptr_t p) {
    kasan_report(p, 1, true, _RET_IP_);
}

void
__asan_report_store2(uintptr_t p) {
    kasan_report(p, 2, true, _RET_IP_);
}

void
__asan_report_store4(uintptr_t p) {
    kasan_report(p, 4, true, _RET_IP_);
}

void
__asan_report_store8(uintptr_t p) {
    kasan_report(p, 8, true, _RET_IP_);
}

void
__asan_report_store16(uintptr_t p) {
    kasan_report(p, 16, true, _RET_IP_);
}

void
__asan_report_load_n(uintptr_t p, size_t size) {
    kasan_report(p, size, false, _RET_IP_);
}

void
__asan_report_store_n(uintptr_t p, size_t size) {
    kasan_report(p, size, true, _RET_IP_);
}

void
__asan_handle_no_return() {
}

void
__asan_stack_malloc_0() {
}

void
__asan_stack_malloc_1() {
}

void
__asan_stack_malloc_2() {
}

void
__asan_stack_malloc_3() {
}

void
__asan_stack_malloc_4() {
}

void
__asan_stack_malloc_5() {
}

void
__asan_stack_malloc_6() {
}

void
__asan_stack_malloc_7() {
}

void
__asan_stack_malloc_8() {
}

void
__asan_stack_malloc_9() {
}

void
__asan_stack_malloc_10() {
}

void
__asan_stack_free_0() {
}

void
__asan_stack_free_1() {
}

void
__asan_stack_free_2() {
}

void
__asan_stack_free_3() {
}

void
__asan_stack_free_4() {
}

void
__asan_stack_free_5() {
}

void
__asan_stack_free_6() {
}

void
__asan_stack_free_7() {
}

void
__asan_stack_free_8() {
}

void
__asan_stack_free_9() {
}

void
__asan_stack_free_10() {
}

void
__asan_poison_cxx_array_cookie() {
}

void
__asan_load_cxx_array_cookie() {
}

void
__asan_poison_stack_memory() {
}

void
__asan_unpoison_stack_memory() {
}

void
__asan_alloca_poison() {
}

void
__asan_allocas_unpoison() {
}

void
check_memory_region(uintptr_t p, size_t size, bool write)
{
    struct kasan_access_info info;
    if (size == 0)
        return;
    if ((void *)p < kasan_shadow_to_mem((void *)KASAN_SHADOW_START))
    {
        info.access_addr = (void *)p;
        info.access_size = size;
        info.is_write = write;
        info.ip = _RET_IP_;
        kasan_report_user_access(&info);
        return;
    }
    if (!memory_is_poisoned(p, size)))
        return;
    kasan_report(p, size, write, _RET_IP_);
}

void
__asan_load1(uintptr_t p) {
    check_memory_region(p, 1, false);
}

void
__asan_load2(uintptr_t p) {
    check_memory_region(p, 2, false);
}

void
__asan_load4(uintptr_t p) {
    check_memory_region(p, 4, false);
}

void
__asan_load8(uintptr_t p) {
    check_memory_region(p, 8, false);
}

void
__asan_load16(uintptr_t p) {
    check_memory_region(p, 16, false);
}

void
__asan_loadN(uintptr_t p, size_t size) {
    check_memory_region(p, size, false);
}

void
__asan_store1(uintptr_t p) {
    check_memory_region(p, 1, true);
}

void
__asan_store2(uintptr_t p) {
    check_memory_region(p, 2, true);
}

void
__asan_store4(uintptr_t p) {
    check_memory_region(p, 4, true);
}

void
__asan_store8(uintptr_t p) {
    check_memory_region(p, 8, true);
}

void
__asan_store16(uintptr_t p) {
    check_memory_region(p, 16, true);
}

void
__asan_storeN(uintptr_t p, size_t size) {
    check_memory_region(p, size, true);
}

void
__sanitizer_ptr_sub() {
}

void
__sanitizer_ptr_cmp() {
}

void
__sanitizer_annotate_contiguous_container() {
}


void
__asan_set_shadow_00() {
}

void
__asan_set_shadow_f1() {

}


void
__asan_set_shadow_f2() {
}

void
__asan_set_shadow_f3() {
}

void
__asan_set_shadow_f5() {
}

void
__asan_set_shadow_f8() {
}


void
__asan_init_v5() {
}

void
__asan_before_dynamic_init() {
}

void
__asan_after_dynamic_init() {
}

void
__asan_unregister_globals() {
}

void
__asan_register_globals() {
}

void
__asan_init() {
}

void
__asan_unregister_image_globals() {
}

void
__asan_register_image_globals() {
}

void
__asan_version_mismatch_check_v8() {
}


void *
__asan_memset()
{
}
