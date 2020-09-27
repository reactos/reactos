# msvcrt.dll - MS VC++ Run Time Library

@ cdecl -arch=x86_64 -version=0x502 $I10_OUTPUT() I10_OUTPUT

# **************** x86 C++ functions ****************
@ cdecl -arch=i386 -norelay ??0__non_rtti_object@@QAE@ABV0@@Z(ptr) MSVCRT___non_rtti_object_copy_ctor # public: __thiscall __non_rtti_object::__non_rtti_object(class __non_rtti_object const &)
@ cdecl -arch=i386 -norelay ??0__non_rtti_object@@QAE@PBD@Z(ptr) MSVCRT___non_rtti_object_ctor # public: __thiscall __non_rtti_object::__non_rtti_object(char const *)
@ cdecl -arch=i386 -norelay ??0bad_cast@@AAE@PBQBD@Z(ptr) MSVCRT_bad_cast_ctor # private: __thiscall bad_cast::bad_cast(char const * const *)
@ cdecl -arch=i386 -norelay ??0bad_cast@@QAE@ABQBD@Z(ptr) MSVCRT_bad_cast_ctor # public: __thiscall bad_cast::bad_cast(char const * const &)
@ cdecl -arch=i386 -norelay ??0bad_cast@@QAE@ABV0@@Z(ptr) MSVCRT_bad_cast_copy_ctor # public: __thiscall bad_cast::bad_cast(class bad_cast const &)
@ cdecl -arch=i386 -norelay ??0bad_cast@@QAE@PBD@Z(ptr) MSVCRT_bad_cast_ctor_charptr # public: __thiscall bad_cast::bad_cast(char const *)
@ cdecl -arch=i386 -norelay ??0bad_typeid@@QAE@ABV0@@Z(ptr) MSVCRT_bad_typeid_copy_ctor # public: __thiscall bad_typeid::bad_typeid(class bad_typeid const &)
@ cdecl -arch=i386 -norelay ??0bad_typeid@@QAE@PBD@Z(ptr) MSVCRT_bad_typeid_ctor # public: __thiscall bad_typeid::bad_typeid(char const *)
@ cdecl -arch=i386 -norelay ??0exception@@QAE@ABQBD@Z(ptr) MSVCRT_exception_ctor # public: __thiscall exception::exception(char const * const &)
@ cdecl -arch=i386 -norelay ??0exception@@QAE@ABQBDH@Z(ptr long) MSVCRT_exception_ctor_noalloc # public: __thiscall exception::exception(char const * const &,int)
@ cdecl -arch=i386 -norelay ??0exception@@QAE@ABV0@@Z(ptr) MSVCRT_exception_copy_ctor # public: __thiscall exception::exception(class exception const &)
@ cdecl -arch=i386 -norelay ??0exception@@QAE@XZ() MSVCRT_exception_default_ctor # public: __thiscall exception::exception(void)
@ cdecl -arch=i386 -norelay ??1__non_rtti_object@@UAE@XZ() MSVCRT___non_rtti_object_dtor # public: virtual __thiscall __non_rtti_object::~__non_rtti_object(void)
@ cdecl -arch=i386 -norelay ??1bad_cast@@UAE@XZ() MSVCRT_bad_cast_dtor # public: virtual __thiscall bad_cast::~bad_cast(void)
@ cdecl -arch=i386 -norelay ??1bad_typeid@@UAE@XZ() MSVCRT_bad_typeid_dtor # public: virtual __thiscall bad_typeid::~bad_typeid(void)
@ cdecl -arch=i386 -norelay ??1exception@@UAE@XZ() MSVCRT_exception_dtor # public: virtual __thiscall exception::~exception(void)
@ cdecl -arch=i386 -norelay ??1type_info@@UAE@XZ() MSVCRT_type_info_dtor # public: virtual __thiscall type_info::~type_info(void)
@ cdecl -arch=i386 ??2@YAPAXI@Z(long) MSVCRT_operator_new # void * __cdecl operator new(unsigned int)
;@ cdecl -arch=i386 ??2@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg # void * __cdecl operator new(unsigned int,int,char const *,int)
@ cdecl -arch=i386 ??3@YAXPAX@Z(ptr) MSVCRT_operator_delete # void __cdecl operator delete(void *)
@ cdecl -arch=i386 -norelay ??4__non_rtti_object@@QAEAAV0@ABV0@@Z(ptr) MSVCRT___non_rtti_object_opequals # public: class __non_rtti_object & __thiscall __non_rtti_object::operator=(class __non_rtti_object const &)
@ cdecl -arch=i386 -norelay ??4bad_cast@@QAEAAV0@ABV0@@Z(ptr) MSVCRT_bad_cast_opequals # public: class bad_cast & __thiscall bad_cast::operator=(class bad_cast const &)
@ cdecl -arch=i386 -norelay ??4bad_typeid@@QAEAAV0@ABV0@@Z(ptr) MSVCRT_bad_typeid_opequals # public: class bad_typeid & __thiscall bad_typeid::operator=(class bad_typeid const &)
@ cdecl -arch=i386 -norelay ??4exception@@QAEAAV0@ABV0@@Z(ptr) MSVCRT_exception_opequals # public: class exception & __thiscall exception::operator=(class exception const &)
@ cdecl -arch=i386 -norelay ??8type_info@@QBEHABV0@@Z(ptr) MSVCRT_type_info_opequals_equals # public: int __thiscall type_info::operator==(class type_info const &)const
@ cdecl -arch=i386 -norelay ??9type_info@@QBEHABV0@@Z(ptr) MSVCRT_type_info_opnot_equals # public: int __thiscall type_info::operator!=(class type_info const &)const
@ extern -arch=i386 ??_7__non_rtti_object@@6B@ MSVCRT___non_rtti_object_vtable # const __non_rtti_object::`vftable'
@ extern -arch=i386 ??_7bad_cast@@6B@ MSVCRT_bad_cast_vtable # const bad_cast::`vftable'
@ extern -arch=i386 ??_7bad_typeid@@6B@ MSVCRT_bad_typeid_vtable # const bad_typeid::`vftable'
@ extern -arch=i386 ??_7exception@@6B@ MSVCRT_exception_vtable # const exception::`vftable'
@ cdecl -arch=i386 -norelay ??_E__non_rtti_object@@UAEPAXI@Z(long) MSVCRT___non_rtti_object_vector_dtor # public: virtual void * __thiscall __non_rtti_object::`vector deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Ebad_cast@@UAEPAXI@Z(long) MSVCRT_bad_cast_vector_dtor # public: virtual void * __thiscall bad_cast::`vector deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Ebad_typeid@@UAEPAXI@Z(long) MSVCRT_bad_typeid_vector_dtor # public: virtual void * __thiscall bad_typeid::`vector deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Eexception@@UAEPAXI@Z(long) MSVCRT_exception_vector_dtor # public: virtual void * __thiscall exception::`vector deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Fbad_cast@@QAEXXZ() MSVCRT_bad_cast_default_ctor # public: void __thiscall bad_cast::`default constructor closure'(void)
@ cdecl -arch=i386 -norelay ??_Fbad_typeid@@QAEXXZ() MSVCRT_bad_typeid_default_ctor # public: void __thiscall bad_typeid::`default constructor closure'(void)
@ cdecl -arch=i386 -norelay ??_G__non_rtti_object@@UAEPAXI@Z(long) MSVCRT___non_rtti_object_scalar_dtor # public: virtual void * __thiscall __non_rtti_object::`scalar deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Gbad_cast@@UAEPAXI@Z(long) MSVCRT_bad_cast_scalar_dtor # public: virtual void * __thiscall bad_cast::`scalar deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Gbad_typeid@@UAEPAXI@Z(long) MSVCRT_bad_typeid_scalar_dtor # public: virtual void * __thiscall bad_typeid::`scalar deleting destructor'(unsigned int)
@ cdecl -arch=i386 -norelay ??_Gexception@@UAEPAXI@Z(long) MSVCRT_exception_scalar_dtor # public: virtual void * __thiscall exception::`scalar deleting destructor'(unsigned int)
@ cdecl -arch=i386 ??_U@YAPAXI@Z(long) MSVCRT_operator_new # void * __cdecl operator new[](unsigned int)
;@ cdecl -arch=i386 ??_U@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg # void * __cdecl operator new[](unsigned int,int,char const *,int)
@ cdecl -arch=i386 ??_V@YAXPAX@Z(ptr) MSVCRT_operator_delete # void __cdecl operator delete[](void *)
@ cdecl -arch=i386 -norelay __uncaught_exception(ptr) MSVCRT___uncaught_exception
@ cdecl -arch=i386 -norelay ?_query_new_handler@@YAP6AHI@ZXZ() MSVCRT__query_new_handler # int (__cdecl*__cdecl _query_new_handler(void))(unsigned int)
@ cdecl -arch=i386 ?_query_new_mode@@YAHXZ() MSVCRT__query_new_mode # int __cdecl _query_new_mode(void)
@ cdecl -arch=i386 -norelay ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) MSVCRT__set_new_handler # int (__cdecl*__cdecl _set_new_handler(int (__cdecl*)(unsigned int)))(unsigned int)
@ cdecl -arch=i386 ?_set_new_mode@@YAHH@Z(long) MSVCRT__set_new_mode # int __cdecl _set_new_mode(int)
@ cdecl -arch=i386 -norelay ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator # void (__cdecl*__cdecl _set_se_translator(void (__cdecl*)(unsigned int,struct _EXCEPTION_POINTERS *)))(unsigned int,struct _EXCEPTION_POINTERS *)
@ cdecl -arch=i386 -norelay ?before@type_info@@QBEHABV1@@Z(ptr) MSVCRT_type_info_before # public: int __thiscall type_info::before(class type_info const &)const
@ cdecl -arch=i386 -norelay ?name@type_info@@QBEPBDXZ() MSVCRT_type_info_name # public: char const * __thiscall type_info::name(void)const
@ cdecl -arch=i386 -norelay ?raw_name@type_info@@QBEPBDXZ() MSVCRT_type_info_raw_name # public: char const * __thiscall type_info::raw_name(void)const
@ cdecl -arch=i386 ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_new_handler # void (__cdecl*__cdecl set_new_handler(void (__cdecl*)(void)))(void)
@ cdecl -arch=i386 ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_terminate # void (__cdecl*__cdecl set_terminate(void (__cdecl*)(void)))(void)
@ cdecl -arch=i386 ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_unexpected # void (__cdecl*__cdecl set_unexpected(void (__cdecl*)(void)))(void)
@ cdecl -arch=i386 ?terminate@@YAXXZ() MSVCRT_terminate # void __cdecl terminate(void)
@ cdecl -arch=i386 ?unexpected@@YAXXZ() MSVCRT_unexpected # void __cdecl unexpected(void)
@ cdecl -arch=i386 -norelay ?what@exception@@UBEPBDXZ() MSVCRT_what_exception # public: virtual char const * __thiscall exception::what(void)const

