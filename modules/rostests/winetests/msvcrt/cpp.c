/* Unit test suite for msvcrt C++ objects
 *
 * Copyright 2003 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include "wine/test.h"
#include "winbase.h"
#include "winnt.h"

typedef void (*vtable_ptr)(void);

typedef struct __exception
{
  vtable_ptr *vtable;
  char *name;
  int   do_free;
} exception;

typedef struct __type_info
{
  vtable_ptr *vtable;
  char *name;
  char  mangled[16];
} type_info;

#undef __thiscall
#ifdef __i386__
#define __thiscall __stdcall
#else
#define __thiscall __cdecl
#endif

/* Function pointers. We need to use these to call these funcs as __thiscall */
static void* (__cdecl *poperator_new)(size_t);
static void  (__cdecl *poperator_delete)(void*);

/* exception */
static void (__thiscall *pexception_ctor)(exception*,LPCSTR*);
static void (__thiscall *pexception_copy_ctor)(exception*,exception*);
static void (__thiscall *pexception_default_ctor)(exception*);
static void (__thiscall *pexception_dtor)(exception*);
static exception* (__thiscall *pexception_opequals)(exception*,exception*);
static char* (__thiscall *pexception_what)(exception*);
static vtable_ptr *pexception_vtable;
static void (__thiscall *pexception_vector_dtor)(exception*,unsigned int);
static void (__thiscall *pexception_scalar_dtor)(exception*,unsigned int);

/* bad_typeid */
static void (__thiscall *pbad_typeid_ctor)(exception*,LPCSTR);
static void (__thiscall *pbad_typeid_ctor_closure)(exception*);
static void (__thiscall *pbad_typeid_copy_ctor)(exception*,exception*);
static void (__thiscall *pbad_typeid_dtor)(exception*);
static exception* (__thiscall *pbad_typeid_opequals)(exception*,exception*);
static char* (__thiscall *pbad_typeid_what)(exception*);
static vtable_ptr *pbad_typeid_vtable;
static void (__thiscall *pbad_typeid_vector_dtor)(exception*,unsigned int);
static void (__thiscall *pbad_typeid_scalar_dtor)(exception*,unsigned int);

/* bad_cast */
static void (__thiscall *pbad_cast_ctor)(exception*,LPCSTR*);
static void (__thiscall *pbad_cast_ctor2)(exception*,LPCSTR);
static void (__thiscall *pbad_cast_ctor_closure)(exception*);
static void (__thiscall *pbad_cast_copy_ctor)(exception*,exception*);
static void (__thiscall *pbad_cast_dtor)(exception*);
static exception* (__thiscall *pbad_cast_opequals)(exception*,exception*);
static char* (__thiscall *pbad_cast_what)(exception*);
static vtable_ptr *pbad_cast_vtable;
static void (__thiscall *pbad_cast_vector_dtor)(exception*,unsigned int);
static void (__thiscall *pbad_cast_scalar_dtor)(exception*,unsigned int);

/* __non_rtti_object */
static void (__thiscall *p__non_rtti_object_ctor)(exception*,LPCSTR);
static void (__thiscall *p__non_rtti_object_copy_ctor)(exception*,exception*);
static void (__thiscall *p__non_rtti_object_dtor)(exception*);
static exception* (__thiscall *p__non_rtti_object_opequals)(exception*,exception*);
static char* (__thiscall *p__non_rtti_object_what)(exception*);
static vtable_ptr *p__non_rtti_object_vtable;
static void (__thiscall *p__non_rtti_object_vector_dtor)(exception*,unsigned int);
static void (__thiscall *p__non_rtti_object_scalar_dtor)(exception*,unsigned int);

/* type_info */
static void  (__thiscall *ptype_info_dtor)(type_info*);
static char* (__thiscall *ptype_info_raw_name)(type_info*);
static char* (__thiscall *ptype_info_name)(type_info*);
static int   (__thiscall *ptype_info_before)(type_info*,type_info*);
static int   (__thiscall *ptype_info_opequals_equals)(type_info*,type_info*);
static int   (__thiscall *ptype_info_opnot_equals)(type_info*,type_info*);

/* RTTI */
static type_info* (__cdecl *p__RTtypeid)(void*);
static void* (__cdecl *p__RTCastToVoid)(void*);
static void* (__cdecl *p__RTDynamicCast)(void*,int,void*,void*,int);

/*Demangle*/
static char* (__cdecl *p__unDName)(char*,const char*,int,void*,void*,unsigned short int);


/* Emulate a __thiscall */
#ifdef __i386__

#include "pshpack1.h"
struct thiscall_thunk
{
    BYTE pop_eax;    /* popl  %eax (ret addr) */
    BYTE pop_edx;    /* popl  %edx (func) */
    BYTE pop_ecx;    /* popl  %ecx (this) */
    BYTE push_eax;   /* pushl %eax */
    WORD jmp_edx;    /* jmp  *%edx */
};
#include "poppack.h"

static void * (WINAPI *call_thiscall_func1)( void *func, void *this );
static void * (WINAPI *call_thiscall_func2)( void *func, void *this, const void *a );

static void init_thiscall_thunk(void)
{
    struct thiscall_thunk *thunk = VirtualAlloc( NULL, sizeof(*thunk),
            MEM_COMMIT, PAGE_EXECUTE_READWRITE );
    thunk->pop_eax  = 0x58;   /* popl  %eax */
    thunk->pop_edx  = 0x5a;   /* popl  %edx */
    thunk->pop_ecx  = 0x59;   /* popl  %ecx */
    thunk->push_eax = 0x50;   /* pushl %eax */
    thunk->jmp_edx  = 0xe2ff; /* jmp  *%edx */
    call_thiscall_func1 = (void *)thunk;
    call_thiscall_func2 = (void *)thunk;
}

#define call_func1(func,_this) call_thiscall_func1(func,_this)
#define call_func2(func,_this,a) call_thiscall_func2(func,_this,(const void*)(a))

#else

#define init_thiscall_thunk() do { } while(0)
#define call_func1(func,_this) func(_this)
#define call_func2(func,_this,a) func(_this,a)

#endif /* __i386__ */

/* Some exports are only available in later versions */
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hmsvcrt,y)
#define SET(x,y) do { SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y); } while(0)

