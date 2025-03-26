
exception * __thiscall MSVCRT_exception_ctor(exception * _this, const char ** name);
exception * __thiscall exception_ctor_noalloc(exception * _this, char ** name, int noalloc);
exception * __thiscall exception_copy_ctor(exception * _this, const exception * rhs);
exception * __thiscall exception_default_ctor(exception * _this);
void __thiscall exception_dtor(exception * _this);
exception * __thiscall exception_opequals(exception * _this, const exception * rhs);
void * __thiscall exception_vector_dtor(exception * _this, unsigned int flags);
void * __thiscall exception_scalar_dtor(exception * _this, unsigned int flags);
const char * __thiscall exception_what(exception * _this);
bad_typeid * __thiscall bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs);
bad_typeid * __thiscall bad_typeid_ctor(bad_typeid * _this, const char * name);
bad_typeid * __thiscall bad_typeid_default_ctor(bad_typeid * _this);
void __thiscall bad_typeid_dtor(bad_typeid * _this);
bad_typeid * __thiscall bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs);
void * __thiscall bad_typeid_vector_dtor(bad_typeid * _this, unsigned int flags);
void * __thiscall bad_typeid_scalar_dtor(bad_typeid * _this, unsigned int flags);
__non_rtti_object * __thiscall __non_rtti_object_copy_ctor(__non_rtti_object * _this, const __non_rtti_object * rhs);
__non_rtti_object * __thiscall __non_rtti_object_ctor(__non_rtti_object * _this, const char * name);
void __thiscall __non_rtti_object_dtor(__non_rtti_object * _this);
__non_rtti_object * __thiscall __non_rtti_object_opequals(__non_rtti_object * _this, const __non_rtti_object *rhs);
void * __thiscall __non_rtti_object_vector_dtor(__non_rtti_object * _this, unsigned int flags);
void * __thiscall __non_rtti_object_scalar_dtor(__non_rtti_object * _this, unsigned int flags);
bad_cast * __thiscall bad_cast_ctor(bad_cast * _this, const char ** name);
bad_cast * __thiscall bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs);
bad_cast * __thiscall bad_cast_ctor_charptr(bad_cast * _this, const char * name);
bad_cast * __thiscall bad_cast_default_ctor(bad_cast * _this);
void __thiscall bad_cast_dtor(bad_cast * _this);
bad_cast * __thiscall bad_cast_opequals(bad_cast * _this, const bad_cast * rhs);
void * __thiscall bad_cast_vector_dtor(bad_cast * _this, unsigned int flags);
void * __thiscall bad_cast_scalar_dtor(bad_cast * _this, unsigned int flags);
int __thiscall type_info_opequals_equals(type_info * _this, const type_info * rhs);
int __thiscall type_info_opnot_equals(type_info * _this, const type_info * rhs);
int __thiscall type_info_before(type_info * _this, const type_info * rhs);
void __thiscall type_info_dtor(type_info * _this);
const char * __thiscall type_info_name(type_info * _this);
const char * __thiscall type_info_raw_name(type_info * _this);
void * __thiscall type_info_vector_dtor(type_info * _this, unsigned int flags);
#if _MSVCR_VER >= 80
bad_alloc* __thiscall MSVCRT_bad_alloc_copy_ctor(bad_alloc* _this, const bad_alloc* rhs);
bad_alloc* __thiscall MSVCRT_bad_alloc_copy_ctor(bad_alloc* _this, const bad_alloc* rhs);
void __thiscall MSVCRT_bad_alloc_dtor(bad_alloc* _this);
#endif /* _MSVCR_VER >= 80 */
#if _MSVCR_VER >= 100
scheduler_resource_allocation_error* __thiscall scheduler_resource_allocation_error_ctor_name(
    scheduler_resource_allocation_error* this, const char* name, HRESULT hr);
scheduler_resource_allocation_error* __thiscall scheduler_resource_allocation_error_ctor(
    scheduler_resource_allocation_error* this, HRESULT hr);