# **************** win64 C++ functions ****************
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@AEBV0@@Z(ptr) MSVCRT___non_rtti_object_copy_ctor # public: __cdecl __non_rtti_object::__non_rtti_object(class __non_rtti_object const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0__non_rtti_object@@QEAA@PEBD@Z(ptr) MSVCRT___non_rtti_object_ctor # public: __cdecl __non_rtti_object::__non_rtti_object(char const * __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_cast@@AAE@PBQBD@Z(ptr) MSVCRT_bad_cast_ctor # private: __thiscall bad_cast::bad_cast(char const near * const near *)
@ cdecl -arch=win64 ??0bad_cast@@AEAA@PEBQEBD@Z(ptr) MSVCRT_bad_cast_ctor # private: __cdecl bad_cast::bad_cast(char const * __ptr64 const * __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_cast@@QAE@ABQBD@Z(ptr) MSVCRT_bad_cast_ctor # public: __thiscall bad_cast::bad_cast(char const near * const near &)
@ cdecl -arch=win64 ??0bad_cast@@QEAA@AEBQEBD@Z(ptr) MSVCRT_bad_cast_ctor # public: __cdecl bad_cast::bad_cast(char const * __ptr64 const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_cast@@QEAA@AEBV0@@Z(ptr) MSVCRT_bad_cast_copy_ctor # public: __cdecl bad_cast::bad_cast(class bad_cast const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_cast@@QEAA@PEBD@Z(ptr) MSVCRT_bad_cast_ctor_charptr # public: __cdecl bad_cast::bad_cast(char const * __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@AEBV0@@Z(ptr) MSVCRT_bad_typeid_copy_ctor # public: __cdecl bad_typeid::bad_typeid(class bad_typeid const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0bad_typeid@@QEAA@PEBD@Z(ptr) MSVCRT_bad_typeid_ctor # public: __cdecl bad_typeid::bad_typeid(char const * __ptr64) __ptr64
@ cdecl -arch=win64 ??0exception@@QEAA@AEBQEBD@Z(ptr) MSVCRT_exception_ctor # public: __cdecl exception::exception(char const * __ptr64 const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0exception@@QEAA@AEBQEBDH@Z(ptr long) MSVCRT_exception_ctor_noalloc # public: __cdecl exception::exception(char const * __ptr64 const & __ptr64,int) __ptr64
@ cdecl -arch=win64 ??0exception@@QEAA@AEBV0@@Z(ptr) MSVCRT_exception_copy_ctor # public: __cdecl exception::exception(class exception const & __ptr64) __ptr64
@ cdecl -arch=win64 ??0exception@@QEAA@XZ() MSVCRT_exception_default_ctor # public: __cdecl exception::exception(void) __ptr64
@ cdecl -arch=win64 ??1__non_rtti_object@@UEAA@XZ() MSVCRT___non_rtti_object_dtor # public: virtual __cdecl __non_rtti_object::~__non_rtti_object(void) __ptr64
@ cdecl -arch=win64 ??1bad_cast@@UEAA@XZ() MSVCRT_bad_cast_dtor # public: virtual __cdecl bad_cast::~bad_cast(void) __ptr64
@ cdecl -arch=win64 ??1bad_typeid@@UEAA@XZ() MSVCRT_bad_typeid_dtor # public: virtual __cdecl bad_typeid::~bad_typeid(void) __ptr64
@ cdecl -arch=win64 ??1exception@@UEAA@XZ() MSVCRT_exception_dtor # public: virtual __cdecl exception::~exception(void) __ptr64
@ cdecl -arch=win64 ??1type_info@@UEAA@XZ() MSVCRT_type_info_dtor # public: virtual __cdecl type_info::~type_info(void) __ptr64
@ cdecl -arch=win64 ??2@YAPEAX_K@Z(double) MSVCRT_operator_new # void * __ptr64 __cdecl operator new(unsigned __int64)
@ cdecl -arch=win64 ??2@YAPEAX_KHPEBDH@Z(int64 long str long) MSVCRT_operator_new_dbg # void * __ptr64 __cdecl operator new(unsigned __int64,int,char const * __ptr64,int)
@ cdecl -arch=win64 ??3@YAXPEAX@Z(ptr) MSVCRT_operator_delete # void __cdecl operator delete(void * __ptr64)
@ cdecl -arch=win64 ??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z(ptr) MSVCRT___non_rtti_object_opequals # public: class __non_rtti_object & __ptr64 __cdecl __non_rtti_object::operator=(class __non_rtti_object const & __ptr64) __ptr64
@ cdecl -arch=win64 ??4bad_cast@@QEAAAEAV0@AEBV0@@Z(ptr) MSVCRT_bad_cast_opequals # public: class bad_cast & __ptr64 __cdecl bad_cast::operator=(class bad_cast const & __ptr64) __ptr64
@ cdecl -arch=win64 ??4bad_typeid@@QEAAAEAV0@AEBV0@@Z(ptr) MSVCRT_bad_typeid_opequals # public: class bad_typeid & __ptr64 __cdecl bad_typeid::operator=(class bad_typeid const & __ptr64) __ptr64
@ cdecl -arch=win64 ??4exception@@QEAAAEAV0@AEBV0@@Z(ptr) MSVCRT_exception_opequals # public: class exception & __ptr64 __cdecl exception::operator=(class exception const & __ptr64) __ptr64
@ cdecl -arch=win64 ??8type_info@@QEBAHAEBV0@@Z(ptr) MSVCRT_type_info_opequals_equals # public: int __cdecl type_info::operator==(class type_info const & __ptr64)const __ptr64
@ cdecl -arch=win64 ??9type_info@@QEBAHAEBV0@@Z(ptr) MSVCRT_type_info_opnot_equals # public: int __cdecl type_info::operator!=(class type_info const & __ptr64)const __ptr64
@ extern -arch=win64 ??_7__non_rtti_object@@6B@ MSVCRT___non_rtti_object_vtable # const __non_rtti_object::`vftable'
@ extern -arch=win64 ??_7bad_cast@@6B@ MSVCRT_bad_cast_vtable # const bad_cast::`vftable'
@ extern -arch=win64 ??_7bad_typeid@@6B@ MSVCRT_bad_typeid_vtable # const bad_typeid::`vftable'
@ extern -arch=win64 ??_7exception@@6B@ MSVCRT_exception_vtable # const exception::`vftable'
@ cdecl -arch=win64 ??_Fbad_cast@@QEAAXXZ() MSVCRT_bad_cast_default_ctor # public: void __cdecl bad_cast::`default constructor closure'(void) __ptr64
@ cdecl -arch=win64 ??_Fbad_typeid@@QEAAXXZ() MSVCRT_bad_typeid_default_ctor # public: void __cdecl bad_typeid::`default constructor closure'(void) __ptr64
@ cdecl -arch=win64 ??_U@YAPEAX_K@Z(long) MSVCRT_operator_new # void * __ptr64 __cdecl operator new[](unsigned __int64)
@ cdecl -arch=win64 ??_U@YAPEAX_KHPEBDH@Z(int64 long str long) MSVCRT_operator_new_dbg # void * __ptr64 __cdecl operator new[](unsigned __int64,int,char const * __ptr64,int)
@ cdecl -arch=win64 ??_V@YAXPEAX@Z(ptr) MSVCRT_operator_delete # void __cdecl operator delete[](void * __ptr64)
@ cdecl -arch=win64 __uncaught_exception(ptr) MSVCRT___uncaught_exception
@ cdecl -arch=win64 ?_query_new_handler@@YAP6AH_K@ZXZ() MSVCRT__query_new_handler # int (__cdecl*__cdecl _query_new_handler(void))(unsigned __int64)
@ cdecl -arch=win64 ?_query_new_mode@@YAHXZ() MSVCRT__query_new_mode # int __cdecl _query_new_mode(void)
@ cdecl -arch=win64 ?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z(ptr) MSVCRT__set_new_handler # int (__cdecl*__cdecl _set_new_handler(int (__cdecl*)(unsigned __int64)))(unsigned __int64)
@ cdecl -arch=win64 ?_set_new_mode@@YAHH@Z(long) MSVCRT__set_new_mode # int __cdecl _set_new_mode(int)
@ cdecl -arch=win64 ?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator # void (__cdecl*__cdecl _set_se_translator(void (__cdecl*)(unsigned int,struct _EXCEPTION_POINTERS * __ptr64)))(unsigned int,struct _EXCEPTION_POINTERS * __ptr64)
@ cdecl -arch=win64 ?before@type_info@@QEBAHAEBV1@@Z(ptr) MSVCRT_type_info_before # public: int __cdecl type_info::before(class type_info const & __ptr64)const __ptr64
@ cdecl -arch=win64 ?name@type_info@@QEBAPEBDXZ() MSVCRT_type_info_name # public: char const * __ptr64 __cdecl type_info::name(void)const __ptr64
@ cdecl -arch=win64 ?raw_name@type_info@@QEBAPEBDXZ() MSVCRT_type_info_raw_name # public: char const * __ptr64 __cdecl type_info::raw_name(void)const __ptr64
@ cdecl -arch=win64 ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_new_handler # void (__cdecl*__cdecl set_new_handler(void (__cdecl*)(void)))(void)
@ cdecl -arch=win64 ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_terminate # void (__cdecl*__cdecl set_terminate(void (__cdecl*)(void)))(void)
@ cdecl -arch=win64 ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_unexpected # void (__cdecl*__cdecl set_unexpected(void (__cdecl*)(void)))(void)
@ cdecl -arch=win64 ?terminate@@YAXXZ() MSVCRT_terminate # void __cdecl terminate(void)
@ cdecl -arch=win64 ?unexpected@@YAXXZ() MSVCRT_unexpected # void __cdecl unexpected(void)
@ cdecl -arch=win64 ?what@exception@@UEBAPEBDXZ() MSVCRT_what_exception # public: virtual char const * __ptr64 __cdecl exception::what(void)const __ptr64