static BOOL InitFunctionPtrs(void)
{
    HMODULE hmsvcrt = GetModuleHandleA("msvcrt.dll");

    SET(pexception_vtable, "??_7exception@@6B@");
    SET(pbad_typeid_vtable, "??_7bad_typeid@@6B@");
    SET(pbad_cast_vtable, "??_7bad_cast@@6B@");
    SET(p__non_rtti_object_vtable, "??_7__non_rtti_object@@6B@");

    SET(p__RTtypeid, "__RTtypeid");
    SET(p__RTCastToVoid, "__RTCastToVoid");
    SET(p__RTDynamicCast, "__RTDynamicCast");

    SET(p__unDName,"__unDName");

    /* Extremely early versions export logic_error, and crash in RTTI */
    if (sizeof(void *) > sizeof(int))  /* 64-bit initialization */
    {
        SETNOFAIL(poperator_new, "??_U@YAPEAX_K@Z");
        SETNOFAIL(poperator_delete, "??_V@YAXPEAX@Z");

        SET(pexception_ctor, "??0exception@@QEAA@AEBQEBD@Z");
        SET(pexception_copy_ctor, "??0exception@@QEAA@AEBV0@@Z");
        SET(pexception_default_ctor, "??0exception@@QEAA@XZ");
        SET(pexception_dtor, "??1exception@@UEAA@XZ");
        SET(pexception_opequals, "??4exception@@QEAAAEAV0@AEBV0@@Z");
        SET(pexception_what, "?what@exception@@UEBAPEBDXZ");
        pexception_vector_dtor = (void*)pexception_vtable[0];
        pexception_scalar_dtor = (void*)pexception_vtable[0];

        SET(pbad_typeid_ctor, "??0bad_typeid@@QEAA@PEBD@Z");
        SETNOFAIL(pbad_typeid_ctor_closure, "??_Fbad_typeid@@QEAAXXZ");
        SET(pbad_typeid_copy_ctor, "??0bad_typeid@@QEAA@AEBV0@@Z");
        SET(pbad_typeid_dtor, "??1bad_typeid@@UEAA@XZ");
        SET(pbad_typeid_opequals, "??4bad_typeid@@QEAAAEAV0@AEBV0@@Z");
        SET(pbad_typeid_what, "?what@exception@@UEBAPEBDXZ");
        pbad_typeid_vector_dtor = (void*)pbad_typeid_vtable[0];
        pbad_typeid_scalar_dtor = (void*)pbad_typeid_vtable[0];

        SET(pbad_cast_ctor, "??0bad_cast@@QEAA@AEBQEBD@Z");
        SET(pbad_cast_ctor2, "??0bad_cast@@QEAA@PEBD@Z");
        SET(pbad_cast_ctor_closure, "??_Fbad_cast@@QEAAXXZ");
        SET(pbad_cast_copy_ctor, "??0bad_cast@@QEAA@AEBV0@@Z");
        SET(pbad_cast_dtor, "??1bad_cast@@UEAA@XZ");
        SET(pbad_cast_opequals, "??4bad_cast@@QEAAAEAV0@AEBV0@@Z");
        SET(pbad_cast_what, "?what@exception@@UEBAPEBDXZ");
        pbad_cast_vector_dtor = (void*)pbad_cast_vtable[0];
        pbad_cast_scalar_dtor = (void*)pbad_cast_vtable[0];

        SET(p__non_rtti_object_ctor, "??0__non_rtti_object@@QEAA@PEBD@Z");
        SET(p__non_rtti_object_copy_ctor, "??0__non_rtti_object@@QEAA@AEBV0@@Z");
        SET(p__non_rtti_object_dtor, "??1__non_rtti_object@@UEAA@XZ");
        SET(p__non_rtti_object_opequals, "??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z");
        SET(p__non_rtti_object_what, "?what@exception@@UEBAPEBDXZ");
        p__non_rtti_object_vector_dtor = (void*)p__non_rtti_object_vtable[0];
        p__non_rtti_object_scalar_dtor = (void*)p__non_rtti_object_vtable[0];

        SET(ptype_info_dtor, "??1type_info@@UEAA@XZ");
        SET(ptype_info_raw_name, "?raw_name@type_info@@QEBAPEBDXZ");
        SET(ptype_info_name, "?name@type_info@@QEBAPEBDXZ");
        SET(ptype_info_before, "?before@type_info@@QEBAHAEBV1@@Z");
        SET(ptype_info_opequals_equals, "??8type_info@@QEBAHAEBV0@@Z");
        SET(ptype_info_opnot_equals, "??9type_info@@QEBAHAEBV0@@Z");
    }
    else
    {
#ifdef __arm__
        SETNOFAIL(poperator_new, "??_U@YAPAXI@Z");
        SETNOFAIL(poperator_delete, "??_V@YAXPAX@Z");

        SET(pexception_ctor, "??0exception@@QAA@ABQBD@Z");
        SET(pexception_copy_ctor, "??0exception@@QAA@ABV0@@Z");
        SET(pexception_default_ctor, "??0exception@@QAA@XZ");
        SET(pexception_dtor, "??1exception@@UAA@XZ");
        SET(pexception_opequals, "??4exception@@QAAAAV0@ABV0@@Z");
        SET(pexception_what, "?what@exception@@UBAPBDXZ");
        pexception_vector_dtor = (void*)pexception_vtable[0];
        pexception_scalar_dtor = (void*)pexception_vtable[0];

        SET(pbad_typeid_ctor, "??0bad_typeid@@QAA@PBD@Z");
        SETNOFAIL(pbad_typeid_ctor_closure, "??_Fbad_typeid@@QAAXXZ");
        SET(pbad_typeid_copy_ctor, "??0bad_typeid@@QAA@ABV0@@Z");
        SET(pbad_typeid_dtor, "??1bad_typeid@@UAA@XZ");
        SET(pbad_typeid_opequals, "??4bad_typeid@@QAAAAV0@ABV0@@Z");
        SET(pbad_typeid_what, "?what@exception@@UBAPBDXZ");
        pbad_typeid_vector_dtor = (void*)pbad_typeid_vtable[0];
        pbad_typeid_scalar_dtor = (void*)pbad_typeid_vtable[0];

        SETNOFAIL(pbad_cast_ctor, "??0bad_cast@@QAE@ABQBD@Z");
        if (!pbad_cast_ctor)
            SET(pbad_cast_ctor, "??0bad_cast@@AAA@PBQBD@Z");
        SETNOFAIL(pbad_cast_ctor2, "??0bad_cast@@QAA@PBD@Z");
        SETNOFAIL(pbad_cast_ctor_closure, "??_Fbad_cast@@QAAXXZ");
        SET(pbad_cast_copy_ctor, "??0bad_cast@@QAA@ABV0@@Z");
        SET(pbad_cast_dtor, "??1bad_cast@@UAA@XZ");
        SET(pbad_cast_opequals, "??4bad_cast@@QAAAAV0@ABV0@@Z");
        SET(pbad_cast_what, "?what@exception@@UBAPBDXZ");
        pbad_cast_vector_dtor = (void*)pbad_cast_vtable[0];
        pbad_cast_scalar_dtor = (void*)pbad_cast_vtable[0];

        SET(p__non_rtti_object_ctor, "??0__non_rtti_object@@QAA@PBD@Z");
        SET(p__non_rtti_object_copy_ctor, "??0__non_rtti_object@@QAA@ABV0@@Z");
        SET(p__non_rtti_object_dtor, "??1__non_rtti_object@@UAA@XZ");
        SET(p__non_rtti_object_opequals, "??4__non_rtti_object@@QAAAAV0@ABV0@@Z");
        SET(p__non_rtti_object_what, "?what@exception@@UBAPBDXZ");
        p__non_rtti_object_vector_dtor = (void*)p__non_rtti_object_vtable[0];
        p__non_rtti_object_scalar_dtor = (void*)p__non_rtti_object_vtable[0];

        SET(ptype_info_dtor, "??1type_info@@UAA@XZ");
        SET(ptype_info_raw_name, "?raw_name@type_info@@QBAPBDXZ");
        SET(ptype_info_name, "?name@type_info@@QBAPBDXZ");
        SET(ptype_info_before, "?before@type_info@@QBAHABV1@@Z");
        SET(ptype_info_opequals_equals, "??8type_info@@QBAHABV0@@Z");
        SET(ptype_info_opnot_equals, "??9type_info@@QBAHABV0@@Z");
#else
        SETNOFAIL(poperator_new, "??_U@YAPAXI@Z");
        SETNOFAIL(poperator_delete, "??_V@YAXPAX@Z");

        SET(pexception_ctor, "??0exception@@QAE@ABQBD@Z");
        SET(pexception_copy_ctor, "??0exception@@QAE@ABV0@@Z");
        SET(pexception_default_ctor, "??0exception@@QAE@XZ");
        SET(pexception_dtor, "??1exception@@UAE@XZ");
        SET(pexception_opequals, "??4exception@@QAEAAV0@ABV0@@Z");
        SET(pexception_what, "?what@exception@@UBEPBDXZ");
        SET(pexception_vector_dtor, "??_Eexception@@UAEPAXI@Z");
        SET(pexception_scalar_dtor, "??_Gexception@@UAEPAXI@Z");

        SET(pbad_typeid_ctor, "??0bad_typeid@@QAE@PBD@Z");
        SETNOFAIL(pbad_typeid_ctor_closure, "??_Fbad_typeid@@QAEXXZ");
        SET(pbad_typeid_copy_ctor, "??0bad_typeid@@QAE@ABV0@@Z");
        SET(pbad_typeid_dtor, "??1bad_typeid@@UAE@XZ");
        SET(pbad_typeid_opequals, "??4bad_typeid@@QAEAAV0@ABV0@@Z");
        SET(pbad_typeid_what, "?what@exception@@UBEPBDXZ");
        SET(pbad_typeid_vector_dtor, "??_Ebad_typeid@@UAEPAXI@Z");
        SET(pbad_typeid_scalar_dtor, "??_Gbad_typeid@@UAEPAXI@Z");

        SETNOFAIL(pbad_cast_ctor, "??0bad_cast@@QAE@ABQBD@Z");
        if (!pbad_cast_ctor)
            SET(pbad_cast_ctor, "??0bad_cast@@AAE@PBQBD@Z");
        SETNOFAIL(pbad_cast_ctor2, "??0bad_cast@@QAE@PBD@Z");
        SETNOFAIL(pbad_cast_ctor_closure, "??_Fbad_cast@@QAEXXZ");
        SET(pbad_cast_copy_ctor, "??0bad_cast@@QAE@ABV0@@Z");
        SET(pbad_cast_dtor, "??1bad_cast@@UAE@XZ");
        SET(pbad_cast_opequals, "??4bad_cast@@QAEAAV0@ABV0@@Z");
        SET(pbad_cast_what, "?what@exception@@UBEPBDXZ");
        SET(pbad_cast_vector_dtor, "??_Ebad_cast@@UAEPAXI@Z");
        SET(pbad_cast_scalar_dtor, "??_Gbad_cast@@UAEPAXI@Z");

        SET(p__non_rtti_object_ctor, "??0__non_rtti_object@@QAE@PBD@Z");
        SET(p__non_rtti_object_copy_ctor, "??0__non_rtti_object@@QAE@ABV0@@Z");
        SET(p__non_rtti_object_dtor, "??1__non_rtti_object@@UAE@XZ");
        SET(p__non_rtti_object_opequals, "??4__non_rtti_object@@QAEAAV0@ABV0@@Z");
        SET(p__non_rtti_object_what, "?what@exception@@UBEPBDXZ");
        SET(p__non_rtti_object_vector_dtor, "??_E__non_rtti_object@@UAEPAXI@Z");
        SET(p__non_rtti_object_scalar_dtor, "??_G__non_rtti_object@@UAEPAXI@Z");

        SET(ptype_info_dtor, "??1type_info@@UAE@XZ");
        SET(ptype_info_raw_name, "?raw_name@type_info@@QBEPBDXZ");
        SET(ptype_info_name, "?name@type_info@@QBEPBDXZ");
        SET(ptype_info_before, "?before@type_info@@QBEHABV1@@Z");
        SET(ptype_info_opequals_equals, "??8type_info@@QBEHABV0@@Z");
        SET(ptype_info_opnot_equals, "??9type_info@@QBEHABV0@@Z");
#endif /* __arm__ */
    }

    if (!poperator_new)
        poperator_new = malloc;
    if (!poperator_delete)
        poperator_delete = free;

    init_thiscall_thunk();
    return TRUE;
}

static void test_exception(void)
{
  static const char* e_name = "An exception name";
  char* name;
  exception e, e2, e3, *pe;

  if (!poperator_new || !poperator_delete ||
      !pexception_ctor || !pexception_copy_ctor || !pexception_default_ctor ||
      !pexception_dtor || !pexception_opequals || !pexception_what ||
      !pexception_vtable || !pexception_vector_dtor || !pexception_scalar_dtor)
    return;

  /* 'const char*&' ctor */
  memset(&e, 0, sizeof(e));
  call_func2(pexception_ctor, &e, &e_name);
  ok(e.vtable != NULL, "Null exception vtable for e\n");
  ok(e.name && e.name != e_name && !strcmp(e.name, "An exception name"), "Bad name '%s' for e\n", e.name);
  ok(e.do_free == 1, "do_free set to %d for e\n", e.do_free);

  /* Copy ctor */
  memset(&e2, 0, sizeof(e2));
  call_func2(pexception_copy_ctor, &e2, &e);
  ok(e2.vtable != NULL, "Null exception vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "An exception name"), "Bad exception name for e2\n");
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);

  /* Default ctor */
  memset(&e3, 1, sizeof(e3));
  call_func1(pexception_default_ctor, &e3);
  ok(e3.vtable != NULL, "Null exception vtable for e3\n");
  ok(e3.name == NULL, "Bad exception name for e3\n");
  ok(e3.do_free == 0, "do_free set to %d for e3\n", e3.do_free);

  ok(e.vtable == e2.vtable && e.vtable == e3.vtable, "exception vtables differ!\n");

  /* Test calling the dtors */
  call_func1(pexception_dtor, &e2);
  call_func1(pexception_dtor, &e3);

  /* Operator equals */
  memset(&e2, 0, sizeof(e2));
  call_func1(pexception_default_ctor, &e2);
  pe = call_func2(pexception_opequals, &e2, &e);
  ok(e2.vtable != NULL, "Null exception vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "An exception name"), "Bad exception name for e2\n");
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);
  ok(pe == &e2, "opequals didn't return e2\n");

  /* what() */
  name = call_func1(pexception_what, &e2);
  ok(e2.name == name, "Bad exception name from e2::what()\n");

  /* vtable ptr */
  ok(e2.vtable == pexception_vtable, "Bad vtable for e2\n");
  call_func1(pexception_dtor, &e2);

  /* new() */
  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    call_func2(pexception_ctor, pe, &e_name);
    /* scalar dtor */
    call_func2(pexception_scalar_dtor, pe, 0); /* Shouldn't delete pe */
    pe->name = NULL;
    pe->do_free = 0;
    call_func2(pexception_scalar_dtor, pe, 1); /* Should delete pe */
  }

  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, single element */
    call_func2(pexception_ctor, pe, &e_name);
    call_func2(pexception_vector_dtor, pe, 1); /* Should delete pe as single element*/
  }

  pe = poperator_new(sizeof(exception) * 4 + sizeof(size_t));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    char name[] = "a constant";
    *((size_t*)pe) = 3;
    pe = (exception*)((size_t*)pe + 1);
    call_func2(pexception_ctor, &pe[0], &e_name);
    call_func2(pexception_ctor, &pe[1], &e_name);
    call_func2(pexception_ctor, &pe[2], &e_name);
    pe[3].name = name;
    pe[3].do_free = 1; /* Crash if we try to free this */
    call_func2(pexception_vector_dtor, pe, 3); /* Should delete all 3 and then pe block */
  }

  /* test our exported vtable is kosher */
  pe = (void*)pexception_vtable; /* Use the exception struct to get vtable ptrs */
  pexception_vector_dtor = (void*)pe->vtable;
  pexception_what = (void*)pe->name;

  name = call_func1(pexception_what, &e);
  ok(e.name == name, "Bad exception name from vtable e::what()\n");

  if (p__RTtypeid)
  {
    /* Check the rtti */
    type_info *ti = p__RTtypeid(&e);
    ok (ti && !strcmp(ti->mangled, ".?AVexception@@"), "bad rtti for e\n");

    if (ti)
    {
      /* Check the returned type_info has rtti too */
      type_info *ti2 = p__RTtypeid(ti);
      ok (ti2 != NULL && !strcmp(ti2->mangled, ".?AVtype_info@@"), "bad rtti for e's type_info\n");
    }
  }

  call_func2(pexception_vector_dtor, &e, 0); /* Should delete e.name, but not e */
}

