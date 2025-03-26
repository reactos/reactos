/*
 * Tests for WIDL and RPC server/clients.
 *
 * Copyright (C) Google 2007 (Dan Hipschman)
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

#define COBJMACROS
#include <windows.h>
#include <ole2.h>
#include <oleauto.h>
#include <secext.h>
#include <rpcdce.h>
#include <netfw.h>
#include "wine/test.h"
#include "server_s.h"
#define SKIP_TYPE_DECLS
#include "server_interp_s.h"
#include "server_defines.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define PORT "4114"
#define PIPE "\\pipe\\wine_rpcrt4_test"

#define INT_CODE 4198

static const char *progname;
static BOOL old_windows_version;

static HANDLE stop_event, stop_wait_event;

static void (WINAPI *pNDRSContextMarshall2)(RPC_BINDING_HANDLE, NDR_SCONTEXT, void*, NDR_RUNDOWN, void*, ULONG);
static NDR_SCONTEXT (WINAPI *pNDRSContextUnmarshall2)(RPC_BINDING_HANDLE, void*, ULONG, void*, ULONG);
static RPC_STATUS (WINAPI *pRpcServerRegisterIfEx)(RPC_IF_HANDLE,UUID*, RPC_MGR_EPV*, unsigned int,
                   unsigned int,RPC_IF_CALLBACK_FN*);
static RPC_STATUS (WINAPI *pRpcBindingSetAuthInfoExA)(RPC_BINDING_HANDLE, RPC_CSTR, ULONG, ULONG,
                                                      RPC_AUTH_IDENTITY_HANDLE, ULONG, RPC_SECURITY_QOS *);
static RPC_STATUS (WINAPI *pRpcServerRegisterAuthInfoA)(RPC_CSTR, ULONG, RPC_AUTH_KEY_RETRIEVAL_FN, LPVOID);

static char *domain_and_user;

static int (__cdecl *int_return)(void);
static int (__cdecl *square)(int x);
static int (__cdecl *sum)(int x, int y);
static signed char (__cdecl *sum_char)(signed char x, signed char y);
static short (__cdecl *sum_short)(short x, short y);
static int (__cdecl *sum_float)(float x, float y);
static int (__cdecl *sum_double_int)(int x, double y);
static hyper (__cdecl *sum_hyper)(hyper x, hyper y);
static int (__cdecl *sum_hyper_int)(hyper x, hyper y);
static int (__cdecl *sum_char_hyper)(signed char x, hyper y);
static void (__cdecl *square_out)(int x, int *y);
static void (__cdecl *square_ref)(int *x);
static int (__cdecl *str_length)(const char *s);
static int (__cdecl *str_t_length)(str_t s);
static int (__cdecl *cstr_length)(const char *s, int n);
static int (__cdecl *dot_self)(vector_t *v);
static double (__cdecl *square_half)(double x, double *y);
static float (__cdecl *square_half_float)(float x, float *y);
static LONG (__cdecl *square_half_long)(LONG x, LONG *y);
static int (__cdecl *sum_fixed_array)(int a[5]);
static int (__cdecl *pints_sum)(pints_t *pints);
static double (__cdecl *ptypes_sum)(ptypes_t *ptypes);
static int (__cdecl *dot_pvectors)(pvectors_t *pvectors);
static int (__cdecl *sum_sp)(sp_t *sp);
static double (__cdecl *square_sun)(sun_t *su);
static int (__cdecl *test_list_length)(test_list_t *ls);
static int (__cdecl *sum_fixed_int_3d)(int m[2][3][4]);
static int (__cdecl *sum_conf_array)(int x[], int n);
static int (__cdecl *sum_conf_ptr_by_conf_ptr)(int n1, int *n2_then_x1, int *x2);
static int (__cdecl *sum_unique_conf_array)(int x[], int n);
static int (__cdecl *sum_unique_conf_ptr)(int *x, int n);
static int (__cdecl *sum_var_array)(int x[20], int n);
static int (__cdecl *dot_two_vectors)(vector_t vs[2]);
static void (__cdecl *get_number_array)(int x[20], int *n);
static int (__cdecl *sum_cs)(cs_t *cs);
static int (__cdecl *sum_cps)(cps_t *cps);
static int (__cdecl *sum_cpsc)(cpsc_t *cpsc);
static int (__cdecl *get_cpsc)(int n, cpsc_t *cpsc);
static int (__cdecl *sum_complex_array)(int n, refpint_t pi[]);
static int (__cdecl *square_puint)(puint_t p);
static int (__cdecl *sum_puints)(puints_t *p);
static int (__cdecl *sum_cpuints)(cpuints_t *p);
static int (__cdecl *dot_copy_vectors)(vector_t u, vector_t v);
static double (__cdecl *square_encu)(encu_t *eu);
static double (__cdecl *square_unencu)(int t, unencu_t *eu);
static int (__cdecl *sum_parr)(int *a[3]);
static int (__cdecl *sum_pcarr)(int *a[], int n);
static int (__cdecl *enum_ord)(e_t e);
static double (__cdecl *square_encue)(encue_t *eue);
static void (__cdecl *check_se2)(se_t *s);
static int (__cdecl *sum_toplev_conf_2n)(int *x, int n);
static int (__cdecl *sum_toplev_conf_cond)(int *x, int a, int b, int c);
static int (__cdecl *square_test_us)(test_us_t *tus);
static double (__cdecl *sum_aligns)(aligns_t *a);
static int (__cdecl *sum_padded)(padded_t *p);
static int (__cdecl *sum_padded2)(padded_t ps[2]);
static int (__cdecl *sum_padded_conf)(padded_t *ps, int n);
static int (__cdecl *sum_bogus)(bogus_t *b);
static void (__cdecl *check_null)(int *null);
static int (__cdecl *str_struct_len)(str_struct_t *s);
static int (__cdecl *wstr_struct_len)(wstr_struct_t *s);
static int (__cdecl *sum_doub_carr)(doub_carr_t *dc);
static void (__cdecl *make_pyramid_doub_carr)(unsigned char n, doub_carr_t **dc);
static unsigned (__cdecl *hash_bstr)(bstr_t s);
static void (__cdecl *get_a_bstr)(bstr_t *s);
static void (__cdecl *get_name)(name_t *name);
static void (__cdecl *get_names)(int *n, str_array_t *names);
static void (__cdecl *get_namesw)(int *n, wstr_array_t *names);
static int (__cdecl *sum_pcarr2)(int n, int **pa);
static int (__cdecl *sum_L1_norms)(int n, vector_t *vs);
static s123_t* (__cdecl *get_s123)(void);
static void (__cdecl *get_numbers)(int length, int size, pints_t pn[]);
static void (__cdecl *get_numbers_struct)(numbers_struct_t **ns);
static str_t (__cdecl *get_filename)(void);
static rint_t (__cdecl *echo_ranged_int)(int i, int j, int k);
static rint_t (__cdecl *echo_ranged_int2)(int i);
static void (__cdecl *get_ranged_enum)(renum_t *re);
static void (__cdecl *context_handle_test)(void);
static void (__cdecl *full_pointer_test)(int *a, int *b);
static void (__cdecl *full_pointer_null_test)(int *a, int *b);
static void (__cdecl *authinfo_test)(unsigned int protseq, int secure);
static void (__cdecl *stop)(void);
static void (__cdecl *stop_autolisten)(void);
static void (__cdecl *ip_test)(ipu_t *a);
static int (__cdecl *sum_ptr_array)(int *a[2]);
static int (__cdecl *sum_array_ptr)(int (*a)[2]);
static ctx_handle_t (__cdecl *get_handle)(void);
static void (__cdecl *get_handle_by_ptr)(ctx_handle_t *r);
static void (__cdecl *test_handle)(ctx_handle_t ctx_handle);

#define SERVER_FUNCTIONS \
    X(int_return) \
    X(square) \
    X(sum) \
    X(sum_char) \
    X(sum_short) \
    X(sum_float) \
    X(sum_double_int) \
    X(sum_hyper) \
    X(sum_hyper_int) \
    X(sum_char_hyper) \
    X(square_out) \
    X(square_ref) \
    X(str_length) \
    X(str_t_length) \
    X(cstr_length) \
    X(dot_self) \
    X(square_half) \
    X(square_half_float) \
    X(square_half_long) \
    X(sum_fixed_array) \
    X(pints_sum) \
    X(ptypes_sum) \
    X(dot_pvectors) \
    X(sum_sp) \
    X(square_sun) \
    X(test_list_length) \
    X(sum_fixed_int_3d) \
    X(sum_conf_array) \
    X(sum_conf_ptr_by_conf_ptr) \
    X(sum_unique_conf_array) \
    X(sum_unique_conf_ptr) \
    X(sum_var_array) \
    X(dot_two_vectors) \
    X(get_number_array) \
    X(sum_cs) \
    X(sum_cps) \
    X(sum_cpsc) \
    X(get_cpsc) \
    X(sum_complex_array) \
    X(square_puint) \
    X(sum_puints) \
    X(sum_cpuints) \
    X(dot_copy_vectors) \
    X(square_encu) \
    X(square_unencu) \
    X(sum_parr) \
    X(sum_pcarr) \
    X(enum_ord) \
    X(square_encue) \
    X(check_se2) \
    X(sum_toplev_conf_2n) \
    X(sum_toplev_conf_cond) \
    X(square_test_us) \
    X(sum_aligns) \
    X(sum_padded) \
    X(sum_padded2) \
    X(sum_padded_conf) \
    X(sum_bogus) \
    X(check_null) \
    X(str_struct_len) \
    X(wstr_struct_len) \
    X(sum_doub_carr) \
    X(make_pyramid_doub_carr) \
    X(hash_bstr) \
    X(get_a_bstr) \
    X(get_name) \
    X(get_names) \
    X(get_namesw) \
    X(sum_pcarr2) \
    X(sum_L1_norms) \
    X(get_s123) \
    X(get_numbers) \
    X(get_numbers_struct) \
    X(get_filename) \
    X(echo_ranged_int) \
    X(echo_ranged_int2) \
    X(get_ranged_enum) \
    X(context_handle_test) \
    X(full_pointer_test) \
    X(full_pointer_null_test) \
    X(authinfo_test) \
    X(stop) \
    X(stop_autolisten) \
    X(ip_test) \
    X(sum_ptr_array) \
    X(sum_array_ptr) \
    X(get_handle) \
    X(get_handle_by_ptr) \
    X(test_handle)

/* type check statements generated in header file */
fnprintf *p_printf = printf;

static const WCHAR helloW[] = { 'H','e','l','l','o',0 };
static const WCHAR worldW[] = { 'W','o','r','l','d','!',0 };

static BOOL is_interp;

static void set_interp_interface(void)
{
    is_interp = TRUE;

#define X(name) name = interp_##name;
    SERVER_FUNCTIONS
#undef X
}

static void set_mixed_interface(void)
{
    is_interp = FALSE;

#define X(name) name = mixed_##name;
    SERVER_FUNCTIONS
#undef X
}

