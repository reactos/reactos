/*

Required functions

*/
#include <stdint.h>

int __asan_option_detect_stack_use_after_return;
typedef unsigned long long uptr;

extern int __asan_option_detect_stack_use_after_return;

void
__asan_memcpy(uintptr_t p, uintptr_t p2, size_t sz)
{

}

void
__asan_report_load1(uintptr_t p) {
}

void
__asan_report_load2(uintptr_t p) {
}

void
__asan_report_load4(uintptr_t p) {
}

void
__asan_report_load8(uintptr_t p) {
}

void
__asan_report_load16(uintptr_t p) {
}

void
__asan_report_store1(uintptr_t p) {
}

void
__asan_report_store2(uintptr_t p) {
}

void
__asan_report_store4(uintptr_t p) {
}

void
__asan_report_store8(uintptr_t p) {
}

void
__asan_report_store16(uintptr_t p) {
}

void
__asan_report_load_n(uintptr_t p, unsigned long sz) {
}

void
__asan_report_store_n(uintptr_t p, unsigned long sz) {
}

void
__asan_handle_no_return(void) {
}

uintptr_t
__asan_stack_malloc_0(size_t sz) {
}

uintptr_t
__asan_stack_malloc_1(size_t sz) {
}

uintptr_t
__asan_stack_malloc_2(size_t sz) {
}

uintptr_t
__asan_stack_malloc_3(size_t sz) {
}

uintptr_t
__asan_stack_malloc_4(size_t sz) {
}

uintptr_t
__asan_stack_malloc_5(size_t sz) {
}

uintptr_t
__asan_stack_malloc_6(size_t sz) {
}

uintptr_t
__asan_stack_malloc_7(size_t sz) {
}

uintptr_t
__asan_stack_malloc_8(size_t sz) {
}

uintptr_t
__asan_stack_malloc_9(size_t sz) {
}

uintptr_t
__asan_stack_malloc_10(size_t sz) {
}

void
__asan_stack_free_0(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_1(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_2(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_3(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_4(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_5(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_6(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_7(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_8(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_9(uintptr_t dst, size_t sz) {
}

void
__asan_stack_free_10(uintptr_t dst, size_t sz) {
}

void
__asan_poison_cxx_array_cookie(uintptr_t p) {
}

uintptr_t
__asan_load_cxx_array_cookie(uintptr_t *p) {
}

void
__asan_poison_stack_memory(uintptr_t addr, size_t size) {
}

void
__asan_unpoison_stack_memory(uintptr_t addr, size_t size) {
}

void
__asan_alloca_poison(uintptr_t addr, uintptr_t size) {
}

void
__asan_allocas_unpoison(uintptr_t top, uintptr_t bottom) {
}

void
__asan_load1(uintptr_t addr) {
}

void
__asan_load2(uintptr_t addr) {
}

void
__asan_load4(uintptr_t addr) {
}

void
__asan_load8(uintptr_t addr) {
}

void
__asan_load16(uintptr_t addr) {
}

void
__asan_loadN(uintptr_t addr, size_t sz) {
}

void
__asan_store1(uintptr_t addr) {
}

void
__asan_store2(uintptr_t addr) {
}

void
__asan_store4(uintptr_t addr) {
}

void
__asan_store8(uintptr_t addr) {
}

void
__asan_store16(uintptr_t addr) {
}

void
__asan_storeN(uintptr_t addr, size_t sz) {
}

void
__sanitizer_ptr_sub(uintptr_t a, uintptr_t b) {
}

void
__sanitizer_ptr_cmp(uintptr_t a, uintptr_t b) {
}

void
__sanitizer_annotate_contiguous_container(const void *beg, const void *end, const void *old_mid, const void *new_mid) {
}


void
__asan_set_shadow_00(uintptr_t addr, size_t sz) {
}

void
__asan_set_shadow_f1(uintptr_t addr, size_t sz) {

}


void
__asan_set_shadow_f2(uintptr_t addr, size_t sz) {
}

void
__asan_set_shadow_f3(uintptr_t addr, size_t sz) {
}

void
__asan_set_shadow_f5(uintptr_t addr, size_t sz) {
}

void
__asan_set_shadow_f8(uintptr_t addr, size_t sz) {
}


void
__asan_init_v5(void) {
}

void
__asan_before_dynamic_init(uintptr_t arg) {
}

void
__asan_after_dynamic_init(void) {
}

void
__asan_unregister_globals(uintptr_t g, uintptr_t n) {
}

void
__asan_register_globals(uintptr_t g, uintptr_t n) {
}

void
__asan_init(void) {
}

void
__asan_unregister_image_globals(uintptr_t ptr) {
}

void
__asan_register_image_globals(uintptr_t ptr) {
}

void
__asan_version_mismatch_check_v8(void) {
}


void *
__asan_memset(void *block, int c, uintptr_t size)
{
}
