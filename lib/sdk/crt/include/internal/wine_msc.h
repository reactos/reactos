
#define DEFINE_THISCALL_WRAPPER(func, args) \
    void __declspec(naked) __thiscall_ ## func (void) \
    { \
        __asm { pop eax } \
        __asm { push ecx } \
        __asm { push eax } \
        __asm { jmp func } \
    }

exception * __stdcall MSVCRT_exception_ctor(exception * _this, const char ** name);
exception * __stdcall MSVCRT_exception_ctor_noalloc(exception * _this, char ** name, int noalloc);
exception * __stdcall MSVCRT_exception_copy_ctor(exception * _this, const exception * rhs);
exception * __stdcall MSVCRT_exception_default_ctor(exception * _this);
void __stdcall MSVCRT_exception_dtor(exception * _this);
exception * __stdcall MSVCRT_exception_opequals(exception * _this, const exception * rhs);
void * __stdcall MSVCRT_exception_vector_dtor(exception * _this, unsigned int flags);
void * __stdcall MSVCRT_exception_scalar_dtor(exception * _this, unsigned int flags);
const char * __stdcall MSVCRT_what_exception(exception * _this);
bad_typeid * __stdcall MSVCRT_bad_typeid_copy_ctor(bad_typeid * _this, const bad_typeid * rhs);
bad_typeid * __stdcall MSVCRT_bad_typeid_ctor(bad_typeid * _this, const char * name);
bad_typeid * __stdcall MSVCRT_bad_typeid_default_ctor(bad_typeid * _this);
void __stdcall MSVCRT_bad_typeid_dtor(bad_typeid * _this);
bad_typeid * __stdcall MSVCRT_bad_typeid_opequals(bad_typeid * _this, const bad_typeid * rhs);
void * __stdcall MSVCRT_bad_typeid_vector_dtor(bad_typeid * _this, unsigned int flags);
void * __stdcall MSVCRT_bad_typeid_scalar_dtor(bad_typeid * _this, unsigned int flags);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_copy_ctor(__non_rtti_object * _this, const __non_rtti_object * rhs);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_ctor(__non_rtti_object * _this, const char * name);
void __stdcall MSVCRT___non_rtti_object_dtor(__non_rtti_object * _this);
__non_rtti_object * __stdcall MSVCRT___non_rtti_object_opequals(__non_rtti_object * _this, const __non_rtti_object *rhs);
void * __stdcall MSVCRT___non_rtti_object_vector_dtor(__non_rtti_object * _this, unsigned int flags);
void * __stdcall MSVCRT___non_rtti_object_scalar_dtor(__non_rtti_object * _this, unsigned int flags);
bad_cast * __stdcall MSVCRT_bad_cast_ctor(bad_cast * _this, const char ** name);
bad_cast * __stdcall MSVCRT_bad_cast_copy_ctor(bad_cast * _this, const bad_cast * rhs);
bad_cast * __stdcall MSVCRT_bad_cast_ctor_charptr(bad_cast * _this, const char * name);
bad_cast * __stdcall MSVCRT_bad_cast_default_ctor(bad_cast * _this);
void __stdcall MSVCRT_bad_cast_dtor(bad_cast * _this);
bad_cast * __stdcall MSVCRT_bad_cast_opequals(bad_cast * _this, const bad_cast * rhs);
void * __stdcall MSVCRT_bad_cast_vector_dtor(bad_cast * _this, unsigned int flags);
void * __stdcall MSVCRT_bad_cast_scalar_dtor(bad_cast * _this, unsigned int flags);
int __stdcall MSVCRT_type_info_opequals_equals(type_info * _this, const type_info * rhs);
int __stdcall MSVCRT_type_info_opnot_equals(type_info * _this, const type_info * rhs);
int __stdcall MSVCRT_type_info_before(type_info * _this, const type_info * rhs);
void __stdcall MSVCRT_type_info_dtor(type_info * _this);
const char * __stdcall MSVCRT_type_info_name(type_info * _this);
const char * __stdcall MSVCRT_type_info_raw_name(type_info * _this);
void * __stdcall MSVCRT_type_info_vector_dtor(type_info * _this, unsigned int flags);

#define __ASM_VTABLE(name,funcs)
//void *MSVCRT_ ## name ##_vtable[] =