static void InitFunctionPointers(void)
{
    HMODULE hrpcrt4 = GetModuleHandleA("rpcrt4.dll");

    pNDRSContextMarshall2 = (void *)GetProcAddress(hrpcrt4, "NDRSContextMarshall2");
    pNDRSContextUnmarshall2 = (void *)GetProcAddress(hrpcrt4, "NDRSContextUnmarshall2");
    pRpcServerRegisterIfEx = (void *)GetProcAddress(hrpcrt4, "RpcServerRegisterIfEx");
    pRpcBindingSetAuthInfoExA = (void *)GetProcAddress(hrpcrt4, "RpcBindingSetAuthInfoExA");
    pRpcServerRegisterAuthInfoA = (void *)GetProcAddress(hrpcrt4, "RpcServerRegisterAuthInfoA");

    if (!pNDRSContextMarshall2) old_windows_version = TRUE;
}

void __RPC_FAR *__RPC_USER
midl_user_allocate(SIZE_T n)
{
  return HeapAlloc(GetProcessHeap(), 0, n);
}

void __RPC_USER
midl_user_free(void __RPC_FAR *p)
{
  HeapFree(GetProcessHeap(), 0, p);
}

static char *
xstrdup(const char *s)
{
  char *d = HeapAlloc(GetProcessHeap(), 0, strlen(s) + 1);
  strcpy(d, s);
  return d;
}

int __cdecl s_int_return(void)
{
  return INT_CODE;
}

int __cdecl s_square(int x)
{
  return x * x;
}

int __cdecl s_sum(int x, int y)
{
  return x + y;
}

signed char __cdecl s_sum_char(signed char x, signed char y)
{
    return x + y;
}

short __cdecl s_sum_short(short x, short y)
{
    return x + y;
}

int __cdecl s_sum_float(float x, float y)
{
    return x + y;
}

int __cdecl s_sum_double_int(int x, double y)
{
    return x + y;
}

hyper __cdecl s_sum_hyper(hyper x, hyper y)
{
    return x + y;
}

int __cdecl s_sum_hyper_int(hyper x, hyper y)
{
    return x + y;
}

int __cdecl s_sum_char_hyper(signed char x, hyper y)
{
    return x + y;
}

void __cdecl s_square_out(int x, int *y)
{
  *y = s_square(x);
}

void __cdecl s_square_ref(int *x)
{
  *x = s_square(*x);
}

int __cdecl s_str_length(const char *s)
{
  return strlen(s);
}

int __cdecl s_str_t_length(str_t s)
{
  return strlen(s);
}

int __cdecl s_cstr_length(const char *s, int n)
{
  int len = 0;
  while (0 < n-- && *s++)
    ++len;
  return len;
}

int __cdecl s_dot_self(vector_t *v)
{
  return s_square(v->x) + s_square(v->y) + s_square(v->z);
}

double __cdecl s_square_half(double x, double *y)
{
  *y = x / 2.0;
  return x * x;
}

float __cdecl s_square_half_float(float x, float *y)
{
  *y = x / 2.0f;
  return x * x;
}

LONG __cdecl s_square_half_long(LONG x, LONG *y)
{
  *y = x / 2;
  return x * x;
}

int __cdecl s_sum_fixed_array(int a[5])
{
  return a[0] + a[1] + a[2] + a[3] + a[4];
}

int __cdecl s_pints_sum(pints_t *pints)
{
  return *pints->pi + **pints->ppi + ***pints->pppi;
}

double __cdecl s_ptypes_sum(ptypes_t *pt)
{
  return *pt->pc + *pt->ps + *pt->pl + *pt->pf + *pt->pd;
}

int __cdecl s_dot_pvectors(pvectors_t *p)
{
  return p->pu->x * (*p->pv)->x + p->pu->y * (*p->pv)->y + p->pu->z * (*p->pv)->z;
}

int __cdecl s_sum_sp(sp_t *sp)
{
  return sp->x + sp->s->x;
}

double __cdecl s_square_sun(sun_t *su)
{
  switch (su->s)
  {
  case SUN_I: return su->u.i * su->u.i;
  case SUN_F1:
  case SUN_F2: return su->u.f * su->u.f;
  case SUN_PI: return (*su->u.pi) * (*su->u.pi);
  default:
    return 0.0;
  }
}

int __cdecl s_test_list_length(test_list_t *list)
{
  return (list->t == TL_LIST
          ? 1 + s_test_list_length(list->u.tail)
          : 0);
}

int __cdecl s_sum_fixed_int_3d(int m[2][3][4])
{
  int i, j, k;
  int sum = 0;

  for (i = 0; i < 2; ++i)
    for (j = 0; j < 3; ++j)
      for (k = 0; k < 4; ++k)
        sum += m[i][j][k];

  return sum;
}

int __cdecl s_sum_conf_array(int x[], int n)
{
  int *p = x, *end = p + n;
  int sum = 0;

  while (p < end)
    sum += *p++;

  return sum;
}

int __cdecl s_sum_conf_ptr_by_conf_ptr(int n1, int *n2_then_x1, int *x2)
{
  int i;
  int sum = 0;
  if(n1 == 0)
    return 0;

  for(i = 1; i < n1; ++i)
    sum += n2_then_x1[i];

  for(i = 0; i < *n2_then_x1; ++i)
    sum += x2[i];

  return sum;
}

int __cdecl s_sum_unique_conf_array(int x[], int n)
{
  return s_sum_conf_array(x, n);
}

int __cdecl s_sum_unique_conf_ptr(int *x, int n)
{
  return x ? s_sum_conf_array(x, n) : 0;
}

int __cdecl s_sum_var_array(int x[20], int n)
{
  ok(0 <= n, "RPC sum_var_array\n");
  ok(n <= 20, "RPC sum_var_array\n");

  return s_sum_conf_array(x, n);
}

int __cdecl s_sum_complex_array(int n, refpint_t pi[])
{
  int total = 0;
  for (; n > 0; n--)
    total += *pi[n - 1];
  return total;
}

int __cdecl s_dot_two_vectors(vector_t vs[2])
{
  return vs[0].x * vs[1].x + vs[0].y * vs[1].y + vs[0].z * vs[1].z;
}

