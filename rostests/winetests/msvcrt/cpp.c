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
 *
 * NOTES
 * This tests is only valid for ix86 platforms, on others it's a no-op.
 * Some tests cannot be checked with ok(), for example the dtors. We simply
 * call them to ensure we don't crash ;-)
 *
 * If we build this test with VC++ in debug mode, we will fail in _chkstk()
 * or at program exit malloc() checking if these methods haven't been
 * implemented correctly (they have).
 *
 * Tested with a range of native msvcrt's from v4 -> v7.
 */
#include "wine/test.h"
#include "winbase.h"
#include "winnt.h"

#ifndef __i386__
/* Skip these tests for non x86 platforms */
START_TEST(cpp)
{
}
#else

typedef struct __exception
{
  void *vtable;
  char *name;
  int   do_free;
} exception;

typedef struct __type_info
{
  void *vtable;
  char *name;
  char  mangled[16];
} type_info;

/* Function pointers. We need to use these to call these funcs as __thiscall */
static HMODULE hMsvcrt;

static void* (__cdecl *poperator_new)(unsigned int);
static void  (__cdecl *poperator_delete)(void*);
static void* (__cdecl *pmalloc)(unsigned int);
static void  (__cdecl *pfree)(void*);

/* exception */
static void (WINAPI *pexception_ctor)(exception*,LPCSTR*);
static void (WINAPI *pexception_copy_ctor)(exception*,exception*);
static void (WINAPI *pexception_default_ctor)(exception*);
static void (WINAPI *pexception_dtor)(exception*);
static exception* (WINAPI *pexception_opequals)(exception*,exception*);
static char* (WINAPI *pexception_what)(exception*);
static void* (WINAPI *pexception_vtable)(exception*);
static void (WINAPI *pexception_vector_dtor)(exception*,unsigned int);
static void (WINAPI *pexception_scalar_dtor)(exception*,unsigned int);

/* bad_typeid */
static void (WINAPI *pbad_typeid_ctor)(exception*,LPCSTR);
static void (WINAPI *pbad_typeid_ctor_closure)(exception*);
static void (WINAPI *pbad_typeid_copy_ctor)(exception*,exception*);
static void (WINAPI *pbad_typeid_dtor)(exception*);
static exception* (WINAPI *pbad_typeid_opequals)(exception*,exception*);
static char* (WINAPI *pbad_typeid_what)(exception*);
static void* (WINAPI *pbad_typeid_vtable)(exception*);
static void (WINAPI *pbad_typeid_vector_dtor)(exception*,unsigned int);
static void (WINAPI *pbad_typeid_scalar_dtor)(exception*,unsigned int);

/* bad_cast */
static void (WINAPI *pbad_cast_ctor)(exception*,LPCSTR*);
static void (WINAPI *pbad_cast_ctor2)(exception*,LPCSTR);
static void (WINAPI *pbad_cast_ctor_closure)(exception*);
static void (WINAPI *pbad_cast_copy_ctor)(exception*,exception*);
static void (WINAPI *pbad_cast_dtor)(exception*);
static exception* (WINAPI *pbad_cast_opequals)(exception*,exception*);
static char* (WINAPI *pbad_cast_what)(exception*);
static void* (WINAPI *pbad_cast_vtable)(exception*);
static void (WINAPI *pbad_cast_vector_dtor)(exception*,unsigned int);
static void (WINAPI *pbad_cast_scalar_dtor)(exception*,unsigned int);

/* __non_rtti_object */
static void (WINAPI *p__non_rtti_object_ctor)(exception*,LPCSTR);
static void (WINAPI *p__non_rtti_object_copy_ctor)(exception*,exception*);
static void (WINAPI *p__non_rtti_object_dtor)(exception*);
static exception* (WINAPI *p__non_rtti_object_opequals)(exception*,exception*);
static char* (WINAPI *p__non_rtti_object_what)(exception*);
static void* (WINAPI *p__non_rtti_object_vtable)(exception*);
static void (WINAPI *p__non_rtti_object_vector_dtor)(exception*,unsigned int);
static void (WINAPI *p__non_rtti_object_scalar_dtor)(exception*,unsigned int);