scheduler_resource_allocation_error* __thiscall MSVCRT_scheduler_resource_allocation_error_copy_ctor(
    scheduler_resource_allocation_error* this,
    const scheduler_resource_allocation_error* rhs);
HRESULT __thiscall scheduler_resource_allocation_error_get_error_code(
    const scheduler_resource_allocation_error* this);
void __thiscall MSVCRT_scheduler_resource_allocation_error_dtor(
    scheduler_resource_allocation_error* this);
improper_lock* __thiscall improper_lock_ctor_str(improper_lock* this, const char* str);
improper_lock* __thiscall improper_lock_ctor(improper_lock* this);
improper_lock* __thiscall MSVCRT_improper_lock_copy_ctor(improper_lock* _this, const improper_lock* rhs);
void __thiscall MSVCRT_improper_lock_dtor(improper_lock* _this);
invalid_scheduler_policy_key* __thiscall invalid_scheduler_policy_key_ctor_str(
    invalid_scheduler_policy_key* this, const char* str);
invalid_scheduler_policy_key* __thiscall invalid_scheduler_policy_key_ctor(
    invalid_scheduler_policy_key* this);
invalid_scheduler_policy_key* __thiscall MSVCRT_invalid_scheduler_policy_key_copy_ctor(
    invalid_scheduler_policy_key* _this, const invalid_scheduler_policy_key* rhs);
void __thiscall MSVCRT_invalid_scheduler_policy_key_dtor(
    invalid_scheduler_policy_key* _this);
invalid_scheduler_policy_value* __thiscall invalid_scheduler_policy_value_ctor_str(
    invalid_scheduler_policy_value* this, const char* str);
invalid_scheduler_policy_value* __thiscall invalid_scheduler_policy_value_ctor(
    invalid_scheduler_policy_value* this);
invalid_scheduler_policy_value* __thiscall MSVCRT_invalid_scheduler_policy_value_copy_ctor(
    invalid_scheduler_policy_value* _this, const invalid_scheduler_policy_value* rhs);
void __thiscall MSVCRT_invalid_scheduler_policy_value_dtor(
    invalid_scheduler_policy_value* _this);
invalid_scheduler_policy_thread_specification* __thiscall invalid_scheduler_policy_thread_specification_ctor_str(
    invalid_scheduler_policy_thread_specification* this, const char* str);
invalid_scheduler_policy_thread_specification* __thiscall invalid_scheduler_policy_thread_specification_ctor(
    invalid_scheduler_policy_thread_specification* this);
invalid_scheduler_policy_thread_specification* __thiscall MSVCRT_invalid_scheduler_policy_thread_specification_copy_ctor(
    invalid_scheduler_policy_thread_specification* _this, const invalid_scheduler_policy_thread_specification* rhs);
void __thiscall MSVCRT_invalid_scheduler_policy_thread_specification_dtor(
    invalid_scheduler_policy_thread_specification* _this);
improper_scheduler_attach* __thiscall improper_scheduler_attach_ctor_str(
    improper_scheduler_attach* this, const char* str);
improper_scheduler_attach* __thiscall improper_scheduler_attach_ctor(
    improper_scheduler_attach* this);
improper_scheduler_attach* __thiscall MSVCRT_improper_scheduler_attach_copy_ctor(
    improper_scheduler_attach* _this, const improper_scheduler_attach* rhs);
void __thiscall MSVCRT_improper_scheduler_attach_dtor(
    improper_scheduler_attach* _this);
improper_scheduler_detach* __thiscall improper_scheduler_detach_ctor_str(
    improper_scheduler_detach* this, const char* str);
improper_scheduler_detach* __thiscall improper_scheduler_detach_ctor(
    improper_scheduler_detach* this);
improper_scheduler_detach* __thiscall MSVCRT_improper_scheduler_detach_copy_ctor(
    improper_scheduler_detach* _this, const improper_scheduler_detach* rhs);
void __thiscall MSVCRT_improper_scheduler_detach_dtor(
    improper_scheduler_detach* _this);
#endif