void __cdecl s_get_number_array(int x[20], int *n)
{
  int c[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  memcpy(x, c, sizeof(c));
  *n = ARRAY_SIZE(c);
}

int __cdecl s_sum_cs(cs_t *cs)
{
  return s_sum_conf_array(cs->ca, cs->n);
}

int __cdecl s_sum_cps(cps_t *cps)
{
  int sum = 0;
  int i;

  for (i = 0; i < *cps->pn; ++i)
    sum += cps->ca1[i];

  for (i = 0; i < 2 * cps->n; ++i)
    sum += cps->ca2[i];

  return sum;
}

int __cdecl s_sum_cpsc(cpsc_t *cpsc)
{
  int sum = 0;
  int i;
  for (i = 0; i < (cpsc->c ? cpsc->a : cpsc->b); ++i)
    sum += cpsc->ca[i];
  return sum;
}

int __cdecl s_get_cpsc(int n, cpsc_t *cpsc)
{
  int i, ret;

  cpsc->a = 2 * n;
  cpsc->b = 2;
  cpsc->c = 1;
  cpsc->ca = MIDL_user_allocate( cpsc->a * sizeof(int) );
  for (i = ret = 0; i < cpsc->a; i++) cpsc->ca[i] = i;
  for (i = ret = 0; i < cpsc->a; i++) ret += cpsc->ca[i];
  return ret;
}

int __cdecl s_square_puint(puint_t p)
{
  int n = atoi(p);
  return n * n;
}

int __cdecl s_sum_puints(puints_t *p)
{
  int sum = 0;
  int i;
  for (i = 0; i < p->n; ++i)
    sum += atoi(p->ps[i]);
  return sum;
}

int __cdecl s_sum_cpuints(cpuints_t *p)
{
  int sum = 0;
  int i;
  for (i = 0; i < p->n; ++i)
    sum += atoi(p->ps[i]);
  return sum;
}

int __cdecl s_dot_copy_vectors(vector_t u, vector_t v)
{
  return u.x * v.x + u.y * v.y + u.z * v.z;
}

int __cdecl s_square_test_us(test_us_t *tus)
{
  int n = atoi(tus->us.x);
  return n * n;
}

double __cdecl s_square_encu(encu_t *eu)
{
  switch (eu->t)
  {
  case ENCU_I: return eu->tagged_union.i * eu->tagged_union.i;
  case ENCU_F: return eu->tagged_union.f * eu->tagged_union.f;
  default:
    return 0.0;
  }
}

double __cdecl s_square_unencu(int t, unencu_t *eu)
{
  switch (t)
  {
  case ENCU_I: return eu->i * eu->i;
  case ENCU_F: return eu->f * eu->f;
  default:
    return 0.0;
  }
}

void __cdecl s_check_se2(se_t *s)
{
  ok(s->f == E2, "check_se2\n");
}

int __cdecl s_sum_parr(int *a[3])
{
  return s_sum_pcarr(a, 3);
}

int __cdecl s_sum_pcarr(int *a[], int n)
{
  int i, s = 0;
  for (i = 0; i < n; ++i)
    s += *a[i];
  return s;
}

int __cdecl s_enum_ord(e_t e)
{
  switch (e)
  {
  case E1: return 1;
  case E2: return 2;
  case E3: return 3;
  case E4: return 4;
  default:
    return 0;
  }
}

double __cdecl s_square_encue(encue_t *eue)
{
  switch (eue->t)
  {
  case E1: return eue->tagged_union.i1 * eue->tagged_union.i1;
  case E2: return eue->tagged_union.f2 * eue->tagged_union.f2;
  default:
    return 0.0;
  }
}

int __cdecl s_sum_toplev_conf_2n(int *x, int n)
{
  int sum = 0;
  int i;
  for (i = 0; i < 2 * n; ++i)
    sum += x[i];
  return sum;
}

int __cdecl s_sum_toplev_conf_cond(int *x, int a, int b, int c)
{
  int sum = 0;
  int n = c ? a : b;
  int i;
  for (i = 0; i < n; ++i)
    sum += x[i];
  return sum;
}

double __cdecl s_sum_aligns(aligns_t *a)
{
  return a->c + a->i + a->s + a->d;
}

int __cdecl s_sum_padded(padded_t *p)
{
  return p->i + p->c;
}

int __cdecl s_sum_padded2(padded_t ps[2])
{
  return s_sum_padded(&ps[0]) + s_sum_padded(&ps[1]);
}

int __cdecl s_sum_padded_conf(padded_t *ps, int n)
{
  int sum = 0;
  int i;
  for (i = 0; i < n; ++i)
    sum += s_sum_padded(&ps[i]);
  return sum;
}

int __cdecl s_sum_bogus(bogus_t *b)
{
  return *b->h.p1 + *b->p2 + *b->p3 + b->c;
}

void __cdecl s_check_null(int *null)
{
  ok(!null, "RPC check_null\n");
}

int __cdecl s_str_struct_len(str_struct_t *s)
{
  return lstrlenA(s->s);
}

int __cdecl s_wstr_struct_len(wstr_struct_t *s)
{
  return lstrlenW(s->s);
}

int __cdecl s_sum_doub_carr(doub_carr_t *dc)
{
  int i, j;
  int sum = 0;
  for (i = 0; i < dc->n; ++i)
    for (j = 0; j < dc->a[i]->n; ++j)
      sum += dc->a[i]->a[j];
  return sum;
}

void __cdecl s_make_pyramid_doub_carr(unsigned char n, doub_carr_t **dc)
{
  doub_carr_t *t;
  int i, j;
  t = MIDL_user_allocate(FIELD_OFFSET(doub_carr_t, a[n]));
  t->n = n;
  for (i = 0; i < n; ++i)
  {
    int v = i + 1;
    t->a[i] = MIDL_user_allocate(FIELD_OFFSET(doub_carr_1_t, a[v]));
    t->a[i]->n = v;
    for (j = 0; j < v; ++j)
      t->a[i]->a[j] = j + 1;
  }
  *dc = t;
}

unsigned __cdecl s_hash_bstr(bstr_t b)
{
  short n = b[-1];
  short *s = b;
  unsigned hash = 0;
  short i;
  for (i = 0; i < n; ++i)
    hash = 5 * hash + (unsigned) s[i];
  return hash;
}

void __cdecl s_get_a_bstr(bstr_t *b)
{
  bstr_t bstr;
  short str[] = {5, 'W', 'i', 'n', 'e', 0};
  bstr = HeapAlloc(GetProcessHeap(), 0, sizeof(str));
  memcpy(bstr, str, sizeof(str));
  *b = bstr + 1;
}

void __cdecl s_get_name(name_t *name)
{
  const char bossman[] = "Jeremy White";
  memcpy(name->name, bossman, min(name->size, sizeof(bossman)));
  /* ensure nul-termination */
  if (name->size < sizeof(bossman))
    name->name[name->size - 1] = 0;
}

void __cdecl s_get_names(int *n, str_array_t *names)
{
  str_array_t list;

  list = MIDL_user_allocate(2 * sizeof(list[0]));
  list[0] = MIDL_user_allocate(6);
  strcpy(list[0], "Hello");
  list[1] = MIDL_user_allocate(7);
  strcpy(list[1], "World!");

  *names = list;
  *n = 2;
}

void __cdecl s_get_namesw(int *n, wstr_array_t *names)
{
  wstr_array_t list;

  list = MIDL_user_allocate(2 * sizeof(list[0]));
  list[0] = MIDL_user_allocate(sizeof(helloW));
  lstrcpyW(list[0], helloW);
  list[1] = MIDL_user_allocate(sizeof(worldW));
  lstrcpyW(list[1], worldW);

  *names = list;
  *n = 2;
}

int __cdecl s_sum_pcarr2(int n, int **pa)
{
  return s_sum_conf_array(*pa, n);
}

int __cdecl s_sum_L1_norms(int n, vector_t *vs)
{
  int i;
  int sum = 0;
  for (i = 0; i < n; ++i)
    sum += abs(vs[i].x) + abs(vs[i].y) + abs(vs[i].z);
  return sum;
}

s123_t * __cdecl s_get_s123(void)
{
  s123_t *s = MIDL_user_allocate(sizeof *s);
  s->f1 = 1;
  s->f2 = 2;
  s->f3 = 3;
  return s;
}

str_t __cdecl s_get_filename(void)
{
    return (char *)__FILE__;
}

int __cdecl s_echo_ranged_int(int i, int j, int k)
{
    return min( 100, i + j + k );
}

int __cdecl s_echo_ranged_int2(int i)
{
    return i;
}

void __cdecl s_get_ranged_enum(renum_t *re)
{
    *re = RE3;
}

void __cdecl s_context_handle_test(void)
{
    NDR_SCONTEXT h;
    RPC_BINDING_HANDLE binding;
    RPC_STATUS status;
    unsigned char buf[20];
    static RPC_SERVER_INTERFACE server_if =
    {
        sizeof(RPC_SERVER_INTERFACE),
        {{0x00000000,0x4114,0x0704,{0x23,0x01,0x00,0x00,0x00,0x00,0x00,0x00}},{1,0}},
        {{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},
        NULL,
        0,
        0,
        0,
        0,
        0,
    };

    binding = I_RpcGetCurrentCallHandle();
    ok(binding != NULL, "I_RpcGetCurrentCallHandle returned NULL\n");

    if (!pNDRSContextMarshall2 || !pNDRSContextUnmarshall2)
    {
        win_skip("NDRSContextMarshall2 or NDRSContextUnmarshall2 not exported from rpcrt4.dll\n");
        return;
    }

    h = pNDRSContextUnmarshall2(binding, NULL, NDR_LOCAL_DATA_REPRESENTATION, NULL, 0);
    ok(h != NULL, "NDRSContextUnmarshall2 returned NULL\n");

    /* marshal a context handle with NULL userContext */
    memset(buf, 0xcc, sizeof(buf));
    pNDRSContextMarshall2(binding, h, buf, NULL, NULL, 0);
    ok(*(ULONG *)buf == 0, "attributes should have been set to 0 instead of 0x%x\n", *(ULONG *)buf);
    ok(UuidIsNil((UUID *)&buf[4], &status), "uuid should have been nil\n");

    h = pNDRSContextUnmarshall2(binding, NULL, NDR_LOCAL_DATA_REPRESENTATION, NULL, 0);
    ok(h != NULL, "NDRSContextUnmarshall2 returned NULL\n");

    /* marshal a context handle with non-NULL userContext */
    memset(buf, 0xcc, sizeof(buf));
    h->userContext = (void *)0xdeadbeef;
    pNDRSContextMarshall2(binding, h, buf, NULL, NULL, 0);
    ok(*(ULONG *)buf == 0, "attributes should have been set to 0 instead of 0x%x\n", *(ULONG *)buf);
    ok(!UuidIsNil((UUID *)&buf[4], &status), "uuid should not have been nil\n");

    /* raises ERROR_INVALID_HANDLE exception on Vista upwards */
    if (0)
    {
    h = pNDRSContextUnmarshall2(binding, buf, NDR_LOCAL_DATA_REPRESENTATION, NULL, 0);
    ok(h != NULL, "NDRSContextUnmarshall2 returned NULL\n");
    ok(h->userContext == (void *)0xdeadbeef, "userContext of interface didn't unmarshal properly: %p\n", h->userContext);

    /* marshal a context handle with an interface specified */
    h = pNDRSContextUnmarshall2(binding, NULL, NDR_LOCAL_DATA_REPRESENTATION, &server_if.InterfaceId, 0);
    ok(h != NULL, "NDRSContextUnmarshall2 returned NULL\n");

    memset(buf, 0xcc, sizeof(buf));
    h->userContext = (void *)0xcafebabe;
    pNDRSContextMarshall2(binding, h, buf, NULL, &server_if.InterfaceId, 0);
    ok(*(ULONG *)buf == 0, "attributes should have been set to 0 instead of 0x%x\n", *(ULONG *)buf);
    ok(!UuidIsNil((UUID *)&buf[4], &status), "uuid should not have been nil\n");

    h = pNDRSContextUnmarshall2(binding, buf, NDR_LOCAL_DATA_REPRESENTATION, &server_if.InterfaceId, 0);
    ok(h != NULL, "NDRSContextUnmarshall2 returned NULL\n");
    ok(h->userContext == (void *)0xcafebabe, "userContext of interface didn't unmarshal properly: %p\n", h->userContext);
    }

    /* test same interface data, but different pointer */
    /* raises ERROR_INVALID_HANDLE exception */
    if (0)
    {
        RPC_SERVER_INTERFACE server_if_clone = server_if;

        pNDRSContextUnmarshall2(binding, buf, NDR_LOCAL_DATA_REPRESENTATION, &server_if_clone.InterfaceId, 0);
    }

    /* test different interface data, but different pointer */
    /* raises ERROR_INVALID_HANDLE exception */
    if (0)
    {
        static RPC_SERVER_INTERFACE server_if2 =
        {
            sizeof(RPC_SERVER_INTERFACE),
            {{0x00000000,0x4114,0x0704,{0x23,0x01,0x00,0x00,0x00,0x00,0x00,0x00}},{1,0}},
            {{0x8a885d04,0x1ceb,0x11c9,{0x9f,0xe8,0x08,0x00,0x2b,0x10,0x48,0x60}},{2,0}},
            NULL,
            0,
            0,
            0,
            0,
            0,
        };
        pNDRSContextMarshall2(binding, h, buf, NULL, &server_if.InterfaceId, 0);

        pNDRSContextUnmarshall2(binding, buf, NDR_LOCAL_DATA_REPRESENTATION, &server_if2.InterfaceId, 0);
    }

    binding = NULL;
    status = RpcBindingServerFromClient(NULL, &binding);

    ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
    ok(binding != NULL, "binding is NULL\n");

    if (status == RPC_S_OK && binding != NULL)
    {
        unsigned char* string_binding = NULL;
        unsigned char* object_uuid = NULL;
        unsigned char* protseq = NULL;
        unsigned char* network_address = NULL;
        unsigned char* endpoint = NULL;
        unsigned char* network_options = NULL;

        status = RpcBindingToStringBindingA(binding, &string_binding);
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        ok(string_binding != NULL, "string_binding is NULL\n");

        status = RpcStringBindingParseA(string_binding, &object_uuid, &protseq, &network_address, &endpoint, &network_options);
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        ok(protseq != NULL && *protseq != '\0', "protseq is %s\n", protseq);
        ok(network_address != NULL && *network_address != '\0', "network_address is %s\n", network_address);

        todo_wine
        {
            ok(object_uuid != NULL && *object_uuid == '\0', "object_uuid is %s\n", object_uuid);
            ok(endpoint != NULL && *endpoint == '\0', "endpoint is %s\n", endpoint);
            ok(network_options != NULL && *network_options == '\0', "network_options is %s\n", network_options);
        }

        RpcStringFreeA(&string_binding);
        RpcStringFreeA(&object_uuid);
        RpcStringFreeA(&protseq);
        RpcStringFreeA(&network_address);
        RpcStringFreeA(&endpoint);
        RpcStringFreeA(&network_options);
        RpcBindingFree(&binding);
    }
}

void __cdecl s_get_numbers(int length, int size, pints_t n[])
{
    int i;
    for (i = 0; i < length; i++)
    {
        n[i].pi = midl_user_allocate(sizeof(*n[i].pi));
        *n[i].pi = i;
        n[i].ppi = NULL;
        n[i].pppi = NULL;
    }
}

void __cdecl s_get_numbers_struct(numbers_struct_t **ns)
{
    int i;
    *ns = midl_user_allocate(FIELD_OFFSET(numbers_struct_t, numbers[5]));
    if (!*ns) return;
    (*ns)->length = 5;
    (*ns)->size = 5;
    for (i = 0; i < (*ns)->length; i++)
    {
        (*ns)->numbers[i].pi = NULL;
        (*ns)->numbers[i].ppi = NULL;
        (*ns)->numbers[i].pppi = NULL;
    }
    (*ns)->numbers[0].pi = midl_user_allocate(sizeof(*(*ns)->numbers[i].pi));
    *(*ns)->numbers[0].pi = 5;
}

void __cdecl s_full_pointer_test(int *a, int *b)
{
    ok(*a == 42, "Expected *a to be 42 instead of %d\n", *a);
    ok(*b == 42, "Expected *b to be 42 instead of %d\n", *a);
    ok(a == b, "Expected a (%p) to point to the same memory as b (%p)\n", a, b);
}

void __cdecl s_full_pointer_null_test(int *a, int *b)
{
    ok(*a == 42, "Expected *a to be 42 instead of %d\n", *a);
    ok(b == NULL, "Expected b to be NULL instead of %p\n", b);
}

void __cdecl s_stop(void)
{
  if (!stop_wait_event)
  {
    ok(RPC_S_OK == RpcMgmtStopServerListening(NULL), "RpcMgmtStopServerListening\n");
    ok(RPC_S_OK == RpcServerUnregisterIf(NULL, NULL, FALSE), "RpcServerUnregisterIf\n");
  }
  ok(SetEvent(stop_event), "SetEvent\n");
  if (stop_wait_event)
  {
    DWORD ret;
    ret = WaitForSingleObject(stop_wait_event, 10000);
    ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");
  }
}

void __cdecl s_stop_autolisten(void)
{
    RPC_STATUS status;
    status = RpcServerUnregisterIf(NULL, NULL, FALSE);
todo_wine
    ok(status == RPC_S_UNKNOWN_MGR_TYPE, "got %u\n", status);
}

void __cdecl s_ip_test(ipu_t *a)
{
    STATSTG st;
    HRESULT hr;

    hr = IStream_Stat(a->tagged_union.stream, &st, STATFLAG_NONAME);
    ok(hr == S_OK, "got %#x\n", hr);
}

int __cdecl s_sum_ptr_array(int *a[2])
{
    return *a[0] + *a[1];
}

int __cdecl s_sum_array_ptr(int (*a)[2])
{
    return (*a)[0] + (*a)[1];
}

ctx_handle_t __cdecl s_get_handle(void)
{
    return (ctx_handle_t)0xdeadbeef;
}

void __cdecl s_get_handle_by_ptr(ctx_handle_t *r)
{
    *r = (ctx_handle_t)0xdeadbeef;
}

void __cdecl s_test_handle(ctx_handle_t ctx_handle)
{
    ok(ctx_handle == (ctx_handle_t)0xdeadbeef, "Unexpected ctx_handle %p\n", ctx_handle);
}

void __RPC_USER ctx_handle_t_rundown(ctx_handle_t ctx_handle)
{
    ok(ctx_handle == (ctx_handle_t)0xdeadbeef, "Unexpected ctx_handle %p\n", ctx_handle);
}

static void
make_cmdline(char buffer[MAX_PATH], const char *test)
{
  sprintf(buffer, "%s server %s", progname, test);
}

static void
run_client(const char *test)
{
  char cmdline[MAX_PATH];
  PROCESS_INFORMATION info;
  STARTUPINFOA startup;

  memset(&startup, 0, sizeof startup);
  startup.cb = sizeof startup;

  make_cmdline(cmdline, test);
  ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
  winetest_wait_child_process( info.hProcess );
  ok(CloseHandle(info.hProcess), "CloseHandle\n");
  ok(CloseHandle(info.hThread), "CloseHandle\n");
}

static void
basic_tests(void)
{
  char string[] = "I am a string";
  WCHAR wstring[] = {'I',' ','a','m',' ','a',' ','w','s','t','r','i','n','g', 0};
  int f[5] = {1, 3, 0, -2, -4};
  vector_t a = {1, 3, 7};
  vector_t vec1 = {4, -2, 1}, vec2 = {-5, 2, 3}, *pvec2 = &vec2;
  pvectors_t pvecs = {&vec1, &pvec2};
  sp_inner_t spi = {42};
  sp_t sp = {-13, &spi};
  aligns_t aligns;
  pints_t pints;
  ptypes_t ptypes;
  padded_t padded;
  padded_t padded2[2];
  bogus_t bogus;
  int i1, i2, i3, *pi2, *pi3, **ppi3;
  double u, v;
  float s, t;
  LONG q, r;
  short h;
  char c;
  int x;
  hyper y;
  str_struct_t ss = {string};
  wstr_struct_t ws = {wstring};
  str_t str;
  se_t se;
  renum_t re;

  ok(int_return() == INT_CODE, "RPC int_return\n");

  ok(square(7) == 49, "RPC square\n");
  x = sum(23, -4);
  ok(x == 19, "RPC sum got %d\n", x);
  c = sum_char(-23, 50);
  ok(c == 27, "RPC sum_char got %d\n", (int)c);
  h = sum_short(1122, -344);
  ok(h == 778, "RPC sum_short got %d\n", (int)h);
  x = sum_float(123.45, -32.2);
  ok(x == 91, "RPC sum_float got %d\n", x);
  x = sum_double_int(-78, 148.46);
  ok(x == 70, "RPC sum_double_int got %d\n", x);
  y = sum_hyper((hyper)0x12345678 << 16, (hyper)0x33557799 << 16);
  ok(y == (hyper)0x4589ce11 << 16, "RPC hyper got %s\n", wine_dbgstr_longlong(y));
  x = sum_hyper_int((hyper)0x24242424 << 16, -((hyper)0x24241212 << 16));
  ok(x == 0x12120000, "RPC hyper_int got 0x%x\n", x);
  x = sum_char_hyper( 12, ((hyper)0x42424242 << 32) | 0x33334444 );
  ok(x == 0x33334450, "RPC char_hyper got 0x%x\n", x);

  x = 0;
  square_out(11, &x);
  ok(x == 121, "RPC square_out\n");

  x = 5;
  square_ref(&x);
  ok(x == 25, "RPC square_ref\n");

  ok(str_length(string) == strlen(string), "RPC str_length\n");
  ok(str_t_length(string) == strlen(string), "RPC str_length\n");
  ok(dot_self(&a) == 59, "RPC dot_self\n");

  ok(str_struct_len(&ss) == lstrlenA(string), "RPC str_struct_len\n");
  ok(wstr_struct_len(&ws) == lstrlenW(wstring), "RPC str_struct_len\n");

  v = 0.0;
  u = square_half(3.0, &v);
  ok(u == 9.0, "RPC square_half\n");
  ok(v == 1.5, "RPC square_half\n");

  t = 0.0f;
  s = square_half_float(3.0f, &t);
  ok(s == 9.0f, "RPC square_half_float\n");
  ok(t == 1.5f, "RPC square_half_float\n");

  r = 0;
  q = square_half_long(3, &r);
  ok(q == 9, "RPC square_half_long\n");
  ok(r == 1, "RPC square_half_long\n");

  i1 = 19;
  i2 = -3;
  i3 = -29;
  pi2 = &i2;
  pi3 = &i3;
  ppi3 = &pi3;
  pints.pi = &i1;
  pints.ppi = &pi2;
  pints.pppi = &ppi3;
  ok(pints_sum(&pints) == -13, "RPC pints_sum\n");

  c = 10;
  h = 3;
  q = 14;
  s = -5.0f;
  u = 11.0;
  ptypes.pc = &c;
  ptypes.ps = &h;
  ptypes.pl = &q;
  ptypes.pf = &s;
  ptypes.pd = &u;
  ok(ptypes_sum(&ptypes) == 33.0, "RPC ptypes_sum\n");

  ok(dot_pvectors(&pvecs) == -21, "RPC dot_pvectors\n");
  ok(dot_copy_vectors(vec1, vec2) == -21, "RPC dot_copy_vectors\n");
  ok(sum_fixed_array(f) == -2, "RPC sum_fixed_array\n");
  ok(sum_sp(&sp) == 29, "RPC sum_sp\n");

  ok(enum_ord(E1) == 1, "RPC enum_ord\n");
  ok(enum_ord(E2) == 2, "RPC enum_ord\n");
  ok(enum_ord(E3) == 3, "RPC enum_ord\n");
  ok(enum_ord(E4) == 4, "RPC enum_ord\n");

  se.f = E2;
  check_se2(&se);

  memset(&aligns, 0, sizeof(aligns));
  aligns.c = 3;
  aligns.i = 4;
  aligns.s = 5;
  aligns.d = 6.0;
  ok(sum_aligns(&aligns) == 18.0, "RPC sum_aligns\n");

  padded.i = -3;
  padded.c = 8;
  ok(sum_padded(&padded) == 5, "RPC sum_padded\n");
  padded2[0].i = -5;
  padded2[0].c = 1;
  padded2[1].i = 3;
  padded2[1].c = 7;
  ok(sum_padded2(padded2) == 6, "RPC sum_padded2\n");
  padded2[0].i = -5;
  padded2[0].c = 1;
  padded2[1].i = 3;
  padded2[1].c = 7;
  ok(sum_padded_conf(padded2, 2) == 6, "RPC sum_padded_conf\n");

  i1 = 14;
  i2 = -7;
  i3 = -4;
  bogus.h.p1 = &i1;
  bogus.p2 = &i2;
  bogus.p3 = &i3;
  bogus.c = 9;
  ok(sum_bogus(&bogus) == 12, "RPC sum_bogus\n");

  check_null(NULL);

  if (!is_interp || sizeof(void*) != 8) { /* broken in widl for win64 */
  str = get_filename();
  ok(!strcmp(str, __FILE__), "get_filename() returned %s instead of %s\n", str, __FILE__);
  midl_user_free(str);
  }

  x = echo_ranged_int(0,0,0);
  ok(x == 0, "echo_ranged_int() returned %d instead of 0\n", x);
  x = echo_ranged_int(10,20,100);
  ok(x == 100, "echo_ranged_int() returned %d instead of 100\n", x);
  x = echo_ranged_int2(40);
  ok(x == 40, "echo_ranged_int() returned %d instead of 40\n", x);

  if (!old_windows_version)
  {
      re = 0xdeadbeef;
      get_ranged_enum(&re);
      ok(re == RE3 ||
         broken(re == MAKELONG(re, 0xdead)), /* Win 8, Win 10 */
         "get_ranged_enum() returned %x instead of RE3\n", re);
  }
}

static void
union_tests(void)
{
  encue_t eue;
  encu_t eu;
  unencu_t uneu;
  sun_t su;
  ipu_t ipu;
  LONG ref;
  int i;

  su.s = SUN_I;
  su.u.i = 9;
  ok(square_sun(&su) == 81.0, "RPC square_sun\n");

  su.s = SUN_F1;
  su.u.f = 5.0;
  ok(square_sun(&su) == 25.0, "RPC square_sun\n");

  su.s = SUN_F2;
  su.u.f = -2.0;
  ok(square_sun(&su) == 4.0, "RPC square_sun\n");

  su.s = SUN_PI;
  su.u.pi = &i;
  i = 11;
  ok(square_sun(&su) == 121.0, "RPC square_sun\n");

  eu.t = ENCU_I;
  eu.tagged_union.i = 7;
  ok(square_encu(&eu) == 49.0, "RPC square_encu\n");

  eu.t = ENCU_F;
  eu.tagged_union.f = 3.0;
  ok(square_encu(&eu) == 9.0, "RPC square_encu\n");

  uneu.i = 4;
  ok(square_unencu(ENCU_I, &uneu) == 16.0, "RPC square_unencu\n");

  uneu.f = 5.0;
  ok(square_unencu(ENCU_F, &uneu) == 25.0, "RPC square_unencu\n");

  eue.t = E1;
  eue.tagged_union.i1 = 8;
  ok(square_encue(&eue) == 64.0, "RPC square_encue\n");

  eue.t = E2;
  eue.tagged_union.f2 = 10.0;
  ok(square_encue(&eue) == 100.0, "RPC square_encue\n");

  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  CreateStreamOnHGlobal(NULL, TRUE, &ipu.tagged_union.stream);
  ip_test(&ipu);
  ref = IStream_Release(ipu.tagged_union.stream);
  ok(!ref, "got %u refs\n", ref);

  CoUninitialize();
}

static test_list_t *
null_list(void)
{
  test_list_t *n = HeapAlloc(GetProcessHeap(), 0, sizeof *n);
  n->t = TL_NULL;
  n->u.x = 0;
  return n;
}

static test_list_t *
make_list(test_list_t *tail)
{
  test_list_t *n = HeapAlloc(GetProcessHeap(), 0, sizeof *n);
  n->t = TL_LIST;
  n->u.tail = tail;
  return n;
}

static void
free_list(test_list_t *list)
{
  if (list->t == TL_LIST)
    free_list(list->u.tail);
  HeapFree(GetProcessHeap(), 0, list);
}

ULONG __RPC_USER
puint_t_UserSize(ULONG *flags, ULONG start, puint_t *p)
{
  return start + sizeof(int);
}

unsigned char * __RPC_USER
puint_t_UserMarshal(ULONG *flags, unsigned char *buffer, puint_t *p)
{
  int n = atoi(*p);
  memcpy(buffer, &n, sizeof n);
  return buffer + sizeof n;
}

unsigned char * __RPC_USER
puint_t_UserUnmarshal(ULONG *flags, unsigned char *buffer, puint_t *p)
{
  int n;
  memcpy(&n, buffer, sizeof n);
  *p = HeapAlloc(GetProcessHeap(), 0, 10);
  sprintf(*p, "%d", n);
  return buffer + sizeof n;
}

void __RPC_USER
puint_t_UserFree(ULONG *flags, puint_t *p)
{
  HeapFree(GetProcessHeap(), 0, *p);
}

ULONG __RPC_USER
us_t_UserSize(ULONG *flags, ULONG start, us_t *pus)
{
  return start + sizeof(struct wire_us);
}

unsigned char * __RPC_USER
us_t_UserMarshal(ULONG *flags, unsigned char *buffer, us_t *pus)
{
  struct wire_us wus;
  wus.x = atoi(pus->x);
  memcpy(buffer, &wus, sizeof wus);
  return buffer + sizeof wus;
}

unsigned char * __RPC_USER
us_t_UserUnmarshal(ULONG *flags, unsigned char *buffer, us_t *pus)
{
  struct wire_us wus;
  memcpy(&wus, buffer, sizeof wus);
  pus->x = HeapAlloc(GetProcessHeap(), 0, 10);
  sprintf(pus->x, "%d", wus.x);
  return buffer + sizeof wus;
}

void __RPC_USER
us_t_UserFree(ULONG *flags, us_t *pus)
{
  HeapFree(GetProcessHeap(), 0, pus->x);
}

ULONG __RPC_USER
bstr_t_UserSize(ULONG *flags, ULONG start, bstr_t *b)
{
  return start + FIELD_OFFSET(user_bstr_t, data[(*b)[-1]]);
}

unsigned char * __RPC_USER
bstr_t_UserMarshal(ULONG *flags, unsigned char *buffer, bstr_t *b)
{
  wire_bstr_t wb = (wire_bstr_t) buffer;
  wb->n = (*b)[-1];
  memcpy(&wb->data, *b, wb->n * sizeof wb->data[0]);
  return buffer + FIELD_OFFSET(user_bstr_t, data[wb->n]);
}

unsigned char * __RPC_USER
bstr_t_UserUnmarshal(ULONG *flags, unsigned char *buffer, bstr_t *b)
{
  wire_bstr_t wb = (wire_bstr_t) buffer;
  short *data = HeapAlloc(GetProcessHeap(), 0, (wb->n + 1) * sizeof *data);
  data[0] = wb->n;
  memcpy(&data[1], wb->data, wb->n * sizeof data[1]);
  *b = &data[1];
  return buffer + FIELD_OFFSET(user_bstr_t, data[wb->n]);
}

void __RPC_USER
bstr_t_UserFree(ULONG *flags, bstr_t *b)
{
  HeapFree(GetProcessHeap(), 0, &((*b)[-1]));
}

static void
pointer_tests(void)
{
  int a[] = {1, 2, 3, 4};
  char p1[] = "11";
  test_list_t *list = make_list(make_list(make_list(null_list())));
  test_us_t tus = {{p1}};
  int *pa[4];
  puints_t pus;
  cpuints_t cpus;
  short bstr_data[] = { 5, 'H', 'e', 'l', 'l', 'o' };
  bstr_t bstr = &bstr_data[1], bstr2;
  name_t name;
  void *buffer;
  int *pa2;
  s123_t *s123;
  int val = 42;

  ok(test_list_length(list) == 3, "RPC test_list_length\n");
  ok(square_puint(p1) == 121, "RPC square_puint\n");
  pus.n = 4;
  pus.ps = HeapAlloc(GetProcessHeap(), 0, pus.n * sizeof pus.ps[0]);
  pus.ps[0] = xstrdup("5");
  pus.ps[1] = xstrdup("6");
  pus.ps[2] = xstrdup("7");
  pus.ps[3] = xstrdup("8");
  ok(sum_puints(&pus) == 26, "RPC sum_puints\n");
  HeapFree(GetProcessHeap(), 0, pus.ps[0]);
  HeapFree(GetProcessHeap(), 0, pus.ps[1]);
  HeapFree(GetProcessHeap(), 0, pus.ps[2]);
  HeapFree(GetProcessHeap(), 0, pus.ps[3]);
  HeapFree(GetProcessHeap(), 0, pus.ps);
  cpus.n = 4;
  cpus.ps = HeapAlloc(GetProcessHeap(), 0, cpus.n * sizeof cpus.ps[0]);
  cpus.ps[0] = xstrdup("5");
  cpus.ps[1] = xstrdup("6");
  cpus.ps[2] = xstrdup("7");
  cpus.ps[3] = xstrdup("8");
  ok(sum_cpuints(&cpus) == 26, "RPC sum_puints\n");
  HeapFree(GetProcessHeap(), 0, cpus.ps[0]);
  HeapFree(GetProcessHeap(), 0, cpus.ps[1]);
  HeapFree(GetProcessHeap(), 0, cpus.ps[2]);
  HeapFree(GetProcessHeap(), 0, cpus.ps[3]);
  HeapFree(GetProcessHeap(), 0, cpus.ps);
  ok(square_test_us(&tus) == 121, "RPC square_test_us\n");

  pa[0] = &a[0];
  pa[1] = &a[1];
  pa[2] = &a[2];
  ok(sum_parr(pa) == 6, "RPC sum_parr\n");

  pa[0] = &a[0];
  pa[1] = &a[1];
  pa[2] = &a[2];
  pa[3] = &a[3];
  ok(sum_pcarr(pa, 4) == 10, "RPC sum_pcarr\n");

  ok(hash_bstr(bstr) == s_hash_bstr(bstr), "RPC hash_bstr_data\n");

  get_a_bstr(&bstr);
  s_get_a_bstr(&bstr2);
  ok(!lstrcmpW((LPCWSTR)bstr, (LPCWSTR)bstr2), "bstr mismatch\n");
  HeapFree(GetProcessHeap(), 0, bstr - 1);
  HeapFree(GetProcessHeap(), 0, bstr2 - 1);

  free_list(list);

  if (!old_windows_version)
  {
      int n;
      str_array_t names;
      wstr_array_t namesw;

      name.size = 10;
      name.name = buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, name.size);
      get_name(&name);
      ok(name.name == buffer, "[in,out] pointer should have stayed as %p but instead changed to %p\n", name.name, buffer);
      ok(!strcmp(name.name, "Jeremy Wh"), "name didn't unmarshall properly, expected \"Jeremy Wh\", but got \"%s\"\n", name.name);
      HeapFree(GetProcessHeap(), 0, name.name);

      if (!is_interp) { /* broken in widl */
      n = -1;
      names = NULL;
      get_names(&n, &names);
      ok(n == 2, "expected 2, got %d\n", n);
      ros_skip_flaky
      ok(!strcmp(names[0], "Hello"), "expected Hello, got %s\n", names[0]);
      ros_skip_flaky
      ok(!strcmp(names[1], "World!"), "expected World!, got %s\n", names[1]);
      MIDL_user_free(names[0]);
      MIDL_user_free(names[1]);
      MIDL_user_free(names);

      n = -1;
      namesw = NULL;
      get_namesw(&n, &namesw);
      ok(n == 2, "expected 2, got %d\n", n);
      ros_skip_flaky
      ok(!lstrcmpW(namesw[0], helloW), "expected Hello, got %s\n", wine_dbgstr_w(namesw[0]));
      ros_skip_flaky
      ok(!lstrcmpW(namesw[1], worldW), "expected World!, got %s\n", wine_dbgstr_w(namesw[1]));
      MIDL_user_free(namesw[0]);
      MIDL_user_free(namesw[1]);
      MIDL_user_free(namesw);
      }
  }

  if (!is_interp) { /* broken in widl */
  pa2 = a;
  ros_skip_flaky
  ok(sum_pcarr2(4, &pa2) == 10, "RPC sum_pcarr2\n");
  }

  s123 = get_s123();
  ok(s123->f1 == 1 && s123->f2 == 2 && s123->f3 == 3, "RPC get_s123\n");
  MIDL_user_free(s123);

  full_pointer_test(&val, &val);
  full_pointer_null_test(&val, NULL);
}

static int
check_pyramid_doub_carr(doub_carr_t *dc)
{
  int i, j;
  for (i = 0; i < dc->n; ++i)
    for (j = 0; j < dc->a[i]->n; ++j)
      if (dc->a[i]->a[j] != j + 1)
        return FALSE;
  return TRUE;
}

static void
free_pyramid_doub_carr(doub_carr_t *dc)
{
  int i;
  for (i = 0; i < dc->n; ++i)
    MIDL_user_free(dc->a[i]);
  MIDL_user_free(dc);
}

static void
array_tests(void)
{
  int m[2][3][4] =
  {
    {{1, 2, 3, 4}, {-1, -3, -5, -7}, {0, 2, 4, 6}},
    {{1, -2, 3, -4}, {2, 3, 5, 7}, {-4, -1, -14, 4114}}
  };
  int c[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  int c2[] = {10, 100, 200};
  int c3[20];
  vector_t vs[2] = {{1, -2, 3}, {4, -5, -6}};
  cps_t cps;
  cpsc_t cpsc;
  cs_t *cs;
  int n;
  int ca[5] = {1, -2, 3, -4, 5};
  int tmp[10];
  doub_carr_t *dc;
  int *pi;
  pints_t api[5];
  numbers_struct_t *ns;
  refpint_t rpi[5];
  int i0 = 1, i1 = 2, *ptr_array[2] = {&i0, &i1}, array[2] = {3, 4};

  if (!old_windows_version)
  {
      const char str1[25] = "Hello";
      ok(cstr_length(str1, sizeof str1) == strlen(str1), "RPC cstr_length\n");
  }

  ok(sum_fixed_int_3d(m) == 4116, "RPC sum_fixed_int_3d\n");

  ok(sum_conf_array(c, 10) == 45, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[5], 2) == 11, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[7], 1) == 7, "RPC sum_conf_array\n");
  ok(sum_conf_array(&c[2], 0) == 0, "RPC sum_conf_array\n");

  ok(sum_conf_ptr_by_conf_ptr(1, c2, c) == 45, "RPC sum_conf_ptr_by_conf_ptr\n");
  ok(sum_conf_ptr_by_conf_ptr(3, c2, c) == 345, "RPC sum_conf_ptr_by_conf_ptr\n");
  c2[0] = 0;
  ok(sum_conf_ptr_by_conf_ptr(3, c2, c) == 300, "RPC sum_conf_ptr_by_conf_ptr\n");

  ok(sum_unique_conf_array(ca, 4) == -2, "RPC sum_unique_conf_array\n");
  ok(sum_unique_conf_ptr(ca, 5) == 3, "RPC sum_unique_conf_array\n");
  ok(sum_unique_conf_ptr(NULL, 10) == 0, "RPC sum_unique_conf_array\n");

  get_number_array(c3, &n);
  ok(n == 10, "RPC get_num_array\n");
  for (; n > 0; n--)
    ok(c3[n-1] == c[n-1], "get_num_array returned wrong value %d @ %d\n",
       c3[n-1], n);
  ok(sum_var_array(c, 10) == 45, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[5], 2) == 11, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[7], 1) == 7, "RPC sum_conf_array\n");
  ok(sum_var_array(&c[2], 0) == 0, "RPC sum_conf_array\n");

  ok(dot_two_vectors(vs) == -4, "RPC dot_two_vectors\n");
  cs = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(cs_t, ca[5]));
  cs->n = 5;
  cs->ca[0] = 3;
  cs->ca[1] = 5;
  cs->ca[2] = -2;
  cs->ca[3] = -1;
  cs->ca[4] = -4;
  ok(sum_cs(cs) == 1, "RPC sum_cs\n");
  HeapFree(GetProcessHeap(), 0, cs);

  n = 5;
  cps.pn = &n;
  cps.ca1 = &c[2];
  cps.n = 3;
  cps.ca2 = &c[3];
  ok(sum_cps(&cps) == 53, "RPC sum_cps\n");

  cpsc.a = 4;
  cpsc.b = 5;
  cpsc.c = 1;
  cpsc.ca = c;
  ok(sum_cpsc(&cpsc) == 6, "RPC sum_cpsc\n");
  cpsc.a = 4;
  cpsc.b = 5;
  cpsc.c = 0;
  cpsc.ca = c;
  ok(sum_cpsc(&cpsc) == 10, "RPC sum_cpsc\n");

  cpsc.ca = NULL;
  ok(get_cpsc(5, &cpsc) == 45, "RPC sum_cpsc\n");
  ok( cpsc.a == 10, "RPC get_cpsc %u\n", cpsc.a );
  for (n = 0; n < 10; n++) ok( cpsc.ca[n] == n, "RPC get_cpsc[%d] = %d\n", n, cpsc.ca[n] );

  memset( tmp, 0x33, sizeof(tmp) );
  cpsc.ca = tmp;
  ok(get_cpsc(4, &cpsc) == 28, "RPC sum_cpsc\n");
  ok( cpsc.a == 8, "RPC get_cpsc %u\n", cpsc.a );
  ok( cpsc.ca == tmp, "RPC get_cpsc %p/%p\n", cpsc.ca, tmp );
  for (n = 0; n < 8; n++) ok( cpsc.ca[n] == n, "RPC get_cpsc[%d] = %d\n", n, cpsc.ca[n] );

  ok(sum_toplev_conf_2n(c, 3) == 15, "RPC sum_toplev_conf_2n\n");
  ok(sum_toplev_conf_cond(c, 5, 6, 1) == 10, "RPC sum_toplev_conf_cond\n");
  ok(sum_toplev_conf_cond(c, 5, 6, 0) == 15, "RPC sum_toplev_conf_cond\n");

  dc = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(doub_carr_t, a[2]));
  dc->n = 2;
  dc->a[0] = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(doub_carr_1_t, a[3]));
  dc->a[0]->n = 3;
  dc->a[0]->a[0] = 5;
  dc->a[0]->a[1] = 1;
  dc->a[0]->a[2] = 8;
  dc->a[1] = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(doub_carr_1_t, a[2]));
  dc->a[1]->n = 2;
  dc->a[1]->a[0] = 2;
  dc->a[1]->a[1] = 3;
  ok(sum_doub_carr(dc) == 19, "RPC sum_doub_carr\n");
  HeapFree(GetProcessHeap(), 0, dc->a[0]);
  HeapFree(GetProcessHeap(), 0, dc->a[1]);
  HeapFree(GetProcessHeap(), 0, dc);

  dc = NULL;
  make_pyramid_doub_carr(4, &dc);
  ok(check_pyramid_doub_carr(dc), "RPC make_pyramid_doub_carr\n");
  free_pyramid_doub_carr(dc);

  ok(sum_L1_norms(2, vs) == 21, "RPC sum_L1_norms\n");

  memset(api, 0, sizeof(api));
  pi = HeapAlloc(GetProcessHeap(), 0, sizeof(*pi));
  *pi = -1;
  api[0].pi = pi;
  get_numbers(1, 1, api);
  ok(api[0].pi == pi, "RPC conformant varying array [out] pointer changed from %p to %p\n", pi, api[0].pi);
  ok(*api[0].pi == 0, "pi unmarshalled incorrectly %d\n", *api[0].pi);

  if (!old_windows_version)
  {
      ns = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FIELD_OFFSET(numbers_struct_t, numbers[5]));
      ns->length = 5;
      ns->size = 5;
      ns->numbers[0].pi = pi;
      get_numbers_struct(&ns);
      ok(ns->numbers[0].pi == pi, "RPC conformant varying struct embedded pointer changed from %p to %p\n", pi, ns->numbers[0].pi);
      ok(*ns->numbers[0].pi == 5, "pi unmarshalled incorrectly %d\n", *ns->numbers[0].pi);
      HeapFree(GetProcessHeap(), 0, ns);
  }
  HeapFree(GetProcessHeap(), 0, pi);

  pi = HeapAlloc(GetProcessHeap(), 0, 5 * sizeof(*pi));
  pi[0] = 3;  rpi[0] = &pi[0];
  pi[1] = 5;  rpi[1] = &pi[1];
  pi[2] = -2; rpi[2] = &pi[2];
  pi[3] = -1; rpi[3] = &pi[3];
  pi[4] = -4; rpi[4] = &pi[4];
  ok(sum_complex_array(5, rpi) == 1, "RPC sum_complex_array\n");
  HeapFree(GetProcessHeap(), 0, pi);

  ok(sum_ptr_array(ptr_array) == 3, "RPC sum_ptr_array\n");
  ok(sum_array_ptr(&array) == 7, "RPC sum_array_ptr\n");
}