/* This test is basically a cut 'n' paste of the exception test. but it verifies that
 * bad_typeid works the exact same way... */
static void test_bad_typeid(void)
{
  static const char* e_name = "A bad_typeid name";
  char* name;
  exception e, e2, e3, *pe;

  if (!poperator_new || !poperator_delete ||
      !pbad_typeid_ctor || !pbad_typeid_copy_ctor ||
      !pbad_typeid_dtor || !pbad_typeid_opequals || !pbad_typeid_what ||
      !pbad_typeid_vtable || !pbad_typeid_vector_dtor || !pbad_typeid_scalar_dtor)
    return;

  /* 'const char*' ctor */
  memset(&e, 0, sizeof(e));
  call_func2(pbad_typeid_ctor, &e, e_name);
  ok(e.vtable != NULL, "Null bad_typeid vtable for e\n");
  ok(e.name && e.name != e_name && !strcmp(e.name, "A bad_typeid name"), "Bad name '%s' for e\n", e.name);
  ok(e.do_free == 1, "do_free set to %d for e\n", e.do_free);

  /* Copy ctor */
  memset(&e2, 0, sizeof(e2));
  call_func2(pbad_typeid_copy_ctor, &e2, &e);
  ok(e2.vtable != NULL, "Null bad_typeid vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A bad_typeid name"), "Bad name '%s' for e2\n", e2.name);
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);

  /* Ctor closure */
  if (pbad_typeid_ctor_closure)
  {
    memset(&e3, 1, sizeof(e3));
    call_func1(pbad_typeid_ctor_closure, &e3);
    ok(e3.vtable != NULL, "Null bad_typeid vtable for e3\n");
    ok(e3.name && !strcmp(e3.name, "bad typeid"), "Bad bad_typeid name for e3\n");
    ok(e3.do_free == 1, "do_free set to %d for e3\n", e3.do_free);
    ok(e.vtable == e3.vtable, "bad_typeid closure vtables differ!\n");
    call_func1(pbad_typeid_dtor, &e3);
  }
  ok(e.vtable == e2.vtable, "bad_typeid vtables differ!\n");

  /* Test calling the dtors */
  call_func1(pbad_typeid_dtor, &e2);

  /* Operator equals */
  memset(&e2, 1, sizeof(e2));
  call_func1(pexception_default_ctor, &e2);
  pe = call_func2(pbad_typeid_opequals, &e2, &e);
  ok(e2.vtable != NULL, "Null bad_typeid vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A bad_typeid name"), "Bad bad_typeid name for e2\n");
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);
  ok(pe == &e2, "opequals didn't return e2\n");

  /* what() */
  name = call_func1(pbad_typeid_what, &e2);
  ok(e2.name == name, "Bad bad_typeid name from e2::what()\n");

  /* vtable ptr */
  ok(e2.vtable == pexception_vtable, "Bad vtable for e2\n");
  call_func1(pbad_typeid_dtor, &e2);

  /* new() */
  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    call_func2(pbad_typeid_ctor, pe, e_name);
    /* scalar dtor */
    call_func2(pbad_typeid_scalar_dtor, pe, 0); /* Shouldn't delete pe */
    pe->name = NULL;
    pe->do_free = 0;
    call_func2(pbad_typeid_scalar_dtor, pe, 1); /* Should delete pe */
  }

  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, single element */
    call_func2(pbad_typeid_ctor, pe, e_name);
    call_func2(pbad_typeid_vector_dtor, pe, 1); /* Should delete pe as single element*/
  }

  pe = poperator_new(sizeof(exception) * 4 + sizeof(size_t));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((size_t*)pe) = 3;
    pe = (exception*)((size_t*)pe + 1);
    call_func2(pbad_typeid_ctor, &pe[0], e_name);
    call_func2(pbad_typeid_ctor, &pe[1], e_name);
    call_func2(pbad_typeid_ctor, &pe[2], e_name);
    pe[3].name = 0;
    pe[3].do_free = 1; /* Crash if we try to free this element */
    call_func2(pbad_typeid_vector_dtor, pe, 3); /* Should delete all 3 and then pe block */
  }

  /* test our exported vtable is kosher */
  pe = (void*)pbad_typeid_vtable; /* Use the exception struct to get vtable ptrs */
  pbad_typeid_vector_dtor = (void*)pe->vtable;
  pbad_typeid_what = (void*)pe->name;

  name = call_func1(pbad_typeid_what, &e);
  ok(e.name == name, "Bad bad_typeid name from vtable e::what()\n");

  if (p__RTtypeid)
  {
    /* Check the rtti */
    type_info *ti = p__RTtypeid(&e);
    ok (ti != NULL && !strcmp(ti->mangled, ".?AVbad_typeid@@"), "bad rtti for e (%s)\n",
        !ti ? "null" : ti->mangled);
  }

  call_func2(pbad_typeid_vector_dtor, &e, 0); /* Should delete e.name, but not e */
}


/* Ditto for this test... */
static void test_bad_cast(void)
{
  static const char* e_name = "A bad_cast name";
  char* name;
  exception e, e2, e3, *pe;

  if (!poperator_new || !poperator_delete ||
      !pbad_cast_ctor || !pbad_cast_copy_ctor ||
      !pbad_cast_dtor || !pbad_cast_opequals || !pbad_cast_what ||
      !pbad_cast_vtable || !pbad_cast_vector_dtor || !pbad_cast_scalar_dtor)
    return;

  if (pbad_cast_ctor2)
  {
    /* 'const char*' ctor */
    memset(&e, 0, sizeof(e));
    call_func2(pbad_cast_ctor2, &e, e_name);
    ok(e.vtable != NULL, "Null bad_cast vtable for e\n");
    ok(e.name && e.name != e_name && !strcmp(e.name, "A bad_cast name"), "Bad name '%s' for e\n", e.name);
    ok(e.do_free == 1, "do_free set to %d for e\n", e.do_free);
    call_func1(pbad_cast_dtor, &e);
  }

  /* 'const char*&' ctor */
  memset(&e, 0, sizeof(e));
  call_func2(pbad_cast_ctor, &e, &e_name);
  ok(e.vtable != NULL, "Null bad_cast vtable for e\n");
  ok(e.name && e.name != e_name && !strcmp(e.name, "A bad_cast name"), "Bad name '%s' for e\n", e.name);
  ok(e.do_free == 1, "do_free set to %d for e\n", e.do_free);

  /* Copy ctor */
  memset(&e2, 0, sizeof(e2));
  call_func2(pbad_cast_copy_ctor, &e2, &e);
  ok(e2.vtable != NULL, "Null bad_cast vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A bad_cast name"), "Bad name '%s' for e2\n", e2.name);
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);

  /* Ctor closure */
  if (pbad_cast_ctor_closure)
  {
    memset(&e3, 1, sizeof(e3));
    call_func1(pbad_cast_ctor_closure, &e3);
    ok(e3.vtable != NULL, "Null bad_cast vtable for e3\n");
    ok(e3.name && !strcmp(e3.name, "bad cast"), "Bad bad_cast name for e3\n");
    ok(e3.do_free == 1, "do_free set to %d for e3\n", e3.do_free);
    ok(e.vtable == e3.vtable, "bad_cast closure vtables differ!\n");
    call_func1(pbad_cast_dtor, &e3);
  }
  ok(e.vtable == e2.vtable, "bad_cast vtables differ!\n");

  /* Test calling the dtors */
  call_func1(pbad_cast_dtor, &e2);

  /* Operator equals */
  memset(&e2, 1, sizeof(e2));
  call_func1(pexception_default_ctor, &e2);
  pe = call_func2(pbad_cast_opequals, &e2, &e);
  ok(e2.vtable != NULL, "Null bad_cast vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A bad_cast name"), "Bad bad_cast name for e2\n");
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);
  ok(pe == &e2, "opequals didn't return e2\n");

  /* what() */
  name = call_func1(pbad_cast_what, &e2);
  ok(e2.name == name, "Bad bad_cast name from e2::what()\n");

  /* vtable ptr */
  ok(e2.vtable == pexception_vtable, "Bad vtable for e2\n");
  call_func1(pbad_cast_dtor, &e2);

  /* new() */
  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    call_func2(pbad_cast_ctor, pe, &e_name);
    /* scalar dtor */
    call_func2(pbad_cast_scalar_dtor, pe, 0); /* Shouldn't delete pe */
    pe->name = NULL;
    pe->do_free = 0;
    call_func2(pbad_cast_scalar_dtor, pe, 1); /* Should delete pe */
  }

  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, single element */
    call_func2(pbad_cast_ctor, pe, &e_name);
    call_func2(pbad_cast_vector_dtor, pe, 1); /* Should delete pe as single element*/
  }

  pe = poperator_new(sizeof(exception) * 4 + sizeof(size_t));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((size_t*)pe) = 3;
    pe = (exception*)((size_t*)pe + 1);
    call_func2(pbad_cast_ctor, &pe[0], &e_name);
    call_func2(pbad_cast_ctor, &pe[1], &e_name);
    call_func2(pbad_cast_ctor, &pe[2], &e_name);
    pe[3].name = 0;
    pe[3].do_free = 1; /* Crash if we try to free this element */
    call_func2(pbad_cast_vector_dtor, pe, 3); /* Should delete all 3 and then pe block */
  }

  /* test our exported vtable is kosher */
  pe = (void*)pbad_cast_vtable; /* Use the exception struct to get vtable ptrs */
  pbad_cast_vector_dtor = (void*)pe->vtable;
  pbad_cast_what = (void*)pe->name;

  name = call_func1(pbad_cast_what, &e);
  ok(e.name == name, "Bad bad_cast name from vtable e::what()\n");

  if (p__RTtypeid)
  {
    /* Check the rtti */
    type_info *ti = p__RTtypeid(&e);
    ok (ti != NULL && !strcmp(ti->mangled, ".?AVbad_cast@@"), "bad rtti for e\n");
  }
  call_func2(pbad_cast_vector_dtor, &e, 0); /* Should delete e.name, but not e */
}