/* type_info */
static void  (WINAPI *ptype_info_dtor)(type_info*);
static char* (WINAPI *ptype_info_raw_name)(type_info*);
static char* (WINAPI *ptype_info_name)(type_info*);
static int   (WINAPI *ptype_info_before)(type_info*,type_info*);
static int   (WINAPI *ptype_info_opequals_equals)(type_info*,type_info*);
static int   (WINAPI *ptype_info_opnot_equals)(type_info*,type_info*);

/* RTTI */
static type_info* (__cdecl *p__RTtypeid)(void*);
static void* (__cdecl *p__RTCastToVoid)(void*);
static void* (__cdecl *p__RTDynamicCast)(void*,int,void*,void*,int);

/*Demangle*/
static char* (__cdecl *p__unDName)(char*,const char*,int,void*,void*,unsigned short int);


/* _very_ early native versions have serious RTTI bugs, so we check */
static void* bAncientVersion;

/* Emulate a __thiscall */
#ifdef _MSC_VER
static inline void* do_call_func1(void *func, void *_this)
{
  volatile void* retval = 0;
  __asm
  {
    push ecx
    mov ecx, _this
    call func
    mov retval, eax
    pop ecx
  }
  return (void*)retval;
}

static inline void* do_call_func2(void *func, void *_this, const void* arg)
{
  volatile void* retval = 0;
  __asm
  {
    push ecx
    push arg
    mov ecx, _this
    call func
    mov retval, eax
    pop ecx
  }
  return (void*)retval;
}
#else
static void* do_call_func1(void *func, void *_this)
{
  void *ret, *dummy;
  __asm__ __volatile__ ("call *%2"
                        : "=a" (ret), "=c" (dummy)
                        : "g" (func), "1" (_this)
                        : "edx", "memory" );
  return ret;
}
static void* do_call_func2(void *func, void *_this, const void* arg)
{
  void *ret, *dummy;
  __asm__ __volatile__ ("pushl %3\n\tcall *%2"
                        : "=a" (ret), "=c" (dummy)
                        : "r" (func), "r" (arg), "1" (_this)
                        : "edx", "memory" );
  return ret;
}
#endif

#define call_func1(x,y)   do_call_func1((void*)x,(void*)y)
#define call_func2(x,y,z) do_call_func2((void*)x,(void*)y,(const void*)z)