void __cdecl s_authinfo_test(unsigned int protseq, int secure)
{
    RPC_BINDING_HANDLE binding;
    RPC_STATUS status;
    ULONG level, authnsvc;
    RPC_AUTHZ_HANDLE privs;
    unsigned char *principal;

    binding = I_RpcGetCurrentCallHandle();
    ok(binding != NULL, "I_RpcGetCurrentCallHandle returned NULL\n");

    level = authnsvc = 0xdeadbeef;
    privs = (RPC_AUTHZ_HANDLE)0xdeadbeef;
    principal = (unsigned char *)0xdeadbeef;

    if (secure || protseq == RPC_PROTSEQ_LRPC)
    {
        status = RpcBindingInqAuthClientA(binding, &privs, &principal, &level, &authnsvc, NULL);
        if (status == RPC_S_CANNOT_SUPPORT)
        {
            win_skip("RpcBindingInqAuthClientA not supported\n");
            return;
        }
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        ok(privs != (RPC_AUTHZ_HANDLE)0xdeadbeef, "privs unchanged\n");
        ok(principal != (unsigned char *)0xdeadbeef, "principal unchanged\n");
        if (protseq != RPC_PROTSEQ_LRPC)
        {
            todo_wine
            ok(principal != NULL, "NULL principal\n");
        }
        if (protseq == RPC_PROTSEQ_LRPC && principal)
        {
            int len;
            char *spn;

            len = WideCharToMultiByte(CP_ACP, 0, (const WCHAR *)privs, -1, NULL, 0, NULL, NULL);
            spn = HeapAlloc( GetProcessHeap(), 0, len );
            WideCharToMultiByte(CP_ACP, 0, (const WCHAR *)privs, -1, spn, len, NULL, NULL);

            ok(!strcmp(domain_and_user, spn), "expected %s got %s\n", domain_and_user, spn);
            HeapFree( GetProcessHeap(), 0, spn );
        }
        ok(level == RPC_C_AUTHN_LEVEL_PKT_PRIVACY, "level unchanged\n");
        ok(authnsvc == RPC_C_AUTHN_WINNT, "authnsvc unchanged\n");
        RpcStringFreeA(&principal);

        status = RpcBindingInqAuthClientA(NULL, &privs, &principal, &level, &authnsvc, NULL);
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        RpcStringFreeA(&principal);

        status = RpcBindingInqAuthClientExA(NULL, &privs, &principal, &level, &authnsvc, NULL, 0);
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        RpcStringFreeA(&principal);

        status = RpcImpersonateClient(NULL);
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);
        status = RpcRevertToSelf();
        ok(status == RPC_S_OK, "expected RPC_S_OK got %u\n", status);

    }
    else
    {
        status = RpcBindingInqAuthClientA(binding, &privs, &principal, &level, &authnsvc, NULL);
        ok(status == RPC_S_BINDING_HAS_NO_AUTH, "expected RPC_S_BINDING_HAS_NO_AUTH got %u\n", status);
        ok(privs == (RPC_AUTHZ_HANDLE)0xdeadbeef, "got %p\n", privs);
        ok(principal == (unsigned char *)0xdeadbeef, "got %s\n", principal);
        ok(level == 0xdeadbeef, "got %u\n", level);
        ok(authnsvc == 0xdeadbeef, "got %u\n", authnsvc);
    }
}