/* ... and this one */
static void test___non_rtti_object(void)
{
  static const char* e_name = "A __non_rtti_object name";
  char* name;
  exception e, e2, *pe;

  if (!poperator_new || !poperator_delete ||
      !p__non_rtti_object_ctor || !p__non_rtti_object_copy_ctor ||
      !p__non_rtti_object_dtor || !p__non_rtti_object_opequals || !p__non_rtti_object_what ||
      !p__non_rtti_object_vtable || !p__non_rtti_object_vector_dtor || !p__non_rtti_object_scalar_dtor)
    return;

  /* 'const char*' ctor */
  memset(&e, 0, sizeof(e));
  call_func2(p__non_rtti_object_ctor, &e, e_name);
  ok(e.vtable != NULL, "Null __non_rtti_object vtable for e\n");
  ok(e.name && e.name != e_name && !strcmp(e.name, "A __non_rtti_object name"), "Bad name '%s' for e\n", e.name);
  ok(e.do_free == 1, "do_free set to %d for e\n", e.do_free);

  /* Copy ctor */
  memset(&e2, 0, sizeof(e2));
  call_func2(p__non_rtti_object_copy_ctor, &e2, &e);
  ok(e2.vtable != NULL, "Null __non_rtti_object vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A __non_rtti_object name"), "Bad name '%s' for e2\n", e2.name);
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);
  ok(e.vtable == e2.vtable, "__non_rtti_object vtables differ!\n");

  /* Test calling the dtors */
  call_func1(p__non_rtti_object_dtor, &e2);

  /* Operator equals */
  memset(&e2, 1, sizeof(e2));
  call_func1(pexception_default_ctor, &e2);
  pe = call_func2(p__non_rtti_object_opequals, &e2, &e);
  ok(e2.vtable != NULL, "Null __non_rtti_object vtable for e2\n");
  ok(e2.name && e2.name != e.name && !strcmp(e2.name, "A __non_rtti_object name"), "Bad __non_rtti_object name for e2\n");
  ok(e2.do_free == 1, "do_free set to %d for e2\n", e2.do_free);
  ok(pe == &e2, "opequals didn't return e2\n");

  /* what() */
  name = call_func1(p__non_rtti_object_what, &e2);
  ok(e2.name == name, "Bad __non_rtti_object name from e2::what()\n");

  /* vtable ptr */
  ok(e2.vtable == pexception_vtable, "Bad vtable for e2\n");
  call_func1(p__non_rtti_object_dtor, &e2);

  /* new() */
  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    call_func2(p__non_rtti_object_ctor, pe, e_name);
    /* scalar dtor */
    call_func2(p__non_rtti_object_scalar_dtor, pe, 0); /* Shouldn't delete pe */
    pe->name = NULL;
    pe->do_free = 0;
    call_func2(p__non_rtti_object_scalar_dtor, pe, 1); /* Should delete pe */
  }

  pe = poperator_new(sizeof(exception));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, single element */
    call_func2(p__non_rtti_object_ctor, pe, e_name);
    call_func2(p__non_rtti_object_vector_dtor, pe, 1); /* Should delete pe as single element*/
  }

  pe = poperator_new(sizeof(exception) * 4 + sizeof(size_t));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((size_t*)pe) = 3;
    pe = (exception*)((size_t*)pe + 1);
    call_func2(p__non_rtti_object_ctor, &pe[0], e_name);
    call_func2(p__non_rtti_object_ctor, &pe[1], e_name);
    call_func2(p__non_rtti_object_ctor, &pe[2], e_name);
    pe[3].name = 0;
    pe[3].do_free = 1; /* Crash if we try to free this element */
    call_func2(p__non_rtti_object_vector_dtor, pe, 3); /* Should delete all 3 and then pe block */
  }

  /* test our exported vtable is kosher */
  pe = (void*)p__non_rtti_object_vtable; /* Use the exception struct to get vtable ptrs */
  p__non_rtti_object_vector_dtor = (void*)pe->vtable;
  p__non_rtti_object_what = (void*)pe->name;

  name = call_func1(p__non_rtti_object_what, &e);
  ok(e.name == name, "Bad __non_rtti_object name from vtable e::what()\n");

  if (p__RTtypeid)
  {
    /* Check the rtti */
    type_info *ti = p__RTtypeid(&e);
    ok (ti != NULL && !strcmp(ti->mangled, ".?AV__non_rtti_object@@"), "bad rtti for e\n");
  }
  call_func2(p__non_rtti_object_vector_dtor, &e, 0); /* Should delete e.name, but not e */
}

static void test_type_info(void)
{
  static type_info t1 = { NULL, NULL,{'.','?','A','V','t','e','s','t','1','@','@',0,0,0,0,0 } };
  static type_info t1_1 = { NULL, NULL,{'?','?','A','V','t','e','s','t','1','@','@',0,0,0,0,0 } };
  static type_info t2 = { NULL, NULL, {'.','?','A','V','t','e','s','t','2','@','@',0,0,0,0,0 } };
  char* name;
  int res;

  if (!ptype_info_dtor || !ptype_info_raw_name ||
      !ptype_info_name || !ptype_info_before ||
      !ptype_info_opequals_equals || !ptype_info_opnot_equals)
    return;

  /* Test calling the dtors */
  call_func1(ptype_info_dtor, &t1); /* No effect, since name is NULL */
  t1.name = malloc(64);
  strcpy(t1.name, "foo");
  call_func1(ptype_info_dtor, &t1); /* Frees t1.name using 'free' */

  /* raw_name */
  t1.name = NULL;
  name = call_func1(ptype_info_raw_name, &t1);

  /* FIXME: This fails on native; it shouldn't though - native bug?
   * ok(name && !strcmp(name, t1.mangled), "bad raw_name '%s' for t1 (expected '%s')\n", name, t1.mangled);
   */
  ok(t1.name == NULL, "raw_name() set name for t1\n");

  /* name */
  t1.name = NULL;
  name = call_func1(ptype_info_name, &t1);
  ok(name && t1.name && !strcmp(name, t1.name), "bad name '%s' for t1\n", name);

  ok(t1.name && !strcmp(t1.name, "class test1"), "demangled to '%s' for t1\n", t1.name);
  call_func1(ptype_info_dtor, &t1);

  /* before */
  t1.name = NULL;
  res = (int)call_func2(ptype_info_before, &t1, &t1);
  ok(res == 0, "expected 0, got %d\n", res);
  res = (int)call_func2(ptype_info_before, &t2, &t1);
  ok(res == 0, "expected 0, got %d\n", res);
  res = (int)call_func2(ptype_info_before, &t1, &t2);
  ok(res == 1, "expected 1, got %d\n", res);
  /* Doesn't check first char */
  res = (int)call_func2(ptype_info_before, &t1, &t1_1);
  ok(res == 0, "expected 0, got %d\n", res);

  /* opequals_equals */
  t1.name = NULL;
  res = (int)call_func2(ptype_info_opequals_equals, &t1, &t1);
  ok(res == 1, "expected 1, got %d\n", res);
  res = (int)call_func2(ptype_info_opequals_equals, &t1, &t2);
  ok(res == 0, "expected 0, got %d\n", res);
  res = (int)call_func2(ptype_info_opequals_equals, &t2, &t1);
  ok(res == 0, "expected 0, got %d\n", res);

  /* opnot_equals */
  t1.name = NULL;
  res = (int)call_func2(ptype_info_opnot_equals, &t1, &t1);
  ok(res == 0, "expected 0, got %d\n", res);
  res = (int)call_func2(ptype_info_opnot_equals, &t1, &t2);
  ok(res == 1, "expected 1, got %d\n", res);
  res = (int)call_func2(ptype_info_opnot_equals, &t2, &t1);
  ok(res == 1, "expected 1, got %d\n", res);
}

static inline vtable_ptr *get_vtable( void *obj )
{
    return *(vtable_ptr **)obj;
}

static inline void/*rtti_object_locator*/ *get_obj_locator( void *cppobj )
{
    const vtable_ptr *vtable = get_vtable( cppobj );
    return (void *)vtable[-1];
}