# **************** ARM C++ functions ****************
@ cdecl -arch=arm ??0__non_rtti_object@@QAA@ABV0@@Z() MSVCRT___non_rtti_object_copy_ctor # public: __cdecl __non_rtti_object::__non_rtti_object(class __non_rtti_object const &)
@ cdecl -arch=arm ??0__non_rtti_object@@QAA@PBD@Z() MSVCRT___non_rtti_object_ctor #public: __cdecl __non_rtti_object::__non_rtti_object(char const *)
@ cdecl -arch=arm ??0bad_cast@@AAA@PBQBD@Z() MSVCRT_bad_cast_ctor # private: __cdecl bad_cast::bad_cast(char const * const *)
@ cdecl -arch=arm ??0bad_cast@@QAA@ABV0@@Z() MSVCRT_bad_cast_copy_ctor # public: __cdecl bad_cast::bad_cast(class bad_cast const &)
@ cdecl -arch=arm ??0bad_cast@@QAA@PBD@Z() MSVCRT_bad_cast_ctor # public: __cdecl bad_cast::bad_cast(char const *)
@ cdecl -arch=arm ??0bad_typeid@@QAA@ABV0@@Z() MSVCRT_bad_typeid_copy_ctor # public: __cdecl bad_typeid::bad_typeid(class bad_typeid const &)
@ cdecl -arch=arm ??0bad_typeid@@QAA@PBD@Z() MSVCRT_bad_typeid_ctor # public: __cdecl bad_typeid::bad_typeid(char const *)
@ cdecl -arch=arm ??0exception@@QAA@ABQBD@Z() MSVCRT_exception_ctor # public: __cdecl exception::exception(char const * const &)
@ cdecl -arch=arm ??0exception@@QAA@ABQBDH@Z() MSVCRT_exception_ctor_noalloc # public: __cdecl exception::exception(char const * const &,int)
@ cdecl -arch=arm ??0exception@@QAA@ABV0@@Z() MSVCRT_exception_copy_ctor # public: __cdecl exception::exception(class exception const &)
@ cdecl -arch=arm ??0exception@@QAA@XZ() MSVCRT_exception_default_ctor # public: __cdecl exception::exception(void)
@ cdecl -arch=arm ??1__non_rtti_object@@UAA@XZ() MSVCRT___non_rtti_object_dtor # public: virtual __cdecl __non_rtti_object::~__non_rtti_object(void)
@ cdecl -arch=arm ??1bad_cast@@UAA@XZ() MSVCRT_bad_cast_dtor # public: virtual __cdecl bad_cast::~bad_cast(void)
@ cdecl -arch=arm ??1bad_typeid@@UAA@XZ() MSVCRT_bad_typeid_dtor # public: virtual __cdecl bad_typeid::~bad_typeid(void)
@ cdecl -arch=arm ??1exception@@UAA@XZ() MSVCRT_exception_dtor # public: virtual __cdecl exception::~exception(void)
@ cdecl -arch=arm ??1type_info@@UAA@XZ() MSVCRT_type_info_dtor # public: virtual __cdecl type_info::~type_info(void)
@ cdecl -arch=arm ??2@YAPAXI@Z() MSVCRT_operator_new # void * __cdecl operator new(unsigned int)
@ cdecl -arch=arm ??2@YAPAXIHPBDH@Z() MSVCRT_operator_new_dbg # void * __cdecl operator new(unsigned int,int,char const *,int)
@ cdecl -arch=arm ??3@YAXPAX@Z() MSVCRT_operator_delete # void __cdecl operator delete(void *)
@ cdecl -arch=arm ??4__non_rtti_object@@QAAAAV0@ABV0@@Z() MSVCRT___non_rtti_object_opequals # public: class __non_rtti_object & __cdecl __non_rtti_object::operator=(class __non_rtti_object const &)
@ cdecl -arch=arm ??4bad_cast@@QAAAAV0@ABV0@@Z() MSVCRT_bad_cast_opequals # public: class bad_cast & __cdecl bad_cast::operator=(class bad_cast const &)
@ cdecl -arch=arm ??4bad_typeid@@QAAAAV0@ABV0@@Z() MSVCRT_bad_typeid_opequals # public: class bad_typeid & __cdecl bad_typeid::operator=(class bad_typeid const &)
@ cdecl -arch=arm ??4exception@@QAAAAV0@ABV0@@Z() MSVCRT_exception_opequals # public: class exception & __cdecl exception::operator=(class exception const &)
@ cdecl -arch=arm ??8type_info@@QBAHABV0@@Z() MSVCRT_type_info_opequals_equals # public: int __cdecl type_info::operator==(class type_info const &)const
@ cdecl -arch=arm ??9type_info@@QBAHABV0@@Z() MSVCRT_type_info_opnot_equals # public: int __cdecl type_info::operator!=(class type_info const &)const
@ extern -arch=arm ??_7__non_rtti_object@@6B@ MSVCRT___non_rtti_object_vtable # const __non_rtti_object::`vftable'
@ extern -arch=arm ??_7bad_cast@@6B@  MSVCRT_bad_cast_vtable # const bad_cast::`vftable'
@ extern -arch=arm ??_7bad_typeid@@6B@ MSVCRT_bad_typeid_vtable # const bad_typeid::`vftable'
@ extern -arch=arm ??_7exception@@6B@ MSVCRT_exception_vtable # const exception::`vftable'
@ cdecl -arch=arm ??_Fbad_cast@@QAAXXZ() MSVCRT_bad_cast_default_ctor # public: void __cdecl bad_cast::`default constructor closure'(void)
@ cdecl -arch=arm ??_Fbad_typeid@@QAAXXZ() MSVCRT_bad_typeid_default_ctor # public: void __cdecl bad_typeid::`default constructor closure'(void)
@ cdecl -arch=arm ??_U@YAPAXI@Z() MSVCRT_operator_new # void * __cdecl operator new[](unsigned int)
@ cdecl -arch=arm ??_U@YAPAXIHPBDH@Z() MSVCRT_operator_new_dbg # void * __cdecl operator new[](unsigned int,int,char const *,int)
@ cdecl -arch=arm ??_V@YAXPAX@Z() MSVCRT_operator_delete # void __cdecl operator delete[](void *)
;@ cdecl -arch=arm _CallMemberFunction0()
;@ cdecl -arch=arm _CallMemberFunction1()
;@ cdecl -arch=arm _CallMemberFunction2()
;@ cdecl -arch=arm __ExceptionPtrAssign()
;@ cdecl -arch=arm __ExceptionPtrCompare()
;@ cdecl -arch=arm __ExceptionPtrCopy()
;@ cdecl -arch=arm __ExceptionPtrCopyException()
;@ cdecl -arch=arm __ExceptionPtrCreate()
;@ cdecl -arch=arm __ExceptionPtrCurrentException()
;@ cdecl -arch=arm __ExceptionPtrDestroy()
;@ cdecl -arch=arm __ExceptionPtrRethrow()
;@ cdecl -arch=arm __ExceptionPtrSwap()
;@ cdecl -arch=arm __ExceptionPtrToBool()
@ cdecl -arch=arm __uncaught_exception(ptr) MSVCRT___uncaught_exception
@ cdecl -arch=arm ?_query_new_handler@@YAP6AHI@ZXZ() MSVCRT__query_new_handler # int (__cdecl*__cdecl _query_new_handler(void))(unsigned int)
@ cdecl -arch=arm ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z() MSVCRT__set_new_handler # int (__cdecl*__cdecl _set_new_handler(int (__cdecl*)(unsigned int)))(unsigned int)
@ cdecl -arch=arm ?_set_new_mode@@YAHH@Z() MSVCRT__set_new_mode # int __cdecl _set_new_mode(int)
@ cdecl -arch=arm ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z() MSVCRT__set_se_translator # void (__cdecl*__cdecl _set_se_translator(void (__cdecl*)(unsigned int,struct _EXCEPTION_POINTERS *)))(unsigned int,struct _EXCEPTION_POINTERS *)
@ cdecl -arch=arm ?before@type_info@@QBAHABV1@@Z() MSVCRT_type_info_before # public: int __cdecl type_info::before(class type_info const &)const
@ cdecl -arch=arm ?name@type_info@@QBAPBDXZ() MSVCRT_type_info_name # public: char const * __cdecl type_info::name(void)const
@ cdecl -arch=arm ?raw_name@type_info@@QBAPBDXZ() MSVCRT_type_info_raw_name # public: char const * __cdecl type_info::raw_name(void)const
@ cdecl -arch=arm ?set_terminate@@YAP6AXXZP6AXXZ@Z() MSVCRT_set_terminate # void (__cdecl*__cdecl set_terminate(void (__cdecl*)(void)))(void)
@ cdecl -arch=arm ?set_unexpected@@YAP6AXXZP6AXXZ@Z() MSVCRT_set_unexpected # void (__cdecl*__cdecl set_unexpected(void (__cdecl*)(void)))(void)
@ cdecl -arch=arm ?terminate@@YAXXZ() MSVCRT_terminate # void __cdecl terminate(void)
@ cdecl -arch=arm ?unexpected@@YAXXZ() MSVCRT_unexpected # void __cdecl unexpected(void)
@ cdecl -arch=arm ?what@exception@@UBAPBDXZ() MSVCRT_what_exception # public: virtual char const * __cdecl exception::what(void)const