static void test_handle_return(void)
{
    ctx_handle_t handle, handle2;

    handle = get_handle();
    test_handle(handle);
    get_handle_by_ptr(&handle2);
    test_handle(handle2);
}

static void
run_tests(void)
{
  basic_tests();
  union_tests();
  pointer_tests();
  array_tests();
  context_handle_test();
  test_handle_return();
}

static void
set_auth_info(RPC_BINDING_HANDLE handle)
{
    RPC_STATUS status;
    RPC_SECURITY_QOS qos;

    qos.Version = 1;
    qos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_STATIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

    status = pRpcBindingSetAuthInfoExA(handle, (RPC_CSTR)domain_and_user, RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                       RPC_C_AUTHN_WINNT, NULL, 0, &qos);
    ok(status == RPC_S_OK, "RpcBindingSetAuthInfoExA failed %d\n", status);
}

#define test_is_server_listening(a,b) _test_is_server_listening(__LINE__,a,b)
static void _test_is_server_listening(unsigned line, RPC_BINDING_HANDLE binding, RPC_STATUS expected_status)
{
    RPC_STATUS status;
    status = RpcMgmtIsServerListening(binding);
    ok_(__FILE__,line)(status == expected_status, "RpcMgmtIsServerListening returned %u, expected %u\n",
                       status, expected_status);
}