#ifdef __i386__
#define DEFINE_RTTI_REF(type, name) type *name
#define RTTI_REF(instance, name) &instance.name
#define RTTI_REF_SIG0(instance, name, base) RTTI_REF(instance, name)
#else
#define DEFINE_RTTI_REF(type, name) unsigned name
#define RTTI_REF(instance, name) FIELD_OFFSET(struct rtti_data, name)
#define RTTI_REF_SIG0(instance, name, base) ((char*)&instance.name-base)
#endif
/* Test RTTI functions */
static void test_rtti(void)
{
  struct _object_locator
  {
      unsigned int signature;
      int base_class_offset;
      unsigned int flags;
      DEFINE_RTTI_REF(type_info, type_descriptor);
      DEFINE_RTTI_REF(struct _rtti_object_hierarchy, type_hierarchy);
      DEFINE_RTTI_REF(void, object_locator);
  } *obj_locator;

  struct _rtti_base_descriptor
  {
    DEFINE_RTTI_REF(type_info, type_descriptor);
    int num_base_classes;
    struct {
      int this_offset;
      int vbase_descr;
      int vbase_offset;
    } this_ptr_offsets;
    unsigned int attributes;
  };

  struct _rtti_base_array {
    DEFINE_RTTI_REF(struct _rtti_base_descriptor, bases[4]);
  };

  struct _rtti_object_hierarchy {
    unsigned int signature;
    unsigned int attributes;
    int array_len;
    DEFINE_RTTI_REF(struct _rtti_base_array, base_classes);
  };

  struct rtti_data
  {
    type_info type_info[4];

    struct _rtti_base_descriptor base_descriptor[4];
    struct _rtti_base_array base_array;
    struct _rtti_object_hierarchy  object_hierarchy;
    struct _object_locator object_locator;
  } simple_class_rtti = {
    { {NULL, NULL, "simple_class"} },
    { {RTTI_REF(simple_class_rtti, type_info[0]), 0, {0, 0, 0}, 0} },
    { {RTTI_REF(simple_class_rtti, base_descriptor[0])} },
    {0, 0, 1, RTTI_REF(simple_class_rtti, base_array)},
    {1, 0, 0, RTTI_REF(simple_class_rtti, type_info[0]), RTTI_REF(simple_class_rtti, object_hierarchy), RTTI_REF(simple_class_rtti, object_locator)}
  }, child_class_rtti = {
    { {NULL, NULL, "simple_class"}, {NULL, NULL, "child_class"} },
    { {RTTI_REF(child_class_rtti, type_info[1]), 0, {4, -1, 0}, 0}, {RTTI_REF(child_class_rtti, type_info[0]), 0, {8, -1, 0}, 0} },
    { {RTTI_REF(child_class_rtti, base_descriptor[0]), RTTI_REF(child_class_rtti, base_descriptor[1])} },
    {0, 0, 2, RTTI_REF(child_class_rtti, base_array)},
    {1, 0, 0, RTTI_REF(child_class_rtti, type_info[1]), RTTI_REF(child_class_rtti, object_hierarchy), RTTI_REF(child_class_rtti, object_locator)}
  }, virtual_base_class_rtti = {
    { {NULL, NULL, "simple_class"}, {NULL, NULL, "child_class"} },
    { {RTTI_REF(virtual_base_class_rtti, type_info[1]), 0, {0x10, sizeof(void*), sizeof(int)}, 0}, {RTTI_REF(virtual_base_class_rtti, type_info[0]), 0, {8, -1, 0}, 0} },
    { {RTTI_REF(virtual_base_class_rtti, base_descriptor[0]), RTTI_REF(virtual_base_class_rtti, base_descriptor[1])} },
    {0, 0, 2, RTTI_REF(virtual_base_class_rtti, base_array)},
    {1, 0, 0, RTTI_REF(virtual_base_class_rtti, type_info[1]), RTTI_REF(virtual_base_class_rtti, object_hierarchy), RTTI_REF(virtual_base_class_rtti, object_locator)}
  };
  static struct rtti_data simple_class_sig0_rtti, child_class_sig0_rtti;

  void *simple_class_vtbl[2] = {&simple_class_rtti.object_locator};
  void *simple_class = &simple_class_vtbl[1];
  void *child_class_vtbl[2] = {&child_class_rtti.object_locator};
  struct {
      void *vtbl;
      char data[4];
  } child_class = { &child_class_vtbl[1] };
  void *simple_class_sig0_vtbl[2] = {&simple_class_sig0_rtti.object_locator};
  void *simple_class_sig0 = &simple_class_sig0_vtbl[1];
  void *child_class_sig0_vtbl[2] = {&child_class_sig0_rtti.object_locator};
  struct {
      void *vtbl;
      char data[4];
  } child_class_sig0 = { &child_class_sig0_vtbl[1] };
  void *virtual_base_class_vtbl[2] = {&virtual_base_class_rtti.object_locator};
  int virtual_base_class_vbtbl[2] = {0, 0x100};
  struct {
      void *virtual_base[2];
      char data[0x110-sizeof(void*)];
      void *vbthis;
  } virtual_base_class = { {&virtual_base_class_vtbl[1], virtual_base_class_vbtbl} };

  static const char* e_name = "name";
  type_info *ti,*bti;
  exception e,b;
  void *casted;
  BOOL old_signature;
#ifndef __i386__
  char *base = (char*)GetModuleHandleW(NULL);
#endif

  if (!p__RTCastToVoid || !p__RTtypeid || !pexception_ctor || !pbad_typeid_ctor
      || !p__RTDynamicCast || !pexception_dtor || !pbad_typeid_dtor)
    return;

  call_func2(pexception_ctor, &e, &e_name);
  call_func2(pbad_typeid_ctor, &b, e_name);

  obj_locator = get_obj_locator(&e);
  if(obj_locator->signature!=1 && sizeof(void*)>sizeof(int))
    old_signature = TRUE;
  else
    old_signature = FALSE;

  /* dynamic_cast to void* */
  casted = p__RTCastToVoid(&e);
  ok (casted == (void*)&e, "failed cast to void\n");

  /* dynamic_cast up */
  ti = p__RTtypeid(&e);
  bti = p__RTtypeid(&b);

  casted = p__RTDynamicCast(&b, 0, NULL, ti, 0);
  if (casted)
  {
    /* New versions do not allow this conversion due to compiler changes */
    ok (casted == (void*)&b, "failed cast from bad_typeid to exception\n");
  }

  /* dynamic_cast down */
  casted = p__RTDynamicCast(&e, 0, NULL, bti, 0);
  ok (casted == NULL, "Cast succeeded\n");

  call_func1(pexception_dtor, &e);
  call_func1(pbad_typeid_dtor, &b);

  simple_class_sig0_rtti = simple_class_rtti;
  simple_class_sig0_rtti.object_locator.signature = 0;
  simple_class_sig0_rtti.base_descriptor[0].type_descriptor = RTTI_REF_SIG0(simple_class_sig0_rtti, type_info[0], base);
  simple_class_sig0_rtti.base_array.bases[0] = RTTI_REF_SIG0(simple_class_sig0_rtti, base_descriptor[0], base);
  simple_class_sig0_rtti.object_hierarchy.base_classes = RTTI_REF_SIG0(simple_class_sig0_rtti, base_array, base);
  simple_class_sig0_rtti.object_locator.type_descriptor = RTTI_REF_SIG0(simple_class_sig0_rtti, type_info[0], base);
  simple_class_sig0_rtti.object_locator.type_hierarchy = RTTI_REF_SIG0(simple_class_sig0_rtti, object_hierarchy, base);

  child_class_sig0_rtti = child_class_rtti;
  child_class_sig0_rtti.object_locator.signature = 0;
  child_class_sig0_rtti.base_descriptor[0].type_descriptor = RTTI_REF_SIG0(child_class_sig0_rtti, type_info[1], base);
  child_class_sig0_rtti.base_descriptor[1].type_descriptor = RTTI_REF_SIG0(child_class_sig0_rtti, type_info[0], base);
  child_class_sig0_rtti.base_array.bases[0] = RTTI_REF_SIG0(child_class_sig0_rtti, base_descriptor[0], base);
  child_class_sig0_rtti.base_array.bases[1] = RTTI_REF_SIG0(child_class_sig0_rtti, base_descriptor[1], base);
  child_class_sig0_rtti.object_hierarchy.base_classes = RTTI_REF_SIG0(child_class_sig0_rtti, base_array, base);
  child_class_sig0_rtti.object_locator.type_descriptor = RTTI_REF_SIG0(child_class_sig0_rtti, type_info[1], base);
  child_class_sig0_rtti.object_locator.type_hierarchy = RTTI_REF_SIG0(child_class_sig0_rtti, object_hierarchy, base);

  ti = p__RTtypeid(&simple_class_sig0);
  ok (ti && !strcmp(ti->mangled, "simple_class"), "incorrect rtti data\n");

  casted = p__RTCastToVoid(&simple_class_sig0);
  ok (casted == (void*)&simple_class_sig0, "failed cast to void\n");

  ti = p__RTtypeid(&child_class_sig0);
  ok (ti && !strcmp(ti->mangled, "child_class"), "incorrect rtti data\n");

  casted = p__RTCastToVoid(&child_class_sig0);
  ok (casted == (void*)&child_class_sig0, "failed cast to void\n");

  casted = p__RTDynamicCast(&child_class_sig0, 0, NULL, simple_class_sig0_rtti.type_info, 0);
  if(casted)
  {
      ok (casted == (char*)&child_class_sig0+8, "failed cast to simple_class (%p %p)\n", casted, &child_class_sig0);
  }

  casted = p__RTDynamicCast(&child_class_sig0, 0, &child_class_sig0_rtti.type_info[0], &child_class_sig0_rtti.type_info[1], 0);
  ok(casted == (char*)&child_class_sig0+4, "failed cast to child class (%p %p)\n", casted, &child_class_sig0);

  if(old_signature) {
      skip("signature==1 is not supported\n");
      return;
  }

  ti = p__RTtypeid(&simple_class);
  ok (ti && !strcmp(ti->mangled, "simple_class"), "incorrect rtti data\n");

  casted = p__RTCastToVoid(&simple_class);
  ok (casted == (void*)&simple_class, "failed cast to void\n");

  ti = p__RTtypeid(&child_class);
  ok (ti && !strcmp(ti->mangled, "child_class"), "incorrect rtti data\n");

  casted = p__RTCastToVoid(&child_class);
  ok (casted == (void*)&child_class, "failed cast to void\n");

  casted = p__RTDynamicCast(&child_class, 0, NULL, simple_class_rtti.type_info, 0);
  if(casted)
  {
    ok (casted == (char*)&child_class+8, "failed cast to simple_class (%p %p)\n", casted, &child_class);
  }

  casted = p__RTDynamicCast(&child_class, 0, &child_class_rtti.type_info[0], &child_class_rtti.type_info[1], 0);
  ok(casted == (char*)&child_class+4, "failed cast to child class (%p %p)\n", casted, &child_class);

  casted = p__RTDynamicCast(&virtual_base_class, 0, &virtual_base_class_rtti.type_info[0], &virtual_base_class_rtti.type_info[1], 0);
  ok(casted == &virtual_base_class.vbthis, "failed cast to child class (%p %p)\n", casted, &virtual_base_class);
}

struct _demangle {
    LPCSTR mangled;
    LPCSTR result;
    BOOL test_in_wine;
};

static void test_demangle_datatype(void)
{
    char * name;
    struct _demangle demangle[]={
        { "BlaBla"," ?? ::Bla", FALSE},
        { "ABVVec4@ref2@dice@@", "class dice::ref2::Vec4 const &", TRUE},
        { "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0H@@@",
            "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,7>", TRUE},
        { "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HO@@@",
            "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,126>",TRUE},
        { "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HOA@@@",
            "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,2016>",TRUE},
        { "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HOAA@@@",
            "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,32256>",TRUE},
        { "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$01@@@",
            "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$01@@@", FALSE},
        { "P8test@@AACXZ", "signed char (__cdecl test::*)(void)", TRUE},
        { "P8test@@BACXZ", "signed char (__cdecl test::*)(void)const ", TRUE},
    };
    int i, num_test = ARRAY_SIZE(demangle);

    for (i = 0; i < num_test; i++)
    {
        name = p__unDName(0, demangle[i].mangled, 0, malloc, free, 0x2800);
        todo_wine_if (!demangle[i].test_in_wine)
            ok(name != NULL && !strcmp(name,demangle[i].result), "Got name \"%s\" for %d\n", name, i);
        free(name);
    }
}

