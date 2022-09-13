/*

Required functions

*/
#include <stdint.h>

int __asan_option_detect_stack_use_after_return;
typedef unsigned long long uptr;

extern int __asan_option_detect_stack_use_after_return;

void
__asan_memcpy()
{

}

void
__asan_memmove()
{
}

void
__asan_report_load1() {
}

void
__asan_report_load2() {
}

void
__asan_report_load4() {
}

void
__asan_report_load8() {
}

void
__asan_report_load16() {
}

void
__asan_report_store1() {
}

void
__asan_report_store2() {
}

void
__asan_report_store4() {
}

void
__asan_report_store8() {
}

void
__asan_report_store16() {
}

void
__asan_report_load_n() {
}

void
__asan_report_store_n() {
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
__asan_load1() {
}

void
__asan_load2() {
}

void
__asan_load4() {
}

void
__asan_load8() {
}

void
__asan_load16() {
}

void
__asan_loadN() {
}

void
__asan_store1() {
}

void
__asan_store2() {
}

void
__asan_store4() {
}

void
__asan_store8() {
}

void
__asan_store16() {
}

void
__asan_storeN() {
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