#define test_is_server_listening2(a,b,c) _test_is_server_listening2(__LINE__,a,b,c)
static void _test_is_server_listening2(unsigned line, RPC_BINDING_HANDLE binding, RPC_STATUS expected_status,
        RPC_STATUS expected_status2)
{
    RPC_STATUS status;
    status = RpcMgmtIsServerListening(binding);
    ok_(__FILE__,line)(status == expected_status || status == expected_status2,
                       "RpcMgmtIsServerListening returned %u, expected %u or %u\n",
                       status, expected_status, expected_status2);
}

static void
client(const char *test)
{
  static unsigned char iptcp[] = "ncacn_ip_tcp";
  static unsigned char np[] = "ncacn_np";
  static unsigned char ncalrpc[] = "ncalrpc";
  static unsigned char address[] = "127.0.0.1";
  static unsigned char address_np[] = "\\\\.";
  static unsigned char port[] = PORT;
  static unsigned char pipe[] = PIPE;
  static unsigned char guid[] = "00000000-4114-0704-2301-000000000000";

  unsigned char *binding;

  if (strcmp(test, "tcp_basic") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, iptcp, address, port, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    run_tests();
    authinfo_test(RPC_PROTSEQ_TCP, 0);
    test_is_server_listening2(IMixedServer_IfHandle, RPC_S_OK, RPC_S_ACCESS_DENIED);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "tcp_secure") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, iptcp, address, port, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    set_auth_info(IMixedServer_IfHandle);
    authinfo_test(RPC_PROTSEQ_TCP, 1);
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_ACCESS_DENIED);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "ncalrpc_basic") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, ncalrpc, NULL, guid, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    run_tests(); /* can cause RPC_X_BAD_STUB_DATA exception */
    authinfo_test(RPC_PROTSEQ_LRPC, 0);
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_OK);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "ncalrpc_autolisten") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, ncalrpc, NULL, guid, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    run_tests();
    authinfo_test(RPC_PROTSEQ_LRPC, 0);