static void test_demangle(void)
{
    static struct {const char* in; const char* out; const char *broken; unsigned int flags;} test[] = {
/* 0 */ {"??0bad_alloc@std@@QAE@ABV01@@Z",
         "public: __thiscall std::bad_alloc::bad_alloc(class std::bad_alloc const &)",
         "public: __thiscall std::bad_alloc::bad_alloc(class bad_alloc::bad_alloc const &)"},
/* 1 */ {"??0bad_alloc@std@@QAE@PBD@Z",
         "public: __thiscall std::bad_alloc::bad_alloc(char const *)"},
/* 2 */ {"??0bad_cast@@AAE@PBQBD@Z",
         "private: __thiscall bad_cast::bad_cast(char const * const *)"},
/* 3 */ {"??0bad_cast@@QAE@ABQBD@Z",
         "public: __thiscall bad_cast::bad_cast(char const * const &)"},
/* 4 */ {"??0bad_cast@@QAE@ABV0@@Z",
         "public: __thiscall bad_cast::bad_cast(class bad_cast const &)"},
/* 5 */ {"??0bad_exception@std@@QAE@ABV01@@Z",
         "public: __thiscall std::bad_exception::bad_exception(class std::bad_exception const &)",
         "public: __thiscall std::bad_exception::bad_exception(class bad_exception::bad_exception const &)"},
/* 6 */ {"??0bad_exception@std@@QAE@PBD@Z",
         "public: __thiscall std::bad_exception::bad_exception(char const *)"},
/* 7 */ {"??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@ABV01@@Z",
         "public: __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> >(class std::basic_filebuf<char,struct std::char_traits<char> > const &)",
         "public: __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> >(class basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> > const &)"},
/* 8 */ {"??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@PAU_iobuf@@@Z",
         "public: __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> >(struct _iobuf *)"},
/* 9 */ {"??0?$basic_filebuf@DU?$char_traits@D@std@@@std@@QAE@W4_Uninitialized@1@@Z",
         "public: __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> >(enum std::_Uninitialized)",
         "public: __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::basic_filebuf<char,struct std::char_traits<char> >(enum basic_filebuf<char,struct std::char_traits<char> >::_Uninitialized)"},
/* 10 */ {"??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@ABV01@@Z",
          "public: __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >(class std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> > const &)",
          "public: __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >(class basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> > const &)"},
/* 11 */ {"??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@PAU_iobuf@@@Z",
          "public: __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >(struct _iobuf *)"},
/* 12 */ {"??0?$basic_filebuf@GU?$char_traits@G@std@@@std@@QAE@W4_Uninitialized@1@@Z",
          "public: __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >(enum std::_Uninitialized)",
          "public: __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >(enum basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::_Uninitialized)"},
/* 13 */ {"??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV01@@Z",
          "public: __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >(class std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> > const &)",
          "public: __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >(class basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 14 */ {"??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@ABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@1@H@Z",
          "public: __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &,int)",
          "public: __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >(class basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &,int)"},
/* 15 */ {"??0?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAE@H@Z",
          "public: __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >(int)"},
/* 16 */ {"??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV01@@Z",
          "public: __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >(class std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)",
          "public: __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >(class basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 17 */ {"??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@ABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@1@H@Z",
          "public: __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,int)",
          "public: __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >(class basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &,int)"},
/* 18 */ {"??0?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAE@H@Z",
          "public: __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >(int)"},
/* 19 */ {"??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z",
          "public: __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >(class std::_Locinfo const &,unsigned int)",
          "public: __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >(class num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::_Locinfo const &,unsigned int)"},
/* 20 */ {"??0?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@QAE@I@Z",
          "public: __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >(unsigned int)"},
/* 21 */ {"??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@ABV_Locinfo@1@I@Z",
          "public: __thiscall std::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >(class std::_Locinfo const &,unsigned int)",
          "public: __thiscall std::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >(class num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >::_Locinfo const &,unsigned int)"},
/* 22 */ {"??0?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@QAE@I@Z", "public: __thiscall std::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >(unsigned int)"},
/* 23 */ {"??0streambuf@@QAE@ABV0@@Z", "public: __thiscall streambuf::streambuf(class streambuf const &)"},
/* 24 */ {"??0strstreambuf@@QAE@ABV0@@Z", "public: __thiscall strstreambuf::strstreambuf(class strstreambuf const &)"},
/* 25 */ {"??0strstreambuf@@QAE@H@Z", "public: __thiscall strstreambuf::strstreambuf(int)"},
/* 26 */ {"??0strstreambuf@@QAE@Q6APAXJ@ZS6AXPAX@Z@Z", "public: __thiscall strstreambuf::strstreambuf(void * (__cdecl*const)(long),void (__cdecl*const volatile)(void *))"},
/* 27 */ {"??0strstreambuf@@QAE@PADH0@Z", "public: __thiscall strstreambuf::strstreambuf(char *,int,char *)"},
/* 28 */ {"??0strstreambuf@@QAE@PAEH0@Z", "public: __thiscall strstreambuf::strstreambuf(unsigned char *,int,unsigned char *)"},
/* 29 */ {"??0strstreambuf@@QAE@XZ", "public: __thiscall strstreambuf::strstreambuf(void)"},
/* 30 */ {"??1__non_rtti_object@std@@UAE@XZ", "public: virtual __thiscall std::__non_rtti_object::~__non_rtti_object(void)"},
/* 31 */ {"??1__non_rtti_object@@UAE@XZ", "public: virtual __thiscall __non_rtti_object::~__non_rtti_object(void)"},
/* 32 */ {"??1?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@UAE@XZ", "public: virtual __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::~num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >(void)"},
/* 33 */ {"??1?$num_get@GV?$istreambuf_iterator@GU?$char_traits@G@std@@@std@@@std@@UAE@XZ", "public: virtual __thiscall std::num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >::~num_get<unsigned short,class std::istreambuf_iterator<unsigned short,struct std::char_traits<unsigned short> > >(void)"},
/* 34 */ {"??4istream_withassign@@QAEAAV0@ABV0@@Z", "public: class istream_withassign & __thiscall istream_withassign::operator=(class istream_withassign const &)"},
/* 35 */ {"??4istream_withassign@@QAEAAVistream@@ABV1@@Z", "public: class istream & __thiscall istream_withassign::operator=(class istream const &)"},
/* 36 */ {"??4istream_withassign@@QAEAAVistream@@PAVstreambuf@@@Z", "public: class istream & __thiscall istream_withassign::operator=(class streambuf *)"},
/* 37 */ {"??5std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAC@Z", "class std::basic_istream<char,struct std::char_traits<char> > & __cdecl std::operator>>(class std::basic_istream<char,struct std::char_traits<char> > &,signed char &)"},
/* 38 */ {"??5std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAD@Z", "class std::basic_istream<char,struct std::char_traits<char> > & __cdecl std::operator>>(class std::basic_istream<char,struct std::char_traits<char> > &,char &)"},
/* 39 */ {"??5std@@YAAAV?$basic_istream@DU?$char_traits@D@std@@@0@AAV10@AAE@Z", "class std::basic_istream<char,struct std::char_traits<char> > & __cdecl std::operator>>(class std::basic_istream<char,struct std::char_traits<char> > &,unsigned char &)"},
/* 40 */ {"??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@P6AAAVios_base@1@AAV21@@Z@Z", "public: class std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> > & __thiscall std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> >::operator<<(class std::ios_base & (__cdecl*)(class std::ios_base &))"},
/* 41 */ {"??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PAV?$basic_streambuf@GU?$char_traits@G@std@@@1@@Z", "public: class std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> > & __thiscall std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> >::operator<<(class std::basic_streambuf<unsigned short,struct std::char_traits<unsigned short> > *)"},
/* 42 */ {"??6?$basic_ostream@GU?$char_traits@G@std@@@std@@QAEAAV01@PBX@Z", "public: class std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> > & __thiscall std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> >::operator<<(void const *)"},
/* 43 */ {"??_8?$basic_fstream@DU?$char_traits@D@std@@@std@@7B?$basic_ostream@DU?$char_traits@D@std@@@1@@", "const std::basic_fstream<char,struct std::char_traits<char> >::`vbtable'{for `std::basic_ostream<char,struct std::char_traits<char> >'}"},
/* 44 */ {"??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_istream@GU?$char_traits@G@std@@@1@@", "const std::basic_fstream<unsigned short,struct std::char_traits<unsigned short> >::`vbtable'{for `std::basic_istream<unsigned short,struct std::char_traits<unsigned short> >'}"},
/* 45 */ {"??_8?$basic_fstream@GU?$char_traits@G@std@@@std@@7B?$basic_ostream@GU?$char_traits@G@std@@@1@@", "const std::basic_fstream<unsigned short,struct std::char_traits<unsigned short> >::`vbtable'{for `std::basic_ostream<unsigned short,struct std::char_traits<unsigned short> >'}"},
/* 46 */ {"??9std@@YA_NPBDABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@0@@Z", "bool __cdecl std::operator!=(char const *,class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 47 */ {"??9std@@YA_NPBGABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@0@@Z", "bool __cdecl std::operator!=(unsigned short const *,class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 48 */ {"??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEAADI@Z", "public: char & __thiscall std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >::operator[](unsigned int)"},
/* 49 */ {"??A?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBEABDI@Z", "public: char const & __thiscall std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> >::operator[](unsigned int)const "},
/* 50 */ {"??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEAAGI@Z", "public: unsigned short & __thiscall std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::operator[](unsigned int)"},
/* 51 */ {"??A?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBEABGI@Z", "public: unsigned short const & __thiscall std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::operator[](unsigned int)const "},
/* 52 */ {"?abs@std@@YAMABV?$complex@M@1@@Z", "float __cdecl std::abs(class std::complex<float> const &)"},
/* 53 */ {"?abs@std@@YANABV?$complex@N@1@@Z", "double __cdecl std::abs(class std::complex<double> const &)"},
/* 54 */ {"?abs@std@@YAOABV?$complex@O@1@@Z", "long double __cdecl std::abs(class std::complex<long double> const &)"},
/* 55 */ {"?cin@std@@3V?$basic_istream@DU?$char_traits@D@std@@@1@A", "class std::basic_istream<char,struct std::char_traits<char> > std::cin"},
/* 56 */ {"?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAG@Z", "protected: virtual class std::istreambuf_iterator<char,struct std::char_traits<char> > __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::do_get(class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::ios_base &,int &,unsigned short &)const "},
/* 57 */ {"?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAI@Z", "protected: virtual class std::istreambuf_iterator<char,struct std::char_traits<char> > __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::do_get(class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::ios_base &,int &,unsigned int &)const "},
/* 58 */ {"?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAJ@Z", "protected: virtual class std::istreambuf_iterator<char,struct std::char_traits<char> > __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::do_get(class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::ios_base &,int &,long &)const "},
/* 59 */ {"?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAK@Z", "protected: virtual class std::istreambuf_iterator<char,struct std::char_traits<char> > __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::do_get(class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::ios_base &,int &,unsigned long &)const "},
/* 60 */ {"?do_get@?$num_get@DV?$istreambuf_iterator@DU?$char_traits@D@std@@@std@@@std@@MBE?AV?$istreambuf_iterator@DU?$char_traits@D@std@@@2@V32@0AAVios_base@2@AAHAAM@Z", "protected: virtual class std::istreambuf_iterator<char,struct std::char_traits<char> > __thiscall std::num_get<char,class std::istreambuf_iterator<char,struct std::char_traits<char> > >::do_get(class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::istreambuf_iterator<char,struct std::char_traits<char> >,class std::ios_base &,int &,float &)const "},
/* 61 */ {"?_query_new_handler@@YAR6AHI@ZXZ", "int (__cdecl*__cdecl _query_new_handler(void))(unsigned int)"},
/* 62 */ {"?register_callback@ios_base@std@@QAEXP6AXW4event@12@AAV12@H@ZH@Z", "public: void __thiscall std::ios_base::register_callback(void (__cdecl*)(enum std::ios_base::event,class std::ios_base &,int),int)"},
/* 63 */ {"?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z", "public: class std::basic_istream<char,struct std::char_traits<char> > & __thiscall std::basic_istream<char,struct std::char_traits<char> >::seekg(long,enum std::ios_base::seekdir)"},
/* 64 */ {"?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z", "public: class std::basic_istream<char,struct std::char_traits<char> > & __thiscall std::basic_istream<char,struct std::char_traits<char> >::seekg(class std::fpos<int>)"},
/* 65 */ {"?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@JW4seekdir@ios_base@2@@Z", "public: class std::basic_istream<unsigned short,struct std::char_traits<unsigned short> > & __thiscall std::basic_istream<unsigned short,struct std::char_traits<unsigned short> >::seekg(long,enum std::ios_base::seekdir)"},
/* 66 */ {"?seekg@?$basic_istream@GU?$char_traits@G@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z", "public: class std::basic_istream<unsigned short,struct std::char_traits<unsigned short> > & __thiscall std::basic_istream<unsigned short,struct std::char_traits<unsigned short> >::seekg(class std::fpos<int>)"},
/* 67 */ {"?seekoff@?$basic_filebuf@DU?$char_traits@D@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z", "protected: virtual class std::fpos<int> __thiscall std::basic_filebuf<char,struct std::char_traits<char> >::seekoff(long,enum std::ios_base::seekdir,int)"},
/* 68 */ {"?seekoff@?$basic_filebuf@GU?$char_traits@G@std@@@std@@MAE?AV?$fpos@H@2@JW4seekdir@ios_base@2@H@Z", "protected: virtual class std::fpos<int> __thiscall std::basic_filebuf<unsigned short,struct std::char_traits<unsigned short> >::seekoff(long,enum std::ios_base::seekdir,int)"},
/* 69 */ {"?set_new_handler@@YAP6AXXZP6AXXZ@Z", "void (__cdecl*__cdecl set_new_handler(void (__cdecl*)(void)))(void)"},
/* 70 */ {"?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z", "public: void __thiscall std::basic_istringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 71 */ {"?str@?$basic_istringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ", "public: class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __thiscall std::basic_istringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(void)const "},
/* 72 */ {"?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z", "public: void __thiscall std::basic_istringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 73 */ {"?str@?$basic_istringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ", "public: class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > __thiscall std::basic_istringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(void)const "},
/* 74 */ {"?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z", "public: void __thiscall std::basic_ostringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 75 */ {"?str@?$basic_ostringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ", "public: class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __thiscall std::basic_ostringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(void)const "},
/* 76 */ {"?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z", "public: void __thiscall std::basic_ostringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 77 */ {"?str@?$basic_ostringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ", "public: class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > __thiscall std::basic_ostringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(void)const "},
/* 78 */ {"?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z", "public: void __thiscall std::basic_stringbuf<char,struct std::char_traits<char>,class std::allocator<char> >::str(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 79 */ {"?str@?$basic_stringbuf@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ", "public: class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __thiscall std::basic_stringbuf<char,struct std::char_traits<char>,class std::allocator<char> >::str(void)const "},
/* 80 */ {"?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z", "public: void __thiscall std::basic_stringbuf<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 81 */ {"?str@?$basic_stringbuf@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ", "public: class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > __thiscall std::basic_stringbuf<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(void)const "},
/* 82 */ {"?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QAEXABV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@@Z", "public: void __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)"},
/* 83 */ {"?str@?$basic_stringstream@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@QBE?AV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@2@XZ", "public: class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > __thiscall std::basic_stringstream<char,struct std::char_traits<char>,class std::allocator<char> >::str(void)const "},
/* 84 */ {"?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QAEXABV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@@Z", "public: void __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > const &)"},
/* 85 */ {"?str@?$basic_stringstream@GU?$char_traits@G@std@@V?$allocator@G@2@@std@@QBE?AV?$basic_string@GU?$char_traits@G@std@@V?$allocator@G@2@@2@XZ", "public: class std::basic_string<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> > __thiscall std::basic_stringstream<unsigned short,struct std::char_traits<unsigned short>,class std::allocator<unsigned short> >::str(void)const "},
/* 86 */ {"?_Sync@ios_base@std@@0_NA", "private: static bool std::ios_base::_Sync"},
/* 87 */ {"??_U@YAPAXI@Z", "void * __cdecl operator new[](unsigned int)"},
/* 88 */ {"??_V@YAXPAX@Z", "void __cdecl operator delete[](void *)"},
/* 89 */ {"??X?$_Complex_base@M@std@@QAEAAV01@ABM@Z", "public: class std::_Complex_base<float> & __thiscall std::_Complex_base<float>::operator*=(float const &)"},
/* 90 */ {"??Xstd@@YAAAV?$complex@M@0@AAV10@ABV10@@Z", "class std::complex<float> & __cdecl std::operator*=(class std::complex<float> &,class std::complex<float> const &)"},
/* 91 */ {"?aaa@@YAHAAUbbb@@@Z", "int __cdecl aaa(struct bbb &)"},
/* 92 */ {"?aaa@@YAHBAUbbb@@@Z", "int __cdecl aaa(struct bbb & volatile)"},
/* 93 */ {"?aaa@@YAHPAUbbb@@@Z", "int __cdecl aaa(struct bbb *)"},
/* 94 */ {"?aaa@@YAHQAUbbb@@@Z", "int __cdecl aaa(struct bbb * const)"},
/* 95 */ {"?aaa@@YAHRAUbbb@@@Z", "int __cdecl aaa(struct bbb * volatile)"},
/* 96 */ {"?aaa@@YAHSAUbbb@@@Z", "int __cdecl aaa(struct bbb * const volatile)"},
/* 97 */ {"??0aa.a@@QAE@XZ", "??0aa.a@@QAE@XZ"},
/* 98 */ {"??0aa$_3a@@QAE@XZ", "public: __thiscall aa$_3a::aa$_3a(void)"},
/* 99 */ {"??2?$aaa@AAUbbb@@AAUccc@@AAU2@@ddd@1eee@2@QAEHXZ", "public: int __thiscall eee::eee::ddd::ddd::aaa<struct bbb &,struct ccc &,struct ccc &>::operator new(void)"},
/* 100 */ {"?pSW@@3P6GHKPAX0PAU_tagSTACKFRAME@@0P6GH0K0KPAK@ZP6GPAX0K@ZP6GK0K@ZP6GK00PAU_tagADDRESS@@@Z@ZA", "int (__stdcall* pSW)(unsigned long,void *,void *,struct _tagSTACKFRAME *,void *,int (__stdcall*)(void *,unsigned long,void *,unsigned long,unsigned long *),void * (__stdcall*)(void *,unsigned long),unsigned long (__stdcall*)(void *,unsigned long),unsigned long (__stdcall*)(void *,void *,struct _tagADDRESS *))"},
/* 101 */ {"?$_aaa@Vbbb@@", "_aaa<class bbb>"},
/* 102 */ {"?$aaa@Vbbb@ccc@@Vddd@2@", "aaa<class ccc::bbb,class ccc::ddd>"},
/* 103 */ { "??0?$Foo@P6GHPAX0@Z@@QAE@PAD@Z", "public: __thiscall Foo<int (__stdcall*)(void *,void *)>::Foo<int (__stdcall*)(void *,void *)>(char *)"},
/* 104 */ { "??0?$Foo@P6GHPAX0@Z@@QAE@PAD@Z", "__thiscall Foo<int (__stdcall*)(void *,void *)>::Foo<int (__stdcall*)(void *,void *)>(char *)", NULL, 0x880},
/* 105 */ { "?Qux@Bar@@0PAP6AHPAV1@AAH1PAH@ZA", "private: static int (__cdecl** Bar::Qux)(class Bar *,int &,int &,int *)" },
/* 106 */ { "?Qux@Bar@@0PAP6AHPAV1@AAH1PAH@ZA", "Bar::Qux", NULL, 0x1800},
/* 107 */ {"?$AAA@$DBAB@", "AAA<`template-parameter257'>"},
/* 108 */ {"?$AAA@?C@", "AAA<`template-parameter-2'>"},
/* 109 */ {"?$AAA@PAUBBB@@", "AAA<struct BBB *>"},
/* 110 */ {"??$ccccc@PAVaaa@@@bar@bb@foo@@DGPAV0@PAV0@PAVee@@IPAPAVaaa@@1@Z",
           "private: static class bar * __stdcall foo::bb::bar::ccccc<class aaa *>(class bar *,class ee *,unsigned int,class aaa * *,class ee *)",
           "??$ccccc@PAVaaa@@@bar@bb@foo@@DGPAV0@PAV0@PAVee@@IPAPAVaaa@@1@Z"},
/* 111 */ {"?f@T@@QAEHQCY1BE@BO@D@Z", "public: int __thiscall T::f(char (volatile * const)[20][30])"},
/* 112 */ {"?f@T@@QAEHQAY2BE@BO@CI@D@Z", "public: int __thiscall T::f(char (* const)[20][30][40])"},
/* 113 */ {"?f@T@@QAEHQAY1BE@BO@$$CBD@Z", "public: int __thiscall T::f(char const (* const)[20][30])"},
/* 114 */ {"??0?$Foo@U?$vector_c@H$00$01$0?1$0A@$0A@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@$0HPPPPPPP@@mpl@boost@@@@QAE@XZ",
           "public: __thiscall Foo<struct boost::mpl::vector_c<int,1,2,-2,0,0,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647> >::Foo<struct boost::mpl::vector_c<int,1,2,-2,0,0,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647,2147483647> >(void)"},
/* 115 */ {"?swprintf@@YAHPAGIPBGZZ", "int __cdecl swprintf(unsigned short *,unsigned int,unsigned short const *,...)"},
/* 116 */ {"?vswprintf@@YAHPAGIPBGPAD@Z", "int __cdecl vswprintf(unsigned short *,unsigned int,unsigned short const *,char *)"},
/* 117 */ {"?vswprintf@@YAHPA_WIPB_WPAD@Z", "int __cdecl vswprintf(wchar_t *,unsigned int,wchar_t const *,char *)"},
/* 118 */ {"?swprintf@@YAHPA_WIPB_WZZ", "int __cdecl swprintf(wchar_t *,unsigned int,wchar_t const *,...)"},
/* 119 */ {"??Xstd@@YAAEAV?$complex@M@0@AEAV10@AEBV10@@Z", "class std::complex<float> & __ptr64 __cdecl std::operator*=(class std::complex<float> & __ptr64,class std::complex<float> const & __ptr64)"},
/* 120 */ {"?_Doraise@bad_cast@std@@MEBAXXZ", "protected: virtual void __cdecl std::bad_cast::_Doraise(void)const __ptr64"},
/* 121 */ {"??$?DM@std@@YA?AV?$complex@M@0@ABMABV10@@Z",
           "class std::complex<float> __cdecl std::operator*<float>(float const &,class std::complex<float> const &)",
           "??$?DM@std@@YA?AV?$complex@M@0@ABMABV10@@Z"},
/* 122 */ {"?_R2@?BN@???$_Fabs@N@std@@YANAEBV?$complex@N@1@PEAH@Z@4NB",
           "double const `double __cdecl std::_Fabs<double>(class std::complex<double> const & __ptr64,int * __ptr64)'::`29'::_R2",
           "?_R2@?BN@???$_Fabs@N@std@@YANAEBV?$complex@N@1@PEAH@Z@4NB"},
/* 123 */ {"?vtordisp_thunk@std@@$4PPPPPPPM@3EAA_NXZ",
           "[thunk]:public: virtual bool __cdecl std::vtordisp_thunk`vtordisp{4294967292,4}' (void) __ptr64",
           "[thunk]:public: virtual bool __cdecl std::vtordisp_thunk`vtordisp{-4,4}' (void) __ptr64"},
/* 124 */ {"??_9CView@@$BBII@AE",
           "[thunk]: __thiscall CView::`vcall'{392,{flat}}' }'",
           "[thunk]: __thiscall CView::`vcall'{392,{flat}}' "},
/* 125 */ {"?_dispatch@_impl_Engine@SalomeApp@@$R4CE@BA@PPPPPPPM@7AE_NAAVomniCallHandle@@@Z",
           "[thunk]:public: virtual bool __thiscall SalomeApp::_impl_Engine::_dispatch`vtordispex{36,16,4294967292,8}' (class omniCallHandle &)",
           "?_dispatch@_impl_Engine@SalomeApp@@$R4CE@BA@PPPPPPPM@7AE_NAAVomniCallHandle@@@Z"},
/* 126 */ {"?_Doraise@bad_cast@std@@MEBAXXZ", "protected: virtual void __cdecl std::bad_cast::_Doraise(void)", NULL, 0x60},
/* 127 */ {"??Xstd@@YAAEAV?$complex@M@0@AEAV10@AEBV10@@Z", "class std::complex<float> & ptr64 cdecl std::operator*=(class std::complex<float> & ptr64,class std::complex<float> const & ptr64)", NULL, 1},
/* 128 */ {"??Xstd@@YAAEAV?$complex@M@0@AEAV10@AEBV10@@Z",
           "class std::complex<float> & std::operator*=(class std::complex<float> &,class std::complex<float> const &)",
           "??Xstd@@YAAEAV?$complex@M@0@AEAV10@AEBV10@@Z", 2},
/* 129 */ {"??$run@XVTask_Render_Preview@@@QtConcurrent@@YA?AV?$QFuture@X@@PEAVTask_Render_Preview@@P82@EAAXXZ@Z",
           "class QFuture<void> __cdecl QtConcurrent::run<void,class Task_Render_Preview>(class Task_Render_Preview * __ptr64,void (__cdecl Task_Render_Preview::*)(void) __ptr64)",
           "??$run@XVTask_Render_Preview@@@QtConcurrent@@YA?AV?$QFuture@X@@PEAVTask_Render_Preview@@P82@EAAXXZ@Z"},
/* 130 */ {"??_E?$TStrArray@$$BY0BAA@D$0BA@@@UAEPAXI@Z",
           "public: virtual void * __thiscall TStrArray<char [256],16>::`vector deleting destructor'(unsigned int)"},
/* 131 */ {"??_R0?AVCC@DD@@@8", "class DD::CC `RTTI Type Descriptor'"},
/* 132 */ {"??$meth@FD@DD@CC@@QAE_NK@Z", "public: bool __thiscall CC::DD::meth<short,char>(unsigned long)"},
/* 133 */ {"?func@@YAXPIFAH@Z", "void __cdecl func(int __unaligned * __restrict)"},
/* 135 */ {"?x@@3PAY02HA", "int (* x)[3]"},
/* 136 */ {"?Qux@Bar@@0PAPAP6AHPAV1@AAH1PAH@ZA",
           "private: static int (__cdecl** * Bar::Qux)(class Bar *,int &,int &,int *)"}, /* variation of 105: note white space handling! */
/* 137 */ {"?x@@3PAW4myenum@@A", "enum myenum * x"},
/* 138 */ {"?pfunc@@3PAY0E@P6AXF@ZA", "void (__cdecl*(* pfunc)[4])(short)"},
/* 139 */ {"??$?0AEAVzzz@BB4@AA@@AEAV012@$0A@@?$pair@Vzzz@BB4@AA@@V123@@std@@QEAA@AEAVzzz@BB4@AA@@0@Z",
           "public: __cdecl std::pair<class AA::BB4::zzz,class AA::BB4::zzz>::pair<class AA::BB4::zzz,class AA::BB4::zzz><class AA::BB4::zzz & __ptr64,class AA::BB4::zzz & __ptr64,0>(class AA::BB4::zzz & __ptr64,class AA::BB4::zzz & __ptr64) __ptr64"},
/* 140 */ {"??$?BH@?$foo@N@@QEAAHXZ", "public: __cdecl foo<double>::operator<int> int(void) __ptr64"},
/* 141 */ {"??Bcastop@@QAEHXZ", "public: __thiscall castop::operator int(void)"},
/* 142 */ {"??Bcastop@@QAE?BHXZ", "public: __thiscall castop::operator int const (void)"},
/* 143 */ {"?pfield@@3PTAA@@DT1@", "char const volatile AA::* const volatile pfield"},
/* 144 */ {"?ptititi1@@3PEQtititi@@IEQ1@", "unsigned int tititi::* __ptr64 __ptr64 ptititi1"},
/* 145 */ {"?ptititi2@@3PERtititi@@IER1@", "unsigned int const tititi::* __ptr64 const __ptr64 ptititi2"},
/* 146 */ {"?ptititi3@@3PEStititi@@IES1@", "unsigned int volatile tititi::* __ptr64 volatile __ptr64 ptititi3"},
/* 147 */ {"?ptititi4@@3PETtititi@@IET1@", "unsigned int const volatile tititi::* __ptr64 const volatile __ptr64 ptititi4"},
/* 148 */ {"?ptititi4v@@3RETtititi@@IET1@", "unsigned int const volatile tititi::* __ptr64 const volatile __ptr64 ptititi4v"},
/* 149 */ {"?meth@AAA@@QFCEXXZ", "public: void __thiscall AAA::meth(void)volatile __unaligned "},
/* 150 */ {"?RegisterModuleUninitializer@<CrtImplementationDetails>@@YAXP$AAVEventHandler@System@@@Z",
           "void __cdecl <CrtImplementationDetails>::RegisterModuleUninitializer(class System::EventHandler ^)"},
/* 151 */ {"?RegisterModuleUninitializer@<CrtImplementationDetails>@@YAXBE$AAVEventHandler@System@@@Z",
           "void __cdecl <CrtImplementationDetails>::RegisterModuleUninitializer(class System::EventHandler % __ptr64 volatile)"},
/* 152 */ {"??$forward@AEAUFFIValue@?1??call@FFIFunctionBinder@@CAHPEAUlua_State@@@Z@@std@@YAAEAUFFIValue@?1??call@"
           "FFIFunctionBinder@@CAHPEAUxlua_State@@@Z@AEAU1?1??23@CAH0@Z@@Z",
           "struct `private: static int __cdecl FFIFunctionBinder::call(struct xlua_State * __ptr64)'::`2'::FFIValue & "
           "__ptr64 __cdecl std::forward<struct `private: static int __cdecl FFIFunctionBinder::call(struct lua_State "
           "* __ptr64)'::`2'::FFIValue & __ptr64>(struct `private: static int __cdecl FFIFunctionBinder::call(struct "
           "xlua_State * __ptr64)'::`2'::FFIValue & __ptr64)"},
/* 153 */ {"?$AAA@XX", "AAA<void,void>"},
/* 154 */ {"?$AAA@", "AAA<>"},
    };
    int i;
    char* name;

    for (i = 0; i < ARRAY_SIZE(test); i++)
    {
	name = p__unDName(0, test[i].in, 0, malloc, free, test[i].flags);
        ok(name != NULL, "%u: unDName failed\n", i);
        if (!name) continue;
        ok( !strcmp(test[i].out, name) ||
            broken(test[i].broken && !strcmp(test[i].broken, name)),
           "%u: Got name \"%s\"\n", i, name );
        ok( !strcmp(test[i].out, name) ||
            broken(test[i].broken && !strcmp(test[i].broken, name)),
           "%u: Expected \"%s\"\n", i, test[i].out );
        free(name);
    }
}

START_TEST(cpp)
{
  if (!InitFunctionPtrs())
    return;

  test_exception();
  test_bad_typeid();
  test_bad_cast();
  test___non_rtti_object();
  test_type_info();
  test_rtti();
  test_demangle_datatype();
  test_demangle();
}
