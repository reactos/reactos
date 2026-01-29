
#define bad_cast exception
#define bad_typeid exception
#define __non_rtti_object exception

exception* __thiscall exception_ctor(exception* _this, const char** name);
exception* __thiscall exception_copy_ctor(exception* _this, const exception* rhs);
exception* __thiscall exception_ctor_noalloc(exception* _this, char** name, int noalloc);
exception* __thiscall exception_default_ctor(exception* _this);
exception* __thiscall exception_opequals(exception* _this, const exception* rhs);
void __thiscall exception_dtor(exception* _this);
void* __thiscall exception_vector_dtor(exception* _this, unsigned int flags);
void* __thiscall exception_scalar_dtor(exception* _this, unsigned int flags);
const char* __thiscall exception_what(exception* _this);
bad_typeid* __thiscall bad_typeid_copy_ctor(bad_typeid* _this, const bad_typeid* rhs);
bad_typeid* __thiscall bad_typeid_ctor(bad_typeid* _this, const char* name);
bad_typeid* __thiscall bad_typeid_default_ctor(bad_typeid* _this);
void __thiscall bad_typeid_dtor(bad_typeid* _this);
bad_typeid* __thiscall bad_typeid_opequals(bad_typeid* _this, const bad_typeid* rhs);
void* __thiscall bad_typeid_vector_dtor(bad_typeid* _this, unsigned int flags);
void* __thiscall bad_typeid_scalar_dtor(bad_typeid* _this, unsigned int flags);
__non_rtti_object* __thiscall __non_rtti_object_copy_ctor(__non_rtti_object* _this, const __non_rtti_object* rhs);
__non_rtti_object* __thiscall __non_rtti_object_ctor(__non_rtti_object* _this, const char* name);
void __thiscall __non_rtti_object_dtor(__non_rtti_object* _this);
__non_rtti_object* __thiscall __non_rtti_object_opequals(__non_rtti_object* _this, const __non_rtti_object* rhs);
void* __thiscall __non_rtti_object_vector_dtor(__non_rtti_object* _this, unsigned int flags);
void* __thiscall __non_rtti_object_scalar_dtor(__non_rtti_object* _this, unsigned int flags);
bad_cast* __thiscall bad_cast_ctor(bad_cast* _this, const char** name);
bad_cast* __thiscall bad_cast_copy_ctor(bad_cast* _this, const bad_cast* rhs);
bad_cast* __thiscall bad_cast_ctor_charptr(bad_cast* _this, const char* name);
bad_cast* __thiscall bad_cast_default_ctor(bad_cast* _this);
void __thiscall bad_cast_dtor(bad_cast* _this);
void* __thiscall bad_cast_vector_dtor(bad_cast* _this, unsigned int flags);
void* __thiscall bad_cast_scalar_dtor(bad_cast* _this, unsigned int flags);
bad_cast* __thiscall bad_cast_opequals(bad_cast* _this, const bad_cast* rhs);
int __thiscall type_info_opequals_equals(type_info* _this, const type_info* rhs);
int __thiscall type_info_opnot_equals(type_info* _this, const type_info* rhs);
int __thiscall type_info_before(type_info* _this, const type_info *rhs);
void __thiscall type_info_dtor(type_info* _this);
const char* __thiscall type_info_name(type_info* _this);
const char* __thiscall type_info_raw_name(type_info* _this);
struct __type_info_node;
const char * __thiscall type_info_name_internal_method(type_info * _this, struct __type_info_node *node);

#undef bad_cast
#undef bad_typeid
#undef __non_rtti_object
