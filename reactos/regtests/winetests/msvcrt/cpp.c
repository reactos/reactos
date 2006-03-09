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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static void* (*poperator_new)(unsigned int);
static void  (*poperator_delete)(void*);
static void* (*pmalloc)(unsigned int);
static void  (*pfree)(void*);

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
static type_info* (*p__RTtypeid)(void*);
static void* (*p__RTCastToVoid)(void*);
static void* (*p__RTDynamicCast)(void*,int,void*,void*,int);

/* _very_ early native versions have serious RTTI bugs, so we check */
static void* bAncientVersion;

/* Emulate a __thiscall */
#ifdef _MSC_VER
inline static void* do_call_func1(void *func, void *_this)
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

inline static void* do_call_func2(void *func, void *_this, void* arg)
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
  void* ret;
  __asm__ __volatile__ ("call *%1"
                        : "=a" (ret)
                        : "g" (func), "c" (_this)
                        : "memory" );
  return ret;
}
static void* do_call_func2(void *func, void *_this, void* arg)
{
  void* ret;
  __asm__ __volatile__ ("pushl %2\n\tcall *%1"
                        : "=a" (ret)
                        : "r" (func), "g" (arg), "c" (_this)
                        : "memory" );
  return ret;
}
#endif

#define call_func1(x,y)   do_call_func1((void*)x,(void*)y)
#define call_func2(x,y,z) do_call_func2((void*)x,(void*)y,(void*)z)

/* Some exports are only available in later versions */
#define SETNOFAIL(x,y) x = (void*)GetProcAddress(hMsvcrt,y)
#define SET(x,y) SETNOFAIL(x,y); ok(x != NULL, "Export '%s' not found\n", y)

static void InitFunctionPtrs()
{
  hMsvcrt = LoadLibraryA("msvcrt.dll");
  ok(hMsvcrt != 0, "LoadLibraryA failed\n");
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
#ifndef __REACTOS__
    SET(ptype_info_name, "?name@type_info@@QBEPBDXZ");
#endif
    SET(ptype_info_before, "?before@type_info@@QBEHABV1@@Z");
    SET(ptype_info_opequals_equals, "??8type_info@@QBEHABV0@@Z");
    SET(ptype_info_opnot_equals, "??9type_info@@QBEHABV0@@Z");

    SET(p__RTtypeid, "__RTtypeid");
    SET(p__RTCastToVoid, "__RTCastToVoid");
    SET(p__RTDynamicCast, "__RTDynamicCast");

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
  ok (casted == (void*)&b, "failed cast from bad_cast to exception\n");

  /* dynamic_cast down */
  casted = p__RTDynamicCast(&e, 0, NULL, bti, 0);
  ok (casted == NULL, "Cast succeeded\n");
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

  if (hMsvcrt)
    FreeLibrary(hMsvcrt);
}
#endif /* __i386__ */