todo_wine
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_NOT_LISTENING);

    stop_autolisten();
    ok(int_return() == INT_CODE, "RPC int_return\n");

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "ncalrpc_secure") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, ncalrpc, NULL, guid, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    set_auth_info(IMixedServer_IfHandle);
    authinfo_test(RPC_PROTSEQ_LRPC, 1);
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_OK);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "np_basic") == 0)
  {
    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, np, address_np, pipe, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    test_is_server_listening(IMixedServer_IfHandle, RPC_S_OK);
    run_tests();
    authinfo_test(RPC_PROTSEQ_NMP, 0);
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_OK);
    stop();
    test_is_server_listening(IMixedServer_IfHandle, RPC_S_NOT_LISTENING);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
  }
  else if (strcmp(test, "np_basic_interp") == 0)
  {
    set_interp_interface();

    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, np, address_np, pipe, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IInterpServer_IfHandle), "RpcBindingFromStringBinding\n");

    test_is_server_listening(IInterpServer_IfHandle, RPC_S_OK);
    run_tests();
    authinfo_test(RPC_PROTSEQ_NMP, 0);
    test_is_server_listening(IInterpServer_IfHandle, RPC_S_OK);

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IInterpServer_IfHandle), "RpcBindingFree\n");
  }
}

static void
server(void)
{
  static unsigned char iptcp[] = "ncacn_ip_tcp";
  static unsigned char port[] = PORT;
  static unsigned char np[] = "ncacn_np";
  static unsigned char pipe[] = PIPE;
  static unsigned char ncalrpc[] = "ncalrpc";
  static unsigned char guid[] = "00000000-4114-0704-2301-000000000000";
  RPC_STATUS status, iptcp_status, np_status, ncalrpc_status;
  DWORD ret;

  /* needed for tests involving interface pointers */
  CoInitializeEx(NULL, COINIT_MULTITHREADED);

  iptcp_status = RpcServerUseProtseqEpA(iptcp, 20, port, NULL);
  ok(iptcp_status == RPC_S_OK, "RpcServerUseProtseqEp(ncacn_ip_tcp) failed with status %d\n", iptcp_status);

  ncalrpc_status = RpcServerUseProtseqEpA(ncalrpc, 0, guid, NULL);
  ok(ncalrpc_status == RPC_S_OK, "RpcServerUseProtseqEp(ncalrpc) failed with status %d\n", ncalrpc_status);

  np_status = RpcServerUseProtseqEpA(np, 0, pipe, NULL);
  if (np_status == RPC_S_PROTSEQ_NOT_SUPPORTED)
    skip("Protocol sequence ncacn_np is not supported\n");
  else
    ok(np_status == RPC_S_OK, "RpcServerUseProtseqEp(ncacn_np) failed with status %d\n", np_status);

  if (pRpcServerRegisterIfEx)
  {
    trace("Using RpcServerRegisterIfEx\n");
    status = pRpcServerRegisterIfEx(s_IMixedServer_v0_0_s_ifspec, NULL, NULL,
                                    RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
                                    RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIfEx failed with status %d\n", status);
    status = pRpcServerRegisterIfEx(s_IInterpServer_v0_0_s_ifspec, NULL, NULL,
                                    RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
                                    RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIfEx failed with status %d\n", status);
  }
  else
  {
    status = RpcServerRegisterIf(s_IMixedServer_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed with status %d\n", status);
    status = RpcServerRegisterIf(s_IInterpServer_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed with status %d\n", status);
  }
  test_is_server_listening(NULL, RPC_S_NOT_LISTENING);
  status = RpcServerListen(1, 20, TRUE);
  ok(status == RPC_S_OK, "RpcServerListen failed with status %d\n", status);
  test_is_server_listening(NULL, RPC_S_OK);
  stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
  ok(stop_event != NULL, "CreateEvent failed with error %d\n", GetLastError());

  if (iptcp_status == RPC_S_OK)
    run_client("tcp_basic");
  else
    skip("tcp tests skipped due to earlier failure\n");

  if (ncalrpc_status == RPC_S_OK)
  {
    run_client("ncalrpc_basic");

    /* we don't need to register RPC_C_AUTHN_WINNT for ncalrpc */
    run_client("ncalrpc_secure");
  }
  else
    skip("lrpc tests skipped due to earlier failure\n");

  if (np_status == RPC_S_OK)
  {
    run_client("np_basic_interp");
    run_client("np_basic");
  }
  else
  {
    skip("np_basic tests skipped due to earlier failure\n");
    /* np client is what signals stop_event, so bail out if we didn't run do it */
    return;
  }

  ret = WaitForSingleObject(stop_event, 1000);
  ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");

  /* if the stop event didn't fire then RpcMgmtWaitServerListen will wait
   * forever, so don't bother calling it in this case */
  if (ret == WAIT_OBJECT_0)
  {
    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListening failed with status %d\n", status);
  }

  CloseHandle(stop_event);
  stop_event = NULL;

  if (pRpcServerRegisterIfEx)
  {
    status = pRpcServerRegisterIfEx(s_IMixedServer_v0_0_s_ifspec, NULL, NULL,
        RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH | RPC_IF_AUTOLISTEN,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf() failed: %u\n", status);

    run_client("ncalrpc_autolisten");

    status = RpcServerUnregisterIf(s_IMixedServer_v0_0_s_ifspec, NULL, TRUE);
    ok(status == RPC_S_OK, "RpcServerUnregisterIf() failed: %u\n", status);
  }

  CoUninitialize();
}

static DWORD WINAPI listen_test_client_thread(void *binding)
{
    RPC_STATUS status;

    status = RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFromStringBinding\n");

    test_is_server_listening(IMixedServer_IfHandle, RPC_S_OK);
    stop();
    trace("stopped\n");

    status = RpcBindingFree(&IMixedServer_IfHandle);
    ok(status == RPC_S_OK, "RpcBindingFree\n");
    return 0;
}

static DWORD WINAPI wait_listen_proc(void *arg)
{
    RPC_STATUS status;

    trace("waiting\n");
    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListening failed with status %d\n", status);
    trace("done\n");

    return 0;
}

static void test_stop_wait_for_call(unsigned char *binding)
{
    HANDLE client_thread, wait_listen_thread;
    RPC_STATUS status;
    DWORD ret;

    status = RpcServerListen(1, 20, TRUE);
    ok(status == RPC_S_OK, "RpcServerListen failed with status %d\n", status);
    test_is_server_listening(NULL, RPC_S_OK);

    stop_wait_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(stop_wait_event != NULL, "CreateEvent failed with error %d\n", GetLastError());
    stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(stop_event != NULL, "CreateEvent failed with error %d\n", GetLastError());

    wait_listen_thread = CreateThread(NULL, 0, wait_listen_proc, 0, 0, NULL);
    ok(wait_listen_thread != NULL, "CreateThread failed\n");

    client_thread = CreateThread(NULL, 0, listen_test_client_thread, binding, 0, NULL);
    ok(client_thread != NULL, "CreateThread failed\n");
    CloseHandle(client_thread);

    ret = WaitForSingleObject(stop_event, 10000);
    ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening\n");
    test_is_server_listening(NULL, RPC_S_NOT_LISTENING);

    ret = WaitForSingleObject(wait_listen_thread, 500);
    ok(WAIT_TIMEOUT == ret, "WaitForSingleObject\n");

    SetEvent(stop_wait_event);

    ret = WaitForSingleObject(wait_listen_thread, 10000);
    ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject returned %u\n", ret);

    CloseHandle(wait_listen_thread);

    CloseHandle(stop_wait_event);
    stop_wait_event = NULL;
    CloseHandle(stop_event);
    stop_event = NULL;
}

static void test_server_listening(void)
{
    static unsigned char np[] = "ncacn_np";
    static unsigned char address_np[] = "\\\\.";
    static unsigned char pipe[] = PIPE "listen_test";
    static unsigned char ncalrpc[] = "ncalrpc";
    static unsigned char guid[] = "00000000-4114-0704-2302-000000000000";
    unsigned char *binding;
    RPC_STATUS status;

    status = RpcServerUseProtseqEpA(np, 0, pipe, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp(ncacn_np) failed with status %d\n", status);

    status = RpcServerRegisterIf(s_IMixedServer_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed with status %d\n", status);

    test_is_server_listening(NULL, RPC_S_NOT_LISTENING);
    status = RpcServerListen(1, 20, TRUE);
    ok(status == RPC_S_OK, "RpcServerListen failed with status %d\n", status);
    test_is_server_listening(NULL, RPC_S_OK);

    status = RpcServerListen(1, 20, TRUE);
    ok(status == RPC_S_ALREADY_LISTENING, "RpcServerListen failed with status %d\n", status);

    status = RpcMgmtStopServerListening(NULL);
    ok(status == RPC_S_OK, "RpcMgmtStopServerListening\n");
    test_is_server_listening(NULL, RPC_S_NOT_LISTENING);

    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListening failed with status %d\n", status);

    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_NOT_LISTENING, "RpcMgmtWaitServerListening failed with status %d\n", status);

    /* test that server stop waits for a call in progress */
    status = RpcStringBindingComposeA(NULL, np, address_np, pipe, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose\n");

    test_stop_wait_for_call(binding);

    status = RpcStringFreeA(&binding);
    ok(status == RPC_S_OK, "RpcStringFree\n");

    /* repeat the test using ncalrpc */
    status = RpcServerUseProtseqEpA(ncalrpc, 0, guid, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp(ncalrpc) failed with status %d\n", status);

    status = RpcStringBindingComposeA(NULL, ncalrpc, NULL, guid, NULL, &binding);
    ok(status == RPC_S_OK, "RpcStringBindingCompose\n");

    test_stop_wait_for_call(binding);

    status = RpcStringFreeA(&binding);
    ok(status == RPC_S_OK, "RpcStringFree\n");
}

static HANDLE create_server_process(void)
{
    SECURITY_ATTRIBUTES sec_attr = { sizeof(sec_attr), NULL, TRUE };
    HANDLE ready_event;
    char cmdline[MAX_PATH];
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD ret;

    memset(&startup, 0, sizeof startup);
    startup.cb = sizeof startup;

    ready_event = CreateEventW(&sec_attr, TRUE, FALSE, NULL);
    ok(ready_event != NULL, "CreateEvent failed: %u\n", GetLastError());

#ifdef __REACTOS__
    sprintf(cmdline, "%s server run %Ix", progname, (UINT_PTR)ready_event);
#else
    sprintf(cmdline, "%s server run %lx", progname, (UINT_PTR)ready_event);
#endif
    trace("running server process...\n");
    ok(CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    ret = WaitForSingleObject(ready_event, 10000);
    ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");

    ok(CloseHandle(info.hThread), "CloseHandle\n");
    ok(CloseHandle(ready_event), "CloseHandle\n");
    return info.hProcess;
}

static void run_server(HANDLE ready_event)
{
    static unsigned char np[] = "ncacn_np";
    static unsigned char pipe[] = PIPE "term_test";
    RPC_STATUS status;
    BOOL ret;

    status = RpcServerUseProtseqEpA(np, 0, pipe, NULL);
    ok(status == RPC_S_OK, "RpcServerUseProtseqEp(ncacn_np) failed with status %d\n", status);

    status = RpcServerRegisterIf(s_IMixedServer_v0_0_s_ifspec, NULL, NULL);
    ok(status == RPC_S_OK, "RpcServerRegisterIf failed with status %d\n", status);

    test_is_server_listening(NULL, RPC_S_NOT_LISTENING);
    status = RpcServerListen(1, 20, TRUE);
    ok(status == RPC_S_OK, "RpcServerListen failed with status %d\n", status);

    stop_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(stop_event != NULL, "CreateEvent failed with error %d\n", GetLastError());

    ret = SetEvent(ready_event);
    ok(ret, "SetEvent failed: %u\n", GetLastError());

    ret = WaitForSingleObject(stop_event, 1000);
    ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");

    status = RpcMgmtWaitServerListen();
    ok(status == RPC_S_OK, "RpcMgmtWaitServerListening failed with status %d\n", status);

    CloseHandle(stop_event);
    stop_event = NULL;
}

static DWORD WINAPI basic_tests_thread(void *arg)
{
    basic_tests();
    return 0;
}

static void test_reconnect(void)
{
    static unsigned char np[] = "ncacn_np";
    static unsigned char address_np[] = "\\\\.";
    static unsigned char pipe[] = PIPE "term_test";
    unsigned char *binding;
    HANDLE threads[32];
    HANDLE server_process;
    unsigned i;
    DWORD ret;

    server_process = create_server_process();

    ok(RPC_S_OK == RpcStringBindingComposeA(NULL, np, address_np, pipe, NULL, &binding), "RpcStringBindingCompose\n");
    ok(RPC_S_OK == RpcBindingFromStringBindingA(binding, &IMixedServer_IfHandle), "RpcBindingFromStringBinding\n");

    for (i = 0; i < ARRAY_SIZE(threads); i++)
    {
        threads[i] = CreateThread(NULL, 0, basic_tests_thread, 0, 0, NULL);
        ok(threads[i] != NULL, "CreateThread failed: %u\n", GetLastError());
    }

    for (i = 0; i < ARRAY_SIZE(threads); i++)
    {
        ret = WaitForSingleObject(threads[i], 10000);
        ok(WAIT_OBJECT_0 == ret, "WaitForSingleObject\n");
        CloseHandle(threads[i]);
    }

    stop();

    winetest_wait_child_process(server_process);
    ok(CloseHandle(server_process), "CloseHandle\n");

    /* create new server, rpcrt4 will connect to it once sending to existing connection fails
     * that current connection is broken. */
    server_process = create_server_process();
    basic_tests();
    stop();

    winetest_wait_child_process(server_process);
    ok(CloseHandle(server_process), "CloseHandle\n");

    ok(RPC_S_OK == RpcStringFreeA(&binding), "RpcStringFree\n");
    ok(RPC_S_OK == RpcBindingFree(&IMixedServer_IfHandle), "RpcBindingFree\n");
}

static BOOL is_process_elevated(void)
{
    HANDLE token;
    if (OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ))
    {
        TOKEN_ELEVATION_TYPE type;
        DWORD size;
        BOOL ret;

        ret = GetTokenInformation( token, TokenElevationType, &type, sizeof(type), &size );
        CloseHandle( token );
        return (ret && type == TokenElevationTypeFull);
    }
    return FALSE;
}

static BOOL is_firewall_enabled(void)
{
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    VARIANT_BOOL enabled = VARIANT_FALSE;

    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_FirewallEnabled( profile, &enabled );
    ok( hr == S_OK, "got %08x\n", hr );

done:
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    return (enabled == VARIANT_TRUE);
}

enum firewall_op
{
    APP_ADD,
    APP_REMOVE
};

static HRESULT set_firewall( enum firewall_op op )
{
    static const WCHAR testW[] = {'r','p','c','r','t','4','_','t','e','s','t',0};
    HRESULT hr, init;
    INetFwMgr *mgr = NULL;
    INetFwPolicy *policy = NULL;
    INetFwProfile *profile = NULL;
    INetFwAuthorizedApplication *app = NULL;
    INetFwAuthorizedApplications *apps = NULL;
    BSTR name, image = SysAllocStringLen( NULL, MAX_PATH );

    if (!GetModuleFileNameW( NULL, image, MAX_PATH ))
    {
        SysFreeString( image );
        return E_FAIL;
    }
    init = CoInitializeEx( 0, COINIT_APARTMENTTHREADED );

    hr = CoCreateInstance( &CLSID_NetFwMgr, NULL, CLSCTX_INPROC_SERVER, &IID_INetFwMgr,
                           (void **)&mgr );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwMgr_get_LocalPolicy( mgr, &policy );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwPolicy_get_CurrentProfile( policy, &profile );
    if (hr != S_OK) goto done;

    hr = INetFwProfile_get_AuthorizedApplications( profile, &apps );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = CoCreateInstance( &CLSID_NetFwAuthorizedApplication, NULL, CLSCTX_INPROC_SERVER,
                           &IID_INetFwAuthorizedApplication, (void **)&app );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    hr = INetFwAuthorizedApplication_put_ProcessImageFileName( app, image );
    if (hr != S_OK) goto done;

    name = SysAllocString( testW );
    hr = INetFwAuthorizedApplication_put_Name( app, name );
    SysFreeString( name );
    ok( hr == S_OK, "got %08x\n", hr );
    if (hr != S_OK) goto done;

    if (op == APP_ADD)
        hr = INetFwAuthorizedApplications_Add( apps, app );
    else if (op == APP_REMOVE)
        hr = INetFwAuthorizedApplications_Remove( apps, image );
    else
        hr = E_INVALIDARG;

done:
    if (app) INetFwAuthorizedApplication_Release( app );
    if (apps) INetFwAuthorizedApplications_Release( apps );
    if (policy) INetFwPolicy_Release( policy );
    if (profile) INetFwProfile_Release( profile );
    if (mgr) INetFwMgr_Release( mgr );
    if (SUCCEEDED( init )) CoUninitialize();
    SysFreeString( image );
    return hr;
}

START_TEST(server)
{
  ULONG size = 0;
  int argc;
  char **argv;
  BOOL firewall_enabled = is_firewall_enabled(), firewall_disabled = FALSE;

  InitFunctionPointers();
  set_mixed_interface();

  ok(!GetUserNameExA(NameSamCompatible, NULL, &size), "GetUserNameExA\n");
  domain_and_user = HeapAlloc(GetProcessHeap(), 0, size);
  ok(GetUserNameExA(NameSamCompatible, domain_and_user, &size), "GetUserNameExA\n");

  argc = winetest_get_mainargs(&argv);
  progname = argv[0];

  if (argc == 3)
  {
    RpcTryExcept
    {
      client(argv[2]);
    }
    RpcExcept(TRUE)
    {
      trace("Exception %d\n", RpcExceptionCode());
    }
    RpcEndExcept
  }
  else if (argc == 4)
  {
    if (!strcmp(argv[3], "listen"))
    {
      test_server_listening();
    }
    else if(!strcmp(argv[2], "run"))
    {
      UINT_PTR event;
#ifdef __REACTOS__
      sscanf(argv[3], "%Ix", &event);
#else
      sscanf(argv[3], "%lx", &event);
#endif
      run_server((HANDLE)event);
    }
  }
  else
  {
    if (firewall_enabled)
    {
      if (is_process_elevated())
      {
        HRESULT hr = set_firewall(APP_ADD);
        if (hr == S_OK)
        {
          firewall_enabled = FALSE;
          firewall_disabled = TRUE;
        }
        else
        {
          skip("can't authorize app in firewall %08x\n", hr);
        }
      }
      else
      {
          trace("no privileges, skipping tests to avoid firewall dialog\n");
      }
    }

    if (!firewall_enabled) server();

    /* Those tests cause occasional crashes on winxp and win2k3 */
    if (GetProcAddress(GetModuleHandleA("rpcrt4.dll"), "RpcExceptionFilter"))
        test_reconnect();
    else
        win_skip("Skipping reconnect tests on too old Windows version\n");

    run_client("test listen");
    if (firewall_disabled) set_firewall(APP_REMOVE);
  }

  HeapFree(GetProcessHeap(), 0, domain_and_user);
}