/* Some exports are only available in later versions */
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static void InitFunctionPtrs(void)
{
  hMsvcrt = GetModuleHandleA("msvcrt.dll");
  if (!hMsvcrt)
    hMsvcrt = GetModuleHandleA("msvcrtd.dll");
  ok(hMsvcrt != 0, "GetModuleHandleA failed\n");
  if (hMsvcrt)
  {
    SETNOFAIL(poperator_new, "??_U@YAPAXI@Z");
    SETNOFAIL(poperator_delete, "??_V@YAXPAX@Z");
    SET(pmalloc, "malloc");
    SET(pfree, "free");

    if (!poperator_new)
      poperator_new = pmalloc;
    if (!poperator_delete)
      poperator_delete = pfree;

    SET(pexception_ctor, "??0exception@@QAE@ABQBD@Z");
    SET(pexception_copy_ctor, "??0exception@@QAE@ABV0@@Z");
    SET(pexception_default_ctor, "??0exception@@QAE@XZ");
    SET(pexception_dtor, "??1exception@@UAE@XZ");
    SET(pexception_opequals, "??4exception@@QAEAAV0@ABV0@@Z");
    SET(pexception_what, "?what@exception@@UBEPBDXZ");
    SET(pexception_vtable, "??_7exception@@6B@");
    SET(pexception_vector_dtor, "??_Eexception@@UAEPAXI@Z");
    SET(pexception_scalar_dtor, "??_Gexception@@UAEPAXI@Z");

    SET(pbad_typeid_ctor, "??0bad_typeid@@QAE@PBD@Z");
    SETNOFAIL(pbad_typeid_ctor_closure, "??_Fbad_typeid@@QAEXXZ");
    SET(pbad_typeid_copy_ctor, "??0bad_typeid@@QAE@ABV0@@Z");
    SET(pbad_typeid_dtor, "??1bad_typeid@@UAE@XZ");
    SET(pbad_typeid_opequals, "??4bad_typeid@@QAEAAV0@ABV0@@Z");
    SET(pbad_typeid_what, "?what@exception@@UBEPBDXZ");
    SET(pbad_typeid_vtable, "??_7bad_typeid@@6B@");
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
    SET(pbad_cast_vtable, "??_7bad_cast@@6B@");
    SET(pbad_cast_vector_dtor, "??_Ebad_cast@@UAEPAXI@Z");
    SET(pbad_cast_scalar_dtor, "??_Gbad_cast@@UAEPAXI@Z");

    SET(p__non_rtti_object_ctor, "??0__non_rtti_object@@QAE@PBD@Z");
    SET(p__non_rtti_object_copy_ctor, "??0__non_rtti_object@@QAE@ABV0@@Z");
    SET(p__non_rtti_object_dtor, "??1__non_rtti_object@@UAE@XZ");
    SET(p__non_rtti_object_opequals, "??4__non_rtti_object@@QAEAAV0@ABV0@@Z");
    SET(p__non_rtti_object_what, "?what@exception@@UBEPBDXZ");
    SET(p__non_rtti_object_vtable, "??_7__non_rtti_object@@6B@");
    SET(p__non_rtti_object_vector_dtor, "??_E__non_rtti_object@@UAEPAXI@Z");
    SET(p__non_rtti_object_scalar_dtor, "??_G__non_rtti_object@@UAEPAXI@Z");

    SET(ptype_info_dtor, "??1type_info@@UAE@XZ");
    SET(ptype_info_raw_name, "?raw_name@type_info@@QBEPBDXZ");
    SET(ptype_info_name, "?name@type_info@@QBEPBDXZ");
    SET(ptype_info_before, "?before@type_info@@QBEHABV1@@Z");
    SET(ptype_info_opequals_equals, "??8type_info@@QBEHABV0@@Z");
    SET(ptype_info_opnot_equals, "??9type_info@@QBEHABV0@@Z");

    SET(p__RTtypeid, "__RTtypeid");
    SET(p__RTCastToVoid, "__RTCastToVoid");
    SET(p__RTDynamicCast, "__RTDynamicCast");

    SET(p__unDName,"__unDName");

    /* Extremely early versions export logic_error, and crash in RTTI */
    SETNOFAIL(bAncientVersion, "??0logic_error@@QAE@ABQBD@Z");
  }
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

  pe = poperator_new(sizeof(exception) * 4 + sizeof(int));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    char name[] = "a constant";
    *((int*)pe) = 3;
    pe = (exception*)((int*)pe + 1);
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

  if (p__RTtypeid && !bAncientVersion)
  {
    /* Check the rtti */
    type_info *ti = p__RTtypeid(&e);
    ok (ti && ti->mangled &&
        !strcmp(ti->mangled, ".?AVexception@@"), "bad rtti for e\n");

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

  pe = poperator_new(sizeof(exception) * 4 + sizeof(int));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((int*)pe) = 3;
    pe = (exception*)((int*)pe + 1);
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

  if (p__RTtypeid && !bAncientVersion)
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

  pe = poperator_new(sizeof(exception) * 4 + sizeof(int));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((int*)pe) = 3;
    pe = (exception*)((int*)pe + 1);
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

  if (p__RTtypeid && !bAncientVersion)
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

  pe = poperator_new(sizeof(exception) * 4 + sizeof(int));
  ok(pe != NULL, "new() failed\n");
  if (pe)
  {
    /* vector dtor, multiple elements */
    *((int*)pe) = 3;
    pe = (exception*)((int*)pe + 1);
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

  if (p__RTtypeid && !bAncientVersion)
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

  if (!pmalloc || !pfree || !ptype_info_dtor || !ptype_info_raw_name ||
      !ptype_info_name || !ptype_info_before ||
      !ptype_info_opequals_equals || !ptype_info_opnot_equals)
    return;

  /* Test calling the dtors */
  call_func1(ptype_info_dtor, &t1); /* No effect, since name is NULL */
  t1.name = pmalloc(64);
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

/* Test RTTI functions */
static void test_rtti(void)
{
  static const char* e_name = "name";
  type_info *ti,*bti;
  exception e,b;
  void *casted;

  if (bAncientVersion ||
      !p__RTCastToVoid || !p__RTtypeid || !pexception_ctor || !pbad_typeid_ctor || !p__RTDynamicCast)
    return;

  call_func2(pexception_ctor, &e, &e_name);
  call_func2(pbad_typeid_ctor, &b, e_name);

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
/*	{ "BlaBla"," ?? ::Bla", FALSE}, */
	{ "ABVVec4@ref2@dice@@","class dice::ref2::Vec4 const &",TRUE},
	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0H@@@", "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,7>", TRUE},
	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HO@@@", "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,126>",TRUE},
	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HOA@@@", "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,2016>",TRUE},
	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$0HOAA@@@", "class CDB_GEN_BIG_ENUM_FLAG<enum CDB_WYSIWYG_BITS_ENUM,32256>",TRUE},
	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$01@@@", "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$01@@@", FALSE},
/*	{ "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$011@@@", "?AV?$CDB_GEN_BIG_ENUM_FLAG@W4CDB_WYSIWYG_BITS_ENUM@@$011@@@",FALSE}, */
    };
    int i, num_test = (sizeof(demangle)/sizeof(struct _demangle));
    
    for (i = 0; i < num_test; i++)
    {
	name = p__unDName(0, demangle[i].mangled, 0, pmalloc, pfree, 0x2800);
	if (demangle[i].test_in_wine)
	    ok(name != NULL && !strcmp(name,demangle[i].result), "Got name \"%s\" for %d\n", name, i);
	else
	    todo_wine ok(name != NULL && !strcmp(name,demangle[i].result), "Got name %s for %d\n", name, i);
	      
    }
}

/* Compare two strings treating multiple spaces (' ', ascii 0x20) in s2 
   as single space. Needed for test_demangle as __unDName() returns sometimes
   two spaces instead of one in some older native msvcrt dlls. */
static int strcmp_space(const char *s1, const char *s2)
{
    const char* s2start = s2;
    do {
        while (*s1 == *s2 && *s1) {
            s1++;
            s2++;
        }
        if (*s2 == ' ' && s2 > s2start && *(s2 - 1) == ' ')
            s2++;
        else
            break;
    } while (*s1 && *s2);
    return *s1 - *s2;
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
/* 26 */ {"??0strstreambuf@@QAE@P6APAXJ@ZP6AXPAX@Z@Z", "public: __thiscall strstreambuf::strstreambuf(void * (__cdecl*)(long),void (__cdecl*)(void *))"},
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
/* 61 */ {"?_query_new_handler@@YAP6AHI@ZXZ", "int (__cdecl*__cdecl _query_new_handler(void))(unsigned int)"},
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

    };
    int i, num_test = (sizeof(test)/sizeof(test[0]));
    char* name;

    for (i = 0; i < num_test; i++)
    {
	name = p__unDName(0, test[i].in, 0, pmalloc, pfree, test[i].flags);
        ok(name != NULL, "%u: unDName failed\n", i);
        if (!name) continue;
        ok( !strcmp_space(test[i].out, name) ||
            broken(test[i].broken && !strcmp_space(test[i].broken, name)),
           "%u: Got name \"%s\"\n", i, name );
        ok( !strcmp_space(test[i].out, name) ||
            broken(test[i].broken && !strcmp_space(test[i].broken, name)),
           "%u: Expected \"%s\"\n", i, test[i].out );
        pfree(name);
    }
}

START_TEST(cpp)
{
  InitFunctionPtrs();

  test_exception();
  test_bad_typeid();
  test_bad_cast();
  test___non_rtti_object();
  test_type_info();
  test_rtti();
  test_demangle_datatype();
  test_demangle();
}
#endif /* __i386__ */