# **************** Common functions ****************
@ cdecl -arch=i386 $I10_OUTPUT() I10_OUTPUT
@ cdecl -arch=i386 _CIacos()
@ cdecl -arch=i386 _CIasin()
@ cdecl -arch=i386 _CIatan()
@ cdecl -arch=i386 _CIatan2()
@ cdecl -arch=i386 _CIcos()
@ cdecl -arch=i386 _CIcosh()
@ cdecl -arch=i386 _CIexp()
@ cdecl -arch=i386 _CIfmod()
@ cdecl -arch=i386 _CIlog()
@ cdecl -arch=i386 _CIlog10()
@ cdecl -arch=i386 _CIpow()
@ cdecl -arch=i386 _CIsin()
@ cdecl -arch=i386 _CIsinh()
@ cdecl -arch=i386 _CIsqrt()
@ cdecl -arch=i386 _CItan()
@ cdecl -arch=i386 _CItanh()
@ stub -version=0x600+ _CrtCheckMemory
@ stub -version=0x600+ _CrtDbgBreak
@ cdecl -version=0x600+ _CrtDbgReport(long str long str str)
@ cdecl -version=0x600+ _CrtDbgReportV(long str long str str ptr)
@ cdecl -version=0x600+ _CrtDbgReportW(long wstr long wstr wstr)
@ cdecl -version=0x600+ _CrtDbgReportWV(long wstr long wstr wstr ptr)
@ stub -version=0x600+ _CrtDoForAllClientObjects
@ stub -version=0x600+ _CrtDumpMemoryLeaks
@ stub -version=0x600+ _CrtIsMemoryBlock
@ stub -version=0x600+ _CrtIsValidHeapPointer
@ stub -version=0x600+ _CrtIsValidPointer
@ stub -version=0x600+ _CrtMemCheckpoint
@ stub -version=0x600+ _CrtMemDifference
@ stub -version=0x600+ _CrtMemDumpAllObjectsSince
@ stub -version=0x600+ _CrtMemDumpStatistics
@ stub -version=0x600+ _CrtReportBlockType
@ stub -version=0x600+ _CrtSetAllocHook
@ stub -version=0x600+ _CrtSetBreakAlloc
@ stub -version=0x600+ _CrtSetDbgBlockType
@ stub -version=0x600+ _CrtSetDbgFlag
@ stub -version=0x600+ _CrtSetDumpClient
@ cdecl -version=0x600+ _CrtSetReportFile(long ptr)
@ stub -version=0x600+ _CrtSetReportHook
@ stub -version=0x600+ _CrtSetReportHook2
@ cdecl -version=0x600+ _CrtSetReportMode(long long)
@ stdcall _CxxThrowException(long long)
@ cdecl -arch=i386 -norelay _EH_prolog()
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ extern _HUGE
@ cdecl _Strftime(str long str ptr ptr)
@ cdecl _XcptFilter(long ptr)
@ stdcall -arch=x86_64,arm __C_specific_handler(ptr long ptr ptr)
@ cdecl __CppXcptFilter(long ptr)
@ stub -version=0x600+ -arch=i386 __CxxCallUnwindDelDtor
@ stub -arch=i386 __CxxCallUnwindDtor
@ stub -arch=i386 __CxxCallUnwindVecDtor
@ cdecl -arch=i386 __CxxDetectRethrow(ptr)
@ cdecl -arch=i386 __CxxExceptionFilter()
@ cdecl -arch=i386,x86_64 -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ cdecl -arch=i386 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -version=0x600+ -arch=x86_64 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -arch=arm -norelay __CxxFrameHandler3(ptr ptr ptr ptr)
@ cdecl -version=0x600+ -arch=i386 -norelay __CxxFrameHandler3(ptr ptr ptr ptr)
@ cdecl -version=0x600+ -arch=x86_64 -norelay __CxxFrameHandler3(ptr ptr ptr ptr) __CxxFrameHandler
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr)
@ cdecl -arch=i386 __CxxQueryExceptionSize()
@ cdecl -arch=i386 __CxxRegisterExceptionObject()
@ cdecl -arch=i386 __CxxUnregisterExceptionObject()
@ cdecl __DestructExceptionObject(ptr)
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ cdecl __STRINGTOLD(ptr ptr str long)
@ cdecl ___lc_codepage_func()
@ cdecl ___lc_collate_cp_func()
@ cdecl ___lc_handle_func()
@ cdecl ___mb_cur_max_func()
@ cdecl -arch=i386,x86_64 ___setlc_active_func()
@ cdecl -arch=i386,x86_64 ___unguarded_readlc_active_add_func()
@ extern __argc
@ extern __argv
@ extern __badioinfo __badioinfo
@ cdecl __crtCompareStringA(long long str long str long) kernel32.CompareStringA
@ cdecl __crtCompareStringW(long long wstr long wstr long) kernel32.CompareStringW
@ cdecl __crtGetLocaleInfoW(long long ptr long) kernel32.GetLocaleInfoW
@ cdecl __crtGetStringTypeW(long long wstr long ptr)
@ cdecl __crtLCMapStringA(long long str long str long long long)
@ cdecl __crtLCMapStringW(long long wstr long wstr long long long)
@ stub -version=0x600+ __daylight
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ stub -version=0x600+ __dstbias
@ cdecl __fpecode()
@ cdecl __getmainargs(ptr ptr ptr long ptr)
@ extern -arch=i386,x86_64 __initenv
@ cdecl __iob_func()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ extern -arch=i386,x86_64 __lc_codepage
@ extern -arch=i386,x86_64 __lc_collate_cp MSVCRT___lc_collate_cp
@ extern __lc_handle MSVCRT___lc_handle
@ cdecl __lconv_init()
@ stub -version=0x600+ -arch=i386 __libm_sse2_acos
@ stub -version=0x600+ -arch=i386 __libm_sse2_acosf
@ stub -version=0x600+ -arch=i386 __libm_sse2_asin
@ stub -version=0x600+ -arch=i386 __libm_sse2_asinf
@ stub -version=0x600+ -arch=i386 __libm_sse2_atan
@ stub -version=0x600+ -arch=i386 __libm_sse2_atan2
@ stub -version=0x600+ -arch=i386 __libm_sse2_atanf
@ stub -version=0x600+ -arch=i386 __libm_sse2_cos
@ stub -version=0x600+ -arch=i386 __libm_sse2_cosf
@ stub -version=0x600+ -arch=i386 __libm_sse2_exp
@ stub -version=0x600+ -arch=i386 __libm_sse2_expf
@ stub -version=0x600+ -arch=i386 __libm_sse2_log
@ stub -version=0x600+ -arch=i386 __libm_sse2_log10
@ stub -version=0x600+ -arch=i386 __libm_sse2_log10f
@ stub -version=0x600+ -arch=i386 __libm_sse2_logf
@ stub -version=0x600+ -arch=i386 __libm_sse2_pow
@ stub -version=0x600+ -arch=i386 __libm_sse2_powf
@ stub -version=0x600+ -arch=i386 __libm_sse2_sin
@ stub -version=0x600+ -arch=i386 __libm_sse2_sinf
@ stub -version=0x600+ -arch=i386 __libm_sse2_tan
@ stub -version=0x600+ -arch=i386 __libm_sse2_tanf
@ extern __mb_cur_max
@ cdecl -arch=i386 __p___argc()
@ cdecl -arch=i386 __p___argv()
@ cdecl -arch=i386 __p___initenv()
@ cdecl -arch=i386 __p___mb_cur_max()
@ cdecl -arch=i386 __p___wargv()
@ cdecl -arch=i386 __p___winitenv()
@ cdecl -arch=i386 __p__acmdln()
@ cdecl -arch=i386 __p__amblksiz()
@ cdecl -arch=i386 __p__commode()
@ cdecl -arch=i386 __p__daylight()
@ cdecl -arch=i386 __p__dstbias()
@ cdecl -arch=i386 __p__environ()
@ cdecl -arch=i386 __p__fileinfo()
@ cdecl -arch=i386 __p__fmode()
@ cdecl -arch=i386 __p__iob() __iob_func
@ cdecl -arch=i386 __p__mbcasemap()
@ cdecl -arch=i386 __p__mbctype()
@ cdecl -arch=i386 __p__osver()
@ cdecl -arch=i386 __p__pctype()
@ cdecl -arch=i386 __p__pgmptr()
@ cdecl -arch=i386 __p__pwctype()
@ cdecl -arch=i386 __p__timezone()
@ cdecl -arch=i386 __p__tzname()
@ cdecl -arch=i386 __p__wcmdln()
@ cdecl -arch=i386 __p__wenviron()
@ cdecl -arch=i386 __p__winmajor()
@ cdecl -arch=i386 __p__winminor()
@ cdecl -arch=i386 __p__winver()
@ cdecl -arch=i386 __p__wpgmptr()
@ cdecl __pctype_func()
@ extern __pioinfo
@ cdecl __pwctype_func()
@ cdecl __pxcptinfoptrs()
@ cdecl __set_app_type(long)
@ extern -arch=i386,x86_64 __setlc_active
@ cdecl __setusermatherr(ptr)
@ stub -version=0x600+ __strncnt
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long)
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ extern -arch=i386,x86_64 __unguarded_readlc_active
@ extern __wargv __wargv
@ cdecl __wcserror(wstr)
@ cdecl -version=0x600+ __wcserror_s(ptr long wstr)
@ stub -version=0x600+ __wcsncnt
@ cdecl __wgetmainargs(ptr ptr ptr long ptr)
@ extern -arch=i386,x86_64 __winitenv
@ cdecl -arch=i386 _abnormal_termination()
# stub _abs64
@ cdecl _access(str long)
@ cdecl -version=0x600+ _access_s(str long)
@ extern _acmdln
@ stdcall -arch=i386 _adj_fdiv_m16i(long)
@ stdcall -arch=i386 _adj_fdiv_m32(long)
@ stdcall -arch=i386 _adj_fdiv_m32i(long)
@ stdcall -arch=i386 _adj_fdiv_m64(double)
@ cdecl -arch=i386 _adj_fdiv_r()
@ stdcall -arch=i386 _adj_fdivr_m16i(long)
@ stdcall -arch=i386 _adj_fdivr_m32(long)
@ stdcall -arch=i386 _adj_fdivr_m32i(long)
@ stdcall -arch=i386 _adj_fdivr_m64(double)
@ cdecl -arch=i386 _adj_fpatan()
@ cdecl -arch=i386 _adj_fprem()
@ cdecl -arch=i386 _adj_fprem1()
@ cdecl -arch=i386 _adj_fptan()
@ extern -arch=i386 _adjust_fdiv
@ extern _aexit_rtn
@ cdecl _aligned_free(ptr)
@ stub -version=0x600+ _aligned_free_dbg
@ cdecl _aligned_malloc(long long)
@ stub -version=0x600+ _aligned_malloc_dbg
@ cdecl _aligned_offset_malloc(long long long)
@ stub -version=0x600+ _aligned_offset_malloc_dbg
@ cdecl _aligned_offset_realloc(ptr long long long)
@ stub -version=0x600+ _aligned_offset_realloc_dbg
@ cdecl _aligned_realloc(ptr long long)
@ stub -version=0x600+ _aligned_realloc_dbg
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long)
@ cdecl _atodbl(ptr str)
@ stub -version=0x600+ _atodbl_l
@ stub -version=0x600+ _atof_l
@ stub -version=0x600+ _atoflt_l
@ cdecl -ret64 _atoi64(str)
@ stub -version=0x600+ _atoi64_l
@ stub -version=0x600+ _atoi_l
@ stub -version=0x600+ _atol_l
@ cdecl _atoldbl(ptr str)
@ stub -version=0x600+ _atoldbl_l
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _beginthreadex(ptr long ptr ptr long ptr)
@ cdecl _c_exit()
@ cdecl _cabs(long)
@ cdecl _callnewh(long)
@ stub -version=0x600+ _calloc_dbg
@ cdecl _cexit()
@ cdecl _cgets(str)
@ stub -version=0x600+ _cgets_s
@ cdecl -stub _cgetws(wstr)
@ stub -version=0x600+ _cgetws_s
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign(double)
@ cdecl -arch=x86_64,arm _chgsignf(long)
@ cdecl -arch=i386 -norelay _chkesp()
@ cdecl _chmod(str long)
@ cdecl _chsize(long long)
@ cdecl -version=0x600+ _chsize_s(long int64)
@ stub -version=0x600+ _chvalidator
@ stub -version=0x600+ _chvalidator_l
@ cdecl -arch=i386,x86_64 _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ extern _commode
@ cdecl -arch=i386,x86_64 _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl -version=0x600+ _controlfp_s(ptr long long)
@ cdecl _copysign( double double )
@ cdecl -arch=x86_64,arm _copysignf(long long)
@ varargs _cprintf(str)
@ stub -version=0x600+ _cprintf_l
@ stub -version=0x600+ _cprintf_p
@ stub -version=0x600+ _cprintf_p_l
@ stub -version=0x600+ _cprintf_s
@ stub -version=0x600+ _cprintf_s_l
@ cdecl _cputs(str)
@ cdecl -stub _cputws(wstr)
@ cdecl _creat(str long)
@ stub -version=0x600+ _crtAssertBusy
@ stub -version=0x600+ _crtBreakAlloc
@ stub -version=0x600+ _crtDbgFlag
@ varargs _cscanf(str)
@ stub -version=0x600+ _cscanf_l
@ stub -version=0x600+ _cscanf_s
@ stub -version=0x600+ _cscanf_s_l
@ stub -version=0x600+ _ctime32
@ stub -version=0x600+ _ctime32_s
@ cdecl _ctime64(ptr)
@ cdecl -version=0x600+ _ctime64_s(ptr long ptr)
@ extern _ctype
@ cdecl _cwait(ptr long long)
@ varargs _cwprintf(wstr)
@ stub -version=0x600+ _cwprintf_l
@ stub -version=0x600+ _cwprintf_p
@ stub -version=0x600+ _cwprintf_p_l
@ stub -version=0x600+ _cwprintf_s
@ stub -version=0x600+ _cwprintf_s_l
@ varargs -stub _cwscanf(wstr)
@ stub -version=0x600+ _cwscanf_l
@ stub -version=0x600+ _cwscanf_s
@ stub -version=0x600+ _cwscanf_s_l
@ extern _daylight
@ stub -version=0x600+ _difftime32
@ stub -version=0x600+ _difftime64
@ extern -arch=i386,x86_64 _dstbias
@ cdecl _dup(long)
@ cdecl _dup2(long long)
@ cdecl _ecvt(double long ptr ptr)
@ stub -version=0x600+ _ecvt_s
@ cdecl _endthread()
@ cdecl _endthreadex(long)
@ extern -arch=i386,x86_64 _environ
@ cdecl _eof(long)
@ cdecl _errno()
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl -arch=i386 -version=0x600+ _except_handler4_common(ptr ptr ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr)
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
@ stub -version=0x600+ _expand_dbg
@ cdecl _fcloseall()
@ cdecl _fcvt(double long ptr ptr)
@ stub -version=0x600+ _fcvt_s
@ cdecl _fdopen(long str)
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr)
@ extern -arch=i386,x86_64 _fileinfo
@ cdecl _filelength(long)
@ cdecl -ret64 _filelengthi64(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ cdecl _findfirst64(str ptr)
@ cdecl _findfirsti64(str ptr)
@ cdecl _findnext(long ptr)
@ cdecl _findnext64(long ptr)
@ cdecl _findnexti64(long ptr)
@ cdecl _finite(double)
@ stub -arch=x86_64 _finitef
@ cdecl _flsbuf(long ptr)
@ cdecl _flushall()
@ extern _fmode
@ cdecl _fpclass(double)
@ cdecl -stub -arch=x86_64 _fpclassf(long)
@ cdecl -arch=i386 _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ stub -version=0x600+ _fprintf_l
@ stub -version=0x600+ _fprintf_p
@ stub -version=0x600+ _fprintf_p_l
@ stub -version=0x600+ _fprintf_s_l
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
@ stub -version=0x600+ _free_dbg
@ stub -version=0x600+ _freea
@ stub -version=0x600+ _freea_s
@ stub -version=0x600+ _fscanf_l
@ stub -version=0x600+ _fscanf_s_l
@ cdecl -version=0x600+ _fseeki64(ptr int64 long)
@ cdecl _fsopen(str str long)
@ cdecl _fstat(long ptr)
@ cdecl _fstat64(long ptr)
@ cdecl _fstati64(long ptr)
@ cdecl _ftime(ptr)
@ stub -version=0x600+ _ftime32
@ stub -version=0x600+ _ftime32_s
@ cdecl _ftime64(ptr)
@ cdecl -version=0x600+ _ftime64_s(ptr)
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl -version=0x600+ -arch=i386 _ftol2(long)
@ cdecl -version=0x600+ -arch=i386 _ftol2_sse(long)
@ stub -version=0x600+ -arch=i386 _ftol2_sse_excpt
@ cdecl _fullpath(ptr str long)
@ stub -version=0x600+ _fullpath_dbg
@ cdecl _futime(long ptr)
@ stub -version=0x600+ _futime32
@ cdecl _futime64(long ptr)
@ stub -version=0x600+ _fwprintf_l
@ stub -version=0x600+ _fwprintf_p
@ stub -version=0x600+ _fwprintf_p_l
@ stub -version=0x600+ _fwprintf_s_l
@ stub -version=0x600+ _fwscanf_l
@ stub -version=0x600+ _fwscanf_s_l
@ cdecl _gcvt(double long str)
@ cdecl -version=0x600+ _gcvt_s(ptr ptr double long)
@ cdecl -version=0x600+ _get_doserrno(ptr)
@ stub -version=0x600+ _get_environ
@ cdecl -version=0x600+ _get_errno(ptr)
@ stub -version=0x600+ _get_fileinfo
@ stub -version=0x600+ _get_fmode
# @ cdecl _get_heap_handle()
@ cdecl _get_osfhandle(long)
@ cdecl -version=0x600+ _get_osplatform(ptr)
@ stub -version=0x600+ _get_osver
@ cdecl -version=0x600+ _get_output_format()
@ cdecl -version=0x600+ _get_pgmptr(ptr)
@ cdecl _get_sbh_threshold()
@ stub -version=0x600+ _get_wenviron
@ stub -version=0x600+ _get_winmajor
@ stub -version=0x600+ _get_winminor
@ stub -version=0x600+ _get_winver
@ cdecl -version=0x600+ _get_wpgmptr(ptr)
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl -arch=i386 _getdllprocaddr(long str long)
@ cdecl -arch=x86_64 -version=0x502 _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getmaxstdio()
@ cdecl _getmbcp()
@ cdecl _getpid() kernel32.GetCurrentProcessId
@ cdecl _getsystime(ptr)
@ cdecl _getw(ptr)
@ cdecl -stub _getwch()
@ cdecl -stub _getwche()
@ cdecl _getws(ptr)
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl -version=0x600+ _gmtime32(ptr)
@ cdecl -version=0x600+ _gmtime32_s(ptr ptr)
@ cdecl _gmtime64(ptr)
@ cdecl -version=0x600+ _gmtime64_s(ptr ptr)
@ cdecl _heapadd(ptr long)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ cdecl -arch=i386 _heapused(ptr ptr)
@ cdecl -arch=x86_64 -version=0x502 _heapused(ptr ptr)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl -arch=x86_64,arm _hypotf(long long)
@ cdecl _i64toa(long long ptr long)
@ cdecl -version=0x600+ _i64toa_s(int64 ptr long long)
@ cdecl _i64tow(long long ptr long)
@ cdecl -version=0x600+ _i64tow_s(int64 ptr long long)
@ cdecl _initterm(ptr ptr)
@ cdecl -version=0x600+ _initterm_e(ptr ptr)
@ cdecl -arch=i386 _inp(long) MSVCRT__inp
@ cdecl -arch=i386 _inpd(long) MSVCRT__inpd
@ cdecl -arch=i386 _inpw(long) MSVCRT__inpw
@ cdecl -version=0x600+ _invalid_parameter(wstr wstr wstr long long)
@ extern _iob
@ cdecl -version=0x600+ _isalnum_l(long ptr)
@ cdecl -version=0x600+ _isalpha_l(long ptr)
@ cdecl _isatty(long)
@ cdecl -version=0x600+ _iscntrl_l(long ptr)
@ cdecl _isctype(long long)
@ cdecl -version=0x600+ _isctype_l(long long ptr)
@ cdecl -version=0x600+ _isdigit_l(long ptr)
@ cdecl -version=0x600+ _isgraph_l(long ptr)
@ cdecl -version=0x600+ _isleadbyte_l(long ptr)
@ cdecl -version=0x600+ _islower_l(long ptr)
@ cdecl _ismbbalnum(long)
@ stub -version=0x600+ _ismbbalnum_l
@ cdecl _ismbbalpha(long)
@ stub -version=0x600+ _ismbbalpha_l
@ cdecl _ismbbgraph(long)
@ stub -version=0x600+ _ismbbgraph_l
@ cdecl _ismbbkalnum(long)
@ stub -version=0x600+ _ismbbkalnum_l
@ cdecl _ismbbkana(long)
@ stub -version=0x600+ _ismbbkana_l
@ cdecl _ismbbkprint(long)
@ stub -version=0x600+ _ismbbkprint_l
@ cdecl _ismbbkpunct(long)
@ stub -version=0x600+ _ismbbkpunct_l
@ cdecl _ismbblead(long)
@ stub -version=0x600+ _ismbblead_l
@ cdecl _ismbbprint(long)
@ stub -version=0x600+ _ismbbprint_l
@ cdecl _ismbbpunct(long)
@ stub -version=0x600+ _ismbbpunct_l
@ cdecl _ismbbtrail(long)
@ stub -version=0x600+ _ismbbtrail_l
@ cdecl _ismbcalnum(long)
@ stub -version=0x600+ _ismbcalnum_l
@ cdecl _ismbcalpha(long)
@ stub -version=0x600+ _ismbcalpha_l
@ cdecl _ismbcdigit(long)
@ stub -version=0x600+ _ismbcdigit_l
@ cdecl _ismbcgraph(long)
@ stub -version=0x600+ _ismbcgraph_l
@ cdecl _ismbchira(long)
@ stub -version=0x600+ _ismbchira_l
@ cdecl _ismbckata(long)
@ stub -version=0x600+ _ismbckata_l
@ cdecl _ismbcl0(long)
@ stub -version=0x600+ _ismbcl0_l
@ cdecl _ismbcl1(long)
@ stub -version=0x600+ _ismbcl1_l
@ cdecl _ismbcl2(long)
@ stub -version=0x600+ _ismbcl2_l
@ cdecl _ismbclegal(long)
@ stub -version=0x600+ _ismbclegal_l
@ cdecl _ismbclower(long)
@ stub -version=0x600+ _ismbclower_l
@ cdecl _ismbcprint(long)
@ stub -version=0x600+ _ismbcprint_l
@ cdecl _ismbcpunct(long)
@ stub -version=0x600+ _ismbcpunct_l
@ cdecl _ismbcspace(long)
@ stub -version=0x600+ _ismbcspace_l
@ cdecl _ismbcsymbol(long)
@ stub -version=0x600+ _ismbcsymbol_l
@ cdecl _ismbcupper(long)
@ stub -version=0x600+ _ismbcupper_l
@ cdecl _ismbslead(ptr ptr)
@ stub -version=0x600+ _ismbslead_l
@ cdecl _ismbstrail(ptr ptr)
@ stub -version=0x600+ _ismbstrail_l
@ cdecl _isnan(double)
@ stub -arch=x86_64 _isnanf
@ cdecl -version=0x600+ _isprint_l(long ptr)
@ cdecl -version=0x600+ _isspace_l(long ptr)
@ cdecl -version=0x600+ _isupper_l(long ptr)
@ cdecl -version=0x600+ _iswalnum_l(long ptr)
@ cdecl -version=0x600+ _iswalpha_l(long ptr)
@ cdecl -version=0x600+ _iswcntrl_l(long ptr)
@ cdecl -version=0x600+ _iswctype_l(long ptr)
@ cdecl -version=0x600+ _iswdigit_l(long ptr)
@ cdecl -version=0x600+ _iswgraph_l(long ptr)
@ cdecl -version=0x600+ _iswlower_l(long ptr)
@ cdecl -version=0x600+ _iswprint_l(long ptr)
@ cdecl -version=0x600+ _iswpunct_l(long ptr)
@ cdecl -version=0x600+ _iswspace_l(long ptr)
@ cdecl -version=0x600+ _iswupper_l(long ptr)
@ cdecl -version=0x600+ _iswxdigit_l(long ptr)
@ cdecl -version=0x600+ _isxdigit_l(long ptr)
@ cdecl _itoa(long ptr long)
@ cdecl -version=0x600+ _itoa_s(long str long long)
@ cdecl _itow(long ptr long)
@ cdecl -version=0x600+ _itow_s() # FIXME
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ stub -version=0x600+ _lfind_s
@ cdecl -arch=i386 _loaddll(str)
@ cdecl -arch=x86_64 -version=0x502 _loaddll(str)
@ cdecl -arch=x86_64 _local_unwind(ptr ptr)
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl -arch=i386 -version=0x600+ _local_unwind4(ptr ptr long)
@ cdecl -version=0x600+ _localtime32(ptr)
@ cdecl -version=0x600+ _localtime32_s(ptr ptr)
@ cdecl _localtime64(ptr)
@ cdecl -version=0x600+ _localtime64_s(ptr ptr)
@ cdecl _lock(long)
@ cdecl _locking(long long long)
@ cdecl _logb(double)
@ cdecl -arch=i386 _longjmpex(ptr long) longjmp
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr long long ptr)
@ stub -version=0x600+ _lsearch_s
@ cdecl _lseek(long long long)
@ cdecl -ret64 _lseeki64(long double long)
@ cdecl _ltoa(long ptr long)
@ cdecl -version=0x600+ _ltoa_s(long str long long)
@ cdecl _ltow(long ptr long)
@ cdecl -version=0x600+ _ltow_s(long ptr long long)
@ cdecl _makepath(ptr str str str str)
@ stub -version=0x600+ _makepath_s
@ stub -version=0x600+ _malloc_dbg
@ cdecl _mbbtombc(long)
@ stub -version=0x600+ _mbbtombc_l
@ cdecl _mbbtype(long long)
@ extern _mbcasemap
@ cdecl _mbccpy (str str)
@ stub -version=0x600+ _mbccpy_l
@ stub -version=0x600+ _mbccpy_s
@ stub -version=0x600+ _mbccpy_s_l
@ cdecl _mbcjistojms(long)
@ stub -version=0x600+ _mbcjistojms_l
@ cdecl _mbcjmstojis(long)
@ stub -version=0x600+ _mbcjmstojis_l
@ cdecl _mbclen(ptr)
@ stub -version=0x600+ _mbclen_l
@ cdecl _mbctohira(long)
@ stub -version=0x600+ _mbctohira_l
@ cdecl _mbctokata(long)
@ stub -version=0x600+ _mbctokata_l
@ cdecl _mbctolower(long)
@ stub -version=0x600+ _mbctolower_l
@ cdecl _mbctombb(long)
@ stub -version=0x600+ _mbctombb_l
@ cdecl _mbctoupper(long)
@ stub -version=0x600+ _mbctoupper_l
@ extern _mbctype
@ stub -version=0x600+ _mblen_l
@ cdecl _mbsbtype(str long)
@ stub -version=0x600+ _mbsbtype_l
@ cdecl _mbscat(str str)
@ stub -version=0x600+ _mbscat_s
@ stub -version=0x600+ _mbscat_s_l
@ cdecl _mbschr(str long)
@ stub -version=0x600+ _mbschr_l
@ cdecl _mbscmp(str str)
@ stub -version=0x600+ _mbscmp_l
@ cdecl _mbscoll(str str)
@ stub -version=0x600+ _mbscoll_l
@ cdecl _mbscpy(ptr str)
@ stub -version=0x600+ _mbscpy_s
@ stub -version=0x600+ _mbscpy_s_l
@ cdecl _mbscspn(str str)
@ stub -version=0x600+ _mbscspn_l
@ cdecl _mbsdec(ptr ptr)
@ stub -version=0x600+ _mbsdec_l
@ cdecl _mbsdup(str)
@ cdecl _mbsicmp(str str)
@ stub -version=0x600+ _mbsicmp_l
@ cdecl _mbsicoll(str str)
@ stub -version=0x600+ _mbsicoll_l
@ cdecl _mbsinc(str)
@ stub -version=0x600+ _mbsinc_l
@ cdecl _mbslen(str)
@ stub -version=0x600+ _mbslen_l
@ cdecl _mbslwr(str)
@ stub -version=0x600+ _mbslwr_l
@ stub -version=0x600+ _mbslwr_s
@ stub -version=0x600+ _mbslwr_s_l
@ cdecl _mbsnbcat(str str long)
@ stub -version=0x600+ _mbsnbcat_l
@ stub -version=0x600+ _mbsnbcat_s
@ stub -version=0x600+ _mbsnbcat_s_l
@ cdecl _mbsnbcmp(str str long)
@ stub -version=0x600+ _mbsnbcmp_l
@ cdecl _mbsnbcnt(ptr long)
@ stub -version=0x600+ _mbsnbcnt_l
@ cdecl _mbsnbcoll(str str long)
@ stub -version=0x600+ _mbsnbcoll_l
@ cdecl _mbsnbcpy(ptr str long)
@ stub -version=0x600+ _mbsnbcpy_l
@ cdecl -version=0x600+ _mbsnbcpy_s(ptr long str long)
@ stub -version=0x600+ _mbsnbcpy_s_l
@ cdecl _mbsnbicmp(str str long)
@ stub -version=0x600+ _mbsnbicmp_l
@ cdecl _mbsnbicoll(str str long)
@ stub -version=0x600+ _mbsnbicoll_l
@ cdecl _mbsnbset(str long long)
@ stub -version=0x600+ _mbsnbset_l
@ stub -version=0x600+ _mbsnbset_s
@ stub -version=0x600+ _mbsnbset_s_l
@ cdecl _mbsncat(str str long)
@ stub -version=0x600+ _mbsncat_l
@ stub -version=0x600+ _mbsncat_s
@ stub -version=0x600+ _mbsncat_s_l
@ cdecl _mbsnccnt(str long)
@ stub -version=0x600+ _mbsnccnt_l
@ cdecl _mbsncmp(str str long)
@ stub -version=0x600+ _mbsncmp_l
@ cdecl _mbsncoll(str str long)
@ stub -version=0x600+ _mbsncoll_l
@ cdecl _mbsncpy(str str long)
@ stub -version=0x600+ _mbsncpy_l
@ stub -version=0x600+ _mbsncpy_s
@ stub -version=0x600+ _mbsncpy_s_l
@ cdecl _mbsnextc(str)
@ stub -version=0x600+ _mbsnextc_l
@ cdecl _mbsnicmp(str str long)
@ stub -version=0x600+ _mbsnicmp_l
@ cdecl _mbsnicoll(str str long)
@ stub -version=0x600+ _mbsnicoll_l
@ cdecl _mbsninc(str long)
@ stub -version=0x600+ _mbsninc_l
@ stub -version=0x600+ _mbsnlen
@ stub -version=0x600+ _mbsnlen_l
@ cdecl _mbsnset(str long long)
@ stub -version=0x600+ _mbsnset_l
@ stub -version=0x600+ _mbsnset_s
@ stub -version=0x600+ _mbsnset_s_l
@ cdecl _mbspbrk(str str)
@ stub -version=0x600+ _mbspbrk_l
@ cdecl _mbsrchr(str long)
@ stub -version=0x600+ _mbsrchr_l
@ cdecl _mbsrev(str)
@ stub -version=0x600+ _mbsrev_l
@ cdecl _mbsset(str long)
@ stub -version=0x600+ _mbsset_l
@ stub -version=0x600+ _mbsset_s
@ stub -version=0x600+ _mbsset_s_l
@ cdecl _mbsspn(str str)
@ stub -version=0x600+ _mbsspn_l
@ cdecl _mbsspnp(str str)
@ stub -version=0x600+ _mbsspnp_l
@ cdecl _mbsstr(str str)
@ stub -version=0x600+ _mbsstr_l
@ cdecl _mbstok(str str)
@ stub -version=0x600+ _mbstok_l
@ stub -version=0x600+ _mbstok_s
@ stub -version=0x600+ _mbstok_s_l
@ cdecl -version=0x600+ _mbstowcs_l(ptr str long ptr)
@ stub -version=0x600+ _mbstowcs_s_l
@ cdecl _mbstrlen(str)
@ stub -version=0x600+ _mbstrlen_l
@ stub -version=0x600+ _mbstrnlen
@ stub -version=0x600+ _mbstrnlen_l
@ cdecl _mbsupr(str)
@ stub -version=0x600+ _mbsupr_l
@ stub -version=0x600+ _mbsupr_s
@ stub -version=0x600+ _mbsupr_s_l
@ cdecl -version=0x600+ _mbtowc_l(ptr wstr long long)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl -version=0x600+ _memicmp_l(ptr ptr long ptr)
@ cdecl _mkdir(str)
@ cdecl _mkgmtime(ptr)
@ cdecl -version=0x600+ _mkgmtime32(ptr)
@ cdecl _mkgmtime64(ptr)
@ cdecl _mktemp(str)
@ stub -version=0x600+ _mktemp_s
@ cdecl -version=0x600+ _mktime32(ptr)
@ cdecl _mktime64(ptr)
@ cdecl _msize(ptr)
@ stub -version=0x600+ -arch=i386 _msize_debug
@ cdecl _nextafter(double double)
@ stub -arch=x86_64 _nextafterf
@ extern _onexit # Declaring it as extern let us use the symbol from msvcrtex while having the __imp_ symbol defined in the import lib
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ extern -arch=i386,x86_64 _osplatform
@ extern _osver
@ cdecl -arch=i386 _outp(long long) MSVCRT__outp
@ cdecl -arch=i386 _outpd(long long) MSVCRT__outpd
@ cdecl -arch=i386 _outpw(long long) MSVCRT__outpw
@ cdecl _pclose(ptr)
@ extern _pctype
@ extern _pgmptr
@ cdecl _pipe(ptr long long)
@ cdecl _popen(str str)
@ stub -version=0x600+ _printf_l
@ stub -version=0x600+ _printf_p
@ stub -version=0x600+ _printf_p_l
@ stub -version=0x600+ _printf_s_l
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
@ stub -version=0x600+ _putenv_s
@ cdecl _putw(long ptr)
@ cdecl _putwch(long)
@ cdecl _putws(wstr)
@ extern _pwctype
@ cdecl _read(long ptr long)
@ stub -version=0x600+ _realloc_dbg
@ cdecl _resetstkoflw()
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
# stub _rotl64
@ cdecl _rotr(long long)
# stub _rotr64
@ cdecl -arch=i386 _safe_fdiv()
@ cdecl -arch=i386 _safe_fdivr()
@ cdecl -arch=i386 _safe_fprem()
@ cdecl -arch=i386 _safe_fprem1()
@ cdecl _scalb(double long)
@ stub -arch=x86_64 _scalbf
@ stub -version=0x600+ _scanf_l
@ stub -version=0x600+ _scanf_s_l
@ varargs _scprintf(str)
@ stub -version=0x600+ _scprintf_l
@ stub -version=0x600+ _scprintf_p_l
@ varargs _scwprintf(wstr)
@ stub -version=0x600+ _scwprintf_l
@ stub -version=0x600+ _scwprintf_p_l
@ cdecl _searchenv(str str ptr)
@ cdecl -version=0x600+ _searchenv_s(str str ptr long)
@ stdcall -version=0x600+ -arch=i386 _seh_longjmp_unwind4(ptr)
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr)
@ stub -arch=i386 _set_SSE2_enable
@ stub -version=0x600+ _set_controlfp
@ cdecl -version=0x600+ _set_doserrno(long)
@ cdecl -version=0x600+ _set_errno(long)
@ cdecl _set_error_mode(long)
@ stub -version=0x600+ _set_fileinfo
@ stub -version=0x600+ _set_fmode
@ stub -version=0x600+ _set_output_format
@ cdecl _set_sbh_threshold(long)
@ cdecl _seterrormode(long)
@ cdecl -norelay _setjmp(ptr)
@ cdecl -arch=i386 -norelay _setjmp3(ptr long)
@ cdecl -arch=x86_64,arm -norelay _setjmpex(ptr ptr)
@ cdecl _setmaxstdio(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ cdecl _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(ptr long str)
@ stub -version=0x600+ _snprintf_c
@ stub -version=0x600+ _snprintf_c_l
@ stub -version=0x600+ _snprintf_l
@ stub -version=0x600+ _snprintf_s
@ stub -version=0x600+ _snprintf_s_l
@ varargs _snscanf(str long str)
@ stub -version=0x600+ _snscanf_l
@ stub -version=0x600+ _snscanf_s
@ stub -version=0x600+ _snscanf_s_l
@ varargs _snwprintf(ptr long wstr)
@ stub -version=0x600+ _snwprintf_l
@ stub -version=0x600+ _snwprintf_s
@ stub -version=0x600+ _snwprintf_s_l
@ varargs _snwscanf(wstr long wstr)
@ stub -version=0x600+ _snwscanf_l
@ stub -version=0x600+ _snwscanf_s
@ stub -version=0x600+ _snwscanf_s_l
@ varargs _sopen(str long long)
@ cdecl -version=0x600+ _sopen_s(ptr str long long long)
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ stub -version=0x600+ _splitpath_s
@ stub -version=0x600+ _sprintf_l
@ stub -version=0x600+ _sprintf_p_l
@ stub -version=0x600+ _sprintf_s_l
@ stub -version=0x600+ _sscanf_l
@ stub -version=0x600+ _sscanf_s_l
@ cdecl _stat(str ptr)
@ cdecl _stat64(str ptr)
@ cdecl _stati64(str ptr)
@ cdecl _statusfp()
@ cdecl _strcmpi(str str)
@ cdecl -version=0x600+ _strcoll_l(str str ptr)
@ cdecl _strdate(ptr)
@ cdecl -version=0x600+ _strdate_s(ptr long)
@ cdecl _strdup(str)
@ stub -version=0x600+ _strdup_dbg
@ cdecl _strerror(long)
@ cdecl -version=0x600+ _strerror_s(ptr long str)
@ cdecl _stricmp(str str)
@ cdecl -version=0x600+ _stricmp_l(str str ptr)
@ cdecl _stricoll(str str)
@ cdecl -version=0x600+ _stricoll_l(str str ptr)
@ cdecl _strlwr(str)
@ stub -version=0x600+ _strlwr_l
@ cdecl -version=0x600+ _strlwr_s(str long)
@ cdecl -version=0x600+ _strlwr_s_l(str long ptr)
@ cdecl _strncoll(str str long)
@ cdecl -version=0x600+ _strncoll_l(str str long ptr)
@ cdecl _strnicmp(str str long)
@ cdecl -version=0x600+ _strnicmp_l(str str long ptr)
@ cdecl _strnicoll(str str long)
@ cdecl -version=0x600+ _strnicoll_l(str str long ptr)
@ cdecl _strnset(str long long)
@ cdecl -version=0x600+ _strnset_s(str long long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl -version=0x600+ _strset_s(str long long)
@ cdecl _strtime(ptr)
@ cdecl -version=0x600+ _strtime_s(ptr long)
@ stub -version=0x600+ _strtod_l
@ cdecl _strtoi64(str ptr long)
@ cdecl -version=0x600+ _strtoi64_l(str ptr long ptr)
@ stub -version=0x600+ _strtol_l
@ cdecl _strtoui64(str ptr long) strtoull
@ stub -version=0x600+ _strtoui64_l
@ cdecl -version=0x600+ _strtoul_l(str ptr long ptr)
@ cdecl _strupr(str)
@ cdecl -version=0x600+ _strupr_l(str ptr)
@ cdecl -version=0x600+ _strupr_s(str long)
@ cdecl -version=0x600+ _strupr_s_l(str long ptr)
@ cdecl -version=0x600+ _strxfrm_l(ptr str long ptr)
@ cdecl _swab(str str long)
@ cdecl -version=0x400-0x502 -impsym _swprintf(ptr str) swprintf # Compatibility for pre NT6
@ cdecl -version=0x600+ _swprintf(ptr str)
@ stub -version=0x600+ _swprintf_c
@ stub -version=0x600+ _swprintf_c_l
@ stub -version=0x600+ _swprintf_p_l
@ stub -version=0x600+ _swprintf_s_l
@ stub -version=0x600+ _swscanf_l
@ stub -version=0x600+ _swscanf_s_l
@ extern _sys_errlist
@ extern _sys_nerr
@ cdecl _tell(long)
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str)
@ stub -version=0x600+ _tempnam_dbg
@ stub -version=0x600+ _time32
@ cdecl _time64(ptr)
@ extern _timezone
@ cdecl _tolower(long)
@ cdecl -version=0x600+ _tolower_l(long ptr)
@ cdecl _toupper(long)
@ cdecl -version=0x600+ _toupper_l(long ptr)
@ cdecl -version=0x600+ _towlower_l(long ptr)
@ cdecl -version=0x600+ _towupper_l(long ptr)
@ extern _tzname
@ cdecl _tzset()
@ cdecl _ui64toa(long long ptr long)
@ cdecl -version=0x600+ _ui64toa_s(int64 ptr long long)
@ cdecl _ui64tow(long long ptr long)
@ cdecl -version=0x600+ _ui64tow_s(int64 ptr long long)
@ cdecl _ultoa(long ptr long)
@ stub -version=0x600+ _ultoa_s
@ cdecl _ultow(long ptr long)
@ stub -version=0x600+ _ultow_s
@ cdecl _umask(long)
@ stub -version=0x600+ _umask_s
@ cdecl _ungetch(long)
# stub _ungetwch
@ cdecl _unlink(str)
@ cdecl -arch=i386 _unloaddll(ptr)
@ cdecl -arch=x86_64 -version=0x502 _unloaddll(ptr)
@ cdecl _unlock(long)
@ cdecl _utime(str ptr)
@ stub -version=0x600+ _utime32
@ cdecl _utime64(str ptr)
@ stub -version=0x600+ _vcprintf
@ stub -version=0x600+ _vcprintf_l
@ stub -version=0x600+ _vcprintf_p
@ stub -version=0x600+ _vcprintf_p_l
@ stub -version=0x600+ _vcprintf_s
@ stub -version=0x600+ _vcprintf_s_l
@ cdecl -version=0x600+ _vcwprintf(wstr ptr)
@ stub -version=0x600+ _vcwprintf_l
@ stub -version=0x600+ _vcwprintf_p
@ stub -version=0x600+ _vcwprintf_p_l
@ stub -version=0x600+ _vcwprintf_s
@ stub -version=0x600+ _vcwprintf_s_l
@ stub -version=0x600+ _vfprintf_l
@ stub -version=0x600+ _vfprintf_p
@ stub -version=0x600+ _vfprintf_p_l
@ stub -version=0x600+ _vfprintf_s_l
@ stub -version=0x600+ _vfwprintf_l
@ stub -version=0x600+ _vfwprintf_p
@ stub -version=0x600+ _vfwprintf_p_l
@ stub -version=0x600+ _vfwprintf_s_l
@ stub -version=0x600+ _vprintf_l
@ stub -version=0x600+ _vprintf_p
@ stub -version=0x600+ _vprintf_p_l
@ stub -version=0x600+ _vprintf_s_l
@ cdecl _vscprintf(str ptr)
@ stub -version=0x600+ _vscprintf_l
@ stub -version=0x600+ _vscprintf_p_l
@ cdecl _vscwprintf(wstr ptr)
@ stub -version=0x600+ _vscwprintf_l
@ stub -version=0x600+ _vscwprintf_p_l
@ cdecl _vsnprintf(ptr long str ptr)
@ stub -version=0x600+ _vsnprintf_c
@ stub -version=0x600+ _vsnprintf_c_l
@ stub -version=0x600+ _vsnprintf_l
@ stub -version=0x600+ _vsnprintf_s
@ stub -version=0x600+ _vsnprintf_s_l
@ cdecl _vsnwprintf(ptr long wstr ptr)
@ stub -version=0x600+ _vsnwprintf_l
@ stub -version=0x600+ _vsnwprintf_s
@ stub -version=0x600+ _vsnwprintf_s_l
@ stub -version=0x600+ _vsprintf_l
@ stub -version=0x600+ _vsprintf_p
@ stub -version=0x600+ _vsprintf_p_l
@ stub -version=0x600+ _vsprintf_s_l
@ cdecl -version=0x400-0x502 -impsym _vswprintf() vswprintf # Compatibility for pre NT6
@ stub -version=0x600+ _vswprintf
@ stub -version=0x600+ _vswprintf_c
@ stub -version=0x600+ _vswprintf_c_l
@ stub -version=0x600+ _vswprintf_l
@ stub -version=0x600+ _vswprintf_p_l
@ stub -version=0x600+ _vswprintf_s_l
@ stub -version=0x600+ _vwprintf_l
@ stub -version=0x600+ _vwprintf_p
@ stub -version=0x600+ _vwprintf_p_l
@ stub -version=0x600+ _vwprintf_s_l
@ cdecl _waccess(wstr long)
@ cdecl -version=0x600+ _waccess_s(wstr long)
@ cdecl _wasctime(ptr)
@ cdecl -version=0x600+ _wasctime_s(ptr long ptr)
@ stub -version=0x600+ _wassert
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ extern _wcmdln
@ cdecl _wcreat(wstr long)
@ cdecl -version=0x600+ _wcscoll_l(str str ptr)
@ cdecl _wcsdup(wstr)
@ stub -version=0x600+ _wcsdup_dbg
@ cdecl _wcserror(long)
@ cdecl -version=0x600+ _wcserror_s(ptr long long)
@ stub -version=0x600+ _wcsftime_l
@ cdecl _wcsicmp(wstr wstr)
@ cdecl -version=0x600+ _wcsicmp_l(wstr wstr ptr)
@ cdecl _wcsicoll(wstr wstr)
@ cdecl -version=0x600+ _wcsicoll_l(wstr wstr ptr)
@ cdecl _wcslwr(wstr)
@ cdecl -version=0x600+ _wcslwr_l(wstr ptr)
@ cdecl -version=0x600+ _wcslwr_s(wstr long)
@ cdecl -version=0x600+ _wcslwr_s_l(wstr long ptr)
@ cdecl _wcsncoll(wstr wstr long)
@ cdecl -version=0x600+ _wcsncoll_l(wstr wstr long ptr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl -version=0x600+ _wcsnicmp_l(wstr wstr long ptr)
@ cdecl _wcsnicoll(wstr wstr long)
@ cdecl -version=0x600+ _wcsnicoll_l(wstr str long ptr)
@ cdecl _wcsnset(wstr long long)
@ cdecl -version=0x600+ _wcsnset_s(wstr long long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl -version=0x600+ _wcsset_s(ptr long long)
@ cdecl _wcstoi64(wstr ptr long)
@ cdecl -version=0x600+ _wcstoi64_l(wstr ptr long ptr)
@ stub -version=0x600+ _wcstol_l
@ cdecl -version=0x600+ _wcstombs_l(ptr wstr long ptr)
@ stub -version=0x600+ _wcstombs_s_l
@ cdecl _wcstoui64(wstr ptr long)
@ cdecl -version=0x600+ _wcstoui64_l(wstr ptr long ptr)
@ stub -version=0x600+ _wcstoul_l
@ cdecl _wcsupr(wstr)
@ cdecl -version=0x600+ _wcsupr_l(wstr ptr)
@ cdecl -version=0x600+ _wcsupr_s(wstr long)
@ cdecl -version=0x600+ _wcsupr_s_l(ptr long ptr)
@ cdecl -version=0x600+ _wcsxfrm_l(ptr wstr long ptr)
@ cdecl _wctime(ptr)
@ stub -version=0x600+ _wctime32
@ stub -version=0x600+ _wctime32_s
@ cdecl _wctime64(ptr)
@ cdecl -version=0x600+ _wctime64_s(ptr long ptr)
@ stub -version=0x600+ _wctomb_l
@ stub -version=0x600+ _wctomb_s_l
@ extern _wctype
@ extern -arch=i386,x86_64 _wenviron
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr)
@ cdecl _wfindfirst(wstr ptr)
@ cdecl _wfindfirst64(wstr ptr)
@ cdecl _wfindfirsti64(wstr ptr)
@ cdecl _wfindnext(long ptr)
@ cdecl _wfindnext64(long ptr)
@ cdecl _wfindnexti64(long ptr)
@ cdecl _wfopen(wstr wstr)
@ cdecl -version=0x600+ _wfopen_s(ptr wstr wstr)
@ cdecl _wfreopen(wstr wstr ptr)
@ stub -version=0x600+ _wfreopen_s
@ cdecl _wfsopen(wstr wstr long)
@ cdecl _wfullpath(ptr wstr long)
@ stub -version=0x600+ _wfullpath_dbg
@ cdecl _wgetcwd(wstr long)
@ cdecl _wgetdcwd(long wstr long)
@ cdecl _wgetenv(wstr)
@ stub -version=0x600+ _wgetenv_s
@ extern _winmajor
@ extern _winminor
@ stub -version=0x600+ _winput_s
@ extern -arch=i386,x86_64 _winver
@ cdecl _wmakepath(ptr wstr wstr wstr wstr)
@ stub -version=0x600+ _wmakepath_s
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
@ stub -version=0x600+ _wmktemp_s
@ varargs _wopen(wstr long)
@ stub -version=0x600+ _woutput_s
@ cdecl _wperror(wstr)
@ extern _wpgmptr
@ cdecl _wpopen(wstr wstr)
@ stub -version=0x600+ _wprintf_l
@ stub -version=0x600+ _wprintf_p
@ stub -version=0x600+ _wprintf_p_l
@ stub -version=0x600+ _wprintf_s_l
@ cdecl _wputenv(wstr)
@ stub -version=0x600+ _wputenv_s
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long)
@ cdecl _wrmdir(wstr)
@ stub -version=0x600+ _wscanf_l
@ stub -version=0x600+ _wscanf_s_l
@ cdecl _wsearchenv(wstr wstr ptr)
@ cdecl -version=0x600+ _wsearchenv_s(wstr wstr ptr long)
@ cdecl _wsetlocale(long wstr)
@ varargs _wsopen(wstr long long)
@ cdecl -version=0x600+ _wsopen_s(ptr wstr long long long)
@ varargs _wspawnl(long wstr wstr)
@ varargs _wspawnle(long wstr wstr)
@ varargs _wspawnlp(long wstr wstr)
@ varargs _wspawnlpe(long wstr wstr)
@ cdecl _wspawnv(long wstr ptr)
@ cdecl _wspawnve(long wstr ptr ptr)
@ cdecl _wspawnvp(long wstr ptr)
@ cdecl _wspawnvpe(long wstr ptr ptr)
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr)
@ stub -version=0x600+ _wsplitpath_s
@ cdecl _wstat(wstr ptr)
@ cdecl _wstat64(wstr ptr)
@ cdecl _wstati64(wstr ptr)
@ cdecl _wstrdate(ptr)
@ cdecl -version=0x600+ _wstrdate_s(ptr long)
@ cdecl _wstrtime(ptr)
@ stub -version=0x600+ _wstrtime_s
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr)
@ stub -version=0x600+ _wtempnam_dbg
@ cdecl _wtmpnam(ptr)
@ stub -version=0x600+ _wtmpnam_s
@ cdecl _wtof(wstr)
@ stub -version=0x600+ _wtof_l
@ cdecl _wtoi(wstr)
@ cdecl _wtoi64(wstr)
@ cdecl -version=0x600+ _wtoi64_l(wstr ptr)
@ stub -version=0x600+ _wtoi_l
@ cdecl _wtol(wstr)
@ stub -version=0x600+ _wtol_l
@ cdecl _wunlink(wstr)
@ cdecl _wutime(wstr ptr)
@ stub -version=0x600+ _wutime32
@ cdecl _wutime64(wstr ptr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double )
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl -arch=x86_64,arm acosf(long)
@ cdecl asctime(ptr)
@ cdecl -version=0x600+ asctime_s(ptr long ptr)
@ cdecl asin(double)
@ cdecl -arch=x86_64,arm asinf(long)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl -arch=x86_64,arm atan2f(long)
@ cdecl -arch=x86_64,arm atanf(long)
@ extern atexit # Declaring it as extern let us use the symbol from msvcrtex while having the __imp_ symbol defined in the import lib for those who really need it
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ stub -version=0x600+ bsearch_s
@ stub -version=0x600+ btowc
@ cdecl calloc(long long)
@ cdecl ceil(double)
@ cdecl -arch=x86_64,arm ceilf(long)
@ cdecl clearerr(ptr)
@ stub -version=0x600+ clearerr_s
@ cdecl clock()
@ cdecl cos(double)
@ cdecl -arch=x86_64,arm cosf(long)
@ cdecl cosh(double)
@ cdecl -arch=x86_64,arm coshf(long)
@ cdecl ctime(ptr)
@ cdecl difftime(long long)
@ cdecl div(long long)
@ cdecl exit(long)
@ cdecl exp(double)
@ cdecl -arch=x86_64,arm expf(long)
@ cdecl fabs(double)
@ cdecl -arch=arm fabsf(double)
@ cdecl fclose(ptr)
@ cdecl feof(ptr)
@ cdecl ferror(ptr)
@ cdecl fflush(ptr)
@ cdecl fgetc(ptr)
@ cdecl fgetpos(ptr ptr)
@ cdecl fgets(ptr long ptr)
@ cdecl fgetwc(ptr)
@ cdecl fgetws(ptr long ptr)
@ cdecl floor(double)
@ cdecl -arch=x86_64,arm floorf(long)
@ cdecl fmod(double double)
@ cdecl -arch=x86_64,arm fmodf(long)
@ cdecl fopen(str str)
@ cdecl -version=0x600+ fopen_s(ptr str str)
@ varargs fprintf(ptr str)
@ stub -version=0x600+ fprintf_s
@ cdecl fputc(long ptr)
@ cdecl fputs(str ptr)
@ cdecl fputwc(long ptr)
@ cdecl fputws(wstr ptr)
@ cdecl fread(ptr long long ptr)
@ cdecl free(ptr)
@ cdecl freopen(str str ptr)
@ stub -version=0x600+ freopen_s
@ cdecl frexp(double ptr)
@ varargs fscanf(ptr str)
@ stub -version=0x600+ fscanf_s
@ cdecl fseek(ptr long long)
@ cdecl fsetpos(ptr ptr)
@ cdecl ftell(ptr)
@ varargs fwprintf(ptr wstr)
@ stub -version=0x600+ fwprintf_s
@ cdecl fwrite(ptr long long ptr)
@ varargs fwscanf(ptr wstr)
@ stub -version=0x600+ fwscanf_s
@ cdecl getc(ptr)
@ cdecl getchar()
@ cdecl getenv(str)
@ stub -version=0x600+ getenv_s
@ cdecl gets(str)
@ cdecl getwc(ptr)
@ cdecl getwchar()
@ cdecl gmtime(ptr)
@ cdecl is_wctype(long long)
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl isleadbyte(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long)
@ cdecl iswalpha(long)
@ cdecl iswascii(long)
@ cdecl iswcntrl(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswgraph(long)
@ cdecl iswlower(long)
@ cdecl iswprint(long)
@ cdecl iswpunct(long)
@ cdecl iswspace(long)
@ cdecl iswupper(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl ldexp(double long)
@ cdecl ldiv(long long)
@ cdecl localeconv()
@ cdecl localtime(ptr)
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl -arch=x86_64,arm log10f(long)
@ cdecl -arch=x86_64,arm logf(long)
@ cdecl longjmp(ptr long)
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
@ cdecl -version=0x600+ mbrlen(str long ptr)
@ cdecl -version=0x600+ mbrtowc(ptr str long ptr)
@ stub -version=0x600+ mbsdup_dbg
@ stub -version=0x600+ mbsrtowcs
@ stub -version=0x600+ mbsrtowcs_s
@ cdecl mbstowcs(ptr str long)
@ stub -version=0x600+ mbstowcs_s
@ cdecl mbtowc(wstr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl -version=0x600+ memcpy_s(ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl -version=0x600+ memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long)
@ cdecl mktime(ptr)
@ cdecl modf(double ptr)
@ cdecl -arch=x86_64,arm modff(long ptr)
@ cdecl perror(str)
@ cdecl pow(double double)
@ cdecl -arch=x86_64,arm powf(long)
@ varargs printf(str)
@ stub -version=0x600+ printf_s
@ cdecl putc(long ptr)
@ cdecl putchar(long)
@ cdecl puts(str)
@ cdecl putwc(long ptr) fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr)
@ stub -version=0x600+ qsort_s
@ cdecl raise(long)
@ cdecl rand()
@ cdecl -version=0x600+ rand_s(ptr)
@ cdecl realloc(ptr long)
@ cdecl remove(str)
@ cdecl rename(str str)
@ cdecl rewind(ptr)
@ varargs scanf(str)
@ stub -version=0x600+ scanf_s
@ cdecl setbuf(ptr ptr)
@ cdecl -arch=x86_64,arm -norelay setjmp(ptr ptr) _setjmp
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long)
@ cdecl signal(long long)
@ cdecl sin(double)
@ cdecl -arch=x86_64,arm sinf(long)
@ cdecl sinh(double)
@ cdecl -arch=x86_64,arm sinhf(long)
@ varargs sprintf(ptr str)
@ varargs -version=0x600+ sprintf_s(ptr long str)
@ cdecl sqrt(double)
@ cdecl -arch=x86_64,arm sqrtf(long)
@ cdecl srand(long)
@ varargs sscanf(str str)
@ stub -version=0x600+ sscanf_s
@ cdecl strcat(str str)
@ cdecl -version=0x600+ strcat_s(ptr long str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl -version=0x600+ strcpy_s(ptr long str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl -version=0x600+ strerror_s(ptr long long)
@ cdecl strftime(str long str ptr)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl -version=0x600+ strncat_s(str long str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl -version=0x600+ strncpy_s(ptr long str long)
@ cdecl -version=0x600+ strnlen(str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtok(str str)
@ cdecl -version=0x600+ strtok_s(str str ptr)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ cdecl strxfrm(ptr str long)
@ varargs swprintf(ptr wstr)
@ stub -version=0x600+ swprintf_s
@ varargs swscanf(wstr wstr)
@ stub -version=0x600+ swscanf_s
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl -arch=x86_64,arm tanf(long)
@ cdecl tanh(double)
@ cdecl -arch=x86_64,arm tanhf(long)
@ cdecl time(ptr)
@ cdecl tmpfile()
@ stub -version=0x600+ tmpfile_s
@ cdecl tmpnam(ptr)
@ stub -version=0x600+ tmpnam_s
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl ungetc(long ptr)
@ cdecl ungetwc(long ptr)
@ stub -version=0x600+ utime
@ cdecl vfprintf(ptr str ptr)
@ stub -version=0x600+ vfprintf_s
@ cdecl vfwprintf(ptr wstr ptr)
@ stub -version=0x600+ vfwprintf_s
@ cdecl vprintf(str ptr)
@ stub -version=0x600+ vprintf_s
@ cdecl -version=0x600+ vsnprintf(ptr long str ptr)
@ cdecl vsprintf(ptr str ptr)
@ stub -version=0x600+ vsprintf_s
@ cdecl vswprintf(ptr wstr ptr)
@ stub -version=0x600+ vswprintf_s
@ cdecl vwprintf(wstr ptr)
@ stub -version=0x600+ vwprintf_s
@ cdecl -version=0x600+ wcrtomb(ptr long ptr)
@ stub -version=0x600+ wcrtomb_s
@ cdecl wcscat(wstr wstr)
@ cdecl -version=0x600+ wcscat_s(wstr long wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscoll(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl -version=0x600+ wcscpy_s(wstr long wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcsftime(ptr long wstr ptr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl -version=0x600+ wcsncat_s(wstr long wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl -version=0x600+ wcsncpy_s(ptr long wstr long)
@ cdecl -version=0x600+ wcsnlen(wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ stub -version=0x600+ wcsrtombs
@ stub -version=0x600+ wcsrtombs_s
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstok(wstr wstr)
@ cdecl -version=0x600+ wcstok_s(wstr wstr ptr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ stub -version=0x600+ wcstombs_s
@ cdecl wcstoul(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
@ stub -version=0x600+ wctob
@ cdecl wctomb(ptr long)
@ stub -version=0x600+ wctomb_s
@ varargs wprintf(wstr)
@ stub -version=0x600+ wprintf_s
@ varargs wscanf(wstr)
@ stub -version=0x600+ wscanf_s
